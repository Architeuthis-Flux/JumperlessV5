#include "py/runtime.h"
#include "py/lexer.h"
#include "py/builtin.h"
#include "py/mperrno.h"


// Port-specific function implementations for embedded systems

mp_import_stat_t mp_import_stat(const char *path) {
    // For embedded systems, we don't support file imports
    (void)path;
    return MP_IMPORT_STAT_NO_EXIST;
}

mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    // For embedded systems, we don't support file-based lexing
    (void)filename;
    mp_raise_OSError(MP_ENOENT);
    return NULL;
}

// Stub for builtin open function - not supported in embedded mode
static mp_obj_t mp_builtin_open_stub(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    (void)n_args;
    (void)args;
    (void)kwargs;
    mp_raise_OSError(MP_ENOENT);
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open_stub);
