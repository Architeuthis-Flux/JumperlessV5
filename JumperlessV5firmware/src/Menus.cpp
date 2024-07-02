#include "Menus.h"
#include "RotaryEncoder.h"
#include "SafeString.h"
#include <Arduino.h>

#include "ArduinoStuff.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "LittleFS.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "RotaryEncoder.h"
#include "Probing.h"

int inClickMenu = 0;

int menuRead = 0;
int menuLength = 0;
char menuChars[1000];

int menuLineIndex = 0;
String menuLines[120];
int menuLevels[120];
int stayOnTop[120];
uint8_t numberOfChoices[120];
uint8_t actions[120]; //>n nodes 1 //>b baud 2 //>v voltage 3

uint32_t optionSlpitLocations[120];
int numberOfLevels = 0;
int optionVoltage = 0;

uint8_t selectMultiple[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int selectMultipleIndex = 0;

char hardCodeOptions[10][10] = {"Tx",  "Rx", "SDA",   "SCL",  "5V",
                                "3V3", "8V", "USB 2", "Print"};

struct action {
  int previousMenuPositions[10];
  int connectOrClear[10];
  char fromAscii[20][10];
  int from[20];
  int to[20];
  int previousMenuIndex;
  int connectIndex;
  int optionVoltage;
  int baud;
  int printOrUSB; // 0 print 1 USB
  float analogVoltage;
};

//>n nodes //>b baud //>v voltage //$Stay on top$ //*choices*

// enum actionCategories {
//   SHOWACTION,
//   RAILSACTION,
//   SLOTSACTION,
//   OUTPUTACTION,
//   ARDUINOACTION,
//   PROBEACTION,
//   NOCATEGORY
// };

// enum showOptions {
//   VOLTAGE,
//   CURRENT,
//   GPIO5V,
//   GPIO3V3,
//   SHOWUART,
//   SHOWI2C,
//   NOSHOW
// };
// enum railOptions { TOP, BOTTOM, BOTH, NORAIL };
// enum slotOptions { SAVETO, LOADFROM, CLEAR, NOSLOT };
// enum outputOptions {
//   VOLTAGE8V,
//   VOLTAGE5V,
//   DIGITAL5V,
//   DIGITAL3V3,
//   OUTPUTUART,
//   OUTPUTI2C,
//   NOOUTPUT
// };
// enum arduinoOptions { RESET, ARDUINOUART, NOARDUINO };
// enum probeOptions { PROBECONNECT, PROBECLEAR, PROBECALIBRATE, NOPROBE };

// struct action {
//   actionCategories Category;
//   showOptions Show;
//   railOptions Rail;
//   slotOptions Slot;
//   outputOptions Output;
//   arduinoOptions Arduino;
//   probeOptions Probe;
//   float floatValue;
//   int intValues[10];
// };

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

int categoryRanges[10][2] = {{-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1},
                             {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}};
int categoryIndex = 0;

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
    if (menuLines[i].startsWith("$")) { // && menuLines[i].endsWith("$")) {
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

    // delay(1000);
    // Serial.print(menuLines[i]);
    // Serial.print(" ");
    // Serial.println(numberOfChoices[i]);

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
      case 's':
        actions[i] = 4;
        break;
      default:
        actions[i] = 0;
        break;
      }
      char actionChar2 = menuLines[i].charAt(actionIndex + 2);
      if (actionChar2 >= '0' && actionChar2 <= '9') {
        numberOfChoices[i] = menuLines[i].charAt(actionIndex + 2) - '0';
      } else {
        // numberOfChoices[i] = 1;
      }

      actionIndex = menuLines[i + 1].indexOf(">");
      if (actionIndex != -1) {
        selectMultiple[selectMultipleIndex] = i;
        selectMultipleIndex++;
      }

      menuLines[i].remove(actionIndex, 3);
    }
  }

  for (int j = 0; j < menuLineIndex; j++) {
    // Serial.println(menuLevels[j]);
    if (menuLevels[j] == 0) {
      categoryRanges[categoryIndex][0] = j;

      if (categoryIndex > 0) {
        categoryRanges[categoryIndex - 1][1] = j - 1;
      }
      categoryIndex++;
    }
  }
  categoryRanges[categoryIndex - 1][1] = menuLineIndex;

  int printMenuLinesAtStartup = 0;
  if (printMenuLinesAtStartup == 1) {
    delay(1000);

    for (int j = 0; j < 10; j++) {
      Serial.print(categoryRanges[j][0]);
      Serial.print(" ");
      Serial.println(categoryRanges[j][1]);
    }
    Serial.println("idx level    line            stay     choices   action");
    for (int i = 0; i <= menuLineIndex; i++) {

      Serial.print(i);
      if (i < 10) {
        Serial.print(" ");
      }
      Serial.print("   ");
      Serial.print(menuLevels[i]);
      Serial.print("   ");
      for (int j = 0; j < menuLevels[i]; j++) {
        Serial.print(" ");
      }
      // Serial.print(" \t");
      int spaces = Serial.print(menuLines[i]);
      for (int j = 0; j < 20 - spaces; j++) {
        Serial.print(" ");
      }
      Serial.print(" ");
      Serial.print(stayOnTop[i]);
      Serial.print(" \t");
      Serial.print(numberOfChoices[i]);
      Serial.print(" \t");
      Serial.print(actions[i]);
      Serial.print(" \t0b");
      for (int j = 0; j < 32; j++) {
        if (j % 8 == 0) {
          Serial.print(" ");
        }
        Serial.print(bitRead(optionSlpitLocations[i], j));
      }

      Serial.print("\n\rMenuLineIndex: ");
      Serial.println(menuLineIndex);
    }
    //   for (int i = 0; i < menuLineIndex; i++) {
    // getActionCategory(i);
    //   }

    menuParsed = 1;
  }
}

uint32_t menuColors[10] = {0x09000a, 0x0f0004, 0x080800, 0x010f00,
                           0x000a03, 0x00030a, 0x040010, 0x070006};

