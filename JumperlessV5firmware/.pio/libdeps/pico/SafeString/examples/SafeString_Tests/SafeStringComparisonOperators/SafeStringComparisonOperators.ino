/*
  Comparing SafeStrings
  Examples of how to compare SafeStrings using the comparison operators

  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"
createSafeString(stringOne, 20);
createSafeString(stringTwo, 20);

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("Comparing SafeStrings:"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true "));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable debugging error msgs


  stringOne = F("this");
  stringTwo = "that";
  Serial.println();

  // two SafeStrings equal:
  if (stringOne == "this") {
    Serial.println("stringOne == \"this\"");
  }
  // two SafeStrings not equal:
  if (stringOne != stringTwo) {
    Serial.print(stringOne); Serial.print(" != "); Serial.println(stringTwo);
  }
  Serial.println(F(" comparing two SafeStrings is faster because the length()'s are checked first"));


  // two SafeStrings not equal (case sensitivity matters):
  stringOne = "This";
  stringTwo = "this";
  if (stringOne != stringTwo) {
    Serial.print(stringOne); Serial.print(" != "); Serial.println(stringTwo);
  }
  // you can also use equals() to see if two SafeStrings are the same:
  if (stringOne.equals(stringTwo)) {
    Serial.print(stringOne); Serial.print(" equals "); Serial.println(stringTwo);
  } else {
    Serial.print(stringOne); Serial.print(" does not equal "); Serial.println(stringTwo);
  }

  // or perhaps you want to ignore case:
  if (stringOne.equalsIgnoreCase(stringTwo)) {
    Serial.print(stringOne); Serial.print(" equals (ignoring case) "); Serial.println(stringTwo);
  } else {
    Serial.print(stringOne); Serial.print(" does not equal (ignoring case) "); Serial.println(stringTwo);
  }

  // two numeric SafeStrings compared:
  stringOne = "2";
  stringTwo = "1";
  if (stringOne >= stringTwo) {
    Serial.print(stringOne); Serial.print(" >= "); Serial.println(stringTwo);
  }

  // comparison operators can be used to compare SafeStrings for alphabetic sorting too:
  stringOne = "Brown";
  if (stringOne < "Charles") {
    Serial.print(stringOne); Serial.println(" < Charles");
  }

  if (stringOne > "Adams") {
    Serial.print(stringOne); Serial.println(" > Adams");
  }

  if (stringOne <= "Browne") {
    Serial.print(stringOne); Serial.println(" <= Browne");
  }

  if (stringOne >= "Brow") {
    Serial.print(stringOne); Serial.println(" >= Brow");
  }

  // the compareTo() operator also allows you to compare SafeStrings
  // it evaluates on the first character that's different.
  // if the first character of the SafeString you're comparing to comes first in
  // alphanumeric order, then compareTo() is greater than 0:
  stringOne = "Cucumber";
  stringTwo = "Cucuracha";
  if (stringOne.compareTo(stringTwo) < 0) {
    Serial.print(stringOne); Serial.print(" comes before "); Serial.println(stringTwo);
  } else {
    Serial.print(stringOne); Serial.print(" comes after "); Serial.print(stringTwo);
  }
  Serial.println();

  Serial.println(F("Compare two readings, as SafeStrings"));

  int count = 0;
  while (count < 10) {
    count++;
    stringOne = "Sensor: ";
    stringTwo = "Sensor: ";

    stringOne += analogRead(A0);
    stringTwo += analogRead(A5);

    if (stringOne.compareTo(stringTwo) < 0) {
      Serial.print(stringOne); Serial.print(" comes before "); Serial.println(stringTwo);
    } else {
      Serial.print(stringOne); Serial.print(" comes after "); Serial.println(stringTwo);
    }
  }
  Serial.println(F(" FINISHED"));

  Serial.println();
  Serial.println(F("Error checking.."));
  Serial.println();

  char *nullPtr = NULL;
  Serial.println(F("char *nullPtr = NULL;"));
  Serial.println(F("stringTwo.compareTo(nullPtr);"));
  int result = stringTwo.compareTo(nullPtr);
  Serial.print(F("stringTwo.compareTo(nullPtr) returns ")); Serial.print(result); Serial.println(F("  Any SafeString is always > NULL "));
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringTwo < nullPtr;"));
  result = stringTwo < nullPtr;
  Serial.print(F("stringTwo < nullPtr; returns ")); Serial.print(result); Serial.println(F("  Any SafeString is always > NULL "));
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringTwo == nullPtr;"));
  result = stringTwo == nullPtr;
  Serial.print(F("stringTwo == nullPtr; returns ")); Serial.print(result); Serial.println(F("  A SafeString is never NULL "));
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {
}
