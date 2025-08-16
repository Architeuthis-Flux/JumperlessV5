// SPDX-License-Identifier: MIT

#include "LogicAnalyzer.h"
#include "JulseView.h" // for debug macros and constants (reuse debug infra)
#include <new>
#include "Peripherals.h"
#include "hardware/address_mapped.h"
#include "hardware/gpio.h"
#include "hardware/watchdog.h"
#include "configManager.h"
#include "Peripherals.h"

// // ADC register definitions (matching JulseView)
// #define ADC_BASE 0x40040000



//TODO: Decimation mode

// Unified PIO program (first instruction identical). We'll vary wrap based on mode.
// Read 8 pins, assert IRQ1, then delay ~256 PIO cycles (8*32) to extend period without extreme clkdiv.
static const uint16_t pio_program_slow[] = {
	(uint16_t)pio_encode_in(pio_pins, 8),
	(uint16_t)pio_encode_irq_set(false, 1),
	(uint16_t)pio_encode_delay(30),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
	(uint16_t)pio_encode_delay(31),
};
static const pio_program prog_slow = { pio_program_slow, 10, -1 };

// // Minimal fast program (single instruction)
// static const uint16_t pio_program_fast[] = {
// 	(uint16_t)pio_encode_in(pio_pins, 8),
// };
// static const pio_program prog_fast = { pio_program_fast, 1, -1 };

LogicAnalyzer::LogicAnalyzer()
	: sample_rate_hz(10000),
	  num_samples(5000),
	  a_mask(0x0F),
	  d_mask(0xFF),
	  initialized(false),
	  armed(false),
	  running(false),
	  lapio(pio0),
	  lasm(-1),
    dreq_pio_rx(-1),
	  dma_dig_0(-1), dma_dig_1(-1), dma_ana_0(-1), dma_ana_1(-1),
	  dig_half0(nullptr), dig_half1(nullptr), ana_half0(nullptr), ana_half1(nullptr),
	  samples_per_half(0), digital_bytes_per_half(0), analog_words_per_half(0),
    txidx(0), cmdidx(0) {
}

LogicAnalyzer* LogicAnalyzer::active_instance = nullptr;

bool LogicAnalyzer::init() {
	initialized = true;
	JULSEDEBUG_STA("LA init()\n\r");

	// Deterministic PIO selection: always use PIO0
	lapio = pio0;
	if (lasm < 0) lasm = pio_claim_unused_sm(lapio, true);
	if (lasm < 0) {
		JULSEDEBUG_ERR("LA: Failed to claim PIO0 SM at init\n\r");
		return false;
	}

	// Load the slow program once by scanning for a free offset on PIO0
	if (!pio_loaded) {
        int offset = pio_add_program(lapio, &prog_slow);
        pio_slow_offset = offset;
        pio_loaded = true;
        JULSEDEBUG_DIG("LA PIO init: loaded slow program at PIO0 offset=%d, SM%d\n\r", pio_slow_offset, lasm);
    }
	return true;
}

bool LogicAnalyzer::is_normal_mode() const {
	uint32_t a_cnt = 0;
	for (int i = 0; i < 8; i++) if ((a_mask >> i) & 1) a_cnt++;
	if (sample_rate_hz < 5000) return false;
	return (a_cnt == 0) || (sample_rate_hz * a_cnt <= 200000);
}

bool LogicAnalyzer::is_slow_mode() const {
	return sample_rate_hz < 5000;
}

void LogicAnalyzer::handler() {
#ifdef USE_TINYUSB
	if (!USBSer2 || !USBSer2.available()) return;
	char ch = USBSer2.read();
    last_command_time = millis();
	if (process_char(ch)) {
		usb_write_blocking((const uint8_t*)rsp, strlen(rsp));
	}
#endif
}

unsigned long last_command_time = 0;