void initMenu(void) {

  delay(10);
  if (menuRead == 0) {
    // Serial.println(menuLines);
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

unsigned long noInputTimer = millis();
unsigned long exitMenuTime = 55000;

int subSelection = -1;

int subMenuChoices[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
int maxNumSelections = 0;

int returnToMenuPosition = -1;
int returnToMenuLevel = -1;
int returningFromTimeout = 0;

char submenuBuffer[20];

char chosenOptions[20][40];

int previousMenuSelection[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

int clickMenu(int menuType, int menuOption, int extraOptions) {
  if (menuLineIndex < 2) {
    //b.clear();
    b.print("No menu file", 0x0f0400, 0xFFFFFF, 0, -1, 0);
    inClickMenu = 0;
    return -1;
  }

  int returnedMenuPosition = -1;
  if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {
    encoderButtonState = IDLE;
    inClickMenu = 1;
    // if (menuRead == 0) {
    //   readMenuFile();
    // }
    // if (menuParsed == 0) {
    //   parseMenuFile();
    // }
    // Serial.setTimeout(1000);
    // Serial.flush();

    // showLoss();
    // while(Serial.available() == 0) ;

    returnedMenuPosition = getMenuSelection();
    while (returnedMenuPosition == -1 && Serial.available() == 0) {
      // delayMicroseconds(5000);
      if (checkProbeButton() == 1) {
        Serial.println("Probe button pressed");
        return -3;
      }
      returnedMenuPosition = getMenuSelection();
    }
    if (returnedMenuPosition == -2) {
      showLEDsCore2 = 1;
      inClickMenu = 0;
      return -2;
    }

    //showLEDsCore2 = 1;

     Serial.print("returnedMenuPosition: ");
    // Serial.println(returnedMenuPosition);
    // Serial.print("subSelection: ");
    // Serial.println(subSelection);
    // Serial.print("actions: ");
    // Serial.println(actions[returnedMenuPosition]);
    // Serial.print("subMenuChoices: ");
    // for (int i = 0; i < 8; i++) {
    //   Serial.print(subMenuChoices[i]);
    //   Serial.print(", ");
    // }
    // Serial.println();
    // populateAction();
    //  doMenuAction(returnedMenuPosition);

    // getMenuSelection();
  }
  inClickMenu = 0;
  return returnedMenuPosition;
}

action currentAction;

int alreadySelected = 0;

void clearAction(void) {
  for (int i = 0; i < 10; i++) {
    currentAction.previousMenuPositions[i] = -1;
    currentAction.connectOrClear[i] = -1;
  }
  for (int i = 0; i < 20; i++) {
    currentAction.from[i] = -1;
    currentAction.to[i] = -1;
  }
  for (int i = 0; i < 20; i++) {
    for (int j = 0; j < 10; j++) {
      currentAction.fromAscii[i][j] = ' ';
    }
  }
  currentAction.previousMenuIndex = 0;
  currentAction.connectIndex = 0;
  currentAction.optionVoltage = 0;
  currentAction.baud = 0;
  currentAction.printOrUSB = 0;
  currentAction.analogVoltage = 0.0;
}

void populateAction(void) {
  int actionIndex = 0;
  int connectIndex = 0;
  for (int i = 0; i < 10; i++) {
    if (previousMenuSelection[i] != -1) {
      currentAction.previousMenuPositions[actionIndex] =
          previousMenuSelection[i];
      // currentAction.connectOrClear[connectIndex] = 1;
      actionIndex++;
      // connectIndex++;
    }
  }
  currentAction.previousMenuIndex = actionIndex;
  /// currentAction.connectIndex = connectIndex;
  currentAction.optionVoltage = optionVoltage;
}

void printActionStruct(void) {

  int spaces = 0;
  Serial.println("\n\r");
  Serial.print("\tpreviousMenuPositions: \t");
  for (int i = 0; i < 10; i++) {
    if (currentAction.previousMenuPositions[i] != -1) {
      Serial.print(menuLines[currentAction.previousMenuPositions[i]]);
      Serial.print(", ");
    }
  }
  Serial.println();
  Serial.print("\tconnectOrClear: \t");
  for (int i = 0; i < 10; i++) {
    if (currentAction.connectOrClear[i] != -1) {
      Serial.print(currentAction.connectOrClear[i]);
      Serial.print(", ");
    }
  }
  Serial.println();
  Serial.print("\tfrom: \t\t");
  for (int i = 0; i < 10; i++) {

    Serial.print(currentAction.from[i]);
    Serial.print(",\t");
  }
  Serial.println();
  Serial.print("\tto: \t\t");
  for (int i = 0; i < 10; i++) {
    Serial.print(currentAction.to[i]);
    Serial.print(",\t");
  }
  Serial.println();
  Serial.print("\tfromAscii: \t");
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 10; j++) {
      if (currentAction.fromAscii[i][j] != ' ' &&
          currentAction.fromAscii[i][j] != 0) {
        Serial.print(currentAction.fromAscii[i][j]);
      }
    }
    Serial.print(",\t");
  }
  Serial.println();
  Serial.print("\tpreviousMenuIndex: ");
  Serial.println(currentAction.previousMenuIndex);
  Serial.print("\tconnectIndex: ");
  Serial.println(currentAction.connectIndex);
  Serial.print("\toptionVoltage: ");
  Serial.println(currentAction.optionVoltage);
  Serial.print("\tanalogVoltage: ");
  Serial.println(currentAction.analogVoltage);
}

int getMenuSelection(void) {

  optionVoltage = 0;
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
  int back = 0;
  subSelection = -1;

  // for (int i = 0; i < 10; i++) {
  //   if (selectMultiple[i] != 0) {
  //     Serial.print("Select multiple: ");
  //     Serial.println(selectMultiple[i]);
  //   }
  // }
  delay(10);

  for (int i = 0; i < 10; i++) {
    previousMenuSelection[i] = -1;
    subMenuChoices[i] = -1;
  }

  int force = 0;

  clearAction();

  clearLEDsExceptRails();
  // delay(100);
  if (returnToMenuPosition != -1 && returnToMenuLevel != -1) {
    menuPosition = returnToMenuPosition;
    returnToMenuPosition = -1;

    menuLevel = returnToMenuLevel;
    returnToMenuLevel = -1;
    returningFromTimeout = 1;
    // force = 1;
  }

  noInputTimer = millis();
  while (Serial.available() == 0) {

    if (Serial.getWriteError()) {

      Serial.clearWriteError();
      Serial.flush();
      Serial.println("Serial error");
    }
    /// rotaryDivider = 9;
    // delayMicroseconds(1000);
    rotaryDivider = 8;
    if (encoderButtonState == DOUBLECLICKED ||
        millis() - noInputTimer > exitMenuTime) {
      encoderButtonState = IDLE;
      lastButtonEncoderState = IDLE;

      returnToMenuPosition = menuPosition;
      returnToMenuLevel = menuLevel;
      b.clear();
      return -2;
    }
    delayMicroseconds(4000);

    if (encoderDirectionState == UP || firstTime == 1 || force == 1) { // ||
      // lastMenuLevel > menuLevel) {
      encoderDirectionState = NONE;
      firstTime = 0;
      // currentAction.Category
      noInputTimer = millis();
      lastMenuPosition = menuPosition;
      if (returningFromTimeout == 0) {
        menuPosition += 1;
      }
      returningFromTimeout = 0;
      if (menuPosition > subMenuEndIndex) {
        menuPosition = subMenuStartIndex;
      }

      int loopCount = 0;
      for (int i = menuPosition; i <= menuLineIndex; i++) {
        // Serial.print(i);
        if (menuLevels[i] == menuLevel) {
          // Serial.println("^\n\r");
          menuPosition = i;
          // currentAction.Category = getActionCategory(menuPosition);
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
          returnToMenuPosition = -1;
          returnToMenuLevel = -1;
          // doMenuAction(menuPosition);

          // for (int i = 0; i < 8; i++) {
          //   if (subMenuChoices[i] != -1) {
          //     Serial.print(menuLines[subMenuChoices[i]]);
          //     Serial.print(" > ");
          //   }
          // }

          // Serial.println();
          int keepSelecting = 0;

          for (int i = 0; i < 10; i++) {
            if (selectMultiple[i] == menuPosition) {
              keepSelecting = 1;
            }
            if (i >= selectMultipleIndex) {
              break;
            }
          }

          if (keepSelecting == 0) {
            delayMicroseconds(100);

            return doMenuAction();
          } else {
            encoderButtonState = RELEASED;
            lastButtonEncoderState = PRESSED;
            // menuPosition++;
            break;
          }
          break;
          // break;
        }
      }

      Serial.print("\r                                              \r");

      for (int j = 0; j <= menuLevels[menuPosition]; j++) {
        Serial.print(">");
        if (j > 8) {
          break;
        }
      }
      // Serial.print(menuLevel);
      Serial.print(" ");
      Serial.print(menuLines[menuPosition]);

      if (stayOnTopLevel != -1 && stayOnTopIndex != -1 &&
          menuLevel != stayOnTopLevel) {
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
      showLEDsCore2 = 2;
      lastMenuLevel = menuLevel;
      // previousMenuSelection[menuLevel] = menuPosition;
      //  b.print(menuPosition);

    } else if (encoderDirectionState == DOWN || (lastMenuLevel < menuLevel)) {
      encoderDirectionState = NONE;
      noInputTimer = millis();
      // lastMenuLevel = menuLevel;
      lastMenuPosition = menuPosition;
      // if (back == 1) {
      //   back = 0;
      // } else {
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
            // currentAction.Category = getActionCategory(menuPosition);
            break;
          }
          if (i == subMenuStartIndex && loopCount == 0) {
            loopCount++;

            i = subMenuEndIndex + 1;
          } else if (i == subMenuStartIndex && loopCount == 1) {

            Serial.println("!!! ");

            b.clear();
            returnToMenuPosition = -1;
            returnToMenuLevel = -1;

            // for (int i = 0; i < 8; i++) {
            //   if (chosen[i] != -1) {
            //     Serial.print(menuLines[chosen[i]]);
            //     Serial.print(" -> ");
            //   }
            // }

            if (actions[menuPosition] == 3 && subSelection != -1) {

              // Serial.println("get float voltage");
              getActionFloat(menuPosition);
            }
            int keepSelecting = 0;

            for (int i = 0; i < 10; i++) {
              if (selectMultiple[i] == menuPosition) {
                keepSelecting = 1;
              }
              if (i >= selectMultipleIndex) {
                break;
              }
            }

            if (keepSelecting == 0) {

              return doMenuAction();
            } else {
              encoderButtonState = RELEASED;
              lastButtonEncoderState = PRESSED;
              // menuPosition++;
              break;
            }
            break;
          }
        }
        // Serial.println("Fuck");
      }
      // }
      //         Serial.print("   \t\t");
      // Serial.println(menuPosition);
      // if (menuLevel == lastMenuLevel) {
      Serial.print("\r                                              \r");
      //} else {
      // Serial.println();
      //}
      for (int j = 0; j <= menuLevels[menuPosition]; j++) {
        Serial.print(">");
        if (j > 8) {
          break;
        }
      }
      // Serial.print(menuLevel);
      delayMicroseconds(100);
      Serial.print(" ");
      Serial.print(menuLines[menuPosition]);
      // sprintf(chosenOptions[menuLevel],menuLines[menuPosition].c_str());
      // Serial.print(chosenOptions[menuLevel]);

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
          //  Serial.println("fuck");
          nextOption = selectSubmenuOption(lastMenuPosition, lastMenuLevel);

          if (nextOption == -1) {
            // Serial.println("-1");
            break;
          }
          // Serial.println(nextOption);
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
        returnToMenuPosition = -1;
        returnToMenuLevel = -1;

        // menuLevel++;

        // if (keepSelecting == 0) {

        return doMenuAction();
        // } else {
        //   encoderButtonState = RELEASED;
        //   lastButtonEncoderState = PRESSED;
        //  // menuPosition++;
        //  break;
        // }

      } else if (actions[menuPosition] == 3 && subSelection != -1) {

        // Serial.println("get float voltage");
        getActionFloat(menuPosition);

        // doMenuAction();
        return doMenuAction();

      } else {

        if ((stayOnTopLevel != -1 && stayOnTopIndex != -1) &&
            menuLevel != stayOnTopLevel) {
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
            // Serial.print("subselection: ");
            // Serial.println(subSelection);
            encoderButtonState = RELEASED;
            lastButtonEncoderState = PRESSED;
          }
          lastMenuPosition = menuPosition;

          force = 0;
        }
      }

      showLEDsCore2 = 2;
      lastMenuLevel = menuLevel;
      // previousMenuSelection[menuLevel] = menuPosition;

      // b.print(menuPosition);
    }

    delayMicroseconds(80);

    if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {

      lastMenuLevel = menuLevel;
      noInputTimer = millis();

      // chosen[chosenIndex] = menuPosition;
      // chosenType[chosenIndex] = 3;
      // chosenIndex++;

      if (stayOnTop[menuPosition] == 1) {
        stayOnTopLevel = menuLevel;
        stayOnTopIndex = menuPosition;
      }

      previousMenuSelection[menuLevel] = menuPosition;

      // Serial.print("[ ");
      // for (int i = 0; i < 10; i++) {
      //   if (previousMenuSelection[i] != -1) {
      //   Serial.print(menuLines[previousMenuSelection[i]]);
      //   Serial.print(", ");
      //   }
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

      // Serial.println();
      while (encoderButtonState == RELEASED)
        ;
    }

    if (encoderButtonState == HELD) {
      noInputTimer = millis();
      lastMenuLevel = menuLevel;
      // Serial.println("Held");

      // if (stayOnTopLevel == menuLevel) {

      stayOnTopLevel = -1;
      stayOnTopIndex = -1;
      //}
      firstTime = 1;
      int noPrevious = 0;

      for (int i = menuLevel + 1; i < 10; i++) {
        previousMenuSelection[i] = -1;
      }

      if (menuLevel > 1) {
        menuLevel -= 1;
        // b.clear();
        // return 0;
      } else if (menuLevel == 1) {
        menuLevel = 0;
        // menuPosition = 0;
      } else {
        b.clear();
        return -2;
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
      // back = 1;
      //  delay(500); // you'll step back a level every 500ms
      noInputTimer = millis();

      while (encoderButtonState == HELD) {
        if (millis() - noInputTimer > 1000) {
          encoderButtonState = IDLE;
          break;
        }
      }

      // if (encoderDirectionState == DOWN || encoderDirectionState == UP) {
      //   encoderButtonState = IDLE;

      //   break;
      // }
      // this determines if you just
      // co back to the top level or if you go back to the previous level
      //   ;
    }
  }

  return -1;
}

