// SPDX-License-Identifier: MIT
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Start the DAC AWG engine.
// - codes_a: pointer to array of 12-bit codes for channel A (size n_frames)
// - n_frames: number of frames in the buffer
// - codes_b: optional pointer to array for channel B (size n_frames); pass NULL to drive only A
// - sample_rate_hz: desired output frames per second (approximate; engine sets I2C baud accordingly)
// Returns 0 on success, negative on error.
int jl_dac_awg_start(const uint16_t *codes_a,
                     size_t n_frames,
                     const uint16_t *codes_b,
                     uint32_t sample_rate_hz);

// Stop the DAC AWG engine, freeing DMA channels and buffers.
void jl_dac_awg_stop(void);

// Returns non-zero if the AWG engine is currently running.
int jl_dac_awg_running(void);

// Convenience presets for common waveforms on A/B.
// - wave: 0=sine, 1=triangle, 2=sawtooth, 3=square
// - amplitude_volts: peak amplitude in volts; clipped to \u00b18V range
// - dc_offset_volts: DC offset in volts; total stays within \u00b18V
// - samples_per_period: number of discrete samples per period (>0)
// - sample_rate_hz: frames per second (frequency = sample_rate_hz / samples_per_period)
// If mirror_b is true, channel B equals channel A; otherwise B is 0V.
// channel: -1=both/mirror, 0=DAC A only, 1=DAC B only
int jl_dac_awg_start_preset(int wave,
                            float amplitude_volts,
                            float dc_offset_volts,
                            size_t samples_per_period,
                            uint32_t sample_rate_hz,
                            int mirror_b,
                            int channel);

#ifdef __cplusplus
}
#endif


