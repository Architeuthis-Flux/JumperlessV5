#include "JulseView.h"
#include "ArduinoStuff.h" // For USBSer2
#include "Graphics.h"
#include "Peripherals.h"

#include "JulseView.h"
#include "class/cdc/cdc_device.h"
#include "config.h"
#include "hardware/pio_instructions.h"
#include "hardware/structs/watchdog.h"
// Global digital buffer size (default 32KB). Can be adjusted at runtime if needed.
uint32_t g_julseview_digital_buf_size = 16384;
#include "hardware/regs/adc.h"
#include "hardware/structs/adc.h"
#include "pico.h"

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
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "tusb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/watchdog.h"

// Global variables for heartbeat watchdog
static volatile uint32_t heartbeat_last_time = 0;
static volatile uint32_t heartbeat_timeout_us = 0;
static volatile bool heartbeat_triggered = false;

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

#define JULSEDEBUG_MASK_ON 1
uint32_t julseview_debug_mask = JULSEDEBUG_MASK_ON ? JULSEDEBUG_ERRORS |
                                                         JULSEDEBUG_COMMANDS |
                                                         JULSEDEBUG_USBS |
                                                         JULSEDEBUG_DMAS |
                                                         JULSEDEBUG_DATA |
                                                         JULSEDEBUG_ANALOG |
                                                         JULSEDEBUG_DIGITAL |
                                                         JULSEDEBUG_TIMING |
                                                         JULSEDEBUG_STATE |
                                                         JULSEDEBUG_BUFFERS
                                                   : 0; // Default: errors and commands only

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

volatile bool pyTrigger = false;
volatile bool control_D[ 4 ] = { true, true, false, false };
volatile float control_A[ 4 ] = { 5.0f, 2.0f, 1.0f, -5.0f };
// Global control byte sampled by control DMA at the digital DREQ rate
volatile uint8_t control_data = 0xfa;

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

static const uint16_t pio_slowgram_instructions[] = {
    (uint16_t)pio_encode_in( pio_pins, 8 ), // in pins, 8 - read 8 pins into input shift register
    (uint16_t)pio_encode_irq_set( false, 1 ),  // raise PIO IRQ 1 (absolute) once per sample
    (uint16_t)pio_encode_delay( 29 ),
    (uint16_t)pio_encode_delay( 30 ),
    (uint16_t)pio_encode_delay( 31 ),
    (uint16_t)pio_encode_delay( 31 ),
    (uint16_t)pio_encode_delay( 31 ),
    (uint16_t)pio_encode_delay( 31 ),
    (uint16_t)pio_encode_delay( 31 ),
    
    (uint16_t)pio_encode_delay( 30 ),
    (uint16_t)pio_encode_delay( 0 ),

};
static const struct pio_program pio_slowgram_program = {
    .instructions = pio_slowgram_instructions,
    .length = 11,
    .origin = -1,
};



// --- Globals for DMA and PIO ---
dma_channel_config acfg0, acfg1, pcfg0, pcfg1;
uint admachan0, admachan1, pdmachan0, pdmachan1, coord_dma_chan;
uint32_t coordinator_buffer[ 2 ]; // Tiny buffer for coordinator DMA
PIO lapio = pio1;                   // Use PIO1 to avoid conflicts with CH446Q (which uses PIO0)
// piosm is now a class member variable, not global

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

// --- Streaming mode control ---
// Stream mode is decided per arm() based on current sample_rate (not a sticky global)
static bool stream_mode = false;
// Conservative sustained CDC-ACM throughput estimate (bytes/sec)
// Full-speed USB typically achieves ~1 MB/s; use a margin for reliability.
volatile uint32_t usb_estimated_bps = 80000;
// Minimum samples per DMA half when streaming to ensure useful chunking
volatile uint32_t stream_min_samples_per_half = 64;
// Empirical correction for RP2350 PIO clock vs clk_sys. If PIO executes at clk_sys/8,
// set this to 8 so divider uses effective PIO input clock = clk_sys / pio_clk_correction.
volatile uint32_t pio_clk_correction = 8;

// Precomputed analog channel list for ISR-driven START_ONCE sequencing
volatile uint8_t ana_enabled_count = 0;
volatile uint8_t ana_enabled_list[ 8 ] = { 0 };
volatile uint32_t pio_irq_pending = 0;
volatile uint32_t pio_irq_count = 0;

// --- PIO IRQ for analog single-shot sampling ---
void julseview_pio_irq0_handler(void) {
    // Clear IRQ 0 (any asserted PIO interrupt bit on this PIO)

    pio_irq_count++;
    //JULSEDEBUG_ERR( "ana_enabled_count: %d\n\r", ana_enabled_count );

    // If no analog channels, nothing to do
    if ( ana_enabled_count == 0 ) {
        pio_interrupt_clear( lapio, 1 );
        return;
    }



    // Issue START_ONCE per enabled channel; DMA with DREQ_ADC will fetch each result
    for ( uint8_t i = 0; i < ana_enabled_count; i++ ) {
        uint8_t ch = ana_enabled_list[ i ];
        adc_select_input( ch );
        adc_hw->cs |= ADC_CS_START_ONCE_BITS;
        // Wait until conversion finished; READY goes 1 when a new conversion can start
        // while ( ( adc_hw->cs & ADC_CS_READY_BITS ) == 0 ) {
        // }
       // JULSEDEBUG_ERR( "ADC IRQ Fired: %d\n\r", ch );
       // Serial.flush( );
    }
    pio_interrupt_clear( lapio, 1 );
}

// --- Helper Functions ---





// (moved ISR below global variable declarations)




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
    if ( !buf || length <= 0 || length > 4096 ) {
        JULSEDEBUG_ERR( "ERROR: Invalid USB transmission parameters - buf=%p, length=%d\n\r", buf, length );
        return false;
    }

    // Check if CDC is connected and ready
    if ( !tud_cdc_n_connected( 2 ) ) {
        JULSEDEBUG_USB( "CDC not connected\n\r" );
        return false;
    }

    // BLOCKING APPROACH: Just block until we can send all the data
    int offset = 0;

    while ( offset < length ) {
        int remaining = length - offset;

        // Wait for USB buffer space to be available
        int available;
        do {
            available = tud_cdc_n_write_available( 2 );
            if ( available == 0 ) {
                tud_task( );              // Process USB events
                delayMicroseconds( 500 ); // Short delay
            }
        } while ( available == 0 );

        // Send as much as we can (up to what's available)
        int chunk_size = min( remaining, available );
        // JULSEDEBUG_ERR("Sending chunk of %d bytes remaining %d available %d\n\r", chunk_size, remaining, available   );

        // Write the chunk
        int written = tud_cdc_n_write( 2, buf, chunk_size );
        if ( written <= 0 ) {
            // This shouldn't happen since we waited for space
            JULSEDEBUG_USB( "Unexpected USB write failure\n\r" );
            tud_task( );
            delayMicroseconds( 100 );
            continue; // Try again
        }

        offset += written;

        // Process USB events periodically
        // if ( offset % 512 == 0 ) {
        tud_task( );
        // }
    }

    // Final flush to ensure all data is transmitted
    tud_cdc_n_write_flush( 2 );

    // delayMicroseconds( 50 );
    tud_task( );

    return true;
}

// --- Class Methods ---
// Constructor - initialize member variables to safe defaults
julseview::julseview( ) {
    // Initialize flags
    initialized = false;
    dma_ended_flag = false;

    // Initialize state variables
    sending = false;
    cont = false;
    aborted = false;
    started = false;
    armed = false;
    isArmed = false;
    isRunning = false;
    isTriggered = false;
    receivedCommand = false;

    // Initialize DMA channels and PIO to unclaimed state
    admachan0 = admachan1 = pdmachan0 = pdmachan1 = coord_dma_chan = -1;
    control_dma_chan0 = control_dma_chan1 = -1;
    piosm = -1; // Initialize PIO state machine as unclaimed

    // Initialize buffer pointers

    capture_buf = nullptr;
    actual_buffer_size = 0;

    // Initialize DMA register pointers
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    // Initialize other variables
    scnt = 0;
    cmdstrptr = 0;
    current_dma_half = 0;

    // Initialize trigger system
    trigger_config.enabled = false;
    trigger_armed = false;
    trigger_detected = false;

    // Initialize timing variables
    lastCommandTime = 0;
    commandTimeout = 5000;
    last_transmission_time = 0;
    transmission_count = 0;

    // Initialize heartbeat watchdog
    heartbeat_watchdog_triggered = false;
    last_heartbeat_time = 0;
    heartbeat_timeout_us = 0;
    expected_capture_time_us = 0;
    completion_signal_sent = false;
}

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

    // Initialize control channel buffer addresses
    cbuf0_start = 0;
    cbuf1_start = 0;

    // Initialize heartbeat watchdog variables
    heartbeat_watchdog_triggered = false;
    last_heartbeat_time = 0;
    heartbeat_timeout_us = 0;
    expected_capture_time_us = 0;
    completion_signal_sent = false;

    JULSEDEBUG_ERR( "This is experimental, just be kind with sample rate and depth\n\r" );
    // JULSEDEBUG_ERR("Analog channels ca\n\r");

    // Reuse existing buffer if already allocated (don't free and reallocate)
    if ( capture_buf && actual_buffer_size > 0 ) {
        JULSEDEBUG_BUF( "Reusing existing capture buffer (%d bytes)\n\r", actual_buffer_size );
        // Clear the buffer for fresh use
        memset( capture_buf, 0, actual_buffer_size );
    }

    // Only allocate new buffer if we don't already have a valid one
    if ( !capture_buf || actual_buffer_size == 0 ) {

        g_julseview_digital_buf_size = rp2040.getFreeHeap( ) - 16384 - JULSEVIEW_ANALOG_BUF_SIZE;
        // Show available memory before allocation
        JULSEDEBUG_BUF( "Free memory before allocation: %d g_julseview_digital_buf_size: %d\n\r", rp2040.getFreeHeap( ), g_julseview_digital_buf_size );

        //  Ensure we don't allocate more memory than available
        uint32_t requested_buffer_size = ( JULSEVIEW_ANALOG_BUF_SIZE + g_julseview_digital_buf_size );
        uint32_t available_memory = rp2040.getFreeHeap( );

        // Leave some safety margin for other operations
        if ( requested_buffer_size > available_memory ) {
            JULSEDEBUG_BUF( "WARNING: Requested buffer size %d bytes may exceed available memory %d bytes\n\r",
                            requested_buffer_size, available_memory );

            // Reduce buffer size to fit with safety margin
            requested_buffer_size = available_memory - 16384;
            JULSEDEBUG_BUF( "Reducing buffer size to %d bytes\n\r", requested_buffer_size );
        }

        // Allocate buffer with simple malloc
        size_t total_allocation = requested_buffer_size;

        // CRITICAL: Check available memory before allocation
        size_t free_heap = rp2040.getFreeHeap( );
        JULSEDEBUG_BUF( "INIT: Memory check - need %d bytes, have %d bytes\n\r", total_allocation, free_heap );
        if ( free_heap < total_allocation ) { // Leave 1KB safety margin
            JULSEDEBUG_ERR( "ERROR: Insufficient memory! Need %d bytes, have %d bytes\n\r",
                            total_allocation, free_heap );
            return false;
        }

        capture_buf = (uint8_t*)malloc( total_allocation );
        if ( !capture_buf ) {
            JULSEDEBUG_ERR( "ERROR: Failed to allocate %d bytes for capture buffer!\n\r", total_allocation );
            JULSEDEBUG_BUF( "Available memory: %d\n\r", free_heap );
            return false;
        }

        JULSEDEBUG_BUF( "Successfully allocated capture buffer: %d bytes at address 0x%08X\n\r",
                        total_allocation, (uint32_t)capture_buf );

        // Initialize buffer to zeros to avoid reading garbage
        memset( capture_buf, 0, total_allocation );

        // Store actual allocated size for later bounds checking
        actual_buffer_size = total_allocation;
    } else {
        JULSEDEBUG_BUF( "Using existing buffer allocation (%d bytes)\n\r", actual_buffer_size );
    }

    // Initialize DMA channels to invalid values - will be claimed in arm() when we know the config
    admachan0 = admachan1 = pdmachan0 = pdmachan1 = coord_dma_chan = -1;

    // Initialize DMA register pointers to null - will be set up in arm()
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    // Claim a state machine if we don't already have one
    JULSEDEBUG_STA( "INIT: piosm before claiming: %d\n\r", piosm );
    if ( piosm < 0 ) {
        piosm = pio_claim_unused_sm( lapio, true );
        if ( piosm < 0 ) {
            JULSEDEBUG_ERR( "ERROR: Failed to claim PIO state machine on PIO1!\n\r" );
            return false;
        }
        JULSEDEBUG_STA( "Successfully claimed PIO1 state machine %d\n\r", piosm );
    } else {
        JULSEDEBUG_STA( "Reusing existing PIO1 state machine %d\n\r", piosm );
    }

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

    // NOTE: Don't reset piosm here as init() may have already claimed it
    // PIO state machine cleanup is handled by end() and deinit()

    // DMA channels will be cleaned up by end() or deinit() when needed
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

// --- Refactored arming helpers (declarations in header) ---
void julseview::armAnalogCommon() {
    adc_run( false );
    acfg0 = dma_channel_get_default_config( admachan0 );
    acfg1 = dma_channel_get_default_config( admachan1 );
    channel_config_set_transfer_data_size( &acfg0, DMA_SIZE_16 );
    channel_config_set_transfer_data_size( &acfg1, DMA_SIZE_16 );
    channel_config_set_read_increment( &acfg0, false );
    channel_config_set_read_increment( &acfg1, false );
    channel_config_set_write_increment( &acfg0, true );
    channel_config_set_write_increment( &acfg1, true );
    channel_config_set_dreq( &acfg0, DREQ_ADC );
    channel_config_set_dreq( &acfg1, DREQ_ADC );

    channel_config_set_chain_to( &acfg0, admachan1 );
    channel_config_set_chain_to( &acfg1, admachan0 );

    uint32_t analog_transfers = use_decimation_mode ? ( analog_samples_per_half * a_chan_cnt )
                                                    : ( samples_per_half * a_chan_cnt );

    dma_channel_configure( admachan0, &acfg0, &capture_buf[ abuf0_start ], &adc_hw->fifo, analog_transfers, false );
    dma_channel_configure( admachan1, &acfg1, &capture_buf[ abuf1_start ], &adc_hw->fifo, analog_transfers, false );
    JULSEDEBUG_DMA( "Analog DMA channels configured\n\r" );

    adc_fifo_setup( true, true, 1, false, false );
    adc_fifo_drain( );

    if ( sample_rate == 0 || a_chan_cnt == 0 ) {
        JULSEDEBUG_ERR( "ERROR: Invalid ADC parameters! sample_rate=%d, a_chan_cnt=%d\n\r", sample_rate, a_chan_cnt );
        return;
    }

    uint32_t actual_adc_rate = use_decimation_mode ? ( sample_rate / analog_decimation_factor ) : sample_rate;
    uint32_t adcdivint = 0;
    uint8_t adc_frac_int = 0;
    if ( !stream_mode ) {
        adcdivint = 48000000ULL / ( actual_adc_rate * a_chan_cnt );
        adc_frac_int = (uint8_t)( ( ( 48000000ULL % ( actual_adc_rate * a_chan_cnt ) ) * 256ULL ) / ( actual_adc_rate * a_chan_cnt ) );
        if ( adcdivint == 0 || adcdivint > 65535 ) {
            JULSEDEBUG_ERR( "ERROR: Invalid ADC divisor! adcdivint=%d\n\r", adcdivint );
            adcdivint = 1;
        }
    } else {
        JULSEDEBUG_ANA( "STREAM MODE: using fast ADC clock (clkdiv=1.0) for single-shot pacing\n\r" );
    }

    uint8_t adc_mask = 0;
    for ( int i = 0; i < 8; i++ ) if ( ( a_mask >> i ) & 1 ) adc_mask |= ( 1 << i );
    adc_first_channel = get_first_adc_channel( adc_mask );
    adc_fifo_drain( );
    if ( stream_mode ) {
        adc_set_clkdiv( 1.0f );
    } else {
        volatile uint32_t* adcdiv = (volatile uint32_t*)( ADC_BASE + 0x10 );
        if ( adcdiv ) {
            if ( adcdivint <= 96 ) *adcdiv = 0; else *adcdiv = ( ( adcdivint - 1 ) << 8 ) | adc_frac_int;
        }
    }
    adc_fifo_drain( );
}

void julseview::armAnalogSlow() { armAnalogCommon(); }
void julseview::armAnalogFast() { armAnalogCommon(); }

