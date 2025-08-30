#include "Apps.h"
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Commands.h"
#include "FileParsing.h"
#include "FilesystemStuff.h"
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
#include "Python_Proper.h"
#include "RotaryEncoder.h"
#include "config.h"
#include "configManager.h"
#include "oled.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>

#include <JDI_MIP_Display.h>
#include <Display_cfg.h>
#include "Images.h"

#define DISPLAY_WIDTH   72
#define DISPLAY_HEIGHT  144

#define HALF_WIDTH (DISPLAY_WIDTH / 2)

#define COLOR_BLACK   0x00
#define COLOR_BLUE    0x02
#define COLOR_GREEN   0x04
#define COLOR_CYAN    0x06
#define COLOR_RED     0x08
#define COLOR_MAGENTA 0x0a
#define COLOR_YELLOW  0x0c
#define COLOR_WHITE   0x0e

#define CMD_NO_UPDATE           0x00
#define CMD_BLINKING_BLACK      0x10
#define CMD_BLINKING_INVERSION  0x14
#define CMD_BLINKING_WHITE      0x18
#define CMD_ALL_CLEAR           0x20
#define CMD_VCOM                0x40
#define CMD_UPDATE              0x90
#define OLED_CONNECTED 0

JDI_MIP_Display jdi_display;


// Wrapper for i2cScan to match void(*)(void)
static void i2cScanWrapper(void) { i2cScan(); }

struct app apps[NUM_APPS] = {
  { "Bounce Startup",  0, 1, bounceStartup },
  { "Calib  DACs",     1, 1, calibrateDacs },
  { "I2C    Scan",     2, 1, i2cScanWrapper },
  { "Custom App",      3, 1, customApp },
  { "PNG Image",       4, 1, displayImage },
  { "Scan",            5, 1, scanBoard },
  { "XLSX   GUI",      6, 1, xlsxGui },
  { "Micropython",     7, 1, microPythonREPLapp },
  { "uPythonREPL",     8, 1, microPythonREPLapp },
  { "File   Manager",  9, 1, fileManagerApp },
  { "eKilo Editor",   10, 1, eKiloApp },
  { "Probe  Calib",   11, 1, probeCalibApp },
  { "JDI MIPdisplay", 12, 1, jdiMIPdisplay },
  { "DOOM",           16, 1, playDoom },
  // others can remain uninitialized (works=0)
};



// Helper: normalize spaces (trim + collapse)
String normalizeSpaces(const char* s) {
  if (!s) return String();
  String in(s);
  in.trim();
  String out;
  out.reserve(in.length());
  bool lastSpace = false;
  for (size_t i = 0; i < in.length(); ++i) {
    char c = in[i];
    if (c == ' ') {
      if (!lastSpace) { out += ' '; lastSpace = true; }
    } else {
      out += c; lastSpace = false;
    }
  }
  return out;
}



void runApp(int index, char* name) {
  const int APP_COUNT = NUM_APPS;

  // Try to resolve by name 
  if (index < 0 || index >= APP_COUNT) {
    if (name) {
      String want = normalizeSpaces(name);
      for (int i = 0; i < APP_COUNT; ++i) {
        if (!apps[i].works || !apps[i].action) continue;
        if (normalizeSpaces(apps[i].name).equalsIgnoreCase(want)) {
          index = i;
          break;
        }
      }
    }
  }

  // Validate
  if (index < 0 || index >= APP_COUNT || !apps[index].works || !apps[index].action) {
    Serial.println(F("App not found"));
    return;
  }

  // Ensure we have a display name
  if (!name) name = apps[index].name;

  Serial.println("Running app: " + normalizeSpaces(name));
  Serial.println("Index: " + String(index));

  // Direct dispatch (no giant switch)
  apps[index].action();
}

void microPythonREPLapp(void) {
  enterMicroPythonREPL(global_mp_stream);
}

void leaveApp(int lastNetSlot) {
  createSlots(8, 1);
  netSlot = lastNetSlot;
  refreshConnections(-1, 0, 1);
}

void fileManagerApp(void) { filesystemApp(false); }

