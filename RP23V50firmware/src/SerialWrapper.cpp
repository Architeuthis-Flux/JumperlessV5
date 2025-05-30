// SPDX-License-Identifier: MIT
#include "SerialWrapper.h"

// Global volatile bitmasks to control Serial redirection
// When 0, uses default Serial behavior
// When non-zero, redirects Serial calls to specified ports
volatile uint8_t serialReadTarget = 0;
volatile uint8_t serialWriteTarget = 0;

// Global instance of SerialWrapper
SerialWrapper SerialWrap;
