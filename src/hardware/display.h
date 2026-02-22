/**
 * @file display.h
 * @brief Módulo de control del display LCD
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>

// Dimensiones del display
#define DISPLAY_WIDTH  800
#define DISPLAY_HEIGHT 480

/**
 * @brief Clase del display LCD RGB
 */
class LGFX : public lgfx::LGFX_Device
{
public:
    lgfx::Bus_RGB     _bus_instance;
    lgfx::Panel_RGB   _panel_instance;

    LGFX(void);
};

// Instancia global del display
extern LGFX lcd;

/**
 * @brief Inicializa el display LCD
 */
void display_init(void);

/**
 * @brief Configura el backlight del display
 * @param brightness Brillo (0-255)
 */
void display_set_brightness(uint8_t brightness);

/**
 * @brief Apaga el backlight
 */
void display_turn_off(void);

/**
 * @brief Enciende el backlight
 */
void display_turn_on(void);

#endif // DISPLAY_H
