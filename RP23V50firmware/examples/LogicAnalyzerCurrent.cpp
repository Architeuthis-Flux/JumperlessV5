// SPDX-License-Identifier: MIT
// Jumperless Logic Analyzer Implementation
// Cleaned and reorganized - only contains functions actually used

#include "LogicAnalyzer.h"
#include "JumperlessDefines.h"
#include "Peripherals.h"
#include "ArduinoStuff.h"
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/adc.h>
#include <hardware/gpio.h>
#include <hardware/timer.h>
#include <Arduino.h>
#include "Graphics.h"
// Debug output control
bool debugLA = false;

// Debug macros
#define DEBUG_LA_PRINTF(...) do { if (debugLA) { changeTerminalColor(219); Serial.printf(__VA_ARGS__); changeTerminalColor(0); } } while(0)
#define DEBUG_LA_PRINTLN(...) do { if (debugLA) { changeTerminalColor(217); Serial.println(__VA_ARGS__); changeTerminalColor(0); } } while(0)

// =============================================================================
// TYPE DEFINITIONS
// =============================================================================

typedef enum {
    TRIGGER_NONE = 0,
    TRIGGER_EXTERNAL = 1,
    TRIGGER_GPIO = 2,
    TRIGGER_THRESHOLD = 3
} TriggerMode;

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================

// Core state variables
bool la_initialized = false;
bool la_enabled = false;
volatile bool logicAnalyzing = false;
volatile bool la_capturing = false;
la_state_t la_capture_state = JL_LA_STOPPED;
LogicAnalyzerMode current_la_mode = LA_MODE_MIXED_SIGNAL;
bool enhanced_mode = false;
bool trigger_la = false;

// Sample configuration
uint32_t sample_rate = 1000000;
uint32_t sample_count = JL_LA_DEFAULT_SAMPLES;
uint32_t analog_mask = 0x00;
uint8_t analog_chan_count = 0;

// Hardware resources
PIO la_pio = nullptr;
uint la_sm = (uint)-1;
uint la_prog_offset = (uint)-1;
int la_dma_chan = -1;

// Buffer management
uint32_t *la_buffer = nullptr;
uint32_t jl_la_buffer_size = 0;
uint32_t jl_la_max_samples = 0;
uint16_t *analog_buffer = nullptr;
size_t analog_buffer_size = 0;

// Timing and trigger
uint32_t last_activity_ms = 0;
TriggerMode trigger_mode = TRIGGER_NONE;

// Analog sampling system
static uint32_t adc_dma_channels[8] = {0};
static uint16_t *adc_channel_buffers[8] = {nullptr};
static volatile uint32_t adc_samples_captured = 0;
static volatile bool adc_dma_capture_active = false;
static repeating_timer_t adc_dma_timer;
static volatile uint32_t adc_dma_timer_ticks = 0;
static uint16_t last_good_adc_value[8] = {0};
static bool adc_oversampling_mode = false;
static uint32_t adc_actual_sample_rate = 0;
static uint32_t adc_duplication_factor = 1;
static bool capture_used_oversampling = false;
static uint32_t capture_duplication_factor = 1;

// Protocol constants
#define JUMPERLESS_CMD_RESET        0x00
#define JUMPERLESS_CMD_RUN          0x01
#define JUMPERLESS_CMD_ID           0x02
#define JUMPERLESS_CMD_GET_HEADER   0x03
#define JUMPERLESS_CMD_SET_CHANNELS 0x04
#define JUMPERLESS_CMD_ARM          0x05
#define JUMPERLESS_CMD_CONFIGURE    0x07

#define JUMPERLESS_RESP_DATA        0x81
#define JUMPERLESS_RESP_STATUS      0x82

#define SUMP_SET_DIVIDER            0x80
#define SUMP_SET_READ_DELAY         0x81
#define SUMP_SET_FLAGS              0x82
#define SUMP_SET_TRIGGER_MASK_0     0xC0
#define SUMP_SET_TRIGGER_VALUE_0    0xC1

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

bool allocateLogicAnalyzerResources();
void processCommand(uint8_t cmd);
void processPulseViewCommand(uint8_t cmd);
bool captureAnalogData();
bool checkTriggerCondition();
bool setupCapture();
bool startCapture();
bool isCaptureDone();
void sendCaptureData();
void clearTrigger();
void handleConnectionStateChange(bool connected);
void updateLastActivity();
bool allocateAllLogicAnalyzerBuffers();
void sendSUMPID();
void sendJumperlessHeader();
void sendStatusResponse(uint8_t status);
void sendErrorResponse(uint8_t error_code);
void sendDigitalOnlyData();
void sendMixedSignalData();
void releaseLogicAnalyzerResources();

// =============================================================================
// PIO PROGRAM - Simple Logic Analyzer
// =============================================================================

// Simple PIO program for logic analysis - reads 8 pins continuously
const uint16_t la_pio_program_instructions[] = {
    0x4020, // in pins, 32
};

const struct pio_program la_pio_program_fast = {
    .instructions = la_pio_program_instructions,
    .length = 1,
    .origin = -1,
};

// PIO initialization function
static inline void la_pio_program_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_in_pins(&c, pin);
    sm_config_set_wrap(&c, offset, offset);
    sm_config_set_clkdiv(&c, 1.0);
    sm_config_set_in_shift(&c, false, true, 32);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);
    
    pio_sm_init(pio, sm, offset, &c);
}

// =============================================================================
// ENTRY POINTS - MAIN INTERFACE FUNCTIONS
// =============================================================================

