// SPDX-License-Identifier: MIT
#include "FileParsing.h"
#include "ArduinoJson.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
// #include "LittleFS.h"
#include "Commands.h"
// #include "MachineCommands.h"
#include "MatrixState.h"
#include "NetManager.h"
#include "Probing.h"
#include "RotaryEncoder.h"
#include "SafeString.h"
// #include "menuTree.h"
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Peripherals.h"
#include "config.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <FatFS.h>
#ifdef DONOTUSE_SERIALWRAPPER
#include "SerialWrapper.h"
#endif

#ifdef DONOTUSE_SERIALWRAPPER
#define Serial SerialWrap
#endif

volatile bool netsUpdated = true;

bool debugFP = EEPROM.read(DEBUG_FILEPARSINGADDRESS);
bool debugFPtime = EEPROM.read(TIME_FILEPARSINGADDRESS);

createSafeString(nodeFileString, 1800);

// General-purpose nodeFileString backup/restore system
static char nodeFileStringBackup[1800];
static bool nodeFileBackupStored = false;

createSafeString(currentColorSlotColorsString,
                 1500); // Cache for current slot's net colors

// Track which slots have net colors assigned (bit mask for performance)
uint32_t slotsWithNetColors = 0;

// Track which slots have been validated (bit mask for performance)
uint32_t slotsValidated = 0;

int numConnsJson = 0;
createSafeString(specialFunctionsString, 2800);

char inputBuffer[INPUTBUFFERLENGTH] = {0};

// ArduinoJson::StaticJsonDocument<8000> wokwiJson;
// ;

String connectionsW[MAX_BRIDGES][5];

File nodeFile;

File wokwiFile;

File nodeFileBuffer;

File menuTreeFile;

File colorFile; // Added for net color storage

unsigned long timeToFP = 0;

enum openType {
  w = 0,
  wplus = 1,
  r = 2,
  rplus = 3,
  a = 4,
  aplus = 5,

};

const char rotaryConnectionString[] =
    "{AREF-GND,D11-GND,D10-UART_TX,D12-UART_RX,D13-GPIO_0, ";

void closeAllFiles() {
  if (nodeFile) {
    nodeFile.close();
  }
  // if (wokwiFile) {
  //   wokwiFile.close();
  // }
  if (nodeFileBuffer) {
    nodeFileBuffer.close();
  }
}

int openFileThreadSafe(int openTypeEnum, int slot, int flashOrLocal) {

  // Serial.print("openFileThreadSafe   ");
  // unsigned long start = micros();


  core1request = 1;
  while (core2busy == true) {
  }
  core1request = 0;
  core1busy = true;

  // Serial.println(micros() - start);
  if (nodeFile) {
    // Serial.println("nodeFile is open");
    nodeFile.close();
  }

  switch (openTypeEnum) {
  case 0:
    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
    break;
  case 1:
    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w+");
    break;
  case 2:
    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r");
    break;
  case 3:

    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r+");
    break;
  case 4:
    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "a");
    break;
  case 5:

    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "a+");
    break;
  default:
    break;
  }

  if (!nodeFile) {
    // if (debugFP)
    //  Serial.println("\n\n\rFailed to open nodeFile\n\n\r");
    // openFileThreadSafe(w, slot);
    core1busy = false;
    return 0;
  } else {
    if (debugFP)
      Serial.println(
          "\n\ropened nodeFile.txt\n\n\rloading bridges from file\n\r");
  }
  // Serial.print("openFileThreadSafe done   ");
  // Serial.println(micros() - start);
  return 1;
}

void writeMenuTree(void) {
  while (core2busy == true) {
    // Serial.println("waiting for core2 to finish");
  }
  core1busy = true;
  // FatFS.begin();
  //    delay(100);
  //    FatFS.remove("/MenuTree.txt");
  //   delay(100);
  menuTreeFile = FatFS.open("/MenuTree.txt", "w");
  if (!menuTreeFile) {

    Serial.println("Failed to open menuTree.txt");

  } else {
    // if (debugFP)
    // {
    //     Serial.println("\n\ropened menuTree.txt\n\r");
    // }
    // else
    // {
    //     // Serial.println("\n\r");
    // }
  }
  int menuIndex = 0;

  // while (menuTree[menuIndex] != '\0') {
  //   menuTreeFile.print(menuTree[menuIndex]);
  //   // Serial.print(menuTree[menuIndex]);
  //   menuIndex++;
  // }

  // menuTreeFile.write(menuTree);
  // menuTreeFile.print(menuTreeString);
  menuTreeFile.close();
  core1busy = false;
}

void createLocalNodeFile(int slot) {

  openFileThreadSafe(aplus, slot);

  nodeFileString.clear();
  nodeFileString.read(nodeFile);
  nodeFile.close();
  core1busy = false;
  // Serial.println(nodeFileString);
  nodeFileString.replace(" ", "");
  nodeFileString.replace(" ", "");
  nodeFileString.replace("{", "");
  nodeFileString.replace("}", "");
  nodeFileString.prefix("{ ");
  nodeFileString.concat(" } ");

  Serial.println(nodeFileString);
}

void clearNodeFileString() { nodeFileString.clear(); }

void saveLocalNodeFile(int slot) {
  // Serial.println("saving local node file");
  // Serial.print("nodeFileString = ");
  // Serial.println(nodeFileString);

  long count = 0;

  openFileThreadSafe(w, netSlot);
  nodeFileString.replace(" ", "");
  // nodeFileString.replace(" ", "");
  nodeFileString.replace("{", "");
  nodeFileString.replace("}", "");
  nodeFileString.prefix("{ ");
  nodeFileString.concat(" } ");
  // Serial.println("nodeFileString");
  // delay(3);

  nodeFileString.printTo(nodeFile);

  nodeFile.close();
  core1busy = false;
  markSlotAsModified(slot); // Mark slot as needing re-validation
  // Serial.println("\n\n\rsaved local node file");
}

//==============================================================================
// General-purpose nodeFileString backup/restore functions
// These can be used from anywhere to temporarily save and restore nodeFileString state
//==============================================================================

void storeNodeFileBackup(void) {
    // Store the current nodeFileString state in backup buffer
    
    // If nodeFileString is empty, load from current slot first
    if (nodeFileString.isEmpty()) {
        createLocalNodeFile(netSlot);
    }
    
    // Store the current state in our backup buffer
    if (nodeFileString.length() < sizeof(nodeFileStringBackup)) {
        strcpy(nodeFileStringBackup, nodeFileString.c_str());
        nodeFileBackupStored = true;
    } else {
        // If string is too long, just mark as not stored
        nodeFileBackupStored = false;
    }
}

void restoreNodeFileBackup(void) {
    // Restore the nodeFileString from backup buffer
    if (nodeFileBackupStored) {
        nodeFileString.clear();
        nodeFileString.concat(nodeFileStringBackup);
        
        // Optionally save the restored state back to the current slot
        // saveLocalNodeFile(netSlot);
        
        // Clear the backup since we've restored it
        nodeFileBackupStored = false;
    }
}

void restoreAndSaveNodeFileBackup(void) {
    // Restore the nodeFileString from backup and save to slot
    if (nodeFileBackupStored) {
        nodeFileString.clear();
        nodeFileString.concat(nodeFileStringBackup);
        
        // Save the restored state back to the current slot
        saveLocalNodeFile(netSlot);
        
        // Clear the backup since we've restored it
        nodeFileBackupStored = false;
    }
}

void clearNodeFileBackup(void) {
    // Clear the backup buffer
    nodeFileBackupStored = false;
    memset(nodeFileStringBackup, 0, sizeof(nodeFileStringBackup));
}

bool hasNodeFileBackup(void) {
    // Check if we have a backup stored
    return nodeFileBackupStored;
}

bool hasNodeFileChanges(void) {
    // Check if current nodeFileString differs from backup
    if (!nodeFileBackupStored) {
        return false; // No backup to compare against
    }
    
    // Compare current state with backup
    return (strcmp(nodeFileString.c_str(), nodeFileStringBackup) != 0);
}

const char* getNodeFileBackup(void) {
    // Get read-only access to the backup buffer
    return nodeFileBackupStored ? nodeFileStringBackup : nullptr;
}

void createSlots(int slot, int overwrite) {

  // FatFS.open("nodeFileSlot0.txt", "r");
  if (slot == -1) {
    // Create python_scripts directory if it doesn't exist
    if (!FatFS.exists("/python_scripts")) {
      if (FatFS.mkdir("/python_scripts")) {
        if (debugFP) {
          Serial.println("Created /python_scripts/ directory");
        }
      } else {
        if (debugFP) {
          Serial.println("Failed to create /python_scripts/ directory");
        }
      }
    }

    // Create empty history.txt file if it doesn't exist
    if (!FatFS.exists("/python_scripts/history.txt")) {
      File historyFile = FatFS.open("/python_scripts/history.txt", "w");
      if (historyFile) {
        historyFile.close();
        if (debugFP) {
          Serial.println("Created empty /history.txt file");
        }
      } else {
        if (debugFP) {
          Serial.println("Failed to create /history.txt file");
        }
      }
    }

    for (int i = 0; i < NUM_SLOTS; i++) {
      int index = 0;
      // while (core2busy == true) {
      //   // Serial.println("waiting for core2 to finish");
      // }
      // core1busy = true;s
      // nodeFile = FatFS.open("nodeFileSlot" + String(i) + ".txt", "w");
      if (overwrite == 1) {
        openFileThreadSafe(w, i);
      } else {
        if (FatFS.exists("nodeFileSlot" + String(i) + ".txt")) {
          continue;
        } else {
          openFileThreadSafe(w, i);
        }
      }
      nodeFile.print("{} ");

      nodeFile.close();
      core1busy = false;
      refreshPaths();
    }
  } else {

    openFileThreadSafe(w, slot);

    nodeFile.print("{} ");

    nodeFile.close();
    core1busy = false;
    refreshPaths();
  }
}

void createConfigFile(int overwrite) {

  if (overwrite == 0) {
    if (FatFS.exists("config.txt")) {
      return;
    }
  }
  while (core2busy == true) {
    // Serial.println("waiting for core2 to finish");
  }
  core1busy = true;

  File configFile = FatFS.open("config.txt", "w");
  configFile.print("#Jumperless Config file\n\r");
  configFile.println("version: 5");
  configFile.print("revision: ");
  configFile.println(EEPROM.read(REVISIONADDRESS));

  configFile.close();
}

int checkIfBridgeExists(int node1, int node2, int slot, int flashOrLocal) {

  return removeBridgeFromNodeFile(node1, node2, slot, flashOrLocal, 1);
  if (flashOrLocal == 0) {

    openFileThreadSafe(rplus, slot);

    if (!nodeFile) {
      if (debugFP) {
        // Serial.println("Failed to open nodeFile (removeBridgeFromNodeFile)");
      }

      return -1;
    } else {
      if (debugFP)
        Serial.println(
            "\n\ropened nodeFile.txt\n\n\rloading bridges from file\n\r");
    }

    if (nodeFile.size() < 2) {
      // Serial.println("empty file");
      nodeFile.close();
      core1busy = false;
      return -1;
    }
    nodeFile.seek(0);
    nodeFile.setTimeout(8);

    // nodeFile.readStringUntil
  }

  return 0;
}

