# Gebruiksaanwijzing

## Opladen / Vervangen Batterijen
Het opladen van de batterijen gebeurt met de XTAR MC2 Plus lader. Plaats de batterijen in de houder met de juiste polariteit. Verbind de lader met je PC via een USB-kabel. Wanneer de 4 blauwe streepjes constant branden, zijn de batterijen opgeladen. Je kunt dit ook controleren met een multimeter; de volledig opgeladen spanning is 4,18V.

## Draadloze Communicatie
### Verbinding maken
Verbinding met de robot gebeurt via Telnet over WiFi. In het `config.h` bestand dien je de SSID en het wachtwoord van je netwerk in te vullen, alsook kan je een hostname instellen voor je robot. Het WiFi netwerk kan zowel een thuisnetwerk als een hotspot zijn, maar het netwerk moet de 2.4GHz band ondersteunen. Dit komt omdat de ESP-32 van de robot alleen op 2.4GHz werkt. Ook moet je ervoor zorgen dat je op hetzelfde WiFi netwerk zit. Wanneer dit is ingevuld kan je de code uploaden naar de ESP-32 (zie handleiding).

Als je de robot met je PC wilt verbinden, hoef je geen extra software te installeren. Wel moet je een Windows-functie inschakelen door naar "Turn Windows features on or off" te gaan. Hier vind je de Telnet Client, die je moet aanvinken zodat de benodigde functies worden geïnstalleerd. 

Het ip address kan je vinden in je hotspot-instellingen of door een USB-verbinding met de robot te maken en de seriële monitor te openen. Deze toont dan het IP adress / telnet poort waarmee de robot kan verbonden worden. 

### 1: Powershell terminal (PC)
1. Zet de hotspot aan of verbind met hetzelfde WiFi-netwerk waar de ESP-32 zich op bevindt.
2. Open een powershell terminal met de toetsencombinatie `WIN` + `X` en selecteer Terminal.
3. Voer het commando telnet in met uw IP of hostname gevolgd door de telnet poort (bvb `telnet 192.168.137.239 2121`)
4. Nu wordt er automatisch een nieuwe window geopend en kan u commando's ingeven.

### 2: Mobile Telnet (smartphone app)
1. Zet de hotspot aan of verbind met hetzelfde WiFi-netwerk waar de ESP-32 zich op bevindt.
2. Open de app en klik rechtsboven op de 3 puntjes
3. Klik op telnet settings en voer het juiste IP address in met de bijhorende poort
4. Klik op connect en nu kan u commando's ingeven.

### Commando's
- **debug** Toont alle instelbare parameters
- **run (start/stop)** Start of stopt de robot
- **set cycle [µs]** Stelt de cyclustijd van de ESP-32 in
- **set power [0..255]** Stelt de kracht van de motoren in
- **set diff [0..1]** Stelt in of de robot moet vertragen in de bochten (0 niets, 1 veel)
- **set kp [0..]** Stelt de proportionele correctie van de fout in (P-regeling)
- **set ki [0..]** Stelt de integrerende correctie van de fout in (I-regeling)
- **set kd [0..]** Stelt de differentiële correctie van de fout in (D-regeling)
- **set ledon** Zet de voor- en achterlichten aan
- **set ledred [0..255]** Stelt de helderheid van de rode lichten in
- **set ledorange [0..255]** Stelt de helderheid van de oranje lichten in
- **set ledwhite [0..255]** Stelt de helderheid van de witte lichten in
- **set ledstatus [0..255]** Stelt de helderheid van de statuslichten in
- **calibrate black** Kalibreert de zwartwaarden
- **calibrate white** Kalibreert de witwaarden

## Kalibratie
Kalibreer de zwartwaarden door de robot op een zwart oppervlak van het circuit te plaatsen en voer vervolgens het commando `calibrate black` in. Herhaal dit proces voor de witwaarden door de robot op een wit oppervlak van het circuit te plaatsen en het commando `calibrate white` in te voeren.

## Instellingen
De robot rijdt stabiel met de volgende parameters:
- **Cycle time**: 2500
- **Power**: 100
- **Diff**: 0.05
- **Kp**: 5
- **Ki**: 0
- **Kd**: 0

## Start/Stop-knop
De start/stop-knop bevindt zich naast de blauwe status-LED. Druk op deze knop om de running state van de robot te wijzigen, wat betekent dat de robot zal beginnen met rijden of stoppen, afhankelijk van de huidige toestand.
