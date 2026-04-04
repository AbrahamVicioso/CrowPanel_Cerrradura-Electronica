#ifndef INFO_SCREEN_H
#define INFO_SCREEN_H

#include <lvgl.h>
#include <Arduino.h>

/**
 * @brief Muestra la pantalla de información del dispositivo.
 *        Se activa con long press (~2s) en la pantalla standby.
 *        Toca cualquier parte para cerrar.
 *
 * @param tbConnected   true si ThingsBoard está conectado
 * @param nfcEnabled    true si NFC está habilitado
 * @param on_close      callback llamado al cerrar (toque o timeout)
 */
void info_screen_show(bool tbConnected, bool nfcEnabled, void (*on_close)(void));

#endif // INFO_SCREEN_H
