#include <LiquidCrystal.h>
#define DEBUG
#include "TempDS.h"
#include "mh_z19.h"



// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 9, en = 8, d4 = 7, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);


void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("hello, world!");
  lcd.setCursor(0, 1);

  Serial.begin(9600);
  mh_setup();

  pinMode(LED_BUILTIN, OUTPUT);
  //pinMode(en,OUTPUT);
}

void loop() {
  
  digitalWrite(LED_BUILTIN, HIGH);
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  lcd.setCursor(0, 1);
  lcd.print(getTemperature());
  lcd.setCursor(8,1);
  lcd.print(mh_getPPM());
  
}




