/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#if 0
#include "LogicAnalyzer.h"
#include <stdlib.h>
#include "hardware/pio.h"
#include "hardware/dma.h"
#include "hardware/adc.h"
#include "hardware/irq.h"
#include "hardware/clocks.h"
#include "JumperlessDefines.h"
#include "Peripherals.h"
#include "Arduino.h"
#include "Config.h"
#include "ArduinoStuff.h"
#include "pico/stdlib.h"

// SUMP Protocol Commands
#define SUMP_CMD_RESET 0x00
#define SUMP_CMD_ID 0x01
#define SUMP_CMD_ARM 0x02
#define SUMP_CMD_QUERY_METADATA 0x04
#define SUMP_CMD_QUERY_ACQUISITION 0x05
#define SUMP_CMD_SET_SAMPLE_RATE 0x80
#define SUMP_CMD_SET_READ_DELAY_COUNT 0x81
#define SUMP_CMD_SET_FLAGS 0x82
#define SUMP_CMD_SET_TRIGGER_MASK 0xC0
#define SUMP_CMD_SET_TRIGGER_VALUES 0xC1
#define SUMP_CMD_SET_TRIGGER_CONFIG 0xC2

// Data format markers for PulseView
#define DIGITAL_ONLY_MARKER 0xDD
#define MIXED_SIGNAL_MARKER 0xDA
#define ANALOG_ONLY_MARKER 0xAA
#define ANALOG_EOF_MARKER   0xA0

// Jumperless Protocol Commands & Responses
#define JUMPERLESS_CMD_RESET          0x00
#define JUMPERLESS_CMD_RUN            0x01
#define JUMPERLESS_CMD_ID             0xA2
#define JUMPERLESS_CMD_GET_HEADER     0xA3
#define JUMPERLESS_CMD_SET_CHANNELS   0xA4
#define JUMPERLESS_CMD_ARM            0xA5
#define JUMPERLESS_CMD_GET_STATUS     0xA6
#define JUMPERLESS_CMD_CONFIGURE      0xA7
#define JUMPERLESS_CMD_SET_SAMPLES    0xA8
#define JUMPERLESS_CMD_SET_MODE       0xA9
#define JUMPERLESS_CMD_SET_TRIGGER    0xAA
#define JUMPERLESS_CMD_CLEAR_TRIGGER  0xAB
#define JUMPERLESS_RESP_HEADER        0x80
#define JUMPERLESS_RESP_DATA          0x81
#define JUMPERLESS_RESP_STATUS        0x82
#define JUMPERLESS_RESP_ERROR         0x83

// Status codes
#define JUMPERLESS_STATUS_OK          0x00
#define JUMPERLESS_STATUS_ERROR       0x01

// Configuration constants
#define JL_LA_MIN_SAMPLES       1
#define JL_LA_MAX_SAMPLES_LIMIT 262144
#define JL_LA_RESERVE_RAM       (32 * 1024) // 32KB safety margin
#define MAX_SAMPLE_RATE_HZ      50000000    // 50 MHz
#define CAPTURE_CHANNEL_FIRST   0           // ADC0

// Logic analyzer mode definitions
typedef enum {
    LA_MODE_DIGITAL_ONLY = 0,
    LA_MODE_MIXED_SIGNAL = 1,
    LA_MODE_ANALOG_ONLY = 2
} LogicAnalyzerMode;

// Debug flags
bool debugLA = true;
#define DEBUG_LA_PRINTF(fmt, ...) do { if(debugLA) { Serial.printf(fmt, ##__VA_ARGS__); } } while(0)
#define DEBUG_LA_PRINTLN(x) do { if(debugLA) { Serial.println(x); } } while(0)

// Global state variables
static LogicAnalyzerMode current_la_mode = LA_MODE_MIXED_SIGNAL;
static uint32_t sample_rate = 100000;
static uint32_t sample_count = 10000;
static uint32_t jl_la_max_samples = 0;
static uint32_t digital_mask = 0xFF;
static uint32_t analog_mask = 0x07; // Default to 3 channels
static uint8_t digital_chan_count = 8;
static uint8_t analog_chan_count = 3;

// Data format structures matching PulseView expectations
typedef struct {
    uint8_t gpio_data;           // [byte 0] GPIO data: (gpio1)(gpio2)(gpio3)(gpio4)(gpio5)(gpio6)(gpio7)(gpio8)
    uint8_t uart_data;           // [byte 1] UART data: (uart_tx)(uart_rx)(unused)(unused)(unused)(unused)(unused)(unused)
    uint8_t format_marker;       // [byte 2] Format marker: 0xDD=Digital, 0xDA=Mixed, 0xAA=Analog
} __attribute__((packed)) DigitalSample;

typedef struct {
    uint8_t gpio_data;           // [byte 0] GPIO data
    uint8_t uart_data;           // [byte 1] UART data  
    uint8_t format_marker;       // [byte 2] Format marker (0xDA for mixed signal)
    uint16_t analog_data[14];    // [bytes 3-30] Analog data (14 channels × 2 bytes each, little-endian)
    uint8_t eof_marker;          // [byte 31] EOF marker: 0xA0
} __attribute__((packed)) MixedSignalSample;

typedef enum {
    STATE_IDLE,
    STATE_ARMED,
    STATE_CAPTURING,
    STATE_DATA_READY
} CaptureState;

CaptureState capture_state = STATE_IDLE;

