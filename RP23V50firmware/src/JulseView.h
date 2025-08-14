#ifndef JULSEVIEW_H
#define JULSEVIEW_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef PICO_STDIO_USB_STDOUT_TIMEOUT_US
#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000
#endif



extern bool la_enabled;
// Using actual Jumperless hardware channel counts
#define JULSEVIEW_MAX_ANALOG_CHANNELS 9
#define JULSEVIEW_MAX_DIGITAL_CHANNELS 16

// Default channel counts for device identification
#define JULSEVIEW_DEFAULT_ANALOG_CHANNELS 4
#define JULSEVIEW_DEFAULT_DIGITAL_CHANNELS 8

// Buffer and clock settings - Producer-Consumer ping-pong design
// Buffer size configuration - larger buffers for stability

#define JULSEVIEW_ANALOG_BUF_SIZE 32768 //+ 2048  // 64KB for analog (8 channels × 4096 samples × 2 bytes)
// Replace fixed digital size with a global variable declared below
extern uint32_t g_julseview_digital_buf_size;  // total digital region size (bytes)
#define JULSEVIEW_DMA_BUF_SIZE (JULSEVIEW_ANALOG_BUF_SIZE + g_julseview_digital_buf_size + 2048)
#define JULSEVIEW_TX_BUF_SIZE 2600
#define JULSEVIEW_TX_BUF_THRESH 64    // Reduced for more frequent, reliable transmission
#define JULSEVIEW_USB_FLUSH_DELAY_US 2000  // Microsecond delay between USB transmissions
#define JULSEVIEW_USB_MAX_BLOCK_SIZE 512   // Maximum bytes per USB transmission
#define JULSEVIEW_USB_RATE_LIMIT_MS 1      // Minimum milliseconds between large transmissions
#define JULSEVIEW_SYS_CLK_BASE 150000
#define JULSEVIEW_SLOW_THRESHOLD_HZ 3000

// Decimation configuration constants
#define JULSEVIEW_ADC_MAX_RATE 200000  // Maximum safe ADC sample rate (200kHz total)
#define JULSEVIEW_DECIMATION_MIN_FACTOR 1  // Minimum decimation factor
#define JULSEVIEW_DECIMATION_MAX_FACTOR 100  // Maximum decimation factor

extern uint32_t julseview_debug_mask;  
extern volatile bool julseview_active;

// Logic analyzer debug flag (kept for compatibility)
extern bool debugLA;

// RX state enum from the Jumperless driver
typedef enum {
    RX_IDLE = 0,        // Not receiving
    RX_ACTIVE = 1,      // Receiving data
    RX_STOPPED = 2,     // Received stop marker, waiting for byte cnt
    RX_ABORT = 3,       // Received aborted marker or other error
} jv_rxstate_t;

// Trigger types
typedef enum {
    TRIGGER_NONE = 0,
    TRIGGER_ANALOG_LEVEL = 1,
    TRIGGER_ANALOG_EDGE = 2,
    TRIGGER_DIGITAL_EDGE = 3,
    TRIGGER_INTERNAL_VAR = 4
} trigger_type_t;

// Edge types for edge triggering
typedef enum {
    EDGE_RISING = 0,
    EDGE_FALLING = 1,
    EDGE_EITHER = 2
} edge_type_t;

// Trigger configuration structure
typedef struct {
    trigger_type_t type;
    uint8_t channel;           // Channel number (analog or digital)
    uint16_t level;            // Analog level threshold (12-bit ADC value)
    edge_type_t edge;          // Edge type for edge triggering
    uint32_t var_address;      // Memory address for internal variable trigger
    uint32_t var_value;        // Expected value for internal variable trigger
    bool enabled;              // Whether trigger is enabled
} trigger_config_t;

// This class will handle the JulseView commands and data
// and send it to the custom sigrok driver for pulseview.
class julseview {
public:
    // Constructor to initialize member variables
    julseview();
    
    bool init();
    void handler();
    int getCommand();
    
    // --- State Access Methods ---
    bool getIsArmed() const { return isArmed; }
    bool getIsRunning() const { return isRunning; }
    bool getIsTriggered() const { return isTriggered; }
    bool getReceivedCommand() const { return receivedCommand; }
    bool isCommandTimedOut() const;
    void setCommandTimeout(uint32_t timeout_ms) { commandTimeout = timeout_ms; }
    uint32_t getCommandTimeout() const { return commandTimeout; }
    
    // Combined state check for UI blocking
    bool getShouldStopOtherStuff() const { return isRunning || isTriggered || isArmed; }
    

    bool isInitialized() const { return initialized; }



    void reset();
    void end();
    void tx_init();
    void applySettings();
    bool process_char(char charin);
    void arm();
    void run();
    void deinit();
    void send_data();

    // --- Control Channel Management (public for timer callback) ---
    uint8_t collect_control_channel_data();  // Collect control channel data into 8-bit value
    void setup_control_channel_dma();        // Setup DMA for control channel capture
    void update_control_channel_buffer();    // Update control channel buffer (called by timer)
    
