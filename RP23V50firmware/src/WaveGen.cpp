//
// NOTE: This file is currently operating in synchronous fallback mode only.
// Any previous experimental helper(s) should be removed to ensure a clean build.
/*!
 *  @file waveGen.cpp
 *
 *   Waveform Generator using MCP4728
 */

#include "WaveGen.h"
#include <math.h>

// Global pointer for static callback
static WaveGen* g_current_wavegen = nullptr;

/*!
 *    @brief  Constructor
 */
WaveGen::WaveGen() : 
    _channel(WAVEGEN_DAC0),
    _waveform(WAVEGEN_SINE),
    _frequency_hz(1000.0f),
    _amplitude_v(1.0f),
    _offset_v(0.0f),
    _running(false),
    _initialized(false),
    _use_fallback(true), // Start in fallback mode for stability
    _last_sample_time(0),
    _sample_interval_us(0),
    _sample_count(0),
    _table_size(64),  // Start with small table
    _table_index(0),
    _successful_writes(0),
    _failed_writes(0),
    _last_stats_time(0),
    _actual_frequency(0.0f),
    _buffer_size(0),
    _buffer_cycles(0),
    _buffer_ready(false),
    _transfer_in_progress(false),
    _completion_pending(false),
    _params_changed(false),
    _last_samples_sent(0),
    _last_transfer_us(0) {
    _repeat_factor = 1;
    _repeat_remaining = 1;
    
    // Initialize calibration arrays
    for (int i = 0; i < 4; i++) {
        _calibration_offset[i] = 0.0f;
        _calibration_gain[i] = 1.0f;
    }
    // Effective samples-per-second tracking
    _effective_samples_per_sec = 0.0f;
    _sps_calibrated = false;
}

/*!
 *    @brief  Initialize the waveform generator
 */
bool WaveGen::begin(uint8_t i2c_address, TwoWire *wire) {
    if (!_dac.begin(i2c_address, wire)) {
        return false;
    }
    
    _initialized = true;
    _buildWaveformTable();
    _updateSampleInterval();

    return true;
}

/*!
 *    @brief  End the waveform generator
 */
void WaveGen::end() {
    stop();
    _dac.end();
    _initialized = false;
}

/*!
 *    @brief  Set the active channel
 */
void WaveGen::setChannel(waveGen_channel_t channel) {
    _channel = channel;
    _params_changed = true;
    if (_initialized) {
        _buildWaveformTable();
    }
}

/*!
 *    @brief  Set the waveform type
 */
void WaveGen::setWaveform(waveGen_waveform_t waveform) {
    _waveform = waveform;
    _params_changed = true;
    if (_initialized) {
        _buildWaveformTable();
    }
}

/*!
 *    @brief  Set the frequency
 */
void WaveGen::setFrequency(float frequency_hz) {
    _frequency_hz = frequency_hz;
    _params_changed = true;
    if (_initialized) {
        // Re-calibrate SPS if not calibrated yet

        _updateTableSize();
        _buildWaveformTable();
        _updateSampleInterval();
        
        // Recalculate buffer for new frequency
        _calculateOptimalBuffer();
        _buildI2CBuffer();
    }
}

/*!
 *    @brief  Set the amplitude
 */
void WaveGen::setAmplitude(float amplitude_v) {
    _amplitude_v = amplitude_v;
    _params_changed = true;
    if (_initialized) {
        _buildWaveformTable();
    }
}

/*!
 *    @brief  Set the offset
 */
void WaveGen::setOffset(float offset_v) {
    _offset_v = offset_v;
    _params_changed = true;
    if (_initialized) {
        _buildWaveformTable();
    }
}

/*!
 *    @brief  Start waveform generation
 */
bool WaveGen::start() {
    if (!_initialized) {
        return false;
    }

    _running = true;
    _last_sample_time = micros();
    _sample_count = 0;
    _table_index = 0;
    // Use synchronous block streaming (no  Wire)
    _use_fallback = false;
    // Ensure SPS is calibrated right before streaming

    _calculateOptimalBuffer();
    _buildI2CBuffer();
    _transfer_in_progress = false;
    _completion_pending = false;
    _params_changed = false;
    _stats_window_start_us = micros();
    _samples_since_stats = 0;
    _indices_since_stats = 0;
    // Initialize repeat logic based on computed table size and target frequency
    _repeat_factor = 1;
    _repeat_remaining = _repeat_factor;

    
    return true;
}

/*!
 *    @brief  Stop waveform generation
 */
void WaveGen::stop() {
    _running = false;
    // no  operations to abort in sync mode
}

/*!
 *    @brief  Service function - call frequently from main loop
 */
