// Input_Blink_Tasks.ino
/*
   (c)2019 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This generated code may be freely used for both private and commercial use
   provided this copyright is maintained.
*/
// install SafeString library from Library manager or from https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include <SafeString.h>
// the loopTimer, BufferedOutput, SafeStringReader and millisDelay are all included in SafeString library V3+
#include <loopTimer.h>
#include <BufferedOutput.h>
#include <millisDelay.h>
#include <SafeStringReader.h>

createSafeStringReader(sfReader, 15, " ,\r\n"); // create a SafeString reader with max Cmd Len 15 and delimiters space, comma, Carrage return and Newline

//Example of using BufferedOutput to release bytes when there is space in the Serial Tx buffer, extra buffer size 80
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

int led = 13;
// Pin 13 has an led connected on most Arduino boards.
bool ledOn = false; // keep track of the led state
millisDelay ledDelay;
millisDelay printDelay;
bool stopBlinking = false;


// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
  // SafeString::setOutput(Serial); //uncomment this to enable error msgs

  bufferedOut.connect(Serial);  // connect bufferedOut to Serial
  sfReader.connect(bufferedOut);
  sfReader.echoOn(); // echo goes out via bufferedOut
  sfReader.setTimeout(100); // set 100ms == 0.1sec non-blocking timeout

  // initialize digital pin led as an output.
  pinMode(led, OUTPUT);
  ledDelay.start(1000); // start the ledDelay, toggle every 1000ms
  printDelay.start(5000); // start the printDelay, print every 5000ms
  Serial.print(F("Led is currently "));  Serial.println(stopBlinking ? " OFF." : " blinking.");
  Serial.println(F("   Enter either start or stop to control the Blinking Led (other commands will be ignored)"));
  Serial.println(F("   Any line ending OR none can be used."));
  Serial.println(F("   You can enter multiple commands on the one line."));
}

// the task method
void blinkLed13(bool stop) {
  if (ledDelay.justFinished()) {   // check if delay has timed out
    ledDelay.repeat(); // start delay again without drift
    if (stop) {
      digitalWrite(led, LOW); // turn led on/off
      ledOn = false;
      return;
    }
    ledOn = !ledOn;     // toggle the led
    digitalWrite(led, ledOn ? HIGH : LOW); // turn led on/off
  } // else nothing to do this call just return, quickly
}

// the task method
void print_ms() {
  if (printDelay.justFinished()) {
    printDelay.repeat(); // start delay again without drift
    bufferedOut.println(millis());   // print the current ms
  } // else nothing to do this call just return, quickly
}

void handleStartCmd() {
  stopBlinking = false;
  bufferedOut.println(F(" Blinking Started"));
}
void handleStopCmd() {
  stopBlinking = true;
  bufferedOut.println(F(" Blinking Stopped"));
}

// task to get the user's cmds, input commands terminated by space or , or \r or \n or no new characters for 2secs
// set Global variable with input cmd
void processUserInput() {
  if (sfReader.read()) { // echo input and 100ms timeout, non-blocking set in setup()
    if (sfReader == "start") {
      handleStartCmd();
    } else if (sfReader == "stop") {
      handleStopCmd();
    } else {
      bufferedOut.println(" !! Invalid command: ");
    }
  } // else no delimited input
}

// the loop function runs over and over again forever
void loop() {
  bufferedOut.nextByteOut(); // call this one or more times each loop() to release buffered chars
  loopTimer.check(bufferedOut);
  processUserInput();
  blinkLed13(stopBlinking); // call the method to blink the led
  print_ms(); // print the time
}
