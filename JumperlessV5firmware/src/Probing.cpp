

#include "Probing.h"
#include "CH446Q.h"
#include "FileParsing.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "MatrixStateRP2040.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
// #include "AdcUsb.h"
#include "Commands.h"
#include "Graphics.h"
#include "PersistentStuff.h"
#include "RotaryEncoder.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <EEPROM.h>
#include <algorithm>

int debugProbing = 0;

#define OLED_CONNECTED 0

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS                                                         \
  0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int probeToRowMap2[102] = {
    0,  1,  2,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,  16,  17,
    18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,  33,  34,
    35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49,  50,  51,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66,  67,  68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83,  84,  85,
    86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101,
};
int lastReadRaw = 0;
int probeSwap = 0;
int probeHalfPeriodus = 20;

unsigned long probingTimer = 0;
long probeFrequency = 25000;

unsigned long probeTimeout = 0;

int lastFoundIndex = 0;

int lastFoundHistory[50] = {-1};

int connectedRowsIndex = 0;
int connectedRows[32] = {-1};

int connectedRows2Index[4] = {0, 0, 0, 0};
int connectedRows2[4][32];

int nodesToConnect[2] = {-1, -1};

int node1or2 = 0;

int probePin = 10;
int buttonPin = 9;

unsigned long probeButtonTimer = 0;

int justSelectedConnectedNodes = 0;

int voltageSelection = SUPPLY_3V3;
int voltageChosen = 0;
int connectOrClearProbe = 0;
int wasRotaryMode = 0;
volatile int inPadMenu = 0;
int rainbowList[13][3] = {
    {40, 50, 80}, {88, 33, 70}, {30, 15, 45}, {8, 27, 45},  {45, 18, 19},
    {35, 42, 5},  {02, 45, 35}, {18, 25, 45}, {40, 12, 45}, {10, 32, 45},
    {18, 5, 43},  {45, 28, 13}, {8, 12, 8}};
int rainbowIndex = 0;

int nextIsSupply = 0;
int nextIsGnd = 0;
int justCleared = 1;

int checkingPads = 0;

int justAttached = 0;
uint32_t deleteFade[13] = {0x371f16, 0x28160b, 0x191307, 0x141005, 0x0f0901,
                           0x090300, 0x050200, 0x030100, 0x030000, 0x020000,
                           0x010000, 0x000000, 0x000000};
int fadeIndex = 0;

