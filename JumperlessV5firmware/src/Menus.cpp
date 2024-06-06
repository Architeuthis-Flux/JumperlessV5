#include "Menus.h"
#include "RotaryEncoder.h"
#include "SafeString.h"
#include <Arduino.h>

#include "Graphics.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "LittleFS.h"
#include "PersistentStuff.h"

int menuRead = 0;
int menuLength = 0;
char menuChars[1000];

int menuLineIndex = 0;
String menuLines[250];
int menuLevels[250];
int stayOnTop[250];
int numberOfLevels = 0;

void readMenuFile(void) {
  LittleFS.begin();

  File menuFile = LittleFS.open("/MenuTree.txt", "r");
  if (!menuFile) {
    Serial.println("Failed to open menu file");
    return;
  }
  menuLength = 0;

  while (menuFile.available()) {
    menuLines[menuLineIndex] = menuFile.readStringUntil('\n');
    menuLineIndex++;
  }
  menuLineIndex--;

  menuRead = 1;

  menuFile.close();
}

int menuParsed = 0;

void parseMenuFile(void) {

  for (int i = 0; i < menuLineIndex + 1;
       i++) { // remove comments and empty lines

    if (menuLines[i].startsWith("/") || menuLines[i].startsWith("#") ||
        menuLines[i].startsWith("\n") || menuLines[i].length() < 2) {

      for (int j = i; j < menuLineIndex; j++) {
        menuLines[j] = menuLines[j + 1];
      }

      i--; // so it checks again
      menuLineIndex--;
    }
  }

  for (int i = 0; i <= menuLineIndex; i++) {

    menuLevels[i] = menuLines[i].lastIndexOf('-') + 1;
    menuLines[i].remove(0, menuLevels[i]);
    if (menuLevels[i] > numberOfLevels) {
      numberOfLevels = menuLevels[i];
    }

    // Serial.print(i);

    // // Serial.print(" ");
    // for (int j = 0; j < menuLevels[i]; j++) {
    //   Serial.print("  ");
    // }

    // Serial.println(menuLines[i]);
  }
  menuParsed = 1;
}

uint32_t menuColors[10] = {0x09000a, 0x0f0004, 0x080800, 0x010f00,
                           0x000a03, 0x00030a, 0x040010, 0x070006};

void initMenu(void) {

  if (menuRead == 0) {
    /// Serial.println(menuLines);
    delay(10);
    readMenuFile();
    // return 0;
  }
  if (menuParsed == 0) {
    delay(10);
    parseMenuFile();
    /// return 0;
  }
}

