////////////////////////////////////////////////////////////////////////////////
// Name: motorControl_pwmHard.ino
//
// Descr: First sketch about motor control based on hard PWM and hard Counter
//        Targe is Arduino Mega
//        Not yet working with BoL motor
//        Many improvements are possible
//        Not yet fully tested
//
//        Add a 100 ohms damping resistor between PWM output and Step input of Motor drive
//
//
// Author:  Guerric Meurice de Dormale <gm@bitandbyte.io>
//          Gaetan Cassiers <gaetan.cassiers@uclouvain.be>
//
//
////////////////////////////////////////////////////////////////////////////////


#include <limits.h>

#define MOTORCTRL_DIR_FORWARD 0
#define MOTORCTRL_DIR_BACKWARD 1
#define MOTORCTRL_DRIVE_ENBL_TRUE 0
#define MOTORCTRL_DRIVE_ENBL_FALSE 1

#define MOTORCTRL_PWM_RUN_VALUE _BV(CS12)
#define MOTORCTRL_PWM_OVF_IRQ_CFG _BV(TOIE1) 
#define MOTORCTRL_PWM_OVF_IRQ TIMER1_OVF_vect

#define MOTORCTRL_STEP_CNT_IRQ TIMER5_COMPA_vect

const byte MOTORCTRL_DRIVE_DIR_PIN = 22;
const byte MOTORCTRL_DRIVE_STEP_PIN = 12; // PWM1 outputB
const byte MOTORCTRL_DRIVE_ENBL_PIN = 24;

const byte MOTORCTRL_STEP_CNT_PIN = 47; // CNT5 input
const byte MOTORCTRL_STEP_CNT_OUT_DBG_PIN = 25;
const byte MOTORCTRL_STEP_CNT_OUT_DBG_PIN2 = 27;

const unsigned int TIM1_FREQ_DIV_FACTOR = 31250;

// We use signed number for position to account for potential extra steps while being at homing position
static volatile long motor_position = 0;
static volatile long motor_target_position = 0;
static volatile byte motor_direction = 0;
static volatile byte motor_inmotion = 0;
static volatile int  motor_step_cnt_incr = 0; 

void setup_pwm1()
{
  cli();//stop interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  // WGM mode 9 (PWM, Phase and Frequency Correct - OCRn)
  // pre-scaler: div-256
  // OCR1A fixes the period
  // OCR1B fixes duty cycle
  // 3 is the minimum period for the timer
  // setting OCR1B = OCR1A guarantees 0 output
  // setting a low period minimizes turn-on latency.
  OCR1A = 3;
  OCR1B = UINT_MAX;
  // WGM mode 9 (PWM, Phase and Frequency Correct - OCRn)
  // pre-scaler: div-256
  // OCR1A fixes the period
  // OCR1B fixes duty cycle
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0) | _BV(WGM10);
  TCCR1B |= _BV(WGM13);

  // Start
  TCCR1B |= MOTORCTRL_PWM_RUN_VALUE;

  // Disable interrupts
  TIMSK1 = 0;

  pinMode(MOTORCTRL_DRIVE_STEP_PIN, OUTPUT);

  sei();//allow interrupts
}


void disable_cnt5_irq()
{
  TIMSK5 &= ~ _BV(OCIE5A);
}

void enable_cnt5_irq()
{
  TIMSK5 |= _BV(OCIE5A);
}

void setup_ext_cnt5()
{
  cli();//stop interrupts

  // Counter for PWM output
  pinMode(MOTORCTRL_STEP_CNT_PIN, INPUT);
  // WGM mode 4 (CTC - OCRn)
  // External rising-edge clock
  // ? add ICNC5 ?
  TCCR5A = 0;
  TCCR5B = _BV(CS52) | _BV(CS51) | _BV(CS50) | _BV(WGM52);
  OCR5A = TIM1_FREQ_DIV_FACTOR; // By default
  TIMSK5 = _BV(OCIE5A);

  sei();//allow interrupts
}


void setup()
{

  setup_pwm1();
  setup_ext_cnt5();

  pinMode(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, OUTPUT);
  digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 0);
  pinMode(MOTORCTRL_STEP_CNT_OUT_DBG_PIN2, OUTPUT);
  digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN2, 0);
  
  // TMC Dir pin
  pinMode(MOTORCTRL_DRIVE_DIR_PIN, OUTPUT);
  // TMC enable pin
  pinMode(MOTORCTRL_DRIVE_ENBL_PIN, OUTPUT);

  Serial.begin(9600);
  while (! Serial); // Wait untilSerial is ready
  Serial.println("Test Monitor startup...");
}



// Interrupt callback for Timer5 (step counter)
ISR(MOTORCTRL_STEP_CNT_IRQ)
{
  irq_step_count_clbk();
}

