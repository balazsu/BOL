#include <LiquidCrystal.h>

// LCD screen pins
#define pin_RS 7
#define pin_EN 6
#define pin_D4 5
#define pin_D3 4
#define pin_D2 3
#define pin_D1 2

// LCD instantiation
LiquidCrystal lcd(pin_RS, pin_EN, pin_D4, pin_D3, pin_D2, pin_D1);

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

#define sensor_pin A5

// Voltage readout
double vout;

// Pressure measures
double p;

unsigned long curr_time;
unsigned long last_measure_p;
unsigned long last_display_p;

// In µs
const unsigned long measure_period = 50000;   // 50 ms
const unsigned long display_period = 250000; // 250 ms 

void measure_p() {
  vout = analogRead(sensor_pin) * 0.0049;
  
  // Invert transfer function
  p = (vout / 5.0 - 0.04) / 0.09;
  // Convert kPa to cmH20
  p *= 101.971 / 10.0;

  // Send to serial for real-time plot
  String p_str = String(p);
  Serial.println(p_str);
}

void measure_p_timed(unsigned long curr_time) {
  if(curr_time - last_measure_p > measure_period) {
    measure_p();
    last_measure_p += measure_period;
  }
}

void display_p() {
    static char buffer [20];
    sprintf(buffer, "MPX: %2d cmH2O", int(p));
    lcd.setCursor(0, 0);
    lcd.print(buffer);
}

void display_p_timed(unsigned long curr_time) {
  if (curr_time - last_display_p > display_period) {
    display_p();
    last_display_p += display_period;  
  }  
}

void setup() {
  pinMode(sensor_pin, INPUT);
  
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.clear();

  // Initial measurement and display
  measure_p();
  last_measure_p = micros();
  display_p();
  last_display_p = micros();
}

void loop() {
  curr_time = micros();

  measure_p_timed(curr_time);
  display_p_timed(curr_time);
}
