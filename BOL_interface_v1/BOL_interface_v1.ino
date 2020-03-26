#include <LiquidCrystal.h>

#define NO_UPDATE           0
#define UPDATE_STATE        1
#define UPDATE_PRESSURE     2
#define ALERT_MAX_PRESSURE  3
#define ALERT_POWER_SUPPLY  4

//#define PARAM_FRESPI        2
//#define PARAM_VTIDAL        3
#define PARAM_PMAX          0
#define PARAM_IERATE        1

#define FRESPI_MIN          12
#define FRESPI_MAX          20
#define FRESPI_STEP         1
#define FRESPI_DEFAULT      15

#define VTIDAL_MIN          300
#define VTIDAL_MAX          600
#define VTIDAL_STEP         10
#define VTIDAL_DEFAULT      450

#define PMAX_MIN            20
#define PMAX_MAX            60
#define PMAX_STEP           1
#define PMAX_DEFAULT        40

#define IERATE_1TO3          0 
#define IERATE_1TO2          1
#define IERATE_1TO15         2
#define IERATE_1TO1          3
#define IERATE_2TO1          4
#define IERATE_DEFAULT       IERATE_1TO15

#define INTER_BUTTON_DELAY      100000
#define INTER_STARTSTOP_DELAY   500000

#define leftPin           2
#define rightPin          3
#define upPin             4  
#define downPin           5
#define vTidalDownPin     14 //A0
#define vTidalUpPin       15 //A1
#define freqRespiDownPin  17 //A3
#define freqRespiUpPin    16 //A3
#define startStopPin      18 //A4 : I2C must NOT be used

// HPA timings
// int timeHighPriority[] = {150,120,150,120,150,400,150,120,150,2000};
#define dAlarmHP          150000
#define dAlarmHP_Spause   120000
#define dAlarmHP_Lpause   400000
#define dAlarmHP_LLpause  2000000

// LPA timings
// int timeMediumPriority[] = {250,120,250,120,250,2000};
#define dAlarmLP          250000
#define dAlarmLP_Spause   120000
#define dAlarmLP_LLpause  2000000

// LCD screen pins
#define pin_RS 13 //7
#define pin_EN 12 //6
#define pin_D4 11 //5
#define pin_D3 10 //4
#define pin_D2 9  //3
#define pin_D1 8  //2

#define alarmPin          7
#define powerSensePin     6
//#define gatePin           1
#define pressureSensePin  A5

#define PEAK_MA_SIZE    128 // power of 2, moving average length
#define PEAK_MASK   PEAK_MA_SIZE-1

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))

LiquidCrystal lcd(pin_RS, pin_EN, pin_D4, pin_D3, pin_D2, pin_D1);

unsigned long dIF =          100000;  //enter every 100ms
unsigned long dMot =         5000;
unsigned long dSense =       10000; 
unsigned long dButton =      10000;
unsigned long dInterButton = 0;

unsigned long tInIF;
unsigned long tInMot;
unsigned long tInSense; 
unsigned long tInButton;
unsigned long tLastButton;
unsigned long currTime;

unsigned long tInAlarm;
unsigned long tAbsAlarm;

// Tones for Alarm
int toneHighPriority[] = {262,0,440,0,349,0,440,0,349,0};
int toneMediumPriority[] = {262,0,440,0,349,0};

byte currParam = PARAM_PMAX;
byte screenType = UPDATE_STATE;
char screenBuffer[10];

byte pMaxVal = PMAX_DEFAULT;
byte fRespi = FRESPI_DEFAULT;
int  vTidal = VTIDAL_DEFAULT;
byte inExpRate = IERATE_DEFAULT;
byte startStopState;

int sPressure = 0;
int sPressureMax = 0;
int sPressureLastCycleMax = 0;
double sPressureV;
int sPressurePrev[PEAK_MA_SIZE] = {}; 
byte sPrevPtr = 0;
long sPressureSum; 
int sLowPower = 1;
bool wasPeak=false, isPeak=false;

/*
   MPX5010DP (http://www.farnell.com/datasheets/2097509.pdf)
   ---------------------------------------------------------
   Vout[V] = Vs * (0.09*p[kPa] + 0.04) with Vs = 5V
   -> p[kPa] = (Vout/Vs - 0.04) / 0.09

   Pin numbering:
   1: Vout (marked with a semi-hole in the pin)
   2: GND
   3: Vs
   4: V1 (?)
   5: V2 (?)
   6: Vex (?)

   P1: positive pressure port (side with part marking)
   P2: reference port
*/

