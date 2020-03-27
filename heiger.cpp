#include <Arduino.h>
#include "heiger.h"
//conversion factors from https://radmon.org/index.php/forum/kit-geiger-counters/936-connecting-geiger-counter-for-arduino-and-raspberry-pi-to-radmon-org
// for J305By geiger tube
#define CONV_FACTOR 0.00812
// for SBM-20 geiger tube (1/361)
//#define CONV_FACTOR 0.00277

#define DBGT(x) Serial.print(F(x));
#define DBG(x) Serial.print(x);
#define DBGLN Serial.println();

#define HEIGER_PIN 2
#define MAX_SLOTS 20  

//this is FIFO array of the measurements
//new measurement goes into the last element of the array; previous elements are shifted to smaller index (e.g. 3th element shifts to 2)
//values are adjusted during shift as described below
//times contain time (in seconds) that has left since the last measurement
//counts contain accumulated number of Heiger ticks since the last measurement
//thus, the first record would contain total count and total time for all slots
uint16_t heiger_counts[MAX_SLOTS];
uint16_t heiger_times[MAX_SLOTS];


int heiger_count;
long heiger_lastms;
float heiger_rad;

void heiger_ISR() {
  static unsigned long heiger_ISR_millis_prev;
  if(millis()-5 > heiger_ISR_millis_prev) 
  {
    heiger_count++;
    //DBG(millis());DBGLN;
  }
  heiger_ISR_millis_prev = millis(); 
}

void heiger_setup()
{
  pinMode(HEIGER_PIN, INPUT);
  digitalWrite(HEIGER_PIN, HIGH);
  heiger_count = 0;
  heiger_rad = 0;
  heiger_lastms = millis();
  attachInterrupt(digitalPinToInterrupt(HEIGER_PIN), heiger_ISR, FALLING);
}


float heiger_getRadiation()
{
  //shift and adjust all slots
  int secsSinceLast = (millis() - heiger_lastms) / 1000;
  if (secsSinceLast < 60) return heiger_rad;//single measurement should last 1 minute at least
  heiger_lastms = millis();
  int cnt = heiger_count;
  heiger_count = 0;
  //DBGT("Heiger: secs=");DBG(secsSinceLast);DBGT(", cnt=");DBG(cnt);DBGLN;
  heiger_rad = 0;
  for (byte i=0;i<MAX_SLOTS-1;i++) {
    heiger_counts[i] = heiger_counts[i+1] + cnt;
    heiger_times[i] = heiger_times[i+1] + secsSinceLast;
    //DBGT("Heiger: ");DBG(i);DBGT(", secs=");DBG(heiger_times[i]);DBGT(", cnt=");DBG(heiger_counts[i]);DBGLN;
  }
  heiger_rad = 60 * heiger_counts[0] * CONV_FACTOR / heiger_times[0];
  //reset the value at the head of the array
  heiger_counts[MAX_SLOTS-1] = 0;
  heiger_times[MAX_SLOTS-1] = 0;
  return heiger_rad;
}
