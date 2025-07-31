// SPDX-License-Identifier: MIT

#ifndef LOGICANALYZER_H
#define LOGICANALYZER_H

#include <Arduino.h>
#include <hardware/pio.h>
#include <hardware/dma.h>
#include <hardware/adc.h>


extern bool debugLA;

extern volatile bool logicAnalyzing;

// =============================================================================
// JUMPERLESS DUAL-MODE LOGIC ANALYZER
// =============================================================================
// Supports both SUMP compatibility mode (digital-only) and enhanced Jumperless
// mode (mixed-signal) for maximum compatibility and functionality

// Hardware Configuration
#define JL_LA_PIN_BASE                20          // Starting GPIO pin (GPIO 20-27)
#define JL_LA_PIN_COUNT               8           // Number of digital pins
#define JL_LA_ANALOG_COUNT            5           // Number of analog channels
#define JL_LA_MAX_SAMPLE_RATE         50000000    // 50 MHz max (conservative for RP2350B)
#define JL_LA_MIN_SAMPLES             1        // Minimum sample count
#define JL_LA_DEFAULT_SAMPLES         10000       // Default sample count (conservative)
#define JL_LA_MAX_SAMPLES_LIMIT       100000     // 256K maximum samples
// #define JL_LA_RESERVE_RAM             (4 * 1024) // Reserve RAM for system

// Protocol Mode Enumeration
typedef enum {
    LA_PROTOCOL_SUMP = 0,       // SUMP/OLS compatibility mode (digital only)
    LA_PROTOCOL_ENHANCED = 1    // Enhanced Jumperless mode (mixed-signal)
} LogicAnalyzerProtocol;

// Capture Mode Enumeration  
typedef enum {
    LA_MODE_DIGITAL_ONLY = 0,   // Digital channels only
    LA_MODE_MIXED_SIGNAL = 1,   // Digital + analog channels
    LA_MODE_ANALOG_ONLY = 2     // Analog channels only
} LogicAnalyzerMode;

// Device State Enumeration
typedef enum {
    LA_STATE_IDLE = 0,          // Ready for commands
    LA_STATE_ARMED = 1,         // Armed and waiting for trigger
    LA_STATE_TRIGGERED = 2,     // Triggered and capturing
    LA_STATE_COMPLETE = 3       // Capture complete, data ready
} LogicAnalyzerState;

// SUMP Protocol Commands (Standard OLS/SUMP)
#define SUMP_CMD_RESET              0x00    // Reset device
#define SUMP_CMD_RUN                0x01    // Start capture  
#define SUMP_CMD_ID                 0x02    // Get device ID
#define SUMP_CMD_METADATA           0x04    // Get metadata (optional)
#define SUMP_CMD_SET_DIVIDER        0x80    // Set sample rate divider (3 bytes)
#define SUMP_CMD_SET_READ_DELAY     0x81    // Set sample count + delay (4 bytes)
#define SUMP_CMD_SET_FLAGS          0x82    // Set capture flags (4 bytes)

// Enhanced Jumperless Protocol Commands (offset by 0xA0 to avoid conflicts)
#define JL_CMD_RESET                0xA0    // Reset device
#define JL_CMD_RUN                  0xA1    // Start capture
#define JL_CMD_ID                   0xA2    // Get device ID
#define JL_CMD_GET_HEADER           0xA3    // Get capabilities header
#define JL_CMD_SET_CHANNELS         0xA4    // Configure channels (8 bytes)
#define JL_CMD_ARM                  0xA5    // Arm trigger
#define JL_CMD_GET_STATUS           0xA6    // Get status
#define JL_CMD_CONFIGURE            0xA7    // Set sample rate/count (8 bytes)
#define JL_CMD_SET_SAMPLES          0xA8    // Set sample count only (4 bytes)

// Enhanced Protocol Response Types
#define JL_RESP_HEADER              0x80    // Device capabilities header
#define JL_RESP_DATA                0x81    // Sample data stream
#define JL_RESP_STATUS              0x82    // Status/acknowledgment
#define JL_RESP_ERROR               0x83    // Error response

// Error Codes
#define JL_ERROR_SUCCESS            0x00    // Success
#define JL_ERROR_INSUFFICIENT_DATA  0x01    // Not enough data received
#define JL_ERROR_INVALID_STATE      0x02    // Invalid state for operation
#define JL_ERROR_UNKNOWN_COMMAND    0xFF    // Unknown command

