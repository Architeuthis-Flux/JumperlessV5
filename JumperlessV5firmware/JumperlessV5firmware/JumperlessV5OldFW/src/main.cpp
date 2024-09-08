// SPDX-License-Identifier: MIT
/*
Kevin Santo Cappuccio
Architeuthis Flux

KevinC@ppucc.io

5/28/2024

*/
#include <Arduino.h>

#define USE_TINYUSB 1

#define LED LED_BUILTIN

#ifdef USE_TINYUSB
#include <Adafruit_TinyUSB.h>
#endif

#ifdef CFG_TUSB_CONFIG_FILE
#include CFG_TUSB_CONFIG_FILE
#else
#include "tusb_config.h"
#endif

#include "ArduinoStuff.h"
#include "JumperlessDefinesRP2040.h"
#include "NetManager.h"
#include "MatrixStateRP2040.h"

#include "NetsToChipConnections.h"
#include "LEDs.h"
// #include "CH446Q.h"
#include "Peripherals.h"
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "MachineCommands.h"
// #include <EEPROM.h>

#include <EEPROM.h>

#include "CH446Q.h"
#include "RotaryEncoder.h"
#include "FileParsing.h"

#include "LittleFS.h"

#include "Probing.h"

#include "quadrature.pio.h"

PIO pioEnc = pio1;
uint offsetEnc, smEnc;

// #include "AdcUsb.h"

// #include "logic_analyzer.h"
// #include "pico/multicore.h"
// #include "hardware/watchdog.h"

// using namespace logic_analyzer;

// int pinStart=26;
// int numberOfPins=2;
// LogicAnalyzer logicAnalyzer;
// Capture capture(MAX_FREQ, MAX_FREQ_THRESHOLD);

Adafruit_USBD_CDC USBSer1;

int supplySwitchPosition = 0;

void machineMode(void);
void lastNetConfirm(int forceLastNet = 0);
void rotaryEncoderStuff(void);

unsigned long lastNetConfirmTimer = 0;
// int machineMode = 0;

volatile int sendAllPathsCore2 = 0; // this signals the core 2 to send all the paths to the CH446Q

// https://wokwi.com/projects/367384677537829889

// void activateTestSignal(int testPin, float dutyCyclePercent) {
//     log("Starting PWM test signal with duty %f %", dutyCyclePercent);
//     pinMode(testPin, OUTPUT);
//     int value = dutyCyclePercent / 100.0 * 255.0;
//     analogWrite(testPin, value);
// }

// void captureHandler(){
//     // we use the led to indicate the capturing
//    // pinMode(LED_BUILTIN, OUTPUT);

//     while(true){
//         if (logicAnalyzer.status() == ARMED){
//             // start capture
//             //digitalWrite(LED_BUILTIN, HIGH);
//             logicAnalyzer.capture();
//            // digitalWrite(LED_BUILTIN, LOW);
//         }
//         // feed the dog
//         watchdog_update();
//     }
// }

void setup()
{
  pinMode(RESETPIN, OUTPUT_12MA);

  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(4);
  digitalWrite(RESETPIN, LOW);
  pinMode(0, OUTPUT);
  pinMode(2, INPUT);
  pinMode(3, INPUT);

  // debugFlagInit();
  USBDevice.setProductDescriptor("Jumperless");
  USBDevice.setManufacturerDescriptor("Architeuthis Flux");
  USBDevice.setSerialDescriptor("0");
  USBDevice.setID(0x1D50, 0xACAB);
  USBDevice.addStringDescriptor("Jumperless");
  USBDevice.addStringDescriptor("Architeuthis Flux");

  USBSer1.setStringDescriptor("Jumperless USB Serial");

  USBSer1.begin(115200);

  // encoder.begin();

#ifdef EEPROMSTUFF
  EEPROM.begin(256);
  debugFlagInit();

#endif
  // delay(1);

  Serial.begin(115200);
  // initADC();
  delay(100);
  initGPIOex();
  initDAC(); // also sets revisionNumber
  delay(1);
  initINA219();
  delay(1);

  initArduino();
  delay(4);
#ifdef FSSTUFF
  LittleFS.begin();
#endif
  setDac0_5Vvoltage(0.0);
  setDac1_8Vvoltage(1.9);

  clearAllNTCC();

  // delay(20);
  // setupAdcUsbStuff(); // I took this out because it was causing a crash on
  delay(10);
}

