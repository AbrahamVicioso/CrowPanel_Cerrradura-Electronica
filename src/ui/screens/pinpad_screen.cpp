/**
 * @file pinpad_screen.cpp
 * @brief Teclado PIN con lockout por intentos fallidos y countdown visual
 */

#include "pinpad_screen.h"
#include "../theme.h"
#include "../../config/config.h"
#include "../../hardware/display.h"

// ============================================
// OBJETOS LVGL
// ============================================

static lv_obj_t* screen_pinpad       = nullptr;
static lv_obj_t* label_pin_display   = nullptr;
static lv_obj_t* label_status        = nullptr;
static lv_obj_t* label_nfc_hint      = nullptr;
static lv_obj_t* label_attempt_dots  = nullptr;  // puntos de intento visual

// ============================================
// ESTADO
// ============================================

static String pinCode    = "";
static String correctPin = DEFAULT_PIN;

// Lockout
static int      failedAttempts       = 0;
static int      maxFailedAttempts    = MAX_FAILED_ATTEMPTS;
static uint32_t lockoutDurationMs    = LOCKOUT_DURATION_MS;
static bool     lockedOut            = false;
static lv_timer_t* lockout_timer     = nullptr;
static int      lockoutSecondsLeft   = 0;
static lv_timer_t* lockout_tick      = nullptr;

// Inactividad
static lv_timer_t* inactivity_timer  = nullptr;
static void (*inactivity_cb)(void)   = nullptr;
#define PINPAD_INACTIVITY_MS 15000

// Callbacks externos
static PinpadSuccessCallback       success_cb       = nullptr;
static PinpadErrorCallback         error_cb         = nullptr;
static PinpadFailedAttemptsCallback failed_atts_cb  = nullptr;

// ============================================
// FORWARD DECLARATIONS
// ============================================
static void reset_inactivity_timer(void);
static void update_attempt_dots(void);
static void show_lockout_state(void);

// ============================================
// HELPERS VISUALES
// ============================================

static void update_attempt_dots(void)
{
    if (!label_attempt_dots) return;

    // Mostrar círculos: rojo para fallidos, gris para restantes
    String dots = "";
    for (int i = 0; i < maxFailedAttempts; i++) {
        if (i > 0) dots += " ";
        dots += (i < failedAttempts) ? LV_SYMBOL_CLOSE : LV_SYMBOL_MINUS;
    }
    lv_label_set_text(label_attempt_dots, dots.c_str());
    lv_obj_set_style_text_color(label_attempt_dots,
        failedAttempts > 0 ? COLOR_ERROR : COLOR_TEXT_MUTED, 0);
}

// ============================================
// LOCKOUT
// ============================================

static void lockout_tick_cb(lv_timer_t* t)
{
    lockoutSecondsLeft--;
    if (lockoutSecondsLeft <= 0) {
        lv_timer_del(t);
        lockout_tick = nullptr;
    }
    if (label_status && lockedOut) {
        char buf[48];
        snprintf(buf, sizeof(buf), "BLOQUEADO — %ds", lockoutSecondsLeft > 0 ? lockoutSecondsLeft : 0);
        lv_label_set_text(label_status, buf);
        lv_obj_set_style_text_color(label_status, COLOR_ERROR, 0);
    }
}

static void lockout_end_cb(lv_timer_t* t)
{
    lv_timer_del(t);
    lockout_timer = nullptr;

    if (lockout_tick) { lv_timer_del(lockout_tick); lockout_tick = nullptr; }

    lockedOut      = false;
    failedAttempts = 0;

    if (label_status) {
        lv_label_set_text(label_status, "Ingrese PIN");
        lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    }
    update_attempt_dots();
    Serial.println("[Pinpad] Lockout terminado");
}

static void show_lockout_state(void)
{
    lockedOut = true;
    lockoutSecondsLeft = (int)(lockoutDurationMs / 1000);

    if (lockout_timer) lv_timer_del(lockout_timer);
    if (lockout_tick)  lv_timer_del(lockout_tick);

    lockout_timer = lv_timer_create(lockout_end_cb, lockoutDurationMs, NULL);
    lv_timer_set_repeat_count(lockout_timer, 1);

    // Tick cada segundo para countdown
    lockout_tick = lv_timer_create(lockout_tick_cb, 1000, NULL);

    if (label_status) {
        char buf[48];
        snprintf(buf, sizeof(buf), "BLOQUEADO — %ds", lockoutSecondsLeft);
        lv_label_set_text(label_status, buf);
        lv_obj_set_style_text_color(label_status, COLOR_ERROR, 0);
    }
    if (label_nfc_hint) lv_obj_add_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);

    Serial.printf("[Pinpad] LOCKOUT por %d s (%d intentos fallidos)\n",
                  lockoutSecondsLeft, failedAttempts);
}

