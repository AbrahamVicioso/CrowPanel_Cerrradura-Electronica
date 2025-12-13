#include <PCA9557.h>
#include <lvgl.h>
#include <LovyanGFX.hpp>
#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <Wire.h>
#include <Adafruit_PN532.h>

// Pines I2C para PN532 (comparte bus con Touch)
#define PN532_SDA 19
#define PN532_SCL 20

// Pin del relay/cerradura
#define LOCK_PIN 38

// Configuración de pantalla LCD
class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_RGB   _panel_instance;

  LGFX(void)
  {
    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;

      cfg.pin_d0  = GPIO_NUM_15; // B0
      cfg.pin_d1  = GPIO_NUM_7;  // B1
      cfg.pin_d2  = GPIO_NUM_6;  // B2
      cfg.pin_d3  = GPIO_NUM_5;  // B3
      cfg.pin_d4  = GPIO_NUM_4;  // B4

      cfg.pin_d5  = GPIO_NUM_9;  // G0
      cfg.pin_d6  = GPIO_NUM_46; // G1
      cfg.pin_d7  = GPIO_NUM_3;  // G2
      cfg.pin_d8  = GPIO_NUM_8;  // G3
      cfg.pin_d9  = GPIO_NUM_16; // G4
      cfg.pin_d10 = GPIO_NUM_1;  // G5

      cfg.pin_d11 = GPIO_NUM_14; // R0
      cfg.pin_d12 = GPIO_NUM_21; // R1
      cfg.pin_d13 = GPIO_NUM_47; // R2
      cfg.pin_d14 = GPIO_NUM_48; // R3
      cfg.pin_d15 = GPIO_NUM_45; // R4

      cfg.pin_henable = GPIO_NUM_41;
      cfg.pin_vsync   = GPIO_NUM_40;
      cfg.pin_hsync   = GPIO_NUM_39;
      cfg.pin_pclk    = GPIO_NUM_0;
      cfg.freq_write  = 15000000;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 40;
      cfg.hsync_pulse_width = 48;
      cfg.hsync_back_porch  = 40;

      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 1;
      cfg.vsync_pulse_width = 31;
      cfg.vsync_back_porch  = 13;

      cfg.pclk_active_neg   = 1;
      cfg.de_idle_high      = 0;
      cfg.pclk_idle_high    = 0;

      _bus_instance.config(cfg);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.memory_width  = 800;
      cfg.memory_height = 480;
      cfg.panel_width  = 800;
      cfg.panel_height = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);
    setPanel(&_panel_instance);
  }
};

LGFX lcd;

// Touch
#define TOUCH_GT911
#define TOUCH_GT911_SCL 20
#define TOUCH_GT911_SDA 19
#define TOUCH_GT911_INT -1
#define TOUCH_GT911_RST -1
#define TOUCH_GT911_ROTATION ROTATION_NORMAL
#define TOUCH_MAP_X1 800
#define TOUCH_MAP_X2 0
#define TOUCH_MAP_Y1 480
#define TOUCH_MAP_Y2 0

int touch_last_x = 0, touch_last_y = 0;

#include <TAMC_GT911.h>
TAMC_GT911 ts = TAMC_GT911(TOUCH_GT911_SDA, TOUCH_GT911_SCL, TOUCH_GT911_INT, TOUCH_GT911_RST,
                            max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));

// NFC Reader
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);

// LVGL
#define TFT_BL 2
static uint32_t screenWidth = 800;
static uint32_t screenHeight = 480;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t disp_draw_buf[800 * 480 / 15];
static lv_disp_drv_t disp_drv;

// Variables de la cerradura
String pinCode = "";
const String correctPin = "123456";
bool isUnlocked = false;

// Pantallas y objetos LVGL
lv_obj_t *screen_welcome;
lv_obj_t *screen_pinpad;
lv_obj_t *screen_unlocked;
lv_obj_t *label_pin_display;
lv_obj_t *label_status;
lv_obj_t *label_nfc_status;
lv_obj_t *arc_animation;

