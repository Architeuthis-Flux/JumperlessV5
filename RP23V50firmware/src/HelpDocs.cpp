#include "HelpDocs.h"
#include "Graphics.h"
#include "configManager.h"
#include <string.h>

// Color definitions for help text formatting
const int HELP_TITLE_COLOR = 51;      // Cyan
const int HELP_COMMAND_COLOR = 221;   // Yellow
const int HELP_DESC_COLOR = 207;      // Magenta
const int HELP_USAGE_COLOR = 69;      // Blue
const int HELP_NOTE_COLOR = 202;      // Orange/Red
const int HELP_NORMAL_COLOR = 38;     // Green

bool isHelpRequest(const char* input) {
    if (!input) return false;
    
    // Check for general help commands
    if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
        return true;
    }
    
    // Check for category help (help <category>)
    if (strncmp(input, "help ", 5) == 0 && strlen(input) > 5) {
        return true;
    }
    
    // Check for command-specific help (command followed by ?)
    int len = strlen(input);
    if (len == 2 && input[1] == '?') {
        return true;
    }
    
    return false;
}

bool handleHelpRequest(const char* input) {
    if (!isHelpRequest(input)) {
        return false;
    }
    
    if (strcmp(input, "help") == 0 || strcmp(input, "h") == 0) {
        showGeneralHelp();
        return true;
    }
    
    // Check for "help <category>" format
    if (strncmp(input, "help ", 5) == 0) {
        const char* category = input + 5; // Skip "help "
        showCategoryHelp(category);
        return true;
    }
    
    if (strlen(input) == 2 && input[1] == '?') {
        showCommandHelp(input[0]);
        return true;
    }
    
    return false;
}

void showGeneralHelp() {
    changeTerminalColor(HELP_TITLE_COLOR, true);
    Serial.println("\n╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                          JUMPERLESS HELP SYSTEM                           │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    
    changeTerminalColor(HELP_DESC_COLOR, true);
    Serial.println("Type any command followed by ? for detailed help (like 'f?' or 'n?')");
    Serial.println("Type 'help <category>' for section-specific help\n");
    changeTerminalColor(HELP_NOTE_COLOR, true);
    Serial.println("  This help system is partially AI generated, so it may contain bullshit");
    Serial.println("  When I make absolutely sure everything is accurate, I'll remove this message\n\r");
    
    // // ASCII art probe
    // changeTerminalColor(HELP_COMMAND_COLOR, true);
    // Serial.println("                    ╭─────────────╮");
    // Serial.println("                    │  THE PROBE  │  ← Your magic wand!");
    // Serial.println("                    ╰─────────────╯");
    // changeTerminalColor(HELP_DESC_COLOR, true);
    // Serial.println("              ●──────────────────────────○ Touch & Click");
    // Serial.println("           Connect               Remove\n");
    
    changeTerminalColor(HELP_TITLE_COLOR, true);
    Serial.println(" HELP CATEGORIES - Type 'help <category>' for details:");
    Serial.println();
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" basics");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("     - Essential commands you'll use every day");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" probe");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("      - How to use the probe for connecting/removing");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" voltage");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("    - Power, measurement, and analog signals");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" arduino");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("    - Arduino integration and UART connections");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" python");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("     - MicroPython REPL, scripts, and hardware control");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" apps");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("       - Built-in applications and utilities");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" display");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("    - OLED display and LED control");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" slots");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("      - Save and load different circuit configurations");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" scripts");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("    - Python script management and examples");
    
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.print(" debug");
    changeTerminalColor(HELP_DESC_COLOR, false);
    Serial.println("      - Troubleshooting and technical internals");
    
            changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(" config");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("     - Configuration file and persistent settings");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(" advanced");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("   - Advanced commands and technical features");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(" glossary");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("   - Definitions of nets, nodes, bridges, and more");
    
    Serial.println();
    changeTerminalColor(HELP_USAGE_COLOR, true);
    Serial.println(" QUICK START:");
    Serial.println("  1. Press probe Connect button (turns blue)");
    Serial.println("  2. Touch two points to connect them");
    Serial.println("  3. Type 'f' to load a full connection file");
    Serial.println("  4. Type 'n' to see what's connected");
    Serial.println("  5. Type 'p' to enter MicroPython REPL");
    
    changeTerminalColor(HELP_NORMAL_COLOR, true);
    Serial.println();
}

