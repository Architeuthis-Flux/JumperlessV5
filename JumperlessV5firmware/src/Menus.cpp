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
String menuLines[150];
int menuLevels[150];
int stayOnTop[150];
uint8_t numberOfChoices[150];
uint8_t actions[150]; //>n nodes 1 //>b baud 2 //>v voltage 3

uint32_t optionSlpitLocations[150];
int numberOfLevels = 0;

//>n nodes //>b baud //>v voltage //$Stay on top$ //*choices*

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
    if (menuLines[i].startsWith("$") && menuLines[i].endsWith("$")) {
      stayOnTop[i] = 1;
      menuLines[i].remove(0, 1);
      menuLines[i].remove(menuLines[i].length() - 1, 1);
    } else {
      stayOnTop[i] = 0;
    }
    int starIndex = menuLines[i].indexOf("*");
    int starCount = 0;

    int shift = 0;
    while (starIndex != -1) {
      menuLines[i].remove(starIndex, 1);

      // optionSlpitLocations[i];
      bitSet(optionSlpitLocations[i], starIndex + shift);
      shift++;
      starCount++;
      starIndex = menuLines[i].indexOf("*");
    }

    numberOfChoices[i] = starCount / 2;

    int actionIndex = menuLines[i].indexOf(">");
    int actionChar = menuLines[i].charAt(actionIndex + 1);
    if (actionIndex != -1) {
      switch (actionChar) {
      case 'n':
        actions[i] = 1;
        break;
      case 'b':
        actions[i] = 2;
        break;
      case 'v':
        actions[i] = 3;
        break;
      default:
        actions[i] = 0;
        break;
      }
      char actionChar2 = menuLines[i].charAt(actionIndex + 2);
      if (actionChar2 >= '0' && actionChar2 <= '9') {
        numberOfChoices[i] = menuLines[i].charAt(actionIndex + 2) - '0';
      }

      menuLines[i].remove(actionIndex, 3);
    }
  }
  // delay(1000);
  // for (int i = 0; i <= menuLineIndex; i++) {
  //   Serial.print(menuLevels[i]);
  //   Serial.print(" \t");
  //   int spaces = Serial.print(menuLines[i]);
  //   for (int j = 0; j < 20 - spaces; j++) {
  //     Serial.print(" ");
  //   }
  //   Serial.print(" ");
  //   Serial.print(stayOnTop[i]);
  //   Serial.print(" \t");
  //   Serial.print(numberOfChoices[i]);
  //   Serial.print(" \t");
  //   Serial.print(actions[i]);
  //   Serial.print(" \t0b");
  //   for (int j = 0; j < 32; j++) {
  //     Serial.print(bitRead(optionSlpitLocations[i], j));
  //   }

  //   Serial.println(" ");
  // }

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
    // getMenuSelection();
  }
  return -1;
}
int subMenuChoices[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
int maxNumSelections = 0;
int getMenuSelection(void) {

  int stayOnTopLevel = -1;
  int stayOnTopIndex = -1;

  int menuPosition = -1;
  int lastMenuPosition = 0;
  int menuLevel = 0;
  int lastMenuLevel = 2;

  int subMenuStartIndex = 0;
  int subMenuEndIndex = menuLineIndex;
  int firstTime = 1;

  int lastSubmenuOption = 0;

  int previousMenuSelection[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
  int force = 0;

  int subSelection = -1;
  clearLEDsExceptRails();
  // delay(100);

  while (Serial.available() == 0) {
    if (encoderButtonState == DOUBLECLICKED) {
      b.clear();
      return -1;
    }

    if (encoderDirectionState == UP || firstTime == 1 || force == 1) { // ||
      // lastMenuLevel > menuLevel) {
      encoderDirectionState = NONE;
      firstTime = 0;

      lastMenuPosition = menuPosition;
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

      if (stayOnTopLevel != -1 && stayOnTopIndex != -1) {
        b.clear();
        b.print(menuLines[stayOnTopIndex].c_str(), menuColors[stayOnTopLevel],
                0xFFFFFF, 0, 0,
                menuLines[stayOnTopIndex].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);
        b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        b.print(
            menuLines[menuPosition].c_str(), menuColors[menuLevel], 0xFFFFFF, 0,
            1, menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1 : 3);

      } else {

        b.clear();
        b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                0xFFFFFF, 0, -1,
                menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                        : 3);
        b.printMenuReminder(menuLevel, menuColors[menuLevel]);
      }

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
      lastMenuPosition = menuPosition;
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
      if (actions[menuPosition] == 1 && subSelection != -1) {
        for (int a = 0; a < 8; a++) {
          subMenuChoices[a] = -1;
        }
        int nextOption = 0;

        subMenuChoices[subSelection] = selectNodeAction(subSelection);
         maxNumSelections = numberOfChoices[menuPosition];
        // Serial.println(maxNumSelections);
        // Serial.println();
        while (nextOption != -1 && maxNumSelections > 1) {
          // Serial.println(maxNumSelections);
          // Serial.println();
          // Serial.println("fuck");
          nextOption = selectSubmenuOption(lastMenuPosition, lastMenuLevel);

          if (nextOption == -1) {
           // Serial.println("-1");
            break;
          }
          Serial.println(nextOption);
          subMenuChoices[nextOption] = selectNodeAction(nextOption);
          maxNumSelections--;
          /// Serial.println("fuck");
          // Serial.print("[ ");

          // for (int a = 0; a < numberOfChoices[menuPosition]; a++) {
          //   Serial.print(subMenuChoices[a]);
          //   Serial.print(",");
          // }
          // Serial.println(" ] \n\r");
        }
        return menuPosition;

      } else {

        if (stayOnTopLevel != -1 && stayOnTopIndex != -1) {
          b.clear();
          b.print(menuLines[stayOnTopIndex].c_str(), menuColors[stayOnTopLevel],
                  0xFFFFFF, 0, 0,
                  menuLines[stayOnTopIndex].length() >= 7 || menuLevel == 0
                      ? 1
                      : 3);
          b.printMenuReminder(menuLevel, menuColors[menuLevel]);
          b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                  0xFFFFFF, 0, 1,
                  menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);

        } else {

          b.clear();
          b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                  0xFFFFFF, 0, -1,
                  menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);
          b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        }
        if (numberOfChoices[menuPosition] > 0) {
          // Serial.println("  fuck     !!");
          subSelection = selectSubmenuOption(menuPosition, menuLevel);

          if (subSelection != -1) {
            // menuLevel++;
            Serial.print("subselection: ");
            Serial.println(subSelection);
            encoderButtonState = RELEASED;
            lastButtonEncoderState = PRESSED;
          }
          lastMenuPosition = menuPosition;

          force = 0;
        }
      }

      showLEDsCore2 = 1;
      lastMenuLevel = menuLevel;
      // previousMenuSelection[menuLevel] = menuPosition;

      // b.print(menuPosition);
    }

    delayMicroseconds(180);

    if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {

      lastMenuLevel = menuLevel;

      if (stayOnTop[menuPosition] == 1) {
        stayOnTopLevel = menuLevel;
        stayOnTopIndex = menuPosition;
      }

      previousMenuSelection[menuLevel] = menuPosition;
      // Serial.print("[ ");
      // for (int i = 0; i < 10; i++) {
      //   Serial.print(previousMenuSelection[i]);
      //   Serial.print(", ");
      // }

      // Serial.print(" ]\n\r");
      // Serial.println(menuPosition);
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

      //             if (numberOfChoices[menuPosition] > 0) {
      // force = 1;
      //       }

      while (encoderButtonState == RELEASED)
        ;
    }

    if (encoderButtonState == HELD) {
      lastMenuLevel = menuLevel;
      // Serial.println("Held");

      if (stayOnTopLevel == menuLevel) {

        stayOnTopLevel = -1;
        stayOnTopIndex = -1;
      }
      firstTime = 1;
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

        if (stayOnTopLevel != -1 && stayOnTopIndex != -1) {
          b.clear();
          b.print(menuLines[stayOnTopIndex].c_str(), menuColors[stayOnTopLevel],
                  0xFFFFFF, 0, 0,
                  menuLines[stayOnTopIndex].length() >= 7 || menuLevel == 0
                      ? 1
                      : 3);
          b.printMenuReminder(menuLevel, menuColors[menuLevel]);
          b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                  0xFFFFFF, 0, 1,
                  menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);

        } else {

          menuPosition = previousMenuSelection[menuLevel];
          b.clear();
          b.print(menuLines[menuPosition].c_str(), menuColors[menuLevel],
                  0xFFFFFF, 0, -1,
                  menuLines[menuPosition].length() >= 7 || menuLevel == 0 ? 1
                                                                          : 3);
          b.printMenuReminder(menuLevel, menuColors[menuLevel]);
        }
      }
      // delay(500); // you'll step back a level every 500ms

      while (encoderButtonState == HELD)
        if (encoderDirectionState == DOWN) {
          encoderButtonState = IDLE;

          break;
        }
      // this determines if you just
      // co back to the top level or if you go back to the previous level
      //   ;
    }
  }

  return -1;
}

