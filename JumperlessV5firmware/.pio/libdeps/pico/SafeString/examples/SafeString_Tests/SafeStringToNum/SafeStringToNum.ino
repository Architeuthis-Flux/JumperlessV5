/*
  SafeString to Number conversion
  Examples of SafeString to Number conversions and comparing these to the results from String methods

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeString.h"

String a_str = "";    // an Arduino Wstring.cpp String object
createSafeString(str, 20); // a SafeString object

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();

  Serial.println(F("SafeString to Number conversion"));
  Serial.println(F(" compared to the results from Arduino String methods (in WString.cpp). (Also see SafeString_stoken example)"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs
  Serial.println();

  long l_num; // to hold result
  float f; // to hold result

  str = " -35  ";
  str.debug(F("str = \" -35  \"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F("not a valid integer."));
  }
  a_str = " -35  ";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"-35\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "5a";
  str.debug(F("str = \"5a\"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid integer. l_num unchanged."));
  }
  a_str = "5a";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"5a\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "5 a";
  str.debug(F("str = \"5 a\"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid integer. l_num unchanged."));
  }
  a_str = "5 a";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"5 a\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "123456789012345";
  str.debug(F("str = \"123456789012345\"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid integer. l_num unchanged."));
  }
  a_str = "123456789012345";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"123456789012345\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "-123456789012345";
  str.debug(F("str = \"-123456789012345\"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid integer. l_num unchanged."));
  }
  a_str = "-123456789012345";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"-123456789012345\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "5.";
  str.debug(F("str = \"5.\"; => "));
  Serial.print(F("SafeString version.  str.toLong(l_num); "));
  if (str.toLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid integer. l_num unchanged."));
  }
  a_str = "5.";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"5.\"; a_str.toInt() => "));
  Serial.println(a_str.toInt());
  Serial.println();

  str = "5.";
  str.debug(F("str = \"5.\"; => "));
  Serial.print(F("SafeString version.  str.toFloat(f); "));
  if (str.toFloat(f)) {
    Serial.print(F("returns true, f set to "));
    Serial.println(f);
  } else {
    Serial.println(F(" returns false, not a valid float. f unchanged."));
  }
  a_str = "5.";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"5.\"; a_str.toFloat() => "));
  Serial.println(a_str.toFloat());
  Serial.println();

  str = "5.3.6";
  str.debug(F("str = \"5.3.6\"; => "));
  Serial.print(F("SafeString version.  str.toFloat(f); "));
  if (str.toFloat(f)) {
    Serial.print(F("returns true, f set to "));
    Serial.println(f);
  } else {
    Serial.println(F(" returns false, not a valid float. f unchanged."));
  }
  a_str = "5.3.6";
  Serial.print(F(" Arduino WString.cpp version. a_str = \"5.3.6\"; a_str.toFloat() => "));
  Serial.println(a_str.toFloat());
  Serial.println();

  Serial.println(F(" binToLong(), octToLong() and hexToLong() versions"));

  str = "0101";
  str.debug(F("str = \"0101\"; => "));
  Serial.print(F("SafeString str.binToLong(l_num); "));
  if (str.binToLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid binary number. l_num unchanged."));
  }
  Serial.print(F("SafeString str.octToLong(l_num); "));
  if (str.octToLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid octal number. l_num unchanged."));
  }
  Serial.print(F("SafeString str.hexToLong(l_num); "));
  if (str.hexToLong(l_num)) {
    Serial.print(F("returns true, l_num set to "));
    Serial.println(l_num);
  } else {
    Serial.println(F(" returns false, not a valid hex number. l_num unchanged."));
  }

}

void loop() {
}
