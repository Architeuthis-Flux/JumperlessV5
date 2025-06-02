#pragma once

#include "config.h"
#include <FatFS.h>
#include "oled.h"

extern bool configChanged;
// Global configuration instance
extern struct config jumperlessConfig;


struct StringIntEntry {
    const char* name;
    int value;
};

// Core configuration functions
void loadConfig(void);
void saveConfig(void);
void resetConfigToDefaults(int clearCalibration = 0, int clearHardware = 0);

// File operations
void updateConfigFromFile(const char* filename);
void saveConfigToFile(const char* filename);

// Serial operations
void printConfigSectionToSerial(int section, bool showNames = true, bool pasteable = true);
void readConfigFromSerial(void);
void printConfigToSerial(bool showNames = true);
void printConfigStructToSerial(bool showNames = true);
void printConfigHelp(void);
bool parseSetting(const char* line, char* section, char* key, char* value);
void parseCommaSeparatedInts(const char* str, int* array, int maxValues);
bool parseBool(const char* str);
float parseFloat(const char* str);
int parseInt(const char* str);
int parseFont(const char* str);
void trim(char* str);
void toLower(char* str); 
void updateConfigValue(const char* section, const char* key, const char* value);
int parseTrueFalse(const char* value);
void printArbitraryFunctionTable(void);
// Fast config parsing function for tight loops - returns quickly if invalid
bool fastParseAndUpdateConfig(const char* configString);
// Template function to get string from a value and a table (auto-deduces table size)
template <size_t N>
const char* getStringFromTable(int value, const StringIntEntry (&table)[N]);



// List of all string options for parseArbitraryFunction
extern const char* arbitraryFunctionStrings[];

// Generic struct for mapping string to int value

// struct font fontList[] = {
//   { &Eurostile_Next_LT_Com_Light_Extended6pt7b, "Eurostl", "Eurostile" },
//   { &BerkeleyMono6pt7b, "Berkley", "Berkeley" },
//   { &JumperlessLowerc12pt7b, "Jumprls", "Jumperless" },
//   { &Jokerman8pt7b, "Jokermn", "Jokerman" },
// };
const StringIntEntry fontTable[] = {
    {"eurostile", 0},
    {"jokerman", 1},
    {"comicsans", 2},
    {"courier", 3},
    {"science", 4},
    {"scienceext", 5}
};
const int fontTableSize = sizeof(fontTable) / sizeof(fontTable[0]);
// Table for parseBool
const StringIntEntry boolTable[] = {
    {"true", 1},
    {"false", 0},
    {"1", 1},
    {"0", 0},
    {"yes", 1},
    {"no", 0},
    {"on", 1},
    {"off", 0},
    {"enable", 1},
    {"disable", 0},
    {"enabled", 1},
    {"disabled", 0},
    {"t", 1},
    {"f", 0},
    {"y", 1},
    {"n", 0}
};
const int boolTableSize = sizeof(boolTable) / sizeof(boolTable[0]);

// Table for parseUartFunction
const StringIntEntry uartFunctionTable[] = {
    {"off", 0},
    {"disable", 0},
    //{"pass", 1},
    {"passthrough", 1},
    {"port_2", 1},
    {"main", 2},
    {"control", 2},
    {"port_1", 2},
    {"gpio", 3},
    {"auxiliary", 3}
};
const int uartFunctionTableSize = sizeof(uartFunctionTable) / sizeof(uartFunctionTable[0]);

// Table for parseLinesWires
const StringIntEntry linesWiresTable[] = {
    {"lines", 0},
    {"l", 0},
    {"wires", 1},
    {"w", 1},
    {"0", 0},
    {"1", 1}
};
const int linesWiresTableSize = sizeof(linesWiresTable) / sizeof(linesWiresTable[0]);

// Table for parseNetColorMode
const StringIntEntry netColorModeTable[] = {
    {"rainbow", 0},
    {"shuffle", 1},
    {"random", 1},
    {"set_from_serial", 2},
    {"set_from_serial_random", 3},
    {"set_from_serial_shuffle", 4},
    {"set_from_serial_rainbow", 5}
};
const int netColorModeTableSize = sizeof(netColorModeTable) / sizeof(netColorModeTable[0]);

// Table for parseArbitraryFunction
const StringIntEntry arbitraryFunctionTable[] = {
    {"off", -1},
    {"none", -1},
    {"uart_tx", 0},
    {"tx", 0},
    {"uart_rx", 1},
    {"rx", 1},
    {"adc_0", 2},
    {"adc_1", 3},
    {"adc_2", 4},
    {"adc_3", 5},
    {"adc_4", 6},
    {"adc_5", 7},
    {"gpio_0", 8},
    {"gpio_1", 9},
    {"gpio_2", 10},
    {"gpio_3", 11},
    {"gpio_4", 12},
    {"gpio_5", 13},
    {"gpio_6", 14},
    {"gpio_7", 15},
    {"gpio_8", 16},
    {"app_1", 17},
    {"app_2", 18},
    {"app_3", 19},
    {"app_4", 20},
    {"app_5", 21},
    {"app_6", 22},
    {"app_7", 23},
    {"app_8", 24},
    {"isense_pos", 25},
    {"isense+", 25},
    {"isense-", 26},
    {"isense_neg", 26},
    {"gpio_1_toggle", 27},
    {"gpio_2_toggle", 28},
    {"gpio_3_toggle", 29},
    {"gpio_4_toggle", 30},
    {"gpio_5_toggle", 31},
    {"gpio_6_toggle", 32},
    {"gpio_7_toggle", 33},
    {"gpio_8_toggle", 34},
    {"gpio_1_high", 35},
    {"gpio_2_high", 36},
    {"gpio_3_high", 37},
    {"gpio_4_high", 38},
    {"gpio_5_high", 39},
    {"gpio_6_high", 40},
    {"gpio_7_high", 41},
    {"gpio_8_high", 42},
    {"gpio_1_low", 43},
    {"gpio_2_low", 44},
    {"gpio_3_low", 45},
    {"gpio_4_low", 46},
    {"gpio_5_low", 47},
    {"gpio_6_low", 48},
    {"gpio_7_low", 49},
    {"gpio_8_low", 50},
    {"dac_0_+", 51},
    {"dac_0_inc", 51},
    {"dac_0_increase", 51},
    {"dac_0_-", 52},
    {"dac_0_dec", 52},
    {"dac_0_decrease", 52},
    {"dac_1_+", 53},
    {"dac_1_inc", 53},
    {"dac_1_increase", 53},
    {"dac_1_-", 54},
    {"dac_1_dec", 54},
    {"dac_1_decrease", 54}
};
const int arbitraryFunctionTableSize = sizeof(arbitraryFunctionTable) / sizeof(arbitraryFunctionTable[0]);
