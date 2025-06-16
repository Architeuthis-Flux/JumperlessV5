/*
 * This file is part of the Jumperless project
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Kevin Santo Cappuccio
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
 */

#include "py/obj.h"
#include "py/runtime.h"
#include "py/builtin.h"
#include <string.h>
#include <stdio.h>

// Note: GPIO functions now always return formatted strings like HIGH/LOW, INPUT/OUTPUT, etc.
// Voltage/current functions still return floats for backward compatibility

// Forward declarations for C functions - these will be implemented in the main Jumperless code
void jl_dac_set(int channel, float voltage, int save);
float jl_dac_get(int channel);
float jl_adc_get(int channel);
float jl_ina_get_current(int sensor);
float jl_ina_get_voltage(int sensor);
float jl_ina_get_bus_voltage(int sensor);
float jl_ina_get_power(int sensor);
void jl_gpio_set(int pin, int value);
int jl_gpio_get(int pin);
void jl_gpio_set_dir(int pin, int direction);
int jl_gpio_get_dir(int pin);
void jl_gpio_set_pull(int pin, int pull);
int jl_gpio_get_pull(int pin);
int jl_nodes_connect(int node1, int node2, int save);
int jl_nodes_disconnect(int node1, int node2);
int jl_nodes_is_connected(int node1, int node2);
int jl_nodes_print_bridges(void);
int jl_nodes_print_paths(void);
int jl_nodes_print_crossbars(void);
int jl_nodes_print_nets(void);
int jl_nodes_print_chip_status(void);
int jl_nodes_clear(void);
int jl_oled_print(const char* text, int size);
int jl_oled_clear(void);
int jl_oled_show(void);
int jl_oled_connect(void);
int jl_oled_disconnect(void);
void jl_arduino_reset(void);
void jl_probe_tap(int node);
void jl_clickwheel_up(int clicks);
void jl_clickwheel_down(int clicks);
void jl_clickwheel_press(void);
void jl_run_app(char* appName);
void jl_help(void);

//=============================================================================
// Custom Boolean-like Types for Jumperless
//=============================================================================

// Forward declarations for custom types
const mp_obj_type_t gpio_state_type;
const mp_obj_type_t gpio_direction_type;
const mp_obj_type_t gpio_pull_type;
const mp_obj_type_t connection_state_type;

// Forward declarations for functions
static void gpio_state_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t gpio_state_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t gpio_state_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void gpio_direction_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t gpio_direction_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t gpio_direction_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void gpio_pull_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t gpio_pull_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t gpio_pull_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void connection_state_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t connection_state_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t connection_state_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

// GPIO State Type (HIGH/LOW) that behaves like bool in conditionals
typedef struct _gpio_state_obj_t {
    mp_obj_base_t base;
    bool value;
} gpio_state_obj_t;

static void gpio_state_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    gpio_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "%s", self->value ? "HIGH" : "LOW");
}

static mp_obj_t gpio_state_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    gpio_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(self->value);
        default:
            return MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    gpio_state_type,
    MP_QSTR_GPIOState,
    MP_TYPE_FLAG_NONE,
    make_new, gpio_state_make_new,
    print, gpio_state_print,
    unary_op, gpio_state_unary_op
);

static mp_obj_t gpio_state_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    gpio_state_obj_t *o = m_new_obj(gpio_state_obj_t);
    o->base.type = &gpio_state_type;
    o->value = mp_obj_is_true(args[0]);
    return MP_OBJ_FROM_PTR(o);
}

// These make_new functions will be implemented after the struct definitions later in the file

static mp_obj_t gpio_state_new(bool value) {
    gpio_state_obj_t *o = m_new_obj(gpio_state_obj_t);
    o->base.type = &gpio_state_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

// GPIO Direction Type (INPUT/OUTPUT)
typedef struct _gpio_direction_obj_t {
    mp_obj_base_t base;
    bool value;  // true = OUTPUT, false = INPUT
} gpio_direction_obj_t;

static void gpio_direction_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    gpio_direction_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "%s", self->value ? "OUTPUT" : "INPUT");
}

