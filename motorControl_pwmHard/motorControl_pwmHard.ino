////////////////////////////////////////////////////////////////////////////////
// Name: motorControl_pwmHard.ino
//
// Descr: First sketch about motor control based on hard PWM and hard Counter
//        Targe is Arduino Mega
//        Not yet working with BoL motor
//        Many improvements are possible
//        Not yet fully tested
//
//
// Author:  Guerric Meurice de Dormale <gm@bitandbyte.io>
//
//
////////////////////////////////////////////////////////////////////////////////


#include <limits.h>

#define MOTORCTRL_DIR_FORWARD 0
#define MOTORCTRL_DIR_BACKWARD 1
#define MOTORCTRL_DRIVE_ENBL_TRUE 0
#define MOTORCTRL_DRIVE_ENBL_FALSE 1

//#define MOTORCTRL_STEP_CNT_IRQ TIMER5_CAPT_vect

const byte MOTORCTRL_DRIVE_DIR_PIN = 22;
const byte MOTORCTRL_DRIVE_STEP_PIN = 11; // PWM1 outputA
const byte MOTORCTRL_DRIVE_ENBL_PIN = 24;

const byte MOTORCTRL_STEP_CNT_PIN = 47; // CNT5 input
const byte MOTORCTRL_STEP_CNT_OUT_DBG_PIN = 25;

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

  // PWM
  pinMode(MOTORCTRL_DRIVE_STEP_PIN, OUTPUT); // A
  // WGM mode 8 (PWM, Phase and Frequency Correct - ICRn)
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(COM1B0);
  // pre-scaler: div-256
  TCCR1B = _BV(CS12) | _BV(WGM13);
  ICR1 = 0;
  OCR1A = 0;

  sei();//allow interrupts
}



void setup_ext_cnt5()
{
  cli();//stop interrupts

  // Counter for PWM output
  pinMode(MOTORCTRL_STEP_CNT_PIN, INPUT);
  // WGM mode 12 (CTC - ICRn)
  TCCR5A = 0;
  // External falling-edge clock
  // ? add ICNC5 ?
  TCCR5B = _BV(CS52) | _BV(CS51) | _BV(WGM53) | _BV(WGM52);
  ICR5 = TIM1_FREQ_DIV_FACTOR; // By default
  TIMSK5 = _BV(ICIE5);

  sei();//allow interrupts
}


void setup()
{

  setup_pwm1();
  setup_ext_cnt5();

  //attachInterrupt(ICF5, step_cnt_cbf, RISING);
  pinMode(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, OUTPUT);
  digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 0);

  // TMC Dir pin
  pinMode(MOTORCTRL_DRIVE_DIR_PIN, OUTPUT);
  // TMC enable pin
  pinMode(MOTORCTRL_DRIVE_ENBL_PIN, OUTPUT);

  Serial.begin(9600);
  while (! Serial); // Wait untilSerial is ready
  Serial.println("Test Monitor startup...");
}



// Interrupt callback for Timer5 (step counter)
ISR(TIMER5_CAPT_vect)
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
      motor_inmotion = 0;
      digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 1);
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
    if (motor_position <= motor_target_position)
    {
      stop_pwm1();
      motor_inmotion = 0;
      digitalWrite(MOTORCTRL_STEP_CNT_OUT_DBG_PIN, 1);
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
  // Should wait for prescaler allignment (to avoid generating a very short first pulse)
  unsigned int timer1_period;
  timer1_period = TIM1_FREQ_DIV_FACTOR / freq;
  ICR1 = timer1_period;
  OCR1A = timer1_period/2;
}

// Maximum input freq is TIM1_FREQ_DIV_FACTOR/2
void stop_pwm1()
{
  cli();//stop interrupts
  ICR1 = 0;
  OCR1A = 0;
  sei();//allow interrupts
}

// Maximum input threshold is FIXME
void set_threshold_cnt5(const unsigned int thresh)
{
  motor_step_cnt_incr = thresh;
  ICR5 = thresh;
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
    }
    else
    {
      motor_direction = MOTORCTRL_DIR_BACKWARD;
      remaining_distance = -remaining_distance;
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
      set_freq_pwm1(speed);
      motor_inmotion = 1;
    }
    else
    {
      digitalWrite(MOTORCTRL_DRIVE_ENBL_PIN, MOTORCTRL_DRIVE_ENBL_FALSE);
      motor_inmotion = 0;
    }
  }
}

void loop()
{

  unsigned int motor_speed = 500; // Max TIM1_FREQ_DIV_FACTOR/2
  unsigned int motor_target_pos = 1;


  while(1) {
    //digitalWrite(MOTORCTRL_DIR_PIN, 1);

    set_motor_goto_position(motor_target_pos, motor_speed);
    for (int i = 0; i<20; i++)
    {
      Serial.print("Motor position: ");
      Serial.println(motor_position);
      delay(500);
    }
    delay(10000);
  }
}
