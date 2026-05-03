/**
 * @file thingsboard.h
 * @brief Módulo ThingsBoard — shared attributes, RPC, telemetría
 *
 * Atributos compartidos recibidos (TB Dashboard → Dispositivo):
 *   credenciales       array JSON de credenciales
 *   autoLockDelay      ms (entero)
 *   nfcEnabled         bool
 *   authorizedNfcUid   "AA:BB:CC:DD"
 *   maxFailedAttempts  entero
 *   lockoutDuration    segundos
 *
 * Comandos RPC recibidos (TB Dashboard → Dispositivo):
 *   unlock             → desbloquear  { "lockState": "unlocked" }
 *   unlockTemporary    → desbloquear N ms  { "duration": 5000 }
 *   resetLockout       → limpiar bloqueo de intentos
 *
 * Control unlock: vía RPC "unlock" — lockState ya NO es shared attribute.
 *   - Dashboard → Dispositivo: TB llama RPC "unlock" → dispositivo abre
 *   - Dispositivo → Dashboard: publica client attr "lockState" para visibilidad
 *
 * Telemetría enviada (Dispositivo → TB):
 *   locked             bool
 *   accessGranted      bool
 *   accessMethod       "pin" / "nfc" / "remote" / "rpc"
 *   failedAttempts     int
 *   uptime             ms
 *
 * Atributos de cliente (Dispositivo → TB, solo al conectar):
 *   firmwareVersion, ipAddress, macAddress
 */

#ifndef THINGSBOARD_H
#define THINGSBOARD_H

#include <WiFi.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include "../hardware/lock.h"
#include "../config/credentials.h"

// ============================================
// TIPOS DE CALLBACK
// ============================================

/** Estado remoto cambiado (lockState attribute) */
typedef void (*RemoteStateCallback)(bool locked);

/** Array de credenciales recibido desde TB (JSON serializado) */
typedef void (*CredentialsUpdateCallback)(const char* json);

/** Delay auto-bloqueo cambiado */
typedef void (*AutoLockDelayCallback)(uint32_t delayMs);

/** NFC habilitado/deshabilitado desde TB */
typedef void (*NfcEnabledCallback)(bool enabled);

/** UID NFC autorizado cambiado */
typedef void (*NfcUidCallback)(const char* uid);

/** Máximo intentos fallidos cambiado */
typedef void (*MaxFailedCallback)(int maxAttempts);

/** Duración lockout cambiada */
typedef void (*LockoutDurationCallback)(int seconds);

/** RPC unlockTemporary recibido */
typedef void (*RpcUnlockTempCallback)(uint32_t durationMs);

/** RPC resetLockout recibido */
typedef void (*RpcResetLockoutCallback)(void);

// ============================================
// FUNCIONES PÚBLICAS — CICLO DE VIDA
// ============================================

void thingsboard_init(void);
bool thingsboard_connect(void);
void thingsboard_disconnect(void);
void thingsboard_loop(void);
void thingsboard_reconnect(void);
bool thingsboard_is_connected(void);

// ============================================
// FUNCIONES PÚBLICAS — PUBLICACIÓN
// ============================================

/** Publica estado de cerradura como telemetría + atributo cliente */
bool thingsboard_publish_lock_state(LockState state);

/**
 * @brief Registra un evento de acceso en ThingsBoard
 * @param granted  true si fue exitoso
 * @param method   "pin", "nfc", "remote", "rpc"
 */
bool thingsboard_publish_access_event(bool granted, const char* method);

/**
 * @brief Registra un evento de acceso con el cuerpo completo de la credencial.
 *        Envía accessGranted, accessMethod y accessEvent (JSON serializado con datos del titular).
 * @param granted  true si fue exitoso
 * @param method   "pin", "nfc_hce", "nfc_json", etc.
 * @param match    Credencial que coincidió (puede ser nullptr si no hay info)
 */
bool thingsboard_publish_access_event_with_credential(bool granted, const char* method,
                                                       const CredentialMatch* match);

/** Publica intentos fallidos actuales */
bool thingsboard_publish_failed_attempts(int count);

/** Publica atributos de cliente (firmware, IP, MAC) */
bool thingsboard_publish_client_attributes(void);

// ============================================
// FUNCIONES PÚBLICAS — REGISTRO DE CALLBACKS
// ============================================

void thingsboard_set_state_change_callback(RemoteStateCallback cb);
void thingsboard_set_credentials_update_callback(CredentialsUpdateCallback cb);
void thingsboard_set_auto_lock_delay_callback(AutoLockDelayCallback cb);
void thingsboard_set_nfc_enabled_callback(NfcEnabledCallback cb);
void thingsboard_set_nfc_uid_callback(NfcUidCallback cb);
void thingsboard_set_max_failed_callback(MaxFailedCallback cb);
void thingsboard_set_lockout_duration_callback(LockoutDurationCallback cb);
void thingsboard_set_rpc_unlock_temp_callback(RpcUnlockTempCallback cb);
void thingsboard_set_rpc_reset_lockout_callback(RpcResetLockoutCallback cb);


#endif // THINGSBOARD_H
