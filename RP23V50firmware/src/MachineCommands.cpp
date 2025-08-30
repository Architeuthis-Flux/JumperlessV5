// SPDX-License-Identifier: MIT
#ifndef MACHINECOMMANDS_H
#define MACHINECOMMANDS_H

#include "JumperlessDefines.h"  // for INPUTBUFFERLENGTH, MAX_NETS, MAX_NODES, etc.
#include "MachineCommands.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <ctype.h>

#include "FileParsing.h"
#include "LittleFS.h"
#include "MatrixState.h"
#include "SafeString.h"
#include "NetManager.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "PersistentStuff.h"



// Keep this EXACTLY in sync with the instruction table below.
#define NUMBEROFINSTRUCTIONS 17

enum machineModeInstruction {
    unknown = 0,
    netlist,
    getnetlist,
    bridgelist,
    getbridgelist,
    lightnode,
    lightnet,
    getmeasurement,
    gpio,
    uart,
    arduinoflash,
    setnetcolor,
    setnodecolor,
    setsupplyswitch,
    getsupplyswitch,
    getchipstatus,
    getunconnectedpaths
};

// Provided elsewhere; size must match INPUTBUFFERLENGTH
extern char inputBuffer[INPUTBUFFERLENGTH];
extern char machineModeInstructionString[NUMBEROFINSTRUCTIONS][20];

// API
enum machineModeInstruction parseMachineInstructions(int *sequenceNumber);
void machineModeRespond(int sequenceNumber, bool ok);
void machineMode(void);

void getUnconnectedPaths(void);
void machineNetlistToNetstruct(void);
void populateBridgesFromNodes(void);
int nodeTokenToInt(char *);
int findReplacement(char *name);
int removeHexPrefix(const char *);

void writeNodeFileFromInputBuffer(void);
void lightUpNodesFromInputBuffer(void);
void lightUpNetsFromInputBuffer(void);

void printSupplySwitch(int supplySwitchPos);
int setSupplySwitch(void);

void listNetsMachine(void);
void listBridgesMachine(void);
void printChipStatusMachine();

#endif // MACHINECOMMANDS_H


// use global declared in PersistentStuff.h
extern bool debugMM;

File nodeFileMachineMode;

createSafeString(nodeString, 1200);

// ArduinoJson v7 style\ nArduinoJson::JsonDocument machineModeJson;
ArduinoJson::JsonDocument machineModeJson;

static enum machineModeInstruction lastReceivedInstruction = unknown;

// Keep this table in-sync with enum machineModeInstruction and NUMBEROFINSTRUCTIONS
char machineModeInstructionString[NUMBEROFINSTRUCTIONS][20] = {
    "unknown",
    "netlist",
    "getnetlist",
    "bridgelist",
    "getbridgelist",
    "lightnode",
    "lightnet",
    "getmeasurement",
    "gpio",
    "uart",
    "arduinoflash",
    "setnetcolor",
    "setnodecolor",
    "setsupplyswitch",
    "getsupplyswitch",
    "getchipstatus",
    "getunconnectedpaths"
};

static unsigned long lastTimeCommandRecieved = 0;
static unsigned long lastTimeNetlistLoaded = 0;

