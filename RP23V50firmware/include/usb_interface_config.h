#ifndef USB_INTERFACE_CONFIG_H
#define USB_INTERFACE_CONFIG_H
// Note: Don't include config.h here - it causes preprocessor issues
// We'll handle dynamic naming in the USB descriptor callback

// =============================================================================
// USB Interface Configuration
// =============================================================================
// Enable/disable individual USB interface types by setting to 1 or 0
// This allows testing different combinations to find optimal configuration

// CDC Serial Interfaces (Communication Device Class)
#define USB_CDC_ENABLE_COUNT     3    // 0-4: Number of CDC interfaces to enable
                                      // 0 = None, 1 = Main only, 2 = Main+Arduino, 
                                      // 3 = Main+Arduino+Routable, 4 = All

// Mass Storage Interface
#define USB_MSC_ENABLE           1    // 0 = Disabled, 1 = Enabled

// HID Interfaces (Human Interface Device)  
#define USB_HID_ENABLE_COUNT     0    // 0-2: Number of HID interfaces to enable

// MIDI Interface
#define USB_MIDI_ENABLE          0    // 0 = Disabled, 1 = Enabled

// Vendor Interface (Custom protocol)
#define USB_VENDOR_ENABLE        0    // 0 = Disabled, 1 = Enabled

// =============================================================================
// Automatic Configuration Validation
// =============================================================================

#if USB_CDC_ENABLE_COUNT > 4
#error "Maximum 4 CDC interfaces supported"
#endif

#if USB_HID_ENABLE_COUNT > 2  
#error "Maximum 2 HID interfaces supported"
#endif

// Calculate total interface count for validation
#define USB_TOTAL_INTERFACES \
    ((USB_CDC_ENABLE_COUNT * 2) + \
     USB_MSC_ENABLE + \
     USB_HID_ENABLE_COUNT + \
     (USB_MIDI_ENABLE * 2) + \
     USB_VENDOR_ENABLE)

#if USB_TOTAL_INTERFACES > 14
#warning "High interface count - may cause enumeration issues"
#endif

// =============================================================================
// Interface Names (for string descriptors) 
// =============================================================================
// Note: These are fallback names. Dynamic names based on config are generated
// at runtime in the USB descriptor callback function.

static const char* USB_CDC_NAMES[] = {
    "Jumperless Main",       // CDC 0 - Always the main serial
    "Jumperless Serial 1",   // CDC 1 - Arduino programming/communication  
    "Jumperless Serial 2",   // CDC 2 - User-configurable serial
    "Jumperless Serial 3"    // CDC 3 - Debug output
};

#define USB_MSC_NAME     "JL Mass Storage"
#define USB_HID_NAME     "JL HID Device" 
#define USB_MIDI_NAME    "JL MIDI Device"
#define USB_VENDOR_NAME  "JL Vendor Device"

#endif // USB_INTERFACE_CONFIG_H 