# Cerradura Electrónica CrowPanel - Instrucciones de Hardware

## Conexiones de Hardware

### 1. Lector NFC PN532 (Modo I2C)

**IMPORTANTE:** El PN532 debe estar configurado en modo I2C. Verifica los interruptores DIP en el módulo:
- Canal 1: ON
- Canal 2: OFF

**Conexiones:**
```
PN532          ESP32-S3 (CrowPanel)
------         --------------------
VCC      →     3.3V
GND      →     GND
SDA      →     GPIO 19
SCL      →     GPIO 20
```

**Nota:** El PN532 comparte el bus I2C con el touchscreen GT911 que ya está conectado en los pines 19 y 20.

### 2. Relay/Cerradura Electromagnética

**Conexiones:**
```
Relay          ESP32-S3 (CrowPanel)
------         --------------------
VCC      →     5V
GND      →     GND
IN       →     GPIO 38
```

**Conexión de la Cerradura:**
```
Relay          Cerradura
------         ---------
NO       →     Cable rojo (+)
COM      →     Fuente 12V (+)

Cerradura      Fuente de Poder
---------      ---------------
Cable negro (-) → GND (-)
```

**Diagrama:**
```
        ┌──────────┐
Fuente  │          │  Relay
12V (+) ├──────────┤  COM
        │          │
        │          │  NO ────┐
        │          │         │
        └──────────┘         │
                             │
                      ┌──────▼──────┐
                      │  Cerradura  │
                      │ Electrónica │
                      └──────┬──────┘
                             │
        ┌──────────┐         │
Fuente  │          │         │
GND (-) ├──────────┴─────────┘
        └──────────┘
```

### 3. Touchscreen GT911

**Ya viene conectado en el CrowPanel:**
```
GT911          ESP32-S3
------         --------
SDA      →     GPIO 19
SCL      →     GPIO 20
```

### 4. Pantalla LCD RGB

**Ya viene conectada en el CrowPanel** (800x480 píxeles)

## Configuración del Software

### PIN de Desbloqueo
El código PIN predeterminado es: **123456**

Para cambiar el PIN, edita la línea 118 en `src/main.cpp`:
```cpp
const String correctPin = "123456";  // Cambia aquí tu PIN
```

### Tiempo de Desbloqueo
La cerradura permanece abierta por **5 segundos** por defecto.

Para cambiar el tiempo, edita las líneas 239 y 313 en `src/main.cpp`:
```cpp
}, 5000, NULL);  // 5000 = 5 segundos
```

### Validación de Tarjetas NFC Específicas

Por defecto, **cualquier tarjeta NFC** desbloquea la cerradura. Para agregar validación de tarjetas específicas, edita la función `nfc_check_timer_cb` en la línea 287:

```cpp
if (success)
{
  Serial.println("NFC Card detected!");
  Serial.print("UID: ");
  for (uint8_t i = 0; i < uidLength; i++)
  {
    Serial.print(" 0x");
    Serial.print(uid[i], HEX);
  }
  Serial.println();

  // AGREGAR VALIDACIÓN AQUÍ
  // Ejemplo: solo permitir tarjeta con UID específico
  if (uidLength == 4 &&
      uid[0] == 0xAB &&
      uid[1] == 0xCD &&
      uid[2] == 0xEF &&
      uid[3] == 0x01)
  {
    // UID correcto - desbloquear
    lv_label_set_text(label_nfc_status, "NFC DETECTADO - ACCESO CONCEDIDO");
    unlockDoor();
    // ... resto del código
  }
  else
  {
    // UID incorrecto
    lv_label_set_text(label_nfc_status, "TARJETA NO AUTORIZADA");
    lv_obj_set_style_text_color(label_nfc_status, COLOR_ERROR, 0);
  }
}
```

## Compilación y Carga

1. Abre el proyecto en **PlatformIO** (Visual Studio Code)
2. Conecta el CrowPanel via USB
3. Presiona el botón **Upload** o ejecuta:
   ```
   pio run --target upload
   ```

## Monitor Serial

Para ver los mensajes de debug y los UIDs de las tarjetas NFC:
```
pio device monitor -b 115200
```

## Especificaciones Técnicas

- **Microcontrolador:** ESP32-S3
- **Pantalla:** 800x480 RGB LCD
- **Touch:** GT911 Capacitivo
- **NFC:** PN532 (13.56MHz, ISO14443A)
- **Relay:** 5V, contacto normalmente abierto (NO)
- **Voltaje Cerradura:** 12V DC (típico)
- **Consumo Cerradura:** 300-500mA (depende del modelo)

## Paleta de Colores de la Interfaz

- **Fondo Principal:** #1a1a2e (Azul oscuro profundo)
- **Fondo Secundario:** #16213e (Azul marino)
- **Acento:** #0f3460 (Azul medio)
- **Éxito:** #00b894 (Verde esmeralda)
- **Error:** #d63031 (Rojo elegante)
- **Texto:** #ecf0f1 (Gris claro)
- **Botones:** #2c3e50 (Gris azulado)

## Solución de Problemas

### El touch no responde
1. Verifica que el módulo GT911 esté correctamente conectado
2. Asegúrate de que no hay conflictos en el bus I2C
3. El touch y NFC comparten el mismo bus I2C (pines 19 y 20)

### El NFC no detecta tarjetas
1. Verifica que el PN532 esté en modo I2C (DIP switches)
2. Verifica las conexiones SDA y SCL
3. Acerca la tarjeta a menos de 3cm del lector
4. Revisa el monitor serial para ver si se detecta el chip PN532

### La cerradura no se activa
1. Verifica la conexión del relay al GPIO 38
2. Verifica que el relay tenga alimentación (5V)
3. Verifica las conexiones de la cerradura electromagnética
4. Asegúrate de que la fuente de 12V tenga suficiente corriente (mínimo 500mA)

### La pantalla se ve cortada
1. El diseño está optimizado para 800x480 píxeles
2. Verifica que la configuración de pantalla en el código coincida con tu panel

## Licencia y Créditos

Sistema de Cerradura Electrónica basado en:
- **LVGL** (v8.3.6) - Biblioteca gráfica
- **LovyanGFX** - Driver de pantalla
- **Adafruit PN532** - Librería NFC
- **TAMC_GT911** - Driver touchscreen

Desarrollado para CrowPanel ESP32-S3 800x480
