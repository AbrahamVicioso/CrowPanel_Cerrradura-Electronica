# CLAUDE.md - CrowPanel Electronic Lock System v2.0

## 🤖 AI Context File

Este archivo proporciona contexto completo para IA sobre el proyecto CrowPanel Electronic Lock System. Incluye toda la información técnica necesaria para entender, mantener y desarrollar el proyecto.

## 📋 Información General del Proyecto

**Nombre**: CrowPanel Electronic Lock System
**Versión**: 2.0.0
**Framework**: PlatformIO + ESP32 Arduino
**Microcontrolador**: ESP32-S3
**Pantalla**: RGB LCD 800×480 con touchscreen capacitivo
**Lenguaje Principal**: C++ (Arduino framework)

### 🎯 Objetivo del Sistema
Sistema de cerradura electrónica inteligente con:
- Autenticación múltiple (PIN + NFC)
- Control remoto via ThingsBoard IoT
- Interfaz táctil moderna
- Seguridad configurable
- Persistencia de configuración

## 🏗️ Arquitectura del Sistema

### Estructura de Directorios
```
CrowPanel_Cerrradura-Electronica/
├── platformio.ini                 # Configuración PlatformIO
├── huge_app.csv                   # Partición custom ESP32
├── esp32-s3-devkitc-1-myboard.json # Board definition
│
├── src/
│   ├── main.cpp                   # Entry point principal
│   │
│   ├── config/                    # Configuración del sistema
│   │   ├── config.h              # Constantes globales
│   │   ├── pins.h                # Definición de pines GPIO
│   │   ├── storage.h/.cpp        # NVS persistence (Preferences)
│   │   ├── credentials.h/.cpp    # Módulo de credenciales multi-PIN (desde TB)
│   │   ├── network_config.h/.cpp # WiFi & ThingsBoard
│   │   └── portal.h/.cpp         # Web portal de configuración
│   │
│   ├── hardware/                  # Drivers de hardware
│   │   ├── display.h/.cpp        # LCD RGB + backlight
│   │   ├── touch.h/.cpp          # GT911 touchscreen
│   │   ├── nfc.h/.cpp            # PN532 RFID/NFC
│   │   └── lock.h/.cpp           # Control relay cerradura
│   │
│   ├── communication/             # Conectividad IoT
│   │   └── thingsboard.h/.cpp    # MQTT ThingsBoard client
│   │
│   └── ui/                       # Interfaz gráfica LVGL
│       ├── theme.h/.cpp          # Estilos y colores
│       └── screens/              # Pantallas del sistema
│           ├── welcome_screen.h/.cpp
│           ├── standby_screen.h/.cpp
│           ├── pinpad_screen.h/.cpp
│           ├── unlocked_screen.h/.cpp
│           ├── locked_screen.h/.cpp
│           ├── error_screen.h/.cpp
│           ├── config_screen.h/.cpp
│           └── info_screen.h/.cpp
│
├── include/                      # Headers LVGL generados
│   ├── lv_conf.h                # Configuración LVGL
│   └── ui*.h                    # UI components
│
├── lib/                         # Librerías locales
└── test/                        # Tests (si existen)
```

### Flujo de Datos Principal
```
[Hardware Interrupts] → [LVGL Touch Events] → [UI Callbacks]
                                    ↓
[User Input] → [PIN/NFC Validation] → [Lock Control]
                                    ↓
[State Changes] → [ThingsBoard MQTT] → [Cloud Dashboard]
                                    ↓
[Telemetry] → [NVS Storage] → [Persistent Config]
```

## 🔧 Configuración Técnica

