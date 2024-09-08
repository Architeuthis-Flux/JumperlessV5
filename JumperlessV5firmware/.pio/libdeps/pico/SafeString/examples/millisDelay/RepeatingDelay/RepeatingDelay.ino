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
bool ledOn = false; // keep track of led On/Off
// Pin 13 has an LED connected on most Arduino boards.
// if using Arduino IDE 1.5 or above you can use pre-defined
// LED_BUILTIN  instead of 'led'
//

// have const here so it is easy to find and change 
const unsigned long FLASH_TIME = 1500; // in ms 
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

  Serial.println("Enter R to start repeating delay and flash LED");
  Serial.println(" or S to stop");
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  ledOn = false;
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
    ledDelay.start(FLASH_TIME);
    digitalWrite(led, HIGH);
    ledOn = true;
    Serial.println("ledDelay.start(1500) called and Led turned ON");
    Serial.println("Repeating Delay running");
  } else if ((c == 'S') || (c == 's')) {
    ledDelay.stop();
    Serial.println("ledDelay.stop() called.  Led stays in current state, On or Off");
  } else if ((c == '\n') || (c == '\r')) {
    // skip end of line chars
  } else if (c != 0) {
    Serial.print("Invalid cmd:"); Serial.println(c);
  }

  if (ledDelay.justFinished()) { // don't combine this test with any other condition
    // toggle led and repeat
    ledDelay.repeat(); // repeat
    Serial.println("ledDelay.repeat() called and led toggled");
    ledOn = !ledOn;   // toggle led
    if (ledOn) {
      digitalWrite(led, HIGH);
    } else {
      digitalWrite(led, LOW);      
    }
  }

}