void setupLogicAnalyzer() {
    DEBUG_LA_PRINTLN("◆ Setting up Logic Analyzer...");
    
    // Configure GPIO pins
    for (int i = 0; i < JL_LA_PIN_COUNT; i++) {
        gpio_init(JL_LA_PIN_BASE + i);
    }
    
    // Default to mixed-signal mode with 8 digital + 5 analog channels
    current_la_mode = LA_MODE_MIXED_SIGNAL;
    analog_mask = 0x1F; // Enable analog channels 0-4 (5 channels)
    analog_chan_count = 5;
    
    la_initialized = true;
    la_enabled = true;
    la_capture_state = JL_LA_STOPPED;
    
    DEBUG_LA_PRINTF("◆ Logic Analyzer ready - pins %d-%d (8 digital + 5 analog channels)\n", 
                 JL_LA_PIN_BASE, JL_LA_PIN_BASE + JL_LA_PIN_COUNT - 1);
}

void handleLogicAnalyzer() {
    if (!la_initialized || !la_enabled) return;
    logicAnalyzing = true;
    bool usb_connected = la_usb_connected();
    handleConnectionStateChange(usb_connected);
    
    if (!usb_connected) {
        enhanced_mode = false;
        return;
    }
    
    // Process commands
    if (la_usb_available()) {
        uint8_t cmd = la_usb_read();
        
        // Filter valid commands
        if ((cmd >= 0xA0 && cmd <= 0xAB) ||
            (cmd >= 0x80 && cmd <= 0x82) ||
            (cmd >= 0xC0 && cmd <= 0xCF) ||
            (cmd >= 0xD0 && cmd <= 0xDF)) {
            if (!enhanced_mode) {
                enhanced_mode = true;
                DEBUG_LA_PRINTLN("◆ Protocol detected");
            }
            processCommand(cmd);
        }
        
        updateLastActivity();
    }
    
    // Handle capture completion
    if (la_capture_state == JL_LA_TRIGGERED && isCaptureDone()) {
        DEBUG_LA_PRINTLN("◆ Capture completed");
        
        // For mixed-signal mode, capture analog data after digital capture is done
        if (current_la_mode == LA_MODE_MIXED_SIGNAL || current_la_mode == LA_MODE_ANALOG_ONLY) {
            DEBUG_LA_PRINTLN("◆ Starting analog data capture...");
            if (captureAnalogData()) {
                DEBUG_LA_PRINTLN("◆ Analog capture completed");
            } else {
                DEBUG_LA_PRINTLN("◆ ERROR: Analog capture failed");
            }
        }
        
        sendCaptureData();
        logicAnalyzing = false;
        
        // Don't clean up resources automatically - let them persist for next capture
        // Resources will be cleaned up on RESET command or when starting new capture
        DEBUG_LA_PRINTLN("◆ Capture complete - resources kept for next capture");
    }
    
    // Check for trigger conditions when armed
    if (la_capture_state == JL_LA_ARMED && !la_capturing) {
        if (checkTriggerCondition()) {
            DEBUG_LA_PRINTLN("◆ Trigger condition met - starting capture");
            
            // Safety check: ensure valid sample count
            if (sample_count == 0) {
                sample_count = JL_LA_DEFAULT_SAMPLES;
                DEBUG_LA_PRINTF("◆ SAFETY: Zero sample count, using default %lu\n", sample_count);
            }
            
            DEBUG_LA_PRINTF("◆ Auto-triggered capture: rate=%lu Hz, samples=%lu, mode=%d\n", 
                         sample_rate, sample_count, current_la_mode);
            
            if (setupCapture() && startCapture()) {
                DEBUG_LA_PRINTLN("◆ Auto-triggered capture started successfully");
            } else {
                DEBUG_LA_PRINTLN("◆ ERROR: Failed to start auto-triggered capture");
                la_capture_state = JL_LA_STOPPED;
            }
        }
    }
}

// =============================================================================
// RESOURCE ALLOCATION AND MANAGEMENT
// =============================================================================