// char *findSubMenuString(int menuPosition, int selection) {
//   String subMenuStrings[8];
//   int menuOptionLengths[8];
//   int maxMenuOptionLength = 0;
//   for (int i = 0; i < 8; i++) {
//     subMenuStrings[i] = " ";
//   }
//   for (int i = 0; i < 20; i++) {
//     submenuBuffer[i] = 0;
//   }

//   int shiftStars = -2;
//   int lastOption = 0;
//   for (int i = 0; i < 8; i++) {
//     int optionStart = -1;
//     int optionEnd = -1;

//     for (int j = lastOption; j < 32; j++) {
//       if (bitRead(optionSlpitLocations[menuPosition], j) == 1) {
//         if (optionStart == -1) {
//           optionStart = j;
//           shiftStars++;
//         } else {
//           optionEnd = j;
//           shiftStars++;
//           lastOption = j + 1;
//           break;
//         }
//       }
//     }
//     // Serial.print("optionStart: ");
//     // Serial.println(optionStart);
//     if (optionStart != -1 && optionEnd != -1) {
//       subMenuStrings[i] = menuLines[menuPosition].substring(
//           optionStart - shiftStars, optionEnd - shiftStars - 1);
//       if (subMenuStrings[i].length() > maxMenuOptionLength) {
//         menuOptionLengths[i] = subMenuStrings[i].length();
//         maxMenuOptionLength = subMenuStrings[i].length();
//       }
//     }
//   }

