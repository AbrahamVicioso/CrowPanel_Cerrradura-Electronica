/**
 * @file error_screen.h
 * @brief Pantalla de error/intento fallido
 */

#ifndef ERROR_SCREEN_H
#define ERROR_SCREEN_H

#include <lvgl.h>

/**
 * @brief Crea la pantalla de error
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* error_screen_create(void);

/**
 * @brief Muestra la pantalla de error
 * @param on_callback Función a llamar cuando termine el delay
 * @param delay_ms Delay antes de volver (0 = manual)
 */
void error_screen_show(void (*on_callback)(void), uint32_t delay_ms = 2000);

#endif // ERROR_SCREEN_H
