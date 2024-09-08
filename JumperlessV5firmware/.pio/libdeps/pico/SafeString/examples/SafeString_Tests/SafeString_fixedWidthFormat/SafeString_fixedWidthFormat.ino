/*
  Fixed 9 Formatting of double/float/long/ing

  by Matthew Ford
  Copyright(c)2021 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include "SafeString.h"

void setup() {
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  Serial.println();
  Serial.println(F("Using SafeString.print(value,decPlaces,width); to format and print to a SafeString a double/float/long/int"));
  Serial.println(F("SafeString::setOutput(Serial); // verbose"));
  // see the SafeString_ConstructorAndDebugging example for debugging settings
  SafeString::setOutput(Serial); // enable full debugging error msgs

  Serial.println(F(" cSF(sfStr, 70); // where to print to text "));
  cSF(sfStr, 70);
  double d = -0.12345123;
  Serial.println(F(" Keep increasing the size of the value until it won't fit in width 9"));
  Serial.println();
  while (!sfStr.hasError()) { // double not too large to fit in 9
    d = d * 10;
    Serial.print(F(" Formatting value d=")); Serial.println(d, 7);
    sfStr = ""; // clear last output
    sfStr.print(F(" using sfStr.print(d,5,9)     |"));
    sfStr.print(d, 5, 9); // 5 is prec requested will be automatically reduced if d will not fit in width 9
    sfStr.println(F("| text following field"));
    Serial.print(sfStr); // OR Serial.print(sfStr.c_str())
  }
  Serial.println();
  Serial.println(F("SafeString Error detected! "));
  Serial.println(F(" NOTE: when the integer number part is too large for the field, "));
  Serial.println(F("       the field is just padded with blanks are concatinated to the SafeString, e.g."));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())
  Serial.println();
  
  Serial.println(F(" By default the + sign is not shown"));
  d = 0.12345123;
  Serial.print(F(" Formatting value d=")); Serial.println(d, 7);
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(d,5,9)     |"));
  sfStr.print(d, 5, 9); // 5 is prec requested will be automatically reduced if d will not fit in width 9
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())
  
  Serial.println();
  Serial.println(F(" But you can force it to be shown by adding the optional bool argument forceSign"));
  d = 0.12345123;
  Serial.print(F(" Formatting value d=")); Serial.println(d, 7);
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(d,5,9,true) |"));
  sfStr.print(d, 5, 9,true); // 5 is prec requested will be automatically reduced if d will not fit in width 9
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())
  
  Serial.println();
  Serial.println(F(" If width is +v the output is right adjusts, -ve width left adjusts"));
  d = 0.12345123;
  Serial.print(F(" Formatting value d=")); Serial.println(d, 7);
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(d,5,-9,true)|"));
  sfStr.print(d, 5, -9,true); // 5 is prec requested will be automatically reduced if d will not fit in width 9
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())

  Serial.println();
  Serial.println(F(" 0.0 is + if forceSign is true"));
  d = 0.0;
  Serial.print(F(" Formatting value d=")); Serial.println(d, 7);
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(d,7,-9,true)|"));
  sfStr.print(d, 7, -9,true); // 5 is prec requested will be automatically reduced if d will not fit in width 9
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())

  Serial.println();
  Serial.println(F(" You can also use print(value,decPlace,width) to format integer to fixed width, just set decPlace == 0"));
  int i = 3264;
  Serial.print(F(" Formatting value i=")); Serial.println(i);
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(i,0,9)      |"));
  sfStr.print(i, 0, 9); // 0 is prec for integer output
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())

  Serial.println();
  i = 0; 
  Serial.print(F(" Formatting value i=")); Serial.print(i); Serial.println(F(" with forceSign = true"));
  sfStr = ""; // clear last output
  sfStr.print(F(" using sfStr.print(i,0,-9,true)|")); 
  sfStr.print(i, 0, -9,true); // 0 is prec for integer output
  sfStr.println(F("| text following field"));
  Serial.print(sfStr); // OR Serial.print(sfStr.c_str())

}

void loop() {
  // put your main code here, to run repeatedly:

}
