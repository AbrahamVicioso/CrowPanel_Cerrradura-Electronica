/**
 * @file pins.h
 * @brief Definición de pines del hardware
 */

#ifndef PINS_H
#define PINS_H

// ============================================
// PINES DEL SISTEMA
// ============================================

// Pin del relay/cerradura
#define LOCK_PIN 43

// Backlight LCD
#define TFT_BL 2

// ============================================
// CONFIGURACIÓN I2C
// ============================================

// Pines I2C (compartidos entre NFC y Touch)
#define I2C_SDA 19
#define I2C_SCL 20

// Pines específicos NFC
#define NFC_SDA I2C_SDA
#define NFC_SCL I2C_SCL

// ============================================
// PINES LCD RGB (RGB Bus Configuration)
// ============================================

// Pines de datos LCD (D0-D15)
#define LCD_PIN_D0  GPIO_NUM_15  // B0
#define LCD_PIN_D1  GPIO_NUM_7   // B1
#define LCD_PIN_D2  GPIO_NUM_6   // B2
#define LCD_PIN_D3  GPIO_NUM_5   // B3
#define LCD_PIN_D4  GPIO_NUM_4   // B4

#define LCD_PIN_D5  GPIO_NUM_9   // G0
#define LCD_PIN_D6  GPIO_NUM_46  // G1
#define LCD_PIN_D7  GPIO_NUM_3   // G2
#define LCD_PIN_D8  GPIO_NUM_8   // G3
#define LCD_PIN_D9  GPIO_NUM_16  // G4
#define LCD_PIN_D10 GPIO_NUM_1   // G5

#define LCD_PIN_D11 GPIO_NUM_14  // R0
#define LCD_PIN_D12 GPIO_NUM_21  // R1
#define LCD_PIN_D13 GPIO_NUM_47  // R2
#define LCD_PIN_D14 GPIO_NUM_48  // R3
#define LCD_PIN_D15 GPIO_NUM_45  // R4

// Pines de control LCD
#define LCD_PIN_HENABLE GPIO_NUM_41
#define LCD_PIN_VSYNC   GPIO_NUM_40
#define LCD_PIN_HSYNC   GPIO_NUM_39
#define LCD_PIN_PCLK    GPIO_NUM_0

// Frecuencia de escritura LCD
#define LCD_FREQ_WRITE 15000000

// ============================================
// CONFIGURACIÓN LCD (Timing)
// ============================================

// Horizontal sync
#define LCD_HSYNC_POLARITY    0
#define LCD_HSYNC_FRONT_PORCH 40
#define LCD_HSYNC_PULSE_WIDTH 48
#define LCD_HSYNC_BACK_PORCH  40

// Vertical sync
#define LCD_VSYNC_POLARITY    0
#define LCD_VSYNC_FRONT_PORCH 1
#define LCD_VSYNC_PULSE_WIDTH 31
#define LCD_VSYNC_BACK_PORCH  13

// Other LCD settings
#define LCD_PCLK_ACTIVE_NEG 1
#define LCD_DE_IDLE_HIGH    0
#define LCD_PCLK_IDLE_HIGH  0

// ============================================
// CONFIGURACIÓN TOUCH GT911
// ============================================

#define TOUCH_SDA     I2C_SDA
#define TOUCH_SCL     I2C_SCL
#define TOUCH_INT     -1  // Sin interrupt
#define TOUCH_RST     -1  // Sin reset
#define TOUCH_ROTATION 1  // Rotación 90° para corregir mapeo

// Mapping de coordenadas touch
// Pantalla: 800x480 (ancho x alto)
#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 800
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 480

// Intercambiar ejes X e Y del touch
// Deshabilitado porque ya no es necesario
// #define TOUCH_SWAP_XY

#endif // PINS_H
