#include "AsyncPassthrough.h"
#if ASYNC_PASSTHROUGH_ENABLED == 1
// pico-sdk
#include <hardware/uart.h>
#include <hardware/irq.h>
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include "config.h"
// TinyUSB
#include <tusb.h>

// Arduino core
#include <Arduino.h>

bool asyncPassthroughEnabled = jumperlessConfig.serial_1.async_passthrough;

// ----------------------------------------------------------------------------
// Configuration
// ----------------------------------------------------------------------------

#ifndef ASYNC_PASSTHROUGH_CDC_ITF
	// Bridge TinyUSB CDC instance 1 (USBSer1) <-> Serial1
	#define ASYNC_PASSTHROUGH_CDC_ITF 1
#endif

#ifndef ASYNC_PASSTHROUGH_UART
	#define ASYNC_PASSTHROUGH_UART uart0
	#define ASYNC_PASSTHROUGH_UART_IRQ UART0_IRQ

	#define ASYNC_PASSTHROUGH_UART_TX_PIN 0
	#define ASYNC_PASSTHROUGH_UART_RX_PIN 1
#endif

#ifndef ASYNC_PASSTHROUGH_UART_DEFAULT_BAUD
	#define ASYNC_PASSTHROUGH_UART_DEFAULT_BAUD 115200u
#endif






// ----------------------------------------------------------------------------
// Helpers / State
// ----------------------------------------------------------------------------

static inline uint16_t make_serial_config_from_line_coding(uint8_t data_bits, uint8_t parity, uint8_t stop_bits) {
	// Map TinyUSB CDC line coding to Arduino's SERIAL_* bitfield
	unsigned long data = 0x400ul; // default 8 bits
	unsigned long par  = 0x3ul;   // default NONE
	unsigned long stop = 0x10ul;  // default 1 stop bit

	switch (data_bits) {
		case 5: data = 0x100ul; break; // SERIAL_DATA_5
		case 6: data = 0x200ul; break; // SERIAL_DATA_6
		case 7: data = 0x300ul; break; // SERIAL_DATA_7
		default: data = 0x400ul; break; // SERIAL_DATA_8
	}

	// Per CDC spec: 0=None,1=Odd,2=Even,3=Mark,4=Space
	switch (parity) {
		case 2: par = 0x1ul; break; // SERIAL_PARITY_EVEN
		case 1: par = 0x2ul; break; // SERIAL_PARITY_ODD
		case 3: par = 0x4ul; break; // SERIAL_PARITY_MARK
		case 4: par = 0x5ul; break; // SERIAL_PARITY_SPACE
		default: par = 0x3ul; break; // SERIAL_PARITY_NONE
	}

	// Per CDC spec: 0=1 stop, 1=1.5 stop, 2=2 stop
	switch (stop_bits) {
		case 2: stop = 0x30ul; break; // SERIAL_STOP_BIT_2
		case 1: stop = 0x20ul; break; // SERIAL_STOP_BIT_1_5
		default: stop = 0x10ul; break; // SERIAL_STOP_BIT_1
	}

	return (uint16_t)(data | par | stop);
}

static inline void bridge_usb_to_uart(uint8_t itf) {
	// Drain CDC RX and forward to Serial1
	uint8_t buf[128];
	while (tud_cdc_n_available(itf)) {
		size_t rd = tud_cdc_n_read(itf, buf, sizeof(buf));
		if (rd == 0) break;
		if (Serial1) {
			Serial1.write(buf, rd);
		}
	}
}

static inline void bridge_uart_to_usb(uint8_t itf) {
	// Drain Serial1 RX and forward to CDC
	if (!tud_inited()) return;
	if (!tud_cdc_n_connected(itf)) return;

	uint8_t buf[128];
	int idx = 0;
	while (Serial1.available() && idx < (int)sizeof(buf)) {
		int c = Serial1.read();
		if (c < 0) break;
		buf[idx++] = (uint8_t)c;
	}
	if (idx > 0) {
		tud_cdc_n_write(itf, buf, (uint32_t)idx);
		tud_cdc_n_write_flush(itf);
	}
}

// ISR-safe flags updated by TinyUSB callbacks, processed in main context
static volatile bool s_usb_rx_pending = false;
static volatile bool s_apply_line_coding_pending = false;
static cdc_line_coding_t s_line_coding = { .bit_rate = ASYNC_PASSTHROUGH_UART_DEFAULT_BAUD, .stop_bits = 0, .parity = 0, .data_bits = 8 };

// ----------------------------------------------------------------------------
// TinyUSB CDC Callbacks (C linkage)
// ----------------------------------------------------------------------------

extern "C" {
	
	void tud_cdc_rx_cb(uint8_t itf) {
		// Mark pending work; do actual bridging in main context to avoid ISR reentrancy
		if (itf == ASYNC_PASSTHROUGH_CDC_ITF) {
			s_usb_rx_pending = true;
		}
	}

	// Cache new line coding and apply it in main context
	void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const* p) {
		if (itf != ASYNC_PASSTHROUGH_CDC_ITF) return;
		s_line_coding = *p;
		s_apply_line_coding_pending = true;
	}
}

// ----------------------------------------------------------------------------
// Foreground task runner
// ----------------------------------------------------------------------------

namespace AsyncPassthrough {

void task() {
	// Apply pending line coding from host
	if (s_apply_line_coding_pending) {
		uint16_t cfg = make_serial_config_from_line_coding(s_line_coding.data_bits, s_line_coding.parity, s_line_coding.stop_bits);
		if (Serial1) {
			Serial1.end();
		}
		Serial1.begin(s_line_coding.bit_rate, cfg);
		s_apply_line_coding_pending = false;
	}

	// USB -> UART when either pending flag set or data available
	if (s_usb_rx_pending || (tud_inited() && tud_cdc_n_available(ASYNC_PASSTHROUGH_CDC_ITF))) {
		bridge_usb_to_uart(ASYNC_PASSTHROUGH_CDC_ITF);
		s_usb_rx_pending = false;
	}

	// UART -> USB
	bridge_uart_to_usb(ASYNC_PASSTHROUGH_CDC_ITF);
}

} // namespace AsyncPassthrough















#endif















