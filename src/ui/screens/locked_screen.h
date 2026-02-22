/**
 * @file locked_screen.h
 * @brief Pantalla de bloqueo (puerta cerrada)
 */

#ifndef LOCKED_SCREEN_H
#define LOCKED_SCREEN_H

#include <lvgl.h>

/**
 * @brief Crea la pantalla de bloqueo
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* locked_screen_create(void);

/**
 * @brief Muestra la pantalla de bloqueo
 * @param on_callback Función a llamar cuando se necesite volver al pinpad
 */
void locked_screen_show(void (*on_callback)(void));

#endif // LOCKED_SCREEN_H