int clickMenu(int menuType, int menuOption, int extraOptions) {

  if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {
    encoderButtonState = IDLE;
    while (getMenuSelection() == -1) {
      delay(10);
    }
   //getMenuSelection();
   
  }
   return -1;
}

  int getMenuSelection(void) {

    int menuPosition = 0;
    int lastMenuPosition = 0;
    int menuLevel = 0;
    int lastMenuLevel = 2;

    int subMenuStartIndex = 0;
    int subMenuEndIndex = menuLineIndex;
    int firstTime = 1;

    int previousMenuSelection[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

    clearLEDsExceptRails();
    // delay(100);
    while (Serial.available() == 0) {
      if (encoderButtonState == DOUBLECLICKED) {
        b.clear();
        return -1;
      }

      if (encoderDirectionState == UP || firstTime == 1) { // ||
        // lastMenuLevel > menuLevel) {
        encoderDirectionState = NONE;
        firstTime = 0;
        menuPosition += 1;

        if (menuPosition > subMenuEndIndex) {
          menuPosition = subMenuStartIndex;
        }
        int loopCount = 0;
        for (int i = menuPosition; i <= menuLineIndex; i++) {
          // Serial.print(i);
          if (menuLevels[i] == menuLevel) {
            // Serial.println("^\n\r");
            menuPosition = i;
            break;
          }
          if (i == subMenuEndIndex && loopCount == 0) {
            // Serial.println();
            // Serial.print(i);
            // Serial.println("\n\n\r");
            loopCount++;
            i = subMenuStartIndex - 1;
          } else if (i == subMenuEndIndex && loopCount == 1) {

            Serial.println("!!! ");
            b.clear();
            return menuPosition;
            break;
          }
        }

        Serial.print("\r                                              \r");

        for (int j = 0; j <= menuLevels[menuPosition]; j++) {
          Serial.print(">");
        }
        // Serial.print(menuLevel);
        Serial.print(" ");
        Serial.print(menuLines[menuPosition]);
        b.clear();
        b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                0xFFFFFF, 0, -1,
                menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                        : 3);
        b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        if (menuLevel != lastMenuLevel) {
          // Serial.println();
        }
        showLEDsCore2 = 1;
        lastMenuLevel = menuLevel;
        // previousMenuSelection[menuLevel] = menuPosition;
        //  b.print(menuPosition);

      } else if (encoderDirectionState == DOWN || (lastMenuLevel < menuLevel)) {
        encoderDirectionState = NONE;

        // lastMenuLevel = menuLevel;

        if (menuPosition == previousMenuSelection[menuLevel]) {
          previousMenuSelection[menuLevel] = -1;
        } else {
          menuPosition -= 1;

          if (menuPosition < subMenuStartIndex) {
            menuPosition = subMenuEndIndex;
          }

          int loopCount = 0;

          for (int i = menuPosition; i >= 0; i--) {

            if (menuLevels[i] == menuLevel) {

              menuPosition = i;
              break;
            }
            if (i == subMenuStartIndex && loopCount == 0) {
              loopCount++;

              i = subMenuEndIndex + 1;
            } else if (i == subMenuStartIndex && loopCount == 1) {

              Serial.println("!!! ");

              b.clear();
              return menuPosition;
              break;
            }
          }
          // Serial.println("Fuck");
        }
        //         Serial.print("   \t\t");
        // Serial.println(menuPosition);
        // if (menuLevel == lastMenuLevel) {
        Serial.print("\r                                              \r");
        //} else {
        // Serial.println();
        //}
        for (int j = 0; j <= menuLevels[menuPosition]; j++) {
          Serial.print(">");
        }
        // Serial.print(menuLevel);
        Serial.print(" ");
        Serial.print(menuLines[menuPosition]);
        b.clear();
        b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                0xFFFFFF, 0, -1,
                menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                        : 3);
        b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        if (menuLevel != lastMenuLevel) {
          // Serial.println();
        }
        showLEDsCore2 = 1;
        lastMenuLevel = menuLevel;
        // previousMenuSelection[menuLevel] = menuPosition;

        // b.print(menuPosition);
      }

      delayMicroseconds(180);

      if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {

        lastMenuLevel = menuLevel;
        previousMenuSelection[menuLevel] = menuPosition;
        // Serial.print("[ ");
        // for (int i = 0; i < 10; i++) {
        //   Serial.print(previousMenuSelection[i]);
        //   Serial.print(", ");
        // }

        // Serial.print(" ]\n\r");
        Serial.println(menuPosition);
        menuLevel++;

        if (menuLevel > numberOfLevels + 1) {
          menuLevel = numberOfLevels;
        }

        for (int i = menuPosition + 1; i < menuLineIndex; i++) {

          if (menuLevels[i] < menuLevel) {
            subMenuEndIndex = i;
            break;
          }
        }

        for (int i = subMenuEndIndex - 1; i > 0; i--) {
          if (menuLevels[i] < menuLevel) {
            subMenuStartIndex = i;
            break;
          }
        }

        while (encoderButtonState == RELEASED)
          ;
      }

      if (encoderButtonState == HELD) {
        lastMenuLevel = menuLevel;
        // Serial.println("Held");
        int noPrevious = 0;

        for (int i = menuLevel; i < 10; i++) {
          previousMenuSelection[i] = -1;
        }

        if (menuLevel > 1) {
          menuLevel -= 1;
          // b.clear();
          // return 0;
        } else {
          menuLevel = 0;
          // menuPosition = 0;
        }
        // Serial.print("Menu Level: ");
        // Serial.println(menuLevel);

        if (menuLevel == 0) {
          subMenuStartIndex = 0;
          subMenuEndIndex = menuLineIndex - 1;
        } else {

          subMenuEndIndex = menuLineIndex;

          for (int i = menuPosition + 1; i <= menuLineIndex; i++) {

            if (menuLevels[i] < menuLevel) {
              subMenuEndIndex = i;
              break;
            }
          }

          for (int i = subMenuEndIndex - 1; i > 0; i--) {
            if (menuLevels[i] < menuLevel) {
              subMenuStartIndex = i;
              // Serial.print("\n\rsubMenuStartIndex: ");
              // Serial.println(subMenuStartIndex);
              // Serial.print("subMenuEndIndex: ");
              // Serial.println(subMenuEndIndex);
              break;
            }
          }
        }

        if (previousMenuSelection[menuLevel] == -1) {
          menuPosition = subMenuStartIndex;
        }

        if (previousMenuSelection[menuLevel] != -1) {
          menuPosition = previousMenuSelection[menuLevel];
          b.clear();
          b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                  0xFFFFFF, 0, -1,
                  menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);
          b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        }

        // delay(500); // you'll step back a level every 500ms

        while (encoderButtonState == HELD)
          ; // this determines if you just
        // co back to the top level or if you go back to the previous level
        //   ;
      }
    }
  
  return -1;
}

