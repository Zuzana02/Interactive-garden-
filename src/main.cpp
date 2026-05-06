#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>

#define LED_PIN 4
#define NUM_LEDS 44

CRGB leds[NUM_LEDS];

const char* ssid = "MyGarden";
const char* password = "12345678";
WebServer server(80);

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleOn() {
  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB::White;
  }
  FastLED.show();
  server.send(200, "text/plain", "ON");
}

void handleOff() {
  FastLED.clear();
  FastLED.show();
  server.send(200, "text/plain", "OFF");
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  // init filesystem
  if(!LittleFS.begin()){
    Serial.println("LittleFS error");
    return;
  }

  WiFi.softAP(ssid, password);
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/on", handleOn);
  server.on("/off", handleOff);

  server.begin();
}

void loop() {
  server.handleClient();
}