int probeMode(int pin, int setOrClear) {
  if (checkingPads == 1) {
    Serial.println("checkingPads\n\r");
    return -1;
  }
    probeActive = 1;
  int deleteMisses[20] = {
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
  };
  int deleteMissesIndex = 0;
  clearLEDsExceptRails();
  startProbe();
  createLocalNodeFile();
  routableBufferPower(1);

restartProbing:
saveLocalNodeFile();

  if (setOrClear == 1) {
    showProbeLEDs = 1;
  } else {
    showProbeLEDs = 2;
  }


  connectOrClearProbe = setOrClear;
  int lastRow[10];

  // Serial.print(numberOfNets);

  if (numberOfNets == 0) {
    // clearNodeFile(netSlot);
  }

  int probedNodes[40][2];
  int probedNodesIndex = 0;

  int row[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  row[1] = -2;
  probeTimeout = millis();

  // probingTimer = millis();

  // Serial.print("Press any key to exit and commit paths (or touch probe to
  // gpio 18)   ");
  Serial.print("\n\r\t  Probing mode\n\n\r");
  Serial.print("   long press  = connect (blue) / clear (red)\n\r");
  Serial.println("   short press = commit");

  if (setOrClear == 1) {
    // sprintf(oledBuffer, "connect  ");
    // drawchar();
    Serial.println("\n\r\t  connect nodes\n\n\r");
    rawOtherColors[1] = 0x4500e8;
    rainbowIndex = 0;
  } else {
    // sprintf(oledBuffer, "clear");
    // drawchar();
    Serial.println("\n\r\t  clear nodes\n\n\r");
    rawOtherColors[1] = 0x6644A8;
    rainbowIndex = 12;
  }

  if (setOrClear == 0) {
    probeButtonTimer = millis();

    // probingTimer = millis() -400;
  }
  showLEDsCore2 = 1;

  unsigned long doubleSelectTimeout = millis();
  int doubleSelectCountdown = 0;

  int lastProbedRows[4] = {0, 0, 0, 0};
  unsigned long fadeTimer = millis();

  // Serial.print("\n\r");
  // Serial.println(setOrClear);
  while (Serial.available() == 0 && (millis() - probeTimeout) < 6200 &&
         encoderButtonState == IDLE) {
    delayMicroseconds(800);

    connectedRowsIndex = 0;

    row[0] = readProbe();
    // Serial.println(row[0]);

    // probeButtonToggle = checkProbeButton();

    if (setOrClear == 1) {
      deleteMissesIndex = 0;
    } else {
      if (millis() - fadeTimer > 50) {
        fadeTimer = millis();

        if (fadeIndex < 12) {
          fadeIndex++;
        } else {
          deleteMissesIndex = 0;
          for (int i = 0; i < 20; i++) {
            deleteMisses[i] = -1;
          }
        }

        int fadeFloor = fadeIndex;
        if (fadeFloor < 0) {
          fadeFloor = 0;
        }

        for (int i = deleteMissesIndex - 1; i >= 0; i--) {
          int fadeOffset = map(i, 0, deleteMissesIndex, 0, 12) + fadeFloor;
          if (fadeOffset > 12) {
            fadeOffset = 12;
          }
          b.printRawRow(0b00000100, deleteMisses[i] - 1, deleteFade[fadeOffset],
                        0xfffffe);
          //   Serial.print(i);
          //   Serial.print("   ");
          //   Serial.print(deleteMisses[i]);
          //   Serial.print("    ");
          //  Serial.println(map(i, 0,deleteMissesIndex, 0, 19));
        }
      }
    }

    if ((row[0] == -18 || row[0] == -16) &&
        (millis() - probingTimer > 500)) { //&&
      // checkProbeButton() == 1 ){//&& (millis() - probeButtonTimer) > 1000) {
      // Serial.println("button\n\r");
      // int buttonState = longShortPress(500);
      // int buttonState = checkProbeButton();

      if (row[0] == -18) {
        // Serial.print("setOrClear: ");
        // Serial.print(setOrClear);
        // Serial.print("\n\r");

        if (setOrClear == 0) {
          showProbeLEDs = 1;
          // delay(100);
        } else {

          setOrClear = 0;
          probingTimer = millis();
          probeButtonTimer = millis();
          // showNets();
          // showLEDsCore2 = 1;
          sfProbeMenu = 0;
          connectedRowsIndex = 0;
          connectedRows[0] = -1;
          goto restartProbing;
        }
        // break;
      } else if (row[0] == -16) {
        // Serial.print("setOrClear: ");
        // Serial.print(setOrClear);
        // Serial.print("\n\r");

        if (setOrClear == 1) {
          showProbeLEDs = 2;
          // delay(100);
        } else {
          setOrClear = 1;

          probingTimer = millis();
          probeButtonTimer = millis();
          // showNets();
          // showLEDsCore2 = 1;
          sfProbeMenu = 0;
          connectedRowsIndex = 0;
          connectedRows[0] = -1;

          for (int i = deleteMissesIndex - 1; i >= 0; i--) {

            b.printRawRow(0b00000100, deleteMisses[i] - 1, 0, 0xfffffe);
            //   Serial.print(i);
            //   Serial.print("   ");
            //   Serial.print(deleteMisses[i]);
            //   Serial.print("    ");
            //  Serial.println(map(i, 0,deleteMissesIndex, 0, 19));
          }

          goto restartProbing;
        }
        // break;
      }

      // Serial.print("\n\rCommitting paths!\n\r");
      row[1] = -2;
      probingTimer = millis();

      connectedRowsIndex = 0;

      node1or2 = 0;
      nodesToConnect[0] = -1;
      nodesToConnect[1] = -1;

      // showLEDsCore2 = 1;
      //saveLocalNodeFile();

      break;
    } else {
      // probingTimer = millis();
    }

    if (row[0] != -1 && row[0] != row[1]) { // && row[0] != lastProbedRows[0] &&
      // row[0] != lastProbedRows[1]) {

      lastProbedRows[1] = lastProbedRows[0];
      lastProbedRows[0] = row[0];
      if (connectedRowsIndex == 1) {
        nodesToConnect[node1or2] = connectedRows[0];
        printNodeOrName(nodesToConnect[0]);

        Serial.print("\r\t");

        if (nodesToConnect[node1or2] > 0 &&
            nodesToConnect[node1or2] <= NANO_RESET_1 && setOrClear == 1) {
          // b.clear();
          b.printRawRow(0b0010001, nodesToConnect[node1or2] - 1, 0x000121e,
                        0xfffffe);
           showLEDsCore2 = 2;
          delay(40);
          b.printRawRow(0b00001010, nodesToConnect[node1or2] - 1, 0x0f0498,
                        0xfffffe);
           showLEDsCore2 = 2;
          delay(40);

          b.printRawRow(0b00000100, nodesToConnect[node1or2] - 1, 0x4000e8,
                        0xfffffe);
           showLEDsCore2 = 2;
          delay(80);
          // showLEDsCore2 = 1;
        }

        node1or2++;
        probingTimer = millis();
        showLEDsCore2 = 1;
        doubleSelectTimeout = millis();
        doubleSelectCountdown = 1500;
        // delay(500);

        // delay(3);
      }
      if (node1or2 == 1) {
        // sprintf(oledBuffer, "%s", definesToChar(nodesToConnect[0]));
        // drawchar();
      }

      if (node1or2 >= 2 || (setOrClear == 0 && node1or2 >= 1)) {
        // Serial.print("\n\n\n\r!!!!!!!!!\n\n\n\r");
        //  if (nodesToConnect[0] != nodesToConnect[1])
        //  {

        // }

        if (setOrClear == 1 && (nodesToConnect[0] != nodesToConnect[1]) &&
            nodesToConnect[0] > 0 && nodesToConnect[1] > 0) {
          // char oledBuffer2[32];
          // int charsPrinted = 0;
          Serial.print("\r           \r");
          // itoa(nodesToConnect[0], oledBuffer, 10);
          printNodeOrName(nodesToConnect[0]);
          Serial.print(" - ");
          printNodeOrName(nodesToConnect[1]);

          Serial.print("   \t ");
          Serial.print("connected\n\r");

          char node1Name[12];

          // node1Name =  (char)definesToChar(nodesToConnect[0]);

          strcpy(node1Name, definesToChar(nodesToConnect[0]));

          char node2Name[12];

          // node2Name =  (char)definesToChar(nodesToConnect[1]);

          strcpy(node2Name, definesToChar(nodesToConnect[1]));

          // sprintf(oledBuffer, "%s - %s           ", node1Name, node2Name);

          // drawchar();
          // Serial.println("fuuuuuuuuck");

          addBridgeToNodeFile(nodesToConnect[0], nodesToConnect[1], netSlot, 1);
          // printNodeFile(netSlot);
          //  delay(100);
          //   rainbowIndex++;
          if (rainbowIndex > 1) {
            rainbowIndex = 0;
          }

          refreshLocalConnections(1);

          row[1] = -1;

          // doubleSelectTimeout = millis();
          for (int i = 0; i < 12; i++) {
            deleteMisses[i] = -1;
          }

          doubleSelectTimeout = millis();
          doubleSelectCountdown = 2000;

          // delay(400);
        } else if (setOrClear == 0) {

          Serial.print("\r                      \r");
          printNodeOrName(nodesToConnect[0]);

          for (int i = 12; i > 0; i--) {
            deleteMisses[i] = deleteMisses[i - 1];
            // Serial.print(i);
            // Serial.print("   ");
            // Serial.println(deleteMisses[i]);
          }
          // Serial.print("\n\r");
          deleteMisses[0] = nodesToConnect[0];

          // deleteMisses[deleteMissesIndex] = nodesToConnect[0];
          if (deleteMissesIndex < 12) {
            deleteMissesIndex++;
          }
          fadeIndex = -3;
          //  Serial.println("\n\r");
          //  Serial.print("deleteMissesIndex: ");
          //   Serial.print(deleteMissesIndex);
          //   Serial.print("\n\r");
          for (int i = deleteMissesIndex - 1; i >= 0; i--) {

            b.printRawRow(0b00000100, deleteMisses[i] - 1,
                          deleteFade[map(i, 0, deleteMissesIndex, 0, 12)],
                          0xfffffe);
            //   Serial.print(i);
            //   Serial.print("   ");
            //   Serial.print(deleteMisses[i]);
            //   Serial.print("    ");
            //  Serial.println(map(i, 0,deleteMissesIndex, 0, 19));
          }
          //  Serial.println();

          if (removeBridgeFromNodeFile(nodesToConnect[0], -1, netSlot, 1) > 0) {
            Serial.print("\t cleared");
            Serial.println();

            rainbowIndex = 12;
            // goto restartProbing;
            refreshLocalConnections(1);
            // deleteMissesIndex = 0;
            // for (int i = 0; i < 20; i++) {
            //   deleteMisses[i] = -1;
            // }
            // delay(20);
            fadeTimer = 0;
          }
        }
        node1or2 = 0;
        nodesToConnect[0] = -1;
        nodesToConnect[1] = -1;
        // row[1] = -2;
        doubleSelectTimeout = millis();
      }

      row[1] = row[0];
    }
    // Serial.print("\n\r");
    // Serial.print(" ");
    // Serial.println(row[0]);

    if (justSelectedConnectedNodes == 1) {
      justSelectedConnectedNodes = 0;
    }

    if (millis() - doubleSelectTimeout > 500) {
      // Serial.println("doubleSelectCountdown");
      row[1] = -2;
      lastReadRaw = 0;
      lastProbedRows[0] = 0;
      lastProbedRows[1] = 0;
      doubleSelectTimeout = millis();
      doubleSelectCountdown = 1000;
    }

    // Serial.println(doubleSelectCountdown);

    if (doubleSelectCountdown <= 0) {

      doubleSelectCountdown = 0;
    } else {
      doubleSelectCountdown =
          doubleSelectCountdown - (millis() - doubleSelectTimeout);

      doubleSelectTimeout = millis();
    }

    probeTimeout = millis();
  }
  // Serial.println("fuck you");
  // digitalWrite(RESETPIN, LOW);

   refreshLocalConnections();
   delay(10);
  saveLocalNodeFile();
  delay(10);
  refreshConnections();
  row[1] = -2;

  // sprintf(oledBuffer, "        ");
  // drawchar();

  // rotaryEncoderMode = wasRotaryMode;
  // routableBufferPower(0);
  // delay(10);
  return 1;
}

unsigned long blinkTimer = 0;
volatile int sfProbeMenu = 0;

uint32_t sfOptionColors[12] = {
    0x09000a, 0x0d0500, 0x000809, 0x040f00, 0x000f03, 0x00030d,
    0x080a00, 0x030013, 0x000a03, 0x00030a, 0x040010, 0x070006,
};

int selectSFprobeMenu(int function) {

  if (checkingPads == 1) {
    // inPadMenu = 0;
    return function;
  }
  
  switch (function) {

  case 132: {
    inPadMenu = 1;
    function = chooseADC();
    delay(100);
    inPadMenu = 0;

    break;
  }
  case 131: {
    inPadMenu = 1;
    function = chooseDAC();
    delay(100);
    inPadMenu = 0;

    break;
  }
  case 130: {
    
    function = chooseGPIO();
    delay(100);

    break;
  }
  case 128 ... 129:
  case 133 ... 134: {
    // b.clear();

    // b.clear();
    clearLEDsExceptRails();
    switch (function) {
    case 128: {
      inPadMenu = 1;
      b.print("Logo", sfOptionColors[3], 0xFFFFFF, 0, 1, 0);
      b.print("Top", sfOptionColors[7], 0xFFFFFF, 0, 0, 0);
      b.printRawRow(0b00000001, 23, 0x400014, 0xffffff);
      b.printRawRow(0b00000011, 24, 0x400014, 0xffffff);
      b.printRawRow(0b00011111, 25, 0x400014, 0xffffff);
      b.printRawRow(0b00011011, 26, 0x400014, 0xffffff);
      b.printRawRow(0b00000001, 27, 0x400014, 0xffffff);

      b.printRawRow(0b00011100, 53, 0x400014, 0xffffff);
      b.printRawRow(0b00011000, 54, 0x400014, 0xffffff);
      b.printRawRow(0b00010000, 55, 0x400014, 0xffffff);
      break;
    }
    case 129: {
      inPadMenu = 1;
      b.print("Logo", sfOptionColors[3], 0xFFFFFF, 0, 1, -1);
      b.print("Bottom", sfOptionColors[5], 0xFFFFFF, 0, 0, -1);

      b.printRawRow(0b00000000, 25, 0x280032, 0xffffff);
      b.printRawRow(0b00000001, 26, 0x280032, 0xffffff);
      b.printRawRow(0b00000011, 27, 0x280032, 0xffffff);

      b.printRawRow(0b00001110, 53, 0x280032, 0xffffff);
      b.printRawRow(0b00011110, 54, 0x280032, 0xffffff);
      b.printRawRow(0b00010000, 55, 0x280032, 0xffffff);
      b.printRawRow(0b00011111, 56, 0x280032, 0xffffff);
      b.printRawRow(0b00011111, 57, 0x280032, 0xffffff);
      b.printRawRow(0b00000011, 58, 0x280032, 0xffffff);

      b.printRawRow(0b00000001, 52, 0x050500, 0xfffffe);
      b.printRawRow(0b00000001, 53, 0x050500, 0xfffffe);
      b.printRawRow(0b00000001, 54, 0x050500, 0xfffffe);
      b.printRawRow(0b00000001, 55, 0x050500, 0xfffffe);
      b.printRawRow(0b00000001, 59, 0x050500, 0xfffffe);

      break;
    }
    case 133: {
      inPadMenu = 1;
      b.print("Buildng", sfOptionColors[6], 0xFFFFFF, 0, 1, -1);
      b.print("Top", sfOptionColors[7], 0xFFFFFF, 0, 0, 1);

      b.printRawRow(0b00011000, 24, 0x200010, 0xffffff);
      b.printRawRow(0b00011000, 25, 0x200010, 0xffffff);
      b.printRawRow(0b00011000, 26, 0x200010, 0xffffff);
      b.printRawRow(0b00011000, 27, 0x200010, 0xffffff);

      b.printRawRow(0b00000011, 24, 0x010201, 0xfffffe);
      b.printRawRow(0b00000011, 25, 0x010201, 0xfffffe);
      b.printRawRow(0b00000011, 26, 0x010201, 0xfffffe);
      b.printRawRow(0b00000011, 27, 0x010201, 0xfffffe);

      break;
    }
    case 134: {
      inPadMenu = 1;
      b.print("Buildng", sfOptionColors[6], 0xFFFFFF, 0, 1, -1);
      b.print("Bottom", sfOptionColors[5], 0xFFFFFF, 0, 0, -1);

      b.printRawRow(0b00000011, 25, 0x200010, 0xffffff);
      b.printRawRow(0b00000011, 26, 0x200010, 0xffffff);
      b.printRawRow(0b00000011, 27, 0x200010, 0xffffff);
      b.printRawRow(0b00000011, 28, 0x200010, 0xffffff);

      b.printRawRow(0b00011000, 25, 0x010201, 0xfffffe);
      b.printRawRow(0b00011000, 26, 0x010201, 0xfffffe);
      b.printRawRow(0b00011000, 27, 0x010201, 0xfffffe);
      b.printRawRow(0b00011000, 28, 0x010201, 0xfffffe);
      break;
    }
    }
    //showLEDsCore2 = 2;
    delayWithButton(800);

    // b.clear();
    clearLEDsExceptRails();

    // lastReadRaw = 0;
    b.print("Attach", sfOptionColors[0], 0xFFFFFF, 0, 0, -1);
    b.print("to Pad", sfOptionColors[2], 0xFFFFFF, 0, 1, -1);
    // showLEDsCore2 = 2;

    delayWithButton(800);

    // delay(800);

    function = attachPadsToSettings(function);
    // node1or2 = 0;
    // nodesToConnect[0] = function;
    // nodesToConnect[1] = -1;
    // connectedRowsIndex = 1;

    // Serial.print("function!!!!!: ");
    printNodeOrName(function, 1);
    showLEDsCore2 = 1;
    lightUpRail();
    delay(200);
    inPadMenu = 0;
    sfProbeMenu = 0;
    // return function;

    delay(100);

    break;
  }

  case 0: {
    Serial.print("0function: ");
    printNodeOrName(function, 1);
    Serial.print(function);
    Serial.println();
    function = -1;
    break;
  }
  case 104:
  case 126: {
    function = 100;
    break;
  }
  default:
  {
   // inPadMenu = 0;
  }
  }
  connectedRows[0] = function;
  connectedRowsIndex = 1;
  lightUpRail();
  // delay(500);
  sfProbeMenu = 0;
  // inPadMenu = 0;

  return function;
}

int logoTopSetting[2] = {-1, -1}; //{function, settingOption}
int logoBottomSetting[2] = {-1, -1};
int buildingTopSetting[2] = {-1, -1};
int buildingBottomSetting[2] = {-1, -1};

int attachPadsToSettings(int pad) {
  int function = -1;
  int functionSetting = -1; // 0 = DAC, 1 = ADC, 2 = GPIO
  int settingOption =
      -1; // 0 = toggle, 1 = up/down, 2 = pwm, 3 = set voltage, 4 = input
  int dacChosen = -1;
  int adcChosen = -1;
  int gpioChosen = -1;
  connectedRowsIndex = 0;
  connectedRows[0] = -1;
  node1or2 = 0;
  unsigned long skipTimer = millis();
inPadMenu = 1;
  b.clear();
  clearLEDsExceptRails();
  // showLEDsCore2 = 2;
  //   lastReadRaw = 0;
  b.print("DAC", sfOptionColors[0], 0xFFFFFF, 0, 0, -1);
  b.print("ADC", sfOptionColors[1], 0xFFFFFF, 4, 0, 0);
  b.print("GPIO", sfOptionColors[2], 0xFFFFFF, 8, 1, 1);

  int selected = -1;

  while (selected == -1 && longShortPress(500) != 1 &&
         longShortPress(500) != 2) {
    int reading = readProbe();
    if (reading != -1) {
      switch (reading) {
      case 1 ... 13: {
        selected = 0;
        functionSetting = 0;
        dacChosen = chooseDAC(1);
        Serial.print("dacChosen: ");
        Serial.println(dacChosen);
        // b.clear();
        settingOption = dacChosen - DAC0;
        clearLEDsExceptRails();
        // showLEDsCore2 = 1;

        break;
      }
      case 18 ... 30: {
        selected = 1;
        functionSetting = 1;
        adcChosen = chooseADC();
        Serial.print("adcChosen: ");
        Serial.println(adcChosen);
        settingOption = adcChosen - ADC0;

        // b.clear();
        clearLEDsExceptRails();
        // showLEDsCore2 = 1;

        break;
      }
      case 37 ... 53: {
        selected = 2;
        functionSetting = 2;
        // b.clear();
        clearLEDsExceptRails();
        // showLEDsCore2 = 2;

        gpioChosen = chooseGPIO(1);
        // b.clear();
        clearLEDsExceptRails();
        // showLEDsCore2 = 2;
        if (gpioChosen >= 122 && gpioChosen <= 125) {
          gpioChosen = gpioChosen - 122 + 5;
        } else if (gpioChosen >= 135 && gpioChosen <= 138) {
          gpioChosen = gpioChosen - 134;
        }
        if (gpioState[gpioChosen] != 0) {
          clearLEDsExceptRails();
         // showLEDsCore2 = 2;
          Serial.print("Set GP");
          Serial.print(gpioChosen);
          Serial.println(" to Output");
          char gpString[4];
          itoa(gpioChosen, gpString, 10);

          b.print("GPIO", sfOptionColors[(gpioChosen + 1) % 7], 0xFFFFFF, 0, 0,
                  0);
          b.print(gpString, sfOptionColors[gpioChosen - 1], 0xFFFFFF, 4, 0, 3);
          // b.print(" ", sfOptionColors[0], 0xFFFFFF, 0, 1, -2);
          b.print("Output", sfOptionColors[(gpioChosen + 3) % 7], 0xFFFFFF, 1,
                  1, 1);
          b.printRawRow(0b00000100, 31, 0x200010, 0xffffff);
          b.printRawRow(0b00000100, 32, 0x200010, 0xffffff);
          b.printRawRow(0b00010101, 33, 0x200010, 0xffffff);
          b.printRawRow(0b00001110, 34, 0x200010, 0xffffff);
          b.printRawRow(0b00000100, 35, 0x200010, 0xffffff);
         // showLEDsCore2 = 2;
          delayWithButton(400);

        } else {
        }
        gpioState[gpioChosen] = 0;
        settingOption = gpioChosen;
        setGPIO();
        clearLEDsExceptRails();

        //showLEDsCore2 = 2;
        b.print("Tap to", sfOptionColors[(gpioChosen + 1) % 7], 0xFFFFFF, 0, 0,
                1);
        b.print("toggle", sfOptionColors[(gpioChosen + 2) % 7], 0xFFFFFF, 0, 1,
                1);
        delayWithButton(500);
        clearLEDsExceptRails();
        // showLEDsCore2 = 1;
        // inPadMenu = 0;

        break;
      }
      }
    }
  }
  // inPadMenu = 0;
  Serial.print("pad: ");
  Serial.println(pad);
  Serial.print("functionSetting: ");
  Serial.println(functionSetting);
  Serial.print("settingOption: ");
  Serial.println(settingOption);
  switch (functionSetting) {
  case 2: {
    switch (gpioChosen) {
    case 1: {
      function = 135;
      break;
    }
    case 2: {
      function = 136;
      break;
    }
    case 3: {
      function = 137;
      break;
    }
    case 4: {
      function = 138;
      break;
    }
    case 5: {
      function = 122;
      break;
    }
    case 6: {
      function = 123;
      break;
    }
    case 7: {
      function = 124;
      break;
    }
    case 8: {
      function = 125;
      break;
    }
    }
    break;
  }
  case 1: {
    function = adcChosen;
    break;
  }
  case 0: {
    function = dacChosen;
    break;
  }
  }

  switch (pad) {
  case 128: {
    logoTopSetting[0] = functionSetting;
    logoTopSetting[1] = settingOption;

    break;
  }
  case 129: {
    logoBottomSetting[0] = functionSetting;
    logoBottomSetting[1] = settingOption;

    break;
  }
  case 133: {
    buildingTopSetting[0] = functionSetting;
    buildingTopSetting[1] = settingOption;

    break;
  }
  case 134: {
    buildingBottomSetting[0] = functionSetting;
    buildingBottomSetting[1] = settingOption;

    break;
  }
  }
  saveLogoBindings();
  delay(3);
   inPadMenu = 0;
  showLEDsCore2 = 1;
  return function;
}

int delayWithButton(int delayTime) {
  unsigned long skipTimer = millis();
  while (millis() - skipTimer < delayTime) {
    int reading = checkProbeButton();
    if (reading == 1) {
      return 1;
    } else if (reading == 2) {
      return 2;
    }
  }
  return 0;
}

int chooseDAC(int justPickOne) {
  int function = -1;
  // b.clear();
  clearLEDsExceptRails();
  showLEDsCore2 = 2;


  // lastReadRaw = 0;
  b.print("DAC", scaleDownBrightness(rawOtherColors[9], 4, 22), 0xFFFFFF, 1, 0,
          3);

  b.print("1", sfOptionColors[0], 0xFFFFFF, 0, 1, 6);
  b.print("5v", sfOptionColors[0], 0xFFFFFF, 0, 0, -2);
  b.printRawRow(0b00011000, 31, sfOptionColors[7], 0xffffff);
  b.printRawRow(0b00000100, 32, sfOptionColors[7], 0xffffff);
  b.printRawRow(0b00000100, 33, sfOptionColors[7], 0xffffff);
  b.printRawRow(0b00010101, 34, sfOptionColors[7], 0xffffff);
  b.printRawRow(0b00001110, 35, sfOptionColors[7], 0xffffff);
  b.printRawRow(0b00000100, 36, sfOptionColors[7], 0xffffff);
  // b.printRawRow(0b00011100, 32,sfOptionColors[0], 0xffffff);
  //  b.printRawRow(0b00011100, 33,sfOptionColors[0], 0xffffff);

  b.print("2", sfOptionColors[2], 0xFFFFFF, 4, 1, 1);
  b.print("8v", sfOptionColors[2], 0xFFFFFF, 5, 0, 1);
  b.printRawRow(0b00011000, 58, sfOptionColors[4], 0xffffff);
  b.printRawRow(0b00000100, 57, sfOptionColors[4], 0xffffff);
  b.printRawRow(0b00000100, 56, sfOptionColors[4], 0xffffff);
  b.printRawRow(0b00010101, 55, sfOptionColors[4], 0xffffff);
  b.printRawRow(0b00001110, 54, sfOptionColors[4], 0xffffff);
  b.printRawRow(0b00000100, 53, sfOptionColors[4], 0xffffff);

  sfProbeMenu = 2;

  int selected = -1;
  function = 0;
  while (selected == -1 && longShortPress(500) == -1) {
    int reading = readProbe();
    if (reading != -1) {
      switch (reading) {
      case 37 ... 43: {
        selected = 106;
        function = 106;
        if (justPickOne == 1) {
          return function;
        }
        setDac0_5Vvoltage(voltageSelect(5));
        // showNets();
        showLEDsCore2 = 1;
        delay(100);

        break;
      }
      case 48 ... 54: {
        selected = 107;
        function = 107;
        if (justPickOne == 1) {
          return function;
          // break;
        }
        setDac1_8Vvoltage(voltageSelect(8));
        // showNets();
        showLEDsCore2 = 1;
        delay(100);
        break;
      }
      }
    }
  }

  return function;
}
int chooseADC(void) {
  int function = -1;
  // b.clear();
  clearLEDsExceptRails();
  // lastReadRaw = 0;
  // showLEDsCore2 = 2;
  b.print(" ADC", scaleDownBrightness(rawOtherColors[8], 4, 22), 0xFFFFFF, 0, 0,
          3);
  // sfProbeMenu = 1;
  //  delay(1000);
  //  function = 111;
  b.print("1", sfOptionColors[0], 0xFFFFFF, 0, 1, 3);
  b.print("2", sfOptionColors[1], 0xFFFFFF, 2, 1, 3);
  b.print("3", sfOptionColors[2], 0xFFFFFF, 4, 1, 3);
  int selected = -1;
  while (selected == -1 && longShortPress(500) != 1) {
    int reading = readProbe();
    // Serial.print("reading: ");
    // Serial.println(reading);
    if (reading != -1) {
      switch (reading) {
      case 34 ... 40: {
        selected = 111;
        function = 111;

        break;
      }
      case 41 ... 48: {
        selected = 112;
        function = 112;
        break;
      }
      case 50 ... 56: {
        selected = 113;
        function = 113;
        break;
      }
      }
    }
  }

  clearLEDsExceptRails();
  // showNets();
  // showLEDsCore2 = 1;
  return function;
}

int chooseGPIOinputOutput(int gpioChosen) {
  int settingOption = -1;

  b.print("Input", sfOptionColors[gpioChosen + 1 % 7], 0xFFFFFF, 1, 0, 3);
  b.print(gpioChosen, sfOptionColors[gpioChosen - 1], 0xFFFFFF, 0, 0, -2);
  b.print("Output", sfOptionColors[gpioChosen % 7], 0xFFFFFF, 0, 1, 3);

  showLEDsCore2 = 2;

  Serial.print("gpioChosen: ");
  Serial.println(gpioChosen);

  delay(100);

  while (settingOption == -1 && longShortPress(500) != 1) {
    int reading = readProbe();
    if (reading != -1) {
      switch (reading) {
      case 9 ... 29: {
        gpioState[gpioChosen - 1] = 2;
        settingOption = 4;
        break;
      }
      case 35 ... 59: {
        gpioState[gpioChosen - 1] = 1;
        settingOption = 0;
        break;
      }
      }
    }
  }

  // clearLEDsExceptRails();
  // showNets();
  // showLEDsCore2 = 1;
  return settingOption;
}

int chooseGPIO(int skipInputOutput) {
  int function = -1;

  b.clear();
  clearLEDsExceptRails();
  showLEDsCore2 = 2;
  sfProbeMenu = 3;
  // lastReadRaw = 0;
  b.print("3v", 0x0f0002, 0xFFFFFF, 0, 0, -2);
  b.print("1", sfOptionColors[0], 0xFFFFFF, 2, 0, 1);
  b.print("2", sfOptionColors[1], 0xFFFFFF, 3, 0, 2);
  b.print("3", sfOptionColors[2], 0xFFFFFF, 4, 0, 3);
  b.print("4", sfOptionColors[3], 0xFFFFFF, 5, 0, 4);
  b.print("5v", 0x0f0200, 0xFFFFFF, 0, 1, -2);
  b.print("5", sfOptionColors[4], 0xFFFFFF, 2, 1, 1);
  b.print("6", sfOptionColors[5], 0xFFFFFF, 3, 1, 2);
  b.print("7", sfOptionColors[6], 0xFFFFFF, 4, 1, 3);
  b.print("8", sfOptionColors[7], 0xFFFFFF, 5, 1, 4);
  int selected = -1;
  delayWithButton(300);
  while (selected == -1 && longShortPress(500) != 1) {
    int reading = readProbe();
    if (reading != -1) {
      switch (reading) {
      case 11 ... 15: {
        selected = 135;
        function = 135;
        break;
      }
      case 16 ... 20: {
        selected = 136;
        function = 136;
        break;
      }
      case 21 ... 25: {
        selected = 137;
        function = 137;
        break;
      }
      case 26 ... 30: {
        selected = 138;
        function = 138;
        break;
      }
      case 41 ... 45: {
        selected = 122;
        function = 122;
        break;
      }
      case 46 ... 50: {
        selected = 123;
        function = 123;
        break;
      }
      case 51 ... 55: {
        selected = 124;
        function = 124;
        break;
      }
      case 56 ... 60: {
        selected = 125;
        function = 125;
        break;
      }
      }
    }
  }
  if (skipInputOutput == 0) {

    int gpioChosen = -1;

    switch (function) {
    case 135 ... 138: {
      gpioChosen = function - 134;
      break;
    }
    case 122 ... 125: {
      gpioChosen = function - 117;
      break;
    }
    }
    Serial.print("gpioChosen: ");
    Serial.println(gpioChosen);
    clearLEDsExceptRails();
    chooseGPIOinputOutput(gpioChosen);
  }
  clearLEDsExceptRails();
  // showNets();
  showLEDsCore2 = 1;
  return function;
}

float voltageSelect(int fiveOrEight) {
  float voltageProbe = 0.0;
  uint32_t color = 0x000000;
  if (fiveOrEight == 5) {

    b.clear();
    clearLEDsExceptRails();

    uint8_t step = 0b0000000;
    for (int i = 31; i <= 60; i++) {
      if ((i - 1) % 6 == 0) {
        step = step << 1;
        step = step | 0b00000001;
      }

      b.printRawRow(step, i - 1, logoColors8vSelect[(i - 31) * 2], 0xffffff);
    }
    // b.print("Set", scaleDownBrightness(rawOtherColors[9], 4, 22),
    //         0xFFFFFF, 1, 0, 3);
    b.print("Set", scaleDownBrightness(rawOtherColors[9], 4, 22), 0xFFFFFF, 1,
            0, 3);
    b.print("0v", sfOptionColors[7], 0xFFFFFF, 0, 0, -2);
    b.print("5v", sfOptionColors[7], 0xFFFFFF, 5, 0, 1);
    int vSelected = -1;
    int encoderReadingPos = 45;
    rotaryDivider = 4;
    while (vSelected == -1) {
      int reading = readProbe();
      // rotaryEncoderStuff();
      int encodeEdit = 0;
      if (encoderDirectionState == UP || reading == -19) {
        encoderDirectionState = NONE;
        voltageProbe = voltageProbe + 0.1;
        encodeEdit = 1;
        // Serial.println(reading);

      } else if (encoderDirectionState == DOWN || reading == -17) {
        encoderDirectionState = NONE;
        voltageProbe = voltageProbe - 0.1;

        encodeEdit = 1;
        // Serial.println(voltageProbe);

      } else if (encoderButtonState == PRESSED &&
                     lastButtonEncoderState == IDLE ||
                 reading == -10) {
        encodeEdit = 1;
        encoderButtonState = IDLE;
        vSelected = 10;
      }
      if (voltageProbe < 0.0) {
        voltageProbe = 0.0;
      } else if (voltageProbe > 5.0) {
        voltageProbe = 5.0;
      }
      // Serial.println(reading);
      if (reading > 0 && reading >= 31 && reading <= 60 || encodeEdit == 1) {
        //
        b.clear(1);

        char voltageString[7] = " 0.0 V";

        if (voltageProbe < 0.0) {
          voltageProbe = 0.0;
        } else if (voltageProbe > 5.0) {
          voltageProbe = 5.0;
        }

        if (encodeEdit == 0) {
          voltageProbe = (reading - 31) * (5.0 / 29);

        } else {
          reading = 31 + (voltageProbe + 8.0) * (29.0 / 16.0);
        }
        // Serial.println(voltageProbe);
        color = logoColors8vSelect[(reading - 31) * 2];

        snprintf(voltageString, 7, "%0.1f v", voltageProbe);
        b.print(voltageString, color, 0xFFFFFF, 0, 1, 3);
        showLEDsCore2 = 2;
        delay(10);
      }
      if (checkProbeButton() > 0 || vSelected == 10) {
        // Serial.println("button\n\r");

        rawSpecialNetColors[4] = color;
        rgbColor rg = unpackRgb(color);
        specialNetColors[4].r = rg.r;
        specialNetColors[4].g = rg.g;
        specialNetColors[4].b = rg.b;
        b.clear();
        // clearLEDsExceptRails();
        // showLEDsCore2 = 1;
        if (vSelected != 10) {
          vSelected = 1;
        } else {
          vSelected = 10;
          Serial.println("encoder button\n\r");
          delay(500);
        }
        // if (checkProbeButton() == 2) {
        //   vSelected = 10;
        // }
        vSelected = 1;
        return voltageProbe;
        showLEDsCore2 = 1;
        break;
      }
    }

    // b.clear();
    // clearLEDsExceptRails();

    // uint8_t step = 0b00000001;
    // for (int i = 32; i <= 60; i++) {
    //   if (i % 6 == 0) {
    //     step = step << 1;
    //     step = step | 0b00000001;
    //   }

    //   b.printRawRow(step, i - 1, logoColors[((60 - i) % LOGO_COLOR_LENGTH)],
    //                 0xffffff);
    // }
    // b.print("Set", scaleDownBrightness(rawOtherColors[9], 4, 22), 0xFFFFFF,
    // 1,
    //         0, 3);
    // b.print("0v", sfOptionColors[7], 0xFFFFFF, 0, 0, -2);
    // b.print("5v", sfOptionColors[7], 0xFFFFFF, 5, 0, 1);
    // int vSelected = -1;

    // while (vSelected == -1) {
    //   int reading = readProbe();

    //   if (reading > 0 && reading >= 31 && reading <= 60) {
    //     // Serial.println(reading);
    //     b.clear(1);

    //     char voltageString[7] = " 0.0 V";
    //     voltageProbe = (reading - 31) * (5.0 / 29);
    //     // Serial.println(voltageProbe);
    //     color = logoColors[((60 - reading)) % LOGO_COLOR_LENGTH];

    //     snprintf(voltageString, 7, "+%0.1f v", voltageProbe);
    //     b.print(voltageString, logoColors[((60 - reading)) %
    //     LOGO_COLOR_LENGTH],
    //             0xFFFFFF, 0, 1, 3);
    //     showLEDsCore2 = 2;
    //     delay(10);
    //   }
    //   if (checkProbeButton() == 1) {
    //     ///Serial.println("button\n\r");
    //     vSelected = 1;
    //     rawSpecialNetColors[4] = color;
    //     rgbColor rg = unpackRgb(color);
    //     specialNetColors[4].r = rg.r;
    //     specialNetColors[4].g = rg.g;
    //     specialNetColors[4].b = rg.b;
    //     b.clear();
    //     // clearLEDsExceptRails();
    //     // showLEDsCore2 = 1;
    //     return voltageProbe;
    //     break;
    //   }
    // }
  } else if (fiveOrEight == 8) {
    b.clear();
    clearLEDsExceptRails();

    uint8_t step = 0b00011111;
    for (int i = 31; i <= 60; i++) {
      if ((i - 1) % 3 == 0 && i < 46 && i > 32) {
        step = step >> 1;
        step = step & 0b01111111;

      } else if ((i) % 3 == 1 && i > 46) {
        step = step << 1;
        step = step | 0b00000001;
      }

      b.printRawRow(step, i - 1, logoColors8vSelect[(i - 31) * 2], 0xffffff);
    }
    // b.print("Set", scaleDownBrightness(rawOtherColors[9], 4, 22),
    //         0xFFFFFF, 1, 0, 3);
    b.print("-8v", sfOptionColors[0], 0xFFFFFF, 0, 0, -2);
    b.print("+8v", sfOptionColors[1], 0xFFFFFF, 4, 0, 1);
    int vSelected = -1;
    int encoderReadingPos = 45;
    rotaryDivider = 4;
    while (vSelected == -1) {
      int reading = readProbe();
      // rotaryEncoderStuff();
      int encodeEdit = 0;
      if (encoderDirectionState == UP || reading == -19) {
        encoderDirectionState = NONE;
        voltageProbe = voltageProbe + 0.1;
        encodeEdit = 1;
        // Serial.println(reading);

      } else if (encoderDirectionState == DOWN || reading == -17) {
        encoderDirectionState = NONE;
        voltageProbe = voltageProbe - 0.1;
        encodeEdit = 1;
        // Serial.println(voltageProbe);

      } else if (encoderButtonState == PRESSED &&
                     lastButtonEncoderState == IDLE ||
                 reading == -10) {
        encodeEdit = 1;
        encoderButtonState = IDLE;
        vSelected = 10;
      }
      // Serial.println(reading);
      if (reading > 0 && reading >= 31 && reading <= 60 || encodeEdit == 1) {
        //
        b.clear(1);

        char voltageString[7] = " 0.0 V";

        if (voltageProbe < -8.0) {
          voltageProbe = -8.0;
        } else if (voltageProbe > 8.0) {
          voltageProbe = 8.0;
        }

        if (encodeEdit == 0) {
          voltageProbe = (reading - 31) * (16.0 / 29);
          voltageProbe = voltageProbe - 8.0;
          if (voltageProbe < 0.4 && voltageProbe > -0.4) {
            voltageProbe = 0.0;
          }
        } else {
          reading = 31 + (voltageProbe + 8.0) * (29.0 / 16.0);
        }
        // Serial.println(voltageProbe);
        color = logoColors8vSelect[(reading - 31) * 2];

        snprintf(voltageString, 7, "%0.1f v", voltageProbe);
        b.print(voltageString, color, 0xFFFFFF, 0, 1, 3);
        showLEDsCore2 = 2;
        delay(10);
      }
      if (checkProbeButton() == 1 || vSelected == 10) {
        // Serial.println("button\n\r");

        rawSpecialNetColors[4] = color;
        rgbColor rg = unpackRgb(color);
        specialNetColors[4].r = rg.r;
        specialNetColors[4].g = rg.g;
        specialNetColors[4].b = rg.b;
        b.clear();
        // clearLEDsExceptRails();
        // showLEDsCore2 = 1;
        if (vSelected != 10) {
          vSelected = 1;
        } else {
          vSelected = 10;
          Serial.println("encoder button\n\r");
          delay(500);
        }
        vSelected = 1;
        showLEDsCore2 = 1;
        return voltageProbe;
        break;
      }
    }
  }
  return 0.0;
}

void routableBufferPower(int offOn) {
  if (offOn == 1) {
    Serial.println("power on\n\r");
    // delay(10);
    removeBridgeFromNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_23, netSlot, 1);

    delay(10);
    // //  removeBridgeFromNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_22, netSlot);
    //  //delay(10);
    addBridgeToNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_23, netSlot, 1);
    delay(10);
    // // addBridgeToNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_22, netSlot);
    // // delay(10);
    // // pinMode(22, OUTPUT_12MA);
    // // digitalWrite(22, HIGH);
    pinMode(23, OUTPUT_12MA);
    digitalWrite(23, HIGH);

    // delay(10);
    // clearAllNTCC();
    // openNodeFile(netSlot);
    // getNodesToConnect();
    // // Serial.print("openNF\n\r");

    // bridgesToPaths();
    // // clearLEDsExceptRails();
    // //assignNetColors();
    // //showNets();
    // delay(30);
    // sendAllPathsCore2 = 1;
    // delay(30);

  } else {
    Serial.println("power off\n\r");
    /// removeBridgeFromNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_22, netSlot);
    delay(10);
    removeBridgeFromNodeFile(ROUTABLE_BUFFER_IN, RP_GPIO_23, netSlot, 1);
    delay(100);
    // // pinMode(22, INPUT);
    // pinMode(23, INPUT);

    // openNodeFile(netSlot);
    // getNodesToConnect();
    // bridgesToPaths();
    // sendAllPathsCore2 = 1;
  }
}