int selectSubmenuOption(int menuPosition, int menuLevel) {

  int optionSelected = -1;
  int highlightedOption = 1;
  // Serial.println("\n\r");
  String subMenuStrings[8];
  int menuOptionLengths[8];
  int maxMenuOptionLength = 0;
  for (int i = 0; i < 8; i++) {
    subMenuStrings[i] = "";
  }

  int shiftStars = -2;
  int lastOption = 0;
  for (int i = 0; i < 8; i++) {
    int optionStart = -1;
    int optionEnd = -1;

    for (int j = lastOption; j < 32; j++) {
      if (bitRead(optionSlpitLocations[menuPosition], j) == 1) {
        if (optionStart == -1) {
          optionStart = j;
          shiftStars++;
        } else {
          optionEnd = j;
          shiftStars++;
          lastOption = j + 1;
          break;
        }
      }
    }

    if (optionStart != -1 && optionEnd != -1) {
      subMenuStrings[i] = menuLines[menuPosition].substring(
          optionStart - shiftStars, optionEnd - shiftStars - 1);
      if (subMenuStrings[i].length() > maxMenuOptionLength) {
        menuOptionLengths[i] = subMenuStrings[i].length();
        maxMenuOptionLength = subMenuStrings[i].length();
      }
    }
  }
  // Serial.println(" ");
  for (int i = 0; i < 8; i++) {
    if (subMenuStrings[i] != "") {
    }
  }
  uint32_t subMenuColors[10] = {
      0x010f00, 0x000a03, 0x00030a, 0x040010,
      0x070006, 0x09000a, 0x0f0004, 0x080800,
  };
  uint32_t selectColor = subMenuColors[(menuLevel + 5) % 8];
  uint32_t backgroundColor = 0xffffff; // 0x0101000;
  int changed = 1;

  String choiceLine = menuLines[menuPosition];

  int cut = 0;
  if (choiceLine.length() > 7) {
    cut = 1;
  }

  int menuType =
      0; // 0 = cycle (numbers) 1 = show both options 2 = show one option

  if (choiceLine.length() <= 7 && maxMenuOptionLength > 1) {
    menuType = 1;
  } else if (maxMenuOptionLength > 2 && choiceLine.length() > 7) {
    menuType = 2;
  }

  // Serial.println("selected Submenu Option\n\r");

  encoderButtonState = IDLE;

  int firstTime = 1;

  while (optionSelected == -1) {

    if (encoderButtonState == DOUBLECLICKED || encoderButtonState == HELD) {
      b.clear();
      return -1;
    }

    if (encoderDirectionState == UP) {
      encoderDirectionState = NONE;
      highlightedOption += 1;
      if (highlightedOption > numberOfChoices[menuPosition] - 1) {
        highlightedOption = 0;
      }
      changed = 1;
    } else if (encoderDirectionState == DOWN || firstTime == 1) {
      encoderDirectionState = NONE;
      highlightedOption -= 1;
      if (highlightedOption < 0) {
        highlightedOption = numberOfChoices[menuPosition] - 1;
      }
      firstTime = 0;
      changed = 1;
    } else if (encoderButtonState == RELEASED &&
               lastButtonEncoderState == PRESSED) {
      encoderButtonState = IDLE;
      optionSelected = highlightedOption;
      Serial.print("\r                      \r");
      Serial.print("Option Selected: ");
      Serial.println(optionSelected);
      // return optionSelected;
    }
    if (changed == 1) {

      b.clear(1);

      if (menuType == 0) {

        b.print(subMenuStrings[highlightedOption].c_str(), selectColor,
                backgroundColor, 3, 1, 0);

        // Serial.println(selectColor, HEX);
        int start = highlightedOption;
        int loopCount = 0;
        int nudge = 0;
        Serial.print("\r                      \r");

        for (int i = 0; i < 7; i++) {

          if (i != (3)) {
            b.print(subMenuStrings[(i + highlightedOption + 5) % 8].c_str(),
                    subMenuColors[menuLevel], 0xFFFFFF, i, 1, nudge);
            Serial.print(subMenuStrings[(i + highlightedOption + 5) % 8]);
          } else {
            Serial.print(" ");
            Serial.print(subMenuStrings[(i + highlightedOption + 5) % 8]);
            Serial.print(" ");
          }
        }
      } else if (menuType == 1) {

        int menuStartPositions[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 8; i++) {
          menuStartPositions[i] = 0;
        }
        int nextStart = 0;
        // selectColor = 0x1a001a;
        for (int i = 0; i < numberOfChoices[menuPosition]; i++) {

          if (i == highlightedOption) {
            b.printRawRow(0b00001010, (nextStart * 4) + 30, selectColor,
                          0xffffff);
            b.printRawRow(0b00000100, (nextStart * 4) + 31, selectColor,
                          0xffffff);
            b.print(subMenuStrings[i].c_str(), selectColor, 0xffffff, nextStart,
                    1, 1);
          } else {
            b.print(subMenuStrings[i].c_str(), subMenuColors[menuLevel],
                    0xffffff, nextStart, 1, 1);
          }

          nextStart += subMenuStrings[i].length() + 1;

          // Serial.print(subMenuStrings[i]);
          // Serial.print(" ");
          // Serial.println(subMenuStrings[i].length());
        }

      } else if (menuType == 2) {
        b.printRawRow(0b00001010, 30, selectColor, 0xffffff);
        b.printRawRow(0b00000100, 31, selectColor, 0xffffff);
        b.print(subMenuStrings[highlightedOption].c_str(),
                subMenuColors[(highlightedOption + menuLevel) % 8],
                backgroundColor, 0, 1, 1);
      }
      showLEDsCore2 = 1;

      changed = 0;
    }

    // b.print(choiceLine.c_str(), subMenuColors[menuLevel], 0xFFFFFF, 0, 1,
    //         numberOfChoices[menuPosition] > 7 ? 1 : 3);

    // delay(6800);
  }
  return highlightedOption;
}

