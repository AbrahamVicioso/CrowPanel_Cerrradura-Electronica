/**
 * @file config_screen.h
 * @brief Pantalla de modo configuración (portal AP activo)
 *
 * Muestra las credenciales del AP para que el usuario
 * pueda conectarse y abrir el panel web.
 */

#ifndef CONFIG_SCREEN_H
#define CONFIG_SCREEN_H

#include <lvgl.h>
#include <Arduino.h>

/**
 * @brief Crea y muestra la pantalla del portal de configuración.
 * @param ap_ssid   SSID del AP (ej. "CerraduraConfig")
 * @param ap_pass   Contraseña del AP
 * @param ap_ip     IP del servidor web (ej. "192.168.4.1")
 */
void config_screen_show(const String& ap_ssid, const String& ap_pass, const String& ap_ip);

#endif // CONFIG_SCREEN_H
