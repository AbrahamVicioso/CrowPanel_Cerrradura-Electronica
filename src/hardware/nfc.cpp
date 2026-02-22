/**
 * @file nfc.cpp
 * @brief Implementación del módulo de control del lector NFC
 */

#include "nfc.h"
#include "../config/pins.h"
#include <Wire.h>

// Instancia global del lector NFC
Adafruit_PN532 nfc(NFC_SDA, NFC_SCL);

bool nfc_init(void)
{
    // Inicializar I2C
    Wire.begin(NFC_SDA, NFC_SCL);
    
    // Inicializar NFC
    nfc.begin();
    
    uint32_t versiondata = nfc.getFirmwareVersion();
    
    if (!versiondata) {
        Serial.println("Error: No se encontró el módulo PN532");
        return false;
    }
    
    Serial.print("Chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    // Configurar SAM (Security Access Module)
    nfc.SAMConfig();
    
    Serial.println("NFC PN532 configurado correctamente");
    return true;
}

bool nfc_read_tag(NfcTag* tag)
{
    if (tag == nullptr) return false;
    
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    uint8_t success = nfc.readPassiveTargetID(
        PN532_MIFARE_ISO14443A, 
        uid, 
        &uidLength, 
        20  // Timeout de 20ms
    );
    
    if (success) {
        tag->uidLength = (uint8_t)min((int)uidLength, (int)NFC_UID_MAX_LENGTH);
        memcpy(tag->uid, uid, tag->uidLength);
        tag->valid = true;
        
        Serial.println("Tarjeta NFC detectada!");
        nfc_print_uid(uid, uidLength);
        
        return true;
    }
    
    tag->valid = false;
    return false;
}

bool nfc_is_present(void)
{
    uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t uidLength;
    
    return nfc.readPassiveTargetID(
        PN532_MIFARE_ISO14443A, 
        uid, 
        &uidLength, 
        100  // Timeout corto para verificación rápida
    );
}

void nfc_print_uid(uint8_t* uid, uint8_t uidLength)
{
    Serial.print("UID: ");
    for (uint8_t i = 0; i < uidLength; i++) {
        Serial.print(" 0x");
        Serial.print(uid[i], HEX);
    }
    Serial.println();
}