bool LogicAnalyzer::process_char(char ch) {
	if (cmdidx >= sizeof(cmdstr) - 1) cmdidx = 0;
	if (ch == '*') { cmdidx = 0; return false; }
	if (ch == '+') { stop(); return false; }
	if (ch == '\r' || ch == '\n') {
		cmdstr[cmdidx] = 0; cmdidx = 0;
		// Minimal subset for driver bring-up
		switch (cmdstr[0]) {
		case 'i': {
			if (!initialized) init();
			// Respond with device ID + 7-bit framing meta (mirrors JulseView style)
			// A08=8 analog channels supported, D08=8 digital channels
			snprintf(rsp, sizeof(rsp), "SRJLV5,A%02d2D%02d,02", 8, 8);
			JULSEDEBUG_CMD("LA CMD i -> %s\n\r", rsp);
			return true;
		}
		case 'a': {
			// Analog channel query: 'a' -> "805x0"; 'aN' -> "<scale>x<offset>"
			size_t len = strlen(cmdstr);
			if (len == 1) {
				snprintf(rsp, sizeof(rsp), "805x0");
				JULSEDEBUG_CMD("LA CMD a -> %s\n\r", rsp);
				return true;
			} else {
				int chn = atoi(&cmdstr[1]);
				uint32_t scale_uv = 0; int32_t offset_uv = 0;
				// Mimic JulseView calibration mapping
				const auto &cal = jumperlessConfig.calibration;
				switch (chn) {
					case 0: scale_uv = (uint32_t)((cal.adc_0_spread * 1000000.0) / 4096.0); offset_uv = -(int32_t)((cal.adc_0_spread / 2) * 1000000.0); break;
					case 1: scale_uv = (uint32_t)((cal.adc_1_spread * 1000000.0) / 4096.0); offset_uv = -(int32_t)((cal.adc_1_spread / 2) * 1000000.0); break;
					case 2: scale_uv = (uint32_t)((cal.adc_2_spread * 1000000.0) / 4096.0); offset_uv = -(int32_t)((cal.adc_2_spread / 2) * 1000000.0); break;
					case 3: scale_uv = (uint32_t)((cal.adc_3_spread * 1000000.0) / 4096.0); offset_uv = -(int32_t)((cal.adc_3_spread / 2) * 1000000.0); break;
					case 4: scale_uv = (uint32_t)((cal.adc_4_spread * 1000000.0) / 4096.0); offset_uv = 0; break;
					case 5: scale_uv = (uint32_t)((3.3 * 1000000.0) / 4096.0); offset_uv = 0; break;
					case 6: scale_uv = (uint32_t)((3.3 * 1000000.0) / 4096.0); offset_uv = 0; break;
					case 7: scale_uv = (uint32_t)((cal.adc_7_spread * 1000000.0) / 4096.0); offset_uv = -(int32_t)((cal.adc_7_spread / 2) * 1000000.0); break;

					default: scale_uv = (uint32_t)((18.28 * 1000000.0) / 4096.0); offset_uv = -(int32_t)(8.0 * 1000000.0); break;
				}
				snprintf(rsp, sizeof(rsp), "%ux%d", scale_uv, offset_uv);
				JULSEDEBUG_CMD("LA CMD a%d -> %s\n\r", chn, rsp);
				return true;
			}
		}
		case 'R': {
			int v = atoi(&cmdstr[1]);
			if (v > 0) sample_rate_hz = (uint32_t)v;
			// Always return decimation factor 1 for now
			snprintf(rsp, sizeof(rsp), "*1");
			JULSEDEBUG_CMD("LA CMD R -> rate=%u, resp=%s\n\r", sample_rate_hz, rsp);
			return true;
		}
		case 'L': {
			int v = atoi(&cmdstr[1]);
			if (v > 0) num_samples = (uint32_t)v;
			snprintf(rsp, sizeof(rsp), "*");
			JULSEDEBUG_CMD("LA CMD L -> samples=%u\n\r", num_samples);
			return true;
		}
		case 'A': {
			// Axy: x=0/1 enable, y=channel 0..7
			uint8_t a_mask_old = a_mask;
			if (strlen(cmdstr) >= 3) {
				int en = cmdstr[1] - '0';
				int chn = atoi(&cmdstr[2]);
				if (chn >= 0 && chn < 8) {
					if (en) a_mask |= (1u << chn); else a_mask &= ~(1u << chn);
				}
			}
			snprintf(rsp, sizeof(rsp), "*");
			if (a_mask != a_mask_old) {
				JULSEDEBUG_CMD("LA CMD A -> a_mask=0x%02X\n\r", (unsigned)(a_mask & 0xFF));
			}
			return true;
		}
		case 'D': {
			// we always capture 8 GPIO (20..27); accept and acknowledge
			uint8_t d_mask_old = d_mask;
			snprintf(rsp, sizeof(rsp), "*");
			if (d_mask != d_mask_old) {
				JULSEDEBUG_CMD("LA CMD D -> d_mask=0x%02X\n\r", (unsigned)(d_mask & 0xFF));
			}
			return true;
		}
		case 'E': {
			// Control channel enable count (0..16), acknowledge
			snprintf(rsp, sizeof(rsp), "*");
			JULSEDEBUG_CMD("LA CMD E\n\r");
			return true;
		}
		case 'C': {
			// 'C' alone -> run; 'Cxy' -> control channel toggle (ack only)
			if (strlen(cmdstr) == 1) {
				JULSEDEBUG_CMD("LA CMD C (start)\n\r");
				if (armed || running) {
					stop();
					resetState();
				}
				if (!arm()) {
					JULSEDEBUG_ERR("LA: arm() failed for C command\n\r");
					stop();
					snprintf(rsp, sizeof(rsp), "!");
					return true;
				}
				run();
				rsp[0] = 0; return false;
			} else {
				snprintf(rsp, sizeof(rsp), "*");
				JULSEDEBUG_CMD("LA CMD C (control)\n\r");
				return true;
			}
		}
		case 'F': {
			JULSEDEBUG_CMD("LA CMD F (start)\n\r");
			// If we are already armed/running from a previous session, stop and reset
			if (armed || running) {
				stop();
				resetState();
			}
			if (!arm()) {
				JULSEDEBUG_ERR("LA: arm() failed for F command\n\r");
				stop();
				watchdog_disable();
				snprintf(rsp, sizeof(rsp), "!");
				return true;
			}
			watchdog_enable(4000, true);
			run();
			watchdog_disable();
			rsp[0] = 0; // no reply while running
			return false;
		}
		case 'S': {
			// Status: Armed (A), Started (S), Sending (R), Trigger enabled (T)
			// Keep simple mappings: A,S,R all 0/1
			int A = armed ? 1 : 0;
			int S = running ? 1 : 0;
			int R = S; // treat running as sending
			int T = 0;
			snprintf(rsp, sizeof(rsp), "A%dS%dR%dT%d", A, S, R, T);
			JULSEDEBUG_CMD("LA CMD S -> %s\n\r", rsp);
			return true;
		}
		default:
			rsp[0] = 0;
			return false;
		}
		last_command_time = millis();
	} else {
		cmdstr[cmdidx++] = ch;
		return false;
	}
}