bool allocateLogicAnalyzerResources() {
    DEBUG_LA_PRINTF("◆ Allocating logic analyzer resources using atomic system...\n");
    
    // Use the new unified buffer allocation system
    if (!allocateAllLogicAnalyzerBuffers()) {
        DEBUG_LA_PRINTF("◆ ERROR: Unified buffer allocation failed\n");
        return false;
    }
    
    // Complete PIO/DMA reset to ensure clean state
    DEBUG_LA_PRINTLN("◆ Performing complete PIO/DMA reset...");
    
    // Clean up any existing resources
    if (la_pio && la_prog_offset != (uint)-1) {
        pio_remove_program(la_pio, &la_pio_program_fast, la_prog_offset);
        DEBUG_LA_PRINTF("◆ Removed program from PIO%d offset %d\n", pio_get_index(la_pio), la_prog_offset);
        la_prog_offset = -1;
    }
    if (la_pio && la_sm != (uint)-1) {
        pio_sm_set_enabled(la_pio, la_sm, false);
        pio_sm_unclaim(la_pio, la_sm);
        DEBUG_LA_PRINTF("◆ Unclaimed PIO%d SM%d\n", pio_get_index(la_pio), la_sm);
        la_sm = -1;
    }
    if (la_dma_chan != -1) {
        dma_channel_abort(la_dma_chan);
        dma_channel_unclaim(la_dma_chan);
        DEBUG_LA_PRINTF("◆ Unclaimed DMA channel %d\n", la_dma_chan);
        la_dma_chan = -1;
    }
    
    la_pio = nullptr;
    DEBUG_LA_PRINTLN("◆ PIO/DMA reset complete");
    
    // Allocate PIO resources
    PIO pio_instances[] = { pio1, pio0, pio2 };
    bool pio_allocated = false;
    
    DEBUG_LA_PRINTLN("◆ Attempting PIO allocation...");
    
    for (int i = 0; i < 3 && !pio_allocated; i++) {
        la_pio = pio_instances[i];
        DEBUG_LA_PRINTF("◆ Trying PIO%d...\n", pio_get_index(la_pio));
        
        int sm = pio_claim_unused_sm(la_pio, false);
        if (sm < 0) {
            DEBUG_LA_PRINTF("◆ PIO%d: No available state machines\n", pio_get_index(la_pio));
            continue;
        }
        
        if (!pio_can_add_program(la_pio, &la_pio_program_fast)) {
            DEBUG_LA_PRINTF("◆ PIO%d: Cannot add program, unclaiming SM%d\n", pio_get_index(la_pio), sm);
            pio_sm_unclaim(la_pio, sm);
            continue;
        }
        
        la_prog_offset = pio_add_program(la_pio, &la_pio_program_fast);
        if (la_prog_offset < 0) {
            DEBUG_LA_PRINTF("◆ PIO%d: Failed to add program, unclaiming SM%d\n", pio_get_index(la_pio), sm);
            pio_sm_unclaim(la_pio, sm);
            continue;
        }
        
        la_sm = sm;
        pio_allocated = true;
        DEBUG_LA_PRINTF("◆ SUCCESS: PIO%d SM%d offset=%d\n", pio_get_index(la_pio), sm, la_prog_offset);
    }
    
    if (!pio_allocated) {
        DEBUG_LA_PRINTLN("◆ ERROR: Failed to allocate PIO resources");
        return false;
    }
    
    // Allocate DMA channel
    int dma = dma_claim_unused_channel(false);
    if (dma < 0) {
        DEBUG_LA_PRINTLN("◆ ERROR: No available DMA channels");
        return false;
    }
    la_dma_chan = dma;
    
    // Verify buffers exist
    if (!la_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Digital buffer not allocated by unified system\n");
        return false;
    }
    
    if ((current_la_mode == LA_MODE_MIXED_SIGNAL || current_la_mode == LA_MODE_ANALOG_ONLY) && !analog_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Analog buffer not allocated by unified system\n");
        return false;
    }
    
    const char* mode_str = (current_la_mode == LA_MODE_DIGITAL_ONLY) ? "digital-only" : 
                           (current_la_mode == LA_MODE_MIXED_SIGNAL) ? "mixed-signal" : "analog-only";
    
    DEBUG_LA_PRINTF("◆ UNIFIED ALLOCATION SUCCESS: PIO%d SM%d DMA%d (mode: %s)\n",
                 pio_get_index(la_pio), la_sm, la_dma_chan, mode_str);
    DEBUG_LA_PRINTF("  Digital: %lu bytes, Analog: %zu bytes, Max samples: %lu\n",
                 jl_la_buffer_size, analog_buffer_size, jl_la_max_samples);
    
    return true;
}

void releaseLogicAnalyzerResources() {
    DEBUG_LA_PRINTLN("◆ Releasing all logic analyzer resources...");
    
    // Stop any ongoing capture first
    la_capturing = false;
    la_capture_state = JL_LA_STOPPED;
    
    // Clean up PIO program and state machine
    if (la_pio && la_prog_offset != (uint)-1) {
        pio_remove_program(la_pio, &la_pio_program_fast, la_prog_offset);
        DEBUG_LA_PRINTF("◆ Removed PIO program from PIO%d offset %d\n", pio_get_index(la_pio), la_prog_offset);
        la_prog_offset = -1;
    }
    
    if (la_pio && la_sm != (uint)-1) {
        pio_sm_set_enabled(la_pio, la_sm, false);
        pio_sm_unclaim(la_pio, la_sm);
        DEBUG_LA_PRINTF("◆ Unclaimed PIO%d SM%d\n", pio_get_index(la_pio), la_sm);
        la_sm = -1;
    }
    
    // Clean up DMA channel
    if (la_dma_chan != -1) {
        dma_channel_abort(la_dma_chan);
        dma_channel_unclaim(la_dma_chan);
        DEBUG_LA_PRINTF("◆ Unclaimed DMA channel %d\n", la_dma_chan);
        la_dma_chan = -1;
    }
    
    // Clean up memory buffers
    if (la_buffer) {
        free(la_buffer);
        la_buffer = nullptr;
        jl_la_buffer_size = 0;
    }
    if (analog_buffer) {
        free(analog_buffer);
        analog_buffer = nullptr;
        analog_buffer_size = 0;
    }
    
    // Reset PIO pointer
    la_pio = nullptr;
    jl_la_max_samples = 0;
    
    DEBUG_LA_PRINTLN("◆ All logic analyzer resources released");
}

// =============================================================================
// BUFFER ALLOCATION SYSTEM
// =============================================================================

// Unified buffer requirements calculation and allocation

