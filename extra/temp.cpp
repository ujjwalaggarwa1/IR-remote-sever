#include "DHT.h"

#define DHTPIN 23        // Connected to D23 on your 30-pin ESP32
#define DHTTYPE DHT22    // Defining the DHT 22 (AM2302) sensor

DHT dht(DHTPIN, DHTTYPE);

void setup() {
  Serial.begin(115200);  // Open serial communications at 115200 baud
  Serial.println(F("DHT22 Sensor Test"));

  dht.begin();
}

void loop() {
  // Wait 2 seconds between measurements
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

  // Print the data to the Serial Monitor
  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  |  "));
  Serial.print(F("Temperature: "));
  Serial.print(temperature);
  Serial.println(F("°C"));

  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print(F("Feels like: "));
  Serial.print(heatIndex);
  Serial.println(F("°C"));
}