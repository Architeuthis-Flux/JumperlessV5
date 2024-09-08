#include <millisDelay.h>
// see the tutorial https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

/*
   (c)2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

#define DEBUG

int led = 13;
bool ledOn = false; // keep track of led On/Off
// Pin 13 has an LED connected on most Arduino boards.
// if using Arduino IDE 1.5 or above you can use pre-defined
// LED_BUILTIN  instead of 'led'
//

const unsigned long MAIN_TIME = 10000; // in ms
const unsigned long FIRST_PART_DELAY = 4000; // in ms
const unsigned long FREEZE_TIME = 2000; // in ms

millisDelay mainDelay; // the delay object, the overall delay
millisDelay firstPartDelay; // the delay object, the delay until we 'freeze' the main delay
millisDelay freezeDelay; // the delay object, the delay of the 'freeze' after which we 're-start' the main delay

unsigned long mainRemainingTime = 0;
unsigned long currentMillis = 0;

// the setup routine runs once when you press reset:
void setup() {
#ifdef DEBUG
  Serial.begin(9600);
  // wait a few sec to let user open the monitor
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
#endif
  // initialize the digital pin as an output.
  pinMode(led, OUTPUT);
  mainDelay.start(MAIN_TIME);
  currentMillis = millis(); // capture current time for print
  firstPartDelay.start(FIRST_PART_DELAY);
#ifdef DEBUG
  // NOTE: prints take time so do them AFTER setting up the delays
  Serial.print("start mainDelay at:"); Serial.print(currentMillis); Serial.print(" for "); Serial.print(MAIN_TIME); Serial.println("ms");
#endif
}

void loop() {

  if (mainDelay.isRunning()) { // set true by mainDelay.start(...) and false by mainDelay.justFinished() or mainDelay.stop()
    digitalWrite(led, HIGH); // led on while main delay is running
  } else {
    digitalWrite(led, LOW); // led off otherwise
  }

  if (mainDelay.justFinished()) { // don't combine this test with any other condition, sets isRunning() false when timed out
    digitalWrite(led, LOW); // turn the led off
#ifdef DEBUG
    Serial.print("mainDelay finished at:"); Serial.print(millis()); Serial.println();
#endif
  }

  if (firstPartDelay.justFinished()) { // finished first part of main delay // don't combine this test with any other condition
    if (mainDelay.isRunning()) {  // put any extra conditions inside here
      mainRemainingTime = mainDelay.remaining();  // remember how long left to run in the main delay
      // NOTE: need to call remaining() BEFORE  calling stop().  After calling stop() remaining will return 0 (ALWAYS)
      mainDelay.stop(); // stop mainDelay.  NOTE: mainDelay.justFinished() is NEVER true after stop()
      freezeDelay.start(FREEZE_TIME); // start freeze delay
      currentMillis = millis(); // capture current time for print
      // NOTE: need to capture remaining() and start freeze delay BEFORE doing debug prints as the prints take noticeble time to output.
      digitalWrite(led, LOW); // turn the led off
#ifdef DEBUG
      Serial.print("'Freeze' mainDelay at:"); Serial.print(currentMillis); Serial.print(" for "); Serial.print(FREEZE_TIME); Serial.println("ms");
      Serial.print(" mainDelay has "); Serial.print(mainRemainingTime); Serial.print("ms remaining"); Serial.println();
#endif
    }
  }

  if (freezeDelay.justFinished()) { // don't combine this test with any other condition
    if (mainRemainingTime > 0) { // put any extra conditions inside here
      // start mainDelay again
      currentMillis = millis(); // capture current time for print
      mainDelay.start(mainRemainingTime);
      digitalWrite(led, HIGH); // turn the led on
#ifdef DEBUG
      Serial.print("Restart mainDelay after 'freeze' at:"); Serial.print(currentMillis); Serial.println();
#endif
    }
  }

}
