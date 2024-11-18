// SPDX-License-Identifier: MIT

/*
Kevin Santo Cappuccio
Architeuthis Flux

KevinC@ppucc.io

5/28/2024

*/

#define PICO_RP2350B 1
#include <Arduino.h>

//  #define LED LED_BUILTIN
//  #ifdef USE_TINYUSB
//  #include
//  "../include/Adafruit_TinyUSB_Arduino_changed/Adafruit_TinyUSB_changed.h"
//  #include
//  "../lib/Adafruit_TinyUSB_Arduino_changed/src/Adafruit_TinyUSB_changed.h"
//  #endif

// #include <Adafruit_TinyUSB.h>

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
// #ifdef CFG_TUSB_CONFIG_FILE
// #include CFG_TUSB_CONFIG_FILE
// #else
// #include "tusb_config.h"
// #endif
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Commands.h"

#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "MachineCommands.h"
#include "MatrixStateRP2040.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "RotaryEncoder.h"
#include "ArduinoStuff.h"

#ifdef USE_FATFS
#include "FatFS.h"
#include "USBfs.h"
#endif
#include "UserCode.h"

// #include <usb-descriptors.c>
//  #include <FreeRTOS.h>

//  #include "hardware/flash.h"
//   #include "pico/multicore.h"
//   #include <picosdk/src/rp2_common/hardware_flash.h>

// #include "AdcUsb.h"
// #include "logic_analyzer.h"
// LogicAnalyzer logicAnalyzer;
// Capture capture(MAX_FREQ, MAX_FREQ_THRESHOLD);

// #include "pico/multicore.h"
// #include "hardware/watchdog.h"

// using namespace logic_analyzer;

bread b;


// bool core1_separate_stack = true;

// SerialUSB Serial;
//  SerialUSB Serial5;
// SerialUSB Serial3;

// PluggableUSBModule PluggableUSB(2, 2, 0);

// RawHID_::RawHID_(void) : PluggableUSBModule(1, 1, epType),
// protocol(HID_REPORT_PROTOCOL), idle(1), dataLength(0), dataAvailable(0),
// featureReport(NULL), featureLength(0)
// {
// 	epType[0] = EP_TYPE_INTERRUPT_IN;

// }

int supplySwitchPosition = 0;
volatile bool core1busy = false;
volatile bool core2busy = false;

// void lastNetConfirm(int forceLastNet = 0);
void rotaryEncoderStuff(void);

void core2stuff(void);

volatile uint8_t pauseCore2 = 0;

volatile int loadingFile = 0;

unsigned long lastNetConfirmTimer = 0;
// int machineMode = 0;

int rotEncInit = 0;
// https://wokwi.com/projects/367384677537829889

int core2initFinished = 0;



void setup() {
  pinMode(RESETPIN, OUTPUT_12MA);

  digitalWrite(RESETPIN, HIGH);
  /// multicore_lockout_victim_init();
  delayMicroseconds(800);
  // Serial.setTimeout(8000);
  //  USB_PID = 0xACAB;
  //  USB_VID = 0x1D50;
  //  USB_MANUFACTURER = "Architeuthis Flux";
  //  USB_PRODUCT = "Jumperless";
  //  USBSetup
  // Serial.ignoreFlowControl(true);
  Serial.begin(115200);

  // while (tud_cdc_connected() == 0 ) {
  //   backpowered = 1;
  //   delay(1);
  // }
  backpowered = 0;
  // USBDevice.setProductDescriptor("Jumperless");
  // USBDevice.setManufacturerDescriptor("Architeuthis Flux");
  // USBDevice.setSerialDescriptor("0");
  // USBDevice.setID(0x1D50, 0xACAB);
  // USBDevice.addStringDescriptor("Jumperless");
  // USBDevice.addStringDescriptor("Architeuthis Flux");
  // USBDevice.begin();
  // stdio_usb_init();

  // Serial2.begin(115200);
  // SerialUSB
  // usbd_serial_init();

  // for (int itf = 0; itf < CFG_TUD_CDC; itf++)
  // init_uart_data(itf);

  // PluggableUSB().plug(Serial3);
  EEPROM.begin(512);

  debugFlagInit();

  delay(5);
  // initGPIOex();
  //  delay(5);
  //  Serial.setTimeout(500);

  // delay(20);
  //  pinMode(buttonPin, INPUT_PULLDOWN);
  initDAC(); // also sets revisionNumber
  pinMode(probePin, OUTPUT_8MA);
  pinMode(10, OUTPUT_8MA);

  digitalWrite(10, HIGH);

  // delay(10);

  // delay(1);

  initINA219();

  initArduino();

  delay(4);
  // #ifdef USE_FATFS
  //  FatFS.begin();
  // #endif
  //  FatFS.format();

  // setDac0_5Vvoltage(0.0);
  // setDac1_8Vvoltage(1.9);

  // createSlots(-1, 0);
  // delay(10);
  clearAllNTCC();

  digitalWrite(RESETPIN, LOW);
  while (core2initFinished == 0) {
    delayMicroseconds(1);
  }
  pinMode(probePin, OUTPUT_8MA);
  // pinMode(buttonPin, INPUT_PULLDOWN);
  digitalWrite(probePin, HIGH);

  initRotaryEncoder();

  // delay(10);

  delay(4);

  // delay(100);
  initMenu();
  initADC();

  pinMode(18, INPUT_PULLUP);
  pinMode(19, INPUT_PULLUP);
  //  pinMode(18, OUTPUT);
  //   pinMode(19, OUTPUT);
  //   digitalWrite(18, HIGH);
  //   digitalWrite(19, HIGH);

  //  showLEDsCore2 = 1;
  // delay(1000);
  // setRailsAndDACs();

  routableBufferPower(1);
  // fatFS
  //  multicore_lockout_victim_init();
  // delay(400);
  // Serial.println("Jumperless");
}

