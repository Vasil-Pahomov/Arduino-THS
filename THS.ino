#include <Adafruit_BME280.h> // 209 bytes (273 bytes with init and read)

#include <SoftwareSerial.h> //one SoftwareSerial uses 126 bytes on clean sketch, two - 157 bytes
#include <SPI.h>

#include "SDLog.h"
#include "Adafruit_CCS811.h"

#include <PCD8544.h>
#include "mh_z19.h"
#include "pms5003.h"

byte buf[32];

unsigned int minPPM = 5000;
unsigned int maxPPM = 0;
int errors, success;

SoftwareSerial btSerial(8,9);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;

//clock, data-in, data select, reset, enable
static PCD8544 lcd(7,6,5,A7,A7);

DLog dlog;

void setup() {
  Serial.begin(9600); //engaging Serial uses 168 bytes on clean sketch

  Serial.println(sizeof(DLog));


  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);
  lcd.print("Initializing");

  analogWrite(3,255);
  btSerial.begin(9600);
/*
  //10 is CS input of card board
  if (!card.init(SPI_HALF_SPEED, 10)) {
    lcd.setCursor(0, 0);
    lcd.print(F("SD card error!"));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10000);
    lcd.clear();
  } else if (!volume.init(card)) {
    lcd.setCursor(0, 0);
    lcd.print(F("SD volume error!"));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10000);
    lcd.clear();
  }
*/
  //todo: check for MH setup error
  mh_setup();

  pinMode(LED_BUILTIN, OUTPUT);
  analogReference(INTERNAL);
  pinMode(A6, INPUT);

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

  //todo: check for PMS setup error
  pms_setup();
  analogWrite(3,10);

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

  if (pms_error) errors++; else success++;

  int acc=analogRead(A6);
  
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

  lcd.setCursor(0, 5);
  lcd.print('E'); lcd.print(errors); lcd.print('/'); lcd.print(success);

  lcd.setCursor(50,0);
  lcd.print(acc);lcd.print('%');
/*
  Serial.print(pms_pm1_cf1);Serial.print('\t'); 
  Serial.print(pms_pm2_5_cf1);Serial.print('\t');
  Serial.print(pms_pm10_cf1);Serial.print('\t');
  Serial.print(pms_pm1_ae);Serial.print('\t');
  Serial.print(pms_pm2_5_ae);Serial.print('\t');
  Serial.print(pms_pm10_ae);Serial.print('\t');
  Serial.print(pms_num_0_3);Serial.print('\t');
  Serial.print(pms_num_0_5);Serial.print('\t');
  Serial.print(pms_num_1);Serial.print('\t');
  Serial.print(pms_num_2_5);Serial.print('\t');
  Serial.print(pms_num_5_0);Serial.print('\t');
  Serial.println(pms_num_10);
/**/ 
  btSerial.print(sec);btSerial.print('\t');btSerial.print(bmeTemp);btSerial.print('\t');btSerial.print(bmeHum);btSerial.print('\t');btSerial.print(ppm);btSerial.print('\t');btSerial.print(ppb);
  btSerial.println();
  btSerial.listen();

/*
  dlog.ssecs=1;
  dlog.temp=2;
  dlog.hum=3;
  dlog.co2=4;
  dlog.pm1=5;
  dlog.pm25=6;
  dlog.pm10=7;
  dlog.tvoc=8;
*/
  dlog.ssecs = millis()/1000;
  dlog.temp = bmeTemp*100;
  dlog.hum = bmeHum*100;
  dlog.co2 = ppm;
  dlog.pm1 = pms_pm1_cf1;
  dlog.pm25 = pms_pm2_5_cf1;
  dlog.pm10 = pms_pm10_cf1;
  dlog.tvoc = ppb;
  
  writeLog(&dlog);
  
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




