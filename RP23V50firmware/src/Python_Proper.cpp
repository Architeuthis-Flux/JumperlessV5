#include "Python_Proper.h"
#include <Arduino.h>

extern "C" {
#include <micropython_embed.h>
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/stackctrl.h"
}

// Global state for proper MicroPython integration
static char mp_heap[32 * 1024];  // 32KB heap for MicroPython
static bool mp_initialized = false;
static bool mp_repl_active = false;

// Command execution state
static char mp_command_buffer[512];
static bool mp_command_ready = false;
static char mp_response_buffer[1024];

// Terminal colors for different REPL states
// 0 = menu, 1 = prompt, 2 = output, 3 = input, 4 = error, 5 = prompt alt 1, 6 = prompt alt 2, 7 = prompt alt 3
static int replColors[8] = {
    38,  // menu
    69,  // prompt
    155, // output
    221, // input
    202, // error
    62,  // prompt alt 1
    56,  // prompt alt 2
    51,  // prompt alt 3
};

// Forward declaration for color function
void changeTerminalColor(int color, bool bold);

// Forward declarations
extern "C" {
    void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len);
    int mp_hal_stdin_rx_chr(void);
    void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len);
    void mp_hal_delay_ms(mp_uint_t ms);
    mp_uint_t mp_hal_ticks_ms(void);
    
    // Arduino wrapper functions for HAL
    int arduino_serial_available(void);
    int arduino_serial_read(void);
    void arduino_serial_write(const char *str, int len);
    void arduino_delay_ms(unsigned int ms);
    unsigned int arduino_millis(void);
}

// Arduino timing functions for MicroPython
extern "C" void mp_hal_delay_ms(mp_uint_t ms) {
    delay(ms);
}

extern "C" mp_uint_t mp_hal_ticks_ms(void) {
    return millis();
}

// Arduino wrapper functions for the HAL layer
extern "C" int arduino_serial_available(void) {
    return Serial.available();
}

extern "C" int arduino_serial_read(void) {
    return Serial.read();
}

extern "C" void arduino_serial_write(const char *str, int len) {
    Serial.write(str, len);
    Serial.flush();
}

extern "C" void arduino_delay_ms(unsigned int ms) {
    delay(ms);
}

extern "C" unsigned int arduino_millis(void) {
    return millis();
}

// Terminal color control function
void changeTerminalColor(int color, bool bold) {
    Serial.print("\033[");
    if (bold) {
        Serial.print("1;");
    }
    Serial.print("38;5;");
    Serial.print(color);
    Serial.print("m");
    Serial.flush();
}

// Character input from Arduino Serial - non-blocking
extern "C" int mp_hal_stdin_rx_chr(void) {
    if (Serial.available()) {
        return Serial.read();
    }
    return -1; // No character available
}

// Character output to Arduino Serial
extern "C" void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
    Serial.write(str, len);
    Serial.flush();
}

// Cooked output (handles newlines, etc.)
extern "C" void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // Check for synchronous command execution requests
    if (len > 10 && strncmp(str, "SYNC_EXEC:", 10) == 0) {
        // Extract command for immediate synchronous execution
        String command = String(str + 10).substring(0, len - 10);
        command.trim();
        
        Serial.print("[SYNC] Executing: ");
        Serial.println(command);
        
        // Execute the hardware command immediately
        char response[512] = {0};
        int result = parseAndExecutePythonCommand((char*)command.c_str(), response);
        
        Serial.print("[HARDWARE] Result: ");
        Serial.println(response);
        
        // Set the Python sync variables immediately
        String python_code = "";
        if (result == 0) {
            // Success - set the result value and type
            python_code = "_sync_result_ready = True; ";
            python_code += "_sync_value = '";
            python_code += String(response);
            python_code += "'; ";
            python_code += "_sync_type = 'str'";
        } else {
            // Error - set error result
            python_code = "_sync_result_ready = True; ";
            python_code += "_sync_value = 'Command failed: ";
            python_code += String(response);
            python_code += "'; ";
            python_code += "_sync_type = 'error'";
        }
        
        // Execute the Python code to set sync variables
        mp_embed_exec_str(python_code.c_str());
        
        return; // Don't print SYNC_EXEC commands to terminal
    }
    
    // Check for regular command execution requests (legacy)
    if (len > 5 && strncmp(str, "EXEC:", 5) == 0) {
        // Extract command for later execution
        size_t cmd_len = len - 5;
        if (cmd_len < sizeof(mp_command_buffer)) {
            strncpy(mp_command_buffer, str + 5, cmd_len);
            mp_command_buffer[cmd_len] = '\0';
            
            // Remove newlines
            for (int i = 0; mp_command_buffer[i]; i++) {
                if (mp_command_buffer[i] == '\n' || mp_command_buffer[i] == '\r') {
                    mp_command_buffer[i] = '\0';
                    break;
                }
            }
            mp_command_ready = true;
            return; // Don't print EXEC commands
        }
    }
    
    // Regular output - convert \n to \r\n for proper terminal display
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\n') {
            Serial.write('\r');
        }
        Serial.write(str[i]);
    }
    Serial.flush();
}

