#include "Python.h"
#include <Arduino.h>
#include <string.h>
#include "Graphics.h"
#include "configManager.h"
#include "config.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "RotaryEncoder.h"
#include "Commands.h"
#include "ArduinoStuff.h"
#include "FileParsing.h"
#include "oled.h"

extern "C" {
#include "micropython/micropython_embed/port/micropython_embed.h"

// Global command buffer for MicroPython integration
static char mp_command_buffer[1024];
static char mp_response_buffer[1024];
static bool mp_command_pending = false;

// Synchronous execution response buffer for direct Python calls
static char sync_response_buffer[1024];

// Global variables for synchronous execution results
static char sync_command_response[1024];
static char sync_value_result[256];
static char sync_type_result[64];
static int sync_execution_result = 0;

// Command queue to avoid deadlock
#define MAX_QUEUED_COMMANDS 5
static char command_queue[MAX_QUEUED_COMMANDS][1024];
static char response_queue[MAX_QUEUED_COMMANDS][1024];
static int queue_write_index = 0;
static int queue_read_index = 0;
static int queued_commands = 0;

// Forward declarations
extern "C" int jumperless_execute_sync_c(const char* command);
extern "C" int jumperless_execute_command_sync(const char* command, char* response_out);
extern "C" int parse_response_for_python(const char* response, char* value_out, char* type_out);
extern "C" int execute_hardware_direct(const char* command);

// C function that can be called from MicroPython to execute Jumperless commands
// This will be made available through a Python wrapper function
// void jumperless_execute_command_c(const char* command) {
//     strncpy(mp_command_buffer, command, sizeof(mp_command_buffer) - 1);
//     mp_command_buffer[sizeof(mp_command_buffer) - 1] = '\0';
//     mp_command_pending = true;
// }

// Synchronous execution function that returns results immediately
// This is for when Python code needs to get actual return values
int jumperless_execute_command_sync(const char* command, char* response_out) {
    Serial.println("[SYNC_EXEC] Function entry");
    Serial.flush();
    
    if (!command || !response_out) {
        Serial.println("[SYNC_EXEC] Error: null parameters");
        Serial.flush();
        return -1;
    }
    
    Serial.println("[SYNC_EXEC] Parameters validated");
    Serial.flush();
    
    Serial.print("[SYNC_EXEC] Command length: ");
    Serial.println(strlen(command));
    Serial.flush();
    
    Serial.print("[SYNC_EXEC] About to parse and execute: '");
    Serial.print(command);
    Serial.println("'");
    Serial.flush();
    
    Serial.println("[SYNC_EXEC] About to call parseAndExecutePythonCommand...");
    Serial.flush();
    
    // Use a local buffer to avoid conflicts with queued commands
    char local_response[1024];
    memset(local_response, 0, sizeof(local_response)); // Clear the buffer
    
    Serial.println("[SYNC_EXEC] Local response buffer prepared, calling parser...");
    Serial.flush();
    
    int result = parseAndExecutePythonCommand((char*)command, local_response);
    
    Serial.print("[SYNC_EXEC] parseAndExecutePythonCommand returned: ");
    Serial.println(result);
    Serial.print("[SYNC_EXEC] Response: ");
    Serial.println(local_response);
    Serial.flush();
    
    // Copy response to output buffer
    strncpy(response_out, local_response, 1023);
    response_out[1023] = '\0';
    
    Serial.println("[SYNC_EXEC] Response copied to output buffer");
    Serial.flush();
    
    return result;
}

// Parse response string and extract typed value for Python
// Returns: 0 = success, -1 = error, 1 = boolean true, 2 = boolean false
int parse_response_for_python(const char* response, char* value_out, char* type_out) {
    if (!response || !value_out || !type_out) {
        return -1;
    }
    
    // Initialize outputs
    value_out[0] = '\0';
    type_out[0] = '\0';
    
    // Check if it's an error response
    if (strncmp(response, "ERROR:", 6) == 0) {
        strcpy(type_out, "error");
        strcpy(value_out, response + 7); // Skip "ERROR: "
        return -1;
    }
    
    // Check if it's a success response
    if (strncmp(response, "SUCCESS:", 8) != 0) {
        strcpy(type_out, "error");
        strcpy(value_out, "Invalid response format");
        return -1;
    }
    
    // Look for " = " pattern to extract return values
    const char* equals = strstr(response, " = ");
    if (equals) {
        const char* value_start = equals + 3; // Skip " = "
        
        // Parse different value types
        if (strstr(value_start, "HIGH")) {
            strcpy(type_out, "bool");
            strcpy(value_out, "True");
            return 1;
        } else if (strstr(value_start, "LOW")) {
            strcpy(type_out, "bool");
            strcpy(value_out, "False");
            return 2;
        } else if (strstr(value_start, "true")) {
            strcpy(type_out, "bool");
            strcpy(value_out, "True");
            return 1;
        } else if (strstr(value_start, "false")) {
            strcpy(type_out, "bool");
            strcpy(value_out, "False");
            return 2;
        } else {
            // Extract numeric value (find the number before units like V, mA, mW, mV)
            char temp[64];
            int i = 0;
            
            // Skip leading whitespace
            while (*value_start == ' ' || *value_start == '\t') value_start++;
            
            // Copy digits, decimal point, and minus sign
            while (i < 63 && *value_start && 
                   ((*value_start >= '0' && *value_start <= '9') || 
                    *value_start == '.' || *value_start == '-')) {
                temp[i++] = *value_start++;
            }
            temp[i] = '\0';
            
            if (i > 0) {
                strcpy(value_out, temp);
                // Check if it contains a decimal point
                if (strchr(temp, '.')) {
                    strcpy(type_out, "float");
                } else {
                    strcpy(type_out, "int");
                }
                return 0;
            }
        }
    }
    
    // If no " = " found, it's probably a status message
    strcpy(type_out, "str");
    strcpy(value_out, response);
    return 0;
}

// Override MicroPython output to redirect to Arduino Serial
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // Check if this is a synchronous command execution request
    if (len > 10 && strncmp(str, "SYNC_EXEC:", 10) == 0) {
        // Extract command
        size_t cmd_len = len - 10;
        char sync_command[1024];
        if (cmd_len >= sizeof(sync_command)) cmd_len = sizeof(sync_command) - 1;
        
        strncpy(sync_command, str + 10, cmd_len);
        sync_command[cmd_len] = '\0';
        
        // Remove trailing newline if present
        char* newline = strchr(sync_command, '\n');
        if (newline) *newline = '\0';
        newline = strchr(sync_command, '\r');
        if (newline) *newline = '\0';
        
        // Debug output
        Serial.print("[SYNC] Executing: ");
        Serial.println(sync_command);
        Serial.flush();
        
        // Execute synchronously and store results
        sync_execution_result = jumperless_execute_sync_c(sync_command);
        
        // Debug output
        Serial.print("[SYNC] Result: ");
        Serial.print(sync_execution_result);
        Serial.print(", Value: ");
        Serial.print(sync_value_result);
        Serial.print(", Type: ");
        Serial.println(sync_type_result);
        Serial.flush();
        
        // Set the result values in the global namespace (ensure they're accessible)
        // Use exec() to ensure they're set in the current execution context
        String setup_globals = "exec(\"global _sync_result_ready, _sync_value, _sync_type, _sync_result\")";
        mp_embed_exec_str(setup_globals.c_str());
        
        // Set a flag that Python can check
        mp_embed_exec_str("_sync_result_ready = True");
        
        // Set the result values that Python can access
        String value_cmd = "_sync_value = '";
        value_cmd += sync_value_result;
        value_cmd += "'";
        mp_embed_exec_str(value_cmd.c_str());
        
        String type_cmd = "_sync_type = '";
        type_cmd += sync_type_result;
        type_cmd += "'";
        mp_embed_exec_str(type_cmd.c_str());
        
        String result_cmd = "_sync_result = ";
        result_cmd += sync_execution_result;
        mp_embed_exec_str(result_cmd.c_str());
        
        Serial.println("[SYNC] Variables set in Python");
        Serial.flush();
        
        return; // Don't print anything for sync commands
    }
    
    // Check if this is a command execution request
    if (len > 5 && strncmp(str, "EXEC:", 5) == 0) {
        // Queue the command instead of executing immediately to avoid deadlock
        if (queued_commands < MAX_QUEUED_COMMANDS) {
            // Extract command
            size_t cmd_len = len - 5;
            if (cmd_len >= sizeof(command_queue[0])) cmd_len = sizeof(command_queue[0]) - 1;
            
            strncpy(command_queue[queue_write_index], str + 5, cmd_len);
            command_queue[queue_write_index][cmd_len] = '\0';
            
            // Remove trailing newline if present
            char* newline = strchr(command_queue[queue_write_index], '\n');
            if (newline) *newline = '\0';
            newline = strchr(command_queue[queue_write_index], '\r');
            if (newline) *newline = '\0';
            
            // Advance queue
            queue_write_index = (queue_write_index + 1) % MAX_QUEUED_COMMANDS;
            queued_commands++;
            
            //Serial.print("COMMAND_QUEUED: ");
            //Serial.println(command_queue[(queue_write_index - 1 + MAX_QUEUED_COMMANDS) % MAX_QUEUED_COMMANDS]);
            Serial.flush();
        } else {
            Serial.println("COMMAND_QUEUE_FULL!");
        }
        return;
    }
    
    // Regular output - write to Arduino Serial, converting \n to \r\n
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\n') {
            Serial.write('\r');
        }
        Serial.write(str[i]);
    }
    Serial.flush();
}
}

// Function to check and process pending commands
void processPendingMicroPythonCommands() {
    if (mp_command_pending) {
        int result = parseAndExecutePythonCommand(mp_command_buffer, mp_response_buffer);
       // Serial.print("MP_RESULT: ");
        // Serial.println(mp_response_buffer);
        Serial.flush();
        mp_command_pending = false;
    }
}

// Function to process queued commands (safe to call from main loop)
void processQueuedCommands() {
    while (queued_commands > 0) {
        // Get the next command from queue
        char* command = command_queue[queue_read_index];
        char* response = response_queue[queue_read_index];
        
        // Execute the command
        int result = parseAndExecutePythonCommand(command, response);
        
        // Print result
        // Serial.print("QUEUE_RESULT: ");
        Serial.println(response);
        Serial.flush();
        
        // Also send result back to MicroPython for future reference
        String mp_result_cmd = "last_result = '";
        mp_result_cmd += response;
        mp_result_cmd += "'";
        mp_embed_exec_str(mp_result_cmd.c_str());
        
        // Advance queue
        queue_read_index = (queue_read_index + 1) % MAX_QUEUED_COMMANDS;
        queued_commands--;
    }
}

// Check if a command looks like a Python function call
bool isPythonCommand(const char* command) {
    if (!command) return false;
    
    // Look for function call pattern: functionName(
    const char* openParen = strchr(command, '(');
    if (!openParen) return false;
    
    // Check if it ends with )
    const char* closeParen = strrchr(command, ')');
    if (!closeParen || closeParen <= openParen) return false;
    
    // Check if function name matches our known Python functions
    char funcName[64];
    size_t nameLen = openParen - command;
    if (nameLen >= sizeof(funcName)) return false;
    
    strncpy(funcName, command, nameLen);
    funcName[nameLen] = '\0';
    
    // Remove trailing whitespace
    for (int i = nameLen - 1; i >= 0 && (funcName[i] == ' ' || funcName[i] == '\t'); i--) {
        funcName[i] = '\0';
    }
    
    // Check against known Python function names
    enum actionType action = getActionFromFunctionName(funcName);
    return (action != action_none);
}

// Route command to appropriate parser
bool processCommand(const char* command, char* response) {
    if (isPythonCommand(command)) {
        // Route to Python parser
        int result = parseAndExecutePythonCommand((char*)command, response);
        return (result == 0);
    } else {
        //! Route to regular command system
        // You'll need to call your existing command processor here
        // For now, just indicate it's not a Python command
        snprintf(response, 1024, "INFO: Not a Python command, routing to main command processor");
        return false; // Let main processor handle it
    }
}

char pythonResponseBuffer[1024];
int pythonResponseBufferIndex;
char pythonCommandBuffer[1024];
int pythonCommandBufferIndex;

// Function name to action type mapping table
typedef struct {
    const char* functionName;
    enum actionType action;
} functionMapping;