bool LogicAnalyzer::compute_layout() {
    // Decide a chunked samples_per_half (ping-pong) small enough to fit memory and aligned
    if (num_samples == 0) num_samples = 10000;
    // Base chunk size (time-samples per half)
    uint32_t half = 2048; // default chunk
    uint32_t a_cnt = 0;
    for (int i = 0; i < 8; i++) if ((a_mask >> i) & 1) a_cnt++;
    uint32_t new_samples_per_half;
    if (a_cnt > 0) { uint32_t aligned = (half / a_cnt) * a_cnt; if (aligned == 0) aligned = a_cnt; new_samples_per_half = aligned; }
    else { new_samples_per_half = half; }
    
    // Proposed sizes for this layout
    uint32_t new_digital_bytes_per_half = new_samples_per_half;  // 1 byte per sample
    uint32_t new_analog_words_per_half = new_samples_per_half * a_cnt; // a_cnt words per sample
    
    // Check free heap and adjust chunk size downward if needed
    uint32_t need_bytes = new_digital_bytes_per_half * 2;
    if (a_cnt > 0) need_bytes += new_analog_words_per_half * 2 * sizeof(uint16_t);
    uint32_t free_heap = rp2040.getFreeHeap();
    JULSEDEBUG_BUF("LA compute_layout: need=%u free=%u half=%u a_cnt=%u\n\r", need_bytes, free_heap, new_samples_per_half, a_cnt);
    while (free_heap < need_bytes + 16384 && new_samples_per_half >= (a_cnt > 0 ? a_cnt * 2 : 2)) {
        new_samples_per_half /= 2;
        new_digital_bytes_per_half = new_samples_per_half;
        new_analog_words_per_half = new_samples_per_half * a_cnt;
        need_bytes = new_digital_bytes_per_half * 2 + (a_cnt ? new_analog_words_per_half * 2 * sizeof(uint16_t) : 0);
    }
    if (free_heap < need_bytes + 16384) { JULSEDEBUG_ERR("LA: Not enough memory for ping-pong buffers after shrink.\n\r"); return false; }
    
    // Always (re)allocate to ensure clean state
    if (dig_half0) { delete [] dig_half0; dig_half0 = nullptr; }
    if (dig_half1) { delete [] dig_half1; dig_half1 = nullptr; }
    if (ana_half0) { delete [] ana_half0; ana_half0 = nullptr; }
    if (ana_half1) { delete [] ana_half1; ana_half1 = nullptr; }

    dig_half0 = new (std::nothrow) uint8_t[new_digital_bytes_per_half];
    dig_half1 = new (std::nothrow) uint8_t[new_digital_bytes_per_half];
    if (!dig_half0 || !dig_half1) return false;
    memset(dig_half0, 0, new_digital_bytes_per_half);
    memset(dig_half1, 0, new_digital_bytes_per_half);

    if (a_cnt > 0) {
        ana_half0 = new (std::nothrow) uint16_t[new_analog_words_per_half];
        ana_half1 = new (std::nothrow) uint16_t[new_analog_words_per_half];
        if (!ana_half0 || !ana_half1) return false;
        memset(ana_half0, 0, new_analog_words_per_half * sizeof(uint16_t));
        memset(ana_half1, 0, new_analog_words_per_half * sizeof(uint16_t));
    }

    // Commit new sizes
    samples_per_half = new_samples_per_half;
    digital_bytes_per_half = new_digital_bytes_per_half;
    analog_words_per_half = new_analog_words_per_half;
    // Initialize sample countdown and half index
    samples_remaining = num_samples;
    current_half_idx = 0;
    JULSEDEBUG_BUF("LA layout final: samples_per_half=%u bytes_dig_half=%u words_ana_half=%u remaining=%u\n\r",
               samples_per_half, digital_bytes_per_half, analog_words_per_half, samples_remaining);
    return true;
}




gpio_function_t last_gpio_function_map[10];
bool last_gpio_dir_map[10];
bool last_gpio_pull_up_map[10];
bool last_gpio_pull_down_map[10];
bool last_gpio_input_enabled_map[10];





bool LogicAnalyzer::setup_pio_for_digital() {
	if (!pio_loaded || pio_slow_offset < 0) {
		JULSEDEBUG_ERR("LA: PIO slow program not loaded; call init() earlier\n\r");
		return false;
	}
	bool slow = is_slow_mode();
	pio_sm_restart(lapio, lasm);
	pio_sm_set_enabled(lapio, lasm, false);
	pio_sm_clear_fifos(lapio, lasm);
	pio_sm_drain_tx_fifo(lapio, lasm);
	printGPIOState();

	for (int i = 0; i < 8; i++) { // errata fix save state
		last_gpio_function_map[i] = gpio_get_function(20 + i);
		last_gpio_dir_map[i] = gpio_get_dir(20 + i);
			// gpio_set_function(20 + i, GPIO_FUNC_SIO);
			// gpio_set_dir(20 + i, false);
			
		last_gpio_input_enabled_map[i] = pads_bank0_hw->io[20+i] & PADS_BANK0_GPIO0_IE_BITS;

		//JULSEDEBUG_DIG("LA PIO setup: gpio_input_enabled_map[%d] = %d\n\r", i, last_gpio_input_enabled_map[i]);
		// PADS_BANK0_GPIO0_IE_BITS,
		// PADS_BANK0_GPIO0_IE_BITS | PADS_BANK0_GPIO0_OD_BITS
		
		
		last_gpio_pull_up_map[i] = gpio_is_pulled_up(20 + i);
		last_gpio_pull_down_map[i] = gpio_is_pulled_down(20 + i);
		// gpio_set_input_enabled(20 + i, false);
		 //gpio_set_pulls(20 + i, false, false);
		 gpio_set_input_enabled(20 + i, true);
		
	}

	pio_sm_config c = pio_get_default_sm_config();
	sm_config_set_in_pins(&c, 20);
	sm_config_set_in_pin_count(&c, 8);
	sm_config_set_in_shift(&c, false, true, 8);
	sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_RX);

	uint offset = (uint)pio_slow_offset;

	if (slow) {
        sm_config_set_wrap(&c, offset, offset + prog_slow.length - 1);
        JULSEDEBUG_DIG("LA PIO setup: mode=slow offset=%u dreq=%d wrap=%s\n\r",
			   offset, dreq_pio_rx,
			   "full");
	}
	else {
        sm_config_set_wrap(&c, offset, offset);
        JULSEDEBUG_DIG("LA PIO setup: mode=fast offset=%u dreq=%d wrap=%s\n\r",
			   offset, dreq_pio_rx, "single");
    }
	pio_sm_init(lapio, lasm, offset, &c);
	pio_sm_set_enabled(lapio, lasm, false);
	dreq_pio_rx = pio_get_dreq(lapio, lasm, false);
	JULSEDEBUG_DIG("LA PIO setup: mode=%s offset=%u dreq=%d wrap=%s\n\r",
			   slow ? "slow" : "fast",
			   offset, dreq_pio_rx,
			   slow ? "full" : "single");
	return dreq_pio_rx >= 0;
}

