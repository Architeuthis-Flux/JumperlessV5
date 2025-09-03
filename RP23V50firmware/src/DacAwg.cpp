// SPDX-License-Identifier: MIT
#include "DacAwg.h"

#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "hardware/dma.h"
#include "hardware/i2c.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/i2c.h"
#include "hardware/address_mapped.h"
#include "pico/stdlib.h"

#include "JumperlessDefines.h"
#include "Peripherals.h" // dacSpread/dacZero, LDAC
#include <math.h>

// Implementation strategy:
// - Use Arduino Wire (I2C0 default) pins 4 (SDA) and 5 (SCL) preconfigured by system init.
// - Build an interleaved fastWrite frame stream for MCP4728: 8 bytes per frame (A,B,C,D high/low)
//   We'll drive A/B from user buffers, and mirror C/D from current steady state readings (midpoint).
// - Configure hardware I2C target address and use DMA to feed IC_DATA_CMD with per-byte commands.
// - Use a chained control DMA to re-prime the data DMA for a circular loop.

namespace {

// MCP4728 default I2C address (7-bit). Adjust if board uses different address.
constexpr uint8_t MCP4728_ADDR = 0x60;

// We stream only data bytes; RESTART/STOP are implicit per continuous write with TAR set.
// Each fastWrite frame is 8 bytes: A_hi, A_lo, B_hi, B_lo, C_hi, C_lo, D_hi, D_lo.

// DMA resources
static int s_dma_data = -1;
static int s_dma_ctrl = -1;
static dma_channel_config s_cfg_data;
static dma_channel_config s_cfg_ctrl;

// Circular frame buffer (8 bytes per frame)
static uint8_t *s_frame_bytes = nullptr;
static size_t s_num_frames = 0;
static uint32_t *s_cmd_words = nullptr;
static size_t s_cmd_word_count = 0;

// Saved Wire settings to restore on stop
static uint32_t s_saved_i2c_freq = 400000;
static bool s_running = false;

// Compute approximate I2C bit rate needed for a given frames-per-second
// I2C bytes per frame = 8 data + 1 addr (handled by TAR), but HW I2C handles address phase.
// TX FIFO is paced per byte; include overhead for START/ACKs. We conservatively budget 10 bytes/frame.
static uint32_t compute_i2c_baud(uint32_t frames_per_sec) {
    uint32_t bytes_per_frame = 10; // conservative overhead
    uint64_t bps = static_cast<uint64_t>(frames_per_sec) * bytes_per_frame * 9; // 8 bits + ACK approx
    if (bps < 100000) return 100000;
    if (bps > 1700000) return 1700000; // stay under ~1.7MHz unless HS mode handshake added
    return static_cast<uint32_t>(bps);
}

// Build the 8-byte frame for index i using provided A/B codes and fixed C/D midpoint
static inline void build_frame(uint8_t *dst8,
                               uint16_t code_a,
                               uint16_t code_b,
                               uint16_t code_c,
                               uint16_t code_d) {
    dst8[0] = static_cast<uint8_t>(code_a >> 8);
    dst8[1] = static_cast<uint8_t>(code_a & 0xFF);
    dst8[2] = static_cast<uint8_t>(code_b >> 8);
    dst8[3] = static_cast<uint8_t>(code_b & 0xFF);
    dst8[4] = static_cast<uint8_t>(code_c >> 8);
    dst8[5] = static_cast<uint8_t>(code_c & 0xFF);
    dst8[6] = static_cast<uint8_t>(code_d >> 8);
    dst8[7] = static_cast<uint8_t>(code_d & 0xFF);
}

} // namespace

