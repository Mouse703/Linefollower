#include <Arduino.h>

const int motor1PWM1 = 12;  //PWM pin for motor 1 driver input 1
const int motor1PWM2 = 14;  //PWM pin for motor 1 driver input 2
const int motor2PWM1 = 26;  //PWM pin for motor 2 driver input 1
const int motor2PWM2 = 27;  //PWM pin for motor 2 driver input 2
const int delayTime = 25;   //Delay in milliseconds between each step

void motor1CW();
void motor1CCW();
void motor2CW();
void motor2CCW();

void setup() 
{
  // Initialize Serial communication at a specific baud rate
  Serial.begin(115200);

  // Initialize PWM pins
  pinMode(motor1PWM1, OUTPUT);
  pinMode(motor1PWM2, OUTPUT);
  pinMode(motor2PWM1, OUTPUT);
  pinMode(motor2PWM2, OUTPUT);
}

void loop() 
{
  //Wait
  delay(2000);

  //Gradually increase motor speed
  Serial.println("Motor 1 clockwise");
  motor1CW();

  //Gradually decrease motor speed
  Serial.println("Motor 2 clockwise");
  motor2CW();

  //Gradually increase motor speed
  Serial.println("Motor 1 counter clockwise");
  motor1CCW();

  //Gradually decrease motor speed
  Serial.println("Motor 2 counter clockwise");
  motor2CCW();

  Serial.println("Restarting loop");
}

void motor1CW()
{
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, speed);
    delay(delayTime);
  }
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor1PWM1, 0);
    analogWrite(motor1PWM2, speed);
    delay(delayTime);
  }
}

void motor2CW()
{
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, speed);
    delay(delayTime);
  }
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor2PWM1, 0);
    analogWrite(motor2PWM2, speed);
    delay(delayTime);
  }
}

void motor1CCW()
{
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor1PWM1, speed);
    analogWrite(motor1PWM2, 0);
    delay(delayTime);
  }
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor1PWM1, speed);
    analogWrite(motor1PWM2, 0);
    delay(delayTime);
  }
}

void motor2CCW()
{
  for (int speed = 40; speed <= 125; speed++)
  {
    analogWrite(motor2PWM1, speed);
    analogWrite(motor2PWM2, 0);
    delay(delayTime);
  }
  for (int speed = 125; speed >= 30; speed--) 
  {
    analogWrite(motor2PWM1, speed);
    analogWrite(motor2PWM2, 0);
    delay(delayTime);
  }
}