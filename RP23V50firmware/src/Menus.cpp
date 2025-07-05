#include "Menus.h"
#include "RotaryEncoder.h"
#include "SafeString.h"
#include <Arduino.h>

#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Commands.h"
#include "FatFS.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "RotaryEncoder.h"
#include "MatrixState.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "configManager.h"
#include "Apps.h"
#include "UserCode.h"
#include "oled.h"
#include "menuTree.h"
#include "SerialWrapper.h"
#include "Highlighting.h"

int inClickMenu = 0;

int menuRead = 0;
int menuLength = 0;
char menuChars[1000];

int menuLineIndex = 0;
//String menuLines[150];
int menuLevels[150];
int stayOnTop[150];
uint8_t numberOfChoices[150];
uint8_t actions[150]; //>n nodes 1 //>b baud 2 //>v voltage 3

uint32_t optionSlpitLocations[150];
int numberOfLevels = 0;
int optionVoltage = 0;

uint8_t selectMultiple[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int selectMultipleIndex = 0;

char hardCodeOptions[10][10] = { "Tx",  "Rx", "SDA",   "SCL",  "5V",
                                "3V3", "8V", "USB 2", "Print" };
int brightnessOptionMap[] = { 3, 4, 6, 9, 10, 14, 18, 26, 32, 42, 48 };
int menuBrightnessOptionMap[] = { -80, -60, -30, -15, 0, 20, 50, 100, 150, 200 };

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


void readMenuFile(int flashOrLocal) {
  // FatFS.begin();
  // delay(10);
  if (flashOrLocal == 0) {
    writeMenuTree();
    // while (core2busy == true) {
    //   // Serial.println("waiting for core2 to finish");
    //   }
    core1busy = true;
    File menuFile = FatFS.open("/MenuTree.txt", "r");
    if (!menuFile) {
      delay(1000);
      Serial.println("Failed to open menu file");
      return;
      core1busy = false;
      }
    menuLength = 0;

    while (menuFile.available()) {
      menuLines[menuLineIndex] = menuFile.readStringUntil('\n');
      menuLineIndex++;
      }
    menuLineIndex--;

    menuRead = 1;

    menuFile.close();
    core1busy = false;
    } else {

    menuLineIndex = 0;

    for (int i = 0; i < 150; i++) {
      // Serial.print(i);
      // Serial.print(": ");
      // Serial.println(menuLines[i]);
      if (menuLines[i] == "end") {
        menuLines[i] = "";
        break;
        } else {
        // menuLines[i] = String(menuTreeStrings[i]);
        menuLineIndex++;
        }
      }

    }
  }

int menuParsed = 0;

int categoryRanges[10][2] = { {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1},
                             {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1}, {-1, -1} };
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

    // Check if line ends with ^ for font selection action
    if (menuLines[i].endsWith("^")) {
      actions[i] = 6;
      menuLines[i].remove(menuLines[i].length() - 1, 1); // Remove the ^
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
        case 'a':
          actions[i] = 5;
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

      // if (menuLines[i].indexOf("^") != -1) {
      //   menuLines[i].remove(actionIndex, 3);
      //   }

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
    delay(2000);

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

uint32_t menuColors[10] = { 0x09000a, 0x0f0004, 0x080800, 0x010f00,
                           0x000a03, 0x00030a, 0x040010, 0x070006 };

void initMenu(void) {

  //FatFS.begin();
 // delay(1);
  if (menuRead == 0) {
    // Serial.println(menuLines);
    //delay(1);
    readMenuFile(1);
    // return 0;
    }
  if (menuParsed == 0) {
    //delay(1);
    parseMenuFile();
    /// return 0;
    }
  }

unsigned long noInputTimer = millis();
unsigned long exitMenuTime = 9995000;

int subSelection = -1;

int subMenuChoices[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
int maxNumSelections = 0;

int returnToMenuPosition = -1;
int returnToMenuLevel = -1;
int returningFromTimeout = 0;

char submenuBuffer[20];

char chosenOptions[20][40];

int previousMenuSelection[10] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

int clickMenu(int menuType, int menuOption, int extraOptions) {
  if (menuLineIndex < 2) {
    // b.clear();
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
    while (returnedMenuPosition == -1 && Serial.available() == 0 && arduinoInReset == 0) {
      // delayMicroseconds(5000);
      if (checkProbeButton() == 1) {
        Serial.println("Probe button pressed");
        return -3;
        }
      //oled.showJogo32h();
      returnedMenuPosition = getMenuSelection();
      }
    if (returnedMenuPosition == -2) {
      showLEDsCore2 = 1;
      inClickMenu = 0;

      oled.showJogo32h();

      return -2;
      }

    // showLEDsCore2 = 1;

   //Serial.print("returnedMenuPosition: ");
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

  //oled.showJogo32h();

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
  Serial.print("\tMenu Path: \t");
  for (int i = 0; i < 10; i++) {
    if (currentAction.previousMenuPositions[i] != -1) {
      if (i > 0) {
        Serial.print(" > ");
        }

      Serial.print(menuLines[currentAction.previousMenuPositions[i]]);
      }
    }
  if (currentAction.connectOrClear[0] != -1) {
    Serial.println();

    Serial.print("\tConnect Or Clear: \t");

    for (int i = 0; i < 10; i++) {
      if (currentAction.connectOrClear[i] != -1) {
        Serial.print(currentAction.connectOrClear[i]);
        Serial.print(", ");
        }
      }
    }
  if (currentAction.from[0] != -1) {
    Serial.println();
    Serial.print("\tfrom: \t\t");
    for (int i = 0; i < 10; i++) {
      if (currentAction.from[i] != -1) {

        Serial.print(currentAction.from[i]);
        Serial.print(",\t");
        }
      }
    }
  if (currentAction.to[0] != -1) {
    Serial.println();
    Serial.print("\tto: \t\t");
    for (int i = 0; i < 10; i++) {
      if (currentAction.to[i] != -1) {
        Serial.print(currentAction.to[i]);
        Serial.print(",\t");
        }
      }
    }
  if (currentAction.fromAscii[0][0] != ' ' &&
      currentAction.fromAscii[0][0] != 0) {
    Serial.println();
    Serial.print("\tfromAscii: \t");
    for (int i = 0; i < 10; i++) {
      if (currentAction.fromAscii[i][0] != ' ' &&
          currentAction.fromAscii[i][0] != 0) {
        for (int j = 0; j < 10; j++) {
          if (currentAction.fromAscii[i][j] != ' ' &&
              currentAction.fromAscii[i][j] != 0) {
            Serial.print(currentAction.fromAscii[i][j]);
            }
          }
        Serial.print(",\t");
        }
      }
    }

  // Serial.println();
  // Serial.print("\tpreviousMenuIndex: ");
  // Serial.println(currentAction.previousMenuIndex);
  if (currentAction.connectIndex != 0) {
    Serial.print("\n\r\tconnectIndex:  ");
    Serial.print(currentAction.connectIndex);
    }
  if (currentAction.optionVoltage != 0.0) {
    Serial.print("\n\r\toptionVoltage:  ");
    Serial.print(currentAction.optionVoltage);
    }
  if (currentAction.analogVoltage != 0.0) {
    Serial.print("\n\r\tanalogVoltage:  ");
    Serial.print(currentAction.analogVoltage);
    }
  Serial.println("\n\r");
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
  int showFonts = 0;
  int lastSubmenuOption = 0;
  int back = 0;
  subSelection = -1;

  // for (int i = 0; i < 10; i++) {
  //   if (selectMultiple[i] != 0) {
  //     Serial.print("Select multiple: ");
  //     Serial.println(selectMultiple[i]);
  //   }
  // }
  //delay(10);



  for (int i = 0; i < 10; i++) {
    previousMenuSelection[i] = -1;
    subMenuChoices[i] = -1;
    }

  int force = 0;

  clearAction();

  clearLEDsExceptRails();
  showLEDsCore2 = -2;
  waitCore2();
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
    rotaryDivider = 4;
    if (millis() - noInputTimer > exitMenuTime) {
      encoderButtonState = IDLE;
      lastButtonEncoderState = IDLE;

      returnToMenuPosition = menuPosition;
      returnToMenuLevel = menuLevel;
      b.clear();
      return -2;
      }
    delayMicroseconds(400);




    if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) { //! click

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
      } else if (encoderDirectionState == UP || firstTime == 1 || force == 1) { // ! up
        // lastMenuLevel > menuLevel) {
        encoderDirectionState = NONE;
        firstTime = 0;
        // currentAction.Category
        resetPosition = true;
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

        delayMicroseconds(100);
        Serial.print(" ");
        if (actions[menuPosition] == 0) {
          Serial.print("\r                                              \r");

          // oled.clear();
          for (int j = 0; j <= menuLevels[menuPosition]; j++) {
            Serial.print(">");

            if (j > 8) {
              break;
              }
            }
            Serial.flush();


          

          String menuLine = menuLines[menuPosition];
          menuLine.replace("~", "±");
          menuLine.replace("_", "-");
          Serial.print(menuLine.c_str());
          Serial.flush();
          menuLine.replace("±", "+-");
            


          oled.clearPrintShow(menuLine.c_str(), 2, true, true, true, -1, -1);
          } else if (actions[menuPosition] == 6) {
            // Map highlightedOption directly to FontFamily enum (now sequential)
            FontFamily previewFont = oled.getFontFamily(menuLines[menuPosition]);

            // Show font preview with both sizes
            oled.clearPrintShow(menuLines[menuPosition], 2, previewFont, true, true, true, -1, -1);
            //oled.clearPrintShow("Preview", 2, previewFont, false, true, true, -1, -1);
            } else {
            //Serial.println(menuLines[menuPosition-1]);
            Serial.println(" ");
            //oled.print(" ");

            }

          /// Serial.print(menuLines[menuPosition]);

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

        } else if (encoderDirectionState == DOWN || (lastMenuLevel < menuLevel)) { //! down
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
        
            }
          // }
          //         Serial.print("   \t\t");
          // Serial.println(menuPosition);
          // if (menuLevel == lastMenuLevel) {

          // Serial.print(menuLevel);
          delayMicroseconds(100);
          Serial.print(" ");
          if (actions[menuPosition] == 0) {
            Serial.print("\r                                              \r");
            // oled.clear();

            for (int j = 0; j <= menuLevels[menuPosition]; j++) {
              Serial.print(">");
              // oled.clearPrintShow(">", 2, 5, 8, false, false);
              if (j > 8) {
                break;
                }
              }

            Serial.flush();

            String menuLine = menuLines[menuPosition];
            menuLine.replace("~", "±");
            menuLine.replace("_", "-");
            Serial.print(menuLine.c_str());
            Serial.flush(); 
            menuLine.replace("±", "+-");

            oled.clearPrintShow(menuLine.c_str(), 2, true, true, true, -1, -1);
            } else if (actions[menuPosition] == 6) {
              // Map highlightedOption directly to FontFamily enum (now sequential)
              FontFamily previewFont = oled.getFontFamily(menuLines[menuPosition]);

              // Show font preview with both sizes
              oled.clearPrintShow(menuLines[menuPosition], 2, previewFont, true, true, true, -1, -1);
              //oled.clearPrintShow("Preview", 2, previewFont, false, true, true, -1, -1);
              } else {
              //Serial.println(menuLines[menuPosition-1]);
              Serial.println(" ");
              //oled.print(" ");

              }


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
                // Serial.println(subSelection);
                // Serial.println(subSelection);
                // Serial.println(subSelection);

                getActionFloat(menuPosition, subSelection);

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



uint32_t nodeSelectionColors[10] = {
    0x0f0700, 0x00090f, 0x0a000f, 0x050d00,
    0x100500, 0x000411, 0x100204, 0x020f02,
  };
uint32_t nodeSelectionColorsHeader[10] = {
    0x151000, 0x00153f, 0x0e003f, 0x0f2d03,
    0x180d00, 0x0000af, 0x1a004f, 0x061f29,
  };



int selectSubmenuOption(int menuPosition, int menuLevel) {

  int railMenu = 0;
  rotaryDivider = 4;
  delayMicroseconds(3000);
  int optionSelected = -1;
  int highlightedOption = 1;
  // Serial.println("\n\r");
  String subMenuStrings[8];
  int menuOptionLengths[8];
  int maxMenuOptionLength = 0;
  int maxExists = -1;
  // int showFonts = 0;
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
        if (strcasecmp(subMenuStrings[i].c_str(), "max ") == 0) {
          maxExists = i;
          } else {
          maxMenuOptionLength = subMenuStrings[i].length();
          }
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
      // Serial.print("previousMenuSelection[1]: ");

      // Serial.print(previousMenuSelection[1]);
      // Serial.print(" ");
      // Serial.println(menuLines[previousMenuSelection[1]]);
    // }

    if (menuLines[previousMenuSelection[1]].indexOf("Load") != -1) {
      // Serial.println("Load");
      menuType = 3;
      }

    if (menuLines[previousMenuSelection[menuLevel - 2]].indexOf("Bright") != -1 && menuLines[previousMenuSelection[menuLevel - 1]].indexOf("Menu") != -1) {

      brightnessMenu = 1;
      }

    if (menuLines[previousMenuSelection[menuLevel - 2]].indexOf("Bright") != -1 && menuLines[previousMenuSelection[menuLevel - 1]].indexOf("Rails") != -1) {

      brightnessMenu = 2;
      }

    if (menuLines[previousMenuSelection[menuLevel - 2]].indexOf("Bright") != -1 && menuLines[previousMenuSelection[menuLevel - 1]].indexOf("Wires") != -1) {

      brightnessMenu = 3;
      }


    if (menuLines[previousMenuSelection[menuLevel - 2]].indexOf("Bright") != -1 && menuLines[previousMenuSelection[menuLevel - 1]].indexOf("Special") != -1) {

      brightnessMenu = 4;
      }

    // if (menuLines[previousMenuSelection[menuLevel - 1]].indexOf("Font") != -1) {
    //   Serial.println("Font\n\r");
    //   showFonts = 1;
    //   }



    // Serial.print("menuType: ");
    // Serial.println(menuType);
    // Serial.println("selected Submenu Option\n\r");

    encoderButtonState = IDLE;
    int lastBrightness = menuBrightnessSetting;
    int firstTime = 1;
    delayMicroseconds(1000);
    while (optionSelected == -1) {
      delayMicroseconds(1000);

      if (encoderButtonState == HELD || Serial.available() > 0) {
        b.clear();
        return -1;
        }
      if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {

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
        } else if (encoderDirectionState == UP) {
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
            }
          if (changed == 1) {

            b.clear(1);

            if (menuType == 0) {

              if (brightnessMenu > 0) {
                // Serial.println();
                // Serial.println(highlightedOption);
                // Serial.println();
                if (brightnessMenu == 1) {
                  switch (highlightedOption) {
                    case 0:
                      menuBrightnessSetting = -70;
                      break;
                    case 1:
                      menuBrightnessSetting = -55;
                      break;
                    case 2:
                      menuBrightnessSetting = -45;
                      break;
                    case 3:
                      menuBrightnessSetting = -35;
                      break;
                    case 4:
                      menuBrightnessSetting = -15;
                      break;
                    case 5:
                      menuBrightnessSetting = -5;
                      break;
                    case 6:
                      menuBrightnessSetting = 0;
                      break;
                    case 7:
                      menuBrightnessSetting = 30;
                      break;
                    case 8:
                      menuBrightnessSetting = 60;
                      break;
                    case 9:
                      menuBrightnessSetting = 60;
                      break;
                    default:
                      menuBrightnessSetting = 0;
                      break;
                    }

                  b.clear();
                  b.print("B", scaleBrightness(menuColors[0], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 0, 0, 3);
                  b.print("r", scaleBrightness(menuColors[1], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 1, 0, 3);
                  b.print("i", scaleBrightness(menuColors[2], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 2, 0, 3);
                  b.print("g", scaleBrightness(menuColors[3], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 3, 0, 3);
                  b.print("h", scaleBrightness(menuColors[4], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 4, 0, 3);
                  b.print("t", scaleBrightness(menuColors[5], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 5, 0, 3);

                  b.printMenuReminder(menuLevel, scaleBrightness(menuColors[menuLevel], menuBrightnessSetting));


                  } else {
                  uint32_t scaledColor[6];
                  hsvColor scaledColorHsv[6];
                  switch (brightnessMenu) {
                    case 2:
                      LEDbrightnessRail = (int)(highlightedOption * 1.35) + 50;
                      jumperlessConfig.display.rail_brightness = LEDbrightnessRail;

                      // for (int i = 0; i < 6; i++) {
                      //   scaledColorHsv[i] = RgbToHsv(menuColors[i]);
                      //   scaledColorHsv[i].v = LEDbrightnessRail;  
                      //   scaledColor[i] = HsvToRaw(scaledColorHsv[i]);
                      // }

                      b.clear();
                      b.print("B", scaleBrightness(menuColors[0], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 0, 0, 3);
                      b.print("r", scaleBrightness(menuColors[1], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 1, 0, 3);
                      b.print("i", scaleBrightness(menuColors[2], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 2, 0, 3);
                      b.print("g", scaleBrightness(menuColors[3], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 3, 0, 3);
                      b.print("h", scaleBrightness(menuColors[4], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 4, 0, 3);
                      b.print("t", scaleBrightness(menuColors[5], menuBrightnessOptionMap[highlightedOption]), 0xffffff, 5, 0, 3);

                      b.printMenuReminder(menuLevel, scaleBrightness(menuColors[menuLevel], menuBrightnessSetting));

                      break;
                    case 3:
                      LEDbrightness = highlightedOption * 3 + 3;
                      jumperlessConfig.display.led_brightness = LEDbrightness;

                      for (int i = 0; i < 6; i++) {
                        scaledColorHsv[i] = RgbToHsv(menuColors[i]);
                        scaledColorHsv[i].v = LEDbrightness;
                        scaledColor[i] = HsvToRaw(scaledColorHsv[i]);
                        }

                      b.clear();
                      b.print("B", scaledColor[0], 0xffffff, 0, 0, 3);
                      b.print("r", scaledColor[1], 0xffffff, 1, 0, 3);
                      b.print("i", scaledColor[2], 0xffffff, 2, 0, 3);
                      b.print("g", scaledColor[3], 0xffffff, 3, 0, 3);
                      b.print("h", scaledColor[4], 0xffffff, 4, 0, 3);
                      b.print("t", scaledColor[5], 0xffffff, 5, 0, 3);
                    case 4:
                      LEDbrightnessSpecial = highlightedOption * 5 + 5;
                      jumperlessConfig.display.special_net_brightness = LEDbrightnessSpecial;

                      for (int i = 0; i < 6; i++) {
                        scaledColorHsv[i] = RgbToHsv(menuColors[i]);
                        scaledColorHsv[i].v = LEDbrightnessSpecial;
                        scaledColor[i] = HsvToRaw(scaledColorHsv[i]);
                        }

                      b.clear();
                      b.print("B", scaledColor[0], 0xffffff, 0, 0, 3);
                      b.print("r", scaledColor[1], 0xffffff, 1, 0, 3);
                      b.print("i", scaledColor[2], 0xffffff, 2, 0, 3);
                      b.print("g", scaledColor[3], 0xffffff, 3, 0, 3);
                      b.print("h", scaledColor[4], 0xffffff, 4, 0, 3);
                      b.print("t", scaledColor[5], 0xffffff, 5, 0, 3);
                      break;
                    }
                  }
                // b.print("Bright" , menuColors[menuLevel-1],
                //         0xFFFFFF, 0, -1, 3);
                // b.printMenuReminder(menuLevel, menuColors[menuLevel]);

                // b.print("n", menuColors[6], 0xffffff, 1, 1, 2);
                // b.print("e", menuColors[4], 0xffffff, 2, 1, 2);
                // b.print("s", menuColors[2], 0xffffff, 3, 1, 2);
                // b.print("s", menuColors[0], 0xffffff, 4, 1, 2);

                if (highlightedOption == 0) {//!
                  selectColor = subMenuColors[(menuLevel + 5) % 8] & 0x030303;
                  } else if (highlightedOption == 1) {
                    selectColor = subMenuColors[(menuLevel + 5) % 8] & 0x070707;
                    }

                  else {
                  selectColor = subMenuColors[(menuLevel + 5) % 8] *
                    (((highlightedOption - 1)));
                  }
                menuBrightnessSetting = lastBrightness;
                }

              b.print(subMenuStrings[highlightedOption].c_str(), selectColor,
                      backgroundColor, 3, 1, 0);

              // Serial.println(selectColor, HEX);
              int start = highlightedOption;
              int loopCount = 0;
              int nudge = 0;
              int moveMax = 0;
              Serial.print("\r                          \r");
              for (int j = 0; j <= menuLevels[menuPosition]; j++) {
                Serial.print(">");
                if (j > 8) {

                  break;
                  }
                }
              Serial.print(" ");

              // Serial.print(subMenuStrings[highlightedOption]);

              // if (showFonts == 1) {

              //   Serial.println(highlightedOption);
              //   oled.setFont(highlightedOption);
              //   }

              String menuLine = subMenuStrings[highlightedOption];
              menuLine.replace("~", "±");
              menuLine.replace("_", "-");
              
              // Add the previous menu level prefix to menuLine if available
              if (menuLevel > 0 && previousMenuSelection[menuLevel - 1] != -1) {
                String prefix = menuLines[previousMenuSelection[menuLevel - 1]];
                prefix += " ";
                menuLine = prefix + menuLine;
              }
              
              Serial.print(menuLine.c_str());
              menuLine.replace("±", "+-");

              oled.clearPrintShow(menuLine.c_str(), 2, true, true, true, 5, 8);
              Serial.flush();

              for (int i = 0; i < 7; i++) {

                if (i != (3)) {
                  if (i < 3) {
                    nudge = -1;
                    } else {
                    nudge = 1;
                    }
                  if (maxExists == (i + highlightedOption + 5) % 8) {

                    // nudge = 5;
                    }

                  b.print(subMenuStrings[(i + highlightedOption + 5) % 8].c_str(),
                          subMenuColors[menuLevel], 0xFFFFFF, i, 1, nudge, 1);
                  // Serial.print(subMenuStrings[(i + highlightedOption + 5) % 8]);
                  } else {
                  // Serial.print("\b");
                  // Serial.println((i + highlightedOption + 5) % 8);
                  // Serial.println(subMenuStrings[(i + highlightedOption + 5) % 8]);
                  //  Serial.print("\r                          \r");
                  //  Serial.print(subMenuStrings[(i + highlightedOption + 5) % 8]);
                  //  Serial.flush();
                  }
                }

              } else if (menuType == 1) {

                int menuStartPositions[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
                for (int i = 0; i < 8; i++) {
                  menuStartPositions[i] = 0;
                  }
                int nextStart = 0;
                // selectColor = 0x1a001a;

                Serial.print("\r                        \r");
                for (int j = 0; j <= menuLevels[menuPosition]; j++) {
                  Serial.print(">");
                  if (j > 8) {

                    break;
                    }
                  }
                Serial.print(" ");
               // Serial.print(subMenuStrings[highlightedOption]);

                if (actions[menuPosition] == 6) {
                  oled.setFont(highlightedOption);
                  }

                String menuLine = subMenuStrings[highlightedOption];
                menuLine.replace("~", "±");
                menuLine.replace("_", "-");
                
                // Add the previous menu level prefix to menuLine if available
                if (menuLevel > 0 && previousMenuSelection[menuLevel - 1] != -1) {
                  String prefix = menuLines[previousMenuSelection[menuLevel - 1]];
                  prefix += " ";
                  menuLine = prefix + menuLine;
                }
                
                Serial.print(menuLine.c_str());
                menuLine.replace("±", "+-");
                oled.clearPrintShow(menuLine.c_str(), 2, true, true, true, 5, 8);

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

                  Serial.print("\r                              \r");
                  oled.clear();
                  for (int j = 0; j <= menuLevels[menuPosition]; j++) {
                    Serial.print(">");
                    if (j > 8) {

                      break;
                      }
                    }
                  Serial.print(" ");
                   //Serial.print(subMenuStrings[highlightedOption]);

                  String menuLine = subMenuStrings[highlightedOption];
                  menuLine.replace("~", "±");
                  menuLine.replace("_", "-");


                  if (menuLine.startsWith("Top"))
                  {
                    menuLine.replace("'", "↑");
                    menuLine.replace("Top R", "Top Rail");
                    brightenedRail = 0;
                  } else if (menuLine.startsWith("Bot"))
                  {
                    menuLine.replace(",", "↓");
                    menuLine.replace("Bot R", "Bottom Rail");
                    brightenedRail = 2;
                  } else {
                    brightenedRail = -1;
                  }

                  if (menuLine.startsWith("DAC 0"))
                  {
                    DACcolorOverride0 = -2;
                    DACcolorOverride1 = -1;
                  } else if (menuLine.startsWith("DAC 1"))
                  {
                    DACcolorOverride1 = -2;
                    DACcolorOverride0 = -1;
                  } else {
                    DACcolorOverride0 = -1;
                    DACcolorOverride1 = -1;
                  }


                  // // Add the previous menu level prefix to menuLine if available
                  // if (menuLevel > 0 && previousMenuSelection[menuLevel - 1] != -1) {
                  //   String prefix = menuLines[previousMenuSelection[menuLevel - 1]];
                  //   prefix += " ";
                  //   menuLine = prefix + menuLine;
                  // }
                  
                  Serial.print(menuLine.c_str());
                  //Serial.println();
                  Serial.flush();


                  menuLine.replace("±", "+-");
                  oled.clearPrintShow(menuLine.c_str(), 2, true, true, true, 5, 8);




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

                      showNets();
                      leds.setPixelColor(bbPixelToNodesMapV5[highlightedOption + 16][1],
                                         nodeSelectionColorsHeader[highlightedOption]);
                      b.print(subMenuStrings[highlightedOption].c_str(),
                              nodeSelectionColors[highlightedOption], 0xFFFFFD, 3, 1, -1, 0);
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

int yesNoMenu(unsigned long timeout)
  {
  inClickMenu = 1;

  rotaryDivider = 4;
  //delayMicroseconds(3000);
  int optionSelected = -1;
  int highlightedOption = 0;
  int changed = 1;
  uint32_t selectColor = 0x1a001a;
  uint32_t yesColor = 0x001004;
  uint32_t yesColorBright = 0x001f0f;
  uint32_t noColor = 0x100003;
  uint32_t noColorBright = 0x1f0008;
  uint32_t backgroundColor = 0x000002;
  int firstTime = 1;
  unsigned long startTime = millis();
  while (optionSelected == -1)
    {
    if (millis() - startTime > timeout) {
      inClickMenu = 0;
      return -1;
      }



    if (Serial.available() > 0) {
      char input = Serial.read();
      if (input == 'y' || input == 'Y')
        {
        highlightedOption = 1;
        } else if (input == 'n' || input == 'N')
          {
          highlightedOption = 0;
          } else
          {
          //b.clear();
          inClickMenu = 0;
          return -1;
          }
          inClickMenu = 0;
          return highlightedOption;
      }


    delayMicroseconds(1000);

    if (encoderButtonState == HELD)
      {
      b.clear();
      inClickMenu = 0;
      return -1;
      }
    if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED)
      {

      encoderButtonState = IDLE;
      optionSelected = highlightedOption;
      inClickMenu = 0;
      return optionSelected;
      changed = 1;
      } else if (encoderDirectionState == UP)
        {
        encoderDirectionState = NONE;
        highlightedOption += 1;
        if (highlightedOption > 1)
          {
          highlightedOption = 0;
          }
        changed = 1;
        } else if (encoderDirectionState == DOWN || firstTime == 1)
          {
          encoderDirectionState = NONE;
          highlightedOption -= 1;
          if (highlightedOption < 0)
            {
            highlightedOption = 1;
            }
          firstTime = 0;
          changed = 1;
          }
        if (changed == 1)
          {
          Serial.print("\r                      \r");

          b.clear(1);
          if (highlightedOption == 1)
            {
            Serial.print("Yes");
            b.print(">", yesColorBright, 0x0, 0, 1, -2);
            b.print("Yes", yesColorBright, 0x0, 1, 1, -2);
            b.print("No", noColor, 0x0, 5, 1, -1);
            } else
            {
            Serial.print("No");
            b.print(">", noColorBright, 0x0, 4, 1, -1);
            b.print("Yes", yesColor, 0x0, 1, 1, -2);
            b.print("No", noColorBright, 0x0, 5, 1, -1);
            }


            showLEDsCore2 = 2;

            changed = 0;
          }
    }
  inClickMenu = 0;
  return optionSelected;





  }

int selectNodeAction(int whichSelection) {
  b.clear();
  showLEDsCore2 = -1;
  // delayMicroseconds(100000);

  int nodeSelected = -1;
  int currentlySelecting = whichSelection;

  // Serial.print("\n\rCurrently Selecting: ");
  // Serial.println(currentlySelecting);
  // Serial.println();

  int highlightedNode = currentlySelecting + 13;
  uint32_t highlightedNodeColor = 0x000000;

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
  delayMicroseconds(300);

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
    if (encoderButtonState == HELD || Serial.available() > 0) {
      b.clear();
      rotaryDivider = 4;
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
          rotaryDivider = 3;
          accelCount = 0;
          }
        scrollAccelerationTimer = micros();

        if (encoderDirectionState == DOWN &&
            lastScrollAccelerationDirection == NONE) {
          //for (int i = 0; i < 30; i++) {
            //if (leds.getPixelColor(bbPixelToNodesMapV5[i][1]) == highlightedNodeColor) {
          leds.setPixelColor(bbPixelToNodesMapV5[highlightedNode - 70][1], 0x000000);

          // }
         // leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
       //  }
          highlightedNode -= scrollAcceleration;
          if (highlightedNode < 0) {
            highlightedNode = NANO_RESET_0;
            inNanoHeader = 1;
            // highlightedNode = 59;
            }
          if (highlightedNode < NANO_D0 && inNanoHeader == 1) {
            highlightedNode = 59;
            inNanoHeader = 0;



            // lightUpRail();
            // showNets();
            for (int a = 0; a < 8; a++) {
              if (subMenuChoices[a] != -1 && subMenuChoices[a] >= NANO_D0) {
                leds.setPixelColor(
                    bbPixelToNodesMapV5[subMenuChoices[a] - 70][1],
                    nodeSelectionColorsHeader[a]);
                highlightedNodeColor = nodeSelectionColorsHeader[a];
                } else if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
                  b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                                nodeSelectionColors[a]);
                  highlightedNodeColor = nodeSelectionColors[a];
                  }
              }

            }
          Serial.print("\r                      \r");

          Serial.print(">>>> ");
          printNodeOrName(highlightedNode, 1);
          oled.clearPrintShow("> ", 2, true, false);
          oled.clearPrintShow(definesToChar(highlightedNode, 0), 2, false, true);
          // oled.clrPrintfsh(">>>> %s", definesToChar(highlightedNode, 1));
          //oled.show();
         // oled.clearPrintShow(">>>>", 3, 5, 5, true);

          } else if (encoderDirectionState == UP &&
                     lastScrollAccelerationDirection == NONE) {
          leds.setPixelColor(bbPixelToNodesMapV5[highlightedNode - 70][1], 0x000000);

          highlightedNode += scrollAcceleration;

          //for (int i = 0; i < 30; i++) {

            //leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
          //  }
          if (highlightedNode > 59 && inNanoHeader == 0) {

            highlightedNode = NANO_D0;
            inNanoHeader = 1;

            }
          if (highlightedNode > NANO_RESET_0) {
            highlightedNode = 0;
            inNanoHeader = 0;

            for (int i = 0; i < 30; i++) {
              // if (leds.getPixelColor(bbPixelToNodesMapV5[i][1]) ==
              //     nodeSelectionColorsHeader[currentlySelecting]) {
              leds.setPixelColor(bbPixelToNodesMapV5[i][1], 0x000000);
              //   }
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
          Serial.print("\r                      \r");

          Serial.print(">>>> ");
          printNodeOrName(highlightedNode, 1);

          //oled.clearPrintShow("> ", 2, true, false);

          oled.clearPrintShow(definesToChar(highlightedNode, 0), 2, false, true);

          // oled.clrPrintfsh(">>>> %s", definesToChar(highlightedNode, 1));

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
      showLEDsCore2 = 2;

      if (inNanoHeader == 1) {


        for (int a = 0; a < 8; a++) {
          if (subMenuChoices[a] != -1 && subMenuChoices[a] >= NANO_D0) {
            leds.setPixelColor(bbPixelToNodesMapV5[subMenuChoices[a] - 70][1],
                               nodeSelectionColorsHeader[a]);

            } else if (subMenuChoices[a] != -1 && subMenuChoices[a] < 60) {
              b.printRawRow(0b00000100, (subMenuChoices[a] - 1), middleColor,
                            nodeSelectionColors[a]);
              } else { //make this not clear things that are already lit up
              // leds.setPixelColor(bbPixelToNodesMapV5[highlightedNode - 70][1], 0x000000);
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


    }
  rotaryDivider = 4;
  if (nodeSelected <= 59 && nodeSelected >= 0) {
    return nodeSelected + 1;
    } else {
    return nodeSelected;
    }
  }

float getActionFloat(int menuPosition, int rail) {
  float currentChoice = -0.1;
  float snapValues[9] = { 1.0, 2.0, 3.0, 3.3, 4.0, 5.0, 6.0, 7.0 };

  int snap = 0; //!make this a config variable

  char floatString[8] = "0.0";
  rotaryDivider = 3;
  b.clear(1);
  int firstTime = 1;
  int snapToValue = 0;

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

  float tempRailVoltage[2] = { 0.0, 0.0 };
  tempRailVoltage[0] = railVoltage[0];
  tempRailVoltage[1] = railVoltage[1];
  // Serial.println("fuck");
  switch (rail) {
    case 0:
      currentChoice = ((railVoltage[0] + railVoltage[1]) / 2.0) - 0.1;
      // Serial.print("[0] ");
      // Serial.println(railVoltage[0]);
      // Serial.print("[1] ");
      // Serial.println(railVoltage[1]);
      // Serial.println(currentChoice);
      break;
    case 1:
      currentChoice = railVoltage[0] - 0.1; // it adds 0.1 on the first loop
      break;
    case 2:
      currentChoice = railVoltage[1] - 0.1;
      break;
    }

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
        delayMicroseconds(380);

        if (encoderDirectionState == UP || firstTime == 1) {

          encoderDirectionState = NONE;
          if (snapToValue > 0) {
            snapToValue--;
            continue;
            } else {
            snapToValue = 0;
            }
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
          if (currentChoice < jumperlessConfig.dacs.limit_max) {
            //currentChoice += scrollAcceleration;
            } else {
            currentChoice = jumperlessConfig.dacs.limit_max;
            }

          //currentChoice += scrollAcceleration;

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
                          if (currentChoice > -0.05 && currentChoice < 0.05) {
                            currentChoice = 0.0;
                            }
                          if (currentChoice < 0.00) {
                            snprintf(floatString, 8, "%0.1f V", currentChoice);
                            } else {
                            snprintf(floatString, 8, " %0.1f V", currentChoice);
                            }
                          b.clear(1);
                          b.print(floatString, numberColor, 0xffffff, 0, 1, 1);
                          Serial.print("\r                        \r");
                          Serial.print(floatString);
                          // oled.setTextSize(3);
                          // oled.clrPrintfsh("%s", floatString);
                          oled.clearPrintShow(floatString, 2, true, true, true);
                          if (rail == 0) {
                            railVoltage[0] = currentChoice;
                            railVoltage[1] = currentChoice;
                            } else if (rail == 1) {
                              railVoltage[0] = currentChoice;
                              } else if (rail == 2) {
                                railVoltage[1] = currentChoice;
                                }

                              if (firstTime == 0 && currentChoice > 0.0 && currentChoice <= 5.0) {
                                switch (rail) {
                                  case 0:
                                    setTopRail(currentChoice, 0, 0);
                                    setBotRail(currentChoice, 0, 0);
                                    break;
                                  case 1:
                                    setTopRail(currentChoice, 0, 0);
                                    break;
                                  case 2:
                                    setBotRail(currentChoice, 0, 0);
                                    break;
                                  }


                                }
                              if (snapToValue == 0 && snap != 0) {
                                for (int i = 0; i < 8; i++) {
                                  if ((abs(currentChoice) > snapValues[i] - 0.05 && abs(currentChoice) < snapValues[i] + 0.05)) {
                                    snapToValue = 3;
                                    }
                                  }
                                }
                              showLEDsCore2 = 2;


                              firstTime = 0;
                              // Serial.println(floatString);

          } else if (encoderDirectionState == DOWN) {
            encoderDirectionState = NONE;

            if (snapToValue > 0 && snap != 0) {
              snapToValue--;
              continue;
              } else {
              snapToValue = 0;
              }
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
            if (currentChoice > jumperlessConfig.dacs.limit_min) {

              } else {
              currentChoice = jumperlessConfig.dacs.limit_min;
              }



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
                      currentChoice = 0.0;
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
                            if (currentChoice < 0.00) {
                              snprintf(floatString, 8, "%0.1f V", currentChoice);
                              } else {
                              snprintf(floatString, 8, " %0.1f V", currentChoice);
                              }
                            b.clear(1);
                            b.print(floatString, numberColor, 0xffffff, 0, 1, 1);
                            Serial.print("\r                        \r");
                            Serial.print(floatString);
                            // oled.clear();
                            // oled.print(floatString);
                            // oled.show();
                            //oled.clearPrintShow(floatString, 1, 0, 0, true);
                            // oled.setTextSize(3);
                            // oled.clrPrintfsh("%s", floatString);
                            oled.clearPrintShow(floatString, 2, true, true, true);
                            // Serial.println(currentChoice);
                            if (rail == 0) {
                              railVoltage[0] = currentChoice;
                              railVoltage[1] = currentChoice;
                              } else if (rail == 1) {
                                railVoltage[0] = currentChoice;
                                } else if (rail == 2) {
                                  railVoltage[1] = currentChoice;
                                  }

                                if (currentChoice > 0.0 && currentChoice <= 5.0) {
                                  switch (rail) {
                                    case 0:
                                      setTopRail(currentChoice, 0, 0);
                                      setBotRail(currentChoice, 0, 0);
                                      break;
                                    case 1:
                                      setTopRail(currentChoice, 0, 0);
                                      break;
                                    case 2:
                                      setBotRail(currentChoice, 0, 0);
                                      break;
                                    }


                                  }
                                if (snapToValue == 0 && snap != 0) {
                                  for (int i = 0; i < 8; i++) {
                                    if ((abs(currentChoice) > snapValues[i] - 0.05 && abs(currentChoice) < snapValues[i] + 0.05)) {
                                      snapToValue = 3;
                                      }
                                    }
                                  }


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
      railVoltage[0] = tempRailVoltage[0];
      railVoltage[1] = tempRailVoltage[1];
      return currentChoice;
  }
//>n nodes 1 //>b baud 2 //>v voltage 3

// subSelection

actionCategories getActionCategory(void) {

  if (menuLines[currentAction.previousMenuPositions[0]].indexOf("Slots") !=
    -1) {
    return SLOTSACTION;
    } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
      "Rails") != -1) {
      return RAILSACTION;
      } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
        "Show") != -1) {
        return SHOWACTION;
        } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
          "Output") != -1) {
          return OUTPUTACTION;
          } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
            "Arduino") != -1) {
            return ARDUINOACTION;
            } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
              "Probe") != -1) {
              return PROBEACTION;
              } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                "Display") != -1) {
                return DISPLAYACTION;

                } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                  "Apps") != -1) {
                  return APPSACTION;

                  } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                    "Routing") != -1) {
                    return ROUTINGACTION;

                    } else if (menuLines[currentAction.previousMenuPositions[0]].indexOf(
                      "OLED") != -1) {
                      return OLEDACTION;

                      } else {
                      return NOCATEGORY;
                      }

                    return NOCATEGORY;
  }

int defconDisplay = -1;
int doMenuAction(int menuPosition, int selection) {

  populateAction();
  printActionStruct();
  clearLEDsExceptRails();
  showLEDsCore2 = -1;
  actionCategories currentCategory = getActionCategory();


  if (currentCategory == SHOWACTION) { //!Show 

    Serial.print("Show Action\n\r");
    if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Voltage") !=
        -1) {

      // printActionStruct();

      for (int i = 0; i < 10; i++) {
        if (currentAction.from[i] != -1) {
          switch (currentAction.from[i]) {
            case 0:
              addBridgeToNodeFile(ADC0, currentAction.to[i], netSlot);
              break;
            case 1:

              addBridgeToNodeFile(ADC1, currentAction.to[i], netSlot);
              break;
              // break;
            case 2:

              addBridgeToNodeFile(ADC2, currentAction.to[i], netSlot);
              break;
            case 3:
              addBridgeToNodeFile(ADC3, currentAction.to[i], netSlot);
              break;
            case 4:
              addBridgeToNodeFile(ADC4, currentAction.to[i], netSlot);
              break;

            default:
              break;
            }

          // break;
          }
        }

      refreshConnections();

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
        } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Digital") != -1) {
          if (menuLines[currentAction.previousMenuPositions[2]].indexOf("GPIO") != -1) {
            for (int i = 0; i < 10; i++) {
              if (currentAction.from[i] != -1) {
                switch (currentAction.from[i]) {
                  case 0:
                    addBridgeToNodeFile(RP_GPIO_1, currentAction.to[i], netSlot);
                    gpioState[0] = 2;
                    break;
                  case 1:
                    addBridgeToNodeFile(RP_GPIO_2, currentAction.to[i], netSlot);
                    gpioState[1] = 2;
                    break;
                  case 2:
                    addBridgeToNodeFile(RP_GPIO_3, currentAction.to[i], netSlot);
                    gpioState[2] = 2;
                    break;
                  case 3:
                    addBridgeToNodeFile(RP_GPIO_4, currentAction.to[i], netSlot);
                    gpioState[3] = 2;
                    break;
                  case 4:
                    addBridgeToNodeFile(RP_GPIO_5, currentAction.to[i], netSlot);
                    gpioState[4] = 2;
                    break;
                  case 5:
                    addBridgeToNodeFile(RP_GPIO_6, currentAction.to[i], netSlot);
                    gpioState[5] = 2;
                    break;
                  case 6:
                    addBridgeToNodeFile(RP_GPIO_7, currentAction.to[i], netSlot);
                    gpioState[6] = 2;
                    break;
                  case 7:
                    addBridgeToNodeFile(RP_GPIO_8, currentAction.to[i], netSlot);
                    gpioState[7] = 2;
                    break;
                  default:
                    break;
                  }
                updateGPIOConfigFromState();
                }
              }
            }
          }

        // digitalWrite(RESETPIN, HIGH);

        // delayMicroseconds(200);

        // digitalWrite(RESETPIN, LOW);

        // showSavedColors(netSlot);
        // sendPaths();
        // sendAllPathsCore2 = 1;
        // chooseShownReadings();

        refreshConnections();

        slotChanged = 0;

        return 10;
        // loadingFile = 1;

    } else if (currentCategory == RAILSACTION) { //!Rails

      Serial.print("Rails Action\n\r");
      showLEDsCore2 = 1;
      waitCore2();

      switch (currentAction.from[0]) {
        case 0: {
        setTopRail(currentAction.analogVoltage, 1, 1);
        delayMicroseconds(100);
        setBotRail(currentAction.analogVoltage, 1, 1);
        break;
        }
        case 1: {
        // delay(100);
        setTopRail(currentAction.analogVoltage, 1, 1);
        break;
        }
        case 2: {
        // delay(100);
        setBotRail(currentAction.analogVoltage, 1, 1);
        break;
        }
        default: {
        break;
        }
        }

        configChanged = true;

      } else if (currentCategory == SLOTSACTION) { //!Slots

        Serial.print("Slots Action\n\r");

        if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Save") !=
            -1) {

          if (currentAction.from[0] >= 0 && currentAction.from[0] < NUM_SLOTS) {
            saveCurrentSlotToSlot(netSlot, currentAction.from[0]);
            netSlot = currentAction.from[0];
            }

          } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
            "Load") != -1) {
            if (currentAction.from[0] >= 0 && currentAction.from[0] < NUM_SLOTS) {
              // saveCurrentSlotToSlot(netSlot, currentAction.from[0]);

              netSlot = currentAction.from[0];
              slotChanged = 1;
              refreshConnections(-1);
              chooseShownReadings();
              printAllChangedNetColorFiles();
              }
            // netSlot = currentAction.from[0];
            return currentAction.from[0];
            } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
              "Clear") != -1) {
              createSlots(currentAction.from[0], 0);
              refreshConnections();

              //  sendAllPathsCore2 = 1;
              chooseShownReadings();
              return 10;
              }

        } else if (currentCategory == OUTPUTACTION) { //!Output

          Serial.print("Output Action\n\r");
          printActionStruct();
          if (menuLines[currentAction.previousMenuPositions[1]].indexOf("GPIO") !=
              -1) {

            printActionStruct();

            for (int i = 0; i < 10; i++) {
              if (currentAction.from[i] != -1) {
                switch (currentAction.from[i]) {
                  case 0:
                    addBridgeToNodeFile(RP_GPIO_1, currentAction.to[i], netSlot);
                    break;
                  case 1:

                    addBridgeToNodeFile(RP_GPIO_2, currentAction.to[i], netSlot);
                    break;

                  case 2:

                    addBridgeToNodeFile(RP_GPIO_3, currentAction.to[i], netSlot);
                    break;
                  case 3:

                    addBridgeToNodeFile(RP_GPIO_4, currentAction.to[i], netSlot);
                    break;

                  case 4:

                    addBridgeToNodeFile(RP_GPIO_5, currentAction.to[i], netSlot);
                    break;

                  case 5:

                    addBridgeToNodeFile(RP_GPIO_6, currentAction.to[i], netSlot);
                    break;

                  case 6:

                    addBridgeToNodeFile(RP_GPIO_7, currentAction.to[i], netSlot);
                    break;

                  case 7:

                    addBridgeToNodeFile(RP_GPIO_8, currentAction.to[i], netSlot);
                    break;

                  default:
                    break;
                  }

                // break;
                }



              // break;

              }


            } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
              "Voltage") != -1) {

              printActionStruct();

              for (int i = 0; i < 10; i++) {
                if (currentAction.from[i] != -1 && currentAction.to[i] != -1) {
                  switch (currentAction.from[i]) {
                    case 0:
                      addBridgeToNodeFile(DAC0, currentAction.to[i], netSlot);
                      // setDac0_5Vvoltage(currentAction.analogVoltage);
                      jumperlessConfig.dacs.dac_0 = currentAction.analogVoltage;
                      dacOutput[0] = currentAction.analogVoltage;
                      break;
                    case 1:

                      addBridgeToNodeFile(DAC1, currentAction.to[i], netSlot);
                      jumperlessConfig.dacs.dac_1 = currentAction.analogVoltage;
                      dacOutput[1] = currentAction.analogVoltage;
                      // setDac1_8Vvoltage(currentAction.analogVoltage);
                      break;
                      // break;

                      case 2:
                        addBridgeToNodeFile(TOP_RAIL, currentAction.to[i], netSlot);
                        jumperlessConfig.dacs.top_rail = currentAction.analogVoltage;
                        railVoltage[0] = currentAction.analogVoltage;
                        break;
                      case 3:
                        addBridgeToNodeFile(BOTTOM_RAIL, currentAction.to[i], netSlot);
                        jumperlessConfig.dacs.bottom_rail = currentAction.analogVoltage;
                        railVoltage[1] = currentAction.analogVoltage;
                        break;

                    default:
                      break;
                    }
                  }
                }
              refreshConnections();
              setRailsAndDACs();

              } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                "UART") != -1) {
                for (int i = 0; i < 10; i++) {
                  if (currentAction.from[i] != -1 && currentAction.to[i] != -1) {
                    switch (currentAction.from[i]) {
                      case 0:
                        addBridgeToNodeFile(RP_UART_TX, currentAction.to[i], netSlot);
                        break;
                      case 1:

                        addBridgeToNodeFile(RP_UART_RX, currentAction.to[i], netSlot);
                        break;
                        // break;
                      default:
                        break;
                      }

                    // break;
                    }
                  }
                } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Limits") != -1) {
                  if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Min Max") != -1) {
                    Serial.print("Min Max\n\r");
                    if (currentAction.from[0] == 0) {
                      jumperlessConfig.dacs.limit_min = 0.0;
                      } else if (currentAction.from[0] == 1) {
                        jumperlessConfig.dacs.limit_min = 0.0;
                        } else if (currentAction.from[0] == 2) {
                          jumperlessConfig.dacs.limit_min = -5.0;
                          } else if (currentAction.from[0] == 3) {
                            jumperlessConfig.dacs.limit_min = -8.0;
                            }

                          if (currentAction.from[0] == 0) {
                            jumperlessConfig.dacs.limit_max = 3.3;
                            } else if (currentAction.from[0] == 1) {
                              jumperlessConfig.dacs.limit_max = 5.0;
                              } else if (currentAction.from[0] == 2) {
                                jumperlessConfig.dacs.limit_max = 5.0;
                                } else if (currentAction.from[0] == 3) {
                                  jumperlessConfig.dacs.limit_max = 8.0;
                                  }

                                configChanged = true;
                    }
                  }

          } else if (currentCategory == APPSACTION) {

            Serial.print("Apps Action\n\r"); //!Apps Action

            if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Games") !=
                -1) {
              doomOn = 1;
              Serial.println("\n\n\n\rGames\n\r");
              }
            // digitalWrite(RESETPIN, HIGH);

            // delayMicroseconds(200);

            // digitalWrite(RESETPIN, LOW);

            // showSavedColors(netSlot);
            // sendPaths();
            // sendAllPathsCore2 = 1;
            // chooseShownReadings();

            // slotChanged = 0;
            inClickMenu = 0;


            for (int i = 0; i < NUM_APPS; i++) {
              if (menuLines[currentAction.previousMenuPositions[1]].indexOf(apps[i].name) != -1) {
                runApp(apps[i].index, apps[i].name);
                //showLEDsCore2 = -1;
                refreshConnections(-1, 0);
                break;
                }
              }


            return 10;

            } else if (currentCategory == ARDUINOACTION) {

              Serial.print("Arduino Action\n\r");

              } else if (currentCategory == PROBEACTION) {

                Serial.print("Probe Action\n\r");

                } else if (currentCategory == ROUTINGACTION) { //!Routing Options

                  if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Stack") !=
                    -1) {
                    if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Rails") !=
                      -1) {
                      jumperlessConfig.routing.stack_rails = currentAction.from[0];

                      if (currentAction.fromAscii[0][0] == 'M' || currentAction.fromAscii[0][0] == 'm') {

                        jumperlessConfig.routing.rail_priority = 2;
                        jumperlessConfig.routing.stack_rails = 7;

                        } else {
                        jumperlessConfig.routing.rail_priority = 1;
                        }

                      } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Paths") !=-1) {

                        jumperlessConfig.routing.stack_paths = currentAction.from[0];

                        if (currentAction.fromAscii[0][0] == 'M' || currentAction.fromAscii[0][0] == 'm') {
                          //pathPriority = 2;
                          //pathDuplicates = 5;
                          jumperlessConfig.routing.stack_paths = 5;
                          }

                        } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("DACs") != -1)
                          {
                          jumperlessConfig.routing.stack_dacs = currentAction.from[0];
                          
                          if (currentAction.fromAscii[0][0] == 'M' || currentAction.fromAscii[0][0] == 'm') {
                            //dacPriority = 2;
                            //dacDuplicates = 4;
                            jumperlessConfig.routing.stack_dacs = 4;
                            } else {
                            //dacPriority = 1;
                            jumperlessConfig.routing.stack_dacs = 1;
                            }
                          }

                    }
                  Serial.print("\n\rDuplicate Rails: ");
                  Serial.println(jumperlessConfig.routing.stack_rails);
                  
                  Serial.print("Rail Priority: ");
                  Serial.println(jumperlessConfig.routing.rail_priority);


                  Serial.print("Duplicate DACs: ");
                  Serial.print(jumperlessConfig.routing.stack_dacs);
                  // Serial.print("\n\rDAC Priority: ");
                  // Serial.print(jumperlessConfig.routing.dac_priority);
                  Serial.print("Duplicate Paths: ");
                  Serial.println(jumperlessConfig.routing.stack_paths);

                  // Serial.print("\n\n\r");

                  // Serial.print("Routing Action\n\r");
                  refreshConnections(-1, 1, 1);
                  configChanged = true;
                  //saveDuplicateSettings(0);

                  } else if (currentCategory == DISPLAYACTION) {//!Display Options





                    if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Jumpers") !=
                        -1) {
                      if (currentAction.from[0] == 0) {
                        jumperlessConfig.display.lines_wires = 1;
                        } else {
                        jumperlessConfig.display.lines_wires = 0;
                        }
                      debugFlagSet(12);


                      } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                        "Bright") != -1) {

                        if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Rails") != -1) {
                          //LEDbrightnessRail = currentAction.from[0] * 10 + 30;
                          } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Wires") != -1) {
                            //LEDbrightness = currentAction.from[0] * 5 + 5;
                            } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Special") != -1) {
                              // LEDbrightnessSpecial = currentAction.from[0] * 5 + 5;
                              } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Menu") != -1) {
                                menuBrightnessSetting = menuBrightnessOptionMap[currentAction.from[0]];
                                }


                              saveLEDbrightness(0);
                              showNets();
                              showLEDsCore2 = 2;
                        } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                          "DEFCON") != -1) {

                          if (currentAction.from[0] == 0) {
                            strcpy(defconString, "Jumper less V5");
                            defconDisplay = 0;
                            } else if (currentAction.from[0] == 1) {
                              defconDisplay = -1;
                              } else if (currentAction.from[0] == 2) {
                                strcpy(defconString, " Fuck    You   ");
                                // defconString[0] = "  Fuck   You";
                                defconDisplay = 0;
                                }

                          } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf(
                            "Colors") != -1) {
                            if (currentAction.from[0] == 0) {
                              netColorMode = 0;
                              } else {
                              netColorMode = 1;
                              }
                            debugFlagSet(13);
                            }
                          Serial.print("Display Action\n\r");

                    } else if (currentCategory == OLEDACTION) {   //!OLED Options

                      // LEDbrightness = (brightnessOptionMap[currentAction.from[0]]);
                      // LEDbrightnessSpecial = (specialBrightnessOptionMap[currentAction.from[0]]);
                      if (menuLines[currentAction.previousMenuPositions[1]].indexOf("ConnectOn Boot") != -1) {
                        if (currentAction.from[0] == 0) {
                          jumperlessConfig.top_oled.connect_on_boot = 1;
                          oled.init();
                          } else if (currentAction.from[0] == 1) {
                            jumperlessConfig.top_oled.connect_on_boot = 0;
                            oled.disconnect();
                            }
                          //oled.init();
                          oled.clear();
                          oled.setTextSize(1);
                          oled.clearPrintShow("Connect \nOn Boot: ", 1, true, false, false, 0, 0);
                          oled.setTextSize(2);
                          // oled.setCursor(0, 0);
                          if (jumperlessConfig.top_oled.connect_on_boot == 1) {
                            oled.print("On");
                            } else {
                            oled.print("Off");
                            }
                          oled.show();
                          oled.setTextSize(1);
                          configChanged = true;

                        } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Lock   Connect") != -1) {
                          if (currentAction.from[0] == 0) {
                            jumperlessConfig.top_oled.lock_connection = 1;
                            } else if (currentAction.from[0] == 1) {
                              jumperlessConfig.top_oled.lock_connection = 0;
                              }
                            oled.clear();
                            oled.setTextSize(1);
                            oled.print("Lock \nConnection: ");
                            oled.setTextSize(2);
                            if (jumperlessConfig.top_oled.lock_connection == 1) {
                              oled.print("On");
                              } else {
                              oled.print("Off");
                              }
                            oled.show();
                            oled.setTextSize(1);
                            configChanged = true;
                          } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Connect") != -1) {
                          if (oled.checkConnection() == 0) {
                              jumperlessConfig.top_oled.enabled = 1;
                              showLEDsCore2 = 1;
                              oled.init();
                              oled.clear();
                              oled.setTextSize(1);
                              oled.print("OLED Connected");
                              oled.show();
                              delay(300);
                              oled.clear();
                              oled.showJogo32h();
                              oled.show();
                              } else {
                                oled.clear();
                                oled.setTextSize(1);
                                oled.print("Disconnecting OLED");
                                oled.show();
                                delay(300);
                                oled.clear();
                                oled.show();
                                oled.disconnect();
                                jumperlessConfig.top_oled.enabled = 0;

                                }
                            } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Font") != -1) {
                              // Map the menu selection to FontFamily value for config
                              FontFamily selectedFamily;
                              int configFontValue;

                              if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Jokermn") != -1) {
                                selectedFamily = FONT_JOKERMAN;
                                configFontValue = 1;
                                } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Eurostl") != -1) {
                                  selectedFamily = FONT_EUROSTILE;
                                  configFontValue = 0;
                                  } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("ComicSns") != -1) {
                                    selectedFamily = FONT_COMIC_SANS;
                                    configFontValue = 2;
                                    } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Courier") != -1) {
                                      selectedFamily = FONT_COURIER_NEW;
                                      configFontValue = 3;
                                      } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("Science") != -1) {
                                        selectedFamily = FONT_NEW_SCIENCE_MEDIUM;
                                        configFontValue = 4;
                                        } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("SciExt") != -1) {
                                          selectedFamily = FONT_NEW_SCIENCE_MEDIUM_EXTENDED;
                                          configFontValue = 5;
                                          } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("AndlMno") != -1) {
                                            selectedFamily = FONT_ANDALE_MONO;
                                            configFontValue = 6;
                                            } else if (menuLines[currentAction.previousMenuPositions[2]].indexOf("FreMno") != -1) {
                                              selectedFamily = FONT_FREE_MONO;
                                              configFontValue = 7;
                                              }

                                        // Set the font family (smart selection will choose appropriate size)
                                        //oled.setFontForSize(selectedFamily, 1);
                                        oled.setFont((FontFamily)selectedFamily);

                                        // Update config with FontFamily value
                                        jumperlessConfig.top_oled.font = configFontValue;
                                        configChanged = true;

                                        // oled.clearPrintShow("Font Set:", 1, selectedFamily, true, true, true, 5, 8);
                                        // delay(1000);
                                        // oled.clearPrintShow("Preview Text", 2, selectedFamily, true, true, true, 5, 8);

                                        Serial.print("Font set to: ");
                        
                                        Serial.println(oled.getFontName(selectedFamily));

                              } else if (menuLines[currentAction.previousMenuPositions[1]].indexOf("Show in Term") != -1) {
                                if (jumperlessConfig.top_oled.show_in_terminal == 1) {
                                  jumperlessConfig.top_oled.show_in_terminal = 0;
                                  } else {
                                    jumperlessConfig.top_oled.show_in_terminal = 1;
                                    }
                                configChanged = true;

                                }





                      } else if (currentCategory == NOCATEGORY) {

                        Serial.print("No Category\n\r");
                        }

                      return 1;
  }

