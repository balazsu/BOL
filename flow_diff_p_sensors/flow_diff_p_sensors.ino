#include <LiquidCrystal.h>
#include <stdint.h>

// LCD screen pins
#define pin_RS 7
#define pin_EN 6
#define pin_D4 5
#define pin_D3 4
#define pin_D2 3
#define pin_D1 2

// LCD instantiation
LiquidCrystal lcd(pin_RS, pin_EN, pin_D4, pin_D3, pin_D2, pin_D1);

#define MPX5010DP_pin A5

/*
   MPXV5004DP (http://www.farnell.com/datasheets/2291564.pdf)
   ---------------------------------------------------------
   Unit conversion: 1kPa = 101.972mmH2O
   
   # Sensitivity
   1 V/kPa ~= 9.8mV/mmH2O
   
   # Pin numbering
   1: DNC (marked with a semi-hole in the pin)
   2: Vs = 5V 
   3: GND
   4: Vout

   # Pressure port identification (P1 > P2)
   - P1: positive pressure port (side with part marking)
   - P2: vacuum side

   # Transfer function
   Vout[V] = Vs * (0.2 * p[kPa] + 0.2) with Vs = 5V
           = p[kPa] + 1
   -> p[kPa] = Vout[V] - 1

   analogRead() precision is 10 bit: 1 = 4.9mV.

   Scale for mmH2O = 0.0049 * 101.971 = 0.4996579
   Offset for mmH2O = -1 * 101.971 = -101.971
   Make that integers by switching µmH2O (x1000):
   Scale for µmH2O = 499.6579 ~ 500
   Offset for µmH2O = -101971

   No overflow can happen on int32_t:
   - Max: 410029
   - Min: -101971
*/

int32_t vout;
// Pressure in µmH2O
int32_t p;
// Flow in mL/min
int32_t f;

// To be calibrated. From datasheet:
// Max sensitivity @60L/min = 1.6 mbar/(L/s)
// -> Scale = 1/1.6 = 0.625 (L/s)/mbar
// 1 L/s = 1000 mL/s = 60000 mL/min
// -> Scale = 0.625 * 60000 = 37500 (mL/min)/mbar
// 1 mBar = 10.1972 mmH2O
// -> Scale = 3677 (mL/min)/mmH2O
const int32_t scale_flow = 3677;
// Offset taken empirically
const int32_t offset_flow = -25739;

const int32_t scale_MPX5010DP = 500;
const int32_t offset_MPX5010DP = -101971;

uint32_t curr_time;
uint32_t last_measure_p;
uint32_t last_display_p;

// In µs
const uint32_t measure_period = 50000;  // 50 ms
const uint32_t display_period = 250000; // 250 ms

void read_MPX5010DP() {
  vout = analogRead(MPX5010DP_pin);
  p = scale_MPX5010DP * vout + offset_MPX5010DP;
  f = scale_flow * (p/1000) + offset_flow;

  // Send to computer via Serial port (TODO: remove)
//  Serial.print("Pressure difference [µmH2O]: ");
//  Serial.print(p);
//  Serial.print("\t");
//  Serial.print("Flow [mL/min]: ");
  Serial.print(f);
  Serial.println();
}

void poll_MPX5010DP(uint32_t curr_time) {
  if (curr_time - last_measure_p > measure_period) {
    read_MPX5010DP();
    last_measure_p += measure_period;
  }
}

void display_p() {
  static char buffer [20];
  lcd.setCursor(0, 0);
  sprintf(buffer, "p: %4d mmH2O", int(p/1000));
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  sprintf(buffer, "f: %3d L/min", int(f/1000));
  lcd.print(buffer);
}

void display_p_timed(uint32_t curr_time) {
  if (curr_time - last_display_p > display_period) {
    display_p();
    last_display_p += display_period;
  }
}

void setup() {
  pinMode(MPX5010DP_pin, INPUT);

  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.clear();

  // Initial measurement and display
  read_MPX5010DP();
  last_measure_p = micros();
  display_p();
  last_display_p = micros();
}

void loop() {
  curr_time = micros();

  poll_MPX5010DP(curr_time);
  display_p_timed(curr_time);
}
