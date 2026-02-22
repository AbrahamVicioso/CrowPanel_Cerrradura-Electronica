/**
 * @file locked_screen.cpp
 * @brief Implementación de la pantalla de bloqueo (puerta cerrada)
 */

#include "locked_screen.h"
#include "../theme.h"

static lv_obj_t* screen_locked = nullptr;
static void (*callback)(void) = nullptr;
static lv_timer_t* auto_return_timer = nullptr;

/**
 * @brief Crea la pantalla de bloqueo
 */
lv_obj_t* locked_screen_create(void)
{
    screen_locked = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_locked, COLOR_PRIMARY, 0);

    // ==================== LOCK ICON ====================
    
    // Círculo de bloqueo
    lv_obj_t *circle_bg = lv_obj_create(screen_locked);
    lv_obj_set_size(circle_bg, 140, 140);
    lv_obj_set_pos(circle_bg, 330, 120);
    lv_obj_set_style_bg_color(circle_bg, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(circle_bg, 3, 0);
    lv_obj_set_style_border_color(circle_bg, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(circle_bg, 70, 0);
    lv_obj_set_style_shadow_width(circle_bg, 20, 0);
    lv_obj_set_style_shadow_opa(circle_bg, LV_OPA_30, 0);

    // Candado cerrado
    lv_obj_t *lock_icon = lv_label_create(circle_bg);
    lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_ACCENT, 0);
    lv_obj_center(lock_icon);

    // ==================== MESSAGE ====================
    
    // Título
    lv_obj_t *label_title = lv_label_create(screen_locked);
    lv_label_set_text(label_title, "SISTEMA BLOQUEADO");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_letter_space(label_title, 2, 0);
    lv_obj_set_pos(label_title, 0, 280);
    lv_obj_set_width(label_title, 800);

    // Subtítulo
    lv_obj_t *label_subtitle = lv_label_create(screen_locked);
    lv_label_set_text(label_subtitle, "Puerta cerrada");
    lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(label_subtitle, 0, 320);
    lv_obj_set_width(label_subtitle, 800);

    // ==================== TIMER INFO ====================
    
    lv_obj_t *label_timer = lv_label_create(screen_locked);
    lv_label_set_text(label_timer, "Volviendo al PIN...");
    lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_timer, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(label_timer, 0, 360);
    lv_obj_set_width(label_timer, 800);

    return screen_locked;
}

static void auto_return_callback(lv_timer_t *timer)
{
    lv_timer_del(timer);
    auto_return_timer = nullptr;
    
    if (callback != nullptr) {
        callback();
    }
}

/**
 * @brief Muestra la pantalla de bloqueo
 */
void locked_screen_show(void (*on_callback)(void))
{
    callback = on_callback;
    lv_scr_load_anim(screen_locked, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
    
    // Auto-retorno después de 3 segundos
    if (auto_return_timer != nullptr) {
        lv_timer_del(auto_return_timer);
    }
    auto_return_timer = lv_timer_create(auto_return_callback, 3000, NULL);
    lv_timer_set_repeat_count(auto_return_timer, 1);
}
