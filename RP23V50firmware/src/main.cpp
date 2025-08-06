// SPDX-License-Identifier: MIT

/*
Kevin Santo Cappuccio
Architeuthis Flux

KevinC@ppucc.io

5/28/2024

*/

#define PICO_RP2350A 0
// #include <pico/stdlib.h>
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
#include "tusb.h" // For tud_task() function
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

#include "Apps.h"
#include "ArduinoStuff.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "MachineCommands.h"
#include "MatrixState.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "Python.h"
#include "RotaryEncoder.h"
#include "configManager.h"
#include "oled.h"
// #define USE_FATFS 1
#ifdef USE_FATFS
#include "FatFS.h"
#include "USBfs.h"
#endif
#include "UserCode.h"
// #include "SerialWrapper.h"
#include "FilesystemStuff.h"
#include "HelpDocs.h"
#include "Highlighting.h"
#include "Python_Proper.h"
#include "USBfs.h"
#include <hardware/adc.h>
#include "JulseView.h"

// #define Serial SerialWrap
// #define USBSer1 SerialWrap
// #define USBSer2 SerialWrap

// #define Serial SerialWrap

bread b;

int supplySwitchPosition = 0;
volatile bool core1busy = false;
volatile bool core2busy = false;

// void lastNetConfirm(int forceLastNet = 0);
void rotaryEncoderStuff(void);
void initRotaryEncoder(void);
void printDirectoryContents(const char *dirname, int level);

void core2stuff(void);

volatile uint8_t pauseCore2 = 0;

volatile int loadingFile = 0;

unsigned long lastNetConfirmTimer = 0;
// int machineMode = 0;

int rotEncInit = 0;
// https://wokwi.com/projects/367384677537829889

volatile bool core2initFinished = false;

volatile bool configLoaded = false;

volatile int startupAnimationFinished = 0;

unsigned long startupTimers[10];

volatile int dumpLED = 0;
unsigned long dumpLEDTimer = 0;
unsigned long dumpLEDrate = 50;

const char firmwareVersion[] = "5.3.0.0"; // remember to update this
bool newConfigOptions = false; // set to true with new config options //!
                               // fix the saving every boot thing

julseview julseview;

void setup() {
  pinMode(RESETPIN, OUTPUT_12MA);

  digitalWrite(RESETPIN, HIGH);

  // FatFS.begin();
  if (!FatFS.begin()) {
    Serial.println("Failed to initialize FatFS");
  } else {
    Serial.println("FatFS initialized successfully");
  }

  startupTimers[0] = millis();

  loadConfig();

  // readSettingsFromConfig();
  configLoaded = 1;
  // Serial.println("Configuration loaded!");
  startupTimers[1] = millis();
  delayMicroseconds(200);

  initNets();
  backpowered = 0;

  // delay(1000);

  if (jumperlessConfig.serial_1.function >= 5 &&
      jumperlessConfig.serial_1.function <= 6) {
    dumpLED = 1;
  }
  if (jumperlessConfig.serial_2.function >= 5 &&
      jumperlessConfig.serial_2.function <= 6) {
    dumpLED = 1;
  }

  if (jumperlessConfig.serial_1.function == 4 ||
      jumperlessConfig.serial_1.function == 6) {
    jumperlessConfig.top_oled.show_in_terminal = 2;
  }
  if (jumperlessConfig.serial_2.function == 4 ||
      jumperlessConfig.serial_2.function == 6) {
    jumperlessConfig.top_oled.show_in_terminal = 3;
  }

  if (jumperlessConfig.serial_2.function != 0) {
    // Serial.begin(jumperlessConfig.serial_2.baud_rate);
    // Serial.enableUSBSer2(true);
    // USBSer2.begin(jumperlessConfig.serial_2.baud_rate);
  }

  Serial.begin(115200);

  initDAC();
  pinMode(PROBE_PIN, OUTPUT_8MA);
  pinMode(BUTTON_PIN, INPUT_PULLDOWN);
  // pinMode(buttonPin, INPUT_PULLDOWN);
  digitalWrite(PROBE_PIN, HIGH);

  routableBufferPower(1, 1);
  // digitalWrite(BUTTON_PIN, HIGH);

  startupTimers[2] = millis();
  initINA219();

  startupTimers[3] = millis();

  delayMicroseconds(100);

  digitalWrite(RESETPIN, LOW);

  while (core2initFinished == 0) {
    // delayMicroseconds(1);
  }

  initSecondSerial();

  // Auto-initialize JulseView for SUMP/libsigrok compatibility
  // Serial.println("Auto-initializing JulseView for logic analyzer...");
  // if (julseview.init()) {
  //   Serial.println("JulseView initialized successfully");
  // } else {
  //   Serial.println("JulseView initialization failed");
  // }

  drawAnimatedImage(0);
  startupAnimationFinished = 1;
  startupTimers[4] = millis();
  clearAllNTCC();

  initRotaryEncoder();
  startupTimers[5] = millis();

  delayMicroseconds(100);
  initArduino();

  // delay(100);
  initMenu();
  startupTimers[6] = millis();
  initADC();
  startupTimers[7] = millis();

  // pinMode(18, INPUT_PULLUP); //reset lines for arduino
  // pinMode(19, INPUT_PULLUP);

  // routableBufferPower(1, 0);
  // routableBufferPower(1, 1);

  getNothingTouched();
  startupTimers[8] = millis();
  createSlots(-1, 0);
  initializeNetColorTracking(); // Initialize net color tracking after slots are
                                // created
  initializeValidationTracking(); // Initialize validation tracking
  startupTimers[9] = millis();

//  setupLogicAnalyzer();
}

unsigned long startupCore2timers[10];

void setupCore2stuff() {
  // delay(2000);
  startupCore2timers[0] = millis();
  initCH446Q();
  startupCore2timers[1] = millis();
  // delay(1);
  while (configLoaded == 0) {
    delayMicroseconds(1);
  }

  initLEDs();
  startupCore2timers[2] = millis();
  initRowAnimations();
  startupCore2timers[3] = millis();
  setupSwirlColors();
  startupCore2timers[4] = millis();

  startupCore2timers[5] = millis();

  startupCore2timers[6] = millis();

  // delay(4);
}

void setup1() {
  // flash_safe_execute_core_init();

  setupCore2stuff();

  core2initFinished = 1;

  while (startupAnimationFinished == 0) {
    // delayMicroseconds(1);
    // if (Serial.available() > 0) {
    //   char c = Serial.read();
    //  // Serial.print(c);
    //   //Serial.flush();
    //   }
  }

  startupCore2timers[7] = millis();
}

char connectFromArduino = '\0';

int input = '\0';

int serSource = 0;
int readInNodesArduino = 0;

int firstLoop = 1;

volatile int probeActive = 0;

int showExtraMenu = 0;

int lastHighlightedNet = -1;
int lastBrightenedNet = -1;
int lastWarningNet = -1;

int dontShowMenu = 0;

unsigned long timer = 0;
int lastProbeButton = 0;
unsigned long waitTimer = 0;
unsigned long switchTimer = 0;
extern volatile bool flashingArduino; // Defined in ArduinoStuff.cpp
int attract = 0;

unsigned long switchPositionCheckTimer = 0;
// int switchPosition = 0;
unsigned long switchPositionCheckInterval = 500;

unsigned long mscModeRefreshTimer = 0;
unsigned long mscModeRefreshInterval = 2000;

volatile int core1passthrough = 1;

int shownMenuItems = 0;
int menuItemCount[4] = {0, 0, 0, 0};
int menuItemCounts[4] = {14, 22, 37, 46};

#include <pico/stdlib.h>


#include <hardware/gpio.h>



#define SETUP_LOGIC_ANALYZER_ON_BOOT 0


