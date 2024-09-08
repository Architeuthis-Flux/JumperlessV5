/*
  sfStream.ino

  SafeStringStream Unit test
  Example of using SafeStringStream

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html
*/

// first install SafeString from the Arduino library manager.
#include "SafeString.h"
#include "SafeStringStream.h"

char contents[] = {
  "First line 08999340; \n"
  "Second line 18999341; \n"
  "Third line 28999342; \n"
  "Fouth line 28999343; \n"
};

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();

  Serial.println(F("SafeStringStream"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose == true"));
  SafeString::setOutput(Serial); // enable verbose debugging error msgs
  Serial.println();

  Serial.println(F("Create a SafeString of the stream input "));
  cSF(sfData, strlen(contents)); // create a SafeString large enough
  sfData = contents; //and copy the contents to it
  sfData.debug(F("cSF(sfData, strlen(contents));  sfData = contents;  => "));

  Serial.println(F(" Wrap it in a SafeStringStream object"));
  Serial.println(F("SafeStringStream sfStream(sfData); // default RX buffer is only 8bytes, you can add a larger buffer here"));
  SafeStringStream sfStream(sfData);
  Serial.println();
  Serial.println(F(" call begin( ) to  set the baud rate"));
  Serial.println(F("sfStream.begin(); // default infinite baud rate so all the data is available instantly"));
  sfStream.begin();
  Serial.println();
  Serial.println(F(" Now you can use any sfStream anywhere that expects a Stream object."));

  Serial.println(F(" e.g. while(sfStream.available()) { Serial.print((char)sfStream.read()); }"));
  while (sfStream.available()) {  // note this only works at infinite baud rate, at begin(9600) only the 8byte RX buffer will be available() instantly
    Serial.print((char)sfStream.read());
  }
  Serial.println();
  Serial.println(F("Note: reading the sfData consumes chars from the underlying SafeString"));
  sfData.debug(F(" After read() "));

  Serial.println(F("  Which leave space available to write chars to the sfStream, which add the chars to the underlying sfData SafeString"));
  Serial.println(F("sfStream.print(\"Write some data to the sfStream \"); sfStream.print(500.5); sfStream.println();"));
  sfStream.print("Write some data to the sfStream "); sfStream.print(500.5); sfStream.println();
  sfData.debug(F(" After sfStream.prints "));

  Serial.println();
  Serial.println(F("  If sfData fills up, the sfStream.write( ) does not block, it just discards the data"));
  Serial.println(F("  and an error msg is printed if SafeString::setOutput(Serial); has been called."));
  Serial.println(F(" sfStream.print(contents);"));
  sfStream.print(contents);

}

void loop() {
}
