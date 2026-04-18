/**
 * @file main.cpp
 * @brief Sistema de Cerradura Electrónica v2.0
 *
 * Mejoras v2.0:
 *  - ThingsBoard: 7 shared attributes + 4 RPC + telemetría completa
 *  - NFC: verificación de UID autorizado (configurable desde TB)
 *  - Seguridad: lockout por intentos fallidos (configurable desde TB)
 *  - Persistencia NVS: PIN y configuración sobreviven al reinicio
 *  - Standby: indicadores WiFi/TB en tiempo real
 *  - PIN y auto-lock delay: configurables remotamente desde TB Dashboard
 */

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#include "config/config.h"
#include "config/pins.h"
#include "config/network_config.h"
#include "config/storage.h"
#include "config/credentials.h"

#include "hardware/display.h"
#include "hardware/touch.h"
#include "hardware/nfc.h"
#include "hardware/lock.h"

#include "communication/thingsboard.h"

#include "ui/theme.h"
#include "ui/screens/welcome_screen.h"
#include "ui/screens/standby_screen.h"
#include "ui/screens/pinpad_screen.h"
#include "ui/screens/unlocked_screen.h"
#include "ui/screens/locked_screen.h"
#include "ui/screens/error_screen.h"
#include "ui/screens/config_screen.h"
#include "ui/screens/info_screen.h"

#include "config/portal.h"

// ============================================
// VARIABLES GLOBALES LVGL
// ============================================

static lv_disp_draw_buf_t draw_buf;
static lv_color_t*        disp_buf1;
static lv_color_t*        disp_buf2;
static lv_disp_drv_t      disp_drv;

// Timer NFC
static lv_timer_t* nfc_timer = nullptr;

// Flag de control de flujo (evitar pantallas concurrentes)
static volatile bool screen_busy  = false;
static volatile bool is_unlocking = false;

// Config dinámica (puede cambiar via ThingsBoard)
static uint32_t current_auto_lock_delay = LOCK_AUTO_LOCK_DELAY_MS;
static bool     nfc_enabled             = true;
static int      failed_attempts_count   = 0;

// Pantallas
static lv_obj_t* screen_welcome  = nullptr;
static lv_obj_t* screen_standby  = nullptr;
static lv_obj_t* screen_pinpad   = nullptr;
static lv_obj_t* screen_unlocked = nullptr;
static lv_obj_t* screen_locked   = nullptr;
static lv_obj_t* screen_error    = nullptr;

// ============================================
// CALLBACKS LVGL
// ============================================

