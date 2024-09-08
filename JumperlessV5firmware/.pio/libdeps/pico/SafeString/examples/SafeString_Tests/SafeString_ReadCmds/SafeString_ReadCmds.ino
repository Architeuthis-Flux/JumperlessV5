// SafeString_ReadCmds.ino
//
// This example takes commmands from the Arduino Monitor input and acts on them
// the available commands are start stop and reset
// Commands are delimited by space comma NL or CR, OR no more input for 2sec
// You can use any Arduino Monitor to line ending setting
//
// These commands can be picked out of a line of user input
// start  stop  reset
//
// download and install the SafeString library from the library manager OR
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
bool echoOn = true; // set to false to suppress echo of chars read  defaults to false if missing
unsigned long timeOut = 2000; // auto terminate input if nothing more received for 2 sec. // defaults to no timeout


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
  Serial.println(F("  an input timeout has been set so the"));
  Serial.println(F(" Arduino monitor line ending can be set to No line Ending, or Newline or Carrage Return or Both NL & CR."));
}

void handleStartCmd() {
  Serial.println(); Serial.println(F(" Found startCmd"));
}
void handleStopCmd() {
  Serial.println(); Serial.println(F(" Found stopCmd"));
}
void handleResetCmd() {
  Serial.println(); Serial.println(F(" Found resetCmd"));
}

void loop() {
  if (input.readUntilToken(Serial, token, delimiters, skipToDelimiter, echoOn, timeOut)) {
    // if (input.readUntilToken(Serial, token, delimiters)) {  // has defaults of skipToDelimiter == false, echoOn == false, and timeOut == 0 (i.e no timeout)
    if (token == startCmd) {
      handleStartCmd();
    } else if (token == stopCmd) {
      handleStopCmd();
    } else if (token == resetCmd) {
      handleResetCmd();
    }
  } // else token is empty

  // rest of code here is executed while the user typing in commands
}