// Capture state variables
static uint8_t* digital_buffer = NULL;
static uint16_t* analog_buffer = NULL;
static size_t analog_buffer_size = 0;
static PIO pio = pio0;
static uint sm;
static uint offset;
static int dma_chan = -1;
volatile bool analog_sample_tick = false;
volatile bool run_capture = false;
repeating_timer_t sample_timer;

// Forward declarations
void invalidateBufferCache();
bool allocateLogicAnalyzerResources();
void releaseLogicAnalyzerResources();
void handleResetCommand();
void handleIdCommand();
void handleSetModeCommand(LogicAnalyzerMode mode);
void handleSetChannelsCommand(uint32_t digital_mask_data, uint32_t analog_mask_data);
void handleArmCommand();
void handleRunCommand();
void handleConfigureCommand(uint32_t rate, uint32_t count);
void handleSetSamplesCommand(uint32_t count);
void handleSetTriggerCommand(uint8_t type, uint32_t mask, uint32_t value, uint32_t edge);
void handleClearTriggerCommand();
void processPulseViewCommand(uint8_t cmd);

// USB Helpers
bool la_usb_connected() {
    #if USB_CDC_ENABLE_COUNT >= 3
    return USBSer2.dtr();
    #else
    return false;
    #endif
}

void la_usb_write(uint8_t data) {
    #if USB_CDC_ENABLE_COUNT >= 3
    USBSer2.write(data);
    #endif
}

void la_usb_write_buffer(const uint8_t* data, size_t len) {
    #if USB_CDC_ENABLE_COUNT >= 3
    // STABILITY: Break large writes into smaller chunks to prevent buffer overruns
    const size_t MAX_CHUNK = 64;  // Smaller chunks for stability
    size_t offset = 0;
    
    while (offset < len) {
        size_t chunk_size = min(MAX_CHUNK, len - offset);
        
        // Wait for buffer space with timeout and yield
        uint32_t start_time = millis();
        while (USBSer2.availableForWrite() < chunk_size) {
            if (millis() - start_time > 500) {  // Shorter timeout
                DEBUG_LA_PRINTLN("◆ WARNING: USB buffer full - continuing");
                return;
            }
            delayMicroseconds(50);  // Shorter delay
            yield();  // Allow other cores to run
        }
        
        USBSer2.write(data + offset, chunk_size);
        offset += chunk_size;
        
        // Small delay between chunks for stability
        if (offset < len) {
            delayMicroseconds(10);
        }
    }
    #endif
}

int la_usb_available() {
    #if USB_CDC_ENABLE_COUNT >= 3
    return USBSer2.available();
    #else
    return 0;
    #endif
}

uint8_t la_usb_read() {
    #if USB_CDC_ENABLE_COUNT >= 3
    return USBSer2.read();
    #else
    return 0;
    #endif
}

void la_usb_read_buffer(uint8_t* data, size_t len) {
    #if USB_CDC_ENABLE_COUNT >= 3
    USBSer2.readBytes(data, len);
    #endif
}

void la_usb_flush() {
    #if USB_CDC_ENABLE_COUNT >= 3
    USBSer2.flush();
    #endif
}

static const struct pio_program la_pio_program_fast = {
    .instructions = (uint16_t[]){
        0x20a0, //  0: wait   1 pin, 0                   
        0x4008, //  1: in     pins, 8                    
        0x8000, //  2: push   noblock                    
        0x0001, //  3: jmp    1                          
    },
    .length = 4,
    .origin = -1,
};

void la_pio_program_init(PIO pio, uint sm, uint offset, uint pin_base) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, pin_base);
    sm_config_set_in_shift(&c, false, true, 8); // Shift left, auto-push, 8 bits
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    pio_sm_init(pio, sm, offset, &c);
}

