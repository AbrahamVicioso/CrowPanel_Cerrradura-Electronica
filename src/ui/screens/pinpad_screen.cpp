/**
 * @file pinpad_screen.cpp
 * @brief Implementación de la pantalla del teclado PIN
 */

#include "pinpad_screen.h"
#include "../theme.h"
#include "../../config/config.h"

// ============================================
// VARIABLES PRIVADAS
// ============================================

static lv_obj_t* screen_pinpad = nullptr;
static lv_obj_t* label_pin_display = nullptr;
static lv_obj_t* label_status = nullptr;
static lv_obj_t* label_nfc_status = nullptr;

static String pinCode = "";
static String correctPin = DEFAULT_PIN;

static PinpadSuccessCallback success_callback = nullptr;
static PinpadErrorCallback error_callback = nullptr;

// ============================================
// FUNCIONES CALLBACK
// ============================================

static void btn_num_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        lv_obj_t *btn = lv_event_get_target(e);
        const char *num = lv_label_get_text(lv_obj_get_child(btn, 0));

        if (pinCode.length() < PIN_LENGTH)
        {
            pinCode += num;
            
            // Ocultar textos de ayuda cuando hay input
            if (pinCode.length() == 1) {
                lv_obj_add_flag(label_status, LV_OBJ_FLAG_HIDDEN);
                lv_obj_add_flag(label_nfc_status, LV_OBJ_FLAG_HIDDEN);
            }
            
            // Actualizar display con puntos
            String display = "";
            for (int i = 0; i < pinCode.length(); i++)
            {
                display += "*";
            }
            lv_label_set_text(label_pin_display, display.c_str());

            // Validar automáticamente cuando llegue a PIN_LENGTH dígitos
            if (pinCode.length() == PIN_LENGTH)
            {
                lv_timer_t *timer = lv_timer_create([](lv_timer_t *t) {
                    if (pinCode == correctPin)
                    {
                        lv_label_set_text(label_status, "ACCESO OK");
                        lv_obj_set_style_text_color(label_status, COLOR_SUCCESS, 0);
                        
                        if (success_callback) {
                            success_callback();
                        }
                    }
                    else
                    {
                        lv_label_set_text(label_status, "PIN INCORRECTO");
                        lv_obj_set_style_text_color(label_status, COLOR_ERROR, 0);
                        
                        if (error_callback) {
                            error_callback();
                        }
                        
                        pinCode = "";
                        lv_label_set_text(label_pin_display, "");

                        // Restaurar mensaje después de 2 segundos
                        lv_timer_t *restore_timer = lv_timer_create([](lv_timer_t *rt) {
                            lv_label_set_text(label_status, "Ingrese PIN");
                            lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
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

static void btn_clear_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED)
    {
        pinCode = "";
        lv_label_set_text(label_pin_display, "");
        lv_label_set_text(label_status, "Ingrese PIN");
        lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
        // Mostrar textos de ayuda
        lv_obj_clear_flag(label_status, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_flag(label_nfc_status, LV_OBJ_FLAG_HIDDEN);
    }
}

// ============================================
// FUNCIONES PÚBLICAS
// ============================================

lv_obj_t* pinpad_screen_create(void)
{
    screen_pinpad = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_pinpad, COLOR_PRIMARY, 0);
    lv_obj_clear_flag(screen_pinpad, LV_OBJ_FLAG_SCROLLABLE);

    // ==================== HEADER BACKGROUND ====================
    
    // Barra de título como label (sin scroll)
    lv_obj_t *header_bg = lv_obj_create(screen_pinpad);
    lv_obj_set_size(header_bg, 800, 60);
    lv_obj_set_pos(header_bg, 0, 0);
    lv_obj_set_style_bg_color(header_bg, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(header_bg, 0, 0);
    lv_obj_set_style_radius(header_bg, 0, 0);
    lv_obj_clear_flag(header_bg, LV_OBJ_FLAG_SCROLLABLE);

    // Icono de candado en header (como label, no scroll)
    lv_obj_t *lock_icon = lv_label_create(header_bg);
    lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_ACCENT, 0);
    lv_obj_set_pos(lock_icon, 30, 18);

    // Título del header (como label fijo)
    lv_obj_t *label_title = lv_label_create(header_bg);
    lv_label_set_text(label_title, "CONTROL DE ACCESO");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_pos(label_title, 70, 20);

    // ==================== STATUS PANEL ====================
    
    // Contenedor de estado
    lv_obj_t *status_container = lv_obj_create(screen_pinpad);
    lv_obj_set_size(status_container, 400, 110);
    lv_obj_set_pos(status_container, 200, 75);
    lv_obj_set_style_bg_color(status_container, COLOR_SECONDARY, 0);
    lv_obj_set_style_radius(status_container, 15, 0);
    lv_obj_set_style_border_width(status_container, 0, 0);
    lv_obj_set_style_pad_all(status_container, 10, 0);
    lv_obj_clear_flag(status_container, LV_OBJ_FLAG_SCROLLABLE);

    // Label de estado
    label_status = lv_label_create(status_container);
    lv_label_set_text(label_status, "Ingrese PIN");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 15);

    // Display del PIN
    label_pin_display = lv_label_create(status_container);
    lv_label_set_text(label_pin_display, "");
    lv_obj_set_style_text_font(label_pin_display, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_pin_display, COLOR_TEXT, 0);
    lv_obj_align(label_pin_display, LV_ALIGN_CENTER, 0, 0);

    // Label NFC
    label_nfc_status = lv_label_create(status_container);
    lv_label_set_text(label_nfc_status, "Use tarjeta NFC");
    lv_obj_set_style_text_font(label_nfc_status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_nfc_status, COLOR_TEXT_MUTED, 0);
    lv_obj_align(label_nfc_status, LV_ALIGN_BOTTOM_MID, 0, -10);

    // ==================== KEYPAD ====================
    
    const char *btn_labels[] = {"1", "2", "3", "4", "5", "6", "7", "8", "9"};

    int spacing = 12;
    int btn_width = 80;
    int btn_height = 55;
    int start_x = 260;
    int start_y = 195;

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
        lv_obj_set_style_shadow_width(btn, 5, 0);
        lv_obj_set_style_shadow_opa(btn, LV_OPA_30, LV_PART_MAIN);

        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, btn_labels[i]);
        lv_obj_set_style_text_font(label, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(label, COLOR_TEXT, 0);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, btn_num_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Botón de borrar (C)
    lv_obj_t *btn_clear = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_clear, btn_width, btn_height);
    lv_obj_set_pos(btn_clear, start_x, start_y + 3 * (btn_height + spacing));
    lv_obj_set_style_bg_color(btn_clear, COLOR_ERROR, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_clear, 10, 0);
    lv_obj_set_style_shadow_width(btn_clear, 5, 0);
    lv_obj_set_style_shadow_opa(btn_clear, LV_OPA_30, LV_PART_MAIN);

    lv_obj_t *label_clear = lv_label_create(btn_clear);
    lv_label_set_text(label_clear, "C");
    lv_obj_set_style_text_font(label_clear, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_clear, COLOR_TEXT, 0);
    lv_obj_center(label_clear);

    lv_obj_add_event_cb(btn_clear, btn_clear_event_cb, LV_EVENT_CLICKED, NULL);

    // Botón 0
    lv_obj_t *btn_0 = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_0, btn_width, btn_height);
    lv_obj_set_pos(btn_0, start_x + (btn_width + spacing), start_y + 3 * (btn_height + spacing));
    lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_0, 10, 0);
    lv_obj_set_style_shadow_width(btn_0, 5, 0);
    lv_obj_set_style_shadow_opa(btn_0, LV_OPA_30, LV_PART_MAIN);

    lv_obj_t *label_0 = lv_label_create(btn_0);
    lv_label_set_text(label_0, "0");
    lv_obj_set_style_text_font(label_0, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(label_0, COLOR_TEXT, 0);
    lv_obj_center(label_0);

    lv_obj_add_event_cb(btn_0, btn_num_event_cb, LV_EVENT_CLICKED, NULL);

    // Botón OK
    lv_obj_t *btn_ok = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_ok, btn_width, btn_height);
    lv_obj_set_pos(btn_ok, start_x + 2 * (btn_width + spacing), start_y + 3 * (btn_height + spacing));
    lv_obj_set_style_bg_color(btn_ok, COLOR_SUCCESS, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_ok, 10, 0);
    lv_obj_set_style_shadow_width(btn_ok, 5, 0);
    lv_obj_set_style_shadow_opa(btn_ok, LV_OPA_30, LV_PART_MAIN);
    lv_obj_clear_flag(btn_ok, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_t *label_ok = lv_label_create(btn_ok);
    lv_label_set_text(label_ok, "OK");
    lv_obj_set_style_text_font(label_ok, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_ok, COLOR_TEXT, 0);
    lv_obj_center(label_ok);

    return screen_pinpad;
}

void pinpad_set_success_callback(PinpadSuccessCallback callback)
{
    success_callback = callback;
}

void pinpad_set_error_callback(PinpadErrorCallback callback)
{
    error_callback = callback;
}

void pinpad_reset(void)
{
    pinCode = "";
    if (label_pin_display != nullptr) {
        lv_label_set_text(label_pin_display, "");
    }
    if (label_status != nullptr) {
        lv_label_set_text(label_status, "Ingrese PIN");
        lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    }
    // Mostrar textos de ayuda
    if (label_nfc_status != nullptr) {
        lv_obj_clear_flag(label_nfc_status, LV_OBJ_FLAG_HIDDEN);
    }
    if (label_status != nullptr) {
        lv_obj_clear_flag(label_status, LV_OBJ_FLAG_HIDDEN);
    }
}

std::string pinpad_get_pin(void)
{
    return pinCode.c_str();
}

void pinpad_set_correct_pin(const String& pin)
{
    correctPin = pin;
}

void pinpad_show_status(const char* message, lv_color_t color)
{
    if (label_status != nullptr) {
        lv_label_set_text(label_status, message);
        lv_obj_set_style_text_color(label_status, color, 0);
    }
}