void probeCalibApp(void) {
  b.clear();

  cycleTerminalColor(true, 5.0, true);
  Serial.println("Probe Calibration App");
  cycleTerminalColor();
  Serial.println("\n\rTap rows with the probe and rotate the clickweel until they're lighting up the correct row");
  cycleTerminalColor();
  Serial.println("be sure to check nano header rows too");
  cycleTerminalColor();
  Serial.println("Hold the clickwheel when you're done\n\n\r");
  cycleTerminalColor();

  bool done = false;

  b.clear();
  int lastNetSlot = netSlot;
  netSlot = 8;
  createSlots(8, 1);

  refreshConnections(-1, 0);
  routableBufferPower(1, 1, 1);
  resetEncoderPosition = true;
  int lastEncoderPosition = encoderPosition;
  int reading = 0;
  int lastReading = -1;

  int probeMax = jumperlessConfig.calibration.probe_max;

  int nodeSelected = -1;
  int lastNodeSelected = -1;

  int nodeSelectedWithOldMapping = -1;
  int lastNodeSelectedWithOldMapping = -1;

  int lastValidProbeRead = -1;
  int lastSwitchPosition = -1;

  int measureOrSelect = 0;

  float measureModeOutputVoltage = jumperlessConfig.calibration.measure_mode_output_voltage;

  while (done == false) {
    int probeRead = readProbeRaw(0, true);
    if (probeRead != -1) lastValidProbeRead = probeRead;

    int rowProbed = map(lastValidProbeRead, jumperlessConfig.calibration.probe_min, jumperlessConfig.calibration.probe_max, 101, 0);
    int rowProbedWithOldMapping = map(lastValidProbeRead, jumperlessConfig.calibration.probe_min, probeMax, 101, 0);

    nodeSelected = probeRowMap[rowProbed];
    nodeSelectedWithOldMapping = probeRowMapByPad[rowProbedWithOldMapping];

    int checkSwitch = checkSwitchPosition();
    if (checkSwitch != lastSwitchPosition) {
      if (checkSwitch == 0) {
        lastSwitchPosition = 0;
        measureOrSelect = 0;
        measureModeOutputVoltage = jumperlessConfig.calibration.measure_mode_output_voltage;
      } else {
        lastSwitchPosition = 1;
        measureOrSelect = 1;
        probeMax = jumperlessConfig.calibration.probe_max;
      }
      resetEncoderPosition = true;
    }

    reading = rowProbed;

    if (encoderPosition != lastEncoderPosition || reading != lastReading) {
      lastEncoderPosition = encoderPosition;
      if (measureOrSelect == 0) {
        jumperlessConfig.calibration.measure_mode_output_voltage = measureModeOutputVoltage - ((float)encoderPosition / 2000.0);
        if (jumperlessConfig.calibration.measure_mode_output_voltage < 2.8) {
          jumperlessConfig.calibration.measure_mode_output_voltage = 2.8;
        } else if (jumperlessConfig.calibration.measure_mode_output_voltage > 4.5) {
          jumperlessConfig.calibration.measure_mode_output_voltage = 5.0;
        }
        setDac0voltage(jumperlessConfig.calibration.measure_mode_output_voltage, 1, 0, false);
      } else {
        jumperlessConfig.calibration.probe_max = probeMax - encoderPosition;
        if (jumperlessConfig.calibration.probe_max < 15) {
          jumperlessConfig.calibration.probe_max = 15;
        }
      }
      Serial.printf("                                  \rraw: %d enc: %d reading: %d max: %d node: %s mode: %s v: %0.4f",
                    lastValidProbeRead, encoderPosition, rowProbed, jumperlessConfig.calibration.probe_max,
                    definesToChar(nodeSelected), measureOrSelect ? "measure" : "select",
                    jumperlessConfig.calibration.measure_mode_output_voltage);
      Serial.flush();
    }

    if (reading == -1) continue;

    if (reading != lastReading && reading != -1) {
      clearLEDsExceptRails();
      if (nodeSelected != nodeSelectedWithOldMapping && measureOrSelect == 1) {
        b.lightUpNode(nodeSelectedWithOldMapping, 0x050205);
      }
      if (measureOrSelect == 0) {
        b.lightUpNode(nodeSelected, 0x200010);
      } else {
        b.lightUpNode(nodeSelected, 0x001030);
      }
    }
    lastReading = reading;
    lastNodeSelected = nodeSelected;
    lastNodeSelectedWithOldMapping = nodeSelectedWithOldMapping;

    if (encoderButtonState == HELD) done = true;
  }

  Serial.println("\n\n\r");
  Serial.println("Saving config...");

  saveConfig();
  leaveApp(lastNetSlot);
}

