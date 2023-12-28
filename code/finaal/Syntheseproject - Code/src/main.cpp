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
#define motor1PWM1 16
#define motor1PWM2 17
#define motor2PWM1 18
#define motor2PWM2 19

int telnetPort = 2121;
int sensors[] = {36, 39, 34, 35, 32, 33};
int normalised[6];
float iTerm = 0;
float lastError = 0;
float debugPosition;
const int ledRed[] = {15, 27};
const int ledWhite[] = {22, 25};
const int ledOrangeLeft[] = {14, 26};
const int ledOrangeRight[] = {23, 21};
const int ledStatusBlue = 5;
const int ledStatusGreen = 4;
const int ledStatusOrange = 0;
const int ledStatusRed = 12; 
const int buttonToggleState = 2;
unsigned long previous;
unsigned long calculationTime;
volatile bool running = false;

void unknownCommand(char *command);
void handleTelnet();
void onRunCommand();
void onDebug();
void onSetTelnet();
void onSetStream();
void onCalibrateTelnet();
void onCalibrateStream();
void standby();
void buttonInterrupt();
void offTrack();

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
  int brightnessRed;
  int brightnessWhite;
  int brightnessOrange;
  int brightnessStatus;
  int blinkLimit;
  float diff;
  float kp;
  float ki;
  float kd;
} params;
#define EEPROM_SIZE sizeof(param_t)

