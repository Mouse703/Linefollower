#include <Arduino.h>
#include <WiFi.h>

#define SerialPort Serial
#define Baudrate 115200

const char* ssid = "DESKTOP-MAINPC";
const char* pswd = "blub0123456789";

int port = 2121;

WiFiServer telnetServer(port);

void setup() 
{
  //Openen v.d. serialport
  SerialPort.begin(Baudrate);
  delay(100);

  //Proberen met Wi-Fi te verbinden
  WiFi.begin(ssid, pswd);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(1000);
    SerialPort.println("Connecting to WiFi...");
  }

  //Indien verbonden IP printen in de serieële console
  SerialPort.println("Connected to WiFi");
  SerialPort.print("IP address: ");
  SerialPort.println(WiFi.localIP());

  //Starten van Telnet server
  telnetServer.begin();
  SerialPort.print("Telnet server started on port ");
  SerialPort.println(port);
}

void loop() 
{
  WiFiClient client = telnetServer.available();

  if (client) 
  {
    SerialPort.println("New Telnet client connected");
    client.println("Welcome to ESP32-Robot Telnet Server");

    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        SerialPort.print(c);
        client.println("Hallo");
      }
    }

    SerialPort.println("Telnet client disconnected");
  }
}