void inputNodeFileList(int addRotaryConnections) {
  // addRotaryConnections = 1;
  // Serial.println("Paste the nodeFile list here\n\n\r");

  unsigned long humanTime = millis();

  int shown = 0;
  while (Serial.available() == 0) {
    if (millis() - humanTime == 400 && shown == 0) {
      Serial.println("Paste the nodeFile list here\n\n\r");
      shown = 1;
    }
  }
  nodeFileString.clear();

  int startInsertion = 0;

  nodeFileString.read(Serial);
  Serial.println("nodeFileString");

  nodeFileString.printTo(Serial);

  // Serial.println("\n\r\n\rnodeFileString");

  int lastTokenIndex = 0;
  int tokenFound = 0;
  uint8_t lastReads[8] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
  uint8_t slotText[8] = {'S', 'l', 'o', 't', ' '};
  uint8_t searchFor[3] = {'f', ' ', '{'};
  while (core2busy == true) {
    //   // Serial.println("waiting for core2 to finish");
  }
  core1busy = true;
  nodeFileBuffer = FatFS.open("nodeFileBuffer.txt", "w+");
  // openFileThreadSafe(wplus, slot);
  nodeFileString.trim();
  if (nodeFileString.endsWith("}") == 0) {
    nodeFileString.concat(" } \n\r");
  }
  // Validate the input first
  int validation_result = validateNodeFile(nodeFileString.c_str(), false);
  if (validation_result != 0) {
    Serial.println("◇ Input validation failed: " +
                   String(getNodeFileValidationError(validation_result)));
    Serial.println("◇ Correct format:");
    Serial.println("\n\rSlot [slot number] \n\n\rf "
                   "{ \n\r[node]-[node],\n\r[node]-[node],\n\r}\n\n\r");
    core1busy = false;
    return;
  }

  int openBraceIdx = nodeFileString.indexOf("{");

  if (openBraceIdx == -1) {
    Serial.println(
        "No opening curly braces found,\n\rhere is the correct format:");
    Serial.println("\n\n\rSlot [slot number] \n\n\rf "
                   "{ \n\r[node]-[node],\n\r[node]-[node],\n\r}\n\n\r");
    core1busy = false;
    return;
  }

  // Serial.println(nodeFileString);
  nodeFileString.printTo(nodeFileBuffer);

  int index = 0;

  int inFileMeat = 0;

  int numberOfSlotsFound = 0;
  int firstSlotNumber = 0;
  int firstSlotFound = 0;

  // nodeFileBuffer.seek(nodeFileBuffer.size());
  // nodeFileBuffer.print("fuck             \n\r");
  nodeFileBuffer.seek(0);
  // Serial.println(" \n\n\n\r");
  //  while (nodeFileBuffer.available())
  //  {
  //  Serial.write(nodeFileBuffer.read());
  //  }

  for (int i = 0; i < NUM_SLOTS;
       i++) // this just searches for how many slots are in the pasted list
  {
    tokenFound = 0;
    nodeFileBuffer.seek(index);
    inFileMeat = 0;

    while (nodeFileBuffer.available()) {
      uint8_t c = nodeFileBuffer.read();
      lastReads[0] = lastReads[1];
      lastReads[1] = lastReads[2];
      lastReads[2] = lastReads[3];
      lastReads[3] = lastReads[4];
      lastReads[4] = lastReads[5];
      lastReads[5] = c;

      if (lastReads[0] == slotText[0] && lastReads[1] == slotText[1] &&
          lastReads[2] == slotText[2] && lastReads[3] == slotText[3] &&
          lastReads[4] == slotText[4] && firstSlotFound == 0) {

        firstSlotFound = 1;
        firstSlotNumber = lastReads[5] - '0';

        // break;
      }

      if (lastReads[3] == searchFor[0] && lastReads[4] == searchFor[1] &&
          lastReads[5] == searchFor[2]) {
        inFileMeat = 1;
        numberOfSlotsFound++;
      }
      if (lastReads[2] == '}') {
        inFileMeat = 0;

        index++;
        break;
      }

      if (inFileMeat == 1) {

        // Serial.println(numberOfSlotsFound);
      }
      index++;
    }
  }

  index = 0;
  int lastSlotNumber = firstSlotNumber + numberOfSlotsFound - 1;

  for (int i = firstSlotNumber; i <= lastSlotNumber;
       i++) // this takes the pasted list fron the serial monitor and saves it
  // to the nodeFileSlot files
  {

    if (i >= firstSlotNumber && i <= lastSlotNumber) {
      // Serial.println(i);
      while (core2busy == true) {
        // Serial.println("waiting for core2 to finish");
      }
      core1busy = true;

      nodeFile = FatFS.open("nodeFileSlot" + String(i) + ".txt", "w");
    }

    // nodeFileStringSlot.clear();
    nodeFileBuffer.seek(index);

    inFileMeat = 0;

    while (nodeFileBuffer.available()) {
      uint8_t c = nodeFileBuffer.read();
      lastReads[0] = lastReads[1];
      lastReads[1] = lastReads[2];
      lastReads[2] = c;

      // nodeFile.write(c);

      if (lastReads[0] == searchFor[0] && lastReads[1] == searchFor[1] &&
          lastReads[2] == searchFor[2]) {
        inFileMeat = 1;
      }
      if (lastReads[1] == '}') {
        inFileMeat = 0;
        break;
      }

      if (inFileMeat == 1 && i >= firstSlotNumber && i <= lastSlotNumber) {
        nodeFile.write(c);
        // Serial.print(index);
      }
      index++;
    }
    if (i >= firstSlotNumber && i <= lastSlotNumber) {
      nodeFile.seek(0);
      nodeFile.close();
      markSlotAsModified(i); // Mark slot as needing re-validation
    }

    // refreshSavedColors(i);
    core1busy = false;
  }

  // Validate all saved slots
  if (debugFP) {
    Serial.println("◆ Validating saved slot files...");
  }
  for (int i = firstSlotNumber; i <= lastSlotNumber; i++) {
    int validation_result = validateNodeFileSlot(i, false);
    if (validation_result == 0) {
      if (debugFP) {
        Serial.println("◆ Slot " + String(i) + " validated successfully");
      }
    } else {
      if (debugFP) {
        Serial.println("◇ Slot " + String(i) + " validation failed: " +
                       String(getNodeFileValidationError(validation_result)));
      }
    }
  }
}

void saveCurrentSlotToSlot(int slotFrom, int slotTo, int flashOrLocalfrom,
                           int flashOrLocalTo) {
  // while (core2busy == true) {
  //   // Serial.println("waiting for core2 to finish");
  // }
  // core1busy = true;
  openFileThreadSafe(r, slotFrom);
  // nodeFile = FatFS.open("nodeFileSlot" + String(slotFrom) + ".txt", "r");
  nodeFileString.clear();
  nodeFileString.read(nodeFile);
  nodeFile.close();

  // nodeFile = FatFS.open("nodeFileSlot" + String(slotTo) + ".txt", "w");
  openFileThreadSafe(w, slotTo);
  nodeFileString.printTo(nodeFile);
  nodeFile.close();
  markSlotAsModified(slotTo); // Mark destination slot as needing re-validation
  core1busy = false;
  // refreshPaths();
}

void savePreformattedNodeFile(int source, int slot, int keepEncoder) {

  specialFunctionsString.clear();
  openFileThreadSafe(w, slot);
  // Serial.println("Slot " + String(slot));

  // if (debugFP) {
  //  Serial.print("source = ");
  //  Serial.println(source);
  //  Serial.flush();
  // }
  // char reads[20] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', '
  // ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '}; int readsIndex = 0;
  if (source == 0 || true) {
    int charCount = 0;

    //   uint8_t portMaskAvailable = Serial.availablePort();

    //   uint8_t existingSerialTarget = Serial.getSerialTarget();
    //   // Serial.println("portMaskAvailable");
    //   // Serial.println(portMaskAvailable, BIN);
    //   // Serial.flush();
    //  // Serial.setSerialTarget(portMaskAvailable);

    //   if (portMaskAvailable == SERIAL_PORT_SERIAL1){
    //     specialFunctionsString.print("116-70, 117-71, ");
    //   }

    if (serialCommandBufferIndex > 0) {
      specialFunctionsString.print("{ 116-70, 117-71, ");
      // specialFunctionsString.print((const char*)serialCommandBuffer);
      // //specialFunctionsString.print(", ");
      // specialFunctionsString.printTo(Serial);
      // Serial.println();
      // Serial.flush();
      serialCommandBufferIndex = 0;
    }

    specialFunctionsString.read(Serial);
    specialFunctionsString.trim();
    if (specialFunctionsString.endsWith(",") == 0) {
      specialFunctionsString.concat(",\n\r");
    }
    nodeFileString.clear();

    specialFunctionsString.printTo(nodeFileString);
    // nodeFileString.print(specialFunctionsString);
    // nodeFileString.printTo(Serial);
    // nodeFileString.concat(specialFunctionsString);
    removeBridgeFromNodeFile(116, -1, 0, 1, 1);
    specialFunctionsString.clear();
    // Serial.println("specialFunctionsString");
    // Serial.flush();
    nodeFileString.printTo(specialFunctionsString);
    // specialFunctionsString.printTo(Serial);
    // specialFunctionsString.concat(nodeFileString);

    // Serial.println("specialFunctionsString");
    // Serial.flush();
    // Serial.setSerialTarget(existingSerialTarget);
    // while (Serial.available() == 0) {
    // }
    // Serial.println("source = 0");
    // nodeFile.print("{");
    // if (jumperlessConfig.serial_1.lock_connection == 1) {
    //   nodeFile.print("116-70, 117-71, ");
    //   }
    // while (Serial.available() > 0) {
    //   // nodeFile.write(Serial.read());
    //   uint8_t c = Serial.read();
    //   // reads[readsIndex] = c;
    //   // readsIndex++;
    //   // if (readsIndex > 19) {
    //   //   readsIndex = 0;
    //   // }

    //   if (c != 'f' && c != '}' && c != '{' && c != ' ' && c != '\n' &&
    //       c != '\r' && c != '\t') {
    //        if (charCount == 0 && c == '-') {
    //          continue;

    //        }
    //     ///nodeFile.write(c);
    //     specialFunctionsString.write(c);
    //     charCount++;
    //     Serial.write(c);
    //   }

    // delayMicroseconds(microsPerByteSerial1);
    // }
  }
  // if (source == 1) {
  //   nodeFile.print("{117-71, 116-70,");
  //   while (Serial1.available() == 0) {
  //   }
  //   delayMicroseconds(microsPerByteSerial1);
  //   // Serial.println("waiting for Arduino to send file");
  //   while (Serial1.available() > 0) {

  //     char c = Serial1.read();
  //     // nodeFile.write(c);
  //     specialFunctionsString.write(c);
  //     delayMicroseconds(microsPerByteSerial1);
  //     // Serial.println(Serial1.available());
  //   }

  //   while (Serial1.available() > 0) {
  //     Serial1.read();
  //     delayMicroseconds(microsPerByteSerial1);
  //   }
  // }

  specialFunctionsString.trim();
  if (specialFunctionsString.endsWith(",") == 0) {
    specialFunctionsString.concat(",");
  }

  // Serial.println("\n\n\rbefore replaceSFNamesWithDefinedInts");
  // specialFunctionsString.printTo(Serial);

  replaceSFNamesWithDefinedInts();
  replaceNanoNamesWithDefinedInts();

  specialFunctionsString.printTo(nodeFile);

  nodeFile.print(" } ");

  nodeFile.close();
  markSlotAsModified(slot); // Mark slot as needing re-validation
  core1busy = false;
}

int getSlotLength(int slot, int flashOrLocal) {
  int slotLength = 0;
  if (flashOrLocal == 0) {
    while (core2busy == true) {
      // Serial.println("waiting for core2 to finish");
    }
    core1busy = true;
    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r");
    while (nodeFile.available()) {
      nodeFile.read();
      slotLength++;
    }
    nodeFile.close();
    core1busy = false;
  } else {
    slotLength = nodeFileString.length();
  }

  return slotLength;
}

void printNodeFile(int slot, int printOrString, int flashOrLocal,
                   int definesInts, bool printEmpty) {

  if (flashOrLocal == 0) {
    while (core2busy == true) {
      // Serial.println("waiting for core2 to finish");
    }
    core1busy = true;

    nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r");
    if (!nodeFile) {
      // if (debugFP)
      // Serial.println("Failed to open nodeFile");
      core1busy = false;
      return;
    } else {
      // if (debugFP)
      // Serial.println("\n\ropened nodeFile.txt\n\n\rloading bridges from
      // file\n\r");
    }
    specialFunctionsString.clear();

    specialFunctionsString.read(nodeFile);
    nodeFile.close();
    core1busy = false;
  } else {
    specialFunctionsString.clear();
    nodeFileString.printTo(specialFunctionsString);
    // specialFunctionsString.read(nodeFileString);
  }

  // Check if slot is empty and we don't want to print empty slots
  if (!printEmpty && isSlotFileEmpty(specialFunctionsString.c_str())) {
    return; // Don't print empty slots when printEmpty is false
  }

  //     int newLines = 0;
  // Serial.println(specialFunctionsString.indexOf(","));
  // Serial.println(specialFunctionsString.charAt(specialFunctionsString.indexOf(",")+1));
  // Serial.println(specialFunctionsString.indexOf(","));
  if (debugFP == 0 || definesInts == 0) {

    // specialFunctionsString.replace("116-80, 117-82, 114-83, 85-100, 81-100,",
    // "rotEnc_0,");

    specialFunctionsString.replace("100", "GND");
    specialFunctionsString.replace("101", "TOP_RAIL");
    specialFunctionsString.replace("102", "BOTTOM_RAIL");
    specialFunctionsString.replace("105", "5V");
    specialFunctionsString.replace("103", "3V3");
    specialFunctionsString.replace("106", "DAC0");
    specialFunctionsString.replace("107", "DAC1");
    specialFunctionsString.replace("108", "I_P");
    specialFunctionsString.replace("109", "I_N");
    specialFunctionsString.replace("110", "ADC0");
    specialFunctionsString.replace("111", "ADC1");
    specialFunctionsString.replace("112", "ADC2");
    specialFunctionsString.replace("113", "ADC3");
    specialFunctionsString.replace("114", "ADC4");
    specialFunctionsString.replace("115", "PROBE_MEASURE");
    specialFunctionsString.replace("116", "UART_TX");
    specialFunctionsString.replace("117", "UART_RX");
    specialFunctionsString.replace("118", "GPIO_18");
    specialFunctionsString.replace("119", "GPIO_19");
    specialFunctionsString.replace("120", "8V_P");
    specialFunctionsString.replace("121", "8V_N");
    specialFunctionsString.replace("70", "D0");
    specialFunctionsString.replace("71", "D1");
    specialFunctionsString.replace("72", "D2");
    specialFunctionsString.replace("73", "D3");
    specialFunctionsString.replace("74", "D4");
    specialFunctionsString.replace("75", "D5");
    specialFunctionsString.replace("76", "D6");
    specialFunctionsString.replace("77", "D7");
    specialFunctionsString.replace("78", "D8");
    specialFunctionsString.replace("79", "D9");
    specialFunctionsString.replace("80", "D10");
    specialFunctionsString.replace("81", "D11");
    specialFunctionsString.replace("82", "D12");
    specialFunctionsString.replace("83", "D13");
    specialFunctionsString.replace("84", "RESET");
    specialFunctionsString.replace("85", "AREF");
    specialFunctionsString.replace("86", "A0");
    specialFunctionsString.replace("87", "A1");
    specialFunctionsString.replace("88", "A2");
    specialFunctionsString.replace("89", "A3");
    specialFunctionsString.replace("90", "A4");
    specialFunctionsString.replace("91", "A5");
    specialFunctionsString.replace("92", "A6");
    specialFunctionsString.replace("93", "A7");
    specialFunctionsString.replace("94", "RESET_0");
    specialFunctionsString.replace("95", "RESET_1");
    // specialFunctionsString.replace("96", "GND_1");
    // specialFunctionsString.replace("97", "GND_0");
    // specialFunctionsString.replace("98", "3V3");
    // specialFunctionsString.replace("99", "5V");
    // specialFunctionsString.replace("128", "LOGO_PAD_TOP");
    // specialFunctionsString.replace("129", "LOGO_PAD_BOTTOM");
    // specialFunctionsString.replace("130", "GPIO_PAD");
    // specialFunctionsString.replace("131", "DAC_PAD");
    // specialFunctionsString.replace("132", "ADC_PAD");
    // specialFunctionsString.replace("133", "BUILDING_PAD_TOP");
    // specialFunctionsString.replace("134", "BUILDING_PAD_BOTTOM");
    // specialFunctionsString.replace("126", "BOTTOM_RAIL_GND");
    // specialFunctionsString.replace("104", "TOP_RAIL_GND");

    specialFunctionsString.replace("131", "GPIO_1");
    specialFunctionsString.replace("132", "GPIO_2");
    specialFunctionsString.replace("133", "GPIO_3");
    specialFunctionsString.replace("134", "GPIO_4");
    specialFunctionsString.replace("135", "GPIO_5");
    specialFunctionsString.replace("136", "GPIO_6");
    specialFunctionsString.replace("137", "GPIO_7");
    specialFunctionsString.replace("138", "GPIO_8");
    specialFunctionsString.replace("139", "BUFFER_IN");
    specialFunctionsString.replace("140", "BUFFER_OUT");
  }

  if (specialFunctionsString.charAt(specialFunctionsString.indexOf(",") + 1) !=
      '\n') {
    specialFunctionsString.replace(" ", "");
    specialFunctionsString.replace(",", ",\n\r");
    // specialFunctionsString.replace("{ ", "{\n\r");
    specialFunctionsString.replace("{", "{\n\r");
  }

  int specialFunctionsStringLength = specialFunctionsString.indexOf("}");
  if (specialFunctionsStringLength != -1) {
    if (specialFunctionsString.charAt(specialFunctionsStringLength + 1) !=
        '\n') {
      specialFunctionsString.replace("}", "}\n\r");
    }
    specialFunctionsString.remove(specialFunctionsStringLength + 2, -1);
  }

  // specialFunctionsString.readUntilToken(specialFunctionsString, "{");
  // specialFunctionsString.removeLast(9);
  // Serial.print("*");
  if (printOrString == 0) {
    Serial.println(specialFunctionsString);
    //     Serial.println('*');
    // specialFunctionsString.clear();
  }
}

