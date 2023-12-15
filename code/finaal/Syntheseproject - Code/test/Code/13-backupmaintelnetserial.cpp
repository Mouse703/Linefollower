#include <Arduino.h>
#include <WiFi.h>
#include "TelnetCommand.h"
#include "TelnetCommand.cpp"
#include "StreamCommand.h"
#include "StreamCommand.cpp"

#define SerialPort Serial
#define Baudrate 115200
#define motor1PWM1 12
#define motor1PWM2 14 
#define motor2PWM1 26
#define motor2PWM2 27 

const char* ssid = "DESKTOP-MAINPC";
const char* pswd = "blub0123456789";

int port = 2121;
int sensors[] = {33, 32, 35, 34, 39, 36};
int normalised[6];
bool run;
float debugPosition;
unsigned long previous;
unsigned long calculationTime;
unsigned long cycleTime = 1000000;

void onDebug();
void unknownCommand(char *command);
void handleTelnet();

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
  telnetCommand.addCommand("set", onDebug);
  telnetCommand.addCommand("calibrate", onCalibrate);
  telnetCommand.setDefaultHandler(unknownCommand);

  streamCommand.addCommand("debugstream", onDebug);
  telnetCommand.addCommand("set", onDebug);
  telnetCommand.addCommand("calibrate", onCalibrate);
  streamCommand.setDefaultHandler(unknownCommand);
}

void loop() 
{
  handleTelnet();
  streamCommand.readStream();
  unsigned long current = micros();

  if (current - previous >= cycleTime) 
  {  
    previous = current;
    Serial.println("One Cycle");
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

void onDebug()
{
  SerialPort.println("Cycle time: " + String(cycleTime));
  SerialPort.println("Calculation time: " + String(calculationTime));
  telnetClient.println("Cycle time: " + String(cycleTime));
  telnetClient.println("Calculation time: " + String(calculationTime));
}