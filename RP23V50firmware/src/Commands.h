#ifndef COMMANDS_H
#define COMMANDS_H

#include <Arduino.h>    
extern volatile int sendAllPathsCore2;
extern volatile int showLEDsCore2;
extern volatile int showProbeLEDs;

extern volatile int core1request;

struct rowLEDs {
  uint32_t color[5];

};
void refreshBlind(int disconnectFirst = -1, int fillUnused = 0, int clean = 0);

unsigned long waitCore2(void);

void refresh(int flashOrLocal = 0, int ledShowOption = -1, int fillUnused = 1, int clean = 0);

void refreshConnections(int ledShowOption = 1,int fillUnused = 1, int clean = 0);
void refreshLocalConnections(int ledShowOption = 1, int fillUnused = 1, int clean = 0);
void updateLEDs(void);
void printSlots(int fileNo = -1);
bool checkFloating(int node);
struct rowLEDs getRowLEDdata (int row);
void setRowLEDdata (int row, struct rowLEDs);

void connectNodes(int node1, int node2);

void disconnectNodes(int node1, int node2);

float measureVoltage(int adcNumber, int node, bool checkForFloating = false);
float measureCurrent(int node1, int node2);

void setRailVoltage(int topBottom, float voltage);

void connectGPIO(int gpioNumber, int node);
























#endif