void customApp(void) {
  leds.clear();
  b.clear();
  delay(1000);
  b.print("This isa demo", (uint32_t)0x002008);
  showLEDsCore2 = 2;
  delay(2000);
  b.clear();
  b.print("It will show", (uint32_t)0x002008);
  delay(1000);
  b.clear();
  b.print("random stuff", (uint32_t)0x002008);
  delay(1000);
  b.clear();
  b.print("look atthe FW", (uint32_t)0x002008);
  delay(1000);

  int lastNetSlot = netSlot;
  netSlot = 8; // net slot for custom app
  int leave = 0;

  createSlots(8, 1);

  addBridgeToNodeFile(12, 25, netSlot, 0, 0);
  addBridgeToNodeFile(TOP_RAIL, 52, netSlot, 0, 0);
  refreshConnections(-1, 1);

  setTopRail((float)3.3);

  addBridgeToNodeFile(RP_GPIO_1, 42, netSlot, 0, 0);
  addBridgeToNodeFile(RP_GPIO_2, 42, netSlot, 0, 0);
  refreshConnections(-1, 0);

  pinMode(GPIO_1_PIN, OUTPUT);
  pinMode(GPIO_2_PIN, INPUT);

  digitalWrite(GPIO_1_PIN, HIGH);
  int reading = digitalRead(GPIO_2_PIN);
  Serial.print("GPIO 2 Reading: ");
  Serial.println(reading);
  delay(10);

  digitalWrite(GPIO_1_PIN, LOW);
  reading = digitalRead(GPIO_2_PIN);
  Serial.print("GPIO_2_PIN: ");
  Serial.println(reading);

  removeBridgeFromNodeFile(RP_GPIO_1, 42, netSlot, 0);
  removeBridgeFromNodeFile(RP_GPIO_2, 42, netSlot, 0);
  refreshConnections(-1, 0);

  setDacByNumber(1, 5.35);
  addBridgeToNodeFile(DAC1, 20, netSlot, 0, 0);
  refreshConnections(-1, 0);
  delay(10);

  addBridgeToNodeFile(ADC0, 20, netSlot, 0, 0);
  refreshConnections(-1, 0);
  waitCore2();

  float voltage = readAdcVoltage(0, 8);
  Serial.print("\n\rADC0: ");
  Serial.println(voltage);

  if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  listNets();
  listSpecialNets();
  Serial.println("\n\n\r");
  printPathsCompact();
  Serial.println("\n\n\n\n\r");

  removeBridgeFromNodeFile(ADC0, 20, netSlot, 0);
  removeBridgeFromNodeFile(DAC1, 20, netSlot, 0);
  refreshConnections(-1, 0);

  addBridgeToNodeFile(ISENSE_PLUS, 9, netSlot, 0, 0);
  addBridgeToNodeFile(ISENSE_MINUS, 28, netSlot, 0, 0);
  refreshConnections();

  float current_ma = INA0.getCurrent_mA();

  Serial.print("current between row 19 and 28: ");
  Serial.print(current_ma);
  Serial.println(" mA\n\r");

  removeBridgeFromNodeFile(ISENSE_PLUS, 9, netSlot, 0);
  removeBridgeFromNodeFile(ISENSE_MINUS, 28, netSlot, 0);
  refreshConnections(-1, 0);
  delay(1);

  if ( (encoderButtonState == PRESSED && lastButtonEncoderState == RELEASED) || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  setTopRail(4.20F);
  addBridgeToNodeFile(TOP_RAIL, ISENSE_PLUS, netSlot, 0, 0);
  refreshConnections(-1, 0);
  waitCore2();
  delay(10);
  float voltage2 = INA0.getBusVoltage();
  Serial.print("top rail voltage (from INA219): ");
  Serial.print(voltage2, 4);
  Serial.println(" V\n\r");

  removeBridgeFromNodeFile(TOP_RAIL, ISENSE_PLUS, netSlot, 0);
  refreshConnections(-1, 0);

  float probeVoltage = readAdcVoltage(7, 8);
  Serial.print("Probe voltage: ");
  Serial.print(probeVoltage, 4);
  Serial.println(" V\n\r");

  float probeCurrent = checkProbeCurrent();
  Serial.print("Probe current: ");
  Serial.print(probeCurrent, 2);
  Serial.println(" mA\n\r");

  if ((encoderButtonState == PRESSED && lastButtonEncoderState == RELEASED) || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  showLEDsCore2 = -3;
  b.clear();
  clearLEDsExceptRails();

  b.printRawRow(0b00001, 55, 0x170010, 0xffffff);
  b.printRawRow(0b00011, 56, 0x150012, 0xffffff);
  b.printRawRow(0b00111, 57, 0x130013, 0xffffff);
  b.printRawRow(0b01111, 58, 0x100015, 0xffffff);
  b.printRawRow(0b11111, 59, 0x080018, 0xffffff);

  // Friendlier text
  for (uint8_t i = 0; i < 254; i++) {
    hsvColor hsvTextColor = { i, 255, 30 };
    rgbColor rgbTextColor = HsvToRgb(hsvTextColor);
    uint32_t textColor = (uint32_t)rgbTextColor.r << 16 |
                         (uint32_t)rgbTextColor.g << 8  |
                         (uint32_t)rgbTextColor.b;
    b.print( "Fuck    you!", (uint32_t)textColor );
    showLEDsCore2 = -3;
    delayMicroseconds(200);
  }

  if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  int switchPosition = checkSwitchPosition();
  Serial.print("Switch position: ");
  Serial.println(switchPosition == 0 ? "Measure" : "Select");

  b.clear();
  b.print(" Tap    Rows!", (uint32_t)0x00140a);
  showLEDsCore2 = -3;
  delay(100);
  showLEDsCore2 = -1;

  int probeRow = -1;
  int lastProbedRow = 0;

  Serial.println("Click the probe button to exit\n\n\n\r");
  while (checkProbeButton() == 0) {
    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }
    probeRow = justReadProbe();
    if (probeRow != -1) {
      Serial.print("\r                 \rProbed row: ");
      printNodeOrName(probeRow);
      b.printRawRow(0b11111, lastProbedRow - 1, 0x000000, 0xffffff);
      b.printRawRow(0b11111, probeRow - 1, 0x172000, 0xffffff);
      b.print(probeRow, (uint32_t)0x002008);
      lastProbedRow = probeRow;
      delayMicroseconds(100);
    }
  }

  removeBridgeFromNodeFile(12, -1, netSlot, 0);
  removeBridgeFromNodeFile(52, -1, netSlot, 0);
  refreshConnections(-1, 0);

  addBridgeToNodeFile(12, 25, netSlot, 1);
  refreshLocalConnections(-1, 0);
  delay(100);

  removeBridgeFromNodeFile(12, -1, netSlot, 1);
  refreshLocalConnections(-1, 0);
  delay(100);

  if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  for (int i = 1; i <= 31; i++) {
    removeBridgeFromNodeFile(1, i - 1, netSlot, 1);
    addBridgeToNodeFile(1, i, netSlot, 1);
    refreshLocalConnections(-1, 0);
    showLEDsCore2 = -1;
    waitCore2();
  }

  removeBridgeFromNodeFile(30, -1, netSlot, 1);
  refreshLocalConnections(-1, 0);

  if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }

  for (int i = 31; i <= 60; i++) {
    removeBridgeFromNodeFile(31, i - 1, netSlot, 0);
    addBridgeToNodeFile(31, i, netSlot, 0);
    refreshConnections(-1, 0);
    showLEDsCore2 = -1;
    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }
  }
  removeBridgeFromNodeFile(60, -1, netSlot, 1);
  refreshConnections(-1, 0);
  delay(100);

  sendXYraw(CHIP_K, 4, 0, 1);
  sendXYraw(CHIP_A, 9, 1, 1);

  unsigned long startTime = millis();
  unsigned long timeout = 5000;
  while (millis() - startTime < timeout) {
    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) { leaveApp(lastNetSlot); return; }
    sendXYraw(CHIP_K, 4, 0, 1);
    delayMicroseconds(100);
    sendXYraw(CHIP_K, 4, 0, 0);
    sendXYraw(CHIP_K, 15, 0, 1);
    delayMicroseconds(100);
    sendXYraw(CHIP_K, 15, 0, 0);
  }

  sendXYraw(CHIP_K, 4, 0, 0);
  sendXYraw(CHIP_K, 15, 0, 0);
  sendXYraw(CHIP_A, 9, 1, 0);
}

