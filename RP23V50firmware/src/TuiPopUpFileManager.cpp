// SPDX-License-Identifier: MIT
#include "TuiPopUpFileManager.h"   // <-- fixed casing
#include "TuiPopup.h"
#include "Tui.h"
#include <cstring>

namespace TUI {

// ---------- Optional editors: weak stubs and detection ----------
static String __stub_launchEkiloREPL(const char* filename) { (void)filename; return String(); }
__attribute__((weak)) String launchEkiloREPL(const char* filename) { return __stub_launchEkiloREPL(filename); }

static void __stub_launchEkiloStandalone(const char* filename) { (void)filename; }
__attribute__((weak)) void launchEkiloStandalone(const char* filename) { __stub_launchEkiloStandalone(filename); }

static inline bool hasREPL()       { return (void*)&launchEkiloREPL       != (void*)&__stub_launchEkiloREPL; }
static inline bool hasStandalone() { return (void*)&launchEkiloStandalone != (void*)&__stub_launchEkiloStandalone; }

// ====== constants for sizing to match Popup::open fractions ======
static constexpr float FM_FRAC_H = 0.60f;   // 60% of screen rows
static constexpr float FM_FRAC_W = 0.80f;   // 80% of screen cols
static constexpr uint16_t FM_MIN_W = 12;
static constexpr uint16_t FM_MIN_H = 6;
static constexpr uint16_t FM_PAD   = 1;     // matches Popup’s PAD in practice

// ====== Host/wrapper handshake over OSC 777 ======
// Spawn:  ESC ] 777 ; spawn ; ekilo ; <path> BEL
// Done:    ESC ] 777 ; done  ; ekilo BEL
// (We also accept ST terminator ESC \ for robustness.)
static inline Stream* ioOut() { return TUI::TUIserial; }  // single source of truth

static void oscSpawnEkilo(const char* path) {
  Stream* io = ioOut();
  if (!io) return;
  io->print("\x1b]777;spawn;ekilo;");
  io->print(path);
  io->print("\x07"); // BEL terminator
  io->flush();
}

static bool waitForOscDoneEkilo(uint32_t timeoutMs /*0 = infinite*/) {
  Stream* io = ioOut();
  if (!io) return true; // nothing to wait on

  uint32_t t0 = millis();
  enum { IDLE, SAW_ESC, SAW_OSC } st = IDLE;
  char buf[128]; uint16_t blen = 0;

  auto resetOsc = [&](){ st = IDLE; blen = 0; };

  while (true) {
    if (timeoutMs && (uint32_t)(millis()-t0) > timeoutMs) return false;

    if (!io->available()) { delay(5); continue; }
    int ch = io->read();
    if (ch < 0) { delay(1); continue; }

    unsigned char c = (unsigned char)ch;

    if (st == IDLE) {
      if (c == 0x1B) { st = SAW_ESC; }
      continue; // ignore other bytes while host owns tty
    }

    if (st == SAW_ESC) {
      if (c == ']') { st = SAW_OSC; blen = 0; }
      else          { st = IDLE; }
      continue;
    }

    // SAW_OSC: accumulate until BEL(0x07) or ST (ESC \)
    if (c == 0x07) { // BEL
      buf[blen] = 0;
      if (strncmp(buf, "777;done;ekilo", 14) == 0) return true;
      resetOsc();
      continue;
    }
    if (c == 0x1B) { // maybe ST
      int c2 = -1; uint32_t t1 = millis();
      while (!io->available() && (millis()-t1) < 10) {}
      if (io->available()) c2 = io->read();
      if (c2 == '\\') {
        buf[blen] = 0;
        if (strncmp(buf, "777;done;ekilo", 14) == 0) return true;
      }
      resetOsc();
      continue;
    }

    if (blen < sizeof(buf)-1) buf[blen++] = (char)c;
    else resetOsc(); // overflow -> reset
  }
}

// Hard clear + show cursor for host-owned session
static inline void hardClear(Stream* io) {
  if (!io) return;
  io->print("\x1b[2J\x1b[H\x1b[?25h");
  io->flush();
}

static void beginExternalSession() {
  Stream* io = ioOut();
  Popup::instance().close();   // also triggers TUI::fullRedraw()
  hardClear(io);               // give host a clean slate
}

static void endExternalSession() {
  Stream* io = ioOut();
  hardClear(io);
  Popup::instance().open("JUMPERLESS FILES", -1, -1, FM_FRAC_H, FM_FRAC_W);
  Popup::instance().clear();
}

// ====== Cached inner sizes (derived from TUI::S + fractions) ======
static int s_innerColsCached = 0;
static int s_innerRowsCached = 0;

static void cacheInnerSize() {
  uint16_t rows = TUI::S.rows ? TUI::S.rows : 24;
  uint16_t cols = TUI::S.cols ? TUI::S.cols : 80;

  const uint16_t maxW = (cols > 2) ? (uint16_t)(cols - 2) : cols;
  const uint16_t maxH = (rows > 2) ? (uint16_t)(rows - 2) : rows;

  uint16_t boxW = (uint16_t)constrain((uint16_t)(cols * FM_FRAC_W), FM_MIN_W, maxW);
  uint16_t boxH = (uint16_t)constrain((uint16_t)(rows * FM_FRAC_H), FM_MIN_H, maxH);

  int innerW = (int)boxW - 2 - 2*FM_PAD;
  int innerH = (int)boxH - 4;

  if (innerW < 10) innerW = 10;
  if (innerH < 6)  innerH = 6;

  s_innerColsCached = innerW;
  s_innerRowsCached = innerH;
}

static inline int innerColsLocal() { if (!s_innerColsCached) cacheInnerSize(); return s_innerColsCached; }
static inline int innerRowsLocal() { if (!s_innerRowsCached) cacheInnerSize(); return s_innerRowsCached; }
static inline int listRowsLocal()  { return max(1, innerRowsLocal() - 5); }

// ===== ASCII width helpers (ANSI-safe; counts visible ASCII chars only) =====
static bool isAnsiStart(const String& s, size_t i) { return i < (size_t)s.length() && (unsigned char)s[i] == 0x1B; }
static size_t skipAnsi(const String& s, size_t i) {
  if (!isAnsiStart(s, i)) return i;
  size_t n = s.length();
  i++; // ESC
  if (i < n && s[i] == '[') {
    i++;
    while (i < n) {
      char c = s[i++];
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == 'm') break;
    }
  }
  return i;
}
static int dispWidthAscii(const String& s) {
  int w = 0;
  size_t i = 0, n = s.length();
  while (i < n) {
    if ((unsigned char)s[i] == 0x1B) { i = skipAnsi(s, i); continue; }
    ++w; ++i; // ASCII only
  }
  return w;
}
static String ellipsizeToAscii(const String& s, int maxw) {
  if (maxw <= 0) return ".";
  int w = dispWidthAscii(s);
  if (w <= maxw) return s;
  if (maxw <= 3) { String dots; for (int i=0;i<maxw;i++) dots += '.'; return dots; }
  String out; out.reserve(maxw);
  int kept = 0;
  for (size_t i=0; i < s.length() && kept < (maxw - 3); ++i) {
    if ((unsigned char)s[i] == 0x1B) { i = skipAnsi(s, i) - 1; continue; }
    out += s[i]; ++kept;
  }
  out += "...";
  return out;
}
static String repeatChar(char c, int count) {
  if (count < 0) count = 0;
  String s; s.reserve(count);
  for (int i=0;i<count;i++) s += c;
  return s;
}

