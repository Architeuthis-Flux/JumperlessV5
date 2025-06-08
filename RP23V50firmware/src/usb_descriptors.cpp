/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "usb_interface_config.h"
#include "config.h"
#include "configManager.h"

extern "C" {

// Declare the real functions for linker wrapping
extern uint8_t const * __real_tud_descriptor_device_cb(void);
extern uint8_t const * __real_tud_descriptor_configuration_cb(uint8_t index);
extern uint16_t const * __real_tud_descriptor_string_cb(uint8_t index, uint16_t langid);

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,

    // Use Interface Association Descriptor (IAD) for CDC
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x1D50,
    .idProduct          = 0xACAB,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,

    .bNumConfigurations = 0x01
};

// Wrapped function that intercepts the library's descriptor callback
__attribute__((used))
uint8_t const * __wrap_tud_descriptor_device_cb(void)
{
  // Debug: Flash the LED to show our descriptor is being called
  static bool first_call = true;
  if (first_call) {
    first_call = false;
  }
  return (uint8_t const *) &desc_device;
}

// HID Report Descriptor
#if USB_HID_ENABLE_COUNT > 0
uint8_t const desc_hid_report[] =
{
  TUD_HID_REPORT_DESC_GENERIC_INOUT(16)
};
#endif

//--------------------------------------------------------------------+
// Dynamic Interface Numbering
//--------------------------------------------------------------------+

// Calculate interface numbers dynamically based on enabled interfaces
enum
{
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

  // MSC interface
#if USB_MSC_ENABLE
  ITF_NUM_MSC,
#endif

  // HID interfaces
#if USB_HID_ENABLE_COUNT >= 1
  ITF_NUM_HID_0,
#endif
#if USB_HID_ENABLE_COUNT >= 2
  ITF_NUM_HID_1,
#endif

  // MIDI interface (uses 2 interface numbers)
#if USB_MIDI_ENABLE
  ITF_NUM_MIDI,
  ITF_NUM_MIDI_STREAMING,
#endif

  // Vendor interface
#if USB_VENDOR_ENABLE
  ITF_NUM_VENDOR,
#endif

  ITF_NUM_TOTAL
};

// Calculate total descriptor length
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + \
                          (USB_CDC_ENABLE_COUNT * TUD_CDC_DESC_LEN) + \
                          (USB_MSC_ENABLE * TUD_MSC_DESC_LEN) + \
                          (USB_HID_ENABLE_COUNT * TUD_HID_DESC_LEN) + \
                          (USB_MIDI_ENABLE * TUD_MIDI_DESC_LEN) + \
                          (USB_VENDOR_ENABLE * TUD_VENDOR_DESC_LEN))

//--------------------------------------------------------------------+
// Dynamic Endpoint Assignment
//--------------------------------------------------------------------+

// CDC endpoints
#if USB_CDC_ENABLE_COUNT >= 1
#define EPNUM_CDC_0_NOTIF   0x81
#define EPNUM_CDC_0_OUT     0x02
#define EPNUM_CDC_0_IN      0x82
#endif

#if USB_CDC_ENABLE_COUNT >= 2
#define EPNUM_CDC_1_NOTIF   0x83
#define EPNUM_CDC_1_OUT     0x04
#define EPNUM_CDC_1_IN      0x84
#endif

#if USB_CDC_ENABLE_COUNT >= 3
#define EPNUM_CDC_2_NOTIF   0x85
#define EPNUM_CDC_2_OUT     0x06
#define EPNUM_CDC_2_IN      0x86
#endif

#if USB_CDC_ENABLE_COUNT >= 4
#define EPNUM_CDC_3_NOTIF   0x87
#define EPNUM_CDC_3_OUT     0x08
#define EPNUM_CDC_3_IN      0x88
#endif

// Other interface endpoints (start after CDC endpoints)
#define EPNUM_NEXT_OUT      (0x02 + (USB_CDC_ENABLE_COUNT * 2))
#define EPNUM_NEXT_IN       (0x82 + (USB_CDC_ENABLE_COUNT * 2))

#if USB_MSC_ENABLE
#define EPNUM_MSC_OUT       EPNUM_NEXT_OUT
#define EPNUM_MSC_IN        (EPNUM_NEXT_IN)
#define EPNUM_AFTER_MSC_OUT (EPNUM_MSC_OUT + 1)
#define EPNUM_AFTER_MSC_IN  (EPNUM_MSC_IN + 1)
#else
#define EPNUM_AFTER_MSC_OUT EPNUM_NEXT_OUT
#define EPNUM_AFTER_MSC_IN  EPNUM_NEXT_IN
#endif

