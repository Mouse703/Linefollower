#include <Arduino.h>
#include <WiFi.h>
#include "TelnetCommand.h"
#include "TelnetCommand.cpp"

#define SerialPort Serial
#define Baudrate 115200

const char* ssid = "HGDevice";
const char* pswd = "KillAllHuman$";

int port = 2121;
int motor1Speed = 0;
int motor2Speed = 0;

const int motor1PWM1 = 16;
const int motor1PWM2 = 17; 
const int motor2PWM1 = 18;
const int motor2PWM2 = 19; 
const int delayTime = 25;

void processMotor1Command();
void processMotor2Command();
void processSyncCommand();
void unknownCommand(char *command);

WiFiServer telnetServer(port);
WiFiClient client;
TelnetCommand myStreamCommand(Serial);

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

  myStreamCommand.setDefaultHandler(unknownCommand);
  myStreamCommand.addCommand("M1", processMotor1Command);
  myStreamCommand.addCommand("M2", processMotor2Command);
  myStreamCommand.addCommand("SYNC", processSyncCommand);
}

void loop() 
{
  client = telnetServer.available();
  myStreamCommand.readStream(client);
  if (client) 
  {
    Serial.println("New Telnet client connected");
    client.println("Welcome to ESP32-Robot Telnet Server");
    while (client.connected()) {if (client.available()) {myStreamCommand.readStream(client);}}
    Serial.println("Telnet client disconnected");
  }
}

void unknownCommand(char *command)
{
  Serial.print("Unknown command: ");
  Serial.println(command);
  client.print("Unknown command: ");
  client.println(command);
}

void processMotor1Command()
{
  int speed = atoi(myStreamCommand.next());
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
  int speed = atoi(myStreamCommand.next());
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
  int speed = atoi(myStreamCommand.next());
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