    // --- Heartbeat Watchdog (public for core 0 access) ---
    void check_heartbeat_watchdog();         // Check heartbeat timeout (called from core 0)
    volatile int heartbeat_enabled;
    volatile bool heartbeat_watchdog_started;


        // --- Decimation Methods ---
        void configure_decimation_mode();      // Configure decimation based on sample rate
        void calculate_asymmetric_buffer_layout(); // Calculate asymmetric buffer layout (always used)
        void duplicate_analog_sample(uint32_t sample_index); // Duplicate analog sample data (unused - driver handles)
    
        
        // --- Configuration ---
        uint32_t sample_rate;
        uint32_t num_samples;
        uint32_t a_mask;
        uint32_t d_mask;
        uint32_t c_mask;  // Control channel mask
        uint8_t a_chan_cnt;
        uint8_t d_chan_cnt;
        uint8_t c_chan_cnt;  // Control channel count
        uint8_t d_tx_bps;          // Digital Transmit bytes per slice
        uint8_t pin_count;
        uint8_t d_nps;             // digital nibbles per slice from a PIO/DMA perspective
        bool use_rle;


            // --- State ---
    volatile bool initialized;
    volatile bool started;
    volatile bool sending;
    volatile bool armed;  // Track if device is armed and ready for trigger
    volatile bool cont; // continuous mode
    volatile bool aborted;
    jv_rxstate_t rxstate;
    
    // --- Enhanced State Tracking ---
    volatile bool isArmed;        // Device is armed and ready for trigger
    volatile bool isRunning;      // Device is actively capturing data
    volatile bool isTriggered;    // Trigger condition has been detected
    volatile bool receivedCommand; // Command received from host
    volatile uint32_t lastCommandTime; // Timestamp of last command (for timeout)
    
    volatile uint32_t commandTimeout;  // Timeout duration in milliseconds 

    // --- Trigger System ---
    trigger_config_t trigger_config;
    volatile bool trigger_armed;     // Trigger is armed and monitoring
    volatile bool trigger_detected;  // Trigger condition has been detected
    volatile uint32_t pre_trigger_samples;  // Number of samples to capture before trigger (forced to 0)
    volatile uint32_t post_trigger_samples; // Number of samples to capture after trigger

    // --- Simple Heartbeat Watchdog ---
    volatile uint32_t last_heartbeat_time;     // Last heartbeat time from core 1 (microseconds)
    volatile uint32_t heartbeat_timeout_us;    // Timeout duration (microseconds)
    volatile bool heartbeat_watchdog_triggered; // Flag indicating watchdog has triggered
    volatile uint32_t expected_capture_time_us; // Expected capture time based on sample rate and count
    volatile bool completion_signal_sent;      // Flag to prevent duplicate completion signals


private:

    
    // Debugging configuration
    bool use_dual_dma = true;  // true = ping-pong DMA, false = single DMA channel
    bool use_smart_pingpong = true;  // true = round-robin aware ping-pong, false = simple ping-pong
    bool use_coordinator_dma = false;  // true = DMA coordinator for hardware-timed switching
    bool use_producer_consumer = true;  // true = producer-consumer ping-pong with buffer clearing
    bool use_single_buffer = false;  // true = single buffer mode, false = producer-consumer mode


    bool single_buffer_mode = false;

    // --- Buffers & Pointers ---
    uint8_t *capture_buf;
    
    uint32_t d_size, a_size;
    uint32_t dbuf0_start, dbuf1_start, abuf0_start, abuf1_start;
    uint32_t samples_per_half;
    uint32_t actual_buffer_size;  // Track actual allocated buffer size
    uint8_t txbuf[JULSEVIEW_TX_BUF_SIZE];
    uint16_t txbufidx;
    
    // --- Control Channel DMA ---
    int control_dma_chan0 = -1;     // DMA channel 0 for control data (writes to cbuf0)
    int control_dma_chan1 = -1;     // DMA channel 1 for control data (writes to cbuf1)
    dma_channel_config ccfg0;       // DMA config for control channel 0
    dma_channel_config ccfg1;       // DMA config for control channel 1
    volatile uint32_t *taddrc0, *taddrc1; // Control DMA write address pointers
    volatile uint32_t *tstsc0, *tstsc1;   // Control DMA status pointers
    uint32_t cbuf0_start, cbuf1_start;    // Control buffer start addresses (within digital buffer)

    // --- Command Processing ---
    char cmdstr[20];
    uint8_t cmdstrptr;
    char rspstr[20];
    
    // --- Sample & Byte Counting ---
    uint32_t scnt; // number of samples sent
    uint64_t byte_cnt;
    uint32_t original_num_samples;  // Store original num_samples to prevent corruption
    uint32_t dma_start_time;  // Track DMA start time for timeout detection

    // --- Transmission Rate Limiting ---
    uint32_t last_transmission_time;  // Track last transmission time for rate limiting
    uint32_t transmission_count;      // Count transmissions for debugging

    // --- RLE state ---
    uint32_t rlecnt;
    uint32_t lval, cval; // last and current digital sample values
    
    // --- Helper variables for slice processing ---
    uint32_t samp_remain;  // remaining samples to process
    uint32_t rxbufdidx;    // receive buffer index

