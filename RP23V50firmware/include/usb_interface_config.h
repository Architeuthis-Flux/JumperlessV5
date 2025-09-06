#ifndef USB_INTERFACE_CONFIG_H
#define USB_INTERFACE_CONFIG_H
#include <stdint.h>
// Note: Don't include config.h here - it causes preprocessor issues
// We'll handle dynamic naming in the USB descriptor callback

// =============================================================================
// SIMPLE STATIC USB Interface Configuration
// =============================================================================
// Fixed configuration: 5 CDC + 1 MSC for logic analyzer on port 3

// CDC Serial Interfaces (Communication Device Class)  
#define USB_CDC_ENABLE_COUNT 4

// MSC (Mass Storage Class) - Always enabled
#define USB_MSC_ENABLE 1

// All other interfaces disabled for simplicity
#define USB_MIDI_ENABLE 0
#define USB_HID_ENABLE 0
#define USB_HID_ENABLE_COUNT 0
#define USB_VENDOR_ENABLE 0

// =============================================================================
// TinyUSB Configuration Mapping
// =============================================================================

#define CFG_TUD_CDC USB_CDC_ENABLE_COUNT
#define CFG_TUD_MSC USB_MSC_ENABLE
#define CFG_TUD_MIDI USB_MIDI_ENABLE
#define CFG_TUD_HID USB_HID_ENABLE

// =============================================================================
// Simple Static Interface Names
// =============================================================================

static const char* USB_CDC_NAMES[] = {
    "Jumperless Main",       // CDC 0 - Main serial
    "Jumperless Serial 1",   // CDC 1 - Arduino/Serial1
    "JL Logic Analyzer",     // CDC 2 - Logic analyzer (port 3)
    "JL TUI",                // CDC 3 - TUI
    "JL Serial 3"            // CDC 4 - Serial 3
};

#define USB_MSC_NAME     "JL Mass Storage"

// =============================================================================
// Interface Number Definitions (for debugging and verification)
// =============================================================================

// Calculate interface numbers based on enabled interfaces
enum {
  // CDC interfaces (each CDC uses 2 interface numbers)
#if USB_CDC_ENABLE_COUNT >= 1
  ITF_NUM_CDC_0 = 0,
  ITF_NUM_CDC_0_DATA,
#endif
#if USB_CDC_ENABLE_COUNT >= 2
  ITF_NUM_CDC_1,
  ITF_NUM_CDC_1_DATA,
#endif
#if USB_CDC_ENABLE_COUNT >= 3
  ITF_NUM_CDC_2,
  ITF_NUM_CDC_2_DATA,
#endif
#if USB_CDC_ENABLE_COUNT >= 4
  ITF_NUM_CDC_3,
  ITF_NUM_CDC_3_DATA,
#endif
#if USB_CDC_ENABLE_COUNT >= 5
  ITF_NUM_CDC_4,
  ITF_NUM_CDC_4_DATA,
#endif

  // MSC interface
#if USB_MSC_ENABLE
  ITF_NUM_MSC,
#endif

  ITF_NUM_TOTAL
};

#endif // USB_INTERFACE_CONFIG_H 