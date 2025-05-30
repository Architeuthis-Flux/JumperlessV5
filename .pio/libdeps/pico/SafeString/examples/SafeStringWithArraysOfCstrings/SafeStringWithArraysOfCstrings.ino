
/* SafeStringWithArraysOfCstrings.ino
    Example of using SafeString for working with char[][xx]

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include <SafeString.h>

#define MAX_STRING_SIZE 40
// this array of char[][] each has space for 39 chars + '\0'
char arr1[][MAX_STRING_SIZE] = {
  "array1 of c string",
  "is fun to use",
  "make sure to properly",
  "tell the array size"
};
// each element of this the array of const char* only has space for the initial string
// NOTE: since these strings are const, the compiler may only keep one copy of identical strings
const char* arr2[] = {
  "array2 of c string",
  "is fun to use",
  "make sure to properly",
  "is fun to use",
  "tell the array size"
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
  SafeString::setOutput(Serial); // enable error msgs

  Serial.println(F(" char arr2[][MAX_STRING_SIZE]"));
  // to modify the string safely wrap it in a SafeString
  cSFA(sf1arr0, arr1[0]); // OR in the long form   createSafeStringFromCharArray(sf1arr0,arr1[0]);
  // the capacity is automatically picked up from the arr[][xx] definition
  Serial.print("sf1arr0 capacity:"); Serial.println(sf1arr0.capacity());
  Serial.println(F("sf1arr0 += \" add a bit more\";"));
  sf1arr0 += " add a bit more";
  Serial.println(F(" Print the underlying array arr1[0]"));
  Serial.println(arr1[0]);
  Serial.println(F("sf1arr0 += \" try to add alot more \";"));
  sf1arr0 += " try to add alot more ";
  Serial.print(F("sf1arr0.hasError():"));  Serial.println(sf1arr0.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");

  Serial.println();
  createSafeStringFromCharArray(sf1arr1, arr1[1]);
  Serial.println(F("sf1arr1.removeFrom(2);")); // just keep the first 2 chars idx 0,1
  sf1arr1.removeFrom(2); // just keep the first 2 chars idx 0,1
  Serial.print(F("arr1[1] = ")); Serial.println(arr1[1]);

  Serial.println();
  Serial.println(F(" Now const char *arr2[]"));

  // to modify the string safely wrap it in a SafeString
  cSFP(sf2arr0, (char*)arr2[0]); // OR in the long form   createSafeStringFromCharArray(sf2arr0,arr2[0]);
  // the capacity is automatically picked up from the arr[][xx] definition
  Serial.print(F("sf2arr0 capacity:")); Serial.println(sf2arr0.capacity());
  Serial.println(F("sf2arr0 += \" add a bit more\";"));
  sf2arr0 += " add a bit more";
  Serial.print(F("sf2arr0.hasError():"));  Serial.println(sf2arr0.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");
  Serial.println(" Print the underlying array arr2[0]");
  Serial.println(arr2[0]);
  Serial.println(F("sf2arr0 += \" try to add alot more \";"));
  sf2arr0 += " try to add alot more ";
  Serial.print(F("sf2arr0.hasError():"));  Serial.println(sf2arr0.hasError() ? "true" : "false");
  Serial.print(F("SafeString::errorDetected():"));  Serial.println(SafeString::errorDetected() ? "true" : "false");

  Serial.println();
  Serial.println(F("Before remove()") );
  Serial.print(F("arr2[1] = ")); Serial.println(arr2[1]);
  Serial.print(F("arr2[3] = ")); Serial.println(arr2[3]);
  createSafeStringFromCharPtr(sf2arr1, (char*)arr2[1]);
  Serial.println(F("sf2arr1.removeFrom(2);")); // just keep the first 2 chars idx 0,1
  sf2arr1.removeFrom(2); // just keep the first 2 chars idx 0,1
  Serial.println(F("After remove() both elements updated "));
  Serial.println(F(" because compiler only allocated on string and reused the const char* for elements 1 and 3") );
  Serial.print(F("arr2[1] = ")); Serial.println(arr2[1]);
  Serial.print(F("arr2[3] = ")); Serial.println(arr2[3]);

}

void loop() {
}