int selectFromLastFound(void) {

  rawOtherColors[1] = 0x0010ff;

  blinkTimer = 0;
  int selected = 0;
  int selectionConfirmed = 0;
  int selected2 = connectedRows[selected];
  Serial.print("\n\r");
  Serial.print("      multiple nodes found\n\n\r");
  Serial.print("  short press = cycle through nodes\n\r");
  Serial.print("  long press  = select\n\r");

  Serial.print("\n\r ");
  for (int i = 0; i < connectedRowsIndex; i++) {

    printNodeOrName(connectedRows[i]);
    if (i < connectedRowsIndex - 1) {
      Serial.print(", ");
    }
  }
  Serial.print("\n\n\r");
  delay(10);

  uint32_t previousColor[connectedRowsIndex];

  for (int i = 0; i < connectedRowsIndex; i++) {
    previousColor[i] = leds.getPixelColor(nodesToPixelMap[connectedRows[i]]);
  }
  int lastSelected = -1;

  while (selectionConfirmed == 0) {
    probeTimeout = millis();
    // if (millis() - blinkTimer > 100)
    // {
    if (lastSelected != selected && selectionConfirmed == 0) {
      for (int i = 0; i < connectedRowsIndex; i++) {
        if (i == selected) {
          leds.setPixelColor(nodesToPixelMap[connectedRows[i]],
                             rainbowList[1][0], rainbowList[1][1],
                             rainbowList[1][2]);
        } else {
          // uint32_t previousColor =
          // leds.getPixelColor(nodesToPixelMap[connectedRows[i]]);
          if (previousColor[i] != 0) {
            int r = (previousColor[i] >> 16) & 0xFF;
            int g = (previousColor[i] >> 8) & 0xFF;
            int b = (previousColor[i] >> 0) & 0xFF;
            leds.setPixelColor(nodesToPixelMap[connectedRows[i]], (r / 4) + 5,
                               (g / 4) + 5, (b / 4) + 5);
          } else {

            leds.setPixelColor(nodesToPixelMap[connectedRows[i]],
                               rainbowList[1][0] / 8, rainbowList[1][1] / 8,
                               rainbowList[1][2] / 8);
          }
        }
      }
      lastSelected = selected;

      Serial.print(" \r");
      // Serial.print("");
      printNodeOrName(connectedRows[selected]);
      Serial.print("  ");
    }
    // leds.show();
    showLEDsCore2 = 2;
    blinkTimer = millis();
    //  }
    delay(30);
    int longShort = longShortPress();
    delay(5);
    if (longShort == 1) {
      selectionConfirmed = 1;
      // for (int i = 0; i < connectedRowsIndex; i++)
      // {
      //     if (i == selected)
      //     // if (0)
      //     {
      //         leds.setPixelColor(nodesToPixelMap[connectedRows[i]],
      //         rainbowList[rainbowIndex][0], rainbowList[rainbowIndex][1],
      //         rainbowList[rainbowIndex][2]);
      //     }
      //     else
      //     {
      //         leds.setPixelColor(nodesToPixelMap[connectedRows[i]], 0, 0, 0);
      //     }
      // }
      // showLEDsCore2 = 1;
      // selected = lastFound[node1or2][selected];
      //  clearLastFound();

      // delay(500);
      selected2 = connectedRows[selected];
      // return selected2;
      break;
    } else if (longShort == 0) {

      selected++;
      blinkTimer = 0;

      if (selected >= connectedRowsIndex) {

        selected = 0;
      }
      // delay(100);
    }
    delay(15);
    //  }
    //}

    // showLEDsCore2 = 1;
  }
  selected2 = connectedRows[selected];

  for (int i = 0; i < connectedRowsIndex; i++) {
    if (i == selected) {
      leds.setPixelColor(nodesToPixelMap[connectedRows[selected]],
                         rainbowList[0][0], rainbowList[0][1],
                         rainbowList[0][2]);
    } else if (previousColor[i] != 0) {

      int r = (previousColor[i] >> 16) & 0xFF;
      int g = (previousColor[i] >> 8) & 0xFF;
      int b = (previousColor[i] >> 0) & 0xFF;
      leds.setPixelColor(nodesToPixelMap[connectedRows[i]], r, g, b);
    } else {

      leds.setPixelColor(nodesToPixelMap[connectedRows[i]], 0, 0, 0);
    }
  }

  // leds.setPixelColor(nodesToPixelMap[selected2], rainbowList[0][0],
  // rainbowList[0][1], rainbowList[0][2]); leds.show(); showLEDsCore2 = 1;
  probeButtonTimer = millis();
  // connectedRowsIndex = 0;
  justSelectedConnectedNodes = 1;
  return selected2;
}

