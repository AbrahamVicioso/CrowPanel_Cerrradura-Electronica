/**
 * @file welcome_screen.cpp
 * @brief Implementación de la pantalla de bienvenida
 */

#include "welcome_screen.h"
#include "../theme.h"

// Objeto de pantalla
static lv_obj_t* welcome_screen = nullptr;
static lv_obj_t* arc_animation = nullptr;
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

    // Logo/Título
    lv_obj_t *label_welcome = lv_label_create(welcome_screen);
    lv_label_set_text(label_welcome, "SISTEMA DE ACCESO");
    lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(label_welcome, COLOR_TEXT, 0);
    lv_obj_align(label_welcome, LV_ALIGN_CENTER, 0, -60);

    // Subtítulo
    lv_obj_t *label_subtitle = lv_label_create(welcome_screen);
    lv_label_set_text(label_subtitle, "Control de Cerradura Electronica");
    lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_subtitle, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(label_subtitle, LV_ALIGN_CENTER, 0, 0);

    // Círculo de carga animado
    arc_animation = lv_arc_create(welcome_screen);
    lv_obj_set_size(arc_animation, 100, 100);
    lv_arc_set_rotation(arc_animation, 270);
    lv_arc_set_bg_angles(arc_animation, 0, 360);
    lv_arc_set_angles(arc_animation, 0, 0);
    lv_obj_set_style_arc_color(arc_animation, COLOR_ACCENT, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc_animation, COLOR_SUCCESS, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc_animation, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc_animation, 8, LV_PART_INDICATOR);
    lv_obj_remove_style(arc_animation, NULL, LV_PART_KNOB);
    lv_obj_align(arc_animation, LV_ALIGN_CENTER, 0, 80);
    lv_obj_clear_flag(arc_animation, LV_OBJ_FLAG_CLICKABLE);

    return welcome_screen;
}

/**
 * @brief Callback de animación
 */
static void animation_callback(lv_timer_t *timer)
{
    static int angle = 0;
    lv_arc_set_value(arc_animation, angle);
    angle += 5;

    if (angle >= 360)
    {
        lv_timer_del(timer);
        if (next_screen != nullptr) {
            lv_scr_load_anim(next_screen, LV_SCR_LOAD_ANIM_FADE_ON, 500, 100, false);
        }
    }
}

/**
 * @brief Inicia la animación hacia la siguiente pantalla
 */
void welcome_screen_animate_to(lv_obj_t* screen_next)
{
    next_screen = screen_next;
    lv_timer_t *timer = lv_timer_create(animation_callback, 20, NULL);
}
