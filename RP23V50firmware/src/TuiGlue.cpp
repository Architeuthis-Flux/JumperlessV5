// RP23V50firmware/src/TuiGlue.cpp
#include "Tui.h"
#include "TuiGlue.h"

#include <Arduino.h>
#include <Wire.h>
#include "ArduinoStuff.h"
#include "LogicAnalyzer.h"
#include "oled.h"
#include "CH446Q.h"
#include "NetsToChipConnections.h"
#include "Commands.h"
#include "FileParsing.h"

using namespace TuiGlue;

Stream* TuiGlue::TUIserial = &USBSer3;
extern int netSlot;

namespace TUI {
  namespace {
    struct PopupState {
      bool     active      = false;
      bool     dirty       = false; 
      String   title;
      static constexpr uint16_t MAX_LINES = 160;
      static constexpr uint16_t PAD       = 2;
      String   lines[MAX_LINES];
      uint16_t lineCount   = 0;   
      uint16_t scroll      = 0;   
      uint32_t autoCloseAt = 0;

      int16_t  userBoxH = -1, userBoxW = -1;    
      float    userFracH = 0.0f, userFracW = 0.0f;

      uint16_t boxR=1, boxC=1, boxH=0, boxW=0;
    } POP;

    struct BorderChars {
      const char* tl; const char* tr; const char* bl; const char* br;
      const char* h;  const char* v;
    };

    inline void mv(uint16_t r, uint16_t c) {
      TuiGlue::TUIserial->print("\x1b["); TuiGlue::TUIserial->print(r);
      TuiGlue::TUIserial->print(";");     TuiGlue::TUIserial->print(c);
      TuiGlue::TUIserial->print("H");
    }
    inline void hline(uint16_t len, char ch) {
      for (uint16_t i=0;i<len;i++) TuiGlue::TUIserial->write(ch);
    }
    inline void hlineGlyph(uint16_t len, const char* g) {
      for (uint16_t i=0;i<len;i++) TuiGlue::TUIserial->print(g);
    }

    BorderChars currentBorderChars() {
      switch (THEME.borderStyle) {
        case BORDER_LIGHT:   return {"┌","┐","└","┘","─","│"};
        case BORDER_ROUNDED: return {"╭","╮","╰","╯","─","│"};
        case BORDER_HEAVY:   return {"┏","┓","┗","┛","━","┃"};
        case BORDER_DOUBLE:  return {"╔","╗","╚","╝","═","║"};
        case BORDER_ASCII:
        default:             return {"+","+","+","+","-","|"};
      }
    }

