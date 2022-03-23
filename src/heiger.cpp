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
#define EMA_ALPHA 40 //Exponential moving average coefficient

int heiger_count;
long heiger_lastms;
float heiger_rad_total;

void heiger_ISR() {
  static unsigned long heiger_ISR_millis_prev;
  if(millis()-5 > heiger_ISR_millis_prev) 
  {
    heiger_count++;
    //DBGT("!");
  }
  heiger_ISR_millis_prev = millis(); 
}

void heiger_setup()
{
  pinMode(HEIGER_PIN, INPUT);
  digitalWrite(HEIGER_PIN, HIGH);
  heiger_count = 0;
  heiger_rad_total = 0.18 * EMA_ALPHA;
  heiger_lastms = millis();
  attachInterrupt(digitalPinToInterrupt(HEIGER_PIN), heiger_ISR, FALLING);
}


float heiger_getRadiation()
{
  int secsSinceLast = (millis() - heiger_lastms) / 1000;
  if (secsSinceLast < 60) return heiger_rad_total/EMA_ALPHA;//single measurement should last 1 minute at least; return last calculated value
  heiger_lastms = millis();
  int cnt = heiger_count;
  heiger_count = 0;

  float cur_rad = 60 * cnt * CONV_FACTOR / secsSinceLast;
  //DBG(cnt);DBGT("/");DBG(secsSinceLast);DBGT("-");DBG(cur_rad);DBGLN;
  heiger_rad_total += cur_rad - heiger_rad_total/EMA_ALPHA;

  return heiger_rad_total/EMA_ALPHA;
}
