/**
 * @file thingsboard.h
 * @brief Módulo de comunicación con ThingsBoard
 */

#ifndef THINGSBOARD_H
#define THINGSBOARD_H

#include <WiFi.h>
#include <PubSubClient.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include "../hardware/lock.h"

/**
 * @brief Inicializa el módulo de comunicación
 */
void thingsboard_init(void);

/**
 * @brief Conecta al servidor ThingsBoard
 * @return true si la conexión fue exitosa
 */
bool thingsboard_connect(void);

/**
 * @brief Desconecta del servidor ThingsBoard
 */
void thingsboard_disconnect(void);

/**
 * @brief Procesa el loop de ThingsBoard (debe llamarse en el loop principal)
 */
void thingsboard_loop(void);

/**
 * @brief Publica el estado de la cerradura
 * @param state Estado actual de la cerradura
 * @return true si se publicó correctamente
 */
bool thingsboard_publish_lock_state(LockState state);

/**
 * @brief Verifica si está conectado a ThingsBoard
 * @return true si está conectado
 */
bool thingsboard_is_connected(void);

/**
 * @brief Intenta reconectar si hay conexión perdida
 */
void thingsboard_reconnect(void);

#endif // THINGSBOARD_H