void setup1()
{
  delay(4);

  initCH446Q();

  delay(4);
  initLEDs();
  delay(4);
  // startupColors();
  delay(4);
  // lightUpRail();

  delay(4);

  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);

  pinMode(BUTTON_ENC, INPUT);

  offsetEnc = pio_add_program(pioEnc, &quadrature_program);
  smEnc = pio_claim_unused_sm(pioEnc, true);
  quadrature_program_init(pioEnc, smEnc, offsetEnc, QUADRATURE_A_PIN, QUADRATURE_B_PIN);

  showLEDsCore2 = 1;

  // logicAnalyzer.setEventHandler(&onEvent);
  //   logicAnalyzer.setDescription("JumpLogic");
  //   logicAnalyzer.setCaptureOnArm(true);
  //   //logicAnalyzer.setTriggerMask(0x11111111);
  //  // activateTestSignal(26, 90.0);

  // logicAnalyzer.begin(USBSer1, &capture, MAX_CAPTURE_SIZE, pinStart, numberOfPins);

  // multicore_launch_core1(captureHandler);
  //  lastNetConfirm(0);
}

char connectFromArduino = '\0';

char input;

int serSource = 0;
int readInNodesArduino = 0;
int baudRate = 115200;

int restoredNodeFile = 0;

const char firmwareVersion[] = "1.3.11"; //// remember to update this

