#ifndef JULSEVIEW_H
#define JULSEVIEW_H

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef PICO_STDIO_USB_STDOUT_TIMEOUT_US
#define PICO_STDIO_USB_STDOUT_TIMEOUT_US 500000
#endif

// Using actual Jumperless hardware channel counts
#define JULSEVIEW_MAX_ANALOG_CHANNELS 9
#define JULSEVIEW_MAX_DIGITAL_CHANNELS 16

// Default channel counts for device identification
#define JULSEVIEW_DEFAULT_ANALOG_CHANNELS 5
#define JULSEVIEW_DEFAULT_DIGITAL_CHANNELS 8

// Buffer and clock settings - Producer-Consumer ping-pong design
// Buffer size configuration - larger buffers for stability

#define JULSEVIEW_ANALOG_BUF_SIZE 65536  // 64KB for analog (8 channels × 4096 samples × 2 bytes)
#define JULSEVIEW_DIGITAL_BUF_SIZE 32768   // 32KB for digital (16KB each for ping-pong)
#define JULSEVIEW_DMA_BUF_SIZE JULSEVIEW_ANALOG_BUF_SIZE + JULSEVIEW_DIGITAL_BUF_SIZE
#define JULSEVIEW_TX_BUF_SIZE 2600
#define JULSEVIEW_TX_BUF_THRESH 64    // Increased from 20 to reduce USB overhead
#define JULSEVIEW_USB_FLUSH_DELAY_US 100  // Microsecond delay between USB transmissions
#define JULSEVIEW_USB_MAX_BLOCK_SIZE 128   // Maximum bytes per USB transmission
#define JULSEVIEW_USB_RATE_LIMIT_MS 1      // Minimum milliseconds between large transmissions
#define JULSEVIEW_SYS_CLK_BASE 150000

// Decimation configuration constants
#define JULSEVIEW_ADC_MAX_RATE 200000  // Maximum safe ADC sample rate (200kHz total)
#define JULSEVIEW_DECIMATION_MIN_FACTOR 1  // Minimum decimation factor
#define JULSEVIEW_DECIMATION_MAX_FACTOR 100  // Maximum decimation factor


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
    bool getShouldStopOtherStuff() const { return isRunning || isTriggered; }
    
    // Debug method to print current state
    void printState() const;

    bool isInitialized() const { return initialized; }

private:
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
    
    // Debugging configuration
    bool use_dual_dma = true;  // true = ping-pong DMA, false = single DMA channel
    bool use_smart_pingpong = true;  // true = round-robin aware ping-pong, false = simple ping-pong
    bool use_coordinator_dma = false;  // true = DMA coordinator for hardware-timed switching
    bool use_producer_consumer = true;  // true = producer-consumer ping-pong with buffer clearing
    bool use_single_buffer = false;  // true = single buffer mode, false = producer-consumer mode

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

    // --- Buffers & Pointers ---
    uint8_t *capture_buf;
    uint8_t *raw_capture_buf;  // Raw unaligned buffer pointer for freeing
    uint32_t d_size, a_size;
    uint32_t dbuf0_start, dbuf1_start, abuf0_start, abuf1_start;
    uint32_t samples_per_half;
    uint32_t actual_buffer_size;  // Track actual allocated buffer size
    uint8_t txbuf[JULSEVIEW_TX_BUF_SIZE];
    uint16_t txbufidx;

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
    // uint32_t last_analog_sample_index;     // REMOVED - driver now handles duplication
    // uint16_t last_analog_values[JULSEVIEW_MAX_ANALOG_CHANNELS]; // REMOVED - driver now handles duplication

    // --- Private Methods ---
    void reset();
    void cleanup_dma_channels();
    void tx_init();
    void applySettings();
    bool process_char(char charin);
    void arm();
    void run();
    void deinit();
    void send_data();
    bool julseview_usb_out_chars(const char *buf, int length);  // Move to member function

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
    void check_rle();
    void check_tx_buf(uint16_t cnt);
    void tx_d_samp(uint32_t cval);
    uint32_t get_cval(uint8_t *dbuf);
    
    // Calibration functions
    void get_calibrated_analog_scaling(uint8_t channel, uint32_t* scale_microvolts, int32_t* offset_microvolts);

    // DMA management
    int current_dma_half;
    bool dma_ended_flag;  // Flag to prevent multiple dma_end() calls
    void dma_check();
    void dma_end();  // Break ping-pong chaining and stop DMA channels

    // --- Decimation Methods ---
    void configure_decimation_mode();      // Configure decimation based on sample rate
    void calculate_asymmetric_buffer_layout(); // Calculate asymmetric buffer layout (always used)
    void duplicate_analog_sample(uint32_t sample_index); // Duplicate analog sample data (unused - driver handles)
    
    // Buffer readiness tracking - ensures buffers are only sent when completely full
    bool buffer0_ready_to_send;
    bool buffer1_ready_to_send;
    bool buffer0_being_transmitted;
    bool buffer1_being_transmitted;

};

#endif
