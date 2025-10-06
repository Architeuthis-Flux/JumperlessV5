// SPDX-License-Identifier: MIT
// TuiPopup.cpp — implementation

#include "TuiPopup.h"
#include "Tui.h"        // uses TUI::S (rows/cols), TUI::THEME.borderStyle, TUI::fullRedraw()
#include <Arduino.h>

namespace TUI {

// --- Input-mode ESC parser state ---
// 0=idle, 1=ESC, 2=ESC[, 3=ESCO, 4=ESC[3 (expect '~')
static uint8_t  s_inputEscState = 0;

Popup& Popup::instance() {
  static Popup P;
  return P;
}

void Popup::setSerial(Stream* ser) { m_ser = ser; }
void popupSetSerial(Stream* s) { Popup::instance().setSerial(s); }

bool Popup::isActive() const { return m_active; }

void Popup::open(const String& title, int16_t boxRows, int16_t boxCols, float fracRows, float fracCols) {
  m_active = true;
  m_title = title;
  m_lineCount = 0;
  m_scroll    = 0;
  m_autoCloseAt = 0;

  m_userBoxH = boxRows;
  m_userBoxW = boxCols;
  m_userFracH = fracRows;
  m_userFracW = fracCols;

  // reset input-related state
  m_inputActive = false;
  m_prompt = "";
  m_input  = "";
  m_cursor = 0;
  m_viewLeft = 0;
  m_submitCb = nullptr;
  m_cancelCb = nullptr;

  s_inputEscState = 0;

  if (m_ser) {
    m_ser->print("\x1b[?25l"); // hide caret in non-input popups
  }

  m_dirty = true;
}

void Popup::close() {
  m_active = false;
  m_autoCloseAt = 0;
  m_dirty = false;

  // clear input callbacks and state
  m_inputActive = false;
  m_submitCb = nullptr;
  m_cancelCb = nullptr;

  s_inputEscState = 0;

  if (m_ser) {
    m_ser->print("\x1b[?25l"); // hide caret
  }

  TUI::fullRedraw();
}

void Popup::clearInterior(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
  if (!m_ser) return;
  if (h < 3 || w < 3) return;
  for (uint16_t r = 1; r < h - 1; ++r) {
    mv(m_ser, (uint16_t)(r0 + r), (uint16_t)(c0 + 1));
    hline(m_ser, (uint16_t)(w - 2), ' ');
  }
}

void Popup::clear() {
  m_lineCount = 0;
  m_scroll = 0;
  m_dirty = true;
}

bool Popup::endsWithNewline(const String& s) {
  int n = (int)s.length();
  return n > 0 && (s[n-1] == '\n' || s[n-1] == '\r');
}

void Popup::print(const String& s) {
  uint16_t start = 0;
  while (start < s.length()) {
    int nl = s.indexOf('\n', start);
    String line = (nl >= 0) ? s.substring(start, nl) : s.substring(start);
    if (m_lineCount < MAX_LINES) {
      m_lines[m_lineCount++] = line;
    } else {
      for (uint16_t i=1;i<MAX_LINES;i++) m_lines[i-1] = m_lines[i];
      m_lines[MAX_LINES-1] = line;
    }
    if (nl < 0) break;
    start = (uint16_t)(nl + 1);
  }
  m_dirty = true;
}

void Popup::println(const String& s) {
  if (endsWithNewline(s)) print(s);
  else                    print(s + "\n");
}

void Popup::autoCloseMs(uint32_t ms) {
  m_autoCloseAt = ms ? (millis() + ms) : 0;
  m_dirty = true;
}

void Popup::drawIfDirty() {
  if (m_active && m_dirty) {
    draw();
    m_dirty = false;
  }
}

// ---------- drawing helpers ----------
struct BorderChars { const char* tl; const char* tr; const char* bl; const char* br; const char* h; const char* v; };

static BorderChars currentBorderChars() {
  switch (TUI::THEME.borderStyle) {
    case TUI::BORDER_LIGHT:   return {"┌","┐","└","┘","─","│"};
    case TUI::BORDER_ROUNDED: return {"╭","╮","╰","╯","─","│"};
    case TUI::BORDER_HEAVY:   return {"┏","┓","┗","┛","━","┃"};
    case TUI::BORDER_DOUBLE:  return {"╔","╗","╚","╝","═","║"};
    case TUI::BORDER_ASCII:
    default:                  return {"+","+","+","+","-","|"};
  }
}

void Popup::mv(Stream* ser, uint16_t r, uint16_t c) {
  if (!ser) return;
  ser->print("\x1b["); ser->print(r); ser->print(";"); ser->print(c); ser->print("H");
}
void Popup::hline(Stream* ser, uint16_t len, char ch) {
  if (!ser) return;
  for (uint16_t i=0;i<len;i++) ser->write(ch);
}
void Popup::hlineGlyph(Stream* ser, uint16_t len, const char* g) {
  if (!ser) return;
  for (uint16_t i=0;i<len;i++) ser->print(g);
}

void Popup::computeBox(uint16_t& r0, uint16_t& c0, uint16_t& h, uint16_t& w) {
  uint16_t rows = TUI::S.rows ? TUI::S.rows : 24;
  uint16_t cols = TUI::S.cols ? TUI::S.cols : 80;

  const uint16_t minW = 12, minH = 6;
  const uint16_t maxW = (cols > 2) ? (uint16_t)(cols - 2) : cols;
  const uint16_t maxH = (rows > 2) ? (uint16_t)(rows - 2) : rows;

  auto autoWidth = [&](){
    uint16_t maxLen = 0;
    for (uint16_t i=0;i<m_lineCount;i++) {
      uint16_t L = (uint16_t)m_lines[i].length();
      if (L > maxLen) maxLen = L;
    }
    const uint16_t maxBoxW = (uint16_t)((cols * 4) / 5); // 80%
    uint16_t ww = (uint16_t)min<uint16_t>(maxBoxW, max<uint16_t>(30, (uint16_t)(maxLen + 2*PAD + 2)));
    return (uint16_t)constrain(ww, minW, maxW);
  };
  auto autoHeight = [&](){
    uint16_t hh = (uint16_t)min<uint16_t>((rows * 3) / 5, (uint16_t)(rows - 2));
    return (uint16_t)constrain(hh, minH, maxH);
  };

  if (m_userBoxW >= 0)                        w = (uint16_t)constrain((uint16_t)m_userBoxW, minW, maxW);
  else if (m_userFracW > 0.0f && m_userFracW <= 1.0f) w = (uint16_t)constrain((uint16_t)(cols * m_userFracW), minW, maxW);
  else                                               w = autoWidth();

  if (m_userBoxH >= 0)                        h = (uint16_t)constrain((uint16_t)m_userBoxH, minH, maxH);
  else if (m_userFracH > 0.0f && m_userFracH <= 1.0f) h = (uint16_t)constrain((uint16_t)(rows * m_userFracH), minH, maxH);
  else                                               h = autoHeight();

  r0 = (uint16_t)((rows > h) ? ((rows - h) / 2 + 1) : 1);
  c0 = (uint16_t)((cols > w) ? ((cols - w) / 2 + 1) : 1);

  m_boxR = r0; m_boxC = c0; m_boxH = h; m_boxW = w;
}

void Popup::drawBorder(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
  if (!m_ser) return;
  BorderChars bc = currentBorderChars();

  mv(m_ser, r0, c0);
  m_ser->print(bc.tl); hlineGlyph(m_ser, (uint16_t)(w-2), bc.h); m_ser->print(bc.tr);
  for (uint16_t r=1; r<h-1; r++) {
    mv(m_ser, r0 + r, c0);     m_ser->print(bc.v);
    mv(m_ser, r0 + r, c0+w-1); m_ser->print(bc.v);
  }
  mv(m_ser, r0 + h - 1, c0);
  m_ser->print(bc.bl); hlineGlyph(m_ser, (uint16_t)(w-2), bc.h); m_ser->print(bc.br);
}

void Popup::drawTitle(uint16_t r0, uint16_t c0, uint16_t w) {
  if (!m_ser) return;
  String t = " " + m_title + " ";
  uint16_t tstart = (uint16_t)(c0 + (w - t.length())/2);
  mv(m_ser, r0, tstart); m_ser->print(t);
}

// Help text goes on the last interior row; only for input mode
void Popup::drawHelp(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
  if (!m_ser) return;
  const uint16_t helpRow = (uint16_t)(r0 + h - 2);
  if (helpRow <= r0 + 1) return;

  const uint16_t innerW = (uint16_t)(w > 2 ? w - 2 : 0);
  if (!innerW) return;

  // clear the line
  mv(m_ser, helpRow, (uint16_t)(c0 + 1));
  hline(m_ser, innerW, ' ');

  if (!m_inputActive) return;

  const char* help = " Enter submit — ESC cancel — Del/Backspace edit — ↑/↓ history — ←/→ move ";
  const uint16_t len = (uint16_t)strlen(help);
  const uint16_t startCol = (uint16_t)(c0 + 1 + ((innerW > len) ? (innerW - len)/2 : 0));
  mv(m_ser, helpRow, startCol);
  m_ser->print(help);
}

uint16_t Popup::countWrappedRows(const String& S, uint16_t innerW) {
  if (innerW == 0) return 0;
  if (S.length() == 0) return 1;
  uint16_t rows = 0;
  uint16_t p = 0, n = (uint16_t)S.length();

  while (p < n) {
    uint16_t q = p, used = 0; bool first = true;
    while (q < n && S[q] == ' ') q++;
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

void Popup::normalizeScroll(uint16_t innerH, uint16_t innerW) {
  if (innerW == 0) { m_scroll = 0; return; }
  uint16_t total = 0;
  for (uint16_t i=0;i<m_lineCount;i++)
    total = (uint16_t)(total + countWrappedRows(m_lines[i], innerW));
  
  if (total <= innerH) { 
    m_scroll = 0; 
    return; 
  }

  if (m_scroll + innerH > total) 
    m_scroll = (uint16_t)(total - innerH);
}

void Popup::drawWrappedContent(uint16_t rStart, uint16_t cStart, uint16_t innerH, uint16_t innerW) {
  if (!m_ser) return;

  for (uint16_t i=0;i<innerH;i++) {
    mv(m_ser, rStart + i, cStart); hline(m_ser, innerW, ' ');
  }

  uint16_t skip = m_scroll, printed = 0;

  for (uint16_t li=0; li<m_lineCount && printed < innerH; li++) {
    const String& S = m_lines[li];
    uint16_t n = (uint16_t)S.length();
    if (innerW == 0) break;

    if (n == 0) {
      if (skip > 0) { skip--; }
      else { mv(m_ser, rStart + printed, cStart); printed++; }
      continue;
    }

    uint16_t p = 0;
    while (p < n && printed < innerH) {
      String out;
      out.reserve(innerW);
      uint16_t q = p;
      while (q < n && S[q] == ' ') q++;
      if (q >= n) {
        if (skip > 0) skip--;
        else { mv(m_ser, rStart + printed, cStart); printed++; }
        p = q; break;
      }

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
      else {
        mv(m_ser, rStart + printed, cStart);
        if (out.length() > 0) m_ser->print(out);
        printed++;
      }

      if (p < q) p = q;
    }
  }
}

void Popup::draw() {
  if (!m_active || !m_ser) return;

  uint16_t r0, c0, h, w;
  computeBox(r0, c0, h, w);

  const uint16_t innerW = (uint16_t)(w - 2 - 2*PAD);
  const uint16_t innerH = (uint16_t)(h >= 4 ? (h - 4) : 0);

  normalizeScroll(innerH, innerW);

  drawBorder(r0, c0, h, w);
  clearInterior(r0, c0, h, w);
  drawTitle(r0, c0, w);
  drawHelp(r0, c0, h, w);

  const uint16_t rStart = (uint16_t)(r0 + 2);
  const uint16_t cStart = (uint16_t)(c0 + 1 + PAD);

  if (m_inputActive) {
    uint16_t contentH = innerH;
    if (contentH > 1) {
      drawWrappedContent(rStart, cStart, (uint16_t)(contentH - 1), innerW);
    }
    drawInputLine(rStart, cStart, innerW, (uint16_t)max<uint16_t>(contentH, 1));
    m_ser->print("\x1b[?25h"); // ensure cursor visible
  } else {
    drawWrappedContent(rStart, cStart, innerH, innerW);
    m_ser->print("\x1b[?25l"); // hide cursor
  }

  // --- Single-ESC cancel: if we saw ESC and no '['/'O' advanced the state
  // during this input batch, treat as bare ESC right now (no timeout needed).
  if (m_inputActive && s_inputEscState == 1) {
    CancelFn cancel = m_cancelCb;
    m_submitCb = nullptr; m_cancelCb = nullptr; m_inputActive = false;
    s_inputEscState = 0;
    close(); if (cancel) cancel();
    return; // popup closed
  }
}

// Reads additional bytes when first byte was ESC (non-input mode only)
bool Popup::handleArrowSeq() {
  if (!m_ser) return true;
  char b[4] = {0,0,0,0};
  int n = 0;

  while (m_ser->available() && n < 3) b[n++] = (char)m_ser->read();
  uint32_t t0 = millis();
  while (n < 3 && (millis() - t0) < 40) {
    if (m_ser->available()) b[n++] = (char)m_ser->read();
  }

  auto lineUp = [&](){ if (m_scroll>0) { m_scroll--; m_dirty = true; } };
  auto lineDown = [&](){
    uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
    uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
    m_scroll++; normalizeScroll(innerH, innerW); m_dirty = true;
  };
  auto page = [&](int dir){
    uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
    uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
    if (dir < 0) m_scroll = (m_scroll > innerH) ? (uint16_t)(m_scroll - innerH) : 0;
    else         { m_scroll = (uint16_t)(m_scroll + innerH); normalizeScroll(innerH, innerW); }
    m_dirty = true;
  };

  if (n >= 1 && b[0] == 'A') { lineUp();   return true; }   // Up
  if (n >= 1 && b[0] == 'B') { lineDown(); return true; }   // Down
  if (n >= 2 && b[0] == '5' && b[1] == '~') { page(-1); return true; } // PgUp
  if (n >= 2 && b[0] == '6' && b[1] == '~') { page(+1); return true; } // PgDn
  return true; // swallow anything else
}

// --- Non-input mode key handler ---
bool Popup::handleKey(int ch) {
  if (!m_active) return false;

  if (m_inputActive) return handleKeyInputMode(ch);

  if (m_autoCloseAt && (int32_t)(millis() - m_autoCloseAt) >= 0) { close(); return true; }

  if (ch == 0x1B) {
    int next = -1;
    uint32_t t0 = millis();
    while ((!m_ser || !m_ser->available()) && (millis() - t0) < 60) { }
    if (m_ser && m_ser->available()) next = m_ser->read();

    if (next == '[') return handleArrowSeq();

    if (next == 'O') {
      int b = -1;
      uint32_t t1 = millis();
      while ((!m_ser || !m_ser->available()) && (millis() - t1) < 60) { }
      if (m_ser && m_ser->available()) b = m_ser->read();
      if (b == 'A') { if (m_scroll>0) { m_scroll--; m_dirty = true; } return true; } // Up
      if (b == 'B') { uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
                      uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
                      m_scroll++; normalizeScroll(innerH, innerW); m_dirty = true; return true; } // Down
      return true;
    }

    if (next < 0) { close(); return true; } // bare ESC
    return true; // swallow anything else
  }

  return true; // modal: swallow everything else
}

void Popup::openInput(const String& title, const String& prompt) {
  open(title, -1, -1, 0.40f, 0.70f); 
  m_prompt       = prompt;
  m_input        = "";
  m_cursor       = 0;
  m_viewLeft     = 0;
  m_inputActive  = true;
  m_autoCloseAt  = 0;
  m_dirty        = true;

  s_inputEscState = 0;

  if (m_ser) {
    m_ser->print("\x1b[?25h");  // show cursor
    m_ser->print("\x1b[?12h");  // enable cursor blink
    m_ser->print("\x1b[5 q");   // fallback cursor style
  }
}

void Popup::setInitialInput(const String& s) {
  m_input  = s;
  m_cursor = (uint16_t)m_input.length();
  m_viewLeft = 0;          // let draw() place it as needed
  m_dirty  = true;
}
void Popup::setOnSubmit(SubmitFn fn) { m_submitCb = fn; }
void Popup::setOnCancel(CancelFn fn) { m_cancelCb = fn; }

void Popup::drawInputLine(uint16_t rStart, uint16_t cStart, uint16_t innerW, uint16_t innerH) {
  if (!m_ser) return;

  // Put input on the last interior row (or only interior row)
  const uint16_t contentRows = (uint16_t)max<uint16_t>(innerH, 1);
  const uint16_t r = (uint16_t)(rStart + contentRows - 1);

  // Label (prompt)
  const String label = m_prompt.length() ? (m_prompt + " ") : String("");
  const uint16_t labLen = (uint16_t)min<uint16_t>(label.length(), innerW);

  // Clear line and print label
  mv(m_ser, r, cStart);
  hline(m_ser, innerW, ' ');
  if (labLen) {
    mv(m_ser, r, cStart);
    m_ser->print(label.substring(0, labLen));
  }

  // Text area width after the label
  const uint16_t textW = (uint16_t)((innerW > labLen) ? (innerW - labLen) : 0);

  // Adjust horizontal view to keep caret visible
  if (m_cursor < m_viewLeft) m_viewLeft = m_cursor;
  if (textW && m_cursor > (uint16_t)(m_viewLeft + textW)) {
    m_viewLeft = (uint16_t)(m_cursor - textW);
  }

  // Emit visible slice
  if (textW) {
    String vis;
    if (m_input.length() <= textW) {
      m_viewLeft = 0;
      vis = m_input;
    } else {
      uint16_t right = (uint16_t)min<uint16_t>((uint16_t)(m_viewLeft + textW), (uint16_t)m_input.length());
      vis = m_input.substring(m_viewLeft, right);
    }
    mv(m_ser, r, (uint16_t)(cStart + labLen));
    if (!vis.isEmpty()) m_ser->print(vis);
  }

  // Compute absolute caret column on screen
  uint16_t visibleCursor = 0;
  if (m_cursor >= m_viewLeft) visibleCursor = (uint16_t)(m_cursor - m_viewLeft);
  if (visibleCursor > textW)  visibleCursor = textW;

  m_inputRow       = r;
  m_inputCaretCol  = (uint16_t)(cStart + labLen + visibleCursor);

  // Clamp to interior
  const uint16_t leftClamp  = cStart;
  const uint16_t rightClamp = (uint16_t)(cStart + innerW - 1);
  if (m_inputCaretCol < leftClamp)  m_inputCaretCol = leftClamp;
  if (m_inputCaretCol > rightClamp) m_inputCaretCol = rightClamp;

  // Place the caret
  mv(m_ser, m_inputRow, m_inputCaretCol);
  m_ser->print("\x1b[?25h");
}

// --- Input-mode key handler (with ←/→, Home/End, Delete, and single-ESC cancel) ---
bool Popup::handleKeyInputMode(int ch) {
  // Continue ESC-sequence parsing if we're already in it
  if (s_inputEscState) {
    if (s_inputEscState == 1) {          // ESC received previously
      if (ch == '[') { s_inputEscState = 2; return true; } // CSI
      if (ch == 'O') { s_inputEscState = 3; return true; } // SS3
      // Something else after ESC => treat prior ESC as cancel
      CancelFn cancel = m_cancelCb;
      m_submitCb = nullptr; m_cancelCb = nullptr; m_inputActive = false;
      s_inputEscState = 0;
      close(); if (cancel) cancel();
      return true;
    }
    if (s_inputEscState == 2) {          // ESC [
      if (ch == 'C') { if (m_cursor < m_input.length()) { m_cursor++; m_dirty = true; } s_inputEscState=0; return true; } // →
      if (ch == 'D') { if (m_cursor > 0)                 { m_cursor--; m_dirty = true; } s_inputEscState=0; return true; } // ←
      if (ch == 'H') { if (m_cursor != 0)                { m_cursor = 0; m_dirty = true; } s_inputEscState=0; return true; } // Home
      if (ch == 'F') { uint16_t L=(uint16_t)m_input.length(); if (m_cursor != L) { m_cursor = L; m_dirty = true; } s_inputEscState=0; return true; } // End
      if (ch == '3') { s_inputEscState = 4; return true; } // expect '~' (Delete)
      // Unknown CSI -> swallow
      s_inputEscState = 0; return true;
    }
    if (s_inputEscState == 4) {          // ESC [ 3
      if (ch == '~') {                   // Delete at cursor
        if (m_cursor < m_input.length()) { m_input.remove(m_cursor, 1); m_dirty = true; }
      }
      s_inputEscState = 0; return true;
    }
    if (s_inputEscState == 3) {          // ESC O (SS3)
      if (ch == 'C') { if (m_cursor < m_input.length()) { m_cursor++; m_dirty = true; } s_inputEscState=0; return true; }
      if (ch == 'D') { if (m_cursor > 0)                 { m_cursor--; m_dirty = true; } s_inputEscState=0; return true; }
      if (ch == 'H') { if (m_cursor != 0)                { m_cursor = 0; m_dirty = true; } s_inputEscState=0; return true; }
      if (ch == 'F') { uint16_t L=(uint16_t)m_input.length(); if (m_cursor != L) { m_cursor = L; m_dirty = true; } s_inputEscState=0; return true; }
      s_inputEscState = 0; return true;
    }
  }

  // ENTER -> submit
  if (ch == '\r' || ch == '\n') {
    SubmitFn cb = m_submitCb; String arg = m_input;
    m_submitCb = nullptr; m_cancelCb = nullptr; m_inputActive = false;
    s_inputEscState = 0;
    close(); if (cb) cb(arg); return true;
  }

  // ESC -> start a possible multi-byte sequence. Do NOT sniff serial here.
  if (ch == 0x1B) {
    s_inputEscState = 1;   // got ESC; let subsequent calls deliver '[' or 'O'
    m_dirty = true;        // ensure draw() runs this iteration to finalize cancel if bare
    return true;
  }

  // Backspace (0x08 or 0x7F)
  if (ch == 0x08 || ch == 0x7F) {
    if (m_cursor > 0) {
      m_input.remove((uint16_t)(m_cursor - 1), 1);
      m_cursor--;
      if (m_cursor < m_viewLeft) m_viewLeft = m_cursor;
      m_dirty = true;
    }
    return true;
  }

  // Printable ASCII
  if (ch >= 32 && ch <= 126) {
    if (m_input.length() < m_inputMax) {
      if (m_cursor == m_input.length()) m_input += (char)ch;
      else m_input = m_input.substring(0, m_cursor) + (char)ch + m_input.substring(m_cursor);
      m_cursor++; m_dirty = true;
    }
    return true;
  }

  return true; // ignore everything else
}

} // namespace TUI
