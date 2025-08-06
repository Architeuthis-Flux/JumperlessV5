#include "JulseView.h"
#include "ArduinoStuff.h" // For USBSer2
#include "Graphics.h"
#include "Peripherals.h"

#include "config.h"

// Logic analyzer debug flag (kept for compatibility)
bool debugLA = false;

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/structs/bus_ctrl.h"
#include "hardware/timer.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define JULSEDEBUG 1

bool initialized = false;

int debugColors[ 10 ] = {
    108, // commands
    156, // buffers
    111, // digital
    224, // analog
    105, // dmas
    226, // usb
    76,  // timing
    195, // data
    196, // errors
    213, // state
};

// Debug categories - use bit flags for selective enabling
#define JULSEDEBUG_COMMANDS ( 1 << 0 ) // Command processing (i, R, L, a, A, D, etc.)
#define JULSEDEBUG_BUFFERS ( 1 << 1 )  // Buffer allocation, DMA setup, memory management
#define JULSEDEBUG_DIGITAL ( 1 << 2 )  // Digital capture, PIO, GPIO operations
#define JULSEDEBUG_ANALOG ( 1 << 3 )   // ADC setup, analog capture, voltage readings
#define JULSEDEBUG_DMAS ( 1 << 4 )     // DMA configuration, transfers, IRQs
#define JULSEDEBUG_USBS ( 1 << 5 )     // USB transmission, flow control, data streaming
#define JULSEDEBUG_TIMING ( 1 << 6 )   // Sample rates, clock dividers, timing issues
#define JULSEDEBUG_DATA ( 1 << 7 )     // Raw data values, sample processing
#define JULSEDEBUG_ERRORS ( 1 << 8 )   // Error conditions, warnings, failures
#define JULSEDEBUG_STATE ( 1 << 9 )    // State machine transitions, mode changes

// Debug level enum (for backward compatibility)
typedef enum {
    JULSEDEBUG_LEVEL_NONE = 0,
    JULSEDEBUG_LEVEL_ERROR = 1,
    JULSEDEBUG_LEVEL_WARNING = 2,
    JULSEDEBUG_LEVEL_INFO = 3,
    JULSEDEBUG_LEVEL_DEBUG = 4
} julseview_debug_level_t;

// Debug configuration
julseview_debug_level_t julseview_debug_level = JULSEDEBUG_LEVEL_DEBUG;

uint32_t julseview_debug_mask =
    JULSEDEBUG_ERRORS |
    JULSEDEBUG_COMMANDS |
   JULSEDEBUG_USBS |
    JULSEDEBUG_DMAS |
    JULSEDEBUG_DATA |
    JULSEDEBUG_ANALOG |
    JULSEDEBUG_DIGITAL |
    JULSEDEBUG_TIMING |
    JULSEDEBUG_STATE |
    JULSEDEBUG_BUFFERS

    ; // Default: errors and commands only

void julseview_debug_color( int color ) {
    Serial.printf( "\033[38;5;%dm", color );
    Serial.flush( );
}

// Helper macros for category-specific debug output
#define JULSEDEBUG_CHECK( category ) ( julseview_debug_level >= JULSEDEBUG_LEVEL_INFO && ( julseview_debug_mask & category ) )