    // -------- Box sizing ----------
    void computeBox(uint16_t& r0, uint16_t& c0, uint16_t& h, uint16_t& w) {
      uint16_t rows = TUI::S.rows ? TUI::S.rows : 24;
      uint16_t cols = TUI::S.cols ? TUI::S.cols : 80;

      // Bounds for the OUTER box (including borders)
      const uint16_t minW = 12, minH = 6;
      const uint16_t maxW = (cols > 2) ? (uint16_t)(cols - 2) : cols;
      const uint16_t maxH = (rows > 2) ? (uint16_t)(rows - 2) : rows;

      auto autoWidth = [&](){
        // Auto width = min(80% of terminal, longest logical line + padding + borders, >=30)
        uint16_t maxLen = 0;
        for (uint16_t i=0;i<POP.lineCount;i++) {
          uint16_t L = (uint16_t)POP.lines[i].length();
          if (L > maxLen) maxLen = L;
        }
        const uint16_t maxBoxW = (uint16_t)((cols * 4) / 5); // 80%
        uint16_t ww = (uint16_t)min<uint16_t>(maxBoxW, max<uint16_t>(30, (uint16_t)(maxLen + 2*POP.PAD + 2)));
        return (uint16_t)constrain(ww, minW, maxW);
      };

      auto autoHeight = [&](){
        // Auto height ~60% of terminal (clamped)
        uint16_t hh = (uint16_t)min<uint16_t>((rows * 3) / 5, (uint16_t)(rows - 2));
        return (uint16_t)constrain(hh, minH, maxH);
      };

      // --- Width decision (outer box cols) ---
      if (POP.userBoxW >= 0) {
        w = (uint16_t)constrain((uint16_t)POP.userBoxW, minW, maxW);
      } else if (POP.userFracW > 0.0f && POP.userFracW <= 1.0f) {
        w = (uint16_t)constrain((uint16_t)(cols * POP.userFracW), minW, maxW);
      } else {
        w = autoWidth();
      }

      // --- Height decision (outer box rows) ---
      if (POP.userBoxH >= 0) {
        h = (uint16_t)constrain((uint16_t)POP.userBoxH, minH, maxH);
      } else if (POP.userFracH > 0.0f && POP.userFracH <= 1.0f) {
        h = (uint16_t)constrain((uint16_t)(rows * POP.userFracH), minH, maxH);
      } else {
        h = autoHeight();
      }

      // Center
      r0 = (uint16_t)((rows > h) ? ((rows - h) / 2 + 1) : 1);
      c0 = (uint16_t)((cols > w) ? ((cols - w) / 2 + 1) : 1);

      POP.boxR = r0; POP.boxC = c0; POP.boxH = h; POP.boxW = w;
    }

    void drawBorder(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
      BorderChars bc = currentBorderChars();

      // top
      mv(r0, c0);                TuiGlue::TUIserial->print(bc.tl);
      hlineGlyph((uint16_t)(w-2), bc.h);
      TuiGlue::TUIserial->print(bc.tr);

      // sides
      for (uint16_t r=1; r<h-1; r++) {
        mv(r0 + r, c0);          TuiGlue::TUIserial->print(bc.v);
        mv(r0 + r, c0+w-1);      TuiGlue::TUIserial->print(bc.v);
      }

      // bottom
      mv(r0 + h - 1, c0);        TuiGlue::TUIserial->print(bc.bl);
      hlineGlyph((uint16_t)(w-2), bc.h);
      TuiGlue::TUIserial->print(bc.br);
    }

    void drawTitle(uint16_t r0, uint16_t c0, uint16_t w) {
      String t = " " + POP.title + " ";
      uint16_t tstart = (uint16_t)(c0 + (w - t.length())/2);
      mv(r0, tstart); TuiGlue::TUIserial->print(t);
    }
    void drawHelp(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
      const char* help = "↑/↓ PgUp/PgDn scroll — ESC close";
      uint16_t len = (uint16_t)strlen(help);
      uint16_t tstart = (uint16_t)(c0 + (w - len)/2);
      mv(r0 + h - 1, tstart); TuiGlue::TUIserial->print(help);
    }

    // -------- Word wrap helpers ----------
    uint16_t countWrappedRows(const String& S, uint16_t innerW) {
      if (innerW == 0) return 0;
      if (S.length() == 0) return 1; // blank consumes a row
      uint16_t rows = 0;
      uint16_t p = 0, n = (uint16_t)S.length();
      while (p < n) {
        uint16_t q = p, used = 0; bool first = true;
        while (q < n && S[q] == ' ') q++; // skip leading spaces
        if (q >= n) { rows++; break; }
        while (true) {
          uint16_t t = q; while (t < n && S[t] != ' ') t++;
          uint16_t tok = (uint16_t)(t - q);
          if (first) {
            if (tok > innerW) { rows++; p = (uint16_t)(q + innerW); break; }
            used = tok; first = false; q = t;
          } else {
            if ((uint16_t)(used + 1 + tok) <= innerW) { used = (uint16_t)(used + 1 + tok); q = t; }
            else { rows++; p = q; break; }
          }
          while (q < n && S[q] == ' ') q++;
          if (q >= n) { rows++; p = q; break; }
        }
      }
      return rows;
    }