UnifiedBufferRequirements calculateAllBufferRequirements() {
    UnifiedBufferRequirements req = {};
    
    // Driver configuration
    req.driver_analog_channels = __builtin_popcount(analog_mask & 0xFF);
    req.driver_digital_channels = 8;
    
    // Memory availability
    size_t free_heap = rp2040.getFreeHeap();
    req.available_memory = free_heap;
    req.safety_reserve = JL_LA_RESERVE_RAM;
    
    if (free_heap > req.safety_reserve) {
        req.usable_memory = free_heap - req.safety_reserve;
    } else {
        req.usable_memory = 1024;
    }
    
    // Storage requirements per sample
    req.digital_storage_per_sample = 1;  // 1 byte per digital sample
    
    if (current_la_mode == LA_MODE_MIXED_SIGNAL || current_la_mode == LA_MODE_ANALOG_ONLY) {
        req.analog_storage_per_sample = req.driver_analog_channels * 2;  // 2 bytes per analog channel
        req.transmission_per_sample = 32;  // 32-byte unified format
    } else {
        req.analog_storage_per_sample = 0;
        req.transmission_per_sample = 3;   // 3-byte digital-only format
    }
    
    // Calculate maximum samples based on memory
    size_t total_storage_per_sample = req.digital_storage_per_sample + req.analog_storage_per_sample;
    
    // Ensure we don't divide by zero and apply reasonable limits
    if (total_storage_per_sample == 0) {
        total_storage_per_sample = 1;
    }
    
    req.max_samples_memory_limit = req.usable_memory / total_storage_per_sample;
    
    // Apply system limits first, then memory constraints
    req.max_samples_final = JL_LA_MAX_SAMPLES_LIMIT;  // Start with system limit
    
    // Reduce if memory doesn't allow
    if (req.max_samples_memory_limit < req.max_samples_final) {
        req.max_samples_final = req.max_samples_memory_limit;
    }
    
    // Ensure minimum
    if (req.max_samples_final < JL_LA_MIN_SAMPLES) {
        req.max_samples_final = JL_LA_MIN_SAMPLES;
    }
    
    // Calculate buffer allocations - use actual sample count, not system maximum
    uint32_t actual_samples = sample_count;
    if (actual_samples == 0) {
        actual_samples = JL_LA_DEFAULT_SAMPLES;
    }
    if (actual_samples > req.max_samples_final) {
        actual_samples = req.max_samples_final;
    }
    
    req.digital_buffer_bytes = actual_samples * req.digital_storage_per_sample;
    
    if (req.analog_storage_per_sample > 0) {
        // For analog buffer: use actual sample count
        req.analog_buffer_bytes = actual_samples * req.analog_storage_per_sample;
    } else {
        req.analog_buffer_bytes = 0;
    }
    
    req.total_buffer_bytes = req.digital_buffer_bytes + req.analog_buffer_bytes;
    
    // Update max_samples_final to reflect actual allocation
    req.max_samples_final = actual_samples;
    
    DEBUG_LA_PRINTF("◆ UNIFIED BUFFER CALC:\n");
    DEBUG_LA_PRINTF("  Driver config: %d digital, %d analog channels\n", 
                 req.driver_digital_channels, req.driver_analog_channels);
    DEBUG_LA_PRINTF("  Memory: %zu free, %zu usable, %zu safety reserve\n", 
                 req.available_memory, req.usable_memory, req.safety_reserve);
    DEBUG_LA_PRINTF("  Buffers: %zu digital + %zu analog = %zu total bytes\n",
                 req.digital_buffer_bytes, req.analog_buffer_bytes, req.total_buffer_bytes);
    
    return req;
}

bool allocateAllLogicAnalyzerBuffers() {
    UnifiedBufferRequirements req = calculateAllBufferRequirements();
    
    if (req.total_buffer_bytes > req.usable_memory) {
        DEBUG_LA_PRINTF("◆ ERROR: Cannot allocate %zu bytes (only %zu available)\n", 
                     req.total_buffer_bytes, req.usable_memory);
        return false;
    }
    
    // Free existing buffers
    if (la_buffer) {
        free(la_buffer);
        la_buffer = nullptr;
    }
    if (analog_buffer) {
        free(analog_buffer);
        analog_buffer = nullptr;
    }
    
    // Allocate digital buffer
    la_buffer = (uint32_t*)malloc(req.digital_buffer_bytes);
    if (!la_buffer) {
        DEBUG_LA_PRINTF("◆ ERROR: Failed to allocate %zu byte digital buffer\n", req.digital_buffer_bytes);
        return false;
    }
    memset(la_buffer, 0, req.digital_buffer_bytes);
    jl_la_buffer_size = req.digital_buffer_bytes;
    jl_la_max_samples = req.max_samples_final;
    
    // Allocate analog buffer if needed
    if (req.analog_buffer_bytes > 0) {
        analog_buffer = (uint16_t*)malloc(req.analog_buffer_bytes);
        if (!analog_buffer) {
            DEBUG_LA_PRINTF("◆ ERROR: Failed to allocate %zu byte analog buffer\n", req.analog_buffer_bytes);
            free(la_buffer);
            la_buffer = nullptr;
            return false;
        }
        memset(analog_buffer, 0, req.analog_buffer_bytes);
        analog_buffer_size = req.analog_buffer_bytes;
        DEBUG_LA_PRINTF("◆ DMA ANALOG BUFFER: Allocated %zu bytes for final data assembly\n", req.analog_buffer_bytes);
    } else {
        analog_buffer = nullptr;
        analog_buffer_size = 0;
    }
    
    DEBUG_LA_PRINTF("◆ UNIFIED ALLOCATION SUCCESS: %zu bytes total (%zu free memory remaining)\n",
                 req.total_buffer_bytes, rp2040.getFreeHeap());
    
    return true;
}

// =============================================================================
// COMMAND PROCESSING
// =============================================================================