#if USB_HID_ENABLE_COUNT >= 1
#define EPNUM_HID_0         EPNUM_AFTER_MSC_IN
#endif

#if USB_HID_ENABLE_COUNT >= 2
#define EPNUM_HID_1         (EPNUM_HID_0 + 1)
#endif

#if USB_MIDI_ENABLE
#define EPNUM_MIDI_OUT      (EPNUM_AFTER_MSC_OUT + (USB_HID_ENABLE_COUNT > 0 ? 1 : 0))
#define EPNUM_MIDI_IN       (EPNUM_AFTER_MSC_IN + USB_HID_ENABLE_COUNT)
#endif

#if USB_VENDOR_ENABLE
#define EPNUM_VENDOR_OUT    (EPNUM_AFTER_MSC_OUT + (USB_HID_ENABLE_COUNT > 0 ? 1 : 0) + (USB_MIDI_ENABLE ? 1 : 0))
#define EPNUM_VENDOR_IN     (EPNUM_AFTER_MSC_IN + USB_HID_ENABLE_COUNT + (USB_MIDI_ENABLE ? 1 : 0))
#endif

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 500),

  // CDC interfaces
#if USB_CDC_ENABLE_COUNT >= 1
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_0, 4, EPNUM_CDC_0_NOTIF, 8, EPNUM_CDC_0_OUT, EPNUM_CDC_0_IN, 64),
#endif

#if USB_CDC_ENABLE_COUNT >= 2
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_1, 5, EPNUM_CDC_1_NOTIF, 8, EPNUM_CDC_1_OUT, EPNUM_CDC_1_IN, 64),
#endif

#if USB_CDC_ENABLE_COUNT >= 3
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_2, 6, EPNUM_CDC_2_NOTIF, 8, EPNUM_CDC_2_OUT, EPNUM_CDC_2_IN, 64),
#endif

#if USB_CDC_ENABLE_COUNT >= 4
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_3, 7, EPNUM_CDC_3_NOTIF, 8, EPNUM_CDC_3_OUT, EPNUM_CDC_3_IN, 64),
#endif

  // MSC interface
#if USB_MSC_ENABLE
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 8, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
#endif

  // HID interfaces
#if USB_HID_ENABLE_COUNT >= 1
  TUD_HID_DESCRIPTOR(ITF_NUM_HID_0, 9, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_0, 16, 10),
#if USB_HID_ENABLE_COUNT >= 2
  TUD_HID_DESCRIPTOR(ITF_NUM_HID_1, 9, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID_1, 16, 10),
#endif
#endif

  // MIDI interface
#if USB_MIDI_ENABLE
  TUD_MIDI_DESCRIPTOR(ITF_NUM_MIDI, 10, EPNUM_MIDI_OUT, EPNUM_MIDI_IN, 64),
#endif

  // Vendor interface
#if USB_VENDOR_ENABLE
  TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 11, EPNUM_VENDOR_OUT, EPNUM_VENDOR_IN, 64),
#endif
};

