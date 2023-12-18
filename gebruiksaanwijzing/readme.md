# Gebruiksaanwijzing

### Opladen / vervangen batterijen
Opladen van de batterijen gebeurd met de XTAR MC2 Plus lader. Plaats de batterijen in de houder met de correcte polariteit. De oplader kan door middel van een USB kabel verbonden worden met u PC. Wanneer de 4 blauwe streepjes constant blijven branden zijn de batterijen opgeladen. Je kan dit nakijken met een multimeter, de volledig opgeladen spanning is 4,18V.
![20231217_191814](https://github.com/Mouse703/Linefollower/assets/76005221/09fd5c1a-2c0e-4e07-a77b-8a387048093c)
![20231210_170905](https://github.com/Mouse703/Linefollower/assets/76005221/feda0d32-787f-4392-ab9b-a30f92c13fbb)


### Draadloze communicatie
#### Verbinding maken
Verbinding met de robot gebeurt met Telnet over WiFi. In het config.h bestand moet je de SSID en passwoord van uw netwerk invullen. Dit kan van een thuisnetwerk zijn of van een hotspot. Belangerijk is dat de band van het netwerk 2.4GHz ondersteunt. De ESP-32 van de robot werkt alleen maar op 2.4GHz
Indien je met de robot met je PC wil verbinden hoef je geen extra software op je PC te installeren. Wel moet je een windows feature aanzetten, dit doe je door naar "Turn Windows features on or off" te gaan. Hier scroll je naar beneden tot je Telnet Client ziet staan, die vink je dan aan zodat de nodige features geinstalleerd worden. Eenmaal dit gedaan is kan je een command prompt openen, hier geef je het IP address in van de robot. Het IP address kan je vinden door naar je hotspot instellingen te gaan. Of je kan de robot verbinden met USB en een serieële monitor openen, daar krijg je ook te zien wat het IP address is van de robot.

#### commando's
##### debug
##### run (start/stop)  
##### set cycle [µs]  
##### set power [0..255]  Instellen power van de motoren
##### set diff [0..1]     Instellen als de robot moet vertragen in de bochten (0 niets, 1 heel veel)
##### set kp [0..]        Instellen proportionele correctie van de fout (P-regeling)
##### set ki [0..]        Instellen integrerende correctie van de fout (I-regeling)
##### set kd [0..]        Instellen differentieële correctie van de fout (D-regeling)
##### set ledon  Aanzetten van voor- en achterlichten
##### set ledred [0..255]  (instellen van brightness rode lichten)
##### set ledorange [0..255]  (instellen van brightness oranje lichten)
##### set ledwhite [0..255]  (instellen van brightness witte lichten)
##### set ledstatus [0..255]  (instellen van brightness status lichten)
##### calibrate black  
##### calibrate white  

### kalibratie
uitleg kalibratie  

### settings
De robot rijdt stabiel met volgende parameters:  

### start/stop button
uitleg locatie + werking start/stop button
