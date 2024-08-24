/* SafeStringInClasses_1.ino
    Example of using SafeString for Class char[]s

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/
#include <SafeString.h>

// ===================================================================
// The class with char[] private variables
class Loco {
  private:
    // make these private to protect from external access
    char _niceName[12];
    char _locoName[12];
    int _locoAddress;

  public:
    // Members
    Loco(const char* niceName, const char* locoName, int locoAddress);
    void printData(Stream& out);
};

// class methods
Loco::Loco(const char* niceName, const char* locoName, int locoAddress) {
  cSFA(sfNiceName, _niceName); //wrap in SafeStrings for processing
  cSFA(sfLocoName, _locoName);
  sfNiceName = niceName;  // SafeString now checks that niceName is not too long
  sfLocoName = locoName;
  _locoAddress = locoAddress;
}
void Loco::printData(Stream &out) {
  out.print(" niceName:"); out.println((char*)_niceName);
  out.print(" locoName:"); out.println((char*)_locoName);
  out.print(" location:"); out.println(_locoAddress);
}
// ===================================================================

// usually the class instances are constructed here
// but if SafeString::errorDetected()
// then move them to inside setup() as shown below to see what the error message is

//Loco locoBad("This nice name is too long", "and so is this loco name", 5);

Loco locoGood("fred", "35454", 5);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  for (int i = 10; i > 0; i--) {
    Serial.print(i); Serial.print(' ');
    delay(500);
  }
  Serial.println();
  //SafeString::setOutput(Serial); // enable error msgs

  if (SafeString::errorDetected()) { // check for errors in global SafeStrings, like locoGood
    while (1) {
      Serial.println(F("Error constructing SafeStrings!!"));
      delay(3000);
    }
  }
  // try and create a loco with names too long
  Loco locoBad("This nice name is too long", "and so is this loco name", 5);
  if (SafeString::errorDetected()) {  // check for errors
    Serial.println(F("Error constructing locoBad"));
    Serial.println(F("Turn on SafeString error msgs with SafeString::setOutput(Serial);"));
  }
  Serial.println(" the locoBad contains");
  locoBad.printData(Serial);

  Serial.println();
  Serial.println(" the locoGood contains");
  locoGood.printData(Serial);
}
void loop() {
}
