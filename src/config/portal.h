/**
 * @file portal.h
 * @brief Portal de configuración: AP WiFi + servidor HTTP
 *
 * Activación: 5 toques en la esquina superior derecha de la pantalla standby
 * Acceso:     conectar al AP "CerraduraConfig" con la contraseña guardada,
 *             luego abrir http://192.168.4.1 en el navegador.
 * Auth:       usuario "admin", contraseña = ap_password (misma del AP)
 */

#ifndef PORTAL_H
#define PORTAL_H

#include <Arduino.h>

/**
 * @brief Inicia el portal de configuración (AP + WebServer).
 *        Desconecta WiFi STA antes de activar AP.
 */
void portal_start(void);

/**
 * @brief Debe llamarse desde el loop principal mientras el portal esté activo.
 *        Procesa clientes HTTP y gestiona reinicio programado.
 */
void portal_loop(void);

/**
 * @brief Retorna true si el portal está activo
 */
bool portal_is_active(void);

/**
 * @brief Retorna el SSID del AP de configuración
 */
String portal_get_ap_ssid(void);

/**
 * @brief Retorna la IP del portal web
 */
String portal_get_ap_ip(void);

/**
 * @brief Retorna la contraseña actual del AP
 */
String portal_get_ap_pass(void);

#endif // PORTAL_H
