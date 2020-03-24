/*
 Alarm code
 T. Pirson
*/

// include the library code:
#include <LiquidCrystal.h>

// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
const int rs = 13, en = 12, d4 = 11, d5 = 10, d6 = 9, d7 = 8;
const int gatePin = 7;
const int buttonPin = 5; 
const int poweSensePin = 6;

int val = 0;
float ANALOG_RANGE = 1023.0;
int counter = 0;
int buttonState = 0; 
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

void setup() {
  Serial.begin(9600);
  Serial.println("Hello, world!");
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // initialize the pushbutton pin as an input:
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(poweSensePin, INPUT);
  
  // Print a message to the LCD.
  lcd.print("ALARM MODULE");
  delay(5000);
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.clear();
  lcd.print("Alarme DESACTIVEE");
  analogWrite(gatePin, 255);
  delay(500);
  lcd.clear();
  
  
  int low_battery_flag = digitalRead(poweSensePin) == HIGH;
  Serial.println(low_battery_flag);
  
  if (low_battery_flag){
    lcd.clear();
    lcd.print("! LOW SECURITY");
    lcd.setCursor(7, 1);
    lcd.print("BATTERY !");
    delay(500);
  }

  if (counter == 15) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Alarme ACTIVEE");
    analogWrite(gatePin, 0);
    lcd.setCursor(0, 1);
    lcd.print("Press *button*");
    buttonState = digitalRead(buttonPin);
    while (buttonState != HIGH) {
      delay(100);
      buttonState = digitalRead(buttonPin);
    }
    counter = 0;
  }
  counter++;
}
