/**
 * @file theme.h
 * @brief Tema visual de la aplicación
 */

#ifndef THEME_H
#define THEME_H

#include <lvgl.h>

// ============================================
// DEFINICIÓN DE COLORES - PALETA MODERNA
// ============================================

// Colores principales - Gradiente azul/morado moderno
#define COLOR_PRIMARY       lv_color_hex(0x1a1a2e)   // Fondo principal oscuro
#define COLOR_SECONDARY     lv_color_hex(0x16213e)   // Fondo secundario
#define COLOR_ACCENT        lv_color_hex(0xe94560)   // Rojo/coral vibrante
#define COLOR_ACCENT_LIGHT  lv_color_hex(0xff6b6b)  // Rojo claro
#define COLOR_SUCCESS       lv_color_hex(0x00d9a5)  // Verde esmeralda brillante
#define COLOR_SUCCESS_DARK  lv_color_hex(0x00b894)  // Verde esmeralda
#define COLOR_ERROR         lv_color_hex(0xff4757)  // Rojo vibrante
#define COLOR_WARNING       lv_color_hex(0xffbe76)  // Amarillo/dorado
#define COLOR_TEXT          lv_color_hex(0xffffff)  // Blanco
#define COLOR_TEXT_SECONDARY lv_color_hex(0xa4b0be) // Gris claro
#define COLOR_TEXT_MUTED    lv_color_hex(0x747d8c)  // Gris medio
#define COLOR_BUTTON        lv_color_hex(0x2f3542)  // Gris oscuro
#define COLOR_BUTTON_LIGHT  lv_color_hex(0x3d4554)  // Gris oscuro claro
#define COLOR_BUTTON_PRESSED lv_color_hex(0x1e272e) // Gris muy oscuro

// Colores de gradiente (simulados con colores sólidos)
#define COLOR_GRADIENT_START lv_color_hex(0x667eea)  // Azul violeta
#define COLOR_GRADIENT_END   lv_color_hex(0x764ba2)  // Violeta

// ============================================
// ESTILOS PREDEFINIDOS
// ============================================

extern lv_style_t style_btn_main;
extern lv_style_t style_btn_pressed;
extern lv_style_t style_btn_success;
extern lv_style_t style_btn_error;
extern lv_style_t style_btn_accent;
extern lv_style_t style_text_main;
extern lv_style_t style_text_title;
extern lv_style_t style_text_subtitle;
extern lv_style_t style_panel;
extern lv_style_t style_panel_card;

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

/**
 * @brief Crea un botón con estilo profesional
 * @param parent Objeto padre
 * @param x Posición X
 * @param y Posición Y
 * @param width Ancho
 * @param height Alto
 * @param label Texto del botón
 * @return Puntero al botón creado
 */
lv_obj_t* theme_create_button(lv_obj_t* parent, int x, int y, int width, int height, const char* label);

/**
 * @brief Crea un panel con estilo card
 * @param parent Objeto padre
 * @param width Ancho
 * @param height Alto
 * @return Puntero al panel creado
 */
lv_obj_t* theme_create_card(lv_obj_t* parent, int width, int height);

#endif // THEME_H
