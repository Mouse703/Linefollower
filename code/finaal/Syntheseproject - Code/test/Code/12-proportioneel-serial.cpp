#include <Arduino.h>
#include <WiFi.h>
#include "StreamCommand.h"
#include "StreamCommand.cpp"
#include <EEPROM.h>

#define SerialPort Serial
#define Baudrate 115200
#define motor1PWM1 12
#define motor1PWM2 14 
#define motor2PWM1 26
#define motor2PWM2 27 

StreamCommand sCmd(SerialPort);
bool debug;
bool run;
unsigned long previous, calculationTime;
int sensors[] = {33, 32, 35, 34, 39, 36};
int normalised[6];
float debugPosition;

void onUnknownCommand(char *command);
void onSet();
void onDebug();
void onCalibrate();

struct param_t 
{
  unsigned long cycleTime;
  int blacks[6];
  int whites[6];
  int power;
  float diff;
  float kp;
} params;

#define EEPROM_SIZE sizeof(param_t)

void setup()
{
  //Serial baudrate instellen
  SerialPort.begin(Baudrate);

  //Commando's toevoegen
  sCmd.addCommand("set", onSet);
  sCmd.addCommand("debug", onDebug);
  sCmd.addCommand("calibrate", onCalibrate);
  sCmd.setDefaultHandler(onUnknownCommand);
  
  //EEPROM Lezen
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();

  //Cycle time reset
  params.cycleTime = 1000000;
  SerialPort.println("Forced a cycle time reset, last EEPROM data skipped");
}

void loop()
{
  sCmd.readStream();
  unsigned long current = micros();

  if (current - previous >= params.cycleTime)
  {
    previous = current;
    
    //Sensorwaardes normaliseren
    for (int i = 0; i < 6; ++i) {normalised[i] = map(analogRead(sensors[i]), params.blacks[i], params.whites[i], 0, 4096);}
    
    //Starten van positie berekening
    int index = 0;
    float position;
    for (int i = 1; i < 6; i++) {if (normalised[i] < normalised[index]) {index = i;}} 

    //Positieberekening in ongeveer mm omzetten
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
    debugPosition = position;

    //Proportioneele regelaar
    float error = -position;
    float output = error * params.kp;

    //Aansturen van de motoren
    output = constrain(output, -510, 510);
    int powerLeft = 0;
    int powerRight = 0;

    if (run) if (output >= 0)
    {
      powerLeft = constrain(params.power + params.diff * output, -255, 255);
      powerRight = constrain(powerLeft - output, -255, 255);
      powerLeft = powerRight + output;
    }
    else
    {
      powerRight = constrain(params.power + params.diff * output, -255, 255);
      powerLeft = constrain(powerRight - output, -255, 255);
      powerRight = powerLeft + output;
    }

    analogWrite(motor1PWM1, powerLeft > 0 ? powerLeft : 0);
    analogWrite(motor1PWM2, powerLeft > 0 ? powerLeft : 0);
    analogWrite(motor2PWM1, powerRight > 0 ? powerRight : 0);
    analogWrite(motor2PWM2, powerRight > 0 ? powerRight : 0);
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
  
  if (strcmp(param, "cycle") == 0) params.cycleTime = atol(value);
  else if (strcmp(param, "power") == 0) params.power = atol(value);
  else if (strcmp(param, "diff") == 0) params.diff = atol(value);
  else if (strcmp(param, "kp") == 0) params.kp = atol(value);

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

  //Sensorwaardes genormaliseerd
  SerialPort.print("normalised values: ");
  for (int i = 0; i < 6; ++i) {SerialPort.print(normalised[i]); SerialPort.print(" ");}
  SerialPort.println();

  //Varia
  SerialPort.print("Position: ");
  SerialPort.println(debugPosition);
  SerialPort.print("Power: ");
  SerialPort.println(params.power);
  SerialPort.print("Diff: ");
  SerialPort.println(params.diff);
  SerialPort.print("Kp: ");
  SerialPort.println(params.kp);
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