// SPDX-License-Identifier: MIT
#ifndef FILEPARSING_H
#define FILEPARSING_H



// #include "RotaryEncoder.h"


//extern File nodeFile;  
void createSlots(int slot = -1, int addRotaryConnections = 0);
void inputNodeFileList(int addRotaryConnections = 0);
//this just opens the file, takes out all the bullshit, and then populates the newBridge array
void parseWokwiFileToNodeFile();
void changeWokwiDefinesToJumperless ();
void writeToNodeFile(int slot = 0);
void removeBridgeFromNodeFile(int node1, int node2 = -1, int slot = 0);
void addBridgeToNodeFile(int node1, int node2, int slot = 0);
void savePreformattedNodeFile (int source = 0, int slot = 0, int keepEncoder = 1);

void openNodeFile(int slot = 0);

void splitStringToFields();

void replaceSFNamesWithDefinedInts();
void printNodeFile(int slot = 0, int printOrString = 0);
void replaceNanoNamesWithDefinedInts();
void saveCurrentSlotToSlot(int slotFrom = 0, int slotTo = 1);
void parseStringToBridges();


void clearNodeFile(int slot = 0);
int lenHelper(int);
int printLen(int);




#endif
