/**
 * @file standby_screen.cpp
 * @brief Pantalla de standby — estética premium dark navy
 *
 * Diseño:
 *   · Fondo degradado profundo navy (#050d1a → #0a1628)
 *   · Hora ultra-thin centrada (fuente Montserrat 48 light)
 *   · Divisor con rombo central azul acento
 *   · Fecha en mayúsculas espaciadas
 *   · Esquinas decorativas tipo HUD
 *   · Puntos de estado y label WiFi arriba
 *   · Texto "toque para desbloquear" minimalista abajo
 *   · Parpadeo suave del separador de hora
 */

#include "standby_screen.h"
#include "../theme.h"
#include "../../config/config.h"
#include "../../hardware/display.h"
#include <time.h>

// ────────────────────────────────────────────────
//  Handles estáticos
// ────────────────────────────────────────────────
static lv_obj_t* standby_screen  = nullptr;
static lv_obj_t* label_hours     = nullptr;
static lv_obj_t* label_colon     = nullptr;
static lv_obj_t* label_mins      = nullptr;
static lv_obj_t* label_date      = nullptr;

// Indicadores de estado de conectividad
static lv_obj_t* dot_wifi        = nullptr;  // punto WiFi
static lv_obj_t* dot_tb          = nullptr;  // punto ThingsBoard
static lv_obj_t* lbl_wifi_status = nullptr;  // texto "WIFI" / "TB"

static void (*tap_callback)(void)    = nullptr;
static void (*portal_callback)(void) = nullptr;
static void (*info_callback)(void)   = nullptr;
static lv_timer_t* clock_timer      = nullptr;
static lv_timer_t* colon_timer      = nullptr;
static bool colon_visible           = true;

// Gesto oculto portal: 5 toques en esquina superior derecha en 6s
#define CORNER_X_MIN      700
#define CORNER_Y_MAX      100
#define CORNER_TAPS_REQ     5
#define CORNER_TIMEOUT_MS 6000

static uint8_t  corner_tap_count = 0;
static uint32_t last_corner_tap  = 0;

// Long press para pantalla de info
#define INFO_LONG_PRESS_MS 2000

static uint32_t  press_start_ms = 0;
static lv_coord_t press_x       = 0;
static lv_coord_t press_y       = 0;

// ────────────────────────────────────────────────
//  Paleta
// ────────────────────────────────────────────────
#define C_BG_DEEP       0x050d1a   // Fondo oscuro principal
#define C_BG_MID        0x0a1628   // Fondo degradado secundario
#define C_TEXT_PRIMARY  0xf0f6ff   // Blanco frío para la hora
#define C_ACCENT        0x3b82f6   // Azul acento
#define C_ACCENT_DIM    0x1e40af   // Azul oscuro para líneas
#define C_TEXT_DATE     0x4a5568   // Gris medio para la fecha
#define C_HINT          0x2d3748   // Gris oscuro para el hint
#define C_CORNER        0x1e3a5f   // Azul apagado para esquinas HUD
#define C_STATUS_ON     0x3b82f6   // Punto de estado activo
#define C_STATUS_OFF    0x1e2d40   // Punto de estado inactivo


// ────────────────────────────────────────────────
//  Helpers
// ────────────────────────────────────────────────

/**
 * @brief Dibuja un rectángulo 1px para líneas decorativas
 */
