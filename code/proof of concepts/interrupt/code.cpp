const int buttonPin = 2;
volatile bool isRunning = false;
volatile bool buttonPressed = false;

void setup() 
{
  pinMode(buttonPin, INPUT_PULLUP);
  Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonInterrupt, CHANGE);
}

void loop() 
{
  if (isRunning) 
  {
    //Code
  }
}

void buttonInterrupt() 
{
  if (!buttonPressed) 
  {
    delay(50); //Debounce
    buttonPressed = true;
    
    if (digitalRead(buttonPin) == LOW) 
    {
      isRunning = !isRunning;
      if (isRunning) {Serial.println("System started.");}
      else {Serial.println("System stopped.");}
  	}
  } else {buttonPressed = false;}
}
