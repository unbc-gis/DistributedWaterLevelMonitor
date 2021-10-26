//Arduino Micro
//Sonar Sensor: MaxBotix MB7052-100       | Data Pin 9                              | https://www.maxbotix.com/articles/095.htm
//RTC: PCF 8523                           | SDA(Pin 2), SCL(Pin 3), Wake(Pin 4)     | https://learn.adafruit.com/adafruit-pcf8523-real-time-clock/rtc-with-arduino
//SD-Card: Adafruit Micro-SD Breakout+    | Pin MISO, MSIO, SLCK, 6                 | https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/introduction
//Water Temp: Adafruit DS18B20            | Pin 2                                   | https://www.adafruit.com/product/381
//Climate Data: Adafruit BME280           | SCK(20), SDO(N/A), SDI(21), CS(N/A)     | https://www.adafruit.com/product/2652
//Communication: RockBLOCK 19354          | Sleep(pin 8) Comm(RX, TX)               | https://github.com/mikalhart/IridiumSBD, http://arduiniana.org/libraries/iridiumsbd/

#include <avr/sleep.h>
#include "RTClib.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <time.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "IridiumSBD.h"
#include <math.h>


/* Preprocessor Macros */

// Round a float to an int
#define floatToInt(x) (x >= 0 ? (int)(x + 0.5) : (int)(x - 0.5))


/* Time Periods */

// For best results recordPeriod should be a clean divisor of transmitPeriod.
#define transmitPeriod 4 // In minutes
#define recordPeriod 1 // In minutes

// We will use the above to calculate the number of readings before to record
// to the SD card. "transmitReadings" macro below defines how many will be sent
// via sat module.
const unsigned int totalReadings = (int)(ceil(transmitPeriod / (float)recordPeriod));

// Payload length
#define transmitReadings 4
#define rLength 10
#define payload 6 + transmitReadings * rLength

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;


/* Arduino Pins */

//struct tm isbd_time;
#define INTERRUPT_PIN 3 // Used for RTC Countdown Timer
#define SONAR_PIN 4     // Sonar Sensor
#define ONE_WIRE_BUS 5  // Water Temp Sensor
#define SD_PIN_CS 53    // SD Card Select Pin
#define SD_PIN_CD 6     // SD Card Card Detect Pin
//#define BME_SCK 52
//#define BME_MISO 50
//#define BME_MOSI 51 
//#define BME_CS 49       // BME280 SPI Pin


/* Sensor Module Data Structures */

IridiumSBD isbd(Serial1, 2);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI
RTC_PCF8523 rtc;


/* Measurements / Global Storage */

int count = 0;
int payload_sonar_dist[transmitReadings], payload_water_temp[transmitReadings],
    payload_air_temp[transmitReadings], payload_humidity[transmitReadings],
    payload_pressure[transmitReadings];
File myFile;
int currentReadingInterval = 0;
int readingIntervals[transmitReadings] = {0};
byte message[payload];
bool sdIsInit = false;
DateTime currentTime;


// Put Arduino to sleep.
void goToSleep()
{
  Serial.println("Sleeping...");
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);

  // Handles odd combinations of transmit period and record period that don't divide evenly.
  if (transmitPeriod % recordPeriod != 0) {
    if (count == totalReadings - 1) {
      rtc.enableCountdownTimer(PCF8523_FrequencyMinute, transmitPeriod % recordPeriod);
    } else {
      rtc.enableCountdownTimer(PCF8523_FrequencyMinute, recordPeriod);
    }
  } else {
    rtc.enableCountdownTimer(PCF8523_FrequencyMinute, recordPeriod);
  }

  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), wakeUp, LOW);

  // Disabled the delay(1000) line below. It's causing a 1 second clock drift on the countdown
  // timer but not everything prints out on the serial before the Arduino goes to sleep.
  // Good to enable for debugging purposes.

    delay(1000); // Helps to ensure that any writes or serial prints finish before sleeping.
  sleep_cpu();
}

void wakeUp()
{
  Serial.println("Awake!");
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
}

