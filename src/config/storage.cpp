/**
 * @file storage.cpp
 * @brief Implementación de persistencia NVS
 */

#include "storage.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;

void storage_init(void)
{
    prefs.begin("lock_cfg", false);
    Serial.println("Storage NVS inicializado");
    Serial.printf("  PIN guardado: %s\n", storage_get_pin().c_str());
    Serial.printf("  NFC UID: %s\n",      storage_get_nfc_uid().c_str());
}

// ── PIN ─────────────────────────────────────────
String storage_get_pin(void)
{
    return prefs.getString("pin", DEFAULT_PIN);
}

void storage_set_pin(const char* pin)
{
    prefs.putString("pin", pin);
    Serial.printf("PIN guardado en NVS: %s\n", pin);
}

// ── NFC UID ──────────────────────────────────────
String storage_get_nfc_uid(void)
{
    return prefs.getString("nfc_uid", "");
}

void storage_set_nfc_uid(const char* uid)
{
    prefs.putString("nfc_uid", uid);
    Serial.printf("NFC UID guardado en NVS: %s\n", uid);
}

// ── Intentos fallidos ───────────────────────────
int storage_get_max_failed_attempts(void)
{
    return prefs.getInt("max_fail", MAX_FAILED_ATTEMPTS);
}

void storage_set_max_failed_attempts(int max)
{
    prefs.putInt("max_fail", max);
}

// ── Auto-lock delay ─────────────────────────────
uint32_t storage_get_auto_lock_delay(void)
{
    return prefs.getUInt("auto_lock", LOCK_AUTO_LOCK_DELAY_MS);
}

void storage_set_auto_lock_delay(uint32_t delayMs)
{
    prefs.putUInt("auto_lock", delayMs);
}

// ── NFC habilitado ──────────────────────────────
bool storage_get_nfc_enabled(void)
{
    return prefs.getBool("nfc_en", true);
}

void storage_set_nfc_enabled(bool enabled)
{
    prefs.putBool("nfc_en", enabled);
}

// ── Duración lockout ────────────────────────────
int storage_get_lockout_duration(void)
{
    return prefs.getInt("lockout_s", (int)(LOCKOUT_DURATION_MS / 1000));
}

void storage_set_lockout_duration(int seconds)
{
    prefs.putInt("lockout_s", seconds);
}
