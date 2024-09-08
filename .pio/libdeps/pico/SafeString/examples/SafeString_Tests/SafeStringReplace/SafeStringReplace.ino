/*
  SafeString replace()
  Examples of SafeString replace for chars and strings

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

  Serial.println(F("SafeString  replace() usage"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable verbose debugging error msgs
  Serial.println();

  stringOne.debug();
  Serial.println();
  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  stringTwo.replace('<', ' ');
  stringTwo.replace('>', ' ');
  Serial.print(F("stringTwo.replace('>', ' '); stringTwo.replace('<', ' '); => ")); stringTwo.debug();
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  stringTwo.replace("<body>", "");
  stringTwo.debug(F("stringTwo.replace(\"<body>\", \"\"); => "));
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));
  stringTwo.replace("<", "</");
  stringTwo.debug(F("stringTwo.replace(\"<\", \"</\"); => "));
  Serial.println();

  createSafeString(findStr, 5);   createSafeString(replaceStr, 5);
  Serial.println(F("createSafeString(findStr,5);   createSafeString(replaceStr,5);"));
  findStr = "</";
  findStr.debug(F("findStr = \"</\"; => "));
  replaceStr = "<";
  replaceStr.debug(F("replaceStr = \"<\"; => "));
  stringTwo.replace(findStr, replaceStr);
  stringTwo.debug(F("stringTwo.replace(findStr,replaceStr); => "));
  Serial.println();

  Serial.println(F("Error checking.."));
  Serial.println();

  Serial.println(F("stringTwo.replace(\"<\", \"<_____\");"));
  stringTwo.replace("<", "<_____");
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringTwo.replace(stringTwo,replaceStr);"));
  stringTwo.replace(stringTwo, replaceStr);
  stringTwo.debug();
  Serial.println();

  stringTwo = stringOne;
  stringTwo.debug(F("stringTwo = stringOne; => "));

  Serial.println(F("stringTwo.replace(find,stringTwo);"));
  stringTwo.replace(findStr, stringTwo);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  createSafeString(emptyString, 3);
  emptyString.debug();
  Serial.println(F("stringTwo.replace(findStr,emptyString);"));
  stringTwo.replace(findStr, emptyString);
  stringTwo.debug();
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringTwo.replace(emptyString,replaceStr);"));
  stringTwo.replace(emptyString, replaceStr);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  char *nullStr = NULL;
  Serial.println(F("char *nullStr = NULL;"));
  Serial.println(F("stringTwo.replace(nullStr,\"\");"));
  stringTwo.replace(nullStr, "");
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("stringTwo.replace(\"<\",nullStr);"));
  stringTwo.replace("<", nullStr);
  Serial.print(F("stringTwo.hasError():"));  Serial.println(stringTwo.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

}

void loop() {
}
