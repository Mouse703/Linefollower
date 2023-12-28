#include <Arduino.h>

unsigned long previous;

void setup()
{

}

void loop()
{
  /*
   Code sensoren
  */
  unsigned long current = micros();
  if (current - previous >= 1000000)
  {
    /*
    strip
    strip.show
    */
    previous = current;

  }

  unsigned long current = micros();
  if (current - previous >= 1000000)
  {
    /*
    strip
    strip.show
    */
    previous = current;

  }
}