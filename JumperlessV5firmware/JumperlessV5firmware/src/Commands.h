#ifndef COMMANDS_H
#define COMMANDS_H


extern volatile int sendAllPathsCore2;
extern volatile int showLEDsCore2;
extern volatile int showProbeLEDs;



void refreshConnections(int ledShowOption = 1);
void refreshLocalConnections(int ledShowOption = 1);
void updateLEDs(void);
void printSlots(int fileNo = -1);




























#endif