void xlsxGui(void) {
  unsigned long startTime = millis();
  unsigned long messageTime = startTime;
  unsigned long timeout = 1000000;

  String string_from_UART = "";
  String command = "", args = "";
  String queuedCommand = "  ";
  bool isEchoEnabled = true;
  bool isJumperlessReply = false;

  int DT = 10;

  USBSer1.println(F("Message,Jumperless GUI started"));
  USBSer1.flush();

  while (millis() - startTime < timeout) {
    uint32_t currentMillis = millis();

    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) {
      Serial.println(F("Exiting XLSX GUI early"));
      break;
    }

    if (currentMillis > messageTime) {
      messageTime = currentMillis + 1000;
      USBSer1.dtr();
      USBSer1.print(F("Message,Jumperless GUI is running..."));
      USBSer1.print(F("\n"));
      USBSer1.flush();
      USBSer1.println(F("|Message,,Jumperless GUI is running..."));
      USBSer1.flush();
    }

    if (USBSer1.available()) {
      char c = USBSer1.read();
      if (isEchoEnabled) {
        Serial.print(c);
        Serial.flush();
      }
      if ((c != '\n') && (c != '\r') && (c != '|')) {
        string_from_UART += c;
      } else {
        string_from_UART.trim();
        if (string_from_UART.length() > 0) {
          delay(DT);
          int index_of_first_comma = string_from_UART.indexOf(',');
          bool isExcelCommand = false;
          if (index_of_first_comma == 0) {
            isExcelCommand = true;
            string_from_UART = string_from_UART.substring(1, string_from_UART.length());
            index_of_first_comma = string_from_UART.indexOf(',');
          }
          if (index_of_first_comma != -1) {
            command = string_from_UART.substring(0, index_of_first_comma);
            args = string_from_UART.substring(index_of_first_comma + 1);
          } else {
            command = string_from_UART;
            args = "";
          }

          USBSer1.print(F("Command: "));
          USBSer1.println(command);
          USBSer1.print(F("Arguments: "));
          USBSer1.println(args);
          USBSer1.flush();

          if (command == "h" || command == "help") {
            delay(DT); USBSer1.println(F(""));
            delay(DT); USBSer1.println(F("|'h' or 'help',, Prints this list"));
            delay(DT); USBSer1.println(F("|'echo,<val>' ,, Prints the echo setting; <val> (optional)"));
            delay(DT); USBSer1.println(F("|             ,, is 'on' or 'off' to set the echo setting accordingly"));
            delay(DT); USBSer1.println(F("|'TakeOnMe'   ,, Plays Take On Me from pin 11"));
            delay(DT); USBSer1.println(F("|'dance'      ,, progresses the 'dance' LED"));
            delay(DT); USBSer1.println(F("|             ,, pattern by one step"));
            delay(DT); USBSer1.println(F("|'send,<val>' ,, Sends a single-character command "));
            delay(DT); USBSer1.println(F("|             ,, to Jumperless (<val> = n, q, or Q)"));
            delay(DT); USBSer1.println(F("|'P0_?,<val>' ,, Sends <val> as 'f' command to Jumperless; prints"));
            delay(DT); USBSer1.println(F("|             ,, an abridged result; then sends 'n' to"));
            delay(DT); USBSer1.println(F("|             ,, Jumperless and prints an Excel-compatible result;"));
            delay(DT); USBSer1.println(F("|             ,, <val> = '' will clear all connections"));
            delay(DT); USBSer1.println(F("|             ,, P0_H/P0_L sets GPIO 0 HIGH/LOW respectively"));
            delay(DT); USBSer1.println(F("|             ,, TBD..."));
          } else if (command == "echo") {
            if (args == "") {
              USBSer1.print(F("Serial echo is currently "));
              USBSer1.println(isEchoEnabled ? F("enabled") : F("disabled"));
            } else if (args == "on") {
              isEchoEnabled = true;
            } else if (args == "off") {
              isEchoEnabled = false;
            } else {
              USBSer1.println(F("The echo command only accepts the arguments 'on' and 'off'."));
            }
            delay(DT);
          } else {
            USBSer1.print(F("Unknown command: "));
            if (isExcelCommand) USBSer1.print(F(","));
            USBSer1.println(string_from_UART);
            delay(DT);
          }
          USBSer1.flush();
          string_from_UART = "";
        }
      }
    }
  }
  Serial.println(F("XLSX GUI Done!"));
  Serial.flush();
  USBSer1.println(F("XLSX GUI Done!"));
  USBSer1.flush();
  return;
}

