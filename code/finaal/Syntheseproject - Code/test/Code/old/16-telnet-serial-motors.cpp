#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "TelnetCommand.h"
#include "TelnetCommand.cpp"
#include "StreamCommand.h"
#include "StreamCommand.cpp"

#define SerialPort Serial
#define Baudrate 115200
const int motor1PWM1 = 16;
const int motor1PWM2 = 17; 
const int motor2PWM1 = 18;
const int motor2PWM2 = 19; 

const char* ssid = "DESKTOP-MAINPC";
const char* pswd = "blub0123456789";

int port = 2121;
int sensors[] = {33, 32, 35, 34, 39, 36};
int normalised[6];
bool run = true;
float debugPosition;
unsigned long previous;
unsigned long calculationTime;
unsigned long cycleTime = 1000000;

void unknownCommand(char *command);
void handleTelnet();
void onDebug();
void onSetTelnet();
void onSetStream();
void onCalibrateTelnet();
void onCalibrateStream();
void onRun();

WiFiServer telnetServer(port);
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
} params;

#define EEPROM_SIZE sizeof(param_t)

void setup() 
{
  SerialPort.begin(Baudrate);
  delay(100);

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
  SerialPort.println(port);

  telnetCommand.addCommand("debug", onDebug);
  telnetCommand.addCommand("set", onSetTelnet);
  telnetCommand.addCommand("calibrate", onCalibrateTelnet);
  telnetCommand.setDefaultHandler(unknownCommand);

  streamCommand.addCommand("debug", onDebug);
  telnetCommand.addCommand("set", onSetStream);
  telnetCommand.addCommand("calibrate", onCalibrateStream);
  streamCommand.setDefaultHandler(unknownCommand);

  //EEPROM Lezen
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();

  //Cycle time reset alleen nodig indien cycle time onder calculationtime komt
  //params.cycleTime = 1000000;
  //SerialPort.println("Forced a cycle time reset, last EEPROM data skipped");
}

void loop() 
{
  handleTelnet();
  streamCommand.readStream();
  unsigned long current = micros();

  if (current - previous >= cycleTime) 
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

    //Aansturen van de motoren
    output = constrain(output, -510, 510);
    int powerLeft = 0;
    int powerRight = 0;

    if (run) if (output >= 0)
    {
      powerLeft = constrain(params.power + params.diff * output, -255, 255);
      powerRight = constrain(powerLeft - output, -255, 255);
      powerLeft = powerRight + output;
      Serial.println("running state 1");
    }
    else
    {
      powerRight = constrain(params.power + params.diff * output, -255, 255);
      powerLeft = constrain(powerRight - output, -255, 255);
      powerRight = powerLeft + output;
      Serial.println("running state 2");
    }

    analogWrite(motor1PWM1, powerLeft > 0 ? powerLeft : 0);
    analogWrite(motor1PWM2, powerLeft > 0 ? powerLeft : 0);
    analogWrite(motor2PWM1, powerRight > 0 ? powerRight : 0);
    analogWrite(motor2PWM2, powerRight > 0 ? powerRight : 0);
    Serial.println(powerLeft);
    Serial.println(powerRight);
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
  
  if (strcmp(param, "cycle") == 0) params.cycleTime = atol(value);
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

  // Write to EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onSetTelnet()
{
  char* param = telnetCommand.next();
  char* value = telnetCommand.next();   
  
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

  //Sensorwaardes genormaliseerd
  telnetClient.print("normalised values: ");
  for (int i = 0; i < 6; ++i) {telnetClient.print(normalised[i]); telnetClient.print(" ");}
  telnetClient.println();

  //Varia
  telnetClient.print("Position: ");
  telnetClient.println(debugPosition);
  telnetClient.print("Power: ");
  telnetClient.println(params.power);
  telnetClient.print("Diff: ");
  telnetClient.println(params.diff);
  telnetClient.print("Kp: ");
  telnetClient.println(params.kp);
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