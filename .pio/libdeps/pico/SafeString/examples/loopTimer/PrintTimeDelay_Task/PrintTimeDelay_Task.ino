// PrintTimeDelay_Task.ino
// Prints time using delay (bad)
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

int led = 13;

/*
  print_ms() task 
  Prints the time every 5 seconds, repeatedly.
*/

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

// the task method
void print_ms() {
  Serial.println(millis());   // print the current ms
  delay(5000);              // wait for a 5 seconds
}

// the loop function runs over and over again forever
void loop() {
  loopTimer.check(Serial);
  print_ms(); // call the method to print the time
}
