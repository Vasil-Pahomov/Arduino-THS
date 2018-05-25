

#include <LiquidCrystal.h>
#include <SoftwareSerial.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_BME280.h>
#include "TempDS.h"
#include "mh_z19.h"

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
LiquidCrystal lcd(9, 8, 7, 6, 5, 4);//rs, en, d4, d5, d6, d7

unsigned int minPPM = 5000;
unsigned int maxPPM = 0;

SoftwareSerial btSerial(10,11);

Adafruit_BMP280 bme;


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  btSerial.begin(9600);

  Serial.begin(9600);
  mh_setup();

  pinMode(LED_BUILTIN, OUTPUT);

  /*bme.settings.commInterface = I2C_MODE;
  bme.settings.I2CAddress = 0x76; //Адрес датчика, в моём случае не стандартный
  bme.settings.runMode=3;
  bme.settings.tStandby=5;
  bme.settings.filter=0;
  bme.settings.tempOverSample = 1;
  bme.settings.humidOverSample = 1;*/
  if (!bme.begin(0x76)) {
    lcd.print(F("Error initializing BMP"));
  }
}

void loop() {
  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(10);
  digitalWrite(LED_BUILTIN, LOW);
  lcd.setCursor(0,0);
  lcd.print(getTemperature());
  lcd.print(F("/"));
  lcd.print(bme.readTemperature());
  lcd.print(F("\xDF"));lcd.print(F("C  "));
  lcd.setCursor(0,1);
  unsigned int ppm = mh_getPPM();
  unsigned int sec = millis()/1000;
  
  lcd.print(ppm);
  if (sec < 1*60) {
    lcd.print(F("?  "));
  } else {
    if (ppm > maxPPM) maxPPM=ppm;
    if (ppm < minPPM) minPPM=ppm;
    lcd.print(F(" ("));
    lcd.print(minPPM);
    lcd.print(F("-"));
    lcd.print(maxPPM);
    lcd.print(F(")  "));
  }
  
  btSerial.print(sec);btSerial.print(F("\t"));btSerial.println(ppm);
  btSerial.listen();
    
  delay(10000);
  if (btSerial.available()> 0)
  {
    byte incomingByte = btSerial.read(); // считываем байт
    if(incomingByte == '0') {
      minPPM = 5000;
      btSerial.println(F("min reset"));
    }
    if(incomingByte == '1') {
      maxPPM = 0;
      btSerial.println(F("max reset"));
    }
  }
  
}




