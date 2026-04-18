/**
 * @file nfc.h
 * @brief Módulo NFC — lectura PN532 + verificación de UID autorizado
 */

#ifndef NFC_H
#define NFC_H

#include <Arduino.h>
#include <Adafruit_PN532.h>

// Longitud máxima del UID (7 bytes = NTAG/MIFARE Ultralight)
#define NFC_UID_MAX_LENGTH 7

// Longitud máxima del payload JSON leído desde tarjeta MIFARE
#define MIFARE_JSON_MAX_LEN 256

/**
 * @brief Datos de una tarjeta NFC leída
 */
typedef struct {
    uint8_t uid[NFC_UID_MAX_LENGTH];
    uint8_t uidLength;
    bool    valid;
} NfcTag;

/**
 * @brief Inicializa el lector NFC
 * @return true si se encontró el módulo PN532
 */
bool nfc_init(void);

/**
 * @brief Lee una tarjeta NFC (timeout corto, no bloqueante)
 * @param tag Puntero al struct donde almacenar el resultado
 * @return true si se detectó una tarjeta
 */
bool nfc_read_tag(NfcTag* tag);

/**
 * @brief Verifica si el UID de la tarjeta está autorizado
 * @param tag Tarjeta leída
 * @return true si está autorizada (o si no hay UID configurado)
 */
bool nfc_is_authorized(const NfcTag* tag);

/**
 * @brief Configura el UID autorizado
 * @param uid_hex Cadena hex con el UID, ej: "A1:B2:C3:D4" o "A1B2C3D4"
 */
void nfc_set_authorized_uid(const char* uid_hex);

/**
 * @brief Convierte el UID de una tarjeta a cadena hex legible
 * @param tag Tarjeta NFC
 * @return String "AA:BB:CC:DD"
 */
String nfc_uid_to_string(const NfcTag* tag);

/**
 * @brief Imprime el UID al Serial
 */
void nfc_print_uid(uint8_t* uid, uint8_t uidLength);

/**
 * @brief Lee el payload JSON desde una tarjeta MIFARE Classic 1K.
 *
 * Estrategia fail-fast: autentica sector a sector con la llave NDEF estándar
 * (0xD3,0xF7,...) y detiene el bucle en cuanto encuentra el cierre del JSON.
 * Salta automáticamente los trailer blocks (cada bloque %4==3).
 *
 * @param tag     Tarjeta detectada previamente por nfc_read_tag()
 * @param buffer  Buffer donde almacenar el JSON (terminado en '\0')
 * @param maxLen  Tamaño máximo del buffer (recomendado: MIFARE_JSON_MAX_LEN)
 * @return true si se extrajo un JSON completo y válido
 */
bool nfc_read_json_payload(const NfcTag* tag, char* buffer, uint16_t maxLen);

#endif // NFC_H