bool initMicroPythonProper(void) {
    if (mp_initialized) {
        return true;
    }
    
    Serial.println("[MP] Initializing MicroPython with proper porting...");
    
    // Initialize MicroPython
    mp_embed_init(mp_heap, sizeof(mp_heap), &mp_heap[sizeof(mp_heap)]);
    
    // Set up Python path and basic modules (with error handling)
    mp_embed_exec_str(
        "try:\n"
        "    import sys\n"
        "    sys.path.append('/lib')\n"
        "    print('MicroPython', sys.version)\n"
        "except ImportError as e:\n"
        "    print('MicroPython initialized (sys module unavailable)')\n"
        "print('\\nJumperless hardware control available')\n"
    );
    changeTerminalColor(replColors[5], true);
    // Test if native jumperless module is available
    mp_embed_exec_str(
        "try:\n"
        "    import jumperless\n"
        "    print('Native jumperless module imported successfully')\n"
        "    print('Available functions:')\n"
        "    for func in dir(jumperless):\n"
        "        if not func.startswith('_'):\n"
        "            print('  jumperless.' + func + '()')\n"
        "    print()  # Empty line\n"
        "except ImportError as e:\n"
        "    print('❌ Native jumperless module not available:', str(e))\n"
        "    print('Loading Python wrapper functions instead...')\n"
        "    print()  # Empty line\n"
    );
    
    mp_initialized = true;
    mp_repl_active = false;
    
    Serial.println("[MP] MicroPython initialized successfully");
    return true;
}

void deinitMicroPythonProper(void) {
    if (mp_initialized) {
        Serial.println("[MP] Deinitializing MicroPython...");
        mp_embed_deinit();
        mp_initialized = false;
        mp_repl_active = false;
    }
}

bool executePythonCodeProper(const char* code) {
    if (!mp_initialized) {
        Serial.println("[MP] Error: MicroPython not initialized");
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
        changeTerminalColor(replColors[4], true);
        Serial.println("[MP] Error: MicroPython not initialized");
        return;
    }
    
    if (mp_repl_active) {
        changeTerminalColor(replColors[4], true);
        Serial.println("[MP] REPL already active");
        return;
    }
    
    // Print Python prompt with color
    changeTerminalColor(replColors[1], true);
    Serial.print(">>> "); // Simple prompt - the processMicroPythonInput handles everything
    Serial.flush();
    
    mp_repl_active = true;
}

void stopMicroPythonREPL(void) {
    if (mp_repl_active) {
        changeTerminalColor(0, false);
        Serial.println("\n[MP] Exiting REPL...");
        mp_repl_active = false;
    }
}

bool isMicroPythonREPLActive(void) {
    return mp_repl_active;
}

