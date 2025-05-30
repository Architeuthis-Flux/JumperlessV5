// SafeString_read.ino
//
// This example takes commmands from the Arduino Monitor input and acts on them
// the available commands are start stop and reset
// Commands are delimited by space dot comma NL or CR
// If you set the Arduino Monitor to No line ending then the command will be ignored until you terminate it with a space or ,
//  Use the settings Newline or Carrage Return or Both NL & CR
//
// These commands can be picked out of a line of user input
// start  stop  reset
//
// download and install the SafeString library from library manager OR from
// www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include "SafeString.h"

char startCmd[] = "start";
char stopCmd[] = "stop";
char resetCmd[] = "reset";
char delimiters[] = " ,\r\n"; // space dot comma CR NL are cmd delimiters

const size_t maxCmdLength = 5; // length of largest command to be recognized, can handle longer input but will not tokenize it.
createSafeString(input, maxCmdLength + 1); //  to read input cmd, large enough to hold longest cmd + leading and trailing delimiters
createSafeString(token, maxCmdLength + 1); // for parsing, capacity should be >= input
bool skipToDelimiter = false; // bool variable to hold the skipToDelimiter state across calls to readUntilToken()
// set skipToDelimiter = true to skip initial data upto first delimiter.
// skipToDelimiter = true can be set at any time to next delimiter.

bool running = true;
unsigned long loopCounter = 0;

void setup() {
  Serial.begin(9600);    // Open serial communications and wait a few seconds
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  // first run without any outputting any error msgs or debugging
  SafeString::setOutput(Serial); // enable error messages and debug() output to be sent to Serial

  Serial.print(F("Enter one, or more, of the commands : ")); Serial.print(startCmd); Serial.print(" , "); Serial.print(stopCmd); Serial.print(" or "); Serial.print(resetCmd);
  Serial.println(F("  separated by space , CR or NL"));
  Serial.println(F(" Arduino monitor must be set to Newline or Carrage Return or Both NL & CR for this sketch to work."));
  Serial.println();
  Serial.println(F(" The SafeString readUntilToken() method does all of this work in one call and also has the option to echo the input."));
  Serial.println(F(" See the SafeString_readUntilToken() example sketch."));
  Serial.println();
  if (running) {
    Serial.println(F(" Counter Started"));
  }
}

void handleStartCmd() {
  running = true;  Serial.print(F("start at Counter:")); Serial.println(loopCounter);
}
void handleStopCmd() {
  running = false; Serial.print(F("stop at Counter:")); Serial.println(loopCounter);
}
void handleResetCmd() {
  loopCounter = 0; Serial.print(F("reset Counter:")); Serial.println(loopCounter);
}

void loop() {
  if (input.read(Serial)) {  // read from Serial, returns true if at least one character was added to SafeString input
    input.debug("after read => ");
  }

  if (skipToDelimiter) {
    if (input.endsWithCharFrom(delimiters)) {
      skipToDelimiter = false; // found next delimiter
    }
    input.clear(); // discard these chars

  } else {
    if (input.nextToken(token, delimiters)) { // process at most one token per loop does not return tokens longer than input.capacity()
      token.debug("after nextToken => ");

      if (token == startCmd) {
        handleStartCmd();
      } else if (token == stopCmd) {
        handleStopCmd();
      } else if (token == resetCmd) {
        handleResetCmd();
      }// else  // not a valid cmd ignore

    } else { // no terminating delimiter found
      if (input.isFull()) {
        // SafeString is full of chars but no delimiter discard the chars and skip input until get next delimiter
        skipToDelimiter = true;
        input.clear();
      }
    }
  }
  // rest of code here is executed while the user typing in commands
  if (running) {
    loopCounter++;
    if ((loopCounter % 100000) == 0) {
      Serial.print(F("Counter:")); Serial.println(loopCounter);
    }
  }
}
