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
    bool ambient = false; // Flag to distinguish between user touch and background ambient ripples
};

Ripple ripples[MAX_RIPPLES];

// --- STANDBY (AMBIENT) MODE CONFIGURATION ---
unsigned long lastTouchTime = 0;

// Idle timeout set to 10 seconds (10000 ms) for quick testing.
// To make it 3 minutes, change this number to 180000.
const unsigned long STANDBY_TIMEOUT = 10000; 
bool isStandby = false;

// Random crawler variables for the wandering light path
float crawlerX = 21.0;
float crawlerY = 3.5;
float crawlerTargetX = 21.0;
float crawlerTargetY = 3.5;
// ------------------------------------------

void handleRoot() {
    // ANTI-CACHE HEADERS: Forces the browser to always download the freshest HTML file
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "0");

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

        // --- WAKE UP FROM STANDBY ---
        lastTouchTime = millis();
        isStandby = false;

        if (physicalId >= 0 && physicalId < NUM_LEDS) {
            
            // MODE 1: DRAGGING (Draw single dot only) 
            if (state == 1) {
                leds[physicalId] = CRGB::White;
            } 
            
            // MODE 2: CLICKING (Trigger ripple wave) 
            else if (state == 2) {
                leds[physicalId] += CRGB::White;
                
                int slot = -1;
                for (int i = 0; i < MAX_RIPPLES; i++) {
                    if (!ripples[i].active) {
                        slot = i;
                        break;
                    }
                }
                
                if (slot == -1) {
                    float maxR = -1;
                    for (int i = 0; i < MAX_RIPPLES; i++) {
                        if (ripples[i].radius > maxR) {
                            maxR = ripples[i].radius;
                            slot = i;
                        }
                    }
                }
                
                ripples[slot].centerX = webId % 42;
                ripples[slot].centerY = webId / 42;
                ripples[slot].radius = 0;
                ripples[slot].active = true;
                ripples[slot].ambient = false; // Triggered by user, full brightness
            }
        }
    }
    server.send(200, "text/plain", "OK");
}

void setup() {
    Serial.begin(115200);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
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

    lastTouchTime = millis();
}

void loop() {
    server.handleClient();

    // Check for inactivity to enter standby mode
    if (millis() - lastTouchTime > STANDBY_TIMEOUT) {
        isStandby = true;
    }

    // Trigger animation frame every 30ms
    EVERY_N_MILLISECONDS(30) { 
        
        // Always apply smooth fading to create organic tails for everything
        fadeToBlackBy(leds, NUM_LEDS, 12); 
        
        // --- STANDBY EXCLUSIVE GENERATORS ---
        if (isStandby) {
            
            // 1. RANDOM CRAWLER (The wandering path of light)
            crawlerX += (crawlerTargetX - crawlerX) * 0.04;
            crawlerY += (crawlerTargetY - crawlerY) * 0.04;

            if (abs(crawlerTargetX - crawlerX) < 1.0 && abs(crawlerTargetY - crawlerY) < 1.0) {
                crawlerTargetX = random(0, 42);
                crawlerTargetY = random(0, 7);
            }

            int cx = constrain((int)crawlerX, 0, 41);
            int cy = constrain((int)crawlerY, 0, 6);
            leds[mapZigZag(cy * 42 + cx)] += CRGB(25, 25, 25); // Very soft glow

            // 2. RANDOM BACKGROUND RIPPLES (Raindrops)
            if (random(0, 100) == 0) {
                int slot = -1;
                for (int i = 0; i < MAX_RIPPLES; i++) {
                    if (!ripples[i].active) {
                        slot = i;
                        break;
                    }
                }
                if (slot != -1) {
                    ripples[slot].centerX = random(0, 42);
                    ripples[slot].centerY = random(0, 7);
                    ripples[slot].radius = 0;
                    ripples[slot].active = true;
                    ripples[slot].ambient = true; // Mark as ambient for low brightness
                }
            }
        }
        
        // --- UNIVERSAL RIPPLE RENDERER ---
        float Y_SCALE = 7.0;

        for (int i = 0; i < MAX_RIPPLES; i++) {
            if (ripples[i].active) {

                ripples[i].radius += 0.25; 

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
                        
                        float maxDiff = 1.0 + (ripples[i].radius * 0.05);
                        if (maxDiff > 4.0) maxDiff = 4.0; 

                        if (diff < maxDiff) {
                            int currentWebId = (y * 42) + x;
                            int physId = mapZigZag(currentWebId);
                            
                            float intensity = 1.0 - (diff / maxDiff);
                            intensity = intensity * intensity * intensity; 
                            
                            int maxBrightness = ripples[i].ambient ? 35 : 150;
                            int brightness = maxBrightness * intensity;
                            
                            leds[physId] += CRGB(brightness, brightness, brightness); 
                        }
                    }
                }
            }
        }
        
        FastLED.show();
    }
}