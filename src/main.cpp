#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>

ESP8266WebServer server(80);

const char* ap_ssid = "AlertMap_Setup";
const char* ap_password = "12345678";

String wifiSSID;
String wifiPassword;

// ---------- читання з EEPROM ----------
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

// ---------- очищення EEPROM ----------
void clearEEPROM() {
  EEPROM.begin(96);
  for (int i = 0; i < 96; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.commit();
  Serial.println("🧹 EEPROM очищено!");
}

// ---------- веб-сторінка ----------
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

void SavePage() {
  // тут зчитуєш ssid і pass з POST
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");
  // зберігаєш у EEPROM і запускаєш підключення до Wi-Fi

  String html =
"<html><head>"
"<meta charset='UTF-8'>"
"<meta name='viewport' content='width=device-width, initial-scale=1'>"
"</head><body>"
"<h2>✅ Дані збережено</h2>"
"<p>Плата намагається підключитись до Wi-Fi</p>"
"<p>Якщо підключення буде успішним — Soft-AP зникне</p>"
"</body></html>";

  server.send(200, "text/html; charset=utf-8", html);
}


// ---------- збереження ----------
void handleSave() {
  wifiSSID = server.arg("ssid");
  wifiPassword = server.arg("pass");

  Serial.println("\n📥 Отримано Wi-Fi дані:");
  Serial.print("SSID: "); Serial.println(wifiSSID);
  Serial.print("Password: "); Serial.println(wifiPassword);

  EEPROM.begin(96);
  for (int i = 0; i < 32; i++) EEPROM.write(i, i < wifiSSID.length() ? wifiSSID[i] : 0);
  for (int i = 0; i < 32; i++) EEPROM.write(32 + i, i < wifiPassword.length() ? wifiPassword[i] : 0);
  EEPROM.commit(); // дуже важливо!

  Serial.print("EEPROM після запису SSID: ");
  for(int i=0;i<32;i++) Serial.print((char)EEPROM.read(i));
  Serial.println();

  server.send(200, "text/html", "<h2>Збережено! Плата перезавантажується…</h2>");
  delay(2000);
  ESP.restart();
}

// ---------- Soft-AP запуск ----------
void startSoftAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);

  Serial.println("❗ Запущено Soft-AP для введення Wi-Fi");
  Serial.print("📡 SSID: "); Serial.println(ap_ssid);
  Serial.println("🌐 Відкрий у браузері: 192.168.4.1");

  server.on("/", handleRoot);
  server.on("/save", handleSave);
  server.begin();
}

void setup() {
  Serial.begin(9600);
  delay(1000);

  Serial.println("\n--- ESP8266 START ---");

  readWiFiFromEEPROM();

  if (wifiSSID.length() == 0 || wifiPassword.length() == 0) {
    // EEPROM порожній → Soft-AP
    startSoftAP();
  } else {
    // Підключаємося до Wi-Fi
    Serial.println("🔐 Знайдено Wi-Fi дані");
    Serial.print("SSID: "); Serial.println(wifiSSID);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

    Serial.print("🔄 Підключення до Wi-Fi");

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 30000) { // timeout 30 сек
      delay(500);
      Serial.print(".");
    }

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
  }
}

void loop() {
  server.handleClient();
}
