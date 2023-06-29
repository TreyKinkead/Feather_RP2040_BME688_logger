/*
  Datalogger using:
  - Adafruit RP2040 Feather
  - Adafruit Adalogger 
  - BME688
*/
#include <SPI.h>
#include <SD.h>
#include <RTClib.h>
#include <Adafruit_BME680.h>

// pin numbers for Adafruit RP2040 Feather + Adaloger FeatherWing
// needed for SD card access
const uint8_t SD_MISO = 20;
const uint8_t SD_MOSI = 19;
const uint8_t SD_CS = 10;
const uint8_t SD_SCK = 18;


// NOTE: This sketch is designed for TWO serials ports, preferred
// in this order:
//    a) Serial (USB)
//    b) Serial1 (UART)
// If Serial (USB) isn't avaialable (not plugged in), the
// console output will switch to Serial1 (UART).

// configure pins for Seria1 (UART)
const auto Serial1_TX = 0;
const auto Serial1_RX = 1;

// configure serial baud rate
const auto baud = 9600;  // value is ignored for USB serial

// other configuration
const auto data_delimiter = ',';
const auto log_prefix = "LOG,";
const auto data_prefix = "DATA,";
const auto main_loop_delay = 1000;
const auto fail_loop_delay = 5000;

Stream* pSerialConsole; // assigned in setup()
RTC_PCF8523 rtc;        // RTC on I2C
Adafruit_BME680 bme;    // sensor on I2C

// date and time callback for SD file
void dtCallback(uint16_t* date, uint16_t* time) {
  DateTime now = rtc.now();
  *date = FAT_DATE(now.year(), now.month(), now.day());
  *time = FAT_TIME(now.hour(), now.minute(), now.second());
}

// blink the built-in LED n times
void blink_led(const uint8_t n) {
  for (auto i = 0; i < n; i++) {
    gpio_put(LED_BUILTIN, true);
    delay(200);
    gpio_put(LED_BUILTIN, false);
    delay(200);
  }
}

void setup() {

  gpio_init(LED_BUILTIN);
  gpio_set_dir(LED_BUILTIN, GPIO_OUT);

  // Serial1 - hardware UART (not USB)
  Serial1.setRX(Serial1_RX);
  Serial1.setTX(Serial1_TX);
  Serial1.begin(baud);
  delay(500);
  Serial1.print(log_prefix);
  Serial1.println("Serial1 configured");

  Serial.begin(baud);
  delay(1000);
  for (auto i=0;!Serial && i<5;i++) {
    // wait for serial port to connect
    Serial1.print(log_prefix);
    Serial1.print("Serial (USB) not ready ");
    Serial1.println(i);
    blink_led(1);
    delay(200);
  }
  if (Serial) {
    // console on Serial (USB)
    Serial1.print(log_prefix);
    Serial1.println("Serial (USB) ready - using it for console");
    pSerialConsole = &Serial;
  } else {
    // switch to console to Serial1
    Serial1.print(log_prefix);
    Serial1.print("switching console to Serial1...");
    pSerialConsole = &Serial1;
    pSerialConsole->println(" done");
  }

  /*--- RTC ---*/
  pSerialConsole->print(log_prefix);
  pSerialConsole->println("initializing RTC");

  if (!rtc.begin()) {
    while (true) {
      pSerialConsole->print(log_prefix);
      pSerialConsole->println("Couldn't find RTC");
      pSerialConsole->flush();
      blink_led(2);
      delay(fail_loop_delay);
    }
  }
  // ensure RTC is running
  rtc.start();
  delay(2000);
  while (!rtc.isrunning()) {
    pSerialConsole->print(log_prefix);
    pSerialConsole->println("RTC is NOT running");
    pSerialConsole->flush();
    blink_led(3);
    delay(fail_loop_delay);
  }
  pSerialConsole->print(log_prefix);
  pSerialConsole->println("RTC initialized");

  /*--- BME ---*/
  pSerialConsole->print(log_prefix);
  pSerialConsole->println("initializing BME");

  // look for the BME sensor
  while (!bme.begin()) {
    pSerialConsole->print(log_prefix);
    pSerialConsole->println("Could not find a valid BME680 sensor");
    pSerialConsole->flush();
    blink_led(4);
    delay(fail_loop_delay);
  }

  // configure BME sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms
  pSerialConsole->print(log_prefix);
  pSerialConsole->println("BME initialized");

  pSerialConsole->print(log_prefix);
  pSerialConsole->println("initializing SD card");

  /*--- SD CARD ---*/
  // configure SPI for the SD card
  SPI.setRX(SD_MISO);
  SPI.setTX(SD_MOSI);
  SPI.setSCK(SD_SCK);

  while (!SD.begin(SD_CS)) {
    pSerialConsole->print(log_prefix);
    pSerialConsole->println("SD initialization failed!");
    pSerialConsole->flush();
    blink_led(5);
    delay(fail_loop_delay);
  }
  SdFile::dateTimeCallback(dtCallback);
  pSerialConsole->print(log_prefix);
  pSerialConsole->println("SD initialized");

  pSerialConsole->print(log_prefix);
  pSerialConsole->println("complete");
}



