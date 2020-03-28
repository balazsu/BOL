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
   MPX5010DP (http://www.farnell.com/datasheets/2097509.pdf)
   ---------------------------------------------------------
   # Pin numbering
   1: Vout (marked with a semi-hole in the pin)
   2: GND
   3: Vs
   4: V1 (?)
   5: V2 (?)
   6: Vex (?)

   P1: positive pressure port (side with part marking)
   P2: reference port

   # Transfer function
   Vout[V] = Vs * (0.09*p[kPa] + 0.04) with Vs = 5V
   -> p[kPa] = (Vout/Vs - 0.04) / 0.09

   analogRead() precision is 10 bit: 1 = 4.9mV.

   Unit conversion: 1kPa = 101.972mmH2O

   Scale for mmH2O = 0.0049/5.0/ 0.09 * 101.971 = 1.110351
   Offset for mmH2O = -0.04 / 0.09 * 101.971 = -45.320444
   Make that integers by switching µmH2O:
   Scale for µmH2O = 1110
   Offset for µmH2O = -45320

   No overflow can happen on int32_t:
   - Max: 1090210
   - Min: -45320
*/

int32_t vout;
// Pressure is thus in µmH2O
int32_t p;

const int32_t scale_MPX5010DP = 1110;
const int32_t offset_MPX5010DP = -45320;

uint32_t curr_time;
uint32_t last_measure_p;
uint32_t last_display_p;

// In µs
const uint32_t measure_period = 50000;  // 50 ms
const uint32_t display_period = 250000; // 250 ms

void read_MPX5010DP() {
  vout = analogRead(MPX5010DP_pin);
  p = scale_MPX5010DP * vout + offset_MPX5010DP;

  // Send to computer via Serial port (TODO: remove)
  Serial.println(p);
}

void poll_MPX5010DP(uint32_t curr_time) {
  if (curr_time - last_measure_p > measure_period) {
    read_MPX5010DP();
    last_measure_p += measure_period;
  }
}

void display_p() {
  static char buffer [20];
  sprintf(buffer, "p: %2d cmH2O", int(p/10000));
  lcd.setCursor(0, 0);
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
