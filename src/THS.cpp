#include <Arduino.h>

#define SERIAL_BUFFER_SIZE 32
#define _SS_MAX_RX_BUFF 32 

#include <Adafruit_BME280.h> // 209 bytes (273 bytes with init and read)

#include <NeoSWSerial.h> 
#include <SPI.h>
#include <PCD8544.h>
#include <Adafruit_CCS811.h>
#include <time.h>
#include <uRTCLib.h>

#include "SDLog.h"

#include "mh_z19.h"
#include "pms5003.h"
#include "heiger.h"

//#define DEBUG
#define READ_INTERVAL_MS 20000
#define BT_TIMEOUT_MS 2000
#define BACKLIGHT_PIN 3
//#define USE_RTC

byte buf[32];

NeoSWSerial btSerial(8,9);
Adafruit_BME280 bme;
Adafruit_CCS811 ccs;


//clock, data-in, data select, reset, enable
PCD8544 lcd(7,6,5,4,A7);

DLog dlog;

uint32_t lastms;//last millis() data was read, used to track intervals between readings
#ifndef USE_RTC
uint32_t long rtimebase;//UNIX time of sensor start, calculates every time STATUS command is received
#endif
uint8_t rcmdlen;//length of currently receiving command
uint32_t lastmsbt;//last millis() BT was read, used to track BT timeout
int batcnt;//counter of battery measurements during "idle" cycle
long batacc;//"accumulator" of battery measurements

tm timestruct;

#ifdef USE_RTC
uRTCLib rtc(0x68);
#endif

void setup() {
  lcd.begin(84, 48);
  lcd.setCursor(0, 0); lcd.print('#');

  pinMode(BACKLIGHT_PIN, OUTPUT);
  digitalWrite(BACKLIGHT_PIN,HIGH);

  // PCD8544-compatible displays may have a different resolution...

  Serial.begin(9600); //engaging Serial uses 168 bytes on clean sketch

  //todo: check for MH setup error
  mh_setup();
  lcd.print('#');

  pinMode(LED_BUILTIN, OUTPUT);
  analogReference(INTERNAL);
  pinMode(A6, INPUT);


  if (!bme.begin(0x76)) {
    //lcd.setCursor(0, 0);
    //lcd.print(F("BMP Error!"));
    //digitalWrite(LED_BUILTIN, HIGH);
    //delay(10000);
    //lcd.clear();
  }

  lcd.print('#');

  if(!ccs.begin()){
    //lcd.setCursor(0, 0);
    //lcd.print(F("CCS Error!"));
    //delay(10000);
    //lcd.clear();
  }
  ccs.setDriveMode(CCS811_DRIVE_MODE_10SEC);

  lcd.print('#');
  //todo: check for PMS setup error
  pms_setup();
  lcd.print('#');

#ifdef USE_RTC
  URTCLIB_WIRE.begin();
#endif

  heiger_setup();
  lcd.print('#');

  lastms = 0;
  btSerial.begin(9600);
  btSerial.println("ST");

  lcd.print('#');
}

void commandSync() {
  uint32_t * rtime = (uint32_t*) (buf + 3);
#ifdef USE_RTC
  gmtime_r(rtime, &timestruct);
  rtc.set(timestruct.tm_sec, timestruct.tm_min, timestruct.tm_hour, 
      timestruct.tm_wday, timestruct.tm_mday, timestruct.tm_mon, timestruct.tm_year-130);
#else
  rtimebase = *rtime - millis()/1000;
#endif
#ifdef DEBUG
  Serial.print(F("Got time:"));Serial.println(*rtime); 
#endif   
}

