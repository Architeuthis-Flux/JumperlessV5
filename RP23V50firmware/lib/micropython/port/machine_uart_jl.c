//#if MICROPY_PY_MACHINE_UART
// rp2 backend for machine.UART for the embedded Jumperless port
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/ringbuf.h"
#include "shared/runtime/mpirq.h"
#include "extmod/modmachine.h"

#include "hardware/irq.h"
#include "hardware/uart.h"
#include "hardware/regs/uart.h"
#include "pico/mutex.h"
#include "pico/time.h"
#include "py/stream.h"
#include <stdbool.h>
// Coordinate with AsyncPassthrough for UART0 ownership (C-only prototypes)
extern void jl_asyncpassthrough_suspend_uart0(void);
extern void jl_asyncpassthrough_resume_uart0(void);
extern void jl_asyncpassthrough_override_line_coding(uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);

#define DEFAULT_UART_BAUDRATE (115200)
#define DEFAULT_UART_BITS (8)
#define DEFAULT_UART_STOP (1)

#ifdef MICROPY_HW_UART_NO_DEFAULT_PINS
#define MICROPY_UART_PINS_ARG_OPTS MP_ARG_REQUIRED
#else
#define MICROPY_UART_PINS_ARG_OPTS 0
#endif

#if !defined(MICROPY_HW_UART0_TX)
#define MICROPY_HW_UART0_TX (0)
#define MICROPY_HW_UART0_RX (1)
#define MICROPY_HW_UART0_CTS (18)
#define MICROPY_HW_UART0_RTS (19)
#endif

#if !defined(MICROPY_HW_UART1_TX)
#define MICROPY_HW_UART1_TX (20)
#define MICROPY_HW_UART1_RX (21)
#define MICROPY_HW_UART1_CTS (22)
#define MICROPY_HW_UART1_RTS (23)
#endif

#define DEFAULT_BUFFER_SIZE  (256)
#define MIN_BUFFER_SIZE      (32)
#define MAX_BUFFER_SIZE      (32766)

#define IS_VALID_PERIPH(uart, pin)  (((((pin) + 4) & 8) >> 3) == (uart))
#if PICO_RP2350
#define IS_VALID_TX(uart, pin)      (((pin) & 1) == 0 && IS_VALID_PERIPH(uart, pin))
#define IS_VALID_RX(uart, pin)      (((pin) & 1) == 1 && IS_VALID_PERIPH(uart, pin))
#else
#define IS_VALID_TX(uart, pin)      (((pin) & 3) == 0 && IS_VALID_PERIPH(uart, pin))
#define IS_VALID_RX(uart, pin)      (((pin) & 3) == 1 && IS_VALID_PERIPH(uart, pin))
#endif
#define IS_VALID_CTS(uart, pin)     (((pin) & 3) == 2 && IS_VALID_PERIPH(uart, pin))
#define IS_VALID_RTS(uart, pin)     (((pin) & 3) == 3 && IS_VALID_PERIPH(uart, pin))

#define UART_INVERT_TX (1)
#define UART_INVERT_RX (2)
#define UART_INVERT_MASK (UART_INVERT_TX | UART_INVERT_RX)

#define UART_HWCONTROL_CTS  (1)
#define UART_HWCONTROL_RTS  (2)
#define MP_UART_ALLOWED_FLAGS (UART_UARTMIS_RTMIS_BITS | UART_UARTMIS_TXMIS_BITS | UART_UARTMIS_BEMIS_BITS)
#define UART_FIFO_SIZE_RX           (32)
#define UART_FIFO_TRIGGER_LEVEL_RX  (24)

static mutex_t write_mutex_0;
static mutex_t write_mutex_1;
static mutex_t read_mutex_0;
static mutex_t read_mutex_1;
auto_init_mutex(write_mutex_0);
auto_init_mutex(write_mutex_1);
auto_init_mutex(read_mutex_0);
auto_init_mutex(read_mutex_1);

