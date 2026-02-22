/**
 * @file touch.cpp
 * @brief Implementación del módulo de control del touchscreen
 */

#include "touch.h"
#include "../config/pins.h"
#include <Wire.h>
#include "display.h"

// Instancia global del touchscreen
TAMC_GT911 ts = TAMC_GT911(
    TOUCH_SDA, 
    TOUCH_SCL, 
    TOUCH_INT, 
    TOUCH_RST,
    max(TOUCH_MAP_X1, TOUCH_MAP_X2), 
    max(TOUCH_MAP_Y1, TOUCH_MAP_Y2)
);

void touch_init(void)
{
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    ts.begin();
    ts.setRotation(TOUCH_ROTATION);
    
    Serial.println("Touchscreen GT911 inicializado correctamente");
}

bool touch_read(int* x, int* y)
{
    ts.read();
    
    if (ts.isTouched) {
        // Map coordinates based on configuration
        #ifdef TOUCH_SWAP_XY
        *x = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width() - 1);
        *y = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
        #else
        *x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width() - 1);
        *y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
        #endif
        return true;
    }
    
    return false;
}

bool touch_is_pressed(void)
{
    ts.read();
    return ts.isTouched;
}
