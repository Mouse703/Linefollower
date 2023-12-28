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

int sensors[] = {33, 32, 35, 34, 39, 36};


void onUnknownCommand(char *command);
void onSet();
void onDebug();
void onCalibrate();

struct param_t 
{
  unsigned long cycleTime;
  int blacks[6];
  int whites[6];
} params;

#define EEPROM_SIZE sizeof(param_t)

void setup()
{
  SerialPort.begin(Baudrate);

  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.addCommand("calibrate", onCalibrate);
  sCmd.setDefaultHandler(onUnknownCommand);
  
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();

  SerialPort.println("ready");

  //Cycle time reset
  params.cycleTime = 1000000;
  SerialPort.println("Forced a cycle time reset, last EEPROM data skipped");
}

void loop()
{
  sCmd.readStream();
 
  unsigned long current = micros();

  int normalised[6];

  if (current - previous >= params.cycleTime)
  {
    previous = current;
    Serial.print("normalised values: ");
    for (int i = 0; i < 6; ++i) 
    {
      normalised[i] = map(analogRead(sensors[i]), params.blacks[i], params.whites[i], 0, 1000);
      SerialPort.print(normalised[i]);
      SerialPort.print(" ");
    }
    SerialPort.println();
    
    int index = 0;
    float position;
    for (int i = 1; i < 6; i++) {if (normalised[i] < normalised[index]) {index = i;}} 

    if (index == 0) {position = -30;}
    else if (index == 5) {position = 30;}
    else 
    {
      int sNul = normalised[index];
      int sMinEen = normalised[index -1];
      int sPlusEen = normalised[index +1];

      float b = sPlusEen - sMinEen;
      b = b / 2;
      
      float a = sPlusEen - b - sNul;

      position = -b / (2 * a);

      position += index;
      position -= 2.5;
      position *= 15;
    }
    SerialPort.print("Positie: ");
    SerialPort.println(position);
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

  SerialPort.print("Blacks: ");
  for (int i = 0; i < 6; i++)
  {
    SerialPort.print(params.blacks[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" ");

  SerialPort.print("Whites: ");
  for (int i = 0; i < 6; i++)
  {
    SerialPort.print(params.whites[i]);
    SerialPort.print(" ");
  }
  SerialPort.println(" ");
}

void onCalibrate()
{
  char* param = sCmd.next();

  if (strcmp(param, "black") == 0)
  {
    SerialPort.print("start calibrating black... ");
    for (int i = 0; i < 6; i++) params.blacks[i]=analogRead(sensors[i]);
    SerialPort.println("done");
  }
  else if (strcmp(param, "white") == 0)
  {
    SerialPort.print("start calibrating white... ");    
    for (int i = 0; i < 6; i++) params.whites[i]=analogRead(sensors[i]);  
    SerialPort.println("done");      
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}