void processCommand(uint8_t cmd) {
    DEBUG_LA_PRINTF("◆ Command: 0x%02X\n", cmd);
    updateLastActivity();
    
    // Handle PulseView/Enhanced protocol commands
    if (cmd >= 0xA0 && cmd <= 0xAB) {
        processPulseViewCommand(cmd);
        return;
    }
    
    // Handle SUMP Protocol Commands
    switch (cmd) {
        case SUMP_SET_DIVIDER:
            if (la_usb_available() >= 3) {
                uint32_t divider = 0;
                divider = (la_usb_read() << 16) | (la_usb_read() << 8) | la_usb_read();
                uint32_t clock_freq = 100000000;
                uint32_t new_sample_rate = (divider == 0) ? clock_freq : (clock_freq / (divider + 1));
                if (new_sample_rate < 100) new_sample_rate = 100;
                if (new_sample_rate > JL_LA_MAX_SAMPLE_RATE) new_sample_rate = JL_LA_MAX_SAMPLE_RATE;
                sample_rate = new_sample_rate;
                DEBUG_LA_PRINTF("◆ SUMP rate: %lu Hz\n", sample_rate);
            }
            break;
        
        case SUMP_SET_READ_DELAY:
            if (la_usb_available() >= 4) {
                uint16_t read_count = (la_usb_read() << 8) | la_usb_read();
                uint16_t delay_count = (la_usb_read() << 8) | la_usb_read();
                uint32_t new_sample_count = (read_count + 1) * 4;
                if (new_sample_count < JL_LA_MIN_SAMPLES) new_sample_count = JL_LA_MIN_SAMPLES;
                if (new_sample_count > jl_la_max_samples) new_sample_count = jl_la_max_samples;
                sample_count = new_sample_count;
                DEBUG_LA_PRINTF("◆ SUMP samples: %lu\n", sample_count);
            }
            break;
        
        case SUMP_SET_FLAGS:
            if (la_usb_available() >= 4) {
                uint32_t flags = 0;
                for (int i = 0; i < 4; i++) {
                    flags |= (la_usb_read() << (i * 8));
                }
                DEBUG_LA_PRINTF("◆ SUMP flags: 0x%08X\n", flags);
            }
            break;
            
        case SUMP_SET_TRIGGER_MASK_0:
        case SUMP_SET_TRIGGER_VALUE_0:
            if (la_usb_available() >= 4) {
                for (int i = 0; i < 4; i++) la_usb_read();
            }
            break;
            
        default:
            if ((cmd >= 0xC0 && cmd <= 0xCF) || (cmd >= 0xD0 && cmd <= 0xDF)) {
                if (la_usb_available() >= 4) {
                    for (int i = 0; i < 4; i++) la_usb_read();
                }
            }
            break;
    }
}

void processPulseViewCommand(uint8_t cmd) {
    DEBUG_LA_PRINTF("◆ PulseView Command: 0x%02X\n", cmd);
    updateLastActivity();
    
    switch (cmd) {
        case JUMPERLESS_CMD_RESET:
            la_capture_state = JL_LA_STOPPED;
            la_capturing = false;
            clearTrigger();
            releaseLogicAnalyzerResources(); // Clean up all resources
            DEBUG_LA_PRINTLN("◆ Device reset (triggers cleared)");
            if (enhanced_mode) sendStatusResponse(0x00);
            break;
            
        case JUMPERLESS_CMD_ID:
            sendSUMPID();
            break;
            
        case JUMPERLESS_CMD_GET_HEADER:
            enhanced_mode = true;
            sendJumperlessHeader();
            break;
            
        case JUMPERLESS_CMD_SET_CHANNELS:
            enhanced_mode = true;
            
            // Flush any stale data from previous transaction to prevent desync
            while (la_usb_available() > 0) {
                la_usb_read();
            }
            delay(5); // Small delay to ensure buffer is clear

            if (la_usb_available() >= 8) {
                // Read digital mask (4 bytes) - consumed but not used
                uint32_t digital_mask = 0;
                for (int i = 0; i < 4; i++) {
                    digital_mask |= (la_usb_read() << (i * 8));
                }
                
                // Read analog mask (4 bytes)
                uint32_t analog_mask_new = 0;
                for (int i = 0; i < 4; i++) {
                    analog_mask_new |= (la_usb_read() << (i * 8));
                }
                
                // Count enabled analog channels
                uint8_t new_analog_chan_count = 0;
                for (int i = 0; i < 32; i++) {
                    if (analog_mask_new & (1UL << i)) {
                        new_analog_chan_count++;
                        DEBUG_LA_PRINTF("◆ Analog channel %d enabled in mask\n", i);
                    }
                }
                DEBUG_LA_PRINTF("◆ Total analog channels detected: %d (mask=0x%08X)\n", 
                             new_analog_chan_count, analog_mask_new);
                analog_mask = analog_mask_new;
                analog_chan_count = new_analog_chan_count;
                
                // Always use mixed-signal mode (32-byte unified format)
                current_la_mode = LA_MODE_MIXED_SIGNAL;
                DEBUG_LA_PRINTF("◆ Channels configured: Mixed-signal mode with %d analog channels (unified format)\n", analog_chan_count);
                
                sendStatusResponse(0x00);
                delay(50);
                sendJumperlessHeader();
            } else {
                sendErrorResponse(0x01);
            }
            break;
        
        case JUMPERLESS_CMD_ARM:
            if (la_capture_state == JL_LA_STOPPED) {
                la_capture_state = JL_LA_ARMED;
                sendStatusResponse(0x00);
                la_usb_flush();  // Ensure response is sent before any delay
                DEBUG_LA_PRINTF("◆ Device armed - waiting for trigger\n");
            } else {
                DEBUG_LA_PRINTF("◆ ARM command failed - invalid state: %d (expected JL_LA_STOPPED=%d)\n", 
                             la_capture_state, JL_LA_STOPPED);
                if (enhanced_mode) sendErrorResponse(0x02);
            }
            break;
            
        case JUMPERLESS_CMD_RUN:
            if (la_capture_state == JL_LA_COMPLETE) {
                // Data already captured and sent - just acknowledge
                DEBUG_LA_PRINTLN("◆ RUN command - data already captured and transmitted");
                sendStatusResponse(0x00);
                la_usb_flush();
            } else if (la_capture_state == JL_LA_STOPPED || la_capture_state == JL_LA_ARMED) {
                // CRITICAL: Send status response FIRST, before any capture logic
                sendStatusResponse(0x00);
                la_usb_flush();
                
                if (la_capture_state == JL_LA_STOPPED) {
                    la_capture_state = JL_LA_ARMED;
                }
                
                DEBUG_LA_PRINTLN("◆ RUN command - starting capture in background");
                
                if (sample_count == 0) {
                    sample_count = 1000;  // Use small default
                    DEBUG_LA_PRINTF("◆ SAFETY: Zero sample count, using default %lu\n", sample_count);
                }
                
                DEBUG_LA_PRINTF("◆ Starting capture: rate=%lu Hz, samples=%lu, mode=%d\n", 
                             sample_rate, sample_count, current_la_mode);
                        
                if (setupCapture() && startCapture()) {
                    DEBUG_LA_PRINTLN("◆ RUN command: Capture started successfully");
                } else {
                    DEBUG_LA_PRINTLN("◆ RUN command: ERROR - Failed to start capture");
                    la_capture_state = JL_LA_STOPPED;
                }
            } else {
                DEBUG_LA_PRINTLN("◆ RUN command in invalid state - sending OK anyway");
                sendStatusResponse(0x00);
                la_usb_flush();
            }
            break;
            
        case JUMPERLESS_CMD_CONFIGURE:
            enhanced_mode = true;
            if (la_usb_available() >= 8) {
                uint32_t new_sample_rate = 0;
                uint32_t new_sample_count = 0;
                
                for (int i = 0; i < 4; i++) {
                    new_sample_rate |= (la_usb_read() << (i * 8));
                }
                for (int i = 0; i < 4; i++) {
                    new_sample_count |= (la_usb_read() << (i * 8));
                }
                
                if (new_sample_rate > 0 && new_sample_rate <= JL_LA_MAX_SAMPLE_RATE) {
                    sample_rate = new_sample_rate;
                } else {
                    sample_rate = 1000000;
                }
                
                if (new_sample_count > 0 && new_sample_count <= jl_la_max_samples) {
                    sample_count = new_sample_count;
                } else if (new_sample_count == 0) {
                    sample_count = 1000;  // Use small default when driver sends 0
                } else {
                    sample_count = jl_la_max_samples;
                }
                
                DEBUG_LA_PRINTF("◆ Config: rate=%lu Hz, samples=%lu\n", 
                             sample_rate, sample_count);
                sendStatusResponse(0x00);
            } else {
                sample_rate = 1000000;
                sample_count = JL_LA_DEFAULT_SAMPLES;
                sendErrorResponse(0x01);
            }
            break;
            
        default:
            DEBUG_LA_PRINTF("◆ Unknown PulseView command: 0x%02X\n", cmd);
            break;
    }
}

