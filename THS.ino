//#include <Adafruit_NeoPixel.h>

#include <Adafruit_BME280.h> // 209 bytes (273 bytes with init and read)

#include <SoftwareSerial.h> //one SoftwareSerial uses 126 bytes on clean sketch, two - 157 bytes
#include <SPI.h>
#include <PCD8544.h>
#include <Adafruit_CCS811.h>
#include <time.h>

#include "SDLog.h"

#include "mh_z19.h"
#include "pms5003.h"

#define DEBUG
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
//Adafruit_NeoPixel pixels = Adafruit_NeoPixel(5, 2, NEO_GRB + NEO_KHZ800); 


//clock, data-in, data select, reset, enable
static PCD8544 lcd(7,6,5,4,A7);

/*
static const byte DEGREES_CHAR = 1;
static const byte degrees_glyph[] = { 0x00, 0x07, 0x05, 0x07, 0x00 };
static const byte CO2_CHAR = 2;
static const byte co2_glyph[] = { 0x06, 0x3e, 0x3c, 0x7c, 0x60 };
static const byte TVOC_CHAR = 3;
static const byte tvoc_glyph[] = { 0x18, 0x24, 0x42, 0x24, 0x18 };
static const byte PM_CHAR = 4;
static const byte pm_glyph[] = { 0x0a, 0x20, 0x48, 0x02, 0x24 };
*/

DLog dlog;

uint32_t lastms;//last millis() data was read, used to track intervals between readings
uint32_t long rtimebase;//UNIX time of sensor start, calculates every time STATUS command is received
uint8_t rcmdlen;//length of currently receiving command
uint32_t lastmsbt;//last millis() BT was read, used to track BT timeout

tm timestruct;


void setup() {
  analogWrite(BACKLIGHT_PIN,255);

  // PCD8544-compatible displays may have a different resolution...
  lcd.begin(84, 48);
  lcd.setCursor(0, 0);

/*
  lcd.createChar(DEGREES_CHAR, degrees_glyph);
  lcd.createChar(CO2_CHAR, co2_glyph);
  lcd.createChar(TVOC_CHAR, tvoc_glyph);
  lcd.createChar(PM_CHAR, pm_glyph);
  lcd.print(F("Init: serial"));
*/    

  Serial.begin(9600); //engaging Serial uses 168 bytes on clean sketch

  
  lcd.setCursor(0, 1);
  lcd.print(F("Init: CO2"));

  //todo: check for MH setup error
  mh_setup();

  pinMode(LED_BUILTIN, OUTPUT);
  analogReference(INTERNAL);
  pinMode(A6, INPUT);

  lcd.setCursor(0, 2);
  lcd.print(F("Init: temp&hum"));

  if (!bme.begin(0x76)) {
    lcd.setCursor(0, 0);
    lcd.print(F("BMP Error!"));
    digitalWrite(LED_BUILTIN, HIGH);
    delay(10000);
    lcd.clear();
  }

  lcd.setCursor(0, 3);
  lcd.print(F("Init: VOC"));

  if(!ccs.begin()){
    lcd.setCursor(0, 0);
    lcd.print(F("CCS Error!"));
    delay(10000);
    lcd.clear();
  }

  ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);

  lcd.setCursor(0, 4);
  lcd.print(F("Init: PM"));
  //todo: check for PMS setup error
  pms_setup();
  analogWrite(BACKLIGHT_PIN,10);
  lcd.setCursor(0, 5);
  lcd.print(F("Init done"));

  lastms = 0;
  btSerial.begin(9600);

}

void commandSync() {
  uint32_t * rtime = (uint32_t*) (buf + 3);
  rtimebase = *rtime - millis()/1000;
#ifdef DEBUG
  Serial.print(F("Got time:"));Serial.println(*rtime); 
#endif   
}

//sets LED color based on the value
//lowval - value of blue (red in reverse) color
//val - value to 
void setLEDColorFromValue(int lowval, int midval, int highval, int val, bool reverse, bool flash)
{
}