void my_disp_flush(lv_disp_drv_t* disp, const lv_area_t* area, lv_color_t* color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)&color_p->full);
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t* indev_driver, lv_indev_data_t* data)
{
    ts.read();
    if (ts.isTouched) {
        data->state   = LV_INDEV_STATE_PR;
        data->point.x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width()  - 1);
        data->point.y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// ============================================
// FLUJO DE PANTALLAS
// ============================================

void on_return_to_standby(void)
{
    pinpad_reset();
    display_turn_on();
    lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    screen_busy = false;
}

void on_lock_after_unlock(void)
{
    lock_lock();
    thingsboard_publish_lock_state(lock_get_state());
    pinpad_reset();
    locked_screen_show(on_return_to_standby);
}

// ── Desbloqueo (cualquier método) ───────────────────────────

static void do_unlock(const char* method)
{
    if (screen_busy || is_unlocking) return;
    screen_busy  = true;
    is_unlocking = true;

    lock_unlock();
    thingsboard_publish_lock_state(lock_get_state());
    thingsboard_publish_access_event(true, method);
    display_turn_on();

    unlocked_screen_show(current_auto_lock_delay, on_lock_after_unlock);
    is_unlocking = false;
}

// ── RPC unlockTemporary (duración personalizada) ─────────────

// Timer para bloquear tras unlockTemporary
static lv_timer_t* rpc_temp_timer = nullptr;

static void rpc_temp_lock_cb(lv_timer_t* t)
{
    lv_timer_del(t);
    rpc_temp_timer = nullptr;
    on_lock_after_unlock();
}

static void do_unlock_temporary(uint32_t durationMs)
{
    if (screen_busy || is_unlocking) return;
    screen_busy  = true;
    is_unlocking = true;

    lock_unlock();
    thingsboard_publish_lock_state(lock_get_state());
    thingsboard_publish_access_event(true, "rpc_temp");
    display_turn_on();

    // Mostrar pantalla desbloqueada con el tiempo especificado
    unlocked_screen_show(durationMs, on_lock_after_unlock);
    is_unlocking = false;
}

// ============================================
// CALLBACKS DE ACCESO
// ============================================

void on_pin_success(void)
{
    do_unlock("pin");
}

void on_pin_error(void)
{
    // Si hay lockout activo, el pinpad ya muestra el countdown — no cambiar pantalla
    if (pinpad_is_locked_out()) return;

    // PIN incorrecto sin lockout: mostrar error screen brevemente
    if (screen_busy) return;
    screen_busy = true;
    display_turn_on();
    error_screen_show([]() {
        // Volver al pinpad (no a standby, para que el usuario reintente)
        display_turn_on();
        lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        screen_busy = false;
    }, 1200);
}

void on_failed_attempts_changed(int count)
{
    failed_attempts_count = count;
    thingsboard_publish_failed_attempts(count);
    Serial.printf("[Main] Intentos fallidos: %d/%d\n",
                  count, storage_get_max_failed_attempts());
}

void on_error_return_to_standby(void)
{
    pinpad_reset();
    display_turn_on();
    lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    screen_busy = false;
}

// ============================================
// CALLBACKS THINGSBOARD — SHARED ATTRIBUTES
// ============================================

void on_remote_state_change(bool locked)
{
    Serial.printf("[Main] Comando remoto: %s\n", locked ? "BLOQUEAR" : "DESBLOQUEAR");

    if (locked) {
        lock_lock();
        thingsboard_publish_lock_state(lock_get_state());
        thingsboard_publish_access_event(false, "remote");
        // Si no está en standby → volver a standby
        if (lv_scr_act() != screen_standby) {
            screen_busy = false;
            display_turn_on();
            lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        }
    } else {
        do_unlock("remote");
    }
}

// ============================================
// TOAST — CREDENCIALES ACTUALIZADAS
// ============================================

static void credentials_toast_fade_out_cb(lv_timer_t* t)
{
    lv_obj_t* toast = (lv_obj_t*)t->user_data;
    lv_timer_del(t);

    if (!toast || !lv_obj_is_valid(toast)) return;

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, toast);
    lv_anim_set_values(&a, LV_OPA_COVER, LV_OPA_TRANSP);
    lv_anim_set_time(&a, 400);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t v){
        if (lv_obj_is_valid((lv_obj_t*)obj))
            lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
    });
    lv_anim_set_deleted_cb(&a, [](lv_anim_t* a2){
        lv_obj_t* obj = (lv_obj_t*)a2->var;
        if (lv_obj_is_valid(obj)) lv_obj_del(obj);
    });
    lv_anim_start(&a);
}

static void show_credentials_toast(int count)
{
    // Panel flotante sobre cualquier pantalla
    lv_obj_t* toast = lv_obj_create(lv_layer_top());
    lv_obj_set_size(toast, 480, 80);
    lv_obj_align(toast, LV_ALIGN_TOP_MID, 0, 24);
    lv_obj_set_style_bg_color(toast, lv_color_hex(0x00b894), 0);  // COLOR_SUCCESS
    lv_obj_set_style_bg_opa(toast, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(toast, 20, 0);
    lv_obj_set_style_border_width(toast, 0, 0);
    lv_obj_set_style_shadow_width(toast, 24, 0);
    lv_obj_set_style_shadow_color(toast, lv_color_hex(0x000000), 0);
    lv_obj_set_style_shadow_opa(toast, LV_OPA_30, 0);
    lv_obj_set_style_pad_all(toast, 0, 0);
    lv_obj_clear_flag(toast, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_opa(toast, LV_OPA_TRANSP, 0);  // empieza invisible

    // Ícono
    lv_obj_t* icon = lv_label_create(toast);
    lv_label_set_text(icon, LV_SYMBOL_REFRESH);
    lv_obj_set_style_text_font(icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(icon, lv_color_white(), 0);
    lv_obj_align(icon, LV_ALIGN_LEFT_MID, 24, 0);

    // Texto principal
    lv_obj_t* lbl = lv_label_create(toast);
    lv_label_set_text(lbl, "Credenciales actualizadas");
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 62, -10);

    // Subtexto con cantidad
    lv_obj_t* sub = lv_label_create(toast);
    char buf[40];
    snprintf(buf, sizeof(buf), "%d credencial%s activa%s",
             count, count == 1 ? "" : "es", count == 1 ? "" : "s");
    lv_label_set_text(sub, buf);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xdfe6e9), 0);
    lv_obj_align(sub, LV_ALIGN_LEFT_MID, 62, 12);

    // Fade in
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, toast);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, [](void* obj, int32_t v){
        if (lv_obj_is_valid((lv_obj_t*)obj))
            lv_obj_set_style_opa((lv_obj_t*)obj, (lv_opa_t)v, 0);
    });
    lv_anim_start(&a);

    // Auto-cerrar con fade out después de 3.5s
    lv_timer_t* t = lv_timer_create(credentials_toast_fade_out_cb, 3500, toast);
    lv_timer_set_repeat_count(t, 1);
}