bool calculateAndAllocateBuffers() {
    DEBUG_LA_PRINTF("◆ ATOMIC: Calculate + Allocate buffers in single step...\n");
    
    // Driver configuration (authoritative)
    uint32_t driver_analog_channels = __builtin_popcount(analog_mask & 0xFF);
    
    // Step 1: Free existing memory to get accurate measurement
    bool had_digital_buffer = (digital_buffer != nullptr);
    bool had_analog_buffer = (analog_buffer != nullptr);
    size_t old_digital_size = 0;
    size_t old_analog_size = analog_buffer_size;
    
    if (had_digital_buffer) {
        old_digital_size = malloc_usable_size(digital_buffer);
        free(digital_buffer);
        digital_buffer = nullptr;
    }
    if (had_analog_buffer) {
        free(analog_buffer);
        analog_buffer = nullptr;
        analog_buffer_size = 0;
    }
    
    DEBUG_LA_PRINTF("◆ FREED: %zu digital + %zu analog bytes for accurate calculation\n",
                 old_digital_size, old_analog_size);
    
    // Step 2: Get accurate free memory and calculate requirements
    size_t free_heap = rp2040.getFreeHeap();
    size_t safety_reserve = JL_LA_RESERVE_RAM + 4096;  // 32KB + 4KB extra margin
    size_t usable_memory;
    
    if (free_heap > safety_reserve) {
        usable_memory = free_heap - safety_reserve;
    } else {
        usable_memory = 1024;  // Absolute minimum
    }
    
    // Calculate storage requirements per sample
    uint32_t digital_bytes_per_sample = 1;  // Always 1 byte digital storage
    uint32_t analog_bytes_per_sample = 0;
    
    if (current_la_mode == LA_MODE_MIXED_SIGNAL || current_la_mode == LA_MODE_ANALOG_ONLY) {
        analog_bytes_per_sample = driver_analog_channels * 2;  // 2 bytes per active channel
    }
    
    uint32_t total_bytes_per_sample = digital_bytes_per_sample + analog_bytes_per_sample;
    
    // Calculate maximum samples based on available memory
    uint32_t max_samples = usable_memory / total_bytes_per_sample;
    
    // Apply 2% stability margin
    max_samples -= max_samples / 50;
    
    // Apply system limits
    if (max_samples < JL_LA_MIN_SAMPLES) {
        max_samples = JL_LA_MIN_SAMPLES;
    } else if (max_samples > JL_LA_MAX_SAMPLES_LIMIT) {
        max_samples = JL_LA_MAX_SAMPLES_LIMIT;
    }
    
    // Calculate actual buffer sizes
    size_t digital_buffer_size = max_samples * digital_bytes_per_sample;
    size_t analog_buffer_size_calc = max_samples * analog_bytes_per_sample;
    size_t total_needed = digital_buffer_size + analog_buffer_size_calc;
    
    DEBUG_LA_PRINTF("◆ CALCULATION: %d channels, %zu usable memory\n", 
                 driver_analog_channels, usable_memory);
    DEBUG_LA_PRINTF("◆ REQUIREMENTS: %lu max samples, %zu total bytes needed\n",
                 max_samples, total_needed);
    
    // Step 3: Verify we can allocate what we calculated
    if (total_needed > usable_memory) {
        DEBUG_LA_PRINTF("◆ ERROR: Calculated requirements exceed available memory!\n");
        return false;
    }
    
    // Step 4: Immediately allocate digital buffer
    digital_buffer = (uint8_t*)malloc(digital_buffer_size);
    if (!digital_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Failed to allocate %zu byte digital buffer\n", digital_buffer_size);
        return false;
    }
    memset(digital_buffer, 0, digital_buffer_size);
    
    // Step 5: Allocate analog buffer if needed
    if (analog_buffer_size_calc > 0) {
        analog_buffer = (uint16_t*)malloc(analog_buffer_size_calc);
        if (!analog_buffer) {
            DEBUG_LA_PRINTF("◆ ERROR: Failed to allocate %zu byte analog buffer\n", analog_buffer_size_calc);
            free(digital_buffer);
            digital_buffer = nullptr;
            return false;
        }
        memset(analog_buffer, 0, analog_buffer_size_calc);
        analog_buffer_size = analog_buffer_size_calc;
    } else {
        analog_buffer = nullptr;
        analog_buffer_size = 0;
    }
    
    // Step 6: Update all global state consistently
    jl_la_max_samples = max_samples;
    analog_chan_count = driver_analog_channels;
    
    // Adjust sample_count if it exceeds new maximum
    if (sample_count > max_samples) {
        sample_count = max_samples;
        DEBUG_LA_PRINTF("◆ Adjusted sample count to new maximum: %lu\n", sample_count);
    }
    
    DEBUG_LA_PRINTF("◆ ATOMIC SUCCESS: %zu digital + %zu analog = %zu total bytes allocated\n",
                 digital_buffer_size, analog_buffer_size_calc, total_needed);
    DEBUG_LA_PRINTF("◆ STATE: max_samples=%lu, sample_count=%lu\n",
                 jl_la_max_samples, sample_count);
    
    return true;
}

void invalidateBufferCache() {
    // This function is a placeholder for a more complex caching system if needed.
    // For now, it just serves as a clear entry point for recalculation.
}