void loop() {
  btSerial.listen();
  while (lastms != 0 && (millis() < lastms + READ_INTERVAL_MS)) {
    while (btSerial.available() > 0)
    {
      buf[rcmdlen++] = btSerial.read();
#ifdef DEBUG
      Serial.print(F("B("));Serial.print(rcmdlen-1);Serial.print(F(")="));Serial.print(buf[rcmdlen-1]); Serial.print(':');Serial.println((char)buf[rcmdlen-1]); 
#endif        
      if (rcmdlen == 1) { //looking for the beginning of the command
        if (buf[0] != 0xDE) {
          rcmdlen = 0;  
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 1st byte"));Serial.println(buf[0]); 
#endif        
          continue;
        }
      } else if (rcmdlen == 2) {
        if (buf[1] != 0xAF) {
          rcmdlen = 0;
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 2nd byte"));Serial.println(buf[1]); 
#endif        
          continue;
        } 
      } else {
      //command received (may be partially)
        
        switch (buf[2]) {
          case 0:
            if (rcmdlen>=7) {
              commandSync();
              rcmdlen = 0;
            }
            break;
          case 1:
            if (rcmdlen>=11) {
              sdTransmitData(&btSerial);
              rcmdlen = 0;
            }
            break;
          case 2:
            if (rcmdlen>=3) {
              sdReset();
              rcmdlen = 0;
            }
            break;
          default:
#ifdef DEBUG
            Serial.print(F("BT: command code error "));Serial.println(buf[2]);
#endif        
            rcmdlen=0;
          break;
        }
      }
   
      
      lastmsbt = millis();
    }
    if (rcmdlen > 1 && millis() > lastmsbt + BT_TIMEOUT_MS) {
      //timeout receiving command, reverting
#ifdef DEBUG
        Serial.print(F("BT: command timeout at length "));Serial.println(rcmdlen); 
#endif        
        rcmdlen = 0;  
    }    
  /**/
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

  
  int acc=(analogRead(A6)-700)/3;

  lcd.clear();
  digitalWrite(LED_BUILTIN, LOW);

  lcd.setCursor(0, 0);
  lcd.print(bmeTemp);lcd.print((char)1);
  lcd.setCursor(42, 0);
  lcd.print(bmeHum);lcd.print('%');
  lcd.setCursor(0, 1);
  lcd.print(ppm);lcd.print((char)2);
  lcd.setCursor(42, 1);
  lcd.print(ppb);lcd.print((char)3);
  lcd.setCursor(0, 2);
  lcd.print(pms_pm1_cf1); lcd.print(' '); lcd.print(pms_pm2_5_cf1); lcd.print(' '); lcd.print(pms_pm10_cf1); lcd.print((char)4);


  if (rtimebase != 0) {
    time_t ttime = rtimebase + millis()/1000 - UNIX_OFFSET;
    gmtime_r(&ttime, &timestruct);
    isotime_r(&timestruct, (char*)buf);
    buf[10]=0;//separate date from time
    lcd.setCursor(0, 3);
    lcd.print((char*)(buf+11));//time
    lcd.setCursor(0, 4);
    lcd.print((char*)buf);//date
  }

  lcd.setCursor(0, 5);
  lcd.print('B');lcd.print(acc);


  dlog.ssecs = millis()/1000;
  dlog.rtime = rtimebase != 0 ? rtimebase + dlog.ssecs : 0;
  dlog.data.temp = bmeTemp*100;
  dlog.data.hum = bmeHum*100;
  dlog.data.co2 = ppm;
  dlog.data.pm1 = pms_pm1_cf1;
  dlog.data.pm25 = pms_pm2_5_cf1;
  dlog.data.pm10 = pms_pm10_cf1;
  dlog.data.tvoc = ppb;
  
  writeLog(&dlog);

  //writing binary data to the bluetooth
  
  buf[0] = 0xDE; //signature
  buf[1] = 0xAF;
  buf[2] = 0;    //command code
  buf[3] = 0;    //status
  buf[4] = acc;   //battery
  btSerial.write(buf,5);
  delay(50);  //not sure why these delays are needed... but there's sometimes garbage in BT channel otherwise
  btSerial.write((byte*)&lastLogIdx, 4); //log index
  delay(50);  
  btSerial.write((byte*)&dlog.data, sizeof(Data)); //data
  
  /**/
}
