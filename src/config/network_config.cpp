/**
 * @file network_config.cpp
 * @brief Implementación de configuración de red
 */

#include "network_config.h"

// WiFi Network Credentials
const char* WIFI_SSID = "HABITACIONES";
const char* WIFI_PASSWORD = "Mixael2003";

// ThingsBoard Server Configuration
const char* THINGSBOARD_SERVER = "10.0.0.33";
const uint16_t THINGSBOARD_PORT = 1883;

// Device Access Token
const char* THINGSBOARD_ACCESS_TOKEN = "mdxequOULBUCtzgcEoyy";

// ============================================
// ATRIBUTOS COMPARTIDOS ThingsBoard
// ============================================
const char* ATTR_LOCK_STATE        = "lockState";
const char* ATTR_CORRECT_PIN       = "correctPin";
const char* ATTR_AUTO_LOCK_DELAY   = "autoLockDelay";
const char* ATTR_NFC_ENABLED       = "nfcEnabled";
const char* ATTR_NFC_UID           = "authorizedNfcUid";
const char* ATTR_MAX_FAILED        = "maxFailedAttempts";
const char* ATTR_LOCKOUT_DURATION  = "lockoutDuration";

// Alias de compatibilidad
const char* LOCK_STATE_ATTRIBUTE   = "lockState";

// MQTT Topics
const char* TB_ATTRIBUTES_TOPIC = "v1/devices/me/attributes";
const char* TB_TELEMETRY_TOPIC  = "v1/devices/me/telemetry";
const char* TB_RPC_TOPIC        = "v1/devices/me/rpc/request/+";
