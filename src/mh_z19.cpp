#include <NeoSWSerial.h>
#include <Arduino.h>
#include "mh_z19.h"
#include "global.h"
//#define DEBUG
NeoSWSerial mhSerial(A3, A2); // A3 - к TX сенсора, A2 - к RX

unsigned int mh_ppmprev;

void mh_setup() {
  mhSerial.begin(9600);
  mhSerial.ignore();
  byte icmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

  mhSerial.write(icmd, 9);
  mhSerial.listen();
  memset(buf, 0, 9);
  mhSerial.readBytes(buf, 9);  
  mhSerial.ignore();
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

int mh_getPPM() 
{  
  byte get_cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
    
  mhSerial.write(get_cmd, 9);
  memset(buf, 0, 9);
  mhSerial.listen();
  mhSerial.readBytes(buf, 9);
  mhSerial.ignore();
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=buf[i];
  crc = 255 - crc;
  crc++;

  unsigned int bufHigh = (unsigned int) buf[2];
  unsigned int bufLow = (unsigned int) buf[3];
  int ppm = (256*bufHigh) + bufLow;
  #ifdef DEBUG
  Serial.print(F("MH: "));Serial.print(ppm);
  #endif

  if ( !(buf[0] == 0xFF && buf[1] == 0x86 && buf[8] == crc) ) {
    #ifdef DEBUG
    Serial.print(F(" CRC error: "));
    Serial.print(crc); Serial.print('/');
    for (int i=0;i<=8;i++) { Serial.print(buf[i]);Serial.print(','); }
    #endif
    int ppmdiff = abs(ppm-mh_ppmprev);
    //sometimes sensor reports zero CRC with readings that seems correct
    if (ppm >= 400 && ppm <= 5000 && ppmdiff <= 500 && buf[8] == 0) {
      #ifdef DEBUG
      Serial.println(F(" assuming OK"));
      #endif
      mh_ppmprev = ppm;
      return ppm;
    } else {
      #ifdef DEBUG
      Serial.print(F(" wrong, diff="));Serial.println(ppmdiff);
      #endif
    }
  } else {
    #ifdef DEBUG
    Serial.println(F(" OK"));
    #endif
    mh_ppmprev = ppm;
    return ppm;
  }
  #ifdef DEBUG
  Serial.println(F("MH fail"));
  #endif
  return 1;
}

void mh_calibrate() {
  byte cal_cmd[9] = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78}; 
  mhSerial.write(cal_cmd, 9);
  delay(500);
  mhSerial.listen();
  mhSerial.readBytes(buf, 9);
}
