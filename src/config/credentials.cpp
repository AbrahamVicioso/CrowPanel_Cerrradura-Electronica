/**
 * @file credentials.cpp
 * @brief Implementación del módulo de credenciales.
 *
 * Parsea el objeto JSON {"huespedes":[...],"personal":[...]} desde ThingsBoard.
 * Cada persona tiene un array "credenciales" con pin, activacion, expiracion (UTC).
 * Usa DynamicJsonDocument (16KB heap DRAM) para soportar payloads de tamaño moderado-grande.
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
    time_t activacion;       // 0 = sin restricción de inicio
    time_t expiracion;       // 0 = nunca expira
    char   activacion_str[24]; // ISO 8601 original, para telemetría
    char   expiracion_str[24]; // ISO 8601 original, para telemetría
    char   tipo;             // 'H' = huesped, 'P' = personal
    int    owner_id;         // huespedId (H) o personalId (P)
    int    reserva_id;       // solo para huespedes
    char   nombre[48];       // solo para personal (vacío en huespedes)
};

static Credential    s_creds[MAX_CREDENTIALS];
static int           s_count = 0;

// Última credencial que coincidió en una validación exitosa
static CredentialMatch s_last_match;
static bool            s_last_match_valid = false;

// ============================================
// HELPERS DE TIEMPO — UTC puro
// ============================================

/**
 * Convierte fecha/hora UTC a Unix timestamp (time_t) usando el
 * algoritmo civil de Howard Hinnant. Solo usar para timestamps
 * explícitamente UTC (con sufijo 'Z'). Para hora local usar mktime.
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
 * Parsea fecha ISO 8601 a Unix timestamp UTC.
 * - Sin sufijo 'Z' → hora local del dispositivo (AST = UTC-4). Usa mktime.
 * - Con sufijo 'Z'  → UTC explícito. Usa algoritmo Hinnant.
 * El servidor envía hora local de República Dominicana sin 'Z'.
 * @return Unix timestamp UTC, o 0 si el parseo falla.
 */
static time_t parse_iso8601(const char* str)
{
    if (!str || str[0] == '\0') return 0;
    int y = 0, mo = 0, d = 0, h = 0, mi = 0, s = 0;
    if (sscanf(str, "%d-%d-%dT%d:%d:%d", &y, &mo, &d, &h, &mi, &s) < 6) return 0;

    // Detectar si el string contiene 'Z' (UTC explícito)
    bool is_utc = (strchr(str, 'Z') != nullptr);

    if (is_utc) {
        // Usar algoritmo Hinnant: interpreta como UTC
        return civil_to_time_t(y, mo, d, h, mi, s);
    } else {
        // Usar mktime: interpreta como hora local (TZ = "AST4" = UTC-4)
        struct tm tm = {};
        tm.tm_year  = y - 1900;
        tm.tm_mon   = mo - 1;
        tm.tm_mday  = d;
        tm.tm_hour  = h;
        tm.tm_min   = mi;
        tm.tm_sec   = s;
        tm.tm_isdst = 0;
        return mktime(&tm);
    }
}

