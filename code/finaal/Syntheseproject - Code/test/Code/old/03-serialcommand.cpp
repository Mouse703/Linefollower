#include <Arduino.h>
#include <SerialCommand.h>

#define Baudrate 115200
int ledPin = 12;

bool debug;
unsigned long previous;

SerialCommand sCmd; // SerialCommand object declaration

void commandUnknown(const char *command);
void commandSet();
void commandLedON();
void commandLedOFF();

void setup()
{
  Serial.begin(Baudrate); // Set serial baudrate at 115200
  sCmd.addCommand("set", commandSet);
  sCmd.setDefaultHandler(commandUnknown);
  Serial.println("Ready");
}

void loop()
{
  sCmd.readSerial(); // Listen and process serial commands
  unsigned long current = micros();
  if (current - previous >= 1000000)
  {
    previous = current;
    if (debug) Serial.println("loop still running");
  }
}

void commandUnknown(const char *command)
{
  Serial.println("Unknown command");
}

void commandSet()
{
  char* param = sCmd.next();
  char* value = sCmd.next();

  if (strcmp(param, "debug") == 0)
  {
    if (strcmp(value, "on") == 0) debug = true;
    else if (strcmp(value, "off") == 0) debug = false;
  }
}

void commandLedON() 
{
  digitalWrite(ledPin, HIGH);
  Serial.println("LED is ON");
}

void commandLedOFF() 
{
  digitalWrite(ledPin, LOW);
  Serial.println("LED is OFF");
}