typedef struct _machine_uart_obj_t {
    mp_obj_base_t base;
    uart_inst_t *const uart;
    uint8_t uart_id;
    uint32_t baudrate;
    uint8_t bits;
    uart_parity_t parity;
    uint8_t stop;
    uint8_t tx;
    uint8_t rx;
    uint8_t cts;
    uint8_t rts;
    uint16_t timeout;
    uint16_t timeout_char;
    uint8_t invert;
    uint8_t flow;
    uint16_t rxbuf_len;
    ringbuf_t read_buffer;
    mutex_t *read_mutex;
    uint16_t txbuf_len;
    ringbuf_t write_buffer;
    mutex_t *write_mutex;
    uint16_t mp_irq_trigger;
    uint16_t mp_irq_flags;
    mp_irq_obj_t *mp_irq_obj;
} machine_uart_obj_t;

// Static ring buffers to avoid GC root registration.
static uint8_t uart0_rx_buf[DEFAULT_BUFFER_SIZE + 1];
static uint8_t uart0_tx_buf[DEFAULT_BUFFER_SIZE + 1];
static uint8_t uart1_rx_buf[DEFAULT_BUFFER_SIZE + 1];
static uint8_t uart1_tx_buf[DEFAULT_BUFFER_SIZE + 1];

static machine_uart_obj_t machine_uart_obj[] = {
    {{&machine_uart_type}, uart0, 0, 0, DEFAULT_UART_BITS, UART_PARITY_NONE, DEFAULT_UART_STOP,
     MICROPY_HW_UART0_TX, MICROPY_HW_UART0_RX, MICROPY_HW_UART0_CTS, MICROPY_HW_UART0_RTS,
     0, 0, 0, 0, 0, {NULL, 1, 0, 0}, &read_mutex_0, 0, {NULL, 1, 0, 0}, &write_mutex_0, 0, 0, NULL},
    {{&machine_uart_type}, uart1, 1, 0, DEFAULT_UART_BITS, UART_PARITY_NONE, DEFAULT_UART_STOP,
     MICROPY_HW_UART1_TX, MICROPY_HW_UART1_RX, MICROPY_HW_UART1_CTS, MICROPY_HW_UART1_RTS,
     0, 0, 0, 0, 0, {NULL, 1, 0, 0}, &read_mutex_1, 0, {NULL, 1, 0, 0}, &write_mutex_1, 0, 0, NULL},
};

static const char *_parity_name[] = {"None", "0", "1"};
static const char *_invert_name[] = {"None", "INV_TX", "INV_RX", "INV_TX|INV_RX"};

/******************************************************************************/
// IRQ and buffer handling

static inline bool write_mutex_try_lock(machine_uart_obj_t *u) {
    return mutex_enter_timeout_ms(u->write_mutex, 0);
}

static inline void write_mutex_unlock(machine_uart_obj_t *u) {
    mutex_exit(u->write_mutex);
}

static inline bool read_mutex_try_lock(machine_uart_obj_t *u) {
    return mutex_enter_timeout_ms(u->read_mutex, 0);
}

static inline void read_mutex_unlock(machine_uart_obj_t *u) {
    mutex_exit(u->read_mutex);
}

// take at most max_items bytes from the fifo and store them in the buffer
static void uart_drain_rx_fifo(machine_uart_obj_t *self, uint32_t max_items) {
    if (read_mutex_try_lock(self)) {
        while (uart_is_readable(self->uart) && ringbuf_free(&self->read_buffer) > 0 && max_items > 0) {
            uint16_t c = uart_get_hw(self->uart)->dr;
            max_items -= 1;
            if (c & UART_UARTDR_BE_BITS) {
                continue;
            }
            ringbuf_put(&(self->read_buffer), c);
        }
        read_mutex_unlock(self);
    }
}

// take bytes from the buffer and put them into the UART FIFO
static void uart_fill_tx_fifo(machine_uart_obj_t *self) {
    if (write_mutex_try_lock(self)) {
        while (uart_is_writable(self->uart) && ringbuf_avail(&self->write_buffer) > 0) {
            uart_get_hw(self->uart)->dr = ringbuf_get(&(self->write_buffer));
        }
        write_mutex_unlock(self);
    }
}

