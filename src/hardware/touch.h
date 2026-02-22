/**
 * @file touch.h
 * @brief Módulo de control del touchscreen
 */

#ifndef TOUCH_H
#define TOUCH_H

#include <TAMC_GT911.h>

// Instancia global del touchscreen
extern TAMC_GT911 ts;

/**
 * @brief Inicializa el touchscreen
 */
void touch_init(void);

/**
 * @brief Lee las coordenadas del touch
 * @param x Puntero a variable para coordenada X
 * @param y Puntero a variable para coordenada Y
 * @return true si hay touch detectado, false en caso contrario
 */
bool touch_read(int* x, int* y);

/**
 * @brief Verifica si hay touch activo
 * @return true si hay touch, false en caso contrario
 */
bool touch_is_pressed(void);

#endif // TOUCH_H
