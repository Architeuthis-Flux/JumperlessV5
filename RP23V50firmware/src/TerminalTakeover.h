#pragma once
#include <Arduino.h>

class TerminalTakeover {
public:
  struct Options {
    const char* title;
    const char* help;
    bool  useAltScreen;
    bool  showCursor;
    uint16_t preDrainMs;
    uint16_t postDrainMs;

    Options()
    : title("Interactive session"),
      help(nullptr),
      useAltScreen(true),
      showCursor(true),
      preDrainMs(10),
      postDrainMs(20) {}
  };

  explicit TerminalTakeover(Stream* io);                    // uses default Options()
  TerminalTakeover(Stream* io, const Options& opt);         // explicit options
  ~TerminalTakeover();

  static void drain(Stream& s, uint32_t ms=0);

  void leave();
  void enter();
  void nudgeFriendlyRepl();

private:
  Stream*  m_io;
  Options  m_opt;
  bool     m_active;

  void altOn();
  void altOff();
  void clearAndHome();
};
