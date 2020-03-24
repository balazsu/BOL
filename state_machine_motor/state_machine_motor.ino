#define PIN_EN 5
#define PIN_DIR 6
#define PIN_STEP 7
#define PIN_START 8

// TO DO: check correct directions for inspiration/expirations phases
#define INSP_DIR HIGH
#define EXP_DIR LOW

enum Motor {INIT, INSP_HIGH, INSP_LOW, EXP_HIGH, EXP_LOW, WAIT};

// Breath parameters
const unsigned long TCT = 2.0 * 1000000;    // Total cycle time [µs]
const unsigned long Ti = 1.5 * 1000000;     // Inspiration time [µs]
const unsigned long Te = TCT - Ti;          // Expiration time [µs]

// Motor parameters
const int m_steps = 16;                     // µsteps per step
const int n_steps = 400;                    // total motor steps
const int tot_pulses = n_steps * m_steps;   // total µsteps/pulses needed

// Pulse periods
unsigned long T_pulse_insp = Ti/tot_pulses;
unsigned long T_pulse_exp = Te/tot_pulses;  

void update_motor_state(unsigned long curr_time) {
  static uint8_t motor_state = INIT;      // initial motor state
  static unsigned long start_half_pulse;  // last time pulse state changed
  static int rem_pulses;                  // remaining pulses to do
  
  switch (motor_state) {
    case INIT:
      if (digitalRead(PIN_START) == HIGH) {
        digitalWrite(PIN_EN, HIGH);
        digitalWrite(PIN_DIR, INSP_DIR); 
        digitalWrite(PIN_STEP, HIGH);
        
        motor_state = INSP_HIGH;
        start_half_pulse = curr_time;
        rem_pulses = tot_pulses;
      }
      break;
      
    case INSP_HIGH:
      if (curr_time - start_half_pulse >= T_pulse_insp/2) {
        digitalWrite(PIN_STEP, LOW);
        
        motor_state = INSP_LOW;
        start_half_pulse += T_pulse_insp/2;  
      }
      break;

    case INSP_LOW:
      if (curr_time - start_half_pulse >= T_pulse_insp/2) {   
        rem_pulses--;

        digitalWrite(PIN_STEP, HIGH);
        start_half_pulse += T_pulse_insp/2;

        if (rem_pulses > 0) {
          motor_state = INSP_HIGH;
        } else {
          digitalWrite(PIN_DIR, EXP_DIR); 
          motor_state = EXP_HIGH;
          rem_pulses = tot_pulses;
        }
      }
      break;

    case EXP_HIGH:
      if (curr_time - start_half_pulse >= T_pulse_exp/2) {
        digitalWrite(PIN_STEP, LOW);
        
        motor_state = EXP_LOW;
        start_half_pulse += T_pulse_exp/2;  
      }
      break;

    case EXP_LOW:
      if (curr_time - start_half_pulse >= T_pulse_exp/2) {
        rem_pulses--;

        digitalWrite(PIN_STEP, HIGH);
        start_half_pulse += T_pulse_exp/2;

        if (rem_pulses > 0) {
          motor_state = EXP_HIGH;
        } else {
          digitalWrite(PIN_DIR, INSP_DIR); 
          motor_state = INSP_HIGH;
          rem_pulses = tot_pulses;
        }
      }
      break;

    case WAIT:
      // To be implemented, waiting phase at the end of the expiration phase to spare the motor a bit
      break;
  }
}

void setup() {
  // Nothing to do here
}

void loop() {
  unsigned long curr_time = micros();
  
  update_motor_state(curr_time);
}
