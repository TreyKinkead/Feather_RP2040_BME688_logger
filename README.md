# Environmental Datalogger

## Serial Port Note
This sketch is designed for TWO serials ports, preferred in this order:
1. `Serial` (USB)
2. `Serial1` (UART)

If `Serial` (USB) isn't available (not plugged in), the console output will switch to `Serial1` (UART).

Please see configuration section in the code.

Console (serial) out lines should be prefixed with `LOG,` or `DATA,` - for example:
```
LOG,initializing RTC
LOG,RTC initialized
LOG,initializing BME
LOG,BME initialized
LOG,initializing SD card
LOG,SD initialized
LOG,complete
DATA,2022-06-29 15:32:02,26.87,756.37,20.57,73.54
DATA,2022-06-29 15:32:04,26.63,756.39,20.31,72.17
DATA,2022-06-29 15:32:05,26.42,756.39,20.23,71.27
DATA,2022-06-29 15:32:07,26.26,756.39,20.20,71.45
DATA,2022-06-29 15:32:08,26.14,756.40,20.18,72.73
```

## Data Files on Micro SD Card
A data file per day will be written to the micro SD card. Files will be named in the form _YYYYMMDD_.csv (e.g., `20220629.csv`).

## Hardware:
- [Adafruit Feather RP2040](https://www.adafruit.com/product/4884)
- [Adalogger FeatherWing - RTC + SD](https://www.adafruit.com/product/2922).
- [BME688 sensor](https://www.adafruit.com/product/5046) connected via I2C

## Third-party Libraries required:
- [RTClib by Adafruit](https://github.com/adafruit/RTClib)
- [Adafruit_BME680](https://github.com/adafruit/Adafruit_BME680)
