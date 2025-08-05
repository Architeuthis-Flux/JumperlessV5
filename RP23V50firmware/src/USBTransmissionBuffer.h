#ifndef USB_TRANSMISSION_BUFFER_H
#define USB_TRANSMISSION_BUFFER_H

#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <hardware/dma.h>
#include <pico/platform.h>

/**
 * USBTransmissionBuffer - High-performance dual-core USB data transmission
 * 
 * Core 0 (producer): Queues data using DMA transfers
 * Core 1 (consumer): Processes and transmits data to USB serial
 * 
 * Features:
 * - 8KB circular buffer for high throughput
 * - DMA-based data queuing from Core 0
 * - Rate-limited transmission on Core 1
 * - Thread-safe design for dual-core operation
 * - Reusable for any USB data transmission needs
 */
class USBTransmissionBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 8192;  // 8KB buffer
    static constexpr size_t MAX_CHUNK_SIZE = 512;  // Max USB transmission chunk
    static constexpr uint32_t RATE_LIMIT_MS = 1;   // Rate limiting between large transmissions
    static constexpr uint32_t FLUSH_DELAY_US = 100; // Delay between chunks

    struct Stats {
        uint32_t total_queued_bytes;
        uint32_t total_sent_bytes;
        uint32_t queue_operations;
        uint32_t send_operations;
        uint32_t dma_errors;
        uint32_t buffer_overflows;
        uint32_t current_buffer_usage;
        bool dma_active;
    };

    USBTransmissionBuffer();
    ~USBTransmissionBuffer();

    // Initialization - call once during startup
    bool begin();

    // === CORE 0 METHODS (Data Producers) ===
    
    /**
     * Queue data for transmission using DMA (blocking)
     * @param data Pointer to data to queue
     * @param length Number of bytes to queue
     * @return true if successfully queued, false if buffer full or error
     */
    bool queueData(const void* data, size_t length);
    
    /**
     * Queue data for transmission using DMA (non-blocking)
     * @param data Pointer to data to queue  
     * @param length Number of bytes to queue
     * @return true if DMA transfer started, false if busy or error
     */
    bool queueDataAsync(const void* data, size_t length);
    
    /**
     * Check if async DMA transfer is complete
     * @return true if DMA is idle and ready for next transfer
     */
    bool isDMAReady() const;

    // === CORE 1 METHODS (Data Consumer) ===
    
    /**
     * Process available buffer data and transmit via USB serial
     * @param serial Reference to USB serial port
     * @return Number of bytes transmitted in this call
     */
    size_t processBuffer(Adafruit_USBD_CDC& serial);
    
    /**
     * Check if data is available for transmission
     * @return true if buffer contains data to send
     */
    bool hasData() const;
    
    /**
     * Get amount of data available for transmission
     * @return Number of bytes ready to send
     */
    size_t getAvailableData() const;

    // === STATUS AND DEBUG ===
    
    /**
     * Get transmission statistics
     * @param stats Reference to stats structure to fill
     */
    void getStats(Stats& stats) const;
    
    /**
     * Get buffer utilization percentage
     * @return Percentage of buffer currently in use (0-100)
     */
    uint8_t getBufferUtilization() const;
    
    /**
     * Reset statistics counters
     */
    void resetStats();

private:
    // Buffer management
    uint8_t* buffer_;
    volatile size_t write_pos_;    // Where next data will be written
    volatile size_t read_pos_;     // Where next data will be read from  
    volatile size_t data_size_;    // Amount of data currently in buffer
    
    // DMA management
    int dma_channel_;
    volatile bool dma_active_;
    
    // Timing and flow control
    uint32_t last_transmission_time_;
    
    // Statistics
    mutable Stats stats_;
    
    // Internal helper methods
    size_t getAvailableSpace() const;
    size_t getContiguousWriteSpace() const;
    size_t getContiguousReadSpace() const;
    bool waitForDMAComplete(uint32_t timeout_ms = 100);
    static void dmaIRQHandler();
    static USBTransmissionBuffer* instance_; // For IRQ handler
};

#endif // USB_TRANSMISSION_BUFFER_H