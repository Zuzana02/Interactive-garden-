#include <Arduino.h>
#include <FastLED.h>

#define LED_PIN 2
#define NUM_LEDS 44

CRGB leds[NUM_LEDS];

void setup() {
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(80);
}

void loop() {
  for(int i = 0; i < NUM_LEDS; i++) {

    fadeToBlackBy(leds, NUM_LEDS, 50);
    
    leds[i] = CRGB::White;  

    FastLED.show();         
    delay(100);            
  }
}