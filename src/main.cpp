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

// MAGIC FUNCTION: Translates logical web clicks to the physical zig-zag wiring
int mapZigZag(int webId) {
    int row = webId / 11; // Find out which of the 4 rows we are in (result 0, 1, 2, or 3)
    int col = webId % 11; // Find out the position within the row (0 to 10)

    if (row % 2 == 0) {
        // Even rows (0 and 2) go normally from left to right
        return webId; 
    } else {
        // Odd rows (1 and 3) go in reverse from right to left
        return (row * 11) + (10 - col);
    }
}

void handleSetLed() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int webId = server.arg("id").toInt();
        int state = server.arg("state").toInt();

        // Use the translator here!
        int physicalId = mapZigZag(webId);

        if (physicalId >= 0 && physicalId < NUM_LEDS) {
            leds[physicalId] = (state == 1) ? CRGB::White : CRGB::Black;
            FastLED.show();
            Serial.printf("Web ID: %d -> Physical ID: %d set to %d\n", webId, physicalId, state);
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
}