void machineMode(void) {
  int sequenceNumber = -1;
  // throttle very chatty hosts
  if (millis() - lastTimeCommandRecieved < 100) {
    machineModeRespond(sequenceNumber, true);
    return;
  }
  enum machineModeInstruction receivedInstruction = parseMachineInstructions(&sequenceNumber);
  lastTimeCommandRecieved = millis();

  switch (receivedInstruction) {
    case netlist: {
      lastTimeNetlistLoaded = millis();
      clearAllNTCC();
      digitalWrite(RESETPIN, HIGH);
      machineNetlistToNetstruct();
      populateBridgesFromNodes();
      bridgesToPaths();
      clearLEDs();
      assignNetColors();
      digitalWrite(RESETPIN, LOW);
      sendAllPathsCore2 = 1;
      break;
    }
    case getnetlist: {
      if (millis() - lastTimeNetlistLoaded > 300) {
        listNetsMachine();
      } else {
        machineModeRespond(sequenceNumber, true);
        return;
      }
      break;
    }
    case bridgelist: {
      clearAllNTCC();
      writeNodeFileFromInputBuffer();
      openNodeFile();
      getNodesToConnect();
      digitalWrite(RESETPIN, HIGH);
      bridgesToPaths();
      clearLEDs();
      assignNetColors();
      digitalWrite(RESETPIN, LOW);
      sendAllPathsCore2 = 1;
      break;
    }
    case getbridgelist:
      listBridgesMachine();
      break;
    case lightnode:
      lightUpNodesFromInputBuffer();
      break;
    case lightnet:
      lightUpNetsFromInputBuffer();
      break;
    case setsupplyswitch:
      machineModeRespond(sequenceNumber, true);
      break;
    case getsupplyswitch:
      break;
    case getchipstatus:
      printChipStatusMachine();
      break;
    case getmeasurement:
    case gpio:
    case uart:
    case arduinoflash:
    case setnetcolor:
    case setnodecolor:
      // not implemented here — intentionally no-op
      break;
    case unknown:
    default:
      machineModeRespond(sequenceNumber, false);
      return;
  }

  machineModeRespond(sequenceNumber, true);
}

// Bounded reader: read until ']' or buffer full
// Expected: "<instruction>[:<seq>][<payload>]" (payload ends on first ']')

enum machineModeInstruction parseMachineInstructions(int *sequenceNumber) {
  if (sequenceNumber) *sequenceNumber = -1;

  size_t idx = 0;
  unsigned long t0 = millis();
  const unsigned long timeoutMs = 50;
  while ((millis() - t0) < timeoutMs && idx < (INPUTBUFFERLENGTH - 1)) {
    if (!Serial.available()) { delayMicroseconds(150); continue; }
    int c = Serial.read();
    if (c < 0) continue;
    inputBuffer[idx++] = (char)c;
    if ((char)c == ']') break;
  }
  inputBuffer[idx] = '\0';

  char instructionBuffer[20] = {0};
  int seqSep = -1;
  size_t ib = 0;
  while (ib < sizeof(instructionBuffer) - 1 && inputBuffer[ib] && inputBuffer[ib] != '[') {
    if (inputBuffer[ib] == ':') seqSep = (int)ib;
    instructionBuffer[ib] = inputBuffer[ib];
    ++ib;
  }
  instructionBuffer[ib] = 0;

  if (seqSep > 0 && sequenceNumber) {
    instructionBuffer[seqSep] = 0;
    *sequenceNumber = atoi(&inputBuffer[seqSep + 1]);
  }

  int instructionNumber = 0; // default unknown
  for (int i = 0; i < NUMBEROFINSTRUCTIONS; i++) {
    if (strcasecmp(instructionBuffer, machineModeInstructionString[i]) == 0) {
      instructionNumber = i;
      break;
    }
  }

  lastReceivedInstruction = static_cast<machineModeInstruction>(instructionNumber);
  return lastReceivedInstruction;
}

void machineModeRespond(int sequenceNumber, bool ok) {
  Serial.print(ok ? "::ok" : "::error");
  if (sequenceNumber >= 0) { Serial.print(":"); Serial.print(sequenceNumber); }
  Serial.println("");
}

void getUnconnectedPaths(void) {
  if (numberOfUnconnectablePaths == 0) return;
  Serial.print("::unconnectedpaths[");
  for (int i = 0; i < numberOfUnconnectablePaths; i++) {
    if (i > 0) Serial.print(",");
    printNodeOrName(unconnectablePaths[i][0]);
    Serial.print("-");
    printNodeOrName(unconnectablePaths[i][1]);
  }
  Serial.println("]");
}

