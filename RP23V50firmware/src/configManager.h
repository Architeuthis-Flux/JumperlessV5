#pragma once

#include "config.h"
#include <FatFS.h>


extern bool configChanged;
// Global configuration instance
//extern struct config jumperlessConfig;

// Core configuration functions
void loadConfig(void);
void saveConfig(void);
void resetConfigToDefaults(void);

// File operations
void updateConfigFromFile(const char* filename);
void saveConfigToFile(const char* filename);

// Serial operations
void printConfigSectionToSerial(int section);
void readConfigFromSerial(void);
void printConfigToSerial(void);
void printConfigStructToSerial(void);

// Helper functions
void parseCommaSeparatedInts(const char* str, int* array, int maxValues);
bool parseBool(const char* str);
float parseFloat(const char* str);
int parseInt(const char* str);
void trim(char* str);
void toLower(char* str); 
void updateConfigValue(const char* section, const char* key, const char* value);