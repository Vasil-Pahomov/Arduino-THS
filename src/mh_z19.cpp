#include <SoftwareSerial.h>
#include <Arduino.h>
#include "mh_z19.h"
#include "global.h"
//#define DEBUG
SoftwareSerial mhSerial(A3, A2); // A0 - Рє TX СЃРµРЅСЃРѕСЂР°, A1 - Рє RX


void mh_setup() {
  mhSerial.begin(9600);
  byte icmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

  mhSerial.write(icmd, 9);
  mhSerial.listen();
  memset(buf, 0, 9);
  mhSerial.readBytes(buf, 9);  
  #ifdef DEBUG
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=buf[i];
  crc = 255 - crc;
  crc++;
  if ( !(buf[0] == 0xFF && buf[1] == 0x99 && buf[8] == crc) ) {
    Serial.print(F("MH: setRange CRC error "));
  } else
  {
    Serial.print(F("MH: setRange OK: "));
  }
  Serial.print(crc); Serial.print('/');Serial.print(buf[0]);Serial.print(','); Serial.print(buf[1]); Serial.print('-');Serial.println(buf[8]);
  #endif    
}

unsigned int mh_getPPM() 
{  
  byte get_cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
  mhSerial.write(get_cmd, 9);
  memset(buf, 0, 9);
  mhSerial.listen();
  mhSerial.readBytes(buf, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=buf[i];
  crc = 255 - crc;
  crc++;

  if ( !(buf[0] == 0xFF && buf[1] == 0x86 && buf[8] == crc) ) {
    #ifdef DEBUG
    Serial.println(F("MH: CRC error: "));
    Serial.print(crc); Serial.print('/');Serial.print(buf[0]);Serial.print(','); Serial.print(buf[1]); Serial.print('-');Serial.println(buf[8]);
    #endif
    return 1;
  } else {
    unsigned int bufHigh = (unsigned int) buf[2];
    unsigned int bufLow = (unsigned int) buf[3];
    unsigned int ppm = (256*bufHigh) + bufLow;
    #ifdef DEBUG
    Serial.print(F("MH: "));Serial.println(ppm);
    #endif
    return ppm;
  }
}

void mh_calibrate() {
  byte cal_cmd[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78}; 
  mhSerial.write(cal_cmd, 9);
  delay(500);
  mhSerial.listen();
  mhSerial.readBytes(buf, 9);
}
