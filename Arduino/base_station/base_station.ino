//Arduino Micro
//Sonar Sensor: MaxBotix MB7052-100       | Data Pin 5                              | https://www.maxbotix.com/articles/095.htm
//RTC: PCF 8523                           | SDA(Pin 2), SCL(Pin 3), Wake(Pin 4)     | https://learn.adafruit.com/adafruit-pcf8523-real-time-clock/rtc-with-arduino
//SD-Card: Adafruit Micro-SD Breakout+    | Pin MISO, MSIO, SLCK, 7                 | https://learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/introduction
//Water Temp: Adafruit DS18B20            | Pin 6                                   | https://www.adafruit.com/product/381
//Climate Data: Adafruit BME280           | SCK(9), SDO(12), SDI(11), CS(10)        | https://www.adafruit.com/product/2652
//Communication: RockBLOCK 19354          | Sleep(pin 8) Comm(RX, TX)               | https://github.com/mikalhart/IridiumSBD, http://arduiniana.org/libraries/iridiumsbd/

#define period 360 //In minutes 

#include <avr/sleep.h>
#include "RTClib.h"
#include <OneWire.h> 
#include <DallasTemperature.h>
#include <Wire.h>
#include <time.h>
#include <SPI.h>
//#include <SD.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "IridiumSBD.h"


#define readings 4
#define rLength 10
#define payload 6 + readings * rLength


struct tm isbd_time;
long t;
#define sonarPin 5
#define ONE_WIRE_BUS 6 //Water Temp Sensor
#define SDPin 7 //SD Card Select Pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
int cm1, cm2, cm3, dist[readings];
int count = 0;
int water_temp[readings], air_temp[readings], humidity[readings], pressure[readings];
IridiumSBD isbd(Serial1, 8); 
//File myFile;
byte message[payload];

#define BME_SCK 9
#define BME_MISO 12
#define BME_MOSI 11 
#define BME_CS 10
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO,  BME_SCK);

RTC_PCF8523 rtc;

void setup() {
  Serial.begin(19200);
  Serial1.begin(19200);
//  while (!Serial) {
//    delay(1);  // for Leonardo/Micro/Zero
//  }
  delay(4000);
  Serial.println("Serial started"); 
  rtc.begin();
  Serial.println("Clock Started.");
  bme.begin();
  Serial.println("Climate Sensors Started.");
//  pinMode(SDPin, OUTPUT);
//  SD.begin(SDPin);
  isbd.begin();

  //First sensor reading always wrong so lets get it out of the way
  sensors.requestTemperatures();
  bme.readTemperature();
  bme.readHumidity();
  bme.readPressure();
  
  Serial.println("Sat Comms Started");
  Serial.print("Getting current time");
  while(isbd.getSystemTime(isbd_time) != ISBD_SUCCESS){
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  rtc.adjust(DateTime(isbd_time.tm_year + 1900, isbd_time.tm_mon + 1, isbd_time.tm_mday, isbd_time.tm_hour, isbd_time.tm_min, isbd_time.tm_sec));
  Serial.println("Clock Set to ");
  DateTime now = rtc.now();
  Serial.print(now.year(), DEC);
  Serial.print('-');
  Serial.print(now.month() + 1, DEC);
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
  Serial.print(period);
  Serial.print(" Minutes");
  Serial.println();
}
//
//String Unixfile(String U)
//{
//  char Name[12] = {0};
//  for (int i = 0; i < 7; i++)
//  {
//    Name[i] = U[i];
//  }
//  Name[7] = '.';
//  for (int i = 7; i < 10; i++)
//  {
//    Name[i + 1] = U[i];
//  }
//
//  return Name;
//}

void loop() {
  Serial.print("Recording measurement: ");
  Serial.println(count);
  if(count == 0){t = rtc.now().unixtime();}
  cm1 = pulseIn(sonarPin, HIGH)/57.87;
  cm2 = pulseIn(sonarPin, HIGH)/57.87;
  cm3 = pulseIn(sonarPin, HIGH)/57.87;
  dist[count] = (cm1 + cm2 + cm3) / 3;
  sensors.requestTemperatures();
  water_temp[count] = (int) sensors.getTempCByIndex(0)* 100;
  air_temp[count] = (int) bme.readTemperature() * 100;
  humidity[count] = (int) bme.readHumidity() * 100;
  pressure[count] = (int) bme.readPressure();
  
  //String file = Unixfile(String(rtc.now().unixtime()));
  //Serial.println(file);
  //myFile = SD.open(file, FILE_WRITE);
  //myFile.print(dist);
  //myFile.print(water_temp);
  //myFile.print(air_temp);;
  //myFile.print(pressure);
  //myFile.print(humidity);
  //myFile.flush();
  //myFile.close();
  if (count < readings - 1){
    count ++;
  }
  else{
    count = 0;
    //Time (4)
    message[0] = (long) t >> 24;
    message[1] = (long) t >> 16;
    message[2] = (long) t >> 8;
    message[3] = (long) t;
    //Diff (2)
    message[4] = (int) period >> 8;
    message[5] = (int) period;
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
    for(int i = 0; i < payload; i+=2){ 
      char buff[2];
      sprintf(buff, "%02X", message[i]);
      Serial.print(buff);
      sprintf(buff, "%02X", message[i+1]);
      Serial.print(buff);
      //Serial.print(" ");  
      }
    Serial.println();
    isbd.sendSBDBinary(message, payload);
    Serial.println("Sent");
  }
  delay(1000L*period*60);
  //delay(500);
}