static const functionMapping functionMap[] = {
    // Hierarchical function mappings
    {"gpio", action_gpio},
    {"nodes", action_nodes},
    {"dac", action_dac},
    {"adc", action_adc},
    {"ina", action_ina},
    {"config", action_config},
    {"arduino", action_arduino},
    {"uart", action_uart},
    {"probe", action_probe},
    {"clickwheel", action_clickwheel},
    {"oled", action_oled},
    {"display", action_display},
    {"slot", action_slot},
    {"breadboard", action_breadboard},
    
    // Legacy single-function mappings for backward compatibility
    {"setDac", action_dac},
    {"getDac", action_dac},
    {"getAdc", action_adc},
    {"getInaCurrent", action_ina},
    {"getInaVoltage", action_ina},
    {"getInaBusVoltage", action_ina},
    {"getInaPower", action_ina},
    {"setGpio", action_gpio},
    {"getGpio", action_gpio},
    {"setGpioDirection", action_gpio},
    {"getGpioDirection", action_gpio},
    {"setGpioPulls", action_gpio},
    {"getGpioPulls", action_gpio},
    {"connectNodes", action_nodes},
    {"addNodes", action_nodes},
    {"removeNodes", action_nodes},
    {"loadNodes", action_nodes},
    {"clearNodes", action_nodes},
    {"setConfig", action_config},
    {"getConfig", action_config},
    {"saveConfig", action_config},
    {"resetArduino", action_arduino},
    {"flashArduino", action_arduino},
    {"connectUart", action_uart},
    {"disconnectUart", action_uart},
    {"simulateProbe", action_probe},
    {"connectOled", action_oled},
    {"disconnectOled", action_oled},
    {"oledClear", action_oled},
    {"oledPrint", action_oled},
    {"oledShow", action_oled},
    {"oledSetCursor", action_oled},
    {"oledSetTextSize", action_oled},
    {"oledCycleFont", action_oled},
    {"oledSetFont", action_oled},
    {"showStartup", action_display},
    {"showMenu", action_display},
    {"showState", action_display},
    {"showConfig", action_display},
    {"setSlot", action_slot},
    {"breadboardPrint", action_breadboard},
    {"breadboardClear", action_breadboard},
    {"breadboardPrintRawRow", action_breadboard},
    
    {NULL, action_none} // Sentinel
};

enum actionType getActionFromFunctionName(const char* functionName) {
    for (int i = 0; functionMap[i].functionName != NULL; i++) {
        if (strcmp(functionName, functionMap[i].functionName) == 0) {
            return functionMap[i].action;
        }
    }
    return action_none;
}

bool parseArgument(const char* argStr, commandArg* arg) {
    if (!argStr || !arg) return false;
    
    // Clear the argument
    memset(arg, 0, sizeof(commandArg));
    
    // Remove leading/trailing whitespace
    while (*argStr == ' ' || *argStr == '\t') argStr++;
    
    // Check for named argument (contains '=')
    const char* equals = strchr(argStr, '=');
    if (equals) {
        // Extract name
        size_t nameLen = equals - argStr;
        if (nameLen >= MAX_ARG_STRING_LENGTH) nameLen = MAX_ARG_STRING_LENGTH - 1;
        strncpy(arg->name, argStr, nameLen);
        arg->name[nameLen] = '\0';
        
        // Remove trailing whitespace from name
        for (int i = nameLen - 1; i >= 0 && (arg->name[i] == ' ' || arg->name[i] == '\t'); i--) {
            arg->name[i] = '\0';
        }
        
        // Move to value part
        argStr = equals + 1;
        while (*argStr == ' ' || *argStr == '\t') argStr++;
    }
    
    // Parse the value
    char* endPtr;
    
    // Check for boolean values
    if (strncmp(argStr, "True", 4) == 0 || strncmp(argStr, "true", 4) == 0) {
        arg->type = ARG_TYPE_BOOL;
        arg->value.boolValue = true;
        return true;
    }
    if (strncmp(argStr, "False", 5) == 0 || strncmp(argStr, "false", 5) == 0) {
        arg->type = ARG_TYPE_BOOL;
        arg->value.boolValue = false;
        return true;
    }
    
    // Check for string (quoted)
    if (*argStr == '"' || *argStr == '\'') {
        char quote = *argStr;
        argStr++; // Skip opening quote
        
        arg->type = ARG_TYPE_STRING;
        int i = 0;
        while (*argStr && *argStr != quote && i < MAX_ARG_STRING_LENGTH - 1) {
            arg->value.stringValue[i++] = *argStr++;
        }
        arg->value.stringValue[i] = '\0';
        return true;
    }
    
    // Check for float (contains decimal point)
    if (strchr(argStr, '.')) {
        arg->type = ARG_TYPE_FLOAT;
        arg->value.floatValue = strtof(argStr, &endPtr);
        return endPtr != argStr;
    }
    
    // Check for integer
    long longVal = strtol(argStr, &endPtr, 10);
    if (endPtr != argStr) {
        arg->type = ARG_TYPE_INT;
        arg->value.intValue = (int)longVal;
        return true;
    }
    
    // Default to string if all else fails
    arg->type = ARG_TYPE_STRING;
    strncpy(arg->value.stringValue, argStr, MAX_ARG_STRING_LENGTH - 1);
    arg->value.stringValue[MAX_ARG_STRING_LENGTH - 1] = '\0';
    
    // Remove trailing whitespace and quotes
    int len = strlen(arg->value.stringValue);
    while (len > 0 && (arg->value.stringValue[len-1] == ' ' || 
                       arg->value.stringValue[len-1] == '\t' ||
                       arg->value.stringValue[len-1] == '"' ||
                       arg->value.stringValue[len-1] == '\'')) {
        arg->value.stringValue[--len] = '\0';
    }
    
    return true;
}

enum actionResult parsePythonCommandString(const char* command, pythonCommand* parsedCommand) {
    if (!command || !parsedCommand) {
        return action_result_invalid_parameter;
    }
    
    // Initialize the parsed command
    memset(parsedCommand, 0, sizeof(pythonCommand));
    parsedCommand->command = (char*)command;
    parsedCommand->result = action_result_error;
    parsedCommand->subAction = -1;
    
    // Find the opening parenthesis
    const char* openParen = strchr(command, '(');
    if (!openParen) {
        return action_result_invalid_command;
    }
    
    // Extract function name
    size_t nameLen = openParen - command;
    if (nameLen >= MAX_ARG_STRING_LENGTH) {
        return action_result_invalid_command;
    }
    
    strncpy(parsedCommand->functionName, command, nameLen);
    parsedCommand->functionName[nameLen] = '\0';
    
    // Remove trailing whitespace from function name
    for (int i = nameLen - 1; i >= 0 && (parsedCommand->functionName[i] == ' ' || parsedCommand->functionName[i] == '\t'); i--) {
        parsedCommand->functionName[i] = '\0';
    }
    
    // Get action type
    parsedCommand->action = getActionFromFunctionName(parsedCommand->functionName);
    if (parsedCommand->action == action_none) {
        return action_result_invalid_function;
    }
    
    // Find the closing parenthesis
    const char* closeParen = strrchr(command, ')');
    if (!closeParen || closeParen <= openParen) {
        return action_result_invalid_command;
    }
    
    // Extract arguments string
    size_t argsLen = closeParen - openParen - 1;
    if (argsLen == 0) {
        // No arguments - for legacy functions, determine sub-action from function name
        if (strstr(parsedCommand->functionName, "set") || 
            strstr(parsedCommand->functionName, "get") ||
            strstr(parsedCommand->functionName, "connect") ||
            strstr(parsedCommand->functionName, "clear") ||
            strstr(parsedCommand->functionName, "show") ||
            strstr(parsedCommand->functionName, "save") ||
            strstr(parsedCommand->functionName, "reset") ||
            strstr(parsedCommand->functionName, "flash") ||
            strstr(parsedCommand->functionName, "disconnect") ||
            strstr(parsedCommand->functionName, "simulate") ||
            strstr(parsedCommand->functionName, "add") ||
            strstr(parsedCommand->functionName, "remove") ||
            strstr(parsedCommand->functionName, "load") ||
            strstr(parsedCommand->functionName, "print")) {
            // Legacy function - determine sub-action from function name
            parsedCommand->subAction = getLegacySubAction(parsedCommand->functionName, parsedCommand->action);
            if (parsedCommand->subAction == -1) {
                return action_result_invalid_subaction;
            }
        }
        parsedCommand->result = action_result_success;
        return action_result_success;
    }
    
    // Create a copy of the arguments string for parsing
    char argsStr[MAX_ARG_STRING_LENGTH];
    strncpy(argsStr, openParen + 1, argsLen);
    argsStr[argsLen] = '\0';
    
    // Parse arguments (simple comma-separated parsing)
    char* argStart = argsStr;
    parsedCommand->argCount = 0;
    
    // First, check if this is a hierarchical command (first arg could be sub-action)
    bool isHierarchical = false;
    bool needsSubAction = false;
    
    // Check if function name is one of the hierarchical ones
    for (int i = 0; functionMap[i].functionName != NULL; i++) {
        if (strcmp(parsedCommand->functionName, functionMap[i].functionName) == 0) {
            // Check if it's a short hierarchical name (not legacy)
            if (strcmp(functionMap[i].functionName, "gpio") == 0 ||
                strcmp(functionMap[i].functionName, "nodes") == 0 ||
                strcmp(functionMap[i].functionName, "dac") == 0 ||
                strcmp(functionMap[i].functionName, "adc") == 0 ||
                strcmp(functionMap[i].functionName, "ina") == 0 ||
                strcmp(functionMap[i].functionName, "config") == 0 ||
                strcmp(functionMap[i].functionName, "arduino") == 0 ||
                strcmp(functionMap[i].functionName, "uart") == 0 ||
                strcmp(functionMap[i].functionName, "probe") == 0 ||
                strcmp(functionMap[i].functionName, "clickwheel") == 0 ||
                strcmp(functionMap[i].functionName, "oled") == 0 ||
                strcmp(functionMap[i].functionName, "display") == 0 ||
                strcmp(functionMap[i].functionName, "slot") == 0 ||
                strcmp(functionMap[i].functionName, "breadboard") == 0) {
                isHierarchical = true;
                needsSubAction = true;
                break;
            }
        }
    }
    
    while (argStart && *argStart && parsedCommand->argCount < MAX_COMMAND_ARGS) {
        // Find the next comma (but not inside quotes)
        char* argEnd = argStart;
        bool inQuotes = false;
        char quoteChar = '\0';
        
        while (*argEnd) {
            if ((*argEnd == '"' || *argEnd == '\'') && !inQuotes) {
                inQuotes = true;
                quoteChar = *argEnd;
            } else if (*argEnd == quoteChar && inQuotes) {
                inQuotes = false;
                quoteChar = '\0';
            } else if (*argEnd == ',' && !inQuotes) {
                break;
            }
            argEnd++;
        }
        
        // Extract this argument
        char argStr[MAX_ARG_STRING_LENGTH];
        strncpy(argStr, argStart, argEnd - argStart);
        argStr[argEnd - argStart] = '\0';
        
        // If this is the first argument and we need a sub-action, try to parse it as such
        if (parsedCommand->argCount == 0 && needsSubAction) {
            // Remove quotes if present
            char* subActionStr = argStr;
            while (*subActionStr == ' ' || *subActionStr == '\t') subActionStr++;
            if (*subActionStr == '"' || *subActionStr == '\'') {
                char quote = *subActionStr;
                subActionStr++;
                char* end = strchr(subActionStr, quote);
                if (end) *end = '\0';
            }
            
            // Try to parse as sub-action
            int subAction = parseSubAction(parsedCommand->action, subActionStr);
            if (subAction != -1) {
                parsedCommand->subAction = subAction;
                strncpy(parsedCommand->subActionString, subActionStr, MAX_ARG_STRING_LENGTH - 1);
                parsedCommand->subActionString[MAX_ARG_STRING_LENGTH - 1] = '\0';
                needsSubAction = false;
                
                // Move to next argument
                if (*argEnd == ',') {
                    argStart = argEnd + 1;
                    while (*argStart == ' ' || *argStart == '\t') argStart++;
                } else {
                    break;
                }
                continue;
            } else {
                // Not a valid sub-action
                return action_result_invalid_subaction;
            }
        }
        
        // Parse the argument normally
        if (!parseArgument(argStr, &parsedCommand->args[parsedCommand->argCount])) {
            return action_result_invalid_parameter_type;
        }
        
        parsedCommand->argCount++;
        
        // Move to next argument
        if (*argEnd == ',') {
            argStart = argEnd + 1;
            while (*argStart == ' ' || *argStart == '\t') argStart++;
        } else {
            break;
        }
    }
    
    // If we still need a sub-action and this is a legacy function, try to determine it
    if (parsedCommand->subAction == -1) {
        parsedCommand->subAction = getLegacySubAction(parsedCommand->functionName, parsedCommand->action);
        if (needsSubAction && parsedCommand->subAction == -1) {
            return action_result_invalid_subaction;
        }
    }
    
    parsedCommand->result = action_result_success;
    return action_result_success;
}

char* parsePythonCommand(char* command) {
    if (!command) {
        command = pythonCommandBuffer;
    }
    
    static pythonCommand parsedCmd;
    enum actionResult result = parsePythonCommandString(command, &parsedCmd);
    
    if (result == action_result_success) {
        snprintf(pythonResponseBuffer, sizeof(pythonResponseBuffer), 
                "SUCCESS: Parsed function '%s' with %d arguments", 
                parsedCmd.functionName, parsedCmd.argCount);
    } else {
        snprintf(pythonResponseBuffer, sizeof(pythonResponseBuffer), 
                "ERROR: Failed to parse command - %s", 
                result == action_result_invalid_command ? "Invalid command format" :
                result == action_result_invalid_parameter ? "Invalid parameter" : "Unknown error");
    }
    
    return pythonResponseBuffer;
}

