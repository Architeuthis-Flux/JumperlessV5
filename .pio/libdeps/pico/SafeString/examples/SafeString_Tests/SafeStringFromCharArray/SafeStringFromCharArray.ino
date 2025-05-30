
/*
  SafeString from char[] constructor
  Examples of how to create SafeStrings from an existing char[]
  also see the SafeString_ConstructorAndDebugging, SafeStringFromCharPtr and SafeStringFromCharPtrWithSize examples


  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"

char charArray[15]; // existing char array

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString from a char[]"));
  Serial.println(F("// Set Stream to send SafeString error messages and debug( ) output to"));
  Serial.println(F("SafeString::setOutput(Serial); // defaults to verbose error messages"));
  SafeString::setOutput(Serial); // enable error messages and debug() output

  Serial.println();
  Serial.println(F("This sketch has an existing char charArray[15] defined. "));
  Serial.println(F(" You can use the createSafeStringFromCharArray(  ); macro to create a SafeString to access and update this char[] safely"));
  Serial.println(F("   You can also use the typing shortcut name cSFA( )"));
  Serial.println(F(" This macro wraps the char[] in a SafeString object, stringOne, and sets its name for debugging"));
  Serial.println(F(" Also see the SafeString_ConstructorAndDebugging, SafeStringFromCharPtr and SafeStringFromCharPtrWithSize examples"));
  Serial.println();
  Serial.println(F(" createSafeStringFromCharArray(stringOne, charArray); // or cSFA(stringOne, charArray); "));
  createSafeStringFromCharArray(stringOne, charArray);
  Serial.println();
  Serial.println(F("Once the SafeString has been created you can use all the SafeString methods to safely manipulate it and update the underlying charArray"));
  Serial.println(F("e.g. stringOne = 55;"));
  Serial.println(F("     stringOne += \" test\";"));
  stringOne = 55;
  stringOne += " test";
  stringOne.debug();
  Serial.print(F(" stringOne.endsWith(\"123\") => "));
  Serial.println(stringOne.endsWith("123") ? "true" : "false");
  Serial.println(F("Print out the underlying char[] using Serial.println(charArray)"));
  Serial.println(charArray);
  Serial.println();
  Serial.println(F("Now perform an unsafe operation on the charArray, e.g.  strcat(charArray,\"123\");"));
  strcat(charArray, "123");
  Serial.println("Print out the charArray, Serial.println(charArray)");
  Serial.println(charArray);
  Serial.println(F(" Provided the unsafe operation has not killed your program, "));
  Serial.println(F("   the next call to any SafeString method cleans up the charArray and resyncs the SafeString, making it safe again. e.g."));
  Serial.print(F("stringOne.endsWith(\"123\") => "));
  Serial.println(stringOne.endsWith("123") ? "true" : "false");
  stringOne.debug("stringOne.debug() => ");
  Serial.println();
  Serial.println();


  Serial.println(F(" Unit tests for createSafeStringFromCharArray."));
  Serial.println(F(" createSafeStringFromCharArray needs to be created from an actual char[] not a char* "));
  Serial.println(F("   because SafeString needs to use sizeof( ) to determine the size of the charArray."));
  Serial.println(F("       (to wrap a char* use createSafeStringFromCharPtr (cSFP) instead) "));
  char *charArrayPtr = charArray;
  Serial.println(F("Check passing char* (NULL or otherwise) as the char[] to createSafeStringFromCharArray, prints error msg but does not blow up program."));
  Serial.println(F("  Note: the sizeof(char*) varies between different Arduino boards Uno/Megs2560 sizeof(char*) == 2, more powerful boards like ESP8266/ESP32,etc sizeof(char*) == 4"));
  Serial.println(F("  So for small char[] use 5 as the mininum size, ie. char cArray[5];"));
  Serial.println();
  
  Serial.println(F("char *charArrayPtr = charArray;"));
  Serial.println(F("cSFA(testStr1,charArrayPtr); // using the typing shortcut name"));
  cSFA(testStr1, charArrayPtr);
  Serial.print(F("testStr1.hasError():"));  Serial.println(testStr1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr1 is valid, but with zero capacity."));
  testStr1.debug(F("testStr1.debug(); => "));
  Serial.println();

  Serial.println(F("Check passing an empty char[], i.e. char emptyArray[0], to createSafeStringFromCharArray, prints error msg but does not blow up program."));
  Serial.println(F("char emptyArray[0];"));
  char emptyArray[0];
  Serial.println(F("cSFA(testStr2,emptyArray); // using the typing shortcut name"));
  cSFA(testStr2, emptyArray);
  Serial.print(F("testStr2.hasError():"));  Serial.println(testStr2.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr1 is valid, but with zero capacity."));
  testStr1.debug(F("testStr2.debug(); => "));
  Serial.println();


  Serial.println(F(" createSafeStringFromCharArray (cSFA) also handles invalid, unterminated char[]s and truncates them to the available size."));
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
  Serial.println(F("You can safely use createSafeStringFromCharArray (cSFA) on buffer_1 to create as valid SafeString."));
  Serial.println(F("cSFA(sfBuffer_1,buffers.buffer_1);"));
  cSFA(sfBuffer_1, buffers.buffer_1);
  Serial.print(F("sfBuffer_1.hasError():"));  Serial.println(sfBuffer_1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();

  Serial.println(F("You can now safely access and update buffers.buffer_1 via the SafeString, sfBuffer_1."));
  Serial.println();

  Serial.println(F("Check wrapping small char[] "));
  char ch0[0];
  Serial.println(F("char ch0[0];"));
  Serial.println(F(" createSafeStringFromCharArray(sfStr0, ch0); // or cSFA(sfStr0, ch0); "));
  createSafeStringFromCharArray(sfStr0, ch0);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharArray(sfStr0, ch0);"));
  } else {
    sfStr0.debug(F("No errors"));
  }
  Serial.println();

  char ch1[1] = "";
  Serial.println(F("char ch1[1]=\"\";"));
  Serial.println(F(" createSafeStringFromCharArray(sfStr1, ch1); // or cSFA(sfStr1, ch1); "));
  createSafeStringFromCharArray(sfStr1, ch1);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharArray(sfStr1, ch1);"));
  } else {
    sfStr1.debug(F("No errors"));
  }
  Serial.println();

  char ch2[2] = "";
  Serial.println(F("char ch2[2]=\"\";"));
  Serial.println(F(" createSafeStringFromCharArray(sfStr2, ch2); // or cSFA(sfStr2, ch2); "));
  createSafeStringFromCharArray(sfStr2, ch2);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharArray(sfStr2, ch2);"));
  } else {
    sfStr2.debug(F("No errors"));
  }
  Serial.println();

  char ch3[3] = "a";
  strcat(ch3,"12");
  Serial.println(F("char ch3[3]=\"a\"; strcat(ch3,\"12\");"));
  Serial.println(F(" createSafeStringFromCharArray(sfStr3, ch3); // or cSFA(sfStr3, ch3); "));
  createSafeStringFromCharArray(sfStr3, ch3);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharArray(sfStr3, ch3);"));
  } else {
    sfStr3.debug(F("No errors"));
  }
  Serial.println();

  char ch4[4] = "ab";
  Serial.println(F("char ch4[4]=\"ab\";"));
  Serial.println(F(" createSafeStringFromCharArray(sfStr4, ch4); // or cSFA(sfStr4, ch4); "));
  createSafeStringFromCharArray(sfStr4, ch4);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharArray(sfStr4, ch4);"));
  } else {
    sfStr4.debug(F("No errors"));
  }

}

void loop() {

}
