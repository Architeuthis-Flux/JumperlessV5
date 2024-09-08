/*
  SafeSting length(), trim()

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
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
  Serial.println(F("SafeString length(), trim() functions"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs
  Serial.println();

  // here's a String with empty spaces at the end (called white space):
  createSafeString(stringOne, 35, "  Hello!       ");
  stringOne.debug();
  Serial.print(F("stringOne.length() = ")); Serial.println(stringOne.length());
  Serial.println();

  stringOne.trim();
  stringOne.debug(F("stringOne.trim(); => "));
  Serial.print(F("stringOne.length() = ")); Serial.println(stringOne.length());
  Serial.println();

  Serial.println(F(" c_str() gives to access to the underly char array as a const char* so you cannot change the contents"));
  Serial.println();

  Serial.print(F("stringOne.c_str() = ")); Serial.println(stringOne.c_str());
  Serial.println();


}

void loop() {
}
