#include <Arduino.h>
#include <WiFi.h>

const int motorLeftForward = 17;  //PWM pin for motor 1 driver input 1
const int motorLeftBackward = 16;  //PWM pin for motor 1 driver input 2
const int motorRightForward = 19;  //PWM pin for motor 2 driver input 1
const int motorRightBackward = 18;  //PWM pin for motor 2 driver input 2
const int delayTime = 5;   //Delay in milliseconds between each step

void motor1CW();          //MotorRightBackwards
void motor1CCW();         //MotorRightForwards
void motor2CW();          //MotorLeftBackwards
void motor2CCW();         //MotorLeft

void setup() 
{
  // Initialize Serial communication at a specific baud rate
  Serial.begin(115200);

  // Initialize PWM pins
  pinMode(motorLeftForward, OUTPUT);
  pinMode(motorLeftBackward, OUTPUT);
  pinMode(motorRightForward, OUTPUT);
  pinMode(motorRightBackward, OUTPUT);
}

void loop() 
{
  //Wait
  delay(2000);

  //Gradually increase motor speed
  Serial.println("Motor left forward");
  motor1CW();

  //Gradually decrease motor speed
  Serial.println("Motor right forward");
  motor2CW();

  //Gradually increase motor speed
  Serial.println("Motor left backward");
  motor1CCW();

  //Gradually decrease motor speed
  Serial.println("Motor right backward");
  motor2CCW();

  Serial.println("Restarting loop");
}

void motor1CW()
{
  for (int speed = 0; speed <= 255; speed++)
  {
    analogWrite(motorLeftForward, speed);
    analogWrite(motorLeftBackward, 0);
    delay(delayTime);
  }
  for (int speed = 255; speed >= 0; speed--) 
  {
    analogWrite(motorLeftForward, speed);
    analogWrite(motorLeftBackward, 0);
    delay(delayTime);
  }
}

void motor2CW()
{
  for (int speed = 0; speed <= 255; speed++)
  {
    analogWrite(motorRightForward, speed);
    analogWrite(motorRightBackward, 0);
    delay(delayTime);
  }
  for (int speed = 255; speed >= 0; speed--) 
  {
    analogWrite(motorRightForward, speed);
    analogWrite(motorRightBackward, 0);
    delay(delayTime);
  }
}

void motor1CCW()
{
  for (int speed = 0; speed <= 255; speed++)
  {
    analogWrite(motorLeftForward, 0);
    analogWrite(motorLeftBackward, speed);
    delay(delayTime);
  }
  for (int speed = 225; speed >= 0; speed--) 
  {
    analogWrite(motorLeftForward, 0);
    analogWrite(motorLeftBackward, speed);
    delay(delayTime);
  }
}

void motor2CCW()
{
  for (int speed = 0; speed <= 225; speed++)
  {
    analogWrite(motorRightForward, 0);
    analogWrite(motorRightBackward, speed);
    delay(delayTime);
  }
  for (int speed = 255; speed >= 0; speed--) 
  {
    analogWrite(motorRightForward, 0);
    analogWrite(motorRightBackward, speed);
    delay(delayTime);
  }
}