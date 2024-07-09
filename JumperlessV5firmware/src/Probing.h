
// SPDX-License-Identifier: MIT
#ifndef PROBING_H
#define PROBING_H

extern volatile int sfProbeMenu;
extern unsigned long probingTimer;
extern long probeFrequency;
extern int probePin;
extern int buttonPin;

extern int connectOrClearProbe;
extern int node1or2;

extern int logoTopSetting[2];
extern int logoBottomSetting[2];
extern int buildingTopSetting[2];
extern int buildingBottomSetting[2];

extern volatile int probeActive;
extern int inPadMenu;

enum measuredState
{
  floating = 0,
  high = 1,
  low = 2,
  probe = 3,
  unknownState = 4 

};

void checkPads(void);
int delayWithButton(int delayTime = 1000);

int chooseGPIO(int skipInputOutput = 0);
int chooseGPIOinputOutput(int gpioChosen);
int chooseADC(void);
int chooseDAC(int justPickOne = 0);
int attachPadsToSettings(int pad);

float voltageSelect(int fiveOrEight = 8);
int longShortPress(int pressLength = 500); 
int doubleSingleClick(void);
int selectFromLastFound(void);
int checkLastFound(int);
void clearLastFound(void);
int probeMode(int pin = 19, int setOrClear = 1);
int checkProbeButton(void);
int readFloatingOrState (int pin = 0, int row = 0);

void startProbe (long probeSpeed = 25000);
void stopProbe();

int selectSFprobeMenu(int function = 0);

int getNothingTouched(int samples = 8);
int scanRows(int pin = 0);

int readRails(int pin = 0);

int readProbe(void);

int readProbeRaw(int readNothingTouched = 0); 
int calibrateProbe(void);

#endif