bool LogicAnalyzer::setup_dma_for_digital() {
	if (dma_dig_0 < 0) dma_dig_0 = dma_claim_unused_channel(true);
	if (dma_dig_1 < 0) dma_dig_1 = dma_claim_unused_channel(true);
	cfg_dig_0 = dma_channel_get_default_config(dma_dig_0);
	cfg_dig_1 = dma_channel_get_default_config(dma_dig_1);
	channel_config_set_transfer_data_size(&cfg_dig_0, DMA_SIZE_8);
	channel_config_set_transfer_data_size(&cfg_dig_1, DMA_SIZE_8);
	channel_config_set_read_increment(&cfg_dig_0, false);
	channel_config_set_read_increment(&cfg_dig_1, false);
	channel_config_set_write_increment(&cfg_dig_0, true);
	channel_config_set_write_increment(&cfg_dig_1, true);
    channel_config_set_dreq(&cfg_dig_0, dreq_pio_rx);
    channel_config_set_dreq(&cfg_dig_1, dreq_pio_rx);
    // ping-pong chain
    channel_config_set_chain_to(&cfg_dig_0, dma_dig_1);
    channel_config_set_chain_to(&cfg_dig_1, dma_dig_0);

    dma_channel_configure(dma_dig_0, &cfg_dig_0,
                     dig_half0,
                     &lapio->rxf[lasm],
                     samples_per_half,  // 1 transfer per sample
                     false);
    dma_channel_configure(dma_dig_1, &cfg_dig_1,
                     dig_half1,
                     &lapio->rxf[lasm],
                     samples_per_half,  // 1 transfer per sample
                     false);
    // Track write address registers for ping-pong reset
    dig_taddr0 = &dma_hw->ch[dma_dig_0].write_addr;
    dig_taddr1 = &dma_hw->ch[dma_dig_1].write_addr;
    JULSEDEBUG_DMA("LA DMA DIG ch0=%d ch1=%d bytes_half=%u\n\r", dma_dig_0, dma_dig_1, digital_bytes_per_half);
	return true;
}

bool LogicAnalyzer::setup_adc_for_analog() {
	// Configure round-robin over enabled channels in a_mask
	uint8_t a_cnt = 0;
	adc_rr_mask = 0;
	for (int i = 0; i < 8; i++) if ((a_mask >> i) & 1) { a_cnt++; adc_rr_mask |= (1u << i); }
	if (a_cnt == 0) return true; // no analog

	adc_init();
	// Configure FIFO: EN write results, DREQ_EN, THRESH=1 (trigger DREQ when 1+ samples ready)
	// Use threshold=1 for immediate response and tight synchronization with PIO
	adc_fifo_setup(false, true, 1, false, false);
	adc_fifo_drain();
	// Use standard Pico SDK call for ADC divider

	int adc_decimation_factor = 1;

	while ((float)(sample_rate_hz * a_cnt / adc_decimation_factor) > 200000) {
		adc_decimation_factor++;
	}

	JULSEDEBUG_ANA("LA ADC DECIMATION FACTOR: %u (disabled for now)\n\r", adc_decimation_factor);
	float adc_div = 48000000.0f / (float)(sample_rate_hz * a_cnt);
	if (adc_div < 1.0f) adc_div = 1.0f;
	if (adc_div > 65535.0f) adc_div = 65535.0f;


	adc_set_clkdiv(adc_div);
	JULSEDEBUG_ANA("LA ADC CALC: div=%.2f target_total=%.1f per_ch=%.1f\n\r",
	               adc_div, (float)(sample_rate_hz * a_cnt), (float)sample_rate_hz);
	// Select first enabled channel and enable round-robin
	adc_select_input(__builtin_ctz(adc_rr_mask));
	adc_set_round_robin(adc_rr_mask);
	ana_enabled_count = 0;
	for (int i = 0; i < 8; i++) if ((a_mask >> i) & 1) ana_enabled_list[ana_enabled_count++] = i;
	JULSEDEBUG_ANA("LA ADC setup: a_cnt=%u rr_mask=0x%02X clkdiv=%.2f\n\r", ana_enabled_count, adc_rr_mask, adc_div);
	JULSEDEBUG_ANA("LA ADC timing: target_rate=%u total_adc_rate=%u effective_per_ch=%u\n\r",
	               sample_rate_hz, sample_rate_hz * a_cnt, sample_rate_hz);
	return true;
}

