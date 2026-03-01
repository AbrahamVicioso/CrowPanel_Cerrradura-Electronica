/**
 * @file thingsboard.cpp
 * @brief Implementación del módulo de comunicación con ThingsBoard
 */

#include "thingsboard.h"
#include "../config/network_config.h"
#include "../config/config.h"
#include "../hardware/lock.h"

// ============================================
// VARIABLES PRIVADAS
// ============================================

// WiFi and MQTT clients
static WiFiClient wifiClient;
static Arduino_MQTT_Client mqttClient(wifiClient);
static ThingsBoard tb(mqttClient);

// Connection status
static bool tbConnected = false;
static unsigned long lastConnectAttempt = 0;

// ============================================
// CALLBACKS MQTT
// ============================================

/**
 * @brief Callback para mensajes MQTT recibidos
 */
static void mqttCallback(char* topic, byte* payload, unsigned int length)
{
    // Serial.print("Mensaje recibido en topic: ");
    // Serial.println(topic);
    
    // Parse RPC topic: v1/devices/me/rpc/request/{id}
    // Usar comparación más eficiente
    if (length < 5) return;  // Early exit para mensajes muy cortos
    
    // Quick check for rpc/request/ - evitar String allocation si no es necesario
    bool isRpcRequest = false;
    for (unsigned int i = 0; i < length - 12 && i < 50; i++) {
        if (topic[i] == 'r' && topic[i+1] == 'p' && topic[i+2] == 'c' && 
            topic[i+3] == '/' && topic[i+4] == 'r' && topic[i+5] == 'e' &&
            topic[i+6] == 'q' && topic[i+7] == 'u' && topic[i+8] == 'e' &&
            topic[i+9] == 's' && topic[i+10] == 't' && topic[i+11] == '/') {
            isRpcRequest = true;
            break;
        }
    }
    
    if (!isRpcRequest) return;
    
    // Parse JSON payload
    char message[128];  // Buffer en stack en lugar de heap
    unsigned int copyLen = (length < 127) ? length : 127;
    memcpy(message, payload, copyLen);
    message[copyLen] = '\0';
    
    // Serial.print("Payload: ");
    // Serial.println(message);
    
    // Buscar method en JSON - búsqueda simple
    char* methodPos = strstr(message, "\"method\"");
    if (!methodPos) methodPos = strstr(message, "\"methodName\"");
    
    if (methodPos != nullptr)
    {
        // Encontrar nombre del método
        char* nameStart = strchr(methodPos, ':');
        if (nameStart != nullptr) {
            nameStart += 2;  // Saltar ": y espacio
            char* nameEnd = strchr(nameStart, '"');
            
            if (nameEnd != nullptr && (nameEnd - nameStart) < 20) {
                char methodName[21];
                int nameLen = nameEnd - nameStart;
                strncpy(methodName, nameStart, nameLen);
                methodName[nameLen] = '\0';
                
                // Handle commands
                if (strcmp(methodName, "unlockDoor") == 0 || 
                    strcmp(methodName, "unlock") == 0 || 
                    strcmp(methodName, "setLockState") == 0)
                {
                    // Check for state parameter
                    bool unlock = true;
                    if (strstr(message, "\"state\":false") != nullptr || 
                        strstr(message, "\"state\": false") != nullptr) {
                        unlock = false;
                    }
                    
                    if (unlock) {
                        lock_unlock();
                    } else {
                        lock_lock();
                    }
                }
                else if (strcmp(methodName, "lockDoor") == 0 || 
                         strcmp(methodName, "lock") == 0)
                {
                    lock_lock();
                }
            }
        }
    }
}

// ============================================
// FUNCIONES PÚBLICAS
// ============================================

void thingsboard_init(void)
{
    // Configurar callback MQTT
    // Nota: ThingsBoard maneja esto internamente
    
    Serial.println("Módulo ThingsBoard inicializado");
}

bool thingsboard_connect(void)
{
    // Check WiFi status
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("ThingsBoard: WiFi no conectado");
        return false;
    }

    if (tb.connected())
    {
        return true;
    }

    Serial.print("Conectando a ThingsBoard ");
    Serial.print(THINGSBOARD_SERVER);
    Serial.print(":");
    Serial.println(THINGSBOARD_PORT);
    
    // Connect to ThingsBoard
    if (tb.connect(THINGSBOARD_SERVER, THINGSBOARD_ACCESS_TOKEN, THINGSBOARD_PORT))
    {
        Serial.println("Conectado a ThingsBoard!");
        
        // Publicar estado inicial
        thingsboard_publish_lock_state(lock_get_state());
        
        tbConnected = true;
        return true;
    }
    else
    {
        Serial.println("Error de conexión ThingsBoard - intentar mas tarde");
        tbConnected = false;
        return false;
    }
}

void thingsboard_disconnect(void)
{
    if (tb.connected()) {
        tb.disconnect();
        tbConnected = false;
    }
}

void thingsboard_loop(void)
{
    // Solo procesar si hay WiFi conectado
    if (WiFi.status() == WL_CONNECTED)
    {
        // Intentar reconectar si es necesario
        if (!tb.connected())
        {
            thingsboard_reconnect();
        }
        
        // Procesar mensajes MQTT - solo si está conectado
        if (tb.connected()) {
            tb.loop();
        }
    }
}

bool thingsboard_publish_lock_state(LockState state)
{
    if (!tb.connected())
    {
        return false;
    }

    // Send shared attribute
    const char* value = (state == LockState::UNLOCKED) ? "unlocked" : "locked";
    
    if (tb.sendAttributeData(LOCK_STATE_ATTRIBUTE, value))
    {
        Serial.print("Estado de cerradura publicado: ");
        Serial.println(value);
        return true;
    }
    else
    {
        Serial.println("Error al publicar estado de cerradura");
        return false;
    }
}

bool thingsboard_is_connected(void)
{
    return tbConnected && tb.connected() && (WiFi.status() == WL_CONNECTED);
}

void thingsboard_reconnect(void)
{
    unsigned long currentMillis = millis();
    
    // Reconnect to WiFi if needed
    if (WiFi.status() != WL_CONNECTED)
    {
        // Intentar reconectar WiFi silenciosamente (sin muchos Serial)
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(100);  // Reducido de 500ms
        return;
    }
    
    // Reconnect to ThingBoard if needed
    if (!tb.connected())
    {
        if (currentMillis - lastConnectAttempt >= THINGSBOARD_RECONNECT_INTERVAL_MS)
        {
            lastConnectAttempt = currentMillis;
            thingsboard_connect();
        }
        tb.loop();
    }
}
