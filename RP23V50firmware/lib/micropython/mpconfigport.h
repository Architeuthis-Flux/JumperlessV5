/* This file is part of the MicroPython project, http://micropython.org/
 * The MIT License (MIT)
 * Copyright (c) 2022-2023 Damien P. George
 */

// Include common MicroPython embed configuration.
#include "micropython_embed/port/mpconfigport_common.h"

// Use BASIC features level to get floating point support with proper QSTR generation
#define MICROPY_CONFIG_ROM_LEVEL                (MICROPY_CONFIG_ROM_LEVEL_BASIC_FEATURES)

// MicroPython configuration.
#define MICROPY_ENABLE_COMPILER                 (1)
#define MICROPY_ENABLE_GC                       (1)
#define MICROPY_PY_GC                           (1)

// Enable floating point support for RP2350 FPU
// This will be properly supported with BASIC_FEATURES level
#define MICROPY_FLOAT_IMPL                      (MICROPY_FLOAT_IMPL_FLOAT)

// Enable commonly needed features for embedded use
#define MICROPY_PY_IO                           (1)
#define MICROPY_PY_IO_BASE                      (1)
#define MICROPY_PY_SYS                          (1)
#define MICROPY_BANNER_MACHINE                  "Jumperless embedded"

// Optimize for embedded system constraints
#define MICROPY_ALLOC_PATH_MAX                  (256)
#define MICROPY_ENABLE_EXTERNAL_IMPORT          (1)
#define MICROPY_READER_VFS                      (0)
#define MICROPY_VFS                             (0)