void loop()
{

  unsigned long timer = 0;


// sendXYraw(10, 4, 0, 1);
// sendXYraw(0, 9, 1, 1);

delay(1000);

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
//   while (1)
//   {
//    // randomColors(0, 30);
//     // delay(5000);
//     //    for (int i = 0; i < 12; i++)
//     //    {
//     //      setCSex(i, 1);
//     //      delay(10);
//     //      setCSex(i, 0);

//     //   // writeGPIOex(1, 0);
//     //   // delayMicroseconds(10000);
//     //   // writeGPIOex(0, 0);
//     // }
//     rainbowBounce(1);
//    //delay(1000);
//  }
// sendXYraw(10, 6, 0, 1);
// sendXYraw(10, 0, 0, 1);

// sendXYraw(8, 0, 0, 1);
// sendXYraw(8, 1, 0, 1);

// sendXYraw(10, 6, 0, 1);
// sendXYraw(0, 9, 1, 1);

delay(1000);
menu:
  // showMeasurements();
  //   unsigned long connecttimer = 0;
  // //   while (tud_connected() == 0)
  // //   {
  // // connecttimer = millis();

  // //   }
  // Serial.print("Updated!\n\r");

  Serial.print("\n\n\r\t\t\tMenu\n\n\r");
  Serial.print("\tn = show netlist\n\r");
  Serial.print("\tb = show bridge array\n\r");
  Serial.print("\ts = show node file\n\r");
  Serial.print("\tf = load formatted node file\n\r");
  Serial.print("\tw = waveGen\n\r");
  Serial.print("\tv = toggle show current/voltage\n\r");
  Serial.print("\tu = set baud rate for USB-Serial\n\r");
  Serial.print("\tl = LED brightness / test\n\r");
  Serial.print("\td = toggle debug flags\n\r");
  Serial.print("\tr = reset Arduino\n\r");
  Serial.print("\tp = probe connections\n\r");
  Serial.print("\tc = clear nodes with probe\n\r");

  // if (1)
  // {
  //   for (int i = 0; i < 4; i++)
  //   {
  //     Serial.print("\n\r");
  //     Serial.print("PIO_0 State Machine ");
  //     Serial.print(i);
  //     Serial.print(" ");
  //     Serial.print(pio_sm_is_claimed(pio0, i));
  //     Serial.print(" ");
  //   }
  //   for (int i = 0; i < 4; i++)
  //   {
  //     Serial.print("\n\r");
  //     Serial.print("PIO_1 State Machine ");
  //     Serial.print(i);
  //     Serial.print(" ");
  //     Serial.print(pio_sm_is_claimed(pio1, i));
  //   }
  //   Serial.print("\n\r");
//}
dontshowmenu:
  connectFromArduino = '\0';

  while (Serial.available() == 0 && connectFromArduino == '\0')
  {
    if (showReadings >= 1)
    {
      showMeasurements();
      // Serial.print("\n\n\r");
      // showLEDsCore2 = 1;
    }
    if (BOOTSEL)
    {
      lastNetConfirm(1);
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
    // }

    // pinMode(19, INPUT);
    //}
  }

  if (connectFromArduino != '\0')
  {
    input = 'f';
    // connectFromArduino = '\0';
  }
  else
  {
    input = Serial.read();
    Serial.print("\n\r");
  }

// Serial.print(input);
skipinput:
  switch (input)
  {
  case '?':
  {
    Serial.print("Jumperless firmware version: ");
    Serial.println(firmwareVersion);
    break;
  }

  case 's':
    Serial.print("\n\n\r");
    Serial.print("\tNode File\n\r");
    Serial.print("\n\ryou can paste this into the menu to reload this circuit");
    Serial.print("\n\r(make sure you grab an extra blank line at the end)\n\r");
    Serial.print("\n\n\rf ");
    printNodeFile();
    Serial.print("\n\n\r");
    break;

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

      // Serial.write("\033"); //these VT100/ANSI commands work on some terminals and not others so I took it out
      // Serial.write("\x1B\x5B");
      // Serial.write("1F");//scroll up one line
      // Serial.write("\x1B\x5B");
      // Serial.write("\033");
      // Serial.write("2K");//clear line
      // Serial.write("\033");
      // Serial.write("\x1B\x5B");
      // Serial.write("1F");//scroll up one line
      // Serial.write("\x1B\x5B");
      // Serial.write("\033");
      // Serial.write("2K");//clear line

      goto dontshowmenu;
      // break;
    }
  case 'p':
  {
    probeMode(19, 1);
    break;
  }
  case 'c':
  {
    // removeBridgeFromNodeFile(19, 1);
    probeMode(19, 0);
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

    // case 'a':
    // {
    //   resetArduino(); // reset works
    //   // uploadArduino(); //this is unwritten
    // }

  case 'f':
    readInNodesArduino = 1;
    clearAllNTCC();

    // sendAllPathsCore2 = 1;
    timer = millis();

    clearNodeFile();

    if (connectFromArduino != '\0')
    {
      serSource = 1;
    }
    else
    {
      serSource = 0;
    }

    savePreformattedNodeFile(serSource);

    // Serial.print("savePFNF\n\r");
    // debugFP = 1;
    openNodeFile();
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

    sendAllPathsCore2 = 1;

    if (debugNMtime)
    {
      Serial.print("\n\n\r");
      Serial.print("took ");
      Serial.print(millis() - timer);
      Serial.print("ms");
    }

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

    resetArduino();

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

// #include <string> // Include the necessary header file

void lastNetConfirm(int forceLastNet)
{
  // while (tud_connected() == 0 && millis() < 500)
  //   ;

  // if (millis() - lastNetConfirmTimer < 3000 && tud_connected() == 1)
  // {
  //   // Serial.println(lastNetConfirmTimer);

  //   // lastNetConfirmTimer = millis();
  //   return;
  // }

  if (forceLastNet == 1)
  {

    int bootselPressed = 0;
    openNodeFile();
    getNodesToConnect();
    // Serial.print("openNF\n\r");
    digitalWrite(RESETPIN, HIGH);
    bridgesToPaths();
    clearLEDs();
    assignNetColors();

    sendAllPathsCore2 = 1;
    Serial.print("\n\r   short press BOOTSEL to restore last netlist\n\r");
    Serial.print("   long press to cancel\n\r");
    delay(250);
    if (BOOTSEL)
    {
      bootselPressed = 1;
    }

    while (forceLastNet == 1)
    {
      if (BOOTSEL)
        bootselPressed = 1;

      // clearLEDs();
      // leds.show();
      leds.clear();
      lightUpRail(-1, -1, 1, 28, supplySwitchPosition);
      leds.show();
      // showLEDsCore2 = 1;

      if (BOOTSEL)
        bootselPressed = 1;

      delay(250);

      // showLEDsCore2 = 2;
      sendAllPathsCore2 = 1;
      // Serial.print("p\n\r");
      if (BOOTSEL)
        bootselPressed = 1;
      // delay(250);

      if (bootselPressed == 1)
      {
        unsigned long longPressTimer = millis();
        int fade = 8;
        while (BOOTSEL)
        {

          sendAllPathsCore2 = 1;
          showLEDsCore2 = 2;
          delay(250);
          clearLEDs();
          // leds.clear();
          showLEDsCore2 = 2;

          if (fade <= 0)
          {
            clearAllNTCC();
            clearLEDs();
            startupColors();
            // clearNodeFile();
            sendAllPathsCore2 = 1;
            lastNetConfirmTimer = millis();
            restoredNodeFile = 0;
            // delay(1000);
            Serial.print("\n\r   cancelled\n\r");
            return;
          }

          delay(fade * 10);
          fade--;
        }

        digitalWrite(RESETPIN, LOW);
        restoredNodeFile = 1;
        sendAllPathsCore2 = 1;
        Serial.print("\n\r   restoring last netlist\n\r");
        printNodeFile();
        return;
      }
      delay(250);
    }
  }
}
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
int pressed = 0;

volatile int encoder_value = 0;
int encoderOffset = 0;
int encoderRaw = 0;
int lastPosition = 0;
int position = 0;
unsigned long lastSwirlTime = 0;

void pressedEncoder()
{
  // holdButtonFirstPressed = millis();
  Serial.println("Pressed!");
  pressed = 1;
  // click();
}

void encoderISR()
{
  Serial.print("Encoder: ");
  Serial.print(encoder_value);
  Serial.print("\n\r");
}

unsigned long logoFlashTimer = 0;

int arduinoReset = 0;
unsigned long lastTimeReset = 0;

int newPositionEncoder = 0;
int lastPositionEncoder = 0;

int swirlCount = 0;
int spread = 7;

int scanPosition = 0;
int rowCount = 0;
int scanPosition2 = 2;
int rowCount2 = 2;

int topRailValue = 1650;
void loop1() // core 2 handles the LEDs and the CH446Q8
{

  // while (1) rainbowBounce(50); //I uncomment this to test the LEDs on a fresh board
  if (showLEDsCore2 >= 1)
  {
    int rails = showLEDsCore2;

    if (rails == 1 || rails == 2)
    {
      lightUpRail(-1, -1, 1, 28, supplySwitchPosition);
    }

    if (rails != 2)
    {
      showNets();
    }

    if (rails > 3)
    {
      Serial.print("\n\r");
      Serial.print(rails);
    }
    delayMicroseconds(2200);

    leds.show();
    delayMicroseconds(5200);
    showLEDsCore2 = 0;
  }

  if (sendAllPathsCore2 == 1)
  // if (0)
  {
    delayMicroseconds(6200);
    sendAllPaths();
    delayMicroseconds(2200);
    showNets();
    delayMicroseconds(7200);
    sendAllPathsCore2 = 0;
  }
  // } else if (USBSer1.available() > 0)
  // {
  //   logicAnalyzer.processCommand();
  // }
  if (millis() - lastSwirlTime > 180)
  {

    logoSwirl(swirlCount, spread);

    lastSwirlTime = millis();

    if (swirlCount >= 41)
    {
      swirlCount = 0;
    }
    else
    {

      swirlCount++;
    }

    leds.show();
  }

  // if (arduinoReset == 0 && USBSer1.peek() == 0x30) // 0x30 is the first thing AVRDUDE sends
  // {
  //   resetArduino();
  //   arduinoReset = 1;
  //   lastTimeReset = millis();
  // }

  // // if (digitalRead(RP_GPIO_0) == 1)
  // // {
  // //   setDac0_5Vvoltage(5.0);
  // // } else
  // // {
  // //   setDac0_5Vvoltage(0.0);
  // // }

  // while (arduinoReset == 1)
  // {
  //   if (USBSer1.available())
  //   {

  //     char ch = USBSer1.read();
  //     Serial1.write(ch);
  //   }

  //   if (Serial1.available())
  //   {
  //     char ch = Serial1.read();
  //     USBSer1.write(ch);
  //   }

  //   if (millis() - lastTimeReset > 4000) // if the arduino hasn't been reset in a second, reset the flag
  //   {
  //     arduinoReset = 0;
  //   }
  // }

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

  if (logoFlash == 2)
  {
    logoFlashTimer = millis();
    logoFlash = 1;
  }

  if (logoFlash == 1 && logoFlashTimer != 0 && millis() - logoFlashTimer > 150)
  {
    logoFlash = 0;
    logoFlashTimer = 0;
    // lightUpRail();
    // leds.setPixelColor(110, rawOtherColors[1]);
    // leds.show();
  }

  rotaryEncoderStuff();
  //  logicAnalyzer.processCommand();
}

int pressedRows[64];
int pressedRowsIndex = 0;

float topRailVoltage = 0.0;
float botRailVoltage = 0.0;

void rotaryEncoderStuff(void)
{
  pio_sm_exec(pioEnc, smEnc, pio_encode_in(pio_x, 32)); // PIO rotary encoder handler

  encoderRaw = (pio_sm_get(pioEnc, smEnc));
  if (lastPositionEncoder != encoderRaw)
  {
    newPositionEncoder = encoderRaw;

    if (lastPositionEncoder > encoderRaw)
    {

      if (position <= 400)
      {
        position -= 5;
      }
      else
      {

        position -= 1;
      }
      if (position < 0)
      {
        position = 0;
      }
      

      //if (botRailVoltage < 9.0)
     // {
        botRailVoltage += 0.3;
        topRailVoltage += 0.3;
     // }
      lastPositionEncoder = encoderRaw;
    }
    else if (lastPositionEncoder < encoderRaw)
    {
      if (position <= 400)
      {
        position += 5;
      }
      else
      {

        position += 1;
      }
      if (position > 500)
      {
        position = 500;
      }

      // if (topRailVoltage > -9.0)
      // {
        topRailVoltage -= 0.3;
        botRailVoltage -= 0.3;
      //}


      lastPositionEncoder = encoderRaw;
    }
    
  }
  if (lastPosition != position)
  {
    /// if (pressedRows[pressedRowsIndex - 1] == lastPosition)
    // {
    //}
    // else
    //{
    // leds.setPixelColor(lastPosition, 0x000000);

    // setTopRail(((position * 8)) % 4095);
    // setBotRail(((position * 8) + 1600) % 4095);
    // Serial.print("Rails: ");
    // Serial.println(((position * 8)) % 4095);

    //setTopRail(topRailVoltage);
    setBotRail(botRailVoltage);
    Serial.print("Top Rail: ");
    Serial.println(topRailVoltage);

    if (lastPosition <= 400)
    {
      leds.setPixelColor(lastPosition + 0, 0x000000);
      leds.setPixelColor(lastPosition + 1, 0x000000);
      leds.setPixelColor(lastPosition + 2, 0x000000);
      leds.setPixelColor(lastPosition + 3, 0x000000);
      leds.setPixelColor(lastPosition + 4, 0x000000);
    }
    // }
    // lightUpNode(newPositionEncoder, 0x000000);
    lastPositionEncoder = newPositionEncoder;
    // Serial.print("Encoder: ");
    // Serial.print(position);
    // Serial.println(" ");
    lastPosition = position;

    if (position < 400)
    {
      int lightUpWholeRow = position;
      leds.setPixelColor(lightUpWholeRow,0x060006);
       leds.setPixelColor(lightUpWholeRow + 1, 0x060006);
       leds.setPixelColor(lightUpWholeRow + 2, 0x060006);
       leds.setPixelColor(lightUpWholeRow + 3, 0x060006);
       leds.setPixelColor(lightUpWholeRow + 4, 0x060006);
    }
    else
    {

      // leds.setPixelColor(position, 0x120012);
    }
    // lightUpNode(position, 0x330033);
    //  delay(100);
    //  encoderISR();

    // spread = abs((position / 5)) % 41;
    //  Serial.print("Spread: ");
    //  Serial.print(spread);
    showLEDsCore2 = 1;
    // delayMicroseconds(100);
  }

  // if (pressed == 1)
  //  pressedEncoder();

  int buttonState = digitalRead(BUTTON_ENC);

  if (buttonState == 0)
  {
    // pressedRows[pressedRowsIndex] = position;
    // pressedRowsIndex++;
    position++;
    delay(200);

    if (position < 400)
    {
      leds.setPixelColor(position, 0x030813);
      // leds.setPixelColor(position + 1, 0x030813);
      // leds.setPixelColor(position + 2, 0x030813);
      // leds.setPixelColor(position + 3, 0x030813);
      // leds.setPixelColor(position + 4, 0x030813);
    }
    else
    {
      leds.setPixelColor(position, 0x131823);
    }
    // Serial.println(spread);

    // lightUpNode(position, 0x330033);
    //  delay(100);
    //  encoderISR();
    // showLEDsCore2 = 1;
    // delay(100);
    // click();
    // click();
    // click();
    // pressed = 1;
    // Serial.println("Pressed!");
  }
}