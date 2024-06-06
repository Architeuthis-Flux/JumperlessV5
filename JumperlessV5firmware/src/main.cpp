// SPDX-License-Identifier: MIT

/*
Kevin Santo Cappuccio
Architeuthis Flux

KevinC@ppucc.io

5/28/2024

*/
#include <Arduino.h>
#define USE_TINYUSB 1
// #define LED LED_BUILTIN
// #ifdef USE_TINYUSB
//#include "../include/Adafruit_TinyUSB_Arduino_changed/Adafruit_TinyUSB_changed.h"
#include "../lib/Adafruit_TinyUSB_Arduino_changed/src/Adafruit_TinyUSB_changed.h"
// #endif
#include <SPI.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
// #ifdef CFG_TUSB_CONFIG_FILE
// #include CFG_TUSB_CONFIG_FILE
// #else
//#include "tusb_config.h"
// #endif
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "LittleFS.h"
#include "MachineCommands.h"
#include "MatrixStateRP2040.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "RotaryEncoder.h"
//#include <Adafruit_TinyUSB.h>

// #include "AdcUsb.h"
// #include "logic_analyzer.h"
// LogicAnalyzer logicAnalyzer;
// Capture capture(MAX_FREQ, MAX_FREQ_THRESHOLD);

// #include "pico/multicore.h"
// #include "hardware/watchdog.h"

// using namespace logic_analyzer;

bread b;

Adafruit_USBD_CDC USBSer1;

int supplySwitchPosition = 0;

void machineMode(void);
void lastNetConfirm(int forceLastNet = 0);
void rotaryEncoderStuff(void);

volatile uint8_t pauseCore2 = 0;

volatile int loadingFile = 0;

unsigned long lastNetConfirmTimer = 0;
// int machineMode = 0;

volatile int sendAllPathsCore2 = 0; // this signals the core 2 to send all the paths to the CH446Q

int rotEncInit = 0;
// https://wokwi.com/projects/367384677537829889

void setup()
{
  pinMode(RESETPIN, OUTPUT_12MA);

  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(4);
  //Serial.setTimeout(8000);
  // USB_PID = 0xACAB;
  // USB_VID = 0x1D50;
  // USB_MANUFACTURER = "Architeuthis Flux";
  // USB_PRODUCT = "Jumperless";
  // USBSetup


  USBDevice.setProductDescriptor("Jumperless");
  USBDevice.setManufacturerDescriptor("Architeuthis Flux");
  USBDevice.setSerialDescriptor("0");
  USBDevice.setID(0x1D50, 0xACAB);
  USBDevice.addStringDescriptor("Jumperless");
  USBDevice.addStringDescriptor("Architeuthis Flux");

delay(20);
  
Serial.begin(115200);

delay(20);


  // USBSer1.setStringDescriptor("Jumperless USB Serial");

  // USBSer1.begin(115200);

  EEPROM.begin(256);

  debugFlagInit();

  delay(10);

  initDAC(); // also sets revisionNumber

  delay(1);

  initINA219();

  

  initArduino();
  delay(4);

  LittleFS.begin();

  // setDac0_5Vvoltage(0.0);
  // setDac1_8Vvoltage(1.9);

  createSlots(-1, rotaryEncoderMode);

  clearAllNTCC();

  initMenu();

  digitalWrite(RESETPIN, LOW);

  initRotaryEncoder();

  delay(10);


}

int core2initFinished = 0;
void setup1()
{
  //delay(10);

  initGPIOex();

  delay(4);

  initCH446Q();

  //delay(4);

  initLEDs();

  delay(4);

  core2initFinished = 1;
  // delay(4);

  //lightUpRail();

  //delay(4);
//showLEDsCore2 = 1;
}

char connectFromArduino = '\0';

char input;

int serSource = 0;
int readInNodesArduino = 0;
int baudRate = 115200;

int restoredNodeFile = 0;

const char firmwareVersion[] = "5.0.00"; //// remember to update this

int firstLoop = 1;

volatile int probeActive = 1;

int showExtraMenu = 0;