    // --- Decimation Configuration ---
    bool use_decimation_mode;              // Whether we're in high-speed decimation mode
    uint32_t analog_decimation_factor;     // How many digital samples per analog sample
    uint32_t digital_samples_per_half;     // Digital samples per buffer (larger)
    uint32_t analog_samples_per_half;      // Analog samples per buffer (smaller)
    
    // --- Control Channel Timer ---
    repeating_timer_t control_timer;       // Timer for control channel updates
    uint32_t control_sample_rate;          // Sample rate for control channels
    // uint32_t last_analog_sample_index;     // REMOVED - driver now handles duplication
    // uint16_t last_analog_values[JULSEVIEW_MAX_ANALOG_CHANNELS]; // REMOVED - driver now handles duplication

    // --- Private Methods ---

    bool julseview_usb_out_chars(const char *buf, int length);  // Move to member function

    void test_usb_performance();  // Performance benchmark function

    // Trigger methods
    void setup_trigger();
    bool check_trigger_condition(uint16_t analog_value, uint8_t digital_value);
    void handle_trigger_detection();
    void start_triggered_capture();
    void configure_analog_trigger(uint8_t channel, uint16_t level, edge_type_t edge);
    void configure_digital_trigger(uint8_t channel, edge_type_t edge);
    void configure_internal_trigger(uint32_t var_address, uint32_t var_value);
    void waitForTrigger();
    
    // Internal trigger check methods
    bool check_analog_level_trigger(uint16_t analog_value);
    bool check_analog_edge_trigger(uint16_t analog_value);
    bool check_digital_edge_trigger(uint8_t digital_value);
    bool check_internal_var_trigger();
    
    // Trigger data handling
    // Pre-trigger functionality removed - simplified to post-trigger only

    // Data sending helpers
    void send_slices_D4(uint8_t *dbuf);
    void send_slice_init(uint8_t *dbuf);
    void send_slices_1B(uint8_t *dbuf);
    void send_slices_2B(uint8_t *dbuf);
    void send_slices_4B(uint8_t *dbuf);
    void send_slices_analog(uint8_t *dbuf, uint8_t *abuf);
    
    // Firmware-side decimation helpers
    void process_analog_sample(uint8_t* abuf, uint32_t sample_index);
    void reset_firmware_decimation_state();
    bool should_read_analog_sample(uint32_t sample_index);  // Deprecated but kept for compatibility
    uint32_t get_analog_buffer_offset(uint32_t sample_index);
    void check_rle();
    void check_tx_buf(uint16_t cnt);
    
    // send_slices_analog helper functions
    uint32_t calculate_sample_count();
    bool validate_buffer_bounds(uint32_t samples_to_process);
    uint32_t read_digital_sample(uint8_t* dbuf, uint32_t sample_index);
    void encode_digital_data(uint32_t digital_value);
    void tx_d_samp(uint32_t cval);
    uint32_t get_cval(uint8_t *dbuf);
    
    // Calibration functions
    void get_calibrated_analog_scaling(uint8_t channel, uint32_t* scale_microvolts, int32_t* offset_microvolts);

    // DMA management
    int current_dma_half;
    bool dma_ended_flag;  // Flag to prevent multiple end() calls
    void dma_check();
    
    // PIO management
    int piosm;  // PIO state machine number (signed to allow -1 for unclaimed)
    // Track our loaded PIO program to remove it safely when switching modes
    int pio_loaded_offset = -1;
    bool pio_loaded_is_slow = false;

    // --- Arm refactor helpers ---
    void armAnalogSlow();
    void armAnalogFast();
    void armDigitalSlow();
    void armDigitalFast();
    void armAnalogCommon();
    void armDigitalSelected(bool use_slow_program);


    // --- Debugging ---
    void julseview_set_debug_mask( uint32_t mask );
    void julseview_set_debug_categories( const char* categories );


    uint32_t calculate_expected_capture_time(); // Calculate expected capture time based on sample rate and count


    // --- Capture Completion Methods ---
    void send_capture_completion_signal(bool is_watchdog_timeout = false); // Send completion signal to host

    // Buffer readiness tracking - ensures buffers are only sent when completely full
    bool buffer0_ready_to_send;
    bool buffer1_ready_to_send;
    bool buffer0_being_transmitted;
    bool buffer1_being_transmitted;

};

julseview julseview;

extern volatile bool julseview_active;

extern volatile bool pyTrigger;

extern volatile bool control_D[4];
extern volatile float control_A[4];
// Global control byte sampled by DMA; may be updated by other subsystems (e.g., MicroPython)
extern volatile uint8_t control_data;






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
// NOTE: When debugLA is false, category-based debug output is suppressed without altering the saved mask
#define JULSEDEBUG_CHECK( category ) ( debugLA && ( julseview_debug_level >= JULSEDEBUG_LEVEL_INFO ) && ( julseview_debug_mask & category ) )

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

 
// Debug configuration helper function prototypes
void julseview_set_debug_mask( uint32_t mask );
void julseview_set_debug_categories( const char* categories );
 
#endif