static lv_obj_t* create_line(lv_obj_t* parent,
                              lv_coord_t x, lv_coord_t y,
                              lv_coord_t w, lv_coord_t h,
                              uint32_t color, lv_opa_t opa = LV_OPA_COVER)
{
    lv_obj_t* obj = lv_obj_create(parent);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_radius(obj, (h <= 2 ? 1 : 0), 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    return obj;
}

/**
 * @brief Actualiza hora y fecha
 */
static void update_clock(void)
{
    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    static char hh[4], mm[4], date_str[80];

    strftime(hh, sizeof(hh), "%H", t);
    strftime(mm, sizeof(mm), "%M", t);

    if (label_hours && label_colon) {
        lv_label_set_text(label_hours, hh);
        // Re-alinear: el texto cambió de ancho, hay que recalcular posición
        lv_obj_align_to(label_hours, label_colon, LV_ALIGN_OUT_LEFT_MID, -4, 0);
    }
    if (label_mins && label_colon) {
        lv_label_set_text(label_mins, mm);
        lv_obj_align_to(label_mins, label_colon, LV_ALIGN_OUT_RIGHT_MID, 4, 0);
    }

    // Fecha en español: "DOMINGO, 22 DE FEBRERO"
    // Usamos strftime con locale — si el sistema está en es_ES devolverá español.
    // Fallback: los nombres estarán en inglés si el locale no está configurado.
    strftime(date_str, sizeof(date_str), "%A, %d DE %B", t);
    for (int i = 0; date_str[i]; i++) {
        if (date_str[i] >= 'a' && date_str[i] <= 'z')
            date_str[i] -= 32;
    }
    if (label_date) lv_label_set_text(label_date, date_str);
}

// ────────────────────────────────────────────────
//  Timer callbacks
// ────────────────────────────────────────────────
static void clock_timer_cb(lv_timer_t*) { update_clock(); }

static void colon_blink_cb(lv_timer_t*)
{
    if (!label_colon) return;
    colon_visible = !colon_visible;
    lv_obj_set_style_text_opa(label_colon,
        colon_visible ? LV_OPA_90 : LV_OPA_20, 0);
}

// ────────────────────────────────────────────────
//  Touch callback — gesto oculto + long press info
// ────────────────────────────────────────────────
static void screen_touch_cb(lv_event_t* e)
{
    lv_event_code_t code = lv_event_get_code(e);

    lv_indev_t* indev = lv_indev_get_act();
    lv_point_t  point = {0, 0};
    if (indev) lv_indev_get_point(indev, &point);

    // ── Registrar inicio de pulsación ─────────────────────
    if (code == LV_EVENT_PRESSED) {
        press_start_ms = millis();
        press_x = point.x;
        press_y = point.y;
        return;
    }

    if (code != LV_EVENT_RELEASED) return;

    uint32_t held_ms = millis() - press_start_ms;

    // ── Esquina superior derecha → gesto portal ───────────
    if (press_x > CORNER_X_MIN && press_y < CORNER_Y_MAX) {
        uint32_t now = millis();
        if (now - last_corner_tap > CORNER_TIMEOUT_MS) corner_tap_count = 0;
        last_corner_tap = now;
        if (++corner_tap_count >= CORNER_TAPS_REQ) {
            corner_tap_count = 0;
            if (portal_callback) portal_callback();
        }
        return;
    }

    corner_tap_count = 0;

    // ── Long press (~2s) → pantalla de info ───────────────
    if (held_ms >= INFO_LONG_PRESS_MS) {
        if (info_callback) info_callback();
        return;
    }

    // ── Toque normal → pinpad ─────────────────────────────
    if (tap_callback) tap_callback();
}

// ────────────────────────────────────────────────
//  API pública
// ────────────────────────────────────────────────

/**
 * @brief Crea la pantalla de standby con diseño premium
 */
lv_obj_t* standby_screen_create(void)
{
    // ── Fondo ──────────────────────────────────────
    // OPTIMIZACIÓN: Sin degradado para reducir carga de CPU/GPU
    standby_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(standby_screen, lv_color_hex(C_BG_DEEP), 0);
    lv_obj_set_style_bg_opa(standby_screen, LV_OPA_COVER, 0);
    lv_obj_clear_flag(standby_screen, LV_OBJ_FLAG_SCROLLABLE);

    // ── Esquinas HUD (20×20 px, L-shape simulada con 2 líneas) ──
    // Top-left
    create_line(standby_screen,  16,  16, 18,  1, C_CORNER, LV_OPA_60);
    create_line(standby_screen,  16,  16,  1, 18, C_CORNER, LV_OPA_60);
    // Top-right
    create_line(standby_screen, 766,  16, 18,  1, C_CORNER, LV_OPA_60);
    create_line(standby_screen, 783,  16,  1, 18, C_CORNER, LV_OPA_60);
    // Bottom-left
    create_line(standby_screen,  16, 463, 18,  1, C_CORNER, LV_OPA_60);
    create_line(standby_screen,  16, 446,  1, 18, C_CORNER, LV_OPA_60);
    // Bottom-right
    create_line(standby_screen, 766, 463, 18,  1, C_CORNER, LV_OPA_60);
    create_line(standby_screen, 783, 446,  1, 18, C_CORNER, LV_OPA_60);

    // ── Barra superior: indicadores de conectividad ──────────
    // Dot WiFi
    dot_wifi = lv_obj_create(standby_screen);
    lv_obj_set_size(dot_wifi, 8, 8);
    lv_obj_set_pos(dot_wifi, 36, 23);
    lv_obj_set_style_bg_color(dot_wifi, lv_color_hex(C_STATUS_OFF), 0);
    lv_obj_set_style_radius(dot_wifi, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot_wifi, 0, 0);
    lv_obj_clear_flag(dot_wifi, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Label "WiFi"
    lv_obj_t* lbl_w = lv_label_create(standby_screen);
    lv_label_set_text(lbl_w, "WiFi");
    lv_obj_set_style_text_font(lbl_w, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_w, lv_color_hex(C_HINT), 0);
    lv_obj_set_pos(lbl_w, 50, 21);

    // Dot ThingsBoard
    dot_tb = lv_obj_create(standby_screen);
    lv_obj_set_size(dot_tb, 8, 8);
    lv_obj_set_pos(dot_tb, 100, 23);
    lv_obj_set_style_bg_color(dot_tb, lv_color_hex(C_STATUS_OFF), 0);
    lv_obj_set_style_radius(dot_tb, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot_tb, 0, 0);
    lv_obj_clear_flag(dot_tb, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // Label "TB"
    lv_obj_t* lbl_tb = lv_label_create(standby_screen);
    lv_label_set_text(lbl_tb, "TB");
    lv_obj_set_style_text_font(lbl_tb, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_tb, lv_color_hex(C_HINT), 0);
    lv_obj_set_pos(lbl_tb, 114, 21);

    // Label versión firmware
    lbl_wifi_status = lv_label_create(standby_screen);
    lv_label_set_text(lbl_wifi_status, "v" FIRMWARE_VERSION);
    lv_obj_set_style_text_font(lbl_wifi_status, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_wifi_status, lv_color_hex(C_HINT), 0);
    lv_obj_set_pos(lbl_wifi_status, 706, 21);

    // ── HORA — encadenado con lv_obj_align_to ────────────
    //
    // El colon es el ANCLA central (LV_ALIGN_CENTER).
    // HH se pega a su izquierda con OUT_LEFT_MID.
    // MM se pega a su derecha con OUT_RIGHT_MID.
    // Así el grupo "HH:MM" siempre queda perfectamente centrado.

    // 1) Separador ":" — ancla central
    label_colon = lv_label_create(standby_screen);
    lv_label_set_text(label_colon, ":");
    lv_obj_set_style_text_font(label_colon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_colon, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_opa(label_colon, LV_OPA_90, 0);
    lv_obj_align(label_colon, LV_ALIGN_CENTER, 0, -28);

    // 2) HH — pegado a la izquierda del colon
    label_hours = lv_label_create(standby_screen);
    lv_label_set_text(label_hours, "--");
    lv_obj_set_style_text_font(label_hours, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_hours, lv_color_hex(C_TEXT_PRIMARY), 0);
    lv_obj_align_to(label_hours, label_colon, LV_ALIGN_OUT_LEFT_MID, -4, 0);

    // 3) MM — pegado a la derecha del colon
    label_mins = lv_label_create(standby_screen);
    lv_label_set_text(label_mins, "--");
    lv_obj_set_style_text_font(label_mins, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_mins, lv_color_hex(C_TEXT_PRIMARY), 0);
    lv_obj_align_to(label_mins, label_colon, LV_ALIGN_OUT_RIGHT_MID, 4, 0);

    // ── Divisor decorativo: línea — rombo — línea ──────
    // Centrado en X=400, a +50px del centro vertical (debajo del grupo hora)
    lv_obj_t* div_left = create_line(standby_screen, 0, 0, 130, 1, C_ACCENT_DIM, LV_OPA_60);
    lv_obj_align(div_left, LV_ALIGN_CENTER, -75, 26);

    lv_obj_t* div_right = create_line(standby_screen, 0, 0, 130, 1, C_ACCENT_DIM, LV_OPA_60);
    lv_obj_align(div_right, LV_ALIGN_CENTER, 75, 26);

    // Rombo central
    // OPTIMIZACIÓN: Sin sombra para reducir carga de renderizado
    lv_obj_t* diamond = lv_obj_create(standby_screen);
    lv_obj_set_size(diamond, 7, 7);
    lv_obj_align(diamond, LV_ALIGN_CENTER, 0, 23);
    lv_obj_set_style_bg_color(diamond, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_radius(diamond, 1, 0);
    lv_obj_set_style_border_width(diamond, 0, 0);
    lv_obj_set_style_transform_angle(diamond, 450, 0);
    lv_obj_clear_flag(diamond, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);

    // ── Fecha ───────────────────────────────────────────
    label_date = lv_label_create(standby_screen);
    lv_label_set_text(label_date, "CARGANDO...");
    lv_obj_set_style_text_font(label_date, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_date, lv_color_hex(C_TEXT_DATE), 0);
    lv_obj_set_style_text_letter_space(label_date, 4, 0);
    lv_obj_align(label_date, LV_ALIGN_CENTER, 0, 52);
    lv_obj_set_width(label_date, 800);
    lv_obj_set_style_text_align(label_date, LV_TEXT_ALIGN_CENTER, 0);

    // ── Línea inferior sutil separadora ─────────────────
    lv_obj_t* bottom_line = create_line(standby_screen, 0, 0, 120, 1, C_HINT, LV_OPA_50);
    lv_obj_align(bottom_line, LV_ALIGN_BOTTOM_MID, 0, -52);

    // ── Hint bottom ─────────────────────────────────────
    lv_obj_t* label_hint = lv_label_create(standby_screen);
    lv_label_set_text(label_hint, "TOQUE PARA DESBLOQUEAR");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(C_HINT), 0);
    lv_obj_set_style_text_letter_space(label_hint, 3, 0);
    lv_obj_align(label_hint, LV_ALIGN_BOTTOM_MID, 0, -26);
    lv_obj_set_width(label_hint, 800);
    lv_obj_set_style_text_align(label_hint, LV_TEXT_ALIGN_CENTER, 0);

    // ── Área de toque (transparente, todo el fondo) ──────
    lv_obj_t* touch_area = lv_obj_create(standby_screen);
    lv_obj_set_size(touch_area, 800, 480);
    lv_obj_set_pos(touch_area, 0, 0);
    lv_obj_set_style_bg_opa(touch_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(touch_area, 0, 0);
    lv_obj_clear_flag(touch_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(touch_area, screen_touch_cb, LV_EVENT_PRESSED,  NULL);
    lv_obj_add_event_cb(touch_area, screen_touch_cb, LV_EVENT_RELEASED, NULL);

    // ── Inicializar y arrancar timers ────────────────────
    update_clock();
    clock_timer = lv_timer_create(clock_timer_cb,   60000, NULL);  // cada minuto
    colon_timer = lv_timer_create(colon_blink_cb,    1000, NULL);  // blink cada 1s (reducido de 500ms)

    return standby_screen;
}

/**
 * @brief Muestra la pantalla de standby con animación
 */
void standby_screen_show(lv_obj_t* from_screen,
                          lv_scr_load_anim_t anim_type,
                          uint32_t duration)
{
    display_turn_on();
    lv_scr_load_anim(standby_screen, anim_type, duration, 0, false);
}

/**
 * @brief Fuerza actualización del reloj
 */
void standby_screen_update_time(void)
{
    update_clock();
}

/**
 * @brief Registra callback de toque normal
 */
void standby_screen_set_tap_callback(void (*callback)(void))
{
    tap_callback = callback;
}

/**
 * @brief Registra callback del gesto oculto (portal de configuración)
 */
void standby_screen_set_portal_callback(void (*callback)(void))
{
    portal_callback = callback;
}

void standby_screen_set_info_callback(void (*callback)(void))
{
    info_callback = callback;
}

/**
 * @brief Actualiza los puntos indicadores de WiFi y ThingsBoard
 */
void standby_screen_update_status(bool wifiOk, bool tbOk)
{
    if (dot_wifi) {
        lv_obj_set_style_bg_color(dot_wifi,
            lv_color_hex(wifiOk ? C_STATUS_ON : C_STATUS_OFF), 0);
    }
    if (dot_tb) {
        lv_obj_set_style_bg_color(dot_tb,
            lv_color_hex(tbOk ? C_STATUS_ON : C_STATUS_OFF), 0);
    }
}