void WaveGen::service() {
    if (!_running || !_initialized) {
        // Serial.println("WaveGen: not running or initialized");
        // Serial.flush();
        
        return;
    }

    // BLOCKING streaming loop for core2: keep the I2C transaction open and
    // write samples continuously. Only exit when stopped or params change.

    // If params changed, rebuild tables and restart streaming
    if (_params_changed) {
        _updateTableSize();
        _buildWaveformTable();
        _updateSampleInterval();
        _calculateOptimalBuffer();
        _buildI2CBuffer();
        _params_changed = false;
    }

    uint32_t t0_overall = micros();
    uint32_t samples_sent_this_call = 0;

    // Per-sample repeated-START streaming to keep bus flow continuous and avoid
    // long buffered drains. Each sample is its own short transaction.

    while (_running && !_params_changed) {
        // Synchronous per-sample write with STOP; simpler and accurate timing
        bool ok = _dac.setChannelValue((MCP4728_channel_t)_channel,
                                       _waveform_table[_table_index]);
        if (!ok) {
            _failed_writes++;
            continue;
        }
        _successful_writes++;

        // Advance to next table entry honoring repeat factor for very low frequencies
        if (_repeat_remaining > 1) {
            _repeat_remaining--;
        } else {
            _repeat_remaining = _repeat_factor;
            _table_index = (_table_index + 1) % _table_size;
            _indices_since_stats++;
        }
        samples_sent_this_call++;
        _samples_since_stats++;
        // Update rolling actual frequency every ~10ms for stability
        uint32_t t_now = micros();
        uint32_t dt = t_now - _stats_window_start_us;
        if (dt >= 10000 && _table_size > 0) { // 10 ms window
            // Effective samples-per-sec at table index rate (one index advance may have multiple repeats)
            float indices_per_sec = ((float)_indices_since_stats) * (1000000.0f / (float)dt);
            _actual_frequency = indices_per_sec / (float)_table_size;
            _stats_window_start_us = t_now;
            _samples_since_stats = 0;
            _indices_since_stats = 0;
        }
        // Optional tiny escape hatch (disabled)
    }

    // No explicit STOP needed; each synchronous write ends with STOP

    // Stats
    uint32_t t1_overall = micros();
    _last_transfer_us = (t1_overall - t0_overall);
    _last_samples_sent = samples_sent_this_call;

    // Periodic frequency estimate
    uint32_t now = t1_overall;
    if (now - _last_stats_time >= 1000000) {
        _updateStatistics();
        _last_stats_time = now;
    }
}

/*!
 *    @brief  Set calibration offset for a channel
 */
void WaveGen::setCalibrationOffset(waveGen_channel_t channel, float offset_v) {
    if ((int)channel < 4) {
        _calibration_offset[channel] = offset_v;
    }
}

/*!
 *    @brief  Set calibration gain for a channel
 */
void WaveGen::setCalibrationGain(waveGen_channel_t channel, float gain) {
    if ((int)channel < 4) {
        _calibration_gain[channel] = gain;
    }
}

/*!
 *    @brief  Get the actual achievable frequency for a desired frequency
 */
float WaveGen::getAchievableFrequency(float desired_freq) const {
    // Use synchronous-per-sample model: 3 bytes per write
    TwoWire *wire = _dac.getWire();
    float i2c_hz = 1000000.0f;
    // if (wire) {
    //     i2c_hz = (float)_dac.getClockHz();
    // }
    const float BYTES_PER_SAMPLE = 4.5f;
    float effective_samples_per_sec = (i2c_hz / 9.0f) / BYTES_PER_SAMPLE;
    if (effective_samples_per_sec < 1.0f) effective_samples_per_sec = 1.0f;

    // Calculate the table size needed for the desired frequency
    float calculated_size = effective_samples_per_sec / desired_freq;
    size_t table_size = (size_t)(calculated_size + 0.5f);
    if (table_size < 2) {
        table_size = 2;
    } else if (table_size > MAX_WAVEFORM_TABLE_SIZE) {
        table_size = MAX_WAVEFORM_TABLE_SIZE;
    }

    // Base achievable frequency without repeats
    float achievable_no_repeat = effective_samples_per_sec / (float)table_size;

    // If desired is lower, simulate repeat factor to predict achievable
    if (desired_freq < achievable_no_repeat) {
        float repeat = achievable_no_repeat / desired_freq;
        if (repeat < 1.0f) repeat = 1.0f;
        float predicted = achievable_no_repeat / (float)((uint32_t)(repeat + 0.5f));
        return predicted;
    }

    return achievable_no_repeat;
}