void loop() {

menu:
  if (firstLoop == 1) {

    if (firstStart == true || autoCalibrationNeeded == true) {
      if (autoCalibrationNeeded == true) {
        Serial.println("New calibration options detected in config.txt. "
                       "Running automatic calibration...");
        delay(2000);
      }
      calibrateDacs();
      firstStart = false;
    }
    firstLoop = 2;

    // Serial.println("--------------------------------");
    loadChangedNetColorsFromFile(netSlot, 0);

    // routableBufferPower(1, 1);
    if (attract == 1) {
      defconDisplay = 0;
      netSlot = -1;
    } else {

      defconDisplay = -1;
    }

    goto loadfile;
  }

  if (firstLoop == 2) {

    if (jumperlessConfig.top_oled.connect_on_boot == 1) {
      // Serial.println("Initializing OLED");
      oled.init();
    }
    

    firstLoop = 0;
#if SETUP_LOGIC_ANALYZER_ON_BOOT == 1
    goto setupla;
#endif
  }

  if (Serial.available() >
      20) { // this is so if you dump a lot of data into the serial buffer, it
            // will consume it and not keep looping
    while (Serial.available() > 0) {
      char c = Serial.read();
      // Serial.print(c);
      // Serial.flush();
    }
  }

  if (lastProbePowerDAC != probePowerDAC) {
    probePowerDACChanged = true;
    // delay(1000);
    Serial.print("probePowerDACChanged = ");
    Serial.println(probePowerDACChanged);
    routableBufferPower(1, 1);
  }

  clearHighlighting();

  if (dontShowMenu == 0) {
  forceprintmenu:

    // Serial.print("showExtraMenu = ");
    // Serial.println(showExtraMenu);
    // Serial.print("shownMenuItems = ");
    // Serial.println(shownMenuItems);

    int numberOfMenuItems = menuItemCounts[showExtraMenu];
    float steps =
        (float)highSaturationBrightColorsCount / ((float)numberOfMenuItems);
    // Serial.print("steps = ");
    // Serial.println(steps);
    shownMenuItems = 0;
    // printSpectrumOrderedColorCube();
    cycleTerminalColor(true, steps, true, &Serial);
    shownMenuItems += printMenuLine("\n\n\r\t\tMenu\n\r\n\r");
    shownMenuItems += printMenuLine("\t'help' for docs or [command]?\n\r");
    shownMenuItems += printMenuLine("\n\r");
    shownMenuItems += printMenuLine("\tm = show this menu\n\r");

    shownMenuItems += printMenuLine(showExtraMenu, 0, "\te = show extra options (%d)\n\r", showExtraMenu);

    //  Serial.println();

    shownMenuItems +=   printMenuLine(showExtraMenu, 0, "\tn = show net list\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\tb = show bridge array\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\tc = show crossbar status\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\ts = show all slot files\n\r");
    if (showExtraMenu >= 0) {
      Serial.println();
    }


    // Serial.println();
   
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\t? = show firmware version\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\t' = show startup animation\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\td = set debug flags\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tl = LED brightness / test\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t\b\b`/~ = edit / print config\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\tp = microPython REPL\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t> = send Python formatted command\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t/ = show filesystem\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t\b\bU/u = enable/disable USB Mass Storage\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\tw = enable logic analyzer\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\tX = resource status\n\r");
    // Serial.print("\tu = disable USB Mass Storage drive\n\r");
    // cycleTerminalColor();


    shownMenuItems +=   printMenuLine(showExtraMenu, 2, "\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\ty = refresh connections\n\r");
    //shownMenuItems++;
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\t< = cycle slots\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tG = reload config.txt\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\to = load node file by slot\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tP = deinitialize MicroPython (free memory)\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\tF = cycle font\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\t_ = print micros per byte\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\t@ = scan I2C (@[sda],[scl] or @[row])\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\t$ = calibrate DACs\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\t= = dump oled frame buffer\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tk = show oled in terminal\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tR = show board LEDs\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\t% = list all filesystem contents\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\tE = don't show this menu\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 3, "\tC = disable terminal colors\n\r");


    if (showExtraMenu >= 2) {

      // Serial.print("\n\r");
    }
  Serial.println();
    //shownMenuItems += printMenuLine(showExtraMenu, 1, "\n\r");
    // Serial.print("\t$ = calibrate DACs\n\r");
    if (probePowerDAC == 0) {
      shownMenuItems += printMenuLine(showExtraMenu, 3, "\t^ = set DAC 1 voltage\n\r");
    } else if (probePowerDAC == 1) {
        shownMenuItems += printMenuLine(showExtraMenu, 3, "\t^ = set DAC 0 voltage\n\r");
    }
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\tv = get ADC reading\n\r");
    // Serial.println();

    shownMenuItems += printMenuLine(showExtraMenu, 3, "\t# = print text from menu\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tg = print gpio state\n\r");
    // Serial.print("\t\b\b\b\b[0-9] = run app by index\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\t. = connect oled\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 2, "\tr = reset Arduino (rt/rb)\n\r");

    shownMenuItems += printMenuLine(showExtraMenu, 1, "\t\b\ba/A = dis/connect UART to D0/D1\n\r");

    // Serial.print("\t    print passthrough");
    // if (printSerial1Passthrough == 1) {
    //   Serial.print(" - on");
    //   } else if (printSerial1Passthrough == 2) {
    //     Serial.print(" - flashing only");
    //     } else if (printSerial1Passthrough == 3) {
    //       Serial.print(" - both");
    //       } else {
    //       Serial.print(" - off");
    //       }
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 1, "\tf = load node file\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\tx = clear all connections\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t+ = add connections\n\r");
    shownMenuItems += printMenuLine(showExtraMenu, 0, "\t- = remove connections\n\r");
    // Serial.print("\te = extra menu options\n\r");
    // Serial.println();

    Serial.println();

    Serial.flush();
  }
  if (configChanged == true && millis() > 2000) {
    // Serial.print("config changed, saving...");
    saveConfig();
    // Serial.println("\r                             \rconfig saved!\n\r");
    // Serial.flush();
    configChanged = false;
  }
  menuItemCount[showExtraMenu] = shownMenuItems;

  // for (int i = 0; i < 4; i++) {
  //   Serial.print("menuItemCount[");
  //   Serial.print(i);
  //   Serial.print("] = ");
  //   Serial.println(menuItemCount[i]);
  // }
 // Serial.flush();
  // Serial.println(millis());
