#include <Arduino.h>
#include <WiFi.h>

unsigned long previousMillis = 0;
unsigned long previousMillis1 = 0;

void setup() 
{
  Serial.begin(115200);
}

void loop() 
{
  unsigned long currentMillis = micros();
  unsigned long currentMillis1 = millis();
  unsigned long elapsedTime = currentMillis - previousMillis;

  if (currentMillis1 - previousMillis1 >= 1000)
  {
    previousMillis1 = currentMillis1;
    Serial.print("Cycle Time: ");
    Serial.print(elapsedTime);
    Serial.println(" microseconds");
  }
  previousMillis = currentMillis;
}

//Should be around 9-8 microseconds