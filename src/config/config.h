/**
 * @file config.h
 * @brief Configuración general del sistema de cerradura electrónica
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// CONFIGURACIÓN DEL SISTEMA
// ============================================

// Versión del firmware
#define FIRMWARE_VERSION "1.0.0"
#define FIRMWARE_NAME "Electronic Lock System"

// Configuración de la cerradura
#define LOCK_DEFAULT_STATE false  // false = bloqueado, true = desbloqueado
#define LOCK_AUTO_LOCK_DELAY_MS 5000  // Tiempo auto-bloqueo en ms

// Configuración del PIN
#define PIN_LENGTH 6
#define DEFAULT_PIN "123456"

// Tiempos de espera
#define WIFI_CONNECT_TIMEOUT_MS 15000
#define THINGSBOARD_RECONNECT_INTERVAL_MS 5000
#define NFC_CHECK_INTERVAL_MS 5000  /* Aumentado a 5s para reducir uso de CPU */

// Dimensiones de pantalla
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 480

#endif // CONFIG_H
