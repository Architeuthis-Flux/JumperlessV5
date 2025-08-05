#include "USBTransmissionBuffer.h"
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <pico/mutex.h>

// Static instance pointer for IRQ handler
USBTransmissionBuffer* USBTransmissionBuffer::instance_ = nullptr;

USBTransmissionBuffer::USBTransmissionBuffer() 
    : buffer_(nullptr)
    , write_pos_(0)
    , read_pos_(0) 
    , data_size_(0)
    , dma_channel_(-1)
    , dma_active_(false)
    , last_transmission_time_(0)
    , stats_{}
{
    // Set static instance for IRQ handler
    instance_ = this;
}

USBTransmissionBuffer::~USBTransmissionBuffer() {
    if (buffer_) {
        free(buffer_);
        buffer_ = nullptr;
    }
    
    if (dma_channel_ >= 0) {
        dma_channel_unclaim(dma_channel_);
    }
    
    instance_ = nullptr;
}

bool USBTransmissionBuffer::begin() {
    // Allocate aligned buffer for DMA efficiency
    buffer_ = (uint8_t*)malloc(BUFFER_SIZE);
    if (!buffer_) {
        return false;
    }
    
    // Clear buffer
    memset(buffer_, 0, BUFFER_SIZE);
    
    // Claim DMA channel
    dma_channel_ = dma_claim_unused_channel(false);
    if (dma_channel_ < 0) {
        free(buffer_);
        buffer_ = nullptr;
        return false;
    }
    
    // Configure DMA channel
    dma_channel_config config = dma_channel_get_default_config(dma_channel_);
    channel_config_set_transfer_data_size(&config, DMA_SIZE_8);  // 8-bit transfers
    channel_config_set_read_increment(&config, true);           // Increment source
    channel_config_set_write_increment(&config, true);          // Increment destination
    channel_config_set_dreq(&config, DREQ_FORCE);              // Software triggered
    
    // Set up IRQ for DMA completion
    dma_channel_set_irq0_enabled(dma_channel_, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dmaIRQHandler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    dma_channel_configure(dma_channel_, &config, nullptr, nullptr, 0, false);
    
    // Initialize stats
    resetStats();
    
    return true;
}

bool USBTransmissionBuffer::queueData(const void* data, size_t length) {
    if (!buffer_ || !data || length == 0) {
        return false;
    }
    
    // Check if we have enough space
    if (getAvailableSpace() < length) {
        stats_.buffer_overflows++;
        return false;
    }
    
    // Wait for any pending DMA to complete
    if (!waitForDMAComplete()) {
        stats_.dma_errors++;
        return false;
    }
    
    // Start DMA transfer
    return queueDataAsync(data, length);
}

bool USBTransmissionBuffer::queueDataAsync(const void* data, size_t length) {
    if (!buffer_ || !data || length == 0 || dma_active_) {
        return false;
    }
    
    // Check available space
    if (getAvailableSpace() < length) {
        stats_.buffer_overflows++;
        return false;
    }
    
    // Handle circular buffer wrap-around
    size_t contiguous_space = getContiguousWriteSpace();
    
    if (length <= contiguous_space) {
        // Single DMA transfer - no wrap around
        dma_active_ = true;
        
        // Set up DMA addresses and transfer count, then trigger
        dma_channel_set_write_addr(dma_channel_, buffer_ + write_pos_, false);
        dma_channel_set_read_addr(dma_channel_, data, false);  
        dma_channel_set_trans_count(dma_channel_, length, true);  // trigger on count set
        
        // Update write position (will be finalized in IRQ handler)
        // Note: We update this optimistically, IRQ handler will confirm
        write_pos_ = (write_pos_ + length) % BUFFER_SIZE;
        __atomic_add_fetch(&data_size_, length, __ATOMIC_SEQ_CST);
        
    } else {
        // Two-part transfer required - handle wrap around
        // For now, fall back to memcpy to keep it simple
        // TODO: Implement dual-DMA for wraparound if needed for performance
        
        // First part: to end of buffer
        memcpy(buffer_ + write_pos_, data, contiguous_space);
        
        // Second part: from beginning of buffer  
        size_t remaining = length - contiguous_space;
        memcpy(buffer_, (const uint8_t*)data + contiguous_space, remaining);
        
        // Update positions
        write_pos_ = remaining;
        __atomic_add_fetch(&data_size_, length, __ATOMIC_SEQ_CST);
    }
    
    // Update stats
    stats_.total_queued_bytes += length;
    stats_.queue_operations++;
    
    return true;
}

bool USBTransmissionBuffer::isDMAReady() const {
    return !dma_active_;
}

size_t USBTransmissionBuffer::processBuffer(Adafruit_USBD_CDC& serial) {
    if (!hasData()) {
        return 0;
    }
    
    // Rate limiting for large transmissions
    uint32_t current_time = millis();
    if (current_time - last_transmission_time_ < RATE_LIMIT_MS) {
        return 0;  // Rate limited
    }
    
    // Calculate how much we can send in this chunk
    size_t available = getAvailableData();
    size_t to_send = (available > MAX_CHUNK_SIZE) ? MAX_CHUNK_SIZE : available;
    size_t contiguous = getContiguousReadSpace();
    
    if (to_send > contiguous) {
        to_send = contiguous;  // Don't cross buffer boundary in single transmission
    }
    
    // Check USB buffer availability
    size_t usb_space = serial.availableForWrite();
    if (usb_space < to_send) {
        to_send = usb_space;  // Don't overwhelm USB buffer
    }
    
    if (to_send == 0) {
        return 0;
    }
    
    // Transmit data
    size_t sent = serial.write(buffer_ + read_pos_, to_send);
    
    if (sent > 0) {
        // Update read position
        read_pos_ = (read_pos_ + sent) % BUFFER_SIZE;
        __atomic_sub_fetch(&data_size_, sent, __ATOMIC_SEQ_CST);
        
        // Update stats
        stats_.total_sent_bytes += sent;
        stats_.send_operations++;
        
        // Update timing
        last_transmission_time_ = current_time;
        
        // Force flush for small packets, delay for flow control on large ones
        if (sent < 64) {
            serial.flush();
        } else {
            delayMicroseconds(FLUSH_DELAY_US);
        }
    }
    
    return sent;
}

bool USBTransmissionBuffer::hasData() const {
    return __atomic_load_n(&data_size_, __ATOMIC_SEQ_CST) > 0;
}

size_t USBTransmissionBuffer::getAvailableData() const {
    return __atomic_load_n(&data_size_, __ATOMIC_SEQ_CST);
}

void USBTransmissionBuffer::getStats(Stats& stats) const {
    stats = stats_;
    stats.current_buffer_usage = getAvailableData();
    stats.dma_active = dma_active_;
}

uint8_t USBTransmissionBuffer::getBufferUtilization() const {
    size_t used = getAvailableData();
    return (uint8_t)((used * 100) / BUFFER_SIZE);
}

void USBTransmissionBuffer::resetStats() {
    memset((void*)&stats_, 0, sizeof(stats_));
}

// === PRIVATE HELPER METHODS ===

size_t USBTransmissionBuffer::getAvailableSpace() const {
    return BUFFER_SIZE - __atomic_load_n(&data_size_, __ATOMIC_SEQ_CST);
}

size_t USBTransmissionBuffer::getContiguousWriteSpace() const {
    if (write_pos_ >= read_pos_) {
        // Write position is ahead or equal - space is from write_pos to end, 
        // but only if there's data or read_pos > 0 (to avoid full buffer ambiguity)
        if (data_size_ == 0) {
            return BUFFER_SIZE - write_pos_;  // Empty buffer
        } else {
            size_t space_to_end = BUFFER_SIZE - write_pos_;
            size_t space_to_read = (read_pos_ == 0) ? 0 : read_pos_;
            return (space_to_end > 0) ? space_to_end : space_to_read;
        }
    } else {
        // Read position is ahead - space is from write_pos to read_pos
        return read_pos_ - write_pos_;
    }
}

size_t USBTransmissionBuffer::getContiguousReadSpace() const {
    size_t available = getAvailableData();
    if (available == 0) {
        return 0;
    }
    
    if (read_pos_ <= write_pos_) {
        // Normal case - data from read_pos to write_pos
        return write_pos_ - read_pos_;
    } else {
        // Wrapped case - data from read_pos to end of buffer
        return BUFFER_SIZE - read_pos_;
    }
}

bool USBTransmissionBuffer::waitForDMAComplete(uint32_t timeout_ms) {
    if (!dma_active_) {
        return true;
    }
    
    uint32_t start = millis();
    while (dma_active_ && (millis() - start) < timeout_ms) {
        tight_loop_contents();  // Yield to other core
    }
    
    if (dma_active_) {
        stats_.dma_errors++;
        return false;
    }
    
    return true;
}

void USBTransmissionBuffer::dmaIRQHandler() {
    if (instance_ && instance_->dma_channel_ >= 0) {
        // Check if our channel triggered the IRQ
        if (dma_channel_get_irq0_status(instance_->dma_channel_)) {
            // Clear the interrupt
            dma_channel_acknowledge_irq0(instance_->dma_channel_);
            
            // Mark DMA as complete
            instance_->dma_active_ = false;
        }
    }
}