void setup() 
{
  digitalWrite(5, LOW);
  digitalWrite(4, LOW);
  digitalWrite(0, LOW);
  digitalWrite(12, LOW);

  pinMode(ledStatusBlue, OUTPUT);
  pinMode(ledStatusGreen, OUTPUT);
  pinMode(ledStatusOrange, OUTPUT);
  pinMode(ledStatusRed, OUTPUT);
  pinMode(buttonToggleState, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonToggleState), buttonInterrupt, FALLING);
  for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i) {pinMode(ledRed[i], OUTPUT);}
  for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i) {pinMode(ledWhite[i], OUTPUT);}
  for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i) {pinMode(ledOrangeLeft[i], OUTPUT);}
  for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i) {pinMode(ledOrangeRight[i], OUTPUT);}

  SerialPort.begin(Baudrate);
  WiFi.setHostname(host);
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) 
  {
    SerialPort.println("Connecting to WiFi...");
    analogWrite(ledStatusBlue, 100);
    delay(1000);
    analogWrite(ledStatusBlue, 0);
    delay(1000);
  }
  SerialPort.println("Connected to WiFi");
  SerialPort.print("IP address: ");
  SerialPort.println(WiFi.localIP());
  analogWrite(ledStatusBlue, 100);

  telnetServer.begin();
  SerialPort.print("Telnet server started on port ");
  SerialPort.println(telnetPort);

  telnetCommand.addCommand("run", onRunCommand);
  telnetCommand.addCommand("debug", onDebug);
  telnetCommand.addCommand("set", onSetTelnet);
  telnetCommand.addCommand("calibrate", onCalibrateTelnet);
  telnetCommand.setDefaultHandler(unknownCommand);

  streamCommand.addCommand("run", onRunCommand);
  streamCommand.addCommand("debug", onDebug);
  streamCommand.addCommand("set", onSetStream);
  streamCommand.addCommand("calibrate", onCalibrateStream);
  streamCommand.setDefaultHandler(unknownCommand);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();

  analogWrite(ledStatusGreen, params.brightnessStatus);
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

    if ((normalised[index] > 3500) && (running == true)) {running = false; analogWrite(ledStatusRed, params.brightnessStatus);}

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

    //Differentierend regelen
    output += params.kd * (error - lastError);
    lastError = error; 

    //Aansturen van de motoren
    output = constrain(output, -510, 510);
    int powerLeft = 0;
    int powerRight = 0;

    if (running) 
    {
      if (output >= 0) 
      {
        powerLeft = constrain(params.power + params.diff * output, -255, 255);
        powerRight = constrain(powerLeft - output, -255, 255);
        powerLeft = powerRight + output;
      }  
      else 
      {
        powerRight = constrain(params.power - params.diff * output, -255, 255);
        powerLeft = constrain(powerRight + output, -255, 255);
        powerRight = powerLeft - output;
      }

      if (position > params.blinkLimit) 
      {
        for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i)  {analogWrite(ledOrangeLeft[i], params.brightnessOrange);}
        for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i)  {analogWrite(ledOrangeRight[i], 0);}
      }
      else if (position < -(params.blinkLimit)) 
      {
        for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i)  {analogWrite(ledOrangeRight[i], params.brightnessOrange);}
        for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i)  {analogWrite(ledOrangeLeft[i], 0);}
      }
      else 
      {
        for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i)  {analogWrite(ledOrangeRight[i], 0);}
        for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i)  {analogWrite(ledOrangeLeft[i], 0);}
      }

      analogWrite(motor1PWM1, powerLeft > 0 ? powerLeft : 0);
      analogWrite(motor1PWM2, powerLeft < 0 ? -powerLeft : 0);
      analogWrite(motor2PWM1, powerRight > 0 ? powerRight : 0);
      analogWrite(motor2PWM2, powerRight < 0 ? -powerRight : 0);
    }
    else {standby();}
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
    telnetCommand.readStream(telnetClient);
  }
  if (telnetClient && !telnetClient.connected()) 
  {
    Serial.println("Telnet client disconnected");
    telnetClient.stop();
    analogWrite(ledStatusBlue, 0);
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

  SerialPort.print("Received param: ");
  SerialPort.println(param);
  SerialPort.print("Received value: ");
  SerialPort.println(value);   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);
    params.ki *= ratio;
    params.kd /= ratio;
    params.cycleTime = newCycleTime;
    SerialPort.print("New cycle time: ");
    SerialPort.println(params.cycleTime);
  }
  else if (strcmp(param, "ki") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
    SerialPort.print("New ki value: ");
    SerialPort.println(params.ki);
  }
  else if (strcmp(param, "kd") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
    SerialPort.print("New kd value: ");
    SerialPort.println(params.kd);
  }
  else if (strcmp(param, "power") == 0) 
  {
    params.power = atol(value);
    SerialPort.print("New power value: ");
    SerialPort.println(params.power);
  }
  else if (strcmp(param, "diff") == 0)
  {
    params.diff = atol(value);
    SerialPort.print("New diff value: ");
    SerialPort.println(params.diff);
  }
  else if (strcmp(param, "kp") == 0)
  {
    params.kp = atol(value);
    SerialPort.print("New kp value: ");
    SerialPort.println(params.kp);
  }
  else if (strcmp(param, "ledred") == 0)
  {
    params.brightnessRed = atol(value);
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i) {analogWrite(ledRed[i], params.brightnessRed);}
    SerialPort.print("New red led brightness: ");
    SerialPort.println(params.brightnessRed);
  }
  else if (strcmp(param, "ledwhite") == 0)
  {
    params.brightnessWhite = atol(value);
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i) {analogWrite(ledWhite[i], params.brightnessWhite);}
    SerialPort.print("New red led brightness: ");
    SerialPort.println(params.brightnessWhite);
  }
  else if (strcmp(param, "ledorange") == 0) 
  {
    params.brightnessOrange = atol(value);
    for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i) {analogWrite(ledOrangeLeft[i], params.brightnessOrange);}
    for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i) {analogWrite(ledOrangeRight[i], params.brightnessOrange);}
    SerialPort.print("New orange LED brightness: ");
    SerialPort.println(params.brightnessOrange);
  }
  else if (strcmp(param, "ledon") == 0)
  {
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i)
    {
      analogWrite(ledRed[i], params.brightnessRed);
    }
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i)
    {
      analogWrite(ledWhite[i], params.brightnessWhite);
    }
    SerialPort.print("Red LED's turned on, brightness: ");
    SerialPort.println(params.brightnessRed);
    SerialPort.print("White LED's turned on, brightness: ");
    SerialPort.println(params.brightnessWhite);
  }
  else if (strcmp(param, "ledoff") == 0)
  {
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i)
    {
      analogWrite(ledRed[i], 0);
    }
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i)
    {
      analogWrite(ledWhite[i], 0);
    }
    SerialPort.print("Red LED's turned on, brightness: ");
    SerialPort.println(params.brightnessRed);
    SerialPort.print("White LED's turned on, brightness: ");
    SerialPort.println(params.brightnessWhite);
  }
  else if (strcmp(param, "blinklimit") == 0)
  {
    params.blinkLimit = atol(value);
    SerialPort.print("New blink limit: ");
    SerialPort.println(params.blinkLimit);
  }
  else if (strcmp(param, "ledstatus") == 0)
  {
    params.brightnessStatus = atol(value);
    SerialPort.print("New status LED brightness: ");
    SerialPort.println(params.brightnessStatus);
  }
  else
  {
    SerialPort.print("Unknown command: ");
    SerialPort.println(param);
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

  SerialPort.print("Received param: ");
  SerialPort.println(param);
  SerialPort.print("Received value: ");
  SerialPort.println(value);   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    float ratio = ((float) newCycleTime) / ((float) params.cycleTime);
    params.ki *= ratio;
    params.kd /= ratio;
    params.cycleTime = newCycleTime;
    telnetClient.print("New cycle time: ");
    telnetClient.println(params.cycleTime);
  }
  else if (strcmp(param, "ki") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.ki = atof(value) * cycleTimeInSec;
    telnetClient.print("New ki value: ");
    telnetClient.println(params.ki);
  }
  else if (strcmp(param, "kd") == 0) 
  {
    float cycleTimeInSec = ((float) params.cycleTime) / 1000000;
    params.kd = atof(value) / cycleTimeInSec;
    telnetClient.print("New kd value: ");
    telnetClient.println(params.kd);
  }
  else if (strcmp(param, "power") == 0) 
  {
    params.power = atol(value);
    telnetClient.print("New power value: ");
    telnetClient.println(params.power);
  }
  else if (strcmp(param, "diff") == 0) 
  {
    params.diff = atof(value);
    telnetClient.print("New diff value: ");
    telnetClient.println(params.diff);
  }
  else if (strcmp(param, "kp") == 0) 
  {
    params.kp = atof(value);
    telnetClient.print("New kp value: ");
    telnetClient.println(params.kp);
  }
  else if (strcmp(param, "ledred") == 0)
  {
    params.brightnessRed = atol(value);
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i) {analogWrite(ledRed[i], params.brightnessRed);}
    telnetClient.print("New red led brightness: ");
    telnetClient.println(params.brightnessRed);
  }
  else if (strcmp(param, "ledwhite") == 0)
  {
    params.brightnessWhite = atol(value);
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i) {analogWrite(ledWhite[i], params.brightnessWhite);}
    telnetClient.print("New red led brightness: ");
    telnetClient.println(params.brightnessWhite);
  }
  else if (strcmp(param, "ledorange") == 0) 
  {
    params.brightnessOrange = atol(value);
    for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i) {analogWrite(ledOrangeLeft[i], params.brightnessOrange);}
    for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i) {analogWrite(ledOrangeRight[i], params.brightnessOrange);}
    telnetClient.print("New orange LED brightness: ");
    telnetClient.println(params.brightnessOrange);
  }
  else if (strcmp(param, "ledon") == 0)
  {
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i)
    {
      analogWrite(ledRed[i], params.brightnessRed);
    }
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i)
    {
      analogWrite(ledWhite[i], params.brightnessWhite);
    }
    telnetClient.print("Red LED's turned on, brightness: ");
    telnetClient.println(params.brightnessRed);
    telnetClient.print("White LED's turned on, brightness: ");
    telnetClient.println(params.brightnessWhite);
  }
  else if (strcmp(param, "ledoff") == 0)
  {
    for (int i = 0; i < sizeof(ledRed) / sizeof(ledRed[0]); ++i)
    {
      analogWrite(ledRed[i], 0);
    }
    for (int i = 0; i < sizeof(ledWhite) / sizeof(ledWhite[0]); ++i)
    {
      analogWrite(ledWhite[i], 0);
    }
    telnetClient.print("Red LED's turned on, brightness: ");
    telnetClient.println(params.brightnessRed);
    telnetClient.print("White LED's turned on, brightness: ");
    telnetClient.println(params.brightnessWhite);
  }
  else if (strcmp(param, "blinklimit") == 0)
  {
    params.blinkLimit = atol(value);
    telnetClient.print("New blink limit: ");
    telnetClient.println(params.blinkLimit);
  }
  else if (strcmp(param, "ledstatus") == 0)
  {
    params.brightnessStatus = atol(value);
    SerialPort.print("New status LED brightness: ");
    SerialPort.println(params.brightnessStatus);
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
  SerialPort.print("Brightness red: ");
  SerialPort.println(params.brightnessRed);
  SerialPort.print("Brightness white: ");
  SerialPort.println(params.brightnessWhite);
  SerialPort.print("Brightness orange: ");
  SerialPort.println(params.brightnessOrange);
  SerialPort.print("Brightness status: ");
  SerialPort.println(params.brightnessStatus);
  SerialPort.print("Blink limit: ");
  SerialPort.println(params.blinkLimit);
  
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
  telnetClient.print("Brightness red: ");
  telnetClient.println(params.brightnessRed);
  telnetClient.print("Brightness white: ");
  telnetClient.println(params.brightnessWhite);
  telnetClient.print("Brightness orange: ");
  telnetClient.println(params.brightnessOrange);
  telnetClient.print("Brightness status: ");
  telnetClient.println(params.brightnessStatus);
  telnetClient.print("Blink limit: ");
  telnetClient.println(params.blinkLimit);
}

