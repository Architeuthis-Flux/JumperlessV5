#include "Python_Proper.h"
#include <Arduino.h>
#include <FatFS.h>
#include "config.h"
#include "FilesystemStuff.h"
#include "EkiloEditor.h"
#include "FileParsing.h"
#include "CH446Q.h"
#include "Commands.h"

extern "C" {
#include "py/gc.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/mpstate.h"
#include "py/repl.h"
#include "py/mpthread.h"
#include <micropython_embed.h>
}

// Global state for proper MicroPython integration
static char mp_heap[64 * 1024]; // 64KB heap for MicroPython (reduced to free memory for editor)
static bool mp_initialized = false;
static bool mp_repl_active = false;
static bool jumperless_globals_loaded = false;
bool mp_interrupt_requested = false; // Flag for Ctrl+Q interrupt

// Global state for REPL initial file loading
static String repl_initial_filepath = "";
static bool repl_has_initial_file = false;

// Keyboard interrupt character storage
static int keyboard_interrupt_char = 17; // Default to Ctrl+Q (ASCII 17)

// Command execution state
static char mp_command_buffer[512];
static bool mp_command_ready = false;
static char mp_response_buffer[1024];

// Terminal colors for different REPL states
/// 0 = menu (cyan) 1 = prompt (light blue) 2 = output (chartreuse) 3 = input
/// (orange-yellow) 4 = error (orange-red) 5 = purple 6 = dark purple 7 = light
/// cyan 8 = magenta 9 = pink 10 = green 11 = grey 12 = dark grey 13 = light
/// grey
static int replColors[15] = {
    38,  // menu (cyan)
    69,  // prompt (light blue)
    155, // output (chartreuse)
    221, // input (orange-yellow)
    202, // error (orange-red)
    92,  // purple
    56,  // dark purple
    51,  // light cyan
    199, // magenta
    207, // pink
    40,  //  green
    8,   // grey
    235, // dark grey
    248, // light grey

};

Stream *global_mp_stream = &Serial;

// C-compatible pointer for HAL functions
extern "C" {
    void *global_mp_stream_ptr = (void *)&Serial;
}

// Forward declaration for color function (from Graphics.cpp)
void changeTerminalColor(int termColor, bool flush, Stream *stream);

// Forward declarations
extern "C" {
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);
int mp_hal_stdin_rx_chr(void);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len);
void mp_hal_delay_ms(mp_uint_t ms);
mp_uint_t mp_hal_ticks_ms(void);

// Forward declaration for filesystem setup
void setupFilesystemAndPaths(void);

// Forward declaration for interrupt checking  
// (Note: Actual function is C++ linkage for use by other modules)

// Arduino wrapper functions for HAL
int arduino_serial_available(Stream *stream = global_mp_stream);
int arduino_serial_read(Stream *stream = global_mp_stream);
void arduino_serial_write(const char *str, int len, void *stream);
void arduino_delay_ms(unsigned int ms);
unsigned int arduino_millis(void);

// Export global_mp_stream for C code
extern void *global_mp_stream_ptr;
}

// Arduino timing functions for MicroPython
extern "C" void mp_hal_delay_ms(mp_uint_t ms) { 
  // Check for interrupt during delays
  unsigned int start_time = millis();
  while (millis() - start_time < ms) {
    // Check for interrupt every millisecond during delays
    mp_hal_check_interrupt();
    delay(1); // Small delay to prevent overwhelming the system
  }
}

extern "C" mp_uint_t mp_hal_ticks_ms(void) { 
  // // Check for interrupt during timing calls (called frequently)
  // mp_hal_check_interrupt();
  return millis(); 
}

// Arduino wrapper functions for the HAL layer
extern "C" int arduino_serial_available(Stream *stream) {
  // Check for interrupt request before checking availability
  // mp_hal_check_interrupt();
  return global_mp_stream->available();
}

extern "C" int arduino_serial_read(Stream *stream) {
  // Check for interrupt request before reading
  mp_hal_check_interrupt();
  return global_mp_stream->read();
}

extern "C" void arduino_serial_write(const char *str, int len, void *stream) {
  Stream *s = (Stream *)stream;
  if (s) {
    // Convert \n to \r\n for proper terminal display
    for (int i = 0; i < len; i++) {

      s->write(str[i]);

      if (str[i] == '\n') {
        s->write('\r');
      }
    }
    s->flush();
  }
}

extern "C" void arduino_delay_ms(unsigned int ms) { 
  // Check for interrupt during delays
  mp_hal_check_interrupt();
  delay(ms); 
}

extern "C" unsigned int arduino_millis() { return millis(); }

// HAL function to set the keyboard interrupt character
extern "C" mp_uint_t mp_hal_set_interrupt_char(int c) {
    keyboard_interrupt_char = c;
    if (global_mp_stream) {
        char char_name = (c >= 1 && c <= 26) ? (char)(c + 64) : '?';
        global_mp_stream->printf("[MP] Keyboard interrupt character set to Ctrl+%c (ASCII %d)\n", char_name, c);
    }
    return 0;
}

// Helper function to get current interrupt character
extern "C" int getCurrentInterruptChar(void) {
    return keyboard_interrupt_char;
}

void setGlobalStream(Stream *stream) {
  global_mp_stream = stream;
  global_mp_stream_ptr = (void *)stream;
}

// Terminal color control function is now in Graphics.cpp

// MicroPython HAL stdout function with Jumperless-specific functionality
extern "C" void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // Check for interrupt before outputting (this is called frequently)
    //mp_hal_check_interrupt();
    
    // Basic output to global stream (regular MicroPython output)
    if (global_mp_stream) {
        for (size_t i = 0; i < len; i++) {
            // // Check for interrupt more frequently during output
            // if (i % 10 == 0) { // Check every 10 characters
            //     mp_hal_check_interrupt();
            // }
            
            if (str[i] == '\n') {
                global_mp_stream->write('\r');
            }
            global_mp_stream->write(str[i]);
        }
        global_mp_stream->flush();
    }
}

// HAL functions are now implemented in lib/micropython/port/mphalport.c

// Function to check for interrupt and raise KeyboardInterrupt if requested
void mp_hal_check_interrupt(void) {
  // More frequent checking - check every call for better responsiveness
  static uint32_t last_interrupt_time = 0;
  static uint32_t last_check_time = 0;
  static uint32_t debug_counter = 0;
  debug_counter++;
  
  // Throttle checking to avoid overwhelming the system, but check more frequently than before
  uint32_t current_time = millis();
  if (current_time - last_check_time < 1) { // Check at most every 1ms instead of every call
    return;
  }
  last_check_time = current_time;
  
  // Check for Ctrl+Q more frequently - check every call when stream is available
  if (global_mp_stream) {
    // Check if there's any input available
    while (global_mp_stream->available()) {
      int c = global_mp_stream->read(); // Read and consume immediately
      
      if (c == keyboard_interrupt_char) { // Check configured interrupt char
        // Reduce debounce time for faster response in tight loops
        if (current_time - last_interrupt_time > 50) { // Reduced from 100ms to 50ms
          char char_display = (keyboard_interrupt_char >= 1 && keyboard_interrupt_char <= 26) ? 
                             (char)(keyboard_interrupt_char + 64) : '?';
          global_mp_stream->printf("^%c\n", char_display);
          if (global_mp_stream) {
            changeTerminalColor(replColors[4], true, global_mp_stream);
            global_mp_stream->printf("KeyboardInterrupt (Ctrl+%c)\n\r", char_display);
            changeTerminalColor(replColors[1], true, global_mp_stream);
            mp_raise_type(&mp_type_KeyboardInterrupt);
          }
          last_interrupt_time = current_time;
          
          // Set flag for interrupt - the actual exception will be raised at Python level
          mp_interrupt_requested = true;
          return; // Exit immediately when interrupt detected
        } else {
          // Too soon after last interrupt, ignore
        }
      }
      // For any other character, just consume it and continue
    }
  }
}




bool initMicroPythonProper(Stream *stream) {
  // global_mp_stream = stream;

  if (mp_initialized) {
    return true;
  }

  global_mp_stream->println(
      "[MP] Initializing MicroPython...");

  // Get proper stack pointer
  char stack_dummy;
  char *stack_top = &stack_dummy;
    // Set up Python path and basic modules (with error handling)
  // mp_embed_exec_str(
  //     "try:\n"
  //     "    print('MicroPython initializing...')\n"
  //     "    import sys\n"
  //     "    sys.path.append('/lib')\n"
  //     "    print('MicroPython', sys.version)\n"
  //     "except ImportError as e:\n"
  //     "    print('MicroPython initialized (sys modu le unavailable)')\n"
  //     "print('\\nJumperless hardware control available')\n");
  // changeTerminalColor(replColors[5], true, global_mp_stream);
  // // Test if native jumperless module is available
  // mp_embed_exec_str(
  //     "try:\n"
  //     "    import jumperless\n"
  //     // "    import time\n"
  //     "    print('Native jumperless module imported successfully')\n"
  //     "    print('Available functions:')\n"
  //     "    for func in dir(jumperless):\n"
  //     "        if not func.startswith('_'):\n"
  //     "            print('  jumperless.' + func + '()')\n"
  //     "    print()  # Empty line\n"
  //     "except ImportError as e:\n"
  //     "    print('❌ Native jumperless module not available:', str(e))\n"
  //     "    print('Loading Python wrapper functions instead...')\n"
  //     "    print()  # Empty line\n");
  // Initialize MicroPython

  changeTerminalColor(replColors[11], true, global_mp_stream);
  mp_embed_init(mp_heap, sizeof(mp_heap), stack_top);

  // Set Ctrl+Q (ASCII 17) as the keyboard interrupt character instead of Ctrl+C (ASCII 3)
  // This enables proper KeyboardInterrupt exceptions that can be caught by try/except
  // and will automatically interrupt running loops/scripts when Ctrl+Q is pressed
  mp_embed_exec_str("import micropython; micropython.kbd_intr(17)");

  // Simple initialization - don't load complex modules during startup
  // mp_embed_exec_str("print('MicroPython ready for Jumperless')");
  changeTerminalColor(replColors[11], true, global_mp_stream);
  // Set up filesystem and module import paths

  setupFilesystemAndPaths();
    
    mp_initialized = true;
  mp_repl_active = false;

  addJumperlessPythonFunctions();

  changeTerminalColor(replColors[11], true, global_mp_stream);
  addMicroPythonModules();

  changeTerminalColor(replColors[11], true, global_mp_stream);
  global_mp_stream->println("[MP] MicroPython initialized successfully");

  changeTerminalColor(replColors[11], true, global_mp_stream);
  //global_mp_stream->println("[MP] interrupt char: " + String(keyboard_interrupt_char));
  return true;
}

void deinitMicroPythonProper(void) {
  if (mp_initialized) {
    global_mp_stream->println("[MP] Deinitializing MicroPython...");
    
    // Close any open files before deinitializing MicroPython
    closeAllOpenFiles();
    
    mp_embed_deinit();
    mp_initialized = false;
    mp_repl_active = false;
    jumperless_globals_loaded = false;  // Reset globals flag
    mp_interrupt_requested = false;  // Clear any pending interrupt
  }
}

bool executePythonCodeProper(const char *code) {
  if (!mp_initialized) {
    global_mp_stream->println("[MP] Error: MicroPython not initialized");
    return false;
  }

  if (!code || strlen(code) == 0) {
    return false;
  }

  // Clear response buffer
  memset(mp_response_buffer, 0, sizeof(mp_response_buffer));

  // Execute the code with proper error handling
  // MicroPython handles errors internally and prints them
  mp_embed_exec_str(code);
  return true;
}

void startMicroPythonREPL(void) {
  if (!mp_initialized) {
    changeTerminalColor(replColors[4], true, global_mp_stream);
    global_mp_stream->println("[MP] Error: MicroPython not initialized");
    return;
  }

  if (mp_repl_active) {
    changeTerminalColor(replColors[4], true, global_mp_stream);
    global_mp_stream->println("[MP] REPL already active");
    return;
  }

  // Clear any pending interrupt flag when starting REPL
  mp_interrupt_requested = false;

  // Print Python prompt with color
  changeTerminalColor(replColors[1], true, global_mp_stream);
  global_mp_stream->print(
      ">>> "); // Simple prompt - the processMicroPythonInput handles everything
  global_mp_stream->flush();

  mp_repl_active = true;
}

void stopMicroPythonREPL(void) {
  if (mp_repl_active) {
    changeTerminalColor(0, false, global_mp_stream);
    global_mp_stream->println("\n[MP] Exiting REPL...");
    
    // Restore to entry state (discard Python changes by default)
    jl_exit_micropython_restore_entry_state();
    
    // Close any open files before exiting REPL
    closeAllOpenFiles();
    
    mp_repl_active = false;
    mp_interrupt_requested = false; // Clear any pending interrupt
  }
}

bool isMicroPythonREPLActive(void) { return mp_repl_active; }

// Helper function to set initial file for REPL
void setREPLInitialFile(const String& filepath) {
  repl_initial_filepath = filepath;
  repl_has_initial_file = (filepath.length() > 0);
}

void enterMicroPythonREPL(Stream *stream) {
  enterMicroPythonREPLWithFile(stream, "");
}

