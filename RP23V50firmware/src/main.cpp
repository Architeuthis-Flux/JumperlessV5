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
#include "RotaryEncoder.h"
#include "Apps.h"
#include "configManager.h"
#include "oled.h"
//#define USE_FATFS 1
#ifdef USE_FATFS
#include "FatFS.h"
#include "USBfs.h"
#endif
#include "UserCode.h"
#include "SerialWrapper.h"
#include "Highlighting.h"

#define Serial SerialWrap

//#define Serial SerialWrap

bread b;


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

volatile bool core2initFinished = false;

volatile bool configLoaded = false;

volatile int startupAnimationFinished = 0;

unsigned long startupTimers[10];



void setup() {
  pinMode(RESETPIN, OUTPUT_12MA);

  digitalWrite(RESETPIN, HIGH);

  //delayMicroseconds(800);
  // Serial.setTimeout(8000);
  //  USB_PID = 0xACAB;
  //  USB_VID = 0x1D50;
  //  USB_MANUFACTURER = "Architeuthis Flux";
  //  USB_PRODUCT = "Jumperless";
    //SerialWrap.enableSerial1(true);  // Enable Serial1 + USBSer1 mirroring
    // SerialWrap.enableSerial2(true);  // Enable Serial2 + USBSer2 mirroring

    // Your existing code remains unchanged!
    // This now automatically initializes ALL enabled serial ports with the same baud rate

    // SerialWrap.enableSerial1(true);

    // SerialWrap.enableUSBSer1(true);

    //Serial.begin(115200);

  startupTimers[0] = millis();
  // Initialize FatFS
  if (!FatFS.begin()) {
    Serial.println("Failed to initialize FatFS");
    }


  //EEPROM.begin(512);

  //debugFlagInit(0); //these will be overridden by the config file

  // Load configuration
  // delay(1000);
  // unsigned long start = millis();

  loadConfig();


  //readSettingsFromConfig();
  configLoaded = 1;
  startupTimers[1] = millis();
  delayMicroseconds(200);
  // Serial.print("config loaded in ");
  // Serial.print(millis() - start);
  // Serial.println("ms");

  backpowered = 0;



  // uint8_t serialTarget = SERIAL_PORT_MAIN;
  // //SerialWrap.enableSerial1(true);
  // Serial.enableSerial1(true);
  // Serial.enableSerial2(true);
  // Serial.enableUSBSer1(true);
  // Serial.enableUSBSer2(true);
  // Serial.enableUSBSer1(true);

  //if (jumperlessConfig.serial_1.function == 2) {

    //SerialWrap.setSerialTarget(SERIAL_PORT_SERIAL1 | SERIAL_PORT_MAIN);
   // SerialWrap.setSerialReadTarget(SERIAL_PORT_SERIAL1 | SERIAL_PORT_MAIN);
    // SerialWrap.setSerialWriteTarget(SERIAL_PORT_USBSER1 | SERIAL_PORT_MAIN);
   // } 

    //  if (jumperlessConfig.serial_2.function == 2) {
    //   //SerialWrap.enableSerial2(true);
    //     serialTarget = SERIAL_PORT_SERIAL2 | SERIAL_PORT_MAIN;
    //   }

  // Serial.setSerialTarget(serialTarget);



  //Serial.println("Hello! This automatically goes to all enabled ports!");
  initArduino();
  initSecondSerial();
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

  drawAnimatedImage(0);
  startupAnimationFinished = 1;
  startupTimers[4] = millis();
  clearAllNTCC();


  initRotaryEncoder();
  startupTimers[5] = millis();

  delayMicroseconds(100);

  // delay(100);
  initMenu();
  startupTimers[6] = millis();
  initADC();
  startupTimers[7] = millis();

  // pinMode(18, INPUT_PULLUP); //reset lines for arduino
  // pinMode(19, INPUT_PULLUP);


  //routableBufferPower(1, 0);
  //routableBufferPower(1, 1);

  getNothingTouched();
  startupTimers[8] = millis();
  createSlots(-1, 0);
  startupTimers[9] = millis();

  // Test the new DefineInfo structs
  //testDefineInfoStructs();

  }

