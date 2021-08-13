//////////////////////////////////////////////////////////////
//
// Dell Fan Proxy
//
//

// Pins to communiate with fan.
const int fan_pwm_pin_output = 9;     // Blue Wire
const int fan_tach_pin_input = 2;     // Yellow Wire

// Pins to communicate back to computer.
const int computer_pwm_input = 10;     // using pin10 since it's paired with pin 9 for TCCR1B
const int computer_tach_output = 12;   // update

// Fan speed map
// Modify the numbers in the fan_curve_map to change the points of the fan curve.
// The first value in each map determines where the fan speed is defined - eg if we read in that value it will output the second value.
// Strongly recommend - do not modify the first and last points unless you fully understand what you're doing.

const unsigned int FAN_MAP_POINTS = 5; // MIN 3 points

unsigned int fan_curve_map[FAN_MAP_POINTS][2] =
{ {0, 0},    // leave as {0,0}
  {40, 20},
  {50, 30},
  {80, 40},
  {100, 100} // leave as {100,100}
};

// Loop variables
unsigned long startTime;
unsigned int loopcounter = 0;

// Clamp variables
const unsigned int min_rpm = 30; // Fan still runs at 30, but 60 would be better for more airflow.
const unsigned int max_rpm = 320;

// Main datastructure for storing fan variables
const unsigned int NUMBER_OF_FANS = 6;

struct fan_variable_structure {
  unsigned int idrac_pwn_percent_request;          // Read: what fan speed is idrac requesting
  unsigned int idrac_rpm;

  
  unsigned long idrac_tach_increment;           // Var: the increment used for timing how often we need to pulse
  unsigned long idrac_start_time_micros;        // Var: last tick start time
  bool idrac_tach_open_drain_toggle;            // Var: state of open drain


  unsigned int fan_pwm_percent;                // Write: output of the fan map that we send to the real fan
  unsigned int  fan_rpm;                        // Read: current RPM of the fan
  unsigned int  fan_rpm_interrupt_count = 0;    // Var: how many interrupts do we get on the fan_tach_pin_input fan per cycle

  
  
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

  


void setup() {
  TCCR1A = 0;  // reset Timer/ Counter Control Registers
  TCCR1B = 0;  // reset adruino core library config
  TCNT1 = 0;   // reset timer counter
  TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11) ;
  TCCR1B = _BV(WGM13) | _BV(CS10); //for pins 9 and 10 on arduino uno see https://arduinoinfo.mywikis.net/wiki/Arduino-PWM-Frequency
  ICR1 = 320;
  //pinMode(fan_tach_pin_input, INPUT_PULLUP);   // use the internal 20k-50k pullup resistor https://www.arduino.cc/en/Tutorial/Foundations/DigitalPins
  pinMode(fan_pwm_pin_output, OUTPUT);
  OCR1A = 0;
  OCR1B = 0;
  Serial.begin(115200);
  attachInterrupt(digitalPinToInterrupt(fan_tach_pin_input), counter, RISING); // yellow wire
  pinMode(computer_pwm_input, INPUT);

  loopcounter = 0;
  Serial.println("Starting up...");

  
  
  fan[0].fan_pwm_percent=60;
   
  startTime = millis();
  fan[0].idrac_start_time_micros = micros();
}

void loop() {
  
  unsigned long duration;

  OCR1A = fan[0].fan_pwm_percent*320 / 100;

  unsigned long current_time_in_micros = micros();

  //////////////////////////////////////////////////////
  // Toggle open drain based on the desired frequency
  // 
  duration = (current_time_in_micros - fan[0].idrac_start_time_micros);
  if ( duration > fan[0].idrac_tach_increment) {  // toggle based on rpm
    fan[0].idrac_start_time_micros = current_time_in_micros;
    fan[0].idrac_tach_open_drain_toggle = !fan[0].idrac_tach_open_drain_toggle;
    openDrain(computer_tach_output, fan[0].idrac_tach_open_drain_toggle);
  }


  //////////////////////////////////////////////////////
  // Once per second read the fan speed and print stats
  // 
  if ((duration = millis() - startTime) > 1000) {  // update once per second


    // Read Requested PWM %
    fan[0].idrac_pwn_percent_request = read_idrac_pwm_value_in_percentage (computer_pwm_input);

    // Measure Fan RPM
    fan[0].fan_rpm = pulses_per_time_to_rpm( fan[0].fan_rpm_interrupt_count, duration);
    
   
    fan[0].fan_pwm_percent =  map_fan_curve_pwm_based_on_input_pwm(fan[0].idrac_pwn_percent_request); // map through requestd pwm via map to fan


      
    fan[0].idrac_rpm =  fan[0].fan_rpm ;                                                                // pass through fan RPM. 
    fan[0].idrac_tach_increment= calculate_idrac_tach_pwm_based_on_actual_fan_pwm(fan[0].idrac_rpm);    // calculate tach increment for desired fanspeed.      
    fan[0].idrac_start_time_micros = micros();                                                          // reset tach timer


    if (loopcounter % 2) {              // only display stats every 2 seconds
      print_fan_statistics();
    }

    startTime = millis();
    fan[0].fan_rpm_interrupt_count = 0;
  }

  ++loopcounter;
}

void counter() {
  fan[0].fan_rpm_interrupt_count++;
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


void print_fan_statistics() {
  char buffer[256];
  
  for (int i=0; i< 1/*NUMBER_OF_FANS*/; i++) {
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
