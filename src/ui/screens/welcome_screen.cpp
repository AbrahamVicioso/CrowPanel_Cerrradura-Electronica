/**
 * @file welcome_screen.cpp
 * @brief Implementación de la pantalla de bienvenida
 */

#include "welcome_screen.h"
#include "../theme.h"
#include "../../hardware/display.h"

// Objeto de pantalla
static lv_obj_t* welcome_screen = nullptr;
static lv_obj_t* next_screen = nullptr;

// Forward declaration
static void animation_callback(lv_timer_t *timer);

/**
 * @brief Crea la pantalla de bienvenida
 */
lv_obj_t* welcome_screen_create(void)
{
    welcome_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(welcome_screen, COLOR_PRIMARY, 0);
    lv_obj_clear_flag(welcome_screen, LV_OBJ_FLAG_SCROLLABLE);

    // Círculo icono — centrado, 1/3 superior de pantalla
    // lv_obj_align pone el CENTRO del obj en (screen_cx + ox, screen_cy + oy)
    // screen_cy=240, circle_half=70 → top = 240-90-70 = 80
    lv_obj_t* circle_bg = lv_obj_create(welcome_screen);
    lv_obj_set_size(circle_bg, 140, 140);
    lv_obj_set_style_bg_color(circle_bg, lv_color_hex(0xe94560), 0);
    lv_obj_set_style_border_width(circle_bg, 0, 0);
    lv_obj_set_style_radius(circle_bg, 70, 0);
    lv_obj_clear_flag(circle_bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_align(circle_bg, LV_ALIGN_CENTER, 0, -90);

    lv_obj_t* lock_icon = lv_label_create(circle_bg);
    lv_label_set_text(lock_icon, LV_SYMBOL_HOME);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_TEXT, 0);
    lv_obj_center(lock_icon);

    // Texto principal centrado
    lv_obj_t* label_title = lv_label_create(welcome_screen);
    lv_label_set_text(label_title, "SISTEMA DE CERRADURA");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_style_text_align(label_title, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_title, 700);
    lv_obj_align(label_title, LV_ALIGN_CENTER, 0, 18);

    lv_obj_t* label_subtitle = lv_label_create(welcome_screen);
    lv_label_set_text(label_subtitle, "ELECTRONICA");
    lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_subtitle, COLOR_TEXT, 0);
    lv_obj_set_style_text_letter_space(label_subtitle, 4, 0);
    lv_obj_set_style_text_align(label_subtitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_subtitle, 700);
    lv_obj_align(label_subtitle, LV_ALIGN_CENTER, 0, 60);

    // Línea decorativa bajo el título
    lv_obj_t* line = lv_obj_create(welcome_screen);
    lv_obj_set_size(line, 160, 2);
    lv_obj_set_style_bg_color(line, COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(line, 0, 0);
    lv_obj_set_style_radius(line, 1, 0);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(line, LV_ALIGN_CENTER, 0, 90);

    // Texto de carga — abajo
    lv_obj_t* label_loading = lv_label_create(welcome_screen);
    lv_label_set_text(label_loading, "Inicializando...");
    lv_obj_set_style_text_font(label_loading, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_loading, COLOR_TEXT_MUTED, 0);
    lv_obj_set_style_text_align(label_loading, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(label_loading, 700);
    lv_obj_align(label_loading, LV_ALIGN_BOTTOM_MID, 0, -24);

    return welcome_screen;
}

/**
 * @brief Callback de animación
 */
static void animation_callback(lv_timer_t* timer)
{
    lv_timer_del(timer);
    if (next_screen != nullptr) {
        lv_scr_load_anim(next_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    }
}

/**
 * @brief Inicia la animación hacia la siguiente pantalla
 */
void welcome_screen_animate_to(lv_obj_t* screen_next)
{
    next_screen = screen_next;
    display_turn_on();
    // Un solo timer de 1000ms — antes era 50×20ms = 50 callbacks innecesarios
    lv_timer_t* timer = lv_timer_create(animation_callback, 1000, NULL);
    lv_timer_set_repeat_count(timer, 1);
}