// ============================================
// TIMER CALLBACKS
// ============================================

static void validate_pin_cb(lv_timer_t* t)
{
    lv_timer_del(t);

    if (lockedOut) return;

    if (pinCode == correctPin) {
        // ── Éxito ──
        lv_label_set_text(label_status, "ACCESO OK");
        lv_obj_set_style_text_color(label_status, COLOR_SUCCESS, 0);
        failedAttempts = 0;
        update_attempt_dots();
        if (success_cb) success_cb();
    } else {
        // ── Fallo ──
        failedAttempts++;
        if (failed_atts_cb) failed_atts_cb(failedAttempts);
        update_attempt_dots();

        pinCode = "";
        if (label_pin_display) lv_label_set_text(label_pin_display, "");

        if (failedAttempts >= maxFailedAttempts) {
            show_lockout_state();
            if (error_cb) error_cb();
        } else {
            int restantes = maxFailedAttempts - failedAttempts;
            char buf[48];
            snprintf(buf, sizeof(buf), "PIN incorrecto (%d restante%s)",
                     restantes, restantes == 1 ? "" : "s");
            if (label_status) {
                lv_label_set_text(label_status, buf);
                lv_obj_set_style_text_color(label_status, COLOR_ERROR, 0);
            }
            if (error_cb) error_cb();

            // Restaurar mensaje después de 2s
            lv_timer_t* rt = lv_timer_create([](lv_timer_t* rt2){
                lv_timer_del(rt2);
                if (label_status && !lockedOut) {
                    lv_label_set_text(label_status, "Ingrese PIN");
                    lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
                }
                if (label_nfc_hint) lv_obj_clear_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);
            }, 2000, NULL);
            lv_timer_set_repeat_count(rt, 1);
        }
    }
}

static void inactivity_timer_cb(lv_timer_t* timer)
{
    lv_timer_del(timer);
    inactivity_timer = nullptr;

    if (lockedOut) return;  // Si está bloqueado, no volver a standby

    pinCode = "";
    if (label_pin_display) lv_label_set_text(label_pin_display, "");

    if (inactivity_cb) {
        auto cb = inactivity_cb;
        inactivity_cb = nullptr;
        cb();
    }
}

static void reset_inactivity_timer(void)
{
    if (inactivity_timer) lv_timer_del(inactivity_timer);
    inactivity_timer = lv_timer_create(inactivity_timer_cb, PINPAD_INACTIVITY_MS, NULL);
    lv_timer_set_repeat_count(inactivity_timer, 1);
}

// ============================================
// EVENTOS DE BOTONES
// ============================================

static void btn_num_event_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (lockedOut) return;

    lv_obj_t* btn = lv_event_get_target(e);
    const char* num = lv_label_get_text(lv_obj_get_child(btn, 0));

    if (pinCode.length() < (size_t)PIN_LENGTH) {
        pinCode += num;
        reset_inactivity_timer();

        if (pinCode.length() == 1) {
            if (label_status) lv_obj_add_flag(label_status, LV_OBJ_FLAG_HIDDEN);
            if (label_nfc_hint) lv_obj_add_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);
        }

        // Mostrar asteriscos
        String display = "";
        for (size_t i = 0; i < pinCode.length(); i++) display += "*";
        if (label_pin_display) lv_label_set_text(label_pin_display, display.c_str());

        // Validar al completar PIN_LENGTH dígitos
        if (pinCode.length() == (size_t)PIN_LENGTH) {
            lv_timer_t* vt = lv_timer_create(validate_pin_cb, 300, NULL);
            lv_timer_set_repeat_count(vt, 1);
        }
    }
}

static void btn_clear_event_cb(lv_event_t* e)
{
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (lockedOut) return;

    pinCode = "";
    if (label_pin_display) lv_label_set_text(label_pin_display, "");
    if (label_status) {
        lv_label_set_text(label_status, "Ingrese PIN");
        lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
        lv_obj_clear_flag(label_status, LV_OBJ_FLAG_HIDDEN);
    }
    if (label_nfc_hint) lv_obj_clear_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);
    reset_inactivity_timer();
}

// ============================================
// CREAR PANTALLA
// ============================================

