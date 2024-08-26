#include "Graphics.h"
#include "Adafruit_NeoPixel.h"
#include "CH446Q.h"
#include "Commands.h"
#include "FileParsing.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "MatrixStateRP2040.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "Probing.h"
#include "RotaryEncoder.h"

/* clang-format off */

uint8_t upperCase [30][3] = {{
0x1e, 0x05, 0x1e, },{ 0x1f, 0x15, 0x0a, },{ 
0x1f, 0x11, 0x11, },{ 0x1f, 0x11, 0x0e, },{ 0x1f, 0x15, 0x11, },{ 0x1f, 0x05, 0x01, },{ 
0x0e, 0x15, 0x1d, },{ 0x1f, 0x04, 0x1f, },{ 0x11, 0x1f, 0x11, },{ 0x08, 0x10, 0x0f, },{ 
0x1f, 0x04, 0x1b, },{ 0x1f, 0x10, 0x10, },{ 0x1f, 0x07, 0x1f, },{ 0x1f, 0x01, 0x1f, },{ 
0x1f, 0x11, 0x1f, },{ 0x1f, 0x05, 0x07, },{ 0x0f, 0x09, 0x17, },{ 0x1f, 0x0d, 0x17, },{ 
0x17, 0x15, 0x1d, },{ 0x01, 0x1f, 0x01, },{ 0x1f, 0x10, 0x1f, },{ 0x0f, 0x10, 0x0f, },{ 
0x1f, 0x0c, 0x1f, },{ 0x1b, 0x04, 0x1b, },{ 0x03, 0x1c, 0x03, },{ 0x19, 0x15, 0x13, },{ //Z

}};

uint8_t lowerCase [30][3] = {{
    0x1c, 0x0a, 0x1c, },{ 0x1e, 0x14, 0x08, },{ 0x0c, 0x12, 0x12, },{ 0x08, 0x14, 0x1e, },{ 
0x0e, 0x16, 0x14, },{ 0x1c, 0x0a, 0x02, },{ 0x14, 0x16, 0x0e, },{ 0x1e, 0x04, 0x18, },{ 
0x00, 0x1d, 0x00, },{ 0x10, 0x0d, 0x00, },{ 0x1e, 0x08, 0x16, },{ 0x00, 0x1e, 0x10, },{ 
0x1e, 0x06, 0x1e, },{ 0x1e, 0x02, 0x1c, },{ 0x1e, 0x12, 0x1e, },{ 0x1e, 0x0a, 0x04, },{ 
0x04, 0x0a, 0x1c, },{ 0x1e, 0x02, 0x04, },{ 0x14, 0x1a, 0x0a, },{ 0x04, 0x1e, 0x14, },{ 
0x1e, 0x10, 0x1e, },{ 0x0e, 0x10, 0x0e, },{ 0x1e, 0x18, 0x1e, },{ 0x16, 0x08, 0x16, },{ 
0x06, 0x18, 0x06, },{ 0x12, 0x1a, 0x16, }}; //z

uint8_t fontNumbers [10][3] = {{
0x1f, 0x11, 0x1f, },{ 0x12, 0x1f, 0x10, },{ 0x1d, 0x15, 0x17, },{ 0x11, 0x15, 0x1f, },{ 
0x07, 0x04, 0x1f, },{ 0x17, 0x15, 0x1d, },{ 0x1f, 0x15, 0x1d, },{ 0x19, 0x05, 0x03, },{ 
0x1f, 0x15, 0x1f, },{ 0x17, 0x15, 0x1f, }}; //9

uint8_t symbols [50][3] = {
{ 0x00, 0x17, 0x00, }, //'!'
{ 0x16, 0x1f, 0x0d, }, //$
{ 0x19, 0x04, 0x13, }, //%
{ 0x02, 0x01, 0x02, }, //^
{ 0x02, 0x07, 0x02, }, //'*'
{ 0x10, 0x10, 0x10, }, //_
{ 0x04, 0x04, 0x04, }, //-
{ 0x04, 0x0e, 0x04, }, //+
{ 0x04, 0x15, 0x04, }, //÷
{ 0x0a, 0x04, 0x0a, }, //x
{ 0x0a, 0x0a, 0x0a, }, //=
{ 0x12, 0x17, 0x12, }, //±
{ 0x01, 0x1d, 0x07, }, //?
{ 0x04, 0x0a, 0x11, }, //<
{ 0x11, 0x0a, 0x04, }, //>
{ 0x06, 0x04, 0x0c, }, //~
{ 0x01, 0x02, 0x00, }, //'
{ 0x10, 0x08, 0x00, }, //,
{ 0x00, 0x10, 0x00, }, //.
{ 0x18, 0x04, 0x03, }, // '/'
{ 0x03, 0x04, 0x18, }, // '\'
{ 0x00, 0x0e, 0x11, }, // (
{ 0x11, 0x0e, 0x00, }, // )
{ 0x00, 0x1f, 0x11, }, // [
{ 0x00, 0x11, 0x1f, }, // ]
{ 0x04, 0x0e, 0x1b, }, // {
{ 0x1b, 0x0e, 0x04, }, // }
{ 0x00, 0x1f, 0x00, }, // |
{ 0x10, 0x0a, 0x00, }, // ;
{ 0x00, 0x0a, 0x00, }, // :
{ 0x1e, 0x08, 0x06, }, // µ
{ 0x07, 0x05, 0x07, }, // °
{ 0x04, 0x0e, 0x1f, }, // ❬ thicc <
{ 0x1f, 0x0e, 0x04, }, // ❭ thicc >
{ 0x03, 0x00, 0x03, }, // "
{ 0x00, 0x03, 0x00, }, // '
{ 0x0a, 0x0f, 0x08, }, // 𝟷
{ 0x0d, 0x0b, 0x00, }, // 𝟸
{ 0x09, 0x0b, 0x0f, }};// 𝟹

