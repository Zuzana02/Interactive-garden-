#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>

#define LED_PIN 4
#define NUM_LEDS 44
CRGB leds[NUM_LEDS];

WebServer server(80);

void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
}

// NOVÁ FUNKCIA: Ovládanie konkrétnej LEDky
void handleSetLed() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int id = server.arg("id").toInt();
        int state = server.arg("state").toInt();

        if (id >= 0 && id < NUM_LEDS) {
            // Ak state = 1, biela, inak zhasnúť (čierna)
            leds[id] = (state == 1) ? CRGB::White : CRGB::Black;
            FastLED.show();
            Serial.printf("Nastavujem LED %d na %s\n", id, state == 1 ? "ON" : "OFF");
        }
    }
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(30);
    FastLED.clear();
    FastLED.show();

    if(!LittleFS.begin()) { Serial.println("FS Error"); return; }

    WiFi.softAP("MyGarden", "12345678");
    
    server.on("/", handleRoot);
    server.on("/set", handleSetLed); // Registrácia novej cesty
    
    server.begin();
    Serial.println("Server bezi na 192.168.4.1");
}

void loop() {
    server.handleClient();
}