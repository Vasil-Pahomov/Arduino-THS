#include <SoftwareSerial.h>
#include <Arduino.h>
#include "mh_z19.h"
SoftwareSerial mySerial(A0, A1); // A0 - к TX сенсора, A1 - к RX

byte cmd[9] = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79}; 
unsigned char response[9];

void mh_setup() {
  mySerial.begin(9600);
}

unsigned int mh_getPPM() 
{
  mySerial.write(cmd, 9);
  memset(response, 0, 9);
  mySerial.readBytes(response, 9);
  int i;
  byte crc = 0;
  for (i = 1; i < 8; i++) crc+=response[i];
  crc = 255 - crc;
  crc++;

  if ( !(response[0] == 0xFF && response[1] == 0x86 && response[8] == crc) ) {
    //#ifdef DEBUG
    Serial.println("MH: CRC error: " + String(crc) + " / "+ String(response[8]));
    //#endif
    return 0;
  } else {
    unsigned int responseHigh = (unsigned int) response[2];
    unsigned int responseLow = (unsigned int) response[3];
    unsigned int ppm = (256*responseHigh) + responseLow;
    //#ifdef DEBUG
    Serial.print(F("MH: "));Serial.println(ppm);
    //#endif
    return ppm;
  }
}
