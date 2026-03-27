#include <Arduino.h>
// ========== INCLUDES ==========
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <Adafruit_NeoPixel.h>


// ========== DEFINES & SETTINGS ==========

// ---------- Pins -----------
#define LED_PIN D7 // D7

// ---------- Settings
#define REGIONS_COUNT 130
#define LED_COUNT 369
#define REQUEST_INTERVAL 10000 // 10 секунд
#define BRIGHTNESS 200
const char* alertServerUrl = "http://192.168.0.145:8000/data";
const char* ap_ssid = "BeSafeMap";
const char* ap_password = "12345678";
const byte DNS_PORT = 53;
const uint8_t ledMap[LED_COUNT] = {
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  103, 103, 103,
  118, 118, 118,
  117, 117, 117,
  22, 22, 22,
  19, 19, 19,
  23, 23, 23,
  25, 25, 25,
  115, 115,
  18, 18, 18, 18,
  26, 26, 26,
  21, 21,
  127, 127,
  129,
  126, 126,
  128, 128, 128,
  24, 24,
  20, 20, 20, 20,
  15, 15, 15, 15,
  53, 53, 53, 53, 53,
  79, 79, 79,
  97, 97,
  98, 98, 98,
  95, 95,
  54, 54, 54, 54, 54,
  57, 57,
  56, 56, 56, 56,
  55, 55, 55,
  93, 93, 93,
  92, 92,
  94, 94,
  96, 96, 96,
  88, 88, 88,
  84, 84,
  87, 87, 87,
  85, 85, 85,
  111, 111, 111, 111, 111, 111, 111,
  114, 114, 114,
  107, 107, 107,
  112, 112,
  86, 86,
  78, 78, 78, 78,
  113, 113, 113,
  49, 49, 49,
  44, 44, 44,
  45, 45,
  1, 1,
  47, 47,
  29, 29, 29,
  28, 28, 28,
  30, 30, 30,
  83, 83,
  80, 80,
  11, 11, 11, 11, 11,
  10, 10,
  8, 8, 8, 8,
  9, 9,
  82, 82,
  81, 81,
  62, 62, 62, 62,
  63, 63,
  58, 58,
  61, 61,
  60, 60,
  64, 64,
  90, 90, 90,
  106, 106, 106,
  27, 27, 27,
  43, 43, 43, 43,
  46, 46,
  48, 48, 48,
  123, 123, 123, 123,
  76, 76,
  77, 77, 77, 77,
  122, 122, 122,
  120, 120, 120, 120,
  121, 121,
  6, 6,
  4, 4, 4,
  104, 104, 104, 104, 104,
  89, 89, 89,
  91, 91,
  59, 59,
  41, 41,
  38, 38, 38, 38, 38,
  40, 40,
  39,
  42, 42,
  32,
  35, 35,
  36, 36,
  31, 31,
  34,
  33, 33,
  37, 37,
  108, 108,
  107, 107,
  109, 109, 109,
  105, 105, 105, 105, 105, 105, 105,
  5, 5, 5,
  3, 3, 3, 3, 3, 3,
  2, 2,
  7, 7,
  52, 52, 52, 52,
  53, 53, 53, 53,
  50, 50, 50,
  12, 12, 12,
  14, 14,
  119, 119, 119,
  116, 116, 116,
  17, 17,
  16, 16, 16,
  51, 51, 51, 51, 51,
  67, 67, 67, 67,
  69, 69, 69,
  73, 73,
  70, 70,
  65, 65, 65,
  66, 66, 66,
  99, 99, 99,
  101, 101,
  102, 102,
  100, 100, 100,
  68, 68, 68,
  74, 74,
  72, 72, 72,
  75, 75,
  71, 71
};


// ========== VARIABLES ==========
String wifiSSID;
String wifiPassword;

bool alertStates[REGIONS_COUNT];
uint32_t lastRequest = 0;
uint32_t fadeTimer = 0;

uint8_t SystemState;
uint8_t NetState; // 0 - normal, 1 - AP/config, 2 - not connected, 3 - lost, 255 - fail
uint8_t FetchErrCount = 0;
boolean apiState = 1;

ESP8266WebServer server(80);
DNSServer dnsserver;

uint8_t brightness = BRIGHTNESS;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ400);


// ========== DEFINITIONS ==========
void fillCollor(uint8_t R, uint8_t G, uint8_t B);


// ========== DECLARATIONS ==========
// ---------- EEPROM helpers ----------
void readWiFiFromEEPROM() {
  EEPROM.begin(96);
  char ssid[33];
  char pass[33];

  for (int i = 0; i < 32; i++) ssid[i] = EEPROM.read(i);
  for (int i = 0; i < 32; i++) pass[i] = EEPROM.read(32 + i);

  ssid[32] = '\0';
  pass[32] = '\0';

  wifiSSID = String(ssid);
  wifiPassword = String(pass);

  wifiSSID.trim();
  wifiPassword.trim();
}

