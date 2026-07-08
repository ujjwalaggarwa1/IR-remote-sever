#include <Arduino.h>
#include <IRremote.hpp>
#include "DHT.h"

// WiFi and Time Core Systems
#include <WiFi.h>
#include <time.h>

// Non-Volatile Memory System
#include <Preferences.h>
Preferences prefs;

// Time Synchronization & Gating Control States
bool timeIsSynced = false;
bool morningScriptExecuted = false;

// Network credentials
#include "secrets.h"
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// Dynamic Application Configurations
bool devMode = false; // Loaded from internal storage on startup. given value here for idk XD

float favTemp = 33.00;
float minTemp = 29.00;

// ============================================================================
// DRY TIMER ARCHITECTURE
// ============================================================================
struct HardwareTimer {
    bool active;
    unsigned long startTime;
    const unsigned long duration;
};

// Instantiating our two isolated timer objects cleanly using the same blueprint
HardwareTimer sensorPauseTimer = {false, 0, 600000}; // 10 minutes (in ms)
HardwareTimer offSignalTimer   = {false, 0, 60000};  // 1 minute (in ms)

// Hardware GPIO Definitions
const int INDICATOR_PIN = 2;       // Built-in LED indicator (GPIO 2)
const int IR_SEND_PIN = 4;         // send signal to ir-led
const int CONFIG_PIN = 18;         // Physical configuration input pin (Pushed to Pullup)
const int CONFIG_GND_PIN = 19;     // Software-controlled current sink (Acts as Ground)
const int AC_FREQUENCY_KHZ = 38; 
const int Temp_Pin = 23;           // Data feed wire connection for DHT22 sensor

#define DHTPIN Temp_Pin   
#define DHTTYPE DHT22              // Declaring the DHT22 (AM2302) sensor target

DHT dht(DHTPIN, DHTTYPE);

// ============================================================================
// RAW AC CLONE ARRAYS (211 elements each)
// ============================================================================

const uint16_t rawTurnOff[] = {
    9300, 4200, 850, 1450, 800, 1450, 750, 350,
    700, 450, 650, 450, 650, 450, 700, 1550,
    650, 1600, 650, 450, 650, 450, 700, 450,
    650, 1600, 600, 500, 650, 450, 650, 450,
    650, 1600, 650, 450, 650, 500, 600, 500,
    600, 500, 600, 500, 600, 550, 600, 500,
    600, 500, 600, 500, 600, 550, 550, 550,
    550, 550, 600, 500, 600, 500, 600, 550,
    550, 550, 600, 500, 600, 500, 600, 500,
    600, 550, 550, 550, 600, 1650, 550, 550,
    600, 500, 600, 500, 600, 550, 550, 550,
    600, 500, 600, 500, 600, 500, 600, 1650,
    600, 550, 550, 550, 600, 500, 600, 500,
    600, 500, 600, 550, 550, 1700, 550, 550,
    600, 500, 600, 500, 600, 500, 600, 550,
    550, 550, 600, 500, 600, 500, 600, 500,
    600, 550, 550, 550, 600, 500, 600, 500,
    600, 550, 550, 550, 550, 550, 600, 500,
    600, 500, 600, 550, 550, 550, 600, 500,
    600, 500, 600, 500, 600, 550, 550, 550,
    600, 500, 600, 500, 600, 500, 600, 550,
    550, 550, 600, 500, 600, 500, 600, 550,
    550, 550, 550, 1700, 550, 550, 600, 1650,
    600, 500, 600, 500, 600, 500, 600, 550,
    550, 550, 600, 500, 600, 500, 600, 550,
    550, 550, 550, 1700, 550, 550, 600, 1650,
    600, 1650, 600
};

// const uint16_t rawTurnOn[] = {
//     9300, 4500, 550, 1700, 550, 1700, 550, 600,
//     500, 600, 550, 550, 550, 550, 550, 1700,
//     550, 1700, 550, 550, 550, 600, 550, 550,
//     550, 550, 550, 550, 550, 550, 550, 600,
//     550, 1700, 550, 550, 550, 550, 550, 550,
//     550, 600, 550, 550, 550, 550, 550, 550,
//     550, 550, 550, 600, 550, 550, 550, 550,
//     550, 550, 550, 550, 550, 600, 550, 550,
//     550, 550, 550, 550, 550, 600, 550, 550,
//     550, 550, 550, 550, 550, 1700, 550, 550,
//     550, 600, 550, 550, 550, 550, 550, 550,
//     550, 550, 550, 600, 550, 550, 550, 1700,
//     550, 550, 550, 550, 550, 600, 500, 600,
//     550, 550, 550, 550, 550, 1700, 550, 550,
//     550, 600, 550, 550, 550, 550, 550, 550,
//     550, 550, 600, 550, 550, 550, 550, 550,
//     550, 550, 550, 600, 500, 600, 550, 550,
//     550, 550, 550, 550, 550, 600, 550, 550,
//     550, 550, 550, 550, 550, 550, 550, 600,
//     550, 550, 550, 550, 550, 1700, 550, 550,
//     550, 550, 550, 600, 550, 550, 550, 550,
//     550, 550, 550, 600, 500, 600, 550, 550,
//     550, 550, 550, 1700, 550, 550, 550, 600,
//     550, 550, 550, 550, 550, 550, 550, 550,
//     550, 600, 550, 550, 550, 550, 550, 1700,
//     550, 550, 550, 600, 500, 1700, 550, 1700,
//     550, 1700, 550
// };