int parseAndExecutePythonCommand(char* command, char* response) {
    if (!command) {
        command = pythonCommandBuffer;
    }
    if (!response) {
        response = pythonResponseBuffer;
    }
    
    pythonCommand parsedCmd;
    enum actionResult result = parsePythonCommandString(command, &parsedCmd);
    
    if (result != action_result_success) {
        snprintf(response, 1024, "ERROR: Failed to parse command - %s", 
                result == action_result_invalid_function ? "Invalid function" :
                result == action_result_invalid_subaction ? "Invalid sub-action" :
                result == action_result_invalid_parameter_type ? "Invalid parameter type" :
                result == action_result_missing_parameter ? "Missing parameter" : "Unknown error");
        return -1;
    }
    
    // Execute the parsed command based on action type and sub-action
    switch (parsedCmd.action) {
        case action_dac: {
            if (parsedCmd.subAction == dac_sub_set) {
                // Example: dac(set, channel, voltage, save=False) or setDac(channel, voltage, save=False)
                if (parsedCmd.argCount >= 2) {
                    if (parsedCmd.args[0].type == ARG_TYPE_INT && 
                        parsedCmd.args[1].type == ARG_TYPE_FLOAT) {
                        
                        int channel = parsedCmd.args[0].value.intValue;
                        float voltage = parsedCmd.args[1].value.floatValue;
                        bool save = true;
                        
                        // Check for save parameter
                        for (int i = 2; i < parsedCmd.argCount; i++) {
                            if (strcmp(parsedCmd.args[i].name, "save") == 0 && 
                                parsedCmd.args[i].type == ARG_TYPE_BOOL) {
                                save = parsedCmd.args[i].value.boolValue;
                                break;
                            }
                        }
                        
                        // Call actual Jumperless function
                        setDacByNumber(channel, voltage, save ? 1 : 0);
                        snprintf(response, 1024, "SUCCESS: dac(set, channel=%d, voltage=%.2f, save=%s)", 
                                channel, voltage, save ? "true" : "false");
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for dac(set)");
                return -1;
            } else if (parsedCmd.subAction == dac_sub_get) {
                // Example: dac(get, channel) or getDac(channel)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int channel = parsedCmd.args[0].value.intValue;
                    
                    // Call actual Jumperless function
                    // TODO: Implement getDacVoltage function if available
                    snprintf(response, 1024, "SUCCESS: dac(get, channel=%d) - function not yet implemented", 
                            channel);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for dac(get)");
                return -1;
            }
            break;
        }
        
        case action_adc: {
            if (parsedCmd.subAction == adc_sub_get) {
                // Example: adc(get, channel) or getAdc(channel)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int channel = parsedCmd.args[0].value.intValue;
                    
                    // Call actual Jumperless function
                     float voltage = readAdcVoltage(channel, 32);
                    snprintf(response, 1024, "SUCCESS: adc(get, channel=%d) = %.3f V", 
                            channel, voltage) ; // placeholder value
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for adc(get)");
                return -1;
            }
            break;
        }
        
        case action_ina: {
            if (parsedCmd.subAction == ina_sub_getCurrent) {
                // Example: ina(getCurrent, sensor) or getInaCurrent(sensor)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int sensor = parsedCmd.args[0].value.intValue;
                    
                    // Validate sensor number (0 or 1)
                    if (sensor < 0 || sensor > 1) {
                        snprintf(response, 1024, "ERROR: INA sensor must be 0 or 1, got %d", sensor);
                        return -1;
                    }
                    
                    // Call actual Jumperless function
                    float current = (sensor == 0) ? INA0.getCurrent_mA() : INA1.getCurrent_mA();
                    snprintf(response, 1024, "SUCCESS: ina(getCurrent, sensor=%d) = %.3f mA", 
                            sensor, current);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for ina(getCurrent)");
                return -1;
            } else if (parsedCmd.subAction == ina_sub_getVoltage) {
                // Example: ina(getVoltage, sensor) or getInaVoltage(sensor)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int sensor = parsedCmd.args[0].value.intValue;
                    
                    // Validate sensor number (0 or 1)
                    if (sensor < 0 || sensor > 1) {
                        snprintf(response, 1024, "ERROR: INA sensor must be 0 or 1, got %d", sensor);
                        return -1;
                    }
                    
                    // Call actual Jumperless function
                    float voltage = (sensor == 0) ? INA0.getShuntVoltage_mV() : INA1.getShuntVoltage_mV();
                    snprintf(response, 1024, "SUCCESS: ina(getVoltage, sensor=%d) = %.3f mV", 
                            sensor, voltage);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for ina(getVoltage)");
                return -1;
            } else if (parsedCmd.subAction == ina_sub_getBusVoltage) {
                // Example: ina(getBusVoltage, sensor) or getInaBusVoltage(sensor)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int sensor = parsedCmd.args[0].value.intValue;
                    
                    // Validate sensor number (0 or 1)
                    if (sensor < 0 || sensor > 1) {
                        snprintf(response, 1024, "ERROR: INA sensor must be 0 or 1, got %d", sensor);
                        return -1;
                    }
                    
                    // Call actual Jumperless function
                    float busVoltage = (sensor == 0) ? INA0.getBusVoltage() : INA1.getBusVoltage();
                    snprintf(response, 1024, "SUCCESS: ina(getBusVoltage, sensor=%d) = %.3f V", 
                            sensor, busVoltage);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for ina(getBusVoltage)");
                return -1;
            } else if (parsedCmd.subAction == ina_sub_getPower) {
                // Example: ina(getPower, sensor) or getInaPower(sensor)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int sensor = parsedCmd.args[0].value.intValue;
                    
                    // Validate sensor number (0 or 1)
                    if (sensor < 0 || sensor > 1) {
                        snprintf(response, 1024, "ERROR: INA sensor must be 0 or 1, got %d", sensor);
                        return -1;
                    }
                    
                    // Call actual Jumperless function
                    float power = (sensor == 0) ? INA0.getPower_mW() : INA1.getPower_mW();
                    snprintf(response, 1024, "SUCCESS: ina(getPower, sensor=%d) = %.3f mW", 
                            sensor, power);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for ina(getPower)");
                return -1;
            }
            break;
        }
        
        case action_gpio: {
            if (parsedCmd.subAction == gpio_sub_set) {
                // Example: gpio(set, pin, value) or setGpio(pin, value)
                if (parsedCmd.argCount >= 2) {
                    if (parsedCmd.args[0].type == ARG_TYPE_INT) {
                        int pin = parsedCmd.args[0].value.intValue;
                        
                        // Validate pin range (only allow 1-10)
                        if (pin < 1 || pin > 10) {
                            snprintf(response, 1024, "ERROR: GPIO pin must be between 1 and 10, got %d", pin);
                            return -1;
                        }
                        
                        // Handle different value types (int, string like "HIGH"/"LOW")
                        int value = 0;
                        if (parsedCmd.args[1].type == ARG_TYPE_INT) {
                            value = parsedCmd.args[1].value.intValue;
                        } else if (parsedCmd.args[1].type == ARG_TYPE_STRING) {
                            if (strcmp(parsedCmd.args[1].value.stringValue, "HIGH") == 0) {
                                value = 1;
                            } else if (strcmp(parsedCmd.args[1].value.stringValue, "LOW") == 0) {
                                value = 0;
                            }
                        }
                        
                        // Call actual Jumperless function
                        digitalWrite(gpioDef[pin][0], value);
                        snprintf(response, 1024, "SUCCESS: gpio(set, pin=%d, value=%s)", 
                                pin, value ? "HIGH" : "LOW");
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for gpio(set)");
                return -1;
            } else if (parsedCmd.subAction == gpio_sub_get) {
                // Example: gpio(get, pin) or getGpio(pin)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int pin = parsedCmd.args[0].value.intValue;
                    
                    // Validate pin range (only allow 1-10)
                    if (pin < 1 || pin > 10) {
                        snprintf(response, 1024, "ERROR: GPIO pin must be between 1 and 10, got %d", pin);
                        return -1;
                    }
                    
                    // Call actual Jumperless function
                    int value = digitalRead(gpioDef[pin][0]);
                    snprintf(response, 1024, "SUCCESS: gpio(get, pin=%d) = %s", pin, value ? "HIGH" : "LOW");
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for gpio(get)");
                return -1;
            } else if (parsedCmd.subAction == gpio_sub_direction) {
                // Example: gpio(direction, pin, direction)
                if (parsedCmd.argCount >= 2) {
                    int pin = parsedCmd.args[0].value.intValue;
                    
                    // Validate pin range (only allow 1-10)
                    if (pin < 1 || pin > 10) {
                        snprintf(response, 1024, "ERROR: GPIO pin must be between 1 and 10, got %d", pin);
                        return -1;
                    }
                    
                    char* direction = parsedCmd.args[1].value.stringValue;
                    
                    snprintf(response, 1024, "SUCCESS: gpio(direction, pin=%d, direction=%s)", 
                            pin, direction);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for gpio(direction)");
                return -1;
            }
            break;
        }
        
        case action_nodes: {
            if (parsedCmd.subAction == nodes_sub_connect || parsedCmd.subAction == nodes_sub_add) {
                // Example: nodes(connect, node1, node2, save=False) or connectNodes(node1, node2, save=False)
                if (parsedCmd.argCount >= 2) {
                    if (parsedCmd.args[0].type == ARG_TYPE_INT && 
                        parsedCmd.args[1].type == ARG_TYPE_INT) {
                        
                        int node1 = parsedCmd.args[0].value.intValue;
                        int node2 = parsedCmd.args[1].value.intValue;
                        bool save = true;
                        
                        // Check for save parameter
                        for (int i = 2; i < parsedCmd.argCount; i++) {
                            if (strcmp(parsedCmd.args[i].name, "save") == 0 && 
                                parsedCmd.args[i].type == ARG_TYPE_BOOL) {
                                save = parsedCmd.args[i].value.boolValue;
                                break;
                            }
                        }
                        
                        // Call actual Jumperless function
                        if (save ) { // TODO: for now, always save
                            addBridgeToNodeFile(node1, node2, netSlot,  0);
                            refreshConnections();
                        } else {
                            addBridgeToNodeFile(node1, node2, netSlot,  1);
                            refreshLocalConnections();
                        }

                        snprintf(response, 1024, "SUCCESS: nodes(connect, node1=%d, node2=%d, save=%s)", 
                                node1, node2, save ? "true" : "false");
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for nodes(connect)");
                return -1;
            } else if (parsedCmd.subAction == nodes_sub_remove) {
                // Example: nodes(remove, node1, node2)
                if (parsedCmd.argCount >= 2) {
                    if (parsedCmd.args[0].type == ARG_TYPE_INT && 
                        parsedCmd.args[1].type == ARG_TYPE_INT) {
                        
                        int node1 = parsedCmd.args[0].value.intValue;
                        int node2 = parsedCmd.args[1].value.intValue;
                        
                        // Call actual Jumperless function
                        removeBridgeFromNodeFile(node1, node2, netSlot, 0);
                        refreshConnections();
                        snprintf(response, 1024, "SUCCESS: nodes(remove, node1=%d, node2=%d)", 
                                node1, node2);
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for nodes(remove)");
                return -1;
            }
            break;
        }
        
        case action_probe: {
            if (parsedCmd.subAction == probe_sub_tap) {
                // Example: probe(tap, node) or simulateProbe(tap, node)
                if (parsedCmd.argCount >= 1) {
                    // Argument is the node/pin
                    if (parsedCmd.args[0].type == ARG_TYPE_STRING || 
                        parsedCmd.args[0].type == ARG_TYPE_INT) {
                        
                        char nodeStr[32];
                        if (parsedCmd.args[0].type == ARG_TYPE_STRING) {
                            strncpy(nodeStr, parsedCmd.args[0].value.stringValue, sizeof(nodeStr)-1);
                        } else {
                            snprintf(nodeStr, sizeof(nodeStr), "%d", parsedCmd.args[0].value.intValue);
                        }
                        
                        // Call actual Jumperless function
                        // TODO: Implement probe simulation function
                        // simulateProbeTap(nodeStr);
                        snprintf(response, 1024, "SUCCESS: probe(tap, %s) - function not yet implemented", nodeStr);
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for probe(tap)");
                return -1;
            }
            break;
        }
        case action_clickwheel: {
            if (parsedCmd.subAction == clickwheel_sub_up) {
                // Example: clickwheel(up, 1) or clickwheel(up, 3)
                int clicks = 1; // default
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    clicks = parsedCmd.args[0].value.intValue;
                }
                
                // // Simulate multiple encoder clicks
                // //for (int i = 0; i < clicks; i++) {

                // Serial.print("lastButtonEncoderState: ");
                // Serial.println(lastButtonEncoderState);
                // Serial.print("encoderButtonState: ");
                // Serial.println(encoderButtonState);
                // Serial.print("encoderDirectionState: ");
                // Serial.println(encoderDirectionState);

                // Serial.print("lastDirectionState: ");
                // Serial.println(lastDirectionState);
                // Serial.print("encoderOverride: ");
                // Serial.println(encoderOverride);
                // Serial.println("Setting encoderOverride to 10");
                 encoderOverride = 10;
                // //lastDirectionState = NONE;
                // encoderButtonState = IDLE;
                // lastButtonEncoderState = IDLE;
                lastDirectionState = NONE;
                encoderDirectionState = UP;
                    //delay(50); // Small delay between clicks
               // }
                
                

                snprintf(response, 1024, "SUCCESS: clickwheel(up, %d) - simulated %d up clicks", clicks, clicks);
                return 0;
                
            } else if (parsedCmd.subAction == clickwheel_sub_down) {
                // Example: clickwheel(down, 1) or clickwheel(down, 3)
                int clicks = 1; // default
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    clicks = parsedCmd.args[0].value.intValue;
                }

                // Serial.print("encoderPositionOffset: ");
                // Serial.println(encoderPositionOffset);
                // Serial.print("encoderPosition: ");
                // Serial.println(encoderPosition);
                
                // resetEncoderPosition = true;
                // encoderPositionOffset -= clicks * 100;
                // Serial.print("encoderPositionOffset: ");
                // Serial.println(encoderPositionOffset);
                
                // Simulate multiple encoder clicks
                //for (int i = 0; i < clicks; i++) {
                encoderOverride = 10;
                lastDirectionState = NONE;
                encoderDirectionState = DOWN;
                    //delay(50); // Small delay between clicks
               // }
                
                snprintf(response, 1024, "SUCCESS: clickwheel(down, %d) - simulated %d down clicks", clicks, clicks);
                return 0;
                
            } else if (parsedCmd.subAction == clickwheel_sub_press) {
                // Example: clickwheel(press)

                // Serial.print("lastButtonEncoderState: ");
                // Serial.println(lastButtonEncoderState);
                // Serial.print("encoderButtonState: ");
                // Serial.println(encoderButtonState);
                // Serial.print("encoderDirectionState: ");
                // Serial.println(encoderDirectionState);

                // Serial.print("lastDirectionState: ");
                // Serial.println(lastDirectionState);
                // Serial.print("encoderOverride: ");
                // Serial.println(encoderOverride);
                // Serial.println("Setting encoderOverride to 10");
                encoderOverride = 10;

                lastButtonEncoderState = PRESSED;
                encoderButtonState = RELEASED;
                
                snprintf(response, 1024, "SUCCESS: clickwheel(press) - simulated button press");
                return 0;
                
            } else if (parsedCmd.subAction == clickwheel_sub_hold) {
                // Example: clickwheel(hold, time=0.5) or clickwheel(hold, 1.0)
                float holdTime = 0.5; // default 500ms
                
                // Check for time parameter (named or positional)
                for (int i = 0; i < parsedCmd.argCount; i++) {
                    if (parsedCmd.args[i].type == ARG_TYPE_FLOAT || parsedCmd.args[i].type == ARG_TYPE_INT) {
                        if (strlen(parsedCmd.args[i].name) == 0 || strcmp(parsedCmd.args[i].name, "time") == 0) {
                            if (parsedCmd.args[i].type == ARG_TYPE_FLOAT) {
                                holdTime = parsedCmd.args[i].value.floatValue;
                            } else {
                                holdTime = (float)parsedCmd.args[i].value.intValue;
                            }
                            break;
                        }
                    }
                }
                
                // Simulate button hold
                lastButtonEncoderState = PRESSED;
                encoderButtonState = HELD;
                
                // Convert to milliseconds and delay
                int holdTimeMs = (int)(holdTime * 1000);
                delay(holdTimeMs);
                
                // Release after hold
                //encoderButtonState = RELEASED;
                
                snprintf(response, 1024, "SUCCESS: clickwheel(hold, %.2f) - held button for %.2f seconds", holdTime, holdTime);
                return 0;
                
            } else if (parsedCmd.subAction == clickwheel_sub_get_press) {
                // Example: clickwheel(get_press)
                bool isPressed = (encoderButtonState == PRESSED);
                snprintf(response, 1024, "SUCCESS: clickwheel(get_press) = %s", isPressed ? "true" : "false");
                return 0;
            }
            
            snprintf(response, 1024, "ERROR: clickwheel(%s) - sub-action not implemented", 
                    subActionToString(parsedCmd.action, parsedCmd.subAction));
            return -1;
        }

        case action_arduino: {
            if (parsedCmd.subAction == arduino_sub_reset) {
                // Example: arduino(reset) or resetArduino()
                resetArduino();
                snprintf(response, 1024, "SUCCESS: arduino(reset) - Arduino reset");
                return 0;
            } else if (parsedCmd.subAction == arduino_sub_flash) {
                // Example: arduino(flash) or flashArduino()
                snprintf(response, 1024, "SUCCESS: arduino(flash) - function not yet implemented");
                return 0;
            }
            break;
        }
        
        case action_uart: {
            if (parsedCmd.subAction == uart_sub_connect) {
                // Example: uart(connect) or connectUart()
                connectArduino(0);
                snprintf(response, 1024, "SUCCESS: uart(connect) - UART connected to Arduino D0/D1");
                return 0;
            } else if (parsedCmd.subAction == uart_sub_disconnect) {
                // Example: uart(disconnect) or disconnectUart()
                disconnectArduino(0);
                snprintf(response, 1024, "SUCCESS: uart(disconnect) - UART disconnected from Arduino D0/D1");
                return 0;
            }
            break;
        }
        
        case action_config: {
            if (parsedCmd.subAction == config_sub_set) {
                // Example: config(set, section, key, value)
                snprintf(response, 1024, "SUCCESS: config(set) - function not yet implemented");
                return 0;
            } else if (parsedCmd.subAction == config_sub_get) {
                // Example: config(get, section, key)
                snprintf(response, 1024, "SUCCESS: config(get) - function not yet implemented");
                return 0;
            } else if (parsedCmd.subAction == config_sub_save) {
                // Example: config(save)
                snprintf(response, 1024, "SUCCESS: config(save) - function not yet implemented");
                return 0;
            }
            break;
        }
        
        case action_display: {
            snprintf(response, 1024, "SUCCESS: display(%s) - function not yet implemented", 
                    subActionToString(parsedCmd.action, parsedCmd.subAction));
            return 0;
        }
        
        case action_oled: {
            if (parsedCmd.subAction == oled_sub_print) {
                // Example: oled(print, "text", textSize=2) - uses clearPrintShow as requested
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_STRING) {
                    const char* text = parsedCmd.args[0].value.stringValue;
                    int textSize = 2; // default
                    
                    // Check for optional textSize parameter
                    if (parsedCmd.argCount >= 2 && parsedCmd.args[1].type == ARG_TYPE_INT) {
                        textSize = parsedCmd.args[1].value.intValue;
                    }
                    
                    // Use clearPrintShow as requested
                    oled.clearPrintShow(text, textSize, true, true, true);
                    snprintf(response, 1024, "SUCCESS: oled(print, \"%s\", textSize=%d)", text, textSize);

                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for oled(print) - need text string");
                return -1;
            } else if (parsedCmd.subAction == oled_sub_clear) {
                // Example: oled(clear)
                oled.clear();
                snprintf(response, 1024, "SUCCESS: oled(clear) - display cleared");
                return 0;
            } else if (parsedCmd.subAction == oled_sub_show) {
                // Example: oled(show)
                oled.show();
                snprintf(response, 1024, "SUCCESS: oled(show) - display updated");
                return 0;
            } else if (parsedCmd.subAction == oled_sub_connect || parsedCmd.subAction == oled_sub_init) {
                // Example: oled(connect) or oled(init)
                int result = oled.init();
                //if (result == 1) {
                    snprintf(response, 1024, "SUCCESS: oled(connect) - OLED connected");
                // } else {
                //     snprintf(response, 1024, "ERROR: oled(connect) - failed to connect OLED");
                // }
                //return result == 1 ? 0 : -1;
                return 0;
            } else if (parsedCmd.subAction == oled_sub_disconnect) {
                // Example: oled(disconnect)
                oled.disconnect();
                snprintf(response, 1024, "SUCCESS: oled(disconnect) - OLED disconnected");
                return 0;
            } else if (parsedCmd.subAction == oled_sub_isConnected) {
                // Example: oled(isConnected)
                bool connected = oled.isConnected();
                snprintf(response, 1024, "SUCCESS: oled(isConnected) = %s", connected ? "true" : "false");
                return 0;
            } else if (parsedCmd.subAction == oled_sub_setTextSize) {
                // Example: oled(setTextSize, 2)
                if (parsedCmd.argCount >= 1 && parsedCmd.args[0].type == ARG_TYPE_INT) {
                    int size = parsedCmd.args[0].value.intValue;
                    oled.setTextSize(size);
                    snprintf(response, 1024, "SUCCESS: oled(setTextSize, %d)", size);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for oled(setTextSize) - need integer size");
                return -1;
            } else if (parsedCmd.subAction == oled_sub_setCursor) {
                // Example: oled(setCursor, 10, 20)
                if (parsedCmd.argCount >= 2 && 
                    parsedCmd.args[0].type == ARG_TYPE_INT && 
                    parsedCmd.args[1].type == ARG_TYPE_INT) {
                    int x = parsedCmd.args[0].value.intValue;
                    int y = parsedCmd.args[1].value.intValue;
                    oled.setCursor(x, y);
                    snprintf(response, 1024, "SUCCESS: oled(setCursor, %d, %d)", x, y);
                    return 0;
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for oled(setCursor) - need x,y coordinates");
                return -1;
            } else if (parsedCmd.subAction == oled_sub_cycleFont) {
                // Example: oled(cycleFont)
                int fontIndex = oled.cycleFont();
                snprintf(response, 1024, "SUCCESS: oled(cycleFont) - switched to font %d", fontIndex);
                return 0;
            } else if (parsedCmd.subAction == oled_sub_setFont) {
                // Example: oled(setFont, "Eurostile") or oled(setFont, 0)
                if (parsedCmd.argCount >= 1) {
                    if (parsedCmd.args[0].type == ARG_TYPE_STRING) {
                        String fontName = parsedCmd.args[0].value.stringValue;
                        int result = oled.setFont(fontName);
                        snprintf(response, 1024, "SUCCESS: oled(setFont, \"%s\") - index %d", 
                                fontName.c_str(), result);
                        return 0;
                    } else if (parsedCmd.args[0].type == ARG_TYPE_INT) {
                        int fontIndex = parsedCmd.args[0].value.intValue;
                        oled.setFont(fontIndex);
                        snprintf(response, 1024, "SUCCESS: oled(setFont, %d)", fontIndex);
                        return 0;
                    }
                }
                snprintf(response, 1024, "ERROR: Invalid arguments for oled(setFont) - need font name or index");
                return -1;
            }
            
            snprintf(response, 1024, "ERROR: oled(%s) - sub-action not implemented", 
                    subActionToString(parsedCmd.action, parsedCmd.subAction));
            return -1;
        }
        
        case action_slot: {
            if (parsedCmd.subAction == slot_sub_set) {
                // Example: slot(set, number)
                snprintf(response, 1024, "SUCCESS: slot(set) - function not yet implemented");
                return 0;
            }
            break;
        }
        
        case action_breadboard: {
            snprintf(response, 1024, "SUCCESS: breadboard(%s) - function not yet implemented", 
                    subActionToString(parsedCmd.action, parsedCmd.subAction));
            return 0;
        }
        
        default: {
            snprintf(response, 1024, "ERROR: Action category '%s' with sub-action %d not implemented yet", 
                    actionTypeToString(parsedCmd.action), parsedCmd.subAction);
            return -1;
        }
    }

    snprintf(response, 1024, "ERROR: Sub-action %d not implemented for %s", 
            parsedCmd.subAction, actionTypeToString(parsedCmd.action));
    return -1;
}

#define MICROPYTHON_HEAP_SIZE 32 * 1024
bool microPythonInitialized = false;

bool initMicroPython(void) {
  // Start with smaller heap to avoid memory issues
  static char heap[MICROPYTHON_HEAP_SIZE] __attribute__((aligned(8)));

  //   Serial.println("Heap allocated successfully");
  //   Serial.flush();
  //   delay(100);

  // Get a more reliable stack pointer
  char stack_dummy;
  char *stack_top = &stack_dummy;

  Serial.println("Initializing MicroPython REPL");
  Serial.println("Heap size: " + String(sizeof(heap)) + " bytes");
  Serial.flush();

  mp_embed_init(&heap[0], sizeof(heap), stack_top);

  // Auto-load JythonModule.py if it exists
  Serial.println("Loading Jumperless module...");
  
  // Try to execute the JythonModule.py file from various locations
  // We use a simple approach - test for existence first, then execute
  
  // Check for file existence and load
//   String checkCommands[] = {
//     "try:\n  exec(open('JythonModule.py').read())\n  print('MODULE_LOADED_0')\nexcept Exception as e: print('FAILED_0:', e)",         // Current directory
//     "try:\n  exec(open('/JythonModule.py').read())\n  print('MODULE_LOADED_1')\nexcept Exception as e: print('FAILED_1:', e)",        // Root directory  
//     "try:\n  exec(open('src/JythonModule.py').read())\n  print('MODULE_LOADED_2')\nexcept Exception as e: print('FAILED_2:', e)",     // src subdirectory
//     "try:\n  exec(open('/src/JythonModule.py').read())\n  print('MODULE_LOADED_3')\nexcept Exception as e: print('FAILED_3:', e)",    // /src directory
//     "try:\n  exec(open('../JythonModule.py').read())\n  print('MODULE_LOADED_4')\nexcept Exception as e: print('FAILED_4:', e)",      // Parent directory
//     "try:\n  exec(open('./src/JythonModule.py').read())\n  print('MODULE_LOADED_5')\nexcept Exception as e: print('FAILED_5:', e)"   // Explicit ./src
//   };
  
//   bool module_loaded = false;
//   for (int i = 0; i < 6; i++) {
//     // Execute the check command - MicroPython will print MODULE_LOADED_X if successful
//     mp_embed_exec_str(checkCommands[i].c_str());
//     delay(10); // Small delay to allow output
//     // Note: We can't easily capture MicroPython output here, but the module will be loaded if successful
//   }
  
//   // Also try to see what's in the current directory
//   mp_embed_exec_str("try:\n  import os\n  print('CWD:', os.getcwd())\nexcept: pass");
//   mp_embed_exec_str("try:\n  import os\n  print('LISTDIR:', os.listdir('.'))\nexcept: pass");
  // Execute full embedded JythonModule
  const char* full_module = R"""(

_on_hardware = True

# Global variables for synchronous execution (must be initialized)
_sync_result_ready = False
_sync_value = ''
_sync_type = ''
_sync_result = 0

def _execute_sync(cmd):
    """Execute a command synchronously and return the actual result"""
    global _sync_result_ready, _sync_value, _sync_type, _sync_result
    _sync_result_ready = False
    print('SYNC_EXEC:' + cmd)
    # Wait briefly for C code to process - try different timing approaches
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

class JumperlessError(Exception):
    pass

def _execute_command(cmd):
    if _on_hardware:
        # Use synchronous execution that returns actual typed results
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
    def set_cursor(self, x, y):
        cmd = 'oled(setCursor, ' + str(x) + ', ' + str(y) + ')'
        return _execute_command(cmd)
    def set_text_size(self, size):
        cmd = 'oled(setTextSize, ' + str(size) + ')'
        return _execute_command(cmd)
    def cycle_font(self):
        cmd = 'oled(cycleFont)'
        return _execute_command(cmd)
    def set_font(self, font):
        if isinstance(font, str):
            cmd = 'oled(setFont, "' + str(font) + '")'
        else:
            cmd = 'oled(setFont, ' + str(font) + ')'
        return _execute_command(cmd)
    def is_connected(self):
        cmd = 'oled(isConnected)'
        return _execute_command(cmd)

class Arduino:
    def reset(self):
        cmd = 'arduino(reset)'
        return _execute_command(cmd)
    def flash(self):
        cmd = 'arduino(flash)'
        return _execute_command(cmd)

class UART:
    def connect(self):
        cmd = 'uart(connect)'
        return _execute_command(cmd)
    def disconnect(self):
        cmd = 'uart(disconnect)'
        return _execute_command(cmd)

class INA:
    def get_current(self, sensor):
        cmd = 'ina(getCurrent, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_voltage(self, sensor):
        cmd = 'ina(getVoltage, ' + str(sensor) + ')'
        return _execute_command(cmd)
    def get_bus_voltage(self, sensor):
        cmd = 'ina(getBusVoltage, ' + str(sensor) + ')'
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
    def hold(self, time=0.5):
        cmd = 'clickwheel(hold, time=' + str(time) + ')'
        return _execute_command(cmd)
    def get_press(self):
        cmd = 'clickwheel(get_press)'
        return _execute_command(cmd)

# Create module instances
dac = DAC()
adc = ADC()
ina = INA()
gpio = GPIO()
nodes = Nodes()
oled = OLED()
arduino = Arduino()
uart = UART()
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
        self.uart = uart
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
    
    def reset_arduino(self):
        return self.arduino.reset()
    
    def help(self):
        print('Jumperless MicroPython Module (Full Embedded)\n\r')
        print('Main Namespace: jl.*\n\r')
        print('Hardware Modules:\n\r')
        print('  jl.dac.set(0, 2.5)      # Set DAC voltage')
        print('  jl.adc.get(0)           # Read ADC')
        print('  jl.ina.get_current(0)   # Read INA current')
        print('  jl.gpio.set(5, "HIGH")  # Set GPIO')
        print('  jl.nodes.connect(1, 5)  # Connect nodes')
        print('  jl.oled.print("Hi")     # Display text')
        print('  jl.arduino.reset()      # Reset Arduino')
        print('  jl.uart.connect()       # Connect UART')
        print('  jl.clickwheel.up(1)     # Scroll up')
        print('  jl.clickwheel.down(3)   # Scroll down 3 clicks')
        print('  jl.clickwheel.press()   # Press clickwheel')
        print('  jl.clickwheel.hold(0.5) # Hold for 0.5 seconds')
        print('\n\rConvenience methods:\n\r')
        print('  jl.read_voltage(0)      # Quick ADC read')
        print('  jl.set_voltage(0, 2.5)  # Quick DAC set')
        print('  jl.display("Hello!")    # Quick OLED print')
        print('  jl.connect_nodes(1, 5)  # Quick node connect')
        print('\n\rNew: Return Values & Variables:\n\r')
        print('voltage = jl.adc.get(0)        # Get actual voltage')
        print('current = jl.ina.get_current(0) # Get actual current')
        print('pin_state = jl.gpio.get(5)     # Get actual state')
        print('if jl.gpio.get(1):\n\r\tprint("Pin 1 is HIGH")')
        print('if voltage > 3.0:\n\r\tjl.oled.print("High voltage!")')
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
)""";
  // Execute the full embedded module
  Serial.println("Executing full embedded module...");
  mp_embed_exec_str(full_module);
  Serial.println("Full embedded module execution completed.");
  

  return true;
}

void deinitMicroPython(void) {
  if (!microPythonInitialized) return;
  mp_embed_deinit();
  microPythonInitialized = false;
}


void executeMicroPython(const char* command) {
  if (!microPythonInitialized) return;
  mp_embed_exec_str(command);
}

void micropython(void) {
  char heap[MICROPYTHON_HEAP_SIZE];

  // Initialise MicroPython.
  //
  // Note: &stack_top below should be good enough for many cases.
  // However, depending on environment, there might be more appropriate
  // ways to get the stack top value.
  // eg. pthread_get_stackaddr_np, pthread_getattr_np,
  // __builtin_frame_address/__builtin_stack_address, etc.
  int stack_top;
  mp_embed_init(&heap[0], sizeof(heap), &stack_top);

  // Run the example scripts (they will be compiled first).
  mp_embed_exec_str(
      "print('hello world!', list(x + 1 for x in range(10)), end='eol\\n')");

  // Deinitialise MicroPython.
  mp_embed_deinit();
}

void micropythonREPL(void) {

  ///@brief 0 = menu, 1 = prompt, 2 = output, 3 = input, 4 = error, 5 = prompt
  ///alt 1, 6 = prompt alt 2, 7 = prompt alt 3
  int replColors[8] = {
      38,  // menu
      69,  // prompt
      155, // output
      221, // input
      202, // error
      62,  // prompt alt 1
      56,  // prompt alt 2
      51,  // prompt alt 3
  };
  changeTerminalColor(replColors[6], true);
  initMicroPython();

  
  // Always show helpful messages
  changeTerminalColor(replColors[7], true);
  Serial.println("MicroPython REPL with embedded Jumperless hardware control!");
  Serial.println("Type normal Python code, then 'run' to execute");
  Serial.println("Remember: for loops need colons → for i in range(5):");
  Serial.println("Use TAB for indentation (or exactly 4 spaces)");
  Serial.println("Type help_jumperless() for hardware control commands");

  changeTerminalColor(replColors[0], true);
  Serial.println("MicroPython initialized successfully");
  Serial.flush();
  delay(200);

  changeTerminalColor(replColors[0], true);
  Serial.println();
  changeTerminalColor(replColors[2], true);
  Serial.println("    MicroPython REPL\n\r");
  //   Serial.println("Multi-line mode: Enter adds newlines");
  changeTerminalColor(replColors[5], true);
  Serial.println(" Commands:");
  changeTerminalColor(replColors[3], false);
  Serial.print("  'run'  ");
  changeTerminalColor(replColors[0], false);
  Serial.println(" -   Run code in buffer");
  changeTerminalColor(replColors[3], false);
  Serial.print("  'clear'");
  changeTerminalColor(replColors[0], false);
  Serial.println(" -   Clear input buffer");
  changeTerminalColor(replColors[3], false);
  Serial.print("  'quit' ");
  changeTerminalColor(replColors[0], false);
  Serial.println(" -   Exit REPL");
  changeTerminalColor(replColors[3], false);
  Serial.print("  'help' ");
  changeTerminalColor(replColors[0], false);
  Serial.println(" -   Show this help");
  changeTerminalColor(replColors[3], false);
  Serial.print("  'mem'  ");
  changeTerminalColor(replColors[0], false);
  Serial.println(" -   Show memory info");
  changeTerminalColor(replColors[7], false);
  Serial.println("\n\r Press clickwheel to exit");
  //   Serial.println("========================");

  Serial.write(0x0E); // turn on interactive mode (only works on the app)
  // Serial.println();
  Serial.flush();
  //   delay(100);
  //   if (Serial.available() > 0) {
  //     Serial.read();
  //   }
  changeTerminalColor(replColors[4], true);
  Serial.print(" Press enter to start REPL");
  Serial.println();
  Serial.flush();
  while (Serial.available() == 0) {
    delay(1);
  }
  Serial.read();
  // Serial.println();
  // changeTerminalColor(replColors[2],true);
  //   Serial.println("Starting REPL...");
  //   Serial.println();
  changeTerminalColor(replColors[1], true);
  Serial.print(">>> ");
  Serial.flush();

  String inputBuffer = "";
  String currentLine = "";

  // Add variables for newline debouncing
  static unsigned long lastNewlineTime = 0;
  static char lastChar = 0;
  const unsigned long NEWLINE_DEBOUNCE_MS = 50; // Debounce time in milliseconds

  // REPL loop
  while (true) {
    // Check for encoder button press to exit
    if (digitalRead(BUTTON_ENC) == 0) {
      changeTerminalColor(replColors[0], true);
      Serial.println("\nExiting REPL...");
      delay(500); // debounce
      break;
    }

    // Check for serial data available - handle character by character
    if (Serial.available() > 0) {
      char c = Serial.read();
      unsigned long currentTime = millis();

      // Handle special characters
      if (c == '\r' || c == '\n') {
        // Debounce consecutive newline characters
        bool isConsecutiveNewline =
            (lastChar == '\r' || lastChar == '\n') &&
            (currentTime - lastNewlineTime < NEWLINE_DEBOUNCE_MS);

        // Only process newline if it's not a consecutive duplicate or if
        // there's actual content
        if (!isConsecutiveNewline || currentLine.length() > 0) {
          // End of line - process the input
          if (currentLine.startsWith(">>> ")) {
            currentLine.remove(0, 4);
          }
          Serial.println(); // Echo the newline

          // Update debounce tracking
          lastNewlineTime = currentTime;

          // Handle special commands
          if (currentLine == "exit" || currentLine == "quit") {
            Serial.println("Exiting REPL...");
            break;
          }

          if (currentLine == "help") {
            changeTerminalColor(replColors[0], true);
            Serial.println("MicroPython REPL Help:");
            Serial.println("  run or ESC - Execute the Python code in buffer");
            Serial.println("  clear     - Clear the input buffer");
            Serial.println("  show      - Show current buffer content");
            Serial.println("  quit      - Exit REPL");
            Serial.println("  mem       - Show memory info");
            Serial.println("  help      - Show this help");
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          if (currentLine == "mem") {
            changeTerminalColor(replColors[0], true);
            mp_embed_exec_str("import gc; print('Free:', gc.mem_free(), "
                              "'Allocated:', gc.mem_alloc())");
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          if (currentLine == "run" || currentLine == "\033") {
            if (inputBuffer.length() > 0) {
              changeTerminalColor(replColors[7], true);
              Serial.println("\n\r Executing:\n\r");
              changeTerminalColor(replColors[3], true);
              inputBuffer.replace("\n", "\n\r");
              Serial.println(inputBuffer);
              changeTerminalColor(replColors[6], true);
              Serial.println("\n\r--- Output ---\n\r");
              changeTerminalColor(replColors[2], true);
              mp_embed_exec_str(inputBuffer.c_str());
              processQueuedCommands();
              changeTerminalColor(replColors[0], true);
              Serial.println("\n\r--- End ---\n\r");

              inputBuffer = "";
            } else {
              changeTerminalColor(replColors[4], true);
              Serial.println("Nothing to execute (buffer is empty)\n\r");
            }
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          if (currentLine == "clear") {
            inputBuffer = "";
            changeTerminalColor(replColors[0], true);
            Serial.println("Buffer cleared\n\r");
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          if (currentLine == "show") {
            if (inputBuffer.length() > 0) {
              changeTerminalColor(replColors[3], true);
              Serial.println("\n\rCurrent buffer:\n\r");
              Serial.println(inputBuffer);
            } else {
              changeTerminalColor(replColors[4], true);
              Serial.println("\n\rBuffer is empty\n\r");
            }
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          // Handle empty input - just add a newline to buffer
          if (currentLine.length() == 0) {
            if (inputBuffer.length() > 0) {
              inputBuffer += "\n";
            }
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          // Clean up any prompt prefixes that might have leaked through
          if (currentLine.startsWith(">>> ")) {
            currentLine.remove(0, 4);
          }
          if (currentLine.startsWith("... ")) {
            currentLine.remove(0, 4);
          }

          // Normalize excessive indentation (more than 8 spaces gets reduced to 4)
          if (currentLine.length() > 0) {
            int leadingSpaces = 0;
            for (int i = 0; i < currentLine.length() && currentLine[i] == ' '; i++) {
              leadingSpaces++;
            }
            if (leadingSpaces > 8) {
              // Replace excessive indentation with reasonable amount
              String content = currentLine.substring(leadingSpaces);
              currentLine = "    " + content;  // 4 spaces for Python indentation
            }
          }

          // Add current line to buffer with proper Python line endings
          // Python expects Unix-style \n line endings, not \n\r
          if (inputBuffer.length() > 0) {
            inputBuffer += "\n";  // Use Unix line endings for Python
          }
          inputBuffer += currentLine;

          // Show continuation prompt
          changeTerminalColor(replColors[1], true);
          Serial.print("... ");
          Serial.flush();
          currentLine = "";

        } // End of debounce check

        // Update lastChar for next iteration
        lastChar = c;

      } else if (c == ' ' && currentLine.startsWith(">>>") &&
                 currentLine.length() == 4) {
        currentLine.remove(0, 4);
        Serial.print("\b\b\b\b");
        Serial.flush();
        continue;
        //   } else if (c == '>' && currentLine.startsWith(">>") &&
        //   currentLine.length() == 3) {
        //     currentLine.remove(0,3);
        //     Serial.print("\b\b\b");
        //     Serial.flush();
        //     continue;
      } else if (c == ' ' && currentLine.startsWith("...") &&
                 currentLine.length() == 4) {
        currentLine.remove(0, 4);
        Serial.print("\b\b\b\b");
        Serial.flush();
        continue;

      } else if (c == '\b' || c == 0x7F) {
        // Backspace - remove last character
        if (currentLine.length() > 0) {
          currentLine.remove(currentLine.length() - 1);
          Serial.print("\b \b"); // Erase character on terminal
          Serial.flush();
        }
        lastChar = c;

      } else if (c == '\t') {
        // Tab character - add 4 spaces for Python indentation
        currentLine += "    "; // 4 spaces for tab
        changeTerminalColor(replColors[3], true);
        Serial.print("    "); // Echo 4 spaces
        Serial.flush();
        lastChar = c;

      } else if (c >= 32 && c <= 126) {
        // Printable character - add to current line and echo
        currentLine += c;
        changeTerminalColor(replColors[3], true);
        Serial.print(c); // Echo the character
        Serial.flush();
        lastChar = c;

      } else {
        // Ignore other control characters but still track them
        lastChar = c;
      }
    }

    delayMicroseconds(10); // Small delay to prevent overwhelming the system
    
    // Process any queued commands from MicroPython
    
  }

  // Cleanup
  Serial.println("Deinitializing MicroPython...");
  mp_embed_deinit();
  Serial.println("MicroPython REPL session ended");

  while (Serial.available() > 0) {
    Serial.read();
  }
  Serial.write(0x0F); // turn off interactive mode (only works on the app)
  Serial.flush();
  delay(10);
}

const char* argTypeToString(enum argType type) {
    switch (type) {
        case ARG_TYPE_NONE: return "NONE";
        case ARG_TYPE_INT: return "INT";
        case ARG_TYPE_FLOAT: return "FLOAT";
        case ARG_TYPE_STRING: return "STRING";
        case ARG_TYPE_BOOL: return "BOOL";
        default: return "UNKNOWN";
    }
}

const char* actionTypeToString(enum actionType action) {
    switch (action) {
        case action_none: return "none";
        case action_gpio: return "gpio";
        case action_nodes: return "nodes";
        case action_dac: return "dac";
        case action_adc: return "adc";
        case action_ina: return "ina";
        case action_config: return "config";
        case action_arduino: return "arduino";
        case action_uart: return "uart";
        case action_probe: return "probe";
        case action_clickwheel: return "clickwheel";
        case action_oled: return "oled";
        case action_display: return "display";
        case action_slot: return "slot";
        case action_breadboard: return "breadboard";
        default: return "unknown";
    }
}

void printParsedCommand(const pythonCommand* parsedCommand) {
    if (!parsedCommand) {
        Serial.println("NULL command");
        return;
    }
    
    Serial.print("Function: ");
    Serial.println(parsedCommand->functionName);
    Serial.print("Action: ");
    Serial.println(actionTypeToString(parsedCommand->action));
    
    // Show sub-action if available
    if (parsedCommand->subAction != -1) {
        Serial.print("Sub-action: ");
        Serial.print(subActionToString(parsedCommand->action, parsedCommand->subAction));
        Serial.print(" (");
        Serial.print(parsedCommand->subAction);
        Serial.println(")");
    }
    
    Serial.print("Argument count: ");
    Serial.println(parsedCommand->argCount);
    
    for (int i = 0; i < parsedCommand->argCount; i++) {
        Serial.print("  Arg ");
        Serial.print(i);
        Serial.print(": ");
        
        if (strlen(parsedCommand->args[i].name) > 0) {
            Serial.print(parsedCommand->args[i].name);
            Serial.print("=");
        }
        
        Serial.print("[");
        Serial.print(argTypeToString(parsedCommand->args[i].type));
        Serial.print("] ");
        
        switch (parsedCommand->args[i].type) {
            case ARG_TYPE_INT:
                Serial.println(parsedCommand->args[i].value.intValue);
                break;
            case ARG_TYPE_FLOAT:
                Serial.println(parsedCommand->args[i].value.floatValue);
                break;
            case ARG_TYPE_STRING:
                Serial.print("\"");
                Serial.print(parsedCommand->args[i].value.stringValue);
                Serial.println("\"");
                break;
            case ARG_TYPE_BOOL:
                Serial.println(parsedCommand->args[i].value.boolValue ? "true" : "false");
                break;
            default:
                Serial.println("(no value)");
                break;
        }
    }
    
    Serial.print("Result: ");
    switch (parsedCommand->result) {
        case action_result_success:
            Serial.println("SUCCESS");
            break;
        case action_result_error:
            Serial.println("ERROR");
            break;
        case action_result_invalid_command:
            Serial.println("INVALID_COMMAND");
            break;
        case action_result_invalid_parameter:
            Serial.println("INVALID_PARAMETER");
            break;
        case action_result_invalid_state:
            Serial.println("INVALID_STATE");
            break;
        case action_result_invalid_function:
            Serial.println("INVALID_FUNCTION");
            break;
        case action_result_invalid_subaction:
            Serial.println("INVALID_SUBACTION");
            break;
        case action_result_invalid_parameter_type:
            Serial.println("INVALID_PARAMETER_TYPE");
            break;
        case action_result_missing_parameter:
            Serial.println("MISSING_PARAMETER");
            break;
        default:
            Serial.println("UNKNOWN_ERROR");
    }
    Serial.println();
}

void testPythonParser() {
    Serial.println("\n=== Python Command Parser Test ===\n");
    
    // Test commands - both legacy and new hierarchical syntax
    const char* testCommands[] = {
        // Legacy syntax
        "setDac(0, 2.5, save=False)",
        "connectNodes(1, 3, save=False)", 
        "simulateProbe(tap, D7)",
        "setGpio(3, HIGH)",
        "getAdc(0)",
        "getInaCurrent(0)",
        "getInaVoltage(0)",
        "getInaBusVoltage(0)",
        "getInaPower(0)",
        "removeNodes(5, 8, save=True)",
        
        // New hierarchical syntax
        "dac(set, 0, 2.5, save=False)",
        "nodes(connect, 1, 3, save=False)",
        "probe(tap, D7)",
        "gpio(set, 3, HIGH)",
        "adc(get, 0)",
        "nodes(remove, 5, 8)",
        "gpio(direction, 2, OUTPUT)",
        "config(set, routing, stack_paths, 3)",
        "oled(print, \"Hello World\", 10, 20)",
        "display(startup)",
        "arduino(reset)",
        "uart(connect)",
        "clickwheel(up, 1)",
        "clickwheel(down, 3)",
        "clickwheel(press)",
        "clickwheel(hold, time=0.5)",
        "clickwheel(get_press)",
        
        // Error cases
        "invalidFunction(1, 2, 3)",
        "gpio(invalidAction, 1)",
        "dac(set)"  // missing parameters
    };
    
    int numTests = sizeof(testCommands) / sizeof(testCommands[0]);
    
    for (int i = 0; i < numTests; i++) {
        Serial.print("Test ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(testCommands[i]);
        Serial.println("---");
        
        pythonCommand parsedCmd;
        enum actionResult result = parsePythonCommandString(testCommands[i], &parsedCmd);
        
        if (result == action_result_success) {
            printParsedCommand(&parsedCmd);
        } else {
            Serial.print("PARSE FAILED: ");
            switch (result) {
                case action_result_invalid_command:
                    Serial.println("Invalid command format");
                    break;
                case action_result_invalid_function:
                    Serial.println("Invalid function name");
                    break;
                case action_result_invalid_subaction:
                    Serial.println("Invalid sub-action");
                    break;
                case action_result_invalid_parameter_type:
                    Serial.println("Invalid parameter type");
                    break;
                case action_result_missing_parameter:
                    Serial.println("Missing required parameter");
                    break;
                default:
                    Serial.println("Unknown error");
                    break;
            }
            Serial.println();
        }
        
        Serial.println("==============================\n");
    }
    
    Serial.println("Parser test completed!");
}

void testPythonExecution() {
    Serial.println("\n=== Python Command Execution Test ===\n");
    
    // Test commands that will actually execute
    const char* testCommands[] = {
        "adc(get, 0)",
        "ina(getCurrent, 0)",
        "ina(getVoltage, 0)",
        "ina(getBusVoltage, 1)",
        "ina(getPower, 0)",
        "dac(set, 0, 2.5)",
        "nodes(connect, 1, 5)",
        "gpio(set, 13, HIGH)",
        "gpio(get, 13)",
        "arduino(reset)",
        "uart(connect)",
        "nodes(remove, 1, 5)",
        "oled(connect)",
        "oled(print, \"Hello OLED!\", 2)",
        "oled(show)",
        "oled(clear)",
        "oled(setTextSize, 1)",
        "oled(setCursor, 10, 10)",
        "oled(cycleFont)",
        "oled(isConnected)"
    };
    
    int numTests = sizeof(testCommands) / sizeof(testCommands[0]);
    
    for (int i = 0; i < numTests; i++) {
        Serial.print("Execute ");
        Serial.print(i + 1);
        Serial.print(": ");
        Serial.println(testCommands[i]);
        Serial.print("Result: ");
        
        char response[1024];
        int result = parseAndExecutePythonCommand((char*)testCommands[i], response);
        
        Serial.println(response);
        Serial.println("---");
        
        delay(500); // Small delay between commands
    }
    
    Serial.println("Execution test completed!");
}

// Sub-action parsing functions
int parseGpioSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "set") == 0) return gpio_sub_set;
    if (strcmp(subActionStr, "get") == 0) return gpio_sub_get;
    if (strcmp(subActionStr, "direction") == 0) return gpio_sub_direction;
    if (strcmp(subActionStr, "pulls") == 0) return gpio_sub_pulls;
    return -1;
}

int parseNodesSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "connect") == 0) return nodes_sub_connect;
    if (strcmp(subActionStr, "add") == 0) return nodes_sub_add;
    if (strcmp(subActionStr, "remove") == 0) return nodes_sub_remove;
    if (strcmp(subActionStr, "load") == 0) return nodes_sub_load;
    if (strcmp(subActionStr, "clear") == 0) return nodes_sub_clear;
    return -1;
}

int parseDacSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "set") == 0) return dac_sub_set;
    if (strcmp(subActionStr, "get") == 0) return dac_sub_get;
    return -1;
}

int parseAdcSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "get") == 0) return adc_sub_get;
    return -1;
}

int parseInaSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "getCurrent") == 0) return ina_sub_getCurrent;
    if (strcmp(subActionStr, "getVoltage") == 0) return ina_sub_getVoltage;
    if (strcmp(subActionStr, "getBusVoltage") == 0) return ina_sub_getBusVoltage;
    if (strcmp(subActionStr, "getPower") == 0) return ina_sub_getPower;
    return -1;
}

int parseConfigSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "set") == 0) return config_sub_set;
    if (strcmp(subActionStr, "get") == 0) return config_sub_get;
    if (strcmp(subActionStr, "save") == 0) return config_sub_save;
    return -1;
}

int parseArduinoSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "reset") == 0) return arduino_sub_reset;
    if (strcmp(subActionStr, "flash") == 0) return arduino_sub_flash;
    return -1;
}

int parseUartSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "connect") == 0) return uart_sub_connect;
    if (strcmp(subActionStr, "disconnect") == 0) return uart_sub_disconnect;
    return -1;
}

int parseProbeSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "tap") == 0) return probe_sub_tap;
    if (strcmp(subActionStr, "click") == 0) return probe_sub_click;
    if (strcmp(subActionStr, "short_click") == 0) return probe_sub_short_click;
    if (strcmp(subActionStr, "release") == 0) return probe_sub_release;
    return -1;
}

int parseClickwheelSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "up") == 0) return clickwheel_sub_up;
    if (strcmp(subActionStr, "down") == 0) return clickwheel_sub_down;
    if (strcmp(subActionStr, "press") == 0) return clickwheel_sub_press;
    if (strcmp(subActionStr, "hold") == 0) return clickwheel_sub_hold;
    if (strcmp(subActionStr, "get_press") == 0) return clickwheel_sub_get_press;
    return -1;
}

int parseOledSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "connect") == 0) return oled_sub_connect;
    if (strcmp(subActionStr, "disconnect") == 0) return oled_sub_disconnect;
    if (strcmp(subActionStr, "clear") == 0) return oled_sub_clear;
    if (strcmp(subActionStr, "print") == 0) return oled_sub_print;
    if (strcmp(subActionStr, "show") == 0) return oled_sub_show;
    if (strcmp(subActionStr, "setCursor") == 0) return oled_sub_setCursor;
    if (strcmp(subActionStr, "setTextSize") == 0) return oled_sub_setTextSize;
    if (strcmp(subActionStr, "cycleFont") == 0) return oled_sub_cycleFont;
    if (strcmp(subActionStr, "setFont") == 0) return oled_sub_setFont;
    if (strcmp(subActionStr, "isConnected") == 0) return oled_sub_isConnected;
    if (strcmp(subActionStr, "init") == 0) return oled_sub_init;
    return -1;
}

int parseDisplaySubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "startup") == 0) return display_sub_startup;
    if (strcmp(subActionStr, "menu") == 0) return display_sub_menu;
    if (strcmp(subActionStr, "state") == 0) return display_sub_state;
    if (strcmp(subActionStr, "config") == 0) return display_sub_config;
    return -1;
}

int parseSlotSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "set") == 0) return slot_sub_set;
    return -1;
}

int parseBreadboardSubAction(const char* subActionStr) {
    if (strcmp(subActionStr, "print") == 0) return breadboard_sub_print;
    if (strcmp(subActionStr, "clear") == 0) return breadboard_sub_clear;
    if (strcmp(subActionStr, "printRawRow") == 0) return breadboard_sub_print_raw_row;
    return -1;
}

// Helper function to parse sub-action based on action type
int parseSubAction(enum actionType action, const char* subActionStr) {
    switch (action) {
        case action_gpio: return parseGpioSubAction(subActionStr);
        case action_nodes: return parseNodesSubAction(subActionStr);
        case action_dac: return parseDacSubAction(subActionStr);
        case action_adc: return parseAdcSubAction(subActionStr);
        case action_ina: return parseInaSubAction(subActionStr);
        case action_config: return parseConfigSubAction(subActionStr);
        case action_arduino: return parseArduinoSubAction(subActionStr);
        case action_uart: return parseUartSubAction(subActionStr);
        case action_probe: return parseProbeSubAction(subActionStr);
        case action_clickwheel: return parseClickwheelSubAction(subActionStr);
        case action_oled: return parseOledSubAction(subActionStr);
        case action_display: return parseDisplaySubAction(subActionStr);
        case action_slot: return parseSlotSubAction(subActionStr);
        case action_breadboard: return parseBreadboardSubAction(subActionStr);
        default: return -1;
    }
}

