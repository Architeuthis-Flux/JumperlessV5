#ifndef PYTHON_PROPER_H
#define PYTHON_PROPER_H

#include <Arduino.h>

extern Stream *global_mp_stream;

// Forward declaration from existing Python.cpp
extern int parseAndExecutePythonCommand(char* command, char* response);

// Forward declarations from JumperlessMicroPythonAPI.cpp
extern "C" void jl_init_micropython_local_copy(void);
extern "C" void jl_exit_micropython_restore_entry_state(void);
extern "C" void jl_restore_micropython_entry_state(void);
extern "C" int jl_has_unsaved_changes(void);

// Command history and filesystem support
class ScriptHistory {
private:
  static const int MAX_HISTORY = 50;
  String history[MAX_HISTORY];
  int history_count = 0;
  int current_history_index = -1;
  String scripts_dir = "/python_scripts";
  String last_saved_script = ""; // Track most recent saved script
  String saved_scripts[10];      // Track up to 10 saved scripts
  int saved_scripts_count = 0;
  int next_script_number = 1;  // For sequential script naming
  String numbered_scripts[20]; // Map numbers to script names for easy loading
  int numbered_scripts_count = 0;

public:
  void initFilesystem();
  void addToHistory(const String &script);
  String getPreviousCommand();
  String getNextCommand();
  String getCurrentHistoryCommand();
  void resetHistoryNavigation();
  String getLastExecutedCommand();
  String getLastSavedScript();
  int getNextScriptNumber();
  int getNumberedScriptsCount();
  String getNumberedScript(int index);
  bool saveScript(const String &script, const String &filename = "");
  String loadScript(const String &filename);
  bool deleteScript(const String &filename);
  void listScripts();
  void clearHistory();

private:
  void findNextScriptNumber();
  void saveHistoryToFile();
  void loadHistoryFromFile();
};

// Text editor helper functions for REPL
struct REPLEditor {
  String current_input = "";
  int cursor_pos = 0;
  bool in_multiline_mode = false;
  bool first_run = true;
  int escape_state = 0;       // 0=normal, 1=ESC, 2=ESC[
  String original_input = ""; // Store original input before history navigation
  bool in_history_mode = false;
  bool multiline_override = false;   // Manual multiline mode override
  bool multiline_forced_on = false;  // Force multiline mode on
  bool multiline_forced_off = false; // Force multiline mode off
  int last_displayed_lines = 0;      // Track how many lines we last displayed
  bool just_loaded_from_history = false; // Flag to track when we just loaded from history
  String last_displayed_content = "";    // Track the last content we displayed

  // Centralized cursor position management
  struct CursorPosition {
    int line = 0;           // Current line number (0-based)
    int column = 0;         // Current column within line (0-based)
    int total_lines = 0;    // Total number of lines in input
    bool is_valid = false;  // Whether position calculations are current
  } cursor_position;

  // Logical cursor management (updates internal cursor position)
  void updateCursorPosition();                     // Calculate line/column from cursor_pos
  void setCursorFromLineColumn(int line, int col); // Set cursor_pos from line/column
  void moveCursorUp();                             // Move logical cursor up one line
  void moveCursorDown();                           // Move logical cursor down one line
  void moveCursorLeft();                           // Move logical cursor left one character
  void moveCursorRight();                          // Move logical cursor right one character
  void moveCursorToLineStart();                    // Move logical cursor to start of current line
  void moveCursorToLineEnd();                      // Move logical cursor to end of current line
  void moveCursorToEnd();                          // Move logical cursor to end of all content
  
  // Content loading and display
  void loadScriptContent(const String &script, const String &message);
  
  // Terminal control (sends ANSI escape codes)
  void clearToEndOfLine(Stream *stream);
  void clearBelow(Stream *stream);
  void moveCursorToColumn(Stream *stream, int column);
  
  // Display management
  void redrawAndPosition(Stream *stream);          // Redraw content and position cursor
  void repositionCursorOnly(Stream *stream);       // Move cursor without redrawing content
  void resetCursorTracking();                      // Reset cursor position tracking
  void invalidateCursorTracking();                 // Mark cursor position as unknown
  void drawFromCurrentLine(Stream *stream);        // Simple drawing from current line (for history)
  void getCurrentLine(String &line, int &line_start, int &cursor_in_line);
  void backspaceOverNewline(Stream *stream);
  void loadFromHistory(Stream *stream, const String &historical_input);
  void exitHistoryMode(Stream *stream);
  void reset();
  void fullReset(); // Complete reset including multiline mode settings
  void drawPrompt(Stream *stream, int level = 0);
};

// Core initialization and cleanup
void setGlobalStream(Stream *stream);
bool initMicroPythonProper(Stream *stream = global_mp_stream);
void deinitMicroPythonProper(void);

// Code execution
bool executePythonCodeProper(const char* code);
bool executePythonSimple(const char* code, char* response, size_t response_size);

// Single command execution for main.cpp
void getMicroPythonCommandFromStream(Stream *stream = &Serial);
bool initMicroPythonQuiet(void);
bool executeSinglePythonCommand(const char* command, char* result_buffer = nullptr, size_t buffer_size = 0);
bool executeSinglePythonCommandFormatted(const char* command, char* result_buffer, size_t buffer_size);
bool executeSinglePythonCommandFloat(const char* command, float* result);
float quickPythonCommand(const char* command);
String parseCommandWithPrefix(const char* command);
bool isJumperlessFunction(const char* function_name);

// Syntax highlighting helper
void displayStringWithSyntaxHighlighting(const String& text, Stream* stream);

// REPL control
void startMicroPythonREPL(void);
void stopMicroPythonREPL(void);
bool isMicroPythonREPLActive(void);
void processMicroPythonInput(Stream *stream = global_mp_stream);

// REPL helper functions
void loadScriptIntoREPL(const String &script, const String &message);

// Simple blocking REPL function - call from main.cpp
void enterMicroPythonREPL(Stream *stream = global_mp_stream);
void enterMicroPythonREPLWithFile(Stream *stream, const String& filepath);

// Helper functions
void addJumperlessPythonFunctions(void);
void addNodeConstantsToGlobalNamespace(void);
void testGlobalImports(void);
void addMicroPythonModules(bool time = true, bool machine = false, bool os = true, bool math = true, bool gc = true);
void testJumperlessNativeModule(void);
void testStreamRedirection(Stream *newStream);
void testSingleCommandExecution(void);
void testFormattedOutput(void);
void showREPLreference(int verbose = 0);

// Status functions
bool isMicroPythonInitialized(void);
void printMicroPythonStatus(void);

// Interrupt handling
extern bool mp_interrupt_requested; // Global interrupt flag for Ctrl+Q
void mp_hal_check_interrupt(void);

// Terminal color control
void changeTerminalColor(Stream *stream = global_mp_stream, int color = 69, bool bold = true);

#endif // PYTHON_PROPER_H 