void onCalibrateStream()
{
  char* param = streamCommand.next();

  if (strcmp(param, "black") == 0)
  {
    SerialPort.println("Calibrating blacks... ");
    for (int i = 0; i < 6; i++)
    {
      params.blacks[i]=analogRead(sensors[i]);
      SerialPort.print("Sensor");
      SerialPort.print(i);
      SerialPort.print(" : ");
      SerialPort.println(params.blacks[i]);
    } 
    SerialPort.println("Done.");
    SerialPort.println();
  }
  else if (strcmp(param, "white") == 0)
  {
    SerialPort.println("Calibrating whites... ");
    for (int i = 0; i < 6; i++) 
    {
      params.whites[i]=analogRead(sensors[i]);
      SerialPort.print("Sensor");
      SerialPort.print(i);
      SerialPort.print(" : ");
      SerialPort.println(params.whites[i]);
    }  
    SerialPort.println("Done.");
    SerialPort.println();
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
    telnetClient.print("start calibrating black... ");
    telnetClient.print("start calibrating black... ");
    for (int i = 0; i < 6; i++) params.blacks[i]=analogRead(sensors[i]);
    telnetClient.println("done");
    telnetClient.println("done");
  }
  else if (strcmp(param, "white") == 0)
  {
    telnetClient.print("start calibrating white... ");
    telnetClient.print("start calibrating white... ");    
    for (int i = 0; i < 6; i++) params.whites[i]=analogRead(sensors[i]);  
    telnetClient.println("done");
    telnetClient.println("done");      
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onRunCommand()
{
  if (running == true)  
  {
    running = false;
    telnetClient.println("Robot set to standby");
  }
  else              
  {
    for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i) {analogWrite(ledOrangeLeft[i], 0);}
    for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i) {analogWrite(ledOrangeRight[i], 0);}
    analogWrite(ledStatusRed, 0);
    running = true;
    telnetClient.println("Robot set to run");
  }
}

