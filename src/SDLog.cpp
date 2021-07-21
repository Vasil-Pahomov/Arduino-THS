#include <SdFat.h>
#include "global.h"
#include "SDlog.h"

#define SD_CS_PIN 10 //CS pin of the SD card

//sector 0 keeps index information:
//bytes 0 to 3 (32-bit word) keeps log record index that will be written no
//assuming sectors will never overrru: about 1 GB holds data for 10 years of continous operation - seems appropriate
#define SECTOR_SIZE 512 //in bytes
#define RECS_PER_FILE SECTOR_SIZE/sizeof(DLog) //records per file
//#define DEBUG //note that disabling it without modifying all other code results in disabling UART reception from Bluetooth module. Seems like magic

SdCard SD;
//number of log record that will be being written; initial value means it's not initialized yet
uint32_t lastLogIdx = 0xFFFFFFFF;
byte sd_buf[SECTOR_SIZE];

bool updateIndexFile()
{
  
  memcpy(&lastLogIdx, sd_buf, 4);
  if (!SD.writeSector(0, sd_buf)) {
#ifdef DEBUG
    Serial.println(F("SD: error index writing"));
#endif      
    return false;
  }
  return true;
}

bool ensureInit() {
  /*
  if (curSectorNum == 0xFFFF) {
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
      curSectorNum = 0;
      if (!updateIndexFile()) {
        return false;
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
      file.read(&curSectorNum,2);
      file.close(); 
#ifdef DEBUG
      Serial.print(F("SD: index read:"));Serial.println(curSectorNum);
#endif
    }
  }
  return true;
  */
}

bool writeLog(DLog* rec)
{
  /*
  if (!ensureInit()) {
    return false;
  }
  String fname = String(curSectorNum);
  long filesize = 0;
  if (SD.exists(fname)) {
    file = SD.open(fname,FILE_READ);
    if (!file) {
  #ifdef DEBUG
      Serial.print(F("SD: error opening log "));Serial.println(curSectorNum);
  #endif      
      return false;
    }
    filesize = file.size();
    file.close();
    if (filesize >= sizeof(DLog)*RECS_PER_FILE) {
      //assume file counter never overruns. once data is written every 10 seconds and single file has 10000 records, 16-bit number should overrun in almost 200 years of continous operation
      curSectorNum++;
      fname = String(curSectorNum);
      if (SD.exists(fname)) {
        SD.remove(fname);
      }
      filesize = 0;
  #ifdef DEBUG
      Serial.print(F("SD: increasing file count to "));Serial.println(curSectorNum);
  #endif      
      if (!updateIndexFile()) {
        return false;
      }
  #ifdef DEBUG
      Serial.print(F("SD: checking file count "));Serial.print(curSectorNum);Serial.print('/');Serial.println(MAX_FILES_COUNT);
  #endif           
      if (curSectorNum >= MAX_FILES_COUNT) {
  #ifdef DEBUG
      Serial.print(F("SD: removing tail file "));Serial.println(curSectorNum - MAX_FILES_COUNT);
  #endif       
        if (!SD.remove(String(curSectorNum - MAX_FILES_COUNT))) {
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
    Serial.print(F("SD: error writing log "));Serial.println(curSectorNum);
#endif      
    return false;
  }
  file.write((uint8_t*)rec, sizeof(DLog));
  file.close(); 

  lastLogIdx = (long)curSectorNum * RECS_PER_FILE + filesize / sizeof(DLog);
#ifdef DEBUG
  Serial.print(F("SD: written rec #"));Serial.print(lastLogIdx);Serial.print(F(" to file #"));Serial.println(curSectorNum);
#endif      
  
  return true;
  */
}

//resets file number so that all previous content of SD card starts being overwritten anew
void sdReset() {
  /*
  curSectorNum = 0;
  updateIndexFile();
  String fname = String(curSectorNum);
  if (SD.exists(fname)) {
    SD.remove(fname);
  }
#ifdef DEBUG
  Serial.println(F("SD: reset done "));
#endif      
*/
}

void sdTransmitData() {
  /*
  uint32_t cIdx = *((uint32_t*) (buf + 3));
  uint32_t toIdx = *((uint32_t*) (buf + 7));
  
  int fileIdx = cIdx / RECS_PER_FILE;
  int recIdx = cIdx % RECS_PER_FILE;
#ifdef DEBUG
  Serial.print(F("SD: read data request"));Serial.print(cIdx);Serial.print('-');Serial.print(toIdx);Serial.print(F(", file/rec="));Serial.print(fileIdx);Serial.print('/');Serial.println(recIdx);
#endif

//once I uncomment one of these lines below, the receiving data by bluetooth stops working!
//I don't know WTF is here
  if (lastLogIdx == 0xFFFFFFFF) {
#ifdef DEBUG
//  Serial.println(F("SD: read data request rejected, no data (yet)"));
#endif
//    return;
  }

  lcd.clear();
  btSerial.write(buf,11);

  long lastmst = millis() - 10000;

  bool firstRecord = true;
  String fname = String(fileIdx);
  while (cIdx <= toIdx) {
    if (firstRecord) {
      // the very first record for transmitting
#ifdef DEBUG
//        Serial.print(F("SD: opening first file / at pos:"));Serial.print(fname);Serial.print('/');Serial.println(recIdx * sizeof(DLog));
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
      btSerial.write(buf, sizeof(DLog));
      if (millis() > lastmst + 500) {
        lcd.setCursor(0,0);
        lcd.print(cIdx);lcd.print('/');lcd.print(toIdx);
        lastmst = millis();
      }
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
      fileIdx++;
      fname = String(fileIdx);
      file = SD.open(fname, FILE_READ);
      recIdx = 0;
#ifdef DEBUG
//      Serial.print(F("SD: Jumping to next file "));Serial.println(fname);
#endif        
    }
    
    cIdx++;
  }
  file.close();
#ifdef DEBUG
//  Serial.println(F("SD: Transmission done "));
#endif        
*/
}
