/*
  Prefixing to SafeStrings using the -= operator and prefix()
  Examples of how to prefix different data types to SafeStrings

  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"
createSafeString(stringOne, 35);
createSafeString(stringTwo, 30);

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();

  Serial.println("Prefixing to a SafeString:");
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  SafeString::setOutput(Serial); // enable debugging error msgs
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  Serial.println();


  Serial.println(F("Examples of prefixing using -= "));
  Serial.println(F(" See also the SafeStringAssignmentAndConcatOperator example for += and concat()"));
  // adding a variable integer to a String:
  stringOne = analogRead(A0);
  Serial.println(stringOne);   // prints "456" or whatever analogRead(A0) is

  // adding a constant string to a String:
  stringOne -= ": ";
  Serial.println(stringOne);  // prints ": 456"

  // adding a constant integer to a String:
  stringOne -= 0;
  Serial.println(stringOne);   // prints "0: 456"

  // adding a constant character to a String:
  stringOne -= 'A';
  Serial.println(stringOne);   // prints "A0: 456"

  // adding a constant string to a String:
  stringOne -= " for input ";
  Serial.println(stringOne);  // prints " for input A0: 456"

  // adding a string to a String:
  stringTwo = "value";
  stringOne -= stringTwo;
  Serial.println(stringOne);  // prints "value for input A0: 456"

  stringOne -= F("Sensor ");
  Serial.println(stringOne);  // prints "Sensor value for input A0: 456"

  Serial.println();
  Serial.println(F("char testChars[] = \"test characters\";"));
  char testChars[] = "test characters";
  Serial.println(F("stringOne.clear();"));
  stringOne.clear();
  Serial.println(F("stringOne.prefix(testChars,4); // prefix using the first 4 chars"));
  stringOne.prefix(testChars, 4).debug();
  Serial.println();

  Serial.println(F("stringOne.prefix(F(\"This is a long string\"),10); // prefix using the first 10 chars"));
  stringOne.prefix(F("This is a long string"), 10).debug();

  Serial.println();
  Serial.println(F("The prefix() method can be used instead of -= and it is chainable"));
  Serial.println(F(" e.g. Serial.println( stringOne.clear().prefix(analogRead(A0)).prefix(F(\"value: \")).prefix(\"Sensor \") );    outputs"));
  Serial.println( stringOne.clear().prefix(analogRead(A0)).prefix(F("value: ")).prefix("Sensor ") );

  Serial.println();
  Serial.println(F("Error checking.."));
  Serial.println();

  Serial.println(F("stringTwo.prefix('\\0');"));
  stringTwo.prefix('\0');
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringTwo -= '\\0';"));
  stringTwo -= '\0';
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char *nullPtr = NULL;
  Serial.println(F("char *nullPtr = NULL;"));
  Serial.println(F("stringTwo.prefix(nullPtr);"));
  stringTwo.prefix(nullPtr);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringTwo -= nullPtr;"));
  stringTwo -= nullPtr;
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.prefix(testChars,24);"));
  stringOne.prefix(testChars, 24);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.prefix(F(\"This is a long string\"),30);"));
  stringOne.prefix(F("This is a long string"), 30);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringOne.prefix(F(\"This is a long F(string)\");"));
  stringOne.prefix(F("This is a very long F(string) "));
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.prefix(\"This is a another very long string\");"));
  stringOne.prefix("This is a another very long string ");
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {

}
