#include <PinFlasher.h>
// see the tutorial https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

/*
 * (c)2021 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

int flasher_pin = 13;
PinFlasher flasher(flasher_pin);
// Pin 13 has an LED connected on most Arduino boards.
// if using Arduino IDE 1.5 or above you can use pre-defined
// LED_BUILTIN  instead of 'led'
//

void setup() {
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
  Serial.println("Led should be OFF");
  Serial.println(" Enter 1 for 1sec flash, 2 for 2sec etc upto 9, enter the letter N to keep on, enter o or digit 0 to turn off, i to invert output");
}


void loop() {
  flasher.update(); // should call this often, atleast every loop()

  int i = Serial.read(); // note read() returns an int
  if (i != -1) { // read() return -1 if there is nothing to be read.
    // got a char handle it
    char c = (char)i;
    Serial.println(c); // need to cast to char c to print it otherwise you get a number instead
    if (isDigit(c)) {
      Serial.print("Start flashing "); Serial.print(c); Serial.print("on, "); Serial.print(c); Serial.println("sec off");
      flasher.setOnOff((c - '0') * 1000);
    } else if ((c == 'o') || (c == 'O'))  {
      Serial.print("Stop flashing, turn off ");  Serial.println();
      flasher.setOnOff(PIN_OFF); // stop
    } else if ((c == 'n') || (c == 'N')) {
      Serial.print("Stop flashing, turn on ");  Serial.println();
      flasher.setOnOff(PIN_ON); // hard on
    } else if ((c == 'i') || (c == 'I')) {
      Serial.print("Invert Output");  Serial.println();
      flasher.invertOutput();
    }
  }
}