static mp_obj_t gpio_direction_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    gpio_direction_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(self->value);
        default:
            return MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    gpio_direction_type,
    MP_QSTR_GPIODirection,
    MP_TYPE_FLAG_NONE,
    make_new, gpio_direction_make_new,
    print, gpio_direction_print,
    unary_op, gpio_direction_unary_op
);

static mp_obj_t gpio_direction_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    gpio_direction_obj_t *o = m_new_obj(gpio_direction_obj_t);
    o->base.type = &gpio_direction_type;
    o->value = mp_obj_is_true(args[0]);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t gpio_direction_new(bool value) {
    gpio_direction_obj_t *o = m_new_obj(gpio_direction_obj_t);
    o->base.type = &gpio_direction_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

// GPIO Pull Type (PULLUP/PULLDOWN/NONE)
typedef struct _gpio_pull_obj_t {
    mp_obj_base_t base;
    int value;  // 1 = PULLUP, -1 = PULLDOWN, 0 = NONE
} gpio_pull_obj_t;

static void gpio_pull_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    gpio_pull_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->value == 1) {
        mp_printf(print, "PULLUP");
    } else if (self->value == -1) {
        mp_printf(print, "PULLDOWN");
    } else {
        mp_printf(print, "NONE");
    }
}

static mp_obj_t gpio_pull_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    gpio_pull_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            // Only PULLUP is "truthy"
            return mp_obj_new_bool(self->value == 1);
        default:
            return MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    gpio_pull_type,
    MP_QSTR_GPIOPull,
    MP_TYPE_FLAG_NONE,
    make_new, gpio_pull_make_new,
    print, gpio_pull_print,
    unary_op, gpio_pull_unary_op
);

static mp_obj_t gpio_pull_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    gpio_pull_obj_t *o = m_new_obj(gpio_pull_obj_t);
    o->base.type = &gpio_pull_type;
    o->value = mp_obj_get_int(args[0]);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t gpio_pull_new(int value) {
    gpio_pull_obj_t *o = m_new_obj(gpio_pull_obj_t);
    o->base.type = &gpio_pull_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

// Connection State Type (CONNECTED/DISCONNECTED)
typedef struct _connection_state_obj_t {
    mp_obj_base_t base;
    bool value;
} connection_state_obj_t;

static void connection_state_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    connection_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    mp_printf(print, "%s", self->value ? "CONNECTED" : "DISCONNECTED");
}

static mp_obj_t connection_state_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    connection_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(self->value);
        default:
            return MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    connection_state_type,
    MP_QSTR_ConnectionState,
    MP_TYPE_FLAG_NONE,
    make_new, connection_state_make_new,
    print, connection_state_print,
    unary_op, connection_state_unary_op
);

static mp_obj_t connection_state_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    connection_state_obj_t *o = m_new_obj(connection_state_obj_t);
    o->base.type = &connection_state_type;
    o->value = mp_obj_is_true(args[0]);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t connection_state_new(bool value) {
    connection_state_obj_t *o = m_new_obj(connection_state_obj_t);
    o->base.type = &connection_state_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

//=============================================================================
// Function Implementations
//=============================================================================

// DAC Functions
static mp_obj_t jl_dac_set_func(size_t n_args, const mp_obj_t *args) {
    int channel = mp_obj_get_int(args[0]);
    float voltage = mp_obj_get_float(args[1]);
    int save = (n_args > 2) ? mp_obj_is_true(args[2]) ? 1 : 0 : 1; // Default save=True
    
    if (channel < 0 || channel > 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("DAC channel must be 0-3"));
    }
    
    jl_dac_set(channel, voltage, save);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_dac_set_obj, 2, 3, jl_dac_set_func);