    void normalizeScroll(uint16_t innerH, uint16_t innerW) {
      if (innerW == 0) { POP.scroll = 0; return; }
      uint16_t total = 0;
      for (uint16_t i=0;i<POP.lineCount;i++)
        total = (uint16_t)(total + countWrappedRows(POP.lines[i], innerW));
      if (total <= innerH) { POP.scroll = 0; return; }
      if (POP.scroll + innerH > total) POP.scroll = (uint16_t)(total - innerH);
    }

    void drawWrappedContent(uint16_t rStart, uint16_t cStart, uint16_t innerH, uint16_t innerW) {
      // clear content region
      for (uint16_t i=0;i<innerH;i++) { mv(rStart + i, cStart); hline(innerW, ' '); }

      uint16_t skip = POP.scroll, printed = 0;

      for (uint16_t li=0; li<POP.lineCount && printed < innerH; li++) {
        const String& S = POP.lines[li];
        uint16_t n = (uint16_t)S.length();
        if (innerW == 0) break;

        if (n == 0) {
          if (skip > 0) { skip--; } else { mv(rStart + printed, cStart); printed++; }
          continue;
        }

        uint16_t p = 0;
        while (p < n && printed < innerH) {
          String out; out.reserve(innerW);
          uint16_t q = p; while (q < n && S[q] == ' ') q++; // trim leading
          if (q >= n) { if (skip > 0) skip--; else { mv(rStart + printed, cStart); printed++; } p = q; break; }

          bool first = true;
          while (true) {
            uint16_t t0 = q, t1 = q; while (t1 < n && S[t1] != ' ') t1++;
            uint16_t tok = (uint16_t)(t1 - t0);
            if (first) {
              if (tok > innerW) { out = S.substring(t0, (uint16_t)(t0 + innerW)); q = (uint16_t)(t0 + innerW); p = q; break; }
              out = S.substring(t0, t1); q = t1; first = false;
            } else {
              if ((uint16_t)(out.length() + 1 + tok) <= innerW) { out += ' '; out += S.substring(t0, t1); q = t1; }
              else break;
            }
            while (q < n && S[q] == ' ') q++;
            if (q >= n) { p = q; break; }
          }

          if (skip > 0) skip--;
          else { mv(rStart + printed, cStart); if (out.length() > 0) TuiGlue::TUIserial->print(out); printed++; }

          if (p < q) p = q;
        }
      }
    }

    bool handleArrowSeq() {
      char b[4] = {0,0,0,0};
      int n = 0;
      while (TuiGlue::TUIserial->available() && n < 3) b[n++] = (char)TuiGlue::TUIserial->read();

      auto lineUp   = [](){ if (POP.scroll>0) { POP.scroll--; POP.dirty = true; } };
      auto lineDown = [](){
        uint16_t innerW = (uint16_t)(POP.boxW > 0 ? (POP.boxW - 2 - 2*POP.PAD) : 60);
        uint16_t innerH = (uint16_t)(POP.boxH > 4 ? (POP.boxH - 4) : 8);
        POP.scroll++;
        normalizeScroll(innerH, innerW);
        POP.dirty = true;
      };
      auto page = [&](int dir){
        uint16_t innerW = (uint16_t)(POP.boxW > 0 ? (POP.boxW - 2 - 2*POP.PAD) : 60);
        uint16_t innerH = (uint16_t)(POP.boxH > 4 ? (POP.boxH - 4) : 8);
        if (dir < 0) {
          POP.scroll = (POP.scroll > innerH) ? (uint16_t)(POP.scroll - innerH) : 0;
        } else {
          POP.scroll = (uint16_t)(POP.scroll + innerH);
          normalizeScroll(innerH, innerW);
        }
        POP.dirty = true;
      };

      if (n >= 1 && b[0] == 'A') { lineUp();   return true; }   // Up
      if (n >= 1 && b[0] == 'B') { lineDown(); return true; }   // Down
      if (n >= 2 && b[0] == '5' && b[1] == '~') { page(-1); return true; } // PgUp
      if (n >= 2 && b[0] == '6' && b[1] == '~') { page(+1); return true; } // PgDn
      return true;
    }
  } // anon

