// RP23V50firmware/src/TuiGlue.cpp
// Uses standalone popup class (TuiPopup) instead of inline popup code.

#include "Tui.h"
#include "TuiGlue.h"
#include "TuiPopup.h"   // <-- new popup class

#include <Arduino.h>
#include <Wire.h>
#include "ArduinoStuff.h"
#include "LogicAnalyzer.h"
#include "oled.h"
#include "CH446Q.h"
#include "NetsToChipConnections.h"
#include "Commands.h"
#include "FileParsing.h"
#include "configManager.h"

using namespace TuiGlue;

Stream* TuiGlue::TUIserial = &USBSer3;
extern int netSlot;

/* ===============================
   App callbacks (menu actions)
   =============================== */

static void cbI2CScan() {
  TUI::popupOpen("I2C Scan", -1, -1, 0.60f, 0.80f);
  TUI::popupClear();

  auto scanOne = [&](TwoWire& bus, const char* tag, uint32_t hz)->uint8_t {
    bus.begin(); bus.setClock(hz);
    uint8_t found = 0;
    TUI::popupPrintln(String("Scanning ") + tag + " @ " + (int)(hz/1000) + "kHz …");
    for (uint8_t addr = 1; addr < 127; ++addr) {
      bus.beginTransmission(addr);
      uint8_t err = bus.endTransmission();
      char buf[8]; snprintf(buf, sizeof(buf), "0x%02X", addr);
      if (err == 0) {
        TUI::popupPrintln(String("  ") + tag + ": device at " + buf);
        ++found;
      } else if (err == 4) {
        TUI::popupPrintln(String("  ") + tag + ": unknown error at " + buf);
      }
      delay(2);
    }
    if (!found) TUI::popupPrintln(String("  ") + tag + ": no devices.");
    return found;
  };

  TUI::setStatus("I2C scan…");
  TUI::popupPrintln("---------------- I2C Scan ----------------");
  uint8_t f0 = scanOne(Wire , "I2C0/Wire" , 100000);
  uint8_t f1 = scanOne(Wire1, "I2C1/Wire1", 100000);
  uint8_t total = (uint8_t)(f0 + f1);

  if (!total) {
    TUI::popupPrintln("Result: no I2C devices found.");
    TUI::setStatus("I2C: none.");
  } else {
    TUI::popupPrintln(String("Result: ") + (int)total + " device(s) total.");
    TUI::setStatus(String("I2C: ") + (int)total);
  }
  TUI::popupPrintln("------------------------------------------");
}

static void cbLogicAnalyzer() {
  TUI::setStatus("Logic analyzer (demo)…");
  TUI::log("LA: would arm capture and stream logs.");
}

static void cbFlashArduino()  {
  TUI::setStatus("Flashing (demo)…");
  for (int i=0;i<=100;i+=25) {
    TUI::log(String("Flash: ")+i+"%");
    delay(10);
  }
  TUI::setStatus("Flash done (demo).");
}

static void cbToggleOLED() {
  if ( jumperlessConfig.top_oled.enabled == 0 ) {
    oled.init( );
    jumperlessConfig.top_oled.enabled = 1;
    oled.clearPrintShow("OLED Set ON", 1, true, true, true, -1, -1, 1500);
  } else {
    oled.clearPrintShow("OLED Set OFF", 1, true, true, true, -1, -1, 1500);
    oled.disconnect( );
    jumperlessConfig.top_oled.enabled = 0;
    oled.oledConnected = false;
  }
  saveConfig( );
  TUI::setStatus(jumperlessConfig.top_oled.enabled ? "OLED ON" : "OLED OFF");
}

static void cbAbout()  {
  TUI::setStatus("About");
  TUI::log("Jumperless RP2350B – DOS-style TUI (demo)");
}

static void cbResizeToTerminal() {
  uint16_t r=0,c=0;
  if (TUI::probeTerminalSize(r,c) && r>0 && c>0) {
    TUI::resizeTo(r,c);
    TUI::fullRedraw();
    TUI::log(String("Resized to ")+r+"x"+c+".");
  } else {
    TUI::log("Resize failed: terminal didn't answer CPR.");
  }
}

static void cbMenuNarrower(){
  TUI::L.leftFracWide = max(0.10f, TUI::L.leftFracWide - 0.02f);
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
}

static void cbMenuWider()   {
  TUI::L.leftFracWide = min(0.80f, TUI::L.leftFracWide + 0.02f);
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
}

static void cbClearAllConnections() {
  digitalWrite( RESETPIN, HIGH );
  delay( 1 );
  refreshPaths( );
  clearAllNTCC( );

  clearNodeFile( netSlot, 0 );
  refreshConnections( -1, 1, 1 );
  digitalWrite( RESETPIN, LOW );

  TUI::log( "Cleared all connections" );
}

static void cbTightenGaps() {
  if (TUI::L.margin) TUI::L.margin--;
  if (TUI::L.gap)    TUI::L.gap--;
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
}

static void cbLoosenGaps()  {
  TUI::L.margin++;
  TUI::L.gap++;
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
}

