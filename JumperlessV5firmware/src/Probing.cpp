

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
#include "RotaryEncoder.h"
#include "hardware/pwm.h"
#include "pico/stdlib.h"
#include <EEPROM.h>
#include <algorithm>

#include "Graphics.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
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

int rainbowList[13][3] = {
    {40, 50, 80}, {88, 33, 70}, {30, 15, 45}, {8, 27, 45},  {45, 18, 19},
    {35, 42, 5},  {02, 45, 35}, {18, 25, 45}, {40, 12, 45}, {10, 32, 45},
    {18, 5, 43},  {45, 28, 13}, {8, 12, 8}};
int rainbowIndex = 0;

int nextIsSupply = 0;
int nextIsGnd = 0;
int justCleared = 1;

char oledBuffer[32] = "                               ";

void drawchar(void) {

  if (OLED_CONNECTED == 0) {
    return;
  }
  display.clearDisplay();
  if (isSpace(oledBuffer[7]) == true) {
    display.setTextSize(3);  // Normal 1:1 pixel scale
    display.setCursor(0, 5); // Start at top-left corner
  } else if (isSpace(oledBuffer[10]) == true &&
             isSpace(oledBuffer[11]) == true) {
    display.setTextSize(2);  // Normal 1:1 pixel scale
    display.setCursor(0, 9); // Start at top-left corner
  } else {
    display.setTextSize(1);  // Normal 1:1 pixel scale
    display.setCursor(0, 0); // Start at top-left corner
  }

  display.setTextColor(SSD1306_WHITE); // Draw white text

  display.cp437(true); // Use full 256 char 'Code Page 437' font

  // Not all the characters will fit on the display. This is normal.
  // Library will draw what it can and the rest will be clipped.
  display.write(oledBuffer);

  display.display();

  for (int i = 0; i < 32; i++) {
    oledBuffer[i] = ' ';
  }
  /// delay(2000);
}

