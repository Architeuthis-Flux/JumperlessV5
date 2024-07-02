
// SPDX-License-Identifier: MIT
#ifndef PROBING_H
#define PROBING_H

extern volatile int sfProbeMenu;
extern unsigned long probingTimer;
extern long probeFrequency;
extern int probePin;
extern int buttonPin;

extern int connectOrClearProbe;


extern volatile int probeActive;

enum measuredState
{
  floating = 0,
  high = 1,
  low = 2,
  probe = 3,
  unknownState = 4 

};


int voltageSelect(void);
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