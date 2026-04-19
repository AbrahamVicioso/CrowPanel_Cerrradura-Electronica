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

// Constructor: (client, receive_buffer, send_buffer, max_stack, apis)
// receive=8192 para soportar JSON de credenciales grandes (~700-2000 bytes)
// send=1024 para soportar accessEvent + campos de credenciales sin truncamiento
// max_response_size (0) se fija explícitamente para evitar confusión con el array de APIs
#if THINGSBOARD_ENABLE_DYNAMIC
static ThingsBoardSized<64> tb(mqttClient, 8192, 1024, Default_Max_Stack_Size, 0, apis);
#else
static ThingsBoardSized<64> tb(mqttClient, 8192, 1024, Default_Max_Stack_Size, apis);
#endif

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
        JsonVariantConst val = data[ATTR_CREDENTIALS];
        if (val.is<const char*>()) {
            // TB envió el valor como string — usar directamente
            json = val.as<const char*>();
        } else {
            // TB envió como array/objeto nativo JSON
            serializeJson(val, json);
        }
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
    Serial.printf("[TB] Módulo ThingsBoard inicializado (DRAM libre: %d KB)\n", ESP.getFreeHeap() / 1024);

    // ── Suscripción ÚNICA a Shared Attributes ──────────────────
    // Se hace en init() porque la librería gestiona la resuscripción automática al reconectar.
    // Esto evita el error "Too many (MaxSubscriptions) subscriptions".
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
        Serial.println("[TB] ERROR: No se pudo registrar suscripción a shared attributes");
    } else {
        Serial.println("[TB] Callbacks de Shared Attributes registrados");
    }

    // ── Suscripción ÚNICA a RPC ──────────────────────────────
    const std::array<RPC_Callback, 2U> rpcCbs = {{
        RPC_Callback{ "unlockTemporary", on_rpc_unlock_temporary },
        RPC_Callback{ "resetLockout",    on_rpc_reset_lockout    }
    }};
    if (!serverSideRPC.RPC_Subscribe(rpcCbs.cbegin(), rpcCbs.cend())) {
        Serial.println("[TB] ERROR: No se pudo registrar suscripción a RPC");
    } else {
        Serial.println("[TB] Callbacks de RPC registrados");
    }
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

    Serial.printf("[TB] Intentando conectar a %s:%d con token %s...\n", 
                  tbServer.c_str(), THINGSBOARD_PORT, tbToken.c_str());

    if (!tb.connect(tbServer.c_str(), tbToken.c_str(), THINGSBOARD_PORT)) {
        Serial.printf("[TB] ERROR de conexión (Servidor: %s)\n", tbServer.c_str());
        tbConnected = false;
        return false;
    }

    Serial.println("[TB] Conectado!");
    tbConnected = true;

    // NOTA: Las suscripciones ya se hicieron en thingsboard_init(). 
    // La librería se encarga de resuscribir los tópicos MQTT automáticamente.

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

    // ── Heartbeat de telemetría (uptime cada 60s) ──────────────
    static uint32_t last_uptime = 0;
    if (millis() - last_uptime > 60000) {
        tb.sendTelemetryData("uptime", millis() / 1000);
        last_uptime = millis();
    }
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
    if (!tb.connected()) {
        Serial.println("[TB] WARN: no conectado — lock state no publicado");
        return false;
    }

    bool locked = (state == LockState::LOCKED);
    const char* stateStr = locked ? "locked" : "unlocked";

    // Telemetría (serie temporal) — siempre se envía, incluso en cambios remotos
    bool ok1 = tb.sendTelemetryData("locked", locked);
    if (!ok1) Serial.println("[TB] ERROR: sendTelemetryData(locked) falló");

    // Atributo cliente — omitir durante cambio remoto para evitar feedback loop
    bool ok2 = true;
    if (!isProcessingRemoteChange) {
        ok2 = tb.sendAttributeData(ATTR_LOCK_STATE, stateStr);
        if (!ok2) Serial.println("[TB] ERROR: sendAttributeData(lockState) falló");
    }

    if (ok1) Serial.printf("[TB] Telemetría publicada: locked=%s\n", stateStr);
    return ok1 && ok2;
}

bool thingsboard_publish_access_event(bool granted, const char* method)
{
    if (!tb.connected()) {
        Serial.println("[TB] WARN: no conectado — access event no publicado");
        return false;
    }

    // Batch: ambos campos en un solo mensaje MQTT usando JSON raw (sin double-escaping)
    char json[128];
    snprintf(json, sizeof(json),
             "{\"accessGranted\":%s,\"accessMethod\":\"%s\"}",
             granted ? "true" : "false", method);

    bool ok = tb.sendTelemetryString(json);

    if (!ok) {
        Serial.printf("[TB] ERROR: sendTelemetryString(accessEvent) falló — %s via %s\n",
                      granted ? "ACCESO OK" : "DENEGADO", method);
    } else {
        Serial.printf("[TB] Evento publicado: %s via %s\n",
                      granted ? "ACCESO OK" : "DENEGADO", method);
    }
    return ok;
}

bool thingsboard_publish_access_event_with_credential(bool granted, const char* method,
                                                       const CredentialMatch* match)
{
    if (!tb.connected()) {
        Serial.println("[TB] WARN: no conectado — access event no publicado");
        return false;
    }

    if (!match) {
        // Sin credencial: usar la versión simple
        return thingsboard_publish_access_event(granted, method);
    }

    // JSON raw manual — evita ArduinoJson double-escaping y overflow de buffer de serialización
    char json[512];
    bool ok = false;

    if (match->tipo == 'P') {
        snprintf(json, sizeof(json),
            "{\"accessGranted\":%s,\"accessMethod\":\"%s\","
            "\"credTipo\":\"personal\",\"credId\":%d,"
            "\"credNombre\":\"%s\",\"credPin\":\"%s\","
            "\"credAct\":\"%s\",\"credExp\":\"%s\"}",
            granted ? "true" : "false", method,
            match->owner_id, match->nombre, match->pin,
            match->activacion_str, match->expiracion_str);
    } else {
        snprintf(json, sizeof(json),
            "{\"accessGranted\":%s,\"accessMethod\":\"%s\","
            "\"credTipo\":\"huesped\",\"credId\":%d,"
            "\"reservaId\":%d,\"credPin\":\"%s\","
            "\"credAct\":\"%s\",\"credExp\":\"%s\"}",
            granted ? "true" : "false", method,
            match->owner_id, match->reserva_id, match->pin,
            match->activacion_str, match->expiracion_str);
    }
    ok = tb.sendTelemetryString(json);

    if (!ok) {
        Serial.printf("[TB] ERROR: sendTelemetry(accessEvent+cred) falló — %s via %s (%s id=%d)\n",
                      granted ? "ACCESO OK" : "DENEGADO", method,
                      match->tipo == 'P' ? "personal" : "huesped",
                      match->owner_id);
    } else {
        Serial.printf("[TB] Evento+cred publicado: %s via %s (%s id=%d)\n",
                      granted ? "ACCESO OK" : "DENEGADO", method,
                      match->tipo == 'P' ? "personal" : "huesped",
                      match->owner_id);
    }
    return ok;
}

bool thingsboard_publish_failed_attempts(int count)
{
    if (!tb.connected()) return false;
    bool ok = tb.sendTelemetryData("failedAttempts", count);
    if (!ok) Serial.printf("[TB] ERROR: sendTelemetryData(failedAttempts=%d) falló\n", count);
    return ok;
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