// char symbolMap[40] = {
// '!', '$', '%', '^', '*', '_', '-', '+', '÷', 'x', '=', '±', '?', '<', '>', '~', '\'', ',', '.', '/', '\\', '(', ')', '[', ']', '{', '}', '|', ';', ':', 'µ', '°', '❬', '❭', '"', '\'', '𝟷', '𝟸', '𝟹'};

wchar_t fontMap[120] = {
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'!', '$', '%', '^', '*', '_', '-', '+', L'÷', 'x', '=', L'±', '?', '<', '>', '~', '\'', ',', '.', '/', '\\', 
'(', ')', '[', ']', '{', '}', '|', ';', ':', L'µ', L'°', L'❬', L'❭', '"', '\'', L'𝟷', L'𝟸', L'𝟹', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};



uint8_t font[][3] = // 'JumperlessFontmap', 500x5px
{{
0x1f, 0x11, 0x1f, },{ 0x12, 0x1f, 0x10, },{ 0x1d, 0x15, 0x17, },{ 0x11, 0x15, 0x1f, },{ 
0x07, 0x04, 0x1f, },{ 0x17, 0x15, 0x1d, },{ 0x1f, 0x15, 0x1d, },{ 0x19, 0x05, 0x03, },{ 
0x1f, 0x15, 0x1f, },{ 0x17, 0x15, 0x1f, },{ //9

0x1e, 0x05, 0x1e, },{ 0x1f, 0x15, 0x0a, },{ 
0x1f, 0x11, 0x11, },{ 0x1f, 0x11, 0x0e, },{ 0x1f, 0x15, 0x11, },{ 0x1f, 0x05, 0x01, },{ 
0x0e, 0x15, 0x1d, },{ 0x1f, 0x04, 0x1f, },{ 0x11, 0x1f, 0x11, },{ 0x08, 0x10, 0x0f, },{ 
0x1f, 0x04, 0x1b, },{ 0x1f, 0x10, 0x10, },{ 0x1f, 0x07, 0x1f, },{ 0x1f, 0x01, 0x1f, },{ 
0x1f, 0x11, 0x1f, },{ 0x1f, 0x05, 0x07, },{ 0x0f, 0x09, 0x17, },{ 0x1f, 0x0d, 0x17, },{ 
0x17, 0x15, 0x1d, },{ 0x01, 0x1f, 0x01, },{ 0x1f, 0x10, 0x1f, },{ 0x0f, 0x10, 0x0f, },{ 
0x1f, 0x0c, 0x1f, },{ 0x1b, 0x04, 0x1b, },{ 0x03, 0x1c, 0x03, },{ 0x19, 0x15, 0x13, },{ //Z

0x1c, 0x0a, 0x1c, },{ 0x1e, 0x14, 0x08, },{ 0x0c, 0x12, 0x12, },{ 0x08, 0x14, 0x1e, },{ 
0x0e, 0x16, 0x14, },{ 0x1c, 0x0a, 0x02, },{ 0x14, 0x16, 0x0e, },{ 0x1e, 0x04, 0x18, },{ 
0x00, 0x1d, 0x00, },{ 0x10, 0x0d, 0x00, },{ 0x1e, 0x08, 0x16, },{ 0x00, 0x1e, 0x10, },{ 
0x1e, 0x06, 0x1e, },{ 0x1e, 0x02, 0x1c, },{ 0x1e, 0x12, 0x1e, },{ 0x1e, 0x0a, 0x04, },{ 
0x04, 0x0a, 0x1c, },{ 0x1e, 0x02, 0x04, },{ 0x14, 0x1a, 0x0a, },{ 0x04, 0x1e, 0x14, },{ 
0x1e, 0x10, 0x1e, },{ 0x0e, 0x10, 0x0e, },{ 0x1e, 0x18, 0x1e, },{ 0x16, 0x08, 0x16, },{ 
0x06, 0x18, 0x06, },{ 0x12, 0x1a, 0x16, },{ //z
    
0x00, 0x17, 0x00, },{ 0x16, 0x1f, 0x0d, },{ 
0x19, 0x04, 0x13, },{ 0x02, 0x01, 0x02, },{ 0x02, 0x07, 0x02, },{ 0x10, 0x10, 0x10, },{ 
0x04, 0x04, 0x04, },{ 0x04, 0x0e, 0x04, },{ 0x04, 0x15, 0x04, },{ 0x0a, 0x04, 0x0a, },{ 
0x0a, 0x0a, 0x0a, },{ 0x12, 0x17, 0x12, },{ 0x01, 0x1d, 0x07, },{ 0x04, 0x0a, 0x11, },{ 
0x11, 0x0a, 0x04, },{ 0x12, 0x17, 0x12, },{ 0x01, 0x02, 0x00, },{ 0x10, 0x08, 0x00, },{ 
0x00, 0x10, 0x00, },{ 0x18, 0x04, 0x03, },{ 0x03, 0x04, 0x18, },{ 0x00, 0x0e, 0x11, },{ 
0x11, 0x0e, 0x00, },{ 0x00, 0x1f, 0x11, },{ 0x00, 0x11, 0x1f, },{ 0x04, 0x0e, 0x1b, },{ 
0x1b, 0x0e, 0x04, },{ 0x00, 0x1f, 0x00, },{ 0x10, 0x0a, 0x00, },{ 0x00, 0x0a, 0x00, },{ 
0x1e, 0x08, 0x06, },{ 0x07, 0x05, 0x07, },{ 0x04, 0x0e, 0x1f, },{ 0x1f, 0x0e, 0x04, },{ 
0x03, 0x00, 0x03, },{ 0x00, 0x03, 0x00, },{ 0x0a, 0x0f, 0x08, },{ 0x0d, 0x0b, 0x00, },{ 
0x09, 0x0b, 0x0f, },{

0x1e, 0x12, 0x1e, },{ 0x14, 0x1e, 0x10, },{ 0x1a, 0x12, 0x16, },{ 0x12, 0x16, 0x1e, },{ //lowercase Numbers
0x0e, 0x08, 0x1e, },{ 0x16, 0x12, 0x1a, },{ 0x1e, 0x1a, 0x1a, },{ 0x12, 0x0a, 0x06, },{ 
0x1e, 0x1a, 0x1e, },{ 0x16, 0x16, 0x1e, }

};


