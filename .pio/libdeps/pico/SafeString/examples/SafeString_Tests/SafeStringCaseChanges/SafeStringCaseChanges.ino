/*
  SafeString Case changes
  Examples of how to change the case of a SafeString

  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString case changes, toUpperCase() and toLowerCase()"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose true "));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable debugging error msgs
  Serial.println(F("  Actually toUpperCase() and toLowerCase() can not give any errors."));

  Serial.println();
  createSafeString(stringOne, 20, "<html><head><body>");
  stringOne.debug();
  Serial.println();

  // toUpperCase() changes all letters to upper case:
  stringOne.toUpperCase();
  stringOne.debug(F("stringOne.toUpperCase(); => "));
  Serial.println();

  // toLowerCase() changes all letters to lower case:
  Serial.println(F(" Now convert back to lower case"));
  stringOne.toLowerCase();
  stringOne.debug(F("stringOne.toLowerCase(); => "));
}

void loop() {

}