char printMainMenu(int extraOptions) {

  Serial.print("\n\n\r\t\tMenu\n\r");
  // Serial.print("Slot ");
  // Serial.print(netSlot);
  Serial.print("\n\r");
  Serial.print("\tm = show this menu\n\r");
  Serial.print("\tn = show netlist\n\r");
  Serial.print("\ts = show node files by slot\n\r");
  Serial.print("\to = load node files by slot\n\r");
  Serial.print("\tf = load node file to current slot\n\r");
  // Serial.print("\tr = rotary encoder mode -");
  //   rotaryEncoderMode == 1 ? Serial.print(" ON (z/x to cycle)\n\r")
  //                          : Serial.print(" off\n\r");
  // Serial.print("\t\b\bz/x = cycle slots - current slot ");
  // Serial.print(netSlot);
  Serial.print("\n\r");
  Serial.print("\te = show extra menu options\n\r");

  if (extraOptions == 1) {
    Serial.print("\tb = show bridge array\n\r");
    Serial.print("\tp = probe connections\n\r");
    Serial.print("\tw = waveGen\n\r");
    Serial.print("\tv = toggle show current/voltage\n\r");
    Serial.print("\tu = set baud rate for USB-Serial\n\r");
    Serial.print("\tl = LED brightness / test\n\r");
    Serial.print("\td = toggle debug flags\n\r");
  }
  // Serial.print("\tc = clear nodes with probe\n\r");
  Serial.print("\n\n\r");
  return ' ';
}