void machineNetlistToNetstruct(void) {
  DeserializationError err = deserializeJson(machineModeJson, inputBuffer);
  if (err) return;

  static char names_pool[MAX_NETS][32];

  for (int i = 0; i < MAX_NETS; i++) {
    if (machineModeJson[i].isNull()) continue;

    int netIndex = machineModeJson[i]["index"] | i;
    const char *nm = machineModeJson[i]["name"] | "";

    strncpy(names_pool[netIndex], nm, sizeof(names_pool[netIndex]) - 1);
    names_pool[netIndex][sizeof(names_pool[netIndex]) - 1] = '\0';
    net[netIndex].name = names_pool[netIndex];

    net[netIndex].number = machineModeJson[i]["number"] | netIndex;

    //Not sure what was intended/ looks like a ternary operation gone bad.
    // uint32_t rawColor = (uint32_t)removeHexPrefix((machineModeJson[i]["color"].as<const char*>() ?: "0"));

    const char* colorStr = machineModeJson[i]["color"].as<const char*>();
    if (!colorStr) colorStr = "0";
      uint32_t rawColor = (uint32_t)removeHexPrefix(colorStr);

    net[netIndex].rawColor = rawColor;
    net[netIndex].machine = true;

    // Special nets propagate rail colors
    if (netIndex < 8) {
      rawSpecialNetColors[netIndex] = rawColor;
      switch (netIndex) {
        case 1:
          for (int k = 0; k < 3; k++) { rawRailColors[k][1] = rawColor; rawRailColors[k][3] = rawColor; }
          break;
        case 2:
          rawRailColors[1][0] = rawColor; rawRailColors[1][2] = rawColor; break;
        case 3:
          rawRailColors[0][0] = rawColor; rawRailColors[0][2] = rawColor; break;
      }
    }

    // nodes[] parsing (tokens from serialized array)
    char nodesChar[300] = {0};
    serializeJson(machineModeJson[i]["nodes"], nodesChar, sizeof(nodesChar));

    const char delim[] = ",\"- ";
    char *nodeTokens[MAX_NODES] = {0};
    int nodesIndex = 0;

    for (int j = 0; j < MAX_NODES; j++) {
      nodeTokens[j] = (j == 0) ? strtok(nodesChar, delim) : strtok(NULL, delim);
      if (!nodeTokens[j]) break;
      int nodeToAdd = nodeTokenToInt(nodeTokens[j]);
      if (net[netIndex].nodes[0] == nodeToAdd) continue; // avoid dup of first
      if (netIndex < 8 && nodesIndex == 0) nodesIndex = 1; // reserve slot 0 for special nets
      net[netIndex].nodes[nodesIndex++] = nodeToAdd;
    }
  }
}

void populateBridgesFromNodes(void) {
  for (int i = 1; i < MAX_NETS; i++) {
    int addBridgeIndex = 0;
    for (int j = 0; j < MAX_NODES; j++) {
      if (net[i].nodes[j] <= 0) continue;
      if (net[i].nodes[j] == net[i].nodes[0]) continue;
      net[i].bridges[addBridgeIndex][0] = net[i].nodes[0];
      net[i].bridges[addBridgeIndex][1] = net[i].nodes[j];
      if (debugMM) {
        Serial.print("net["); Serial.print(i); Serial.print("] bridge ");
        Serial.print(addBridgeIndex); Serial.print(": ");
        Serial.print(net[i].bridges[addBridgeIndex][0]); Serial.print("-");
        Serial.println(net[i].bridges[addBridgeIndex][1]);
      }
      addBridgeIndex++;
    }
  }
}

int removeHexPrefix(const char *str) {
  if (!str) return 0;
  if (strncmp(str, "0x", 2) == 0 || strncmp(str, "0X", 2) == 0) return (int)strtol(str + 2, nullptr, 16);
  if (str[0] == '#') return (int)strtol(str + 1, nullptr, 16);
  return atoi(str);
}

