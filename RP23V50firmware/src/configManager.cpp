#include "configManager.h"
#include <FatFS.h>
#include "MatrixStateRP2040.h"
#include "config.h"
#include "PersistentStuff.h"
#include "LEDs.h"
#include "Commands.h"
#include "FileParsing.h"

// Define the global configuration instance

bool configChanged = false;

struct config jumperlessConfig;

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

bool parseBool(const char* str) {
    char lower[32];
    strncpy(lower, str, sizeof(lower)-1);
    lower[sizeof(lower)-1] = '\0';
    toLower(lower);
    return (strcmp(lower, "true") == 0) || (strcmp(lower, "1") == 0);
}

float parseFloat(const char* str) {
    return atof(str);
}

int parseInt(const char* str) {
    return atoi(str);
}

void resetConfigToDefaults(void) {
    // Save current hardware version values
    int saved_generation = jumperlessConfig.hardware_version.generation;
    int saved_hardware_revision = jumperlessConfig.hardware_version.hardware_revision;
    int saved_probe_version = jumperlessConfig.hardware_version.probe_version;
    
    // Initialize with default values from config.h
    jumperlessConfig = config();
    
    // Restore hardware version values
    jumperlessConfig.hardware_version.generation = saved_generation;
    jumperlessConfig.hardware_version.hardware_revision = saved_hardware_revision;
    jumperlessConfig.hardware_version.probe_version = saved_probe_version;

    saveConfig();
}


