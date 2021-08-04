#include <SD.h>

//Arduino Micro
//Sonar Sensor: MaxBotix MB7052-100       | Data Pin 9                              | https://www.maxbotix.com/articles/095.htm
//RTC: PCF 8523                           | SDA(Pin 2), SCL(Pin 3), Wake(Pin 4)     | https://learn.adafruit.com/adafruit-pcf8523-real-time-clock/rtc-with-arduino
//SD-Card: Adafruit Micro-SD Breakout+    | Pin MISO, MSIO, SLCK, 7                 | https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/introduction
//Water Temp: Adafruit DS18B20            | Pin 2                                   | https://www.adafruit.com/product/381
//Climate Data: Adafruit BME280           | SCK(20), SDO(N/A), SDI(21), CS(N/A)     | https://www.adafruit.com/product/2652
//Communication: RockBLOCK 19354          | Sleep(pin 8) Comm(RX, TX)               | https://github.com/mikalhart/IridiumSBD, http://arduiniana.org/libraries/iridiumsbd/

#define transmitPeriod 360 // In minutes 
#define recordPeriod 10 // In minutes

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


#define readings 4
#define rLength 10
#define payload 6 + readings * rLength


struct tm isbd_time;
long t;
#define INTERRUPT_PIN 3
#define sonarPin 9
#define ONE_WIRE_BUS 2 //Water Temp Sensor
#define SDPin 53 //SD Card Select Pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int sonar[3], dist[readings];
int count = 0;
int water_temp[readings], air_temp[readings], humidity[readings], pressure[readings];
IridiumSBD isbd(Serial1, 8);
File myFile;
byte message[payload];
bool sdIsInit = false;

// No longer needed because I2C is currently being used.
// #define BME_SCK 52
// #define BME_MISO 50
// #define BME_MOSI 51
// #define BME_CS 53

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);  // Software SPI
//Adafruit_BME280 bme(BME_CS);  // Hardware SPI

void wakeUp()
{
  Serial.println("Awake!");
  sleep_disable();
  detachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN));
}

RTC_PCF8523 rtc;

void setup() {
  digitalWrite(8, LOW);
  Serial.begin(19200);
  Serial1.begin(19200);
  Serial1.begin(19200);
  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }
  delay(1000);
  Serial.println("Serial started");

  if (!rtc.begin()) {
    Serial.println("Clock not started.");
  } else {
    Serial.println("Clock started.");
  }

  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  delay(1000);

  Serial.println("Sleeping...");
  // Put the Arduino to sleep.
  sleep_enable();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  rtc.deconfigureAllTimers();
  rtc.enableCountdownTimer(PCF8523_FrequencySecond, 10);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), wakeUp, LOW);
  delay(1000); // Helps to ensure that any writes or serial prints finish before sleeping.
  sleep_cpu();

  sensors.begin();

  //Initialize BME280
  if (!bme.begin(0x76)) {
    Serial.println("Could not find BME280 Climate Sensor.");
  } else {
    Serial.println("BME820 Climate Sensor initialized.");
  }

  //Initialize Adafruit Micro SD Breakout+
  pinMode(SDPin, OUTPUT);
  sdIsInit = SD.begin(SDPin);
  if (!sdIsInit) {
    Serial.println("Micro SD Card Reader could not initialize.");
  } else {
    Serial.println("Micro SD Card Reader initalized.");
  }

  Serial.println("Debug 0");
  isbd.useMSSTMWorkaround(false);
  isbd.begin();

  Serial.println("Debug 1");
  //First sensor reading always wrong so lets get it out of the way
  sensors.requestTemperatures();

  bme.readTemperature();
  bme.readHumidity();
  bme.readPressure();

  Serial.println("Sat Comms Started");
  Serial.print("Getting current time");
  //  while(isbd.getSystemTime(isbd_time) != ISBD_SUCCESS){
  //    delay(1000);
  //    Serial.print(".");
  //  }
  //  Serial.println();
  //  rtc.adjust(DateTime(isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday, isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec));
  Serial.println("Clock Set to ");
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('-');
  Serial.print(now.month(), DEC);
  Serial.print('-');
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print("Taking Measurement every ");
  Serial.print(transmitPeriod);
  Serial.print(" Minutes");
  Serial.println();
}

// Macro should work with floats and doubles
#define float_to_int(x) (x >= 0 ? (int)(x + 0.5) : (int)(x - 0.5))

void insertion_sort(int *arr, size_t arr_size)
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

void swap(int *arr, int i, int j)
{
  int temp = arr[j];
  arr[j] = arr[i];
  arr[i] = temp;
}

int median(int arr[], size_t arr_size)
{
  insertion_sort(arr, arr_size);
  if (arr_size & 1) { // Bitwise check for odd number (faster than x % 2 == 0)
    return arr[(arr_size) / 2];
  } else {
    return float_to_int((arr[(arr_size / 2) - 1] + arr[(arr_size / 2) + 1]) / 2.0f);
  }
}

