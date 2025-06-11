#ifndef PYTHON_H
#define PYTHON_H





extern char pythonResponseBuffer[1024];
extern int pythonResponseBufferIndex;

extern char pythonCommandBuffer[1024];
extern int pythonCommandBufferIndex;




enum actionType {
    action_none,
    
    // GPIO category
    action_gpio,
    
    // Nodes category  
    action_nodes,
    
    // DAC category
    action_dac,
    
    // ADC category
    action_adc,

    // INA219 category
    action_ina,
    
    // Config category
    action_config,
    
    // Arduino category
    action_arduino,
    
    // UART category
    action_uart,
    
    // Probe category
    action_probe,

    // Clickwheel category
    action_clickwheel,
    
    // OLED category
    action_oled,
    
    // Display category
    action_display,
    
    // Slot category
    action_slot,
    
    // Breadboard category
    action_breadboard,
};

// Sub-action types for each category
enum gpioSubAction {
    gpio_sub_set,
    gpio_sub_get,
    gpio_sub_direction,
    gpio_sub_pulls
};

enum nodesSubAction {
    nodes_sub_connect,
    nodes_sub_add,        // alias for connect
    nodes_sub_remove,
    nodes_sub_load,
    nodes_sub_clear
};

enum dacSubAction {
    dac_sub_set,
    dac_sub_get
};

enum adcSubAction {
    adc_sub_get
};

enum configSubAction {
    config_sub_set,
    config_sub_get,
    config_sub_save
};

enum arduinoSubAction {
    arduino_sub_reset,
    arduino_sub_flash
};

enum uartSubAction {
    uart_sub_connect,
    uart_sub_disconnect
};

enum probeSubAction {
    probe_sub_tap,
    probe_sub_click,
    probe_sub_short_click,  // alias for click
    probe_sub_release
};

enum clickwheelSubAction {
    clickwheel_sub_up,
    clickwheel_sub_down,
    clickwheel_sub_press,
    clickwheel_sub_hold,
    clickwheel_sub_get_press
};

enum oledSubAction {
    oled_sub_connect,
    oled_sub_disconnect,
    oled_sub_clear,
    oled_sub_print,
    oled_sub_show,
    oled_sub_setCursor,
    oled_sub_setTextSize,
    oled_sub_cycleFont,
    oled_sub_setFont,
    oled_sub_isConnected,
    oled_sub_init
};

enum displaySubAction {
    display_sub_startup,
    display_sub_menu,
    display_sub_state,
    display_sub_config
};

enum slotSubAction {
    slot_sub_set
};

enum breadboardSubAction {
    breadboard_sub_print,
    breadboard_sub_clear,
    breadboard_sub_print_raw_row
};

enum inaSubAction {
    ina_sub_getCurrent,
    ina_sub_getVoltage,
    ina_sub_getBusVoltage,
    ina_sub_getPower
};

enum actionResult {
    action_result_success,
    action_result_error,
    action_result_invalid_command,
    action_result_invalid_function,
    action_result_invalid_subaction,
    action_result_invalid_parameter,
    action_result_missing_parameter,
    action_result_invalid_parameter_type,
    action_result_invalid_state,
};

// Maximum number of arguments a command can have
#define MAX_COMMAND_ARGS 8
#define MAX_ARG_STRING_LENGTH 32

// Argument types for parsed commands
enum argType {
    ARG_TYPE_NONE,
    ARG_TYPE_INT,
    ARG_TYPE_FLOAT,
    ARG_TYPE_STRING,
    ARG_TYPE_BOOL
};

// Structure to hold a single argument
typedef struct {
    enum argType type;
    union {
        int intValue;
        float floatValue;
        char stringValue[MAX_ARG_STRING_LENGTH];
        bool boolValue;
    } value;
    char name[MAX_ARG_STRING_LENGTH]; // For named arguments like save=False
} commandArg;