int wireStatus[62][5]; // row, led (net stored)
char defconString[16] = " Fuck    You   ";
/* clang-format on */
int colorCycle = 0;
void defcon(int start, int spread, int color) {
  spread = 13;
  b.clear();
  b.print(defconString[0],
          logoColorsAll[color][(start) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 0, 0, 1);
  b.print(defconString[1],
          logoColorsAll[color][(start + spread) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 1, 0, 1);

  b.print(defconString[2],
          logoColorsAll[color][(start + spread * 2) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 2, 0, 1);
  b.print(defconString[3],
          logoColorsAll[color][(start + spread * 3) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 3, 0, 1);
  b.print(defconString[4],
          logoColorsAll[color][(start + spread * 4) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 4, 0, 1);
  b.print(defconString[5],
          logoColorsAll[color][(start + spread * 5) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 5, 0, 1);
  b.print(defconString[6],
          logoColorsAll[color][(start + spread * 5) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 6, 0, 1);
  b.print(defconString[7],
          logoColorsAll[color][(start + spread * 6) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 0, 1, -1);
  b.print(defconString[8],
          logoColorsAll[color][(start + spread * 7) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 1, 1, -1);
  b.print(defconString[9],
          logoColorsAll[color][(start + spread * 8) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 2, 1, -1);
  b.print(defconString[10],
          logoColorsAll[color][(start + spread * 9) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 3, 1, -1);
  b.print(defconString[11],
          logoColorsAll[color][(start + spread * 10) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 4, 1, -1);
  b.print(defconString[12],
          logoColorsAll[color][(start + spread * 11) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 5, 1, -1);
  b.print(defconString[13],
          logoColorsAll[color][(start + spread * 12) % (LOGO_COLOR_LENGTH - 1)],
          (uint32_t)0xffffff, 6, 1, -1);

  leds.setPixelColor(bbPixelToNodesMapV5[1][1],
                     logoColorsAll[0][(start) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[3][1],
      logoColorsAll[0][(start + spread) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[5][1],
      logoColorsAll[0][(start + spread * 2) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[7][1],
      logoColorsAll[0][(start + spread * 3) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[9][1],
      logoColorsAll[0][(start + spread * 4) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[11][1],
      logoColorsAll[0][(start + spread * 5) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[13][1],
      logoColorsAll[0][(start + spread * 6) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[15][1],
      logoColorsAll[0][(start + spread * 7) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[17][1],
      logoColorsAll[0][(start + spread * 8) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[19][1],
      logoColorsAll[0][(start + spread * 9) % (LOGO_COLOR_LENGTH - 1)]);

  leds.setPixelColor(
      bbPixelToNodesMapV5[21][1],
      logoColorsAll[0][(start + spread * 10) % (LOGO_COLOR_LENGTH - 1)]);
  leds.setPixelColor(
      bbPixelToNodesMapV5[23][1],
      logoColorsAll[0][(start + spread * 11) % (LOGO_COLOR_LENGTH - 1)]);

  // b.print('M', logoColors[(start + spread * 2) % (LOGO_COLOR_LENGTH - 1)],
  // 2,0); b.print('P', logoColors[(start + spread * 3) % (LOGO_COLOR_LENGTH -
  // 1)], 3,0); b.print('E', logoColors[(start + spread * 4) %
  // (LOGO_COLOR_LENGTH - 1)], 4,0); b.print('R', logoColors[(start + spread *
  // 5) % (LOGO_COLOR_LENGTH - 1)], 5,0); b.print('L', logoColors[(start +
  // spread * 6) % (LOGO_COLOR_LENGTH - 1)], 6,0); b.print('E',
  // logoColors[(start + spread * 7) % (LOGO_COLOR_LENGTH - 1)], 8,1);
  // b.print('S', logoColors[(start + spread * 8) % (LOGO_COLOR_LENGTH - 1)],
  // 9,1); b.print('S', logoColors[(start + spread * 9) % (LOGO_COLOR_LENGTH -
  // 1)], 10,1); b.print(' ', logoColors[(start + spread * 10) %
  // (LOGO_COLOR_LENGTH - 1)], 11,1); b.print('V', logoColors[(start + spread *
  // 11) % (LOGO_COLOR_LENGTH - 1)], 12,1); b.print('5', logoColors[(start +
  // spread * 12) % (LOGO_COLOR_LENGTH - 1)], 13,1);
}
volatile bool dontUpdateLEDs = false;
volatile bool ledsUpdating = false;
volatile bool dontSwirl = false;  

bool ledUpdate(__unused struct repeating_timer *t) {
  // if (dontUpdateLEDs == true) {
  //   return false;
  // } // else{
  // // if (checkingButton == 1) {
  // //   return false;
  // // }
  // if (ledsUpdating == true) {
  //   return false;
  // }
cancel_repeating_timer(&timerStruct);
delayMicroseconds(100);
  if (sendAllPathsCore2 == 1) {
    sendAllPathsCore2 = 0;
   pathHandler();
   //return true;
  } 
  displayHandler();

 add_repeating_timer_ms(10, ledUpdate, NULL, &timerStruct);
  return true;
  
  //}
  //return false;
}

unsigned long logoFlashTimer = 0;

int arduinoReset = 0;
unsigned long lastTimeReset = 0;

unsigned long lastSwirlTime = 0;

int swirlCount = 42;
int spread = 7;

int csCycle = 0;
int onOff = 0;
float topRailVoltage = 0.0;
float botRailVoltage = 0.0;

int readcounter = 0;
unsigned long schedulerTimer = 0;
unsigned long schedulerUpdateTime = 500;
int rowProbed = 0;
int swirled = 0;
int countsss = 0;

int probeCycle = 0;

int tempDD = 0;

uint8_t lastPixelBuffer[LED_COUNT * 3];
int countFire = 0;
unsigned long lastFire = 0;


void displayHandler(void) {
  ledsUpdating = true;
  core2busy = false;
  // countFire++;
  // if (countFire == 10) {
  //   //Serial.println("Fire");
  //   Serial.println(millis() - lastFire);
  //   lastFire = millis();
  //   countFire = 0;
  // }
  // strobeCS();
  // setCSex(0,1);
  // setCSex(0,0);

  if (millis() - lastSwirlTime > 50 && loadingFile == 0 && showLEDsCore2 == 0 && dontSwirl == false) {
    readcounter++;

    // logoSwirl(swirlCount, spread, probeActive);

    lastSwirlTime = millis();

    if (swirlCount >= LOGO_COLOR_LENGTH - 1) {
      swirlCount = 0;

    } else {

      swirlCount++;
    }

    if (swirlCount % 10 == 0) {
      countsss++;
    }
    if (probeActive == 0) {
      showProbeLEDs = 3;
    }

    // probeLEDs.setPixelColor(0, 0xffffff);
    if (showLEDsCore2 == 0) {
      swirled = 1;
    }

    // leds.show();
  } else if (inClickMenu == 0 && probeActive == 0) {

    if (((countsss > 8 && defconDisplay > 0) || countsss > 20) &&
        defconDisplay != -1) {
      countsss = 0;

      if (defconDisplay == 0) {
        tempDD++;

        if (tempDD > 6) {
          tempDD = 0;
        }
        defconDisplay = tempDD;
      } else {
        // defconDisplay = 0;
      }
    }

    if (defconDisplay > 6) {
      defconDisplay = 0;
    }
    if (readcounter > 100) {
      readcounter = 0;
      // probeCycle++;
      if (probeCycle > 4) {
        probeCycle = 1;
      }
      // setGPIO();
      // showLEDsCore2 = 1;

      // readGPIO();
    }

    // readGPIO();

     if (dontSwirl == false) {
     
    showLEDmeasurements();
    }
  }

  if (showLEDsCore2 == 0 && swirled == 0 && sendAllPathsCore2 == 0) {

    rotaryEncoderStuff();
    return;
  }
  if (((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3 ||
       swirled == 1)) {

    // Serial.println(showLEDsCore2);
    int rails =
        showLEDsCore2; // 3 doesn't show nets and keeps control of the LEDs

    // if (swirled == 1) {
    //   rails = 2;
    // }
    if (rails != 3) {
      // core2busy = true;
      lightUpRail(-1, -1, 1);
      logoSwirl(swirlCount, spread, probeActive);
      /// core2busy = false;
    }

    if (rails == 5 || rails == 3) {
      // core2busy = true;
      logoSwirl(swirlCount, spread, probeActive);
      // core2busy = false;
    }

    if (rails != 2 && rails != 5 && rails != 3 && inClickMenu == 0 &&
        inPadMenu == 0) {
      if (defconDisplay >= 0 && probeActive == 0) {
        // core2busy = true;
        defcon(swirlCount, spread, defconDisplay);
        // core2busy = false;
      } else {
        // multicore_lockout_start_blocking();
        // multicore_lockout_start_timeout_us(1000);
        //  while(core1busy == true){
        //   core2busy = false;
        //   }
        // core2busy = true;

        showNets();

        // core2busy = false;
        // multicore_lockout_end_timeout_us(1000);
        // multicore_lockout_end_blocking();
      }
    }
    swirled = 0;
    // delayMicroseconds(220);
    // core2busy = true;
    //
    //   bool differentPixels = false;
    //   for (int i = 0; i < LED_COUNT*3; i++) {
    //     //Serial.println(lastPixelBuffer[i]);
    //     if (lastPixelBuffer[i] != leds.getPixels()[i]) {
    //       differentPixels = true;
    //       //Serial.println("Different Pixels");
    //       break;
    //     }

    //  }
    //  if (differentPixels) {
    // Serial.println(micros() - howLong);
    leds.show();

    // for (int i = 0; i < LED_COUNT * 3; i++) {
    //   lastPixelBuffer[i] = leds.getPixels()[i];
    // }
    // } else {
    //   //Serial.println("Same Pixels");
    // }
    // Serial.print(differentPixels? "Diff\t" : "Same\t");
    // Serial.println(micros() - howLong);

    // core2busy = false;
    //  probeLEDs.clear();

    // if (checkingButton == 0) {
    //  Serial.print("probeActive = ");
    //  Serial.println(probeActive);
    // showProbeLEDs = probeCycle;
    switch (showProbeLEDs) {
    case 1:
      probeLEDs.setPixelColor(0, 0x0000ff);
      // probeLEDs.show();
      break;
    case 2:
      probeLEDs.setPixelColor(0, 0xff0000);
      // probeLEDs.show();
      break;
    case 3:
      probeLEDs.setPixelColor(0, 0x00ff00);
      // probeLEDs.show();
      break;
    case 4:
      probeLEDs.setPixelColor(0, 0xffffff);
      // probeLEDs.show();
      break;

    default:
      break;
      showProbeLEDs = 0;
    }
    // core2busy = true;

    probeLEDs.show();

    //}

    // probeLEDs.setPixelColor(0, 0x000005);

    // probeLEDs.show();

    if (rails != 3) {
      showLEDsCore2 = 0;
    }
    // if (inClickMenu == 1) {
    //   rotaryEncoderStuff();
    // }
  }



  schedulerTimer = micros();
  ledsUpdating = false;
  return;
}

void drawWires(int net) {
  // int fillSequence[6] = {0,2,4,1,3,};
  // assignNetColors();

  int fillSequence[6] = {0, 1, 2, 3, 4};
  int fillIndex = 0;
  int filledPaths[60][3] = {-1}; // node1 node2 rowfilled

  for (int i = 0; i < 62; i++) {
    for (int j = 0; j < 5; j++) {
      wireStatus[i][j] = 0;
    }
  }
  if (net == -1) {

    for (int i = 0; i < numberOfNets; i++) {

      int sameLevel = 0;
      int bothOnTop = 0;
      int bothOnBottom = 0;
      int bothOnBB = 0;
      int whichIsLarger = 0;

      if (path[i].node1 != -1 && path[i].node2 != -1 &&
          path[i].node1 != path[i].node2) {
        if (path[i].node1 <= 60 && path[i].node2 <= 60) {
          bothOnBB = 1;
          if (path[i].node1 > 0 && path[i].node1 <= 30 && path[i].node2 > 0 &&
              path[i].node2 <= 30) {
            bothOnTop = 1;
            sameLevel = 1;
            if (path[i].node1 > path[i].node2) {
              whichIsLarger = 1;
            } else {
              whichIsLarger = 2;
            }
          } else if (path[i].node1 > 30 && path[i].node1 <= 60 &&
                     path[i].node2 > 30 && path[i].node2 <= 60) {
            bothOnBottom = 1;
            sameLevel = 1;
            if (path[i].node1 > path[i].node2) {
              whichIsLarger = 1;
            } else {
              whichIsLarger = 2;
            }
          }
        } else {
          lightUpNet(path[i].net);
        }

        if (sameLevel == 1) {
          int range = 0;
          int first = 0;
          int last = 0;
          if (whichIsLarger == 1) {
            range = path[i].node1 - path[i].node2;
            first = path[i].node2;
            last = path[i].node1;
          } else {
            range = path[i].node2 - path[i].node1;
            first = path[i].node1;
            last = path[i].node2;
          }

          // Serial.print("\nfirst = ");
          // Serial.println(first);
          // Serial.print("last = ");
          // Serial.println(last);
          // Serial.print("range = ");
          // Serial.println(range);
          // Serial.print("net = ");
          // Serial.println(path[i].net);

          int inside = 0;
          int largestFillIndex = 0;

          for (int j = first; j <= first + range; j++) {
            // Serial.print("j = ");
            // Serial.println(j);
            for (int w = 0; w < 5; w++) {
              if ((wireStatus[j][w] == path[i].net || wireStatus[j][w] == 0) &&
                  w >= largestFillIndex) {

                // wireStatus[j][w] = path[i].net;
                if (w > largestFillIndex) {
                  largestFillIndex = w;
                }
                // Serial.print("j = ");
                // Serial.println(j);
                // if (first > 30) {
                //   Serial.print("bottom ");
                // }
                // Serial.print("largestFillIndex = ");
                // Serial.println(largestFillIndex);
                break;
              }
            }
          }

          for (int j = first; j <= first + range; j++) {
            if (j == first || j == last) {
              for (int k = largestFillIndex; k < 5; k++) {

                wireStatus[j][k] = path[i].net;
                // wireStatus[j][largestFillIndex] = path[i].net;
              }
            } else {
              wireStatus[j][largestFillIndex] = path[i].net;
            }
          }

          fillIndex = largestFillIndex;

          filledPaths[i][0] = first;
          filledPaths[i][1] = last;
          filledPaths[i][2] = fillSequence[fillIndex];
          // showLEDsCore2 = 1;
        } else {
          for (int j = 0; j < 5; j++) {

            wireStatus[path[i].node1][j] = path[i].net;
            wireStatus[path[i].node2][j] = path[i].net;
          }
        }
      }
    }
    for (int i = 0; i <= 60; i++) {
      for (int j = 0; j < 4; j++) {
        if (wireStatus[i][j] != 0) {
          if (wireStatus[i][j + 1] != wireStatus[i][j] &&
              wireStatus[i][j + 1] != 0 &&
              wireStatus[i][4] == wireStatus[i][j]) {
            wireStatus[i][j + 1] = wireStatus[i][j];
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
          } else {
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x100010);
          }
        }
      }
    }
    for (int i = 1; i <= 60; i++) {
      if (i <= 30) {

        for (int j = 0; j < 5; j++) {

          uint32_t color3 = 0x100010;

          rgbColor colorRGB = netColors[wireStatus[i][j]];

          hsvColor colorHSV = RgbToHsv(colorRGB);

          // colorHSV.v = colorHSV.v * 0.25;
          // colorHSV.s = colorHSV.s * 0.5;
          colorRGB = HsvToRgb(colorHSV);

          uint32_t color = packRgb(colorRGB.r, colorRGB.g, colorRGB.b);

          if (wireStatus[i][j] == 0) {
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
          } else {
            leds.setPixelColor((((i - 1) * 5) + j), color);
          }
        }
      } else {
        for (int j = 0; j < 5; j++) {

          uint32_t color3 = 0x100010;

          rgbColor colorRGB = netColors[wireStatus[i][j]];

          hsvColor colorHSV = RgbToHsv(colorRGB);

          // colorHSV.v = colorHSV.v * 0.25;
          // colorHSV.s = colorHSV.s * 0.5;
          colorRGB = HsvToRgb(colorHSV);

          uint32_t color = packRgb(colorRGB.r, colorRGB.g, colorRGB.b);

          if (wireStatus[i][j] == 0) {
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
          } else {
            leds.setPixelColor((((i - 1) * 5) + (4 - j)), color);
          }
        }
      }
    }
  }
}
void printWireStatus(void) {

  for (int s = 1; s <= 30; s++) {
    Serial.print(s);
    Serial.print(" ");
    if (s < 9) {
      Serial.print(" ");
    }
  }
  Serial.println();

  int level = 1;
  for (int r = 0; r < 5; r++) {
    for (int s = 1; s <= 30; s++) {
      Serial.print(wireStatus[s][r]);
      Serial.print(" ");
      if (wireStatus[s][r] < 10) {
        Serial.print(" ");
      }
    }
    Serial.println();
  }
  Serial.println("\n\n");
  for (int s = 31; s <= 60; s++) {
    Serial.print(s);
    Serial.print(" ");
    if (s < 9) {
      Serial.print(" ");
    }
  }
  Serial.println();
  for (int r = 0; r < 5; r++) {
    for (int s = 31; s <= 60; s++) {
      Serial.print(wireStatus[s][r]);
      Serial.print(" ");
      if (wireStatus[s][r] < 10) {
        Serial.print(" ");
      }
    }
    Serial.println();
  }
}
// }
uint32_t defaultColor = 0x001012;

bread::bread() {
  // defaultColor = 0x060205;
}

void bread::print(const char c) { printChar(c, defaultColor); }

void bread::print(const char c, int position) {
  printChar(c, defaultColor, position);
}

void bread::print(const char c, uint32_t color) { printChar(c, color); }

void bread::print(const char c, uint32_t color, int position, int topBottom) {
  printChar(c, color, position, topBottom);
}

void bread::print(const char c, uint32_t color, int topBottom) {
  printChar(c, color, topBottom);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor) {
  printChar(c, color, backgroundColor);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom) {
  printChar(c, color, backgroundColor, position, topBottom);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge) {
  printChar(c, color, backgroundColor, position, topBottom, nudge);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge, int lowercaseNumber) {
  printChar(c, color, backgroundColor, position, topBottom, nudge,
            lowercaseNumber);
}

void bread::print(const char *s) {
  // Serial.println("1");
  printString(s, defaultColor);
}

void bread::print(const char *s, int position) {
  // Serial.println("2");
  printString(s, defaultColor, 0xffffff, position);
}

void bread::print(const char *s, uint32_t color) {
  // Serial.println("3");
  printString(s, color);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor) {
  // Serial.println("4");
  printString(s, color, backgroundColor);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom, nudge);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge, int lowercaseNumber) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom, nudge,
              lowercaseNumber);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int topBottom) {
  // Serial.println("6");
  printString(s, color, backgroundColor, 0, topBottom);
}

void bread::print(int i) {
  // Serial.println("7");
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, defaultColor);
  // Serial.println(buffer);
}

// void bread::print(int i, int position) {
//   char buffer[15];
//   itoa(i, buffer, 10);
//   printString(buffer, defaultColor, 0xffffff, position);
// }

void bread::print(int i, uint32_t color) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color);
}

void bread::print(int i, uint32_t color, int position) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position);
}

void bread::print(int i, uint32_t color, int position, int topBottom) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position, topBottom);
}
void bread::print(int i, uint32_t color, int position, int topBottom,
                  int nudge) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position, topBottom, nudge);
}
void bread::print(int i, uint32_t color, int position, int topBottom, int nudge,
                  int lowercase) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position, topBottom, nudge);
}

void bread::print(int i, uint32_t color, uint32_t backgroundColor) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, backgroundColor);
}

void bread::printMenuReminder(int menuDepth, uint32_t color) {
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};
  uint8_t graphicRow[3] = {0x00, 0x00, 0x00};

  if (menuDepth > 6) {
    menuDepth = 6;
  }

  switch (menuDepth) {
  case 1:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00010000;
    graphicRow[0] = 0b00010000;
    break;
  case 2:
    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00001000;
    graphicRow[0] = 0b00011000;

    break;

  case 3:
    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000100;
    graphicRow[0] = 0b00011100;

    break;

  case 4:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000010;
    graphicRow[0] = 0b00011110;

    break;

  case 5:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000001;
    graphicRow[0] = 0b00011111;

    break;

  case 6:

    graphicRow[2] = 0b00000001;
    graphicRow[1] = 0b00000001;
    graphicRow[0] = 0b00011111;

    break;
  }

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }

  for (int i = 0; i < 3; i++) {

    printGraphicsRow(graphicRow[i], i, color);
  }
}

void bread::printRawRow(uint8_t data, int row, uint32_t color, uint32_t bg) {

  if (row <= 60) {
    printGraphicsRow(data, row, color, bg);
  } else {
    for (int i = 0; i < 35; i++) {
      if (bbPixelToNodesMapV5[i][0] == row + 1) {
        leds.setPixelColor(bbPixelToNodesMapV5[i][1], color);
        return;
      }
    }
  }
}
/*

||||||||||||||||||||||||||||||
  |0| |1| |2| |3| |4| |5| |6|
||||||||||||||||||||||||||||||

||||||||||||||||||||||||||||||
  |7| |8| |9| |A| |B| |C| |D|
||||||||||||||||||||||||||||||


*/
void printGraphicsRow(uint8_t data, int row, uint32_t color, uint32_t bg) {
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }
  if (bg == 0xFFFFFF) {

    for (int j = 4; j >= 0; j--) {
      // Serial.println(((data) & columnMask[j]) != 0 ? "1" : "0");
      if (((data)&columnMask[j]) != 0) {

        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        leds.setPixelColor(((row) * 5) + j, 0);
      }
    }
  } else if (bg == 0xFFFFFE) {

    for (int j = 4; j >= 0; j--) {
      // Serial.println(((data) & columnMask[j]) != 0 ? "1" : "0");
      if (((data)&columnMask[j]) != 0) {

        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        // leds.getPixelColor(((row) * 5) + j);
        // leds.setPixelColor(((row) * 5) + j, 0);
      }
    }
  } else {
    for (int j = 4; j >= 0; j--) {
      if (((data)&columnMask[j]) != 0) {
        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        leds.setPixelColor(((row) * 5) + j, bg);
      }
    }
  }
}

void printChar(const char c, uint32_t color, uint32_t bg, int position,
               int topBottom, int nudge, int lowercaseNumber) {

  int charPosition = position;
  if (topBottom == 1) {
    charPosition = charPosition % 7;
    charPosition += 7;
    if (charPosition > 13) {
      return;
    }
  }
  if (topBottom == 0) {
    if (charPosition > 6) {
      return;
      // charPosition = charPosition % 7;
    }
    charPosition = charPosition % 7;
  }

  charPosition = charPosition % 14;

  charPosition = charPosition * 4;

  if (charPosition > (6 * 4)) {
    charPosition = charPosition + 2;
  }
  // charPosition = charPosition * 4;
  charPosition = charPosition + 2;

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }
  int fontMapIndex = -1;
  int start = 0;

  if (lowercaseNumber > 0) {
    start = 90;
  }

  for (int i = start; i < 120; i++) {
    if (c == fontMap[i]) {
      fontMapIndex = i;
      break;
    }
  }
  if (fontMapIndex == -1) {
    return;
  }
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};

  if (bg == 0xFFFFFF) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 5; j++) {
        if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
        } else {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, 0);
        }
      }
    }
  } else if (bg == 0xFFFFFD) {
    for (int j = 0; j < 5; j++) {

      leds.setPixelColor(((charPosition + nudge - 1) * 5) + j, 0);
    }

    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 5; j++) {
        if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
        } else {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, 0);
        }
      }
    }
    for (int j = 0; j < 5; j++) {

      leds.setPixelColor(((charPosition + nudge + 3) * 5) + j, 0);
    }
  } else if (bg == 0xFFFFFE) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 5; j++) {
        if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
        } else {
          // leds.setPixelColor((i*5)+j, bg);
        }
      }
    }
  } else {
    if (charPosition + nudge != 0) {
      for (int j = 0; j < 5; j++) {

        leds.setPixelColor(((charPosition + nudge - 1) * 5) + j, bg);
      }
    }
    for (int i = 0; i < 4; i++) {
      if (i < 3) {
        for (int j = 0; j < 5; j++) {
          if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
            leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
          } else {
            leds.setPixelColor(((charPosition + i + nudge) * 5) + j, bg);
          }
        }
      } else {
        for (int j = 0; j < 5; j++) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, bg);
        }
      }
    }
  }
}