int longShortPress(int pressLength) {
  int longShort = 0;
  unsigned long clickTimer = 0;

  clickTimer = millis();
  int buttonState = checkProbeButton();
  if (buttonState > 0) {

    while (millis() - clickTimer < pressLength) {
      if (checkProbeButton() == 0) {
        // Serial.print("buttonState: ");
        // Serial.println(buttonState);
        return buttonState;
      }
      delay(5);
    }
  } else {
    return -1;
  }
  // Serial.print("buttonState: ");
  // Serial.println(buttonState);
  return buttonState;
}
int countLED = 0;
volatile int checkingButton = 0;

int checkProbeButton(void) {
  int buttonState = 0;
  int buttonState2 = 0;
  checkingButton = 1;

  //     probeLEDs.setPixelColor(0, 0x000000);
  //   probeLEDs.show();
  // probeLEDs.setPin(-1);
  // delayMicroseconds(100);
  // pinMode(2, OUTPUT);
  // digitalWrite(2, HIGH);
  //  pinMode(9, OUTPUT);
  // digitalWrite(9, HIGH);
  // delay(10);
  // digitalWrite(9, LOW);
  // delay(10);

  pinMode(9, INPUT_PULLDOWN);
  // pinMode(10, OUTPUT_12MA);

  digitalWrite(10, HIGH);

  delayMicroseconds(500);

  buttonState = digitalRead(9);

  delayMicroseconds(500);

  pinMode(9, INPUT_PULLUP);
  delayMicroseconds(500);
  buttonState2 = digitalRead(9);

  // pinMode(9, INPUT);

  // pinMode(9, OUTPUT);
  pinMode(9, OUTPUT_12MA);
  digitalWrite(9, LOW);
  // probeLEDs.setPin(9);
  // probeLEDs.begin();
  // probeLEDs.setPixelColor(0, 0x000000);
  // probeLEDs.show();
  // delayMicroseconds(300);
  checkingButton = 0;
  // switch(countLED){
  //   case 0:
  //   probeLEDs.setPixelColor(0, 0x000008);
  //   probeLEDs.show();
  //   countLED ++;
  //   break;
  //   case 1:
  //   probeLEDs.setPixelColor(0, 0x000800);
  //   probeLEDs.show();
  //   countLED ++;
  //   break;
  //   case 2:
  //   probeLEDs.setPixelColor(0, 0x080000);
  //   probeLEDs.show();
  //   countLED = 0;
  // }

  // probeLEDs.begin();
  // probeLEDs.clear();
  delayMicroseconds(500);
  probeLEDs.show();

  // if (buttonState == 1) {
  //      Serial.print("buttonState");

  //    Serial.println(buttonState);
  // }
  if (buttonState == 1 && buttonState2 == 0) { // disconnect Button
    //   Serial.print("buttonState ");
    // Serial.println(buttonState);
    // Serial.print("buttonState2 ");
    // Serial.println(buttonState2);
    // Serial.println(" ");
    return 1;
  } else if (buttonState == 0 && buttonState2 == 0) { // connect Button
    // Serial.print("buttonState ");
    // Serial.println(buttonState);
    // Serial.print("buttonState2 ");
    // Serial.println(buttonState2);
    // Serial.println(" ");
    return 2;
  }
  return buttonState;
}