const uint16_t rawTimer30Min[] = {
    9300, 4350, 800, 1450, 800, 1450, 800, 300,
    750, 400, 700, 400, 650, 500, 650, 1600,
    650, 1600, 600, 500, 650, 450, 650, 450,
    650, 500, 600, 500, 600, 500, 600, 500,
    600, 1650, 600, 500, 600, 550, 550, 550,
    550, 550, 550, 550, 600, 550, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 1700, 550, 550,
    550, 550, 550, 550, 600, 1700, 550, 1650,
    600, 1650, 600, 1700, 550, 500, 600, 1650,
    600, 500, 600, 550, 550, 550, 550, 550,
    550, 550, 600, 550, 550, 1700, 550, 550,
    550, 550, 550, 550, 600, 500, 600, 550,
    550, 550, 550, 550, 550, 550, 600, 500,
    600, 550, 550, 550, 550, 550, 600, 550,
    550, 550, 550, 550, 550, 550, 550, 550,
    600, 500, 600, 550, 550, 550, 550, 550,
    550, 550, 600, 500, 600, 1650, 600, 1650,
    600, 550, 550, 550, 550, 550, 600, 500,
    600, 500, 600, 550, 550, 550, 550, 550,
    600, 500, 600, 1700, 550, 550, 550, 1700,
    550, 1700, 550, 550, 550, 550, 550, 550,
    600, 500, 600, 550, 550, 1700, 550, 1700,
    550, 1700, 550, 550, 550, 550, 550, 1700,
    550, 550, 600
};

//Sending functions

// void turnOn() {
//     Serial.println(F("[IR] Sending: AC TURN ON"));
//     size_t signalLength = sizeof(rawTurnOn) / sizeof(rawTurnOn[0]);
//     IrSender.sendRaw(rawTurnOn, signalLength, AC_FREQUENCY_KHZ);
// }

void turnOff() {
    Serial.println(F("[IR] Sending: AC TURN OFF"));
    size_t signalLength = sizeof(rawTurnOff) / sizeof(rawTurnOff[0]);
    IrSender.sendRaw(rawTurnOff, signalLength, AC_FREQUENCY_KHZ);
}

void start30MinTimer() {
    Serial.println(F("[IR] Sending: AC 30-MIN TIMER"));
    size_t signalLength = sizeof(rawTimer30Min) / sizeof(rawTimer30Min[0]);
    IrSender.sendRaw(rawTimer30Min, signalLength, AC_FREQUENCY_KHZ);
}

//Memory function

void initMemory() {
    prefs.begin("ac-config", false); 
    // Reads "devModeState". Fallback defaults to false if empty
    devMode = prefs.getBool("devModeState", false); 
    prefs.end(); 
    
    Serial.print(F("[STORAGE] Boot Configuration Initialized. devMode = "));
    Serial.println(devMode ? "TRUE" : "FALSE");
}

void startTimer(HardwareTimer &t) {
    t.active = true;
    t.startTime = millis();
}

bool isTimerExpired(HardwareTimer &t) {
    if (!t.active) return false;
    
    if (millis() - t.startTime >= t.duration) {
        t.active = false; // Reset container state automatically
        return true;
    }
    return false;
}

// Core Execution Triggers
void wrap() {
    start30MinTimer();
    startTimer(sensorPauseTimer);
    Serial.println(F("[TIMER] 10-minute sensor pause engine engaged."));
}

void triggerOffSequence() {
    turnOff();
    startTimer(offSignalTimer);
    Serial.println(F("[TIMER] 1-minute OFF verification sequence engaged."));
}

float checkReading() {
    delay(2000); // 2sec delay as the sensor is slow
    float humidity = dht.readHumidity(); // Read humidity (percentage)
    float temperature = dht.readTemperature(); // Read temperature as Celsius (the default)

    // Check if any reads failed and exit early (to try again)
    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Failed to read from DHT sensor! Check your connections."));
        return -999.9;  // value to represent null/none
    }

    float heatIndex = dht.computeHeatIndex(temperature, humidity, false); //calculates the "feels like temperature"

    if (true == devMode) {
        Serial.print("Feels like: ");
        Serial.print(heatIndex);
        Serial.println("C");
    }

    return heatIndex;
}


// ============================================================================
// BACKGROUND WORKER THREADS (CORE 0 ASYNC OPERATORS)
// ============================================================================

