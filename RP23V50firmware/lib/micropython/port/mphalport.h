#ifndef MICROPY_INCLUDED_MPHALPORT_H
#define MICROPY_INCLUDED_MPHALPORT_H

#include <stdint.h>
#include <stddef.h>

// Basic MicroPython types
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;

// Define so there's no dependency on extmod/virtpin.h
#define mp_hal_pin_obj_t

// HAL function declarations needed by MicroPython - match core signatures
mp_uint_t mp_hal_set_interrupt_char(int c);
void mp_hal_stdout_tx_str(const char *str);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len);
int mp_hal_stdin_rx_chr(void);
void mp_hal_delay_ms(mp_uint_t ms);
void mp_hal_delay_us(mp_uint_t us);
mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);

#endif // MICROPY_INCLUDED_MPHALPORT_H
