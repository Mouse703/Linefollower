#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "TelnetCommand.h"
#include "TelnetCommand.cpp"
#include "StreamCommand.h"
#include "StreamCommand.cpp"
#include "UserConfig.h"

#define SerialPort Serial
#define Baudrate 115200

int telnetPort = 2121;
int sensors[] = {33, 32, 35, 34, 39, 36};
int normalised[6];
bool run;
float iTerm = 0;
float lastError = 0;
float debugPosition;
const int motor1PWM1 = 16;
const int motor1PWM2 = 17; 
const int motor2PWM1 = 18;
const int motor2PWM2 = 19; 
unsigned long previous;
unsigned long calculationTime;

void unknownCommand(char *command);
void handleTelnet();
void onRun();
void onDebug();
void onSetTelnet();
void onSetStream();
void onCalibrateTelnet();
void onCalibrateStream();

WiFiServer telnetServer(telnetPort);
WiFiClient telnetClient;
TelnetCommand telnetCommand(SerialPort);
StreamCommand streamCommand(SerialPort);

struct param_t 
{
  unsigned long cycleTime;
  int blacks[6];
  int whites[6];
  int power;
  float diff;
  float kp;
  float ki;
  float kd;
} params;
#define EEPROM_SIZE sizeof(param_t)

void setup() 
{
  SerialPort.begin(Baudrate);
  pinMode(motor1PWM1, OUTPUT);
  pinMode(motor1PWM2, OUTPUT);
  pinMode(motor2PWM1, OUTPUT);
  pinMode(motor2PWM2, OUTPUT);

  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    SerialPort.println("Connecting to WiFi...");
  }

  SerialPort.println("Connected to WiFi");
  SerialPort.print("IP address: ");
  SerialPort.println(WiFi.localIP());

  telnetServer.begin();
  SerialPort.print("Telnet server started on port ");
  SerialPort.println(telnetPort);

  telnetCommand.addCommand("run", onRun);
  telnetCommand.addCommand("debug", onDebug);
  telnetCommand.addCommand("set", onSetTelnet);
  telnetCommand.addCommand("calibrate", onCalibrateTelnet);
  telnetCommand.setDefaultHandler(unknownCommand);

  streamCommand.addCommand("run", onRun);
  streamCommand.addCommand("debug", onDebug);
  streamCommand.addCommand("set", onSetStream);
  streamCommand.addCommand("calibrate", onCalibrateStream);
  streamCommand.setDefaultHandler(unknownCommand);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();

  //Cycle only necessary when cycletime param accidently drops below the
  //params.cycleTime = 1000000;
  //SerialPort.println("Forced a cycle time reset, last EEPROM data skipped");
}

void loop() 
{
  handleTelnet();
  streamCommand.readStream();
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

    //if (normalised[index] > 3000) run = false;

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

    //Integrerende regelaar
    iTerm += params.ki * error;
    iTerm = constrain(iTerm, -510, 510);
    output += iTerm;

    //Differentiërend regelen
    output += params.kd * (error - lastError);
    lastError = error; 

    //Aansturen van de motoren
    output = constrain(output, -510, 510);
    int powerLeft = 0;
    int powerRight = 0;

    if (run) {
    if (output >= 0) 
    {
      powerLeft = constrain(params.power + params.diff * output, -255, 255);
      powerRight = constrain(powerLeft - output, -255, 255);
    } 
    else 
    {
      powerRight = constrain(params.power + params.diff * output, -255, 255);
      powerLeft = constrain(powerRight + output, -255, 255);
    }

    if (powerLeft > 0) 
    {
      analogWrite(motor1PWM1, powerLeft);
      analogWrite(motor1PWM2, 0);
    } 
    else 
    {
      analogWrite(motor1PWM1, 0);
      analogWrite(motor1PWM2, powerLeft);
    }

    if (powerRight > 0) 
    {
      analogWrite(motor2PWM1, powerRight);
      analogWrite(motor2PWM2, 0);
    } 
    else 
    {
      analogWrite(motor2PWM1, 0);
      analogWrite(motor2PWM2, powerRight);
    }

    Serial.println(powerLeft);
    Serial.println(powerRight);
}

  }

  unsigned long difference = micros() - current;
  if (difference > calculationTime) calculationTime = difference;
}

