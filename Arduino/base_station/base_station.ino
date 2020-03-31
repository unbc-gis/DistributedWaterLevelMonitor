#include <avr/sleep.h>
#include "RTClib.h"

int wakePin = 4;
const int sonarPin = 5;
long pulse, cm1, cm2, cm3;

RTC_PCF8523 rtc;

void setup() {

  while (!Serial) {
    delay(1);  // for Leonardo/Micro/Zero
  }
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  
  Serial.begin(57600);
  pinMode(wakePin, INPUT);
  pinMode(sonarPin, INPUT);
  //attachInterrupt(0, wakeUpNow, LOW);
  // following line sets the RTC to the date & time this sketch was compiled
  // rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
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
  delay(1000);
  pulse = pulseIn(sonarPin, HIGH);
  cm1 = pulse/57.87;
  cm2 = pulse/57.87;
  cm3 = pulse/57.87;
  Serial.print("Distance: ");
  Serial.print((cm1 + cm2 + cm3) / 3);
  Serial.print("cm at ");
  DateTime now = rtc.now();
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
// sleepNow();
}
