/*
  tui:
  - Text User Interface (TUI) for Jumperless
  - Draws to the serial terminal, handles input, menus, popups, logging
  - Most of this lives inside namespace TUI and is driven by TuiGlue
  - You can check isActive() via TuiGlue; this header provides the TUI core
*/

#pragma once
#include <Arduino.h>
#include "ArduinoStuff.h"
#include "TuiPopup.h"
#include <cstdarg>

// ========= Configurable =========
#ifndef TUI_LOGPF_BUF
  #define TUI_LOGPF_BUF 500   // scratch buffer for printf-style logging
#endif

namespace TUI {

// ========= Menu model =========
struct MenuItem;

enum ItemKind : uint8_t {
  ITEM_ACTION    = 0,
  ITEM_SUBMENU   = 1,
  ITEM_SEPARATOR = 2
};

struct MenuView {
  const MenuItem* items = nullptr;
  uint8_t count    = 0;
  int8_t  selected = 0;
  uint8_t top      = 0;
  const char* title = nullptr;
};

struct MenuItem {
  const char*    label;
  void         (*onEnter)();
  const MenuItem* children  = nullptr;
  uint8_t        childCount = 0;
  const char*    childTitle = nullptr;
  ItemKind       kind       = ITEM_ACTION;  // default keeps old initializers working
};

// Convenience macros (backward-compatible with existing code)
#ifndef TUI_ARRLEN
  #define TUI_ARRLEN(a) (uint8_t)(sizeof(a)/sizeof((a)[0]))
#endif

#ifndef TUI_ACTION
  #define TUI_ACTION(label, cb) \
    { (label), (cb), nullptr, 0, nullptr, TUI::ITEM_ACTION }
#endif

#ifndef TUI_SEPARATOR_ENTRY
  #define TUI_SEPARATOR_ENTRY() \
    { "", nullptr, nullptr, 0, nullptr, TUI::ITEM_SEPARATOR }
#endif

#ifdef TUI_SUBMENU_ENTRY
  #undef TUI_SUBMENU_ENTRY
#endif
#define TUI_SUBMENU_ENTRY(label, arr, title) \
  { (label), nullptr, (arr), (uint8_t)(sizeof(arr)/sizeof((arr)[0])), (title), TUI::ITEM_SUBMENU }

constexpr uint8_t MENU_STACK_MAX = 6;
struct MenuStack { MenuView stack[MENU_STACK_MAX]; int8_t depth = -1; };

// ========= Globals / state (header-safe via inline) =========
inline Stream* TUIserial = &USBSer3;  // single global serial for TUI

#ifndef TUI_DEBUG_MODE
  #define TUI_DEBUG_MODE 0
#endif

// ========= Popup convenience wrappers (delegate to TuiPopup) =========
inline void popupOpenInput(const String& title, const String& prompt)   { Popup::instance().openInput(title, prompt); }
inline void popupOnSubmit(Popup::SubmitFn fn)                           { Popup::instance().setOnSubmit(fn); }
inline void popupOnCancel(Popup::CancelFn fn)                           { Popup::instance().setOnCancel(fn); }
inline void popupSetInitial(const String& s)                            { Popup::instance().setInitialInput(s); }

// ========= Logging (visible log pane, no ring buffer) =========
inline void log(const String& line);
inline void logPrint(const String& s);
inline void logPrint(const char* s) { logPrint(String(s)); }
inline void logPrint(char c)        { String t; t += c; logPrint(t); }
inline void logPrintln(const String& s);

// printf-style helpers (fast path with small stack buffer)
inline void vlogprintf_(bool add_newline, const char* fmt, va_list ap_in) {
  char sbuf[TUI_LOGPF_BUF];

  va_list ap1; va_copy(ap1, ap_in);
  int n = vsnprintf(sbuf, sizeof(sbuf), fmt, ap1);
  va_end(ap1);
  if (n < 0) return;

  if (n < (int)sizeof(sbuf)) {
    if (add_newline) logPrintln(String(sbuf));
    else             logPrint(String(sbuf));
    return;
  }

  const size_t need = (size_t)n + 1;
  char* dyn = (char*)malloc(need);
  if (!dyn) { // OOM: emit truncated
    if (add_newline) logPrintln(String(sbuf));
    else             logPrint(String(sbuf));
    return;
  }

  va_list ap2; va_copy(ap2, ap_in);
  vsnprintf(dyn, need, fmt, ap2);
  va_end(ap2);

  if (add_newline) logPrintln(String(dyn));
  else             logPrint(String(dyn));
  free(dyn);
}
inline void logprintf(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt); vlogprintf_(false, fmt, ap); va_end(ap);
}
inline void logprintlnf(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt); vlogprintf_(true, fmt, ap); va_end(ap);
}