### PlatformIO Environment
```ini
[env:esp32-s3-devkitc-1-myboard]
platform = espressif32
board = esp32-s3-devkitc-1-myboard
framework = arduino
platform_packages = framework-arduinoespressif32 @ https://github.com/espressif/arduino-esp32#2.0.3

board_build.f_cpu = 240000000L
board_build.f_flash = 80000000L
board_build.flash_mode = qio

build_flags =
    -D LV_LVGL_H_INCLUDE_SIMPLE
    -I./include
    -I./src
    -D LV_COLOR_DEPTH=16
    -D LV_COLOR_16_SWAP=0
    -O2
    -pipe
    -ffunction-sections
    -fdata-sections
    -Wl,--gc-sections
    -D CONFIG_ESPTOOLPY_FLASHSIZE_4MB
    -D CONFIG_PARTITION_TABLE_CUSTOM
    -D BOARD_HAS_PSRAM

lib_deps =
    lvgl/lvgl@8.3.6
    tamctec/TAMC_GT911@^1.0.2
    moononournation/GFX Library for Arduino@1.2.8
    lovyan03/LovyanGFX@^1.1.12
    maxpromer/PCA9557-arduino@^1.0.0
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/Adafruit PN532@^1.3.1
    thingsboard/ThingsBoard@^0.15.0
    thingsboard/TBPubSubClient@^2.12.1

board_build.partitions = huge_app.csv
extra_scripts = scripts/limit_jobs.py
```

### Constantes Globales (config.h)
```cpp
#define FIRMWARE_VERSION "2.0.0"
#define FIRMWARE_NAME   "Electronic Lock System"

// Cerradura
#define LOCK_DEFAULT_STATE          false
#define LOCK_AUTO_LOCK_DELAY_MS     5000
#define LOCK_AUTO_LOCK_DELAY_MIN    2000
#define LOCK_AUTO_LOCK_DELAY_MAX    60000

// PIN
#define PIN_LENGTH      6
#define DEFAULT_PIN     "123456"

// Seguridad
#define MAX_FAILED_ATTEMPTS         3
#define LOCKOUT_DURATION_MS         30000
#define LOCKOUT_DURATION_MIN_S      10
#define LOCKOUT_DURATION_MAX_S      300

// Tiempos
#define WIFI_CONNECT_TIMEOUT_MS             15000
#define THINGSBOARD_RECONNECT_INTERVAL_MS   5000
#define NFC_CHECK_INTERVAL_MS               1000

// Pantalla
#define DISPLAY_WIDTH    800
#define DISPLAY_HEIGHT   480
```

### Pines GPIO (pins.h)
```cpp
// Cerradura
#define LOCK_PIN 43

// Backlight LCD
#define TFT_BL 2

// I2C (compartido)
#define I2C_SDA 19
#define I2C_SCL 20

// NFC (compartido con touch)
#define NFC_SDA I2C_SDA
#define NFC_SCL I2C_SCL

// LCD RGB bus
#define LCD_PIN_D0  GPIO_NUM_15  // B0
#define LCD_PIN_D1  GPIO_NUM_7   // B1
// ... (D2-D15 definidos)

// LCD control
#define LCD_PIN_HENABLE GPIO_NUM_41
#define LCD_PIN_VSYNC   GPIO_NUM_40
#define LCD_PIN_HSYNC   GPIO_NUM_39
#define LCD_PIN_PCLK    GPIO_NUM_0

// Touch GT911
#define TOUCH_SDA     I2C_SDA
#define TOUCH_SCL     I2C_SCL
#define TOUCH_INT     -1
#define TOUCH_RST     -1
#define TOUCH_ROTATION 1
```

## 💻 Convenciones de Código

### Estilo C++
- **Nombres de archivos**: snake_case (ej: `pinpad_screen.cpp`)
- **Clases/Structs**: PascalCase (ej: `NfcTag`)
- **Funciones**: snake_case (ej: `nfc_read_tag()`)
- **Variables**: snake_case (ej: `current_pin`)
- **Constantes**: UPPER_SNAKE_CASE (ej: `DEFAULT_PIN`)
- **Macros**: UPPER_SNAKE_CASE (ej: `LOCK_PIN`)