int readFloatingOrState(int pin, int rowBeingScanned) {
  // return 0;
  enum measuredState state = unknownState;
  // enum measuredState state2 = floating;

  int readingPullup = 0;
  int readingPullup2 = 0;
  int readingPullup3 = 0;

  int readingPulldown = 0;
  int readingPulldown2 = 0;
  int readingPulldown3 = 0;

  // pinMode(pin, INPUT_PULLUP);

  if (rowBeingScanned != -1) {

    analogWrite(probePin, 128);

    while (1) // this is the silliest way to align to the falling edge of the
              // probe PWM signal
    {
      if (gpio_get(probePin) != 0) {
        if (gpio_get(probePin) == 0) {
          break;
        }
      }
    }
  }

  delayMicroseconds((probeHalfPeriodus * 5) + (probeHalfPeriodus / 2));

  readingPullup = digitalRead(pin);
  delayMicroseconds(probeHalfPeriodus * 3);
  readingPullup2 = digitalRead(pin);
  delayMicroseconds(probeHalfPeriodus * 1);
  readingPullup3 = digitalRead(pin);

  // pinMode(pin, INPUT_PULLDOWN);

  if (rowBeingScanned != -1) {
    while (1) // this is the silliest way to align to the falling edge of the
              // probe PWM signal
    {
      if (gpio_get(probePin) != 0) {
        if (gpio_get(probePin) == 0) {
          break;
        }
      }
    }
  }

  delayMicroseconds((probeHalfPeriodus * 5) + (probeHalfPeriodus / 2));

  readingPulldown = digitalRead(pin);
  delayMicroseconds(probeHalfPeriodus * 3);
  readingPulldown2 = digitalRead(pin);
  delayMicroseconds(probeHalfPeriodus * 1);
  readingPulldown3 = digitalRead(pin);

  // if (readingPullup == 0 && readingPullup2 == 1 && readingPullup3 == 0 &&
  // readingPulldown == 1 && readingPulldown2 == 0 && readingPulldown3 == 1)
  // {
  //     state = probe;
  // }

  if ((readingPullup != readingPullup2 || readingPullup2 != readingPullup3) &&
      (readingPulldown != readingPulldown2 ||
       readingPulldown2 != readingPulldown3) &&
      rowBeingScanned != -1) {
    state = probe;

    // if (readingPulldown != readingPulldown2 || readingPulldown2 !=
    // readingPulldown3)
    // {
    //     state = probe;

    // } else
    // {
    //     Serial.print("!");
    // }
  } else {

    if (readingPullup2 == 1 && readingPulldown2 == 0) {

      state = floating;
    } else if (readingPullup2 == 1 && readingPulldown2 == 1) {
      //              Serial.print(readingPullup);
      // // Serial.print(readingPullup2);
      // // Serial.print(readingPullup3);
      // // //Serial.print(" ");
      //  Serial.print(readingPulldown);
      // // Serial.print(readingPulldown2);
      // // Serial.print(readingPulldown3);
      //  Serial.print("\n\r");

      state = high;
    } else if (readingPullup2 == 0 && readingPulldown2 == 0) {
      //  Serial.print(readingPullup);
      // // Serial.print(readingPullup2);
      // // Serial.print(readingPullup3);
      // // //Serial.print(" ");
      //  Serial.print(readingPulldown);
      // // Serial.print(readingPulldown2);
      // // Serial.print(readingPulldown3);
      //  Serial.print("\n\r");
      state = low;
    } else if (readingPullup == 0 && readingPulldown == 1) {
      // Serial.print("shorted");
    }
  }

  // Serial.print("\n");
  // showLEDsCore2 = 1;
  // leds.show();
  // delayMicroseconds(100);

  return state;
}

