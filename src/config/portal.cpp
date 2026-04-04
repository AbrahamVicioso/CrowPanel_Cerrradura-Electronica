/**
 * @file portal.cpp
 * @brief Portal de configuración HTTP sobre AP WiFi
 *
 * Implementado con WiFiServer raw (sin WebServer.h) para evitar el error
 * "CreateProcess: No such file or directory" del toolchain en Windows.
 *
 * Dos contraseñas separadas:
 *   - AP WiFi: aleatoria cada sesión (mostrada en pantalla del dispositivo)
 *   - Portal web: fija, guardada en NVS, configurable desde el panel web
 */

#include "portal.h"
#include "storage.h"
#include <WiFi.h>

#define PORTAL_AP_SSID "CerraduraConfig"

static WiFiServer httpServer(80);
static bool      portal_active   = false;
static bool      should_restart  = false;
static uint32_t  restart_at      = 0;
static String    session_ap_pass;  // aleatoria cada sesión (WiFi AP)
static String    web_pass;         // fija, NVS (autenticación del formulario web)

// ============================================
// Generador de contraseña aleatoria
// ============================================

static String generateSessionPass()
{
    // Solo dígitos: fácil de escribir en el celular, sin caracteres ambiguos
    char buf[9];
    for (int i = 0; i < 8; i++) {
        buf[i] = '0' + (char)random(0, 10);
    }
    buf[8] = '\0';
    return String(buf);
}

// ============================================
// Utilidades HTTP
// ============================================

static String urlDecode(const String& s)
{
    String out;
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] == '+') {
            out += ' ';
        } else if (s[i] == '%' && i + 2 < (int)s.length()) {
            char h[3] = { s[i + 1], s[i + 2], '\0' };
            out += (char)strtol(h, nullptr, 16);
            i += 2;
        } else {
            out += s[i];
        }
    }
    return out;
}

static String getField(const String& body, const char* key)
{
    String k = String(key) + "=";
    int s = body.indexOf(k);
    if (s < 0) return "";
    s += k.length();
    int e = body.indexOf('&', s);
    return urlDecode(body.substring(s, e < 0 ? body.length() : e));
}

// ============================================
// HTML (PROGMEM)
// ============================================

