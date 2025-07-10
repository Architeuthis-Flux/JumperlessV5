/*
 * MicroPython port configuration for Jumperless embedding
 * Based on the embed port with built-in modules enabled
 */

#include <stdint.h>
#include <alloca.h>
#include <stddef.h>

// Basic type definitions
typedef intptr_t mp_int_t;
typedef uintptr_t mp_uint_t;
typedef float mp_float_t;
typedef long mp_off_t;

// Hardware abstraction layer (HAL) types for machine module
typedef uint32_t mp_hal_pin_obj_t;

// Remove conflicting typedefs - let MicroPython define these
// typedef struct _mp_obj_base_t mp_obj_base_t;
// typedef struct _mp_obj_t *mp_obj_t;

// Memory allocation - minimal for microcontroller
#ifndef PATH_MAX
#define PATH_MAX 256
#endif
#define MICROPY_ALLOC_PATH_MAX      (256)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (0)  // Disable to save memory
#define MICROPY_MEM_STATS           (1)  // Disable to save memory
#define MICROPY_KBD_EXCEPTION      (1)

// REPL configuration - basic only
#define MICROPY_REPL_AUTO_INDENT    (1)  // Disable to save memory
#define MICROPY_REPL_EMACS_KEYS     (1)  // Disable to save memory

// Float support - enable single-precision floating point
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_PY_BUILTINS_FLOAT   (1)

// Python builtins - minimal set
#define MICROPY_PY_BUILTINS_COMPILE (0)  // Disable to save memory
#define MICROPY_PY_BUILTINS_EVAL_EXEC (1)
#define MICROPY_PY_BUILTINS_HELP    (1)
#define MICROPY_PY___FILE__         (0)  // Disable to avoid import path issues
#define MICROPY_PY_SYS_PLATFORM     "jumperless-rp2350"
#define MICROPY_PY_SYS_EXIT         (1)
#define MICROPY_PY_SYS_PATH         (1)  
#define MICROPY_PY_SYS_PS1_PS2      (1)  // Enable for REPL
#define MICROPY_PY_SYS_STDIO_BUFFER (1)  
#define MICROPY_PY_SYS_ATTR_DELEGATION (1)  


#define MICROPY_PY_FSTRINGS         (1)


#define MICROPY_STACK_CHECK (1)
#define MICROPY_STACK_CHECK_MARGIN (1024)  // 1KB margin for embedded systems

// Basic modules - minimal set
#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_COLLECTIONS      (1)  // Disable to save memory
#define MICROPY_PY_IO               (1)  // Disable to save memory
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_GC               (1)
#define MICROPY_PY_BINASCII         (0)  // Disable to save memory
#define MICROPY_PY_ERRNO            (1)  // Disable to save memory
#define MICROPY_PY_JSON             (0)
#define MICROPY_PY_RE               (0)
#define MICROPY_PY_HEAPQ            (0)
#define MICROPY_PY_HASHLIB          (0)
#define MICROPY_PY_RANDOM           (0)

// Standard library modules - disable most to save memory
#define MICROPY_PY_TIME             (1)  // Keep disabled to avoid import issues
#define MICROPY_PY_TIME_TIME_TIME_NS (0)
#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME (0)

// OS module - keep disabled to avoid port-specific requirements
#define MICROPY_PY_OS               (1)  // Enable now that we include extmod
#define MICROPY_PY_OS_DUPTERM       (0)
#define MICROPY_PY_OS_DUPTERM_NOTIFY (0)
#define MICROPY_PY_OS_SYNC          (0)
#define MICROPY_PY_OS_UNAME         (1)  // Enable uname function
#define MICROPY_PY_OS_URANDOM       (0)

// Machine module - completely disable
#define MICROPY_PY_MACHINE                      (0)
#define MICROPY_PY_MACHINE_RESET                (0)
#define MICROPY_PY_MACHINE_BARE_METAL_FUNCS     (0)
#define MICROPY_PY_MACHINE_DISABLE_IRQ_ENABLE_IRQ (0)
#define MICROPY_PY_MACHINE_PWM                  (0)
#define MICROPY_PY_MACHINE_SPI                  (0)
#define MICROPY_PY_MACHINE_I2C                  (0)
#define MICROPY_PY_MACHINE_MEMX                 (0)
#define MICROPY_PY_MACHINE_PIN_BASE             (0)
#define MICROPY_PY_MACHINE_SIGNAL               (0)
#define MICROPY_PY_MACHINE_ADC                  (0)
#define MICROPY_PY_MACHINE_BITSTREAM            (0)
#define MICROPY_PY_MACHINE_PULSE                (0)

// Additional useful modules - disable to save memory
#define MICROPY_PY_ONEWIRE          (1)

// Optimize for size but keep features
#define MICROPY_OPT_COMPUTED_GOTO   (0)
#define MICROPY_MODULE_WEAK_LINKS   (1)

// Enable error reporting features
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_ENABLE_SOURCE_LINE  (1)

#define MICROPY_ENABLE_EXTERNAL_IMPORT (1)  // Disable to avoid sys.path dependency
#define MICROPY_MALLOC_USES_ALLOCATED_SIZE (1)

// Additional features for embedded use
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)
#define MICROPY_ENABLE_SCHEDULER    (1)
#define MICROPY_SCHEDULER_DEPTH     (8)

// VFS support needed for extmod modules
#define MICROPY_VFS                 (1)
#define MICROPY_VFS_FAT             (0)  // Disable FAT to save memory
#define MICROPY_VFS_LFS2            (0)  // Disable LFS2 to save memory
#define MICROPY_VFS_POSIX           (0)  // Disable POSIX VFS

// Time module configuration
#define MICROPY_PY_TIME_INCLUDEFILE "shared/timeutils/timeutils.h"

// Platform module for os.uname()
#define MICROPY_PY_PLATFORM         (1)

// User C modules (Jumperless module will be added here)
#define MODULE_JUMPERLESS_ENABLED   (1)

// Force all print output through our HAL functions instead of sys.stdout
#define MICROPY_PY_SYS_STDFILES     (0)

// Board name for sys.platform
#define MICROPY_HW_BOARD_NAME "jumperless-v5"
#define MICROPY_HW_MCU_NAME   "rp2350b"

// Built-in modules - minimal set (most modules disabled to save memory)
// Only the jumperless module will be available via MP_REGISTER_MODULE

// Add built-in modules to the list - empty for maximum compatibility
#define MICROPY_PORT_BUILTIN_MODULES \

// Module weak links for compatibility - empty
#define MICROPY_PORT_BUILTIN_MODULE_WEAK_LINKS \

#define MP_STATE_PORT MP_STATE_VM
