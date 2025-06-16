/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2022-2023 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include "py/mphal.h"

#include "py/runtime.h"
#include "py/lexer.h"
#include "py/builtin.h"
#include "py/mperrno.h"

// Import the global stream from our main Arduino code
extern void* global_mp_stream_ptr;
extern void arduino_serial_write(const char *str, int len, void *stream);
extern int arduino_serial_read(void *stream);

// Send string of given length to stdout, converting \n to \r\n.
// Note: Implementation moved to Python.cpp for Jumperless-specific functionality
// void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
//     if (global_mp_stream_ptr) {
//         arduino_serial_write(str, len, global_mp_stream_ptr);
//     }
// }

// Send string of given length to stdout (raw).
mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (global_mp_stream_ptr) {
        arduino_serial_write(str, len, global_mp_stream_ptr);
    }
    return len;
}

mp_uint_t mp_hal_set_interrupt_char(int c) {
    (void)c;
    return 0;
}

// Receive single character from stdin, non-blocking.
int mp_hal_stdin_rx_chr(void) {
    if (global_mp_stream_ptr) {
        return arduino_serial_read(global_mp_stream_ptr);
    }
    // For embedded use, we don't support stdin input
    return -1; // No character available
}

// Import stat function implementation removed - using inline version from builtin.h

// Lexer function - return NULL since we don't support file-based lexing
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    // Could also do: mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("File access not supported"));
    return NULL;
}

// Open function implementation removed - VFS provides this when MICROPY_VFS is enabled

// HAL timing functions - basic implementations for embedded use
// Note: mp_hal_delay_ms and mp_hal_ticks_ms are defined in Python_Proper.cpp
#include <Arduino.h>

void mp_hal_delay_us(mp_uint_t us) {
    delayMicroseconds(us);
}

mp_uint_t mp_hal_ticks_us(void) {
    return micros();
}

mp_uint_t mp_hal_ticks_cpu(void) {
    // For RP2040/RP2350, we can use the same as ticks_us
    // In a real implementation, this might use a higher resolution counter
    return micros();
}

