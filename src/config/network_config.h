/**
 * @file network_config.h
 * @brief Configuración de red (WiFi y ThingsBoard)
 */

#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include <Arduino.h>

// ============================================
// CONFIGURACIÓN WI-FI
// ============================================

// WiFi Network Credentials
extern const char* WIFI_SSID;
extern const char* WIFI_PASSWORD;

// WiFi Connection Settings
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define WIFI_RECONNECT_DELAY_MS 500

// ============================================
 // CONFIGURACIÓN THINGBOARD
// ============================================

// ThingsBoard Server Configuration
extern const char* THINGSBOARD_SERVER;
extern const uint16_t THINGSBOARD_PORT;

// Device Access Token
extern const char* THINGSBOARD_ACCESS_TOKEN;

// ============================================
// NOMBRES DE ATRIBUTOS COMPARTIDOS (TB → Device)
// ============================================
extern const char* ATTR_LOCK_STATE;          // "lockState"       → "locked"/"unlocked"
extern const char* ATTR_CORRECT_PIN;         // "correctPin"      → "XXXXXX"
extern const char* ATTR_AUTO_LOCK_DELAY;     // "autoLockDelay"   → ms (int)
extern const char* ATTR_NFC_ENABLED;         // "nfcEnabled"      → bool
extern const char* ATTR_NFC_UID;             // "authorizedNfcUid"→ "AA:BB:CC:DD"
extern const char* ATTR_MAX_FAILED;          // "maxFailedAttempts"→ int
extern const char* ATTR_LOCKOUT_DURATION;    // "lockoutDuration" → segundos

// Alias de compatibilidad
extern const char* LOCK_STATE_ATTRIBUTE;

// MQTT Topics
extern const char* TB_ATTRIBUTES_TOPIC;
extern const char* TB_TELEMETRY_TOPIC;
extern const char* TB_RPC_TOPIC;

// Reconnection settings
#define THINGSBOARD_RECONNECT_INTERVAL_MS 5000

#endif // NETWORK_CONFIG_H
