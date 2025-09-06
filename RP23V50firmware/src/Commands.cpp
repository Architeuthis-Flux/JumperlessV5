#include "Commands.h"
#include "CH446Q.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "MatrixState.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "RotaryEncoder.h"

#include "USBfs.h"

volatile int sendAllPathsCore2 =
    0; // this signals the core 2 to send all the paths to the CH446Q

    ///negative values clear, 1 = show the netlist as in node file, 2 = keep added graphics, 3 = don't clear this
volatile int showLEDsCore2 = 0; // this signals the core 2 to show the LEDs
volatile int showProbeLEDs =
    0; // this signals the core 2 to show the probe LEDs

volatile int core1request = 0; // this signals core 1 wants to do something

unsigned long waitCore2() {

  // delayMicroseconds(60);
  unsigned long timeout = micros();
  core1request = 1;
  while (core2busy || (sendAllPathsCore2 != 0)) {
    // Serial.println("waiting for core2 to finish");
    if (micros() - timeout > 25000) {  // Reduced timeout from 50000 to 25000
      //Serial.println("wait core2 timeout");
      core2busy = false;
      sendAllPathsCore2 = 0;
      break;
    }
    }
    // Serial.println(micros() - timeout);

  core1request = 0;
  return micros() - timeout;
}

int lastSlot = netSlot;

void refresh(int flashOrLocal, int ledShowOption, int fillUnused, int clean) {

  if (flashOrLocal == 1) {
    //if (ledShowOption == 0){
      //refreshBlind(1, fillUnused, clean);
    //} else {
      refreshLocalConnections(ledShowOption, fillUnused, clean);
    //}
  
  } else {
    refreshConnections(ledShowOption, fillUnused, clean);
  }
}

//#define DEBUG_REFRESH 1

void refreshConnections(int ledShowOption, int fillUnused, int clean) {

 // waitCore2();

  unsigned long start = millis();
  //core1busy = true;
  clearAllNTCC();
  //core1busy = true;
  // return;
  openNodeFile(netSlot, 0);
#ifdef DEBUG_REFRESH
      Serial.print("refreshConnections openNodeFile = ");
      Serial.println(millis() - start);
#endif

  getNodesToConnect();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections getNodesToConnect = ");
  Serial.println(millis() - start);
#endif
//core1busy = false;
  bridgesToPaths();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections bridgesToPaths = ");
  Serial.println(millis() - start);
#endif
  checkChangedNetColors(-1);
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections checkChangedNetColors = ");
  Serial.println(millis() - start);
#endif
  assignNetColors();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections assignNetColors = ");
  Serial.println(millis() - start);
#endif
  chooseShownReadings();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections chooseShownReadings = ");
  Serial.println(millis() - start);
#endif
  assignTermColor();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections assignTermColor = ");
  Serial.println(millis() - start);
#endif
  //findChangedNetColors();
  //assignNetColors();
  
  // Restore GPIO configurations from jumperlessConfig after net processing
  //setGPIO();
#ifdef DEBUG_REFRESH
  Serial.print("refreshConnections setGPIO = ");
  Serial.println(millis() - start);
#endif

  // if (lastSlot != netSlot) {
  //   createLocalNodeFile(netSlot);
  //   lastSlot = netSlot;
  // }

  // Serial.print("refreshConnections: ");
  // Serial.println(ledShowOption);
  // Serial.print("fillUnused: ");
  // Serial.println(fillUnused);
  // Serial.print("clean: ");
  // Serial.println(clean);
  // Serial.print("jumperlessConfig.routing.stack_paths: ");
  // Serial.println(jumperlessConfig.routing.stack_paths);
  // Serial.print("jumperlessConfig.routing.stack_rails: ");
  // Serial.println(jumperlessConfig.routing.stack_rails);
  // Serial.print("jumperlessConfig.routing.stack_dacs: ");
  // Serial.println(jumperlessConfig.routing.stack_dacs);


  if (ledShowOption != 0) {

    showLEDsCore2 = ledShowOption;
    waitCore2();
  }
  if (clean == 1) {
    sendAllPathsCore2 = -1;
    if (rp2040.cpuid() == 1) {
      sendPaths(sendAllPathsCore2);
      //sendAllPathsCore2 = 0;
    } 
    } else {
      sendAllPathsCore2 = 1;
      if (rp2040.cpuid() == 1) {
        sendPaths(sendAllPathsCore2);
        //sendAllPathsCore2 = 0;
      }
    }
#ifdef DEBUG_REFRESH
  Serial.print("after waitCore2 time = ");  

  Serial.println(millis() - start);
#endif
  
  
  // sendPaths();
}