// Helper function to determine sub-action from legacy function names
int getLegacySubAction(const char* functionName, enum actionType action) {
    // Map legacy function names to sub-actions
    if (action == action_dac) {
        if (strncmp(functionName, "set", 3) == 0) return dac_sub_set;
        if (strncmp(functionName, "get", 3) == 0) return dac_sub_get;
    } else if (action == action_adc) {
        if (strncmp(functionName, "get", 3) == 0) return adc_sub_get;
    } else if (action == action_ina) {
        if (strstr(functionName, "Current")) return ina_sub_getCurrent;
        if (strstr(functionName, "Voltage") && strstr(functionName, "Bus")) return ina_sub_getBusVoltage;
        if (strstr(functionName, "Voltage")) return ina_sub_getVoltage;
        if (strstr(functionName, "Power")) return ina_sub_getPower;
    } else if (action == action_gpio) {
        if (strncmp(functionName, "set", 3) == 0) {
            if (strstr(functionName, "Direction")) return gpio_sub_direction;
            if (strstr(functionName, "Pulls")) return gpio_sub_pulls;
            return gpio_sub_set;
        }
        if (strncmp(functionName, "get", 3) == 0) {
            if (strstr(functionName, "Direction")) return gpio_sub_direction;
            if (strstr(functionName, "Pulls")) return gpio_sub_pulls;
            return gpio_sub_get;
        }
    } else if (action == action_nodes) {
        if (strstr(functionName, "connect") || strstr(functionName, "add")) return nodes_sub_connect;
        if (strstr(functionName, "remove")) return nodes_sub_remove;
        if (strstr(functionName, "load")) return nodes_sub_load;
        if (strstr(functionName, "clear")) return nodes_sub_clear;
    } else if (action == action_config) {
        if (strncmp(functionName, "set", 3) == 0) return config_sub_set;
        if (strncmp(functionName, "get", 3) == 0) return config_sub_get;
        if (strncmp(functionName, "save", 4) == 0) return config_sub_save;
    } else if (action == action_arduino) {
        if (strstr(functionName, "reset")) return arduino_sub_reset;
        if (strstr(functionName, "flash")) return arduino_sub_flash;
    } else if (action == action_uart) {
        if (strstr(functionName, "connect")) return uart_sub_connect;
        if (strstr(functionName, "disconnect")) return uart_sub_disconnect;
    } else if (action == action_probe) {
        if (strstr(functionName, "simulate")) return probe_sub_tap; // Default for legacy
    } else if (action == action_oled) {
        if (strstr(functionName, "connect")) return oled_sub_connect;
        if (strstr(functionName, "disconnect")) return oled_sub_disconnect;
        if (strstr(functionName, "clear") || strstr(functionName, "Clear")) return oled_sub_clear;
        if (strstr(functionName, "print") || strstr(functionName, "Print")) return oled_sub_print;
        if (strstr(functionName, "show") || strstr(functionName, "Show")) return oled_sub_show;
        if (strstr(functionName, "setCursor") || strstr(functionName, "SetCursor")) return oled_sub_setCursor;
        if (strstr(functionName, "setTextSize") || strstr(functionName, "SetTextSize")) return oled_sub_setTextSize;
        if (strstr(functionName, "cycleFont") || strstr(functionName, "CycleFont")) return oled_sub_cycleFont;
        if (strstr(functionName, "setFont") || strstr(functionName, "SetFont")) return oled_sub_setFont;
    } else if (action == action_display) {
        if (strstr(functionName, "Startup")) return display_sub_startup;
        if (strstr(functionName, "Menu")) return display_sub_menu;
        if (strstr(functionName, "State")) return display_sub_state;
        if (strstr(functionName, "Config")) return display_sub_config;
    } else if (action == action_slot) {
        if (strstr(functionName, "set")) return slot_sub_set;
    } else if (action == action_breadboard) {
        if (strstr(functionName, "clear")) return breadboard_sub_clear;
        if (strstr(functionName, "Raw")) return breadboard_sub_print_raw_row;
        if (strstr(functionName, "print")) return breadboard_sub_print;
    }
    
    return -1;
}

