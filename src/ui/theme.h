/**
 * @file theme.h
 * @brief Tema visual de la aplicación
 */

#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

// ============================================
// DEFINICIÓN DE COLORES
// ============================================

// Colores corporativos elegantes
#define COLOR_PRIMARY       lv_color_hex(0x1a1a2e)   // Azul oscuro profundo
#define COLOR_SECONDARY     lv_color_hex(0x16213e)   // Azul marino
#define COLOR_ACCENT        lv_color_hex(0x0f3460)    // Azul medio
#define COLOR_SUCCESS       lv_color_hex(0x00b894)   // Verde esmeralda
#define COLOR_ERROR         lv_color_hex(0xd63031)   // Rojo elegante
#define COLOR_WARNING       lv_color_hex(0xf39c12)   // Naranja
#define COLOR_TEXT          lv_color_hex(0xecf0f1)   // Gris claro
#define COLOR_TEXT_SECONDARY lv_color_hex(0x95a5a6)  // Gris medio
#define COLOR_BUTTON        lv_color_hex(0x2c3e50)  // Gris azulado
#define COLOR_BUTTON_PRESSED lv_color_hex(0x34495e) // Gris azulado oscuro

// ============================================
// ESTILOS PREDEFINIDOS
// ============================================

extern lv_style_t style_btn_main;
extern lv_style_t style_btn_pressed;
extern lv_style_t style_btn_success;
extern lv_style_t style_btn_error;
extern lv_style_t style_text_main;
extern lv_style_t style_panel;

/**
 * @brief Inicializa los estilos del tema
 */
void theme_init_styles(void);

/**
 * @brief Aplica estilo de botón a un objeto
 * @param obj Objeto al que aplicar el estilo
 * @param is_pressed Si es true, aplica estilo de presionado
 */
void theme_apply_button_style(lv_obj_t* obj, bool is_pressed = false);

#endif // THEME_H
