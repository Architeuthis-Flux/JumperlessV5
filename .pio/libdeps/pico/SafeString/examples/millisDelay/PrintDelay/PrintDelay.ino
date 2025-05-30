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
const unsigned long LED_DELAY = 20000; // in ms (10sec)
millisDelay ledDelay; // the delay object

const unsigned long PRINT_DELAY = 2000; // in ms (2sec)
millisDelay printDelay; // the delay object

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

  Serial.println("Using ledDelay.start(20000) to turn Led for 20sec.");
  Serial.println(" Enter F to finish early ");
  Serial.println(" or S to stop delay and leave LED on ");
  Serial.println(" or R to restart turn led ON and restart ledDelay");
  Serial.println(" Every 2 sec another delay prints out the time spent and remaining for ledDelay");
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);
  ledDelay.start(LED_DELAY);
  printDelay.start(PRINT_DELAY);
}

void loop() {
  if (printDelay.justFinished()) { // don't combine this test with any other condition
    printDelay.repeat(); // for next print
    Serial.print(" ledDelay remaining:"); Serial.println(ledDelay.remaining());
  }

  char c = 0;
  if (Serial.available()) {
    c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear rest on input
    }
  }
  if ((c == 'F') || (c == 'f')) {
    if (!ledDelay.isRunning()) {
      Serial.println("ledDelay already finished so calling finish() does nothing");
    }
    ledDelay.finish();
    Serial.println("ledDelay.finished() called");
  } else if ((c == 'S') || (c == 's')) {
    if (!ledDelay.isRunning()) {
      Serial.println("ledDelay already finished so calling stop() does nothing");
    }
    ledDelay.stop();
    Serial.println("ledDelay.stop() called");
  } else if ((c == 'R') || (c == 'r')) {
    ledDelay.restart();
    digitalWrite(led, HIGH);
    Serial.println("Led turned ON and ledDelay.restart() called");
  } else if ((c == '\n') || (c == '\r')) {
    // skip end of line chars
  } else if (c != 0) {
    Serial.print("Invalid cmd:"); Serial.println(c);
  }

  if (ledDelay.justFinished()) { // don't combine this test with any other condition
    // delay timed out
    digitalWrite(led, LOW); // turn led off
    Serial.println("Delay finished and stopped. Led Turned off.");
  }
}