// Wrapped function that intercepts the library's configuration descriptor callback
__attribute__((used))
uint8_t const * __wrap_tud_descriptor_configuration_cb(uint8_t index)
{
  (void) index; // for multiple configurations
  return desc_fs_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// String Descriptor Index
enum {
  STRID_LANGID = 0,
  STRID_MANUFACTURER,
  STRID_PRODUCT,
  STRID_SERIAL,
  STRID_CDC_0,
  STRID_CDC_1,
  STRID_CDC_2,
  STRID_CDC_3,
  STRID_MSC,
  STRID_HID,
  STRID_MIDI,
  STRID_VENDOR
};

// Dynamic string descriptor array
static const char* string_desc_arr [] =
{
  (const char[]) { 0x09, 0x04 }, // 0: is supported language is English (0x0409)
  "Architeuthis Flux",           // 1: Manufacturer
  "Jumperless V5",               // 2: Product  
  "JL5V001",                     // 3: Serials, should use chip ID
  
  // CDC interface names (only include if enabled)
#if USB_CDC_ENABLE_COUNT >= 1
  USB_CDC_NAMES[0],              // 4: CDC Interface 0
#endif
#if USB_CDC_ENABLE_COUNT >= 2
  USB_CDC_NAMES[1],              // 5: CDC Interface 1
#endif
#if USB_CDC_ENABLE_COUNT >= 3
  USB_CDC_NAMES[2],              // 6: CDC Interface 2
#endif
#if USB_CDC_ENABLE_COUNT >= 4
  USB_CDC_NAMES[3],              // 7: CDC Interface 3
#endif

  // Other interface names
#if USB_MSC_ENABLE
  USB_MSC_NAME,                  // 8: MSC Interface
#endif
#if USB_HID_ENABLE_COUNT > 0
  USB_HID_NAME,                  // 9: HID Interface
#endif
#if USB_MIDI_ENABLE
  USB_MIDI_NAME,                 // 10: MIDI Interface
#endif
#if USB_VENDOR_ENABLE
  USB_VENDOR_NAME,               // 11: Vendor Interface
#endif
};

static uint16_t _desc_str[32];

// Helper function to generate dynamic CDC interface names based on config
const char* get_dynamic_cdc_name(uint8_t cdc_index) {
  static char dynamic_name[32];
  
  switch(cdc_index) {
    case 0:
      return "Jumperless Main";
      
    case 1: {
      // Get function name from table for serial 1
      const char* function_name = getStringFromTable(jumperlessConfig.serial_1.function, uartFunctionTable);
      if (function_name && strcmp(function_name, "off") != 0 && strcmp(function_name, "disable") != 0) {
        // Capitalize first letter and format nicely
        char formatted_function[16];
        strncpy(formatted_function, function_name, sizeof(formatted_function) - 1);
        formatted_function[sizeof(formatted_function) - 1] = '\0';
        
        // Capitalize first letter
        if (formatted_function[0] >= 'a' && formatted_function[0] <= 'z') {
          formatted_function[0] = formatted_function[0] - 'a' + 'A';
        }
        
        // Replace underscores with spaces
        for (int i = 0; formatted_function[i]; i++) {
          if (formatted_function[i] == '_') {
            formatted_function[i] = ' ';
          }
        }
        
        snprintf(dynamic_name, sizeof(dynamic_name), "JL %s", formatted_function);
      } else {
        snprintf(dynamic_name, sizeof(dynamic_name), "Jumperless Serial 1");
      }
      return dynamic_name;
    }
      
    case 2: {
      // Get function name from table for serial 2
      const char* function_name = getStringFromTable(jumperlessConfig.serial_2.function, uartFunctionTable);
      if (function_name && strcmp(function_name, "off") != 0 && strcmp(function_name, "disable") != 0) {
        // Capitalize first letter and format nicely
        char formatted_function[16];
        strncpy(formatted_function, function_name, sizeof(formatted_function) - 1);
        formatted_function[sizeof(formatted_function) - 1] = '\0';
        
        // Capitalize first letter
        if (formatted_function[0] >= 'a' && formatted_function[0] <= 'z') {
          formatted_function[0] = formatted_function[0] - 'a' + 'A';
        }
        
        // Replace underscores with spaces
        for (int i = 0; formatted_function[i]; i++) {
          if (formatted_function[i] == '_') {
            formatted_function[i] = ' ';
          }
        }
        
        snprintf(dynamic_name, sizeof(dynamic_name), "JL %s", formatted_function);
      } else {
        snprintf(dynamic_name, sizeof(dynamic_name), "Jumperless Serial 2");
      }
      return dynamic_name;
    }
      
    case 3:
      return "Jumperless Debug";
      
    default:
      return USB_CDC_NAMES[cdc_index]; // Fallback to static names
  }
}

// Wrapped function that intercepts the library's string descriptor callback
__attribute__((used))
uint16_t const* __wrap_tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
  (void) langid;

  uint8_t chr_count;

  if ( index == 0)
  {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  }
  else
  {
    // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
    // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

    const char* str = NULL;
    
    // Handle dynamic CDC interface naming
    if (index >= STRID_CDC_0 && index <= STRID_CDC_3) {
      uint8_t cdc_index = index - STRID_CDC_0;
      str = get_dynamic_cdc_name(cdc_index);
    }
    else {
      // Use static string array for non-CDC interfaces
      if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) return NULL;
      str = string_desc_arr[index];
    }

    if (!str) return NULL;

    // Cap at max char
    chr_count = strlen(str);
    if ( chr_count > 31 ) chr_count = 31;

    // Convert ASCII string into UTF-16
    for(uint8_t i=0; i<chr_count; i++)
    {
      _desc_str[1+i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2*chr_count + 2);

  return _desc_str;
}

} // extern "C" 