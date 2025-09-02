/*
 * Minimal machine.Pin implementation for Jumperless embed port (RP23xx)
 * Provides basic Pin IN/OUT/OPEN_DRAIN functionality with pull control.
 *
 * Behaviour mirrors the rp2 port where practical, but omits IRQ and
 * board/cpu pin name tables for now. Pins are addressed by integer id.
 */

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"



// No need to include extmod/modmachine.h here

#include "hardware/gpio.h"

// Fallback Pin protocol definitions if extmod/virtpin.h is not packaged
#ifndef MP_PIN_READ
#define MP_PIN_READ   (1)
#define MP_PIN_WRITE  (2)
typedef struct _mp_pin_p_t {
    mp_uint_t (*ioctl)(mp_obj_t obj, mp_uint_t request, uintptr_t arg, int *errcode);
} mp_pin_p_t;
#endif

// Pull flags (match rp2 values used by machine.Pin)
#define JL_GPIO_PULL_UP    (1)
#define JL_GPIO_PULL_DOWN  (2)

// Default number of GPIOs on RP2 family bank0 (GP0..GP29)
#ifndef JL_NUM_GPIOS
#define JL_NUM_GPIOS (30)
#endif

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    uint8_t id;
} machine_pin_obj_t;

// Forward declare the type used in static initialiser below
extern const mp_obj_type_t machine_pin_type;

// Track which pins are configured as OPEN_DRAIN to emulate behaviour
static uint64_t jl_open_drain_mask;

// Forward decls
static mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

// Statically-defined pin objects for GPIO 0..JL_NUM_GPIOS-1
static const machine_pin_obj_t machine_pin_obj_table[JL_NUM_GPIOS] = {
#define PIN_OBJ_ENTRY(n) { .base = { &machine_pin_type }, .id = (uint8_t)(n) }
    PIN_OBJ_ENTRY(0),  PIN_OBJ_ENTRY(1),  PIN_OBJ_ENTRY(2),  PIN_OBJ_ENTRY(3),
    PIN_OBJ_ENTRY(4),  PIN_OBJ_ENTRY(5),  PIN_OBJ_ENTRY(6),  PIN_OBJ_ENTRY(7),
    PIN_OBJ_ENTRY(8),  PIN_OBJ_ENTRY(9),  PIN_OBJ_ENTRY(10), PIN_OBJ_ENTRY(11),
    PIN_OBJ_ENTRY(12), PIN_OBJ_ENTRY(13), PIN_OBJ_ENTRY(14), PIN_OBJ_ENTRY(15),
    PIN_OBJ_ENTRY(16), PIN_OBJ_ENTRY(17), PIN_OBJ_ENTRY(18), PIN_OBJ_ENTRY(19),
    PIN_OBJ_ENTRY(20), PIN_OBJ_ENTRY(21), PIN_OBJ_ENTRY(22), PIN_OBJ_ENTRY(23),
    PIN_OBJ_ENTRY(24), PIN_OBJ_ENTRY(25), PIN_OBJ_ENTRY(26), PIN_OBJ_ENTRY(27),
    PIN_OBJ_ENTRY(28), PIN_OBJ_ENTRY(29)
#undef PIN_OBJ_ENTRY
};

static void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    (void)kind;
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "Pin(%u)", (unsigned)self->id);
}

enum {
    ARG_mode, ARG_pull, ARG_value, ARG_alt
};

static const mp_arg_t jl_allowed_args[] = {
    { MP_QSTR_mode,  MP_ARG_OBJ,                  { .u_rom_obj = MP_ROM_NONE } },
    { MP_QSTR_pull,  MP_ARG_OBJ,                  { .u_rom_obj = MP_ROM_NONE } },
    { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, { .u_rom_obj = MP_ROM_NONE } },
    { MP_QSTR_alt,   MP_ARG_KW_ONLY | MP_ARG_INT, { .u_int = GPIO_FUNC_SIO } },
};

