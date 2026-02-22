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
// Style for accent buttons
lv_style_t style_btn_accent;
// Style for text labels
lv_style_t style_text_main;
// Style for titles
lv_style_t style_text_title;
// Style for subtitles
lv_style_t style_text_subtitle;
// Style for panel backgrounds
lv_style_t style_panel;
// Style for card backgrounds
lv_style_t style_panel_card;

void theme_init_styles(void)
{
    // Estilo para botones principales
    lv_style_init(&style_btn_main);
    lv_style_set_bg_color(&style_btn_main, COLOR_BUTTON);
    lv_style_set_radius(&style_btn_main, 15);
    lv_style_set_shadow_width(&style_btn_main, 8);
    lv_style_set_shadow_opa(&style_btn_main, LV_OPA_40);
    lv_style_set_shadow_color(&style_btn_main, lv_color_black());
    lv_style_set_border_width(&style_btn_main, 0);
    lv_style_set_pad_all(&style_btn_main, 10);
    
    // Estilo para botones presionados
    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, COLOR_BUTTON_PRESSED);
    lv_style_set_radius(&style_btn_pressed, 15);
    lv_style_set_shadow_width(&style_btn_pressed, 4);
    lv_style_set_shadow_opa(&style_btn_pressed, LV_OPA_20);
    
    // Estilo para botones de éxito
    lv_style_init(&style_btn_success);
    lv_style_set_bg_color(&style_btn_success, COLOR_SUCCESS);
    lv_style_set_radius(&style_btn_success, 15);
    lv_style_set_shadow_width(&style_btn_success, 12);
    lv_style_set_shadow_opa(&style_btn_success, LV_OPA_50);
    lv_style_set_shadow_color(&style_btn_success, COLOR_SUCCESS_DARK);
    
    // Estilo para botones de error
    lv_style_init(&style_btn_error);
    lv_style_set_bg_color(&style_btn_error, COLOR_ERROR);
    lv_style_set_radius(&style_btn_error, 15);
    lv_style_set_shadow_width(&style_btn_error, 12);
    lv_style_set_shadow_opa(&style_btn_error, LV_OPA_50);
    lv_style_set_shadow_color(&style_btn_error, COLOR_ERROR);
    
    // Estilo para botones de acento
    lv_style_init(&style_btn_accent);
    lv_style_set_bg_color(&style_btn_accent, COLOR_ACCENT);
    lv_style_set_radius(&style_btn_accent, 15);
    lv_style_set_shadow_width(&style_btn_accent, 12);
    lv_style_set_shadow_opa(&style_btn_accent, LV_OPA_50);
    lv_style_set_shadow_color(&style_btn_accent, COLOR_ACCENT_LIGHT);
    
    // Estilo para texto principal
    lv_style_init(&style_text_main);
    lv_style_set_text_color(&style_text_main, COLOR_TEXT);
    lv_style_set_text_font(&style_text_main, &lv_font_montserrat_16);
    
    // Estilo para títulos
    lv_style_init(&style_text_title);
    lv_style_set_text_color(&style_text_title, COLOR_TEXT);
    lv_style_set_text_font(&style_text_title, &lv_font_montserrat_32);
    lv_style_set_text_letter_space(&style_text_title, 2);
    
    // Estilo para subtítulos
    lv_style_init(&style_text_subtitle);
    lv_style_set_text_color(&style_text_subtitle, COLOR_TEXT_SECONDARY);
    lv_style_set_text_font(&style_text_subtitle, &lv_font_montserrat_16);
    
    // Estilo para paneles
    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, COLOR_SECONDARY);
    lv_style_set_border_width(&style_panel, 0);
    lv_style_set_radius(&style_panel, 20);
    
    // Estilo para cards
    lv_style_init(&style_panel_card);
    lv_style_set_bg_color(&style_panel_card, lv_color_hex(0x2d3436));
    lv_style_set_border_width(&style_panel_card, 0);
    lv_style_set_radius(&style_panel_card, 25);
    lv_style_set_shadow_width(&style_panel_card, 20);
    lv_style_set_shadow_opa(&style_panel_card, LV_OPA_30);
    lv_style_set_shadow_color(&style_panel_card, lv_color_black());
}

void theme_apply_button_style(lv_obj_t* obj, bool is_pressed)
{
    if (is_pressed) {
        lv_obj_add_style(obj, &style_btn_pressed, LV_STATE_PRESSED);
    }
    lv_obj_add_style(obj, &style_btn_main, LV_PART_MAIN);
}

lv_obj_t* theme_create_button(lv_obj_t* parent, int x, int y, int width, int height, const char* label)
{
    lv_obj_t* btn = lv_btn_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_set_pos(btn, x, y);
    lv_obj_add_style(btn, &style_btn_main, LV_PART_MAIN);
    
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, label);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
    lv_obj_center(lbl);
    
    return btn;
}

lv_obj_t* theme_create_card(lv_obj_t* parent, int width, int height)
{
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, width, height);
    lv_obj_add_style(card, &style_panel_card, LV_PART_MAIN);
    return card;
}