int jl_dac_awg_start(const uint16_t *codes_a,
                     size_t n_frames,
                     const uint16_t *codes_b,
                     uint32_t sample_rate_hz) {
    if (s_running) {
        return -2; // already running
    }
    if (!codes_a || n_frames == 0) {
        return -1;
    }

    // Calculate I2C baud to achieve desired frame rate
    uint32_t baud = compute_i2c_baud(sample_rate_hz);
    i2c_set_baudrate(i2c0, baud);

    // Program I2C target address directly (ensure enabled)
    volatile io_rw_32 *i2c0_enable = (volatile io_rw_32 *)(I2C0_BASE + I2C_IC_ENABLE_OFFSET);
    volatile io_rw_32 *i2c0_con    = (volatile io_rw_32 *)(I2C0_BASE + I2C_IC_CON_OFFSET);
    volatile io_rw_32 *i2c0_tar    = (volatile io_rw_32 *)(I2C0_BASE + I2C_IC_TAR_OFFSET);
    volatile io_rw_32 *i2c0_datacmd= (volatile io_rw_32 *)(I2C0_BASE + I2C_IC_DATA_CMD_OFFSET);

    hw_clear_bits(i2c0_enable, I2C_IC_ENABLE_ENABLE_BITS);
    *i2c0_tar = MCP4728_ADDR & 0x3FF;
    *i2c0_con = (I2C_IC_CON_MASTER_MODE_BITS |
                 I2C_IC_CON_IC_RESTART_EN_BITS |
                 (I2C_IC_CON_SPEED_VALUE_FAST << I2C_IC_CON_SPEED_LSB));
    hw_set_bits(i2c0_enable, I2C_IC_ENABLE_ENABLE_BITS);

    // Allocate frame buffer (8 bytes per frame)
    size_t total_bytes = n_frames * 8;
    s_frame_bytes = static_cast<uint8_t *>(malloc(total_bytes));
    if (!s_frame_bytes) {
        // Restore a reasonable default I2C clock
        i2c_set_baudrate(i2c0, 1000000);
        return -3;
    }
    s_num_frames = n_frames;

    // Use mid-scale for C/D rails to avoid changing rails while streaming
    uint16_t code_c = 1650; // board-specific midpoint already used elsewhere
    uint16_t code_d = 1650;

    for (size_t i = 0; i < n_frames; ++i) {
        uint16_t ca = codes_a[i] & 0x0FFF;
        uint16_t cb = codes_b ? (codes_b[i] & 0x0FFF) : ca; // mirror A if B not provided
        build_frame(&s_frame_bytes[i * 8], ca, cb, code_c, code_d);
    }

    // Ensure LDAC is low so outputs update immediately per channel write (Fast Write mode)
    pinMode(LDAC, OUTPUT);
    digitalWrite(LDAC, LOW);

    // Reserve DMA channels
    s_dma_data = dma_claim_unused_channel(true);
    s_dma_ctrl = dma_claim_unused_channel(true);

    // Data DMA: copy command words -> I2C data_cmd register (32-bit transfers)
    s_cfg_data = dma_channel_get_default_config(s_dma_data);
    channel_config_set_transfer_data_size(&s_cfg_data, DMA_SIZE_32);
    channel_config_set_read_increment(&s_cfg_data, true);
    channel_config_set_write_increment(&s_cfg_data, false);
    channel_config_set_dreq(&s_cfg_data, DREQ_I2C0_TX);
    // Chain to control DMA when data completes one full frames buffer
    channel_config_set_chain_to(&s_cfg_data, s_dma_ctrl);

    // Prebuild 32-bit DATA_CMD words with proper RESTART/STOP flags
    const uint32_t RESTART = (1u << I2C_IC_DATA_CMD_RESTART_LSB);
    const uint32_t STOP    = (1u << I2C_IC_DATA_CMD_STOP_LSB);
    const uint32_t WRITE   = 0; // CMD=0 for write

    s_cmd_word_count = n_frames * 8;
    s_cmd_words = (uint32_t *)malloc(sizeof(uint32_t) * s_cmd_word_count);
    if (!s_cmd_words) {
        free(s_frame_bytes);
        s_frame_bytes = nullptr;
        return -4;
    }
    // Convert each data byte into a DATA_CMD word, inserting RESTART on first and STOP on last per frame
    for (size_t f = 0; f < n_frames; ++f) {
        for (int b = 0; b < 8; ++b) {
            uint32_t v = s_frame_bytes[f * 8 + b];
            if (b == 0) v |= RESTART;
            if (b == 7) v |= STOP;
            s_cmd_words[f * 8 + b] = v;
        }
    }

    // Configure but don't start
    dma_channel_configure(
        s_dma_data,
        &s_cfg_data,
        (void *)i2c0_datacmd,          // write address
        s_cmd_words,                   // read address
        s_cmd_word_count,              // number of 32-bit words
        false);

    // Control DMA: re-prime data DMA READ_ADDR/WRITE_ADDR/TRANS_COUNT_TRIG to loop and chain back to data
    static volatile uint32_t s_ctrl_block[4];
    // CTRL_TRIG: data size 32-bit, inc read, no inc write, dreq I2C0_TX, chain back to ctrl
    {
        dma_channel_config tmp = dma_channel_get_default_config(s_dma_data);
        channel_config_set_transfer_data_size(&tmp, DMA_SIZE_32);
        channel_config_set_read_increment(&tmp, true);
        channel_config_set_write_increment(&tmp, false);
        channel_config_set_dreq(&tmp, DREQ_I2C0_TX);
        channel_config_set_chain_to(&tmp, s_dma_ctrl);
        s_ctrl_block[0] = tmp.ctrl;                      // CHx_AL1_CTRL
    }
    s_ctrl_block[1] = (uint32_t) s_cmd_words;           // CHx_AL1_READ_ADDR
    s_ctrl_block[2] = (uint32_t) i2c0_datacmd;          // CHx_AL1_WRITE_ADDR
    s_ctrl_block[3] = (uint32_t) s_cmd_word_count;      // CHx_AL1_TRANS_COUNT_TRIG (also starts channel)

    s_cfg_ctrl = dma_channel_get_default_config(s_dma_ctrl);
    channel_config_set_transfer_data_size(&s_cfg_ctrl, DMA_SIZE_32);
    channel_config_set_read_increment(&s_cfg_ctrl, true);
    channel_config_set_write_increment(&s_cfg_ctrl, true);
    // Pace by a benign DREQ so we don't stall; we'll trigger manually
    channel_config_set_dreq(&s_cfg_ctrl, DREQ_FORCE);
    channel_config_set_chain_to(&s_cfg_ctrl, s_dma_data);

    dma_channel_configure(
        s_dma_ctrl,
        &s_cfg_ctrl,
        (void *) &dma_hw->ch[s_dma_data].al1_ctrl,      // start at CTRL alias
        (const void *) s_ctrl_block,
        4,                                             // CTRL, READ_ADDR, WRITE_ADDR, TRANS_COUNT_TRIG
        false);

    // Kick the first data DMA run, it will chain to control and then repeat
    dma_channel_start(s_dma_data);
    s_running = true;
    return 0;
}

