// FirstDamperControl.ino
// Only one call to runStepper() task in the loop();
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
#include <AccelStepper.h>

#include <MAX31856_noDelay.h>
// Use software SPI: CS, DI, DO, CLK
MAX31856_noDelay maxthermo = MAX31856_noDelay(10, 11, 12, 13);
// use hardware SPI, just pass in the CS pin
//MAX31856_noDelay maxthermo = MAX31856_noDelay(10);

createSafeStringReader(sfReader, 15, " ,\r\n"); // create a SafeString reader with max Cmd Len 15 and delimiters space, comma, Carrage return and Newline

//Example of using BufferedOutput to release bytes when there is space in the Serial Tx buffer, extra buffer size 80
createBufferedOutput(bufferedOut, 80, DROP_UNTIL_EMPTY);

int led = 7; // new pin for led
// Pin 13 is used for the MAX31856 board
bool ledOn = false; // keep track of the led state
millisDelay ledDelay;
millisDelay printDelay;

float tempReading = 0.0; // from readTemp
float simulatedTempReading = 0.0; // from user input
bool closeDampler = true;

millisDelay max31856Delay;
const unsigned long MAX31856_DELAY_MS = 200; // max single shot conversion time is 185ms
bool readingStarted = false;

AccelStepper stepper; // Defaults to AccelStepper::FULL4WIRE (4 pins) on 2, 3, 4, 5

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

  //initialize digital pin led as an output.
  pinMode(led, OUTPUT);

  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);

  ledDelay.start(1000); // start the ledDelay, toggle every 1000ms
  printDelay.start(5000); // start the printDelay, print every 5000ms
  Serial.println(F("Enter simulated temperature, 0 to 100, or run  to start damper control or close to close the damper."));

  stepper.setMaxSpeed(1000);
  stepper.setSpeed(500); // need to call atleast every 2ms
  stepper.setAcceleration(50);
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
    bufferedOut.print(F("Temp:")); bufferedOut.println(simulatedTempReading);
    bufferedOut.print(F("Position current:")); bufferedOut.print(stepper.currentPosition());
    if (closeDampler) {
      bufferedOut.println(F(" Close Damper"));
    } else {
      bufferedOut.println(F(" Damper running"));
    }
  } // else nothing to do this call just return, quickly
}

// task to get the user's cmds, input commands terminated by space or , or \r or \n or no new characters for 2secs
// set Global variable with input cmd
void processUserInput() {
  if (sfReader.read()) { // echo input and 100ms timeout, non-blocking!!
    sfReader.toLowerCase(); // ignore case
    if (sfReader == "close") { // all lower case
      closeDampler = true;
    } else if (sfReader ==  "run") {
      closeDampler = false;
    } else { //try and convert as temp
      float newSimulatedTempReading = simulatedTempReading;
      if (!sfReader.toFloat(newSimulatedTempReading)) {
        // conversion failed,  newSimulatedTempReading unchanged
        bufferedOut.print(F(" -- Invalid SimulatedTemp or close or run cmds."));
      } else { // have valid float, check range
        if ((newSimulatedTempReading < 0.0) || (newSimulatedTempReading > 100.0)) {
          bufferedOut.print(F(" -- Invalid SimulatedTemp must be between 0.0 and 100.0 ")); bufferedOut.println();
        } else {
          simulatedTempReading = newSimulatedTempReading; // update it
        }
      }
    }
  } // else token is empty
}

// return 0 if have new reading and no errors
// returns -1 if no new reading
// returns >0 if have errors
int readTemp() {
  if (!readingStarted) { // start one now
    maxthermo.oneShotTemperature();
    // start delay to pick up results
    max31856Delay.start(MAX31856_DELAY_MS);
  }
  if (max31856Delay.justFinished()) {
    readingStarted = false;
    // can pick up both results now
    tempReading = maxthermo.readThermocoupleTemperature();
    return 0; // new reading
  }
  return -1; // no new reading
}

void setDamperPosition() {
  if (closeDampler) {
    stepper.moveTo(0);
  } else {
    long stepPosition = simulatedTempReading * 50;
    stepper.moveTo(stepPosition);
  }
}

void runStepper() {
  stepper.run();
}

// the loop function runs over and over again forever
void loop() {
  bufferedOut.nextByteOut(); // call this one or more times each loop() to release buffered chars
  loopTimer.check(bufferedOut);
  processUserInput();
  blinkLed7(closeDampler); // call the method to blink the led
  printTemp(); // print the temp
  int rtn = readTemp(); // check for errors here
  if (rtn == 0) {
    // have new reading
  }
  setDamperPosition();
  runStepper();
}
