#ifndef APPS_H
#define APPS_H

#include <Arduino.h>
#include <stdint.h>

#define NUM_APPS 30


struct app {
  char name[20];
    int index;
    int works;
    void (*action)(void);
};


extern struct app apps[30];


void runApp (int index = -1, char* name = nullptr);

int i2cScan(int sdaRow = -1 , int sclRow = -1, int sdaPin = 26, int sclPin = 27, int leaveConnections = 0);
void scanBoard(void);
void calibrateDacs(void);
void bounceStartup(void);
void fileManagerApp(void);
void customApp(void);
void xlsxGui(void);
void probeCalibApp(void);


void displayImage(void);
const char* addressToHexString(uint8_t address);

void printSerial1stuff(void);
void microPythonREPLapp(void);
// int i2cScan(int sdaRow = -1 , int sclRow = -1, int sdaPin = 22, int sclPin = 23, int leaveConnections = 0);









#endif