unsigned long startupCore2timers[10];

void setupCore2stuff() {
  // delay(2000);
  startupCore2timers[0] = millis();
  initCH446Q();
  startupCore2timers[1] = millis();
  //delay(1);
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

char input = '\0';

int serSource = 0;
int readInNodesArduino = 0;


const char firmwareVersion[] = "5.1.3.2"; // remember to update this

int firstLoop = 1;

volatile int probeActive = 0;

int showExtraMenu = 1;

int lastHighlightedNet = -1;
int lastBrightenedNet = -1;
int lastWarningNet = -1;






unsigned long timer = 0;
int lastProbeButton = 0;
unsigned long waitTimer = 0;
unsigned long switchTimer = 0;
int flashingArduino = 0;
int attract = 0;

volatile int core1passthrough = 1;

#include <pico/stdlib.h>

#include <hardware/adc.h>
#include <hardware/gpio.h>
void loop() {

  if (firstLoop == 1) {

    if (firstStart == true) {
      calibrateDacs();
      firstStart = false;
      }
    firstLoop = 2;


    // Serial.println("--------------------------------");
    loadChangedNetColorsFromFile(netSlot, 0);



    if (jumperlessConfig.serial_1.connect_on_boot == 1) {
      connectArduino(0, 0);
      }
    if (jumperlessConfig.serial_2.connect_on_boot == 1) {
      connectArduino(0, 0);
      }

    //routableBufferPower(1, 1);
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
      //Serial.println("Initializing OLED");
      oled.init();
      }

    firstLoop = 1;
    }

menu:

  if (SerialWrap.available() > 20) { //this is so if you dump a lot of data into the serial buffer, it will consume it and not keep looping
    while (SerialWrap.available() > 0) {
      char c = SerialWrap.read();
      //Serial.print(c);
     // Serial.flush();
      }
    }

  if (lastProbePowerDAC != probePowerDAC) {
    probePowerDACChanged = true;
    //delay(1000);
    Serial.print("probePowerDACChanged = "); Serial.println(probePowerDACChanged);
    routableBufferPower(1, 1);
    }

  clearHighlighting();

  Serial.print("\n\n\r\t\tMenu\n\r");

  Serial.print("\n\r");
  Serial.print("\tm = show this menu\n\r");

  //Serial.println();

  Serial.print("\tn = show net list\n\r");
  Serial.print("\tb = show bridge array\n\r");
  Serial.print("\tc = show crossbar status\n\r");
  //Serial.print("\t! = print node file\n\r");
  Serial.print("\ts = show all slot files\n\r");

  Serial.print("\t\b\b</> = cycle slots\n\r");

  Serial.println();

  Serial.print("\t? = show firmware version\n\r");
  Serial.print("\t' = show startup animation\n\r");
  Serial.print("\td = set debug flags\n\r");
  Serial.print("\tl = LED brightness / test\n\r");
  Serial.print("\t\b\b`/~ = edit / print config\n\r");

  Serial.println();

  //Serial.print("\t$ = calibrate DACs\n\r");
  if (probePowerDAC == 0) {
    Serial.print("\t^ = set DAC 1 voltage\n\r");
    } else if (probePowerDAC == 1) {
      Serial.print("\t^ = set DAC 0 voltage\n\r");
      }
    Serial.print("\tv = get ADC reading\n\r");

    // Serial.println();

    Serial.print("\t# = print text from menu\n\r");
    Serial.print("\tg = print gpio state\n\r");
    //Serial.print("\t\b\b\b\b[0-9] = run app by index\n\r");
    Serial.print("\t. = connect oled\n\r");
    Serial.print("\tr = reset Arduino (rt/rb)\n\r");

    Serial.print("\t\b\ba/A = dis/connect UART to D0/D1\n\r");

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
    Serial.println();


    Serial.println();

    Serial.print("\tf = load node file\n\r");
    Serial.print("\tx = clear all connections\n\r");
    Serial.print("\t+ = add connections\n\r");
    Serial.print("\t- = remove connections\n\r");

    //Serial.print("\te = extra menu options\n\r");

    //if (showExtraMenu == 1) {
      //Serial.println();








      //Serial.print("\to = load node file by slot\n\r");
     // Serial.print("\tu = scan board\n\r");





      //Serial.print("\t` = edit config\n\r");


     // }

    // for (int i = 0; i < 10; i++) {
    //   Serial.print("gpioState[");
    //   Serial.print(i);
    //   Serial.print("]: ");
    //   Serial.print(gpioState[i]);
    //   Serial.print("\tgpioNet[");
    //   Serial.print(i);
    //   Serial.print("]: ");
    //   Serial.println(gpioNet[i]);
    //   }

    Serial.print("\n\n\r");
    //b.clear();
      //Serial.println(yesNoMenu());



    Serial.flush();

    if (configChanged == true) {
      Serial.print("config changed, saving...");
      saveConfig();
      Serial.println("\r                             \rconfig saved!\n\r");
      Serial.flush();
      configChanged = false;
      }
  dontshowmenu:


    connectFromArduino = '\0';
    firstConnection = -1;
    core1passthrough = 1;

    ///Serial.setSerialTarget( SERIAL_PORT_USBSER1);
    //SerialWrap.setSerialTarget(SERIAL_PORT_MAIN | SERIAL_PORT_USBSER1 | SERIAL_PORT_SERIAL1);

    //uint8_t serialTarget = SERIAL_PORT_MAIN;

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

    //Serial.setSerialTarget(serialTarget);

        //! This is the main busy wait loop waiting for input
    while (SerialWrap.available() == 0 && connectFromArduino == '\0' && slotChanged == 0) {

      //warningNet = 7;
      //firstConnection = -1;
      checkPads();
      int encoderNetHighlighted = encoderNetHighlight();
      if (encoderNetHighlighted != -1) {
        firstConnection = encoderNetHighlighted;
        // Serial.print("firstConnection: ");
        // Serial.println(firstConnection);
        // Serial.flush();
        } else {
        // firstConnection = -1;
        }
      // if (attract == 1) {
      //   // rotaryEncoderStuff();
      //   if (attractMode() == 1) {
      //     goto loadfile;
      //     }
      //   }
// core1passthrough = 1;
      secondSerialHandler();
      // //core1passthrough = 0;

      if (clickMenu() >= 0) {
        // defconDisplay = -1;
        core1passthrough = 0;
        goto loadfile;
        }

      int probeReading = justReadProbe(true);


      checkForReadingChanges();

      warnNetTimeout(1);
      if (probeReading > 0) {
        // Serial.print("probeReading = ");
        // Serial.println(probeReading);
        if (highlightNets(probeReading) != -1) {
          // Serial.print("highlightNets(probeReading) != -1");
          // Serial.print("     probeReading = ");
          // Serial.println(probeReading);
          // Serial.flush();

          // Serial.print("firstConnection: ");
          // Serial.println(probeReading);
          // Serial.flush();
          firstConnection = probeReading;
          }
        // Serial.print("lastValidProbeReading = ");
        // Serial.println(probeReading);
        // Serial.flush();
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
            //firstConnection = 5;//probeReading;
            if (firstConnection > 0) {
              if (warningNet == brightenedNet && warningTimeout > 0) {
                //warningNet = -1;
                //brightenedNet = 0;
                warningTimeout = 0;
                //warningTimer = 0;
                connectOrClearProbe = 0;
                showProbeLEDs = 2;
                probeActive = 1;
                input = '{';
                probingTimer = millis();
                startupTimers[0] = millis();
                // Serial.println("probing\n\r");

                goto skipinput;
                //warnNet(-1);
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
                  //warnNet(firstConnection);
                  //assignNetColors();
                  //showLEDsCore2 = 1;

                  // Serial.print("-4 warningNet = ");
                  // Serial.println(warningNet);
                  // Serial.flush();
                  //clearHighlighting();

                  firstConnection = -1;
                  blockProbeButton = 500;
                  blockProbeButtonTimer = millis();
                  }
        } else {
        firstConnection = -1;
        }

      if ((millis() - waitTimer) > 30) {
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


          if (showReadings >= 1) {
            //chooseShownReadings();
            showMeasurements(8, 2, 0);
            }

          if (millis() - oled.lastConnectionCheck > oled.connectionCheckInterval && jumperlessConfig.top_oled.enabled == 1) {
            ///Serial.println("oled connection lost");
            oled.checkConnection();
            if (oled.isConnected() == false) {
              // Serial.println("oled connection lost");
              oled.oledConnected = false;
              oled.disconnect();
              jumperlessConfig.top_oled.enabled = 0;
              if (jumperlessConfig.top_oled.lock_connection == 1 && oled.connectionRetries < oled.maxConnectionRetries) {
                oled.connectionRetries++;
                // if (oled.connectionRetries > oled.maxConnectionRetries) {
                //   oled.connectionRetries = 0;
                oled.init();
                //}
                }
              }
            }
      }

    if (slotChanged == 1) {
      //Serial.println("slotChanged");
      refreshPaths();
      //clearChangedNetColors(0);
      loadChangedNetColorsFromFile(netSlot, 0);
      goto loadfile;
      }

    // if (connectFromArduino != '\0') {
    //   } else {

    // if (serialCommandBufferIndex > 0) {
    //   input = serialCommandBuffer[0];
    //   Serial.print("command input = ");
    //   Serial.println(input);
    //   Serial.flush();
    //   serialCommandBufferIndex--;
    //   for (int i = 0; i < serialCommandBufferIndex; i++) {
    //     serialCommandBuffer[i] = serialCommandBuffer[i + 1];
    //     }
    //   serialCommandBuffer[serialCommandBufferIndex] = '\0';
    // } else {
    //  if (SerialWrap.available() > 0) {

    // Serial.print("Serial target = ");
    // Serial.println(Serial.getSerialTarget());
    input = SerialWrap.read();
    // Serial.print("input = ");
    // Serial.println(input);
    // for (int i = 0; i < 10; i++) {
    //   Serial.print(SerialWrap.read());
    //   }
    // Serial.println();
    // Serial.flush();
  // } else {
  //   input = Serial.read();
  // }
//  }
  //  Serial.setSerialTarget(SERIAL_PORT_MAIN);
    // Serial.print("input = ");
    // Serial.println(input);
    //Serial.setSerialTarget(SERIAL_PORT_MAIN);
    // Serial.print("\n\r");
    if (input == '}' || input == ' ' || input == '\n' || input == '\r') {
      goto dontshowmenu;
      }
    //}

  skipinput:

    // if (isDigit(input)) {
    //   runApp(input - '0');
    //   return;
    //   }



    switch (input) {
      case 'k': {
      // for (int i = 0; i < 255; i++) {
      //   Serial.print(i);
      //   Serial.print(": ");
      //   char* name = colorToName(i, -1);
      //   Serial.println(name);
      //   }

      // Call the demo function directly - it will check for range input itself
      Serial.println("Displaying color names (enter range like '10-200' for specific range)");
      colorPicker(0, 255);

      break;
      }
      case 'R': {
      printWireStatus();
      // printSerial1stuff();
       //printAllRLEimageData();
      break;
      }
      case '.': {
      //initOLED();
      if (jumperlessConfig.top_oled.enabled == 0) {
        jumperlessConfig.top_oled.enabled = 1;
        configChanged = true;
        } else {
        jumperlessConfig.top_oled.enabled = 0;
        configChanged = true;
        }

      oled.init();
      // oled.print("FUCK");
      // oled.show();
      // oled.test();
      break;
      }

      case 'c': {
      printChipStateArray();
      break;
      }

      case '_': {
      printMicrosPerByte();
      break;
      }

      case 'g': {
      printGPIOState();
      break;
      }
      case '&': {
      loadChangedNetColorsFromFile(netSlot, 0);
      break;
      int node1 = -1;
      int node2 = -1;
      while (SerialWrap.available() == 0) {
        }
      //char c = Serial.read();
      node1 = SerialWrap.parseInt();
      node2 = SerialWrap.parseInt();
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

      case '\'': {
      pauseCore2 = 1;
      delay(1);
      drawAnimatedImage(0);
      pauseCore2 = 0;
      break;
      }
      case 'x': {
      digitalWrite(RESETPIN, HIGH);
      delay(1);
      refreshPaths();
      clearAllNTCC();

      clearNodeFile(netSlot, 0);
      refreshConnections(-1);
      digitalWrite(RESETPIN, LOW);

      break;
      }

      case '+': {

      readStringFromSerial(0, 0);
      goto loadfile;

      break;
      }

      case '-': {
      readStringFromSerial(0, 1);
      goto loadfile;
      break;
      }


      case '~': {
      core1busy = 1;
      waitCore2();
      printConfigToSerial();
      core1busy = 0;
      break;
      }
      case '`': {
      core1busy = 1;
      waitCore2();
      readConfigFromSerial();
      core1busy = 0;
      break;
      }
              // case '2': {
              // runApp(2);
              // break;
              // }

      case '^': {
      // doomOn = 1;
      // Serial.println(yesNoMenu());
      // break;
      char f[8] = { ' ' };
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
        break;
      }


      case '?': {
      Serial.print("Jumperless firmware version: ");
      Serial.println(firmwareVersion);
      break;
      }
      case '@': {
      // printWireStatus();

      i2cScan(8, 7, 22, 23, 1);
      oledTest(8, 7, 22, 23, 1);

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
      case 'r': {
      if (SerialWrap.available() > 0) {
        char c = SerialWrap.read();
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

      case 'A': {
      // delay(100);
      int justAsk = 0;
      while (SerialWrap.available() > 0) {
        // Serial.print("checking for arduino connection");
        char c = SerialWrap.read();
        if (c == ' ') {
          continue;
          }
        if (c == '?') {
          if (checkIfArduinoIsConnected() == 1) {
            justAsk = 1;
            Serial.println("Y");
            break;
            } else {
            justAsk = 1;
            Serial.println("n");
            break;
            }
          } else {
          break;
          }
        }
      if (justAsk == 0) {
        connectArduino(0);
        //   removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, 0);
        //   removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, 0);
        //   addBridgeToNodeFile(RP_UART_RX, NANO_D1, netSlot, 0, 1);
        //   addBridgeToNodeFile(RP_UART_TX, NANO_D0, netSlot, 0, 1);
        //   //ManualArduinoReset = true;
          //  // goto loadfile;
          //   refreshConnections(-1);
        }
      break;
      }
      case 'a': {
      // delay(100);
      int justAsk = 0;
      while (SerialWrap.available() > 0) {
        // Serial.print("checking for arduino connection");
        char c = SerialWrap.read();
        if (c == ' ') {
          continue;
          }
        if (c == '?') {
          if (checkIfArduinoIsConnected() == 1) {
            justAsk = 1;
            Serial.println("Y");
            break;
            } else {
            justAsk = 1;
            Serial.println("n");
            break;
            }
          } else {
          break;
          }
        }
      if (justAsk == 0) {
        disconnectArduino(0);
        // removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, 0);
        // removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, 0);
        // // removeBridgeFromNodeFile(RP_UART_RX, -1, netSlot, 0);
        // // removeBridgeFromNodeFile(RP_UART_TX, -1, netSlot, 0);
        // //refreshLocalConnections(-1);
        // refreshConnections(-1);
        }
      //goto loadfile;
      break;
      }

      case 'F':
        oled.cycleFont();
        break;

      case '=': {
      //  while (SerialWrap.available() == 0) {
      //  }
      //  char o = SerialWrap.read();
       //oled.testCharBounds("Fuck", 2);

      Serial.println("\n\r");
      //oled.debugJokermanBaseline();
      oled.dumpFrameBuffer();
      oled.testMenuPositioning();
      oled.dumpFrameBuffer();
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


      break;





      }



      case 'i': {
      if (oled.isConnected() == false) {
        if (oled.init() == false) {
          Serial.println("Failed to initialize OLED");
          break;
          }
        }



      oledTest(NANO_D2, NANO_D3, 22, 23);

      break;
      }

      case '#': {
      // pauseCore2 = 1;
      //  while (slotChanged == 0)
      //  {
      //
      while (SerialWrap.available() == 0 && slotChanged == 0) {
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
        if (SerialWrap.available() > 0) {
          char c = SerialWrap.read();

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
          // break;
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

      case '>': {

      if (netSlot == NUM_SLOTS - 1) {
        netSlot = 0;
        } else {
        netSlot++;
        }

      Serial.print("\r                                         \r");
      Serial.print("Slot ");
      Serial.print(netSlot);
      slotPreview = netSlot;
      slotChanged = 1;
      // printAllChangedNetColorFiles();

      goto loadfile;
      }
      case '<': {

      if (netSlot == 0) {
        netSlot = NUM_SLOTS - 1;
        } else {
        netSlot--;
        }
      Serial.print("\r                                         \r");
      Serial.print("Slot ");
      Serial.print(netSlot);
      slotPreview = netSlot;
      slotChanged = 1;
      //printAllChangedNetColorFiles();
      goto loadfile;
      }
      case 'y': {
    loadfile:
      loadingFile = 1;
      if (slotChanged == 1) {
        //clearChangedNetColors(0);
        loadChangedNetColorsFromFile(netSlot, 0);
        }

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

        // if (connectFromArduino != '\0') {
        //   serSource = 1;
        //   } else {
        //   serSource = 0;
        //   }
        savePreformattedNodeFile(serSource, netSlot, rotaryEncoderMode);

        refreshConnections(-1);

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
        checkChangedNetColors(-1);

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
      Serial.print("\n\r6. show probe current         =    ");
      Serial.print(showProbeCurrent);
      Serial.print("\n\n\r7. print serial 1 passthrough =    ");
      if (jumperlessConfig.serial_1.print_passthrough == 1) {
        Serial.print("on");
        } else if (jumperlessConfig.serial_1.print_passthrough == 2) {
          Serial.print("flashing only");
          } else if (jumperlessConfig.serial_1.print_passthrough == 0) {
            Serial.print("off");
            }
          Serial.print("\n\r8. print serial 2 passthrough =    ");
          if (jumperlessConfig.serial_2.print_passthrough == 1) {
            Serial.print("on");
            } else if (jumperlessConfig.serial_2.print_passthrough == 2) {
              Serial.print("flashing only");
              } else if (jumperlessConfig.serial_2.print_passthrough == 0) {
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

              while (SerialWrap.available() == 0)
                ;

              int toggleDebug = SerialWrap.read();
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

        if (SerialWrap.read() == ':') {
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
        while (SerialWrap.available() > 0) {
          int f = SerialWrap.read();
          // delayMicroseconds(30);
          }

        break;
      }
    delayMicroseconds(1000);
    while (SerialWrap.available() > 0) {
      SerialWrap.read();
      delayMicroseconds(1000);
      }
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

void loop1() {
  // int timer = micros();

  // while (startupAnimationFinished == 0) {

  //   }

  if (doomOn == 1) {
    playDoom();
    doomOn = 0;
    } else if (pauseCore2 == 0) {
      core2stuff();
      }


    if (core1passthrough == 0 || inClickMenu == 1 || inPadMenu == 1 || probeActive == 1) {
      passthroughStatus = secondSerialHandler();
      }

    //     } else {
    //      //core1passthrough = 0;
    //      }

    // //core1passthrough = 0;
    // int passthroughCount = 0;
    //    while (passthroughStatus == 1) {
    //     passthroughStatus = secondSerialHandler();
    //     // Serial.println("Serial2 passthrough");
    //     // Serial.println(passthroughStatus);
    //     // Serial.flush();
    //     if (passthroughStatus == 0) {
    //        passthroughCount++;
    //       //break;
    //       }

    //     if (passthroughCount > 100) {
    //       break;
    //       }
     // }

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

    if ((((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3 ||
          (swirled == 1) && sendAllPathsCore2 == 0) ||
         showProbeLEDs != lastProbeLEDs) && sendAllPathsCore2 == 0) {

      // Serial.println(showLEDsCore2);
      //secondSerialHandler();
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

          readGPIO(); //if want, I can make this update the LEDs like 10 times faster by putting outside this loop, 
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
            //secondSerialHandler();
            rotaryEncoderStuff();
            }
          schedulerTimer = micros();
          core2busy = false;
          // readGPIO();
    }
  }