// ========= Debug flag =========
inline uint8_t& dbgMode() { static uint8_t m = (uint8_t)TUI_DEBUG_MODE; return m; }
inline void setDebugMode(uint8_t m) { dbgMode() = m; }
inline uint8_t getDebugMode()       { return dbgMode(); }

// ========= ANSI helpers =========
inline void W(const char* s)  { TUIserial->print(s); }
inline void W(char c)         { TUIserial->write(c); }
inline void P(const String& s){ TUIserial->print(s); }

inline void clr()        { W("\x1b[2J"); }
inline void home()       { W("\x1b[H"); }
inline void hideCursor() { W("\x1b[?25l"); }
inline void showCursor() { W("\x1b[?25h"); }
inline void rs()         { W("\x1b[0m"); }   // reset color
inline void invOn()      { W("\x1b[7m"); }
inline void invOff()     { W("\x1b[27m"); }
inline void boldOn()     { W("\x1b[1m"); }
inline void boldOff()    { W("\x1b[22m"); }

inline void at(uint8_t row, uint8_t col) {
  char buf[16];
  snprintf(buf, sizeof(buf), "\x1b[%u;%uH", (unsigned)row, (unsigned)col);
  W(buf);
}
inline void hline(uint8_t row, uint8_t col1, uint8_t col2, char ch=' ') {
  at(row, col1);
  for (uint8_t c = col1; c <= col2; ++c) W(ch);
}

// 256-color helpers
inline void fg256(uint8_t c){ char b[20]; snprintf(b,sizeof(b),"\x1b[38;5;%um",c); TUIserial->print(b); }
inline void bg256(uint8_t c){ char b[20]; snprintf(b,sizeof(b),"\x1b[48;5;%um",c); TUIserial->print(b); }

// ========= Theme & layout =========
enum BorderStyle : uint8_t { BORDER_ASCII, BORDER_LIGHT, BORDER_ROUNDED, BORDER_HEAVY, BORDER_DOUBLE };

struct Theme {
  uint8_t    borderFg   = 45;     // cyan
  bool       unicodeBox = true;   // ┌─┐/││/└─┘ vs ASCII
  uint8_t    hotkeyFg   = 196;    // red
  uint8_t    hotkeyBg   = 236;    // dark gray
  BorderStyle borderStyle = BORDER_DOUBLE;
};
inline Theme THEME;

struct LayoutCfg { float leftFracWide=0.15f; float leftFracNarrow=0.45f; uint8_t margin=2; uint8_t gap=2; };
inline LayoutCfg L;

struct LayoutLimits { uint8_t minMenu=15; uint8_t maxMenu=80; uint8_t minLog=24; };
inline LayoutLimits LIM;

// Unicode/ASCII framed box drawing (used for frames)
inline void boxColored(uint8_t r1, uint8_t c1, uint8_t r2, uint8_t c2, uint8_t color = 0xFF) {
  if (color != 0xFF) fg256(color); else fg256(THEME.borderFg);

  const bool ascii = !THEME.unicodeBox || THEME.borderStyle == BORDER_ASCII;
  const char *H, *V, *TL, *TR, *BL, *BR;

  if (ascii) {
    H = "-"; V = "|"; TL = "+"; TR = "+"; BL = "+"; BR = "+";
  } else {
    switch (THEME.borderStyle) {
      case BORDER_ROUNDED: H = "─"; V = "│"; TL = "╭"; TR = "╮"; BL = "╰"; BR = "╯"; break;
      case BORDER_HEAVY:   H = "━"; V = "┃"; TL = "┏"; TR = "┓"; BL = "┗"; BR = "┛"; break;
      case BORDER_DOUBLE:  H = "═"; V = "║"; TL = "╔"; TR = "╗"; BL = "╚"; BR = "╝"; break;
      case BORDER_LIGHT:
      default:             H = "─"; V = "│"; TL = "┌"; TR = "┐"; BL = "└"; BR = "┘"; break;
    }
  }

  at(r1, c1); TUIserial->print(TL);
  for (uint8_t c = c1 + 1; c < c2; ++c) TUIserial->print(H);
  TUIserial->print(TR);

  for (uint8_t r = r1 + 1; r < r2; ++r) {
    at(r, c1); TUIserial->print(V);
    at(r, c2); TUIserial->print(V);
  }

  at(r2, c1); TUIserial->print(BL);
  for (uint8_t c = c1 + 1; c < c2; ++c) TUIserial->print(H);
  TUIserial->print(BR);
  rs();
}

