// SPDX-License-Identifier: MIT
#include "SerialWrapper.h"

// Global volatile bitmask to control Serial redirection
// When 0, uses default Serial behavior
// When non-zero, redirects Serial calls to specified ports
volatile uint8_t serialTarget = 0;

// Global instance of SerialWrapper
SerialWrapper SerialWrap;
