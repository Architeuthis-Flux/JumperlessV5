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
// #include "py/mphal.h"

#include "py/runtime.h"
#include "py/lexer.h"
#include "py/builtin.h"
#include "py/mperrno.h"
// Send string of given length to stdout, converting \n to \r\n.
// void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
//     printf("%.*s", (int)len, str);
// }



// Import stat function - return "not found" for all imports
mp_import_stat_t mp_import_stat(const char *path) {
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

// Lexer function - return NULL since we don't support file-based lexing
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    (void)filename;
    // Could also do: mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("File access not supported"));
    return NULL;
}

// Stub implementation for embedded use
static mp_obj_t mp_builtin_open_stub(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    // For embedded use, we don't support file operations
    mp_raise_msg(&mp_type_OSError, MP_ERROR_TEXT("File operations not supported in embedded mode"));
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open_stub);

