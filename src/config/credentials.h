/**
 * @file credentials.h
 * @brief Gestión de credenciales de acceso recibidas desde ThingsBoard.
 *
 * El atributo compartido "credenciales" tiene la estructura:
 * {
 *   "huespedes": [
 *     { "huespedId": 1, "reservaId": 3002,
 *       "credenciales": [
 *         {"pin":"142820","activacion":"2026-04-17T22:54:18","expiracion":"2026-04-18T18:53:00"},
 *         ...
 *       ]
 *     }
 *   ],
 *   "personal": [
 *     { "personalId": 1, "nombre": "...",
 *       "credenciales": [
 *         {"pin":"805714","activacion":"2026-04-18T00:31:00","expiracion":"2026-05-01T00:31:00"},
 *         ...
 *       ]
 *     }
 *   ]
 * }
 *
 * Todas las fechas se interpretan como UTC.
 * Las credenciales se persisten en SPIFFS para operar sin internet.
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <Arduino.h>

/** Máximo de PINs individuales almacenados en memoria */
#define MAX_CREDENTIALS 100

/**
 * @brief Actualiza las credenciales desde el JSON del atributo "credenciales".
 * @param json  JSON serializado con estructura {"huespedes":[...],"personal":[...]}
 * @return true si el JSON fue parseado correctamente
 */
bool credentials_update(const char* json);

/**
 * @brief Valida un PIN contra las credenciales vigentes.
 * @param pin  PIN ingresado (String de dígitos)
 * @return true si alguna credencial activa coincide
 */
bool credentials_validate_pin(const String& pin);

/** Cantidad de PINs individuales actualmente cargados */
int credentials_get_count(void);

#endif // CREDENTIALS_H