void startProbe(long probeSpeed) {

  pinMode(probePin, OUTPUT_12MA);
  // pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(ADC0_PIN, INPUT);
  digitalWrite(probePin, HIGH);
}

void stopProbe() {
  pinMode(probePin, INPUT);
  // pinMode(buttonPin, INPUT);
}

int checkLastFound(int found) {
  int found2 = 0;
  return found2;
}

void clearLastFound() {}

int probeADCmap[102];

int nothingTouchedReading = 35;
int calibrateProbe() {
  /* clang-format off */

  int probeRowMap[102] = {

      0,	      1,	      2,	      3,	      4,	      5,	      6,	      7,	      8,
      9,	      10,	      11,	      12,	      13,	      14,	      15,	      16,	      17,
      18,	      19,	      20,	      21,	      22,	      23,	      24,	      25,	      26,
      27,	      28,	      29,	      30,	      TOP_RAIL,	      TOP_RAIL_GND,	      BOTTOM_RAIL,	      BOTTOM_RAIL_GND,
      31,	      32,	      33,	      34,	      35,	      36,	      37,	      38,	      39,
      40,	      41,	      42,	      43,	      44,	      45,	      46,	      47,	      48,
      49,	      50,	      51,	      52,	      53,	      54,	      55,	      56,	      57,
      58,	      59,	      60,	      NANO_D1,	      NANO_D0,	      NANO_RESET_1,	      NANO_GND_1,	      NANO_D2,	      NANO_D3,
      NANO_D4,	      NANO_D5,	      NANO_D6,	      NANO_D7,	      NANO_D8,	      NANO_D9,	      NANO_D10,	      NANO_D11,	      NANO_D12,
      NANO_D13,	      NANO_3V3,	      NANO_AREF,	      NANO_A0,	      NANO_A1,	      NANO_A2,	      NANO_A3,	      NANO_A4,	      NANO_A5,
      NANO_A6,	      NANO_A7,	      NANO_5V,	      NANO_RESET_0,	      NANO_GND_0,	      NANO_VIN,	
      LOGO_PAD_BOTTOM,	      LOGO_PAD_TOP,	      GPIO_PAD,
      DAC_PAD,	      ADC_PAD,	      BUILDING_PAD_TOP,	      BUILDING_PAD_BOTTOM,
  };
  /* clang-format on */
  while (1) {

    int nothingTouchedAverage = 0;
    for (int i = 0; i < 6; i++) {
      int reading = readProbeRaw(1);
      while (reading == -1) {
        reading = readProbeRaw();
      }
      nothingTouchedAverage += readProbeRaw();
      delay(10);
    }

    nothingTouchedReading = nothingTouchedAverage / 6;
    // nothingTouchedReading = readProbeRaw();
    Serial.print("nothingTouchedReading: ");
    Serial.println(nothingTouchedReading);

    int rowProbedCalibration = 0;

    while (1) {

      // int probeRead = 0;

      // delay(10);

      int probeRead = readProbeRaw();
      while (probeRead == -1) {
        probeRead = readProbeRaw();
      }
      // Serial.println(probeRead);
      //  Serial.print("mapped: ");
      //  Serial.print(probeRead);
      //  Serial.print(" -> ");
      //  Serial.print(nothingTouchedReading);
      //  Serial.print(" - ");
      //  Serial.print(probeRead1);
      //  Serial.print("\n\n\r");

      int mappedRead = map(probeRead, 100, 4055, 100, 0);

      if (mappedRead < 0) {
        mappedRead = 0;
      }
      if (mappedRead > 101) {
        mappedRead = 101;
      }
      // delay(10);
      rowProbedCalibration = probeRowMap[mappedRead];

      // Serial.print(rowProbedCalibration);
      // Serial.print(" ");
      // Serial.print(mappedRead);
      //   Serial.print("\t\t");
      // Serial.println(nothingTouchedReading);
      // Serial.println();
      // delay(200);
      if (checkProbeButton() == 1) {
        break;
      }
      if (rowProbedCalibration > 0 && rowProbedCalibration <= 60) {
        b.clear();

        b.printRawRow(0b00011111, rowProbedCalibration - 1, 0x4500e8, 0xffffff);
        leds.show();

        // showLEDsCore2 = 1;

      } else if (rowProbedCalibration >= NANO_D0 &&
                 rowProbedCalibration < NANO_RESET_1) {
        b.clear();

        for (int i = 0; i < 35; i++) {
          if (bbPixelToNodesMapV5[i][0] == rowProbedCalibration) {
            leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x4500e8);
            leds.show();
            // break;
          } else {
            leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
          }
        }
      }
      // leds.setPixelColor(bbPixelToNodesMapV5[rowProbedCalibration],
      // 0x4500e8);
    }
  }
}

int nothingTouchedSamples[16] = {0};

int getNothingTouched(int samples) {

  startProbe();
  int rejects = 0;
  int loops = 0;

  for (int i = 0; i < 16; i++) {

    nothingTouchedSamples[i] = 0;
  }
  do {

    // samples = 2;

    int sampleAverage = 0;
    rejects = 0;
    nothingTouchedReading = 0;
    for (int i = 0; i < samples; i++) {
      // int reading = readProbeRaw(1);
      int readNoth = readAdc(0, 32);
      nothingTouchedSamples[i] = readNoth;
      //   delayMicroseconds(50);
      //   Serial.print("nothingTouchedSample ");
      //   Serial.print(i);
      //   Serial.print(": ");
      // Serial.println(readNoth);
    }
    loops++;

    for (int i = 0; i < samples; i++) {

      if (nothingTouchedSamples[i] < 100) {
        sampleAverage += nothingTouchedSamples[i];
      } else {
        rejects++;
      }
    }
    if (samples - rejects <= 1) {
      Serial.println("All nothing touched samples rejected, check sense pad "
                     "connections\n\r");
      nothingTouchedReading = 36;
      return 0;
      break;
    }
    sampleAverage = sampleAverage / (samples - rejects);
    rejects = 0;

    for (int i = 0; i < samples; i++) {
      if (abs(nothingTouchedSamples[i] - sampleAverage) < 15) {
        nothingTouchedReading += nothingTouchedSamples[i];
        //       Serial.print("nothingTouchedSample ");
        //   Serial.print(i);
        //   Serial.print(": ");
        // Serial.println(nothingTouchedSamples[i]);
      } else {
        rejects++;
      }
    }

    nothingTouchedReading = nothingTouchedReading / (samples - rejects);

    if (loops > 10) {
      break;
    }

  } while ((nothingTouchedReading > 80 || rejects > samples / 2) && loops < 9);
  // Serial.print("nothingTouchedReading: ");
  // Serial.println(nothingTouchedReading);
  return nothingTouchedReading;
}
unsigned long doubleTimeout = 0;

unsigned long padTimeout = 0;
int padTimeoutLength = 250;

int state = 0;
int lastPadTouched = 0;
unsigned long padNoTouch = 0;

