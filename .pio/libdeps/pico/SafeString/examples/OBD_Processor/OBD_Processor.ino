/*
  OBD_Processor.ino  for SafeString V4.0.0+

  Example of using SafeString to process Car OnBoardData

  by Matthew Ford
  Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  This example code is in the public domain.

  download and install the SafeString library V4.0.0+ from Arduino library manager
  or from www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
*/


#include "SafeString.h"
// see https://github.com/PowerBroker2/ELMduino/issues/44#issuecomment-712617781
//Rawdata from OBD:
// some test data
char rawData[] = "7F22127F22127F22127F22127F221203E0:620101FFF7E71:FF8A00000000832:00240ED71713133:141713000010C14:22C12800008B005:00450B0000434B6:000019A80000187:1200200EE70D018:7B0000000003E8";

// where to store the results
struct dataFrames_struct {
  char frames[9][20]; // 9 frames each of 20chars
};

typedef struct dataFrames_struct dataFrames; // create a simple name for this type of data
dataFrames results; // this struct will hold the results

void setup() {
  // Open serial communications and wait a few seconds
  Serial.begin(9600);
  for (int i = 10; i > 0; i--) {
    Serial.print(' '); Serial.print(i);
    delay(500);
  }
  Serial.println();
  SafeString::setOutput(Serial);
}

void clearResultFrames(dataFrames & results) {
  for (int i = 0; i < 9; i++) {
    results.frames[i][0] = '\0';
  }
}

// format is <headerBytes> then <frameNumberByte>:<frameDataBytes> repeated
void processPayload(char *OBDdata, dataFrames & results) {
  clearResultFrames(results);
  cSFP(data, OBDdata); // wrap in a SafeString, use strlen(OBDdata) to set the capacity since ELMduino terminates the returned data char[]
  //Serial.println(OBDdata);
  int idx = data.indexOf(':'); // skip over header and find first delimiter
  while (idx >= 0) {
    int frameIdx = data[idx - 1] - '0'; // the char before :
    if ((frameIdx < 0) || (frameIdx > 8)) { // error in frame number skip this frame, print a message here
      SafeString::Output.print("frameIdx:"); SafeString::Output.print(frameIdx); SafeString::Output.print(" outside range data: "); data.debug();
      idx = data.indexOf(':', idx + 1); // step over : and find next :
      continue;
    }
    cSFA(frame, results.frames[frameIdx]); // wrap a result frame in a SafeString to store this frame's data
    idx++; // step over :
    int nextIdx = data.indexOf(':', idx); // find next : -1 if not found
    if (nextIdx > 0) {
      data.substring(frame, idx, nextIdx - 1); // next : found so take chars upto 1 char before next :
    } else {
      data.substring(frame, idx, nextIdx); // next : not found so take all the remaining chars as this field
    }
    SafeString::Output.print("frameIdx:"); SafeString::Output.print(frameIdx); SafeString::Output.print(" "); frame.debug();
    idx = nextIdx; // step onto next frame
  }
}

int convertToInt(char* dataFrame, unsigned int offset, unsigned int numberBytes) {
  // define a local SafeString on the stack for this method
  cSFP(frame, dataFrame);
  cSF(hexSubString, frame.capacity()); // allow for taking entire frame as a substring
  frame.substring(hexSubString, offset, offset + (numberBytes * 2)); // endIdx in exclusive in SafeString V2+
  hexSubString.debug(F(" hex number "));
  long num = 0;
  if (!hexSubString.hexToLong(num)) {
    hexSubString.debug(F(" invalid hex number "));
  }
  SafeString::Output.print(F(" hexToLong:"));SafeString::Output.println(num);
  return num;
}

void printBatteryVolts(dataFrames & results) {
  float BATTv = convertToInt(results.frames[2], 4, 2) / 10.0;
  Serial.print("Battery Volts:"); Serial.println(BATTv, 1);
}

bool haveData = true;

void loop() {
  // .....
  // read in rawData via ODB
  /** code to actually read the ODB data
    myELM327.sendCommand("AT SH 7E4");       // Set Header BMS
    if (myELM327.queryPID("220101")) {      // BMS PID = hex 22 0101 => dec 34, 257
      char* payload = myELM327.payload;
      //size_t payloadLen = myELM327.recBytes; // But ELMduino terminates the result with '\0' so can just use strlen()
      processPayload(payload, results);
      printBatteryVolts(results); // plus other data
  */
  // test data code
  if (haveData) { // use test data instead of reading OBD
    char *payload = rawData;
    processPayload(payload, results);
    printBatteryVolts(results); // plus other data
    haveData = false; // only do this once
  }
}
