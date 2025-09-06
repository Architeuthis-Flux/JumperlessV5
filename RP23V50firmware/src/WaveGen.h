/*!
 *  @file waveGen.h
 *
 *   Waveform Generator using MCP4728
 *  Provides high-performance, non-blocking waveform generation
 */

#ifndef _WAVEGEN_H
#define _WAVEGEN_H

#include "Arduino.h"
#include "MCP4728.h"

// Waveform types
typedef enum {
    WAVEGEN_SINE,
    WAVEGEN_TRIANGLE,
    WAVEGEN_SAWTOOTH,
    WAVEGEN_SQUARE,
} waveGen_waveform_t;

// Channel selection
typedef enum {
    WAVEGEN_DAC0,
    WAVEGEN_DAC1,
    WAVEGEN_DAC2,
    WAVEGEN_DAC3,
} waveGen_channel_t;

/*!
 *    @brief  Class for  waveform generation
 */
class WaveGen {
public:
    WaveGen();
    
    // Configuration
    bool begin(uint8_t i2c_address = MCP4728_I2CADDR_DEFAULT, TwoWire *wire = &Wire);
    void end();
    
    // Waveform configuration
    void setChannel(waveGen_channel_t channel);
    void setWaveform(waveGen_waveform_t waveform);
    void setFrequency(float frequency_hz);
    void setAmplitude(float amplitude_v);
    void setOffset(float offset_v);
    // Query current configuration
    waveGen_channel_t getChannel() const { return _channel; }
    waveGen_waveform_t getWaveform() const { return _waveform; }
    float getFrequency() const { return _frequency_hz; }
    float getAmplitude() const { return _amplitude_v; }
    float getOffset() const { return _offset_v; }
    
    // Control
    bool start();
    void stop();
    bool isRunning() const { return _running; }
    
    // Service function - call frequently from main loop
    void service();
    
    // Statistics
    uint32_t getSuccessfulWrites() const { return _successful_writes; }
    uint32_t getFailedWrites() const { return _failed_writes; }
    float getActualFrequency() const { return _actual_frequency; }
    size_t getTableSize() const { return _table_size; }
    
    // Frequency management
    float getAchievableFrequency(float desired_freq) const;
    float setFrequencyAdjusted(float frequency_hz);
    
    // Fallback mode for stability
    void setFallbackMode(bool use_fallback) { _use_fallback = use_fallback; }
    bool isFallbackMode() const { return _use_fallback; }
    
    // Buffer mode information
    size_t getBufferSize() const { return _buffer_size; }
    size_t getBufferCycles() const { return _buffer_cycles; }
    
    // Calibration (if needed)
    void setCalibrationOffset(waveGen_channel_t channel, float offset_v);
    void setCalibrationGain(waveGen_channel_t channel, float gain);

private:
    // Configuration
    MCP4728 _dac;
    volatile waveGen_channel_t _channel;
    volatile waveGen_waveform_t _waveform;
    volatile float _frequency_hz;
    volatile float _amplitude_v;
    volatile float _offset_v;
    
    // State
    volatile bool _running;
    volatile bool _initialized;
    bool _use_fallback;
    
    // Timing
    uint32_t _last_sample_time;
    uint32_t _sample_interval_us;
    uint32_t _sample_count;
    
    // Waveform data
    static const size_t MAX_WAVEFORM_TABLE_SIZE = 8192;
    volatile uint16_t _waveform_table[MAX_WAVEFORM_TABLE_SIZE];
    volatile size_t _table_size;        // Current table size (dynamic)
    volatile size_t _table_index;
    // Sample repeat control for very low frequencies
    volatile uint32_t _repeat_factor;     // number of times to resend each sample
    volatile uint32_t _repeat_remaining;  // countdown for repeats before advancing index
    
    // Buffer management for efficient I2C transfers
    static const size_t MAX_BUFFER_SIZE = 8192; // 4KB buffer
    alignas(4) uint8_t _i2c_buffer[MAX_BUFFER_SIZE];
    size_t _buffer_size; // Actual buffer size for current frequency
    size_t _buffer_cycles; // Number of waveform cycles in buffer (legacy; used for stats only)
    size_t _buffer_samples; // Number of samples packed in current buffer
    bool _buffer_ready;
    bool _transfer_in_progress;
    volatile bool _completion_pending;
    volatile bool _params_changed;
    
    // Statistics
    uint32_t _successful_writes;
    uint32_t _failed_writes;
    uint32_t _last_stats_time;
    float _actual_frequency;
    // Live stats window for accurate frequency reporting during continuous streaming
    volatile uint32_t _stats_window_start_us;
    volatile uint32_t _samples_since_stats;
    volatile uint32_t _indices_since_stats;
    // Measured transfer stats (sync mode)
    size_t _last_samples_sent;
    uint32_t _last_transfer_us;
    
    // Calibration
    float _calibration_offset[4];
    float _calibration_gain[4];
    
    // Internal functions
    // Effective samples-per-second management
    float _effective_samples_per_sec;
    bool _sps_calibrated;
    float _estimateSamplesPerSec() const;


    void _buildWaveformTable();
    uint16_t _voltsToCode(float voltage, waveGen_channel_t channel);
    void _updateTableSize();
    void _updateSampleInterval();
    void _updateStatistics();
    void _sendNextSample();
    
    // Buffer management functions
    void _calculateOptimalBuffer();
    void _buildI2CBuffer();
    void _startBufferTransfer();
    void _onTransferComplete();
    
    // Static callback for  completion
    static void _staticTransferComplete();
};

#endif