void standby()
{
  analogWrite(motor1PWM1, 0);
  analogWrite(motor1PWM2, 0);
  analogWrite(motor2PWM1, 0);
  analogWrite(motor2PWM2, 0);

  analogWrite(ledStatusOrange, 0);
  analogWrite(ledStatusGreen, params.brightnessStatus);

  const int blinkInterval = 200;
  static unsigned long previousMillis = 0;
  static bool ledState = false;
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= blinkInterval)
  {
    previousMillis = currentMillis;
    ledState = !ledState;
    for (int i = 0; i < sizeof(ledOrangeLeft) / sizeof(ledOrangeLeft[0]); ++i)  {analogWrite(ledOrangeLeft[i], ledState ? params.brightnessOrange : 0);}
    for (int i = 0; i < sizeof(ledOrangeRight) / sizeof(ledOrangeRight[0]); ++i)  {analogWrite(ledOrangeRight[i], ledState ? params.brightnessOrange : 0);}
  }
}

void buttonInterrupt() 
{
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 250;
  unsigned long currentTime = millis();

  if (currentTime - lastDebounceTime >= debounceDelay)
  {
    analogWrite(ledStatusGreen, 0);
    analogWrite(ledStatusRed, 0);
    analogWrite(ledStatusOrange, params.brightnessStatus);
    running = !running;
    lastDebounceTime = currentTime;
  }
}