bool allocateLogicAnalyzerResources() {
    DEBUG_LA_PRINTF("◆ Allocating logic analyzer resources using atomic system...\n");
    
    // Use the atomic calculate+allocate function (handles memory allocation)
    if (!calculateAndAllocateBuffers()) {
        DEBUG_LA_PRINTF("◆ ERROR: Atomic allocation failed\n");
        return false;
    }
    
    // Complete PIO/DMA reset to ensure clean state
    DEBUG_LA_PRINTLN("◆ Performing complete PIO/DMA reset...");
    
    // Clean up any existing resources
    if (pio != pio0 && offset != (uint)-1) {
        pio_remove_program(pio, &la_pio_program_fast, offset);
        DEBUG_LA_PRINTF("◆ Removed program from PIO%d offset %d\n", pio_get_index(pio), offset);
        offset = -1;
    }
    if (pio != pio0 && sm != (uint)-1) {
        pio_sm_set_enabled(pio, sm, false);  // Disable SM first
        pio_sm_unclaim(pio, sm);
        DEBUG_LA_PRINTF("◆ Unclaimed PIO%d SM%d\n", pio_get_index(pio), sm);
        sm = -1;
    }
    if (dma_chan != -1) {
        dma_channel_abort(dma_chan);  // Stop any active transfers
        dma_channel_unclaim(dma_chan);
        DEBUG_LA_PRINTF("◆ Unclaimed DMA channel %d\n", dma_chan);
        dma_chan = -1;
    }
    
    // Reset PIO pointers
    pio = pio0;  // Default to pio0
    
    DEBUG_LA_PRINTLN("◆ PIO/DMA reset complete");
    
    // Memory is now allocated - allocate PIO/DMA resources
    
    // Allocate PIO resources with detailed debugging
    PIO pio_instances[] = { pio0, pio1 };
    bool pio_allocated = false;
    
    DEBUG_LA_PRINTLN("◆ Attempting PIO allocation...");
    
    for (int i = 0; i < 2 && !pio_allocated; i++) {
        pio = pio_instances[i];
        DEBUG_LA_PRINTF("◆ Trying PIO%d...\n", pio_get_index(pio));
        
        int sm_try = pio_claim_unused_sm(pio, false);
        if (sm_try < 0) {
            DEBUG_LA_PRINTF("◆ PIO%d: No available state machines\n", pio_get_index(pio));
            continue;
        }
        DEBUG_LA_PRINTF("◆ PIO%d: Claimed SM%d\n", pio_get_index(pio), sm_try);
        
        if (!pio_can_add_program(pio, &la_pio_program_fast)) {
            DEBUG_LA_PRINTF("◆ PIO%d: Cannot add program, unclaiming SM%d\n", pio_get_index(pio), sm_try);
            pio_sm_unclaim(pio, sm_try);  // Clean up claimed SM
            continue;
        }
        
        uint offset_try = pio_add_program(pio, &la_pio_program_fast);
        // Cast to signed to check for -1
        if ((int)offset_try < 0) {
            DEBUG_LA_PRINTF("◆ PIO%d: Failed to add program, unclaiming SM%d\n", pio_get_index(pio), sm_try);
            pio_sm_unclaim(pio, sm_try);  // Clean up claimed SM
            continue;
        }
        
        sm = sm_try;
        offset = offset_try;
        pio_allocated = true;
        DEBUG_LA_PRINTF("◆ SUCCESS: PIO%d SM%d offset=%d\n", pio_get_index(pio), sm, offset);
    }
    
    if (!pio_allocated) {
        DEBUG_LA_PRINTLN("◆ ERROR: Failed to allocate PIO resources after trying all instances");
        return false;
    }
    
    // Allocate DMA channel
    int dma = dma_claim_unused_channel(false);
    if (dma < 0) {
        DEBUG_LA_PRINTLN("◆ ERROR: No available DMA channels");
        return false;
    }
    dma_chan = dma;
    DEBUG_LA_PRINTF("◆ SUCCESS: Claimed DMA channel %d\n", dma_chan);
    
    // Memory buffers already allocated by calculateAndAllocateBuffers()
    // Just verify they exist
    if (!digital_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Digital buffer not allocated by atomic function\n");
        return false;
    }
    
    if (analog_chan_count > 0 && !analog_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Analog buffer not allocated by atomic function\n");
        return false;
    }
    
    DEBUG_LA_PRINTF("◆ RESOURCE ALLOCATION COMPLETE: PIO%d SM%d offset=%d, DMA=%d\n",
                 pio_get_index(pio), sm, offset, dma_chan);
    
    return true;
}

void releaseLogicAnalyzerResources() {
    DEBUG_LA_PRINTLN("◆ Releasing all logic analyzer resources...");
    
    // Clean up PIO program and state machine
    if (pio && offset != (uint)-1) {
        pio_remove_program(pio, &la_pio_program_fast, offset);
        DEBUG_LA_PRINTF("◆ Removed PIO program from PIO%d offset %d\n", pio_get_index(pio), offset);
        offset = -1;
    }
    
    if (pio && sm != (uint)-1) {
        pio_sm_set_enabled(pio, sm, false);
        pio_sm_unclaim(pio, sm);
        DEBUG_LA_PRINTF("◆ Unclaimed PIO%d SM%d\n", pio_get_index(pio), sm);
        sm = -1;
    }
    
    // Clean up DMA channel
    if (dma_chan != -1) {
        dma_channel_abort(dma_chan);
        dma_channel_unclaim(dma_chan);
        DEBUG_LA_PRINTF("◆ Unclaimed DMA channel %d\n", dma_chan);
        dma_chan = -1;
    }
    
    // Clean up memory buffers
    if (digital_buffer) {
        free(digital_buffer);
        digital_buffer = nullptr;
    }
    if (analog_buffer) {
        free(analog_buffer);
        analog_buffer = nullptr;
    }
    
    DEBUG_LA_PRINTLN("◆ All logic analyzer resources released");
}