void setup()
{
  startStopState = 0;
  pinMode(alarmPin,OUTPUT);
  //pinMode(gatePin,OUTPUT);
  pinMode(leftPin,INPUT_PULLUP);
  pinMode(rightPin,INPUT_PULLUP);
  pinMode(upPin,INPUT_PULLUP);
  pinMode(downPin,INPUT_PULLUP);
  pinMode(vTidalDownPin,INPUT_PULLUP);
  pinMode(vTidalUpPin,INPUT_PULLUP);
  pinMode(freqRespiDownPin,INPUT_PULLUP);
  pinMode(freqRespiUpPin,INPUT_PULLUP);
  
  pinMode(startStopPin,INPUT_PULLUP);
  pinMode(powerSensePin,INPUT_PULLUP);

  //digitalWrite(gatePin,HIGH);

  lcd.begin(16, 2);
  lcd.setCursor(0,0);
  lcd.print("Breath for Life");
  lcd.setCursor(0,1);
  lcd.print("V0.1");
  delay(4000);
  lcd.clear();
  lcd.setCursor(0, 0);

  tInIF = micros();
  tInMot = micros();
  tInSense = micros(); 
  tInButton = micros();
  tInAlarm = micros();
  tAbsAlarm = micros();
  set_interface();
  
}