int probeMode(int pin, int setOrClear) {

startProbe();
restartProbing:
  probeActive = 1;
  connectOrClearProbe = setOrClear;
  int lastRow[10];

  int pokedNumber = 0;

  // Serial.print(numberOfNets);

  if (numberOfNets == 0) {
    // clearNodeFile(netSlot);
  }
  clearAllNTCC();
  openNodeFile(netSlot);
  getNodesToConnect();

  bridgesToPaths();
  clearLEDs();
  assignNetColors();
  showNets();
  // delay(18);
  lightUpRail();
  showLEDsCore2 = 1;
  // delay(28);
  int probedNodes[40][2];
  int probedNodesIndex = 0;

  int row[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  row[1] = -2;
  probeTimeout = millis();

  int readPercentage[101];
  probingTimer = millis();

  // Serial.print("Press any key to exit and commit paths (or touch probe to
  // gpio 18)   ");
  Serial.print("\n\r\t  Probing mode\n\n\r");
  Serial.print("   long press  = connect (pink) / clear (orange)\n\r");
  Serial.print("   short press = commit\n\r");

  if (setOrClear == 1) {
    // sprintf(oledBuffer, "connect  ");
    // drawchar();
    Serial.print("\n\r\t  connect nodes\n\n\n\r");
    rawOtherColors[1] = 0x4500e8;
    rainbowIndex = 0;
  } else {
    // sprintf(oledBuffer, "clear");
    // drawchar();
    Serial.print("\n\r\t  clear nodes\n\n\n\r");
    rawOtherColors[1] = 0x6644A8;
    rainbowIndex = 12;
  }

  if (setOrClear == 0) {
    probeButtonTimer = millis();

    // probingTimer = millis() -400;
  }

  unsigned long doubleSelectTimeout = millis();
  int doubleSelectCountdown = 0;

  int lastProbedRows[4] = {0, 0, 0, 0};

  Serial.print("\n\r");
  Serial.println(setOrClear);
  while (Serial.available() == 0 && (millis() - probeTimeout) < 6200 &&
         encoderButtonState == IDLE) {
    delayMicroseconds(800);

    connectedRowsIndex = 0;


    row[0] = readProbe();
    //Serial.println (row[0]);


    if (row[0] == -18 && (millis() - probingTimer > 500) &&
        checkProbeButton() == 1 && (millis() - probeButtonTimer) > 1000) {
      if (longShortPress(750) == 1) {
        setOrClear = !setOrClear;
        probingTimer = millis();
        probeButtonTimer = millis();
        showLEDsCore2 = 1;
        sfProbeMenu = 0;
        goto restartProbing;
        break;
      }

      // Serial.print("\n\rCommitting paths!\n\r");
      row[1] = -2;
      probingTimer = millis();

      connectedRowsIndex = 0;

      node1or2 = 0;
      nodesToConnect[0] = -1;
      nodesToConnect[1] = -1;

      // showLEDsCore2 = 1;

      break;
    } else {
      // probingTimer = millis();
    }

    if (row[0] != -1 && row[0] != row[1] && row[0] != lastProbedRows[0] &&
        row[0] != lastProbedRows[1]) {

      lastProbedRows[1] = lastProbedRows[0];
      lastProbedRows[0] = row[0];
      if (connectedRowsIndex == 1) {
        nodesToConnect[node1or2] = connectedRows[0];
        printNodeOrName(nodesToConnect[0]);

        Serial.print("\r\t");


        if (nodesToConnect[node1or2] > 0 && nodesToConnect[node1or2] <= 60) {
          // b.clear();

          b.printRawRow(0b00011111, nodesToConnect[node1or2] - 1, 0x4500e8,
                        0xffffff);
          

          showLEDsCore2 = 1;

        } else if (nodesToConnect[node1or2] >= NANO_D0 &&
                   nodesToConnect[node1or2] < NANO_RESET_1) {
          // b.clear();

          for (int i = 0; i < 35; i++) {
            if (bbPixelToNodesMapV5[i][0] == nodesToConnect[node1or2]) {
              leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x4500e8);
              // leds.show();
              // break;
            } else {
              // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
            }
          }
        }
        // leds.show();
        // }

        node1or2++;
        probingTimer = millis();
        showLEDsCore2 = 2;
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
        // if (nodesToConnect[0] != nodesToConnect[1])
        // {

        // }

        if (setOrClear == 1 && (nodesToConnect[0] != nodesToConnect[1]) &&
            nodesToConnect[0] != -1 && nodesToConnect[1] != -1) {
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

          addBridgeToNodeFile(nodesToConnect[0], nodesToConnect[1], netSlot);
          // printNodeFile(netSlot);
          //  delay(100);
          //   rainbowIndex++;
          if (rainbowIndex > 1) {
            rainbowIndex = 0;
          }

          delay(10);
          clearAllNTCC();
          openNodeFile(netSlot);
          getNodesToConnect();
          // Serial.print("openNF\n\r");

          bridgesToPaths();
          // clearLEDsExceptRails();
          assignNetColors();
          showNets();
          delay(10);
          showLEDsCore2 = 1;

          row[1] = -1;

          // doubleSelectTimeout = millis();

          doubleSelectTimeout = millis();
          doubleSelectCountdown = 2000;

          //delay(400);
        } else if (setOrClear == 0) {
          Serial.print("\r           \r");
          printNodeOrName(nodesToConnect[0]);
          Serial.print("\t cleared\n\r");
          removeBridgeFromNodeFile(nodesToConnect[0], -1, netSlot);
          // leds.setPixelColor(nodesToPixelMap[nodesToConnect[0]], 0, 0, 0);

          // leds.setPixelColor(nodesToPixelMap[nodesToConnect[1]], 0, 0, 0);
          rainbowIndex = 12;
          // goto restartProbing;
          openNodeFile(netSlot);
          delay(10);
          clearAllNTCC();
          getNodesToConnect();
          // Serial.print("openNF\n\r");

          bridgesToPaths();
          // clearLEDsExceptRails();
          assignNetColors();
          showNets();
          delay(20);
          showLEDsCore2 = 1;
          // delay(20);
        }
        node1or2 = 0;
        nodesToConnect[0] = -1;
        nodesToConnect[1] = -1;
        // row[1] = -2;
        doubleSelectTimeout = millis();
        doubleSelectCountdown = 2000;
      }

      row[1] = row[0];
    }
    // Serial.print("\n\r");
    // Serial.print(" ");
    // Serial.print(row[0]);

    if (justSelectedConnectedNodes == 1) {
      justSelectedConnectedNodes = 0;
    }



    if ((node1or2 == 0 && doubleSelectCountdown <= 0)) {
      // Serial.println("doubleSelectCountdown");
      row[1] = -2;
      doubleSelectTimeout = millis();
      doubleSelectCountdown = 1000;
      
    }

     //Serial.println(doubleSelectCountdown);

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
  clearAllNTCC();
  openNodeFile(netSlot);
  getNodesToConnect();

  bridgesToPaths();
  // clearLEDs();
  //  leds.clear();
  assignNetColors();
  // // Serial.print("bridgesToPaths\n\r");
  //
  showNets();
  showLEDsCore2 = 1;
  rawOtherColors[1] = 0x550004;
  // showLEDsCore2 = 1;
  delay(18);
  sendAllPathsCore2 = 1;
  // delay(25);
  // pinMode(probePin, INPUT);
  delay(100);
  row[1] = -2;

  // sprintf(oledBuffer, "        ");
  // drawchar();

  // rotaryEncoderMode = wasRotaryMode;
  return 1;
}

unsigned long blinkTimer = 0;
volatile int sfProbeMenu = 0;

int selectSFprobeMenu(int function)
{
b.clear();
clearLEDsExceptRails();
showLEDsCore2 = 2;
switch(function)
{

  case 132:
  {
    b.print(" ADC", scaleDownBrightness( rawOtherColors[8], 4, 22), 0xFFFFFF, 0, 0, 3);
    sfProbeMenu = 1;
    // delay(1000);
    // function = 111;
    b.print("1", scaleDownBrightness( rawOtherColors[8], 4, 22), 0xFFFFFF, 0,1,3);
     b.print("2", scaleDownBrightness( rawOtherColors[9], 4, 22), 0xFFFFFF, 2,1,3);
      b.print("3", scaleDownBrightness( rawOtherColors[10], 4, 22), 0xFFFFFF, 4,1,3);
      int selected = -1;
      while (selected == -1){
        int reading = readProbe();
        if ( reading != -1)
        {
          switch (reading)
          {
            case 34 ... 40:
            {
              selected = 111;
              function = 111;
              break;
            }
            case 41 ... 48:
            {
              selected = 112;
              function = 112;
              break;
            }
            case 50 ... 56:
            {
              selected = 113;
              function = 113;
              break;
            }
          }
        }


      }



    break;
  }
  case 131:
  {
b.print("DAC", scaleDownBrightness( rawOtherColors[9], 4, 22), 0xFFFFFF, 0, 0, 2);
sfProbeMenu = 2;
break;
  }
  case 130:
{
  b.print("GPIO", scaleDownBrightness( rawOtherColors[10], 8, 22), 0xFFFFFF, 0, 0, 2);
  sfProbeMenu = 3;
  break;
}



}
lightUpRail();
//delay(500);
sfProbeMenu = 0;

return function;



}

int voltageSelect(void) {
  return SUPPLY_5V;
  int selected = SUPPLY_3V3;
  int selectionConfirmed = 0;
  int selected2 = 0;
  int lastSelected = 0;
  connectedRows[0] = -1;
  Serial.print("\r                                \r");
  Serial.print("\n\r");
  Serial.print("      select voltage\n\n\r");
  Serial.print("  short press = cycle through voltages\n\r");
  Serial.print("  long press  = select\n\r");

  Serial.print("\n\r ");

  while (selectionConfirmed == 0) {

    if (lastSelected != selected) {
      if (selected == SUPPLY_3V3) {
        Serial.print("\r3.3V   ");
        clearLEDsExceptRails();
        leds.setPixelColor(nodesToPixelMap[1], 55, 0, 12);
        leds.setPixelColor(nodesToPixelMap[2], 55, 0, 12);
        leds.setPixelColor(nodesToPixelMap[3], 55, 0, 12);

        leds.setPixelColor(nodesToPixelMap[31], 15, 0, 0);
        leds.setPixelColor(nodesToPixelMap[32], 15, 0, 0);
        leds.setPixelColor(nodesToPixelMap[33], 15, 0, 0);

        leds.setPixelColor(nodesToPixelMap[34], 15, 0, 0);
        leds.setPixelColor(nodesToPixelMap[35], 15, 0, 0);
      } else if (selected == SUPPLY_5V) {
        Serial.print("\r5V    ");
        clearLEDsExceptRails();
        leds.setPixelColor(nodesToPixelMap[1], 15, 0, 6);
        leds.setPixelColor(nodesToPixelMap[2], 15, 0, 6);
        leds.setPixelColor(nodesToPixelMap[3], 15, 0, 6);

        leds.setPixelColor(nodesToPixelMap[31], 55, 0, 0);
        leds.setPixelColor(nodesToPixelMap[32], 55, 0, 0);
        leds.setPixelColor(nodesToPixelMap[33], 55, 0, 0);

        leds.setPixelColor(nodesToPixelMap[34], 55, 0, 0);
        leds.setPixelColor(nodesToPixelMap[35], 55, 0, 0);
      }
      // showLEDsCore2 = 1;

      leds.show();
      lastSelected = selected;
    }

    delay(30);

    int longShort = longShortPress();
    if (longShort == 1) {
      selectionConfirmed = 1;
      voltageSelection = selected;
      // break;
    } else if (longShort == 0) {
      if (selected == SUPPLY_3V3) {
        selected = SUPPLY_5V;
      } else {
        selected = SUPPLY_3V3;
      }
    }
  }

  // Serial.print("\n\r");
  leds.clear();
  showNets();
  showLEDsCore2 = 2;

  return voltageSelection;
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

  if (checkProbeButton() == 1) {

    while (millis() - clickTimer < pressLength) {
      if (checkProbeButton() == 0) {
        return 0;
      }
      delay(5);
    }
  } else {
    return -1;
  }

  return 1;
}

int checkProbeButton(void) {
  int buttonState = 0;

  pinMode(9, INPUT_PULLDOWN);
  pinMode(10, OUTPUT_12MA);

  digitalWrite(10, HIGH);

  delayMicroseconds(1500);

  buttonState = digitalRead(9);

  // if (buttonState == 1) {
  //      Serial.print("buttonState");

  //    Serial.println(buttonState);
  // }

  return buttonState;
}

int readFloatingOrState(int pin, int rowBeingScanned) {
  return 0;
  enum measuredState state = unknownState;
  // enum measuredState state2 = floating;

  int readingPullup = 0;
  int readingPullup2 = 0;
  int readingPullup3 = 0;

  int readingPulldown = 0;
  int readingPulldown2 = 0;
  int readingPulldown3 = 0;

  pinMode(pin, INPUT_PULLUP);

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

  pinMode(pin, INPUT_PULLDOWN);

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
  // pwm_set_irq_enabled(1, true);
  // pwm_set_clkdiv_mode(1,PWM_DIV_B_RISING);

  pinMode(probePin, OUTPUT_12MA);
  pinMode(buttonPin, INPUT_PULLDOWN);
  pinMode(ADC0_PIN, INPUT);
  digitalWrite(probePin, HIGH);
  // probeFrequency = probeSpeed;
  // probeHalfPeriodus = 1000000 / probeSpeed / 2;
  // pinMode(probePin, OUTPUT);
  // analogWriteFreq(probeSpeed);
  // analogWrite(probePin, 128);
  // delayMicroseconds(10);
  // pinMode(buttonPin, INPUT);
}

void stopProbe() {
  pinMode(probePin, INPUT);
  pinMode(buttonPin, INPUT);
}

int checkLastFound(int found) {
  int found2 = 0;
  return found2;
}

void clearLastFound() {}

int probeADCmap[102];

int nothingTouchedReading = 145;
int calibrateProbe() {

  int probeRowMap[102] = {

      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10,
      11,
      12,
      13,
      14,
      15,
      16,
      17,
      18,
      19,
      20,
      21,
      22,
      23,
      24,
      25,
      26,
      27,
      28,
      29,
      30,
      TOP_RAIL,
      TOP_RAIL_GND,
      BOTTOM_RAIL,
      BOTTOM_RAIL_GND,
      31,
      32,
      33,
      34,
      35,
      36,
      37,
      38,
      39,
      40,
      41,
      42,
      43,
      44,
      45,
      46,
      47,
      48,
      49,
      50,
      51,
      52,
      53,
      54,
      55,
      56,
      57,
      58,
      59,
      60,
      NANO_D1,
      NANO_D0,
      NANO_RESET_1,
      NANO_GND_1,
      NANO_D2,
      NANO_D3,
      NANO_D4,
      NANO_D5,
      NANO_D6,
      NANO_D7,
      NANO_D8,
      NANO_D9,
      NANO_D10,
      NANO_D11,
      NANO_D12,
      NANO_D13,
      NANO_3V3,
      NANO_AREF,
      NANO_A0,
      NANO_A1,
      NANO_A2,
      NANO_A3,
      NANO_A4,
      NANO_A5,
      NANO_A6,
      NANO_A7,
      NANO_5V,
      NANO_RESET_0,
      NANO_GND_0,
      NANO_VIN,

      LOGO_PAD_BOTTOM,
      LOGO_PAD_TOP,
      GPIO_PAD,
      DAC_PAD,
      ADC_PAD,
      BUILDING_PAD_TOP,
      BUILDING_PAD_BOTTOM,
  };
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

    //   for (int i = 1; i < 60; i++) {
    //     if (i%5 == 0 || i == 1){

    //       Serial.print("Touch probe to row ");
    //       Serial.println(i);
    //       int probeADCreading = readProbeRaw();
    // delay(100);
    //       while (abs (probeADCreading -nothingTouchedReading < 5)) {
    //         probeADCreading = readProbeRaw();
    //       }

    //       probeADCmap[i] = probeADCreading;
    //       Serial.print("probeADCreading: ");
    //       Serial.println(probeADCreading);
    //       while (abs(readProbeRaw()-probeADCreading )< 5) {
    //         delay(10);
    //       }
    //     } else {
    //       probeADCmap[i] = 0;
    //     }

    //   }

    //   for (int i = 1; i < 102; i++) {
    //     Serial.print(probeADCmap[i]);
    //     Serial.print(" ");
    //   }
    //   int rangeFillLow  = 0;
    //   int rangeFillHigh = 0;
    //   int rangeSeparation = 0;
    //   float rangeFill = 0;

    //   for (int i = 1; i < 60; i++) {
    //     if (i%5 == 0 || i == 1){
    //       rangeFillLow = probeADCmap[i];
    //       rangeFillHigh = probeADCmap[i+5];
    //       Serial.print("rangeFillLow: ");
    //       Serial.println(rangeFillLow);
    //       Serial.print("rangeFillHigh: ");
    //       Serial.println(rangeFillHigh);
    //       rangeSeparation = abs(rangeFillHigh - rangeFillLow);
    //       Serial.print("rangeSeparation: ");
    //       Serial.println(rangeSeparation);

    //       rangeFill = abs(rangeSeparation / 5);
    //       Serial.print("rangeFill: ");
    //       Serial.println(rangeFill);
    //       for (int j = i+1; j < i+5; j++) {
    //         probeADCmap[j] = (int)rangeFill*j;
    //       }

    //     } else {
    //       //probeADCmap[i] = 0;
    //     }

    //   }
    //   for (int i = 1; i < 102; i++) {
    //     Serial.print(probeADCmap[i]);
    //     Serial.print(" ");
    //   }

    int rowProbedCalibration = 0;
    //   Serial.println("calibrating probe\n\r");

    //   Serial.println("press probe to row 1\n\r");
    //   delay(100);

    //   int probeRead1 = readProbeRaw();

    //   while (probeRead1 == nothingTouchedReading || probeRead1 < 3900) {
    //     probeRead1 = readProbeRaw();
    //   }
    //   Serial.println(probeRead1);
    //  delay(300);
    // Serial.println("press probe to row \n\r");

    //   Serial.print(" -> ");
    //   Serial.print(nothingTouchedReading);
    //   Serial.print(" - ");
    //   Serial.print(probeRead1);
    //   Serial.print("\t\t");
    //   Serial.println(nothingTouchedReading);

    //  for (int i = 0; i < 100; i++) {
    //    Serial.print(probeRowMap[i]);
    //     Serial.print(" ");

    //  }
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
    //   Serial.println("\n\rpress probe to row 30\n\r");
    //   int probeRead30 = readProbeRaw();

    //   Serial.println(probeRead30);
    // delay(10);
    //   Serial.println("\n\rpress probe to row 60\n\r");
    //   int probeRead60 = readProbeRaw();
    // delay(10);
    //   Serial.println(probeRead60);

    //  Serial.println("\n\r press probe to ADC pad");
    //  int probeReadADC = readProbeRaw();

    //  Serial.println(probeReadADC);
    //   delay(10);
    // int probeReads[4] = {probeRead1, probeRead30, probeRead60, probeReadADC};

    // int probeReadsAverage = 0;

    // for (int i = 0; i < 4; i++) {
    //   probeReadsAverage = probeReadsAverage + probeReads[i];
    // }

    // probeReadsAverage = probeReadsAverage / 4;

    // Serial.print("probeReadsAverage: ");
    // Serial.println(probeReadsAverage);

    // int probeReadsVariance = 0;

    // for (int i = 0; i < 4; i++) {
    //   probeReadsVariance = probeReadsVariance + abs(probeReads[i] -
    //   probeReadsAverage);
    // }

    // probeReadsVariance = probeReadsVariance / 4;

    // Serial.print("probeReadsVariance: ");
    // Serial.println(probeReadsVariance);

    // return probeReadsAverage;
  }
}