char LEDbrightnessMenu(void) {

  char input = ' ';
  Serial.print("\n\r\t\tLED Brightness Menu \t\n\n\r");
  Serial.print("\n\r\tl = LED brightness     =   ");
  Serial.print(LEDbrightness);
  Serial.print("\n\r\tr = Rail brightness    =   ");
  Serial.print(LEDbrightnessRail);
  Serial.print("\n\r\ts = Special brightness =   ");
  Serial.print(LEDbrightnessSpecial);
  Serial.print("\n\r\tt = All types\t");
  Serial.print("\n\n\r\td = Reset to defaults");
  Serial.print("\n\n\r\tb = Rainbow Bounce test");
  Serial.print("\n\r\tc = Random Color test\n\r");

  Serial.print("\n\r\tx = Exit\n\n\r");
  // Serial.print(leds.getBrightness());
  if (LEDbrightness > 50 || LEDbrightnessRail > 50 ||
      LEDbrightnessSpecial > 70) {
    // Serial.print("\tBrightness settings above ~50 will cause significant
    // heating, it's not recommended\n\r");
    delay(10);
  }

  while (Serial.available() == 0) {
    delayMicroseconds(10);
  }

  input = Serial.read();

  if (input == 'x') {
    saveLEDbrightness(0);

    return ' ';
  } else if (input == 'd') {
    saveLEDbrightness(1);

    return ' ';
  } else if (input == 'l') {
    Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tx = exit\n\r");
    while (input == 'l') {

      while (Serial.available() == 0)
        ;
      char input2 = Serial.read();
      if (input2 == '+') {
        LEDbrightness += 1;

        if (LEDbrightness > 200) {

          LEDbrightness = 200;
        }

        showLEDsCore2 = 1;
      } else if (input2 == '-') {
        LEDbrightness -= 1;

        if (LEDbrightness < 2) {
          LEDbrightness = 1;
        }

        showLEDsCore2 = 1;
      } else if (input2 == 'x') {
        input = ' ';
      } else {
      }

      for (int i = 8; i <= numberOfNets; i++) {
        lightUpNet(i, -1, 1, LEDbrightness, 0);
      }
      showLEDsCore2 = 1;

      if (Serial.available() == 0) {

        Serial.print("LED brightness:  ");
        Serial.print(LEDbrightness);
        Serial.print("\n\r");
        if (LEDbrightness > 50) {
          // Serial.print("Brightness settings above ~50 will cause
          // significant heating, it's not recommended\n\r");
        }
      }
    }
  } else if (input == 'r') {
    Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tx = exit\n\r");
    while (input == 'r') {

      while (Serial.available() == 0)
        ;
      char input2 = Serial.read();
      if (input2 == '+') {

        LEDbrightnessRail += 1;

        if (LEDbrightnessRail > 200) {

          LEDbrightnessRail = 200;
        }

        showLEDsCore2 = 1;
      } else if (input2 == '-') {

        LEDbrightnessRail -= 1;

        if (LEDbrightnessRail < 2) {
          LEDbrightnessRail = 1;
        }

        showLEDsCore2 = 1;
      } else if (input2 == 'x') {
        input = ' ';
      } else {
      }
      lightUpRail(-1, -1, 1, LEDbrightnessRail);

      if (Serial.available() == 0) {

        Serial.print("Rail brightness:  ");
        Serial.print(LEDbrightnessRail);
        Serial.print("\n\r");
        if (LEDbrightnessRail > 50) {
          // Serial.println("Brightness settings above ~50 will cause
          // significant heating, it's not recommended\n\n\r");
        }
      }
    }

    // Serial.print(input);
    Serial.print("\n\r");
  }

  else if (input == 's') {
    // Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tx =
    // exit\n\n\r");
    while (input == 's') {

      while (Serial.available() == 0)
        ;
      char input2 = Serial.read();
      if (input2 == '+') {

        LEDbrightnessSpecial += 1;

        if (LEDbrightnessSpecial > 200) {

          LEDbrightnessSpecial = 200;
        }

        // showLEDsCore2 = 1;
      } else if (input2 == '-') {

        LEDbrightnessSpecial -= 1;

        if (LEDbrightnessSpecial < 2) {
          LEDbrightnessSpecial = 1;
        }

        // showLEDsCore2 = 1;
      } else if (input2 == 'x') {
        input = ' ';
      } else {
      }

      for (int i = 0; i < 8; i++) {
        lightUpNet(i, -1, 1, LEDbrightnessSpecial, 0);
      }
      showLEDsCore2 = 1;

      if (Serial.available() == 0) {

        Serial.print("Special brightness:  ");
        Serial.print(LEDbrightnessSpecial);
        Serial.print("\n\r");
        if (LEDbrightnessSpecial > 70) {
          // Serial.print("Brightness settings above ~70 for special nets will
          // cause significant heating, it's not recommended\n\n\r ");
        }
      }
    }

    // Serial.print(input);
    Serial.print("\n\r");
  } else if (input == 't') {

    Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tx = exit\n\n\r");
    while (input == 't') {

      while (Serial.available() == 0)
        ;
      char input2 = Serial.read();
      if (input2 == '+') {

        LEDbrightness += 1;
        LEDbrightnessRail += 1;
        LEDbrightnessSpecial += 1;

        if (LEDbrightness > 200) {

          LEDbrightness = 200;
        }
        if (LEDbrightnessRail > 200) {

          LEDbrightnessRail = 200;
        }
        if (LEDbrightnessSpecial > 200) {

          LEDbrightnessSpecial = 200;
        }

        showLEDsCore2 = 1;
      } else if (input2 == '-') {

        LEDbrightness -= 1;
        LEDbrightnessRail -= 1;
        LEDbrightnessSpecial -= 1;

        if (LEDbrightness < 2) {
          LEDbrightness = 1;
        }
        if (LEDbrightnessRail < 2) {
          LEDbrightnessRail = 1;
        }
        if (LEDbrightnessSpecial < 2) {
          LEDbrightnessSpecial = 1;
        }

        showLEDsCore2 = 1;
      } else if (input2 == 'x' || input2 == ' ' || input2 == 'm' ||
                 input2 == 'l') {
        input = ' ';
      } else {
      }

      for (int i = 8; i <= numberOfNets; i++) {
        lightUpNet(i, -1, 1, LEDbrightness, 0);
      }

      lightUpRail(-1, -1, 1, LEDbrightnessRail);
      for (int i = 0; i < 8; i++) {
        lightUpNet(i, -1, 1, LEDbrightnessSpecial, 0);
      }
      showLEDsCore2 = 1;

      if (Serial.available() == 0) {

        Serial.print("LED brightness:      ");
        Serial.print(LEDbrightness);
        Serial.print("\n\r");
        Serial.print("Rail brightness:     ");
        Serial.print(LEDbrightnessRail);
        Serial.print("\n\r");
        Serial.print("Special brightness:  ");
        Serial.print(LEDbrightnessSpecial);
        Serial.print("\n\r");
        if (LEDbrightness > 50 || LEDbrightnessRail > 50 ||
            LEDbrightnessSpecial > 70) {
          // Serial.print("Brightness settings above ~50 will cause
          // significant heating, it's not recommended\n\n\r ");
        }
      }
    }
  } else if (input == 'b') {
    Serial.print("\n\rPress any key to exit\n\n\r");
    leds.clear();
    while (Serial.available() == 0) {
      startupColorsV5();
      clearLEDsExceptRails();
      showLEDsCore2 = 1;

      delay(2000);
      // rainbowBounce(3);
    }
    showNets();
    lightUpRail(-1, -1, 1);
    showLEDsCore2 = 1;

    input = '!'; // this tells the main fuction to reset the leds
  } else if (input == 'c') {
    Serial.print("\n\rPress any key to exit\n\n\r");
    while (Serial.available() == 0) {
      randomColors(0, 90);
    }
    delay(100);
    input = '!';
  } else if (input == 'p') {
    for (int i = 0; i < LED_COUNT; i++) {
      uint32_t color = leds.getPixelColor(i);
      rgbColor currentPixel = unpackRgb(color);
      char padZero = '0';

      // String colorString =
      //     ("0x") + (currentPixel.r > 15 ? : '0') + String(currentPixel.r,
      //     16)
      //     + (currentPixel.g > 15 ? : '0') + String(currentPixel.g, 16) +
      //     (currentPixel.b > 15 ? : '0') + String(currentPixel.b, 16);
      Serial.print("0x");
      currentPixel.r > 15 ?: Serial.print(padZero);
      Serial.print(currentPixel.r, 16);
      currentPixel.g > 15 ?: Serial.print(padZero);
      Serial.print(currentPixel.g, 16);
      currentPixel.b > 15 ?: Serial.print(padZero);
      Serial.print(currentPixel.b, 16);
      Serial.print(", ");

      if (i % 8 == 0 && i > 0) {
        Serial.println();
      }

      // Serial.println(colorString);
    }

    return ' ';
  } else {
    saveLEDbrightness(0);
    assignNetColors();

    return input;
  }
  return input;
}

