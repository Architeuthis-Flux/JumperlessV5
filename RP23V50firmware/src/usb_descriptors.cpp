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
#include "JumperlessDefines.h"

// Override compile-time config with runtime dynamic config
#define OVERRIDE_USB_CONFIG 1
#include "usb_interface_config.h"
#include "config.h"
#include "configManager.h"

// Add missing interface name constants
#define USB_HID_NAME     "JL HID Interface"
#define USB_MIDI_NAME    "JL MIDI Interface"  
#define USB_VENDOR_NAME  "JL Vendor Interface"

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

    // Use Interface Association Descriptor (IAD) for composite device
    .bDeviceClass       = TUSB_CLASS_MISC,
    .bDeviceSubClass    = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol    = MISC_PROTOCOL_IAD,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0x1D50,
    .idProduct          = 0xACAB,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

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
    Serial.println("◆ USB Device descriptor requested");
    Serial.printf("◆ Device class: 0x%02X, subclass: 0x%02X, protocol: 0x%02X\n",
                 desc_device.bDeviceClass, desc_device.bDeviceSubClass, desc_device.bDeviceProtocol);
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
// Interface numbers are defined in usb_interface_config.h
//--------------------------------------------------------------------+

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

// CDC endpoints - use sequential numbering to avoid conflicts
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

#if USB_CDC_ENABLE_COUNT >= 5
#define EPNUM_CDC_4_NOTIF   0x89
#define EPNUM_CDC_4_OUT     0x0A
#define EPNUM_CDC_4_IN      0x8A
#endif

// MSC endpoints - place after all CDC endpoints to avoid conflicts
#if USB_MSC_ENABLE
#define EPNUM_MSC_OUT       0x0B    // EP11 OUT (after CDC endpoints)
#define EPNUM_MSC_IN        0x8B    // EP11 IN (after CDC endpoints)
#endif

// HID endpoints (if enabled in future)
#if USB_HID_ENABLE_COUNT >= 1
#define EPNUM_HID_0         (EPNUM_MSC_IN + 1)
#endif

#if USB_HID_ENABLE_COUNT >= 2
#define EPNUM_HID_1         (EPNUM_HID_0 + 1)
#endif

// MIDI endpoints (if enabled in future)
#if USB_MIDI_ENABLE
#define EPNUM_MIDI_OUT      (EPNUM_MSC_OUT + 1)
#define EPNUM_MIDI_IN       (EPNUM_MSC_IN + 1)
#endif

// Vendor endpoints (if enabled in future)
#if USB_VENDOR_ENABLE
#define EPNUM_VENDOR_OUT    (EPNUM_MSC_OUT + 1)
#define EPNUM_VENDOR_IN     (EPNUM_MSC_IN + 1)
#endif

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_fs_configuration[] =
{
  // Config number, interface count, string index, total length, attribute, power in mA
  TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x80, 500),

  // CDC interfaces with IADs for better enumeration
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

#if USB_CDC_ENABLE_COUNT >= 5
  TUD_CDC_DESCRIPTOR(ITF_NUM_CDC_4, 8, EPNUM_CDC_4_NOTIF, 8, EPNUM_CDC_4_OUT, EPNUM_CDC_4_IN, 64),
#endif

  // MSC interface - place after CDC interfaces for better compatibility
#if USB_MSC_ENABLE
  TUD_MSC_DESCRIPTOR(ITF_NUM_MSC, 9, EPNUM_MSC_OUT, EPNUM_MSC_IN, 64),
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
  
  static bool first_call = true;
  if (first_call) {
    first_call = false;
    Serial.println("◆ USB Configuration descriptor requested");
    Serial.printf("◆ Total interfaces: %d\n", ITF_NUM_TOTAL);
    Serial.printf("◆ CDC interfaces: %d\n", USB_CDC_ENABLE_COUNT);
    Serial.printf("◆ MSC enabled: %d\n", USB_MSC_ENABLE);
    Serial.printf("◆ Config length: %d bytes\n", CONFIG_TOTAL_LEN);
    
    // Debug interface assignments
    Serial.println("◆ Interface assignments:");
#if USB_CDC_ENABLE_COUNT >= 1
    Serial.printf("◆   CDC 0: interfaces %d, %d\n", ITF_NUM_CDC_0, ITF_NUM_CDC_0_DATA);
#endif
#if USB_CDC_ENABLE_COUNT >= 2
    Serial.printf("◆   CDC 1: interfaces %d, %d\n", ITF_NUM_CDC_1, ITF_NUM_CDC_1_DATA);
#endif
#if USB_CDC_ENABLE_COUNT >= 3
    Serial.printf("◆   CDC 2: interfaces %d, %d\n", ITF_NUM_CDC_2, ITF_NUM_CDC_2_DATA);
#endif
#if USB_CDC_ENABLE_COUNT >= 4
    Serial.printf("◆   CDC 3: interfaces %d, %d\n", ITF_NUM_CDC_3, ITF_NUM_CDC_3_DATA);
#endif
#if USB_CDC_ENABLE_COUNT >= 5
    Serial.printf("◆   CDC 4: interfaces %d, %d\n", ITF_NUM_CDC_4, ITF_NUM_CDC_4_DATA);
#endif
#if USB_MSC_ENABLE
    Serial.printf("◆   MSC: interface %d\n", ITF_NUM_MSC);
    Serial.printf("◆   MSC endpoints: OUT=0x%02X, IN=0x%02X\n", EPNUM_MSC_OUT, EPNUM_MSC_IN);
#endif
  }
  
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
  STRID_CDC_4,
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
  "JLV5port",                     // 3: Serials, should use chip ID
  //"JLLA",            // 4: Logic Analyzer
  
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

#if USB_CDC_ENABLE_COUNT >= 5
  USB_CDC_NAMES[4],              // 8: CDC Interface 4
#endif

  // Other interface names
#if USB_MSC_ENABLE
  USB_MSC_NAME,                  // 9: MSC Interface
#endif
#if USB_HID_ENABLE_COUNT > 0
  USB_HID_NAME,                  // 10: HID Interface
#endif
#if USB_MIDI_ENABLE
  USB_MIDI_NAME,                 // 11: MIDI Interface
#endif
#if USB_VENDOR_ENABLE
  USB_VENDOR_NAME,               // 12: Vendor Interface
#endif
};

static uint16_t _desc_str[32];

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
    // Use static string array only - no dynamic naming for now
    if ( !(index < sizeof(string_desc_arr)/sizeof(string_desc_arr[0])) ) {
      return NULL;
    }
    
    const char* str = string_desc_arr[index];
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