typedef struct {
    char* command;
    char* response;
    enum actionType action;
    enum actionResult result;
    
    // Sub-action (determined by first positional argument)
    int subAction;  // Cast to appropriate sub-action enum based on action type
    char subActionString[MAX_ARG_STRING_LENGTH]; // Original string for sub-action
    
    // Parsed arguments (excluding sub-action)
    int argCount;
    commandArg args[MAX_COMMAND_ARGS];
    
    // Function name extracted from command
    char functionName[MAX_ARG_STRING_LENGTH];
} pythonCommand;

/// @brief Parse a Python command and return the response
/// @param command The command to parse (default is pythonCommandBuffer)
/// @param response The response to return (default is pythonResponseBuffer)
/// @return The exit code
int parseAndExecutePythonCommand(char* command = pythonCommandBuffer, char* response = pythonResponseBuffer);

/// @brief Parse a Python command string into a pythonCommand struct
/// @param command The command string to parse
/// @param parsedCommand Pointer to pythonCommand struct to populate
/// @return action_result_success on success, error code on failure
enum actionResult parsePythonCommandString(const char* command, pythonCommand* parsedCommand);

/// @brief Get action type from function name
/// @param functionName The function name to look up
/// @return The corresponding actionType, or action_none if not found
enum actionType getActionFromFunctionName(const char* functionName);

/// @brief Parse a single argument from a string
/// @param argStr The argument string to parse
/// @param arg Pointer to commandArg to populate
/// @return true on success, false on error
bool parseArgument(const char* argStr, commandArg* arg);

/// @brief Parse sub-action based on action type
/// @param action The action type
/// @param subActionStr The sub-action string to parse
/// @return Sub-action enum value, or -1 if invalid
int parseSubAction(enum actionType action, const char* subActionStr);

/// @brief Get legacy sub-action from function name
/// @param functionName The function name
/// @param action The action type
/// @return Sub-action enum value, or -1 if invalid
int getLegacySubAction(const char* functionName, enum actionType action);

/// @brief Print parsed command details for debugging
/// @param parsedCommand The parsed command to print
void printParsedCommand(const pythonCommand* parsedCommand);

/// @brief Get string representation of argument type
/// @param type The argument type
/// @return String representation of the type
const char* argTypeToString(enum argType type);

/// @brief Get string representation of action type  
/// @param action The action type
/// @return String representation of the action
const char* actionTypeToString(enum actionType action);

/// @brief Get string representation of sub-action
/// @param action The main action type  
/// @param subAction The sub-action enum value
/// @return String representation of the sub-action
const char* subActionToString(enum actionType action, int subAction);

/// @brief Test the Python command parser with example commands
void testPythonParser();

/// @brief Test actual command execution with real Jumperless functions
void testPythonExecution();

/// @brief Initialize MicroPython if not already initialized
bool initMicroPython(void);

/// @brief Deinitialize MicroPython
void deinitMicroPython(void);

/// @brief Execute a MicroPython command string
void executeMicroPython(const char* command);

/// @brief Execute a Python script from the filesystem
bool executePythonScript(const char* filename);

/// @brief Set up the Jumperless MicroPython module
void setupJumperlessModule(void);

/// @brief Test the Jumperless MicroPython module functionality
void testJumperlessModule(void);

/// @brief Process any pending MicroPython commands
void processPendingMicroPythonCommands(void);

/// @brief Check if a command is a Python-style function call
bool isPythonCommand(const char* command);

/// @brief Route command to appropriate parser (Python or main)
bool processCommand(const char* command, char* response);

/// @brief Read a single Python command from serial input
void readPythonCommand(void);

/// @brief Enter Python command mode where all input is interpreted as Python
void pythonCommandMode(void);

char* parsePythonCommand(char* command = pythonCommandBuffer);

/// @brief Start MicroPython
void micropython(void);

/// @brief Start the MicroPython REPL
void micropythonREPL(void);




























#endif