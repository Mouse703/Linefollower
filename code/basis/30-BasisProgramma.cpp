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
#define telnetPort 2121

const int ledOrange[] = {14, 21, 23, 26};
const int buttonToggleState = 2;

unsigned long previous;
unsigned long calculationTime;

void buttonInterrupt();
void handleTelnet();
void unknownCommand(char *command);
void onRunCommand();
void onDebug();
void onSetTelnet();
void onSetStream();

WiFiServer telnetServer(telnetPort);
WiFiClient telnetClient;
TelnetCommand telnetCommand(SerialPort);
StreamCommand streamCommand(SerialPort);

struct param_t
{
  unsigned long cycleTime;
  int brightnessOrange;
} params;

#define EEPROM_SIZE sizeof(param_t)

volatile bool running = false;

void setup()
{
  pinMode(buttonToggleState, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonToggleState), buttonInterrupt, FALLING);
  for (int i = 0; i < sizeof(ledOrange) / sizeof(ledOrange[0]); ++i) {pinMode(ledOrange[i], OUTPUT);}
  
  //Setting up communication
  SerialPort.begin(Baudrate);
  WiFi.setHostname(host);					 
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    SerialPort.println("Connecting to WiFi...");
  }
  SerialPort.println("Connected to WiFi");
  SerialPort.print("IP address: ");
  SerialPort.println(WiFi.localIP());

  //Setting up telnetserver
  telnetServer.begin();
  SerialPort.print("Telnet server started on port ");
  SerialPort.println(telnetPort);

  // etting up telnet commands
  telnetCommand.addCommand("run", onRunCommand);
  telnetCommand.addCommand("debug", onDebug);
  telnetCommand.addCommand("set", onSetTelnet);
  telnetCommand.setDefaultHandler(unknownCommand);

  //Setting up stream commands (serial)
  streamCommand.addCommand("run", onRunCommand);
  streamCommand.addCommand("debug", onDebug);
  streamCommand.addCommand("set", onSetStream);
  streamCommand.setDefaultHandler(unknownCommand);

  //Reading EEPROM memory and applying parameters
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();
}

void loop()
{
  handleTelnet();
  streamCommand.readStream();
  unsigned long current = micros();

  if (current - previous >= params.cycleTime) 
  {  
    previous = current;
    if (running)
    {
      //Blink orange LED's
      const int blinkInterval = 200;
      static unsigned long previousMillis = 0;
      static bool ledState = false;
      unsigned long currentMillis = millis();

      if (currentMillis - previousMillis >= blinkInterval)
      {   
        previousMillis = currentMillis;
        ledState = !ledState;
        for (int i = 0; i < sizeof(ledOrange) / sizeof(ledOrange[0]); ++i)
        {
        analogWrite(ledOrange[i], ledState ? params.brightnessOrange : 0);
        }
      }
    }
    else 
    {
      for (int i = 0; i < sizeof(ledOrange) / sizeof(ledOrange[0]); ++i)
      {
        analogWrite(ledOrange[i], 0);
      }
    }
  }
  else {}

  unsigned long difference = micros() - current;
  if (difference > calculationTime) calculationTime = difference;
}

void buttonInterrupt() 
{
  static unsigned long lastDebounceTime = 0;
  const unsigned long debounceDelay = 250;
  unsigned long currentTime = millis();

  if (currentTime - lastDebounceTime >= debounceDelay)
  {
    running = !running;
    lastDebounceTime = currentTime;
  }
}

void handleTelnet()
{
  if (telnetServer.hasClient())
  {
    if (!telnetClient || !telnetClient.connected())
    {
      if (telnetClient)
      {
        telnetClient.stop();
      }
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
  //SERIAL
  SerialPort.println("Cycle time: " + String(params.cycleTime));
  SerialPort.println("Calculation time: " + String(calculationTime));
  SerialPort.print("Brightness orange: ");
  SerialPort.println(params.brightnessOrange);

  //TELNET
  telnetClient.println("Cycle time: " + String(params.cycleTime));
  telnetClient.println("Calculation time: " + String(calculationTime));
  telnetClient.print("Brightness orange: ");
  telnetClient.println(params.brightnessOrange);
}

void onSetTelnet()
{
  char *param = telnetCommand.next();
  char *value = telnetCommand.next();

  SerialPort.print("Received param: ");
  SerialPort.println(param);
  SerialPort.print("Received value: ");
  SerialPort.println(value);   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    params.cycleTime = newCycleTime;
    telnetClient.print("New cycle time: ");
    telnetClient.println(params.cycleTime);
  }
  else if (strcmp(param, "ledorange") == 0) 
  {
    params.brightnessOrange = atol(value);
    for (int i = 0; i < sizeof(ledOrange) / sizeof(ledOrange[0]); ++i) {analogWrite(ledOrange[i], params.brightnessOrange);}
    telnetClient.print("New orange LED brightness: ");
    telnetClient.println(params.brightnessOrange);
  }
  else
  {
    telnetClient.print("Unknown command: ");
    telnetClient.println(param);
  }

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put(0, params);
  EEPROM.commit();
  EEPROM.end();
}

void onSetStream()
{
  char *param = streamCommand.next();
  char *value = streamCommand.next();

  SerialPort.print("Received param: ");
  SerialPort.println(param);
  SerialPort.print("Received value: ");
  SerialPort.println(value);   
  
  if (strcmp(param, "cycle") == 0) 
  {
    long newCycleTime = atol(value);
    params.cycleTime = newCycleTime;
    SerialPort.print("New cycle time: ");
    SerialPort.println(params.cycleTime);
  }
  else if (strcmp(param, "ledorange") == 0) 
  {
    params.brightnessOrange = atol(value);
    for (int i = 0; i < sizeof(ledOrange) / sizeof(ledOrange[0]); ++i) {analogWrite(ledOrange[i], params.brightnessOrange);}
    SerialPort.print("New orange LED brightness: ");
    SerialPort.println(params.brightnessOrange);
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

void onRunCommand()
{
  running = !running;
  if (running)
  {
    SerialPort.println("Running");
    telnetClient.println("Running");
  }
  else
  {
    SerialPort.println("Standby");
    telnetClient.println("Standby");
  }
}