// Timer para NFC
lv_timer_t *nfc_timer;

// Colores corporativos elegantes
#define COLOR_PRIMARY lv_color_hex(0x1a1a2e)      // Azul oscuro profundo
#define COLOR_SECONDARY lv_color_hex(0x16213e)    // Azul marino
#define COLOR_ACCENT lv_color_hex(0x0f3460)       // Azul medio
#define COLOR_SUCCESS lv_color_hex(0x00b894)      // Verde esmeralda
#define COLOR_ERROR lv_color_hex(0xd63031)        // Rojo elegante
#define COLOR_TEXT lv_color_hex(0xecf0f1)         // Gris claro
#define COLOR_BUTTON lv_color_hex(0x2c3e50)       // Gris azulado
#define COLOR_BUTTON_PRESSED lv_color_hex(0x34495e) // Gris azulado oscuro

// Funciones LVGL
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  lcd.pushImageDMA(area->x1, area->y1, w, h, (lgfx::rgb565_t*)&color_p->full);
  lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  ts.read();
  if (ts.isTouched)
  {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = map(ts.points[0].x, TOUCH_MAP_X1, TOUCH_MAP_X2, 0, lcd.width() - 1);
    data->point.y = map(ts.points[0].y, TOUCH_MAP_Y1, TOUCH_MAP_Y2, 0, lcd.height() - 1);
  }
  else
  {
    data->state = LV_INDEV_STATE_REL;
  }
  delay(5);
}

// Funciones de control de cerradura
void unlockDoor()
{
  digitalWrite(LOCK_PIN, HIGH);
  isUnlocked = true;
  Serial.println("Cerradura desbloqueada");
}

void lockDoor()
{
  digitalWrite(LOCK_PIN, LOW);
  isUnlocked = false;
  Serial.println("Cerradura bloqueada");
}

// Animación de bienvenida
void welcome_animation(lv_timer_t *timer)
{
  static int angle = 0;
  lv_arc_set_value(arc_animation, angle);
  angle += 5;

  if (angle >= 360)
  {
    lv_timer_del(timer);
    lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_FADE_ON, 500, 100, false);
  }
}

// Callback para botones numéricos
void btn_num_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
    lv_obj_t *btn = lv_event_get_target(e);
    const char *num = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (pinCode.length() < 6)
    {
      pinCode += num;
      String display = "";
      for (int i = 0; i < pinCode.length(); i++)
      {
        display += "● ";
      }
      lv_label_set_text(label_pin_display, display.c_str());

      // Efecto visual del botón
      lv_obj_set_style_bg_color(btn, COLOR_BUTTON_PRESSED, LV_PART_MAIN);

      // Validar automáticamente cuando llegue a 6 dígitos
      if (pinCode.length() == 6)
      {
        lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
          if (pinCode == correctPin)
          {
            lv_label_set_text(label_status, "ACCESO CONCEDIDO");
            lv_obj_set_style_text_color(label_status, COLOR_SUCCESS, 0);
            unlockDoor();
            lv_scr_load_anim(screen_unlocked, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 1000, false);

            // Auto-bloquear después de 5 segundos
            lv_timer_t *lock_timer = lv_timer_create([](lv_timer_t *lt) {
              lockDoor();
              pinCode = "";
              lv_label_set_text(label_pin_display, "");
              lv_label_set_text(label_status, "Ingrese PIN");
              lv_obj_set_style_text_color(label_status, COLOR_TEXT, 0);
              lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
              lv_timer_del(lt);
            }, 5000, NULL);
            lv_timer_set_repeat_count(lock_timer, 1);
          }
          else
          {
            lv_label_set_text(label_status, "PIN INCORRECTO");
            lv_obj_set_style_text_color(label_status, COLOR_ERROR, 0);
            pinCode = "";
            lv_label_set_text(label_pin_display, "");

            // Restaurar mensaje después de 2 segundos
            lv_timer_t *restore_timer = lv_timer_create([](lv_timer_t *rt) {
              lv_label_set_text(label_status, "Ingrese PIN");
              lv_obj_set_style_text_color(label_status, COLOR_TEXT, 0);
              lv_timer_del(rt);
            }, 2000, NULL);
            lv_timer_set_repeat_count(restore_timer, 1);
          }
          lv_timer_del(t);
        }, 300, NULL);
        lv_timer_set_repeat_count(timer, 1);
      }
    }
  }
}