static mp_obj_t jl_dac_get_func(mp_obj_t channel_obj) {
    int channel = mp_obj_get_int(channel_obj);
    
    if (channel < 0 || channel > 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("DAC channel must be 0-3"));
    }
    
    float voltage = jl_dac_get(channel);
    
    // Return voltage as float for backward compatibility
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_dac_get_obj, jl_dac_get_func);

// ADC Functions
static mp_obj_t jl_adc_get_func(mp_obj_t channel_obj) {
    int channel = mp_obj_get_int(channel_obj);
    
    if (channel < 0 || channel > 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("ADC channel must be 0-3"));
    }
    
    float voltage = jl_adc_get(channel);
    
    // Return voltage as float for backward compatibility
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_adc_get_obj, jl_adc_get_func);

// INA Functions
static mp_obj_t jl_ina_get_current_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float current = jl_ina_get_current(sensor);
    
    // Return current as float for backward compatibility
    return mp_obj_new_float(current);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_current_obj, jl_ina_get_current_func);

static mp_obj_t jl_ina_get_voltage_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float voltage = jl_ina_get_voltage(sensor);
    
    // Return voltage as float for backward compatibility
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_voltage_obj, jl_ina_get_voltage_func);

static mp_obj_t jl_ina_get_bus_voltage_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float voltage = jl_ina_get_bus_voltage(sensor);
    
    // Return voltage as float for backward compatibility  
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_bus_voltage_obj, jl_ina_get_bus_voltage_func);

static mp_obj_t jl_ina_get_power_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float power = jl_ina_get_power(sensor);
    
    // Return power as float for backward compatibility
    return mp_obj_new_float(power);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_power_obj, jl_ina_get_power_func);

// GPIO Functions
static mp_obj_t jl_gpio_set_func(mp_obj_t pin_obj, mp_obj_t value_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int value;
    
    if (pin < 1 || pin > 10) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10"));
    }
    
    // Handle both int and bool values
    if (mp_obj_is_bool(value_obj)) {
        value = mp_obj_is_true(value_obj) ? 1 : 0;
    } else {
        value = mp_obj_get_int(value_obj);
        value = value ? 1 : 0; // Normalize to 0 or 1
    }
    
    jl_gpio_set(pin, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_obj, jl_gpio_set_func);

static mp_obj_t jl_gpio_set_dir_func(mp_obj_t pin_obj, mp_obj_t direction_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int direction = mp_obj_get_int(direction_obj);
    
    // if (pin < 1 || pin > 10) {
    //     mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10"));
    // }
    
   // if (direction != 0 && direction != 1) {
        // mp_printf(&mp_plat_print, "direction: %d\n", direction);
       // mp_raise_ValueError(MP_ERROR_TEXT("GPIO direction must be 0 or 1"));
   // }
    jl_gpio_set_dir(pin, direction);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_dir_obj, jl_gpio_set_dir_func);

static mp_obj_t jl_gpio_get_func(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    
    if (pin < 1 || pin > 10) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10"));
    }
    
    int value = jl_gpio_get(pin);
    
    // Return custom GPIO state object that displays as HIGH/LOW but behaves as boolean
    return gpio_state_new(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_obj, jl_gpio_get_func);

static mp_obj_t jl_gpio_get_dir_func(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int direction = jl_gpio_get_dir(pin);
    
    // Return custom GPIO direction object that displays as INPUT/OUTPUT but behaves as boolean
    return gpio_direction_new(direction);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_dir_obj, jl_gpio_get_dir_func);

static mp_obj_t jl_gpio_set_pull_func(mp_obj_t pin_obj, mp_obj_t pull_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int pull = mp_obj_get_int(pull_obj);
    
    jl_gpio_set_pull(pin, pull);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_pull_obj, jl_gpio_set_pull_func);

static mp_obj_t jl_gpio_get_pull_func(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    int pull = jl_gpio_get_pull(pin);
    
    // Return custom GPIO pull object that displays as PULLUP/PULLDOWN/NONE
    return gpio_pull_new(pull);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_pull_obj, jl_gpio_get_pull_func);