//   subMenuStrings[selection].toCharArray(submenuBuffer, 20, 0);

//   return submenuBuffer;
// }

uint32_t nodeSelectionColors[10] = {
    0x0f0700, 0x00090f, 0x0a000f, 0x050d00,
    0x100500, 0x000411, 0x100204, 0x020f02,
};
uint32_t nodeSelectionColorsHeader[10] = {
    0x151000, 0x00153f, 0x0e003f, 0x0f2d03,
    0x180d00, 0x0000af, 0x1a004f, 0x061f29,
};

int selectSubmenuOption(int menuPosition, int menuLevel) {

  rotaryDivider = 16;
  delayMicroseconds(3000);
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
  // for (int i = 0; i < 8; i++) {
  //   if (subMenuStrings[i] != "") {

  //     Serial.print(subMenuStrings[i]);
  //     Serial.print(" ");
  //   }
  // }
  uint32_t subMenuColors[10] = {
      0x000f02, 0x000a03, 0x00030a, 0x040010,
      0x100001, 0x0d0200, 0x080900, 0x030e00,
  };

  uint32_t subMenuColorsHeader[10] = {
      0x001f09, 0x001508, 0x00061f, 0x070020,
      0x200005, 0x120600, 0x0f1200, 0x061200,
  };

  //   uint32_t subMenuColors[10] = {
  //     0x010f00, 0x000a03, 0x00030a, 0x040010,
  //     0x070006, 0x09000a, 0x0f0004, 0x080800,
  // };

  //   uint32_t nodeSelectionColors[10] = {
  //     0x0f0700, 0x00090f, 0x0a000f, 0x050d00,
  //     0x100500, 0x000411, 0x100204, 0x020f02,
  // };
  uint32_t selectColor = subMenuColors[(menuLevel + 5) % 8];
  uint32_t backgroundColor = 0xffffff; // 0x0101000;
  int changed = 1;

  String choiceLine = menuLines[menuPosition];
  int brightnessMenu = 0;
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
  // if (previousMenuSelection[1] != -1) {
  //   Serial.print("previousMenuSelection[1]: ");

  //   Serial.print(previousMenuSelection[1]);
  //   Serial.print(" ");
  //   Serial.println(menuLines[previousMenuSelection[1]]);
  // }

  if (menuLines[previousMenuSelection[1]].indexOf("Load") != -1) {
    // Serial.println("Load");
    menuType = 3;
  }

  if (menuLines[previousMenuSelection[1]].indexOf("Bright") != -1) {

    brightnessMenu = 1;
  }
  // Serial.print("menuType: ");
  // Serial.println(menuType);
  // Serial.println("selected Submenu Option\n\r");

  encoderButtonState = IDLE;

  int firstTime = 1;
  delayMicroseconds(1000);
  while (optionSelected == -1) {
    delayMicroseconds(1000);

    if (encoderButtonState == DOUBLECLICKED || encoderButtonState == HELD ||
        Serial.available() > 0) {
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

      for (int i = 0; i < 10; i++) {
        alreadySelected = 0;
        if (currentAction.from[i] != -1 &&
            currentAction.from[i] == optionSelected) {
          alreadySelected = 1;
          break;
        }
      }
      if (alreadySelected == 0) {
        currentAction.from[currentAction.connectIndex] = optionSelected;
      }

      // char menuBuffer[20];
      // Serial.print("Selected: ");
      // Serial.println(subMenuStrings[optionSelected]);
      if (subMenuStrings[optionSelected].indexOf("V") != -1) {
        if (subMenuStrings[optionSelected].indexOf("3") != -1) {
          optionVoltage = 3;
          currentAction.optionVoltage = 3;
        } else if (subMenuStrings[optionSelected].indexOf("5") != -1) {
          optionVoltage = 5;
          currentAction.optionVoltage = 5;
        } else if (subMenuStrings[optionSelected].indexOf("8") != -1) {
          optionVoltage = 8;
          currentAction.optionVoltage = 8;
        }

      } else {
        if (alreadySelected == 0) {
          subMenuStrings[optionSelected].toCharArray(
              currentAction.fromAscii[currentAction.connectIndex], 10, 0);
        }
      }

      // currentAction.fromAscii[currentAction.connectIndex][0] = menuBuffer;

      // rotaryDivider = 8;

      return optionSelected;
      changed = 1;
    }
    if (changed == 1) {

      b.clear(1);

      if (menuType == 0) {

        if (brightnessMenu == 1) {
          if (highlightedOption == 0) {
            selectColor = subMenuColors[(menuLevel + 5) % 8] & 0x030303;
          } else if (highlightedOption == 1) {
            selectColor = subMenuColors[(menuLevel + 5) % 8] & 0x070707;
          }

          else {
            selectColor = subMenuColors[(menuLevel + 5) % 8] *
                          (((highlightedOption - 1)));
          }
        }

        b.print(subMenuStrings[highlightedOption].c_str(), selectColor,
                backgroundColor, 3, 1, 0);

        // Serial.println(selectColor, HEX);
        int start = highlightedOption;
        int loopCount = 0;
        int nudge = 0;
        Serial.print("\r                      \r");

        Serial.print(subMenuStrings[highlightedOption]);

        for (int i = 0; i < 7; i++) {

          if (i != (3)) {
            b.print(subMenuStrings[(i + highlightedOption + 5) % 8].c_str(),
                    subMenuColors[menuLevel], 0xFFFFFF, i, 1, nudge, 1);
            // Serial.print(subMenuStrings[(i + highlightedOption + 5) % 8]);
          } else {
            // Serial.print("\b");
            // Serial.println((i + highlightedOption + 5) % 8);
            // Serial.println(subMenuStrings[(i + highlightedOption + 5) % 8]);
            // Serial.print(" ");
          }
        }

      } else if (menuType == 1) {

        int menuStartPositions[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        for (int i = 0; i < 8; i++) {
          menuStartPositions[i] = 0;
        }
        int nextStart = 0;
        // selectColor = 0x1a001a;

        Serial.print("\r                      \r");

        Serial.print(subMenuStrings[highlightedOption]);
        for (int i = 0; i < numberOfChoices[menuPosition]; i++) {

          if (i == highlightedOption) {
            b.printRawRow(0b00001010, (nextStart * 4) + 30, selectColor,
                          0xffffff);
            b.printRawRow(0b00000100, (nextStart * 4) + 31, selectColor,
                          0xffffff);
            b.print(subMenuStrings[i].c_str(), selectColor, 0xffffff, nextStart,
                    1, 1);
            //                    Serial.print(subMenuStrings[i]);
            // Serial.print(" ");
          } else {
            b.print(subMenuStrings[i].c_str(), subMenuColors[menuLevel],
                    0xffffff, nextStart, 1, 1);
          }

          nextStart += subMenuStrings[i].length() + 1;

          // Serial.println(subMenuStrings[i].length());
        }

      } else if (menuType == 2) {

        // Serial.println(subMenuColors[(highlightedOption + menuLevel) % 8],
        // HEX);
        b.printRawRow(0b00001010, 30, selectColor, 0xffffff);
        b.printRawRow(0b00000100, 31, selectColor, 0xffffff);
        b.print(subMenuStrings[highlightedOption].c_str(),
                subMenuColors[(highlightedOption + menuLevel) % 8],
                backgroundColor, 0, 1, 1);

        Serial.print("\r                      \r");

        Serial.print(subMenuStrings[highlightedOption]);
      } else if (menuType == 3) {
        // Serial.println(subMenuColors[(highlightedOption + menuLevel) % 8],

        // inClickMenu = 0;
        // Serial.println(highlightedOption);
        if (highlightedOption < 0) {
          highlightedOption = 0;
        } else if (highlightedOption > 7) {
          highlightedOption = 7;
        }

        lightUpRail();
        showSavedColors(highlightedOption);
        b.print(subMenuStrings[highlightedOption].c_str(),
                nodeSelectionColors[highlightedOption], 0xFFFFFF, 3, 1, 0, 0);
        showNets();
        leds.setPixelColor(bbPixelToNodesMapV5[highlightedOption + 16][1],
                           nodeSelectionColorsHeader[highlightedOption]);
        showLEDsCore2 = 1;
        // inClickMenu = 1;
      }
      showLEDsCore2 = 2;

      changed = 0;
    }

    // b.print(choiceLine.c_str(), subMenuColors[menuLevel], 0xFFFFFF, 0, 1,
    //         numberOfChoices[menuPosition] > 7 ? 1 : 3);

    // delay(6800);
  }
  // Serial.println(highlightedOption);
  return optionSelected;
}

int selectNodeAction(int whichSelection) {
  b.clear();
  showLEDsCore2 = 1;
  // delayMicroseconds(100000);

  int nodeSelected = -1;
  int currentlySelecting = whichSelection;

  Serial.print("Currently Selecting: ");
  Serial.println(currentlySelecting);
  Serial.println();

  int highlightedNode = currentlySelecting + 13;
  int firstTime = 1;
  int inNanoHeader = 0;

  uint8_t middle = 0b00001110;
  uint32_t middleColor = 0x121215;
  uint8_t hatch = 0b00011111;
  uint32_t overlappingColor = 0xffffff;

  if (subMenuChoices[currentlySelecting] != -1) {
    if (subMenuChoices[currentlySelecting] >= NANO_D0 &&
        subMenuChoices[currentlySelecting] <= NANO_RESET_0) {
      highlightedNode = subMenuChoices[currentlySelecting];
      inNanoHeader = 1;
    } else {

      highlightedNode = subMenuChoices[currentlySelecting] + 1;
    }
    subMenuChoices[currentlySelecting] = -1;
    maxNumSelections++;
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
  rotaryDivider = 4;
  delayMicroseconds(3000);

  unsigned long scrollAccelerationTimer = micros();
  unsigned long scrollAccelerationDelay = 29000;
  int scrollAcceleration = 1;
  int scrollAccelerationDirection = 0; // 0 = up, 1 = down
  encoderDirectionStates lastScrollAccelerationDirection =
      NONE; // 0 = up, 1 = down
  int accelCount = 0;
  // Serial.println();
  // for (int a = 0; a < 8; a++) {
  //   Serial.print(subMenuChoices[a]);
  //   Serial.print(", ");
  // }
  // Serial.println();
  // while (1) {
  //   for (int i = 0; i < 10; i++) {
  //     leds.setPixelColor(bbPixelToNodesMapV5[i * 2][1],
  //     nodeSelectionColors[i]); leds.setPixelColor(bbPixelToNodesMapV5[(i * 2)
  //     + 1][1],
  //                        nodeSelectionColorsHeader[i]);

  //     leds.setPixelColor(i * 5, nodeSelectionColors[i]);
  //     leds.setPixelColor((i * 5)+1, nodeSelectionColors[i]);
  //     leds.setPixelColor((i * 5) + 2, nodeSelectionColors[i]);
  //     leds.setPixelColor((i * 5) + 3, nodeSelectionColors[i]);
  //     leds.setPixelColor((i * 5) + 4, nodeSelectionColors[i]);

  //     leds.setPixelColor(((i+30) * 5) + 0, nodeSelectionColorsHeader[i]);
  //     leds.setPixelColor(((i+30) * 5) + 1, nodeSelectionColorsHeader[i]);
  //     leds.setPixelColor(((i+30) * 5) + 2, nodeSelectionColorsHeader[i]);
  //     leds.setPixelColor(((i+30) * 5) + 3, nodeSelectionColorsHeader[i]);
  //     leds.setPixelColor(((i+30) * 5) + 4, nodeSelectionColorsHeader[i]);
  //   }
  //   showLEDsCore2 = 3;
  // }

  while (nodeSelected == -1 && Serial.available() == 0) {
    // rotaryEncoderStuff();
    delayMicroseconds(400);
    if (encoderButtonState == DOUBLECLICKED || encoderButtonState == HELD ||
        Serial.available() > 0) {
      b.clear();
      rotaryDivider = 8;
      return -1;
    }

    if (encoderDirectionState == DOWN || encoderDirectionState == UP ||
        firstTime == 1) {

      if (firstTime != 1) {
        // Serial.println(micros() - scrollAccelerationTimer);
        if (micros() - scrollAccelerationTimer < scrollAccelerationDelay ||
            abs(numberOfSteps) >= 2) {

          accelCount++;
          // Serial.print(encoderRaw);
          // Serial.print(" ");
          // Serial.println(numberOfSteps);
          // scrollAcceleration = scrollAcceleration + accelStep;
          switch (accelCount) {
          case 1:
            rotaryDivider = 2;

            break;

          case 4:
            scrollAcceleration = 2;
            // rotaryDivider = 1;

            // scrollAcceleration = 3;
            break;
          case 8:
            // scrollAcceleration = 24;
            break;
          }
          // Serial.println(scrollAcceleration);
          if (scrollAcceleration > 24) {
            scrollAcceleration = 24;
          }
        } else {
          scrollAcceleration = 1;
          rotaryDivider = 4;
          accelCount = 0;
        }
        scrollAccelerationTimer = micros();

        if (encoderDirectionState == DOWN &&
            lastScrollAccelerationDirection == NONE) {

          highlightedNode -= scrollAcceleration;
          if (highlightedNode < 0) {
            highlightedNode = NANO_RESET_0;
            inNanoHeader = 1;
            // highlightedNode = 59;
          }
          if (highlightedNode < NANO_D0 && inNanoHeader == 1) {
            highlightedNode = 59;
            inNanoHeader = 0;

            for (int i = 0; i < 30; i++) {
              if (leds.getPixelColor(bbPixelToNodesMapV5[i][1]) ==
                  nodeSelectionColorsHeader[currentlySelecting]) {
                leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
              }
              // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
            }

            // lightUpRail();
            // showNets();
            for (int a = 0; a < 8; a++) {
              if (subMenuChoices[a] != -1 && subMenuChoices[a] >= NANO_D0) {
                leds.setPixelColor(
                    bbPixelToNodesMapV5[subMenuChoices[a] - 70][1],
                    nodeSelectionColorsHeader[a]);

              } else if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
                b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                              nodeSelectionColors[a]);
              }
            }
          }

        } else if (encoderDirectionState == UP &&
                   lastScrollAccelerationDirection == NONE) {

          highlightedNode += scrollAcceleration;
          if (highlightedNode > 59 && inNanoHeader == 0) {

            highlightedNode = NANO_D0;
            inNanoHeader = 1;
          }
          if (highlightedNode > NANO_RESET_0) {
            highlightedNode = 0;
            inNanoHeader = 0;

            for (int i = 0; i < 30; i++) {
              if (leds.getPixelColor(bbPixelToNodesMapV5[i][1]) ==
                  nodeSelectionColorsHeader[currentlySelecting]) {
                leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
              }
              // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
            }

            // lightUpRail();
            // showNets();
            for (int a = 0; a < 8; a++) {
              if (subMenuChoices[a] != -1 && subMenuChoices[a] >= NANO_D0) {
                leds.setPixelColor(
                    bbPixelToNodesMapV5[subMenuChoices[a] - 70][1],
                    nodeSelectionColorsHeader[a]);

              } else if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
                b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                              nodeSelectionColors[a]);
              }
            }
          }
        }
      }
      lastScrollAccelerationDirection = encoderDirectionState;
      encoderDirectionState = NONE;
      firstTime = 0;

      // highlightedNode -= 1;

      int overlappingSelection = -1;
      int overlappingConnection = -1;

      b.clear();
      showNets();
      showLEDsCore2 = 4;

      if (inNanoHeader == 1) {
        for (int i = 0; i < 30; i++) {
          if (leds.getPixelColor(bbPixelToNodesMapV5[i][1]) ==
              nodeSelectionColorsHeader[currentlySelecting]) {
            leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
          }
          // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
        }

        // lightUpRail();
        // showNets();
        for (int a = 0; a < 8; a++) {
          if (subMenuChoices[a] != -1 && subMenuChoices[a] >= NANO_D0) {
            leds.setPixelColor(bbPixelToNodesMapV5[subMenuChoices[a] - 70][1],
                               nodeSelectionColorsHeader[a]);

          } else if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
            b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                          nodeSelectionColors[a]);
          }
        }
        leds.setPixelColor(bbPixelToNodesMapV5[highlightedNode - 70][1],
                           nodeSelectionColorsHeader[currentlySelecting]);

      } else {

        if (leds.getPixelColor((highlightedNode * 5) + 3) != 0x000000) {

          overlappingConnection = highlightedNode + 1;
        }
        for (int a = 0; a < 8; a++) {
          if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
            if (subMenuChoices[a] == highlightedNode + 1 &&
                subMenuChoices[a] < 60) {
              hatch = 0b00010101;
              overlappingColor = nodeSelectionColors[a];
              overlappingSelection = a;
            }

            b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                          nodeSelectionColors[a]);

            // b.print(subMenuChoices[a], nodeSelectionColors[a], 0xffffff, a,
            // 1, 1);
          }
        }

        if (overlappingConnection != -1 || overlappingSelection != -1) {
          if (highlightedNode <= 30) {
            b.printRawRow(middle, (highlightedNode),
                          leds.getPixelColor(((highlightedNode) * 5) + 4),
                          nodeSelectionColors[currentlySelecting]);
          } else {
            b.printRawRow(middle, (highlightedNode),
                          leds.getPixelColor(((highlightedNode) * 5) + 0),
                          nodeSelectionColors[currentlySelecting]);
          }
          b.printRawRow(0b00000100, (highlightedNode + 1),
                        nodeSelectionColors[currentlySelecting], 0x000000);
          // leds.getPixelColor(((highlightedNode + 1) * 5) + 3));
          b.printRawRow(0b00000100, (highlightedNode - 1),
                        nodeSelectionColors[currentlySelecting], 0x000000);
          // leds.getPixelColor(((highlightedNode - 1) * 5) + 3));
        } else {
          b.printRawRow(0b00000100, (highlightedNode), middleColor,
                        nodeSelectionColors[currentlySelecting]);
        }
      }
      // Serial.print("\r                        \r");
      // Serial.print(highlightedNode + 1);
      showLEDsCore2 = 2;
      // leds.show();

    } else if (encoderButtonState == RELEASED &&
               lastButtonEncoderState == PRESSED) {
      encoderButtonState = IDLE;
      nodeSelected = highlightedNode;

      if (alreadySelected == 0) {

        currentAction.to[currentAction.connectIndex] = highlightedNode + 1;

        currentAction.connectIndex++;
      } else {
        for (int i = 0; i < 10; i++) {
          if (currentAction.from[i] == currentlySelecting) {
            if (nodeSelected > 0 && nodeSelected < 60) {
              currentAction.to[i] = highlightedNode + 1;
            } else {
              currentAction.to[i] = highlightedNode;
            }

            break;
          }
        }
      }

    } else {
      lastScrollAccelerationDirection = NONE;
    }

    // } else if (encoderButtonState == HELD) {

    //   highlightedNode += 30;
    //   highlightedNode = highlightedNode % 60;
    //   if (highlightedNode > 59) {
    //     highlightedNode = 0;
    //   }
    //   b.clear();
    //   uint8_t hatch = 0b00011111;
    //   uint32_t overlappingColor = 0xffffff;
    //   for (int a = 0; a < 8; a++) {
    //     if (subMenuChoices[a] != -1) {
    //       if (subMenuChoices[a] == highlightedNode + 1) {
    //         hatch = 0b00010101;
    //         overlappingColor = nodeSelectionColors[a];
    //       }

    //       b.printRawRow(0b00011111, (subMenuChoices[a] - 1),
    //                     nodeSelectionColors[a], 0xffffff);

    //       // b.print(subMenuChoices[a], nodeSelectionColors[a], 0xffffff,
    //       a, 1,
    //       // 1);
    //     }
    //     b.printRawRow(hatch, (highlightedNode),
    //                   nodeSelectionColors[currentlySelecting],
    //                   overlappingColor);
    //   }
    //   showLEDsCore2 = 2;
    //   // leds.show();
    //   while (encoderButtonState == HELD)
    //     ;
    // }

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
  if (nodeSelected <= 59 && nodeSelected >= 0) {
    return nodeSelected + 1;
  } else {
    return nodeSelected;
  }
}

