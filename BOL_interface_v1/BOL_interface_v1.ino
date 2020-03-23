#include <Wire.h>
#include <LiquidCrystal.h>

#define NO_UPDATE           0
#define UPDATE_STATE        1
#define ALERT_MAX_PRESSURE  2
#define ALERT_POWER_SUPPLY  3

#define PARAM_FRESPI        0
#define PARAM_VTIDAL        1
#define PARAM_PMAX          2
#define PARAM_IERATE        3

#define FRESPI_MIN          0
#define FRESPI_MAX          40

#define VTIDAL_MIN          0
#define VTIDAL_MAX          1500

#define PMAX_MIN            20
#define PMAX_MAX            60

#define IERATE1TO3          0 
#define IERATE1TO2          1
#define IERATE1TO15         2
#define IERATE1TO1          3
#define IERATE2TO1          4

#define INTER_BUTTON_DELAY  100

LiquidCrystal lcd(13, 12, 11, 10, 9, 8);

int leftPin = 2;
int rightPin = 3;
int upPin = 4;
int downPin = 5;
int alarmPin = 7;
int powerSensePin = 6;

int dIF = 100;  //enter every 100ms
int dMot = 10;
int dSense = 10; 
int dButton = 40;
int dInterButton = 0;

long tInIF;
long tInMot;
long tInSense; 
long tInButton;
long currTime;

byte currParam = 0;
byte screenType = UPDATE_STATE;

byte pMaxVal = 20;
byte fRespi = 30;
int  vTidal = 500;
byte inExpRate = 1;

int sPressure;
int sLowPower;


/* On screen : 
Line 1 : 
  2 chars : fRespi : 0-40
  1 space
  4 chars : vTidal : 0-1500ml
  1 space
  2 chars : pMax : 20-60mBar
  1 space
  5 chars : inExpRate : 1:1,5
Line 2: 
  Name of the current parameter afffected by +-
*/

void setup()
{
  pinMode(alarmPin,OUTPUT);
  pinMode(leftPin,INPUT_PULLUP);
  pinMode(rightPin,INPUT_PULLUP);
  pinMode(upPin,INPUT_PULLUP);
  pinMode(downPin,INPUT_PULLUP);

  lcd.begin(16, 2);               // start lcd
  lcd.setCursor(0,0);
  lcd.print("Breath for Life");
  lcd.setCursor(0,1);
  lcd.print("V0.1");
  delay(4000);
  lcd.clear();
  lcd.setCursor(0, 0);

  tInIF = millis();
  tInMot = millis();
  tInSense = millis(); 
  tInButton = millis();
}