// Node Functions
static mp_obj_t jl_nodes_connect_func(size_t n_args, const mp_obj_t *args) {
    int node1 = mp_obj_get_int(args[0]);
    int node2 = mp_obj_get_int(args[1]);
    int save = (n_args > 2) ? mp_obj_is_true(args[2]) ? 1 : 0 : 1; // Default save=True
    
    jl_nodes_connect(node1, node2, save);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_nodes_connect_obj, 2, 3, jl_nodes_connect_func);

static mp_obj_t jl_nodes_disconnect_func(mp_obj_t node1_obj, mp_obj_t node2_obj) {
    int node1 = mp_obj_get_int(node1_obj);
    int node2 = mp_obj_get_int(node2_obj);
    
    jl_nodes_disconnect(node1, node2);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_nodes_disconnect_obj, jl_nodes_disconnect_func);

static mp_obj_t jl_nodes_clear_func(void) {
    jl_nodes_clear();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_clear_obj, jl_nodes_clear_func);


static mp_obj_t jl_nodes_is_connected_func(mp_obj_t node1_obj, mp_obj_t node2_obj) {
    int node1 = mp_obj_get_int(node1_obj);
    int node2 = mp_obj_get_int(node2_obj);
    int connected = jl_nodes_is_connected(node1, node2);
    
    // Return custom connection state object that displays as CONNECTED/DISCONNECTED but behaves as boolean
    return connection_state_new(connected);
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_nodes_is_connected_obj, jl_nodes_is_connected_func);



// OLED Functions
static mp_obj_t jl_oled_print_func(size_t n_args, const mp_obj_t *args) {
    const char* text = mp_obj_str_get_str(args[0]);
    int size = (n_args > 1) ? mp_obj_get_int(args[1]) : 2; // Default size=2
    
    jl_oled_print(text, size);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_oled_print_obj, 1, 2, jl_oled_print_func);

static mp_obj_t jl_oled_clear_func(void) {
    jl_oled_clear();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_oled_clear_obj, jl_oled_clear_func);

static mp_obj_t jl_oled_show_func(void) {
    jl_oled_show();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_oled_show_obj, jl_oled_show_func);

static mp_obj_t jl_oled_connect_func(void) {
    int result = jl_oled_connect();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_oled_connect_obj, jl_oled_connect_func);

static mp_obj_t jl_oled_disconnect_func(void) {
    jl_oled_disconnect();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_oled_disconnect_obj, jl_oled_disconnect_func);

// Arduino Functions
static mp_obj_t jl_arduino_reset_func(void) {
    jl_arduino_reset();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_arduino_reset_obj, jl_arduino_reset_func);


// Status Functions

static mp_obj_t jl_nodes_print_bridges_func(void) {
    jl_nodes_print_bridges();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_print_bridges_obj, jl_nodes_print_bridges_func);

static mp_obj_t jl_nodes_print_paths_func(void) {
    jl_nodes_print_paths();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_print_paths_obj, jl_nodes_print_paths_func);

static mp_obj_t jl_nodes_print_crossbars_func(void) {
    jl_nodes_print_crossbars();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_print_crossbars_obj, jl_nodes_print_crossbars_func);

static mp_obj_t jl_nodes_print_nets_func(void) {
    jl_nodes_print_nets();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_print_nets_obj, jl_nodes_print_nets_func);

static mp_obj_t jl_nodes_print_chip_status_func(void) {
    jl_nodes_print_chip_status();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_print_chip_status_obj, jl_nodes_print_chip_status_func);


static mp_obj_t jl_run_app_func(mp_obj_t appName_obj) {
    const char* appName = mp_obj_str_get_str(appName_obj);
    jl_run_app((char*)appName);  // Cast to remove const qualifier
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_run_app_obj, jl_run_app_func);

// Format output function removed - GPIO functions now always return formatted strings



