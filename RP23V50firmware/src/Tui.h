#pragma once
#include <Arduino.h>

namespace TUI {

// === Debug/timing mode =============================================
// 0 = off, 1 = log
#ifndef TUI_DEBUG_MODE
  #define TUI_DEBUG_MODE 0
#endif

#ifndef TUI_ARRLEN
  #define TUI_ARRLEN(a) (uint8_t)(sizeof(a)/sizeof((a)[0]))
#endif
#ifndef TUI_SUBMENU_ENTRY
  #define TUI_SUBMENU_ENTRY(label, arr, title) \
    { (label), nullptr, (arr), (uint8_t)(sizeof(arr)/sizeof((arr)[0])), (title) }
#endif

// ---------- logging fwd ----------
inline void log(const String& line);

// ---------- runtime debug ----------
inline uint8_t& dbgMode() { static uint8_t m = (uint8_t)TUI_DEBUG_MODE; return m; }
inline void setDebugMode(uint8_t m) { dbgMode() = m; }
inline uint8_t getDebugMode()       { return dbgMode(); }

// ---------- ANSI helpers ----------
inline void W(const char* s) { Serial.print(s); }
inline void W(char c)        { Serial.write(c); }
inline void P(const String& s){ Serial.print(s); }

inline void clr()            { W("\x1b[2J"); }
inline void home()           { W("\x1b[H"); }
inline void hideCursor()     { W("\x1b[?25l"); }
inline void showCursor()     { W("\x1b[?25h"); }
inline void rs()             { W("\x1b[0m"); } // reset color
inline void invOn()          { W("\x1b[7m"); }
inline void invOff()         { W("\x1b[27m"); }
inline void boldOn()         { W("\x1b[1m"); }
inline void boldOff()        { W("\x1b[22m"); }

inline void at(uint8_t row, uint8_t col) {
  char buf[16];
  snprintf(buf, sizeof(buf), "\x1b[%u;%uH", (unsigned)row, (unsigned)col);
  W(buf);
}

inline void hline(uint8_t row, uint8_t col1, uint8_t col2, char ch=' ') {
  at(row, col1);
  for (uint8_t c=col1; c<=col2; ++c) W(ch);
}

inline void box(uint8_t r1, uint8_t c1, uint8_t r2, uint8_t c2) {
  at(r1,c1); W("+");
  for (uint8_t c=c1+1;c<c2;c++) W("-");
  W("+");
  for (uint8_t r=r1+1;r<r2;r++) { at(r,c1); W("|"); at(r,c2); W("|"); }
  at(r2,c1); W("+");
  for (uint8_t c=c1+1;c<c2;c++) W("-");
  W("+");
}

// ---- 256-color helpers ----
inline void fg256(uint8_t c){ char b[20]; snprintf(b,sizeof(b),"\x1b[38;5;%um",c); Serial.print(b); }
inline void bg256(uint8_t c){ char b[20]; snprintf(b,sizeof(b),"\x1b[48;5;%um",c); Serial.print(b); }

// ---- Theme ----
enum BorderStyle : uint8_t { BORDER_ASCII, BORDER_LIGHT, BORDER_ROUNDED, BORDER_HEAVY, BORDER_DOUBLE };

struct Theme {
  uint8_t borderFg   = 45;   // cyan-ish
  bool    unicodeBox = true; // ┌─┐/││/└─┘ vs ASCII
  uint8_t hotkeyFg = 196;  // vivid red
  uint8_t hotkeyBg = 236;  // dark gray block behind glyph
  BorderStyle borderStyle = BORDER_DOUBLE; 
};

inline Theme THEME;


// ---- Layout knobs ----


struct LayoutCfg { float leftFracWide=0.15f; float leftFracNarrow=0.45f; uint8_t margin=2; uint8_t gap=2; };
inline LayoutCfg L;

struct LayoutLimits { uint8_t minMenu=15; uint8_t maxMenu=80; uint8_t minLog=24; };
inline LayoutLimits LIM;

// ---- Colored box (Unicode with ASCII fallback) ----
inline void boxColored(uint8_t r1, uint8_t c1, uint8_t r2, uint8_t c2, uint8_t color = 0xFF) {
  if (color != 0xFF) fg256(color); else fg256(THEME.borderFg);

  const bool ascii = !THEME.unicodeBox || THEME.borderStyle == BORDER_ASCII;

  const char *H, *V, *TL, *TR, *BL, *BR;

  if (ascii) {
    H = "-"; V = "|"; TL = "+"; TR = "+"; BL = "+"; BR = "+";
  } else {
    switch (THEME.borderStyle) {
      case BORDER_ROUNDED:
        H = "─"; V = "│"; TL = "╭"; TR = "╮"; BL = "╰"; BR = "╯";
        break;
      case BORDER_HEAVY:
        H = "━"; V = "┃"; TL = "┏"; TR = "┓"; BL = "┗"; BR = "┛";
        break;
      case BORDER_DOUBLE:
        H = "═"; V = "║"; TL = "╔"; TR = "╗"; BL = "╚"; BR = "╝";
        break;
      case BORDER_LIGHT:
      default:
        H = "─"; V = "│"; TL = "┌"; TR = "┐"; BL = "└"; BR = "┘";
        break;
    }
  }

  at(r1, c1); Serial.print(TL);
  for (uint8_t c = c1 + 1; c < c2; ++c) Serial.print(H);
  Serial.print(TR);

  for (uint8_t r = r1 + 1; r < r2; ++r) {
    at(r, c1); Serial.print(V);
    at(r, c2); Serial.print(V);
  }

  at(r2, c1); Serial.print(BL);
  for (uint8_t c = c1 + 1; c < c2; ++c) Serial.print(H);
  Serial.print(BR);

  rs();
}

// ---------- OLED mirror ----------
using OledClearFn     = void (*)();
using OledPrintLineFn = void (*)(uint8_t row, const char* text);

inline OledClearFn     oledClear     = nullptr;
inline OledPrintLineFn oledPrintLine = nullptr;
inline bool            oledEnabled   = false;
inline uint8_t         oledRows      = 2;
inline uint8_t         oledMaxChars  = 21;

inline bool haveOled() { return oledClear && oledPrintLine; }



inline void setOledCallbacks(OledClearFn clr, OledPrintLineFn printLine,
                             uint8_t rows=2, uint8_t maxChars=21) {
  oledClear = clr; oledPrintLine = printLine; oledRows = rows; oledMaxChars = maxChars;
  oledEnabled = (clr && printLine);
}

inline String trimForOled(const String& s) {
  if (s.length() <= oledMaxChars) return s;
  return s.substring(0, oledMaxChars-1) + "…";
}

// ---------- Menu model ----------
struct MenuItem;
struct MenuView {
  const MenuItem* items = nullptr;
  uint8_t count = 0;
  int8_t  selected = 0;
  uint8_t top = 0;
  const char* title = nullptr;
};
struct MenuItem {
  const char* label;
  void (*onEnter)();
  const MenuItem* children = nullptr;
  uint8_t        childCount = 0;
  const char*    childTitle = nullptr;
};
constexpr uint8_t MENU_STACK_MAX = 6;
struct MenuStack { MenuView stack[MENU_STACK_MAX]; int8_t depth = -1; };

// ---------- Screen layout/state ----------
struct Screen {
  uint8_t cols=80, rows=24;
  String  appTitle   = "Jumperless";
  String  status     = "Ready.";
  uint8_t headerRow  = 1;
  uint8_t statusRow  = 24;
  uint8_t menuTop=3, menuLeft=2, menuBottom=22, menuRight=46;
  uint8_t logTop=3,  logLeft=48, logBottom=22, logRight=79;
  MenuStack menus;
  bool logDirty = true;
  bool    inputActive = false;
  String  inputTitle;
  String  inputPrompt;
  String  inputBuffer;
  void  (*onInputSubmit)(const String&) = nullptr;
  void  (*onInputCancel)() = nullptr;
};
inline Screen S;

// --- Forward declarations (solve Arduino order issues) ----------------
enum Key : uint8_t { NONE, ENTER, UP, DOWN, ESC, CHAR, UNKNOWN };
inline void drawMenuItems();
inline void drawHeader();
inline void drawStatus();
inline void drawLog();
inline void drawInputModal();
inline void setStatus(const String& s);
inline void setTitle(const String& s);
inline void pushMenu(const MenuItem* items, uint8_t count, const char* title);
inline void popMenu();
inline void enterSelected();
inline void fullRedraw();
inline bool inEscapeSeq();
inline Key  readKey();
inline bool handleInput();
// hotkey helpers
inline char  lower_ascii(char c);
inline bool  is_alnum_ascii(char c);
inline char  labelHotkeyAndRender(const char* src, String& rendered);
inline bool  hotkeySelect(char ch);
inline bool  hotkeyActivate(char ch);
inline int   lastChar();

// --- Terminal size probe (CPR) and responsive layout -----------------
inline bool readCPR(uint16_t& row, uint16_t& col, uint32_t timeout_ms=400) {
  char buf[32]; size_t n=0; uint32_t t0=millis();
  while ((millis()-t0) < timeout_ms && n < sizeof(buf)-1) {
    if (!Serial.available()) { delayMicroseconds(300); continue; }
    char ch = (char)Serial.read();
    if (n==0 && ch!='\x1b') continue;
    buf[n++] = ch;
    if (ch=='R') break;
  }
  buf[n]=0; int r=0,c=0;
  return (sscanf(buf, "\x1b[%d;%dR", &r, &c)==2) ? (row=r, col=c, true) : false;
}

inline bool probeTerminalSize(uint16_t& rows, uint16_t& cols) {
  while (Serial.available()) (void)Serial.read();
  Serial.print("\x1b[s");
  Serial.print("\x1b[9999;9999H");
  Serial.print("\x1b[6n");
  Serial.flush();
  bool ok = readCPR(rows, cols, 600);
  Serial.print("\x1b[u");
  Serial.flush();
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

// ---------- Log ring ----------
constexpr uint16_t LOG_CAP = 128;
inline String   logBuf[LOG_CAP];
inline uint16_t logHead = 0;
inline uint16_t logCount = 0;

inline void log(const String& line) {
  logBuf[logHead] = line;
  logHead = (logHead + 1) % LOG_CAP;
  if (logCount < LOG_CAP) logCount++;
  S.logDirty = true;
}

// ---------- Drawing ----------
inline void drawHeader() {
  at(S.headerRow,1); 
  invOn(); 
  hline(S.headerRow,1,S.cols,' ');

  if (THEME.unicodeBox && THEME.borderStyle == BORDER_ROUNDED) {
    at(S.headerRow, 1);     W("╭");
    at(S.headerRow, S.cols); W("╮");
  }

  
  at(S.headerRow,3); 
  boldOn(); P(S.appTitle); 
  boldOff(); 
  invOff(); rs();
  
  Serial.flush();
}

inline void drawStatus() {
  at(S.statusRow,1);
  invOn();
  hline(S.statusRow,1,S.cols,' ');

  if (THEME.unicodeBox && THEME.borderStyle == BORDER_ROUNDED) {
    at(S.statusRow, 1);      W("╰");
    at(S.statusRow, S.cols); W("╯");
  }

  at(S.statusRow,3); P(S.status); invOff();
  rs();

  // --- SAFE OLED MIRROR ---
  if (oledEnabled && oledClear && oledPrintLine) {
    oledClear();
    if (oledRows >= 1) 
      oledPrintLine(0, trimForOled(S.appTitle).c_str());
    if (oledRows >= 2) 
      oledPrintLine(1, trimForOled(S.status).c_str());
  }

  Serial.flush();
}

inline void setOledEnabled(bool en) {
  if (en && !haveOled()) {
    oledEnabled = false;
    log("OLED enable ignored: callbacks not set.");
    return;
  }
  oledEnabled = en;
  drawStatus();  // reflect state on both terminals immediately
}


inline void drawMenuFrame() {
  boxColored(S.menuTop, S.menuLeft, S.menuBottom, S.menuRight);
  at(S.menuTop, S.menuLeft+2); fg256(THEME.borderFg); P(" Menu "); rs();
  Serial.flush();
}

inline void drawLogFrame() {
  boxColored(S.logTop, S.logLeft, S.logBottom, S.logRight);
  at(S.logTop, S.logLeft+2); fg256(THEME.borderFg); P(" Log "); rs();
  Serial.flush();
}

inline const MenuView& curMenu() { return S.menus.stack[S.menus.depth]; }
inline MenuView&       curMenuRW(){ return S.menus.stack[S.menus.depth]; }

// --- Hotkey helpers ---------------------------------------------------
inline char lower_ascii(char c){ return (c>='A' && c<='Z') ? (char)(c+32) : c; }
inline bool is_alnum_ascii(char c){ return (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z'); }

// Extract hotkey from label and produce a rendered string without '&'.
inline char labelHotkeyAndRender(const char* src, String& rendered){
  char hk = 0; rendered = ""; if (!src) return 0; bool seenAmp = false;
  for (const char* p = src; *p; ++p){
    char ch = *p;
    if (!seenAmp && ch == '&'){ seenAmp = true; continue; }     // drop '&'
    if (seenAmp){ if (!hk && ch > ' ') hk = lower_ascii(ch); rendered += ch; seenAmp = false; continue; }
    rendered += ch;
  }
  if (!hk){
    for (uint16_t i=0;i<rendered.length();++i){ char ch = rendered[i]; if (is_alnum_ascii(ch)){ hk = lower_ascii(ch); break; } }
  }
  return hk;
}

// Reset just the foreground color (keeps inverse/bold state intact)
// Reset just FG / BG (don't nuke other attrs)
inline void fgDefault(){ W("\x1b[39m"); }
inline void bgDefault(){ W("\x1b[49m"); }

// Strict extractor: only honors explicit &X (no fallback)
// Returns hk (lowercased) or 0. 'rendered' = label without '&'.
// Only honors explicit &X; 'rendered' gets label without '&'
inline char labelHotkeyAndRenderStrict(const char* src, String& rendered){
  char hk = 0; rendered = ""; if (!src) return 0; bool seenAmp = false;
  for (const char* p = src; *p; ++p){
    char ch = *p;
    if (!seenAmp && ch == '&'){ seenAmp = true; continue; }
    if (seenAmp){ if (!hk && ch > ' ') hk = lower_ascii(ch); rendered += ch; seenAmp = false; continue; }
    rendered += ch;
  }
  return hk;
}

// Color the first occurrence of hk using THEME.hotkeyFg/Bg.
// If 'selected', momentarily cancel inverse for that glyph so it pops.
inline void printLineWithHotColorStrict(const String& line, char hk, bool selected){
  if (!hk) { P(line); return; }
  char want = lower_ascii(hk);
  int hit = -1;

  for (uint16_t i = 0; i < line.length(); ++i){
    char c = line[i];
    if (i == 0 && c == ' ') continue; // skip left padding
    if (lower_ascii(c) == want) { hit = (int)i; break; }
  }
  if (hit < 0) { P(line); return; }

  for (uint16_t i = 0; i < line.length(); ++i){
    if (i == (uint16_t)hit){
      if (selected){
        invOff();                // step out of inverse for one glyph
        fg256(THEME.hotkeyFg);
        bg256(THEME.hotkeyBg);
        W(line[i]);
        rs();                    // reset all attrs
        invOn();                 // restore selection bar
      } else {
        fg256(THEME.hotkeyFg);   // just FG on normal rows
        W(line[i]);
        fgDefault();
      }
    } else {
      W(line[i]);
    }
  }
}


inline void drawMenuItems() {
  if (S.menus.depth < 0) return;
  MenuView& MRW = curMenuRW();
  const MenuView& M = MRW;

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
    const uint8_t titleCol = (uint8_t)(S.menuLeft + 10);
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

    if (idx == (uint8_t)MRW.selected) invOn();

    const MenuItem& it = M.items[idx];

    // STRICT: only highlight if label contains '&'
    String rendered;
    char hk = labelHotkeyAndRenderStrict(it.label, rendered);

    String line = String(" ") + rendered + ((it.children && it.childCount) ? "  >" : "");
    if (textMaxC > 0 && line.length() > textMaxC) {
      line = (textMaxC > 1) ? (line.substring(0, textMaxC - 1) + "…") : "";
    }

    if (hk) {
      printLineWithHotColorStrict(line, hk, /*selected*/ idx == (uint8_t)MRW.selected);
    } else {
      P(line);
    }



    if (idx == (uint8_t)MRW.selected) invOff();
  }

  rs();
  Serial.flush();
}

// ---------- Menu ops ----------
inline void pushMenu(const MenuItem* items, uint8_t count, const char* title=nullptr) {
  if (S.menus.depth+1 >= (int)MENU_STACK_MAX) return;
  ++S.menus.depth;
  auto& M = S.menus.stack[S.menus.depth];
  M.items = items; M.count = count; M.selected = 0; M.top = 0; M.title = title;
  drawMenuItems();
}

inline void enterSelected() {
  if (S.menus.depth < 0) return;
  const auto& M = curMenu();
  if (M.count == 0) return;
  const auto& it = M.items[M.selected];
  if (it.children && it.childCount) { pushMenu(it.children, it.childCount, it.childTitle); return; }
  if (it.onEnter) it.onEnter();
}

// ---------- Hotkey select/activate ----------
inline bool hotkeyActivate(char ch){
  if (S.menus.depth < 0) return false;
  ch = lower_ascii(ch);
  MenuView& M = curMenuRW();
  if (!M.count) return false;

  const int sel = M.selected;
  int first = -1, next = -1;

  for (int i=0;i<M.count;++i){
    const MenuItem& it = M.items[i];
    String rendered; char hk = labelHotkeyAndRender(it.label, rendered);
    if (hk == ch){ if (first < 0) first = i; if (i > sel && next < 0) next = i; }
  }
  if (first < 0) return false;
  const int target = (next >= 0) ? next : first;
  if (target != sel) M.selected = (uint8_t)target;
  enterSelected();
  return true;
}

inline bool hotkeySelect(char ch){
  if (S.menus.depth < 0) return false;
  ch = lower_ascii(ch);
  MenuView& M = curMenuRW();
  if (!M.count) return false;

  int sel = M.selected;
  int first = -1, next = -1;

  for (int i=0;i<M.count;++i){
    const MenuItem& it = M.items[i];
    String rendered; char hk = labelHotkeyAndRender(it.label, rendered);
    if (hk == ch){ if (first < 0) first = i; if (i > sel && next < 0) next = i; }
  }
  if (first < 0) return false;

  int target = (next >= 0) ? next : first;
  if (target != sel){ M.selected = (uint8_t)target; drawMenuItems(); }
  return true;
}

inline void drawLog() {
  for (uint8_t r=S.logTop+1; r<=S.logBottom-1; ++r) {
    at(r, S.logLeft+1);
    for (uint8_t c=S.logLeft+1; c<=S.logRight-1; ++c) W(' ');
  }
  uint8_t vis = (S.logBottom-1) - (S.logTop+1) + 1;
  int32_t start = (int32_t)logCount - vis;
  if (start < 0) start = 0;

  for (uint8_t i=0; i<vis; ++i) {
    int32_t idx = start + i;
    if (idx >= (int32_t)logCount) break;
    uint16_t ringIdx = (logHead + LOG_CAP - (logCount - idx)) % LOG_CAP;
    at(S.logTop+1+i, S.logLeft+2);
    const String& s = logBuf[ringIdx];
    uint8_t maxw = (S.logRight-2) - (S.logLeft+2) + 1;
    P((s.length() <= maxw) ? s : (s.substring(0, maxw-1) + "…"));
  }
  S.logDirty = false;
  Serial.flush();
}

// ---------- Modal input ----------
inline void drawInputModal() {
  uint8_t w = (uint8_t)(S.cols * 0.6f); if (w < 40) w = 40;
  uint8_t h = 7;
  uint8_t r1 = (S.rows - h)/2, c1 = (S.cols - w)/2;
  uint8_t r2 = r1 + h - 1,     c2 = c1 + w - 1;

  boxColored(r1, c1, r2, c2);  // <— was: box(r1,c1,r2,c2)

  at(r1,   c1+2); P(" Input ");
  at(r1+1, c1+2); P(S.inputTitle);
  at(r1+2, c1+2); P(S.inputPrompt);
  at(r1+4, c1+2); P(String("> ") + S.inputBuffer);
  at(r2,   c1+2); P("Enter=OK   ESC=Cancel    BKSP=Delete");
  Serial.flush();
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

// ---------- Menu ops & full redraw ----------
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
  if (S.inputActive) drawInputModal();
  rs();
  Serial.flush();
}


inline void setBorderStyle(BorderStyle s) { 
  THEME.borderStyle = s; 
  fullRedraw(); 
}

// ---------- Input handling ----------



namespace {
  enum EscState { IDLE, GOT_ESC, GOT_CSI, GOT_O, CSI_PARAMS };
  static EscState esc_state = IDLE;
  static uint32_t esc_ts = 0;
  static char     csi_buf[8];
  static uint8_t  csi_len = 0;
  static int      ungot = -1;
  static int      last_char_code = -1;
}

inline int lastChar() { return last_char_code; }
inline void unget(int ch) { ungot = ch; }
inline bool inEscapeSeq() { return esc_state != IDLE; }

inline int read_byte_nonblock() {
  if (ungot >= 0) { int v = ungot; ungot = -1; return v; }
  if (!Serial.available()) return -1;
  int b = Serial.read();
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

// Aggressive, one-shot sequence parser (avoids timing dependence)
inline Key readKey() {

  const uint32_t ESC_TIMEOUT_MS = 200;
  const auto timed_out = [&](uint32_t start)->bool {
    return (esc_state != IDLE) && (millis() - start > ESC_TIMEOUT_MS);
  };

  auto next_byte = [&](bool midEsc)->int {
    if (midEsc) 
      return read_byte_with_wait(10);
    
    int b = read_byte_nonblock();
    if (b < 0) 
      b = read_byte_with_wait(3);

    return b;
  };

  uint32_t esc_start = esc_ts ? esc_ts : millis();

  for (uint8_t steps = 0; steps < 20; ++steps) {
    if (timed_out(esc_start)) { reset_esc(); return ESC; }

    int ch = next_byte(esc_state != IDLE);
    if (ch < 0) { if (esc_state == IDLE) return NONE; continue; }

    if (esc_state == IDLE) {
      if (ch == '\r' || ch == '\n') 
        return ENTER;
      
      if (ch == 'w' || ch == 'W' ) 
        return UP;
      
      if (ch == 's' || ch == 'S' ) 
        return DOWN;
      
      if (ch == 'q' || ch == 'Q') 
        return ESC;
      
      if (ch == 0x1b) { 
        esc_state = GOT_ESC; 
        esc_ts = esc_start = millis(); 
        continue; 
      }

      if (ch >= 32 && ch <= 126) { 
        last_char_code = ch; 
        return CHAR; 
      }

      return NONE;
    }

    if (esc_state == GOT_ESC) {
      esc_ts = esc_start = millis();
      
      if (ch == '[') { 
        esc_state = GOT_CSI; continue; 
      }
      
      if (ch == 'O') { 
        esc_state = GOT_O; 
        continue; 
      }
      reset_esc(); 
      unget(ch); 
      return ESC;
    }

    if (esc_state == GOT_CSI) {
      esc_ts = esc_start = millis();
      if (ch >= '0' && ch <= '9') { 
          csi_len = 0; 
          csi_buf[csi_len++] = (char)ch; 
          esc_state = CSI_PARAMS; 
          continue; 
      }
      
      if (ch == 'A') { 
        reset_esc(); 
        return UP; 
      }
      
      if (ch == 'B') { 
        reset_esc(); 
        return DOWN; 
      }
      
      if (ch == 'C' || ch == 'D') { 
        reset_esc(); 
        return UNKNOWN; 
      }
      
      reset_esc(); 
      return UNKNOWN;
    }

    if (esc_state == GOT_O) {
      if (ch == 'A') { 
        reset_esc(); 
        return UP; 
      }

      if (ch == 'B') 
      { 
        reset_esc(); 
        return DOWN; 
      }
      
      reset_esc(); 
        return UNKNOWN;
    }

    if (esc_state == CSI_PARAMS) {
      
      esc_ts = esc_start = millis();
      if (csi_len < sizeof(csi_buf)-1 && ((ch >= '0' && ch <= '9') || ch == ';')) { 
        csi_buf[csi_len++] = (char)ch; continue; 
      }

      if (ch == 'A') { 
        reset_esc(); 
        return UP; 
      }
      
      if (ch == 'B') { 
        reset_esc(); return DOWN; 
      }
      reset_esc(); return UNKNOWN;
    }
  }
  return NONE;
}

inline bool handleInput() {
  if (S.inputActive) {
    int ch = read_byte_nonblock();
    if (ch < 0) 
      return false;
    
    if (ch == '\r' || ch == '\n') { 
      submitInput(); return true; 
    }
    
    if (ch == 0x1b) { 
      
      cancelInput(); 
      if (S.onInputCancel) 
        S.onInputCancel(); 
      
      return true; 
    }
      
    if (ch == 0x7f || ch == 0x08) { 
      if (!S.inputBuffer.isEmpty()) { 
        S.inputBuffer.remove(S.inputBuffer.length()-1); 
        redrawInput(); 
      } 
      return true; 
    }
    
    if (ch >= 32 && ch <= 126) { 
      S.inputBuffer += (char)ch; redrawInput(); 
      return true; 
    }

    return false;
  }

  Key k = readKey();
  switch (k) {
    
    case UP:
      if (S.menus.depth >= 0 && curMenu().selected > 0) { 
        curMenuRW().selected--; drawMenuItems(); 
      }
      
      return true;
    
      case DOWN:
      if (S.menus.depth >= 0 && curMenu().selected < (curMenu().count-1)) { 
        curMenuRW().selected++; drawMenuItems(); 
      }
      return true;

    case ENTER: enterSelected(); 
      return true;
    
    case ESC:   popMenu(); setStatus("Back."); 
      return true;
    
    case CHAR: {
      int c = lastChar();
      if (c >= 0 && hotkeyActivate((char)c)) 
        return true;

      return false;
    }
    default: return false;
  }
}

} // namespace TUI