void sendJumperlessHeader() {
        // Implementation for sending Jumperless header
        
        DEBUG_LA_PRINTLN("◆ Sending Jumperless header...");
        
        if (!la_usb_connected()) {
            DEBUG_LA_PRINTLN("◆ ERROR: USB not connected");
            return;
        }
        
        la_usb_write(JUMPERLESS_RESP_HEADER);
        
        struct {
            char magic[8];
            uint8_t version;
            uint8_t capture_mode;
            uint8_t max_digital_channels;
            uint8_t max_analog_channels;
            uint32_t sample_rate;
            uint32_t sample_count;
            uint32_t digital_channel_mask;
            uint32_t analog_channel_mask;
            uint8_t bytes_per_sample;
            uint8_t digital_bytes_per_sample;
            uint8_t analog_bytes_per_sample;
            uint8_t adc_resolution_bits;
            uint32_t trigger_channel_mask;
            uint32_t trigger_pattern;
            uint32_t trigger_edge_mask;
            uint32_t pre_trigger_samples;
            float analog_voltage_range;
            uint64_t max_sample_rate;
            uint64_t max_memory_depth;
            uint8_t supports_triggers;
            uint8_t supports_compression;
            uint8_t supported_modes;
            char firmware_version[16];
            char device_id[16];
            uint32_t checksum;
        } __attribute__((packed)) header;
        
        memset(&header, 0, sizeof(header));
        strncpy(header.magic, "$JLDATA", sizeof(header.magic));
        header.version = 2;
        header.capture_mode = current_la_mode;
        header.max_digital_channels = 8;  // Fixed: Match actual GPIO count (GPIO 1-8) and driver expectations
        header.max_analog_channels = 14;  // Updated to match actual hardware capability
        header.sample_rate = sample_rate;
        header.sample_count = sample_count;
        header.digital_channel_mask = 0xFF;
        header.analog_channel_mask = analog_mask;
        
        if (current_la_mode == LA_MODE_DIGITAL_ONLY) {
            header.bytes_per_sample = 3;
            header.digital_bytes_per_sample = 3;
            header.analog_bytes_per_sample = 0;
        } else {
            // Efficient packing: 3 digital + (2 × enabled analog channels) + 1 EOF
            header.bytes_per_sample = 32;
            header.digital_bytes_per_sample = 3;
            header.analog_bytes_per_sample = analog_chan_count * 2;
        }
        
        header.adc_resolution_bits = 12;
        header.analog_voltage_range = 18.28f;
        header.max_sample_rate = 10000000;
        header.max_memory_depth = jl_la_max_samples;
        header.supported_modes = 0x03;  // Digital + Mixed
        strncpy(header.firmware_version, "RP2350-v2.0", sizeof(header.firmware_version));
        strncpy(header.device_id, "Jumperless-LA", sizeof(header.device_id));
        
        // Calculate checksum
        uint8_t* header_bytes = (uint8_t*)&header;
        uint32_t checksum = 0;
        for (size_t i = 0; i < sizeof(header) - sizeof(header.checksum); i++) {
            checksum ^= header_bytes[i];
        }
        header.checksum = checksum;
        
        uint16_t header_len = sizeof(header);
        la_usb_write_buffer((uint8_t*)&header_len, 2);
        la_usb_write_buffer((uint8_t*)&header, sizeof(header));
        la_usb_flush();
        
        DEBUG_LA_PRINTF("◆ Header sent: %d samples, %d bytes/sample, %d analog channels\n", 
                     sample_count, header.bytes_per_sample, analog_chan_count);
    
}

void sendStatusResponse(uint8_t status) {
    DEBUG_LA_PRINTF("◆ Sending status response: 0x82 with status 0x%02X\n", status);
    
    // Ensure USB is connected before sending
    if (!la_usb_connected()) {
        DEBUG_LA_PRINTLN("◆ ERROR: Cannot send status response - USB disconnected");
        return;
    }
    
    la_usb_write(JUMPERLESS_RESP_STATUS);  // 0x82
    uint16_t resp_len = 1;
    la_usb_write_buffer((uint8_t*)&resp_len, 2);
    la_usb_write(status);
    la_usb_flush();
    
    // Add delay to ensure response is fully transmitted before any subsequent data
    delayMicroseconds(2000);  // 2ms delay for proper protocol separation
    
    DEBUG_LA_PRINTLN("◆ Status response sent and flushed");
}

void sendErrorResponse(uint8_t error_code) {
    la_usb_write(JUMPERLESS_RESP_ERROR);
    uint16_t err_len = 1;
    la_usb_write_buffer((uint8_t*)&err_len, 2);
    la_usb_write(error_code);
    la_usb_flush();
}

void sendSUMPID() {
    const char sump_id[] = "1SLO";
    la_usb_write_buffer((const uint8_t*)sump_id, 4);
    la_usb_flush();
    DEBUG_LA_PRINTLN("◆ Sent SUMP ID: 1SLO");
}

void sendCaptureData() {
    DEBUG_LA_PRINTF("◆ SEND: Starting data transmission...\n");
    DEBUG_LA_PRINTF("◆ SEND: USB connected=%s\n", la_usb_connected() ? "YES" : "NO");
    DEBUG_LA_PRINTF("◆ SEND: Mode=%d, analog_chan_count=%d\n", current_la_mode, analog_chan_count);
    
    la_usb_write(JUMPERLESS_RESP_DATA);
    
    // Jumperless always uses mixed-signal format (32 bytes per sample)
    uint32_t total_data_size = sample_count * sizeof(MixedSignalSample);
    DEBUG_LA_PRINTF("◆ SEND: Mixed-signal mode - %lu samples × %zu bytes = %lu total bytes\n", 
                 sample_count, sizeof(MixedSignalSample), total_data_size);
    
    la_usb_write_buffer((uint8_t*)&total_data_size, 4);
    DEBUG_LA_PRINTF("◆ SEND: Size header sent (4 bytes)\n");
    
    DEBUG_LA_PRINTF("◆ SEND: Converting and sending data on-the-fly...\n");
    
    // Always send mixed-signal format (32 bytes per sample)
    for (uint32_t i = 0; i < sample_count; i++) {
        MixedSignalSample pulseview_sample;
        
        // Convert from separate storage to PulseView format
        pulseview_sample.gpio_data = digital_buffer[i];
        pulseview_sample.uart_data = 0; // No UART data for now
        pulseview_sample.format_marker = MIXED_SIGNAL_MARKER; // 0xDA
        
        // Fill all 14 analog channels
        for (int j = 0; j < 14; j++) {
            if (j < analog_chan_count && analog_buffer) {
                // Real ADC data with LSB validity flag
                uint16_t adc_value = analog_buffer[i * analog_chan_count + j];
                pulseview_sample.analog_data[j] = (adc_value & 0xFFFE) | 0x0001; // Set LSB to 1 for real data
            } else {
                // Unused channels set to 0 (LSB=0 indicates dummy data)
                pulseview_sample.analog_data[j] = 0x0000;
            }
        }
        
        pulseview_sample.eof_marker = ANALOG_EOF_MARKER; // 0xA0
        
        // Send this converted sample
        la_usb_write_buffer((uint8_t*)&pulseview_sample, sizeof(MixedSignalSample));
        
        if (i % 1000 == 0) {
            DEBUG_LA_PRINTF("◆ SEND: Converted sample %lu/%lu\n", i, sample_count);
        }
    }
    
    DEBUG_LA_PRINTF("◆ SEND: All %lu samples converted and sent (%lu bytes)\n", sample_count, total_data_size);
    
    la_usb_flush();
    DEBUG_LA_PRINTF("◆ SEND: USB flush completed\n");
}