// Callback para botón de borrar
void btn_clear_event_cb(lv_event_t *e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED)
  {
    pinCode = "";
    lv_label_set_text(label_pin_display, "");
    lv_label_set_text(label_status, "Ingrese PIN");
    lv_obj_set_style_text_color(label_status, COLOR_TEXT, 0);
  }
}

// Función para leer NFC
void nfc_check_timer_cb(lv_timer_t *timer)
{
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength, 20);

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

    // Desbloquear con cualquier tarjeta NFC (puedes agregar validación de UID aquí)
    lv_label_set_text(label_nfc_status, "NFC DETECTADO - ACCESO CONCEDIDO");
    lv_obj_set_style_text_color(label_nfc_status, COLOR_SUCCESS, 0);

    unlockDoor();
    lv_scr_load_anim(screen_unlocked, LV_SCR_LOAD_ANIM_OVER_TOP, 500, 0, false);

    // Auto-bloquear después de 5 segundos
    lv_timer_t *lock_timer = lv_timer_create([](lv_timer_t *lt) {
      lockDoor();
      lv_label_set_text(label_nfc_status, "Acerque tarjeta NFC");
      lv_obj_set_style_text_color(label_nfc_status, COLOR_TEXT, 0);
      lv_scr_load_anim(screen_pinpad, LV_SCR_LOAD_ANIM_FADE_ON, 500, 0, false);
      lv_timer_del(lt);
    }, 5000, NULL);
    lv_timer_set_repeat_count(lock_timer, 1);
  }
}

// Crear pantalla de bienvenida
void create_welcome_screen()
{
  screen_welcome = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_welcome, COLOR_PRIMARY, 0);

  // Logo/Título con animación
  lv_obj_t *label_welcome = lv_label_create(screen_welcome);
  lv_label_set_text(label_welcome, "SISTEMA DE ACCESO");
  lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(label_welcome, COLOR_TEXT, 0);
  lv_obj_align(label_welcome, LV_ALIGN_CENTER, 0, -60);

  // Subtítulo
  lv_obj_t *label_subtitle = lv_label_create(screen_welcome);
  lv_label_set_text(label_subtitle, "Control de Cerradura Electronica");
  lv_obj_set_style_text_font(label_subtitle, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(label_subtitle, lv_color_hex(0x95a5a6), 0);
  lv_obj_align(label_subtitle, LV_ALIGN_CENTER, 0, 0);

  // Círculo de carga animado
  arc_animation = lv_arc_create(screen_welcome);
  lv_obj_set_size(arc_animation, 100, 100);
  lv_arc_set_rotation(arc_animation, 270);
  lv_arc_set_bg_angles(arc_animation, 0, 360);
  lv_arc_set_angles(arc_animation, 0, 0);
  lv_obj_set_style_arc_color(arc_animation, COLOR_ACCENT, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc_animation, COLOR_SUCCESS, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc_animation, 8, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc_animation, 8, LV_PART_INDICATOR);
  lv_obj_remove_style(arc_animation, NULL, LV_PART_KNOB);
  lv_obj_align(arc_animation, LV_ALIGN_CENTER, 0, 80);
  lv_obj_clear_flag(arc_animation, LV_OBJ_FLAG_CLICKABLE);

  // Iniciar animación
  lv_timer_t *timer = lv_timer_create(welcome_animation, 20, NULL);
}