//#define DEBUG_REFRESH 1

void refreshLocalConnections(int ledShowOption, int fillUnused, int clean) {

  if (rp2040.cpuid() == 0) {
   waitCore2();
  }
   
unsigned long start2 = millis();
  clearAllNTCC();
  //core1busy = true;
  openNodeFile(netSlot, 1);


  getNodesToConnect();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections getNodesToConnect = ");
  Serial.println(millis() - start2);
#endif
  bridgesToPaths();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections bridgesToPaths = ");
  Serial.println(millis() - start2);
#endif
  checkChangedNetColors(-1);
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections checkChangedNetColors = ");
  Serial.println(millis() - start2);
#endif
  assignNetColors();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections assignNetColors = ");
  Serial.println(millis() - start2);
#endif
  chooseShownReadings();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections chooseShownReadings = ");
  Serial.println(millis() - start2);
#endif
  assignTermColor();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections assignTermColor = ");
  Serial.println(millis() - start2);
#endif
  // Restore GPIO configurations from jumperlessConfig after net processing
  setGPIO();
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections time = ");
  Serial.println(millis() - start2);
#endif
  //core1busy = false;
  if (ledShowOption != 0) {

    showLEDsCore2 = ledShowOption;
    //waitCore2();
  }
  if (clean == 1) {
    sendAllPathsCore2 = -1;
    if (rp2040.cpuid() == 1) {
      sendPaths(sendAllPathsCore2);
      //sendAllPathsCore2 = 0;
    } 
    } else {
      sendAllPathsCore2 = 1;
      if (rp2040.cpuid() == 1) {
        sendPaths(sendAllPathsCore2);
        //sendAllPathsCore2 = 0;
      }
    }
#ifdef DEBUG_REFRESH
  Serial.print("refreshLocalConnections after waitCore2 time = ");
  Serial.println(millis() - start2);
#endif

  // Serial.print("Free heap = ");
  // Serial.println(rp2040.getFreeHeap());


  // sendPaths();
  
  //waitCore2();
}

void refreshBlind(
    int disconnectFirst,
    int fillUnused,
    int clean) { // this doesn't actually touch the flash so we don't
  // need to wait for core 2
  waitCore2();
  //core1busy = true;
  // fillUnused = 0;
  clearAllNTCC();
  openNodeFile(netSlot, 1);
  //core1busy = true;
  getNodesToConnect();
  bridgesToPaths();
  checkChangedNetColors(-1);
  assignNetColors();
  assignTermColor();
  // printPathsCompact();
  //core1busy = false;
  //   if (lastSlot != netSlot) {
  //   createLocalNodeFile(netSlot);
  //   lastSlot = netSlot;
  // }
  // if (disconnectFirst == 1) {
  //   sendAllPathsCore2 = 1;
  // } else if (disconnectFirst == 0) {
  //   sendAllPathsCore2 = 1;
  // } else {
  //   sendAllPathsCore2 = 1; // disconnectFirst;
  // }
  if (clean == 1) {
    sendAllPathsCore2 = -1;
    if (rp2040.cpuid() == 1) {
      sendPaths(sendAllPathsCore2);
     // sendAllPathsCore2 = 0;
    } 
    } else {
      sendAllPathsCore2 = 1;
      if (rp2040.cpuid() == 1) {
        sendPaths(sendAllPathsCore2);
        //sendAllPathsCore2 = 0;
      }
    }


  chooseShownReadings();
  
  // Restore GPIO configurations from jumperlessConfig after net processing
  setGPIO();
  
  // sendPaths();
  //core1busy = false;
  waitCore2();
}

struct rowLEDs getRowLEDdata(int row) {

  struct rowLEDs rowLEDs = {0, 0, 0, 0, 0};
  // uint8_t *pixelPointer = leds.getPixels();
  if (row < 1) {
    return rowLEDs;
  } else if (row >= 70 && row < 125) {
    // row = row - 1;
    for (int i = 0; i < 35; i++) { // stored in GRB order
      if (bbPixelToNodesMapV5[i][0] == row) {
        rowLEDs.color[0] = leds.getPixelColor(bbPixelToNodesMapV5[i][1]);
        return rowLEDs;
      }
    }

    // Serial.print(row);
    // Serial.print(" ");
    // Serial.println(rowLEDs.color[0]);

    return rowLEDs;
  }
  row = row - 1;
  for (int i = 0; i < 5; i++) { // stored in GRB order
  rowLEDs.color[i] = 0x000000;
    rowLEDs.color[i] = leds.getPixelColor(row * 5 + i);
    // rowLEDs.color[i] = packRgb(pixelPointer[15 * row + (3 * i) + 1],
    //                            pixelPointer[15 * row + (3 * i)],
    //                            pixelPointer[15 * row + (3 * i) + 2]);
    // Serial.print(row * 5 + i);
    // Serial.print(" ");
    // Serial.println(leds.getPixelColor(row * 5 + i));
  }