// ======= State =======
String  PopupFileManager::s_path = "/";
int     PopupFileManager::s_sel  = 0;
int     PopupFileManager::s_top  = 0;
int     PopupFileManager::s_count= 0;
PopupFileManager::Entry* PopupFileManager::s_list = nullptr;
int     PopupFileManager::s_cap  = 0;

volatile bool PopupFileManager::s_promptDone = false;
volatile bool PopupFileManager::s_promptOk   = false;
String        PopupFileManager::s_promptBuf  = "";

// Expose size helpers to class
int PopupFileManager::innerCols() { return innerColsLocal(); }
int PopupFileManager::innerRows() { return innerRowsLocal(); }
int PopupFileManager::listRows()  { return listRowsLocal(); }

// ======= Prompt thunks =======
void PopupFileManager::onSubmitThunk(const String& v) { s_promptBuf = v; s_promptOk = true; s_promptDone = true; }
void PopupFileManager::onCancelThunk() { s_promptOk = false; s_promptDone = true; }

// ------------------------ Public entry ------------------------
void PopupFileManager::runModal(const String& startPath) {
  if (!TUI::TUIserial) {
    Popup::instance().open("JUMPERLESS FILES", -1, -1, FM_FRAC_H, FM_FRAC_W);
    Popup::instance().clear();
    status("No UI stream set. Call TuiGlue::setSerial(...) first.");
    delay(1200);
    Popup::instance().close();
    return;
  }

  s_path = startPath.length() ? startPath : "/";
  s_sel = 0; s_top = 0;

  if (!s_list) {
    s_cap = 128;
    s_list = new Entry[s_cap];
  }

  openPopup();
  if (!refresh()) status("Filesystem not available");

  for (;;) {
    Popup::instance().drawIfDirty();
    if (!step()) break;
    delay(1);
  }

  closePopup();
}

