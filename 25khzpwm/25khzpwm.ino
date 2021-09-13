///////////////////////////////////////////////////////////////////////////
//
// Dell Fan Proxy
//
// For ATmega2560
//
///////////////////////////////////////////////////////////////////////////

const unsigned int NUMBER_OF_FANS = 6;

// Pins to communiate with fan.
const int fan_pwm_pin_output[NUMBER_OF_FANS] = {44,45,46,6,7,8};     // Blue Wire.  Uses PWM registers and output   TODO: CHANGE PIN 11 to 44
const int fan_tach_pin_input[NUMBER_OF_FANS] = {2,3,18,19,20,21};      // Yellow Wire. Uses digital interrupts TODO: 20 and 21 don't work because of the internal 10k pullup

// Pins to communicate back to computer.
const int computer_pwm_input[NUMBER_OF_FANS] = {22,23,24,25,26,27};     // uses pulseIn to read a sample pwm length
const int computer_tach_output[NUMBER_OF_FANS] = {32,33,34,35,36,37};   // Uses open drain to fake signal back.

///////////////////////////////////////////////////////////////////////////
// Fan speed map
// Modify the numbers in the fan_curve_map to change the points of the fan curve.
// The first value in each map determines where the fan speed is defined - eg if we read in that value it will output the second value.
// Strongly recommend - do not modify the first and last points unless you fully understand what you're doing.
///////////////////////////////////////////////////////////////////////////

const unsigned int FAN_MAP_POINTS = 5; // MIN 3 points

unsigned int fan_curve_map[FAN_MAP_POINTS][2] =
{ {0, 0},    // leave as {0,0}
  {40, 20},
  {50, 30},
  {80, 40},
  {100, 100} // leave as {100,100}
};

///////////////////////////////////////////////////////////////////////////

// Loop variables
unsigned long startTime;
unsigned int loopcounter = 0;

// Clamp variables
const unsigned int min_rpm = 30; // Fan still runs at 30, but 60 would be better for more airflow.
const unsigned int max_rpm = 320;

// Main datastructure for storing fan variables
struct fan_variable_structure {
  unsigned int idrac_pwn_percent_request;       // Read: what fan speed is idrac requesting
  unsigned int idrac_rpm;                       // Var: desired rpm to generate for idrac
  
  unsigned long idrac_tach_increment;           // Var: the increment used for timing how often we need to pulse
  unsigned long idrac_start_time_micros;        // Var: last tick start time
  bool idrac_tach_open_drain_toggle;            // Var: state of open drain

  unsigned int fan_pwm_percent;                 // Write: output of the fan map that we send to the real fan
  unsigned int  fan_rpm;                        // Read: current RPM of the fan
  volatile unsigned int  fan_rpm_interrupt_count = 0;    // Var: how many interrupts do we get on the fan_tach_pin_input fan per cycle
};

fan_variable_structure fan[NUMBER_OF_FANS];

///////////////////////////////////////////////////////////////////////////
// Helper function to perform the fan curve mapping.
// Uses linear algebra to calcaulate points between control points
///////////////////////////////////////////////////////////////////////////
unsigned int map_fan_curve_pwm_based_on_input_pwm (unsigned int input_pwm ) {

  if (input_pwm > 100 ) input_pwm = 100;

  for (unsigned int i = 1; i < FAN_MAP_POINTS; i++) {
    if ( fan_curve_map[i - 1][0] < input_pwm && input_pwm <= fan_curve_map[i][0] ) {
      float fan_curve_base = i - 1;
      float y, x;

      y = (fan_curve_map[i][1] - fan_curve_map[i - 1][1]);
      x = (fan_curve_map[i][0] - fan_curve_map[i - 1][0]);
      float ratio = y / x;

      return ( ( (input_pwm - fan_curve_map[i - 1][0] ) * ratio ) + fan_curve_map[i - 1][1] );

    }
  }


  return 100;    //default safe
}

unsigned int calculate_idrac_tach_pwm_based_on_actual_fan_pwm (unsigned int input_rpm ) {
  // Fake RPM
  // max we are likely to see is ~ 650HZ
  unsigned long rpm;
  rpm = input_rpm / 60L * 100 / 96;                              // 17k gives 16160 in bios 20k gives 19218 ~ 4-5% error
  return (1000000L / (4 * rpm))  ; // two pulses a second = 4 edges dumbass
}

///////////////////////////////////////////////////////////////////////////
//
//  SETUP
// 
///////////////////////////////////////////////////////////////////////////

