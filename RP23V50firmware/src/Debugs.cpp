

#include "Debugs.h"
#include "PersistentStuff.h"
#include "JulseView.h"
#include "AsyncPassthrough.h"
#include "configManager.h"
#include "config.h"
#include "ArduinoStuff.h"
#include "JulseView.h"
#include "Peripherals.h"
#include "Probing.h"
#include "NetManager.h"
#include "FileParsing.h"
#include "LEDs.h"
#include "Graphics.h"
#include "JulseView.h"









bool debugFlagsMenu() {
debugFlagInit();

debugFlags:

  int lastSerial1Passthrough = jumperlessConfig.serial_1.print_passthrough;
  int lastSerial2Passthrough = jumperlessConfig.serial_2.print_passthrough;
  printSerial1Passthrough = 0;
  printSerial2Passthrough = 0;

  // Interactive main debug menu: toggle by number, Enter to apply, 'l' to open LA submenu
  bool temp_debugFP = debugFP;
  bool temp_debugNM = debugNM;
  bool temp_debugNTCC = debugNTCC;
  bool temp_debugNTCC2 = debugNTCC2;
  bool temp_debugLEDs = debugLEDs;
  bool temp_debugLA = debugLA;
  int temp_showProbeCurrent = showProbeCurrent;
  int temp_passthrough = jumperlessConfig.serial_1.print_passthrough;
  bool temp_asyncPassthrough = jumperlessConfig.serial_1.async_passthrough;
  // Track originals for diffing on commit
  bool orig_debugFP = debugFP;
  bool orig_debugNM = debugNM;
  bool orig_debugNTCC = debugNTCC;
  bool orig_debugNTCC2 = debugNTCC2;
  bool orig_debugLEDs = debugLEDs;
  bool orig_debugLA = debugLA;
  int orig_showProbeCurrent = showProbeCurrent;
  int orig_passthrough = jumperlessConfig.serial_1.print_passthrough;
  bool orig_asyncPassthrough = jumperlessConfig.serial_1.async_passthrough;
  
  int lines = 0;
  int last_bulk_cmd = -1; // 0 for all off, 9 for all on; reset to -1 on individual changes

  auto print_main_debug_menu = [&](int &lines_printed) {
    lines_printed = 4;
    cycleTerminalColor(true, 2.5 );
    Serial.print("\n\n\rx.   all off"); lines_printed++;
    cycleTerminalColor();
    Serial.print("\n\rz.   all on"); lines_printed++;
    cycleTerminalColor();
    Serial.print("\n\n\r(press enter to exit)\n\r"); lines_printed+=2; cycleTerminalColor();
    Serial.print("\n\rf. file parsing               =    "); Serial.print(temp_debugFP); lines_printed++; cycleTerminalColor(); 
    Serial.print("\n\rn. net manager                =    "); Serial.print(temp_debugNM); lines_printed++; cycleTerminalColor();
    Serial.print("\n\rc. chip connections           =    "); Serial.print(temp_debugNTCC); lines_printed++; cycleTerminalColor();
    Serial.print("\n\rh. chip conns alt paths       =    "); Serial.print(temp_debugNTCC2); lines_printed++; cycleTerminalColor();
    Serial.print("\n\re. LEDs                       =    "); Serial.print(temp_debugLEDs); lines_printed++; cycleTerminalColor();
    Serial.print("\n\rs. show probe current         =    "); Serial.print(temp_showProbeCurrent); lines_printed++; cycleTerminalColor();
  //  Serial.print("\n\rs. show probe current         =    "); Serial.print(temp_showProbeCurrent); lines_printed++; cycleTerminalColor();
    Serial.print("\n\ru. print serial 1 passthrough =    ");
    if (temp_passthrough == 1) {
      Serial.print("on");
    } else if (temp_passthrough == 2) {
      Serial.print("flashing only");
    } else {
      Serial.print("off");
    }
    lines_printed++; cycleTerminalColor();

    Serial.print("\n\rp. async passthrough          =    "); Serial.print(temp_asyncPassthrough ? 1 : 0); lines_printed++; cycleTerminalColor();

    Serial.print("\n\ra. Arduino debug level        =    "); Serial.print(jumperlessConfig.debug.arduino); lines_printed++; cycleTerminalColor();

    Serial.print("\n\rl. logic analyzer debug       =    "); Serial.print(temp_debugLA); lines_printed++; cycleTerminalColor();
 
    Serial.print("\n\rj. logic analyzer debug menu  >    "); lines_printed++; cycleTerminalColor();
    Serial.print("\n\r\n\r\n\r"); lines_printed += 2;
    Serial.flush();
  };

  print_main_debug_menu(lines);

  while (true) {
    while (Serial.available() == 0) { ; }
    int sel = Serial.read();
    // Enter confirms and applies changes
    if (sel == '\r' || sel == '\n') {
      // Apply bulk commands if selected and no further individual changes
      if (last_bulk_cmd == 0 || last_bulk_cmd == 9) {
        // Re-render once before leaving to clear the menu area
        Serial.printf("\033[%dA", lines);
        for (int i = 0; i < lines; i++) { Serial.print("\033[2K\r\n\r"); }
        Serial.printf("\033[%dA", lines);
        Serial.flush();
        debugFlagSet(last_bulk_cmd);
      } else {
        // Commit individual diffs using debugFlagSet only for changed items
        if (temp_debugFP != orig_debugFP) debugFlagSet(1);
        if (temp_debugNM != orig_debugNM) debugFlagSet(2);
        if (temp_debugNTCC != orig_debugNTCC) debugFlagSet(3);
        if (temp_debugNTCC2 != orig_debugNTCC2) debugFlagSet(4);
        if (temp_debugLEDs != orig_debugLEDs) debugFlagSet(5);
        if (temp_debugLA != orig_debugLA) debugFlagSet(6);
        if (temp_showProbeCurrent != orig_showProbeCurrent) debugFlagSet(7);
        if (temp_passthrough != orig_passthrough) {
          int cur = jumperlessConfig.serial_1.print_passthrough;
          int safety = 0;
          while (cur != temp_passthrough && safety < 4) {
            debugFlagSet(8);
            cur = jumperlessConfig.serial_1.print_passthrough;
            safety++;
          }
        }
        if (temp_asyncPassthrough != orig_asyncPassthrough) {
          jumperlessConfig.serial_1.async_passthrough = temp_asyncPassthrough;
          if (temp_asyncPassthrough) {
            AsyncPassthrough::begin();
          }
        }
        // Persist Arduino debug level if changed via menu
        saveConfig();
      }
      printSerial1Passthrough = lastSerial1Passthrough;
      printSerial2Passthrough = lastSerial2Passthrough;
      break; // leave menu
    }

    // Open JulseView LA submenu
    if (sel == 'j' || sel == 'J') {
      const char* categories[10] = {
        "commands", "buffers", "digital", "analog", "dma",
        "usb", "timing", "data", "errors", "state"
      };
      uint32_t local_mask = julseview_debug_mask;
      auto print_mask_menu = [&](uint32_t mask, int &lines_printed) {
        lines_printed = 1;
        cycleTerminalColor(true, 2.5 );
        Serial.print("\n\rJulseView Debug Categories (toggle by number, Enter to confirm)\n\r"); lines_printed+=2;
        Serial.printf("mask = 0x%08lX\n\r", (unsigned long)mask); lines_printed+=2; cycleTerminalColor();
        for (int i = 0; i < 10; i++) {
          int enabled = (mask & (1u << i)) ? 1 : 0;
          Serial.printf("%d. %-10s = %s\n\r", i, categories[i], enabled ? "on" : "off");
          lines_printed++; cycleTerminalColor();
        }
        Serial.print("\n\r"); lines_printed++; cycleTerminalColor();
        Serial.flush();
      };
      int llines = 0;
      print_mask_menu(local_mask, llines);
      while (true) {
        while (Serial.available() == 0) { ; }
        int ch = Serial.read();
        if (ch == '\r' || ch == '\n') {
          while (Serial.available() > 0) {
            int c2 = Serial.peek();
            if (c2 == '\r' || c2 == '\n') { Serial.read(); } else { break; }
          }
          break;
        }
        if (ch >= '0' && ch <= '9') {
          int idx = ch - '0';
          local_mask ^= (1u << idx);
          julseview_set_debug_mask(local_mask);
          Serial.printf("\033[%dA", llines);
          for (int i = 0; i < llines; i++) { Serial.print("\033[2K\r\n\r"); }
          Serial.printf("\033[%dA", llines);
          Serial.flush();
          print_mask_menu(local_mask, llines);
        }
      }
      // After returning from submenu, re-render the main menu
      Serial.printf("\033[%dA", lines);
      for (int i = 0; i < lines; i++) { Serial.print("\033[2K\r\n\r"); }
      Serial.printf("\033[%dA", lines);
      Serial.flush();
      print_main_debug_menu(lines);
      continue;
    }

    // Toggle items and redraw, but do not persist yet
    if (sel == 'x' || sel == 'X') {
      temp_debugFP = false;
      temp_debugNM = false;
      temp_debugNTCC = false;
      temp_debugNTCC2 = false;
      temp_debugLEDs = false;
      temp_debugLA = false;
      temp_showProbeCurrent = 0;
      last_bulk_cmd = 0;
    } else if (sel == 'z' || sel == 'Z') {
      temp_debugFP = true;
      temp_debugNM = true;
      temp_debugNTCC = true;
      temp_debugNTCC2 = true;
      temp_debugLEDs = true;
      temp_debugLA = true;
      temp_showProbeCurrent = 1;
      last_bulk_cmd = 9;
    } else if (sel == 'u' || sel == 'U') {
      // Cycle passthrough: 0 -> 2 -> 1 -> 0
      temp_passthrough = (temp_passthrough == 0) ? 2 : (temp_passthrough == 2) ? 1 : 0;
      last_bulk_cmd = -1;
    } else if (sel == 'f' || sel == 'F') { temp_debugFP = !temp_debugFP; last_bulk_cmd = -1; }
    else if (sel == 'n' || sel == 'N') { temp_debugNM = !temp_debugNM; last_bulk_cmd = -1; }
    else if (sel == 'c' || sel == 'C') { temp_debugNTCC = !temp_debugNTCC; last_bulk_cmd = -1; }
    else if (sel == 'h' || sel == 'H') { temp_debugNTCC2 = !temp_debugNTCC2; last_bulk_cmd = -1; }
    else if (sel == 'e' || sel == 'E') { temp_debugLEDs = !temp_debugLEDs; last_bulk_cmd = -1; }
    else if (sel == 'l' || sel == 'L') { temp_debugLA = !temp_debugLA; last_bulk_cmd = -1; }
    else if (sel == 'p' || sel == 'P') { temp_asyncPassthrough = !temp_asyncPassthrough; last_bulk_cmd = -1; }
    else if (sel == 's' || sel == 'S') { temp_showProbeCurrent = temp_showProbeCurrent ? 0 : 1; last_bulk_cmd = -1; }
    else {
      // not a recognized toggle key; fall through to next handlers
    }

    // If a recognized toggle occurred above, redraw and continue loop
    if (sel == 'x' || sel == 'X' || sel == 'z' || sel == 'Z' || sel == 'u' || sel == 'U' ||
        sel == 'f' || sel == 'F' || sel == 'n' || sel == 'N' || sel == 'c' || sel == 'C' ||
        sel == 'h' || sel == 'H' || sel == 'e' || sel == 'E' || sel == 'l' || sel == 'L' || sel == 'p' || sel == 'P' || sel == 's' || sel == 'S') {
      Serial.printf("\033[%dA", lines);
      for (int i = 0; i < lines; i++) { Serial.print("\033[2K\r\n\r"); }
      Serial.printf("\033[%dA", lines);
      Serial.flush();
      print_main_debug_menu(lines);
      continue;
    }

    // Adjust Arduino debug level: 'a' increments 0->1->2->0
    if (sel == 'a' || sel == 'A') {
      int level = jumperlessConfig.debug.arduino;
      level = (level >= 2) ? 0 : level + 1;
      jumperlessConfig.debug.arduino = level;
      debugArduino = level;
      // Clear and redraw the menu
      Serial.printf("\033[%dA", lines);
      for (int i = 0; i < lines; i++) { Serial.print("\033[2K\r\n\r"); }
      Serial.printf("\033[%dA", lines);
      Serial.flush();
      print_main_debug_menu(lines);
      continue;
    }

    // Any other key exits without saving
    printSerial1Passthrough = lastSerial1Passthrough;
    printSerial2Passthrough = lastSerial2Passthrough;
    break;
  }

  return true;
}
