#include <Arduino.h>
#include <IRremote.hpp>

const int INDICATOR_PIN = 2; //D2 (in-built led)
const int IR_SEND_PIN = 4; // The pin to send signal
const int IR_RECEIVE_PIN = 23; //D23 pin

// Raw signal codes



void setup() {
  Serial.begin(115200);
  pinMode(INDICATOR_PIN, OUTPUT);
  digitalWrite(INDICATOR_PIN, HIGH);
  delay(500);
  Serial.println(F("___________________="));
  Serial.println(F("ESP32 AC IR Receiver Initializing. . . ."));

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK);

  Serial.print(F("Ready to receive IR signal at pin D"));
  Serial.println(IR_RECEIVE_PIN);


}

void loop() {
  if (IrReceiver.decode()) {
    Serial.println();
    Serial.println(F("----- New IR Signal Detected -----"));

    Serial.print(F("Protocol: "));
    Serial.println(IrReceiver.getProtocolString());

    Serial.print(F("Address: 0x"));
    Serial.println(IrReceiver.decodedIRData.command, HEX);

    Serial.println(F("Raw Data Summary: "));
    IrReceiver.printIRResultShort(&Serial);
    
    Serial.println(F("\nFull Raw Timings (Use this for ac cloning): "));
    IrReceiver.printIRResultRawFormatted(&Serial, true);

    Serial.println(F("______________________________"));

    IrReceiver.resume();
  }
}
