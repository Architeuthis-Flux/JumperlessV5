/*
  SafeStringReader_GPS.ino

  This example reads GPS data from a SafeStringStream continuously using a SafeStringReader

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/

// Only the $GPRMC message is parsed.
//
#include "SafeString.h"
#include "SafeStringReader.h"
#include "SafeStringStream.h"
#include "BufferedOutput.h"

//#define TEST_DATA

#ifdef TEST_DATA
const uint32_t TESTING_BAUD_RATE = 9600; // how fast to release the data from sfStream to be read by this sketch
SafeStringStream sfStream;
cSF(sfTestData, 120);
#endif

createSafeStringReader(sfReader, 80, '\n');

createBufferedOutput(output, 100, DROP_UNTIL_EMPTY); // create an extra output buffer

// UTC date/time
int year;
int month;
int day;
int hour;
int minute;
float seconds;
int latDegs = 0; float latMins = 0.0;
int longDegs = 0; float longMins = 0.0;
float speed = 0.0;
float angle = 0.0;


void setup() {
  Serial.begin(9600);    // Open serial communications and wait a few seconds
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  output.connect(Serial); // connect the output buffer to the Serial, this flushes Serial
  SafeString::setOutput(output); // enable error messages and debug() output to be sent to output
  sfReader.echoOn(); // echo the input

#ifdef TEST_DATA
  Serial.print("Automated Serial testing at "); Serial.println(TESTING_BAUD_RATE);
  Serial.println("Comment out #define TEST_DATA to disable test data");
  sfReader.connect(sfStream); // read from test data
  sfTestData = F(
                 "$GPRMC,194509.000,A,4042.6142,N,07400.4168,W,2.03,221.11,160412,,,A*77\n"
                 "$GPVTG,054.7,T,034.4,M,005.5,N,010.2,K*48\n"
               ); // initialized the test data
  Serial.println("Test Data:-");
  Serial.println(sfTestData);
  Serial.println();
  sfStream.begin(sfTestData, TESTING_BAUD_RATE); // NOTE: do this last !!! or will miss first few chars of sfTestData
#else
  Serial.println();
  Serial.println(F(" Set Newline in Arduino Monitor and"));
  Serial.println(F("enter GPS msg $GPxxx,..."));
  Serial.println(F("e.g. $GPRMC,194509.000,A,4042.6142,N,07400.4168,W,2.03,221.11,160412,,,A*77"));
  Serial.println("Uncomment #define TEST_DATA to use test data");
  sfReader.connect(output);
  sfReader.echoOn();
#endif
}

bool checkSum(SafeString &msg) {
  int idxStar = msg.indexOf('*');
  // could do these checks also
  // BUT SafeString will just return empty sfCheckSumHex and so fail to hexToLong conversion below
  //  if (idxStar < 0) {
  //    return false; // missing * //this also checks for empty string
  //  }
  //  // check for 2 chars
  //  if (((msg.length()-1) - idxStar) != 2) { // msg.length() -1 is the last idx of the msg
  //    return false; // too few or to many chars after *
  //  }
  cSF(sfCheckSumHex, 2);
  // if * not found then idxStar == -1 and idxStart+1 == 0 and sfCheckSumHex will be too small to hold substring and be returned empty
  msg.substring(sfCheckSumHex, idxStar + 1); // next 2 chars SafeString will complain and return empty substring if more than 2 chars
  long sum = 0;
  if (!sfCheckSumHex.hexToLong(sum)) {
    return false; // not a valid hex number
  }
  for (int i = 1; i < idxStar; i++) { // skip the $ and the *checksum
    sum ^= msg[i];
  }
  return (sum == 0);
}

void parseTime(SafeString &timeField) {
  float timef = 0.0;
  if (!timeField.toFloat(timef)) { // an empty field is not a valid float
    return; // not valid skip the rest
  }
  uint32_t time = timef;
  hour = time / 10000;
  minute = (time % 10000) / 100;
  seconds = fmod(timef, 100.0);
}

void parseDegMin(SafeString &degField, int &d, float& m, int degDigits) {
  cSF(sfSub, 10); // temp substring
  degField.substring(sfSub, 0, degDigits); // degs, 2 for lat, 3 for long
  int degs = 0;
  if (!sfSub.toInt(degs)) {
    return; // invalid
  }
  float mins = 0.0;
  degField.substring(sfSub, degDigits, degField.length()); // mins
  if (!sfSub.toFloat(mins)) {
    return; // invalid
  }
  // both deg/mins valid update returns
  d = degs;
  m = mins;
}

void parseDate(SafeString &dateField) {
  long lDate = 0;
  if (!dateField.toLong(lDate)) {
    return; // invalid
  }
  day = lDate / 10000;
  month = (lDate % 10000) / 100;
  year = (lDate % 100);
}
/**
   Fields: (note fields can be empty)
    123519.723   Fix taken at 12:35:19,723 UTC
    A            Status A=active or V=Void.
    4807.038,N   Latitude 48 deg 07.038' N
    01131.000,E  Longitude 11 deg 31.000' E
    022.4        Speed over the ground in knots
    084.4        Track angle in degrees True
    230394       Date 23rd of March 1994
    003.1,W      Magnetic Variation
*/
// just leaves existing values unchanged if new ones are not valid invalid
// returns false if msg Not Active
bool parseGPRMC(SafeString &msg) {

  cSF(sfField, 11); // temp SafeString to received fields, max field len is <11;
  char delims[] = ",*"; // fields delimited by , or *
  bool returnEmptyFields = true; // return empty field for ,,
  int idx = 0;
  idx = msg.stoken(sfField, idx, delims, returnEmptyFields);
  if (sfField != "$GPRMC") {  // first field should be $GPRMC else called with wrong msg
    return false;
  }

  cSF(sfTimeField, 11); // temp SafeString to hold time for later passing, after checking 'A'
  idx = msg.stoken(sfTimeField, idx, delims, returnEmptyFields); // time, keep for later

  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // A / V
  if (sfField != 'A') {
    return false; // not active
  }
  // else A so update time
  parseTime(sfTimeField);

  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // Lat
  parseDegMin(sfField, latDegs, latMins, 2);

  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // N / S or empty
  if (sfField == 'S') {
    latDegs = -latDegs;
  }
  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // Long
  parseDegMin(sfField, longDegs, longMins, 3);

  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // N / S or empty
  if (sfField == 'W') {
    longDegs = -longDegs;
  }
  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // speed
  if (!sfField.toFloat(speed) ) {
    // invalid, speed not changed
  }
  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // track angle true
  if (!sfField.toFloat(angle) ) {
    // invalid, angle not changed
  }
  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // date
  parseDate(sfField);

  idx = msg.stoken(sfField, idx, delims, returnEmptyFields); // magnetic variation
  // skip parsing this for now
  return true;
}

