/**
 * @file nfc.h
 * @brief Módulo de control del lector NFC
 */

#ifndef NFC_H
#define NFC_H

#include <Adafruit_PN532.h>

// Longitud máxima del UID
#define NFC_UID_MAX_LENGTH 7

/**
 * @brief Estructura para almacenar datos de tarjeta NFC
 */
typedef struct {
    uint8_t uid[NFC_UID_MAX_LENGTH];
    uint8_t uidLength;
    bool valid;
} NfcTag;

/**
 * @brief Inicializa el lector NFC
 * @return true si se inicializó correctamente, false en caso de error
 */
bool nfc_init(void);

/**
 * @brief Lee una tarjeta NFC
 * @param tag Puntero a estructura para almacenar los datos de la tarjeta
 * @return true si se detectó una tarjeta, false en caso contrario
 */
bool nfc_read_tag(NfcTag* tag);

/**
 * @brief Verifica si hay una tarjeta NFC presente
 * @return true si hay tarjeta, false en caso contrario
 */
bool nfc_is_present(void);

/**
 * @brief Imprime el UID de la tarjeta al serial
 * @param uid Puntero al array de UID
 * @param uidLength Longitud del UID
 */
void nfc_print_uid(uint8_t* uid, uint8_t uidLength);

#endif // NFC_H
