# Environmental Datalogger

## Serial Port Note
This sketch is designed for TWO serials ports, preferred in this order:
1. `Serial` (USB)
2. `Serial1` (UART)

If `Serial` (USB) isn't avaialable (not plugged in), the console output will switch to `Serial1` (UART).

Please see configuration section in the code.

## Hardware:
- [Adafruit Feather RP2040](https://www.adafruit.com/product/4884)
- [Adalogger FeatherWing - RTC + SD](https://www.adafruit.com/product/2922).
- [BME688 sensor](https://www.adafruit.com/product/5046) connected via I2C

## Third-party Libraries required:
- [RTClib by Adafruit](https://github.com/adafruit/RTClib)
- [Adafruit_BME680](https://github.com/adafruit/Adafruit_BME680)