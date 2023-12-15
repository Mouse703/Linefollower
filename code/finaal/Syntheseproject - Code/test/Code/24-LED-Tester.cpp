#include <Arduino.h>
#include <WiFi.h>

int leds[] = {25, 26, 27, 14, 15, 21, 23, 22, 5, 4, 0, 12};

void setup()
{
  for (int i = 0; i < 12; ++i) {pinMode(leds[i], OUTPUT);}
}

void loop()
{ 
  for (int i = 0; i < 12; ++i) {digitalWrite(leds[i], HIGH);}
  delay(1000);
  for (int i = 0; i < 12; ++i) {digitalWrite(leds[i], LOW);}
  delay(1000);
  pinMode(8, OUTPUT);
  digitalWrite(8, HIGH);
  delay(1000);
  digitalWrite(8, LOW);
  

}