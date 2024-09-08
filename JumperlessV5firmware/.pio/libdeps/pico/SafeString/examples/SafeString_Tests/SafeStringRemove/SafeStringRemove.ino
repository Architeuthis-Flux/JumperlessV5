/*
  SafeString removeFrom(), removeBefore(), remove(), removeLast(), keepLast()
  Examples of SafeString remove

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"
createSafeString(stringOne, 20, "<html><head><body>");
createSafeString(stringTwo, 30);

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();

  Serial.println(F("SafeString  removeFrom(), removeBefore(), remove(), removeLast(), keepLast() usage"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable verbose debugging error msgs
  Serial.println();

  stringOne.debug();
  Serial.println();
  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  size_t idx = stringTwo.indexOf("<head>");
  stringTwo.removeFrom(idx);
  Serial.println(F("size_t idx = stringTwo.indexOf(\"<head>\")"));
  Serial.println(F(" remove all chars from idx to end of string"));
  stringTwo.debug(F("stringTwo.removeFrom(idx); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  stringTwo.removeBefore(idx);
  Serial.println(F(" remove chars from before idx "));
  stringTwo.debug(F("stringTwo.removeBefore(idx); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  stringTwo.remove(idx, 6);
  Serial.println(F(" remove 6 chars from idx onwards"));
  stringTwo.debug(F("stringTwo.remove(idx,6); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  Serial.println(F(" removeLast 6 chars"));
  stringTwo.removeLast(6);
  stringTwo.debug(F("stringTwo.removeLast(6); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  Serial.println(F(" keepLast 6 chars"));
  stringTwo.keepLast(6);
  stringTwo.debug(F("stringTwo.keepLast(6); => "));
  Serial.println();


  Serial.println(F("Error checking.."));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));

  Serial.println(F(" idx == stringTwo.length() for removeFrom is OK, nothing removed"));
  stringTwo.removeFrom(stringTwo.length());
  stringTwo.debug(F("stringTwo.removeFrom(stringTwo.length()); => "));
  Serial.println();

  stringTwo = stringOne;
  Serial.println(F(" idx > stringTwo.length() is error, stringTwo unchanged"));
  Serial.println(F("stringTwo.removeFrom(stringTwo.length()+1);"));
  stringTwo.removeFrom(stringTwo.length() + 1);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F(" idx == stringTwo.length() for removeBefore is OK, everything removed"));
  stringTwo.removeBefore(stringTwo.length());
  stringTwo.debug(F("stringTwo.removeBefore(stringTwo.length()); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  Serial.println(F(" idx > stringTwo.length() is error, stringTwo unchanged"));
  Serial.println(F("stringTwo.removeBefore(stringTwo.length()+1);"));
  stringTwo.removeBefore(stringTwo.length() + 1);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  Serial.println(F(" idx + count > stringTwo.length() is error, stringTwo unchanged"));
  Serial.println(F("stringTwo.remove( 10, 9);"));
  stringTwo.remove( 10, 9);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));

  Serial.println(F(" removeLast(count), count > length() is error, stringTwo unchanged."));
  Serial.println(F("stringTwo.removeLast(19);"));
  stringTwo.removeLast(19);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));

  Serial.println(F(" keepLast(count), count > length() is error, stringTwo unchanged."));
  Serial.println(F("stringTwo.keepLast(19);"));
  stringTwo.keepLast(19);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));

  Serial.println(F(" remove(-1) is valid and just returns with out an error"));
  Serial.println(F("stringTwo.remove(-1);"));
  stringTwo.remove(-1);
  stringTwo.debug();
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F(" removeBefore(-1) is valid and clears whole SafeString "));
  Serial.println(F("stringTwo.removeBefore(-1);"));
  stringTwo.removeBefore(-1);
  stringTwo.debug();
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {
}
