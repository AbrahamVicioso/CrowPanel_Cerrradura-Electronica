/**
 * @file pinpad_screen.h
 * @brief Pantalla del teclado PIN con soporte de lockout por intentos fallidos
 */

#ifndef PINPAD_SCREEN_H
#define PINPAD_SCREEN_H

#include <lvgl.h>
#include <Arduino.h>
#include <string>

typedef void (*PinpadSuccessCallback)(void);
typedef void (*PinpadErrorCallback)(void);
typedef void (*PinpadFailedAttemptsCallback)(int count);

/** Crea la pantalla (llamar una sola vez en setup) */
lv_obj_t* pinpad_screen_create(void);

/** Muestra la pantalla con animación (puede llamarse varias veces) */
void pinpad_screen_show(lv_obj_t* from_screen, lv_scr_load_anim_t anim_type, uint32_t duration);

// ── Callbacks ───────────────────────────────────────────────
void pinpad_set_success_callback(PinpadSuccessCallback cb);
void pinpad_set_error_callback(PinpadErrorCallback cb);
/** Llamado cada vez que aumentan los intentos fallidos */
void pinpad_set_failed_attempts_callback(PinpadFailedAttemptsCallback cb);
/** Llamado cuando expira el timer de inactividad */
void pinpad_set_inactivity_callback(void (*callback)(void));
/** Llamado cuando el usuario toca el botón Inicio (volver a standby) */
void pinpad_set_back_callback(void (*callback)(void));

// ── Control de PIN ───────────────────────────────────────────
void        pinpad_reset(void);
std::string pinpad_get_pin(void);

/** Registra la función que valida el PIN ingresado.
 *  Debe devolver true si el PIN es correcto y vigente. */
typedef bool (*PinValidatorCallback)(const String& pin);
void        pinpad_set_pin_validator(PinValidatorCallback validator);

// ── Lockout ──────────────────────────────────────────────────
void pinpad_set_max_failed_attempts(int max);
void pinpad_set_lockout_duration_ms(uint32_t ms);
void pinpad_reset_failed_attempts(void);  // usado por RPC resetLockout
bool pinpad_is_locked_out(void);

// ── Misc ─────────────────────────────────────────────────────
void pinpad_show_status(const char* message, lv_color_t color);

#endif // PINPAD_SCREEN_H