dontshowmenu:

  connectFromArduino = '\0';
  firstConnection = -1;
  core1passthrough = 1;

  /// Serial.setSerialTarget( SERIAL_PORT_USBSER1);
  // SerialWrap.setSerialTarget(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 |
  // SERIAL_PORT_SERIAL1);

  // uint8_t serialTarget = SERIAL_PORT_MAIN;

  // if (jumperlessConfig.serial_1.function == 2) {
  //   Serial.println("Serial 1 is set to serial1");
  //   Serial.setSerialTarget(SERIAL_PORT_SERIAL1 | SERIAL_PORT_MAIN);
  //   } else if (jumperlessConfig.serial_2.function == 2) {
  //     Serial.println("Serial 2 is set to serial2");
  //     Serial.setSerialTarget(SERIAL_PORT_SERIAL2 | SERIAL_PORT_MAIN);
  //   } else {
  //     Serial.println("Serial is set to main");
  //     Serial.setSerialTarget(SERIAL_PORT_MAIN);
  //     }

  // Serial.setSerialTarget(serialTarget);

  //! This is the main busy wait loop waiting for input
  while (Serial.available() == 0 && connectFromArduino == '\0' &&
         slotChanged == 0) {

    unsigned long busyTimer = millis();

    if (julseview.getShouldStopOtherStuff() == true) {
      continue;
    }


    
    int encoderNetHighlighted = encoderNetHighlight();
    if (encoderNetHighlighted != -1) {
      firstConnection = encoderNetHighlighted;

    } else {
      // firstConnection = -1;
    }

    checkPads();

    secondSerialHandler();

    // Handle USB tasks (required for MSC and other USB interfaces)
    // #ifdef USE_TINYUSB
    tud_task();
    // #endif
    //  //core1passthrough = 0;

    if (clickMenu() >= 0) {
      // defconDisplay = -1;
      core1passthrough = 0;
      goto loadfile;
    }

    //     // **NORMAL MODE**: Single call when not in connection mode
    // handleLogicAnalyzer();
    //}

    int probeReading = justReadProbe(true);

    checkForReadingChanges();

    warnNetTimeout(1);
    if (probeReading > 0) {
      // Serial.print("probeReading = ");
      // Serial.println(probeReading);
      if (highlightNets(probeReading) != -1) {

        firstConnection = probeReading;
      }
    }

    if (brightenedNet > 0) {
      int probeToggleResult = probeToggle();
      if (probeToggleResult != -1) {
        // Serial.print("probeToggleResult = ");
        // Serial.println(probeToggleResult);
        // Serial.flush();
      }
      if (probeToggleResult >= 0 && brightenedNet > 0) {

        blockProbeButton = gpioToggleFrequency;
        blockProbeButtonTimer = millis();

      } else if (probeToggleResult == -5) {
        // Serial.print("probeToggleResult = ");
        // Serial.println(probeToggleResult);
        // Serial.flush();
        // firstConnection = 5;//probeReading;
        if (firstConnection > 0) {
          if (warningNet == brightenedNet && warningTimeout > 0) {
            // warningNet = -1;
            // brightenedNet = 0;
            warningTimeout = 0;
            // warningTimer = 0;
            connectOrClearProbe = 0;
            showProbeLEDs = 2;
            probeActive = 1;
            input = '{';
            probingTimer = millis();
            startupTimers[0] = millis();
            // Serial.println("probing\n\r");

            goto skipinput;
            // warnNet(-1);
          } else {
            warnNet(firstConnection);
            warningTimeout = 3800;
            warningTimer = millis();
          }
        }

        blockProbeButton = 800;
        blockProbeButtonTimer = millis();
      } else if (probeToggleResult == -3) {
        blockProbeButton = 800;
        blockProbeButtonTimer = millis();

      } else if (probeToggleResult == -2) {
        blockProbeButton = 800;
        blockProbeButtonTimer = millis();

      } else if (probeToggleResult == -4) {
        // warnNet(firstConnection);
        // assignNetColors();
        // showLEDsCore2 = 1;

        // Serial.print("-4 warningNet = ");
        // Serial.println(warningNet);
        // Serial.flush();
        // clearHighlighting();

        firstConnection = -1;
        blockProbeButton = 500;
        blockProbeButtonTimer = millis();
      }
    } else {
      firstConnection = -1;
    }

    if ((millis() - waitTimer) > 12) {
      waitTimer = millis();

      int probeButton = checkProbeButton();

      if (probeButton != lastProbeButton) {

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
            input = '}';
            probingTimer = millis();
            brightenedNet = 0;
            core1passthrough = 0;
            goto skipinput;
          } else if (probeButton == 1) {
            // getNothingTouched();
            startupTimers[0] = millis();
            connectOrClearProbe = 0;
            showProbeLEDs = 2;
            probeActive = 1;
            input = '{';
            probingTimer = millis();
            // Serial.println("probing\n\r");
            brightenedNet = 0;
            core1passthrough = 0;
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

    if (lastHighlightedNet != highlightedNet) {
      // Serial.print("\n\rhighlightedNet = ");
      // Serial.println(highlightedNet);
      // Serial.print("brightenedNet = ");
      // Serial.println(brightenedNet);
      // Serial.print("warningNet = ");
      // Serial.println(warningNet);
      // Serial.flush();
      lastHighlightedNet = highlightedNet;
    } else if (lastBrightenedNet != brightenedNet) {
      // Serial.print("\n\rhighlightedNet = ");
      // Serial.println(highlightedNet);
      // Serial.print("brightenedNet = ");
      // Serial.println(brightenedNet);
      // Serial.print("warningNet = ");
      // Serial.println(warningNet);
      // Serial.flush();
      lastBrightenedNet = brightenedNet;
    } else if (lastWarningNet != warningNet) {
      // Serial.print("\n\rhighlightedNet = ");
      // Serial.println(highlightedNet);
      // Serial.print("brightenedNet = ");
      // Serial.println(brightenedNet);
      // Serial.print("warningNet = ");
      // Serial.println(warningNet);
      // Serial.flush();
      lastWarningNet = warningNet;
    }

    // Serial.println(showReadings);
    if (showReadings >= 1) {
      // chooseShownReadings();
      showMeasurements(16, 0, 0);
    }

    if (mscModeEnabled == true) {
      if (millis() - mscModeRefreshTimer > mscModeRefreshInterval) {
        mscModeRefreshTimer = millis();
        // refreshUSBFilesystem();
        // refreshConnections(-1);
        // Serial.println("Periodic filesystem refresh completed");
        //   refreshUSBFilesystem();
      }
    }

    if (millis() - oled.lastConnectionCheck > oled.connectionCheckInterval &&
        jumperlessConfig.top_oled.enabled == 1) {
      // Serial.println("checking oled connection");
      // Serial.println(oled.checkConnection());
      oled.lastConnectionCheck = millis();
      if (checkIfBridgeExists(jumperlessConfig.top_oled.sda_row,
                              jumperlessConfig.top_oled.gpio_sda) == true) {
        if (checkIfBridgeExists(jumperlessConfig.top_oled.scl_row,
                                jumperlessConfig.top_oled.gpio_scl) == true) {

          if (oled.checkConnection() == false) {
            //Serial.print("\r                                             "
                    //     "\roled connection lost, retrying...");
            oled.oledConnected = false;
            // oled.disconnect();
            // jumperlessConfig.top_oled.enabled = 0;

            // if (jumperlessConfig.top_oled.lock_connection == 1 &&
            if (oled.connectionRetries < oled.maxConnectionRetries) {
              // oled.connectionRetries++;
              //  if (oled.connectionRetries > oled.maxConnectionRetries) {
              //    oled.connectionRetries = 0;
              // Serial.println("retrying oled connection");
              if (oled.init() != 0) {
                Serial.print("\r                                          \r");
                Serial.flush();
              }
            }
          }

          oled.connectionRetries = 0;
        }
      }
    }

    if (millis() - switchPositionCheckTimer > switchPositionCheckInterval) {
      switchPositionCheckTimer = millis();
      switchPosition = checkSwitchPosition();
      // Serial.print("switchPosition = ");
      // Serial.println(switchPosition);
      // Serial.flush();
    }

    // Serial.print("busyTimer = ");
    // Serial.println(millis() - busyTimer);
  }


  input = Serial.read();

  timer = millis();
  // Serial.print("input = ");
  // Serial.println(input);
  // Serial.flush();

  // Handle multi-character help commands
  if (input == 'h') {
    // Check if next character is available (for "help" command)
    unsigned long helpTimer = millis();
    while (Serial.available() == 0 && millis() - helpTimer < 100) {
      // Small timeout for typing "help"
    }
    if (Serial.available() > 0) {
      String helpString = "h";
      while (Serial.available() > 0 && helpString.length() < 50) {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
          break;
        helpString += c;
      }
      if (helpString == "help") {
        showGeneralHelp();
        goto dontshowmenu;
      } else if (helpString.startsWith("help ")) {
        String category = helpString.substring(5);
        category.trim();
        showCategoryHelp(category.c_str());
        goto dontshowmenu;
      }
    } else {
      // Just 'h' alone, let it fall through to normal processing
    }
  }

  // Handle command? help requests
  if (input != '\n' && input != '\r' && input != ' ') {
    // Check if next character is '?' for command-specific help
    unsigned long helpTimer = millis();
    while (Serial.available() == 0 && millis() - helpTimer < 100) {
      // Small timeout for typing command?
    }
    if (Serial.available() > 0) {
      char nextChar = Serial.peek();
      if (nextChar == '?') {
        Serial.read(); // consume the '?'
        showCommandHelp(input);
        goto dontshowmenu;
      }
    }
  }

  if (input == ' ' || input == '\n' || input == '\r') {

    // Serial.print(input);
    // Serial.flush();
    goto dontshowmenu;
  }
skipinput:



  switch (input) {


    case 'w': //! w - Setup logic analyzer
    {
      // int tempReading = adc_read_blocking(8);
setupla:
      // for (int i = 0; i < 8; i++) {
      //   adc_select_input(8);
      //   int tempReading = adc_read();
      //   Serial.print(tempReading);
      //   Serial.println(" ");
      // }
      julseview.init();

      //Serial.println();
      //setupLogicAnalyzer();
      // DON'T call init() again - JulseView is already initialized during boot
      // Calling init() multiple times can cause resource conflicts with CH446Q
      // julseview.init();
     // Serial.println("Logic analyzer is ready (already initialized during boot)");

      // for (int i = 0; i < 8; i++) {
      //   adc_select_input(8);
      //   int tempReading = adc_read();
      //   Serial.print(tempReading);
      //   Serial.println(" ");
      // }
      break;
    }

  case 'X': { //! X - Resource Status
    Serial.println("Resource Allocation Status:");
    Serial.println("==========================");
    
    // Check logic analyzer status
    Serial.println("✗ Logic Analyzer: Not available (removed)");
    
    // Check rotary encoder status
    if (isRotaryEncoderInitialized()) {
      Serial.println("✓ Rotary Encoder: Initialized");
      printRotaryEncoderStatus();
    } else {
      Serial.println("✗ Rotary Encoder: Not initialized");
    }
    
    // Check for conflicts
    Serial.println("\nConflict Detection:");
    Serial.println("Logic Analyzer conflicts: N/A (removed)");
    
    Serial.println();
    break;
  }

  case 'G': { //! G - Load config.txt changes
    Serial.println("Reloading config.txt...");
    configChanged = true;

    /// loadConfigChanges();
    // goto dontshowmenu;
    break;
  }
  case 'S': { //! S - raw speed test
    Serial.println("Raw speed test...");
    Serial.println("Read frequency on row 29\n\n\r");

    pauseCore2 = true;
    unsigned long cycles = 1000000;
    unsigned long start = micros();
    sendXYraw(10, 0, 4, 1);
    for (int i = 0; i < cycles; i++) {
      sendXYraw(10, 0, 0, 1);
      //delayMicroseconds(1);
      sendXYraw(10, 0,0,0);
      //delayMicroseconds(1);
    }
    unsigned long end = micros();
    Serial.print("Time for ");
    Serial.print(cycles);
    Serial.print(" on off cycles: ");
    Serial.print(end - start);
    Serial.println(" microseconds");
    Serial.print("Time per cycle: ");
    Serial.print((end - start) / cycles);
    Serial.println(" microseconds");
    Serial.print("Frequency: ");
    Serial.print(((float)cycles / (float)(end - start)) * 1000);
    Serial.println(" kHz\n\r");
    Serial.flush();
    pauseCore2 = false;





    break;
  }

  case 'j':

    for (int i = 0; i < highSaturationSpectrumColorsCount; i++) {
      changeTerminalColorHighSat(i, true, &Serial, 0);
      Serial.print(i);
      Serial.print(": ");
      if (i < 10) {
        Serial.print(" ");
      }
      Serial.print(highSaturationSpectrumColors[i]);

      Serial.print("\t\t");
      if (i < highSaturationBrightColorsCount) {
        changeTerminalColorHighSat(i, true, &Serial, 1);
        Serial.print(i);
        Serial.print(": ");
        if (i < 10) {
          Serial.print(" ");
        }
        Serial.print(highSaturationBrightColors[i]);
      }
      Serial.println();
    }

    goto dontshowmenu;
    break;

  case 'U': { //! U - Enable USB Mass Storage drive\n

    if (mscModeEnabled == false) {
      Serial.println("Enabling USB Mass Storage drive...");
      if (initUSBMassStorage()) {
        Serial.println("USB Mass Storage enabled - device will appear as "
                       "'JUMPERLESS' drive\n\r");
        Serial.println("\tu = disable USB Mass Storage");
        Serial.println("\tG = reload config.txt");
        Serial.println("\ty = refresh connections when files change");
        Serial.println("\tS = show status");
        Serial.println("\n\r");
        Serial.flush();
        delay(3000);
      } else {
        Serial.println("USB Mass Storage initialization failed");
        Serial.flush();
      }
    } else {
      Serial.println("USB Mass Storage is already enabled");
      printUSBMassStorageStatus();
      refreshConnections(-1);
      Serial.flush();
    }

    delay(3000);
    unsigned long mscModeTimer = millis();
    while (mscModeEnabled == true) {
      while (Serial.available() == 0) {
        // if (millis() - mscModeTimer > 3000) {
        //   manualRefreshFromUSB();
        //   refreshConnections(-1);
        //   mscModeTimer = millis();
        // }
      }
      if (Serial.available() > 0) {
        char c = Serial.read();
        if (c == 'u') {
          Serial.println("Disabling USB Mass Storage");
          disableUSBMassStorage();
          mscModeEnabled = false;
          Serial.flush();
          refreshConnections(-1, 1, 1);
        }
        if (c == 'U') {
          Serial.println("Enabling USB Mass Storage");
          initUSBMassStorage();
          printUSBMassStorageStatus();
          Serial.flush();
        }
        if (c == 'y' || c == 'Y') {
          Serial.println("Refreshing connections");
          manualRefreshFromUSB();
          delay(100);
          refreshConnections(-1);

          Serial.flush();
        }
        if (c == 'G' || c == 'g') {
          Serial.println("Reloading config.txt");

          Serial.flush();
          manualRefreshFromUSB();
          delay(100);
          loadConfig();
          Serial.flush();
        }
        if (c == 's' || c == 'S') {
          Serial.println("Showing status");
          printUSBMassStorageStatus();
          Serial.flush();
        }
      }
    }
    //  Serial.println("\n╭─────────────────────────────────────╮");
    //    Serial.println("│       USB Mass Storage Control      │");
    //    Serial.println("╰─────────────────────────────────────╯");
    //    printUSBMassStorageStatus();
    //    Serial.println("Usage:");
    //    Serial.println("  • Device appears as mass storage automatically");
    //    Serial.println("  • No special mode needed (CircuitPython-style)");
    //    Serial.println("  • Device remains responsive during file access");
    //    Serial.println("  • Use 'U' command to check status anytime");
    //    if (isUSBMassStorageMounted()) {
    //      Serial.println("Currently mounted by host - ready for file access");
    //    } else if (isUSBMassStorageEjected()) {
    //      Serial.println("Device was ejected by host");
    //    } else {
    //      Serial.println("Waiting for host to mount device");
    //    }
    //    Serial.flush();
    goto dontshowmenu;
    break;
  }

  case 'Z': { //! Z - Toggle USB debug mode
    Serial.println("╭─────────────────────────────────╮");
    Serial.println("│        USB Debug Control        │");
    Serial.println("├─────────────────────────────────┤");
    Serial.println("│ 1. Toggle USB debug mode        │");
    Serial.println("│ 2. Manual refresh from USB      │");
    Serial.println("│ 3. Validate all slots           │");
    Serial.println("│ Any other key - Cancel          │");
    Serial.println("╰─────────────────────────────────╯");
    Serial.print("Choose option: ");
    Serial.flush();

    // Wait for input
    while (Serial.available() == 0) {
      delay(1);
    }
    char choice = Serial.read();
    Serial.println(choice);

    switch (choice) {
    case '1':
      Serial.println("\nToggling USB debug mode...");
      setUSBDebug(!usb_debug_enabled);
      break;
    case '2':
      if (isUSBMassStorageMounted()) {
        Serial.println("\nPerforming manual refresh from USB...");
        manualRefreshFromUSB();
      } else {
        Serial.println("\nUSB drive not mounted");
      }
      break;
    case '3':
      Serial.println("\nValidating all slot files...");
      // validateAllSlots(true);
      break;
    default:
      Serial.println("\nCancelled");
      break;
    }

    Serial.flush();
    goto dontshowmenu;
    break;
  }

  case 'u': { //! u - Disable USB Mass Storage drive
    if (mscModeEnabled == true) {
      Serial.println("Disabling USB Mass Storage drive...");
      if (disableUSBMassStorage()) {
        Serial.println(
            "USB Mass Storage disabled - device no longer appears as drive");
        Serial.println("Use 'U' command to re-enable when needed");
      } else {
        Serial.println("USB Mass Storage disable failed");
      }
    } else {
      Serial.println("USB Mass Storage is already disabled");
      Serial.println("Use 'U' command to enable");
    }

    goto dontshowmenu;
    break;
  }

  case 'D': { //! D - Run USB MSC diagnostic test
    // Serial.println("Running USB Mass Storage diagnostic test...");
    // Serial.flush();

    // quickUSBMSCDiagnostic();

    // Serial.println("\nDiagnostic complete. Press any key to continue...");
    // Serial.flush();
    // while (Serial.available() == 0) {
    //   delay(10);
    // }
    // // Clear the input
    // while (Serial.available() > 0) {
    //   Serial.read();
    // }
    // goto dontshowmenu;
    // printUSBDeviceInfo();            // Shows updated port detection info
    // testSUMPProtocol();            // Tests SUMP protocol on USBSer2
    // testUSBSer1Alternative();      // Tests Bus Pirate-style CDC Interface 1
    // printLogicAnalyzerConflictDiagnosis();  // Shows conflicts
    break;
  }

  case '/': { //!  /

    runApp(-1, (char *)"File Manager");
    Serial.write(0x0F);
    Serial.flush();
    break;
  }

  case 'C': { //!  C
    disableTerminalColors = !disableTerminalColors;
    if (disableTerminalColors) {
      Serial.println("Terminal colors disabled");
    } else {
      Serial.println("Terminal colors enabled");
    }
    Serial.flush();
    // goto dontshowmenu;
    break;
  }
  case 'E': { //!  E
    if (dontShowMenu == 0) {
      dontShowMenu = 1;
    } else {
      dontShowMenu = 0;
    }
    break;
  }
  case 'k': { //!  k
    // for (int i = 0; i < 255; i++) {
    //   Serial.print(i);
    //   Serial.print(": ");
    //   char* name = colorToName(i, -1);
    //   Serial.println(name);
    //   }

    // Call the demo function directly - it will check for range input itself
    // Serial.println("Displaying color names (enter range like '10-200' for
    // specific range)"); colorPicker(0, 255);
    if (jumperlessConfig.top_oled.show_in_terminal > 0) {
      jumperlessConfig.top_oled.show_in_terminal = 1;
    } else {
      jumperlessConfig.top_oled.show_in_terminal = 0;
    }
    configChanged = true;
    break;
  }

  case 0x10: { //! DLE
    input = '\0';
    dumpLED = 0;
    goto dontshowmenu;
  }
  case 'R': { //!  R
              // printWireStatus();

    // for (int i = 0; i < 10; i++) {
    if (dumpLED == 1) {
      dumpLED = 0;
    } else {
      dumpLED = 1;
    }
    // }
    // printSerial1stuff();
    // printAllRLEimageData();
    goto dontshowmenu;
    break;
  }

    // Add this case for single Python command
  case '>': { //! > - Execute single Python command
    // readPythonCommand();
    getMicroPythonCommandFromStream();
    Serial.flush();
    goto dontshowmenu;
    break;
  }

  // Modify the existing P case for Python command mode
  case 'P': { //! P - Deinitialize MicroPython to free memory
    Serial.println(
        "Deinitializing MicroPython to free memory... Total memory: " +
        String(rp2040.getTotalHeap()));
    Serial.println("Free memory: " + String(rp2040.getFreeHeap()));
    deinitMicroPythonProper();
    Serial.println("MicroPython deinitialized. Memory freed.");

    Serial.println("Total memory: " + String(rp2040.getTotalHeap()));
    Serial.println("Free memory: " + String(rp2040.getFreeHeap()));
    Serial.println("Use 'p' to reinitialize and enter REPL again.");
    goto dontshowmenu;
    break;
  }

  case 'p': { //!  p
    // micropythonREPL();
    //  Serial.println("Entering MicroPython REPL");
    //  Serial.println("choose a stream: ");
    //  Serial.println("1 = Port 1 (this)");
    //  Serial.println("2 = Port 2");
    //  Serial.println("3 = Port 3");
    //  Serial.println("4 = Port 4");

    // // testStreamRedirection(&USBSer1);
    // int streamChoice = 1;
    // // while (streamChoice == -1) {
    //   if (Serial.available() > 0) {
    //   streamChoice = Serial.parseInt();
    //   if (streamChoice == 1) {
    //     setGlobalStream(&Serial);
    //   } else if (streamChoice == 2) {
    //     setGlobalStream(&USBSer1);
    //   } else if (streamChoice == 3  ) {
    //     setGlobalStream(&USBSer2);
    //   } else if (streamChoice == 4) {
    //     #ifdef USBSer3
    //     setGlobalStream(&USBSer3);
    //     #endif
    //   }
    // }
    // Serial.println("Using stream: " + String(streamChoice));
    enterMicroPythonREPL();

    refreshConnections(-1, 1, 1);
    Serial.write(0x0F);
    Serial.flush();
    // printAllConnectableNodes();
    break;
  }
  case '.': { //!  .
    // initOLED();
    if (jumperlessConfig.top_oled.enabled == 0) {
      Serial.println("oled enabled");
      oled.init();
      jumperlessConfig.top_oled.enabled = 1;
      configChanged = true;
    } else {
      oled.disconnect();
      jumperlessConfig.top_oled.enabled = 0;
      oled.oledConnected = false;

      configChanged = true;
      Serial.println("oled disconnected");
    }

    // oled.print("FUCK");
    // oled.show();
    // oled.test();
    goto dontshowmenu;
    break;
  }

  case 'c': { //!  c
    printChipStateArray();
    goto dontshowmenu;
    break;
  }

  case '_': { //!  _
    printMicrosPerByte();
    goto dontshowmenu;
    break;
  }

  case 'g': { //!  g
    printGPIOState();
    break;
  }
  case '&': { //!  &
    loadChangedNetColorsFromFile(netSlot, 0);
    goto dontshowmenu;
    break;
    int node1 = -1;
    int node2 = -1;
    while (Serial.available() == 0) {
    }
    // char c = Serial.read();
    node1 = Serial.parseInt();
    node2 = Serial.parseInt();
    Serial.print("node1 = ");
    Serial.println(node1);
    Serial.print("node2 = ");
    Serial.println(node2);
    Serial.print("checkIfBridgeExistsLocal(node1, node2) = ");
    long unsigned int timer = micros();
    Serial.println(checkIfBridgeExistsLocal(node1, node2));
    Serial.print("time taken = ");
    Serial.print(micros() - timer);
    Serial.println(" microseconds");

    Serial.flush();

    // if (node2 == -1 && node1 > 0) {
    //   Serial.println(checkIfBridgeExists(node1,node2));
    //   }

    break;
  }

  case '\'': { //!  '
    pauseCore2 = 1;
    delay(1);
    drawAnimatedImage(0);
    pauseCore2 = 0;
    goto dontshowmenu;
    break;
  }
  case 'x': { //!  x
    digitalWrite(RESETPIN, HIGH);
    delay(1);
    refreshPaths();
    clearAllNTCC();
    // oled.oledConnected = false;

    clearNodeFile(netSlot, 0);
    refreshConnections(-1, 1, 1);
    digitalWrite(RESETPIN, LOW);

    Serial.println("Cleared all connections");

    goto dontshowmenu;

    break;
  }

  case '+': { //!  +

    readStringFromSerial(0, 0);
    goto loadfile;

    break;
  }

  case '-': { //!  -
    readStringFromSerial(0, 1);
    goto loadfile;
    break;
  }

  case '~': { //!  ~
    core1busy = 1;
    waitCore2();
    printConfigToSerial();
    core1busy = 0;
    Serial.flush();
    goto dontshowmenu;
    break;
  }
  case '`': { //!  `
    core1busy = 1;
    waitCore2();
    readConfigFromSerial();
    core1busy = 0;
    Serial.flush();
    goto dontshowmenu;
    break;
  }
    // case '2': {
    // runApp(2);
    // break;
    // }

  case '^': { //!  ^
    // doomOn = 1;
    // Serial.println(yesNoMenu());
    // break;
    char f[8] = {' '};
    int index = 0;
    float f1 = 0.0;
    unsigned long timer = millis();
    while (Serial.available() == 0 && millis() - timer < 1000) {
    }
    while (index < 8) {
      f[index] = Serial.read();
      index++;
    }

    f1 = atof(f);
    // Serial.print("f = ");
    // Serial.println(f1);
    if (probePowerDAC == 1) {
      setDac0voltage(f1, 1, 1);
    } else if (probePowerDAC == 0) {
      setDac1voltage(f1, 1, 1);
    }
    configChanged = true;
    Serial.printf("DAC %d = %0.2f V\n", !probePowerDAC, f1);
    Serial.flush();
    // playDoom();
    // doomOn = 0;
    goto dontshowmenu;
    break;
  }

  case '?': { //!  ?
    Serial.print("Jumperless firmware version: ");
    Serial.println(firmwareVersion);
    Serial.flush();
    goto dontshowmenu;
    break;
  }
  case '@': { //!  @
    Serial.flush();

    if (Serial.available() > 0) {
      String input = Serial.readString();
      input.trim(); // Remove whitespace

      if (input.indexOf(',') != -1) {
        // Format: @5,10 - SDA at row 5, SCL at row 10
        int commaIndex = input.indexOf(',');
        int sdaRow = input.substring(0, commaIndex).toInt();
        int sclRow = input.substring(commaIndex + 1).toInt();

        changeTerminalColor(69, true);
        Serial.print("I2C scan with SDA=");
        Serial.print(sdaRow);
        Serial.print(", SCL=");
        Serial.println(sclRow);
        changeTerminalColor(38, true);

        if (i2cScan(sdaRow, sclRow, 26, 27, 1) > 0) {
          Serial.println("Found devices");
          return;
        } else {
          removeBridgeFromNodeFile(RP_GPIO_26, sdaRow, netSlot, 0);
          removeBridgeFromNodeFile(RP_GPIO_27, sclRow, netSlot, 0);
          refreshConnections(-1, 1);
        }
      } else if (input.length() > 0 && isdigit(input[0])) {
        // Format: @5 - try all 4 combinations around row 5
        int baseRow = input.toInt();

        changeTerminalColor(69, true);
        Serial.print("I2C scan trying all combinations around row ");
        Serial.println(baseRow);
        changeTerminalColor(38, true);

        // Try all 4 combinations: SDA=base SCL=base+1, SDA=base+1 SCL=base,
        // SDA=base SCL=base-1, SDA=base-1 SCL=base
        int combinations[4][2] = {
            {baseRow, baseRow + 1}, // SDA=base, SCL=base+1
            {baseRow + 1, baseRow}, // SDA=base+1, SCL=base
            {baseRow, baseRow - 1}, // SDA=base, SCL=base-1
            {baseRow - 1, baseRow}  // SDA=base-1, SCL=base
        };

        for (int i = 0; i < 4; i++) {
          int sdaRow = combinations[i][0];
          int sclRow = combinations[i][1];

          // // Skip invalid row numbers (must be 1-60)
          // if (sdaRow < 1 || sdaRow > 60 || sclRow < 1 || sclRow > 60) {
          //   continue;
          // }

          changeTerminalColor(202, true);
          Serial.print("\nTrying SDA=");
          Serial.print(sdaRow);
          Serial.print(", SCL=");
          Serial.print(sclRow);
          Serial.println(":");
          changeTerminalColor(38, true);
          int devicesFound = i2cScan(sdaRow, sclRow, 26, 27, 0);
          if (devicesFound > 0) {
            changeTerminalColor(199, true);
            Serial.printf(
                "\n\rfound %d devices: SDA at row %d, SCL at row %d\n\r",
                devicesFound, sdaRow, sclRow);
            changeTerminalColor(-1);
            return;
          }
          // Serial.println("Found devices");
          // } else {
          //   removeBridgeFromNodeFile(sdaRow, -1, netSlot, 0);
          //   removeBridgeFromNodeFile(sclRow, -1, netSlot, 0);
          //   refreshConnections(-1, 1);
          // }
          delay(1); // Small delay between scans
        }
      } else {
        // // Legacy format for '1' or '2' options
        // if(input == "1") {
        //   i2cScan(1, 2, 26, 27, 1);
        // } else if(input == "2") {
        //   i2cScan(1, 2, 26, 27, 0);
        // } else {
        //   changeTerminalColor(202, true);
        //   Serial.println("Invalid format. Use @5,10 for specific pins or @5
        //   for auto-try"); changeTerminalColor(38, true);
        // }
      }
    } else {
      // Interactive mode - prompt for SDA and SCL
      Serial.print("Enter SDA row: ");
      Serial.flush();
      while (Serial.available() == 0) {
      }
      int rowSDA = Serial.parseInt();
      Serial.print("Enter SCL row: ");
      Serial.flush();
      while (Serial.available() == 0) {
      }
      int rowSCL = Serial.parseInt();

      changeTerminalColor(69, true);
      Serial.print("I2C scan with SDA=");
      Serial.print(rowSDA);
      Serial.print(", SCL=");
      Serial.println(rowSCL);
      changeTerminalColor(38, true);

      if (i2cScan(rowSDA, rowSCL, 26, 27, 1) > 0) {
        // Serial.println("Found devices");
      } else {
        removeBridgeFromNodeFile(RP_GPIO_26, rowSDA, netSlot, 0);
        removeBridgeFromNodeFile(RP_GPIO_27, rowSCL, netSlot, 0);
        refreshConnections(-1, 1);
      }
    }

    goto dontshowmenu;
    break;
  }
  case '$': { //!  $
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
  case 'r': { //!  r
    if (Serial.available() > 0) {
      char c = Serial.read();
      if (c == '0' || c == '2' || c == 't') {
        resetArduino(0);
      }
      if (c == '1' || c == '2' || c == 'b') {
        resetArduino(1);
      }
    } else {
      resetArduino();
    }
    goto dontshowmenu;
    break;
  }

  case 'A': { //!  A
    // delay(100);
    int justAsk = 0;
    if (Serial.available() > 0) {
      // Serial.print("checking for arduino connection");
      char c = Serial.read();
      // if (c == ' ') {
      //   continue;
      //   }
      if (c == '?') {
        if (checkIfArduinoIsConnected() == 1) {
          justAsk = 1;
          Serial.println("Y");
          Serial.flush();
          // break;
        } else {
          justAsk = 1;
          Serial.println("n");
          Serial.flush();
          // break;
        }
      } else {
        // break;
      }
    }
    if (justAsk == 0) {
      connectArduino(0);
      Serial.println("UART connected to Arduino D0 and D1");
      Serial.flush();
      //   removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, 0);
      //   removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, 0);
      //   addBridgeToNodeFile(RP_UART_RX, NANO_D1, netSlot, 0, 1);
      //   addBridgeToNodeFile(RP_UART_TX, NANO_D0, netSlot, 0, 1);
      //   //ManualArduinoReset = true;
      //  // goto loadfile;
      //   refreshConnections(-1);
    }
    goto dontshowmenu;
    break;
  }
  case 'a': { //!  a
    // delay(100);
    int justAsk = 0;
    while (Serial.available() > 0) {
      // Serial.print("checking for arduino connection");
      char c = Serial.read();
      // if (c == ' ') {
      //   continue;
      //   }
      if (c == '?') {
        if (checkIfArduinoIsConnected() == 1) {
          justAsk = 1;
          Serial.println("Y");
          Serial.flush();
          // break;
        } else {
          justAsk = 1;
          Serial.println("n");
          Serial.flush();
          // break;
        }
      } else {
        // break;
      }
    }
    if (justAsk == 0) {
      disconnectArduino(0);
      Serial.println("UART disconnected from Arduino D0 and D1");
      Serial.flush();
      // removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, 0);
      // removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, 0);
      // // removeBridgeFromNodeFile(RP_UART_RX, -1, netSlot, 0);
      // // removeBridgeFromNodeFile(RP_UART_TX, -1, netSlot, 0);
      // //refreshLocalConnections(-1);
      // refreshConnections(-1);
    }
    // goto loadfile;
    goto dontshowmenu;
    break;
  }

  case 'F': //!  F
    oled.cycleFont();
    break;

    // case '%': { //!  %
    //   // Print entire filesystem
    //   Serial.println("\n\rFilesystem Contents:");
    //   Serial.println("====================");
    //   printDirectoryContents("/", 0);
    //   Serial.println("====================");
    //   break;
    // }

  case '=': { //!  =
    //  while (SerialWrap.available() == 0) {
    //  }
    //  char o = SerialWrap.read();
    // oled.testCharBounds("Fuck", 2);

    Serial.println("\n\r");
    // oled.debugJokermanBaseline();
    oled.dumpFrameBuffer();
    // oled.testMenuPositioning();
    // oled.dumpFrameBuffer();
    //  if (isDigit(o)) {
    //  if (o == '0') {
    //   oled.setFont(0);
    //   } else if (o == '1') {
    //     oled.setFont(1);
    //     }
    // else if (o == '2') {
    //   oled.setFont(2);
    //   }
    // else if (o == '3') {
    //   oled.setFont(3);
    //   }
    // } else {
    //   oled.clear();

    // }

    goto dontshowmenu;
    break;
  }

  case 'i': { //!  i
    if (oled.isConnected() == false) {
      if (oled.init() == false) {
        Serial.println("Failed to initialize OLED");
        break;
      }
    }

    // oledTest(NANO_D2, NANO_D3, 22, 23);

    break;
  }

  case '#': { //!  #
    // pauseCore2 = 1;
    //  while (slotChanged == 0)
    //  {
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
  case 'e': { //!  e
    showExtraMenu++;
    if (showExtraMenu > 3) {
      showExtraMenu = 0;
    }
    break;
  }

  case 's': { //!  s
    printSlots(-1);

    break;
  }
  case 'v': { //!  v
    if (Serial.available() > 0) {
      char c = Serial.read();

      if (isdigit(c) == 1) {
        int adc = c - '0';
        if (adc >= 0 && adc <= 4) {
          Serial.print(" adc");
          Serial.print(adc);
          Serial.print(" = ");
          float adcVoltage = readAdcVoltage(adc, 32);
          if (adcVoltage > 0.00) {
            Serial.print(" ");
          }
          Serial.println(adcVoltage);
        } else if (c == 'p') {
          Serial.print(" probe = ");
          float probeVoltage = readAdcVoltage(7, 32);
          if (probeVoltage > 0.00) {
            Serial.print(" ");
          }
          Serial.println(probeVoltage);
        }
      } else if (c == 'i') {
        if (Serial.available() > 0) {
          char c = Serial.read();
          if (c == '1') {
            float iSense = INA1.getCurrent_mA();
            Serial.print("ina1 = ");
            Serial.print(iSense);
            Serial.println("mA");
          }
        } else {
          float iSense = INA0.getCurrent_mA();
          Serial.print("ina0 = ");
          Serial.print(iSense);
          Serial.print("mA \t");

          iSense = INA0.getBusVoltage();
          Serial.print(iSense);
          Serial.print("V \t");

          iSense = INA0.getPower_mW();
          Serial.print(iSense);
          Serial.println("mW");
        }
      } else if (c == 'l') {

        if (showReadings == 1) {
          showReadings = 0;
          Serial.println("showReadings = 0");
        } else {
          showReadings = 1;
          Serial.println("showReadings = 1");
        }
        chooseShownReadings();
      }
      Serial.flush();
    } else {
      Serial.println();
      for (int i = 0; i < 5; i++) {
        Serial.print("adc");
        Serial.print(i);
        Serial.print(" = ");
        float adcVoltage = readAdcVoltage(i, 32);
        if (adcVoltage > 0.00) {
          Serial.print(" ");
        }
        Serial.println(adcVoltage);
      }
      Serial.print("probe = ");
      float probeVoltage = readAdcVoltage(7, 32);
      if (probeVoltage > 0.00) {
        Serial.print(" ");
      }
      Serial.println(probeVoltage);
    }
    Serial.flush();
    goto dontshowmenu;
    break;

    if (showReadings >= 3 || (inaConnected == 0 && showReadings >= 1)) {
      showReadings = 0;
      break;
    } else {
      showReadings++;

      chooseShownReadings();
      // Serial.println(showReadings);

      goto dontshowmenu;
      break;
    }
  }
  case '}': {
    // probeActive = 1;
    //   Serial.print("pdebugLEDs = ");
    //  Serial.println(debugLEDs);
    /// delayMicroseconds(5);
    //  Serial.print("firstConnection = ");
    //  Serial.println(firstConnection);
    //  Serial.flush();
    blockProbeButton = 300;
    blockProbeButtonTimer = millis();
    probeMode(1, firstConnection);
    //      Serial.print("apdebugLEDs = ");
    // Serial.println(debugLEDs);
    // delayMicroseconds(5);
    probeActive = 0;

    clearHighlighting();
    // clearLEDs();
    // assignNetColors();
    // showNets();
    // showLEDsCore2 = 1;
    goto menu;
    // break;
  }
  case '{': {
    // removeBridgeFromNodeFile(19, 1);
    // probeActive = 1;
    // delayMicroseconds(5);
    blockProbeButton = 300;
    blockProbeButtonTimer = millis();
    int probeReturn = probeMode(0, firstConnection);

    // delayMicroseconds(5);
    probeActive = 0;
    // clearLEDs();
    // assignNetColors();
    // showNets();
    // showLEDsCore2 = 1;
    clearHighlighting();

    // Serial.print("millis() - startupTimers[0] = ");
    // Serial.println(millis() - startupTimers[0]);
    // Serial.flush();

    goto menu;
    // break;
  }
  case 'n':
    couldntFindPath(1);
    core1passthrough = 0;
    Serial.print("\n\n\rnetlist\n\r");
    // listSpecialNets();
    // Serial.print("\n\n\r");
    // Serial.print("anythingInteractiveConnected(-1) = ");
    // Serial.println(anythingInteractiveConnected(-1));
    // Serial.flush();
    listNets(anythingInteractiveConnected(-1));

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
    Serial.println(jumperlessConfig.routing.stack_paths);
    Serial.print("dacDuplicates: ");
    Serial.println(jumperlessConfig.routing.stack_dacs);
    Serial.print("railsDuplicates: ");
    Serial.println(jumperlessConfig.routing.stack_rails);
    Serial.print("railPriority: ");
    Serial.println(jumperlessConfig.routing.rail_priority);
    couldntFindPath(1);
    Serial.print("\n\rBridge Array\n\r");
    printBridgeArray();
    Serial.print("\n\n\n\rPaths\n\r");
    printPathsCompact(showDupes);
    Serial.print("\n\n\rChip Status\n\r");
    printChipStatus();
    Serial.print("\n\n\r");
    // Serial.print("Revision ");
    // Serial.print(revisionNumber);
    Serial.print("\n\n\r");
    break;
  }
  case 'm':
    goto forceprintmenu;
    break;

  case '!':
    printNodeFile(netSlot, 0, 0, 0, true);
    break;


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

  // case '>': {

  //   if (netSlot == NUM_SLOTS - 1) {
  //     netSlot = 0;
  //   } else {
  //     netSlot++;
  //   }

  //   Serial.print("\r                                         \r");
  //   Serial.print("Slot ");
  //   Serial.print(netSlot);
  //   slotPreview = netSlot;
  //   slotChanged = 1;
  //   // printAllChangedNetColorFiles();

  //   goto loadfile;
  // }
  case '<': {

    if (netSlot == 0) {
      netSlot = NUM_SLOTS - 1;
    } else {
      netSlot--;
    }
    // Serial.print("slotChanged = ");
    // Serial.println(millis() - timer);
    // Serial.print("\r                                         \r");
    Serial.print("Slot ");
    Serial.println(netSlot);
    slotPreview = netSlot;
    slotChanged = 1;
    // printAllChangedNetColorFiles();
    goto loadfile;
  }
  case 'y': {
  loadfile:
    loadingFile = 1;
    // Serial.print("loadingFile = ");
    // Serial.println(millis() - timer);
    if (slotChanged == 1) {
      // clearChangedNetColors(0);
      loadChangedNetColorsFromFile(netSlot, 0);
      // Serial.print("loadChangedNetColorsFromFile = ");
      // Serial.println(millis() - timer);
    }

    slotChanged = 0;
    loadingFile = 0;

    // Check if this is a USB refresh request
    // if (isUSBMassStorageMounted()) {
    //   manualRefreshFromUSB();
    // } else {
    refreshConnections(-1);
    //}
    // chooseShownReadings();
    //  setGPIO();
    // Serial.print("refreshConnections = ");
    // Serial.println(millis() - timer);
    break;
  }
  case 'f': {

    probeActive = 1;
    readInNodesArduino = 1;
    // clearAllNTCC();

    // sendAllPathsCore2 = 1;
    // timer = millis();

    // clearNodeFile(netSlot);

    // if (connectFromArduino != '\0') {
    //   serSource = 1;
    //   } else {
    //   serSource = 0;
    //   }
    savePreformattedNodeFile(serSource, netSlot, rotaryEncoderMode);

    // Validate the saved node file
    int validation_result = validateNodeFileSlot(netSlot, true);
    if (validation_result == 0) {
      Serial.println("NodeFile validated successfully");
      refreshConnections(-1);
    } else {
      Serial.println("NodeFile validation failed: " +
                     String(getNodeFileValidationError(validation_result)));
      Serial.println("Connections not refreshed due to invalid node file");
    }

    // //if (debugNMtime) {
    //   Serial.print("\n\n\r");
    //   Serial.print("took ");
    //   Serial.print(millis() - timer);
    //   Serial.print("ms");
    //  // }
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
  }

    // case '\n':
    //   goto menu;
    //   break;

  case 't': { //! t - Test MSC callbacks
    // Test function disabled
    goto dontshowmenu;
    break;
  }

  case 'T': { //! T - Show netlist info
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
    checkChangedNetColors(-1);

    assignNetColors();
#ifdef PIOSTUFF
    sendAllPaths();
#endif

    break;
  }

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

    int lastSerial1Passthrough = jumperlessConfig.serial_1.print_passthrough;
    int lastSerial2Passthrough = jumperlessConfig.serial_2.print_passthrough;
    printSerial1Passthrough = 0;
    printSerial2Passthrough = 0;

    Serial.print("\n\n\r0.   all off");
    Serial.print("\n\r9.   all on");
    Serial.print("\n\ra-z. exit\n\r");

    Serial.print("\n\r1. file parsing               =    ");
    Serial.print(debugFP);
    Serial.print("\n\r2. net manager                =    ");
    Serial.print(debugNM);
    Serial.print("\n\r3. chip connections           =    ");
    Serial.print(debugNTCC);
    Serial.print("\n\r4. chip conns alt paths       =    ");
    Serial.print(debugNTCC2);
    Serial.print("\n\r5. LEDs                       =    ");
    Serial.print(debugLEDs);
    Serial.print("\n\r6. logic analyzer debug       =    ");
    Serial.print(debugLA);
    Serial.print("\n\r7. show probe current         =    ");
    Serial.print(showProbeCurrent);
    Serial.print("\n\n\r8. print serial 1 passthrough =    ");
    if (jumperlessConfig.serial_1.print_passthrough == 1) {
      Serial.print("on");
    } else if (jumperlessConfig.serial_1.print_passthrough == 2) {
      Serial.print("flashing only");
    } else if (jumperlessConfig.serial_1.print_passthrough == 0) {
      Serial.print("off");
    }

    // Serial.print("\n\n\r6. swap probe pin         =    ");
    // if (probeSwap == 0) {
    //   Serial.print("19");
    // } else {
    //   Serial.print("18");
    // }

    Serial.println("\n\n\n\r");
    Serial.flush();

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
      printSerial1Passthrough = lastSerial1Passthrough;
      printSerial2Passthrough = lastSerial2Passthrough;
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
  delayMicroseconds(1000);
  while (Serial.available() > 5) {
    Serial.read();
    delayMicroseconds(1000);
  }
  Serial.flush();
  goto menu;
}

unsigned long lastSwirlTime = 0;

int swirlCount = 42;
int spread = 13;

int readcounter = 0;
unsigned long schedulerTimer = 0;
unsigned long schedulerUpdateTime = 6300;

int swirled = 0;
int countsss = 0;

int probeCycle = 0;
int netUpdateRefreshCount = 0;

int tempDD = 0;
int clearBeforeSend = 0;

unsigned long tempTimer = 0;
int lastTemp = 0;

int passthroughStatus = 0;

unsigned long serialInfoTimer = 0;

void loop1() {
  // int timer = micros();

  // while (startupAnimationFinished == 0) {

  //   }

  // Handle USB tasks on core1 as well (important for MSC interface)
  // #ifdef USE_TINYUSB
  //  tud_task();
  // #endif

  while (pauseCore2 == true) {
    tight_loop_contents();
  }
  
  // Only call logic analyzer if it's enabled and there's USB activity
  static uint32_t last_la_check = 0;
  uint32_t current_time = millis();
  
  // ENHANCED STATE-BASED HANDLER CALLING
  // Use the new state variables to make smarter decisions about when to call the handler
  bool should_call_handler = false;
  
  // Always call if device is actively running or armed
  if (julseview.getIsRunning() || julseview.getIsArmed() || julseview.getReceivedCommand()) {
    should_call_handler = true;
  

    
    julseview.handler();
  } else if (millis() - last_la_check >= 10) {
    last_la_check = millis();
    should_call_handler = true;
    julseview.handler();
  }
  
  // CRITICAL: Post-deinit DMA watchdog to prevent buffer overflow crashes
  // static uint32_t last_dma_watchdog_check = 0;
  // if (!julseview_active && (current_time - last_dma_watchdog_check > 10000)) {  // Check every 10 seconds when idle
  //   last_dma_watchdog_check = current_time;
    
  //   // Check if any DMA channels are still running (they shouldn't be after deinit)
  //   bool dma_still_running = false;
  //   for (int i = 0; i < NUM_DMA_CHANNELS; i++) {
  //     if (dma_channel_is_busy(i)) {
  //       dma_still_running = true;
  //       break;
  //     }
  //   }
    
  //   if (dma_still_running) {
  //    // Serial.println("WARNING: DMA channels still running after deinit - forcing cleanup");
  //     // Force cleanup of any remaining DMA activity
  //     for (int i = 0; i < NUM_DMA_CHANNELS; i++) {
  //       dma_channel_abort(i);
  //       dma_channel_wait_for_finish_blocking(i);
  //     }
  //   }
  // }
  

  // while (logicAnalyzing == true) {
  //   //handleLogicAnalyzer();
  //   delayMicroseconds(1000);
  // }

  if (doomOn == 1) {
    playDoom();
    doomOn = 0;
  } else if (pauseCore2 == 0 && julseview_active == false) {
    core2stuff();
  }

  // if (millis() - serialInfoTimer > 10) {
  //   serialInfoTimer = millis();

  if (core1passthrough == 0 || inClickMenu == 1 || inPadMenu == 1 ||
      probeActive == 1) {
    passthroughStatus = secondSerialHandler();
  }

  replyWithSerialInfo();

  if (dumpLED == 1) {

    if (millis() - dumpLEDTimer > dumpLEDrate) {
      if (core1busy == false) {
        core2busy = true;
        core1busy = true;
        delayMicroseconds(2000);
        dumpLEDs();
        delayMicroseconds(1000);
        core2busy = false;
        core1busy = false;
      }

      dumpLEDTimer = millis();
    }
  }

  if (blockProbingTimer > 0) {
    if (millis() - blockProbingTimer > blockProbing) {
      blockProbing = 0;
      blockProbingTimer = 0;
      // Serial.println("probing unblocked");
    }
  }
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

  // if (showLEDsCore2 != 0 || core1busy != false || core1request != 0 ||
  // sendAllPathsCore2 != 0) { Serial.println("showLEDsCore2 = " +
  // String(showLEDsCore2)); Serial.println("core1busy = " + String(core1busy));
  // Serial.println("core1request = " + String(core1request));
  // Serial.println("sendAllPathsCore2 = " + String(sendAllPathsCore2));
  // Serial.println();

  // }
  if (micros() - schedulerTimer > schedulerUpdateTime || showLEDsCore2 == 3 ||
      showLEDsCore2 == 4 ||
      showLEDsCore2 == 6 && core1busy == false && core1request == 0) {

    if ((((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3 ||
          (swirled == 1) && sendAllPathsCore2 == 0) ||
         showProbeLEDs != lastProbeLEDs) &&
        sendAllPathsCore2 == 0) {

      // Serial.println(showLEDsCore2);
      // secondSerialHandler();
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
          inPadMenu == 0 && hideNets == 0) {

        if (defconDisplay >= 0 && probeActive == 0) {

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

          readGPIO(); // if want, I can make this update the LEDs like 10 times
                      // faster by putting outside this loop,
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

      if (sendAllPathsCore2 == 1) {
        sendPaths(0);
      } else if (sendAllPathsCore2 == -1) {
        sendPaths(1);
      } else {
        sendPaths(sendAllPathsCore2);
      }
      sendAllPathsCore2 = 0;

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

        if (defconDisplay > 5) {
          defconDisplay = 0;
        }
      }

      if (readcounter > 100) {
        readcounter = 0;
        // probeCycle++;
        if (probeCycle > 4) {
          probeCycle = 1;
        }
      }

      rotaryEncoderStuff();

      if (inClickMenu == 0 && loadingFile == 0 && showLEDsCore2 == 0 &&
          core1busy == false) {
        // showAllRowAnimations();
      }
    } else {
      // secondSerialHandler();
      rotaryEncoderStuff();
    }
    schedulerTimer = micros();
    core2busy = false;
    // readGPIO();
  }
}