// void parseWokwiFileToNodeFile(void) {

//   // delay(3000);
//   // FatFS.begin();
//   timeToFP = millis();

//   wokwiFile = FatFS.open("wokwi.txt", "w+");
//   if (!wokwiFile) {
//     if (debugFP)
//       Serial.println("Failed to open wokwi.txt");
//     return;
//   } else {
//     if (debugFP) {
//       Serial.println("\n\ropened wokwi.txt\n\r");
//     } else {
//       // Serial.println("\n\r");
//     }
//   }

//   Serial.println("paste Wokwi diagram.json here\n\r");
//   while (Serial.available() == 0) {
//   }

//   int numCharsRead = 0;

//   char firstChar = Serial.read();

//   if (firstChar != '{') // in case you just paste a wokwi file in from the menu,
//                         // the opening brace will have already been read
//   {
//     inputBuffer[numCharsRead] = '{';
//     numCharsRead++;
//   } else {
//     inputBuffer[numCharsRead] = firstChar;
//     numCharsRead++;
//   }
//   /*
//       Serial.println(firstChar);
//     Serial.println(firstChar);
//       Serial.println(firstChar);
//      Serial.println(firstChar);
//      Serial.println(firstChar);
//      Serial.print(firstChar);
//   */
//   delay(1);
//   while (Serial.available() > 0) {
//     char c = Serial.read();
//     inputBuffer[numCharsRead] = c;

//     numCharsRead++;

//     delayMicroseconds(1000);
//   }

//   createSafeStringFromCharArray(wokwiFileString, inputBuffer);
//   delay(3);
//   wokwiFile.write(inputBuffer, numCharsRead);

//   delay(3);

//   wokwiFile.seek(0);

//   if (debugFP)
//     Serial.println("\n\n\rwokwiFile\n\n\r");

//   /* for (int i = 0; i < numCharsRead; i++)
//    {
//        Serial.print((char)wokwiFile.read());
//    }*/
//   if (debugFP) {
//     Serial.print(wokwiFileString);

//     Serial.println("\n\n\rnumCharsRead = ");

//     Serial.print(numCharsRead);

//     Serial.println("\n\n\r");
//   }
//   wokwiFile.close();

//   deserializeJson(wokwiJson, inputBuffer);

//   if (debugFP) {

//     Serial.println("\n\n\rwokwiJson\n\n\r");

//     Serial.println("\n\n\rconnectionsW\n\n\r");
//   }

//   numConnsJson = wokwiJson["connections"].size();

//   copyArray(wokwiJson["connections"], connectionsW);

//   // deserializeJson(connectionsW, Serial);
//   if (debugFP) {
//     Serial.println(wokwiJson["connections"].size());

//     for (int i = 0; i < MAX_BRIDGES; i++) {
//       // Serial.println(wokwiJson["connections"].size());
//       if (connectionsW[i][0] == "") {
//         break;
//       }
//       Serial.print(connectionsW[i][0]);
//       Serial.print(",   \t ");

//       Serial.print(connectionsW[i][1]);
//       Serial.print(",   \t ");

//       Serial.print(connectionsW[i][2]);
//       Serial.print(",   \t ");

//       Serial.print(connectionsW[i][3]);
//       Serial.print(",   \t ");

//       Serial.print(connectionsW[i][4]);
//       Serial.print(",   \t ");

//       Serial.println();
//     }

//     Serial.println("\n\n\rRedefining\n\n\r");
//   }

//   changeWokwiDefinesToJumperless();

//   writeToNodeFile();
//   // while(1);
//   openNodeFile();
// }

// void changeWokwiDefinesToJumperless(void) {

//   String connString1 = "                               ";
//   String connString2 = "                               ";
//   String connStringColor = "                               ";
//   String bb = "bb1:";

//   int nodeNumber;

//   for (int i = 0; i < numConnsJson; i++) {
//     if (debugFP) {
//       Serial.println(' ');
//     }
//     for (int j = 0; j < 2; j++) {
//       nodeNumber = -1;
//       connString1 = connectionsW[i][j];
//       if (debugFP) {
//         Serial.print(connString1);
//         Serial.print("   \t\t  ");
//       }
//       if (connString1.startsWith("bb1:") || connString1.startsWith("bb2:")) {
//         // Serial.print("bb1 or bb2  ");

//         int periodIndex = connString1.indexOf(".");
//         connString1 = connString1.substring(4, periodIndex);

//         if (connString1.endsWith("b")) {
//           nodeNumber = 30;
//           // Serial.println("bottom");
//           connString1.substring(0, connString1.length() - 1);
//           nodeNumber += connString1.toInt();
//         } else if (connString1.endsWith("t")) {
//           nodeNumber = 0;
//           // Serial.println("top");
//           connString1.substring(0, connString1.length() - 1);
//           nodeNumber += connString1.toInt();
//         } else if (connString1.endsWith("n")) {
//           nodeNumber = GND;
//         } else if (connString1.endsWith("p")) {
//           nodeNumber = SUPPLY_5V;
//         }
//       } else if (connString1.startsWith("nano:")) {
//         // Serial.print("nano\t");
//         int periodIndex = connString1.indexOf(".");
//         connString1 = connString1.substring(5, periodIndex);

//         nodeNumber = NANO_D0;

//         if (isDigit(connString1[connString1.length() - 1])) {

//           nodeNumber += connString1.toInt();
//         } else if (connString1.equals("5V")) {
//           nodeNumber = SUPPLY_5V;
//         } else if (connString1.equalsIgnoreCase("AREF")) {

//           nodeNumber = NANO_AREF;
//         } else if (connString1.equalsIgnoreCase("GND")) {
//           nodeNumber = GND;
//         } else if (connString1.equalsIgnoreCase("RESET")) {

//           nodeNumber = NANO_RESET;
//         } else if (connString1.equalsIgnoreCase("3.3V")) {
//           nodeNumber = SUPPLY_3V3;
//         } else if (connString1.startsWith("A")) {
//           nodeNumber = NANO_A0;
//           nodeNumber += connString1.toInt();
//         }
//       } else if (connString1.startsWith("vcc1:")) {
//         // Serial.print("vcc1\t");
//         nodeNumber = SUPPLY_5V;
//       } else if (connString1.startsWith("vcc2:")) {
//         // Serial.print("vcc2\t");
//         nodeNumber = SUPPLY_3V3;
//       } else if (connString1.startsWith("gnd1:")) {
//         // Serial.print("gnd1\t");
//         nodeNumber = GND;
//       } else if (connString1.startsWith("gnd2:")) {
//         // Serial.print("gnd2\t");
//         nodeNumber = GND;
//       } else if (connString1.startsWith("gnd3:")) {
//         nodeNumber = GND;
//       } else if (connString1.startsWith("pot1:")) {
//         nodeNumber = DAC0;
//       } else {

//         connectionsW[i][j] = "-1";
//       }

//       // nodeNumber += connString1.toInt();
//       String nodeNumberString = String(nodeNumber);
//       connectionsW[i][j] = nodeNumberString;
//       // connectionsW[i][j] = nodeNumber;
//       if (debugFP) {
//         Serial.print(connectionsW[i][j]);

//         Serial.print("   \t ");
//       }
//     }
//   }
// }
void clearNodeFile(int slot, int flashOrLocal) {
  if (flashOrLocal == 0) {
    openFileThreadSafe(w, slot);
    // nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
    nodeFile.print("{");
    if (jumperlessConfig.serial_1.lock_connection == 1) {
      nodeFile.print("116-70, 117-71, ");
    }
    if (jumperlessConfig.top_oled.lock_connection == 1) {
      nodeFile.print(jumperlessConfig.top_oled.sda_row);
      nodeFile.print("-");
      nodeFile.print(jumperlessConfig.top_oled.gpio_sda);
      nodeFile.print(", ");
      nodeFile.print(jumperlessConfig.top_oled.scl_row);
      nodeFile.print("-");
      nodeFile.print(jumperlessConfig.top_oled.gpio_scl);
      nodeFile.print(", ");
    }
    nodeFile.print("} ");

    nodeFile.close();
    markSlotAsModified(slot); // Mark slot as needing re-validation

    clearChangedNetColors();
    removeNetColorFile(slot); // Remove the file and clear tracking bit
  } else {
    nodeFileString.clear();
    clearChangedNetColors();
    setSlotHasNetColors(slot, false); // Clear tracking bit for cache-only mode
    currentColorSlotColorsString.clear();
  }
  
}

String slicedLines[130];
int slicedLinesIndex = 0;

// Global variables for storing last removed nodes
int lastRemovedNodes[20] = {-1};
int lastRemovedNodesIndex = 0;
bool disconnectedNodeNewData = false;