void printString(const char *s, uint32_t color, uint32_t bg, int position,
                 int topBottom, int nudge, int lowercaseNumber) {
  // int position = 0;

  for (int i = 0; i < strlen(s); i++) {

    // if (topBottom == 1) {
    //   position = position % 7;

    // } else if (topBottom == 0) {
    //   position = position % 7;

    // } else {
    //   position = position % 14;
    // }
    // Serial.print(s[i]);
    // Serial.print(" ");
    // Serial.println(position);
    // if (i > strlen(s))
    // {
    //     printChar(' ', 0x000000, 0x000000, position, topBottom);
    // } else {
    // Serial.println(position);
    printChar(s[i], color, bg, position, topBottom, nudge, lowercaseNumber);
    // }

    position++;
  }
  // Serial.println();
}

void bread::clear(int topBottom) {
  if (topBottom == -1) {
    for (int i = 0; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
    }
  } else if (topBottom == 0) {
    for (int i = 0; i < 30; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
    }
  } else if (topBottom == 1) {
    for (int i = 30; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
    }
  }
  // leds.show();
}

void scrollFont() {
  // pauseCore2 = 1;
  //  scroll font
  //  uint8_t font[] = // 'JumperlessFontmap', 500x5px
  //  {0x1f, 0x11, 0x1f, 0x00, 0x12, 0x1f, 0x10, 0x00, 0x1d, 0x15, 0x17, 0x00,
  //  0x11, 0x15, 0x1f, 0x00,};
  uint32_t color = 0x060205;
  int scrollSpeed = 120;
  int scrollPosition = 0;

  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};
  while (Serial.available() == 0) {

    for (int i = 0; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        if ((font[(i + scrollPosition) % 500][0] & columnMask[j]) != 0) {
          // Serial.print("1");
          leds.setPixelColor((i * 5) + j, color);
        } else {
          // Serial.print("0");
          leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
        }
      }
    }
    leds.show();
    delay(scrollSpeed);
    scrollPosition++;
    if (scrollPosition > 499) {
      scrollPosition = 0;
    }
  }
}
