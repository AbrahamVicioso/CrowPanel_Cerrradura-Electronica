/**
 * @file storage.cpp
 * @brief Implementación de persistencia NVS
 */

#include "storage.h"
#include "config.h"
#include "network_config.h"
#include <Preferences.h>
#include <esp_log.h>

static Preferences prefs;

void storage_init(void)
{
    // Suprimir mensajes verbose de Preferences (NOT_FOUND en primer arranque)
    esp_log_level_set("Preferences", ESP_LOG_NONE);
    prefs.begin("lock_cfg", false);
    Serial.println("Storage NVS inicializado");
}

// ── PIN ─────────────────────────────────────────
String storage_get_pin(void)
{
    return prefs.getString("pin", DEFAULT_PIN);
}

void storage_set_pin(const char* pin)
{
    prefs.putString("pin", pin);
    Serial.printf("PIN guardado en NVS: %s\n", pin);
}

// ── NFC UID ──────────────────────────────────────
String storage_get_nfc_uid(void)
{
    return prefs.getString("nfc_uid", "");
}

void storage_set_nfc_uid(const char* uid)
{
    prefs.putString("nfc_uid", uid);
    Serial.printf("NFC UID guardado en NVS: %s\n", uid);
}

// ── Intentos fallidos ───────────────────────────
int storage_get_max_failed_attempts(void)
{
    return prefs.getInt("max_fail", MAX_FAILED_ATTEMPTS);
}

void storage_set_max_failed_attempts(int max)
{
    prefs.putInt("max_fail", max);
}

// ── Auto-lock delay ─────────────────────────────
uint32_t storage_get_auto_lock_delay(void)
{
    return prefs.getUInt("auto_lock", LOCK_AUTO_LOCK_DELAY_MS);
}

void storage_set_auto_lock_delay(uint32_t delayMs)
{
    prefs.putUInt("auto_lock", delayMs);
}

// ── NFC habilitado ──────────────────────────────
bool storage_get_nfc_enabled(void)
{
    return prefs.getBool("nfc_en", true);
}

void storage_set_nfc_enabled(bool enabled)
{
    prefs.putBool("nfc_en", enabled);
}

// ── Duración lockout ────────────────────────────
int storage_get_lockout_duration(void)
{
    return prefs.getInt("lockout_s", (int)(LOCKOUT_DURATION_MS / 1000));
}

void storage_set_lockout_duration(int seconds)
{
    prefs.putInt("lockout_s", seconds);
}

// ── Credenciales WiFi ────────────────────────────
String storage_get_wifi_ssid(void)
{
    return prefs.getString("wifi_ssid", WIFI_SSID);
}

void storage_set_wifi_ssid(const char* ssid)
{
    prefs.putString("wifi_ssid", ssid);
    Serial.printf("WiFi SSID guardado: %s\n", ssid);
}

String storage_get_wifi_pass(void)
{
    return prefs.getString("wifi_pass", WIFI_PASSWORD);
}

void storage_set_wifi_pass(const char* pass)
{
    prefs.putString("wifi_pass", pass);
    Serial.println("WiFi password guardado");
}

// ── Credenciales ThingsBoard ─────────────────────
String storage_get_tb_server(void)
{
    return prefs.getString("tb_server", THINGSBOARD_SERVER);
}

void storage_set_tb_server(const char* server)
{
    prefs.putString("tb_server", server);
    Serial.printf("TB server guardado: %s\n", server);
}

String storage_get_tb_token(void)
{
    return prefs.getString("tb_token", THINGSBOARD_ACCESS_TOKEN);
}

void storage_set_tb_token(const char* token)
{
    prefs.putString("tb_token", token);
    Serial.println("TB token guardado");
}

// ── Credenciales de acceso (JSON completo) ───────
String storage_get_credentials(void)
{
    return prefs.getString("creds_json", "");
}

void storage_set_credentials(const char* json)
{
    prefs.putString("creds_json", json);
    Serial.printf("[Storage] Credenciales guardadas en NVS (%d bytes)\n", strlen(json));
}

// ── Contraseña portal de configuración ──────────
String storage_get_ap_password(void)
{
    return prefs.getString("ap_pass", "admin1234");
}

void storage_set_ap_password(const char* pass)
{
    prefs.putString("ap_pass", pass);
    Serial.println("AP password guardado");
}