bool captureAnalogData() {
    DEBUG_LA_PRINTF("◆ Starting analog data capture...\n");
    
    if (analog_chan_count == 0 || !analog_buffer) {
        DEBUG_LA_PRINTLN("◆ No analog channels enabled or buffer not allocated");
        return true;  // Not an error if no analog channels
    }
    
    DEBUG_LA_PRINTF("◆ Capturing analog data: %lu samples at %lu Hz from %d channels (mask=0x%08X)\n",
                 jl_la_max_samples, sample_rate, analog_chan_count, analog_mask);
    
    // Calculate timing parameters
    uint32_t sample_interval_us = 1000000 / sample_rate;
    if (sample_interval_us == 0) sample_interval_us = 1;
    
    // Capture analog data for each sample
    for (uint32_t sample = 0; sample < jl_la_max_samples; sample++) {
        uint32_t start_time = micros();
        
        // Read all enabled analog channels for this sample
        int enabled_channel_index = 0;
        for (int ch = 0; ch < 8; ch++) {
            if (analog_mask & (1UL << ch)) {
                if (enabled_channel_index < analog_chan_count) {
                    // Read ADC channel
                    adc_select_input(ch);
                    uint16_t adc_value = adc_read();
                    
                    // Store in buffer (interleaved by enabled channels)
                    size_t buffer_index = sample * analog_chan_count + enabled_channel_index;
                    if (buffer_index < (analog_buffer_size / sizeof(uint16_t))) {
                        analog_buffer[buffer_index] = adc_value;
                    }
                    
                    enabled_channel_index++;
                }
            }
        }
        
        // Wait for next sample time
        while ((micros() - start_time) < sample_interval_us) {
            // Busy wait for precise timing
        }
        
        // Yield occasionally for long captures
        if (sample % 1000 == 0) {
            yield();
        }
    }
    
    DEBUG_LA_PRINTF("◆ Analog capture complete: %lu samples × %d channels\n", 
                 jl_la_max_samples, analog_chan_count);
    return true;
}

bool checkTriggerCondition() {
    return trigger_mode == TRIGGER_NONE || trigger_la;
}

bool setupCapture() {
    DEBUG_LA_PRINTLN("◆ Setting up capture...");
    
    // Clean up any existing resources first to ensure clean state
    releaseLogicAnalyzerResources();
    DEBUG_LA_PRINTLN("◆ Previous resources cleaned up");
    
    // Allocate resources only when needed for capture
    if (!allocateLogicAnalyzerResources()) {
        DEBUG_LA_PRINTLN("◆ ERROR: Failed to allocate resources for capture");
        releaseLogicAnalyzerResources(); // Clean up any partial allocations
        return false;
    }
    
    DEBUG_LA_PRINTF("◆ Capture setup complete - ready for %lu samples\n", sample_count);
    return true;
}

bool startCapture() {
    DEBUG_LA_PRINTLN("◆ Starting digital capture...");
    
    if (!la_buffer || jl_la_max_samples == 0) {
        DEBUG_LA_PRINTLN("◆ ERROR: No buffer allocated for capture");
        return false;
    }
    
    // Configure PIO for digital capture
    pio_sm_set_enabled(la_pio, la_sm, false);
    pio_sm_clear_fifos(la_pio, la_sm);
    pio_sm_restart(la_pio, la_sm);
    
    // Configure DMA for digital data
    dma_channel_config dma_config = dma_channel_get_default_config(la_dma_chan);
    channel_config_set_transfer_data_size(&dma_config, DMA_SIZE_32);
    channel_config_set_read_increment(&dma_config, false);
    channel_config_set_write_increment(&dma_config, true);
    channel_config_set_dreq(&dma_config, pio_get_dreq(la_pio, la_sm, false));
    
    dma_channel_configure(la_dma_chan, &dma_config,
                         la_buffer,           // Destination
                         &la_pio->rxf[la_sm], // Source
                         jl_la_max_samples,   // Transfer count
                         false);              // Don't start yet
    
    // Start PIO state machine
    pio_sm_set_enabled(la_pio, la_sm, true);
    
    // Start DMA transfer
    dma_start_channel_mask(1u << la_dma_chan);
    
    la_capture_state = JL_LA_TRIGGERED;
    la_capturing = true;
    
    DEBUG_LA_PRINTF("◆ Digital capture started: %lu samples\n", jl_la_max_samples);
    return true;
}