### Patrón de Callbacks
```cpp
// Definición de callback type
typedef void (*PinSuccessCallback)(void);

// Registro de callback
void pinpad_set_success_callback(PinSuccessCallback cb) {
    success_cb = cb;
}

// Uso del callback
if (success_cb) success_cb();
```

### Manejo de Estados
```cpp
// Estados booleanos globales
static volatile bool screen_busy = false;
static volatile bool is_unlocking = false;

// Control de concurrencia
if (screen_busy || is_unlocking) return;

// Marcar operación en progreso
screen_busy = true;
// ... operación
screen_busy = false;
```

### Gestión de Memoria LVGL
```cpp
// Buffers en PSRAM para mejor rendimiento
static lv_color_t* disp_buf1;
static lv_color_t* disp_buf2;

disp_buf1 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
disp_buf2 = (lv_color_t*)heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

// Fallback a DRAM si PSRAM no disponible
if (!disp_buf1) {
    disp_buf1 = (lv_color_t*)malloc(buf_size * sizeof(lv_color_t));
}
```

## 🔄 Ciclo Principal (main.cpp)

### Setup Sequence
```cpp
void setup() {
    Serial.begin(115200);

    // 1. Storage NVS - Cargar configuración persistente
    storage_init();

    // 2. Hardware - Inicializar relay cerradura
    lock_init();

    // 3. Display - Configurar LCD RGB
    display_init();

    // 4. WiFi + NTP - Conectar red y sincronizar hora
    connect_wifi();
    configTime(-4 * 3600, 0, "pool.ntp.org", "time.google.com");

    // 5. ThingsBoard - Configurar MQTT client
    thingsboard_init();
    thingsboard_connect();

    // 6. LVGL - Inicializar UI framework
    lv_init();
    setup_lvgl_display();
    setup_lvgl_touch();

    // 7. NFC - Inicializar lector RFID
    nfc_init();

    // Crear pantallas UI
    create_all_screens();

    // Configurar callbacks
    setup_callbacks();

    // Iniciar flujo: welcome → standby
    welcome_screen_animate_to(screen_standby);
}
```

### Loop Principal
```cpp
void loop() {
    // LVGL tiene máxima prioridad
    uint32_t next = lv_timer_handler();

    // Portal de configuración (modo AP)
    if (portal_is_active()) {
        portal_loop();
        yield();
        return;
    }

    // ThingsBoard MQTT loop
    if (WiFi.status() == WL_CONNECTED) {
        thingsboard_loop();
    }

    // Actualizar indicadores de estado
    update_status_indicators();

    // Watchdog de memoria
    memory_watchdog();

    // Delay adaptativo
    if (next > 5) delay(2);
    yield();
}
```

## 🌐 ThingsBoard IoT Integration

### Shared Attributes (Configuración Remota)
```cpp
// Atributos suscritos (6 total) — lockState eliminado, ahora es RPC
static const char* ATTR_CREDENTIALS = "credenciales";   // objeto JSON con huespedes + personal
static const char* ATTR_AUTO_LOCK_DELAY = "autoLockDelay";
static const char* ATTR_NFC_ENABLED = "nfcEnabled";
static const char* ATTR_NFC_UID = "authorizedNfcUid";
static const char* ATTR_MAX_FAILED = "maxFailedAttempts";
static const char* ATTR_LOCKOUT_DURATION = "lockoutDuration";
```