/*!
 *    @brief  Set frequency and return the actual achievable frequency
 */
float WaveGen::setFrequencyAdjusted(float frequency_hz) {
    // Set desired frequency
    setFrequency(frequency_hz);

    // Predict actual based on current configuration (_table_size, _repeat_factor)
    TwoWire *wire = _dac.getWire();
    float i2c_hz = 1000000.0f;
    // if (wire) {
    //     i2c_hz = (float)_dac.getClockHz();
    // }
    const float BYTES_PER_SAMPLE = 4.5f;
    float effective_samples_per_sec = (i2c_hz / 9.0f) / BYTES_PER_SAMPLE;
    if (effective_samples_per_sec < 1.0f) effective_samples_per_sec = 1.0f;
    size_t ts = _table_size > 0 ? _table_size : 1;
    uint32_t rf = _repeat_factor > 0 ? _repeat_factor : 1;
    float predicted = effective_samples_per_sec / ((float)ts * (float)rf);
    return predicted;
}

/*!
 *    @brief  Update table size based on frequency for accurate frequency generation
 */
void WaveGen::_updateTableSize() {
    // Effective sample rate based on I2C clock and 3-byte per sample
    // Approximate bytes per sec = I2C_HZ / 9 bits per byte
    TwoWire *wire = _dac.getWire();
    float i2c_hz = 1000000.0f;
    // if (wire) {
    //     // If MCP4728 tracked clock, use it; otherwise keep default
    //     i2c_hz = (float)_dac.getClockHz();
    //     Serial.print("I2C Hz: ");
    //     Serial.println(i2c_hz);
    // }
    const float BYTES_PER_SAMPLE = 4.5f;
    float effective_samples_per_sec = (i2c_hz / 9.0f) / BYTES_PER_SAMPLE;
    if (effective_samples_per_sec < 1.0f) effective_samples_per_sec = 1.0f;

    // Calculate table size to achieve the desired frequency
    float calculated_size = effective_samples_per_sec / _frequency_hz;

    // Round to nearest integer for exact frequency control
    _table_size = (size_t)(calculated_size + 0.5f); // Round to nearest integer

    // Ensure we stay within reasonable bounds
    if (_table_size < 2) {
        _table_size = 2; // Minimum for reasonable waveform quality
    } else if (_table_size > MAX_WAVEFORM_TABLE_SIZE) {
        _table_size = MAX_WAVEFORM_TABLE_SIZE; // Maximum table size
    }

    // Determine repeat factor for very low frequencies to extend effective period
    // If desired frequency is below the minimum achievable for this table size,
    // repeat each sample N times to reduce effective sample rate without changing I2C speed.
    _repeat_factor = 1;
    if (_table_size > 0) {
        float achievable_freq = effective_samples_per_sec / (float)_table_size;
        if (_frequency_hz < achievable_freq) {
            float required_sps = _frequency_hz * (float)_table_size; // samples/sec needed
            if (required_sps < 1.0f) required_sps = 1.0f;
            float repeat = (effective_samples_per_sec / required_sps);
            if (repeat < 1.0f) repeat = 1.0f;
            if (repeat > 1.0f) {
                // Clamp to a reasonable maximum to avoid overflow
                if (repeat > 1000000.0f) repeat = 1000000.0f;
                _repeat_factor = (uint32_t)(repeat + 0.5f);
                if (_repeat_factor == 0) _repeat_factor = 1;
            }
        }
    }
    _repeat_remaining = _repeat_factor;

    // Debug output (clean formatting)
    Serial.print("Table size calc: size=");
    Serial.print(calculated_size, 2);
    Serial.print(" -> ");
    Serial.print(_table_size);
    Serial.print(", est_no_repeat=");
    Serial.print(effective_samples_per_sec / (float)_table_size, 2);
    Serial.print(" Hz, repeat=");
    Serial.print((uint32_t)_repeat_factor);
    Serial.print(", est_with_repeat=");
    float denom = (float)_table_size * (float)(_repeat_factor > 0 ? _repeat_factor : 1);
    Serial.print(effective_samples_per_sec / denom, 2);
    Serial.println(" Hz");
    Serial.flush();
}

/*!
 *    @brief  Build the waveform lookup table
 */
