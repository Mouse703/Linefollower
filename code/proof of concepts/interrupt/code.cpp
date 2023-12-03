#include <Arduino.h>
#include <WiFi.h>

const int buttonPin = 2; 
const int led = 15;
volatile bool isRunning = false;

void buttonInterrupt();

void setup() 
{
  pinMode(led, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, FALLING);
  Serial.begin(115200);
}

void loop() 
{
  if (isRunning)
  {
    digitalWrite(led, HIGH);
    Serial.println("System is ON");
  } else {
    digitalWrite(led, LOW);
    Serial.println("System is OFF");
  }
}

void buttonInterrupt() 
{
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 250;
  unsigned long currentTime = millis();

  if (currentTime - lastDebounceTime >= debounceDelay)
  {
    isRunning = !isRunning;
    lastDebounceTime = currentTime;
  }
}