bool isCaptureDone() {
    if (!la_capturing) {
        return true;
    }
    
    // Check if DMA transfer is complete
    bool digital_done = !dma_channel_is_busy(la_dma_chan);
    
    if (digital_done) {
        DEBUG_LA_PRINTF("◆ Capture complete: %lu samples\n", jl_la_max_samples);
        la_capturing = false;
        la_capture_state = JL_LA_COMPLETE;
        
        // Auto-send data when capture completes (no waiting for RUN command)
        // Always capture analog data for unified 32-byte format
        if (captureAnalogData()) {
            DEBUG_LA_PRINTLN("◆ Analog capture completed");
        }
        
        // Send all data immediately (always mixed-signal format)
        sendCaptureData();
        
        return true;
    }
    
    return false;
}

void sendCaptureData() {
    DEBUG_LA_PRINTF("◆ Sending data: %lu samples (unified 32-byte format)\n", jl_la_max_samples);
    
    if (!la_buffer) {
        DEBUG_LA_PRINTLN("◆ ERROR: No capture data available");
        return;
    }
    
    // Always use 32 bytes per sample (unified format)
    size_t total_bytes = jl_la_max_samples * 32;
    
    DEBUG_LA_PRINTF("◆ Data header: %zu bytes (%lu samples × 32 bytes/sample transmission)\n", 
                 total_bytes, jl_la_max_samples);
    
    // Always send mixed-signal unified format
    sendMixedSignalData();
    
    DEBUG_LA_PRINTLN("◆ Data transmission complete");
    
    // Reset state to allow subsequent captures
    la_capture_state = JL_LA_STOPPED;
    DEBUG_LA_PRINTLN("◆ State reset for next capture");
}

void sendDigitalOnlyData() {
    DEBUG_LA_PRINTF("◆ Sending digital-only data: %lu samples\n", jl_la_max_samples);
    
    // Send data using new unified format (3 bytes per sample)
    for (uint32_t i = 0; i < jl_la_max_samples; i++) {
        uint8_t sample[3];
        
        // Extract digital data from PIO buffer
        uint32_t raw_sample = la_buffer[i];
        sample[0] = (raw_sample >> 0) & 0xFF;  // GPIO data
        sample[1] = 0x00;                      // UART data (unused)
        sample[2] = 0xDD;                      // Digital-only marker
        
        la_usb_write_buffer(sample, 3);
        
        // Yield periodically to prevent USB timeout
        if (i % 100 == 0) {
            la_usb_flush();
        }
    }
    
    la_usb_flush();
    DEBUG_LA_PRINTF("◆ Digital data sent: %lu samples (3 bytes each)\n", jl_la_max_samples);
}

void sendMixedSignalData() {
    DEBUG_LA_PRINTF("◆ Sending mixed-signal data: %lu samples, 32 bytes/sample\n", jl_la_max_samples);
    
    // First capture analog data if not already done
    if (!analog_buffer || analog_buffer_size == 0) {
        DEBUG_LA_PRINTLN("◆ ERROR: Analog data not captured - run captureAnalogData() first");
        return;
    }
    
    DEBUG_LA_PRINTF("◆ CHANNEL MAPPING DEBUG: analog_mask=0x%08X\n", analog_mask);
    int output_channel = 0;
    for (int i = 0; i < 8; i++) {
        if (analog_mask & (1UL << i)) {
            DEBUG_LA_PRINTF("◆   Output Ch%d → Buffer pos %d\n", output_channel, i);
            output_channel++;
        }
    }
    
    DEBUG_LA_PRINTF("◆ CHANNEL VERIFICATION: mask=0x%08X, expected=%d, chan_count=%d\n", 
                 analog_mask, __builtin_popcount(analog_mask & 0xFF), analog_chan_count);
    DEBUG_LA_PRINTF("◆ BUFFER VERIFICATION: size=%zu bytes, elements=%zu, samples=%lu\n", 
                 analog_buffer_size, analog_buffer_size / sizeof(uint16_t), jl_la_max_samples);
    
    // Send unified mixed-signal format (32 bytes per sample)
    for (uint32_t i = 0; i < jl_la_max_samples; i++) {
        uint8_t sample[32];
        memset(sample, 0, 32);
        
        // Digital data (3 bytes) - Extract lower 8 bits like old implementation
        uint32_t raw_digital = la_buffer[i];
        sample[0] = raw_digital & 0xFF;         // GPIO data (lower 8 bits only)
        sample[1] = 0x00;                       // UART data (unused)
        sample[2] = 0xDA;                       // Mixed-signal marker
        
        // Analog data (28 bytes: 14 channels × 2 bytes each)
        uint8_t *analog_ptr = &sample[3];
        
        // Fill all 14 firmware channels (only some may have real data)
        for (int ch = 0; ch < 14; ch++) {
            uint16_t adc_value = 0;
            
            if (ch < 8 && (analog_mask & (1UL << ch))) {
                // This is an enabled ADC channel - find its data in analog_buffer
                int enabled_index = 0;
                for (int prev_ch = 0; prev_ch < ch; prev_ch++) {
                    if (analog_mask & (1UL << prev_ch)) {
                        enabled_index++;
                    }
                }
                
                // Get value from analog buffer (interleaved by enabled channels)
                if (enabled_index < analog_chan_count) {
                    size_t buffer_index = i * analog_chan_count + enabled_index;
                    if (buffer_index < (analog_buffer_size / sizeof(uint16_t))) {
                        adc_value = analog_buffer[buffer_index];
                    }
                }
            } else {
                // Disabled or non-ADC channel - send dummy data
                adc_value = 0x0000;
            }
            
            // Store as little-endian
            analog_ptr[0] = adc_value & 0xFF;
            analog_ptr[1] = (adc_value >> 8) & 0xFF;
            analog_ptr += 2;
        }
        
        // EOF marker
        sample[31] = 0xA0;
        
        la_usb_write_buffer(sample, 32);
        
        // Yield periodically
        if (i % 50 == 0) {
            la_usb_flush();
        }
    }
    
    la_usb_flush();
    DEBUG_LA_PRINTF("◆ Mixed-signal data sent: %lu samples (32 bytes each)\n", jl_la_max_samples);
}

