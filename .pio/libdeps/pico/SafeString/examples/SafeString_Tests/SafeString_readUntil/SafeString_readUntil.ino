/*
  SafeString readUntil, non-blocking until delimiter found
  Example of how to use the non-blocking readUntil() method to parse a CSV line

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include "SafeString.h"

/** test inputs  copy and past to the monitor input with Newline or Carrage Return or Both NL & CR settings
**   this one has an empty final field
  5.33 , abcd,33 ,test,
** this one has a non empty final field
  5.33 , abcd,33 ,test, 66
** this one has fields which are too long
  1234567890123abcdefghijklmn,441234567890123,33
****/
const size_t inputBufferSize = 10;
createSafeString(input, inputBufferSize); // SafeString to collect Stream input. Should have capacity > largest field length + 1 for delimiter
char delimiters[] = ",\n\r"; // , newline or carrage return are delimiters
int fieldCount = 1;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();

  Serial.println();
  Serial.println(F("Using SafeString readUntil() to read a comma separated line from Serial and parse it into fields."));
  Serial.println(F("  The input SafeString is only a capacity 10 >= the size of the largest field + 2 delimiters."));
  Serial.println(F("  This sketch assumes CSV lines are terminated by Newline (\\n) or Carrage Return (\\r) or Both "));
  Serial.println(F("  Example inputs ... empty final field, non-empty final field and fiels are that too long "));
  Serial.println(F("5.33 , abcd,33 ,test,"));
  Serial.println(F("5.33 , abcd,33 ,test, 66"));
  Serial.println(F("1234567890123abcdefghijklmn,441234567890123,33"));
  Serial.println();
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  //SafeString::setOutput(Serial); // uncomment this line to see the input debugging

  Serial.println("Input data terminated by Newline or Carrage Return or Both");
}

bool processingOverflowField = false;
// this is only called if input full or ends in a delimiter
void processField() {
  cSF(field, inputBufferSize); // temporary SafeString to
  if (!input.endsWithCharFrom(delimiters)) { //  input full/overflowed
    if (!processingOverflowField) { // start processing overflow
      Serial.print(F(" Field ")); Serial.print(F(" overflowed '"));
    }
    // else just continue processing overflow
    input.nextToken(field, delimiters); // skips any leading delimiters but leaves rest of input since no terminating delimiter
    Serial.print(input); // print what is left after skipping leading delimiters
    input.clear();
    processingOverflowField = true;
    return;
  }
  // else have delimiter since readUntil only returns true when a delimiter is read or input full
  //bool endOfLine = input.endsWithCharFrom("\n\r"); // is this the end of the line  test this here as input may be just \n or \r
  input.nextToken(field, delimiters); // skip delimiters and pick up and remove the field.  Leaves space for more input
  // returns true if not just delimiters
  if (processingOverflowField) {
    // finish outputing overflow field
    Serial.print(field); Serial.println('\'');
  } else {
    if (!field.isEmpty()) {
      Serial.print(F(" Field '")); Serial.print(field); Serial.println('\'');
    }
  }
  processingOverflowField = false;
}

void loop() {
  if (input.readUntil(Serial, delimiters)) { // returns true if delimiter found or if input is full
    input.debug(F(" readUntil => "));
    processField();
  }
}