// Device Capabilities Header (88 bytes)
typedef struct {
    char magic[8];                      // "$JLDATA\0"
    uint8_t version;                    // Protocol version (2)
    uint8_t capture_mode;               // Current mode (0=digital, 1=mixed, 2=analog)
    uint16_t num_digital_channels;      // Number of digital channels (8)
    uint16_t num_analog_channels;       // Number of analog channels (5)
    uint32_t sample_rate;               // Current sample rate (Hz)
    uint32_t sample_count;              // Number of samples to capture
    uint32_t digital_channel_mask;      // Available digital channels (0xFF)
    uint32_t analog_channel_mask;       // Enabled analog channels
    uint8_t bytes_per_sample;           // Total bytes per sample
    uint8_t digital_bytes_per_sample;   // Digital bytes per sample (1)
    uint8_t analog_bytes_per_sample;    // Analog bytes per sample (N*2)
    uint8_t analog_resolution;          // ADC resolution (12 bits)
    uint32_t trigger_channel_mask;      // Trigger channels (future)
    uint32_t trigger_pattern;           // Trigger pattern (future)
    uint32_t trigger_edge_mask;         // Edge trigger mask (future)
    uint32_t pre_trigger_samples;       // Pre-trigger samples (future)
    float analog_voltage_range;         // ADC voltage range (18.28V)
    uint32_t max_sample_rate;           // Maximum sample rate (50MHz)
    uint32_t max_samples;               // Maximum sample count
    uint8_t supported_modes;            // Supported modes bitmask
    char firmware_version[16];          // Firmware version string
    char device_id[16];                 // Device identifier
    uint32_t checksum;                  // Header checksum (XOR)
} __attribute__((packed)) JumperlessHeader;

// Global State Structure
typedef struct {
    // Protocol and mode
    LogicAnalyzerProtocol protocol_mode;    // SUMP or Enhanced
    LogicAnalyzerMode capture_mode;         // Digital/Mixed/Analog
    LogicAnalyzerState device_state;        // Current state
    
    // Channel configuration
    uint32_t digital_channel_mask;          // Active digital channels
    uint32_t analog_channel_mask;           // Active analog channels
    uint8_t analog_channel_count;           // Number of active analog channels
    
    // Capture configuration
    uint32_t sample_rate;                   // Sample rate in Hz
    uint32_t sample_count;                  // Number of samples
    uint8_t bytes_per_sample;               // Total bytes per sample
    
    // Hardware resources
    PIO pio_instance;                       // PIO instance (pio0 or pio1)
    uint pio_sm;                            // PIO state machine number
    uint pio_program_offset;                // Program offset in PIO memory
    uint dma_channel;                       // DMA channel number
    uint8_t *sample_buffer;                 // Sample data buffer
    uint32_t buffer_size;                   // Buffer size in bytes
    uint32_t max_samples;                   // Maximum samples based on available RAM
    
    // State tracking
    uint32_t samples_captured;              // Number of samples captured
    uint32_t capture_start_us;              // Capture start timestamp
    bool capturing;                         // Currently capturing flag
    
    // Connection management
    bool usb_connected;                     // USB connection state
    uint32_t last_activity_ms;              // Last USB activity timestamp
    uint32_t connection_timeout_ms;         // Connection timeout
    
} LogicAnalyzerContext;




typedef struct {
    uint8_t digital_data[2];
    uint8_t header_byte;        // 0xDA - mixed signal marker
    uint16_t analog_data[8];    // 8 ADC channels
    uint16_t dac_data[2];       // 2 DAC channels  
    uint16_t ina0_data[2];      // INA0 voltage/current
    uint16_t ina1_data[2];      // INA1 voltage/current
    uint8_t eof_byte;           // 0xA0 - end of frame marker
    
} __attribute__((packed)) mixed_signal_sample;


struct UnifiedBufferRequirements {
    // Driver configuration
    uint32_t driver_analog_channels;
    uint32_t driver_digital_channels;
    
    // Memory calculations
    size_t available_memory;
    size_t safety_reserve;
    size_t usable_memory;
    
    // Sample calculations
    uint32_t max_samples_memory_limit;
    uint32_t max_samples_final;
    
    // Buffer size calculations
    size_t digital_storage_per_sample;
    size_t analog_storage_per_sample;
    size_t transmission_per_sample;
    
    size_t digital_buffer_bytes;
    size_t analog_buffer_bytes;
    size_t total_buffer_bytes;
};

// =============================================================================
// PUBLIC API FUNCTIONS
// =============================================================================

// Core initialization and management
void setupLogicAnalyzer(void);
void handleLogicAnalyzer(void);
void stopLogicAnalyzer(void);
void gracefulLogicAnalyzerShutdown(void);
bool checkLogicAnalyzerConflicts(void);
bool disableRotaryEncoderForLogicAnalyzer(void);

// State queries
bool isLogicAnalyzerAvailable(void);
bool isLogicAnalyzerCapturing(void);
LogicAnalyzerState getLogicAnalyzerState(void);
LogicAnalyzerProtocol getProtocolMode(void);
LogicAnalyzerMode getCaptureMode(void);

// Manual control (for testing)
void enableLogicAnalyzer(void);
void disableLogicAnalyzer(void);
void setLogicAnalyzerMode(LogicAnalyzerMode mode);
void startLogicAnalyzerCapture(void);

// Status and debugging
void printLogicAnalyzerStatus(void);
void printLogicAnalyzerMemoryInfo(void);
void printProtocolStatistics(void);

// Mixed signal data acquisition and transmission
bool acquireMixedSignalData(void);
void transmitMixedSignalBuffer(void);

// Oversampling calculation functions
uint32_t calculateOversamplingFactor();
uint32_t calculateEffectiveSampleCount(uint32_t physical_samples);

