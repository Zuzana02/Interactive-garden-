#define FASTLED_ALLOW_INTERRUPTS 0
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

// Pridáme si dve premenné, ktoré budú fungovať ako "pamäť" pre ESP
bool ledStateChanged = false; // Hovorí, či sa niečo zmenilo
bool isLedOn = false;         // Hovorí, či chceme svietiť alebo nie

void handleRoot() {
  File file = LittleFS.open("/index.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleOn() {
  Serial.println("Button ON was pushed"); 
  isLedOn = true;         // Zapamätaj si, že chceme zapnúť
  ledStateChanged = true; // Daj signál slučke loop(), že má niečo spraviť
  server.send(200, "text/plain", "ON");
}

void handleOff() {
  Serial.println("Button OF was pushed"); 
  isLedOn = false;        // Zapamätaj si, že chceme vypnúť
  ledStateChanged = true; // Daj signál slučke loop(), že má niečo spraviť
  server.send(200, "text/plain", "OFF");
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(30); // Nechajme zatiaľ jas na 30 pre test
  FastLED.clear();           // Pre istotu všetko zhasneme pri štarte
  FastLED.show();

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
  server.handleClient(); // ESP sa stará o web

  // Magický trik: Posielame signál neustále každých 50 milisekúnd
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate > 50) {
    if (isLedOn) {
      fill_solid(leds, NUM_LEDS, CRGB::White); // Rýchlejší spôsob ako zasvietiť všetky
    } else {
      FastLED.clear();
    }
    
    FastLED.show(); // Pošli dáta do pásika
    lastUpdate = millis();
  }
}