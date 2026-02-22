/**
 * @file main.cpp
 * @brief Sistema de Cerradura Electrónica - Punto de entrada principal
 * 
 * Este proyecto implementa un sistema de control de acceso con:
 * - Interfaz táctil con PIN
 * - Lector NFC
 * - Integración con ThingsBoard (MQTT)
 * - Control de cerradura electrónica
 */

#include <Arduino.h>
#include <lvgl.h>
#include <Wire.h>

// Módulos del sistema
#include "config/config.h"
#include "config/pins.h"
#include "config/network_config.h"

#include "hardware/display.h"
#include "hardware/touch.h"
#include "hardware/nfc.h"
#include "hardware/lock.h"

#include "communication/thingsboard.h"

#include "ui/theme.h"
#include "ui/screens/welcome_screen.h"
#include "ui/screens/pinpad_screen.h"
#include "ui/screens/unlocked_screen.h"

// ============================================
// VARIABLES GLOBALES DE LVGL
// ============================================

static uint32_t screenWidth = DISPLAY_WIDTH;
static uint32_t screenHeight = DISPLAY_HEIGHT;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf[DISPLAY_WIDTH * DISPLAY_HEIGHT / 15];
static lv_disp_drv_t disp_drv;

// Timer para NFC
static lv_timer_t *nfc_timer = nullptr;

// Pantallas
static lv_obj_t* screen_welcome = nullptr;
static lv_obj_t* screen_pinpad = nullptr;
static lv_obj_t* screen_unlocked = nullptr;

// ============================================
// CALLBACKS DE LVGL
// ============================================

/**
 * @brief Callback de flush del display
 */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)&color_p->full);
    lv_disp_flush_ready(disp);
}

/**
 * @brief Callback de lectura del touchpad
 */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    ts.read();
    if (ts.isTouched)
    {
        data->state = LV_INDEV_STATE_PR;
        
        // Mapping de coordenadas con soporte para swap
        #ifdef TOUCH_SWAP_XY
        data->point.x = map(ts.points[0].y, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width() - 1);
        data->point.y = map(ts.points[0].x, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
        #else
        data->point.x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width() - 1);
        data->point.y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
        #endif
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    delay(5);
}

// ============================================
// CALLBACKS DE EVENTOS
// ============================================

/**
 * @brief Callback cuando el PIN es correcto
 */
void on_pin_success(void)
{
    lock_unlock();
    thingsboard_publish_lock_state(lock_get_state());
    
    unlocked_screen_show(LOCK_AUTO_LOCK_DELAY_MS, []() {
        lock_lock();
        thingsboard_publish_lock_state(lock_get_state());
        pinpad_reset();
        lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
    });
}

/**
 * @brief Callback cuando el PIN es incorrecto
 */
void on_pin_error(void)
{
    // El manejo ya se hace en pinpad_screen.cpp
}

/**
 * @brief Callback de timer para verificar NFC
 */
void nfc_check_timer_cb(lv_timer_t *timer)
{
    NfcTag tag;
    
    if (nfc_read_tag(&tag))
    {
        // Cualquier tarjeta NFC detected - desbloquear
        lock_unlock();
        thingsboard_publish_lock_state(lock_get_state());
        
        unlocked_screen_show(LOCK_AUTO_LOCK_DELAY_MS, []() {
            lock_lock();
            thingsboard_publish_lock_state(lock_get_state());
            pinpad_reset();
            lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
        });
    }
}

// ============================================
// FUNCIONES DE CONEXIÓN WIFI
// ============================================

/**
 * @brief Conecta a WiFi
 */
void connect_wifi(void)
{
    Serial.println("Conectando a WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("");
        Serial.print("WiFi conectado. IP: ");
        Serial.println(WiFi.localIP());
    }
    else
    {
        Serial.println("");
        Serial.println("Error: No se pudo conectar a WiFi");
    }
}

// ============================================
// SETUP Y LOOP
// ============================================

void setup()
{
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("Sistema de Cerradura Electronica v" FIRMWARE_VERSION);
    Serial.println("========================================");
    
    // 1. Inicializar hardware
    Serial.println("\n[1/6] Inicializando hardware...");
    lock_init();
    
    // 2. Inicializar display
    Serial.println("[2/6] Inicializando display...");
    display_init();
    
    // 3. Conectar WiFi
    Serial.println("[3/6] Conectando a red...");
    connect_wifi();
    
    // 4. Inicializar ThingsBoard
    Serial.println("[4/6] Inicializando ThingsBoard...");
    if (WiFi.status() == WL_CONNECTED) {
        thingsboard_init();
        thingsboard_connect();
    }
    
    // 5. Inicializar LVGL
    Serial.println("[5/6] Inicializando LVGL...");
    lv_init();
    
    // Inicializar touch
    Wire.begin(TOUCH_SDA, TOUCH_SCL);
    ts.begin();
    ts.setRotation(TOUCH_ROTATION);
    
    // Configurar display buffer
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 15);
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    
    // Configurar input device
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    
    // Configurar backlight
    display_turn_on();
    
    // 6. Crear UI
    Serial.println("[6/6] Creando interfaz de usuario...");
    
    // Inicializar estilos
    theme_init_styles();
    
    // Crear pantallas
    screen_welcome = welcome_screen_create();
    screen_pinpad = pinpad_screen_create();
    screen_unlocked = unlocked_screen_create();
    
    // Registrar callbacks
    pinpad_set_success_callback(on_pin_success);
    pinpad_set_error_callback(on_pin_error);
    
    // Iniciar animación de bienvenida
    welcome_screen_animate_to(screen_pinpad);
    
    // Inicializar NFC
    nfc_init();
    
    // Timer para chequear NFC cada segundo
    nfc_timer = lv_timer_create(nfc_check_timer_cb, NFC_CHECK_INTERVAL_MS, NULL);
    
    Serial.println("\n========================================");
    Serial.println("Sistema iniciado correctamente!");
    Serial.println("PIN correcto: " DEFAULT_PIN);
    Serial.println("========================================\n");
}

void loop()
{
    // Procesar ThingsBoard
    if (WiFi.status() == WL_CONNECTED)
    {
        thingsboard_loop();
    }
    
    // Procesar LVGL
    lv_timer_handler();
    delay(1);
}
