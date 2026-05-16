/**
 * @file nfc.cpp
 * @brief Implementación NFC — PN532 + autorización por UID
 */

#include "nfc.h"
#include "../config/pins.h"
#include <Wire.h>

static Adafruit_PN532 nfc(NFC_SDA, NFC_SCL);

static char authorized_uid[25] = "";

bool nfc_init(void)
{
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

bool nfc_read_tag(NfcTag* tag)
{
    if (!tag) return false;

    uint8_t uid[NFC_UID_MAX_LENGTH] = {0};
    uint8_t uidLength = 0;

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

void nfc_set_authorized_uid(const char* uid_hex)
{
    if (!uid_hex) {
        authorized_uid[0] = '\0';
        return;
    }
    strncpy(authorized_uid, uid_hex, sizeof(authorized_uid) - 1);
    authorized_uid[sizeof(authorized_uid) - 1] = '\0';

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

    if (strlen(authorized_uid) == 0) return false;

    String tagStr = "";
    for (int i = 0; i < tag->uidLength; i++) {
        if (tag->uid[i] < 0x10) tagStr += "0";
        tagStr += String(tag->uid[i], HEX);
    }
    tagStr.toUpperCase();

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
//  InDataExchange con debug — ver error code real del PN532
// ────────────────────────────────────────────────────────────
//
// La library Adafruit retorna false si pn532_packetbuffer[7] & 0x3f != 0
// pero no expone el error code. Reimplementamos para ver el error.
//
// Según PN532 UM10232 §7.3.8: InDataExchange ENVÍA RATS automáticamente
// al primer APDU si el target es ISO 14443-4 (SAK bit 6 set).
// No necesita InATR ni InCommunicateThru manual.
static bool nfc_in_data_exchange(uint8_t *send, uint8_t sendLen,
                                  uint8_t *response, uint8_t *respLen)
{
    // Construir frame: [0x40][Tg][data...]
    uint8_t cmd[2 + sendLen];
    cmd[0] = 0x40;  // InDataExchange
    cmd[1] = 0x01;  // Target #1
    memcpy(&cmd[2], send, sendLen);

    if (!nfc.sendCommandCheckAck(cmd, 2 + sendLen, 1000)) {
        Serial.println("[NFC] InDataExchange: ACK timeout");
        return false;
    }

    // Poll RDY
    bool ready = false;
    uint32_t t0 = millis();
    while (millis() - t0 < 2000) {
        Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)1);
        if (Wire.available() && (Wire.read() & 0x01)) { ready = true; break; }
        delay(10);
    }
    if (!ready) {
        Serial.println("[NFC] InDataExchange: RDY timeout");
        return false;
    }

    // Leer respuesta completa (JSON ~40 bytes + PN532 frame ~10 bytes)
    // Wire I2C buffer en ESP32 es 128 bytes por defecto
    uint8_t buf[128];
    Wire.requestFrom((uint8_t)PN532_I2C_ADDRESS, (uint8_t)sizeof(buf));
    uint8_t n = 0;
    while (Wire.available() && n < sizeof(buf)) {
        buf[n++] = Wire.read();
    }

    // Buscar frame: D5 41 [Status] [data...]
    for (uint8_t i = 0; i + 2 < n; i++) {
        if (buf[i] == 0xD5 && buf[i + 1] == 0x41) {
            uint8_t status = buf[i + 2];
            Serial.printf("[NFC] InDataExchange status: 0x%02X\n", status);

            if ((status & 0x3F) != 0) {
                Serial.printf("[NFC] PN532 error code: 0x%02X — ", status & 0x3F);
                switch (status & 0x3F) {
                    case 0x01: Serial.println("Timeout"); break;
                    case 0x02: Serial.println("CRC Error"); break;
                    case 0x03: Serial.println("Parity Error"); break;
                    case 0x05: Serial.println("Framing Error"); break;
                    case 0x06: Serial.println("Collision"); break;
                    case 0x0A: Serial.println("Buffer Overflow"); break;
                    case 0x0D: Serial.println("RF Protocol Error"); break;
                    case 0x29: Serial.println("DEP Protocol Error"); break;
                    default:   Serial.println("Unknown"); break;
                }
                // Dump raw para debug
                Serial.printf("[NFC] Raw response (%d bytes): ", n);
                nfc.PrintHex(buf, min(n, (uint8_t)32));
                return false;
            }

            // Éxito — extraer data (todo después del status byte)
            // Frame: [RDY][00][00][FF][LEN][LCS][D5][41][Status][...data...][DCS][00]
            //  index:   0    1    2    3    4     5    6    7       8
            // LEN está en buf[i-2] (2 bytes antes de D5 = index 4)
            uint8_t frameLen = (i >= 2) ? buf[i - 2] : 0;
            // frameLen incluye TFI(1)+cmd(1)+status(1)+data = 3+dataLen
            uint8_t dataLen = (frameLen > 3) ? frameLen - 3 : 0;
            if (dataLen > *respLen) dataLen = *respLen;
            if (i + 3 + dataLen > n) dataLen = (i + 3 < n) ? n - i - 3 : 0;

            memcpy(response, &buf[i + 3], dataLen);
            *respLen = dataLen;
            return true;
        }
    }

    Serial.printf("[NFC] InDataExchange: no D5 41 found (%d bytes)\n", n);
    nfc.PrintHex(buf, min(n, (uint8_t)32));
    return false;
}

// ────────────────────────────────────────────────────────────
//  Lectura JSON via HCE (smartphone)
// ────────────────────────────────────────────────────────────
//
// Flujo:
// 1. readPassiveTargetID() → InListPassiveTarget → target ISO 14443-3
// 2. nfc_in_data_exchange() → InDataExchange → PN532 envía RATS auto
//    si SAK bit6 set → SELECT AID APDU → phone responde JSON + SW 9000
bool nfc_read_hce_payload(char* buffer, uint16_t maxLen)
{
    if (!buffer || maxLen < 2) return false;

    uint8_t aid[] = HCE_AID;
    const uint8_t aidLen = sizeof(aid);

    // SELECT AID APDU: CLA INS P1 P2 Lc [AID] Le
    uint8_t selectCmd[6 + aidLen];
    selectCmd[0] = 0x00;          // CLA
    selectCmd[1] = 0xA4;          // INS = SELECT
    selectCmd[2] = 0x04;          // P1  = Select by name
    selectCmd[3] = 0x00;          // P2  = First occurrence
    selectCmd[4] = aidLen;        // Lc  = AID length
    memcpy(&selectCmd[5], aid, aidLen);
    selectCmd[5 + aidLen] = 0x00; // Le  = Accept any length

    const uint8_t selectLen = 6 + aidLen;

    Serial.printf("[NFC] SELECT AID (%d bytes): ", selectLen);
    nfc.PrintHex(selectCmd, selectLen);

    uint8_t response[255];
    uint8_t respLen = sizeof(response);

    bool ok = nfc_in_data_exchange(selectCmd, selectLen, response, &respLen);

    if (!ok) {
        return false;
    }

    Serial.printf("[NFC] HCE respuesta (%d bytes)\n", respLen);

    if (respLen < 2) {
        Serial.println("[NFC] Respuesta muy corta");
        return false;
    }

    uint8_t sw1 = response[respLen - 2];
    uint8_t sw2 = response[respLen - 1];
    Serial.printf("[NFC] SW: %02X %02X\n", sw1, sw2);

    if (sw1 != 0x90 || sw2 != 0x00) {
        Serial.printf("[NFC] Status inválido (esperado 9000, recibido %02X%02X)\n", sw1, sw2);
        return false;
    }

    uint16_t jsonLen = respLen - 2;
    uint16_t copyLen = (jsonLen < maxLen - 1) ? jsonLen : (maxLen - 1);
    memcpy(buffer, response, copyLen);
    buffer[copyLen] = '\0';

    Serial.printf("[NFC] HCE payload (%u bytes): %s\n", copyLen, buffer);
    return true;
}

bool nfc_read_json_payload(const NfcTag* tag, char* buffer, uint16_t maxLen)
{
    if (!tag || !tag->valid || !buffer || maxLen < 2) return false;

    uint8_t ndefKey[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };

    uint16_t bufPos    = 0;
    bool     started   = false;
    int      depth     = 0;

    for (uint8_t block = 4; block < 64 && bufPos < (uint16_t)(maxLen - 1); block++) {

        if ((block % 4) == 3) continue;

        if ((block % 4) == 0) {
            bool auth = nfc.mifareclassic_AuthenticateBlock(
                (uint8_t*)tag->uid, tag->uidLength,
                block, 0, ndefKey);
            if (!auth) {
                Serial.printf("[NFC] Auth falló en bloque %d\n", block);
                break;
            }
        }

        uint8_t blockData[16];
        if (!nfc.mifareclassic_ReadDataBlock(block, blockData)) {
            Serial.printf("[NFC] Read falló en bloque %d\n", block);
            break;
        }

        for (int i = 0; i < 16 && bufPos < (uint16_t)(maxLen - 1); i++) {
            uint8_t c = blockData[i];

            if (!started) {
                if (c == '{') {
                    started = true;
                    depth   = 1;
                    buffer[bufPos++] = c;
                }
                continue;
            }

            if      (c == '{') depth++;
            else if (c == '}') depth--;

            buffer[bufPos++] = c;

            if (depth == 0) {
                buffer[bufPos] = '\0';
                Serial.printf("[NFC] JSON extraído (%u bytes)\n", bufPos);
                return true;
            }
        }
    }

    return false;
}