// ========= Screen state =========
struct Screen {
  uint8_t cols=80, rows=24;
  String  appTitle   = "Jumperless";
  String  status     = "Ready.";
  uint8_t headerRow  = 1;
  uint8_t statusRow  = 24;
  uint8_t menuTop=3,  menuLeft=2,  menuBottom=22, menuRight=46;
  uint8_t logTop =3,  logLeft =48, logBottom =22, logRight =79;
  MenuStack menus;

  // Logging & input
  bool     logDirty     = true;
  bool     inputActive  = false;
  String   inputTitle;
  String   inputPrompt;
  String   inputBuffer;
  void  (*onInputSubmit)(const String&) = nullptr;
  void  (*onInputCancel)()              = nullptr;
};
inline Screen S;

// ========= Forward decls =========
enum Key : uint8_t { NONE, ENTER, UP, DOWN, ESC, CHAR, UNKNOWN };

inline void drawMenuItems();
inline void drawHeader();
inline void drawStatus();
inline void drawLog();
inline void drawInputModal();
inline void setStatus(const String& s);
inline void setTitle(const String& s);
inline void pushMenu(const MenuItem* items, uint8_t count, const char* title=nullptr);
inline void popMenu();
inline void enterSelected();
inline void fullRedraw();
inline Key  readKey();
inline bool handleInput();

inline char  labelHotkeyAndRender(const char* src, String& rendered);
inline void  fgDefault();
inline int   lastChar();

// ========= Terminal size probe (CPR) =========
inline bool readCPR(uint16_t& row, uint16_t& col, uint32_t timeout_ms=800) {
  char buf[32]; size_t n=0; uint32_t t0=millis();
  while ((millis()-t0) < timeout_ms && n < sizeof(buf)-1) {
    if (!TUIserial->available()) { delayMicroseconds(300); continue; }
    char ch = (char)TUIserial->read();
    if (n==0 && ch!='\x1b') continue;
    buf[n++] = ch;
    if (ch=='R') break;
  }
  buf[n]=0; int r=0,c=0;
  return (sscanf(buf, "\x1b[%d;%dR", &r, &c)==2) ? (row=r, col=c, true) : false;
}

inline bool probeTerminalSize(uint16_t& rows, uint16_t& cols) {
  while (TUIserial->available()) TUIserial->read();
  TUIserial->print("\x1b[s");
  TUIserial->print("\x1b[9999;9999H");
  TUIserial->print("\x1b[6n");
  TUIserial->flush();
  bool ok = readCPR(rows, cols, 400);
  TUIserial->print("\x1b[u");
  TUIserial->flush();
  return ok;
}

inline void resizeTo(uint16_t rows, uint16_t cols) {
  if (rows < 16) rows = 16; if (rows > 250) rows = 250;
  if (cols < 60) cols = 60; if (cols > 250) cols = 250;
  S.rows = (uint8_t)rows; S.cols = (uint8_t)cols;

  S.headerRow  = 1;
  S.statusRow  = S.rows;
  S.menuTop    = 2;
  S.menuBottom = (uint8_t)(S.rows - 3);

  float leftFrac = (S.cols >= 120) ? L.leftFracWide : L.leftFracNarrow;
  if (leftFrac < 0.10f) leftFrac = 0.10f;
  if (leftFrac > 0.90f) leftFrac = 0.90f;

  const uint8_t margin = L.margin;
  const uint8_t gap    = L.gap;

  const uint8_t rightBound = (uint8_t)(S.cols - (margin + 1));
  const uint8_t usable     = (uint8_t)(rightBound - margin);

  uint8_t menuWidth = (uint8_t)(leftFrac * usable);
  const int16_t maxAllowed = (int16_t)usable - (int16_t)gap - (int16_t)LIM.minLog;
  if ((int16_t)menuWidth > maxAllowed) menuWidth = (maxAllowed > 14) ? (uint8_t)maxAllowed : 14;
  if (menuWidth < 14) menuWidth = 14;

  S.menuLeft  = margin;
  S.menuRight = (uint8_t)(S.menuLeft + menuWidth);
  S.logLeft   = (uint8_t)(S.menuRight + gap);
  S.logRight  = rightBound;
}

