// RP23V50firmware/src/TuiGlue.h
#pragma once

#include <Arduino.h>
#include "ArduinoStuff.h"



namespace TuiGlue {

  Stream *TUIserial = &Ser3;

  // Build the TUI model (menus, titles). Safe to call multiple times.
  void init();

  // Bring the TUI up immediately (CPR probe + resize + full redraw).
  // Safe to call even if already active.
  void openOnDemand();

  // True if the TUI is active (drawing/responding to input).
  bool isActive();

  // Drive the UI from your Arduino loop().
  void loop();
}