void checkPads(void) {
  // startProbe();
  checkingPads = 1;
  int probeReading = readProbeRaw();
  if (probeReading == -1) {

    checkingPads = 0;
    padNoTouch++;

    if (millis() - padTimeout > padTimeoutLength) {
      padTimeout = millis();
      lastReadRaw = 0;
    }
    return;
  }
  // Serial.print("padNoTouch: ");
  // Serial.println(padNoTouch);
  padNoTouch = 0;

  /* clang-format off */
  int probeRowMap[103] = {

      -1,        1,         2,        3,        4,        5,        6,        7,       8,
       9,       10,        11,       12,       13,       14,       15,       16,
      17,       18,        19,       20,       21,       22,       23,       24,
      25,       26,        27,       28,       29,       30,       TOP_RAIL,       TOP_RAIL_GND,
      BOTTOM_RAIL,       BOTTOM_RAIL_GND,      31,       32,       33,       34,       35,       36,
      37,       38,       39,       40,        41,       42,       43,       44,       45,
      46,       47,       48,       49,        50,       51,       52,       53,       54,
      55,       56,       57,       58,        59,       60,       NANO_D1,       NANO_D0,       NANO_RESET_1,
      NANO_GND_1,       NANO_D2,       NANO_D3,       NANO_D4,       NANO_D5,       NANO_D6,       NANO_D7,       NANO_D8,
      NANO_D9,	      NANO_D10,	      NANO_D11,	      NANO_D12,	      NANO_D13,	      NANO_3V3,	      NANO_AREF,	      NANO_A0,
      NANO_A1,	      NANO_A2,	      NANO_A3,	      NANO_A4,	      NANO_A5,	      NANO_A6,	      NANO_A7,	      NANO_5V,
      NANO_RESET_0,	      NANO_GND_0,	      NANO_VIN,	      LOGO_PAD_BOTTOM,	      LOGO_PAD_TOP,	      GPIO_PAD,	      DAC_PAD,
      ADC_PAD,	      BUILDING_PAD_TOP,	      BUILDING_PAD_BOTTOM,
  };

  /* clang-format on */
  probeReading = probeRowMap[map(probeReading, 40, 4050, 101, 0)];

  // stopProbe();
  if (probeReading < LOGO_PAD_TOP || probeReading > BUILDING_PAD_BOTTOM) {
    padTimeout = millis();
    lastReadRaw = 0;
    checkingPads = 0;
    return;
  }

  padTimeout = millis();
  // Serial.print("probeReading: ");
  // Serial.println(probeReading);
  int foundGpio = 0;
  int foundAdc = 0;
  int foundDac = 0;
  // inPadMenu = 1;
  switch (probeReading) {

  case LOGO_PAD_TOP: {
    switch (logoTopSetting[0]) {
    case -1:
      break;
    case 0: {
      foundDac = 1;
      if (logoTopSetting[1] >= 0 && logoTopSetting[1] <= 1) {

        // probeActive = 1;
        sfProbeMenu = 1;
        clearLEDsExceptRails();
        // checkingPads = 0;
        if (logoTopSetting[1] == 0) {

          dacOutput[0] = voltageSelect(5);

        } else {
          dacOutput[1] = voltageSelect(8);
        }

        setRailsAndDACs();
      }

      break;
    }
    case 1: {
      foundAdc = 1;
      if (logoTopSetting[1] >= 0 && logoTopSetting[1] <= 3) {
        adcRange[logoTopSetting[1]][1] += 0.1;
      }
      break;
    }
    case 2: {
      foundGpio = 1;
      if (logoTopSetting[1] >= 0 && logoTopSetting[1] <= 8) {
        if (gpioState[logoTopSetting[1]] == 0) {
          gpioState[logoTopSetting[1]] = 1;
        } else {
          gpioState[logoTopSetting[1]] = 0;
        }
      }
      setGPIO();
      break;
    }
    }
    break;
  }

  case LOGO_PAD_BOTTOM: {
    switch (logoBottomSetting[0]) {
    case -1:
      break;
    case 0: {
      foundDac = 1;
      if (logoBottomSetting[1] >= 0 && logoBottomSetting[1] <= 1) {
        // probeActive = 1;
        // sfProbeMenu = 1;
        clearLEDsExceptRails();
        // checkingPads = 0;
        if (logoBottomSetting[1] == 0) {
          dacOutput[2] = voltageSelect(5);
        } else {
          dacOutput[3] = voltageSelect(8);
        }
        setRailsAndDACs();
      }
      break;
    }
    case 1: {
      foundAdc = 1;
      if (logoBottomSetting[1] >= 0 && logoBottomSetting[1] <= 3) {
        adcRange[logoBottomSetting[1]][1] += 0.1;
      }
      break;
    }
    case 2: {
      foundGpio = 1;
      if (logoBottomSetting[1] >= 0 && logoBottomSetting[1] <= 8) {
        if (gpioState[logoBottomSetting[1]] == 0) {
          gpioState[logoBottomSetting[1]] = 1;
        } else {
          gpioState[logoBottomSetting[1]] = 0;
        }
      }
      setGPIO();
      break;
    }
    }
    break;
  }
  case BUILDING_PAD_TOP: {
    switch (buildingTopSetting[0]) {
    case -1:
      break;
    case 0: {
      foundDac = 1;
      if (buildingTopSetting[1] >= 0 && buildingTopSetting[1] <= 1) {
        // probeActive = 1;
        // sfProbeMenu = 1;
        clearLEDsExceptRails();
        // checkingPads = 0;
        if (buildingTopSetting[1] == 0) {
          dacOutput[4] = voltageSelect(5);
        } else {
          dacOutput[5] = voltageSelect(8);
        }
        setRailsAndDACs();
      }
      break;
    }
    case 1: {
      foundAdc = 1;
      if (buildingTopSetting[1] >= 0 && buildingTopSetting[1] <= 3) {
        adcRange[buildingTopSetting[1]][1] += 0.1;
      }
      break;
    }
    case 2: {
      foundGpio = 1;
      if (buildingTopSetting[1] >= 0 && buildingTopSetting[1] <= 8) {
        if (gpioState[buildingTopSetting[1]] == 0) {
          gpioState[buildingTopSetting[1]] = 1;
        } else {
          gpioState[buildingTopSetting[1]] = 0;
        }
      }
      setGPIO();
      break;
    }
    }
    break;
  }
  case BUILDING_PAD_BOTTOM: {
    switch (buildingBottomSetting[0]) {
    case -1:
      break;
    case 0: {
      foundDac = 1;
      if (buildingBottomSetting[1] >= 0 && buildingBottomSetting[1] <= 1) {
        // probeActive = 1;
        // sfProbeMenu = 1;
        clearLEDsExceptRails();
        // checkingPads = 0;
        if (buildingBottomSetting[1] == 0) {
          dacOutput[6] = voltageSelect(5);
        } else {
          dacOutput[7] = voltageSelect(8);
        }
        setRailsAndDACs();
      }
      break;
    }
    case 1: {
      foundAdc = 1;
      if (buildingBottomSetting[1] >= 0 && buildingBottomSetting[1] <= 3) {
        adcRange[buildingBottomSetting[1]][1] += 0.1;
      }
      break;
    }
    case 2: {
      foundGpio = 1;
      if (buildingBottomSetting[1] >= 0 && buildingBottomSetting[1] <= 8) {
        if (gpioState[buildingBottomSetting[1]] == 0) {
          gpioState[buildingBottomSetting[1]] = 1;
        } else {
          gpioState[buildingBottomSetting[1]] = 0;
        }
      }
      setGPIO();
      break;
    }
    }
    break;
  }
  }
   inPadMenu = 0;
  //  delay(1000);
}

