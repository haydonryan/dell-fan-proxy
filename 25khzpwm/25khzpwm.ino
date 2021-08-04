// Dell Fan Proxy
//
//

const int fanControlPin = 9;
int count = 0;
unsigned long startTime;
int rpm;

void setup() {
TCCR1A = 0;
TCCR1B = 0;   // reset adruino core library config
TCNT1 = 0;  //reset timer
TCCR1A = _BV(COM1A1) | _BV(COM1B1) | _BV(WGM11) ;
TCCR1B = _BV(WGM13) | _BV(CS10); //for pins 9 and 10
ICR1= 320;
pinMode(fanControlPin, OUTPUT);
OCR1A = 0;
OCR1B = 0;
Serial.begin(9600);
attachInterrupt(digitalPinToInterrupt(2), counter, RISING); // yellow wire
}


void loop() {
//for( int pwm=0; pwm <= 320; pwm +=64) {
int pwm=160;
  OCR1A = pwm;
  delay(5000);
  startTime = millis();
  count = 0;
  while ((millis() - startTime ) < 1000) {      // Count how many pulses in a 1 second period.
  }
  rpm = count * 30;     // x 60 / 2   Twp pulses per revolution.
  Serial.print("PWM = ");
  Serial.print(map(pwm,0,320,0,100));
  Serial.print("%, Speed = ");
   Serial.print(count);
  Serial.print(" count ");    
  Serial.print(rpm);
  Serial.println(" rpm");    
//  }
}

void counter() {
  count++;
}
