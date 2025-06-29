// SPDX-License-Identifier: MIT
#ifndef FILEPARSING_H
#define FILEPARSING_H

#include <Arduino.h>

// #include "RotaryEncoder.h"


//extern File nodeFile;  
extern volatile bool core1busy;
extern volatile bool netsUpdated;
void createConfigFile(int overwrite = 0);

int saveChangedNetColorsToFile(int slot = 0, int flashOrLocal = 0);
int loadChangedNetColorsFromFile(int slot = 0, int flashOrLocal = 0);
int printChangedNetColorFile(int slot = 0, int flashOrLocal = 0);
void printAllChangedNetColorFiles(void);
int checkIfBridgeExists(int node1, int node2 = -1, int slot = -1, int flashOrLocal = 1);

void clearNodeFileString(void);
void closeAllFiles(void);
void usbFSbegin(void);
int openFileThreadSafe(int openTypeEnum, int slot = 0, int flashOrLocal = 0);
void createLocalNodeFile(int slot = 0);
void saveLocalNodeFile(int slot = 0);   
void writeMenuTree(void);
void createSlots(int slot = -1,  int overwrite = 0);
void inputNodeFileList(int addRotaryConnections = 0);
//this just opens the file, takes out all the bullshit, and then populates the newBridge array
void parseWokwiFileToNodeFile();
void changeWokwiDefinesToJumperless ();
void writeToNodeFile(int slot = 0, int flashOrLocal = 0);
int removeBridgeFromNodeFile(int node1, int node2 = -1, int slot = 0, int flashOrLocal = 0, int onlyCheck = 0);
int addBridgeToNodeFile(int node1, int node2, int slot = 0, int flashOrLocal = 0, int allowDuplicates = 1); //returns 1 if duplicate was found
void savePreformattedNodeFile (int source = 0, int slot = 0, int keepEncoder = 1);

void readStringFromSerial(int source = 0, int addRemove = 0);
// void addStringToNodeFile(String str);
// void removeStringFromNodeFile(String str);
int parseStringToNode(int source = 0);

int getSlotLength(int slot, int flashOrLocal = 0);
void openNodeFile(int slot = 0, int flashOrLocal = 0);

void splitStringToFields();

void replaceSFNamesWithDefinedInts();
void printNodeFile(int slot = 0, int printOrString = 0, int flashOrLocal = 0, int definesInts = 0, bool printEmpty = true);
void replaceNanoNamesWithDefinedInts();
void saveCurrentSlotToSlot(int slotFrom = 0, int slotTo = 1, int flashOrLocalfrom = 0, int flashOrLocalTo = 0);
void parseStringToBridges();


void clearNodeFile(int slot = 0, int flashOrLocal = 0);
int lenHelper(int);
int printLen(int);
int isNodeValid(int node);

// NodeFile validation functions
int validateNodeFile(const String& content, bool verbose = false);
int validateNodeFileSlot(int slot, bool verbose = false);
const char* getNodeFileValidationError(int errorCode);

// Slot content caching and validation functions
String readSlotFileContent(int slot);
bool isSlotContentValid(const String& content);
bool hasSlotContentChanged(void);
void updateSlotCache(void);
void validateAllSlots(bool verbose = false);
bool isSlotFileEmpty(const String& content);
bool isSlotFileEmpty(int slot);

// External declarations for node file operations
// extern File nodeFile;
// extern SafeString nodeFileString;
// extern int netSlot;

// Buffer to store nodes that were removed in the last removeBridgeFromNodeFile call
extern int lastRemovedNodes[20];
extern int lastRemovedNodesIndex;
extern bool disconnectedNodeNewData;

// Function declarations
// void closeAllFiles();
// int openFileThreadSafe(int openTypeEnum, int slot = 0, int flashOrLocal = 0);

// Helper to print disconnected nodes after calling removeBridgeFromNodeFile
int printDisconnectedNodes(void);
int disconnectedNode(void);



#endif