void enterMicroPythonREPL(void) {
    // Colorful initialization like original implementation
    changeTerminalColor(replColors[6], true);
    
    // Initialize MicroPython if not already done
    if (!mp_initialized) {
        if (!initMicroPythonProper()) {
            changeTerminalColor(replColors[4], true); // error color
            Serial.println("Failed to initialize MicroPython!");
            return;
        }
        // Note: Jumperless native module is now automatically available via MP_REGISTER_MODULE
    // No need to manually add Python functions - the native 'jumperless' module is built-in
    }
    
    // Check if REPL is already active
    if (mp_repl_active) {
        changeTerminalColor(replColors[4], true);
        Serial.println("[MP] REPL already active");
        return;
    }
    
    // Show colorful welcome messages
    changeTerminalColor(replColors[7], true);
    Serial.println("MicroPython REPL with embedded Jumperless hardware control!");
    Serial.println("Type normal Python code, then press Enter to execute");
    Serial.println("Use TAB for indentation (or exactly 4 spaces)");
    Serial.println("Type help_jumperless() for hardware control commands");

    changeTerminalColor(replColors[0], true);
    Serial.println("MicroPython initialized successfully");
    Serial.flush();
    delay(200);

    changeTerminalColor(replColors[0], true);
    Serial.println();
    changeTerminalColor(replColors[2], true);
    Serial.println("    MicroPython REPL");
    
    // Show commands menu
    changeTerminalColor(replColors[5], true);
    Serial.println("\n Commands:");
    changeTerminalColor(replColors[3], false);
    Serial.print("  'quit' ");
    changeTerminalColor(replColors[0], false);
    Serial.println(" -   Exit REPL");
    changeTerminalColor(replColors[3], false);
    Serial.print("  'help' ");
    changeTerminalColor(replColors[0], false);
    Serial.println(" -   Show help");
    changeTerminalColor(replColors[3], false);
    Serial.print("  'jl.help()' ");
    changeTerminalColor(replColors[0], false);
    Serial.println(" - Show hardware commands");
    //changeTerminalColor(replColors[7], false);
    //Serial.println("\nPress Enter to start REPL");

    Serial.write(0x0E); // turn on interactive mode
    Serial.flush();
    
    // Wait for user to press enter
    changeTerminalColor(replColors[4], true);
    Serial.print("Press enter to start REPL");
    Serial.println();
    Serial.flush();
    while (Serial.available() == 0) {
        delay(1);
    }
    Serial.read(); // consume the enter keypress
    
    // Start the REPL with colors
    changeTerminalColor(replColors[1], true);
    startMicroPythonREPL();
    
    // Blocking loop - stay in REPL until user exits
    while (mp_repl_active) {
        processMicroPythonInput();
        delayMicroseconds(10); // Small delay to prevent overwhelming
    }
    
    // Cleanup with colors
    changeTerminalColor(replColors[0], true);
    Serial.println("\\nExiting REPL...");
    Serial.write(0x0F); // turn off interactive mode
    Serial.flush();
    changeTerminalColor(replColors[2], true);
    Serial.println("Returned to Arduino mode");
}