int removeBridgeFromNodeFile(int node1, int node2, int slot, int flashOrLocal,
                             int onlyCheck) {
  // Reset the lastRemovedNodes buffer
  lastRemovedNodesIndex = 0;
  for (int i = 0; i < 20; i++) {
    lastRemovedNodes[i] = -1;
  }
  disconnectedNodeNewData = false;

  unsigned long timerStart = millis();
  unsigned long timerEnd[5] = {0, 0, 0, 0, 0};


  if (onlyCheck == 0) {
  for (int i = 0; i < 8; i++) { // idk if I should do this here but YOLO
    if (node1 == RP_GPIO_1 + i || node2 == RP_GPIO_1 + i) {
      if (gpioNet[i] != -2) {
        gpioNet[i] = -1;
      }
    }
  }
  }
  // Serial.print("Slot = ");
  // Serial.println(slot);
  if (flashOrLocal == 0) {

    openFileThreadSafe(rplus, slot);

    if (!nodeFile) {
      if (debugFP) {
        // Serial.println("Failed to open nodeFile (removeBridgeFromNodeFile)");
      }

      return -1;
    } else {
      if (debugFP)
        Serial.println(
            "\n\ropened nodeFile.txt\n\n\rloading bridges from file\n\r");
    }

    if (nodeFile.size() < 2) {
      // Serial.println("empty file");
      nodeFile.close();
      core1busy = false;
      return -1;
    }
    nodeFile.seek(0);
    nodeFile.setTimeout(8);
  }
  if (onlyCheck == 1) {
    // Serial.print("Checking for bridge between ");
    // Serial.print(node1);
    // Serial.print(" and ");
    // Serial.println(node2);
  }
  timerEnd[0] = millis() - timerStart;

  for (int i = 0; i < 120; i++) {
    slicedLines[i] = " ";
  }
  slicedLinesIndex = 0;
  int numberOfLines = 0;
  // nodeFileString.clear();
  String lineBufString = "";
  // nodeFileString.printTo(Serial);
  // Serial.println(" ");
  // Serial.print("nodeFileString = ");
  // Serial.println(nodeFileString);
  // core1busy = true;
  createSafeString(lineBufSafe, 40);
  int lineIdx = 0;
  int charIdx = 0;
  for (int i = 0; i < 120; i++) {
    slicedLines[lineIdx] = " ";

    if (flashOrLocal == 0) {
      lineBufString = nodeFile.readStringUntil(',');

    } else {
      charIdx = nodeFileString.stoken(lineBufSafe, charIdx, ",");
      lineBufString = lineBufSafe.c_str();
      if (charIdx == -1) {
        numberOfLines = lineIdx;
        // Serial.print ("\n\r\t\t\t\tt\t\tnumberOfLines = ");
        // Serial.println(numberOfLines);
        //  Serial.println("end of file char idx");

        break;
      }
    }
    // Serial.print("lineBufSafe = ");
    // Serial.println(lineBufSafe);
    // Serial.print("lineBufString = ");
    // Serial.println(lineBufString);

    lineBufString.trim();
    lineBufString.replace(" ", "");
    // Serial.print("$");
    // Serial.print(lineBufString);
    // Serial.println("$");

    if (lineBufString.length() < 3 || lineBufString == " ") {
      numberOfLines = lineIdx;
      // Serial.print ("numberOfLines = ");
      // Serial.println(numberOfLines);
      //  Serial.println("end of file");

      break;
    }
    slicedLines[lineIdx].concat(lineBufString);

    // Serial.print("#");
    // Serial.print(slicedLines[lineIdx]);
    // Serial.println("#");
    slicedLines[lineIdx].replace("\n", "");
    slicedLines[lineIdx].replace("\r", "");

    slicedLines[lineIdx].replace("{", "");
    slicedLines[lineIdx].replace("}", "");
    slicedLines[lineIdx].replace(",", "");
    // slicedLines[lineIdx].trim();
    slicedLines[lineIdx].replace("-", " - ");
    slicedLines[lineIdx].concat(" , ");
    // Serial.print("*");
    // Serial.print(slicedLines[lineIdx]);
    // Serial.println("*");

    lineIdx++;
  }
  timerEnd[1] = millis() - timerStart;
  numberOfLines = lineIdx;
  // Serial.print("numberOfLines = ");
  // Serial.println(numberOfLines);
  // Serial.print("nodeFileString = ");
  // Serial.println(nodeFileString);
  // nodeFileString.clear();
  // nodeFile.close();
  // Serial.println(nodeFileString);
  // Serial.print("lineIdx = ");
  // Serial.println(lineIdx);
  char nodeAsChar[40];
  itoa(node1, nodeAsChar, 10);
  String paddedNode1 = " ";
  String paddedNode2 = " ";

  paddedNode1.concat(nodeAsChar);
  paddedNode1.concat(" ");
  // Serial.print("paddedNode1 = *");
  // Serial.print(paddedNode1);
  // Serial.println("*");

  if (node2 != -1) {
    itoa(node2, nodeAsChar, 10);

    paddedNode2.concat(nodeAsChar);
    paddedNode2.concat(" ");
    // Serial.print("paddedNode2 = *");
    // Serial.print(paddedNode2);
    // Serial.println("*");
  }

  // nodeFile.truncate(0);

  timerEnd[2] = millis() - timerStart;

  if (flashOrLocal == 0) {
    nodeFile.close();
    openFileThreadSafe(w, slot);
    // nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
    nodeFile.print("{");
  } else {
    nodeFileString.clear();
    nodeFileString.concat("{ ");
  }
  // nodeFile.print(" { \n\r");
  // Serial.print("numberOfLines = ");
  // Serial.println(numberOfLines);
  int removedLines = 0;

  for (int i = 0; i < numberOfLines; i++) {
    // Serial.print("\n\rslicedLines[");

    // Serial.print(i);
    // Serial.print("] = ");
    // Serial.println(slicedLines[i]);
    // delay(10);
    // Serial.println(millis()-timerStart);
    int remove = 0;

    if (node2 == -1 && slicedLines[i].indexOf(paddedNode1) != -1) {
      if (onlyCheck == 0) {
        remove = 1;

        // Extract the other node in this connection
        String lineStr = slicedLines[i];
        lineStr.replace(" ", "");
        int dashPos = lineStr.indexOf("-");

        if (dashPos != -1) {
          String node1Str = lineStr.substring(0, dashPos);
          String node2Str = lineStr.substring(dashPos + 1);

          int nodeA = node1Str.toInt();
          int nodeB = node2Str.toInt();

          // Add the other node to our list
          int otherNode = (nodeA == node1) ? nodeB : nodeA;

          // Store in the buffer if we have space
          if (lastRemovedNodesIndex < 20 && otherNode > 0) {
            lastRemovedNodes[lastRemovedNodesIndex++] = otherNode;
            disconnectedNodeNewData = true;
          }
        }
      }

      removedLines++;
    } else if (slicedLines[i].indexOf(paddedNode1) != -1 &&
               slicedLines[i].indexOf(paddedNode2) != -1) {
      if (onlyCheck == 0) {
        remove = 1;

        // When removing a specific connection, add the other node
        if (lastRemovedNodesIndex < 20) {
          lastRemovedNodes[lastRemovedNodesIndex++] = node2;
          disconnectedNodeNewData = true;
        }
      }

      removedLines++;
    }
    if (remove == 0) {
      remove = 0;
      slicedLines[i].replace(" ", "");

      if (flashOrLocal == 0) {
        nodeFile.print(slicedLines[i]);
      } else {
        // nodeFileString.concat(slicedLines[i]);

        for (int j = 0; j < 39; j++) {

          nodeAsChar[j] = ' ';
        }
        // Serial.print("nodeAsChar1 = ");
        // Serial.println(nodeAsChar);

        slicedLines[i].toCharArray(nodeAsChar, 40);
        // Serial.print("nodeAsChar2 = *");
        // Serial.print(nodeAsChar);
        // Serial.println("*");
        slicedLines[i].replace(",", "");
        slicedLines[i].replace(" ", "");
        slicedLines[i].concat(",");
        nodeFileString.concat(nodeAsChar);
        nodeFileString.replace(" ", "");
        //     Serial.print("sliceLines[i].length() = ");
        //     Serial.println(slicedLines[i].length());
        //         Serial.print("nodeFileString = ");
        // Serial.println(nodeFileString);
        nodeFileString.replace("{ ", "");
        nodeFileString.replace(" } ", "");
        nodeFileString.replace("{", "");
        nodeFileString.replace("}", "");
        nodeFileString.prefix("{ ");
        // nodeFileString.concat(" } ");
        //  Serial.print("nodeFileString = ");
        //  Serial.println(nodeFileString);
      }
    }
    //     Serial.print("slicedLines[");
    //       Serial.print(i);
    //       Serial.print("] = ");
    //  Serial.println(slicedLines[i]);
  }

  if (flashOrLocal == 0) {
    nodeFile.print(" } ");
    nodeFile.close();
    if (onlyCheck == 0) { // Only mark as modified if we actually changed something
      markSlotAsModified(slot);
    }
  } else {
    nodeFileString.concat(" } ");
    // Serial.print("nodeFileString = ");
    // Serial.println(nodeFileString);
  }

  // for (int i = 0; i <= numberOfLines; i++) {
  //   Serial.print("\n\rslicedLines[");
  //   Serial.print(i);
  //   Serial.print("] = ");
  //   Serial.println(slicedLines[i]);

  // }
  core1busy = false;

  if (onlyCheck == 0) {
    removeChangedNetColors(node1, 1);
    if (node2 != -1) {
      removeChangedNetColors(node2, 0);
    }
  }

  timerEnd[3] = millis() - timerStart;
  // Serial.print("timerEnd[0] = ");

  //   Serial.println(timerEnd[0]);
  //   Serial.print("timerEnd[1] = ");
  //   Serial.println(timerEnd[1]);
  //   Serial.print("timerEnd[3] = ");
  //   Serial.println(timerEnd[3]);
  return removedLines;
}

int addBridgeToNodeFile(int node1, int node2, int slot, int flashOrLocal,
                        int allowDuplicates) {

  // Serial.print("nodeFileStringAdd = ");
  // Serial.println(nodeFileString);
  unsigned long timerStart[5];
  timerStart[0] = micros();
  if (flashOrLocal == 0) {
    // nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r+");

    openFileThreadSafe(rplus, slot);
    // Serial.println(nodeFile);
    // Serial.print("Slot = ");
    // Serial.println(slot);
    if (!nodeFile) {
      // if (debugFP) {
      // Serial.println("Failed to open nodeFile (addBridgeToNodeFile)");
      //  }
      // reateSlots(slot, 0);
      //  delay(10);
      //  nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt",
      //  "w+"); nodeFile.print("{ ");
      openFileThreadSafe(w, slot);
      nodeFile.print("{ } ");
      // return;
    } else {
      if (debugFP)
        Serial.println(
            "\n\ropened nodeFile.txt\n\n\rloading bridges from file\n\r");
    }
    //     while (nodeFile.available()) {
    //     Serial.write(nodeFile.read());
    //   }
    //   nodeFile.seek(0);
    nodeFile.setTimeout(15);
  }

  // Serial.print("flashOrLocal = ");
  // Serial.println(flashOrLocal);

  for (int i = 0; i < 120; i++) {
    slicedLines[i] = " ";
  }
  timerStart[1] = micros();

  int numberOfLines = 0;
  // Serial.print("nodeFileString = ");
  // Serial.println(nodeFileString);
  // nodeFileString.printTo(Serial);
  // Serial.println(" ");
  String lineBufString = "";

  createSafeString(lineBufSafe, 30);
  int lineIdx = 0;
  int charIdx = 0;
  nodeFileString.trim();

  for (int i = 0; i < 120; i++) {

    slicedLines[lineIdx] = " ";
    // if (i == 0 && nodeFileString.startsWith("{") == -1 && flashOrLocal == 1)
    // {
    //   slicedLines[0].concat("{");

    // }

    if (flashOrLocal == 0) {
      lineBufString = nodeFile.readStringUntil(',');

    } else {
      charIdx = nodeFileString.stoken(lineBufSafe, charIdx, ",");
      // if (charIdx == -1 || lineBufSafe == "}") {
      //   numberOfLines = lineIdx;
      //   break;
      // }
      lineBufString = lineBufSafe.c_str();
    }

    // Serial.print("lineBufSafe = ");
    // Serial.println(lineBufSafe);
    // Serial.print("lineBufString = ");
    // Serial.println(lineBufString);

    lineBufString.trim();
    lineBufString.replace(" ", "");

    if (lineBufString.length() < 3 || lineBufString == " ") {
      // Serial.println("end of file");
      numberOfLines = lineIdx;
      break;
    }
    slicedLines[lineIdx].concat(lineBufString);

    slicedLines[lineIdx].replace("\n", "");
    slicedLines[lineIdx].replace("\r", "");

    slicedLines[lineIdx].replace("{", "");
    slicedLines[lineIdx].replace("}", "");
    slicedLines[lineIdx].replace(",", "");
    slicedLines[lineIdx].concat(",");

    // Serial.print("lineBufString = ");
    // Serial.println(lineBufString);
    // slicedLines[lineIdx].trim();

    // Serial.print("*");
    // Serial.print(slicedLines[lineIdx]);
    // Serial.println("*");

    lineIdx++;
  }
  timerStart[2] = micros();
  numberOfLines = lineIdx;

  // Serial.print("\n\rnumberOfLines = ");
  // Serial.println(numberOfLines);

  // nodeFileString.clear();
  // nodeFile.close();
  // Serial.println(nodeFileString);

  char nodeAsChar[40];
  itoa(node1, nodeAsChar, 10);
  String addNode1 = "";
  String addNode2 = "";

  addNode1.concat(nodeAsChar);
  addNode1.concat("-");

  itoa(node2, nodeAsChar, 10);

  addNode2.concat(nodeAsChar);
  addNode2.concat(",");

  addNode1.concat(addNode2);

  addNode1.toCharArray(nodeAsChar, addNode1.length() + 1);
  nodeAsChar[addNode1.length()] = '\0';

  // createSafeString(addNodeSafe, 40);

  int duplicateFound = 0;

  if (flashOrLocal == 0) {
    // Serial.println("flash");
    openFileThreadSafe(wplus, slot);
    // nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w+");
    nodeFile.print("{ ");

    for (int i = 0; i < numberOfLines; i++) {

      if (slicedLines[i].indexOf(addNode1) != -1) {
        duplicateFound = 1;
        // Serial.println("Duplicate found (flash)");
      }

      nodeFile.print(slicedLines[i]);
    }

    if (duplicateFound == 0 || allowDuplicates == 1) {
      nodeFile.print(addNode1);
    }
    nodeFile.print(" } ");
    // nodeFile.seek(0);

    nodeFile.close();
    markSlotAsModified(slot); // Mark slot as needing re-validation

  } else {
    //  Serial.println("local");
    // Serial.print("nodeFileString1 = ");
    // Serial.println(nodeFileString);
    // Serial.print("addNode1 = ");
    // Serial.println(addNode1);

    if (nodeFileString.indexOf(nodeAsChar, 0) != -1) {
      duplicateFound = 1;
      // Serial.println("Duplicate found (local)");
    }

    nodeFileString.replace('\n', "");
    nodeFileString.replace('\r', "");
    nodeFileString.replace(' ', "");
    nodeFileString.replace('{', "");
    nodeFileString.replace('}', "");

    // Serial.print("\n\n\rduplicateFound = ");
    //     Serial.println(duplicateFound);
    //     Serial.print("nodeAsChar = ");
    //     Serial.println(nodeAsChar);

    if (duplicateFound == 0 || allowDuplicates == 1) {
      // nodeFileString.concat(addNode1);
      nodeFileString.concat(nodeAsChar, addNode1.length());
    }

    nodeFileString.prefix("{ ");
    nodeFileString.concat(" } ");

    //     Serial.print("nodeFileStringAdd = ");
    // Serial.println(nodeFileString);
  }
  timerStart[3] = micros();

  // for (int i = 0; i < 4; i++) {
  //   Serial.print("timerStart[");
  //   Serial.print(i);
  //   Serial.print("] = ");
  //   Serial.println(timerStart[i] - timerStart[0]);
  // }
  return duplicateFound;
}