void on_credentials_update(const char* json)
{
    Serial.println("[Main] Credenciales recibidas desde TB");
    credentials_update(json);
    show_credentials_toast(credentials_get_count());
}

void on_auto_lock_delay_update(uint32_t delayMs)
{
    current_auto_lock_delay = delayMs;
    Serial.printf("[Main] Auto-lock delay: %u ms\n", delayMs);
}

void on_nfc_enabled_update(bool enabled)
{
    nfc_enabled = enabled;
    Serial.printf("[Main] NFC %s\n", enabled ? "habilitado" : "deshabilitado");
}

void on_nfc_uid_update(const char* uid)
{
    nfc_set_authorized_uid(uid);
}

void on_max_failed_update(int max)
{
    pinpad_set_max_failed_attempts(max);
}

void on_lockout_duration_update(int seconds)
{
    pinpad_set_lockout_duration_ms((uint32_t)seconds * 1000);
}

// ============================================
// CALLBACKS THINGSBOARD — RPC
// ============================================

void on_rpc_unlock_temporary(uint32_t durationMs)
{
    do_unlock_temporary(durationMs);
}

void on_rpc_reset_lockout(void)
{
    pinpad_reset_failed_attempts();
    failed_attempts_count = 0;
    thingsboard_publish_failed_attempts(0);
}

// ============================================
// TIMER NFC
// ============================================

void nfc_check_timer_cb(lv_timer_t* timer)
{
    if (!nfc_enabled)           return;
    if (screen_busy)            return;
    if (is_unlocking)           return;
    if (pinpad_is_locked_out()) return;

    NfcTag tag;
    if (!nfc_read_tag(&tag)) return;

    // ── Intento 1: JSON payload en tarjeta MIFARE Classic ───────
    static char json_buf[MIFARE_JSON_MAX_LEN];
    if (nfc_read_json_payload(&tag, json_buf, sizeof(json_buf))) {
        StaticJsonDocument<512> doc;
        DeserializationError err = deserializeJson(doc, json_buf);
        if (!err) {
            // Soporta tanto huéspedes como personal: ambos usan "credencial"→"pin"
            const char* rawPin = doc["credencial"]["pin"];
            if (rawPin && strlen(rawPin) > 0) {
                String extractedPin = String(rawPin);
                Serial.printf("[NFC] PIN leído de tarjeta MIFARE (%u chars)\n",
                              extractedPin.length());
                if (credentials_validate_pin(extractedPin)) {
                    Serial.println("[NFC] PIN en tarjeta válido — desbloqueando");
                    do_unlock("nfc_json");
                } else {
                    Serial.println("[NFC] PIN en tarjeta no válido o expirado");
                    thingsboard_publish_access_event(false, "nfc_json");
                    if (lv_scr_act() == screen_pinpad) {
                        pinpad_show_status("Credencial no válida", COLOR_ERROR);
                    }
                }
                return;  // No continuar con chequeo de UID
            }
        }
    }

    // ── Intento 2 (fallback): verificación por UID ───────────────
    if (nfc_is_authorized(&tag)) {
        Serial.println("[Main] NFC autorizado (UID)");
        do_unlock("nfc");
    } else {
        Serial.println("[Main] NFC rechazado — UID no autorizado");
        thingsboard_publish_access_event(false, "nfc");
        if (lv_scr_act() == screen_pinpad) {
            pinpad_show_status("Tarjeta no autorizada", COLOR_ERROR);
        }
    }
}

// ============================================
// PANTALLA DE INFORMACIÓN
// ============================================

void on_info_activate(void)
{
    bool tbOk = thingsboard_is_connected();
    info_screen_show(tbOk, nfc_enabled, []() {
        display_turn_on();
        lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
    });
}