// Convenience macros for common debug categories
#define JULSEDEBUG_CMD( fmt, ... )             \
    julseview_debug_color( debugColors[ 0 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_COMMANDS, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_BUF( fmt, ... )             \
    julseview_debug_color( debugColors[ 1 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_BUFFERS, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_DIG( fmt, ... )             \
    julseview_debug_color( debugColors[ 2 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_DIGITAL, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_ANA( fmt, ... )             \
    julseview_debug_color( debugColors[ 3 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_ANALOG, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_DMA( fmt, ... )             \
    julseview_debug_color( debugColors[ 4 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_DMAS, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_USB( fmt, ... )             \
    julseview_debug_color( debugColors[ 5 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_USBS, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_TIM( fmt, ... )             \
    julseview_debug_color( debugColors[ 6 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_TIMING, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_DAT( fmt, ... )             \
    julseview_debug_color( debugColors[ 7 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_DATA, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_ERR( fmt, ... )             \
    julseview_debug_color( debugColors[ 8 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_ERRORS, fmt, ##__VA_ARGS__ );
#define JULSEDEBUG_STA( fmt, ... )             \
    julseview_debug_color( debugColors[ 9 ] ); \
    JULSEDEBUG_PRINTF_CAT( JULSEDEBUG_STATE, fmt, ##__VA_ARGS__ );

// Category-specific debug macros
#define JULSEDEBUG_PRINTF_CAT( category, fmt, ... ) \
    if ( JULSEDEBUG_CHECK( category ) )             \
    Serial.printf( fmt, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINT_CAT( category, str, ... ) \
    if ( JULSEDEBUG_CHECK( category ) )            \
    Serial.print( str, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINTLN_CAT( category, str, ... ) \
    if ( JULSEDEBUG_CHECK( category ) )              \
    Serial.println( str, ##__VA_ARGS__ )

// Legacy macros (for backward compatibility) - always enabled if debug level >= 1
#define JULSEDEBUG_PRINTF( fmt, ... ) \
    if ( julseview_debug_level >= 1 ) \
    Serial.printf( fmt, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINT( str, ... )  \
    if ( julseview_debug_level >= 1 ) \
    Serial.print( str, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINTLN( str, ... ) \
    if ( julseview_debug_level >= 1 )  \
    Serial.println( str, ##__VA_ARGS__ )

// Level-specific macros (for backward compatibility)
#define JULSEDEBUG_PRINTF2( fmt, ... ) \
    if ( julseview_debug_level >= 2 )  \
    Serial.printf( fmt, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINT2( str, ... ) \
    if ( julseview_debug_level >= 2 ) \
    Serial.print( str, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINTLN2( str, ... ) \
    if ( julseview_debug_level >= 2 )   \
    Serial.println( str, ##__VA_ARGS__ )

#define JULSEDEBUG_PRINTF3( fmt, ... ) \
    if ( julseview_debug_level == 3 )  \
    Serial.printf( fmt, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINT3( str, ... ) \
    if ( julseview_debug_level == 3 ) \
    Serial.print( str, ##__VA_ARGS__ )
#define JULSEDEBUG_PRINTLN3( str, ... ) \
    if ( julseview_debug_level == 3 )   \
    Serial.println( str, ##__VA_ARGS__ )

// Debug configuration helper function
void julseview_set_debug_mask( uint32_t mask ) {
    julseview_debug_mask = mask;
    JULSEDEBUG_PRINTF( "Debug mask set to 0x%08X\n", mask );
}

// Debug configuration helper function with string input
void julseview_set_debug_categories( const char* categories ) {
    uint32_t mask = 0;
    if ( strstr( categories, "commands" ) )
        mask |= JULSEDEBUG_COMMANDS;
    if ( strstr( categories, "buffers" ) )
        mask |= JULSEDEBUG_BUFFERS;
    if ( strstr( categories, "digital" ) )
        mask |= JULSEDEBUG_DIGITAL;
    if ( strstr( categories, "analog" ) )
        mask |= JULSEDEBUG_ANALOG;
    if ( strstr( categories, "dma" ) )
        mask |= JULSEDEBUG_DMAS;
    if ( strstr( categories, "usb" ) )
        mask |= JULSEDEBUG_USBS;
    if ( strstr( categories, "timing" ) )
        mask |= JULSEDEBUG_TIMING;
    if ( strstr( categories, "data" ) )
        mask |= JULSEDEBUG_DATA;
    if ( strstr( categories, "errors" ) )
        mask |= JULSEDEBUG_ERRORS;
    if ( strstr( categories, "state" ) )
        mask |= JULSEDEBUG_STATE;
    if ( strstr( categories, "all" ) )
        mask = 0x3FF; // All categories
    if ( strstr( categories, "none" ) )
        mask = 0; // No categories

    julseview_set_debug_mask( mask );
}

volatile bool julseview_active = false;

// --- PIO Program for fast capture with interrupt pacing ---
// Use pio_encode_in to generate the correct instruction for 8 pins
static const uint16_t pio_program_instructions[] = {
    (uint16_t)pio_encode_in( pio_pins, 8 ), // in pins, 8 - read 8 pins into input shift register
};
static const struct pio_program pio_logic_analyzer_program = {
    .instructions = pio_program_instructions,
    .length = 1,
    .origin = -1,
};

// --- ADC Channel Monitor Timer ---
// Use hardware timer to monitor ADC AINSEL register instead of PIO
// (PIO can't directly access memory-mapped registers like ADC_CS)

// --- Globals for DMA and PIO ---
dma_channel_config acfg0, acfg1, pcfg0, pcfg1;
uint admachan0, admachan1, pdmachan0, pdmachan1, coord_dma_chan;
uint32_t coordinator_buffer[ 2 ]; // Tiny buffer for coordinator DMA
PIO pio = pio1;                   // Use PIO1 to avoid conflicts with CH446Q (which uses PIO0)
uint piosm = 0;

uint dreq = 0;
volatile uint32_t *tstsa0, *tstsa1, *tstsd0, *tstsd1;
volatile uint32_t *taddra0, *taddra1, *taddrd0, *taddrd1;
volatile int lowerhalf;

// --- ADC Monitor Timer Globals ---
volatile bool adc_first_channel_ready = false; // Set by timer IRQ when ADC reaches first enabled channel
volatile uint32_t adc_sync_count = 0;          // Debug counter
volatile bool adc_monitor_active = false;      // Enable/disable monitoring
volatile uint8_t adc_first_channel = 0;        // First enabled channel in round-robin
// DMA Coordinator state
volatile bool coordinator_switch_ready = false;
volatile bool buffer0_active = true; // Track which buffer is currently capturing

// Producer-Consumer ping-pong state
volatile bool buffer0_capturing = true;     // Buffer 0 is capturing data
volatile bool buffer1_capturing = false;    // Buffer 1 is capturing data
volatile bool buffer0_ready = false;        // Buffer 0 is ready for transmission
volatile bool buffer1_ready = false;        // Buffer 1 is ready for transmission
volatile bool buffer0_transmitting = false; // Buffer 0 is being transmitted
volatile bool buffer1_transmitting = false; // Buffer 1 is being transmitted
uint32_t buffer_max_samples;                // Maximum samples per buffer before switch
uint32_t fixed_switch_samples;              // Fixed sample boundary for switching (prevents stale data)
uint32_t current_buffer_samples;            // Samples captured in current buffer
uint32_t buffer0_configured_samples;        // Actual samples configured for buffer 0 DMA
uint32_t buffer1_configured_samples;        // Actual samples configured for buffer 1 DMA
volatile bool mask_xfer_err;
uint32_t num_halves;
uint8_t d_dma_bps;
uint32_t rxbufdidx;
uint32_t samp_remain;

uint32_t total_bytes_sent; // Track total bytes sent for completion signal

// --- Helper Functions ---

// Check USB buffer status to prevent overflow
bool check_usb_buffer_status( ) {
    // Check if USB serial is available for writing
    if ( !USBSer2 ) {
        return false;
    }

    //  Add timeout protection to prevent infinite blocking
    static uint32_t last_check_time = 0;
    uint32_t current_time = millis( );

    // Prevent excessive checking that could cause timing issues
    if ( current_time - last_check_time < 1 ) {
        return true; // Assume available if checking too frequently
    }
    last_check_time = current_time;

    return true;
}

// Find the first (lowest) enabled channel in the ADC mask
uint8_t get_first_adc_channel( uint8_t adc_mask ) {
    for ( uint8_t i = 0; i < 8; i++ ) {
        if ( adc_mask & ( 1 << i ) ) {
            return i;
        }
    }
    return 0; // Fallback to channel 0 if no channels enabled
}

bool julseview::julseview_usb_out_chars( const char* buf, int length ) {
    // CRITICAL SAFETY: Add null pointer and bounds checking
    if ( !buf || length <= 0 || length > 1024 ) {
        JULSEDEBUG_ERR( "ERROR: Invalid USB transmission parameters - buf=%p, length=%d\n\r", buf, length );
        return false;
    }

    // Check USB buffer status before transmission
    if ( !check_usb_buffer_status( ) ) {
        JULSEDEBUG_USB( "USB buffer not available for transmission\n\r" );
        return false;
    }

    // Simple, fast transmission for optimized performance
    USBSer2.write( (const uint8_t*)buf, length );
    USBSer2.flush( );
    return true;
}

// --- Class Methods ---
bool julseview::init( ) {
    reset( );

    // Initialize with default values (will be configured properly in arm())
    a_mask = 0b00011111;
    d_mask = 0b11111111;
    c_mask = 0; // Control channel mask
    sample_rate = 10000;
    num_samples = 5000;
    a_chan_cnt = 5; // Will be calculated in arm() based on a_mask
    d_chan_cnt = 8; // Will be calculated in arm() based on d_mask
    c_chan_cnt = 0; // Control channel count
    d_nps = 0;      // Will be calculated in arm()
    cmdstrptr = 0;
    use_rle = false;

    // Reuse existing buffer if already allocated (don't free and reallocate)
    if ( raw_capture_buf && capture_buf && actual_buffer_size > 0 ) {
        JULSEDEBUG_BUF( "Reusing existing capture buffer (%d bytes)\n\r", actual_buffer_size );
        // Clear the buffer for fresh use
        memset( capture_buf, 0, actual_buffer_size );
    } else if ( raw_capture_buf ) {
        // Free and reallocate if buffer is corrupted
        JULSEDEBUG_BUF( "Freeing corrupted capture buffer\n\r" );
        free( raw_capture_buf );
        raw_capture_buf = nullptr;
        capture_buf = nullptr;
        actual_buffer_size = 0; // Reset buffer size
    }

    // Only allocate new buffer if we don't already have a valid one
    if ( !raw_capture_buf || !capture_buf || actual_buffer_size == 0 ) {
        // Show available memory before allocation
        JULSEDEBUG_BUF( "Free memory before allocation: %d\n\r", rp2040.getFreeHeap( ) );

        //  Ensure we don't allocate more memory than available
        uint32_t requested_buffer_size = JULSEVIEW_DMA_BUF_SIZE + 1024;
        uint32_t available_memory = rp2040.getFreeHeap( );

        // Leave some safety margin for other operations
        if ( requested_buffer_size > available_memory - 8192 ) {
            JULSEDEBUG_BUF( "WARNING: Requested buffer size %d bytes may exceed available memory %d bytes\n\r",
                            requested_buffer_size, available_memory );

            // Reduce buffer size to fit with safety margin
            requested_buffer_size = available_memory - 8192;
            JULSEDEBUG_BUF( "Reducing buffer size to %d bytes\n\r", requested_buffer_size );
        }

        // CRITICAL: Allocate buffer with proper alignment for ring buffers
        // We need 16KB alignment for analog ring buffers (largest ring size = 2^14)
        const size_t alignment = 16384; // 16KB alignment for 2^14 ring buffers (kept for compatibility)

        // Allocate extra space to allow for alignment adjustment
        size_t total_allocation = requested_buffer_size + alignment;

        // CRITICAL: Check available memory before allocation
        size_t free_heap = rp2040.getFreeHeap( );
        if ( free_heap < total_allocation + 1024 ) { // Leave 1KB safety margin
            JULSEDEBUG_ERR( "ERROR: Insufficient memory! Need %d bytes, have %d bytes\n\r",
                            total_allocation, free_heap );
            return false;
        }

        uint8_t* raw_buf = (uint8_t*)malloc( total_allocation );
        if ( !raw_buf ) {
            JULSEDEBUG_ERR( "ERROR: Failed to allocate %d bytes for capture buffer!\n\r", total_allocation );
            JULSEDEBUG_BUF( "Available memory: %d\n\r", free_heap );
            return false;
        }

        // Align the buffer to 16KB boundary
        capture_buf = (uint8_t*)( (uintptr_t)( raw_buf + alignment - 1 ) & ~( alignment - 1 ) );

        // Calculate how much extra memory we allocated
        size_t alignment_offset = (uint8_t*)capture_buf - raw_buf;
        size_t unused_memory = alignment - alignment_offset;

        JULSEDEBUG_BUF( "Aligned allocation: raw=0x%08X, aligned=0x%08X (offset=%d, unused=%d)\n\r",
                        (uint32_t)raw_buf, (uint32_t)capture_buf, alignment_offset, unused_memory );

        // Store raw buffer pointer so we can free it later
        raw_capture_buf = raw_buf;

        // Initialize buffer to zeros to avoid reading garbage
        memset( capture_buf, 0, requested_buffer_size );

        // CRITICAL: Add memory guard to detect buffer overflows
        uint32_t guard_value = 0xDEADBEEF;
        memcpy( raw_buf + total_allocation - 4, &guard_value, 4 );

        JULSEDEBUG_BUF( "Successfully allocated and initialized capture buffer: %d bytes at address 0x%08X\n\r",
                        requested_buffer_size, (uint32_t)capture_buf );

        // Store actual allocated size for later bounds checking
        actual_buffer_size = requested_buffer_size;
    } else {
        JULSEDEBUG_BUF( "Using existing buffer allocation (%d bytes)\n\r", actual_buffer_size );
    }

    // Initialize DMA channels to invalid values - will be claimed in arm() when we know the config
    admachan0 = admachan1 = pdmachan0 = pdmachan1 = coord_dma_chan = -1;

    // Initialize DMA register pointers to null - will be set up in arm()
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    piosm = pio_claim_unused_sm( pio, true );

    // Set bus priority for DMA operations (MUST be restored to 0 in deinit() to prevent CH446Q conflicts)
    bus_ctrl_hw->priority = BUSCTRL_BUS_PRIORITY_DMA_W_BITS | BUSCTRL_BUS_PRIORITY_DMA_R_BITS;

   // JULSEDEBUG_STA( "JulseView init() complete - DMA setup deferred to arm()\n\r" );
    initialized = true;
    return true;
}

void julseview::reset( ) {
    sending = false;
    cont = false;
    aborted = false;
    started = false;
    armed = false; // Device starts unarmed
    scnt = 0;
    rxstate = RX_IDLE;

    // Initialize enhanced state tracking
    isArmed = false;
    isRunning = false;
    isTriggered = false;
    receivedCommand = false;
    lastCommandTime = 0;
    commandTimeout = 5000; // Default 5 second timeout

    // Initialize trigger system
    trigger_config.type = TRIGGER_NONE;
    trigger_config.channel = 0;
    trigger_config.level = 0;
    trigger_config.edge = EDGE_RISING;
    trigger_config.var_address = 0;
    trigger_config.var_value = 0;
    trigger_config.enabled = false;
    trigger_armed = false;
    trigger_detected = false;
    pre_trigger_samples = 0;
    post_trigger_samples = 0;

    //  Reset ping-pong buffer state to ensure consistent behavior
    current_dma_half = 0; // Always start with buffer 0

    // Initialize transmission rate limiting
    last_transmission_time = 0;
    transmission_count = 0;

    // Initialize decimation configuration
    use_decimation_mode = false;
    analog_decimation_factor = 1;
    digital_samples_per_half = 0;
    analog_samples_per_half = 0;

    // Clean up DMA channels if they were claimed
    cleanup_dma_channels( );
}

void julseview::cleanup_dma_channels( ) {
    // Release DMA channels if they were claimed
    if ( admachan0 >= 0 ) {
        dma_channel_unclaim( admachan0 );
        admachan0 = -1;
    }
    if ( admachan1 >= 0 ) {
        dma_channel_unclaim( admachan1 );
        admachan1 = -1;
    }
    if ( pdmachan0 >= 0 ) {
        dma_channel_unclaim( pdmachan0 );
        pdmachan0 = -1;
    }
    if ( pdmachan1 >= 0 ) {
        dma_channel_unclaim( pdmachan1 );
        pdmachan1 = -1;
    }
    if ( coord_dma_chan >= 0 ) {
        dma_channel_unclaim( coord_dma_chan );
        coord_dma_chan = -1;
    }

    // Clear DMA register pointers
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    // ADC monitor timer cleanup (if needed in future)

    JULSEDEBUG_DMA( "DMA channels cleaned up\n\r" );
}

void julseview::handler( ) {
    // FAST HANDLER - Minimal checks for tight loop performance

    // Quick USB check
    if ( !USBSer2 || !USBSer2.available( ) ) {
        return;
    }

    // Process USB data immediately
    char charin = USBSer2.read( );
    if ( process_char( charin ) ) {
        // Update command state when we process a command
        receivedCommand = true;
        lastCommandTime = millis( );
        this->julseview_usb_out_chars( rspstr, strlen( rspstr ) );
    }
}

void julseview::arm( ) {
    JULSEDEBUG_STA( "=== JULSEVIEW ARM() START ===\n\r" );

    // DEBUG: Print PIO state machine status before arming
    printPIOStateMachines( );

    //  Set default num_samples if driver didn't send 'L' command
    if ( num_samples == 0 ) {
        JULSEDEBUG_CMD( "WARNING: Driver didn't send 'L' command, using default 10000 samples\n\r" );
        num_samples = 10000;
    }

    JULSEDEBUG_STA( "Stopping ADC...\n\r" );
    adc_run( false );

    //  Set default sample_rate if driver didn't send 'R' command
    if ( sample_rate == 0 ) {
        JULSEDEBUG_CMD( "WARNING: Driver didn't send 'R' command, using default 5000 Hz\n\r" );
        sample_rate = 5000;
    }

    julseview_active = true;
    isArmed = true;
    isRunning = false; // Not running yet, just armed
    isTriggered = false;
    receivedCommand = false; // Reset command state
    dma_ended_flag = false; // Reset DMA end flag for new capture
    JULSEDEBUG_STA( "JulseView active set to true, isArmed=true\n\r" );

    JULSEDEBUG_STA( "Configuring DMA mode...\n\r" );

    // SMART HYBRID CONFIGURATION:
    // This mode automatically selects the optimal DMA strategy:
    // - Single DMA if all samples fit in one buffer (prevents switching issues)
    // - Producer-Consumer + Coordinator DMA if multiple buffers needed (hardware-timed safe switching)

    use_dual_dma = true;          // May be overridden to false if single buffer sufficient
    use_smart_pingpong = false;   // Using hybrid approach instead
    use_coordinator_dma = false;  // Will be enabled automatically if multiple buffers needed
    use_producer_consumer = true; // Enable smart mode selection

    JULSEDEBUG_STA( "DMA mode: Smart Hybrid (will auto-select optimal strategy)\n\r" );

    JULSEDEBUG_STA( "Counting analog channels...\n\r" );
    JULSEDEBUG_STA( "DEBUG: a_mask before counting: 0x%08X\n\r", a_mask );
    a_chan_cnt = 0;
    for ( int i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
        if ( ( ( a_mask ) >> i ) & 1 ) {
            a_chan_cnt++;
            JULSEDEBUG_STA( "DEBUG: Found enabled analog channel %d\n\r", i );
        }
    }

    if ( a_chan_cnt == 0 ) {
        a_chan_cnt = JULSEVIEW_DEFAULT_ANALOG_CHANNELS;
        a_mask = (1 << JULSEVIEW_DEFAULT_ANALOG_CHANNELS) - 1;
        JULSEDEBUG_STA( "DEBUG: No channels enabled, using default: %d channels, mask=0x%08X\n\r", a_chan_cnt, a_mask );
    }

    JULSEDEBUG_STA( "Analog channels: %d\n\r", a_chan_cnt );

    // Configure decimation mode based on sample rate and analog channel count
    configure_decimation_mode();

    JULSEDEBUG_STA( "Setting up digital channels...\n\r" );
    //  Always capture 8 digital channels for consistent DMA configuration
    d_chan_cnt = 8; // Always 8 channels regardless of mask
    JULSEDEBUG_DIG( "DEBUG: d_mask=0x%08X, using fixed d_chan_cnt=%d\n\r", d_mask, d_chan_cnt );

    // Calculate pin count for display purposes only
    pin_count = 0;
    if ( d_mask & 0x0000000F )
        pin_count += 4;
    if ( d_mask & 0x000000F0 )
        pin_count += 4;
    if ( d_mask & 0x0000FF00 )
        pin_count += 8;
    if ( d_mask & 0x0FFF0000 )
        pin_count += 16;
    if ( ( pin_count == 4 ) && ( a_chan_cnt ) ) {
        pin_count = 8;
    }

    d_nps = pin_count / 4;
    d_tx_bps = 2;

    // Set buffer bytes per sample - always 1 byte for fixed 8-bit digital capture
    JULSEDEBUG_DIG( "DEBUG: Fixed d_chan_cnt=%d for consistent DMA\n\r", d_chan_cnt );

    d_dma_bps = 1; // Always 1 byte for fixed 8-bit capture
    JULSEDEBUG_DIG( "DEBUG: Set d_dma_bps=1 (fixed 8-bit capture)\n\r" );

    JULSEDEBUG_DIG( "JulseView pin_count: %d, d_dma_bps: %d\n\r", pin_count, d_dma_bps );
    JULSEDEBUG_DIG( "JulseView d_chan_cnt: %d, d_nps: %d, d_tx_bps: %d\n\r", d_chan_cnt, d_nps, d_tx_bps );

    JULSEDEBUG_STA( "Starting DMA setup...\n\r" );
    // SETUP DMA CHANNELS AND CONFIGURATIONS (moved from init() to arm() for efficiency)
    // Only set up what we actually need based on the current configuration
    JULSEDEBUG_DMA( "Setting up DMA channels and configurations...\n\r" );

    // Initialize ADC if we have analog channels
     if (a_chan_cnt > 0) {
    JULSEDEBUG_STA( "Initializing ADC...\n\r" );
    adc_init( );
    JULSEDEBUG_ANA( "ADC initialized for %d analog channels\n\r", a_chan_cnt );
     } else {
    // JULSEDEBUG_STA("No analog channels, skipping ADC init\n\r");
    }

    // Claim DMA channels based on what we actually need
    if (a_chan_cnt > 0) {
    JULSEDEBUG_STA( "Claiming analog DMA channels...\n\r" );
    admachan0 = dma_claim_unused_channel( true );
    admachan1 = dma_claim_unused_channel( true );
    JULSEDEBUG_DMA( "Claimed analog DMA channels: %d, %d\n\r", admachan0, admachan1 );
    }

    if (d_chan_cnt > 0) {
    JULSEDEBUG_STA( "Claiming digital DMA channels...\n\r" );
    pdmachan0 = dma_claim_unused_channel( true );
    pdmachan1 = dma_claim_unused_channel( true );
    JULSEDEBUG_DMA( "Claimed digital DMA channels: %d, %d\n\r", pdmachan0, pdmachan1 );
    }

    // Claim coordinator DMA channel if we need dual-buffer mode
    // coord_dma_chan = dma_claim_unused_channel(true);
  //  JULSEDEBUG_DMA( "Claimed coordinator DMA channel: %d\n\r", coord_dma_chan );

    // Setup DMA configurations based on actual channel counts
    if (a_chan_cnt > 0) {
    acfg0 = dma_channel_get_default_config( admachan0 );
    acfg1 = dma_channel_get_default_config( admachan1 );

    // Configure analog DMA channels
    channel_config_set_transfer_data_size( &acfg0, DMA_SIZE_16 ); // 16-bit to preserve 12-bit ADC resolution
    channel_config_set_transfer_data_size( &acfg1, DMA_SIZE_16 );
    channel_config_set_read_increment( &acfg0, false ); // ADC FIFO doesn't increment
    channel_config_set_read_increment( &acfg1, false );
    channel_config_set_write_increment( &acfg0, true ); // Write to buffer
    channel_config_set_write_increment( &acfg1, true );
    channel_config_set_dreq( &acfg0, DREQ_ADC ); // Use ADC data request
    channel_config_set_dreq( &acfg1, DREQ_ADC );

    JULSEDEBUG_DMA( "Analog DMA channels configured\n\r" );
    }

    if (d_chan_cnt > 0) {
    pcfg0 = dma_channel_get_default_config( pdmachan0 );
    pcfg1 = dma_channel_get_default_config( pdmachan1 );

    // Configure digital DMA channels - always use 8-bit transfers for fixed 8-bit capture
    channel_config_set_transfer_data_size( &pcfg0, DMA_SIZE_8 );
    channel_config_set_transfer_data_size( &pcfg1, DMA_SIZE_8 );

    channel_config_set_read_increment( &pcfg0, false ); // PIO FIFO doesn't increment
    channel_config_set_read_increment( &pcfg1, false );
    channel_config_set_write_increment( &pcfg0, true ); // Write to buffer
    channel_config_set_write_increment( &pcfg1, true );
    // DREQ will be set when PIO is configured - we'll set it to use PIO RX FIFO

    JULSEDEBUG_DMA( "Digital DMA channels configured for %d channels\n\r", d_chan_cnt );
    }

    // Setup DMA coordinator IRQ
    // dma_channel_set_irq0_enabled(coord_dma_chan, true);
    // irq_set_exclusive_handler(DMA_IRQ_0, coordinator_dma_irq_handler);
    // irq_set_enabled(DMA_IRQ_0, true);
    // JULSEDEBUG_DMA("DMA coordinator IRQ configured\n\r");

    // ADC Monitor Timer setup (commented out - not needed for current implementation)

    // Set up DMA register pointers for direct access during transmission
    // This allows us to monitor DMA progress without using the SDK functions
    // which can be slow and cause timing issues during high-speed data transmission
    if (a_chan_cnt > 0) {
    taddra0 = &dma_hw->ch[ admachan0 ].write_addr; // Analog DMA channel 0 write address
    taddra1 = &dma_hw->ch[ admachan1 ].write_addr; // Analog DMA channel 1 write address
    // Use sigrok-pico approach: DMA_BASE + channel offset + register offset
    tstsa0 = (volatile uint32_t*)( DMA_BASE + 0x40 * admachan0 + 0xc ); // DMA_WRITE_sts offset
    tstsa1 = (volatile uint32_t*)( DMA_BASE + 0x40 * admachan1 + 0xc ); // DMA_WRITE_sts offset
                                                                        }
                                                                        // }

     if (d_chan_cnt > 0) {
    taddrd0 = &dma_hw->ch[ pdmachan0 ].write_addr; // Digital DMA channel 0 write address
    taddrd1 = &dma_hw->ch[ pdmachan1 ].write_addr; // Digital DMA channel 1 write address
    // Use sigrok-pico approach: DMA_BASE + channel offset + register offset
    tstsd0 = (volatile uint32_t*)( DMA_BASE + 0x40 * pdmachan0 + 0xc ); // DMA_WRITE_sts offset
    tstsd1 = (volatile uint32_t*)( DMA_BASE + 0x40 * pdmachan1 + 0xc ); // DMA_WRITE_sts offset
                                                                        }

    JULSEDEBUG_DMA( "DMA setup complete - all channels and configurations ready\n\r" );

    lowerhalf = 1;
    num_halves = 0;

    // --- Asymmetric Buffer Layout for Decimation Mode ---
    // Calculate buffer layout based on decimation mode
    calculate_asymmetric_buffer_layout();

    JULSEDEBUG_BUF( "Buffer addresses: dbuf0=0x%08X, abuf0=0x%08X, dbuf1=0x%08X, abuf1=0x%08X\n\r",
                    (uint32_t)&capture_buf[ dbuf0_start ], (uint32_t)&capture_buf[ abuf0_start ],
                    (uint32_t)&capture_buf[ dbuf1_start ], (uint32_t)&capture_buf[ abuf1_start ] );

    // Calculate samples per buffer based on fixed buffer sizes
    // With fixed 8KB digital and 64KB analog buffers, determine the limiting factor
    // uint32_t digital_samples = (d_chan_cnt > 0) ? (d_size / d_dma_bps) : UINT32_MAX;
    // uint32_t analog_samples = (a_chan_cnt > 0) ? (a_size / (a_chan_cnt * 2)) : UINT32_MAX;

    // ADAPTIVE TRANSFER SIZE: Calculate sample counts based on decimation mode
    if (use_decimation_mode) {
        // In decimation mode, use asymmetric sample counts
        samples_per_half = digital_samples_per_half; // Use digital count as primary
        JULSEDEBUG_BUF( "DECIMATION MODE: digital_samples_per_half=%d, analog_samples_per_half=%d\n\r",
                        digital_samples_per_half, analog_samples_per_half );
        JULSEDEBUG_BUF( "DECIMATION MODE: using digital sample count as primary: %d\n\r", samples_per_half );
    } else {
        // Normal mode: Calculate which buffer can hold fewer samples
        uint32_t digital_samples_per_buffer = ( d_chan_cnt > 0 ) ? ( d_size / d_dma_bps ) : UINT32_MAX;
        uint32_t analog_samples_per_buffer = ( a_chan_cnt > 0 ) ? ( a_size / ( a_chan_cnt * 2 ) ) : UINT32_MAX;

        // Use the smaller of the two as our base sample count
        uint32_t base_samples = ( digital_samples_per_buffer < analog_samples_per_buffer ) ? digital_samples_per_buffer : analog_samples_per_buffer;

        JULSEDEBUG_BUF( "NORMAL MODE: digital_samples_per_buffer=%d, analog_samples_per_buffer=%d\n\r",
                        digital_samples_per_buffer, analog_samples_per_buffer );
        JULSEDEBUG_BUF( "NORMAL MODE: selected base_samples=%d\n\r", base_samples );

        // CRITICAL ADC ALIGNMENT FIX: Ensure samples_per_half is multiple of ADC channels
        if (a_chan_cnt > 0) {
            samples_per_half = ( base_samples / a_chan_cnt ) * a_chan_cnt; // Round down to multiple of channels
            JULSEDEBUG_BUF( "ADC CHANNEL ALIGNMENT: base_samples=%d, channels=%d, aligned_samples=%d\n\r",
                            base_samples, a_chan_cnt, samples_per_half );
        } else {
            samples_per_half = base_samples;
        }
        
        // Set asymmetric sample counts for consistency
        digital_samples_per_half = analog_samples_per_half = samples_per_half;
    }

    if ( samples_per_half == UINT32_MAX || samples_per_half == 0 ) {
        JULSEDEBUG_ERR( "ERROR: No channels enabled, cannot calculate samples_per_half.\n\r" );
        return;
    }

    // Always use producer-consumer for simplicity and robustness.
    use_single_buffer = false;
    use_producer_consumer = true;

    // JULSEDEBUG_BUF( "--- RING BUFFER CONFIGURATION ---\n\r" );
    // JULSEDEBUG_BUF( "Total Buffer: %d KB (Analog-first layout for perfect alignment)\n\r", JULSEVIEW_DMA_BUF_SIZE / 1024 );
    // JULSEDEBUG_BUF( "Optimized d_size: %d, a_size: %d\n\r", d_size, a_size );
    // JULSEDEBUG_BUF( "Layout: abuf0[0x%04X], abuf1[0x%04X], dbuf0[0x%04X], dbuf1[0x%04X]\n\r", abuf0_start, abuf1_start, dbuf0_start, dbuf1_start );
    // JULSEDEBUG_BUF( "Samples per buffer: %d\n\r", samples_per_half );
    // JULSEDEBUG_BUF( "Ring Buffers: Analog=64KB(2^16), Digital=8KB(2^13) + Manual Address Reset\n\r" );
    // JULSEDEBUG_BUF( "------------------------------------\n\r" );

    // dma_channel_abort( admachan0 );
    // dma_channel_abort( admachan1 );
    // dma_channel_abort( pdmachan0 );
    // dma_channel_abort( pdmachan1 );

    //         // --- Simplified DMA Configuration ---
    // uint32_t analog_transfers = a_size / 2;
    // uint32_t digital_transfers = d_size / d_dma_bps;  // Back to 8-bit: d_size / 1

    //  Use correct transfer counts based on decimation mode
    uint32_t digital_transfers, analog_transfers;
    
    if (use_decimation_mode) {
        // In decimation mode, use asymmetric transfer counts
        digital_transfers = digital_samples_per_half * d_dma_bps;
        analog_transfers = analog_samples_per_half * a_chan_cnt * 2 / 2;
        
        JULSEDEBUG_DMA( "DECIMATION TRANSFER COUNTS: digital=%d, analog=%d\n\r", 
                        digital_transfers, analog_transfers );
    } else {
        // Normal mode: use same sample count for both
        digital_transfers = samples_per_half * d_dma_bps;
        analog_transfers = samples_per_half * a_chan_cnt * 2 / 2;
        
        JULSEDEBUG_DMA( "NORMAL TRANSFER COUNTS: digital=%d, analog=%d\n\r", 
                        digital_transfers, analog_transfers );
    }

    JULSEDEBUG_DMA( "TRANSFER COUNT FIX: samples_per_half=%d, digital_transfers=%d, analog_transfers=%d\n\r",
                    samples_per_half, digital_transfers, analog_transfers );

    JULSEDEBUG_DMA( "Transfer counts: digital=%d, analog=%d\n\r", digital_transfers, analog_transfers );

    //  Enable proper DMA chaining using corrected RP2350B register approach
    // Use sigrok-pico DMA chaining with correct register offsets
    if ( a_chan_cnt > 0 ) {
        channel_config_set_chain_to( &acfg0, admachan1 ); // Analog 0 chains to Analog 1
        channel_config_set_chain_to( &acfg1, admachan0 ); // Analog 1 chains to Analog 0
    }
    if ( d_chan_cnt > 0 ) {
        channel_config_set_chain_to( &pcfg0, pdmachan1 ); // Digital 0 chains to Digital 1
        channel_config_set_chain_to( &pcfg1, pdmachan0 ); // Digital 1 chains to Digital 0
    }

    // --- Analog Channels ---
    if ( a_chan_cnt > 0 ) {
        // CRITICAL: DMA chaining approach with proper startup
        dma_channel_configure( admachan0, &acfg0, &capture_buf[ abuf0_start ], &adc_hw->fifo, analog_transfers, false ); // Start chaining
        dma_channel_configure( admachan1, &acfg1, &capture_buf[ abuf1_start ], &adc_hw->fifo, analog_transfers, false ); // Don't start
    }

    // --- Digital Channels ---
    if ( d_chan_cnt > 0 ) {
        dreq = pio_get_dreq( pio, piosm, false ); // Use false like sigrok-pico
        channel_config_set_dreq( &pcfg0, dreq );
        channel_config_set_dreq( &pcfg1, dreq );

        // CRITICAL: DMA chaining approach with proper startup
        // Don't start DMA immediately in trigger mode - wait for trigger detection
        bool start_dma_immediately = !trigger_config.enabled;
        dma_channel_configure( pdmachan0, &pcfg0, &capture_buf[ dbuf0_start ], &pio->rxf[ piosm ], digital_transfers, false ); // Start chaining
        dma_channel_configure( pdmachan1, &pcfg1, &capture_buf[ dbuf1_start ], &pio->rxf[ piosm ], digital_transfers, false ); // Don't start
    }

    // --- ADC Configuration (only if analog channels enabled) ---
    if ( a_chan_cnt > 0 ) {
        adc_run( false );
        JULSEDEBUG_ANA( "ADC stopped\n\r" );

        // Two-stage setup like reference: first setup without enabling
        adc_fifo_setup( false, true, 1, false, false );
        adc_fifo_drain( );

        JULSEDEBUG_ANA( "ADC FIFO setup\n\r" );

        // CRITICAL SAFETY: Add ADC divisor calculation validation
        if ( sample_rate == 0 || a_chan_cnt == 0 ) {
            JULSEDEBUG_ERR( "ERROR: Invalid ADC parameters! sample_rate=%d, a_chan_cnt=%d\n\r", sample_rate, a_chan_cnt );
            return;
        }

        // Calculate ADC divisor with fractional part like reference
        uint32_t actual_adc_rate = use_decimation_mode ? (sample_rate / analog_decimation_factor) : sample_rate;
        uint32_t adcdivint = 48000000ULL / ( actual_adc_rate * a_chan_cnt );
        uint8_t adc_frac_int = (uint8_t)( ( ( 48000000ULL % ( actual_adc_rate * a_chan_cnt ) ) * 256ULL ) / ( actual_adc_rate * a_chan_cnt ) );
        
        JULSEDEBUG_ANA( "ADC RATE CALCULATION: requested=%d Hz, actual=%d Hz, decimation_factor=%d\n\r", 
                        sample_rate, actual_adc_rate, analog_decimation_factor );

        // CRITICAL SAFETY: Validate ADC divisor
        if ( adcdivint == 0 || adcdivint > 65535 ) {
            JULSEDEBUG_ERR( "ERROR: Invalid ADC divisor! adcdivint=%d\n\r", adcdivint );
            adcdivint = 1;
        }

        uint8_t adc_mask = 0;
        /* Build ADC round-robin mask from actual enabled channels */
        for ( int i = 0; i < 8; i++ ) {
            if ( ( a_mask >> i ) & 1 ) {
                // CRITICAL SAFETY: Validate ADC pin number
                if ( 40 + i > 47 ) {
                    JULSEDEBUG_ERR( "ERROR: ADC pin out of range! pin=%d\n\r", 40 + i );
                    continue;
                }

                // adc_gpio_init(40 + i);  // Jumperless ADCs on pins 40-47
                adc_mask |= ( 1 << i );
                JULSEDEBUG_ANA( "ADC enabled on pin %d (channel %d)\n\r", 40 + i, i );
            }
        }

        JULSEDEBUG_ANA( "ADC round-robin mask: 0x%02X (binary: %08b)\n\r", adc_mask, adc_mask );

        // Calculate first enabled channel for synchronization
        adc_first_channel = get_first_adc_channel( adc_mask );
        JULSEDEBUG_ANA( "First enabled ADC channel: %d\n\r", adc_first_channel );

        // CRITICAL: Must call adc_select_input() immediately before adc_set_round_robin()
        // to clear AINSEL so round-robin starts deterministically at first enabled channel
        adc_select_input( adc_first_channel );
        adc_set_round_robin( adc_mask );
        adc_fifo_drain( ); // Clear any stale data to ensure we start fresh

        // Apply ADC divisor like reference using direct register write
        volatile uint32_t* adcdiv = (volatile uint32_t*)( ADC_BASE + 0x10 );

        // CRITICAL SAFETY: Validate ADC register pointer
        if ( adcdiv == nullptr ) {
            JULSEDEBUG_ERR( "ERROR: ADC divisor register pointer is null!\n\r" );
            return;
        }

        if ( adcdivint <= 96 ) {
            *adcdiv = 0; // Special case for high sample rates
        } else {
            *adcdiv = ( ( adcdivint - 1 ) << 8 ) | adc_frac_int;
        }

        // Use 16-bit FIFO mode for full 12-bit ADC resolution
        uint8_t fifo_threshold = 8;                                 // Higher threshold to buffer multiple samples and reduce timing sensitivity
        adc_fifo_setup( true, true, fifo_threshold, false, false ); // byte_shift=false for 16-bit

        JULSEDEBUG_ANA( "ADC FIFO threshold set to: %d (16-bit mode)\n\r", fifo_threshold );

        JULSEDEBUG_ANA( "ADC DMA config - transfer count: %d, bytes per transfer: 2 (16-bit mode)\n\r", analog_transfers );

        // The DMA channels are now configured in the 'Simplified DMA Configuration' block.
        // This old logic is no longer needed.

        // Critical: Drain FIFO after DMA setup to ensure clean start
        adc_fifo_drain( );

        // Add FIFO monitoring debug
        JULSEDEBUG_ANA( "ADC FIFO level after drain: %d\n\r", adc_fifo_get_level( ) );
        JULSEDEBUG_ANA( "ADC sample rate: %d\n\r", sample_rate );
        JULSEDEBUG_ANA( " Hz, channels: %d\n\r", a_chan_cnt );
        JULSEDEBUG_ANA( "effective rate per channel: %d\n\r", sample_rate / a_chan_cnt );
        JULSEDEBUG_ANA( " Hz, data rate: %d\n\r", sample_rate * a_chan_cnt * 2 / 1000 );
        JULSEDEBUG_ANA( " KB/s\n\r" );
        JULSEDEBUG_ANA( "ADC divisor: int=%d\n\r", adcdivint );
        JULSEDEBUG_ANA( "frac=%d\n\r", adc_frac_int );
    } else {
        JULSEDEBUG_ANA( "No analog channels enabled - skipping ADC configuration\n\r" );
    }

    //   }

    if (d_chan_cnt > 0) {

    // CRITICAL SAFETY: Add bounds checking before buffer operations
    if ( capture_buf == nullptr ) {
        JULSEDEBUG_ERR( "ERROR: capture_buf is null during digital setup!\n\r" );
        return;
    }

    if ( dbuf0_start + d_size > actual_buffer_size || dbuf1_start + d_size > actual_buffer_size ) {
        JULSEDEBUG_ERR( "ERROR: Digital buffer bounds check failed! dbuf0_start=%d, dbuf1_start=%d, d_size=%d, actual_buffer_size=%d\n\r",
                        dbuf0_start, dbuf1_start, d_size, actual_buffer_size );
        return;
    }

    // CRITICAL: Clear digital buffer to prevent contamination from previous captures
    memset( &capture_buf[ dbuf0_start ], 0, d_size );
    memset( &capture_buf[ dbuf1_start ], 0, d_size );
    JULSEDEBUG_DIG( "◆ Digital capture buffers cleared\n\r" );

    // CRITICAL SAFETY: Add PIO validation
    if ( pio == nullptr ) {
        JULSEDEBUG_ERR( "ERROR: PIO is null during digital setup!\n\r" );
        return;
    }

    // Debug: Show PIO state before attempting to add program
    JULSEDEBUG_CMD( "PIO %d state before program add: SM0=%s, SM1=%s, SM2=%s, SM3=%s\n\r",
                    pio,
                    pio_sm_is_claimed( pio, 0 ) ? "CLAIMED" : "FREE",
                    pio_sm_is_claimed( pio, 1 ) ? "CLAIMED" : "FREE",
                    pio_sm_is_claimed( pio, 2 ) ? "CLAIMED" : "FREE",
                    pio_sm_is_claimed( pio, 3 ) ? "CLAIMED" : "FREE" );

    // Check if we can add the PIO program before attempting to add it
    if ( !pio_can_add_program( pio, &pio_logic_analyzer_program ) ) {
        JULSEDEBUG_ERR( "ERROR: Cannot add PIO program - instruction memory full or program already loaded!\n\r" );
        JULSEDEBUG_ERR( "PIO %d instruction memory may be full or program already present\n\r", pio );

        // Try to remove the program first and then re-add it
        JULSEDEBUG_CMD( "Attempting to remove and re-add PIO program...\n\r" );
        pio_remove_program( pio, &pio_logic_analyzer_program, 0 );

        // Check again if we can add it now
        if ( !pio_can_add_program( pio, &pio_logic_analyzer_program ) ) {
            JULSEDEBUG_ERR( "ERROR: Still cannot add PIO program after removal!\n\r" );
            return;
        }
        JULSEDEBUG_CMD( "Successfully removed and can re-add PIO program\n\r" );
    }

    // pio_sm_set_enabled(pio, piosm, false);
    // pio_sm_clear_fifos(pio, piosm);
    // pio_sm_restart(pio, piosm);

    uint offset = pio_add_program( pio, &pio_logic_analyzer_program );
    if ( offset == -1 ) {
        JULSEDEBUG_ERR( "ERROR: Failed to add PIO program despite can_add check!\n\r" );
        return;
    }
    JULSEDEBUG_DIG( "PIO %d program added\n\r", pio );
    pio_sm_config c = pio_get_default_sm_config( );

    sm_config_set_in_pins( &c, 20 ); // Start at GPIO 20
    sm_config_set_in_pin_count( &c, 8 );
    sm_config_set_in_shift( &c, false, true, 8 ); // 8-bit shift, autopush enabled for 8 channels (fixed)
    sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );

    //  Set wrap to loop continuously over the single instruction (like sigrok-pico)
    sm_config_set_wrap( &c, offset, offset );

    // Clock divider calculation using the working method
    uint32_t sys_clock = clock_get_hz( clk_sys );
    float clock_div = (float)sys_clock / sample_rate; // the 2 is because there are 2 instructions in the state machine
    if ( clock_div < 1.0f )
        clock_div = 1.0f;
    if ( clock_div > 65536.0f )
        clock_div = 65536.0f;

    sm_config_set_clkdiv( &c, clock_div );

    // Debug: Log PIO configuration details
    JULSEDEBUG_CMD( "PIO CONFIG DEBUG: sample_rate=%d, sys_clock=%d, clock_div=%.2f\n\r",
                    sample_rate, sys_clock, clock_div );
    JULSEDEBUG_CMD( "PIO CONFIG DEBUG: trigger_enabled=%d, pre_trigger_samples=%d, post_trigger_samples=%d\n\r",
                    trigger_config.enabled, pre_trigger_samples, post_trigger_samples );

    pio_sm_init( pio, piosm, offset, &c );

    // Verify PIO state machine was initialized correctly
    if ( !pio_sm_is_claimed( pio, piosm ) ) {
        JULSEDEBUG_ERR( "ERROR: PIO state machine %d not claimed after initialization!\n\r", piosm );
        return;
    }

    JULSEDEBUG_CMD( "PIO program loaded successfully at offset %d on PIO %d, SM %d\n\r", offset, pio, piosm );

    // CRITICAL SAFETY: Add DMA configuration validation
    if ( pdmachan0 < 0 || pdmachan1 < 0 ) {
        JULSEDEBUG_ERR( "ERROR: Digital DMA channels not claimed! pdmachan0=%d, pdmachan1=%d\n\r", pdmachan0, pdmachan1 );
        return;
    }

    // Configure DMA for digital capture - use 8-bit transfers for 8 channels
    channel_config_set_transfer_data_size( &pcfg0, DMA_SIZE_8 );
    channel_config_set_transfer_data_size( &pcfg1, DMA_SIZE_8 );

    channel_config_set_read_increment( &pcfg0, false ); // PIO FIFO doesn't increment
    channel_config_set_read_increment( &pcfg1, false );

    channel_config_set_write_increment( &pcfg0, true ); // Write to buffer
    channel_config_set_write_increment( &pcfg1, true );

    dreq = pio_get_dreq( pio, piosm, false );
    if ( dreq == -1 ) {
        JULSEDEBUG_ERR( "ERROR: Failed to get PIO DREQ!\n\r" );
        return;
    }

    channel_config_set_dreq( &pcfg0, dreq ); // Use PIO RX FIFO
    channel_config_set_dreq( &pcfg1, dreq );

    JULSEDEBUG_DIG( "PIO configured for %d channels, sample rate: %d Hz\n\r", d_chan_cnt, sample_rate );
    JULSEDEBUG_DIG( "PIO clock divider: %.2f (sys_clock=%d Hz)\n\r", clock_div, sys_clock );
    JULSEDEBUG_DIG( "Digital DMA configured for 8-bit transfers, buffer size: %d bytes\n\r", d_size );
    JULSEDEBUG_DIG( "Digital DMA DREQ configured: %d (PIO RX FIFO)\n\r", pio_get_dreq( pio, piosm, false ) );

    // The DMA channels are now configured in the 'Simplified DMA Configuration' block.
    // This old logic is no longer needed.

    // CRITICAL DEBUG: Verify PIO and DMA configuration match (fixed 8-channel)
    JULSEDEBUG_DIG( "Digital configuration verification:\n\r" );
    JULSEDEBUG_DIG( "  Channels: %d (fixed), d_dma_bps: %d, PIO autopush: 8 bits\n\r",
                    d_chan_cnt, d_dma_bps );
    JULSEDEBUG_DIG( "  DMA transfer size: 8-bit (fixed)\n\r" );

    // CRITICAL CHECK: Verify DMA and PIO configurations are compatible
    uint8_t expected_dma_bps = 1; // Fixed for 8 channels
    if ( d_dma_bps != expected_dma_bps ) {
        JULSEDEBUG_DIG( "WARNING: DMA bytes per sample (%d) doesn't match fixed 8-channel setup!\n\r", d_dma_bps );
        JULSEDEBUG_DIG( "Expected: %d bytes for %d channels with 8-bit autopush\n\r",
                        expected_dma_bps, d_chan_cnt );
    } else {
        JULSEDEBUG_DIG( "✓ DMA and PIO configurations match (fixed 8-channel)\n\r" );
    }
     }

    // Final FIFO cleanup before starting capture
    if (a_mask) {   
    adc_fifo_drain( );
    JULSEDEBUG_ANA( "Final ADC FIFO level before capture start: %d\n\r", adc_fifo_get_level( ) );
     }

    // Initialize capture state variables
    sending = false;      // Will be set to true when run() starts
    started = false;      // Will be set to true when run() starts
    armed = true;         // Device is now armed and ready for trigger
    scnt = 0;             // Reset sample counter for new capture
    total_bytes_sent = 0; // Reset byte counter for new capture

    //  Ensure num_samples is not corrupted before storing
    if ( num_samples == 0 ) {
        JULSEDEBUG_ERR( "ERROR: num_samples is 0 before capture start! Using default 10000\n\r" );
        num_samples = 10000;
    }

    // Store original num_samples to prevent corruption
    original_num_samples = num_samples;
    JULSEDEBUG_DIG( "DEBUG: Armed with num_samples: %d (stored: %d)\n\r", num_samples, original_num_samples );

    JULSEDEBUG_DIG( "JulseView armed - analog channels: %d, digital channels: %d, samples: %d\n\r", a_chan_cnt, d_chan_cnt, num_samples );

    JULSEDEBUG_STA( "=== JULSEVIEW ARM() COMPLETE - READY FOR TRIGGER ===\n\r" );

    // Setup trigger if configured
    if ( trigger_config.enabled ) {
        JULSEDEBUG_CMD( "Trigger is enabled, setting up trigger system\n\r" );
        setup_trigger( );
    } else {
        JULSEDEBUG_CMD( "No trigger configured, proceeding with normal capture\n\r" );
    }
}

void julseview::run( ) {
    JULSEDEBUG_STA( "=== JULSEVIEW RUN() START ===\n\r" );

    // DEBUG: Print PIO state machine status before running
    /// printPIOStateMachines();
    bool trigger_monitoring = trigger_config.enabled && !trigger_detected;

    JULSEDEBUG_CMD( "Starting main capture loop - trigger_enabled=%d, trigger_detected=%d\n\r",
                    trigger_config.enabled, trigger_detected );
    JULSEDEBUG_CMD( "Loop condition check: julseview_active=%d, cont=%d, scnt=%d, num_samples=%d\n\r",
                    julseview_active, cont, scnt, num_samples );

    // If trigger is enabled, wait for trigger before starting capture

    // Set capture state to active
    sending = true;
    started = true;
    isRunning = true;
    isArmed = false; // No longer armed, now running
    isTriggered = true;
    JULSEDEBUG_STA( "Device state: isRunning=true, isArmed=false\n\r" );

    JULSEDEBUG_DIG( "JulseView capture started - analog channels: %d, digital channels: %d, samples: %d\n\r", a_chan_cnt, d_chan_cnt, num_samples );

    // Prepare analog capture if enabled (but don't start yet in trigger mode)
    adc_fifo_drain( );

    // Initialize current DMA half tracking
    current_dma_half = 0; // Start with half 0

    pio_sm_set_enabled( pio, piosm, true );
    adc_run( true );

    // Main capture loop
    uint32_t loop_count = 0;
    uint32_t last_debug_time = millis( );

    if ( trigger_config.enabled && !trigger_detected ) {
        JULSEDEBUG_CMD( "Waiting for trigger before starting capture...\n\r" );
        waitForTrigger( );
        // JULSEDEBUG_CMD("Trigger detected - starting capture  scnt=%d\n\r", scnt);

        // Reset sample counter to 0 when trigger is detected
        // This ensures only post-trigger samples are counted
        scnt = 0;
        // JULSEDEBUG_CMD("Sample counter reset to 0 for post-trigger capture\n\r");
    }

    

    
    

    // Start DMA channels - PIO/ADC are already running from arm()
    if (d_chan_cnt > 0) {
        pio_sm_clear_fifos( pio, piosm );
        dma_channel_start( pdmachan0 );
        //JULSEDEBUG_CMD("Digital DMA STARTED: %s mode\n\r", trigger_config.enabled ? "Post-trigger" : "Normal");
    }
    if (a_chan_cnt > 0) {
        adc_fifo_drain( );    
        adc_select_input( get_first_adc_channel(a_mask) );
        
        // Small delay to ensure ADC is ready
        sleep_us(100);
        
        dma_channel_start( admachan0 );
        //JULSEDEBUG_CMD("Analog DMA STARTED: %s mode\n\r", trigger_config.enabled ? "Post-trigger" : "Normal");
    }

    // Main capture loop (simplified - no trigger monitoring here)

    //JULSEDEBUG_CMD( "Starting main capture loop julseview_active= %s scnt= %d num_samples= %d\n\r", julseview_active ? "true" : "false", scnt, num_samples );
    while ( julseview_active && scnt < num_samples ) {

        dma_check( );

        // if ( millis( ) - last_debug_time > 200 ) {
        //     if ( USBSer2.available( ) ) {
        //         // process_char(USBSer2.read());

        //         JULSEDEBUG_CMD( "USBSer2.available()\n\r" );
        //     }
        //     last_debug_time = millis( );

        //     JULSEDEBUG_DIG( "Capture loop - samples: %d/%d, loops: %d\n\r", scnt, num_samples, loop_count );
        //     JULSEDEBUG_CMD( "  Current DMA half: %d\n\r", current_dma_half );
        //     JULSEDEBUG_CMD( "  Trigger monitoring: enabled=%d, detected=%d\n\r", trigger_config.enabled, trigger_detected );
        //     JULSEDEBUG_CMD( "  PIO FIFO Level: %d\n\r", pio_sm_get_rx_fifo_level( pio, piosm ) );
        //     // JULSEDEBUG_CMD( "  DMA-A0 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( admachan0 ) ? "Y" : "N", dma_channel_hw_addr( admachan0 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-A1 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( admachan1 ) ? "Y" : "N", dma_channel_hw_addr( admachan1 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-D0 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( pdmachan0 ) ? "Y" : "N", dma_channel_hw_addr( pdmachan0 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-D1 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( pdmachan1 ) ? "Y" : "N", dma_channel_hw_addr( pdmachan1 )->write_addr );
        // }
    }

    // Send final completion signal if not already sent
    if ( scnt >= num_samples ) {
        JULSEDEBUG_CMD( "Capture complete - sending final completion signal\n\r" );
        JULSEDEBUG_CMD( "Final sample count: %d/%d, total bytes: %d\n\r", scnt, num_samples, scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ) );

        // CRITICAL: Stop DMA before sending completion signal
        dma_end( );

        char completion_str[ 32 ];
        int total_bytes = scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ); // Total bytes sent
        sprintf( completion_str, "$%d+", total_bytes );
        delayMicroseconds( 1000 );
        this->julseview_usb_out_chars( completion_str, strlen( completion_str ) );
        JULSEDEBUG_CMD( "Final completion signal sent: %s\n\r", completion_str );
    }

    // CRITICAL SAFETY: Add delay before deinit to ensure USB transmission completes
    delayMicroseconds( 100000 ); // 1ms delay to ensure USB transmission completes

    deinit( );

    JULSEDEBUG_STA( "=== JULSEVIEW RUN() COMPLETE ===\n\r" );
}

void julseview::dma_check( ) {
    //  Implement sigrok-pico DMA chaining approach
    // This prevents infinite loops and ensures proper synchronization

    // Determine which channels to check based on current half
    uint admachan_current = ( current_dma_half == 0 ) ? admachan0 : admachan1;
    uint pdmachan_current = ( current_dma_half == 0 ) ? pdmachan0 : pdmachan1;

    // Check if current DMA half has completed (both analog and digital)
    bool analog_complete = ( a_chan_cnt == 0 ) || !dma_channel_is_busy( admachan_current );
    bool digital_complete = ( d_chan_cnt == 0 ) || !dma_channel_is_busy( pdmachan_current );

    if ( analog_complete && digital_complete ) {
        // Debug: Log DMA completion
        static uint32_t dma_completion_count = 0;
        dma_completion_count++;
        // if (dma_completion_count % 100 == 0) {  // Log every 100th completion
        //     JULSEDEBUG_CMD("DMA completion #%d - half: %d\n\r", dma_completion_count, current_dma_half);
        // }

        // Sample limit checks removed - let DMA run until completion

        // A half-transfer is complete - process the data
        // JULSEDEBUG_DMA("DMA half-transfer complete. Half: %d\n\r", current_dma_half);

        // ULTRA-FAST ATOMIC BUFFER SWITCH - Minimize timing gap to prevent ADC round-robin misalignment
        uint8_t completed_half = current_dma_half; // Save before switching
        current_dma_half = 1 - current_dma_half;   // Switch to next half immediately

        // Pre-compute everything to minimize critical section timing
        volatile uint32_t* completed_sts_a = ( completed_half == 0 ) ? tstsa0 : tstsa1;
        volatile uint32_t* completed_sts_d = ( completed_half == 0 ) ? tstsd0 : tstsd1;
        volatile uint32_t* completed_addr_a = ( completed_half == 0 ) ? taddra0 : taddra1;
        volatile uint32_t* completed_addr_d = ( completed_half == 0 ) ? taddrd0 : taddrd1;
        uint8_t self_chain_a = ( completed_half == 0 ) ? admachan0 : admachan1;
        uint8_t self_chain_d = ( completed_half == 0 ) ? pdmachan0 : pdmachan1;
        uint32_t reset_addr_a = (uint32_t)&capture_buf[ ( completed_half == 0 ) ? abuf0_start : abuf1_start ];
        uint32_t reset_addr_d = (uint32_t)&capture_buf[ ( completed_half == 0 ) ? dbuf0_start : dbuf1_start ];

        // ULTRA-ATOMIC SECTION: Just reset addresses, don't break the chain!
        // Let the RP2350B's automatic chaining handle everything
        if ( a_chan_cnt > 0 ) {
            // Just reset the address - don't touch the chain
            *completed_addr_a = reset_addr_a;
            // JULSEDEBUG_DMA("ULTRA-ATOMIC: DMA-A%d addr=0x%08X (chain preserved)\n\r", (completed_half == 0) ? 0 : 1, reset_addr_a);
        }
        if ( d_chan_cnt > 0 ) {
            // Just reset the address - don't touch the chain
            *completed_addr_d = reset_addr_d;
            // JULSEDEBUG_DMA("ULTRA-ATOMIC: DMA-D%d addr=0x%08X (chain preserved)\n\r", (completed_half == 0) ? 0 : 1, reset_addr_d);
        }

        // NO chain manipulation - let automatic chaining work!
        // The RP2350B's built-in chaining will handle the ping-pong automatically

        // Step 2: Process data from completed half (can take time now)
        uint8_t* dbuf = &capture_buf[ completed_half == 0 ? dbuf0_start : dbuf1_start ];
        uint8_t* abuf = &capture_buf[ completed_half == 0 ? abuf0_start : abuf1_start ];

        // Trigger detection is now handled by direct GPIO polling in the main loop
        // No need to check triggers here anymore - just process data normally

        unsigned long start_time = micros( );
        send_slices_analog( dbuf, abuf );
        unsigned long end_time = micros( );
        // JULSEDEBUG_DMA("FAST: Time taken to send slices: %d us\n\r", end_time - start_time);

        // Note: Overflow detection already handled above in the chain re-establishment logic

        //  Don't update scnt here - let send_slices_analog handle all sample counting
        // This prevents double-counting in the completion logic
        // JULSEDEBUG_DAT("DEBUG: Processing buffer with %d samples, current scnt=%d/%d\n\r", samples_per_half, scnt, num_samples);

        // JULSEDEBUG_CMD("Switched to half: %d, DMA chaining active\n\r", current_dma_half);
    }
}

void julseview::dma_end( ) {
    // CRITICAL: Break ping-pong chaining and stop DMA channels cleanly
    // This prevents lingering DMA activity that could cause Core 2 crashes
    
    // SAFETY: Prevent multiple calls to dma_end() which can cause hangs
    if (dma_ended_flag) {
        JULSEDEBUG_DMA( "DMA_END: Already called, skipping to prevent hang\n\r" );
        return;
    }
    dma_ended_flag = true;

    JULSEDEBUG_DMA( "DMA_END: Breaking ping-pong chaining and stopping DMA channels\n\r" );

    // Step 1: Break the chaining by setting channels to chain to themselves (no-op)
    if ( a_chan_cnt > 0 ) {
        // Break analog DMA chaining
        channel_config_set_chain_to( &acfg0, admachan0 ); // Chain to self (no-op)
        channel_config_set_chain_to( &acfg1, admachan1 ); // Chain to self (no-op)

        // Reconfigure channels to break the chain
        dma_channel_configure( admachan0, &acfg0, NULL, NULL, 0, false );
        dma_channel_configure( admachan1, &acfg1, NULL, NULL, 0, false );

        JULSEDEBUG_DMA( "DMA_END: Analog DMA chaining broken\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        // Break digital DMA chaining
        channel_config_set_chain_to( &pcfg0, pdmachan0 ); // Chain to self (no-op)
        channel_config_set_chain_to( &pcfg1, pdmachan1 ); // Chain to self (no-op)

        // Reconfigure channels to break the chain
        dma_channel_configure( pdmachan0, &pcfg0, NULL, NULL, 0, false );
        dma_channel_configure( pdmachan1, &pcfg1, NULL, NULL, 0, false );

        JULSEDEBUG_DMA( "DMA_END: Digital DMA chaining broken\n\r" );
    }

    // Step 2: Reset write addresses to beginning of buffers
    if ( a_chan_cnt > 0 ) {
        // Reset analog DMA write addresses
        *taddra0 = (uint32_t)&capture_buf[ abuf0_start ];
        *taddra1 = (uint32_t)&capture_buf[ abuf1_start ];
        JULSEDEBUG_DMA( "DMA_END: Analog DMA addresses reset to buffer start\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        // Reset digital DMA write addresses
        *taddrd0 = (uint32_t)&capture_buf[ dbuf0_start ];
        *taddrd1 = (uint32_t)&capture_buf[ dbuf1_start ];
        JULSEDEBUG_DMA( "DMA_END: Digital DMA addresses reset to buffer start\n\r" );
    }

    // Step 3: Stop all DMA channels
    if ( a_chan_cnt > 0 ) {
        dma_channel_abort( admachan0 );
        dma_channel_abort( admachan1 );
        JULSEDEBUG_DMA( "DMA_END: Analog DMA channels stopped\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        dma_channel_abort( pdmachan0 );
        dma_channel_abort( pdmachan1 );
        JULSEDEBUG_DMA( "DMA_END: Digital DMA channels stopped\n\r" );
    }

    // Step 4: Wait for channels to actually stop (brief wait)
    uint32_t timeout = 1000; // 1ms timeout
    while ( timeout-- > 0 ) {
        bool all_stopped = true;

        if ( a_chan_cnt > 0 ) {
            if ( dma_channel_is_busy( admachan0 ) || dma_channel_is_busy( admachan1 ) ) {
                all_stopped = false;
            }
        }

        if ( d_chan_cnt > 0 ) {
            if ( dma_channel_is_busy( pdmachan0 ) || dma_channel_is_busy( pdmachan1 ) ) {
                all_stopped = false;
            }
        }

        if ( all_stopped ) {
            break;
        }

        delayMicroseconds( 1 );
    }

    if ( timeout == 0 ) {
        JULSEDEBUG_ERR( "DMA_END: WARNING - DMA channels did not stop within timeout\n\r" );
    } else {
        JULSEDEBUG_DMA( "DMA_END: All DMA channels stopped successfully\n\r" );
    }

    // Step 5: Reset current DMA half to 0 for next capture
    current_dma_half = 0;

    JULSEDEBUG_DMA( "DMA_END: Ping-pong chaining broken, addresses reset, channels stopped\n\r" );
}

void julseview::deinit( ) {
    JULSEDEBUG_STA( "JulseView deinit() - cleaning up resources\n\r" );

    // CRITICAL SAFETY: Set active flag to false immediately to prevent new operations
    julseview_active = false;

    // CRITICAL: Use dma_end() for proper ping-pong chaining cleanup
    JULSEDEBUG_STA( "Proceeding with immediate cleanup\n\r" );

    // Stop ADC first
    adc_run( false );

    // CRITICAL: Use dma_end() to properly break ping-pong chaining and stop DMA
    dma_end( );

    // CRITICAL: Clean up PIO resources to prevent conflicts with CH446Q
    if ( pio != nullptr && d_chan_cnt > 0 ) {
        JULSEDEBUG_STA( "Cleaning up PIO1 resources...\n\r" );

        // Stop the PIO state machine first
        pio_sm_set_enabled( pio, piosm, false );
        pio_sm_clear_fifos( pio, piosm );

        // Remove the PIO program
        pio_remove_program( pio, &pio_logic_analyzer_program, 0 );
        JULSEDEBUG_STA( "PIO program removed from PIO1\n\r" );
    }

    // CRITICAL: Clean up DMA channels to prevent resource conflicts
    cleanup_dma_channels( );

    // Reset state flags
    sending = false;
    started = false;
    armed = false; // Device is no longer armed
    julseview_active = false;

    // Reset enhanced state tracking
    isArmed = false;
    isRunning = false;
    isTriggered = false;
    receivedCommand = false;
    JULSEDEBUG_STA( "Device state reset: all flags cleared\n\r" );

    // Reset trigger state
    trigger_armed = false;
    trigger_detected = false;
    trigger_config.enabled = false;

    //  Reset ping-pong buffer state to ensure consistent behavior
    current_dma_half = 0; // Always start with buffer 0

    // CRITICAL: Add stabilization delay for multicore synchronization
    // JulseView's intensive DMA operations can destabilize core coordination
    // delayMicroseconds(1000);  // 1ms delay to allow system stabilization
    // __dmb();  // Data Memory Barrier
    // __isb();  // Instruction Synchronization Barrier

    // //  Clear capture buffers to prevent stale data (with bounds checking)
    // if (capture_buf != nullptr && actual_buffer_size > 0 && actual_buffer_size <= (JULSEVIEW_DMA_BUF_SIZE + 1024)) {
    //     memset(capture_buf, 0, actual_buffer_size);
    //     JULSEDEBUG_BUF("Capture buffers cleared (%d bytes)\n\r", actual_buffer_size);

    //     // CRITICAL: Check memory guard for buffer overflow
    //     if (raw_capture_buf != nullptr) {
    //         uint32_t guard_value;
    //         memcpy(&guard_value, raw_capture_buf + actual_buffer_size + 16 - 4, 4);
    //         if (guard_value != 0xDEADBEEF) {
    //             JULSEDEBUG_ERR("ERROR: Memory guard corrupted! Expected 0xDEADBEEF, got 0x%08X\n\r", guard_value);
    //         } else {
    //             JULSEDEBUG_BUF("Memory guard intact - no buffer overflow detected\n\r");
    //         }
    //     }
    // } else if (capture_buf != nullptr) {
    //     JULSEDEBUG_ERR("ERROR: Invalid buffer size %d during cleanup (max allowed: %d)\n\r", actual_buffer_size, JULSEVIEW_DMA_BUF_SIZE + 1024);
    // }

    // Reset coordinator state
    coordinator_switch_ready = false;
    buffer0_active = true;

    // Reset producer-consumer state
    buffer0_capturing = true;
    buffer1_capturing = false;
    buffer0_ready = false;
    buffer1_ready = false;
    buffer0_transmitting = false;
    buffer1_transmitting = false;

    // CRITICAL: Invalidate DMA register pointers to prevent continued writes
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    // // CRITICAL: Keep buffers allocated but clear them to prevent stale data
    // if (capture_buf != nullptr) {
    //     // Clear buffers but keep them allocated for reuse
    //     memset(capture_buf, 0, actual_buffer_size);
    //     JULSEDEBUG_BUF("Buffers cleared but kept allocated for reuse (%d bytes)\n\r", actual_buffer_size);
    // }

    // Restore ADC to normal operation for other functions
    JULSEDEBUG_ANA( "Restoring ADC to normal operation\n\r" );

    // Re-initialize ADC to restore normal operation
    adc_init( );

    // Set Arduino ADC resolution back to 12 bits for compatibility
    analogReadResolution( 12 );
    adc_set_clkdiv( 1.0 );
    adc_set_round_robin( 0 );
    adc_run( false );

    // Re-enable ADC GPIO pins for normal operation
    for ( int i = 0; i < 8; i++ ) {
        // if (a_mask & (1 << i)) {
        // JULSEDEBUG_ANA("Restoring ADC GPIO pin %d reading %d\n\r", 40 + i, analogRead(40 + i));
        adc_gpio_init( 40 + i ); // Jumperless ADCs on pins 40-47
                                 // JULSEDEBUG_ANA("Restored ADC GPIO pin %d\n\r", 40 + i);
        //}
    }

    JULSEDEBUG_STA( "JulseView deinit() complete - ADC restored\n\r" );

    // CRITICAL: Restore bus priority to normal to prevent interference with CH446Q PIO operations
    bus_ctrl_hw->priority = 0; // Reset to default priority (CPU priority)
    JULSEDEBUG_STA( "Bus priority restored to normal\n\r" );

    // CRITICAL: Skip final verification to avoid blocking calls
    JULSEDEBUG_STA( "Cleanup completed - skipping verification\n\r" );

    // CRITICAL: Add post-deinit watchdog to detect and handle any remaining DMA activity
    static uint32_t deinit_watchdog_time = 0;
    deinit_watchdog_time = millis( );

    julseview_active = false;

    // Schedule a check in 5 seconds to ensure DMA is truly stopped
    // This will be handled in the main loop
}

bool julseview::process_char( char charin ) {
    // CRITICAL SAFETY: Add bounds checking for command buffer
    if ( cmdstrptr >= sizeof( cmdstr ) - 1 ) {
        JULSEDEBUG_ERR( "ERROR: Command buffer overflow, resetting\n\r" );
        cmdstrptr = 0;
        return false;
    }

    int tmpint, tmpint2;
    bool needs_response = false;

    rspstr[ 0 ] = '*';
    rspstr[ 1 ] = 0;

    if ( charin == '*' ) {
        reset( );
        return false;
    } else if ( charin == '+' ) {
        JULSEDEBUG_CMD( "JulseView + command received\n\r" );
        julseview_active = false;

        // Update enhanced state tracking
        isArmed = false;
        isRunning = false;
        isTriggered = false;
        receivedCommand = false;

        deinit( );
        return false;
    } else if ( ( charin == '\r' ) || ( charin == '\n' ) ) {
        cmdstr[ cmdstrptr ] = 0;
        switch ( cmdstr[ 0 ] ) {
        case 'i':
            if ( !initialized ) {
                julseview::init();
            }
            sprintf( rspstr, "SRJLV5,A%02d2D%02d,02", JULSEVIEW_DEFAULT_ANALOG_CHANNELS, JULSEVIEW_DEFAULT_DIGITAL_CHANNELS );
            JULSEDEBUG_CMD( "JulseView ID response: %s\n\r", rspstr );
            needs_response = true;
            break;
        case 'R':
            tmpint = atol( &cmdstr[ 1 ] );
            JULSEDEBUG_CMD( "DEBUG: R command received - tmpint=%d, current sample_rate=%d\n\r", tmpint, sample_rate );
            if ( tmpint > 0 ) {
                sample_rate = tmpint;
                JULSEDEBUG_CMD( "DEBUG: sample_rate set to %d (trigger_enabled=%d, pre_trigger=%d)\n\r", sample_rate, trigger_config.enabled, pre_trigger_samples );
                
                // Configure decimation mode and send factor to driver
                configure_decimation_mode();
                
                // Send decimation factor in response: "*<decimation_factor>"
                sprintf( rspstr, "*%d", analog_decimation_factor );
                JULSEDEBUG_CMD( "DEBUG: Sending decimation factor %d to driver\n\r", analog_decimation_factor );
                
                needs_response = true;
            } else {
                JULSEDEBUG_ERR( "ERROR: R command with invalid sample rate %d\n\r", tmpint );
            }
            break;
        case 'L':
            tmpint = atol( &cmdstr[ 1 ] );
            if ( tmpint > 0 ) {
                num_samples = tmpint;
                needs_response = true;
            }
            break;
        case 'a':
            // Handle both 'a' and 'a0', 'a1', etc. commands
            if ( cmdstrptr == 1 ) {
                // Simple 'a' command
                sprintf( rspstr, "805x0" );
                JULSEDEBUG_CMD( "JulseView a command -> 805x0\n\r" );
            } else {
                // Numbered analog channel query 'a0', 'a1', etc.
                tmpint = atoi( &cmdstr[ 1 ] );
                if ( tmpint >= 0 && tmpint < JULSEVIEW_MAX_ANALOG_CHANNELS ) {
                    // Use calibrated scaling and offset
                    uint32_t scale_microvolts;
                    int32_t offset_microvolts;
                    get_calibrated_analog_scaling( tmpint, &scale_microvolts, &offset_microvolts );

                    sprintf( rspstr, "%dx%d", scale_microvolts, offset_microvolts );
                    JULSEDEBUG_CMD( "JulseView a%d command -> %s (calibrated)\n\r", tmpint, rspstr );
                } else {
                    JULSEDEBUG_ERR( "JulseView invalid analog channel: a%d\n\r", tmpint );
                    sprintf( rspstr, "ERR" );
                }
            }
            needs_response = true;
            break;
        case 'F':
            cont = false;
            arm( );
            run( ); // Start capture immediately after arming
            needs_response = false;
            break;
        case 'C':
            // Check if this is a control channel command (C<enable><channel>) or continuous mode (C)
            if ( cmdstrptr >= 3 ) {
                // Control channel individual control: C<enable><channel>
                tmpint = cmdstr[ 1 ] - '0';     // Enable/disable flag
                tmpint2 = atoi( &cmdstr[ 2 ] ); // Channel number (can be 2 digits)
                JULSEDEBUG_CMD( "JulseView C command: %d%d\n\r", tmpint, tmpint2 );

                // Accept any channel number (0-15) for control channels
                if ( ( tmpint >= 0 ) && ( tmpint <= 1 ) && ( tmpint2 >= 0 ) && ( tmpint2 < 16 ) ) {
                    if ( tmpint ) {
                        c_mask |= ( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> ENABLE control channel %d\n\r", tmpint2 );
                    } else {
                        c_mask &= ~( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> DISABLE control channel %d\n\r", tmpint2 );
                    }

                    // Update control channel count based on highest enabled channel
                    c_chan_cnt = 0;
                    for ( int i = 15; i >= 0; i-- ) {
                        if ( c_mask & ( 1 << i ) ) {
                            c_chan_cnt = i + 1;
                            break;
                        }
                    }

                    needs_response = true;
                } else {
                    JULSEDEBUG_CMD( " -> IGNORED (invalid parameters)\n\r" );
                }
            } else {
                // Continuous mode command (single 'C')
                cont = true;
                arm( );

                run( ); // Start capture immediately after arming
                needs_response = false;
            }
            break;
        case 'A':
            //  Handle all possible analog channel commands (A00-A31)
            if ( cmdstrptr >= 3 ) {
                tmpint = cmdstr[ 1 ] - '0';     // Enable/disable flag
                tmpint2 = atoi( &cmdstr[ 2 ] ); // Channel number (can be 2 digits)
                JULSEDEBUG_CMD( "JulseView A command: %d%d\n\r", tmpint, tmpint2 );

                // CRITICAL: Accept any channel number (0-31) for compatibility
                if ( ( tmpint >= 0 ) && ( tmpint <= 1 ) && ( tmpint2 >= 0 ) && ( tmpint2 < 32 ) ) {
                    if ( tmpint ) {
                        a_mask |= ( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> ENABLE channel %d\n\r", tmpint2 );
                    } else {
                        a_mask &= ~( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> DISABLE channel %d\n\r", tmpint2 );
                    }
                    needs_response = true;
                } else {
                    JULSEDEBUG_CMD( " -> IGNORED (invalid parameters)\n\r" );
                }
            } else {
                JULSEDEBUG_CMD( " -> IGNORED (incomplete command)\n\r" );
            }
            break;
        case 'D':
            //  Handle all possible digital channel commands (D00-D31)
            if ( cmdstrptr >= 3 ) {
                tmpint = cmdstr[ 1 ] - '0';     // Enable/disable flag
                tmpint2 = atoi( &cmdstr[ 2 ] ); // Channel number (can be 2 digits)
                JULSEDEBUG_CMD( "JulseView D command: %d%d\n\r", tmpint, tmpint2 );

                // CRITICAL: Accept any channel number (0-31) for compatibility
                if ( ( tmpint >= 0 ) && ( tmpint <= 1 ) && ( tmpint2 >= 0 ) && ( tmpint2 < 32 ) ) {
                    if ( tmpint ) {
                        d_mask |= ( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> ENABLE channel %d\n\r", tmpint2 );
                    } else {
                        d_mask &= ~( 1 << tmpint2 );
                        JULSEDEBUG_CMD( " -> DISABLE channel %d\n\r", tmpint2 );
                    }

                    // Note: Digital mask continuity check removed from driver - allow any mask pattern

                    needs_response = true;
                } else {
                    JULSEDEBUG_CMD( " -> IGNORED (invalid parameters)\n\r" );
                }
            } else {
                JULSEDEBUG_CMD( " -> IGNORED (incomplete command)\n\r" );
            }
            break;

        case 'T':
            // Trigger command - start capture if already armed
            if ( started == false && sending == false ) {
                // Check if device is armed
                if ( armed && julseview_active && ( a_chan_cnt > 0 || d_chan_cnt > 0 ) ) {
                    JULSEDEBUG_CMD( "Trigger received - starting capture\n\r" );
                    run( ); // Start capture
                    needs_response = false;
                } else {
                    JULSEDEBUG_ERR( "ERROR: Cannot trigger - device not armed\n\r" );
                    sprintf( rspstr, "ERR" );
                    needs_response = true;
                }
            } else {
                JULSEDEBUG_ERR( "ERROR: Cannot trigger - capture already active\n\r" );
                sprintf( rspstr, "ERR" );
                needs_response = true;
            }
            break;
        case 'S':
            // Status command - return current state
            sprintf( rspstr, "A%dS%dR%dT%d",
                     armed ? 1 : 0,                    // Armed state
                     started ? 1 : 0,                  // Started state
                     sending ? 1 : 0,                  // Sending state
                     trigger_config.enabled ? 1 : 0 ); // Trigger enabled
            JULSEDEBUG_CMD( "Status: armed=%d, started=%d, sending=%d, trigger=%d\n\r",
                            armed ? 1 : 0, started ? 1 : 0, sending ? 1 : 0, trigger_config.enabled ? 1 : 0 );
            needs_response = true;
            break;
        case 't':
            // Trigger configuration command: t<type><channel><level><edge>
            // Examples: t10020480 (analog level, channel 0, level 2048, rising edge)
            //           t20001 (digital edge, channel 0, rising edge)
            //           t3000000000000000001 (internal var, address 0, value 1)
            if ( cmdstrptr >= 4 ) {
                uint8_t trigger_type = cmdstr[ 1 ] - '0';
                uint8_t channel = 0;
                uint16_t level = 0;
                edge_type_t edge = EDGE_RISING;

                JULSEDEBUG_CMD( "Trigger command parsing: type=%d, channel=%d, cmdstr='%s', len=%d\n\r",
                                trigger_type, channel, cmdstr, cmdstrptr );

                // Debug: show all characters in the command
                JULSEDEBUG_CMD( "Command breakdown: " );
                for ( int i = 0; i < cmdstrptr; i++ ) {
                    JULSEDEBUG_CMD( "'%c'(%d) ", cmdstr[ i ], cmdstr[ i ] - '0' );
                }
                JULSEDEBUG_CMD( "\n\r" );

                if ( trigger_type == 1 ) { // Analog level trigger
                    if ( cmdstrptr >= 8 ) {
                        level = atoi( &cmdstr[ 3 ] );
                        uint8_t libsigrok_edge = cmdstr[ 7 ] - '0';
                        JULSEDEBUG_CMD( "Analog trigger: level=%d, libsigrok_edge=%d\n\r", level, libsigrok_edge );

                        // Convert driver trigger values to our edge types
                        // Driver maps: SR_TRIGGER_RISING(3)->2, SR_TRIGGER_FALLING(4)->3, SR_TRIGGER_EDGE(5)->4
                        switch ( libsigrok_edge ) {
                        case 2: // SR_TRIGGER_RISING (mapped from 3)
                            edge = EDGE_RISING;
                            break;
                        case 3: // SR_TRIGGER_FALLING (mapped from 4)
                            edge = EDGE_FALLING;
                            break;
                        case 4: // SR_TRIGGER_EDGE (mapped from 5)
                            edge = EDGE_EITHER;
                            break;
                        case 6:                 // SR_TRIGGER_OVER (over threshold)
                            edge = EDGE_RISING; // Treat as rising edge over threshold
                            break;
                        case 7:                  // SR_TRIGGER_UNDER (under threshold)
                            edge = EDGE_FALLING; // Treat as falling edge under threshold
                            break;
                        default:
                            JULSEDEBUG_ERR( "Invalid driver analog edge type: %d\n\r", libsigrok_edge );
                            break;
                        }

                        if ( edge <= EDGE_EITHER ) {
                            configure_analog_trigger( channel, level, edge );
                            needs_response = true;
                        }
                    } else {
                        JULSEDEBUG_ERR( "Analog trigger command too short: %d chars\n\r", cmdstrptr );
                    }
                } else if ( trigger_type == 2 || trigger_type == 3 || trigger_type == 4 || trigger_type == 5 ) { // Digital edge trigger
                    if ( cmdstrptr >= 4 ) {
                        // Format is t<val><idx+2> where val=trigger_type, idx+2=channel_index+2
                        // Parse the last two digits as channel_index+2
                        uint8_t channel_idx_plus_2 = atoi( &cmdstr[ 2 ] );
                        uint8_t actual_channel = channel_idx_plus_2 - 2; // Convert back to actual channel index

                        JULSEDEBUG_CMD( "Digital trigger: channel_idx_plus_2=%d, actual_channel=%d\n\r", channel_idx_plus_2, actual_channel );

                        // The trigger type is already the edge type (2=rising, 3=falling, 4=edge, 5=either)
                        switch ( trigger_type ) {
                        case 2: // SR_TRIGGER_RISING (mapped from 3)
                            edge = EDGE_RISING;
                            break;
                        case 3: // SR_TRIGGER_FALLING (mapped from 4)
                            edge = EDGE_FALLING;
                            break;
                        case 4: // SR_TRIGGER_EDGE (mapped from 5)
                            edge = EDGE_EITHER;
                            break;
                        case 5: // SR_TRIGGER_EDGE (direct)
                            edge = EDGE_EITHER;
                            break;
                        default:
                            JULSEDEBUG_ERR( "Invalid driver edge type: %d (valid: 2=rising, 3=falling, 4=edge, 5=either)\n\r", trigger_type );
                            break;
                        }

                        if ( edge <= EDGE_EITHER ) {
                            configure_digital_trigger( actual_channel, edge );
                            needs_response = true;
                        }
                    } else {
                        JULSEDEBUG_ERR( "Digital trigger command too short: %d chars\n\r", cmdstrptr );
                    }
                } else if ( trigger_type == 6 ) { // Libsigrok SR_TRIGGER_OVER (analog over threshold)
                    // Format: t6<channel><level>
                    if ( cmdstrptr >= 6 ) {
                        level = atoi( &cmdstr[ 3 ] );
                        JULSEDEBUG_CMD( "Libsigrok OVER trigger: channel=%d, level=%d\n\r", channel, level );
                        configure_analog_trigger( channel, level, EDGE_RISING );
                        needs_response = true;
                    } else {
                        JULSEDEBUG_ERR( "Libsigrok OVER trigger command too short: %d chars\n\r", cmdstrptr );
                    }
                } else if ( trigger_type == 7 ) { // Libsigrok SR_TRIGGER_UNDER (analog under threshold)
                    // Format: t7<channel><level>
                    if ( cmdstrptr >= 6 ) {
                        level = atoi( &cmdstr[ 3 ] );
                        JULSEDEBUG_CMD( "Libsigrok UNDER trigger: channel=%d, level=%d\n\r", channel, level );
                        configure_analog_trigger( channel, level, EDGE_FALLING );
                        needs_response = true;
                    } else {
                        JULSEDEBUG_ERR( "Libsigrok UNDER trigger command too short: %d chars\n\r", cmdstrptr );
                    }
                } else {
                    JULSEDEBUG_ERR( "Invalid trigger type: %d\n\r", trigger_type );
                }
            } else {
                JULSEDEBUG_ERR( "Trigger command too short: %d chars\n\r", cmdstrptr );
            }
            break;
        case 'd':
            // Disable trigger command
            trigger_config.enabled = false;
            trigger_armed = false;
            trigger_detected = false;
            JULSEDEBUG_CMD( "Trigger disabled\n\r" );
            needs_response = true;
            break;
        case 'p':
            // Pre-trigger samples command: p<pre> - force to 0 for now
            if ( cmdstrptr >= 2 ) {
                int requested_pre = atoi( &cmdstr[ 1 ] );
                pre_trigger_samples = 0; // Force pre-trigger to 0
                // Set post-trigger samples to a reasonable default if not specified
                if ( post_trigger_samples == 0 ) {
                    post_trigger_samples = 10000; // Default 10k post-trigger samples
                }
                JULSEDEBUG_CMD( "Pre-trigger command: requested=%d, forced=0 (post: %d)\n\r", requested_pre, post_trigger_samples );
                needs_response = true;
            }
            break;
        case 'E':
            // Control channel enable command: E<n> where n is number of control channels (0-16)
            tmpint = atoi( &cmdstr[ 1 ] );
            JULSEDEBUG_CMD( "JulseView E command: enable %d control channels\n\r", tmpint );
            if ( tmpint >= 0 && tmpint <= 16 ) {
                c_chan_cnt = tmpint;                                   // Set number of control channels
                c_mask = ( tmpint > 0 ) ? ( ( 1 << tmpint ) - 1 ) : 0; // Create mask for enabled channels
                JULSEDEBUG_CMD( " -> Control channels enabled: %d (mask: 0x%02X)\n\r", c_chan_cnt, c_mask );
                needs_response = true;
            } else {
                JULSEDEBUG_CMD( " -> IGNORED (invalid number of channels: %d)\n\r", tmpint );
            }
            break;
        }
        cmdstrptr = 0;
    } else {
        if ( cmdstrptr < 19 ) {
            cmdstr[ cmdstrptr++ ] = charin;
        } else {
            cmdstrptr = 0;
        }
    }
    return needs_response;
}

// REMOVED: check_half function - no longer used with simplified DMA modes

// Helper function to send digital sample with 7-bit encoding
void julseview::tx_d_samp( uint32_t cval ) {

    for ( char b = 0; b < d_tx_bps; b++ ) {
        uint8_t data_byte = ( cval & 0x7F ) + 0x30; // Ensure >= 0x30
        if ( data_byte < 0x80 )
            data_byte |= 0x80; // Set high bit for digital data
        txbuf[ txbufidx++ ] = data_byte;
        cval >>= 7;
    }
}

// Get current value from buffer based on actual channel count
uint32_t julseview::get_cval( uint8_t* dbuf ) {
    uint32_t cval = 0;

    // Read based on how many bytes we actually stored (d_dma_bps)
    if ( d_dma_bps == 1 ) {
        cval = dbuf[ rxbufdidx ];
    } else if ( d_dma_bps == 2 ) {
        cval = ( *( (uint16_t*)( dbuf + rxbufdidx ) ) );
    } else if ( d_dma_bps == 3 ) {
        cval = dbuf[ rxbufdidx ] | ( dbuf[ rxbufdidx + 1 ] << 8 ) | ( dbuf[ rxbufdidx + 2 ] << 16 );
    } else {
        cval = ( *( (uint32_t*)( dbuf + rxbufdidx ) ) );
    }

    // Mask to keep only the enabled channels
    uint32_t channel_mask = ( 1 << d_chan_cnt ) - 1;
    cval &= channel_mask;

    // Only show debug for first few samples to avoid spam
    // if ((rxbufdidx / d_dma_bps) < 5) {
    //     JULSEDEBUG_CMD("DEBUG: get_cval[%d] = 0x%08X\n\r", rxbufdidx / d_dma_bps, cval);
    //     JULSEDEBUG_CMD(" from addr 0x%08X\n\r", (uint32_t)(dbuf + rxbufdidx));
    // }

    rxbufdidx += d_dma_bps; // Advance by actual bytes per sample
    return cval;
}

// RLE encoding for 5-21 channels
void julseview::check_rle( ) {
    while ( rlecnt >= 1568 ) {
        txbuf[ txbufidx++ ] = 127;
        rlecnt -= 1568;
    }
    if ( rlecnt > 32 ) {
        uint16_t rlediv = rlecnt >> 5;
        txbuf[ txbufidx++ ] = rlediv + 78;
        rlecnt -= rlediv << 5;
    }
    if ( rlecnt ) {
        txbuf[ txbufidx++ ] = 47 + rlecnt;
        rlecnt = 0;
    }
}

// Send txbuf to USB based on threshold
void julseview::check_tx_buf( uint16_t cnt ) {
    if ( txbufidx >= cnt ) {
        // Check if we have a significant amount of data to transmit
        if ( txbufidx > 0 ) {
            // Transmit the buffer with error checking
            bool success = this->julseview_usb_out_chars( (char*)txbuf, txbufidx );
            if ( success ) {
                byte_cnt += txbufidx;
                // Reset buffer index only if transmission succeeded
                txbufidx = 0;
            } else {
                // Keep data in buffer for retry - no debug output for performance
            }
        }
    }
}

// Common init for send_slices_1B/2B/4B
void julseview::send_slice_init( uint8_t* dbuf ) {
    rxbufdidx = 0;
    //  Don't update scnt here - update it after processing
    samp_remain = samples_per_half;
    if ( !cont && ( scnt + samp_remain > num_samples ) ) {
        samp_remain = num_samples - scnt;
    }
    txbufidx = 0;
    // Send first sample to establish previous value for RLE
    lval = get_cval( dbuf );
    // Mask invalid bits if in 4B mode
    lval <<= 11;
    lval >>= 11;
    tx_d_samp( lval );
    samp_remain--;
    rlecnt = 0;

    //  Update scnt with actual samples processed
    scnt += samp_remain + 1; // +1 for the first sample we just sent
    JULSEDEBUG_DAT( "DEBUG: Sent %d samples, total scnt=%d/%d\n\r", samp_remain + 1, scnt, num_samples );
}

// Send slices for 4-bit nibble data (special D4 encoding)
void julseview::send_slices_D4( uint8_t* dbuf ) {
    uint8_t nibcurr, niblast;
    uint32_t cword, lword;
    uint32_t* cptr;

    txbufidx = 0;
    cptr = (uint32_t*)&( dbuf[ 0 ] );
    cword = *cptr;
    lword = cword;

    // Send first 8 samples directly
    for ( int j = 0; j < 8; j++ ) {
        nibcurr = cword & 0xF;
        txbuf[ j ] = ( nibcurr ) | 0x80;
        cword >>= 4;
    }
    niblast = nibcurr;
    txbufidx = 8;
    rxbufdidx = 4;
    rlecnt = 0;

    if ( samples_per_half <= 8 ) {
        this->julseview_usb_out_chars( (char*)txbuf, txbufidx );
        //  Update scnt after processing
        scnt += samples_per_half;
        JULSEDEBUG_DAT( "DEBUG: Sent %d samples, total scnt=%d/%d\n\r", samples_per_half, scnt, num_samples );
        return;
    }

    samp_remain = samples_per_half - 8;
    if ( !cont && ( scnt + samp_remain > num_samples ) ) {
        samp_remain = num_samples - scnt;
        JULSEDEBUG_CMD( "DEBUG: Limiting samp_remain to %d due to sample count limit\n\r", samp_remain );
    }

    // Process remaining words
    for ( int i = 0; i < ( samp_remain >> 3 ); i++ ) {
        cptr = (uint32_t*)&( dbuf[ rxbufdidx ] );
        cword = *cptr;

        for ( int j = 0; j < 8; j++ ) {
            nibcurr = cword & 0xF;
            if ( nibcurr == niblast ) {
                rlecnt++;
            } else {
                // Send RLE if any
                while ( rlecnt >= 640 ) {
                    txbuf[ txbufidx++ ] = 127;
                    rlecnt -= 640;
                }
                if ( rlecnt > 7 ) {
                    int rleend = rlecnt & 0x3F8;
                    txbuf[ txbufidx++ ] = ( rleend >> 3 ) + 47;
                }
                if ( rlecnt ) {
                    rlecnt &= 0x7;
                    rlecnt--;
                    txbuf[ txbufidx++ ] = 0x80 | nibcurr | rlecnt << 4;
                    rlecnt = 0;
                }
                if ( txbufidx ) {
                    this->julseview_usb_out_chars( (char*)txbuf, txbufidx );
                    byte_cnt += txbufidx;
                    txbufidx = 0;
                }
            }
            niblast = nibcurr;
            cword >>= 4;
        }
        rxbufdidx += 4;
    }

    //  Update scnt with actual samples processed
    uint32_t total_samples_sent = 8 + samp_remain; // First 8 + remaining samples
    scnt += total_samples_sent;
    JULSEDEBUG_DAT( "DEBUG: Sent %d samples, total scnt=%d/%d\n\r", total_samples_sent, scnt, num_samples );

    // Note: Termination character moved to end of complete capture
}



#define PRINT_IN_TRANSMIT 0
#define PRINT_DATA_SAMPLES 1 // Set to 1 to enable sample data printing

// Send mixed analog and digital data - UNIFIED VERSION
void julseview::send_slices_analog( uint8_t* dbuf, uint8_t* abuf ) {
    JULSEDEBUG_DAT("=== SENDING UNIFIED ANALOG/DIGITAL DATA ===\n\r");
    JULSEDEBUG_DAT("Mode: %s, Digital samples: %d, Analog samples: %d\n\r", 
                   use_decimation_mode ? "DECIMATION" : "NORMAL",
                   digital_samples_per_half, analog_samples_per_half);
    JULSEDEBUG_DAT("Buffer sizes: a_size=%d, d_size=%d, decimation_factor=%d\n\r", 
                   a_size, d_size, use_decimation_mode ? analog_decimation_factor : 1);
    JULSEDEBUG_DAT("Buffer addresses: dbuf=%p, abuf=%p\n\r", dbuf, abuf);
    
    // DEBUG: Show raw buffer contents for debugging
    JULSEDEBUG_DAT("Raw digital buffer (first 20 bytes): ");
    if (dbuf) {
        for (int i = 0; i < 20 && i < d_size; i++) {
            JULSEDEBUG_DAT("%d ", dbuf[i]);
        }
    }
    JULSEDEBUG_DAT("\n\r");
    
    JULSEDEBUG_DAT("Raw analog buffer (first 20 bytes): ");
    if (abuf) {
        for (int i = 0; i < 20 && i < a_size; i++) {
            JULSEDEBUG_DAT("%d ", abuf[i]);
        }
    }
    JULSEDEBUG_DAT("\n\r");
    
    // Determine correct sample count based on mode
    uint32_t current_samples;
    if (use_decimation_mode) {
        current_samples = digital_samples_per_half; // Use full digital capacity in decimation mode
    } else {
        current_samples = samples_per_half; // Use compatibility sample count in normal mode
    }

#if PRINT_DATA_SAMPLES
    JULSEDEBUG_DAT( "JulseView sending analog data - samples: %d, a_chan_cnt: %d, d_chan_cnt: %d\n\r", current_samples, a_chan_cnt, d_chan_cnt );
    JULSEDEBUG_ANA( "=== ANALOG DATA TRANSMISSION STARTING ===\n\r" );
    JULSEDEBUG_ANA( "Data format: Each sample = %d digital bytes + %d analog bytes = %d bytes total per sample\n\r", d_tx_bps, a_chan_cnt * 2, d_tx_bps + ( a_chan_cnt * 2 ) );
    JULSEDEBUG_ANA( "=== STARTING DATA STREAM ===\n\r" );
#endif

    uint32_t rxbufaidx = 0;
    rxbufdidx = 0;

    // Use correct sample count based on mode

    samp_remain = current_samples;

    // CRITICAL FIX: In decimation mode, we must limit to the smaller buffer capacity
    if (use_decimation_mode) {
        // In decimation mode, we can process up to digital_samples_per_half digital samples
        // but we need to ensure we don't exceed analog buffer capacity
        uint32_t max_analog_samples = analog_samples_per_half;
        uint32_t max_digital_samples = digital_samples_per_half;
        
        // CRITICAL: Calculate how many digital samples the analog buffer can support
        uint32_t max_digital_by_analog = max_analog_samples * analog_decimation_factor;
        
        // Use the smaller of digital buffer capacity and analog-supported capacity
        uint32_t max_samples_by_buffer = (max_digital_by_analog < max_digital_samples) ? 
                                         max_digital_by_analog : max_digital_samples;
        
        if (samp_remain > max_samples_by_buffer) {
            samp_remain = max_samples_by_buffer;
            JULSEDEBUG_DAT("DECIMATION BUFFER LIMIT: Limited samp_remain to %d (max_analog=%d, max_digital=%d, max_by_analog=%d)\n\r", 
                           samp_remain, max_analog_samples, max_digital_samples, max_digital_by_analog);
        }
        
        JULSEDEBUG_DAT("DECIMATION PROCESSING: %d samples with factor %d = %d analog samples needed\n\r", 
                       samp_remain, analog_decimation_factor, samp_remain / analog_decimation_factor);
        
        // CRITICAL: Verify we're not exceeding analog buffer capacity
        uint32_t analog_samples_needed = samp_remain / analog_decimation_factor;
        if (analog_samples_needed > max_analog_samples) {
            JULSEDEBUG_ERR("CRITICAL: Analog samples needed (%d) > available (%d) - this will cause buffer overrun!\n\r", 
                           analog_samples_needed, max_analog_samples);
            samp_remain = max_analog_samples * analog_decimation_factor;
            JULSEDEBUG_DAT("FORCED LIMIT: Reduced samp_remain to %d to prevent buffer overrun\n\r", samp_remain);
        }
        
        // CRITICAL: Ensure we don't exceed the actual number of samples requested
        if (samp_remain > num_samples - scnt) {
            samp_remain = num_samples - scnt;
            JULSEDEBUG_DAT("SAMPLE LIMIT: Reduced samp_remain to %d to match requested samples\n\r", samp_remain);
        }
    }

    if ( !cont && ( scnt + samp_remain > num_samples ) ) {
        samp_remain = num_samples - scnt;
        JULSEDEBUG_DAT( "CRITICAL: Limiting samp_remain to %d to avoid exceeding sample limit\n\r", samp_remain );
    }
    
    // FINAL VERIFICATION: Ensure buffer capacity is sufficient
    if (use_decimation_mode) {
        uint32_t final_analog_samples_needed = samp_remain / analog_decimation_factor;
        JULSEDEBUG_DAT("FINAL VERIFICATION: Processing %d samples = %d analog samples needed (available: %d)\n\r", 
                       samp_remain, final_analog_samples_needed, analog_samples_per_half);
        
        if (final_analog_samples_needed > analog_samples_per_half) {
            JULSEDEBUG_ERR("FINAL ERROR: Still exceeding analog buffer capacity!\n\r");
            samp_remain = analog_samples_per_half * analog_decimation_factor;
            JULSEDEBUG_DAT("FINAL FIX: Forced samp_remain to %d\n\r", samp_remain);
        }
    }

    // Sample limit checks removed - let data processing continue

    txbufidx = 0;

    // OPTIMIZATION: Pre-calculate constants outside the loop
    //  Calculate actual bytes per sample based on enabled channels
    const uint32_t analog_bytes_per_sample = a_chan_cnt * 2; // DMA buffer still contains all channels
    const uint32_t total_bytes_per_sample = d_tx_bps + analog_bytes_per_sample;
    const uint32_t max_buffer_size = sizeof( txbuf );

    // CONSERVATIVE: Smaller batch size for stability
    const uint32_t batch_size = 32; // Reduced from 128 to 32 for stability
    uint32_t samples_processed = 0;

#if PRINT_DATA_SAMPLES
    // Debug: Show first few raw ADC values
    JULSEDEBUG_DAT( "Raw ADC data (first 10 bytes): " );
    if ( a_chan_cnt > 0 && abuf != nullptr ) {
        for ( int i = 0; i < 10; i++ ) {
            JULSEDEBUG_DAT( "%d ", abuf[ i ] );
        }
    } else {
        JULSEDEBUG_DAT( "No analog data (a_chan_cnt=%d, abuf=%p)", a_chan_cnt, abuf );
    }
    JULSEDEBUG_DAT( "\n\r" );

    // Debug: Show first few raw digital values
    JULSEDEBUG_DAT( "Raw digital data (first 10 bytes): " );
    if ( dbuf ) {
        for ( int i = 0; i < 10; i++ ) {
            JULSEDEBUG_DAT( "%d ", dbuf[ i ] );
        }
        JULSEDEBUG_PRINTLN( );
    } else {
        JULSEDEBUG_ERR( "dbuf is NULL!\n\r" );
    }
    JULSEDEBUG_DAT( "\n\r" );
#endif

    while ( samples_processed < samp_remain ) {
        uint32_t batch_end = samples_processed + batch_size;
        if ( batch_end > samp_remain ) {
            batch_end = samp_remain;
        }

        // // CONSERVATIVE: More conservative buffer management
        // if (txbufidx + (batch_end - samples_processed) * total_bytes_per_sample > max_buffer_size * 3 / 4) {
        //     check_tx_buf(1);  // Force flush at 3/4 capacity
        // }

        // Process batch of samples
        for ( uint32_t s = samples_processed; s < batch_end; s++ ) {
            // SAFETY: Calculate buffer offsets with bounds checking
            uint32_t analog_offset = s * analog_bytes_per_sample;
            uint32_t digital_offset = s * d_dma_bps;

            // SAFETY: Strict bounds checking - only check analog bounds if analog channels are enabled
            bool analog_bounds_ok = ( a_chan_cnt == 0 ) || ( analog_offset < a_size );
            bool digital_bounds_ok = digital_offset < d_size;
            
            if ( !analog_bounds_ok || !digital_bounds_ok ) {
                JULSEDEBUG_ERR( "Buffer overrun detected - analog_offset=%d, a_size=%d, digital_offset=%d, d_size=%d\n\r", 
                               analog_offset, a_size, digital_offset, d_size );
                goto end_processing; // Exit both loops
            }

            // Send digital data if any digital channels enabled
            uint32_t cval = 0;
            // if (d_mask) {
            // SAFETY: Use original get_cval function for stability
            cval = get_cval( dbuf );
            
            // DEBUG: Show digital data reading for first few samples
            if (s < 5) {
                JULSEDEBUG_DAT("DIGITAL SAMPLE %d: cval=0x%08X (binary: ", s, cval);
                for (int b = 7; b >= 0; b--) {
                    JULSEDEBUG_DAT("%d", (cval >> b) & 1);
                }
                JULSEDEBUG_DAT(") from rxbufdidx=%d\n\r", rxbufdidx - d_dma_bps);
            }

            // OPTIMIZATION: Inline digital transmission
            for ( char b = 0; b < d_tx_bps; b++ ) {
                uint8_t data_byte = ( cval & 0x7F ) + 0x30;
                if ( data_byte < 0x80 )
                    data_byte |= 0x80;
                txbuf[ txbufidx++ ] = data_byte;
                cval >>= 7;
            }
            // }

            // UNIFIED ANALOG PROCESSING - handles both normal and decimation modes
            if ( a_chan_cnt > 0 && abuf != nullptr ) {
                if (use_decimation_mode) {
                    // DECIMATION MODE: Only send analog data for samples that should have new data
                    uint32_t analog_sample_idx = s / analog_decimation_factor;
                    
                    // SAFETY: Check if this analog sample index is within bounds
                    if (analog_sample_idx >= analog_samples_per_half) {
                        JULSEDEBUG_ERR("Analog sample index out of bounds: %d >= %d\n\r", 
                                       analog_sample_idx, analog_samples_per_half);
                        goto end_processing;
                    }
                    
                    if (s % analog_decimation_factor == 0) {
                        // This sample should have new analog data
                        uint32_t analog_offset = analog_sample_idx * (a_chan_cnt * 2);
                        
                        // SAFETY: Check analog bounds
                        if (analog_offset + (a_chan_cnt * 2) > a_size) {
                            JULSEDEBUG_ERR("Analog buffer overrun in decimation mode: offset=%d, size=%d\n\r", 
                                           analog_offset + (a_chan_cnt * 2), a_size);
                            goto end_processing;
                        }
                        
                        // DEBUG: Show what we're reading from the analog buffer
                        if (s < 5) { // Only show first few samples to avoid spam
                            JULSEDEBUG_DAT("DECIMATION SAMPLE %d: reading from analog_offset=%d (sample_idx=%d)\n\r", 
                                           s, analog_offset, analog_sample_idx);
                            JULSEDEBUG_DAT("  Raw bytes: ");
                            for (int j = 0; j < 10 && (analog_offset + j) < a_size; j++) {
                                JULSEDEBUG_DAT("%d ", abuf[analog_offset + j]);
                            }
                            JULSEDEBUG_DAT("\n\r");
                        }
                        
                        // Send analog data for enabled channels
                        for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                            if ( !( ( a_mask >> i ) & 1 ) ) {
                                continue; // Skip disabled channels
                            }

                            // Read 16-bit little-endian value from DMA buffer
                            uint16_t adc_value = abuf[ analog_offset ] | ( abuf[ analog_offset + 1 ] << 8 );

                            // Extract 12-bit value and split into 7-bit chunks
                            uint16_t adc_12bit = adc_value & 0x0FFF;
                            uint8_t byte1 = adc_12bit & 0x7F;
                            uint8_t byte2 = ( adc_12bit >> 7 ) & 0x7F;

                            txbuf[ txbufidx ] = byte1 + 0x30;
                            txbuf[ txbufidx + 1 ] = byte2 + 0x30;
                            txbufidx += 2;
                            analog_offset += 2; // 16-bit mode: 2 bytes per channel
                        }
                    }
                    // For decimation samples without new analog data, send nothing
                    // Driver will duplicate the previous analog sample
                    
                } else {
                    // NORMAL MODE: Send analog data for every sample
                    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                        if ( !( ( a_mask >> i ) & 1 ) ) {
                            continue; // Skip disabled channels
                        }

                        // SAFETY: Check bounds before reading
                        if ( rxbufaidx + 1 >= a_size ) {
                            JULSEDEBUG_ERR( "Analog buffer overrun at channel %d\n\r", i );
                            goto end_processing;
                        }

                        // Read 16-bit little-endian value from DMA buffer
                        uint16_t adc_value = abuf[ rxbufaidx ] | ( abuf[ rxbufaidx + 1 ] << 8 );

                        // Extract 12-bit value and split into 7-bit chunks
                        uint16_t adc_12bit = adc_value & 0x0FFF;
                        uint8_t byte1 = adc_12bit & 0x7F;
                        uint8_t byte2 = ( adc_12bit >> 7 ) & 0x7F;

                        txbuf[ txbufidx ] = byte1 + 0x30;
                        txbuf[ txbufidx + 1 ] = byte2 + 0x30;
                        txbufidx += 2;
                        rxbufaidx += 2; // 16-bit mode: 2 bytes per channel
                    }
                }
            } else {
                JULSEDEBUG_ANA( "No analog channels enabled or abuf is null - skipping analog processing\n\r" );
            }

#if PRINT_DATA_SAMPLES
            // Debug: Show sample data in table format for first, middle, and last 20 samples
            bool show_sample = ( s < 20 ) || ( s >= ( samp_remain / 2 - 20 ) && s < ( samp_remain / 2 + 20 ) ) || ( s >= samp_remain - 20 );

            if ( show_sample ) {
                if ( s == 0 ) {
                    JULSEDEBUG_STA( "\n\r=== SAMPLE DATA TABLE (First 20) ===\n\r" );
                    JULSEDEBUG_STA( "Sample tot | Sample cnk |  Digital | ADC0 | ADC1 | ADC2 | ADC3 | ADC4\n\r" );
                    JULSEDEBUG_STA( "-----------|------------|----------|------|------|------|------|------\n\r" );
                } else if ( s == ( samp_remain / 2 - 10 ) ) {
                    JULSEDEBUG_STA( "\n\r=== SAMPLE DATA TABLE (Middle 20) ===\n\r" );
                    JULSEDEBUG_STA( "Sample tot | Sample cnk |  Digital | ADC0 | ADC1 | ADC2 | ADC3 | ADC4\n\r" );
                    JULSEDEBUG_STA( "-----------|------------|----------|------|------|------|------|------\n\r" );
                } else if ( s == samp_remain - 20 ) {
                    JULSEDEBUG_STA( "\n\r=== SAMPLE DATA TABLE (Last 20) ===\n\r" );
                    JULSEDEBUG_STA( "Sample tot | Sample cnk|  Digital | ADC0 | ADC1 | ADC2 | ADC3 | ADC4\n\r" );
                    JULSEDEBUG_STA( "-----------|------------|----------|------|------|------|------|------\n\r" );
                }

                // Print sample number
                JULSEDEBUG_STA( "%8d  |  %8d   | ", scnt + s, s );

                // Print digital data as binary
                for ( int b = 7; b >= 0; b-- ) {
                    JULSEDEBUG_STA( "%d", ( cval >> b ) & 1 );
                }
                JULSEDEBUG_STA( " | " );

                // Print analog values - convert back to millivolts for readability
                //  Only show enabled channels in debug output
                if ( a_chan_cnt > 0 && abuf != nullptr ) {
                    uint32_t temp_rxbufaidx = rxbufaidx - ( a_chan_cnt * 2 ); // Go back to start of this sample's analog data
                    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                        // Only show channels that are enabled in a_mask
                        if ( !( ( a_mask >> i ) & 1 ) ) {
                            JULSEDEBUG_STA( "  -- | " ); // Show dashes for disabled channels
                            continue;
                        }

                        // Read 16-bit little-endian value from DMA buffer
                        uint16_t adc_raw = abuf[ temp_rxbufaidx ] | ( abuf[ temp_rxbufaidx + 1 ] << 8 ); // Little-endian
                        uint16_t adc_12bit = adc_raw & 0x0FFF;                                           // Mask to 12 bits (bits 0-11)

                        JULSEDEBUG_STA( "%4d | ", adc_12bit );
                        temp_rxbufaidx += 2; // 16-bit mode: 2 bytes per channel
                    }
                } else {
                    // Show dashes for all analog channels when none are enabled
                    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                        JULSEDEBUG_STA( "  -- | " );
                    }
                }
                JULSEDEBUG_STA( "\n\r" );
            }
#endif

            check_tx_buf( JULSEVIEW_TX_BUF_THRESH );
        }

        samples_processed = batch_end;

        // CONSERVATIVE: Flush buffer after each batch
        check_tx_buf( JULSEVIEW_TX_BUF_THRESH );
    }

end_processing:

#if PRINT_DATA_SAMPLES
    // Show transmission summary
    JULSEDEBUG_DAT( "\n\r=== TRANSMISSION SUMMARY ===\n\r" );
    JULSEDEBUG_DAT( "Total samples configured: %d\n\r", num_samples );
    JULSEDEBUG_DAT( "Total samples processed: %d\n\r", samp_remain );
    JULSEDEBUG_DAT( "Total samples sent: %d\n\r", scnt + samp_remain );
    JULSEDEBUG_DAT( "Analog channels: %d (±8V: ADC0-3, 0-5V: ADC4)\n\r", a_chan_cnt );
    JULSEDEBUG_DAT( "Digital channels: %d (GPIO pins 20-27)\n\r", d_chan_cnt );
    JULSEDEBUG_DAT( "=== CHUNK TRANSMISSION COMPLETE ===\n\r" );
#endif

    //  Update scnt with actual samples processed and check for completion
    scnt += samp_remain;
    JULSEDEBUG_DAT( "DEBUG: Sent %d samples, total scnt=%d/%d\n\r", samp_remain, scnt, num_samples );

    //  Don't send completion signal here - let the main capture loop handle it
    // This prevents race conditions where completion is sent before DMA finishes
    if ( scnt >= num_samples ) {
#if PRINT_DATA_SAMPLES
        JULSEDEBUG_CMD( "Sample limit reached (%d/%d) - DMA will continue until all data is captured\n\r", scnt, num_samples );
#endif
    }
}

// Calculate calibrated scaling and offset for analog channels //unused
void julseview::get_calibrated_analog_scaling( uint8_t channel, uint32_t* scale_microvolts, int32_t* offset_microvolts ) {
    // Get calibration data from config
    const auto& cal = jumperlessConfig.calibration;

    switch ( channel ) {
    case 0: // ADC_0
        // Use the same calculation method as readAdcVoltage()
        // Scale: (spread * 1000000) / 4095 (not 4096!)
        // Offset: -zero * 1000000 (negative because it gets subtracted in PulseView)
        JULSEDEBUG_CMD( "ADC_0: Spread: %f, Zero: %f\n\r", cal.adc_0_spread, cal.adc_0_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_0_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = -(int32_t)( ( cal.adc_0_spread / 2 ) * 1000000.0 );
        break;

    case 1: // ADC_1
        JULSEDEBUG_CMD( "ADC_1: Spread: %f, Zero: %f\n\r", cal.adc_1_spread, cal.adc_1_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_1_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = -(int32_t)( ( cal.adc_1_spread / 2 ) * 1000000.0 );
        break;

    case 2: // ADC_2
        JULSEDEBUG_CMD( "ADC_2: Spread: %f, Zero: %f\n\r", cal.adc_2_spread, cal.adc_2_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_2_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = -(int32_t)( ( cal.adc_2_spread / 2 ) * 1000000.0 );
        break;

    case 3: // ADC_3
        JULSEDEBUG_CMD( "ADC_3: Spread: %f, Zero: %f\n\r", cal.adc_3_spread, cal.adc_3_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_3_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = -(int32_t)( ( cal.adc_3_spread / 2 ) * 1000000.0 );
        break;

    case 4: // ADC_4
        // ADC4: No offset applied (same as readAdcVoltage logic)
        JULSEDEBUG_CMD( "ADC_4: Spread: %f, Zero: %f\n\r", cal.adc_4_spread, cal.adc_4_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_4_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = 0; // No offset for ADC4
        break;

    case 5: // ADC_5 (Pad_Sense)
        // ADC5: 0-3.3V range (no calibration data available)
        // JULSEDEBUG_CMD("ADC_5: %f, %f\n\r", cal.adc_5_spread, cal.adc_5_zero);
        *scale_microvolts = (uint32_t)( ( 3.3 * 1000000.0 ) / 4096.0 ); // 806 microvolts per step
        *offset_microvolts = 0;                                         // 0V = 0 microvolts
        break;

    case 7: // ADC_7 (if used)
        JULSEDEBUG_CMD( "ADC_7: Spread: %f, Zero: %f\n\r", cal.adc_7_spread, cal.adc_7_spread / 2 );
        *scale_microvolts = (uint32_t)( ( cal.adc_7_spread * 1000000.0 ) / 4096.0 );
        *offset_microvolts = -(int32_t)( ( cal.adc_7_spread / 2 ) * 1000000.0 );
        break;

    default:
        // Fallback for any other channels
        *scale_microvolts = (uint32_t)( ( 18.28 * 1000000.0 ) / 4096.0 ); // Default ±8V range
        *offset_microvolts = -(int32_t)( 8.0 * 1000000.0 );               // Default -8V offset
        break;
    }
}

// Check if command has timed out
bool julseview::isCommandTimedOut( ) const {
    if ( !receivedCommand ) {
        return false; // No command received, so no timeout
    }

    uint32_t current_time = millis( );
    uint32_t elapsed = current_time - lastCommandTime;

    // Handle millis() overflow
    if ( elapsed > 0x80000000 ) {
        elapsed = 0; // Reset if overflow occurred
    }

    return elapsed > commandTimeout;
}

// Print current state for debugging
void julseview::printState( ) const {
    JULSEDEBUG_STA( "=== JULSEVIEW STATE ===\n\r" );
    JULSEDEBUG_STA( "Legacy state: active=%d, armed=%d, started=%d, sending=%d\n\r",
                    julseview_active, armed, started, sending );
    JULSEDEBUG_STA( "Enhanced state: isArmed=%d, isRunning=%d, isTriggered=%d\n\r",
                    isArmed, isRunning, isTriggered );
    JULSEDEBUG_STA( "Command state: receivedCommand=%d, timeout=%d, elapsed=%d\n\r",
                    receivedCommand, commandTimeout,
                    receivedCommand ? ( millis( ) - lastCommandTime ) : 0 );
    JULSEDEBUG_STA( "Trigger state: enabled=%d, detected=%d, armed=%d\n\r",
                    trigger_config.enabled, trigger_detected, trigger_armed );
    JULSEDEBUG_STA( "Sample state: scnt=%d, num_samples=%d, cont=%d\n\r",
                    scnt, num_samples, cont );
    JULSEDEBUG_STA( "=====================\n\r" );
}

// Trigger system implementation
void julseview::setup_trigger( ) {
    JULSEDEBUG_CMD( "Setting up trigger system - type: %d, channel: %d\n\r",
                    trigger_config.type, trigger_config.channel );

    trigger_armed = true;
    trigger_detected = false;

    // Simplified trigger system - no pre-trigger samples
    pre_trigger_samples = 0;
    if ( post_trigger_samples == 0 ) {
        post_trigger_samples = num_samples; // All samples after trigger
    }

    JULSEDEBUG_CMD( "Trigger armed - post: %d samples (no pre-trigger)\n\r", post_trigger_samples );
}

bool julseview::check_trigger_condition( uint16_t analog_value, uint8_t digital_value ) {
    if ( !trigger_armed || trigger_detected ) {
        return false;
    }

    // Debug trigger checking
    static uint32_t check_count = 0;
    check_count++;
    if ( check_count % 1000 == 0 ) { // Log every 1000th check to avoid spam
        JULSEDEBUG_CMD( "Trigger check #%d: type=%d, analog=%d, digital=0x%02X\n\r",
                        check_count, trigger_config.type, analog_value, digital_value );
    }

    switch ( trigger_config.type ) {
    case TRIGGER_ANALOG_LEVEL:
        return check_analog_level_trigger( analog_value );

    case TRIGGER_ANALOG_EDGE:
        return check_analog_edge_trigger( analog_value );

    case TRIGGER_DIGITAL_EDGE:
        return check_digital_edge_trigger( digital_value );

    case TRIGGER_INTERNAL_VAR:
        return check_internal_var_trigger( );

    default:
        return false;
    }
}

bool julseview::check_analog_level_trigger( uint16_t analog_value ) {
    // Simple level crossing detection
    static uint16_t last_value = 0;
    bool triggered = false;

    switch ( trigger_config.edge ) {
    case EDGE_RISING:
        triggered = ( last_value < trigger_config.level ) && ( analog_value >= trigger_config.level );
        break;
    case EDGE_FALLING:
        triggered = ( last_value > trigger_config.level ) && ( analog_value <= trigger_config.level );
        break;
    case EDGE_EITHER:
        triggered = ( ( last_value < trigger_config.level ) && ( analog_value >= trigger_config.level ) ) ||
                    ( ( last_value > trigger_config.level ) && ( analog_value <= trigger_config.level ) );
        break;
    }

    last_value = analog_value;
    return triggered;
}

bool julseview::check_analog_edge_trigger( uint16_t analog_value ) {
    // Edge detection using slope calculation
    static uint16_t last_value = 0;
    static uint16_t second_last_value = 0;
    bool triggered = false;

    int16_t current_slope = analog_value - last_value;
    int16_t last_slope = last_value - second_last_value;

    switch ( trigger_config.edge ) {
    case EDGE_RISING:
        // Detect rising edge: positive slope with previous negative or zero
        triggered = ( current_slope > 0 ) && ( last_slope <= 0 );
        break;
    case EDGE_FALLING:
        // Detect falling edge: negative slope with previous positive or zero
        triggered = ( current_slope < 0 ) && ( last_slope >= 0 );
        break;
    case EDGE_EITHER:
        // Detect any edge: slope change from positive to negative or vice versa
        triggered = ( ( current_slope > 0 ) && ( last_slope <= 0 ) ) ||
                    ( ( current_slope < 0 ) && ( last_slope >= 0 ) );
        break;
    }

    second_last_value = last_value;
    last_value = analog_value;
    return triggered;
}

bool julseview::check_digital_edge_trigger( uint8_t digital_value ) {
    static uint8_t last_value = 0;
    bool triggered = false;

    uint8_t channel_mask = 1 << trigger_config.channel;
    uint8_t current_bit = ( digital_value & channel_mask ) ? 1 : 0;
    uint8_t last_bit = ( last_value & channel_mask ) ? 1 : 0;

    // Debug digital trigger checking
    static uint32_t debug_count = 0;
    debug_count++;
    if ( debug_count % 1000 == 0 ) { // Log every 1000th check
        JULSEDEBUG_CMD( "Digital trigger: channel=%d, mask=0x%02X, current=0x%02X(%d), last=0x%02X(%d), edge=%d\n\r",
                        trigger_config.channel, channel_mask, digital_value, current_bit, last_value, last_bit, trigger_config.edge );
    }

    switch ( trigger_config.edge ) {
    case EDGE_RISING:
        triggered = ( last_bit == 0 ) && ( current_bit == 1 );
        break;
    case EDGE_FALLING:
        triggered = ( last_bit == 1 ) && ( current_bit == 0 );
        break;
    case EDGE_EITHER:
        triggered = ( last_bit != current_bit );
        break;
    }

    if ( triggered ) {
        JULSEDEBUG_CMD( "DIGITAL TRIGGER DETECTED! channel=%d, edge=%d\n\r", trigger_config.channel, trigger_config.edge );
    }

    last_value = digital_value;
    return triggered;
}

bool julseview::check_internal_var_trigger( ) {
    // Check if the variable at the specified address matches the expected value
    if ( trigger_config.var_address == 0 ) {
        return false;
    }

    uint32_t* var_ptr = (uint32_t*)trigger_config.var_address;
    return ( *var_ptr == trigger_config.var_value );
}

void julseview::handle_trigger_detection( ) {
    JULSEDEBUG_CMD( "TRIGGER DETECTED!\n\r" );

    trigger_detected = true;
    isTriggered = true;
    JULSEDEBUG_CMD( "Enhanced state: isTriggered=true\n\r" );

    // Start the actual capture
    start_triggered_capture( );
}

void julseview::start_triggered_capture( ) {
    JULSEDEBUG_CMD( "Starting triggered capture - post: %d samples\n\r", post_trigger_samples );

    // Reset sample counter for post-trigger capture
    scnt = 0;
    num_samples = post_trigger_samples; // Capture post-trigger samples

    // The capture systems are already running from monitoring mode
    // Just enable data transmission
    sending = true;
    started = true;

    JULSEDEBUG_CMD( "Triggered capture started - will capture %d post-trigger samples\n\r", num_samples );
}

void julseview::configure_analog_trigger( uint8_t channel, uint16_t level, edge_type_t edge ) {
    trigger_config.type = TRIGGER_ANALOG_LEVEL;
    trigger_config.channel = channel;
    trigger_config.level = level;
    trigger_config.edge = edge;
    trigger_config.enabled = true;

    JULSEDEBUG_CMD( "Analog trigger configured - channel: %d, level: %d, edge: %d\n\r",
                    channel, level, edge );
}

void julseview::configure_digital_trigger( uint8_t channel, edge_type_t edge ) {
    trigger_config.type = TRIGGER_DIGITAL_EDGE;
    trigger_config.channel = channel;
    trigger_config.edge = edge;
    trigger_config.enabled = true;

    JULSEDEBUG_CMD( "Digital trigger configured - channel: %d, edge: %d\n\r",
                    channel, edge );
    JULSEDEBUG_CMD( "Trigger config: type=%d, channel=%d, edge=%d, enabled=%d\n\r",
                    trigger_config.type, trigger_config.channel, trigger_config.edge, trigger_config.enabled );
}

void julseview::configure_internal_trigger( uint32_t var_address, uint32_t var_value ) {
    trigger_config.type = TRIGGER_INTERNAL_VAR;
    trigger_config.var_address = var_address;
    trigger_config.var_value = var_value;
    trigger_config.enabled = true;

    JULSEDEBUG_CMD( "Internal trigger configured - address: 0x%08X, value: 0x%08X\n\r",
                    var_address, var_value );
}

// Global trigger interrupt flag
static volatile bool trigger_interrupt_fired = false;

// GPIO interrupt handler for trigger detection
static void trigger_irq_handler( uint gpio, uint32_t event_mask ) {
    trigger_interrupt_fired = true;
}

void julseview::waitForTrigger( ) {
    JULSEDEBUG_CMD( "waitForTrigger() - starting trigger monitoring with GPIO interrupt\n\r" );

    if ( trigger_config.type == TRIGGER_DIGITAL_EDGE ) {
        uint8_t trigger_pin = trigger_config.channel + 20; // GPIO 20-27 for channels 0-7

        JULSEDEBUG_CMD( "Trigger monitoring: channel=%d -> GPIO %d, edge=%d\n\r",
                        trigger_config.channel, trigger_pin, trigger_config.edge );

        // Reset interrupt flag
        trigger_interrupt_fired = false;

        // Configure GPIO interrupt based on trigger edge
        uint32_t irq_event_mask = 0;
        switch ( trigger_config.edge ) {
        case EDGE_RISING:
            irq_event_mask = GPIO_IRQ_EDGE_RISE;
            break;
        case EDGE_FALLING:
            irq_event_mask = GPIO_IRQ_EDGE_FALL;
            break;
        case EDGE_EITHER:
            irq_event_mask = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
            break;
        }

        // Set up GPIO interrupt
        gpio_set_irq_enabled_with_callback( trigger_pin, irq_event_mask, true, trigger_irq_handler );

        JULSEDEBUG_CMD( "GPIO interrupt configured for pin %d with mask 0x%X\n\r", trigger_pin, irq_event_mask );

        // Wait for interrupt
        while ( !trigger_interrupt_fired && !trigger_detected && julseview_active ) {
            // Small delay to prevent busy waiting
            sleep_ms( 1 );
        }

        // Disable GPIO interrupt
        gpio_set_irq_enabled( trigger_pin, irq_event_mask, false );

        if ( trigger_interrupt_fired ) {
            JULSEDEBUG_CMD( "TRIGGER DETECTED via GPIO interrupt! Pin %d (GPIO %d)\n\r",
                            trigger_config.channel, trigger_pin );
            trigger_detected = true;
            isTriggered = true;
        }
    }

    JULSEDEBUG_CMD( "waitForTrigger() - completed\n\r" );
}

// Pre-trigger functionality removed - simplified to post-trigger only

// ============================================================================
// DECIMATION MODE FUNCTIONS
// ============================================================================

// Configure decimation mode based on sample rate
void julseview::configure_decimation_mode() {
    JULSEDEBUG_STA("=== CONFIGURING DECIMATION MODE ===\n\r");
    JULSEDEBUG_STA("DEBUG: sample_rate=%d, a_chan_cnt=%d, total_load=%d, limit=%d\n\r", 
                   sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE);
    
    // Check if we need decimation mode
    // ADC limit is 200kHz TOTAL, not per channel
    if (sample_rate * a_chan_cnt > JULSEVIEW_ADC_MAX_RATE) {
        use_decimation_mode = true;
        
        // Calculate initial decimation factor (how many digital samples per analog sample)
        uint32_t initial_factor = (sample_rate * a_chan_cnt + JULSEVIEW_ADC_MAX_RATE - 1) / JULSEVIEW_ADC_MAX_RATE; // Round up
        
        // CRITICAL: Ensure decimation factor is an integer multiple of total samples
        // This prevents partial analog samples and ensures proper data alignment
        if (num_samples > 0) {
            // Find the smallest factor that's >= initial_factor and divides num_samples evenly
            analog_decimation_factor = initial_factor;
            
            // If the initial factor doesn't divide num_samples evenly, find the next suitable factor
            while (num_samples % analog_decimation_factor != 0) {
                analog_decimation_factor++;
                
                // Safety check to prevent infinite loop
                if (analog_decimation_factor > JULSEVIEW_DECIMATION_MAX_FACTOR) {
                    JULSEDEBUG_STA("WARNING: Could not find suitable decimation factor, using initial factor\n\r");
                    analog_decimation_factor = initial_factor;
                    break;
                }
            }
            
            JULSEDEBUG_STA("DECIMATION FACTOR ADJUSTMENT: initial=%d, final=%d (divides %d samples evenly)\n\r", 
                           initial_factor, analog_decimation_factor, num_samples);
        } else {
            // If num_samples is 0 (continuous mode), use the initial factor
            analog_decimation_factor = initial_factor;
        }
        
        // Clamp to reasonable limits
        if (analog_decimation_factor < JULSEVIEW_DECIMATION_MIN_FACTOR) {
            analog_decimation_factor = JULSEVIEW_DECIMATION_MIN_FACTOR;
        }
        if (analog_decimation_factor > JULSEVIEW_DECIMATION_MAX_FACTOR) {
            analog_decimation_factor = JULSEVIEW_DECIMATION_MAX_FACTOR;
        }
        
        JULSEDEBUG_STA("DECIMATION ENABLED: sample_rate=%d Hz × %d channels = %d Hz > %d Hz total ADC limit\n\r", 
                       sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE);
        JULSEDEBUG_STA("DECIMATION FACTOR: %d (each analog sample duplicated %d times)\n\r", 
                       analog_decimation_factor, analog_decimation_factor);
        JULSEDEBUG_STA("SAMPLE ALIGNMENT: %d samples ÷ %d factor = %d analog samples (remainder: %d)\n\r", 
                       num_samples, analog_decimation_factor, num_samples / analog_decimation_factor, 
                       num_samples % analog_decimation_factor);
        
        // CRITICAL: Verify the decimation factor divides the total samples evenly
        if (num_samples % analog_decimation_factor != 0) {
            JULSEDEBUG_STA("ERROR: Decimation factor %d does not divide %d samples evenly!\n\r", 
                           analog_decimation_factor, num_samples);
            JULSEDEBUG_STA("  This will cause data misalignment and buffer overruns!\n\r");
        } else {
            JULSEDEBUG_STA("✓ Decimation factor %d properly divides %d samples (remainder: 0)\n\r", 
                           analog_decimation_factor, num_samples);
        }
        
        // Calculate actual ADC sample rate per channel
        uint32_t actual_adc_rate_per_channel = (sample_rate * a_chan_cnt) / analog_decimation_factor / a_chan_cnt;
        JULSEDEBUG_STA("ACTUAL ADC RATE: %d Hz per channel (digital at %d Hz)\n\r", 
                       actual_adc_rate_per_channel, sample_rate);
        
    } else {
        use_decimation_mode = false;
        analog_decimation_factor = 1;
        
        uint32_t max_rate_per_channel = JULSEVIEW_ADC_MAX_RATE / a_chan_cnt;
        JULSEDEBUG_STA("NORMAL MODE: sample_rate=%d Hz × %d channels = %d Hz <= %d Hz total ADC limit\n\r", 
                       sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE);
        JULSEDEBUG_STA("NORMAL MODE: max rate per channel = %d Hz\n\r", max_rate_per_channel);
    }
}

// Calculate asymmetric buffer layout - ALWAYS USE ASYMMETRIC LAYOUT BY DEFAULT
void julseview::calculate_asymmetric_buffer_layout() {
    JULSEDEBUG_BUF("=== CALCULATING ASYMMETRIC BUFFER LAYOUT (DEFAULT) ===\n\r");
    
    // Always use asymmetric layout for maximum compatibility and performance
    // This gives us larger digital buffers and consistent behavior
    
    // Use full buffer sizes for maximum capacity
    a_size = JULSEVIEW_ANALOG_BUF_SIZE / 2;  // 32KB analog buffers
    d_size = JULSEVIEW_DIGITAL_BUF_SIZE / 2; // 16KB digital buffers
    
    // Buffer layout: [abuf0 32KB][abuf1 32KB][dbuf0 16KB][dbuf1 16KB] = 96KB total
    abuf0_start = 0;     // 0x00000 (perfectly aligned at buffer start)
    abuf1_start = 32768; // 0x8000 (32KB after abuf0)
    dbuf0_start = 65536; // 0x10000 (after analog buffers)
    dbuf1_start = 81920; // 0x14000 (16KB after dbuf0)
    
    JULSEDEBUG_BUF("ASYMMETRIC BUFFER LAYOUT (DEFAULT):\n\r");
    JULSEDEBUG_BUF("  abuf0: 0x%08X - 0x%08X (%d bytes)\n\r", 
                   abuf0_start, abuf0_start + a_size - 1, a_size);
    JULSEDEBUG_BUF("  abuf1: 0x%08X - 0x%08X (%d bytes)\n\r", 
                   abuf1_start, abuf1_start + a_size - 1, a_size);
    JULSEDEBUG_BUF("  dbuf0: 0x%08X - 0x%08X (%d bytes)\n\r", 
                   dbuf0_start, dbuf0_start + d_size - 1, d_size);
    JULSEDEBUG_BUF("  dbuf1: 0x%08X - 0x%08X (%d bytes)\n\r", 
                   dbuf1_start, dbuf1_start + d_size - 1, d_size);
    
    // Calculate samples per buffer based on actual buffer sizes
    digital_samples_per_half = d_size / d_dma_bps;  // 16384 samples (16KB / 1 byte)
    analog_samples_per_half = a_size / (a_chan_cnt * 2);  // 3276 samples (32KB / 10 bytes for 5 channels)
    
    JULSEDEBUG_BUF("SAMPLE COUNTS:\n\r");
    JULSEDEBUG_BUF("  digital_samples_per_half: %d\n\r", digital_samples_per_half);
    JULSEDEBUG_BUF("  analog_samples_per_half: %d\n\r", analog_samples_per_half);
    
    // For compatibility, set samples_per_half to the smaller of the two
    // This ensures we don't exceed either buffer capacity
    samples_per_half = (digital_samples_per_half < analog_samples_per_half) ? 
                       digital_samples_per_half : analog_samples_per_half;
    
    JULSEDEBUG_BUF("  samples_per_half (compatibility): %d\n\r", samples_per_half);
    
    // If we're in decimation mode, we can use the full digital buffer capacity
    if (use_decimation_mode) {
        JULSEDEBUG_BUF("DECIMATION MODE: Using full digital buffer capacity\n\r");
        JULSEDEBUG_BUF("  digital_samples_per_half: %d (full capacity)\n\r", digital_samples_per_half);
        JULSEDEBUG_BUF("  analog_samples_per_half: %d (decimated)\n\r", analog_samples_per_half);
        
        // CRITICAL: Verify we have enough analog samples to support the decimation factor
        uint32_t required_analog_samples = digital_samples_per_half / analog_decimation_factor;
        if (required_analog_samples > analog_samples_per_half) {
            JULSEDEBUG_BUF("WARNING: Decimation factor %d requires %d analog samples, but only %d available\n\r", 
                           analog_decimation_factor, required_analog_samples, analog_samples_per_half);
            JULSEDEBUG_BUF("  This may cause buffer overruns - consider reducing sample rate or decimation factor\n\r");
        } else {
            JULSEDEBUG_BUF("DECIMATION CAPACITY: %d analog samples can support %d digital samples with factor %d\n\r", 
                           analog_samples_per_half, analog_samples_per_half * analog_decimation_factor, analog_decimation_factor);
        }
    }
}

// send_slices_analog_decimated function removed - unified into send_slices_analog

// Duplicate analog sample data for decimation mode - REMOVED
// This function is no longer needed as the driver now handles duplication
// The firmware only sends actual analog data and lets the driver duplicate it
