#include "TuiGlue.h"
#include "Tui.h"
#include "TuiPopup.h"

#include <Wire.h>
#include "ArduinoStuff.h"
#include "LogicAnalyzer.h"
#include "oled.h"
#include "CH446Q.h"
#include "NetsToChipConnections.h"
#include "Commands.h"
#include "FileParsing.h"
#include "configManager.h"
#include "tuiNetHelpers.h"
#include "Python_Proper.h"
#include "TerminalTakeover.h"
#include "TuiPopUpFileManager.h"

// External
extern int netSlot;


namespace {
  static const uint8_t HIST_MAX = 32;
  String   s_hist[HIST_MAX];
  uint8_t  s_histCount  = 0;
  int8_t   s_histBrowse = -1;

  inline void histPush(const String& raw) {
    String t = raw; t.trim();
    if (!t.length()) return;
    if (s_histCount > 0 && t == s_hist[(uint8_t)(s_histCount - 1)]) return;
    if (s_histCount < HIST_MAX) s_hist[s_histCount++] = t;
    else {
      for (uint8_t i=1;i<HIST_MAX;i++) s_hist[i-1] = s_hist[i];
      s_hist[HIST_MAX-1] = t;
    }
  }
  inline bool histPrev(String& out) {
    if (s_histCount == 0) 
      return false;

    if (s_histBrowse < (int8_t)(s_histCount - 1)) 
      s_histBrowse++;
    out = s_hist[(uint8_t)(s_histCount - 1 - s_histBrowse)];

    return true;
  }
  inline bool histNext(String& out) {
    if (s_histCount == 0) 
      return false;
    
    if (s_histBrowse > 0) {
    
      s_histBrowse--;
      out = s_hist[(uint8_t)(s_histCount - 1 - s_histBrowse)];
      return true;

    } else if (s_histBrowse == 0) {

      s_histBrowse = -1; out = ""; return true;

    }

    return false;
  }

  // -------------------- Free-function callbacks --------------------

  // Border helpers
  const char* borderStyleName(TUI::BorderStyle s) {
    switch (s) {

      case TUI::BORDER_ASCII:   return "ASCII";
      case TUI::BORDER_LIGHT:   return "Light";
      case TUI::BORDER_ROUNDED: return "Rounded";
      case TUI::BORDER_HEAVY:   return "Heavy";
      case TUI::BORDER_DOUBLE:  return "Double";
      default:                  return "?";

    }
  }
  void applyBorder(TUI::BorderStyle s) {
    TUI::setBorderStyle(s); 

    TUI::log(String("Border → ") + borderStyleName(s));
    TUI::setStatus(String("Border: ") + borderStyleName(s));
  }

  /*
  Call backs and handlers
  */
  void onAddNetSubmit(const String& text) {
    TUI::log(String("Add Net: ") + text);
    histPush(text);

    auto res = tuiNetHelpers::parse(text);
    struct Stats { uint8_t attempted=0, applied=0, errors=0, failed=0, notfound=0; } st;
    st.attempted = res.total;

    for (uint8_t i=0;i<res.total;++i){
      const auto& ep = res.items[i];
      const String A = tuiNetHelpers::toLabel(ep.from);
      const String B = tuiNetHelpers::toLabel(ep.to);
      if (ep.status != tuiNetHelpers::OK) { st.errors++; TUI::log(String("  Error: ") + ep.message); continue; }
      int rc = addBridgeToNodeFile((int)ep.from, (int)ep.to, netSlot, 0, 0);
      if (rc >= 0) { st.applied++; TUI::log(String("  + Added ") + A + "-" + B); }
      else         { st.failed++;  TUI::log(String("  ! Add failed for ") + A + "-" + B + " (rc=" + rc + ")"); }
    }

    saveConfig(); refreshConnections(-1,1,1);
    if (res.truncated) TUI::log(String("  Note: input truncated to first ") + tuiNetHelpers::MAX_NET_PAIRS + " pairs.");
    TUI::setStatus(String("Add: ") + (int)st.applied + " applied, " + (int)(st.errors+st.failed+st.notfound) + " skipped/err.");
    if (TUI::S.logDirty) TUI::drawLog();
  }

