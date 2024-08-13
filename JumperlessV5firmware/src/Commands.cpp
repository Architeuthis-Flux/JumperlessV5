#include "Commands.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "Probing.h"
#include "RotaryEncoder.h"

volatile int sendAllPathsCore2 =
    0; // this signals the core 2 to send all the paths to the CH446Q
volatile int showLEDsCore2 = 0; // this signals the core 2 to show the LEDs
volatile int showProbeLEDs =
    0; // this signals the core 2 to show the probe LEDs

void refreshConnections(int ledShowOption) {

  clearAllNTCC();
  openNodeFile(netSlot, 0);
  getNodesToConnect();
  // digitalWrite(RESETPIN, HIGH);
  bridgesToPaths();

  // delay(1);
  assignNetColors();
  clearLEDsExceptRails();
  // delay(1);
  //  digitalWrite(RESETPIN, LOW);
  // delay(5);
  sendAllPathsCore2 = 1;
   delay(5);
  showLEDsCore2 = 1;
  // while (showLEDsCore2 != 0)
  // {

  // }
  // delay(1);

  // delay(1);
}

void refreshLocalConnections(int ledShowOption) {
  clearAllNTCC();
  openNodeFile(netSlot, 1);
  getNodesToConnect();
  // digitalWrite(RESETPIN, HIGH);
  bridgesToPaths();

  assignNetColors();
  clearLEDsExceptRails();
  // delay(1);

  // digitalWrite(RESETPIN, LOW);
  // delay(5);
  sendAllPathsCore2 = 1;
   delay(5);
  showLEDsCore2 = ledShowOption;
}

void printSlots(int fileNo) {
  if (fileNo == -1)

    if (Serial.available() > 0) {
      fileNo = Serial.read();
      // break;
    }

  Serial.print("\n\n\r");
  if (fileNo == -1) {
    Serial.print("\tSlot Files");
  } else {
    Serial.print("\tSlot File ");
    Serial.print(fileNo - '0');
  }
  Serial.print("\n\n\r");
  Serial.print(
      "\n\ryou can paste this text reload this circuit (enter 'o' first)");
  Serial.print("\n\r(or even just a single slot)\n\n\n\r");
  if (fileNo == -1) {
    for (int i = 0; i < NUM_SLOTS; i++) {
      Serial.print("\n\rSlot ");
      Serial.print(i);
      if (i == netSlot) {
        Serial.print("        <--- current slot");
      }

      Serial.print("\n\rnodeFileSlot");
      Serial.print(i);
      Serial.print(".txt\n\r");
      if (getSlotLength(i, 0) > 0) {
        Serial.print("\n\rf ");
        printNodeFile(i);
        Serial.print("\n\n\r");
      }
    }
  } else {

    Serial.print("\n\rnodeFileSlot");
    Serial.print(fileNo - '0');
    Serial.print(".txt\n\r");

    Serial.print("\n\rf ");

    printNodeFile(fileNo - '0');
    Serial.print("\n\r");
  }
}