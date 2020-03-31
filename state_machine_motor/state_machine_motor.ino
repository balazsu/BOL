#define PIN_EN 3
#define PIN_DIR 4
#define PIN_STEP 5
#define PIN_START 2
#define PIN_PAUSE 19
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
const uint32_t n_steps = 500;                    // total motor steps

uint32_t plateau_pulses;
uint32_t insp_pulses;
uint32_t exp_pulses;

const uint32_t pulses_home_final = m_steps * 10;   // amount of step to properly set the initial low point
const uint32_t pulses_full_range = 1000 * m_steps;   // total µsteps/pulses needed

// Pulse periods
unsigned long T_pulse_insp;
unsigned long T_pulse_plateau;
unsigned long T_pulse_exp = 1000000/(m_steps*500);
uint32_t exp_wait_time;
uint32_t T_pulse_home = T_home/pulses_full_range;


uint32_t curr_pulses;

uint32_t pulses_nb[] = {350, 350, 350, 350, 400, 450, 500, 550, 600, 650, 700};
uint32_t pulses_offset = 0;
uint32_t nb_breathings = sizeof(pulses_nb)/sizeof(pulses_nb[0]);

void set_breath_params(uint32_t amplitude) {
    plateau_pulses = amplitude/30;
    insp_pulses = amplitude - plateau_pulses;
    exp_pulses = amplitude;
    uint32_t t_plateau = Ti/10;
    uint32_t t_insp = Ti - t_plateau;
    T_pulse_insp = t_insp/insp_pulses;
    T_pulse_plateau = t_plateau/plateau_pulses;
    exp_wait_time = Te - T_pulse_exp*exp_pulses;
}

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

void stop_motor(void) {
    rem_steps = 0;
}
///////////////////////////////////////


//////////// High-level motor control //////////////

typedef enum {
    INIT,
    INSP,
    PLATEAU,
    EXP_MOVE,
    EXP_WAIT,
    WAIT,
    HOME_FIRST,
    HOME_SEC,
    HOME_UP,
    HOME_FINAL,
    PAUSE,
    FAIL
    } motor_state;
motor_state motor_hl_st;

uint32_t exp_end_time;

const motor_state post_home_st = EXP_WAIT; // will start INSP

void init_motor_hl(void) {
    motor_hl_st = INIT;
    // end-couse switches
    pinMode(PIN_ECS_UP, INPUT_PULLUP);
    pinMode(PIN_ECS_DOWN, INPUT_PULLUP);
    
    pinMode(PIN_PAUSE, INPUT_PULLUP);
        pinMode(PIN_START, INPUT_PULLUP);
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
                move_motor(plateau_pulses, T_pulse_plateau, INSP_DIR, curr_time);
                motor_hl_st = PLATEAU;
                PRINTLN("start plateau");
            }
            break;

        case PLATEAU:
            if (!motor_moving() || digitalRead(PIN_ECS_DOWN) == HIGH) {
                move_motor(exp_pulses, T_pulse_exp, EXP_DIR, curr_time);
                motor_hl_st = EXP_MOVE;
                PRINTLN("start exp_move");
            }
            break;

        case EXP_MOVE:
            if (!motor_moving() || digitalRead(PIN_ECS_UP) == HIGH) {
                if (pulses_offset >= nb_breathings) {
                    motor_hl_st = FAIL;
                } else {
                    stop_motor();
                    disable_motor();
                    motor_hl_st = EXP_WAIT;
                    exp_end_time = curr_time + exp_wait_time;
                    PRINTLN("start exp_wait");
                }
            }
            break;

        case EXP_WAIT:
            if (curr_time >= exp_end_time) {
                set_breath_params(m_steps*pulses_nb[pulses_offset]);
                pulses_offset += 1;
                enable_motor();
                move_motor(insp_pulses, T_pulse_insp, INSP_DIR, curr_time);
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