  void onRemoveNetSubmit(const String& text) {
    TUI::log(String("Remove Net: ") + text);
    histPush(text);

    auto res = tuiNetHelpers::parse(text);
    struct Stats { uint8_t attempted=0, applied=0, errors=0, failed=0, notfound=0; } st;
    st.attempted = res.total;

    for (uint8_t i=0;i<res.total;++i){
      const auto& ep = res.items[i];
      const String A = tuiNetHelpers::toLabel(ep.from);
      const String B = tuiNetHelpers::toLabel(ep.to);
      if (ep.status != tuiNetHelpers::OK) { st.errors++; TUI::log(String("  Error: ") + ep.message); continue; }

      int rc = removeBridgeFromNodeFile((int)ep.from, (int)ep.to, netSlot, 0, 0);
      if (rc == 0) {
        int rc2 = removeBridgeFromNodeFile((int)ep.to, (int)ep.from, netSlot, 0, 0);
        if (rc2 > 0) rc = rc2; else if (rc2 < 0 && rc == 0) rc = rc2;
      }
      if (rc > 0) {
        st.applied++; TUI::log(String("  - Removed ") + A + "-" + B + " (" + rc + " match)");
      }
      else if (rc == 0) {
        st.notfound++; TUI::log(String("  ! Not found: ") + A + "-" + B);
      }
      else {
        st.failed++;  TUI::log(String("  ! Remove failed for ") + A + "-" + B + " (rc=" + rc + ")");
      }
    }

    saveConfig(); refreshConnections(-1,1,1);
    if (res.truncated)
      TUI::log(String("  Note: input truncated to first ") + tuiNetHelpers::MAX_NET_PAIRS + " pairs.");
    TUI::setStatus(String("Remove: ") + (int)st.applied + " applied, " + (int)(st.errors+st.failed+st.notfound) + " skipped/err.");
    if (TUI::S.logDirty) TUI::drawLog();
  }

  // Menu actions
  void cbAddNet() {
    TUI::popupOpen("Add Nets", -1, -1, 0.60f, 0.80f);
    TUI::popupClear();
    TUI::popupOpenInput("Add Nets", "Nets (e.g., 2-9;1-3): ");
    TUI::popupSetInitial("");
    TUI::popupOnSubmit(onAddNetSubmit);
    TUI::popupOnCancel([](){ TUI::log("Add Nets: cancelled."); });
    
  }

  void cbRemoveNet() {
    TUI::popupOpen("Remove Nets", -1, -1, 0.60f, 0.80f);
    TUI::popupClear();
    TUI::popupOpenInput("Remove Nets", "Nets (e.g., 2-9;1-3): ");
    TUI::popupSetInitial("");
    TUI::popupOnSubmit(onRemoveNetSubmit);
    TUI::popupOnCancel([](){ TUI::log("Remove Nets: cancelled."); });
    
  }

  void cbClearAllConnections() {
    clearNodeFile(netSlot, 0);
    refreshConnections(-1,1,1);
    TUI::log("Cleared all connections");
  }

  void cbToggleOLED() {
  if (jumperlessConfig.top_oled.enabled == 0) {
    oled.init(); oled.show(); jumperlessConfig.top_oled.enabled = 1;
    oled.clearPrintShow("OLED Set ON", 1, true, true, true, -1, -1, 1500);
  } else {
    oled.clearPrintShow("OLED Set OFF", 1, true, true, true, -1, -1, 1500);
    oled.disconnect(); jumperlessConfig.top_oled.enabled = 0; oled.oledConnected = false;
  }
  configChanged = true;
  TUI::setStatus(jumperlessConfig.top_oled.enabled ? "OLED ON" : "OLED OFF");
  //USBSer3.print("\x1bPq#0;2;0;0;0#1;2;100;100;0#2;2;0;100;0#1~~@@vv@@~~@@~~$#2??}}GG}}??}}\?\?-#1!14@\x1b");
  USBSer3.flush();
}



