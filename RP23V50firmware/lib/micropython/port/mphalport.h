#ifndef MICROPY_INCLUDED_MPHALPORT_H
#define MICROPY_INCLUDED_MPHALPORT_H

#include <stdint.h>
#include <stddef.h>

// Basic MicroPython types
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;

// Forward declarations for C functions - these will be implemented in the main Jumperless code
void jl_gpio_set_dir(int pin, int direction);
void jl_gpio_set(int pin, int value);
void jl_gpio_set_pull(int pin, int pull);
int jl_gpio_get(int pin);
int jl_gpio_get_pull(int pin);




// Timing/stdio HAL (implemented in mphalport.c)
mp_uint_t mp_hal_set_interrupt_char(int c);
void mp_hal_stdout_tx_str(const char *str);
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len);
int mp_hal_stdin_rx_chr(void);
void mp_hal_delay_ms(mp_uint_t ms);
void mp_hal_delay_us(mp_uint_t us);
mp_uint_t mp_hal_ticks_ms(void);
mp_uint_t mp_hal_ticks_us(void);
mp_uint_t mp_hal_ticks_cpu(void);

// C-level pin HAL. Only use Pico SDK under Arduino/PICO builds.
//#ifdef JL_USE_PICO_HAL
#include "py/obj.h"
#include "hardware/gpio.h"

#define MP_HAL_PIN_FMT "%u"
#define mp_hal_pin_obj_t uint32_t

static inline unsigned int mp_hal_pin_name(mp_hal_pin_obj_t pin) { return pin; }
static inline mp_hal_pin_obj_t mp_hal_get_pin_obj(mp_obj_t pin_in) { return (mp_hal_pin_obj_t)mp_obj_get_int(pin_in); }
static inline void mp_hal_pin_input(mp_hal_pin_obj_t pin) { gpio_set_dir(pin, 1); }
static inline void mp_hal_pin_output(mp_hal_pin_obj_t pin) { gpio_set_dir(pin, 0); }
static inline void mp_hal_pin_open_drain_with_value(mp_hal_pin_obj_t pin, int v) { if (v) { gpio_set_dir(pin, 1); gpio_put(pin, 0); } else { gpio_put(pin, 0); gpio_set_dir(pin, 0); } }
static inline void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin) { mp_hal_pin_open_drain_with_value(pin, 1); }
static inline void mp_hal_pin_config(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt) { (void)alt; gpio_set_dir(pin, mode); gpio_set_pulls(pin, pull == 1, pull == 2); }
static inline int mp_hal_pin_read(mp_hal_pin_obj_t pin) { return (int)gpio_get(pin); }
static inline void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v) { gpio_put(pin, v); }
static inline void mp_hal_pin_od_low(mp_hal_pin_obj_t pin) { gpio_set_dir(pin, 0); }
static inline void mp_hal_pin_od_high(mp_hal_pin_obj_t pin) { gpio_set_dir(pin, 1); }
// #else
// // Minimal stub pin HAL for embed/host build so py/mphal.h won't include extmod/virtpin.h
// #include "py/obj.h"
// #define MP_HAL_PIN_FMT "%u"
// #define mp_hal_pin_obj_t uintptr_t
// static inline unsigned int mp_hal_pin_name(mp_hal_pin_obj_t pin) { return (unsigned int)pin; }
// static inline mp_hal_pin_obj_t mp_hal_get_pin_obj(mp_obj_t pin_in) { return (mp_hal_pin_obj_t)(uintptr_t)pin_in; }
// static inline void mp_hal_pin_input(mp_hal_pin_obj_t pin) { (void)pin; }
// static inline void mp_hal_pin_output(mp_hal_pin_obj_t pin) { (void)pin; }
// static inline void mp_hal_pin_open_drain_with_value(mp_hal_pin_obj_t pin, int v) { (void)pin; (void)v; }
// static inline void mp_hal_pin_open_drain(mp_hal_pin_obj_t pin) { (void)pin; }
// static inline void mp_hal_pin_config(mp_hal_pin_obj_t pin, uint32_t mode, uint32_t pull, uint32_t alt) { (void)pin; (void)mode; (void)pull; (void)alt; }
// static inline int mp_hal_pin_read(mp_hal_pin_obj_t pin) { (void)pin; return 0; }
// static inline void mp_hal_pin_write(mp_hal_pin_obj_t pin, int v) { (void)pin; (void)v; }
// static inline void mp_hal_pin_od_low(mp_hal_pin_obj_t pin) { (void)pin; }
// static inline void mp_hal_pin_od_high(mp_hal_pin_obj_t pin) { (void)pin; }
// #endif

#define mp_hal_quiet_timing_enter() (0)
#define mp_hal_quiet_timing_exit(irq_state) (void)(irq_state)

#endif // MICROPY_INCLUDED_MPHALPORT_H