createSafeString(serialString, 100);
createSafeString(dash, 2);

createSafeString(comma, 2);


void readStringFromSerial(int source, int addRemove) {

  int node1 = 0;
  int node2 = 0;
  int numberOfBridges = 0;
  int finished = 1;
  int singleNode = 0;

  specialFunctionsString.clear();
  serialString.clear();
  // dash.clear();
  // comma.clear();
  // dash.concat("-");
  // comma.concat(",");
  if (source == 0) {
    specialFunctionsString.read(Serial);
  } else if (source == 1) {
   // specialFunctionsString.read(Serial1);
  }

  if (specialFunctionsString.endsWith("-") == 1 ||
      specialFunctionsString.endsWith(";") == 1 ||
      specialFunctionsString.endsWith("}") == 1 ||
      specialFunctionsString.endsWith("[") == 1) {
    specialFunctionsString.removeLast(1);
  }

  if (specialFunctionsString.startsWith("{") == 1 ||
      specialFunctionsString.startsWith("f") == 1 ||
      specialFunctionsString.startsWith("-") == 1 ||
      specialFunctionsString.startsWith("[") == 1) {
    specialFunctionsString.removeBefore(1);
  }

  replaceSFNamesWithDefinedInts();
  replaceNanoNamesWithDefinedInts();

  do {
    // nodeFileString.clear();
    // nodeFileString.read(Serial);
    int dashIndex = specialFunctionsString.indexOf("-");
    if (dashIndex == -1 && addRemove == 0) {
      Serial.println("Invalid input");
      return;
    } else if (dashIndex == -1 && addRemove == 1) {
      singleNode = 1;
      node2 = -1;
      finished = 1;
      dashIndex = specialFunctionsString.length();
    }

    specialFunctionsString.substring(serialString, 0, dashIndex);
    // serialString.readUntil(specialFunctionsString, "-");
    // serialString.removeLast(1);
    serialString.toInt(node1);
    // serialString.printTo(Serial);
    // Serial.println();
    // nodeFileString.printTo(Serial);
    //  Serial.println();
    // nodeFileString.clear();
    serialString.clear();

    if (singleNode != 1) {
      int commaIndex = specialFunctionsString.indexOf(",");
      if (commaIndex != -1) {
        specialFunctionsString.substring(serialString, dashIndex + 1,
                                         commaIndex);
      } else {
        commaIndex = specialFunctionsString.length();
        specialFunctionsString.substring(serialString, dashIndex + 1,
                                         commaIndex);
      }

      // specialFunctionsString.printTo(Serial);
      // Serial.println();

      if (specialFunctionsString.indexOfCharFrom("-", dashIndex + 1) != -1) {
        specialFunctionsString.removeBefore(commaIndex + 1);
        // specialFunctionsString.substring(specialFunctionsString, commaIndex +
        // 1, specialFunctionsString.length());
        finished = 0;
      } else {
        finished = 1;
      }

      // specialFunctionsString.printTo(Serial);
      // Serial.println();

      serialString.toInt(node2);
      // serialString.printTo(Serial);
      // Serial.println();
      // Serial.print("node1 = ");
      // Serial.println(node1);
      Serial.print("node2 = ");
      Serial.println(node2);
    }

    if (isNodeValid(node1) != 1) {
      Serial.println("Invalid node 1 number");
      return;
    }

    if (isNodeValid(node2) != 1 && (addRemove == 0)) {
      Serial.println("Invalid node 2 number");
      return;
    }

    if (addRemove == 0) {
      addBridgeToNodeFile(node1, node2, netSlot, 0, 1);
    } else if (addRemove == 1) {
      if (node1 == node2 || isNodeValid(node2) == 0) {
        node2 = -1;
      }

      removeBridgeFromNodeFile(node1, node2, netSlot, 0, 0);
    }

  } while (finished == 0);
  printNodeFile(netSlot, 0, 0, 0, true);
}

int parseStringToNode(int source) { return 0; }

int isNodeValid(int node) {
  if (node == -1) {
    return -1;
  } else if (node >= 1 && node <= 60) {
    return 1;
  } else if (node >= 70 && node <= 93) {
    return 1;
  } else if (node >= 100 && node <= 117) {
    return 1;
  } else if (node >= RP_GPIO_1 &&
             node <= ROUTABLE_BUFFER_OUT) { // Extended range to include 126-134
    return 1;
  } else {
    return 0;
  }
}

// Lightning-fast validation using character parsing instead of String operations
int validateNodeFileFast(const char* content, int contentLen, bool verbose) {
  if (contentLen < 4) {
    if (verbose) Serial.println("◇ Content too short");
    return 1;
  }

  // Find braces using simple character scanning
  int openBrace = -1, closeBrace = -1;
  for (int i = 0; i < contentLen; i++) {
    if (content[i] == '{' && openBrace == -1) openBrace = i;
    else if (content[i] == '}') closeBrace = i;
  }

  if (openBrace == -1) {
    if (verbose) Serial.println("◇ Missing opening brace");
    return 2;
  }
  if (closeBrace == -1 || closeBrace <= openBrace) {
    if (verbose) Serial.println("◇ Missing/invalid closing brace");
    return 3;
  }

  // Fast validation: check basic structure only
  int connectionCount = 0;
  bool inNumber = false;
  bool foundDash = false;
  int node1 = 0, node2 = 0;
  bool valid = true;

  for (int i = openBrace + 1; i < closeBrace; i++) {
    char c = content[i];
    
    if (c >= '0' && c <= '9') {
      if (!inNumber) {
        inNumber = true;
        if (!foundDash) node1 = node1 * 10 + (c - '0');
        else node2 = node2 * 10 + (c - '0');
      } else {
        if (!foundDash) node1 = node1 * 10 + (c - '0');
        else node2 = node2 * 10 + (c - '0');
      }
    } else if (c == '-') {
      if (!inNumber || foundDash) {
        valid = false;
        break;
      }
      foundDash = true;
      inNumber = false;
    } else if (c == ',' || c == ' ' || c == '\n' || c == '\r' || c == '\t') {
      if (foundDash && inNumber) {
        // End of a connection - quick validate
        if (isNodeValid(node1) != 1 || isNodeValid(node2) != 1) {
          valid = false;
          break;
        }
        connectionCount++;
        node1 = node2 = 0;
        foundDash = false;
      }
      inNumber = false;
    } else if (c != ' ') {
      // Invalid character
      valid = false;
      break;
    }
  }

  // Handle last connection if no trailing comma
  if (valid && foundDash && inNumber) {
    if (isNodeValid(node1) == 1 && isNodeValid(node2) == 1) {
      connectionCount++;
    } else {
      valid = false;
    }
  }

  if (verbose) {
    Serial.print("◆ Fast validation: ");
    Serial.print(connectionCount);
    Serial.print(" connections, ");
    Serial.println(valid ? "VALID" : "INVALID");
  }

  return valid ? 0 : 5;
}

// Keep the old validation for comparison/fallback if needed
int validateNodeFile(const String &content, bool verbose) {
  // Use fast validation instead of slow String operations
  return validateNodeFileFast(content.c_str(), content.length(), verbose);
}

int validateNodeFileSlot(int slot, bool verbose) {
  // Fast validation using direct file reading instead of String operations
  String filename = "nodeFileSlot" + String(slot) + ".txt";

  if (verbose) {
    Serial.println("◆ Fast validating " + filename + "...");
  }

  if (!FatFS.exists(filename)) {
    if (verbose)
      Serial.println("◇ Slot file does not exist");
    return 1; // File doesn't exist, treat as empty
  }

  File slotFile = FatFS.open(filename, "r");
  if (!slotFile) {
    if (verbose)
      Serial.println("◇ Failed to open slot file");
    return 1;
  }

  // Read file efficiently into a small buffer
  size_t fileSize = slotFile.size();
  if (fileSize > 512) {
    // File too large, probably corrupted
    slotFile.close();
    if (verbose) Serial.println("◇ File too large");
    return 5;
  }

  char buffer[513]; // Small stack buffer
  size_t bytesRead = slotFile.readBytes(buffer, min(fileSize, 512));
  buffer[bytesRead] = '\0'; // Null terminate
  slotFile.close();

  return validateNodeFileFast(buffer, bytesRead, verbose);
}

const char *getNodeFileValidationError(int errorCode) {
  switch (errorCode) {
  case 0:
    return "Valid";
  case 1:
    return "Empty or missing content";
  case 2:
    return "Missing opening brace '{'";
  case 3:
    return "Missing closing brace '}'";
  case 4:
    return "Invalid node number found";
  case 5:
    return "Malformed connection format";
  case 6:
    return "Invalid connection format";
  default:
    return "Unknown error";
  }
}

bool isSlotFileEmpty(const String &content) {
  // Check if the content is just empty braces after stripping whitespace
  String trimmed = content;
  trimmed.trim();
  trimmed.replace(" ", "");
  trimmed.replace("\n", "");
  trimmed.replace("\r", "");
  trimmed.replace("\t", "");

  return (trimmed == "{}" || trimmed == "{ }" || trimmed.length() < 4);
}

bool isSlotFileEmpty(int slot) {
  String content = readSlotFileContent(slot);
  return isSlotFileEmpty(content);
}

void writeToNodeFile(int slot, int flashOrLocal) {

  /// core1busy = true;

  // nodeFileString.printTo(Serial);
  // nodeFile = FatFS.open("nodeFile" + String(slot) + ".txt", "w");
  openFileThreadSafe(w, slot);

  if (!nodeFile) {
    if (debugFP)
      Serial.println("Failed to open nodeFile");
    return;
  } else {
    if (debugFP)
      Serial.println("\n\rrecreated nodeFile.txt\n\n\rloading bridges from "
                     "wokwi.txt\n\r");
  }
  nodeFile.print("{\n\r");
  for (int i = 0; i < numConnsJson; i++) {
    if (connectionsW[i][0] == "-1" && connectionsW[i][1] != "-1") {
      // lightUpNode(connectionsW[i][0].toInt());
      continue;
    }
    if (connectionsW[i][1] == "-1" && connectionsW[i][0] != "-1") {
      // lightUpNode(connectionsW[i][1].toInt());
      continue;
    }
    if (connectionsW[i][0] == connectionsW[i][1]) {
      // lightUpNode(connectionsW[i][0].toInt());
      continue;
    }

    nodeFile.print(connectionsW[i][0]);
    nodeFile.print("-");
    nodeFile.print(connectionsW[i][1]);
    nodeFile.print(",\n\r");
  }
  nodeFile.print("\n\r}\n\r");

  if (debugFP) {
    Serial.println("wrote to nodeFile.txt");

    Serial.println("nodeFile.txt contents:");
    nodeFile.seek(0);

    while (nodeFile.available()) {
      Serial.write(nodeFile.read());
    }
    Serial.println("\n\r");
  }
  nodeFile.close();
  core1busy = false;
}