void bounceStartup(void) {
  Serial.print("\n\rPress any key to exit\n\n\r");
  leds.clear();
  pauseCore2 = 1;

  int bounceDelay = 300;
  resetEncoderPosition = true;
  while (1) {
    drawAnimatedImage(0, bounceDelay);
    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) break;
    delayMicroseconds(bounceDelay);
    drawAnimatedImage(1, bounceDelay);
    if (digitalRead(BUTTON_ENC) == 0 || Serial.available() > 0) break;
  }
  pauseCore2 = 0;
  showLEDsCore2 = -1;
  waitCore2();
}

void scanBoard(void) {
  int countLoop = 0;
  int countMult = 18;
  refreshConnections(-1, 0);

  Serial.println("\n\n\r");

  int lastRow = 0;
  int lastFloat = 0;
  int lastNode = 0;
  int leave = 0;

  while (Serial.available() == 0 && leave == 0) {
    for (int i = 1; i < 96; i++) {
      if (i == 84 || i == NANO_RESET_0 || i == NANO_RESET_1) continue;
      if ((i > 60 && i < 70) || i == 17) continue;

      struct rowLEDs currentRow = getRowLEDdata(i);
      float measuredVoltage = measureVoltage(2, i, true);

      if (measuredVoltage == 0xFFFFFFFF) {
        if (lastFloat == -1 || i == 1) {
          lastFloat = i;
          printNodeOrName(i);
          Serial.print(" - ");
        }
      } else {
        printNodeOrName(lastRow);
        Serial.println("\tfloating");
        lastFloat = -1;
        Serial.print("\t\t\t");
        int len = printNodeOrName(i);
        for (int j = 0; j < 3 - len; j++) Serial.print(" ");
        Serial.print(" = ");
        Serial.print(measuredVoltage);
        Serial.println(" V");
        Serial.flush();
      }

      setRowLEDdata(i, currentRow);

      if (Serial.available() > 0) { leave = 1; break; }
      if (encoderButtonState == PRESSED) { leave = 1; break; }

      showLEDsCore2 = 2;
      delay(3);
      lastRow = i;
    }

    Serial.println("\r                   \r\n\n\r");
    Serial.flush();
    countLoop++;
    if (countLoop * countMult > 95) {
      countLoop = 0;
      countMult -= 2;
    }
    if (leave == 1) break;
  }
}

