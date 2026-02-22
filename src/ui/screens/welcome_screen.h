/**
 * @file welcome_screen.h
 * @brief Pantalla de bienvenida con animación
 */

#ifndef WELCOME_SCREEN_H
#define WELCOME_SCREEN_H

#include <lvgl.h>

/**
 * @brief Crea la pantalla de bienvenida
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* welcome_screen_create(void);

/**
 * @brief Inicia la animación de la pantalla de bienvenida
 * @param screen_next Pantalla a cargar después de la animación
 */
void welcome_screen_animate_to(lv_obj_t* screen_next);

#endif // WELCOME_SCREEN_H