// https://www.robotshop.com/community/forum/t/arduino-101-timers-and-interrupts/13072
void setup() {
  TCCR5A = 0;  // reset Timer Counter Control Registers
  TCCR5B = 0;  // reset Timer Counter Control Register
  TCNT5 = 0;   // reset Timer Count Register

  // Set to PWM Phase Correct Mode WGM11+WGM13 See https://ww1.microchip.com/downloads/en/devicedoc/atmel-2549-8-bit-avr-microcontroller-atmega640-1280-1281-2560-2561_datasheet.pdf PG 145

  TCCR5A = _BV(COM5A1) | _BV(COM5B1) | _BV(COM5C1)| _BV(WGM11) ;   // _BV() is bit value equivalent to 1<<COM1A1
  TCCR5B = _BV(WGM13) | _BV(CS10); // Arduino Uno: pins 9 and 10 on arduino uno see https://arduinoinfo.mywikis.net/wiki/Arduino-PWM-Frequency
                                   // Arudino Mega: Pins 11 and 12
  ICR5 = 320;  // Set the top of the count (Input Capture Register) in PWM Phase Correct mode
  for (int i=0; i< NUMBER_OF_FANS;i++) {
    pinMode(fan_pwm_pin_output[i], OUTPUT);
  }
  OCR5A = 0;   // Reset Output Compare Registers
  OCR5B = 0;   // Reset Output Compare Registers
  OCR5C = 0;   // Reset Output Compare Registers
  Serial.begin(115200);

  // Disable i2C as pins 20 and 21 use it. 
  pinMode(SDA, INPUT);
  pinMode(SCL, INPUT);

  attachInterrupt(digitalPinToInterrupt(fan_tach_pin_input[0]), fan0counter, RISING); // yellow wire
  pinMode(computer_pwm_input[0], INPUT);

  loopcounter = 0;
  Serial.println("Starting up...");
  
  fan[0].fan_pwm_percent=60;
  fan[1].fan_pwm_percent=60;
  fan[2].fan_pwm_percent=60;
  fan[3].fan_pwm_percent=60;
  fan[4].fan_pwm_percent=60;
  fan[5].fan_pwm_percent=60;
  OCR5C = fan[0].fan_pwm_percent*320 / 100;
  OCR5B = fan[1].fan_pwm_percent*320 / 100;
  OCR5A = fan[2].fan_pwm_percent*320 / 100;
  OCR4A = fan[3].fan_pwm_percent*320 / 100;
  OCR4B = fan[4].fan_pwm_percent*320 / 100;
  OCR4C = fan[5].fan_pwm_percent*320 / 100;

  startTime = millis();
  for (int i=0; i< NUMBER_OF_FANS;i++) {
    fan[i].idrac_start_time_micros = micros();
  }
}

///////////////////////////////////////////////////////////////////////////
//
//  LOOP
// 
///////////////////////////////////////////////////////////////////////////

void loop() {
  
  unsigned long duration;

  unsigned long current_time_in_micros = micros();

  //////////////////////////////////////////////////////
  // Toggle open drain based on the desired frequency
  // 
  for (int i=0; i< NUMBER_OF_FANS;i++) {
    duration = (current_time_in_micros - fan[0].idrac_start_time_micros);
    if ( duration > fan[0].idrac_tach_increment) {  // toggle based on rpm
      fan[0].idrac_start_time_micros = current_time_in_micros;
      fan[0].idrac_tach_open_drain_toggle = !fan[0].idrac_tach_open_drain_toggle;
      openDrain(computer_tach_output[0], fan[0].idrac_tach_open_drain_toggle);
    }
  }

  //////////////////////////////////////////////////////
  // Once per second read the fan speed and print stats
  // 
  if ((duration = millis() - startTime) > 1000) {  // update once per second

    for (int i=0; i< NUMBER_OF_FANS;i++) {

      // Read Requested PWM %
      fan[i].idrac_pwn_percent_request = read_idrac_pwm_value_in_percentage (computer_pwm_input[i]);

      // Measure Fan RPM
      fan[i].fan_rpm = pulses_per_time_to_rpm(fan[i].fan_rpm_interrupt_count, duration);
    
      // Map idrac PWM % request to what we want fan PWM % to be
      fan[i].fan_pwm_percent =  map_fan_curve_pwm_based_on_input_pwm(fan[i].idrac_pwn_percent_request);
    }

    OCR5C = fan[0].fan_pwm_percent*320 / 100;
    OCR5B = fan[1].fan_pwm_percent*320 / 100;
    OCR5A = fan[2].fan_pwm_percent*320 / 100;
    OCR4A = fan[3].fan_pwm_percent*320 / 100;
    OCR4B = fan[4].fan_pwm_percent*320 / 100;
    OCR4C = fan[5].fan_pwm_percent*320 / 100;

    // Update RPM and tach that we want to generate for idrac
    for (int i=0; i< NUMBER_OF_FANS;i++) {
      fan[i].idrac_rpm = map_idrac_rpm_based_from_pwm(fan[i].idrac_pwn_percent_request);                  // pass through fan RPM. 
      fan[i].idrac_tach_increment= calculate_idrac_tach_pwm_based_on_actual_fan_pwm(fan[i].idrac_rpm);    // calculate tach increment for desired fanspeed.      
      fan[i].idrac_start_time_micros = micros();                                                          // reset tach timer
    }

    if (loopcounter % 2) {              // only display stats every 2 seconds
      print_fan_statistics();
    }

    startTime = millis();
    for (int i=0; i< NUMBER_OF_FANS;i++) {
      fan[i].fan_rpm_interrupt_count = 0 ;
    }
  }

  ++loopcounter;
}