bool LogicAnalyzer::setup_dma_for_analog() {
	uint8_t a_cnt = ana_enabled_count;  // Use pre-calculated count
	if (a_cnt == 0) return true;
	if (dma_ana_0 < 0) dma_ana_0 = dma_claim_unused_channel(true);
	if (dma_ana_1 < 0) dma_ana_1 = dma_claim_unused_channel(true);
	cfg_ana_0 = dma_channel_get_default_config(dma_ana_0);
	cfg_ana_1 = dma_channel_get_default_config(dma_ana_1);
	channel_config_set_transfer_data_size(&cfg_ana_0, DMA_SIZE_16);
	channel_config_set_transfer_data_size(&cfg_ana_1, DMA_SIZE_16);
	channel_config_set_read_increment(&cfg_ana_0, false);
	channel_config_set_read_increment(&cfg_ana_1, false);
	channel_config_set_write_increment(&cfg_ana_0, true);
	channel_config_set_write_increment(&cfg_ana_1, true);
	channel_config_set_dreq(&cfg_ana_0, DREQ_ADC);
	channel_config_set_dreq(&cfg_ana_1, DREQ_ADC);
    // ping-pong chain
    channel_config_set_chain_to(&cfg_ana_0, dma_ana_1);
    channel_config_set_chain_to(&cfg_ana_1, dma_ana_0);

    // Analog DMA: samples_per_half × a_cnt transfers (one per channel per sample)
    uint32_t ana_transfers = samples_per_half * ana_enabled_count;
    dma_channel_configure(dma_ana_0, &cfg_ana_0,
                     ana_half0,
                     &adc_hw->fifo,
                     ana_transfers,
                     false);
    dma_channel_configure(dma_ana_1, &cfg_ana_1,
                     ana_half1,
                     &adc_hw->fifo,
                     ana_transfers,
                     false);
    ana_taddr0 = &dma_hw->ch[dma_ana_0].write_addr;
    ana_taddr1 = &dma_hw->ch[dma_ana_1].write_addr;
    JULSEDEBUG_DMA("LA DMA ANA ch0=%d ch1=%d words_half=%u\n\r", dma_ana_0, dma_ana_1, analog_words_per_half);
	return true;
}

bool LogicAnalyzer::arm() {
	if (!initialized) init();
	JULSEDEBUG_STA("LA arm() start\n\r");
	// Compute and allocate ping-pong halves
	if (!compute_layout()) return false;
	// Setup hardware
	if (!setup_pio_for_digital()) return false;
	if (!setup_dma_for_digital()) return false;
	if (!setup_adc_for_analog()) return false;
	if (!setup_dma_for_analog()) return false;
	armed = true;
	JULSEDEBUG_STA("LA arm() complete\n\r");
	return true;
}

void LogicAnalyzer::start_hardware() {
	// Recompute timing on every start so changes to sample_rate_hz take effect without re-arming
	uint32_t sysclk = clock_get_hz(clk_sys);
	bool slow_now = is_slow_mode();
	uint32_t cycles_per_sample = slow_now ? 32u : 1u;
	double desired_div = (double)sysclk / ((double)sample_rate_hz * (double)cycles_per_sample);
	if (desired_div < 1.0) desired_div = 1.0;
	if (desired_div > 65535.996) desired_div = 65535.996; // cap before rounding
	uint32_t intdiv = (uint32_t)desired_div;
	uint32_t frac8 = (uint32_t)llround((desired_div - (double)intdiv) * 256.0);
	if (frac8 >= 256) { frac8 = 0; intdiv++; }
	if (lasm >= 0) {
		pio_sm_set_clkdiv_int_frac(lapio, lasm, (uint16_t)intdiv, (uint8_t)frac8);
		// Compute actual rate from programmed divider
		double actual_div = (double)intdiv + (double)frac8 / 256.0;
		double actual_rate = (double)sysclk / (actual_div * (double)cycles_per_sample);
		JULSEDEBUG_DIG("LA PIO runtime div: target=%uHz cycles=%u div=%u+%u/256 actual=%.6fHz\n\r",
				   sample_rate_hz, cycles_per_sample, intdiv, frac8, actual_rate);
	}
	// Program ADC control
	uint8_t a_cnt = 0; for (int i=0;i<8;i++) if ((a_mask>>i)&1) a_cnt++;
	if (a_cnt > 0) {
		// FIFO: Enable write, DREQ, THRESH=1, no error byte shift
		adc_fifo_setup(false, true, 1, false, false);
		adc_fifo_drain();
		adc_run(false);
		adc_hw->cs = 0;
		adc_hw->fcs = ADC_FCS_EN_BITS | ADC_FCS_DREQ_EN_BITS | (1u << ADC_FCS_THRESH_LSB);
		if (slow_now) {
			// Slow mode: fastest ADC clock, manual per-sample triggering via PIO IRQ
			adc_set_clkdiv(1.0f);
			// Select first channel, disable round-robin; do not START_MANY here
			adc_hw->cs |= ADC_CS_EN_BITS;
			adc_hw->cs = (adc_hw->cs & ~ADC_CS_AINSEL_BITS) | (((uint)(__builtin_ctz(adc_rr_mask)) << ADC_CS_AINSEL_LSB) & ADC_CS_AINSEL_BITS);
			adc_hw->cs &= ~ADC_CS_RROBIN_BITS;
			JULSEDEBUG_ANA("LA start(SLOW): ADC EN, clkdiv=1.0, manual START_ONCE via PIO IRQ, first=%u\n\r", (unsigned)__builtin_ctz(adc_rr_mask));
		} else {
			// Normal mode: set divider and RR masks; defer START_MANY until after DMA starts
			float adc_div = 48000000.0f / (float)(sample_rate_hz * a_cnt);
			if (adc_div < 1.0f) adc_div = 1.0f;
			if (adc_div > 65535.0f) adc_div = 65535.0f;
			adc_set_clkdiv(adc_div);
			adc_hw->cs |= ADC_CS_EN_BITS;
			adc_hw->cs = (adc_hw->cs & ~ADC_CS_AINSEL_BITS) | (((uint)(__builtin_ctz(adc_rr_mask)) << ADC_CS_AINSEL_LSB) & ADC_CS_AINSEL_BITS);
			adc_hw->cs = (adc_hw->cs & ~ADC_CS_RROBIN_BITS) | (((uint)adc_rr_mask << ADC_CS_RROBIN_LSB) & ADC_CS_RROBIN_BITS);
			JULSEDEBUG_ANA("LA prep(NORM): ADC ready rr=0x%02X div=%.2f (START_MANY deferred)\n\r", (unsigned)(adc_rr_mask), adc_div);
		}
	}
	// Prepare PIO SM, but defer enabling until after DMA starts
	pio_sm_clear_fifos(lapio, lasm);
	if (slow_now) {
		pio_interrupt_clear(lapio, 1);
		// Enable PIO block interrupt flag 1 to route to BOTH CPU IRQ lines for robustness
		pio_set_irq0_source_enabled(lapio, pis_interrupt1, true);
		//pio_set_irq1_source_enabled(lapio, pis_interrupt1, true);
		active_instance = this;
		// Map active PIO to its IRQ lines
		uint irq0_num = PIO0_IRQ_0;
		uint irq1_num = PIO0_IRQ_1;
		if (lapio == pio1) { irq0_num = PIO1_IRQ_0; irq1_num = PIO1_IRQ_1; }
		#ifdef PIO2_IRQ_0
		else if (lapio == pio2) { irq0_num = PIO2_IRQ_0; irq1_num = PIO2_IRQ_1; }
		#endif
		if (!irq_has_shared_handler(irq0_num)) {
			irq_add_shared_handler(irq0_num, [](){ LogicAnalyzer::pio_irq1_isr(); }, 0);
			irq_set_enabled(irq0_num, true);
		}
		// if (!irq_has_shared_handler(irq1_num)) {
		// 	irq_add_shared_handler(irq1_num, [](){ LogicAnalyzer::pio_irq1_isr(); }, 0);
		// 	irq_set_enabled(irq1_num, true);
		// }
		JULSEDEBUG_DIG("LA prep: slow IRQ pacing configured (irq0=%u irq1=%u)\n\r", (unsigned)irq0_num, (unsigned)irq1_num);
	}
	// Do NOT enable PIO SM here; will be enabled after DMA starts
}