static const char HTML_FORM[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Cerradura Config</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#050d1a;color:#f0f6ff;font-family:-apple-system,BlinkMacSystemFont,sans-serif;min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
.card{background:#0a1628;border:1px solid #1e3a5f;border-radius:14px;padding:32px 28px;width:100%;max-width:440px}
h1{color:#3b82f6;font-size:18px;text-align:center;letter-spacing:3px;margin-bottom:6px}
.sub{color:#4a5568;font-size:12px;text-align:center;margin-bottom:24px;letter-spacing:1px}
.err{background:#1a0a0a;border:1px solid #7f1d1d;border-radius:7px;color:#fca5a5;font-size:13px;padding:10px 14px;margin-bottom:20px;text-align:center}
.section{font-size:10px;color:#2d3748;letter-spacing:2px;text-transform:uppercase;margin:22px 0 13px;border-top:1px solid #1e2d40;padding-top:15px}
.section:first-of-type{margin-top:4px;border-top:none;padding-top:0}
.group{margin-bottom:15px}
label{display:block;font-size:11px;color:#3b82f6;letter-spacing:1px;margin-bottom:5px}
input{width:100%;background:#050d1a;border:1px solid #1e3a5f;border-radius:7px;color:#f0f6ff;padding:10px 13px;font-size:14px;outline:none}
input:focus{border-color:#3b82f6}
input::placeholder{color:#2d3748}
.auth-field input{border-color:#3b82f6;background:#060f20}
.hint{font-size:11px;color:#2d3748;margin-top:4px}
button{width:100%;background:#3b82f6;color:#fff;border:none;border-radius:7px;padding:13px;font-size:14px;cursor:pointer;margin-top:10px;letter-spacing:2px;font-weight:600}
button:active{background:#1e40af}
.note{font-size:11px;color:#2d3748;text-align:center;margin-top:12px}
</style>
</head>
<body>
<div class="card">
<h1>CONFIGURACION</h1>
<p class="sub">Cerradura Electronica</p>
{{ERROR}}
<form method="POST" action="/save">
  <div class="section">Contrasena del Portal Web</div>
  <div class="group auth-field">
    <label>CONTRASENA DEL PORTAL (requerida para guardar)</label>
    <input name="p" type="password" placeholder="Ingresa la contrasena del portal" required autofocus>
  </div>
  <div class="section">Red WiFi</div>
  <div class="group">
    <label>SSID</label>
    <input name="ssid" type="text" value="{{ssid}}" placeholder="Nombre de la red" required>
  </div>
  <div class="group">
    <label>CONTRASENA WIFI</label>
    <input name="pass" type="password" placeholder="Dejar vacio para no cambiar">
    <p class="hint">Dejar vacio conserva la contrasena actual</p>
  </div>
  <div class="section">ThingsBoard</div>
  <div class="group">
    <label>SERVIDOR (IP o dominio)</label>
    <input name="tb_server" type="text" value="{{tb_server}}" placeholder="10.0.0.1" required>
  </div>
  <div class="group">
    <label>TOKEN DE ACCESO</label>
    <input name="tb_token" type="text" value="{{tb_token}}" placeholder="Token del dispositivo" required>
  </div>
  <div class="section">Cambiar Contrasena del Portal Web</div>
  <div class="group">
    <label>NUEVA CONTRASENA DEL PORTAL (min. 8 caracteres)</label>
    <input name="new_web_pass" type="password" placeholder="Dejar vacio para no cambiar">
    <p class="hint">Esta es la contrasena que usas en este formulario</p>
  </div>
  <button type="submit">GUARDAR Y REINICIAR</button>
  <p class="note">El dispositivo se reiniciara automaticamente</p>
</form>
</div>
</body>
</html>)rawhtml";

static const char HTML_SUCCESS[] PROGMEM = R"rawhtml(<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Guardado</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{background:#050d1a;color:#f0f6ff;font-family:-apple-system,sans-serif;display:flex;align-items:center;justify-content:center;min-height:100vh}
.card{background:#0a1628;border:1px solid #1e3a5f;border-radius:14px;padding:48px 36px;text-align:center;max-width:380px;width:90%}
.check{font-size:52px;color:#3b82f6;margin-bottom:20px}
h2{font-size:20px;margin-bottom:10px}
p{color:#4a5568;font-size:14px}
</style>
</head>
<body>
<div class="card">
<div class="check">&#10003;</div>
<h2>Configuracion guardada</h2>
<p>Reiniciando en 3 segundos...</p>
</div>
</body>
</html>)rawhtml";

// ============================================
// HTTP helpers
// ============================================

static void sendHttp(WiFiClient& client, int code, const char* status,
                     const char* ctype, const String& body)
{
    client.print("HTTP/1.1 ");
    client.print(code);
    client.print(" ");
    client.println(status);
    client.print("Content-Type: ");
    client.println(ctype);
    client.print("Content-Length: ");
    client.println(body.length());
    client.println("Connection: close");
    client.println();
    client.print(body);
}

static String buildForm(const String& errorMsg)
{
    String page = FPSTR(HTML_FORM);
    if (errorMsg.length() > 0) {
        page.replace("{{ERROR}}",
                     String("<div class=\"err\">") + errorMsg + "</div>");
    } else {
        page.replace("{{ERROR}}", "");
    }
    page.replace("{{ssid}}",      storage_get_wifi_ssid());
    page.replace("{{tb_server}}", storage_get_tb_server());
    page.replace("{{tb_token}}",  storage_get_tb_token());
    return page;
}

// ============================================
// Manejo de clientes HTTP
// ============================================

static void handleClient(WiFiClient& client)
{
    String reqLine = client.readStringUntil('\n');
    bool isPost = reqLine.startsWith("POST");

    int contentLength = 0;
    while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r" || line.length() == 0) break;
        if (line.startsWith("Content-Length: ")) {
            contentLength = line.substring(16).toInt();
        }
    }

    String body;
    if (isPost && contentLength > 0) {
        uint32_t t = millis();
        while (client.connected() && (int)body.length() < contentLength
               && millis() - t < 3000) {
            if (client.available()) body += (char)client.read();
        }
    }

    if (isPost) {
        // Verificar contraseña del portal web
        String pwd = getField(body, "p");
        if (pwd != web_pass) {
            Serial.println("[Portal] Contrasena del portal incorrecta");
            sendHttp(client, 200, "OK", "text/html; charset=utf-8",
                     buildForm("Contrasena incorrecta. Intenta de nuevo."));
            return;
        }

        // Guardar configuración de red
        String ssid         = getField(body, "ssid");
        String pass         = getField(body, "pass");
        String tb_server    = getField(body, "tb_server");
        String tb_token     = getField(body, "tb_token");
        String new_web_pass = getField(body, "new_web_pass");

        if (ssid.length()         > 0) storage_set_wifi_ssid(ssid.c_str());
        if (pass.length()         > 0) storage_set_wifi_pass(pass.c_str());
        if (tb_server.length()    > 0) storage_set_tb_server(tb_server.c_str());
        if (tb_token.length()     > 0) storage_set_tb_token(tb_token.c_str());

        // Cambiar contraseña del portal web (≥8 caracteres)
        if (new_web_pass.length() >= 8) {
            storage_set_ap_password(new_web_pass.c_str());
            web_pass = new_web_pass;
            Serial.println("[Portal] Contrasena del portal web actualizada");
        }

        Serial.println("[Portal] Configuracion guardada. Reiniciando en 3s...");
        sendHttp(client, 200, "OK", "text/html; charset=utf-8", FPSTR(HTML_SUCCESS));

        should_restart = true;
        restart_at     = millis() + 3000;

    } else {
        sendHttp(client, 200, "OK", "text/html; charset=utf-8", buildForm(""));
    }
}

// ============================================
// API pública
// ============================================

void portal_start(void)
{
    // Contraseña del portal web: fija, desde NVS
    web_pass = storage_get_ap_password();

    // Contraseña WiFi AP: aleatoria cada sesión (nunca se guarda)
    randomSeed(millis() ^ (uint32_t)(ESP.getEfuseMac() & 0xFFFFFFFF));
    session_ap_pass = generateSessionPass();

    portal_active  = false;
    should_restart = false;

    // Apagar WiFi completamente antes de cambiar a modo AP
    WiFi.disconnect(false);
    WiFi.mode(WIFI_OFF);
    delay(500);

    // Iniciar AP con contraseña generada
    WiFi.mode(WIFI_AP);
    delay(200);

    bool ok = WiFi.softAP(PORTAL_AP_SSID, session_ap_pass.c_str());
    delay(500);

    if (!ok) {
        Serial.println("[Portal] WARN: softAP fallo, reintentando...");
        WiFi.mode(WIFI_OFF);
        delay(1000);
        WiFi.mode(WIFI_AP);
        delay(300);
        ok = WiFi.softAP(PORTAL_AP_SSID, session_ap_pass.c_str());
        delay(500);
    }

    httpServer.begin();
    portal_active = true;

    Serial.printf("[Portal] softAP: %s\n", ok ? "OK" : "FALLO");
    Serial.printf("[Portal] SSID:   %s\n", PORTAL_AP_SSID);
    Serial.printf("[Portal] Pass:   %s\n", session_ap_pass.c_str());
    Serial.printf("[Portal] IP:     %s\n", WiFi.softAPIP().toString().c_str());
}

void portal_loop(void)
{
    if (!portal_active) return;

    WiFiClient client = httpServer.available();
    if (client) {
        handleClient(client);
        client.stop();
    }

    if (should_restart && millis() >= restart_at) {
        Serial.println("[Portal] Reiniciando...");
        delay(100);
        ESP.restart();
    }
}

bool   portal_is_active(void)   { return portal_active; }
String portal_get_ap_ssid(void) { return String(PORTAL_AP_SSID); }
String portal_get_ap_ip(void)   { return portal_active ? WiFi.softAPIP().toString() : String("192.168.4.1"); }
String portal_get_ap_pass(void) { return session_ap_pass; }  // contraseña WiFi aleatoria