int nodeTokenToInt(char *nodeToken) {
  if (!nodeToken) return -1;
  char uppercaseToken[20];
  size_t len = strlen(nodeToken);
  if (len >= sizeof(uppercaseToken)) len = sizeof(uppercaseToken) - 1;
  memcpy(uppercaseToken, nodeToken, len);
  uppercaseToken[len] = 0;
  for (size_t i = 0; i < len; i++) uppercaseToken[i] = toupper((unsigned char)uppercaseToken[i]);

  for (int i = 0; i < 90; i++) {
    if (strcmp(uppercaseToken, sfMappings[i].name) == 0) {
      if (debugMM) { Serial.print("mapped value = "); Serial.println(sfMappings[i].replacement); }
      return sfMappings[i].replacement;
    }
  }
  int intToken = atoi(uppercaseToken);
  if (debugMM) { Serial.print("mapped value = "); Serial.println(intToken); }
  return intToken;
}

void writeNodeFileFromInputBuffer(void) {
  LittleFS.remove("nodeFile.txt");
  delayMicroseconds(60);
  nodeFileMachineMode = LittleFS.open("nodeFile.txt", "w+");
  if (!nodeFileMachineMode) { if (debugFP) Serial.println("Failed to open nodeFileMachineMode"); return; }
 else {
    if (debugFP)
      Serial.println("\n\ropened nodeFile.txt\n\n\rloading bridges from file\n\r");
    }

  for (int i = 0; i < INPUTBUFFERLENGTH; i++) {
    if (inputBuffer[i] == '\0') break;
    if (inputBuffer[i] == ']') { if (i > 0 && inputBuffer[i - 1] != ',') nodeFileMachineMode.print(",]"); else nodeFileMachineMode.print("]"); break; }
    nodeFileMachineMode.print(inputBuffer[i]);
  }

  if (debugMM) {
    Serial.println("wrote to nodeFile.txt");
    Serial.println("nodeFile.txt contents:");
    nodeFileMachineMode.seek(0);
    while (nodeFileMachineMode.available()) Serial.write(nodeFileMachineMode.read());
    Serial.println("\n\r");
  }
  nodeFileMachineMode.close();
}

void lightUpNodesFromInputBuffer(void) {
  char *token[50] = {0};
  token[0] = strtok(inputBuffer, ",:[] ");
  int count = 0;
  for (int i = 0; i < 50; i++) { if (i > 0) token[i] = strtok(NULL, ",:[] "); if (!token[i]) { count = i; break; } }
  if (count < 2) return; // need at least one node and a color

  uint32_t color = (uint32_t)removeHexPrefix(token[count - 1]);
  for (int i = 0; i < count - 1; i++) {
    int nodeNumber = nodeTokenToInt(token[i]);
    if (nodeNumber == -1) continue;
    lightUpNode(nodesToPixelMap[nodeNumber], color);
  }
}

void printSupplySwitch(int supplySwitchPos) {
  const char *positionString[] = {"3.3V", "5V", "8V"};
  if (supplySwitchPos < 0 || supplySwitchPos > 2) supplySwitchPos = 1; // default 5V
  Serial.print("::supplyswitch[");
  Serial.print(positionString[supplySwitchPos]);
  Serial.println("]");
}

int setSupplySwitch(void) {
  const char *supplySwitchPositionString[] = {"3.3V", "3V3", "+3.3V", "+3V3", "5V", "+5V", "+-8V", "8V"};
  int supplySwitchPositionInt[] = {0, 0, 0, 0, 1, 1, 2, 2};
  char *tok = strtok(inputBuffer, ",:[] \"");
  int supplySwitch = 1; // default 5V
  if (tok) { for (int i = 0; i < 8; i++) { if (strcasecmp(tok, supplySwitchPositionString[i]) == 0) { supplySwitch = supplySwitchPositionInt[i]; break; } } }
  return supplySwitch;
}

