// LoopTimer_BlinkDelay.ino
/*
 * (c)2019 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
// install SafeString library from Library manager or from https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include <loopTimer.h>
// the loopTimer, BufferedOutput, SafeStringReader and millisDelay are all included in SafeString library V3+

/*
  Blink

  Turns an led on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board led you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. led is set to
  the correct led pin independent of which board is used.
  If you want to know what pin the on-board led is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://www.arduino.cc/en/Main/Products

  modified 8 May 2014
  by Scott Fitzgerald
  modified 2 Sep 2016
  by Arturo Guadalupi
  modified 8 Sep 2016
  by Colby Newman

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/Blink
*/
int led = 13;
// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.println(i);
    delay(500);
  }
  // initialize digital pin led as an output.
  pinMode(led, OUTPUT);
}

// the loop function runs over and over again forever
void loop() {
  loopTimer.check(Serial);
  digitalWrite(led, HIGH);   // turn the led on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(led, LOW);    // turn the led off by making the voltage LOW
  delay(1000);                       // wait for a second
}
