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

// C function that can be called from MicroPython to execute Jumperless commands
// This will be made available through a Python wrapper function
void jumperless_execute_command_c(const char* command) {
    strncpy(mp_command_buffer, command, sizeof(mp_command_buffer) - 1);
    mp_command_buffer[sizeof(mp_command_buffer) - 1] = '\0';
    mp_command_pending = true;
}



// Override MicroPython output to redirect to Arduino Serial
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    // Check if this is a command execution request
    if (len > 5 && strncmp(str, "EXEC:", 5) == 0) {
        // Extract and execute the command
        char command[1024];
        strncpy(command, str + 5, len - 5);
        command[len - 5] = '\0';
        
        // Remove trailing newline if present
        char* newline = strchr(command, '\n');
        if (newline) *newline = '\0';
        newline = strchr(command, '\r');
        if (newline) *newline = '\0';
        
        // Execute the command immediately
        char response[1024];
        int result = parseAndExecutePythonCommand(command, response);
        Serial.print("RESULT: ");
        Serial.println(response);
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
        Serial.print("MP_RESULT: ");
        Serial.println(mp_response_buffer);
        mp_command_pending = false;
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
    {"config", action_config},
    {"arduino", action_arduino},
    {"uart", action_uart},
    {"probe", action_probe},
    {"oled", action_oled},
    {"display", action_display},
    {"slot", action_slot},
    {"breadboard", action_breadboard},
    
    // Legacy single-function mappings for backward compatibility
    {"setDac", action_dac},
    {"getDac", action_dac},
    {"getAdc", action_adc},
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
                strcmp(functionMap[i].functionName, "config") == 0 ||
                strcmp(functionMap[i].functionName, "arduino") == 0 ||
                strcmp(functionMap[i].functionName, "uart") == 0 ||
                strcmp(functionMap[i].functionName, "probe") == 0 ||
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
                        bool save = false;
                        
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
                        bool save = false;
                        
                        // Check for save parameter
                        for (int i = 2; i < parsedCmd.argCount; i++) {
                            if (strcmp(parsedCmd.args[i].name, "save") == 0 && 
                                parsedCmd.args[i].type == ARG_TYPE_BOOL) {
                                save = parsedCmd.args[i].value.boolValue;
                                break;
                            }
                        }
                        
                        // Call actual Jumperless function
                        if (save) { // TODO: for now, always save
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
                if (result == 1) {
                    snprintf(response, 1024, "SUCCESS: oled(connect) - OLED connected");
                } else {
                    snprintf(response, 1024, "ERROR: oled(connect) - failed to connect OLED");
                }
                return result == 1 ? 0 : -1;
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

#define MICROPYTHON_HEAP_SIZE 16 * 1024

bool microPythonInitialized = false;
bool initMicroPython(void) {
  if (microPythonInitialized) return true;
  char heap[MICROPYTHON_HEAP_SIZE];
  int stack_top;
  mp_embed_init(&heap[0], sizeof(heap), &stack_top);
  microPythonInitialized = true;
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
            Serial.println("  run or %% - Execute the code in buffer");
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

          if (currentLine == "run" || currentLine == "%%") {
            if (inputBuffer.length() > 0) {
              changeTerminalColor(replColors[7], true);
              Serial.println("\n\r Executing:\n\r");
              changeTerminalColor(replColors[3], true);
              Serial.println(inputBuffer);
              changeTerminalColor(replColors[6], true);
              Serial.println("\n\r--- Output ---\n\r");
              changeTerminalColor(replColors[2], true);
              mp_embed_exec_str(inputBuffer.c_str());
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
              inputBuffer += "\n\r";
            }
            changeTerminalColor(replColors[1], true);
            Serial.print(">>> ");
            Serial.flush();
            currentLine = "";
            continue;
          }

          // Add current line to buffer
          if (inputBuffer.length() > 0) {
            if (currentLine.startsWith(">>> ")) {
              currentLine.remove(0, 4);
            }

            if (currentLine.startsWith("... ")) {
              currentLine.remove(0, 4);
              
            }

            if (currentLine.startsWith("run")) {
              currentLine.remove(0, 3);
              //inputBuffer += "\n\r";
            }
inputBuffer += "\n\r";
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
        case action_config: return "config";
        case action_arduino: return "arduino";
        case action_uart: return "uart";
        case action_probe: return "probe";
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
        case action_config: return parseConfigSubAction(subActionStr);
        case action_arduino: return parseArduinoSubAction(subActionStr);
        case action_uart: return parseUartSubAction(subActionStr);
        case action_probe: return parseProbeSubAction(subActionStr);
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

void setupJumperlessModule(void) {
    if (!microPythonInitialized) {
        Serial.println("Setting up MicroPython for Jumperless...");
        if (!initMicroPython()) {
            Serial.println("ERROR: Failed to initialize MicroPython");
            return;
        }
    }
    
    // Make the jumperless_execute_command function available to MicroPython
    // This allows Python code to call: jumperless_execute_command("dac(set, 0, 2.5)")
    executeMicroPython("import sys");
    executeMicroPython("class JumperlessNative:");
    executeMicroPython("    @staticmethod");
    executeMicroPython("    def execute(cmd): return 'Function registered'");
    
    Serial.println("Jumperless MicroPython module ready!");
    Serial.println("Import with: import JythonModule as jl");
    Serial.println("Examples:");
    Serial.println("  jl.dac.set(0, 2.5)");
    Serial.println("  jl.nodes.connect(1, 5)");
    Serial.println("  jl.oled.print('Hello!')");
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