// ========= Visible log =========
constexpr uint8_t LOG_VIS_MAX = 80;
inline String  logVis[LOG_VIS_MAX];
inline uint8_t logVisUsed = 0;

inline uint8_t logRowsVisible() { return (uint8_t)((S.logBottom - 1) - (S.logTop + 1) + 1); }
inline uint8_t logColsVisible() { return (uint8_t)((S.logRight - 2) - (S.logLeft + 2) + 1); }
inline uint8_t logVisCap()      { uint8_t vis = logRowsVisible(); return (vis < LOG_VIS_MAX) ? vis : LOG_VIS_MAX; }

inline void ensureActiveLine() {
  if (logVisUsed == 0) { logVis[0] = ""; logVisUsed = 1; }
}
inline void clampToVisible() {
  uint8_t cap = logVisCap();
  ensureActiveLine();
  while (logVisUsed > cap) {
    for (uint8_t i = 1; i < logVisUsed; ++i) logVis[i-1] = logVis[i];
    --logVisUsed;
  }
  if (logVisUsed == 0) { logVis[0] = ""; logVisUsed = 1; }
}
inline void clearLogPaneInner() {
  for (uint8_t r = (uint8_t)(S.logTop + 1); r <= (uint8_t)(S.logBottom - 1); ++r) {
    at(r, (uint8_t)(S.logLeft + 1));
    for (uint8_t c = (uint8_t)(S.logLeft + 1); c <= (uint8_t)(S.logRight - 1); ++c) W(' ');
  }
}
inline void drawLog() {
  clampToVisible();
  clearLogPaneInner();

  const uint8_t used = logVisUsed;
  const uint8_t maxw = logColsVisible();
  const uint8_t firstRow = (uint8_t)((S.logBottom - 1) - (used - 1));

  for (uint8_t i = 0; i < used; ++i) {
    const String& s = logVis[i];
    at((uint8_t)(firstRow + i), (uint8_t)(S.logLeft + 2));
    P((s.length() <= maxw) ? s : (s.substring(0, maxw - 1) + "…"));
  }

  S.logDirty = false;
  TUIserial->flush();
}

inline void logPrint(const String& s) {
  clampToVisible();
  ensureActiveLine();
  logVis[(uint8_t)(logVisUsed - 1)] += s;

  const uint8_t maxw = logColsVisible();
  const uint8_t lastRow = (uint8_t)(S.logBottom - 1);
  const String& line = logVis[(uint8_t)(logVisUsed - 1)];

  at(lastRow, (uint8_t)(S.logLeft + 1));
  for (uint8_t c = (uint8_t)(S.logLeft + 1); c <= (uint8_t)(S.logRight - 1); ++c) W(' ');
  at(lastRow, (uint8_t)(S.logLeft + 2));
  P((line.length() <= maxw) ? line : (line.substring(0, maxw - 1) + "…"));

  TUIserial->flush();
}
inline void logPrintln(const String& s) {
  logPrint(s);
  const uint8_t cap = logVisCap();
  if (logVisUsed < cap) {
    logVis[logVisUsed++] = "";
  } else {
    for (uint8_t i = 1; i < logVisUsed; ++i) logVis[i-1] = logVis[i];
    logVis[(uint8_t)(logVisUsed - 1)] = "";
  }
  drawLog();
}
inline void log(const String& line) {
  ensureActiveLine();
  if (logVis[(uint8_t)(logVisUsed - 1)].isEmpty()) {
    logVis[(uint8_t)(logVisUsed - 1)] = line;
  } else {
    const uint8_t cap = logVisCap();
    if (logVisUsed < cap) {
      logVis[logVisUsed++] = line;
    } else {
      for (uint8_t i = 1; i < logVisUsed; ++i) logVis[i-1] = logVis[i];
      logVis[(uint8_t)(logVisUsed - 1)] = line;
    }
  }
  // make room for next active line
  const uint8_t cap = logVisCap();
  if (logVisUsed < cap) {
    logVis[logVisUsed++] = "";
  } else {
    for (uint8_t i = 1; i < logVisUsed; ++i) logVis[i-1] = logVis[i];
    logVis[(uint8_t)(logVisUsed - 1)] = "";
  }
  drawLog();
}