void printSMstatus(void)
{
  for (int i = 0; i < 4; i++)
  {
    Serial.print("PIO_0 State Machine ");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(pio_sm_is_claimed(pio0, i));
    Serial.println(" ");
  }

  for (int i = 0; i < 4; i++)
  {
    Serial.print("PIO_1 State Machine ");
    Serial.print(i);
    Serial.print(" ");
    Serial.print(pio_sm_is_claimed(pio1, i));
    Serial.println(" ");
  }
}
int tinyUSB = 0;
unsigned long timer = 0;

void loop()
{
// #ifdef USE_TINYUSB
//   tinyUSB = 1;
// #endif

// while (core2initFinished == 0)
// {
// }
 delay(15);
// Serial.flush();
// startupColorsV5();
//lightUpRail();

//showLEDsCore2 = 3;



menu:
  //showLEDsCore2 = 1;
  //Serial.println(showLEDsCore2);
  Serial.print("Fuck you!\n\r");

  printMainMenu(showExtraMenu);
 
  if (firstLoop == 1 && rotaryEncoderMode == 1)
  {
    Serial.print("Use the rotary encoder to change slots\n\r");
    Serial.print("Press the button to select\n\r");
    Serial.print("\n\n\r");
    firstLoop = 0;
    probeActive = 0;

    goto loadfile;
  }

dontshowmenu:

  connectFromArduino = '\0';

  while (Serial.available() == 0 && connectFromArduino == '\0' && slotChanged == 0)
  {
      clickMenu();
     
    if (showReadings >= 1)
    {
      showMeasurements();
    }

    // if ((millis() % 200) < 5)
    // {
    //   if (checkProbeButton() == 1)
    //   {
    //     int longShort = longShortPress(1000);
    //     if (longShort == 1)
    //     {
    //       input = 'c';
    //       probingTimer = millis();
    //       goto skipinput;
    //     }
    //     else if (longShort == 0)
    //     {
    //       input = 'p';
    //       probingTimer = millis();
    //       goto skipinput;
    //     }
    //   }

      // pinMode(19, INPUT);
   // }
  }

  if (slotChanged == 1)
  {
    goto loadfile;
  }

  if (connectFromArduino != '\0')
  {
  }
  else
  {
    input = Serial.read();
    // Serial.print("\n\r");
    if (input == '}' || input == ' ' || input == '\n' || input == '\r')
    {
      goto dontshowmenu;
    }
  }

skipinput:
  switch (input)
  {
  case '?':
  {
    Serial.print("Jumperless firmware version: ");
    Serial.println(firmwareVersion);
    break;
  }
  case '$':
  {
    // return current slot number
    Serial.println(netSlot);
    break;
  }

  case 'g':
  {
    // pauseCore2 = 1;
    //  while (slotChanged == 0)
    //  {
    //
    while (Serial.available() == 0 && slotChanged == 0)
    {
      if (slotChanged == 1)
      {
        // b.print("Jumperless", 0x101000, 0x020002, 0);
        // delay(100);
        goto menu;
      }
    }
    b.clear();

    char f[80] = {' '};
    int index = 0;
    // leds.clear();
    while (Serial.available() > 0)
    {
      if (index > 19)
      {
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
    uint32_t color = 0x101000;
    // Serial.print(index);
    b.print(f, color);

    break;
  }
  case 'e':
  {
    if (showExtraMenu == 0)
    {
      showExtraMenu = 1;
    }
    else
    {
      showExtraMenu = 0;
    }
    break;
  }

  case 's':
  {
    int fileNo = -1;

    if (Serial.available() > 0)
    {
      fileNo = Serial.read();
      // break;
    }
    Serial.print("\n\n\r");
    if (fileNo == -1)
    {
      Serial.print("\tSlot Files");
    }
    else
    {
      Serial.print("\tSlot File ");
      Serial.print(fileNo - '0');
    }
    Serial.print("\n\n\r");
    Serial.print("\n\ryou can paste this text reload this circuit (enter 'o' first)");
    Serial.print("\n\r(or even just a single slot)\n\n\n\r");
    if (fileNo == -1)
    {
      for (int i = 0; i < NUM_SLOTS; i++)
      {
        Serial.print("\n\rSlot ");
        Serial.print(i);
        if (i == netSlot)
        {
          Serial.print("        <--- current slot");
        }

        Serial.print("\n\rnodeFileSlot");
        Serial.print(i);
        Serial.print(".txt\n\r");

        Serial.print("\n\rf ");
        printNodeFile(i);
        Serial.print("\n\n\n\r");
      }
    }
    else
    {

      Serial.print("\n\rnodeFileSlot");
      Serial.print(fileNo - '0');
      Serial.print(".txt\n\r");

      Serial.print("\n\rf ");

      printNodeFile(fileNo - '0');
      Serial.print("\n\r");
    }

    break;
  }
  case 'v':

    if (showReadings >= 3 || (inaConnected == 0 && showReadings >= 1))
    {
      showReadings = 0;
      break;
    }
    else
    {
      showReadings++;

      chooseShownReadings();
      // Serial.println(showReadings);

      goto dontshowmenu;
      // break;
    }
  case 'p':
  {
    probeActive = 1;

    delayMicroseconds(1500);
    probeMode(19, 1);
    delayMicroseconds(1500);
    probeActive = 0;
    break;
  }
  case 'c':
  {
    // removeBridgeFromNodeFile(19, 1);
    probeActive = 1;
    delayMicroseconds(1500);
    probeMode(19, 0);
    delayMicroseconds(1500);
    probeActive = 0;
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

    if (waveGen() == 1)
    {
      break;
    }
  case 'o':
  {
    // probeActive = 1;
    inputNodeFileList(rotaryEncoderMode);
    showSavedColors(netSlot);
    // input = ' ';
    showLEDsCore2 = 1;
    // probeActive = 0;
    goto loadfile;
    // goto dontshowmenu;
    break;
  }

  case 'x':
  {

    if (netSlot == NUM_SLOTS - 1)
    {
      netSlot = 0;
    }
    else
    {
      netSlot++;
    }

    Serial.print("\r                                         \r");
    Serial.print("Slot ");
    Serial.print(netSlot);
    slotPreview = netSlot;
    goto loadfile;
  }
  case 'z':
  {

    if (netSlot == 0)
    {
      netSlot = NUM_SLOTS - 1;
    }
    else
    {
      netSlot--;
    }
    Serial.print("\r                                         \r");
    Serial.print("Slot ");
    Serial.print(netSlot);
    slotPreview = netSlot;
    goto loadfile;
  }
  case 'y':
  {
  loadfile:
    loadingFile = 1;

    digitalWrite(RESETPIN, HIGH);


    delayMicroseconds(2);
 
    digitalWrite(RESETPIN, LOW);

    showSavedColors(netSlot);
    sendAllPathsCore2 = 1;
    //showLEDsCore2 = 1;
    slotChanged = 0;
    loadingFile = 0;
    // input = ' ';
    //  break;
    //  if (rotaryEncoderMode == 1)
    //  {
    //    goto dontshowmenu;
    //  }
    probeActive = 0;
    break;
  }
  case 'f':

    probeActive = 1;
    readInNodesArduino = 1;
    clearAllNTCC();

    // sendAllPathsCore2 = 1;
    //timer = millis();

    // clearNodeFile(netSlot);

    if (connectFromArduino != '\0')
    {
      serSource = 1;
    }
    else
    {
      serSource = 0;
    }
    savePreformattedNodeFile(serSource, netSlot, rotaryEncoderMode);

    // Serial.print("savePFNF\n\r");
    // debugFP = 1;
    openNodeFile(netSlot);
    getNodesToConnect();
    // Serial.print("openNF\n\r");
    digitalWrite(RESETPIN, HIGH);
    bridgesToPaths();
    clearLEDs();
    assignNetColors();
    delay(1);
    // Serial.print("bridgesToPaths\n\r");
    digitalWrite(RESETPIN, LOW);
    // showNets();
    // saveRawColors(netSlot);
    sendAllPathsCore2 = 1;
    showLEDsCore2 = 1;

    if (debugNMtime)
    {
      Serial.print("\n\n\r");
      Serial.print("took ");
      Serial.print(millis() - timer);
      Serial.print("ms");
    }
    input = ' ';

    probeActive = 0;
    if (connectFromArduino != '\0')
    {
      connectFromArduino = '\0';
      // Serial.print("connectFromArduino\n\r");
      //  delay(2000);
      input = ' ';
      readInNodesArduino = 0;

      goto dontshowmenu;
    }

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
    if (LEDbrightnessMenu() == '!')
    {
      clearLEDs();
      delayMicroseconds(9200);
      sendAllPathsCore2 = 1;
    }
    break;

  case 'r':

    if (rotaryEncoderMode == 1)
    {
      // unInitRotaryEncoder();

      rotaryEncoderMode = 0;
      // createSlots(-1, rotaryEncoderMode);
      //  showSavedColors(netSlot);
      // assignNetColors();

      // showNets();
      lightUpRail();

      showLEDsCore2 = 1;
      debugFlagSet(10); // encoderModeOff
      goto menu;
    }
    else
    {
      rotaryEncoderMode = 1;
      if (rotEncInit == 0) // only do this once
      {
        createSlots(-1, rotaryEncoderMode);
        // initRotaryEncoder();
        rotEncInit = 1;
        // Serial.print("\n\n\r (you should unplug an)");
      }
      printRotaryEncoderHelp();
      delay(100);
      // initRotaryEncoder();
      // refreshSavedColors();
      showSavedColors(netSlot);
      showLEDsCore2 = 1;
      debugFlagSet(11); // encoderModeOn

      //   delay(700);
      //   Serial.flush();
      //   Serial.end();

      //   delay(700);
      //       watchdog_enable(1, 1);
      // while(1);
      //*((volatile uint32_t*)(PPB_BASE + 0x0ED0C)) = 0x5FA0004;
    }
    goto dontshowmenu;

    break;

  case 'u':
    Serial.print("\n\r");
    Serial.print("enter baud rate\n\r");
    while (Serial.available() == 0)
      ;
    baudRate = Serial.parseInt();
    Serial.print("\n\r");
    Serial.print("setting baud rate to ");
    Serial.print(baudRate);
    Serial.print("\n\r");

    setBaudRate(baudRate);
    break;

  case 'd':
  {
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
    if (probeSwap == 0)
    {
      Serial.print("19");
    }
    else
    {
      Serial.print("18");
    }

    Serial.print("\n\n\n\r");

    while (Serial.available() == 0)
      ;

    int toggleDebug = Serial.read();
    Serial.write(toggleDebug);
    toggleDebug -= '0';

    if (toggleDebug >= 0 && toggleDebug <= 9)
    {

      debugFlagSet(toggleDebug);

      delay(10);

      goto debugFlags;
    }
    else
    {
      break;
    }
  }

  case ':':

    if (Serial.read() == ':')
    {
      // Serial.print("\n\r");
      // Serial.print("entering machine mode\n\r");
      machineMode();
      showLEDsCore2 = 1;
      goto dontshowmenu;
      break;
    }
    else
    {
      break;
    }

  default:
    while (Serial.available() > 0)
    {
      int f = Serial.read();
      delayMicroseconds(30);
    }

    break;
  }

  goto menu;
}

unsigned long logoFlashTimer = 0;

int arduinoReset = 0;
unsigned long lastTimeReset = 0;

unsigned long lastSwirlTime = 0;

int swirlCount = 0;
int spread = 7;

int csCycle = 0;
int onOff = 0;
float topRailVoltage = 0.0;
float botRailVoltage = 0.0;


unsigned long schedulerTimer = 0;
unsigned long schedulerUpdateTime = 5;

void loop1() // core 2 handles the LEDs and the CH446Q8
{

  if (millis() - schedulerTimer > schedulerUpdateTime || showLEDsCore2 == 3)
  {
   
    if ((showLEDsCore2 >= 1 && loadingFile == 0) || showLEDsCore2 == 3) {
     // Serial.println(showLEDsCore2);
      int rails = showLEDsCore2; //3 doesn't show nets and keeps control of the LEDs

      if (rails == 1 || rails == 2) {
        lightUpRail(-1, -1, 1);
        logoSwirl(swirlCount, spread);
      }

      if (rails == 5 || rails == 3) {
        logoSwirl(swirlCount, spread);
      }

      if (rails != 2 && rails != 5 && rails != 3) {
        showNets();
      }

      // if (rails == 3) {
      //   Serial.print("\n\r");
      //   Serial.print(rails);
      // }
      delayMicroseconds(2220);

      leds.show();
      
      if (rails != 3) {
        delayMicroseconds(2200);
        showLEDsCore2 = 0;
      }
      

    } else if (sendAllPathsCore2 == 1) {
      //leds.show();
      delayMicroseconds(4200);
      sendAllPaths();
      delayMicroseconds(2200);
      //showNets();
      //leds.show();
      //delayMicroseconds(7200);
      //showLEDsCore2 = 1;
      sendAllPathsCore2 = 0;

    } else if (millis() - lastSwirlTime > 330 && loadingFile == 0 &&
               showLEDsCore2 == 0) {


      logoSwirl(swirlCount, spread);

      lastSwirlTime = millis();

      if (swirlCount >= 41) {
        swirlCount = 0;

      } else {

        swirlCount++;
      }
      showLEDsCore2 = 2;
      // leds.show();
    } else {

      rotaryEncoderStuff();
    }
    schedulerTimer = millis();
  } 

  

}




// if (readInNodesArduino == 0)
// {

//   if (USBSer1.available())
//   {

//     char ch = USBSer1.read();
//     Serial1.write(ch);
//     // Serial.print(ch);
//   }

//   if (Serial1.available())
//   {
//     char ch = Serial1.read();
//     USBSer1.write(ch);
//     // Serial.print(ch);

//     if (ch == 'f' && connectFromArduino == '\0')
//     {
//       input = 'f';

//       connectFromArduino = 'f';
//       // Serial.print("!!!!");
//     }
//     else
//     {
//       // connectFromArduino = '\0';
//     }
//   }
// }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// void lastNetConfirm(int forceLastNet)
// {
//   // while (tud_connected() == 0 && millis() < 500)
//   //   ;

//   // if (millis() - lastNetConfirmTimer < 3000 && tud_connected() == 1)
//   // {
//   //   // Serial.println(lastNetConfirmTimer);

//   //   // lastNetConfirmTimer = millis();
//   //   return;
//   // }

//   if (forceLastNet == 1)
//   {

//     int bootselPressed = 0;
//     openNodeFile();
//     getNodesToConnect();
//     // Serial.print("openNF\n\r");
//     digitalWrite(RESETPIN, HIGH);
//     bridgesToPaths();
//     clearLEDs();
//     assignNetColors();

//     sendAllPathsCore2 = 1;
//     Serial.print("\n\r   short press BOOTSEL to restore last netlist\n\r");
//     Serial.print("   long press to cancel\n\r");
//     delay(250);
//     if (BOOTSEL)
//     {
//       bootselPressed = 1;
//     }

//     while (forceLastNet == 1)
//     {
//       if (BOOTSEL)
//         bootselPressed = 1;

//       // clearLEDs();
//       // leds.show();
//       leds.clear();
//       lightUpRail(-1, -1, 1, 28, supplySwitchPosition);
//       leds.show();
//       // showLEDsCore2 = 1;

//       if (BOOTSEL)
//         bootselPressed = 1;

//       delay(250);

//       // showLEDsCore2 = 2;
//       sendAllPathsCore2 = 1;
//       // Serial.print("p\n\r");
//       if (BOOTSEL)
//         bootselPressed = 1;
//       // delay(250);

//       if (bootselPressed == 1)
//       {
//         unsigned long longPressTimer = millis();
//         int fade = 8;
//         while (BOOTSEL)
//         {

//           sendAllPathsCore2 = 1;
//           showLEDsCore2 = 2;
//           delay(250);
//           clearLEDs();
//           // leds.clear();
//           showLEDsCore2 = 2;

//           if (fade <= 0)
//           {
//             clearAllNTCC();
//             clearLEDs();
//             startupColors();
//             // clearNodeFile();
//             sendAllPathsCore2 = 1;
//             lastNetConfirmTimer = millis();
//             restoredNodeFile = 0;
//             // delay(1000);
//             Serial.print("\n\r   cancelled\n\r");
//             return;
//           }

//           delay(fade * 10);
//           fade--;
//         }

//         digitalWrite(RESETPIN, LOW);
//         restoredNodeFile = 1;
//         sendAllPathsCore2 = 1;
//         Serial.print("\n\r   restoring last netlist\n\r");
//         printNodeFile();
//         return;
//       }
//       delay(250);
//     }
//   }
// }
unsigned long lastTimeNetlistLoaded = 0;
unsigned long lastTimeCommandRecieved = 0;

void machineMode(void) // read in commands in machine readable format
{
  int sequenceNumber = -1;

  lastTimeCommandRecieved = millis();

  if (millis() - lastTimeCommandRecieved > 100)
  {
    machineModeRespond(sequenceNumber, true);
    return;
  }
  enum machineModeInstruction receivedInstruction = parseMachineInstructions(&sequenceNumber);

  // Serial.print("receivedInstruction: ");
  // Serial.print(receivedInstruction);
  // Serial.print("\n\r");

  switch (receivedInstruction)
  {
  case netlist:
    lastTimeNetlistLoaded = millis();
    clearAllNTCC();

    // writeNodeFileFromInputBuffer();

    digitalWrite(RESETPIN, HIGH);

    machineNetlistToNetstruct();
    populateBridgesFromNodes();
    bridgesToPaths();

    clearLEDs();
    assignNetColors();
    // showNets();
    digitalWrite(RESETPIN, LOW);
    sendAllPathsCore2 = 1;
    break;

  case getnetlist:
    if (millis() - lastTimeNetlistLoaded > 300)
    {

      listNetsMachine();
    }
    else
    {
      machineModeRespond(0, true);
      // Serial.print ("too soon bro\n\r");
      return;
    }
    break;

  case bridgelist:
    clearAllNTCC();

    writeNodeFileFromInputBuffer();

    openNodeFile();
    getNodesToConnect();
    // Serial.print("openNF\n\r");
    digitalWrite(RESETPIN, HIGH);
    bridgesToPaths();
    clearLEDs();
    assignNetColors();
    // Serial.print("bridgesToPaths\n\r");
    digitalWrite(RESETPIN, LOW);
    // showNets();

    sendAllPathsCore2 = 1;
    break;

  case getbridgelist:
    listBridgesMachine();
    break;

  case lightnode:
    lightUpNodesFromInputBuffer();
    break;

  case lightnet:
    lightUpNetsFromInputBuffer();
    //   lightUpNet();
    // assignNetColors();
    // showLEDsCore2 = 1;
    break;

    // case getmeasurement:
    //   showMeasurements();
    //   break;

  case setsupplyswitch:

    supplySwitchPosition = setSupplySwitch();
    // printSupplySwitch(supplySwitchPosition);
    machineModeRespond(sequenceNumber, true);

    showLEDsCore2 = 1;
    break;

  case getsupplyswitch:
    // if (millis() - lastTimeNetlistLoaded > 100)
    //{

    printSupplySwitch(supplySwitchPosition);
    // machineModeRespond(sequenceNumber, true);

    // }else {
    // Serial.print ("\n\rtoo soon bro\n\r");
    // machineModeRespond(0, true);
    // return;
    // }
    break;

  case getchipstatus:
    printChipStatusMachine();
    break;

    // case gpio:
    //   break;
  case getunconnectedpaths:
    getUnconnectedPaths();
    break;

  case unknown:
    machineModeRespond(sequenceNumber, false);
    return;
  }

  machineModeRespond(sequenceNumber, true);
}