void clearTrigger() {
    trigger_mode = TRIGGER_NONE;
    trigger_la = false;
}

void handleConnectionStateChange(bool connected) {
    static bool last_connected = false;
    if (connected != last_connected) {
        DEBUG_LA_PRINTF("◆ USB connection %s\n", connected ? "established" : "lost");
        last_connected = connected;
    }
}

void updateLastActivity() {
    last_activity_ms = millis();
}

// =============================================================================
// USB COMMUNICATION INTERFACE
// =============================================================================

bool la_usb_connected() {
    return USBSer2;
}

int la_usb_available() {
    return USBSer2.available();
}

uint8_t la_usb_read() {
    return USBSer2.read();
}

void la_usb_write(uint8_t data) {
    USBSer2.write(data);
}

void la_usb_write_buffer(const uint8_t *data, size_t length) {
    USBSer2.write(data, length);
}

void la_usb_flush() {
    USBSer2.flush();
}

// =============================================================================
// PROTOCOL RESPONSES
// =============================================================================

void sendSUMPID() {
    const char* sump_id = "1SLO";
    DEBUG_LA_PRINTF("◆ Sent SUMP ID: %s\n", sump_id);
    la_usb_write_buffer((const uint8_t*)sump_id, 4);
    la_usb_flush();
}

void sendJumperlessHeader() {
    DEBUG_LA_PRINTLN("◆ Sending Jumperless header...");
    
    // Send response header first (0x80) to match driver expectations
    la_usb_write(0x80);
    
    // Then send the 88-byte header data
    uint8_t header[88];
    memset(header, 0, 88);
    
    // Magic string
    memcpy(header, "$JLDATA", 7);
    header[7] = 0;
    
    // Basic info
    header[8] = 2; // Protocol version
    header[9] = LA_MODE_MIXED_SIGNAL; // Always mixed-signal mode
    
    // Channel counts (maximum available, not currently enabled) - MUST be uint8_t
    header[10] = 8; // Digital channels (max available)
    header[11] = JL_LA_ANALOG_COUNT; // Analog channels (max available: 5)
    
    // Current configuration
    *(uint32_t*)(header + 12) = sample_rate; // Sample rate (Hz)
    *(uint32_t*)(header + 16) = sample_count; // Number of samples to capture
    *(uint32_t*)(header + 20) = 0xFF; // Digital channel mask (all 8 channels available)
    *(uint32_t*)(header + 24) = analog_mask; // Analog channel mask
    
    // Data format - always 32 bytes per sample (unified format)
    header[28] = 32; // bytes_per_sample
    header[29] = 3;  // digital_bytes_per_sample
    header[30] = 28; // analog_bytes_per_sample (5 channels × 2 bytes + others)
    header[31] = 12; // adc_resolution_bits
    
    // Trigger configuration (future - set to 0 for now)
    *(uint32_t*)(header + 32) = 0; // trigger_channel_mask
    *(uint32_t*)(header + 36) = 0; // trigger_pattern  
    *(uint32_t*)(header + 40) = 0; // trigger_edge_mask
    *(uint32_t*)(header + 44) = 0; // pre_trigger_samples
    
    // Analog voltage range
    *(float*)(header + 48) = 18.28f; // analog_voltage_range
    
    // Limits and capabilities  
    *(uint64_t*)(header + 52) = JL_LA_MAX_SAMPLE_RATE; // max_sample_rate
    *(uint64_t*)(header + 60) = calculateAllBufferRequirements().max_samples_final; // max_memory_depth
    
    // Support flags
    header[68] = 0; // supports_triggers
    header[69] = 0; // supports_compression  
    header[70] = 0x02; // supported_modes (only mixed-signal mode supported)
    
    // Version strings - limited to fit in 88 bytes
    memcpy(header + 71, "RP2350-v2.0", 11); // firmware_version
    memcpy(header + 82, "JLV5", 4); // device_id
    
    // Simple checksum (last 2 bytes)
    *(uint16_t*)(header + 86) = 0x1234; // Simple checksum placeholder
    
    la_usb_write_buffer(header, 88);
    la_usb_flush();
    
    DEBUG_LA_PRINTF("◆ Header sent: %lu samples, 32 bytes/sample, %d analog channels\n", 
                 sample_count, analog_chan_count);
}

void sendStatusResponse(uint8_t status) {
    DEBUG_LA_PRINTF("◆ Sending status response: 0x82 with status 0x%02X\n", status);
    la_usb_write(0x82);
    la_usb_write(status);
    la_usb_flush();
    DEBUG_LA_PRINTLN("◆ Status response sent and flushed");
}

void sendErrorResponse(uint8_t error_code) {
    DEBUG_LA_PRINTF("◆ Sending error response: 0x83 with error 0x%02X\n", error_code);
    la_usb_write(0x83);
    la_usb_write(error_code);
    la_usb_flush();
}