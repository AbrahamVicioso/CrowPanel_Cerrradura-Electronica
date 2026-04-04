/**
 * @file storage.h
 * @brief Persistencia NVS — guarda configuración entre reinicios
 *
 * Claves almacenadas (namespace "lock_cfg"):
 *   pin          → PIN correcto
 *   nfc_uid      → UID NFC autorizado (hex string)
 *   max_fail     → máximo intentos fallidos
 *   auto_lock    → delay auto-bloqueo (ms)
 *   nfc_en       → NFC habilitado
 *   lockout_s    → duración lockout (segundos)
 */

#ifndef STORAGE_H
#define STORAGE_H

#include <Arduino.h>

/**
 * @brief Inicializa el storage NVS
 */
void storage_init(void);

// PIN
String storage_get_pin(void);
void   storage_set_pin(const char* pin);

// NFC UID autorizado
String storage_get_nfc_uid(void);
void   storage_set_nfc_uid(const char* uid);

// Intentos máximos
int  storage_get_max_failed_attempts(void);
void storage_set_max_failed_attempts(int max);

// Delay auto-bloqueo en ms
uint32_t storage_get_auto_lock_delay(void);
void     storage_set_auto_lock_delay(uint32_t delayMs);

// NFC habilitado
bool storage_get_nfc_enabled(void);
void storage_set_nfc_enabled(bool enabled);

// Duración lockout en segundos
int  storage_get_lockout_duration(void);
void storage_set_lockout_duration(int seconds);

// ── Credenciales WiFi ────────────────────────────
String storage_get_wifi_ssid(void);
void   storage_set_wifi_ssid(const char* ssid);
String storage_get_wifi_pass(void);
void   storage_set_wifi_pass(const char* pass);

// ── Credenciales ThingsBoard ─────────────────────
String storage_get_tb_server(void);
void   storage_set_tb_server(const char* server);
String storage_get_tb_token(void);
void   storage_set_tb_token(const char* token);

// ── Contraseña del portal de configuración ───────
String storage_get_ap_password(void);
void   storage_set_ap_password(const char* pass);

#endif // STORAGE_H