// =============================================================================
// PROTOCOL HANDLERS  
// =============================================================================

// SUMP Protocol Handler
void handleSUMPProtocol(void);
bool processSUMPCommand(uint8_t command);
void sendSUMPResponse(const uint8_t *data, size_t length);

// Enhanced Protocol Handler  
void handleEnhancedProtocol(void);
bool processEnhancedCommand(uint8_t command);
void sendEnhancedResponse(uint8_t response_type, const uint8_t *data, size_t length);

// Common protocol functions
void detectProtocolMode(void);
void resetProtocolState(void);

// =============================================================================
// CAPTURE FUNCTIONS
// =============================================================================

// Capture control
bool setupCapture(void);
bool startCapture(void);
bool isCaptureDone(void);
void abortCapture(void);
void resetCaptureHardware(void);

// Data transmission
void sendCaptureData(void);
void sendDigitalOnlyData(void);
void sendMixedSignalData(void);
void sendAnalogOnlyData(void);

// SUMP-specific data transmission
void sendDigitalOnlyDataSUMP(void);
void sendMixedSignalDataSUMP(void);

// Enhanced protocol data transmission
void sendEnhancedCaptureData(void);
void sendDigitalOnlyDataEnhanced(void);
void sendMixedSignalDataEnhanced(void);

// =============================================================================
// HARDWARE FUNCTIONS
// =============================================================================

// Resource management
bool allocateHardwareResources(void);
void releaseHardwareResources(void);
void calculateOptimalBufferSize(void);

// GPIO and PIO configuration
void configurePIOForCapture(void);
void configureGPIOPins(void);
void configureDMAForCapture(void);

// ADC functions for mixed-signal mode
void setupADCForCapture(void);
int16_t getAnalogBaseline(int channel);
uint16_t readADCChannel(uint8_t channel);
float convertADCToVoltage(uint16_t adc_value, uint8_t channel);

// RP2350B specific functions
void unlatchRP2350BGPIOPins(void);

// =============================================================================
// USB COMMUNICATION
// =============================================================================

// USB interface functions (using USBSer2 for logic analyzer)
bool la_usb_connected(void);
int la_usb_available(void);
uint8_t la_usb_read(void);
void la_usb_write(uint8_t data);
void la_usb_write_buffer(const uint8_t *data, size_t length);
void la_usb_flush(void);

// Connection management
void handleConnectionStateChange(bool connected);
bool checkConnectionTimeout(void);
void updateLastActivity(void);

// =============================================================================
// UTILITY AND TEST FUNCTIONS  
// =============================================================================

// Test functions
void testLogicAnalyzerPins(void);
void testLogicAnalyzerCapture(void);
void testSUMPProtocol(void);
void testEnhancedProtocol(void);
void testMixedSignalCapture(void);
void testPulseViewCompatibility(void);

// Diagnostic functions
void printLogicAnalyzerConflictDiagnosis(void);
void printHardwareConfiguration(void);
void printProtocolCompatibility(void);

// Validation functions
bool validateSampleConfiguration(void);
bool validateChannelConfiguration(void);
bool validateHardwareState(void);

// =============================================================================
// LEGACY COMPATIBILITY DEFINITIONS
// =============================================================================

// Legacy state type (for compatibility with current implementation)
typedef LogicAnalyzerState la_state_t;

// Legacy state constants (for compatibility with current implementation)
#define JL_LA_STOPPED               LA_STATE_IDLE
#define JL_LA_ARMED                 LA_STATE_ARMED
#define JL_LA_TRIGGERED             LA_STATE_TRIGGERED
#define JL_LA_COMPLETE              LA_STATE_COMPLETE

// =============================================================================
// CONSTANTS AND DEFAULTS
// =============================================================================

// Default configuration values
#define JL_LA_DEFAULT_SAMPLE_RATE   100000     // 100 kHz
#define JL_LA_DEFAULT_PROTOCOL      LA_PROTOCOL_SUMP
#define JL_LA_DEFAULT_MODE          LA_MODE_MIXED_SIGNAL
#define JL_LA_DEFAULT_TIMEOUT       15000       // 15 seconds
#define JL_LA_CAPTURE_TIMEOUT       60000       // 60 seconds

// SUMP constants
#define SUMP_ID_STRING              "1SLO"      // SUMP device ID
#define SUMP_CLOCK_BASE             100000000   // 100 MHz base clock

// Enhanced protocol constants  
#define JL_MAGIC_STRING             "$JLDATA"   // Header magic
#define JL_PROTOCOL_VERSION         2           // Current protocol version
#define JL_DEVICE_ID                "Jumperless Logic Analyzer v2.0"
#define JL_FIRMWARE_VERSION         "RP2350-v2.0"
#define JL_ANALOG_VOLTAGE_RANGE     18.28f      // Jumperless ADC spread

// Buffer and timing constants
#define JL_LA_CHUNK_SIZE            256         // USB transfer chunk size
#define JL_LA_USB_TIMEOUT           1000        // USB operation timeout
#define JL_LA_RESPONSE_DELAY_US     100         // Response delay microseconds

#endif // LOGICANALYZER_H