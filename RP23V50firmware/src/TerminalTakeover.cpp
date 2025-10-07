#include "TerminalTakeover.h"

static inline void writeStr(Stream* io, const char* s) { io->print(s); }

void TerminalTakeover::drain(Stream& s, uint32_t ms) {
  uint32_t tEnd = millis() + ms;
  do {
    while (s.available()) (void)s.read();
    if (ms && (int32_t)(millis() - tEnd) < 0) delay(2);
  } while (ms && (int32_t)(millis() - tEnd) < 0);
}

void TerminalTakeover::altOn()  {
  if (!m_opt.useAltScreen) return;
  writeStr(m_io, "\x1b[?1049h\x1b[?1047h\x1b[?47h");
}
void TerminalTakeover::altOff() {
  if (!m_opt.useAltScreen) return;
  writeStr(m_io, "\x1b[?1049l\x1b[?1047l\x1b[?47l");
}
void TerminalTakeover::clearAndHome() {
  writeStr(m_io, "\x1b[2J\x1b[H");
}

TerminalTakeover::TerminalTakeover(Stream* io)
: m_io(io), m_opt(Options()), m_active(false) {
  if (!m_io) return;
  enter();
}

TerminalTakeover::TerminalTakeover(Stream* io, const Options& opt)
: m_io(io), m_opt(opt), m_active(false) {
  if (!m_io) return;
  enter();
}

TerminalTakeover::~TerminalTakeover() {
  leave();
}

void TerminalTakeover::enter() {
  if (m_active || !m_io) return;

  altOn();
  clearAndHome();
  writeStr(m_io, "\x1b[?25h");

  m_io->print("\r\n\x1b[1m");
  m_io->print(m_opt.title ? m_opt.title : "Interactive session");
  m_io->print("\x1b[0m\r\n");
  if (m_opt.help && *m_opt.help) { m_io->print(m_opt.help); m_io->print("\r\n"); }
  m_io->print("\r\n");
  m_io->flush();

  if (m_opt.preDrainMs) drain(*m_io, m_opt.preDrainMs);
  m_active = true;
}

void TerminalTakeover::leave() {
  if (!m_active || !m_io) return;
  m_io->print("\r\n");
  m_io->flush();
  altOff();
  m_io->flush();
  if (m_opt.postDrainMs) drain(*m_io, m_opt.postDrainMs);
  m_active = false;
}

void TerminalTakeover::nudgeFriendlyRepl() {
  if (!m_io) return;
  m_io->write('\x02'); // ^B
  m_io->write('\r');
  m_io->flush();
}
