/**
 * @file unlocked_screen.cpp
 * @brief Implementación de la pantalla de desbloqueo exitoso
 */

#include "unlocked_screen.h"
#include "../theme.h"
#include "../../config/config.h"
#include "../../hardware/display.h"

static lv_obj_t* screen_unlocked = nullptr;
static void (*lock_callback)(void) = nullptr;
static lv_timer_t* auto_lock_timer = nullptr;

lv_obj_t* unlocked_screen_create(void)
{
    screen_unlocked = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_unlocked, COLOR_PRIMARY, 0);
    lv_obj_clear_flag(screen_unlocked, LV_OBJ_FLAG_SCROLLABLE);

    // Círculo éxito — centrado en 1/3 superior
    lv_obj_t* circle_bg = lv_obj_create(screen_unlocked);
    lv_obj_set_size(circle_bg, 150, 150);
    lv_obj_set_style_bg_color(circle_bg, COLOR_SUCCESS, 0);
    lv_obj_set_style_border_width(circle_bg, 0, 0);
    lv_obj_set_style_radius(circle_bg, 75, 0);
    lv_obj_clear_flag(circle_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(circle_bg, LV_ALIGN_CENTER, 0, -80);

    lv_obj_t* icon_check = lv_label_create(circle_bg);
    lv_label_set_text(icon_check, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon_check, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon_check, lv_color_white(), 0);
    lv_obj_center(icon_check);

    // Texto principal
    lv_obj_t* label_success = lv_label_create(screen_unlocked);
    lv_label_set_text(label_success, "ACCESO CONCEDIDO");
    lv_obj_set_style_text_font(label_success, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_success, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_letter_space(label_success, 2, 0);
    lv_obj_set_style_text_align(label_success, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_success, 700);
    lv_obj_align(label_success, LV_ALIGN_CENTER, 0, 30);

    lv_obj_t* label_welcome = lv_label_create(screen_unlocked);
    lv_label_set_text(label_welcome, "Bienvenido");
    lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_welcome, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(label_welcome, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_welcome, 700);
    lv_obj_align(label_welcome, LV_ALIGN_CENTER, 0, 68);

    // Barra inferior
    lv_obj_t* status_bar = lv_obj_create(screen_unlocked);
    lv_obj_set_size(status_bar, 800, 44);
    lv_obj_set_style_bg_color(status_bar, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);
    lv_obj_set_style_radius(status_bar, 0, 0);
    lv_obj_clear_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(status_bar, LV_ALIGN_BOTTOM_MID, 0, 0);

    lv_obj_t* label_status = lv_label_create(status_bar);
    lv_label_set_text(label_status, "Puerta desbloqueada");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(label_status, LV_ALIGN_CENTER, 0, 0);

    return screen_unlocked;
}

static void auto_lock_timer_callback(lv_timer_t *timer)
{
    lv_timer_del(timer);
    auto_lock_timer = nullptr;  // CRITICAL: Resetear puntero después de eliminar

    if (lock_callback != nullptr) {
        void (*cb)(void) = lock_callback;  // Copiar callback
        lock_callback = nullptr;            // Resetear callback para evitar doble ejecución
        cb();                              // Ejecutar callback
    }
}

void unlocked_screen_show(uint32_t auto_lock_delay_ms, void (*on_lock_callback)(void))
{
    lock_callback = on_lock_callback;
    display_turn_on();  // Asegurar que el backlight esté encendido
    
    // Sin animación para mejor rendimiento
    lv_scr_load_anim(screen_unlocked, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    
    // Configurar timer de auto-bloqueo
    if (auto_lock_delay_ms > 0) {
        // Eliminar timer anterior si existe
        if (auto_lock_timer != nullptr) {
            lv_timer_del(auto_lock_timer);
        }
        auto_lock_timer = lv_timer_create(
            auto_lock_timer_callback, 
            auto_lock_delay_ms, 
            NULL
        );
        lv_timer_set_repeat_count(auto_lock_timer, 1);
    }
}
