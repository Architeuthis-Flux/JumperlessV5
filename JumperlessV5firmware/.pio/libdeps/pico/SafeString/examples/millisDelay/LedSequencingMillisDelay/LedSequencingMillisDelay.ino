// LedSequencingMillisDelay
#include <millisDelay.h>
// see the tutorial https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

/*
   (c)2018 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/

int led = 13;
// Pin 13 has an LED connected on most Arduino boards.
// if using Arduino IDE 1.5 or above you can use pre-defined
// LED_BUILTIN  instead of 'led'
//

unsigned long onOffDelays[] = {2000, 1000, 500, 100, 500, 200};
//                             off,  on,  off,  on, off,  on     
//                           the off/on sequence depends of the led (on or off) setting in startSequence for the first delay

const size_t NUMBER_OF_STEPS = 6; // can be smaller the number of elements in onOffDelays but not larger
size_t stepIdx = 0;

millisDelay ledOnOffDelay; // the delay object
bool ledOn = false;

// the setup routine runs once when you press reset:
void setup() {
  // initialize the digital pin as an output.
  Serial.begin(9600);
  // wait a few sec to let user open the monitor
  for (int i = 5; i > 0; i--) {
    delay(1000);
    Serial.print(i); Serial.print(' ');
  }
  Serial.println();
  //check the size of the onOffDelay versus NUMBER_OF_STEPS
  if ( (sizeof(onOffDelays) / sizeof(unsigned long))< NUMBER_OF_STEPS) {
    while (1) {
      Serial.print(" !! Error NUMBER_OF_STEPS:"); Serial.print(NUMBER_OF_STEPS); Serial.println(" exceeds number of values in onOffDelays[]");
      delay(5000); // print every 5 secs for ever
    }
  }

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW); // led off
  startSequence(); // start the sequence
}

void startSequence() {
  stepIdx = 0;
  ledOnOffDelay.start(onOffDelays[stepIdx]);
  digitalWrite(led, LOW); // TURN led off for first step
  ledOn = false;
}

void loop() {

  if (ledOnOffDelay.justFinished()) { // don't combine this test with any other condition
    // on delay timed out
    // toggle led
    ledOn = !ledOn;
    if (ledOn) {
      digitalWrite(led, HIGH); // turn led on
    } else {
      digitalWrite(led, LOW); // turn led off
    }
    stepIdx++;
    if (stepIdx >= NUMBER_OF_STEPS) {
      stepIdx = 0; // repeat sequence
    }
    ledOnOffDelay.start(onOffDelays[stepIdx]);
  }

}
