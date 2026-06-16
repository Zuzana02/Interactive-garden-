#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <math.h>

#define LED_PIN 4
#define NUM_LEDS 294 
CRGB leds[NUM_LEDS];

WebServer server(80);

// Max ripples
#define MAX_RIPPLES 5 

// Structure to define what a single ripple looks like
struct Ripple {
    bool active = false;
    float radius = 0;
    int centerX = -1;
    int centerY = -1;
};

Ripple ripples[MAX_RIPPLES];



void handleRoot() {
    File file = LittleFS.open("/index.html", "r");
    server.streamFile(file, "text/html");
    file.close();
}

// Translates logical web clicks to physical zig-zag wiring
int mapZigZag(int webId) {
    int row = webId / 42; 
    int col = webId % 42; 

    // Even rows go normally from left to right, odd rows go in reverse
    if (row % 2 == 0) return webId; 
    else return (row * 42) + (41 - col); 
}

void handleSetLed() {
    if (server.hasArg("id") && server.hasArg("state")) {
        int webId = server.arg("id").toInt();
        int state = server.arg("state").toInt();
        
        int physicalId = mapZigZag(webId);

        if (physicalId >= 0 && physicalId < NUM_LEDS) {
            
            // MODE 1: DRAGGING (Draw single dot only) 
            if (state == 1) {
                leds[physicalId] = CRGB::White;
            } 
            
            // MODE 2: CLICKING (Trigger ripple wave) 
            else if (state == 2) {
                // Additive blend for the starting point
                leds[physicalId] += CRGB::White;
                
                // Find an available slot for a new ripple
                int slot = -1;
                for (int i = 0; i < MAX_RIPPLES; i++) {
                    if (!ripples[i].active) {
                        slot = i;
                        break;
                    }
                }
                
                // If all slots are full, override the oldest wave
                if (slot == -1) {
                    float maxR = -1;
                    for (int i = 0; i < MAX_RIPPLES; i++) {
                        if (ripples[i].radius > maxR) {
                            maxR = ripples[i].radius;
                            slot = i;
                        }
                    }
                }
                
                // Start the new ripple animation in the found slot
                ripples[slot].centerX = webId % 42;
                ripples[slot].centerY = webId / 42;
                ripples[slot].radius = 0;
                ripples[slot].active = true;
            }
        }
    }
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    // Safety limit to prevent high current draw during testing
    FastLED.setBrightness(100); 
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
}

void loop() {
    server.handleClient();

    // Trigger animation frame every 30ms
    EVERY_N_MILLISECONDS(30) { 
        
        fadeToBlackBy(leds, NUM_LEDS, 12); 
        
        // Scale factor for circle (rows are 7cm apart, LEDs are 1cm apart)
        float Y_SCALE = 7.0;

        for (int i = 0; i < MAX_RIPPLES; i++) {
            
            if (ripples[i].active) {
                
                // Smooth, steady wave expansion
                ripples[i].radius += 0.3; 

                if (ripples[i].radius > 60) { 
                    ripples[i].active = false;
                    continue; 
                }

                for (int y = 0; y < 7; y++) {
                    for (int x = 0; x < 42; x++) {
                        
                        float dx = x - ripples[i].centerX;
                        float dy = (y - ripples[i].centerY) * Y_SCALE;
                        float distance = sqrt(dx*dx + dy*dy);

                        float diff = abs(distance - ripples[i].radius);
                        
                        // --- MAGIC NUMBER 2: SOFT WAVE FRONT ---
                        // Only add light to LEDs that are exactly on the wave's edge (thickness 1.5).
                        // We don't force them to turn off anymore, the fadeToBlackBy handles that.
                        if (diff < 1.5) {
                            int currentWebId = (y * 42) + x;
                            int physId = mapZigZag(currentWebId);
                            
                            // Linear, soft brightness based on how close it is to the exact radius line
                            float intensity = 1.0 - (diff / 1.5);
                            int brightness = 150 * intensity;
                            
                            // Additively blend the light ("charge" the LED up)
                            leds[physId] += CRGB(brightness, brightness, brightness); 
                        }
                    }
                }
            }
        }
        
        FastLED.show();
    }
}