  // ---------- Public popup API ----------
  // boxRows/boxCols: absolute outer box size (incl. borders), -1 = auto
  // fracRows/fracCols: 0..1 fraction of terminal, used if absolute not set
  void popupOpen(const String& title,
                 int16_t boxRows = -1, int16_t boxCols = -1,
                 float fracRows = 0.0f, float fracCols = 0.0f) {
    POP.active = true;
    POP.title  = title;
    POP.lineCount = 0;
    POP.scroll    = 0;      // visual rows
    POP.autoCloseAt = 0;

    POP.userBoxH = boxRows;
    POP.userBoxW = boxCols;
    POP.userFracH = fracRows;
    POP.userFracW = fracCols;

    POP.dirty = true; // first paint
  }

  void popupClose() {
    POP.active = false;
    POP.autoCloseAt = 0;
    POP.dirty = false;
    // restore the underlying UI immediately
    TUI::fullRedraw();
  }

  void popupClear() { POP.lineCount = 0; POP.scroll = 0; POP.dirty = true; }

  namespace {
    inline bool endsWithNewline(const String& s) {
      int n = (int)s.length();
      return n > 0 && (s[n-1] == '\n' || s[n-1] == '\r');
    }
  }


  void popupPrint(const String& s) {
    uint16_t start = 0;
    while (start < s.length()) {
      int nl = s.indexOf('\n', start);
      String line = (nl >= 0) ? s.substring(start, nl) : s.substring(start);
      if (POP.lineCount < POP.MAX_LINES) {
        POP.lines[POP.lineCount++] = line;
      } else {
        for (uint16_t i=1;i<POP.MAX_LINES;i++) POP.lines[i-1] = POP.lines[i];
        POP.lines[POP.MAX_LINES-1] = line;
      }
      if (nl < 0) break;
      start = (uint16_t)(nl + 1);
    }
    POP.dirty = true;
  }

  void popupPrintln(const String& s) {
    if (endsWithNewline(s)) popupPrint(s);
    else                    popupPrint(s + "\n");
  }

  void popupAutoCloseMs(uint32_t ms) { POP.autoCloseAt = ms ? (millis() + ms) : 0; POP.dirty = true; }

  bool isPopupActive() { return POP.active; }

  void drawPopup() {
    if (!POP.active) return;

    uint16_t r0,c0,h,w;
    computeBox(r0,c0,h,w);

    // Inner geometry
    uint16_t innerW = (uint16_t)(w - 2 - 2*POP.PAD);
    uint16_t innerH = (uint16_t)(h - 4);

    normalizeScroll(innerH, innerW);

    drawBorder(r0,c0,h,w);
    drawTitle(r0,c0,w);
    drawHelp(r0,c0,h,w);

    uint16_t rStart = (uint16_t)(r0 + 2);
    uint16_t cStart = (uint16_t)(c0 + 1 + POP.PAD);
    drawWrappedContent(rStart, cStart, innerH, innerW);
  }

  // Public helper so outer code doesn’t touch internal POP
  void popupDrawIfDirty() {
    if (POP.active && POP.dirty) {
      drawPopup();
      POP.dirty = false;
    }
  }

