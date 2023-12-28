#include <Arduino.h>
#include <SerialCommand.h>

const int motor1PWM1 = 12;  //PWM pin for motor 1 driver input 1
const int motor1PWM2 = 14;  //PWM pin for motor 1 driver input 2
const int motor2PWM1 = 26;  //PWM pin for motor 2 driver input 1
const int motor2PWM2 = 27;  //PWM pin for motor 2 driver input 2
const int delayTime = 25;   //Delay in milliseconds between each step

bool debug;
unsigned long previous;

SerialCommand sCmd; // SerialCommand object declaration

void motor1CW();
void motor1CCW();
void motor2CW();
void motor2CCW();
void commandUnknown(const char *command);

void setup() 
{
  // Initialize Serial communication at a specific baud rate
  Serial.begin(115200);

  sCmd.addCommand("M1CW", motor1CW);
  sCmd.addCommand("M1CCW", motor1CCW);
  sCmd.addCommand("M2CW", motor2CW);
  sCmd.addCommand("M2CCW", motor2CCW);
  sCmd.setDefaultHandler(commandUnknown);

  // Initialize PWM pins
  pinMode(motor1PWM1, OUTPUT);
  pinMode(motor1PWM2, OUTPUT);
  pinMode(motor2PWM1, OUTPUT);
  pinMode(motor2PWM2, OUTPUT);
}

void loop() 
{
  sCmd.readSerial(); // Listen and process serial commands
  unsigned long current = micros();
  if (current - previous >= 1000000)
  {
    previous = current;
    if (debug) Serial.println("loop still running");
  }
}

void commandUnknown(const char *command)
{
  Serial.println("Unknown command");
}

void motor1CW()
{
  Serial.println("Serial command received");
  Serial.println("Turning motor 1 clockwise");
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, speed);
    delay(delayTime);
  }
  Serial.println("Stopping motor 1 clockwise");
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, speed);
    delay(delayTime);
  }
  Serial.println("Done");
}

void motor2CW()
{
  Serial.println("Serial command received");
  Serial.println("Turning motor 2 clockwise");
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, speed);
    delay(delayTime);
  }
  Serial.println("Stopping motor 2 clockwise");
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, speed);
    delay(delayTime);
  }
  Serial.println("Done");
}

void motor1CCW()
{
  Serial.println("Serial command received");
  Serial.println("Turning motor 1 counter clockwise");
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor1PWM1, speed);
    analogWrite(motor1PWM2, 0);
    delay(delayTime);
  }
  Serial.println("Stopping motor 1 counter clockwise");
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor1PWM1, speed);
    analogWrite(motor1PWM2, 0);
    delay(delayTime);
  }
  Serial.println("Done");
}

void motor2CCW()
{
  Serial.println("Serial command received");
  Serial.println("Turning motor 2 counter clockwise");
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor2PWM1, speed);
    analogWrite(motor2PWM2, 0);
    delay(delayTime);
  }
  Serial.println("Stopping motor 2 counter clockwise");
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor2PWM1, speed);
    analogWrite(motor2PWM2, 0);
    delay(delayTime);
  }
  Serial.println("Done");
}