void timeSyncTask(void * parameter) {
    while (!timeIsSynced) {
        Serial.println(F("[Core 0] Requesting Wi-Fi Handshake..."));
        WiFi.begin(ssid, password);
        
        int timeoutCounter = 0;
        while (WiFi.status() != WL_CONNECTED && timeoutCounter < 20) {
            vTaskDelay(500 / portTICK_PERIOD_MS);
            timeoutCounter++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(F("[Core 0] Connection established. Fetching NTP network packet..."));
            configTime(19800, 0, "pool.ntp.org"); // IST +5:30 offset
            
            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
                timeIsSynced = true; 
                Serial.println(F("[Core 0] System hardware clock successfully synchronized."));
            }
        }

        if (!timeIsSynced) {
            Serial.println(F("[Core 0] Synchronize failed. Killing radio, retrying in 30 seconds..."));
            WiFi.disconnect(true);
            vTaskDelay(30000 / portTICK_PERIOD_MS); 
        }
    }
    
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    vTaskDelete(NULL); 
}

void pinWatchTask(void * parameter) {
    // 1. Establish D19 as our constant Software Ground point
    pinMode(CONFIG_GND_PIN, OUTPUT);
    digitalWrite(CONFIG_GND_PIN, LOW);

    // 2. Configure D18 to watch for the short circuit tie-in to D19
    pinMode(CONFIG_PIN, INPUT_PULLUP);

    while (true) {
        if (digitalRead(CONFIG_PIN) == LOW) {
            vTaskDelay(50 / portTICK_PERIOD_MS); // Filter out electrical debounce bounce
            
            if (digitalRead(CONFIG_PIN) == LOW) {
                devMode = !devMode; 
                
                prefs.begin("ac-config", false);
                prefs.putBool("devModeState", devMode);
                prefs.end();
                
                Serial.print(F("[Core 0] Intercept: Side-by-side pins shorted! devMode: "));
                Serial.println(devMode ? "ON" : "OFF");

                digitalWrite(INDICATOR_PIN, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
                digitalWrite(INDICATOR_PIN, LOW);

                // Trap until the physical jumper/short wire is removed
                while (digitalRead(CONFIG_PIN) == LOW) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); 
    }
}

// ============================================================================
// SYSTEM LIFECYCLE CONTROLLERS (CORE 1 EXECUTORS)
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(INDICATOR_PIN, OUTPUT);

    for (int i = 1; i <= 3; i++) {
        digitalWrite(INDICATOR_PIN, HIGH); 
        delay(100);
        digitalWrite(INDICATOR_PIN, LOW); 
        delay(100);
    }
    
    IrSender.begin(IR_SEND_PIN, INDICATOR_PIN);
    dht.begin();

    initMemory();

    // Spawn Core 0 background dependencies
    xTaskCreatePinnedToCore(timeSyncTask, "TimeSync", 4000, NULL, 1, NULL, 0);
    xTaskCreatePinnedToCore(pinWatchTask, "PinWatch", 2048, NULL, 1, NULL, 0);

    Serial.println(F("Controller initialization routine finalized.\n"));
}

void loop() {
    // 1. Morning Roadblock Routing Gate (5:00 AM to 7:30 AM)
    if (timeIsSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            int currentMinutes = (timeinfo.tm_hour * 60) + timeinfo.tm_min;

            if (currentMinutes >= 300 && currentMinutes <= 450) {
                if (!morningScriptExecuted) {
                    Serial.println(F("[GATE] Morning window hit. Halting primary execution lines."));
                    turnOff(); 
                    morningScriptExecuted = true; 
                }
                return; // Blocks sensors from executing down below
            } 
            else {
                if (morningScriptExecuted) {
                    Serial.println(F("[GATE] Morning window complete. Relinquishing tracking control."));
                    morningScriptExecuted = false;
                }
            }
        }
    }

    // ========================================================================
    // PARALLEL TIMER COUNTDOWN ENGINES
    // ========================================================================
    
    if (isTimerExpired(sensorPauseTimer)) {
        Serial.println(F("[TIMER] 10 minutes are up! Resuming primary background tracking."));
        if (true == devMode) { checkReading(); }
    }

    if (isTimerExpired(offSignalTimer)) {
        Serial.println(F("[TIMER] 1 minute up! Cleared safety block."));
        if (true == devMode) { checkReading(); }
    }

    // ========================================================================
    // AUTOMATION DISPATCH PIPELINE
    // ========================================================================
    
    // Core automation only steps forward if NO tracking countdown clocks are currently ticked active
    if (!sensorPauseTimer.active && !offSignalTimer.active) {
        float heatIndex = checkReading();
        if (heatIndex == -999.9) { return; }

        if (heatIndex >= favTemp) { 
            wrap(); 
        }
        else if (heatIndex <= minTemp) { 
            triggerOffSequence(); // Engages the new 1-minute tracking clock
        }
    }

    return;
}