void fan0counter() {
  fan[0].fan_rpm_interrupt_count++;
}

void fan1counter() {
  fan[1].fan_rpm_interrupt_count++;
}

void fan2counter() {
  fan[2].fan_rpm_interrupt_count++;
}

void fan3counter() {
  fan[3].fan_rpm_interrupt_count++;
}

void fan4counter() {
  fan[4].fan_rpm_interrupt_count++;
}

void fan5counter() {
  fan[5].fan_rpm_interrupt_count++;
}

// pulses occur twice every rotation
unsigned int pulses_per_time_to_rpm( unsigned long total_pulses, unsigned int duration_in_miliseconds) {
  unsigned long fan_rpm = total_pulses  * 60 / 2 * 1000 / duration_in_miliseconds;

  return fan_rpm;
}


unsigned int read_idrac_pwm_value_in_percentage (unsigned int pin) {
   unsigned long high_duration, low_duration, percent;

    high_duration = pulseIn(pin, HIGH, 400);
    low_duration = pulseIn(pin, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);
    if (percent > 100) percent=100;

    if(0) {  // debug flag
      Serial.println();
      Serial.print(" highduration = ");
      Serial.println(high_duration);
  
      Serial.print(" low duration = ");
      Serial.println(low_duration);
  
      Serial.print(" % = ");
      Serial.println(percent);
     }
    return percent;
}

void openDrain(byte pin, bool value)
{
  if (value)
    pinMode(pin, INPUT);
  else {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}

const unsigned int MAX_RPM_MAP_POINTS = 11; 

unsigned int rpm_map[MAX_RPM_MAP_POINTS][2] =
{ {0, 0},    // leave as {0,0}
  {10, 2007},
  {20, 4045},
  {30, 6053},
  {40, 8091},
  {50, 10129},
  {60, 12077},
  {70, 14715},
  {80, 16213},
  {90, 16700},
  {100, 17000} // leave as {100,100}
};


unsigned int map_idrac_rpm_based_from_pwm(unsigned int input_pwm ) {

  if (input_pwm > 100 ) input_pwm = 100;

  for (unsigned int i = 1; i < MAX_RPM_MAP_POINTS; i++) {
    if ( rpm_map[i - 1][0] < input_pwm && input_pwm <= rpm_map[i][0] ) {
      float fan_curve_base = i - 1;
      float y, x;

      y = (rpm_map[i][1] - rpm_map[i - 1][1]);
      x = (rpm_map[i][0] - rpm_map[i - 1][0]);
      float ratio = y / x;

      return ( ( (input_pwm - rpm_map[i - 1][0] ) * ratio ) + rpm_map[i - 1][1] );

    }
  }


  return rpm_map[MAX_RPM_MAP_POINTS - 1][1];    //default return RPMs for 100%
}

void print_fan_statistics() {
  char buffer[256];
  
  for (int i=0; i< 3/*NUMBER_OF_FANS*/; i++) {
    sprintf(buffer, "Fan [%u] idrac: (%u%%, %u rpm) fan: (%u%%, %u rpm)", i,  fan[i].idrac_pwn_percent_request, fan[i].idrac_rpm, fan[i].fan_pwm_percent, fan[i].fan_rpm);
    sprintf(buffer, "Fan [%u] idrac: (%u%%, %u rpm) fan: (%u%%, %u rpm)", i,  fan[i].idrac_pwn_percent_request, fan[i].idrac_rpm, fan[i].fan_pwm_percent, fan[i].fan_rpm);
    Serial.println(buffer);

  }  
  Serial.println();

  return;
}

// notes: default Falcon CPU speed 27% PWM

// Waveform Generation Mode bits (WGM): these control the overall mode of the timer. (These bits are split between TCCRnA and TCCRnB.)
// Clock Select bits (CS): these control the clock prescaler
// Compare Match Output A Mode bits (COMnA): these enable/disable/invert output A
// Compare Match Output B Mode bits (COMnB): these enable/disable/invert output B