  bool popupHandleKey(int ch) {
    if (!POP.active) return false;

    // Timeout path: allow closing even without keypresses
    if (POP.autoCloseAt && (int32_t)(millis() - POP.autoCloseAt) >= 0) {
      popupClose(); 
      return true;
    }

    // ESC-prefixed sequences (arrows/PgUp/PgDn) — handle BEFORE treating ESC as "close"
    if (ch == 0x1B) {
      // Tiny coalescing wait so ESC-[ / ESC-O and the following byte arrive
      int next = -1;
      uint32_t t0 = millis();
      while (!TuiGlue::TUIserial->available() && (millis() - t0) < 10) { /* wait ~10ms */ }
      if (TuiGlue::TUIserial->available()) next = TuiGlue::TUIserial->read();

      if (next == '[') {
        // CSI sequences: ESC [ A/B (arrows), ESC [ 5~ / 6~ (PgUp/Dn)
        return handleArrowSeq(); // scrolls and sets POP.dirty
      }

      if (next == 'O') {
        // Application-cursor mode: ESC O A/B (up/down)
        int b = -1;
        uint32_t t1 = millis();
        while (!TuiGlue::TUIserial->available() && (millis() - t1) < 10) { }
        if (TuiGlue::TUIserial->available()) b = TuiGlue::TUIserial->read();

        if (b == 'A') { // Up
          if (POP.scroll > 0) { POP.scroll--; POP.dirty = true; }
          return true;
        } else if (b == 'B') { // Down
          uint16_t innerW = (uint16_t)(POP.boxW > 0 ? (POP.boxW - 2 - 2*POP.PAD) : 60);
          uint16_t innerH = (uint16_t)(POP.boxH > 4 ? (POP.boxH - 4) : 8);
          POP.scroll++;
          normalizeScroll(innerH, innerW);
          POP.dirty = true;
          return true;
        }
        return true; // swallow unknown ESC O ? sequences
      }

      // Standalone ESC → close popup
      if (next < 0) {
        popupClose();
        return true;
      }
      return true; // swallow any other ESC sequence
    }

    // Swallow everything else while modal
    return true;
  }
} // namespace TUI

/* ===============================
   App callbacks (menu actions)
   =============================== */

// I2C scanner: popup-only output; status updates
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

  if (oled.isConnected() == false) {
    oled.init();
    TUI::log("OLED " + String(TUI::oledEnabled ? "ON" : "OFF"));

  } else {
    oled.disconnect();
  }

  delay(100);


  oled.clearPrintShow("OLED " + String(TUI::oledEnabled ? "ON" : "OFF"),
                      1, true, true, true, -1, -1, 1500);


  TUI::setOledEnabled(!TUI::oledEnabled);
  TUI::setStatus(TUI::oledEnabled ? "OLED ON" : "OLED OFF");
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

  TUI::TUIserial = TuiGlue::TUIserial;

  TUI::S.appTitle = "Jumperless V5 – RP2350B";
  TUI::S.status   = "Ready.";

  TUI::pushMenu(MAIN_ITEMS, MAIN_ITEMS_N, "Main");
}

static void activate() {
  if (!s_modelBuilt)
    TuiGlue::init();

  // Keep engine+glue on the same stream
  TUI::TUIserial = TuiGlue::TUIserial;

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
      (void)TUI::popupHandleKey(ch);
      spins++;
      if ((spins & 0x07) == 0) delayMicroseconds(250);
    }

    TUI::popupDrawIfDirty();

  } else {
    // Normal input path
    while ((TuiGlue::TUIserial->available() || TUI::inEscapeSeq()) && spins < 2) {

      (void)TUI::handleInput();
      spins++;
      if ((spins & 0x07) == 0) delayMicroseconds(250);
    }


    // Deferred probe to avoid the "press Enter to draw" issue
    if (s_needDeferredRedraw && (int32_t)(millis() - s_nextRetryAtMs) >= 0) {
      bool done = false;

      uint16_t rr=0, cc=0;

      if (TUI::probeTerminalSize(rr, cc) && rr>0 && cc>0) {
        TUI::resizeTo(rr, cc);
        TUI::fullRedraw();
        done = true;
      } else if (s_seenHostByte) {
        // Terminal spoke at least once; try again immediately
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
