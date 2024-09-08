/*
  SSring indexOf() and lastIndexOf()

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include "SafeString.h"

createSafeString(stringOne, 20, "<HTML><HEAD><BODY>");
createSafeString(stringTwo, 35, "<UL><LI>item<LI>item<LI>item</UL>");

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString indexOf() and lastIndexOf() functions"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs
  Serial.println();

  // indexOf() returns the position (i.e. index) of a particular character or string in a SafeString.
  // the return is of type int, -1 is returned if character is not found
  // OR if there is an error
  stringOne.debug();

  Serial.println();
  Serial.println(F(" A return position of >= 0 indicates the char / string was found."));
  Serial.println(F(" fromIndex == length() is a valid argument, so if returned position is  < length() then can always add one and check again"));
  Serial.println();
  Serial.println(F(" Find all the positions of '>' in stringOne"));
  int startIdx = 0;
  while ((startIdx = stringOne.indexOf('>', startIdx)) >= 0) { // found one
    Serial.print(F("The next index of > in the string '")); Serial.print(stringOne); Serial.print(F(" is ")); Serial.println(startIdx);
    Serial.println(F("Step over > and search again "));
    startIdx += 1;
  }
  Serial.print(F("No more > found indexOf returned ")); Serial.println(startIdx);
  Serial.println();

  Serial.println(F("int bodyTagIdx = stringOne.indexOf(\"<BODY>\");"));
  int bodyTagIdx = stringOne.indexOf("<BODY>");
  Serial.print(F("The index of '<BODY>' in the string '")); Serial.print(stringOne); Serial.print(F("' is ")); Serial.println(bodyTagIdx);
  Serial.println();

  startIdx = 12;
  int lastIdx = stringOne.lastIndexOf('<', startIdx);
  Serial.print(F("The last index of < in the string '")); Serial.print(stringOne); Serial.print(F("' from index:")); Serial.print(startIdx); Serial.print(F(" is ")); Serial.println(lastIdx);
  Serial.println();

  stringTwo.debug();
  Serial.println(F("int liTagIdx = stringTwo.lastIndexOf(\"<LI>\");"));
  int liTagIdx = stringTwo.lastIndexOf("<LI>");
  Serial.print(F("The last index of '<LI>' in the string '")); Serial.print(stringTwo); Serial.print(F("' is ")); Serial.println(liTagIdx);
  Serial.println();

  Serial.println(F("indexOfCharFrom(validChars,idx) finds the first occurance of any char in validChars searching starting from idx"));
  stringOne = "abc = 5.335";
  stringOne.debug(F("stringOne = \"abc = 5.335\"; => "));
  createSafeString(validDigits, 12, "0123456789.");
  validDigits.debug();
  int startOfNumberIdx = stringOne.indexOfCharFrom(validDigits);
  Serial.println(F("int startOfNumberIdx = stringOne.indexOfCharFrom(validDigits);"));
  Serial.print(F("The first index of the number in the string ")); Serial.print(stringOne); Serial.print(F(" is ")); Serial.println(startOfNumberIdx);
  Serial.println();

  Serial.println(F("int idx = stringOne.lastIndexOf('3',11); // fromIndex == length() is OK"));
  int idx = stringOne.lastIndexOf('3', 11);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned is : ")); Serial.print(idx);  Serial.println(F("  i.e. not found"));
  Serial.println();

  Serial.println(F("idx = stringOne.lastIndexOf(\"abc = 5.33533\"); "));
  idx = stringOne.lastIndexOf("abc = 5.33533");
  Serial.print(F("idx returned is : ")); Serial.print(idx); Serial.println(F("  i.e. not found"));
  Serial.println();

  Serial.println(F("idx = stringOne.lastIndexOf(\"5.3\"); "));
  idx = stringOne.lastIndexOf("5.3");
  Serial.print(F("idx returned is : ")); Serial.println(idx);
  Serial.println();

  Serial.println(F("Error checking.."));
  Serial.println();
  stringOne.debug();
  Serial.println();
  Serial.println(F("A start index of -1 is valid and just returns -1 without error"));
  Serial.println(F("idx = stringOne.lastIndexOf(\"5.3\",-1); "));
  idx = stringOne.lastIndexOf("5.3", -1);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned is : ")); Serial.println(idx);
  Serial.println();

  Serial.println(F("idx = stringOne.indexOf('\\0');"));
  idx = stringOne.indexOf('\0');
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("char *nullStr = NULL;"));
  Serial.println(F("idx = stringOne.indexOf(nullStr);"));
  char *nullStr = NULL;
  idx = stringOne.indexOf(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("char emptyStr[] = \"\";"));
  Serial.println(F("idx = stringOne.indexOf(emptyStr);"));
  char emptyStr[] = "";
  idx = stringOne.indexOf(emptyStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("idx = stringOne.lastIndexOf('\\0');"));
  idx = stringOne.lastIndexOf('\0');
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("idx = stringOne.lastIndexOf(nullStr);"));
  idx = stringOne.lastIndexOf(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("idx = stringOne.lastIndexOf(emptyStr);"));
  idx = stringOne.lastIndexOf(emptyStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("idx = stringOne.indexOfCharFrom(nullStr);"));
  idx = stringOne.indexOfCharFrom(nullStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

  Serial.println();
  Serial.println(F("idx = stringOne.indexOfCharFrom(emptyStr);"));
  idx = stringOne.indexOfCharFrom(emptyStr);
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.print(F("idx returned after Error : ")); Serial.println(idx);

}

void loop() {
}
