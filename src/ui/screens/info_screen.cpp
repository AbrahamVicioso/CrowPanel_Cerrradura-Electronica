/**
 * @file info_screen.cpp
 * @brief Pantalla de información del dispositivo
 *
 * Muestra: firmware, MAC, WiFi SSID, IP, señal, uptime,
 *          ThingsBoard, NFC, memoria libre.
 * Se activa con long press (~2s) en standby.
 * Toca para cerrar o se cierra solo a los 30s.
 */

#include "info_screen.h"
#include "../theme.h"
#include "../../config/config.h"
#include <WiFi.h>

#define C_BG     0x050d1a
#define C_CARD   0x0a1628
#define C_BORDER 0x1e3a5f
#define C_ACCENT 0x3b82f6
#define C_TEXT   0xf0f6ff
#define C_DIM    0x4a5568
#define C_ON     0x22c55e   // verde
#define C_OFF    0xef4444   // rojo
#define C_WARN   0xf59e0b   // amarillo

// ── Callback de cierre (almacenado para el timer auto-close) ──
static void (*close_cb)(void) = nullptr;
static lv_timer_t* auto_close_timer = nullptr;
static lv_obj_t*   info_scr         = nullptr;

static void do_close(void)
{
    if (auto_close_timer) {
        lv_timer_del(auto_close_timer);
        auto_close_timer = nullptr;
    }
    if (close_cb) close_cb();
}

static void auto_close_cb(lv_timer_t*) { do_close(); }

static void touch_close_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) do_close();
}

// ── Helper: añade una fila label + valor ─────────────────────
static void add_row(lv_obj_t* parent,
                    const char* key, const char* val,
                    lv_coord_t x, lv_coord_t y,
                    uint32_t val_color = 0xf0f6ff)
{
    lv_obj_t* lk = lv_label_create(parent);
    lv_label_set_text(lk, key);
    lv_obj_set_style_text_font(lk, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lk, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_letter_space(lk, 1, 0);
    lv_obj_set_pos(lk, x, y);

    lv_obj_t* lv = lv_label_create(parent);
    lv_label_set_text(lv, val);
    lv_obj_set_style_text_font(lv, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(lv, lv_color_hex(val_color), 0);
    lv_obj_set_pos(lv, x, y + 15);
}

void info_screen_show(bool tbConnected, bool nfcEnabled, void (*on_close)(void))
{
    close_cb = on_close;

    // ── Recoger datos del sistema ────────────────────────
    String fw      = String("v") + FIRMWARE_VERSION;
    String mac     = WiFi.macAddress();
    String ssid    = (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : String("Sin conexion");
    String ip      = (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : String("---");
    int    rssi    = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    uint32_t heap  = ESP.getFreeHeap();

    // Uptime
    uint32_t secs  = millis() / 1000;
    uint32_t hrs   = secs / 3600;
    uint32_t mins  = (secs % 3600) / 60;
    uint32_t s     = secs % 60;
    char uptime_buf[20];
    snprintf(uptime_buf, sizeof(uptime_buf), "%uh %02um %02us", hrs, mins, s);

    // Señal WiFi
    const char* rssi_desc;
    uint32_t    rssi_color;
    if (!WiFi.isConnected()) {
        rssi_desc = "---";
        rssi_color = C_DIM;
    } else if (rssi >= -50) {
        rssi_desc = "Excelente";  rssi_color = C_ON;
    } else if (rssi >= -60) {
        rssi_desc = "Buena";      rssi_color = C_ON;
    } else if (rssi >= -70) {
        rssi_desc = "Regular";    rssi_color = C_WARN;
    } else {
        rssi_desc = "Debil";      rssi_color = C_OFF;
    }
    char rssi_buf[24];
    if (WiFi.isConnected())
        snprintf(rssi_buf, sizeof(rssi_buf), "%s (%d dBm)", rssi_desc, rssi);
    else
        snprintf(rssi_buf, sizeof(rssi_buf), "---");

    // Memoria libre
    char heap_buf[20];
    snprintf(heap_buf, sizeof(heap_buf), "%u bytes", heap);

    // ── Pantalla base ─────────────────────────────────────
    info_scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(info_scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(info_scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(info_scr, LV_OBJ_FLAG_SCROLLABLE);

    // ── Tarjeta ───────────────────────────────────────────
    lv_obj_t* card = lv_obj_create(info_scr);
    lv_obj_set_size(card, 740, 420);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(C_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(card, touch_close_cb, LV_EVENT_CLICKED, NULL);

    // ── Título ────────────────────────────────────────────
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "INFORMACION DEL DISPOSITIVO");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_letter_space(title, 2, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);

    // ── Separador ─────────────────────────────────────────
    lv_obj_t* sep = lv_obj_create(card);
    lv_obj_set_size(sep, 680, 1);
    lv_obj_align(sep, LV_ALIGN_TOP_MID, 0, 48);
    lv_obj_set_style_bg_color(sep, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_bg_opa(sep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(sep, 0, 0);
    lv_obj_clear_flag(sep, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // ── Columna izquierda (x=30) ──────────────────────────
    const lv_coord_t COL_L = 30;
    const lv_coord_t COL_R = 390;
    const lv_coord_t ROW0  = 62;
    const lv_coord_t STEP  = 68;

    add_row(card, "FIRMWARE",     fw.c_str(),      COL_L, ROW0);
    add_row(card, "MAC ADDRESS",  mac.c_str(),     COL_L, ROW0 + STEP);
    add_row(card, "NFC",
            nfcEnabled ? "Habilitado" : "Deshabilitado",
            COL_L, ROW0 + STEP * 2,
            nfcEnabled ? C_ON : C_DIM);
    add_row(card, "UPTIME",       uptime_buf,      COL_L, ROW0 + STEP * 3);

    // ── Separador vertical ────────────────────────────────
    lv_obj_t* vsep = lv_obj_create(card);
    lv_obj_set_size(vsep, 1, 300);
    lv_obj_set_pos(vsep, 360, 58);
    lv_obj_set_style_bg_color(vsep, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_bg_opa(vsep, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(vsep, 0, 0);
    lv_obj_clear_flag(vsep, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // ── Columna derecha (x=390) ───────────────────────────
    add_row(card, "RED WIFI",     ssid.c_str(),    COL_R, ROW0);
    add_row(card, "DIRECCION IP", ip.c_str(),      COL_R, ROW0 + STEP);
    add_row(card, "SENAL WIFI",   rssi_buf,        COL_R, ROW0 + STEP * 2, rssi_color);
    add_row(card, "THINGSBOARD",
            tbConnected ? "Conectado" : "Desconectado",
            COL_R, ROW0 + STEP * 3,
            tbConnected ? C_ON : C_OFF);

    // Memoria libre ocupa la fila siguiente debajo de ambas columnas
    add_row(card, "MEMORIA LIBRE", heap_buf,       COL_L, ROW0 + STEP * 4);

    // ── Hint inferior ─────────────────────────────────────
    lv_obj_t* hint = lv_label_create(card);
    lv_label_set_text(hint, "Toca para cerrar");
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_text_letter_space(hint, 1, 0);
    lv_obj_align(hint, LV_ALIGN_BOTTOM_MID, 0, -14);

    // ── Auto-cierre a los 30 segundos ─────────────────────
    auto_close_timer = lv_timer_create(auto_close_cb, 30000, NULL);
    lv_timer_set_repeat_count(auto_close_timer, 1);

    lv_scr_load_anim(info_scr, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
}