int readProbse() {

  int probeRowMap[102] = {

      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10,
      11,
      12,
      13,
      14,
      15,
      16,
      17,
      18,
      19,
      20,
      21,
      22,
      23,
      24,
      25,
      26,
      27,
      28,
      29,
      30,
      TOP_RAIL,
      TOP_RAIL_GND,
      BOTTOM_RAIL,
      BOTTOM_RAIL_GND,
      31,
      32,
      33,
      34,
      35,
      36,
      37,
      38,
      39,
      40,
      41,
      42,
      43,
      44,
      45,
      46,
      47,
      48,
      49,
      50,
      51,
      52,
      53,
      54,
      55,
      56,
      57,
      58,
      59,
      60,
      NANO_D1,
      NANO_D0,
      NANO_RESET_1,
      NANO_GND_1,
      NANO_D2,
      NANO_D3,
      NANO_D4,
      NANO_D5,
      NANO_D6,
      NANO_D7,
      NANO_D8,
      NANO_D9,
      NANO_D10,
      NANO_D11,
      NANO_D12,
      NANO_D13,
      NANO_3V3,
      NANO_AREF,
      NANO_A0,
      NANO_A1,
      NANO_A2,
      NANO_A3,
      NANO_A4,
      NANO_A5,
      NANO_A6,
      NANO_A7,
      NANO_5V,
      NANO_RESET_0,
      NANO_GND_0,
      NANO_VIN,

      LOGO_PAD_BOTTOM,
      LOGO_PAD_TOP,
      GPIO_PAD,
      DAC_PAD,
      ADC_PAD,
      BUILDING_PAD_TOP,
      BUILDING_PAD_BOTTOM,
  };

  connectedRows[0] = -1;

  //startProbe();
  int probeRead = readProbeRaw();
  if (probeRead == -1) {
    return -1;
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
    return -1;
    mappedRead = 0;
  }
  if (mappedRead > 101) {
    return -1;
    mappedRead = 101;
  }
  // delay(10);
  int rowProbed = probeRowMap[mappedRead];

  connectedRows[0] = rowProbed;
  connectedRowsIndex = 1;

  // Serial.print(rowProbedCalibration);
  // Serial.print(" ");
  // Serial.print(mappedRead);
  //   Serial.print("\t\t");
  // Serial.println(nothingTouchedReading);
  // Serial.println();
  // delay(200);
  //  if (checkProbeButton() == 1) {
  //    break;
  //  }
  // if (rowProbed > 0 && rowProbed <= 60) {
  //   b.clear();

  // b.printRawRow(0b00011111, rowProbed-1, 0x4500e8, 0xffffff);
  // leds.show();

  // //showLEDsCore2 = 1;

  //   } else if (rowProbed >= NANO_D0 && rowProbed< NANO_RESET_1) {
  //     b.clear();

  //     for (int i = 0; i <35; i++) {
  //       if (bbPixelToNodesMapV5[i][0] == rowProbed) {
  // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x4500e8);
  // leds.show();
  // //break;
  //       } else {
  //         leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);}

  //       }
  //     }
  // leds.setPixelColor(bbPixelToNodesMapV5[rowProbedCalibration], 0x4500e8);
  return rowProbed;
}
int lastReadRaw = 0;

