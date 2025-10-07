#pragma once
// ASCII-only popup-based file manager for Jumperless TUI
// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include "Tui.h"
#include "TuiPopup.h"
#include "FatFS.h"

// Optional editor entry points. If not linked, weak stubs in the .cpp provide no-ops.
extern String launchEkiloREPL(const char* filename);       // returns text/sentinel if supported; otherwise stub returns ""
extern void   launchEkiloStandalone(const char* filename); // standalone editor; otherwise stub is no-op

namespace TUI {

class PopupFileManager {
public:
  static void runModal(const String& startPath = "/");

private:
  struct Entry {
    String name;
    String fullPath;
    bool   dir   = false;
    size_t size  = 0;
    time_t mtime = 0;
  };
  static int  cmpEntries(const Entry& a, const Entry& b);

  // State
  static String   s_path;
  static int      s_sel;
  static int      s_top;
  static int      s_count;
  static Entry*   s_list;
  static int      s_cap;

  // Derived sizes (from screen fractions)
  static int innerCols();
  static int innerRows();
  static int listRows();

  // UI
  static void openPopup();
  static void closePopup();
  static void draw();
  static void status(const String&);

  // FS ops
  static bool   refresh();
  static void   normalizeSel();
  static void   enter();
  static void   upDir();
  static void   goRoot();

  static void   quickView();
  static void   info();
  static void   makeFile();
  static void   makeDir();
  static void   renameItem();
  static void   deleteItem();
  static void   memoryStat();

  // Helpers
  static String joinPath(const String& base, const String& name);
  static String fmtSize(size_t b);
  static String fmtTime(time_t t);

  // Prompts
  static String promptSync(const String& title, const String& label, const String& initial = "");
  static bool   confirmSync(const String& title, const String& question, bool defNo = true);

  // Event loop
  static bool readKeyChar(int& ch);
  static bool step();

  // Prompt callbacks
  static volatile bool s_promptDone;
  static volatile bool s_promptOk;
  static String        s_promptBuf;

  static void onSubmitThunk(const String& v);
  static void onCancelThunk();
};

} // namespace TUI
