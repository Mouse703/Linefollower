#include <Arduino.h>
#include <WiFi.h>
#include "StreamCommand.h"
#include "StreamCommand.cpp"
#include <EEPROM.h>

#define SerialPort Serial
#define Baudrate 115200

StreamCommand sCmd(SerialPort);
bool debug;
unsigned long previous, calculationTime;

void onUnknownCommand(char *command);
void onSet();
void onDebug();

struct param_t {
  unsigned long cycleTime;
} params;

#define EEPROM_SIZE sizeof(param_t)

void setup()
{
  SerialPort.begin(Baudrate);

  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.setDefaultHandler(onUnknownCommand);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();
  //params.cycleTime = 1000000;
  SerialPort.println("ready");
}

void loop()
{
  sCmd.readStream();
 
  unsigned long current = micros();
  if (current - previous >= params.cycleTime)
  {
    previous = current;
    Serial.println("Loop");
    /* code die cyclisch moet uitgevoerd worden programmeer je hier ... */

    /* normaliseren en interpoleren sensor */

    /* pid regeling */

    /* aansturen motoren */
  }

  unsigned long difference = micros() - current;
  if (difference > calculationTime) calculationTime = difference;
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
  else if (strcmp(param, "cycle") == 0) params.cycleTime = atol(value);

  // Write to EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onDebug()
{
  SerialPort.println("Cycle time: " + String(params.cycleTime));
  SerialPort.println("Calculation time: " + String(calculationTime));
}