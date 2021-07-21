#include <SoftwareSerial.h>
#include <Arduino.h>
#include "pms5003.h"
#include "global.h"
#define DEBUG
SoftwareSerial pmsSerial(A1, A0); // A1- к TX сенсора, A0 - к RX

unsigned int 
  pms_pm1_cf1, 
  pms_pm2_5_cf1, 
  pms_pm10_cf1, 
  pms_pm1_ae, 
  pms_pm2_5_ae,
  pms_pm10_ae,
  pms_num_0_3,
  pms_num_0_5,
  pms_num_1,
  pms_num_2_5,
  pms_num_5_0,
  pms_num_10;
  
bool pms_error;
  
#define PMS_CMD_READ 0xe2
#define PMS_CMD_MODE 0xe1
#define PMS_CMD_SLEEP 0xe4

#define intbuf(highidx, lowidx) (buf[highidx] << 8) + (buf[lowidx])

void pms_setup() {
  pmsSerial.begin(9600);
  pms_setactive(false);
}

bool pms_read()
{
  pms_cmd(PMS_CMD_READ, 0, 0);
  memset(buf,0,32);
  pmsSerial.listen();
  pmsSerial.setTimeout(2000);
  pmsSerial.readBytes(buf, 32);
  pms_error = false;

  if (buf[0] != 0x42 || buf[1] != 0x4d) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: signature incorrect, expected 16973, found:"));Serial.println(intbuf(0,1));
#endif
  }
  
  if (buf[2] != 0 || buf[3] != 28) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: frame length is incorrect, expected 28, found:"));Serial.println(intbuf(2,3));
#endif
  }

  unsigned int checksum = 0;
  for (byte i=0;i<30;i++)
  {
    checksum += buf[i];
  }
  if (buf[30] != (checksum >> 8) || buf[31] != (checksum & 255)) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: checksum is incorrect, expected")); Serial.print(checksum); Serial.print(F(", found:"));Serial.println(intbuf(30,31));     
#endif
  }

#ifdef DEBUG
  if (pms_error) {
    Serial.print(F("PMS: resp "));
    for (byte i = 0; i < 32; i++) {
      Serial.print(buf[i]);Serial.print(' ');
    }
    Serial.println();    
  }
#endif

  if (!pms_error) {
    pms_pm1_cf1 = intbuf(4,5);
    pms_pm2_5_cf1 = intbuf(6,7);
    pms_pm10_cf1 = intbuf(8,9);
    pms_pm1_ae = intbuf(10,11);
    pms_pm2_5_ae = intbuf(12,13);
    pms_pm10_ae = intbuf(14,15);
    pms_num_0_3 = intbuf(16,17);
    pms_num_0_5 = intbuf(18,19);
    pms_num_1 = intbuf(20,21);
    pms_num_2_5 = intbuf(22,23);
    pms_num_5_0 = intbuf(24,25);
    pms_num_10 = intbuf(26,27);
  }
}

void pms_cmd(byte command, byte datah, byte datal)
{
  byte icmd[7] = {0x42, 0x4d, command, datah, datal, 0x00, 0x00};
  int vb = icmd[0] + icmd[1] + icmd[2] + icmd[3] + icmd[4];
  icmd[5] = (byte) (vb / 256);
  icmd[5] = (byte) (vb & 255);
  pmsSerial.write(icmd, 7);
#ifdef DEBUG
  Serial.print(F("PMS: cmd "));
  for (byte i = 0; i < 7; i++) {
    Serial.print(icmd[i]);Serial.print(' ');
  }
  Serial.println();
#endif
}

void pms_readcmdbuf()
{
  //though not documented, on some commands sensor sends the following buf:
  //0x42 0x4d - signature bytes
  //0x00 0x04 - always 4 (a kind of frame length?)
  //<command byte> <data byte> <data byte> - the same as sent in command
  //<verify_high_byte> <verify_low_byte> - sum of all bytes except signature (thus it equals to corresponding sum in the command plus 4)
  pmsSerial.listen();
  pmsSerial.readBytes(buf, 9);
#ifdef DEBUG
  Serial.print(F("PMS: cmdresp="));Serial.print(pmsSerial.readBytes(buf, 9));
  Serial.print(':');
  for (byte i = 0; i < 32; i++) {
    Serial.print(buf[i]);Serial.print(' ');
  }
  Serial.println();    
#endif
  //todo: add buf check    
}
void pms_setactive(bool isActive)
{
  pms_cmd(PMS_CMD_MODE, 0, isActive ? 1 : 0);
  pms_readcmdbuf();  
}

void pms_setsleep(bool isSleep)
{
  pms_cmd(PMS_CMD_SLEEP, 0, isSleep ? 0 : 1);
  if (isSleep) //sensor does not sends buf to wakeup command
  {
    pms_readcmdbuf();
  }
}



