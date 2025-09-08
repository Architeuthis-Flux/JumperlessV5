#include <FatFS.h>
#include "Graphics.h"
#include "MatrixState.h"
#include "config.h"
#include "PersistentStuff.h"
#include "LEDs.h"
#include "Commands.h"
#include "FileParsing.h"
#include "configManager.h"
#include "NetManager.h"
#include "Peripherals.h"
#include "FilesystemStuff.h"
#include "oled.h"
#include "ArduinoStuff.h"
#include "Apps.h"

#ifdef DONOTUSE_SERIALWRAPPER
    #include "SerialWrapper.h"
    #define Serial SerialWrap
#endif


// Define the global configuration instance

bool configChanged = false;
bool autoCalibrationNeeded = false;

struct config jumperlessConfig;

int showNames = 1;
int lastShowNames = 1;

// Helper function to convert string to lowercase
void toLower(char* str) {
    for(int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

// Helper function to trim whitespace
void trim(char* str) {
    char* end;
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0) return;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    // Write new null terminator
    *(end+1) = 0;
}

// Parse comma-separated integers into an array
void parseCommaSeparatedInts(const char* str, int* array, int maxValues) {
    char buffer[32];
    strncpy(buffer, str, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    
    char* token = strtok(buffer, ",");
    int i = 0;
    while(token != NULL && i < maxValues) {
        trim(token);
        array[i++] = atoi(token);
        token = strtok(NULL, ",");
    }
}

// Parse comma-separated floats into an array
void parseCommaSeparatedFloats(const char* str, float* array, int maxValues) {
    char buffer[256];
    strncpy(buffer, str, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    
    char* token = strtok(buffer, ",");
    int i = 0;
    while(token != NULL && i < maxValues) {
        trim(token);
        array[i++] = atof(token);
        token = strtok(NULL, ",");
    }
}

// Parse comma-separated booleans into an array
void parseCommaSeparatedBools(const char* str, bool* array, int maxValues) {
    char buffer[256];
    strncpy(buffer, str, sizeof(buffer)-1);
    buffer[sizeof(buffer)-1] = '\0';
    
    char* token = strtok(buffer, ",");
    int i = 0;
    while(token != NULL && i < maxValues) {
        trim(token);
        array[i++] = parseBool(token);
        token = strtok(NULL, ",");
    }
}



// Helper for all parse functions
static int parseFromTable(const StringIntEntry* table, int tableSize, const char* str, int fallbackIsAtoi = 1, int fallbackValue = -1) {
    char lower[32];
    strncpy(lower, str, sizeof(lower)-1);
    lower[sizeof(lower)-1] = '\0';
    toLower(lower);
    for (int i = 0; i < tableSize; ++i) {
        if (strcmp(lower, table[i].name) == 0) {
            return table[i].value;
        }
    }
    if (fallbackIsAtoi)
        return atoi(str);
    else
        return fallbackValue;
}

static int printFromTable(const StringIntEntry* table, int tableSize, const char* str) {
    char lower[32];
    strncpy(lower, str, sizeof(lower)-1);
    lower[sizeof(lower)-1] = '\0';
    toLower(lower);
    for (int i = 0; i < tableSize; ++i) {
        if (strcmp(lower, table[i].name) == 0) {
            return Serial.print(table[i].name);
        }
    }
    return -1;
}

int parseHex(const char* str) {
    if (str[0] == '0' && str[1] == 'x') {
        return strtol(str, NULL, 16);
    }
    return atoi(str);
}

bool parseBool(const char* str) {
    int result = parseFromTable(boolTable, boolTableSize, str, 1, 0);
    return result;
}

int parseUartFunction(const char* str) {
    return parseFromTable(uartFunctionTable, uartFunctionTableSize, str);
}

int parseLinesWires(const char* str) {
    return parseFromTable(linesWiresTable, linesWiresTableSize, str);
}

int parseNetColorMode(const char* str) {
    return parseFromTable(netColorModeTable, netColorModeTableSize, str);
}

int parseArbitraryFunction(const char* str) {
    return parseFromTable(arbitraryFunctionTable, arbitraryFunctionTableSize, str);
}

int parseFont(const char* str) {
    return parseFromTable(fontTable, fontTableSize, str);
}

int parseSerialPort(const char* str) {
    return parseFromTable(serialPortTable, serialPortTableSize, str);
}

int parseDumpFormat(const char* str) {
    return parseFromTable(dumpFormatTable, dumpFormatTableSize, str);
}



void printArbitraryFunctionTable(void) {
    for (int i = 0; i < arbitraryFunctionTableSize; i++) {
        Serial.print(arbitraryFunctionTable[i].name);
        Serial.print(" = ");
        Serial.println(arbitraryFunctionTable[i].value);
    }
}

int printArbitraryFunction(int function) {
    for (int i = 0; i < arbitraryFunctionTableSize; i++) {
        if (arbitraryFunctionTable[i].value == function) {
            return Serial.print(arbitraryFunctionTable[i].name);
        }
    }
    return -1;
}

float parseFloat(const char* str) {
    return atof(str);
}

int parseInt(const char* str) {
    return atoi(str);
}

void resetConfigToDefaults(int clearCalibration, int clearHardware) {
    // Save current hardware version values
    int saved_generation = jumperlessConfig.hardware.generation;
    int saved_revision = jumperlessConfig.hardware.revision;
    int saved_probe_revision = jumperlessConfig.hardware.probe_revision;

    //save calibration values
    int saved_top_rail_zero = jumperlessConfig.calibration.top_rail_zero;
    int saved_bottom_rail_zero = jumperlessConfig.calibration.bottom_rail_zero;
    int saved_dac_0_zero = jumperlessConfig.calibration.dac_0_zero;
    int saved_dac_1_zero = jumperlessConfig.calibration.dac_1_zero;
    float saved_top_rail_spread = jumperlessConfig.calibration.top_rail_spread;
    float saved_bottom_rail_spread = jumperlessConfig.calibration.bottom_rail_spread;
    float saved_dac_0_spread = jumperlessConfig.calibration.dac_0_spread;
    float saved_dac_1_spread = jumperlessConfig.calibration.dac_1_spread;
    float saved_adc_0_zero = jumperlessConfig.calibration.adc_0_zero;
    float saved_adc_0_spread = jumperlessConfig.calibration.adc_0_spread;
    float saved_adc_1_zero = jumperlessConfig.calibration.adc_1_zero;
    float saved_adc_1_spread = jumperlessConfig.calibration.adc_1_spread;
    float saved_adc_2_zero = jumperlessConfig.calibration.adc_2_zero;
    float saved_adc_2_spread = jumperlessConfig.calibration.adc_2_spread;
    float saved_adc_3_zero = jumperlessConfig.calibration.adc_3_zero;
    float saved_adc_3_spread = jumperlessConfig.calibration.adc_3_spread;
    float saved_adc_4_zero = jumperlessConfig.calibration.adc_4_zero;
    float saved_adc_4_spread = jumperlessConfig.calibration.adc_4_spread;
    float saved_adc_7_zero = jumperlessConfig.calibration.adc_7_zero;
    float saved_adc_7_spread = jumperlessConfig.calibration.adc_7_spread;
    int saved_probe_max = jumperlessConfig.calibration.probe_max;
    int saved_probe_min = jumperlessConfig.calibration.probe_min;
    float saved_probe_switch_threshold = jumperlessConfig.calibration.probe_switch_threshold;
    float saved_measure_mode_output_voltage = jumperlessConfig.calibration.measure_mode_output_voltage;
    float saved_probe_current_zero = jumperlessConfig.calibration.probe_current_zero;
    // Serial.print("saved_probe_min = ");
    // Serial.println(saved_probe_min);
    // Serial.print("saved_probe_max = ");
    // Serial.println(saved_probe_max);
    
    
    // Initialize with default values from config.h
    jumperlessConfig = config();
    
    // Restore hardware version values
    if (clearHardware == 0) {
    jumperlessConfig.hardware.generation = saved_generation;
    jumperlessConfig.hardware.revision = saved_revision;
    jumperlessConfig.hardware.probe_revision = saved_probe_revision;
    }
    // Restore calibration values

        if (saved_probe_min == 0 || saved_probe_max == 0) {
        jumperlessConfig.calibration.probe_min = 15;
        jumperlessConfig.calibration.probe_max = 4060;
    } 


    if (clearCalibration == 0) {
        


    jumperlessConfig.calibration.top_rail_zero = saved_top_rail_zero;
    jumperlessConfig.calibration.bottom_rail_zero = saved_bottom_rail_zero;
    jumperlessConfig.calibration.dac_0_zero = saved_dac_0_zero;
    jumperlessConfig.calibration.dac_1_zero = saved_dac_1_zero;
    jumperlessConfig.calibration.probe_max = saved_probe_max;
    jumperlessConfig.calibration.probe_min = saved_probe_min;
    jumperlessConfig.calibration.top_rail_spread = saved_top_rail_spread;
    jumperlessConfig.calibration.bottom_rail_spread = saved_bottom_rail_spread;
    jumperlessConfig.calibration.dac_0_spread = saved_dac_0_spread;
    jumperlessConfig.calibration.dac_1_spread = saved_dac_1_spread;
    jumperlessConfig.calibration.adc_0_zero = saved_adc_0_zero;
    jumperlessConfig.calibration.adc_0_spread = saved_adc_0_spread;
    jumperlessConfig.calibration.adc_1_zero = saved_adc_1_zero;
    jumperlessConfig.calibration.adc_1_spread = saved_adc_1_spread;
    jumperlessConfig.calibration.adc_2_zero = saved_adc_2_zero;
    jumperlessConfig.calibration.adc_2_spread = saved_adc_2_spread;
    jumperlessConfig.calibration.adc_3_zero = saved_adc_3_zero;
    jumperlessConfig.calibration.adc_3_spread = saved_adc_3_spread;
    jumperlessConfig.calibration.adc_4_zero = saved_adc_4_zero;
    jumperlessConfig.calibration.adc_4_spread = saved_adc_4_spread;
    jumperlessConfig.calibration.adc_7_zero = saved_adc_7_zero;
    jumperlessConfig.calibration.adc_7_spread = saved_adc_7_spread;
    jumperlessConfig.calibration.probe_switch_threshold = saved_probe_switch_threshold;
    jumperlessConfig.calibration.measure_mode_output_voltage = saved_measure_mode_output_voltage;
    jumperlessConfig.calibration.probe_current_zero = saved_probe_current_zero;
    } 



    saveConfig();
}

void updateConfigFromFile(const char* filename) {

    if (!FatFS.exists(filename)) {
        firstStart = 1;
        resetConfigToDefaults();
        return;
    }

    File file = FatFS.open(filename, "r");
    if (!file) {
        Serial.println("Failed to open config file");
        return;
    }

    char line[128];
    char section[32] = "";
    char key[32];
    char value[64];
    // Config version tracking  
    const char* currentFirmwareVersion = firmwareVersion;

    
    bool foundConfigVersion = false;
    char configFirmwareVersion[16] = {0};
    bool needsReset = false;
    delay(200);//!son of a bitch
    while (file.available()) {
        int bytesRead = file.readBytesUntil('\n', line, sizeof(line)-1);
        line[bytesRead] = '\0';
        trim(line);
        
        if (line[0] == '\0' || line[0] == '#' || (line[0] == '/' && line[1] == '/')) continue;

        if (line[0] == '[' && line[strlen(line)-1] == ']') {
            strncpy(section, line+1, strlen(line)-2);
            section[strlen(line)-2] = '\0';
            toLower(section);
            continue;
        }

        char* equalsPos = strchr(line, '=');
        if (!equalsPos) continue;
        
        *equalsPos = '\0';
        strcpy(key, line);
        strcpy(value, equalsPos + 1);
        trim(key);
        trim(value);
        toLower(key);

        // Update config based on section and key
        if (strcmp(section, "config") == 0) {
            if (strcmp(key, "firmware_version") == 0) {
                // Strip trailing semicolon from version string first
                if (value[strlen(value)-1] == ';') {
                    value[strlen(value)-1] = '\0';
                }
                // Trim leading and trailing whitespace more robustly
                char* trimmed = value;
                while (isspace(*trimmed)) trimmed++;  // Skip leading spaces
                char* end = trimmed + strlen(trimmed) - 1;
                while (end > trimmed && isspace(*end)) end--;  // Find end of non-space chars
                *(end + 1) = '\0';  // Null terminate
                
                strncpy(configFirmwareVersion, trimmed, sizeof(configFirmwareVersion)-1);
                configFirmwareVersion[sizeof(configFirmwareVersion)-1] = '\0';  // Ensure null termination
                // Serial.print("configFirmwareVersion = ");
                // Serial.println(configFirmwareVersion);
                // Serial.print("firmwareVersion = ");
                // Serial.println(firmwareVersion);
                // Serial.print("strcmp(configFirmwareVersion, firmwareVersion) = ");
                //Serial.println(strcmp(configFirmwareVersion, firmwareVersion));
                foundConfigVersion = true;
            }
            //! this is a place to add new config options
        } else if (strcmp(section, "hardware") == 0) {
            if (strcmp(key, "generation") == 0) jumperlessConfig.hardware.generation = parseInt(value);
            else if (strcmp(key, "revision") == 0) jumperlessConfig.hardware.revision = parseInt(value);
            else if (strcmp(key, "probe_revision") == 0) jumperlessConfig.hardware.probe_revision = parseInt(value);
        } else if (strcmp(section, "dacs") == 0) {
            if (strcmp(key, "top_rail") == 0) jumperlessConfig.dacs.top_rail = parseFloat(value);
            else if (strcmp(key, "bottom_rail") == 0) jumperlessConfig.dacs.bottom_rail = parseFloat(value);
            else if (strcmp(key, "dac_0") == 0) jumperlessConfig.dacs.dac_0 = parseFloat(value);
            else if (strcmp(key, "dac_1") == 0) jumperlessConfig.dacs.dac_1 = parseFloat(value);
            else if (strcmp(key, "set_dacs_on_boot") == 0) jumperlessConfig.dacs.set_dacs_on_boot = parseBool(value);
            else if (strcmp(key, "set_rails_on_boot") == 0) jumperlessConfig.dacs.set_rails_on_boot = parseBool(value);
            else if (strcmp(key, "probe_power_dac") == 0) jumperlessConfig.dacs.probe_power_dac = parseInt(value);
            else if (strcmp(key, "limit_max") == 0) jumperlessConfig.dacs.limit_max = parseFloat(value);
            else if (strcmp(key, "limit_min") == 0) jumperlessConfig.dacs.limit_min = parseFloat(value);
        } else if (strcmp(section, "debug") == 0) {
            if (strcmp(key, "file_parsing") == 0) jumperlessConfig.debug.file_parsing = parseBool(value);
            else if (strcmp(key, "net_manager") == 0) jumperlessConfig.debug.net_manager = parseBool(value);
            else if (strcmp(key, "nets_to_chips") == 0) jumperlessConfig.debug.nets_to_chips = parseBool(value);
            else if (strcmp(key, "nets_to_chips_alt") == 0) jumperlessConfig.debug.nets_to_chips_alt = parseBool(value);
            else if (strcmp(key, "leds") == 0) jumperlessConfig.debug.leds = parseBool(value);
            else if (strcmp(key, "logic_analyzer") == 0) jumperlessConfig.debug.logic_analyzer = parseBool(value);
            else if (strcmp(key, "arduino") == 0) jumperlessConfig.debug.arduino = parseInt(value);
        } else if (strcmp(section, "routing") == 0) {
            if (strcmp(key, "stack_paths") == 0) {
                jumperlessConfig.routing.stack_paths = parseInt(value);
                // Serial.print("Updated stack_paths to: ");
                // Serial.println(jumperlessConfig.routing.stack_paths);
            }
            else if (strcmp(key, "stack_rails") == 0) jumperlessConfig.routing.stack_rails = parseInt(value);
            else if (strcmp(key, "stack_dacs") == 0) jumperlessConfig.routing.stack_dacs = parseInt(value);
            else if (strcmp(key, "rail_priority") == 0) jumperlessConfig.routing.rail_priority = parseInt(value);
        } else if (strcmp(section, "calibration") == 0) {
            if (strcmp(key, "top_rail_zero") == 0) jumperlessConfig.calibration.top_rail_zero = parseInt(value);
            else if (strcmp(key, "top_rail_spread") == 0) jumperlessConfig.calibration.top_rail_spread = parseFloat(value);
            else if (strcmp(key, "bottom_rail_zero") == 0) jumperlessConfig.calibration.bottom_rail_zero = parseInt(value);
            else if (strcmp(key, "bottom_rail_spread") == 0) jumperlessConfig.calibration.bottom_rail_spread = parseFloat(value);
            else if (strcmp(key, "dac_0_zero") == 0) jumperlessConfig.calibration.dac_0_zero = parseInt(value);
            else if (strcmp(key, "dac_0_spread") == 0) jumperlessConfig.calibration.dac_0_spread = parseFloat(value);
            else if (strcmp(key, "dac_1_zero") == 0) jumperlessConfig.calibration.dac_1_zero = parseInt(value);
            else if (strcmp(key, "dac_1_spread") == 0) jumperlessConfig.calibration.dac_1_spread = parseFloat(value);
            else if (strcmp(key, "adc_0_zero") == 0) jumperlessConfig.calibration.adc_0_zero = parseFloat(value);
            else if (strcmp(key, "adc_0_spread") == 0) jumperlessConfig.calibration.adc_0_spread = parseFloat(value);
            else if (strcmp(key, "adc_1_zero") == 0) jumperlessConfig.calibration.adc_1_zero = parseFloat(value);
            else if (strcmp(key, "adc_1_spread") == 0) jumperlessConfig.calibration.adc_1_spread = parseFloat(value);
            else if (strcmp(key, "adc_2_zero") == 0) jumperlessConfig.calibration.adc_2_zero = parseFloat(value);
            else if (strcmp(key, "adc_2_spread") == 0) jumperlessConfig.calibration.adc_2_spread = parseFloat(value);
            else if (strcmp(key, "adc_3_zero") == 0) jumperlessConfig.calibration.adc_3_zero = parseFloat(value);
            else if (strcmp(key, "adc_3_spread") == 0) jumperlessConfig.calibration.adc_3_spread = parseFloat(value);
            else if (strcmp(key, "adc_4_zero") == 0) jumperlessConfig.calibration.adc_4_zero = parseFloat(value);
            else if (strcmp(key, "adc_4_spread") == 0) jumperlessConfig.calibration.adc_4_spread = parseFloat(value);
            else if (strcmp(key, "adc_7_zero") == 0) jumperlessConfig.calibration.adc_7_zero = parseFloat(value);
            else if (strcmp(key, "adc_7_spread") == 0) jumperlessConfig.calibration.adc_7_spread = parseFloat(value);
            else if (strcmp(key, "probe_max") == 0) jumperlessConfig.calibration.probe_max = parseInt(value);
            else if (strcmp(key, "probe_min") == 0) jumperlessConfig.calibration.probe_min = parseInt(value);
            else if (strcmp(key, "probe_switch_threshold") == 0) jumperlessConfig.calibration.probe_switch_threshold = parseFloat(value);
            else if (strcmp(key, "measure_mode_output_voltage") == 0) jumperlessConfig.calibration.measure_mode_output_voltage = parseFloat(value);
            else if (strcmp(key, "probe_current_zero") == 0) jumperlessConfig.calibration.probe_current_zero = parseFloat(value);
        } else if (strcmp(section, "logo_pads") == 0) {
            if (strcmp(key, "top_guy") == 0) jumperlessConfig.logo_pads.top_guy = parseArbitraryFunction(value);
            else if (strcmp(key, "bottom_guy") == 0) jumperlessConfig.logo_pads.bottom_guy = parseArbitraryFunction(value);
            else if (strcmp(key, "building_pad_top") == 0) jumperlessConfig.logo_pads.building_pad_top = parseArbitraryFunction(value);
            else if (strcmp(key, "building_pad_bottom") == 0) jumperlessConfig.logo_pads.building_pad_bottom = parseArbitraryFunction(value);
        } else if (strcmp(section, "display") == 0) {
            if (strcmp(key, "lines_wires") == 0) jumperlessConfig.display.lines_wires = parseLinesWires(value);
            else if (strcmp(key, "menu_brightness") == 0) jumperlessConfig.display.menu_brightness = parseInt(value);
            else if (strcmp(key, "led_brightness") == 0) jumperlessConfig.display.led_brightness = parseInt(value);
            else if (strcmp(key, "rail_brightness") == 0) jumperlessConfig.display.rail_brightness = parseInt(value);
            else if (strcmp(key, "special_net_brightness") == 0) jumperlessConfig.display.special_net_brightness = parseInt(value);
            else if (strcmp(key, "net_color_mode") == 0) jumperlessConfig.display.net_color_mode = parseNetColorMode(value);
            else if (strcmp(key, "dump_leds") == 0) jumperlessConfig.display.dump_leds = parseSerialPort(value);
            else if (strcmp(key, "dump_format") == 0) jumperlessConfig.display.dump_format = parseDumpFormat(value);
        } else if (strcmp(section, "gpio") == 0) {
            if (strcmp(key, "direction") == 0) parseCommaSeparatedInts(value, jumperlessConfig.gpio.direction, 10);
            else if (strcmp(key, "pulls") == 0) parseCommaSeparatedInts(value, jumperlessConfig.gpio.pulls, 10);
            else if (strcmp(key, "pwm_frequency") == 0) parseCommaSeparatedFloats(value, jumperlessConfig.gpio.pwm_frequency, 10);
            else if (strcmp(key, "pwm_duty_cycle") == 0) parseCommaSeparatedFloats(value, jumperlessConfig.gpio.pwm_duty_cycle, 10);
            else if (strcmp(key, "pwm_enabled") == 0) parseCommaSeparatedBools(value, jumperlessConfig.gpio.pwm_enabled, 10);
            else if (strcmp(key, "uart_tx_function") == 0) jumperlessConfig.gpio.uart_tx_function = parseArbitraryFunction(value);
            else if (strcmp(key, "uart_rx_function") == 0) jumperlessConfig.gpio.uart_rx_function = parseArbitraryFunction(value);
        } else if (strcmp(section, "serial_1") == 0) {
            if (strcmp(key, "function") == 0) jumperlessConfig.serial_1.function = parseUartFunction(value);
            else if (strcmp(key, "baud_rate") == 0) jumperlessConfig.serial_1.baud_rate = parseInt(value);
            else if (strcmp(key, "print_passthrough") == 0) jumperlessConfig.serial_1.print_passthrough = parseBool(value);
            else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.serial_1.connect_on_boot = parseBool(value);
            else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.serial_1.lock_connection = parseBool(value);
            else if (strcmp(key, "autoconnect_flashing") == 0) jumperlessConfig.serial_1.autoconnect_flashing = parseBool(value);
            else if (strcmp(key, "async_passthrough") == 0) jumperlessConfig.serial_1.async_passthrough = parseBool(value);
        } else if (strcmp(section, "serial_2") == 0) {
            if (strcmp(key, "function") == 0) jumperlessConfig.serial_2.function = parseUartFunction(value);
            else if (strcmp(key, "baud_rate") == 0) jumperlessConfig.serial_2.baud_rate = parseInt(value);
            else if (strcmp(key, "print_passthrough") == 0) jumperlessConfig.serial_2.print_passthrough = parseBool(value);
            else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.serial_2.connect_on_boot = parseBool(value);
            else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.serial_2.lock_connection = parseBool(value);
            else if (strcmp(key, "autoconnect_flashing") == 0) jumperlessConfig.serial_2.autoconnect_flashing = parseBool(value);
        } else if (strcmp(section, "top_oled") == 0) {
            if (strcmp(key, "enabled") == 0) jumperlessConfig.top_oled.enabled = parseBool(value);
            else if (strcmp(key, "i2c_address") == 0) jumperlessConfig.top_oled.i2c_address = parseInt(value);
            else if (strcmp(key, "width") == 0) jumperlessConfig.top_oled.width = parseInt(value);
            else if (strcmp(key, "height") == 0) jumperlessConfig.top_oled.height = parseInt(value);
            else if (strcmp(key, "sda_pin") == 0) jumperlessConfig.top_oled.sda_pin = parseInt(value);
            else if (strcmp(key, "scl_pin") == 0) jumperlessConfig.top_oled.scl_pin = parseInt(value);
            else if (strcmp(key, "gpio_sda") == 0) jumperlessConfig.top_oled.gpio_sda = parseInt(value);
            else if (strcmp(key, "gpio_scl") == 0) jumperlessConfig.top_oled.gpio_scl = parseInt(value);
            else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.top_oled.connect_on_boot = parseBool(value);
            else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.top_oled.lock_connection = parseBool(value);
            else if (strcmp(key, "show_in_terminal") == 0) jumperlessConfig.top_oled.show_in_terminal = parseSerialPort(value);
            else if (strcmp(key, "font") == 0) jumperlessConfig.top_oled.font = parseFont(value);
        }
    }
    file.close();

    // Check if config needs to be reset due to version differences
    if (!foundConfigVersion) {
        // Old config without version tracking - reset to be safe
        Serial.println("Config file missing version info. Resetting to defaults (preserving hardware/calibration)...");
        needsReset = true;
    } else {
        // Parse version numbers to compare
        int configGen = 5, configMajor = 0, configMinor = 0, configPatch = 0;
        int currentGen = 5, currentMajor = 0, currentMinor = 0, currentPatch = 0;
        
        sscanf(configFirmwareVersion, "%d.%d.%d.%d", &configGen, &configMajor, &configMinor, &configPatch);
        sscanf(currentFirmwareVersion, "%d.%d.%d.%d", &currentGen, &currentMajor, &currentMinor, &currentPatch);
        
        // Check if current firmware is newer than config firmware
        bool isNewerFirmware = (currentMajor > configMajor) || 
                              (currentMajor == configMajor && currentMinor > configMinor) ||
                              (currentMajor == configMajor && currentMinor == configMinor && currentPatch > configPatch);
        
        if (isNewerFirmware && newConfigOptions) {
            Serial.print("Firmware updated from ");
            Serial.print(configFirmwareVersion);
            Serial.print(" to ");
            Serial.print(currentFirmwareVersion);
            Serial.println(" with new config options. Reloading config...");
            
            // Save ALL current config values before reset
            struct config savedConfig = jumperlessConfig;
            
            // Reset to defaults to get any new options
            resetConfigToDefaults(0, 0);  // Don't clear calibration or hardware
            
            // Check if there are new calibration options by comparing calibration sections
            bool hasNewCalibrationOptions = false;
            
            // Compare key calibration parameters to detect new options
            if (jumperlessConfig.calibration.top_rail_zero != savedConfig.calibration.top_rail_zero ||
                jumperlessConfig.calibration.top_rail_spread != savedConfig.calibration.top_rail_spread ||
                jumperlessConfig.calibration.bottom_rail_zero != savedConfig.calibration.bottom_rail_zero ||
                jumperlessConfig.calibration.bottom_rail_spread != savedConfig.calibration.bottom_rail_spread ||
                jumperlessConfig.calibration.dac_0_zero != savedConfig.calibration.dac_0_zero ||
                jumperlessConfig.calibration.dac_0_spread != savedConfig.calibration.dac_0_spread ||
                jumperlessConfig.calibration.dac_1_zero != savedConfig.calibration.dac_1_zero ||
                jumperlessConfig.calibration.dac_1_spread != savedConfig.calibration.dac_1_spread ||
                jumperlessConfig.calibration.adc_0_zero != savedConfig.calibration.adc_0_zero ||
                jumperlessConfig.calibration.adc_0_spread != savedConfig.calibration.adc_0_spread ||
                jumperlessConfig.calibration.adc_1_zero != savedConfig.calibration.adc_1_zero ||
                jumperlessConfig.calibration.adc_1_spread != savedConfig.calibration.adc_1_spread ||
                jumperlessConfig.calibration.adc_2_zero != savedConfig.calibration.adc_2_zero ||
                jumperlessConfig.calibration.adc_2_spread != savedConfig.calibration.adc_2_spread ||
                jumperlessConfig.calibration.adc_3_zero != savedConfig.calibration.adc_3_zero ||
                jumperlessConfig.calibration.adc_3_spread != savedConfig.calibration.adc_3_spread ||
                jumperlessConfig.calibration.adc_4_zero != savedConfig.calibration.adc_4_zero ||
                jumperlessConfig.calibration.adc_4_spread != savedConfig.calibration.adc_4_spread ||
                jumperlessConfig.calibration.adc_7_zero != savedConfig.calibration.adc_7_zero ||
                jumperlessConfig.calibration.adc_7_spread != savedConfig.calibration.adc_7_spread ||
                jumperlessConfig.calibration.probe_switch_threshold != savedConfig.calibration.probe_switch_threshold ||
                jumperlessConfig.calibration.measure_mode_output_voltage != savedConfig.calibration.measure_mode_output_voltage ||
                jumperlessConfig.calibration.probe_current_zero != savedConfig.calibration.probe_current_zero) {
                hasNewCalibrationOptions = true;
            }
            
            // Restore all saved values (this preserves user settings while adding any new defaults)
            jumperlessConfig.hardware = savedConfig.hardware;
            jumperlessConfig.dacs = savedConfig.dacs;
            jumperlessConfig.debug = savedConfig.debug;
            jumperlessConfig.routing = savedConfig.routing;
            jumperlessConfig.calibration = savedConfig.calibration;
            jumperlessConfig.logo_pads = savedConfig.logo_pads;
            jumperlessConfig.display = savedConfig.display;
            jumperlessConfig.gpio = savedConfig.gpio;
            jumperlessConfig.serial_1 = savedConfig.serial_1;
            jumperlessConfig.serial_2 = savedConfig.serial_2;
            jumperlessConfig.top_oled = savedConfig.top_oled;
            
            // Save the updated config with current firmware version
            saveConfig();
            Serial.println("Config updated with new firmware version.");
            
            // Set flag to run calibration later if there are new calibration options
            if (hasNewCalibrationOptions) {
                Serial.println("New calibration options detected. Calibration will run after initialization...");
                autoCalibrationNeeded = true;
            }
            
            // Reset the flag so this only happens once per firmware update
            newConfigOptions = false;
            return;
        }
        
        // Check if firmware is significantly older (for backward compatibility warnings)
        bool majorVersionDiff = (currentMajor > configMajor);
        bool minorVersionDiff = (currentMajor == configMajor && currentMinor > configMinor + 1);
        
        if (majorVersionDiff || minorVersionDiff) {
            Serial.print("Config from firmware ");
            Serial.print(configFirmwareVersion);
            Serial.print(" is significantly older than current firmware ");
            Serial.print(currentFirmwareVersion);
            Serial.println(". Resetting to defaults (preserving hardware/calibration)...");
            needsReset = true;
        }
    }
    
    if (needsReset) {
        // Save ALL current config values before reset
        struct config savedConfig = jumperlessConfig;
        
        // Reset to defaults to get any new options
        resetConfigToDefaults(1, 1);  // Clear calibration and hardware too, we'll restore them
        
        // Restore all saved values (this preserves user settings while adding any new defaults)
        jumperlessConfig.hardware = savedConfig.hardware;
        jumperlessConfig.dacs = savedConfig.dacs;
        jumperlessConfig.debug = savedConfig.debug;
        jumperlessConfig.routing = savedConfig.routing;
        jumperlessConfig.calibration = savedConfig.calibration;
        jumperlessConfig.logo_pads = savedConfig.logo_pads;
        jumperlessConfig.display = savedConfig.display;
        jumperlessConfig.gpio = savedConfig.gpio;
        jumperlessConfig.serial_1 = savedConfig.serial_1;
        jumperlessConfig.serial_2 = savedConfig.serial_2;
        jumperlessConfig.top_oled = savedConfig.top_oled;
        
        // Save the updated config with preserved user settings + any new defaults
        saveConfig();
        return;
    }
    
    readSettingsFromConfig();
    //initChipStatus();
}

void saveConfigToFile(const char* filename) {
    //core1busy = true;
    if (FatFS.exists(filename)) {
        FatFS.remove(filename);
    }
    
    File file = FatFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to create config file");
        return;
    }
 //! this is a place to add new config options
    // Write config metadata section
    file.println("[config]");
    file.print("firmware_version = "); file.print(firmwareVersion); file.println(";");
    file.println();

    // Write hardware version section
    file.println("[hardware]");
    file.print("generation = "); file.print(jumperlessConfig.hardware.generation); file.println(";");
    file.print("revision = "); file.print(jumperlessConfig.hardware.revision); file.println(";");
    file.print("probe_revision = "); file.print(jumperlessConfig.hardware.probe_revision); file.println(";");
    file.println();

    // Write DAC settings section
    file.println("[dacs]");
    file.print("top_rail = "); file.print(jumperlessConfig.dacs.top_rail); file.println(";");
    file.print("bottom_rail = "); file.print(jumperlessConfig.dacs.bottom_rail); file.println(";");
    file.print("dac_0 = "); file.print(jumperlessConfig.dacs.dac_0); file.println(";");
    file.print("dac_1 = "); file.print(jumperlessConfig.dacs.dac_1); file.println(";");
    file.print("set_dacs_on_boot = "); file.print(jumperlessConfig.dacs.set_dacs_on_boot ? 1:0); file.println(";");
    file.print("set_rails_on_boot = "); file.print(jumperlessConfig.dacs.set_rails_on_boot ? 1:0); file.println(";");
    file.print("probe_power_dac = "); file.print(jumperlessConfig.dacs.probe_power_dac == 0 ? 0 : 1); file.println(";");
    file.print("limit_max = "); file.print(jumperlessConfig.dacs.limit_max); file.println(";");
    file.print("limit_min = "); file.print(jumperlessConfig.dacs.limit_min); file.println(";");
    file.println();

    // Write debug flags section
    file.println("[debug]");
    file.print("file_parsing = "); file.print(jumperlessConfig.debug.file_parsing ? 1:0); file.println(";");
    file.print("net_manager = "); file.print(jumperlessConfig.debug.net_manager ? 1:0); file.println(";");
    file.print("nets_to_chips = "); file.print(jumperlessConfig.debug.nets_to_chips ? 1:0); file.println(";");
    file.print("nets_to_chips_alt = "); file.print(jumperlessConfig.debug.nets_to_chips_alt ? 1:0); file.println(";");
    file.print("leds = "); file.print(jumperlessConfig.debug.leds ? 1:0); file.println(";");
    file.print("logic_analyzer = "); file.print(jumperlessConfig.debug.logic_analyzer ? 1:0); file.println(";");
    file.print("arduino = "); file.print(jumperlessConfig.debug.arduino); file.println(";");
    file.println();

    // Write routing settings section
    file.println("[routing]");
    file.print("stack_paths = "); file.print(jumperlessConfig.routing.stack_paths); file.println(";");
    file.print("stack_rails = "); file.print(jumperlessConfig.routing.stack_rails); file.println(";");
    file.print("stack_dacs = "); file.print(jumperlessConfig.routing.stack_dacs); file.println(";");
    file.print("rail_priority = "); file.print(jumperlessConfig.routing.rail_priority); file.println(";");
    file.println();

    // Write calibration section
    file.println("[calibration]");
    file.print("top_rail_zero = "); file.print(jumperlessConfig.calibration.top_rail_zero); file.println(";");
    file.print("top_rail_spread = "); file.print(jumperlessConfig.calibration.top_rail_spread); file.println(";");
    file.print("bottom_rail_zero = "); file.print(jumperlessConfig.calibration.bottom_rail_zero); file.println(";");
    file.print("bottom_rail_spread = "); file.print(jumperlessConfig.calibration.bottom_rail_spread); file.println(";");
    file.print("dac_0_zero = "); file.print(jumperlessConfig.calibration.dac_0_zero); file.println(";");
    file.print("dac_0_spread = "); file.print(jumperlessConfig.calibration.dac_0_spread); file.println(";");
    file.print("dac_1_zero = "); file.print(jumperlessConfig.calibration.dac_1_zero); file.println(";");
    file.print("dac_1_spread = "); file.print(jumperlessConfig.calibration.dac_1_spread); file.println(";");
    file.print("adc_0_zero = "); file.print(jumperlessConfig.calibration.adc_0_zero); file.println(";");
    file.print("adc_0_spread = "); file.print(jumperlessConfig.calibration.adc_0_spread); file.println(";");
    file.print("adc_1_zero = "); file.print(jumperlessConfig.calibration.adc_1_zero); file.println(";");
    file.print("adc_1_spread = "); file.print(jumperlessConfig.calibration.adc_1_spread); file.println(";");
    file.print("adc_2_zero = "); file.print(jumperlessConfig.calibration.adc_2_zero); file.println(";");
    file.print("adc_2_spread = "); file.print(jumperlessConfig.calibration.adc_2_spread); file.println(";");
    file.print("adc_3_zero = "); file.print(jumperlessConfig.calibration.adc_3_zero); file.println(";");
    file.print("adc_3_spread = "); file.print(jumperlessConfig.calibration.adc_3_spread); file.println(";");
    file.print("adc_4_zero = "); file.print(jumperlessConfig.calibration.adc_4_zero); file.println(";");
    file.print("adc_4_spread = "); file.print(jumperlessConfig.calibration.adc_4_spread); file.println(";");
    file.print("adc_7_zero = "); file.print(jumperlessConfig.calibration.adc_7_zero); file.println(";");
    file.print("adc_7_spread = "); file.print(jumperlessConfig.calibration.adc_7_spread); file.println(";");
    file.print("probe_max = "); file.print(jumperlessConfig.calibration.probe_max); file.println(";");
    file.print("probe_min = "); file.print(jumperlessConfig.calibration.probe_min); file.println(";");
    file.print("probe_switch_threshold = "); file.print(jumperlessConfig.calibration.probe_switch_threshold); file.println(";");
    file.print("measure_mode_output_voltage = "); file.print(jumperlessConfig.calibration.measure_mode_output_voltage); file.println(";");
    file.print("probe_current_zero = "); file.print(jumperlessConfig.calibration.probe_current_zero); file.println(";");
    file.println();

    // Write logo pad settings section
    file.println("[logo_pads]");
    file.print("top_guy = "); file.print(jumperlessConfig.logo_pads.top_guy); file.println(";");
    file.print("bottom_guy = "); file.print(jumperlessConfig.logo_pads.bottom_guy); file.println(";");
    file.print("building_pad_top = "); file.print(jumperlessConfig.logo_pads.building_pad_top); file.println(";");
    file.print("building_pad_bottom = "); file.print(jumperlessConfig.logo_pads.building_pad_bottom); file.println(";");
    file.println();

    // Write display settings section
    file.println("[display]");
    file.print("lines_wires = "); file.print(jumperlessConfig.display.lines_wires); file.println(";");
    file.print("menu_brightness = "); file.print(jumperlessConfig.display.menu_brightness); file.println(";");
    file.print("led_brightness = "); file.print(jumperlessConfig.display.led_brightness); file.println(";");
    file.print("rail_brightness = "); file.print(jumperlessConfig.display.rail_brightness); file.println(";");
    file.print("special_net_brightness = "); file.print(jumperlessConfig.display.special_net_brightness); file.println(";");
    file.print("net_color_mode = "); file.print(jumperlessConfig.display.net_color_mode); file.println(";");
    file.print("dump_leds = "); file.print(jumperlessConfig.display.dump_leds); file.println(";");
    file.print("dump_format = "); file.print(jumperlessConfig.display.dump_format); file.println(";");
    file.println();

    // Write GPIO section
    file.println("[gpio]");
    file.print("direction = ");
    for (int i = 0; i < 10; i++) {
        if (i > 0) file.print(",");
        file.print(jumperlessConfig.gpio.direction[i]);
    }
    file.println(";");
    file.print("pulls = ");
    for (int i = 0; i < 10; i++) {
        if (i > 0) file.print(",");
        file.print(jumperlessConfig.gpio.pulls[i]);
    }
    file.println(";");

    file.print("uart_tx_function = "); file.print(jumperlessConfig.gpio.uart_tx_function); file.println(";");
    file.print("uart_rx_function = "); file.print(jumperlessConfig.gpio.uart_rx_function); file.println(";");
    file.println();

    // Write serial section
    file.println("[serial_1]");
    file.print("function = "); file.print(jumperlessConfig.serial_1.function); file.println(";");
    file.print("baud_rate = "); file.print(jumperlessConfig.serial_1.baud_rate); file.println(";");
    file.print("print_passthrough = "); file.print(jumperlessConfig.serial_1.print_passthrough); file.println(";");
    file.print("connect_on_boot = "); file.print(jumperlessConfig.serial_1.connect_on_boot); file.println(";");
    file.print("lock_connection = "); file.print(jumperlessConfig.serial_1.lock_connection); file.println(";");
    file.print("autoconnect_flashing = "); file.print(jumperlessConfig.serial_1.autoconnect_flashing); file.println(";");
    file.print("async_passthrough = "); file.print(jumperlessConfig.serial_1.async_passthrough ? 1:0); file.println(";");

    file.println("[serial_2]");
    file.print("function = "); file.print(jumperlessConfig.serial_2.function); file.println(";");
    file.print("baud_rate = "); file.print(jumperlessConfig.serial_2.baud_rate); file.println(";");
    file.print("print_passthrough = "); file.print(jumperlessConfig.serial_2.print_passthrough); file.println(";");
    file.print("connect_on_boot = "); file.print(jumperlessConfig.serial_2.connect_on_boot); file.println(";");
    file.print("lock_connection = "); file.print(jumperlessConfig.serial_2.lock_connection); file.println(";");
    file.print("autoconnect_flashing = "); file.print(jumperlessConfig.serial_2.autoconnect_flashing); file.println(";");

    // Write top_oled section
    file.println("[top_oled]");
    file.print("enabled = "); file.print(jumperlessConfig.top_oled.enabled); file.println(";");
    file.print("i2c_address = "); file.print(jumperlessConfig.top_oled.i2c_address); file.println(";");
    file.print("width = "); file.print(jumperlessConfig.top_oled.width); file.println(";");
    file.print("height = "); file.print(jumperlessConfig.top_oled.height); file.println(";");
    file.print("sda_pin = "); file.print(jumperlessConfig.top_oled.sda_pin); file.println(";");
    file.print("scl_pin = "); file.print(jumperlessConfig.top_oled.scl_pin); file.println(";");
    file.print("gpio_sda = "); file.print(jumperlessConfig.top_oled.gpio_sda); file.println(";");
    file.print("gpio_scl = "); file.print(jumperlessConfig.top_oled.gpio_scl); file.println(";");
    file.print("sda_row = "); file.print(jumperlessConfig.top_oled.sda_row); file.println(";");
    file.print("scl_row = "); file.print(jumperlessConfig.top_oled.scl_row); file.println(";");
    file.print("connect_on_boot = "); file.print(jumperlessConfig.top_oled.connect_on_boot ? 1:0); file.println(";");
    file.print("lock_connection = "); file.print(jumperlessConfig.top_oled.lock_connection ? 1:0); file.println(";");
    file.print("show_in_terminal = "); file.print(jumperlessConfig.top_oled.show_in_terminal ? 1:0); file.println(";");
    file.print("font = "); file.print(jumperlessConfig.top_oled.font); file.println(";");
    file.close();
    //core1busy = false;
}

void saveConfig(void) {
    int hwRevision = jumperlessConfig.hardware.revision;

    if (jumperlessConfig.calibration.probe_min == 0 || jumperlessConfig.calibration.probe_max == 0) {
        jumperlessConfig.calibration.probe_min = 15;
        jumperlessConfig.calibration.probe_max = 4060;
    }

   // printGPIOState();
  // printConfigSectionToSerial(7, true, false);
   saveConfigToFile("/config.txt");
   /// printGPIOState();
    readSettingsFromConfig();
  ///  printGPIOState();
    ///initChipStatus();
    //if (jumperlessConfig.hardware.hardware_revision != hwRevision ) {
        // leds.clear();
        // leds.end();
        // leds.begin();
       
        // showLEDsCore2 = -1;
   // }
}

void loadConfig(void) {
    updateConfigFromFile("/config.txt");

    if (jumperlessConfig.calibration.probe_min == 0 || jumperlessConfig.calibration.probe_max == 0) {
        jumperlessConfig.calibration.probe_min = 15;
        jumperlessConfig.calibration.probe_max = 4060;
    }
    
    readSettingsFromConfig();
    // Defer initChipStatus to reduce startup time - it can be done later
    // initChipStatus();
}

int parseSectionName(const char* sectionName) {
    if (strcmp(sectionName, "config") == 0) return -2; // Special case for config section
    else if (strcmp(sectionName, "hardware") == 0) return 0;
    else if (strcmp(sectionName, "dacs") == 0) return 1;
    else if (strcmp(sectionName, "debug") == 0) return 2;
    else if (strcmp(sectionName, "routing") == 0) return 3;
    else if (strcmp(sectionName, "calibration") == 0) return 4;
    else if (strcmp(sectionName, "logo_pads") == 0) return 5;
    else if (strcmp(sectionName, "display") == 0) return 6;
    else if (strcmp(sectionName, "gpio") == 0) return 7;
    else if (strcmp(sectionName, "serial_1") == 0) return 8;
    else if (strcmp(sectionName, "serial_2") == 0) return 9;
    else if (strcmp(sectionName, "top_oled") == 0) return 10;
    return -1;
}

void printConfigSectionToSerial(int section, bool showNames, bool pasteable) {
    // If section is -1, try to parse input
    if (showNames) {
        showNames = 1;
    }
    else {
        showNames = 0;
    }
 //! this is a place to add new config options
    if (pasteable == true) {
        Serial.println("\n\rcopy / edit / paste any of these lines \n\rinto the main menu to change a setting\n\r");
    }
    if (section == -1) {
        Serial.println("Jumperless Config:\n\r");
    }
    cycleTerminalColor(true, (highSaturationBrightColorsCount/8.0), true, &Serial, 0, 1);
    // Print config metadata section
    if (section == -1 || section == -2) {
        Serial.print("\n`[config] ");
        if (pasteable == false) Serial.println();
        Serial.print("firmware_version = "); Serial.print(firmwareVersion); Serial.println(";");
    }
    
    // Print hardware version section
    if (section == -1 || section == 0) {
        Serial.print("\n`[hardware] ");
        if (pasteable == false) Serial.println();
        Serial.print("generation = "); Serial.print(jumperlessConfig.hardware.generation); Serial.println(";");
        if (pasteable == true) Serial.print("`[hardware] ");
        Serial.print("revision = "); Serial.print(jumperlessConfig.hardware.revision); Serial.println(";");
        if (pasteable == true) Serial.print("`[hardware] ");
        Serial.print("probe_revision = "); Serial.print(jumperlessConfig.hardware.probe_revision); Serial.println(";");
    }
    cycleTerminalColor();
    // Print DAC settings section
    if (section == -1 || section == 1) {
        Serial.print("\n`[dacs] ");
        if (pasteable == false) Serial.println();
        Serial.print("top_rail = "); Serial.print(jumperlessConfig.dacs.top_rail); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("bottom_rail = "); Serial.print(jumperlessConfig.dacs.bottom_rail); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("dac_0 = "); Serial.print(jumperlessConfig.dacs.dac_0); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("dac_1 = "); Serial.print(jumperlessConfig.dacs.dac_1); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("set_dacs_on_boot = "); Serial.print(getStringFromTable(jumperlessConfig.dacs.set_dacs_on_boot, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("set_rails_on_boot = "); Serial.print(getStringFromTable(jumperlessConfig.dacs.set_rails_on_boot, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("probe_power_dac = "); Serial.print(jumperlessConfig.dacs.probe_power_dac); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("limit_max = "); Serial.print(jumperlessConfig.dacs.limit_max); Serial.println(";");
        if (pasteable == true) Serial.print("`[dacs] ");
        Serial.print("limit_min = "); Serial.print(jumperlessConfig.dacs.limit_min); Serial.println(";");
    }
    cycleTerminalColor();
    // Print debug flags section
    if (section == -1 || section == 2) {
        Serial.print("\n`[debug] ");
        if (pasteable == false) Serial.println();
        Serial.print("file_parsing = "); Serial.print(getStringFromTable(jumperlessConfig.debug.file_parsing, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("net_manager = "); Serial.print(getStringFromTable(jumperlessConfig.debug.net_manager, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("nets_to_chips = "); Serial.print(getStringFromTable(jumperlessConfig.debug.nets_to_chips, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("nets_to_chips_alt = "); Serial.print(getStringFromTable(jumperlessConfig.debug.nets_to_chips_alt, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("leds = "); Serial.print(getStringFromTable(jumperlessConfig.debug.leds, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("logic_analyzer = "); Serial.print(getStringFromTable(jumperlessConfig.debug.logic_analyzer, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[debug] ");
        Serial.print("arduino = "); Serial.print(jumperlessConfig.debug.arduino); Serial.println(";");
    }
    cycleTerminalColor();
    // Print routing settings section
    if (section == -1 || section == 3) {
        Serial.print("\n`[routing] ");
        if (pasteable == false) Serial.println();
        Serial.print("stack_paths = "); Serial.print(jumperlessConfig.routing.stack_paths); Serial.println(";");
        if (pasteable == true) Serial.print("`[routing] ");
        Serial.print("stack_rails = "); Serial.print(jumperlessConfig.routing.stack_rails); Serial.println(";");
        if (pasteable == true) Serial.print("`[routing] ");
        Serial.print("stack_dacs = "); Serial.print(jumperlessConfig.routing.stack_dacs); Serial.println(";");
        if (pasteable == true) Serial.print("`[routing] ");
        Serial.print("rail_priority = "); Serial.print(jumperlessConfig.routing.rail_priority); Serial.println(";");
    }
    cycleTerminalColor();
    // Print calibration section
    if (section == -1 || section == 4) {
        Serial.print("\n`[calibration] ");
        if (pasteable == false) Serial.println();
        Serial.print("top_rail_zero = "); Serial.print(jumperlessConfig.calibration.top_rail_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("top_rail_spread = "); Serial.print(jumperlessConfig.calibration.top_rail_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("bottom_rail_zero = "); Serial.print(jumperlessConfig.calibration.bottom_rail_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("bottom_rail_spread = "); Serial.print(jumperlessConfig.calibration.bottom_rail_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("dac_0_zero = "); Serial.print(jumperlessConfig.calibration.dac_0_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("dac_0_spread = "); Serial.print(jumperlessConfig.calibration.dac_0_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("dac_1_zero = "); Serial.print(jumperlessConfig.calibration.dac_1_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("dac_1_spread = "); Serial.print(jumperlessConfig.calibration.dac_1_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_0_zero = "); Serial.print(jumperlessConfig.calibration.adc_0_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_0_spread = "); Serial.print(jumperlessConfig.calibration.adc_0_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_1_zero = "); Serial.print(jumperlessConfig.calibration.adc_1_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_1_spread = "); Serial.print(jumperlessConfig.calibration.adc_1_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_2_zero = "); Serial.print(jumperlessConfig.calibration.adc_2_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_2_spread = "); Serial.print(jumperlessConfig.calibration.adc_2_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_3_zero = "); Serial.print(jumperlessConfig.calibration.adc_3_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_3_spread = "); Serial.print(jumperlessConfig.calibration.adc_3_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_4_zero = "); Serial.print(jumperlessConfig.calibration.adc_4_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_4_spread = "); Serial.print(jumperlessConfig.calibration.adc_4_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_7_zero = "); Serial.print(jumperlessConfig.calibration.adc_7_zero); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("adc_7_spread = "); Serial.print(jumperlessConfig.calibration.adc_7_spread); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("probe_max = "); Serial.print(jumperlessConfig.calibration.probe_max); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("probe_min = "); Serial.print(jumperlessConfig.calibration.probe_min); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("probe_switch_threshold = "); Serial.print(jumperlessConfig.calibration.probe_switch_threshold); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("measure_mode_output_voltage = "); Serial.print(jumperlessConfig.calibration.measure_mode_output_voltage); Serial.println(";");
        if (pasteable == true) Serial.print("`[calibration] ");
        Serial.print("probe_current_zero = "); Serial.print(jumperlessConfig.calibration.probe_current_zero); Serial.println(";");
    }
    cycleTerminalColor();
    // Print logo pad settings section
    if (section == -1 || section == 5) {
        Serial.print("\n`[logo_pads] ");
        if (pasteable == false) Serial.println();   
        Serial.print("top_guy = "); Serial.print(getStringFromTable(jumperlessConfig.logo_pads.top_guy, arbitraryFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[logo_pads] ");
        Serial.print("bottom_guy = "); Serial.print(getStringFromTable(jumperlessConfig.logo_pads.bottom_guy, arbitraryFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[logo_pads] ");
        Serial.print("building_pad_top = "); Serial.print(getStringFromTable(jumperlessConfig.logo_pads.building_pad_top, arbitraryFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[logo_pads] ");
        Serial.print("building_pad_bottom = "); Serial.print(getStringFromTable(jumperlessConfig.logo_pads.building_pad_bottom, arbitraryFunctionTable)); Serial.println(";");
    }
    cycleTerminalColor();
    // Print display settings section
    if (section == -1 || section == 6) {
        Serial.print("\n`[display] ");
        if (pasteable == false) Serial.println();
        Serial.print("lines_wires = "); Serial.print(getStringFromTable(jumperlessConfig.display.lines_wires, linesWiresTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("menu_brightness = "); Serial.print(jumperlessConfig.display.menu_brightness); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("led_brightness = "); Serial.print(jumperlessConfig.display.led_brightness); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("rail_brightness = "); Serial.print(jumperlessConfig.display.rail_brightness); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("special_net_brightness = "); Serial.print(jumperlessConfig.display.special_net_brightness); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("net_color_mode = "); Serial.print(getStringFromTable(jumperlessConfig.display.net_color_mode, netColorModeTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("dump_leds = "); Serial.print(getStringFromTable(jumperlessConfig.display.dump_leds, serialPortTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[display] ");
        Serial.print("dump_format = "); Serial.print(getStringFromTable(jumperlessConfig.display.dump_format, dumpFormatTable)); Serial.println(";");
    }
    cycleTerminalColor();
    // Print GPIO section
    if (section == -1 || section == 7) {
        Serial.print("\n`[gpio] ");
        if (pasteable == false) Serial.println();
        Serial.print("direction = ");
        for (int i = 0; i < 10; i++) {
            if (i > 0) Serial.print(",");
            Serial.print(jumperlessConfig.gpio.direction[i]);
        }
        Serial.println(";");
        if (pasteable == true) Serial.print("`[gpio] ");
        Serial.print("pulls = ");
        for (int i = 0; i < 10; i++) {
            if (i > 0) Serial.print(",");
            Serial.print(jumperlessConfig.gpio.pulls[i]);
        }
        Serial.println(";");
        if (pasteable == true) Serial.print("`[gpio] ");
        Serial.print("uart_tx_function = "); Serial.print(getStringFromTable(jumperlessConfig.gpio.uart_tx_function, arbitraryFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[gpio] ");
        Serial.print("uart_rx_function = "); Serial.print(getStringFromTable(jumperlessConfig.gpio.uart_rx_function, arbitraryFunctionTable)); Serial.println(";");
    }
    cycleTerminalColor();
    // Print serial_1 section
    if (section == -1 || section == 8) {
        Serial.print("\n`[serial_1] ");
        if (pasteable == false) Serial.println();
        Serial.print("function = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.function, uartFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("baud_rate = "); Serial.print(jumperlessConfig.serial_1.baud_rate); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("print_passthrough = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.print_passthrough, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("connect_on_boot = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.connect_on_boot, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("lock_connection = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.lock_connection, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("autoconnect_flashing = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.autoconnect_flashing, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_1] ");
        Serial.print("async_passthrough = "); Serial.print(getStringFromTable(jumperlessConfig.serial_1.async_passthrough, boolTable)); Serial.println(";");
    }
    cycleTerminalColor();
    // Print serial_2 section
    if (section == -1 || section == 9) {
        Serial.print("\n`[serial_2] ");
        if (pasteable == false) Serial.println();
        Serial.print("function = "); Serial.print(getStringFromTable(jumperlessConfig.serial_2.function, uartFunctionTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_2] ");
        Serial.print("baud_rate = "); Serial.print(jumperlessConfig.serial_2.baud_rate); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_2] ");
        Serial.print("print_passthrough = "); Serial.print(getStringFromTable(jumperlessConfig.serial_2.print_passthrough, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_2] ");
        Serial.print("connect_on_boot = "); Serial.print(getStringFromTable(jumperlessConfig.serial_2.connect_on_boot, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_2] ");
        Serial.print("lock_connection = "); Serial.print(getStringFromTable(jumperlessConfig.serial_2.lock_connection, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[serial_2] ");
        Serial.print("autoconnect_flashing = "); Serial.print(getStringFromTable(jumperlessConfig.serial_2.autoconnect_flashing, boolTable)); Serial.println(";");
    }
    cycleTerminalColor();
    // Print top_oled section
    if (section == -1 || section == 10) {
        Serial.print("\n`[top_oled] ");
        if (pasteable == false) Serial.println();
        Serial.print("enabled = "); Serial.print(getStringFromTable(jumperlessConfig.top_oled.enabled, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("i2c_address = 0x"); Serial.print(jumperlessConfig.top_oled.i2c_address, HEX); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("width = "); Serial.print(jumperlessConfig.top_oled.width); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("height = "); Serial.print(jumperlessConfig.top_oled.height); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("sda_pin = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.sda_pin, 0));
        else Serial.print(jumperlessConfig.top_oled.sda_pin);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("scl_pin = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.scl_pin, 0));
        else Serial.print(jumperlessConfig.top_oled.scl_pin);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("gpio_sda = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.gpio_sda, 0));
        else Serial.print(jumperlessConfig.top_oled.gpio_sda);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("gpio_scl = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.gpio_scl, 0));
        else Serial.print(jumperlessConfig.top_oled.gpio_scl);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("sda_row = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.sda_row, 0));
        else Serial.print(jumperlessConfig.top_oled.sda_row);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("scl_row = ");
        if (showNames) Serial.print(definesToChar(jumperlessConfig.top_oled.scl_row, 0));
        else Serial.print(jumperlessConfig.top_oled.scl_row);
        Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("connect_on_boot = "); Serial.print(getStringFromTable(jumperlessConfig.top_oled.connect_on_boot, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("lock_connection = "); Serial.print(getStringFromTable(jumperlessConfig.top_oled.lock_connection, boolTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("show_in_terminal = "); Serial.print(getStringFromTable(jumperlessConfig.top_oled.show_in_terminal, serialPortTable)); Serial.println(";");
        if (pasteable == true) Serial.print("`[top_oled] ");
        Serial.print("font = "); Serial.print(getStringFromTable(jumperlessConfig.top_oled.font, fontTable)); Serial.println(";");
    }
    cycleTerminalColor();
    if (section == -1) {
        Serial.println("\nEND\n\r");
    }
}

// Helper function to clean whitespace
void cleanWhitespace(const char* input, char* output) {
    int j = 0;
    for (int i = 0; input[i]; i++) {
        if (!isspace(input[i])) {
            output[j++] = tolower(input[i]);
        }
    }
    output[j] = '\0';
}

// Helper function to parse setting
bool parseSetting(const char* line, char* section, char* key, char* value) {
    // Serial.print("Parsing line: '");
    // Serial.print(line);
    // Serial.println("'");
    
    // Check if this is dot notation format (config.section.key = value)
    if (strncmp(line, "config.", 7) == 0) {
        const char* start = line + 7;  // Skip "config."
        const char* firstDot = strchr(start, '.');
        const char* equals = strchr(start, '=');
        
        if (firstDot && equals && firstDot < equals) {
            // Extract section
            strncpy(section, start, firstDot - start);
            section[firstDot - start] = '\0';
            
            // Extract key
            const char* keyStart = firstDot + 1;
            strncpy(key, keyStart, equals - keyStart);
            key[equals - keyStart] = '\0';
            trim(key);
            
            // Extract value
            const char* valueStart = equals + 1;
            while (isspace(*valueStart)) valueStart++; // Skip leading whitespace
            strcpy(value, valueStart);
            
            // Trim trailing whitespace and semicolon from value
            char* end = value + strlen(value) - 1;
            while (end > value && (isspace(*end) || *end == ';')) {
                *end = '\0';
                end--;
            }
            
            // Serial.print("Dot notation - Section: '");
            // Serial.print(section);
            // Serial.print("', Key: '");
            // Serial.print(key);
            // Serial.print("', Value: '");
            // Serial.print(value);
            // Serial.println("'");
            
            return true;
        }
    }
    
    // Original bracket notation format
    const char* sectionEnd = strchr(line, ']');
    if (!sectionEnd) {
        Serial.println("No ] found and not dot notation format");
        return false;
    }
    
    // Extract section (skip the [)
    strncpy(section, line + 1, sectionEnd - line - 1);
    section[sectionEnd - line - 1] = '\0';
    
    // Convert section to lowercase for comparison
    char sectionLower[32];
    strcpy(sectionLower, section);
    for(int i = 0; sectionLower[i]; i++) {
        sectionLower[i] = tolower(sectionLower[i]);
    }
    
    // Find the equals sign
    const char* equalsPos = strchr(sectionEnd, '=');
    if (!equalsPos) {
        Serial.println("No = found");
        return false;
    }
    
    // Extract key (skip the ])
    const char* keyStart = sectionEnd + 1;
    while (isspace(*keyStart)) keyStart++; // Skip leading whitespace
    
    // Find the end of the key (before the =)
    const char* keyEnd = equalsPos;
    while (keyEnd > keyStart && isspace(*(keyEnd-1))) keyEnd--; // Skip trailing whitespace
    
    strncpy(key, keyStart, keyEnd - keyStart);
    key[keyEnd - keyStart] = '\0';
    
    // Extract value (skip the =)
    const char* valueStart = equalsPos + 1;
    while (isspace(*valueStart)) valueStart++; // Skip leading whitespace
    strcpy(value, valueStart);
    
    // Trim trailing whitespace and semicolon from value
    char* end = value + strlen(value) - 1;
    while (end > value && (isspace(*end) || *end == ';')) {
        *end = '\0';
        end--;
    }
    
    return true;
}

// Helper function to print setting change
void printSettingChange(const char* section, const char* key, const char* oldValue, const char* newValue) {
    // Try to print names for enums/bools if possible
    const char* oldName = nullptr;
    const char* newName = nullptr;
    if (strcmp(section, "display") == 0 && strcmp(key, "lines_wires") == 0) {
        oldName = getStringFromTable(atoi(oldValue), linesWiresTable);
        newName = getStringFromTable(atoi(newValue), linesWiresTable);
    } else if (strcmp(section, "display") == 0 && strcmp(key, "net_color_mode") == 0) {
        oldName = getStringFromTable(atoi(oldValue), netColorModeTable);
        newName = getStringFromTable(atoi(newValue), netColorModeTable);
    } else if ((strcmp(section, "serial_1") == 0 || strcmp(section, "serial_2") == 0 || strcmp(section, "gpio") == 0) && (strstr(key, "function") != NULL)) {
        oldName = getStringFromTable(atoi(oldValue), uartFunctionTable);
        newName = getStringFromTable(atoi(newValue), uartFunctionTable);
        initArduino();
    } else if (strcmp(section, "logo_pads") == 0) {
        oldName = getStringFromTable(atoi(oldValue), arbitraryFunctionTable);
        newName = getStringFromTable(atoi(newValue), arbitraryFunctionTable);
    } else if (strcmp(section, "top_oled") == 0 && strcmp(key, "font") == 0) {
        oldName = getStringFromTable(atoi(oldValue), fontTable);
        newName = getStringFromTable(atoi(newValue), fontTable);
    } else if (strcmp(section, "top_oled") == 0 && strcmp(key, "show_in_terminal") == 0) {
        oldName = getStringFromTable(atoi(oldValue), serialPortTable);
        newName = getStringFromTable(atoi(newValue), serialPortTable);
        initArduino();
    } else if (strcmp(section, "display") == 0 && strcmp(key, "dump_leds") == 0) {
        oldName = getStringFromTable(atoi(oldValue), serialPortTable);
        newName = getStringFromTable(atoi(newValue), serialPortTable);
        initArduino();
    } else if (strcmp(section, "display") == 0 && strcmp(key, "dump_format") == 0) {
        oldName = getStringFromTable(atoi(oldValue), dumpFormatTable);
        newName = getStringFromTable(atoi(newValue), dumpFormatTable);
    } else if (strcmp(section, "debug") == 0 && strcmp(key, "logic_analyzer") == 0) {
        oldName = getStringFromTable(atoi(oldValue), boolTable);
        newName = getStringFromTable(atoi(newValue), boolTable);
    } else if (
        (strcmp(section, "dacs") == 0 && (strcmp(key, "set_dacs_on_startup") == 0 || strcmp(key, "set_rails_on_startup") == 0)) ||
        (strcmp(section, "debug") == 0) ||
        (strcmp(section, "serial_1") == 0 && (strcmp(key, "print_passthrough") == 0 || strcmp(key, "connect_on_boot") == 0 || strcmp(key, "lock_connection") == 0)) ||
        (strcmp(section, "serial_2") == 0 && (strcmp(key, "print_passthrough") == 0 || strcmp(key, "connect_on_boot") == 0 || strcmp(key, "lock_connection") == 0))
    ) {
        oldName = getStringFromTable(atoi(oldValue), boolTable);
        newName = getStringFromTable(atoi(newValue), boolTable);
    }
    Serial.print("Changed [");
    Serial.print(section);
    Serial.print("] ");
    Serial.print(key);
    Serial.print(" from ");
    if (oldName) Serial.print(oldName); else Serial.print(oldValue);
    Serial.print(" to ");
    Serial.println(newValue);
}

void printConfigHelp() {

    
    Serial.println("\n\r");
    cycleTerminalColor(true, 8.0, true,  &Serial, 12, 1);
    Serial.println("                              Read config ");
    cycleTerminalColor(false, 2.0, true,  &Serial, 0, 1);
    Serial.println("                          ~ = show current config");
    Serial.println("                     ~names = show names for settings");
    Serial.println("                   ~numbers = show numbers for settings");
    Serial.println("                 ~[section] = show specific section (e.g. ~[routing])");
    cycleTerminalColor(true, 15.0, true,  &Serial, 22, 1);
    Serial.println("\n\r");
    Serial.println("                              Write config ");
    cycleTerminalColor(false, 2.0, true,  &Serial, 0, 1);
    Serial.println("`[section] setting = value; = enter config settings (pro tip: copy/paste setting from ~ output and just change the value)");
    // Serial.println("\n\r    config setting format (prefix with ` to paste from main menu)\n\r");    
    // Serial.println("    Example: ");
    // Serial.println("`[serial_1]connect_on_boot = true;");
    cycleTerminalColor(true, 15.0, true,  &Serial, 1, 1);

    Serial.println("\n\r");
    Serial.println("                              Reset config");
    cycleTerminalColor(false, 2.0, true,  &Serial, 0, 1);
    Serial.println("                     `reset = reset to defaults (keeps calibration and hardware version)");
    Serial.println("            `reset_hardware = reset hardware settings (keeps calibration)");
    Serial.println("         `reset_calibration = reset calibration settings (keeps hardware version)");
    Serial.println("                 `reset_all = reset to defaults and clear all settings");
    cycleTerminalColor(false, 1.0, true,  &Serial, 0, 1);
    Serial.println("         `force_first_start = clears everything to factory settings and runs first startup calibration");

    cycleTerminalColor(true, 15.0, true,  &Serial, 18, 1);
    Serial.println("\n\r");
    Serial.println("                              Help");
    cycleTerminalColor(false, 1.0, true,  &Serial, 0, 1);
    Serial.println("                         ~? = show this help\n\r");
    // Serial.println("\n\r\tor you can use dot notation\n\r");
    // Serial.println("`config.routing.stack_paths = 1;");
    // Serial.println("\n\r\tor paste a whole section\n\r");
    // Serial.println("`[dacs]");
    // Serial.println("top_rail = 5.0;");
    // Serial.println("bottom_rail = 3.3;");
    // Serial.println("dac_0 = -2.0;");
    // Serial.println("dac_1 = 3.33;");
    Serial.println("\n\r");
    delayMicroseconds(3000);
}

void printConfigToSerial(bool showNamesArg) {
    char line[128] = {0};
    int lineIndex = 0;
    unsigned long lastCharTime = millis();
    const unsigned long timeout = 1000; // 100ms timeout

    // Wait for input with timeout
    while (true) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (lineIndex < sizeof(line) - 1) {
                line[lineIndex++] = c;
                line[lineIndex] = '\0';
                lastCharTime = millis();
            }

            // Check for ~names or ~numbers
            if (strncmp(line, "names", 5) == 0) {
                showNames = 1;
                Serial.println("showing names");
                lineIndex = 0;
                line[0] = '\0';
                continue;
            } else if (strncmp(line, "numbers", 7) == 0) {
                showNames = 0;
                Serial.println("showing numbers");
                lineIndex = 0;
                line[0] = '\0';
                continue;
            } else if (strncmp(line, "help", 4) == 0 || strncmp(line, "?", 1) == 0 || strncmp(line, "-h", 2) == 0 || strncmp(line, "--help", 6) == 0) {
                printConfigHelp();
                lineIndex = 0;
                line[0] = '\0';
                continue;
            }

            // Check if we have a section
            if (lineIndex >= 2 && line[0] == '[') {
                char* endBracket = strchr(line, ']');
                if (endBracket) {
                    char sectionName[32] = {0};
                    strncpy(sectionName, line + 1, endBracket - (line + 1));
                    sectionName[endBracket - (line + 1)] = '\0';

                    int section = parseSectionName(sectionName);
                    if (section != -1) {
                        printConfigSectionToSerial(section, showNamesArg);
                    } else {
                        Serial.print("Unknown section: ");
                        Serial.println(sectionName);
                    }
                    return;
                }
            }
        }

        // Check for timeout
        if (millis() - lastCharTime > timeout) {
            printConfigSectionToSerial(-1, showNamesArg);
            Serial.println("\n\n");
            return;
        }
    }
}

void readConfigFromSerial() {
    char line[128] = {0};
    int lineIndex = 0;
    char currentSection[32] = {0};
    bool inSection = false;
    Serial.println("\n\renter config settings (? for help)\n\r");


bool ledChange = false;
bool dacChange = false;
    unsigned long lastCharTime = millis();
    const unsigned long timeout = 10;

    while (Serial.available() == 0) {
        // delayMicroseconds(10);
        if (millis() - lastCharTime > 400) {
            //Serial.println("No input detected. Showing help.");
            printConfigHelp();
            return;
        }
    }
    int timedOut = 0;
    while (true) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
               // parseSetting(line);
                // Serial.println("New line");
            }

            //lastCharTime = millis();

            // Handle backspace
            if (c == '\b' || c == 0x7F) {
                if (lineIndex > 0) {
                    lineIndex--;
                    Serial.print(" \b"); // Erase character
                }
                continue;
            }

            // Add character to line buffer if there's space
            if (lineIndex < sizeof(line) - 1) {
                line[lineIndex++] = c;
                line[lineIndex] = '\0'; // Keep string null-terminated

                // Check for help commands as soon as they're typed
                if (strcmp(line, "?") == 0 || strcmp(line, "-h") == 0 || strcmp(line, "--help") == 0 || strcmp(line, "help") == 0) {
                    printConfigHelp();
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                }

                // Check for reset
                if (strcmp(line, "reset") == 0) {
                    resetConfigToDefaults();
                    saveConfigToFile("/config.txt");
                    Serial.println("Done. Settings have been reset to defaults");
                    ledChange = true;
                    dacChange = true;
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                } else if (strcmp(line, "clear_calibration") == 0 || strcmp(line, "clear_cal") == 0 || strcmp(line, "reset_calibration") == 0 || strcmp(line, "reset_cal") == 0) {
                    resetConfigToDefaults(1, 0);
                    Serial.println("Done. Calibration has been cleared");
                    ledChange = true;
                    dacChange = true;
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                } else if (strcmp(line, "clear_hardware") == 0 || strcmp(line, "clear_hw") == 0 || strcmp(line, "reset_hardware") == 0 || strcmp(line, "reset_hw") == 0) {
                    resetConfigToDefaults(0, 1);
                    Serial.println("Done. Hardware has been cleared");
                    ledChange = true;
                    dacChange = true;
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                } else if (strcmp(line, "clear_all") == 0 || strcmp(line, "reset_all") == 0) {
                    resetConfigToDefaults(1, 1);
                    Serial.println("Done. All settings have been cleared");
                    ledChange = true;
                    dacChange = true;
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                } else if (strcmp(line, "clear_filesystem") == 0 || strcmp(line, "reset_filesystem") == 0) {
                    Serial.println("Deleting all filesystem contents...");
                    bool deleteSuccess = deleteDirectoryContents("/");
                    Serial.println("Filesystem contents deleted.");
                    continue;
                
                } else if (strcmp(line, "force_first_start") == 0 || strcmp(line, "factory_reset") == 0) {
                    //firstStart = 1;
                    cycleTerminalColor(true, 100.0, true,  &Serial, 0, 1);
                    FatFS.remove("/config.txt");

                    Serial.println("Config file deleted.");
                    Serial.flush();
                    
                    // // Delete all contents of the filesystem recursively
                    bool deleteSuccess = deleteDirectoryContents("/");
                    
                    cycleTerminalColor(false, 100.0, true,  &Serial, 0, 1);
                    if (deleteSuccess) {
                        Serial.println("All filesystem contents deleted successfully.");
                    } else {
                        Serial.println("Some files/directories could not be deleted (this may be normal).");
                    }
                    Serial.flush();


                    

                    EEPROM.write(FIRSTSTARTUPADDRESS, 0x00);
                    EEPROM.commit();
                    cycleTerminalColor(false, 100.0, true,  &Serial, 0, 1);
                    Serial.println("First startup flag cleared.");
                    Serial.flush();
                    

                    cycleTerminalColor(false, 100.0, true,  &Serial, 0, 1);
                    Serial.println("Done. All settings have been cleared");
                    delay(200);
                    cycleTerminalColor(false, 100.0, true,  &Serial, 0, 1);
                   // Serial.println("\n\rPower cycle your Jumperless to reset config and force startup calibration.");

                    unsigned long startTime = millis()+1000;
                    int dots = 0;
//return;
                    while (millis() < 3000) {
                         if (millis() - startTime > 500) {
                        Serial.print("\r                                           \r");
                        Serial.print("Power cycling");
                       
                            dots++;
                            for (int i = 0; i < dots; i++) {
                                Serial.print(".");
                            }
                            startTime = millis();
                        }
                        if (dots >= 3) {
                            
                            dots = 0;
                        }
                        Serial.flush();
            
                    }
                    // saveConfigToFile("/config.txt");
                    // Serial.println("Done. All settings have been reset to defaults");
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    rp2040.reboot();
                    continue;
                }
            }
            // ... existing code ...
                        // Process line when newline or semicolon is received
            if (c == '\n' || c == '\r' || c == ';') {
                if (lineIndex > 0) {
                    line[lineIndex] = '\0';
                    
                    // Check if this is a section header
                    if (line[0] == '[') {
                        char* endBracket = strchr(line, ']');
                        if (endBracket) {
                            // If there's content after the closing bracket, split it into section header and setting
                            if (endBracket[1] != '\0') {
                                // Save the section header
                                *endBracket = '\0';
                                strncpy(currentSection, line + 1, sizeof(currentSection) - 1);
                                inSection = true;
                                
                                // Process the setting part if it exists
                                const char* settingPart = endBracket + 1;
                                while (*settingPart == ' ' || *settingPart == '\t') settingPart++; // Skip whitespace
                                if (*settingPart != '\0') {
                                    char section[32], key[32], value[64];
                                    char tempLine[256];
                                    snprintf(tempLine, sizeof(tempLine), "[%s]%s", currentSection, settingPart);
                                    if (parseSetting(tempLine, section, key, value)) {
                                        updateConfigValue(section, key, value);
                                    }
                                }
                            } else {
                                // Pure section header
                                *endBracket = '\0';
                                strncpy(currentSection, line + 1, sizeof(currentSection) - 1);
                                inSection = true;
                            }
                        }
                    }
                    // Check if this is dot notation format
                    else if (strncmp(line, "config.", 7) == 0) {
                        char section[32], key[32], value[64];
                        if (parseSetting(line, section, key, value)) {
                            updateConfigValue(section, key, value);
                        }
                    }
                    // Process key=value pair if we're in a section
                    else if (inSection && strchr(line, '=')) {
                        char section[32], key[32], value[64];
                        strcpy(section, currentSection);
                        
                        // Create a temporary line with section header for parsing
                        char tempLine[256];
                        snprintf(tempLine, sizeof(tempLine), "[%s]%s", section, line);
                        
                        if (parseSetting(tempLine, section, key, value)) {
                            updateConfigValue(section, key, value);
                        }
                    }
                    
                    // Clear line buffer but maintain section context
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                }
            }
        } else if (millis() - lastCharTime > 10) {
            lastCharTime = millis();
            timedOut++;
           

        }
 //Serial.println(timedOut);
        if (timedOut > timeout) {
            // printConfigHelp();
            // Serial.println("\n\r");
            // Serial.flush();
            memset(line, 0, sizeof(line));
            lineIndex = 0;

            break;
        }
    }

    while (Serial.available() > 0) {
        Serial.read();
        delayMicroseconds(100);
    }
   // configChanged = true;
   readSettingsFromConfig();
//    Serial.println(railVoltage[0]);
//    Serial.println(railVoltage[1]);
//    Serial.println(dacOutput[0]);
//    Serial.println(dacOutput[1]);
    setRailsAndDACs(0);
    showLEDsCore2 = -1;
}

int parseTrueFalse(const char* value) {
    if (strcmp(value, "true") == 0) return 1;
    else if (strcmp(value, "false") == 0) return 0;
    else if (strcmp(value, "1") == 0) return 1;
    else if (strcmp(value, "0") == 0) return 0;
    else return -1;
}

void updateConfigValue(const char* section, const char* key, const char* value) {
    char oldValue[64] = {0};
     //! this is a place to add new config options
    // Get old value
    if (strcmp(section, "hardware") == 0) {
        if (strcmp(key, "generation") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware.generation);
        else if (strcmp(key, "revision") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware.revision);
        else if (strcmp(key, "probe_revision") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware.probe_revision);
    }
    else if (strcmp(section, "dacs") == 0) {
        if (strcmp(key, "top_rail") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.top_rail);
        else if (strcmp(key, "bottom_rail") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.bottom_rail);
        else if (strcmp(key, "dac_0") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.dac_0);
        else if (strcmp(key, "dac_1") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.dac_1);
        else if (strcmp(key, "set_dacs_on_boot") == 0) sprintf(oldValue, "%d", jumperlessConfig.dacs.set_dacs_on_boot);
        else if (strcmp(key, "set_rails_on_boot") == 0) sprintf(oldValue, "%d", jumperlessConfig.dacs.set_rails_on_boot);
        else if (strcmp(key, "probe_power_dac") == 0) sprintf(oldValue, "%d", jumperlessConfig.dacs.probe_power_dac);
        else if (strcmp(key, "limit_max") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.limit_max);
        else if (strcmp(key, "limit_min") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dacs.limit_min);
    }
    else if (strcmp(section, "debug") == 0) {
        if (strcmp(key, "file_parsing") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.file_parsing);
        else if (strcmp(key, "net_manager") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.net_manager);
        else if (strcmp(key, "nets_to_chips") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.nets_to_chips);
        else if (strcmp(key, "nets_to_chips_alt") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.nets_to_chips_alt);
        else if (strcmp(key, "leds") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.leds);
        else if (strcmp(key, "logic_analyzer") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.logic_analyzer);
        else if (strcmp(key, "arduino") == 0) sprintf(oldValue, "%d", jumperlessConfig.debug.arduino);
    }
    else if (strcmp(section, "routing") == 0) {
        if (strcmp(key, "stack_paths") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing.stack_paths);
        else if (strcmp(key, "stack_rails") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing.stack_rails);
        else if (strcmp(key, "stack_dacs") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing.stack_dacs);
        else if (strcmp(key, "rail_priority") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing.rail_priority);
    }
    else if (strcmp(section, "calibration") == 0) {
        if (strcmp(key, "top_rail_zero") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.top_rail_zero);
        else if (strcmp(key, "top_rail_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.top_rail_spread);
        else if (strcmp(key, "bottom_rail_zero") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.bottom_rail_zero);
        else if (strcmp(key, "bottom_rail_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.bottom_rail_spread);
        else if (strcmp(key, "dac_0_zero") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.dac_0_zero);
        else if (strcmp(key, "dac_0_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.dac_0_spread);
        else if (strcmp(key, "dac_1_zero") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.dac_1_zero);
        else if (strcmp(key, "dac_1_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.dac_1_spread);
        else if (strcmp(key, "adc_0_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_0_zero);
        else if (strcmp(key, "adc_0_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_0_spread);
        else if (strcmp(key, "adc_1_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_1_zero);
        else if (strcmp(key, "adc_1_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_1_spread);
        else if (strcmp(key, "adc_2_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_2_zero);
        else if (strcmp(key, "adc_2_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_2_spread);
        else if (strcmp(key, "adc_3_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_3_zero);
        else if (strcmp(key, "adc_3_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_3_spread);
        else if (strcmp(key, "adc_4_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_4_zero);
        else if (strcmp(key, "adc_4_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_4_spread);
        else if (strcmp(key, "adc_7_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_7_zero);
        else if (strcmp(key, "adc_7_spread") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.adc_7_spread);
        else if (strcmp(key, "probe_max") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.probe_max);
        else if (strcmp(key, "probe_min") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.probe_min);
        else if (strcmp(key, "probe_switch_threshold") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.probe_switch_threshold);
        else if (strcmp(key, "probe_current_zero") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.calibration.probe_current_zero);
        }
    else if (strcmp(section, "logo_pads") == 0) {
        if (strcmp(key, "top_guy") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pads.top_guy);
        else if (strcmp(key, "bottom_guy") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pads.bottom_guy);
        else if (strcmp(key, "building_pad_top") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pads.building_pad_top);
        else if (strcmp(key, "building_pad_bottom") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pads.building_pad_bottom);
    }
    else if (strcmp(section, "display") == 0) {
        if (strcmp(key, "lines_wires") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.lines_wires);
        else if (strcmp(key, "menu_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.menu_brightness);
        else if (strcmp(key, "led_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.led_brightness);
        else if (strcmp(key, "rail_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.rail_brightness);
        else if (strcmp(key, "special_net_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.special_net_brightness);
        else if (strcmp(key, "net_color_mode") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.net_color_mode);
        else if (strcmp(key, "dump_leds") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.dump_leds);
        else if (strcmp(key, "dump_format") == 0) sprintf(oldValue, "%d", jumperlessConfig.display.dump_format);
    }
    else if (strcmp(section, "gpio") == 0) {
        if (strcmp(key, "direction") == 0) {
            char temp[128] = {0};
                for (int i = 0; i < 10; i++) {
                if (i > 0) strcat(temp, ",");
                char num[4];
                sprintf(num, "%d", jumperlessConfig.gpio.direction[i]);
                strcat(temp, num);
            }
            strcpy(oldValue, temp);
        }
        else if (strcmp(key, "pulls") == 0) {
            char temp[128] = {0};
            for (int i = 0; i < 10; i++) {
                if (i > 0) strcat(temp, ",");
                char num[4];
                sprintf(num, "%d", jumperlessConfig.gpio.pulls[i]);
                strcat(temp, num);
            }
            strcpy(oldValue, temp);
        }
        else if (strcmp(key, "uart_tx_function") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_tx_function);
        else if (strcmp(key, "uart_rx_function") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_rx_function);
    }
    else if (strcmp(section, "serial_1") == 0) {
        if (strcmp(key, "function") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.function);
        else if (strcmp(key, "baud_rate") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.baud_rate);
        else if (strcmp(key, "print_passthrough") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.print_passthrough);
        else if (strcmp(key, "connect_on_boot") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.connect_on_boot);
        else if (strcmp(key, "lock_connection") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.lock_connection);
        else if (strcmp(key, "autoconnect_flashing") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.autoconnect_flashing);
        else if (strcmp(key, "async_passthrough") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_1.async_passthrough);
    }
    else if (strcmp(section, "serial_2") == 0) {
        if (strcmp(key, "function") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.function);
        else if (strcmp(key, "baud_rate") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.baud_rate);
        else if (strcmp(key, "print_passthrough") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.print_passthrough);
        else if (strcmp(key, "connect_on_boot") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.connect_on_boot);
        else if (strcmp(key, "lock_connection") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.lock_connection);
        else if (strcmp(key, "autoconnect_flashing") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial_2.autoconnect_flashing);
    }
    else if (strcmp(section, "top_oled") == 0) {
        if (strcmp(key, "enabled") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.enabled);
        else if (strcmp(key, "i2c_address") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.i2c_address);
        else if (strcmp(key, "width") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.width);
        else if (strcmp(key, "height") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.height);
        else if (strcmp(key, "sda_pin") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.sda_pin);
        else if (strcmp(key, "scl_pin") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.scl_pin);
        else if (strcmp(key, "gpio_sda") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.gpio_sda);
        else if (strcmp(key, "gpio_scl") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.gpio_scl);
        else if (strcmp(key, "sda_row") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.sda_row);
        else if (strcmp(key, "scl_row") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.scl_row);
        else if (strcmp(key, "connect_on_boot") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.connect_on_boot);
        else if (strcmp(key, "lock_connection") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.lock_connection);
        else if (strcmp(key, "show_in_terminal") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.show_in_terminal);
        else if (strcmp(key, "font") == 0) sprintf(oldValue, "%d", jumperlessConfig.top_oled.font);
    }
    // Update the config structure
    // Accept string names for enums/bools and convert to int
    if (strcmp(section, "hardware") == 0) {
        if (strcmp(key, "generation") == 0) jumperlessConfig.hardware.generation = parseInt(value);
        else if (strcmp(key, "revision") == 0) jumperlessConfig.hardware.revision = parseInt(value);
        else if (strcmp(key, "probe_revision") == 0) jumperlessConfig.hardware.probe_revision = parseInt(value);
    }
    else if (strcmp(section, "dacs") == 0) {
        if (strcmp(key, "top_rail") == 0) jumperlessConfig.dacs.top_rail = parseFloat(value);
        else if (strcmp(key, "bottom_rail") == 0) jumperlessConfig.dacs.bottom_rail = parseFloat(value);
        else if (strcmp(key, "dac_0") == 0) jumperlessConfig.dacs.dac_0 = parseFloat(value);
        else if (strcmp(key, "dac_1") == 0) jumperlessConfig.dacs.dac_1 = parseFloat(value);
        else if (strcmp(key, "set_dacs_on_boot") == 0) jumperlessConfig.dacs.set_dacs_on_boot = parseBool(value);
        else if (strcmp(key, "set_rails_on_boot") == 0) jumperlessConfig.dacs.set_rails_on_boot = parseBool(value);
        else if (strcmp(key, "probe_power_dac") == 0) jumperlessConfig.dacs.probe_power_dac = parseInt(value);
        else if (strcmp(key, "limit_max") == 0) jumperlessConfig.dacs.limit_max = parseFloat(value);
        else if (strcmp(key, "limit_min") == 0) jumperlessConfig.dacs.limit_min = parseFloat(value);
    }
    else if (strcmp(section, "debug") == 0) {
        if (strcmp(key, "file_parsing") == 0) jumperlessConfig.debug.file_parsing = parseBool(value);
        else if (strcmp(key, "net_manager") == 0) jumperlessConfig.debug.net_manager = parseBool(value);
        else if (strcmp(key, "nets_to_chips") == 0) jumperlessConfig.debug.nets_to_chips = parseBool(value);
        else if (strcmp(key, "nets_to_chips_alt") == 0) jumperlessConfig.debug.nets_to_chips_alt = parseBool(value);
        else if (strcmp(key, "leds") == 0) jumperlessConfig.debug.leds = parseBool(value);
        else if (strcmp(key, "logic_analyzer") == 0) jumperlessConfig.debug.logic_analyzer = parseBool(value);
        else if (strcmp(key, "arduino") == 0) jumperlessConfig.debug.arduino = parseInt(value);
    }
    else if (strcmp(section, "routing") == 0) {
        if (strcmp(key, "stack_paths") == 0) jumperlessConfig.routing.stack_paths = parseInt(value);
        else if (strcmp(key, "stack_rails") == 0) jumperlessConfig.routing.stack_rails = parseInt(value);
        else if (strcmp(key, "stack_dacs") == 0) jumperlessConfig.routing.stack_dacs = parseInt(value);
        else if (strcmp(key, "rail_priority") == 0) jumperlessConfig.routing.rail_priority = parseInt(value);
    }
    else if (strcmp(section, "calibration") == 0) {
        if (strcmp(key, "top_rail_zero") == 0) jumperlessConfig.calibration.top_rail_zero = parseInt(value);
        else if (strcmp(key, "top_rail_spread") == 0) jumperlessConfig.calibration.top_rail_spread = parseFloat(value);
        else if (strcmp(key, "bottom_rail_zero") == 0) jumperlessConfig.calibration.bottom_rail_zero = parseInt(value);
        else if (strcmp(key, "bottom_rail_spread") == 0) jumperlessConfig.calibration.bottom_rail_spread = parseFloat(value);
        else if (strcmp(key, "dac_0_zero") == 0) jumperlessConfig.calibration.dac_0_zero = parseInt(value);
        else if (strcmp(key, "dac_0_spread") == 0) jumperlessConfig.calibration.dac_0_spread = parseFloat(value);
        else if (strcmp(key, "dac_1_zero") == 0) jumperlessConfig.calibration.dac_1_zero = parseInt(value);
        else if (strcmp(key, "dac_1_spread") == 0) jumperlessConfig.calibration.dac_1_spread = parseFloat(value);
        else if (strcmp(key, "adc_0_zero") == 0) jumperlessConfig.calibration.adc_0_zero = parseFloat(value);
        else if (strcmp(key, "adc_0_spread") == 0) jumperlessConfig.calibration.adc_0_spread = parseFloat(value);
        else if (strcmp(key, "adc_1_zero") == 0) jumperlessConfig.calibration.adc_1_zero = parseFloat(value);
        else if (strcmp(key, "adc_1_spread") == 0) jumperlessConfig.calibration.adc_1_spread = parseFloat(value);
        else if (strcmp(key, "adc_2_zero") == 0) jumperlessConfig.calibration.adc_2_zero = parseFloat(value);
        else if (strcmp(key, "adc_2_spread") == 0) jumperlessConfig.calibration.adc_2_spread = parseFloat(value);
        else if (strcmp(key, "adc_3_zero") == 0) jumperlessConfig.calibration.adc_3_zero = parseFloat(value);
        else if (strcmp(key, "adc_3_spread") == 0) jumperlessConfig.calibration.adc_3_spread = parseFloat(value);
        else if (strcmp(key, "adc_4_zero") == 0) jumperlessConfig.calibration.adc_4_zero = parseFloat(value);
        else if (strcmp(key, "adc_4_spread") == 0) jumperlessConfig.calibration.adc_4_spread = parseFloat(value);
        else if (strcmp(key, "adc_7_zero") == 0) jumperlessConfig.calibration.adc_7_zero = parseFloat(value);
        else if (strcmp(key, "adc_7_spread") == 0) jumperlessConfig.calibration.adc_7_spread = parseFloat(value);
        else if (strcmp(key, "probe_max") == 0) jumperlessConfig.calibration.probe_max = parseInt(value);
        else if (strcmp(key, "probe_min") == 0) jumperlessConfig.calibration.probe_min = parseInt(value);
        else if (strcmp(key, "measure_mode_output_voltage") == 0) jumperlessConfig.calibration.measure_mode_output_voltage = parseFloat(value);
        else if (strcmp(key, "probe_switch_threshold") == 0) jumperlessConfig.calibration.probe_switch_threshold = parseFloat(value);
        else if (strcmp(key, "probe_current_zero") == 0) jumperlessConfig.calibration.probe_current_zero = parseFloat(value);
        }
    else if (strcmp(section, "logo_pads") == 0) {
        if (strcmp(key, "top_guy") == 0) jumperlessConfig.logo_pads.top_guy = parseArbitraryFunction(value);
        else if (strcmp(key, "bottom_guy") == 0) jumperlessConfig.logo_pads.bottom_guy = parseArbitraryFunction(value);
        else if (strcmp(key, "building_pad_top") == 0) jumperlessConfig.logo_pads.building_pad_top = parseArbitraryFunction(value);
        else if (strcmp(key, "building_pad_bottom") == 0) jumperlessConfig.logo_pads.building_pad_bottom = parseArbitraryFunction(value);
    }
    else if (strcmp(section, "display") == 0) {
        if (strcmp(key, "lines_wires") == 0) jumperlessConfig.display.lines_wires = parseLinesWires(value);
        else if (strcmp(key, "menu_brightness") == 0) jumperlessConfig.display.menu_brightness = parseInt(value);
        else if (strcmp(key, "led_brightness") == 0) jumperlessConfig.display.led_brightness = parseInt(value);
        else if (strcmp(key, "rail_brightness") == 0) jumperlessConfig.display.rail_brightness = parseInt(value);
        else if (strcmp(key, "special_net_brightness") == 0) jumperlessConfig.display.special_net_brightness = parseInt(value);
        else if (strcmp(key, "net_color_mode") == 0) jumperlessConfig.display.net_color_mode = parseNetColorMode(value);
        else if (strcmp(key, "dump_leds") == 0) jumperlessConfig.display.dump_leds = parseSerialPort(value);
        else if (strcmp(key, "dump_format") == 0) jumperlessConfig.display.dump_format = parseDumpFormat(value);
    }
    else if (strcmp(section, "gpio") == 0) {
        if (strcmp(key, "direction") == 0) {
            parseCommaSeparatedInts(value, jumperlessConfig.gpio.direction, 10);
        }
        else if (strcmp(key, "pulls") == 0) {
            parseCommaSeparatedInts(value, jumperlessConfig.gpio.pulls, 10);
        }
        else if (strcmp(key, "pwm_frequency") == 0) {
            parseCommaSeparatedFloats(value, jumperlessConfig.gpio.pwm_frequency, 10);
        }
        else if (strcmp(key, "pwm_duty_cycle") == 0) {
            parseCommaSeparatedFloats(value, jumperlessConfig.gpio.pwm_duty_cycle, 10);
        }
        else if (strcmp(key, "pwm_enabled") == 0) {
            parseCommaSeparatedBools(value, jumperlessConfig.gpio.pwm_enabled, 10);
        }
        else if (strcmp(key, "uart_tx_function") == 0) jumperlessConfig.gpio.uart_tx_function = parseArbitraryFunction(value);
        else if (strcmp(key, "uart_rx_function") == 0) jumperlessConfig.gpio.uart_rx_function = parseArbitraryFunction(value);
    }
    else if (strcmp(section, "serial_1") == 0) {
        if (strcmp(key, "function") == 0) jumperlessConfig.serial_1.function = parseUartFunction(value);
        else if (strcmp(key, "baud_rate") == 0) jumperlessConfig.serial_1.baud_rate = parseInt(value);
        else if (strcmp(key, "print_passthrough") == 0) jumperlessConfig.serial_1.print_passthrough = parseBool(value);
        else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.serial_1.connect_on_boot = parseBool(value);
        else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.serial_1.lock_connection = parseBool(value);
        else if (strcmp(key, "autoconnect_flashing") == 0) jumperlessConfig.serial_1.autoconnect_flashing = parseBool(value);
        else if (strcmp(key, "async_passthrough") == 0) jumperlessConfig.serial_1.async_passthrough = parseBool(value);
    }
    else if (strcmp(section, "serial_2") == 0) {
        if (strcmp(key, "function") == 0) jumperlessConfig.serial_2.function = parseUartFunction(value);
        else if (strcmp(key, "baud_rate") == 0) jumperlessConfig.serial_2.baud_rate = parseInt(value);
        else if (strcmp(key, "print_passthrough") == 0) jumperlessConfig.serial_2.print_passthrough = parseBool(value);
        else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.serial_2.connect_on_boot = parseBool(value);
        else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.serial_2.lock_connection = parseBool(value);
        else if (strcmp(key, "autoconnect_flashing") == 0) jumperlessConfig.serial_2.autoconnect_flashing = parseBool(value);
    }
    else if (strcmp(section, "top_oled") == 0) {
        if (strcmp(key, "enabled") == 0) jumperlessConfig.top_oled.enabled = parseBool(value);
        if (strcmp(key, "i2c_address") == 0) jumperlessConfig.top_oled.i2c_address = parseHex(value);
        else if (strcmp(key, "width") == 0) jumperlessConfig.top_oled.width = parseInt(value);
        else if (strcmp(key, "height") == 0) jumperlessConfig.top_oled.height = parseInt(value);
        else if (strcmp(key, "sda_pin") == 0) jumperlessConfig.top_oled.sda_pin = parseInt(value);
        else if (strcmp(key, "scl_pin") == 0) jumperlessConfig.top_oled.scl_pin = parseInt(value);
        else if (strcmp(key, "gpio_sda") == 0) jumperlessConfig.top_oled.gpio_sda = parseInt(value);
        else if (strcmp(key, "gpio_scl") == 0) jumperlessConfig.top_oled.gpio_scl = parseInt(value);
        else if (strcmp(key, "sda_row") == 0) jumperlessConfig.top_oled.sda_row = parseInt(value);
        else if (strcmp(key, "scl_row") == 0) jumperlessConfig.top_oled.scl_row = parseInt(value);
        else if (strcmp(key, "connect_on_boot") == 0) jumperlessConfig.top_oled.connect_on_boot = parseBool(value);
        else if (strcmp(key, "lock_connection") == 0) jumperlessConfig.top_oled.lock_connection = parseBool(value);
        else if (strcmp(key, "show_in_terminal") == 0) jumperlessConfig.top_oled.show_in_terminal = parseSerialPort(value);
        else if (strcmp(key, "font") == 0) {
            jumperlessConfig.top_oled.font = parseFont(value);
            
            // Convert config value directly to FontFamily (now sequential)
            FontFamily fontFamily = (FontFamily)jumperlessConfig.top_oled.font;
            
            // Set font using the new smart font system (default to size 1)
            oled.setFontForSize(fontFamily, 1);
            oled.show();
        }
    }
    saveConfigToFile("/config.txt");
    printSettingChange(section, key, oldValue, value);
}

// Fast config parsing function optimized for tight loops
// Returns true if valid config setting was parsed and updated, false otherwise
// Designed to return quickly for invalid strings to minimize loop overhead
//
// Usage example in a tight loop:
// while (someCondition) {
//     char* inputString = getNextString(); // Your string source
//     if (fastParseAndUpdateConfig(inputString)) {
//         // Config was successfully updated
//         Serial.println("Config updated");
//     }
//     // Function returns quickly for invalid strings, minimizing loop overhead
// }
//
// Supported formats:
// - Dot notation: "config.section.key = value"
// - Bracket notation: "`[section]key = value"
bool fastParseAndUpdateConfig(const char* configString) {
    // Quick validation - must have minimum length and contain '='
    if (!configString || strlen(configString) < 5) {
        Serial.println("too short");
        return false;
    }
    
    const char* equals = strchr(configString, '=');
    if (!equals) {
        Serial.println("no equals");
        return false;
    }
    
    // Quick check for valid config formats
    bool isDotNotation = (strncmp(configString, "config.", 7) == 0);
    bool isBracketNotation = (configString[0] == '`' && configString[1] == '[');
    
    if (!isDotNotation && !isBracketNotation) {
       // Serial.println(configString);
        Serial.println("not a dot or bracket notation");
        Serial.println(configString);
        return false;
    }
    
    // Use existing parsing logic but with early returns for efficiency
    char section[32], key[32], value[64];
    
    if (isDotNotation) {
        // Parse dot notation: config.section.key = value
        const char* start = configString + 7;  // Skip "config."
        const char* firstDot = strchr(start, '.');
        
        if (!firstDot || firstDot >= equals) {
            Serial.println("not a valid dot notation");
            return false;
        }
        
        // Extract section
        int sectionLen = firstDot - start;
        if (sectionLen >= sizeof(section)) {
            Serial.println("section too long");
            return false;
        }
        strncpy(section, start, sectionLen);
        section[sectionLen] = '\0';
        
        // Extract key
        const char* keyStart = firstDot + 1;
        int keyLen = equals - keyStart;
        if (keyLen >= sizeof(key) || keyLen <= 0) {
            Serial.println("key too long");
            return false;
        }
        strncpy(key, keyStart, keyLen);
        key[keyLen] = '\0';
        trim(key);
        
        // Extract value
        const char* valueStart = equals + 1;
        while (isspace(*valueStart)) valueStart++; // Skip leading whitespace
        if (strlen(valueStart) >= sizeof(value)) {
            Serial.println("value too long");
            return false;
        }
        strcpy(value, valueStart);
        
        // Trim trailing whitespace and semicolon from value
        char* end = value + strlen(value) - 1;
        while (end > value && (isspace(*end) || *end == ';')) {
            *end = '\0';
            end--;
        }
        
    } else if (isBracketNotation) {
        // Parse bracket notation: [section]key = value
        const char* sectionEnd = strchr(configString, ']');
        if (!sectionEnd || sectionEnd >= equals) {
            Serial.println("not a valid bracket notation");
            return false;
        }
        
        // Extract section (skip the `[)
        int sectionLen = sectionEnd - configString - 2;  // -2 to account for `[
        if (sectionLen >= sizeof(section) || sectionLen <= 0) {
            Serial.println("section too long");
            return false;
        }
        strncpy(section, configString + 2, sectionLen);
        section[sectionLen] = '\0';
        
        // Extract key (skip the ])
        const char* keyStart = sectionEnd + 1;
        while (isspace(*keyStart)) keyStart++; // Skip leading whitespace
        
        int keyLen = equals - keyStart;
        if (keyLen >= sizeof(key) || keyLen <= 0) {
            Serial.println("key too long");
            return false;
        }
        strncpy(key, keyStart, keyLen);
        key[keyLen] = '\0';
        
        // Remove trailing whitespace from key
        char* keyEnd = key + strlen(key) - 1;
        while (keyEnd > key && isspace(*keyEnd)) {
            *keyEnd = '\0';
            keyEnd--;
        }
        
        // Extract value (skip the =)
        const char* valueStart = equals + 1;
        while (isspace(*valueStart)) valueStart++; // Skip leading whitespace
        if (strlen(valueStart) >= sizeof(value)) {
            Serial.println("value too long");
                    return false;
        }
        strcpy(value, valueStart);
        
        // Trim trailing whitespace and semicolon from value
        char* end = value + strlen(value) - 1;
        while (end > value && (isspace(*end) || *end == ';')) {
            *end = '\0';
            end--;
        }
    }
    
    // Quick validation that we have non-empty section, key, and value
    if (strlen(section) == 0 || strlen(key) == 0 || strlen(value) == 0) {
        Serial.println("section, key, or value is empty");
        return false;
    }
    
    // Convert section to lowercase for comparison
    for(int i = 0; section[i]; i++) {
        section[i] = tolower(section[i]);
    }
    
    // Quick section validation - only proceed if it's a known section
    if (strcmp(section, "config") != 0 &&
        strcmp(section, "hardware") != 0 && 
        strcmp(section, "dacs") != 0 && 
        strcmp(section, "debug") != 0 && 
        strcmp(section, "routing") != 0 && 
        strcmp(section, "calibration") != 0 && 
        strcmp(section, "logo_pads") != 0 && 
        strcmp(section, "display") != 0 && 
        strcmp(section, "gpio") != 0 && 
        strcmp(section, "serial_1") != 0 && 
        strcmp(section, "serial_2") != 0 && 
        strcmp(section, "top_oled") != 0) {
        Serial.println("section not found");
        Serial.println(section);
        return false;
    }
    
    // Update the config value using existing function
    //updateConfigValue(section, key, value);
    // Serial.print("section: ");
    // Serial.println(section);
    // Serial.print("key: ");
    // Serial.println(key);
    // Serial.print("value: ");
    // Serial.println(value);
    updateConfigValue(section, key, value);
    //configChanged = true;
    return true;
}

const char* getArbitraryFunctionString(int function) {
    for (int i = 0; i < arbitraryFunctionTableSize; i++) {
        if (arbitraryFunctionTable[i].value == function) {
            return arbitraryFunctionTable[i].name;
        }
    }
    return NULL; // or some default string
}

template <size_t N>
const char* getStringFromTable(int value, const StringIntEntry (&table)[N]) {
    if (showNames) {
        for (size_t i = 0; i < N; i++) {
            if (table[i].value == value) {
                // Serial.print("getStringFromTable: ");
                // Serial.println(table[i].name);
                return table[i].name;
            }
        }
    } else {
        static char buf[16];
        snprintf(buf, sizeof(buf), "%d", value);
        return buf;
    }
    return NULL; // or some default string
}
