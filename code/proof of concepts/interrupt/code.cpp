#include <Arduino.h>
#include <WiFi.h>

const int buttonPin = 2;  // Replace with the actual pin where your button is connected
volatile bool isRunning = false;

void buttonInterrupt();

void setup() 
{
  pinMode(17, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
  Serial.begin(115200);
}

void loop() 
{
  if (isRunning)
  {
    digitalWrite(17, HIGH);
    //Serial.println("System is ON");
  } else {
    digitalWrite(17, LOW);
    //Serial.println("System is OFF");
  }
}

void buttonInterrupt() 
{
  delay(50); //Debounce
  isRunning = !isRunning;
}
