#include <Arduino.h>
#include "heiger.h"
//conversion factors from https://radmon.org/index.php/forum/kit-geiger-counters/936-connecting-geiger-counter-for-arduino-and-raspberry-pi-to-radmon-org
// for J305By geiger tube
#define CONV_FACTOR 0.00812
// for SBM-20 geiger tube (1/361)
//#define CONV_FACTOR 0.00277

#define HEIGER_PIN 2
#define MEASURE_RANGE 1200000//milliseconds
#define MAX_SLOTS 60

uint16_t heiger_counts[MAX_SLOTS];
uint16_t heiger_times[MAX_SLOTS];


int heiger_count;
long heiger_lastms;
float heiger_rad;

void heiger_ISR() {
  heiger_count++;
  //Serial.write('*');
}

void heiger_setup()
{
  pinMode(HEIGER_PIN, INPUT);
  digitalWrite(HEIGER_PIN, HIGH);
  heiger_count = 0;
  heiger_lastms = millis();
  attachInterrupt(digitalPinToInterrupt(HEIGER_PIN), heiger_ISR, FALLING);
}


float heiger_getRadiation()
{
  long deltat = millis() - heiger_lastms;
  Serial.println("Heiger: deltat=" + String(deltat) +", cnt=" + String(heiger_count));
  if (deltat < MEASURE_RANGE) {
    return heiger_rad;
  }
  heiger_rad = 60000 * heiger_count * CONV_FACTOR / deltat;
  heiger_lastms = millis();
  heiger_count = 0;
  return heiger_rad;
}
