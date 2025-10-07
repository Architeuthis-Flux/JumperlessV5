#pragma once
#include <Arduino.h>

class TuiGlue {
public:
  explicit TuiGlue(Stream* serial = nullptr);

  // Public API
  void setSerial(Stream* s);
  void init();
  void openOnDemand();
  bool isActive() const;
  void loop();

private:
  // State
  Stream*  m_serial            = nullptr;
  bool     m_modelBuilt        = false;
  bool     m_active            = false;
  bool     m_needDeferredRedraw= false;
  uint32_t m_nextRetryAtMs     = 0;
  uint8_t  m_redrawTries       = 0;
  bool     m_seenHostByte      = false;
  bool     m_lastDTR           = false;   // USB CDC only
  bool     m_wasPopup          = false;
  uint32_t m_ignoreHostUntilMs = 0;

  // Internals
  void activate();
  void checkUSBconnection();

  // Utility helpers
  static bool isNoiseByte(int ch);
  static void drainHost(Stream* s, uint32_t forMs = 0);
};
