/**
 * @file lock.h
 * @brief Módulo de control de la cerradura electrónica
 */

#ifndef LOCK_H
#define LOCK_H

#include <Arduino.h>

/**
 * @brief Estado actual de la cerradura
 */
enum class LockState {
    LOCKED,      // Cerradura bloqueada
    UNLOCKED     // Cerradura desbloqueada
};

/**
 * @brief Inicializa el módulo de cerradura
 */
void lock_init(void);

/**
 * @brief Desbloquea la cerradura
 */
void lock_unlock(void);

/**
 * @brief Bloquea la cerradura
 */
void lock_lock(void);

/**
 * @brief Alterna el estado de la cerradura
 */
void lock_toggle(void);

/**
 * @brief Obtiene el estado actual de la cerradura
 * @return Estado actual de la cerradura
 */
LockState lock_get_state(void);

/**
 * @brief Verifica si la cerradura está desbloqueada
 * @return true si está desbloqueada, false si está bloqueada
 */
bool lock_is_unlocked(void);

#endif // LOCK_H
