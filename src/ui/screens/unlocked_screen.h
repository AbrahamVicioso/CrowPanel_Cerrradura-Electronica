/**
 * @file unlocked_screen.h
 * @brief Pantalla de desbloqueo exitoso
 */

#ifndef UNLOCKED_SCREEN_H
#define UNLOCKED_SCREEN_H

#include <lvgl.h>

/**
 * @brief Crea la pantalla de desbloqueo exitoso
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* unlocked_screen_create(void);

/**
 * @brief Muestra la pantalla de desbloqueo
 * @param auto_lock_delay_ms Tiempo hasta auto-bloqueo en ms
 * @param on_lock_callback Callback a llamar cuando se auto-bloquee
 */
void unlocked_screen_show(uint32_t auto_lock_delay_ms, void (*on_lock_callback)(void));

#endif // UNLOCKED_SCREEN_H