void print2digits(Print& out, int num) {
  if (num <= 9) {
    out.print('0');
  }
  out.print(num);
}

void printPosition() {
  cSF(results, 60);
  results.concat(F(" > > > "));
  results.concat(F("20"));
  print2digits(results, year); results.concat('/'); print2digits(results, month); results.concat('/'); print2digits(results, day);
  results.concat(F("  "));
  print2digits(results, hour); results.concat(':'); print2digits(results, minute); results.concat(':'); print2digits(results, seconds);
  results.concat(F("  "));
  results.concat(longDegs).concat(' ').concat(longMins).concat(F("', ")).concat(latDegs).concat(' ').concat(latMins).concat(F("'")).newline();
  output.clearSpace(results.length()); // only clears space in the extra buffer not the Serial tx buffer
  output.print(results);
}

void loop() {
  output.nextByteOut();
  if (sfReader.read()) { // echo on write back chars read to end of sfStream
    sfReader.trim(); // remove and leading/trailing white space
#ifdef TEST_DATA
    output.println(sfReader); // display input
#endif
    if (checkSum(sfReader)) { // if the check sum is OK
      if (sfReader.startsWith("$GPRMC,")) {  // this is the one we want
        if (parseGPRMC(sfReader)) {
          printPosition(); // print new data
        }
      } else {  // ignore but print first 10 chars
        cSF(tmp, 10);
        output.print(sfReader.substring(tmp, 0, 7)); output.println("...");
      }
    } else {
      output.clearSpace(12); // make space for at least the start of this error
      output.print("bad checksum : "); output.println(sfReader);
    }
  } // else token is empty
}
