#ifndef HIGHLIGHTING_H
#define HIGHLIGHTING_H

#include <Arduino.h>
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "NetManager.h"
#include "RotaryEncoder.h"

// Global variables for highlighting functionality
extern rgbColor highlightedOriginalColor;
extern rgbColor brightenedOriginalColor;  
extern rgbColor warningOriginalColor;

extern int firstConnection;

extern int showReadingRow;
extern int showReadingNet;
extern int highlightedRow;
extern int lastNodeHighlighted;
extern int lastNetPrinted;
extern int lastPrintedNet;

extern int currentHighlightedNode;
extern int currentHighlightedNet;

extern int warningRow;
extern int warningNet;
extern unsigned long warningTimeout;
extern unsigned long warningTimer;

extern unsigned long highlightTimer;

// Additional highlighting variables that were in LEDs.h
extern int highlightedNet;
extern int probeConnectHighlight;
extern int brightenedNode;
extern int brightenedNet;
extern int brightenedRail;
extern int brightenedAmount;
extern int brightenedNodeAmount;
extern int brightenedNetAmount;

// Function declarations
void clearHighlighting(void);
int encoderNetHighlight(int print = 1, int mode = 1, int divider = 4);
int brightenNet(int node, int addBrightness = 5);
int warnNet(int node);
void warnNetTimeout(int clearAll = 1);
int highlightNets(int probeReading, int encoderNetHighlighted = -1, int print = 1);
int checkForReadingChanges(void);

#endif // HIGHLIGHTING_H