// ========= Drawing =========
inline void drawHeader() {
  at(S.headerRow,1);
  invOn();
  hline(S.headerRow,1,S.cols,' ');
  if (THEME.unicodeBox && THEME.borderStyle == BORDER_ROUNDED) {
    at(S.headerRow, 1);      W("╭");
    at(S.headerRow, S.cols); W("╮");
  }
  at(S.headerRow,3); boldOn(); P(S.appTitle); boldOff(); invOff(); rs();
  TUIserial->flush();
}
inline void drawStatus() {
  at(S.statusRow,1);
  invOn();
  hline(S.statusRow,1,S.cols,' ');
  if (THEME.unicodeBox && THEME.borderStyle == BORDER_ROUNDED) {
    at(S.statusRow, 1);      W("╰");
    at(S.statusRow, S.cols); W("╯");
  }
  at(S.statusRow,3); P(S.status); invOff(); rs();
  TUIserial->flush();
}
inline void drawMenuFrame() {
  boxColored(S.menuTop, S.menuLeft, S.menuBottom, S.menuRight);
  at(S.menuTop, S.menuLeft+2); fg256(THEME.borderFg); P(" Menu "); rs();
  TUIserial->flush();
}
inline void drawLogFrame() {
  boxColored(S.logTop, S.logLeft, S.logBottom, S.logRight);
  at(S.logTop, S.logLeft+2); fg256(THEME.borderFg); P(" Log "); rs();
  TUIserial->flush();
}

inline const MenuView& curMenu() { return S.menus.stack[S.menus.depth]; }
inline MenuView&       curMenuRW(){ return S.menus.stack[S.menus.depth]; }

// ========= Hotkeys & menu rendering =========
inline void fgDefault(){ W("\x1b[39m"); }

inline char labelHotkeyAndRender(const char* src, String& rendered){
  char hk = 0; rendered = ""; if (!src) return 0;
  bool seenAmp = false;

  for (const char* p = src; *p; ++p){
    char ch = *p;
    if (!seenAmp && ch == '&'){ seenAmp = true; continue; }
    if (seenAmp){
      if (!hk && ch > ' ') hk = ch; // exact case
      rendered += ch;
      seenAmp = false;
      continue;
    }
    rendered += ch;
  }
  return hk; // 0 => no explicit hotkey
}

inline bool isSeparator(const MenuItem& it)  { return it.kind == ITEM_SEPARATOR; }
inline bool isSelectable(const MenuItem& it) { return it.kind != ITEM_SEPARATOR; }

inline const char* hrGlyphForStyle() {
  const bool ascii = !THEME.unicodeBox || THEME.borderStyle == BORDER_ASCII;
  if (ascii) return "-";
  switch (THEME.borderStyle) {
    case BORDER_HEAVY:   return "━";
    case BORDER_DOUBLE:  return "═";
    case BORDER_ROUNDED: return "─";
    case BORDER_LIGHT:
    default:             return "─";
  }
}

inline int nextSelectableIndex(const MenuView& MV, int from, int dir) {
  if (MV.count == 0) return from;
  int i = from;
  for (uint16_t k = 0; k < MV.count; ++k) {
    i = (i + dir + MV.count) % MV.count;
    if (isSelectable(MV.items[i])) return i;
  }
  return from; // all separators
}