// Memory card formatted as FAT32, so this function produces a
// file name using 8.3 naming format (otherwise we will surpass amount name quickly).
String Unixfile(String U)
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

void loop() {
  Serial.print("Recording measurement: ");
  Serial.println(count);
  if (count == 0) {
    t = rtc.now().unixtime();
  }
  Serial.print("t: ");
  Serial.print(t);
  Serial.print("\n");
  Serial.println(pulseIn(sonarPin, HIGH));
  sonar[0] = pulseIn(sonarPin, HIGH) / 57.87;
  Serial.print("Sonar measurement #1: ");
  Serial.println(sonar[0]);
  sonar[1] = pulseIn(sonarPin, HIGH) / 57.87;
  Serial.print("Sonar measurement #2: ");
  Serial.println(sonar[1]);
  sonar[2] = pulseIn(sonarPin, HIGH) / 57.87;
  Serial.print("Sonar measurement #3: ");
  Serial.println(sonar[2]);
  // TODO: Change this to take the median value.
  dist[count] = (sonar[0] + sonar[1] + sonar[2]) / 3;
  Serial.print("Sonar measurement avg: ");
  Serial.println(dist[count]);

  sonar[0] = 25;
  sonar[1] = 21;
  sonar[2] = 23;
  insertion_sort(sonar, 3);
  Serial.print("Sorted Sonar measurement #1: ");
  Serial.println(sonar[0]);
  Serial.print("Sorted Sonar measurement #2: ");
  Serial.println(sonar[1]);
  Serial.print("Sorted Sonar measurement #3: ");
  Serial.println(sonar[2]);
  Serial.print("Median is: ");
  Serial.println(median(sonar, 3));
  
  sensors.requestTemperatures();
  water_temp[count] = float_to_int(sensors.getTempCByIndex(0) * 100);
  Serial.print("Water Temp: ");
  Serial.println(water_temp[count]);

  air_temp[count] = float_to_int(bme.readTemperature() * 100);
  Serial.print("Air Temp: ");
  Serial.println(air_temp[count]);
  humidity[count] = float_to_int(bme.readHumidity() * 100);
  Serial.print("Humidity: ");
  Serial.println(humidity[count]);
  pressure[count] = float_to_int(bme.readPressure());
  Serial.print("Pressure: ");
  Serial.println(pressure[count]);

  // Write to Micro SD Card.
  // Formatted in JSON.
  String file = Unixfile(String(rtc.now().unixtime()));
  if (sdIsInit) {
    if (!SD.exists(file)) {
      Serial.println(rtc.now().unixtime());
      Serial.println(file);
      myFile = SD.open(file, FILE_WRITE);
      myFile.print("{\n");
      myFile.print("\t\"sonar\": \"");
      myFile.print(dist[count]);
      myFile.print("\",\n");
      myFile.print("\t\"waterTemp\": \"");
      myFile.print(water_temp[count]);
      myFile.print("\",\n");
      myFile.print("\t\"airTemp\": \"");
      myFile.print(air_temp[count]);
      myFile.print("\",\n");
      myFile.print("\t\"pressure\": \"");
      myFile.print(pressure[count]);
      myFile.print("\",\n");
      myFile.print("\t\"humidity\": \"");
      myFile.print(humidity[count]);
      myFile.print("\",\n");
      myFile.print("}");
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
  if (count < readings - 1) {
    count ++;
  }
  else {
    count = 0;
    //Time (4)
    message[0] = (long) t >> 24;
    message[1] = (long) t >> 16;
    message[2] = (long) t >> 8;
    message[3] = (long) t;
    //Diff (2)
    message[4] = (int) transmitPeriod >> 8;
    message[5] = (int) transmitPeriod;
    for (int i = 0; i < readings; i++)
    {
      //Distance
      message[6 + (i * rLength)] = (int) dist[i] >> 8;
      message[7 + (i * rLength)] = (int) dist[i];
      //Water Temp
      message[8 + (i * rLength)] = (int) water_temp[i] >> 8;
      message[9 + (i * rLength)] = (int) water_temp[i];
      //Air Temp
      message[10 + (i * rLength)] = (int) air_temp[i] >> 8;
      message[11 + (i * rLength)] = (int) air_temp[i];
      //Humidity
      message[12 + (i * rLength)] = (int) humidity[i] >> 8;
      message[13 + (i * rLength)] = (int) humidity[i];
      //Pressure
      message[14 + (i * rLength)] = (int) pressure[i] >> 8;
      message[15 + (i * rLength)] = (int) pressure[i];
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
    isbd.sendSBDBinary(message, payload);
    Serial.println("Sent");
  }
  delay(1000L * transmitPeriod * 60);
  //delay(500);
}
