#include <SoftwareSerial.h>
#include <Arduino.h>
#include "pms5003.h"
#include "global.h"
//#define DEBUG
SoftwareSerial pmsSerial(A2, A3); // A2 - к TX сенсора, A3 - к RX

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

#define intresponse(highidx, lowidx) (response[highidx] << 8) + (response[lowidx])

void pms_setup() {
  pmsSerial.begin(9600);
  pms_setactive(false);
}

bool pms_read()
{
  pms_cmd(PMS_CMD_READ, 0, 0);
  memset(response,0,32);
  pmsSerial.readBytes(response, 32);
  pms_error = false;

  if (response[0] != 0x42 || response[1] != 0x4d) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: signature incorrect, expecled 16973, found:"));Serial.println(intresponse(0,1));
#endif
  }
  
  if (response[2] != 0 || response[3] != 28) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: frame length is incorrect, expected 28, found:"));Serial.println(intresponse(2,3));
#endif
  }

  unsigned int checksum = 0;
  for (byte i=0;i<30;i++)
  {
    checksum += response[i];
  }
  if (response[30] != (checksum >> 8) || response[31] != (checksum & 255)) {
    pms_error = true;
#ifdef DEBUG
    Serial.print(F("PMS read warning: checksum is incorrect, expected")); Serial.print(checksum); Serial.print(F(", found:"));Serial.println(intresponse(30,31));     
#endif
  }

#ifdef DEBUG
  if (pms_error) {
    Serial.print("PMS: resp ");
    for (byte i = 0; i < 32; i++) {
      Serial.print(response[i]);Serial.print(' ');
    }
    Serial.println();    
  }
#endif

  if (!pms_error) {
    pms_pm1_cf1 = intresponse(4,5);
    pms_pm2_5_cf1 = intresponse(6,7);
    pms_pm10_cf1 = intresponse(8,9);
    pms_pm1_ae = intresponse(10,11);
    pms_pm2_5_ae = intresponse(12,13);
    pms_pm10_ae = intresponse(14,15);
    pms_num_0_3 = intresponse(16,17);
    pms_num_0_5 = intresponse(18,19);
    pms_num_1 = intresponse(20,21);
    pms_num_2_5 = intresponse(22,23);
    pms_num_5_0 = intresponse(24,25);
    pms_num_10 = intresponse(26,27);
  }
}

void pms_cmd(byte command, byte datah, byte datal)
{
  pmsSerial.listen();
  byte icmd[7] = {0x42, 0x4d, command, datah, datal, 0x00, 0x00};
  int vb = icmd[0] + icmd[1] + icmd[2] + icmd[3] + icmd[4];
  icmd[5] = (byte) (vb / 256);
  icmd[5] = (byte) (vb & 255);
  pmsSerial.write(icmd, 7);
#ifdef DEBUG
  Serial.print("PMS: cmd ");
  for (byte i = 0; i < 7; i++) {
    Serial.print(icmd[i]);Serial.print(' ');
  }
  Serial.println();
#endif
}

void pms_readcmdresponse()
{
  //though not documented, on some commands sensor sends the following response:
  //0x42 0x4d - signature bytes
  //0x00 0x04 - always 4 (a kind of frame length?)
  //<command byte> <data byte> <data byte> - the same as sent in command
  //<verify_high_byte> <verify_low_byte> - sum of all bytes except signature (thus it equals to corresponding sum in the command plus 4)
#ifdef DEBUG
  Serial.print("PMS: cmdresp=");Serial.println(pmsSerial.readBytes(response, 9));
#else
  pmsSerial.readBytes(response, 9);
#endif
  //todo: add response check    
}
void pms_setactive(bool isActive)
{
  pms_cmd(PMS_CMD_MODE, 0, isActive ? 1 : 0);
  pms_readcmdresponse();  
}

void pms_setsleep(bool isSleep)
{
  pms_cmd(PMS_CMD_SLEEP, 0, isSleep ? 0 : 1);
  if (isSleep) //sensor does not sends response to wakeup command
  {
    pms_readcmdresponse();
  }
}


