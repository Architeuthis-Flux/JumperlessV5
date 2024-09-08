/*
  SafeStringReader_flushInput.ino

  This example flushes any initial input and also starts flushing if "flush" is found in the text stream

  by Matthew Ford
  Copyright(c)2021 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

// SafeStringReader_flushInput.ino
// Reads words into a SafeString delimited by space or timeout
// uses line limit, timeout and input flushing
// https://www.forward.com.au/pfod/ArduinoProgramming/SoftwareSolutions/index.html
//
// download SafeString library from Arduino library manager
// or from the tutorial page
// https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
//
// Pros: Minimal Code. Non-Blocking, Robust flushes any initial partial input, if flushInput() called in setup() or loop().
//       Skips un-expected long input lines (missing terminator).
//       Returns un-terminated input, if a timeout is set. Option echoOn() setting to echo all input.
// Cons: Nothing really, except needs SafeString library to be installed.

#include "SafeStringReader.h"

createSafeStringReader(sfReader, 20, " "); // a reader for upto 20 chars to read tokens terminated by space or timeout

void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println(F("SafeStringReader_flushInput.ino"));
  Serial.println(F(" Enter words separated by spaces, any initial buffered RX input (entered while counting down) is flushed"));
  Serial.println(F("   Entering the word 'flush' clears the Serial TX buffer and skips to the next space or end of input timeout"));
  SafeString::setOutput(Serial);
  sfReader.setTimeout(1000); // set 1 sec timeout
  sfReader.flushInput(); // empty Serial RX buffer and then skip until either find delimiter or timeout
  sfReader.connect(Serial); // read from Serial
}

void loop() {
  if (sfReader.read()) { // got a line or timed out  delimiter is NOT returned
    if (sfReader.hasError()) { // input length exceeded
      Serial.println(F(" sfReader hasError. Read '\\0' or input overflowed."));
    }
    if (sfReader.getDelimiter() == -1) { // no delimiter so timed out
      Serial.println(F(" Input timed out without space"));
    }
    Serial.print(F(" got a line of input '")); Serial.print(sfReader); Serial.println("'");
    if (sfReader == "flush") {
      sfReader.flushInput();
    }
    // no need to clear sfReader as read() does that
  }
}
