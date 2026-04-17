/**
 * @file thingsboard.cpp
 * @brief Implementación del módulo ThingsBoard
 *
 * Características:
 *  - 7 shared attributes suscritos en una sola suscripción
 *  - 4 RPC server-side: lock, unlock, unlockTemporary, resetLockout
 *  - Telemetría: locked, accessGranted, accessMethod, failedAttempts, uptime
 *  - Atributos cliente al conectar: firmwareVersion, ipAddress, macAddress
 *  - Re-solicitud de shared attributes al reconectar
 */

#include "thingsboard.h"
#include "../config/network_config.h"
#include "../config/config.h"
#include "../config/storage.h"
#include "../hardware/lock.h"

#include <Shared_Attribute_Update.h>
#include <Server_Side_RPC.h>

// ============================================
// INSTANCIAS MQTT / TB
// ============================================

static WiFiClient    wifiClient;
static Arduino_MQTT_Client mqttClient(wifiClient);

// Shared attributes: 1 suscripción, máx 7 atributos
static Shared_Attribute_Update<1U, 7U> sharedAttrUpdate;

// Server-side RPC: máx 2 callbacks, respuesta con hasta 2 pares clave-valor
static Server_Side_RPC<2U, 2U> serverSideRPC;

static const std::array<IAPI_Implementation*, 2U> apis = {
    &sharedAttrUpdate,
    &serverSideRPC
};

// Buffer envío 256 bytes, recepción 1024 bytes (suficiente para array credenciales ~450 bytes)
static ThingsBoardSized<64> tb(mqttClient, 256, 1024, Default_Max_Stack_Size, apis);

// ============================================
// ESTADO INTERNO
// ============================================

static bool     tbConnected               = false;
static uint32_t lastConnectAttempt        = 0;
static bool     isProcessingRemoteChange  = false;

// ============================================
// CALLBACKS REGISTRADOS EXTERNAMENTE
// ============================================

static RemoteStateCallback      stateChangeCb       = nullptr;
static CredentialsUpdateCallback credentialsUpdateCb = nullptr;
static AutoLockDelayCallback    autoLockDelayCb     = nullptr;
static NfcEnabledCallback       nfcEnabledCb        = nullptr;
static NfcUidCallback           nfcUidCb            = nullptr;
static MaxFailedCallback        maxFailedCb         = nullptr;
static LockoutDurationCallback  lockoutDurationCb   = nullptr;
static RpcUnlockTempCallback    rpcUnlockTempCb     = nullptr;
static RpcResetLockoutCallback  rpcResetLockoutCb   = nullptr;

// ============================================
// CALLBACK ATRIBUTOS COMPARTIDOS
// ============================================

/**
 * @brief Procesa todos los shared attributes recibidos desde el dashboard.
 *        Un solo callback cubre los 7 atributos configurados.
 */