// Crear pantalla de teclado PIN
void create_pinpad_screen()
{
  screen_pinpad = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_pinpad, COLOR_PRIMARY, 0);

  // Panel superior
  lv_obj_t *top_panel = lv_obj_create(screen_pinpad);
  lv_obj_set_size(top_panel, 800, 70);
  lv_obj_set_pos(top_panel, 0, 0);
  lv_obj_set_style_bg_color(top_panel, COLOR_SECONDARY, 0);
  lv_obj_set_style_border_width(top_panel, 0, 0);
  lv_obj_set_style_radius(top_panel, 0, 0);

  // Título
  lv_obj_t *label_title = lv_label_create(top_panel);
  lv_label_set_text(label_title, "SISTEMA DE ACCESO");
  lv_obj_set_style_text_font(label_title, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
  lv_obj_align(label_title, LV_ALIGN_CENTER, -50, 0);

  // Icono de candado
  lv_obj_t *lock_icon = lv_label_create(top_panel);
  lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
  lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(lock_icon, COLOR_ACCENT, 0);
  lv_obj_align(lock_icon, LV_ALIGN_CENTER, 150, 0);

  // Contenedor central para PIN
  lv_obj_t *pin_container = lv_obj_create(screen_pinpad);
  lv_obj_set_size(pin_container, 500, 110);
  lv_obj_align(pin_container, LV_ALIGN_TOP_MID, 0, 75);
  lv_obj_set_style_bg_color(pin_container, COLOR_ACCENT, 0);
  lv_obj_set_style_border_width(pin_container, 2, 0);
  lv_obj_set_style_border_color(pin_container, COLOR_SUCCESS, 0);
  lv_obj_set_style_radius(pin_container, 15, 0);
  lv_obj_set_style_shadow_width(pin_container, 20, 0);
  lv_obj_set_style_shadow_opa(pin_container, LV_OPA_30, 0);

  // Label de estado
  label_status = lv_label_create(pin_container);
  lv_label_set_text(label_status, "Ingrese PIN");
  lv_obj_set_style_text_font(label_status, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_color(label_status, COLOR_TEXT, 0);
  lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 8);

  // Display del PIN (puntos)
  label_pin_display = lv_label_create(pin_container);
  lv_label_set_text(label_pin_display, "");
  lv_obj_set_style_text_font(label_pin_display, &lv_font_montserrat_36, 0);
  lv_obj_set_style_text_color(label_pin_display, COLOR_SUCCESS, 0);
  lv_obj_align(label_pin_display, LV_ALIGN_CENTER, 0, 5);

  // Label NFC
  label_nfc_status = lv_label_create(pin_container);
  lv_label_set_text(label_nfc_status, "Acerque tarjeta NFC");
  lv_obj_set_style_text_font(label_nfc_status, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(label_nfc_status, lv_color_hex(0x95a5a6), 0);
  lv_obj_align(label_nfc_status, LV_ALIGN_BOTTOM_MID, 0, -8);

  // Teclado numérico (3x4)
  const char *btnm_map[] = {"1", "2", "3", "\n",
                             "4", "5", "6", "\n",
                             "7", "8", "9", "\n",
                             LV_SYMBOL_BACKSPACE, "0", LV_SYMBOL_OK, ""};

  int btn_numbers[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 0};
  const char *btn_labels[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9", "0"};

  int spacing = 10;
  int btn_width = 95;
  int btn_height = 55;
  int start_x = 252;
  int start_y = 200;

  // Crear botones 1-9
  for (int i = 0; i < 9; i++)
  {
    int row = i / 3;
    int col = i % 3;

    lv_obj_t *btn = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn, btn_width, btn_height);
    lv_obj_set_pos(btn, start_x + col * (btn_width + spacing), start_y + row * (btn_height + spacing));
    lv_obj_set_style_bg_color(btn, COLOR_BUTTON, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn, COLOR_BUTTON_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 10, 0);
    lv_obj_set_style_shadow_width(btn, 10, 0);
    lv_obj_set_style_shadow_opa(btn, LV_OPA_30, 0);

    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, btn_labels[i]);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
    lv_obj_center(label);

    lv_obj_add_event_cb(btn, btn_num_event_cb, LV_EVENT_CLICKED, NULL);
  }

  // Botón de borrar (posición 0 del cuarto row)
  lv_obj_t *btn_clear = lv_btn_create(screen_pinpad);
  lv_obj_set_size(btn_clear, btn_width, btn_height);
  lv_obj_set_pos(btn_clear, start_x, start_y + 3 * (btn_height + spacing));
  lv_obj_set_style_bg_color(btn_clear, COLOR_ERROR, LV_PART_MAIN);
  lv_obj_set_style_radius(btn_clear, 10, 0);
  lv_obj_set_style_shadow_width(btn_clear, 10, 0);
  lv_obj_set_style_shadow_opa(btn_clear, LV_OPA_30, 0);

  lv_obj_t *label_clear = lv_label_create(btn_clear);
  lv_label_set_text(label_clear, LV_SYMBOL_BACKSPACE);
  lv_obj_set_style_text_font(label_clear, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_clear, COLOR_TEXT, 0);
  lv_obj_center(label_clear);

  lv_obj_add_event_cb(btn_clear, btn_clear_event_cb, LV_EVENT_CLICKED, NULL);

  // Botón 0 (posición 1 del cuarto row)
  lv_obj_t *btn_0 = lv_btn_create(screen_pinpad);
  lv_obj_set_size(btn_0, btn_width, btn_height);
  lv_obj_set_pos(btn_0, start_x + (btn_width + spacing), start_y + 3 * (btn_height + spacing));
  lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON_PRESSED, LV_STATE_PRESSED);
  lv_obj_set_style_radius(btn_0, 10, 0);
  lv_obj_set_style_shadow_width(btn_0, 10, 0);
  lv_obj_set_style_shadow_opa(btn_0, LV_OPA_30, 0);

  lv_obj_t *label_0 = lv_label_create(btn_0);
  lv_label_set_text(label_0, "0");
  lv_obj_set_style_text_font(label_0, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_0, COLOR_TEXT, 0);
  lv_obj_center(label_0);

  lv_obj_add_event_cb(btn_0, btn_num_event_cb, LV_EVENT_CLICKED, NULL);

  // Botón OK (posición 2 del cuarto row) - decorativo
  lv_obj_t *btn_ok = lv_btn_create(screen_pinpad);
  lv_obj_set_size(btn_ok, btn_width, btn_height);
  lv_obj_set_pos(btn_ok, start_x + 2 * (btn_width + spacing), start_y + 3 * (btn_height + spacing));
  lv_obj_set_style_bg_color(btn_ok, COLOR_SUCCESS, LV_PART_MAIN);
  lv_obj_set_style_radius(btn_ok, 10, 0);
  lv_obj_set_style_shadow_width(btn_ok, 10, 0);
  lv_obj_set_style_shadow_opa(btn_ok, LV_OPA_30, 0);
  lv_obj_add_flag(btn_ok, LV_OBJ_FLAG_CLICKABLE);

  lv_obj_t *label_ok = lv_label_create(btn_ok);
  lv_label_set_text(label_ok, LV_SYMBOL_OK);
  lv_obj_set_style_text_font(label_ok, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_ok, COLOR_TEXT, 0);
  lv_obj_center(label_ok);
}