### Estructura del atributo "credenciales"
```json
{
  "huespedes": [
    {
      "huespedId": 1, "reservaId": 3002,
      "credenciales": [
        {"pin":"142820","activacion":"2026-04-17T22:54:18","expiracion":"2026-04-18T18:53:00"}
      ]
    }
  ],
  "personal": [
    {
      "personalId": 1, "nombre": "Abraham Vicioso",
      "credenciales": [
        {"pin":"805714","activacion":"2026-04-18T00:31:00","expiracion":"2026-05-01T00:31:00"}
      ]
    }
  ]
}
```
- Cada PIN tiene `activacion` y `expiracion` — **el servidor envía hora local de República Dominicana (AST, UTC-4) SIN sufijo 'Z'**
- `parse_iso8601` detecta sufijo 'Z': con 'Z' → UTC (algoritmo Hinnant); sin 'Z' → hora local (mktime, respeta TZ="AST4")
- El parse en `credentials.cpp` usa `DynamicJsonDocument(16384)` en DRAM
- **NO usar `#include <esp_heap_caps.h>`** — es header pesado IDF que causa CreateProcess en Windows MSYS2
- TB puede enviar el valor como string escapado o como JSON nativo — ambos soportados
- Constructor ThingsBoard: `ThingsBoardSized<128> tb(mqttClient, receive=16384, send=2048, stack=2048, apis)` — receive PRIMERO, send SEGUNDO (orden fácil de confundir)
- **`Shared_Attribute_Update`** solo recibe actualizaciones FUTURAS. Para obtener valores actuales al conectar, usar **`Attribute_Request::Shared_Attributes_Request(cb)`** — CRÍTICO al reconectar para recibir credenciales/config ya guardadas en TB
- Receive buffer 8192: soporta JSON de credenciales grandes sin truncamiento
- Send buffer 1024: `Send_Json_String` valida `send_buffer_size < json_size` y retorna false silenciosamente si el payload es muy grande — el accessEvent con credencial puede llegar a ~512 bytes, requiere al menos 1024
- **REGLA**: iteración sobre JsonArrayConst → usar `for (JsonVariantConst v : arr)` + `v.as<JsonObjectConst>()` explícito (la conversión implícita `for (JsonObjectConst obj : arr)` puede silenciosamente retornar null en ArduinoJson 6.21.5)

### RPC Server-side (Control Remoto)
Implementados 3 métodos RPC:
| Método          | Params                        | Descripción               |
|-----------------|-------------------------------|---------------------------|
| unlock          | `{"lockState": "unlocked"}`   | Desbloquear puerta        |
| unlockTemporary | `{"duration": 5000}`          | Desbloquear N ms (1s-5min)|
| resetLockout    | —                             | (stub, lockout eliminado) |

### Telemetría
```cpp
// Publicar estado de cerradura
thingsboard_publish_lock_state(lock_get_state());

// Publicar evento de acceso
thingsboard_publish_access_event(granted, method);

// Publicar intentos fallidos
thingsboard_publish_failed_attempts(count);
```

## 🎨 Sistema de UI (LVGL)

### Tema y Estilos
```cpp
// Paleta de colores
#define COLOR_PRIMARY    lv_color_hex(0x1a1a2e)  // Azul oscuro profundo
#define COLOR_SECONDARY  lv_color_hex(0x16213e)  // Azul marino
#define COLOR_ACCENT     lv_color_hex(0x0f3460)  // Azul medio
#define COLOR_SUCCESS    lv_color_hex(0x00b894)  // Verde esmeralda
#define COLOR_ERROR      lv_color_hex(0xd63031)  // Rojo elegante
#define COLOR_TEXT       lv_color_hex(0xecf0f1)  // Gris claro
#define COLOR_BUTTON     lv_color_hex(0x2c3e50)  // Gris azulado

// Estilos globales
static lv_style_t style_bg;
static lv_style_t style_btn;
static lv_style_t style_label;

void theme_init_styles() {
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, COLOR_PRIMARY);
    lv_style_set_text_color(&style_bg, COLOR_TEXT);
    // ... más estilos
}
```

