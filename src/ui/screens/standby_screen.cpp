/**
 * @file standby_screen.cpp
 * @brief Implementación de la pantalla de standby con reloj
 */

#include "standby_screen.h"
#include "../theme.h"
#include "../../hardware/display.h"
#include <time.h>

// Objeto de pantalla
static lv_obj_t* standby_screen = nullptr;
static lv_obj_t* label_time = nullptr;
static lv_obj_t* label_date = nullptr;
static lv_obj_t* label_hint = nullptr;
static lv_obj_t* btn_area = nullptr;

// Callback para cuando se toca la pantalla
static void (*tap_callback)(void) = nullptr;

// Timer para actualizar el reloj
static lv_timer_t* clock_timer = nullptr;

/**
 * @brief Actualiza el tiempo mostrado en pantalla
 */
static void update_clock(void)
{
    time_t now = time(nullptr);
    struct tm* timeinfo = localtime(&now);
    
    // Formato hora: HH:MM
    static char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    
    if (label_time != nullptr) {
        lv_label_set_text(label_time, time_str);
    }
    
    // Formato fecha: Día, DD de Mes
    static char date_str[64];
    strftime(date_str, sizeof(date_str), "%A, %d de %B", timeinfo);
    
    // Capitalizar primera letra
    if (date_str[0] >= 'a' && date_str[0] <= 'z') {
        date_str[0] = date_str[0] - 32;
    }
    
    if (label_date != nullptr) {
        lv_label_set_text(label_date, date_str);
    }
}

/**
 * @brief Callback del timer de reloj
 */
static void clock_timer_callback(lv_timer_t* timer)
{
    update_clock();
}

/**
 * @brief Callback cuando se toca el área de la pantalla
 */
static void screen_touch_event(lv_event_t* e)
{
    if (e->code == LV_EVENT_CLICKED || e->code == LV_EVENT_PRESSED) {
        if (tap_callback != nullptr) {
            tap_callback();
        }
    }
}

/**
 * @brief Muestra la pantalla de standby con animación
 */
void standby_screen_show(lv_obj_t* from_screen, lv_scr_load_anim_t anim_type, uint32_t duration)
{
    display_turn_on();  // Asegurar que el backlight estéendido
    lv_scr_load_anim(standby_screen, anim_type, duration, 0, false);
}

/**
 * @brief Crea la pantalla de standby
 */
lv_obj_t* standby_screen_create(void)
{
    standby_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(standby_screen, lv_color_hex(0x0f172a), 0);  // Dark blue-gray
    lv_obj_set_style_bg_grad_color(standby_screen, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_bg_grad_dir(standby_screen, LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(standby_screen, LV_OPA_COVER, 0);

    // ==================== TIME DISPLAY ====================
    
    // Hora grande
    label_time = lv_label_create(standby_screen);
    lv_label_set_text(label_time, "--:--");
    lv_obj_set_style_text_font(label_time, &lv_font_montserrat_42, 0);
    lv_obj_set_style_text_color(label_time, lv_color_hex(0xf8fafc), 0);  // White
    lv_obj_set_pos(label_time, 0, 140);
    lv_obj_set_width(label_time, 800);
    
    // ==================== DATE DISPLAY ====================
    
    // Fecha
    label_date = lv_label_create(standby_screen);
    lv_label_set_text(label_date, "Cargando fecha...");
    lv_obj_set_style_text_font(label_date, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_date, lv_color_hex(0x94a3b8), 0);  // Gray
    lv_obj_set_pos(label_date, 0, 230);
    lv_obj_set_width(label_date, 800);
    
    // ==================== DECORATIVE ELEMENTS ====================
    
    // Línea decorativa debajo de la hora
    lv_obj_t* line = lv_obj_create(standby_screen);
    lv_obj_set_size(line, 100, 2);
    lv_obj_set_pos(line, 350, 215);
    lv_obj_set_style_bg_color(line, lv_color_hex(0x3b82f6), 0);  // Blue
    lv_obj_set_style_radius(line, 1, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_80, 0);
    
    // ==================== HINT AREA ====================
    
    // Icono de candado pequeño
    lv_obj_t* lock_icon = lv_label_create(standby_screen);
    lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lock_icon, lv_color_hex(0x64748b), 0);
    lv_obj_set_pos(lock_icon, 320, 380);
    
    // Texto de sugerencia
    label_hint = lv_label_create(standby_screen);
    lv_label_set_text(label_hint, "Toque para desbloquear");
    lv_obj_set_style_text_font(label_hint, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_hint, lv_color_hex(0x64748b), 0);
    lv_obj_set_pos(label_hint, 0, 382);
    lv_obj_set_width(label_hint, 800);
    
    // ==================== TOUCH AREA ====================
    
    // Área transparente para detectar toque (solo clic)
    btn_area = lv_obj_create(standby_screen);
    lv_obj_set_size(btn_area, 800, 480);
    lv_obj_set_pos(btn_area, 0, 0);
    lv_obj_set_style_bg_opa(btn_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn_area, 0, 0);
    lv_obj_set_style_outline_width(btn_area, 0, 0);
    lv_obj_clear_flag(btn_area, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(btn_area, screen_touch_event, LV_EVENT_CLICKED, NULL);
    
    // Inicializar tiempo inmediatamente
    update_clock();
    
    // Crear timer para actualizar cada segundo
    clock_timer = lv_timer_create(clock_timer_callback, 1000, NULL);
    
    return standby_screen;
}

/**
 * @brief Actualiza el reloj de la pantalla standby
 */
void standby_screen_update_time(void)
{
    update_clock();
}

/**
 * @brief Establece el callback para cuando se toca la pantalla
 */
void standby_screen_set_tap_callback(void (*callback)(void))
{
    tap_callback = callback;
}