unsigned long usb_send_time[50];

unsigned long dma_capture_time[50];
int dma_capture_idx = 0;
int usb_send_idx = 0;

void LogicAnalyzer::run() {
	if (!armed) return;
	running = true;

    for (int i = 0; i < 50; i++) usb_send_time[i] = 0;
	for (int i = 0; i < 50; i++) dma_capture_time[i] = 0;
    usb_send_idx = 0;
	dma_capture_idx = 0;


	JULSEDEBUG_STA("LA run(): N=%u rate=%u Hz half=%u mode=%s\n\r", 
	               num_samples, sample_rate_hz, samples_per_half,
	               is_slow_mode() ? "SLOW" : (is_normal_mode() ? "NORMAL" : "OTHER"));
	
	// Use configuration prepared by arm(); do not reallocate/reconfigure here to avoid
	// unnecessary heap churn and DMA/PIO resets between captures.
	
	// Reset per-run counters and DMA addresses for both halves
	samples_remaining = num_samples;
	current_half_idx = 0;
	if (dig_taddr0) *dig_taddr0 = (uint32_t)dig_half0;
	if (dig_taddr1) *dig_taddr1 = (uint32_t)dig_half1;
	if (ana_taddr0) *ana_taddr0 = (uint32_t)ana_half0;
	if (ana_taddr1) *ana_taddr1 = (uint32_t)ana_half1;

	// Make sure any previous run's pending USB data is flushed and local TX index reset
#ifdef USE_TINYUSB
	tud_task();
	tud_cdc_n_write_flush(2);
#endif
	txidx = 0;
	
	uint8_t a_cnt_runtime = ana_enabled_count;
	JULSEDEBUG_DMA("LA DMA counts preset in setup (acnt=%u)\n\r", a_cnt_runtime);

	// In slow mode, show samples roughly every 0.1s by capping half size
	uint32_t effective_half_samples = samples_per_half;
	if (is_slow_mode()) {
		uint32_t desired = sample_rate_hz / 10u; // samples in 100ms
		if (desired == 0) desired = 1;
		if (desired < effective_half_samples) effective_half_samples = desired;
		// Program both DMA channels to this capped size so ping-pong halves are ~0.1s
		if (dma_dig_0 >= 0) dma_channel_set_trans_count(dma_dig_0, effective_half_samples, false);
		if (dma_dig_1 >= 0) dma_channel_set_trans_count(dma_dig_1, effective_half_samples, false);
		if (a_cnt_runtime) {
			uint32_t ana_cnt = effective_half_samples * a_cnt_runtime;
			if (dma_ana_0 >= 0) dma_channel_set_trans_count(dma_ana_0, ana_cnt, false);
			if (dma_ana_1 >= 0) dma_channel_set_trans_count(dma_ana_1, ana_cnt, false);
		}
		JULSEDEBUG_DMA("LA slow cap: half_samples=%u (%.1f ms)\n\r", effective_half_samples, (effective_half_samples * 1000.0f) / (float)sample_rate_hz);
	}
	
	// Start hardware (ADC/PIO only, no DMA starts)
	start_hardware();
	//adc_run(false);
	// Start first DMA channels (they will chain automatically per setup)
	if (dma_dig_0 >= 0) dma_channel_start(dma_dig_0);
	if (a_cnt_runtime && dma_ana_0 >= 0) dma_channel_start(dma_ana_0);


	
	delayMicroseconds(100000);
	// adc_fifo_drain();

	// Now that DMA is primed, enable PIO SM and start ADC conversions (normal mode)
	


	io_rw_32 adcMask = adc_hw->cs;
	if (adc_rr_mask) {
		uint first_ch = (uint)__builtin_ctz(adc_rr_mask);
		adcMask = (adcMask & ~ADC_CS_AINSEL_BITS) | (((first_ch << ADC_CS_AINSEL_LSB) & ADC_CS_AINSEL_BITS));
	}
	adcMask |= ADC_CS_START_MANY_BITS;
	
	
	if (!is_slow_mode()) {
		
		adc_hw->cs = adcMask;
	}
	pio_sm_set_enabled(lapio, lasm, true);
 
	uint32_t total_sent = 0;
	unsigned long start_time = micros();
	
	auto wait_half_complete = [&](int idx) {
		int ch_d = (idx == 0) ? dma_dig_0 : dma_dig_1;
		int ch_a = (idx == 0) ? dma_ana_0 : dma_ana_1;
		while (dma_channel_is_busy(ch_d) || (a_cnt_runtime && dma_channel_is_busy(ch_a))) {
			// spin; hardware paced
		}
	};

	delayMicroseconds(1000);
	
	while (samples_remaining > 0) { //! this is the capture loop
		// Wait for current half completion
		start_time = micros();
		wait_half_complete(current_half_idx);
		
		uint32_t this_half = effective_half_samples;
		if (this_half > samples_remaining) this_half = samples_remaining;


		unsigned long capture_time = micros() - start_time;
		dma_capture_time[dma_capture_idx] = capture_time;
		dma_capture_idx = (dma_capture_idx + 1) % 50;
		


		// Reset only write address for the just-completed half; do not touch counts
		if (current_half_idx == 0) {
			if (dig_taddr0) *dig_taddr0 = (uint32_t)dig_half0;
			if (a_cnt_runtime && ana_taddr0) *ana_taddr0 = (uint32_t)ana_half0;
		} else {
			if (dig_taddr1) *dig_taddr1 = (uint32_t)dig_half1;
			if (a_cnt_runtime && ana_taddr1) *ana_taddr1 = (uint32_t)ana_half1;
		}
		
		// Process and send the completed half
		send_half(current_half_idx, this_half);
		total_sent += this_half;
		samples_remaining -= this_half;
		if (samples_remaining == 0 || USBSer2.peek() == '+') break;
		
		// Switch to next half
		current_half_idx ^= 1;
	}
	

	char completion[32];
	uint32_t a_cnt = ana_enabled_count;
	uint32_t bytes_per_sample = 2 + (a_cnt * 2);
	sprintf(completion, "$%u+", total_sent * bytes_per_sample);
	usb_write_blocking((const uint8_t*)completion, strlen(completion));
	stop();
	
    // Print USB send times
    JULSEDEBUG_USB("LA USB send times:    ");
    for (int i = 0; i < usb_send_idx; i++) {
        JULSEDEBUG_USB("%7lu us  ", usb_send_time[i]);
    }
    JULSEDEBUG_USB("\n\r");

	// Print DMA capture times
    JULSEDEBUG_USB("LA DMA capture times: ");
    for (int i = 0; i < dma_capture_idx; i++) {
        JULSEDEBUG_USB("%7lu us  ", dma_capture_time[i]);
    }
    JULSEDEBUG_USB("\n\r");

    //running = false;
    JULSEDEBUG_STA("LA run() complete: total_sent=%u bytes/sample=%u\n\r", total_sent, bytes_per_sample);

#ifdef USE_TINYUSB
    tud_task();
    tud_cdc_n_write_flush(2);
#endif
   // txidx = 0;
   // armed = false;

    // Fully teardown to free memory after each capture
    deinit();

	watchdog_disable();
}

