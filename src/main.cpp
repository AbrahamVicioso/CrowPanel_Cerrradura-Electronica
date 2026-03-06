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
#include "ui/screens/standby_screen.h"
#include "ui/screens/pinpad_screen.h"
#include "ui/screens/unlocked_screen.h"
#include "ui/screens/locked_screen.h"
#include "ui/screens/error_screen.h"

// ============================================
// VARIABLES GLOBALES DE LVGL
// ============================================

static uint32_t screenWidth = DISPLAY_WIDTH;
static uint32_t screenHeight = DISPLAY_HEIGHT;
static lv_disp_draw_buf_t draw_buf;

// Buffers usando PSRAM para evitar fragmentación y mejor performance
// Doble buffer en PSRAM: buffer1 para renderizado activo, buffer2 para DMA
static lv_color_t* disp_draw_buf1;  // Ubicado en PSRAM
static lv_color_t* disp_draw_buf2;  // Ubicado en PSRAM
static lv_disp_drv_t disp_drv;

// Timer para NFC
static lv_timer_t *nfc_timer = nullptr;

// Flags para evitar múltiples activaciones
static volatile bool is_screen_active = false;  // Flag para evitar pantallas concurrentes
static volatile bool is_unlocking = false;       // Flag para evitar desbloqueos simultáneos

// Pantallas
static lv_obj_t* screen_welcome = nullptr;
static lv_obj_t* screen_standby = nullptr;
static lv_obj_t* screen_pinpad = nullptr;
static lv_obj_t* screen_unlocked = nullptr;
static lv_obj_t* screen_locked = nullptr;
static lv_obj_t* screen_error = nullptr;

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
    // Removed delay(5) - it was blocking and unnecessary
}

// ============================================
// CALLBACKS DE EVENTOS (SIN LAMBDAS ANIDADAS)
// ============================================

/**
 * @brief Callback final: volver a standby desde locked
 */
void on_return_to_standby(void)
{
    pinpad_reset();
    display_turn_on();
    lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    is_screen_active = false;  // Liberar el flag al final del flujo
}

/**
 * @brief Callback después de unlock: mostrar locked y volver
 */
void on_lock_after_unlock(void)
{
    lock_lock();
    thingsboard_publish_lock_state(lock_get_state());
    pinpad_reset();
    locked_screen_show(on_return_to_standby);
}

/**
 * @brief Callback cuando el PIN es correcto
 */
void on_pin_success(void)
{
    // Evitar activación si ya está activo
    if (is_screen_active || is_unlocking) return;
    is_screen_active = true;
    is_unlocking = true;

    lock_unlock();
    thingsboard_publish_lock_state(lock_get_state());
    display_turn_on();

    unlocked_screen_show(LOCK_AUTO_LOCK_DELAY_MS, on_lock_after_unlock);

    // Resetear flag de unlocking después de un momento
    is_unlocking = false;
}

/**
 * @brief Callback para volver a standby después de error
 */
void on_error_return_to_standby(void)
{
    pinpad_reset();
    display_turn_on();
    lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    is_screen_active = false;
}

/**
 * @brief Callback cuando el PIN es incorrecto
 */
void on_pin_error(void)
{
    // Evitar activación si ya está activo
    if (is_screen_active) return;
    is_screen_active = true;

    display_turn_on();
    error_screen_show(on_error_return_to_standby, 1500);
}

/**
 * @brief Callback de timer para verificar NFC
 */
