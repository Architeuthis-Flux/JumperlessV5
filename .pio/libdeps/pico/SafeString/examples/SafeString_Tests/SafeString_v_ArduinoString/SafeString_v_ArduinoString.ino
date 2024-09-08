#include <SafeString.h>

String emptyStr1("");
cSF(sfemptyStr1, 2, "");
String emptyStr2("");
cSF(sfemptyStr2, 2, "");
String nonEmptyStr("non");
cSF(sfnonEmptyStr, 3, "non");

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  //  SafeString::setOutput(Serial);
  if (SafeString::errorDetected()) {
    while (1) {
      Serial.println(F("Error in SafeString globals!!"));
      delay(3000);
    }
  }
  Serial.println(F(" This sketch compares the results for SafeString v Arduino Strings for empty strings"));
  Serial.println();
  Serial.print(F("emptyStr1 == emptyStr2:")); Serial.println(emptyStr1 == emptyStr2 ? "true" : "false");
  Serial.print(F("sfemptyStr1 == sfemptyStr2:")); Serial.println(sfemptyStr1 == sfemptyStr2 ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1 == nonEmptyStr:")); Serial.println(emptyStr1 == nonEmptyStr ? "true" : "false");
  Serial.print(F("sfemptyStr1 == sfnonEmptyStr:")); Serial.println(sfemptyStr1 == sfnonEmptyStr ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1 > emptyStr2:")); Serial.println(emptyStr1 > emptyStr2 ? "true" : "false");
  Serial.print(F("sfemptyStr1 > sfemptyStr2:")); Serial.println(sfemptyStr1 > sfemptyStr2 ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1 > nonEmptyStr:")); Serial.println(emptyStr1 > nonEmptyStr ? "true" : "false");
  Serial.print(F("sfemptyStr1 > sfnonEmptyStr:")); Serial.println(sfemptyStr1 > sfnonEmptyStr ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1 < nonEmptyStr:")); Serial.println(emptyStr1 < nonEmptyStr ? "true" : "false");
  Serial.print(F("sfemptyStr1 < sfnonEmptyStr:")); Serial.println(sfemptyStr1 < sfnonEmptyStr ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1.startsWith(emptyStr2):")); Serial.println(emptyStr1.startsWith(emptyStr2) ? "true" : "false");
  Serial.print(F("sfemptyStr1.startsWith(sfemptyStr2):")); Serial.println(sfemptyStr1.startsWith(sfemptyStr2) ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1.indexOf(emptyStr2):")); Serial.println(emptyStr1.indexOf(emptyStr2));
  Serial.print(F("sfemptyStr1.indexOf(sfemptyStr2):")); Serial.println(sfemptyStr1.indexOf(sfemptyStr2));
  Serial.println();

  Serial.print(F("emptyStr1.endsWith(emptyStr2):")); Serial.println(emptyStr1.endsWith(emptyStr2) ? "true" : "false");
  Serial.print(F("sfemptyStr1.endsWith(sfemptyStr2):")); Serial.println(sfemptyStr1.endsWith(sfemptyStr2) ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf(emptyStr2):")); Serial.println(emptyStr1.lastIndexOf(emptyStr2));
  Serial.print(F("sfemptyStr1.lastIndexOf(sfemptyStr2):")); Serial.println(sfemptyStr1.lastIndexOf(sfemptyStr2));
  Serial.println();

  Serial.print(F("nonEmptyStr.startsWith(emptyStr2,nonEmptyStr.length()):")); Serial.println(nonEmptyStr.startsWith(emptyStr2, nonEmptyStr.length()) ? "true" : "false");
  Serial.print(F("sfnonEmptyStr.startsWith(sfemptyStr2,sfnonEmptyStr.length()):")); Serial.println(sfnonEmptyStr.startsWith(sfemptyStr2, sfnonEmptyStr.length()) ? "true" : "false");
  Serial.println();
  Serial.print(F("nonEmptyStr.indexOf(emptyStr2,nonEmptyStr.length()):")); Serial.println(nonEmptyStr.indexOf(emptyStr2, nonEmptyStr.length()));
  Serial.print(F("sfnonEmptyStr.indexOf(sfemptyStr2,sfnonEmptyStr.length()):")); Serial.println(sfnonEmptyStr.indexOf(sfemptyStr2, sfnonEmptyStr.length()));
  Serial.println();

  Serial.print(F("emptyStr1.endsWith(nonEmptyStr):")); Serial.println(emptyStr1.endsWith(nonEmptyStr) ? "true" : "false");
  Serial.print(F("sfemptyStr1.endsWith(sfnonEmptyStr):")); Serial.println(sfemptyStr1.endsWith(sfnonEmptyStr) ? "true" : "false");
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf(nonEmptyStr):")); Serial.println(emptyStr1.lastIndexOf(nonEmptyStr));
  Serial.print(F("sfemptyStr1.lastIndexOf(sfnonEmptyStr):")); Serial.println(sfemptyStr1.lastIndexOf(sfnonEmptyStr));
  Serial.println();

  Serial.print(F("emptyStr1.indexOf('\\0'):")); Serial.println(emptyStr1.indexOf('\0'));
  Serial.print(F("sfemptyStr1.indexOf('\\0'):")); Serial.println(sfemptyStr1.indexOf('\0'));
  Serial.println();
  Serial.print(F("nonEmptyStr.indexOf('\\0'):")); Serial.println(nonEmptyStr.indexOf('\0'));
  Serial.print(F("sfnonEmptyStr.indexOf('\\0'):")); Serial.println(sfnonEmptyStr.indexOf('\0'));
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf('\\0'):")); Serial.println(emptyStr1.lastIndexOf('\0'));
  Serial.print(F("sfemptyStr1.lastIndexOf('\\0'):")); Serial.println(sfemptyStr1.lastIndexOf('\0'));
  Serial.println();
  Serial.print(F("nonEmptyStr.lastIndexOf('\\0'):")); Serial.println(nonEmptyStr.lastIndexOf('\0'));
  Serial.print(F("sfnonEmptyStr.lastIndexOf('\\0'):")); Serial.println(sfnonEmptyStr.lastIndexOf('\0'));
  Serial.println();
  Serial.print(F("emptyStr1.indexOf('\\0',-1):")); Serial.println(emptyStr1.indexOf('\0', -1));
  Serial.print(F("sfemptyStr1.indexOf('\\0',-1):")); Serial.println(sfemptyStr1.indexOf('\0', -1));
  Serial.println();
  Serial.print(F("nonEmptyStr.indexOf('\\0',-1):")); Serial.println(nonEmptyStr.indexOf('\0', -1));
  Serial.print(F("sfnonEmptyStr.indexOf('\\0',-1):")); Serial.println(sfnonEmptyStr.indexOf('\0', -1));
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf('\\0',-1):")); Serial.println(emptyStr1.lastIndexOf('\0', -1));
  Serial.print(F("sfemptyStr1.lastIndexOf('\\0',-1):")); Serial.println(sfemptyStr1.lastIndexOf('\0', -1));
  Serial.println();
  Serial.print(F("nonEmptyStr.lastIndexOf('\\0',-1):")); Serial.println(nonEmptyStr.lastIndexOf('\0', -1));
  Serial.print(F("sfnonEmptyStr.lastIndexOf('\\0',-1):")); Serial.println(sfnonEmptyStr.lastIndexOf('\0', -1));
  Serial.println();

  Serial.print(F("emptyStr1.indexOf('\\0',emptyStr1.length()):")); Serial.println(emptyStr1.indexOf('\0', emptyStr1.length()));
  Serial.print(F("sfemptyStr1.indexOf('\\0',sfemptyStr1.length()):")); Serial.println(sfemptyStr1.indexOf('\0', sfemptyStr1.length()));
  Serial.println();
  Serial.print(F("nonEmptyStr.indexOf('\\0',nonEmptyStr.length()):")); Serial.println(nonEmptyStr.indexOf('\0', nonEmptyStr.length()));
  Serial.print(F("sfnonEmptyStr.indexOf('\\0',sfnonEmptyStr.length()):")); Serial.println(sfnonEmptyStr.indexOf('\0', sfnonEmptyStr.length()));
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf('\\0',emptyStr1.length()):")); Serial.println(emptyStr1.lastIndexOf('\0', emptyStr1.length()));
  Serial.print(F("sfemptyStr1.lastIndexOf('\\0',sfemptyStr1.length()):")); Serial.println(sfemptyStr1.lastIndexOf('\0', sfemptyStr1.length()));
  Serial.println();
  Serial.print(F("nonEmptyStr.lastIndexOf('\\0',nonEmptyStr.length()):")); Serial.println(nonEmptyStr.lastIndexOf('\0', nonEmptyStr.length()));
  Serial.print(F("sfnonEmptyStr.lastIndexOf('\\0',sfnonEmptyStr.length()):")); Serial.println(sfnonEmptyStr.lastIndexOf('\0', sfnonEmptyStr.length()));
  Serial.println();

  Serial.print(F("emptyStr1.indexOf('\\0',emptyStr1.length()-1):")); Serial.println(emptyStr1.indexOf('\0', emptyStr1.length() - 1));
  Serial.print(F("sfemptyStr1.indexOf('\\0',sfemptyStr1.length()-1):")); Serial.println(sfemptyStr1.indexOf('\0', sfemptyStr1.length() - 1));
  Serial.println();
  Serial.print(F("nonEmptyStr.indexOf('\\0',nonEmptyStr.length()-1):")); Serial.println(nonEmptyStr.indexOf('\0', nonEmptyStr.length() - 1));
  Serial.print(F("sfnonEmptyStr.indexOf('\\0',sfnonEmptyStr.length()-1):")); Serial.println(sfnonEmptyStr.indexOf('\0', sfnonEmptyStr.length() - 1));
  Serial.println();
  Serial.print(F("emptyStr1.lastIndexOf('\\0',emptyStr1.length()-1):")); Serial.println(emptyStr1.lastIndexOf('\0', emptyStr1.length() - 1));
  Serial.print(F("sfemptyStr1.lastIndexOf('\\0',sfemptyStr1.length()-1):")); Serial.println(sfemptyStr1.lastIndexOf('\0', sfemptyStr1.length() - 1));
  Serial.println();
  Serial.print(F("nonEmptyStr.lastIndexOf('\\0',nonEmptyStr.length()-1):")); Serial.println(nonEmptyStr.lastIndexOf('\0', nonEmptyStr.length() - 1));
  Serial.print(F("sfnonEmptyStr.lastIndexOf('\\0',sfnonEmptyStr.length()-1):")); Serial.println(sfnonEmptyStr.lastIndexOf('\0', sfnonEmptyStr.length() - 1));
  Serial.println();

  Serial.print(F("sfnonEmptyStr.indexOfCharFrom(sfemptyStr1):")); Serial.println(sfnonEmptyStr.indexOfCharFrom(sfemptyStr1));
  Serial.print(F("sfemptyStr1.indexOfCharFrom(sfnonEmptyStr):")); Serial.println(sfemptyStr1.indexOfCharFrom(sfnonEmptyStr));
  Serial.println();
}

void loop() {
}
