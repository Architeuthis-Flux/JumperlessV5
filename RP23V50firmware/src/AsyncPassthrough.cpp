#include "AsyncPassthrough.h"
#include "class/cdc/cdc_device.h"
#if ASYNC_PASSTHROUGH_ENABLED == 1
// pico-sdk
#include "config.h"
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/uart.h>
#include <hardware/regs/uart.h>
#include <hardware/structs/uart.h>
#include <pico/stdlib.h>
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

unsigned long usPerByteSerial1 = ( 1000000 / 115200 + 1 ) * ( 8 + 0 + 1 );
unsigned long serial1baud = 115200;

// ----------------------------------------------------------------------------
// Helpers / State
// ----------------------------------------------------------------------------

static inline void set_micros_per_byte( cdc_line_coding_t const* line_coding ) {

    // Compute an approximate microseconds-per-byte based on line coding.
    // Bits per character = 1 start + data_bits + parity(0|1) + stop_bits(1|1.5|2)
    const uint32_t us_per_bit = ( 1000000u + line_coding->bit_rate - 1 ) / line_coding->bit_rate; // ceil

    const uint32_t parity_bits = ( line_coding->parity == 0 ? 0u : 1u );

    // CDC stop_bits: 0=1 stop, 1=1.5 stop, 2=2 stop
    uint32_t us_for_stops = us_per_bit; // default 1 stop
    if ( line_coding->stop_bits == 2 ) {
        us_for_stops = 2u * us_per_bit;
    } else if ( line_coding->stop_bits == 1 ) {
        // 1.5 stop bits -> round up
        us_for_stops = us_per_bit + ( us_per_bit >> 1 );
    }

    const uint32_t us_without_stops = ( 1u + (uint32_t)line_coding->data_bits + parity_bits ) * us_per_bit;
    usPerByteSerial1 = us_without_stops + us_for_stops;
	serial1baud = line_coding->bit_rate;
}

static inline uint16_t make_serial_config_from_line_coding( uint8_t data_bits, uint8_t parity, uint8_t stop_bits ) {
    // Map TinyUSB CDC line coding to Arduino's SERIAL_* bitfield
    unsigned long data = 0x400ul; // default 8 bits
    unsigned long par = 0x3ul;    // default NONE
    unsigned long stop = 0x10ul;  // default 1 stop bit

    switch ( data_bits ) {
    case 5:
        data = 0x100ul;
        break; // SERIAL_DATA_5
    case 6:
        data = 0x200ul;
        break; // SERIAL_DATA_6
    case 7:
        data = 0x300ul;
        break; // SERIAL_DATA_7
    default:
        data = 0x400ul;
        break; // SERIAL_DATA_8
    }

    // Per CDC spec: 0=None,1=Odd,2=Even,3=Mark,4=Space
    switch ( parity ) {
    case 2:
        par = 0x1ul;
        break; // SERIAL_PARITY_EVEN
    case 1:
        par = 0x2ul;
        break; // SERIAL_PARITY_ODD
    case 3:
        par = 0x4ul;
        break; // SERIAL_PARITY_MARK
    case 4:
        par = 0x5ul;
        break; // SERIAL_PARITY_SPACE
    default:
        par = 0x3ul;
        break; // SERIAL_PARITY_NONE
    }

    // Per CDC spec: 0=1 stop, 1=1.5 stop, 2=2 stop
    switch ( stop_bits ) {
    case 2:
        stop = 0x30ul;
        break; // SERIAL_STOP_BIT_2
    case 1:
        stop = 0x20ul;
        break; // SERIAL_STOP_BIT_1_5
    default:
        stop = 0x10ul;
        break; // SERIAL_STOP_BIT_1
    }

    return (uint16_t)( data | par | stop );
}

// ----------------------------------------------------------------------------
// UART RX IRQ support: push to TinyUSB or fallback ring buffer
// ----------------------------------------------------------------------------

static volatile bool s_uart_flush_pending = false;
static uint8_t s_uart_rx_ring[ 4096 ];
static volatile uint16_t s_uart_rx_head = 0;
static volatile uint16_t s_uart_rx_tail = 0;
static volatile uint32_t s_uart_ring_overflow_count = 0;
static volatile uint32_t s_uart_overrun_count = 0;
#define UART_RING_MASK ( (uint16_t)( sizeof( s_uart_rx_ring ) - 1 ) )

