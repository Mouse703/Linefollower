#include <Arduino.h>
#include <WiFi.h>
#include <EEPROM.h>
#include "TelnetCommand.h"
#include "TelnetCommand.cpp"
#include "StreamCommand.h"
#include "StreamCommand.cpp"


#define SerialPort Serial
#define Baudrate 115200

const char* ssid = "DESKTOP-JHQ2KA4";
const char* pswd = "blub0123456789";

int port = 2121;
int motor1Speed = 0;
int motor2Speed = 0;
int sensors[] = {33, 32, 35, 34, 39, 36};
bool debug;
unsigned long previous, calculationTime;

const int motor1PWM1 = 19;
const int motor1PWM2 = 18; 
const int motor2PWM1 = 17;
const int motor2PWM2 = 16; 
const int delayTime = 25;

void onSet();
void onDebug();
void onCalibrate();
void processMotor1Command();
void processMotor2Command();
void processSyncCommand();
void onUnknownCommand(char *command);

WiFiServer telnetServer(port);
WiFiClient client;
TelnetCommand telnetCmd(SerialPort);
StreamCommand serialCmd(SerialPort);

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

  telnetCmd.setDefaultHandler(onUnknownCommand);
  telnetCmd.addCommand("M1", processMotor1Command);
  telnetCmd.addCommand("M2", processMotor2Command);
  telnetCmd.addCommand("SYNC", processSyncCommand);

  serialCmd.addCommand("set", onSet);
  serialCmd.addCommand("debug", onDebug);
  serialCmd.addCommand("calibrate", onCalibrate);
  serialCmd.setDefaultHandler(onUnknownCommand);
  
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get(0, params);
  EEPROM.end();
}

void loop() 
{
  client = telnetServer.available();
  telnetCmd.readStream(client);
  if (client) 
  {
    Serial.println("New Telnet client connected");
    client.println("Welcome to ESP32-Robot Telnet Server");
    while (client.connected()) {if (client.available()) {telnetCmd.readStream(client);}}
    Serial.println("Telnet client disconnected");
  }
}

void onUnknownCommand(char *command)
{
  Serial.print("Unknown command: ");
  Serial.println(command);
  client.print("Unknown command: ");
  client.println(command);
}

void processMotor1Command()
{
  int speed = atoi(telnetCmd.next());
  motor1Speed = speed;
  if (motor1Speed >= 0) 
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, motor1Speed);
    Serial.println("Motor 1: Clockwise | PWM Value: " + String(motor1Speed));
    client.println("\nMotor 1: Clockwise | PWM Value: " + String(motor1Speed));
  } 
  else 
  {
    analogWrite(motor1PWM1, abs(motor1Speed));
    analogWrite(motor1PWM2, 0);
    Serial.println("Motor 1: Counter Clockwise | PWM Value: " + String(motor1Speed));
    client.println("\nMotor 1: Counter Clockwise | PWM Value: " + String(motor1Speed));
  }
}

void processMotor2Command()
{
  int speed = atoi(telnetCmd.next());
  motor2Speed = speed;
  if (motor2Speed >= 0) 
  {
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, motor2Speed);
    Serial.print("Motor 2 clockwise @PWMValue: ");
    client.print("\nMotor 2 clockwise @PWMValue: ");
  } 
  else 
  {
    analogWrite(motor2PWM1, abs(motor2Speed));
    analogWrite(motor2PWM2, 0);
    Serial.print("Motor 2 counter clockwise @PWMValue: ");
    client.print("\nMotor 2 counter clockwise @PWMValue: ");
  }
  Serial.println(motor2Speed);
  client.println(motor2Speed);
}

void processSyncCommand()
{
  int speed = atoi(telnetCmd.next());
  if (motor1Speed >= 0) 
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, speed);
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, speed);
    Serial.print("Motors clockwise @PWMValue: ");
    client.print("\nMotors clockwise @PWMValue: ");
  } 
  else 
  {
    analogWrite(motor1PWM1, abs(speed));
    analogWrite(motor1PWM2, 0);
    analogWrite(motor2PWM1, abs(speed));
    analogWrite(motor2PWM2, 0);
    Serial.print("Motors counter clockwise @PWMValue: ");
    client.print("\nMotors counter clockwise @PWMValue: ");
  }
  Serial.println(speed);
  client.println(speed);
}