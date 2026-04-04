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
 * @brief Muestra la pantalla de standby con transición
 * @param screen Puntero a la pantalla actual (para la animación)
 * @param anim_type Tipo de animación
 * @param duration Duración de la animación en ms
 */
void standby_screen_show(lv_obj_t* from_screen, lv_scr_load_anim_t anim_type, uint32_t duration);

/**
 * @brief Actualiza el reloj de la pantalla standby
 */
void standby_screen_update_time(void);

/**
 * @brief Establece el callback para cuando se toca la pantalla (abre pinpad)
 * @param callback Función a llamar cuando se toca
 */
void standby_screen_set_tap_callback(void (*callback)(void));

/**
 * @brief Registra callback del gesto oculto para el portal de configuración.
 *        Se activa con 5 toques en la esquina superior derecha dentro de 6 segundos.
 * @param callback Función a llamar cuando se detecta el gesto
 */
void standby_screen_set_portal_callback(void (*callback)(void));

/**
 * @brief Registra callback de long press (mantener ~2s) → pantalla de info.
 */
void standby_screen_set_info_callback(void (*callback)(void));

/**
 * @brief Actualiza los indicadores de conectividad en la pantalla standby
 * @param wifiOk   true si WiFi conectado
 * @param tbOk     true si ThingsBoard conectado
 */
void standby_screen_update_status(bool wifiOk, bool tbOk);

#endif // STANDBY_SCREEN_H
