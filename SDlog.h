#ifndef SDlog_h
#define SDLog_h

//defines logging entry
//yes I know using static structure is not flexible, but it much simplier to handle
typedef struct DLog {
  //system (internal) time in seconds, starting from turning on
  uint32_t ssecs;
  
  //external (real) time, UNIX timestamp. Zero when external time is not known
  uint32_t rtime;

  //temperature, in Celsius multiplied by 100
  int16_t temp;

  //relative humidity, in percent multiplied by 100
   uint16_t hum;

   //CO2, in ppm
   uint16_t co2;
   
   //PM1.0 reading, in ug/m^3
   uint16_t pm1;

   //PM2.5 reading, in ug/m^3
   uint16_t pm25;

   //PM10 reading, in ug/m^3
   uint16_t pm10;

   //TVOC reading, ppd
   uint16_t tvoc;
   
} DLog;

bool writeLog(DLog* rec);

#endif