static inline bool ring_push_byte( uint8_t b ) {
    uint16_t next_head = (uint16_t)( ( s_uart_rx_head + 1 ) & UART_RING_MASK );
    if ( next_head == s_uart_rx_tail ) {
        s_uart_ring_overflow_count++;
        return false;
    }
    s_uart_rx_ring[ s_uart_rx_head ] = b;
    s_uart_rx_head = next_head;
    return true;
}

static inline bool ring_pop_byte( uint8_t* out ) {
    if ( s_uart_rx_tail == s_uart_rx_head ) return false;
    *out = s_uart_rx_ring[ s_uart_rx_tail ];
    s_uart_rx_tail = (uint16_t)( ( s_uart_rx_tail + 1 ) & UART_RING_MASK );
    return true;
}

static void async_uart_irq_handler( void ) {
    while ( uart_is_readable( ASYNC_PASSTHROUGH_UART ) ) {
        uint8_t c = (uint8_t)uart_getc( ASYNC_PASSTHROUGH_UART );
        ring_push_byte( c );
    }
    // Record and clear sticky error flags (e.g., overrun)
    uart_hw_t* hw = uart_get_hw( ASYNC_PASSTHROUGH_UART );
    uint32_t rsr = hw->rsr;
    if ( rsr & ( UART_UARTRSR_OE_BITS | UART_UARTRSR_FE_BITS | UART_UARTRSR_PE_BITS | UART_UARTRSR_BE_BITS ) ) {
        if ( rsr & UART_UARTRSR_OE_BITS ) {
            s_uart_overrun_count++;
        }
        hw->rsr = 0xFFFFFFFFu; // write to RSR (alias of ECR) clears errors
    }
    s_uart_flush_pending = true;
}

static inline void bridge_usb_to_uart( uint8_t itf ) {
    // Drain CDC RX and forward to UART (pico-sdk, HW FIFO)
    uint8_t buf[ 2048 ];
    while ( tud_cdc_n_available( itf ) ) {
        size_t rd = tud_cdc_n_read( itf, buf, sizeof( buf ) );
        if ( rd == 0 )
            break;
        uart_write_blocking( ASYNC_PASSTHROUGH_UART, buf, rd );
    }
}

static inline void bridge_uart_to_usb( uint8_t itf ) {
    // Flush any ring-buffered bytes from UART IRQ to CDC
    if ( !tud_inited( ) ) return;
    if ( !tud_cdc_n_connected( itf ) ) return;
    uint32_t wrote = 0;
    uint32_t avail = tud_cdc_n_write_available( itf );
    uint8_t c;
    while ( avail > 0 && ring_pop_byte( &c ) ) {
        tud_cdc_n_write_char( itf, c );
        wrote++;
        avail--;
    }
    if ( wrote > 0 || s_uart_flush_pending ) {
        tud_cdc_n_write_flush( itf );
        tud_task(); // service USB to reduce latency under load
    }
    s_uart_flush_pending = ( s_uart_rx_tail != s_uart_rx_head );
}

// ISR-safe flags updated by TinyUSB callbacks, processed in main context
static volatile bool s_usb_rx_pending = false;
static volatile bool s_apply_line_coding_pending = false;
static cdc_line_coding_t s_line_coding = { .bit_rate = ASYNC_PASSTHROUGH_UART_DEFAULT_BAUD, .stop_bits = 1, .parity = 0, .data_bits = 8 };

// ----------------------------------------------------------------------------
// TinyUSB CDC Callbacks (C linkage)
// ----------------------------------------------------------------------------

extern "C" {

void tud_cdc_rx_cb( uint8_t itf ) {
    // Mark pending work; do actual bridging in main context to avoid ISR reentrancy
    if ( itf == ASYNC_PASSTHROUGH_CDC_ITF ) {
        s_usb_rx_pending = true;
    }
}

// Cache new line coding and apply it in main context
void tud_cdc_line_coding_cb( uint8_t itf, cdc_line_coding_t const* p ) {
    if ( itf != ASYNC_PASSTHROUGH_CDC_ITF )
        return;
    s_line_coding = *p;
    s_apply_line_coding_pending = true;
}
}