void updateConfigFromFile(const char* filename) {
    if (!FatFS.exists(filename)) {
        Serial.println("Config file not found");
        resetConfigToDefaults();
        
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
    
    while (file.available()) {
        int bytesRead = file.readBytesUntil('\n', line, sizeof(line)-1);
        line[bytesRead] = '\0';
        trim(line);
        
        if (line[0] == '\0' || line[0] == '#') continue;

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
        if (strcmp(section, "hardware_version") == 0) {
            if (strcmp(key, "generation") == 0) jumperlessConfig.hardware_version.generation = parseInt(value);
            else if (strcmp(key, "hardware_revision") == 0) jumperlessConfig.hardware_version.hardware_revision = parseInt(value);
            else if (strcmp(key, "probe_version") == 0) jumperlessConfig.hardware_version.probe_version = parseInt(value);
        } else if (strcmp(section, "dac_settings") == 0) {
            if (strcmp(key, "top_rail") == 0) jumperlessConfig.dac_settings.top_rail = parseFloat(value);
            else if (strcmp(key, "bottom_rail") == 0) jumperlessConfig.dac_settings.bottom_rail = parseFloat(value);
            else if (strcmp(key, "dac_0") == 0) jumperlessConfig.dac_settings.dac_0 = parseFloat(value);
            else if (strcmp(key, "dac_1") == 0) jumperlessConfig.dac_settings.dac_1 = parseFloat(value);
            else if (strcmp(key, "set_dacs_on_startup") == 0) jumperlessConfig.dac_settings.set_dacs_on_startup = parseBool(value);
            else if (strcmp(key, "set_rails_on_startup") == 0) jumperlessConfig.dac_settings.set_rails_on_startup = parseBool(value);
            else if (strcmp(key, "limit_max") == 0) jumperlessConfig.dac_settings.limit_max = parseFloat(value);
            else if (strcmp(key, "limit_min") == 0) jumperlessConfig.dac_settings.limit_min = parseFloat(value);
        } else if (strcmp(section, "debug_flags") == 0) {
            if (strcmp(key, "file_parsing") == 0) jumperlessConfig.debug_flags.file_parsing = parseBool(value);
            else if (strcmp(key, "net_manager") == 0) jumperlessConfig.debug_flags.net_manager = parseBool(value);
            else if (strcmp(key, "net_to_chip_connections") == 0) jumperlessConfig.debug_flags.net_to_chip_connections = parseBool(value);
            else if (strcmp(key, "net_to_chip_connections_alt") == 0) jumperlessConfig.debug_flags.net_to_chip_connections_alt = parseBool(value);
            else if (strcmp(key, "leds") == 0) jumperlessConfig.debug_flags.leds = parseBool(value);
        } else if (strcmp(section, "routing_settings") == 0) {
            if (strcmp(key, "stack_paths") == 0) {
                jumperlessConfig.routing_settings.stack_paths = parseInt(value);
                Serial.print("Updated stack_paths to: ");
                Serial.println(jumperlessConfig.routing_settings.stack_paths);
            }
            else if (strcmp(key, "stack_rails") == 0) jumperlessConfig.routing_settings.stack_rails = parseInt(value);
            else if (strcmp(key, "stack_dacs") == 0) jumperlessConfig.routing_settings.stack_dacs = parseInt(value);
            else if (strcmp(key, "rail_priority") == 0) jumperlessConfig.routing_settings.rail_priority = parseInt(value);
        } else if (strcmp(section, "calibration") == 0) {
            if (strcmp(key, "top_rail_zero") == 0) jumperlessConfig.calibration.top_rail_zero = parseInt(value);
            else if (strcmp(key, "top_rail_spread") == 0) jumperlessConfig.calibration.top_rail_spread = parseFloat(value);
            else if (strcmp(key, "bottom_rail_zero") == 0) jumperlessConfig.calibration.bottom_rail_zero = parseInt(value);
            else if (strcmp(key, "bottom_rail_spread") == 0) jumperlessConfig.calibration.bottom_rail_spread = parseFloat(value);
            else if (strcmp(key, "dac_0_zero") == 0) jumperlessConfig.calibration.dac_0_zero = parseInt(value);
            else if (strcmp(key, "dac_0_spread") == 0) jumperlessConfig.calibration.dac_0_spread = parseFloat(value);
            else if (strcmp(key, "dac_1_zero") == 0) jumperlessConfig.calibration.dac_1_zero = parseInt(value);
            else if (strcmp(key, "dac_1_spread") == 0) jumperlessConfig.calibration.dac_1_spread = parseFloat(value);
            else if (strcmp(key, "probe_max") == 0) jumperlessConfig.calibration.probe_max = parseInt(value);
            else if (strcmp(key, "probe_min") == 0) jumperlessConfig.calibration.probe_min = parseInt(value);
        } else if (strcmp(section, "logo_pad_settings") == 0) {
            if (strcmp(key, "top_guy") == 0) jumperlessConfig.logo_pad_settings.top_guy = parseInt(value);
            else if (strcmp(key, "bottom_guy") == 0) jumperlessConfig.logo_pad_settings.bottom_guy = parseInt(value);
            else if (strcmp(key, "building_pad_top") == 0) jumperlessConfig.logo_pad_settings.building_pad_top = parseInt(value);
            else if (strcmp(key, "building_pad_bottom") == 0) jumperlessConfig.logo_pad_settings.building_pad_bottom = parseInt(value);
        } else if (strcmp(section, "display_settings") == 0) {
            if (strcmp(key, "lines_wires") == 0) jumperlessConfig.display_settings.lines_wires = parseInt(value);
            else if (strcmp(key, "menu_brightness") == 0) jumperlessConfig.display_settings.menu_brightness = parseInt(value);
            else if (strcmp(key, "led_brightness") == 0) jumperlessConfig.display_settings.led_brightness = parseInt(value);
            else if (strcmp(key, "rail_brightness") == 0) jumperlessConfig.display_settings.rail_brightness = parseInt(value);
            else if (strcmp(key, "special_net_brightness") == 0) jumperlessConfig.display_settings.special_net_brightness = parseInt(value);
            else if (strcmp(key, "net_color_mode") == 0) jumperlessConfig.display_settings.net_color_mode = parseInt(value);
        } else if (strcmp(section, "gpio") == 0) {
            if (strcmp(key, "direction") == 0) parseCommaSeparatedInts(value, jumperlessConfig.gpio.direction, 8);
            else if (strcmp(key, "pulls") == 0) parseCommaSeparatedInts(value, jumperlessConfig.gpio.pulls, 8);
            else if (strcmp(key, "uart_tx_pin") == 0) jumperlessConfig.gpio.uart_tx_pin = parseInt(value);
            else if (strcmp(key, "uart_rx_pin") == 0) jumperlessConfig.gpio.uart_rx_pin = parseInt(value);
            else if (strcmp(key, "uart_tx_pull") == 0) jumperlessConfig.gpio.uart_tx_pull = parseInt(value);
            else if (strcmp(key, "uart_rx_pull") == 0) jumperlessConfig.gpio.uart_rx_pull = parseInt(value);
            else if (strcmp(key, "uart_tx_function") == 0) jumperlessConfig.gpio.uart_tx_function = parseInt(value);
            else if (strcmp(key, "uart_rx_function") == 0) jumperlessConfig.gpio.uart_rx_function = parseInt(value);
        } else if (strcmp(section, "serial") == 0) {
            if (strcmp(key, "serial_1.function") == 0) jumperlessConfig.serial.serial_1.function = parseInt(value);
            else if (strcmp(key, "serial_1.baud_rate") == 0) jumperlessConfig.serial.serial_1.baud_rate = parseInt(value);
            else if (strcmp(key, "serial_2.function") == 0) jumperlessConfig.serial.serial_2.function = parseInt(value);
            else if (strcmp(key, "serial_2.baud_rate") == 0) jumperlessConfig.serial.serial_2.baud_rate = parseInt(value);
        }
    }
    file.close();

    
    readSettingsFromConfig();
    //initChipStatus();
}

void saveConfigToFile(const char* filename) {
    core1busy = true;
    if (FatFS.exists(filename)) {
        FatFS.remove(filename);
    }
    
    File file = FatFS.open(filename, "w");
    if (!file) {
        Serial.println("Failed to create config file");
        return;
    }

    // Write hardware version section
    file.println("[hardware_version]");
    file.print("generation="); file.print(jumperlessConfig.hardware_version.generation); file.println(";");
    file.print("hardware_revision="); file.print(jumperlessConfig.hardware_version.hardware_revision); file.println(";");
    file.print("probe_version="); file.print(jumperlessConfig.hardware_version.probe_version); file.println(";");
    file.println();

    // Write DAC settings section
    file.println("[dac_settings]");
    file.print("top_rail="); file.print(jumperlessConfig.dac_settings.top_rail); file.println(";");
    file.print("bottom_rail="); file.print(jumperlessConfig.dac_settings.bottom_rail); file.println(";");
    file.print("dac_0="); file.print(jumperlessConfig.dac_settings.dac_0); file.println(";");
    file.print("dac_1="); file.print(jumperlessConfig.dac_settings.dac_1); file.println(";");
    file.print("set_dacs_on_startup="); file.print(jumperlessConfig.dac_settings.set_dacs_on_startup ? "true" : "false"); file.println(";");
    file.print("set_rails_on_startup="); file.print(jumperlessConfig.dac_settings.set_rails_on_startup ? "true" : "false"); file.println(";");
    file.print("limit_max="); file.print(jumperlessConfig.dac_settings.limit_max); file.println(";");
    file.print("limit_min="); file.print(jumperlessConfig.dac_settings.limit_min); file.println(";");
    file.println();

    // Write debug flags section
    file.println("[debug_flags]");
    file.print("file_parsing="); file.print(jumperlessConfig.debug_flags.file_parsing ? "true" : "false"); file.println(";");
    file.print("net_manager="); file.print(jumperlessConfig.debug_flags.net_manager ? "true" : "false"); file.println(";");
    file.print("net_to_chip_connections="); file.print(jumperlessConfig.debug_flags.net_to_chip_connections ? "true" : "false"); file.println(";");
    file.print("net_to_chip_connections_alt="); file.print(jumperlessConfig.debug_flags.net_to_chip_connections_alt ? "true" : "false"); file.println(";");
    file.print("leds="); file.print(jumperlessConfig.debug_flags.leds ? "true" : "false"); file.println(";");
    file.println();

    // Write routing settings section
    file.println("[routing_settings]");
    file.print("stack_paths="); file.print(jumperlessConfig.routing_settings.stack_paths); file.println(";");
    file.print("stack_rails="); file.print(jumperlessConfig.routing_settings.stack_rails); file.println(";");
    file.print("stack_dacs="); file.print(jumperlessConfig.routing_settings.stack_dacs); file.println(";");
    file.print("rail_priority="); file.print(jumperlessConfig.routing_settings.rail_priority); file.println(";");
    file.println();

    // Write calibration section
    file.println("[calibration]");
    file.print("top_rail_zero="); file.print(jumperlessConfig.calibration.top_rail_zero); file.println(";");
    file.print("top_rail_spread="); file.print(jumperlessConfig.calibration.top_rail_spread); file.println(";");
    file.print("bottom_rail_zero="); file.print(jumperlessConfig.calibration.bottom_rail_zero); file.println(";");
    file.print("bottom_rail_spread="); file.print(jumperlessConfig.calibration.bottom_rail_spread); file.println(";");
    file.print("dac_0_zero="); file.print(jumperlessConfig.calibration.dac_0_zero); file.println(";");
    file.print("dac_0_spread="); file.print(jumperlessConfig.calibration.dac_0_spread); file.println(";");
    file.print("dac_1_zero="); file.print(jumperlessConfig.calibration.dac_1_zero); file.println(";");
    file.print("dac_1_spread="); file.print(jumperlessConfig.calibration.dac_1_spread); file.println(";");
    file.print("probe_max="); file.print(jumperlessConfig.calibration.probe_max); file.println(";");
    file.print("probe_min="); file.print(jumperlessConfig.calibration.probe_min); file.println(";");
    file.println();

    // Write logo pad settings section
    file.println("[logo_pad_settings]");
    file.print("top_guy="); file.print(jumperlessConfig.logo_pad_settings.top_guy); file.println(";");
    file.print("bottom_guy="); file.print(jumperlessConfig.logo_pad_settings.bottom_guy); file.println(";");
    file.print("building_pad_top="); file.print(jumperlessConfig.logo_pad_settings.building_pad_top); file.println(";");
    file.print("building_pad_bottom="); file.print(jumperlessConfig.logo_pad_settings.building_pad_bottom); file.println(";");
    file.println();

    // Write display settings section
    file.println("[display_settings]");
    file.print("lines_wires="); file.print(jumperlessConfig.display_settings.lines_wires); file.println(";");
    file.print("menu_brightness="); file.print(jumperlessConfig.display_settings.menu_brightness); file.println(";");
    file.print("led_brightness="); file.print(jumperlessConfig.display_settings.led_brightness); file.println(";");
    file.print("rail_brightness="); file.print(jumperlessConfig.display_settings.rail_brightness); file.println(";");
    file.print("special_net_brightness="); file.print(jumperlessConfig.display_settings.special_net_brightness); file.println(";");
    file.print("net_color_mode="); file.print(jumperlessConfig.display_settings.net_color_mode); file.println(";");
    file.println();

    // Write GPIO section
    file.println("[gpio]");
    file.print("direction=");
    for (int i = 0; i < 8; i++) {
        if (i > 0) file.print(",");
        file.print(jumperlessConfig.gpio.direction[i]);
    }
    file.println(";");
    file.print("pulls=");
    for (int i = 0; i < 8; i++) {
        if (i > 0) file.print(",");
        file.print(jumperlessConfig.gpio.pulls[i]);
    }
    file.println(";");
    file.print("uart_tx_pin="); file.print(jumperlessConfig.gpio.uart_tx_pin); file.println(";");
    file.print("uart_rx_pin="); file.print(jumperlessConfig.gpio.uart_rx_pin); file.println(";");
    file.print("uart_tx_pull="); file.print(jumperlessConfig.gpio.uart_tx_pull); file.println(";");
    file.print("uart_rx_pull="); file.print(jumperlessConfig.gpio.uart_rx_pull); file.println(";");
    file.print("uart_tx_function="); file.print(jumperlessConfig.gpio.uart_tx_function); file.println(";");
    file.print("uart_rx_function="); file.print(jumperlessConfig.gpio.uart_rx_function); file.println(";");
    file.println();

    // Write serial section
    file.println("[serial]");
    file.print("serial_1.function="); file.print(jumperlessConfig.serial.serial_1.function); file.println(";");
    file.print("serial_1.baud_rate="); file.print(jumperlessConfig.serial.serial_1.baud_rate); file.println(";");
    file.print("serial_2.function="); file.print(jumperlessConfig.serial.serial_2.function); file.println(";");
    file.print("serial_2.baud_rate="); file.print(jumperlessConfig.serial.serial_2.baud_rate); file.println(";");

    file.close();
    core1busy = false;
}

void saveConfig(void) {
    int hwRevision = jumperlessConfig.hardware_version.hardware_revision;
    saveConfigToFile("/config.txt");
    
    readSettingsFromConfig();
    ///initChipStatus();
    //if (jumperlessConfig.hardware_version.hardware_revision != hwRevision ) {
        // leds.clear();
        // leds.end();
        // leds.begin();
       
        // showLEDsCore2 = -1;
   // }
}

void loadConfig(void) {
    updateConfigFromFile("/config.txt");
    initChipStatus();


}

int parseSectionName(const char* sectionName) {
    if (strcmp(sectionName, "hardware_version") == 0) return 0;
    else if (strcmp(sectionName, "dac_settings") == 0) return 1;
    else if (strcmp(sectionName, "debug_flags") == 0) return 2;
    else if (strcmp(sectionName, "routing_settings") == 0) return 3;
    else if (strcmp(sectionName, "calibration") == 0) return 4;
    else if (strcmp(sectionName, "logo_pad_settings") == 0) return 5;
    else if (strcmp(sectionName, "display_settings") == 0) return 6;
    else if (strcmp(sectionName, "gpio") == 0) return 7;
    else if (strcmp(sectionName, "serial") == 0) return 8;
    return -1;
}

void printConfigSectionToSerial(int section) {
    // If section is -1, try to parse input
    if (section == -1) {
        Serial.println("Jumperless Config:\n\r");
    }

    // Print hardware version section
    if (section == -1 || section == 0) {
        Serial.println("\n[hardware_version]");
        Serial.print("generation="); Serial.print(jumperlessConfig.hardware_version.generation); Serial.println(";");
        Serial.print("hardware_revision="); Serial.print(jumperlessConfig.hardware_version.hardware_revision); Serial.println(";");
        Serial.print("probe_version="); Serial.print(jumperlessConfig.hardware_version.probe_version); Serial.println(";");
    }

    // Print DAC settings section
    if (section == -1 || section == 1) {
        Serial.println("\n[dac_settings]");
        Serial.print("top_rail="); Serial.print(jumperlessConfig.dac_settings.top_rail); Serial.println(";");
        Serial.print("bottom_rail="); Serial.print(jumperlessConfig.dac_settings.bottom_rail); Serial.println(";");
        Serial.print("dac_0="); Serial.print(jumperlessConfig.dac_settings.dac_0); Serial.println(";");
        Serial.print("dac_1="); Serial.print(jumperlessConfig.dac_settings.dac_1); Serial.println(";");
        Serial.print("set_dacs_on_startup="); Serial.print(jumperlessConfig.dac_settings.set_dacs_on_startup ? "true" : "false"); Serial.println(";");
        Serial.print("set_rails_on_startup="); Serial.print(jumperlessConfig.dac_settings.set_rails_on_startup ? "true" : "false"); Serial.println(";");
        Serial.print("limit_max="); Serial.print(jumperlessConfig.dac_settings.limit_max); Serial.println(";");
        Serial.print("limit_min="); Serial.print(jumperlessConfig.dac_settings.limit_min); Serial.println(";");
    }

    // Print debug flags section
    if (section == -1 || section == 2) {
        Serial.println("\n[debug_flags]");
        Serial.print("file_parsing="); Serial.print(jumperlessConfig.debug_flags.file_parsing ? "true" : "false"); Serial.println(";");
        Serial.print("net_manager="); Serial.print(jumperlessConfig.debug_flags.net_manager ? "true" : "false"); Serial.println(";");
        Serial.print("net_to_chip_connections="); Serial.print(jumperlessConfig.debug_flags.net_to_chip_connections ? "true" : "false"); Serial.println(";");
        Serial.print("net_to_chip_connections_alt="); Serial.print(jumperlessConfig.debug_flags.net_to_chip_connections_alt ? "true" : "false"); Serial.println(";");
        Serial.print("leds="); Serial.print(jumperlessConfig.debug_flags.leds ? "true" : "false"); Serial.println(";");
    }

    // Print routing settings section
    if (section == -1 || section == 3) {
        Serial.println("\n[routing_settings]");
        Serial.print("stack_paths="); Serial.print(jumperlessConfig.routing_settings.stack_paths); Serial.println(";");
        Serial.print("stack_rails="); Serial.print(jumperlessConfig.routing_settings.stack_rails); Serial.println(";");
        Serial.print("stack_dacs="); Serial.print(jumperlessConfig.routing_settings.stack_dacs); Serial.println(";");
        Serial.print("rail_priority="); Serial.print(jumperlessConfig.routing_settings.rail_priority); Serial.println(";");
    }

    // Print calibration section
    if (section == -1 || section == 4) {
        Serial.println("\n[calibration]");
        Serial.print("top_rail_zero="); Serial.print(jumperlessConfig.calibration.top_rail_zero); Serial.println(";");
        Serial.print("top_rail_spread="); Serial.print(jumperlessConfig.calibration.top_rail_spread); Serial.println(";");
        Serial.print("bottom_rail_zero="); Serial.print(jumperlessConfig.calibration.bottom_rail_zero); Serial.println(";");
        Serial.print("bottom_rail_spread="); Serial.print(jumperlessConfig.calibration.bottom_rail_spread); Serial.println(";");
        Serial.print("dac_0_zero="); Serial.print(jumperlessConfig.calibration.dac_0_zero); Serial.println(";");
        Serial.print("dac_0_spread="); Serial.print(jumperlessConfig.calibration.dac_0_spread); Serial.println(";");
        Serial.print("dac_1_zero="); Serial.print(jumperlessConfig.calibration.dac_1_zero); Serial.println(";");
        Serial.print("dac_1_spread="); Serial.print(jumperlessConfig.calibration.dac_1_spread); Serial.println(";");
        Serial.print("probe_max="); Serial.print(jumperlessConfig.calibration.probe_max); Serial.println(";");
        Serial.print("probe_min="); Serial.print(jumperlessConfig.calibration.probe_min); Serial.println(";");
    }

    // Print logo pad settings section
    if (section == -1 || section == 5) {
        Serial.println("\n[logo_pad_settings]");
        Serial.print("top_guy="); Serial.print(jumperlessConfig.logo_pad_settings.top_guy); Serial.println(";");
        Serial.print("bottom_guy="); Serial.print(jumperlessConfig.logo_pad_settings.bottom_guy); Serial.println(";");
        Serial.print("building_pad_top="); Serial.print(jumperlessConfig.logo_pad_settings.building_pad_top); Serial.println(";");
        Serial.print("building_pad_bottom="); Serial.print(jumperlessConfig.logo_pad_settings.building_pad_bottom); Serial.println(";");
    }

    // Print display settings section
    if (section == -1 || section == 6) {
        Serial.println("\n[display_settings]");
        Serial.print("lines_wires="); Serial.print(jumperlessConfig.display_settings.lines_wires); Serial.println(";");
        Serial.print("menu_brightness="); Serial.print(jumperlessConfig.display_settings.menu_brightness); Serial.println(";");
        Serial.print("led_brightness="); Serial.print(jumperlessConfig.display_settings.led_brightness); Serial.println(";");
        Serial.print("rail_brightness="); Serial.print(jumperlessConfig.display_settings.rail_brightness); Serial.println(";");
        Serial.print("special_net_brightness="); Serial.print(jumperlessConfig.display_settings.special_net_brightness); Serial.println(";");
        Serial.print("net_color_mode="); Serial.print(jumperlessConfig.display_settings.net_color_mode); Serial.println(";");
    }

    // Print GPIO section
    if (section == -1 || section == 7) {
        Serial.println("\n[gpio]");
        Serial.print("direction=");
        for (int i = 0; i < 8; i++) {
            if (i > 0) Serial.print(",");
            Serial.print(jumperlessConfig.gpio.direction[i]);
        }
        Serial.println(";");
        Serial.print("pulls=");
        for (int i = 0; i < 8; i++) {
            if (i > 0) Serial.print(",");
            Serial.print(jumperlessConfig.gpio.pulls[i]);
        }
        Serial.println(";");
        Serial.print("uart_tx_pin="); Serial.print(jumperlessConfig.gpio.uart_tx_pin); Serial.println(";");
        Serial.print("uart_rx_pin="); Serial.print(jumperlessConfig.gpio.uart_rx_pin); Serial.println(";");
        Serial.print("uart_tx_pull="); Serial.print(jumperlessConfig.gpio.uart_tx_pull); Serial.println(";");
        Serial.print("uart_rx_pull="); Serial.print(jumperlessConfig.gpio.uart_rx_pull); Serial.println(";");
        Serial.print("uart_tx_function="); Serial.print(jumperlessConfig.gpio.uart_tx_function); Serial.println(";");
        Serial.print("uart_rx_function="); Serial.print(jumperlessConfig.gpio.uart_rx_function); Serial.println(";");
    }

    // Print serial section
    if (section == -1 || section == 8) {
        Serial.println("\n[serial]");
        Serial.print("serial_1.function="); Serial.print(jumperlessConfig.serial.serial_1.function); Serial.println(";");
        Serial.print("serial_1.baud_rate="); Serial.print(jumperlessConfig.serial.serial_1.baud_rate); Serial.println(";");
        Serial.print("serial_2.function="); Serial.print(jumperlessConfig.serial.serial_2.function); Serial.println(";");
        Serial.print("serial_2.baud_rate="); Serial.print(jumperlessConfig.serial.serial_2.baud_rate); Serial.println(";");
    }

    if (section == -1) {
        Serial.println("\nEND\n\n\r");
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
    Serial.print("Changed [");
    Serial.print(section);
    Serial.print("] ");
    Serial.print(key);
    Serial.print(" from ");
    Serial.print(oldValue);
    Serial.print(" to ");
    Serial.println(newValue);
}

void printConfigToSerial() {
    char line[128] = {0};
    int lineIndex = 0;
    unsigned long lastCharTime = millis();
    const unsigned long timeout = 100; // 100ms timeout

    // Wait for input with timeout
    while (true) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (lineIndex < sizeof(line) - 1) {
                line[lineIndex++] = c;
                line[lineIndex] = '\0';
                lastCharTime = millis();
               // Serial.print(c);
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
                        printConfigSectionToSerial(section);
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
            ///if (lineIndex > 0) {
                // No section found, print full config
                printConfigSectionToSerial(-1);
                Serial.println("\n\n");
                //printConfigStructToSerial();
            //}
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
    
    unsigned long lastCharTime = millis();
    const unsigned long timeout = 10;

    while (Serial.available() == 0) {
       // delayMicroseconds(10);
    }
    int timedOut = 0;
    while (true) {
        if (Serial.available() > 0) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
               // Serial.println("New line");
            }
            
            lastCharTime = millis();
            
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
                    Serial.println("\t~ = show current config");
                    Serial.println("\t~[section] = show specific section (e.g. ~[routing_settings])");
                    Serial.println("\t` = enter config settings");
                    Serial.println("\t? = show this help");
                    Serial.println("    reset = reset to defaults");

                    Serial.println("\n    config setting formats (prefix with ` to paste from main menu)\n\r");    
                    Serial.println("[routing_settings]stack_paths = 1;");
                    Serial.println("\n\r\tor you can use dot notation\n\r");
                    Serial.println("config.routing_settings.stack_paths = 1;");
                    Serial.println("\n\r\tor paste a whole section\n\r");
                    Serial.println("[dac_settings]");
                    Serial.println("top_rail = 5.0;");
                    Serial.println("bottom_rail = 3.3;");
                    Serial.println("dac_0 = -2.0;");
                    Serial.println("dac_1 = 3.33;");
                    Serial.println("\n\r");
                    delayMicroseconds(3000);
                    // Clear the line buffer after showing help
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                }
                
                // Check for reset commands
                if (strcmp(line, "default") == 0 || strcmp(line, "reset") == 0) {
                    Serial.println("\nResetting all settings to defaults (preserving hardware version)...");
                    resetConfigToDefaults();
                    saveConfigToFile("/config.txt");
                    Serial.println("Done. Settings have been reset to defaults");
                    
                    // Clear the line buffer after reset
                    memset(line, 0, sizeof(line));
                    lineIndex = 0;
                    continue;
                }
            }
            
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

        if (timedOut > timeout) {
            break;
        }
    }

    while (Serial.available() > 0) {
        Serial.read();
        delayMicroseconds(100);
    }
}