int getNothingTouched(int samples) {

  startProbe();

  for (int i = 0; i < samples; i++) {
    // int reading = readProbeRaw(1);
    nothingTouchedReading += readAdc(0, 64);
    delayMicroseconds(50);
  }
  nothingTouchedReading = nothingTouchedReading / samples;
  Serial.print("nothingTouchedReading: ");
  Serial.println(nothingTouchedReading);
  return nothingTouchedReading;
}

int readProbeRaw(int readNothingTouched) {
  // nothingTouchedReading = 165;

  int measurements[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  //digitalWrite(probePin, HIGH);
  for (int i = 0; i < 4; i++) {
    measurements[i] = readAdc(0, 64);
    delayMicroseconds(100);
    // Serial.println(measurements[i]);
    // delay(300);
  }

  //  Serial.print("adc1: ");
  //  Serial.println(readAdc(1, 16));
  //   Serial.print("adc2: ");
  //   Serial.println(readAdc(2, 16));
  //   Serial.print("adc3: ");
  //   Serial.println(readAdc(3, 16));

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

  if (maxVariance < 4 && (abs(average - nothingTouchedReading) > 15) &&
      (abs(average - lastReadRaw) > 12)) {
    lastReadRaw = average;
  //     Serial.print("adc0: ");

  //  Serial.println(measurements[0]);

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
int readProbe() {
  int found = -1;
  // connectedRows[0] = -1;

  // if (checkProbeButton() == 1) {
  //   return -18;
  // }
  int probeRowMap[103] = {

      -1,
      1,
      2,
      3,
      4,
      5,
      6,
      7,
      8,
      9,
      10,
      11,
      12,
      13,
      14,
      15,
      16,
      17,
      18,
      19,
      20,
      21,
      22,
      23,
      24,
      25,
      26,
      27,
      28,
      29,
      30,
      TOP_RAIL,
      TOP_RAIL_GND,
      BOTTOM_RAIL,
      BOTTOM_RAIL_GND,
      31,
      32,
      33,
      34,
      35,
      36,
      37,
      38,
      39,
      40,
      41,
      42,
      43,
      44,
      45,
      46,
      47,
      48,
      49,
      50,
      51,
      52,
      53,
      54,
      55,
      56,
      57,
      58,
      59,
      60,
      NANO_D1,
      NANO_D0,
      NANO_RESET_1,
      NANO_GND_1,
      NANO_D2,
      NANO_D3,
      NANO_D4,
      NANO_D5,
      NANO_D6,
      NANO_D7,
      NANO_D8,
      NANO_D9,
      NANO_D10,
      NANO_D11,
      NANO_D12,
      NANO_D13,
      NANO_3V3,
      NANO_AREF,
      NANO_A0,
      NANO_A1,
      NANO_A2,
      NANO_A3,
      NANO_A4,
      NANO_A5,
      NANO_A6,
      NANO_A7,
      NANO_5V,
      NANO_RESET_0,
      NANO_GND_0,
      NANO_VIN,
          LOGO_PAD_BOTTOM,
          LOGO_PAD_TOP,
          GPIO_PAD,
          DAC_PAD,
          ADC_PAD,
          BUILDING_PAD_TOP,
          BUILDING_PAD_BOTTOM,
          };

  // startProbe();
  int measurements[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  int probeRead = readProbeRaw();
  // delay(100);
  //  Serial.println(probeRead);
  while (probeRead == -1  && encoderButtonState == IDLE) {
    /// delay(50);
    // return -1;
    // Serial.println(probeRead);

    probeRead = readProbeRaw();

    if (checkProbeButton() == 1) {
      return -18;
    }
    delayMicroseconds(200);
  }
  //     Serial.print("probeRead: ");

  int rowProbed = map(probeRead, 50, 4050, 101, 0);

  rowProbed = selectSFprobeMenu(probeRowMap[rowProbed]);
  // Serial.print("rowProbed: ");
  // Serial.println(rowProbed);

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

  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(20);
  digitalWrite(RESETPIN, LOW);
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
