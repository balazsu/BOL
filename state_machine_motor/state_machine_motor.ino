#define PIN_EN 5
#define PIN_DIR 6
#define PIN_STEP 7
#define PIN_START 2
#define PIN_PAUSE 3
#define PIN_ECS_UP 8
#define PIN_ECS_DOWN 9

// TO DO: check correct directions for inspiration/expirations phases
#define INSP_DIR HIGH
#define EXP_DIR LOW

#if 1
#define PRINT(x) do { Serial.print(x); } while (0);
#define PRINTLN(x) do { Serial.println(x); } while (0);
#else
#define PRINT(x) do { } while (0);
#define PRINTLN(x) do { } while (0);
#endif

#define ENABLE_MOTOR 1


// Breath parameters
const unsigned long TCT = 5 * 1000000;    // Total cycle time [µs]
const unsigned long Ti = 2.5 * 1000000;     // Inspiration time [µs]

const unsigned long Te = TCT - Ti;          // Expiration time [µs]
const uint32_t T_home = 2*TCT;

// Motor parameters
const uint32_t m_steps = 16;                     // µsteps per step
const uint32_t n_steps = 100;                    // total motor steps
const uint32_t tot_pulses = n_steps * m_steps;   // total µsteps/pulses needed
const uint32_t pulses_home_final = m_steps * 50;   // amount of step to properly set the initial low point

const uint32_t pulses_full_range = 800 * m_steps;   // total µsteps/pulses needed

// Pulse periods
unsigned long T_pulse_insp = Ti/tot_pulses;
unsigned long T_pulse_exp = Te/tot_pulses;  
uint32_t T_pulse_home = T_home/pulses_full_range;


//////////// Low-level motor control //////////////
uint32_t rem_steps;
uint32_t half_step_dur; // us
uint8_t step_level;
uint32_t next_half_step;

void poll_motor_ll(uint32_t curr_time) {
    if (curr_time >= next_half_step) {
        if (step_level == HIGH) {
            digitalWrite(PIN_STEP, LOW);
            step_level = LOW;
            next_half_step += half_step_dur;
        } else if (rem_steps > 0) {
            digitalWrite(PIN_STEP, HIGH);
            step_level = HIGH;
            next_half_step += half_step_dur;
            rem_steps -= 1;
        }
    }
}

void init_motor_ll() {
    // Global variables
    rem_steps = 0;
    half_step_dur = 0;
    step_level = 0;
    next_half_step = 0;
    // Setup pins
    pinMode(PIN_EN, OUTPUT);
    pinMode(PIN_DIR, OUTPUT);
    pinMode(PIN_STEP, OUTPUT);
    disable_motor();
    digitalWrite(PIN_DIR,LOW);
    digitalWrite(PIN_STEP,LOW);
}

void enable_motor(void) {
#if ENABLE_MOTOR
  digitalWrite(PIN_EN, LOW);
#endif
}

void disable_motor(void) {
  digitalWrite(PIN_EN, HIGH);
}

void move_motor(uint32_t n_steps, uint32_t step_dur, uint32_t dir, uint32_t curr_time) {
  digitalWrite(PIN_DIR, dir);
  rem_steps = n_steps;
  half_step_dur = step_dur/2;
  next_half_step = curr_time;
}

bool motor_moving() {
    return rem_steps != 0;
}
///////////////////////////////////////


//////////// High-level motor control //////////////

typedef enum {INIT, INSP, EXP, WAIT, HOME_FIRST, HOME_SEC, HOME_UP, HOME_FINAL, PAUSE, FAIL} motor_state;
motor_state motor_hl_st;

const motor_state post_home_st = EXP; // will start INSP

void init_motor_hl(void) {
    motor_hl_st = INIT;
    // end-couse switches
    pinMode(PIN_ECS_UP, INPUT_PULLUP);
    pinMode(PIN_ECS_DOWN, INPUT_PULLUP);
}

void poll_motor_hl(uint32_t curr_time) {
    if (digitalRead(PIN_PAUSE) == LOW) {
        motor_hl_st = INIT;
    }
    switch (motor_hl_st) {
        case INIT:
            if (digitalRead(PIN_START) == LOW) {
                PRINTLN("starting");
                enable_motor();
                motor_hl_st = HOME_FIRST;
            }
            break;

        case HOME_FIRST:
                move_motor(2*pulses_full_range, T_pulse_home, INSP_DIR, curr_time);
                motor_hl_st = HOME_SEC;
                break;

        case HOME_SEC:
            if (!motor_moving()) {
                motor_hl_st = FAIL;
            } else if (digitalRead(PIN_ECS_DOWN) == HIGH) {
                // rope in correct state for down -> moving up
                move_motor(pulses_full_range, T_pulse_home, EXP_DIR, curr_time);
                motor_hl_st = HOME_UP;
            } else if (digitalRead(PIN_ECS_UP) == HIGH) {
                // rope unrolled, wait for it to be rolled
            }
            break;

        case HOME_UP:
            if (!motor_moving()) {
                motor_hl_st = FAIL;
            } else if (digitalRead(PIN_ECS_UP) == HIGH) {
                motor_hl_st = HOME_FINAL;
                move_motor(pulses_home_final, T_pulse_home, INSP_DIR, curr_time);
            }
            break;
        case HOME_FINAL:
            if ((digitalRead(PIN_ECS_UP) == LOW) && !motor_moving()){
              motor_hl_st = post_home_st;
            }
            break;

        case INSP:
            if (!motor_moving() || digitalRead(PIN_ECS_DOWN) == HIGH) {
                move_motor(tot_pulses, T_pulse_exp, EXP_DIR, curr_time);
                motor_hl_st = EXP;
                PRINTLN("start exp");
            }
            break;

        case EXP:
            if (!motor_moving() || digitalRead(PIN_ECS_UP) == HIGH) {
                move_motor(tot_pulses, T_pulse_insp, INSP_DIR, curr_time);
                motor_hl_st = INSP;
                PRINTLN("start insp");
            }
            break;

        case WAIT:
            // To be implemented, waiting phase at the end of the expiration phase to spare the motor a bit
            break;

        case PAUSE:
        // to be implemented
            break;

        case FAIL:
        // to be implemented
            break;
    }
}


void setup() {
    Serial.begin(9600);
    init_motor_ll();
    init_motor_hl();
    PRINTLN("setup");
}

void loop() {
//        PRINTLN("pcds");
  unsigned long curr_time = micros();
  
  poll_motor_ll(curr_time);
  poll_motor_hl(curr_time);
  
}