/** Formatea un time_t UTC a string legible (para logging) */
static void fmt_time(time_t t, char* buf, size_t len)
{
    if (t == 0) { snprintf(buf, len, "(sin fecha)"); return; }
    struct tm* tm = gmtime(&t);
    if (tm) snprintf(buf, len, "%04d-%02d-%02dT%02d:%02d:%02d",
        tm->tm_year+1900, tm->tm_mon+1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    else snprintf(buf, len, "(%ld)", (long)t);
}

/**
 * Extrae credenciales de un JsonArray "credenciales" y las añade a s_creds.
 * @param arr        Array de objetos {pin, activacion, expiracion}
 * @param tipo       'H' o 'P'
 * @param owner_id   huespedId o personalId
 * @param reserva_id reservaId (solo huespedes; 0 para personal)
 * @param nombre     Nombre del personal (solo personal; "" para huespedes)
 * @return           Número de credenciales añadidas
 */
static int load_credentials_array(JsonArrayConst arr, char tipo,
                                   int owner_id, int reserva_id,
                                   const char* nombre)
{
    int added = 0;
    for (JsonVariantConst elem : arr) {
        JsonObjectConst cred = elem.as<JsonObjectConst>();
        if (cred.isNull()) continue;
        if (s_count >= MAX_CREDENTIALS) break;

        const char* pin = cred["pin"] | "";
        if (pin[0] == '\0') continue;  // saltar credenciales sin PIN

        const char* act_str = cred["activacion"] | "";
        const char* exp_str = cred["expiracion"] | "";

        Credential& c = s_creds[s_count];
        strncpy(c.pin, pin, sizeof(c.pin) - 1);
        c.pin[sizeof(c.pin) - 1] = '\0';
        c.activacion = parse_iso8601(act_str);
        c.expiracion = parse_iso8601(exp_str);
        strncpy(c.activacion_str, act_str, sizeof(c.activacion_str) - 1);
        c.activacion_str[sizeof(c.activacion_str) - 1] = '\0';
        strncpy(c.expiracion_str, exp_str, sizeof(c.expiracion_str) - 1);
        c.expiracion_str[sizeof(c.expiracion_str) - 1] = '\0';
        c.tipo       = tipo;
        c.owner_id   = owner_id;
        c.reserva_id = reserva_id;
        strncpy(c.nombre, nombre ? nombre : "", sizeof(c.nombre) - 1);
        c.nombre[sizeof(c.nombre) - 1] = '\0';

        s_count++;
        added++;
    }
    return added;
}

// ============================================
// API PÚBLICA
// ============================================

// Allocator que usa PSRAM cuando está disponible, con fallback a DRAM.
// Evita consumir 16KB de DRAM limitada en el parse de credenciales.
struct PsramAllocator {
    void* allocate(size_t size) {
        if (psramFound()) return heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        return malloc(size);
    }
    void deallocate(void* ptr) { free(ptr); }
    void* reallocate(void* ptr, size_t new_size) {
        if (psramFound()) return heap_caps_realloc(ptr, new_size, MALLOC_CAP_SPIRAM);
        return realloc(ptr, new_size);
    }
};

bool credentials_update(const char* json)
{
    Serial.printf("[Creds] Actualizando credenciales (DRAM libre: %d KB)\n", ESP.getFreeHeap() / 1024);
    // 16KB en PSRAM (fallback DRAM) — suficiente para decenas de huéspedes/personal
    BasicJsonDocument<PsramAllocator> doc(16384);
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("[Creds] Error parse JSON: %s\n", err.c_str());
        return false;
    }

    s_count = 0;
    int n_huespedes = 0, n_personal = 0;
    int c_huespedes = 0, c_personal = 0;

    // ── Huéspedes ──────────────────────────────────────────
    JsonArrayConst huespedes = doc["huespedes"].as<JsonArrayConst>();
    if (!huespedes.isNull()) {
        for (JsonVariantConst hv : huespedes) {
            JsonObjectConst h = hv.as<JsonObjectConst>();
            if (h.isNull()) continue;
            n_huespedes++;
            int huesped_id  = h["huespedId"]  | 0;
            int reserva_id  = h["reservaId"]  | 0;
            JsonArrayConst creds = h["credenciales"].as<JsonArrayConst>();
            if (!creds.isNull()) {
                c_huespedes += load_credentials_array(creds, 'H', huesped_id, reserva_id, "");
            }
        }
    }

    // ── Personal ───────────────────────────────────────────
    JsonArrayConst personal = doc["personal"].as<JsonArrayConst>();
    if (!personal.isNull()) {
        for (JsonVariantConst pv : personal) {
            JsonObjectConst p = pv.as<JsonObjectConst>();
            if (p.isNull()) continue;
            n_personal++;
            int         personal_id = p["personalId"] | 0;
            const char* nombre      = p["nombre"]     | "";
            JsonArrayConst creds = p["credenciales"].as<JsonArrayConst>();
            if (!creds.isNull()) {
                c_personal += load_credentials_array(creds, 'P', personal_id, 0, nombre);
            }
        }
    }

    if (s_count == 0) {
        Serial.println("[Creds] WARN: JSON parseado pero sin PINs válidos");
    }

    // Persistir en SPIFFS para operar sin internet
    storage_set_credentials(json);

    // ── Log en serial ──────────────────────────────────────
    char tbuf1[32], tbuf2[32];
    Serial.println("[Creds] ─────────────────────────────────────────────────");
    Serial.printf( "[Creds] Huéspedes: %d persona(s) → %d PIN(s)\n", n_huespedes, c_huespedes);
    for (int i = 0; i < s_count; i++) {
        if (s_creds[i].tipo != 'H') continue;
        fmt_time(s_creds[i].activacion, tbuf1, sizeof(tbuf1));
        fmt_time(s_creds[i].expiracion, tbuf2, sizeof(tbuf2));
        Serial.printf("[Creds]  H  pin=%-8s  desde=%-20s  hasta=%s\n",
                      s_creds[i].pin, tbuf1, tbuf2);
    }
    Serial.printf( "[Creds] Personal:  %d persona(s) → %d PIN(s)\n", n_personal, c_personal);
    for (int i = 0; i < s_count; i++) {
        if (s_creds[i].tipo != 'P') continue;
        fmt_time(s_creds[i].activacion, tbuf1, sizeof(tbuf1));
        fmt_time(s_creds[i].expiracion, tbuf2, sizeof(tbuf2));
        Serial.printf("[Creds]  P  pin=%-8s  desde=%-20s  hasta=%s\n",
                      s_creds[i].pin, tbuf1, tbuf2);
    }
    Serial.printf( "[Creds] Total: %d PIN(s) cargados\n", s_count);
    Serial.println("[Creds] ─────────────────────────────────────────────────");

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
        Serial.printf("[Creds] Validando PIN, hora UTC: %s\n", nowbuf);
    } else {
        Serial.printf("[Creds] WARN: NTP no sincronizado (now=%ld)\n", (long)now);
    }

    char tbuf1[32], tbuf2[32];

    for (int i = 0; i < s_count; i++) {
        if (pin != s_creds[i].pin) continue;

        Serial.printf("[Creds] PIN coincide con credencial #%d (%c)\n",
                      i + 1, s_creds[i].tipo);

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

        // Guardar coincidencia para telemetría de acceso
        s_last_match_valid          = true;
        s_last_match.tipo           = s_creds[i].tipo;
        s_last_match.owner_id       = s_creds[i].owner_id;
        s_last_match.reserva_id     = s_creds[i].reserva_id;
        strncpy(s_last_match.nombre,        s_creds[i].nombre,        sizeof(s_last_match.nombre)        - 1);
        strncpy(s_last_match.pin,           s_creds[i].pin,           sizeof(s_last_match.pin)           - 1);
        strncpy(s_last_match.activacion_str, s_creds[i].activacion_str, sizeof(s_last_match.activacion_str) - 1);
        strncpy(s_last_match.expiracion_str, s_creds[i].expiracion_str, sizeof(s_last_match.expiracion_str) - 1);
        s_last_match.nombre[sizeof(s_last_match.nombre) - 1]               = '\0';
        s_last_match.pin[sizeof(s_last_match.pin) - 1]                     = '\0';
        s_last_match.activacion_str[sizeof(s_last_match.activacion_str) - 1] = '\0';
        s_last_match.expiracion_str[sizeof(s_last_match.expiracion_str) - 1] = '\0';

        return true;
    }

    Serial.println("[Creds] PIN no coincide con ninguna credencial activa");
    return false;
}

bool credentials_get_last_match(CredentialMatch* out)
{
    if (!out || !s_last_match_valid) return false;
    *out = s_last_match;
    s_last_match_valid = false;  // consumir — evita reuso en siguiente evento
    return true;
}

int credentials_get_count(void)
{
    return s_count;
}
