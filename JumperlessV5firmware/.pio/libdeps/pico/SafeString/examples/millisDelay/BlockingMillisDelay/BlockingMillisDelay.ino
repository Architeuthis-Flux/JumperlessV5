// BlockingMillisDelay
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

// have const here so it is easy to find and change
const unsigned long LED_ON_DELAY_TIME = 2000; // in ms (2sec)
millisDelay ledOnDelay; // the delay object to keep led on for 1sec

const unsigned long BLOCKING_DELAY_TIME = 10000; // in ms (10sec)
millisDelay ledBlockOnDelay; // the delay object to prevent turning led on again for 10sec after it goes off

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

  Serial.println("Enter R to turn LED on for 1sec");
  Serial.println(" After LED goes off it is blocked from turning on again for 10sec");

  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
}

void loop() {
  ledBlockOnDelay.justFinished(); // check for timeout and set isRunning() false when timed out

  if (ledOnDelay.justFinished()) { // don't combine this test with any other condition
    // on delay timed out
    digitalWrite(led, LOW); // turn led off
    Serial.println("ledOnDelay finished, LED turned off.");
    ledBlockOnDelay.start(BLOCKING_DELAY_TIME);
  }

  char c = 0;
  if (Serial.available()) {
    c = Serial.read();
    while (Serial.available()) {
      Serial.read(); // clear rest on input
    }
  }

  if ((c == 'R') || (c == 'r')) {
    if (ledBlockOnDelay.isRunning()) {
      Serial.print("LED blocked from turning on for next "); Serial.print(ledBlockOnDelay.remaining()); Serial.println("ms");
    } else {
      // can turn led on again
      ledOnDelay.start(LED_ON_DELAY_TIME);
      digitalWrite(led, HIGH); // turn let on
      Serial.println("ledDelay.start(....) called and Led turned ON");
    }
  } else if ((c == '\n') || (c == '\r')) {
    // skip end of line chars
  } else if (c != 0) {
    Serial.print("Invalid cmd:"); Serial.println(c);
  }
}
