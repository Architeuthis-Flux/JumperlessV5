// SafeStringReader_CmdsTimed.ino
//
// Example of NON-Blocking read commmands from the Arduino Monitor input and acts on them
// the available commands are start stop
// the commands and their functions are stored in struct array
//
// Commands are delimited by space dot comma NL or CR, OR if there is no more input for 2secs
// If you set the Arduino Monitor to No line ending then the last command will be processed after 2sec
//
// These commands can be picked out of a line of user input
// start  stop reset
// The input line can be as long as you like 100's of Kb long, but only two small buffers need to parse the commands
//
// download and install the SafeString library from
// www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include "SafeStringReader.h"

// create an sfReader instance of SafeStringReader class
// that will handle commands upto 5 chars long
// delimited by space, comma or CarrageReturn or NewLine
// the createSafeStringReader( ) macro creates both the SafeStringReader (sfReader) and the necessary SafeString that holds input chars until a delimiter is found
// args are (ReaderInstanceName, expectedMaxCmdLength, delimiters)
const unsigned int MAX_CMD_LENGTH = 5;
createSafeStringReader(sfReader, MAX_CMD_LENGTH, " ,\r\n");

bool running = true;
unsigned long loopCounter = 0;

void startCmd(SafeString& sfStr) {
  running = true;
  sfStr.println(); sfStr.print(F("> start at Counter:")); sfStr.println(loopCounter);
}
void stopCmd(SafeString& sfStr) {
  running = true;
  sfStr.println(); sfStr.print(F("> stop at Counter:")); sfStr.println(loopCounter);
}
void resetCmd(SafeString& sfStr) {
  loopCounter = 0;
  sfStr.println(); sfStr.print(F("> reset Counter:")); sfStr.println(loopCounter);
}

typedef struct {
  const char* cmd;
  void (*cmdFn)(SafeString&); // a method taking a SafeString& arg and returning void
} COMMANDS_STRUCT;

COMMANDS_STRUCT commands[] = {
  {"start", startCmd},
  {"stop", stopCmd},
  {"reset", resetCmd}
};

const size_t NO_OF_CMDS = sizeof commands / sizeof commands[0];

void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) { // pause a little to give you time to open the Arduino monitor
    Serial.print(i); Serial.print(' '); delay(500);
  }
  Serial.println();
  SafeString::setOutput(Serial); // enable error messages and SafeString.debug() output to be sent to Serial

  // check MAX_CMD_LENGTH
  size_t maxCmdLen = 0;
  for (size_t i = 0; i < NO_OF_CMDS; i++) {
    size_t cmdLen = strlen(commands[i].cmd);
    if (cmdLen > maxCmdLen) {
      maxCmdLen = cmdLen;
    }
  }
  if (maxCmdLen > MAX_CMD_LENGTH) {
    while (1) {
      Serial.print("MAX_CMD_LENGTH needs to be at least "); Serial.println(maxCmdLen);
      delay(3000); // stop here
    }
  }
  Serial.print(F(" Valid cmds are:-"));
  for (size_t i = 0; i < NO_OF_CMDS; i++) {
    Serial.print(" "); Serial.print(commands[i].cmd);
  }
  Serial.println();

  if (running) {
    Serial.println(F(" Counter Started"));
  }
  sfReader.connect(Serial); // where SafeStringReader will read from
  sfReader.echoOn(); // echo back all input, by default echo is off
  sfReader.setTimeout(1000); // 1000ms = 1sec timeout
  //sfReader.returnEmptyTokens(); if you want ,, etc to return true from sfReader.read() with an empty sfReader
}

// Check token against valid commands
void processCmd(SafeString & token) {
  for (size_t i = 0; i < NO_OF_CMDS; i++) {
    if (token == commands[i].cmd) { // found one
      cSF(sfMsg, 30);
      commands[i].cmdFn(sfMsg); // call the cmd fn
      Serial.println(sfMsg);
      return; // found cmd
    }
  }
  // else no cmd found have finished processing this token, clear it so it is not output
  Serial.println(F(" Not valid cmd "));
}

void loop() {
  if (sfReader.read()) {
    processCmd(sfReader); //sfReader holds the tokenized command
  } // else no delimited command yet

  // rest of code here is executed while the user typing in commands
  if (running) {
    loopCounter++;
    if ((loopCounter % 100000) == 0) { // print the current counter every now and again
      Serial.print(F("Counter:")); Serial.println(loopCounter);
    }
  }
}