static inline void uart_service_interrupt(machine_uart_obj_t *self) {
    if (uart_get_hw(self->uart)->mis & (UART_UARTMIS_RXMIS_BITS | UART_UARTMIS_RTMIS_BITS)) {
        uart_get_hw(self->uart)->icr = UART_UARTICR_BITS & ~(UART_UARTICR_TXIC_BITS | UART_UARTICR_BEIC_BITS);
        uart_drain_rx_fifo(self, UART_FIFO_TRIGGER_LEVEL_RX - 1);
    }
    if (uart_get_hw(self->uart)->mis & UART_UARTMIS_TXMIS_BITS) {
        uart_get_hw(self->uart)->icr = UART_UARTICR_BITS & ~(UART_UARTICR_RXIC_BITS | UART_UARTICR_RTIC_BITS | UART_UARTICR_BEIC_BITS);
        if (ringbuf_avail(&self->write_buffer) > 0) {
            uart_fill_tx_fifo(self);
        }
    }
    if (uart_get_hw(self->uart)->mis & UART_UARTMIS_BEMIS_BITS) {
        hw_set_bits(&uart_get_hw(self->uart)->icr, UART_UARTICR_BEIC_BITS);
    }
}

static void uart0_irq_handler(void) {
    uart_service_interrupt(&machine_uart_obj[0]);
}

static void uart1_irq_handler(void) {
    uart_service_interrupt(&machine_uart_obj[1]);
}

/******************************************************************************/
// MicroPython bindings for UART

static void mp_machine_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, parity=%s, stop=%u, tx=%d, rx=%d, txbuf=%d, rxbuf=%d, timeout=%u, timeout_char=%u, invert=%s)",
        self->uart_id, self->baudrate, self->bits, _parity_name[self->parity],
        self->stop, self->tx, self->rx, self->txbuf_len, self->rxbuf_len,
        self->timeout, self->timeout_char, _invert_name[self->invert]);
}

static void mp_machine_uart_init_helper(machine_uart_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // Minimal, qstr-lite parsing: only positional args supported to avoid adding qstrs.
    // Usage: UART(id[, baudrate])
    bool explicit_baud = false;
    if (n_args >= 1 && pos_args != NULL) {
        // First positional arg is always baudrate for both make_new and init
        mp_int_t br = mp_obj_get_int(pos_args[0]);
        if (br > 0) {
            self->baudrate = (uint32_t)br;
            explicit_baud = true;
        }
    }

    if (n_args > 0 || kw_args->used > 0 || self->baudrate == 0) {
        if (self->baudrate == 0) {
            self->baudrate = DEFAULT_UART_BAUDRATE;
            self->timeout_char = 0;
        }
        uint32_t min_timeout_char = 13000 / self->baudrate + 1;
        if (self->timeout_char < min_timeout_char) {
            self->timeout_char = min_timeout_char;
        }

        bool was_enabled = uart_is_enabled(self->uart);
        if (!was_enabled) {
            // If taking over UART0, suspend AsyncPassthrough first (preempt mode)
            // if (self->uart_id == 0) {
            //     jl_asyncpassthrough_suspend_uart0();
            // }
            uart_init(self->uart, self->baudrate);
            uart_set_format(self->uart, self->bits, self->stop, self->parity);
            __dsb();
            uart_set_fifo_enabled(self->uart, true);
            __dsb();
            gpio_set_function(self->tx, UART_FUNCSEL_NUM(self->uart, self->tx));
            gpio_set_function(self->rx, UART_FUNCSEL_NUM(self->uart, self->rx));
            if (self->invert & UART_INVERT_RX) {
                gpio_set_inover(self->rx, GPIO_OVERRIDE_INVERT);
            }
            if (self->invert & UART_INVERT_TX) {
                gpio_set_outover(self->tx, GPIO_OVERRIDE_INVERT);
            }
            if (self->flow & UART_HWCONTROL_CTS) {
                gpio_set_function(self->cts, GPIO_FUNC_UART);
            }
            if (self->flow & UART_HWCONTROL_RTS) {
                gpio_set_function(self->rts, GPIO_FUNC_UART);
            }
            uart_set_hw_flow(self->uart, self->flow & UART_HWCONTROL_CTS, self->flow & UART_HWCONTROL_RTS);
        } else {
            // Already configured by passthrough; apply only explicitly-provided settings
            bool override_baud = explicit_baud;
            if (override_baud) {
                uart_set_baudrate(self->uart, self->baudrate);
            }
            // Only update format if we ever add explicit parsing for it
            if (self->uart_id == 0 && override_baud) {
                jl_asyncpassthrough_override_line_coding(self->baudrate, self->bits, (uint8_t)self->parity, self->stop);
            }
        }

        // // Always enforce requested baudrate and sync passthrough regardless of prior state
        // uart_set_baudrate(self->uart, self->baudrate);
        // if (self->uart_id == 0) {
        //     jl_asyncpassthrough_override_line_coding(self->baudrate, self->bits, (uint8_t)self->parity, self->stop);
        // }

        if (self->read_buffer.buf == NULL) {
            if (self->uart_id == 0) {
                self->read_buffer.buf = uart0_rx_buf;
                self->read_buffer.size = sizeof(uart0_rx_buf);
            } else {
                self->read_buffer.buf = uart1_rx_buf;
                self->read_buffer.size = sizeof(uart1_rx_buf);
            }
            self->read_buffer.iget = 0;
            self->read_buffer.iput = 0;
        }
        if (self->write_buffer.buf == NULL) {
            if (self->uart_id == 0) {
                self->write_buffer.buf = uart0_tx_buf;
                self->write_buffer.size = sizeof(uart0_tx_buf);
            } else {
                self->write_buffer.buf = uart1_tx_buf;
                self->write_buffer.size = sizeof(uart1_tx_buf);
            }
            self->write_buffer.iget = 0;
            self->write_buffer.iput = 0;
        }

        // Only attach IRQs when we initialized the UART
        if (!was_enabled) {
            if (self->uart_id == 0) {
                // UART0 integrates with AsyncPassthrough: use shared handler
                irq_add_shared_handler(UART0_IRQ, uart0_irq_handler, 1);
                irq_set_enabled(UART0_IRQ, true);
            } else {
                // UART1 uses standard rp2 behavior: exclusive handler
                irq_set_exclusive_handler(UART1_IRQ, uart1_irq_handler);
                irq_set_enabled(UART1_IRQ, true);
            }
            uart_set_irq_enables(self->uart, true, true);
        }
    }
}

