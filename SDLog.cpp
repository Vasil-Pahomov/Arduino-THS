#include <SPI.h>
#include <SD.h>
#include "global.h"
#include "SDlog.h"

//CS pin of the SD card
#define SD_CS_PIN 10
#define INDEX_FILE_NAME F("i")
#define RECS_PER_FILE 10000 //10000 records per file
#define MAX_FILES_COUNT 4000 //this results in less than 1 gig in the total files size and stores data for 10 years of continous operation - seems appropriate
#define DEBUG

File file;
//number of current file being written, initial value means it's not initialized yet
uint16_t curFileNum = 0xFFFF;
uint32_t lastLogIdx = 0xFFFFFFFF;

bool updateIndexFile()
{
  file = SD.open(INDEX_FILE_NAME, FILE_WRITE);
  if (!file) {
#ifdef DEBUG
    Serial.println(F("SD: error index writing"));
#endif      
    return false;
  }
  file.seek(0);
  file.write((uint8_t*)&curFileNum,2);
  file.close(); 
  return true;
}

bool ensureInit() {
  if (curFileNum == 0xFFFF) {
#ifdef DEBUG
      Serial.println(F("SD: initializing"));
#endif
    if (!SD.begin(SD_CS_PIN)) {
#ifdef DEBUG
      Serial.println(F("SD: init error"));
#endif
      return false;
    }

    if (!SD.exists(INDEX_FILE_NAME)) {
#ifdef DEBUG
      Serial.println(F("SD: no index file, initializing"));
#endif
      curFileNum = 0;
      if (!updateIndexFile()) {
        return;
      }
    } else {
#ifdef DEBUG
      Serial.println(F("SD: read index file"));
#endif
      file = SD.open(INDEX_FILE_NAME, FILE_READ);
      if (!file) {
#ifdef DEBUG
        Serial.println(F("SD: error index reading"));
#endif      
        return false;
      }
      file.read(&curFileNum,2);
      file.close(); 
#ifdef DEBUG
      Serial.print(F("SD: index read:"));Serial.println(curFileNum);
#endif
    }
  }
  return true;
}



bool writeLog(DLog* rec)
{
  if (!ensureInit()) {
    return false;
  }
  String fname = String(curFileNum);
  long filesize = file.size();
  if (SD.exists(fname)) {
    file = SD.open(fname,FILE_READ);
    if (!file) {
  #ifdef DEBUG
      Serial.print(F("SD: error opening log "));Serial.println(curFileNum);
  #endif      
      return false;
    }
    file.close();
    if (filesize > sizeof(DLog)*RECS_PER_FILE) {
      //assume file counter never overruns. once data is written every 10 seconds and single file has 10000 records, 16-bit number should overrun in almost 200 years of continous operation
      curFileNum++;
  #ifdef DEBUG
      Serial.print(F("SD: increasing file count to "));Serial.println(curFileNum);
  #endif      
      if (!updateIndexFile()) {
        return;
      }
  #ifdef DEBUG
      Serial.print(F("SD: checking file count "));Serial.print(curFileNum);Serial.print('/');Serial.println(MAX_FILES_COUNT);
  #endif           
      if (curFileNum >= MAX_FILES_COUNT) {
  #ifdef DEBUG
      Serial.print(F("SD: removing tail file "));Serial.println(curFileNum - MAX_FILES_COUNT);
  #endif       
        if (!SD.remove(String(curFileNum - MAX_FILES_COUNT))) {
  #ifdef DEBUG
      Serial.println(F("SD: error removing tail file"));
  #endif      
        }
      }
    }
  }
  file = SD.open(fname,FILE_WRITE);
  if (!file) {
#ifdef DEBUG
    Serial.print(F("SD: error writing log "));Serial.println(curFileNum);
#endif      
    return false;
  }
  file.write((uint8_t*)rec, sizeof(DLog));
  file.close(); 

  lastLogIdx = curFileNum * RECS_PER_FILE + filesize / sizeof(DLog);
  
  return true;
}