void LogicAnalyzer::resetState() {
    // Clear staging and flags; keep allocations and DMA channel claims
#ifdef USE_TINYUSB
    tud_task();
    tud_cdc_n_write_flush(2);
#endif
    txidx = 0;
    running = false;
    // Do not clear 'armed' here; caller decides whether to keep armed state
}

void LogicAnalyzer::stop() {
	JULSEDEBUG_STA("LA stop()\n\r");
	// Abort all DMA
	if (dma_dig_0 >= 0) dma_channel_abort(dma_dig_0);
	if (dma_dig_1 >= 0) dma_channel_abort(dma_dig_1);
	if (dma_ana_0 >= 0) dma_channel_abort(dma_ana_0);
	if (dma_ana_1 >= 0) dma_channel_abort(dma_ana_1);
	// Disable PIO
	if (lasm >= 0) pio_sm_set_enabled(lapio, lasm, false);


	//JULSEDEBUG_STA("adc fifo left: %d\n\r", adc_fifo_get_level());
	if (adc_fifo_get_level() > 0) {
		JULSEDEBUG_STA("adc fifo left: %d\n\r", adc_fifo_get_level());
		adc_fifo_drain();
	}

		// ADC stop
		adc_run(false);

		JULSEDEBUG_STA("adc fifo left after stop: %d\n\r", adc_fifo_get_level());
	// Reset DMA write addrs and counts to safe defaults
	if (dig_taddr0) *dig_taddr0 = (uint32_t)dig_half0;
	if (dig_taddr1) *dig_taddr1 = (uint32_t)dig_half1;
	if (ana_taddr0) *ana_taddr0 = (uint32_t)ana_half0;
	if (ana_taddr1) *ana_taddr1 = (uint32_t)ana_half1;
	if (dma_dig_0 >= 0) dma_channel_set_trans_count(dma_dig_0, 0, false);
	if (dma_dig_1 >= 0) dma_channel_set_trans_count(dma_dig_1, 0, false);
	if (dma_ana_0 >= 0) dma_channel_set_trans_count(dma_ana_0, 0, false);
	if (dma_ana_1 >= 0) dma_channel_set_trans_count(dma_ana_1, 0, false);
	// Clear PIO FIFOs and restart SM
	pio_sm_restart(lapio, lasm);
	pio_sm_clear_fifos(lapio, lasm);

	JULSEDEBUG_STA("LA stop()\n\r");
	initADC();

//erattaClearGPIO();

	for (int i = 0; i < 8; i++) { // errata fix 
		//gpio_set_function(20 + i, last_gpio_function_map[i]);

		gpio_set_input_enabled(20 + i, false);
		delayMicroseconds(1);

		gpio_set_input_enabled(20 + i, last_gpio_input_enabled_map[i]);

		gpio_set_function(20 + i, last_gpio_function_map[i]);
		
		gpio_set_dir(20 + i, last_gpio_dir_map[i]);
		//gpio_set_input_enabled(20 + i, true);
		gpio_set_pulls(20 + i, last_gpio_pull_up_map[i], last_gpio_pull_down_map[i]);
	}

	// Ensure USB CDC write FIFO is flushed between runs to avoid stale data
#ifdef USE_TINYUSB
	tud_task();
	tud_cdc_n_write_flush(2);
#endif
	// Clear TX staging buffer index
	//txidx = 0;

}