float getActionFloat(int menuPosition) {
  float currentChoice = -0.1;

  char floatString[8] = "0.0";
  rotaryDivider = 4;
  b.clear(1);
  int firstTime = 1;

  uint32_t posColor = 0x090600;
  uint32_t negColor = 0x04000f;

  uint32_t threeColor = 0x00060e;
  uint32_t fiveColor = 0x140101;
  uint32_t maxColor = 0x100505;
  uint32_t zeroColor = 0x000e02;

  uint32_t fiveBlended = 0x0e0300;
  uint32_t threeBlended = 0x060f00;
  uint32_t zeroBlended = 0x060801;

  uint32_t numberColor = posColor;

  float scrollAcceleration = 0.1;
  unsigned long lastScrollTime = 0;
  unsigned long scrollAccelTime = 40000;
  float min = -8.0;
  float max = 8.0;

  if (currentAction.optionVoltage == 5) {
    min = 0.0;
    max = 5.0;
  } else if (currentAction.optionVoltage == 3) {
    min = 0.0;
    max = 3.3;
  } else if (currentAction.optionVoltage == 8) {
    min = -8.0;
    max = 8.0;
  }

  while (encoderButtonState != HELD && Serial.available() == 0) {
    delayMicroseconds(3800);

    if (encoderDirectionState == UP || firstTime == 1) {
      firstTime = 0;
      encoderDirectionState = NONE;
      if (micros() - lastScrollTime < scrollAccelTime) {
        scrollAcceleration += 0.1;
        if (scrollAcceleration > 0.6) {
          scrollAcceleration = 0.6;
        }
      } else {
        scrollAcceleration = 0.1;
      }
      lastScrollTime = micros();

      currentChoice += scrollAcceleration;

      if (currentChoice > max) {
        currentChoice = max;
      } else if (currentChoice < min) {
        currentChoice = min;
      }
      if (currentChoice > 0.05) {
        numberColor = posColor;
      }

      if (currentChoice < 5.3 && currentChoice > 4.7) {
        numberColor = fiveBlended;
      } else if (currentChoice < 3.45 && currentChoice > 3.05) {
        numberColor = threeBlended;
      } else if (currentChoice < 8.25 && currentChoice > 7.75) {
        scrollAcceleration = 0.1;
      } else if (currentChoice < 0.35 && currentChoice > -0.35) {
        numberColor = zeroBlended;
      }

      if (currentChoice > -0.05 && currentChoice < 0.05) {
        numberColor = zeroColor;
      } else if (currentChoice > 3.25 && currentChoice < 3.35) {
        numberColor = threeColor;
        // delayMicroseconds(8000);
      } else if (currentChoice > 4.95 && currentChoice < 5.05) {
        numberColor = fiveColor;
        // delayMicroseconds(8000);
      } else if (currentChoice > 7.95 && currentChoice < 8.55) {
        numberColor = maxColor;
      } else if (currentChoice < -0.05) {
        numberColor = negColor;
      }
      // } else if (currentChoice > 0.05) {
      //   numberColor = posColor;
      // }
      if (currentChoice == -0.0) {
        currentChoice = 0.0;
      }
      if (currentChoice <= 0.0) {
        snprintf(floatString, 7, "%0.1f V", currentChoice);
      } else {
        snprintf(floatString, 7, "%0.1f  V", currentChoice);
      }
      b.clear(1);
      b.print(floatString, numberColor, 0xffffff, 0, 1, 1);
      Serial.print("\r                        \r");
      Serial.print(floatString);
      showLEDsCore2 = 2;
      // Serial.println(floatString);

    } else if (encoderDirectionState == DOWN) {
      encoderDirectionState = NONE;
      if (micros() - lastScrollTime < scrollAccelTime) {
        scrollAcceleration += 0.1;
        if (scrollAcceleration > 0.6) {
          scrollAcceleration = 0.6;
        }

      } else {
        scrollAcceleration = 0.1;
      }
      lastScrollTime = micros();

      currentChoice -= scrollAcceleration;

      if (currentChoice > max) {
        currentChoice = max;
      } else if (currentChoice < min) {
        currentChoice = min;
      }
      if (currentChoice > 0.05) {
        numberColor = posColor;
      }
      if (currentChoice < 5.3 && currentChoice > 4.7) {
        numberColor = fiveBlended;
      } else if (currentChoice < 3.75 && currentChoice > 3.00) {
        numberColor = threeBlended;
      } else if (currentChoice < 8.25 && currentChoice > 7.75) {
        scrollAcceleration = 0.1;
      } else if (currentChoice < 0.55 && currentChoice > -0.55) {
        numberColor = zeroBlended;
      }

      if (currentChoice > -0.05 && currentChoice < 0.05) {
        numberColor = zeroColor;
      } else if (currentChoice > 3.25 && currentChoice < 3.35) {
        numberColor = threeColor;
      } else if (currentChoice > 4.95 && currentChoice < 5.05) {
        numberColor = fiveColor;
      } else if (currentChoice > 7.95 && currentChoice < 8.55) {
        numberColor = maxColor;
      } else if (currentChoice < -0.05) {
        numberColor = negColor;
      }
      if (currentChoice < 0.0) {
        snprintf(floatString, 7, "%0.1f V", currentChoice);
      } else {
        snprintf(floatString, 7, "%0.1f  V", currentChoice);
      }
      b.clear(1);
      b.print(floatString, numberColor, 0xffffff, 0, 1, 1);
      Serial.print("\r                        \r");
      Serial.print(floatString);
      // Serial.println(currentChoice);

      showLEDsCore2 = 2;
    } else if (encoderButtonState == RELEASED &&
               lastButtonEncoderState == PRESSED) {
      encoderButtonState = IDLE;
      currentAction.analogVoltage = currentChoice;

      int keepSelecting = 0;
      // Serial.println (menuPosition);

      for (int i = 0; i < 10; i++) {
        if (selectMultiple[i] == menuPosition) {
          keepSelecting = 1;
        }
        if (i >= selectMultipleIndex) {
          break;
        }
      }
      if (keepSelecting == 1) {
        // currentAction.
        selectNodeAction();
      }
      return currentChoice;

      // Serial.println(floatString);
    }
  }
  return currentChoice;
}
//>n nodes 1 //>b baud 2 //>v voltage 3

