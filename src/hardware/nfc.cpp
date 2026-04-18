/**
 * @file nfc.cpp
 * @brief Implementación NFC — PN532 + autorización por UID
 */

#include "nfc.h"
#include "../config/pins.h"
#include <Wire.h>

// Instancia del lector PN532 (I2C)
static Adafruit_PN532 nfc(NFC_SDA, NFC_SCL);

// UID autorizado (vacío = aceptar cualquier tarjeta)
static char authorized_uid[25] = "";  // "AA:BB:CC:DD:EE:FF:GG\0"

// ────────────────────────────────────────────────────────────
//  Inicialización
// ────────────────────────────────────────────────────────────

bool nfc_init(void)
{
    Wire.begin(NFC_SDA, NFC_SCL);
    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (!versiondata) {
        Serial.println("[NFC] ERROR: Módulo PN532 no encontrado");
        return false;
    }

    Serial.printf("[NFC] PN5%02X firmware %d.%d\n",
                  (versiondata >> 24) & 0xFF,
                  (versiondata >> 16) & 0xFF,
                  (versiondata >>  8) & 0xFF);

    nfc.SAMConfig();
    Serial.println("[NFC] Lector PN532 listo");
    return true;
}

// ────────────────────────────────────────────────────────────
//  Lectura de tarjeta
// ────────────────────────────────────────────────────────────

bool nfc_read_tag(NfcTag* tag)
{
    if (!tag) return false;

    uint8_t uid[NFC_UID_MAX_LENGTH] = {0};
    uint8_t uidLength = 0;

    // Timeout 20ms para no bloquear el loop principal
    bool success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,
                                            uid, &uidLength, 20);

    if (!success) {
        tag->valid = false;
        return false;
    }

    tag->uidLength = (uint8_t)min((int)uidLength, (int)NFC_UID_MAX_LENGTH);
    memcpy(tag->uid, uid, tag->uidLength);
    tag->sak   = nfc._lastSAK;
    tag->valid = true;

    Serial.printf("[NFC] Detectado — UID: ");
    nfc_print_uid(uid, uidLength);
    Serial.printf("[NFC] SAK: 0x%02X (%s)\n", tag->sak,
        (tag->sak == 0x20 || tag->sak == 0x28) ? "ISO-DEP/HCE" :
        (tag->sak == 0x08)                      ? "MIFARE Classic 1K" :
        (tag->sak == 0x18)                      ? "MIFARE Classic 4K" :
        (tag->sak == 0x00)                      ? "MIFARE Ultralight" : "Desconocido");

    return true;
}

// ────────────────────────────────────────────────────────────
//  Autorización por UID
// ────────────────────────────────────────────────────────────

void nfc_set_authorized_uid(const char* uid_hex)
{
    if (!uid_hex) {
        authorized_uid[0] = '\0';
        return;
    }
    strncpy(authorized_uid, uid_hex, sizeof(authorized_uid) - 1);
    authorized_uid[sizeof(authorized_uid) - 1] = '\0';

    // Convertir a mayúsculas
    for (int i = 0; authorized_uid[i]; i++) {
        if (authorized_uid[i] >= 'a' && authorized_uid[i] <= 'f')
            authorized_uid[i] -= 32;
    }

    if (strlen(authorized_uid) == 0) {
        Serial.println("[NFC] UID autorizado: ANY (acepta todas)");
    } else {
        Serial.printf("[NFC] UID autorizado: %s\n", authorized_uid);
    }
}

bool nfc_is_authorized(const NfcTag* tag)
{
    if (!tag || !tag->valid) return false;

    // Sin UID configurado → RECHAZAR por defecto (para obligar a usar el JSON o configurar un UID explicitamente)
    if (strlen(authorized_uid) == 0) return false;

    // Convertir UID leído a string sin separadores
    String tagStr = "";
    for (int i = 0; i < tag->uidLength; i++) {
        if (tag->uid[i] < 0x10) tagStr += "0";
        tagStr += String(tag->uid[i], HEX);
    }
    tagStr.toUpperCase();

    // Convertir UID autorizado a string sin separadores
    String authStr = String(authorized_uid);
    authStr.replace(":", "");
    authStr.replace(" ", "");
    authStr.replace("-", "");
    authStr.toUpperCase();

    bool authorized = (tagStr == authStr);
    Serial.printf("[NFC] UID leído: %s  Autorizado: %s  Resultado: %s\n",
                  tagStr.c_str(), authStr.c_str(),
                  authorized ? "ACEPTADO" : "RECHAZADO");
    return authorized;
}

// ────────────────────────────────────────────────────────────
//  Utilidades
// ────────────────────────────────────────────────────────────

String nfc_uid_to_string(const NfcTag* tag)
{
    String result = "";
    for (int i = 0; i < tag->uidLength; i++) {
        if (i > 0) result += ":";
        if (tag->uid[i] < 0x10) result += "0";
        result += String(tag->uid[i], HEX);
    }
    result.toUpperCase();
    return result;
}

