#include <SPI.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include "global.h"
#include "SDlog.h"

#define SD_CS_PIN 10 //CS pin of the SD card
#define INDEX_FILE_NAME F("i")
#define RECS_PER_FILE 5 //records per file
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
  long filesize = 0;
  if (SD.exists(fname)) {
    file = SD.open(fname,FILE_READ);
    if (!file) {
  #ifdef DEBUG
      Serial.print(F("SD: error opening log "));Serial.println(curFileNum);
  #endif      
      return false;
    }
    filesize = file.size();
    file.close();
    if (filesize >= sizeof(DLog)*RECS_PER_FILE) {
      //assume file counter never overruns. once data is written every 10 seconds and single file has 10000 records, 16-bit number should overrun in almost 200 years of continous operation
      curFileNum++;
      fname = String(curFileNum);
      if (SD.exists(fname)) {
        SD.remove(fname);
      }
      filesize = 0;
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
    }//end if file overrun
  }//end if file exists
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
#ifdef DEBUG
  Serial.print(F("SD: written rec #"));Serial.print(lastLogIdx);Serial.print(F(" to file #"));Serial.println(curFileNum);
#endif      
  
  return true;
}

//resets file number so that all previous content of SD card starts being overwritten anew
void sdReset() {
  curFileNum = 0;
  updateIndexFile();
  String fname = String(curFileNum);
  if (SD.exists(fname)) {
    SD.remove(fname);
  }
#ifdef DEBUG
  Serial.println(F("SD: reset done "));
#endif      
}

void sdTransmitData(SoftwareSerial *btSerial) {
  uint32_t cIdx = *((uint32_t*) (buf + 3));
  uint32_t toIdx = *((uint32_t*) (buf + 7));
  int fileIdx = cIdx / RECS_PER_FILE;
  int recIdx = cIdx % RECS_PER_FILE;
#ifdef DEBUG
        Serial.print(F("SD: read data request"));Serial.print(cIdx);Serial.print('-');Serial.print(toIdx);Serial.print(F(", file/rec="));Serial.print(fileIdx);Serial.print('/');Serial.println(recIdx);
#endif        

  bool firstRecord = true;
  String fname = String(fileIdx);
  while (cIdx <= toIdx) {
    if (firstRecord) {
      // the very first record for transmitting
#ifdef DEBUG
        Serial.print(F("SD: opening first file / at pos:"));Serial.print(fname);Serial.print('/');Serial.println(recIdx * sizeof(DLog));
#endif        
      file = SD.open(fname, FILE_READ);
      if (!file) {
#ifdef DEBUG
        Serial.print(F("SD: error opening file for transmitting: "));Serial.println(fname);
#endif        
        return;
      }
      if (!file.seek(recIdx * sizeof(DLog))) {
#ifdef DEBUG
        Serial.print(F("SD: error seeking file to "));Serial.println(recIdx * sizeof(DLog));
#endif        
        return;
      }
    } 
    firstRecord = false;

    //transmitting the record
    if (file.read(buf, sizeof(DLog))) {
#ifdef DEBUG
      Serial.print(F("SD: transmitting rec "));Serial.print(recIdx);Serial.print(F(" of file "));Serial.print(fname);Serial.print(' ');Serial.print(cIdx);Serial.print('/');Serial.println(toIdx);
#endif        
      btSerial->write(buf, sizeof(DLog));
    } else {
#ifdef DEBUG
        Serial.print(F("SD: error reading file "));Serial.println(fname);
#endif        
      return;
    }
    recIdx++;
    if (recIdx >= RECS_PER_FILE) {
      //jumping to the next file
      file.close();
      fname = String(fileIdx);
      file = SD.open(fname, FILE_READ);
      fileIdx++;
      recIdx = 0;
#ifdef DEBUG
      Serial.print(F("SD: Jumping to next file "));Serial.println(fname);
#endif        
    }
    
    cIdx++;
  }
  file.close();
#ifdef DEBUG
  Serial.print(F("SD: Transmission done "));Serial.println(fname);
#endif        
}