void setupCore2stuff() {
  initCH446Q();

  delay(1);

  initLEDs();
  initRowAnimations();
  setupSwirlColors();
initSecondSerial();
  // delay(4);
}

void setup1() {
  // flash_safe_execute_core_init();


  setupCore2stuff();

  // if (backpowered == 1) {
  //   leds.clear();
  //   leds.show();
  //   while(backpowered == 1) {
  //     delay(1);
  //   }
  // }
  core2initFinished = 1;

  digitalWrite(GPIO_2_PIN, LOW);

  // delay(4);
  // multicore_lockout_victim_init();
  //  lightUpRail();

  // delay(4);
  // showLEDsCore2 = 1;
}

unsigned long teardownTimer = 0;
unsigned long lastTeardown = 0;
unsigned long teardownTime = 2000;

char connectFromArduino = '\0';

char input;

int serSource = 0;
int readInNodesArduino = 0;

int restoredNodeFile = 0;

const char firmwareVersion[] = "5.1.0.3"; // remember to update this

int firstLoop = 1;

volatile int probeActive = 0;

int showExtraMenu = 1;
int tearDownToggle = 0;

int tinyUSB = 0;
unsigned long timer = 0;
int lastProbeButton = 0;
unsigned long waitTimer = 0;
unsigned long switchTimer = 0;
int flashingArduino = 0;
int attractMode = 0;
void loop() {

  // delay(100);
  // setRailsAndDACs();
  //  if (PROTOTYPE_VERSION > 0) {

  //}
menu:
  getNothingTouched();
  // Serial.print("nothing touched reading = ");
  // Serial.println(getNothingTouched());

  // pinMode(A0, INPUT);
  // pinMode(A1, INPUT);
  // pinMode(A2, INPUT);
  // pinMode(A3, INPUT);
  // pinMode(A4, INPUT);
  // pinMode(A5, INPUT);
  // pinMode(A6, INPUT);
  // pinMode(A7, INPUT);

  printMainMenu(showExtraMenu);
  //   while (Serial.available() == 0) {
  //      Serial.print("ADC 0: ");
  //   Serial.println(analogRead(A0));
  //   Serial.print("ADC 1: ");
  //   Serial.println(analogRead(A1));
  //   Serial.print("ADC 2: ");
  //   Serial.println(analogRead(A2));
  //   Serial.print("ADC 3: ");
  //   Serial.println(analogRead(A3));
  //   Serial.print("ADC 4: ");
  //  Serial.println(analogRead(A4));
  //   //Serial.print(ADC5_PIN);
  //   Serial.print(" ADC 5: ");
  //   Serial.println(analogRead(A5));

  //      Serial.print("ADC 6: ");
  //   Serial.println(analogRead(A6));
  //   Serial.print("ADC 7: ");
  //   Serial.println(analogRead(A7));
  //   Serial.println();

  //   delay(100);
  //   }
  //   int tg = 0;
  // while(1) {

  //   digitalWrite(GPIO_1_PIN, tg);
  //   tg = !tg;
  //   delay(10);
  // }
  Serial.println();

  //   while(1)
  // {
  //   Serial3.println("Hello");
  //   delay(1000);
  //   Serial5.println("Fuck");
  //   delay(1000);
  // }

  int toggg = 0;

  int chipSc = 0;
  unsigned long lastTimerrr = 0;

  if (firstLoop == 1) {
    firstLoop = 0;
    // delay(100);
    setRailsAndDACs();
    if (attractMode == 1) {
      defconDisplay = 0;
      netSlot = -1;
    } else {

      defconDisplay = -1;
    }

    goto loadfile;
  }

dontshowmenu:
  // if (debugNTCC > 0) {
  // Serial.print("debugLEDs = ");
  //   Serial.println(debugLEDs);
  // }
  // Serial.print("core2busy = ");
  // Serial.println(core2busy);

  // defconDisplay = 0;
  //  readVoltages();
  // routableBufferPower(1);
  //  refreshConnections();

  // delay(500);
  // Serial.print("ADC 6: ");
  // Serial.println(analogRead(ADC6_PIN));
  // Serial.print("ADC 7: ");
  // Serial.println(analogRead(ADC7_PIN));

  connectFromArduino = '\0';
  // showLEDsCore2 = 1;
  while (Serial.available() == 0 && connectFromArduino == '\0' &&
         slotChanged == 0) {
    // digitalWrite(RESETPIN, HIGH);

    // if (digitalRead(18) == 0 || digitalRead(19) == 0) {
    //   Serial.println("switching");
    //   flashingArduino = 1;
    //   removeBridgeFromNodeFile(NANO_D0, -1, netSlot, 0);
    //   removeBridgeFromNodeFile(NANO_D1,  -1, netSlot, 0);

    //   refreshConnections(1);
    //   delay(1000);
    //   printNodeFile(netSlot, 0, 1);
    //   printNodeFile(netSlot, 0, 0);

    //  //refreshConnections();
    //   refreshLocalConnections(-1);

    //   //delay(3000);
    //   //refreshConnections();
    //   //flashingArduino = 0;
    // }

    if (attractMode == 1) {
      // rotaryEncoderStuff();

      if (encoderDirectionState == DOWN) {
        // attractMode = 0;
        defconDisplay = -1;
        netSlot++;
        if (netSlot >= NUM_SLOTS) {
          netSlot = -1;
          defconDisplay = 0;
        }
        Serial.print("netSlot = ");
        Serial.println(netSlot);
        slotChanged = 1;
        showLEDsCore2 = -1;
        encoderDirectionState = NONE;
        goto loadfile;
        // goto menu;
      } else if (encoderDirectionState == UP) {
        // attractMode = 0;
        defconDisplay = -1;
        netSlot--;
        if (netSlot <= -1) {
          netSlot = NUM_SLOTS;
          defconDisplay = 0;
        }
        Serial.print("netSlot = ");
        Serial.println(netSlot);
        slotChanged = 1;
        showLEDsCore2 = -1;
        encoderDirectionState = NONE;
        goto loadfile;
        // goto menu;
      }
    }
    // {  pinMode(26, INPUT);
    // pinMode(10, OUTPUT_8MA);
    // digitalWrite(10, HIGH);
    // pinMode(9, OUTPUT_8MA);
    // digitalWrite(9, HIGH);
    // Serial.println(defconDisplay);
    // yield();

    // core1busy = false;
    if (clickMenu() >= 0) {
      // defconDisplay = -1;
      goto loadfile;
    }
    int probeReading = justReadProbe();

    if (probeReading > 0) {
      // Serial.print("justReadProbe = ");
      // Serial.println(probeReading);
      // delay(100);

      int netHighlighted = brightenNet(probeReading);
      // Serial.print("netHighlighted = ");
      // Serial.println(netHighlighted);
      if (netHighlighted != -1) {

        Serial.print("\r                                               \r");
        switch (netHighlighted) {
        case 0:
          break;
        case 1:
          Serial.print("Net Highlighted = GND");
          break;
        case 2:
          Serial.print("Net Highlighted = Top Rail  ");

          Serial.print(railVoltage[0]);
          break;
        case 3:
          Serial.print("Net Highlighted = Bottom Rail  ");

          Serial.print(railVoltage[1]);
          break;
        case 4:
          Serial.print("Net Highlighted = DAC 0  ");
          Serial.print(dacOutput[0]);
          break;
        case 5:
          Serial.print("Net Highlighted = DAC 1  ");
          Serial.print(dacOutput[1]);
          break;
        default:
          Serial.print("Net Highlighted = ");
          Serial.print(netHighlighted);
        }
        showLEDsCore2 = 1;
      } else {

        // brightenNet(-1);
      }

      //      // showLEDsCore2 = 1;
    }

    //   int netFound = findNodeInNet(probeReading);
    // if (netFound != -1) {
    //   Serial.print("findNodeInNet = ");
    //   Serial.println(netFound);
    //   brightenNet(netFound);
    //   showLEDsCore2 = 1;
    //   //brightenedNet = netFound;
    // } else {
    //  brightenNet(-1);
    // }

    // }
    // Serial.println(digitalRead(11));
    // delay(300);

    // Serial.println(digitalRead(buttonPin));

    if ((millis() - waitTimer) > 100) {
      waitTimer = millis();

      int probeButton = checkProbeButton();
      // Serial.print("lasrProbeButton = ");
      // Serial.println(lastProbeButton);
      //     Serial.print("probeButton = ");
      //     Serial.println(probeButton);

      if (probeButton != lastProbeButton) {

        // Serial.print("probeButton = ");
        // Serial.println(probeButton);
        // Serial.print("lastProbeButton = ");
        // Serial.println(lastProbeButton);

        lastProbeButton = probeButton;

        // if (switchPosition == 1) {
        if (probeButton > 0) {
          //&& inPadMenu == 0) {

          // Serial.print("probeButton = ");
          // Serial.println(probeButton);

          // int longShort = longShortPress(1000);
          // defconDisplay = -1;
          // probeLEDs.show();
          if (probeButton == 2) {
            connectOrClearProbe = 1;
            probeActive = 1;
            showProbeLEDs = 1;
            input = 'p';
            probingTimer = millis();
            goto skipinput;
          } else if (probeButton == 1) {
            // getNothingTouched();
            connectOrClearProbe = 0;
            showProbeLEDs = 2;
            probeActive = 1;
            input = 'c';
            probingTimer = millis();
            // Serial.println("probing\n\r");

            goto skipinput;
          }
        }

        //   } else {
        //    measureMode();
        //  }
      }
    }
    // pinMode(19, INPUT);
    // } else if ((millis() - waitTimer) > 50) {
    //   if (PROTOTYPE_VERSION > 0) {
    //     // Serial.print("Prototype Version ");
    //     // Serial.print(PROTOTYPE_VERSION);
    //     // Serial.print("\n\r");
    // checkPads();

    // if ((millis() - switchTimer) > 1000) {
    //   switchTimer = millis();
    //   // checkPads();
    //   checkSwitchPosition();
    //   // Serial.print("switchPosition = ");
    //   // Serial.println(switchPosition);

    //     if (switchPosition == 1) {
    //       showProbeLEDs = 4;
    //     } else if (switchPosition == 0) {
    //       showProbeLEDs = 6;
    //     }

    // }

    if (showReadings >= 1) {
      chooseShownReadings();
      showMeasurements();
    }
  }

  if (slotChanged == 1) {
    goto loadfile;
  }

  if (connectFromArduino != '\0') {
  } else {
    input = Serial.read();
    // Serial.print("\n\r");
    if (input == '}' || input == ' ' || input == '\n' || input == '\r') {
      goto dontshowmenu;
    }
  }

skipinput:
  switch (input) {

  case '^': {
    doomOn = 1;
    // playDoom();
    // doomOn = 0;
    break;
  }
  case '?': {
    Serial.print("Jumperless firmware version: ");
    Serial.println(firmwareVersion);
    break;
  }
  case '@': {
    printWireStatus();
    break;
  }
  case '$': {
    // return current slot number
    Serial.println(netSlot);
    break;
  }
  case 'u': {
    // setRailVoltage(0, 2.0);
    sketchOne();
    break;
  }
  case 'a': {
ManualArduinoReset = true;
    break;
  }

  case '+': {
#ifdef USE_FATFS
    refreshConnections();
    saveLocalNodeFile();

    printNodeFile();
    delay(10);
    usbFSbegin();
    delay(10);
    while (Serial.available() == 0) {
      USBloop();
    }
    delay(10);
    USBdisconnect();
    delay(10);
    goto loadfile;
    refreshConnections();
#endif
    break;
  }

  case 'i': {
    Serial.println("I2C scan\n\n\r");
    Serial.println("enter SCL row\n\r");
    Serial.print("SCL = ");
    delay(100);
    int sclRow = -1;
    int sdaRow = -1;
    while (Serial.available() == 0) {
    }
    while (sclRow == -1 || (sclRow < 1 || sclRow > 100)) {
      sclRow = Serial.parseInt();
      delay(100);
    }
    Serial.println("enter SDA row\n\r");

    while (Serial.available() == 0) {
    }
    while (sdaRow == -1 || (sdaRow < 1 || sdaRow > 100)) {
      sdaRow = Serial.parseInt();
      delay(100);
    }
    Serial.print("SCL = ");
    Serial.println(sclRow);
    Serial.print("SDA = ");
    Serial.println(sdaRow);
    Serial.println("Scanning I2C bus\n\r");

    i2cScan(sdaRow, sclRow);
    break;
  }

  case 'g': {
    // pauseCore2 = 1;
    //  while (slotChanged == 0)
    //  {
    //
    while (Serial.available() == 0 && slotChanged == 0) {
      if (slotChanged == 1) {
        // b.print("Jumperless", 0x101000, 0x020002, 0);
        // delay(100);
        goto menu;
      }
    }
    b.clear();

    char f[80] = {' '};
    int index = 0;
    // leds.clear();
    while (Serial.available() > 0) {
      if (index > 19) {
        break;
      }
      f[index] = Serial.read();
      index++;
      // b.print(f);
      // delayMicroseconds(30);
      // leds.show();
    }
    f[index] = ' ';
    f[index + 1] = ' ';
    uint32_t color = 0x100010;
    // Serial.print(index);
    defconString[0] = f[0];
    defconString[1] = f[1];
    defconString[2] = f[2];
    defconString[3] = f[3];
    defconString[4] = f[4];
    defconString[5] = f[5];
    defconString[6] = f[6];
    defconString[7] = f[7];
    defconString[8] = f[8];
    defconString[9] = f[9];
    defconString[10] = f[10];
    defconString[11] = f[11];
    defconString[12] = f[12];
    defconString[13] = f[13];
    defconString[14] = f[14];
    defconString[15] = f[15];
    defconDisplay = 0;
    // b.print(f, color);

    break;
  }
  case 'e': {
    if (showExtraMenu == 0) {
      showExtraMenu = 1;
    } else {
      showExtraMenu = 0;
    }
    break;
  }

  case 's': {
    printSlots(-1);

    break;
  }
  case 'v':

    if (showReadings >= 3 || (inaConnected == 0 && showReadings >= 1)) {
      showReadings = 0;
      break;
    } else {
      showReadings++;

      chooseShownReadings();
      // Serial.println(showReadings);

      goto dontshowmenu;
      // break;
    }
  case 'p': {
    // probeActive = 1;
    //   Serial.print("pdebugLEDs = ");
    //  Serial.println(debugLEDs);
    delayMicroseconds(5);
    probeMode(10, 1);
    //      Serial.print("apdebugLEDs = ");
    // Serial.println(debugLEDs);
    delayMicroseconds(5);
    probeActive = 0;
    // clearLEDs();
    // assignNetColors();
    // showNets();
    // showLEDsCore2 = 1;
    break;
  }
  case 'c': {
    // removeBridgeFromNodeFile(19, 1);
    // probeActive = 1;
    delayMicroseconds(5);
    probeMode(19, 0);
    delayMicroseconds(5);
    probeActive = 0;
    // clearLEDs();
    // assignNetColors();
    // showNets();
    // showLEDsCore2 = 1;
    break;
  }
  case 'n':
    couldntFindPath(1);
    Serial.print("\n\n\rnetlist\n\n\r");
    listSpecialNets();
    listNets();

    break;
  case 'b':
    couldntFindPath(1);
    Serial.print("\n\n\rBridge Array\n\r");
    printBridgeArray();
    Serial.print("\n\n\n\rPaths\n\r");
    printPathsCompact();
    Serial.print("\n\n\rChip Status\n\r");
    printChipStatus();
    Serial.print("\n\n\r");
    Serial.print("Revision ");
    Serial.print(revisionNumber);
    Serial.print("\n\n\r");
    break;

  case 'm':

    break;

  case '!':
    printNodeFile();
    break;

  case 'w':

    if (waveGen() == 1) {
      break;
    }
  case 'o': {
    // probeActive = 1;
    inputNodeFileList(rotaryEncoderMode);
    showSavedColors(netSlot);
    // input = ' ';
    showLEDsCore2 = -1;
    // probeActive = 0;
    goto loadfile;
    // goto dontshowmenu;
    break;
  }

  case 'x': {

    if (netSlot == NUM_SLOTS - 1) {
      netSlot = 0;
    } else {
      netSlot++;
    }

    Serial.print("\r                                         \r");
    Serial.print("Slot ");
    Serial.print(netSlot);
    slotPreview = netSlot;
    goto loadfile;
  }
  case 'z': {

    if (netSlot == 0) {
      netSlot = NUM_SLOTS - 1;
    } else {
      netSlot--;
    }
    Serial.print("\r                                         \r");
    Serial.print("Slot ");
    Serial.print(netSlot);
    slotPreview = netSlot;
    goto loadfile;
  }
  case 'y': {
  loadfile:
    loadingFile = 1;

    // digitalWrite(RESETPIN, HIGH);

    // delayMicroseconds(2);

    // digitalWrite(RESETPIN, LOW);

    // showSavedColors(netSlot);
    //  drawWires();

    // showLEDsCore2 = 1;
    slotChanged = 0;
    loadingFile = 0;
    refreshConnections();
    chooseShownReadings();
    // setGPIO();
    break;
  }
  case 'f':

    probeActive = 1;
    readInNodesArduino = 1;
    // clearAllNTCC();

    // sendAllPathsCore2 = 1;
    // timer = millis();

    // clearNodeFile(netSlot);

    if (connectFromArduino != '\0') {
      serSource = 1;
    } else {
      serSource = 0;
    }
    savePreformattedNodeFile(serSource, netSlot, rotaryEncoderMode);

    refreshConnections(-1);

    if (debugNMtime) {
      Serial.print("\n\n\r");
      Serial.print("took ");
      Serial.print(millis() - timer);
      Serial.print("ms");
    }
    input = ' ';

    probeActive = 0;
    if (connectFromArduino != '\0') {
      connectFromArduino = '\0';
      // Serial.print("connectFromArduino\n\r");
      //  delay(2000);
      input = ' ';
      readInNodesArduino = 0;

      goto dontshowmenu;
    }
    // chooseShownReadings();

    connectFromArduino = '\0';
    readInNodesArduino = 0;
    break;

  case '\n':
    goto menu;
    break;

  case 't':
#ifdef FSSTUFF
    clearNodeFile();
#endif

#ifdef EEPROMSTUFF
    lastCommandWrite(input);

    runCommandAfterReset('t');
#endif

#ifdef FSSTUFF
    openNodeFile();
    getNodesToConnect();
#endif
    Serial.println("\n\n\rnetlist\n\n\r");

    bridgesToPaths();

    listSpecialNets();
    listNets();
    printBridgeArray();
    Serial.print("\n\n\r");
    Serial.print(numberOfNets);

    Serial.print("\n\n\r");
    Serial.print(numberOfPaths);

    assignNetColors();
#ifdef PIOSTUFF
    sendAllPaths();
#endif

    break;

  case 'l':
    if (LEDbrightnessMenu() == '!') {
      clearLEDs();
      delayMicroseconds(9200);
      sendAllPathsCore2 = 1;
    }
    break;

    goto dontshowmenu;

    break;

  case 'd': {
    debugFlagInit();

  debugFlags:

    Serial.print("\n\r0.   all off");
    Serial.print("\n\r9.   all on");
    Serial.print("\n\ra-z. exit\n\r");

    Serial.print("\n\r1. file parsing           =    ");
    Serial.print(debugFP);
    Serial.print("\n\r2. net manager            =    ");
    Serial.print(debugNM);
    Serial.print("\n\r3. chip connections       =    ");
    Serial.print(debugNTCC);
    Serial.print("\n\r4. chip conns alt paths   =    ");
    Serial.print(debugNTCC2);
    Serial.print("\n\r5. LEDs                   =    ");
    Serial.print(debugLEDs);
    Serial.print("\n\n\r6. swap probe pin         =    ");
    if (probeSwap == 0) {
      Serial.print("19");
    } else {
      Serial.print("18");
    }

    Serial.print("\n\n\n\r");

    while (Serial.available() == 0)
      ;

    int toggleDebug = Serial.read();
    Serial.write(toggleDebug);
    toggleDebug -= '0';

    if (toggleDebug >= 0 && toggleDebug <= 9) {

      debugFlagSet(toggleDebug);

      delay(10);

      goto debugFlags;
    } else {
      break;
    }
  }

  case ':':

    if (Serial.read() == ':') {
      // Serial.print("\n\r");
      // Serial.print("entering machine mode\n\r");
      // machineMode();
      showLEDsCore2 = 1;
      goto dontshowmenu;
      break;
    } else {
      break;
    }

  default:
    while (Serial.available() > 0) {
      int f = Serial.read();
      // delayMicroseconds(30);
    }

    break;
  }

  goto menu;
}