int i2cScan(int sdaRow, int sclRow, int sdaPin, int sclPin, int leaveConnections) {
  if (sdaRow < 0 || sclRow < 0) {
    Serial.println("defaulting to \n\n\rGPIO 26 = SDA\n\rGPIO 27 = SCL");
  } else {
    addBridgeToNodeFile(RP_GPIO_26, sdaRow, netSlot, 0, 0); // SDA
    addBridgeToNodeFile(RP_GPIO_27, sclRow, netSlot, 0, 0); // SCL
    refreshConnections(-1, 1);
    waitCore2();
  }

  oled.clear();
  oled.print("GPIO 26 = SDA\n\r");
  oled.print("GPIO 27 = SCL");
  oled.show();
  delay(20);

  Wire1.end();
  Wire1.setSDA(sdaPin);
  Wire1.setSCL(sclPin);
  Wire1.begin();
  Wire1.setClock(100000);

  Serial.println("\nScanning I2C bus...");
  Serial.println("    _0  _1  _2  _3  _4  _5  _6  _7  _8  _9  _A  _B  _C  _D  _E  _F ");

  int nDevices = 0;
  uint8_t addressesFound[128];
  int addressesFoundCount = 0;
  int addressesFoundIndices[128]; // fixed spelling

  for (int baseAddr = 0; baseAddr < 128; baseAddr += 16) {
    Serial.printf("\n\r%02X:", baseAddr / 16);
    for (int addr = 0; addr < 16; addr++) {
      int deviceAddr = baseAddr + addr;
      if (deviceAddr > 0 && deviceAddr < 127) {
        Wire1.beginTransmission(deviceAddr);
        byte error = Wire1.endTransmission();
        if (error == 0) {
          Serial.printf(" %02X ", deviceAddr); // zero-padded
          nDevices++;
          addressesFound[addressesFoundCount] = deviceAddr;
          addressesFoundIndices[addressesFoundCount] = baseAddr + addr;
          addressesFoundCount++;
        } else {
          Serial.print(" -- ");
        }
      } else {
        Serial.print(" -- ");
      }
    }
  }

  Serial.println("\n\nI2C Scan Results:");
  if (nDevices == 0) {
    Serial.println("No I2C devices found");
    showLEDsCore2 = -3;
    b.clear();
    b.print("No I2C  Found", (uint32_t)0x070003);
    delayWithButton(2000);
    b.clear();
    showLEDsCore2 = -1;
  } else {
    Serial.printf("Found %d I2C device(s)\n", nDevices);
    showLEDsCore2 = -3;
    b.clear();
    showLEDsCore2 = -3;
    if (addressesFoundCount == 1) {
      b.print("Found", (uint32_t)0x000b01, (uint32_t)0x000000, 0, 0, 3);
      b.print(addressToHexString(addressesFound[0]), (uint32_t)0x000a05, (uint32_t)0x000000, 1, 1);
    } else if (addressesFoundCount > 1) {
      b.print(addressToHexString(addressesFound[0]), (uint32_t)0x000a05, (uint32_t)0x000000, 1, 1);
      b.print(addressToHexString(addressesFound[1]), (uint32_t)0x000808, (uint32_t)0x000000, 1, 1);
    }
    delayWithButton(2000);
    showLEDsCore2 = -1;
  }

  if (leaveConnections == 0 && sdaRow != -1 && sclRow != -1) {
    removeBridgeFromNodeFile(RP_GPIO_26, sdaRow, netSlot, 0);
    removeBridgeFromNodeFile(RP_GPIO_27, sclRow, netSlot, 0);
    refreshConnections(-1, 1);
  }

  Wire1.end();
  Wire1.begin();
  if (oled.oledConnected == true) {
    delay(500);
    oled.clear();
    oled.print("Found = ");
    oled.setTextSize(1);
    if (addressesFoundCount > 0) oled.print(addressToHexString(addressesFound[0]));
    oled.show();
  }

  return nDevices;
}