void lightUpNetsFromInputBuffer(void) {
  const int numberOfNRNs = 10;
  const char *notReallyNets[] = {"headerglow", "glow", "hg", "+8v", "8v", "-8v", "logo", "status", "logoflash", "statusflash"};
  int notReallyNetsInt[]      = {0,            0,      0,     3,     3,     4,      1,      1,        2,           2};

  char *token[50] = {0};
  token[0] = strtok(inputBuffer, ",:[] ");
  int count = 0;
  for (int i = 0; i < 50; i++) { if (i > 0) token[i] = strtok(NULL, ",:[] "); if (!token[i]) { count = i; break; } }
  if (count < 2) return;

  uint32_t color = (uint32_t)removeHexPrefix(token[count - 1]);

  for (int i = 0; i < count - 1; i++) {
    if (!token[i]) break;
    bool handled = false;
    for (int j = 0; j < numberOfNRNs; j++) {
      if (strcasecmp(token[i], notReallyNets[j]) == 0) {
        rawOtherColors[notReallyNetsInt[j]] = color;
        if (notReallyNetsInt[j] == 3) rawRailColors[2][0] = color; // +8V
        if (notReallyNetsInt[j] == 4) rawRailColors[2][2] = color; // -8V
        handled = true; break;
      }
    }
    if (handled) continue;

    int netNumber = atoi(token[i]);
    if (netNumber < 8) {
      rawSpecialNetColors[netNumber] = color;
      net[netNumber].rawColor = color;
      switch (netNumber) {
        case 1:
          for (int j = 0; j < 3; j++) { rawRailColors[j][1] = color; rawRailColors[j][3] = color; }
          break;
        case 2:
          rawRailColors[1][0] = color; rawRailColors[1][2] = color; break;
        case 3:
          rawRailColors[0][0] = color; rawRailColors[0][2] = color; break;
      }
    } else {
      net[netNumber].rawColor = color;
    }

    if (debugMM) {
      Serial.print("net["); Serial.print(netNumber); Serial.print("] .rawColor = "); Serial.println(net[netNumber].rawColor, HEX);
      Serial.print("net["); Serial.print(netNumber); Serial.print("] .name = "); Serial.println(net[netNumber].name);
    }
  }
}

void listNetsMachine(void) {
  Serial.println("\n\r:netlist-begin");
  for (int i = 1; i < MAX_NETS; i++) {
    struct netStruct *n = &net[i];
    if (n->number == 0 || n->nodes[0] == -1) continue;

    Serial.print("::net[");
    Serial.print(i); Serial.print(',');
    Serial.print(n->number); Serial.print(',');

    for (int j = 0; j < MAX_NODES; j++) {
      if (n->nodes[j] == 0) break;
      if (j > 0) Serial.print(';');
      printNodeOrName(n->nodes[j], 1);
    }
    Serial.print(',');

    Serial.print(n->specialFunction ? "true," : "false,");

    rgbColor color = unpackRgb(scaleUpBrightness(n->rawColor));
    char buf[8];
    snprintf(buf, sizeof(buf), "%.2x%.2x%.2x,", color.r, color.g, color.b);
    Serial.print(buf);

    Serial.print(n->machine ? "true," : "false,");
    Serial.print(n->name);
    Serial.println("]");
  }
  Serial.println("::netlist-end");
}

void listBridgesMachine(void) {
  Serial.print("::bridgelist[");
  bool started = false;
  for (int i = 1; i < MAX_NETS; i++) {
    struct netStruct *n = &net[i];
    if (n->number == 0 || n->nodes[0] == -1) continue;
    for (int j = 0; j < MAX_NODES; j++) {
      if (n->bridges[j][0] <= 0) continue;
      if (started) Serial.print(","); else started = true;
      printNodeOrName(n->bridges[j][0], 1);
      Serial.print("-");
      printNodeOrName(n->bridges[j][1], 1);
    }
  }
  Serial.println("]");
}

void printChipStatusMachine() {
  Serial.println("::chipstatus-begin");
  for (int i = 0; i < 12; i++) {
    Serial.print("::chipstatus[");
    Serial.print(chipNumToChar(i));
    Serial.print(",");
    for (int j = 0; j < 16; j++) { Serial.print(ch[i].xStatus[j]); Serial.print(","); }
    for (int j = 0; j < 8;  j++) { Serial.print(ch[i].yStatus[j]); if (j != 7) Serial.print(","); }
    Serial.println("]");
  }
  Serial.println("::chipstatus-end");
}
