
/*
  SafeString from char* constructor
  Examples of how to create SafeStrings from an existing pointer to a char[]
  also see the SafeString_ConstructorAndDebugging, SafeStringFromCharArray and SafeStringFromCharPtrWithSize examples

  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"

char charArray[20] = "initial value"; // existing char array with some chars
char* arrayPtr = charArray; // a pointer to the charArray

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString from a char*"));
  Serial.println(F("// Set Stream to send SafeString error messages and debug( ) output to"));
  Serial.println(F("SafeString::setOutput(Serial); // defaults to verbose error messages"));
  SafeString::setOutput(Serial); // enable error messages and debug() output

  Serial.println();
  Serial.println(F("This sketch has an existing char charArray[20]= \"initial value\";  and a pointer (char*) to it, arrayPtr."));
  Serial.println(F(" You can use the createSafeStringFromCharPtr(  ); macro to create a SafeString to access and update this char[] safely via its pointer"));
  Serial.println(F("   You can also use the typing shortcut name cSFP( )"));
  Serial.println(F(" This macro wraps the char[] pointed to in a SafeString object, stringOne, and sets its name for debugging"));
  Serial.println(F(" The initial strlen( ) sets the valid size for the SafeString object and can not be increased."));
  Serial.println(F(" Also see the SafeString_ConstructorAndDebugging, SafeStringFromCharArray and SafeStringFromCharPtrWithSize examples"));
  Serial.println();
  Serial.println(F(" createSafeStringFromCharPtr(stringOne, arrayPtr); // or cSFP(stringOne, arrayPtr); "));
  createSafeStringFromCharPtr(stringOne, arrayPtr);
  Serial.println();
  Serial.println(F("Once the SafeString has been created you can use all the SafeString methods to safely manipulate it and update the underlying charArray"));
  Serial.println(F("You can remove chars, take substrings etc, but you cannot add chars past the initial length of the \"string\" when the SafeString was created."));
  Serial.println(F("e.g. stringOne += \"extra chars\";"));
  stringOne += "extra chars";
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F(" Examples of safely modifying the charArray via SafeString"));
  Serial.println(F("e.g. stringOne = 55;"));
  Serial.println(F("     stringOne += \" test chars\";"));
  stringOne = 55;
  stringOne += " test chars";
  stringOne.debug();
  Serial.print(F(" stringOne.endsWith(\"123\") => "));
  Serial.println(stringOne.endsWith("123") ? "true" : "false");
  Serial.println(F("Print out the underlying char[] using Serial.println(arrayPtr)"));
  Serial.println(arrayPtr);
  Serial.println();
  Serial.println(F("Now perform an unsafe operation on the arrayPtr, e.g.  strcat(arrayPtr,\"123\");"));
  strcat(arrayPtr, "123");
  Serial.println("Print out the charArray, Serial.println(arrayPtr)");
  Serial.println(arrayPtr);
  Serial.println(F(" Provided the unsafe operation has not killed your program, "));
  Serial.println(F("   the next call to any SafeString method cleans up the arrayPtr and resyncs the SafeString, making it safe again"));
  Serial.println(F(" NOTE Carefully Each call to a SafeString method reterminates the char array at the initial length of the \"string\" when the SafeString was created."));
  Serial.println(F("  SafeString prevents any access past the length that was initially passed in."));
  Serial.println(F("stringOne.endsWith(\"123\")"));
  stringOne.endsWith("123");
  stringOne.debug("stringOne.debug() => ");
  Serial.print(F("stringOne.hasError():"));  Serial.println(stringOne.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();


  Serial.println(F(" Unit tests for createSafeStringFromCharPtr."));
  Serial.println(F("Check passing NULL char* to createSafeStringFromCharPtr, prints error msg but does not blow up program."));
  Serial.println(F("char *nullArrayPtr = NULL;"));
  char *nullArrayPtr = NULL;
  Serial.println(F("cSFP(testStr1,nullArrayPtr); // using the typing shortcut name"));
  cSFP(testStr1, nullArrayPtr); // using the typing shortcut name
  Serial.print(F("testStr1.hasError():"));  Serial.println(testStr1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr1 is valid, but with zero capacity."));
  testStr1.debug(F("testStr1.debug(); => "));
  Serial.println();

  Serial.println(F("Check passing an empty char* to createSafeStringFromCharPtr."));
  Serial.println(F("charArray[0] = '\\0'; // charArray is now an empty (zero length) c-string"));
  Serial.println(F("cSFP(testStr2,charArray); // using the typing shortcut name"));
  cSFP(testStr2, charArray); // using the typing shortcut name
  Serial.print(F("testStr2.hasError():"));  Serial.println(testStr2.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr2 is valid, but with zero capacity."));
  testStr1.debug(F("testStr2.debug(); => "));
  Serial.println();

  Serial.println(F(" The Robustness of createSafeStringFromCharPtr depends on passing a pointer to a valid correctly terminated char[] "));
  Serial.println(F(" If the intial charArray is not termiated correctly createSafeStringFromCharPtr cannot correct that error."));
  Serial.println(F(" The following struct has an unterminated char[], buffer_1."));
  struct {
    char buffer_0[8] = "abcdefg";
    char buffer_1[8] = {'0', '1', '2', '3', '4', '5', '6', '7'}; // no terminating null for this char buffer
    char buffer_2[8] = "hijklmn";
  } buffers;
  Serial.println(F("struct {"));
  Serial.println(F("  char buffer_0[8] = \"abcdefg\";  "));
  Serial.println(F("  char buffer_1[8] = {'0','1','2','3','4','5','6','7'};  // no terminating null for this char[]"));
  Serial.println(F("  char buffer_2[8] = \"hijklmn\";  "));
  Serial.println(F("} buffers;"));
  Serial.println();

  Serial.println(F(" buffer.buffer_1 is a char[] with no terminating null. When you print it, the output continues on to the next buffer's chars."));
  Serial.println(F("Serial.println(buffers.buffer_1);"));
  Serial.println(buffers.buffer_1);
  Serial.println();
  Serial.println(F("If you use createSafeStringFromCharPtr (cSFP) on buffers.buffer_1 to create a SafeString its capacity will include buffer_2."));
  Serial.println(F("cSFP(sfBuffer_1,buffers.buffer_1);"));
  cSFP(sfBuffer_1, buffers.buffer_1);
  sfBuffer_1.debug();
  Serial.print(F("sfBuffer_1.hasError():"));  Serial.println(sfBuffer_1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("You can avoid this error by using either createSafeStringFromCharArray(sfBuffer_1,buffers.buffer_1), cSFA(sfBuffer_1,buffers.buffer_1) "));
  Serial.println(F(" or createSafeStringFromCharPtrWithSize(sfBuffer_1,buffers.buffer_1,8), cSFPS(sfBuffer_1,buffers.buffer_1,8)"));
  Serial.println(F(" Note: the 8 allows for the terminating '\\0' the char buffer_1[8] can only hold a 7 char c-string"));

  Serial.println();
  Serial.println(F("Check wrapping small char[] as char*"));
  char ch1[1] = "";
  Serial.println(F("char ch1[1]=\"\";"));
  Serial.println(F(" createSafeStringFromCharPtr(sfStr1, ch1); // or cSFP(sfStr1, ch1); "));
  createSafeStringFromCharPtr(sfStr1, ch1);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtr(sfStr1, ch1);"));
  } else {
    sfStr1.debug(F("No errors"));
  }
  Serial.println();

  char ch2[2] = "a";
  Serial.println(F("char ch2[2]=\"a\";"));
  Serial.println(F(" createSafeStringFromCharPtr(sfStr2, ch2); // or cSFP(sfStr2, ch2); "));
  createSafeStringFromCharPtr(sfStr2, ch2);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtr(sfStr2, ch2);"));
  } else {
    sfStr2.debug(F("No errors"));
  }
  Serial.println();

  char ch3[3] = "a";
  Serial.println(F("char ch3[3]=\"a\";"));
  Serial.println(F(" createSafeStringFromCharPtr(sfStr3, ch3); // or cSFP(sfStr3, ch3); "));
  createSafeStringFromCharPtr(sfStr3, ch3);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtr(sfStr3, ch3);"));
  } else {
    sfStr3.debug(F("No errors"));
  }
  Serial.println();

  char ch4[4] = "ab";
  Serial.println(F("char ch4[4]=\"ab\";"));
  Serial.println(F(" createSafeStringFromCharPtr(sfStr4, ch4); // or cSFP(sfStr4, ch4); "));
  createSafeStringFromCharPtr(sfStr4, ch4);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtr(sfStr4, ch4);"));
  } else {
    sfStr4.debug(F("No errors"));
  }

}

void loop() {

}
