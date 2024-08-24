/*
  Appending to SafeStrings using the += operator and concat()
  Examples of how to append different data types to SafeStrings
  Also has examples of using hasError() method

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

  Serial.println(F("Assignment to a SafeString and append with += and concat():"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose output "));
  SafeString::setOutput(Serial); // enable debugging error msgs
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  Serial.println();

  Serial.println(F("Examples of assignment. Also see the SafeStringPrint example for extra format control."));
  stringOne = "Hello SafeString";
  stringOne.debug("stringOne = \"Hello SafeString\"; => ");

  // converting a constant char into a String:
  stringOne = 'a';
  stringOne.debug("stringOne = 'a'; => ");

  // converting a F() constant string into a String object:
  stringOne =  F("This is a string");
  stringOne.debug("stringTwo =  F(\"This is a string\"); => ");


  // using a constant integer:
  stringOne =  13;
  stringOne.debug("stringOne =  13; => ");

  // using a long
  stringOne = millis();
  stringOne.debug("stringOne = millis(); => ");
  // prints "123456" or whatever the value of millis() is:

  // using a float :
  stringOne = 5.698;
  stringOne.debug("stringOne = 5.698; => ");
  Serial.println();


  Serial.println(F("Examples of appending using += "));
  stringOne = F("Sensor "); // can initialize with F()
  stringTwo = "value"; // or just " "
  Serial.println();

  Serial.println(stringOne);  // prints  "Sensor "

  // adding a string to a String:
  stringOne += stringTwo;
  Serial.println(stringOne);  // prints "Sensor value"

  // adding a constant string to a String:
  stringOne += " for input ";
  Serial.println(stringOne);  // prints "Sensor value for input"

  // adding a constant character to a String:
  stringOne += 'A';
  Serial.println(stringOne);   // prints "Sensor value for input A"

  // adding a constant integer to a String:
  stringOne += 0;
  Serial.println(stringOne);   // prints "Sensor value for input A0"

  // adding a constant string to a String:
  stringOne += ": ";
  Serial.println(stringOne);  // prints "Sensor value for input A0:"

  // adding a variable integer to a String:
  stringOne += analogRead(A0);
  Serial.println(stringOne);   // prints "Sensor value for input A0: 456" or whatever analogRead(A0) is

  Serial.println();
  Serial.println(F("The concat() method can be used instead of += and it is chainable"));
  Serial.println(F(" e.g. Serial.println( stringOne.clear().concat(\"Sensor \").concat(F(\"value: \")).concat(analogRead(A0)) );    outputs"));
  Serial.println( stringOne.clear().concat("Sensor ").concat(F("value: ")).concat(analogRead(A0)));

  Serial.println();
  Serial.println(F("Error checking.."));
  Serial.println(F("  hasError() returns true if any error detected. "));
  Serial.println(F("  each call to hasError() clears the internal errorFlag for that SafeString "));
  Serial.println(F("  each SafeString has its own errorFlag SafeString::errorDetected()"));
  Serial.println(F("  hasError() and SafeString::errorDetected() ALWAYS detects errors, even if SafeString::setOutput( ) has NOT been called "));
  Serial.println();

  Serial.println(F("stringTwo.concat('\\0');"));
  stringTwo.concat('\0');
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F(" Each call to hasError() and SafeString::errorDetected() clears it"));
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringTwo += '\\0';"));
  stringTwo += '\0';
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char *nullPtr = NULL;
  Serial.println(F("char *nullPtr = NULL;"));
  Serial.println(F("stringTwo.concat(nullPtr);"));
  stringTwo.concat(nullPtr);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringTwo += nullPtr;"));
  stringTwo += nullPtr;
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("char testChars[] = \"test characters\";"));
  char testChars[] = "test characters";

  Serial.println(F("stringOne.concat(testChars,24);"));
  stringOne.concat(testChars, 24);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.concat(F(\"This is a long string\"),30);"));
  stringOne.concat(F("This is a long string"), 30);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("stringOne.clear();"));
  stringOne.clear();
  Serial.println(F("stringOne.concat(F(\"This is a long F(string)\");"));
  stringOne.concat(F("This is a very long F(string) "));
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.concat(\"This is a another very long string\");"));
  stringOne.concat("This is a another very long string ");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char str[] = "A test char[]";
  Serial.println(F(" Wrap str[] in a SafeString and print it out using Serial.println(sfStr);"));
  createSafeStringFromCharArray(sfStr, str);
  Serial.println(sfStr);
  Serial.println(F(" Use strcat to cause a buffer overrun, i.e. strcat(str,\"more text\");"));
  strcat(str, "more text");
  Serial.println(F(" Serial.println(sfStr); again"));
  Serial.println(sfStr);
  Serial.print(F("sfStr.hasError():"));  Serial.println(sfStr.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {

}
