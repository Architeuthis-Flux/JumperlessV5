// SPDX-License-Identifier: MIT

#include "LogicAnalyzer.h"
#include "Arduino.h"
#include "ArduinoStuff.h"
#include "Peripherals.h"
#include "config.h"

// Hardware includes for RP2350B
#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/regs/dreq.h"
#include "hardware/structs/dma.h"
#include "pico/stdlib.h"

#include "JumperlessDefines.h"

#include "Graphics.h"

// Add these includes at the top of the file after existing includes
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/regs/adc.h>

#include <pico/time.h>

/*
DATA FORMAT SPECIFICATION:

Digital Mode (3 bytes per sample):
------------------------------------
[byte 0] GPIO data: (gpio1)(gpio2)(gpio3)(gpio4)(gpio5)(gpio6)(gpio7)(gpio8)
[byte 1] UART data:
(uart_tx)(uart_rx)(unused)(unused)(unused)(unused)(unused)(unused) [byte 2]
Format marker:
  - 0xDD: Digital only (end of sample)
  - 0xDA: Mixed signal (expect 28 more analog bytes)
  - 0xAA: Analog only (digital bytes are dummy, expect 28 more analog bytes)

Analog/Mixed Mode (32 bytes per sample):
---------------------------------------
[bytes 0-2] Digital data (same as above)
[bytes 3-30] Analog data (14 channels × 2 bytes each):
  - Channel 0-7: ADC 0-7 (12-bit values, little-endian)
  - Channel 8-9: DAC 0-1 values
  - Channel 10-13: INA219 data (2 devices × 2 channels each)
[byte 31] EOF marker: 0xA0

Note: 12-bit ADC data uses LSB as validity flag (1=real, 0=dummy)
*/

// =============================================================================
// PROTOCOL DEFINITIONS
// =============================================================================

volatile bool logicAnalyzing = false;

// Jumperless Protocol Commands
#define JUMPERLESS_CMD_RESET 0x00
#define JUMPERLESS_CMD_RUN 0x01
#define JUMPERLESS_CMD_ID 0xA2
#define JUMPERLESS_CMD_GET_HEADER 0xA3
#define JUMPERLESS_CMD_SET_CHANNELS 0xA4
#define JUMPERLESS_CMD_ARM 0xA5
#define JUMPERLESS_CMD_GET_STATUS 0xA6
#define JUMPERLESS_CMD_CONFIGURE 0xA7
#define JUMPERLESS_CMD_SET_SAMPLES 0xA8
#define JUMPERLESS_CMD_SET_MODE 0xA9
#define JUMPERLESS_CMD_SET_TRIGGER 0xAA
#define JUMPERLESS_CMD_CLEAR_TRIGGER 0xAB
#define JL_CMD_END_DATA 0xAC // Signal end of data transmission

// SUMP Protocol Commands
#define SUMP_SET_DIVIDER 0x80
#define SUMP_SET_READ_DELAY 0x81
#define SUMP_SET_FLAGS 0x82
#define SUMP_SET_TRIGGER_MASK_0 0xC0
#define SUMP_SET_TRIGGER_VALUE_0 0xD0

// Response codes
#define JUMPERLESS_RESP_HEADER 0x80
#define JUMPERLESS_RESP_DATA 0x81
#define JUMPERLESS_RESP_STATUS 0x82
#define JUMPERLESS_RESP_ERROR 0x83
#define JUMPERLESS_RESP_END_DATA 0x84 // End of data transmission response

// Data format markers
#define DIGITAL_ONLY_MARKER 0xDD
#define MIXED_SIGNAL_MARKER 0xDA
#define ANALOG_ONLY_MARKER 0xAA
#define ANALOG_EOF_MARKER 0xA0

// Configuration constants
#define JL_LA_MIN_SAMPLES 1
#define JL_LA_DEFAULT_SAMPLES 1000
#define JL_LA_MAX_SAMPLES_LIMIT 25000   // More conservative limit
#define JL_LA_RESERVE_RAM ( 24 * 1024 ) // Increased reserve for stability
#define JL_LA_MAX_SAMPLE_RATE 50000000

#define FORCE_5_CHANNELS 1

// =============================================================================
// GLOBAL STATE
// =============================================================================

// Debug flags
bool debugLA = true;
bool debugLA2 = false;

// Round-robin alignment correction
int detected_channel_offset = 0;

// Trigger system
typedef enum {
    TRIGGER_NONE = 0,     // No trigger, auto-start after ARM
    TRIGGER_EXTERNAL = 1, // External trigger via trigger_la variable
    TRIGGER_GPIO = 2,     // GPIO trigger on specified pin/pattern
    TRIGGER_THRESHOLD = 3 // Analog threshold trigger
} trigger_type_t;

// External trigger variables (accessible from Python/other modules)
volatile bool trigger_la = false; // Set this to true to trigger capture
static trigger_type_t trigger_mode = TRIGGER_NONE;
static uint32_t gpio_trigger_mask = 0x00;    // Which GPIO pins to monitor
static uint32_t gpio_trigger_pattern = 0x00; // Pattern to match (1=high, 0=low)
static bool gpio_trigger_edge = false;       // true=edge trigger, false=level trigger
static uint32_t last_gpio_state = 0x00;      // For edge detection

