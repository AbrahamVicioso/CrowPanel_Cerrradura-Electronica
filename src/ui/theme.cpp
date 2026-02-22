/**
 * @file theme.cpp
 * @brief Implementación del tema visual
 */

#include "theme.h"

// ============================================
// ESTILOS PREDEFINIDOS
// ============================================

// Style for main buttons
lv_style_t style_btn_main;
// Style for pressed buttons  
lv_style_t style_btn_pressed;
// Style for success buttons
lv_style_t style_btn_success;
// Style for error buttons
lv_style_t style_btn_error;
// Style for text labels
lv_style_t style_text_main;
// Style for panel backgrounds
lv_style_t style_panel;

void theme_init_styles(void)
{
    // Estilo para botones principales
    lv_style_init(&style_btn_main);
    lv_style_set_bg_color(&style_btn_main, COLOR_BUTTON);
    lv_style_set_radius(&style_btn_main, 10);
    lv_style_set_shadow_width(&style_btn_main, 10);
    lv_style_set_shadow_opa(&style_btn_main, LV_OPA_30);
    
    // Estilo para botones presionados
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, COLOR_BUTTON_PRESSED);
    lv_style_set_radius(&style_btn_pressed, 10);
    
    // Estilo para botones de éxito
    lv_style_init(&style_btn_success);
    lv_style_set_bg_color(&style_btn_success, COLOR_SUCCESS);
    lv_style_set_radius(&style_btn_success, 10);
    lv_style_set_shadow_width(&style_btn_success, 10);
    lv_style_set_shadow_opa(&style_btn_success, LV_OPA_30);
    
    // Estilo para botones de error
    lv_style_init(&style_btn_error);
    lv_style_set_bg_color(&style_btn_error, COLOR_ERROR);
    lv_style_set_radius(&style_btn_error, 10);
    lv_style_set_shadow_width(&style_btn_error, 10);
    lv_style_set_shadow_opa(&style_btn_error, LV_OPA_30);
    
    // Estilo para texto principal
    lv_style_init(&style_text_main);
    lv_style_set_text_color(&style_text_main, COLOR_TEXT);
    
    // Estilo para paneles
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, COLOR_SECONDARY);
    lv_style_set_border_width(&style_panel, 0);
    lv_style_set_radius(&style_panel, 15);
}

void theme_apply_button_style(lv_obj_t* obj, bool is_pressed)
{
    if (is_pressed) {
        lv_obj_add_style(obj, &style_btn_pressed, LV_STATE_PRESSED);
    }
    lv_obj_add_style(obj, &style_btn_main, LV_PART_MAIN);
}
