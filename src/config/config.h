/**
 * @file config.h
 * @brief Configuración general del sistema de cerradura electrónica
 */

#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// VERSIÓN DEL FIRMWARE
// ============================================
#define FIRMWARE_VERSION "2.0.0"
#define FIRMWARE_NAME   "Electronic Lock System"

// ============================================
// CONFIGURACIÓN DE LA CERRADURA
// ============================================
#define LOCK_DEFAULT_STATE          false   // false = bloqueado
#define LOCK_AUTO_LOCK_DELAY_MS     5000    // ms antes de re-bloquear (default)
#define LOCK_AUTO_LOCK_DELAY_MIN    2000    // mínimo permitido via TB
#define LOCK_AUTO_LOCK_DELAY_MAX    60000   // máximo permitido via TB

// ============================================
// CONFIGURACIÓN DEL PIN
// ============================================
#define PIN_LENGTH      6
#define DEFAULT_PIN     "123456"

// ============================================
// SEGURIDAD — BLOQUEO POR INTENTOS
// ============================================
#define MAX_FAILED_ATTEMPTS         3       // intentos antes de lockout
#define LOCKOUT_DURATION_MS         30000   // 30 segundos de bloqueo
#define LOCKOUT_DURATION_MIN_S      10      // mínimo configurable desde TB
#define LOCKOUT_DURATION_MAX_S      300     // máximo configurable desde TB

// ============================================
// TIEMPOS DE ESPERA
// ============================================
#define WIFI_CONNECT_TIMEOUT_MS             15000
#define THINGSBOARD_RECONNECT_INTERVAL_MS   5000
#define NFC_CHECK_INTERVAL_MS               1000   // 1s para respuesta más ágil

// ============================================
// DIMENSIONES DE PANTALLA
// ============================================
#define SCREEN_WIDTH    800
#define SCREEN_HEIGHT   480

#endif // CONFIG_H