void handleTelnet() 
{
  if (telnetServer.hasClient()) 
  {
    if (!telnetClient || !telnetClient.connected()) 
    {
      if (telnetClient) {telnetClient.stop();}
      telnetClient = telnetServer.available();
      if (telnetClient) 
      {
        Serial.println("New Telnet client connected");
        telnetClient.println("Welcome to ESP32 - Line follower Telnet Server");
      }
    }
  }

  if (telnetClient && telnetClient.connected() && telnetClient.available()) 
  {
    //Lezen van commando's uit de telnetclient.
    telnetCommand.readStream(telnetClient);
  }
  if (telnetClient && !telnetClient.connected()) 
  {
    //Indien telnet verloren, verwittig serial en stop de telnetclient.
    Serial.println("Telnet client disconnected");
    telnetClient.stop();
  }
}

void unknownCommand(char *command)
{
  Serial.print("Unknown command: ");
  Serial.println(command);
  telnetClient.print("Unknown command: ");
  telnetClient.println(command);
}

void onSetStream()
{
  char* param = streamCommand.next();
  char* value = streamCommand.next();   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);
    params.ki *= ratio;
    params.cycleTime = newCycleTime;
    SerialPort.print("New cycle time: ");
    SerialPort.println(params.cycleTime);
    telnetClient.print("New cycle time: ");
    telnetClient.println(params.cycleTime);
  }
  else if (strcmp(param, "ki") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
    SerialPort.print("New ki value: ");
    SerialPort.println(params.ki);
    telnetClient.print("New ki value: ");
    telnetClient.println(params.ki);
  }
  else if (strcmp(param, "kd") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
    SerialPort.print("New kd value: ");
    SerialPort.println(params.kd);
    telnetClient.print("New kd value: ");
    telnetClient.println(params.kd);
  }
  else if (strcmp(param, "power") == 0) params.power = atol(value);
  else if (strcmp(param, "diff") == 0) params.diff = atol(value);
  else if (strcmp(param, "kp") == 0) params.kp = atol(value);
  else if (strcmp(param, "on") == 0) 
  {
    run = true;
    Serial.println("running state true");
  }
  else if (strcmp(param, "off") == 0)
    {
    run = false;
    Serial.println("running state false");
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onSetTelnet()
{
  char* param = telnetCommand.next();
  char* value = telnetCommand.next();   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);
    params.ki *= ratio;
    params.kd /= ratio;
    params.cycleTime = newCycleTime;
    SerialPort.print("New cycle time: ");
    SerialPort.println(params.cycleTime);
    telnetClient.print("New cycle time: ");
    telnetClient.println(params.cycleTime);
  }
  else if (strcmp(param, "ki") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
    SerialPort.print("New ki value: ");
    SerialPort.println(params.ki);
    telnetClient.print("New ki value: ");
    telnetClient.println(params.ki);
  }
  else if (strcmp(param, "kd") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
    SerialPort.print("New kd value: ");
    SerialPort.println(params.kd);
    telnetClient.print("New kd value: ");
    telnetClient.println(params.kd);
  }
  else if (strcmp(param, "power") == 0) 
  {
    params.power = atol(value);
    SerialPort.print("New power value: ");
    SerialPort.println(params.power);
    telnetClient.print("New power value: ");
    telnetClient.println(params.power);
  }
  else if (strcmp(param, "diff") == 0) 
  {
    params.diff = atof(value);
    SerialPort.print("New diff value: ");
    SerialPort.println(params.diff);
    telnetClient.print("New diff value: ");
    telnetClient.println(params.diff);
  }
  else if (strcmp(param, "kp") == 0) 
  {
    params.kp = atof(value);
    SerialPort.print("New kp value: ");
    SerialPort.println(params.kp);
    telnetClient.print("New kp value: ");
    telnetClient.println(params.kp);
  }
  else
  {
    Serial.print("Unknown command: ");
    Serial.println(param);
    telnetClient.print("Unknown command: ");
    telnetClient.println(param);
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onDebug()
{
  //SERIAL
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

  SerialPort.print("normalised values: ");
  for (int i = 0; i < 6; ++i) {SerialPort.print(normalised[i]); SerialPort.print(" ");}
  SerialPort.println();

  SerialPort.print("Position: ");
  SerialPort.println(debugPosition);
  SerialPort.print("Power: ");
  SerialPort.println(params.power);
  SerialPort.print("Diff: ");
  SerialPort.println(params.diff);
  SerialPort.print("Kp: ");
  SerialPort.println(params.kp);
  
  float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
  float ki = params.ki / cycleTimeInSec;
  SerialPort.print("Ki: ");
  SerialPort.println(ki);

  float kd = params.kd * cycleTimeInSec;
  SerialPort.print("Kd: ");
  SerialPort.println(kd);

  //TELNET
  telnetClient.println("Cycle time: " + String(params.cycleTime));
  telnetClient.println("Calculation time: " + String(calculationTime));

  telnetClient.print("Blacks: ");
  for (int i = 0; i < 6; i++)
  {
    telnetClient.print(params.blacks[i]);
    telnetClient.print(" ");
  }
  telnetClient.println(" ");

  telnetClient.print("Whites: ");
  for (int i = 0; i < 6; i++)
  {
    telnetClient.print(params.whites[i]);
    telnetClient.print(" ");
  }
  telnetClient.println(" ");

  telnetClient.print("normalised values: ");
  for (int i = 0; i < 6; ++i) {telnetClient.print(normalised[i]); telnetClient.print(" ");}
  telnetClient.println();

  telnetClient.print("Position: ");
  telnetClient.println(debugPosition);
  telnetClient.print("Power: ");
  telnetClient.println(params.power);
  telnetClient.print("Diff: ");
  telnetClient.println(params.diff);
  telnetClient.print("Kp: ");
  telnetClient.println(params.kp);
  telnetClient.print("Ki: ");
  telnetClient.println(ki);
  telnetClient.print("Kd: ");
  telnetClient.println(kd);
}

void onCalibrateStream()
{
  char* param = streamCommand.next();

  if (strcmp(param, "black") == 0)
  {
    SerialPort.print("start calibrating black... ");
    SerialPort.print("start calibrating black... ");
    for (int i = 0; i < 6; i++) params.blacks[i]=analogRead(sensors[i]);
    SerialPort.println("done");
    SerialPort.println("done");
  }
  else if (strcmp(param, "white") == 0)
  {
    SerialPort.print("start calibrating white... ");
    SerialPort.print("start calibrating white... ");    
    for (int i = 0; i < 6; i++) params.whites[i]=analogRead(sensors[i]);  
    SerialPort.println("done");
    SerialPort.println("done");      
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onCalibrateTelnet()
{
  char* param = telnetCommand.next();

  if (strcmp(param, "black") == 0)
  {
    SerialPort.print("start calibrating black... ");
    SerialPort.print("start calibrating black... ");
    for (int i = 0; i < 6; i++) params.blacks[i]=analogRead(sensors[i]);
    SerialPort.println("done");
    SerialPort.println("done");
  }
  else if (strcmp(param, "white") == 0)
  {
    SerialPort.print("start calibrating white... ");
    SerialPort.print("start calibrating white... ");    
    for (int i = 0; i < 6; i++) params.whites[i]=analogRead(sensors[i]);  
    SerialPort.println("done");
    SerialPort.println("done");      
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onRun()
{
  if (run == true)
  {
    run = false;
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, 0);
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, 0);
  }
  else {run = true;}
}