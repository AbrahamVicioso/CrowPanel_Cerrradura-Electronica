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

// Longitud máxima del payload JSON leído desde tarjeta MIFARE o HCE
#define MIFARE_JSON_MAX_LEN 256

// AID (Application Identifier) de la app móvil HCE
#define HCE_AID { 0xF0, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 }

/**
 * @brief Datos de una tarjeta NFC leída
 */
typedef struct {
    uint8_t uid[NFC_UID_MAX_LENGTH];
    uint8_t uidLength;
    uint8_t sak;   // SEL_RES: 0x08=MIFARE Classic 1K, 0x20/0x28=ISO-DEP (HCE/phone)
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

/**
 * @brief Lee el payload JSON desde un smartphone con HCE (Host Card Emulation).
 *
 * Envía un APDU SELECT AID al dispositivo detectado. Si responde con SW 9000,
 * el cuerpo de la respuesta se copia al buffer como JSON terminado en '\0'.
 * En tarjetas MIFARE Classic (que no soportan ISO-DEP) el comando falla
 * rápidamente sin bloquear el bus.
 *
 * @param buffer  Buffer de salida para el JSON
 * @param maxLen  Tamaño máximo del buffer (recomendado: MIFARE_JSON_MAX_LEN)
 * @return true si se recibió un payload JSON válido con SW 9000
 */
bool nfc_read_hce_payload(char* buffer, uint16_t maxLen);

#endif // NFC_H