### Pantallas del Sistema
```cpp
// Tipos de pantalla
static lv_obj_t* screen_welcome;
static lv_obj_t* screen_standby;
static lv_obj_t* screen_pinpad;
static lv_obj_t* screen_unlocked;
static lv_obj_t* screen_locked;
static lv_obj_t* screen_error;
static lv_obj_t* screen_config;

// Navegación entre pantallas
lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_NONE, 0, 0, false);
lv_scr_load_anim(screen_standby, LV_SCR_LOAD_ANIM_FADE_ON, 200, 0, false);
```

### Callbacks de UI
```cpp
// Callback de botón PIN
static void pin_button_event_cb(lv_event_t* e) {
    lv_obj_t* btn = lv_event_get_target(e);
    const char* txt = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (strcmp(txt, "⌫") == 0) {
        pinpad_backspace();
    } else {
        pinpad_append_digit(txt[0]);
    }
}
```

## 🔒 Sistema de Seguridad

### Validación PIN — via credentials module
```cpp
// pinpad_screen.cpp usa un callback validador en lugar de PIN fijo
static PinValidatorCallback pin_validator = nullptr;
void pinpad_set_pin_validator(PinValidatorCallback validator);

// credentials.cpp valida contra credenciales cargadas desde TB
bool credentials_validate_pin(const String& pin);
// - Tanto huéspedes como personal tienen activacion + expiracion (ambas UTC)
// - Si activacion != 0 y now < activacion → DENEGADO (no activo aún)
// - Si expiracion != 0 y now > expiracion → DENEGADO (expirado)
// - Si NTP no sincronizado (now < 1700000000): permite si PIN coincide (warning en serial)
// - Al boot: carga credenciales previas desde NVS blob para funcionar sin internet
```

### Sistema de Lockout
**ELIMINADO** — intentos fallidos no bloquean la pantalla.
`pinpad_is_locked_out()` siempre devuelve `false`.

### NFC Authorization
```cpp
// Estructura para tag NFC
typedef struct {
    uint8_t uid[10];
    uint8_t uidLength;
    uint8_t sak;
    uint8_t atqa[2];
} NfcTag;

// Verificación de UID autorizado
bool nfc_is_authorized(const NfcTag* tag) {
    if (!nfc_enabled) return false;

    String tagUid = nfc_tag_to_string(tag);
    return (tagUid == authorized_uid);
}
```

## 💾 Sistema de Almacenamiento

### Credenciales — NVS blob (`"creds"`)
Las credenciales se guardan como blob NVS usando `putBytes`/`getBytes` (ya disponible en `Preferences.h`).
- `storage_set_credentials(json)` → `prefs.putBytes("creds", json, len)`
- `storage_get_credentials()` → `prefs.getBytes("creds", buf, len)` → `String`
- Límite práctico: ~14 KB (partición NVS 20 KB minus overhead)
- **NO usar `#include <SPIFFS.h>`** en storage.cpp — es header pesado que LDF agrega a TODOS los archivos fuente, alargando las líneas de comando de SCons más allá del límite de Windows CreateProcess

### Configuración — NVS
Namespace: `"lock_cfg"` (Preferences library)

| Clave NVS    | Función                              |
|--------------|--------------------------------------|
| `"nfc_uid"`  | UID NFC autorizado (hex string)      |
| `"max_fail"` | Máx. intentos fallidos (int)         |
| `"auto_lock"`| Delay auto-bloqueo ms (uint)         |
| `"nfc_en"`   | NFC habilitado (bool)                |
| `"lockout_s"`| Duración lockout en segundos (int)   |
| `"wifi_ssid"`| SSID WiFi (string)                   |
| `"wifi_pass"`| Contraseña WiFi (string)             |
| `"tb_server"`| Servidor ThingsBoard (string)        |
| `"tb_token"` | Token de acceso TB (string)          |
| `"ap_pass"`  | Contraseña del portal web (string)   |

### Funciones de Storage
```cpp
void storage_init() {
    prefs.begin("lock_cfg", false);  // SIN SPIFFS — no incluir SPIFFS.h
}

// Credenciales: NVS blob (putBytes/getBytes), NO SPIFFS
String storage_get_credentials();           // lee blob "creds" → String JSON
void   storage_set_credentials(const char*); // escribe blob "creds"
```