int checkI2cDeviceStatus(int address) {
  Wire.beginTransmission(address);
  unsigned int error = Wire.endTransmission();

  return error;
}

void printTime()
{
  DateTime t = rtc.now();
  Serial.print(t.toString("YYYY"));
  Serial.print('-');
  Serial.print(t.toString("MM"));
  Serial.print('-');
  Serial.print(t.toString("DD"));
  Serial.print(" ");
  Serial.print(t.toString("hh"));
  Serial.print(':');
  Serial.print(t.toString("mm"));
  Serial.print(':');
  Serial.print(t.toString("ss"));
  Serial.println();
}

// Insertion sort works well for low number of indices.
void insertionSort(int *arr, size_t arr_size)
{
  int i = 1;
  while (i < arr_size) {
    int j = i;
    while (j > 0 && arr[j - 1] > arr[j]) {
      swap(arr, j, j - 1);
      j--;
    }
    i++;
  }
}

// Simple array swap.
void swap(int *arr, int i, int j)
{
  int temp = arr[j];
  arr[j] = arr[i];
  arr[i] = temp;
}

// Find the median of a sorted array.
int median(int arr[], size_t arr_size)
{
  insertionSort(arr, arr_size);
  if (arr_size % 2 == 0) {
    return floatToInt((arr[(arr_size / 2) - 1] + arr[(arr_size / 2) + 1]) / 2.0f);
  } else {
    return arr[(arr_size) / 2];
  }
}

// Memory card formatted as FAT32, so this function produces a
// file name using the "8.3 naming format". FAT32 can only support
// the 8.3 naming format if not using special extensions that the
// SD.h library does not seem to support.
String convertToFat32CompatibleName(String U)
{
  char Name[12] = {0};
  for (int i = 0; i < 7; i++)
  {
    Name[i] = U[i];
  }
  Name[7] = '.';
  for (int i = 7; i < 10; i++)
  {
    Name[i + 1] = U[i];
  }

  return Name;
}

void setup() {
  Wire.begin();
  
  digitalWrite(8, LOW);

  // Begin debugging I/O
  Serial.begin(19200);

  // Begin serial TX/RX for Sat Module.
  Serial1.begin(19200);

  // Tried this to see if it would fix time issues with the sat module
  //  isbd.setPowerProfile(IridiumSBD::USB_POWER_PROFILE);

  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }
  delay(1000);
  Serial.println("Serial started");

  Serial.print("Number of readings: ");
  Serial.println(totalReadings);

  // Required for RTC to countdown each reading interval.
  // Interrupt pin should be pulled up to HIGH until we need to wake
  // the Arduino fom sleep mode via the countdown timer. (Countdown
  // timer drops voltage from HIGH to LOW at the end of each interval).
  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  delay(1000);
  Serial.println("Interrupt pin set to high.");

  // Disabling for now. Had issues with the RTC going to nonsense times.
  // PCF8523 countdown timer example sketch said to use this once at the start
  // to clear registers or something but I had a suspicion it was causing the issue.
  // Hopefully disabling this fixes it?
  //  rtc.deconfigureAllTimers();

//  // Initialize PCF8523
  if (!rtc.begin()) {
    Serial.println("Clock not started.");
  } else {
    Serial.println("Clock started.");
  }

  // Initialize Adafruit DS18B20
  sensors.begin();
  Serial.println("Adafruit DS18B20 inititalized.");

  // Initialize BME280
  if (!bme.begin(0x77)) { // I2C
  //if (!bme.begin()) { // SPI
    Serial.println("Could not find BME280 Climate Sensor.");
  } else {
    Serial.println("BME820 Climate Sensor initialized.");
  }

  // Initialize Adafruit Micro SD Breakout+
  pinMode(SD_PIN_CS, OUTPUT);
  sdIsInit = SD.begin(SD_PIN_CS);
  if (!sdIsInit) {
    Serial.println("Micro SD Card Reader could not initialize.");
  } else {
    Serial.println("Micro SD Card Reader initalized.");
  }

  // Pin used to check if SD Card is inserted. Pulls up to high when not
  // inserted and low when inserted.
  pinMode(SD_PIN_CD, INPUT_PULLUP);

   //First sensor reading always wrong so lets get it out of the way
  sensors.requestTemperatures();
  bme.readTemperature();
  bme.readHumidity();
  bme.readPressure();

  int isbd_err = isbd.begin();
  if (isbd_err != ISBD_SUCCESS) {
    Serial.println("RockBLOCK could not intialize.");
  } else {
    Serial.println("RockBLOCK initialized.");
  }
