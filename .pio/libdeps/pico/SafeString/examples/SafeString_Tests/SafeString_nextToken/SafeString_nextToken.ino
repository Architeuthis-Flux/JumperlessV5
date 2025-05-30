/*
  Tokenizing SafeStrings and converting to numbers
  Examples of how to use the nextToken() and toLong() and toDouble() to parse a CSV line

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include "SafeString.h"

char line[] = ",23.5, 44a ,, , -5. , +.5, 7a, 33,fred5, 6.5.3, a.5,b.3\n";
char delimiters[] = ","; // just comma for delimiter, could also use ",;" if comma or semi-colon seperated fields

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("Using SafeString nextToken() to parse a comma separated line for the numbers it contains."));
  Serial.println(F("SafeString::setOutput(Serial); // verbose"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs

  createSafeString(sfLine, 64); // a SafeString large enough to hold the whole line.
  sfLine = line; // initialize the SaftString for processing
  Serial.print(F("The sfLine line is '")); Serial.print(sfLine); Serial.println('\'');
  Serial.println(F("The delimiter is comma"));
  Serial.println(F("Note: By default the last field will be returned even if not terminated by a delimiter.  The optional returnLastNonDelimitedToken argument controls this."));
  createSafeString(field, 10); // for the field strings. Should have capacity > largest field length
  Serial.println();
  Serial.println(F("Fields with numbers are:-  (returning empty fields, last field and an empty first field if first char is a delimiter)"));
  bool haveToken = sfLine.firstToken(field, delimiters); // will return true and empty field if first char is ,
  // bool haveToken = sfLine.nextToken(field, delimiters, true, true, true); // firstToken === nextToken with the last (optional) arg true
  while (haveToken) {
    double d;
    if (field.toDouble(d)) {
      Serial.println(d);
    } else {
      Serial.print(F("  Field '")); Serial.print(field); Serial.println(F("' is not a number"));
    }
    haveToken = sfLine.nextToken(field, delimiters, true);
  }
  Serial.println();

  Serial.println(F("After processing by nextToken() the Input line is empty because nextToken() removes the tokens and delimiters from the line being processed."));
  sfLine.debug();

  Serial.println();
  Serial.println(F("sfLine = line; // re-initialize the line since nextToken removes the tokens and delimiters"));
  sfLine = line; // re-initialize the line since nextToken removes the tokens and delimiters
  Serial.print(F("The untrimmed Input line is '")); Serial.print(sfLine); Serial.println('\'');
  createSafeString(sDelimiters, 4, ",");
  sDelimiters.debug(F(" Test using a SafeString for the delimiters  --  "));
  Serial.println(F(" The fields with integers are:-  (NOT returning empty fields)"));
  haveToken = sfLine.nextToken(field, delimiters); // will not return empty field if first char is ,
  while (haveToken) {
    long l_num;
    if (field.toLong(l_num)) {
      Serial.println(l_num);
    } else {
      Serial.print(F("  Field ")); Serial.print(field); Serial.println(F(" is not an integer number"));
    }
    haveToken = sfLine.nextToken(field, delimiters);
  }
  Serial.println();


  Serial.println(F("Error checking.."));
  Serial.println();

  Serial.println(F("Check if field SafeString not large enough for token"));
  Serial.println(F("sfLine = line;"));
  sfLine = line;
  createSafeString(smallField, 2);
  Serial.println(F("bool rtn = sfLine.nextToken(smallField, delimiters);"));
  bool rtn = sfLine.nextToken(smallField, delimiters);
  if (rtn) {
    smallField.debug(F(" the rtn == true, but smallField is empty."));
  }
  Serial.println(F(" If the SafeString argument is too small, nextToken() returns true and removes the token, but returns an empty smallField"));
  sfLine.debug();
  Serial.println(F(" This is to prevent token processing loops like "));
  Serial.println(F("while (sfLine.nextToken(smallField,delimiters)) {"));
  Serial.println(F(" looping forever "));
  Serial.println();
  Serial.println(F(" Also hasError() is set for both sfLine and smallField"));
  Serial.print(F(" sfLine.hasError() => ")); Serial.println(sfLine.hasError() ? "true" : "false");
  Serial.print(F(" smallField.hasError() => ")); Serial.println(smallField.hasError() ? "true" : "false");
  Serial.println();

  Serial.println(F("Check for empty delimitiers"));
  char emptyDelimiters[] = "";
  Serial.println(F("sfLine.nextToken(smallField,emptyDelimiters);"));
  sfLine.nextToken(smallField, emptyDelimiters);
  Serial.print(F(" sfLine.hasError() => ")); Serial.println(sfLine.hasError() ? "true" : "false");
  Serial.print(F(" smallField.hasError() => ")); Serial.println(smallField.hasError() ? "true" : "false");
  smallField.debug();
  Serial.println();

  Serial.println(F("Check if delimitiers NULL"));
  char *nullDelims = NULL;
  Serial.println(F("sfLine.nextToken(smallField,nullDelims);"));
  sfLine.nextToken(smallField, nullDelims);
  Serial.print(F(" sfLine.hasError() => ")); Serial.println(sfLine.hasError() ? "true" : "false");
  Serial.print(F(" smallField.hasError() => ")); Serial.println(smallField.hasError() ? "true" : "false");
  smallField.debug();
  Serial.println();

}

void loop() {
  // nothing here
}