// Threshold trigger variables
static uint8_t threshold_channel = 0;    // ADC channel for threshold (0-7)
static float threshold_voltage = 2.5f;   // Threshold voltage level
static bool threshold_rising = true;     // true=rising edge, false=falling edge
static bool threshold_triggered = false; // To prevent multiple triggers
static float last_adc_voltage = 0.0f;    // For edge detection
#define DEBUG_LA_PRINTF( fmt, ... )              \
    do {                                         \
        if ( debugLA ) {                         \
            Serial.printf( fmt, ##__VA_ARGS__ ); \
        }                                        \
    } while ( 0 )
#define DEBUG_LA_PRINTLN( x )    \
    do {                         \
        if ( debugLA ) {         \
            Serial.println( x ); \
        }                        \
    } while ( 0 )
#define DEBUG_LA2_PRINTF( fmt, ... )             \
    do {                                         \
        if ( debugLA2 ) {                        \
            Serial.printf( fmt, ##__VA_ARGS__ ); \
        }                                        \
    } while ( 0 )
#define DEBUG_LA2_PRINTLN( x )   \
    do {                         \
        if ( debugLA2 ) {        \
            Serial.println( x ); \
        }                        \
    } while ( 0 )

// Hardware resources
static bool la_initialized = false;
static bool la_enabled = false;
static PIO la_pio = nullptr;
static uint la_sm = 0;
static uint la_dma_chan = 0;
static uint la_prog_offset = -1;
static uint32_t* la_buffer = nullptr;
static mixed_signal_sample* mixed_signal_buffer = nullptr;

// Analog capture buffer - separate from digital capture
static uint16_t* analog_buffer = nullptr;
static uint32_t analog_buffer_size = 0;
static bool analog_capture_complete = false;

// Capture state
static volatile la_state_t la_capture_state = JL_LA_STOPPED;
static volatile bool la_capturing = false;
static LogicAnalyzerMode current_la_mode = LA_MODE_DIGITAL_ONLY;

// Configuration
static uint32_t sample_rate = 1000000;
static uint32_t sample_count = JL_LA_DEFAULT_SAMPLES;
static uint32_t analog_mask = 0x00;
static uint8_t analog_chan_count = 0;
static uint32_t jl_la_max_samples = JL_LA_DEFAULT_SAMPLES;
static uint32_t jl_la_buffer_size = JL_LA_DEFAULT_SAMPLES;

// Connection management
static bool usb_was_connected = false;
static uint32_t last_usb_activity = 0;
static bool enhanced_mode = false;

// PIO program for fast capture with interrupt pacing
const uint16_t la_program_fast[] = {
    0x4008, // in pins, 8
    0xc000  // irq set 0 (raise interrupt 0 for pacing, continues immediately)
};

const struct pio_program la_pio_program_fast = {
    .instructions = la_program_fast,
    .length = 2,
    .origin = -1,
};

// Debug: Print PIO program instructions
void debug_pio_program( ) {
    DEBUG_LA_PRINTF( "◆ PIO Program: [0x%04x, 0x%04x] length=%d\n",
                     la_program_fast[ 0 ], la_program_fast[ 1 ], la_pio_program_fast.length );
}

// Add these global variables for hardware timer control
static volatile bool analog_sample_ready = false;
static volatile uint32_t analog_samples_taken = 0;
static volatile uint32_t analog_target_samples = 0;

// DMA-based ADC variables
static int adc_dma_channel = -1;
static volatile bool adc_dma_complete = false;
static uint16_t* adc_dma_buffer = nullptr;
static uint adc_timer_num = 0; // DMA timer used for pacing

int temperature_reading = 0;

int channel_offset = 0;

// Oversampling for high sample rates
#define ADC_MAX_SAMPLE_RATE 100000            // 250 ksps maximum safe ADC rate for RP2350B
static bool adc_oversampling_mode = false;    // Flag when sample rate > 250 ksps
static uint32_t adc_actual_sample_rate = 0;   // Actual ADC sample rate (capped at max)
static uint32_t adc_duplication_factor = 1;   // How many times to duplicate each sample
static uint32_t adc_min_sample_rate = 100000; // Minimum sample rate for ADC
static uint32_t adc_decimation_factor = 1;    // How many times to decimate the sample rate

// AINSEL embedded capture - no separate buffer needed
// AINSEL channel info will be embedded in upper 4 bits of each 16-bit sample
// Format: [15:12] = AINSEL channel, [11:0] = 12-bit ADC result

// Persistent oversampling state (survives cleanup for data transmission)
static bool capture_used_oversampling = false;
static uint32_t capture_duplication_factor = 1;

static bool capture_used_decimation = false;
static uint32_t capture_decimation_factor = 1;

// =============================================================================
// MAIN LOGIC ANALYZER FUNCTIONS (Primary Interface)
// =============================================================================

void setupLogicAnalyzer( );
void handleLogicAnalyzer( );

// =============================================================================
// FORWARD DECLARATIONS
// =============================================================================

// =============================================================================
// PROTOCOL FUNCTIONS
// =============================================================================

void processCommand( uint8_t cmd );
void sendSUMPID( );
void sendJumperlessHeader( );
void sendStatusResponse( uint8_t status );
void sendEndOfDataSignal( );
void sendErrorResponse( uint8_t error_code );
void configureModeAndChannels( LogicAnalyzerMode mode, uint32_t analog_channel_mask );

// =============================================================================
// DMA-BASED ADC SAMPLING FUNCTIONS
// =============================================================================

void la_counter_handler( );
void adcDmaHandler( );
bool initAdcDma( uint32_t sample_rate );
bool captureAnalogDataDMA( );
void cleanupAdcDma( );
void embedAinselInSamples( );
bool setupCapture( );
bool startCapture( );
bool isCaptureDone( );
bool captureAnalogData( );

// =============================================================================
// DATA TRANSMISSION FUNCTIONS
// =============================================================================

void sendCaptureData( );
void sendDigitalData( );
void sendMixedSignalData( );

// =============================================================================
// BUFFER MANAGEMENT FUNCTIONS
// =============================================================================

bool allocateAllLogicAnalyzerBuffers( );
void releaseAllLogicAnalyzerBuffers( );
UnifiedBufferRequirements calculateUnifiedBufferRequirements( void );
// void invalidateBufferCache();

// =============================================================================
// RESOURCE MANAGEMENT FUNCTIONS
// =============================================================================

bool allocateLogicAnalyzerResources( );
void releaseLogicAnalyzerResources( );
bool checkLogicAnalyzerConflicts( );
bool disableRotaryEncoderForLogicAnalyzer( );

// =============================================================================
// USB INTERFACE FUNCTIONS
// =============================================================================

bool la_usb_connected( );
void la_usb_write( uint8_t data );
void la_usb_write_buffer( const uint8_t* data, size_t len );
int la_usb_available( );
uint8_t la_usb_read( );
void la_usb_flush( );
void handleConnectionStateChange( bool connected );
void updateLastActivity( );

// =============================================================================
// UTILITY FUNCTIONS
// =============================================================================

uint32_t calculateTransmissionBytesPerSample( );
uint32_t calculateStorageBytesPerSample( );
uint32_t calculateBytesPerSample( );
uint32_t calculateOversamplingFactor( );
uint32_t calculateEffectiveSampleCount( uint32_t physical_samples );
int16_t getAnalogBaseline( int channel );

// =============================================================================
// TRIGGER FUNCTIONS
// =============================================================================

bool checkTriggerCondition( );

unsigned int la_counter = 0;

//! main

void setupLogicAnalyzer( ) {
    DEBUG_LA_PRINTLN( "◆ Setting up Logic Analyzer..." );

    // Check for conflicts before setup - but don't fail if some PIOs are unavailable
    if ( !checkLogicAnalyzerConflicts( ) ) {
        DEBUG_LA_PRINTLN( "◆ WARNING: Resource conflicts detected during setup" );
        DEBUG_LA_PRINTLN( "◆ Logic analyzer will attempt to use available resources" );
    }

    Serial.println( "Setting up Logic Analyzer..." );
    changeTerminalColorHighSat( 1, true, &Serial, 1 );
    Serial.println( "This is VERY experimental, and may not work" );
    Serial.println( "There's a fork of Pulseview to support this that should kinda work" );
    Serial.println( "https://github.com/Architeuthis-Flux/julseview" );
    changeTerminalColorHighSat( 0, true, &Serial, 0 );
    // CRITICAL: Check for specific component conflicts

    debugLA = true;
    DEBUG_LA_PRINTLN( "◆ Checking for specific component conflicts..." );

    // Check if rotary encoder is initialized and which PIO it's using
    extern PIO pioEnc;
    extern uint smEnc;
    // if (pioEnc != nullptr && smEnc != (uint)-1) {
    //   DEBUG_LA_PRINTF("◆ INFO: Rotary encoder is using PIO%d SM%d\n", pio_get_index(pioEnc), smEnc);
    //   if (pioEnc == pio1) {
    //     DEBUG_LA_PRINTLN("◆ WARNING: Rotary encoder is using PIO1 - logic analyzer will use PIO2 or PIO0");
    //   }
    // } else {
    //   DEBUG_LA_PRINTLN("◆ INFO: Rotary encoder is not initialized");
    // }

    // Check if NeoPixel LEDs are active
    // extern bool splitLEDs;
    // if (splitLEDs) {
    //   DEBUG_LA_PRINTLN("◆ INFO: NeoPixel LEDs are active - may use PIO resources dynamically");
    // }

    // Do not allocate buffers on startup. They will be allocated on-demand.

    // Configure GPIO pins
    // for (int i= 0; i < JL_LA_PIN_COUNT; i++) {
    //   gpio_init(JL_LA_PIN_BASE + i);
    // }

    current_la_mode = LA_MODE_MIXED_SIGNAL;
    la_initialized = true;
    la_enabled = true;
    la_capture_state = JL_LA_STOPPED;

    DEBUG_LA_PRINTF( "◆ Logic Analyzer ready - pins %d-%d\n", JL_LA_PIN_BASE,
                     JL_LA_PIN_BASE + JL_LA_PIN_COUNT - 1 );
}

void handleLogicAnalyzer( ) {
    // Rate limiting to prevent overwhelming USB buffers
    static uint32_t last_handler_call = 0;
    uint32_t current_time = millis( );
    if ( current_time - last_handler_call < 5 ) { // Limit to max 200Hz for better stability
        return;
    }
    last_handler_call = current_time;

    bool usb_connected = la_usb_connected( );
    handleConnectionStateChange( usb_connected );

    if ( !usb_connected ) {
        enhanced_mode = false;
        logicAnalyzing = false;
        return;
    }

    // Process commands with rate limiting
    static uint32_t last_cmd_time = 0;

    if ( la_usb_available( ) ) {
        // Rate limit command processing to prevent overwhelming the system
        if ( current_time - last_cmd_time < 10 ) { // Minimum 10ms between commands
            return;
        }
        last_cmd_time = current_time;

        uint8_t cmd = la_usb_read( );

        // Auto-setup logic analyzer if not initialized
        if ( !la_initialized || !la_enabled ) {
            DEBUG_LA_PRINTF( "◆ Auto-setting up logic analyzer for command 0x%02X\n", cmd );
            setupLogicAnalyzer( );
        }

        // Filter valid commands
        if ( ( cmd >= 0xA0 &&
               cmd <= 0xAB ) ||                 // Enhanced Jumperless commands (0xA0-0xAB)
             ( cmd >= 0x80 && cmd <= 0x82 ) ||  // SUMP commands
             ( cmd >= 0xC0 && cmd <= 0xCF ) ||  // Other commands
             ( cmd >= 0xD0 && cmd <= 0xDF ) ) { // Other commands

            if ( !enhanced_mode ) {
                enhanced_mode = true;
                DEBUG_LA_PRINTLN( "◆ Protocol detected" );
            }

            // Add error handling for command processing
            DEBUG_LA_PRINTF( "◆ Processing command: 0x%02X\n", cmd );

            // Add timeout protection for command processing
            uint32_t cmd_start_time = millis( );
            processCommand( cmd );
            uint32_t cmd_duration = millis( ) - cmd_start_time;

            if ( cmd_duration > 100 ) {
                DEBUG_LA_PRINTF( "◆ WARNING: Command 0x%02X took %lu ms\n", cmd,
                                 cmd_duration );
            }

            DEBUG_LA_PRINTF( "◆ Command 0x%02X completed in %lu ms\n", cmd,
                             cmd_duration );
        } else {
            DEBUG_LA_PRINTF( "◆ Ignoring invalid command: 0x%02X\n", cmd );
        }

        updateLastActivity( );
    }

    // Handle capture completion
    if ( la_capture_state == JL_LA_TRIGGERED && isCaptureDone( ) ) {
        DEBUG_LA_PRINTLN( "◆ Capture completed" );

        // For mixed-signal mode, capture analog data after digital capture is done
        if ( current_la_mode == LA_MODE_MIXED_SIGNAL ||
             current_la_mode == LA_MODE_ANALOG_ONLY ) {
            DEBUG_LA_PRINTLN( "◆ Starting analog data capture..." );
            // if (captureAnalogData()) {
            //   DEBUG_LA_PRINTLN("◆ Analog capture completed");
            // } else {
            //   DEBUG_LA_PRINTLN("◆ ERROR: Analog capture failed");
            // }
        }

        // After capture is complete, send the data packet immediately.
        // The driver's receive_data callback will be listening for this.
        sendCaptureData( );
        logicAnalyzing = false;
    }

    // Check for trigger conditions when armed
    if ( la_capture_state == JL_LA_ARMED && !la_capturing ) {
        if ( checkTriggerCondition( ) ) {
            DEBUG_LA_PRINTLN( "◆ Trigger condition met - starting capture" );

            // Safety check: ensure valid sample count
            if ( sample_count == 0 ) {
                sample_count = JL_LA_DEFAULT_SAMPLES;
                DEBUG_LA_PRINTF( "◆ SAFETY: Zero sample count, using default %lu\n",
                                 sample_count );
            }

            DEBUG_LA_PRINTF( "◆ Auto-triggered capture: rate=%lu Hz, samples=%lu, "
                             "mode=%d, trigger=%s\n",
                             sample_rate, sample_count, current_la_mode,
                             trigger_mode == TRIGGER_NONE        ? "auto"
                             : trigger_mode == TRIGGER_EXTERNAL  ? "external"
                             : trigger_mode == TRIGGER_GPIO      ? "GPIO"
                             : trigger_mode == TRIGGER_THRESHOLD ? "threshold"
                                                                 : "unknown" );

            if ( setupCapture( ) && startCapture( ) ) {
                DEBUG_LA_PRINTLN( "◆ Auto-triggered capture started successfully" );
            } else {
                DEBUG_LA_PRINTLN( "◆ ERROR: Failed to start auto-triggered capture" );
                la_capture_state = JL_LA_STOPPED;
            }
        }
    }
    // logicAnalyzing= false;
}

// =============================================================================
// USB INTERFACE
// =============================================================================

bool la_usb_connected( ) {
#if USB_CDC_ENABLE_COUNT >= 3
    // FIXED: Only check DTR signal - availableForWrite() can be 0 during normal
    // flow control Don't confuse full USB buffers with actual disconnection Add
    // debouncing to prevent false disconnection detection
    static bool last_dtr_state = false;
    static uint32_t last_dtr_change = 0;
    static bool debounced_state = false;

    bool current_dtr = USBSer2.dtr( );
    uint32_t current_time = millis( );

    // Debounce DTR changes with 10ms delay
    if ( current_dtr != last_dtr_state ) {
        last_dtr_change = current_time;
        last_dtr_state = current_dtr;
    }

    // Only change debounced state after 10ms of stable signal
    if ( current_time - last_dtr_change > 10 ) {
        debounced_state = current_dtr;
    }

    return debounced_state;
#else
    return false;
#endif
}

void la_usb_write( uint8_t data ) {
#if USB_CDC_ENABLE_COUNT >= 3
    // Add flow control to prevent buffer overruns
    uint32_t start_time = millis( );
    while ( USBSer2.availableForWrite( ) < 1 ) {
        if ( millis( ) - start_time > 100 ) { // 100ms timeout
            DEBUG_LA2_PRINTLN( "◆ WARNING: USB write timeout" );
            return;
        }
        delayMicroseconds( 100 );
        yield( );
    }
    USBSer2.write( data );
#endif
}

void la_usb_write_buffer( const uint8_t* data, size_t len ) {
#if USB_CDC_ENABLE_COUNT >= 3
    // STABILITY: Break large writes into smaller chunks to prevent buffer
    // overruns
    const size_t MAX_CHUNK = 32; // Even smaller chunks for better stability
    size_t offset = 0;

    while ( offset < len ) {
        size_t chunk_size = min( MAX_CHUNK, len - offset );

        // Wait for buffer space with adaptive timeout and better flow control
        uint32_t start_time = millis( );
        uint32_t wait_time = 0;
        uint32_t max_wait_time = 1000; // Reduced timeout for faster recovery

        while ( USBSer2.availableForWrite( ) < chunk_size ) {
            wait_time = millis( ) - start_time;
            if ( wait_time > max_wait_time ) {
                DEBUG_LA_PRINTF(
                    "◆ WARNING: USB buffer full after %lu ms - continuing\n",
                    wait_time );
                return;
            }

            // Adaptive delay based on wait time
            if ( wait_time < 50 ) {
                delayMicroseconds( 500 ); // Shorter delay initially
            } else if ( wait_time < 200 ) {
                delayMicroseconds( 1000 ); // Medium delay
            } else {
                delayMicroseconds( 2000 ); // Longer delay for extended waits
            }

            yield( ); // Allow other cores to run

            // Check USB connection periodically
            if ( wait_time % 50 == 0 && !la_usb_connected( ) ) {
                DEBUG_LA_PRINTLN( "◆ ERROR: USB disconnected during transmission" );
                return;
            }
        }

        // Write the chunk
        size_t written = USBSer2.write( data + offset, chunk_size );
        if ( written != chunk_size ) {
            DEBUG_LA_PRINTF( "◆ WARNING: Partial write %zu/%zu bytes\n", written,
                             chunk_size );
            offset += written;
            if ( written == 0 ) {
                delayMicroseconds( 500 ); // Wait if no bytes written
            }
        } else {
            offset += chunk_size;
        }

        // Small delay between chunks for stability
        if ( offset < len ) {
            delayMicroseconds( 25 ); // Reduced delay between chunks
        }
    }
#endif
}

int la_usb_available( ) {
#if USB_CDC_ENABLE_COUNT >= 3
    return USBSer2.available( );
#else
    return 0;
#endif
}

uint8_t la_usb_read( ) {
#if USB_CDC_ENABLE_COUNT >= 3
    return USBSer2.read( );
#else
    return 0;
#endif
}

void la_usb_flush( ) {
#if USB_CDC_ENABLE_COUNT >= 3
    USBSer2.flush( );
#endif
}

// =============================================================================
// CONNECTION MANAGEMENT
// =============================================================================

void updateLastActivity( ) { last_usb_activity = millis( ); }

void handleConnectionStateChange( bool connected ) {
    static uint32_t last_connection_change = 0;
    uint32_t current_time = millis( );

    if ( connected && !usb_was_connected ) {
        DEBUG_LA2_PRINTLN( "◆ USB connection established" );
        updateLastActivity( );
        usb_was_connected = true;
        last_connection_change = current_time;
    } else if ( !connected && usb_was_connected ) {
        DEBUG_LA_PRINTLN( "◆ USB connection lost" );
        uint32_t idle_time = millis( ) - last_usb_activity;
        DEBUG_LA_PRINTF( "◆ Connection lost - idle time: %lu ms\n", idle_time );

        if ( idle_time > 10000 ) { // 10 second grace period
            if ( la_capture_state != JL_LA_STOPPED ) {
                la_capturing = false;
                if ( la_dma_chan >= 0 && la_dma_chan < 12 ) {
                    dma_channel_abort( la_dma_chan );
                }
                if ( la_pio && la_sm >= 0 && la_sm < 4 ) {
                    pio_sm_clear_fifos( la_pio, la_sm );
                }
                la_capture_state = JL_LA_STOPPED;
                DEBUG_LA_PRINTLN( "◆ Capture safely stopped" );
            }
        }
        // Reset the connection flag so we can detect reconnection
        usb_was_connected = false;
        last_connection_change = current_time;
    }

    // Add protection against rapid connection changes
    if ( current_time - last_connection_change <
         100 ) { // 100ms minimum between changes
        DEBUG_LA2_PRINTLN( "◆ WARNING: Rapid connection state changes detected" );
    }
}

// =============================================================================
// RESOURCE MANAGEMENT
// =============================================================================

// // // Forward declaration
// // void releaseLogicAnalyzerResources();

// uint32_t calculateTransmissionBytesPerSample() {
//   if (current_la_mode == LA_MODE_DIGITAL_ONLY) {
//     return 3; // Digital only: 3 bytes per sample
//   } else {
//     // Mixed/Analog mode: Always 32 bytes per sample for driver compatibility
//     // This ensures the driver can parse the unified format correctly
//     return 32; // 3 bytes digital + 28 bytes analog + 1 EOF byte = 32 bytes
//                // total
//   }
// }

// uint32_t calculateStorageBytesPerSample() {
//   // For capture storage, we only store digital data (1 byte per sample)
//   // Analog data is captured in real-time during transmission
//   Serial.println("calculateStorageBytesPerSample");
//   return 2; // 1 byte digital data per sample
// }

// // Backward compatibility alias
// uint32_t calculateBytesPerSample() {
//   return calculateTransmissionBytesPerSample();
// }

// Legacy struct removed - using atomic allocation system instead

// =============================================================================
// UNIFIED BUFFER MANAGEMENT SYSTEM
// =============================================================================
// This replaces multiple overlapping buffer calculation and allocation
// functions with a single, comprehensive system that ensures consistency and
// prevents leaks.

// // Forward declarations
// void releaseAllLogicAnalyzerBuffers();
// bool allocateAllLogicAnalyzerBuffers();

// // Cached buffer configuration - calculated once at allocation, used throughout
// // capture
// struct CachedBufferConfig {
//   bool is_valid;
//   uint32_t analog_channels_to_capture;
//   uint32_t max_samples;
//   size_t digital_buffer_size;
//   size_t analog_buffer_size;
//   size_t total_buffer_size;
//   uint32_t digital_bytes_per_sample;
//   uint32_t analog_bytes_per_sample;
//   uint32_t total_bytes_per_sample;
// };

bool allocateLogicAnalyzerResources( ) {
    DEBUG_LA_PRINTF( "◆ Allocating logic analyzer resources using atomic system...\n" );

    // Use the new unified buffer allocation system
    if ( !allocateAllLogicAnalyzerBuffers( ) ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Unified buffer allocation failed\n" );
        return false;
    }

    // Complete PIO/DMA reset to ensure clean state
    DEBUG_LA_PRINTLN( "◆ Performing complete PIO/DMA reset..." );

    // Clean up any existing resources
    if ( la_pio && la_prog_offset != (uint)-1 ) {
        pio_remove_program( la_pio, &la_pio_program_fast, la_prog_offset );
        DEBUG_LA_PRINTF( "◆ Removed program from PIO%d offset %d\n", pio_get_index( la_pio ), la_prog_offset );
        la_prog_offset = -1;
    }
    if ( la_pio && la_sm != (uint)-1 ) {
        pio_sm_set_enabled( la_pio, la_sm, false );
        pio_sm_unclaim( la_pio, la_sm );
        DEBUG_LA_PRINTF( "◆ Unclaimed PIO%d SM%d\n", pio_get_index( la_pio ), la_sm );
        la_sm = -1;
    }
    if ( la_dma_chan != -1 ) {
        dma_channel_abort( la_dma_chan );
        dma_channel_unclaim( la_dma_chan );
        DEBUG_LA_PRINTF( "◆ Unclaimed DMA channel %d\n", la_dma_chan );
        la_dma_chan = -1;
    }

    la_pio = nullptr;
    DEBUG_LA_PRINTLN( "◆ PIO/DMA reset complete" );

    // Allocate PIO resources with smart priority order
    // Priority: PIO1 (rotary encoder), PIO0 (CH446Q), PIO2 (NeoPixels)
    PIO pio_instances[] = { pio1, pio0, pio2 };
    bool pio_allocated = false;

    DEBUG_LA_PRINTLN( "◆ Attempting PIO allocation..." );

    for ( int i = 0; i < 3 && !pio_allocated; i++ ) {
        la_pio = pio_instances[ i ];
        DEBUG_LA_PRINTF( "◆ Trying PIO%d...\n", pio_get_index( la_pio ) );

        int sm = pio_claim_unused_sm( la_pio, false );
        if ( sm < 0 ) {
            DEBUG_LA_PRINTF( "◆ PIO%d: No available state machines\n", pio_get_index( la_pio ) );
            continue;
        }

        if ( !pio_can_add_program( la_pio, &la_pio_program_fast ) ) {
            DEBUG_LA_PRINTF( "◆ PIO%d: Cannot add program, unclaiming SM%d\n", pio_get_index( la_pio ), sm );
            pio_sm_unclaim( la_pio, sm );
            continue;
        }

        la_prog_offset = pio_add_program( la_pio, &la_pio_program_fast );
        if ( la_prog_offset < 0 ) {
            DEBUG_LA_PRINTF( "◆ PIO%d: Failed to add program, unclaiming SM%d\n", pio_get_index( la_pio ), sm );
            pio_sm_unclaim( la_pio, sm );
            continue;
        }

        la_sm = sm;
        pio_allocated = true;
        DEBUG_LA_PRINTF( "◆ SUCCESS: PIO%d SM%d offset=%d\n", pio_get_index( la_pio ), sm, la_prog_offset );
    }

    if ( !pio_allocated ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Failed to allocate PIO resources" );
        return false;
    }

    // Allocate DMA channel
    int dma = dma_claim_unused_channel( false );
    if ( dma < 0 ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: No available DMA channels" );
        return false;
    }
    la_dma_chan = dma;

    // Verify buffers exist
    if ( !la_buffer ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Digital buffer not allocated by unified system\n" );
        return false;
    }

    if ( ( current_la_mode == LA_MODE_MIXED_SIGNAL || current_la_mode == LA_MODE_ANALOG_ONLY ) && !analog_buffer ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Analog buffer not allocated by unified system\n" );
        return false;
    }

    const char* mode_str = ( current_la_mode == LA_MODE_DIGITAL_ONLY ) ? "digital-only" : ( current_la_mode == LA_MODE_MIXED_SIGNAL ) ? "mixed-signal"
                                                                                                                                      : "analog-only";

    DEBUG_LA_PRINTF( "◆ UNIFIED ALLOCATION SUCCESS: PIO%d SM%d DMA%d (mode: %s)\n",
                     pio_get_index( la_pio ), la_sm, la_dma_chan, mode_str );
    DEBUG_LA_PRINTF( "  Digital: %lu bytes, Analog: %zu bytes, Max samples: %lu\n",
                     jl_la_buffer_size, analog_buffer_size, jl_la_max_samples );

    return true;
}

// =============================================================================
// TRIGGER SYSTEM
// =============================================================================

bool checkTriggerCondition( ) {
    switch ( trigger_mode ) {
    case TRIGGER_NONE:
        // In enhanced mode, don't auto-trigger - wait for RUN command
        // In SUMP mode (non-enhanced), auto-trigger after ARM
        return !enhanced_mode; // Only auto-start in SUMP mode

    case TRIGGER_EXTERNAL:
        if ( trigger_la ) {
            trigger_la = false; // Reset trigger flag
            DEBUG_LA_PRINTLN( "◆ External trigger activated" );
            return true;
        }
        return false;

    case TRIGGER_GPIO: {
        if ( gpio_trigger_mask == 0 )
            return false; // No pins configured

        uint32_t current_gpio = gpio_get_all( ) & gpio_trigger_mask;

        if ( gpio_trigger_edge ) {
            // Edge trigger: detect change from last state
            uint32_t edges = ( current_gpio ^ last_gpio_state ) & gpio_trigger_mask;
            uint32_t rising_edges = edges & current_gpio;
            uint32_t falling_edges = edges & ( ~current_gpio );

            // Check if trigger pattern matches rising or falling edges
            bool triggered = ( ( gpio_trigger_pattern & rising_edges ) != 0 ) ||
                             ( ( ~gpio_trigger_pattern & falling_edges ) != 0 );

            last_gpio_state = current_gpio;

            if ( triggered ) {
                DEBUG_LA_PRINTF( "◆ GPIO edge trigger: 0x%08X\n", current_gpio );
                return true;
            }
        } else {
            // Level trigger: match current pattern
            if ( ( current_gpio & gpio_trigger_mask ) ==
                 ( gpio_trigger_pattern & gpio_trigger_mask ) ) {
                DEBUG_LA_PRINTF( "◆ GPIO level trigger: 0x%08X\n", current_gpio );
                return true;
            }
        }
        return false;
    }

    case TRIGGER_THRESHOLD: {
        // Read current ADC value from specified channel
        if ( threshold_channel >= 8 )
            return false; // Invalid channel

        adc_select_input( threshold_channel );
        uint16_t adc_raw = adc_read( );

        // Convert ADC reading to voltage using calibration
        int16_t baseline = getAnalogBaseline( threshold_channel );
        int32_t calibrated_adc = adc_raw + ( 2048 - baseline );
        if ( calibrated_adc < 0 )
            calibrated_adc = 0;
        if ( calibrated_adc > 4095 )
            calibrated_adc = 4095;

        // Convert to voltage (assuming 18.28V spread from calibration)
        float current_voltage = ( (float)calibrated_adc * 18.28f / 4095.0f ) - 9.14f;

        bool triggered = false;

        if ( threshold_rising ) {
            // Rising edge trigger: voltage crosses above threshold
            triggered = ( last_adc_voltage < threshold_voltage &&
                          current_voltage >= threshold_voltage );
        } else {
            // Falling edge trigger: voltage crosses below threshold
            triggered = ( last_adc_voltage > threshold_voltage &&
                          current_voltage <= threshold_voltage );
        }

        last_adc_voltage = current_voltage;

        if ( triggered && !threshold_triggered ) {
            threshold_triggered = true; // Prevent multiple triggers
            DEBUG_LA_PRINTF( "◆ Threshold trigger: CH%d %.3fV crossed %.3fV (%s)\n",
                             threshold_channel, current_voltage, threshold_voltage,
                             threshold_rising ? "rising" : "falling" );
            return true;
        }
        return false;
    }

    default:
        return false;
    }
}

void setTriggerMode( trigger_type_t mode ) {
    trigger_mode = mode;
    trigger_la = false;                // Reset external trigger
    last_gpio_state = gpio_get_all( ); // Initialize GPIO state
    threshold_triggered = false;       // Reset threshold trigger

    const char* mode_str = "UNKNOWN";
    switch ( mode ) {
    case TRIGGER_NONE:
        mode_str = "NONE (auto-start)";
        break;
    case TRIGGER_EXTERNAL:
        mode_str = "EXTERNAL (trigger_la variable)";
        break;
    case TRIGGER_GPIO:
        mode_str = "GPIO";
        break;
    case TRIGGER_THRESHOLD:
        mode_str = "THRESHOLD (analog)";
        break;
    }
    DEBUG_LA_PRINTF( "◆ Trigger mode set to: %s\n", mode_str );
}

void setGPIOTrigger( uint32_t pin_mask, uint32_t pattern, bool edge_trigger ) {
    gpio_trigger_mask = pin_mask;
    gpio_trigger_pattern = pattern;
    gpio_trigger_edge = edge_trigger;
    last_gpio_state = gpio_get_all( );

    DEBUG_LA_PRINTF(
        "◆ GPIO trigger configured: mask=0x%08X, pattern=0x%08X, edge=%s\n",
        pin_mask, pattern, edge_trigger ? "YES" : "NO" );
}

void setThresholdTrigger( uint8_t channel, float voltage, bool rising_edge ) {
    threshold_channel = channel;
    threshold_voltage = voltage;
    threshold_rising = rising_edge;
    threshold_triggered = false; // Reset trigger state

    // Initialize baseline reading
    if ( channel < 8 ) {
        adc_select_input( channel );
        uint16_t adc_raw = adc_read( );
        int16_t baseline = getAnalogBaseline( channel );
        int32_t calibrated_adc = adc_raw + ( 2048 - baseline );
        if ( calibrated_adc < 0 )
            calibrated_adc = 0;
        if ( calibrated_adc > 4095 )
            calibrated_adc = 4095;
        last_adc_voltage = ( (float)calibrated_adc * 18.28f / 4095.0f ) - 9.14f;
    }

    DEBUG_LA_PRINTF(
        "◆ Threshold trigger configured: CH%d, %.3fV %s edge (current: %.3fV)\n",
        channel, voltage, rising_edge ? "rising" : "falling", last_adc_voltage );
}

void clearTrigger( ) {
    setTriggerMode( TRIGGER_NONE );
    gpio_trigger_mask = 0x00;
    gpio_trigger_pattern = 0x00;
    gpio_trigger_edge = false;
    threshold_channel = 0;
    threshold_voltage = 2.5f;
    threshold_rising = true;
    threshold_triggered = false;
    last_adc_voltage = 0.0f;
    DEBUG_LA_PRINTLN( "◆ All triggers cleared" );
}

// =============================================================================
// PROTOCOL HANDLERS
// =============================================================================

void configureModeAndChannels( LogicAnalyzerMode mode,
                               uint32_t analog_channel_mask ) {
    // analog_mask = analog_channel_mask;

    // Count enabled analog channels
    analog_chan_count = 0;
    for ( int i = 0; i < 32; i++ ) {
        if ( analog_mask & ( 1UL << i ) ) {
            analog_chan_count++;
        }
    }

    // Determine mode based on actual channels enabled (override driver's
    // suggestion)
    if ( analog_chan_count > 0 ) {
        current_la_mode =
            LA_MODE_MIXED_SIGNAL; // Any analog channels = mixed signal mode
        DEBUG_LA_PRINTF(
            "◆ Switching to mixed-signal mode (%d analog channels enabled)\n",
            analog_chan_count );
    } else {
        current_la_mode = LA_MODE_DIGITAL_ONLY;
        DEBUG_LA_PRINTF( "◆ Switching to digital-only mode (no analog channels)\n" );
    }

    // Stop any ongoing capture when mode changes
    la_capture_state = JL_LA_STOPPED;

    // Recalculate buffer configuration based on new mode and channel settings
    UnifiedBufferRequirements req = calculateUnifiedBufferRequirements( );
    uint32_t physical_max_samples = req.max_samples_final;
    uint32_t effective_max_samples =
        calculateEffectiveSampleCount( physical_max_samples );
    jl_la_max_samples = effective_max_samples;
    if ( sample_count > effective_max_samples ) {
        sample_count = effective_max_samples;
    }

    DEBUG_LA_PRINTF( "◆ Mode configured: %d, analog channels: %d (mask=0x%08X), "
                     "max samples: %lu (physical: %lu, effective: %lu)\n",
                     current_la_mode, analog_chan_count, analog_mask,
                     jl_la_max_samples, physical_max_samples,
                     effective_max_samples );

    // Header will be sent by the command handler that triggered this
    // configuration Don't send duplicate headers here to avoid confusing the
    // driver
}

void sendSUMPID( ) {
    if ( !la_usb_connected( ) ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: USB not connected for SUMP ID" );
        return;
    }

    const char sump_id[] = "1SLO";
    DEBUG_LA2_PRINTLN( "◆ Sending SUMP ID response..." );
    la_usb_write_buffer( (const uint8_t*)sump_id, 4 );
    la_usb_flush( );
    DEBUG_LA_PRINTLN( "◆ SUMP ID sent successfully: 1SLO" );
}

void sendJumperlessHeader( ) {
    DEBUG_LA2_PRINTLN( "◆ Sending Jumperless header..." );

    if ( !la_usb_connected( ) ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: USB not connected" );
        return;
    }

    struct {
        char magic[ 8 ];
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
        char firmware_version[ 16 ];
        char device_id[ 16 ];
        uint32_t checksum;
    } __attribute__( ( packed ) ) header;

    memset( &header, 0, sizeof( header ) );
    strncpy( header.magic, "$JLDATA", sizeof( header.magic ) );
    header.version = 2;
    header.capture_mode = current_la_mode;
    header.max_digital_channels = 16; // Updated to match driver expectations
    header.max_analog_channels =
        14; // Updated to match actual hardware capability
    header.sample_rate = sample_rate;
    header.sample_count = sample_count;
    header.digital_channel_mask = 0xFF;
    header.analog_channel_mask = analog_mask;

    // ALWAYS use mixed-signal mode (32 bytes per sample)
    current_la_mode = LA_MODE_MIXED_SIGNAL;
    header.bytes_per_sample = 32;        // Always 32 bytes per sample
    header.digital_bytes_per_sample = 2; // Always 3 bytes for digital data
    header.analog_bytes_per_sample = 29; // Always 29 bytes for analog data

    header.adc_resolution_bits = 12;
    header.analog_voltage_range = 18.28f;
    header.max_sample_rate = JL_LA_MAX_SAMPLE_RATE;

    // Use actual allocated buffer sizes for max_memory_depth instead of
    // theoretical maximum This prevents the UI from showing incorrect sample
    // limits after memory fragmentation
    uint32_t actual_max_samples = 0;
    if ( jl_la_max_samples > 0 ) {
        // Use the actual allocated sample count (updated after buffer allocation)
        actual_max_samples = jl_la_max_samples;
        DEBUG_LA_PRINTF( "◆ HEADER: Using actual allocated max samples: %lu\n",
                         actual_max_samples );
    } else {
        // Fallback to theoretical calculation if no buffers are allocated
        UnifiedBufferRequirements req_header = calculateUnifiedBufferRequirements( );
        uint32_t physical_max_samples = req_header.max_samples_final;
        actual_max_samples = calculateEffectiveSampleCount( physical_max_samples );
        DEBUG_LA_PRINTF(
            "◆ HEADER: Using theoretical max samples: %lu (no buffers allocated)\n",
            actual_max_samples );
    }

    header.max_memory_depth = actual_max_samples;

    header.supported_modes = 0x07; // Digital, Analog, and Mixed
    strncpy( header.firmware_version, "RP2350-v2.1",
             sizeof( header.firmware_version ) );
    strncpy( header.device_id, "Jumperless-LA", sizeof( header.device_id ) );

    // Calculate checksum
    uint8_t* header_bytes = (uint8_t*)&header;
    uint32_t checksum = 0;
    for ( size_t i = 0; i < sizeof( header ) - sizeof( header.checksum ); i++ ) {
        checksum ^= header_bytes[ i ];
    }
    header.checksum = checksum;

    // Send header response with SOH/EOH framing
    la_usb_write( JUMPERLESS_RESP_HEADER ); // 0x80
    la_usb_write( 0xAF );                   // SOH (Start of Header)
    la_usb_write_buffer( (uint8_t*)&header, sizeof( header ) );
    la_usb_write( 0xBF ); // EOH (End of Header)
    la_usb_flush( );

    DEBUG_LA_PRINTF(
        "◆ Header sent: %d samples, %d bytes/sample, %d analog channels\n",
        sample_count, header.bytes_per_sample, analog_chan_count );
}

void sendStatusResponse( uint8_t status ) {
    if ( !enhanced_mode )
        return;

    la_usb_write( JUMPERLESS_RESP_STATUS );
    la_usb_write( status );
    la_usb_flush( );

    DEBUG_LA_PRINTF( "◆ Sending status response: 0x%02X with status 0x%02X\n",
                     JUMPERLESS_RESP_STATUS, status );
}

void sendEndOfDataSignal( ) {
    if ( !enhanced_mode )
        return;

    DEBUG_LA_PRINTLN( "◆ Sending end-of-data signal..." );
    la_usb_write( JUMPERLESS_RESP_END_DATA );
    la_usb_write( 0x00 ); // Status: success
    la_usb_flush( );
    DEBUG_LA2_PRINTLN( "◆ End-of-data signal sent" );
}

void sendErrorResponse( uint8_t error_code ) {
    // la_usb_write(JUMPERLESS_RESP_ERROR);
    la_usb_write( error_code );
    la_usb_flush( );
}

void processCommand( uint8_t cmd ) {
    DEBUG_LA_PRINTF( "◆ Command: 0x%02X\n", cmd );
    updateLastActivity( );

    // Handle SUMP Protocol Commands
    switch ( cmd ) {
    case JL_CMD_RESET:
        la_capture_state = JL_LA_STOPPED;
        la_capturing = false;
        clearTrigger( ); // Reset trigger state
        DEBUG_LA_PRINTLN( "◆ Device reset (triggers cleared)" );
        if ( enhanced_mode )
            sendStatusResponse( 0x00 );
        break;

    case JL_CMD_ID:
        DEBUG_LA_PRINTLN( "◆ Processing ID command (0xA2)" );
        sendSUMPID( );
        DEBUG_LA_PRINTLN( "◆ ID command completed" );
        break;

    case JL_CMD_GET_HEADER:
        enhanced_mode = true;
        sendJumperlessHeader( );
        break;

    case JL_CMD_SET_CHANNELS:
        enhanced_mode = true;
        delay( 10 );
        if ( la_usb_available( ) >= 8 ) {
            // Read digital mask (4 bytes) - not used currently but consumed
            uint32_t digital_mask = 0;
            for ( int i = 0; i < 4; i++ ) {
                digital_mask |= ( la_usb_read( ) << ( i * 8 ) );
            }

            // Read analog mask (4 bytes)
            uint32_t analog_mask_new = 0;
            for ( int i = 0; i < 4; i++ ) {
                analog_mask_new |= ( la_usb_read( ) << ( i * 8 ) );
            }

            // Count enabled analog channels
            uint8_t new_analog_chan_count = 0;
            for ( int i = 0; i < 32; i++ ) {
                if ( analog_mask_new & ( 1UL << i ) ) {
                    new_analog_chan_count++;
                    DEBUG_LA_PRINTF( "◆ Analog channel %d enabled in mask\n", i );
                }
            }
            DEBUG_LA_PRINTF( "◆ Total analog channels detected: %d (mask=0x%08X)\n",
                             new_analog_chan_count, analog_mask_new );
            analog_mask = analog_mask_new;
            // Configure mode based on channel selection
            if ( analog_mask_new > 0 ) {
                configureModeAndChannels( LA_MODE_MIXED_SIGNAL, analog_mask_new );
                DEBUG_LA_PRINTF( "◆ Channels configured: Mixed-signal mode with %d "
                                 "analog channels\n",
                                 analog_chan_count );
            } else {
                configureModeAndChannels( LA_MODE_DIGITAL_ONLY, 0x00 );
                DEBUG_LA_PRINTF( "◆ Channels configured: Digital-only mode\n" );
            }

            sendStatusResponse( 0x00 );

            // Give the driver a moment to process the status response before we send
            // the unsolicited header. This helps prevent race conditions.
            delay( 10 );

            // Send the updated header to inform the driver of new capabilities,
            // such as a new maximum sample depth based on the channel config.
            sendJumperlessHeader( );

        } else {
            sendErrorResponse( 0x01 );
        }
        sendStatusResponse( 0x00 );
        break;

    case JL_CMD_ARM:
        if ( la_capture_state == JL_LA_STOPPED || la_capture_state == JL_LA_ARMED ) {
            logicAnalyzing = true;
            // Allow arming from STOPPED state or re-arming from ARMED state
            if ( la_capture_state == JL_LA_STOPPED ) {
                la_capture_state = JL_LA_ARMED;
            }

            //     if (trigger_mode == TRIGGER_NONE) {
            //         DEBUG_LA_PRINTLN("◆ No triggers configured - starting capture
            //         immediately");

            //         if (setupCapture() && startCapture()) {
            //             DEBUG_LA_PRINTLN("◆ Auto-capture started successfully");
            //             sendStatusResponse(0x00);
            //         } else {
            //             DEBUG_LA_PRINTLN("◆ Auto-capture failed to start");
            //             sendStatusResponse(0x02); // Send proper status response
            //             with error code
            //         }
            //     } else {
            //         const char* trigger_desc = "unknown";
            //         if (trigger_mode == TRIGGER_EXTERNAL) {
            //             trigger_desc = "external";
            //         } else if (trigger_mode == TRIGGER_GPIO) {
            //             trigger_desc = "GPIO";
            //         } else if (trigger_mode == TRIGGER_THRESHOLD) {
            //             trigger_desc = "threshold";
            //         }
            //         DEBUG_LA_PRINTF("◆ Device armed - waiting for trigger (mode:
            //         %s, protocol: %s)\n",
            //                      trigger_desc, enhanced_mode ? "Enhanced" :
            //                      "SUMP");
            //         sendStatusResponse(0x00);
            //     }
            // } else {
            //     DEBUG_LA_PRINTF("◆ Cannot arm in current state: %d\n",
            //     la_capture_state); sendStatusResponse(0x02); // Send proper status
            //     response with error code
            // }
        }

        sendStatusResponse( 0x00 );
        break;

    case JL_CMD_RUN:
        if ( la_capture_state == JL_LA_STOPPED || la_capture_state == JL_LA_ARMED ) {
            logicAnalyzing = true;
            // RUN command always starts immediately, regardless of trigger settings
            if ( la_capture_state == JL_LA_STOPPED ) {
                la_capture_state = JL_LA_ARMED;
            }

            DEBUG_LA_PRINTLN( "◆ RUN command - forcing immediate capture start" );

            // Safety check: ensure valid sample count
            if ( sample_count == 0 ) {
                sample_count = JL_LA_DEFAULT_SAMPLES;
                DEBUG_LA_PRINTF( "◆ SAFETY: Zero sample count, using default %lu\n",
                                 sample_count );
            }

            DEBUG_LA_PRINTF( "◆ Starting capture: rate=%lu Hz, samples=%lu, mode=%d\n",
                             sample_rate, sample_count, current_la_mode );

            if ( setupCapture( ) && startCapture( ) ) {
                DEBUG_LA_PRINTLN( "◆ RUN command: Capture started successfully" );
                sendStatusResponse( 0x00 );
            } else {
                DEBUG_LA_PRINTLN( "◆ RUN command: ERROR - Failed to start capture" );
                la_capture_state = JL_LA_STOPPED;
                sendStatusResponse( 0x02 ); // Send proper status response with error code
            }
        } else if ( la_capture_state == JL_LA_TRIGGERED ) {
            //     // If capture is in progress, acknowledge the RUN command
            //     DEBUG_LA_PRINTLN("◆ RUN command: Capture already in progress,
            //     acknowledging"); sendStatusResponse(0x00);
            // } else if (la_capture_state == JL_LA_COMPLETE) {
            //     // If capture is already complete, the data has already been sent.
            //     // Just acknowledge the command.
            //     DEBUG_LA_PRINTLN("◆ RUN command: Capture complete, acknowledging");
            //     sendStatusResponse(0x00);
            // } else {
            //     DEBUG_LA_PRINTF("◆ Cannot run in current state: %d\n",
            //     la_capture_state); sendStatusResponse(0x02); // Send proper status
            //     response with error code
        }
        sendStatusResponse( 0x00 );
        break;

    case JUMPERLESS_CMD_CONFIGURE:
        enhanced_mode = true;
        delay( 10 );
        if ( la_usb_available( ) >= 8 ) {
            uint32_t new_sample_rate = 0;
            uint32_t new_sample_count = 0;

            for ( int i = 0; i < 4; i++ ) {
                new_sample_rate |= ( la_usb_read( ) << ( i * 8 ) );
            }
            for ( int i = 0; i < 4; i++ ) {
                new_sample_count |= ( la_usb_read( ) << ( i * 8 ) );
            }

            if ( new_sample_rate > 0 && new_sample_rate <= JL_LA_MAX_SAMPLE_RATE ) {
                sample_rate = new_sample_rate;
            } else {
                sample_rate = 1000000;
            }

            if ( new_sample_count > 0 && new_sample_count <= jl_la_max_samples ) {
                sample_count = new_sample_count;
            } else if ( new_sample_count == 0 ) {
                sample_count = JL_LA_MIN_SAMPLES;
            } else {
                sample_count = jl_la_max_samples;
            }

            DEBUG_LA_PRINTF( "◆ Config: rate=%lu Hz, samples=%lu\n", sample_rate,
                             sample_count );
            sendStatusResponse( 0x00 );

            // After configuring timing, send an updated header back to the driver
            // so it knows about new capabilities (e.g., max samples with
            // oversampling).
            delay( 20 ); // Give driver a moment to process status
            sendJumperlessHeader( );

        } else {
            sample_rate = 1000000;
            sample_count = JL_LA_DEFAULT_SAMPLES;
            sendStatusResponse( 0x00 );
        }
        break;

    case JUMPERLESS_CMD_SET_MODE:
        enhanced_mode = true;
        delay( 10 );
        if ( la_usb_available( ) >= 1 ) {
            uint8_t new_mode = la_usb_read( );

            // Set mode with sensible default channel configurations
            switch ( new_mode ) {
            case 0: // Digital only
                configureModeAndChannels( LA_MODE_DIGITAL_ONLY, 0x00 );
                break;
            case 1: // Mixed signal - enable first 8 ADC channels by default
                configureModeAndChannels( LA_MODE_MIXED_SIGNAL, 0xFF );
                break;
            case 2: // Analog only - enable all 14 channels
                configureModeAndChannels( LA_MODE_ANALOG_ONLY, 0x3FFF );
                break;
            default:
                sendErrorResponse( 0x01 );
                return;
            }

            DEBUG_LA_PRINTF( "◆ Mode set via SET_MODE command: %d\n", new_mode );
            DEBUG_LA_PRINTLN(
                "◆ NOTE: Use SET_CHANNELS (0xA4) for precise channel control" );

            // Send status response first
            sendStatusResponse( 0x00 );

            // Small delay to ensure status response is processed, then send updated
            // header
            delay( 10 );
            sendJumperlessHeader( );

            DEBUG_LA_PRINTLN(
                "◆ Mode configuration complete with updated header sent" );
        } else {
            sendErrorResponse( 0x01 );
        }
        break;

    case JUMPERLESS_CMD_SET_TRIGGER:
        enhanced_mode = true;
        delay( 10 );
        if ( la_usb_available( ) >= 13 ) { // Updated to handle threshold trigger data
            uint8_t trigger_type = la_usb_read( );
            uint32_t mask = 0;
            uint32_t pattern = 0;

            // Read mask (4 bytes)
            for ( int i = 0; i < 4; i++ ) {
                mask |= ( la_usb_read( ) << ( i * 8 ) );
            }
            // Read pattern (4 bytes)
            for ( int i = 0; i < 4; i++ ) {
                pattern |= ( la_usb_read( ) << ( i * 8 ) );
            }

            switch ( trigger_type ) {
            case 0: // No trigger (auto-start)
                setTriggerMode( TRIGGER_NONE );
                break;
            case 1: // External trigger
                setTriggerMode( TRIGGER_EXTERNAL );
                break;
            case 2: // GPIO level trigger
                setTriggerMode( TRIGGER_GPIO );
                setGPIOTrigger( mask, pattern, false );
                break;
            case 3: // GPIO edge trigger
                setTriggerMode( TRIGGER_GPIO );
                setGPIOTrigger( mask, pattern, true );
                break;
            case 4:                               // Threshold trigger (rising)
            case 5:                               // Threshold trigger (falling)
                if ( la_usb_available( ) >= 4 ) { // Additional 4 bytes for threshold data
                    setTriggerMode( TRIGGER_THRESHOLD );

                    // Read threshold data: [channel:1][voltage:4 bytes float]
                    uint8_t channel = mask & 0xFF; // Use mask as channel

                    // Read voltage as float (little-endian)
                    union {
                        uint32_t i;
                        float f;
                    } voltage_union;
                    voltage_union.i = pattern; // Use pattern as voltage bytes

                    bool rising = ( trigger_type == 4 );
                    setThresholdTrigger( channel, voltage_union.f, rising );
                } else {
                    sendErrorResponse( 0x01 ); // Insufficient data
                    return;
                }
                break;
            default:
                sendErrorResponse( 0x01 ); // Invalid trigger type
                return;
            }

            DEBUG_LA_PRINTF(
                "◆ Trigger configured: type=%d, mask=0x%08X, pattern=0x%08X\n",
                trigger_type, mask, pattern );
            sendStatusResponse( 0x00 );
        } else {
            sendErrorResponse( 0x01 ); // Insufficient data
        }
        break;

    case JUMPERLESS_CMD_CLEAR_TRIGGER:
        enhanced_mode = true;
        clearTrigger( );
        sendStatusResponse( 0x00 );
        logicAnalyzing = false;
        break;
    case SUMP_SET_DIVIDER:
        if ( la_usb_available( ) >= 3 ) {
            uint32_t divider = 0;
            divider = ( la_usb_read( ) << 16 ) | ( la_usb_read( ) << 8 ) | la_usb_read( );
            uint32_t clock_freq = 100000000;
            uint32_t new_sample_rate =
                ( divider == 0 ) ? clock_freq : ( clock_freq / ( divider + 1 ) );
            if ( new_sample_rate < 100 )
                new_sample_rate = 100;
            if ( new_sample_rate > JL_LA_MAX_SAMPLE_RATE )
                new_sample_rate = JL_LA_MAX_SAMPLE_RATE;
            sample_rate = new_sample_rate;
            DEBUG_LA_PRINTF( "◆ SUMP rate: %lu Hz\n", sample_rate );
            updateLastActivity( );
        }
        break;

    case SUMP_SET_READ_DELAY:
        if ( la_usb_available( ) >= 4 ) {
            uint16_t read_count = ( la_usb_read( ) << 8 ) | la_usb_read( );
            uint16_t delay_count = ( la_usb_read( ) << 8 ) | la_usb_read( );
            uint32_t new_sample_count = ( read_count + 1 ) * 4;
            if ( new_sample_count < JL_LA_MIN_SAMPLES )
                new_sample_count = JL_LA_MIN_SAMPLES;
            if ( new_sample_count > jl_la_max_samples )
                new_sample_count = jl_la_max_samples;
            sample_count = new_sample_count;
            DEBUG_LA_PRINTF( "◆ SUMP samples: %lu\n", sample_count );
            updateLastActivity( );
        }
        break;

    case SUMP_SET_FLAGS:
        if ( la_usb_available( ) >= 4 ) {
            uint32_t flags = 0;
            for ( int i = 0; i < 4; i++ ) {
                flags |= ( la_usb_read( ) << ( i * 8 ) );
            }
            DEBUG_LA_PRINTF( "◆ SUMP flags: 0x%08X\n", flags );
        }
        break;

    // SUMP Trigger commands (consume data but don't implement)
    case SUMP_SET_TRIGGER_MASK_0:
    case SUMP_SET_TRIGGER_VALUE_0:
        if ( la_usb_available( ) >= 4 ) {
            for ( int i = 0; i < 4; i++ )
                la_usb_read( );
        }
        break;

    default:
        // Handle other SUMP trigger commands
        if ( ( cmd >= 0xC0 && cmd <= 0xCF ) || ( cmd >= 0xD0 && cmd <= 0xDF ) ) {
            if ( la_usb_available( ) >= 4 ) {
                for ( int i = 0; i < 4; i++ )
                    la_usb_read( );
            }
        } else {
            DEBUG_LA_PRINTF( "◆ Unknown SUMP command: 0x%02X\n", cmd );
        }
        break;
    }
}

// =============================================================================
// CAPTURE IMPLEMENTATION
// =============================================================================

bool setupCapture( ) {
    DEBUG_LA_PRINTLN( "◆ Setting up capture..." );
    if ( !allocateLogicAnalyzerResources( ) ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Failed to allocate resources" );
        return false;
    }
    DEBUG_LA_PRINTLN( "◆ Resources allocated successfully" );

    // Determine the actual number of samples to DMA, capped by our buffer size.
    uint32_t samples_to_capture = sample_count;
    if ( samples_to_capture > jl_la_max_samples ) {
        DEBUG_LA_PRINTF( "◆ WARNING: Requested samples (%lu) exceeds buffer "
                         "capacity (%lu). Capping capture.\n",
                         samples_to_capture, jl_la_max_samples );
        samples_to_capture = jl_la_max_samples;
    }

    // For mixed-signal mode, analog buffer is already allocated by
    // allocateAllLogicAnalyzerBuffers()
    if ( current_la_mode == LA_MODE_MIXED_SIGNAL ||
         current_la_mode == LA_MODE_ANALOG_ONLY ) {
        // Analog buffer is already allocated - just verify it exists
        if ( !analog_buffer ) {
            DEBUG_LA_PRINTLN(
                "◆ ERROR: Analog buffer not allocated by atomic function" );
            return false;
        }

        // The analog buffer was already allocated by allocateAllLogicAnalyzerBuffers()
        // Just verify it's the right size for what we need
        size_t analog_buffer_size_needed = analog_buffer_size; // Use the size that was already allocated

        DEBUG_LA_PRINTF(
            "◆ Analog buffer verification: %lu bytes allocated for %d channels\n",
            analog_buffer_size_needed, analog_chan_count );

        // Verify the analog buffer is properly allocated
        if ( !analog_buffer || analog_buffer_size == 0 ) {
            DEBUG_LA_PRINTLN( "◆ ERROR: Analog buffer not properly allocated" );
            return false;
        }

        DEBUG_LA_PRINTF(
            "◆ Using pre-allocated analog buffer: %lu bytes (%d channels)\n",
            analog_buffer_size, analog_chan_count );

        // Clear analog buffer
        memset( analog_buffer, 0, analog_buffer_size );
        analog_capture_complete = false;
    }

    // Configure GPIO pins
    for ( int i = 0; i < JL_LA_PIN_COUNT; i++ ) {
        gpio_init( JL_LA_PIN_BASE + i );
    }

    // CRITICAL: Clear digital buffer to prevent contamination from previous
    // captures
    memset( la_buffer, 0, jl_la_buffer_size );
    DEBUG_LA_PRINTLN( "◆ Capture buffers cleared" );

    // Configure PIO
    pio_sm_set_enabled( la_pio, la_sm, false );
    pio_sm_clear_fifos( la_pio, la_sm );
    pio_sm_restart( la_pio, la_sm );

    pio_sm_config c = pio_get_default_sm_config( );
    sm_config_set_in_pins( &c, JL_LA_PIN_BASE );
    sm_config_set_in_shift( &c, false, true, 8 );
    sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );

    uint program_end = la_prog_offset + la_pio_program_fast.length - 1;
    sm_config_set_wrap( &c, la_prog_offset, program_end );

    uint32_t sys_clock = clock_get_hz( clk_sys );
    float clock_div = (float)sys_clock / sample_rate / 2;
    if ( clock_div < 1.0f )
        clock_div = 1.0f;
    if ( clock_div > 65536.0f )
        clock_div = 65536.0f;

    sm_config_set_clkdiv( &c, clock_div );
    pio_sm_init( la_pio, la_sm, la_prog_offset, &c );

    // Configure DMA for digital capture
    dma_channel_config dma_config = dma_channel_get_default_config( la_dma_chan );
    channel_config_set_transfer_data_size( &dma_config, DMA_SIZE_8 );
    channel_config_set_read_increment( &dma_config, false );
    channel_config_set_write_increment( &dma_config, true );
    channel_config_set_dreq( &dma_config, pio_get_dreq( la_pio, la_sm, false ) );

    dma_channel_configure( la_dma_chan, &dma_config, la_buffer,
                           &la_pio->rxf[ la_sm ], samples_to_capture, false );

    DEBUG_LA_PRINTLN( "◆ Capture setup complete" );
    return true;
}

bool captureAnalogData( void );

bool startCapture( ) {
    DEBUG_LA_PRINTF( "◆ Starting capture (state=%d)...\n", la_capture_state );
    if ( la_capture_state != JL_LA_ARMED ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Cannot start capture in state %d\n",
                         la_capture_state );
        return false;
    }

    la_capturing = true;
    la_capture_state = JL_LA_TRIGGERED;
    la_counter = 0; // Reset counter for new capture

    captureAnalogData( );

    DEBUG_LA_PRINTF( "◆ Capture started - PIO%d SM%d and DMA active, counter=%d\n",
                     pio_get_index( la_pio ), la_sm, la_counter );
    return true;
}

