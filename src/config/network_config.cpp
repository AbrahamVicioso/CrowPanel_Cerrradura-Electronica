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
const char* THINGSBOARD_ACCESS_TOKEN = "cdjFjdUvQe1WBNJ9qXuG";

// Shared attribute name for lock state
const char* LOCK_STATE_ATTRIBUTE = "lockState";

// MQTT Topics
const char* TB_ATTRIBUTES_TOPIC = "v1/devices/me/attributes";
const char* TB_TELEMETRY_TOPIC = "v1/devices/me/telemetry";
const char* TB_RPC_TOPIC = "v1/devices/me/rpc/request/+";
