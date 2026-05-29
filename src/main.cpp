#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>

#define LED_PIN 4
#define NUM_LEDS 44
CRGB leds[NUM_LEDS];

unsigned long ledTimeouts[NUM_LEDS] = {0};

WebServer server(80);

void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
}

int mapZigZag(int webId) {
    int row = webId / 11; 
    int col = webId % 11; 

    if (row % 2 == 0) {
        return webId; 
    } else {
        return (row * 11) + (10 - col);
    }
}

void handleSetLed() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int webId = server.arg("id").toInt();
        int state = server.arg("state").toInt();

        int physicalId = mapZigZag(webId);

        if (physicalId >= 0 && physicalId < NUM_LEDS) {
            if (state == 1) {
                leds[physicalId] = CRGB::White;
                // Nastavené presne na 3000 milisekúnd (3 sekundy)
                ledTimeouts[physicalId] = millis() + 1000; 
                FastLED.show();
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

    unsigned long currentMillis = millis();
    bool changeDetected = false;

    for (int i = 0; i < NUM_LEDS; i++) {
        if (ledTimeouts[i] != 0 && currentMillis > ledTimeouts[i]) {
            leds[i] = CRGB::Black; 
            ledTimeouts[i] = 0;    
            changeDetected = true;
        }
    }

    if (changeDetected) {
        FastLED.show();
    }
}