void processMicroPythonInput(void) {
    if (!mp_initialized) {
        return;
    }
    
    // Process any queued hardware commands
    if (mp_command_ready) {
        Serial.printf("[MP] Processing hardware command: %s\n", mp_command_buffer);
        
        // Execute the hardware command through your existing system
        char response[512];
        int result = parseAndExecutePythonCommand(mp_command_buffer, response);
        
        // Make result available to Python
        if (result == 0) {
            String python_result = "globals()['_last_result'] = '";
            python_result += response;
            python_result += "'";
            mp_embed_exec_str(python_result.c_str());
        }
        
        mp_command_ready = false;
    }
    
    // Handle REPL input if active - let MicroPython do all the work
    if (mp_repl_active) {
        // Simple approach: let MicroPython handle everything
        // Just execute a minimal REPL loop that processes input character by character
        static String current_input = "";
        static bool first_run = true;
        static bool in_multiline_mode = false;
        
        if (first_run) {
            // Start with a fresh prompt
            changeTerminalColor(replColors[1], true);
            
            Serial.flush();
            first_run = false;
        }
        // Check for available input
        if (Serial.available()) {
            int c = Serial.read();
            
            // Handle Ctrl+C (ASCII 3) - cancel current input and reset
            if (c == 3) {
                Serial.println("^C");
                Serial.println("KeyboardInterrupt");
                current_input = "";
                first_run = true;
                in_multiline_mode = false;
                changeTerminalColor(replColors[1], true);
                Serial.print(">>> ");
                Serial.flush();
                return;
            }
            
            // Handle Enter key - check for multiline or execute
            if (c == '\r' || c == '\n') {
                Serial.println(); // Echo newline
                
                // Check for exit commands
                String trimmed_input = current_input;
                trimmed_input.trim();
                if (trimmed_input == "exit()" || trimmed_input == "quit()" || 
                    trimmed_input == "exit" || trimmed_input == "quit") {
                    stopMicroPythonREPL();
                    current_input = "";
                    first_run = true;
                    in_multiline_mode = false;
                    return;
                }
                
                // Check if this is an empty line in multiline mode (escape mechanism)
                bool force_execution = false;
                if (in_multiline_mode && current_input.endsWith("\n")) {
                    // If we're in multiline and just pressed enter on what might be an empty line
                    String lines = current_input;
                    int last_newline = lines.lastIndexOf('\n', lines.length() - 2);
                    String last_line = "";
                    if (last_newline >= 0) {
                        last_line = lines.substring(last_newline + 1, lines.length() - 1);
                    } else {
                        last_line = lines.substring(0, lines.length() - 1);
                    }
                    last_line.trim();
                    if (last_line.length() == 0) {
                        force_execution = true; // Empty line in multiline mode
                    }
                }
                
                // Check if MicroPython needs more input (multiline detection)
                bool needs_more_input = false;
                if (current_input.length() > 0 && !force_execution) {
                    needs_more_input = mp_repl_continue_with_input(current_input.c_str());
                }
                
                if (needs_more_input && !force_execution) {
                    in_multiline_mode = true;
                    // Add newline to continue building multiline statement
                    current_input += "\n";
                    
                    // Smart auto-indent: maintain or increase indentation level
                    String lines = current_input;
                    int last_newline = lines.lastIndexOf('\n', lines.length() - 2); // -2 to skip the newline we just added
                    String last_line = "";
                    if (last_newline >= 0) {
                        last_line = lines.substring(last_newline + 1, lines.length() - 1); // exclude the newline we just added
                    } else {
                        last_line = lines.substring(0, lines.length() - 1); // first line, exclude newline
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
                    
                    // Add the indentation to the input
                    current_input += indent_spaces;
                    
                    // Show the prompt and indentation
                    changeTerminalColor(replColors[1], true);
                    Serial.print("... ");
                    if (indent_spaces.length() > 0) {
                        changeTerminalColor(replColors[3], false);
                        Serial.print(indent_spaces); // Show the indentation
                    }
                    Serial.flush();
                } else {
                    // Execute the complete statement (or force execution)
                    if (current_input.length() > 0) {
                        changeTerminalColor(replColors[2], true);
                        
                        // Clean up the input (remove trailing newlines)
                        String clean_input = current_input;
                        while (clean_input.endsWith("\n")) {
                            clean_input = clean_input.substring(0, clean_input.length() - 1);
                        }
                        
                        if (clean_input.length() > 0) {
                            // Let MicroPython handle the complete statement
                            mp_embed_exec_str(clean_input.c_str());
                        }
                        
                        changeTerminalColor(replColors[1], true);
                    }
                    
                    // Reset and show new prompt
                    current_input = "";
                    in_multiline_mode = false;
                    Serial.print(">>> ");
                    Serial.flush();
                }
                
            } else if (c == '\b' || c == 127) { // Backspace
                if (current_input.length() > 0) {
                    char last_char = current_input.charAt(current_input.length() - 1);
                    current_input.remove(current_input.length() - 1);
                    
                    if (last_char == '\n') {
                        // Backspacing over a newline
                        // Check if we're going back to single line mode
                        if (current_input.indexOf('\n') == -1) {
                            // No more newlines, back to single line mode
                            in_multiline_mode = false;
                        }
                        
                        // Find the last newline to get the current line content
                        int last_newline = current_input.lastIndexOf('\n');
                        String current_line = "";
                        if (last_newline >= 0) {
                            current_line = current_input.substring(last_newline + 1);
                        } else {
                            current_line = current_input;
                            in_multiline_mode = false; // Back to single line
                        }
                        
                        // Clear current line and redraw with appropriate prompt
                        Serial.print("\r"); // Go to start of line
                        if (in_multiline_mode) {
                            Serial.print("... "); // Continuation prompt
                        } else {
                            Serial.print(">>> "); // Primary prompt
                        }
                        changeTerminalColor(replColors[3], false);
                        Serial.print(current_line); // Current line content
                        Serial.flush();
                    } else {
                        // Normal character backspace
                        // Check if we're backspacing over indentation (4 spaces)
                        if (current_input.length() >= 4 && 
                            current_input.endsWith("    ")) {
                            // Remove 4 spaces at once (like a TAB)
                            current_input = current_input.substring(0, current_input.length() - 3); // Remove 3 more spaces (we already removed 1)
                            Serial.print("\b \b\b \b\b \b\b \b"); // Erase 4 characters visually
                            Serial.flush();
                        } else {
                            Serial.print("\b \b"); // Erase character visually
                            Serial.flush();
                        }
                    }
                }
            } else if (c == '\t') { // TAB character
                // Convert TAB to 4 spaces for Python indentation
                current_input += "    ";
                changeTerminalColor(replColors[3], false);
                Serial.print("    "); // Show 4 spaces
                Serial.flush();
            } else if (c >= 32 && c <= 126) { // Printable characters
                current_input += (char)c;
                // Echo the character with input color
                changeTerminalColor(replColors[3], false);
                Serial.write(c);
                Serial.flush();
            }
            // All other characters are ignored
        }
    }
}

// Helper function to add complete Jumperless hardware module
void addJumperlessPythonFunctions(void) {
    if (!mp_initialized) {
        return;
    }
    
    // Execute the complete embedded JythonModule from your original implementation
    const char* full_module = R"""(
_on_hardware = True

# Check if native jumperless module is available
try:
    import jumperless
    _native_available = True
    print('✅ Native jumperless module available')
except ImportError:
    _native_available = False
    print('⚠️  Native jumperless module not available - using software fallback')

# Global variables for synchronous execution
_sync_result_ready = False
_sync_value = ''
_sync_type = ''
_sync_result = 0

def _execute_sync(cmd):
    """Execute a command synchronously and return the actual result"""
    global _sync_result_ready, _sync_value, _sync_type, _sync_result
    _sync_result_ready = False
    print('SYNC_EXEC:' + cmd)
    # Wait briefly for C code to process
    try:
        import time
        time.sleep_ms(1)
    except:
        # Fallback: simple busy wait
        for i in range(1000):
            pass
    
    # Check multiple times with small delays
    for attempt in range(10):
        if _sync_result_ready:
            if _sync_type == 'bool':
                return _sync_value == 'True'
            elif _sync_type == 'float':
                return float(_sync_value)
            elif _sync_type == 'int':
                return int(_sync_value)
            elif _sync_type == 'error':
                raise RuntimeError(_sync_value)
            else:
                return _sync_value
        # Small delay between checks
        try:
            import time
            time.sleep_ms(1)
        except:
            for i in range(100):
                pass
    
    # If we get here, sync failed
    raise RuntimeError('Sync execution failed - timeout after 10 attempts')

def _execute_command(cmd):
    if _on_hardware:
        return _execute_sync(cmd)
    else:
        print('>COMMAND: ' + cmd)
        return '<SUCCESS: Command sent (external mode)'

class DAC:
    def set(self, channel, voltage, save=False):
        cmd = 'dac(set, ' + str(channel) + ', ' + str(voltage) + ', save=' + str(save) + ')'
        return _execute_command(cmd)
    def get(self, channel):
        cmd = 'dac(get, ' + str(channel) + ')'
        return _execute_command(cmd)

class ADC:
    def get(self, channel):
        cmd = 'adc(get, ' + str(channel) + ')'
        return _execute_command(cmd)
    def read(self, channel):
        return self.get(channel)

class GPIO:
    def __init__(self):
        self.HIGH = 'HIGH'
        self.LOW = 'LOW'
        self.OUTPUT = 'OUTPUT'
        self.INPUT = 'INPUT'
    def set(self, pin, value):
        if isinstance(value, bool):
            value = 'HIGH' if value else 'LOW'
        elif isinstance(value, int):
            value = 'HIGH' if value else 'LOW'
        cmd = 'gpio(set, ' + str(pin) + ', ' + str(value) + ')'
        return _execute_command(cmd)
    def get(self, pin):
        cmd = 'gpio(get, ' + str(pin) + ')'
        return _execute_command(cmd)
    def direction(self, pin, direction):
        cmd = 'gpio(direction, ' + str(pin) + ', ' + str(direction) + ')'
        return _execute_command(cmd)

class Nodes:
    def connect(self, node1, node2, save=False):
        cmd = 'nodes(connect, ' + str(node1) + ', ' + str(node2) + ', save=' + str(save) + ')'
        return _execute_command(cmd)
    def disconnect(self, node1, node2):
        cmd = 'nodes(remove, ' + str(node1) + ', ' + str(node2) + ')'
        return _execute_command(cmd)
    def remove(self, node1, node2):
        return self.disconnect(node1, node2)
    def clear(self):
        cmd = 'nodes(clear)'
        return _execute_command(cmd)

class OLED:
    def connect(self):
        cmd = 'oled(connect)'
        return _execute_command(cmd)
    def disconnect(self):
        cmd = 'oled(disconnect)'
        return _execute_command(cmd)
    def print(self, text, size=2):
        cmd = 'oled(print, "' + str(text) + '", ' + str(size) + ')'
        return _execute_command(cmd)
    def clear(self):
        cmd = 'oled(clear)'
        return _execute_command(cmd)
    def show(self):
        cmd = 'oled(show)'
        return _execute_command(cmd)

class Arduino:
    def reset(self):
        cmd = 'arduino(reset)'
        return _execute_command(cmd)
    def flash(self):
        cmd = 'arduino(flash)'
        return _execute_command(cmd)

class INA:
    def get_current(self, sensor):
        cmd = 'ina(getCurrent, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_voltage(self, sensor):
        cmd = 'ina(getVoltage, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_power(self, sensor):
        cmd = 'ina(getPower, ' + str(sensor) + ')'
        return _execute_command(cmd)

class Probe:
    def tap(self, node):
        cmd = 'probe(tap, ' + str(node) + ')'
        return _execute_command(cmd)
    def click(self, action='click'):
        cmd = 'probe(' + str(action) + ')'
        return _execute_command(cmd)

class Clickwheel:
    def up(self, clicks=1):
        cmd = 'clickwheel(up, ' + str(clicks) + ')'
        return _execute_command(cmd)
    def down(self, clicks=1):
        cmd = 'clickwheel(down, ' + str(clicks) + ')'
        return _execute_command(cmd)
    def press(self):
        cmd = 'clickwheel(press)'
        return _execute_command(cmd)

# Create module instances
dac = DAC()
adc = ADC()
ina = INA()
gpio = GPIO()
nodes = Nodes()
oled = OLED()
arduino = Arduino()
probe = Probe()
clickwheel = Clickwheel()

class JL:
    def __init__(self):
        self.dac = dac
        self.adc = adc
        self.ina = ina
        self.gpio = gpio
        self.nodes = nodes
        self.oled = oled
        self.arduino = arduino
        self.probe = probe
        self.clickwheel = clickwheel
    
    def read_voltage(self, channel):
        return self.adc.get(channel)
    
    def set_voltage(self, channel, voltage, save=False):
        return self.dac.set(channel, voltage, save)
    
    def connect_nodes(self, node1, node2, save=False):
        return self.nodes.connect(node1, node2, save)
    
    def display(self, text, size=2):
        return self.oled.print(text, size)
    
    def help(self):
        print('Jumperless MicroPython Module')
        print('Main Namespace: jl.*')
        print('Hardware Modules:')
        print('  jl.dac.set(0, 2.5)      # Set DAC voltage')
        print('  jl.adc.get(0)           # Read ADC')
        print('  jl.ina.get_current(0)   # Read INA current')
        print('  jl.gpio.set(5, "HIGH")  # Set GPIO')
        print('  jl.nodes.connect(1, 5)  # Connect nodes')
        print('  jl.oled.print("Hi")     # Display text')
        print('  jl.arduino.reset()      # Reset Arduino')
        print('  jl.clickwheel.up(1)     # Scroll up')
        print('  jl.probe.tap(5)         # Tap probe on node 5')
        print('')
        print('Convenience methods:')
        print('  jl.read_voltage(0)      # Quick ADC read')
        print('  jl.set_voltage(0, 2.5)  # Quick DAC set')
        print('  jl.connect_nodes(1, 5)  # Quick node connect')
        print('')
        return 'Help displayed'

jl = JL()

def help_jumperless():
    return jl.help()

def connect_nodes(node1, node2, save=False):
    return nodes.connect(node1, node2, save)

def set_voltage(channel, voltage, save=False):
    return dac.set(channel, voltage, save)

def read_voltage(channel):
    return adc.get(channel)

def display(text, size=2):
    return oled.print(text, size)

def reset_arduino():
    return arduino.reset()

// Simple helper functions
def connect(node1, node2):
    return nodes.connect(node1, node2)

def disconnect(node1, node2):
    return nodes.disconnect(node1, node2)

def voltage(channel, volts):
    return dac.set(channel, volts)

def measure(channel):
    return adc.get(channel)
)""";

    Serial.println("[MP] Loading complete Jumperless hardware module...");
    mp_embed_exec_str(full_module);
    Serial.println("[MP] Jumperless hardware module loaded successfully");
}

// Simple execution function for one-off commands
bool executePythonSimple(const char* code, char* response, size_t response_size) {
    if (!mp_initialized) {
        if (response) strncpy(response, "ERROR: MicroPython not initialized", response_size - 1);
        return false;
    }
    
    // Clear response buffer
    memset(mp_response_buffer, 0, sizeof(mp_response_buffer));
    
    // Execute and capture any output
    bool success = executePythonCodeProper(code);
    
    // Copy response if buffer provided
    if (response && response_size > 0) {
        if (success) {
            strncpy(response, "SUCCESS", response_size - 1);
        } else {
            strncpy(response, "ERROR: Execution failed", response_size - 1);
        }
        response[response_size - 1] = '\0';
    }
    
    return success;
}

// Status functions
bool isMicroPythonInitialized(void) {
    return mp_initialized;
}

void printMicroPythonStatus(void) {
    Serial.println("\n=== MicroPython Status ===");
    Serial.printf("Initialized: %s\n", mp_initialized ? "Yes" : "No");
    Serial.printf("REPL Active: %s\n", mp_repl_active ? "Yes" : "No");
    Serial.printf("Heap Size: %d bytes\n", sizeof(mp_heap));
    
    if (mp_initialized) {
        // Get memory info
        mp_embed_exec_str("import gc; print(f'Free: {gc.mem_free()}, Used: {gc.mem_alloc()}')");
    }
    Serial.println("=========================\n");
}

// Test function to verify the native Jumperless module is working
void testJumperlessNativeModule(void) {
    if (!mp_initialized) {
        Serial.println("[MP] Error: MicroPython not initialized for module test");
        return;
    }
    
    Serial.println("[MP] Testing native Jumperless module...");
    
    // Simple test to verify the module can be imported and functions are accessible
    const char* test_code = R"""(
try:
    import jumperless
    print("✅ Native jumperless module imported successfully")
    
    # Test that functions exist
    if hasattr(jumperless, 'dac_set') and hasattr(jumperless, 'adc_get'):
        print("✅ Core DAC/ADC functions found")
    else:
        print("❌ Core DAC/ADC functions missing")
        
    if hasattr(jumperless, 'nodes_connect') and hasattr(jumperless, 'gpio_set'):
        print("✅ Node and GPIO functions found")
    else:
        print("❌ Node and GPIO functions missing")
        
    if hasattr(jumperless, 'oled_print') and hasattr(jumperless, 'ina_get_current'):
        print("✅ OLED and INA functions found")
    else:
        print("❌ OLED and INA functions missing")
        
    print("✅ Native Jumperless module test completed successfully")
    
except ImportError as e:
    print("❌ Failed to import native jumperless module:", str(e))
except Exception as e:
    print("❌ Error testing native jumperless module:", str(e))
)""";

    Serial.println("[MP] Executing native module test...");
    mp_embed_exec_str(test_code);
    Serial.println("[MP] Native module test complete");
} 