// subSelection
int doMenuAction(int menuPosition, int selection) {

  populateAction();
  printActionStruct();

  actionCategories currentCategory;

  if (menuLines[currentAction.previousMenuPositions[0]].indexOf("Slots") !=
      -1) {
    currentCategory = SLOTSACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Rails") != -1) {
    currentCategory = RAILSACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Show") != -1) {
    currentCategory = SHOWACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Output") != -1) {
    currentCategory = OUTPUTACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Arduino") != -1) {
    currentCategory = ARDUINOACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Probe") != -1) {
    currentCategory = PROBEACTION;
  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                 "Display") != -1) {
    currentCategory = DISPLAYACTION;

  } else {
    currentCategory = NOCATEGORY;
  }

  if (currentCategory == SHOWACTION) {

    Serial.print("Show Action\n\r");
    if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Voltage") !=
        -1) {

      printActionStruct();

      for (int i = 0; i < 10; i++) {
        if (currentAction.from[i] != -1) {
          switch (currentAction.from[i]) {
          case 0:
            addBridgeToNodeFile(ADC1, currentAction.to[i], netSlot);
            break;
          case 1:

            addBridgeToNodeFile(ADC2, currentAction.to[i], netSlot);
            break;
            // break;
          case 2:

            addBridgeToNodeFile(ADC3, currentAction.to[i], netSlot);
            break;
          default:
            break;
          }

          // break;
        }
      }

    } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                   "Current") != -1) {

      // printActionStruct();

      for (int i = 0; i < 10; i++) {
        if (currentAction.from[i] != -1) {
          switch (currentAction.from[i]) {
          case 0:
            addBridgeToNodeFile(ISENSE_PLUS, currentAction.to[i], netSlot);
            break;
          case 1:

            addBridgeToNodeFile(ISENSE_MINUS, currentAction.to[i], netSlot);
            break;
            // break;

          default:
            break;
          }

          // break;
        }
      }
    }
    digitalWrite(RESETPIN, HIGH);

    delayMicroseconds(200);

    digitalWrite(RESETPIN, LOW);

    showSavedColors(netSlot);
    sendAllPathsCore2 = 1;
    chooseShownReadings();

    slotChanged = 0;

    return 10;
    // loadingFile = 1;

  } else if (currentCategory == RAILSACTION) {

    Serial.print("Rails Action\n\r");

    switch (currentAction.from[0]) {
    case 0: {
      setTopRail(currentAction.analogVoltage);
      delayMicroseconds(100);
      setBotRail(currentAction.analogVoltage);
      break;
    }
    case 1: {
      // delay(100);
      setTopRail(currentAction.analogVoltage);
      break;
    }
    case 2: {
      // delay(100);
      setBotRail(currentAction.analogVoltage);
      break;
    }
    default: {
      break;
    }
    }

  } else if (currentCategory == SLOTSACTION) {

    Serial.print("Slots Action\n\r");

    if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Save") !=
        -1) {

      saveCurrentSlotToSlot(netSlot, currentAction.from[0]);

    } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                   "Load") != -1) {

      netSlot = currentAction.from[0];
      return currentAction.from[0];
    }

  } else if (currentCategory == OUTPUTACTION) {

    Serial.print("Output Action\n\r");

  } else if (currentCategory == ARDUINOACTION) {

    Serial.print("Arduino Action\n\r");

  } else if (currentCategory == PROBEACTION) {

    Serial.print("Probe Action\n\r");

  } else if (currentCategory == DISPLAYACTION) {

    if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Jumpers") !=
        -1) {
      if (currentAction.from[0] == 0) {
        displayMode = 1;
      } else {
        displayMode = 0;
      }
      debugFlagSet(12);
    } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                   "Bright") != -1) {
      int brightnessOptionMap[] = {2, 3, 5, 7, 10, 14, 17, 20, 25, 32, 36};
      LEDbrightness = (brightnessOptionMap[currentAction.from[0]]);
      saveLEDbrightness(0);
      showNets();
      showLEDsCore2 = 2;
    }
    Serial.print("Display Action\n\r");

  } else if (currentCategory == NOCATEGORY) {

    Serial.print("No Category\n\r");
  }

  return 1;
}