void irq_step_count_clbk()
{
  long remaining_distance;

  if (motor_direction == MOTORCTRL_DIR_FORWARD)
  {
    // Before interrupt, counter is reset
    // if execution is not fast enough, take remaining steps (if any)
    motor_position += get_cnt5();
    motor_position += motor_step_cnt_incr;

    if (motor_position >= motor_target_position)
    {
      stop_pwm1();
    }
    else
    {
      remaining_distance = motor_target_position - motor_position;
      if (remaining_distance <= UINT_MAX)
      {
        set_threshold_cnt5(remaining_distance);
      }
      else
      {
        set_threshold_cnt5(UINT_MAX);
      }
    }
  }
  else
  {
    motor_position -= get_cnt5();
    motor_position -= motor_step_cnt_incr;
    if (motor_position <= motor_target_position)
    {
      stop_pwm1();
    }
    else
    {
      remaining_distance = motor_position - motor_target_position;
      if (remaining_distance <= UINT_MAX)
      {
        set_threshold_cnt5(remaining_distance);
      }
      else
      {
        set_threshold_cnt5(UINT_MAX);
      }
    }
  }
}

// Maximum input freq is TIM1_FREQ_DIV_FACTOR/2
void set_freq_pwm1(const unsigned int freq)
{
  unsigned int timer1_period;
  timer1_period = TIM1_FREQ_DIV_FACTOR / freq;
  // The updates to these values are sampled when the counter reaches zero,
  // hence the following instructions should happen atomically most of the
  // time.

  cli();//stop interrupts
  // Stop counter
  TCCR1B &= ~MOTORCTRL_PWM_RUN_VALUE;
  // Update TOP values
  OCR1A = timer1_period;
  OCR1B = timer1_period/2;
  // Start counter
  TCCR1B |= MOTORCTRL_PWM_RUN_VALUE;
  sei();//allow interrupts
}

// Maximum input freq is TIM1_FREQ_DIV_FACTOR/2
void stop_pwm1()
{
  cli();//stop interrupts
  // 3 is the minimum period for the timer
  // setting OCR1B = OCR1A guarantees 0 output
  // setting a low period minimizes turn-on latency.
  // We first set OCR1B to UINT_MAX to guarantee absence of glitches.

  // Stop counter
  TCCR1B &= ~MOTORCTRL_PWM_RUN_VALUE;
  // Update TOP values to minimum
  OCR1B = 3;
  OCR1A = 3;  
  // Enable overflow interrupt (end of operations)
  TIFR1 |= _BV(TOV1); // Clear pending interrupt flag (if any)
  TIMSK1 |= MOTORCTRL_PWM_OVF_IRQ_CFG;
  // Start counter
  TCCR1B |= MOTORCTRL_PWM_RUN_VALUE;
  sei();//allow interrupts
}

// Interrupt callback for Timer1 (PWM overflow)
ISR(MOTORCTRL_PWM_OVF_IRQ)
{
  //digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 1);
  // Disable overflow interrupt
  TIMSK1 &= ~ MOTORCTRL_PWM_OVF_IRQ_CFG;
  // Report that motor is not in motion anymore
  motor_inmotion = 0;
}

// Maximum input threshold is UINT_MAX
void set_threshold_cnt5(const unsigned int thresh)
{
  motor_step_cnt_incr = thresh;
  // Counter interrupt is off by one, so need to adapt threshold value
  OCR5A = thresh-1;
}

void reset_cnt5()
{
  TCNT5 = 0;
}

unsigned int get_cnt5()
{
  return TCNT5;
}

void set_motor_goto_position(const unsigned long position, const unsigned int speed)
{
  long remaining_distance;

  if (motor_inmotion == 0)
  {
    reset_cnt5();

    motor_target_position = position;
    remaining_distance = motor_target_position - motor_position;
    if (remaining_distance >= 0)
    {
      motor_direction = MOTORCTRL_DIR_FORWARD;
      if (remaining_distance == 1) // Special case: counter config is problematic in that case
      {
        motor_target_position += 1;
        remaining_distance = 2; // Will go a little bit too far
      }
    }
    else
    {
      motor_direction = MOTORCTRL_DIR_BACKWARD;
      remaining_distance = -remaining_distance;
      if (remaining_distance == 1) // Special case: counter config is problematic in that case
      {
        motor_target_position -= 1;
        remaining_distance = 2; // Will go a little bit too far
      }
    }

    digitalWrite(MOTORCTRL_DRIVE_DIR_PIN, motor_direction);

    if (remaining_distance > 0)
    {
      if (remaining_distance <= UINT_MAX)
      {
        set_threshold_cnt5(remaining_distance);
      }
      else
      {
        set_threshold_cnt5(UINT_MAX);
      }

      digitalWrite(MOTORCTRL_DRIVE_ENBL_PIN, MOTORCTRL_DRIVE_ENBL_TRUE);
      motor_inmotion = 1;
      set_freq_pwm1(speed);
      // Verify delay between set_freq_pwm1() and real start of PWM.
      //digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 1);
    }
    else
    {
      digitalWrite(MOTORCTRL_DRIVE_ENBL_PIN, MOTORCTRL_DRIVE_ENBL_FALSE);
    }
  }
}

void loop()
{

  unsigned int motor_speed = 1000; // Max TIM1_FREQ_DIV_FACTOR/2
  unsigned long motor_target_pos = 10;
  unsigned long motor_target_pos_limit = 20;

  while(1) {

    set_motor_goto_position(motor_target_pos, motor_speed);
    motor_target_pos +=4;
    if (motor_target_pos > motor_target_pos_limit)
    {
      motor_target_pos = 0;
    }

    while (motor_inmotion)
    {
        delay(100);
    }
  }
}