String categoryNames[] = { "Show",  "Rails", "Slots",   "Output",    "Arduino",
                          "Probe", "Apps",  "Routing", "NoCategory" };

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

// char printMainMenu(int extraOptions) {

//   Serial.print("\n\n\r\t\tMenu\n\r");
//   // Serial.print("Slot ");
//   // Serial.print(netSlot);
//   Serial.print("\n\r");
//   Serial.print("\tm = show this menu\n\r");
//   Serial.print("\tn = show netlist\n\r");
//   Serial.print("\ts = show node files by slot\n\r");
//   Serial.print("\to = load node files by slot\n\r");
//   Serial.print("\tf = load node file to current slot\n\r");
//   // Serial.print("\tr = rotary encoder mode -");
//   //   rotaryEncoderMode == 1 ? Serial.print(" ON (z/x to cycle)\n\r")
//   //                          : Serial.print(" off\n\r");
//   // Serial.print("\t\b\bz/x = cycle slots - current slot ");
//   // Serial.print(netSlot);
//   Serial.print("\n\r");
//   Serial.print("\te = show extra menu options\n\r");

//   if (extraOptions == 1) {
//     Serial.print("\tb = show bridge array\n\r");
//     Serial.print("\tp = probe connections\n\r");
//     Serial.print("\tw = waveGen\n\r");
//     Serial.print("\tv = toggle show current/voltage\n\r");
//     // Serial.print("\tu = set baud rate for USB-Serial\n\r");
//     Serial.print("\tl = LED brightness / test\n\r");
//     Serial.print("\td = toggle debug flags\n\r");
//     }
//   // Serial.print("\tc = clear nodes with probe\n\r");
//   Serial.print("\n\n\r");
//   return ' ';
//   }



