/*
  SafeStringReader_Assign.ino

  This example = to SafeStringReader

  by Matthew Ford
  Copyright(c)2021 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

#include "SafeStringReader.h"

createSafeStringReader(sfReader, 10, '\n');

void showError(const char *err) {
  while (1) {
    Serial.print(F("Error: ")); Serial.println(err);
    delay(3000);
  }
}

void setup() {
  Serial.begin(9600);    // Open serial communications and wait a few seconds
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  // SafeString::setOutput(Serial);

  sfReader = "abc";
  if (sfReader != "abc") {
    showError("= abc");
  }
  sfReader = F("fabc");
  if (sfReader != "fabc") {
    showError("= F(fabc)");
  }
  cSF(sfStr, 5, "sfstr");
  sfReader = sfStr;
  if (sfReader != "sfstr") {
    showError("= sfstr");
  }
  sfReader = 'c';
  if (sfReader != "c") {
    showError("= c");
  }
  sfReader = (unsigned char)5;
  if (sfReader != "5") {
    showError("= 5");
  }
  sfReader = (int) - 6;
  if (sfReader != "-6") {
    showError("= -6");
  }
  sfReader = (unsigned int)7;
  if (sfReader != "7") {
    showError("= 7");
  }
  sfReader = (long)8;
  if (sfReader != "8") {
    showError("= 8");
  }
  sfReader = (unsigned long)9;
  if (sfReader != "9") {
    showError("= 9");
  }
  sfReader = (float)10.23;
  if (sfReader != "10.23") {
    showError("= 10.23");
  }
  sfReader = (double)12.23;
  if (sfReader != "12.23") {
    showError("= 12.23");
  }
  sfReader = "";
  sfReader += "concatabc";
  if (sfReader != "concatabc") {
    showError("+= concatabc");
  }
  sfReader -= "P";
  if (sfReader != "Pconcatabc") {
    showError("-= Pconcatabc");
  }
  sfReader = '\0'; // use = "" instead
  // sfReader = '\0' fails but sfReader still cleared because = .. always clears
  if (!sfReader.hasError()) {
    showError(" = 0");
  }
  sfReader += '\0'; // use += "" instead
  if (!sfReader.hasError()) {
    showError(" += 0");
  }
  if (sfReader.hasError()) {
    Serial.println(" Some Tests failed");
  } else {
    Serial.println(" Tests completed sucessfully");
  }
}


void loop() {
}
