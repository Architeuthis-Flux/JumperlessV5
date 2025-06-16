#ifndef MICROPY_INCLUDED_MICROPYTHON_EMBED_H
#define MICROPY_INCLUDED_MICROPYTHON_EMBED_H

// Main header for MicroPython embedding in Jumperless
// This file provides the interface for integrating MicroPython into C/C++ applications

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "py/mpthread.h"
#include "genhdr/mpversion.h"

#ifdef __cplusplus
extern "C" {
#endif

// MicroPython embedding functions

// Initialize the MicroPython runtime
int mp_embed_init(void *heap, size_t heap_size, void *stack_top);

// Deinitialize the MicroPython runtime
void mp_embed_deinit(void);

// Execute a Python string and return the result
int mp_embed_exec_str(const char *str);

// Execute a Python file
int mp_embed_exec_file(const char *filename);

// REPL functionality
void mp_embed_repl(void);

// Get/set global variables from/to MicroPython
mp_obj_t mp_embed_get_global(const char *name);
void mp_embed_set_global(const char *name, mp_obj_t obj);

// Convert between C types and MicroPython objects
mp_obj_t mp_embed_obj_from_int(int val);
mp_obj_t mp_embed_obj_from_float(float val);
mp_obj_t mp_embed_obj_from_str(const char *str);
mp_obj_t mp_embed_obj_from_bool(bool val);

int mp_embed_obj_to_int(mp_obj_t obj);
float mp_embed_obj_to_float(mp_obj_t obj);
const char* mp_embed_obj_to_str(mp_obj_t obj);
bool mp_embed_obj_to_bool(mp_obj_t obj);

// Error handling
const char* mp_embed_get_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // MICROPY_INCLUDED_MICROPYTHON_EMBED_H 