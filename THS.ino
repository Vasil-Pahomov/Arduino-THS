#include <LiquidCrystal.h>
#include <OneWire.h>
#define DEBUG

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
OneWire ds(2);

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
  lcd.setCursor(0, 1);

  Serial.begin(9600);

  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(en,OUTPUT);
}

void loop() {

  

  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):

  // print the number of seconds since reset:
  //lcd.print(millis() / 1000);
  digitalWrite(LED_BUILTIN, HIGH);
  //digitalWrite(rs,HIGH);
  //delay(500);
  digitalWrite(LED_BUILTIN, LOW);
  //digitalWrite(en,LOW);
  //delay(500);

  lcd.setCursor(0, 1);
  lcd.print(getTemperature());
  
}

float getTemperature()
{
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius = 0;
  byte count = 0;

  ds.reset_search();
  
  while (ds.search(addr))
  {
  
    count++;
    
    #ifdef DEBUG
    Serial.print(F("DS: ROM ="));
      for( i = 0; i < 8; i++) {
      Serial.write(' ');
      Serial.print(addr[i], HEX);
    }
    #endif
    
    if (OneWire::crc8(addr, 7) != addr[7]) {
        #ifdef DEBUG
        Serial.println("DS: CRC is not valid!");
        #endif
        return -101;
    }
    #ifdef DEBUG
    Serial.println();
    #endif
   
    // the first ROM byte indicates which chip
    switch (addr[0]) {
      case 0x10:
        #ifdef DEBUG
        Serial.println("DS:  Chip = DS18S20");  // or old DS1820
        #endif
        type_s = 1;
        break;
      case 0x28:
        #ifdef DEBUG
        Serial.println("  Chip = DS18B20");
        #endif
        type_s = 0;
        break;
      case 0x22:
        #ifdef DEBUG
        Serial.println("  Chip = DS1822");
        #endif
        type_s = 0;
        break;
      default:
        #ifdef DEBUG
        Serial.println("Device is not a DS18x20 family device.");
        #endif
        return;
    } 
  
    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);        // start conversion, with parasite power on at the end
    
    delay(1000);     // maybe 750ms is enough, maybe not
    // we might do a ds.depower() here, but the reset will take care of it.
    
    present = ds.reset();
    ds.select(addr);    
    ds.write(0xBE);         // Read Scratchpad
  
    #ifdef DEBUG
    Serial.print("  Data = ");
    Serial.print(present, HEX);
    Serial.print(" ");
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
      Serial.print(data[i], HEX);
      Serial.print(" ");
    }
    Serial.print(" CRC=");
    Serial.print(OneWire::crc8(data, 8), HEX);
    Serial.println();
    #else
    for ( i = 0; i < 9; i++) {           // we need 9 bytes
      data[i] = ds.read();
    }
    #endif
  
    // Convert the data to actual temperature
    // because the result is a 16 bit signed integer, it should
    // be stored to an "int16_t" type, which is always 16 bits
    // even when compiled on a 32 bit processor.
    int16_t raw = (data[1] << 8) | data[0];
    if (type_s) {
      raw = raw << 3; // 9 bit resolution default
      if (data[7] == 0x10) {
        // "count remain" gives full 12 bit resolution
        raw = (raw & 0xFFF0) + 12 - data[6];
      }
    } else {
      byte cfg = (data[4] & 0x60);
      // at lower res, the low bits are undefined, so let's zero them
      if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
      else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
      else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
      //// default is 12 bit resolution, 750 ms conversion time
    }
    celsius = celsius + ((float)raw / 16.0);
    #ifdef DEBUG
    Serial.print("  Temperature = ");
    Serial.print(celsius);
    Serial.print(" Celsius, ");
    #endif
  }

  #ifdef DEBUG
  Serial.println(F("DS: No more addresses."));
  #endif
  if (count == 0) 
    return -100;
  else
    return celsius/count;

}


