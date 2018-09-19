#include <SoftwareSerial.h>
#include <Arduino.h>
#include "mh_z19.h"
#include "global.h"
//#define DEBUG
SoftwareSerial mhSerial(A0, A1); // A0 - к TX сенсора, A1 - к RX

byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 

void mh_setup() {
  mhSerial.begin(9600);
  byte icmd[9] = {0xFF, 0x01, 0x99, 0x00, 0x00, 0x00, 0x13, 0x88, 0xCB};

  mhSerial.write(icmd, 9);
  memset(buf, 0, 9);
  mhSerial.readBytes(buf, 9);  
  #ifdef DEBUG
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=buf[i];
  crc = 255 - crc;
  crc++;
  if ( !(buf[0] == 0xFF && buf[1] == 0x99 && buf[8] == crc) ) {
    Serial.println("MH: setRange CRC error: " + String(crc) + " / "+ String(buf[0]) + "," + String(buf[1]) + ",...," +String(buf[8]));
  } else
  {
    Serial.println("MH: setRange OK");
  }
  #endif    
}

unsigned int mh_getPPM() 
{  
  mhSerial.listen();
  mhSerial.write(cmd, 9);
  memset(buf, 0, 9);
  mhSerial.readBytes(buf, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=buf[i];
  crc = 255 - crc;
  crc++;

  if ( !(buf[0] == 0xFF && buf[1] == 0x86 && buf[8] == crc) ) {
    #ifdef DEBUG
    Serial.println("MH: CRC error: " + String(crc) + " / "+ String(buf[0]) + "," + String(buf[1]) + ",...," +String(buf[8]));
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
