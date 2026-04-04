/**
 * @file config_screen.cpp
 * @brief Pantalla de modo configuración
 */

#include "config_screen.h"
#include "../theme.h"
#include "../../config/config.h"

#define C_BG      0x050d1a
#define C_CARD    0x0a1628
#define C_BORDER  0x1e3a5f
#define C_ACCENT  0x3b82f6
#define C_TEXT    0xf0f6ff
#define C_DIM     0x4a5568
#define C_WARN    0xf59e0b

void config_screen_show(const String& ap_ssid, const String& ap_pass, const String& ap_ip)
{
    lv_obj_t* scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_hex(C_BG), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    // Tarjeta centrada
    lv_obj_t* card = lv_obj_create(scr);
    lv_obj_set_size(card, 520, 390);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(C_CARD), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_border_width(card, 1, 0);
    lv_obj_set_style_radius(card, 12, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    // Título
    lv_obj_t* title = lv_label_create(card);
    lv_label_set_text(title, "MODO CONFIGURACION");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(C_ACCENT), 0);
    lv_obj_set_style_text_letter_space(title, 3, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 28);

    lv_obj_t* sub = lv_label_create(card);
    lv_label_set_text(sub, "Conecta tu celular y abre el navegador");
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(C_DIM), 0);
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 54);

    // --- Fila: Red WiFi ---
    lv_obj_t* k1 = lv_label_create(card);
    lv_label_set_text(k1, "RED WIFI");
    lv_obj_set_style_text_font(k1, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(k1, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_letter_space(k1, 2, 0);
    lv_obj_align(k1, LV_ALIGN_TOP_LEFT, 36, 96);

    lv_obj_t* v1 = lv_label_create(card);
    lv_label_set_text(v1, ap_ssid.c_str());
    lv_obj_set_style_text_font(v1, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(v1, lv_color_hex(C_TEXT), 0);
    lv_obj_align(v1, LV_ALIGN_TOP_LEFT, 36, 112);

    // --- Fila: Contraseña WiFi (amarilla) ---
    lv_obj_t* k2 = lv_label_create(card);
    lv_label_set_text(k2, "CONTRASENA WIFI  (nueva cada vez)");
    lv_obj_set_style_text_font(k2, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(k2, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_letter_space(k2, 1, 0);
    lv_obj_align(k2, LV_ALIGN_TOP_LEFT, 36, 152);

    lv_obj_t* v2 = lv_label_create(card);
    lv_label_set_text(v2, ap_pass.c_str());
    lv_obj_set_style_text_font(v2, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(v2, lv_color_hex(C_WARN), 0);
    lv_obj_align(v2, LV_ALIGN_TOP_LEFT, 36, 164);

    // --- Fila: URL ---
    lv_obj_t* k3 = lv_label_create(card);
    lv_label_set_text(k3, "ABRIR EN EL NAVEGADOR");
    lv_obj_set_style_text_font(k3, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(k3, lv_color_hex(C_DIM), 0);
    lv_obj_set_style_text_letter_space(k3, 2, 0);
    lv_obj_align(k3, LV_ALIGN_TOP_LEFT, 36, 232);

    String url = String("http://") + ap_ip;
    lv_obj_t* v3 = lv_label_create(card);
    lv_label_set_text(v3, url.c_str());
    lv_obj_set_style_text_font(v3, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(v3, lv_color_hex(C_TEXT), 0);
    lv_obj_align(v3, LV_ALIGN_TOP_LEFT, 36, 248);

    // Botón CANCELAR
    lv_obj_t* btn = lv_btn_create(card);
    lv_obj_set_size(btn, 160, 36);
    lv_obj_align(btn, LV_ALIGN_BOTTOM_MID, 0, -24);
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_BORDER), 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(C_ACCENT), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn, 6, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, [](lv_event_t*) {
        Serial.println("[Portal] Cancelado — reiniciando");
        delay(100);
        ESP.restart();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t* btn_lbl = lv_label_create(btn);
    lv_label_set_text(btn_lbl, "CANCELAR");
    lv_obj_set_style_text_font(btn_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(btn_lbl, lv_color_hex(C_TEXT), 0);
    lv_obj_set_style_text_letter_space(btn_lbl, 2, 0);
    lv_obj_center(btn_lbl);

    lv_scr_load_anim(scr, LV_SCR_LOAD_ANIM_FADE_ON, 300, 0, false);
}