void WaveGen::_buildWaveformTable() {
    for (size_t i = 0; i < _table_size; i++) {
        float t = (float)i / (float)_table_size; // 0..1
        float value = 0.0f;
        
        switch (_waveform) {
            case WAVEGEN_SINE: {
                value = sinf(2.0f * M_PI * t);
                break;
            }
            case WAVEGEN_TRIANGLE: {
                value = 4.0f * fabsf(t - floorf(t + 0.5f)) - 1.0f; // -1..1
                break;
            }
            case WAVEGEN_SAWTOOTH: {
                value = 2.0f * (t - floorf(t + 0.5f)); // -1..1
                break;
            }
            case WAVEGEN_SQUARE: {
                value = (t < 0.5f) ? 1.0f : -1.0f;
                break;
            }
        }
        
        // Apply amplitude and offset
        float voltage = _offset_v + _amplitude_v * value;
        
        // Convert to DAC code
        _waveform_table[i] = _voltsToCode(voltage, _channel);
    }
}

/*!
 *    @brief  Convert voltage to DAC code using proper ±8V scaling and calibration
 */
uint16_t WaveGen::_voltsToCode(float voltage, waveGen_channel_t channel) {
    // Apply calibration
    voltage = voltage * _calibration_gain[channel] + _calibration_offset[channel];
    
    // Clamp to valid range (±8V)
    voltage = constrain(voltage, -8.0f, 8.0f);
    
    // Use the same calibration system as Peripherals.cpp
    // dacSpread[channel] is the voltage range, dacZero[channel] is the zero point
    extern float dacSpread[4];
    extern int dacZero[4];
    
    // Convert voltage to DAC code using calibration values
    int code = (int)(voltage * 4095.0f / dacSpread[channel]) + dacZero[channel];
    
    return constrain(code, 0, 4095);
}

/*!
 *    @brief  Update the sample interval - fixed at 100us for 10kHz rate
 */
void WaveGen::_updateSampleInterval() {
    // Fixed sample rate of 10000 Hz (100us intervals)
    // This provides much higher frequency support while maintaining stability
    _sample_interval_us = 1; // not used with per-sample streaming; maintained for compatibility
}

/*!
 *    @brief  Send the next sample to the DAC
 */
void WaveGen::_sendNextSample() {
    // Get the current sample value
    uint16_t sample_value = _waveform_table[_table_index];
    
    // Prepare all 4 channel values (only active channel gets the signal)
    uint16_t channel_values[4] = {0, 0, 0, 0};
    channel_values[_channel] = sample_value;
    
    bool success = false;
    
   // if (_use_fallback) {
        // Fallback mode: use synchronous writes for stability

       // while (Serial.available() == 0) {
        success = _dac.quickSetChannelValue((MCP4728_channel_t)_channel, sample_value, false, false);
        //}
    //} else {
        // // Async mode: check if previous operation is complete
        // if (!_dac.finishedAsync()) {
        //     _failed_writes++;
        //     return; // Skip this sample if previous one isn't done
        // }
        
        // // Send async write with error checking
        // noInterrupts(); // Disable interrupts during critical section
        // success = _dac.fastWriteAsync(channel_values[0], channel_values[1], 
        //                              channel_values[2], channel_values[3]);
        // interrupts(); // Re-enable interrupts
  //  }
    
    if (success) {
        _successful_writes++;
    } else {
        _failed_writes++;
    }
    
    // Advance table index using dynamic table size
    _table_index = (_table_index + 1) % _table_size;
}

/*!
 *    @brief  Update statistics
 */
void WaveGen::_updateStatistics() {
    // Rolling window stats are updated in the streaming loop for accuracy
    // Backstop: compute from index advances so repeats are accounted correctly
    if (_table_size > 0) {
        uint32_t dt = micros() - _stats_window_start_us;
        if (dt > 0) {
            float indices_per_sec = ((float)_indices_since_stats) * (1000000.0f / (float)dt);
            _actual_frequency = indices_per_sec / (float)_table_size;
        }
        _stats_window_start_us = micros();
        _samples_since_stats = 0;
        _indices_since_stats = 0;
    }
}

//
// Estimate effective samples-per-second from configured I2C clock
// Fallback used before runtime calibration is available
//
float WaveGen::_estimateSamplesPerSec() const {
    if (_sps_calibrated && _effective_samples_per_sec > 0.0f) {
        return _effective_samples_per_sec;
    }
    TwoWire *wire = _dac.getWire();
    float i2c_hz = 1000000.0f;
    // if (wire) {
    //     i2c_hz = (float)_dac.getClockHz();
    // }
    const float BYTES_PER_SAMPLE = 4.0f; // address + 3 data bytes per sample
    float sps = (i2c_hz / 9.0f) / BYTES_PER_SAMPLE;
    if (sps < 1.0f) sps = 1.0f;
    return sps;
}


/*!
 *    @brief  Calculate optimal buffer size for seamless looping
 */
