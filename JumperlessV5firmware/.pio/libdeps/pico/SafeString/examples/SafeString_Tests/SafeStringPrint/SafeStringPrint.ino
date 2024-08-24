/*
  Appending to SafeStrings using print()/println()
  Examples of how to use the standard print()/println() to append and format different data types to SafeStrings

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

  Serial.println(F("Printing to a SafeString"));
  Serial.println(F("Using the well known print() methods is the easiest and most flexiable way to build up a SafeString"));
  Serial.println(F(" Also see the SafeString_fixedWidthFormat example for padding numbers prints to a fixed width, left or right alinged"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs
  Serial.println();

  createSafeString(stringOne, 20);
  stringOne.debug(F("createSafeString(stringOne, 20); => "));
  Serial.println();

  stringOne.print((unsigned char)'a', HEX);
  stringOne.debug(F("stringOne.print((unsigned char)'a'), HEX); => "));
  Serial.println();

  stringOne.print(F("  "));
  stringOne.debug(F("stringOne.print(F(\"  \")); => "));
  Serial.println();

  stringOne.clear();
  stringOne.debug(F("stringOne.clear(); => "));
  stringOne.println(1.0 / 3.0, 7);
  stringOne.debug(F("stringOne.println(1.0 / 3.0, 7); => "));
  Serial.println();


  createSafeString(stringTwo, 25);
  stringTwo.debug(F("createSafeString(stringTwo, 25); => "));
  Serial.println();

  stringTwo.print(" str2: ");
  stringTwo.debug(F("stringTwo.print(\" str2: \"); => "));
  Serial.println();

  stringTwo.print(stringOne);
  stringTwo.debug(F("stringTwo.print(stringOne); => "));
  Serial.println();

  stringTwo.clear();
  stringTwo.debug(F("stringTwo.clear(); => "));
  stringTwo.print((unsigned long) - 1);
  stringTwo.debug(F("stringTwo.print((unsigned long)-1); => "));
  Serial.println();

  Serial.println(F("Error checking.."));
  Serial.println();
  Serial.println(F("You can print to yourself if there is enough room"));
  stringOne = "abc_";
  stringOne.debug(F("stringOne = \"abc_\"; => "));
  stringOne.print(stringOne);
  stringOne.debug(F("stringOne.print(stringOne); => "));
  Serial.println();

  Serial.println("stringOne.print('\\0');");
  stringOne.print('\0');
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println("stringOne.write(0);");
  stringOne.write(0);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  uint8_t bytes[] = { 35, 33, 0, 55, 66 };
  Serial.println(F("uint8_t bytes[] = { 35, 33, 0, 55, 66 };"));
  Serial.println("stringOne.write(bytes,sizeof(bytes));  // try to write byte array containing a 0 byte i.e. '\\0' ");
  stringOne.write(bytes, sizeof(bytes));
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char *nullStr = NULL;
  Serial.println("stringOne.print(nullStr);");
  stringOne.print(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  createSafeString(smallSafeStr, 2);
  smallSafeStr.debug(F("createSafeString(smallSafeStr, 2); => "));
  Serial.println("smallSafeStr.println('a'); // i.e. append char a and \\r\\n");
  smallSafeStr.println('a');
  Serial.print(F("smallSafeStr.hasError():"));  Serial.println(smallSafeStr.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();



}

void loop() {
  // nothing here
}
