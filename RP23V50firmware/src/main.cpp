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

#include "ArduinoStuff.h"
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
#include "Apps.h"

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

  debugFlagInit(0);

  // delay(2000);
  //  initGPIOex();
  //   delay(5);
  //   Serial.setTimeout(500);

  // delay(20);
  //  pinMode(buttonPin, INPUT_PULLDOWN);
  initDAC();
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

  digitalWrite(RESETPIN, LOW);
  while (core2initFinished == 0) {
    delayMicroseconds(1);
    }

  clearAllNTCC();
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

  routableBufferPower(1, 0);
  routableBufferPower(1, 1);
  // fatFS
  //  multicore_lockout_victim_init();
  // delay(400);
  // Serial.println("Jumperless");
  }

void setupCore2stuff() {
  // delay(2000);
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


  // while(1) {
  //   delay(1);
  //   }
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

const char firmwareVersion[] = "5.1.0.5"; // remember to update this

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
int attract = 0;
#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/gpio.h>
void loop() {


  // unsigned long adcSpeedTimer = 0;  
  // unsigned long adcSpeedTimer2 = 0;
  // float adcSpeed = 0;
  // int speedRead = 0;
  // int readSpeedCount = 0;
  // adc_init();
  // adc_gpio_init(ADC0_PIN);
  // adc_fifo_setup(true, false, 1, false, false);
  // adc_select_input(0);
  // adc_run(true);

  //   while (1){

  //    //speedRead = adc_read();
  //     //speedRead = adc_fifo_get_blocking();
  //     speedRead = analogRead(ADC0_PIN);
  //     //delayMicroseconds(1);

  //     readSpeedCount++;
  //     if (micros() - adcSpeedTimer2 > 1000000) {

  //       Serial.print("ADC Speed: ");
  //       Serial.println(readSpeedCount);
  //       adcSpeedTimer2 = micros();
  //       readSpeedCount = 0;
  //     }


  //   }

menu:
  getNothingTouched();
  //   Serial.print("\n\rPaths: \n\n\r");
  //   printPathsCompact();
  //   Serial.print("\n\n\n\rChip Status: \n\n\r");
  // printChipStatus();
    //printMainMenu(showExtraMenu);

  Serial.print("\n\n\r\t\tMenu\n\r");
  // Serial.print("Slot ");
  // Serial.print(netSlot);
 // Serial.print("\n\r");
  // Serial.print("\tm = show this menu\n\r");
  // Serial.print("\tn = show netlist\n\r");
  // Serial.print("\ts = show node files by slot\n\r");
  // Serial.print("\to = load node files by slot\n\r");
  // Serial.print("\tf = load node file to current slot\n\r");
  // // Serial.print("\tr = rotary encoder mode -");
  // //   rotaryEncoderMode == 1 ? Serial.print(" ON (z/x to cycle)\n\r")
  // //                          : Serial.print(" off\n\r");
  // // Serial.print("\t\b\bz/x = cycle slots - current slot ");
  // // Serial.print(netSlot);
  // Serial.print("\n\r");
  // Serial.print("\te = show extra menu options\n\r");

  // if (extraOptions == 1) {
  //   Serial.print("\tb = show bridge array\n\r");
  //   Serial.print("\tp = probe connections\n\r");
  //   Serial.print("\tw = waveGen\n\r");
  //   Serial.print("\tv = toggle show current/voltage\n\r");
  //   // Serial.print("\tu = set baud rate for USB-Serial\n\r");
  //   Serial.print("\tl = LED brightness / test\n\r");
  //   Serial.print("\td = toggle debug flags\n\r");
  // }
  // Serial.print("\tc = clear nodes with probe\n\r");

  Serial.print("\n\r");
  Serial.print("\tn = show netlist\n\r");
  Serial.print("\tb = show bridge array\n\r");
  Serial.print("\tf = load node file\n\r");
  Serial.print("\tm = show this menu\n\r");
  Serial.print("\td = toggle debug flags\n\r");
  Serial.print("\te = extra menu options\n\r");

  if (showExtraMenu == 1) {
    Serial.println();
    Serial.print("\t\b\bz/x = cycle slots\n\r");
    Serial.print("\t$ = calibrate DACs\n\r");

    Serial.print("\t# = print text from menu\n\r");
    Serial.print("\tl = LED brightness / test\n\r");
    Serial.print("\t^ = set DAC voltage\n\r");
    Serial.print("\t? = show firmware version\n\r");
    Serial.print("\t! = print node file\n\r");
    Serial.print("\ts = show all slot files\n\r");
    Serial.print("\to = load node file by slot\n\r");
    Serial.print("\tu = scan board\n\r");
    Serial.print("\tv = toggle show readings\n\r");


    }

  //  Serial.print("\n\r");
   // Serial.print("revision ");
   // Serial.println(senseRevision());
   // Serial.println("\n\r");
  //Serial.print("\ta = manual Arduino reset\n\r");
  //Serial.print("\t+ = refresh connections and save node file\n\r");
  //Serial.print("\ti = I2C scan\n\r");
  //Serial.print("\t@ = print wire status\n\r");



  //Serial.print("\tp = probe mode\n\r");
  //Serial.print("\tc = clear probe\n\r");
  //Serial.print("\tw = wave generator\n\r");




  //Serial.print("\tz = cycle slots backward\n\r");
  //Serial.print("\ty = load file\n\r");

  //Serial.print("\tt = clear node file and list nets\n\r");

  Serial.print("\n\n\r");
  //b.clear();
    //Serial.println(yesNoMenu());

  if (firstLoop == 1) {
    firstLoop = 0;
    // delay(100);
    setRailsAndDACs();
    if (attract == 1) {
      defconDisplay = 0;
      netSlot = -1;
      } else {

      defconDisplay = -1;
      }

    goto loadfile;
    }

dontshowmenu:

  // delay(500);
  // Serial.print("ADC 6: ");
  // Serial.println(analogRead(ADC6_PIN));
  // Serial.print("ADC 7: ");
  // Serial.println(analogRead(ADC7_PIN));

  connectFromArduino = '\0';
  // showLEDsCore2 = 1;
  while (Serial.available() == 0 && connectFromArduino == '\0' && slotChanged == 0) {

    if (attract == 1) {
      // rotaryEncoderStuff();
      if (attractMode() == 1) {
        goto loadfile;
        }
      }

    if (clickMenu() >= 0) {
      // defconDisplay = -1;
      goto loadfile;
      }
    int probeReading = justReadProbe();

    if (probeReading > 0) {
      highlightNets(probeReading);
      }

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

        } else if (probeButton > 0 && lastProbeButton > 0 &&
                   probeButton == lastProbeButton) {

        // Serial.print("probeButton = ");
        // Serial.print(probeButton);
        // Serial.print("\tlastProbeButton = ");
        // Serial.println(lastProbeButton);
        }
      }

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

    case '2': {
      runApp(2);
      break;
      }

    case '^': {
    // doomOn = 1;
    // Serial.println(yesNoMenu());
    // break;
    char f[8] = { ' ' };
    int index = 0;
    float f1 = 0.0;

    while (Serial.available() == 0) {
      }
    while (index < 8) {
      f[index] = Serial.read();
      index++;
      }

    f1 = atof(f);
    Serial.print("f = ");
    Serial.println(f1);
    setDac0voltage(f1);
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
    // printWireStatus();
    Serial.println("Fuck you!!!");

    // printPathArray();
    break;
    }
    case '$': {
    // return current slot number
    for (int d = 0; d < 4; d++) {
      Serial.print("dacSpread[");
      Serial.print(d);
      Serial.print("] = ");
      Serial.println(dacSpread[d]);
      }

    for (int d = 0; d < 4; d++) {
      Serial.print("dacZero[");
      Serial.print(d);
      Serial.print("] = ");
      Serial.println(dacZero[d]);
      }

    calibrateDacs();
    // Serial.println(netSlot);
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
    if (lightUpName == true)
      {
      lightUpName = false;
      } else {
      lightUpName = true;
      }



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

    oledTest(17, 18, 22, 23);
    // Serial.println("I2C scan\n\n\r");
    // Serial.println("enter SCL row\n\r");
    // Serial.print("SCL = ");
    // delay(100);
    // int sclRow = -1;
    // int sdaRow = -1;
    // while (Serial.available() == 0) {
    //   }
    // while (sclRow == -1 || (sclRow < 1 || sclRow > 100)) {
    //   sclRow = Serial.parseInt();
    //   delay(100);
    //   }
    // Serial.println("enter SDA row\n\r");

    // while (Serial.available() == 0) {
    //   }
    // while (sdaRow == -1 || (sdaRow < 1 || sdaRow > 100)) {
    //   sdaRow = Serial.parseInt();
    //   delay(100);
    //   }
    // Serial.print("SCL = ");
    // Serial.println(sclRow);
    // Serial.print("SDA = ");
    // Serial.println(sdaRow);
    // Serial.println("Scanning I2C bus\n\r");

   // i2cScan(sdaRow, sclRow);
    break;
    }

    case '#': {
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
    printTextFromMenu();

    clearLEDs();
    showLEDsCore2 = 1;
    defconDisplay = -1;
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
    case 'b': {
    int showDupes = 1;
    char in = Serial.read();
    if (in == '0') {
      showDupes = 0;
      } else if (in == '2') {
        showDupes = 2;
        }
      Serial.print("\n\rpathDuplicates: ");
      Serial.println(pathDuplicates);
      Serial.print("dacDuplicates: ");
      Serial.println(dacDuplicates);
      Serial.print("powerDuplicates: ");
      Serial.println(powerDuplicates);
      Serial.print("dacPriority: ");
      Serial.println(dacPriority);
      Serial.print("powerPriority: ");
      Serial.println(powerPriority);
      couldntFindPath(1);
      Serial.print("\n\rBridge Array\n\r");
      printBridgeArray();
      Serial.print("\n\n\n\rPaths\n\r");
      printPathsCompact(showDupes);
      Serial.print("\n\n\rChip Status\n\r");
      printChipStatus();
      Serial.print("\n\n\r");
      Serial.print("Revision ");
      Serial.print(revisionNumber);
      Serial.print("\n\n\r");
      break;
    }
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


    slotChanged = 0;
    loadingFile = 0;
    refreshConnections(-1);
    //chooseShownReadings();
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
      break;
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
    Serial.print("\n\r6. show probe current     =    ");
    Serial.print(showProbeCurrent);
    // Serial.print("\n\n\r6. swap probe pin         =    ");
    // if (probeSwap == 0) {
    //   Serial.print("19");
    // } else {
    //   Serial.print("18");
    // }

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
unsigned long schedulerUpdateTime = 5300;
int rowProbed = 0;
int swirled = 0;
int countsss = 0;

int probeCycle = 0;
int netUpdateRefreshCount = 0;

int tempDD = 0;
int clearBeforeSend = 0;

unsigned long measureLEDTimer = 0;

unsigned long ardTimer = 0;

unsigned long tempTimer = 0;
int lastTemp = 0;

void loop1() {
  // int timer = micros();

  if (doomOn == 1) {
    playDoom();
    doomOn = 0;
    } else {
    core2stuff();
    }
  // leds.clear();
  // leds.show();

  secondSerialHandler();

  // core2busy = true;
  // rotaryEncoderStuff();
  // core2busy = false;

  if (blockProbingTimer > 0) {
    if (millis() - blockProbingTimer > blockProbing) {
      blockProbing = 0;
      blockProbingTimer = 0;
      // Serial.println("probing unblocked");
      }
    }

  // if( millis() - probeRainbowTimer > 7)
  // {
  // hsvProbe++;
  // showProbeLEDs = 7;
  // probeRainbowTimer = millis();

  // }

  // if (millis() - tempTimer > 5000) {
  //     tempTimer = millis();
  //     Serial.print(" ");

  //     float temp = 0;
  //     for (int i = 0; i < 20; i++) {
  //       temp += analogReadTemp();
  //       delay(10);
  //     }
  //     temp = temp / 20;
  //     float tempF = (temp * 1.8) + 32;
  //     Serial.print(tempF);
  //     Serial.print(" F  \t");
  //     Serial.print(millis()/1000);

  //     for (int i = 0; i < ((tempF - 100.0)*5); i++) {
  // Serial.print(" ");
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
      showLEDsCore2 == 4 ||
      showLEDsCore2 == 6 && core1busy == false && core1request == 0) {

    if (((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3 ||
         (swirled == 1) && sendAllPathsCore2 == 0) ||
        showProbeLEDs != lastProbeLEDs) {

      // Serial.println(showLEDsCore2);

      if (showLEDsCore2 == 6) {
        showLEDsCore2 = 1;
        }
      int rails =
        showLEDsCore2; // 3 doesn't show nets and keeps control of the LEDs

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
          // Serial.print("defconDisplay = ");
          // Serial.println(defconDisplay);
          // Serial.print("probeActive = ");
          // Serial.println(probeActive);
          // core2busy = true;
          defcon(swirlCount, spread, defconDisplay);
          // core2busy = false;
          } else {

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

          showNets();

          showAllRowAnimations();

          core2busy = false;
          netUpdateRefreshCount = 0;
          }
        }

      core2busy = true;

      leds.show();

      // probeLEDs.clear();

      if (checkingButton == 0 || showProbeLEDs == 2) {
        probeLEDhandler();
        // core2busy = false;
        }
      core2busy = false;
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

              if (defconDisplay > 6) {
                defconDisplay = 0;
                }
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
            // core2busy = true;
            rotaryEncoderStuff();
            // core2busy = false;
            //  multicore_lockout_end_blocking();

            // if (probeActive == 0 && measureModeActive == 0) {
            //   if (millis() - measureLEDTimer > 50) {
            //     measureLEDTimer = millis();

            //     showLEDmeasurements();
            //   }
            // }
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
  // if (sendAllPathsCore2 != -1) {

  // Serial.println("sendPaths");
  // multicore_lockout_start_blocking();
  // multicore_lockout_start_timeout_us(1000);

  // digitalWrite(RESETPIN, HIGH);
  // delayMicroseconds(50);
  // digitalWrite(RESETPIN, LOW);
  // delayMicroseconds(2200);

  //}
  unsigned long pathTimer = micros();

  sendAllPaths();
  core2busy = false;
  // core2busy = false;
  unsigned long pathTime = micros() - pathTimer;

  // delayMicroseconds(3200);
  //  Serial.print("pathTime = ");
  //  Serial.println(pathTime);
  sendAllPathsCore2 = 0;

  // }
  }
