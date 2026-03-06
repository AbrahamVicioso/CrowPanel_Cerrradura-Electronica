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

    // ==================== LOGO SECTION ====================
    
    // Icono de candado grande (círculo decorativo sin sombra)
    lv_obj_t* circle_bg = lv_obj_create(welcome_screen);
    lv_obj_set_size(circle_bg, 140, 140);
    lv_obj_set_pos(circle_bg, 330, 100);
    lv_obj_set_style_bg_color(circle_bg, lv_color_hex(0xe94560), 0);
    lv_obj_set_style_border_width(circle_bg, 0, 0);
    lv_obj_set_style_radius(circle_bg, 70, 0);
    // Shadow removido - era costoso en CPU

    // Icono de candado usando símbolos LVGL
    lv_obj_t *lock_icon = lv_label_create(circle_bg);
    lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_TEXT, 0);
    lv_obj_center(lock_icon);

    // Título principal
    lv_obj_t *label_title = lv_label_create(welcome_screen);
    lv_label_set_text(label_title, "SISTEMA DE");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT_SECONDARY, 0);
    lv_obj_set_pos(label_title, 0, 260);
    lv_obj_set_width(label_title, 800);

    // Subtítulo (nombre del sistema)
    lv_obj_t *label_subtitle = lv_label_create(welcome_screen);
    lv_label_set_text(label_subtitle, "CERRADURA ELECTRONICA");
    lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_24, 0);  // Cambiado de 28 a 24 (optimización)
    lv_obj_set_style_text_color(label_subtitle, COLOR_TEXT, 0);
    lv_obj_set_style_text_letter_space(label_subtitle, 2, 0);
    lv_obj_set_pos(label_subtitle, 0, 295);
    lv_obj_set_width(label_subtitle, 800);

    // Línea decorativa
    lv_obj_t* line = lv_obj_create(welcome_screen);
    lv_obj_set_size(line, 150, 3);
    lv_obj_set_pos(line, 325, 345);
    lv_obj_set_style_bg_color(line, COLOR_ACCENT, 0);
    lv_obj_set_style_radius(line, 2, 0);

    // ==================== LOADING SECTION ====================
    
    // Loading indicator simple (reemplazado spinner costoso)
    lv_obj_t *label_loading = lv_label_create(welcome_screen);
    lv_label_set_text(label_loading, "Inicializando...");
    lv_obj_set_style_text_font(label_loading, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_loading, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(label_loading, 0, 420);
    lv_obj_set_width(label_loading, 800);

    return welcome_screen;
}

/**
 * @brief Callback de animación
 */
static void animation_callback(lv_timer_t *timer)
{
    static int counter = 0;
    counter++;

    if (counter >= 50)  // Wait about 1 second (50 * 20ms)
    {
        lv_timer_del(timer);
        if (next_screen != nullptr) {
            // Transición elegante con fade
            lv_scr_load_anim(next_screen, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
        }
    }
}

/**
 * @brief Inicia la animación hacia la siguiente pantalla
 */
void welcome_screen_animate_to(lv_obj_t* screen_next)
{
    next_screen = screen_next;
    display_turn_on();  // Asegurar que el backlight estéendido
    lv_timer_t *timer = lv_timer_create(animation_callback, 20, NULL);
}
