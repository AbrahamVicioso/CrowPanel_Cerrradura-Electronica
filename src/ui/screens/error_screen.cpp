/**
 * @file error_screen.cpp
 * @brief Implementación de la pantalla de error/intento fallido
 */

#include "error_screen.h"
#include "../theme.h"
#include "../../hardware/display.h"

static lv_obj_t* screen_error = nullptr;
static void (*callback)(void) = nullptr;
static lv_timer_t* error_return_timer = nullptr;

/**
 * @brief Crea la pantalla de error
 */
lv_obj_t* error_screen_create(void)
{
    screen_error = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_error, COLOR_PRIMARY, 0);

    // ==================== ERROR ICON ====================
    
    // Círculo de error (sin sombra para mejor rendimiento)
    lv_obj_t *circle_bg = lv_obj_create(screen_error);
    lv_obj_set_size(circle_bg, 140, 140);
    lv_obj_set_pos(circle_bg, 330, 100);
    lv_obj_set_style_bg_color(circle_bg, lv_color_hex(0x2d1515), 0);
    lv_obj_set_style_border_width(circle_bg, 3, 0);
    lv_obj_set_style_border_color(circle_bg, COLOR_ERROR, 0);
    lv_obj_set_style_radius(circle_bg, 70, 0);
    // Shadow removido - era costoso en CPU

    // X icon
    lv_obj_t *error_icon = lv_label_create(circle_bg);
    lv_label_set_text(error_icon, "X");
    lv_obj_set_style_text_font(error_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(error_icon, COLOR_ERROR, 0);
    lv_obj_center(error_icon);

    // ==================== MESSAGE ====================
    
    // Título de error
    lv_obj_t *label_title = lv_label_create(screen_error);
    lv_label_set_text(label_title, "ACCESO DENEGADO");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, COLOR_ERROR, 0);
    lv_obj_set_style_text_letter_space(label_title, 2, 0);
    lv_obj_set_pos(label_title, 0, 260);
    lv_obj_set_width(label_title, 800);

    // Mensaje de error
    lv_obj_t *label_message = lv_label_create(screen_error);
    lv_label_set_text(label_message, "Credenciales invalidas");
    lv_obj_set_style_text_font(label_message, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_message, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(label_message, 0, 300);
    lv_obj_set_width(label_message, 800);

    return screen_error;
}

static void auto_return_callback(lv_timer_t *timer)
{
    lv_timer_del(timer);
    error_return_timer = nullptr;  // Resetear puntero

    if (callback != nullptr) {
        void (*cb)(void) = callback;  // Copiar callback
        callback = nullptr;            // Resetear para evitar doble ejecución
        cb();                          // Ejecutar callback
    }
}

void error_screen_show(void (*on_callback)(void), uint32_t delay_ms)
{
    callback = on_callback;
    display_turn_on();  // Asegurar que el backlight esté encendido
    
    // Transición con efecto
    lv_scr_load_anim(screen_error, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    
    // Volver después del delay - guardar timer para evitar memory leak
    if (delay_ms > 0) {
        if (error_return_timer != nullptr) {
            lv_timer_del(error_return_timer);
        }
        error_return_timer = lv_timer_create(auto_return_callback, delay_ms, NULL);
        lv_timer_set_repeat_count(error_return_timer, 1);
    }
}