const char* subActionToString(enum actionType action, int subAction) {
    switch (action) {
        case action_gpio:
            switch (subAction) {
                case gpio_sub_set: return "set";
                case gpio_sub_get: return "get";
                case gpio_sub_direction: return "direction";
                case gpio_sub_pulls: return "pulls";
                default: return "unknown_gpio";
            }
        case action_nodes:
            switch (subAction) {
                case nodes_sub_connect: return "connect";
                case nodes_sub_add: return "add";
                case nodes_sub_remove: return "remove";
                case nodes_sub_load: return "load";
                case nodes_sub_clear: return "clear";
                default: return "unknown_nodes";
            }
        case action_dac:
            switch (subAction) {
                case dac_sub_set: return "set";
                case dac_sub_get: return "get";
                default: return "unknown_dac";
            }
        case action_adc:
            switch (subAction) {
                case adc_sub_get: return "get";
                default: return "unknown_adc";
            }
        case action_ina:
            switch (subAction) {
                case ina_sub_getCurrent: return "getCurrent";
                case ina_sub_getVoltage: return "getVoltage";
                case ina_sub_getBusVoltage: return "getBusVoltage";
                case ina_sub_getPower: return "getPower";
                default: return "unknown_ina";
            }
        case action_config:
            switch (subAction) {
                case config_sub_set: return "set";
                case config_sub_get: return "get";
                case config_sub_save: return "save";
                default: return "unknown_config";
            }
        case action_arduino:
            switch (subAction) {
                case arduino_sub_reset: return "reset";
                case arduino_sub_flash: return "flash";
                default: return "unknown_arduino";
            }
        case action_uart:
            switch (subAction) {
                case uart_sub_connect: return "connect";
                case uart_sub_disconnect: return "disconnect";
                default: return "unknown_uart";
            }
        case action_probe:
            switch (subAction) {
                case probe_sub_tap: return "tap";
                case probe_sub_click: return "click";
                case probe_sub_short_click: return "short_click";
                case probe_sub_release: return "release";
                default: return "unknown_probe";
            }
        case action_clickwheel:
            switch (subAction) {
                case clickwheel_sub_up: return "up";
                case clickwheel_sub_down: return "down";
                case clickwheel_sub_press: return "press";
                case clickwheel_sub_hold: return "hold";
                case clickwheel_sub_get_press: return "get_press";
                default: return "unknown_clickwheel";
            }
        case action_oled:
            switch (subAction) {
                case oled_sub_connect: return "connect";
                case oled_sub_disconnect: return "disconnect";
                case oled_sub_clear: return "clear";
                case oled_sub_print: return "print";
                case oled_sub_show: return "show";
                case oled_sub_setCursor: return "setCursor";
                case oled_sub_setTextSize: return "setTextSize";
                case oled_sub_cycleFont: return "cycleFont";
                case oled_sub_setFont: return "setFont";
                case oled_sub_isConnected: return "isConnected";
                case oled_sub_init: return "init";
                default: return "unknown_oled";
            }
        case action_display:
            switch (subAction) {
                case display_sub_startup: return "startup";
                case display_sub_menu: return "menu";
                case display_sub_state: return "state";
                case display_sub_config: return "config";
                default: return "unknown_display";
            }
        case action_slot:
            switch (subAction) {
                case slot_sub_set: return "set";
                default: return "unknown_slot";
            }
        case action_breadboard:
            switch (subAction) {
                case breadboard_sub_print: return "print";
                case breadboard_sub_clear: return "clear";
                case breadboard_sub_print_raw_row: return "printRawRow";
                default: return "unknown_breadboard";
            }
        default:
            return "unknown_action";
    }
}

bool executePythonScript(const char* filename) {
    if (!microPythonInitialized) {
        Serial.println("ERROR: MicroPython not initialized");
        return false;
    }
    
    // Try to read the file and execute it
    Serial.print("Executing Python script: ");
    Serial.println(filename);
    
    // Create the command to execute the file
    String command = "exec(open('";
    command += filename;
    command += "').read())";
    
    // Execute the script - note: error handling would need to be done at MicroPython level
    mp_embed_exec_str(command.c_str());
    Serial.println("Script executed");
    return true;
}

// C function that can be called from MicroPython to execute Jumperless commands
extern "C" const char* jumperless_execute_command(const char* command) {
    static char response[1024];
    int result = parseAndExecutePythonCommand((char*)command, response);
    return response;
}

// Synchronous execution function that can be called from embedded Python
extern "C" int jumperless_execute_sync_c(const char* command) {
    if (!command) {
        Serial.println("[SYNC_C] Error: null command");
        Serial.flush();
        return -1;
    }
    
    //Serial.print("[SYNC_C] Starting execution of: ");
    //Serial.println(command);
    //Serial.flush();
    
    // REAL HARDWARE: Use direct hardware execution instead of complex parser
    //Serial.println("[SYNC_C] Using direct hardware execution...");
    //Serial.flush();
    
    sync_execution_result = execute_hardware_direct(command);
    
    //Serial.print("[SYNC_C] Hardware execution completed with result: ");
    //Serial.println(sync_execution_result);
    
    // Skip the complex parsing for now
    int result = jumperless_execute_command_sync(command, sync_command_response);
    sync_execution_result = parse_response_for_python(sync_command_response, sync_value_result, sync_type_result);
    
    //Serial.print("[SYNC_C] Final result: ");
    Serial.print(sync_execution_result);
    //Serial.print(", Value: ");
    //Serial.print(sync_value_result);
    //Serial.print(", Type: ");
    //Serial.println(sync_type_result);
    Serial.flush();
    
    return sync_execution_result;
}

// Functions to get parsed results (callable from embedded Python)
extern "C" const char* jumperless_get_sync_value() {
    return sync_value_result;
}

extern "C" const char* jumperless_get_sync_type() {
    return sync_type_result;
}

extern "C" const char* jumperless_get_sync_response() {
    return sync_command_response;
}

// Helper function to get string representation of command results
extern "C" const char* jumperless_get_last_response() {
    return sync_response_buffer;
}

void setupJumperlessModule(void) {
    if (!microPythonInitialized) {
        Serial.println("Setting up MicroPython for Jumperless...");
        if (!initMicroPython()) {
            Serial.println("ERROR: Failed to initialize MicroPython");
            return;
        }
    }
    
    // The sync execution system is now built into JythonModule.py itself
    // No additional setup needed since the module contains all necessary functions
    
    Serial.println("Jumperless MicroPython module ready!");
    Serial.println("Import with: import JythonModule as jl");
    Serial.println("Examples:");
    Serial.println("  jl.dac.set(0, 2.5)");
    Serial.println("  jl.nodes.connect(1, 5)");
    Serial.println("  jl.oled.print('Hello!')");
    Serial.println("New: Use return values in variables and conditionals:");
    Serial.println("  voltage = jl.adc.get(0)");
    Serial.println("  if jl.gpio.get(1): print('Pin is HIGH')");
}