## 🔌 Portal de Configuración Web

### Modo AP (portal.cpp)
```cpp
// SSID fijo del AP
#define PORTAL_AP_SSID "CerraduraConfig"

// Servidor: WiFiServer raw (NO usar WebServer.h — causa errores CreateProcess en Windows)
static WiFiServer httpServer(80);
// Sin DNSServer — el usuario navega directo a 192.168.4.1

// Dos contraseñas separadas:
//   - WiFi AP: aleatoria cada sesión (8 dígitos, mostrada en pantalla del device)
//   - Portal web: fija, guardada en NVS ("ap_pass"), default "admin1234"
```

### Endpoints HTTP (raw, sin framework)
- `GET /`   → formulario HTML (requiere contraseña web para guardar)
- `POST /save` → guarda config: `ssid`, `pass`, `tb_server`, `tb_token`, `new_web_pass`
- Tras guardar, reinicia el ESP32 en 3 segundos

### Campos configurables desde el portal
| Campo POST    | Descripción                                 |
|---------------|---------------------------------------------|
| `p`           | Contraseña del portal web (requerida)        |
| `ssid`        | SSID WiFi                                    |
| `pass`        | Contraseña WiFi (vacío = no cambiar)         |
| `tb_server`   | IP/dominio ThingsBoard                       |
| `tb_token`    | Token de acceso ThingsBoard                  |
| `new_web_pass`| Nueva contraseña portal (≥8 chars o vacío)  |

## 🛠️ Comandos de Desarrollo

### PlatformIO
```bash
# Instalar dependencias
pio pkg install

# Compilar
pio run

# Cargar
pio run --target upload

# Monitor serial
pio device monitor -b 115200

# Limpiar
pio run --target clean
```

### Debug
```bash
# Verificar pines
pio device monitor | grep "GPIO"

# Logs ThingsBoard
pio device monitor | grep "\[TB\]"

# Logs NFC
pio device monitor | grep "\[NFC\]"
```

## ⚠️ Problemas Comunes y Soluciones

### Touchscreen no funciona
```cpp
// Verificar configuración I2C
Wire.begin(TOUCH_SDA, TOUCH_SCL);  // Pines 19,20
ts.begin();
ts.setRotation(TOUCH_ROTATION);    // Rotación 90°
```

### NFC no detecta tarjetas
```cpp
// Verificar modo I2C del PN532
// DIP switches: CH1=ON, CH2=OFF
// Distancia máxima: 3cm
// UID logging activado por defecto
```

### ThingsBoard desconectado
```cpp
// Credenciales en network_config.cpp (se cargan desde NVS en runtime)
// Default: storage_get_tb_server() → THINGSBOARD_SERVER
//          storage_get_tb_token()  → THINGSBOARD_ACCESS_TOKEN

// Verificar conectividad WiFi
if (WiFi.status() != WL_CONNECTED) {
    reconnect_wifi();
}
```

### Pantalla se ve cortada
```cpp
// Verificar configuración LCD
#define DISPLAY_WIDTH 800
#define DISPLAY_HEIGHT 480

// Verificar timing RGB
#define LCD_HSYNC_FRONT_PORCH 40
#define LCD_HSYNC_PULSE_WIDTH 48
#define LCD_HSYNC_BACK_PORCH  40
```

## 📊 Métricas de Rendimiento

### Memoria
- **PSRAM**: 8MB para buffers LVGL
- **DRAM**: ~300KB libres en operación normal
- **NVS**: ~2KB usados para configuración

### CPU
- **LVGL**: ~15-20% CPU en UI activa
- **ThingsBoard**: ~5% CPU en polling normal
- **NFC**: ~2% CPU en polling 1Hz

