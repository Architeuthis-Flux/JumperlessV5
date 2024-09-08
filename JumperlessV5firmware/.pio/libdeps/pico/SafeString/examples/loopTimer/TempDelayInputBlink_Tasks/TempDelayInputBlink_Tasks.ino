// TempDelayInputBlink_Tasks.ino
// Uses Adafruit's MAX31856 library which has delay(), (bad)
/*
   (c)2020 Forward Computing and Control Pty. Ltd.
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

#include <Adafruit_MAX31856.h>
// Use software SPI: CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin
//Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10);

createSafeStringReader(sfReader, 15, " ,\r\n"); // create a SafeString reader with max Cmd Len 15 and delimiters space, comma, Carrage return and Newline

//Example of using BufferedOutput to release bytes when there is space in the Serial Tx buffer, extra buffer size 80
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

int led = 7; // new pin for led
// Pin 13 is used for the MAX31856 board
bool ledOn = false; // keep track of the led state
millisDelay ledDelay;
millisDelay printDelay;

bool stopTempReadings = true;
float tempReading = 0.0;

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
  //SafeString::setOutput(Serial); //uncomment this to enable error msgs

  bufferedOut.connect(Serial);  // connect bufferedOut to Serial
  sfReader.connect(bufferedOut);
  sfReader.echoOn(); // echo goes out via bufferedOut
  sfReader.setTimeout(100); // set 100ms == 0.1sec non-blocking timeout

  // initialize digital pin led as an output.
  pinMode(led, OUTPUT);

  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);

  ledDelay.start(1000); // start the ledDelay, toggle every 1000ms
  printDelay.start(5000); // start the printDelay, print every 5000ms
  Serial.println(F("To control Temp readings use commands startTemp or stopTemp (other commands will be ignored)"));
  Serial.println(F("   Any line ending OR none can be used."));
  Serial.println(F("   You can enter multiple commands on the one line."));
}

// the task method
void blinkLed7(bool stop) {
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
void printTemp() {
  if (printDelay.justFinished()) {
    printDelay.repeat(); // start delay again without drift
    if (stopTempReadings) {
      bufferedOut.println(F(" Temp reading stopped"));
    } else {
      bufferedOut.print(F(" Temp:")); bufferedOut.println(tempReading);
    }
  } // else nothing to do this call just return, quickly
}

void handleStartCmd() {
  stopTempReadings = false; bufferedOut.terminateLastLine(); bufferedOut.println(" Start Temp Readings");
}
void handleStopCmd() {
  stopTempReadings = true; bufferedOut.terminateLastLine(); bufferedOut.println(" Stop Temp Readings");
}

// task to get the user's cmds, input commands terminated by space or , or \r or \n or no new characters for 2secs
// set Global variable with input cmd
void processUserInput() {
  if (sfReader.read()) { // echo input and 100ms timeout, non-blocking!!
    sfReader.toLowerCase(); // ignore case
    if (sfReader == "starttemp") { // all lower case
      handleStartCmd();
    } else if (sfReader == "stoptemp") { // all lower case
      handleStopCmd();
    } else {
      bufferedOut.println(" !! Invalid command: ");
      // ignore
    }
  } // else no delimited token
}

// returns 0 if have reading and no errors, else non-zero
int readTemp() {
  tempReading = maxthermo.readThermocoupleTemperature();
  return 0;
}


// the loop function runs over and over again forever
void loop() {
  bufferedOut.nextByteOut(); // call this one or more times each loop() to release buffered chars
  loopTimer.check(bufferedOut);
  processUserInput();
  blinkLed7(stopTempReadings); // call the method to blink the led
  printTemp(); // print the temp
  if (!stopTempReadings) {
    int rtn = readTemp(); // check for errors here
    if (rtn == 0) {
      // have new reading
    }
  }
}
