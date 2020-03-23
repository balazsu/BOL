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

/*
  HSC S AA N 001PD AA 5 (http://www.farnell.com/datasheets/2145070.pdf)
  ---------------------------------------------------------------------
  Transfer function: Vout[V] = (0.8 * Vs)/(Pmax - Pmin) * (p - Pmin) + 0.10*Vs
  with Vs = 5V, Pmin = -1 Psi and Pmax = +1 Psi
  -> p[Psi] = ((Vout - 0.10*Vs) * (Pmax-Pmin))/(0.8*Vs) + Pmin
            = (Vout - 0.50)/2.0 - 1.0

  Pin numbering:
  1: NC (left on the side with part marking)
  2: Vs
  3: Vout
  4: GND

  P1: positive pressure port (side with part marking)
  P2: reference port
*/

#define sensor_pin1 A5
#define sensor_pin2 A0

// Voltage readout
double vout1 = 0;
double vout2 = 0;

// Pressure measures
double p1 = 0;
double p2 = 0;

void setup() {
  pinMode(sensor_pin1, INPUT);
  pinMode(sensor_pin2, INPUT);
  
  Serial.begin(9600);

  lcd.begin(16, 2);
  lcd.clear();
}

void loop() {
  // 5V on 1024 values, 1 = 4.9mV
  vout1 = analogRead(sensor_pin1) * 0.0049;
  vout2 = analogRead(sensor_pin2) * 0.0049;

  // MPX5010DP
  p1 = (vout1 / 5.0 - 0.04) / 0.09;
  p1 *= 101.971; // Convert kPa to mmH20

  String p1_str = String(p1);
  Serial.println(p1_str);
  //Serial.print("Pressure #1 [mmH20] = " );
  //Serial.println(p1);

  // HSCSAAN001PDAA5
  int Pmax = 1;
  int Pmin = -1;
  p2 = (vout2 - 0.50) / 2.0 - 1.0;
  p2 *= 703.069; // Convert Psi to mmH20

  //Serial.print("Pressure #2 [mmH20] = " );
  //Serial.println(p2);

  // Printing on the LCD screen
  char buffer [20];
  sprintf(buffer, "MPX: %4d  mmH2O", int(p1));
  lcd.setCursor(0, 0);
  lcd.print(buffer);

  sprintf(buffer, "HSC: %4d  mmH2O", int(p2));
  lcd.setCursor(0, 1);
  lcd.print(buffer);

  // One measurement roughly every 50ms
  delay(50);
}