void loop() {

  btSerial.listen();
  batcnt = 0;
  batacc = 0;
  while (lastms != 0 && (millis() < lastms + READ_INTERVAL_MS)) {
    while (btSerial.available() > 0)
    {
      buf[rcmdlen++] = btSerial.read();
      //Serial.println(buf[rcmdlen-1]);
#ifdef DEBUG
      Serial.print(F("B("));Serial.print(rcmdlen-1);Serial.print(F(")="));Serial.print(buf[rcmdlen-1]); Serial.print(':');Serial.println((char)buf[rcmdlen-1]); 
#endif        
      if (rcmdlen == 1) { //looking for the beginning of the command
        if (buf[0] != 0xDE) {
          rcmdlen = 0;  
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 1st byte: "));Serial.println(buf[0]); 
#endif        
          continue;
        }
      } else if (rcmdlen == 2) {
        if (buf[1] != 0xAF) {
          rcmdlen = 0;
#ifdef DEBUG
          Serial.print(F("BT: invalid sign 2nd byte: "));Serial.println(buf[1]); 
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
              lastms = millis() - READ_INTERVAL_MS;
            }
            break;
          case 1:
            if (rcmdlen>=11) {
#ifdef USE_RTC
                sdTransmitData();
#else
              if (rtimebase !=0) {
                sdTransmitData();
                lastms = millis() - READ_INTERVAL_MS;
              } else
              {
#ifdef DEBUG
                Serial.print(F("No sync - no transmission"));
#endif        
              }
#endif              
              rcmdlen = 0;
            }
            break;
          case 2:
            if (rcmdlen>=3) {
              sdReset();
              rcmdlen = 0;
              lastms = millis() - READ_INTERVAL_MS;
          }
            break;
          case 3:
            if (rcmdlen>=3) {
              mh_calibrate();
              rcmdlen = 0;
              lastms = millis() - READ_INTERVAL_MS;
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
    batacc += analogRead(A6);
    batcnt++;
    
    delay(100);
  }

  lastms = millis();

  digitalWrite(LED_BUILTIN, HIGH);
  float bmeTemp = bme.readTemperature();
  float bmeHum = bme.readHumidity();

  int ppb = -3;//data not available
  ccs.setEnvironmentalData((int)bmeHum, bmeTemp);
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
  unsigned int ppm = mh_getPPM();
  pms_read();

  float rad = heiger_getRadiation();
  //950 - 100%
  //700 - 0% (на 5 минут)
  int acc;
  if (batcnt == 0) {
    acc = 255;
  } else {
    acc=(batacc/batcnt-700)*2/5;
    if (acc > 100) acc = 100;
    if (acc < 0) acc = 0;
  }
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(bmeTemp);lcd.print('C');
  lcd.setCursor(42, 0);
  lcd.print(bmeHum);lcd.print('%');
  lcd.setCursor(0, 1);
  lcd.print(ppm);lcd.print('@');
  lcd.setCursor(42, 1);
  lcd.print(ppb);lcd.print('#');
  lcd.setCursor(0, 2);
  lcd.print(pms_pm1_cf1); lcd.print(' '); lcd.print(pms_pm2_5_cf1); lcd.print(' '); lcd.print(pms_pm10_cf1); lcd.print('*');
  lcd.setCursor(42,3); lcd.print(rad); lcd.print('R');
#ifdef USE_RTC
    rtc.refresh();
    timestruct.tm_sec = rtc.second();
    timestruct.tm_min = rtc.minute();
    timestruct.tm_hour = rtc.hour();
    timestruct.tm_wday = rtc.dayOfWeek();
    timestruct.tm_mday = rtc.day();
    timestruct.tm_mon = rtc.month();
    timestruct.tm_year = 100 + rtc.year();
    time_t ttime = mk_gmtime(&timestruct);
    dlog.rtime = ttime;
#else
  if (rtimebase != 0) {
    time_t ttime = rtimebase + millis()/1000 - UNIX_OFFSET;
    gmtime_r(&ttime, &timestruct);
    dlog.rtime = rtimebase != 0 ? rtimebase + dlog.ssecs : 0;
#endif
    isotime_r(&timestruct, (char*)buf);
    buf[10]=0;//separate date from time
    lcd.setCursor(0, 4);
    lcd.print((char*)(buf+11));//time
    lcd.setCursor(0, 5);
    lcd.print((char*)buf);//date
#ifndef USE_RTC
  }
#endif


  if (acc != 255) {
    lcd.setCursor(0, 3);
    lcd.print('B');lcd.print(acc);lcd.print('%');
  }
  dlog.ssecs = millis()/1000;
  dlog.data.temp = bmeTemp*100;
  dlog.data.hum = bmeHum*100;
  dlog.data.co2 = ppm;
  dlog.data.pm1 = pms_pm1_cf1;
  dlog.data.pm25 = pms_pm2_5_cf1;
  dlog.data.pm10 = pms_pm10_cf1;
  dlog.data.tvoc = ppb;
  dlog.data.rad = rad*1000;
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
  digitalWrite(LED_BUILTIN, LOW);
}