void WaveGen::_calculateOptimalBuffer() {
    // I2C considerations:
    // - 1MHz I2C speed
    // - MCP4728 Multi-Write per-sample: 3 bytes (command + 2 data)
    // - Pack many samples in a single transaction to eliminate per-transfer overhead
    // - Target: ~10ms worth of samples to reduce CPU service load

    const size_t BYTES_PER_SAMPLE = 4; // account for pacing/overhead per sample in repeated-START path
    // Use maximum capacity with whole-wave alignment to minimize inter-burst gaps
    size_t capacity_samples = (MAX_BUFFER_SIZE) / BYTES_PER_SAMPLE;
    _buffer_samples = capacity_samples;

    // Force an integer number of waveform cycles to minimize phase jitter and avoid mid-wave cutoffs
    if (_table_size > 0) {
        if (_buffer_samples < _table_size) {
            _buffer_samples = _table_size;
        } else {
            _buffer_samples = (_buffer_samples / _table_size) * _table_size;
        }
        if (_buffer_samples > capacity_samples) {
            _buffer_samples = (capacity_samples / _table_size) * _table_size;
            if (_buffer_samples == 0) _buffer_samples = _table_size;
        }
    }

    _buffer_size = _buffer_samples * BYTES_PER_SAMPLE;
    _buffer_cycles = (_table_size > 0) ? (_buffer_samples / _table_size) : 0;
    _buffer_ready = false; // Need to rebuild buffer
}

/*!
 *    @brief  Build the I2C buffer with multiple waveform cycles
 */
void WaveGen::_buildI2CBuffer() {
    if (_buffer_size == 0 || _table_size == 0) {
        return;
    }

    size_t buffer_pos = 0;
    const uint8_t cmd = (uint8_t)(0x40 | (((uint8_t)_channel) << 1) | 0x00); // UDAC=0 immediate

    for (size_t s = 0; s < _buffer_samples; s++) {
        if (buffer_pos + 3 > MAX_BUFFER_SIZE) {
            break;
        }

        size_t idx = (_table_index + s) % _table_size;
        uint16_t value = _waveform_table[idx];
        _i2c_buffer[buffer_pos++] = cmd;
        _i2c_buffer[buffer_pos++] = (uint8_t)((value >> 8) & 0xFF);
        _i2c_buffer[buffer_pos++] = (uint8_t)(value & 0xFF);
    }

    // Update actual size in case we broke early
    _buffer_size = buffer_pos;
    _buffer_ready = (_buffer_size > 1);

    // Ensure buffer encodes whole cycles only (sanity assert-like guard)
    if (_table_size > 0) {
        size_t remainder = _buffer_samples % _table_size;
        if (remainder != 0) {
            size_t valid_samples = _buffer_samples - remainder;
            _buffer_size = valid_samples * 3;
            _buffer_samples = valid_samples;
        }
    }
}

/*!
 *    @brief  Start async transfer (simplified)
 */
void WaveGen::_startBufferTransfer() {
    if (!_running) {
        return;
    }

    // Safety check: ensure we have valid data
    if (_table_size == 0 || _table_index >= _table_size) {
        return;
    }

    if (!_buffer_ready) {
        return;
    }

    // Per-sample synchronous streaming to avoid chunk boundaries completely
    size_t total_samples_sent = 0;
    uint32_t t0 = micros();
    while (_running && total_samples_sent < _table_size) {
        uint16_t sample_value = _waveform_table[_table_index];
        bool ok = _dac.setChannelValue((MCP4728_channel_t)_channel, sample_value);
        if (!ok) {
            _failed_writes++;
            break;
        }
        _successful_writes++;
        _table_index = (_table_index + 1) % _table_size;
        total_samples_sent++;
    }
    uint32_t t1 = micros();
    _last_transfer_us = (t1 - t0);
    _last_samples_sent = total_samples_sent;
    // Build buffer for next call (not used in per-sample mode but keeps state consistent)
    _buildI2CBuffer();
}

/*!
 *    @brief  Static callback for  completion
 */
void WaveGen::_staticTransferComplete() {
    // IRQ context: set flag only, no Serial or heavy work
    if (g_current_wavegen) {
        g_current_wavegen->_completion_pending = true;
    }
}

/*!
 *    @brief  Called when buffer transfer completes
 */
void WaveGen::_onTransferComplete() {
    if (!_running) {
        return; // Safety check
    }
    
    _transfer_in_progress = false;
    
    // Advance to next buffer window
    if (_table_size > 0 && _buffer_samples > 0) {
        _table_index = (_table_index + _buffer_samples) % _table_size;
    }

    // Refill buffer for next window
    _buildI2CBuffer();

    // Immediately start next transfer for continuous streaming
    if (_running) {
        _startBufferTransfer();
    }
}