void handleResetCommand() {
    DEBUG_LA_PRINTLN("◆ RESET command - cleaning up resources");
    releaseLogicAnalyzerResources();
    capture_state = STATE_IDLE;
    run_capture = false;
    DEBUG_LA_PRINTLN("◆ RESET command processed");
    sendStatusResponse(JUMPERLESS_STATUS_OK); // Acknowledge
}

void handleIdCommand() {
    sendSUMPID();
}

void handleSetModeCommand(LogicAnalyzerMode mode) {
    current_la_mode = mode;
    invalidateBufferCache();
    DEBUG_LA_PRINTF("◆ Mode configured: %d\n", mode);
}

void handleSetChannelsCommand(uint32_t digital_mask_data, uint32_t analog_mask_data) {
    DEBUG_LA_PRINTF("◆ Processing channel configuration: digital=0x%08X, analog=0x%08X\n", 
                   digital_mask_data, analog_mask_data);
    
    // Update digital channels (8 GPIO channels)
    digital_mask = digital_mask_data & 0xFF;  // Only use bottom 8 bits for GPIO
    digital_chan_count = __builtin_popcount(digital_mask);
    
    // Update analog channels 
    analog_mask = analog_mask_data;
    analog_chan_count = __builtin_popcount(analog_mask);
    
    // Force mixed-signal mode - Jumperless should never be digital-only
    current_la_mode = LA_MODE_MIXED_SIGNAL;
    
    invalidateBufferCache();
    
    DEBUG_LA_PRINTF("◆ Channels configured: %d digital (mask=0x%02X), %d analog (mask=0x%08X)\n", 
                   digital_chan_count, digital_mask, analog_chan_count, analog_mask);
    DEBUG_LA_PRINTF("◆ Forced to mixed-signal mode (Jumperless never uses digital-only)\n");
    
    sendStatusResponse(JUMPERLESS_STATUS_OK);
    delay(50);
    sendJumperlessHeader();
}

void handleArmCommand() {
    DEBUG_LA_PRINTLN("◆ ARM command - preparing for capture");
    DEBUG_LA_PRINTF("◆ ARM: Current state: capture_state=%d, run_capture=%s\n", 
                   capture_state, run_capture ? "true" : "false");
    DEBUG_LA_PRINTF("◆ ARM: Config: rate=%lu Hz, samples=%lu, mode=%d\n", 
                   sample_rate, sample_count, current_la_mode);
    DEBUG_LA_PRINTF("◆ ARM: Channels: analog_mask=0x%08X (%d channels)\n", 
                   analog_mask, analog_chan_count);

    // Defer allocation until ARM command is received
    DEBUG_LA_PRINTLN("◆ ARM: Starting resource allocation...");
    if (!allocateLogicAnalyzerResources()) {
        DEBUG_LA_PRINTLN("◆ ARM ERROR: Failed to allocate resources on ARM");
        sendErrorResponse(0xEE); // Send a custom error code
        return;
    }

    capture_state = STATE_ARMED;
    run_capture = false;  // Don't auto-start - wait for RUN command
    DEBUG_LA_PRINTLN("◆ ARM SUCCESS: Resources allocated, waiting for RUN command...");
    sendStatusResponse(JUMPERLESS_STATUS_OK); // Send success status (0x00)
}

void handleRunCommand() {
    DEBUG_LA_PRINTLN("◆ RUN command received");
    
    if (capture_state != STATE_ARMED) {
        DEBUG_LA_PRINTF("◆ RUN ERROR: Device not armed (state=%d)\n", capture_state);
        sendErrorResponse(JUMPERLESS_STATUS_ERROR);
        return;
    }
    
    DEBUG_LA_PRINTLN("◆ RUN: Starting capture...");
    run_capture = true;  // Start the capture
    sendStatusResponse(JUMPERLESS_STATUS_OK); // Send success status (0x00)
}

void handleConfigureCommand(uint32_t new_sample_rate, uint32_t new_sample_count) {
     if (new_sample_rate > 0) {
        if (new_sample_rate > MAX_SAMPLE_RATE_HZ) {
            sample_rate = MAX_SAMPLE_RATE_HZ;
        } else {
            sample_rate = new_sample_rate;
        }
    }

    if (new_sample_count > 0) {
        if (new_sample_count < JL_LA_MIN_SAMPLES) {
            sample_count = JL_LA_MIN_SAMPLES;
        } else if (new_sample_count > jl_la_max_samples && jl_la_max_samples > 0) {
            sample_count = jl_la_max_samples;
        } else {
            sample_count = new_sample_count;
        }
    }
    
    DEBUG_LA_PRINTF("◆ Config: rate=%lu Hz, samples=%lu\n",
                     sample_rate, sample_count);

    invalidateBufferCache();
}