static mp_obj_t machine_pin_obj_init_helper(const machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[MP_ARRAY_SIZE(jl_allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(jl_allowed_args), jl_allowed_args, args);

    // Determine initial value for OUT/OPEN_DRAIN modes
    int initial_value = -1;
    if (args[ARG_value].u_obj != mp_const_none) {
        initial_value = mp_obj_is_true(args[ARG_value].u_obj);
    }

    // Configure mode if provided
    if (args[ARG_mode].u_obj != mp_const_none) {
        mp_int_t mode = mp_obj_get_int(args[ARG_mode].u_obj);
        if (mode == 0 /* MACHINE_PIN_MODE_IN */) {
            gpio_set_dir(self->id, GPIO_IN);
            gpio_set_function(self->id, GPIO_FUNC_SIO);
            jl_open_drain_mask &= ~(1ULL << self->id);
        } else if (mode == 1 /* MACHINE_PIN_MODE_OUT */) {
            // Set initial value before configuring as output
            if (initial_value != -1) {
                gpio_put(self->id, initial_value);
            }
            gpio_set_dir(self->id, GPIO_OUT);
            gpio_set_function(self->id, GPIO_FUNC_SIO); 
            jl_open_drain_mask &= ~(1ULL << self->id);
        } else if (mode == 2 /* MACHINE_PIN_MODE_OPEN_DRAIN */) {
            // Open-drain: high=INPUT, low=OUTPUT driving 0
            int od_high = initial_value == -1 ? 1 : initial_value;
            if (od_high) {
                gpio_set_dir(self->id, GPIO_IN); // float
            } else {
                gpio_put(self->id, 0);
                gpio_set_dir(self->id, GPIO_OUT); // drive low
            }
            gpio_set_function(self->id, GPIO_FUNC_SIO);
            jl_open_drain_mask |= (1ULL << self->id);
        } else if (mode == 3 /* MACHINE_PIN_MODE_ALT */) {
            uint32_t af = (uint32_t)args[ARG_alt].u_int;
            gpio_set_function(self->id, af);
            jl_open_drain_mask &= ~(1ULL << self->id);
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("invalid pin mode"));
        }
    }

    // Configure pull (unconditionally because None means no-pull)
    uint32_t pull_bits = 0;
    if (args[ARG_pull].u_obj != mp_const_none) {
        pull_bits = (uint32_t)mp_obj_get_int(args[ARG_pull].u_obj);
    }
    bool up = pull_bits & JL_GPIO_PULL_UP;
    bool down = pull_bits & JL_GPIO_PULL_DOWN;
    
    gpio_set_pulls(self->id, up, down);

    return mp_const_none;
}

// constructor(id, ...)
static mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // Accept only integers: allow physical 0..29
    if (!mp_obj_is_int(all_args[0])) {
        mp_raise_ValueError(MP_ERROR_TEXT("Pin id must be integer"));
    }
    int wanted_pin = mp_obj_get_int(all_args[0]);
    bool valid = (wanted_pin >= 0 && wanted_pin < JL_NUM_GPIOS);
    if (!valid) {
        mp_raise_ValueError(MP_ERROR_TEXT("invalid pin"));
    }

    const machine_pin_obj_t *self = &machine_pin_obj_table[wanted_pin];

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
        machine_pin_obj_init_helper(self, n_args - 1, all_args + 1, &kw_args);
    }

    return MP_OBJ_FROM_PTR(self);
}

// fast method for getting/setting pin value
static mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    (void)n_kw;
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (n_args == 0) {
        return MP_OBJ_NEW_SMALL_INT(gpio_get(self->id));
    } else {
        bool value = mp_obj_is_true(args[0]);
        if (jl_open_drain_mask & (1ULL << self->id)) {
            // Open-drain: drive low = output, drive high = input (float)
            if (!value) {
                gpio_put(self->id, 0);
                gpio_set_dir(self->id, GPIO_OUT);
            } else {
                gpio_set_dir(self->id, GPIO_IN);
            }
        } else {
            gpio_put(self->id, value);
        }
        return mp_const_none;
    }
}

// pin.init(mode=..., pull=..., *, value=None, alt=GPIO_FUNC_SIO)
static mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
static mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// pin.low()
static mp_obj_t machine_pin_low(mp_obj_t self_in) {
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (jl_open_drain_mask & (1ULL << self->id)) {
        gpio_put(self->id, 0);
        gpio_set_dir(self->id, GPIO_OUT);
    } else {
        gpio_put(self->id, 0);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_low_obj, machine_pin_low);

// pin.high()
static mp_obj_t machine_pin_high(mp_obj_t self_in) {
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (jl_open_drain_mask & (1ULL << self->id)) {
        gpio_set_dir(self->id, GPIO_IN);
    } else {
        gpio_put(self->id, 1);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_high_obj, machine_pin_high);

// pin.toggle()
static mp_obj_t machine_pin_toggle(mp_obj_t self_in) {
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    int v = gpio_get(self->id);
    mp_obj_t args[2] = { self_in, MP_OBJ_NEW_SMALL_INT(!v) };
    return machine_pin_value(2, args);
}
static MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_toggle_obj, machine_pin_toggle);

// Pin protocol support
static mp_uint_t pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    const machine_pin_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (request) {
        case MP_PIN_READ:
            return gpio_get(self->id);
        case MP_PIN_WRITE:
            gpio_put(self->id, arg);
            return 0;
    }
    return (mp_uint_t)-1;
}

static const mp_pin_p_t pin_pin_p = {
    .ioctl = pin_ioctl,
};

// Locals dict
static const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init),   MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),  MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_low),    MP_ROM_PTR(&machine_pin_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_high),   MP_ROM_PTR(&machine_pin_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle), MP_ROM_PTR(&machine_pin_toggle_obj) },

    // class constants (match rp2 values)
    { MP_ROM_QSTR(MP_QSTR_IN),         MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_OUT),        MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_ALT),        MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),    MP_ROM_INT(JL_GPIO_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN),  MP_ROM_INT(JL_GPIO_PULL_DOWN) },
};

static MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    machine_pin_type,
    MP_QSTR_Pin,
    MP_TYPE_FLAG_NONE,
    make_new, mp_pin_make_new,
    print, machine_pin_print,
    call, machine_pin_call,
    protocol, &pin_pin_p,
    locals_dict, &machine_pin_locals_dict
    );