void loop()
{
  currTime = micros();
  if(tInIF+dIF<currTime){ // && tInIF+dIF > tInIF ? overflowcheck
    tInIF = micros();
    // Set interface
    if (screenType == UPDATE_STATE){
      set_interface();
      set_pressure();
      // Until new state, update only pressure from now
      screenType = UPDATE_PRESSURE;  
    } else if (screenType == UPDATE_PRESSURE){
      set_pressure();
    } else if (screenType != NO_UPDATE){
      set_alert();
      set_pressure();
    }
    
  }
  if(tInMot+dMot<currTime){
    tInMot = micros();
    // Set motors
    // motorOn = 1 / 0 defines if motors are actve / not active
  }
  if(tInSense+dSense<currTime){
    tInSense = micros();
    // Read sensors
    // Pressure sensor value in int sPressure
    // to set alert PMAX : 
    // screenType = ALERT_MAX_PRESSURE;
    sPressureV = analogRead(pressureSensePin) * 0.0049;
    sPressure = int((sPressureV / 5.0 - 0.04) / 0.09 * 101.971); // Transfer function + Convert kPa to mmH20
    
    // Calculate moving average (sum)
    sPressureSum = sPressure + sPressureSum - sPressurePrev[sPrevPtr];
    // Update prev list
    sPressurePrev[sPrevPtr] = sPressure;
    sPrevPtr+=1;
    sPrevPtr = sPrevPtr & PEAK_MASK;
    // Calculate if sensed value is deviating PEAK_THR*stddev from the moving average over PEAK_MA_SIZE
    wasPeak = isPeak;
    isPeak = sPressure*PEAK_MA_SIZE;
    // Check if peak: record max
    sPressureMax = (isPeak && sPressureMax<sPressure) ? sPressure : sPressureMax;
    // If not Peak but was peak, update screen & reset sPressureMax
    if (!isPeak && wasPeak) {
      sPressureLastCycleMax = sPressureMax;
      sPressureMax = 0;
    }
    
    if (sPressure>=pMaxVal){
      screenType = ALERT_MAX_PRESSURE;
    }
    if (digitalRead(powerSensePin)==HIGH) {
      screenType = ALERT_POWER_SUPPLY;
      // TODO : Deactivate motors
    }
  } 

  if((tInButton+dButton+dInterButton)<currTime){
      
    tInButton = micros();
    
    // Read buttons, update state
    byte leftVal = digitalRead(leftPin);
    byte rightVal = digitalRead(rightPin);
    byte upVal = digitalRead(upPin);
    byte downVal = digitalRead(downPin);

    byte vTidalDownVal = digitalRead(vTidalDownPin);
    byte vTidalUpVal = digitalRead(vTidalUpPin);
    byte freqRespiDownVal = digitalRead(freqRespiDownPin);
    byte freqRespiUpVal = digitalRead(freqRespiUpPin);
    
    byte startStopVal = digitalRead(startStopPin);
    
    if (leftVal==LOW || rightVal==LOW || upVal==LOW || downVal==LOW || vTidalDownVal==LOW || vTidalUpVal==LOW || freqRespiDownVal==LOW || freqRespiUpVal == LOW || startStopVal == LOW){
      //Serial.println(currTime);
      
      // record time of last push
      tLastButton=micros();
      if (screenType>=ALERT_MAX_PRESSURE){
        // An alarm was triggered -> Don't record button pushes until error is present
        // i.e. : push any button once for reset, if error gone, return to normal, if not, button had no effect
        // reset alarm on any push of a button  
        //noTone(alarmPin);
        //screenType = UPDATE_STATE; // acts as reset alarm
        stop_alarm();
      } else {
        // update screen : either values or reset alarm
        screenType = UPDATE_STATE;
        // if a button was pressed, next one must wait 
        dInterButton = INTER_BUTTON_DELAY;
        if (startStopVal == LOW){
          startStopState = 1-startStopState;
          dInterButton = INTER_STARTSTOP_DELAY;
        }
        if(leftVal==LOW){
          //currParam = (currParam-1)&0x1;
          currParam = 1-currParam;
        } else if(rightVal==LOW){
          //currParam = (currParam+1)&0x1;
          currParam = 1-currParam;
        }
        // Next 2 params have dedicated buttons 
        if (freqRespiUpVal==LOW) {
          fRespi = (fRespi>=FRESPI_MAX) ? FRESPI_MAX : fRespi+FRESPI_STEP;   
        } else if (freqRespiDownVal==LOW) {
          fRespi = (fRespi<=FRESPI_MIN) ? FRESPI_MIN : fRespi-FRESPI_STEP;
        }
        if (vTidalUpVal==LOW) {
          vTidal = (vTidal>=VTIDAL_MAX) ? VTIDAL_MAX : vTidal+VTIDAL_STEP;
        } else if (vTidalDownVal==LOW) {
          vTidal = (vTidal<=VTIDAL_MIN) ? VTIDAL_MIN : vTidal-VTIDAL_STEP;
        }
        if ((currParam == PARAM_PMAX) && upVal==LOW) {
          pMaxVal = (pMaxVal>=PMAX_MAX) ? PMAX_MAX : pMaxVal+PMAX_STEP;
        } else if ((currParam == PARAM_PMAX) && downVal==LOW) {
          pMaxVal = (pMaxVal<=PMAX_MIN) ? PMAX_MIN : pMaxVal-PMAX_STEP;
        }
        if ((currParam == PARAM_IERATE) && upVal==LOW) {
          inExpRate = (inExpRate==IERATE_2TO1) ? IERATE_2TO1 : inExpRate+1;
        } else if ((currParam == PARAM_IERATE) && downVal==LOW) {
          inExpRate = (inExpRate==IERATE_1TO3) ? IERATE_1TO3 : inExpRate-1;
        }
      }
    } 
    
    else {
      // no button was pressed, reset inter button additional delay
      dInterButton = 0;
    }
  }

  // HPA
  if (screenType == ALERT_POWER_SUPPLY){

      update_tAbsAlarm();

      if (tAbsAlarm < dAlarmHP) {
        tone(alarmPin,toneHighPriority[0]);
      }
      if (tAbsAlarm > dAlarmHP && tAbsAlarm < (dAlarmHP + dAlarmHP_Spause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > dAlarmHP + dAlarmHP_Spause && tAbsAlarm < (2*dAlarmHP + dAlarmHP_Spause)) {
        tone(alarmPin,toneHighPriority[2]);
      }
      if (tAbsAlarm > (2*dAlarmHP + dAlarmHP_Spause) && tAbsAlarm < (2*dAlarmHP + 2*dAlarmHP_Spause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (2*dAlarmHP + 2*dAlarmHP_Spause) && tAbsAlarm < (3*dAlarmHP + 2*dAlarmHP_Spause)) {
        tone(alarmPin,toneHighPriority[4]);
      }
      if (tAbsAlarm > (3*dAlarmHP + 2*dAlarmHP_Spause) && tAbsAlarm < (3*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_Lpause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (3*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_Lpause) && tAbsAlarm < (4*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_Lpause)) {
        tone(alarmPin,toneHighPriority[6]);
      }
      if (tAbsAlarm > (4*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_Lpause) && tAbsAlarm < (4*dAlarmHP + 3*dAlarmHP_Spause + dAlarmHP_Lpause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (4*dAlarmHP + 3*dAlarmHP_Spause + dAlarmHP_Lpause) && tAbsAlarm < (5*dAlarmHP + 3*dAlarmHP_Spause + dAlarmHP_Lpause)) {
        tone(alarmPin,toneHighPriority[8]);
      }
      if (tAbsAlarm > (5*dAlarmHP + 3*dAlarmHP_Spause + dAlarmHP_Lpause) && tAbsAlarm < (5*dAlarmHP + 3*dAlarmHP_Spause +  dAlarmHP_Lpause + dAlarmHP_LLpause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (5*dAlarmHP + 3*dAlarmHP_Spause +  dAlarmHP_Lpause + dAlarmHP_LLpause)) {
        tAbsAlarm = 0;
      }
  }
  // LPA
  else if (screenType == ALERT_MAX_PRESSURE){

      update_tAbsAlarm();

      if (tAbsAlarm < dAlarmHP) {
        tone(alarmPin,toneMediumPriority[0]);
      }
      if (tAbsAlarm > dAlarmHP && tAbsAlarm < (dAlarmHP + dAlarmHP_Spause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > dAlarmHP + dAlarmHP_Spause && tAbsAlarm < (2*dAlarmHP + dAlarmHP_Spause)) {
        tone(alarmPin,toneMediumPriority[2]);
      }
      if (tAbsAlarm > (2*dAlarmHP + dAlarmHP_Spause) && tAbsAlarm < (2*dAlarmHP + 2*dAlarmHP_Spause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (2*dAlarmHP + 2*dAlarmHP_Spause) && tAbsAlarm < (3*dAlarmHP + 2*dAlarmHP_Spause)) {
        tone(alarmPin,toneMediumPriority[4]);
      }
      if (tAbsAlarm > (3*dAlarmHP + 2*dAlarmHP_Spause) && tAbsAlarm < (3*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_LLpause)) {
        noTone(alarmPin);
      }
      if (tAbsAlarm > (3*dAlarmHP + 2*dAlarmHP_Spause + dAlarmHP_LLpause)) {
        tAbsAlarm = 0;
      }
  }

}

void set_pressure() {
  lcd.setCursor(0,0);
  sprintf(screenBuffer," %2d %2d",(sPressure>=0)?sPressure:0,sPressureLastCycleMax);
  lcd.print(screenBuffer);
}

void set_interface(){
  lcd.clear();  
  lcd.setCursor(9,0);
  if (startStopState==1){
    lcd.print("SYS  ON");
  } else {
    lcd.print("SYS OFF");
  }
  lcd.setCursor(0,1);
  lcd.print(vTidal);
  lcd.setCursor(4,1);
  lcd.print(fRespi);

  if (currParam == PARAM_PMAX){
    lcd.setCursor(8,1);
    lcd.print("Pmax");
    lcd.setCursor(13,1);
    lcd.print(pMaxVal);  
  } else if (currParam == PARAM_IERATE){
    lcd.setCursor(8,1);
    lcd.print("I:E");
    lcd.setCursor(11,1);
    switch (inExpRate){
      case 0:
        lcd.print("  1:3");
      break;
      case 1:
        lcd.print("  1:2");
      break;
      case 2:
        lcd.print("1:1,5");
      break;
      case 3:
        lcd.print("  1:1");
      break;
      case 4:
        lcd.print("  2:1");
      break;
    }  
  }
}

void set_alert() {
  lcd.setCursor(9, 0);
  switch (screenType){
   case ALERT_MAX_PRESSURE:
    lcd.print("ErrPmax");
    break;
    case ALERT_POWER_SUPPLY:
    lcd.print("Err PWR");
    break;
   default :
    lcd.print("Err????");
    break;
  }
  //digitalWrite(gatePin,LOW);
}

// ALARM functions
void stop_alarm() {
  noTone(alarmPin);
  //digitalWrite(gatePin,HIGH);
  screenType = UPDATE_STATE;
}

void update_tAbsAlarm() {
  if (tAbsAlarm == 0) {
    tInAlarm = currTime;
    tAbsAlarm = micros() - currTime;
  }
  else{
    tAbsAlarm = micros() - tInAlarm;
  }
}
