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
 * @brief Información completa de la credencial que coincidió en la última validación exitosa.
 *        Usada para enviar el cuerpo de la credencial a ThingsBoard en eventos de acceso.
 */
typedef struct {
    char tipo;              // 'H' = huesped, 'P' = personal
    int  owner_id;          // huespedId (H) o personalId (P)
    int  reserva_id;        // solo para huespedes
    char nombre[48];        // solo para personal (vacío en huespedes)
    char pin[16];
    char activacion_str[24]; // ISO 8601 original, ej "2026-04-17T22:54:18"
    char expiracion_str[24]; // ISO 8601 original
} CredentialMatch;

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

/**
 * @brief Recupera los datos completos de la última credencial validada exitosamente.
 * @param out  Estructura donde se copia la información (no puede ser nullptr)
 * @return true si hay una coincidencia disponible
 */
bool credentials_get_last_match(CredentialMatch* out);

/** Cantidad de PINs individuales actualmente cargados */
int credentials_get_count(void);

#endif // CREDENTIALS_H