void nfc_check_timer_cb(lv_timer_t *timer)
{
    // Evitar activación si ya está activo
    if (is_screen_active || is_unlocking) return;

    NfcTag tag;

    if (nfc_read_tag(&tag))
    {
        is_screen_active = true;
        is_unlocking = true;

        lock_unlock();
        thingsboard_publish_lock_state(lock_get_state());
        display_turn_on();

        unlocked_screen_show(LOCK_AUTO_LOCK_DELAY_MS, on_lock_after_unlock);

        // Resetear flag de unlocking después de un momento
        is_unlocking = false;
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
    Serial.print("SSID: ");
    Serial.println(WIFI_SSID);
    
    WiFi.mode(WIFI_STA);
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
        Serial.print("WiFi conectado! IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
        Serial.print("DNS: ");
        Serial.println(WiFi.dnsIP());
    }
    else
    {
        Serial.println("");
        Serial.print("Error WiFi. Estado: ");
        Serial.println(WiFi.status());
        // Posibles errores:
        // 0 = WL_IDLE_STATUS
        // 1 = WL_NO_SSID_AVAIL
        // 2 = WL_SCAN_COMPLETED
        // 3 = WL_CONNECTED
        // 4 = WL_CONNECT_FAILED
        // 5 = WL_CONNECTION_LOST
        // 6 = WL_DISCONNECTED
        switch(WiFi.status()) {
            case WL_NO_SSID_AVAIL:
                Serial.println("Red WiFi no encontrada!");
                break;
            case WL_CONNECT_FAILED:
                Serial.println("Contraseña incorrecta!");
                break;
            case WL_CONNECTION_LOST:
                Serial.println("Conexion perdida!");
                break;
            case WL_DISCONNECTED:
                Serial.println("Desconectado!");
                break;
        }
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
    
    // Debug: Mostrar estado WiFi
    Serial.print("Estado WiFi: ");
    Serial.println(WiFi.status());
    if (WiFi.status() == WL_CONNECTED) {
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.print("Gateway: ");
        Serial.println(WiFi.gatewayIP());
    }
    
    // Configurar timezone y sincronizar hora (solo si hay WiFi)
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Configurando zona horaria (Republica Dominicana)...");
        setenv("TZ", "America/Santo_Domingo", 1);
        tzset();
        // Usar servidores NTP de Caribe/América Latina
        configTime(-4 * 3600, 0, "pool.ntp.org", "time.google.com");
        
        // Sincronizar hora (esperar hasta 5 segundos)
        Serial.println("Sincronizando hora...");
        struct tm timeinfo;
        int retry = 0;
        while (getLocalTime(&timeinfo) == false && retry < 10) {
            delay(500);
            Serial.print(".");
            retry++;
        }
        if (getLocalTime(&timeinfo)) {
            Serial.println("\nHora sincronizada: " + String(timeinfo.tm_hour) + ":" + String(timeinfo.tm_min));
        } else {
            Serial.println("\nNo se pudo sincronizar la hora");
        }
    } else {
        Serial.println("WiFi no conectado - usando hora del sistema");
    }
    
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

    // Alocar buffers en PSRAM para mejor rendimiento y evitar fragmentación
    // Buffer size: 1/5 de pantalla para balance entre memoria y velocidad
    uint32_t buf_size = screenWidth * screenHeight / 5;
    Serial.print("Alocando buffers en PSRAM: ");
    Serial.print(buf_size * sizeof(lv_color_t) * 2);
    Serial.println(" bytes");

    disp_draw_buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    disp_draw_buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (!disp_draw_buf1 || !disp_draw_buf2) {
        Serial.println("ERROR: No se pudo alocar buffers en PSRAM!");
        Serial.println("Intentando usar memoria interna...");
        // Fallback a memoria interna con buffer más pequeño
        if (!disp_draw_buf1) disp_draw_buf1 = (lv_color_t*)malloc(buf_size / 4 * sizeof(lv_color_t));
        if (!disp_draw_buf2) disp_draw_buf2 = (lv_color_t*)malloc(buf_size / 4 * sizeof(lv_color_t));
        buf_size = buf_size / 4;
    }

    // Configurar doble buffer para rendering sin bloqueos
    lv_disp_draw_buf_init(&draw_buf, disp_draw_buf1, disp_draw_buf2, buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 0;  // Permitir partial refresh para mejor performance
    lv_disp_drv_register(&disp_drv);

    Serial.println("Buffers LVGL configurados correctamente");
    
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
    screen_standby = standby_screen_create();
    screen_pinpad = pinpad_screen_create();
    screen_unlocked = unlocked_screen_create();
    screen_locked = locked_screen_create();
    screen_error = error_screen_create();
    
    // Configurar callback de tap en standby
    standby_screen_set_tap_callback([]() {
        display_turn_on();  // Asegurar que el backlight estéendido
        // Usar pinpad_screen_show para iniciar el timer de inactividad
        pinpad_screen_show(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0);
    });
    
    // Registrar callbacks
    pinpad_set_success_callback(on_pin_success);
    pinpad_set_error_callback(on_pin_error);
    
    // Callback de inactividad (volver a standby después de 15 seg sin actividad)
    pinpad_set_inactivity_callback([]() {
        display_turn_on();
        lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
    });
    
    // Iniciar animación de bienvenida -> standby
    welcome_screen_animate_to(screen_standby);
    
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
    // CRITICAL: Verificar memoria disponible para evitar crash
    static uint32_t last_mem_check = 0;
    if (millis() - last_mem_check > 5000) {  // Check cada 5 segundos
        uint32_t free_heap = ESP.getFreeHeap();
        if (free_heap < 10000) {  // Menos de 10KB libre = PELIGRO
            Serial.print("WARNING: Low memory! Free heap: ");
            Serial.println(free_heap);
            // Resetear flags para intentar recuperar
            is_screen_active = false;
            is_unlocking = false;
        }
        last_mem_check = millis();
    }

    // OPTIMIZACIÓN: Procesar LVGL con máxima prioridad
    // lv_timer_handler() retorna tiempo hasta el próximo timer
    uint32_t time_till_next = lv_timer_handler();

    // Procesar ThingsBoard solo si hay conexión (menos frecuente que LVGL)
    static uint32_t last_tb_check = 0;
    if (WiFi.status() == WL_CONNECTED && (millis() - last_tb_check > 100))
    {
        thingsboard_loop();
        last_tb_check = millis();
    }

    // Delay adaptativo basado en el próximo evento LVGL
    // Esto minimiza latencia mientras permite que otras tareas funcionen
    if (time_till_next > 5) {
        delay(2);  // Delay mínimo para no saturar el CPU
    }

    // Watchdog - resetear si hay un watchdog habilitado
    yield();  // Dar tiempo a otras tareas del sistema
}
