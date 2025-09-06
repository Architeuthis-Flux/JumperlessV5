// SPDX-License-Identifier: MIT
// TuiPopup.cpp — implementation
#include "TuiPopup.h"
#include "Tui.h"        // uses TUI::S (rows/cols), TUI::THEME.borderStyle, TUI::fullRedraw()
#include <Arduino.h>

namespace TUI {

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

  m_dirty = true;
}

void Popup::close() {
  m_active = false;
  m_autoCloseAt = 0;
  m_dirty = false;
  // Let the main UI repaint itself
  TUI::fullRedraw();
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

void Popup::drawHelp(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w) {
  if (!m_ser) return;
  const char* help = "↑/↓ PgUp/PgDn scroll — ESC close";
  uint16_t len = (uint16_t)strlen(help);
  uint16_t tstart = (uint16_t)(c0 + (w - len)/2);
  mv(m_ser, r0 + h - 1, tstart); m_ser->print(help);
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
  if (total <= innerH) { m_scroll = 0; return; }
  if (m_scroll + innerH > total) m_scroll = (uint16_t)(total - innerH);
}

void Popup::drawWrappedContent(uint16_t rStart, uint16_t cStart, uint16_t innerH, uint16_t innerW) {
  if (!m_ser) return;
  for (uint16_t i=0;i<innerH;i++) { mv(m_ser, rStart + i, cStart); hline(m_ser, innerW, ' '); }

  uint16_t skip = m_scroll, printed = 0;
  for (uint16_t li=0; li<m_lineCount && printed < innerH; li++) {
    const String& S = m_lines[li];
    uint16_t n = (uint16_t)S.length();
    if (innerW == 0) break;

    if (n == 0) {
      if (skip > 0) { skip--; } else { mv(m_ser, rStart + printed, cStart); printed++; }
      continue;
    }

    uint16_t p = 0;
    while (p < n && printed < innerH) {
      String out; out.reserve(innerW);
      uint16_t q = p; while (q < n && S[q] == ' ') q++;
      if (q >= n) { if (skip > 0) skip--; else { mv(m_ser, rStart + printed, cStart); printed++; } p = q; break; }

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
      else { mv(m_ser, rStart + printed, cStart); if (out.length() > 0) m_ser->print(out); printed++; }

      if (p < q) p = q;
    }
  }
}

void Popup::draw() {
  if (!m_active || !m_ser) return;

  uint16_t r0,c0,h,w;
  computeBox(r0,c0,h,w);

  uint16_t innerW = (uint16_t)(w - 2 - 2*PAD);
  uint16_t innerH = (uint16_t)(h - 4);

  normalizeScroll(innerH, innerW);

  drawBorder(r0,c0,h,w);
  drawTitle(r0,c0,w);
  drawHelp(r0,c0,h,w);

  uint16_t rStart = (uint16_t)(r0 + 2);
  uint16_t cStart = (uint16_t)(c0 + 1 + PAD);
  drawWrappedContent(rStart, cStart, innerH, innerW);
}

// Reads additional bytes from the same serial stream when first byte was ESC
bool Popup::handleArrowSeq() {
  if (!m_ser) return true;
  char b[4] = {0,0,0,0};
  int n = 0;
  while (m_ser->available() && n < 3) b[n++] = (char)m_ser->read();

  auto lineUp = [&](){
    if (m_scroll>0) { m_scroll--; m_dirty = true; }
  };
  auto lineDown = [&](){
    uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
    uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
    m_scroll++;
    normalizeScroll(innerH, innerW);
    m_dirty = true;
  };
  auto page = [&](int dir){
    uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
    uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
    if (dir < 0) {
      m_scroll = (m_scroll > innerH) ? (uint16_t)(m_scroll - innerH) : 0;
    } else {
      m_scroll = (uint16_t)(m_scroll + innerH);
      normalizeScroll(innerH, innerW);
    }
    m_dirty = true;
  };

  if (n >= 1 && b[0] == 'A') { lineUp();   return true; }   // Up
  if (n >= 1 && b[0] == 'B') { lineDown(); return true; }   // Down
  if (n >= 2 && b[0] == '5' && b[1] == '~') { page(-1); return true; } // PgUp
  if (n >= 2 && b[0] == '6' && b[1] == '~') { page(+1); return true; } // PgDn
  return true; // swallow anything else
}

bool Popup::handleKey(int ch) {
  if (!m_active) return false;

  // Auto close
  if (m_autoCloseAt && (int32_t)(millis() - m_autoCloseAt) >= 0) {
    close();
    return true;
  }

  // ESC-prefixed sequences (arrows/pages) before treating ESC as close
  if (ch == 0x1B) {
    // small coalescing wait so ESC-[ lands
    int next = -1;
    uint32_t t0 = millis();
    while ((!m_ser || !m_ser->available()) && (millis() - t0) < 10) { /* wait */ }
    if (m_ser && m_ser->available()) next = m_ser->read();

    if (next == '[') return handleArrowSeq();

    if (next == 'O') {
      int b = -1;
      uint32_t t1 = millis();
      while ((!m_ser || !m_ser->available()) && (millis() - t1) < 10) { }
      if (m_ser && m_ser->available()) b = m_ser->read();
      if (b == 'A') { if (m_scroll>0) { m_scroll--; m_dirty = true; } return true; } // Up
      if (b == 'B') { uint16_t innerW = (uint16_t)(m_boxW > 0 ? (m_boxW - 2 - 2*PAD) : 60);
                      uint16_t innerH = (uint16_t)(m_boxH > 4 ? (m_boxH - 4) : 8);
                      m_scroll++; normalizeScroll(innerH, innerW); m_dirty = true; return true; } // Down
      return true;
    }

    if (next < 0) { close(); return true; } // bare ESC closes
    return true; // swallow any other ESC sequence
  }

  // swallow everything else (modal)
  return true;
}

} // namespace TUI
