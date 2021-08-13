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

// Loop variables
unsigned long startTime;
unsigned int loopcounter = 0;

// Clamp variables
const unsigned int min_rpm = 30; // Fan still runs at 30, but 60 would be better for more airflow.
const unsigned int max_rpm = 320;

unsigned int rpm_interrupt_count = 0;
unsigned int rpm;

const unsigned int NUMBER_OF_FANS = 6;

typedef struct fan_variable_structure {
  unsigned long idrac_percent_request;          // Read: what fan speed is idrac requesting
  unsigned long idrac_rpm;
  
  unsigned long idrac_tach_increment;           // Var: the increment used for timing how often we need to pulse
  unsigned long idrac_start_time_micros;        // Var: last tick start time

  unsigned int  fan_rpm;                        // Read: current RPM of the fan
  unsigned int  fan_rpm_interrupt_count = 0;    // Var: how many interrupts do we get on the fan_tach_pin_input fan per cycle

  
  
};

fan_variable_structure fan[NUMBER_OF_FANS];


const unsigned int FAN_MAP_POINTS = 5; // MIN 3 points

unsigned int fan_curve_map[FAN_MAP_POINTS][2] =
{ {0, 0},
  {40, 20},
  {50, 30},
  {80, 40},
  {100, 100}
};

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

  Serial.print("map[90]= ");
  Serial.println(map_fan_curve_pwm_based_on_input_pwm (90));
  Serial.print("map[1001]= ");
  Serial.println(map_fan_curve_pwm_based_on_input_pwm (1001));

  // 
  // Fake RPM
  // max we are likely to see is ~ 650HZ
  unsigned long freq;
  freq = 600 / 60L * 100 / 96;                              // 17k gives 16160 in bios 20k gives 19218 ~ 4-5% error
  fan[0].idrac_tach_increment = 1000000L / (4 * freq)  ; // two pulses a second = 4 edges dumbass
   
  startTime = millis();
  fan[0].idrac_start_time_micros = micros();
}

bool toggle;


void loop() {
  int pwm = 60;
  unsigned long duration;

  OCR1A = pwm;

  unsigned long high_duration, low_duration, percent;


  unsigned long current_time_in_micros = micros();


  duration = (current_time_in_micros - fan[0].idrac_start_time_micros);
  if ( duration > fan[0].idrac_tach_increment) {  // toggle based on rpm
    /*Serial.print(" fan[0].idrac_tach_increment = ");

      Serial.print(" fan[0].idrac_start_time_micros = ");
      Serial.print(fan[0].idrac_start_time_micros);
      Serial.print(" micros = ");
      Serial.print(current_time_in_micros);
      Serial.print(" sub = ");
      Serial.println((current_time_in_micros - fan[0].idrac_start_time_micros));
    */
    fan[0].idrac_start_time_micros = current_time_in_micros;
    toggle = !toggle;
    openDrain(computer_tach_output, toggle);


  }



  if ((duration = millis() - startTime) > 1000) {  // update RPM once per second
    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);

    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);

    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);


    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);

    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);


    high_duration = pulseIn(computer_pwm_input, HIGH, 400);
    low_duration = pulseIn(computer_pwm_input, LOW, 400);
    percent = (100 * high_duration) / (low_duration + high_duration);



    Serial.println();
    Serial.print(" highduration = ");
    Serial.println(high_duration);

    Serial.print(" low duration = ");
    Serial.println(low_duration);

    Serial.print(" % = ");
    Serial.println(percent);


    rpm = rpm_interrupt_count * 30;
    int temp = pulses_per_time_to_rpm( rpm_interrupt_count, duration);
    Serial.print("rpm="); Serial.print(rpm);
    Serial.print(" , "); Serial.println(temp);

    if (loopcounter % 2) {              // only display stats every 2 seconds
      Serial.print("PWM = ");
      Serial.print(map(pwm, 0, 320, 0, 100));

      Serial.print("%, Speed = (");
      Serial.print(rpm_interrupt_count);
      Serial.print(" count ");
      Serial.print(rpm);
      Serial.print(" rpm )");
      Serial.print(" loop_counter = ");
      Serial.println(loopcounter);
    }

    startTime = millis();
    rpm_interrupt_count = 0;
  }

  ++loopcounter;
}

void counter() {
  rpm_interrupt_count++;
}

// pulses occur twice every rotation
unsigned int pulses_per_time_to_rpm( unsigned long total_pulses, unsigned int duration_in_miliseconds) {
  unsigned long fan_rpm = total_pulses  * 60 / 2 * 1000 / duration_in_miliseconds;

  return fan_rpm;
}


void openDrain(byte pin, bool value)
{ if (value)
    pinMode(pin, INPUT);
  else
  { pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
}


void print_fan_statistics() {
char buffer[32];
Serial.println(" Fan #   IDRAC: ");
  for (int i=0; i< NUMBER_OF_FANS; i++) {
    sprintf(buffer, "Fan [%d] idrac: %d%% ", i,  i);
    Serial.println(buffer);
  }
}

// notes: default Falcon CPU speed 27% PWM

// Waveform Generation Mode bits (WGM): these control the overall mode of the timer. (These bits are split between TCCRnA and TCCRnB.)
// Clock Select bits (CS): these control the clock prescaler
// Compare Match Output A Mode bits (COMnA): these enable/disable/invert output A
// Compare Match Output B Mode bits (COMnB): these enable/disable/invert output B