/*
0Show
1  Voltage
2    0 1 2
3      Nodes
1  Current
2    Nodes
1  Digital
2    5V
3      0 1 2 3
4        Nodes
2    3.3V
3      0 1 2 3
4        Nodes
2    UART
3      Tx Rx
4        Nodes
5          2nd USBPrint
6            Baud
2    I2C
3      SDA SCL
4        Nodes
5          2nd USBPrint
6            Baud
1  Options
2    Middle Out
3      Range
2    Bottom Up
3      Range
2    Bright
3      Range
2    Color
3      Range
0Rails
1  Both
2    VoltageSet
1  Top
2    VoltageSet
1  Bottom
2    VoltageSet
0Slots
1  Save
2    Which?
1  Load
2    Which?
1  Clear
2    Which?
0Output
1  Volage
4        5V or ±8V?
3      VoltageSet
4        Nodes
1  Digital
2    5V
3      0 1 2 3
4        Nodes
2    3.3V
3      0 1 2 3
4        Nodes
1  UART
2    Tx Rx
3      Nodes
4        2nd USBPrint
5          Baud
1  Buffer
2    In Out
3      Nodes
0Arduino
1  UART
2    Tx Rx
3      Nodes
4        2nd USBPrint
5          Baud
1  Reset
0Probe
1  Connect
1  Clear
1  Calibration



*/