static mp_obj_t mp_machine_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);
    int uart_id = mp_obj_get_int(args[0]);
    if (uart_id < 0 || uart_id >= MP_ARRAY_SIZE(machine_uart_obj)) {
        mp_raise_msg_varg(&mp_type_ValueError, MP_ERROR_TEXT("UART(%d) doesn't exist"), uart_id);
    }
    machine_uart_obj_t *self = (machine_uart_obj_t *)&machine_uart_obj[uart_id];
    self->rxbuf_len = DEFAULT_BUFFER_SIZE;
    self->read_buffer.buf = NULL;
    self->txbuf_len = DEFAULT_BUFFER_SIZE;
    self->write_buffer.buf = NULL;
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, 0, NULL);
    mp_machine_uart_init_helper(self, n_args - 1, args + 1, &kw_args);
    return MP_OBJ_FROM_PTR(self);
}

static void mp_machine_uart_deinit(machine_uart_obj_t *self) {
    uart_tx_wait_blocking(self->uart);
    bool was_enabled = uart_is_enabled(self->uart);
    if (!was_enabled) {
        uart_deinit(self->uart);
    }
    if (self->uart_id == 0) {
        if (!was_enabled) {
            irq_set_enabled(UART0_IRQ, false);
            // Remove our shared handler so exclusive handlers can be restored
            irq_remove_handler(UART0_IRQ, uart0_irq_handler);
            // Release UART0 back to AsyncPassthrough
            jl_asyncpassthrough_resume_uart0();
        }
    } else {
        if (!was_enabled) {
            irq_set_enabled(UART1_IRQ, false);
            irq_remove_handler(UART1_IRQ, uart1_irq_handler);
        }
    }
    self->baudrate = 0;
    // Static buffers persist; nothing to clear in GC state.
}

static mp_obj_t machine_uart_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    mp_machine_uart_init_helper(MP_OBJ_TO_PTR(args[0]), n_args - 1, args + 1, kw_args);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_uart_init_obj, 1, machine_uart_init);

static mp_obj_t machine_uart_deinit_obj_fun(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_machine_uart_deinit(self);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_deinit_obj, machine_uart_deinit_obj_fun);

static mp_int_t mp_machine_uart_any(machine_uart_obj_t *self) {
    uart_drain_rx_fifo(self, UART_FIFO_SIZE_RX + 1);
    return ringbuf_avail(&self->read_buffer);
}

static mp_obj_t machine_uart_any_obj_fun(mp_obj_t self_in) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    return MP_OBJ_NEW_SMALL_INT(mp_machine_uart_any(self));
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_uart_any_obj, machine_uart_any_obj_fun);