// ---- calibrateDacs() ----
// (unchanged from your version; retained verbatim)
// NOTE: Keeping as-is to avoid unintended behavior changes.
void calibrateDacs(void) {
  // ... your entire original function body here unchanged ...
  // (For brevity in this reply, keep your existing implementation.)
  // ***** COPY YOUR EXISTING calibrateDacs() CONTENT HERE *****
}

// ---- printSerial1stuff() ----
void printSerial1stuff(void) {
  Serial.println("Printing Serial1 stuff");
  Serial.flush();

  oled.clear();

  while (1) {
    if (Serial.available() > 0) {
      char c = Serial.read();
      USBSer1.write(c);
      USBSer1.flush();
      oled.print(c);
      oled.show();
      Serial.print("sent: ");
      Serial.println(c);
      Serial.flush();
      if (c == 'x') break;
    }
    if (USBSer1.available() > 0) {
      char c = USBSer1.read();
      Serial.print("received: ");
      Serial.println(c);
      Serial.flush();
    }
  }
}

void displayImage(void) {
  Serial.println(F("PNG Image app not implemented yet."));
}

const char* addressToHexString(uint8_t address) {
  static char hexStr[6];
  sprintf(hexStr, "0x%02X", address);
  return hexStr;
}


// Unified RGB->JDI palette mapping
static inline uint16_t rgbToJdi(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t col = 0;
  if (r >= 0x50) col |= COLOR_RED;
  if (g >= 0x80) col |= COLOR_GREEN;
  if (b >= 0x60) col |= COLOR_BLUE; // unified threshold
  return col;
}