static void cbScreenResetLayout() {
  TUI::L.leftFracWide   = 0.15f;
  TUI::L.leftFracNarrow = 0.50f;
  TUI::L.margin = 2;
  TUI::L.gap    = 2;
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
  TUI::log("Screen layout reset.");
}

// ----- Border style helpers/callbacks --------------------------------
static const char* borderStyleName(TUI::BorderStyle s) {
  switch (s) {
    case TUI::BORDER_ASCII:   return "ASCII";
    case TUI::BORDER_LIGHT:   return "Light";
    case TUI::BORDER_ROUNDED: return "Rounded";
    case TUI::BORDER_HEAVY:   return "Heavy";
    case TUI::BORDER_DOUBLE:  return "Double";
    default:                  return "?";
  }
}
static void applyBorder(TUI::BorderStyle s) {
  TUI::setBorderStyle(s);  // does fullRedraw()
  TUI::log(String("Border → ") + borderStyleName(s));
  TUI::setStatus(String("Border: ") + borderStyleName(s));
}
static void cbBorderCycle() {
  using namespace TUI;
  BorderStyle s = THEME.borderStyle;
  switch (s) {
    case BORDER_ASCII:   s = BORDER_LIGHT;   break;
    case BORDER_LIGHT:   s = BORDER_ROUNDED; break;
    case BORDER_ROUNDED: s = BORDER_HEAVY;   break;
    case BORDER_HEAVY:   s = BORDER_DOUBLE;  break;
    case BORDER_DOUBLE:  s = BORDER_ASCII;   break;
    default:             s = BORDER_LIGHT;   break;
  }
  applyBorder(s);
}
static void cbBorderAscii()   { applyBorder(TUI::BORDER_ASCII); }
static void cbBorderLight()   { applyBorder(TUI::BORDER_LIGHT); }
static void cbBorderRounded() { applyBorder(TUI::BORDER_ROUNDED); }
static void cbBorderHeavy()   { applyBorder(TUI::BORDER_HEAVY); }
static void cbBorderDouble()  { applyBorder(TUI::BORDER_DOUBLE); }

// ----- Settings menus -------------------------------------------------
static const TUI::MenuItem SETTINGS_SCREEN_BORDER_ITEMS[] = {
  {"&Light  (┌─┐ │ │ └─┘)",   cbBorderLight},
  {"&Rounded(╭─╮ │ │ ╰─╯)",   cbBorderRounded},
  {"&Heavy  (┏━┓ ┃ ┃ ┗━┛)",   cbBorderHeavy},
  {"&Double (╔═╗ ║ ║ ╚═╝)",   cbBorderDouble},
  {"&ASCII  (+-+ | | +-+)",   cbBorderAscii},
  {"C&ycle styles",            cbBorderCycle},
};

static const uint8_t SETTINGS_SCREEN_BORDER_ITEMS_N = TUI_ARRLEN(SETTINGS_SCREEN_BORDER_ITEMS);

static const TUI::MenuItem SETTINGS_SCREEN_ITEMS[] = {
  TUI_SUBMENU_ENTRY("&Borders", SETTINGS_SCREEN_BORDER_ITEMS, "Settings / Screen / Borders"),
  {"Resize to Terminal", cbResizeToTerminal},
  {"Menu Narrower",      cbMenuNarrower},
  {"Menu Wider",         cbMenuWider},
  {"Tighten Gaps",       cbTightenGaps},
  {"Loosen Gaps",        cbLoosenGaps},
  {"Reset Layout",       cbScreenResetLayout},
};
static const uint8_t SETTINGS_SCREEN_ITEMS_N = TUI_ARRLEN(SETTINGS_SCREEN_ITEMS);

static const TUI::MenuItem SETTINGS_ITEMS[] = {
  TUI_SUBMENU_ENTRY("Screen", SETTINGS_SCREEN_ITEMS, "Settings / Screen"),
  {"Toggle OLED", cbToggleOLED},
  {"About",       cbAbout},
};
static const uint8_t SETTINGS_ITEMS_N = TUI_ARRLEN(SETTINGS_ITEMS);

static const TUI::MenuItem TOOL_ITEMS[] = {
  {"I2C Scanner",    cbI2CScan},  // uses popup
};
static const uint8_t TOOLS_ITEMS_N = TUI_ARRLEN(TOOL_ITEMS);

static const TUI::MenuItem MAIN_ITEMS[] = {
  {"Logic Analyzer", cbLogicAnalyzer},
  {"Flash Arduino",  cbFlashArduino},
  {"Clear All Connections", cbClearAllConnections},
  TUI_SUBMENU_ENTRY("&Tools", TOOL_ITEMS, "Tools"),
  TUI_SUBMENU_ENTRY("Se&ttings", SETTINGS_ITEMS, "Settings"),
};
static const uint8_t MAIN_ITEMS_N = TUI_ARRLEN(MAIN_ITEMS);

// ===============================
// Glue state & helpers
// ===============================
namespace {
  bool     s_modelBuilt          = false;
  bool     s_active              = false;