static mp_uint_t mp_machine_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t start = mp_hal_ticks_ms();
    mp_uint_t timeout = self->timeout;
    uint8_t *dest = buf_in;
    for (size_t i = 0; i < size; i++) {
        while (ringbuf_avail(&self->read_buffer) == 0) {
            if (uart_is_readable(self->uart)) {
                uart_drain_rx_fifo(self, UART_FIFO_SIZE_RX + 1);
                break;
            }
            mp_uint_t elapsed = mp_hal_ticks_ms() - start;
            if (elapsed > timeout) {
                if (i <= 0) {
                    *errcode = MP_EAGAIN;
                    return MP_STREAM_ERROR;
                } else {
                    return i;
                }
            }
            mp_event_handle_nowait();
        }
        *dest++ = ringbuf_get(&(self->read_buffer));
        start = mp_hal_ticks_ms();
        timeout = self->timeout_char;
    }
    return size;
}

static mp_uint_t mp_machine_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    machine_uart_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_uint_t start = mp_hal_ticks_ms();
    mp_uint_t timeout = self->timeout;
    const uint8_t *src = buf_in;
    size_t i = 0;
    while (i < size && ringbuf_free(&(self->write_buffer)) > 0) {
        ringbuf_put(&(self->write_buffer), *src++);
        ++i;
    }
    uart_fill_tx_fifo(self);
    while (i < size) {
        while (ringbuf_free(&(self->write_buffer)) == 0) {
            mp_uint_t elapsed = mp_hal_ticks_ms() - start;
            if (elapsed > timeout) {
                if (i <= 0) {
                    *errcode = MP_EAGAIN;
                    return MP_STREAM_ERROR;
                } else {
                    return i;
                }
            }
            mp_event_wait_ms(timeout - elapsed);
        }
        ringbuf_put(&(self->write_buffer), *src++);
        ++i;
        start = mp_hal_ticks_ms();
        timeout = self->timeout_char;
        uart_fill_tx_fifo(self);
    }
    return size;
}

static bool mp_machine_uart_txdone(machine_uart_obj_t *self) {
    return ringbuf_avail(&self->write_buffer) == 0
           && (uart_get_hw(self->uart)->fr & (UART_UARTFR_TXFE_BITS | UART_UARTFR_BUSY_BITS)) == UART_UARTFR_TXFE_BITS;
}

static mp_uint_t mp_machine_uart_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    machine_uart_obj_t *self = self_in;
    if (request == MP_STREAM_POLL) {
        uintptr_t flags = arg;
        mp_uint_t ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && (uart_is_readable(self->uart) || ringbuf_avail(&self->read_buffer) > 0)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && ringbuf_free(&self->write_buffer) > 0) {
            ret |= MP_STREAM_POLL_WR;
        }
        return ret;
    } else if (request == MP_STREAM_FLUSH) {
        uint64_t timeout = time_us_64() + (uint64_t)(33 + self->write_buffer.size) * 13000000ll * 2 / self->baudrate;
        while (1) {
            if (mp_machine_uart_txdone(self)) {
                return 0;
            }
            uint64_t now = time_us_64();
            if (now >= timeout) {
                break;
            }
            mp_event_handle_nowait();
        }
        *errcode = MP_ETIMEDOUT;
        return MP_STREAM_ERROR;
    } else {
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}

static const mp_stream_p_t uart_stream_p = {
    .read = mp_machine_uart_read,
    .write = mp_machine_uart_write,
    .ioctl = mp_machine_uart_ioctl,
    .is_text = false,
};

static const mp_rom_map_elem_t machine_uart_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&machine_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit), MP_ROM_PTR(&machine_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_any), MP_ROM_PTR(&machine_uart_any_obj) },
};
static MP_DEFINE_CONST_DICT(machine_uart_locals_dict, machine_uart_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_uart_type,
    MP_QSTR_UART,
    MP_TYPE_FLAG_ITER_IS_STREAM,
    make_new, mp_machine_uart_make_new,
    print, mp_machine_uart_print,
    protocol, &uart_stream_p,
    locals_dict, &machine_uart_locals_dict
    );

// Export a reference to ensure the type is linked
const mp_obj_type_t *jl_uart_type_export = &machine_uart_type;

// No dynamic root pointers needed; using static buffers.

//#endif // MICROPY_PY_MACHINE_UART