void openNodeFile(int slot, int flashOrLocal) {
  timeToFP = millis();
  netsUpdated = false;

  // Ultra-fast validation check: only validate if not already validated (only for flash files)
  if (flashOrLocal == 0) {
    if (!slotIsValidated(slot)) {
      if (debugFP) {
        Serial.println("◆ Ultra-fast validating nodeFileSlot" + String(slot) + ".txt...");
      }
      
      // Do minimal validation - just check if file exists and has basic structure
      if (FatFS.exists("nodeFileSlot" + String(slot) + ".txt")) {
        File quickCheck = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r");
        if (quickCheck) {
          size_t fileSize = quickCheck.size();
          
          // For very small files, check everything
          if (fileSize <= 64) {
            bool hasOpenBrace = false, hasCloseBrace = false;
            char buffer[65];
            int bytesRead = quickCheck.readBytes(buffer, fileSize);
            buffer[bytesRead] = '\0';
            
            for (int i = 0; i < bytesRead; i++) {
              if (buffer[i] == '{') hasOpenBrace = true;
              if (buffer[i] == '}') hasCloseBrace = true;
            }
            
            // Only fix if actually missing braces in small files
            if (!hasOpenBrace || !hasCloseBrace) {
              quickCheck.close();
              if (debugFP) {
                Serial.println("◇ Small file missing braces, fixing");
              }
              File fixFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
              if (fixFile) {
                fixFile.print("{ }");
                fixFile.close();
              }
            } else {
              quickCheck.close();
            }
          } else {
            // For larger files, check beginning and end for braces
            bool hasOpenBrace = false, hasCloseBrace = false;
            char startBuffer[32], endBuffer[32];
            
            // Check beginning for opening brace
            int startRead = quickCheck.readBytes(startBuffer, 31);
            for (int i = 0; i < startRead; i++) {
              if (startBuffer[i] == '{') {
                hasOpenBrace = true;
                break;
              }
            }
            
            // Check end for closing brace  
            if (fileSize > 31) {
              quickCheck.seek(fileSize - 31);
              int endRead = quickCheck.readBytes(endBuffer, 31);
              for (int i = 0; i < endRead; i++) {
                if (endBuffer[i] == '}') {
                  hasCloseBrace = true;
                  break;
                }
              }
            }
            
            quickCheck.close();
            
            // For larger files, assume they're probably OK even if we don't find braces
            // (they might be in the middle section we didn't read)
            if (debugFP && (!hasOpenBrace || !hasCloseBrace)) {
              Serial.println("◇ Large file, braces not found in start/end - assuming OK");
            }
          }
        }
      } else {
        // File doesn't exist, create empty one
        if (debugFP) {
          Serial.println("◇ File doesn't exist, creating empty file");
        }
        File createFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
        if (createFile) {
          createFile.print("{ }");
          createFile.close();
        }
      }
      
      // Mark as validated after our quick check
      setSlotValidated(slot, true);
      if (debugFP) {
        Serial.println("◆ Slot " + String(slot) + " ultra-fast validated and cached");
      }
    } else {
      if (debugFP) {
        Serial.println("◆ Slot " + String(slot) + " already validated (skipping)");
      }
    }
  }

  // Serial.println(nodeFileString);
  // Serial.println("opening nodeFileSlot" + String(slot) + ".txt");
  // Serial.println("flashOrLocal = " + String(flashOrLocal));

  // Serial.println(flashOrLocal?"local":"flash");

  // while (core2busy == true) {
  //   // Serial.println("waiting for core2 to finish");
  // }
  // core1busy = true;
  if ((nodeFileString.length() < 3 && flashOrLocal == 1) || flashOrLocal == 0) {

    // if (flashOrLocal == 0) {
    // multicore_lockout_start_blocking();
    // Serial.println("opening nodeFileSlot" + String(slot) + ".txt");

    // nodeFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "r");
    openFileThreadSafe(r, slot);
    
    if (!nodeFile) {
      if (debugFP)
        Serial.println("Failed to open nodeFile");

      // createSlots(slot, 0);
      openFileThreadSafe(w, slot);
      nodeFile.print("{ } ");
      // core1busy = false;
      // return;
    } else {
      if (debugFP)
        Serial.println("\n\ropened nodeFileSlot" + String(slot) +
                       +".txt\n\n\rloading bridges from file\n\r");
    }

    nodeFileString.clear();
    nodeFileString.read(nodeFile);
    // delay(10);
    // Serial.println(nodeFileString);

    nodeFile.close();

    // multicore_lockout_end_blocking();
  }

  // Additional validation of the loaded content before parsing
  // Only validate if this slot hasn't been validated yet (skip if already validated)
  if (flashOrLocal == 0 && !slotIsValidated(slot)) {
    int validation_result = validateNodeFile(nodeFileString.c_str(), false);
    if (validation_result != 0) {
      if (debugFP) {
        Serial.println("◇ Loaded content failed validation, clearing and using "
                       "empty file");
      }
      nodeFileString.clear();
      nodeFileString.concat("{ }");
      clearNodeFile(slot, 0);
    }
  }

  // Serial.println(nodeFileString);
  // Serial.println();
  //   Serial.println("nodeFileString = ");
  // nodeFileString.printTo(Serial);

  splitStringToFields();
  
  core1busy = false;
  // parseStringToBridges();
}

void splitStringToFields() {
  int openBraceIndex = 0;
  int closeBraceIndex = 0;

  if (debugFP)
    Serial.println("\n\rraw input file\n\r");
  if (debugFP)
    Serial.println(nodeFileString);
  if (debugFP)
    Serial.println("\n\rsplitting and cleaning up string\n\r");
  if (debugFP)
    Serial.println("_");

  openBraceIndex = nodeFileString.indexOf("{");
  closeBraceIndex = nodeFileString.indexOf("}");
  int fIndex = nodeFileString.indexOf("f");

  int foundOpenBrace = -1;
  int foundCloseBrace = -1;
  int foundF = -1;

  if (openBraceIndex != -1) {
    foundOpenBrace = 1;
  }
  if (closeBraceIndex != -1) {
    foundCloseBrace = 1;
  }
  if (fIndex != -1) {
    foundF = 1;
  }

  // Serial.println(openBraceIndex);
  // Serial.println(closeBraceIndex);
  // Serial.println(fIndex);

  if (foundF == 1) {
    nodeFileString.substring(nodeFileString, fIndex + 1,
                             nodeFileString.length());
  }

  if (foundOpenBrace == 1 && foundCloseBrace == 1) {

    nodeFileString.substring(specialFunctionsString, openBraceIndex + 1,
                             closeBraceIndex);
  } else {
    nodeFileString.substring(specialFunctionsString, 0,
                             -1); // nodeFileString.length());
  }
  specialFunctionsString.trim();

  if (debugFP)
    Serial.println(specialFunctionsString);

  if (debugFP)
    Serial.println("^\n\r");
  /*
      nodeFileString.remove(0, closeBraceIndex + 1);
      nodeFileString.trim();

      openBraceIndex = nodeFileString.indexOf("{");
      closeBraceIndex = nodeFileString.indexOf("}");
      //nodeFileString.substring(specialFunctionsString, openBraceIndex + 1,
     closeBraceIndex); specialFunctionsString.trim();
      if(debugFP)Serial.println("_");
      if(debugFP)Serial.println(specialFunctionsString);
      if(debugFP)Serial.println("^\n\r");
      */
  replaceSFNamesWithDefinedInts();
  replaceNanoNamesWithDefinedInts();
  parseStringToBridges();
}

void replaceSFNamesWithDefinedInts(void) {
  specialFunctionsString.toUpperCase();
  if (debugFP) {
    Serial.println("replacing special function names with defined ints\n\r");
    Serial.println(specialFunctionsString);
  }

  specialFunctionsString.replace("GND", "100");
  specialFunctionsString.replace("GROUND", "100");
  specialFunctionsString.replace("TOP_RAIL", "101");
  specialFunctionsString.replace("TOPRAIL", "101");
  specialFunctionsString.replace("T_R", "101");
  specialFunctionsString.replace("TOP_R", "101");
  specialFunctionsString.replace("BOTTOM_RAIL", "102");
  specialFunctionsString.replace("BOT_RAIL", "102");
  specialFunctionsString.replace("BOTTOMRAIL", "102");
  specialFunctionsString.replace("BOTRAIL", "102");
  specialFunctionsString.replace("B_R", "102");
  specialFunctionsString.replace("BOT_R", "102");

  specialFunctionsString.replace("SUPPLY_5V", "105");
  specialFunctionsString.replace("SUPPLY_3V3", "103");

  specialFunctionsString.replace("DAC0_5V", "106");
  specialFunctionsString.replace("DAC1_8V", "107");
  specialFunctionsString.replace("DAC0", "106");
  specialFunctionsString.replace("DAC1", "107");
  specialFunctionsString.replace("DAC_0", "106");
  specialFunctionsString.replace("DAC_1", "107");

  specialFunctionsString.replace("INA_N", "109");
  specialFunctionsString.replace("INA_P", "108");
  specialFunctionsString.replace("I_N", "109");
  specialFunctionsString.replace("I_P", "108");
  specialFunctionsString.replace("CURRENT_SENSE_MINUS", "109");
  specialFunctionsString.replace("CURRENT_SENSE_PLUS", "108");
  specialFunctionsString.replace("ISENSE_MINUS", "109");

  specialFunctionsString.replace("ISENSE_PLUS", "108");

  specialFunctionsString.replace("ISENSE_NEGATIVE", "109");
  specialFunctionsString.replace("ISENSE_POSITIVE", "108");
  specialFunctionsString.replace("ISENSE_POS", "108");
  specialFunctionsString.replace("ISENSE_NEG", "109");
  specialFunctionsString.replace("ISENSE_N", "109");
  specialFunctionsString.replace("ISENSE_P", "108");

  specialFunctionsString.replace("BUFFER_IN", "139");
  specialFunctionsString.replace("BUFFER_OUT", "140");
  specialFunctionsString.replace("BUF_IN", "139");
  specialFunctionsString.replace("BUF_OUT", "140");
  specialFunctionsString.replace("BUFF_IN", "139");
  specialFunctionsString.replace("BUFF_OUT", "140");
  specialFunctionsString.replace("BUFFIN", "139");
  specialFunctionsString.replace("BUFFOUT", "140");

  specialFunctionsString.replace("EMPTY_NET", "127");

  specialFunctionsString.replace("ADC0_8V", "110");
  specialFunctionsString.replace("ADC1_8V", "111");
  specialFunctionsString.replace("ADC2_8V", "112");
  specialFunctionsString.replace("ADC3_8V", "113");
  specialFunctionsString.replace("ADC4_5V", "114");
  specialFunctionsString.replace("PROBE_MEASURE", "115");
  specialFunctionsString.replace("115", "139");
  specialFunctionsString.replace("ADC0", "110");
  specialFunctionsString.replace("ADC1", "111");
  specialFunctionsString.replace("ADC2", "112");
  specialFunctionsString.replace("ADC3", "113");
  specialFunctionsString.replace("ADC4", "114");
  specialFunctionsString.replace("PROBE_MEASURE", "139");

  specialFunctionsString.replace("ADC_0", "110");
  specialFunctionsString.replace("ADC_1", "111");
  specialFunctionsString.replace("ADC_2", "112");
  specialFunctionsString.replace("ADC_3", "113");
  specialFunctionsString.replace("ADC_4", "114");
  specialFunctionsString.replace("ADC_7", "115");

  specialFunctionsString.replace("GPIO_1", "131");
  specialFunctionsString.replace("GPIO_2", "132");
  specialFunctionsString.replace("GPIO_3", "133");
  specialFunctionsString.replace("GPIO_4", "134");

  specialFunctionsString.replace("GPIO1", "131");
  specialFunctionsString.replace("GPIO2", "132");
  specialFunctionsString.replace("GPIO3", "133");
  specialFunctionsString.replace("GPIO4", "134");

  specialFunctionsString.replace("GPIO_5", "135");
  specialFunctionsString.replace("GPIO_6", "136");
  specialFunctionsString.replace("GPIO_7", "137");
  specialFunctionsString.replace("GPIO_8", "138");

  specialFunctionsString.replace("GPIO5", "135");
  specialFunctionsString.replace("GPIO6", "136");
  specialFunctionsString.replace("GPIO7", "137");
  specialFunctionsString.replace("GPIO8", "138");

  specialFunctionsString.replace("GP_1", "131");
  specialFunctionsString.replace("GP_2", "132");
  specialFunctionsString.replace("GP_3", "133");
  specialFunctionsString.replace("GP_4", "134");
  specialFunctionsString.replace("GP_5", "135");
  specialFunctionsString.replace("GP_6", "136");
  specialFunctionsString.replace("GP_7", "137");
  specialFunctionsString.replace("GP_8", "138");

  specialFunctionsString.replace("GP1", "131");
  specialFunctionsString.replace("GP2", "132");
  specialFunctionsString.replace("GP3", "133");
  specialFunctionsString.replace("GP4", "134");
  specialFunctionsString.replace("GP5", "135");
  specialFunctionsString.replace("GP6", "136");
  specialFunctionsString.replace("GP7", "137");
  specialFunctionsString.replace("GP8", "138");

  specialFunctionsString.replace("+5V", "105");
  specialFunctionsString.replace("5V", "105");
  specialFunctionsString.replace("3.3V", "103");
  specialFunctionsString.replace("3V3", "103");

  specialFunctionsString.replace("RP_UART_TX", "116");
  specialFunctionsString.replace("RP_UART_RX", "117");

  specialFunctionsString.replace("UART_TX", "116");
  specialFunctionsString.replace("UART_RX", "117");

  specialFunctionsString.replace("TX", "116");
  specialFunctionsString.replace("RX", "117");
}

void replaceNanoNamesWithDefinedInts(
    void) // for dome reason Arduino's String wasn't replacing like 1 or 2 of
// the names, so I'm using SafeString now and it works
{
  if (debugFP)
    Serial.println("replacing special function names with defined ints\n\r");

  char nanoName[10];
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }

  itoa(NANO_D10, nanoName, 10);
  specialFunctionsString.replace("D10", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }

  itoa(NANO_D11, nanoName, 10);
  specialFunctionsString.replace("D11", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D12, nanoName, 10);
  specialFunctionsString.replace("D12", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D13, nanoName, 10);
  specialFunctionsString.replace("D13", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D0, nanoName, 10);
  specialFunctionsString.replace("D0", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D1, nanoName, 10);
  specialFunctionsString.replace("D1", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D2, nanoName, 10);
  specialFunctionsString.replace("D2", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D3, nanoName, 10);
  specialFunctionsString.replace("D3", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D4, nanoName, 10);
  specialFunctionsString.replace("D4", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D5, nanoName, 10);
  specialFunctionsString.replace("D5", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D6, nanoName, 10);
  specialFunctionsString.replace("D6", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D7, nanoName, 10);
  specialFunctionsString.replace("D7", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D8, nanoName, 10);
  specialFunctionsString.replace("D8", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_D9, nanoName, 10);
  specialFunctionsString.replace("D9", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_RESET, nanoName, 10);
  specialFunctionsString.replace("RESET", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_AREF, nanoName, 10);
  specialFunctionsString.replace("AREF", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A0, nanoName, 10);
  specialFunctionsString.replace("A0", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A1, nanoName, 10);
  specialFunctionsString.replace("A1", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A2, nanoName, 10);
  specialFunctionsString.replace("A2", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A3, nanoName, 10);
  specialFunctionsString.replace("A3", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A4, nanoName, 10);
  specialFunctionsString.replace("A4", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A5, nanoName, 10);
  specialFunctionsString.replace("A5", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A6, nanoName, 10);
  specialFunctionsString.replace("A6", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  itoa(NANO_A7, nanoName, 10);
  specialFunctionsString.replace("A7", nanoName);
  for (int i = 0; i < 10; i++) {
    nanoName[i] = ' ';
  }
  // if(debugFP)Serial.println(bridgeString);
  if (debugFP)
    Serial.println(specialFunctionsString);
  if (debugFP)
    Serial.println("\n\n\r");
}