// Probe Functions
static mp_obj_t jl_probe_tap_func(mp_obj_t node_obj) {
    int node = mp_obj_get_int(node_obj);
    jl_probe_tap(node);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_probe_tap_obj, jl_probe_tap_func);

// Clickwheel Functions
static mp_obj_t jl_clickwheel_up_func(size_t n_args, const mp_obj_t *args) {
    int clicks = (n_args > 0) ? mp_obj_get_int(args[0]) : 1; // Default clicks=1
    jl_clickwheel_up(clicks);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_clickwheel_up_obj, 0, 1, jl_clickwheel_up_func);

static mp_obj_t jl_clickwheel_down_func(size_t n_args, const mp_obj_t *args) {
    int clicks = (n_args > 0) ? mp_obj_get_int(args[0]) : 1; // Default clicks=1
    jl_clickwheel_down(clicks);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_clickwheel_down_obj, 0, 1, jl_clickwheel_down_func);

static mp_obj_t jl_clickwheel_press_func(void) {
    jl_clickwheel_press();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_clickwheel_press_obj, jl_clickwheel_press_func);

// Note: Formatted output is enabled by default
// Functions return formatted strings like "HIGH", "3.300V", "123.4mA", etc.

// Help Function
static mp_obj_t jl_help_func(void) {
    mp_printf(&mp_plat_print, "Jumperless Native MicroPython Module\n");
    mp_printf(&mp_plat_print, "Hardware Control Functions with Formatted Output:\n");
    mp_printf(&mp_plat_print, "(GPIO functions return formatted strings like HIGH/LOW, INPUT/OUTPUT, PULLUP/NONE, CONNECTED/DISCONNECTED)\n\n");
    mp_printf(&mp_plat_print, "DAC (Digital-to-Analog Converter):\n");
    mp_printf(&mp_plat_print, "  jumperless.dac_set(channel, voltage)         - Set DAC output voltage\n");
    mp_printf(&mp_plat_print, "  jumperless.dac_get(channel)                  - Get DAC output voltage\n\n");
    mp_printf(&mp_plat_print, "          channel 0: DAC 0\n");    
    mp_printf(&mp_plat_print, "          channel 1: DAC 1\n\n");   
    mp_printf(&mp_plat_print, "          channel 2: top rail\n");    
    mp_printf(&mp_plat_print, "          channel 3: bottom rail\n");    
 
    mp_printf(&mp_plat_print, "            voltage: -8.0 to 8.0V\n\n");
    mp_printf(&mp_plat_print, "ADC (Analog-to-Digital Converter):\n");
    mp_printf(&mp_plat_print, "  jumperless.adc_get(channel)                  - Read ADC input voltage\n\n");
    mp_printf(&mp_plat_print, "                                              channel: 0-4\n\n");
    mp_printf(&mp_plat_print, "INA (Current/Power Monitor):\n");
    mp_printf(&mp_plat_print, "  jumperless.ina_get_current(sensor)          - Read current in amps\n");
    mp_printf(&mp_plat_print, "  jumperless.ina_get_voltage(sensor)          - Read shunt voltage\n");
    mp_printf(&mp_plat_print, "  jumperless.ina_get_bus_voltage(sensor)      - Read bus voltage\n");
    mp_printf(&mp_plat_print, "  jumperless.ina_get_power(sensor)            - Read power in watts\n\n");
    mp_printf(&mp_plat_print, "             sensor: 0 or 1\n\n");
    mp_printf(&mp_plat_print, "GPIO:\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_set(pin, value)             - Set GPIO pin state\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_get(pin)                    - Read GPIO pin state\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_set_dir(pin, direction)     - Set GPIO pin direction\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_get_dir(pin)                - Get GPIO pin direction\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_set_pull(pin, pull)         - Set GPIO pull-up/down\n");
    mp_printf(&mp_plat_print, "  jumperless.gpio_get_pull(pin)               - Get GPIO pull-up/down\n\n");
    mp_printf(&mp_plat_print, "            pin 1-8: GPIO 1-8\n");
    mp_printf(&mp_plat_print, "            pin   9: UART Tx\n");
    mp_printf(&mp_plat_print, "            pin  10: UART Rx\n");
    mp_printf(&mp_plat_print, "              value: True/False   for HIGH/LOW\n");
    mp_printf(&mp_plat_print, "          direction: True/False   for OUTPUT/INPUT\n");
    mp_printf(&mp_plat_print, "               pull: -1/0/1       for PULL_DOWN/NONE/PULL_UP\n\n");
    mp_printf(&mp_plat_print, "Node Connections:\n");
    mp_printf(&mp_plat_print, "  jumperless.connect(node1, node2)            - Connect two nodes\n");
    mp_printf(&mp_plat_print, "  jumperless.disconnect(node1, node2)         - Disconnect nodes\n");
    mp_printf(&mp_plat_print, "  jumperless.is_connected(node1, node2)       - Check if nodes are connected\n\n");
    mp_printf(&mp_plat_print, "  jumperless.nodes_clear()                    - Clear all connections\n");
    mp_printf(&mp_plat_print, "         set node2 to -1 to disconnect everything connected to node1\n\n");
    mp_printf(&mp_plat_print, "OLED Display:\n");
    mp_printf(&mp_plat_print, "  jumperless.oled_print(\"text\")               - Display text\n");
    mp_printf(&mp_plat_print, "  jumperless.oled_clear()                     - Clear display\n");
    // mp_printf(&mp_plat_print, "  jumperless.oled_show()                      - Update display\n");
    mp_printf(&mp_plat_print, "  jumperless.oled_connect()                   - Connect OLED\n");
    mp_printf(&mp_plat_print, "  jumperless.oled_disconnect()                - Disconnect OLED\n\n");
    // mp_printf(&mp_plat_print, "    size: 1 or 2 (default 2)\n\n");

    mp_printf(&mp_plat_print, "Clickwheel:\n");
    mp_printf(&mp_plat_print, "  jumperless.clickwheel_up([clicks])          - Scroll up\n");
    mp_printf(&mp_plat_print, "  jumperless.clickwheel_down([clicks])        - Scroll down\n");
    mp_printf(&mp_plat_print, "  jumperless.clickwheel_press()               - Press button\n");
    mp_printf(&mp_plat_print, "           clicks: number of steps\n\n");
    mp_printf(&mp_plat_print, "Status:\n");
    mp_printf(&mp_plat_print, "  jumperless.print_bridges()                  - Print all bridges\n");
    mp_printf(&mp_plat_print, "  jumperless.print_paths()                    - Print path between nodes\n");
    mp_printf(&mp_plat_print, "  jumperless.print_crossbars()                - Print crossbar array\n");
    mp_printf(&mp_plat_print, "  jumperless.print_nets()                     - Print nets\n");
    mp_printf(&mp_plat_print, "  jumperless.print_chip_status()              - Print chip status\n\n");

    mp_printf(&mp_plat_print, "Misc:\n");
    mp_printf(&mp_plat_print, "  jumperless.arduino_reset()                  - Reset Arduino\n");
    mp_printf(&mp_plat_print, "  jumperless.probe_tap(node)                  - Tap probe on node (unimplemented)\n");
    mp_printf(&mp_plat_print, "  jumperless.run_app(appName)                 - Run app\n");
    mp_printf(&mp_plat_print, "  jumperless.format_output(True/False)        - Enable/disable formatted output\n\n");
    mp_printf(&mp_plat_print, "Help:\n");
    mp_printf(&mp_plat_print, "  jumperless.help()                           - Display this help\n\n");
    mp_printf(&mp_plat_print, "Examples:\n");
    mp_printf(&mp_plat_print, "  jumperless.dac_set(3, 3.3)                  # Set Top Rail to 3.3V\n");
    mp_printf(&mp_plat_print, "  voltage = jumperless.adc_get(1)             # Read ADC1\n");
    mp_printf(&mp_plat_print, "  jumperless.nodes_connect(4, 20)             # Connect node 4 to 20\n");
    mp_printf(&mp_plat_print, "  jumperless.oled_print(\"Fuck you!\")          # Display text\n");
    mp_printf(&mp_plat_print, "  current = jumperless.ina_get_current(0)     # Read current\n\n");
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_help_obj, jl_help_func);

