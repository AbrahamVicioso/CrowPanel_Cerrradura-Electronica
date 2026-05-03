/**
 * @file locked_screen.cpp
 * @brief Implementación de la pantalla de bloqueo (puerta cerrada)
 */

#include "locked_screen.h"
#include "../theme.h"
#include "../../hardware/display.h"

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
    lv_obj_clear_flag(screen_locked, LV_OBJ_FLAG_SCROLLABLE);

    // Círculo icono — centrado, 1/3 superior
    lv_obj_t* circle_bg = lv_obj_create(screen_locked);
    lv_obj_set_size(circle_bg, 140, 140);
    lv_obj_set_style_bg_color(circle_bg, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(circle_bg, 3, 0);
    lv_obj_set_style_border_color(circle_bg, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(circle_bg, 70, 0);
    lv_obj_clear_flag(circle_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(circle_bg, LV_ALIGN_CENTER, 0, -85);

    lv_obj_t* lock_icon = lv_label_create(circle_bg);
    lv_label_set_text(lock_icon, LV_SYMBOL_EYE_CLOSE);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_ACCENT, 0);
    lv_obj_center(lock_icon);

    lv_obj_t* label_title = lv_label_create(screen_locked);
    lv_label_set_text(label_title, "SISTEMA BLOQUEADO");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_style_text_letter_space(label_title, 2, 0);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_title, 700);
    lv_obj_align(label_title, LV_ALIGN_CENTER, 0, 28);

    lv_obj_t* label_subtitle = lv_label_create(screen_locked);
    lv_label_set_text(label_subtitle, "Puerta cerrada");
    lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(label_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_subtitle, 700);
    lv_obj_align(label_subtitle, LV_ALIGN_CENTER, 0, 66);

    lv_obj_t* label_timer = lv_label_create(screen_locked);
    lv_label_set_text(label_timer, "Volviendo al teclado...");
    lv_obj_set_style_text_font(label_timer, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_timer, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(label_timer, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_timer, 700);
    lv_obj_align(label_timer, LV_ALIGN_BOTTOM_MID, 0, -28);

    return screen_locked;
}

static void auto_return_callback(lv_timer_t *timer)
{
    lv_timer_del(timer);
    auto_return_timer = nullptr;  // Resetear puntero

    if (callback != nullptr) {
        void (*cb)(void) = callback;  // Copiar callback
        callback = nullptr;            // Resetear para evitar doble ejecución
        cb();                          // Ejecutar callback
    }
}

/**
 * @brief Muestra la pantalla de bloqueo
 */
void locked_screen_show(void (*on_callback)(void))
{
    callback = on_callback;
    display_turn_on();  // Asegurar que el backlight esté encendido
    lv_scr_load_anim(screen_locked, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    
    // Auto-retorno después de 3 segundos
    if (auto_return_timer != nullptr) {
        lv_timer_del(auto_return_timer);
    }
    auto_return_timer = lv_timer_create(auto_return_callback, 3000, NULL);
    lv_timer_set_repeat_count(auto_return_timer, 1);
}
