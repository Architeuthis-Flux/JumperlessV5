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
void jl_gpio_set_direction(int pin, int direction);
void jl_nodes_connect(int node1, int node2, int save);
void jl_nodes_disconnect(int node1, int node2);
void jl_nodes_clear(void);
void jl_oled_print(const char* text, int size);
void jl_oled_clear(void);
void jl_oled_show(void);
int jl_oled_connect(void);
void jl_oled_disconnect(void);
void jl_arduino_reset(void);
void jl_probe_tap(int node);
void jl_clickwheel_up(int clicks);
void jl_clickwheel_down(int clicks);
void jl_clickwheel_press(void);

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
    return mp_obj_new_float(current);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_current_obj, jl_ina_get_current_func);

static mp_obj_t jl_ina_get_voltage_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float voltage = jl_ina_get_voltage(sensor);
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_voltage_obj, jl_ina_get_voltage_func);

static mp_obj_t jl_ina_get_bus_voltage_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float voltage = jl_ina_get_bus_voltage(sensor);
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_ina_get_bus_voltage_obj, jl_ina_get_bus_voltage_func);

static mp_obj_t jl_ina_get_power_func(mp_obj_t sensor_obj) {
    int sensor = mp_obj_get_int(sensor_obj);
    
    if (sensor < 0 || sensor > 1) {
        mp_raise_ValueError(MP_ERROR_TEXT("INA sensor must be 0 or 1"));
    }
    
    float power = jl_ina_get_power(sensor);
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

static mp_obj_t jl_gpio_get_func(mp_obj_t pin_obj) {
    int pin = mp_obj_get_int(pin_obj);
    
    if (pin < 1 || pin > 10) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10"));
    }
    
    int value = jl_gpio_get(pin);
    return mp_obj_new_bool(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_obj, jl_gpio_get_func);

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
    return mp_obj_new_bool(result);
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
    
    // Node functions
    { MP_ROM_QSTR(MP_QSTR_nodes_connect), MP_ROM_PTR(&jl_nodes_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_disconnect), MP_ROM_PTR(&jl_nodes_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_clear), MP_ROM_PTR(&jl_nodes_clear_obj) },
    
    // OLED functions
    { MP_ROM_QSTR(MP_QSTR_oled_print), MP_ROM_PTR(&jl_oled_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_clear), MP_ROM_PTR(&jl_oled_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_show), MP_ROM_PTR(&jl_oled_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_connect), MP_ROM_PTR(&jl_oled_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_disconnect), MP_ROM_PTR(&jl_oled_disconnect_obj) },
    
    // Arduino functions
    { MP_ROM_QSTR(MP_QSTR_arduino_reset), MP_ROM_PTR(&jl_arduino_reset_obj) },
    
    // Probe functions
    { MP_ROM_QSTR(MP_QSTR_probe_tap), MP_ROM_PTR(&jl_probe_tap_obj) },
    
    // Clickwheel functions
    { MP_ROM_QSTR(MP_QSTR_clickwheel_up), MP_ROM_PTR(&jl_clickwheel_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_down), MP_ROM_PTR(&jl_clickwheel_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_press), MP_ROM_PTR(&jl_clickwheel_press_obj) },
};

static MP_DEFINE_CONST_DICT(jumperless_module_globals, jumperless_module_globals_table);

const mp_obj_module_t jumperless_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&jumperless_module_globals,
};

// Register the module with MicroPython
MP_REGISTER_MODULE(MP_QSTR_jumperless, jumperless_user_cmodule); 