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

auto& console = Serial;
const auto baud = 115200;  // value is ignored for USB serial
const auto delimiter = ',';
const auto main_loop_delay = 1000;
const auto fail_loop_delay = 5000;

RTC_PCF8523 rtc;
Adafruit_BME680 bme;  // I2C

// macro to print the current function name to specified serial port
#define PRINT_FUNC_NAME(_ser) \
  _ser.print("["); \
  _ser.print(__func__); \
  _ser.print("] ");


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

  console.begin(baud);
  delay(1000);
  while (!console) {
    // wait for serial port to connect
    blink_led(1);
    delay(1000);
  }


  /*--- RTC ---*/
  PRINT_FUNC_NAME(console);
  console.println("initializing RTC");

  if (!rtc.begin()) {
    while (true) {
      PRINT_FUNC_NAME(console);
      console.println("Couldn't find RTC");
      console.flush();
      blink_led(2);
      delay(fail_loop_delay);
    }
  }
  // ensure RTC is running
  rtc.start();
  delay(2000);
  while (!rtc.isrunning()) {
    PRINT_FUNC_NAME(console);
    console.println("RTC is NOT running");
    console.flush();
    blink_led(3);
    delay(fail_loop_delay);
  }
  PRINT_FUNC_NAME(console); 
  console.println("RTC initialized");

  /*--- BME ---*/
  PRINT_FUNC_NAME(console);
  console.println("initializing BME");

  // look for the BME sensor
  while (!bme.begin()) {
    PRINT_FUNC_NAME(console);
    console.println("Could not find a valid BME680 sensor");
    console.flush();
    blink_led(4);
    delay(fail_loop_delay);
  }

  // configure BME sensor
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);  // 320*C for 150 ms
  PRINT_FUNC_NAME(console); 
  console.println("BME initialized");

  PRINT_FUNC_NAME(console);
  console.println("initializing SD card");

  /*--- SD CARD ---*/
  // configure SPI for the SD card
  SPI.setRX(SD_MISO);
  SPI.setTX(SD_MOSI);
  SPI.setSCK(SD_SCK);

  while (!SD.begin(SD_CS)) {
    PRINT_FUNC_NAME(console);
    console.println("SD initialization failed!");
    console.flush();
    blink_led(5);
    delay(fail_loop_delay);
  }
  SdFile::dateTimeCallback(dtCallback);
  PRINT_FUNC_NAME(console); 
  console.println("SD initialized");


  PRINT_FUNC_NAME(console);
  console.println("complete");
}



void printReadingValues() {
  PRINT_FUNC_NAME(console);
  console.print("Temperature = ");
  console.print(bme.temperature);
  console.println(" *C");

  PRINT_FUNC_NAME(console);
  console.print("Pressure = ");
  console.print(bme.pressure / 100.0);
  console.println(" hPa");

  PRINT_FUNC_NAME(console);
  console.print("Humidity = ");
  console.print(bme.humidity);
  console.println(" %");

  PRINT_FUNC_NAME(console);
  console.print("Gas = ");
  console.print(bme.gas_resistance / 1000.0);
  console.println(" KOhms");
}

/**
 * Create a String with the header line to be logged to SD card.
 */
String makeHeaderLine() {
  String headerLine{};
  headerLine += "timestamp";
  headerLine += delimiter;
  headerLine += "temp_C";
  headerLine += delimiter;
  headerLine += "pressure_hPa";
  headerLine += delimiter;
  headerLine += "humidity_pct";
  headerLine += delimiter;
  headerLine += "gas_resistance_kohm";
  return headerLine;
}

/**
 * Create a String with the data to be logged to SD card.
 */
String makeDataLine(const char* timestamp) {
  String dataLine{ timestamp };
  dataLine += delimiter;
  dataLine += bme.temperature;  // C
  dataLine += delimiter;
  dataLine += (bme.pressure / 100.0);  // hPa
  dataLine += delimiter;
  dataLine += bme.humidity;  // %
  dataLine += delimiter;
  dataLine += (bme.gas_resistance / 1000.0);  // kOhms
  return dataLine;
}



void loop() {

  // uncomment these three lines to monitor memory
  // PRINT_FUNC_NAME(console);
  // console.print("free heap: ");
  // console.println(rp2040.getFreeHeap());

  // want RTC time and reading temporally adjacent
  DateTime now = rtc.now();
  if (bme.performReading()) {

    const time_t now_t = now.unixtime();
    const auto now_tm = gmtime(&now_t);

    char timestamp_buffer[sizeof("YYYY-MM-DD HH:MI:SS")];

    strftime(timestamp_buffer, sizeof(timestamp_buffer), "%Y-%m-%d %H:%M:%S", now_tm);

    String dataLine = makeDataLine(timestamp_buffer);

    console.println(dataLine);

    char filename[sizeof("YYYYMMDD.csv")];
    strftime(filename, sizeof(filename), "%Y%m%d.csv", now_tm);
    const bool file_exists = SD.exists(filename);
    if (File dataFile = SD.open(filename, FILE_WRITE)) {
      if (!file_exists) {
        dataFile.println(makeHeaderLine());
      }
      dataFile.println(dataLine);
      dataFile.close();
    }

  } else {
    PRINT_FUNC_NAME(console);
    console.println("Failed to perform BME reading :(");
  }

  delay(main_loop_delay);
}