void clearEEPROM() {
  EEPROM.begin(96);
  for (int i = 0; i < 96; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
}

// ---------- WEB ----------
// WIFI settings page
String MainHtml =
"<html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"<style>"

"body{"
"font-family:Arial, sans-serif;"
"background:#F7F7F7;"
"padding:20px;"
"}"

".card{"
"background:#ffffff;"
"padding:20px;"
"border-radius:12px;"
"}"

"h2{"
"margin-bottom:10px;"
"}"

"p{"
"font-size:14px;"
"color:#555555;"
"margin-bottom:16px;"
"}"

"input{"
"width:100%;"
"padding:12px;"
"margin:10px 0;"
"border-radius:8px;"
"border:1px solid #cccccc;"
"font-size:16px;"
"}"

"button{"
"width:100%;"
"padding:12px;"
"background:#000000;"
"color:#ffffff;"
"border:none;"
"border-radius:8px;"
"font-size:16px;"
"}"

"</style>"
"</head><body>"

"<div class='card'>"
"<h2>Підключення до карти</h2>"
"<p>Введи Wi-Fi, до якого буде підʼєднуватись карта</p>"

"<form action='/save' method='POST'>"
"<input name='ssid' placeholder='Wi-Fi name'>"
"<input name='pass' type='text' placeholder='Password'>"
"<button>Підключитись</button>"
"</form>"

"</div>"
"</body></html>";
void handleRoot() {
  server.send(200, "text/html", MainHtml);
}

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("pass");

  EEPROM.begin(96);
  for (uint i = 0; i < 32; i++) EEPROM.write(i, i < wifiSSID.length() ? wifiSSID[i] : 0);
  for (uint i = 0; i < 32; i++) EEPROM.write(32 + i, i < wifiPassword.length() ? wifiPassword[i] : 0);
  EEPROM.commit();

  server.send(200, "text/html", "<h2>Збережено! Плата перезавантажується…</h2>");
  delay(2000);
  ESP.restart();
}

boolean fetchAlertData() {
  if (WiFi.status() != WL_CONNECTED) return 0;

  HTTPClient http;
  WiFiClient client;

  http.begin(client, alertServerUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();

    int start = response.indexOf("\"pattern\":\"");
    if (start == -1) {
      http.end();
      return 0;
    }

    start += 11;
    int end = response.indexOf("\"", start);

    String pattern = response.substring(start, end);

    int len = pattern.length();
    if (len > REGIONS_COUNT) {
      len = REGIONS_COUNT;
    }

    for (int i = 0; i < len; i++) {
      char c = pattern.charAt(i);
      alertStates[i] = (c == 'A') ? 1 : 0;
    }
    http.end();
    return 1;
  }
  return 0;
}

// ---------- WiFi ----------
void startSoftAP() {
  fillCollor(0, 255, 0);
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  dnsserver.start(DNS_PORT, "*", WiFi.softAPIP());
  server.onNotFound([]() {
    server.send(200, "text/html", MainHtml);
  });

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void SysStateFetch() {
  if (WiFi.getMode() != WIFI_AP) {
    switch (WiFi.status()) {
    case WL_CONNECTED:
      NetState = 0;
      break;
    case WL_CONNECTION_LOST:
      NetState = 3;
      break;
    case WL_DISCONNECTED:
      NetState = 2;
      break;
    default:
      NetState = 255;
      break;
    }
  } else NetState = 1;
}

// ---------- LEDS ----------
void fillCollor(uint8_t R, uint8_t G, uint8_t B) {
  for (int i = 0; i < LED_COUNT; i++) {
    strip.setPixelColor(i, strip.Color(R, G, B));
  }
  strip.show();
}

void MapColorUpdate(boolean mode) { // mode 0 - allgood, 1 - old data
  if (!mode) {
    brightness = BRIGHTNESS;
    for (int i = 0; i < LED_COUNT; i++) {
      if (alertStates[ledMap[i]] == 1) {
        strip.setPixelColor(i, 0xff0000);
      } else {
        strip.setPixelColor(i, 0x00ff00);
      }
    }
  } else {
    for (int i = 0; i < LED_COUNT; i++) {
      if (alertStates[ledMap[i]] == 1) {
        strip.setPixelColor(i, 0xff0000);
      } else {
        strip.setPixelColor(i, 0xffff00);
      }
    }
  }
  strip.setBrightness(brightness);
  strip.show();
}

void fade() {
  static boolean fadeDir;
  if (fadeTimer - millis() >= 100) {
    fadeTimer = millis();
    fadeDir ? brightness-- : brightness++;
    if (brightness <= 1 || brightness >= 255) fadeDir = !fadeDir;
  }
  strip.setBrightness(brightness);
  strip.show();
}

void setup() {
  strip.begin();
  strip.setBrightness(brightness);
  fillCollor(0, 0, 255);

  readWiFiFromEEPROM();

  if (wifiSSID.length() == 0 || wifiPassword.length() == 0) { // EEPROM порожній → Soft-AP
    startSoftAP();
  } else { // Підключаємося до Wi-Fi
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    fillCollor(255, 255 , 0);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) { // timeout 30 сек
      delay(500);
    }
    fillCollor(0, 0, 255);
    if (WiFi.status() != WL_CONNECTED) {
      clearEEPROM();
      delay(500);
      startSoftAP();
    }
  }
  fadeTimer = millis();
}

void loop() {
  SysStateFetch();
  if (NetState == 1) {
    dnsserver.processNextRequest();
    server.handleClient();
  }

  if (NetState == 0) {
    if (millis() - lastRequest >= REQUEST_INTERVAL) {
      lastRequest = millis();

      if (fetchAlertData()) {
        FetchErrCount = 0;
        apiState = 1;
        MapColorUpdate(0);
      } else {
        if (FetchErrCount < 5) {
          FetchErrCount++;
        } else {
          apiState = 0;
          MapColorUpdate(1);
        }
      }
    }

    if (!apiState) fade();
  } 
}
