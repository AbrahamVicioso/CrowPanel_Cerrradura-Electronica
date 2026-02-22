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
    Serial.print("Mensaje recibido en topic: ");
    Serial.println(topic);
    
    // Parse RPC topic: v1/devices/me/rpc/request/{id}
    String topicStr = String(topic);
    
    if (topicStr.indexOf("rpc/request/") >= 0)
    {
        // Parse JSON payload
        char message[length + 1];
        memcpy(message, payload, length);
        message[length] = '\0';
        
        Serial.print("Payload: ");
        Serial.println(message);
        
        // Try to find method in JSON
        String payloadStr = String(message);
        int methodStart = payloadStr.indexOf("\"method\"");
        if (methodStart < 0) methodStart = payloadStr.indexOf("\"methodName\"");
        
        if (methodStart >= 0)
        {
            // Find the method name
            int nameStart = payloadStr.indexOf(":", methodStart) + 2;
            int nameEnd = payloadStr.indexOf("\"", nameStart);
            String methodName = payloadStr.substring(nameStart, nameEnd);
            
            Serial.print("Método: ");
            Serial.println(methodName);
            
            // Handle commands
            if (methodName == "unlockDoor" || methodName == "unlock" || methodName == "setLockState")
            {
                // Check for state parameter
                bool unlock = true;
                if (payloadStr.indexOf("\"state\":false") >= 0) {
                    unlock = false;
                }
                
                if (unlock) {
                    lock_unlock();
                } else {
                    lock_lock();
                }
            }
            else if (methodName == "lockDoor" || methodName == "lock")
            {
                lock_lock();
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
        Serial.println("WiFi no conectado");
        return false;
    }

    if (tb.connected())
    {
        return true;
    }

    Serial.print("Conectando a ThingsBoard MQTT...");
    
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
        Serial.println("Error de conexión ThingsBoard");
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
    if (WiFi.status() == WL_CONNECTED)
    {
        if (!tb.connected())
        {
            thingsboard_reconnect();
        }
        // Process ThingsBoard MQTT messages
        tb.loop();
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
        Serial.print("Conectando a WiFi...");
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        delay(500);
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