static void processSharedAttributes(const JsonObjectConst& data)
{
    Serial.println("[TB] Shared attributes recibidos:");

    // ── lockState ────────────────────────────────────────────
    if (data.containsKey(ATTR_LOCK_STATE)) {
        const char* val = data[ATTR_LOCK_STATE];
        bool shouldLock = (strcmp(val, "locked") == 0);
        Serial.printf("  lockState = %s\n", val);

        if (!isProcessingRemoteChange && stateChangeCb) {
            isProcessingRemoteChange = true;
            stateChangeCb(shouldLock);
            isProcessingRemoteChange = false;
        }
    }

    // ── credenciales ─────────────────────────────────────────
    if (data.containsKey(ATTR_CREDENTIALS)) {
        String json;
        serializeJson(data[ATTR_CREDENTIALS], json);
        Serial.printf("  credenciales recibidas (%d bytes)\n", json.length());
        if (credentialsUpdateCb) credentialsUpdateCb(json.c_str());
    }

    // ── autoLockDelay ────────────────────────────────────────
    if (data.containsKey(ATTR_AUTO_LOCK_DELAY)) {
        uint32_t delayMs = data[ATTR_AUTO_LOCK_DELAY].as<uint32_t>();
        // Clampar a límites seguros
        if (delayMs < LOCK_AUTO_LOCK_DELAY_MIN) delayMs = LOCK_AUTO_LOCK_DELAY_MIN;
        if (delayMs > LOCK_AUTO_LOCK_DELAY_MAX) delayMs = LOCK_AUTO_LOCK_DELAY_MAX;
        Serial.printf("  autoLockDelay = %u ms\n", delayMs);
        storage_set_auto_lock_delay(delayMs);
        if (autoLockDelayCb) autoLockDelayCb(delayMs);
    }

    // ── nfcEnabled ───────────────────────────────────────────
    if (data.containsKey(ATTR_NFC_ENABLED)) {
        bool enabled = data[ATTR_NFC_ENABLED].as<bool>();
        Serial.printf("  nfcEnabled = %s\n", enabled ? "true" : "false");
        storage_set_nfc_enabled(enabled);
        if (nfcEnabledCb) nfcEnabledCb(enabled);
    }

    // ── authorizedNfcUid ─────────────────────────────────────
    if (data.containsKey(ATTR_NFC_UID)) {
        const char* uid = data[ATTR_NFC_UID];
        Serial.printf("  authorizedNfcUid = %s\n", uid);
        storage_set_nfc_uid(uid);
        if (nfcUidCb) nfcUidCb(uid);
    }

    // ── maxFailedAttempts ────────────────────────────────────
    if (data.containsKey(ATTR_MAX_FAILED)) {
        int maxFailed = data[ATTR_MAX_FAILED].as<int>();
        if (maxFailed < 1) maxFailed = 1;
        if (maxFailed > 10) maxFailed = 10;
        Serial.printf("  maxFailedAttempts = %d\n", maxFailed);
        storage_set_max_failed_attempts(maxFailed);
        if (maxFailedCb) maxFailedCb(maxFailed);
    }

    // ── lockoutDuration ──────────────────────────────────────
    if (data.containsKey(ATTR_LOCKOUT_DURATION)) {
        int seconds = data[ATTR_LOCKOUT_DURATION].as<int>();
        if (seconds < LOCKOUT_DURATION_MIN_S) seconds = LOCKOUT_DURATION_MIN_S;
        if (seconds > LOCKOUT_DURATION_MAX_S) seconds = LOCKOUT_DURATION_MAX_S;
        Serial.printf("  lockoutDuration = %d s\n", seconds);
        storage_set_lockout_duration(seconds);
        if (lockoutDurationCb) lockoutDurationCb(seconds);
    }
}

// ============================================
// CALLBACKS RPC
// Firma correcta del SDK 0.15.0:
//   void callback(JsonVariantConst const& params, JsonDocument& response)
// ============================================

static void on_rpc_unlock_temporary(JsonVariantConst const& data, JsonDocument& response)
{
    uint32_t duration = 5000;
    if (!data.isNull() && data.containsKey("duration")) {
        duration = data["duration"].as<uint32_t>();
    }
    // Clampar: mínimo 1s, máximo 5 min
    if (duration < 1000)   duration = 1000;
    if (duration > 300000) duration = 300000;

    Serial.printf("[TB] RPC: unlockTemporary %u ms\n", duration);
    if (rpcUnlockTempCb) rpcUnlockTempCb(duration);
    response["result"] = duration;
}

static void on_rpc_reset_lockout(JsonVariantConst const& data, JsonDocument& response)
{
    Serial.println("[TB] RPC: resetLockout");
    if (rpcResetLockoutCb) rpcResetLockoutCb();
    response["result"] = "ok";
}

// ============================================
// FUNCIONES PÚBLICAS
// ============================================

void thingsboard_init(void)
{
    Serial.println("[TB] Módulo ThingsBoard v2 inicializado");
}

bool thingsboard_connect(void)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[TB] WiFi no conectado");
        return false;
    }
    if (tb.connected()) return true;

    String tbServer = storage_get_tb_server();
    String tbToken  = storage_get_tb_token();

    Serial.printf("[TB] Conectando a %s:%d ...\n", tbServer.c_str(), THINGSBOARD_PORT);

    if (!tb.connect(tbServer.c_str(), tbToken.c_str(), THINGSBOARD_PORT)) {
        Serial.println("[TB] Error de conexión");
        tbConnected = false;
        return false;
    }

    Serial.println("[TB] Conectado!");
    tbConnected = true;

    // ── Suscribir a los 7 shared attributes ───────────────
    const std::array<const char*, 7U> attrs = {{
        ATTR_LOCK_STATE,
        ATTR_CREDENTIALS,
        ATTR_AUTO_LOCK_DELAY,
        ATTR_NFC_ENABLED,
        ATTR_NFC_UID,
        ATTR_MAX_FAILED,
        ATTR_LOCKOUT_DURATION
    }};
    Shared_Attribute_Callback<7U> attrCb(processSharedAttributes,
                                          attrs.cbegin(), attrs.cend());
    if (!sharedAttrUpdate.Shared_Attributes_Subscribe(attrCb)) {
        Serial.println("[TB] ERROR: No se pudo suscribir a shared attributes");
    } else {
        Serial.println("[TB] Suscrito a 7 shared attributes");
    }

    // ── Suscribir a RPC ───────────────────────────────────
    const std::array<RPC_Callback, 2U> rpcCbs = {{
        RPC_Callback{ "unlockTemporary", on_rpc_unlock_temporary },
        RPC_Callback{ "resetLockout",    on_rpc_reset_lockout    }
    }};
    if (!serverSideRPC.RPC_Subscribe(rpcCbs.cbegin(), rpcCbs.cend())) {
        Serial.println("[TB] ERROR: No se pudo suscribir a RPC");
    } else {
        Serial.println("[TB] Suscrito a 4 métodos RPC");
    }

    // ── Publicar atributos de cliente ─────────────────────
    thingsboard_publish_client_attributes();

    // ── Publicar estado inicial ───────────────────────────
    thingsboard_publish_lock_state(lock_get_state());

    return true;
}

