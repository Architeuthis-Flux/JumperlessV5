// BufferedPrintTime_Blink.ino
// Print and Blink without delay and with buffered prints
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/
#include <loopTimer.h>
// install the loopTimer library from https://www.forward.com.au/pfod/ArduinoProgramming/RealTimeArduino/TimingDelaysInArduino.html
// loopTimer.h also needs the millisDelay library installed from https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html
#include <BufferedOutput.h>
// install SafeString library from Library manager or from https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
// to get BufferedOutput. See https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html for a full tutorial
// on Arduino Serial I/O that Works
#include <millisDelay.h>

//Example of using BufferedOutput to release bytes when there is space in the Serial Tx buffer, extra buffer size 80
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

int led = 13;
// Pin 13 has an led connected on most Arduino boards.
bool ledOn = false; // keep track of the led state
millisDelay ledDelay;
millisDelay printDelay;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.println(i);
    delay(500);
  }
  bufferedOut.connect(Serial);  // connect buffered stream to Serial

  // initialize digital pin led as an output.
  pinMode(led, OUTPUT);
  ledDelay.start(1000); // start the ledDelay, toggle every 1000ms
  printDelay.start(5000); // start the printDelay, print every 5000ms
}

// the task method
void blinkLed13() {
  if (ledDelay.justFinished()) {   // check if delay has timed out
    ledDelay.repeat(); // start delay again without drift
    ledOn = !ledOn;     // toggle the led
    bufferedOut.print("The built-in board led, pin 13, is being turned "); bufferedOut.println(ledOn?"ON":"OFF");
    digitalWrite(led, ledOn?HIGH:LOW); // turn led on/off
  } // else nothing to do this call just return, quickly
}

// the task method
void print_ms() {
  if (printDelay.justFinished()) {
    printDelay.repeat(); // start delay again without drift
    bufferedOut.println(millis());   // print the current ms
  } // else nothing to do this call just return, quickly
}

// the loop function runs over and over again forever
void loop() {
  bufferedOut.nextByteOut(); // call at least once per loop to release chars
  loopTimer.check(bufferedOut); // send loop timer output to the bufferedOut
  blinkLed13(); // call the method to blink the led
  print_ms(); // print the time
}
