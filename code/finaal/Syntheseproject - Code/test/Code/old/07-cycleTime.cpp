#include <Arduino.h>
#include <WiFi.h>
#include "StreamCommand.h"
#include "StreamCommand.cpp"

#define SerialPort Serial
#define Baudrate 115200

StreamCommand sCmd(SerialPort);
bool debug;
unsigned long previous;

void onSet();
void onUnknownCommand(char *command);

void setup()
{
  SerialPort.begin(Baudrate);

  sCmd.addCommand("set", onSet);
  sCmd.setDefaultHandler(onUnknownCommand);

  SerialPort.println("ready");
}

void loop()
{
  sCmd.readStream(); 
  unsigned long current = micros();
  if (current - previous >= 1000000)
  {
    previous = current;
    if (debug) Serial.println("loop still running");
  }
}

void onUnknownCommand(char *command)
{
  SerialPort.print("unknown command: \"");
  SerialPort.print(command);
  SerialPort.println("\"");
}

void onSet()
{  
  char* param = sCmd.next();
  char* value = sCmd.next();

  if (strcmp(param, "debug") == 0)
  {
    if (strcmp(value, "on") == 0) debug = true;
    else if (strcmp(value, "off") == 0) debug = false;
  }
}