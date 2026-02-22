/**
 * @file pinpad_screen.h
 * @brief Pantalla del teclado PIN
 */

#ifndef PINPAD_SCREEN_H
#define PINPAD_SCREEN_H

#include <lvgl.h>
#include <Arduino.h>
#include <string>

/**
 * @brief Callback cuando se ingresa el PIN correcto
 */
typedef void (*PinpadSuccessCallback)(void);

/**
 * @brief Callback cuando el PIN es incorrecto
 */
typedef void (*PinpadErrorCallback)(void);

/**
 * @brief Crea la pantalla del teclado PIN
 * @return Puntero al objeto de pantalla
 */
lv_obj_t* pinpad_screen_create(void);

/**
 * @brief Registra callback para PIN correcto
 * @param callback Función a llamar
 */
void pinpad_set_success_callback(PinpadSuccessCallback callback);

/**
 * @brief Registra callback para PIN incorrecto
 * @param callback Función a llamar
 */
void pinpad_set_error_callback(PinpadErrorCallback callback);

/**
 * @brief Reinicia el PIN
 */
void pinpad_reset(void);

/**
 * @brief Obtiene el PIN actual
 * @return String con el PIN actual
 */
std::string pinpad_get_pin(void);

/**
 * @brief Establece el PIN correcto
 * @param pin Nuevo PIN correcto
 */
void pinpad_set_correct_pin(const String& pin);

/**
 * @brief Muestra mensaje de estado
 * @param message Mensaje a mostrar
 * @param color Color del texto
 */
void pinpad_show_status(const char* message, lv_color_t color);

#endif // PINPAD_SCREEN_H