void loop()
{
  currTime = millis();
  if(tInIF+dIF<currTime){
    tInIF = millis();
    // Set interface
    if (screenType == UPDATE_STATE){
      set_interface();  
    } else if (screenType != NO_UPDATE){
      set_alert();
    }
  }
  if(tInMot+dMot<currTime){
    tInMot = millis();
    // Set motors
  }
  if(tInSense+dSense<currTime){
    tInSense = millis();
    // Read sensors
    // Pressure sensor value in int sPressure
    // to set alert PMAX : 
    // screenType = ALERT_MAX_PRESSURE;
    if (digitalRead(powerSensePin)==LOW) {
      screenType = ALERT_POWER_SUPPLY;
      // TODO : Deactivate motors
    }
  } 
  if(tInButton+dButton+dInterButton<currTime){
    tInButton = millis();
    
    // Read buttons, update state
    byte leftVal = digitalRead(leftPin);
    byte rightVal = digitalRead(rightPin);
    byte upVal = digitalRead(upPin);
    byte downVal = digitalRead(downPin);
    
    if(leftVal==LOW){
      currParam = (currParam-1)&0x3;
    } else if(rightVal==LOW){
      currParam = (currParam+1)&0x3;
    }

    if (leftVal==LOW || rightVal==LOW || upVal==LOW || downVal==LOW){
      // update screen : either values or reset alarm
      screenType = UPDATE_STATE;
      // reset alarm on any push of a button
      //noTone(alarmPin);
      digitalWrite(alarmPin,LOW);
      // if a button was pressed, next one must wait 
      dInterButton = INTER_BUTTON_DELAY;
    } else {
      // no button was pressed, reset inter button additional delay
      dInterButton = 0;
    }
    
    
    if ((currParam == PARAM_FRESPI) && upVal==LOW) {
      fRespi = (fRespi>=FRESPI_MAX) ? FRESPI_MAX : fRespi+1;   
    } else if ((currParam == 0) && downVal==LOW) {
      fRespi = (fRespi<FRESPI_MIN) ? FRESPI_MIN : fRespi-1;
    }

    if ((currParam == PARAM_VTIDAL) && upVal==LOW) {
      vTidal = (vTidal>=VTIDAL_MAX) ? VTIDAL_MAX : vTidal+10;
    } else if ((currParam == PARAM_VTIDAL) && downVal==LOW) {
      vTidal = (vTidal<=VTIDAL_MIN) ? VTIDAL_MIN : vTidal-10;
    }

    if ((currParam == PARAM_PMAX) && upVal==LOW) {
      pMaxVal = (pMaxVal>=PMAX_MAX) ? PMAX_MAX : pMaxVal+1;
    } else if ((currParam == PARAM_PMAX) && downVal==LOW) {
      pMaxVal = (pMaxVal<=PMAX_MIN) ? PMAX_MIN : pMaxVal-1;
    }

    if ((currParam == PARAM_IERATE) && upVal==LOW) {
      inExpRate = (inExpRate==IERATE2TO1) ? IERATE2TO1 : inExpRate+1;
    } else if ((currParam == PARAM_IERATE) && downVal==LOW) {
      inExpRate = (inExpRate==IERATE1TO3) ? IERATE1TO3 : inExpRate-1;
    }
  }  
}

void set_interface(){
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(fRespi);
  lcd.setCursor(3,0);
  lcd.print(vTidal);
  lcd.setCursor(8,0);
  lcd.print(pMaxVal);
  lcd.setCursor(11,0);
  switch (inExpRate){
    case 0:
      lcd.print("1:3");
    break;
    case 1:
      lcd.print("1:2");
    break;
    case 2:
      lcd.print("1:1,5");
    break;
    case 3:
      lcd.print("1:1");
    break;
    case 4:
      lcd.print("2:1");
    break;
  }
  // Second line
  lcd.setCursor(0,1);
  switch (currParam) {
      case 0:
        lcd.print("Freq Respiration");
      break;
      case 1:
        lcd.print("  Volume Tidal  ");
      break;
      case 2:
        lcd.print("  Pression Max  ");
      break;
      case 3:
        lcd.print("  Rapport I:E   ");
      break;
  }
  screenType = NO_UPDATE;
}

void set_alert() {
  lcd.setCursor(0, 0);
  lcd.print("ALERTE : Erreur");
  lcd.setCursor(0, 1);
  switch (screenType){
   case ALERT_MAX_PRESSURE:
    lcd.print("P Max Depasse");
    break;
    case ALERT_POWER_SUPPLY:
    lcd.print("Alimentation");
    break;
   default :
    lcd.setCursor(0, 1);
    lcd.print("Erreur inconnue");
    break;
  }
  //tone(alarmPin,1000);
  digitalWrite(alarmPin,HIGH);
  
  /*
  for (int i=0; i<5;i++){
    lcd.setCursor(5, 0);
    lcd.print("      ");
    //digitalWrite(buzzerPin,HIGH);
    tone(buzzerPin,1000);
    delay(500);
    noTone(buzzerPin);
    lcd.setCursor(5, 0);
    lcd.print("ALERTE");
    //digitalWrite(buzzerPin,LOW);
    delay(500);
  }*/
}