// ------------------------ Popup UI ------------------------
void PopupFileManager::openPopup() {
  cacheInnerSize();
  Popup::instance().open("JUMPERLESS FILES", -1, -1, FM_FRAC_H, FM_FRAC_W);
  Popup::instance().clear();
  draw();
}
void PopupFileManager::closePopup() { Popup::instance().close(); }

void PopupFileManager::draw() {
  cacheInnerSize();
  Popup& P = Popup::instance();
  P.clear();

  const int cols = innerCols();

  // Header
  String header = String("Path: ") + s_path + "   (" + String(s_count) + " items)";
  P.println(header);

  // Rule across width
  P.println(repeatChar('-', cols));

  // Listing
  const int rows  = listRows();
  const int start = max(0, s_top);
  const int end   = min(s_count, start + rows);

  for (int i = start; i < end; ++i) {
    const Entry& e = s_list[i];

    const String marker = (i == s_sel) ? "> " : "  ";
    const String icon   = e.dir ? "[D] " : "-  ";
    const String right  = e.dir ? "<DIR>" : fmtSize(e.size);

    const int leftFixedW = dispWidthAscii(marker) + dispWidthAscii(icon);
    const int rightW     = dispWidthAscii(right);
    const int gapMin     = 2;

    int maxNameW = cols - leftFixedW - rightW - gapMin;
    if (maxNameW < 1) maxNameW = 1;

    String name = e.name;
    if (dispWidthAscii(name) > maxNameW) name = ellipsizeToAscii(name, maxNameW);

    String left = marker + icon + name;

    int spaces = cols - dispWidthAscii(left) - rightW;
    if (spaces < 1) spaces = 1;

    String line = left + repeatChar(' ', spaces) + right;

    int pad = cols - dispWidthAscii(line);
    if (pad > 0) line += repeatChar(' ', pad);

    P.println(line);
  }

  // Fill remaining lines
  for (int padLines = end - start; padLines < rows; ++padLines) P.println(String());

  // Status line (reserved)
  P.println(" ");

  // Help footer (ASCII)
  P.println("enter=open  .(up)  /(root)  e=edit  v=view  n=new  d=mkdir  r=refresh  x=del");
  P.println("i=info  u=mem  PgUp/PgDn=jump  q/ESC=quit");
  P.drawIfDirty();
}

void PopupFileManager::status(const String& s) {
  Popup& P = Popup::instance();
  draw();
  P.println("[ " + s + " ]");
  P.drawIfDirty();
}