void handleSetSamplesCommand(uint32_t new_sample_count) {
    if (new_sample_count > 0) {
        if (new_sample_count < JL_LA_MIN_SAMPLES) {
            sample_count = JL_LA_MIN_SAMPLES;
        } else if (new_sample_count > jl_la_max_samples && jl_la_max_samples > 0) {
            sample_count = jl_la_max_samples;
        } else {
            sample_count = new_sample_count;
        }
        DEBUG_LA_PRINTF("◆ Sample count set: %lu\n", sample_count);
        invalidateBufferCache();
    }
}

void handleSetTriggerCommand(uint8_t trigger_type, uint32_t mask, uint32_t value, uint32_t edge) {
    DEBUG_LA_PRINTF("◆ Set Trigger command ignored (not implemented)\n");
}

void handleClearTriggerCommand() {
    DEBUG_LA_PRINTF("◆ Clear Trigger command ignored (not implemented)\n");
}

void processPulseViewCommand(uint8_t cmd) {
    DEBUG_LA_PRINTF("◆ PulseView Command: 0x%02X\n", cmd);
    
    switch (cmd) {
                 case JUMPERLESS_CMD_RESET:
             capture_state = STATE_IDLE;
             run_capture = false;
             DEBUG_LA_PRINTLN("◆ Device reset");
             sendStatusResponse(JUMPERLESS_STATUS_OK);
             break;
            
        case JUMPERLESS_CMD_ID:
            sendSUMPID();
            break;
            
        case JUMPERLESS_CMD_GET_HEADER:
            sendJumperlessHeader();
            break;
            
        case JUMPERLESS_CMD_SET_CHANNELS: {
            if (la_usb_available() >= 8) { // Driver sends 8 bytes: 4 digital + 4 analog
                uint32_t digital_mask_data = 0;
                uint32_t analog_mask_data = 0;
                la_usb_read_buffer((uint8_t*)&digital_mask_data, 4);
                la_usb_read_buffer((uint8_t*)&analog_mask_data, 4);
                DEBUG_LA_PRINTF("◆ SET_CHANNELS: digital_mask=0x%08X, analog_mask=0x%08X\n", 
                               digital_mask_data, analog_mask_data);
                handleSetChannelsCommand(digital_mask_data, analog_mask_data);
            }
            break;
        }
        
        case JUMPERLESS_CMD_ARM:
            handleArmCommand();
            break;
            
        case JUMPERLESS_CMD_RUN:
            handleRunCommand();
            break;
            
                 case JUMPERLESS_CMD_CONFIGURE: {
             if (la_usb_available() >= 8) {
                 uint32_t rate, count;
                 la_usb_read_buffer((uint8_t*)&rate, 4);
                 la_usb_read_buffer((uint8_t*)&count, 4);
                 handleConfigureCommand(rate, count);
                 sendStatusResponse(JUMPERLESS_STATUS_OK);
             }
             break;
         }
            
                 case JUMPERLESS_CMD_SET_MODE: {
             if (la_usb_available() >= 1) {
                 uint8_t new_mode = la_usb_read();
                 handleSetModeCommand((LogicAnalyzerMode)new_mode);
                 sendStatusResponse(JUMPERLESS_STATUS_OK);
                 delay(10);
                 sendJumperlessHeader();
             }
             break;
         }
        
                 case JUMPERLESS_CMD_SET_TRIGGER:
         case JUMPERLESS_CMD_CLEAR_TRIGGER:
             // Not implemented, but acknowledge
             sendStatusResponse(JUMPERLESS_STATUS_OK);
             break;
            
        default:
            DEBUG_LA_PRINTF("◆ Unknown PulseView command: 0x%02X\n", cmd);
            sendErrorResponse(0xFF);
            break;
    }
}

void processCommand(uint8_t cmd) {
    DEBUG_LA_PRINTF("◆ Command: 0x%02X\n", cmd);
    
    // Handle PulseView/Enhanced protocol commands (0xA0 - 0xAB)
    if (cmd >= 0xA0 && cmd <= 0xAB) {
        processPulseViewCommand(cmd);
        return;
    }
    
    // Handle SUMP Protocol Commands
    switch (cmd) {
        case SUMP_CMD_SET_SAMPLE_RATE: {
            if (la_usb_available() >= 4) {
                 uint32_t rate = 0;
                 la_usb_read_buffer((uint8_t*)&rate, 4);
                 handleConfigureCommand(rate, sample_count);
            }
            break;
        }
        
        case SUMP_CMD_SET_READ_DELAY_COUNT: {
            if (la_usb_available() >= 4) {
                uint32_t count = 0;
                la_usb_read_buffer((uint8_t*)&count, 4);
                handleSetSamplesCommand(count);
            }
            break;
        }
        
        case SUMP_CMD_SET_FLAGS:
        case SUMP_CMD_SET_TRIGGER_MASK:
        case SUMP_CMD_SET_TRIGGER_VALUES:
        case SUMP_CMD_SET_TRIGGER_CONFIG:
            // Consume 4 bytes for these commands but do nothing
            if (la_usb_available() >= 4) {
                uint8_t dummy[4];
                la_usb_read_buffer(dummy, 4);
            }
            break;
            
        default:
            DEBUG_LA_PRINTF("◆ Unknown SUMP command: 0x%02X\n", cmd);
            break;
    }
}