void enterMicroPythonREPLWithFile(Stream *stream, const String& filepath) {
  // Colorful initialization like original implementation
  changeTerminalColor(replColors[6], true, global_mp_stream);

  // Initialize MicroPython if not already done
  if (!mp_initialized) {
    if (!initMicroPythonProper()) {
      changeTerminalColor(replColors[4], true, global_mp_stream); // error color
      global_mp_stream->println("Failed to initialize MicroPython!");
      return;
    }
  }
  
  // Always add jumperless functions to global namespace when entering REPL
  // This makes all functions available without the jumperless. prefix
  addJumperlessPythonFunctions();

  // Initialize local copy of current nodefile for faster operations
  jl_init_micropython_local_copy();

  // Automatically create MicroPython examples if needed
  initializeMicroPythonExamples();

  // Check if REPL is already active
  if (mp_repl_active) {
    changeTerminalColor(replColors[4], true, global_mp_stream);
    global_mp_stream->println("[MP] REPL already active");
    return;
  }

  // Set initial file if provided
  if (filepath.length() > 0) {
    setREPLInitialFile(filepath);
  }

  // Show colorful welcome messages
  // changeTerminalColor(replColors[7], true,global_mp_stream);
  // global_mp_stream->println("MicroPython REPL with embedded Jumperless
  // hardware control!"); global_mp_stream->println("Type normal Python code,
  // then press Enter to execute"); global_mp_stream->println("Use TAB for
  // indentation (or exactly 4 spaces)"); global_mp_stream->println("Use ↑/↓
  // arrows for command history, ←/→ arrows for cursor movement");
  // global_mp_stream->println("Navigate multiline code with ← to beginning of
  // lines"); global_mp_stream->println("Type help_jumperless() for hardware
  // control commands");

  changeTerminalColor(replColors[0], true, global_mp_stream);
  global_mp_stream->println("MicroPython initialized successfully");
  global_mp_stream->flush();
  delay(200);

  changeTerminalColor(replColors[0], true, global_mp_stream);
  global_mp_stream->println();
  changeTerminalColor(replColors[2], true, global_mp_stream);
  global_mp_stream->println("    MicroPython REPL");

  // Show commands menu
  changeTerminalColor(replColors[5], true, global_mp_stream);
  global_mp_stream->println("\n Commands:");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  quit ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("       -   Exit REPL");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->printf("  Ctrl+%c ", (char)(keyboard_interrupt_char + 64)); // Convert ASCII to Ctrl+ notation
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("     -   quit REPL or interrupt running script");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  helpl");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("       -   Show REPLhelp");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  history ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("    -   Show command history");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  save ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("       -   Save last script");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  load ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("       -   Load saved script");
  // changeTerminalColor(replColors[3], false, global_mp_stream);
  // global_mp_stream->print("  delete ");
  // changeTerminalColor(replColors[0], false, global_mp_stream);
  // global_mp_stream->println("     -   Delete saved script");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  files ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("      -   Open file manager (python_scripts)");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  new ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("        -   Create new script with eKilo editor");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  Ctrl+E ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("     -   Edit current input in main eKilo editor");
  // changeTerminalColor(replColors[3], false, global_mp_stream);
  // global_mp_stream->print("  multiline ");
  // changeTerminalColor(replColors[0], false, global_mp_stream);
  // global_mp_stream->println("  -   Toggle multiline mode");
  // changeTerminalColor(replColors[3], false, global_mp_stream);
  // global_mp_stream->print("  run ");
  // changeTerminalColor(replColors[0], false, global_mp_stream);
  // global_mp_stream->println("        -   Execute script (multiline mode)");
  changeTerminalColor(replColors[3], false, global_mp_stream);
  global_mp_stream->print("  help() ");
  changeTerminalColor(replColors[0], false, global_mp_stream);
  global_mp_stream->println("     -   Show hardware commands");

  changeTerminalColor(replColors[5], true, global_mp_stream);
  global_mp_stream->println("\nNavigation:");
  changeTerminalColor(replColors[8], false, global_mp_stream);
  global_mp_stream->println("  ↑/↓ arrows - Browse command history");
  global_mp_stream->println("  ←/→ arrows - Move cursor, edit text");
  global_mp_stream->println("  TAB        - Add 4-space indentation");
  global_mp_stream->println(
      "  Enter      - Execute (on an empty line)");
  // global_mp_stream->println("  Ctrl+Q     - Force quit REPL or interrupt running script");
  // global_mp_stream->println("  files      - Browse and manage Python scripts");
  // global_mp_stream->println("  new        - Create new scripts with eKilo editor");
  // global_mp_stream->println("  run        - Execute accumulated script "
  //                           "(multiline forced ON)");

  changeTerminalColor(replColors[5], true, global_mp_stream);
  global_mp_stream->println("\nHardware:");
  changeTerminalColor(replColors[7], false, global_mp_stream);
  global_mp_stream->println(
      "  help()           - Show Jumperless hardware commands");
  char int_char = (keyboard_interrupt_char >= 1 && keyboard_interrupt_char <= 26) ? 
                  (char)(keyboard_interrupt_char + 64) : '?';
  // global_mp_stream->printf(
  //     "  check_interrupt() - Call in tight loops to allow Ctrl+%c\n", int_char);
  // global_mp_stream->println(
  //     "  ");
  global_mp_stream->println();

  if (global_mp_stream == &Serial) {
    global_mp_stream->write(0x0E); // turn on interactive mode
    global_mp_stream->flush();
  }

  // Wait for user to press enter
  changeTerminalColor(replColors[4], true, global_mp_stream);
  global_mp_stream->print("\n\rPress enter to start REPL");
  global_mp_stream->println();
  global_mp_stream->flush();
  while (global_mp_stream->available() == 0) {
    delay(1);
  }
  global_mp_stream->read(); // consume the enter keypress

  // Start the REPL with colors
  changeTerminalColor(replColors[1], true, global_mp_stream);
  startMicroPythonREPL();

  // Blocking loop - stay in REPL until user exits
  while (mp_repl_active) {
    processMicroPythonInput(global_mp_stream);
   // mp_hal_check_interrupt();
    delayMicroseconds(10); // Small delay to prevent overwhelming
  }

  // Cleanup with colors
  changeTerminalColor(replColors[0], true, global_mp_stream);
  global_mp_stream->println("\nExiting REPL...");
  if (global_mp_stream == &Serial) {
    global_mp_stream->write(0x0F); // turn off interactive mode
    global_mp_stream->flush();
    //delay(100); // Give system time to switch modes
    // global_mp_stream->write(0x0E); // turn interactive mode back on for main menu
    // global_mp_stream->flush();
  }
  global_mp_stream->print("\033[0m");
  // stream->println("Returned to Arduino mode");
}

