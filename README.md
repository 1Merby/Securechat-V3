# NOTE 1 = This project was written in Turkish, code notes are English so that you can change it to English by yourself if you want!
# NOTE 2 = If you want to reset login settings, you need to run eeprom_clear example from your coding app.

# SecureChat V3

A local-network encrypted messaging app running on ESP8266.

## Features
- Encrypted messaging
- Encrypted web interface
- EEPROM-based storage (chat is wiped on reset)
- WhatsApp-like UI

## Requirements
- ESP8266 board (must be installed in Arduino IDE)
- No additional libraries needed, all included in ESP8266 core

## Setup
1. Install ESP8266 board in Arduino IDE
2. Flash the `.ino` file
3. Do the setup from Serial Monitor
4. Connect to the same network, open the device IP

## Warning
This project is intended for local network use only. Not designed for production environments.

## Changelog
- **v1.0** — Basic messaging, no encryption, weak UI
- **v2.0** — Improved security and interface. No encryption.
- **v2.3** — Showing IP of web-side users in serial monitor whenever they text.
- **v3.0** — Added EEPROM encryption.