// Crear pantalla de desbloqueo exitoso
void create_unlocked_screen()
{
  screen_unlocked = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(screen_unlocked, COLOR_PRIMARY, 0);

  // Panel verde de éxito
  lv_obj_t *success_panel = lv_obj_create(screen_unlocked);
  lv_obj_set_size(success_panel, 600, 350);
  lv_obj_align(success_panel, LV_ALIGN_CENTER, 0, 0);
  lv_obj_set_style_bg_color(success_panel, COLOR_SUCCESS, 0);
  lv_obj_set_style_border_width(success_panel, 0, 0);
  lv_obj_set_style_radius(success_panel, 20, 0);
  lv_obj_set_style_shadow_width(success_panel, 30, 0);
  lv_obj_set_style_shadow_opa(success_panel, LV_OPA_50, 0);

  // Icono de check grande
  lv_obj_t *icon_check = lv_label_create(success_panel);
  lv_label_set_text(icon_check, LV_SYMBOL_OK);
  lv_obj_set_style_text_font(icon_check, &lv_font_montserrat_48, 0);
  lv_obj_set_style_text_color(icon_check, lv_color_white(), 0);
  lv_obj_align(icon_check, LV_ALIGN_TOP_MID, 0, 40);

  // Círculo alrededor del check
  lv_obj_t *circle = lv_obj_create(success_panel);
  lv_obj_set_size(circle, 120, 120);
  lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_bg_color(circle, lv_color_hex(0x00d2a4), 0);
  lv_obj_set_style_border_width(circle, 4, 0);
  lv_obj_set_style_border_color(circle, lv_color_white(), 0);
  lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
  lv_obj_move_background(circle);

  // Texto de éxito
  lv_obj_t *label_success = lv_label_create(success_panel);
  lv_label_set_text(label_success, "ACCESO CONCEDIDO");
  lv_obj_set_style_text_font(label_success, &lv_font_montserrat_40, 0);
  lv_obj_set_style_text_color(label_success, lv_color_white(), 0);
  lv_obj_align(label_success, LV_ALIGN_CENTER, 0, 20);

  // Mensaje secundario
  lv_obj_t *label_welcome = lv_label_create(success_panel);
  lv_label_set_text(label_welcome, "Bienvenido");
  lv_obj_set_style_text_font(label_welcome, &lv_font_montserrat_28, 0);
  lv_obj_set_style_text_color(label_welcome, lv_color_white(), 0);
  lv_obj_align(label_welcome, LV_ALIGN_BOTTOM_MID, 0, -30);

  // Icono de candado abierto
  lv_obj_t *unlock_icon = lv_label_create(success_panel);
  lv_label_set_text(unlock_icon, LV_SYMBOL_UPLOAD );
  lv_obj_set_style_text_font(unlock_icon, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(unlock_icon, lv_color_white(), 0);
  lv_obj_align(unlock_icon, LV_ALIGN_BOTTOM_MID, 0, -80);
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Iniciando Sistema de Cerradura Electrónica...");

  // Configurar pin de cerradura
  pinMode(LOCK_PIN, OUTPUT);
  lockDoor();

  // Inicializar I2C para NFC
  Wire.begin(PN532_SDA, PN532_SCL);

  // Inicializar NFC
  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata)
  {
    Serial.println("No se encontró el módulo PN532");
  }
  else
  {
    Serial.print("Chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    Serial.print("Firmware ver. ");
    Serial.print((versiondata >> 16) & 0xFF, DEC);
    Serial.print('.');
    Serial.println((versiondata >> 8) & 0xFF, DEC);

    nfc.SAMConfig();
    Serial.println("PN532 configurado correctamente");
  }

  // Inicializar LCD
  lcd.begin();
  lcd.fillScreen(TFT_BLACK);
  delay(100);

  // Inicializar LVGL
  lv_init();

  // Inicializar Touch (usar el Wire ya inicializado para NFC)
  ts.begin();
  ts.setRotation(TOUCH_GT911_ROTATION);

  // Configurar display
  lv_disp_draw_buf_init(&draw_buf, disp_draw_buf, NULL, screenWidth * screenHeight / 15);
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Configurar input
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = my_touchpad_read;
  lv_indev_drv_register(&indev_drv);

  // Configurar backlight
  ledcSetup(1, 300, 8);
  ledcAttachPin(TFT_BL, 1);
  ledcWrite(1, 255);

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // Crear pantallas
  create_welcome_screen();
  create_pinpad_screen();
  create_unlocked_screen();

  // Cargar pantalla de bienvenida
  lv_scr_load(screen_welcome);

  // Timer para chequear NFC cada 1000ms (1 segundo)
  nfc_timer = lv_timer_create(nfc_check_timer_cb, 1000, NULL);

  Serial.println("Sistema iniciado correctamente");
  Serial.println("PIN correcto: 123456");
}

void loop()
{
  lv_timer_handler();
  delay(1);
}
