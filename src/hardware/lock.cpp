/**
 * @file lock.cpp
 * @brief Implementación del módulo de control de la cerradura
 */

#include "lock.h"
#include "../config/pins.h"
#include "../config/config.h"

// Estado actual de la cerradura
static LockState currentState = LockState::LOCKED;

// Callback para notificar cambio de estado (usado por ThingsBoard)
static void (*state_change_callback)(LockState) = nullptr;

void lock_init(void)
{
    pinMode(LOCK_PIN, OUTPUT);
    lock_lock();  // Estado inicial: bloqueado
    
    Serial.println("Cerradura electrónica inicializada");
}

void lock_unlock(void)
{
    digitalWrite(LOCK_PIN, HIGH);
    currentState = LockState::UNLOCKED;
    Serial.println("Cerradura desbloqueada");
    
    // Notificar cambio de estado si hay callback
    if (state_change_callback != nullptr) {
        state_change_callback(currentState);
    }
}

void lock_lock(void)
{
    digitalWrite(LOCK_PIN, LOW);
    currentState = LockState::LOCKED;
    Serial.println("Cerradura bloqueada");
    
    // Notificar cambio de estado si hay callback
    if (state_change_callback != nullptr) {
        state_change_callback(currentState);
    }
}

void lock_toggle(void)
{
    if (currentState == LockState::LOCKED) {
        lock_unlock();
    } else {
        lock_lock();
    }
}

LockState lock_get_state(void)
{
    return currentState;
}

bool lock_is_unlocked(void)
{
    return currentState == LockState::UNLOCKED;
}

/**
 * @brief Registra un callback para cambios de estado
 * @param callback Función a llamar cuando cambie el estado
 */
void lock_set_state_callback(void (*callback)(LockState))
{
    state_change_callback = callback;
}