void jl_dac_awg_stop(void) {
    if (!s_running) return;

    // Stop DMA channels
    if (s_dma_data >= 0) {
        dma_channel_abort(s_dma_data);
        dma_channel_unclaim(s_dma_data);
        s_dma_data = -1;
    }
    if (s_dma_ctrl >= 0) {
        dma_channel_abort(s_dma_ctrl);
        dma_channel_unclaim(s_dma_ctrl);
        s_dma_ctrl = -1;
    }

    // Free buffers
    if (s_frame_bytes) {
        free(s_frame_bytes);
        s_frame_bytes = nullptr;
    }
    if (s_cmd_words) {
        free(s_cmd_words);
        s_cmd_words = nullptr;
        s_cmd_word_count = 0;
    }
    s_num_frames = 0;

    // Restore I2C clock
    Wire.setClock(s_saved_i2c_freq);
    s_running = false;
}

int jl_dac_awg_running(void) {
    return s_running ? 1 : 0;
}

static inline uint16_t volts_to_code(float v, int channel_index) {
    // Clamp to +/-8V
    if (v > 8.0f) v = 8.0f;
    if (v < -8.0f) v = -8.0f;
    // Use board calibration: code = (voltage * 4095 / dacSpread[n]) + dacZero[n]
    float spread = dacSpread[ channel_index & 3 ];
    int zero = dacZero[ channel_index & 3 ];
    if (spread < 1.0f) spread = 20.1f;
    float code = (v * 4095.0f / spread) + (float)zero;
    if (code < 0) code = 0;
    if (code > 4095) code = 4095;
    return (uint16_t)code;
}

int jl_dac_awg_start_preset(int wave,
                            float amplitude_volts,
                            float dc_offset_volts,
                            size_t samples_per_period,
                            uint32_t sample_rate_hz,
                            int mirror_b,
                            int channel) {
    if (samples_per_period == 0) return -1;
    // Allocate buffers for A (and optionally B)
    uint16_t *buf_a = (uint16_t *)malloc(samples_per_period * sizeof(uint16_t));
    if (!buf_a) return -2;
    uint16_t *buf_b = (mirror_b || channel == 1) ? (uint16_t *)malloc(samples_per_period * sizeof(uint16_t)) : nullptr;
    if ((mirror_b || channel == 1) && !buf_b) {
        free(buf_a);
        return -3;
    }

    for (size_t i = 0; i < samples_per_period; ++i) {
        float t = (float)i / (float)samples_per_period; // 0..1
        float s = 0.0f;
        switch (wave) {
            case 0: // sine
                s = sinf(2.0f * (float)M_PI * t);
                break;
            case 1: // triangle (-1..1)
                s = 4.0f * fabsf(t - floorf(t + 0.5f)) - 1.0f;
                break;
            case 2: // sawtooth (-1..1)
                s = 2.0f * (t - floorf(t + 0.5f));
                break;
            case 3: // square (-1..1)
                s = (t < 0.5f) ? 1.0f : -1.0f;
                break;
            default:
                s = 0.0f;
                break;
        }
        float v = dc_offset_volts + amplitude_volts * s; // volts
        uint16_t code = volts_to_code(v, 0) & 0x0FFF; // channel A calib by default
        buf_a[i] = code;
        if (buf_b) buf_b[i] = volts_to_code(v, 1) & 0x0FFF; // channel B calib
    }

    // Route based on requested channel
    int rc = 0;
    if (channel == 0) {
        rc = jl_dac_awg_start(buf_a, samples_per_period, NULL, sample_rate_hz);
    } else if (channel == 1) {
        rc = jl_dac_awg_start(buf_b ? buf_b : buf_a, samples_per_period, NULL, sample_rate_hz);
    } else {
        rc = jl_dac_awg_start(buf_a, samples_per_period, buf_b, sample_rate_hz);
    }
    // The engine copies/uses provided buffers directly; keep them allocated for now.
    // If we want to free here, we must copy inside jl_dac_awg_start. We didn't, so leave allocated.
    return rc;
}