void testJumperlessModule(void) {
    Serial.println("\n=== Testing Jumperless MicroPython Module ===\n");
    
    setupJumperlessModule();
    
    Serial.println("\nRunning test commands...");
    
    // Test basic module functionality
    const char* testCommands[] = {
        "import JythonModule as jl",
        "print('Module imported successfully')",
        "jl.help()",
        "jl.dac.set(0, 2.5)",
        "jl.adc.get(0)", 
        "jl.gpio.set(5, jl.gpio.HIGH)",
        "jl.nodes.connect(1, 5)",
        "jl.oled.print('Test!')",
        "print('All tests completed')"
    };
    
    int numTests = sizeof(testCommands) / sizeof(testCommands[0]);
    
    for (int i = 0; i < numTests; i++) {
        Serial.print(">>> ");
        Serial.println(testCommands[i]);
        
        executeMicroPython(testCommands[i]);
        delay(100);
    }
    
    Serial.println("\nMicroPython module test completed!");
}

// Read a Python command from serial input
void readPythonCommand() {
    // Serial.print(">>> ");
    // Serial.flush();
    
    String command = "";
    char c;
    
    // Read until newline
    while (Serial.available() > 0) {

        
            c = Serial.read();
            if (c == '\n' || c == '\r') {
                //if (command.length() > 1) {
                    break;
                //}
                // Ignore empty lines
                //continue;
            } else if (c == '\b' || c == 0x7F) {
                // Backspace
                if (command.length() > 0) {
                    command.remove(command.length() - 1);
                    Serial.print("\b \b");
                    Serial.flush();
                }
            } else if (c >= 32 && c <= 255) {
                // Printable character
                command += c;
                //Serial.print(c);
                //Serial.print(" ");
                //Serial.flush();
            }
        
    }
    
    //Serial.println(); // Echo newline
    
    if (command.length() > 0) {
        char response[1024];
        if (processCommand(command.c_str(), response)) {
            Serial.println(response);
            Serial.flush();
        } else {
            Serial.println("ERROR: Invalid Python command");
            Serial.flush();
        }
    }
}

// Python command mode - interpret all input as Python commands
void pythonCommandMode() {
    Serial.println("\n=== Python Command Mode ===");
    Serial.println("All input will be interpreted as Python commands");
    Serial.println("Type 'exit' or 'quit' to return to main menu");
    Serial.println("Type 'help' for Python command help\n");
    
    while (true) {
        Serial.print("py> ");
        Serial.flush();
        
        String command = "";
        char c;
        
        // Read until newline
        while (true) {
            if (Serial.available() > 0) {
                c = Serial.read();
                if (c == '\n' || c == '\r') {
                    if (command.length() > 0) {
                        break;
                    }
                    // Ignore empty lines, show prompt again
                    Serial.print("py> ");
                    Serial.flush();
                    continue;
                } else if (c == '\b' || c == 0x7F) {
                    // Backspace
                    if (command.length() > 0) {
                        command.remove(command.length() - 1);
                        Serial.print("\b \b");
                    }
                } else if (c >= 32 && c <= 126) {
                    // Printable character
                    command += c;
                    Serial.print(c);
                }
            }
        }
        
        Serial.println(); // Echo newline
        
        // Check for exit commands
        if (command.equals("exit") || command.equals("quit")) {
            Serial.println("Exiting Python command mode\n");
            break;
        }
        
        // Check for help
        if (command.equals("help")) {
            Serial.println("Python Command Help:");
            Serial.println("  dac(set, channel, voltage)     - Set DAC output");
            Serial.println("  adc(get, channel)              - Read ADC input");
            Serial.println("  gpio(set, pin, HIGH/LOW)       - Set GPIO pin");
            Serial.println("  gpio(get, pin)                 - Read GPIO pin");
            Serial.println("  nodes(connect, node1, node2)   - Connect nodes");
            Serial.println("  nodes(remove, node1, node2)    - Disconnect nodes");
            Serial.println("  oled(print, \"text\", size)      - Display on OLED");
            Serial.println("  arduino(reset)                 - Reset Arduino");
            Serial.println("  uart(connect/disconnect)       - UART control");
            Serial.println("\nExamples:");
            Serial.println("  dac(set, 0, 2.5)");
            Serial.println("  gpio(set, 5, HIGH)");
            Serial.println("  nodes(connect, 1, 5, save=false)");
            Serial.println();
            continue;
        }
        
        if (command.length() > 0) {
            char response[1024];
            if (processCommand(command.c_str(), response)) {
                Serial.println(response);
            } else {
                Serial.print("ERROR: ");
                Serial.println(response);
            }
        }
    }
}

// Simple direct hardware test function to bypass complex parser
extern "C" int test_direct_hardware_sync(const char* command) {
    Serial.print("[DIRECT_TEST] Testing: ");
    Serial.println(command);
    Serial.flush();
    
    // Simple hardcoded responses for testing
    if (strcmp(command, "gpio(get, 1)") == 0) {
        // Test GPIO read - return a fake boolean result
        strcpy(sync_value_result, "False");
        strcpy(sync_type_result, "bool");
        Serial.println("[DIRECT_TEST] Returning fake GPIO False");
        return 2; // 2 = boolean false
    } 
    else if (strcmp(command, "oled(connect)") == 0) {
        // Test OLED connect - return success
        strcpy(sync_value_result, "OLED connected");
        strcpy(sync_type_result, "str");
        Serial.println("[DIRECT_TEST] Returning fake OLED success");
        return 0; // 0 = success
    }
    else {
        // Unknown command
        strcpy(sync_value_result, "Test command not supported");
        strcpy(sync_type_result, "error");
        Serial.println("[DIRECT_TEST] Unknown test command");
        return -1;
    }
}

// Direct hardware execution function - calls actual hardware without complex parser
extern "C" int execute_hardware_direct(const char* command) {
    Serial.print("[HARDWARE] Executing: ");
    Serial.println(command);
    Serial.flush();
    
    // Parse command into parts
    if (strncmp(command, "gpio(get,", 9) == 0) {
        // Extract pin number: gpio(get, 1) -> pin = 1
        int pin = atoi(command + 9);
        if (pin >= 1 && pin <= 10) {
            int value = digitalRead(gpioDef[pin][0]);
            strcpy(sync_value_result, value ? "True" : "False");
            strcpy(sync_type_result, "bool");
            Serial.print("[HARDWARE] GPIO pin ");
            Serial.print(pin);
            Serial.print(" read: ");
            Serial.println(value ? "HIGH" : "LOW");
            return value ? 1 : 2; // 1=True, 2=False
        } else {
            strcpy(sync_value_result, "Invalid GPIO pin");
            strcpy(sync_type_result, "error");
            return -1;
        }
    }
    else if (strncmp(command, "gpio(set,", 9) == 0) {
        // Extract pin and value: gpio(set, 1, HIGH) -> pin=1, value=HIGH
        const char* comma1 = strchr(command + 9, ',');
        if (comma1) {
            int pin = atoi(command + 9);
            const char* value_str = comma1 + 1;
            while (*value_str == ' ') value_str++; // skip spaces
            
            int value = 0;
            if (strncmp(value_str, "HIGH", 4) == 0 || strncmp(value_str, "1", 1) == 0) {
                value = 1;
            }
            
            if (pin >= 1 && pin <= 10) {
                digitalWrite(gpioDef[pin][0], value);
                strcpy(sync_value_result, "GPIO set");
                strcpy(sync_type_result, "str");
                Serial.print("[HARDWARE] GPIO pin ");
                Serial.print(pin);
                Serial.print(" set to ");
                Serial.println(value ? "HIGH" : "LOW");
                return 0;
            }
        }
        strcpy(sync_value_result, "Invalid GPIO command");
        strcpy(sync_type_result, "error");
        return -1;
    }
    else if (strncmp(command, "adc(get,", 8) == 0) {
        // Extract channel: adc(get, 0) -> channel = 0
        int channel = atoi(command + 8);
        float voltage = readAdcVoltage(channel, 32);
        
        // Convert float to string
        snprintf(sync_value_result, sizeof(sync_value_result), "%.3f", voltage);
        strcpy(sync_type_result, "float");
        Serial.print("[HARDWARE] ADC channel ");
        Serial.print(channel);
        Serial.print(" voltage: ");
        Serial.println(voltage);
        return 0;
    }
    else if (strncmp(command, "ina(getCurrent,", 15) == 0) {
        // Extract sensor: ina(getCurrent, 0) -> sensor = 0
        int sensor = atoi(command + 15);
        if (sensor == 0 || sensor == 1) {
            float current = (sensor == 0) ? INA0.getCurrent_mA() : INA1.getCurrent_mA();
            snprintf(sync_value_result, sizeof(sync_value_result), "%.3f", current);
            strcpy(sync_type_result, "float");
            Serial.print("[HARDWARE] INA");
            Serial.print(sensor);
            Serial.print(" current: ");
            Serial.println(current);
            return 0;
        }
    }
    else if (strncmp(command, "ina(getVoltage,", 15) == 0) {
        int sensor = atoi(command + 15);
        if (sensor == 0 || sensor == 1) {
            float voltage = (sensor == 0) ? INA0.getShuntVoltage_mV() : INA1.getShuntVoltage_mV();
            snprintf(sync_value_result, sizeof(sync_value_result), "%.3f", voltage);
            strcpy(sync_type_result, "float");
            return 0;
        }
    }
    else if (strncmp(command, "ina(getBusVoltage,", 18) == 0) {
        int sensor = atoi(command + 18);
        if (sensor == 0 || sensor == 1) {
            float voltage = (sensor == 0) ? INA0.getBusVoltage() : INA1.getBusVoltage();
            snprintf(sync_value_result, sizeof(sync_value_result), "%.3f", voltage);
            strcpy(sync_type_result, "float");
            return 0;
        }
    }
    else if (strncmp(command, "ina(getPower,", 13) == 0) {
        int sensor = atoi(command + 13);
        if (sensor == 0 || sensor == 1) {
            float power = (sensor == 0) ? INA0.getPower_mW() : INA1.getPower_mW();
            snprintf(sync_value_result, sizeof(sync_value_result), "%.3f", power);
            strcpy(sync_type_result, "float");
            return 0;
        }
    }
    else if (strncmp(command, "oled(connect)", 13) == 0) {
        int result = oled.init();
        strcpy(sync_value_result, "OLED connected");
        strcpy(sync_type_result, "str");
        Serial.println("[HARDWARE] OLED connected");
        return 0;
    }
    else if (strncmp(command, "oled(disconnect)", 16) == 0) {
        oled.disconnect();
        strcpy(sync_value_result, "OLED disconnected");
        strcpy(sync_type_result, "str");
        return 0;
    }
    else if (strncmp(command, "oled(clear)", 11) == 0) {
        oled.clear();
        strcpy(sync_value_result, "OLED cleared");
        strcpy(sync_type_result, "str");
        return 0;
    }
    else if (strncmp(command, "oled(show)", 10) == 0) {
        oled.show();
        strcpy(sync_value_result, "OLED updated");
        strcpy(sync_type_result, "str");
        return 0;
    }
    else if (strncmp(command, "oled(print,", 11) == 0) {
        // Extract text: oled(print, "Hello", 2) -> text="Hello", size=2
        const char* text_start = strchr(command + 11, '"');
        if (text_start) {
            text_start++; // skip opening quote
            const char* text_end = strchr(text_start, '"');
            if (text_end) {
                int text_len = text_end - text_start;
                char text[128];
                strncpy(text, text_start, text_len);
                text[text_len] = '\0';
                
                // Extract size (default to 2)
                int size = 2;
                const char* size_start = text_end + 1;
                while (*size_start && (*size_start == '"' || *size_start == ',' || *size_start == ' ')) size_start++;
                if (*size_start >= '1' && *size_start <= '9') {
                    size = atoi(size_start);
                }
                
                oled.clearPrintShow(text, size, true, true, true);
                strcpy(sync_value_result, "Text displayed");
                strcpy(sync_type_result, "str");
                Serial.print("[HARDWARE] OLED printed: ");
                Serial.println(text);
                return 0;
            }
        }
    }
    else if (strncmp(command, "dac(set,", 8) == 0) {
        // Extract channel and voltage: dac(set, 0, 2.5) -> channel=0, voltage=2.5
        const char* comma1 = strchr(command + 8, ',');
        if (comma1) {
            int channel = atoi(command + 8);
            float voltage = atof(comma1 + 1);
            setDacByNumber(channel, voltage, 0); // save=false for now
            strcpy(sync_value_result, "DAC set");
            strcpy(sync_type_result, "str");
            Serial.print("[HARDWARE] DAC channel ");
            Serial.print(channel);
            Serial.print(" set to ");
            Serial.println(voltage);
            return 0;
        }
    }
    else if (strncmp(command, "arduino(reset)", 14) == 0) {
        resetArduino();
        strcpy(sync_value_result, "Arduino reset");
        strcpy(sync_type_result, "str");
        return 0;
    }
    else if (strncmp(command, "uart(connect)", 13) == 0) {
        connectArduino(0);
        strcpy(sync_value_result, "UART connected");
        strcpy(sync_type_result, "str");
        return 0;
    }
    else if (strncmp(command, "uart(disconnect)", 16) == 0) {
        disconnectArduino(0);
        strcpy(sync_value_result, "UART disconnected");
        strcpy(sync_type_result, "str");
        return 0;
    }





            // Unknown command
    strcpy(sync_value_result, "Unknown command");
    strcpy(sync_type_result, "error");
    Serial.print("[HARDWARE] Unknown command: ");
    Serial.println(command);
    return -1;
}
