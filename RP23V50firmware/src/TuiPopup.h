// SPDX-License-Identifier: MIT
// TuiPopup.h — reusable popup manager (no dependency on TuiGlue)

#pragma once
#include <Arduino.h>

class Stream;

namespace TUI {

class Popup {
public:
  static Popup& instance();

  void setSerial(Stream* ser);  // must be set by host (e.g., TuiGlue)
  bool isActive() const;

  // Open/close/printing
  void open(const String& title,
            int16_t boxRows = -1, int16_t boxCols = -1,
            float fracRows = 0.0f, float fracCols = 0.0f);
  void close();
  void clear();
  void print(const String& s);
  void println(const String& s);
  void autoCloseMs(uint32_t ms);

  // Rendering + input handling
  void drawIfDirty();
  bool handleKey(int ch);

private:
  Popup() = default;

  // drawing helpers
  void draw();
  void computeBox(uint16_t& r0, uint16_t& c0, uint16_t& h, uint16_t& w);
  void drawBorder(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w);
  void drawTitle(uint16_t r0, uint16_t c0, uint16_t w);
  void drawHelp(uint16_t r0, uint16_t c0, uint16_t h, uint16_t w);
  void drawWrappedContent(uint16_t rStart, uint16_t cStart, uint16_t innerH, uint16_t innerW);
  void normalizeScroll(uint16_t innerH, uint16_t innerW);
  uint16_t countWrappedRows(const String& S, uint16_t innerW);
  bool handleArrowSeq();
  static bool endsWithNewline(const String& s);
  static void mv(Stream* ser, uint16_t r, uint16_t c);
  static void hline(Stream* ser, uint16_t len, char ch);
  static void hlineGlyph(Stream* ser, uint16_t len, const char* g);

  // state
  Stream*  m_ser = nullptr;

  bool     m_active      = false;
  bool     m_dirty       = false;
  String   m_title;

  static constexpr uint16_t MAX_LINES = 160;
  static constexpr uint16_t PAD       = 2;
  String   m_lines[MAX_LINES];
  uint16_t m_lineCount   = 0;
  uint16_t m_scroll      = 0;
  uint32_t m_autoCloseAt = 0;

  // requested sizing from caller
  int16_t  m_userBoxH = -1, m_userBoxW = -1;
  float    m_userFracH = 0.0f, m_userFracW = 0.0f;

  // computed box geometry (including borders)
  uint16_t m_boxR=1, m_boxC=1, m_boxH=0, m_boxW=0;
};

// ---- Back-compat free functions (keep your current code unchanged) ----
void popupSetSerial(Stream* s);
inline bool isPopupActive() { return Popup::instance().isActive(); }
inline void popupOpen(const String& title, int16_t r=-1, int16_t c=-1, float fr=0.0f, float fc=0.0f) {
  Popup::instance().open(title, r, c, fr, fc);
}
inline void popupClose()                  { Popup::instance().close(); }
inline void popupClear()                  { Popup::instance().clear(); }
inline void popupPrint(const String& s)   { Popup::instance().print(s); }
inline void popupPrintln(const String& s) { Popup::instance().println(s); }
inline void popupAutoCloseMs(uint32_t ms) { Popup::instance().autoCloseMs(ms); }
inline void popupDrawIfDirty()            { Popup::instance().drawIfDirty(); }
inline bool popupHandleKey(int ch)        { return Popup::instance().handleKey(ch); }

} // namespace TUI