void setupLogicAnalyzer() {
    // No setup needed here, resources are allocated on ARM
    DEBUG_LA_PRINTLN("◆ Logic analyzer initialized, resources will be allocated on ARM");
}

void handleLogicAnalyzer() {
    while (la_usb_available()) {
        uint8_t cmd = la_usb_read();
        processCommand(cmd);
    }

    if (run_capture || capture_state == STATE_DATA_READY) {
        static uint32_t samples_captured = 0;
        
        switch (capture_state) {
            case STATE_IDLE:
                // Nothing to do
                break;
            
            case STATE_ARMED: {
                 DEBUG_LA_PRINTLN("◆ Capture armed, starting...");
                 samples_captured = 0;

                 // Set up the ADC
                 DEBUG_LA_PRINTF("◆ Configuring ADC for %d channels starting at pin %d...\n", analog_chan_count, 26 + CAPTURE_CHANNEL_FIRST);
                 adc_init();
                 for (int i = 0; i < analog_chan_count; i++) {
                     adc_gpio_init(26 + CAPTURE_CHANNEL_FIRST + i);
                     DEBUG_LA_PRINTF("◆ ADC pin %d initialized\n", 26 + CAPTURE_CHANNEL_FIRST + i);
                 }
                 uint32_t adc_mask = 0;
                 if (analog_chan_count > 0) {
                    adc_mask = (1 << analog_chan_count) -1;
                 }
                 adc_set_round_robin(adc_mask);
                 DEBUG_LA_PRINTF("◆ ADC round-robin mask: 0x%02X\n", adc_mask);
                 adc_fifo_setup(
                     true,    // Write each completed conversion to the sample FIFO
                     false,   // Don't start DMA yet
                     1,       // DREQ asserted when at least 1 sample is in the FIFO
                     false,   // Don't enable error bit
                     false    // 8-bit samples
                 );
                 
                 // Configure PIO for digital capture (resources already allocated)
                 DEBUG_LA_PRINTF("◆ Configuring PIO%d SM%d...\n", pio_get_index(pio), sm);
                 la_pio_program_init(pio, sm, offset, 0); // Logic analyzer input on GPIO0
                 pio_sm_set_enabled(pio, sm, true);
                 DEBUG_LA_PRINTF("◆ PIO configured: pins=%d, rate=%lu Hz\n", 0, sample_rate);
 
                 capture_state = STATE_CAPTURING;
                 break;
            }
            
            case STATE_CAPTURING: {
                if (samples_captured < sample_count) {
                    // For simplicity, we just poll the PIO and ADC FIFOs
                    bool pio_data_available = !pio_sm_is_rx_fifo_empty(pio, sm);
                    bool adc_data_available = !adc_fifo_is_empty();

                    if (pio_data_available) { 
                        uint8_t digital_data = pio_sm_get(pio, sm);

                        // Store digital data in the digital_buffer
                        digital_buffer[samples_captured] = digital_data;
                        
                        // Read analog data from all enabled channels
                        if (analog_chan_count > 0) {
                            if(adc_data_available) {
                               uint16_t* analog_sample_ptr = &analog_buffer[samples_captured * analog_chan_count];
                               for (int j = 0; j < analog_chan_count; j++) {
                                  analog_sample_ptr[j] = adc_read();
                               }
                            }
                        }
                        
                         samples_captured++;
                         
                         // Add debug output for samples
                         if (samples_captured <= 10 || samples_captured % 1000 == 0) {
                             DEBUG_LA_PRINTF("◆ Sample %lu: digital=0x%02X (pio_avail=%s), analog=[%d,%d,%d]\n", 
                                          samples_captured, digital_data, pio_data_available ? "YES" : "NO",
                                         analog_buffer ? analog_buffer[(samples_captured-1) * analog_chan_count] : -1,
                                         analog_buffer && analog_chan_count > 1 ? analog_buffer[(samples_captured-1) * analog_chan_count + 1] : -1,
                                         analog_buffer && analog_chan_count > 2 ? analog_buffer[(samples_captured-1) * analog_chan_count + 2] : -1);
                         }
                    }
                } else {
                    DEBUG_LA_PRINTF("◆ Capture finished: %lu samples\n", samples_captured);
                    run_capture = false;
                    capture_state = STATE_DATA_READY;
                }
                break;
            }
            
            case STATE_DATA_READY: {
                 DEBUG_LA_PRINTF("◆ DATA_READY: Preparing to send %lu samples\n", sample_count);
                 DEBUG_LA_PRINTF("◆ DATA_READY: Stored as %zu bytes, will transmit as %zu bytes\n", 
                              (sample_count * (1 + analog_chan_count * 2)), sample_count * sizeof(MixedSignalSample));
                 DEBUG_LA_PRINTF("◆ Buffer check: digital_buffer=%p, analog_buffer=%p\n",
                              digital_buffer, analog_buffer);
                               
                  sendCaptureData();
                  // Don't release resources immediately - let RESET command handle cleanup
                  capture_state = STATE_IDLE;
                  run_capture = false;
                  DEBUG_LA_PRINTLN("◆ DATA_READY: Data sent, waiting for next command...");
                  break;
             }
        }
    }
} 
#endif