bool isCaptureDone( ) {
    if ( la_capture_state != JL_LA_TRIGGERED )
        return false;

    if ( !dma_channel_is_busy( la_dma_chan ) ) {
        DEBUG_LA_PRINTF( "◆ Capture complete: %lu samples\n", sample_count );
        la_capturing = false;
        la_capture_state = JL_LA_COMPLETE;
        return true;
    }
    return false;
}

int16_t getAnalogBaseline( int channel ) {
    int16_t analog_offset = 0;

    switch ( channel ) {
    case 0:
        if ( jumperlessConfig.calibration.adc_0_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_0_zero /
                                 jumperlessConfig.calibration.adc_0_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 1:
        if ( jumperlessConfig.calibration.adc_1_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_1_zero /
                                 jumperlessConfig.calibration.adc_1_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 2:
        if ( jumperlessConfig.calibration.adc_2_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_2_zero /
                                 jumperlessConfig.calibration.adc_2_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 3:
        if ( jumperlessConfig.calibration.adc_3_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_3_zero /
                                 jumperlessConfig.calibration.adc_3_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 4:
        if ( jumperlessConfig.calibration.adc_4_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_4_zero /
                                 jumperlessConfig.calibration.adc_4_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 5:
        analog_offset = jumperlessConfig.calibration.probe_min - 2048;
        break;
    case 7:
        if ( jumperlessConfig.calibration.adc_7_spread != 0.0f ) {
            float offset_adc = ( jumperlessConfig.calibration.adc_7_zero /
                                 jumperlessConfig.calibration.adc_7_spread ) *
                               4095.0f;
            analog_offset = (int16_t)round( offset_adc );
        }
        break;
    case 8:
        analog_offset = jumperlessConfig.calibration.dac_0_zero - 2048;
        break;
    case 9:
        analog_offset = jumperlessConfig.calibration.dac_1_zero - 2048;
        break;
    default:
        analog_offset = 0;
        break;
    }

    return analog_offset;
}

bool captureAnalogData( ) {
    if ( !analog_buffer ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Analog buffer not allocated" );
        return false;
    }

    // Reset persistent oversampling state for new capture
    capture_used_oversampling = false;
    capture_duplication_factor = 1;

    if ( adc_oversampling_mode ) {
        DEBUG_LA_PRINTF( "◆ Capturing analog data: %lu samples requested at %lu Hz, "
                         "ADC runs at %lu Hz (OVERSAMPLING)\n",
                         sample_count, sample_rate, adc_actual_sample_rate );
        DEBUG_LA_PRINTF( "◆ %d channels (mask=0x%08X), capturing %lu ADC samples "
                         "(dup_factor=%lu)\n",
                         analog_chan_count, analog_mask,
                         ( sample_count + adc_duplication_factor - 1 ) /
                             adc_duplication_factor,
                         adc_duplication_factor );
    } else {
        DEBUG_LA_PRINTF( "◆ Capturing analog data: %lu samples at %lu Hz from %d "
                         "channels (mask=0x%08X)\n",
                         sample_count, sample_rate, analog_chan_count, analog_mask );
    }

    // Try DMA-based ADC capture first (fastest and most precise)
    if ( initAdcDma( sample_rate ) ) {
        DEBUG_LA_PRINTLN( "◆ Attempting DMA-based ADC capture..." );
        if ( captureAnalogDataDMA( ) ) {
            DEBUG_LA_PRINTLN( "◆ DMA analog capture successful!" );
            analog_capture_complete = true;
            cleanupAdcDma( ); // Clean up DMA resources
            return true;
        } else {
            DEBUG_LA_PRINTLN( "◆ DMA capture failed - falling back to timer method" );
            cleanupAdcDma( ); // Clean up failed DMA attempt
        }
    } else {
        DEBUG_LA2_PRINTLN( "◆ DMA initialization failed - using timer method" );
    }
    return false;

    // Channel count was set by allocateAllLogicAnalyzerBuffers()
    analog_capture_complete = true;
    DEBUG_LA_PRINTF( "◆ Analog capture complete: %lu samples × %d channels = %lu "
                     "total readings\n",
                     sample_count, analog_chan_count,
                     sample_count * analog_chan_count );
    return true;
}

void sendDigitalData( ) {
    uint8_t* sample_buffer = (uint8_t*)la_buffer;
    const uint32_t CHUNK_SIZE =
        48;                            // 16 samples * 3 bytes - smaller chunks for better flow control
    static uint8_t chunk_buffer[ 48 ]; // Static buffer for stability
    uint32_t chunk_pos = 0;

    DEBUG_LA_PRINTF(
        "◆ Sending digital data: %lu samples (3-byte unified format)\n",
        sample_count );

    for ( uint32_t sample = 0; sample < sample_count; sample++ ) {
        if ( chunk_pos + 3 > CHUNK_SIZE ) {
            la_usb_write_buffer( chunk_buffer, chunk_pos );
            la_usb_flush( );
            delayMicroseconds( 500 ); // Reduced delay for better throughput
            // yield();  // Allow other cores to run
            updateLastActivity( );
            chunk_pos = 0;
        }

        // UNIFIED DIGITAL FORMAT: Always 3 bytes per sample
        // [byte 0] GPIO data: 8 digital channels
        chunk_buffer[ chunk_pos++ ] = sample_buffer[ sample ];
        // [byte 1] UART data: placeholder for future use
        chunk_buffer[ chunk_pos++ ] = 0x00;
        // [byte 2] Format marker: 0xDD = digital only
        chunk_buffer[ chunk_pos++ ] = DIGITAL_ONLY_MARKER;

        // More frequent yields for better responsiveness
        if ( sample % 1000 == 0 && sample > 0 ) {
            updateLastActivity( );
            // yield();  // Allow other cores to run
        }
    }

    if ( chunk_pos > 0 ) {
        la_usb_write_buffer( chunk_buffer, chunk_pos );
        la_usb_flush( );
        delayMicroseconds( 500 ); // Final stability delay
    }

    DEBUG_LA_PRINTLN( "◆ Digital data sent (3-byte unified format)" );
}

void sendMixedSignalData( ) {
    uint8_t* digital_buffer = (uint8_t*)la_buffer;
    const uint32_t UNIFIED_BYTES_PER_SAMPLE = 32; // Always 32 bytes per sample for driver compatibility
    const uint32_t CHUNK_SIZE = 512;              // Smaller chunks for better flow control - 8 samples per chunk

    // Use static buffer to avoid malloc/free during critical transmission
    static uint8_t chunk_buffer[ 512 ];
    uint32_t chunk_pos = 0;

    // Calculate how many samples we actually have in the buffer
    uint32_t actual_buffer_samples =
        capture_used_oversampling
            ? ( sample_count + capture_duplication_factor - 1 ) /
                  capture_duplication_factor
            : sample_count;

    if ( capture_used_oversampling ) {
        DEBUG_LA_PRINTF(
            "◆ Sending unified mixed-signal data: %lu samples, 32 bytes/sample "
            "(OVERSAMPLING: %lu buffer samples, dup_factor=%lu)\n",
            sample_count, actual_buffer_samples, capture_duplication_factor );
    } else {
        DEBUG_LA_PRINTF( "◆ Sending unified mixed-signal data: %lu samples, 32 "
                         "bytes/sample format\n",
                         sample_count );
    }

    // Progress tracking
    uint32_t expected_analog_channels = 0;
    for ( int i = 0; i < 8; i++ ) {
        if ( analog_mask & ( 1 << i ) ) {
            expected_analog_channels++;
        }
    }

    if ( expected_analog_channels != analog_chan_count ) {
        DEBUG_LA_PRINTF( "◆ WARNING: Channel count mismatch: expected %lu from "
                         "mask, got %d from calculation\n",
                         expected_analog_channels, analog_chan_count );
    }

    // Verify USB connection before starting
    if ( !la_usb_connected( ) ) {
        DEBUG_LA_PRINTLN(
            "◆ ERROR: USB not connected at start of mixed-signal transmission" );
        return;
    }

    // Check if analog data was captured
    if ( !analog_capture_complete || !analog_buffer ) {
        DEBUG_LA_PRINTLN(
            "◆ ERROR: Analog data not captured - run captureAnalogData() first" );
        return;
    }

    uint32_t lastSamples[ 14 ] = { 2048, 2048, 2048, 2048, 2048, 2048, 2048,
                                   2048, 2048, 2048, 2048, 2048, 2048, 2048 };
    for ( uint32_t sample = 0; sample < sample_count; sample++ ) {

        // Check if we need to flush chunk
        if ( chunk_pos + UNIFIED_BYTES_PER_SAMPLE > CHUNK_SIZE ) {
            // STABILITY: Send chunk with better flow control
            while ( la_usb_available( ) ) {
                la_usb_read( );
            }
            la_usb_write_buffer( chunk_buffer, chunk_pos );
            la_usb_flush( );
            // DEBUG_LA_PRINTLN(chunk_buffer);
            // STABILITY: Yield to other cores and prevent USB buffer saturation
            // delayMicroseconds(1500); // Reduced delay for better throughput
            // yield();                // Allow other cores to run
            // updateLastActivity();

            chunk_pos = 0;
        }

        // UNIFIED FORMAT: Always 32 bytes per sample
        // [bytes 0-2] Digital data (GPIO, UART, format marker)
        chunk_buffer[ chunk_pos++ ] = digital_buffer[ sample ];
        chunk_buffer[ chunk_pos++ ] = 0x00; // UART placeholder
        chunk_buffer[ chunk_pos++ ] =
            MIXED_SIGNAL_MARKER; // 0xDA - mixed signal marker

        // [bytes 3-30] Analog data (14 channels × 2 bytes each = 28 bytes)
        for ( int channel = 0; channel < 14; channel++ ) {
            uint16_t adc_value = 2048; // Default center value
            lastSamples[ channel ] = adc_value;

            // Check if this channel was captured (is enabled and < 8)
            if ( channel < 8 && ( analog_mask & ( 1UL << channel ) ) ) {
                // NEW APPROACH: Use AINSEL capture to find the correct sample
                // Search through the captured samples to find one that matches this
                // channel

                // Handle oversampling mode: map output sample to buffer sample range
                uint32_t buffer_sample = capture_used_oversampling
                                             ? sample / capture_duplication_factor
                                             : sample;

                // Search for a sample from the correct channel in this time slice
                bool found_channel_sample = false;
                for ( uint32_t adc_sample_offset = 0;
                      adc_sample_offset < analog_chan_count && !found_channel_sample;
                      adc_sample_offset++ ) {
                    uint32_t buffer_index =
                        buffer_sample * analog_chan_count + adc_sample_offset;

                    // Check bounds
                    size_t max_buffer_elements = analog_buffer_size / sizeof( uint16_t );
                    if ( buffer_index >= max_buffer_elements )
                        break;

                    // Extract embedded channel info from upper 4 bits of sample
                    uint16_t sample_value = analog_buffer[ buffer_index ];
                    uint8_t embedded_channel =
                        ( sample_value >> 12 ) & 0x0F; // Extract channel from bits [15:12]
                    uint16_t adc_result =
                        sample_value &
                        0x0FFF; // Extract 12-bit ADC result from bits [11:0]

                    // Found a sample from the channel we're looking for!
                    if ( embedded_channel == channel ) {
                        adc_value = adc_result;
                        found_channel_sample = true;

                        // Debug: Show what we're reading for first few samples
                        if ( sample < 5 ) {
                            DEBUG_LA2_PRINTF( "◆ EMBEDDED Sample %lu Ch %d: buffer[%lu] = %d "
                                              "(embedded_ch=%d)\n",
                                              sample, channel, buffer_index, adc_value,
                                              embedded_channel );
                        }
                    }
                }

                // If no channel-specific sample was found, use default value
                if ( !found_channel_sample && sample < 5 ) {
                    DEBUG_LA_PRINTF( "◆ No embedded channel data for Sample %lu Ch %d, "
                                     "using default\n",
                                     sample, channel );
                }

            } else if ( sample_count > 1 && sample_rate > 0 ) {
                // Generate test pattern for virtual channels (DAC, INA219, etc.)
                // Keep existing test pattern generation for non-ADC channels
            }

            // Store as little-endian 16-bit values
            chunk_buffer[ chunk_pos++ ] = adc_value & 0xFF;
            chunk_buffer[ chunk_pos++ ] = ( adc_value >> 8 ) & 0xFF;
        }

        // [byte 31] EOF marker: 0xA0
        chunk_buffer[ chunk_pos++ ] = ANALOG_EOF_MARKER;

        // More frequent yields for better responsiveness and USB flow control
        if ( sample % 500 == 0 && sample > 0 ) {
            updateLastActivity( );
            yield( ); // Allow other cores to run
        }
    }

    // Send any remaining data
    if ( chunk_pos >= 32 ) {
        while ( la_usb_available( ) ) {
            la_usb_read( );
        }
        la_usb_write_buffer( chunk_buffer, chunk_pos );

        // DEBUG_LA_PRINTLN(chunk_buffer);
        la_usb_flush( );
        delayMicroseconds( 1500 ); // Final stability delay
    }

    DEBUG_LA2_PRINTF(
        "◆ Unified mixed-signal data sent: 32 bytes/sample format\n" );
}

void sendCaptureData( ) {
    if ( la_capture_state != JL_LA_COMPLETE || !la_buffer ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Cannot send data" );
        return;
    }

    DEBUG_LA_PRINTF( "◆ Sending data: %lu samples, mode=%d\n", sample_count,
                     current_la_mode );

    if ( !la_usb_connected( ) ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: USB not connected" );
        return;
    }

    // Send data response header
    la_usb_write( JUMPERLESS_RESP_DATA );

    // ALWAYS use 32 bytes per sample (mixed-signal format)
    uint32_t transmission_bytes_per_sample = 32;
    uint32_t total_data_size = sample_count * transmission_bytes_per_sample;

    // Verify sample count is valid for our captured buffer
    // (Buffer stores 1 byte per sample regardless of mode)
    uint32_t max_captured_samples =
        jl_la_buffer_size; // 1 byte storage per sample
    if ( sample_count > max_captured_samples ) {
        sample_count = max_captured_samples;
        total_data_size = sample_count * transmission_bytes_per_sample;
        DEBUG_LA_PRINTF( "◆ Adjusted sample count to captured amount: %lu samples\n",
                         sample_count );
    }

    DEBUG_LA_PRINTF( "◆ Data header: %lu bytes (%lu samples × %lu bytes/sample "
                     "transmission)\n",
                     total_data_size, sample_count, transmission_bytes_per_sample );

    // Send data length (what the driver will receive)
    la_usb_write_buffer( (uint8_t*)&total_data_size, 4 );
    la_usb_flush( );
    delay( 1 ); // Allow driver to prepare

    // ALWAYS send mixed-signal data (32-byte format)
    sendMixedSignalData( );

    // Send end-of-data signal to notify driver that transmission is complete
    sendEndOfDataSignal( );

    // Reset for next capture
    la_capture_state = JL_LA_STOPPED;
    DEBUG_LA_PRINTLN( "◆ Data transmission complete, freeing buffers..." );
    releaseLogicAnalyzerResources( );
    DEBUG_LA_PRINTLN( "◆ Buffers freed.\n\n\n\n\r" );
}

// =============================================================================
// MAIN INTERFACE FUNCTIONS
// =============================================================================

// CRITICAL: Function to check for resource conflicts before starting
bool checkLogicAnalyzerConflicts( ) {
    DEBUG_LA_PRINTLN( "◆ Checking for logic analyzer resource conflicts..." );

    bool conflicts_found = false;
    bool pio_available = false;

    // Check PIO availability - we need at least ONE PIO with available state machines
    for ( int i = 0; i < 3; i++ ) {
        PIO test_pio = ( i == 0 ) ? pio0 : ( i == 1 ) ? pio1
                                                      : pio2;
        int test_sm = pio_claim_unused_sm( test_pio, false );
        if ( test_sm >= 0 ) {
            pio_sm_unclaim( test_pio, test_sm );
            DEBUG_LA_PRINTF( "◆ PIO%d: SM%d available\n", i, test_sm );
            pio_available = true; // We found at least one available PIO
        } else {
            DEBUG_LA_PRINTF( "◆ PIO%d: No state machines available\n", i );
            if ( i == 0 ) {
                DEBUG_LA_PRINTLN( "◆ WARNING: PIO0 conflict detected - CH446Q may be using it" );
            } else if ( i == 1 ) {
                DEBUG_LA_PRINTLN( "◆ WARNING: PIO1 conflict detected - Rotary Encoder is using it" );
            } else if ( i == 2 ) {
                DEBUG_LA_PRINTLN( "◆ WARNING: PIO2 conflict detected - NeoPixel LEDs may be using it" );
            }
        }
    }

    // Only treat as conflict if NO PIOs are available
    if ( !pio_available ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: No PIO state machines available - logic analyzer cannot function" );
        conflicts_found = true;
    }

    // Check DMA availability
    int test_dma1 = dma_claim_unused_channel( false );
    int test_dma2 = dma_claim_unused_channel( true );

    if ( test_dma1 >= 0 ) {
        dma_channel_unclaim( test_dma1 );
        DEBUG_LA_PRINTF( "◆ DMA channel %d available for digital capture\n", test_dma1 );
    } else {
        DEBUG_LA_PRINTLN( "◆ ERROR: No DMA channels available for digital capture" );
        conflicts_found = true;
    }

    if ( test_dma2 >= 0 ) {
        dma_channel_unclaim( test_dma2 );
        DEBUG_LA_PRINTF( "◆ DMA channel %d available for ADC capture\n", test_dma2 );
    } else {
        DEBUG_LA_PRINTLN( "◆ ERROR: No DMA channels available for ADC capture" );
        conflicts_found = true;
    }

    if ( conflicts_found ) {
        DEBUG_LA_PRINTLN( "◆ CONFLICTS DETECTED: Logic analyzer may not work properly" );
        return false;
    } else {
        DEBUG_LA_PRINTLN( "◆ No resource conflicts detected" );
        return true;
    }
}

// CRITICAL: Function to temporarily disable rotary encoder if needed
bool disableRotaryEncoderForLogicAnalyzer( ) {
    DEBUG_LA_PRINTLN( "◆ Attempting to disable rotary encoder for logic analyzer..." );

    // Check if rotary encoder is using PIO1
    extern PIO pioEnc;
    extern uint smEnc;

    if ( pioEnc == pio1 ) {
        DEBUG_LA_PRINTLN( "◆ Rotary encoder detected on PIO1 - attempting to free resources" );

        // Try to unclaim the rotary encoder's state machine
        // Note: This is risky and may break rotary encoder functionality
        // Only use if absolutely necessary for logic analyzer operation

        // For now, just warn and suggest manual intervention
        DEBUG_LA_PRINTLN( "◆ WARNING: Rotary encoder conflict detected" );
        DEBUG_LA_PRINTLN( "◆ SUGGESTION: Disable rotary encoder or use different PIO for logic analyzer" );
        return false;
    }

    return true;
}

// =============================================================================
// PUBLIC API FUNCTIONS
// =============================================================================

bool isLogicAnalyzerAvailable( ) { return la_initialized && la_enabled; }

bool isLogicAnalyzerCapturing( ) { return la_capturing; }

void enableLogicAnalyzer( ) {
    if ( la_initialized ) {
        la_enabled = true;
        DEBUG_LA_PRINTLN( "◆ Logic Analyzer enabled" );
    }
}

void disableLogicAnalyzer( ) {
    la_enabled = false;
    la_capture_state = JL_LA_STOPPED;
    DEBUG_LA_PRINTLN( "◆ Logic Analyzer disabled" );
}

void setLogicAnalyzerMode( LogicAnalyzerMode mode ) {
    if ( current_la_mode != mode && la_capture_state == JL_LA_STOPPED ) {
        current_la_mode = mode;

        if ( mode == LA_MODE_MIXED_SIGNAL ) {
            analog_mask = 0xFF;
            analog_chan_count = 8;
            DEBUG_LA_PRINTLN( "◆ Switched to Mixed-Signal mode" );
        } else {
            analog_mask = 0x00;
            analog_chan_count = 0;
            DEBUG_LA_PRINTLN( "◆ Switched to Digital-Only mode" );
        }
    }
}

LogicAnalyzerMode getLogicAnalyzerMode( ) { return current_la_mode; }

// =============================================================================
// TRIGGER API FUNCTIONS
// =============================================================================

void setExternalTrigger( ) {
    setTriggerMode( TRIGGER_EXTERNAL );
    DEBUG_LA_PRINTLN( "◆ External trigger mode enabled - use trigger_la variable" );
}

void triggerCapture( ) {
    trigger_la = true;
    DEBUG_LA_PRINTLN( "◆ External trigger activated" );
}

void setGPIOLevelTrigger( uint32_t pin_mask, uint32_t pattern ) {
    setTriggerMode( TRIGGER_GPIO );
    setGPIOTrigger( pin_mask, pattern, false );
    DEBUG_LA_PRINTF( "◆ GPIO level trigger enabled: pins=0x%08X, pattern=0x%08X\n",
                     pin_mask, pattern );
}

void setGPIOEdgeTrigger( uint32_t pin_mask, uint32_t pattern ) {
    setTriggerMode( TRIGGER_GPIO );
    setGPIOTrigger( pin_mask, pattern, true );
    DEBUG_LA_PRINTF( "◆ GPIO edge trigger enabled: pins=0x%08X, pattern=0x%08X\n",
                     pin_mask, pattern );
}

void setThresholdRisingTrigger( uint8_t channel, float voltage ) {
    setTriggerMode( TRIGGER_THRESHOLD );
    setThresholdTrigger( channel, voltage, true );
    DEBUG_LA_PRINTF( "◆ Threshold rising trigger enabled: CH%d %.3fV\n", channel,
                     voltage );
}

void setThresholdFallingTrigger( uint8_t channel, float voltage ) {
    setTriggerMode( TRIGGER_THRESHOLD );
    setThresholdTrigger( channel, voltage, false );
    DEBUG_LA_PRINTF( "◆ Threshold falling trigger enabled: CH%d %.3fV\n", channel,
                     voltage );
}

void disableTrigger( ) {
    clearTrigger( );
    DEBUG_LA_PRINTLN( "◆ Trigger disabled - auto-start mode" );
}

bool isTriggerActive( ) { return trigger_mode != TRIGGER_NONE; }

const char* getTriggerModeString( ) {
    switch ( trigger_mode ) {
    case TRIGGER_NONE:
        return "NONE";
    case TRIGGER_EXTERNAL:
        return "EXTERNAL";
    case TRIGGER_GPIO:
        return "GPIO";
    case TRIGGER_THRESHOLD:
        return "THRESHOLD";
    default:
        return "UNKNOWN";
    }
}

void stopLogicAnalyzer( ) {
    DEBUG_LA_PRINTLN( "◆ Stopping Logic Analyzer..." );

    la_capturing = false;
    disableLogicAnalyzer( );

    if ( la_usb_connected( ) ) {
        sendStatusResponse( 0xFF ); // Disconnect signal
        delay( 100 );
    }

    handleConnectionStateChange( false );
    releaseLogicAnalyzerResources( );
    la_initialized = false;

    DEBUG_LA_PRINTLN( "◆ Logic Analyzer stopped" );
}

void gracefulLogicAnalyzerShutdown( ) {
    DEBUG_LA_PRINTLN( "◆ Graceful shutdown" );

    if ( la_initialized && la_usb_connected( ) ) {
        sendStatusResponse( 0xFE ); // Shutdown signal
        delay( 100 );
    }

    stopLogicAnalyzer( );
}

void printLogicAnalyzerStatus( ) {
    Serial.println( "◆ Logic Analyzer Status:" );
    Serial.printf( "  Initialized: %s\n", la_initialized ? "YES" : "NO" );
    Serial.printf( "  Enabled: %s\n", la_enabled ? "YES" : "NO" );
    Serial.printf( "  Mode: %s\n", current_la_mode == LA_MODE_MIXED_SIGNAL
                                       ? "Mixed-Signal"
                                       : "Digital-Only" );
    Serial.printf( "  USB Connected: %s\n", la_usb_connected( ) ? "YES" : "NO" );

    if ( la_initialized ) {
        const char* state_str = "UNKNOWN";
        switch ( la_capture_state ) {
        case JL_LA_STOPPED:
            state_str = "STOPPED";
            break;
        case JL_LA_ARMED:
            state_str = "ARMED";
            break;
        case JL_LA_TRIGGERED:
            state_str = "TRIGGERED";
            break;
        case JL_LA_COMPLETE:
            state_str = "COMPLETE";
            break;
        }

        Serial.printf( "  State: %s\n", state_str );
        Serial.printf( "  Sample Rate: %lu Hz\n", sample_rate );
        Serial.printf( "  Sample Count: %lu\n", sample_count );

        if ( current_la_mode == LA_MODE_MIXED_SIGNAL ) {
            Serial.printf( "  Analog Channels: %d (mask=0x%02X)\n", analog_chan_count,
                           analog_mask );
        }

        // Trigger information
        Serial.printf( "  Trigger Mode: %s\n", getTriggerModeString( ) );
        if ( trigger_mode == TRIGGER_GPIO ) {
            Serial.printf( "  GPIO Trigger: mask=0x%08X, pattern=0x%08X, edge=%s\n",
                           gpio_trigger_mask, gpio_trigger_pattern,
                           gpio_trigger_edge ? "YES" : "NO" );
        }
        if ( trigger_mode == TRIGGER_EXTERNAL ) {
            Serial.printf( "  External Trigger Ready: %s\n",
                           trigger_la ? "WAITING" : "ARMED" );
        }
        if ( trigger_mode == TRIGGER_THRESHOLD ) {
            Serial.printf(
                "  Threshold Trigger: CH%d %.3fV %s edge (current: %.3fV)\n",
                threshold_channel, threshold_voltage,
                threshold_rising ? "rising" : "falling", last_adc_voltage );
        }
    }
}

void startLogicAnalyzerCapture( ) {
    if ( la_initialized && la_enabled && la_capture_state == JL_LA_STOPPED ) {
        if ( setupCapture( ) ) {
            la_capture_state = JL_LA_ARMED;
            startCapture( );
        }
    } else {
        DEBUG_LA_PRINTLN( "◆ ERROR: Logic analyzer not ready" );
    }
}

// =============================================================================
// DMA-BASED ADC SAMPLING FUNCTIONS (RP2350B)
// =============================================================================
//
// RP2350B ADC Pin Mapping:
// - Channel 0: GPIO 40 (Pin 40)
// - Channel 1: GPIO 41 (Pin 41)
// - Channel 2: GPIO 42 (Pin 42)
// - Channel 3: GPIO 43 (Pin 43)
// - Channel 4: GPIO 44 (Pin 44)
// - Channel 5: GPIO 45 (Pin 45)
// - Channel 6: GPIO 46 (Pin 46)
// - Channel 7: GPIO 47 (Pin 47)
//
// The RP2350B supports up to 8 ADC channels with hardware round-robin sampling.
// DMA automatically captures samples from all enabled channels in sequence.
//
// unsigned int la_counter= 0;



// DMA interrupt handler that embeds AINSEL in samples
// This will be called by DMA completion interrupt to post-process samples
void __not_in_flash_func( embedAinselInSamples )( ) {
    // Post-process the captured samples to embed AINSEL channel info
    // This function will read each ADC sample and embed the expected channel
    // number based on round-robin position

    if ( !analog_buffer || !adc_dma_complete )
        return;

    // Calculate total transfers completed
    uint32_t total_samples = sample_count;
    uint32_t buffer_samples =
        capture_used_oversampling
            ? ( total_samples + capture_duplication_factor - 1 ) /
                  capture_duplication_factor
            : total_samples;
    uint32_t total_transfers = buffer_samples * analog_chan_count;

    DEBUG_LA_PRINTF(
        "◆ Embedding AINSEL channel info in %lu samples (%d channels)\n",
        total_transfers, analog_chan_count );

    // Embed channel info in each sample based on position in round-robin sequence
    for ( uint32_t transfer_idx = 0; transfer_idx < total_transfers;
          transfer_idx++ ) {
        uint8_t channel_in_sequence = transfer_idx % analog_chan_count;

        // Find which actual channel this sequence position represents
        uint8_t actual_channel = 0;
        uint8_t sequence_pos = 0;
        for ( uint8_t ch = 0; ch < 8; ch++ ) {
            if ( analog_mask & ( 1 << ch ) ) {
                if ( sequence_pos == channel_in_sequence ) {
                    actual_channel = ch;
                    break;
                }
                sequence_pos++;
            }
        }

        // Get the current ADC value and embed channel info in upper 4 bits
        uint16_t adc_value = analog_buffer[ transfer_idx ];
        uint16_t adc_result = adc_value & 0x0FFF; // Keep only 12-bit ADC result
        uint16_t channel_embedded =
            ( actual_channel << 12 ) | adc_result; // Embed channel in bits [15:12]

        // Store back the sample with embedded channel info
        analog_buffer[ transfer_idx ] = channel_embedded;

        // Debug: Show first few embedded samples
        if ( transfer_idx < 10 ) {
            DEBUG_LA_PRINTF(
                "◆ Embedded[%lu]: raw=%d → ch=%d, result=%d, combined=0x%04X\n",
                transfer_idx, adc_value, actual_channel, adc_result,
                channel_embedded );
        }
    }
}

// Initialize DMA-based ADC capture for RP2350B
bool initAdcDma( uint32_t sample_rate ) {
    // Channel count was set by allocateAllLogicAnalyzerBuffers()
    DEBUG_LA_PRINTF( "◆ Initializing RP2350B DMA-based ADC: %lu Hz, %d channels "
                     "(mask=0x%08X)\n",
                     sample_rate, analog_chan_count, analog_mask );

    return true;
}

uint32_t adc_timer_count = 0;

// void __not_in_flash_func( adc_timing_handler )( ) {
//     adc_timer_count++;
//     if ( adc_timer_count % 100 == 0 ) {
//         DEBUG_LA_PRINTF( "◆ ADC timer count: %d   %d\n", adc_timer_count, micros() );
//     }
// }


void la_counter_handler( ) {
  la_counter++;
  pio_interrupt_clear( la_pio, la_sm );
  // DEBUG_LA_PRINTF("◆ LA counter handler: %d\n", la_counter);
}

// DMA interrupt handler for ADC completion
void __not_in_flash_func( adcDmaHandler )( ) {
  // Clear DMA interrupt
  adc_timer_count++;
  if ( adc_dma_channel >= 0 ) {
      dma_hw->ints0 = 1u << adc_dma_channel;
      adc_dma_complete = true;

      // Embed AINSEL channel info in captured samples
      embedAinselInSamples( );

      // Note: Only print debug message for the final chunk completion
      // Individual chunk completions are handled silently for efficiency
  }
}



// Capture analog data using DMA
bool captureAnalogDataDMA( ) {

    // Initialize ADC if not already done
    adc_init( );
    // Set up GPIO pins for enabled channels (RP2350B: 8 channels starting at pin
    // 40)
    for ( int i = 0; i < 8; i++ ) {
        if ( analog_mask & ( 1 << i ) ) {
            uint32_t gpio_pin = 40 + i; // RP2350B ADC channels: pin 40-47 = ADC 0-7
            adc_gpio_init( gpio_pin );
            DEBUG_LA_PRINTF( "◆ ADC GPIO %lu (channel %d) initialized\n", gpio_pin, i );
        }
    }

    DEBUG_LA_PRINTF( "PIO counter: %d\n", la_counter );
    // Build round-robin mask from enabled channels (RP2350B supports all 8
    // channels)
    uint32_t round_robin_mask = analog_mask & 0xFF; // All 8 channels (0-7)
    if ( round_robin_mask == 0 ) {
        // DEBUG_LA_PRINTLN("◆ ERROR: No valid ADC channels in mask");
        // return false;

        // round_robin_mask = 0x1F;  // Default to channels 0-4 (5 channels)
        // analog_mask = round_robin_mask;
        // analog_chan_count = 5;
    }

    DEBUG_LA_PRINTF( "◆ Setting round-robin mask: 0x%02X (%d enabled channels)\n",
                     round_robin_mask, __builtin_popcount( round_robin_mask ) );

    // Debug: Show exact round-robin channel order
    DEBUG_LA_PRINTF( "◆ Round-robin order will be: " );
    for ( int i = 0; i < 8; i++ ) {
        if ( round_robin_mask & ( 1 << i ) ) {
            DEBUG_LA_PRINTF( "Ch%d ", i );
        }
    }
    DEBUG_LA_PRINTF( "\n" );

    // Configure ADC FIFO for DMA with improved buffering
    adc_fifo_setup(
        true,                      // Write each completed conversion to the sample FIFO
        true,                  // Enable DMA data request (DREQ)
        analog_chan_count, // DREQ asserted when at least 8 samples present (better buffering)
        false,             // Don't include ERR bit (we're using 16-bit reads)
        false              // Keep full 12-bit resolution (don't shift to 8-bit)
    );

    DEBUG_LA_PRINTF(
        "◆ Round-robin counter reset: disabled->enabled with mask 0x%02X\n",
        round_robin_mask );

    // Set ADC clock divider for desired sample rate
    // ADC runs at 48MHz, each conversion takes 96 cycles (2µs minimum)
    // CRITICAL: Round-robin is SEQUENTIAL, not parallel - each channel needs full
    // conversion time
    uint32_t actual_adc_channels = __builtin_popcount( analog_mask & 0xFF ); // Only count enabled ADC channels

    // FIXED CALCULATION: Account for sequential nature of round-robin
    float min_conversion_cycles = 96.0f; // Minimum cycles per conversion
    float total_cycles_per_round_robin = min_conversion_cycles * actual_adc_channels;
    float clk_div = ( 48000000.0f / sample_rate ) / total_cycles_per_round_robin - 1.0f;

    adc_set_clkdiv( clk_div );

    float actual_sample_rate = 48000000.0f / ( ( clk_div + 1.0f ) * total_cycles_per_round_robin );
    DEBUG_LA_PRINTF( "◆ ADC timing: clk_div=%.2f, %d channels, %.0f Hz actual "
                     "rate (requested %.0f Hz)\n",
                     clk_div, actual_adc_channels, actual_sample_rate,
                     (float)sample_rate );

    // Claim DMA channel
    adc_dma_channel = dma_claim_unused_channel( true );
    if ( adc_dma_channel < 0 ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Could not claim DMA channel for ADC" );
        return false;
    }

    DEBUG_LA_PRINTF( "◆ Claimed DMA channel %d for ADC\n", adc_dma_channel );

    // // // Set up DMA interrupt
    dma_channel_set_irq0_enabled( adc_dma_channel, true );
    irq_set_exclusive_handler( DMA_IRQ_0, adcDmaHandler );
    irq_set_enabled( DMA_IRQ_0, false );

    // dma_channel_set_irq0_enabled( adc_dma_channel, true );
    // irq_set_exclusive_handler( DREQ_ADC, adcDmaHandler );
    // irq_set_enabled( DREQ_ADC, false );

    // Calculate how many ADC samples we actually need to capture
    // Use physical sample count (not effective count) since analog doesn't get
    // oversampled
    uint32_t physical_samples = sample_count / calculateOversamplingFactor( );
    uint32_t actual_adc_samples =
        adc_oversampling_mode ? ( physical_samples + adc_duplication_factor - 1 ) /
                                    adc_duplication_factor
                              : physical_samples;

    // Channel count was set by allocateAllLogicAnalyzerBuffers()
    // Calculate total DMA buffer size needed
    uint32_t total_transfers = actual_adc_samples * analog_chan_count;
    size_t dma_buffer_size = total_transfers * sizeof( uint16_t );

    if ( adc_oversampling_mode ) {
        DEBUG_LA_PRINTF( "◆ DMA transfer calculation: %lu ADC samples × %d channels "
                         "= %lu transfers (%lu bytes)\n",
                         actual_adc_samples, analog_chan_count, total_transfers,
                         dma_buffer_size );
        DEBUG_LA_PRINTF( "◆ Will generate %lu output samples by duplicating each "
                         "ADC sample %lu times\n",
                         sample_count, adc_duplication_factor );
    } else {
        DEBUG_LA_PRINTF( "◆ DMA transfer calculation: %lu physical samples × %d "
                         "channels = %lu transfers (%lu bytes)\n",
                         actual_adc_samples, analog_chan_count, total_transfers,
                         dma_buffer_size );
    }

    // CRITICAL: Verify the pre-allocated buffer is large enough and valid
    if ( !analog_buffer ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Analog buffer is null - allocation failed" );
        return false;
    }

    size_t existing_size = malloc_usable_size( analog_buffer );

    // CRITICAL: Add safety margin to prevent buffer overflow
    size_t safety_margin = 1024; // 1KB safety margin
    if ( existing_size < ( dma_buffer_size + safety_margin ) ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Pre-allocated buffer too small: have %lu bytes, "
                         "need %lu bytes (with %lu safety margin)\n",
                         existing_size, dma_buffer_size, safety_margin );
        return false;
    }

    // CRITICAL: Clear buffer to prevent undefined behavior
    memset( analog_buffer, 0, dma_buffer_size );
    DEBUG_LA_PRINTF( "◆ Buffer cleared: %lu bytes zeroed\n", dma_buffer_size );

    DEBUG_LA_PRINTF(
        "◆ Using pre-allocated analog buffer: %lu bytes for %lu transfers\n",
        existing_size, total_transfers );

    // Use analog_buffer directly as DMA target (no separate DMA buffer!)
    adc_dma_buffer = analog_buffer;

    // Note: No separate AINSEL buffer needed - channel info will be embedded in
    // samples post-capture

    // Configure DMA timer for precise pacing instead of relying on ADC DREQ
    // Use DMA timer 0 for this purpose
    adc_timer_num = 0;
    dma_timer_claim( adc_timer_num );

    

    // Check if we need oversampling mode (sample rate > max ADC rate)
    if ( sample_rate > ADC_MAX_SAMPLE_RATE ) {
        adc_oversampling_mode = true;
        adc_actual_sample_rate = ADC_MAX_SAMPLE_RATE; // Cap ADC at maximum safe rate

        adc_duplication_factor = ( sample_rate + ADC_MAX_SAMPLE_RATE - 1 ) /
                                 ADC_MAX_SAMPLE_RATE; // Round up

        // Store persistent state for data transmission
        capture_used_oversampling = true;
        capture_duplication_factor = adc_duplication_factor;

        DEBUG_LA_PRINTF(
            "◆ OVERSAMPLING: Requested %lu Hz > %d Hz limit, using %lu Hz ADC\n",
            sample_rate, ADC_MAX_SAMPLE_RATE, adc_actual_sample_rate );
        DEBUG_LA_PRINTF(
            "◆ Each ADC sample will be duplicated %lu times to match timing\n",
            adc_duplication_factor );
    } else {
        adc_oversampling_mode = false;
        adc_actual_sample_rate = sample_rate; // Use requested rate directly
        adc_duplication_factor = 1;           // No duplication needed

        if ( adc_actual_sample_rate < 100000 ) {
            adc_actual_sample_rate = 100000;
        }

        // Store persistent state for data transmission
        capture_used_oversampling = false;
        capture_duplication_factor = 1;

        DEBUG_LA_PRINTF( "◆ NORMAL SAMPLING: Using %lu Hz ADC rate\n",
                         adc_actual_sample_rate );
    }

    // adc_select_input(8);
    // for (int i= 0; i < 32; i++) {
    //   temperature_reading+= adc_read();
    // }
    // adc_run(false);
    // temperature_reading= temperature_reading / 32;

    // DEBUG_LA_PRINTF("◆ Temperature reading: %d\n\r", temperature_reading);

    adc_fifo_drain( );
    delayMicroseconds( 100 );

    // Calculate DMA timer fraction for the actual ADC sample rate
    // The DMA timer runs at system_clock_freq * numerator / denominator
    // We want: timer_freq = adc_actual_sample_rate * analog_chan_count  (for
    // round-robin)
    uint32_t sys_clock = clock_get_hz( clk_sys );
    uint32_t target_freq = adc_actual_sample_rate * analog_chan_count; // Account for round-robin

    // Find best numerator/denominator pair within 16-bit limits
    uint16_t numerator = 1;
    uint16_t denominator = sys_clock / target_freq;

    //   // Adjust if denominator is too large or for better precision
    while ( denominator > 65535 ) {
        numerator++;
        denominator = ( sys_clock * numerator ) / target_freq;
        if ( numerator > 65535 ) {
            // Fallback: use maximum precision possible
            numerator = 65535;
            denominator = ( sys_clock * numerator ) / target_freq;
            if ( denominator > 65535 )
                denominator = 65535;
            break;
        }
    }

    // Set the DMA timer frequency
    dma_timer_set_fraction( adc_timer_num, numerator, denominator );

    float actual_freq = (float)sys_clock * numerator / denominator;
    DEBUG_LA_PRINTF(
        "◆ DMA timer pacing: timer %d, %d/%d, freq=%.0f Hz (target=%.0f Hz)\n",
        adc_timer_num, numerator, denominator, actual_freq, (float)target_freq );

    if ( adc_oversampling_mode ) {
        DEBUG_LA_PRINTF( "◆ OVERSAMPLING: ADC samples at %lu Hz, each sample sent "
                         "%lu times to match %lu Hz\n",
                         adc_actual_sample_rate, adc_duplication_factor,
                         sample_rate );
    }

    // // Clear ADC FIFO before starting DMA to ensure clean capture
    // DEBUG_LA_PRINTF("◆ Clearing ADC FIFO before DMA setup (level=%d)\n",
    // adc_fifo_get_level()); adc_fifo_drain(); DEBUG_LA_PRINTF("◆ ADC FIFO
    // cleared (level=%d)\n", adc_fifo_get_level());

    // Configure DMA transfer for single continuous capture
    dma_channel_config cfg = dma_channel_get_default_config( adc_dma_channel );

    channel_config_set_transfer_data_size( &cfg, DMA_SIZE_16 ); // 16-bit transfers

    channel_config_set_read_increment( &cfg, false ); // Read from ADC FIFO (fixed address)

    channel_config_set_write_increment( &cfg, true ); // Write to buffer (incrementing)

    channel_config_set_dreq( &cfg, dma_get_timer_dreq( adc_timer_num ) ); // Paced by DMA timer
    //channel_config_set_dreq( &cfg, DREQ_ADC ); // Paced by DMA timer

    //channel_con

    // channel_config_set_dreq(&cfg, DREQ_ADC); // Paced by DMA timer
    total_transfers += ( total_transfers % analog_chan_count );

    DEBUG_LA_PRINTF( "◆ Starting continuous DMA ADC capture: %lu samples, %lu "
                     "total transfers\n",
                     sample_count, total_transfers );

    // Configure single DMA transfer for entire capture
    dma_channel_configure( adc_dma_channel, &cfg,
                           adc_dma_buffer,  // Destination: our DMA buffer
                           &adc_hw->fifo,   // Source: ADC FIFO
                           total_transfers, // Total transfer count
                           false            // Don't start immediately
    );

    // uint32_t round_robin_mask = analog_mask & 0xFF;  // All 8 channels (0-7)
    // DEBUG_LA_PRINTF("◆ ADC CAPTURE: analog_mask=0x%08X,
    // round_robin_mask=0x%02X\n", analog_mask, round_robin_mask);

    // CRITICAL FIX: Ensure we have a valid round-robin mask
    if ( round_robin_mask == 0 ) {
        // DEBUG_LA_PRINTLN("◆ WARNING: Zero round-robin mask, using default
        // (channels 0-4)");
        round_robin_mask = 0x1F; // Default to channels 0-4 (5 channels)
    }

    // Reset completion flag and start continuous capture
    adc_dma_complete = false;
    uint32_t start_time = millis( );

    ///! above this was initDMA()

    if ( adc_dma_channel < 0 ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: DMA not initialized" );
        return false;
    }

    // Analog buffer should already be allocated by setupCapture()
    if ( !analog_buffer ) {
        DEBUG_LA_PRINTLN( "◆ ERROR: Analog buffer not pre-allocated" );
        return false;
    }

    // Step 3: Disable round-robin completely and wait
    // Step 3: Force round-robin counter reset by cycling through single channel
    // mode
    DEBUG_LA_PRINTF(
        "◆ Resetting round-robin counter via single channel selection\n" );

    // Find the first enabled channel (lowest bit set in mask)
    uint32_t first_channel = 0;
    for ( int i = 0; i < 8; i++ ) {
        if ( round_robin_mask & ( 1 << i ) ) {
            first_channel = i;
            break;
        }
    }
    adc_select_input( first_channel - 1 % analog_chan_count );
    // // Step 3a: Select single channel mode with the first channel
    // Step 4: Now enable round-robin - this should start from the first channel
    DEBUG_LA_PRINTF(
        "◆ Setting round-robin mask: 0x%02X (starting from channel %d)\n",
        round_robin_mask, first_channel );

    adc_fifo_drain( );

    DEBUG_LA_PRINTF( "◆ Round-robin mask set: 0x%02X (channels: ",
                     round_robin_mask );
    for ( int i = 0; i < 8; i++ ) {
        if ( round_robin_mask & ( 1 << i ) ) {
            DEBUG_LA_PRINTF( "%d ", i );
        }
    }
    //   DEBUG_LA_PRINTF(")\n");
    // delayMicroseconds(200);  // Give ADC time to configure round-robin properly

    //! Verify FIFO is still empty
    //  DEBUG_LA_PRINTF("◆ FIFO level after round-robin setup: %d\n",
    //  adc_fifo_get_level());

    // Step 6: Verify FIFO is empty before starting anything
    // DEBUG_LA_PRINTF("◆ FIFO level before start: %d\n", adc_fifo_get_level());
    Serial.flush( );
    delayMicroseconds( 100 );

    //   adc_run(true);

    //     //delayMicroseconds(1000);
    //    //
    // DEBUG_LA_PRINTF("◆ FIFO level before start: %d\n", adc_fifo_get_level());
    // int i = 0;
    // while (adc_get_selected_input() != first_channel) {
    //      tight_loop_contents();
    //     // i++;
    // }
    //  adc_run(false);

    // if (adc_fifo_get_level() > 0) {
    //     adc_fifo_drain();
    // }

    irq_set_enabled( DMA_IRQ_0, true );
    // adc_select_input(first_channel);
    // delayMicroseconds(100);

    noInterrupts( );
    adc_set_round_robin( round_robin_mask );

    // DEBUG_LA_PRINTF("◆ FIFO level before start: %d\n", adc_fifo_get_level());
    // delayMicroseconds(100);
    // adc_fifo_drain();

    // for (int i = 0; i < 10; i++) {
    //  adc_fifo_drain();
    //}
    adc_run( true );

    // while (adc_get_selected_input() != first_channel) {
    //     // if(adc_fifo_get_level() > 0) {
    //     //     adc_fifo_drain();
    //     // }
    //    // tight_loop_contents();
    // }

    // while (adc_get_selected_input() != ((first_channel - 1)% analog_chan_count)) {
    //   tight_loop_contents();
    // }

    pio_sm_set_enabled( la_pio, la_sm, true );
    adc_hw->fcs |= 0x00000001;
    // dma_channel_start(adc_dma_channel);
    // delayMicroseconds(100);
    // start both digital and analog DMA at the same time
    dma_start_channel_mask( ( 0x1 << adc_dma_channel ) | ( 0x1 << la_dma_chan ) );

    interrupts( );

    // delayMicroseconds(10);
    // dma_channel_start(la_dma_chan);

    // while (adc_get_selected_input() != first_channel) {
    //     tight_loop_contents();
    // }

    // DEBUG_LA_PRINTF("◆ Waited %d cycles for round-robin to settle\n", i);

    // delayMicroseconds(100);

    //

    // Note: Channel embedding will be done post-capture, no timer needed

    // adc_run(true);

    // DEBUG_LA_PRINTF("◆ Starting DMA\n");

    // DEBUG_LA_PRINTF("◆ Starting ADC for capture\n");

    // Step 9: Let a few samples settle to ensure round-robin is properly
    // synchronized
    // DEBUG_LA_PRINTF("◆ Allowing initial samples to settle\n");
    // delayMicroseconds(500);  // Let 5-10 samples pass to ensure proper
    // synchronization

    // DEBUG_LA_PRINTF("◆ Enhanced ADC/DMA start: ADC stopped, FIFO verified
    // empty, round-robin reset, DMA ready, ADC started\n");

    // USBSer2.flush();
    // Wait for entire capture to complete
    uint32_t timeout_ms = ( sample_count * 1000 ) / adc_actual_sample_rate +
                          2000; // Sample time + 2s margin

    while ( !adc_dma_complete && ( millis( ) - start_time ) < timeout_ms ) {
        tight_loop_contents( );
        // if (la_counter % 100 == 0) {
        //   DEBUG_LA_PRINTF("◆ LA counter: %d\n", la_counter);
        // }

        // // Debug: Check PIO state machine status every 1000ms
        // static uint32_t last_pio_check = 0;
        // if (millis() - last_pio_check > 1000) {
        //   last_pio_check = millis();
        //   DEBUG_LA_PRINTF("◆ PIO debug check: counter=%d, time=%lu ms\n",
        //                   la_counter, millis() - start_time);
        // }

        // // Check for manual cancellation
        // if (USBSer2.available() > 0) {
        //     DEBUG_LA_PRINTLN("◆ DMA capture cancelled by driver");
        //     adc_run(false);
        //     return false;
        // }

        // // Periodic progress (without stopping capture)
        // if ((millis() - start_time) % 1000 == 0) {
        //     uint32_t elapsed_ms = millis() - start_time;
        //     uint32_t expected_samples = (elapsed_ms * sample_rate) / 1000;
        //     if (expected_samples > 0 && expected_samples % 1000 == 0) {
        //         DEBUG_LA_PRINTF("◆ DMA progress: ~%lu samples captured (%lu
        //         ms)\n",
        //                      min(expected_samples, sample_count), elapsed_ms);
        //     }
        // }
    }

    // Stop ADC and check completion
    // adc_fifo_drain();

    adc_set_round_robin( 0 );
    adc_run( false );
    adc_select_input( first_channel );
    delayMicroseconds( 100 );
    for ( int i = 0; i < 10; i++ ) {
        adc_fifo_drain( );
    }
    // adc_fifo_drain();

    if ( !adc_dma_complete ) {
        DEBUG_LA_PRINTF( "◆ ERROR: DMA capture timeout after %lu ms\n",
                         millis( ) - start_time );
        return false;
    }

    // DETAILED BUFFER ANALYSIS: Examine raw DMA buffer immediately after capture
    DEBUG_LA_PRINTF( "◆ RAW DMA BUFFER ANALYSIS (first 80 values = 16 samples × "
                     "%d channels):\n",
                     analog_chan_count );
    uint16_t* raw_buffer = (uint16_t*)adc_dma_buffer;

    // First, verify the expected channel sequence
    DEBUG_LA_PRINTF( "◆ Expected channel sequence: " );
    for ( int i = 0; i < 8; i++ ) {
        if ( round_robin_mask & ( 1 << i ) ) {
            DEBUG_LA_PRINTF( "CH%d ", i );
        }
    }
    DEBUG_LA_PRINTF( "\n" );

    // Show raw buffer data to analyze round-robin alignment
    DEBUG_LA_PRINTF( "◆ RAW DMA BUFFER ANALYSIS (first 80 values = 16 samples × 5 "
                     "channels):\n" );
    for ( int i = 0; i < 180; i += analog_chan_count ) {
        int sample_num = i / analog_chan_count;
        DEBUG_LA_PRINTF( "  Raw[%2d]:", sample_num );
        for ( int j = 0; j < analog_chan_count; j++ ) {
            DEBUG_LA_PRINTF( ", [%4u]", raw_buffer[ i + j ] & 0x0FFF );
        }
        DEBUG_LA_PRINTF( "\n" );
    }

    // CRITICAL: Detect round-robin channel alignment and calculate offset
    int channel_offset = 0; // How many channels the round-robin is shifted


    DEBUG_LA_PRINTF( "◆ ADC timer count: %d   %d\n", adc_timer_count, micros() );
    // ADC HARDWARE STATE DEBUGGING
    DEBUG_LA_PRINTF( "◆ ADC HARDWARE STATE after capture:\n" );
    DEBUG_LA_PRINTF( "  FIFO level: %u\n", adc_fifo_get_level( ) );
    DEBUG_LA_PRINTF( "  ADC enabled: %s\n",
                     ( adc_hw->cs & ADC_CS_EN_BITS ) ? "YES" : "NO" );
    DEBUG_LA_PRINTF( "  Round-robin mask: 0x%02X\n",
                     ( adc_hw->cs & ADC_CS_RROBIN_BITS ) >> ADC_CS_RROBIN_LSB );
    DEBUG_LA_PRINTF( "  Current ADC input: %u\n",
                     ( adc_hw->cs & ADC_CS_AINSEL_BITS ) >> ADC_CS_AINSEL_LSB );

    // DMA TIMER STATE DEBUGGING
    if ( adc_timer_num >= 0 ) {
        // Note: Can't easily read DMA timer state, but we can verify it was
        // configured
        DEBUG_LA_PRINTF(
            "  DMA timer %d: configured for %lu Hz target (%d channels × %lu Hz)\n",
            adc_timer_num, target_freq, analog_chan_count, adc_actual_sample_rate );
        DEBUG_LA_PRINTF( "  DMA transfers completed: %lu (expected %lu)\n",
                         total_transfers, total_transfers );

        // CRITICAL TIMING ANALYSIS
        float dma_period_us = 1000000.0f / target_freq; // Time between DMA requests
        float adc_conversion_time_us =
            96.0f / 48.0f; // 96 cycles at 48MHz = 2µs per conversion
        float full_round_robin_time_us = adc_conversion_time_us * analog_chan_count;

        DEBUG_LA_PRINTF( "  TIMING ANALYSIS:\n" );
        DEBUG_LA_PRINTF( "    DMA requests sample every: %.2f µs\n", dma_period_us );
        DEBUG_LA_PRINTF( "    ADC conversion time: %.2f µs per channel\n",
                         adc_conversion_time_us );
        DEBUG_LA_PRINTF( "    Full round-robin cycle: %.2f µs (%d channels)\n",
                         full_round_robin_time_us, analog_chan_count );

        if ( dma_period_us < full_round_robin_time_us ) {
            DEBUG_LA_PRINTF( "    *** TIMING ERROR: DMA too fast! Requesting samples "
                             "%.2f µs faster than ADC can provide ***\n",
                             full_round_robin_time_us - dma_period_us );
        } else {
            DEBUG_LA_PRINTF(
                "    Timing OK: DMA period %.2f µs >= round-robin time %.2f µs\n",
                dma_period_us, full_round_robin_time_us );
        }
    }

    // Drain any remaining FIFO data for analysis
    int remaining_fifo_samples = 0;
    DEBUG_LA_PRINTF( "◆ REMAINING FIFO DATA: " );
    while ( adc_fifo_get_level( ) > 0 && remaining_fifo_samples < 10 ) {
        uint16_t fifo_value = adc_fifo_get_blocking( );
        DEBUG_LA_PRINTF( "[%u] ", fifo_value );
        remaining_fifo_samples++;
    }
    if ( remaining_fifo_samples == 0 ) {
        DEBUG_LA_PRINTF( "(empty)" );
    }
    DEBUG_LA_PRINTF( "\n" );

    DEBUG_LA_PRINTF(
        "◆ DMA analog capture complete: %lu samples × %d channels in %lu ms\n",
        sample_count, analog_chan_count, millis( ) - start_time );

    return true;
}

// Clean up DMA resources
void cleanupAdcDma( ) {
    if ( adc_dma_channel >= 0 ) {
        // Stop DMA and disable interrupt
        dma_channel_abort( adc_dma_channel );
        dma_channel_set_irq0_enabled( adc_dma_channel, false );
        dma_channel_unclaim( adc_dma_channel );
        adc_dma_channel = -1;

        DEBUG_LA_PRINTLN( "◆ DMA ADC resources cleaned up" );
    }

    // Clean up DMA timer
    if ( adc_timer_num >= 0 ) {
        dma_timer_unclaim( adc_timer_num );
        adc_timer_num = -1;
        DEBUG_LA_PRINTLN( "◆ DMA timer unclaimed" );
    }

    // Reset temporary oversampling state (persistent state survives for data
    // transmission)
    adc_oversampling_mode = false;
    adc_actual_sample_rate = 0;
    adc_duplication_factor = 1;

    if ( capture_used_oversampling ) {
        DEBUG_LA_PRINTF( "◆ Preserving oversampling state for data transmission "
                         "(dup_factor=%lu)\n",
                         capture_duplication_factor );
    }

    // Note: No AINSEL timer or buffer to clean up - channel info is embedded in
    // samples

    // Reset DMA buffer pointer (it points to analog_buffer, don't free it here)
    adc_dma_buffer = nullptr;

    // Stop ADC
    adc_run( false );
    adc_fifo_drain( );
}

// =============================================================================
// UNIFIED BUFFER MANAGEMENT SYSTEM
// =============================================================================
// This replaces multiple overlapping buffer calculation and allocation
// functions with a single, comprehensive system that ensures consistency and
// prevents leaks.

// Forward declarations
void releaseAllLogicAnalyzerBuffers( );
bool allocateAllLogicAnalyzerBuffers( );

struct LogicAnalyzerBuffers {
    // Digital capture buffer
    uint32_t* digital_buffer;
    size_t digital_buffer_size;
    uint32_t digital_max_samples;

    // Analog capture buffer
    uint16_t* analog_buffer;
    size_t analog_buffer_size;
    uint32_t analog_max_samples;
    uint32_t analog_channels_active;

    // DMA resources
    int dma_channel_digital;
    int dma_channel_analog;

    // PIO resources
    PIO pio_instance;
    uint pio_state_machine;
    uint pio_program_offset;

    // Buffer allocation tracking
    bool digital_allocated;
    bool analog_allocated;
    bool hardware_allocated;

    // Memory usage tracking
    size_t total_allocated_bytes;
    size_t available_memory_at_allocation;
};

// Global unified buffer manager
static LogicAnalyzerBuffers g_la_buffers = { };

// Single source of truth for all buffer calculations

UnifiedBufferRequirements calculateUnifiedBufferRequirements( ) {
    UnifiedBufferRequirements req = { };

    // Get available memory with more conservative safety margin
    size_t free_heap = rp2040.getFreeHeap( );
    req.safety_reserve = JL_LA_RESERVE_RAM;
    if ( free_heap > req.safety_reserve ) {
        req.usable_memory = free_heap - req.safety_reserve;
    } else {
        req.usable_memory = 1024;
    }

    // Calculate bytes per sample for storage
    req.driver_analog_channels = __builtin_popcount( analog_mask & 0xFF );
    req.digital_storage_per_sample = 1;
    req.analog_storage_per_sample = ( current_la_mode == LA_MODE_DIGITAL_ONLY )
                                        ? 0
                                        : req.driver_analog_channels * 2;
    size_t total_storage_per_sample =
        req.digital_storage_per_sample + req.analog_storage_per_sample;
    if ( total_storage_per_sample == 0 )
        total_storage_per_sample = 1;

    // Calculate max samples based on memory - be more conservative
    req.max_samples_memory_limit = req.usable_memory / total_storage_per_sample;

    // Apply system limits
    if ( req.max_samples_memory_limit > JL_LA_MAX_SAMPLES_LIMIT ) {
        req.max_samples_memory_limit = JL_LA_MAX_SAMPLES_LIMIT;
    }
    if ( req.max_samples_memory_limit < JL_LA_MIN_SAMPLES ) {
        req.max_samples_memory_limit = JL_LA_MIN_SAMPLES;
    }

    req.max_samples_final = req.max_samples_memory_limit;

    // Calculate actual buffer sizes needed
    req.digital_buffer_bytes =
        req.max_samples_final * req.digital_storage_per_sample;
    req.analog_buffer_bytes =
        req.max_samples_final * req.analog_storage_per_sample;
    req.total_buffer_bytes = req.digital_buffer_bytes + req.analog_buffer_bytes;

    DEBUG_LA_PRINTF( "◆ UNIFIED BUFFER CALC:\n" );
    DEBUG_LA_PRINTF( "  Driver config: 8 digital, %lu analog channels\n",
                     req.driver_analog_channels );
    DEBUG_LA_PRINTF( "  Memory: %lu free, %lu usable, %lu safety reserve\n",
                     free_heap, req.usable_memory, req.safety_reserve );
    DEBUG_LA_PRINTF( "  Storage: %lu bytes/sample (%lu digital + %lu analog)\n",
                     total_storage_per_sample, req.digital_storage_per_sample,
                     req.analog_storage_per_sample );
    DEBUG_LA_PRINTF( "  Max samples: %lu (memory limit: %lu)\n",
                     req.max_samples_final, req.max_samples_memory_limit );
    DEBUG_LA_PRINTF( "  Buffers: %lu digital + %lu analog = %lu total bytes\n",
                     req.digital_buffer_bytes, req.analog_buffer_bytes,
                     req.total_buffer_bytes );

    // Show oversampling information
    uint32_t oversampling_factor = calculateOversamplingFactor( );
    uint32_t effective_samples =
        calculateEffectiveSampleCount( req.max_samples_final );
    DEBUG_LA_PRINTF(
        "  Oversampling: %lux factor → %lu effective samples for driver\n",
        oversampling_factor, effective_samples );

    return req;
}

bool allocateAllLogicAnalyzerBuffers( ) {
    // Calculate unified requirements
    UnifiedBufferRequirements req = calculateUnifiedBufferRequirements( );

    // Check if we can satisfy the memory requirement
    if ( req.total_buffer_bytes > req.usable_memory ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Cannot allocate %zu bytes (only %zu available)\n",
                         req.total_buffer_bytes, req.usable_memory );
        return false;
    }

    // Free any existing allocations first
    releaseAllLogicAnalyzerBuffers( ); // Forward reference - defined below

    // Allocate digital buffer (always needed)
    // Allocate digital buffer with extra space for safety
    size_t extra_digital_space = 1024; // 1KB extra space for safety
    size_t total_digital_bytes = req.digital_buffer_bytes + extra_digital_space;

    g_la_buffers.digital_buffer = (uint32_t*)malloc( total_digital_bytes );
    if ( !g_la_buffers.digital_buffer ) {
        DEBUG_LA_PRINTF( "◆ ERROR: Failed to allocate %zu byte digital buffer (requested %zu + %zu extra)\n",
                         total_digital_bytes, req.digital_buffer_bytes, extra_digital_space );
        return false;
    }
    g_la_buffers.digital_buffer_size = total_digital_bytes;
    g_la_buffers.digital_max_samples = req.max_samples_final;
    g_la_buffers.digital_allocated = true;

    // Clear digital buffer
    memset( g_la_buffers.digital_buffer, 0, total_digital_bytes );

    DEBUG_LA_PRINTF( "◆ UNIFIED: Digital buffer allocated: %zu bytes (%zu + %zu extra)\n",
                     total_digital_bytes, req.digital_buffer_bytes, extra_digital_space );

    // Allocate analog buffer if needed (with extra space for DMA safety)
    if ( req.analog_buffer_bytes > 0 ) {
        // Add extra buffer space to prevent DMA overflow
        size_t extra_buffer_space = 2048; // 2KB extra space for safety
        size_t total_analog_bytes = req.analog_buffer_bytes + extra_buffer_space;

        g_la_buffers.analog_buffer = (uint16_t*)malloc( total_analog_bytes );
        if ( !g_la_buffers.analog_buffer ) {
            DEBUG_LA_PRINTF( "◆ ERROR: Failed to allocate %zu byte analog buffer (requested %zu + %zu extra)\n",
                             total_analog_bytes, req.analog_buffer_bytes, extra_buffer_space );
            releaseAllLogicAnalyzerBuffers( ); // Clean up digital buffer
            return false;
        }
        g_la_buffers.analog_buffer_size = total_analog_bytes;
        g_la_buffers.analog_max_samples = req.max_samples_final;
        g_la_buffers.analog_channels_active = req.driver_analog_channels;
        g_la_buffers.analog_allocated = true;

        // Clear analog buffer
        memset( g_la_buffers.analog_buffer, 0, total_analog_bytes );

        DEBUG_LA_PRINTF( "◆ UNIFIED: Analog buffer allocated: %zu bytes (%zu + %zu extra)\n",
                         total_analog_bytes, req.analog_buffer_bytes, extra_buffer_space );
    }

    // Track total allocation
    g_la_buffers.total_allocated_bytes = req.total_buffer_bytes;
    g_la_buffers.available_memory_at_allocation = req.available_memory;

    // Update global state to match unified calculation
    jl_la_max_samples = req.max_samples_final;
    jl_la_buffer_size = req.digital_buffer_bytes;

    // Update legacy pointers for compatibility
    la_buffer = g_la_buffers.digital_buffer;
    analog_buffer = g_la_buffers.analog_buffer;
    analog_buffer_size = g_la_buffers.analog_buffer_size;

    DEBUG_LA_PRINTF( "◆ UNIFIED ALLOCATION SUCCESS: %zu bytes total (%zu free "
                     "memory remaining)\n",
                     req.total_buffer_bytes, rp2040.getFreeHeap( ) );

    return true;
}

void releaseAllLogicAnalyzerBuffers( ) {
    // This function will handle freeing all allocated buffers
    // It should be the single point of truth for de-allocation
}

void releaseLogicAnalyzerResources( ) {
    DEBUG_LA_PRINTLN( "◆ Releasing logic analyzer resources..." );

    // CRITICAL: Clean up interrupt handlers based on which PIO was used
    if ( la_pio == pio0 ) {
        irq_remove_handler( PIO0_IRQ_0, la_counter_handler );
        irq_set_enabled( PIO0_IRQ_0, false );
        DEBUG_LA_PRINTLN( "◆ PIO0 interrupt handler removed" );
    } else if ( la_pio == pio1 ) {
        irq_remove_handler( PIO1_IRQ_0, la_counter_handler );
        irq_set_enabled( PIO1_IRQ_0, false );
        DEBUG_LA_PRINTLN( "◆ PIO1 interrupt handler removed" );
    } else if ( la_pio == pio2 ) {
        irq_remove_handler( PIO2_IRQ_0, la_counter_handler );
        irq_set_enabled( PIO2_IRQ_0, false );
        DEBUG_LA_PRINTLN( "◆ PIO2 interrupt handler removed" );
    }

    // CRITICAL: Clean up PIO resources first
    if ( la_pio && la_prog_offset != (uint)-1 ) {
        pio_remove_program( la_pio, &la_pio_program_fast, la_prog_offset );
        la_prog_offset = -1;
        DEBUG_LA_PRINTF( "◆ PIO program removed from PIO%d\n", pio_get_index( la_pio ) );
    }
    if ( la_pio && la_sm != (uint)-1 ) {
        pio_sm_set_enabled( la_pio, la_sm, false ); // Disable before unclaiming
        pio_sm_unclaim( la_pio, la_sm );
        la_sm = -1;
        DEBUG_LA_PRINTF( "◆ PIO state machine unclaimed from PIO%d\n", pio_get_index( la_pio ) );
    }

    // CRITICAL: Clean up DMA resources
    if ( la_dma_chan != -1 ) {
        dma_channel_abort( la_dma_chan ); // Stop any active transfers
        dma_channel_unclaim( la_dma_chan );
        la_dma_chan = -1;
        DEBUG_LA_PRINTLN( "◆ Digital DMA channel unclaimed" );
    }

    // Clean up memory buffers (handled by atomic system, but kept for safety)
    if ( la_buffer ) {
        free( la_buffer );
        la_buffer = nullptr;
    }

    // Free analog buffer if it was separately allocated
    if ( analog_buffer ) {
        free( analog_buffer );
        analog_buffer = nullptr;
        analog_buffer_size = 0;
    }

    //   // Note: No separate AINSEL buffer to free - channel info embedded in samples

    DEBUG_LA_PRINTLN( "◆ All logic analyzer resources released" );
}

// Calculate oversampling factor based on current sample rate
uint32_t calculateOversamplingFactor( ) {
    DEBUG_LA_PRINTF( "◆ OVERSAMPLING CALC: sample_rate = %lu Hz\n", sample_rate );

    if ( sample_rate <= 100000 ) {
        DEBUG_LA_PRINTF( "◆ OVERSAMPLING: Rate %lu <= 100000 Hz. Factor: 1\n",
                         sample_rate );
        return 1; // No oversampling below 100kHz
    }

    // Calculate oversampling factor: higher sample rates get more oversampling
    uint32_t factor = 1;
    if ( sample_rate >= 1000000 ) {
        factor = 10; // 1MHz+ gets 10x oversampling
        DEBUG_LA_PRINTF( "◆ OVERSAMPLING: Rate %lu >= 1000000 Hz. Factor: %lu\n",
                         sample_rate, factor );
    } else if ( sample_rate >= 500000 ) {
        factor = 5; // 500kHz+ gets 5x oversampling
        DEBUG_LA_PRINTF( "◆ OVERSAMPLING: Rate %lu >= 500000 Hz. Factor: %lu\n",
                         sample_rate, factor );
    } else if ( sample_rate >= 200000 ) {
        factor = 2; // 200kHz+ gets 2x oversampling
        DEBUG_LA_PRINTF( "◆ OVERSAMPLING: Rate %lu >= 200000 Hz. Factor: %lu\n",
                         sample_rate, factor );
    }

    return factor;
}

// Calculate effective sample count for driver (accounts for oversampling)
uint32_t calculateEffectiveSampleCount( uint32_t physical_samples ) {
    uint32_t oversampling_factor = calculateOversamplingFactor( );
    uint32_t effective_samples = physical_samples * oversampling_factor;

    DEBUG_LA_PRINTF(
        "◆ EFFECTIVE SAMPLES: %lu physical × %lu factor = %lu effective\n",
        physical_samples, oversampling_factor, effective_samples );
    return effective_samples;
}
