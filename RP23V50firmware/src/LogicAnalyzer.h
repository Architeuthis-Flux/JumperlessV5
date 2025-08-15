// SPDX-License-Identifier: MIT
#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <stdbool.h>

#include "ArduinoStuff.h"

#include "hardware/adc.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/pio.h"
#include "hardware/timer.h"

#ifdef USE_TINYUSB
#include "tusb.h"
#endif

// Simple, self-contained logic analyzer focused on clarity and robustness.
// Modes:
// - Normal mode: sample_rate >= 5 kHz and sample_rate * a_chan_cnt <= 200 kHz
// - Slow mode:   sample_rate < 5 kHz and num_samples > 1000 (stream as we go)
// - Decimation:  TODO (when digital rate exceeds ADC capability)

class LogicAnalyzer {
public:
	LogicAnalyzer();

	// Configuration inputs (set before arm())
	uint32_t sample_rate_hz;   // desired digital sample rate (Hz)
	uint32_t num_samples;      // total time-samples to capture
	uint32_t a_mask;           // analog channels bitmask (0..7)
	uint32_t d_mask;           // digital mask (currently informational)


	Stream* la_stream = &USBSer2;

	// Lifecycle
	bool init();
	bool arm();      // configures PIO/DMA and allocates ping-pong buffers
	void run();      // starts capture, streams halves over USB
	void stop();     // stop DMA/PIO/ADC cleanly
	void deinit();   // free buffers, reset state
	void resetState(); // reset internal flags and buffers (no deallocation)

	// Micropython compatibility helpers
	bool getIsRunning() const { return running; }
	void reset() { stop(); }

	// Command/IO
	void handler();
	bool process_char(char ch);

	// State
	bool is_initialized() const { return initialized; }
	bool is_armed() const { return armed; }
	bool is_running() const { return running; }

	unsigned long last_command_time;

private:
	// --- Internal helpers ---
	bool compute_layout();
	bool setup_pio_for_digital();
	bool setup_dma_for_digital();
	bool setup_adc_for_analog();
	bool setup_dma_for_analog();

	void start_hardware();
	void send_half(uint32_t half_index, uint32_t samples_in_half);
	void send_flush();
	bool usb_write_blocking(const uint8_t* data, int len);

	// Packs the 16-bit digital value per sample using current GPIO readings and
	// the 8-bit PIO sample (GP20..GP27). Mapping (bit15..bit0):
	// [gp0,gp1,gp4,gp5,gp6,gp7,gp18,gp19,gp20,gp21,gp22,gp23,gp24,gp25,gp26,gp27]
	uint8_t pack_digital_8(uint8_t pio_bits);

	// Transmission helpers (ASCII 7-bit framing like JulseView)
	void encode_and_queue_digital(uint8_t digi);
	void encode_and_queue_analog(uint16_t value_12b);

	// Mode selection
	bool is_normal_mode() const;
	bool is_slow_mode() const;
	static void pio_irq1_isr();
	void on_pio_irq1();

	// --- State and resources ---
	bool initialized;
	bool armed;
	bool running;

	PIO lapio;
	int lasm;                   // state machine index
	int dreq_pio_rx;          // PIO RX DREQ

	bool pio_loaded;
	int pio_slow_offset;      // cached offset of slow program in IMEM

	// DMA channels
	int dma_dig_0; // digital half 0
	int dma_dig_1; // digital half 1
	int dma_ana_0; // analog half 0
	int dma_ana_1; // analog half 1

	dma_channel_config cfg_dig_0;
	dma_channel_config cfg_dig_1;
	dma_channel_config cfg_ana_0;
	dma_channel_config cfg_ana_1;

	// Ping-pong buffers (RAII-managed via new/delete in deinit; no manual arena)
	uint8_t*  dig_half0;   // one byte per digital sample from PIO (GP20..27)
	uint8_t*  dig_half1;
	uint16_t* ana_half0;   // interleaved per channel (16-bit per sample)
	uint16_t* ana_half1;

	// Sizes
	uint32_t samples_per_half;          // time-samples per half (divides num_samples)
	uint32_t digital_bytes_per_half;    // = samples_per_half * sizeof(uint8_t)
	uint32_t analog_words_per_half;     // = samples_per_half * a_chan_cnt
	uint32_t samples_remaining;         // countdown across halves
	uint8_t current_half_idx;           // 0 or 1 for ping-pong

	// Transmit buffer
	static constexpr uint32_t TX_BUF_SIZE = 16384;
	uint8_t txbuf[TX_BUF_SIZE];
	uint16_t txidx;

	// Command parser state
	char cmdstr[20];
	uint8_t cmdidx;
	char rsp[32];

	// Slow-mode analog pacing
	uint8_t ana_enabled_count;
	uint8_t ana_enabled_list[8];
	static LogicAnalyzer* active_instance;

	// DMA write address pointers for resetting on completion (ringing halves)
	volatile uint32_t* dig_taddr0;
	volatile uint32_t* dig_taddr1;
	volatile uint32_t* ana_taddr0;
	volatile uint32_t* ana_taddr1;

	// Cached ADC round-robin mask
	uint8_t adc_rr_mask;
};