  bool     s_needDeferredRedraw  = false;
  uint32_t s_nextRetryAtMs       = 0;
  uint8_t  s_redrawTries         = 0;
  bool     s_seenHostByte        = false;

  // DTR change watcher (for USB CDC streams)
  bool     s_lastDTR             = false;

  inline bool currentDTR() {
  #if defined(ARDUINO_ARCH_RP2040)
    return (TuiGlue::TUIserial == &USBSer3) ? USBSer3.dtr() : true;
  #else
    return true;
  #endif
  }

  void checkUSBconnection() {
  #if defined(ARDUINO_ARCH_RP2040)
    if (TuiGlue::TUIserial == &USBSer3) {
      bool d = USBSer3.dtr();
      if (d != s_lastDTR) {
        s_lastDTR = d;
        // On reopen, schedule a deferred size probe/redraw (no escape queries)
        s_needDeferredRedraw = true;
        s_redrawTries        = 12;
        s_nextRetryAtMs      = millis() + 120;
        s_seenHostByte       = false;
        TUI::fullRedraw();
      }
    }
  #endif
  }
}

void TuiGlue::init() {
  if (s_modelBuilt) return;
  s_modelBuilt = true;

  // Keep engine on the same stream as glue
  TUI::TUIserial = TuiGlue::TUIserial;
  // NEW: popup uses the same serial
  TUI::popupSetSerial(TuiGlue::TUIserial);

  TUI::S.appTitle = "Jumperless V5 – RP2350B";
  TUI::S.status   = "Ready.";

  TUI::pushMenu(MAIN_ITEMS, MAIN_ITEMS_N, "Main");
}

static void activate() {
  if (!s_modelBuilt)
    TuiGlue::init();

  // Keep engine+popup+glue on the same stream
  TUI::TUIserial = TuiGlue::TUIserial;
  TUI::popupSetSerial(TuiGlue::TUIserial); // NEW

#if defined(ARDUINO_ARCH_RP2040)
  if (TuiGlue::TUIserial == &USBSer3) {
    uint32_t t0 = millis();
    while (!USBSer3.dtr() && (millis() - t0 < 1200)) { delay(10); }
    s_lastDTR = USBSer3.dtr();
  }
#endif

  // Nudge terminal so some programs wake up
  TuiGlue::TUIserial->print("\r\n");
  TuiGlue::TUIserial->flush();
  delay(20);

  s_active = true;
  TUI::fullRedraw();

  // Initial size probe (conservative)
  uint16_t r=0, c=0;
  if (TUI::probeTerminalSize(r, c) && r>0 && c>0) {
    TUI::resizeTo(r, c);
    TUI::fullRedraw();
    s_needDeferredRedraw = false;
  } else {
    // If terminal isn’t ready yet, schedule retries until we see host bytes
    s_needDeferredRedraw = true;
    s_seenHostByte       = false;
    s_redrawTries        = 12;      // ~1.4s total with 120–140ms steps
    s_nextRetryAtMs      = millis() + 120;
  }

  TUI::log("Use \x1b[1mUp/Down\x1b[0m to navigate, \x1b[1mEnter\x1b[0m to select, \x1b[1mESC\x1b[0m to go back.");
}

void TuiGlue::openOnDemand() { activate(); }
bool TuiGlue::isActive()     { return s_active; }

void TuiGlue::loop() {
  if (!s_modelBuilt) TuiGlue::init();
  if (!s_active)     activate();
  if (!s_active)     return;

  checkUSBconnection();

  if (TuiGlue::TUIserial->available())
    s_seenHostByte = true;

  uint16_t spins = 0;

  if (TUI::isPopupActive()) {
    while (TuiGlue::TUIserial->available() && spins < 12) {
      int ch = TuiGlue::TUIserial->read();
     TUI::popupHandleKey(ch);
      spins++;
      if ((spins & 0x07) == 0) 
        delayMicroseconds(250);
    }

    TUI::popupDrawIfDirty();

  } else {
    
    while ((TuiGlue::TUIserial->available() || TUI::inEscapeSeq()) && spins < 2) {
      TUI::handleInput();
      spins++;
      if ((spins & 0x07) == 0) 
        delayMicroseconds(250);
    }

    if (s_needDeferredRedraw && (int32_t)(millis() - s_nextRetryAtMs) >= 0) {
      bool done = false;
      uint16_t rr=0, cc=0;

      if (TUI::probeTerminalSize(rr, cc) && rr>0 && cc>0) {
        TUI::resizeTo(rr, cc);
        TUI::fullRedraw();
        done = true;
        
      } else if (s_seenHostByte) {
        
        if (TUI::probeTerminalSize(rr, cc) && rr>0 && cc>0) {
          TUI::resizeTo(rr, cc);
          TUI::fullRedraw();
          done = true;
        }
      }

      if (done || s_redrawTries == 0) {
        s_needDeferredRedraw = false;
      } else {
        s_redrawTries--;
        s_nextRetryAtMs = millis() + 140;
      }
    }

    if (TUI::S.logDirty) TUI::drawLog();
  }

  TuiGlue::TUIserial->flush();
}
