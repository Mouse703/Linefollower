#include <Arduino.h>
#include <WiFi.h>

#define SerialPort Serial
#define Baudrate 115200

bool debug;
unsigned long previous, calculationTime;
unsigned long cycleTime = 1000;

int sensors[] = {33, 32, 35, 34, 39, 36};

void setup()
{
  SerialPort.begin(Baudrate);
  SerialPort.println("ready");
  //analogReadResolution(12); // Set ADC resolution to 12 bits (default)

  // Set the ADC attenuation to extend the voltage range to 0-3.3V
  analogSetAttenuation(ADC_11db);
}

void loop()
{ 
  unsigned long current = micros();

  if (current - previous >= 1000)
  {
    previous = current;
    Serial.print("Sensor values: ");
    for (int i = 0; i < 6; ++i) 
    {
      SerialPort.print(analogRead(sensors[i]));
      SerialPort.print(" ");
    }
    SerialPort.println();
  }
}