  return rowLEDs;
}

void setRowLEDdata(int row, struct rowLEDs rowLEDcolors) {

  // uint8_t *pixelPointer = leds.getPixels();
  if (row < 1 || row > 125) {
    return;
  } else if (row >= 70 && row < 125) {
    // row = row - 1;
    rgbColor colorrgb = unpackRgb(rowLEDcolors.color[0]);
    for (int i = 0; i < 35; i++) { // stored in GRB order
      if (bbPixelToNodesMapV5[i][0] == row) {
        leds.setPixelColor(bbPixelToNodesMapV5[i][1], colorrgb.r, colorrgb.g,
                           colorrgb.b);
        return;
      }
    }
    return;
  }
  row = row - 1;
  for (int i = 0; i < 5; i++) { // stored in GRB order

    leds.setPixelColor(row * 5 + i, rowLEDcolors.color[i]);
    // rgbColor colorrgb = unpackRgb(rowLEDcolors.color[i]);
    // pixelPointer[15 * row + (3 * i) + 1] = colorrgb.r;
    // pixelPointer[15 * row + (3 * i)] = colorrgb.g;
    // pixelPointer[15 * row + (3 * i) + 2] = colorrgb.b;
  }
  return;
}

void connectNodes(int node1, int node2) {

  if (node1 == node2 || node1 < 1 || node2 < 1) {
    return;
  }
  if ((node1 > 60 && node1 < 70) || (node2 > 60 && node2 < 70)) {
    return;
  }

  addBridgeToNodeFile(node1, node2, netSlot, 0);

  refreshConnections();
  waitCore2();
  // createLocalNodeFile(netSlot);
}

void disconnectNodes(int node1, int node2) {
  removeBridgeFromNodeFile(node1, node2, netSlot, 0);
  refreshConnections();
  waitCore2();
}

float measureVoltage(int adcNumber, int node, bool checkForFloating) {
  int adcDefine = 0;

  switch (adcNumber) {
  case 0:
    adcDefine = ADC0;
    break;
  case 1:
    adcDefine = ADC1;
    break;
  case 2:
    adcDefine = ADC2;
    break;
  case 3:
    adcDefine = ADC3;
    break;
  case 4:
    adcDefine = ADC4;
    break;
  case 5:
    // adcDefine = ADC5;
    break;
  case 6:
    // adcDefine = ADC6;
    break;
  case 7:
    adcDefine = ADC7;
    break;
  default:
    return 0.0;
  }

  // removeBridgeFromNodeFile(adcDefine, -1, netSlot, 0);

  // delay(2);
  //waitCore2();
  removeBridgeFromNodeFile(adcDefine, -1, netSlot, 1);
  addBridgeToNodeFile(node, adcDefine, netSlot, 1);
  refreshLocalConnections(1 , 0, 0);
  waitCore2();
  //refreshBlind(-1);
  //         printPathsCompact();
  // printChipStatus();

  // Serial.println(readAdc(adcNumber, 32) * (5.0 / 4095));
  // core1busy = true;
  float voltage = readAdcVoltage(adcNumber, 8);

  // Serial.print("voltage = ");
  // Serial.println(voltage);

  int floating = 0;
  if (checkForFloating == true) {
    if (voltage < 0.3 && voltage > -0.3) {

      if (checkFloating(node) == true) {
        floating = 1;
      }
    }
    waitCore2();
  }
  removeBridgeFromNodeFile(node, adcDefine, netSlot, 1);
  refreshLocalConnections(0, 0, 0);
  //refreshBlind();
  waitCore2();

  if (floating == 1) {

    return 0xFFFFFFFF;
  }

  return voltage;
}

