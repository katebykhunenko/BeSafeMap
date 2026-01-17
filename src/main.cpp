#include <Arduino.h>

#include <FastLED.h>

#define LED_PIN 2       // GPIO2 (D4 на NodeMCU)
#define LED_COUNT 12

CRGB leds[LED_COUNT];

void setup() {
  FastLED.addLeds<WS2815, LED_PIN, GRB>(leds, LED_COUNT);
  FastLED.setBrightness(50); 

  for (int i = 0; i < LED_COUNT; i++) {
    //leds[i].setHue(i * 255 / LED_COUNT);
    leds[i].setRGB(255, 255, 0);
  }
  FastLED.show();
}

void loop() {}