void showCommandHelp(char command) {
    changeTerminalColor(HELP_TITLE_COLOR, true);
    Serial.print("\nHelp for command: ");
    changeTerminalColor(HELP_COMMAND_COLOR, false);
    Serial.println(command);
    Serial.println();
    
    switch (command) {
        case 'f':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Load a full set of connections");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: f");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Then type connections like:");
            Serial.println("  f 1-5 (connects breadboard holes 1 and 5)");
            Serial.println("  f D2-A3 (connects Arduino D2 to A3)");
            Serial.println("  f GND-30 (connects ground rail to hole 30)");
            Serial.println("  f 1-5,7-12,D2-A3 (multiple connections at once)");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            // Serial.println("This is the main way to wire up your circuit!");
            break;
            
        case '+':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Add connections to your current setup");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: +");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Then type new connections like: + 1-5, D2-A3");
            Serial.println("This adds to existing connections without clearing them.");
            break;
            
        case '-':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Remove specific connections");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: -");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Then type connections to remove like: - 1-5, D2-A3");
            Serial.println("Only removes the connections you specify.");
            break;
            
        case 'x':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Clear ALL connections - nuclear option!");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: x");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("This removes everything in the current netlist.");
            break;
            
        case 'n':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show current connections (netlist)");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: n");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows all active connections in a nice list.");
            // Serial.println("Great for seeing what's currently wired up.");
            break;
            
        case '^':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Set DAC voltage output");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: ^3.3  (sets DAC 1 voltage to 3.3V)");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("DAC features:");
            Serial.println("  - High precision 12-bit output");
            Serial.println("  - Multiple DAC channels available");
            Serial.println("  - Range: -8V to +8V");
            Serial.println("  - Also available via Python: jumperless.set_dac(0, 3.3)");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            // Serial.println("Perfect for testing circuits with precise known voltages!");
            break;
            
        case 'v':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Read voltages and currents with high precision");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage:");
            Serial.println("  v       - show all ADC readings");
            Serial.println("  v0-v4   - show specific ADC (0-4)");
            Serial.println("  vi      - show current sensor readings");
            Serial.println("  vi1     - show current sensor 1");
            Serial.println("  vl      - toggle live readings display");
            //Serial.println("  vp      - read probe voltage");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Features:");
            Serial.println("  - High resolution 12-bit ADC readings");
            Serial.println("  - Real-time monitoring capabilities");
            Serial.println("  - Python access: jumperless.get_adc(0)");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            // Serial.println("Perfect for precision circuit debugging and monitoring!");
            break;
            
        case 'A':
        case 'a':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Connect or disconnect Jumperless Routable UART to Arduino D0 and D1 pins");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: A to connect, a to disconnect");
            changeTerminalColor(HELP_DESC_COLOR, true);
            //Serial.println("This lets you program and communicate with an Arduino.");
            Serial.println("Add '?' to check connection status: A?");
            break;
            
        
            // changeTerminalColor(HELP_DESC_COLOR, true);
            // Serial.println("Disconnect UART from Arduino");
            // changeTerminalColor(HELP_USAGE_COLOR, true);
            // Serial.println("Usage: a");
            // changeTerminalColor(HELP_DESC_COLOR, true);
            // Serial.println("Breaks the connection so you can use D0/D1 for other stuff.");
            // break;
            
        case 'r':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Reset Arduino or Jumperless");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage:");
            Serial.println("  r   - reset both Arduino Reset Pins");
            Serial.println("  rt  - reset top Arduino Reset Pin only");
            Serial.println("  rb  - reset bottom Arduino Reset Pin only");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Sometimes you just need to turn it off and on again.");
            break;
            
        case 'p':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Start MicroPython REPL with full scripting support!");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: p");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("REPL features:");
            Serial.println("  - Command history (up/down arrows)");
            Serial.println("  - Multi-line input with smart indentation");
            Serial.println("  - Script save/load functionality");
            Serial.println("  - File management and eKilo editor integration");
            Serial.println("\nHardware control functions:");
            Serial.println("  - jumperless.connect(1, 5)   - Make connections");
            Serial.println("  - jumperless.remove(1, 5)    - Remove connections");
            Serial.println("  - jumperless.set_gpio(2, 1)  - Digital I/O");
            Serial.println("  - jumperless.get_adc(0)      - Read voltages");
            Serial.println("  - jumperless.run_app('i2c')  - Run built-in apps");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("This is where the real magic happens - full Python control!");
            break;
            
        case '>':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Execute a single Python command");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: >");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Then type a Python command like:");
            Serial.println("  jumperless.connect(1, 5)");
            Serial.println("  jumperless.set_gpio(2, 1)");
            Serial.println("  jumperless.run_app('i2c')");
            Serial.println("  print(jumperless.get_adc(0))");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Quick way to run commands without entering full REPL.");
            break;
            
        case 'P':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show all connectable nodes and Python capabilities");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: P");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Displays node reference including:");
            Serial.println("  - All breadboard holes (1-60)");
            Serial.println("  - Arduino pins (D0-D13, A0-A5)");
            Serial.println("  - Power rails (GND, +5V, +3.3V)");
            Serial.println("  - GPIO pins and special functions");
            Serial.println("  - Available jumperless Python commands");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Essential reference for Python scripting and connections!");
            break;
            
        case '.':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Connect and initialize the I2C OLED display");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: .");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggles the little screen on/off.");
            Serial.println("Note: connecting GPIO to the OLED is not the same as actually initializing the I2C display, this will do both");
            break;
            
        case 'l':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("LED brightness control and test pattern menu");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: l");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Opens a menu to:");
            Serial.println("  - Adjust LED brightness");
            Serial.println("  - Run test patterns");
            Serial.println("  - Check if LEDs are working");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("If your connections look dim, crank up the brightness");
            break;
            
        case '\'':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show the startup animation");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: '");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Replays that cool swirly animation you saw when it booted up.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Pure eye candy, but hey, we all need some joy in debugging.");
            break;
            
        case '<':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Cycle through saved configuration slots");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: <");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Jumperless has multiple slots to save different circuit configs.");
            Serial.println("This cycles backwards through them (use 'o' to pick specific ones).");
            break;
            
        case 'o':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Load a specific configuration slot");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: o");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows a menu of saved configurations you can load.");
            Serial.println("Each slot can hold a complete circuit setup.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Great for switching between different projects!");
            break;
            
        case 's':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show all saved slot files");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: s");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Lists all your saved configurations with their slot numbers.");
            Serial.println("You can copy and paste this output to reload it later");
            break;
            
        case 'b':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show bridge array and routing paths - the technical guts");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage:");
            Serial.println("  b   - show everything");
            Serial.println("  b0  - hide duplicates"); 
            Serial.println("  b2  - show extra details");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("This shows how Jumperless actually routes your connections");
            //Serial.println("through the internal crossbar switches. Very technical!");
            break;
            
        case 'c':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show crossbar chip connection status");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: c");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows the state of all the internal switching chips.");
            //Serial.println("Useful for debugging weird connection issues.");
            break;
            
        case 'd':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Debug flags menu - for troubleshooting");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: d");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Opens a menu to enable/disable various debug outputs:");
            Serial.println("  - File parsing debug");
            Serial.println("  - Connection manager debug");
            Serial.println("  - LED debug info");
            Serial.println("  - Serial passthrough options");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Use d0-d9 to directly toggle specific debug categories.");
            break;
            
        case '?':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show firmware version");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: ?");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Tells you what version of Jumperless firmware you're running.");
            //Serial.println("Helpful when reporting bugs or checking for updates.");
            break;
            
        case '@':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("I2C device scanner with flexible row targeting");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage:");
            Serial.println("  @           - Interactive mode (prompts for SDA/SCL rows)");
            Serial.println("  @5,10       - Scan with SDA on row 5, SCL on row 10");
            Serial.println("  @5          - Auto-try 4 combinations around row 5:");
            Serial.println("                SDA=5 SCL=6, SDA=6 SCL=5, SDA=5 SCL=4, SDA=4 SCL=5");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("I2C scanning features:");
            Serial.println("  - Comprehensive device detection");
            Serial.println("  - Flexible pin assignment");
            Serial.println("  - Auto-discovery mode for unknown wiring");
            Serial.println("  - Detailed device information with addresses");
            Serial.println("  - Also available as app: jumperless.run_app('i2c')");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            //Serial.println("Perfect for finding I2C devices when you're not sure of the wiring!");
            break;
            
        case '$':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Calibrate the DAC outputs");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: $");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Runs a calibration routine to make sure DAC voltages are accurate.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Do this occasionally, especially if voltages seem off.");
            break;
            
        case 'g':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Print current GPIO pin states");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: g");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows the current state of all GPIO pins.");
            //Serial.println("Helpful for debugging digital circuits.");
            break;
            
        case '#':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Print text from menu to LED display");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: #");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Interactive mode - follow prompts to display text on the LEDs.");
            Serial.println("Because sometimes you want your breadboard to say things.");
            break;
            
        case '~':
            // Use the comprehensive config help system
            printConfigHelp();
            break;
            
        case '`':
            // Use the comprehensive config help system
            printConfigHelp();
            break;
            
        //case 'E':
            // changeTerminalColor(HELP_DESC_COLOR, true);
            // Serial.println("Toggle extra menu options display");
            // changeTerminalColor(HELP_USAGE_COLOR, true);
            // Serial.println("Usage: E");
            // changeTerminalColor(HELP_DESC_COLOR, true);
            // Serial.println("Hides/shows the extra commands in the main menu.");
            // Serial.println("Makes the menu less cluttered if you don't need technical stuff.");
            // break;
            
        case 'e':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show/hide extra menu options");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: e");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggles display of extra commands in the menu.");
            break;
            
        case 'F':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Cycle through available OLED fonts");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: F");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Changes the font used on the OLED display.");
            //Serial.println("Because comic sans is never the answer.");
            break;
            
        case '=':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Dump OLED frame buffer contents");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: =");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows the raw pixel data from the OLED display.");
            //Serial.println("Very technical - mainly for display debugging.");
            break;
            
        case 'k':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggle OLED display in terminal");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: k");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows/hides a text version of the OLED display in your terminal.");
            //Serial.println("Handy when you can't see the physical display clearly.");
            break;
            
        case 'R':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show board LEDs in terminal (dump LED states)");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: R");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggles a visual representation of the LED array in your terminal.");
            Serial.println("This can make a mess of the terminal, but it's rad");
            break;
            
        case '_':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Print timing statistics (microseconds per byte)");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: _");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows performance statistics for data processing.");
            Serial.println("Mainly useful for debugging Arduino Serial passthrough");
            break;

        // case '&':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Load changed net colors from file");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: &");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Reloads the color configuration for current slot.");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("Advanced command for color management and debugging.");
        //     break;
            
        case 'U':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Enter USB Mass Storage mode");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: U");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Activates USB Mass Storage Device mode:");
            Serial.println("  - Jumperless appears as a removable USB drive");
            Serial.println("  - Edit files directly from your computer's file manager");
            Serial.println("  - Access Python scripts, config files, and node files");
            Serial.println("  - Jumperless becomes unresponsive during file editing");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("SAFETY: Always safely eject the drive before unplugging!");
            break;
            
        case 'u':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Exit USB Mass Storage mode");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: u");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Deactivates USB Mass Storage mode and returns to normal operation.");
            Serial.println("Alternatively, safely eject the drive from your computer.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Always exit USB mode properly to prevent file corruption!");
            break;

        case 'm':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Show the main menu");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: m");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Displays the command menu again if it scrolled off screen.");
           // Serial.println("Your lifeline when you forget what commands are available.");
            break;
            
        case '!':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Print the current node file contents");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: !");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows the raw node file data for the current slot.");
           // Serial.println("Technical details about how connections are stored.");
            break;
            
        case 'G':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Reload config.txt changes (for USB Mass Storage mode)");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: G");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Reloads the configuration file without restarting the device.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Use this after editing config.txt via USB Mass Storage mode.");
            break;

        // case 'j':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Internal navigation command");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: j");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Internal command for menu navigation - typically not used directly.");
        //     break;

        case 'Z':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("USB Mass Storage Debug Control menu");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: Z");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Opens advanced USB debugging menu with options:");
            Serial.println("  1. Toggle USB debug mode");
            Serial.println("  2. Manual refresh from USB");
            Serial.println("  3. Validate all slot files");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("For troubleshooting USB Mass Storage issues.");
            break;

        case '/':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("File Manager application");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: /");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Launches the built-in file manager for:");
            Serial.println("  - Browsing files and directories");
            Serial.println("  - Creating, editing, and deleting files");
            Serial.println("  - Managing Python scripts and config files");
            Serial.println("  - Viewing file contents and information");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Use file manager commands: h(help), v(view), e(edit), n(new), d(delete)");
            break;

        case 'C':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggle terminal colors");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: C");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Enables or disables colored terminal output.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Useful for terminal clients that don't support ANSI colors.");
            break;

        case 'E':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Toggle menu display");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: E");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Shows or hides the main menu after each command.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Different from 'e' - this controls menu visibility entirely.");
            break;

        // case 'i':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Initialize OLED display");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: i");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Manually initialize the OLED display if not already connected.");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("Usually not needed - the '.' command handles both connection and init.");
        //     break;

        // case '{':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Probe mode - explore connections");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: { (or press the probe button)");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Touch the probe to any point to see what it's connected to.");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("The probe is amazing for tracing circuits and debugging!");
        //     break;

        // case '}':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Probe mode - make connections");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: } (or long-press the probe button)");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Touch two points with the probe to connect them.");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("Super intuitive way to wire up your circuit!");
        //     break;

        case 'w':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Wave generator function (don't use, it's a janky pile of garbage)");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: w");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Generates various waveforms on DAC outputs.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("If wave generation fails, falls back to slot selection menu.");
            break;

        case 'y':
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Refresh connections and load files");
            changeTerminalColor(HELP_USAGE_COLOR, true);
            Serial.println("Usage: y");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Refreshes all connections and loads any file changes.");
            Serial.println("If USB Mass Storage is mounted, performs manual USB refresh.");
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.println("Use after editing files via USB Mass Storage mode.");
            break;

        // case 't':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Test MSC callbacks (disabled)");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: t");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Internal test function - currently disabled.");
        //     break;

        // case 'T':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Show detailed netlist information");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: T");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Displays comprehensive netlist details including:");
        //     Serial.println("  - Complete connection information");
        //     Serial.println("  - Bridge array data");
        //     Serial.println("  - Path routing information");
        //     Serial.println("  - Special nets and technical internals");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("More detailed than 'n' - shows technical implementation details.");
        //     break;

        // case ':':
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Machine mode activation");
        //     changeTerminalColor(HELP_USAGE_COLOR, true);
        //     Serial.println("Usage: :: (type colon twice)");
        //     changeTerminalColor(HELP_DESC_COLOR, true);
        //     Serial.println("Enters machine mode for automated control.");
        //     changeTerminalColor(HELP_NOTE_COLOR, true);
        //     Serial.println("Advanced feature for programmatic control of Jumperless.");
        //     break;

        default:
            changeTerminalColor(HELP_NOTE_COLOR, true);
            Serial.print("No specific help available for command '");
            Serial.print(command);
            Serial.println("'");
            changeTerminalColor(HELP_DESC_COLOR, true);
            Serial.println("Try 'help' for a list of all available commands.");
            break;
    }
    
    changeTerminalColor(HELP_NORMAL_COLOR, true);
    Serial.println();
}