inline void drawMenuItems() {
  if (S.menus.depth < 0) return;
  MenuView& MRW = curMenuRW();
  const MenuView& M = MRW;

  // Clear inner
  for (uint8_t r = S.menuTop + 1; r <= S.menuBottom - 1; ++r) {
    at(r, S.menuLeft + 1);
    for (uint8_t c = S.menuLeft + 1; c <= S.menuRight - 1; ++c) W(' ');
  }

  const uint8_t vis = (uint8_t)((S.menuBottom - 1) - (S.menuTop + 1) + 1);
  if (MRW.selected < 0) MRW.selected = 0;
  if (M.count && MRW.selected >= M.count) MRW.selected = M.count - 1;
  if (MRW.selected < MRW.top) MRW.top = (uint8_t)MRW.selected;
  if (MRW.selected >= (int)(MRW.top + vis)) MRW.top = (uint8_t)(MRW.selected - vis + 1);

  if (M.title) {
    const uint8_t titleCol  = (uint8_t)(S.menuLeft + 10);
    const uint8_t maxTitleW = (titleCol < (S.menuRight - 2))
        ? (uint8_t)((S.menuRight - 2) - titleCol + 1) : 0;
    if (maxTitleW > 0) {
      String ttl = String("[ ") + M.title + " ]";
      if (ttl.length() > maxTitleW) ttl = (maxTitleW > 1) ? (ttl.substring(0, maxTitleW - 1) + "…") : "";
      at(S.menuTop, titleCol); P(ttl);
    }
  }

  const uint8_t textCol  = (uint8_t)(S.menuLeft + 3);
  const uint8_t textMaxC = (textCol <= (S.menuRight - 1))
      ? (uint8_t)((S.menuRight - 1) - textCol + 1) : 0;

  for (uint8_t i = 0; i < vis; ++i) {
    const uint8_t idx = (uint8_t)(MRW.top + i);
    if (idx >= M.count) break;

    const uint8_t row = (uint8_t)(S.menuTop + 1 + i);
    at(row, textCol);

    const MenuItem& it = M.items[idx];
    const bool sel = (idx == (uint8_t)MRW.selected) && isSelectable(it);

    if (isSeparator(it)) {
      // draw horizontal rule
      fg256(THEME.borderFg);
      const char* H = hrGlyphForStyle();
      at(row, (uint8_t)(S.menuLeft + 1));
      for (uint8_t c = (uint8_t)(S.menuLeft + 1); c <= (uint8_t)(S.menuRight - 1); ++c) TUIserial->print(H);
      rs();
      continue;
    }

    if (sel) invOn();

    String rendered;
    char hk = labelHotkeyAndRender(it.label, rendered);

    String line = String(" ") + rendered + ((it.children && it.childCount) ? "  >" : "");
    if (textMaxC > 0 && line.length() > textMaxC) {
      line = (textMaxC > 1) ? (line.substring(0, textMaxC - 1) + "…") : "";
    }

    if (hk) {
      // case-sensitive highlight of the hotkey character
      int hit = -1;
      for (uint16_t j = 0; j < line.length(); ++j){
        char c = line[j];
        if (j == 0 && c == ' ') continue;
        if (c == hk) { hit = (int)j; break; }
      }
      if (hit < 0) {
        P(line);
      } else {
        for (uint16_t j = 0; j < line.length(); ++j){
          if (j == (uint16_t)hit){
            if (sel){
              invOff();
              fg256(THEME.hotkeyFg); bg256(THEME.hotkeyBg); W(line[j]); rs(); invOn();
            } else {
              fg256(THEME.hotkeyFg); W(line[j]); fgDefault();
            }
          } else {
            W(line[j]);
          }
        }
      }
    } else {
      P(line);
    }

    if (sel) invOff();
  }

  rs();
  TUIserial->flush();
}

inline void pushMenu(const MenuItem* items, uint8_t count, const char* title) {
  if (S.menus.depth+1 >= (int)MENU_STACK_MAX) return;
  ++S.menus.depth;
  auto& M = S.menus.stack[S.menus.depth];
  M.items = items; M.count = count; M.top = 0; M.title = title;

  // Pick first selectable
  int sel = 0;
  for (uint8_t i = 0; i < count; ++i) { if (isSelectable(items[i])) { sel = i; break; } }
  M.selected = (int8_t)sel;

  drawMenuItems();
}

inline void enterSelected() {
  if (S.menus.depth < 0) return;
  const auto& M = curMenu();
  if (M.count == 0) return;

  const auto& it = M.items[M.selected];
  if (!isSelectable(it)) return; // ignore separators

  if (it.children && it.childCount) { 
    pushMenu(it.children, it.childCount, it.childTitle); 
    return; 
  }
  if (it.onEnter) it.onEnter();
}

