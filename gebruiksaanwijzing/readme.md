# Gebruiksaanwijzing

### Opladen / vervangen batterijen
Opladen van de batterijen gebeurd met de XTAR MC2 Plus lader. Plaats de batterijen in de houder met de correcte polariteit. De oplader kan door middel van een USB kabel verbonden worden met u PC. Wanneer de 4 blauwe streepjes constant blijven branden zijn de batterijen opgeladen. Je kan dit nakijken met een multimeter, de volledig opgeladen spanning is 4,18V.
![20231217_191814](https://github.com/Mouse703/Linefollower/assets/76005221/09fd5c1a-2c0e-4e07-a77b-8a387048093c)


### draadloze communicatie
#### Verbinding maken
Verbinding met de robot gebeurt met Telnet over WiFi. In het config.h bestand moet je de SSID en passwoord van uw netwerk invullen. Dit kan van een thuisnetwerk zijn of van een hotspot. Belangerijk is dat de band van het netwerk 2.4GHz ondersteunt. De ESP-32 van de robot werkt alleen maar op 2.4GHz

#### commando's
debug [on/off]  
start  
stop  
set cycle [µs]  
set power [0..255]  
set diff [0..1]  
set kp [0..]  
set ki [0..]  
set kd [0..]  
calibrate black  
calibrate white  

### kalibratie
uitleg kalibratie  

### settings
De robot rijdt stabiel met volgende parameters:  

### start/stop button
uitleg locatie + werking start/stop button
