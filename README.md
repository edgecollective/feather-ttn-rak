# Setup 

## Raspberry Pi

- [RAK 831 Developer Kit or Equivalent](https://store.rakwireless.com/products/rak831-gateway-module?variant=22375114801252)

- [Chirpstack OS](https://www.chirpstack.io/gateway-os/overview/)


## Arduino IDE

### Board Definitions

- Arduino SAMD
- Adafruit SAMD

### Arduino Libraries

- [Fork of RTCZero that fixes 'lockup' issue](https://github.com/sslupsky/RTCZero/tree/fix-lockup-when-wake-from-sleep).  Note that 'main' repo is [here](https://github.com/arduino-libraries/RTCZero), but they haven't merged the relevant fork / pull request.
- [Latest verison of CayenneLPP](https://github.com/ElectronicCats/CayenneLPP)
- [Latest version of ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [Version 2.3.2 of arduino-lmic](https://github.com/mcci-catena/arduino-lmic/releases/tag/v2.3.2)

Optional:
- [SDI-12 library](https://github.com/EnviroDIY/Arduino-SDI-12)

## Adafruit Feather M0 LoRA Hardware

Pin 6 connected to Pin i01

