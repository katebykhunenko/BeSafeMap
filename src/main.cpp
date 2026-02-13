#include <Arduino.h>
// ========== INCLUDES ==========
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>


// ========== DEFINES & SETTINGS ==========
#define DEBUG // uncomment to enable debug mode

// ---------- Pins -----------
#define LED_PIN 2 //GPIO2 aka D4

// ---------- Settings
#define REGIONS_COUNT 130 // aka LED's count
#define REQUEST_INTERVAL 10000 // 10 секунд
const char* alertServerUrl = "https://10.0.1.41:8000/data";
const char* ap_ssid = "AlertMap_Setup";
const char* ap_password = "12345678";


// ========== VARIABLES ==========
String wifiSSID;
String wifiPassword;

bool alertStates[REGIONS_COUNT];
uint32_t lastRequest = 0;

ESP8266WebServer server(80);



// ========== DEFINITIONS ==========


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
  #ifdef DEBUG
  Serial.println("🧹 EEPROM очищено!");
  #endif
}

// ---------- WEB ----------
// WIFI settings page
void handleRoot() {
  String html =
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

  server.send(200, "text/html", html);
}

void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("pass");

  #ifdef DEBUG
  Serial.println("\n📥 Отримано Wi-Fi дані:");
  Serial.print("SSID: "); Serial.println(wifiSSID);
  Serial.print("Password: "); Serial.println(wifiPassword);
  #endif

  EEPROM.begin(96);
  for (uint i = 0; i < 32; i++) EEPROM.write(i, i < wifiSSID.length() ? wifiSSID[i] : 0);
  for (uint i = 0; i < 32; i++) EEPROM.write(32 + i, i < wifiPassword.length() ? wifiPassword[i] : 0);
  EEPROM.commit();

  #ifdef DEBUG
  Serial.print("EEPROM після запису SSID: ");
  for(int i=0;i<32;i++) Serial.print((char)EEPROM.read(i));
  Serial.println();
  #endif

  server.send(200, "text/html", "<h2>Збережено! Плата перезавантажується…</h2>");
  delay(2000);
  ESP.restart();
}

void fetchAlertData() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  WiFiClient client;

  http.begin(client, alertServerUrl);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String response = http.getString();

    #ifdef DEBUG
    Serial.println("📦 Відповідь сервера:");
    Serial.println(response);
    #endif

    int start = response.indexOf("\"pattern\":\"");
    if (start == -1) {
      #ifdef DEBUG
      Serial.println("❌ pattern не знайдено");
      #endif
      http.end();
      return;
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

    #ifdef DEBUG
    Serial.print("🧠 Стани: ");
    for (int i = 0; i < REGIONS_COUNT; i++) {
      Serial.print(alertStates[i]);
    }
    Serial.println();
    #endif
  } 
  #ifdef DEBUG
  else {
    Serial.println("❌ HTTP помилка");
  }
  #endif

  http.end();
}

// ---------- Soft-AP запуск ----------
void startSoftAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  #ifdef DEBUG
  Serial.println("❗ Запущено Soft-AP для введення Wi-Fi");
  Serial.print("📡 SSID: "); Serial.println(ap_ssid);
  Serial.println("🌐 Відкрий у браузері: 192.168.4.1");
  #endif

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}


// ---------- LEDS + JSON ----------
void MapColorUpdate() {
  for (int i = 0; i < REGIONS_COUNT; i++) {
    Serial.print(alertStates[i]);
  }
  Serial.println();
  /*
  for (int i = 0; i < REGIONS_COUNT; i++) {
    if (alertStates[i] == 1) {
      leds[i].setRGB(255, 0, 0);
    }
    else if (alertStates[i] == 0) {
      leds[i].setRGB(0, 255, 0);
    }
    }
  FastLED.show();
*/
}


void setup() {
  #ifdef DEBUG
  Serial.begin(9600);
  Serial.println("\n--- ESP8266 START ---");
  #endif

  //FastLED.addLeds<WS2815, LED_PIN, GRB>(leds, REGIONS_COUNT);
  //FastLED.setBrightness(50);


  readWiFiFromEEPROM();

  if (wifiSSID.length() == 0 || wifiPassword.length() == 0) { // EEPROM порожній → Soft-AP
    startSoftAP();
  } else { // Підключаємося до Wi-Fi
    #ifdef DEBUG
    Serial.println("🔐 Знайдено Wi-Fi дані");
    Serial.print("SSID: "); Serial.println(wifiSSID);
    Serial.print("🔄 Підключення до Wi-Fi");
    #endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) { // timeout 30 сек
      delay(500);
      #ifdef DEBUG
      Serial.print(".");
      #endif
    }

    #ifdef DEBUG
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\n✅ Wi-Fi підключено!");
      Serial.print("📍 IP адреса: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("\n❌ Не вдалося підключитися до Wi-Fi. Скидаємо EEPROM і запускаємо Soft-AP…");
      clearEEPROM();
      delay(500);
      startSoftAP();
    }
    #else
    if (WiFi.status() != WL_CONNECTED) {
      clearEEPROM();
      delay(500);
      startSoftAP();
    }
    #endif
  }
}

void loop() {
  server.handleClient(); // працює тільки якщо Soft-AP

  if (WiFi.status() == WL_CONNECTED) {
    if (millis() - lastRequest >= REQUEST_INTERVAL) {
      lastRequest = millis();
      fetchAlertData();
      MapColorUpdate();
    }
  }
}