void thingsboard_disconnect(void)
{
    if (tb.connected()) {
        tb.disconnect();
        tbConnected = false;
        Serial.println("[TB] Desconectado");
    }
}

void thingsboard_loop(void)
{
    if (WiFi.status() != WL_CONNECTED) return;

    if (!tb.connected()) {
        thingsboard_reconnect();
        return;
    }
    tb.loop();
}

bool thingsboard_is_connected(void)
{
    return tbConnected && tb.connected() && (WiFi.status() == WL_CONNECTED);
}

void thingsboard_reconnect(void)
{
    if (WiFi.status() != WL_CONNECTED) {
        WiFi.begin();
        return;
    }
    uint32_t now = millis();
    if (now - lastConnectAttempt >= THINGSBOARD_RECONNECT_INTERVAL_MS) {
        lastConnectAttempt = now;
        Serial.println("[TB] Intentando reconectar...");
        thingsboard_connect();
    }
}

// ── Publicación ──────────────────────────────────────────────

bool thingsboard_publish_lock_state(LockState state)
{
    if (!tb.connected()) return false;
    if (isProcessingRemoteChange) return true;  // evitar loop

    bool locked = (state == LockState::LOCKED);
    const char* stateStr = locked ? "locked" : "unlocked";

    // Telemetría (serie temporal)
    bool ok = tb.sendTelemetryData("locked", locked);
    // Atributo cliente (estado actual)
    ok |= tb.sendAttributeData(ATTR_LOCK_STATE, stateStr);

    Serial.printf("[TB] Estado publicado: %s\n", stateStr);
    return ok;
}

bool thingsboard_publish_access_event(bool granted, const char* method)
{
    if (!tb.connected()) return false;

    bool ok = true;
    ok &= tb.sendTelemetryData("accessGranted", granted);
    ok &= tb.sendTelemetryData("accessMethod",  method);

    Serial.printf("[TB] Evento: %s via %s\n",
                  granted ? "ACCESO OK" : "DENEGADO", method);
    return ok;
}

bool thingsboard_publish_failed_attempts(int count)
{
    if (!tb.connected()) return false;
    return tb.sendTelemetryData("failedAttempts", count);
}

bool thingsboard_publish_client_attributes(void)
{
    if (!tb.connected()) return false;

    bool ok = true;
    ok &= tb.sendAttributeData("firmwareVersion", FIRMWARE_VERSION);
    ok &= tb.sendAttributeData("ipAddress",       WiFi.localIP().toString().c_str());
    ok &= tb.sendAttributeData("macAddress",      WiFi.macAddress().c_str());
    ok &= tb.sendTelemetryData("uptime",          (long)millis());

    Serial.printf("[TB] Atributos cliente publicados (IP: %s)\n",
                  WiFi.localIP().toString().c_str());
    return ok;
}

// ── Registro de callbacks ────────────────────────────────────

void thingsboard_set_state_change_callback(RemoteStateCallback cb)    { stateChangeCb        = cb; }
void thingsboard_set_credentials_update_callback(CredentialsUpdateCallback cb) { credentialsUpdateCb = cb; }
void thingsboard_set_auto_lock_delay_callback(AutoLockDelayCallback cb){ autoLockDelayCb  = cb; }
void thingsboard_set_nfc_enabled_callback(NfcEnabledCallback cb)      { nfcEnabledCb     = cb; }
void thingsboard_set_nfc_uid_callback(NfcUidCallback cb)              { nfcUidCb         = cb; }
void thingsboard_set_max_failed_callback(MaxFailedCallback cb)         { maxFailedCb      = cb; }
void thingsboard_set_lockout_duration_callback(LockoutDurationCallback cb){ lockoutDurationCb = cb; }
void thingsboard_set_rpc_unlock_temp_callback(RpcUnlockTempCallback cb){ rpcUnlockTempCb = cb; }
void thingsboard_set_rpc_reset_lockout_callback(RpcResetLockoutCallback cb){ rpcResetLockoutCb = cb; }