// ============================================
// PORTAL DE CONFIGURACIÓN
// ============================================

void on_portal_activate(void)
{
    Serial.println("[Main] Gesto oculto detectado — iniciando portal de configuración");

    // Detener NFC y ThingsBoard
    if (nfc_timer) {
        lv_timer_del(nfc_timer);
        nfc_timer = nullptr;
    }
    thingsboard_disconnect();

    // Iniciar AP + WebServer
    portal_start();

    // Mostrar pantalla de configuración
    config_screen_show(portal_get_ap_ssid(), portal_get_ap_pass(), portal_get_ap_ip());
}

// ============================================
// WiFi
// ============================================

void connect_wifi(void)
{
    String ssid = storage_get_wifi_ssid();
    String pass = storage_get_wifi_pass();
    Serial.printf("Conectando a WiFi: %s ...\n", ssid.c_str());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), pass.c_str());

    for (int i = 0; i < 30 && WiFi.status() != WL_CONNECTED; i++) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi conectado! IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.printf("WiFi fallido (estado %d) — operando sin conexión\n", WiFi.status());
    }
}

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.printf("Cerradura Electronica v%s\n", FIRMWARE_VERSION);
    Serial.println("========================================\n");

    // 1. Storage NVS
    Serial.println("[1/7] Storage NVS...");
    storage_init();

    // Cargar credenciales guardadas (contingencia sin internet)
    String savedCreds = storage_get_credentials();
    if (savedCreds.length() > 0) {
        Serial.println("[Main] Credenciales previas encontradas en NVS:");
        credentials_update(savedCreds.c_str());
    } else {
        Serial.println("[Main] Sin credenciales en NVS — esperando ThingsBoard");
    }

    // 2. Hardware
    Serial.println("[2/7] Hardware (cerradura)...");
    lock_init();

    // 3. Display
    Serial.println("[3/7] Display...");
    display_init();

    // 4. WiFi + NTP
    Serial.println("[4/7] WiFi...");
    connect_wifi();

    if (WiFi.status() == WL_CONNECTED) {
        // Timezone: República Dominicana (AST, UTC-4, sin DST)
        setenv("TZ", "AST4", 1);
        tzset();
        configTime(-4 * 3600, 0, "pool.ntp.org", "time.google.com");

        struct tm t;
        Serial.print("Sincronizando NTP");
        for (int i = 0; i < 10 && !getLocalTime(&t); i++) {
            delay(500); Serial.print(".");
        }
        Serial.println();
        if (getLocalTime(&t)) {
            Serial.printf("Hora: %02d:%02d:%02d\n", t.tm_hour, t.tm_min, t.tm_sec);
        }
    }

    // 5. ThingsBoard
    Serial.println("[5/7] ThingsBoard...");
    if (WiFi.status() == WL_CONNECTED) {
        thingsboard_init();

        // Registrar todos los callbacks ANTES de conectar
        thingsboard_set_state_change_callback(on_remote_state_change);
        thingsboard_set_credentials_update_callback(on_credentials_update);
        thingsboard_set_auto_lock_delay_callback(on_auto_lock_delay_update);
        thingsboard_set_nfc_enabled_callback(on_nfc_enabled_update);
        thingsboard_set_nfc_uid_callback(on_nfc_uid_update);
        thingsboard_set_max_failed_callback(on_max_failed_update);
        thingsboard_set_lockout_duration_callback(on_lockout_duration_update);
        thingsboard_set_rpc_unlock_temp_callback(on_rpc_unlock_temporary);
        thingsboard_set_rpc_reset_lockout_callback(on_rpc_reset_lockout);

        thingsboard_connect();
    }

    // 6. LVGL
    Serial.println("[6/7] LVGL + UI...");
    lv_init();

    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    ts.begin();
    ts.setRotation(TOUCH_ROTATION);

    // Buffers en PSRAM (pantalla 800×480 = 768KB total)
    uint32_t buf_size = DISPLAY_WIDTH * DISPLAY_HEIGHT / 5;
    disp_buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    disp_buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!disp_buf1 || !disp_buf2) {
        Serial.println("WARN: PSRAM no disponible, usando DRAM (buffer reducido)");
        buf_size /= 4;
        if (!disp_buf1) disp_buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
        if (!disp_buf2) disp_buf2 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
    }
    lv_disp_draw_buf_init(&draw_buf, disp_buf1, disp_buf2, buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res     = DISPLAY_WIDTH;
    disp_drv.ver_res     = DISPLAY_HEIGHT;
    disp_drv.flush_cb    = my_disp_flush;
    disp_drv.draw_buf    = &draw_buf;
    disp_drv.full_refresh = 0;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    display_turn_on();

    // Crear pantallas
    theme_init_styles();
    screen_welcome  = welcome_screen_create();
    screen_standby  = standby_screen_create();
    screen_pinpad   = pinpad_screen_create();
    screen_unlocked = unlocked_screen_create();
    screen_locked   = locked_screen_create();
    screen_error    = error_screen_create();

    // Registrar validador de PIN (usa credenciales recibidas desde TB)
    pinpad_set_pin_validator(credentials_validate_pin);
    pinpad_set_max_failed_attempts(storage_get_max_failed_attempts());
    pinpad_set_lockout_duration_ms((uint32_t)storage_get_lockout_duration() * 1000);
    current_auto_lock_delay = storage_get_auto_lock_delay();
    nfc_enabled = storage_get_nfc_enabled();

    // Callbacks del pinpad
    pinpad_set_success_callback(on_pin_success);
    pinpad_set_error_callback(on_pin_error);
    pinpad_set_failed_attempts_callback(on_failed_attempts_changed);
    pinpad_set_inactivity_callback([]() {
        screen_busy = false;
        display_turn_on();
        lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    });

    // Callback tap en standby → abrir pinpad
    standby_screen_set_tap_callback([]() {
        display_turn_on();
        pinpad_screen_show(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0);
    });

    // Callback gesto oculto → portal de configuración
    standby_screen_set_portal_callback(on_portal_activate);

    // Callback long press → pantalla de información
    standby_screen_set_info_callback(on_info_activate);

    // Bienvenida → standby
    welcome_screen_animate_to(screen_standby);

    // 7. NFC
    Serial.println("[7/7] NFC...");
    nfc_init();
    // Cargar UID autorizado desde NVS
    String savedUid = storage_get_nfc_uid();
    if (savedUid.length() > 0) {
        nfc_set_authorized_uid(savedUid.c_str());
    }
    // Timer NFC cada 1 segundo
    nfc_timer = lv_timer_create(nfc_check_timer_cb, NFC_CHECK_INTERVAL_MS, NULL);

    Serial.println("\n========================================");
    Serial.printf("Sistema listo! Creds: %d | NFC: %s\n",
                  credentials_get_count(),
                  nfc_enabled ? "habilitado" : "deshabilitado");
    Serial.println("ThingsBoard shared attrs: lockState, credenciales,");
    Serial.println("  autoLockDelay, nfcEnabled, authorizedNfcUid,");
    Serial.println("  maxFailedAttempts, lockoutDuration");
    Serial.println("ThingsBoard RPC: unlockTemporary, resetLockout");
    Serial.println("========================================\n");
}