  void cbMicroPython() {

    TerminalTakeover::Options opt;
    opt.title = "MicroPython REPL";
    opt.help  = "Ctrl-C: interrupt • Ctrl-D: exit • In screen, send ^A as Ctrl-a a";

    TerminalTakeover session(&USBSer3, opt);
    session.nudgeFriendlyRepl();

    setGlobalStream(&USBSer3);
    enterMicroPythonREPL();

    uint16_t r=0, c=0;
    if (TUI::probeTerminalSize(r,c) && r>0 && c>0) 
      TUI::resizeTo(r,c);

    TUI::fullRedraw();
    TUI::setStatus("Returned from MicroPython");

  }

  void cbI2CScan() {

    TUI::popupOpen("I2C Scan", -1, -1, 0.60f, 0.80f);
    TUI::popupClear();

    auto scanOne = [&](TwoWire& bus, const char* tag, uint32_t hz)->uint8_t {
      bus.begin(); 
      bus.setClock(hz);

      uint8_t found = 0;
      TUI::popupPrintln(String("Scanning ") + tag + " @ " + (int)(hz/1000) + "kHz …");
      for (uint8_t addr = 1; addr < 127; ++addr) {

        bus.beginTransmission(addr);
        uint8_t err = bus.endTransmission();
        char buf[8]; snprintf(buf, sizeof(buf), "0x%02X", addr);
        if (err == 0) { 
          TUI::popupPrintln(String("  ") + tag + ": device at " + buf); ++found; 
        }        
        else if (err == 4) 
          TUI::popupPrintln(String("  ") + tag + ": unknown error at " + buf);

        delay(2);

      }
      
      if (!found) 
        TUI::popupPrintln(String("  ") + tag + ": no devices.");

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
    }

    else { 
      TUI::popupPrintln(String("Result: ") + (int)total + " device(s) total."); 
      TUI::setStatus(String("I2C: ") + (int)total); 
    }

    TUI::popupPrintln("------------------------------------------");

  }

  void cbAbout() { 
    TUI::setStatus("About"); 
    TUI::log("Jumperless RP2350B – DOS-style TUI (BETA, bugs are expected!)"); 
  }

  void cbResizeToTerminal() {
    
    uint16_t r=0,c=0;
    if (TUI::probeTerminalSize(r,c) && r>0 && c>0) { 
  
      TUI::resizeTo(r,c); 
      TUI::fullRedraw(); 
      TUI::log(String("Resized to ")+r+"x"+c+"."); 
      
    }
    else 
      TUI::log("Resize failed: terminal didn't answer CPR.");
  }

  void cbMenuNarrower(){ TUI::L.leftFracWide = max(0.10f, TUI::L.leftFracWide - 0.02f); TUI::resizeTo(TUI::S.rows, TUI::S.cols); TUI::fullRedraw(); }
  void cbMenuWider()   { TUI::L.leftFracWide = min(0.80f, TUI::L.leftFracWide + 0.02f); TUI::resizeTo(TUI::S.rows, TUI::S.cols); TUI::fullRedraw(); }
  void cbTightenGaps() { if (TUI::L.margin) TUI::L.margin--; if (TUI::L.gap) TUI::L.gap--; TUI::resizeTo(TUI::S.rows, TUI::S.cols); TUI::fullRedraw(); }
  void cbLoosenGaps()  { TUI::L.margin++; TUI::L.gap++; TUI::resizeTo(TUI::S.rows, TUI::S.cols); TUI::fullRedraw(); }
  void cbScreenResetLayout() { TUI::L.leftFracWide=0.15f; TUI::L.leftFracNarrow=0.50f; TUI::L.margin=2; TUI::L.gap=2; TUI::resizeTo(TUI::S.rows, TUI::S.cols); TUI::fullRedraw(); TUI::log("Screen layout reset."); }