void nfc_print_uid(uint8_t* uid, uint8_t uidLength)
{
    for (uint8_t i = 0; i < uidLength; i++) {
        if (i > 0) Serial.print(":");
        if (uid[i] < 0x10) Serial.print("0");
        Serial.print(uid[i], HEX);
    }
    Serial.println();
}

// ────────────────────────────────────────────────────────────
//  Lectura JSON via HCE (smartphone)
// ────────────────────────────────────────────────────────────

bool nfc_read_hce_payload(char* buffer, uint16_t maxLen)
{
    if (!buffer || maxLen < 2) return false;

    // Construir APDU SELECT AID: CLA INS P1 P2 Lc [AID bytes]
    uint8_t aid[] = HCE_AID;
    const uint8_t aidLen = sizeof(aid);

    uint8_t cmd[5 + aidLen];
    cmd[0] = 0x00;   // CLA
    cmd[1] = 0xA4;   // INS: SELECT FILE
    cmd[2] = 0x04;   // P1: select by AID
    cmd[3] = 0x00;   // P2: primera ocurrencia
    cmd[4] = aidLen; // Lc: longitud del AID
    memcpy(&cmd[5], aid, aidLen);

    uint8_t response[255];
    uint8_t respLen = sizeof(response);

    if (!nfc.inDataExchange(cmd, sizeof(cmd), response, &respLen)) {
        // La tarjeta/dispositivo no responde a APDUs — no es HCE
        return false;
    }

    // Verificar Status Word: últimos 2 bytes deben ser 90 00
    if (respLen < 2 || response[respLen - 2] != 0x90 || response[respLen - 1] != 0x00) {
        Serial.printf("[NFC] HCE SW inválido: %02X %02X\n",
                      respLen >= 2 ? response[respLen - 2] : 0,
                      respLen >= 1 ? response[respLen - 1] : 0);
        return false;
    }

    // Extraer cuerpo JSON (todo antes del SW)
    uint8_t jsonLen = respLen - 2;
    if (jsonLen == 0) return false;

    uint16_t copyLen = (jsonLen < maxLen - 1) ? jsonLen : (maxLen - 1);
    memcpy(buffer, response, copyLen);
    buffer[copyLen] = '\0';

    Serial.printf("[NFC] HCE payload recibido (%u bytes)\n", copyLen);
    return true;
}

// ────────────────────────────────────────────────────────────
//  Lectura JSON desde MIFARE Classic 1K
// ────────────────────────────────────────────────────────────

bool nfc_read_json_payload(const NfcTag* tag, char* buffer, uint16_t maxLen)
{
    if (!tag || !tag->valid || !buffer || maxLen < 2) return false;

    // Llave estándar NFC Forum para sectores NDEF (sectores 1-15)
    uint8_t ndefKey[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };

    uint16_t bufPos    = 0;
    bool     started   = false;  // ¿Se encontró el primer '{'?
    int      depth     = 0;      // Profundidad de anidamiento de llaves

    // Empezar desde el bloque 4 (sector 1).
    // Sector 0 (bloques 0-3): fabricante + MAD, no contiene datos NDEF útiles.
    // MIFARE Classic 1K: 64 bloques totales (0-63).
    // Trailer blocks (llaves): bloques 3, 7, 11, 15, … → bloque%4==3 → saltar.
    for (uint8_t block = 4; block < 64 && bufPos < (uint16_t)(maxLen - 1); block++) {

        // Saltar trailer block
        if ((block % 4) == 3) continue;

        // Al entrar a un nuevo sector, autenticar con llave NDEF
        if ((block % 4) == 0) {
            bool auth = nfc.mifareclassic_AuthenticateBlock(
                (uint8_t*)tag->uid, tag->uidLength,
                block, 0 /*key A*/, ndefKey);
            if (!auth) {
                // Fallo de autenticación: tarjeta retirada o fin de datos
                Serial.printf("[NFC] Auth falló en bloque %d\n", block);
                break;
            }
        }

        uint8_t blockData[16];
        if (!nfc.mifareclassic_ReadDataBlock(block, blockData)) {
            Serial.printf("[NFC] Read falló en bloque %d\n", block);
            break;
        }

        // Procesar los 16 bytes del bloque
        for (int i = 0; i < 16 && bufPos < (uint16_t)(maxLen - 1); i++) {
            uint8_t c = blockData[i];

            if (!started) {
                // Descartar metadata NDEF hasta encontrar el primer '{'
                if (c == '{') {
                    started = true;
                    depth   = 1;
                    buffer[bufPos++] = c;
                }
                continue;
            }

            // JSON en progreso: rastrear profundidad de llaves
            if      (c == '{') depth++;
            else if (c == '}') depth--;

            buffer[bufPos++] = c;

            if (depth == 0) {
                // JSON completo — salir inmediatamente (fail-fast)
                buffer[bufPos] = '\0';
                Serial.printf("[NFC] JSON extraído (%u bytes)\n", bufPos);
                return true;
            }
        }
    }

    // El bucle terminó sin encontrar JSON completo
    return false;
}
