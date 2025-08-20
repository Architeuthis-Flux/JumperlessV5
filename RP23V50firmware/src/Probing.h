
// SPDX-License-Identifier: MIT
#ifndef PROBING_H
#define PROBING_H
#include "JumperlessDefines.h"

#define MINIMUM_PROBE_READING 48

extern volatile int sfProbeMenu;
extern unsigned long probingTimer;

extern int probePin;
extern int buttonPin;

extern volatile unsigned long blockProbing;
extern volatile unsigned long blockProbingTimer;

extern volatile unsigned long blockProbeButton;
extern volatile unsigned long blockProbeButtonTimer;

extern volatile int connectOrClearProbe;
extern int node1or2;
extern int probeHighlight;
extern int logoTopSetting[2];
extern int logoBottomSetting[2];
extern int buildingTopSetting[2];
extern int buildingBottomSetting[2];
extern int showProbeCurrent;

extern volatile int probeActive;
extern volatile int inPadMenu;
extern volatile int checkingButton;
extern volatile int measureModeActive;

extern int probeRowMapByPad[108];
extern int probeRowMap[108];


extern int probePowerDAC;
extern int lastProbePowerDAC;
extern bool probePowerDACChanged;

extern volatile int removeFade;

extern volatile bool bufferPowerConnected;

extern int debugProbing;

enum probePressType {
  connectPress = 2,
  disconnectPress = 1,
  connectLongPress = 4,
  disconnectLongPress = 3,
  doubleClickConnect = 5,
  doubleClickDisconnect = 6,
  noPress = 0
};



enum measuredState
{
  floating = 2,
  high = 1,
  low = 0,
  probe = 3,
  unknownState = 4 

};
extern volatile int showingProbeLEDs;
extern int switchPosition;
extern int lastSwitchPositions[3];



float measureMode(int updateSpeed = 150);
void checkPads(void);
int delayWithButton(int delayTime = 1000);

int chooseGPIO(int skipInputOutput = 0);
int chooseGPIOinputOutput(int gpioChosen);
int chooseADC(void);
int chooseDAC(int justPickOne = 0);
int attachPadsToSettings(int pad);

float voltageSelect(int fiveOrEight = 8);


int longShortPress(int pressLength = 500);


int selectFromLastFound(void);
int checkLastFound(int);
void clearLastFound(void);
int probeMode(int setOrClear = 1, int firstConnection = -1);
int checkProbeButton(void);
int checkProbeDoubleClick(unsigned long timeout, int waitForRelease = 0);
int readFloatingOrState (int pin = 0, int row = 0);

int checkSwitchPosition(void);
float checkProbeCurrent(void);
float checkProbeCurrentZero(void);

void routableBufferPower (int offOn, int flash = 0, int force = 0);

void startProbe (long probeSpeed = 25000);
void stopProbe();

int selectSFprobeMenu(int function = 0);

int getNothingTouched(int samples = 8);
int scanRows(int pin = 0);

int readRails(int pin = 0);
int justReadProbe(bool allowDuplicates = false, int rawPad = 0);
int readProbe(void);

int readProbeRaw(int readNothingTouched = 0, bool allowDuplicates = false); 
int calibrateProbe(void);
void calibrateDac0(float target = 3.3);

extern int lastProbeLEDs;


void probeLEDhandler(void); 
// int highlightNets(int probeReading, int encoderNetHighlighted = -1, int print = 1);

extern int probeRowMap[108];
  

#endif