void showCategoryHelp(const char* category) {
    changeTerminalColor(HELP_TITLE_COLOR, true);
    Serial.print("\n╭───────────────────────────────────────────────────────────────────────────╮\n");
    
    // Center the category text in the header
    String headerText = String(category) + " HELP";
    int totalWidth = 74; // Available space between │ characters
    int textLen = headerText.length();
    int leftPadding = (totalWidth - textLen) / 2;
    int rightPadding = totalWidth - textLen - leftPadding;
    
    Serial.print("│");
    for (int i = 0; i < leftPadding; i++) {
        Serial.print(" ");
    }
    Serial.print(headerText);
    for (int i = 0; i < rightPadding; i++) {
        Serial.print(" ");
    }
    Serial.print("│\n");
    Serial.print("╰───────────────────────────────────────────────────────────────────────────╯\n");
    
    if (strcmp(category, "basics") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Essential commands you'll use every day:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("f  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Load a full set of connections (main wiring command)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("+  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Add connections to existing setup");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("-  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Remove specific connections");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("x  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Clear ALL connections (nuclear option!)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("n  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show current connections (netlist)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("m  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show main menu again");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Connection format examples:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1-5          (breadboard holes)");
        Serial.println("  D2-A3        (Arduino pins)");
        Serial.println("  GND-30       (rail to hole)");
        Serial.println("  1-5,7-12     (multiple connections)");
        
    } else if (strcmp(category, "probe") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
       // Serial.println("Master the probe - your circuit's best friend!\n");
        
        // ASCII art probe diagram
        changeTerminalColor(HELP_COMMAND_COLOR, true);


char probe_art[] = R"""(
                     Select        Measure                     
                 .━━━━━━━━━.▁█▁▁▁▁▁.━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━.
           ▁▁.━━' Connect   ╭────────────────        ───────────────╮   \
  ───╼━━━━{       Remove    │Connect                      Remove    │    ┃
           ▔▔`━━. Measure   ╰────────────────        ───────────────╯   /
                 `━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━'
                                               
)""";
Serial.println(probe_art);




        // Old ASCII art replaced with modern probe diagram above
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println(" CONNECT MODE (Blue):");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1. Press Connect button (turns blue)");
        Serial.println("  2. Touch first point - probe 'holds' it");
        Serial.println("  3. Touch second point - creates connection");
        Serial.println("  4. Repeat for more connections");
        Serial.println("  5. Press Connect again to exit");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n REMOVE MODE (Red):");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1. Press Remove button (turns red)");
        Serial.println("  2. Touch any connected point");
        Serial.println("  3. That connection gets removed");
        Serial.println("  4. Press Remove again to exit");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n IMPORTANT - Switch Position:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Keep switch on 'SELECT' mode for best results");
        Serial.println("  'MEASURE' mode is experimental and flaky");
        
        // changeTerminalColor(HELP_USAGE_COLOR, true);
        // Serial.println("\n KEYBOARD SHORTCUTS:");
        // changeTerminalColor(HELP_DESC_COLOR, true);
        // Serial.println("  {  - Probe explore mode (same as pressing probe button)");
        // Serial.println("  }  - Probe connect mode (same as long-pressing probe button)");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Special Functions:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Tap the pads near the logo for:");
        Serial.println("  - GPIO pins (programmable digital I/O)");
        Serial.println("  - ADC inputs (read voltages)");
        Serial.println("  - DAC outputs (generate voltages)");
     
        
    } else if (strcmp(category, "voltage") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Work with power, signals, and measurements:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("^  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Set DAC voltage output (^3.3 for 3.3V)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("v  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Read ADC voltages and currents");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("$  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Calibrate DACs (run this occasionally)");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Voltage Reading Options:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  v     - All ADC readings");
        Serial.println("  v0-v4 - Specific ADC channel");
        Serial.println("  vi    - Current sensor readings");
        Serial.println("  vi1   - Current sensor 1 only");
        Serial.println("  vl    - Toggle live readings");
        Serial.println("  vp    - Read probe voltage");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n PWM Signal Generation (Python):");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  jumperless.pwm(1, 1000, 0.5)           - 1kHz PWM on GPIO_1, 50% duty");
        Serial.println("  jumperless.pwm(2, 0.1, 0.25)           - 0.1Hz slow PWM on GPIO_2, 25% duty");
        Serial.println("  jumperless.pwm_set_frequency(1, 500)   - Change frequency to 500Hz");
        Serial.println("  jumperless.pwm_set_duty_cycle(1, 0.75) - Change duty cycle to 75%");
        Serial.println("  jumperless.pwm_stop(GPIO_1)            - Stop PWM on GPIO_1");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n PWM Frequency Ranges:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Hardware PWM: 10Hz to 62.5MHz (high precision)");
        Serial.println("  Slow PWM: 0.001Hz to 10Hz (hardware timer based)");
        Serial.println("  Automatic mode selection based on frequency");
        Serial.println("  Ultra-slow PWM: 0.001Hz = 1000 second period!");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Animated Voltage Display:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Green (0V) → Red (5V) → Pink (8V+)");
        Serial.println("  Blue/icy colors for negative voltages");
        Serial.println("  Rails pulse toward top/bottom");
        
    } else if (strcmp(category, "arduino") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Arduino integration and UART connections:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("A  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Connect UART to Arduino D0/D1");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("a  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Disconnect UART from Arduino");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("r  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Reset Arduino (rt=top, rb=bottom)");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n UART Passthrough:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  When UART is connected:");
        Serial.println("  - Second serial port appears");
        Serial.println("  - Arduino IDE can flash directly");
        Serial.println("  - Serial Monitor works normally");
        Serial.println("  - Auto-detects upload attempts");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Auto-flashing Magic:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Jumperless detects when Arduino IDE uploads");
        Serial.println("  Automatically handles reset timing");
        Serial.println("  Works with just one USB cable!");
        
    } else if (strcmp(category, "python") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("MicroPython REPL, scripting, and hardware control:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("p  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Start full MicroPython REPL with history");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(">  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Execute single Python command");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("P  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Python command mode / show all nodes");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Hardware Control:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  jumperless.connect(1, 5)    - Make connections");
        Serial.println("  jumperless.remove(1, 5)     - Remove connections");
        Serial.println("  jumperless.netlist()        - Show connections");
        Serial.println("  jumperless.clear_all()      - Clear all connections");
        Serial.println("  jumperless.set_gpio(2, 1)   - Digital output");
        Serial.println("  jumperless.get_adc(0)       - Read voltage");
        Serial.println("  jumperless.set_dac(0, 3.3)  - Set voltage");
        Serial.println("  jumperless.pwm(1, 1000, 0.5) - PWM output (1kHz, 50% duty)");
        Serial.println("  jumperless.run_app('i2c')   - Run built-in apps");
        Serial.println("  jumperless.pause_core2()    - Pause core2 processing");
        Serial.println("  jumperless.send_raw(data)   - Send raw data to core2");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n REPL Features:");
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Wave generator:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  wavegen_set_output(DAC1)           - Output on DAC1 (aliases: TOP_RAIL, BOTTOM_RAIL, DAC0)");
        Serial.println("  wavegen_set_freq(100.0)            - Frequency in Hz (0.0001 to 10000)");
        Serial.println("  wavegen_set_wave(SINE)             - Waveform: SINE, TRIANGLE, RAMP, SQUARE (ARBITRARY later)");
        Serial.println("  wavegen_set_amplitude(3.3)         - Amplitude in Vpp (0.0 to 16.0)");
        Serial.println("  wavegen_set_offset(1.65)           - DC offset in Volts (-8.0 to +8.0)");
        Serial.println("  wavegen_set_sweep(10, 1000, 2.0)   - Sweep start/end Hz over N seconds (config only)");
        Serial.println("  wavegen_start(True)                - Start output (default True if no arg)");
        Serial.println("  wavegen_stop()                     - Stop output");
        Serial.println("  Note: You can change frequency/wave/amplitude/offset while running.");
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("  IMPORTANT: Wavegen runs on core2 and is fully blocking while active.");
        Serial.println("  LEDs and routing updates will pause until wavegen_stop() is called.");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Command history (up/down arrows)");
        Serial.println("  - Multi-line input support");
        Serial.println("  - Smart indentation");
        Serial.println("  - Script save/load functionality");
        Serial.println("  - Tab completion for functions");
        Serial.println("  - File manager and eKilo editor integration");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n REPL Commands:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  run     - Execute code buffer");
        Serial.println("  clear   - Clear input buffer");
        Serial.println("  quit    - Exit REPL");
        Serial.println("  help    - Show REPL help");
        Serial.println("  save    - Save current session as script");
        Serial.println("  load    - Load and run a saved script");
        Serial.println("  new     - Create new script with eKilo editor");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Python Features:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Full MicroPython standard library");
        Serial.println("  - Filesystem access for script storage");
        Serial.println("  - Real-time hardware interaction");
        Serial.println("  - Complete Jumperless module with all functions");
        
    } else if (strcmp(category, "apps") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Built-in applications and utilities:\n");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println(" Access Apps:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Navigate to Apps menu");
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  - Or run from Python: ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("jumperless.run_app('appname')");
        
        changeTerminalColor(HELP_COMMAND_COLOR, true);
        Serial.println("\n Available Apps:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  i2c        ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- I2C device scanner");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  scope      ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Simple oscilloscope functionality");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  calibrate  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- DAC/ADC calibration utility");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  custom     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Custom user application");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  python     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- MicroPython REPL (same as 'p' command)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  /          ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- File Manager (browse/edit files)");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n App Examples:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  Connect I2C device, run i2c app to find address");
        Serial.println("  Use scope app to visualize signals on ADC pins");
        Serial.println("  Run calibrate app if voltages seem off");
        Serial.println("  Use / to browse and edit files directly");
        
    } else if (strcmp(category, "scripts") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Python script management and examples:\n");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println(" Script Management in REPL:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  save                    - Save current session");
        Serial.println("  save 'filename'         - Save with specific name");
        Serial.println("  load 'filename'         - Load and run script");
        Serial.println("  list                    - Show saved scripts");
        Serial.println("  delete 'filename'       - Remove script");
        
        changeTerminalColor(HELP_COMMAND_COLOR, true);
        Serial.println("\n Examples Available:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  jumperless_demo.py        ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Basic hardware demo");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  external_python_control.py ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- External control examples");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  sync_demo.py             ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Synchronization examples");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("  custom_boolean_types.py  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Custom data types");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Script Features:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Automatic numbering for easy loading");
        Serial.println("  - History persistence across reboots");
        Serial.println("  - Full filesystem access");
        Serial.println("  - Error handling and debugging");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Getting Started:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1. Enter REPL with 'p'");
        Serial.println("  2. Write some code");
        Serial.println("  3. Type 'save' to store it");
        Serial.println("  4. Type 'load 1' to run script #1");
        Serial.println("  5. Check examples/ directory for inspiration");
        
    } else if (strcmp(category, "display") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("OLED display and LED control:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(".  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Connect/disconnect OLED display");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("l  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- LED brightness and test patterns");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("'  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show startup animation");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("F  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Cycle OLED fonts");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("k  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Toggle OLED display in terminal");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("R  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show board LEDs in terminal");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n OLED Setup:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1. Get 128x32 SSD1306 OLED display");
        Serial.println("  2. Friction fit into SBC board");
        Serial.println("  3. Type '.' to connect data lines");
        Serial.println("  4. Auto-disconnects if not found");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n LED Features:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Connections show as colored lines");
        Serial.println("  - Voltages animate with color coding");
        Serial.println("  - GPIO states show red/green");
        Serial.println("  - Custom colors per slot saved");
        
    } else if (strcmp(category, "slots") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Save and load different circuit configurations:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("<  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Cycle through saved slots (backwards)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("o  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Load specific slot (shows menu)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("s  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show all saved slot files");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Slot System:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - 8 slots by default (0-7)");
        Serial.println("  - Each slot saves connections");
        Serial.println("  - Custom colors saved per slot");
        Serial.println("  - Switch projects instantly");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Pro Tips:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Use different slots for different projects");
        Serial.println("  - Slot files are just text (copy/paste friendly)");
        Serial.println("  - Colors and settings persist across reboots");
        
    } else if (strcmp(category, "debug") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Troubleshooting and technical internals:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("b  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Bridge array and routing paths");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("c  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Crossbar chip connection status");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("d  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Debug flags menu (d0-d9 for specific flags)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("?  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show firmware version");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("g  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Print GPIO states");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("T  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Detailed netlist information");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("Z  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- USB debug control menu");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("!  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Print raw node file contents");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n When Things Go Wrong:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  1. Check 'n' (netlist) for connections");
        Serial.println("  2. Use 'b' to see routing internals");
        Serial.println("  3. Try 'T' for detailed technical info");
        Serial.println("  4. Enable debug flags with 'd' menu");
        Serial.println("  5. Message me on Discord or wherever");
        
    } else if (strcmp(category, "config") == 0) {
        // Use the existing detailed config help system
        printConfigHelp();
        
    } else if (strcmp(category, "advanced") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Advanced commands and technical features:");
        Serial.println("A lot of these are for my own debugging, but you can use them if you want.\n\r");
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("G  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Reload config.txt changes");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("C  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Toggle terminal colors");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("E  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Toggle menu display");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("i  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Initialize OLED display");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("w  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Wave generator function");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("y  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Refresh connections and load files");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("t  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Test MSC callbacks (disabled)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print(":: ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Machine mode activation");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("m  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Show main menu");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("j  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Internal navigation command");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("&  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Load changed net colors from file");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("_  ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Print timing statistics");
        
        changeTerminalColor(HELP_USAGE_COLOR, true);
        Serial.println("\n Advanced Python Functions:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  jumperless.pause_core2()    - Pause core2 processing");
        Serial.println("  jumperless.send_raw(data)   - Send raw data to core2");
        Serial.println("  jumperless.pwm(1, 0.001, 0.5) - Ultra-slow PWM (0.001Hz)");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Advanced Features:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Most users won't need these commands");
        Serial.println("  - Some are for internal system operation");
        Serial.println("  - Others are for power users and debugging");
        
    } else if (strcmp(category, "glossary") == 0) {
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("Definitions of key terms:\n");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("net      ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Group of all nodes connected together");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("node     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Any connectable point (breadboard, pins, GPIO, etc)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("row      ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Breadboard column (I know it's wrong) or nano header pin");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("rail     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Power rails (top, bottom, GND)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("bridge   ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Connection between exactly two nodes");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("path     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Crossbar routing needed for a bridge");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("slot     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- Saved configuration file (nodeFileSlot[0-7].txt)");
        
        changeTerminalColor(HELP_COMMAND_COLOR, false);
        Serial.print("chip     ");
        changeTerminalColor(HELP_DESC_COLOR, false);
        Serial.println("- CH446Q crossbar switch (A-L)");
        
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.println("\n Key Concepts:");
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("  - Nodes are connected into nets");
        Serial.println("  - Bridges connect pairs of exactly two nodes");
        Serial.println("  - Paths route bridges through chips");
        Serial.println("  - Slots save complete netlists");
        
    } else {
        changeTerminalColor(HELP_NOTE_COLOR, true);
        Serial.print("Unknown category: ");
        Serial.println(category);
        changeTerminalColor(HELP_DESC_COLOR, true);
        Serial.println("\nAvailable categories:");
        Serial.println("  basics, probe, voltage, arduino, python, apps");
        Serial.println("  display, slots, scripts, debug, config, advanced, glossary");
    }
    
    changeTerminalColor(HELP_NORMAL_COLOR, true);
    Serial.println();
}
