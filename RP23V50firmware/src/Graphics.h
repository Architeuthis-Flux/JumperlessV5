#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Arduino.h>
#include "LEDs.h"

extern const int screenMap[445];
extern const int screenMapNoRails[445];

extern volatile bool dumpingToSerial;

extern bool disableTerminalColors;

class bread {
public:
  bread();
  void print(const char c);
  void print(const char c, int position);
  void print(const char c, uint32_t color);
  void print(const char c, uint32_t color, int position, int topBottom);
  void print(const char c, uint32_t color, int position);
  void print(const char c, uint32_t color, uint32_t backgroundColor);
  void print(const char c, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom);
  void print(const char c, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom, int nudge);
  void print(const char c, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom, int nudge, int lowercase);
  void print(const char c, uint32_t color, uint32_t backgroundColor,
             int position);

  void print(const char *s);
  void print(const char *s, int position);
  void print(const char *s, uint32_t color);
  void print(const char *s, uint32_t color, int position);
  void print(const char *s, uint32_t color, int position, int topBottom);
  void print(const char *s, uint32_t color, uint32_t backgroundColor);
  void print(const char *s, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom);
  void print(const char *s, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom, int nudge);
  void print(const char *s, uint32_t color, uint32_t backgroundColor,
             int position, int topBottom, int nudge, int lowercaseNumber);
  void print(const char *s, uint32_t color, uint32_t backgroundColor,
             int position);

  void print(int i);
  void print(int i, int position);
  void print(int i, uint32_t color);
  void print(int i, uint32_t color, int position);
  void print(int i, uint32_t color, int position, int topBottom);
  void print(int i, uint32_t color, int position, int topBottom, int nudge);
  void print(int i, uint32_t color, int position, int topBottom, int nudge,
             int lowercase);
  void print(int i, uint32_t color, uint32_t backgroundColor);

  void barGraph(int position, int value, int maxValue, int leftRight,
                uint32_t color, uint32_t bg);

  void printMenuReminder(int menuDepth, uint32_t color);
  void printRawRow(uint8_t data, int row, uint32_t color, uint32_t bg,
                   int scale = 1);

  void clear(int topBottom = -1);
};



struct logoLedAssoc {
  int ledNumber;
  const char* text;
  int type;
  int defaultColor;
  int overrideMap;
  logoOverrideNames overrideName;
};

extern logoLedAssoc logoLedAssociations[20];
extern volatile bool logoLedAccess;
struct specialRowAnimation {
  int index;
  int net;
  int row;
  unsigned long currentFrame;
  int direction;
  int numberOfFrames = 8;
  uint32_t frames[15] = {0xffffff};
  unsigned long lastFrameTime;
  unsigned long frameInterval = 100;
  int type;
};

extern int defNudge;

extern specialRowAnimation rowAnimations[50];

extern specialRowAnimation warningRowAnimation;
extern specialRowAnimation warningNetAnimation;

extern int numberOfRowAnimations;
extern bool animationsEnabled;
extern char defconString[16];
extern const uint8_t font[][3];
extern volatile int doomOn;

extern uint32_t gpioReadingColors[10];
extern uint8_t gpioAnimationBaseHues[10];

extern int menuBrightnessSetting;
extern bread b;

extern const int highSaturationSpectrumColors[54];
extern const int highSaturationSpectrumColorsCount;

void printSpectrumOrderedColorCube(void);

void playDoom(void);
void showArray(uint8_t *array, int size);
void initRowAnimations(void);
void showAllRowAnimations(void);
void assignRowAnimations(void);
void showRowAnimation(int net);
void showRowAnimation(int index, int net);
void animateBrightenedRow(int net);
void printGraphicsRow(uint8_t data, int row, uint32_t color = 0xFFFFFF,
                      uint32_t bg = 0xFFFFFF);

void scrollFont(void);

void printChar(const char c, uint32_t color = 0xFFFFFF, uint32_t bg = 0xFFFFFF,
               int position = 0, int topBottom = -1, int nudge = 0,
               int lowercase = 0);

void printString(const char *s, uint32_t color = 0xFFFFFF,
                 uint32_t bg = 0xFFFFFF, int position = 0, int topBottom = -1,
                 int nudge = 0, int lowercase = 0);

void drawWires(int net = -1);
void printWireStatus(void);

void defcon(int start, int spread, int color = 0, int nudge = 1);

void printTextFromMenu(int print = 1);
int attractMode(void);

void changeTerminalColor(int termColor = -1, bool flush = true,
                         Stream *stream = &Serial);

// void cycleTerminalColor(bool reset = false, bool reverse = false, int step = -1, bool flush = true, Stream *stream = &Serial);
void cycleTerminalColor(bool reset = false, float step = 100.0, bool flush = true, Stream *stream = &Serial);

void drawImage(int imageIndex = 0);
void drawAnimatedImage(int imageIndex = 0, int speed = 2000);
void printRLEimageData(int imageIndex);
void printAllRLEimageData(void);

void dumpLEDs(int posX = 50, int posY = 27, int pixelsOrRows = 0,
              int header = 0, int rgbOrRaw = 0, int logo = 0,
              Stream *stream = &Serial);
void dumpHeader(int posX = 50, int posY = 20, int absolute = 1, int wide = 0,
                Stream *stream = &Serial);
void dumpHeaderHex(Stream *stream = &Serial);
void moveCursor(int posX = -1, int posY = -1, int absolute = 1,
                Stream *stream = &Serial, bool flush = false);
void saveCursorPosition(Stream *stream = &Serial);
void restoreCursorPosition(Stream *stream = &Serial);

// Alternate screen buffer functions for saving/restoring entire screen state
void saveScreenState(Stream *stream = &Serial);
void restoreScreenState(Stream *stream = &Serial);

void dumpHeaderMain(int posX = 50, int posY = 20, int absolute = 1,
                    int wide = 0);
void dumpLEDsMain(int posX = 50, int posY = 27, int pixelsOrRows = 0,
                  int header = 0, int rgbOrRaw = 0, int logo = 0);

#endif