int readProbeRaw(int readNothingTouched) {
  // nothingTouchedReading = 165;
  // lastReadRaw = 0;

  int measurements[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  // digitalWrite(probePin, HIGH);
  if (connectOrClearProbe == 1) {

    for (int i = 0; i < 4; i++) {
      measurements[i] = readAdc(0, 16);
      delayMicroseconds(30);
    }
  } else if (checkingPads == 1) {
    for (int i = 0; i < 4; i++) {
      measurements[i] = readAdc(0, 128);
      delayMicroseconds(30);
    }

  } else {
    for (int i = 0; i < 4; i++) {
      measurements[i] = readAdc(0, 4);
      delayMicroseconds(30);
    }
  }

  int maxVariance = 0;
  int average = 0;
  for (int i = 0; i < 3; i++) {
    if (abs(measurements[i] - measurements[i + 1]) > maxVariance) {
      maxVariance = abs(measurements[i] - measurements[i + 1]);
    }
    average = average + measurements[i];
  }
  average = average / 3;
  // Serial.print("average ");
  // Serial.println(average);
  int rowProbed = -1;
  // if (average < 90 && abs(average - nothingTouchedReading) > 10) {
  //   Serial.print("average ");
  //   Serial.println(average);
  // }

  if (maxVariance < 4 && (abs(average - nothingTouchedReading) > 35) &&
      (abs(average - lastReadRaw) > 5)) {
    lastReadRaw = average;
    //     Serial.print("adc0: ");

    // Serial.println(average);

    return average;

  } else {
    // Serial.print("average ");
    // Serial.println(average);
    return -1;
  }
}

int convertPadsToRows(int pad) {
  int row = pad;
  if (pad == LOGO_PAD_BOTTOM) {
    row = 108;
  } else if (pad == LOGO_PAD_TOP) {
    row = 109;
  } else if (pad == GPIO_PAD) {
    row = 116;
  } else if (pad == DAC_PAD) {
    row = 106;
  } else if (pad == ADC_PAD) {
    row = 111;
  } else if (pad == BUILDING_PAD_TOP) {
    row = 116;
  } else if (pad == BUILDING_PAD_BOTTOM) {
    row = 117;
  }
  return row;
}
unsigned long lastProbeTime = millis();
int readProbe() {
  int found = -1;
  // connectedRows[0] = -1;

  // if (checkProbeButton() == 1) {
  //   return -18;
  // }

  /* clang-format off */
  
  int probeRowMap[108] = {

      -1,        1,         2,        3,        4,        5,        6,        7,       8,
       9,       10,        11,       12,       13,       14,       15,       16,
      17,       18,        19,       20,       21,       22,       23,       24,
      25,       26,        27,       28,       29,       30,       TOP_RAIL,       TOP_RAIL_GND,
      BOTTOM_RAIL,       BOTTOM_RAIL_GND,      31,       32,       33,       34,       35,       36,
      37,       38,       39,       40,        41,       42,       43,       44,       45,
      46,       47,       48,       49,        50,       51,       52,       53,       54,
      55,       56,       57,       58,        59,       60,       NANO_D1,       NANO_D0,       NANO_RESET_1,
      NANO_GND_1,       NANO_D2,       NANO_D3,       NANO_D4,       NANO_D5,       NANO_D6,       NANO_D7,       NANO_D8,
      NANO_D9,	      NANO_D10,	      NANO_D11,	      NANO_D12,	      NANO_D13,	      NANO_3V3,	      NANO_AREF,	      NANO_A0,
      NANO_A1,	      NANO_A2,	      NANO_A3,	      NANO_A4,	      NANO_A5,	      NANO_A6,	      NANO_A7,	      NANO_5V,
      NANO_RESET_0,	      NANO_GND_0,	      NANO_VIN,	      LOGO_PAD_BOTTOM,	      LOGO_PAD_TOP,	      GPIO_PAD,	      DAC_PAD,
      ADC_PAD,	      BUILDING_PAD_TOP,	      BUILDING_PAD_BOTTOM, -1, -1 , -1, -1
  };

  /* clang-format on */

  // startProbe();
  int measurements[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  int probeRead = readProbeRaw();
  // delay(100);
  // Serial.println(probeRead);

  while (probeRead <= 0) {
    /// delay(50);
    // return -1;
    // Serial.println(probeRead);

    probeRead = readProbeRaw();
    rotaryEncoderStuff();

    if (encoderDirectionState != NONE) {
      if (encoderDirectionState == UP) {
        return -19;
      } else if (encoderDirectionState == DOWN) {
        return -17;
      }
    } else if (encoderButtonState == PRESSED &&
               lastButtonEncoderState == IDLE) {
      return -10;
    }
    int buttonState = checkProbeButton();
    if (buttonState == 1) {
      return -18;
    } else if (buttonState == 2) {
      return -16;
    }
    delayMicroseconds(200);

    if (millis() - doubleTimeout > 1000) {
      doubleTimeout = millis();
      lastReadRaw = 0;
    }

    if (millis() - lastProbeTime > 50) {
      lastProbeTime = millis();
      // Serial.println("probe timeout");
      return -1;
    }
  }
  doubleTimeout = millis();
  if (debugProbing == 1) {
    Serial.print("probeRead: ");
    Serial.println(probeRead);
  }
  if (probeRead == -1) {
    return -1;
  }
  int rowProbed = map(probeRead, 40, 4050, 101, 0);

  if (rowProbed <= 0 || rowProbed >= sizeof(probeRowMap)) {
    if (debugProbing == 1) {
      Serial.print("out of bounds of probeRowMap[");
      Serial.println(rowProbed);
    }
    return -1;
  }
  if (debugProbing == 1) {
    Serial.print("probeRowMap[");
    Serial.print(rowProbed);
    Serial.print("]: ");
    Serial.println(probeRowMap[rowProbed]);
  }

  rowProbed = selectSFprobeMenu(probeRowMap[rowProbed]);
  if (debugProbing == 1) {
    Serial.print("rowProbed: ");
    Serial.println(rowProbed);
  }
  connectedRows[0] = rowProbed;
  connectedRowsIndex = 1;

  // Serial.print("maxVariance: ");
  // Serial.println(maxVariance);
  return rowProbed;
  // return probeRowMap[rowProbed];
}

int scanRows(int pin) {
  return readProbe();
  return 0;
  int found = -1;
  connectedRows[0] = -1;

  if (checkProbeButton() == 1) {
    return -18;
  }

  // pin = ADC1_PIN;

  // digitalWrite(RESETPIN, HIGH);
  // delayMicroseconds(20);
  // digitalWrite(RESETPIN, LOW);
  // delayMicroseconds(20);

  pinMode(probePin, INPUT);
  delayMicroseconds(400);
  int probeRead = readFloatingOrState(probePin, -1);

  if (probeRead == high) {
    found = voltageSelection;
    connectedRows[connectedRowsIndex] = found;
    connectedRowsIndex++;
    found = -1;
    // return connectedRows[connectedRowsIndex];
    // Serial.print("high");
    // return found;
  }

  else if (probeRead == low) {
    found = GND;
    connectedRows[connectedRowsIndex] = found;
    connectedRowsIndex++;
    // return found;
    found = -1;
    // return connectedRows[connectedRowsIndex];
    // Serial.print(connectedRows[connectedRowsIndex]);

    // return connectedRows[connectedRowsIndex];
  }

  startProbe();
  int chipToConnect = 0;
  int rowBeingScanned = 0;

  int xMapRead = 15;

  if (pin == ADC0_PIN) {
    xMapRead = 2;
  } else if (pin == ADC1_PIN) {
    xMapRead = 3;
  } else if (pin == ADC2_PIN) {
    xMapRead = 4;
  } else if (pin == ADC3_PIN) {
    xMapRead = 5;
  }

  for (int chipScan = CHIP_A; chipScan < 8;
       chipScan++) // scan the breadboard (except the corners)
  {

    sendXYraw(CHIP_L, xMapRead, chipScan, 1);

    for (int yToScan = 1; yToScan < 8; yToScan++) {

      sendXYraw(chipScan, 0, 0, 1);
      sendXYraw(chipScan, 0, yToScan, 1);

      rowBeingScanned = ch[chipScan].yMap[yToScan];
      if (readFloatingOrState(pin, rowBeingScanned) == probe) {
        found = rowBeingScanned;

        if (found != -1) {
          connectedRows[connectedRowsIndex] = found;
          connectedRowsIndex++;
          found = -1;
          // delayMicroseconds(100);
          // stopProbe();
          // break;
        }
      }

      sendXYraw(chipScan, 0, 0, 0);
      sendXYraw(chipScan, 0, yToScan, 0);
    }
    sendXYraw(CHIP_L, 2, chipScan, 0);
  }

  int corners[4] = {1, 30, 31, 60};
  sendXYraw(CHIP_L, xMapRead, 0, 1);
  for (int cornerScan = 0; cornerScan < 4; cornerScan++) {

    sendXYraw(CHIP_L, cornerScan + 8, 0, 1);

    rowBeingScanned = corners[cornerScan];
    if (readFloatingOrState(pin, rowBeingScanned) == probe) {
      found = rowBeingScanned;
      // if (nextIsSupply)
      // {
      //     leds.setPixelColor(nodesToPixelMap[rowBeingScanned], 65, 10, 10);
      // }
      // else if (nextIsGnd)
      // {
      //     leds.setPixelColor(nodesToPixelMap[rowBeingScanned], 10, 65, 10);
      // }
      // else
      // {
      //     leds.setPixelColor(nodesToPixelMap[rowBeingScanned],
      //     rainbowList[rainbowIndex][0], rainbowList[rainbowIndex][1],
      //     rainbowList[rainbowIndex][2]);
      // }
      // showLEDsCore2 = 1;
      if (found != -1) {
        connectedRows[connectedRowsIndex] = found;
        connectedRowsIndex++;
        found = -1;
        // stopProbe();
        // break;
      }
    }

    sendXYraw(CHIP_L, cornerScan + 8, 0, 0);
  }
  sendXYraw(CHIP_L, xMapRead, 0, 0);

  for (int chipScan2 = CHIP_I; chipScan2 <= CHIP_J;
       chipScan2++) // scan the breadboard (except the corners)
  {

    int pinHeader = ADC0_PIN + (chipScan2 - CHIP_I);

    for (int xToScan = 0; xToScan < 12; xToScan++) {

      sendXYraw(chipScan2, xToScan, 0, 1);
      sendXYraw(chipScan2, 13, 0, 1);

      // analogRead(ADC0_PIN);

      rowBeingScanned = ch[chipScan2].xMap[xToScan];
      //   Serial.print("rowBeingScanned: ");
      //     Serial.println(rowBeingScanned);
      //     Serial.print("chipScan2: ");
      //     Serial.println(chipScan2);
      //     Serial.print("xToScan: ");
      //     Serial.println(xToScan);

      if (readFloatingOrState(pinHeader, rowBeingScanned) == probe) {

        found = rowBeingScanned;

        // if (nextIsSupply)
        // {
        //     //leds.setPixelColor(nodesToPixelMap[rowBeingScanned], 65, 10,
        //     10);
        // }
        // else if (nextIsGnd)
        // {
        //    // leds.setPixelColor(nodesToPixelMap[rowBeingScanned], 10, 65,
        //    10);
        // }
        // else
        // {
        //     //leds.setPixelColor(nodesToPixelMap[rowBeingScanned],
        //     rainbowList[rainbowIndex][0], rainbowList[rainbowIndex][1],
        //     rainbowList[rainbowIndex][2]);
        // }
        // //showLEDsCore2 = 1;
        // // leds.show();

        if (found != -1) {
          connectedRows[connectedRowsIndex] = found;
          connectedRowsIndex++;
          found = -1;
          // stopProbe();
          // break;
        }
      }
      sendXYraw(chipScan2, xToScan, 0, 0);
      sendXYraw(chipScan2, 13, 0, 0);
    }
  }

  // stopProbe();
  // probeTimeout = millis();

  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(RESETPIN, LOW);
  return connectedRows[0];
  // return found;

  // return 0;
}

int readRails(int pin) {
  int state = -1;

  // Serial.print("adc0 \t");
  // Serial.println(adcReadings[0]);
  // Serial.print("adc1 \t");
  // Serial.println(adcReadings[1]);
  // Serial.print("adc2 \t");
  // Serial.println(adcReadings[2]);
  // Serial.print("adc3 \t");
  // Serial.println(adcReadings[3]);

  return state;
}

// while (Serial.available() == 0 && (millis() - probeTimeout) < 6200)
// {
//     delayMicroseconds(8000);

//     // Serial.println(readRails(ADC0_PIN));

//     row[0] = scanRows(ADC0_PIN);

//     // delayMicroseconds(10);
//     // row[1] = scanRows(0);
//     // delayMicroseconds(10);
//     // row[2] = scanRows(0);
//     // delayMicroseconds(10);

//     if (row[0] != -1 && row[1] != row[0])
//     {
//         row[1] = row[0];

//         if (row[0] == -18 && millis() - probingTimer > 500)
//         {
//             Serial.print("\n\rCommitting paths!\n\r");
//             probingTimer = millis();
//             break;
//         }
//         else if (row[0] == -18)
//         {
//             continue;
//         }

//         // delayMicroseconds(10);

//         // Serial.print("\n\r");

//          int connectedReads = 0;

//         if (row[0] != SUPPLY_5V && row[0] != GND)
//         {

//             for (int i = 0; i < 10; i++)
//             {
//                 delayMicroseconds(50);

//                 int scanForOthers = scanRows(ADC0_PIN);

//                 if (scanForOthers != row[0] && scanForOthers != -1 )
//                 {
//                     lastRow[connectedReads] = scanForOthers;

//                      Serial.print("lastRow[");
//                         Serial.print(connectedReads);
//                         Serial.print("]: ");
//                         Serial.println(lastRow[connectedReads]);

//                         connectedReads++;

//                 }

//             }
//         }
//         // Serial.print("\n\r");
//         // Serial.print("lastFoundIndex: ");
//         // Serial.println (lastFoundIndex);
//         Serial.print("\n\r");
//         Serial.print("node1or2: ");
//         Serial.println(node1or2);

//         // delay(10);
//         lastRow[pokedNumber] = row[0];
//         probedNodes[probedNodesIndex][pokedNumber] = row[0];

//         pokedNumber++;

//         printNodeOrName(row[0]);
//         Serial.print("\r\t");

//         if (connectedReads > 1)
//         {
//             Serial.print("connectedReads: ");
//             Serial.println(connectedReads);

//             // for (int i = 0; i < lastFoundIndex[node1or2]; i++)
//             // {
//             //     Serial.print("lastFound[");
//             //     Serial.print(i);
//             //     Serial.print("][");
//             //     Serial.print(node1or2);
//             //     Serial.print("]: ");
//             //     Serial.print(connectedRows[i]);
//             //     Serial.print("\n\r");
//             // }

//             row[0] = selectFromLastFound();
//             probingTimer = millis();
//             // clearLastFound();
//             //  lastFoundIndex[node1or2] = 0;
//         }

//         if (pokedNumber >= 2)
//         {
//             node1or2 = 0;
//             Serial.print("\r            \r");
//             printNodeOrName(probedNodes[probedNodesIndex][0]);
//             Serial.print(" - ");
//             printNodeOrName(probedNodes[probedNodesIndex][1]);
//             Serial.print("\n\r");

//             Serial.print("\n\r");
//             node1or2 = 0;
//             for (int i = 0; i < probedNodesIndex; i++)
//             {

//                 /// Serial.print("\n\r");

//                 if ((probedNodes[i][0] == probedNodes[probedNodesIndex][0] &&
//                 probedNodes[i][1] == probedNodes[probedNodesIndex][1]) ||
//                 (probedNodes[i][0] == probedNodes[probedNodesIndex][1] &&
//                 probedNodes[i][1] == probedNodes[probedNodesIndex][0]))
//                 {
//                     probedNodes[probedNodesIndex][0] = 0;
//                     probedNodes[probedNodesIndex][1] = 0;

//                     leds.setPixelColor(nodesToPixelMap[probedNodes[i][0]],
//                     0);
//                     leds.setPixelColor(nodesToPixelMap[probedNodes[i][1]],
//                     0);

//                     for (int j = i; j < probedNodesIndex; j++)
//                     {
//                         probedNodes[j][0] = probedNodes[j + 1][0];
//                         probedNodes[j][1] = probedNodes[j + 1][1];
//                     }
//                     // probedNodes[i][0] = -1;
//                     // probedNodes[i][1] = -1;
//                     pokedNumber = 0;

//                     showLEDsCore2 = 1;
//                     probedNodesIndex--;
//                     probedNodesIndex--;
//                     // break;
//                 }
//             }
//             // Serial.print("\n\n\n\r");

//             // Serial.print("\r            \r");
//             // printNodeOrName(probedNodes[probedNodesIndex][0]);
//             // Serial.print(" - ");
//             // printNodeOrName(probedNodes[probedNodesIndex][1]);
//             // Serial.print("\n\r");

//             for (int i = probedNodesIndex; i >= 0; i--)
//             {
//                 // Serial.print ("    ");
//                 // Serial.print (i);
//                 Serial.print("\t");
//                 printNodeOrName(probedNodes[i][0]);
//                 Serial.print(" - ");
//                 printNodeOrName(probedNodes[i][1]);
//                 Serial.print("\n\r");
//             }
//             Serial.print("\n\n\r");

//             // delay(18);
//             pokedNumber = 0;
//             probedNodesIndex++;

//             // clearLEDs();
//             // openNodeFile();
//             // getNodesToConnect();

//             // bridgesToPaths();

//             /// assignNetColors();
//             delay(8);
//             row[0] = -1;
//             // showLEDsCore2 = 1;
//             // delay(18);

//             rainbowIndex++;
//             if (rainbowIndex > 11)
//             {
//                 rainbowIndex = 0;
//             }
//             scanRows(0, true);
//         }
//         else
//         {
//             node1or2 = 1;
//         }
//     }
// }

// for (int i = 0; i < probedNodesIndex; i++)
// {
//     addBridgeToNodeFile(probedNodes[i][0], probedNodes[i][1]);
// }

// for (int i = 0; i < 100; i++)
// {
//     delayMicroseconds(1);
//     row[0] = scanRows(ADC0_PIN);
//     connectedRowsIndex = 0;
//     //row[0] = checkProbeButton();
//     readPercentage[i] = row[0];
// }
// int percent = 0;
// for (int i = 0; i < 100; i++)
// {
//     if (readPercentage[i] != -1)
//     {
//         percent++;
//     }
// }
// Serial.print("percent: ");
// Serial.print(percent);
// Serial.print("%\n\r");
// // delayMicroseconds(10);
// // Serial.println(scanRows(ADC0_PIN));
