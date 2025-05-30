
/*
  SafeString from char* with specified size constructor
  Examples of how to create SafeStrings from an existing pointer to a char[]
  also see the SafeString_ConstructorAndDebugging, SafeStringFromCharArray and SafeStringFromCharPtr examples

  by Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  Modified from String Examples by Tom Igoe
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"

const size_t charArraySize = 25;
char charArray[charArraySize] = "initial value"; // existing char array with some chars
char* arrayPtr = charArray; // a pointer to the charArray or size charArraySize

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeString from a char* with size"));
  Serial.println(F("// Set Stream to send SafeString error messages and debug( ) output to"));
  Serial.println(F("SafeString::setOutput(Serial); // defaults to verbose error messages"));
  SafeString::setOutput(Serial); // enable error messages and debug() output

  Serial.println();
  Serial.println(F("This sketch has an existing char charArray[charArraySize]= \"initial value\";  and a pointer (char*) to it, arrayPtr."));
  Serial.println(F("  charArraySize == 25 so the charArray can hold at most 24 chars + the terminating '\\0'"));
  Serial.println(F(" You can use the createSafeStringFromCharPtrWithSize(  ); macro to create a SafeString to access and update this char[] safely via its pointer"));
  Serial.println(F("   You can also use the typing shortcut name cSFPS( )"));
  Serial.println(F(" This macro wraps the char[] pointed to in a SafeString object, stringOne, and sets its name for debugging"));
  Serial.println(F(" The size of the char[] pointed to by this pointer is passed in and sets the capacity of the SafeString to one less to allow for the terminating '\\0'."));
  Serial.println(F(" Also see the SafeString_ConstructorAndDebugging, SafeStringFromCharArray and SafeStringFromCharPtr examples"));
  Serial.println();
  Serial.println(F(" createSafeStringFromCharPtrWithSize(stringOne, arrayPtr, charArraySize); // or cSFPS(stringOne, arrayPtr, charArraySize); "));
  Serial.println(F(" NOTE: The size passed in the 3rd argument is the actual size of the array, the number of chars it can hold will be one less.  "));
  createSafeStringFromCharPtrWithSize(stringOne, arrayPtr, charArraySize); // or cSFPS(stringOne, arrayPtr, charArraySize); "));
  stringOne.debug("stringOne.debug() => ");
  Serial.println();
  Serial.println(F("Once the SafeString has been created you can use all the SafeString methods to safely manipulate it and update the underlying char[]"));
  Serial.println(F("e.g. stringOne += \" extras\";"));
  stringOne += " extras";
  Serial.println(F(" Printing the char* shows it updated"));
  Serial.println(F(" e.g. Serial.println(arrayPtr);"));
  Serial.println(arrayPtr);
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
  Serial.println(F("   Each call to a SafeString method reterminates the char array at the size passed in when the SafeString was created."));
  Serial.println(F("  SafeString prevents any access past the length that was initially passed in."));
  Serial.print(F("stringOne.endsWith(\"123\")  "));
  Serial.println(stringOne.endsWith("123") ? "true" : "false");
  stringOne.debug("stringOne.debug() => ");
  Serial.println();


  Serial.println(F(" Unit tests for createSafeStringFromCharPtrWithSize."));
  Serial.println(F("Check passing NULL char* to createSafeStringFromCharPtrWithSize, prints error msg but does not blow up program."));
  Serial.println(F("char *nullArrayPtr = NULL;"));
  char *nullArrayPtr = NULL;
  Serial.println(F("cSFPS(testStr1,nullArrayPtr,charArraySize); // using the typing shortcut name"));
  cSFPS(testStr1, nullArrayPtr, charArraySize); // using the typing shortcut name
  Serial.print(F("testStr1.hasError():"));  Serial.println(testStr1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr1 is valid, but with zero capacity."));
  testStr1.debug(F("testStr1.debug(); => "));
  Serial.println();

  Serial.println(F("Check passing size 0 to createSafeStringFromCharPtrWithSize, prints error msg but does not blow up program."));
  Serial.println(F("cSFPS(testStr2,arrayPtr,0); // using the typing shortcut name"));
  cSFPS(testStr2, arrayPtr, 0); // using the typing shortcut name
  Serial.print(F("testStr2.hasError():"));  Serial.println(testStr2.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println();
  Serial.println(F("The resulting testStr1 is valid, but with zero capacity."));
  testStr1.debug(F("testStr1.debug(); => "));
  Serial.println();

  Serial.println(F(" The Robustness of createSafeStringFromCharPtrWithSize depends on passing the correct size of the char[] pointed to by the char* "));
  Serial.println(F(" If the size passed in is not correct (i.e. larger than the actual char[]),  createSafeStringFromCharPtrWithSize cannot correct that error."));
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
  Serial.println(F("You can use createSafeStringFromCharPtrWithSize (cSFPS) on buffers.buffer_1 to create a SafeString with the correct size for this buffer."));
  Serial.println(F("cSFPS(sfBuffer_1,buffers.buffer_1, 8);"));
  Serial.println(F(" Note: the 8 to allows for the terminating '\\0' the char buffer_1[8] can only hold a 7 char c-string"));
  cSFPS(sfBuffer_1, buffers.buffer_1, 8);
  Serial.print(F("sfBuffer_1.hasError():"));  Serial.println(sfBuffer_1.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  sfBuffer_1.debug();
  Serial.println();
  Serial.println(F("When working with char*, SafeString always terminates the array at its capacity(), in this case overwriting the 7 with a terminating '\\0'"));
  Serial.println();

  Serial.println(F("Check wrapping small char[] as char* with size"));
  char ch0[0];
  Serial.println(F("char ch0[0];"));
  Serial.println(F(" createSafeStringFromCharPtrWithSize(sfStr0, ch0, 0); // or cSFPS(sfStr0, ch0, 0); "));
  createSafeStringFromCharPtrWithSize(sfStr0, ch0, 0);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtrWithSize(sfStr0, ch0, 0);"));
  } else {
    sfStr0.debug(F("No errors"));
  }
  Serial.println();

  char ch1[1] = "";
  Serial.println(F("char ch1[1]=\"\";"));
  Serial.println(F(" createSafeStringFromCharPtrWithSize(sfStr1, ch1, 1); // or cSFPS(sfStr1, ch1, 1); "));
  createSafeStringFromCharPtrWithSize(sfStr1, ch1, 1);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtrWithSize(sfStr1, ch1, 1);"));
  } else {
    sfStr1.debug(F("No errors"));
  }
  Serial.println();

  char ch2[2] = "";
  Serial.println(F("char ch2[2]=\"\";"));
  Serial.println(F(" createSafeStringFromCharPtrWithSize(sfStr2, ch2,2); // or cSFPS(sfStr2, ch2, 2); "));
  createSafeStringFromCharPtrWithSize(sfStr2, ch2, 2);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtrWithSize(sfStr2, ch2, 2);"));
  } else {
    sfStr2.debug(F("No errors"));
  }
  Serial.println();

  char ch3[3] = "a";
  strcat(ch3,"12");
  Serial.println(F("char ch3[3] = \"a\"; strcat(ch3,\"12\");"));
  Serial.println(F(" createSafeStringFromCharPtrWithSize(sfStr3, ch3, 3); // or cSFPS(sfStr3, ch3, 3); "));
  createSafeStringFromCharPtrWithSize(sfStr3, ch3, 3);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtrWithSize(sfStr3, ch3, 3);"));
  } else {
    sfStr3.debug(F("No errors"));
  }
  Serial.println();

  char ch4[4] = "ab";
  Serial.println(F("char ch4[4]=\"ab\";"));
  Serial.println(F(" createSafeStringFromCharPtrWithSize(sfStr4, ch4, 4); // or cSFPS(sfStr4, ch4, 4); "));
  createSafeStringFromCharPtrWithSize(sfStr4, ch4, 4);
  if (SafeString::errorDetected()) {
    Serial.println(F(" Error in createSafeStringFromCharPtrWithSize(sfStr4, ch4, 4);"));
  } else {
    sfStr4.debug(F("No errors"));
  }

}

void loop() {

}