void parseStringToBridges(void) {

  // int bridgeStringLength = bridgeString.length() - 1;

  int specialFunctionsStringLength = specialFunctionsString.length() - 1;

  int readLength = 0;

  newBridgeLength = 0;
  newBridgeIndex = 0;

  if (debugFP) {
    Serial.println("parsing bridges into array\n\r");
  }
  int stringIndex = 0;
  char delimitersCh[] = "[,- \n\r";

  createSafeString(buffer, 10);
  createSafeStringFromCharArray(delimiters, delimitersCh);
  int doneReading = 0;

  for (int i = 0; i <= specialFunctionsStringLength; i++) {

    stringIndex =
        specialFunctionsString.stoken(buffer, stringIndex, delimiters);
    if (stringIndex == -1) {
      break;
    }

    // Serial.print("buffer = ");
    // Serial.println(buffer);

    // Serial.print("stringIndex = ");
    // Serial.println(stringIndex);

    buffer.toInt(path[newBridgeIndex].node1);

    // Serial.print("path[newBridgeIndex].node1 = ");
    // Serial.println(path[newBridgeIndex].node1);

    if (debugFP) {
      Serial.print("node1 = ");
      Serial.println(path[newBridgeIndex].node1);
    }

    stringIndex =
        specialFunctionsString.stoken(buffer, stringIndex, delimiters);

    buffer.toInt(path[newBridgeIndex].node2);

    if (debugFP) {
      Serial.print("node2 = ");
      Serial.println(path[newBridgeIndex].node2);
    }

    readLength = stringIndex;

    if (readLength == -1) {
      doneReading = 1;
      break;
    }
    newBridgeLength++;
    newBridgeIndex++;

    if (debugFP) {
      Serial.print("readLength = ");
      Serial.println(readLength);
      Serial.print("specialFunctionsString.length() = ");
      Serial.println(specialFunctionsString.length());
    }

    if (debugFP)
      Serial.print(newBridgeIndex);
    if (debugFP)
      Serial.print("-");
    if (debugFP)
      Serial.println(newBridge[newBridgeIndex][1]);
  }

  newBridgeIndex = 0;
  if (debugFP)
    for (int i = 0; i < newBridgeLength; i++) {
      Serial.print("[");
      Serial.print(path[newBridgeIndex].node1);
      Serial.print("-");
      Serial.print(path[newBridgeIndex].node2);
      Serial.print("],");
      newBridgeIndex++;
    }
  if (debugFP)
    Serial.print("\n\rbridge pairs = ");
  if (debugFP)
    Serial.println(newBridgeLength);

  // nodeFileString.clear();

  // if(debugFP)Serial.println(nodeFileString);
  timeToFP = millis() - timeToFP;
  if (debugFPtime)
    Serial.print("\n\rtook ");

  if (debugFPtime)
    Serial.print(timeToFP);
  if (debugFPtime)
    Serial.println("ms to open and parse file\n\r");

  // printBridgeArray();

  // printNodeFile();
}

int lenHelper(int x) {
  if (x >= 1000000000)
    return 10;
  if (x >= 100000000)
    return 9;
  if (x >= 10000000)
    return 8;
  if (x >= 1000000)
    return 7;
  if (x >= 100000)
    return 6;
  if (x >= 10000)
    return 5;
  if (x >= 1000)
    return 4;
  if (x >= 100)
    return 3;
  if (x >= 10)
    return 2;
  return 1;
}

int printLen(int x) { return x < 0 ? lenHelper(-x) + 1 : lenHelper(x); }

// Helper functions for net color tracking and directory management
void ensureNetColorsDirectoryExists() {
  if (!FatFS.exists("/net_colors")) {
    if (FatFS.mkdir("/net_colors")) {
      if (debugFP) {
        Serial.println("Created /net_colors/ directory");
      }
    } else {
      if (debugFP) {
        Serial.println("Failed to create /net_colors/ directory");
      }
    }
  }
}

bool slotHasNetColors(int slot) {
  if (slot < 0 || slot >= 32) return false; // Support up to 32 slots with bitmask
  return (slotsWithNetColors & (1U << slot)) != 0;
}

void setSlotHasNetColors(int slot, bool hasColors) {
  if (slot < 0 || slot >= 32) return;
  if (hasColors) {
    slotsWithNetColors |= (1U << slot);
  } else {
    slotsWithNetColors &= ~(1U << slot);
  }
}

void removeNetColorFile(int slot) {
  String colorFileName = "/net_colors/netColorsSlot" + String(slot) + ".txt";
  if (FatFS.exists(colorFileName.c_str())) {
    FatFS.remove(colorFileName.c_str());
    if (debugFP) {
      Serial.println("Removed empty net color file: " + colorFileName);
    }
  }
  setSlotHasNetColors(slot, false);
}

bool slotIsValidated(int slot) {
  if (slot < 0 || slot >= 32) return false; // Support up to 32 slots with bitmask
  return (slotsValidated & (1U << slot)) != 0;
}

void setSlotValidated(int slot, bool validated) {
  if (slot < 0 || slot >= 32) return;
  if (validated) {
    slotsValidated |= (1U << slot);
  } else {
    slotsValidated &= ~(1U << slot);
  }
}

void markSlotAsModified(int slot) {
  // When a slot is modified, it needs re-validation
  setSlotValidated(slot, false);
  if (debugFP) {
    Serial.println("Marked slot " + String(slot) + " as needing validation");
  }
}

void initializeNetColorTracking() {
  // Reset tracking variable
  slotsWithNetColors = 0;
  
  // Check if net colors directory exists
  if (!FatFS.exists("/net_colors")) {
    if (debugFP) {
      Serial.println("Net colors directory does not exist. No existing colors to track.");
    }
    return;
  }
  
  // Scan for existing net color files
  int foundFiles = 0;
  for (int slot = 0; slot < 32 && slot < NUM_SLOTS; slot++) {
    String colorFileName = "/net_colors/netColorsSlot" + String(slot) + ".txt";
    if (FatFS.exists(colorFileName.c_str())) {
      // Check if file has content
      File tempFile = FatFS.open(colorFileName.c_str(), "r");
      if (tempFile && tempFile.size() > 0) {
        setSlotHasNetColors(slot, true);
        foundFiles++;
        if (debugFP) {
          Serial.println("Found net colors for slot " + String(slot));
        }
      }
      if (tempFile) {
        tempFile.close();
      }
    }
  }
  
  if (debugFP) {
    Serial.println("Initialized net color tracking. Found " + String(foundFiles) + " slots with colors.");
  }
}

void initializeValidationTracking() {
  // Reset validation tracking - all slots need validation on startup
  slotsValidated = 0;
  
  if (debugFP) {
    Serial.println("Initialized validation tracking. All slots marked for validation on first use.");
  }
}





///@brief prints the disconnected nodes (separated by commas)
///@return the number of disconnected nodes
int printDisconnectedNodes() {
  int charCount = 0;

  if (lastRemovedNodesIndex > 0) {
    // Serial.print("");
    for (int i = 0; i < lastRemovedNodesIndex; i++) {
      if (lastRemovedNodes[i] != -1) {
        charCount += printNodeOrName(lastRemovedNodes[i], 1);
        if (i < lastRemovedNodesIndex - 1 && i < 3) {
          Serial.print(", ");
          charCount += 2;
        }
      }
    }
  }
  return lastRemovedNodesIndex;
}

///@brief returns the next disconnected node (repeated calls return the next
///one), returns -1 if no more disconnected nodes
///@return the next disconnected node, or -1 if no more disconnected nodes
int disconnectedNode() {
  static int lastIndex = 0;
  if (disconnectedNodeNewData) {
    lastIndex = 0;
    disconnectedNodeNewData = false;
  }
  if (lastIndex >= lastRemovedNodesIndex || lastRemovedNodesIndex == 0) {
    return -1;
  }
  return lastRemovedNodes[lastIndex++];
}

int loadChangedNetColorsFromFile(int slot, int flashOrLocal) {
  // debugFP = 1;
  if (flashOrLocal == 1) {
    // Loading from an in-memory string is not implemented for colors in this
    // version.
    if (debugFP) {
      Serial.println(
          "Loading net colors from memory string not implemented for slot " +
          String(slot));
    }
    return 0; // Indicate operation not performed or not applicable
  }

  // Fast check: if slot has no colors according to our tracking variable, skip file operations
  if (!slotHasNetColors(slot)) {
    if (debugFP) {
      Serial.println("Slot " + String(slot) + " has no net colors (fast check). Skipping file operations.");
    }
    return 0; // No colors to load
  }

  String colorFileName = "/net_colors/netColorsSlot" + String(slot) + ".txt";

  // Check if file exists before attempting to get its size or open it
  if (debugFP) {
    Serial.print("if exists = ");
    Serial.println(millis());
  }
  if (!FatFS.exists(colorFileName.c_str())) {
    if (debugFP) {
      Serial.println(colorFileName + " does not exist. No colors loaded.");
      Serial.print("does not exist = ");
      Serial.println(millis());
    }
    // File doesn't exist but tracking says it should - clear the tracking bit
    setSlotHasNetColors(slot, false);
    return 0; // File doesn't exist, nothing to load
  }

  // Open temporarily to check size.
  // Note: This involves an open/close. If performance is critical and FatFS
  // offers a stat-like function, that would be better.
  File tempFile = FatFS.open(colorFileName.c_str(), "r");
  if (!tempFile) {
    if (debugFP) {
      Serial.println("Error opening " + colorFileName +
                     " for size check despite existing. No colors loaded.");
    }
    return 0; // Should not happen if exists() passed, but a safeguard
  }
  size_t fileSize = tempFile.size();
  tempFile.close();

  if (fileSize == 0) {
    if (debugFP) {
      Serial.println(colorFileName + " is empty. No colors loaded.");
    }
    return 0; // File is empty
  }

  // Thread safety mechanism
  // core1request = 1;
  // while (core2busy == true) {
  //   // Yield or delay slightly if needed, current pattern is spin-wait
  // }
  //core1request = 0;
  core1busy = true;

  // Initialize/clear the global ::changedNetColors array
  // Assumes changedNetColors is declared extern and MAX_NETS is defined
  for (int i = 0; i < MAX_NETS; ++i) {
    ::changedNetColors[i].net = 0;    // Or -1 to indicate invalid/empty
    ::changedNetColors[i].color = 0;  // Default color
    ::changedNetColors[i].node1 = 0;  // Or -1
    ::changedNetColors[i].node2 = -1; // Default from struct definition
  }

  if (::colorFile) { // Close if already open from a previous logical operation
    ::colorFile.close();
  }

  ::colorFile = FatFS.open(colorFileName.c_str(), "r");

  if (!::colorFile) {
    if (debugFP) {
      Serial.println("Failed to open " + colorFileName +
                     " for reading colors.");
    }
    //core1busy = false;
    return 0; // Indicate failure
  }

  if (debugFP) {
    Serial.println("Opened " + colorFileName + " for reading net colors.");
  }

  int colorsLoadedCount = 0;
  createSafeString(
      lineBuffer,
      120); // Buffer for a single line, e.g., "123:AABBCC:123:123" + padding
  createSafeString(token, 20); // Buffer for individual tokens

  while (::colorFile.available() && colorsLoadedCount < MAX_NETS) {
    String arduinoLine = ::colorFile.readStringUntil('\n');

    // Check for genuine EOF if readStringUntil returns empty at the end of file
    if (arduinoLine.length() == 0 && !::colorFile.available()) {
      break;
    }

    lineBuffer = arduinoLine.c_str(); // Copy to SafeString for parsing
    lineBuffer
        .trim(); // Remove leading/trailing whitespace (including potential \r)

    if (lineBuffer.isEmpty()) {
      continue; // Skip empty lines
    }

    int currentIndex = 0;
    int netId = 0, n1 = 0, n2 = 0;
    uint32_t colorVal = 0;

    // Parse netId
    token.clear();
    currentIndex = lineBuffer.stoken(token, currentIndex, ":");
    if (currentIndex == -1 || !token.toInt(netId)) {
      if (debugFP)
        Serial.printf("Malformed line (netId): %s\n", lineBuffer.c_str());
      continue;
    }

    // Parse color (hex string)
    token.clear();
    currentIndex = lineBuffer.stoken(token, currentIndex, ":");
    if (currentIndex == -1) {
      if (debugFP)
        Serial.printf("Malformed line (color): %s\n", lineBuffer.c_str());
      continue;
    }
    colorVal =
        strtoul(token.c_str(), NULL, 16); // Convert hex string to uint32_t

    // Parse node1
    token.clear();
    currentIndex = lineBuffer.stoken(token, currentIndex, ":");
    if (currentIndex == -1 || !token.toInt(n1)) {
      if (debugFP)
        Serial.printf("Malformed line (node1): %s\n", lineBuffer.c_str());
      continue;
    }

    // Parse node2 (rest of the string after the last colon)
    token.clear();
    if (currentIndex < lineBuffer.length()) {
      lineBuffer.substring(token, currentIndex + 1,
                           lineBuffer.length()); // Get the rest of the string

      // Serial.print("token = ");
      // Serial.println(token);
      if (!token.toInt(n2)) {
        if (debugFP)
          Serial.printf("Malformed line (node2 valuennn): %s\n",
                        lineBuffer.c_str());
        n2 = -1;
        // continue;
      }
    } else {
      if (debugFP)
        Serial.printf("Malformed line (node2 missing): %s\n",
                      lineBuffer.c_str());
      // continue; // Malformed line, missing node2 part
    }

    // Populate the struct in the global array

    if (netId > 0 && netId < MAX_NETS) {
      int index = netId;
      changedNetColors[index].net = netId;
      changedNetColors[index].color = colorVal;
      changedNetColors[index].node1 = n1;
      changedNetColors[index].node2 = n2;
    }
    colorsLoadedCount++;
  }

  if (debugFP) {
    Serial.println(String(colorsLoadedCount) +
                   " net color entries loaded from " + colorFileName);
  }

  /// printChangedNetColorFile(slot, flashOrLocal);
  // colorFile.seek(0);
  // currentColorSlotColorsString = colorFile.read();
  // currentColorSlotColorsString.printTo(Serial);
  // Serial.println();

  ::colorFile.close();
  //core1busy = false;
  // debugFP = 0;
  return 1; // Indicate success
}

