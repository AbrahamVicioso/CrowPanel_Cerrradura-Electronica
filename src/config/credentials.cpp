/**
 * @file credentials.cpp
 * @brief Implementación del módulo de credenciales.
 *
 * Parsea el array JSON del atributo "credenciales" de ThingsBoard y valida
 * PINs contra ventanas de tiempo en UTC, sin depender del timezone del sistema.
 */

#include "credentials.h"
#include "storage.h"
#include <ArduinoJson.h>
#include <time.h>

// ============================================
// TIPOS INTERNOS
// ============================================

struct Credential {
    char   pin[16];
    time_t activacion;  // 0 = sin restricción de inicio
    time_t expiracion;  // 0 = nunca expira
};

static Credential s_creds[MAX_CREDENTIALS];
static int        s_count = 0;

// ============================================
// HELPERS DE TIEMPO — UTC puro
// ============================================

/**
 * Convierte fecha/hora UTC a Unix timestamp (time_t) usando el
 * algoritmo civil de Howard Hinnant. No depende de mktime ni del
 * timezone configurado en el sistema.
 */
static time_t civil_to_time_t(int y, int mo, int d, int h, int mi, int s)
{
    y -= (mo <= 2) ? 1 : 0;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const int yoe = y - era * 400;
    const int doy = (153 * (mo + (mo > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    const int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    const int32_t days = era * 146097 + doe - 719468;
    return (time_t)days * 86400 + h * 3600 + mi * 60 + s;
}

/**
 * Parsea fecha ISO 8601 UTC: "2026-04-17T22:54:18.8207937"
 * Ignora fracción de segundos y sufijo 'Z' si existe.
 * @return Unix timestamp UTC, o 0 si el parseo falla.
 */
static time_t parse_iso8601(const char* str)
{
    if (!str || str[0] == '\0') return 0;
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (sscanf(str, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) < 6) return 0;
    return civil_to_time_t(y, mo, d, h, mi, s);
}

/** Formatea un time_t UTC a string legible (para logging) */
static void fmt_time(time_t t, char* buf, size_t len)
{
    if (t == 0) { snprintf(buf, len, "(sin fecha)"); return; }
    struct tm* tm = gmtime(&t);
    if (tm) snprintf(buf, len, "%04d-%02d-%02dT%02d:%02d:%02d UTC",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    else snprintf(buf, len, "(%ld)", (long)t);
}

// ============================================
// API PÚBLICA
// ============================================

bool credentials_update(const char* json)
{
    DynamicJsonDocument doc(2048);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Creds] Error parse JSON: %s\n", err.c_str());
        return false;
    }

    JsonArray arr = doc.as<JsonArray>();
    if (arr.isNull()) {
        Serial.println("[Creds] El valor de 'credenciales' no es un array");
        return false;
    }

    s_count = 0;
    char tbuf1[32], tbuf2[32];

    for (JsonObject obj : arr) {
        if (s_count >= MAX_CREDENTIALS) break;

        const char* pin  = obj["pin"]  | (const char*)nullptr;
        const char* tipo = obj["tipo"] | "?";

        Credential& c = s_creds[s_count];

        // Guardar PIN (vacío si no tiene)
        if (pin && pin[0] != '\0') {
            strncpy(c.pin, pin, sizeof(c.pin) - 1);
            c.pin[sizeof(c.pin) - 1] = '\0';
        } else {
            c.pin[0] = '\0';
        }

        if (strcmp(tipo, "huesped") == 0) {
            c.activacion = parse_iso8601(obj["activacion"] | "");
            c.expiracion = parse_iso8601(obj["expiracion"] | "");
        } else {
            c.activacion = 0;
            JsonVariant expVar = obj["expiracion"];
            c.expiracion = expVar.isNull() ? 0 : parse_iso8601(expVar.as<const char*>());
        }

        s_count++;
    }

    // Persistir JSON en NVS para operar sin internet
    storage_set_credentials(json);

    // Mostrar lista en serial
    Serial.println("[Creds] ────────────────────────────────────────────");
    Serial.printf( "[Creds] %d credenciales cargadas:\n", s_count);
    for (int i = 0; i < s_count; i++) {
        fmt_time(s_creds[i].activacion, tbuf1, sizeof(tbuf1));
        fmt_time(s_creds[i].expiracion, tbuf2, sizeof(tbuf2));
        Serial.printf("[Creds]  #%d  pin=%-8s  activa=%-30s  expira=%s\n",
                      i + 1,
                      s_creds[i].pin[0] ? s_creds[i].pin : "(sin pin)",
                      tbuf1, tbuf2);
    }
    Serial.println("[Creds] ────────────────────────────────────────────");

    return true;
}

bool credentials_validate_pin(const String& pin)
{
    if (s_count == 0) {
        Serial.println("[Creds] Sin credenciales cargadas — acceso denegado");
        return false;
    }

    time_t now = time(nullptr);
    bool ntp_ok = (now > 1700000000L);  // > Nov 2023 = NTP sincronizado

    if (ntp_ok) {
        char nowbuf[32];
        fmt_time(now, nowbuf, sizeof(nowbuf));
        Serial.printf("[Creds] Hora actual: %s\n", nowbuf);
    } else {
        Serial.printf("[Creds] WARN: NTP no sincronizado (now=%ld)\n", (long)now);
    }

    char tbuf1[32], tbuf2[32];

    for (int i = 0; i < s_count; i++) {
        if (pin != s_creds[i].pin) continue;

        Serial.printf("[Creds] PIN coincide con credencial #%d\n", i + 1);

        // Sin NTP: omitir validación temporal y permitir acceso
        if (!ntp_ok) {
            Serial.println("[Creds] WARN: sin NTP — validación temporal omitida, acceso concedido");
            return true;
        }

        if (s_creds[i].activacion != 0 && now < s_creds[i].activacion) {
            fmt_time(s_creds[i].activacion, tbuf1, sizeof(tbuf1));
            long faltan = (long)(s_creds[i].activacion - now);
            Serial.printf("[Creds] Denegado: no activo hasta %s (faltan %ldh %ldm)\n",
                          tbuf1, faltan/3600, (faltan%3600)/60);
            continue;
        }

        if (s_creds[i].expiracion != 0 && now > s_creds[i].expiracion) {
            fmt_time(s_creds[i].expiracion, tbuf2, sizeof(tbuf2));
            long hace = (long)(now - s_creds[i].expiracion);
            Serial.printf("[Creds] Denegado: expirado el %s (hace %ldh %ldm)\n",
                          tbuf2, hace/3600, (hace%3600)/60);
            continue;
        }

        Serial.println("[Creds] Acceso concedido — PIN válido y vigente");
        return true;
    }

    Serial.println("[Creds] PIN no coincide con ninguna credencial activa");
    return false;
}

int credentials_get_count(void)
{
    return s_count;
}