int selectNodeAction(int whichSelection) {
  b.clear();

  uint32_t nodeSelectionColors[10] = {
      0x0f0700, 0x00090f, 0x0a000f, 0x050d00,
      0x100500, 0x000411, 0x100204, 0x020f02,
  };
  int nodeSelected = -1;
  int currentlySelecting = whichSelection;

  Serial.print("Currently Selecting: ");
  Serial.println(currentlySelecting);
  Serial.println();

  int highlightedNode = 13;
  int firstTime = 1;
  rotaryDivider = 4;

if (subMenuChoices[currentlySelecting] != -1)
  {
    highlightedNode = subMenuChoices[currentlySelecting]-1;
    subMenuChoices[currentlySelecting] = -1;
    maxNumSelections ++;
  }

  // if (subMenuChoices[0] != -1)
  // {
  //   for (int i = 0; i < 8; i++)
  //   {
  //     if (subMenuChoices[i] == -1)
  //     {
  //       currentlySelecting = i;

  //     highlightedNode = subMenuChoices[i-1]-1;
  //     break;
  //     }
  //   }
  // }

  while (nodeSelected == -1) {
    if (encoderButtonState == DOUBLECLICKED) {
      b.clear();
      rotaryDivider = 8;
      return -1;
    }

    if (encoderDirectionState == UP || firstTime == 1) {
      firstTime = 0;
      encoderDirectionState = NONE;
      highlightedNode += 1;
      if (highlightedNode > 59) {
        highlightedNode = 0;
      }
      b.clear();
      for (int a = 0; a < 8; a++) {
        if (subMenuChoices[a] != -1) {
          b.printRawRow(0b00011111, (subMenuChoices[a] - 1),
                        nodeSelectionColors[a], 0xffffff);

          // b.print(subMenuChoices[a], nodeSelectionColors[a], 0xffffff, a, 1,
          // 1);
        }
        b.printRawRow(0b00011111, (highlightedNode),
                      nodeSelectionColors[currentlySelecting], 0xffffff);
      }
      showLEDsCore2 = 1;
      // leds.show();
    } else if (encoderDirectionState == DOWN) {
      encoderDirectionState = NONE;
      highlightedNode -= 1;
      if (highlightedNode < 0) {
        highlightedNode = 59;
      }
      b.clear();
      for (int a = 0; a < 8; a++) {
        if (subMenuChoices[a] != -1) {
          b.printRawRow(0b00011111, (subMenuChoices[a] - 1),
                        nodeSelectionColors[a], 0xffffff);

          // b.print(subMenuChoices[a], nodeSelectionColors[a], 0xffffff, a, 1,
          // 1);
        }
        b.printRawRow(0b00011111, (highlightedNode),
                      nodeSelectionColors[currentlySelecting], 0xffffff);
      }
      showLEDsCore2 = 1;
      // leds.show();
    } else if (encoderButtonState == RELEASED &&
               lastButtonEncoderState == PRESSED) {
      encoderButtonState = IDLE;
      nodeSelected = highlightedNode;
      Serial.print("\r                      \r");
      // Serial.print("Node Selected: ");
      // Serial.println(nodeSelected + 1);
     // delay(100);

    } else if (encoderButtonState == HELD) {

      highlightedNode += 30;
      highlightedNode = highlightedNode % 60;
      if (highlightedNode > 59) {
        highlightedNode = 0;
      }
      b.clear();
            for (int a = 0; a < 8; a++) {
        if (subMenuChoices[a] != -1) {
          b.printRawRow(0b00011111, (subMenuChoices[a] - 1),
                        nodeSelectionColors[a], 0xffffff);

          // b.print(subMenuChoices[a], nodeSelectionColors[a], 0xffffff, a, 1,
          // 1);
        }
            }
      b.printRawRow(0b00011111, (highlightedNode),
                    nodeSelectionColors[currentlySelecting], 0xffffff);
      showLEDsCore2 = 1;
      // leds.show();
      while (encoderButtonState == HELD)
        ;
    }

    // b.clear(1);
    // b.print("Node: ", 0x010f00, 0xffffff, 0, 0, 0);
    // b.print(highlightedNode, 0x010f00, 0xffffff, 6, 0, 0);

    // for (int i = 0; i < numberOfNets; i++)
    // {
    //   if (i == highlightedNode)
    //   {
    //     b.printRawRow(0b00001010, (i * 4) + 30, 0x010f00, 0xffffff);
    //     b.printRawRow(0b00000100, (i * 4) + 31, 0x010f00, 0xffffff);
    //     b.print(i, 0x010f00, 0xffffff, i, 1, 1);
    //   }
    //   else
    //   {
    //     b.print(i, 0x010f00, 0xffffff, i, 1, 1);
    //   }
    // }
  }
  rotaryDivider = 8;
  return nodeSelected + 1;
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