// ------------------------ Model / FS ------------------------
String PopupFileManager::joinPath(const String& base, const String& name) {
  if (base == "/") return "/" + name;
  return base + "/" + name;
}

int PopupFileManager::cmpEntries(const Entry& a, const Entry& b) {
  if (a.name == "..") return -1;
  if (b.name == "..") return +1;
  if (a.dir != b.dir) return a.dir ? -1 : +1; // dirs first
  return a.name.compareTo(b.name);
}

bool PopupFileManager::refresh() {
  s_count = 0;

  // Parent virtual entry
  if (s_path != "/") {
    if (s_count < s_cap) {
      s_list[s_count].name = "..";
      s_list[s_count].fullPath = "[UP]";
      s_list[s_count].dir = true;
      s_list[s_count].size = 0;
      s_list[s_count].mtime = 0;
      s_count++;
    }
  }

  Dir d = FatFS.openDir(s_path);
  bool ok = false;
  while (d.next()) { ok = true; break; }
  if (!ok && !FatFS.exists(s_path.c_str())) {
    status("Cannot open: " + s_path);
    return false;
  }

  // Rewind & read
  d = FatFS.openDir(s_path);
  while (d.next() && s_count < s_cap) {
    String name = d.fileName();
    if (name.length() && name[0] == '.') continue; // hide dotfiles

    Entry& e = s_list[s_count++];
    e.name = name;
    e.fullPath = joinPath(s_path, name);
    e.dir = d.isDirectory();
    e.size = e.dir ? 0 : d.fileSize();
    e.mtime = d.fileCreationTime();
  }

  // Sort (simple O(n^2) bubble; small lists so fine)
  for (int i=0;i<s_count;i++)
    for (int j=i+1;j<s_count;j++)
      if (cmpEntries(s_list[i], s_list[j]) > 0) {
        Entry t = s_list[i]; s_list[i] = s_list[j]; s_list[j] = t;
      }

  normalizeSel();
  draw();
  return true;
}

void PopupFileManager::normalizeSel() {
  if (s_count == 0) { s_sel = s_top = 0; return; }
  if (s_sel < 0) s_sel = 0;
  if (s_sel >= s_count) s_sel = s_count - 1;

  const int rows = listRows();
  if (s_sel < s_top) s_top = s_sel;
  if (s_sel >= s_top + rows) s_top = s_sel - rows + 1;
}

// ------------------------ Actions ------------------------
void PopupFileManager::enter() {
  if (s_count == 0) return;

  Entry& e = s_list[s_sel];

  // ".." -> up
  if (e.name == ".." && e.fullPath == "[UP]") { upDir(); return; }

  // Directory -> descend
  if (e.dir) {
    s_path = e.fullPath;
    s_sel = s_top = 0;
    if (!refresh()) status("Failed to open: " + e.fullPath);
    return;
  }

  // ---------- File -> launch editor in separate host window and WAIT ----------
  beginExternalSession();

  bool requested = false;

  if (hasREPL()) {
    (void)launchEkiloREPL(e.fullPath.c_str());
    requested = true;
  } else if (hasStandalone()) {
    launchEkiloStandalone(e.fullPath.c_str());
    requested = true;
  } else {
    // Direct host request via OSC (Python wrapper listens for this)
    oscSpawnEkilo(e.fullPath.c_str());
    requested = true;
  }

  // Block until host signals exit. Timeout 0 = infinite.
  if (requested) {
    (void)waitForOscDoneEkilo(0);
  }

  endExternalSession();
  refresh();
  status("Editor closed");
}

void PopupFileManager::upDir() {
  if (s_path == "/") return;
  int slash = s_path.lastIndexOf('/');
  s_path = (slash <= 0) ? "/" : s_path.substring(0, slash);
  s_sel = s_top = 0;
  refresh();
}

void PopupFileManager::goRoot() {
  if (s_path == "/") return;
  s_path = "/";
  s_sel = s_top = 0;
  refresh();
}