void printReadingValues() {
  pSerialConsole->print(log_prefix);
  pSerialConsole->print("Temperature = ");
  pSerialConsole->print(bme.temperature);
  pSerialConsole->println(" *C");

  pSerialConsole->print(log_prefix);
  pSerialConsole->print("Pressure = ");
  pSerialConsole->print(bme.pressure / 100.0);
  pSerialConsole->println(" hPa");

  pSerialConsole->print(log_prefix);
  pSerialConsole->print("Humidity = ");
  pSerialConsole->print(bme.humidity);
  pSerialConsole->println(" %");

  pSerialConsole->print(log_prefix);
  pSerialConsole->print("Gas = ");
  pSerialConsole->print(bme.gas_resistance / 1000.0);
  pSerialConsole->println(" KOhms");
}

/**
 * Create a String with the header line to be logged to SD card.
 */
String makeHeaderLine() {
  String headerLine{};
  headerLine += "timestamp";
  headerLine += data_delimiter;
  headerLine += "temp_C";
  headerLine += data_delimiter;
  headerLine += "pressure_hPa";
  headerLine += data_delimiter;
  headerLine += "humidity_pct";
  headerLine += data_delimiter;
  headerLine += "gas_resistance_kohm";
  return headerLine;
}

/**
 * Create a String with the data to be logged to SD card.
 */
String makeDataLine(const char* timestamp) {
  String dataLine{ timestamp };
  dataLine += data_delimiter;
  dataLine += bme.temperature;  // C
  dataLine += data_delimiter;
  dataLine += (bme.pressure / 100.0);  // hPa
  dataLine += data_delimiter;
  dataLine += bme.humidity;  // %
  dataLine += data_delimiter;
  dataLine += (bme.gas_resistance / 1000.0);  // kOhms
  return dataLine;
}



void loop() {

  // uncomment these three lines to monitor memory
  // PRINT_FUNC_NAME(console);
  // console->print("free heap: ");
  // console->println(rp2040.getFreeHeap());


  // want RTC time and reading temporally adjacent
  DateTime now = rtc.now();
  if (bme.performReading()) {
    const time_t now_t = now.unixtime();
    const auto now_tm = gmtime(&now_t);

    char timestamp_buffer[sizeof("YYYY-MM-DD HH:MI:SS")];

    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%d %H:%M:%S", now_tm);

    String dataLine = makeDataLine(timestamp_buffer);

    pSerialConsole->print(data_prefix);
    pSerialConsole->println(dataLine);

    char filename[sizeof("YYYYMMDD.csv")];
    strftime(filename, sizeof(filename), "%Y%m%d.csv", now_tm);
    const bool file_exists = SD.exists(filename);
    if (File dataFile = SD.open(filename, FILE_WRITE)) {
      if (!file_exists) {
        dataFile.println(makeHeaderLine());
      }
      gpio_put(LED_BUILTIN, true);
      dataFile.println(dataLine);
      gpio_put(LED_BUILTIN, false);
      dataFile.close();
    }

  } else {
    pSerialConsole->print(log_prefix);
    pSerialConsole->println("Failed to perform BME reading :(");
  }

  delay(main_loop_delay);
}
