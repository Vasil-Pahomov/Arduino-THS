#include <Adafruit_BME280.h> // 209 bytes (273 bytes with init and read)

#include <SoftwareSerial.h> //one SoftwareSerial uses 126 bytes on clean sketch, two - 157 bytes
#include <SPI.h>
#include <SD.h>

#include "Adafruit_CCS811.h"

#include <PCD8544.h>
#include "mh_z19.h"
#include "pms5003.h"

byte response[32];

unsigned int minPPM = 5000;
unsigned int maxPPM = 0;

SoftwareSerial btSerial(8,9);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

//clock, data-in, data select, reset, enable
static PCD8544 lcd(7,6,5,3,4);

void setup() {
  Serial.begin(9600); //engaging Serial uses 168 bytes on clean sketch

  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);
  lcd.print("Initializing");
  
  btSerial.begin(9600);

  mh_setup();

  pinMode(LED_BUILTIN, OUTPUT);

  if (!bme.begin(0x76)) {
    btSerial.print(F("BMP Error!"));
    lcd.setCursor(0, 0);
    lcd.print(F("BMP Error!"));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10000);
    lcd.clear();
  }

  if(!ccs.begin()){
    btSerial.print(F("CCS Error!"));
    lcd.setCursor(0, 0);
    lcd.print(F("CCS Error!"));
    delay(10000);
    lcd.clear();
  }

  ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);

  pms_setup();
}

void loop() {
  
  digitalWrite(LED_BUILTIN, HIGH);

  float bmeTemp = bme.readTemperature();
  float bmeHum = bme.readHumidity();

  unsigned int ppm = mh_getPPM();
  unsigned int sec = millis()/1000;
  
  ccs.setEnvironmentalData((int)bmeHum, bmeTemp);
  int ppb = -3;//data not available
  int eCO2 = -3;
  if(ccs.available()){
    if (!ccs.readData()) {
      eCO2 = ccs.geteCO2();
      if (eCO2 != 0) {
        ppb = ccs.getTVOC();
      } else {
        ppb = -1;//eCO2=0, warming up
      }
    } else 
    {
      ppb = -2;//data not read
    }
      
  }

  if (sec < 1*60) {

  } else {
    if (ppm > maxPPM) maxPPM=ppm;
    if (ppm < minPPM) minPPM=ppm;
  }

  pms_read();
  
  lcd.clear();
  digitalWrite(LED_BUILTIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print(bmeTemp);lcd.print(F(" C"));
  lcd.setCursor(0, 1);
  lcd.print(bmeHum);lcd.print(F(" %"));
  lcd.setCursor(0, 2);
  lcd.print(ppm);lcd.print(' ');lcd.print(minPPM);lcd.print('-');lcd.print(maxPPM);
  lcd.setCursor(0, 3);
  lcd.print(eCO2); lcd.print(F(" e "));lcd.print(ppb);lcd.print(F(" ppb "));
  lcd.setCursor(0, 4);
  lcd.print(pms_pm1_cf1); lcd.print(' '); lcd.print(pms_pm2_5_cf1); lcd.print(' '); lcd.print(pms_pm10_cf1); lcd.print(F(" PM"));
  
   
  btSerial.print(sec);btSerial.print('\t');btSerial.print(bmeTemp);btSerial.print('\t');btSerial.print(bmeHum);btSerial.print('\t');btSerial.print(ppm);btSerial.print('\t');btSerial.print(ppb);
  btSerial.println();
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

  
  /**/
}