bool checkFloating(int node) {
  // delay(2);
  // Serial.print("node = ");
  // Serial.println(node);
  int gpioNumber = RP_GPIO_1;
  int gpioPin = GPIO_1_PIN;

  switch (node) {

  case 1 ... 93: {
    for (int i = 0; i < 4; i++) {
      if (ch[11].xStatus[i + 4] == -1) {
        gpioNumber = RP_GPIO_1 + i;
        break;
      }
    }
    break;
  }

  // case 70 ... 120: {
  //   for (int i = 0; i < 12; i++) {
  //     if (ch[8].xMap[i] == node) {
  //       gpioNumber = RP_UART_RX;
  //       break;
  //     }
  //     if (ch[9].xMap[i] == node) {
  //       gpioNumber = RP_UART_TX;
  //       break;
  //     }
  //   }

  //   break;
  // }
  default:
  // Serial.print("cant find node = ");
  // Serial.println(node);
    return true;
  }
  // Serial.print("gpioNumber = ");
  // Serial.println(gpioNumber);

  switch (gpioNumber) {
  case RP_GPIO_1:
    gpioPin = GPIO_1_PIN;
    break;
  case RP_GPIO_2:

    gpioPin = GPIO_2_PIN;
    break;
  case RP_GPIO_3:

    gpioPin = GPIO_3_PIN;
    break;
  case RP_GPIO_4:
    gpioPin = GPIO_4_PIN;
    break;
  case RP_GPIO_5:
    gpioPin = GPIO_5_PIN;
    break;
  case RP_GPIO_6:
    gpioPin = GPIO_6_PIN;
    break;
  case RP_GPIO_7:
    gpioPin = GPIO_7_PIN;
    break;
  case RP_GPIO_8:
    gpioPin = GPIO_8_PIN;
    break;
    
    
  case RP_UART_RX:
    gpioPin = GPIO_RX_PIN;
    break;
  case RP_UART_TX:
    gpioPin = GPIO_TX_PIN;
    break;
  }
  // Serial.print("gpioPin = ");
  // Serial.println(gpioPin);

  // removeBridgeFromNodeFile(gpioNumber, -1, netSlot, 1);
  addBridgeToNodeFile(node, gpioNumber, netSlot, 1);
  refreshLocalConnections(0, 0, 0); 
  //refreshBlind(0, 0, 0);
  waitCore2();
  //delay(100);

  int floating = gpioReadWithFloating(gpioPin, 100);
  // Serial.print("floating = ");
  // Serial.println(floating);

removeBridgeFromNodeFile(node, gpioNumber, netSlot, 1);
refreshLocalConnections(0, 0, 0);
waitCore2();

  if (floating == 2) {
    return true;
  } else {
    return false;
  }

  

  // return floating;
  // delayMicroseconds(30);
  // int reading = digitalRead(gpioPin);

  
  // if (reading == HIGH) {

  //   removeBridgeFromNodeFile(node, gpioNumber, netSlot, 1);

  //   return true;
  // } else {
  //   removeBridgeFromNodeFile(node, gpioNumber, netSlot, 1);
  //   return false;
  // }
}

float measureCurrent(int node1, int node2) { return 0; }

void setRailVoltage(int topBottom, float voltage) {
  switch (topBottom) {
  case 0:
    setTopRail(voltage, 1);
    break;
  case 1:
    setBotRail(voltage, 1);
    break;
  default:
    break;
  }

  return;
}

void connectGPIO(int gpioNumber, int node) {

  switch (gpioNumber) {
  case 1:
    gpioNumber = RP_GPIO_1;
    break;
  case 2:
    gpioNumber = RP_GPIO_2;
    break;
  case 3:
    gpioNumber = RP_GPIO_3;
    break;
  case 4:
    gpioNumber = RP_GPIO_4;
    break;
  case 5:
    gpioNumber = RP_GPIO_5;
    break;
  case 6:
    gpioNumber = RP_GPIO_6;
    break;
  case 7:
    gpioNumber = RP_GPIO_7;
    break;
  case 8:
    gpioNumber = RP_GPIO_8;
    break;
  }
  addBridgeToNodeFile(gpioNumber, node, netSlot, 0);
  refreshConnections();
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
      if (getSlotLength(i, 0) > 0) {  // Only print headers and content if slot has content
        Serial.print("\n\rSlot ");
        Serial.print(i);
        if (i == netSlot) {
          Serial.print("        <--- current slot");
        }

        Serial.print("\n\rnodeFileSlot");
        Serial.print(i);
        Serial.print(".txt\n\r");
        Serial.print("\n\rf ");
        printNodeFile(i, 0, 0, 0, false);
        Serial.print("\n\n\r");
      }
    }
  } else {

    Serial.print("\n\rnodeFileSlot");
    Serial.print(fileNo - '0');
    Serial.print(".txt\n\r");

    Serial.print("\n\rf ");

    printNodeFile(fileNo - '0', 0, 0, 0, true); // Print empty slots when showing specific slot
    Serial.print("\n\r");
  }
}

