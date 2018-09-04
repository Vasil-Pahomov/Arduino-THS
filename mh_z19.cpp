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
  memset(response, 0, 9);
  mhSerial.readBytes(response, 9);  
  #ifdef DEBUG
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=response[i];
  crc = 255 - crc;
  crc++;
  if ( !(response[0] == 0xFF && response[1] == 0x99 && response[8] == crc) ) {
    Serial.println("MH: setRange CRC error: " + String(crc) + " / "+ String(response[0]) + "," + String(response[1]) + ",...," +String(response[8]));
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
  memset(response, 0, 9);
  mhSerial.readBytes(response, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=response[i];
  crc = 255 - crc;
  crc++;

  if ( !(response[0] == 0xFF && response[1] == 0x86 && response[8] == crc) ) {
    #ifdef DEBUG
    Serial.println("MH: CRC error: " + String(crc) + " / "+ String(response[0]) + "," + String(response[1]) + ",...," +String(response[8]));
    #endif
    return 1;
  } else {
    unsigned int responseHigh = (unsigned int) response[2];
    unsigned int responseLow = (unsigned int) response[3];
    unsigned int ppm = (256*responseHigh) + responseLow;
    #ifdef DEBUG
    Serial.print(F("MH: "));Serial.println(ppm);
    #endif
    return ppm;
  }
}
