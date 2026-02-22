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

lv_obj_t* unlocked_screen_create(void)
{
    screen_unlocked = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_unlocked, COLOR_PRIMARY, 0);

    // ==================== SUCCESS ICON ====================
    
    // Círculo grande de éxito
    lv_obj_t *circle_bg = lv_obj_create(screen_unlocked);
    lv_obj_set_size(circle_bg, 150, 150);
    lv_obj_set_pos(circle_bg, 325, 80);
    lv_obj_set_style_bg_color(circle_bg, COLOR_SUCCESS, 0);
    lv_obj_set_style_border_width(circle_bg, 0, 0);
    lv_obj_set_style_radius(circle_bg, 75, 0);
    lv_obj_set_style_shadow_width(circle_bg, 30, 0);
    lv_obj_set_style_shadow_opa(circle_bg, LV_OPA_50, 0);

    // Checkmark icon
    lv_obj_t *icon_check = lv_label_create(circle_bg);
    lv_label_set_text(icon_check, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon_check, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon_check, lv_color_white(), 0);
    lv_obj_center(icon_check);

    // ==================== MESSAGE ====================
    
    // Texto de éxito principal
    lv_obj_t *label_success = lv_label_create(screen_unlocked);
    lv_label_set_text(label_success, "ACCESO CONCEDIDO");
    lv_obj_set_style_text_font(label_success, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_success, COLOR_SUCCESS, 0);
    lv_obj_set_style_text_letter_space(label_success, 2, 0);
    lv_obj_set_pos(label_success, 0, 250);
    lv_obj_set_width(label_success, 800);

    // Mensaje de bienvenida
    lv_obj_t *label_welcome = lv_label_create(screen_unlocked);
    lv_label_set_text(label_welcome, "Bienvenido");
    lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_welcome, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(label_welcome, 0, 290);
    lv_obj_set_width(label_welcome, 800);

    // ==================== STATUS BAR ====================
    
    lv_obj_t *status_bar = lv_obj_create(screen_unlocked);
    lv_obj_set_size(status_bar, 800, 50);
    lv_obj_set_pos(status_bar, 0, 430);
    lv_obj_set_style_bg_color(status_bar, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(status_bar, 0, 0);

    // Estado
    lv_obj_t *label_status = lv_label_create(status_bar);
    lv_label_set_text(label_status, "Puerta desbloqueada");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(label_status, 20, 15);

    return screen_unlocked;
}

static void auto_lock_timer_callback(lv_timer_t *timer)
{
    lv_timer_del(timer);
    
    if (lock_callback != nullptr) {
        lock_callback();
    }
}

void unlocked_screen_show(uint32_t auto_lock_delay_ms, void (*on_lock_callback)(void))
{
    lock_callback = on_lock_callback;
    display_turn_on();  // Asegurar que el backlight esté encendido
    
    // Usar animación de fade para evitar problemas
    lv_scr_load_anim(screen_unlocked, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
    
    // Configurar timer de auto-bloqueo
    if (auto_lock_delay_ms > 0) {
        lv_timer_t *lock_timer = lv_timer_create(
            auto_lock_timer_callback, 
            auto_lock_delay_ms, 
            NULL
        );
        lv_timer_set_repeat_count(lock_timer, 1);
    }
}