inline bool hotkeyActivate(char ch){
  if (S.menus.depth < 0) return false;
  MenuView& M = curMenuRW();
  if (!M.count) return false;

  const int sel = M.selected;
  int first = -1, next = -1;

  for (int i = 0; i < M.count; ++i){
    const MenuItem& it = M.items[i];
    if (!isSelectable(it)) continue;
    String rendered; 
    char hk = labelHotkeyAndRender(it.label, rendered);
    if (hk && hk == ch){
      if (first < 0) first = i;
      if (i > sel && next < 0) next = i;
    }
  }
  if (first < 0) return false;

  const int target = (next >= 0) ? next : first;
  if (target != sel) { M.selected = (uint8_t)target; drawMenuItems(); }
  enterSelected();
  return true;
}

// ========= Modal input =========
inline void drawInputModal() {
  uint8_t w = (uint8_t)(S.cols * 0.6f); if (w < 40) w = 40;
  uint8_t h = 7;
  uint8_t r1 = (S.rows - h)/2, c1 = (S.cols - w)/2;
  uint8_t r2 = r1 + h - 1,     c2 = c1 + w - 1;

  boxColored(r1, c1, r2, c2);
  at(r1,   c1+2); P(" Input ");
  at(r1+1, c1+2); P(S.inputTitle);
  at(r1+2, c1+2); P(S.inputPrompt);
  at(r1+4, c1+2); P(String("> ") + S.inputBuffer);
  at(r2,   c1+2); P("Enter=OK   ESC=Cancel    BKSP=Delete");
  TUIserial->flush();
}
inline void startInput(const String& title, const String& prompt, const String& initial,
                       void (*onSubmit)(const String&), void (*onCancel)() ) {
  S.inputActive = true;
  S.inputTitle = title; S.inputPrompt = prompt; S.inputBuffer = initial;
  S.onInputSubmit = onSubmit; S.onInputCancel = onCancel;
  drawInputModal();
}
inline void cancelInput() { S.inputActive = false; drawMenuItems(); drawStatus(); }
inline void submitInput() {
  auto cb = S.onInputSubmit; String val = S.inputBuffer;
  S.inputActive = false;
  drawMenuItems(); drawStatus();
  if (cb) cb(val);
}
inline void redrawInput() { drawInputModal(); }

// ========= Redraw & border =========
inline void setStatus(const String& s) { S.status = s; drawStatus(); }
inline void setTitle(const String& s)  { S.appTitle = s; drawHeader(); }

inline void popMenu() {
  if (S.menus.depth <= 0) return;
  --S.menus.depth;
  drawMenuItems();
}
inline void fullRedraw() {
  clr(); home(); hideCursor();
  drawHeader(); drawMenuFrame(); drawLogFrame(); drawMenuItems(); drawStatus();
  drawLog(); // ensure log pane is painted
  if (S.inputActive) drawInputModal();
  rs();
  TUIserial->flush();
}
inline void setBorderStyle(BorderStyle s) { THEME.borderStyle = s; fullRedraw(); }

// ========= Input handling =========
namespace {
  enum EscState { IDLE, GOT_ESC, GOT_CSI, GOT_O, CSI_PARAMS };
  static EscState esc_state = IDLE;
  static uint32_t esc_ts = 0;
  static char     csi_buf[8];
  static uint8_t  csi_len = 0;
  static int      ungot = -1;
  static int      last_char_code = -1;
}

inline int  lastChar()      { return last_char_code; }
inline void unget(int ch)   { ungot = ch; }
inline bool inEscapeSeq()   { return esc_state != IDLE; }

inline int read_byte_nonblock() {
  if (ungot >= 0) { int v = ungot; ungot = -1; return v; }
  if (!TUIserial->available()) return -1;
  int b = TUIserial->read();
  if (getDebugMode() == 1) log(String("key 0x") + String(b, HEX));
  return b;
}
inline int read_byte_with_wait(uint32_t wait_ms) {
  int v = read_byte_nonblock();
  if (v >= 0) return v;
  if (wait_ms == 0) return -1;
  const uint32_t start = millis();
  while ((millis() - start) < wait_ms) {
    v = read_byte_nonblock();
    if (v >= 0) return v;
    delayMicroseconds(300);
  }
  return -1;
}
inline void reset_esc() { esc_state = IDLE; esc_ts = 0; csi_len = 0; }

