ает#include <Adafruit_NeoPixel.h>

#include <Adafruit_BME280.h> // 209 bytes (273 bytes with init and read)

#include <SoftwareSerial.h> //one SoftwareSerial uses 126 bytes on clean sketch, two - 157 bytes
#include <SPI.h>

#include "SDLog.h"
#include "Adafruit_CCS811.h"

#include <PCD8544.h>
#include "mh_z19.h"
#include "pms5003.h"

#define READ_INTERVAL_MS 10000
#define BT_TIMEOUT_MS 2000
#define BACKLIGHT_PIN 3

//todo: move values to config (EEPROM)
#define LED_TEMP_NUM 0        //number of LED showing temperature
#define LED_TEMP_BLU 18       //lowest temperature, blue color
#define LED_TEMP_GRN 22       //normal temperature, green color
#define LED_TEMP_RED 27       //highest temperature, red color

#define LED_HUM_NUM 1         //number of LED showing humidity
#define LED_HUM_RED 20        //lowest humidity, red color
#define LED_HUM_GRN 50        //normal humidity, green color
#define LED_HUM_BLU 80        //highest humidity, blue color

#define LED_CO2_NUM 2         //number of LED showing CO2
#define LED_CO2_BLU 400       //lowest ppm, blue color
#define LED_CO2_GRN 600       //"normal" ppm, green color
#define LED_CO2_RED 2000      //highest ppm, red color

#define LED_PM_NUM 3         //number of LED showing PM
#define LED_PM_BLU 30        //lowest PM, blue color
#define LED_PM_GRN 100       //"normal" PM, green color
#define LED_PM_RED 300       //highest PM, red color

#define LED_VOC_NUM 4         //number of LED showing VOC
#define LED_VOC_BLU 0        //lowest VOC, blue color
#define LED_VOC_GRN 30       //"normal" VOC, green color
#define LED_VOC_RED 100       //highest VOC, red color

byte buf[32];

SoftwareSerial btSerial(8,9);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(5, 2, NEO_GRB + NEO_KHZ800); 


//clock, data-in, data select, reset, enable
static PCD8544 lcd(7,6,5,A7,A7);

DLog dlog;

uint32_t lastms;//last millis() data was read, used to track intervals between readings
uint32_t long rtimebase;//UNIX time of sensor start, calculates every time STATUS command is received
uint8_t rcmdlen;//length of currently receiving command
uint32_t lastmsbt;//last millis() BT was read, used to track BT timeout

void setup() {
  analogWrite(BACKLIGHT_PIN,255);

  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);
  lcd.print("Initializing");

  Serial.begin(9600); //engaging Serial uses 168 bytes on clean sketch

  btSerial.begin(9600);

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
  analogWrite(BACKLIGHT_PIN,10);

  lastms = millis();

}

void setTimeFromCommand() {
  uint32_t * rtime = (uint32_t*) (buf + 3);
  rtimebase = *rtime - millis()/1000;
#ifdef DEBUG
  Serial.print(F("BT: rtimebase="));Serial.println(rtimebase); 
#endif   
}

//sets LED color based on the value
//lowval - value of blue (red in reverse) color
//val - value to 
void setLEDColorFromValue(int lowval, int midval, int highval, int val, bool reverse, bool flash)
{
  if (val 
}

void loop() {

  btSerial.listen();
  while (millis() < lastms + READ_INTERVAL_MS) {
    if (btSerial.available() > 0)
    {
      buf[rcmdlen++] = btSerial.read();
      if (rcmdlen == 0) { //looking for the beginning of the command
        if (buf[0] != 0xDE) {
          rcmdlen = 0;  
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 1st byte"));Serial.println(buf[0]); 
#endif        
        }
      } else if (rcmdlen == 1) {
        if (buf[1] != 0xAF) {
          rcmdlen = 0;
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 2nd byte"));Serial.println(buf[1]); 
#endif        
        } 
      } 
      if (rcmdlen > 0 && millis() > lastmsbt + BT_TIMEOUT_MS) {
        //timeout receiving command, reverting
#ifdef DEBUG
          Serial.print(F("BT: command timeout at length "));Serial.println(rcmdlen); 
#endif        
          rcmdlen = 0;  
      } else if (rcmdlen == 16) {
        //command fully received
        uint8_t chk = 0;
        for (uint8_t i=2;i<15;i++) {
          chk+=buf[i];
        }
        if (chk!=buf[15]) {
          buf[2] = 0xFF;
#ifdef DEBUG
          Serial.print(F("BT: checksum error "));Serial.print(ckh);Serial.print('/');Serial.print(buf[15]); 
#endif        
          break;
        }
        
        switch (buf[2]) {
          case 0:
            setTimeFromCommand();
            //todo: command implementation
            break;
          case 1:
            setTimeFromCommand();
            //todo: command implementation
            break;
          default:
#ifdef DEBUG
          Serial.print(F("BT: command code error "));Serial.println(buf[2]);
#endif        
            break;
        }
        rcmdlen=0;
      }
      
      lastmsbt = millis();
    }
  }
  
  lastms = millis();
  
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

  pms_read();

  int acc=analogRead(A6);
  
  lcd.clear();
  digitalWrite(LED_BUILTIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print(bmeTemp);lcd.print(F(" C"));
  lcd.setCursor(0, 1);
  lcd.print(bmeHum);lcd.print(F(" %"));
  lcd.setCursor(0, 2);
  lcd.print(ppm);lcd.print(' ');
  lcd.setCursor(0, 3);
  lcd.print(eCO2); lcd.print(F(" e "));lcd.print(ppb);lcd.print(F(" ppb "));
  lcd.setCursor(0, 4);
  lcd.print(pms_pm1_cf1); lcd.print(' '); lcd.print(pms_pm2_5_cf1); lcd.print(' '); lcd.print(pms_pm10_cf1); lcd.print(F(" PM"));

  lcd.setCursor(50,0);
  lcd.print(acc);lcd.print('%');

  //btSerial.print(sec);btSerial.print('\t');btSerial.print(bmeTemp);btSerial.print('\t');btSerial.print(bmeHum);btSerial.print('\t');btSerial.print(ppm);btSerial.print('\t');btSerial.print(ppb);
  
  btSerial.println();
  btSerial.listen();

  dlog.ssecs = millis()/1000;
  dlog.rtime = rtimebase != 0 ? rtimebase + dlog.ssecs : 0;
  dlog.temp = bmeTemp*100;
  dlog.hum = bmeHum*100;
  dlog.co2 = ppm;
  dlog.pm1 = pms_pm1_cf1;
  dlog.pm25 = pms_pm2_5_cf1;
  dlog.pm10 = pms_pm10_cf1;
  dlog.tvoc = ppb;
  
  writeLog(&dlog);
  
  //LEDs are 0 - humidity, 1 - temperature, 2 - CO2, 3 - PM, 4 - VOC
  
  
  /**/
}