void PopupFileManager::quickView() {
  if (s_count == 0) return;
  const Entry& e = s_list[s_sel];
  if (e.dir) { status("Cannot view a directory"); return; }

  File f = FatFS.open(e.fullPath.c_str(), "r");
  if (!f) { status("Open failed: " + e.fullPath); return; }

  Popup& P = Popup::instance();
  P.clear();

  const int cols = innerCols();
  P.println(String("FILE VIEW: ") + e.fullPath);
  P.println(repeatChar('-', cols));

  int lines = min(innerRows() - 2, 18);
  for (int i=0; i<lines && f.available(); ++i) {
    String line = f.readStringUntil('\n');
    if (line.endsWith("\r")) line.remove(line.length()-1);
    if (dispWidthAscii(line) > cols) line = ellipsizeToAscii(line, cols-1);
    P.println(line);
  }
  f.close();
  P.println("");
  P.println("[Any key to return]");
  P.drawIfDirty();

  int ch;
  while (!readKeyChar(ch)) delay(1);

  draw();
}

void PopupFileManager::info() {
  if (s_count == 0) return;
  const Entry& e = s_list[s_sel];
  String s = String("Name: ") + e.name +
             "\nPath: " + (e.name==".." ? "(parent)" : e.fullPath) +
             "\nType: " + (e.dir ? "Directory" : "File");
  if (!e.dir) s += "\nSize: " + fmtSize(e.size);
  if (e.mtime) s += "\nMod:  " + fmtTime(e.mtime);
  status(s);
}

void PopupFileManager::makeFile() {
  String name = promptSync("New file", "Filename:");
  if (name.length()==0) { status("Cancelled"); return; }
  String fp = joinPath(s_path, name);
  if (FatFS.exists(fp.c_str())) { status("Exists: " + name); return; }
  File f = FatFS.open(fp.c_str(), "w");
  if (!f) { status("Create failed"); return; }
  f.close();
  status("Created: " + name);
  refresh();
}

void PopupFileManager::makeDir() {
  String name = promptSync("New directory", "Directory name:");
  if (name.length()==0) { status("Cancelled"); return; }
  String fp = joinPath(s_path, name);
  if (FatFS.exists(fp.c_str())) { status("Exists: " + name); return; }
  if (!FatFS.mkdir(fp.c_str())) { status("mkdir failed"); return; }
  status("Created dir: " + name);
  refresh();
}

void PopupFileManager::renameItem() {
  if (s_count == 0) return;
  const Entry& e = s_list[s_sel];
  if (e.name == "..") { status("Cannot rename '..'"); return; }
  String nn = promptSync("Rename", "New name:", e.name);
  if (nn.length()==0) { status("Cancelled"); return; }
  String dst = joinPath(s_path, nn);
  if (FatFS.exists(dst.c_str())) { status("Exists: " + nn); return; }
  status("Rename not implemented on this FS");
}

void PopupFileManager::deleteItem() {
  if (s_count == 0) return;
  const Entry& e = s_list[s_sel];
  if (e.name == "..") { status("Cannot delete '..'"); return; }
  if (!confirmSync("Delete", String("Delete '") + e.name + "'?")) { status("Cancelled"); return; }

  bool ok = false;
  if (e.dir) ok = FatFS.rmdir(e.fullPath.c_str());
  else       ok = FatFS.remove(e.fullPath.c_str());

  if (ok) status(String("Deleted: ") + e.name);
  else    status(String("Delete failed: ") + e.name);
  refresh();
}

void PopupFileManager::memoryStat() {
  size_t freeHeap = rp2040.getFreeHeap();
  status("Memory: " + String(freeHeap/1024) + " KB free; items cap " + String(s_cap));
}

