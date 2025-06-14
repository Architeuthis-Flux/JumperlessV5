#ifndef PYTHON_PROPER_H
#define PYTHON_PROPER_H

#include <Arduino.h>

// Forward declaration from existing Python.cpp
extern int parseAndExecutePythonCommand(char* command, char* response);

// Core initialization and cleanup
bool initMicroPythonProper(void);
void deinitMicroPythonProper(void);

// Code execution
bool executePythonCodeProper(const char* code);
bool executePythonSimple(const char* code, char* response, size_t response_size);

// REPL control
void startMicroPythonREPL(void);
void stopMicroPythonREPL(void);
bool isMicroPythonREPLActive(void);
void processMicroPythonInput(void);

// Simple blocking REPL function - call from main.cpp
void enterMicroPythonREPL(void);

// Helper functions
void addJumperlessPythonFunctions(void);
void testJumperlessNativeModule(void);

// Status functions
bool isMicroPythonInitialized(void);
void printMicroPythonStatus(void);

// Terminal color control
void changeTerminalColor(int color, bool bold);

#endif // PYTHON_PROPER_H 