#ifndef PMS5003_H
#define PMS5003_H
void pms_setup();
extern unsigned int 
  pms_pm1_cf1, 
  pms_pm2_5_cf1, 
  pms_pm10_cf1, 
  pms_pm1_ae, 
  pms_pm2_5_ae,
  pms_pm10_ae,
  pms_total_ae,
  pms_num_0_3,
  pms_num_0_5,
  pms_num_1,
  pms_num_2_5,
  pms_num_5_0,
  pms_num_10;

extern bool pms_error;
  
void pms_setup();
bool pms_read();

void pms_cmd(byte command, byte datah, byte datal);
void pms_setactive(bool isActive);
void pms_setsleep(bool isSleep);



#endif