  // Border menu callbacks
  void cbBorderAscii()   { applyBorder(TUI::BORDER_ASCII); }
  void cbBorderLight()   { applyBorder(TUI::BORDER_LIGHT); }
  void cbBorderRounded() { applyBorder(TUI::BORDER_ROUNDED); }
  void cbBorderHeavy()   { applyBorder(TUI::BORDER_HEAVY); }
  void cbBorderDouble()  { applyBorder(TUI::BORDER_DOUBLE); }
  
  void cbBorderCycle() {
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

  void cbFreePythonMemory() {
  
    TUI::logPrintln("_______________________________________________________________________________");
    TUI::logPrintln("Freeing MicroPython memory…" + String( rp2040.getTotalHeap( ) ));

    TUI::logPrintln( "Free memory: " + String( rp2040.getFreeHeap( ) ) );
    deinitMicroPythonProper( );
    TUI::logPrintln( "MicroPython deinitialized. Memory freed." );

    TUI::logPrintln( "Total memory: " + String( rp2040.getTotalHeap( ) ) );
    TUI::logPrintln( "Free memory: " + String( rp2040.getFreeHeap( ) ) );

    TUI::logPrintln("_______________________________________________________________________________");
  } 

}
// end callbacks namespace

void cbRunFileManager() {
  TUI::setStatus("File Manager");
  TUI::PopupFileManager::runModal("/");
  TUI::fullRedraw();
  TUI::setStatus("Returned from File Manager");
}


// ---------- Small helpers ----------
bool TuiGlue::isNoiseByte(int ch) {
  if (ch < 0) 
    return true;

  switch (ch) {
    case '\r': case '\n': case 0x00: case 0x05: case 0x07:
    case 0x11: case 0x13: case 0x03: return true;
  }
  
  if (ch < 0x20 && ch != 0x1B) 
    return true; // keep ESC

  return false;
}

void TuiGlue::drainHost(Stream* s, uint32_t forMs) {

  if (!s) 
    return;
  uint32_t tEnd = millis() + forMs;
  
  do {
    while (s->available()) 
      s->read();

    if (forMs && (int32_t)(millis() - tEnd) < 0) 
      delay(2);
  } while (forMs && (int32_t)(millis() - tEnd) < 0);

}

// ---------- Ctor ----------
TuiGlue::TuiGlue(Stream* serial) : m_serial(serial) {
  if (!m_serial) 
    m_serial = &USBSer3;   // default UI CDC port
}

// ---------- Basic API ----------
void TuiGlue::setSerial(Stream* s) {
  m_serial = s ? s : &USBSer3; TUI::TUIserial = m_serial;
  TUI::popupSetSerial(m_serial);

}

bool TuiGlue::isActive() const { return m_active; }

void TuiGlue::init() {
  if (m_modelBuilt) 
    return;
  
  m_modelBuilt = true;

  TUI::TUIserial = m_serial;
  TUI::popupSetSerial(m_serial);

  TUI::S.appTitle = "Jumperless V5 – RP2350B";
  TUI::S.status   = "Ready.";

  // --- Menus (typed items + separators) ---
  static const TUI::MenuItem SETTINGS_SCREEN_BORDER_ITEMS[] = {
    TUI_ACTION("&Light",   &cbBorderLight),
    TUI_ACTION("&Rounded",   &cbBorderRounded),
    TUI_ACTION("&Heavy",   &cbBorderHeavy),
    TUI_ACTION("&Double",   &cbBorderDouble),
    TUI_SEPARATOR_ENTRY(),
    TUI_ACTION("C&ycle styles", &cbBorderCycle),
  };

  static const TUI::MenuItem SETTINGS_SCREEN_ITEMS[] = {
    TUI_SUBMENU_ENTRY("&Borders", SETTINGS_SCREEN_BORDER_ITEMS, "Settings / Screen / Borders"),
    TUI_SEPARATOR_ENTRY(),
    TUI_ACTION("Resize to Terminal", &cbResizeToTerminal),
    TUI_ACTION("Menu Narrower",      &cbMenuNarrower),
    TUI_ACTION("Menu Wider",         &cbMenuWider),
    TUI_ACTION("Tighten Gaps",       &cbTightenGaps),
    TUI_ACTION("Loosen Gaps",        &cbLoosenGaps),
    TUI_SEPARATOR_ENTRY(),
    TUI_ACTION("Reset Layout",       &cbScreenResetLayout),
  };

  static const TUI::MenuItem SETTINGS_ITEMS[] = {
    TUI_SUBMENU_ENTRY("Screen", SETTINGS_SCREEN_ITEMS, "Settings / Screen"),
    TUI_SEPARATOR_ENTRY(),
    TUI_ACTION("Toggle OLED", &cbToggleOLED),
    TUI_ACTION("About",       &cbAbout),
  };

  static const TUI::MenuItem PYTHON_ITEMS[] = {
    TUI_ACTION("&MicroPython", &cbMicroPython),
    TUI_ACTION("&Free MicroPython Memory", &cbFreePythonMemory),
  };

  static const TUI::MenuItem TOOL_ITEMS[] = {
    TUI_ACTION("I2C Scanner", &cbI2CScan),
  };

  static const TUI::MenuItem MAIN_ITEMS[] = {
    TUI_ACTION("&Add Connections",       &cbAddNet),
    TUI_ACTION("&Remove Connections",    &cbRemoveNet),
    TUI_ACTION("&Clear All Connections", &cbClearAllConnections),
    TUI_SEPARATOR_ENTRY(), // << separator between Clear All and Python
    TUI_SUBMENU_ENTRY("&Python",  PYTHON_ITEMS,   "Python"),
    TUI_SUBMENU_ENTRY("&Tools",   TOOL_ITEMS,     "Tools"),
    TUI_SUBMENU_ENTRY("Se&ttings", SETTINGS_ITEMS,"Settings"),
    TUI_ACTION("File Manager", &cbRunFileManager),
    
  };

  TUI::pushMenu(MAIN_ITEMS, TUI_ARRLEN(MAIN_ITEMS), "Main");
}

void TuiGlue::checkUSBconnection() {

  if (m_serial == &USBSer3) {
    bool d = USBSer3.dtr();
    if (d != m_lastDTR) {
      m_lastDTR = d;
      m_needDeferredRedraw = true;
      m_redrawTries        = 12;
      m_nextRetryAtMs      = millis() + 10;
      m_seenHostByte       = false;
      TUI::fullRedraw();
      m_ignoreHostUntilMs  = millis() + 150;
      drainHost(m_serial, 20);
    }
  }

}

void TuiGlue::activate() {
  if (!m_modelBuilt) init();

  if (m_serial == &USBSer3) {
    uint32_t t0 = millis();
    while (!USBSer3.dtr() && (millis() - t0 < 1200)) { delay(10); }
    m_lastDTR = USBSer3.dtr();
  }

  m_serial->print("\r\n");
  m_serial->flush();
  delay(10);

  drainHost(m_serial, 20);
  m_ignoreHostUntilMs = millis() + 150;

  m_active = true;
  TUI::fullRedraw();

  uint16_t r=0,c=0;
  if (TUI::probeTerminalSize(r,c) && r>0 && c>0) {
    TUI::resizeTo(r,c);
    TUI::fullRedraw();
    m_needDeferredRedraw = false;
  } else {
    m_needDeferredRedraw = true;
    m_seenHostByte       = false;
    m_redrawTries        = 12;
    m_nextRetryAtMs      = millis() + 10;
  }

  TUI::log(
    "Use \x1b[1mUp/Down\x1b[0m to navigate, \x1b[1mEnter\x1b[0m to select, \x1b[1mESC\x1b[0m to go back. "
    "In popups: \x1b[1m↑/↓\x1b[0m recalls command history."
  );
}

void TuiGlue::openOnDemand() { activate(); }

void TuiGlue::loop() {
  if (!m_modelBuilt) init();
  if (!m_active)     activate();
  if (!m_active)     return;

  checkUSBconnection();

  bool inQuarantine = (int32_t)(millis() - m_ignoreHostUntilMs) < 0;
  if (inQuarantine) 
    drainHost(m_serial);
  
  if (m_serial->available()) 
    m_seenHostByte = true;

  uint16_t spins = 0;
  const bool popupNow = TUI::isPopupActive();

  if (popupNow) {
  
    while (m_serial->available() && spins < 64) {
      
      int ch = m_serial->read();
      if (!inQuarantine) {
        if (ch == 0x1B) {
          int ch1 = -1, ch2 = -1;
          if (m_serial->available()) ch1 = m_serial->read();
          if (ch1 == '[' || ch1 == 'O') {
            if (m_serial->available()) ch2 = m_serial->read();
            if (ch2 == 'A' || ch2 == 'B') {
              String repl;
              bool ok = (ch2 == 'A') ? histPrev(repl) : histNext(repl);
              if (ok) 
                TUI::popupSetInitial(repl);

              spins++; 
              if ((spins & 0x07) == 0) 
                delayMicroseconds(250);
              
              continue;
            } else {
              
              TUI::popupHandleKey(0x1B);
              
              if (ch1 != -1) 
                TUI::popupHandleKey(ch1);

              if (ch2 != -1) 
                TUI::popupHandleKey(ch2);
              
              spins++; 
              if ((spins & 0x07) == 0) 
                delayMicroseconds(250);

              continue;
            }
          } else {
            
            TUI::popupHandleKey(0x1B);
            
            if (ch1 != -1) 
              TUI::popupHandleKey(ch1);
            spins++; 
            
            if ((spins & 0x07) == 0) 
              delayMicroseconds(250);
            
            continue;

          }
        }
        
        if (ch == '\r' || ch == '\n') {
          if (m_serial->available()) {
            int next = m_serial->peek();
            if ((ch == '\r' && next == '\n') || (ch == '\n' && next == '\r')) 
              (void)m_serial->read();

          }
          TUI::popupHandleKey('\n');
        } else {
          if (ch == 0x7F) ch = 0x08;

          if (ch == 0x08 || !isNoiseByte(ch)) 
            TUI::popupHandleKey(ch);

        }
      }
      spins++; if ((spins & 0x07) == 0) delayMicroseconds(250);
    }
    TUI::popupDrawIfDirty();
  } else {

    if (m_wasPopup && !popupNow) {
    
      m_ignoreHostUntilMs = millis() + 120;
      drainHost(m_serial, 10);
      inQuarantine = true;

    }
    if (!inQuarantine) {
      
      while ((m_serial->available() || TUI::inEscapeSeq()) && spins < 8) {
        TUI::handleInput();
        spins++; if ((spins & 0x07) == 0) delayMicroseconds(250);
      }

    } else {
      
      drainHost(m_serial);

    }

    if (m_needDeferredRedraw && (int32_t)(millis() - m_nextRetryAtMs) >= 0) {
      bool done = false; uint16_t rr=0, cc=0;
      if (TUI::probeTerminalSize(rr, cc) && rr>0 && cc>0) { 
        TUI::resizeTo(rr, cc); 
        TUI::fullRedraw(); 
        done = true; 
      }
      else if (m_seenHostByte) {
        if (TUI::probeTerminalSize(rr, cc) && rr>0 && cc>0) { 
          TUI::resizeTo(rr, cc); 
          TUI::fullRedraw(); 
          done = true; 
        }
      }

      if (done || m_redrawTries == 0) 
        m_needDeferredRedraw = false;
      else { 
        m_redrawTries--; 
        m_nextRetryAtMs = millis() + 10; 
      }
    }

    if (TUI::S.logDirty) 
      TUI::drawLog();
  }

  m_wasPopup = popupNow;
  m_serial->flush();
}

