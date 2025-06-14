/*
 * MicroPython port configuration for Jumperless embedding
 */

#include <limits.h>
#include "micropython_embed/port/mpconfigport_common.h"

// Memory allocation
#ifndef PATH_MAX
#define PATH_MAX 1024
#endif
#define MICROPY_ALLOC_PATH_MAX      (PATH_MAX)
#define MICROPY_ENABLE_GC           (1)
#define MICROPY_HELPER_REPL         (1)
#define MICROPY_HELPER_LEXER_UNIX   (1)
#define MICROPY_MEM_STATS           (0)

// REPL configuration
#define MICROPY_REPL_AUTO_INDENT    (1)
#define MICROPY_REPL_EMACS_KEYS     (1)

// Python builtins
#define MICROPY_PY_BUILTINS_COMPILE (1)
#define MICROPY_PY_BUILTINS_EVAL_EXEC (1)
// Float support - set implementation before mpconfig.h processes it
#define MICROPY_FLOAT_IMPL          (MICROPY_FLOAT_IMPL)
// MICROPY_PY_BUILTINS_FLOAT will be automatically set by mpconfig.h based on MICROPY_FLOAT_IMPL
#define MICROPY_PY_SYS_PLATFORM     "embed"
#define MICROPY_PY_SYS_EXIT         (1)
#define MICROPY_PY_SYS_PATH         (1)
#define MICROPY_PY_SYS_PS1_PS2      (1)
#define MICROPY_PY_SYS_STDIO_BUFFER (1)

// Basic modules
#define MICROPY_PY_ARRAY            (1)
#define MICROPY_PY_COLLECTIONS      (1)
#define MICROPY_PY_IO               (1)
#define MICROPY_PY_STRUCT           (1)
#define MICROPY_PY_MATH             (1)
#define MICROPY_PY_GC               (1)

// Optimize for size
#define MICROPY_OPT_COMPUTED_GOTO   (0)
#define MICROPY_MODULE_WEAK_LINKS   (0)

// Enable error reporting features
#define MICROPY_ERROR_REPORTING     (MICROPY_ERROR_REPORTING_DETAILED)
#define MICROPY_ENABLE_SOURCE_LINE  (1)

// Additional features for embedded use
#define MICROPY_PY_MICROPYTHON_MEM_INFO (1)

