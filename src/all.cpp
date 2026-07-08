#include <Arduino.h>
#include <IRremote.hpp>
#include "DHT.h"

//constants 

float Favtemp = 33.00;
bool isTimerActive = false;           
unsigned long timerStartTime = 0; 
const unsigned long duration10Min = 600000; // 10 minutes target window (in ms)

const int INDICATOR_PIN = 2; // In-built LED indicator (GPIO 2)
const int IR_SEND_PIN = 4; // GPIO 4 use to connect to transistor's base
const int AC_FREQUENCY_KHZ = 38; 
const int Temp_Pin = 23; // GPIO 23 is used to sence data from DHT22

#define DHTPIN Temp_Pin   
#define DHTTYPE DHT22    // Defining the DHT22 (AM2302) sensor

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

const uint16_t rawTurnOn[] = {
    9050, 4500, 550, 1700, 550, 1700, 550, 600,
    500, 600, 550, 550, 550, 550, 550, 1700,
    550, 1700, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 550, 550, 600,
    550, 1700, 550, 550, 550, 550, 550, 550,
    550, 600, 550, 550, 550, 550, 550, 550,
    550, 550, 550, 600, 550, 550, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 1700, 550, 550,
    550, 600, 550, 550, 550, 550, 550, 550,
    550, 550, 550, 600, 550, 550, 550, 1700,
    550, 550, 550, 550, 550, 600, 500, 600,
    550, 550, 550, 550, 550, 1700, 550, 550,
    550, 600, 550, 550, 550, 550, 550, 550,
    550, 550, 600, 550, 550, 550, 550, 550,
    550, 550, 550, 600, 500, 600, 550, 550,
    550, 550, 550, 550, 550, 600, 550, 550,
    550, 550, 550, 550, 550, 550, 550, 600,
    550, 550, 550, 550, 550, 1700, 550, 550,
    550, 550, 550, 600, 550, 550, 550, 550,
    550, 550, 550, 600, 500, 600, 550, 550,
    550, 550, 550, 1700, 550, 550, 550, 600,
    550, 550, 550, 550, 550, 550, 550, 550,
    550, 600, 550, 550, 550, 550, 550, 1700,
    550, 550, 550, 600, 500, 1700, 550, 1700,
    550, 1700, 550
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
    550, 550, 600
};

//Sending functions

void turnOn() {
    Serial.println(F("[IR] Sending: AC TURN ON"));
    
    size_t signalLength = sizeof(rawTurnOn) / sizeof(rawTurnOn[0]);
    IrSender.sendRaw(rawTurnOn, signalLength, AC_FREQUENCY_KHZ);
}

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

void MyTimer() {
  isTimerActive = true;
  timerStartTime = millis(); // Record the exact timestamp when this happened
  Serial.println(F("[TIMER] 10-minute window has started. Sensor has been shut."));
}

void wrap() {
    start30MinTimer();
    MyTimer();
    return;
}

// Temp functions




// setup loop

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(INDICATOR_PIN, OUTPUT);

    for (int i = 1; i <= 3; i++) //small 600ms pulsing of led to indicate the initialisation.
    {
        digitalWrite(INDICATOR_PIN, HIGH); 
        delay(100);
        digitalWrite(INDICATOR_PIN, LOW); 
        delay(100);
    }
    
    IrSender.begin(IR_SEND_PIN, INDICATOR_PIN);
    Serial.println(F("Controler started.\n"));
    dht.begin();

}

void loop() {
    
    if (false == isTimerActive) {
        
        // 2sec delay as the sensor is slow
        delay(2000);

        // Read humidity (percentage)
        float humidity = dht.readHumidity();
        // Read temperature as Celsius (the default)
        float temperature = dht.readTemperature();

        // Check if any reads failed and exit early (to try again)
        if (isnan(humidity) || isnan(temperature)) {
            Serial.println(F("Failed to read from DHT sensor! Check your connections."));
            return;
        }

        float heatIndex = dht.computeHeatIndex(temperature, humidity, false); //calculates the "feels like temperature"

        if (heatIndex >= Favtemp) {
            Serial.print("Feels like: ");
            Serial.print(heatIndex);
            Serial.println("C");
            wrap();
        }

        return;
    }
    

    else{
        unsigned long currentTime = millis();
        if (isTimerActive && (currentTime - timerStartTime >= duration10Min)) {
            isTimerActive = false; // Flip the switch off
            Serial.println(F("[TIMER] 10 minutes are up! Starting sensor."));
        }
        return;
    }
    
}
