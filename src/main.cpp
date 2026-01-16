#include <Arduino.h>

#include <FastLED.h>

#define LED_PIN 4       // GPIO4 (D2 на NodeMCU)
#define LED_COUNT 12
#define LED_TYPE WS2815
#define COLOR_ORDER RGB

CRGB leds[LED_COUNT];

void setup() {
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, LED_COUNT);
  FastLED.setBrightness(100);
  delay(100); // даємо ESP стартувати

  // Усі LED червоним
  for (int i = 0; i < LED_COUNT; i++) {
    leds[i] = CRGB::Red;
  }
  FastLED.show();
}

void loop() {}