// ------------------------ Prompts ------------------------
String PopupFileManager::promptSync(const String& title, const String& label, const String& initial) {
  s_promptDone = false;
  s_promptOk   = false;
  s_promptBuf  = "";

  Popup& P = Popup::instance();
  P.openInput(title, label);
  if (initial.length()) P.setInitialInput(initial);

  P.setOnSubmit(&PopupFileManager::onSubmitThunk);
  P.setOnCancel(&PopupFileManager::onCancelThunk);

  int ch;
  while (!s_promptDone) {
    if (readKeyChar(ch)) P.handleKey(ch);
    P.drawIfDirty();
    delay(1);
  }

  P.close();
  openPopup();
  refresh();

  return s_promptOk ? s_promptBuf : String("");
}

bool PopupFileManager::confirmSync(const String& title, const String& question, bool defNo) {
  String prompt = question + (defNo ? " (y/N)" : " (Y/n)");
  String r = promptSync(title, prompt, "");
  if (!r.length()) return false;
  char c = r[0];
  if (defNo) return (c=='y' || c=='Y');
  else       return !(c=='n' || c=='N');
}

// ------------------------ Helpers ------------------------
String PopupFileManager::fmtSize(size_t b) {
  if (b < 1024) return String(b) + " B";
  if (b < 1024UL*1024UL) return String(b/1024) + " KB";
  return String(b/(1024UL*1024UL)) + " MB";
}
String PopupFileManager::fmtTime(time_t ts) {
  if (ts == 0) return "Unknown";
  struct tm* ti = localtime(&ts);
  if (!ti) return String((long)ts);
  char buf[32];
  strftime(buf, sizeof(buf), "%m/%d/%y %H:%M", ti);
  return String(buf);
}

// ------------------------ Input loop ------------------------
bool PopupFileManager::readKeyChar(int& ch) {
  ch = -1;
  Stream* s = TUI::TUIserial;
  if (!s) return false;

  if (!s->available()) {
    delayMicroseconds(300);
    if (!s->available()) return false;
  }
  ch = s->read();
  return true;
}

bool PopupFileManager::step() {
  int ch;
  if (!readKeyChar(ch)) return true;

  if (ch == 0x1B) { // ESC
    int a = -1, b = -1;
    unsigned long t0 = millis();
    Stream* s = ioOut();
    if (!s) return false;

    while ((millis()-t0) < 3 && !s->available()) {}
    if (s->available()) a = s->read();
    if (a == '[') {
      while ((millis()-t0) < 6 && !s->available()) {}
      if (s->available()) b = s->read();
      if (b == 'A') { s_sel--; normalizeSel(); draw(); return true; } // Up
      if (b == 'B') { s_sel++; normalizeSel(); draw(); return true; } // Down
      if (b == '5') { // PgUp ~
        while ((millis()-t0) < 6 && !s->available()) {}
        if (s->available() && s->read()=='~') {
          s_sel = max(0, s_sel - listRows()); normalizeSel(); draw(); return true;
        }
      }
      if (b == '6') { // PgDn ~
        while ((millis()-t0) < 6 && !s->available()) {}
        if (s->available() && s->read()=='~') {
          s_sel = min(s_count-1, s_sel + listRows()); normalizeSel(); draw(); return true;
        }
      }
      return true; // swallow other ESC sequences
    }
    return false; // bare ESC quits
  }

  if (ch == '\r' || ch == '\n') { enter(); return true; }
  if (ch == 'q' || ch == 'Q' || ch == 17 /*Ctrl-Q*/) return false;

  switch (ch) {
    case 'w': case 'W': s_sel--; normalizeSel(); draw(); break;
    case 's': case 'S': s_sel++; normalizeSel(); draw(); break;
    case '.':           upDir(); break;
    case '/':           goRoot(); break;
    case 'r': case 'R': refresh(); status("Refreshed"); break;
    case 'v': case 'V': quickView(); break;
    case 'e': case 'E': enter(); break;
    case 'i': case 'I': info(); break;
    case 'n': case 'N': makeFile(); break;
    case 'd': case 'D': makeDir(); break;
    case 'x': case 'X': deleteItem(); break;
    case 'u': case 'U': memoryStat(); break;
    default: break;
  }
  return true;
}

} // namespace TUI
