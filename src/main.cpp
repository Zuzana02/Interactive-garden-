#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 44

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White; // farba
  }
  FastLED.show();
}

void loop() {
}