void updateConfigValue(const char* section, const char* key, const char* value) {
    char oldValue[64] = {0};
    
    // Get old value
    if (strcmp(section, "hardware_version") == 0) {
        if (strcmp(key, "generation") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware_version.generation);
        else if (strcmp(key, "hardware_revision") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware_version.hardware_revision);
        else if (strcmp(key, "probe_version") == 0) sprintf(oldValue, "%d", jumperlessConfig.hardware_version.probe_version);
    }
    else if (strcmp(section, "dac_settings") == 0) {
        if (strcmp(key, "top_rail") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.top_rail);
        else if (strcmp(key, "bottom_rail") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.bottom_rail);
        else if (strcmp(key, "dac_0") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.dac_0);
        else if (strcmp(key, "dac_1") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.dac_1);
        else if (strcmp(key, "set_dacs_on_startup") == 0) sprintf(oldValue, "%s", jumperlessConfig.dac_settings.set_dacs_on_startup ? "true" : "false");
        else if (strcmp(key, "set_rails_on_startup") == 0) sprintf(oldValue, "%s", jumperlessConfig.dac_settings.set_rails_on_startup ? "true" : "false");
        else if (strcmp(key, "limit_max") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.limit_max);
        else if (strcmp(key, "limit_min") == 0) sprintf(oldValue, "%.2f", jumperlessConfig.dac_settings.limit_min);
    }
    else if (strcmp(section, "debug_flags") == 0) {
        if (strcmp(key, "file_parsing") == 0) sprintf(oldValue, "%s", jumperlessConfig.debug_flags.file_parsing ? "true" : "false");
        else if (strcmp(key, "net_manager") == 0) sprintf(oldValue, "%s", jumperlessConfig.debug_flags.net_manager ? "true" : "false");
        else if (strcmp(key, "net_to_chip_connections") == 0) sprintf(oldValue, "%s", jumperlessConfig.debug_flags.net_to_chip_connections ? "true" : "false");
        else if (strcmp(key, "net_to_chip_connections_alt") == 0) sprintf(oldValue, "%s", jumperlessConfig.debug_flags.net_to_chip_connections_alt ? "true" : "false");
        else if (strcmp(key, "leds") == 0) sprintf(oldValue, "%s", jumperlessConfig.debug_flags.leds ? "true" : "false");
    }
    else if (strcmp(section, "routing_settings") == 0) {
        if (strcmp(key, "stack_paths") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing_settings.stack_paths);
        else if (strcmp(key, "stack_rails") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing_settings.stack_rails);
        else if (strcmp(key, "stack_dacs") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing_settings.stack_dacs);
        else if (strcmp(key, "rail_priority") == 0) sprintf(oldValue, "%d", jumperlessConfig.routing_settings.rail_priority);
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
        else if (strcmp(key, "probe_max") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.probe_max);
        else if (strcmp(key, "probe_min") == 0) sprintf(oldValue, "%d", jumperlessConfig.calibration.probe_min);
    }
    else if (strcmp(section, "logo_pad_settings") == 0) {
        if (strcmp(key, "top_guy") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pad_settings.top_guy);
        else if (strcmp(key, "bottom_guy") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pad_settings.bottom_guy);
        else if (strcmp(key, "building_pad_top") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pad_settings.building_pad_top);
        else if (strcmp(key, "building_pad_bottom") == 0) sprintf(oldValue, "%d", jumperlessConfig.logo_pad_settings.building_pad_bottom);
    }
    else if (strcmp(section, "display_settings") == 0) {
        if (strcmp(key, "lines_wires") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.lines_wires);
        else if (strcmp(key, "menu_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.menu_brightness);
        else if (strcmp(key, "led_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.led_brightness);
        else if (strcmp(key, "rail_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.rail_brightness);
        else if (strcmp(key, "special_net_brightness") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.special_net_brightness);
        else if (strcmp(key, "net_color_mode") == 0) sprintf(oldValue, "%d", jumperlessConfig.display_settings.net_color_mode);
    }
    else if (strcmp(section, "gpio") == 0) {
        if (strcmp(key, "direction") == 0) {
            char temp[128] = {0};
            for (int i = 0; i < 8; i++) {
                if (i > 0) strcat(temp, ",");
                char num[4];
                sprintf(num, "%d", jumperlessConfig.gpio.direction[i]);
                strcat(temp, num);
            }
            strcpy(oldValue, temp);
        }
        else if (strcmp(key, "pulls") == 0) {
            char temp[128] = {0};
            for (int i = 0; i < 8; i++) {
                if (i > 0) strcat(temp, ",");
                char num[4];
                sprintf(num, "%d", jumperlessConfig.gpio.pulls[i]);
                strcat(temp, num);
            }
            strcpy(oldValue, temp);
        }
        else if (strcmp(key, "uart_tx_pin") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_tx_pin);
        else if (strcmp(key, "uart_rx_pin") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_rx_pin);
        else if (strcmp(key, "uart_tx_pull") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_tx_pull);
        else if (strcmp(key, "uart_rx_pull") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_rx_pull);
        else if (strcmp(key, "uart_tx_function") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_tx_function);
        else if (strcmp(key, "uart_rx_function") == 0) sprintf(oldValue, "%d", jumperlessConfig.gpio.uart_rx_function);
    }
    else if (strcmp(section, "serial") == 0) {
        if (strcmp(key, "serial_1.function") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial.serial_1.function);
        else if (strcmp(key, "serial_1.baud_rate") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial.serial_1.baud_rate);
        else if (strcmp(key, "serial_2.function") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial.serial_2.function);
        else if (strcmp(key, "serial_2.baud_rate") == 0) sprintf(oldValue, "%d", jumperlessConfig.serial.serial_2.baud_rate);
    }

    // Update the config structure
    if (strcmp(section, "hardware_version") == 0) {
        if (strcmp(key, "generation") == 0) jumperlessConfig.hardware_version.generation = parseInt(value);
        else if (strcmp(key, "hardware_revision") == 0) jumperlessConfig.hardware_version.hardware_revision = parseInt(value);
        else if (strcmp(key, "probe_version") == 0) jumperlessConfig.hardware_version.probe_version = parseInt(value);
    }
    else if (strcmp(section, "dac_settings") == 0) {
        if (strcmp(key, "top_rail") == 0) jumperlessConfig.dac_settings.top_rail = parseFloat(value);
        else if (strcmp(key, "bottom_rail") == 0) jumperlessConfig.dac_settings.bottom_rail = parseFloat(value);
        else if (strcmp(key, "dac_0") == 0) jumperlessConfig.dac_settings.dac_0 = parseFloat(value);
        else if (strcmp(key, "dac_1") == 0) jumperlessConfig.dac_settings.dac_1 = parseFloat(value);
        else if (strcmp(key, "set_dacs_on_startup") == 0) jumperlessConfig.dac_settings.set_dacs_on_startup = parseBool(value);
        else if (strcmp(key, "set_rails_on_startup") == 0) jumperlessConfig.dac_settings.set_rails_on_startup = parseBool(value);
        else if (strcmp(key, "limit_max") == 0) jumperlessConfig.dac_settings.limit_max = parseFloat(value);
        else if (strcmp(key, "limit_min") == 0) jumperlessConfig.dac_settings.limit_min = parseFloat(value);
    }
    else if (strcmp(section, "debug_flags") == 0) {
        if (strcmp(key, "file_parsing") == 0) jumperlessConfig.debug_flags.file_parsing = parseBool(value);
        else if (strcmp(key, "net_manager") == 0) jumperlessConfig.debug_flags.net_manager = parseBool(value);
        else if (strcmp(key, "net_to_chip_connections") == 0) jumperlessConfig.debug_flags.net_to_chip_connections = parseBool(value);
        else if (strcmp(key, "net_to_chip_connections_alt") == 0) jumperlessConfig.debug_flags.net_to_chip_connections_alt = parseBool(value);
        else if (strcmp(key, "leds") == 0) jumperlessConfig.debug_flags.leds = parseBool(value);
    }
    else if (strcmp(section, "routing_settings") == 0) {
        if (strcmp(key, "stack_paths") == 0) jumperlessConfig.routing_settings.stack_paths = parseInt(value);
        else if (strcmp(key, "stack_rails") == 0) jumperlessConfig.routing_settings.stack_rails = parseInt(value);
        else if (strcmp(key, "stack_dacs") == 0) jumperlessConfig.routing_settings.stack_dacs = parseInt(value);
        else if (strcmp(key, "rail_priority") == 0) jumperlessConfig.routing_settings.rail_priority = parseInt(value);
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
        else if (strcmp(key, "probe_max") == 0) jumperlessConfig.calibration.probe_max = parseInt(value);
        else if (strcmp(key, "probe_min") == 0) jumperlessConfig.calibration.probe_min = parseInt(value);
    }
    else if (strcmp(section, "logo_pad_settings") == 0) {
        if (strcmp(key, "top_guy") == 0) jumperlessConfig.logo_pad_settings.top_guy = parseInt(value);
        else if (strcmp(key, "bottom_guy") == 0) jumperlessConfig.logo_pad_settings.bottom_guy = parseInt(value);
        else if (strcmp(key, "building_pad_top") == 0) jumperlessConfig.logo_pad_settings.building_pad_top = parseInt(value);
        else if (strcmp(key, "building_pad_bottom") == 0) jumperlessConfig.logo_pad_settings.building_pad_bottom = parseInt(value);
    }
    else if (strcmp(section, "display_settings") == 0) {
        if (strcmp(key, "lines_wires") == 0) jumperlessConfig.display_settings.lines_wires = parseInt(value);
        else if (strcmp(key, "menu_brightness") == 0) jumperlessConfig.display_settings.menu_brightness = parseInt(value);
        else if (strcmp(key, "led_brightness") == 0) jumperlessConfig.display_settings.led_brightness = parseInt(value);
        else if (strcmp(key, "rail_brightness") == 0) jumperlessConfig.display_settings.rail_brightness = parseInt(value);
        else if (strcmp(key, "special_net_brightness") == 0) jumperlessConfig.display_settings.special_net_brightness = parseInt(value);
        else if (strcmp(key, "net_color_mode") == 0) jumperlessConfig.display_settings.net_color_mode = parseInt(value);
    }
    else if (strcmp(section, "gpio") == 0) {
        if (strcmp(key, "direction") == 0) {
            parseCommaSeparatedInts(value, jumperlessConfig.gpio.direction, 8);
        }
        else if (strcmp(key, "pulls") == 0) {
            parseCommaSeparatedInts(value, jumperlessConfig.gpio.pulls, 8);
        }
        else if (strcmp(key, "uart_tx_pin") == 0) jumperlessConfig.gpio.uart_tx_pin = parseInt(value);
        else if (strcmp(key, "uart_rx_pin") == 0) jumperlessConfig.gpio.uart_rx_pin = parseInt(value);
        else if (strcmp(key, "uart_tx_pull") == 0) jumperlessConfig.gpio.uart_tx_pull = parseInt(value);
        else if (strcmp(key, "uart_rx_pull") == 0) jumperlessConfig.gpio.uart_rx_pull = parseInt(value);
        else if (strcmp(key, "uart_tx_function") == 0) jumperlessConfig.gpio.uart_tx_function = parseInt(value);
        else if (strcmp(key, "uart_rx_function") == 0) jumperlessConfig.gpio.uart_rx_function = parseInt(value);
    }
    else if (strcmp(section, "serial") == 0) {
        if (strcmp(key, "serial_1.function") == 0) jumperlessConfig.serial.serial_1.function = parseInt(value);
        else if (strcmp(key, "serial_1.baud_rate") == 0) jumperlessConfig.serial.serial_1.baud_rate = parseInt(value);
        else if (strcmp(key, "serial_2.function") == 0) jumperlessConfig.serial.serial_2.function = parseInt(value);
        else if (strcmp(key, "serial_2.baud_rate") == 0) jumperlessConfig.serial.serial_2.baud_rate = parseInt(value);
    }

    // Save the updated config to the permanent file
    saveConfigToFile("/config.txt");
    
    // Print the change
    printSettingChange(section, key, oldValue, value);
}

void printConfigStructToSerial() {
  Serial.println("\nJumperless Config (RAM):");
  Serial.println("-------------------");

  // Hardware version
  Serial.println("[hardware_version]");
  Serial.print("generation = ");
  Serial.println(jumperlessConfig.hardware_version.generation);
  Serial.print("hardware_revision = ");
  Serial.println(jumperlessConfig.hardware_version.hardware_revision);
  Serial.print("probe_version = ");
  Serial.println(jumperlessConfig.hardware_version.probe_version);
  Serial.println();

  // Debug flags
  Serial.println("[debug_flags]");
  Serial.print("file_parsing = ");
  Serial.println(jumperlessConfig.debug_flags.file_parsing);
  Serial.print("net_manager = ");
  Serial.println(jumperlessConfig.debug_flags.net_manager);
  Serial.print("net_to_chip_connections = ");
  Serial.println(jumperlessConfig.debug_flags.net_to_chip_connections);
  Serial.print("net_to_chip_connections_alt = ");
  Serial.println(jumperlessConfig.debug_flags.net_to_chip_connections_alt);
  Serial.print("leds = ");
  Serial.println(jumperlessConfig.debug_flags.leds);
  Serial.println();

  // Display settings
  Serial.println("[display_settings]");
  Serial.print("led_brightness = ");
  Serial.println(jumperlessConfig.display_settings.led_brightness);
  Serial.print("rail_brightness = ");
  Serial.println(jumperlessConfig.display_settings.rail_brightness);
  Serial.print("special_net_brightness = ");
  Serial.println(jumperlessConfig.display_settings.special_net_brightness);
  Serial.print("menu_brightness = ");
  Serial.println(jumperlessConfig.display_settings.menu_brightness);
  Serial.print("net_color_mode = ");
  Serial.println(jumperlessConfig.display_settings.net_color_mode);
  Serial.println();

  // Routing settings
  Serial.println("[routing_settings]");
  Serial.print("stack_paths = ");
  Serial.println(jumperlessConfig.routing_settings.stack_paths);
  Serial.print("stack_rails = ");
  Serial.println(jumperlessConfig.routing_settings.stack_rails);
  Serial.print("stack_dacs = ");
  Serial.println(jumperlessConfig.routing_settings.stack_dacs);
  Serial.print("rail_priority = ");
  Serial.println(jumperlessConfig.routing_settings.rail_priority);
  Serial.println();

  // Calibration
  Serial.println("[calibration]");
  Serial.print("dac_0_spread = ");
  Serial.println(jumperlessConfig.calibration.dac_0_spread);
  Serial.print("dac_1_spread = ");
  Serial.println(jumperlessConfig.calibration.dac_1_spread);
  Serial.print("top_rail_spread = ");
  Serial.println(jumperlessConfig.calibration.top_rail_spread);
  Serial.print("bottom_rail_spread = ");
  Serial.println(jumperlessConfig.calibration.bottom_rail_spread);
  Serial.print("dac_0_zero = ");
  Serial.println(jumperlessConfig.calibration.dac_0_zero);
  Serial.print("dac_1_zero = ");
  Serial.println(jumperlessConfig.calibration.dac_1_zero);
  Serial.print("top_rail_zero = ");
  Serial.println(jumperlessConfig.calibration.top_rail_zero);
  Serial.print("bottom_rail_zero = ");
  Serial.println(jumperlessConfig.calibration.bottom_rail_zero);
  Serial.println();

  // DAC settings
  Serial.println("[dac_settings]");
  Serial.print("top_rail = ");
  Serial.println(jumperlessConfig.dac_settings.top_rail);
  Serial.print("bottom_rail = ");
  Serial.println(jumperlessConfig.dac_settings.bottom_rail);
  Serial.print("dac_0 = ");
  Serial.println(jumperlessConfig.dac_settings.dac_0);
  Serial.print("dac_1 = ");
  Serial.println(jumperlessConfig.dac_settings.dac_1);
  Serial.println();

  // GPIO settings
  Serial.println("[gpio]");
  Serial.print("direction = [");
  for (int i = 0; i < 8; i++) {
    Serial.print(jumperlessConfig.gpio.direction[i]);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("]");
  Serial.print("pulls = [");
  for (int i = 0; i < 8; i++) {
    Serial.print(jumperlessConfig.gpio.pulls[i]);
    if (i < 7) Serial.print(", ");
  }
  Serial.println("]");
  Serial.println();

  Serial.println("END");
} 