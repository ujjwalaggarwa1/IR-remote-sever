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

// Network Credentials Profile
#include "secrets.h"
const char* ssid     = SECRET_SSID;
const char* password = SECRET_PASS;

// Dynamic Application Configurations
bool devMode = false; // Loaded from internal storage on startup

float favTemp = 33.00;
float minTemp = 27.00;
bool isTimerActive = false;           
unsigned long timerStartTime = 0; 
const unsigned long duration10Min = 600000; // 10 minutes target window (in ms)

// Hardware GPIO Definitions
const int INDICATOR_PIN = 2;       // Built-in LED indicator (GPIO 2)
const int IR_SEND_PIN = 4;         // Transistor Base terminal link for IR emitter
const int CONFIG_PIN = 18;         // Physical configuration pin (Short to GND to toggle devMode)
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

const uint16_t rawTimer30Min[] = {
    9300, 4250, 800, 1450, 800, 1450, 800, 300,
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
    550, 1700, 600
};

// Hardware Signal Execution Functions
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

// Memory Operations
void initMemory() {
    prefs.begin("ac-config", false); 
    // Reads "devModeState". Fallback defaults to false if empty
    devMode = prefs.getBool("devModeState", false); 
    prefs.end(); 
    
    Serial.print(F("[STORAGE] Boot Configuration Initialized. devMode = "));
    Serial.println(devMode ? "TRUE" : "FALSE");
}

// Operational Logic Handlers
void MyTimer() {
    isTimerActive = true;
    timerStartTime = millis(); 
}

void wrap() {
    start30MinTimer();
    MyTimer();
    Serial.println(F("[TIMER] 10-minute window has started. Sensor logic paused."));
}

float checkReading() {
    delay(2000); 
    float humidity = dht.readHumidity(); 
    float temperature = dht.readTemperature(); 

    if (isnan(humidity) || isnan(temperature)) {
        Serial.println(F("Failed to read from DHT sensor! Check your connections."));
        return -999.9;  
    }

    float heatIndex = dht.computeHeatIndex(temperature, humidity, false); 

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

// Worker Task 1: Non-blocking Network Sync Engine
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
            // Set for IST (+5:30 Offset = 19800 seconds)
            configTime(19800, 0, "pool.ntp.org");
            
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
    
    // Shut off internal radio infrastructure once sync is locked to reduce thermal footprints
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println(F("[Core 0] TimeSync routine complete. Thread deleted."));
    vTaskDelete(NULL); 
}

// Worker Task 2: Ground Pin Short Dynamic Config Interrupter
void pinWatchTask(void * parameter) {
    pinMode(CONFIG_PIN, INPUT_PULLUP);

    while (true) {
        // Drop to LOW matches a ground connection event
        if (digitalRead(CONFIG_PIN) == LOW) {
            vTaskDelay(50 / portTICK_PERIOD_MS); // Software debounce verification filter
            
            if (digitalRead(CONFIG_PIN) == LOW) {
                devMode = !devMode; // Flip active execution boolean state
                
                // Mount and update Flash memory map allocations securely
                prefs.begin("ac-config", false);
                prefs.putBool("devModeState", devMode);
                prefs.end();
                
                Serial.print(F("[Core 0] Intercept: Pins shorted! Toggled devMode to: "));
                Serial.println(devMode ? "ON" : "OFF");

                // Toggle internal indicator light to act as a tangible visual signal
                digitalWrite(INDICATOR_PIN, HIGH);
                vTaskDelay(300 / portTICK_PERIOD_MS);
                digitalWrite(INDICATOR_PIN, LOW);

                // Trap process until operator removes short connection wire to prevent rapid state cycling
                while (digitalRead(CONFIG_PIN) == LOW) {
                    vTaskDelay(100 / portTICK_PERIOD_MS);
                }
            }
        }
        vTaskDelay(100 / portTICK_PERIOD_MS); // Release core execution slice back to system scheduler
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

    // 1. Unpack saved operational parameters from flash memory registers
    initMemory();

    // 2. Spawn Network Handshake thread to run in parallel on Core 0
    xTaskCreatePinnedToCore(timeSyncTask, "TimeSync", 4000, NULL, 1, NULL, 0);

    // 3. Spawn Physical Configuration tracking listener loop onto Core 0
    xTaskCreatePinnedToCore(pinWatchTask, "PinWatch", 2048, NULL, 1, NULL, 0);

    Serial.println(F("Controller initialization routine finalized.\n"));
}

void loop() {
    // Priority Time Gate Roadblock Routing
    if (timeIsSynced) {
        struct tm timeinfo;
        if (getLocalTime(&timeinfo)) {
            // Flatten dynamic hour configurations into standardized system minutes
            int currentMinutes = (timeinfo.tm_hour * 60) + timeinfo.tm_min;

            // Strict evaluation window checking loop (5:00 AM [300 mins] to 7:30 AM [450 mins])
            if (currentMinutes >= 300 && currentMinutes <= 450) {
                if (!morningScriptExecuted) {
                    Serial.println(F("[GATE] Inside morning window. Intercepting core script paths..."));
                    
                    // Trigger your automated action profile here
                    turnOff(); // Shuts down current AC operations at 5:00 AM target mark
                    
                    morningScriptExecuted = true; 
                }
                return; // Early return forces roadblock context, halting DHT sensors down below
            } 
            else {
                // Recycle Execution Gate Latches once out of range boundaries
                if (morningScriptExecuted) {
                    Serial.println(F("[GATE] Target window window elapsed. Restoring core sensor arrays."));
                    morningScriptExecuted = false;
                }
            }
        }
    }

    // Standard Temperature Management Automation Pipeline
    if (false == isTimerActive) {
        float heatIndex = checkReading();
        if (heatIndex == -999.9) { return; }

        if (heatIndex >= favTemp) { 
            wrap(); 
        }
        else if (heatIndex <= minTemp) { 
            turnOff(); 
        }
    }
    else {
        unsigned long currentTime = millis();
        if (isTimerActive && (currentTime - timerStartTime >= duration10Min)) {
            isTimerActive = false; 
            Serial.println(F("[TIMER] 10 minutes are up! Starting sensor."));
            if (true == devMode) { checkReading(); }
        }
    }

    return;
}