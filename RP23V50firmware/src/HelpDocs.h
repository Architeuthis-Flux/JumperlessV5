#ifndef HELPDOCS_H
#define HELPDOCS_H

#include <Arduino.h>

// Function to show help for a specific command
void showCommandHelp(char command);

// Function to show general help menu
void showGeneralHelp();

// Function to show category-specific help
void showCategoryHelp(const char* category);

// Function to parse and handle help requests
bool handleHelpRequest(const char* input);

// Helper function to check if input is a help request
bool isHelpRequest(const char* input);

#endif