//  isbd.useMSSTMWorkaround(false);

  Serial.println("Sat Comms Started");
  Serial.println("Getting current time");
//  while(isbd.getSystemTime(isbd_time) != ISBD_SUCCESS){
//    delay(1000);
//    Serial.print(".");
//  }

  Serial.println("Debug 2");
  //  rtc.adjust(DateTime(isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday, isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec));
  //  Serial.println("Clock Set to ");

  // This is used to keep track of which intervals we want to send.
  // For example with a 360 min transmit period, and a period of 15 minutes for
  // recording to SD card, there will be 24 readings altogether before satellite
  // transmission. Because we can only send 4 of them via the sat module, we
  // would want to send every 6th, 12th, 18th, and 24th reading to get an even
  // spread. The math with all the modulos just covers us in the unlikely event
  // where the transmit period, the recording period, and the number of readings
  // to send via satellite don't divide nicely.
  for (int i = 0; i < transmitReadings; i++) {
    readingIntervals[i] = (i + 1) * (floor(totalReadings / transmitReadings))
                          + ((count < (totalReadings % transmitReadings)) ? (count + 1) : count);
    Serial.println("Reading interval #" + String(i + 1) + ": " + String(readingIntervals[i]));
  }
}



void loop() {
  Serial.println("Loop!");
  
  currentTime = rtc.now();
  int sonar[3], sonar_dist;
  int water_temp, air_temp, humidity, pressure;

  Serial.print("Date/Time according to RTC: " + currentTime.timestamp() + "; ");
  Serial.print("Taking measurement every " + String(recordPeriod) + " minutes" + "; ");
  Serial.print("Transmitting every " + String(transmitPeriod) + " minutes" + "; ");
  Serial.print("Recording measurement #:" + String(count) + "; ");

  Serial.println();

  sonar[0] = pulseIn(SONAR_PIN, HIGH) / 57.87;
  sonar[1] = pulseIn(SONAR_PIN, HIGH) / 57.87;
  sonar[2] = pulseIn(SONAR_PIN, HIGH) / 57.87;
  sonar_dist = median(sonar, 3);
  Serial.print("Sonar measurements: " + String(sonar[0]) + ", "
      + String(sonar[1]) + ", " + String(sonar[2]) + ", med: " + String(sonar_dist)+ "; ");

  sensors.requestTemperatures();
  water_temp = floatToInt(sensors.getTempCByIndex(0) * 100);
  Serial.print("Water temp: " + String(water_temp) + "; ");

  air_temp = floatToInt(bme.readTemperature() * 100);
  Serial.print("Air Temp: " + String(air_temp) + "; ");

  humidity = floatToInt(bme.readHumidity() * 100);
  Serial.print("Humidity: " + String(humidity) + "; ");

  pressure = floatToInt(bme.readPressure());
  Serial.print("Pressure: " + String(pressure) + "; ");

  Serial.print("Count: " + String(count) + "; ");
  Serial.print("Reading interval: " + String(readingIntervals[currentReadingInterval] - 1) + "; ");

  if (count == (readingIntervals[currentReadingInterval] - 1)) {
//    Serial.println("Writing payload reading #: " + String(count));
    payload_sonar_dist[currentReadingInterval] = sonar_dist;
//    Serial.println("Sonar dist: " + String(sonar_dist));
//    Serial.println("Sonar dist: " + String(payload_sonar_dist[currentReadingInterval]));
    payload_water_temp[currentReadingInterval] = water_temp;
//    Serial.println("Water temp: " + String(water_temp));
//    Serial.println("Water temp: " + String(water_temp));
    payload_air_temp[currentReadingInterval] = air_temp;
//    Serial.println("Air temp: " + String(air_temp));
//    Serial.println("Air temp: " + String(payload_air_temp[currentReadingInterval]));
    payload_humidity[currentReadingInterval] = humidity;
//    Serial.println("Humidity: " + String(humidity));
//    Serial.println("Humidity: " + String(payload_humidity[currentReadingInterval]));
    payload_pressure[currentReadingInterval] = pressure;
//    Serial.println("Pressure: " + String(pressure));
//    Serial.println("Pressure: " + String(payload_pressure[currentReadingInterval]));
    currentReadingInterval++;
  }

  Serial.println();

  // Write to Micro SD Card.
  // Formatted in JSON.

  String file = convertToFat32CompatibleName(String(currentTime.unixtime()));
  if (sdIsInit && digitalRead(SD_PIN_CD) == HIGH) {
    if (!SD.exists(String("/") + String(currentTime.toString("YYYYMM")))) {
      Serial.println("Directory for YYYYMM does not exist on SD Card. Creating...");
      SD.mkdir(currentTime.toString("YYYYMM"));
    } else {
      Serial.println("Found directory on SD Card.");
    }

    if (!SD.exists(file)) {
      Serial.print("Writing to: ");
      Serial.println("/" + String(currentTime.toString("YYYYMM")) + "/" + file);
      myFile = SD.open("/" + String(currentTime.toString("YYYYMM")) + "/" + file, FILE_WRITE);
      myFile.print("{\n");
      myFile.print("\t\"iso8601Timestamp\": \"");
      myFile.print(currentTime.timestamp());
      myFile.print("\",\n");
      myFile.print("\t\"unixTimestamp\": \"");
      myFile.print(currentTime.unixtime());
      myFile.print("\",\n");
      myFile.print("\t\"sonar\": \"");
      myFile.print(sonar_dist);
      myFile.print("\",\n");
      myFile.print("\t\"waterTemp\": \"");
      myFile.print(water_temp);
      myFile.print("\",\n");
      myFile.print("\t\"airTemp\": \"");
      myFile.print(air_temp);
      myFile.print("\",\n");
      myFile.print("\t\"Pressure\": \"");
      myFile.print(pressure);
      myFile.print("\",\n");
      myFile.print("\t\"Humidity\": \"");
      myFile.print(humidity);
      myFile.print("\",\n");
      myFile.print("\t\"Current Reading Interval\": \"");
      myFile.print(currentReadingInterval);
      myFile.flush();
      myFile.close();
      Serial.println("Done writing to file.");
    } else {
      Serial.println("File already exists.");
    }
  } else {
    Serial.println("SD Card reader/writer did not initialize -- no file written.");
  }

  // Prepare to transmit after certain number of readings.
  if (count < totalReadings - 1) {
    count ++;
  }
  else {
    count = 0;
    currentReadingInterval = 0;
    Serial.println("Final reading before count = 0.");
    long t = currentTime.unixtime();
    //Time (4)
    message[0] = (long) t >> 24;
    message[1] = (long) t >> 16;
    message[2] = (long) t >> 8;
    message[3] = (long) t;
    //Diff (2)
    message[4] = (int) transmitPeriod >> 8;
    message[5] = (int) transmitPeriod;
    for (int i = 0; i < transmitReadings; i++)
    {
      //      Serial.println("Sonar dist #" + String(i) + ": " + String(payload_sonar_dist[i]));
      //      Serial.println("Water temp #" + String(i) + ": " + String(payload_water_temp[i]));
      //      Serial.println("Air temp #" + String(i) + ": " + String(payload_air_temp[i]));
      //      Serial.println("Humidity #" + String(i) + ": " + String(payload_humidity[i]));
      //      Serial.println("Pressure #" + String(i) + ": " + String(payload_pressure[i]));
      //Distance
      message[6 + (i * rLength)] = (int) payload_sonar_dist[i] >> 8;
      message[7 + (i * rLength)] = (int) payload_sonar_dist[i];
      //Water Temp
      message[8 + (i * rLength)] = (int) payload_water_temp[i] >> 8;
      message[9 + (i * rLength)] = (int) payload_water_temp[i];
      //Air Temp
      message[10 + (i * rLength)] = (int) payload_air_temp[i] >> 8;
      message[11 + (i * rLength)] = (int) payload_air_temp[i];
      //Humidity
      message[12 + (i * rLength)] = (int) payload_humidity[i] >> 8;
      message[13 + (i * rLength)] = (int) payload_humidity[i];
      //Pressure
      message[14 + (i * rLength)] = (int) payload_pressure[i] >> 8;
      message[15 + (i * rLength)] = (int) payload_pressure[i];
    }
    Serial.print("Sending Message: ");
    for (int i = 0; i < payload; i += 2) {
      char buff[2];
      sprintf(buff, "%02X", message[i]);
      Serial.print(buff);
      sprintf(buff, "%02X", message[i + 1]);
      Serial.print(buff);
      //Serial.print(" ");
    }
    Serial.println();
//    isbd.sendSBDBinary(message, payload);
    Serial.println("Sent");

    for (int i = 0; i < transmitReadings; i++) {
      payload_sonar_dist[i] = 0;
      payload_water_temp[i] = 0;
      payload_air_temp[i] = 0;
      payload_humidity[i] = 0;
      payload_pressure[i] = 0;
    }

    struct tm isbd_time;
    char isbdbuffer[32];
    sprintf(isbdbuffer, "%d-%02d-%02d %02d:%02d:%02d",
       isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday,
       isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec);

    Serial.print("Before tm change:" + String(isbdbuffer) + "; ");
    int isdb_err = isbd.getSystemTime(isbd_time);
    Serial.print("ISBD Error Code: " + String(isdb_err) + "; ");
    if (isdb_err == ISBD_SUCCESS) {
      sprintf(isbdbuffer, "%d-%02d-%02d %02d:%02d:%02d",
         isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday,
         isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec);
      Serial.print("Iridium time/date is " + String(isbdbuffer) + "; ");
      rtc.adjust(DateTime(isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday,
         isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec));
      Serial.print("Adjusting RTC time to Iridium time.; ");
    } else {
      Serial.print("Err: Couldn't get ISBD time; ");
    }
  
    Serial.println();
  
    if (SD.exists("/" + String(currentTime.toString("YYYYMM")))) {
      myFile = SD.open("/" + String(currentTime.toString("YYYYMM")) + "/" + file, FILE_WRITE);

      myFile.print(",\n");
      myFile.print("\t\"ISBD Error Code\": \"");
      myFile.print(isdb_err);
      myFile.print("\",\n");

      myFile.print("\t\"ISBD Time\": \"");
      myFile.print(isbdbuffer);
      myFile.print("\",\n");

      
      myFile.print("\t\"Payload\": \"");
      
      for (int i = 0; i < payload; i += 2) {
        char buff[2];
        sprintf(buff, "%02X", message[i]);
        myFile.print(buff);
        sprintf(buff, "%02X", message[i + 1]);
        myFile.print(buff);
      
      }
      myFile.flush();
      myFile.close();
    } else {
      Serial.println("File does not exist.");
    }
  }

  if (SD.exists("/" + String(currentTime.toString("YYYYMM")))) {
    myFile = SD.open("/" + String(currentTime.toString("YYYYMM")) + "/" + file, FILE_WRITE);
    myFile.print("\"\n");
    myFile.print("}");
    myFile.flush();
    myFile.close();
  } else {
    Serial.println("File does not exist.");
  }

  //  delay(1000L * transmitPeriod * 60);
  if (!checkI2cDeviceStatus(0x68)) {
    goToSleep();
  } else {
    delay(1000L * transmitPeriod * 60);
  }
}

//void ISBDConsoleCallback(IridiumSBD *device, char c)
//{
//  Serial.write(c);
//}
//
//void ISBDDiagsCallback(IridiumSBD *device, char c)
//{
//  Serial.write(c);
//}
