/**
 * @file standby_screen.h
 * @brief Pantalla de standby con reloj
 */

#ifndef STANDBY_SCREEN_H
#define STANDBY_SCREEN_H

#include <lvgl.h>

/**
 * @brief Crea la pantalla de standby
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* standby_screen_create(void);

/**
 * @brief Actualiza el reloj de la pantalla standby
 */
void standby_screen_update_time(void);

/**
 * @brief Establece el callback para cuando se toca la pantalla
 * @param callback Función a llamar cuando se toca
 */
void standby_screen_set_tap_callback(void (*callback)(void));

#endif // STANDBY_SCREEN_H