char LEDbrightnessMenu(void) {

  char input = ' ';
  Serial.print("\n\r\t\tLED Brightness Menu \t\n\n\r");
  Serial.print("\n\r\tl = LED brightness        =   ");
  Serial.print(LEDbrightness);
  Serial.print("\n\r\tr = rail brightness       =   ");
  Serial.print(LEDbrightnessRail);
  Serial.print("\n\r\ts = special brightness    =   ");
  Serial.print(LEDbrightnessSpecial);
  Serial.print("\n\r\tc = click menu brightness =   ");
  Serial.print(menuBrightnessSetting);
  Serial.print("\n\r\tt = all types\t");
  Serial.println();
  Serial.print("\n\r\td = reset to defaults");
  Serial.println();
  Serial.print("\n\r\tn = color name test");
  Serial.print("\n\r\tb = bounce logo");
  Serial.print("\n\r\tc = random colors");
  Serial.print("\n\r\t? = is this?");
  Serial.println();
  Serial.print("\n\r\tm = return to main menu\n\n\r");
  // Serial.print(leds.getBrightness());
  if (LEDbrightness > 50 || LEDbrightnessRail > 50 ||
      LEDbrightnessSpecial > 70) {
    // Serial.print("\tBrightness settings above ~50 will cause significant
    // heating, it's not recommended\n\r");
   /// delay(10);
    }

  while (Serial.available() == 0) {
    //delayMicroseconds(10);
    }

  input = Serial.read();

  if (input == 'm') {
    saveLEDbrightness(0);

    return ' ';
    } else if (input == 'n') {
      int hue = colorPicker();
      Serial.print("Hue: ");
      Serial.println(hue);
      Serial.print("Color: ");
      hsvColor hsv = { (uint8_t)hue, 255, LEDbrightness };
      Serial.printf("0x%06X\n\r", HsvToRaw(hsv));
      return ' ';
      } else if (input == 'd') {
        saveLEDbrightness(1);

        return ' ';
        } else if (input == 'l') {
          Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tm = exit\n\n\r");
          Serial.flush();
          while (input == 'l') {

            while (Serial.available() == 0)
              ;
            char input2 = Serial.read();
            if (input2 == '+') {
              LEDbrightness += 1;

              if (LEDbrightness > 200) {

                LEDbrightness = 200;
                }
              Serial.print("\r                            \r");
              Serial.print("LED brightness:  ");
              Serial.print(LEDbrightness);
              Serial.print("   ");
              //Serial.print("\n\r");
              Serial.flush();

              showLEDsCore2 = 2;
              } else if (input2 == '-') {
                LEDbrightness -= 1;

                if (LEDbrightness < 2) {
                  LEDbrightness = 1;
                  }
                Serial.print("\r                            \r");
                Serial.print("LED brightness:  ");
                Serial.print(LEDbrightness);
                Serial.print("   ");
                //Serial.print("\n\r");
                Serial.flush();

                showLEDsCore2 = 2;
                } else if (input2 == 'x' || input2 == ' ' || input2 == 'm') {
                  input = ' ';
                  } else {
                  }
                //showNets();

                // for (int i = 8; i <= numberOfNets; i++) {
                //   lightUpNet(i, -1, 1, LEDbrightness, 0);
                // }
                showLEDsCore2 = 1;

                if (Serial.available() == 0) {
                  Serial.print("\r                            \r");
                  Serial.print("LED brightness:  ");
                  Serial.print(LEDbrightness);
                  Serial.print("   ");
                  //Serial.print("\n\r");
                  Serial.flush();
                  if (LEDbrightness > 50) {
                    // Serial.print("Brightness settings above ~50 will cause
                    // significant heating, it's not recommended\n\r");
                    }
                  }
            }
          } else if (input == 'r') {
            Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tm = exit\n\n\r");
            while (input == 'r') {

              while (Serial.available() == 0)
                ;
              char input2 = Serial.read();
              if (input2 == '+' || input2 == '=') {

                LEDbrightnessRail += 1;

                if (LEDbrightnessRail > 200) {

                  LEDbrightnessRail = 200;
                  }
                Serial.print("\r                            \r");
                Serial.print("Rail brightness:  ");
                Serial.print(LEDbrightnessRail);
                Serial.print("   ");
                //Serial.print("\n\r");
                Serial.flush();

                showLEDsCore2 = 2;
                } else if (input2 == '-' || input2 == '_') {

                  LEDbrightnessRail -= 1;

                  if (LEDbrightnessRail < 2) {
                    LEDbrightnessRail = 1;
                    }
                  Serial.print("\r                            \r");
                  Serial.print("Rail brightness:  ");
                  Serial.print(LEDbrightnessRail);
                  Serial.print("   ");
                  //Serial.print("\n\r");
                  Serial.flush();

                  showLEDsCore2 = 2;
                  } else if (input2 == 'x' || input2 == ' ' || input2 == 'm') {
                    input = ' ';
                    saveLEDbrightness(0);
                    return ' ';
                    } else {
                    }
                  lightUpRail(-1, -1, 1, LEDbrightnessRail);

                  if (Serial.available() == 0) {
                    Serial.print("\r                            \r");
                    Serial.print("Rail brightness:  ");
                    Serial.print(LEDbrightnessRail);
                    //Serial.print("\n\r");
                    Serial.flush();
                    if (LEDbrightnessRail > 50) {
                      // Serial.println("Brightness settings above ~50 will cause
                      // significant heating, it's not recommended\n\n\r");
                      }
                    }
              }

            // Serial.print(input);
              //Serial.print("\n\r");
            } else if (input == 'h') {
              Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tm = exit\n\n\r");
              b.clear();
              b.print("B", menuColors[0], 0xffffff, 0, 0, 1);
              b.print("r", menuColors[1], 0xffffff, 1, 0, 1);
              b.print("i", menuColors[2], 0xffffff, 2, 0, 1);
              b.print("g", menuColors[3], 0xffffff, 3, 0, 1);
              b.print("h", menuColors[4], 0xffffff, 4, 0, 1);
              b.print("t", menuColors[5], 0xffffff, 5, 0, 1);

              b.print("n", menuColors[6], 0xffffff, 1, 1, 2);
              b.print("e", menuColors[4], 0xffffff, 2, 1, 2);
              b.print("s", menuColors[2], 0xffffff, 3, 1, 2);
              b.print("s", menuColors[0], 0xffffff, 4, 1, 2);

              showLEDsCore2 = 2;
              while (input == 'h') {

                while (Serial.available() == 0)
                  ;
                char input2 = Serial.read();
                if (input2 == '+') {
                  menuBrightnessSetting += 5;
                  if (menuBrightnessSetting > 150) {
                    menuBrightnessSetting = 150;
                    }

                  b.clear();

                  b.print("B", menuColors[0], 0xffffff, 0, 0, 1);
                  b.print("r", menuColors[1], 0xffffff, 1, 0, 1);
                  b.print("i", menuColors[2], 0xffffff, 2, 0, 1);
                  b.print("g", menuColors[3], 0xffffff, 3, 0, 1);
                  b.print("h", menuColors[4], 0xffffff, 4, 0, 1);
                  b.print("t", menuColors[5], 0xffffff, 5, 0, 1);

                  b.print("n", menuColors[6], 0xffffff, 1, 1, 2);
                  b.print("e", menuColors[4], 0xffffff, 2, 1, 2);
                  b.print("s", menuColors[2], 0xffffff, 3, 1, 2);
                  b.print("s", menuColors[0], 0xffffff, 4, 1, 2);

                  showLEDsCore2 = 2;
                  } else if (input2 == '-') {

                    menuBrightnessSetting -= 5;
                    if (menuBrightnessSetting < -100) {
                      menuBrightnessSetting = -100;
                      }
                    b.clear();
                    b.print("B", menuColors[0], 0xffffff, 0, 0, 1);
                    b.print("r", menuColors[1], 0xffffff, 1, 0, 1);
                    b.print("i", menuColors[2], 0xffffff, 2, 0, 1);
                    b.print("g", menuColors[3], 0xffffff, 3, 0, 1);
                    b.print("h", menuColors[4], 0xffffff, 4, 0, 1);
                    b.print("t", menuColors[5], 0xffffff, 5, 0, 1);

                    b.print("n", menuColors[6], 0xffffff, 1, 1, 2);
                    b.print("e", menuColors[4], 0xffffff, 2, 1, 2);
                    b.print("s", menuColors[2], 0xffffff, 3, 1, 2);
                    b.print("s", menuColors[0], 0xffffff, 4, 1, 2);

                    showLEDsCore2 = 2;
                    } else if (input2 == 'x') {
                      input = ' ';
                      } else {
                      }
                    lightUpRail(-1, -1, 1, LEDbrightnessRail);

                    if (Serial.available() == 0) {

                      Serial.print("Click menu brightness:  ");
                      Serial.print(menuBrightnessSetting);
                      Serial.print("\n\r");
                      }
                }

              // Serial.print(input);
              Serial.print("\n\r");
              } else if (input == 's') {
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
                      } else if (input2 == 'x' || input2 == ' ' || input2 == 'm') {
                        input = ' ';
                        saveLEDbrightness(0);
                        return ' ';
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

                  Serial.print("\n\r\t+ = increase\n\r\t- = decrease\n\r\tm = exit\n\n\r");
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
                        saveLEDbrightness(0);
                        return ' ';
                        } else {
                          }

                        for (int i = 6; i <= numberOfNets; i++) {
                          lightUpNet(i, -1, 1, LEDbrightness, 0);
                          }

                        lightUpRail(-1, -1, 1, LEDbrightnessRail);
                        for (int i = 0; i < 6; i++) {
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
                          Serial.print("    ");
                          Serial.flush();
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
                    pauseCore2 = 1;
                    while (Serial.available() == 0) {
                      //startupColorsV5();
                      drawAnimatedImage(0);
                      delay(80);
                      drawAnimatedImage(1);
                      //  delay(100);

                     // clearLEDsExceptRails();
                      //showLEDsCore2 = 1;

                     // delay(2000);
                      // rainbowBounce(3);
                      }
                    pauseCore2 = 0;
                    //showNets();
                    //lightUpRail(-1, -1, 1);
                    showLEDsCore2 = -1;

                    input = '!'; // this tells the main fuction to reset the leds
                    } else if (input == 'c') {
                      Serial.print("\n\rPress any key to exit\n\n\r");
                      pauseCore2 = 1;
                      while (Serial.available() == 0) {

                        randomColors();
                        leds.show();
                        delayMicroseconds(random(500, 80000));
                        showLEDsCore2 = -3;
                        }
                      pauseCore2 = 0;
                      showLEDsCore2 = -1;
                      //delay(100);
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
                          currentPixel.r > 15 ? : Serial.print(padZero);
                          Serial.print(currentPixel.r, 16);
                          currentPixel.g > 15 ? : Serial.print(padZero);
                          Serial.print(currentPixel.g, 16);
                          currentPixel.b > 15 ? : Serial.print(padZero);
                          Serial.print(currentPixel.b, 16);
                          Serial.print(", ");

                          if (i % 8 == 0 && i > 0) {
                            Serial.println();
                            }

                          // Serial.println(colorString);
                          }

                        return ' ';
                        } else if (input == '?') {
                          showLoss();
                          while (Serial.available() == 0) {
                            }
                          showLEDsCore2 = -1;
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
  showLEDsCore2 = -3;
  uint32_t guyColor = 0x0a0a1a;
  uint32_t hairColor = 0x1a0902;
  uint32_t nurseColor = 0x1a0207;
  uint32_t doctorColor = 0x1b0a1e;
  uint32_t patientColor = 0x070a0f;

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



// */

// void drawMainMenu(void) {
//     clearDisplay();
//     drawString("Jumperless", 0, 0, 2, WHITE);
//     drawLine(0, 20, 128, 20, WHITE);

//     const char* options[] = {
//         "Nets",
//         "Probing",
//         "Settings",
//         "Apps",
//         "Config"
//     };
//     const int numOptions = 5;
//     menuPositionMax = numOptions - 1;
//     menuPositionMin = 0;

//     // Draw menu options
//     for (int i = 0; i < numOptions; i++) {
//         int y = 24 + (i * 16);
//         if (menuPosition == i) {
//             drawString(">", 0, y, 2, WHITE);
//             drawString(options[i], 20, y, 2, WHITE);
//         } else {
//             drawString(options[i], 20, y, 2, DARKGREY);
//         }
//     }

//     // Handle menu selection
//     if (menuConfirm) {
//         menuConfirm = 0;
//         switch (menuPosition) {
//             case 0:
//                 menuState = MENU_NETS;
//                 break;
//             case 1:
//                 menuState = MENU_PROBING;
//                 break;
//             case 2:
//                 menuState = MENU_SETTINGS;
//                 break;
//             case 3:
//                 menuState = MENU_APPS;
//                 break;
//             case 4:
//                 menuState = MENU_CONFIG;
//                 break;
//         }
//         menuPosition = 0;
//     }
// }

// void drawConfigMenu(void) {
//     clearDisplay();
//     drawString("Configuration", 0, 0, 1, WHITE);
//     drawLine(0, 10, 128, 10, WHITE);

//     // Menu options
//     const char* options[] = {
//         "Print Config",
//         "Load Config",
//         "Reset to Default",
//         "Back"
//     };
//     const int numOptions = 4;
//     menuPositionMax = numOptions - 1;
//     menuPositionMin = 0;

//     // Draw menu options
//     for (int i = 0; i < numOptions; i++) {
//         int y = 16 + (i * 12);
//         if (menuPosition == i) {
//             drawString(">", 0, y, 1, WHITE);
//             drawString(options[i], 10, y, 1, WHITE);
//         } else {
//             drawString(options[i], 10, y, 1, DARKGREY);
//         }
//     }

//     // Handle menu selection
//     if (menuConfirm) {
//         menuConfirm = 0;
//         switch (menuPosition) {
//             case 0: // Print Config
//                 printConfigToSerial();
//                 break;
//             case 1: // Load Config
//                 readConfigFromSerial();
//                 break;
//             case 2: // Reset to Default
//                 resetConfigToDefaults();
//                 saveConfig();
//                 Serial.println("Configuration reset to defaults");
//                 break;
//             case 3: // Back
//                 menuState = MENU_MAIN;
//                 menuPosition = 0;
//                 break;
//         }
//     }
// }

// void drawMenu(void) {
//     switch (menuState) {
//         case MENU_MAIN:
//             drawMainMenu();
//             break;
//         case MENU_NETS:
//             drawNetsMenu();
//             break;
//         case MENU_PROBING:
//             drawProbingMenu();
//             break;
//         case MENU_SETTINGS:
//             drawSettingsMenu();
//             break;
//         case MENU_APPS:
//             drawAppsMenu();
//             break;
//         case MENU_CONFIG:
//             drawConfigMenu();
//             break;
//     }
// }