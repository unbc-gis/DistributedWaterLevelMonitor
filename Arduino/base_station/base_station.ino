//Arduino Micro
//Sonar Sensor: MaxBotix MB7052-100       | Data Pin 5                              | https://www.maxbotix.com/articles/095.htm
//RTC: PCF 8523                           | SDA(Pin 2), SCL(Pin 3), Wake(Pin 4)     | https://learn.adafruit.com/adafruit-pcf8523-real-time-clock/rtc-with-arduino
//SD-Card: Adafruit Micro-SD Breakout+    | Pin MISO, MSIO, SLCK, 7                 | https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/introduction
//Water Temp: Adafruit DS18B20            | Pin 6                                   | https://www.adafruit.com/product/381
//Climate Data: Adafruit BME280           | SCK(9), SDO(12), SDI(11), CS(10)        | https://www.adafruit.com/product/2652
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

int wakePin = 4;
int sigQuality = 6;
struct tm isbd_time;
const int sonarPin = 5;
#define ONE_WIRE_BUS 6 //Water Temp Sensor
const int chipSelect = 7; //SD Card Select Pin
Sd2Card card;
SdVolume volume;
SdFile root;
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
long pulse, cm1, cm2, cm3, dist;
float water_temp, air_temp, humidity, pressure;
IridiumSBD isbd(Serial1, 8); 

#define BME_SCK 9
#define BME_MISO 12
#define BME_MOSI 11 
#define BME_CS 10
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

RTC_PCF8523 rtc;

void setup() {
  //Serial.begin(19200);
  Serial1.begin(19200); 
  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }
  //Serial.println("Serial Established");
  if (! rtc.begin()) {
    //Serial.println("Couldn't find RTC");
    while (1);
  }

  if (!bme.begin()) {  
    //Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  if (!card.init(SPI_HALF_SPEED, chipSelect)) {
    //Serial.println("initialization failed. Things to check:");
    //Serial.println("* is a card inserted?");
    //Serial.println("* is your wiring correct?");
    //Serial.println("* did you change the chipSelect pin to match your shield or module?");
    while (1);
  } 
  pinMode(wakePin, INPUT);
  pinMode(sonarPin, INPUT);
  //Serial.println("Starting RockBlock");
  isbd.begin();
  //Serial.println("RockBlock Initalized");
  //Serial.print("Getting Iridium Time");
  while(isbd.getSystemTime(isbd_time) != ISBD_SUCCESS){
    //Serial.print(".");
    delay(1000);
  }
  //Serial.println();
  //char buf[32];
  //sprintf(buf, "%d-%02d-%02d %02d:%02d:%02d",
  //  isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday, isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec);
  //Serial.print("Iridium time/date is ");
  //Serial.print(buf);
  //Serial.println();
  rtc.adjust(DateTime(isbd_time.tm_year, isbd_time.tm_mon, isbd_time.tm_mday, isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec));
  //attachInterrupt(0, wakeUpNow, LOW);
}

void wakeUpNow()// here the interrupt is handled after wakeup
{
  
}
//Function to put Arduino to sleep
void sleepNow() {
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  attachInterrupt(0, wakeUpNow, LOW);
  sleep_mode();
  //Arduino is sleeping here
  sleep_disable();
  detachInterrupt(0);
}
void loop() {
  DateTime now = rtc.now();
  pulse = pulseIn(sonarPin, HIGH);
  cm1 = pulse/57.87;
  cm2 = pulse/57.87;
  cm3 = pulse/57.87;
  dist = (cm1 + cm2 + cm3) / 3;
  sensors.requestTemperatures();
  water_temp = sensors.getTempCByIndex(0);
  air_temp = bme.readTemperature();
  humidity = bme.readPressure()/100;
  pressure = bme.readPressure()/100;

  //Serial.print(now.hour(), DEC);
  //Serial.print(':');
  //Serial.print(now.minute(), DEC);
  //Serial.print(':');
  //Serial.print(now.second(), DEC);
  //Serial.print(" - ");
  //erial.print(dist);
  //Serial.print("cm, ");
  //Serial.print("water: ");
  //Serial.print(water_temp);
  //Serial.print("C, Air: ");
  //Serial.print(ait_temp);
  //Serial.print("C, Pressure: ");
  //Serial.print(pressure);
  //Serial.print("hPa, Humidity: "); 
  //Serial.print(humidity);
  //Serial.print("%, RSSI: ");
  //isbd.getSignalQuality(sigQuality);
  //Serial.print(sigQuality);
  //Serial.println();
  
  delay(4000);
// sleepNow();
}