void processMicroPythonInput(Stream *stream) {
  if (!mp_initialized) {
    return;
  }

  // Use global variables for initial content

  // // Process any queued hardware commands
  // if (mp_command_ready) {
  //     global_mp_stream->printf("[MP] Processing hardware command: %s\n",
  //     mp_command_buffer);

  //     // Execute the hardware command through your existing system
  //     char response[512];
  //     int result = parseAndExecutePythonCommand(mp_command_buffer, response);

  //     // Make result available to Python
  //     if (result == 0) {
  //         String python_result = "globals()['_last_result'] = '";
  //         python_result += response;
  //         python_result += "'";
  //         mp_embed_exec_str(python_result.c_str());
  //     }

  //     mp_command_ready = false;
  // }

  // Handle REPL input with proper text editor functionality and history
  if (mp_repl_active) {
    static REPLEditor editor;
    static ScriptHistory history;
    static bool history_initialized = false;

    // Check for pending initial file to load (do this BEFORE first_run check)
    if (repl_has_initial_file && repl_initial_filepath.length() > 0) {
      // Reset the editor to ensure we can load the file
      editor.reset();
      
      // Initialize history if needed
      if (!history_initialized) {
        history.initFilesystem();
        history_initialized = true;
      }

      // Load the file content into the editor
      File file = FatFS.open(repl_initial_filepath.c_str(), "r");
      if (file) {
        String fileContent = file.readString();
        file.close();
        
        editor.current_input = fileContent;
        editor.cursor_pos = fileContent.length();
        editor.in_multiline_mode = (fileContent.indexOf('\n') >= 0);
        
        changeTerminalColor(replColors[5], true, global_mp_stream);
        global_mp_stream->println("Script loaded from file: " + repl_initial_filepath);
        changeTerminalColor(replColors[1], true, global_mp_stream);
        
        // Show the loaded content
        editor.drawFromCurrentLine(global_mp_stream);
      } else {
        changeTerminalColor(replColors[4], true, global_mp_stream);
        global_mp_stream->println("Failed to load file: " + repl_initial_filepath);
        changeTerminalColor(replColors[1], true, global_mp_stream);
      }
      
      // Clear the pending file
      repl_initial_filepath = "";
      repl_has_initial_file = false;
      editor.first_run = false;
      return;
    }

    if (editor.first_run) {
      // Initialize history first, before any input processing
      if (!history_initialized) {
        history.initFilesystem();
        history_initialized = true;
      }

      // Start with a fresh prompt
      changeTerminalColor(replColors[1], true, global_mp_stream);
      global_mp_stream->flush();
      editor.first_run = false;
    }

    // Check for available input
    if (global_mp_stream->available()) {
      int c = global_mp_stream->read();

      // Character processing for escape sequences

      // Handle escape sequences for arrow keys
      if (editor.escape_state == 0 && c == 27) { // ESC
        editor.escape_state = 1;
        return;
      } else if (editor.escape_state == 1 && c == 91) { // [
        editor.escape_state = 2;
        return;
      } else if (editor.escape_state == 2) {
        editor.escape_state = 0; // Reset escape state

        switch (c) {
        case 65: // Up arrow - history previous
        {
          // Only allow history navigation if:
          // 1. Current input is empty (blank prompt), OR
          // 2. We just loaded from history and no other keys were pressed
          bool allow_history = (editor.current_input.length() == 0) || 
                               (editor.just_loaded_from_history);
          
          if (allow_history) {
            String prev_cmd = history.getPreviousCommand();
            if (prev_cmd.length() > 0) {
              editor.loadFromHistory(global_mp_stream, prev_cmd);
              global_mp_stream->flush();
            }
          } else if (editor.in_multiline_mode) {
            // In multiline mode, move cursor up one line
            editor.moveCursorUp();
            editor.repositionCursorOnly(global_mp_stream);
          }
          // If neither history nor multiline, do nothing
        }
          return;

        case 66: // Down arrow - history next
        {
          // Only allow history navigation if:
          // 1. We're currently in history mode, OR
          // 2. Current input is empty (blank prompt), OR  
          // 3. We just loaded from history and no other keys were pressed
          bool allow_history = (editor.in_history_mode) ||
                               (editor.current_input.length() == 0) || 
                               (editor.just_loaded_from_history);
          
          if (allow_history) {
            String next_cmd = history.getNextCommand();
            if (next_cmd.length() > 0) {
              editor.loadFromHistory(global_mp_stream, next_cmd);
              global_mp_stream->flush();
            } else {
              // Return to original input
              editor.exitHistoryMode(global_mp_stream);
              global_mp_stream->flush();
            }
          } else if (editor.in_multiline_mode) {
            // In multiline mode, move cursor down one line
            editor.moveCursorDown();
            editor.repositionCursorOnly(global_mp_stream);
          }
          // If neither history nor multiline, do nothing
        }
          return;

        case 67: // Right arrow
          // Exit history mode when user starts navigating
          if (editor.in_history_mode) {
            editor.in_history_mode = false;
            editor.just_loaded_from_history = false; // Clear the flag
            history.resetHistoryNavigation();
          }
          
          if (editor.cursor_pos < editor.current_input.length()) {
            editor.moveCursorRight();
            editor.repositionCursorOnly(global_mp_stream);
          }
          return;

        case 68: // Left arrow
          // Exit history mode when user starts navigating
          if (editor.in_history_mode) {
            editor.in_history_mode = false;
            editor.just_loaded_from_history = false; // Clear the flag
            history.resetHistoryNavigation();
          }
          
          if (editor.cursor_pos > 0) {
            editor.moveCursorLeft();
            editor.repositionCursorOnly(global_mp_stream);
          }
          return;

        default:
          // Unknown escape sequence - just ignore it
          return;
        }
      } else if (editor.escape_state > 0) {
        // We're in the middle of an escape sequence but got an unexpected
        // character
        editor.escape_state = 0; // Reset escape state
        // Don't process this character as regular input
        return;
      }

      // Handle configured interrupt character - force quit REPL or interrupt script
      if (c == keyboard_interrupt_char) {
        char char_display = (keyboard_interrupt_char >= 1 && keyboard_interrupt_char <= 26) ? 
                           (char)(keyboard_interrupt_char + 64) : '?';
        global_mp_stream->printf("^%c\n", char_display);
        
        // Set interrupt flag for script execution
        mp_interrupt_requested = true;
        
        // Always exit REPL immediately when interrupt char is pressed during input
        // This covers the case where user is typing but not executing a script
        changeTerminalColor(replColors[4], true, global_mp_stream);
        global_mp_stream->printf("KeyboardInterrupt (Ctrl+%c)\n", char_display);
        global_mp_stream->println("Force quit - exiting REPL...");
        changeTerminalColor(replColors[1], true, global_mp_stream);
        stopMicroPythonREPL();
        editor.reset();
        return;
      }

      // Handle Enter key - check for multiline or execute
      if (c == '\r' || c == '\n') {
        global_mp_stream->println(); // Echo newline

        // Clear history flags at the start of enter processing
        editor.just_loaded_from_history = false;
        
        // Check for special commands first
        String trimmed_input = editor.current_input;
        trimmed_input.trim();

        //! Exit commands
        if (trimmed_input == "exit()" || trimmed_input == "quit()" ||
            trimmed_input == "exit" || trimmed_input == "quit") {
          stopMicroPythonREPL();
          editor.reset();
          return;
        }

        //! History commands
        if (trimmed_input == "history" || trimmed_input == "history()") {
          history.listScripts();
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Multiline mode commands
        if (trimmed_input == "multiline" || trimmed_input == "multiline()") {
          global_mp_stream->println("Multiline mode status:");
          if (editor.multiline_forced_on) {
            global_mp_stream->println("  Currently: FORCED ON");
          } else if (editor.multiline_forced_off) {
            global_mp_stream->println("  Currently: FORCED OFF");
          } else {
            global_mp_stream->println("  Currently: AUTO (default)");
          }
          global_mp_stream->println("Commands:");
          global_mp_stream->println("  multiline on   - Force multiline mode "
                                    "ON (use 'run' to execute)");
          global_mp_stream->println(
              "  multiline off  - Force multiline mode OFF");
          global_mp_stream->println(
              "  multiline auto - Return to automatic detection");
                  global_mp_stream->println(
            "  multiline edit - Use main eKilo editor for multiline input");
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        if (trimmed_input == "multiline on") {
          editor.multiline_forced_on = true;
          editor.multiline_forced_off = false;
          editor.multiline_override = true;
          global_mp_stream->println("Multiline mode: FORCED ON");
          changeTerminalColor(replColors[7], false, global_mp_stream);
          global_mp_stream->println("Enter will add new lines. Type 'run' to "
                                    "execute accumulated script.");
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        if (trimmed_input == "multiline off") {
          editor.multiline_forced_on = false;
          editor.multiline_forced_off = true;
          editor.multiline_override = true;
          global_mp_stream->println("Multiline mode: FORCED OFF");
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        if (trimmed_input == "multiline auto") {
          editor.multiline_forced_on = false;
          editor.multiline_forced_off = false;
          editor.multiline_override = false;
          global_mp_stream->println("Multiline mode: AUTO (default)");
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        if (trimmed_input == "multiline edit") {
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("Opening eKilo editor...");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          
          // Create python_scripts directory if it doesn't exist
          if (!FatFS.exists("/python_scripts")) {
            FatFS.mkdir("/python_scripts");
          }
          
          // Save current input to temporary file
          String tempFile = "/python_scripts/_temp_repl_edit.py";
          File file = FatFS.open(tempFile, "w");
          if (file) {
            if (editor.current_input.length() > 0) {
              file.print(editor.current_input);
            } else {
              // Start with a helpful comment if no input
              file.println("# Edit your Python script here");
              file.println("# Press Ctrl+S to save and return to REPL");
              file.println("# Press Ctrl+P to save and execute immediately");
            }
            file.close();
          }
          
          // Launch main eKilo editor with temporary file
          String savedContent = launchEkiloREPL(tempFile.c_str());
          
          // Restore interactive mode after returning from eKilo
          if (global_mp_stream == &Serial) {
            global_mp_stream->write(0x0E); // turn on interactive mode
            global_mp_stream->flush();
          }
          
          // Handle the return from eKilo
          if (savedContent.length() > 0) {
            // Check if this was a Ctrl+P (save and execute) request
            if (savedContent.startsWith("[LAUNCH_REPL]")) {
              // Remove the marker and execute the content
              String contentToExecute = savedContent.substring(13); // Remove "[LAUNCH_REPL]"
              if (contentToExecute.length() > 0) {
                changeTerminalColor(replColors[2], true, global_mp_stream);
                global_mp_stream->println("Executing script from eKilo:");
                
                // Add to history before execution
                history.addToHistory(contentToExecute);
                
                // Execute the script
                mp_embed_exec_str(contentToExecute.c_str());
              }
            } else {
              // Regular save - load content into REPL editor
              editor.current_input = savedContent;
              editor.cursor_pos = savedContent.length();
              editor.in_multiline_mode = (savedContent.indexOf('\n') >= 0);
              changeTerminalColor(replColors[5], true, global_mp_stream);
              global_mp_stream->println("Script loaded into REPL editor");
              // Don't reset - show the loaded content
              editor.drawFromCurrentLine(global_mp_stream);
              return;
            }
          } else {
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Returned from eKilo editor");
          }
          
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }



        //! New command - create new script with eKilo editor
        if (trimmed_input == "new" || trimmed_input == "new()") {
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("Opening eKilo editor...");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          
          // Launch eKilo editor in REPL mode
          String savedContent = launchEkiloREPL(nullptr);
          
          // Restore interactive mode after returning from eKilo
          if (global_mp_stream == &Serial) {
            global_mp_stream->write(0x0E); // turn on interactive mode
            global_mp_stream->flush();
          }
          
          // If content was saved, load it into the editor
          if (savedContent.length() > 0) {
            editor.current_input = savedContent;
            editor.cursor_pos = savedContent.length();
            editor.in_multiline_mode = (savedContent.indexOf('\n') >= 0);
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Script content loaded into REPL");
            // Don't reset - show the loaded content
            editor.redrawAndPosition(global_mp_stream);
            return;
          } else {
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Returned from eKilo editor");
            
            editor.reset();
            changeTerminalColor(replColors[1], true, global_mp_stream);
            global_mp_stream->print(">>> ");
            global_mp_stream->flush();
            return;
          }
        }

        //! Edit command - launch main eKilo editor for multiline editing
        if (trimmed_input == "edit" || trimmed_input == "edit()") {
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("Opening eKilo editor...");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          
          // Create python_scripts directory if it doesn't exist
          if (!FatFS.exists("/python_scripts")) {
            FatFS.mkdir("/python_scripts");
          }
          
          // Save current input to temporary file
          String tempFile = "/python_scripts/_temp_repl_edit.py";
          File file = FatFS.open(tempFile, "w");
          if (file) {
            if (editor.current_input.length() > 0) {
              file.print(editor.current_input);
            } else {
              // Start with a helpful comment if no input
              file.println("# Edit your Python script here");
              file.println("# Press Ctrl+S to save and return to REPL");
              file.println("# Press Ctrl+P to save and execute immediately");
            }
            file.close();
          }
          
          // Launch main eKilo editor with temporary file
          String savedContent = launchEkiloREPL(tempFile.c_str());
          
          // Restore interactive mode after returning from eKilo
          if (global_mp_stream == &Serial) {
            global_mp_stream->write(0x0E); // turn on interactive mode
            global_mp_stream->flush();
          }
          
          // Handle the return from eKilo
          if (savedContent.length() > 0) {
            // Check if this was a Ctrl+P (save and execute) request
            if (savedContent.startsWith("[LAUNCH_REPL]")) {
              // Remove the marker and execute the content
              String contentToExecute = savedContent.substring(13); // Remove "[LAUNCH_REPL]"
              if (contentToExecute.length() > 0) {
                changeTerminalColor(replColors[2], true, global_mp_stream);
                global_mp_stream->println("Executing script from eKilo:");
                
                // Add to history before execution
                history.addToHistory(contentToExecute);
                
                // Execute the script
                mp_embed_exec_str(contentToExecute.c_str());
              }
            } else {
              // Regular save - load content into REPL editor
              editor.current_input = savedContent;
              editor.cursor_pos = savedContent.length();
              editor.in_multiline_mode = (savedContent.indexOf('\n') >= 0);
              changeTerminalColor(replColors[5], true, global_mp_stream);
              global_mp_stream->println("Script loaded into REPL editor");
              // Don't reset - show the loaded content
              editor.drawFromCurrentLine(global_mp_stream);
              return;
            }
          } else {
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Returned from eKilo editor");
          }
          
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Help commands
        if (trimmed_input == "helpl" || trimmed_input == "helpl()") {
          // Show REPL help
          changeTerminalColor(replColors[7], true, global_mp_stream);
          global_mp_stream->println("\n   MicroPython REPL Help");
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("\nCommands:");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  quit/exit ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("    -   Exit REPL");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  history ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println(
              "      -   Show command history & saved scripts");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  save [name] ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println(
              "  -   Save last script (auto: script_N.py)");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  load <name> ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("  -   Load script by name");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  delete <name>");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println(" -   Delete saved script");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  files ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("       -   Open file manager (python_scripts)");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  new ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("         -   Create new script with eKilo editor");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  edit ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println("        -   Launch main eKilo editor");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  multiline ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println(
              "    -   Toggle multiline mode (on/off/auto/edit)");
          changeTerminalColor(replColors[3], false, global_mp_stream);
          global_mp_stream->print("  run ");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          global_mp_stream->println(
              "          -   Execute script (when multiline forced ON)");

          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("\nNavigation:");
          changeTerminalColor(replColors[8], false, global_mp_stream);
          global_mp_stream->println("  ↑/↓ arrows - Browse command history");
          global_mp_stream->println("  ←/→ arrows - Move cursor, edit text");
          global_mp_stream->println("  TAB        - Add 4-space indentation");
          global_mp_stream->println(
              "  Enter      - Execute (empty line in multiline to finish)");
          char int_char = (keyboard_interrupt_char >= 1 && keyboard_interrupt_char <= 26) ? 
                          (char)(keyboard_interrupt_char + 64) : '?';
          global_mp_stream->printf("  Ctrl+%c     - Force quit REPL or interrupt running script\n", int_char);
          global_mp_stream->println("  files      - Browse and manage Python scripts");
          global_mp_stream->println("  new        - Create new scripts with eKilo editor");
          global_mp_stream->println("  edit       - Launch main eKilo editor for multiline scripts");
          global_mp_stream->println("  run        - Execute accumulated script "
                                    "(multiline forced ON)");
          changeTerminalColor(replColors[8], false, global_mp_stream);
          global_mp_stream->println("  multiline edit - Launch main eKilo editor");

          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("\nHardware:");
          changeTerminalColor(replColors[7], false, global_mp_stream);
          global_mp_stream->println(
              "  help()  - Show Jumperless hardware commands");
          global_mp_stream->println(
              "  ");
          global_mp_stream->println();

          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Save command - save the last executed script from history
        if (trimmed_input.startsWith("save ") || trimmed_input == "save" ||
            trimmed_input == "save()") {
          // Get the most recent executed script from history
          String last_script = history.getLastExecutedCommand();
          if (last_script.length() > 0) {
            String filename = "";
            if (trimmed_input.startsWith("save ")) {
              filename = trimmed_input.substring(5);
              filename.trim();
            }
            if (history.saveScript(last_script, filename)) {
              global_mp_stream->println("Script saved to filesystem");
            } else {
              global_mp_stream->println("Failed to save script");
            }
          } else {
            global_mp_stream->println("No previous script to save");
          }
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Load command - load a script from filesystem
        if (trimmed_input.startsWith("load ") || trimmed_input == "load") {
          if (trimmed_input == "load") {
            // Show available scripts when no filename provided
            global_mp_stream->println("Available scripts:");
            history.listScripts();
            String recent_script = history.getLastSavedScript();
            if (recent_script.length() > 0) {
              global_mp_stream->println("Most recent: " + recent_script);
              global_mp_stream->println("Try: load " + recent_script);
            }
            global_mp_stream->println(
                "Usage: load <number> or load <filename>");
          } else {
            String arg = trimmed_input.substring(5);
            arg.trim();
            if (arg.length() > 0) {
              String filename = "";

              // Check if argument is a number
              bool is_number = true;
              for (int i = 0; i < arg.length(); i++) {
                if (!isdigit(arg.charAt(i))) {
                  is_number = false;
                  break;
                }
              }

              if (is_number) {
                // Handle numeric input
                int script_number = arg.toInt();
                if (script_number >= 1 &&
                    script_number <= history.getNumberedScriptsCount()) {
                  filename = history.getNumberedScript(
                      script_number - 1); // Convert 1-based to 0-based
                  global_mp_stream->println("Loading script " +
                                            String(script_number) + ": " +
                                            filename);
                } else {
                  global_mp_stream->println(
                      "Invalid script number. Use 'history' to see available "
                      "scripts.");
                  editor.reset();
                  changeTerminalColor(replColors[1], true, global_mp_stream);
                  global_mp_stream->print(">>> ");
                  global_mp_stream->flush();
                  return;
                }
              } else {
                // Handle filename input
                filename = arg;
              }

              if (filename.length() > 0) {
                String loaded_script = history.loadScript(filename);
                if (loaded_script.length() > 0) {
                  // Load the script into the editor
                  editor.current_input = loaded_script;
                  editor.cursor_pos = loaded_script.length();
                  editor.in_multiline_mode = (loaded_script.indexOf('\n') >= 0);
                  editor.redrawAndPosition(global_mp_stream);
                  return; // Stay in editing mode
                }
              }
            } else {
              global_mp_stream->println(
                  "Usage: load <number> or load <filename>");
            }
          }
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Delete command - delete a script from filesystem
        if (trimmed_input.startsWith("delete ") ||
            trimmed_input.startsWith("del ")) {
          int start_pos = trimmed_input.startsWith("delete ") ? 7 : 4;
          String filename = trimmed_input.substring(start_pos);
          filename.trim();
          if (filename.length() > 0) {
            history.deleteScript(filename);
          } else {
            global_mp_stream->println("Usage: delete filename");
          }
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
          return;
        }

        //! Files command - launch file manager in python_scripts directory
        if (trimmed_input == "files" || trimmed_input == "files()" ||
            trimmed_input == "filemanager" || trimmed_input == "filemanager()") {
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("Opening file manager...");
          changeTerminalColor(replColors[0], false, global_mp_stream);
          
          // Launch file manager in REPL mode
          String savedContent = filesystemAppPythonScriptsREPL();
          
          // Restore interactive mode after returning from file manager
          if (global_mp_stream == &Serial) {
            global_mp_stream->write(0x0E); // turn on interactive mode
            global_mp_stream->flush();
          }
          
          // If content was saved, load it into the editor
          if (savedContent.length() > 0) {
            editor.current_input = savedContent;
            editor.cursor_pos = savedContent.length();
            editor.in_multiline_mode = (savedContent.indexOf('\n') >= 0);
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("File content loaded into REPL");
            // Don't reset - show the loaded content
            editor.redrawAndPosition(global_mp_stream);
            return;
          } else {
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Returned from file manager");
            editor.reset();
            changeTerminalColor(replColors[1], true, global_mp_stream);
            global_mp_stream->print(">>> ");
            global_mp_stream->flush();
            return;
          }
        }

        //! Special handling for forced multiline mode
        if (editor.multiline_forced_on) {
          // Check if the user typed 'run' (as the only content or last line)
          if (trimmed_input == "run") {
            // Only "run" was typed - no script to execute
            global_mp_stream->println("No script to execute.");
            editor.reset();
            changeTerminalColor(replColors[1], true, global_mp_stream);
            global_mp_stream->print(">>> ");
            global_mp_stream->flush();
            return;
          } else if (trimmed_input.endsWith("\nrun") &&
                     trimmed_input.length() > 4) {
            // Script followed by 'run' command
            String script_to_execute =
                trimmed_input.substring(0, trimmed_input.length() - 4);
            script_to_execute.trim();

            if (script_to_execute.length() > 0) {
              changeTerminalColor(replColors[2], true, global_mp_stream);
              global_mp_stream->println("Executing accumulated script:");

              // Execute the user's current input (edited or original)
              // No longer override with history command - user edits should be respected

              // Add to history before execution
              history.addToHistory(script_to_execute);

              // Reset history navigation now that we're executing
              history.resetHistoryNavigation();

              // Reset local nodefile copy for multiline scripts (start fresh each time)
              jl_init_micropython_local_copy();

              // Execute the complete script
              mp_embed_exec_str(script_to_execute.c_str());
            }

            // Reset and show new prompt
            editor.reset();
            changeTerminalColor(replColors[1], true, global_mp_stream);
            global_mp_stream->print(">>> ");
            global_mp_stream->flush();
            return;
          }
          // If not 'run', force multiline continuation (never execute
          // individual lines)
        }

        // Check if this is an empty line in multiline mode (escape mechanism)
        bool force_execution = false;
        bool force_multiline = false;
        
        // Check the current line content to determine if we should execute
        int line_start = editor.current_input.lastIndexOf('\n', editor.cursor_pos - 1);
        line_start = (line_start >= 0) ? line_start + 1 : 0;
        String current_line = editor.current_input.substring(line_start, editor.cursor_pos);
        current_line.trim();
        
        // Don't force multiline mode when loading from history - let normal detection work
        if (editor.in_multiline_mode && !editor.multiline_forced_on) {
          // Only allow empty line escape in AUTO mode, not when forced ON
          if (current_line.length() == 0) {
            force_execution = true; // Empty line in multiline mode
          }
        }

        // Check if MicroPython needs more input (multiline detection)
        // Use current input WITHOUT adding newline first
        bool needs_more_input = false;
        if (force_multiline) {
          // Force multiline mode (e.g., when loading from history with content)
          needs_more_input = true;
        } else if (editor.multiline_forced_on) {
          // In forced ON mode, ALWAYS continue - never execute until 'run' is
          // typed
          needs_more_input = true;
        } else if (editor.current_input.length() > 0 && !force_execution) {
          if (editor.multiline_forced_off) {
            // In forced OFF mode, NEVER continue (always execute on Enter)
            needs_more_input = false;
          } else {
            // Use automatic detection (default behavior)
            String input_for_check = editor.current_input;
            needs_more_input =
                mp_repl_continue_with_input(input_for_check.c_str());
          }
        }

        if (needs_more_input && !force_execution) {
          editor.in_multiline_mode = true;

          // Add newline at cursor position since we need more input
          editor.current_input =
              editor.current_input.substring(0, editor.cursor_pos) + "\n" +
              editor.current_input.substring(editor.cursor_pos);
          editor.cursor_pos++;

          // Smart auto-indent: maintain or increase indentation level
          // Get the line we just finished (before the newline we just added)
          String lines = editor.current_input;
          int last_newline = lines.lastIndexOf(
              '\n',
              editor.cursor_pos - 2); // -2 to skip the newline we just added
          String last_line = "";
          if (last_newline >= 0) {
            last_line = lines.substring(
                last_newline + 1,
                editor.cursor_pos - 1); // exclude the newline we just added
          } else {
            last_line = lines.substring(
                0, editor.cursor_pos - 1); // first line, exclude newline
          }

          // Calculate current indentation level of the previous line
          int current_indent = 0;
          for (int i = 0; i < last_line.length(); i++) {
            if (last_line.charAt(i) == ' ') {
              current_indent++;
            } else {
              break;
            }
          }

          String trimmed_last_line = last_line;
          trimmed_last_line.trim();

          String indent_spaces = "";
          if (trimmed_last_line.endsWith(":")) {
            // Increase indentation level by 4 spaces
            for (int i = 0; i < current_indent + 4; i++) {
              indent_spaces += " ";
            }
          } else if (current_indent > 0) {
            // Maintain current indentation level
            for (int i = 0; i < current_indent; i++) {
              indent_spaces += " ";
            }
          }

          // Insert the indentation at cursor position
          editor.current_input =
              editor.current_input.substring(0, editor.cursor_pos) +
              indent_spaces + editor.current_input.substring(editor.cursor_pos);
          editor.cursor_pos += indent_spaces.length();

          // Show the appropriate prompt
          changeTerminalColor(replColors[1], true, global_mp_stream);
          if (editor.multiline_forced_on) {
            global_mp_stream->print("... "); // Standard multiline prompt
            // Show reminder on first few lines
            static int line_count = 0;
            line_count++;
            if (line_count < 1) { // Show on first multiline prompt
              changeTerminalColor(replColors[12], false, global_mp_stream);
              global_mp_stream->print(" (type 'run' to execute)");
              global_mp_stream->println();
              changeTerminalColor(replColors[1], true, global_mp_stream);
              global_mp_stream->print("... ");
            }
          } else {
            global_mp_stream->print("... ");
          }

          if (indent_spaces.length() > 0) {
            changeTerminalColor(replColors[3], false, global_mp_stream);
            global_mp_stream->print(indent_spaces); // Show the indentation
          }
          global_mp_stream->flush();
        } else {
          // Execute the complete statement (or force execution)
          if (editor.current_input.length() > 0) {
            changeTerminalColor(replColors[2], true, global_mp_stream);

            // Clean up the input (remove trailing newlines)
            String clean_input = editor.current_input;
            while (clean_input.endsWith("\n")) {
              clean_input = clean_input.substring(0, clean_input.length() - 1);
            }

            if (clean_input.length() > 0) {
              // Execute the user's current input (edited or original)
              // No longer override with history command - user edits should be respected

              // Add to history before execution
              history.addToHistory(clean_input);

              // Reset history navigation now that we're executing
              history.resetHistoryNavigation();

              // Let MicroPython handle the complete statement
              mp_embed_exec_str(clean_input.c_str());
            }

            changeTerminalColor(replColors[1], true, global_mp_stream);
          }

          // Reset and show new prompt
          editor.reset();
          changeTerminalColor(replColors[1], true, global_mp_stream);
          global_mp_stream->print(">>> ");
          global_mp_stream->flush();
        }

      } else if (c == '\b' || c == 127) { // Backspace
        // Exit history mode when user starts editing
        if (editor.in_history_mode) {
          editor.in_history_mode = false;
          editor.just_loaded_from_history = false; // Clear the flag when user starts editing
          history.resetHistoryNavigation();
        }
        
        if (editor.cursor_pos > 0) {
          char char_to_delete =
              editor.current_input.charAt(editor.cursor_pos - 1);

          if (char_to_delete == '\n') {
            // Use the proper backspace over newline function
            editor.backspaceOverNewline(global_mp_stream);
          } else {
            // Check if we're backspacing over a complete tab (4 spaces)
            bool is_tab_backspace = false;
            if (editor.cursor_pos >= 4) {
              String potential_tab = editor.current_input.substring(
                  editor.cursor_pos - 4, editor.cursor_pos);
              if (potential_tab == "    ") {
                // Check if these 4 spaces are at the start of a line or after
                // other whitespace
                int line_start = editor.current_input.lastIndexOf(
                    '\n', editor.cursor_pos - 1);
                line_start = (line_start >= 0) ? line_start + 1 : 0;
                String line_before_cursor = editor.current_input.substring(
                    line_start, editor.cursor_pos - 4);

                // If line before cursor is all whitespace, treat as tab
                bool all_whitespace = true;
                for (int i = 0; i < line_before_cursor.length(); i++) {
                  if (line_before_cursor.charAt(i) != ' ') {
                    all_whitespace = false;
                    break;
                  }
                }

                if (all_whitespace) {
                  is_tab_backspace = true;
                  // Remove 4 spaces at once
                  editor.current_input.remove(editor.cursor_pos - 4, 4);
                  editor.cursor_pos -= 4;
                  editor.redrawAndPosition(global_mp_stream);
                }
              }
            }

            if (!is_tab_backspace) {
              // Normal single character backspace
              editor.current_input.remove(editor.cursor_pos - 1, 1);
              editor.cursor_pos--;
              editor.redrawAndPosition(global_mp_stream);
            }
          }
        }
      } else if (c == 5) { // Ctrl+E - Edit current input in main eKilo editor
        changeTerminalColor(replColors[5], true, global_mp_stream);
        global_mp_stream->println("\n[Opening eKilo editor...]");
        changeTerminalColor(replColors[0], false, global_mp_stream);
        
        // Create python_scripts directory if it doesn't exist
        if (!FatFS.exists("/python_scripts")) {
          FatFS.mkdir("/python_scripts");
        }
        
        // Save current input to temporary file
        String tempFile = "/python_scripts/_temp_repl_edit.py";
        File file = FatFS.open(tempFile, "w");
        if (file) {
          if (editor.current_input.length() > 0) {
            file.print(editor.current_input);
          } else {
            // Start with a helpful comment if no input
            file.println("# Edit your Python script here");
            file.println("# Press Ctrl+S to save and return to REPL");
            file.println("# Press Ctrl+P to save and execute immediately");
          }
          file.close();
        }
        
        // Launch main eKilo editor with temporary file
        String savedContent = launchEkiloREPL(tempFile.c_str());
        
        // Restore interactive mode after returning from eKilo
        if (global_mp_stream == &Serial) {
          global_mp_stream->write(0x0E); // turn on interactive mode
          global_mp_stream->flush();
        }
        
        // Handle the return from eKilo
        if (savedContent.length() > 0) {
          // Check if this was a Ctrl+P (save and execute) request
          if (savedContent.startsWith("[LAUNCH_REPL]")) {
            // Remove the marker and execute the content
            String contentToExecute = savedContent.substring(13); // Remove "[LAUNCH_REPL]"
            if (contentToExecute.length() > 0) {
              changeTerminalColor(replColors[2], true, global_mp_stream);
              global_mp_stream->println("Executing script from eKilo:");
              
              // Add to history before execution
              history.addToHistory(contentToExecute);
              
              // Execute the script
              mp_embed_exec_str(contentToExecute.c_str());
            }
          } else {
            // Regular save - load content into REPL editor
            editor.current_input = savedContent;
            editor.cursor_pos = savedContent.length();
            editor.in_multiline_mode = (savedContent.indexOf('\n') >= 0);
            changeTerminalColor(replColors[5], true, global_mp_stream);
            global_mp_stream->println("Script loaded into REPL editor");
            // Don't reset - show the loaded content
            editor.drawFromCurrentLine(global_mp_stream);
            return;
          }
        } else {
          changeTerminalColor(replColors[5], true, global_mp_stream);
          global_mp_stream->println("Returned from eKilo editor");
        }
        
        editor.reset();
        changeTerminalColor(replColors[1], true, global_mp_stream);
        global_mp_stream->print(">>> ");
        global_mp_stream->flush();
        return;
        
      } else if (c == '\t') { // TAB character
        // Exit history mode when user starts editing
        if (editor.in_history_mode) {
          editor.in_history_mode = false;
          editor.just_loaded_from_history = false; // Clear the flag when user starts editing
          history.resetHistoryNavigation();
        }
        
        // Convert TAB to 4 spaces at cursor position
        String spaces = "    ";
        editor.current_input =
            editor.current_input.substring(0, editor.cursor_pos) + spaces +
            editor.current_input.substring(editor.cursor_pos);
        editor.cursor_pos += 4;
        
        // Always redraw the entire input buffer to keep everything synchronized
        editor.redrawAndPosition(global_mp_stream);
      } else if (c >= 32 && c <= 126) { // Printable characters
        // Exit history mode when user starts editing
        if (editor.in_history_mode) {
          editor.in_history_mode = false;
          editor.just_loaded_from_history = false; // Clear the flag when user starts editing
          history.resetHistoryNavigation();
        }

        // Insert character at cursor position
        editor.current_input =
            editor.current_input.substring(0, editor.cursor_pos) + (char)c +
            editor.current_input.substring(editor.cursor_pos);
        editor.cursor_pos++;

        // Always redraw the entire input buffer to keep everything synchronized
        editor.redrawAndPosition(global_mp_stream);
      } else {
        mp_repl_continue_with_input(editor.current_input.c_str());
      }
      //Serial.println("looping");
      // All other characters are ignored
    }
  }
}

// Helper function to add complete Jumperless hardware module
void addNodeConstantsToGlobalNamespace(void) {
  if (!mp_initialized) {
    return;
  }
  
  // This function is now redundant since addJumperlessPythonFunctions() 
  // does 'from jumperless import *' which imports everything.
  // Keeping this for backward compatibility, but just calls the main function.
  addJumperlessPythonFunctions();
}

void testGlobalImports(void) {
  if (!mp_initialized) {
    return;
  }
  
  mp_embed_exec_str(
      "print('Testing global imports...')\n"
      "print('oled_connect available:', 'oled_connect' in globals())\n"
      "print('connect available:', 'connect' in globals())\n"
      "print('TOP_RAIL available:', 'TOP_RAIL' in globals())\n"
      "print('D13 available:', 'D13' in globals())\n"
      "print('jumperless module available:', 'jumperless' in globals())\n");
}

void addJumperlessPythonFunctions(void) {
  if (!mp_initialized) {
    return;
  }
  
  // Only load once to avoid redundant imports
  if (jumperless_globals_loaded) {
    if (global_mp_stream) {
      //global_mp_stream->println("[DEBUG] Jumperless globals already loaded, skipping");
    }
    return;
  }
  
  // Debug: print that this function is being called
  if (global_mp_stream) {
    //global_mp_stream->println("[DEBUG] Loading jumperless globals for first time");
  }

  // Import jumperless module and add ALL functions and constants to global namespace
  mp_embed_exec_str(
      "try:\n"
      "    print('Attempting to import jumperless module...')\n"
      "    import jumperless\n"
      "    print('Native jumperless module available')\n"
      "    funcs = [attr for attr in dir(jumperless) if not attr.startswith('_')]\n"
      "    #print('Available functions: ' + str(funcs))\n"
      "    \n"
      "    # Import all jumperless functions into global namespace\n"
      "    # This eliminates the need for jumperless. prefix\n"
      "    #print('Importing all functions globally...')\n"
      "    from jumperless import *\n"
      "    \n"
      "    # Also keep jumperless module available for explicit access if needed\n"
      "    globals()['jumperless'] = jumperless\n"
      "    \n"
      "    # Add a helper function for checking interrupts in tight loops\n"
      "    def check_interrupt():\n"
      "        '''Call this function in tight loops to allow keyboard interrupt.'''\n"
      "        import time\n"
      "        time.sleep_ms(1)  # Minimal delay that triggers interrupt checking\n"
      "    \n"
      "    # Make it available globally\n"
      "    globals()['check_interrupt'] = check_interrupt\n"
      "    \n"
      "    # Test that functions are actually available\n"
      "    #available_functions = [name for name in globals() if not name.startswith('_') and callable(globals()[name])]\n"
      "    #print(' Available global functions: ' + str(len(available_functions)))\n"
      "    #if 'oled_connect' in globals():\n"
      "    #    print(' oled_connect() is available globally')\n"
      "    #else:\n"
      "    #    print(' oled_connect() not found in globals')\n"
      "    \n"
      "    #print('All jumperless functions and constants available globally')\n"
      "    #print('You can now use: connect(), dac_set(), TOP_RAIL, D13, etc.')\n"
      "    #print('For tight loops, use check_interrupt() to allow interrupts')\n"
      "    \n"
      "except ImportError as e:\n"
      "    print('△ Native jumperless module not available: ' + str(e))\n"
      "except Exception as e:\n"
      "    print('△ Error setting up globals: ' + str(e))\n"
      "    import traceback\n"
      "    traceback.print_exc()\n");
  
  // Mark as successfully loaded
  jumperless_globals_loaded = true;
}

void addMicroPythonModules(bool time, bool machine, bool os, bool math, bool gc) {
  if (!mp_initialized) {
    return;
  }
  
  if (time) {
    mp_embed_exec_str("import time\n");
   // mp_embed_exec_str("print('Time module imported successfully')\n");
  }
  // if (machine) {
  //   mp_embed_exec_str("import machine\n");
  //   mp_embed_exec_str("print('Machine module imported successfully')\n");
  // }
  if (os) {
    mp_embed_exec_str("import os\n");
  //  mp_embed_exec_str("print('OS module imported successfully')\n");
  }
  if (math) {
    mp_embed_exec_str("import math\n");
   // mp_embed_exec_str("print('Math module imported successfully')\n");
  }
  if (gc) {
    mp_embed_exec_str("import gc\n");
   // mp_embed_exec_str("print('GC module imported successfully')\n");
  }
}

const char *test_code = R"""(
try:
    import jumperless
            print("☺ Native jumperless module imported successfully")
    
    # Test that functions exist
    if hasattr(jumperless, 'dac_set') and hasattr(jumperless, 'adc_get'):
        print("☺ Core DAC/ADC functions found")
    else:
        print("☹ Core DAC/ADC functions missing")
        
    if hasattr(jumperless, 'nodes_connect') and hasattr(jumperless, 'gpio_set'):
        print("☺ Node and GPIO functions found")
    else:
        print("☹ Node and GPIO functions missing")
        
    if hasattr(jumperless, 'oled_print') and hasattr(jumperless, 'ina_get_current'):
                print("☺ OLED and INA functions found")
    else:
        print("☹ OLED and INA functions missing")
    
    print("☺ Native Jumperless module test completed successfully")
    
except ImportError as e:
    print("☹ Failed to import native jumperless module:", str(e))
except Exception as e:
    print("☹ Error testing native jumperless module:", str(e))
)""";
// Simple execution function for one-off commands
bool executePythonSimple(const char *code, char *response,
                         size_t response_size) {
  if (!mp_initialized) {
    if (response)
      strncpy(response, "ERROR: MicroPython not initialized",
              response_size - 1);
    return false;
  }

  // Clear response buffer
  memset(mp_response_buffer, 0, sizeof(mp_response_buffer));

  // Execute and capture any output
  bool success = executePythonCodeProper(code);

  // Copy response if buffer provided
  if (response && response_size > 0) {
    if (success) {
      global_mp_stream->println("[MP] Executing native module test...");
      mp_embed_exec_str(test_code);
      global_mp_stream->println("[MP] Native module test complete");
    }
  }

  return success;
}

// Status functions
bool isMicroPythonInitialized(void) { return mp_initialized; }

void printMicroPythonStatus(void) {
  global_mp_stream->println("\n=== MicroPython Status ===");
  global_mp_stream->printf("Initialized: %s\n", mp_initialized ? "Yes" : "No");
  global_mp_stream->printf("REPL Active: %s\n", mp_repl_active ? "Yes" : "No");
  global_mp_stream->printf("Heap Size: %d bytes\n", sizeof(mp_heap));

  if (mp_initialized) {
    // Get memory info
    mp_embed_exec_str(
        "import gc; print(f'Free: {gc.mem_free()}, Used: {gc.mem_alloc()}')");
  }
  global_mp_stream->println("=========================\n");
}

// Test function to verify the native Jumperless module is working
void testJumperlessNativeModule(void) {
  if (!mp_initialized) {
    global_mp_stream->println(
        "[MP] Error: MicroPython not initialized for module test");
    return;
  }

  global_mp_stream->println("[MP] Testing native Jumperless module...");

  // Simple test to verify the module can be imported and functions are
  // accessible

  global_mp_stream->println("[MP] Executing native module test...");
  mp_embed_exec_str(test_code);
  global_mp_stream->println("[MP] Native module test complete");
}

// Test function to verify stream redirection is working
void testStreamRedirection(Stream *newStream) {
  if (!mp_initialized) {
    global_mp_stream->println(
        "[MP] Error: MicroPython not initialized for stream test");
    return;
  }

  Stream *oldStream = global_mp_stream;

  // Test output to original stream
  global_mp_stream->println("[MP] Testing stream redirection...");
  global_mp_stream->println("[MP] This should appear on the original stream");
  mp_embed_exec_str("print('Python output to original stream')");

  // Change to new stream using proper setter
  setGlobalStream(newStream);
  newStream->println(
      "[MP] Stream changed - this should appear on the new stream");
  mp_embed_exec_str("print('Python output to new stream')");

  // Change back to original stream using proper setter
  setGlobalStream(oldStream);
  oldStream->println("[MP] Stream changed back - this should appear on the "
                     "original stream again");
  mp_embed_exec_str("print('Python output back to original stream')");

  global_mp_stream->println("[MP] Stream redirection test complete");
}

// ScriptHistory method implementations
void ScriptHistory::initFilesystem() {
  // Note: FatFS should already be initialized by main application
  // Do not call FatFS.begin() here as it can interfere with config loading
  
  // Create scripts directory if it doesn't exist
  if (!FatFS.exists(scripts_dir)) {
    if (!FatFS.mkdir(scripts_dir)) {
      global_mp_stream->println("Failed to create scripts directory");
      return;
    }
  }

  // Load existing history from file
  loadHistoryFromFile();

  // Find the next available script number
  findNextScriptNumber();
}

void ScriptHistory::addToHistory(const String &script) {
  if (script.length() == 0)
    return;

  // Check if this command already exists anywhere in history
  for (int i = 0; i < history_count; i++) {
    if (history[i] == script) {
      // Move this command to the end (most recent)
      String temp = history[i];
      for (int j = i; j < history_count - 1; j++) {
        history[j] = history[j + 1];
      }
      history[history_count - 1] = temp;
      current_history_index = -1; // Reset navigation
      return;
    }
  }

  // Shift history if full
  if (history_count >= MAX_HISTORY) {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      history[i] = history[i + 1];
    }
    history_count = MAX_HISTORY - 1;
  }

  history[history_count++] = script;
  current_history_index = -1; // Reset navigation
  saveHistoryToFile();
}

String ScriptHistory::getPreviousCommand() {
  if (history_count == 0)
    return "";

  if (current_history_index == -1) {
    current_history_index = history_count - 1;
  } else if (current_history_index > 0) {
    current_history_index--;
  }
  // If already at the oldest command, stay there

  return history[current_history_index];
}

String ScriptHistory::getNextCommand() {
  if (history_count == 0 || current_history_index == -1)
    return "";

  if (current_history_index < history_count - 1) {
    current_history_index++;
    return history[current_history_index];
  } else {
    // Moving forward past the newest command returns to original input
    current_history_index = -1;
    return ""; // Return to current input
  }
}

String ScriptHistory::getCurrentHistoryCommand() {
  if (current_history_index >= 0 && current_history_index < history_count) {
    return history[current_history_index];
  }
  return "";
}

void ScriptHistory::resetHistoryNavigation() { 
  current_history_index = -1; 
}

void ScriptHistory::clearHistory() {
  history_count = 0;
  current_history_index = -1;
  saveHistoryToFile();
}

String ScriptHistory::getLastExecutedCommand() {
  if (history_count == 0)
    return "";
  return history[history_count - 1]; // Return most recent without affecting navigation
}

String ScriptHistory::getLastSavedScript() { 
  return last_saved_script; 
}

int ScriptHistory::getNextScriptNumber() { 
  return next_script_number; 
}

int ScriptHistory::getNumberedScriptsCount() { 
  return numbered_scripts_count; 
}

String ScriptHistory::getNumberedScript(int index) {
  if (index >= 0 && index < numbered_scripts_count) {
    return numbered_scripts[index];
  }
  return "";
}

bool ScriptHistory::saveScript(const String &script, const String &filename) {
  String fname = filename;
  if (fname.length() == 0) {
    // Generate sequential filename
    fname = "script_" + String(next_script_number);

    // Make sure this filename doesn't already exist, increment if needed
    String fullPath = scripts_dir + "/" + fname + ".py";
    while (FatFS.exists(fullPath)) {
      next_script_number++;
      fname = "script_" + String(next_script_number);
      fullPath = scripts_dir + "/" + fname + ".py";
    }
    next_script_number++; // Increment for next time
  }
  if (!fname.endsWith(".py")) {
    fname += ".py";
  }

  String fullPath = scripts_dir + "/" + fname;
  File file = FatFS.open(fullPath, "w");
  if (!file) {
    global_mp_stream->println("Failed to create script file: " + fullPath);
    return false;
  }

  file.print(script);
  file.close();

  last_saved_script = fname; // Store for easy reference

  // Add to saved scripts list (avoid duplicates)
  bool already_exists = false;
  for (int i = 0; i < saved_scripts_count; i++) {
    if (saved_scripts[i] == fname) {
      already_exists = true;
      break;
    }
  }

  if (!already_exists && saved_scripts_count < 10) {
    saved_scripts[saved_scripts_count++] = fname;
  }

  global_mp_stream->println("Script saved as: " + fullPath);
  addToHistory(script); // Also add to memory history
  return true;
}

String ScriptHistory::loadScript(const String &filename) {
  String fullPath = scripts_dir + "/" + filename;
  if (!filename.endsWith(".py")) {
    fullPath += ".py";
  }

  if (!FatFS.exists(fullPath)) {
    global_mp_stream->println("Script not found: " + fullPath);
    return "";
  }

  File file = FatFS.open(fullPath, "r");
  if (!file) {
    global_mp_stream->println("Failed to open script file: " + fullPath);
    return "";
  }

  String content = file.readString();
  file.close();

  global_mp_stream->println("Script loaded: " + fullPath);
  return content;
}

bool ScriptHistory::deleteScript(const String &filename) {
  String fullPath = scripts_dir + "/" + filename;

  if (filename.startsWith("history")) {
    fullPath = scripts_dir + "/history.txt";
    global_mp_stream->println("Deleting history file: " + fullPath);
    clearHistory();
    return true;
  }


  if (!filename.endsWith(".py")) {
    //fullPath += ".py";
  }

  if (!FatFS.exists(fullPath)) {
    global_mp_stream->println("Script not found: " + fullPath);
    return false;
  }

  if (FatFS.remove(fullPath)) {
    // Remove from saved scripts tracking
    for (int i = 0; i < saved_scripts_count; i++) {
      String saved_name = saved_scripts[i];
      if (!saved_name.endsWith(".py")) {
        //saved_name += ".py";
      }
      String check_name = filename;
      if (!check_name.endsWith(".py")) {
       // check_name += ".py";
      }

      if (saved_name == check_name) {
        // Shift remaining scripts down
        for (int j = i; j < saved_scripts_count - 1; j++) {
          saved_scripts[j] = saved_scripts[j + 1];
        }
        saved_scripts_count--;
        break;
      }
    }

    global_mp_stream->println("Script deleted: " + fullPath);
    return true;
  } else {
    global_mp_stream->println("Failed to delete script: " + fullPath);
    return false;
  }
}

void ScriptHistory::listScripts() {
  changeTerminalColor(replColors[9], false, global_mp_stream);

  // Reset numbered scripts mapping
  numbered_scripts_count = 0;

  // Show recent command history (without numbers)
  changeTerminalColor(replColors[6], false, global_mp_stream);
  global_mp_stream->println("\n\rRecent Commands:");
  changeTerminalColor(replColors[9], false, global_mp_stream);
  for (int i = history_count - 1; i >= 0 && i >= history_count - 5;
       i--) { // Show last 5
    String history_line = history[i].substring(0, 60);
    history_line.replace("\n", "\n\r");
    if (history_line.length() > 60)
      history_line += "...";
    global_mp_stream->printf("   %s\n\r", history_line.c_str());
    if (history[i].length() > 60)
      global_mp_stream->println("...");
  }
  if (history_count == 0) {
    global_mp_stream->println("   No commands in history");
  }

  // Show saved script files with numbers
  changeTerminalColor(replColors[6], false, global_mp_stream);
  global_mp_stream->println("\n\rSaved Scripts:");
  changeTerminalColor(replColors[8], false, global_mp_stream);
  if (!FatFS.exists(scripts_dir)) {
    global_mp_stream->println("   No scripts directory");
    return;
  }

  int script_count = 0;

  // First, show scripts we know we saved in this session
  for (int i = 0; i < saved_scripts_count; i++) {
    String fullPath = scripts_dir + "/" + saved_scripts[i];
    if (!saved_scripts[i].endsWith(".py")) {
      fullPath += ".py";
    }

    if (FatFS.exists(fullPath)) {
      File file = FatFS.open(fullPath, "r");
      if (file) {
        if (numbered_scripts_count < 20) {
          numbered_scripts[numbered_scripts_count] = saved_scripts[i];
          String display_name = saved_scripts[i];
          if (!display_name.endsWith(".py")) {
            display_name += ".py";
          }
          global_mp_stream->printf("   %d. %s (%d bytes) [recent]\n\r",
                                   numbered_scripts_count + 1,
                                   display_name.c_str(), file.size());
          numbered_scripts_count++;
          script_count++;
        }
        file.close();
      }
    }
  }

  // Check for sequential numbered scripts that aren't tracked in memory
  for (int i = 1; i <= 50; i++) { // Check script_1.py through script_50.py
    String script_name = "script_" + String(i);
    String test_script = scripts_dir + "/" + script_name + ".py";

    if (FatFS.exists(test_script)) {
      // Check if we already listed this one
      bool already_listed = false;
      for (int j = 0; j < saved_scripts_count; j++) {
        if (saved_scripts[j] == script_name ||
            (saved_scripts[j] + ".py") == (script_name + ".py")) {
          already_listed = true;
          break;
        }
      }

      if (!already_listed && numbered_scripts_count < 20) {
        File file = FatFS.open(test_script, "r");
        if (file) {
          numbered_scripts[numbered_scripts_count] = script_name;
          global_mp_stream->printf("   %d. %s.py (%d bytes)\n\r",
                                   numbered_scripts_count + 1,
                                   script_name.c_str(), file.size());
          numbered_scripts_count++;
          script_count++;
          file.close();
        }
      }
    }
  }

  // Also check for some common named scripts that might exist
  String common_names[] = {"test",  "demo", "main",
                           "setup", "loop", "example"};
  int num_common = sizeof(common_names) / sizeof(common_names[0]);

  for (int i = 0; i < num_common; i++) {
    String test_script = scripts_dir + "/" + common_names[i] + ".py";
    if (FatFS.exists(test_script)) {
      // Check if we already listed this one
      bool already_listed = false;
      for (int j = 0; j < saved_scripts_count; j++) {
        if (saved_scripts[j] == common_names[i] ||
            (saved_scripts[j] + ".py") == (common_names[i] + ".py")) {
          already_listed = true;
          break;
        }
      }

      if (!already_listed && numbered_scripts_count < 20) {
        File file = FatFS.open(test_script, "r");
        if (file) {
          numbered_scripts[numbered_scripts_count] = common_names[i];
          global_mp_stream->printf("   %d. %s.py (%d bytes)\n\r",
                                   numbered_scripts_count + 1,
                                   common_names[i].c_str(), file.size());
          numbered_scripts_count++;
          script_count++;
          file.close();
        }
      }
    }
  }

  if (script_count == 0) {
    global_mp_stream->println("   No saved scripts found");
    global_mp_stream->println(
        "   Use 'save' or 'save scriptname' to save scripts");
  } else {
    global_mp_stream->printf(
        "\n   Type 'load <number>' or 'load <name>' to load a script\n\r");
  }

  global_mp_stream->println();
}

void ScriptHistory::findNextScriptNumber() {
  // Scan for existing script_X.py files to find the next available number
  next_script_number = 1;
  for (int i = 1; i <= 100; i++) { // Check up to script_100.py
    String test_script = scripts_dir + "/script_" + String(i) + ".py";
    if (FatFS.exists(test_script)) {
      next_script_number = i + 1; // Set to next available number
    } else {
      break; // Found first gap, use it
    }
  }
}

void ScriptHistory::saveHistoryToFile() {
  String historyPath = scripts_dir + "/history.txt";
  File file = FatFS.open(historyPath, "w");
  if (!file) {
    return; // Fail silently to avoid spam
  }

  for (int i = 0; i < history_count; i++) {
    file.println("===SCRIPT_START===");
    file.print(history[i]);
    file.println("\n===SCRIPT_END===");
  }
  file.close();
}

void ScriptHistory::loadHistoryFromFile() {
  String historyPath = scripts_dir + "/history.txt";
  if (!FatFS.exists(historyPath)) {
    return; // No history file exists yet
  }

  File file = FatFS.open(historyPath, "r");
  if (!file) {
    return; // Fail silently
  }

  String content = file.readString();
  file.close();

  // Parse saved history
  int start = 0;
  while (start < content.length()) {
    int script_start = content.indexOf("===SCRIPT_START===", start);
    if (script_start == -1)
      break;

    int script_end = content.indexOf("===SCRIPT_END===", script_start);
    if (script_end == -1)
      break;

    script_start += 18; // Length of "===SCRIPT_START==="
    if (content.charAt(script_start) == '\n')
      script_start++;

    String script = content.substring(script_start, script_end);
    script.trim();

    if (script.length() > 0 && history_count < MAX_HISTORY) {
      history[history_count++] = script;
    }

    start = script_end + 16; // Length of "===SCRIPT_END==="
  }
}

// REPLEditor method implementations
// Static variables for cursor tracking (shared between functions)
static bool cursor_position_known = false;
static int last_terminal_line = 0;
static int last_terminal_column = 0;

void REPLEditor::getCurrentLine(String &line, int &line_start, int &cursor_in_line) {
  // Find the newline before the cursor position (start of current line)
  int line_start_pos = 0;
  for (int i = cursor_pos - 1; i >= 0; i--) {
    if (current_input.charAt(i) == '\n') {
      line_start_pos = i + 1;
      break;
    }
  }
  
  // Find the newline after the cursor position (end of current line)
  int line_end_pos = current_input.length();
  for (int i = cursor_pos; i < current_input.length(); i++) {
    if (current_input.charAt(i) == '\n') {
      line_end_pos = i;
      break;
    }
  }
  
  line_start = line_start_pos;
  line = current_input.substring(line_start_pos, line_end_pos);
  cursor_in_line = cursor_pos - line_start_pos;
}

void REPLEditor::moveCursorToColumn(Stream *stream, int column) {
  stream->print("\033[");
  stream->print(column + 1); // Terminal columns are 1-based
  stream->print("G");
  stream->flush();
}

void REPLEditor::clearToEndOfLine(Stream *stream) {
  stream->print("\033[K"); // CSI K - Erase to Right
  stream->flush();
}

void REPLEditor::clearBelow(Stream *stream) {
  stream->print("\033[J"); // CSI J - Erase Below
  stream->flush();
}

void REPLEditor::backspaceOverNewline(Stream *stream) {
  if (cursor_pos > 0 && current_input.charAt(cursor_pos - 1) == '\n') {
    // Remove the newline
    current_input.remove(cursor_pos - 1, 1);
    cursor_pos--;

    // Check if we're leaving multiline mode
    if (current_input.indexOf('\n') == -1) {
      in_multiline_mode = false;
    }

    // Redraw after removing newline
    redrawAndPosition(stream);
  }
}

void REPLEditor::loadFromHistory(Stream *stream, const String &historical_input) {
  if (!in_history_mode) {
    original_input = current_input; // Save current input
    in_history_mode = true;
  }

  current_input = historical_input;
  cursor_pos = current_input.length();
  in_multiline_mode = (current_input.indexOf('\n') >= 0);
  escape_state = 0; // Reset escape state when loading new input
  
  // Flag that we just loaded from history - first Enter should add newline
  just_loaded_from_history = true;

  // For history, just print simply from current line - no complex positioning
  drawFromCurrentLine(stream);

  // Small delay to prevent input processing issues
  delayMicroseconds(100);
}

// Simple drawing function for history - clears previous display and shows new content
void REPLEditor::drawFromCurrentLine(Stream *stream) {
  // Clear the previous history display if we have one
  if (last_displayed_lines > 0) {
    // Move to beginning of current line
    stream->print("\r");
    
    // Move up to the start of the previous display
    for (int i = 0; i < last_displayed_lines; i++) {
      stream->print("\033[A"); // Move up one line
    }
    
    // Move to beginning of line and clear everything below
    stream->print("\r");
    clearBelow(stream);
  } else {
    // Just clear the current line and below
    stream->print("\r");
    clearBelow(stream);
  }
  
  // If we have no input, just show prompt
  if (current_input.length() == 0) {
    changeTerminalColor(replColors[1], true, stream);
    stream->print(">>> ");
    stream->flush();
    last_displayed_lines = 0;
    return;
  }

  // Split input into lines and display each one
  String lines = current_input;
  int line_start = 0;
  int current_line_num = 0;
  int lines_displayed = 0;

  for (int i = 0; i <= lines.length(); i++) {
    if (i == lines.length() || lines.charAt(i) == '\n') {
      String line = lines.substring(line_start, i);

      // Show appropriate prompt
      if (current_line_num == 0) {
        changeTerminalColor(replColors[1], true, stream);
        stream->print(">>> ");
      } else {
        changeTerminalColor(replColors[1], true, stream);
        stream->print("... ");
      }

      // Show line content with syntax highlighting
      displayStringWithSyntaxHighlighting(line, stream);

      // Add newline if not the last line
      if (i < lines.length()) {
        stream->println();
        lines_displayed++;
      }

      line_start = i + 1;
      current_line_num++;
    }
  }

  // Update tracking for next time
  last_displayed_lines = lines_displayed;

  stream->flush();
}

void REPLEditor::exitHistoryMode(Stream *stream) {
  if (in_history_mode) {
    current_input = original_input;
    cursor_pos = current_input.length();
    in_multiline_mode = (current_input.indexOf('\n') >= 0);
    in_history_mode = false;
    just_loaded_from_history = false; // Clear the flag when exiting history mode
    escape_state = 0; // Reset escape state
    drawFromCurrentLine(stream);
  }
}


// ============================================================================
// CENTRALIZED CURSOR POSITION MANAGEMENT SYSTEM
// ============================================================================

// Update cursor position calculations from current cursor_pos
void REPLEditor::updateCursorPosition() {
  cursor_position.line = 0;
  cursor_position.column = 0;
  cursor_position.total_lines = 1; // At least one line
  cursor_position.is_valid = true;
  
  if (current_input.length() == 0) {
    return; // Already initialized to 0,0
  }
  
  // Count total lines
  for (int i = 0; i < current_input.length(); i++) {
    if (current_input.charAt(i) == '\n') {
      cursor_position.total_lines++;
    }
  }
  
  // Find current line and column
  int line_start = 0;
  for (int i = 0; i <= current_input.length() && i <= cursor_pos; i++) {
    if (i == cursor_pos) {
      cursor_position.column = cursor_pos - line_start;
      break;
    }
    
    if (i < current_input.length() && current_input.charAt(i) == '\n') {
      cursor_position.line++;
      line_start = i + 1;
    }
  }
}

// Set cursor_pos from line/column coordinates
void REPLEditor::setCursorFromLineColumn(int line, int col) {
  if (line < 0 || col < 0) return;
  
  // Ensure we have current position data
  if (!cursor_position.is_valid) {
    updateCursorPosition();
  }
  
  // Clamp line to valid range
  line = min(line, cursor_position.total_lines - 1);
  
  // Find the start of the target line
  int target_line_start = 0;
  int current_line = 0;
  
  for (int i = 0; i <= current_input.length(); i++) {
    if (current_line == line) {
      target_line_start = i;
      break;
    }
    
    if (i < current_input.length() && current_input.charAt(i) == '\n') {
      current_line++;
      target_line_start = i + 1;
    }
  }
  
  // Find the end of the target line
  int target_line_end = current_input.length();
  for (int i = target_line_start; i < current_input.length(); i++) {
    if (current_input.charAt(i) == '\n') {
      target_line_end = i;
      break;
    }
  }
  
  // Calculate line length and clamp column
  int line_length = target_line_end - target_line_start;
  col = min(col, line_length);
  
  // Set cursor position
  cursor_pos = target_line_start + col;
  
  // Update position cache
  cursor_position.line = line;
  cursor_position.column = col;
  cursor_position.is_valid = true;
}

// Move cursor up one line (data only, no terminal output)
void REPLEditor::moveCursorUp() {
  updateCursorPosition();
  
  if (cursor_position.line > 0) {
    setCursorFromLineColumn(cursor_position.line - 1, cursor_position.column);
  }
}

// Move cursor down one line (data only, no terminal output)
void REPLEditor::moveCursorDown() {
  updateCursorPosition();
  
  if (cursor_position.line < cursor_position.total_lines - 1) {
    setCursorFromLineColumn(cursor_position.line + 1, cursor_position.column);
  }
}

// Move cursor left one character (data only, no terminal output)
void REPLEditor::moveCursorLeft() {
  if (cursor_pos > 0) {
    cursor_pos--;
    cursor_position.is_valid = false; // Mark for recalculation
  }
}

// Move cursor right one character (data only, no terminal output)
void REPLEditor::moveCursorRight() {
  if (cursor_pos < current_input.length()) {
    cursor_pos++;
    cursor_position.is_valid = false; // Mark for recalculation
  }
}

// Move cursor to start of current line (data only, no terminal output)
void REPLEditor::moveCursorToLineStart() {
  updateCursorPosition();
  setCursorFromLineColumn(cursor_position.line, 0);
}

// Move cursor to end of current line (data only, no terminal output)
void REPLEditor::moveCursorToLineEnd() {
  updateCursorPosition();
  
  // Find the current line length
  int line_start = cursor_pos - cursor_position.column;
  int line_end = current_input.length();
  
  for (int i = line_start; i < current_input.length(); i++) {
    if (current_input.charAt(i) == '\n') {
      line_end = i;
      break;
    }
  }
  
  int line_length = line_end - line_start;
  setCursorFromLineColumn(cursor_position.line, line_length);
}

// Redraw content and position cursor - fixed positioning approach
void REPLEditor::redrawAndPosition(Stream *stream) {
  updateCursorPosition();
  
  // The key insight: we need to always position our display at the same location
  // To do this, we'll track how many lines our display occupies and always
  // clear exactly that many lines, then redraw from the same starting position
  
  // Step 1: Calculate how many lines we need to display
  int total_display_lines = 1; // At least one line for content
  if (current_input.length() > 0) {
    for (int i = 0; i < current_input.length(); i++) {
      if (current_input.charAt(i) == '\n') {
        total_display_lines++;
      }
    }
  }
  
  // Step 2: Clear our previous display area
  // Move to beginning of current line
  stream->print("\r");
  
  // Calculate how far to move up based on where the cursor currently is
  // The terminal cursor should be on the line corresponding to cursor_position.line
  // We need to move up to the first line of our display
  int lines_to_move_up = cursor_position.line;
  
  // Also account for any extra lines if the previous display was larger
  int extra_lines_to_clear = max(0, last_displayed_lines - (total_display_lines - 1));
  lines_to_move_up += extra_lines_to_clear;
  
  if (lines_to_move_up > 0) {
    stream->print("\033[");
    stream->print(lines_to_move_up);
    stream->print("A"); // Move up to first line of display
  }
  
  // Clear everything below this position
  clearBelow(stream);
  
  // Step 3: If we have no input, just show prompt
  if (current_input.length() == 0) {
    changeTerminalColor(replColors[1], true, stream);
    stream->print(">>> ");
    stream->flush();
    last_displayed_lines = 0;
    return;
  }

  // Step 4: Display all lines
  String lines = current_input;
  int line_start = 0;
  int current_line_num = 0;
  int lines_with_newlines = 0;

  for (int i = 0; i <= lines.length(); i++) {
    if (i == lines.length() || lines.charAt(i) == '\n') {
      String line = lines.substring(line_start, i);

      // Show appropriate prompt
      if (current_line_num == 0) {
        changeTerminalColor(replColors[1], true, stream);
        stream->print(">>> ");
      } else {
        changeTerminalColor(replColors[1], true, stream);
        stream->print("... ");
      }

      // Show line content with syntax highlighting
      displayStringWithSyntaxHighlighting(line, stream);

      // Add newline if not the last line
      if (i < lines.length()) {
        stream->println();
        lines_with_newlines++;
      }

      line_start = i + 1;
      current_line_num++;
    }
  }

  // Step 5: Update tracking
  last_displayed_lines = lines_with_newlines;

  // Step 6: Position cursor at the target location
  // We're currently at the end of the last line
  // Move to beginning of last line, then up to first line, then down to target
  stream->print("\r");
  
  if (lines_with_newlines > 0) {
    stream->print("\033[");
    stream->print(lines_with_newlines);
    stream->print("A"); // Move up to first line
  }
  
  // Move down to target line
  if (cursor_position.line > 0) {
    // When at bottom of screen, cursor can't move down, so add newlines instead
    for (int i = 0; i < cursor_position.line; i++) {
      stream->println(); // Add newline to push content up
    }
  }
  
  // Position horizontally
  stream->print("\r");
  int prompt_length = (cursor_position.line == 0) ? 4 : 4;
  int target_column = prompt_length + cursor_position.column;
  moveCursorToColumn(stream, target_column);
  
  stream->flush();
  
  // Reset cursor position tracking for repositionCursorOnly
  resetCursorTracking();
}

// Reset the cursor position tracking (call after redrawAndPosition)
void REPLEditor::resetCursorTracking() {
  updateCursorPosition();
  cursor_position_known = true;
  last_terminal_line = cursor_position.line;
  last_terminal_column = cursor_position.column;
}

// Mark cursor position as unknown (forces next movement to redraw)
void REPLEditor::invalidateCursorTracking() {
  cursor_position_known = false;
}

// Move cursor to correct position without redrawing content
void REPLEditor::repositionCursorOnly(Stream *stream) {
  updateCursorPosition();
  
  // If we don't know where the terminal cursor is, do a full reposition
  if (!cursor_position_known) {
    redrawAndPosition(stream);
    return;
  }
  
  // Calculate relative movement needed
  int line_diff = cursor_position.line - last_terminal_line;
  
  // Move vertically if needed
  if (line_diff != 0) {
    if (line_diff > 0) {
      // Need to move down - but when at bottom of screen, cursor can't move down
      // Instead, add newlines to create space and push content up
      for (int i = 0; i < line_diff; i++) {
        stream->println(); // Add newline to push content up
      }
    } else {
      // Move up, but don't go above line 0
      int lines_to_move_up = min(-line_diff, last_terminal_line);
      if (lines_to_move_up > 0) {
        stream->print("\033[");
        stream->print(lines_to_move_up);
        stream->print("A");
      }
    }
  }
  
  // Always recalculate horizontal position to account for prompts
  stream->print("\r");
  int prompt_length = (cursor_position.line == 0) ? 4 : 4;
  int target_column = prompt_length + cursor_position.column;
  moveCursorToColumn(stream, target_column);
  
  // Update tracking
  last_terminal_line = cursor_position.line;
  last_terminal_column = cursor_position.column;
  
  stream->flush();
}

void REPLEditor::reset() {
  current_input = "";
  cursor_pos = 0;
  in_multiline_mode = false;
  first_run = true;
  escape_state = 0;
  original_input = "";
  in_history_mode = false;
  just_loaded_from_history = false; // Clear the flag on reset
  // Don't reset multiline mode settings - preserve user's choice
  // multiline_override, multiline_forced_on, multiline_forced_off should persist
  last_displayed_lines = 0;
  last_displayed_content = ""; // Clear the last displayed content
  mp_interrupt_requested = false; // Clear any pending interrupt
  
  // Reset cursor position tracking
  cursor_position.line = 0;
  cursor_position.column = 0;
  cursor_position.total_lines = 0;
  cursor_position.is_valid = false;
  
  // Reset terminal cursor tracking
  cursor_position_known = false;
}

void REPLEditor::fullReset() {
  current_input = "";
  cursor_pos = 0;
  in_multiline_mode = false;
  first_run = true;
  escape_state = 0;
  original_input = "";
  in_history_mode = false;
  just_loaded_from_history = false; // Clear the flag on full reset
  multiline_override = false;
  multiline_forced_on = false;
  multiline_forced_off = false;
  last_displayed_lines = 0;
  last_displayed_content = ""; // Clear the last displayed content
  
  // Reset cursor position tracking
  cursor_position.line = 0;
  cursor_position.column = 0;
  cursor_position.total_lines = 0;
  cursor_position.is_valid = false;
  
  // Reset terminal cursor tracking
  cursor_position_known = false;
}

// ============================================================================
// CURSOR MOVEMENT FUNCTIONS - CLEANED UP
// ============================================================================
// 
// This REPLEditor now has a clean, simple cursor movement system:
//
// LOGICAL CURSOR FUNCTIONS (update internal position only):
//   - moveCursorUp(), moveCursorDown() - move cursor between lines
//   - moveCursorLeft(), moveCursorRight() - move cursor within line
//   - moveCursorToLineStart(), moveCursorToLineEnd() - move to line boundaries
//   - updateCursorPosition(), setCursorFromLineColumn() - position management
//
// TERMINAL CONTROL FUNCTIONS (send ANSI escape codes):
//   - clearBelow() - clear everything below cursor
//   - moveCursorToColumn() - position terminal cursor at column
//
// DISPLAY FUNCTIONS:
//   - redrawAndPosition() - redraws content and positions cursor correctly
//   - repositionCursorOnly() - moves cursor without redrawing (for arrow keys)
//   - getCurrentLine() - gets current line information
//   - backspaceOverNewline() - handles backspace over newlines
//   - loadFromHistory(), exitHistoryMode() - history navigation
//
// All deprecated functions have been removed to eliminate confusion.
// ============================================================================

// Note: enterPasteMode function removed - replaced with "new" command that opens eKilo editor
// for creating new scripts. This provides a better user experience than paste mode.

// New functions for single command execution from main.cpp


char result_buffer[64];

// Helper function to apply syntax highlighting to a string
void displayStringWithSyntaxHighlighting(const String& text, Stream* stream) {
  if (text.length() == 0) return;
  
  // Simple syntax highlighting keywords (reusing from eKilo editor)
  const char* python_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del",
    "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass", "print",
    "raise", "return", "try", "while", "with", "yield", "async", "await",
    "nonlocal", "True", "False", "None", nullptr
  };
  
  const char* python_builtins[] = {
    "abs", "all", "any", "bin", "bool", "bytes", "callable", "chr", "dict", 
    "dir", "enumerate", "eval", "filter", "float", "format", "getattr", 
    "globals", "hasattr", "hash", "help", "hex", "id", "input", "int", 
    "isinstance", "iter", "len", "list", "locals", "map", "max", "min", 
    "next", "object", "oct", "open", "ord", "pow", "print", "range", 
    "repr", "reversed", "round", "set", "setattr", "slice", "sorted", 
    "str", "sum", "super", "tuple", "type", "vars", "zip", "self", "cls", nullptr
  };
  
  const char* jumperless_functions[] = {
    "dac_set", "dac_get", "set_dac", "get_dac", "adc_get", "get_adc",
    "ina_get_current", "ina_get_voltage", "ina_get_bus_voltage", "ina_get_power",
    "get_current", "get_voltage", "get_bus_voltage", "get_power",
    "gpio_set", "gpio_get", "gpio_set_dir", "gpio_get_dir", "gpio_set_pull", "gpio_get_pull",
    "set_gpio", "get_gpio", "set_gpio_dir", "get_gpio_dir", "set_gpio_pull", "get_gpio_pull",
    "connect", "disconnect", "is_connected", "nodes_clear", "node",
    "oled_print", "oled_clear", "oled_connect", "oled_disconnect",
    "clickwheel_up", "clickwheel_down", "clickwheel_press",
    "print_bridges", "print_paths", "print_crossbars", "print_nets", "print_chip_status",
    "probe_read", "read_probe", "probe_read_blocking", "probe_read_nonblocking",
    "get_button", "probe_button", "probe_button_blocking", "probe_button_nonblocking",
    "probe_wait", "wait_probe", "probe_touch", "wait_touch", "button_read", "read_button",
    "check_button", "button_check", "arduino_reset", "probe_tap", "run_app", "format_output",
    "help_nodes", "pwm", "pwm_set_frequency", "pwm_set_duty_cycle", "pwm_stop", "send_raw", nullptr
  };
  
  const char* jumperless_constants[] = {
    "TOP_RAIL", "BOTTOM_RAIL", "GND", "DAC0", "DAC1", "ADC0", "ADC1", "ADC2", "ADC3", "ADC4",
    "PROBE", "ISENSE_PLUS", "ISENSE_MINUS", "UART_TX", "UART_RX", "BUFFER_IN", "BUFFER_OUT",
    "GPIO_1", "GPIO_2", "GPIO_3", "GPIO_4", "GPIO_5", "GPIO_6", "GPIO_7", "GPIO_8",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "D10", "D11", "D12", "D13",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "D13_PAD", "TOP_RAIL_PAD", "BOTTOM_RAIL_PAD",
    "LOGO_PAD_TOP", "LOGO_PAD_BOTTOM", "CONNECT_BUTTON", "REMOVE_BUTTON", "BUTTON_NONE",
    "CONNECT", "REMOVE", "NONE", "INPUT", "OUTPUT", "PULLUP", "PULLDOWN", "KEEPER", "HIGH", "LOW", nullptr
  };
  
  const char* jfs_functions[] = {
    // JFS module functions
    "open", "read", "write", "close", "seek", "tell", "size", "available",
    "exists", "listdir", "mkdir", "rmdir", "remove", "rename", "stat", "info",
    "SEEK_SET", "SEEK_CUR", "SEEK_END",
    // Basic filesystem functions
    "fs_exists", "fs_listdir", "fs_read", "fs_write", "fs_cwd", nullptr
  };
  
  auto is_separator = [](char c) -> bool {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != nullptr;
  };
  
  auto is_keyword = [&](const char* word, int len, const char* keywords[]) -> bool {
    for (int i = 0; keywords[i] != nullptr; i++) {
      if (strlen(keywords[i]) == len && !strncmp(word, keywords[i], len)) {
        return true;
      }
    }
    return false;
  };
  
  int i = 0;
  int current_color = -1;
  const char* text_cstr = text.c_str();
  int text_len = text.length();
  
  while (i < text_len) {
    char c = text_cstr[i];
    
    // Handle comments
    if (c == '#') {
      if (current_color != 34) {
        stream->print("\x1b[38;5;34m"); // Green for comments
        current_color = 34;
      }
      // Print rest of line as comment
      while (i < text_len && text_cstr[i] != '\n') {
        stream->write(text_cstr[i]);
        i++;
      }
      continue;
    }
    
    // Handle strings
    if (c == '"' || c == '\'') {
      if (current_color != 39) {
        stream->print("\x1b[38;5;39m"); // Cyan for strings
        current_color = 39;
      }
      char quote = c;
      stream->write(c);
      i++;
      while (i < text_len && text_cstr[i] != quote) {
        if (text_cstr[i] == '\\' && i + 1 < text_len) {
          stream->write(text_cstr[i]); // Backslash
          i++;
          if (i < text_len) {
            stream->write(text_cstr[i]); // Escaped character
            i++;
          }
        } else {
          stream->write(text_cstr[i]);
          i++;
        }
      }
      if (i < text_len) {
        stream->write(text_cstr[i]); // Closing quote
        i++;
      }
      continue;
    }
    
    // Handle numbers
    if (isdigit(c) || (c == '.' && i + 1 < text_len && isdigit(text_cstr[i + 1]))) {
      if (current_color != 199) {
        stream->print("\x1b[38;5;199m"); // Bright red for numbers
        current_color = 199;
      }
      while (i < text_len && (isdigit(text_cstr[i]) || text_cstr[i] == '.')) {
        stream->write(text_cstr[i]);
        i++;
      }
      continue;
    }
    
    // Handle keywords/identifiers
    if (isalpha(c) || c == '_') {
      int start = i;
      while (i < text_len && (isalnum(text_cstr[i]) || text_cstr[i] == '_')) {
        i++;
      }
      
      int word_len = i - start;
      int new_color = 255; // Default white
      
      // Check for different types of keywords
      if (is_keyword(text_cstr + start, word_len, python_keywords)) {
        new_color = 214; // Orange for Python keywords
      } else if (is_keyword(text_cstr + start, word_len, python_builtins)) {
        new_color = 79; // Green for Python builtins
      } else if (is_keyword(text_cstr + start, word_len, jfs_functions)) {
        new_color = 45; // Cyan-blue for JFS filesystem functions
      } else if (is_keyword(text_cstr + start, word_len, jumperless_functions)) {
        new_color = 207; // Bright magenta for Jumperless functions
      } else if (is_keyword(text_cstr + start, word_len, jumperless_constants)) {
        new_color = 105; // Purple for Jumperless constants
      }
      
      if (current_color != new_color) {
        stream->print("\x1b[38;5;");
        stream->print(new_color);
        stream->print("m");
        current_color = new_color;
      }
      
      // Print the word
      for (int j = start; j < start + word_len; j++) {
        stream->write(text_cstr[j]);
      }
      continue;
    }
    
    // Default characters
    if (current_color != 255) {
      stream->print("\x1b[38;5;255m"); // White for default
      current_color = 255;
    }
    stream->write(c);
    i++;
  }
  
  // Reset color at end
  if (current_color != -1) {
    stream->print("\x1b[0m");
  }
}

void getMicroPythonCommandFromStream(Stream *stream) {
  stream->print("Python> ");
  stream->flush();
  
  String command = "";
  while (stream->available() == 0) {
    delay(1); // Wait for input
  }
  
  // Read input character by character with syntax highlighting
  while (stream->available() > 0) {
    char c = stream->read();
    if (c == '\r' || c == '\n') {
      break;
    }
    if (c == '\b' || c == 127) { // Backspace
      if (command.length() > 0) {
        command = command.substring(0, command.length() - 1);
        stream->print("\b \b"); // Erase character
      }
    } else if (c >= 32 && c <= 126) { // Printable characters
      command += c;
      // Real-time syntax highlighting - redraw the visible part
      stream->print("\rPython> ");
      displayStringWithSyntaxHighlighting(command, stream);
      stream->flush();
    }
  }
  
  stream->println(); // New line after input
  command.trim();
  
  if (command.length() > 0) {
    bool success = executeSinglePythonCommandFormatted(command.c_str(), result_buffer, sizeof(result_buffer));
    stream->printf(result_buffer);
  }
}

/**
 * Initialize MicroPython quietly without any output
 * Returns true if successful, false if failed
 */
bool initMicroPythonQuiet(void) {
  if (mp_initialized) {
    return true;
  }

  // Store original stream and redirect to null
  Stream *original_stream = global_mp_stream;
  global_mp_stream = nullptr;
  global_mp_stream_ptr = nullptr;

  // Get proper stack pointer
  char stack_dummy;
  char *stack_top = &stack_dummy;

  // Initialize MicroPython silently
  mp_embed_init(mp_heap, sizeof(mp_heap), stack_top);
  
  // Set Ctrl+Q (ASCII 17) as the keyboard interrupt character instead of Ctrl+C (ASCII 3)
  // This enables proper KeyboardInterrupt exceptions that can be caught by try/except
  // and will automatically interrupt running loops/scripts when Ctrl+Q is pressed
  mp_embed_exec_str("import micropython; micropython.kbd_intr(17)");
  
  mp_initialized = true;
  mp_repl_active = false;

  // Restore original stream
  global_mp_stream = original_stream;
  global_mp_stream_ptr = (void *)original_stream;
  
  // Import all jumperless functions and constants globally (silently)
  // This ensures everything is available for single commands without prefix
  mp_embed_exec_str(
      "try:\n"
      "    import jumperless\n"
      "    from jumperless import *\n"
      "    globals()['jumperless'] = jumperless\n"
      "    \n"
      "    # Add interrupt checking helper for tight loops\n"
      "    def check_interrupt():\n"
      "        import time\n"
      "        time.sleep_ms(1)\n"
      "    globals()['check_interrupt'] = check_interrupt\n"
      "except: pass\n");
  
  return true;
}

// Function output type enumeration
enum FunctionOutputType {
  OUTPUT_NONE,           // No output formatting
  OUTPUT_VOLTAGE,        // Format as voltage with V unit
  OUTPUT_CURRENT,        // Format as current with mA unit  
  OUTPUT_POWER,          // Format as power with mW unit
  OUTPUT_GPIO_STATE,     // Format as HIGH/LOW
  OUTPUT_GPIO_DIR,       // Format as INPUT/OUTPUT
  OUTPUT_GPIO_PULL,      // Format as PULLUP/NONE/PULLDOWN
  OUTPUT_BOOL_CONNECTED, // Format as CONNECTED/DISCONNECTED
  OUTPUT_BOOL_YESNO,     // Format as YES/NO
  OUTPUT_COUNT,          // Format as simple number
  OUTPUT_FLOAT           // Format as float with precision
};

// Function type mapping structure
struct FunctionTypeMap {
  const char* function_name;
  FunctionOutputType output_type;
};

/**
 * Global mapping of function names to their output types for formatted printing
 */
static const FunctionTypeMap function_type_map[] = {
  // DAC functions
  {"dac_set", OUTPUT_NONE},
  {"dac_get", OUTPUT_VOLTAGE},
  {"set_dac", OUTPUT_NONE},           // Alias
  {"get_dac", OUTPUT_VOLTAGE},        // Alias
  
  // ADC functions  
  {"adc_get", OUTPUT_VOLTAGE},
  {"get_adc", OUTPUT_VOLTAGE},        // Alias
  
  // INA functions
  {"ina_get_current", OUTPUT_CURRENT},
  {"ina_get_voltage", OUTPUT_VOLTAGE},
  {"ina_get_bus_voltage", OUTPUT_VOLTAGE},
  {"ina_get_power", OUTPUT_POWER},
  {"get_ina_current", OUTPUT_CURRENT},      // Alias
  {"get_ina_voltage", OUTPUT_VOLTAGE},      // Alias
  {"get_ina_bus_voltage", OUTPUT_VOLTAGE},  // Alias
  {"get_ina_power", OUTPUT_POWER},          // Alias
  {"get_current", OUTPUT_CURRENT},          // Alias
  {"get_voltage", OUTPUT_VOLTAGE},          // Alias
  {"get_bus_voltage", OUTPUT_VOLTAGE},      // Alias
  {"get_power", OUTPUT_POWER},              // Alias
  
  // PWM functions
  {"pwm", OUTPUT_NONE},
  {"pwm_set_frequency", OUTPUT_NONE},
  {"pwm_set_duty_cycle", OUTPUT_NONE},
  {"pwm_stop", OUTPUT_NONE},
  {"set_pwm", OUTPUT_NONE},                    // Alias
  {"set_pwm_frequency", OUTPUT_NONE},          // Alias
  {"set_pwm_duty_cycle", OUTPUT_NONE},         // Alias
  {"stop_pwm", OUTPUT_NONE},                   // Alias
  
  // GPIO functions
  {"gpio_set", OUTPUT_NONE},
  {"gpio_get", OUTPUT_GPIO_STATE},
  {"gpio_set_dir", OUTPUT_NONE},
  {"gpio_get_dir", OUTPUT_GPIO_DIR},
  {"gpio_set_pull", OUTPUT_NONE},
  {"gpio_get_pull", OUTPUT_GPIO_PULL},
  {"set_gpio", OUTPUT_NONE},               // Alias
  {"get_gpio", OUTPUT_GPIO_STATE},         // Alias
  {"set_gpio_dir", OUTPUT_NONE},           // Alias
  {"get_gpio_dir", OUTPUT_GPIO_DIR},       // Alias
  {"set_gpio_pull", OUTPUT_NONE},          // Alias
  {"get_gpio_pull", OUTPUT_GPIO_PULL},     // Alias
  {"set_gpio_direction", OUTPUT_NONE},     // Alias
  {"get_gpio_direction", OUTPUT_GPIO_DIR}, // Alias
  
  // Node functions
  {"connect", OUTPUT_BOOL_CONNECTED},
  {"disconnect", OUTPUT_NONE},
  {"nodes_clear", OUTPUT_NONE},
  {"is_connected", OUTPUT_BOOL_CONNECTED},
  {"connect_nodes", OUTPUT_BOOL_CONNECTED},     // Alias
  {"disconnect_nodes", OUTPUT_NONE},            // Alias
  {"clear_nodes", OUTPUT_NONE},                 // Alias
  {"clear_connections", OUTPUT_NONE},           // Alias
  {"nodes_connected", OUTPUT_BOOL_CONNECTED},   // Alias
  {"connected", OUTPUT_BOOL_CONNECTED},         // Alias
  
  // OLED functions
  {"oled_print", OUTPUT_NONE},
  {"oled_clear", OUTPUT_NONE},
  {"oled_show", OUTPUT_NONE},
  {"oled_connect", OUTPUT_BOOL_YESNO},
  {"oled_disconnect", OUTPUT_NONE},
  {"print_oled", OUTPUT_NONE},          // Alias
  {"clear_oled", OUTPUT_NONE},          // Alias
  {"show_oled", OUTPUT_NONE},           // Alias
  {"connect_oled", OUTPUT_BOOL_YESNO},  // Alias
  {"disconnect_oled", OUTPUT_NONE},     // Alias
  {"display_print", OUTPUT_NONE},       // Alias
  {"display_clear", OUTPUT_NONE},       // Alias
  {"display_show", OUTPUT_NONE},        // Alias
  
  // Status functions
  {"print_bridges", OUTPUT_NONE},
  {"print_paths", OUTPUT_NONE},
  {"print_crossbars", OUTPUT_NONE},
  {"print_nets", OUTPUT_NONE},
  {"print_chip_status", OUTPUT_NONE},
  {"show_bridges", OUTPUT_NONE},        // Alias
  {"show_paths", OUTPUT_NONE},          // Alias
  {"show_crossbars", OUTPUT_NONE},      // Alias
  {"show_nets", OUTPUT_NONE},           // Alias
  {"show_chip_status", OUTPUT_NONE},    // Alias
  {"bridges", OUTPUT_NONE},             // Alias
  {"paths", OUTPUT_NONE},               // Alias
  {"crossbars", OUTPUT_NONE},           // Alias
  {"nets", OUTPUT_NONE},                // Alias
  {"chip_status", OUTPUT_NONE},         // Alias
  
  // Other functions
  {"arduino_reset", OUTPUT_NONE},
  {"probe_tap", OUTPUT_NONE},
  {"clickwheel_up", OUTPUT_NONE},
  {"clickwheel_down", OUTPUT_NONE},
  {"clickwheel_press", OUTPUT_NONE},
  {"run_app", OUTPUT_NONE},
  {"help", OUTPUT_NONE},
  {"reset_arduino", OUTPUT_NONE},       // Alias
  {"reset", OUTPUT_NONE},               // Alias
  {"app_run", OUTPUT_NONE},             // Alias
  {"tap_probe", OUTPUT_NONE},           // Alias
  {"tap", OUTPUT_NONE},                 // Alias
  {"wheel_up", OUTPUT_NONE},            // Alias
  {"wheel_down", OUTPUT_NONE},          // Alias
  {"wheel_press", OUTPUT_NONE},         // Alias
  {"click_up", OUTPUT_NONE},            // Alias
  {"click_down", OUTPUT_NONE},          // Alias
  {"click_press", OUTPUT_NONE},         // Alias
  {"scroll_up", OUTPUT_NONE},           // Alias
  {"scroll_down", OUTPUT_NONE},         // Alias
  {"press", OUTPUT_NONE},               // Alias
  {"send_raw", OUTPUT_NONE},            
  {"pause_core2", OUTPUT_NONE},
  
  {nullptr, OUTPUT_NONE} // End marker
};

/**
 * List of jumperless module function names for automatic prefix detection
 */
static const char* jumperless_functions[] = {
  // DAC functions
  "dac_set", "dac_get", "set_dac", "get_dac",
  // ADC functions  
  "adc_get", "get_adc",
  // INA functions
  "ina_get_current", "ina_get_voltage", "ina_get_bus_voltage", "ina_get_power",
  "get_ina_current", "get_ina_voltage", "get_ina_bus_voltage", "get_ina_power",
  "get_current", "get_voltage", "get_bus_voltage", "get_power",
  // PWM functions
  "pwm", "pwm_set_frequency", "pwm_set_duty_cycle", "pwm_stop",
  "set_pwm", "set_pwm_frequency", "set_pwm_duty_cycle", "stop_pwm",
  // GPIO functions
  "gpio_set", "gpio_get", "gpio_set_dir", "gpio_get_dir", "gpio_set_pull", "gpio_get_pull",
  "set_gpio", "get_gpio", "set_gpio_dir", "get_gpio_dir", "set_gpio_pull", "get_gpio_pull",
  "set_gpio_direction", "get_gpio_direction",
  // Node functions
  "connect", "disconnect", "nodes_clear", "is_connected",
  "connect_nodes", "disconnect_nodes", "clear_nodes", "clear_connections", "nodes_connected", "connected",
  // OLED functions
  "oled_print", "oled_clear", "oled_show", "oled_connect", "oled_disconnect",
  "print_oled", "clear_oled", "show_oled", "connect_oled", "disconnect_oled",
  "display_print", "display_clear", "display_show",
  // Status functions
  "print_bridges", "print_paths", "print_crossbars", "print_nets", "print_chip_status",
  "show_bridges", "show_paths", "show_crossbars", "show_nets", "show_chip_status",
  "bridges", "paths", "crossbars", "nets", "chip_status",
  // Other functions
  "arduino_reset", "probe_tap", "clickwheel_up", "clickwheel_down", "clickwheel_press", "run_app", "help",
  "reset_arduino", "reset", "app_run", "tap_probe", "tap",
  "wheel_up", "wheel_down", "wheel_press", "click_up", "click_down", "click_press",
  "scroll_up", "scroll_down", "press", "send_raw", "pause_core2",
  nullptr // End marker
};

/**
 * Check if a function name is a jumperless module function
 */
bool isJumperlessFunction(const char* function_name) {
  for (int i = 0; jumperless_functions[i] != nullptr; i++) {
    if (strcmp(function_name, jumperless_functions[i]) == 0) {
      return true;
    }
  }
  return false;
}

/**
 * Get the output type for a function name
 */
FunctionOutputType getFunctionOutputType(const char* function_name) {
  for (int i = 0; function_type_map[i].function_name != nullptr; i++) {
    if (strcmp(function_type_map[i].function_name, function_name) == 0) {
      return function_type_map[i].output_type;
    }
  }
  return OUTPUT_NONE;
}

/**
 * Extract function name from a command string
 * e.g., "gpio_get(2)" -> "gpio_get"
 */
String extractFunctionName(const String& command) {
  int paren_pos = command.indexOf('(');
  if (paren_pos == -1) {
    return command; // No parentheses found
  }
  
  String func_name = command.substring(0, paren_pos);
  func_name.trim();
  
  // Since functions are now globally imported, no prefix handling needed
  return func_name;
}

/**
 * Format a result value based on the function output type
 */
String formatResult(float value, FunctionOutputType output_type) {
  switch (output_type) {
    case OUTPUT_VOLTAGE:
      return String(value, 3) + "V";
      
    case OUTPUT_CURRENT:
      if (value >= 1000.0f) {
        return String(value / 1000.0f, 3) + "A";
      } else {
        return String(value, 1) + "mA";
      }
      
    case OUTPUT_POWER:
      if (value >= 1000.0f) {
        return String(value / 1000.0f, 3) + "W";
      } else {
        return String(value, 1) + "mW";
      }
      
    case OUTPUT_GPIO_STATE:
      return (value != 0.0f) ? "HIGH" : "LOW";
      
    case OUTPUT_GPIO_DIR:
      return (value != 0.0f) ? "OUTPUT" : "INPUT";
      
    case OUTPUT_GPIO_PULL:
      if (value > 1.5f) return "KEEPER";
      else if (value > 0.5f) return "PULLUP";
      else if (value < -0.5f) return "PULLDOWN";
      else return "NONE";
      
    case OUTPUT_BOOL_CONNECTED:
      return (value != 0.0f) ? "CONNECTED" : "DISCONNECTED";
      
    case OUTPUT_BOOL_YESNO:
      return (value != 0.0f) ? "YES" : "NO";
      
    case OUTPUT_COUNT:
      return String((int)value);
      
    case OUTPUT_FLOAT:
      return String(value, 3);
      
    case OUTPUT_NONE:
    default:
      return "OK";
  }
}

/**
 * Parse a command and add jumperless. prefix if needed
 * Returns a new String with the parsed command
 */
String parseCommandWithPrefix(const char* command) {
  // Since all jumperless functions are now globally imported,
  // we no longer need to add prefixes - just return the command as-is
  String cmd = String(command);
  cmd.trim();
  return cmd;
}

/**
 * Execute a single MicroPython command with automatic initialization and prefix handling
 * This function can be called from main.cpp
 * 
 * @param command The command to execute (e.g., "gpio_get(2)" or "dac_set(0, 3.3)")
 * @param result_buffer Optional buffer to store string result (can be nullptr)
 * @param buffer_size Size of result buffer
 * @return true if command executed successfully, false otherwise
 */
bool executeSinglePythonCommand(const char* command, char* result_buffer, size_t buffer_size) {
  // Initialize quietly if needed
  if (!mp_initialized) {
    if (!initMicroPythonQuiet()) {
      if (result_buffer && buffer_size > 0) {
        strncpy(result_buffer, "ERROR: Failed to initialize MicroPython", buffer_size - 1);
        result_buffer[buffer_size - 1] = '\0';
      }
      return false;
    }
  }
  
  // Parse command and add prefix if needed
  String parsed_command = parseCommandWithPrefix(command);
  
  // Clear result buffer
  if (result_buffer && buffer_size > 0) {
    memset(result_buffer, 0, buffer_size);
  }
  
  bool success = true;
  
  // Execute the command directly - MicroPython will handle errors internally
  mp_embed_exec_str(parsed_command.c_str());
  

  if (result_buffer && buffer_size > 0) {
    strncpy(result_buffer, "OK", buffer_size - 1);
    result_buffer[buffer_size - 1] = '\0';
  }
  
  return success;
}

/**
 * Enhanced command execution with formatted output
 * This version captures the return value and formats it according to function type
 */
bool executeSinglePythonCommandFormatted(const char* command, char* result_buffer, size_t buffer_size) {
  // Initialize quietly if needed
  if (!mp_initialized) {
    if (!initMicroPythonQuiet()) {
      if (result_buffer && buffer_size > 0) {
        strncpy(result_buffer, "ERROR: Failed to initialize MicroPython", buffer_size - 1);
        result_buffer[buffer_size - 1] = '\0';
      }
      return false;
    }
  }
  
  // Parse command and add prefix if needed
  String parsed_command = parseCommandWithPrefix(command);
  
  // Clear result buffer
  if (result_buffer && buffer_size > 0) {
    memset(result_buffer, 0, buffer_size);
  }
  
  // Note: Formatted output is now handled natively by the jumperless C module
  // Functions automatically return formatted strings like "HIGH", "3.300V", "123.4mA"
  
  // Simply execute the command - formatting is now handled by the native C module
  mp_embed_exec_str(parsed_command.c_str());
  
  // if (result_buffer && buffer_size > 0) {
  //   strncpy(result_buffer, "Formatted by native module", buffer_size - 1);
  //   result_buffer[buffer_size - 1] = '\0';
  // }
  
  return true;
}

/**
 * Execute a single MicroPython command and return float result
 * Useful for functions that return numeric values like adc_get(), gpio_get(), etc.
 * 
 * @param command The command to execute (e.g., "gpio_get(2)")
 * @param result Pointer to store the numeric result
 * @return true if command executed successfully and result is valid, false otherwise
 */
bool executeSinglePythonCommandFloat(const char* command, float* result) {
  if (!result) return false;
  
  // Initialize quietly if needed
  if (!mp_initialized) {
    if (!initMicroPythonQuiet()) {
      return false;
    }
  }
  
  // Parse command and add prefix if needed
  String parsed_command = parseCommandWithPrefix(command);
  
  bool success = true;
  *result = 0.0f;
  
  // For now, just execute the command directly
  // TODO: Implement proper result capture to get the actual return value
  mp_embed_exec_str(parsed_command.c_str());
  
  return success;
}

/**
 * Simple convenience function for common commands
 * Returns the result as a float (useful for sensor readings)
 */
float quickPythonCommand(const char* command) {
  float result = 0.0f;
  executeSinglePythonCommandFloat(command, &result);
  return result;
}

/**
 * Test function to demonstrate single command execution
 * Can be called from main.cpp to test the functionality
 */
void testSingleCommandExecution(void) {
  if (!global_mp_stream) return;
  
  global_mp_stream->println("\n=== Testing Single Command Execution ===");
  
  // Ensure MicroPython is initialized with jumperless module
  if (!mp_initialized) {
    global_mp_stream->println("Initializing MicroPython quietly...");
    if (!initMicroPythonQuiet()) {
      global_mp_stream->println("ERROR: Failed to initialize MicroPython!");
      return;
    }
    global_mp_stream->println("MicroPython initialized successfully");
  }
  
  // Test 1: Simple command with automatic prefix
  global_mp_stream->println("Test 1: GPIO read with automatic prefix");
  char result_buffer[64];
  bool success = executeSinglePythonCommand("gpio_get(2)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("Command: gpio_get(2) -> %s (success: %s)\n", 
                           result_buffer, success ? "true" : "false");
  
  // Test 2: Command that already has prefix (should not add another)
  global_mp_stream->println("\nTest 2: Command with existing prefix");
  success = executeSinglePythonCommand("jumperless.dac_set(0, 2.5)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("Command: jumperless.dac_set(0, 2.5) -> %s (success: %s)\n", 
                           result_buffer, success ? "true" : "false");
  
  // Test 3: Python command that should not get prefix
  global_mp_stream->println("\nTest 3: Python command (no prefix)");
  success = executeSinglePythonCommand("print('Hello from Python!')", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("Command: print('Hello from Python!') -> %s (success: %s)\n", 
                           result_buffer, success ? "true" : "false");
  
  // Test 4: Float result function
  global_mp_stream->println("\nTest 4: Float result function");
  float float_result = 0.0f;
  success = executeSinglePythonCommandFloat("adc_get(0)", &float_result);
  global_mp_stream->printf("Command: adc_get(0) -> %.3f (success: %s)\n", 
                           float_result, success ? "true" : "false");
  
  // Test 5: Quick command function
  global_mp_stream->println("\nTest 5: Quick command function");
  float quick_result = quickPythonCommand("gpio_get(1)");
  global_mp_stream->printf("quickPythonCommand('gpio_get(1)') -> %.3f\n", quick_result);
  
  // Test 6: Command parsing demonstration
  global_mp_stream->println("\nTest 6: Command parsing examples");
  String parsed;
  
  parsed = parseCommandWithPrefix("gpio_get(2)");
  global_mp_stream->println("gpio_get(2) -> " + parsed);
  
  parsed = parseCommandWithPrefix("jumperless.dac_set(0, 3.3)");
  global_mp_stream->println("jumperless.dac_set(0, 3.3) -> " + parsed);
  
  parsed = parseCommandWithPrefix("print('test')");
  global_mp_stream->println("print('test') -> " + parsed);
  
  parsed = parseCommandWithPrefix("connect(1, 5)");
  global_mp_stream->println("connect(1, 5) -> " + parsed);
  
  global_mp_stream->println("\n=== Single Command Test Complete ===\n");
}

/**
 * Test function to demonstrate formatted output
 */
void testFormattedOutput(void) {
  if (!global_mp_stream) return;
  
  global_mp_stream->println("\n=== Testing Formatted Output ===");
  
  // Ensure MicroPython is initialized
  if (!mp_initialized) {
    if (!initMicroPythonQuiet()) {
      global_mp_stream->println("ERROR: Failed to initialize MicroPython!");
      return;
    }
  }
  
  char result_buffer[64];
  bool success;
  
  // Test GPIO state formatting
  global_mp_stream->println("\nGPIO State Formatting:");
  success = executeSinglePythonCommandFormatted("gpio_get(2)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  gpio_get(2) -> %s\n", result_buffer);
  
  // Test voltage formatting
  global_mp_stream->println("\nVoltage Formatting:");
  success = executeSinglePythonCommandFormatted("dac_get(0)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  dac_get(0) -> %s\n", result_buffer);
  
  success = executeSinglePythonCommandFormatted("adc_get(1)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  adc_get(1) -> %s\n", result_buffer);
  
  // Test current formatting
  global_mp_stream->println("\nCurrent/Power Formatting:");
  success = executeSinglePythonCommandFormatted("ina_get_current(0)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  ina_get_current(0) -> %s\n", result_buffer);
  
  success = executeSinglePythonCommandFormatted("ina_get_power(0)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  ina_get_power(0) -> %s\n", result_buffer);
  
  // Test GPIO direction formatting
  global_mp_stream->println("\nGPIO Direction Formatting:");
  success = executeSinglePythonCommandFormatted("gpio_get_dir(3)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  gpio_get_dir(3) -> %s\n", result_buffer);
  
  // Test GPIO pull formatting
  global_mp_stream->println("\nGPIO Pull Formatting:");
  success = executeSinglePythonCommandFormatted("gpio_get_pull(4)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  gpio_get_pull(4) -> %s\n", result_buffer);
  
  // Test connection formatting
  global_mp_stream->println("\nConnection Status Formatting:");
  success = executeSinglePythonCommandFormatted("is_connected(1, 5)", result_buffer, sizeof(result_buffer));
  global_mp_stream->printf("  is_connected(1, 5) -> %s\n", result_buffer);
  
  global_mp_stream->println("\n=== Formatted Output Test Complete ===\n");
}


//Very recent change here

// Filesystem setup and module path configuration
void setupFilesystemAndPaths(void) {
  changeTerminalColor(replColors[11], true, global_mp_stream);
  global_mp_stream->println("Setting up filesystem and module paths...");
  changeTerminalColor(replColors[13], true, global_mp_stream);
  
  // Set up sys.path for module imports using our filesystem bridge
  mp_embed_exec_str(
    "try:\n"
    "    import sys\n"
    "    import jumperless\n"
    "    \n"
    "    # Clear existing sys.path and set up Jumperless-specific paths\n"
    "    sys.path.clear()\n"
    "    sys.path.append('')  # Current directory\n"
    "    \n"
    "    # Add Jumperless module directories using our filesystem bridge\n"
    "    paths_to_add = [\n"
    "        '/python_scripts',\n"
    "        '/python_scripts/lib',\n"
    "        '/python_scripts/modules',\n"
    "        '/python_scripts/examples',\n"
    "        #'/lib',\n"
    "        #'/modules'\n"
    "    ]\n"
    "    \n"
    "    for path in paths_to_add:\n"
    "        try:\n"
    "            # Check if path exists using jumperless filesystem bridge\n"
    "            if path in ['/', '']:\n"
    "                if path not in sys.path:\n"
    "                    sys.path.append(path)\n"
    "            elif jumperless.fs_exists(path):\n"
    "                if path not in sys.path:\n"
    "                    sys.path.append(path)\n"
    "                   # print('Added ' + path + ' to sys.path')\n"
    "                #else:\n"
    "                   # print('Path already in sys.path: ' + path)\n"
    "            #else:\n"
    "                #print('Skipping non-existent path: ' + path)\n"
    "        except Exception as e:\n"
    "            print('Error adding ' + path + ': ' + str(e))\n"
    "    \n"
    "    print('Module search paths:')\n"
    "    for i, path in enumerate(sys.path):\n"
    "        if path == '':\n"
    "            print('  ' + str(i) + ': ' + '/  (root)')\n"
    "        else:\n"
    "            print('  ' + str(i) + ': ' + path)\n"
    "    \n"
    "    #print()\n"
    "    print('Place .py and .mpy modules in:')\n"
    "    print('  /python_scripts/lib/  - User modules')\n"
    "    print('  /python_scripts/      - User scripts')\n"
    "    print()\n"
    "    \n"
    "except ImportError as e:\n"
    "    print('sys or os module not available:', e)\n"
    "except Exception as e:\n"
    "    print('Error setting up module paths:', e)\n"
  );
  
  // Test basic module availability
  // mp_embed_exec_str(
  //   "try:\n"
  //   "    # Test that we can import basic modules\n"
  //   "    import time\n"
  //   "    print('✓ time module available')\n"
  //   "except ImportError:\n"
  //   "    print('✗ time module not available')\n"
  //   "\n"
  //   "try:\n"
  //   "    import os\n"
  //   "    print('✓ os module available')\n"
  //   "except ImportError:\n"
  //   "    print('✗ os module not available')\n"
  //   "\n"
  //   "try:\n"
  //   "    import gc\n"
  //   "    print('✓ gc module available')\n"
  //   "except ImportError:\n"
  //   "    print('✗ gc module not available')\n"
  // );
}

/**
 * Comprehensive file cleanup function
 * Closes all potentially open files across the entire system
 */
void closeAllOpenFiles(void) {
  // if (global_mp_stream) {
  //   global_mp_stream->println("[FS] Closing all open files...");
  // }
  
  // 1. Close global file handles from FileParsing.cpp
  closeAllFiles();
  
  // 2. File manager cleanup is handled automatically when it goes out of scope
  // if (global_mp_stream) {
  //   global_mp_stream->println("[FS] File manager cleanup handled automatically...");
  // }
  
  // 3. JFS files are automatically cleaned up by Python garbage collection
  // Skip MicroPython execution during cleanup to avoid crashes during shutdown
  // if (global_mp_stream) {
  //   // global_mp_stream->println("[FS] JFS files will be cleaned up by garbage collection");
  // }
  
  // 4. Light filesystem sync - just flush without restarting the filesystem
  // if (global_mp_stream) {
  //   // global_mp_stream->println("[FS] File cleanup complete");
  // }
}