lv_obj_t* pinpad_screen_create(void)
{
    screen_pinpad = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_pinpad, COLOR_PRIMARY, 0);
    lv_obj_clear_flag(screen_pinpad, LV_OBJ_FLAG_SCROLLABLE);

    // ── Header ──────────────────────────────────────────────
    lv_obj_t* header = lv_obj_create(screen_pinpad);
    lv_obj_set_size(header, 800, 60);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, COLOR_SECONDARY, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lock_icon = lv_label_create(header);
    lv_label_set_text(lock_icon, LV_SYMBOL_LOOP);
    lv_obj_set_style_text_font(lock_icon, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lock_icon, COLOR_ACCENT, 0);
    lv_obj_set_pos(lock_icon, 30, 18);

    lv_obj_t* label_title = lv_label_create(header);
    lv_label_set_text(label_title, "CONTROL DE ACCESO");
    lv_obj_set_style_text_font(label_title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_title, COLOR_TEXT, 0);
    lv_obj_set_pos(label_title, 70, 20);

    // ── Panel de estado ──────────────────────────────────────
    lv_obj_t* status_cont = lv_obj_create(screen_pinpad);
    lv_obj_set_size(status_cont, 420, 120);
    lv_obj_set_pos(status_cont, 190, 72);
    lv_obj_set_style_bg_color(status_cont, COLOR_SECONDARY, 0);
    lv_obj_set_style_radius(status_cont, 15, 0);
    lv_obj_set_style_border_width(status_cont, 0, 0);
    lv_obj_set_style_pad_all(status_cont, 10, 0);
    lv_obj_clear_flag(status_cont, LV_OBJ_FLAG_SCROLLABLE);

    label_status = lv_label_create(status_cont);
    lv_label_set_text(label_status, "Ingrese PIN");
    lv_obj_set_style_text_font(label_status, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 8);

    label_pin_display = lv_label_create(status_cont);
    lv_label_set_text(label_pin_display, "");
    lv_obj_set_style_text_font(label_pin_display, &lv_font_montserrat_32, 0);
    lv_obj_set_style_text_color(label_pin_display, COLOR_TEXT, 0);
    lv_obj_align(label_pin_display, LV_ALIGN_CENTER, 0, 5);

    label_nfc_hint = lv_label_create(status_cont);
    lv_label_set_text(label_nfc_hint, "o acerca tu tarjeta NFC");
    lv_obj_set_style_text_font(label_nfc_hint, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(label_nfc_hint, COLOR_TEXT_MUTED, 0);
    lv_obj_align(label_nfc_hint, LV_ALIGN_BOTTOM_MID, 0, -8);

    // ── Puntos de intento ────────────────────────────────────
    label_attempt_dots = lv_label_create(screen_pinpad);
    lv_label_set_text(label_attempt_dots, "");
    lv_obj_set_style_text_font(label_attempt_dots, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(label_attempt_dots, COLOR_TEXT_MUTED, 0);
    lv_obj_set_pos(label_attempt_dots, 0, 202);
    lv_obj_set_width(label_attempt_dots, 800);
    lv_obj_set_style_text_align(label_attempt_dots, LV_TEXT_ALIGN_CENTER, 0);
    update_attempt_dots();

    // ── Teclado numérico ─────────────────────────────────────
    const char* labels[] = {"1","2","3","4","5","6","7","8","9"};
    const int   bw = 80, bh = 56, sp = 12;
    const int   sx = 260, sy = 220;

    for (int i = 0; i < 9; i++) {
        int row = i / 3, col = i % 3;
        lv_obj_t* btn = lv_btn_create(screen_pinpad);
        lv_obj_set_size(btn, bw, bh);
        lv_obj_set_pos(btn, sx + col*(bw+sp), sy + row*(bh+sp));
        lv_obj_set_style_bg_color(btn, COLOR_BUTTON, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, COLOR_BUTTON_PRESSED, LV_STATE_PRESSED);
        lv_obj_set_style_radius(btn, 12, 0);
        lv_obj_set_style_border_width(btn, 0, 0);

        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, labels[i]);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_24, 0);
        lv_obj_set_style_text_color(lbl, COLOR_TEXT, 0);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, btn_num_event_cb, LV_EVENT_CLICKED, NULL);
    }

    // Botón C (borrar)
    lv_obj_t* btn_c = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_c, bw, bh);
    lv_obj_set_pos(btn_c, sx, sy + 3*(bh+sp));
    lv_obj_set_style_bg_color(btn_c, COLOR_ERROR, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_c, lv_color_hex(0xc0392b), LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_c, 12, 0);
    lv_obj_set_style_border_width(btn_c, 0, 0);
    lv_obj_t* lbl_c = lv_label_create(btn_c);
    lv_label_set_text(lbl_c, LV_SYMBOL_BACKSPACE);
    lv_obj_set_style_text_font(lbl_c, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_c, COLOR_TEXT, 0);
    lv_obj_center(lbl_c);
    lv_obj_add_event_cb(btn_c, btn_clear_event_cb, LV_EVENT_CLICKED, NULL);

    // Botón 0
    lv_obj_t* btn_0 = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_0, bw, bh);
    lv_obj_set_pos(btn_0, sx + (bw+sp), sy + 3*(bh+sp));
    lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON, LV_PART_MAIN);
    lv_obj_set_style_bg_color(btn_0, COLOR_BUTTON_PRESSED, LV_STATE_PRESSED);
    lv_obj_set_style_radius(btn_0, 12, 0);
    lv_obj_set_style_border_width(btn_0, 0, 0);
    lv_obj_t* lbl_0 = lv_label_create(btn_0);
    lv_label_set_text(lbl_0, "0");
    lv_obj_set_style_text_font(lbl_0, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(lbl_0, COLOR_TEXT, 0);
    lv_obj_center(lbl_0);
    lv_obj_add_event_cb(btn_0, btn_num_event_cb, LV_EVENT_CLICKED, NULL);

    // Botón OK (decorativo — validación es automática)
    lv_obj_t* btn_ok = lv_btn_create(screen_pinpad);
    lv_obj_set_size(btn_ok, bw, bh);
    lv_obj_set_pos(btn_ok, sx + 2*(bw+sp), sy + 3*(bh+sp));
    lv_obj_set_style_bg_color(btn_ok, COLOR_SUCCESS, LV_PART_MAIN);
    lv_obj_set_style_radius(btn_ok, 12, 0);
    lv_obj_set_style_border_width(btn_ok, 0, 0);
    lv_obj_clear_flag(btn_ok, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_t* lbl_ok = lv_label_create(btn_ok);
    lv_label_set_text(lbl_ok, LV_SYMBOL_OK);
    lv_obj_set_style_text_font(lbl_ok, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(lbl_ok, COLOR_TEXT, 0);
    lv_obj_center(lbl_ok);

    return screen_pinpad;
}

// ============================================
// API PÚBLICA
// ============================================

void pinpad_screen_show(lv_obj_t* from_screen, lv_scr_load_anim_t anim_type, uint32_t duration)
{
    display_turn_on();
    lv_scr_load_anim(screen_pinpad, anim_type, duration, 0, false);
    if (!lockedOut) reset_inactivity_timer();
}

void pinpad_set_success_callback(PinpadSuccessCallback cb)              { success_cb     = cb; }
void pinpad_set_error_callback(PinpadErrorCallback cb)                  { error_cb       = cb; }
void pinpad_set_failed_attempts_callback(PinpadFailedAttemptsCallback cb){ failed_atts_cb = cb; }
void pinpad_set_inactivity_callback(void (*cb)(void))                   { inactivity_cb  = cb; }

void pinpad_reset(void)
{
    pinCode = "";
    if (inactivity_timer) { lv_timer_del(inactivity_timer); inactivity_timer = nullptr; }
    if (label_pin_display) lv_label_set_text(label_pin_display, "");
    if (label_status) {
        lv_label_set_text(label_status, "Ingrese PIN");
        lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
        lv_obj_clear_flag(label_status, LV_OBJ_FLAG_HIDDEN);
    }
    if (label_nfc_hint) lv_obj_clear_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);
}

std::string pinpad_get_pin(void)
{
    return pinCode.c_str();
}

void pinpad_set_correct_pin(const String& pin)
{
    correctPin = pin;
    Serial.printf("[Pinpad] PIN actualizado\n");
}

void pinpad_set_max_failed_attempts(int max)
{
    maxFailedAttempts = (max < 1) ? 1 : max;
    update_attempt_dots();
}

void pinpad_set_lockout_duration_ms(uint32_t ms)
{
    lockoutDurationMs = ms;
}

void pinpad_reset_failed_attempts(void)
{
    failedAttempts = 0;
    if (lockedOut) {
        if (lockout_timer) { lv_timer_del(lockout_timer); lockout_timer = nullptr; }
        if (lockout_tick)  { lv_timer_del(lockout_tick);  lockout_tick  = nullptr; }
        lockedOut = false;
        if (label_status) {
            lv_label_set_text(label_status, "Ingrese PIN");
            lv_obj_set_style_text_color(label_status, COLOR_TEXT_SECONDARY, 0);
        }
        if (label_nfc_hint) lv_obj_clear_flag(label_nfc_hint, LV_OBJ_FLAG_HIDDEN);
    }
    update_attempt_dots();
    Serial.println("[Pinpad] Intentos fallidos reseteados");
}

bool pinpad_is_locked_out(void)
{
    return lockedOut;
}

void pinpad_show_status(const char* message, lv_color_t color)
{
    if (label_status) {
        lv_label_set_text(label_status, message);
        lv_obj_set_style_text_color(label_status, color, 0);
    }
}
