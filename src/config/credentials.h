/**
 * @file credentials.h
 * @brief Gestión de credenciales de acceso recibidas desde ThingsBoard.
 *
 * Soporta credenciales tipo "huesped" (ventana activacion/expiracion) y
 * "personal" (expiracion opcional o null = sin expiración).
 * Las fechas en el JSON se interpretan como UTC.
 */

#ifndef CREDENTIALS_H
#define CREDENTIALS_H

#include <Arduino.h>

/** Máximo de credenciales almacenadas en memoria */
#define MAX_CREDENTIALS 20

/**
 * @brief Actualiza la lista de credenciales desde un array JSON.
 * @param json  Array JSON serializado, ej:
 *   [{"tipo":"huesped","pin":"445363","activacion":"2026-04-16T01:17:14",
 *     "expiracion":"2026-04-16T21:03:00"},
 *    {"tipo":"personal","pin":"999999","expiracion":null}]
 * @return true si el JSON fue parseado correctamente
 */
bool credentials_update(const char* json);

/**
 * @brief Valida un PIN contra las credenciales vigentes.
 *        Verifica coincidencia de PIN y vigencia temporal en UTC.
 * @param pin  PIN ingresado (String de dígitos)
 * @return true si alguna credencial activa coincide con el PIN
 */
bool credentials_validate_pin(const String& pin);

/** Cantidad de credenciales actualmente cargadas */
int credentials_get_count(void);

#endif // CREDENTIALS_H