unsigned long logoFlashTimer = 0;

int arduinoReset = 0;
unsigned long lastTimeReset = 0;

unsigned long lastSwirlTime = 0;

int swirlCount = 42;
int spread = 13;

int csCycle = 0;
int onOff = 0;
float topRailVoltage = 0.0;
float botRailVoltage = 0.0;

int readcounter = 0;
unsigned long schedulerTimer = 0;
unsigned long schedulerUpdateTime = 3900;
int rowProbed = 0;
int swirled = 0;
int countsss = 0;

int probeCycle = 0;
int netUpdateRefreshCount = 0;

int tempDD = 0;
int clearBeforeSend = 0;

unsigned long measureLEDTimer = 0;
int lastProbeLEDs = -1;
volatile int showingProbeLEDs = 0;

unsigned long ardTimer = 0;





void loop1() {
  // int timer = micros();

  if (doomOn == 1) {
    playDoom();
    doomOn = 0;
  } else {
    core2stuff();
  }

secondSerialHandler();


  // if (millis() - ardTimer == 2000) {
  //   Serial1.begin(115200);
  // }
  //}

  // timer = micros() - timer;

  // yield();
}

void core2stuff() // core 2 handles the LEDs and the CH446Q8
{
  core2busy = false;

  if (showLEDsCore2 < 0) {
    showLEDsCore2 = abs(showLEDsCore2);
    // Serial.println("clearBeforeSend = 1");

    clearBeforeSend = 1;
  }

  if (showProbeLEDs != lastProbeLEDs) {
    // lastProbeLEDs = showProbeLEDs;
    //  probeLEDs.clear();
  }
  if (micros() - schedulerTimer > schedulerUpdateTime || showLEDsCore2 == 3 ||
      showLEDsCore2 == 4 && core1busy == false) {

    if (((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3 ||
         (swirled == 1) && sendAllPathsCore2 == 0) ||
        showProbeLEDs != lastProbeLEDs) {

      // Serial.println(showLEDsCore2);
      int rails =
          showLEDsCore2; // 3 doesn't show nets and keeps control of the LEDs

      //       if (clearBeforeSend == 1) {
      //   clearLEDsExceptRails();
      //   Serial.println("clearing");
      //   //clearBeforeSend = 0;
      // }

      if (rails != 3) {
        core2busy = true;
        lightUpRail(-1, -1, 1);
        logoSwirl(swirlCount, spread, probeActive);
        core2busy = false;
      }

      if (rails == 5 || rails == 3) {
        core2busy = true;

        logoSwirl(swirlCount, spread, probeActive);
        core2busy = false;
      }

      if (rails != 2 && rails != 5 && rails != 3 && inClickMenu == 0 &&
          inPadMenu == 0) {
        if (defconDisplay >= 0 && probeActive == 0) {
          core2busy = true;
          defcon(swirlCount, spread, defconDisplay);
          core2busy = false;
        }
        if ((rails == 1 || probeActive == 1 || true) && inClickMenu == 0 &&
            inPadMenu == 0) {
          // multicore_lockout_start_blocking();
          // multicore_lockout_start_timeout_us(1000);

          while (core1busy == true) {
            // core2busy = false;
          }
          core2busy = true;
          if (clearBeforeSend == 1) {
            clearLEDsExceptRails();
            // Serial.println("clearing");
            clearBeforeSend = 0;
          }
          // Serial.println("showNets");
          // delay(100);
          showLEDmeasurements();
          // showAllRowAnimations();

          showNets();
          if (inClickMenu == 0) {
            showAllRowAnimations();
          }

          core2busy = false;
          netUpdateRefreshCount = 0;
          // showLEDmeasurements();
          // multicore_lockout_end_timeout_us(1000);
          // multicore_lockout_end_blocking();

        } else {
          // netUpdateRefreshCount++;
          //  Serial.println(rails);
        }

      } else {

        // Serial.print("showLEDsCore2 = ");
        // Serial.println(showLEDsCore2);
        // Serial.print("inClickMenu = ");
        // Serial.println(inClickMenu);
        // Serial.print("inPadMenu = ");
        // Serial.println(inPadMenu);
      }

      // delayMicroseconds(220);
      core2busy = true;

      leds.show();
      core2busy = false;
      // probeLEDs.clear();

      if (checkingButton == 0) {
        //   Serial.print("probeActive = ");
        //   Serial.println(probeActive);
        // showProbeLEDs = probeCycle;
        // while(checkingButton == 1) {
        //   //Serial.println("checkingButton");
        // }
        core2busy = true;
        // pinMode(2, OUTPUT);
        // pinMode(9, INPUT);
        showingProbeLEDs = 1;
        //         Serial.print("showProbeLEDs = ");
        // Serial.println(showProbeLEDs);
        switch (showProbeLEDs) {
        case 1:
          probeLEDs.setPixelColor(0, 0x000036); // connect
          // probeLEDs[0].setColorCode(0x000011);
          //  Serial.println(showProbeLEDs);
          //   probeLEDs.show();
          break;
        case 2:
          probeLEDs.setPixelColor(0, 0x360000); // remove
          // probeLEDs[0].setColorCode(0x110000);
          //  probeLEDs.show();
          //  Serial.println(showProbeLEDs);
          break;
        case 3:
          probeLEDs.setPixelColor(0, 0x003600); // measure
          // probeLEDs[0].setColorCode(0x001100);
          //  probeLEDs.show();
          //  Serial.println(showProbeLEDs);
          break;
        case 4:

          probeLEDs.setPixelColor(0, 0x170c17); // select idle
          // probeLEDs[0].setColorCode(0x110011);
          //  probeLEDs.show();
          //  Serial.println(showProbeLEDs);
          break;
        case 5:
          probeLEDs.setPixelColor(0, 0x111111); // all
          // probeLEDs[0].setColorCode(0x111111);
          //  Serial.println(showProbeLEDs);
          break;
        case 6:
          probeLEDs.setPixelColor(0, 0x0c190c); // measure dim
          break;

        default:
          break;
        }
        lastProbeLEDs = showProbeLEDs;

        // while(checkingButton == 1) {
        //   //Serial.println("checkingButton");
        // }

        // FastLED.show();
        // delay(1);
        /// pinMode(2, INPUT);
        // pinMode(9, OUTPUT);
        //       Serial.print("showProbeLEDs = ");
        // Serial.println(showProbeLEDs);

        probeLEDs.show();
        showingProbeLEDs = 0;
        core2busy = false;
      }
      if (rails != 3 && swirled == 0) {
        showLEDsCore2 = 0;

        // delayMicroseconds(3200);
      }

      swirled = 0;
      if (inClickMenu == 1) {
        rotaryEncoderStuff();
      }
      core2busy = false;

    } else if (sendAllPathsCore2 != 0) {
      sendPaths();

    } else if (millis() - lastSwirlTime > 51 && loadingFile == 0 &&
               showLEDsCore2 == 0 && core1busy == false) {
      readcounter++;

      // logoSwirl(swirlCount, spread, probeActive);

      lastSwirlTime = millis();

      if (swirlCount >= LOGO_COLOR_LENGTH - 1) {
        swirlCount = 0;

      } else {

        swirlCount++;
      }

      if (swirlCount % 20 == 0) {
        countsss++;
        // showProbeLEDs++;
        // if (showProbeLEDs > 5) {
        //   showProbeLEDs = 1;
        // }
        // Serial.print("showProbeLEDs = ");
        // Serial.println(showProbeLEDs);
        // defconDisplay = countsss;
        // showAllRowAnimations();
      }

      if (showLEDsCore2 == 0) {
        swirled = 1;
      }

      // leds.show();
    } else if (inClickMenu == 0 && probeActive == 0) {

      if (((countsss > 8 && defconDisplay >= 0) || countsss > 10) &&
          defconDisplay != -1) {
        countsss = 0;

        if (defconDisplay != -1) {
          tempDD++;

          if (tempDD > 6) {
            tempDD = 0;
          }
          // Serial.print("tempDD = ");
          // Serial.println(tempDD);
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

        //  readGPIO();
      }

      // readGPIO();
      // multicore_lockout_start_blocking();
      core2busy = true;
      rotaryEncoderStuff();
      core2busy = false;
      // multicore_lockout_end_blocking();

      if (probeActive == 0 && measureModeActive == 0) {
        if (millis() - measureLEDTimer > 50) {
          measureLEDTimer = millis();

          showLEDmeasurements();
        }
      }
      if (inClickMenu == 0 && loadingFile == 0 && showLEDsCore2 == 0 &&
          core1busy == false) {
        // showAllRowAnimations();
      }
    } else {
      // readGPIO();
    }
    schedulerTimer = micros();
    // readGPIO();
  }
}

void sendPaths(void) {
  // if (sendAllPathsCore2 == 1) {
  while (core1busy == true) {
  }
  core2busy = true;
  if (sendAllPathsCore2 != -1) {

    // Serial.println("sendPaths");
    // multicore_lockout_start_blocking();
    // multicore_lockout_start_timeout_us(1000);
    digitalWrite(RESETPIN, HIGH);
    delayMicroseconds(50);
    digitalWrite(RESETPIN, LOW);
    delayMicroseconds(2200);
  }
  unsigned long pathTimer = micros();

  sendAllPaths();
  // core2busy = false;
  unsigned long pathTime = micros() - pathTimer;

  delayMicroseconds(3200);
  // Serial.print("pathTime = ");
  // Serial.println(pathTime);
  sendAllPathsCore2 = 0;
  core2busy = false;
  // }
}
