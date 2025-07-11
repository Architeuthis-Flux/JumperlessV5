#pragma once

// Uncomment to use EEPROM storage instead of config file
//#define EEPROMSTUFF

#include "config.h"
#include <EEPROM.h>
#include "configManager.h"

#ifndef PERSSISTENTSTUFF_H
#define PERSSISTENTSTUFF_H

extern bool debugFP;
extern bool debugFPtime;

extern bool debugNM;
extern bool debugNMtime;

extern bool debugNTCC;
extern bool debugNTCC2;

extern bool debugLEDs;
extern bool debugMM;


extern bool calibrateOnStart;
extern bool firstStart;

extern const char firmwareVersion[16];

void debugFlagSet(int flag);
void debugFlagInit(int forceDefaults = 0);
void saveLEDbrightness(int forceDefaults = 0);

void saveDuplicateSettings(int forceDefaults = 0);
void saveVoltages(float top , float bot , float dac0 ,  float dac1 );
void readVoltages(void);
void saveDebugFlags(void);
void saveDacCalibration(void);

void saveLogoBindings(void);
void readLogoBindings(void);
char lastCommandRead(void);
void lastCommandWrite(char lastCommand);

void runCommandAfterReset(char);

void readSettingsFromConfig();

void updateGPIOConfigFromState(void);
void updateStateFromGPIOConfig(void);

#endif