// ----------------------------------------------------------------------------
// Foreground task runner
// ----------------------------------------------------------------------------

namespace AsyncPassthrough {

void begin( unsigned long baud ) {
    // Configure UART pins and UART with HW FIFO enabled
    gpio_set_function( ASYNC_PASSTHROUGH_UART_TX_PIN, GPIO_FUNC_UART );
    gpio_set_function( ASYNC_PASSTHROUGH_UART_RX_PIN, GPIO_FUNC_UART );

    uart_init( ASYNC_PASSTHROUGH_UART, baud );
    uart_set_format( ASYNC_PASSTHROUGH_UART, 8, 1, UART_PARITY_NONE );
    uart_set_fifo_enabled( ASYNC_PASSTHROUGH_UART, true );

    // Enable RX interrupt for immediate forwarding; include RX timeout interrupt
    irq_set_exclusive_handler( ASYNC_PASSTHROUGH_UART_IRQ, async_uart_irq_handler );
    irq_set_priority( ASYNC_PASSTHROUGH_UART_IRQ, 0 ); // highest priority
    irq_set_enabled( ASYNC_PASSTHROUGH_UART_IRQ, true );
    uart_set_irq_enables( ASYNC_PASSTHROUGH_UART, true, false );
    // Ensure RX timeout and overrun interrupts are enabled
    hw_set_bits( &uart_get_hw( ASYNC_PASSTHROUGH_UART )->imsc, UART_UARTIMSC_RTIM_BITS | UART_UARTIMSC_OEIM_BITS );
    // Set RX IRQ trigger at 1/8 FIFO, TX at 1/2 FIFO
    hw_write_masked( &uart_get_hw( ASYNC_PASSTHROUGH_UART )->ifls,
                     ( 0u << UART_UARTIFLS_RXIFLSEL_LSB ) | ( 2u << UART_UARTIFLS_TXIFLSEL_LSB ),
                     UART_UARTIFLS_RXIFLSEL_BITS | UART_UARTIFLS_TXIFLSEL_BITS );

    // Track initial micros per byte based on default config
    s_line_coding.bit_rate = (uint32_t)baud;
    s_line_coding.data_bits = 8;
    s_line_coding.parity = 0;
    s_line_coding.stop_bits = 0; // 1 stop
    set_micros_per_byte( &s_line_coding );
}

void task( ) {
    // Apply pending line coding from host
    if ( s_apply_line_coding_pending ) {
        // Apply to pico-sdk UART
        uint data_bits = 8;
        switch ( s_line_coding.data_bits ) {
        case 5: data_bits = 5; break;
        case 6: data_bits = 6; break;
        case 7: data_bits = 7; break;
        default: data_bits = 8; break;
        }

        uart_parity_t parity = UART_PARITY_NONE;
        if ( s_line_coding.parity == 1 ) {
            parity = UART_PARITY_ODD;
        } else if ( s_line_coding.parity == 2 ) {
            parity = UART_PARITY_EVEN;
        } else {
            parity = UART_PARITY_NONE;
        }

        uint stop_bits = 1;
        if ( s_line_coding.stop_bits == 2 ) {
            stop_bits = 2;
        } else if ( s_line_coding.stop_bits == 1 ) {
            // 1.5 not supported; approximate with 2
            stop_bits = 2;
        } else {
            stop_bits = 1;
        }

        uart_set_baudrate( ASYNC_PASSTHROUGH_UART, s_line_coding.bit_rate );
        uart_set_format( ASYNC_PASSTHROUGH_UART, data_bits, stop_bits, parity );

        serial1baud = s_line_coding.bit_rate;
        set_micros_per_byte( &s_line_coding );
        s_apply_line_coding_pending = false;
    }


    // USB -> UART when either pending flag set or data available
    if ( s_usb_rx_pending || ( tud_inited( ) && tud_cdc_n_available( ASYNC_PASSTHROUGH_CDC_ITF ) ) ) {
        bridge_usb_to_uart( ASYNC_PASSTHROUGH_CDC_ITF );
        s_usb_rx_pending = false;
    }
    // UART -> USB
    bridge_uart_to_usb( ASYNC_PASSTHROUGH_CDC_ITF );

    // Service USB stack regardless to minimize latency and prevent CDC TX stalling
    tud_task();

}

} // namespace AsyncPassthrough

#endif