### Consumo
- **Standby**: ~150mA (backlight reducido)
- **Activo**: ~200mA (pantalla full brightness)
- **Cerradura**: +500mA durante activación

## 🔄 Flujo de Trabajo de Desarrollo

### Nueva Feature
1. **Planificación**: Definir requerimientos
2. **Implementación**: Seguir convenciones de código
3. **Testing**: Verificar en hardware real
4. **Documentación**: Actualizar este CLAUDE.md

### Bug Fix
1. **Reproducción**: Identificar el problema
2. **Debug**: Usar logs serial
3. **Fix**: Aplicar solución minimalista
4. **Verificación**: Test exhaustivo

### Code Review Checklist
- ✅ Convenciones de nombres seguidas
- ✅ Memoria manejada correctamente
- ✅ Callbacks thread-safe
- ✅ Estado global protegido
- ✅ Logs informativos añadidos

## 📝 Notas para IA Assistant

### ⚡ REGLA DE AUTO-CORRECCIÓN (OBLIGATORIA)
> **Si al leer el código fuente encuentras información en este CLAUDE.md que no coincide con la realidad, EDITA ESTE ARCHIVO inmediatamente para corregirlo.** No esperes a que el usuario lo solicite. Un CLAUDE.md incorrecto es peor que no tenerlo.

### Contexto Importante
- **Hardware específico**: CrowPanel ESP32-S3 con pantalla RGB
- **Framework limitado**: Arduino ESP32, no full C++
- **Memoria crítica**: PSRAM esencial para buffers LVGL
- **Threading**: Single-thread, usar callbacks y timers
- **I2C compartido**: Touch y NFC en mismo bus
- **Relay**: `HIGH` = desbloqueado, `LOW` = bloqueado (GPIO 43)

### Patrones Comunes
- **Global state**: Variables `static volatile` para estado
- **Screen management**: `screen_busy` flag para concurrencia
- **Callback pattern**: Typedef + registro + invocación
- **Timer usage**: `lv_timer_create` para operaciones periódicas

### Consideraciones de Diseño
- **UI responsiveness**: LVGL maneja input/output
- **Power management**: Backlight y estados de bajo consumo
- **Network reliability**: Reconnection automática
- **Security**: Sin lockout de pantalla — credenciales con ventana temporal desde TB

### ⚠️ Reglas de Performance ESP32
- **NUNCA** imprimir strings largos (JSON completo) en el loop o en callbacks frecuentes — usar `Serial.printf` solo con longitud, no contenido
- **`ThingsBoardSized<N>`**: N controla (1) umbral de rechazo de mensajes entrantes (commas+{+[ en payload; si supera N el mensaje se descarta) y (2) tamaño de `StaticJsonDocument<JSON_OBJECT_SIZE(N)>` en stack (~N×16 bytes). Con N=128: ~2KB de stack, seguro en ESP32-S3. Aumentar si el JSON de credenciales crece mucho.
- **Buffer MQTT**: constructor es `(client, receive, send)` — receive PRIMERO. receive=8192 para credenciales grandes; send=1024 para accessEvent con credencial (hasta ~512 bytes serializado)
- **`DynamicJsonDocument`**: en credentials.cpp usar 16384 (soporta ~100 credenciales con ArduinoJson zero-copy)
- **NUNCA** usar `#include <esp_heap_caps.h>`, `#include <SPIFFS.h>` ni `#include <WebServer.h>` en archivos src/ — son headers pesados IDF que LDF propaga a todos los archivos causando "CreateProcess: No such file or directory" en Windows MSYS2
- **ArduinoJson iteración**: usar `for (JsonVariantConst v : arr) { auto obj = v.as<JsonObjectConst>(); }` — la conversión implícita `for (JsonObjectConst obj : arr)` retorna null silenciosamente

---

**Este archivo CLAUDE.md debe mantenerse actualizado con cualquier cambio significativo en el proyecto.**