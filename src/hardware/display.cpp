/**
 * @file display.cpp
 * @brief Implementación del módulo de control del display LCD
 */

#include "display.h"
#include "../config/pins.h"

// ============================================
// IMPLEMENTACIÓN DE LA CLASE LGFX
// ============================================

LGFX::LGFX(void)
{
    {
        auto cfg = _bus_instance.config();
        cfg.panel = &_panel_instance;

        // Data pins (D0-D15)
        cfg.pin_d0  = LCD_PIN_D0;
        cfg.pin_d1  = LCD_PIN_D1;
        cfg.pin_d2  = LCD_PIN_D2;
        cfg.pin_d3  = LCD_PIN_D3;
        cfg.pin_d4  = LCD_PIN_D4;

        cfg.pin_d5  = LCD_PIN_D5;
        cfg.pin_d6  = LCD_PIN_D6;
        cfg.pin_d7  = LCD_PIN_D7;
        cfg.pin_d8  = LCD_PIN_D8;
        cfg.pin_d9  = LCD_PIN_D9;
        cfg.pin_d10 = LCD_PIN_D10;

        cfg.pin_d11 = LCD_PIN_D11;
        cfg.pin_d12 = LCD_PIN_D12;
        cfg.pin_d13 = LCD_PIN_D13;
        cfg.pin_d14 = LCD_PIN_D14;
        cfg.pin_d15 = LCD_PIN_D15;

        // Control pins
        cfg.pin_henable = LCD_PIN_HENABLE;
        cfg.pin_vsync   = LCD_PIN_VSYNC;
        cfg.pin_hsync   = LCD_PIN_HSYNC;
        cfg.pin_pclk    = LCD_PIN_PCLK;
        cfg.freq_write = LCD_FREQ_WRITE;

        // Horizontal sync timing
        cfg.hsync_polarity    = LCD_HSYNC_POLARITY;
        cfg.hsync_front_porch = LCD_HSYNC_FRONT_PORCH;
        cfg.hsync_pulse_width = LCD_HSYNC_PULSE_WIDTH;
        cfg.hsync_back_porch  = LCD_HSYNC_BACK_PORCH;

        // Vertical sync timing
        cfg.vsync_polarity    = LCD_VSYNC_POLARITY;
        cfg.vsync_front_porch = LCD_VSYNC_FRONT_PORCH;
        cfg.vsync_pulse_width = LCD_VSYNC_PULSE_WIDTH;
        cfg.vsync_back_porch  = LCD_VSYNC_BACK_PORCH;

        // Other settings
        cfg.pclk_active_neg   = LCD_PCLK_ACTIVE_NEG;
        cfg.de_idle_high      = LCD_DE_IDLE_HIGH;
        cfg.pclk_idle_high    = LCD_PCLK_IDLE_HIGH;

        _bus_instance.config(cfg);
    }
    {
        auto cfg = _panel_instance.config();
        cfg.memory_width  = DISPLAY_WIDTH;
        cfg.memory_height = DISPLAY_HEIGHT;
        cfg.panel_width   = DISPLAY_WIDTH;
        cfg.panel_height  = DISPLAY_HEIGHT;
        cfg.offset_x = 0;
        cfg.offset_y = 0;
        _panel_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
}

LGFX lcd;

// ============================================
// FUNCIONES PÚBLICAS
// ============================================

void display_init(void)
{
    lcd.begin();
    lcd.fillScreen(TFT_BLACK);
    delay(100);
    
    Serial.println("Display LCD inicializado correctamente");
}

void display_set_brightness(uint8_t brightness)
{
    ledcSetup(1, 300, 8);
    ledcAttachPin(TFT_BL, 1);
    ledcWrite(1, brightness);
}

void display_turn_off(void)
{
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, LOW);
}

void display_turn_on(void)
{
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
}
