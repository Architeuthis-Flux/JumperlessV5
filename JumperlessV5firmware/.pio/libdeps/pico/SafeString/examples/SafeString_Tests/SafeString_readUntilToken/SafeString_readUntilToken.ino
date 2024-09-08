// SafeString_readUntilToken.ino
//
// This example can either read data from the Serial input OR from a pre-defined test data SafeString
// #define TEST_DATA determines which. Currently set to read from test data.
//
// For Serial input this example takes commmands from the Arduino Monitor input and acts on them
// the available commands are start stop and reset
// Commands are delimited by space dot comma NL or CR
// If you set the Arduino Monitor to No line ending then the last command will be ignored until it is terminated by a space or ,
//  Use the settings Newline or Carrage Return or Both NL & CR
//
// These commands can be picked out of a line of user input
// start  stop  reset
//
// download and install the SafeString library from
// www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include "SafeString.h"
#include "SafeStringStream.h"

#define TEST_DATA

#ifdef TEST_DATA
SafeStringStream sfStream;
cSF(sfTestData, 128);
#endif

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
  Serial.println(F(" The SafeString readUntilToken() method does all the work. By default it echos the input, but you can turn that off."));
  Serial.println(F(" See the SafeString_read() example sketch for the internals of readUntilToken()."));
  Serial.println();
  if (running) {
    Serial.println(F(" Counter Started"));
  }
#ifdef TEST_DATA
  sfTestData = F("looooooooooooong stop input, start,, nothing, stop, reset\n"); // initialized the test data
  //sfTestData.debug();
  Serial.println("Using test data input");
  Serial.println(sfTestData);
  sfStream.begin(sfTestData, 1200);
#define StreamInput sfStream
#else
  Serial.println("Reading input from IDE monitor, Serial");
#define StreamInput Serial
#endif

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
  if (input.readUntilToken(StreamInput, token, delimiters, skipToDelimiter, true)) { // echo true on makes TEST_DATA, sfStream, loop continually as read chars are just written back on the end
    if (token == startCmd) {
      handleStartCmd();
    } else if (token == stopCmd) {
      handleStopCmd();
    } else if (token == resetCmd) {
      handleResetCmd();
    }
  } // else token is empty

  // rest of code here is executed while the user typing in commands
  if (running) {
    loopCounter++;
    if ((loopCounter % 100000) == 0) {
      Serial.print(F("Counter:")); Serial.println(loopCounter);
    }
  }
}