// Module globals table
static const mp_rom_map_elem_t jumperless_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jumperless) },
    
    // DAC functions
    { MP_ROM_QSTR(MP_QSTR_dac_set), MP_ROM_PTR(&jl_dac_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_dac_get), MP_ROM_PTR(&jl_dac_get_obj) },
    
    // ADC functions
    { MP_ROM_QSTR(MP_QSTR_adc_get), MP_ROM_PTR(&jl_adc_get_obj) },
    
    // INA functions
    { MP_ROM_QSTR(MP_QSTR_ina_get_current), MP_ROM_PTR(&jl_ina_get_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_voltage), MP_ROM_PTR(&jl_ina_get_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_bus_voltage), MP_ROM_PTR(&jl_ina_get_bus_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_power), MP_ROM_PTR(&jl_ina_get_power_obj) },
    
    // GPIO functions
    { MP_ROM_QSTR(MP_QSTR_gpio_set), MP_ROM_PTR(&jl_gpio_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get), MP_ROM_PTR(&jl_gpio_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_set_dir), MP_ROM_PTR(&jl_gpio_set_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get_dir), MP_ROM_PTR(&jl_gpio_get_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_set_pull), MP_ROM_PTR(&jl_gpio_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get_pull), MP_ROM_PTR(&jl_gpio_get_pull_obj) }, 
    
    // Node functions
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&jl_nodes_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&jl_nodes_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_clear), MP_ROM_PTR(&jl_nodes_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_connected), MP_ROM_PTR(&jl_nodes_is_connected_obj) },


    
    // OLED functions
    { MP_ROM_QSTR(MP_QSTR_oled_print), MP_ROM_PTR(&jl_oled_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_clear), MP_ROM_PTR(&jl_oled_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_show), MP_ROM_PTR(&jl_oled_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_connect), MP_ROM_PTR(&jl_oled_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_disconnect), MP_ROM_PTR(&jl_oled_disconnect_obj) },
    
    // Misc functions
    { MP_ROM_QSTR(MP_QSTR_arduino_reset), MP_ROM_PTR(&jl_arduino_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_app), MP_ROM_PTR(&jl_run_app_obj) },

        // Status functions
    { MP_ROM_QSTR(MP_QSTR_print_bridges), MP_ROM_PTR(&jl_nodes_print_bridges_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_paths), MP_ROM_PTR(&jl_nodes_print_paths_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_crossbars), MP_ROM_PTR(&jl_nodes_print_crossbars_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_nets), MP_ROM_PTR(&jl_nodes_print_nets_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_chip_status), MP_ROM_PTR(&jl_nodes_print_chip_status_obj) },
    
    // Probe functions
    { MP_ROM_QSTR(MP_QSTR_probe_tap), MP_ROM_PTR(&jl_probe_tap_obj) },
    
    // Clickwheel functions
    { MP_ROM_QSTR(MP_QSTR_clickwheel_up), MP_ROM_PTR(&jl_clickwheel_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_down), MP_ROM_PTR(&jl_clickwheel_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_press), MP_ROM_PTR(&jl_clickwheel_press_obj) },

    // Help function
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&jl_help_obj) },
};

static MP_DEFINE_CONST_DICT(jumperless_module_globals, jumperless_module_globals_table);

const mp_obj_module_t jumperless_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&jumperless_module_globals,
};

// Register the module with MicroPython
MP_REGISTER_MODULE(MP_QSTR_jumperless, jumperless_user_cmodule); 