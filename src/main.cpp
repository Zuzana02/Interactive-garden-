#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>

#define LED_PIN 4
#define NUM_LEDS 294
CRGB leds[NUM_LEDS];

WebServer server(80);

void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
}

int mapZigZag(int webId) {
    int row = webId / 42; 
    int col = webId % 42; 

    if (row % 2 == 0) {
        return webId; 
    } else {
        return (row * 42) + (41 - col);
    }
}

void handleSetLed() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int webId = server.arg("id").toInt();
        int state = server.arg("state").toInt();

        int physicalId = mapZigZag(webId);

        if (physicalId >= 0 && physicalId < NUM_LEDS) {
            if (state == 1) {
                // Iba ju "rozsvietime" naplno. O zhasínanie sa postará loop()!
                leds[physicalId] = CRGB::White;
            }
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

    if(!LittleFS.begin()) { 
        Serial.println("FS Error: Could not mount LittleFS"); 
        return; 
    }

    WiFi.softAP("MyGarden", "12345678");
    
    server.on("/", handleRoot);
    server.on("/set", handleSetLed);
    
    server.begin();
    Serial.println("Server is running at 192.168.4.1");
}

void loop() {
    server.handleClient();

    // MAGICKÁ FASTLED ANIMÁCIA
    EVERY_N_MILLISECONDS(30) { 
        // Každých 30ms stiahni jas všetkých LEDiek o 15 (z 255)
        // Čím menšie číslo (napr. 5), tým pomalšie to bude zhasínať
        fadeToBlackBy(leds, NUM_LEDS, 15); 
        FastLED.show();
    }
}