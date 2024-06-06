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

extern int probeSwap;



void debugFlagSet(int flag);
void debugFlagInit(int forceDefaults = 0);
void saveLEDbrightness(int forceDefaults = 0);



void saveDebugFlags(void);

char lastCommandRead(void);
void lastCommandWrite(char lastCommand);

void runCommandAfterReset(char);
#endif