#include <WiFi.h>
#include <WebServer.h>
#include <FastLED.h>
#include <LittleFS.h>
#include <math.h> // Pridaná knižnica pre matematické funkcie (odmocnina)

#define LED_PIN 4
#define NUM_LEDS 294 
CRGB leds[NUM_LEDS];

WebServer server(80);

// --- PREMENNÉ PRE VLNU (RIPPLE) ---
bool isRippling = false;
float rippleRadius = 0;
int rippleCenterX = -1;
int rippleCenterY = -1;
// ----------------------------------

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
                // 1. Rozsvietime stredový bod dotyku naplno
                leds[physicalId] = CRGB::White;
                
                // 2. Nastavíme počiatočné hodnoty pre novú vlnu
                rippleCenterX = webId % 42; // X súradnica (0 až 41)
                rippleCenterY = webId / 42; // Y súradnica (0 až 6)
                rippleRadius = 0;           // Začíname s nulovým polomerom
                isRippling = true;          // Spustíme animáciu v slučke loop()
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
}

void loop() {
    server.handleClient();

    EVERY_N_MILLISECONDS(30) { 
        // 1. Plynule zhasínaj všetko, čo svieti (vytvára to ten pekný chvost vlny)
        fadeToBlackBy(leds, NUM_LEDS, 25); // Zvýšila som z 15 na 25, aby vlna mizla trošku rýchlejšie a ostrejšie
        
        // 2. Vykresľovanie expandujúcej vlny
        if (isRippling) {
            rippleRadius += 0.8; // Rýchlosť rozširovania vlny (čím väčšie číslo, tým rýchlejšia vlna)

            // Zastav vlnu, ak už vyšla mimo plátna (42 je šírka tvojej matice)
            if (rippleRadius > 45) {
                isRippling = false;
            }

            // Prehľadaj celú mriežku a nájdi LEDky, ktoré ležia na okraji kruhu
            for (int y = 0; y < 7; y++) {
                for (int x = 0; x < 42; x++) {
                    // Výpočet vzdialenosti bodu od stredu vlny (Pytagorova veta)
                    float dx = x - rippleCenterX;
                    float dy = y - rippleCenterY;
                    float distance = sqrt(dx*dx + dy*dy);

                    // Ak je vzdialenosť tejto LEDky zhodná (s toleranciou 1.0) s aktuálnym polomerom vlny
                    if (abs(distance - rippleRadius) < 1.0) {
                        int currentWebId = (y * 42) + x;
                        int physId = mapZigZag(currentWebId);
                        
                        // Rozsvietime okraj vlny na bielo. Používame modifikátor += 
                        // aby vlna "neprebila" stredový bod, ale sčítala sa s ním.
                        leds[physId] += CRGB(150, 150, 150); // Mierne stlmená biela pre krajší efekt
                    }
                }
            }
        }

        FastLED.show();
    }
}