void julseview::armDigitalSelected(bool use_slow_program) {
    if ( capture_buf == nullptr ) {
        JULSEDEBUG_ERR( "ERROR: capture_buf is null during digital setup!\n\r" );
        return;
    }

    pcfg0 = dma_channel_get_default_config( pdmachan0 );
    pcfg1 = dma_channel_get_default_config( pdmachan1 );
    channel_config_set_transfer_data_size( &pcfg0, DMA_SIZE_8 );
    channel_config_set_transfer_data_size( &pcfg1, DMA_SIZE_8 );
    channel_config_set_read_increment( &pcfg0, false );
    channel_config_set_read_increment( &pcfg1, false );
    channel_config_set_write_increment( &pcfg0, true );
    channel_config_set_write_increment( &pcfg1, true );
    // Match old digital setup: set up ping-pong chaining between the two digital DMA channels
    channel_config_set_chain_to( &pcfg0, pdmachan1 );
    channel_config_set_chain_to( &pcfg1, pdmachan0 );

    // Recompute transfers using the latest samples_per_half after any stream-mode adjustments
    uint32_t digital_transfers = use_decimation_mode ? ( digital_samples_per_half * d_dma_bps )
                                                     : ( samples_per_half * d_dma_bps );

    // Match old behavior: clear digital halves prior to arming
    memset( &capture_buf[ dbuf0_start ], 0, d_size );
    memset( &capture_buf[ dbuf1_start ], 0, d_size );

    // Always use the slow program; emulate fast mode by wrapping to the first instruction only.
    const struct pio_program* selected_program = &pio_slowgram_program;
    uint32_t cycles_per_sample = use_slow_program ? 32u : 1u;

    static bool last_use_slow_program = false;
    static int last_offset = 0;

    pio_sm_restart( lapio, piosm );
    pio_sm_set_enabled( lapio, piosm, false );
    pio_sm_clear_fifos( lapio, piosm );
   
    pio_sm_drain_tx_fifo( lapio, piosm );




    if ( !pio_can_add_program_at_offset( lapio, selected_program, last_offset ) ) {
        pio_remove_program( lapio, selected_program, last_offset );
        if ( !pio_can_add_program_at_offset( lapio, selected_program, last_offset ) ) {
            JULSEDEBUG_ERR( "ERROR: Still cannot add selected PIO program after removal!\n\r" );
            return;
        }
    }

    last_offset = pio_add_program_at_offset( lapio, selected_program, last_offset );

   // printPIOStateMachines( );


    uint offset = last_offset;
    if ( offset == (uint)-1 ) {
        JULSEDEBUG_ERR( "ERROR: Failed to add selected PIO program despite can_add check!\n\r" );
        return;
    }


    JULSEDEBUG_DIG( "last_offset: %d  pio program counter: %d\n\r", last_offset, pio_sm_get_pc(lapio, piosm));
    pio_sm_config c = pio_get_default_sm_config( );
    sm_config_set_in_pins( &c, 20 );
    sm_config_set_in_pin_count( &c, 8 );
    sm_config_set_in_shift( &c, false, true, 8 );
    sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );
    // For fast mode: wrap only the first instruction (in pins, 8) -> acts as fast capture loop
    if ( use_slow_program ) {
        sm_config_set_wrap( &c, offset, offset + selected_program->length - 1 );
    } else {
        sm_config_set_wrap( &c, offset, offset );
    }

    uint32_t sys_clock = clock_get_hz( clk_sys );
    // With unified slow program, cycles/sample is 32 in slow mode, 1 in fast mode
    float clock_div = (float)sys_clock / ( (float)sample_rate * (float)cycles_per_sample );
    // In fast mode on RP2350 PIO, slight correction may be needed if PIO clock differs from clk_sys
    // if ( !stream_mode && pio_clk_correction > 1 ) {
    //     clock_div *= (float)pio_clk_correction;
    // }
    if ( clock_div < 1.0f ) clock_div = 1.0f;
    if ( clock_div > 65536.0f ) clock_div = 65536.0f;
    sm_config_set_clkdiv( &c, clock_div );

    // Old sequence: init SM, clear/enable PIO IRQ1, bind shared handler, then set up DMA
    pio_sm_init( lapio, piosm, offset, &c );
    pio_interrupt_clear( lapio, 1 );
    pio_set_irq1_source_enabled( lapio, pis_interrupt1, true );
    if ( !irq_has_shared_handler( PIO1_IRQ_1 ) ) {
        irq_add_shared_handler( PIO1_IRQ_1, julseview_pio_irq0_handler, 0 );
        irq_set_enabled( PIO1_IRQ_1, true );
    }

    // Old function uses RX DREQ (false -> RX)
    dreq = pio_get_dreq( lapio, piosm, false );
    if ( dreq == (uint)-1 ) {
        JULSEDEBUG_ERR( "ERROR: Failed to get PIO DREQ!\n\r" );
        return;
    }
    channel_config_set_dreq( &pcfg0, dreq );
    channel_config_set_dreq( &pcfg1, dreq );
    // Match old behavior: configure but do not start, ping-pong addresses set now
    dma_channel_configure( pdmachan0, &pcfg0, &capture_buf[ dbuf0_start ], &lapio->rxf[ piosm ], digital_transfers, false );
    dma_channel_configure( pdmachan1, &pcfg1, &capture_buf[ dbuf1_start ], &lapio->rxf[ piosm ], digital_transfers, false );
    JULSEDEBUG_DMA( "Digital DMA configured: transfers=%u, write=0x%08X, read=0x%08X\n\r",
                    (unsigned)digital_transfers,
                    (unsigned)(uintptr_t)&capture_buf[ dbuf0_start ],
                    (unsigned)(uintptr_t)&lapio->rxf[ piosm ] );
}

void julseview::armDigitalSlow() { armDigitalSelected(true); }
void julseview::armDigitalFast() { armDigitalSelected(false); }

