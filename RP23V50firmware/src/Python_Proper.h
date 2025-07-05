#ifndef PYTHON_PROPER_H
#define PYTHON_PROPER_H

#include <Arduino.h>

extern Stream *global_mp_stream;

// Forward declaration from existing Python.cpp
extern int parseAndExecutePythonCommand(char* command, char* response);

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

  // Methods
  void getCurrentLine(String &line, int &line_start, int &cursor_in_line);
  void moveCursorToColumn(Stream *stream, int column);
  void clearToEndOfLine(Stream *stream);
  void clearEntireLine(Stream *stream);
  void clearScreen(Stream *stream);
  void clearBelow(Stream *stream);
  void moveCursorUp(Stream *stream, int lines = 1);
  void moveCursorDown(Stream *stream, int lines = 1);
  void redrawCurrentLine(Stream *stream);
  void navigateToLine(Stream *stream, int target_line);
  void backspaceOverNewline(Stream *stream);
  void navigateOverNewline(Stream *stream);
  void loadFromHistory(Stream *stream, const String &historical_input);
  void exitHistoryMode(Stream *stream);
  void redrawFullInput(Stream *stream);
  void reset();
  void fullReset(); // Complete reset including multiline mode settings
  void enterPasteMode(Stream *stream);
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

// REPL control
void startMicroPythonREPL(void);
void stopMicroPythonREPL(void);
bool isMicroPythonREPLActive(void);
void processMicroPythonInput(Stream *stream = global_mp_stream);

// Simple blocking REPL function - call from main.cpp
void enterMicroPythonREPL(Stream *stream = global_mp_stream);

// Helper functions
void addJumperlessPythonFunctions(void);
void addNodeConstantsToGlobalNamespace(void);
void testGlobalImports(void);
void addMicroPythonModules(bool time = true, bool machine = false, bool os = true, bool math = true, bool gc = true);
void testJumperlessNativeModule(void);
void testStreamRedirection(Stream *newStream);
void testSingleCommandExecution(void);
void testFormattedOutput(void);

// Status functions
bool isMicroPythonInitialized(void);
void printMicroPythonStatus(void);

// Interrupt handling
extern bool mp_interrupt_requested; // Global interrupt flag for Ctrl+Q
void mp_hal_check_interrupt(void);

// Terminal color control
void changeTerminalColor(Stream *stream = global_mp_stream, int color = 69, bool bold = true);

#endif // PYTHON_PROPER_H 