inline Key readKey() {
  const uint32_t ESC_TIMEOUT_MS = 200;
  const auto timed_out = [&](uint32_t start)->bool {
    return (esc_state != IDLE) && (millis() - start > ESC_TIMEOUT_MS);
  };

  auto next_byte = [&](bool midEsc)->int {
    if (midEsc) return read_byte_with_wait(10);
    int b = read_byte_nonblock();
    if (b < 0) b = read_byte_with_wait(3);
    return b;
  };

  uint32_t esc_start = esc_ts ? esc_ts : millis();

  for (uint8_t steps = 0; steps < 20; ++steps) {
    if (timed_out(esc_start)) { reset_esc(); return ESC; }

    int ch = next_byte(esc_state != IDLE);
    if (ch < 0) { if (esc_state == IDLE) return NONE; continue; }

    if (esc_state == IDLE) {
      if (ch == '\r' || ch == '\n') return ENTER;
      if (ch == 'w' || ch == 'W' )  return UP;
      if (ch == 's' || ch == 'S' )  return DOWN;
      if (ch == 'q' || ch == 'Q')   return ESC;
      if (ch == 0x1b) { esc_state = GOT_ESC; esc_ts = esc_start = millis(); continue; }
      if (ch >= 32 && ch <= 126) { last_char_code = ch; return CHAR; }
      return NONE;
    }

    if (esc_state == GOT_ESC) {
      esc_ts = esc_start = millis();
      if (ch == '[') { esc_state = GOT_CSI; continue; }
      if (ch == 'O') { esc_state = GOT_O;  continue; }
      reset_esc(); unget(ch); return ESC;
    }

    if (esc_state == GOT_CSI) {
      esc_ts = esc_start = millis();
      if (ch >= '0' && ch <= '9') { csi_len = 0; csi_buf[csi_len++] = (char)ch; esc_state = CSI_PARAMS; continue; }
      if (ch == 'A') { reset_esc(); return UP; }
      if (ch == 'B') { reset_esc(); return DOWN; }
      if (ch == 'C' || ch == 'D') { reset_esc(); return UNKNOWN; }
      reset_esc(); return UNKNOWN;
    }

    if (esc_state == GOT_O) {
      if (ch == 'A') { reset_esc(); return UP; }
      if (ch == 'B') { reset_esc(); return DOWN; }
      reset_esc(); return UNKNOWN;
    }

    if (esc_state == CSI_PARAMS) {
      esc_ts = esc_start = millis();
      if (csi_len < sizeof(csi_buf)-1 && ((ch >= '0' && ch <= '9') || ch == ';')) { csi_buf[csi_len++] = (char)ch; continue; }
      if (ch == 'A') { reset_esc(); return UP; }
      if (ch == 'B') { reset_esc(); return DOWN; }
      reset_esc(); return UNKNOWN;
    }
  }
  return NONE;
}

inline bool handleInput() {
  if (S.inputActive) {
    int ch = read_byte_nonblock();
    if (ch < 0) return false;

    if (ch == '\r' || ch == '\n') { submitInput(); return true; }
    if (ch == 0x1b) { cancelInput(); if (S.onInputCancel) S.onInputCancel(); return true; }
    if (ch == 0x7f || ch == 0x08) {
      if (!S.inputBuffer.isEmpty()) {
        S.inputBuffer.remove(S.inputBuffer.length()-1);
        redrawInput();
      }
      return true;
    }
    if (ch >= 32 && ch <= 126) { S.inputBuffer += (char)ch; redrawInput(); return true; }
    return false;
  }

  Key k = readKey();
  switch (k) {
    case UP:
      if (S.menus.depth >= 0 && curMenu().count) {
        MenuView& M = curMenuRW();
        M.selected = (int8_t)nextSelectableIndex(M, M.selected, -1);
        drawMenuItems();
      }
      return true;

    case DOWN:
      if (S.menus.depth >= 0 && curMenu().count) {
        MenuView& M = curMenuRW();
        M.selected = (int8_t)nextSelectableIndex(M, M.selected, +1);
        drawMenuItems();
      }
      return true;

    case ENTER: enterSelected(); return true;

    case ESC:   popMenu(); setStatus("Back."); return true;

    case CHAR: {
      int c = lastChar();
      if (c >= 0 && hotkeyActivate((char)c)) return true;
      return false;
    }

    default: return false;
  }
}

} // namespace TUI