void julseview::arm( ) {
    JULSEDEBUG_STA( "=== JULSEVIEW ARM() START ===\n\r" );

    // DEBUG: Print PIO state machine status before arming
    // printPIOStateMachines( );

    //  Set default num_samples if driver didn't send 'L' command
    if ( num_samples == 0 ) {
        JULSEDEBUG_CMD( "WARNING: Driver didn't send 'L' command, using default 10000 samples\n\r" );
        num_samples = 10000;
    }

    // JULSEDEBUG_STA( "Stopping ADC...\n\r" );
    // adc_run( false );

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
    dma_ended_flag = false;  // Reset DMA end flag for new capture
    JULSEDEBUG_STA( "JulseView active set to true, isArmed=true\n\r" );

    // JULSEDEBUG_STA( "Configuring DMA mode...\n\r" );

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

    JULSEDEBUG_STA( "Analog channels: %d\n\r", a_chan_cnt );

    // Configure decimation mode based on sample rate and analog channel count
    configure_decimation_mode( );

    JULSEDEBUG_STA( "Setting up digital channels...\n\r" );

    // Check if control channels are enabled to determine digital channel configuration
    bool control_channels_enabled = ( c_mask != 0 );

    if ( control_channels_enabled ) {
        // Control channels enabled - total logical channels = 16, but PIO provides 8-bit GPIO only.
        // We capture GPIO via DMA (1 byte/sample) and capture control via a separate DMA.
        d_chan_cnt = 16; // 16 channels total (8 GPIO + 8 control)
        d_dma_bps = 1;   // 1 byte per sample from PIO (GPIO only)
        d_tx_bps = 3;    // 3 bytes for transmission (16 channels need 3 7-bit chunks)
        JULSEDEBUG_DIG( "CONTROL CHANNELS ENABLED: d_chan_cnt=16, d_dma_bps=1, d_tx_bps=3\n\r" );
        JULSEDEBUG_DIG( "DEBUG: c_mask=0x%08X, d_mask=0x%08X\n\r", c_mask, d_mask );
    } else {
        // No control channels - use 8 digital channels (GPIO only)
        d_chan_cnt = 8; // 8 channels total
        d_dma_bps = 1;  // 1 byte per sample for 8 channels
        d_tx_bps = 2;   // 2 bytes for transmission (8 channels need 2 7-bit chunks)
        JULSEDEBUG_DIG( "NO CONTROL CHANNELS: d_chan_cnt=8, d_dma_bps=1, d_tx_bps=2\n\r" );
    }

    // Calculate pin count for display purposes only
    // pin_count = 0;
    // if ( d_mask & 0x0000000F )
    //     pin_count += 4;
    // if ( d_mask & 0x000000F0 )
    //     pin_count += 4;
    // if ( d_mask & 0x0000FF00 )
    //     pin_count += 8;
    // if ( d_mask & 0x0FFF0000 )
    //     pin_count += 16;
    // if ( ( pin_count == 4 ) && ( a_chan_cnt ) ) {
    //     pin_count = 8;
    // }

    // d_nps = pin_count / 4;

    JULSEDEBUG_DIG( "JulseView pin_count: %d, d_dma_bps: %d\n\r", pin_count, d_dma_bps );
    JULSEDEBUG_DIG( "JulseView d_chan_cnt: %d, d_nps: %d, d_tx_bps: %d\n\r", d_chan_cnt, d_nps, d_tx_bps );

    // Decide streaming mode early (before buffer sizing) based on sample rate
    // so we can size ping-pong halves appropriately.
    // Decide stream mode for this arm() based on current sample_rate
    const uint32_t slow_threshold_hz = 10000; // same threshold used for slow PIO selection
    stream_mode = ((uint32_t)sample_rate <= slow_threshold_hz);
    JULSEDEBUG_BUF( "STREAM MODE: %s (sample_rate=%d, threshold=%u)\n\r",
                    stream_mode ? "enabled" : "disabled", sample_rate, slow_threshold_hz );

    JULSEDEBUG_STA( "Starting DMA setup...\n\r" );
    // SETUP DMA CHANNELS AND CONFIGURATIONS (moved from init() to arm() for efficiency)
    // Only set up what we actually need based on the current configuration
    JULSEDEBUG_DMA( "Setting up DMA channels and configurations...\n\r" );

    if ( a_chan_cnt > 0 ) {
        // Build fast list for ISR sequencing
        ana_enabled_count = 0;
        for ( uint8_t i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
            if ( ( a_mask >> i ) & 1 ) {
                if ( ana_enabled_count < 8 ) {
                    ana_enabled_list[ ana_enabled_count++ ] = i;
                }
            }
        }
        JULSEDEBUG_STA( "Initializing ADC...\n\r" );
        adc_init( );
        JULSEDEBUG_ANA( "ADC initialized for %d analog channels\n\r", a_chan_cnt );
        JULSEDEBUG_STA( "Claiming analog DMA channels...\n\r" );
        admachan0 = dma_claim_unused_channel( true );
        admachan1 = dma_claim_unused_channel( true );
        JULSEDEBUG_DMA( "Claimed analog DMA channels: %d, %d\n\r", admachan0, admachan1 );
        taddra0 = &dma_hw->ch[ admachan0 ].write_addr; // Analog DMA channel 0 write address
        taddra1 = &dma_hw->ch[ admachan1 ].write_addr; // Analog DMA channel 1 write address
        // Use sigrok-pico approach: DMA_BASE + channel offset + register offset
        tstsa0 = (volatile uint32_t*)( DMA_BASE + 0x40 * admachan0 + 0xc ); // DMA_WRITE_sts offset
        tstsa1 = (volatile uint32_t*)( DMA_BASE + 0x40 * admachan1 + 0xc ); // DMA_WRITE_sts offset
    }
    // }

    if ( d_chan_cnt > 0 ) {
        JULSEDEBUG_STA( "Claiming digital DMA channels...\n\r" );
        pdmachan0 = dma_claim_unused_channel( true );
        pdmachan1 = dma_claim_unused_channel( true );
        JULSEDEBUG_DMA( "Claimed digital DMA channels: %d, %d\n\r", pdmachan0, pdmachan1 );

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
    calculate_asymmetric_buffer_layout( );

    JULSEDEBUG_BUF( "Buffer addresses: dbuf0=0x%08X, abuf0=0x%08X, dbuf1=0x%08X, abuf1=0x%08X\n\r",
                    (uint32_t)&capture_buf[ dbuf0_start ], (uint32_t)&capture_buf[ abuf0_start ],
                    (uint32_t)&capture_buf[ dbuf1_start ], (uint32_t)&capture_buf[ abuf1_start ] );

    // Calculate samples per buffer based on fixed buffer sizes
    // With fixed 8KB digital and 64KB analog buffers, determine the limiting factor
    // uint32_t digital_samples = (d_chan_cnt > 0) ? (d_size / d_dma_bps) : UINT32_MAX;
    // uint32_t analog_samples = (a_chan_cnt > 0) ? (a_size / (a_chan_cnt * 2)) : UINT32_MAX;

    // ADAPTIVE TRANSFER SIZE: Calculate sample counts based on decimation mode
    if ( use_decimation_mode ) {
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
        if ( a_chan_cnt > 0 ) {
            samples_per_half = ( base_samples / a_chan_cnt ) * a_chan_cnt; // Round down to multiple of channels
            JULSEDEBUG_BUF( "ADC CHANNEL ALIGNMENT: base_samples=%d, channels=%d, aligned_samples=%d\n\r",
                            base_samples, a_chan_cnt, samples_per_half );
        } else {
            samples_per_half = base_samples;
        }

        // Set asymmetric sample counts for consistency
        digital_samples_per_half = analog_samples_per_half = samples_per_half;

        // Optional stream mode: further reduce samples_per_half so each half can be
        // transmitted over USB before the next half completes capturing.
        // Only apply small streaming chunks for slow rates; skip at fast rates
        const uint32_t slow_threshold_hz_stream = 10000;
        if ( stream_mode && (uint32_t)sample_rate <= slow_threshold_hz_stream ) {
            // bytes per sample sent over USB (digital 7-bit chunks + analog raw)
            uint32_t bytes_per_sample = d_tx_bps + ( a_chan_cnt * 2 );
            if ( bytes_per_sample == 0 ) bytes_per_sample = 1;

            // Include a conservative constant overhead for each half-buffer transmit (USB flush, task, etc.)
            // Choose 2 ms overhead to provide headroom on FS USB.
            const uint32_t overhead_us = 4000;

            // We need minimal S such that: S/sample_rate >= overhead + (S*bytes_per_sample)/usb_estimated_bps
            // Solve for S: S * (1/sample_rate - bytes_per_sample/usb_estimated_bps) >= overhead
            // If denominator <= 0, streaming cannot keep up; choose minimal aligned chunk.
            // Use microseconds to avoid float.
            uint64_t denom_num = (uint64_t)usb_estimated_bps - (uint64_t)bytes_per_sample * (uint64_t)sample_rate; // bytes/sec - bytes/sample*samples/sec
            uint32_t target_samples = 0;
            if ( (int64_t)denom_num > 0 ) {
                // Convert inequality to integer math:
                // S >= overhead_us * sample_rate * usb_estimated_bps / (1e6 * denom_num)
                uint64_t num = (uint64_t)overhead_us * (uint64_t)sample_rate * (uint64_t)usb_estimated_bps;
                uint64_t den = 1000000ULL * denom_num;
                target_samples = (uint32_t)(( num + den - 1 ) / den); // ceil
            } else {
                // Can't keep up; pick smallest reasonable chunk to keep latency low
                target_samples = 1;
            }

            // Maintain ADC channel alignment if analog enabled
            if ( a_chan_cnt > 0 && target_samples > 0 ) {
                if ( target_samples < (uint32_t)a_chan_cnt ) target_samples = a_chan_cnt;
                target_samples = ( target_samples / a_chan_cnt ) * a_chan_cnt;
            }

            // Bound within buffer capacity
            if ( target_samples == 0 ) target_samples = 1;
            // Enforce a minimum chunk size to avoid 1-sample halves
            if ( target_samples < stream_min_samples_per_half ) {
                target_samples = stream_min_samples_per_half;
            }
            if ( target_samples < samples_per_half ) {
                JULSEDEBUG_BUF( "STREAM MODE: samples_per_half %d -> %d (bytes/sample=%d, usb_bps=%u, overhead=%uus)\n\r",
                                samples_per_half, target_samples, bytes_per_sample, usb_estimated_bps, overhead_us );
                samples_per_half = target_samples;
                digital_samples_per_half = analog_samples_per_half = samples_per_half;
            }
        }
    }

    if ( samples_per_half == UINT32_MAX || samples_per_half == 0 ) {
        JULSEDEBUG_ERR( "ERROR: No channels enabled, cannot calculate samples_per_half.\n\r" );
        return;
    }

    // Always use producer-consumer for simplicity and robustness.
    use_single_buffer = false;
    use_producer_consumer = true;

    //  Use correct transfer counts based on decimation mode
    uint32_t digital_transfers, analog_transfers;

    if ( use_decimation_mode ) {
        // In decimation mode, use asymmetric transfer counts
        digital_transfers = digital_samples_per_half * d_dma_bps;
        analog_transfers = analog_samples_per_half * a_chan_cnt; // 16-bit transfers per channel

        JULSEDEBUG_DMA( "DECIMATION TRANSFER COUNTS: digital=%d (full rate), analog=%d (base rate)\n\r",
                        digital_transfers, analog_transfers );
        JULSEDEBUG_DMA( "DECIMATION RATES: digital_rate=%d Hz, adc_base_rate=%d Hz, factor=%d\n\r",
                        sample_rate, sample_rate / analog_decimation_factor, analog_decimation_factor );
        JULSEDEBUG_DMA( "DECIMATION BUFFER CHECK: analog_samples=%d, channels=%d, buffer_bytes=%d, transfer_bytes=%d\n\r",
                        analog_samples_per_half, a_chan_cnt, a_size, analog_transfers * 2 );
    } else {
        // Normal mode: use same sample count for both
        digital_transfers = samples_per_half * d_dma_bps;
        analog_transfers = samples_per_half * a_chan_cnt; // 16-bit transfers per channel

        JULSEDEBUG_DMA( "NORMAL TRANSFER COUNTS: digital=%d, analog=%d\n\r",
                        digital_transfers, analog_transfers );
    }

    JULSEDEBUG_DMA( "TRANSFER COUNT FIX: samples_per_half=%d, digital_transfers=%d, analog_transfers=%d\n\r",
                    samples_per_half, digital_transfers, analog_transfers );

    JULSEDEBUG_DMA( "Transfer counts: digital=%d, analog=%d\n\r", digital_transfers, analog_transfers );

    // Refactored arming dispatch: choose slow/fast paths for analog and digital
    {
        const uint32_t slow_threshold_hz = 10000;
        if ( a_chan_cnt > 0 ) {
            if ( (uint32_t)sample_rate <= slow_threshold_hz ) armAnalogSlow(); else armAnalogFast();
        }
        if ( d_chan_cnt > 0 ) {
            if ( (uint32_t)sample_rate <= slow_threshold_hz ) armDigitalSlow(); else armDigitalFast();
        }
    }

    //return;

    // --- Analog Channels ---

    // --- ADC Configuration (only if analog channels enabled) ---
    // if ( a_chan_cnt > 0 ) {
    //     adc_run( false );

    //     // Initialize channel configs BEFORE configuring channels
    //     acfg0 = dma_channel_get_default_config( admachan0 );
    //     acfg1 = dma_channel_get_default_config( admachan1 );

    //     // Configure analog DMA channels
    //     channel_config_set_transfer_data_size( &acfg0, DMA_SIZE_16 ); // 16-bit to preserve 12-bit ADC resolution
    //     channel_config_set_transfer_data_size( &acfg1, DMA_SIZE_16 );
    //     channel_config_set_read_increment( &acfg0, false ); // ADC FIFO doesn't increment
    //     channel_config_set_read_increment( &acfg1, false );
    //     channel_config_set_write_increment( &acfg0, true ); // Write to buffer
    //     channel_config_set_write_increment( &acfg1, true );
    //     channel_config_set_dreq( &acfg0, DREQ_ADC ); // Use ADC data request
    //     channel_config_set_dreq( &acfg1, DREQ_ADC );

    //     // Set up chaining between the two analog DMA channels
    //     channel_config_set_chain_to( &acfg0, admachan1 ); // Analog 0 chains to Analog 1
    //     channel_config_set_chain_to( &acfg1, admachan0 ); // Analog 1 chains to Analog 0

    //     // Now configure the channels with the prepared configs
    //     // Configure but DO NOT start here; start in run()
    //     dma_channel_configure( admachan0, &acfg0, &capture_buf[ abuf0_start ], &adc_hw->fifo, analog_transfers, false );
    //     dma_channel_configure( admachan1, &acfg1, &capture_buf[ abuf1_start ], &adc_hw->fifo, analog_transfers, false );

    //     JULSEDEBUG_DMA( "Analog DMA channels configured\n\r" );
    //     // Two-stage setup like reference: first setup without enabling
    //     adc_fifo_setup( true, true, 1, false, false );
    //     adc_fifo_drain( );

    //     JULSEDEBUG_ANA( "ADC FIFO setup\n\r" );

    //     // CRITICAL SAFETY: Add ADC divisor calculation validation
    //     if ( sample_rate == 0 || a_chan_cnt == 0 ) {
    //         JULSEDEBUG_ERR( "ERROR: Invalid ADC parameters! sample_rate=%d, a_chan_cnt=%d\n\r", sample_rate, a_chan_cnt );
    //         return;
    //     }

    //     // Calculate ADC divisor with fractional part like reference
    //     uint32_t actual_adc_rate = use_decimation_mode ? ( sample_rate / analog_decimation_factor ) : sample_rate;
    //     uint32_t adcdivint = 48000000ULL / ( actual_adc_rate * a_chan_cnt );
    //     uint8_t adc_frac_int = (uint8_t)( ( ( 48000000ULL % ( actual_adc_rate * a_chan_cnt ) ) * 256ULL ) / ( actual_adc_rate * a_chan_cnt ) );

    //     JULSEDEBUG_ANA( "ADC RATE CALCULATION: requested=%d Hz, actual=%d Hz, decimation_factor=%d\n\r",
    //                     sample_rate, actual_adc_rate, analog_decimation_factor );

    //     // CRITICAL SAFETY: Validate ADC divisor
    //     if ( adcdivint == 0 || adcdivint > 65535 ) {
    //         JULSEDEBUG_ERR( "ERROR: Invalid ADC divisor! adcdivint=%d\n\r", adcdivint );
    //         adcdivint = 1;
    //     }

    //     uint8_t adc_mask = 0;
    //     /* Build ADC round-robin mask from actual enabled channels */
    //     for ( int i = 0; i < 8; i++ ) {
    //         if ( ( a_mask >> i ) & 1 ) {
    //             // CRITICAL SAFETY: Validate ADC pin number
    //             if ( 40 + i > 47 ) {
    //                 JULSEDEBUG_ERR( "ERROR: ADC pin out of range! pin=%d\n\r", 40 + i );
    //                 continue;
    //             }

    //             // adc_gpio_init(40 + i);  // Jumperless ADCs on pins 40-47
    //             adc_mask |= ( 1 << i );
    //             JULSEDEBUG_ANA( "ADC enabled on pin %d (channel %d)\n\r", 40 + i, i );
    //         }
    //     }

    //     // Safe binary print (no %b in printf)
    //     char adc_mask_binary[ 9 ];
    //     for ( int bit = 7; bit >= 0; --bit ) {
    //         adc_mask_binary[ 7 - bit ] = ( ( adc_mask >> bit ) & 0x1 ) ? '1' : '0';
    //     }
    //     adc_mask_binary[ 8 ] = '\0';
    //     JULSEDEBUG_ANA( "ADC round-robin mask: 0x%02X (binary: %s)\n\r", adc_mask, adc_mask_binary );

    //     // Calculate first enabled channel for synchronization
    //     adc_first_channel = get_first_adc_channel( adc_mask );
    //     JULSEDEBUG_ANA( "First enabled ADC channel: %d\n\r", adc_first_channel );

    //     // Prepare atomic register values for CS and FCS
    //     // Drain FIFO once before reconfig, then configure FCS/CS atomically at run() start
    //     adc_fifo_drain( );

    //     // Apply ADC divisor like reference using direct register write
    //     volatile uint32_t* adcdiv = (volatile uint32_t*)( ADC_BASE + 0x10 );

    //     // CRITICAL SAFETY: Validate ADC register pointer
    //     if ( adcdiv == nullptr ) {
    //         JULSEDEBUG_ERR( "ERROR: ADC divisor register pointer is null!\n\r" );
    //         return;
    //     }

    //     if ( adcdivint <= 96 ) {
    //         *adcdiv = 0; // Special case for high sample rates
    //     } else {
    //         *adcdiv = ( ( adcdivint - 1 ) << 8 ) | adc_frac_int;
    //     }

    //     // Use 16-bit FIFO mode for full 12-bit ADC resolution
    //     uint8_t fifo_threshold = 8; // Higher threshold to buffer multiple samples and reduce timing sensitivity
    //     // We'll apply FCS atomically at run() start
    //     JULSEDEBUG_ANA( "ADC FIFO threshold set to: %d (16-bit mode)\n\r", fifo_threshold );

    //     JULSEDEBUG_ANA( "ADC DMA config - transfer count: %d, bytes per transfer: 2 (16-bit mode)\n\r", analog_transfers );

    //     // The DMA channels are now configured in the 'Simplified DMA Configuration' block.
    //     // This old logic is no longer needed.

    //     // Critical: Drain FIFO after DMA setup to ensure clean start
    //     adc_fifo_drain( );

    //     // Add FIFO monitoring debug
    //     // JULSEDEBUG_ANA( "ADC FIFO level after drain: %d\n\r", adc_fifo_get_level( ) );
    //     // JULSEDEBUG_ANA( "ADC sample rate: %d\n\r", sample_rate );
    //     // JULSEDEBUG_ANA( " Hz, channels: %d\n\r", a_chan_cnt );
    //     // JULSEDEBUG_ANA( "effective rate per channel: %d\n\r", sample_rate * a_chan_cnt );
    //     // JULSEDEBUG_ANA( " Hz, data rate: %d\n\r", sample_rate * a_chan_cnt * 2 / 1000 );
    //     // JULSEDEBUG_ANA( " KB/s\n\r" );
    //     // JULSEDEBUG_ANA( "ADC divisor: int=%d\n\r", adcdivint );
    //     // JULSEDEBUG_ANA( "frac=%d\n\r", adc_frac_int );
    // } else {
    //     JULSEDEBUG_ANA( "No analog channels enabled - skipping ADC configuration\n\r" );
    // }

    //   }

    if ( d_chan_cnt > 0 && false) {

        // CRITICAL SAFETY: Add bounds checking before buffer operations
        if ( capture_buf == nullptr ) {
            JULSEDEBUG_ERR( "ERROR: capture_buf is null during digital setup!\n\r" );
            return;
        }

        pcfg0 = dma_channel_get_default_config( pdmachan0 );
        pcfg1 = dma_channel_get_default_config( pdmachan1 );

        // Always use PIO for first 8 GPIO pins (20-27), control channels read separately
        channel_config_set_transfer_data_size( &pcfg0, DMA_SIZE_8 );
        channel_config_set_transfer_data_size( &pcfg1, DMA_SIZE_8 );
        channel_config_set_read_increment( &pcfg0, false ); // PIO FIFO doesn't increment
        channel_config_set_read_increment( &pcfg1, false );
        channel_config_set_write_increment( &pcfg0, true ); // Write to buffer
        channel_config_set_write_increment( &pcfg1, true );
        // DREQ will be set when PIO is configured - we'll set it to use PIO RX FIFO
        JULSEDEBUG_DMA( "Digital DMA channels configured for 8 GPIO pins (PIO mode)\n\r" );

        dreq = pio_get_dreq( lapio, piosm, false ); // Use false like sigrok-pico
        channel_config_set_dreq( &pcfg0, dreq );
        channel_config_set_dreq( &pcfg1, dreq );

        channel_config_set_chain_to( &pcfg0, pdmachan1 ); // Digital 0 chains to Digital 1
        channel_config_set_chain_to( &pcfg1, pdmachan0 ); // Digital 1 chains to Digital 0

        // CRITICAL: Digital-only fast path if request fits into total digital capacity (both halves)
        uint32_t total_digital_capacity = ( d_size * 2 ) / d_dma_bps; // total samples across both halves
        JULSEDEBUG_DMA( "Digital capacity: %d\n\r", total_digital_capacity );
        // if (a_chan_cnt == 0 && num_samples > 0 && (uint32_t)num_samples <= total_digital_capacity) {
        //     JULSEDEBUG_DMA( "Digital-only fast path: %d\n\r", num_samples );
        //     single_buffer_mode = true;
        //     dma_channel_configure( pdmachan0, &pcfg0, &capture_buf[ dbuf0_start ], &pio->rxf[ piosm ], num_samples, false );
        //     // Disable secondary channel in single-transfer mode
        //     dma_channel_configure( pdmachan1, &pcfg1, NULL, NULL, 0, false );
        //     use_producer_consumer = true;
        // } else {
        JULSEDEBUG_DMA( "Normal ping-pong: %d\n\r", digital_transfers );
        // Configure but DO NOT start here; start in run()
        dma_channel_configure( pdmachan0, &pcfg0, &capture_buf[ dbuf0_start ], &lapio->rxf[ piosm ], digital_transfers, false );
        dma_channel_configure( pdmachan1, &pcfg1, &capture_buf[ dbuf1_start ], &lapio->rxf[ piosm ], digital_transfers, false );
        //   }


        // CRITICAL: Clear digital buffers (entire halves)
        memset( &capture_buf[ dbuf0_start ], 0, d_size );
        memset( &capture_buf[ dbuf1_start ], 0, d_size );
        JULSEDEBUG_DIG( "◆ Digital capture buffers cleared\n\r" );

        // Always use PIO for first 8 GPIO pins (20-27), control channels read separately
        // CRITICAL SAFETY: Add PIO validation
        if ( lapio == nullptr ) {
            JULSEDEBUG_ERR( "ERROR: PIO is null during digital setup!\n\r" );
            return;
        }

        // Debug: Show PIO state before attempting to add program
        JULSEDEBUG_CMD( "PIO %d state before program add: SM0=%s, SM1=%s, SM2=%s, SM3=%s\n\r",
                        pio_get_index( lapio ),
                        pio_sm_is_claimed( lapio, 0 ) ? "CLAIMED" : "FREE",
                        pio_sm_is_claimed( lapio, 1 ) ? "CLAIMED" : "FREE",
                        pio_sm_is_claimed( lapio, 2 ) ? "CLAIMED" : "FREE",
                        pio_sm_is_claimed( lapio, 3 ) ? "CLAIMED" : "FREE" );

        // Select PIO program based on requested sample rate
        const uint32_t slow_threshold_hz = 10000; // switch to slow program below this rate
        const struct pio_program* selected_program = ( sample_rate <= (int)slow_threshold_hz )
                                                         ? &pio_slowgram_program
                                                         : &pio_logic_analyzer_program;
        // Slow program cycles:
        // IN (1) + 6x delay(31)=6*32 + delay(30)=31 + irq_set=1 + delay(30)=31 + delay(0)=1
        // Total = 1 + 192 + 31 + 1 + 31 + 1 = 256 cycles
        const uint32_t cycles_per_sample = ( selected_program == &pio_slowgram_program ) ? 32u : 1u;

        // Auto-enable stream mode for slow rates (pairs well with slow PIO)
        if ( selected_program == &pio_slowgram_program ) {
            stream_mode = true;
            // Ensure ADC runs at full speed for single-shot conversions so DMA sees each sample promptly
            adc_set_clkdiv( 1.0f );
            JULSEDEBUG_CMD( "ADC clkdiv forced to 1.0 for slow mode single-shot conversions\n\r" );

            int enabled_idx = 0;
            for ( uint8_t i = 0; i < 8; i++ ) {
                if ( ( a_mask >> i ) & 1 ) {
                    ana_enabled_list[ enabled_idx++ ] = i;
                } 
            }
            ana_enabled_count = enabled_idx;
        }

        JULSEDEBUG_CMD( "PIO program selection: %s (cycles/sample=%u) for sample_rate=%d Hz, stream_mode=%d\n\r",
                        ( selected_program == &pio_slowgram_program ) ? "slow" : "fast",
                        cycles_per_sample, sample_rate, (int)stream_mode );

        // Check if we can add the selected PIO program before attempting to add it
        if ( !pio_can_add_program( lapio, selected_program ) ) {
            // JULSEDEBUG_DIG( "ERROR: Cannot add selected PIO program - instruction memory full or program already loaded!\n\r" );
            // JULSEDEBUG_DIG( "PIO %d instruction memory may be full or program already present\n\r", pio_get_index( pio ) );

            // Try to remove the program first and then re-add it
            JULSEDEBUG_CMD( "Attempting to remove and re-add selected PIO program...\n\r" );
            pio_remove_program( lapio, selected_program, 0 );

            // Check again if we can add it now
            if ( !pio_can_add_program( lapio, selected_program ) ) {
                JULSEDEBUG_ERR( "ERROR: Still cannot add selected PIO program after removal!\n\r" );
                return;
            }
            JULSEDEBUG_CMD( "Successfully removed and can re-add selected PIO program\n\r" );
        }

        // pio_sm_set_enabled(lapio, piosm, false);
        // pio_sm_clear_fifos(lapio, piosm);
        // pio_sm_restart(lapio, piosm);

        uint offset = pio_add_program( lapio, selected_program );
        if ( offset == (uint)-1 ) {
            JULSEDEBUG_ERR( "ERROR: Failed to add selected PIO program despite can_add check!\n\r" );
            return;
        }
        JULSEDEBUG_DIG( "PIO %d program added (offset=%u, length=%u)\n\r", pio_get_index( lapio ), offset, selected_program->length );
        pio_sm_config c = pio_get_default_sm_config( );

        sm_config_set_in_pins( &c, 20 ); // Start at GPIO 20

        // Always configure PIO for 8 pins since that's what we're actually reading
        // Control channels are added in software
        sm_config_set_in_pin_count( &c, 8 );
        sm_config_set_in_shift( &c, false, true, 8 ); // 8-bit shift, autopush enabled for 8 channels
        JULSEDEBUG_DIG( "PIO configured for 8 GPIO pins (control channels added in software)\n\r" );

        sm_config_set_fifo_join( &c, PIO_FIFO_JOIN_RX );

        // Set wrap to loop over the entire selected program
        sm_config_set_wrap( &c, offset, offset + selected_program->length - 1 );

        // Clock divider calculation accounts for instruction cycles per sample
        uint32_t sys_clock = clock_get_hz( clk_sys );

        JULSEDEBUG_CMD( "Clock divider calculation: sys_clock=%d, sample_rate=%d, cycles_per_sample=%u, offset=%d, length=%u\n\r",
                        sys_clock, sample_rate, cycles_per_sample, offset, selected_program->length );

        float clock_div = (float)sys_clock / ( (float)sample_rate * (float)cycles_per_sample );


        JULSEDEBUG_CMD( "Clock divider calculation: clock_div=%.2f\n\r", clock_div );

        if ( clock_div < 1.0f ) {
            JULSEDEBUG_ERR( "Clock divider too low, setting to 1\n\r" );
            clock_div = 1.0f;
        }
        if ( clock_div > 65536.0f ) {
            JULSEDEBUG_ERR( "Clock divider too high, setting to 65536\n\r" );
            clock_div = 65536.0f;
        }

        sm_config_set_clkdiv( &c, clock_div );

        // Debug: Log PIO configuration details
        JULSEDEBUG_CMD( "PIO CONFIG DEBUG: sample_rate=%d, sys_clock=%d, cycles/sample=%u, clock_div=%.2f\n\r",
                        sample_rate, sys_clock, cycles_per_sample, clock_div );
        JULSEDEBUG_CMD( "PIO CONFIG DEBUG: trigger_enabled=%d, pre_trigger_samples=%d, post_trigger_samples=%d\n\r",
                        trigger_config.enabled, pre_trigger_samples, post_trigger_samples );

        JULSEDEBUG_STA( "ARM: About to init PIO SM %d on PIO1\n\r", piosm );
        pio_sm_init( lapio, piosm, offset, &c );

        // Enable CPU interrupt for the exact PIO IRQ flag raised by the program (absolute IRQ 1)
        pio_interrupt_clear( lapio, 1 );
        pio_set_irq1_source_enabled( lapio, pis_interrupt1, true );


        
        // Ensure GPIOs are initialized for PIO and configured as inputs for sampling
        // // Follow CH446Q pattern by explicitly claiming pins for PIO block
        // for ( uint gpio = 20; gpio < 28; ++gpio ) {
        //     pio_gpio_init( lapio, gpio );
        // }
        // pio_sm_set_consecutive_pindirs( lapio, piosm, 20, 8, false );

        // Bind handler to PIO1 IRQ1 and enable it
        JULSEDEBUG_CMD( "Binding PIO1 IRQ1 to julseview handler (CH446Q style)\n\r" );

        if ( !irq_has_shared_handler( PIO1_IRQ_1 ) ) {
            irq_add_shared_handler( PIO1_IRQ_1, julseview_pio_irq0_handler, 0 );
            irq_set_enabled( PIO1_IRQ_1, true );
        }


        JULSEDEBUG_CMD( "IRQ setup verification: PIO1 IRQ1=%d enabled=%d\n\r",
                        PIO1_IRQ_1, irq_is_enabled( PIO1_IRQ_1 ) );
        JULSEDEBUG_CMD( "PIO1 INTE1 mask: 0x%08X\n\r", pio1_hw->inte1 );

        // Verify PIO state machine was initialized correctly
        JULSEDEBUG_STA( "ARM: Checking if PIO SM %d is claimed\n\r", piosm );
        if ( !pio_sm_is_claimed( lapio, piosm ) ) {
            JULSEDEBUG_ERR( "ERROR: PIO state machine %d not claimed after initialization!\n\r", piosm );
            return;
        }

        JULSEDEBUG_CMD( "PIO program loaded successfully at offset %d on PIO %d, SM %d\n\r", offset, pio_get_index( lapio ), piosm );

        // CRITICAL SAFETY: Add DMA configuration validation
        if ( pdmachan0 < 0 || pdmachan1 < 0 ) {
            JULSEDEBUG_ERR( "ERROR: Digital DMA channels not claimed! pdmachan0=%d, pdmachan1=%d\n\r", pdmachan0, pdmachan1 );
            return;
        }

        // Configure DMA for digital capture - always use 8-bit transfers since PIO only provides 8 bits
        // We'll combine with control channels in software
        channel_config_set_transfer_data_size( &pcfg0, DMA_SIZE_8 );
        channel_config_set_transfer_data_size( &pcfg1, DMA_SIZE_8 );
        JULSEDEBUG_DIG( "Digital DMA configured for 8-bit transfers (PIO provides 8 bits, control channels added in software)\n\r" );

        channel_config_set_read_increment( &pcfg0, false ); // PIO FIFO doesn't increment
        channel_config_set_read_increment( &pcfg1, false );

        channel_config_set_write_increment( &pcfg0, true ); // Write to buffer
        channel_config_set_write_increment( &pcfg1, true );

        dreq = pio_get_dreq( lapio, piosm, false );
        if ( dreq == -1 ) {
            JULSEDEBUG_ERR( "ERROR: Failed to get PIO DREQ!\n\r" );
            return;
        }

        channel_config_set_dreq( &pcfg0, dreq ); // Use PIO RX FIFO
        channel_config_set_dreq( &pcfg1, dreq );

        JULSEDEBUG_DIG( "PIO configured for %d channels, sample rate: %d Hz\n\r", d_chan_cnt, sample_rate );
        JULSEDEBUG_DIG( "PIO clock divider: %.2f (sys_clock=%d Hz)\n\r", clock_div, sys_clock );
        JULSEDEBUG_DIG( "Digital DMA configured for 8-bit transfers, buffer size: %d bytes\n\r", d_size );
        JULSEDEBUG_DIG( "Digital DMA DREQ configured: %d (PIO RX FIFO)\n\r", pio_get_dreq( lapio, piosm, false ) );

        // The DMA channels are now configured in the 'Simplified DMA Configuration' block.
        // This old logic is no longer needed.

        // CRITICAL DEBUG: Verify PIO and DMA configuration match
        JULSEDEBUG_DIG( "Digital configuration verification:\n\r" );
        JULSEDEBUG_DIG( "  Channels: %d, d_dma_bps: %d, PIO autopush: 8 bits\n\r",
                        d_chan_cnt, d_dma_bps );
        JULSEDEBUG_DIG( "  DMA transfer size: 8-bit (PIO provides 8 bits, control channels added in software)\n\r" );

        // CRITICAL CHECK: Verify DMA and PIO configurations are compatible
        uint8_t expected_dma_bps = 1;  // Always 1 byte since PIO provides 8 bits
        uint8_t expected_pio_bits = 8; // Always 8 bits from PIO

        if ( d_dma_bps != expected_dma_bps ) {
            JULSEDEBUG_DIG( "WARNING: DMA bytes per sample (%d) doesn't match setup!\n\r", d_dma_bps );
            JULSEDEBUG_DIG( "Expected: %d bytes for 8-bit PIO autopush\n\r", expected_dma_bps );
        } else {
            JULSEDEBUG_DIG( "✓ DMA and PIO configurations match (8-bit PIO + separate control DMA)\n\r" );
        }
    }

    // Final FIFO cleanup before starting capture
    if ( a_mask ) {
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

    // Setup control channel DMA if needed
    if ( d_chan_cnt > 8 || c_mask != 0 ) {
        setup_control_channel_dma( );
    }

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

    // Setup and start heartbeat watchdog for crash recovery
    //    setup_heartbeat_watchdog();

    //    start_heartbeat_watchdog();
    heartbeat_enabled = 1;
    // Reset completion signal flag for new capture
    completion_signal_sent = false;

    // Prepare analog capture if enabled (but don't start yet in trigger mode)
    adc_fifo_drain( );

    watchdog_enable( calculate_expected_capture_time( ) * 2, true );

    JULSEDEBUG_ERR( "Watchdog enabled for %d ms\n\r", calculate_expected_capture_time( ) * 2 );
    // Initialize current DMA half tracking
    current_dma_half = 0; // Start with half 0

    pio_sm_set_enabled( lapio, piosm, true );

    adc_fifo_setup( false, false, 1, false, false );

    // Build FCS mask: EN (write results), DREQ_EN, THRESH=1 sample, clear sticky OVER/UNDER
    uint32_t fcs_mask = 0;
    fcs_mask |= ADC_FCS_EN_BITS;                                             // write results to FIFO
    fcs_mask |= ADC_FCS_DREQ_EN_BITS;                                        // DMA requests when data present
    fcs_mask |= ( (uint32_t)1 << ADC_FCS_THRESH_LSB ) & ADC_FCS_THRESH_BITS; // threshold = 1
    fcs_mask |= ADC_FCS_OVER_BITS | ADC_FCS_UNDER_BITS;                      // write-1-to-clear sticky flags

    // Build CS mask: EN, AINSEL = first enabled
    // In stream_mode: disable round-robin and START_MANY (ISR issues START_ONCE per channel)
    // Otherwise: enable round-robin across enabled channels and START_MANY
    uint32_t cs_mask = 0;
    cs_mask |= ADC_CS_EN_BITS;
    cs_mask |= ( ( (uint32_t)get_first_adc_channel( a_mask ) << ADC_CS_AINSEL_LSB ) & ADC_CS_AINSEL_BITS );
    if ( !stream_mode ) {
        cs_mask |= ( ( (uint32_t)a_mask << ADC_CS_RROBIN_LSB ) & ADC_CS_RROBIN_BITS );
        cs_mask |= ADC_CS_START_MANY_BITS;
    }

    // Start analog DMA after FCS/CS are programmed

    if ( a_chan_cnt > 0 ) {
        dma_channel_start( admachan0 );
        delayMicroseconds( 1000 );
    }
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
    if ( d_chan_cnt > 0 ) {
        pio_sm_clear_fifos( lapio, piosm );
        dma_channel_start( pdmachan0 );
        // JULSEDEBUG_CMD("Digital DMA STARTED: %s mode\n\r", trigger_config.enabled ? "Post-trigger" : "Normal");
    }
    if ( a_chan_cnt > 0 ) {

        // FIFO control and status
        // 0x0f000000 [27:24] THRESH       (0x0) DREQ/IRQ asserted when level >= threshold
        // 0x000f0000 [19:16] LEVEL        (0x0) The number of conversion results currently waiting in the FIFO
        // 0x00000800 [11]    OVER         (0) 1 if the FIFO has been overflowed
        // 0x00000400 [10]    UNDER        (0) 1 if the FIFO has been underflowed
        // 0x00000200 [9]     FULL         (0)
        // 0x00000100 [8]     EMPTY        (0)
        // 0x00000008 [3]     DREQ_EN      (0) If 1: assert DMA requests when FIFO contains data
        // 0x00000004 [2]     ERR          (0) If 1: conversion error bit appears in the FIFO alongside...
        // 0x00000002 [1]     SHIFT        (0) If 1: FIFO results are right-shifted to be one byte in size
        // 0x00000001 [0]     EN           (0) If 1: write result to the FIFO after each conversion

        // ADC Control and Status
        // 0x01ff0000 [24:16] RROBIN       (0x000) Round-robin sampling
        // 0x0000f000 [15:12] AINSEL       (0x0) Select analog mux input
        // 0x00000400 [10]    ERR_STICKY   (0) Some past ADC conversion encountered an error
        // 0x00000200 [9]     ERR          (0) The most recent ADC conversion encountered an error;...
        // 0x00000100 [8]     READY        (0) 1 if the ADC is ready to start a new conversion
        // 0x00000008 [3]     START_MANY   (0) Continuously perform conversions whilst this bit is 1
        // 0x00000004 [2]     START_ONCE   (0) Start a single conversion
        // 0x00000002 [1]     TS_EN        (0) Power on temperature sensor
        // 0x00000001 [0]     EN           (0) Power on ADC and enable its clock

        adc_fifo_drain( );
        adc_hw->fcs = fcs_mask;
        adc_hw->cs = cs_mask;

        // adc_fifo_drain( );
        // adc_select_input( get_first_adc_channel( a_mask ) );

        // adc_fifo_setup( true, true, 1, false, false );

        // adc_hw->cs |= ADC_CS_START_MANY_BITS;
    }

    delayMicroseconds( 1000 );
    //  Main capture loop (simplified - no trigger monitoring here)

    // JULSEDEBUG_CMD( "Starting main capture loop julseview_active= %s scnt= %d num_samples= %d\n\r", julseview_active ? "true" : "false", scnt, num_samples );
    while ( julseview_active && scnt < num_samples ) {

        // Check if we've reached the sample limit
        // if ( scnt >= num_samples ) {
        //     JULSEDEBUG_CMD( "Sample limit reached (%d/%d) - breaking capture loop\n\r", scnt, num_samples );
        //     break; // Exit the loop
        // }

        // if ( USBSer2.available( ) ) {
        //     JULSEDEBUG_CMD( "USBSer2.available()\n\r" );
        //     if ( USBSer2.read( ) == '+' ) {
        //         JULSEDEBUG_CMD( "+ received\n\r" );
        //         break;
        //     }
        // }
        // tud_task();

        dma_check( );

        if ( scnt % 32 == 0 && scnt > 100 ) {
            uint8_t c = 0;
            if ( tud_cdc_n_peek( 2, &c ) == 1 && c == '+' ) {
                JULSEDEBUG_ERR( "Stopping capture with signal\n\r" );
                // watchdog_update();
                break;
            }
        }

        // if ( millis( ) - last_debug_time > 200 ) {
        //     if ( USBSer2.available( ) ) {
        //         // process_char(USBSer2.read());

        //         JULSEDEBUG_CMD( "USBSer2.available()\n\r" );
        //     }
        //     last_debug_time = millis( );

        //     JULSEDEBUG_DIG( "Capture loop - samples: %d/%d, loops: %d\n\r", scnt, num_samples, loop_count );
        //     JULSEDEBUG_CMD( "  Current DMA half: %d\n\r", current_dma_half );
        //     JULSEDEBUG_CMD( "  Trigger monitoring: enabled=%d, detected=%d\n\r", trigger_config.enabled, trigger_detected );
        //     JULSEDEBUG_CMD( "  PIO FIFO Level: %d\n\r", pio_sm_get_rx_fifo_level( lapio, piosm ) );
        //     // JULSEDEBUG_CMD( "  DMA-A0 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( admachan0 ) ? "Y" : "N", dma_channel_hw_addr( admachan0 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-A1 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( admachan1 ) ? "Y" : "N", dma_channel_hw_addr( admachan1 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-D0 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( pdmachan0 ) ? "Y" : "N", dma_channel_hw_addr( pdmachan0 )->write_addr );
        //     // JULSEDEBUG_CMD( "  DMA-D1 busy: %s, Addr: 0x%x\n\r", dma_channel_is_busy( pdmachan1 ) ? "Y" : "N", dma_channel_hw_addr( pdmachan1 )->write_addr );
        // }
    }
    heartbeat_enabled = 0;

    // CRITICAL FINAL FLUSH: Ensure any remaining data is flushed before completion signal
    JULSEDEBUG_CMD( "Final run() flush check - txbufidx: %d bytes\n\r", txbufidx );
    if ( txbufidx > 0 ) {
        check_tx_buf( 1 ); // Force flush any remaining data
        JULSEDEBUG_CMD( "Final run() flush completed - txbufidx now: %d bytes\n\r", txbufidx );
    }

    // Send final completion signal if not already sent
    send_capture_completion_signal( false );

    // Stop the heartbeat watchdog
    // stop_heartbeat_watchdog();

    // ENHANCED SAFETY: Longer delay to ensure USB transmission completes after final flush
    delayMicroseconds( 15000 ); // 15ms delay to ensure USB transmission completes after aggressive flushing

    // deinit( );
    end( );
    JULSEDEBUG_STA( "=== JULSEVIEW RUN() COMPLETE ===\n\r" );
}

// Remove timer-based control channel update mechanism (replaced by DMA paced by PIO DREQ)

void julseview::dma_check( ) {
    //  Implement sigrok-pico DMA chaining approach
    // This prevents infinite loops and ensures proper synchronization

    // Determine which channels to check based on current half
    uint admachan_current = ( current_dma_half == 0 ) ? admachan0 : admachan1;
    uint pdmachan_current = ( current_dma_half == 0 ) ? pdmachan0 : pdmachan1;

    // Check current DMA completion
    bool analog_complete = ( a_chan_cnt == 0 ) || !dma_channel_is_busy( admachan_current );
    bool digital_complete = ( d_chan_cnt == 0 ) || !dma_channel_is_busy( pdmachan_current );

    // // In digital-only mode, rely solely on digital completion to avoid handoff races
    if ( a_chan_cnt == 0 ) {
        analog_complete = true;
    }

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

        // SIMPLIFIED DMA CHAINING: Don't break chaining during data processing
        // Instead, just reset addresses and let the other channel continue running
        // This prevents PIO FIFO overflow and chaining failures

        if ( a_chan_cnt > 0 ) {
            // Reset the address for the completed channel
            *completed_addr_a = reset_addr_a;
        }

        if ( d_chan_cnt > 0 ) {
            // Reset the address for the completed channel
            *completed_addr_d = reset_addr_d;
        }

        // Step 2: Process data from completed half (can take time now)
        uint8_t* dbuf = ( d_chan_cnt == 0 ) ? nullptr : &capture_buf[ completed_half == 0 ? dbuf0_start : dbuf1_start ];
        uint8_t* abuf = ( a_chan_cnt == 0 ) ? nullptr : &capture_buf[ completed_half == 0 ? abuf0_start : abuf1_start ];

        // Trigger detection is now handled by direct GPIO polling in the main loop
        // No need to check triggers here anymore - just process data normally

        // CRITICAL: Check if we've already reached the sample limit before processing
        // if ( scnt >= num_samples ) {
        //     JULSEDEBUG_CMD( "Sample limit already reached (%d/%d) - skipping data processing\n\r", scnt, num_samples );
        //     // Still switch to next half to keep DMA running
        //     current_dma_half = 1 - current_dma_half;
        // } else {
        //     // static uint32_t buffer_cycle_count = 0;
        //     // buffer_cycle_count++;
        //     // JULSEDEBUG_DAT( "BUFFER CYCLE #%d: Processing DMA half %d (scnt=%d/%d)\n\r",
        //     //                buffer_cycle_count, current_dma_half, scnt, num_samples );

        uint64_t start_time = micros( );
        send_slices_analog( dbuf, abuf );
        uint64_t end_time = micros( );
        // }

        int64_t time_to_capture = ( samples_per_half * ( 1.0f / (float)sample_rate ) );

        // JULSEDEBUG_DAT("samples_per_half: %d x sample_rate: %d = %d us\n\r", samples_per_half, sample_rate, time_to_capture);
        // JULSEDEBUG_DAT("start_time: %d us, end_time: %d us\n\r", start_time, end_time);

        int64_t time_to_send = end_time - start_time;
        // JULSEDEBUG_DAT("Time to capture: %d us, time to send: %d us\n\r", time_to_capture, time_to_send);
    }
}

void julseview::end( ) {
    JULSEDEBUG_STA( "JulseView end() - stopping operations and cleaning up resources\n\r" );

    // CRITICAL SAFETY: Set active flag to false immediately to prevent new operations
    julseview_active = false;

    // SAFETY: Prevent multiple calls to end() which can cause hangs
    if ( dma_ended_flag ) {
        JULSEDEBUG_DMA( "END: Already called, skipping to prevent hang\n\r" );
        return;
    }
    dma_ended_flag = true;

    // === STEP 1: STOP ALL DMA OPERATIONS ===
    JULSEDEBUG_DMA( "END: Breaking ping-pong chaining and stopping DMA channels\n\r" );

    // Break ping-pong chaining by reconfiguring channels
    if ( a_chan_cnt > 0 ) {
        dma_channel_configure( admachan0, &acfg0, NULL, NULL, 0, false );
        dma_channel_configure( admachan1, &acfg1, NULL, NULL, 0, false );
        JULSEDEBUG_DMA( "END: Analog DMA chaining broken\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        dma_channel_configure( pdmachan0, &pcfg0, NULL, NULL, 0, false );
        dma_channel_configure( pdmachan1, &pcfg1, NULL, NULL, 0, false );
        JULSEDEBUG_DMA( "END: Digital DMA chaining broken\n\r" );
    }

    // Stop control DMA channels if active
    if ( control_dma_chan0 >= 0 ) {
        dma_channel_abort( control_dma_chan0 );
    }
    if ( control_dma_chan1 >= 0 ) {
        dma_channel_abort( control_dma_chan1 );
    }

    // Reset DMA write addresses to beginning of buffers
    if ( a_chan_cnt > 0 ) {
        *taddra0 = (uint32_t)&capture_buf[ abuf0_start ];
        *taddra1 = (uint32_t)&capture_buf[ abuf1_start ];
        JULSEDEBUG_DMA( "END: Analog DMA addresses reset to buffer start\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        *taddrd0 = (uint32_t)&capture_buf[ dbuf0_start ];
        *taddrd1 = (uint32_t)&capture_buf[ dbuf1_start ];
        JULSEDEBUG_DMA( "END: Digital DMA addresses reset to buffer start\n\r" );
    }

    // Abort all DMA channels
    if ( a_chan_cnt > 0 ) {
        dma_channel_abort( admachan0 );
        dma_channel_abort( admachan1 );
        JULSEDEBUG_DMA( "END: Analog DMA channels stopped\n\r" );
    }

    if ( d_chan_cnt > 0 ) {
        dma_channel_abort( pdmachan0 );
        dma_channel_abort( pdmachan1 );
        JULSEDEBUG_DMA( "END: Digital DMA channels stopped\n\r" );
    }

    // Wait for all channels to actually stop
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
        JULSEDEBUG_ERR( "END: WARNING - DMA channels did not stop within timeout\n\r" );
    } else {
        JULSEDEBUG_DMA( "END: All DMA channels stopped successfully\n\r" );
    }

    // === STEP 2: STOP HARDWARE OPERATIONS ===
    // Stop ADC and PIO operations
    pio_sm_set_clkdiv( lapio, piosm, 1.0 );

    // Clean up PIO resources to prevent conflicts with CH446Q and rotary encoder
    // if ( pio != nullptr && d_chan_cnt > 0 ) {
    //     JULSEDEBUG_STA( "END: Cleaning up PIO1 resources...\n\r" );

    //     // Disable the state machine first
    //     pio_sm_set_enabled( lapio, piosm, false );

    //     // Clear FIFOs to ensure clean state
    //     pio_sm_clear_fifos( lapio, piosm );

    //     // Remove the program
    //     pio_remove_program( lapio, &pio_logic_analyzer_program, 0 );

    //     // CRITICAL: Unclaim the state machine to prevent interference with other PIO users
    //     pio_sm_unclaim( lapio, piosm );
    //     piosm = -1; // Mark as unclaimed

    //     JULSEDEBUG_STA( "END: PIO program removed and state machine unclaimed from PIO1\n\r" );
    // }

    // === STEP 3: CLEAN UP DMA CHANNELS ===
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
    if ( control_dma_chan0 >= 0 ) {
        dma_channel_unclaim( control_dma_chan0 );
        control_dma_chan0 = -1;
    }
    if ( control_dma_chan1 >= 0 ) {
        dma_channel_unclaim( control_dma_chan1 );
        control_dma_chan1 = -1;
    }

    // Clear DMA register pointers
    taddra0 = taddra1 = taddrd0 = taddrd1 = nullptr;
    tstsa0 = tstsa1 = tstsd0 = tstsd1 = nullptr;

    JULSEDEBUG_DMA( "END: DMA channels cleaned up\n\r" );

    // === STEP 4: CLEAN UP CONTROL CHANNELS ===
    if ( d_chan_cnt == 16 && c_mask != 0 ) {
        c_mask = 0;
        c_chan_cnt = 0;
        JULSEDEBUG_DIG( "END: Control channel resources cleaned up\n\r" );
    }

    adc_fifo_setup( false, false, 0, false, false );
    adc_fifo_drain( );

    // Set Arduino ADC resolution back to 12 bits for compatibility
    // analogReadResolution( 12 );
    adc_set_clkdiv( 1.0 );
    adc_select_input( 0 );
    adc_set_round_robin( 0 );
    adc_run( false );

    // Re-enable ADC GPIO pins for normal operation
    for ( int i = 0; i < 8; i++ ) {
        adc_gpio_init( 40 + i ); // Jumperless ADCs on pins 40-47
    }

    // CRITICAL: Restore rotary encoder pullups that may have been disabled
    // This fixes the issue where encoder becomes erratic after logic capture
    // gpio_set_pulls( 12, true, false ); // QUADRATURE_A_PIN pullup
    // gpio_set_pulls( 13, true, false ); // QUADRATURE_B_PIN pullup
    // JULSEDEBUG_STA( "DEINIT: Rotary encoder pullups restored on pins 12 and 13\n\r" );

    // Verify ADC restoration with test readings
    for ( int i = 0; i < 8; i++ ) {
        float voltage = readAdcVoltage( i, 32 );
        adc_select_input( i );
        int raw_reading = adc_read( );
        JULSEDEBUG_ANA( "DEINIT: ADC pin %d restored - voltage: %f, raw: %d\n\r", 40 + i, voltage, raw_reading );
    }

    // === STEP 5: RESET STATE FLAGS ===
    sending = false;
    cont = false;
    aborted = false;
    started = false;
    armed = false;
    isArmed = false;
    isRunning = false;
    isTriggered = false;
    receivedCommand = false;

    // Reset trigger state
    trigger_armed = false;
    trigger_detected = false;
    trigger_config.enabled = false;

    // Reset ping-pong buffer state
    current_dma_half = 0;

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

    // Disable watchdog
    watchdog_disable( );

    JULSEDEBUG_STA( "END: Device state reset - operations stopped, resources cleaned, memory preserved\n\r" );
}

void julseview::deinit( ) {
    JULSEDEBUG_STA( "JulseView deinit() - complete teardown and restore system state\n\r" );

    // === STEP 1: STOP ALL OPERATIONS ===
    // Call end() to handle all operational cleanup (DMA, PIO, state reset)
    end( );

    // === STEP 2: FREE ALL MEMORY BUFFERS ===
    // Free capture buffers and restore memory to system
    if ( capture_buf != nullptr ) {
        // Check memory guard for buffer overflow before freeing
        if ( actual_buffer_size > 0 ) {
            uint32_t guard_value;
            // Guard is placed at raw_buf + actual_buffer_size - 4 (same as total_allocation in init)
            if ( actual_buffer_size >= 4 ) {
                memcpy( &guard_value, (uint8_t*)capture_buf + actual_buffer_size - 4, 4 );
                if ( guard_value != 0xDEADBEEF ) {
                    JULSEDEBUG_ERR( "DEINIT: WARNING - Memory guard corrupted! Expected 0xDEADBEEF, got 0x%08X\n\r", guard_value );
                } else {
                    JULSEDEBUG_BUF( "DEINIT: Memory guard intact - no buffer overflow detected\n\r" );
                }
            }
        }

        // Free the raw buffer
        free( capture_buf );
        // raw_capture_buf = nullptr;
        capture_buf = nullptr;
        actual_buffer_size = 0;

        JULSEDEBUG_BUF( "DEINIT: Capture buffers freed and memory returned to system\n\r" );

        // Show memory status after freeing
        JULSEDEBUG_BUF( "DEINIT: Free memory after buffer deallocation: %d bytes\n\r", rp2040.getFreeHeap( ) );
    } else {
        JULSEDEBUG_BUF( "DEINIT: No capture buffers to free\n\r" );
    }

    // === STEP 3: RESTORE ADC TO NORMAL OPERATION ===
    JULSEDEBUG_ANA( "DEINIT: Restoring ADC to normal operation\n\r" );

    // Re-initialize ADC to restore normal operation
    // adc_init( );

    // === STEP 4: RESTORE SYSTEM SETTINGS ===
    // Restore bus priority to normal to prevent interference with CH446Q PIO operations
    bus_ctrl_hw->priority = 0; // Reset to default priority (CPU priority)
    JULSEDEBUG_STA( "DEINIT: Bus priority restored to normal\n\r" );

    // Reset initialization flag
    initialized = false;
    dma_ended_flag = false; // Reset for next initialization

    // === STEP 5: FINAL STATE RESET ===
    // Call reset() to ensure all state variables are properly initialized
    reset( );

    JULSEDEBUG_STA( "DEINIT: Complete teardown finished - system restored to original state\n\r" );
    JULSEDEBUG_STA( "DEINIT: Memory freed, hardware restored, ready for reinitialization\n\r" );
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

        end( );
        return false;
    } else if ( ( charin == '\r' ) || ( charin == '\n' ) ) {
        cmdstr[ cmdstrptr ] = 0;

        switch ( cmdstr[ 0 ] ) {
        case 'i':
            if ( !initialized ) {
                JULSEDEBUG_CMD( "JulseView ID command received - initializing\n\r" );
                julseview::init( );
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
                configure_decimation_mode( );

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

                if ( !initialized ) {
                    JULSEDEBUG_CMD( "JulseView ID command received - initializing\n\r" );
                    julseview::init( );
                }
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

            // if ( !initialized ) {
            //     JULSEDEBUG_CMD( "JulseView ID command received - initializing\n\r" );
            //     julseview::init( );
            // }

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
    // IMPROVED: More conservative buffer overflow checking
    // Use smaller thresholds to prevent buffer overflow
    uint32_t threshold = use_decimation_mode ? ( JULSEVIEW_TX_BUF_SIZE / 2 ) : ( JULSEVIEW_TX_BUF_SIZE * 2 / 3 );
    if ( txbufidx + d_tx_bps >= threshold ) {
        // Buffer is getting full, force a transmission
        check_tx_buf( 1 ); // Force transmission of any data
    }

    for ( char b = 0; b < d_tx_bps; b++ ) {
        uint8_t data_byte = ( cval & 0x7F ) + 0x30; // Ensure >= 0x30
        if ( data_byte < 0x80 )
            data_byte |= 0x80; // Set high bit for digital data
        txbuf[ txbufidx++ ] = data_byte;
        cval >>= 7;
    }
}

// Get current value from buffer - always 8 bits from PIO
uint32_t julseview::get_cval( uint8_t* dbuf ) {
    uint32_t cval = 0;

    // Always read 1 byte from PIO buffer
    cval = dbuf[ rxbufdidx ];

    // Mask to keep only the lower 8 bits (GPIO data)
    cval &= 0xFF;

    // Only show debug for first few samples to avoid spam
    // if ((rxbufdidx / d_dma_bps) < 5) {
    //     JULSEDEBUG_CMD("DEBUG: get_cval[%d] = 0x%08X\n\r", rxbufdidx / d_dma_bps, cval);
    //     JULSEDEBUG_CMD(" from addr 0x%08X\n\r", (uint32_t)(dbuf + rxbufdidx));
    // }

    rxbufdidx += 1; // Advance by 1 byte
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

// Send txbuf to USB based on threshold - STREAMING OPTIMIZED VERSION
void julseview::check_tx_buf( uint16_t cnt ) {
    if ( txbufidx >= cnt ) {
        // Check if we have data to transmit
        if ( txbufidx > 0 ) {
            // STREAMING-OPTIMIZED TRANSMISSION: Minimal retries, fast recovery
            bool success = this->julseview_usb_out_chars( (char*)txbuf, txbufidx );

            if ( success ) {
                // Success - update counters and reset buffer
                byte_cnt += txbufidx;
                txbufidx = 0;
            } else {
                // STREAMING FAILURE RECOVERY: Check USB buffer status and adjust strategy
                int usb_available = tud_cdc_n_write_available( 2 );

                if ( usb_available == 0 ) {
                    // Buffer completely full - need aggressive intervention
                    JULSEDEBUG_USB( "USB buffer completely full - implementing emergency flow control\n\r" );

                    // Emergency flow control: Multiple USB processing cycles with delays
                    for ( int i = 0; i < 5; i++ ) {
                        tud_task( );
                        delayMicroseconds( 100 ); // Significant delay to let USB catch up
                        usb_available = tud_cdc_n_write_available( 2 );
                        if ( usb_available > 512 )
                            break; // Some space freed up
                    }
                }

                tud_task( ); // Process USB events to clear any bottlenecks
                // delayMicroseconds( 5000 ); // Longer delay for persistent issues
                // tud_cdc_n_write_flush(2);

                success = this->julseview_usb_out_chars( (char*)txbuf, txbufidx );

                if ( success ) {
                    // Second attempt succeeded
                    byte_cnt += txbufidx;
                    txbufidx = 0;
                    JULSEDEBUG_USB( "USB transmission recovered on second attempt\n\r" );
                } else {
                    // PERSISTENT FAILURE: Implement data rate throttling
                    JULSEDEBUG_USB( "USB transmission failed - implementing rate limiting\n\r" );

                    // Add significant delay to throttle data production rate
                    delayMicroseconds( 200 ); // Slow down overall data rate
                    tud_task( );

                    // For streaming applications, it's better to continue with some data loss
                    // than to completely block the stream. Reset buffer to continue.
                    byte_cnt += txbufidx; // Count as sent (even if partial) to maintain flow
                    txbufidx = 0;
                }
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

#define PRINT_IN_TRANSMIT 0
#define PRINT_DATA_SAMPLES 0 // !Set to 1 to enable sample data printing

// Send mixed analog and digital data - FIRMWARE DECIMATION VERSION
void julseview::send_slices_analog( uint8_t* dbuf, uint8_t* abuf ) {
    // Initialize transmission state
    rxbufdidx = 0;
    txbufidx = 0;

    // FIRMWARE DECIMATION: Reset static variables for new capture
    // This ensures we start fresh and don't carry over old analog values
    reset_firmware_decimation_state( );

    // Calculate and validate sample count
    uint32_t current_samples = calculate_sample_count( );
    validate_buffer_bounds( current_samples );

#if PRINT_IN_TRANSMIT
    JULSEDEBUG_DAT( "JulseView sending analog data - samples: %d, a_chan_cnt: %d, d_chan_cnt: %d\n\r",
                    samp_remain, a_chan_cnt, d_chan_cnt );
    JULSEDEBUG_ANA( "=== ANALOG DATA TRANSMISSION STARTING ===\n\r" );
    JULSEDEBUG_ANA( "Data format: Each sample = %d digital bytes + %d analog bytes\n\r",
                    d_tx_bps, a_chan_cnt * 2 );

    // Debug: Show first few raw buffer values
    if ( a_chan_cnt > 0 && abuf != nullptr ) {
        JULSEDEBUG_DAT( "Raw ADC data (first 10 bytes): " );
        for ( int i = 0; i < 10; i++ ) {
            JULSEDEBUG_DAT( "%d ", abuf[ i ] );
        }
        JULSEDEBUG_DAT( "\n\r" );
    }

    if ( dbuf ) {
        JULSEDEBUG_DAT( "Raw digital data (first 10 bytes): " );
        for ( int i = 0; i < 10; i++ ) {
            JULSEDEBUG_DAT( "%d ", dbuf[ i ] );
        }
        JULSEDEBUG_DAT( "\n\r" );
    }

    if ( use_decimation_mode ) {
        JULSEDEBUG_DAT( "DECIMATION MODE: Using adaptive timing (factor=%d, batch_size=%d)\n\r",
                        analog_decimation_factor, batch_size );
        JULSEDEBUG_DAT( "DECIMATION MODE: analog_samples_per_half=%d, max_analog_bytes=%d\n\r",
                        analog_samples_per_half, analog_samples_per_half * a_chan_cnt * 2 );
    }

#endif

    // OPTIMIZED: Process samples in smaller batches for better USB flow control
    const uint32_t batch_size = use_decimation_mode ? 16 : 32; // Even smaller batches for better reliability
    uint32_t samples_processed = 0;

    while ( samples_processed < samp_remain ) {
        uint32_t batch_end = samples_processed + batch_size;
        if ( batch_end > samp_remain ) {
            batch_end = samp_remain;
        }

        // Process each sample in the batch
        for ( uint32_t s = samples_processed; s < batch_end; s++ ) {
            // CRITICAL: Check bounds before processing - simplified but effective
            uint32_t digital_offset = s * d_dma_bps;
            if ( digital_offset >= d_size ) {
                JULSEDEBUG_ERR( "Digital buffer overrun at sample %d (offset=%d, size=%d)\n\r",
                                s, digital_offset, d_size );
                goto end_processing;
            }

            // Process digital data
            uint32_t digital_value = read_digital_sample( dbuf, s );
            uint32_t original_digital = digital_value; // Save for debug output
            encode_digital_data( digital_value );

            // Process analog data if enabled
            if ( a_chan_cnt > 0 && abuf != nullptr ) {
                process_analog_sample( abuf, s );
            }

            // Debug: Track sample structure for first few samples
            // if ( s < 10 ) {
            //     JULSEDEBUG_DAT( "SAMPLE %d: digital_bytes=%d, analog_bytes=%d, total_bytes=%d\n\r",
            //                    s, d_tx_bps, (a_chan_cnt > 0) ? (a_chan_cnt * 2) : 0,
            //                    d_tx_bps + ((a_chan_cnt > 0) ? (a_chan_cnt * 2) : 0) );
            // }

#if PRINT_DATA_SAMPLES
            // Debug output for sample data visualization
            bool show_sample = ( s < 5 ) ||
                               ( s >= ( samp_remain / 2 - 5 ) && s < ( samp_remain / 2 + 5 ) ) ||
                               ( s >= samp_remain - 5 );

            if ( show_sample ) {
                if ( s == 0 || s == ( samp_remain / 2 - 5 ) || s == samp_remain - 5 ) {
                    JULSEDEBUG_STA( "\n\r=== SAMPLE DATA TABLE ===\n\r" );
                    if ( d_chan_cnt == 16 && c_mask != 0 ) {
                        JULSEDEBUG_STA( "Sample tot | Sample cnk | GPIO(8) | Ctrl(8) | ADC0 | ADC1 | ADC2 | ADC3 | ADC4\n\r" );
                        JULSEDEBUG_STA( "-----------|------------|---------|---------|------|------|------|------|------\n\r" );
                    } else {
                        JULSEDEBUG_STA( "Sample tot | Sample cnk |  Digital | ADC0 | ADC1 | ADC2 | ADC3 | ADC4\n\r" );
                        JULSEDEBUG_STA( "-----------|------------|----------|------|------|------|------|------\n\r" );
                    }
                }

                // Print sample number and digital data
                JULSEDEBUG_STA( "%8d  |  %8d   | ", scnt + s, s );

                if ( d_chan_cnt == 16 && c_mask != 0 ) {
                    // 16-channel mode: show GPIO and control separately
                    uint8_t gpio_data = original_digital & 0xFF;
                    uint8_t control_data = ( original_digital >> 8 ) & 0xFF;

                    for ( int b = 7; b >= 0; b-- ) {
                        JULSEDEBUG_STA( "%d", ( gpio_data >> b ) & 1 );
                    }
                    JULSEDEBUG_STA( " | " );
                    for ( int b = 7; b >= 0; b-- ) {
                        JULSEDEBUG_STA( "%d", ( control_data >> b ) & 1 );
                    }
                    JULSEDEBUG_STA( " | " );
                } else {
                    // 8-channel mode: show all bits
                    for ( int b = 7; b >= 0; b-- ) {
                        JULSEDEBUG_STA( "%d", ( original_digital >> b ) & 1 );
                    }
                    JULSEDEBUG_STA( " | " );
                }

                // Print analog values - now always available due to firmware-side decimation
                if ( a_chan_cnt > 0 && abuf != nullptr ) {
                    // Note: In decimation mode, this shows duplicated values after firmware processing
                    uint32_t analog_offset = get_analog_buffer_offset( s );

                    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                        if ( !( ( a_mask >> i ) & 1 ) ) {
                            JULSEDEBUG_STA( "  -- | " );
                            continue;
                        }

                        uint16_t adc_raw = abuf[ analog_offset ] | ( abuf[ analog_offset + 1 ] << 8 );
                        uint16_t adc_12bit = adc_raw & 0x0FFF;
                        JULSEDEBUG_STA( "%4d | ", adc_12bit );
                        analog_offset += 2;
                    }
                } else {
                    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
                        JULSEDEBUG_STA( "  -- | " );
                    }
                }
                JULSEDEBUG_STA( "\n\r" );
            }
#endif

            // // OPTIMIZED FREQUENCY: Check transmission buffer more frequently for reliability
            // if ( use_decimation_mode ) {
            //     // In decimation mode, check more frequently to prevent buffer overflow
            //     if ( ( s & 0x1F ) == 0 ) { // Every 32 samples in decimation mode (increased frequency)
            //         check_tx_buf( JULSEVIEW_TX_BUF_THRESH );
            //     }
            // } else {
            //     // In normal mode, also check more frequently
            //     if ( ( s & 0x3F ) == 0 ) { // Every 64 samples in normal mode (increased frequency)
            //         check_tx_buf( JULSEVIEW_TX_BUF_THRESH );
            //     }
            // }

            // // ADDITIONAL: Force transmission if buffer is getting very full
            // if ( txbufidx >= (JULSEVIEW_TX_BUF_SIZE * 7 / 8) ) {
            //     // Buffer is 87.5% full - force immediate transmission with USB status check
            //     int usb_available = tud_cdc_n_write_available(2);
            //     if ( usb_available < 256 ) {
            //         // USB buffer is also nearly full - emergency throttling
            //         JULSEDEBUG_USB( "Emergency: Both buffers near full - USB:%d, TX:%d\n\r", usb_available, txbufidx );
            //         tud_task();
            //         delayMicroseconds( 150 ); // Emergency throttle
            //     }
            //     check_tx_buf( 1 );
            // }
        }

        samples_processed = batch_end;

        // IMPROVED: More aggressive buffer flushing after each batch
        check_tx_buf( JULSEVIEW_TX_BUF_THRESH ); // Use lower threshold for more frequent flushing

        // // SIMPLE FLOW CONTROL: Basic USB buffer monitoring
        // if ( samples_processed % 128 == 0 ) {  // Check every 128 samples
        //     int usb_available = tud_cdc_n_write_available(2);
        //    // JULSEDEBUG_ERR("%d\n\r", usb_available);

        //     // Simple progressive backpressure
        //     if ( usb_available < 1024 ) {  // Less than 1KB available
        //         tud_task(); // Process USB events
        //         delayMicroseconds( 50 );

        //         if ( usb_available < 256 ) {   // Very low - force flush
        //             check_tx_buf( 1 );
        //         }
        //     }
        // }

        watchdog_update( );
    }

end_processing:

#if PRINT_TRANSMIT_SUMMARY
    // Show transmission summary
    JULSEDEBUG_DAT( "\n\r=== TRANSMISSION SUMMARY ===\n\r" );
    JULSEDEBUG_DAT( "Total samples configured: %d\n\r", num_samples );
    JULSEDEBUG_DAT( "Total samples processed: %d\n\r", samp_remain );
    JULSEDEBUG_DAT( "Total samples sent: %d\n\r", scnt + samp_remain );
    JULSEDEBUG_DAT( "Analog channels: %d, Digital channels: %d\n\r", a_chan_cnt, d_chan_cnt );
    JULSEDEBUG_DAT( "=== CHUNK TRANSMISSION COMPLETE ===\n\r" );
#endif

    // Update sample count and check for completion
    scnt += samp_remain;
    // JULSEDEBUG_DAT( "DEBUG: Sent %d samples, total scnt=%d/%d (buffer cycle complete)\n\r", samp_remain, scnt, num_samples );

    if ( scnt >= num_samples ) {
#if PRINT_DATA_SAMPLES
        JULSEDEBUG_CMD( "Sample limit reached (%d/%d) - DMA will continue until all data is captured\n\r",
                        scnt, num_samples );
#endif
    }
}

// Helper functions for send_slices_analog

uint32_t julseview::calculate_sample_count( ) {
    uint32_t current_samples;

    if ( !use_producer_consumer && a_chan_cnt == 0 ) {
        // Single-transfer digital-only path: process remaining requested samples
        current_samples = num_samples - scnt;
    } else if ( use_decimation_mode && a_chan_cnt > 0 ) {
        // CRITICAL FIX: In decimation mode, limit to analog buffer capacity
        // Analog buffer holds analog_samples_per_half time samples
        // Digital samples are generated via firmware decimation to match
        current_samples = analog_samples_per_half * analog_decimation_factor;
        // JULSEDEBUG_DAT( "DECIMATION SAMPLE COUNT: analog_samples=%d × factor=%d = %d digital samples\n\r",
        //                analog_samples_per_half, analog_decimation_factor, current_samples );
    } else {
        current_samples = samples_per_half; // Use compatibility sample count in normal mode
    }

    return current_samples;
}

bool julseview::validate_buffer_bounds( uint32_t samples_to_process ) {
    // Start with requested sample count

    samp_remain = samples_to_process;

    // PRIORITY 1: Always limit to remaining requested samples
    if ( !cont && ( scnt + samp_remain > num_samples ) ) {
        uint32_t remaining_requested = num_samples - scnt;
        if ( samp_remain > remaining_requested ) {
            JULSEDEBUG_DAT( "SAMPLE LIMIT: Reducing from %d to %d (remaining requested)\n\r",
                            samp_remain, remaining_requested );
            samp_remain = remaining_requested;
        }
    }

    // PRIORITY 2: Check digital buffer capacity (applies to all modes)
    if ( samp_remain > digital_samples_per_half ) {
        JULSEDEBUG_DAT( "DIGITAL BUFFER LIMIT: Reducing from %d to %d\n\r",
                        samp_remain, digital_samples_per_half );
        samp_remain = digital_samples_per_half;
    }

    // Serial.println( "DIGITAL BUFFER LIMIT: " );
    // Serial.print( "samp_remain: " );
    // Serial.println( samp_remain );
    // Serial.print( "digital_samples_per_half: " );
    // Serial.println( digital_samples_per_half );
    // Serial.print( "samp_remain > digital_samples_per_half: " );
    // Serial.println( samp_remain > digital_samples_per_half );


    // PRIORITY 3: In decimation mode, maintain exact synchronization ratio
    if ( use_decimation_mode && a_chan_cnt > 0 ) {
        // Ensure digital samples remain exact multiple of analog samples × decimation factor
        uint32_t required_analog_samples = ( samp_remain + analog_decimation_factor - 1 ) / analog_decimation_factor; // Round up

        if ( required_analog_samples > analog_samples_per_half ) {
            // Reduce to maintain exact sync ratio
            uint32_t max_digital_sync = analog_samples_per_half * analog_decimation_factor;
            JULSEDEBUG_DAT( "DECIMATION SYNC LIMIT: Reducing from %d to %d (analog supports %d samples × %d factor)\n\r",
                            samp_remain, max_digital_sync, analog_samples_per_half, analog_decimation_factor );
            samp_remain = max_digital_sync;
        }
    } else if ( !use_decimation_mode && a_chan_cnt > 0 ) {
        // In normal (synchronized) mode, analog and digital must match sample counts
        if ( samp_remain > analog_samples_per_half ) {
            JULSEDEBUG_DAT( "ANALOG BUFFER LIMIT (sync mode): Reducing from %d to %d\n\r",
                            samp_remain, analog_samples_per_half );
            samp_remain = analog_samples_per_half;
        }
    }

    return ( samp_remain != samples_to_process ); // Return true if adjustment was made
}

uint32_t julseview::read_digital_sample( uint8_t* dbuf, uint32_t sample_index ) {
    uint32_t cval = 0;

    if ( d_chan_cnt > 8 || c_mask != 0 ) {
        // 16 channels with control channels enabled
        uint8_t gpio_data = dbuf[ rxbufdidx ];

        // Get control channel data
        uint8_t control_data = 0;
        uint32_t quarter_size = g_julseview_digital_buf_size / 4;
        uint32_t control_sample_idx = scnt + sample_index;
        uint32_t control_buffer_half = ( control_sample_idx / quarter_size ) % 2;
        uint32_t control_buffer_offset = control_sample_idx % quarter_size;

        if ( control_buffer_half == 0 ) {
            control_data = capture_buf[ cbuf0_start + control_buffer_offset ];
        } else {
            control_data = capture_buf[ cbuf1_start + control_buffer_offset ];
        }

        // Combine into 16-bit value: GPIO data in lower 8 bits, control in upper 8 bits
        cval = gpio_data | ( control_data << 8 );
        rxbufdidx += 1;
    } else {
        // 8 channels - use original logic
        cval = get_cval( dbuf );
    }

    return cval;
}

void julseview::encode_digital_data( uint32_t digital_value ) {
    uint32_t cval = digital_value;

    // Encode digital data into 7-bit chunks
    for ( char b = 0; b < d_tx_bps; b++ ) {
        uint8_t data_byte = ( cval & 0x7F ) + 0x30;
        if ( data_byte < 0x80 )
            data_byte |= 0x80;
        txbuf[ txbufidx++ ] = data_byte;
        cval >>= 7;
    }
}

// DEPRECATED: Decimation logic moved to firmware-side in process_analog_sample()
// This function is kept for backward compatibility but no longer used
bool julseview::should_read_analog_sample( uint32_t sample_index ) {
    // Firmware-side decimation handles this logic now
    return ( a_chan_cnt > 0 );
}

uint32_t julseview::get_analog_buffer_offset( uint32_t sample_index ) {
    if ( use_decimation_mode ) {
        uint32_t analog_sample_idx = sample_index / analog_decimation_factor;

        // CRITICAL: Check if we're trying to read beyond the analog buffer
        if ( analog_sample_idx >= analog_samples_per_half ) {
            JULSEDEBUG_ERR( "ANALOG BUFFER OVERRUN: sample_index=%d, analog_sample_idx=%d, analog_samples_per_half=%d\n\r",
                            sample_index, analog_sample_idx, analog_samples_per_half );
            // Return a safe offset that won't cause memory corruption
            return ( analog_samples_per_half - 1 ) * ( a_chan_cnt * 2 );
        }

        return analog_sample_idx * ( a_chan_cnt * 2 );
    } else {
        return sample_index * ( a_chan_cnt * 2 );
    }
}

void julseview::process_analog_sample( uint8_t* abuf, uint32_t sample_index ) {
    // FIRMWARE-SIDE DECIMATION: Static storage for last analog values
    static uint16_t last_analog_values[ JULSEVIEW_MAX_ANALOG_CHANNELS ] = { 0 };
    static bool first_sample = true;

    // Calculate absolute sample index for proper decimation timing across buffer boundaries
    uint32_t absolute_sample_index = scnt + sample_index;

    // Auto-reset for new capture (when absolute sample count is 0)
    if ( absolute_sample_index == 0 ) {
        first_sample = true;
        // Clear stored values
        for ( int i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
            last_analog_values[ i ] = 0;
        }
        // JULSEDEBUG_DAT( "FIRMWARE DECIMATION: Auto-reset for new capture\n\r" );
    }

    bool should_read_new_data = true;
    uint32_t analog_offset = 0;

    if ( use_decimation_mode ) {
        // CRITICAL FIX: Use absolute sample index for decimation timing
        should_read_new_data = ( absolute_sample_index % analog_decimation_factor == 0 );

        if ( should_read_new_data ) {
            // Read new data from buffer
            analog_offset = get_analog_buffer_offset( sample_index );
        }
        // For duplication, we'll use the stored last_analog_values (no need to calculate offset)

        // Debug for first few buffer switches
        // if ( sample_index < 5 && absolute_sample_index % 8192 < 5 ) {
        //     JULSEDEBUG_DAT( "DECIMATION SYNC: abs_sample=%d, rel_sample=%d, should_read=%d\n\r",
        //                    absolute_sample_index, sample_index, should_read_new_data );
        // }
    } else {
        // In normal mode, always read new data
        analog_offset = get_analog_buffer_offset( sample_index );
    }

    // Debug output for first few analog samples
    // if ( sample_index < 2 ) {
    //     JULSEDEBUG_DAT( "ANALOG PROCESS: sample=%d, offset=%d, a_size=%d, abuf=%p\n\r",
    //                    sample_index, analog_offset, a_size, abuf );

    //     // Show the actual bytes being read
    //     if ( analog_offset < 20 ) {
    //         JULSEDEBUG_DAT( "ANALOG BYTES: offset=%d, bytes=[%d %d %d %d]\n\r",
    //                        analog_offset,
    //                        abuf[analog_offset], abuf[analog_offset+1],
    //                        abuf[analog_offset+2], abuf[analog_offset+3] );
    //     }
    // }

    // Safety bounds check - only needed when reading new data from buffer
    if ( should_read_new_data ) {
        uint32_t max_analog_bytes = a_size;
        if ( use_decimation_mode ) {
            // In decimation mode, only part of the buffer is actually used
            max_analog_bytes = analog_samples_per_half * a_chan_cnt * 2;
        }

        if ( analog_offset + ( a_chan_cnt * 2 ) > max_analog_bytes ) {
            JULSEDEBUG_ERR( "Analog buffer overrun at sample %d (offset=%d, max_bytes=%d, a_size=%d)\n\r",
                            sample_index, analog_offset, max_analog_bytes, a_size );
            return;
        }
    }
    // Note: When duplicating data (!should_read_new_data), no bounds check needed since we're using stored values

    // IMPROVED: More conservative buffer overflow checking for analog data
    uint32_t bytes_needed = a_chan_cnt * 2; // 2 bytes per analog channel
    // Use smaller thresholds to prevent buffer overflow
    uint32_t threshold = use_decimation_mode ? ( JULSEVIEW_TX_BUF_SIZE / 2 ) : ( JULSEVIEW_TX_BUF_SIZE * 2 / 3 );
    if ( txbufidx + bytes_needed >= threshold ) {
        // Buffer is getting full, force a transmission
        check_tx_buf( 1 ); // Force transmission of any data
    }

    // Process enabled analog channels with firmware-side decimation
    uint32_t current_analog_offset = analog_offset; // Local copy for incrementing

    for ( char i = 0; i < JULSEVIEW_MAX_ANALOG_CHANNELS; i++ ) {
        if ( !( ( a_mask >> i ) & 1 ) ) {
            continue; // Skip disabled channels
        }

        uint16_t adc_12bit;

        if ( use_decimation_mode && !should_read_new_data && !first_sample ) {
            // DECIMATION: Use previously stored value (firmware-side duplication)
            adc_12bit = last_analog_values[ i ];

            // Debug output for first few duplicated samples
            // if ( sample_index < 20 && sample_index % analog_decimation_factor != 0 ) {
            //     JULSEDEBUG_DAT( "FIRMWARE DECIMATION: sample=%d, channel=%d, duplicating value=%d\n\r",
            //                    sample_index, i, adc_12bit );
            // }
        } else {
            // Read new data from DMA buffer
            uint16_t adc_value = abuf[ current_analog_offset ] | ( abuf[ current_analog_offset + 1 ] << 8 );
            adc_12bit = adc_value & 0x0FFF;

            // Store this value for future duplication in decimation mode
            last_analog_values[ i ] = adc_12bit;

            // Debug output for first few new samples
            // if ( sample_index < 20 && ( !use_decimation_mode || sample_index % analog_decimation_factor == 0 ) ) {
            //     JULSEDEBUG_DAT( "FIRMWARE NEW DATA: sample=%d, channel=%d, new value=%d\n\r",
            //                    sample_index, i, adc_12bit );
            // }

            current_analog_offset += 2; // 16-bit mode: 2 bytes per channel
        }

        // Split 12-bit value into 7-bit chunks and transmit
        uint8_t byte1 = adc_12bit & 0x7F;
        uint8_t byte2 = ( adc_12bit >> 7 ) & 0x7F;

        txbuf[ txbufidx ] = byte1 + 0x30;
        txbuf[ txbufidx + 1 ] = byte2 + 0x30;
        txbufidx += 2;
    }

    first_sample = false; // Clear first sample flag after processing first sample
}

// Reset firmware decimation state for new capture
void julseview::reset_firmware_decimation_state( ) {
    // The reset is now handled automatically in process_analog_sample when sample_index == 0
    // This function is kept for explicit resets if needed
    // JULSEDEBUG_DAT( "FIRMWARE DECIMATION: Reset requested (auto-reset on sample 0)\n\r" );
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

        watchdog_update( );
    }

    JULSEDEBUG_CMD( "waitForTrigger() - completed\n\r" );
}

// Pre-trigger functionality removed - simplified to post-trigger only

// ============================================================================
// DECIMATION MODE FUNCTIONS
// ============================================================================

// Configure decimation mode based on sample rate
void julseview::configure_decimation_mode( ) {
    JULSEDEBUG_STA( "=== CONFIGURING DECIMATION MODE ===\n\r" );
    JULSEDEBUG_STA( "DEBUG: sample_rate=%d, a_chan_cnt=%d, total_load=%d, limit=%d\n\r",
                    sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE );

    // Check if we need decimation mode
    // ADC limit is 200kHz TOTAL, not per channel

    if ( a_chan_cnt == 0 ) {
        use_decimation_mode = false;
        analog_decimation_factor = 1;
        return;
    }

    if ( sample_rate * a_chan_cnt > JULSEVIEW_ADC_MAX_RATE ) {
        use_decimation_mode = true;

        // Calculate initial decimation factor (how many digital samples per analog sample)
        uint32_t initial_factor = ( sample_rate * a_chan_cnt + JULSEVIEW_ADC_MAX_RATE - 1 ) / JULSEVIEW_ADC_MAX_RATE; // Round up

        // CRITICAL: Ensure decimation factor is an integer multiple of total samples
        // This prevents partial analog samples and ensures proper data alignment
        if ( num_samples > 0 ) {
            // Find the smallest factor that's >= initial_factor and divides num_samples evenly
            analog_decimation_factor = initial_factor;

            // If the initial factor doesn't divide num_samples evenly, find the next suitable factor
            while ( num_samples % analog_decimation_factor != 0 ) {
                analog_decimation_factor++;

                // Safety check to prevent infinite loop
                if ( analog_decimation_factor > JULSEVIEW_DECIMATION_MAX_FACTOR ) {
                    JULSEDEBUG_STA( "WARNING: Could not find suitable decimation factor, using initial factor\n\r" );
                    analog_decimation_factor = initial_factor;
                    break;
                }
            }

            JULSEDEBUG_STA( "DECIMATION FACTOR ADJUSTMENT: initial=%d, final=%d (divides %d samples evenly)\n\r",
                            initial_factor, analog_decimation_factor, num_samples );
        } else {
            // If num_samples is 0 (continuous mode), use the initial factor
            analog_decimation_factor = initial_factor;
        }

        // Clamp to reasonable limits
        if ( analog_decimation_factor < JULSEVIEW_DECIMATION_MIN_FACTOR ) {
            analog_decimation_factor = JULSEVIEW_DECIMATION_MIN_FACTOR;
        }
        if ( analog_decimation_factor > JULSEVIEW_DECIMATION_MAX_FACTOR ) {
            analog_decimation_factor = JULSEVIEW_DECIMATION_MAX_FACTOR;
        }

        JULSEDEBUG_STA( "DECIMATION ENABLED: sample_rate=%d Hz × %d channels = %d Hz > %d Hz total ADC limit\n\r",
                        sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE );
        JULSEDEBUG_STA( "DECIMATION FACTOR: %d (each analog sample duplicated %d times)\n\r",
                        analog_decimation_factor, analog_decimation_factor );
        JULSEDEBUG_STA( "SAMPLE ALIGNMENT: %d samples ÷ %d factor = %d analog samples (remainder: %d)\n\r",
                        num_samples, analog_decimation_factor, num_samples / analog_decimation_factor,
                        num_samples % analog_decimation_factor );

        // CRITICAL: Verify the decimation factor divides the total samples evenly
        if ( num_samples % analog_decimation_factor != 0 ) {
            JULSEDEBUG_STA( "ERROR: Decimation factor %d does not divide %d samples evenly!\n\r",
                            analog_decimation_factor, num_samples );
            JULSEDEBUG_STA( "  This will cause data misalignment and buffer overruns!\n\r" );
        } else {
            JULSEDEBUG_STA( "✓ Decimation factor %d properly divides %d samples (remainder: 0)\n\r",
                            analog_decimation_factor, num_samples );
        }

        // Calculate actual ADC sample rate per channel
        uint32_t actual_adc_rate_per_channel = ( sample_rate * a_chan_cnt ) / analog_decimation_factor / a_chan_cnt;
        JULSEDEBUG_STA( "ACTUAL ADC RATE: %d Hz per channel (digital at %d Hz)\n\r",
                        actual_adc_rate_per_channel, sample_rate );

    } else {
        use_decimation_mode = false;
        analog_decimation_factor = 1;

        uint32_t max_rate_per_channel = JULSEVIEW_ADC_MAX_RATE / a_chan_cnt;
        JULSEDEBUG_STA( "NORMAL MODE: sample_rate=%d Hz × %d channels = %d Hz <= %d Hz total ADC limit\n\r",
                        sample_rate, a_chan_cnt, sample_rate * a_chan_cnt, JULSEVIEW_ADC_MAX_RATE );
        JULSEDEBUG_STA( "NORMAL MODE: max rate per channel = %d Hz\n\r", max_rate_per_channel );
    }
}

// Calculate asymmetric buffer layout - ALWAYS USE ASYMMETRIC LAYOUT BY DEFAULT
void julseview::calculate_asymmetric_buffer_layout( ) {
    JULSEDEBUG_BUF( "=== CALCULATING ASYMMETRIC BUFFER LAYOUT (DEFAULT) ===\n\r" );

    // Always use asymmetric layout for maximum compatibility and performance
    // If no analog channels are enabled, repurpose the entire analog buffer for digital data
    if ( a_chan_cnt == 0 ) {

        // g_julseview_digital_buf_size = JULSEVIEW_ANALOG_BUF_SIZE + 32768 + 2048;
        uint32_t total_digital_bytes = g_julseview_digital_buf_size; // 96KB total (default)
        a_size = 0;                                                  // No analog buffers
        d_size = total_digital_bytes / 2;                            // 48KB per digital half

        // Buffer layout (no analog): [dbuf0 48KB][dbuf1 48KB]
        abuf0_start = 0;
        abuf1_start = 0;
        dbuf0_start = 0;
        dbuf1_start = d_size;
        JULSEDEBUG_BUF( "NO-ANALOG LAYOUT:\n\r" );
        JULSEDEBUG_BUF( "  dbuf0: 0x%08X - 0x%08X (%d bytes)\n\r", dbuf0_start, dbuf0_start + d_size - 1, d_size );
        JULSEDEBUG_BUF( "  dbuf1: 0x%08X - 0x%08X (%d bytes)\n\r", dbuf1_start, dbuf1_start + d_size - 1, d_size );
    } else {
        // Use full buffer sizes for maximum capacity
        a_size = JULSEVIEW_ANALOG_BUF_SIZE / 2;    // 32KB analog buffers
        d_size = g_julseview_digital_buf_size / 2; // half of global digital region

        // Note: In decimation mode, a_size remains the full buffer size
        // but we only read from a portion of it based on analog_samples_per_half

        // Buffer layout: [abuf0 32KB][abuf1 32KB][dbuf0 16KB][dbuf1 16KB] = 96KB total
        abuf0_start = 0;
        abuf1_start = abuf0_start + a_size;
        dbuf0_start = abuf1_start + a_size;
        dbuf1_start = dbuf0_start + d_size;

        JULSEDEBUG_BUF( "ASYMMETRIC BUFFER LAYOUT (DEFAULT):\n\r" );
        JULSEDEBUG_BUF( "  abuf0: 0x%08X - 0x%08X (%d bytes)\n\r",
                        abuf0_start, abuf0_start + a_size - 1, a_size );
        JULSEDEBUG_BUF( "  abuf1: 0x%08X - 0x%08X (%d bytes)\n\r",
                        abuf1_start, abuf1_start + a_size - 1, a_size );
        // Calculate actual buffer size for debug output
        uint32_t actual_dbuf_size = ( d_chan_cnt == 16 && c_mask != 0 ) ? ( g_julseview_digital_buf_size / 4 ) : d_size;
        JULSEDEBUG_BUF( "  dbuf0: 0x%08X - 0x%08X (%d bytes)\n\r",
                        dbuf0_start, dbuf0_start + actual_dbuf_size - 1, actual_dbuf_size );
        JULSEDEBUG_BUF( "  dbuf1: 0x%08X - 0x%08X (%d bytes)\n\r",
                        dbuf1_start, dbuf1_start + actual_dbuf_size - 1, actual_dbuf_size );

        // Show control channel buffers if enabled
        if ( d_chan_cnt == 16 && c_mask != 0 ) {
            uint32_t quarter_size = g_julseview_digital_buf_size / 4; // quarter of global digital region
            JULSEDEBUG_BUF( "  cbuf0: 0x%08X - 0x%08X (%d bytes)\n\r",
                            cbuf0_start, cbuf0_start + quarter_size - 1, quarter_size );
            JULSEDEBUG_BUF( "  cbuf1: 0x%08X - 0x%08X (%d bytes)\n\r",
                            cbuf1_start, cbuf1_start + quarter_size - 1, quarter_size );
        }
    }

    // Handle control channel buffer setup for 16-channel mode
    if ( d_chan_cnt > 8 && c_mask != 0 ) {
        // 16-channel mode with control channels: split digital buffer into quarters
        uint32_t quarter_size = g_julseview_digital_buf_size / 4; // quarter of global digital region

        if ( a_chan_cnt == 0 ) {
            // Digital-only mode: control channels use third and fourth quarters
            cbuf0_start = dbuf0_start + ( quarter_size * 2 ); // third quarter
            cbuf1_start = dbuf0_start + ( quarter_size * 3 ); // fourth quarter
        } else {
            // Mixed mode: control channels use third and fourth quarters after digital region
            cbuf0_start = dbuf0_start + ( quarter_size * 2 ); // third quarter
            cbuf1_start = dbuf0_start + ( quarter_size * 3 ); // fourth quarter
        }

        // Show control channel buffers
        JULSEDEBUG_BUF( "  cbuf0: 0x%08X - 0x%08X (%d bytes)\n\r",
                        cbuf0_start, cbuf0_start + quarter_size - 1, quarter_size );
        JULSEDEBUG_BUF( "  cbuf1: 0x%08X - 0x%08X (%d bytes)\n\r",
                        cbuf1_start, cbuf1_start + quarter_size - 1, quarter_size );
    } else {
        // No control channels in 8-channel mode
        cbuf0_start = 0;
        cbuf1_start = 0;
    }

    // Calculate samples per buffer based on actual buffer sizes
    if ( d_chan_cnt == 16 && c_mask != 0 ) {
        // 16-channel mode: digital buffer split into quarters
        uint32_t total_digital_bytes = ( a_chan_cnt == 0 ) ? ( JULSEVIEW_ANALOG_BUF_SIZE + g_julseview_digital_buf_size )
                                                           : g_julseview_digital_buf_size;
        uint32_t quarter_size = total_digital_bytes / 4;
        digital_samples_per_half = quarter_size / d_dma_bps;
    } else {
        // 8-channel mode: full digital buffer halves (can be 16KB or 48KB when no analog)
        digital_samples_per_half = d_size / d_dma_bps;
    }
    analog_samples_per_half = ( a_chan_cnt == 0 ) ? 0 : ( a_size / ( a_chan_cnt * 2 ) );

    JULSEDEBUG_BUF( "SAMPLE COUNTS:\n\r" );
    JULSEDEBUG_BUF( "  digital_samples_per_half: %d\n\r", digital_samples_per_half );
    JULSEDEBUG_BUF( "  analog_samples_per_half: %d\n\r", analog_samples_per_half );

    // In decimation mode, ensure exact synchronization: digital = analog × decimation_factor
    if ( use_decimation_mode ) {
        // CRITICAL: Digital samples must be exact multiple of analog samples × decimation factor
        uint32_t physical_digital_limit = d_size / d_dma_bps; // Actual samples that fit in buffer
        uint32_t required_digital = analog_samples_per_half * analog_decimation_factor;

        // Ensure we don't exceed physical buffer capacity
        if ( required_digital <= physical_digital_limit ) {
            // Perfect fit - use exact synchronization
            digital_samples_per_half = required_digital;
            samples_per_half = required_digital;
            JULSEDEBUG_BUF( "DECIMATION MODE: Perfect sync - digital=%d (analog=%d × factor=%d)\n\r",
                            digital_samples_per_half, analog_samples_per_half, analog_decimation_factor );
        } else {
            // Buffer too small - reduce analog buffer to maintain exact sync
            uint32_t max_analog = physical_digital_limit / analog_decimation_factor;
            analog_samples_per_half = max_analog;
            digital_samples_per_half = max_analog * analog_decimation_factor;
            samples_per_half = digital_samples_per_half;
            JULSEDEBUG_BUF( "DECIMATION MODE: Buffer-limited sync - analog reduced to %d, digital=%d\n\r",
                            analog_samples_per_half, digital_samples_per_half );
        }

        JULSEDEBUG_BUF( "DECIMATION MODE: Final counts (digital=%d, analog=%d, factor=%d, ratio=%.1f)\n\r",
                        digital_samples_per_half, analog_samples_per_half, analog_decimation_factor,
                        (float)digital_samples_per_half / analog_samples_per_half );
    } else {
        // For normal mode, hard-limit to the smaller half so digital never overruns analog
        samples_per_half = ( a_chan_cnt == 0 ) ? digital_samples_per_half
                                               : ( ( digital_samples_per_half < analog_samples_per_half ) ? digital_samples_per_half : analog_samples_per_half );

        // Harmonize per-half counts so both sides use the same time-aligned sample count
        if ( a_chan_cnt > 0 ) {
            digital_samples_per_half = samples_per_half;
            analog_samples_per_half = samples_per_half;
        } else {
            // Digital-only: leave analog at 0, keep digital as computed above
            digital_samples_per_half = samples_per_half;
        }
    }

    JULSEDEBUG_BUF( "  samples_per_half (compatibility): %d\n\r", samples_per_half );
}

// send_slices_analog_decimated function removed - unified into send_slices_analog

// Duplicate analog sample data for decimation mode - REMOVED
// This function is no longer needed as the driver now handles duplication
// The firmware only sends actual analog data and lets the driver duplicate it

// Timer-based control update removed; control bytes now written via DMA paced by PIO DREQ
void julseview::update_control_channel_buffer( ) {}

// Setup DMA for control channel capture paced by the same PIO DREQ
void julseview::setup_control_channel_dma( ) {
    if ( c_mask == 0 ) {
        return; // Only needed for 16-channel mode with control channels
    }

    JULSEDEBUG_DIG( "Setting up control channel DMA (DREQ-paced) ...\n\r" );

    // Control buffer addresses are now calculated in calculate_asymmetric_buffer_layout()
    // Quarters: 0=GPIO0, 1=GPIO1, 2=Control0, 3=Control1
    JULSEDEBUG_DIG( "Using shared digital buffer, split into quarters\n\r" );
    JULSEDEBUG_DIG( "Digital buffer size: %d bytes, quarter size: %d bytes\n\r", d_size, d_size / 4 );

    uint32_t quarter_size = g_julseview_digital_buf_size / 4; // quarter size of total digital region
    JULSEDEBUG_DIG( "Control buffer addresses: cbuf0=%d, cbuf1=%d\n\r", cbuf0_start, cbuf1_start );
    JULSEDEBUG_DIG( "Buffer layout: GPIO0=%d, GPIO1=%d, Control0=%d, Control1=%d\n\r", dbuf0_start, dbuf1_start, cbuf0_start, cbuf1_start );
    JULSEDEBUG_DIG( "Quarter size: %d bytes, Total digital buffer: %d bytes\n\r", quarter_size, g_julseview_digital_buf_size );

    // Clear control buffer quarters
    memset( capture_buf + cbuf0_start, 0, quarter_size );
    memset( capture_buf + cbuf1_start, 0, quarter_size );

    // Claim two DMA channels: one per half of the control buffer
    control_dma_chan0 = dma_claim_unused_channel( true );
    control_dma_chan1 = dma_claim_unused_channel( true );
    if ( control_dma_chan0 < 0 || control_dma_chan1 < 0 ) {
        JULSEDEBUG_ERR( "ERROR: Failed to claim control DMA channels! ch0=%d ch1=%d\n\r", control_dma_chan0, control_dma_chan1 );
        return;
    }

    // Global volatile source byte updated by other subsystems
    extern volatile uint8_t control_data;

    // Configure DMA for 8-bit copies from a fixed address to incrementing buffer
    ccfg0 = dma_channel_get_default_config( control_dma_chan0 );
    channel_config_set_transfer_data_size( &ccfg0, DMA_SIZE_8 );
    channel_config_set_read_increment( &ccfg0, false );
    channel_config_set_write_increment( &ccfg0, true );
    channel_config_set_dreq( &ccfg0, dreq ); // pace by same PIO DREQ as GPIO DMA

    ccfg1 = dma_channel_get_default_config( control_dma_chan1 );
    channel_config_set_transfer_data_size( &ccfg1, DMA_SIZE_8 );
    channel_config_set_read_increment( &ccfg1, false );
    channel_config_set_write_increment( &ccfg1, true );
    channel_config_set_dreq( &ccfg1, dreq );

    // Configure ping-pong transfers sized to each quarter
    dma_channel_configure( control_dma_chan0, &ccfg0,
                           capture_buf + cbuf0_start,  // write addr
                           (const void*)&control_data, // read addr (fixed)
                           quarter_size,               // transfer count (bytes)
                           false );

    dma_channel_configure( control_dma_chan1, &ccfg1,
                           capture_buf + cbuf1_start,
                           (const void*)&control_data,
                           quarter_size,
                           false );

    // Chain control DMA channels to each other for continuous operation
    channel_config_set_chain_to( &ccfg0, control_dma_chan1 );
    channel_config_set_chain_to( &ccfg1, control_dma_chan0 );

    // Apply updated configs after chaining setup
    dma_channel_set_config( control_dma_chan0, &ccfg0, false );
    dma_channel_set_config( control_dma_chan1, &ccfg1, false );

    // Start ch0 now; ch1 will chain automatically
    dma_channel_start( control_dma_chan0 );

    JULSEDEBUG_DIG( "Control DMA channels configured: ch0=%d ch1=%d (DREQ %d)\n\r", control_dma_chan0, control_dma_chan1, dreq );
}

// Collect control channel data into an 8-bit value
// This function reads control_D[0-3] and physical pins 0, 1, 6, 7
// Returns 8 bits for control channels C0-C7
uint8_t julseview::collect_control_channel_data( ) {
    uint8_t control_data = 0;
    return 0b01010101;

    // Read control channels C0-C7:
    // C0: UART_TX (pin 0)
    // C1: UART_RX (pin 1)
    // C2: Python_D1 (control_D[0])
    // C3: Python_D2 (control_D[1])
    // C4: Python_D3 (control_D[2])
    // C5: Python_D4 (control_D[3])
    // C6: RP_6 (pin 6)
    // C7: RP_7 (pin 7)

    // Read physical pins for UART and RP channels
    bool uart_tx = gpio_get( 0 ); // UART_TX pin
    bool uart_rx = gpio_get( 1 ); // UART_RX pin
    bool rp_6 = gpio_get( 6 );    // RP_6 pin
    bool rp_7 = gpio_get( 7 );    // RP_7 pin

    // Read MicroPython control variables
    bool python_d1 = control_D[ 0 ];
    bool python_d2 = control_D[ 1 ];
    bool python_d3 = control_D[ 2 ];
    bool python_d4 = control_D[ 3 ];

    // Build control channel data (bits 0-7)
    control_data |= ( uart_tx << 0 );   // C0: UART_TX
    control_data |= ( uart_rx << 1 );   // C1: UART_RX
    control_data |= ( python_d1 << 2 ); // C2: Python_D1
    control_data |= ( python_d2 << 3 ); // C3: Python_D2
    control_data |= ( python_d3 << 4 ); // C4: Python_D3
    control_data |= ( python_d4 << 5 ); // C5: Python_D4
    control_data |= ( rp_6 << 6 );      // C6: RP_6
    control_data |= ( rp_7 << 7 );      // C7: RP_7

    // Debug output for first few calls
    static int debug_count = 0;
    // if (debug_count < 5) {
    //     JULSEDEBUG_DIG("Control data collection %d: uart_tx=%d, uart_rx=%d, python_d[0-3]=%d%d%d%d, rp_6=%d, rp_7=%d, result=0x%02X\n\r",
    //                    debug_count, uart_tx, uart_rx, python_d1, python_d2, python_d3, python_d4, rp_6, rp_7, control_data);
    //     debug_count++;
    // }

    return control_data;
}

// =============================================================================
// SIMPLE HEARTBEAT WATCHDOG IMPLEMENTATION
// =============================================================================

volatile int heartbeat_enabled = 0;
volatile bool heartbeat_watchdog_started = false;

// Setup the heartbeat watchdog for capture loop recovery
bool julseview::setup_heartbeat_watchdog( ) {

    // Calculate expected capture time based on sample rate and count
    expected_capture_time_us = calculate_expected_capture_time( );

    // Set timeout to 2x expected time
    heartbeat_timeout_us = expected_capture_time_us * 2;

    // Apply minimum timeout based on trigger configuration
    uint32_t min_timeout_us;
    if ( trigger_config.enabled ) {
        min_timeout_us = 10000000; // 10 seconds minimum for trigger mode
        JULSEDEBUG_CMD( "Trigger mode: using 10 second minimum timeout\n\r" );
    } else {
        min_timeout_us = 1000000; // 1 second minimum for normal mode
        JULSEDEBUG_CMD( "Normal mode: using 1 second minimum timeout\n\r" );
    }

    // Ensure timeout is at least the minimum
    if ( heartbeat_timeout_us < min_timeout_us ) {
        heartbeat_timeout_us = min_timeout_us;
        JULSEDEBUG_CMD( "Heartbeat timeout increased to minimum: %lu us\n\r", heartbeat_timeout_us );
    }

    // Initialize global heartbeat state
    heartbeat_timeout_us = heartbeat_timeout_us;
    heartbeat_triggered = false;
    heartbeat_last_time = 0;

    JULSEDEBUG_CMD( "Heartbeat watchdog setup: expected_capture_time=%lu us, timeout=%lu us\n\r",
                    expected_capture_time_us, heartbeat_timeout_us );

    return true;
}

// Start the heartbeat watchdog monitoring
void julseview::start_heartbeat_watchdog( ) {
    // Record start time and reset state
    heartbeat_last_time = time_us_32( );
    heartbeat_triggered = false;

    JULSEDEBUG_CMD( "Heartbeat watchdog started - monitoring core 1 heartbeat\n\r" );
}

// Stop the heartbeat watchdog monitoring
void julseview::stop_heartbeat_watchdog( ) {
    heartbeat_triggered = false;
    JULSEDEBUG_CMD( "Heartbeat watchdog stopped\n\r" );
}

// Send heartbeat from core 1 (called during capture loop)
void julseview::send_heartbeat( ) {
    heartbeat_last_time = time_us_32( );
}

// Check heartbeat timeout (called from core 0)
void julseview::check_heartbeat_watchdog( ) {

    if ( heartbeat_enabled == 0 ) {
        if ( heartbeat_watchdog_started == true ) {
            stop_heartbeat_watchdog( );
            heartbeat_watchdog_started = false;
        }
        return;
    }
    if ( heartbeat_enabled == 1 && heartbeat_watchdog_started == false ) {
        setup_heartbeat_watchdog( );
        start_heartbeat_watchdog( );
        heartbeat_watchdog_started = true;
    }

    static uint32_t heartbeatCount = 0;

    uint32_t current_time = time_us_32( );
    uint32_t elapsed_time = current_time - heartbeat_last_time;
    heartbeatCount++;
    if ( heartbeatCount % 5000 == 0 ) {
        // JULSEDEBUG_CMD("Heartbeat elapsed_time = %lu us, heartbeat_timeout_us = %lu us, heatbeatCount = %d core %d\n\r", elapsed_time, heartbeat_timeout_us, heatbeatCount, rp2040.cpuid());
    }

    if ( elapsed_time > heartbeat_timeout_us ) {
        JULSEDEBUG_ERR( "HEARTBEAT TIMEOUT: No heartbeat for %lu us (timeout: %lu us)\n\r",
                        elapsed_time, heartbeat_timeout_us );

        // Set the watchdog triggered flag
        heartbeat_triggered = true;

        // Force cleanup by setting julseview_active to false
        // This will break the main capture loop
        // julseview_active = false;
        handle_heartbeat_watchdog_timeout( );
    }
}

// Handle heartbeat watchdog timeout - restart core 1
void julseview::handle_heartbeat_watchdog_timeout( ) {
    JULSEDEBUG_ERR( "=== HEARTBEAT WATCHDOG TIMEOUT - RESTARTING CORE 1 ===\n\r" );

    // Stop the heartbeat monitoring
    stop_heartbeat_watchdog( );

    // Send error completion signal to host

    julseview_active = false;
    isRunning = false;
    isArmed = false;
    send_capture_completion_signal( true );

    // Restart core 1 using rp2040.restartCore1()
    JULSEDEBUG_ERR( "Restarting core 1 due to heartbeat timeout\n\r" );
    // rp2040.restartCore1();

    JULSEDEBUG_ERR( "Core 1 restart initiated\n\r" );
}

// Calculate expected capture time based on sample rate and sample count
uint32_t julseview::calculate_expected_capture_time( ) {
    if ( sample_rate == 0 || num_samples == 0 ) {
        return 1000000; // Default 1 second if invalid parameters
    }

    // Calculate time in milliseconds: (samples * 1,000) / sample_rate
    uint32_t expected_time_ms = ( num_samples * 1000UL ) / sample_rate;

    // Add 10% buffer for processing overhead
    expected_time_ms = ( expected_time_ms * 110 ) / 100;

    // Ensure minimum expected time of 1000ms
    if ( expected_time_ms < 10000 ) {
        expected_time_ms = 10000;
    }

    // Ensure maximum expected time of 30 seconds
    if ( expected_time_ms > 30000 ) {
        expected_time_ms = 30000;
    }

    return expected_time_ms;
}

// Check if watchdog was triggered during last capture
bool julseview::was_watchdog_triggered( ) const {
    return heartbeat_triggered;
}

// Send capture completion signal to host
void julseview::send_capture_completion_signal( bool is_watchdog_timeout ) {
    // Prevent duplicate completion signals
    if ( completion_signal_sent ) {
        JULSEDEBUG_CMD( "Completion signal already sent - skipping\n\r" );
        return;
    }

    if ( is_watchdog_timeout ) {
        JULSEDEBUG_CMD( "Watchdog timeout - sending error completion signal\n\r" );
        char error_str[ 64 ];
        sprintf( error_str, "$WATCHDOG_TIMEOUT+" );
        julseview_usb_out_chars( error_str, strlen( error_str ) );
        JULSEDEBUG_CMD( "Watchdog timeout signal sent: %s\n\r", error_str );
        completion_signal_sent = true;
    } else if ( scnt >= num_samples ) {
        JULSEDEBUG_CMD( "Capture complete - sending final completion signal\n\r" );
        JULSEDEBUG_CMD( "Final sample count: %d/%d, total bytes: %d\n\r", scnt, num_samples, scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ) );

        // CRITICAL: Stop DMA before sending completion signal
        end( );

        // CRITICAL FINAL FLUSH: Ensure ALL remaining data is transmitted before completion signal
        JULSEDEBUG_CMD( "Performing final buffer flush - txbufidx: %d bytes\n\r", txbufidx );

        // Force flush any remaining data in txbuf regardless of threshold
        if ( txbufidx > 0 ) {
            check_tx_buf( 1 ); // Force transmission of all remaining data
            JULSEDEBUG_CMD( "Final flush attempt completed - txbufidx now: %d bytes\n\r", txbufidx );
        }

        // ADDITIONAL SAFETY: Multiple USB processing cycles to ensure all data is transmitted
        for ( int flush_cycles = 0; flush_cycles < 10; flush_cycles++ ) {
            tud_task( );              // Process USB events
            delayMicroseconds( 100 ); // Allow USB processing time

            // Check if there's still data in USB buffer that needs to be transmitted
            int usb_available = tud_cdc_n_write_available( 2 );
            if ( usb_available >= 16384 - 1024 ) { // Nearly empty (within 1KB of full capacity)
                JULSEDEBUG_CMD( "USB buffer drained after %d cycles (available: %d)\n\r", flush_cycles + 1, usb_available );
                break;
            }
        }

        // FINAL USB FLUSH: Ensure everything is sent
        tud_cdc_n_write_flush( 2 );
        tud_task( );
        delayMicroseconds( 500 ); // Final settling time

        char completion_str[ 32 ];
        int total_bytes = scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ); // Total bytes sent
        sprintf( completion_str, "$%d+", total_bytes );

        JULSEDEBUG_CMD( "Sending completion signal after final flush\n\r" );
        julseview_usb_out_chars( completion_str, strlen( completion_str ) );
        JULSEDEBUG_CMD( "Final completion signal sent: %s\n\r", completion_str );
        completion_signal_sent = true;
    } else {
        JULSEDEBUG_CMD( "Capture incomplete - sending partial completion signal\n\r" );
        JULSEDEBUG_CMD( "Partial sample count: %d/%d, total bytes: %d\n\r", scnt, num_samples, scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ) );

        // CRITICAL: Stop DMA before sending completion signal
        end( );

        // CRITICAL FINAL FLUSH: Ensure ALL remaining data is transmitted before partial completion signal
        JULSEDEBUG_CMD( "Performing final buffer flush for partial completion - txbufidx: %d bytes\n\r", txbufidx );

        // Force flush any remaining data in txbuf regardless of threshold
        if ( txbufidx > 0 ) {
            check_tx_buf( 1 ); // Force transmission of all remaining data
            JULSEDEBUG_CMD( "Partial final flush attempt completed - txbufidx now: %d bytes\n\r", txbufidx );
        }

        // ADDITIONAL SAFETY: Multiple USB processing cycles to ensure all data is transmitted
        for ( int flush_cycles = 0; flush_cycles < 10; flush_cycles++ ) {
            tud_task( );              // Process USB events
            delayMicroseconds( 100 ); // Allow USB processing time

            // Check if there's still data in USB buffer that needs to be transmitted
            int usb_available = tud_cdc_n_write_available( 2 );
            if ( usb_available >= 16384 - 1024 ) { // Nearly empty (within 1KB of full capacity)
                JULSEDEBUG_CMD( "USB buffer drained for partial completion after %d cycles (available: %d)\n\r", flush_cycles + 1, usb_available );
                break;
            }
        }

        // FINAL USB FLUSH: Ensure everything is sent
        tud_cdc_n_write_flush( 2 );
        tud_task( );
        delayMicroseconds( 500 ); // Final settling time

        char completion_str[ 32 ];
        int total_bytes = scnt * ( d_tx_bps + ( a_chan_cnt * 2 ) ); // Total bytes sent
        sprintf( completion_str, "$%d+", total_bytes );

        JULSEDEBUG_CMD( "Sending partial completion signal after final flush\n\r" );
        julseview_usb_out_chars( completion_str, strlen( completion_str ) );
        JULSEDEBUG_CMD( "Partial completion signal sent: %s\n\r", completion_str );
        completion_signal_sent = true;
    }
}

// Test function to benchmark USB transmission performance
void julseview::test_usb_performance( ) {
    JULSEDEBUG_CMD( "=== USB Performance Test ===\n\r" );

    // Test data buffer
    uint8_t test_data[ 1024 ];
    for ( int i = 0; i < 1024; i++ ) {
        test_data[ i ] = i & 0xFF; // Simple pattern
    }

    // Test 1: Small chunks (64 bytes)
    uint32_t start_time = micros( );
    for ( int i = 0; i < 10; i++ ) {
        this->julseview_usb_out_chars( (char*)test_data, 64 );
    }
    uint32_t small_chunk_time = micros( ) - start_time;

    // Test 2: Medium chunks (256 bytes)
    start_time = micros( );
    for ( int i = 0; i < 10; i++ ) {
        this->julseview_usb_out_chars( (char*)test_data, 256 );
    }
    uint32_t medium_chunk_time = micros( ) - start_time;

    // Test 3: Large chunks (1024 bytes)
    start_time = micros( );
    for ( int i = 0; i < 10; i++ ) {
        this->julseview_usb_out_chars( (char*)test_data, 1024 );
    }
    uint32_t large_chunk_time = micros( ) - start_time;

    // Calculate throughput
    uint32_t small_bytes = 10 * 64;
    uint32_t medium_bytes = 10 * 256;
    uint32_t large_bytes = 10 * 1024;

    float small_throughput = (float)small_bytes / ( small_chunk_time / 1000000.0f );
    float medium_throughput = (float)medium_bytes / ( medium_chunk_time / 1000000.0f );
    float large_throughput = (float)large_bytes / ( large_chunk_time / 1000000.0f );

    JULSEDEBUG_CMD( "Small chunks (64 bytes x 10): %d us, %.2f KB/s\n\r",
                    small_chunk_time, small_throughput / 1024.0f );
    JULSEDEBUG_CMD( "Medium chunks (256 bytes x 10): %d us, %.2f KB/s\n\r",
                    medium_chunk_time, medium_throughput / 1024.0f );
    JULSEDEBUG_CMD( "Large chunks (1024 bytes x 10): %d us, %.2f KB/s\n\r",
                    large_chunk_time, large_throughput / 1024.0f );
    JULSEDEBUG_CMD( "Optimal chunk size: %s\n\r",
                    ( large_throughput > medium_throughput && large_throughput > small_throughput ) ? "Large" : ( medium_throughput > small_throughput ) ? "Medium"
                                                                                                                                                         : "Small" );
    JULSEDEBUG_CMD( "=== Test Complete ===\n\r" );
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
