#ifndef SDlog_h
#define SDLog_h

#include "Data.h" 

extern uint32_t lastLogIdx;

//defines logging entry
//yes I know using static structure is not flexible, but it much simplier to handle
typedef struct DLog {
  //system (internal) time in seconds, starting from turning on
  uint32_t ssecs;
  
  //external (real) time, UNIX timestamp. Zero when external time is not known
  uint32_t rtime;

  //measurement data
  Data data;
} DLog;

bool writeLog(DLog* rec);

void sdReset();

#endif