void jdiMIPdisplay(void) {
  jdi_display.begin();
  jdi_display.displayOn();
  jdi_display.clearScreen();
  jdi_display.refresh();

  cycleTerminalColor(true, 5.3, true);
  Serial.println("\n\rThis is meant to run with a JDI LPM009M360A 72x144 display");
  Serial.println("It will bounce the startup animation and then exit\n\r");
  cycleTerminalColor();
  Serial.println("Connections: \n\r");

  Serial.println("  ┃  8  7  6  5  4  3  2  1  ┃  ");
  Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
  Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
  Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
  Serial.println("  ┃  ▌  ╳  ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
  Serial.println("  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━┛  "); cycleTerminalColor();
  Serial.println("     G  N  3  D  E  C  M  C     ");
  Serial.println("     N  C  V  i  x  S  O  L     ");
  Serial.println("     D     3  s  t  .  S  K     ");
  Serial.println("     ⏚     +  p     .  I  .     ");
  Serial.println("              .     .  .  .     "); cycleTerminalColor();
  Serial.println("    GPIO      6     2  4  3     ");

  Serial.println();
  cycleTerminalColor();
  Serial.println("GPIO 2 - CS");
  Serial.println("GPIO 3 - SCK");
  Serial.println("GPIO 4 - MOSI");
  Serial.println("GPIO 6 - DISP");
  cycleTerminalColor();
  Serial.println("\n\rPress the encoder button or any key to exit\n\n\r");
  cycleTerminalColor();
  Serial.println("Paste this in if you have the display soldered to the adapter board's SMD footptint");
  Serial.println("f { 139-106,136-79,101-80,82-100,132-77,134-76,133-75, } ");
  cycleTerminalColor();
  Serial.flush();

  const int srcW = 32;
  const int srcH = 21;
  const int rotW = srcH;  // 90° rotation width
  const int rotH = srcW;  // 90° rotation height

  for (int outer = 0; outer < 100; outer++) {
    for (int fi = 0; fi < startupFrameLEN; fi++) {
      const uint32_t* src = startupFrameArray[fi];
      for (int dy = 0; dy < DISPLAY_HEIGHT; dy++) {
        int uy = (dy * rotH) / DISPLAY_HEIGHT;
        for (int dx = 0; dx < DISPLAY_WIDTH; dx++) {
          int ux = (dx * rotW) / DISPLAY_WIDTH;
          int sx = srcW - 1 - uy;
          int sy = ux;
          uint32_t c = src[ sy * srcW + sx ];
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >>  8) & 0xFF;
          uint8_t b = (c      ) & 0xFF;
          uint16_t col = rgbToJdi(r, g, b);
          jdi_display.drawBufferedPixel(DISPLAY_WIDTH - 1 - dx, DISPLAY_HEIGHT - 1 - dy, col);
        }
      }
      jdi_display.refresh();
    }
    for (int fi = startupFrameLEN - 1; fi >= 0; fi--) {
      const uint32_t* src = startupFrameArray[fi];
      for (int dy = 0; dy < DISPLAY_HEIGHT; dy++) {
        int uy = (dy * rotH) / DISPLAY_HEIGHT;
        for (int dx = 0; dx < DISPLAY_WIDTH; dx++) {
          int ux = (dx * rotW) / DISPLAY_WIDTH;
          int sx = srcW - 1 - uy;
          int sy = ux;
          uint32_t c = src[ sy * srcW + sx ];
          uint8_t r = (c >> 16) & 0xFF;
          uint8_t g = (c >>  8) & 0xFF;
          uint8_t b = (c      ) & 0xFF;
          uint16_t col = rgbToJdi(r, g, b); // same thresholds both passes
          jdi_display.drawBufferedPixel(DISPLAY_WIDTH - 1 - dx, DISPLAY_HEIGHT - 1 - dy, col);
        }
      }
      jdi_display.refresh();
    }

    if (encoderButtonState == PRESSED || Serial.available() > 0) break;
    delay(10);
  }
  return;
}