String categoryNames[] = {"Show",    "Rails", "Slots",     "Output",
                          "Arduino", "Probe", "NoCategory"};

actionCategories getActionCategory(int menuPosition) {

  for (int i = 0; i < categoryIndex; i++) {
    if (menuPosition >= categoryRanges[i][0] &&
        menuPosition < categoryRanges[i][1]) {
      Serial.print("Category: ");
      Serial.println(categoryNames[i]);
      Serial.println("\n\r");
      return (actionCategories)i; // this assumes the enum is in the right order
    }
  }

  return NOCATEGORY;
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

        showLEDsCore2 = 2;
      } else if (input2 == '-') {
        LEDbrightness -= 1;

        if (LEDbrightness < 2) {
          LEDbrightness = 1;
        }

        showLEDsCore2 = 2;
      } else if (input2 == 'x' || input2 == ' ' || input2 == 'm') {
        input = ' ';
      } else {
      }
      showNets();

      // for (int i = 8; i <= numberOfNets; i++) {
      //   lightUpNet(i, -1, 1, LEDbrightness, 0);
      // }
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

        showLEDsCore2 = 2;
      } else if (input2 == '-') {

        LEDbrightnessRail -= 1;

        if (LEDbrightnessRail < 2) {
          LEDbrightnessRail = 1;
        }

        showLEDsCore2 = 2;
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

        // showLEDsCore2 = 2;
      } else if (input2 == '-') {

        LEDbrightnessSpecial -= 1;

        if (LEDbrightnessSpecial < 2) {
          LEDbrightnessSpecial = 1;
        }

        // showLEDsCore2 = 2;
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

void showLoss(void) {
  b.clear();

  uint32_t guyColor = 0x000011;
  uint32_t hairColor = 0x000011;
  uint32_t nurseColor = 0x08010f;
  uint32_t doctorColor = 0x050509;
  uint32_t patientColor = 0x04060f;

  b.printRawRow(0b00001111, 5, guyColor, hairColor);

  b.printRawRow(0b00001111, 20, guyColor, hairColor);
  b.printRawRow(0b00001111, 25, nurseColor, 0xffffff);

  b.printRawRow(0b00001111, 34, guyColor, hairColor);
  b.printRawRow(0b00011111, 38, doctorColor, 0xffffff);

  b.printRawRow(0b00001111, 49, guyColor, hairColor);
  b.printRawRow(0b00000010, 51, patientColor, 0xffffff);
  b.printRawRow(0b00000010, 52, patientColor, 0xffffff);
  b.printRawRow(0b00000010, 53, patientColor, 0xffffff);
  b.printRawRow(0b00000010, 54, patientColor, 0xffffff);
  b.printRawRow(0b00000010, 55, patientColor, 0xffffff);
  b.printRawRow(0b00000010, 56, patientColor, 0xffffff);
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