// ============================================
// LOOP
// ============================================

void loop()
{
    // ── LVGL (máxima prioridad) ─────────────────────────────
    uint32_t next = lv_timer_handler();

    // ── Portal de configuración ──────────────────────────────
    if (portal_is_active()) {
        portal_loop();
        yield();
        return;  // no ejecutar lógica normal mientras el portal está activo
    }

    // ── ThingsBoard ──────────────────────────────────────────
    static uint32_t last_tb = 0;
    if (WiFi.status() == WL_CONNECTED && millis() - last_tb > 50) {
        thingsboard_loop();
        last_tb = millis();
    }

    // ── Actualizar indicadores de estado en standby ──────────
    static uint32_t last_status = 0;
    if (millis() - last_status > 2000) {
        bool wifiOk = (WiFi.status() == WL_CONNECTED);
        bool tbOk   = thingsboard_is_connected();
        standby_screen_update_status(wifiOk, tbOk);
        last_status = millis();
    }

    // ── Watchdog de memoria ──────────────────────────────────
    static uint32_t last_mem = 0;
    if (millis() - last_mem > 10000) {
        uint32_t free = ESP.getFreeHeap();
        Serial.printf("[Heap] Libre: %u bytes\n", free);
        if (free < 8000) {
            Serial.println("WARN: memoria baja — reseteando flags");
            screen_busy  = false;
            is_unlocking = false;
        }
        last_mem = millis();
    }

    // ── Delay adaptativo ────────────────────────────────────
    if (next > 5) delay(2);
    yield();
}
