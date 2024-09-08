/*
  SafeString startsWith(), startsWithIgnoreCase(), endsWithCharFrom() and endsWith()

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include "SafeString.h"

createSafeString(string, 25);
createSafeString(stringOne, 35);

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString startsWith(), startsWithIgnoreCase(), endsWithCharFrom() and endsWith() functions"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs
  Serial.println();

  // startsWith() checks to see if a SafeString starts with a particular substring:
  stringOne = F("HTTP/1.1 200 OK");
  stringOne.debug(F("stringOne = F(\"HTTP/1.1 200 OK\"); => "));
  if (stringOne.startsWith("HTTP/1.1")) {
    Serial.println(F("stringOne.startsWith(\"HTTP/1.1\") returned true"));
  } else {
    Serial.println(F("stringOne does not start with 'HTTP/1.1'"));
  }
  Serial.println();

  Serial.println(F("Check startsWith starting from index 9"));
  if (stringOne.startsWith("200 OK", 9)) {
    Serial.println(F("stringOne.startsWith(\"200 OK\", 9) returned true"));
  } else {
    Serial.println(F("Did not find '200 OK' searching from index 9"));
  }
  Serial.println();

  stringOne = F("test aBc 553");
  stringOne.debug(F("stringOne = F(\"test aBc 553\"); => "));
  if (stringOne.startsWithIgnoreCase("abc", 5)) {
    Serial.println(F("stringOne.startsWithIgnoreCase(\"abc\", 5) returned true"));
  } else {
    Serial.println(F("Did not find 'abc' (ignoring case) searching from index 5"));
  }
  Serial.println();

  string = "abc";
  string.debug(F("string = \"abc\"; => "));
  if (stringOne.startsWith(string, 5)) {
    Serial.println(F("stringOne.startsWith(string, 5) returned true"));
  } else {
    Serial.println(F("stringOne.startsWith(string, 5) did not find string searching from index 5"));
  }
  Serial.println();
  if (stringOne.startsWithIgnoreCase(string, 5)) {
    Serial.println(F("stringOne.startsWithIgnoreCase(string, 5) returned true"));
  } else {
    Serial.println(F("Did not find string (ignoring case) searching from index 5"));
  }
  Serial.println();

  if (stringOne.endsWith("53")) {
    Serial.println(F("stringOne.endsWith(\"53\") returned true"));
  } else {
    Serial.println(F("Did not find '53' at end of stringOne"));
  }
  Serial.println();

  Serial.println(F("stringOne.startsWith(\"53\",12);"));
  if (stringOne.startsWith("53", 12)) {
    Serial.println(F("stringOne.startsWith(\"53\",12) returned true"));
  } else {
    Serial.println(F("fromIndex == stringOne.length() == 12 is a valid argument"));
    Serial.println(F("Did not find '53' starting from index 12"));
  }
  Serial.println();
  Serial.println(F("stringOne.startsWith(\"53\",-1);"));
  if (stringOne.startsWith("53", -1)) {
    Serial.println(F("stringOne.startsWith(\"53\",12) returned true"));
  } else {
    Serial.println(F("fromIndex == -1 is a valid argument"));
    Serial.println(F("Did not find '53' starting from index -1"));
  }
  Serial.println();

  if (stringOne.endsWithCharFrom("1234567890")) {
    Serial.println(F("stringOne.endsWithCharFrom(\"1234567890\") returned true"));
  } else {
    Serial.println(F("stringOne does not end with one of the chars \"1234567890\""));
  }
  Serial.println();

  Serial.println(F("Error checking.."));
  Serial.println();

  stringOne.debug();
  Serial.print(F("stringOne.startsWith(\"\"):"));  Serial.println(stringOne.startsWith("") ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.print(F("stringOne.startsWith(\"\",12):"));  Serial.println(stringOne.startsWith("", 12) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  cSF(stringTwo, 3);
  stringTwo.debug(F(" Empty stringTwo "));
  Serial.print(F("stringTwo.startsWith(\"\"):"));  Serial.println(stringTwo.startsWith("") ? "true" : "false");
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.print(F("stringTwo.endsWith(\"\"):"));  Serial.println(stringTwo.endsWith("") ? "true" : "false");
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringOne.debug();
  Serial.print(F("stringOne.endsWith(\"\"):"));  Serial.println(stringOne.endsWith("") ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringOne.debug();
  stringTwo.debug();
  Serial.print(F("stringOne.endsWith(stringTwo):"));  Serial.println(stringOne.endsWith(stringTwo) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char * nullStr = NULL;
  Serial.println(F("char * nullStr = NULL;"));
  Serial.println(F("stringOne.startsWith(nullStr);"));
  stringOne.startsWith(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.endsWith(nullStr);"));
  stringOne.endsWith(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringOne.startsWith(\"53\",13);"));
  stringOne.startsWith("53", 13);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  string = "";
  string.debug(F("string.clear(); => "));
  stringOne.debug();
  Serial.print(F("stringOne.startsWith(string):"));  Serial.println(stringOne.startsWith(string) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.print(F("stringOne.startsWith(string,12):"));  Serial.println(stringOne.startsWith(string, 12) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.print(F("stringOne.startsWith(string,-1):"));  Serial.println(stringOne.startsWith(string, -1) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.print(F("stringOne.endsWith(string):"));  Serial.println(stringOne.endsWith(string) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("char * nullStr = NULL;"));
  Serial.println(F("stringOne.endsWithCharFrom(nullStr);"));
  stringOne.endsWithCharFrom(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  string = "";
  string.debug(F("string.clear(); => "));
  Serial.print(F("stringOne.endsWithCharFrom(string):"));  Serial.println(stringOne.endsWithCharFrom(string) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  stringOne.clear();
  stringOne.debug(F("stringOne.clear(); => "));
  Serial.print(F("stringOne.endsWithCharFrom(string):"));  Serial.println(stringOne.endsWithCharFrom(string) ? "true" : "false");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {
}