void LogicAnalyzer::deinit() {
	//stop();
	if (dma_dig_0 >= 0) { dma_channel_unclaim(dma_dig_0); dma_dig_0 = -1; }
	if (dma_dig_1 >= 0) { dma_channel_unclaim(dma_dig_1); dma_dig_1 = -1; }
	if (dma_ana_0 >= 0) { dma_channel_unclaim(dma_ana_0); dma_ana_0 = -1; }
	if (dma_ana_1 >= 0) { dma_channel_unclaim(dma_ana_1); dma_ana_1 = -1; }
	if (dig_half0) { delete [] dig_half0; dig_half0 = nullptr; }
	if (dig_half1) { delete [] dig_half1; dig_half1 = nullptr; }
	if (ana_half0) { delete [] ana_half0; ana_half0 = nullptr; }
	if (ana_half1) { delete [] ana_half1; ana_half1 = nullptr; }
	armed = false; initialized = false; running = false; txidx = 0;




}

uint8_t LogicAnalyzer::pack_digital_8(uint8_t pio_bits) {
	return pio_bits; // lower 8 bits only (GP20..GP27)
}

void LogicAnalyzer::encode_and_queue_digital(uint8_t digi) {
	// ASCII 7-bit packing across 2 bytes for 8 bits
	uint8_t b0 = (digi & 0x7F) + 0x30; if (b0 < 0x80) b0 |= 0x80;
	uint8_t b1 = ((digi >> 7) & 0x01) + 0x30; if (b1 < 0x80) b1 |= 0x80;
	if (txidx + 2 > TX_BUF_SIZE) send_flush();
	txbuf[txidx++] = b0; txbuf[txidx++] = b1;
}

void LogicAnalyzer::encode_and_queue_analog(uint16_t v12) {
	uint8_t b0 = (v12 & 0x7F) + 0x30;
	uint8_t b1 = ((v12 >> 7) & 0x7F) + 0x30;
	if (txidx + 2 > TX_BUF_SIZE) send_flush();
	txbuf[txidx++] = b0; txbuf[txidx++] = b1;
}

bool LogicAnalyzer::usb_write_blocking(const uint8_t* data, int len) {
#ifdef USE_TINYUSB
	if (!tud_cdc_n_connected(2)) return false;
	// Push big chunks into TinyUSB TX fifo; rely on its flush threshold to xfer
	int off = 0;
	while (off < len) {
		uint32_t space = tud_cdc_n_write_available(2);
		if (space == 0) {
			// Give TinyUSB a chance to move data to USB IN endpoint
			tud_task();
			// Try to flush any queued packet if endpoint claimed
			tud_cdc_n_write_flush(2);
			// brief wait to avoid busy-spin
			delayMicroseconds(10);
			continue;
		}
		uint32_t chunk = (uint32_t)(len - off);
		if (chunk > space) chunk = space;
		uint32_t w = tud_cdc_n_write(2, data + off, chunk);
		off += (int)w;
		// Periodically service USB
		tud_task();
	}
	// Final flush ensures tail < packet size is sent promptly
	tud_cdc_n_write_flush(2);
	return true;
#else
	return false;
#endif
}

void LogicAnalyzer::send_flush() {
	if (txidx == 0) return;
	usb_write_blocking(txbuf, txidx);
	txidx = 0;
}

void LogicAnalyzer::send_half(uint32_t half_index, uint32_t samples_in_half) {
	uint8_t* dptr = (half_index == 0) ? dig_half0 : dig_half1;
	uint16_t* aptr = (half_index == 0) ? ana_half0 : ana_half1;
	uint8_t a_cnt = 0; for (int i=0;i<8;i++) if ((a_mask>>i)&1) a_cnt++;

    unsigned long start_time = micros();
	// Larger batching with partial flush: try to fill big TX buffer, flush only when needed
	for (uint32_t i = 0; i < samples_in_half; i++) {
		uint8_t digi = pack_digital_8(dptr[i]);
		encode_and_queue_digital(digi);
		if (a_cnt) {
			uint32_t base = i * a_cnt;
			for (int ch = 0; ch < 8; ch++) {
				if (!((a_mask >> ch) & 1)) continue;
				uint16_t raw = aptr[base++];
				uint16_t v12 = raw & 0x0FFF; // 12-bit
				encode_and_queue_analog(v12);
			}
		}
		// Flush when buffer is nearing capacity; keep 256-byte headroom
		if (txidx >= (TX_BUF_SIZE - 256)) send_flush();
	}
	send_flush();
    usb_send_time[usb_send_idx] = micros() - start_time;
    usb_send_idx = (usb_send_idx + 1) % 50;

	watchdog_update();
}

void LogicAnalyzer::pio_irq1_isr() {
	if (active_instance) active_instance->on_pio_irq1();
}

void LogicAnalyzer::on_pio_irq1() {
	// Slow mode ISR: kick ADC START_ONCE per enabled channel with READY gating
	if (!is_slow_mode() || ana_enabled_count == 0) {
		pio_interrupt_clear(lapio, 1);
		return;
	}
	for (uint8_t i = 0; i < ana_enabled_count; i++) {
		uint8_t ch = ana_enabled_list[i];
		adc_select_input(ch);
		adc_hw->cs |= ADC_CS_START_ONCE_BITS;
		while ((adc_hw->cs & ADC_CS_READY_BITS) == 0) {}
	}
	pio_interrupt_clear(lapio, 1);
}


