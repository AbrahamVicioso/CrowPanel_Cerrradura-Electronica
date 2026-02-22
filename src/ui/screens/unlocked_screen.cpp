/**
 * @file unlocked_screen.cpp
 * @brief Implementación de la pantalla de desbloqueo exitoso
 */

#include "unlocked_screen.h"
#include "../theme.h"
#include "../../config/config.h"

static lv_obj_t* screen_unlocked = nullptr;
static void (*lock_callback)(void) = nullptr;

lv_obj_t* unlocked_screen_create(void)
{
    screen_unlocked = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_unlocked, COLOR_PRIMARY, 0);

    // Panel verde de éxito
    lv_obj_t *success_panel = lv_obj_create(screen_unlocked);
    lv_obj_set_size(success_panel, 600, 350);
    lv_obj_align(success_panel, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(success_panel, COLOR_SUCCESS, 0);
    lv_obj_set_style_border_width(success_panel, 0, 0);
    lv_obj_set_style_radius(success_panel, 20, 0);
    lv_obj_set_style_shadow_width(success_panel, 30, 0);
    lv_obj_set_style_shadow_opa(success_panel, LV_OPA_50, LV_PART_MAIN);

    // Icono de check grande
    lv_obj_t *icon_check = lv_label_create(success_panel);
    lv_label_set_text(icon_check, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(icon_check, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(icon_check, lv_color_white(), 0);
    lv_obj_align(icon_check, LV_ALIGN_TOP_MID, 0, 40);

    // Círculo alrededor del check
    lv_obj_t *circle = lv_obj_create(success_panel);
    lv_obj_set_size(circle, 120, 120);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x00d2a4), 0);
    lv_obj_set_style_border_width(circle, 4, 0);
    lv_obj_set_style_border_color(circle, lv_color_white(), 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_move_background(circle);

    // Texto de éxito
    lv_obj_t *label_success = lv_label_create(success_panel);
    lv_label_set_text(label_success, "ACCESO CONCEDIDO");
    lv_obj_set_style_text_font(label_success, &lv_font_montserrat_40, 0);
    lv_obj_set_style_text_color(label_success, lv_color_white(), 0);
    lv_obj_align(label_success, LV_ALIGN_CENTER, 0, 20);

    // Mensaje secundario
    lv_obj_t *label_welcome = lv_label_create(success_panel);
    lv_label_set_text(label_welcome, "Bienvenido");
    lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label_welcome, lv_color_white(), 0);
    lv_obj_align(label_welcome, LV_ALIGN_BOTTOM_MID, 0, -30);

    // Icono de candado abierto
    lv_obj_t *unlock_icon = lv_label_create(success_panel);
    lv_label_set_text(unlock_icon, LV_SYMBOL_UPLOAD);
    lv_obj_set_style_text_font(unlock_icon, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(unlock_icon, lv_color_white(), 0);
    lv_obj_align(unlock_icon, LV_ALIGN_BOTTOM_MID, 0, -80);

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
    
    // Mostrar pantalla con animación
    lv_scr_load_anim(screen_unlocked, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 0, false);
    
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
