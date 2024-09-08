#include <millisDelay.h>
// see the tutorial https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

/*
 * (c)2018 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

int led = 13;
// Pin 13 has an LED connected on most Arduino boards.
// if using Arduino IDE 1.5 or above you can use pre-defined
// LED_BUILTIN  instead of 'led'
//

// have const here so it is easy to find and change
const unsigned long DELAY_TIME = 10000; // in ms (2sec)

millisDelay ledDelay; // the delay object

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

  Serial.println("Enter R to run single delay turning LED on for 10sec");
  Serial.println(" or F to finish the delay early, if still running");
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
}

void loop() {
  char c = 0;
  if (Serial.available()) {
    c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear rest on input
    }
  }
  if ((c == 'R') || (c == 'r')) {
    ledDelay.start(DELAY_TIME);
    digitalWrite(led, HIGH); // turn let on
    Serial.println("ledDelay.start(10000) called and Led turned ON");
  } else if ((c == 'F') || (c == 'f')) {
    if (ledDelay.isRunning()) {
      Serial.println("Finish Delay early");
    } else {
      Serial.println("Delay already finished. Calling ledDelay.finish() does nothing");
    }
    ledDelay.finish();
    Serial.println("ledDelay.finish() called.");
  } else if ((c == '\n') || (c == '\r')) {
    // skip end of line chars
  } else if (c != 0) {
    Serial.print("Invalid cmd:"); Serial.println(c);
  }

  if (ledDelay.justFinished()) { // don't combine this test with any other condition
    // delay timed out
    digitalWrite(led, LOW); // turn led off
    Serial.println("Single Delay finished, LED turned off.");
  }
}