void printAllChangedNetColorFiles(void) {
  bool foundAnyColors = false;
  
  for (int i = 0; i < NUM_SLOTS; i++) {
    // Only check slots that have colors according to our tracking variable
    if (slotHasNetColors(i)) {
      foundAnyColors = true;
      Serial.println("Slot " + String(i) + ":");
      printChangedNetColorFile(i, 0);
    }
  }
  
  if (!foundAnyColors) {
    Serial.println("No slots have net color overrides.");
  }
}

int printChangedNetColorFile(int slot, int flashOrLocal) {
  if (flashOrLocal == 1) {
    // Print from cache (currentColorSlotColorsString)
    // The cache always refers to the current netSlot, so 'slot' param is
    // implicitly current netSlot
    core1request = 1; // Lock for reading global variable
    while (core2busy == true) {
    }
    core1request = 0;
    core1busy = true;

    if (debugFP) {
      Serial.println(
          "Printing cached net colors (currentColorSlotColorsString):");
    }
    if (currentColorSlotColorsString.isEmpty()) {
      Serial.println("[Cache is empty or not yet loaded]");
    } else {
      Serial.print(currentColorSlotColorsString);
      if (!currentColorSlotColorsString.endsWith(
              "\n")) { // Ensure a newline for cleaner output if not present
        Serial.println();
      }
    }
    core1busy = false;
    return 1;
  }

  // flashOrLocal == 0, print from file
  // Fast check: if slot has no colors according to our tracking variable, skip file operations
  if (!slotHasNetColors(slot)) {
    Serial.println("Slot " + String(slot) + " has no net color overrides.");
    return 1; // Success - no colors is valid
  }

  String colorFileName = "/net_colors/netColorsSlot" + String(slot) + ".txt";

  if (!FatFS.exists(colorFileName.c_str())) {
    Serial.println("Color file " + colorFileName + " does not exist.");
    // File doesn't exist but tracking says it should - clear the tracking bit
    setSlotHasNetColors(slot, false);
    return 0;
  }

  core1request = 1;
  while (core2busy == true) {
  }
  core1request = 0;
  core1busy = true;

  if (::colorFile) {
    ::colorFile.close();
  }

  ::colorFile = FatFS.open(colorFileName.c_str(), "r");

  if (!::colorFile) {
    if (debugFP) {
      Serial.println("Failed to open " + colorFileName + " for printing.");
    }
    core1busy = false;
    return 0;
  }

  if (debugFP) {
    Serial.println("Printing contents of " + colorFileName + ":");
  }

  if (::colorFile.size() == 0) {
    Serial.println("[File is empty]");
  } else {
    while (::colorFile.available()) {
      Serial.write(::colorFile.read());
    }
    // Ensure a newline if the file doesn't end with one for cleaner console
    // output This requires peeking or reading the last char, which is slightly
    // complex here. Simpler: just print a newline if Serial.print didn't just
    // do one (hard to track) For now, we assume file content includes newlines
    // or user handles formatting.
  }

  ::colorFile.close();
  core1busy = false;
  return 1;
}

int saveChangedNetColorsToFile(int slot, int flashOrLocal) {
  // Serialize ::changedNetColors into a temporary string first
  createSafeString(tempColorDataString,
                   1500); // Matches currentColorSlotColorsString size
  int colorsSerializedCount = 0;
  for (int i = 0; i < numberOfNets; ++i) {
    if (::changedNetColors[i].net > 0) { // Only save if net is valid
      tempColorDataString.print(::changedNetColors[i].net);
      tempColorDataString.print(":");

      char hexColor[7]; // For RRGGBB format + null terminator
      sprintf(hexColor, "%06X", ::changedNetColors[i].color & 0xFFFFFF);
      tempColorDataString.print(hexColor);
      tempColorDataString.print(":");
      tempColorDataString.print(::changedNetColors[i].node1);
      tempColorDataString.print(":");
      tempColorDataString.println(::changedNetColors[i].node2);
      colorsSerializedCount++;
    }
  }

  if (debugFP) {
    Serial.println("Serialized " + String(colorsSerializedCount) +
                   " net color entries internally.");
  }

  // If no colors to save, remove file if it exists and clear tracking bit
  if (colorsSerializedCount == 0) {
    if (debugFP) {
      Serial.println("No net colors to save for slot " + String(slot) + ". Cleaning up.");
    }
    
    if (flashOrLocal == 0) {
      removeNetColorFile(slot);
    } else {
      setSlotHasNetColors(slot, false);
      currentColorSlotColorsString.clear();
    }
    return 1; // Success - no colors is valid
  }

  if (flashOrLocal == 0) { // Save to Flash and update cache if current slot
    core1request = 1;
    while (core2busy == true) {
    }
    core1request = 0;
    core1busy = true;

    // Ensure net colors directory exists
    ensureNetColorsDirectoryExists();

    String colorFileName = "/net_colors/netColorsSlot" + String(slot) + ".txt";
    if (::colorFile) {
      ::colorFile.close();
    }
    ::colorFile = FatFS.open(colorFileName.c_str(), "w");

    if (!::colorFile) {
      if (debugFP) {
        Serial.println("Failed to open " + colorFileName +
                       " for writing colors.");
      }
      core1busy = false;
      return 0; // Indicate failure
    }

    tempColorDataString.printTo(
        ::colorFile); // Write the serialized string to file
    ::colorFile.close();

    // Update tracking bit to indicate this slot has colors
    setSlotHasNetColors(slot, true);

    if (debugFP) {
      Serial.println("Saved net colors to " + colorFileName);
    }

    // If this is the currently active slot, update the cache
    if (slot ==
        netSlot) { // Assuming netSlot is the global variable for current slot
      currentColorSlotColorsString = tempColorDataString;
      if (debugFP) {
        Serial.println("Updated currentColorSlotColorsString for slot " +
                       String(slot));
      }
    }
    core1busy = false;

    // printChangedNetColorFile(slot, flashOrLocal);
    return 1; // Indicate success

  } else { // flashOrLocal == 1, Save to Cache ONLY (for current slot)
    // Assuming this mode implies the operation is for the current netSlot
    core1request = 1; // Maintain lock for consistency, as it modifies a
                      // shared-context global
    while (core2busy == true) {
    }
    core1request = 0;
    core1busy = true;

    currentColorSlotColorsString = tempColorDataString;
    // Update tracking bit to indicate this slot has colors (cache has data)
    setSlotHasNetColors(slot, true);
    
    if (debugFP) {
      Serial.println("Updated currentColorSlotColorsString from "
                     "::changedNetColors (cache only).");
    }
    core1busy = false;
    return 1; // Indicate success
  }
}

// Node File Validation and Repair System
// =====================================
// This system prevents infinite loops and unresponsive behavior when bad data
// exists in node files (e.g., malformed connections like "ILES," without
// dashes).
//
// The repair process:
// 1. validateAndRepairNodeFile() - Main entry point, attempts repair up to
// maxRetries times
// 2. attemptNodeFileRepair() - Parses file, removes malformed connections,
// saves clean version
// 3. If repair fails completely, clears the file to ensure system remains
// responsive
//
// This is called from openNodeFile() before any parsing to prevent crashes
// during startup.

bool attemptNodeFileRepair(int slot) {
  if (debugFP) {
    Serial.println("◇ Attempting to repair nodeFileSlot" + String(slot) +
                   ".txt");
  }

  String content = readSlotFileContent(slot);
  if (content.length() == 0) {
    return false; // Nothing to repair
  }

  // Extract content between braces
  int openBraceIdx = content.indexOf("{");
  int closeBraceIdx = content.indexOf("}");

  if (openBraceIdx == -1 || closeBraceIdx == -1) {
    if (debugFP) {
      Serial.println("◇ Missing braces, creating clean file");
    }
    clearNodeFile(slot, 0);
    return true;
  }

  String connections = content.substring(openBraceIdx + 1, closeBraceIdx);
  connections.trim();

  if (connections.length() == 0) {
    return true; // Empty file is valid
  }

  // Split connections and validate each one
  String repairedConnections = "";
  int validConnections = 0;
  int startIdx = 0;
  int commaIdx = connections.indexOf(',', startIdx);

  while (startIdx < connections.length()) {
    String connection;

    if (commaIdx == -1) {
      connection = connections.substring(startIdx);
    } else {
      connection = connections.substring(startIdx, commaIdx);
      startIdx = commaIdx + 1;
      commaIdx = connections.indexOf(',', startIdx);
    }

    connection.trim();

    // Skip empty connections
    if (connection.length() == 0) {
      if (commaIdx == -1)
        break;
      continue;
    }

    // Validate connection format
    int dashIdx = connection.indexOf('-');
    if (dashIdx != -1) {
      String node1Str = connection.substring(0, dashIdx);
      String node2Str = connection.substring(dashIdx + 1);

      node1Str.trim();
      node2Str.trim();

      // Check if both parts are valid numbers or known names
      bool valid = true;
      int node1 = node1Str.toInt();
      int node2 = node2Str.toInt();

      // If toInt() returns 0, check if it was actually "0" or a failed
      // conversion
      if ((node1 == 0 && node1Str != "0") || (node2 == 0 && node2Str != "0")) {
        // Check if they're valid node names (will be converted later)
        if (node1Str.length() < 2 || node2Str.length() < 2) {
          valid = false;
        }
      } else {
        // Validate numeric node numbers
        if (isNodeValid(node1) != 1 || isNodeValid(node2) != 1) {
          valid = false;
        }
      }

      if (valid) {
        if (repairedConnections.length() > 0) {
          repairedConnections += ",";
        }
        repairedConnections += connection;
        validConnections++;
      } else {
        if (debugFP) {
          Serial.println("◇ Removed invalid connection: '" + connection + "'");
        }
      }
    } else {
      if (debugFP) {
        Serial.println("◇ Removed malformed connection (no dash): '" +
                       connection + "'");
      }
    }

    if (commaIdx == -1)
      break;
  }

  // Write the repaired content back to file
  core1request = 1;
  while (core2busy == true) {
  }
  core1request = 0;
  core1busy = true;

  File slotFile = FatFS.open("nodeFileSlot" + String(slot) + ".txt", "w");
  if (slotFile) {
    slotFile.print("{ ");
    if (repairedConnections.length() > 0) {
      slotFile.print(repairedConnections);
    }
    slotFile.print(" }");
    slotFile.close();

    if (debugFP) {
      Serial.println("◆ Repaired nodeFileSlot" + String(slot) + ".txt with " +
                     String(validConnections) + " valid connections");
    }
  }

  core1busy = false;
  return validConnections >=
         0; // Success even if no valid connections (empty is valid)
}

bool validateAndRepairNodeFile(int slot, int maxRetries) {
  for (int attempt = 0; attempt < maxRetries; attempt++) {
    int validation_result = validateNodeFileSlot(slot, debugFP);

    if (validation_result == 0) {
      if (debugFP && attempt > 0) {
        Serial.println("◆ NodeFile validation passed after repair");
      }
      return true; // Valid
    }

    if (debugFP) {
      Serial.println(
          "◇ NodeFile validation failed (attempt " + String(attempt + 1) +
          "): " + String(getNodeFileValidationError(validation_result)));
    }

    // Attempt repair
    if (!attemptNodeFileRepair(slot)) {
      if (debugFP) {
        Serial.println("◇ Repair attempt failed");
      }
      break;
    }
  }

  // If we get here, repair failed - clear the file
  if (debugFP) {
    Serial.println("◇ All repair attempts failed, clearing nodeFileSlot" +
                   String(slot) + ".txt");
  }
  clearNodeFile(slot, 0);
  return true; // Cleared file is valid
}
