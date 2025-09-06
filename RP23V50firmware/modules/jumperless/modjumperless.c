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
#include "py/lexer.h"
#include "py/mperrno.h"
#include "py/builtin.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>




// Terminal color functions
extern void jl_change_terminal_color(int color, bool flush);
extern void jl_cycle_term_color(bool reset, float step, bool flush);

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

// Wavegen C wrappers (C linkage)
void jl_wavegen_set_output(int channel);
void jl_wavegen_set_freq(float hz);
void jl_wavegen_set_wave(int wave);
void jl_wavegen_set_amplitude(float vpp);
void jl_wavegen_set_offset(float v);
void jl_wavegen_set_sweep(float start_hz, float end_hz, float seconds);
void jl_wavegen_start(int start);
void jl_wavegen_stop(void);
int jl_wavegen_get_output(void);
float jl_wavegen_get_freq(void);
int jl_wavegen_get_wave(void);
float jl_wavegen_get_amplitude(void);
float jl_wavegen_get_offset(void);
int jl_wavegen_is_running(void);
void jl_wavegen_get_sweep(float *start_hz, float *end_hz, float *seconds);
void jl_gpio_set(int pin, int value);
int jl_gpio_get(int pin);
void jl_gpio_set_dir(int pin, int direction);
int jl_gpio_get_dir(int pin);
void jl_gpio_set_pull(int pin, int pull);
int jl_gpio_get_pull(int pin);
int jl_nodes_connect(int node1, int node2, int save);
int jl_nodes_disconnect(int node1, int node2);
int jl_nodes_is_connected(int node1, int node2);
int jl_nodes_save(int slot);
int jl_nodes_print_bridges(void);
int jl_nodes_print_paths(void);
int jl_nodes_print_crossbars(void);
int jl_nodes_print_nets(void);
int jl_nodes_print_chip_status(void);
void jl_init_micropython_local_copy(void);
void jl_send_raw(int chip, int x, int y, int setOrClear);
void jl_send_raw_str(const char* chip_str, int x, int y, int setOrClear);
int jl_switch_slot(int slot);
void jl_restore_micropython_entry_state(void);
int jl_has_unsaved_changes(void);



// Logic Analyzer Functions
void jl_logic_analyzer_set_analog(int channel, float value);
void jl_logic_analyzer_set_digital(int channel, bool value);
bool jl_logic_analyzer_set_trigger(int trigger_type, int channel, float value);
bool jl_logic_analyzer_capture_single_sample(void);

// Logic Analyzer Functions
bool jl_la_set_trigger(int trigger_type, int channel, float value);
bool jl_la_capture_single_sample(void);
bool jl_la_start_continuous_capture(void);
bool jl_la_stop_capture(void);
bool jl_la_is_capturing(void);
void jl_la_set_sample_rate(uint32_t sample_rate);
void jl_la_set_num_samples(uint32_t num_samples);
void jl_la_enable_channel(int channel_type, int channel, bool enable);
void jl_la_set_control_analog(int channel, float value);
void jl_la_set_control_digital(int channel, bool value);
float jl_la_get_control_analog(int channel);
bool jl_la_get_control_digital(int channel);

void jl_logic_analyzer_enable_channel(int channel);
void jl_logic_analyzer_enable_channel_mask(uint32_t channel_mask);
void jl_logic_analyzer_set_sample_rate(int sample_rate);
void jl_logic_analyzer_set_num_samples(int num_samples);
void jl_logic_analyzer_set_trigger_type(int trigger_type);
void jl_logic_analyzer_set_trigger_channel(int trigger_channel);
void jl_logic_analyzer_set_trigger_value(float trigger_value);


// Filesystem functions - bridge to existing FatFS 
int jl_fs_exists(const char* path);
char* jl_fs_listdir(const char* path);
char* jl_fs_read_file(const char* path);
int jl_fs_write_file(const char* path, const char* content);
char* jl_fs_get_current_dir(void);

// Extended file operations
void* jl_fs_open_file(const char* path, const char* mode);
void jl_fs_close_file(void* file_handle);
int jl_fs_read_bytes(void* file_handle, char* buffer, int size);
int jl_fs_write_bytes(void* file_handle, const char* data, int size);
int jl_fs_seek(void* file_handle, int position, int mode);
int jl_fs_position(void* file_handle);
int jl_fs_size(void* file_handle);
int jl_fs_available(void* file_handle);
char* jl_fs_name(void* file_handle);

// Directory operations
int jl_fs_mkdir(const char* path);
int jl_fs_rmdir(const char* path);
int jl_fs_remove(const char* path);
int jl_fs_rename(const char* pathFrom, const char* pathTo);

// Filesystem info
int jl_fs_total_bytes(void);
int jl_fs_used_bytes(void);
int jl_nodes_clear(void);
int jl_oled_print(const char* text, int size);
int jl_oled_clear(void);

int jl_oled_show(void);
int jl_oled_connect(void);
int jl_oled_disconnect(void);
void jl_arduino_reset(void);
void jl_probe_tap(int node);
int jl_probe_read_blocking(void);
int jl_probe_read_nonblocking(void);
int jl_probe_button_blocking(void);
int jl_probe_button_nonblocking(void);
void jl_clickwheel_up(int clicks);
void jl_clickwheel_down(int clicks);
void jl_clickwheel_press(void);
void jl_run_app(char* appName);
void jl_help(void);
void jl_help_section(const char* section);
void jl_pause_core2(bool pause);
int jl_pwm_setup(int gpio_pin, float frequency, float duty_cycle);
int jl_pwm_set_duty_cycle(int gpio_pin, float duty_cycle);
int jl_pwm_set_frequency(int gpio_pin, float frequency);
int jl_pwm_stop(int gpio_pin);

//=============================================================================
// Custom Boolean-like Types for Jumperless
//=============================================================================

// Forward declarations for custom types
const mp_obj_type_t gpio_state_type;
const mp_obj_type_t gpio_direction_type;
const mp_obj_type_t gpio_pull_type;
const mp_obj_type_t connection_state_type;
const mp_obj_type_t probe_button_type;
const mp_obj_type_t node_type;
const mp_obj_type_t probe_pad_type;

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

// Helper function declarations
static int get_direction_value(mp_obj_t obj);
static int get_pull_value(mp_obj_t obj);
static int get_gpio_state_value(mp_obj_t obj);
static void node_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t node_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t node_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void probe_button_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t probe_button_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t probe_button_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);
static void probe_pad_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind);
static mp_obj_t probe_pad_unary_op(mp_unary_op_t op, mp_obj_t self_in);
static mp_obj_t probe_pad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args);

//=============================================================================
// Node Name Mapping - Comprehensive table of all node names and aliases
//=============================================================================

typedef struct {
    const char* name;
    int value;
} NodeMapping;

// All possible node name mappings including aliases
static const NodeMapping node_mappings[] = {
    // Special functions with all aliases
    {"GND", 100}, {"GROUND", 100},
    {"TOP_RAIL", 101}, {"TOPRAIL", 101}, {"T_R", 101}, {"TOP_R", 101},
    {"BOTTOM_RAIL", 102}, {"BOT_RAIL", 102}, {"BOTTOMRAIL", 102}, {"BOTRAIL", 102}, {"B_R", 102}, {"BOT_R", 102},
    {"SUPPLY_3V3", 103}, {"3V3", 103}, {"3.3V", 103},
    {"TOP_RAIL_GND", 104}, {"TOP_GND", 104},
    {"SUPPLY_5V", 105}, {"5V", 105}, {"+5V", 105},
    
    // DACs
    {"DAC0", 106}, {"DAC_0", 106}, {"DAC0_5V", 106},
    {"DAC1", 107}, {"DAC_1", 107}, {"DAC1_8V", 107},
    
    // Current sense
    {"ISENSE_PLUS", 108}, {"ISENSE_POS", 108}, {"ISENSE_P", 108}, {"INA_P", 108}, {"I_P", 108},
    {"CURRENT_SENSE_PLUS", 108}, {"ISENSE_POSITIVE", 108}, {"I_POS", 108},
    {"ISENSE_MINUS", 109}, {"ISENSE_NEG", 109}, {"ISENSE_N", 109}, {"INA_N", 109}, {"I_N", 109},
    {"CURRENT_SENSE_MINUS", 109}, {"ISENSE_NEGATIVE", 109}, {"I_NEG", 109},
    
    // ADCs
    {"ADC0", 110}, {"ADC_0", 110}, {"ADC0_8V", 110},
    {"ADC1", 111}, {"ADC_1", 111}, {"ADC1_8V", 111},
    {"ADC2", 112}, {"ADC_2", 112}, {"ADC2_8V", 112},
    {"ADC3", 113}, {"ADC_3", 113}, {"ADC3_8V", 113},
    {"ADC4", 114}, {"ADC_4", 114}, {"ADC4_5V", 114},
    {"ADC7", 115}, {"ADC_7", 115}, {"ADC7_PROBE", 115}, {"PROBE", 115},
    
    // UART
    {"RP_UART_TX", 116}, {"UART_TX", 116}, {"TX", 116}, {"RP_GPIO_16", 116},
    {"RP_UART_RX", 117}, {"UART_RX", 117}, {"RX", 117}, {"RP_GPIO_17", 117},
    
    // Other RP GPIOs
    {"RP_GPIO_18", 118}, {"GP_18", 118},
    {"RP_GPIO_19", 119}, {"GP_19", 119},
    
    // Power supplies
    {"SUPPLY_8V_P", 120}, {"8V_P", 120}, {"8V_POS", 120},
    {"SUPPLY_8V_N", 121}, {"8V_N", 121}, {"8V_NEG", 121},
    
    // Ground rails
    {"BOTTOM_RAIL_GND", 126}, {"BOT_GND", 126}, {"BOTTOM_GND", 126},
    {"EMPTY_NET", 127}, {"EMPTY", 127},
    
    // User GPIO pins (with all common aliases)
    {"RP_GPIO_1", 131}, {"GPIO_1", 131}, {"GPIO1", 131}, {"GP_1", 131}, {"GP1", 131},
    {"RP_GPIO_2", 132}, {"GPIO_2", 132}, {"GPIO2", 132}, {"GP_2", 132}, {"GP2", 132},
    {"RP_GPIO_3", 133}, {"GPIO_3", 133}, {"GPIO3", 133}, {"GP_3", 133}, {"GP3", 133},
    {"RP_GPIO_4", 134}, {"GPIO_4", 134}, {"GPIO4", 134}, {"GP_4", 134}, {"GP4", 134},
    {"RP_GPIO_5", 135}, {"GPIO_5", 135}, {"GPIO5", 135}, {"GP_5", 135}, {"GP5", 135},
    {"RP_GPIO_6", 136}, {"GPIO_6", 136}, {"GPIO6", 136}, {"GP_6", 136}, {"GP6", 136},
    {"RP_GPIO_7", 137}, {"GPIO_7", 137}, {"GPIO7", 137}, {"GP_7", 137}, {"GP7", 137},
    {"RP_GPIO_8", 138}, {"GPIO_8", 138}, {"GPIO8", 138}, {"GP_8", 138}, {"GP8", 138},
    
    // Buffer
    {"ROUTABLE_BUFFER_IN", 139}, {"BUFFER_IN", 139}, {"BUF_IN", 139}, {"BUFF_IN", 139}, {"BUFFIN", 139},
    {"ROUTABLE_BUFFER_OUT", 140}, {"BUFFER_OUT", 140}, {"BUF_OUT", 140}, {"BUFF_OUT", 140}, {"BUFFOUT", 140},
    
    // Arduino Nano pins
    {"NANO_VIN", 69}, {"VIN", 69},
    {"NANO_D0", 70}, {"D0", 70},
    {"NANO_D1", 71}, {"D1", 71},
    {"NANO_D2", 72}, {"D2", 72},
    {"NANO_D3", 73}, {"D3", 73},
    {"NANO_D4", 74}, {"D4", 74},
    {"NANO_D5", 75}, {"D5", 75},
    {"NANO_D6", 76}, {"D6", 76},
    {"NANO_D7", 77}, {"D7", 77},
    {"NANO_D8", 78}, {"D8", 78},
    {"NANO_D9", 79}, {"D9", 79},
    {"NANO_D10", 80}, {"D10", 80},
    {"NANO_D11", 81}, {"D11", 81},
    {"NANO_D12", 82}, {"D12", 82},
    {"NANO_D13", 83}, {"D13", 83},
    {"NANO_RESET", 84}, {"RESET", 84},
    {"NANO_AREF", 85}, {"AREF", 85},
    {"NANO_A0", 86}, {"A0", 86},
    {"NANO_A1", 87}, {"A1", 87},
    {"NANO_A2", 88}, {"A2", 88},
    {"NANO_A3", 89}, {"A3", 89},
    {"NANO_A4", 90}, {"A4", 90},
    {"NANO_A5", 91}, {"A5", 91},
    {"NANO_A6", 92}, {"A6", 92},
    {"NANO_A7", 93}, {"A7", 93},
    {"NANO_RESET_0", 94}, {"RST0", 94},
    {"NANO_RESET_1", 95}, {"RST1", 95},
    {"NANO_GND_1", 96}, {"N_GND1", 96},
    {"NANO_GND_0", 97}, {"N_GND0", 97},
    {"NANO_3V3", 98},
    {"NANO_5V", 99},
};

static const size_t node_mappings_count = sizeof(node_mappings) / sizeof(NodeMapping);

//=============================================================================
// Probe Pad Mapping - Define all possible probe pad types
//=============================================================================

typedef struct {
    const char* name;
    int value;
} PadMapping;

// Define all possible probe pad types including special pads
static const PadMapping pad_mappings[] = {
    // Regular breadboard pads (1-60)
    {"PAD_1", 1}, {"PAD_2", 2}, {"PAD_3", 3}, {"PAD_4", 4}, {"PAD_5", 5},
    {"PAD_6", 6}, {"PAD_7", 7}, {"PAD_8", 8}, {"PAD_9", 9}, {"PAD_10", 10},
    {"PAD_11", 11}, {"PAD_12", 12}, {"PAD_13", 13}, {"PAD_14", 14}, {"PAD_15", 15},
    {"PAD_16", 16}, {"PAD_17", 17}, {"PAD_18", 18}, {"PAD_19", 19}, {"PAD_20", 20},
    {"PAD_21", 21}, {"PAD_22", 22}, {"PAD_23", 23}, {"PAD_24", 24}, {"PAD_25", 25},
    {"PAD_26", 26}, {"PAD_27", 27}, {"PAD_28", 28}, {"PAD_29", 29}, {"PAD_30", 30},
    {"PAD_31", 31}, {"PAD_32", 32}, {"PAD_33", 33}, {"PAD_34", 34}, {"PAD_35", 35},
    {"PAD_36", 36}, {"PAD_37", 37}, {"PAD_38", 38}, {"PAD_39", 39}, {"PAD_40", 40},
    {"PAD_41", 41}, {"PAD_42", 42}, {"PAD_43", 43}, {"PAD_44", 44}, {"PAD_45", 45},
    {"PAD_46", 46}, {"PAD_47", 47}, {"PAD_48", 48}, {"PAD_49", 49}, {"PAD_50", 50},
    {"PAD_51", 51}, {"PAD_52", 52}, {"PAD_53", 53}, {"PAD_54", 54}, {"PAD_55", 55},
    {"PAD_56", 56}, {"PAD_57", 57}, {"PAD_58", 58}, {"PAD_59", 59}, {"PAD_60", 60},
    
    // Special pads
    {"NO_PAD", -1}, {"NONE", -1},
    {"LOGO_PAD_TOP", 142}, {"LOGO_PAD_BOTTOM", 143},
    {"GPIO_PAD", 144}, {"DAC_PAD", 145}, {"ADC_PAD", 146},
    {"BUILDING_PAD_TOP", 147}, {"BUILDING_PAD_BOTTOM", 148},
    
    // Nano header pads (digital pins)
    {"NANO_D0", 70}, {"D0_PAD", 70},
    {"NANO_D1", 71}, {"D1_PAD", 71},
    {"NANO_D2", 72}, {"D2_PAD", 72},
    {"NANO_D3", 73}, {"D3_PAD", 73},
    {"NANO_D4", 74}, {"D4_PAD", 74},
    {"NANO_D5", 75}, {"D5_PAD", 75},
    {"NANO_D6", 76}, {"D6_PAD", 76},
    {"NANO_D7", 77}, {"D7_PAD", 77},
    {"NANO_D8", 78}, {"D8_PAD", 78},
    {"NANO_D9", 79}, {"D9_PAD", 79},
    {"NANO_D10", 80}, {"D10_PAD", 80},
    {"NANO_D11", 81}, {"D11_PAD", 81},
    {"NANO_D12", 82}, {"D12_PAD", 82},
    {"NANO_D13", 83}, {"D13_PAD", 83},
    {"NANO_RESET", 84}, {"RESET_PAD", 84},
    {"NANO_AREF", 85}, {"AREF_PAD", 85},
    
    // Nano header pads (analog pins)
    {"NANO_A0", 86}, {"A0_PAD", 86},
    {"NANO_A1", 87}, {"A1_PAD", 87},
    {"NANO_A2", 88}, {"A2_PAD", 88},
    {"NANO_A3", 89}, {"A3_PAD", 89},
    {"NANO_A4", 90}, {"A4_PAD", 90},
    {"NANO_A5", 91}, {"A5_PAD", 91},
    {"NANO_A6", 92}, {"A6_PAD", 92},
    {"NANO_A7", 93}, {"A7_PAD", 93},
    
    // Nano power/control pads (generally not routable but detectable)
    {"NANO_VIN", 69}, {"VIN_PAD", 69},
    {"NANO_RESET_0", 94}, {"RESET_0_PAD", 94},
    {"NANO_RESET_1", 95}, {"RESET_1_PAD", 95},
    {"NANO_GND_1", 96}, {"GND_1_PAD", 96},
    {"NANO_GND_0", 97}, {"GND_0_PAD", 97},
    {"NANO_3V3", 98}, {"3V3_PAD", 98},
    {"NANO_5V", 99}, {"5V_PAD", 99},
    
    // Rail pads
    {"TOP_RAIL", 101}, {"TOP_RAIL_PAD", 101},
    {"BOTTOM_RAIL", 102}, {"BOTTOM_RAIL_PAD", 102}, {"BOT_RAIL_PAD", 102},
    {"TOP_RAIL_GND", 104}, {"TOP_GND_PAD", 104},
    {"BOTTOM_RAIL_GND", 126}, {"BOT_RAIL_GND", 126}, {"BOTTOM_GND_PAD", 126}, {"BOT_GND_PAD", 126},
};

static const size_t pad_mappings_count = sizeof(pad_mappings) / sizeof(PadMapping);

// Forward declaration of function to get node name from value
const char* jl_get_node_name(int node_value);

// Function to get pad name from value
const char* jl_get_pad_name(int pad_value) {
    // Check for special pads first
    for (size_t i = 0; i < pad_mappings_count; i++) {
        if (pad_mappings[i].value == pad_value) {
            return pad_mappings[i].name;
        }
    }
    
    // For numbered pads (1-60), return the number as string
    static char pad_str[8];
    if (pad_value >= 1 && pad_value <= 60) {
        snprintf(pad_str, sizeof(pad_str), "%d", pad_value);
        return pad_str;
    }
    
    return "UNKNOWN_PAD";
}

// Implementation of function to get node name from value
const char* jl_get_node_name(int node_value) {


        // For numbered nodes (1-60), just return the number as string
    static char num_str[8];
    if (node_value >= 1 && node_value <= 60) {
        snprintf(num_str, sizeof(num_str), "%d", node_value);
        return num_str;
    }
    
    // Search in mappings for a name
    for (size_t i = 0; i < node_mappings_count; i++) {
        if (node_mappings[i].value == node_value) {
            return node_mappings[i].name;
        }
    }
    

    
    return ""; // Unknown node
}

// Function to find node value from string name (case-insensitive)
static int find_node_value(const char* name) {
    // Convert to uppercase for comparison
    char upper_name[32];
    size_t name_len = strlen(name);
    if (name_len >= sizeof(upper_name)) {
        return -1; // Name too long
    }
    
    for (size_t i = 0; i < name_len; i++) {
        upper_name[i] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 'a' + 'A') : name[i];
    }
    upper_name[name_len] = '\0';
    
    // Check direct integer first
    char* endptr;
    long int_val = strtol(upper_name, &endptr, 10);
    if (*endptr == '\0' && int_val >= 1 && int_val <= 200) {
        return (int)int_val;
    }
    
    // Search in mappings
    for (size_t i = 0; i < node_mappings_count; i++) {
        if (strcmp(upper_name, node_mappings[i].name) == 0) {
            return node_mappings[i].value;
        }
    }
    
    return -1; // Not found
}

// GPIO State Type (HIGH/LOW/FLOATING) that behaves like bool in conditionals
typedef enum {
    GPIO_STATE_LOW = 0,
    GPIO_STATE_HIGH = 1,
    GPIO_STATE_FLOATING = 2
} gpio_state_value_t;

typedef struct _gpio_state_obj_t {
    mp_obj_base_t base;
    gpio_state_value_t value;
} gpio_state_obj_t;

static void gpio_state_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    gpio_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (self->value) {
        case GPIO_STATE_HIGH:
            mp_printf(print, "HIGH");
            break;
        case GPIO_STATE_LOW:
            mp_printf(print, "LOW");
            break;
        case GPIO_STATE_FLOATING:
            mp_printf(print, "FLOATING");
            break;
        default:
            mp_printf(print, "UNKNOWN");
            break;
    }
}

static mp_obj_t gpio_state_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    gpio_state_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            // HIGH = True, LOW = False, FLOATING = False
            return mp_obj_new_bool(self->value == GPIO_STATE_HIGH);
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(state) conversion (HIGH=1, LOW=0, FLOATING=2)
            return mp_obj_new_int(self->value);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(state) conversion (HIGH=1.0, LOW=0.0, FLOATING=2.0)
            return mp_obj_new_float((float)self->value);
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
    
    if (mp_obj_is_int(args[0])) {
        // Handle integer values: 0=LOW, 1=HIGH, 2=FLOATING
        int val = mp_obj_get_int(args[0]);
        if (val == 0) o->value = GPIO_STATE_LOW;
        else if (val == 1) o->value = GPIO_STATE_HIGH;
        else if (val == 2) o->value = GPIO_STATE_FLOATING;
        else o->value = mp_obj_is_true(args[0]) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
    } else if (mp_obj_is_str(args[0])) {
        // Handle string values: "HIGH", "LOW", "FLOATING"
        const char *str = mp_obj_str_get_str(args[0]);
        if (strcmp(str, "HIGH") == 0 || strcmp(str, "high") == 0 || strcmp(str, "1") == 0) {
            o->value = GPIO_STATE_HIGH;
        } else if (strcmp(str, "LOW") == 0 || strcmp(str, "low") == 0 || strcmp(str, "0") == 0) {
            o->value = GPIO_STATE_LOW;
        } else if (strcmp(str, "FLOATING") == 0 || strcmp(str, "floating") == 0 || 
                   strcmp(str, "FLOAT") == 0 || strcmp(str, "float") == 0 || strcmp(str, "2") == 0) {
            o->value = GPIO_STATE_FLOATING;
        } else {
            mp_raise_ValueError("GPIO state must be 'HIGH', 'LOW', or 'FLOATING'");
        }
    } else {
        // Handle boolean values (default behavior)
        o->value = mp_obj_is_true(args[0]) ? GPIO_STATE_HIGH : GPIO_STATE_LOW;
    }
    
    return MP_OBJ_FROM_PTR(o);
}

// These make_new functions will be implemented after the struct definitions later in the file

static mp_obj_t gpio_state_new(gpio_state_value_t value) {
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
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(direction) conversion (OUTPUT=1, INPUT=0)
            return mp_obj_new_int(self->value ? 1 : 0);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(direction) conversion (OUTPUT=1.0, INPUT=0.0)
            return mp_obj_new_float(self->value ? 1.0 : 0.0);
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
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(pull) conversion (PULLUP=1, PULLDOWN=-1, NONE=0)
            return mp_obj_new_int(self->value);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(pull) conversion (PULLUP=1.0, PULLDOWN=-1.0, NONE=0.0)
            return mp_obj_new_float((mp_float_t)self->value);
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
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(connection) conversion (CONNECTED=1, DISCONNECTED=0)
            return mp_obj_new_int(self->value ? 1 : 0);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(connection) conversion (CONNECTED=1.0, DISCONNECTED=0.0)
            return mp_obj_new_float(self->value ? 1.0 : 0.0);
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
// Probe Button Type - Represents probe button states (NONE/CONNECT/REMOVE)
//=============================================================================

typedef struct _probe_button_obj_t {
    mp_obj_base_t base;
    int value;  // 0 = NONE, 1 = CONNECT, 2 = REMOVE
} probe_button_obj_t;

static void probe_button_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    probe_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->value == 1) {
        mp_printf(print, "CONNECT");
    } else if (self->value == 2) {
        mp_printf(print, "REMOVE");
    } else {
        mp_printf(print, "NONE");
    }
}

static mp_obj_t probe_button_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    probe_button_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            // Only CONNECT and REMOVE are "truthy"
            return mp_obj_new_bool(self->value != 0);
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(button) conversion (CONNECT=1, REMOVE=2, NONE=0)
            return mp_obj_new_int(self->value);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(button) conversion (CONNECT=1.0, REMOVE=2.0, NONE=0.0)
            return mp_obj_new_float((mp_float_t)self->value);
        default:
            return MP_OBJ_NULL;
    }
}

MP_DEFINE_CONST_OBJ_TYPE(
    probe_button_type,
    MP_QSTR_ProbeButton,
    MP_TYPE_FLAG_NONE,
    make_new, probe_button_make_new,
    print, probe_button_print,
    unary_op, probe_button_unary_op
);

static mp_obj_t probe_button_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    probe_button_obj_t *o = m_new_obj(probe_button_obj_t);
    o->base.type = &probe_button_type;
    o->value = mp_obj_get_int(args[0]);
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t probe_button_new(int value) {
    probe_button_obj_t *o = m_new_obj(probe_button_obj_t);
    o->base.type = &probe_button_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

//=============================================================================
// Node Type - Handles string names and aliases for node numbers
//=============================================================================

typedef struct _node_obj_t {
    mp_obj_base_t base;
    int value;  // the actual node number
} node_obj_t;

static void node_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    node_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    // Get the human-readable name for this node
    const char* name = jl_get_node_name(self->value);
    if (name && strlen(name) > 0) {
        mp_printf(print, "%s", name);
    } else {
        mp_printf(print, "%d", self->value);
    }
}

static mp_obj_t node_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    node_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            return mp_obj_new_bool(self->value != 0);
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(node) conversion - returns the node number
            return mp_obj_new_int(self->value);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(node) conversion - returns the node number as float
            return mp_obj_new_float((mp_float_t)self->value);
        default:
            return MP_OBJ_NULL;
    }
}

// Binary operations to allow nodes to work with integers
static mp_obj_t node_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    if (mp_obj_get_type(lhs_in) == &node_type) {
        node_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
        if (op == MP_BINARY_OP_EQUAL) {
            if (mp_obj_get_type(rhs_in) == &node_type) {
                node_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
                return mp_obj_new_bool(lhs->value == rhs->value);
            } else if (mp_obj_is_int(rhs_in)) {
                return mp_obj_new_bool(lhs->value == mp_obj_get_int(rhs_in));
            }
        } else if (op == MP_BINARY_OP_NOT_EQUAL) {
            if (mp_obj_get_type(rhs_in) == &node_type) {
                node_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
                return mp_obj_new_bool(lhs->value != rhs->value);
            } else if (mp_obj_is_int(rhs_in)) {
                return mp_obj_new_bool(lhs->value != mp_obj_get_int(rhs_in));
            }
        }
    }
    return MP_OBJ_NULL;
}

MP_DEFINE_CONST_OBJ_TYPE(
    node_type,
    MP_QSTR_node,
    MP_TYPE_FLAG_NONE,
    make_new, node_make_new,
    print, node_print,
    unary_op, node_unary_op,
    binary_op, node_binary_op
);

static mp_obj_t node_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    
    node_obj_t *o = m_new_obj(node_obj_t);
    o->base.type = &node_type;
    
    if (mp_obj_is_str(args[0])) {
        // Handle string input
        const char* name = mp_obj_str_get_str(args[0]);
        int value = find_node_value(name);
        if (value == -1) {
            mp_raise_ValueError(MP_ERROR_TEXT("Unknown node name"));
        }
        o->value = value;
    } else if (mp_obj_is_int(args[0])) {
        // Handle integer input
        o->value = mp_obj_get_int(args[0]);
    } else if (mp_obj_get_type(args[0]) == &node_type) {
        // Handle copying another node
        node_obj_t *other = MP_OBJ_TO_PTR(args[0]);
        o->value = other->value;
    } else {
        mp_raise_TypeError(MP_ERROR_TEXT("Node must be created from string, int, or another node"));
    }
    
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t node_new(int value) {
    node_obj_t *o = m_new_obj(node_obj_t);
    o->base.type = &node_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

//=============================================================================
// Probe Pad Type - Represents probe pad readings and states
//=============================================================================

typedef struct _probe_pad_obj_t {
    mp_obj_base_t base;
    int value;  // the pad number or -1 for no pad
} probe_pad_obj_t;

static void probe_pad_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    probe_pad_obj_t *self = MP_OBJ_TO_PTR(self_in);
    
    // Get the human-readable name for this pad
    const char* name = jl_get_pad_name(self->value);
    mp_printf(print, "%s", name);
}

static mp_obj_t probe_pad_unary_op(mp_unary_op_t op, mp_obj_t self_in) {
    probe_pad_obj_t *self = MP_OBJ_TO_PTR(self_in);
    switch (op) {
        case MP_UNARY_OP_BOOL:
            // Only valid pads (not -1) are "truthy"
            return mp_obj_new_bool(self->value != -1);
        case MP_UNARY_OP_INT_MAYBE:
            // Support int(pad) conversion
            return mp_obj_new_int(self->value);
        case MP_UNARY_OP_FLOAT_MAYBE:
            // Support float(pad) conversion
            return mp_obj_new_float((mp_float_t)self->value);
        default:
            return MP_OBJ_NULL;
    }
}

// Binary operations to allow pads to work with integers and strings
static mp_obj_t probe_pad_binary_op(mp_binary_op_t op, mp_obj_t lhs_in, mp_obj_t rhs_in) {
    // Handle string concatenation for all cases involving ProbePad
    if (op == MP_BINARY_OP_ADD) {
        if (mp_obj_is_str(lhs_in) && mp_obj_get_type(rhs_in) == &probe_pad_type) {
            // "string" + ProbePad
            const char* lhs_str = mp_obj_str_get_str(lhs_in);
            probe_pad_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
            const char* rhs_str = jl_get_pad_name(rhs->value);
            
            // Concatenate strings
            size_t lhs_len = strlen(lhs_str);
            size_t rhs_len = strlen(rhs_str);
            char* result = m_new(char, lhs_len + rhs_len + 1);
            strcpy(result, lhs_str);
            strcat(result, rhs_str);
            
            mp_obj_t result_obj = mp_obj_new_str(result, lhs_len + rhs_len);
            m_del(char, result, lhs_len + rhs_len + 1);
            return result_obj;
            
        } else if (mp_obj_get_type(lhs_in) == &probe_pad_type && mp_obj_is_str(rhs_in)) {
            // ProbePad + "string"
            probe_pad_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
            const char* lhs_str = jl_get_pad_name(lhs->value);
            const char* rhs_str = mp_obj_str_get_str(rhs_in);
            
            // Concatenate strings
            size_t lhs_len = strlen(lhs_str);
            size_t rhs_len = strlen(rhs_str);
            char* result = m_new(char, lhs_len + rhs_len + 1);
            strcpy(result, lhs_str);
            strcat(result, rhs_str);
            
            mp_obj_t result_obj = mp_obj_new_str(result, lhs_len + rhs_len);
            m_del(char, result, lhs_len + rhs_len + 1);
            return result_obj;
        } else if (mp_obj_get_type(rhs_in) == &probe_pad_type) {
            // Handle reverse operation: when string + ProbePad fails, MicroPython tries ProbePad + string
            // Convert any left operand to string and concatenate with ProbePad
            probe_pad_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
            const char* rhs_str = jl_get_pad_name(rhs->value);
            
            // Convert left operand to string
            if (mp_obj_is_str(lhs_in)) {
                const char* lhs_str = mp_obj_str_get_str(lhs_in);
                size_t lhs_len = strlen(lhs_str);
                size_t rhs_len = strlen(rhs_str);
                char* result = m_new(char, lhs_len + rhs_len + 1);
                strcpy(result, lhs_str);
                strcat(result, rhs_str);
                
                mp_obj_t result_obj = mp_obj_new_str(result, lhs_len + rhs_len);
                m_del(char, result, lhs_len + rhs_len + 1);
                return result_obj;
            }
        }
    }
    
    if (mp_obj_get_type(lhs_in) == &probe_pad_type) {
        probe_pad_obj_t *lhs = MP_OBJ_TO_PTR(lhs_in);
        if (op == MP_BINARY_OP_EQUAL) {
            if (mp_obj_get_type(rhs_in) == &probe_pad_type) {
                probe_pad_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
                return mp_obj_new_bool(lhs->value == rhs->value);
            } else if (mp_obj_is_int(rhs_in)) {
                return mp_obj_new_bool(lhs->value == mp_obj_get_int(rhs_in));
            }
        } else if (op == MP_BINARY_OP_NOT_EQUAL) {
            if (mp_obj_get_type(rhs_in) == &probe_pad_type) {
                probe_pad_obj_t *rhs = MP_OBJ_TO_PTR(rhs_in);
                return mp_obj_new_bool(lhs->value != rhs->value);
            } else if (mp_obj_is_int(rhs_in)) {
                return mp_obj_new_bool(lhs->value != mp_obj_get_int(rhs_in));
            }
        }
    }
    return MP_OBJ_NULL;
}

// __str__ method for ProbePad
static mp_obj_t probe_pad_str(mp_obj_t self_in) {
    probe_pad_obj_t *self = MP_OBJ_TO_PTR(self_in);
    const char* name = jl_get_pad_name(self->value);
    return mp_obj_new_str(name, strlen(name));
}
static MP_DEFINE_CONST_FUN_OBJ_1(probe_pad_str_obj, probe_pad_str);

// ProbePad methods table
static const mp_rom_map_elem_t probe_pad_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___str__), MP_ROM_PTR(&probe_pad_str_obj) },
};
static MP_DEFINE_CONST_DICT(probe_pad_locals_dict, probe_pad_locals_dict_table);

MP_DEFINE_CONST_OBJ_TYPE(
    probe_pad_type,
    MP_QSTR_ProbePad,
    MP_TYPE_FLAG_NONE,
    make_new, probe_pad_make_new,
    print, probe_pad_print,
    unary_op, probe_pad_unary_op,
    binary_op, probe_pad_binary_op,
    locals_dict, &probe_pad_locals_dict
);

static mp_obj_t probe_pad_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, 1, false);
    
    probe_pad_obj_t *o = m_new_obj(probe_pad_obj_t);
    o->base.type = &probe_pad_type;
    o->value = mp_obj_get_int(args[0]);
    
    return MP_OBJ_FROM_PTR(o);
}

static mp_obj_t probe_pad_new(int value) {
    probe_pad_obj_t *o = m_new_obj(probe_pad_obj_t);
    o->base.type = &probe_pad_type;
    o->value = value;
    return MP_OBJ_FROM_PTR(o);
}

// Helper function to extract integer from node or int argument
static int get_node_value(mp_obj_t obj) {
    if (mp_obj_get_type(obj) == &node_type) {
        node_obj_t *node = MP_OBJ_TO_PTR(obj);
        return node->value;
    } else if (mp_obj_is_int(obj)) {
        return mp_obj_get_int(obj);
    } else if (mp_obj_is_str(obj)) {
        // Allow direct string to int conversion in functions
        const char* name = mp_obj_str_get_str(obj);
        int value = find_node_value(name);
        if (value == -1) {
            mp_raise_ValueError(MP_ERROR_TEXT("Unknown node name"));
        }
        return value;
    }
    mp_raise_TypeError(MP_ERROR_TEXT("Expected node, int, or string"));
}

// Helper function to map node values to DAC channels
static int map_node_to_dac_channel(int node_value) {
    switch (node_value) {
        case 106: // DAC0
            return 0;
        case 107: // DAC1
            return 1;
        case 101: // TOP_RAIL
            return 2;
        case 102: // BOTTOM_RAIL
            return 3;
        default:
            // If it's already a channel number (0-3), return it
            if (node_value >= 0 && node_value <= 3) {
                return node_value;
            }
            return -1; // Invalid
    }
}

// Helper function to get DAC channel from node or int argument
static int get_dac_channel(mp_obj_t obj) {
    int node_value = get_node_value(obj);
    int channel = map_node_to_dac_channel(node_value);
    if (channel == -1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid DAC channel or node. Use 0-3, DAC0, DAC1, TOP_RAIL, or BOTTOM_RAIL"));
    }
    return channel;
}

//=============================================================================
// Function Implementations
//=============================================================================

// DAC Functions
static mp_obj_t jl_dac_set_func(size_t n_args, const mp_obj_t *args) {
    int channel = get_dac_channel(args[0]);
    float voltage = mp_obj_get_float(args[1]);
    int save = (n_args > 2) ? mp_obj_is_true(args[2]) ? 1 : 0 : 1; // Default save=True
    
    printf("jl_dac_set: channel: %d, voltage: %f, save: %d\n", channel, voltage, save);
    jl_dac_set(channel, voltage, save);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_dac_set_obj, 2, 3, jl_dac_set_func);

static mp_obj_t jl_dac_get_func(mp_obj_t channel_obj) {
    int channel = get_dac_channel(channel_obj);
    
    float voltage = jl_dac_get(channel);
    
    // Return voltage as float for backward compatibility
    return mp_obj_new_float(voltage);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_dac_get_obj, jl_dac_get_func);

// Wavegen MicroPython bindings
// Helper: parse node/int/str to DAC channel 0..3 for wavegen_set_output
static int get_wavegen_channel(mp_obj_t obj) {
    int node_value = get_node_value(obj);
    int ch = map_node_to_dac_channel(node_value);
    if (ch < 0 || ch > 3) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid output. Use DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL"));
    }
    return ch;
}

// Helper: parse waveform from int or string
static int get_wavegen_wave(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        int w = mp_obj_get_int(obj);
        if (w < 0 || w > 4) {
            mp_raise_ValueError(MP_ERROR_TEXT("Waveform must be 0-4"));
        }
        return w;
    } else if (mp_obj_is_str(obj)) {
        const char *s = mp_obj_str_get_str(obj);
        // accept common aliases case-insensitively
        char up[24];
        size_t n = strlen(s);
        if (n > 23) n = 23;
        for (size_t i = 0; i < n; i++) up[i] = (char)toupper((unsigned char)s[i]);
        up[n] = '\0';
        if (strcmp(up, "SINE") == 0) return 0;
        if (strcmp(up, "TRIANGLE") == 0 || strcmp(up, "TRI") == 0) return 1;
        if (strcmp(up, "RAMP") == 0 || strcmp(up, "SAW") == 0 || strcmp(up, "SAWTOOTH") == 0) return 2;
        if (strcmp(up, "SQUARE") == 0 || strcmp(up, "SQ") == 0) return 3;
        if (strcmp(up, "ARBITRARY") == 0 || strcmp(up, "ARB") == 0) return 4;
        mp_raise_ValueError(MP_ERROR_TEXT("Unknown waveform"));
    }
    mp_raise_TypeError(MP_ERROR_TEXT("Expected int or string for waveform"));
}

static mp_obj_t jl_wavegen_set_output_func(mp_obj_t out_obj) {
    int ch = get_wavegen_channel(out_obj);
    jl_wavegen_set_output(ch);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_wavegen_set_output_obj, jl_wavegen_set_output_func);

static mp_obj_t jl_wavegen_set_freq_func(mp_obj_t hz_obj) {
    float hz = mp_obj_get_float(hz_obj);
    jl_wavegen_set_freq(hz);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_wavegen_set_freq_obj, jl_wavegen_set_freq_func);

static mp_obj_t jl_wavegen_set_wave_func(mp_obj_t w_obj) {
    int w = get_wavegen_wave(w_obj);
    if (w == 4) {
        // ARBITRARY not implemented yet
        mp_raise_NotImplementedError(MP_ERROR_TEXT("ARBITRARY waveform not implemented yet"));
    }
    jl_wavegen_set_wave(w);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_wavegen_set_wave_obj, jl_wavegen_set_wave_func);

static mp_obj_t jl_wavegen_set_sweep_func(size_t n_args, const mp_obj_t *args) {
    if (n_args != 3) {
        mp_raise_TypeError(MP_ERROR_TEXT("wavegen_set_sweep(start_hz, end_hz, seconds)"));
    }
    float start_hz = mp_obj_get_float(args[0]);
    float end_hz = mp_obj_get_float(args[1]);
    float seconds = mp_obj_get_float(args[2]);
    jl_wavegen_set_sweep(start_hz, end_hz, seconds);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_wavegen_set_sweep_obj, 3, 3, jl_wavegen_set_sweep_func);

static mp_obj_t jl_wavegen_set_amplitude_func(mp_obj_t vpp_obj) {
    float vpp = mp_obj_get_float(vpp_obj);
    jl_wavegen_set_amplitude(vpp);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_wavegen_set_amplitude_obj, jl_wavegen_set_amplitude_func);

static mp_obj_t jl_wavegen_set_offset_func(mp_obj_t v_obj) {
    float v = mp_obj_get_float(v_obj);
    jl_wavegen_set_offset(v);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_wavegen_set_offset_obj, jl_wavegen_set_offset_func);

static mp_obj_t jl_wavegen_start_func(size_t n_args, const mp_obj_t *args) {
    // wavegen_start([run=True])
    int run = 1;
    if (n_args >= 1) {
        run = mp_obj_is_true(args[0]) ? 1 : 0;
    }
    jl_wavegen_start(run);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_wavegen_start_obj, 0, 1, jl_wavegen_start_func);

static mp_obj_t jl_wavegen_stop_func(void) {
    jl_wavegen_stop();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_stop_obj, jl_wavegen_stop_func);

// Getters
static mp_obj_t jl_wavegen_get_output_func(void) { return mp_obj_new_int(jl_wavegen_get_output()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_get_output_obj, jl_wavegen_get_output_func);
static mp_obj_t jl_wavegen_get_freq_func(void) { return mp_obj_new_float(jl_wavegen_get_freq()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_get_freq_obj, jl_wavegen_get_freq_func);
static mp_obj_t jl_wavegen_get_wave_func(void) { return mp_obj_new_int(jl_wavegen_get_wave()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_get_wave_obj, jl_wavegen_get_wave_func);
static mp_obj_t jl_wavegen_get_amplitude_func(void) { return mp_obj_new_float(jl_wavegen_get_amplitude()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_get_amplitude_obj, jl_wavegen_get_amplitude_func);
static mp_obj_t jl_wavegen_get_offset_func(void) { return mp_obj_new_float(jl_wavegen_get_offset()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_get_offset_obj, jl_wavegen_get_offset_func);
static mp_obj_t jl_wavegen_is_running_func(void) { return mp_obj_new_bool(jl_wavegen_is_running()); }
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wavegen_is_running_obj, jl_wavegen_is_running_func);
// Remove old AWG stubs; replaced by wavegen_* API

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
// Helper: map an incoming pin object (int or node) to a physical GPIO that
// jl_gpio_* backends understand. Supports:
// - Breadboard/routable GPIO indices: 1-10 (as-is)
// - RP GPIOs 20-27 (as-is)
// - Node constants GPIO_1..GPIO_8 (131..138) → RP GPIO 20..27
// - UART nodes UART_TX (116) → 0, UART_RX (117) → 1
static int map_pin_obj_to_physical_gpio(mp_obj_t pin_obj) {
    int pin = -1;

    // Accept pre-wrapped node objects
    if (mp_obj_get_type(pin_obj) == &node_type) {
        int node = get_node_value(pin_obj);
        if (node >= 131 && node <= 138) {
            // GPIO_1..GPIO_8 → GP20..GP27
            pin = 20 + (node - 131);
        } else if (node == 116) {
            // UART_TX → GP0
            pin = 0;
        } else if (node == 117) {
            // UART_RX → GP1
            pin = 1;
        } else {
            // Not a supported GPIO-like node
            pin = -1;
        }
    } else {
        // Try numeric conversion (accepts small-int and int objects)
        pin = mp_obj_get_int(pin_obj);
    }

    // Validate the mapped pin for our backends
    if (!((pin >= 1 && pin <= 10) || (pin >= 20 && pin <= 27) || pin == 0 || pin == 1)) {
        return -1;
    }
    return pin;
}

static mp_obj_t jl_gpio_set_func(mp_obj_t pin_obj, mp_obj_t value_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    int value = get_gpio_state_value(value_obj);
    
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    
    jl_gpio_set(pin, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_obj, jl_gpio_set_func);

static mp_obj_t jl_gpio_set_dir_func(mp_obj_t pin_obj, mp_obj_t direction_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    int direction = get_direction_value(direction_obj);
    
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    
    jl_gpio_set_dir(pin, direction);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_dir_obj, jl_gpio_set_dir_func);

static mp_obj_t jl_gpio_get_func(mp_obj_t pin_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    
    int value = jl_gpio_get(pin);
    
    // Return custom GPIO state object that displays as HIGH/LOW/FLOATING
    // jl_gpio_get returns: 0=LOW, 1=HIGH, 2=FLOATING
    gpio_state_value_t state;
    switch (value) {
        case 0: state = GPIO_STATE_LOW; break;
        case 1: state = GPIO_STATE_HIGH; break;
        case 2: state = GPIO_STATE_FLOATING; break;
        default: state = GPIO_STATE_LOW; break; // Default to LOW for safety
    }
    
    return gpio_state_new(state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_obj, jl_gpio_get_func);

static mp_obj_t jl_gpio_get_dir_func(mp_obj_t pin_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    int direction = jl_gpio_get_dir(pin);
    
    // Return custom GPIO direction object that displays as INPUT/OUTPUT but behaves as boolean
    return gpio_direction_new(direction);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_dir_obj, jl_gpio_get_dir_func);

static mp_obj_t jl_gpio_set_pull_func(mp_obj_t pin_obj, mp_obj_t pull_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    int pull = get_pull_value(pull_obj);
    
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    jl_gpio_set_pull(pin, pull);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_gpio_set_pull_obj, jl_gpio_set_pull_func);

static mp_obj_t jl_gpio_get_pull_func(mp_obj_t pin_obj) {
    int pin = map_pin_obj_to_physical_gpio(pin_obj);
    if (pin < 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-10, GPIO_1-GPIO_8, GPIO_20-GPIO_27, or UART_TX/UART_RX"));
    }
    int pull = jl_gpio_get_pull(pin);
    
    // Return custom GPIO pull object that displays as PULLUP/PULLDOWN/NONE
    return gpio_pull_new(pull);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_gpio_get_pull_obj, jl_gpio_get_pull_func);

// PWM Functions
static mp_obj_t jl_pwm_func(size_t n_args, const mp_obj_t *args) {
    int gpio_pin = mp_obj_get_int(args[0]);
    float frequency = 1.0; // Default frequency
    float duty_cycle = 0.5;   // Default duty cycle
    
    if (n_args > 1) {
        frequency = mp_obj_get_float(args[1]);
    }
    if (n_args > 2) {
        duty_cycle = mp_obj_get_float(args[2]);
    }
    
    // Convert GPIO node constants (131-138) to pin numbers (1-8)
    if (gpio_pin >= 131 && gpio_pin <= 138) {
        gpio_pin = gpio_pin - 131 + 1;
    }
    
    if (gpio_pin < 1 || gpio_pin > 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-8 or GPIO_1-GPIO_8"));
    }
    
    if (frequency < 0.0009 || frequency > 62500000.0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM frequency must be 0.001Hz to 62.5MHz"));
    }
    
    if (duty_cycle < 0.0 || duty_cycle > 1.0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM duty cycle must be 0.0 to 1.0"));
    }
    
    int result = jl_pwm_setup(gpio_pin, frequency, duty_cycle);
    
    if (result != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM setup failed"));
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_pwm_obj, 1, 3, jl_pwm_func);

static mp_obj_t jl_pwm_set_duty_cycle_func(mp_obj_t pin_obj, mp_obj_t duty_cycle_obj) {
    int gpio_pin = mp_obj_get_int(pin_obj);
    float duty_cycle = mp_obj_get_float(duty_cycle_obj);
    
    // Convert GPIO node constants (131-138) to pin numbers (1-8)
    if (gpio_pin >= 131 && gpio_pin <= 138) {
        gpio_pin = gpio_pin - 131 + 1;
    }
    
    if (gpio_pin < 1 || gpio_pin > 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-8 or GPIO_1-GPIO_8"));
    }
    
    if (duty_cycle < 0.0 || duty_cycle > 1.0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM duty cycle must be 0.0 to 1.0"));
    }
    
    int result = jl_pwm_set_duty_cycle(gpio_pin, duty_cycle);
    
        if (result != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM duty cycle set failed"));
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_pwm_set_duty_cycle_obj, jl_pwm_set_duty_cycle_func);

static mp_obj_t jl_pwm_set_frequency_func(mp_obj_t pin_obj, mp_obj_t frequency_obj) {
    int gpio_pin = mp_obj_get_int(pin_obj);
    float frequency = mp_obj_get_float(frequency_obj);
    
    // Convert GPIO node constants (131-138) to pin numbers (1-8)
    if (gpio_pin >= 131 && gpio_pin <= 138) {
        gpio_pin = gpio_pin - 131 + 1;
    }
    
    if (gpio_pin < 1 || gpio_pin > 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-8 or GPIO_1-GPIO_8"));
    }
    
    if (frequency < 0.0009 || frequency > 62500000.0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM frequency must be 0.001Hz to 62.5MHz"));
    }
    
    int result = jl_pwm_set_frequency(gpio_pin, frequency);
    
    if (result != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM frequency set failed"));
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_pwm_set_frequency_obj, jl_pwm_set_frequency_func);

static mp_obj_t jl_pwm_stop_func(mp_obj_t pin_obj) {
    int gpio_pin = mp_obj_get_int(pin_obj);
    
    // Convert GPIO node constants (131-138) to pin numbers (1-8)
    if (gpio_pin >= 131 && gpio_pin <= 138) {
        gpio_pin = gpio_pin - 131 + 1;
    }
    
    if (gpio_pin < 1 || gpio_pin > 8) {
        mp_raise_ValueError(MP_ERROR_TEXT("GPIO pin must be 1-8 or GPIO_1-GPIO_8"));
    }
    
    int result = jl_pwm_stop(gpio_pin);
    
    if (result != 0) {
        mp_raise_ValueError(MP_ERROR_TEXT("PWM stop failed"));
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_pwm_stop_obj, jl_pwm_stop_func);

// Node Functions
static mp_obj_t jl_nodes_connect_func(size_t n_args, const mp_obj_t *args) {
    int node1 = get_node_value(args[0]);
    int node2 = get_node_value(args[1]);
    int save = (n_args > 2) ? mp_obj_is_true(args[2]) ? 1 : 0 : 0; // Default save=False (use local copy)
    
    jl_nodes_connect(node1, node2, save);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_nodes_connect_obj, 2, 3, jl_nodes_connect_func);

static mp_obj_t jl_nodes_disconnect_func(mp_obj_t node1_obj, mp_obj_t node2_obj) {
    int node1 = get_node_value(node1_obj);
    int node2 = get_node_value(node2_obj);
    
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
    int node1 = get_node_value(node1_obj);
    int node2 = get_node_value(node2_obj);
    int connected = jl_nodes_is_connected(node1, node2);
    
    // Return custom connection state object that displays as CONNECTED/DISCONNECTED but behaves as boolean
    return connection_state_new(connected);
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_nodes_is_connected_obj, jl_nodes_is_connected_func);

static mp_obj_t jl_nodes_save_func(size_t n_args, const mp_obj_t *args) {
    int slot = (n_args > 0) ? mp_obj_get_int(args[0]) : -1; // Default to current slot if not specified
    
    int result = jl_nodes_save(slot);
    return mp_obj_new_int(result);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_nodes_save_obj, 0, 1, jl_nodes_save_func);

// Raw Hardware Functions
static mp_obj_t jl_send_raw_func(size_t n_args, const mp_obj_t *args) {
    int x = mp_obj_get_int(args[1]);
    int y = mp_obj_get_int(args[2]);
    int setOrClear = (n_args > 3) ? mp_obj_get_int(args[3]) : 1; // Default to set (1)
    
    // Handle chip parameter (can be int, string, or char)
    if (mp_obj_is_int(args[0])) {
        // Integer chip number (0-11)
        int chip = mp_obj_get_int(args[0]);
        jl_send_raw(chip, x, y, setOrClear);
    } else if (mp_obj_is_str(args[0])) {
        // String chip identifier ("A"-"L" or "0"-"11")
        const char* chip_str = mp_obj_str_get_str(args[0]);
        jl_send_raw_str(chip_str, x, y, setOrClear);
    } else {
        mp_raise_ValueError(MP_ERROR_TEXT("Chip must be integer (0-11) or string ('A'-'L')"));
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_send_raw_obj, 3, 4, jl_send_raw_func);

static mp_obj_t jl_switch_slot_func(mp_obj_t slot_obj) {
    int slot = mp_obj_get_int(slot_obj);
    int result = jl_switch_slot(slot);
    
    if (result == -1) {
        mp_raise_ValueError(MP_ERROR_TEXT("Invalid slot number"));
    }
    
    return mp_obj_new_int(result);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_switch_slot_obj, jl_switch_slot_func);

static mp_obj_t jl_nodes_discard_func(void) {
    jl_restore_micropython_entry_state();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_discard_obj, jl_nodes_discard_func);

static mp_obj_t jl_nodes_has_changes_func(void) {
    int has_changes = jl_has_unsaved_changes();
    return mp_obj_new_bool(has_changes);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_nodes_has_changes_obj, jl_nodes_has_changes_func);

// OLED Functions
static mp_obj_t jl_oled_print_func(size_t n_args, const mp_obj_t *args) {
    const char* text;
    char buffer[64]; // Buffer for converting non-string types
    
    // Handle different input types
    if (mp_obj_is_str(args[0])) {
        // String - use directly
        text = mp_obj_str_get_str(args[0]);
    } else if (mp_obj_is_int(args[0])) {
        // Integer - convert to string
        int value = mp_obj_get_int(args[0]);
        snprintf(buffer, sizeof(buffer), "%d", value);
        text = buffer;
    } else if (mp_obj_get_type(args[0]) == &node_type) {
        // Node type - get its string representation
        node_obj_t *node = MP_OBJ_TO_PTR(args[0]);
        const char* name = jl_get_node_name(node->value);
        if (name && strlen(name) > 0) {
            strncpy(buffer, name, sizeof(buffer) - 1);
            buffer[sizeof(buffer) - 1] = '\0';
        } else {
            snprintf(buffer, sizeof(buffer), "%d", node->value);
        }
        text = buffer;
    } else if (mp_obj_get_type(args[0]) == &gpio_state_type) {
        // GPIO State - HIGH/LOW/FLOATING
        gpio_state_obj_t *state = MP_OBJ_TO_PTR(args[0]);
        switch (state->value) {
            case GPIO_STATE_HIGH: text = "HIGH"; break;
            case GPIO_STATE_LOW: text = "LOW"; break;
            case GPIO_STATE_FLOATING: text = "FLOATING"; break;
            default: text = "UNKNOWN"; break;
        }
    } else if (mp_obj_get_type(args[0]) == &gpio_direction_type) {
        // GPIO Direction - INPUT/OUTPUT
        gpio_direction_obj_t *dir = MP_OBJ_TO_PTR(args[0]);
        text = dir->value ? "OUTPUT" : "INPUT";
    } else if (mp_obj_get_type(args[0]) == &gpio_pull_type) {
        // GPIO Pull - PULLUP/PULLDOWN/NONE
        gpio_pull_obj_t *pull = MP_OBJ_TO_PTR(args[0]);
        if (pull->value == 1) {
            text = "PULLUP";
        } else if (pull->value == -1) {
            text = "PULLDOWN";
        } else {
            text = "NONE";
        }
    } else if (mp_obj_get_type(args[0]) == &connection_state_type) {
        // Connection State - CONNECTED/DISCONNECTED
        connection_state_obj_t *conn = MP_OBJ_TO_PTR(args[0]);
        text = conn->value ? "CONNECTED" : "DISCONNECTED";
    } else if (mp_obj_get_type(args[0]) == &probe_button_type) {
        // Probe Button - CONNECT/REMOVE/NONE
        probe_button_obj_t *button = MP_OBJ_TO_PTR(args[0]);
        if (button->value == 1) {
            text = "CONNECT";
        } else if (button->value == 2) {
            text = "REMOVE";
        } else {
            text = "NONE";
        }
    } else if (mp_obj_get_type(args[0]) == &probe_pad_type) {
        // Probe Pad - get its string representation
        probe_pad_obj_t *pad = MP_OBJ_TO_PTR(args[0]);
        const char* name = jl_get_pad_name(pad->value);
        strncpy(buffer, name, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = '\0';
        text = buffer;
    } else if (mp_obj_is_float(args[0])) {
        // Float - convert to string with 3 decimal places
        float value = mp_obj_get_float(args[0]);
        snprintf(buffer, sizeof(buffer), "%.3f", value);
        text = buffer;
    } else if (mp_obj_is_bool(args[0])) {
        // Boolean - True/False
        text = mp_obj_is_true(args[0]) ? "True" : "False";
    } else {
        // Fallback - try to convert to string representation
        mp_obj_print_helper(&mp_plat_print, args[0], PRINT_STR);
        text = "???"; // This is a fallback if we can't convert
    }
    
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

// Core2 Functions
static mp_obj_t jl_pause_core2_func(mp_obj_t pause_obj) {
    bool pause;
    
    // Handle different input types: bool, int, or string
    if (mp_obj_is_bool(pause_obj)) {
        pause = mp_obj_is_true(pause_obj);
    } else if (mp_obj_is_int(pause_obj)) {
        pause = mp_obj_get_int(pause_obj) != 0;
    } else if (mp_obj_is_str(pause_obj)) {
        const char* str = mp_obj_str_get_str(pause_obj);
        // Accept "true", "1", "on", "yes" as true, everything else as false
        pause = (strcmp(str, "true") == 0 || strcmp(str, "1") == 0 || 
                strcmp(str, "on") == 0 || strcmp(str, "yes") == 0);
    } else {
        mp_raise_TypeError("pause_core2() argument must be bool, int, or string");
    }
    
    jl_pause_core2(pause);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_pause_core2_obj, jl_pause_core2_func);

// Terminal Color Functions
static mp_obj_t jl_change_terminal_color_func(size_t n_args, const mp_obj_t *args) {
    int color = -1;
    bool flush = true;
    
    if (n_args >= 1) {
        color = mp_obj_get_int(args[0]);
    }
    if (n_args >= 2) {
        flush = mp_obj_is_true(args[1]);
    }
    
    jl_change_terminal_color(color, flush);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_change_terminal_color_obj, 0, 2, jl_change_terminal_color_func);

static mp_obj_t jl_cycle_term_color_func(size_t n_args, const mp_obj_t *args) {
    bool reset = false;
    float step = 100.0;
    bool flush = true;
    
    if (n_args >= 1) {
        reset = mp_obj_is_true(args[0]);
    }
    if (n_args >= 2) {
        step = mp_obj_get_float(args[1]);
    }
    if (n_args >= 3) {
        flush = mp_obj_is_true(args[2]);
    }
    
    jl_cycle_term_color(reset, step, flush);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_cycle_term_color_obj, 0, 3, jl_cycle_term_color_func);

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
    int node = get_node_value(node_obj);
    jl_probe_tap(node);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_probe_tap_obj, jl_probe_tap_func);

static mp_obj_t jl_probe_read_blocking_func(void) {
    int pad = jl_probe_read_blocking();
    
    // Check for interrupt signal (-999)
    if (pad == -999) {
        mp_raise_msg(&mp_type_KeyboardInterrupt, "Ctrl+Q");
    }
    
    return probe_pad_new(pad);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_read_blocking_obj, jl_probe_read_blocking_func);

static mp_obj_t jl_probe_read_nonblocking_func(void) {
    int pad = jl_probe_read_nonblocking();
    return probe_pad_new(pad);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_read_nonblocking_obj, jl_probe_read_nonblocking_func);

static mp_obj_t jl_probe_button_blocking_func(void) {
    int button_state = jl_probe_button_blocking();
    
    // Check for interrupt signal (-999) 
    if (button_state == -999) {
        mp_raise_msg(&mp_type_KeyboardInterrupt, "Ctrl+Q");
    }
    
    return probe_button_new(button_state);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_button_blocking_obj, jl_probe_button_blocking_func);

static mp_obj_t jl_probe_button_nonblocking_func(void) {
    int button_state = jl_probe_button_nonblocking();
    return probe_button_new(button_state);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_button_nonblocking_obj, jl_probe_button_nonblocking_func);

// Probe aliases
static mp_obj_t jl_probe_read_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_read_obj, jl_probe_read_func);

static mp_obj_t jl_read_probe_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_read_probe_obj, jl_read_probe_func);

// Parameterized probe_read function with blocking parameter
static mp_obj_t jl_probe_read_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_read_blocking_func();
    } else {
        return jl_probe_read_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_probe_read_param_obj, 0, 1, jl_probe_read_param_func);

// Parameterized read_probe function with blocking parameter
static mp_obj_t jl_read_probe_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_read_blocking_func();
    } else {
        return jl_probe_read_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_read_probe_param_obj, 0, 1, jl_read_probe_param_func);

// Additional probe_read_blocking aliases
static mp_obj_t jl_probe_wait_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_wait_obj, jl_probe_wait_func);

static mp_obj_t jl_wait_probe_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wait_probe_obj, jl_wait_probe_func);

static mp_obj_t jl_probe_touch_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_touch_obj, jl_probe_touch_func);

static mp_obj_t jl_wait_touch_func(void) {
    return jl_probe_read_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_wait_touch_obj, jl_wait_touch_func);

// Probe button aliases (blocking by default for simplicity)
static mp_obj_t jl_get_button_func(void) {
    return jl_probe_button_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_get_button_obj, jl_get_button_func);

static mp_obj_t jl_button_read_func(void) {
    return jl_probe_button_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_button_read_obj, jl_button_read_func);

static mp_obj_t jl_read_button_func(void) {
    return jl_probe_button_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_read_button_obj, jl_read_button_func);

static mp_obj_t jl_probe_button_func(void) {
    return jl_probe_button_blocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_probe_button_obj, jl_probe_button_func);

// Non-blocking button aliases
static mp_obj_t jl_check_button_func(void) {
    return jl_probe_button_nonblocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_check_button_obj, jl_check_button_func);

static mp_obj_t jl_button_check_func(void) {
    return jl_probe_button_nonblocking_func();
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_button_check_obj, jl_button_check_func);

// Parameterized button functions with blocking parameter
static mp_obj_t jl_probe_button_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_button_blocking_func();
    } else {
        return jl_probe_button_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_probe_button_param_obj, 0, 1, jl_probe_button_param_func);

static mp_obj_t jl_get_button_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_button_blocking_func();
    } else {
        return jl_probe_button_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_get_button_param_obj, 0, 1, jl_get_button_param_func);

static mp_obj_t jl_button_read_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_button_blocking_func();
    } else {
        return jl_probe_button_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_button_read_param_obj, 0, 1, jl_button_read_param_func);

static mp_obj_t jl_read_button_param_func(size_t n_args, const mp_obj_t *args) {
    bool blocking = true; // Default to blocking
    if (n_args > 0) {
        blocking = mp_obj_is_true(args[0]);
    }
    
    if (blocking) {
        return jl_probe_button_blocking_func();
    } else {
        return jl_probe_button_nonblocking_func();
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_read_button_param_obj, 0, 1, jl_read_button_param_func);

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

// Node creation function
static mp_obj_t jl_node_func(mp_obj_t name_obj) {
    if (mp_obj_is_str(name_obj)) {
        const char* name = mp_obj_str_get_str(name_obj);
        int value = find_node_value(name);
        if (value == -1) {
            mp_raise_ValueError(MP_ERROR_TEXT("Unknown node name"));
        }
        return node_new(value);
    } else if (mp_obj_is_int(name_obj)) {
        return node_new(mp_obj_get_int(name_obj));
    } else if (mp_obj_get_type(name_obj) == &node_type) {
        // Return copy of existing node
        return name_obj;
    }
    mp_raise_TypeError(MP_ERROR_TEXT("Node must be created from string, int, or another node"));
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_node_obj, jl_node_func);

// Pre-defined node constants
static const node_obj_t node_top_rail_obj = { .base = { &node_type }, .value = 101 };
static const node_obj_t node_bottom_rail_obj = { .base = { &node_type }, .value = 102 };
static const node_obj_t node_gnd_obj = { .base = { &node_type }, .value = 100 };
static const node_obj_t node_dac0_obj = { .base = { &node_type }, .value = 106 };
static const node_obj_t node_dac1_obj = { .base = { &node_type }, .value = 107 };

// Pre-defined probe button constants
static const probe_button_obj_t probe_button_none_obj = { .base = { &probe_button_type }, .value = 0 };
static const probe_button_obj_t probe_button_connect_obj = { .base = { &probe_button_type }, .value = 1 };
static const probe_button_obj_t probe_button_remove_obj = { .base = { &probe_button_type }, .value = 2 };

// Pre-defined probe pad constants
static const probe_pad_obj_t probe_no_pad_obj = { .base = { &probe_pad_type }, .value = -1 };
static const probe_pad_obj_t probe_logo_pad_top_obj = { .base = { &probe_pad_type }, .value = 142 };
static const probe_pad_obj_t probe_logo_pad_bottom_obj = { .base = { &probe_pad_type }, .value = 143 };
static const probe_pad_obj_t probe_gpio_pad_obj = { .base = { &probe_pad_type }, .value = 144 };
static const probe_pad_obj_t probe_dac_pad_obj = { .base = { &probe_pad_type }, .value = 145 };
static const probe_pad_obj_t probe_adc_pad_obj = { .base = { &probe_pad_type }, .value = 146 };
static const probe_pad_obj_t probe_building_pad_top_obj = { .base = { &probe_pad_type }, .value = 147 };
static const probe_pad_obj_t probe_building_pad_bottom_obj = { .base = { &probe_pad_type }, .value = 148 };

// Nano power/control pad constants
static const probe_pad_obj_t probe_nano_vin_obj = { .base = { &probe_pad_type }, .value = 69 };
static const probe_pad_obj_t probe_nano_reset_0_obj = { .base = { &probe_pad_type }, .value = 94 };
static const probe_pad_obj_t probe_nano_reset_1_obj = { .base = { &probe_pad_type }, .value = 95 };
static const probe_pad_obj_t probe_nano_gnd_1_obj = { .base = { &probe_pad_type }, .value = 96 };
static const probe_pad_obj_t probe_nano_gnd_0_obj = { .base = { &probe_pad_type }, .value = 97 };
static const probe_pad_obj_t probe_nano_3v3_obj = { .base = { &probe_pad_type }, .value = 98 };
static const probe_pad_obj_t probe_nano_5v_obj = { .base = { &probe_pad_type }, .value = 99 };

// Nano digital pin pad constants
static const probe_pad_obj_t probe_d0_pad_obj = { .base = { &probe_pad_type }, .value = 70 };
static const probe_pad_obj_t probe_d1_pad_obj = { .base = { &probe_pad_type }, .value = 71 };
static const probe_pad_obj_t probe_d2_pad_obj = { .base = { &probe_pad_type }, .value = 72 };
static const probe_pad_obj_t probe_d3_pad_obj = { .base = { &probe_pad_type }, .value = 73 };
static const probe_pad_obj_t probe_d4_pad_obj = { .base = { &probe_pad_type }, .value = 74 };
static const probe_pad_obj_t probe_d5_pad_obj = { .base = { &probe_pad_type }, .value = 75 };
static const probe_pad_obj_t probe_d6_pad_obj = { .base = { &probe_pad_type }, .value = 76 };
static const probe_pad_obj_t probe_d7_pad_obj = { .base = { &probe_pad_type }, .value = 77 };
static const probe_pad_obj_t probe_d8_pad_obj = { .base = { &probe_pad_type }, .value = 78 };
static const probe_pad_obj_t probe_d9_pad_obj = { .base = { &probe_pad_type }, .value = 79 };
static const probe_pad_obj_t probe_d10_pad_obj = { .base = { &probe_pad_type }, .value = 80 };
static const probe_pad_obj_t probe_d11_pad_obj = { .base = { &probe_pad_type }, .value = 81 };
static const probe_pad_obj_t probe_d12_pad_obj = { .base = { &probe_pad_type }, .value = 82 };
static const probe_pad_obj_t probe_d13_pad_obj = { .base = { &probe_pad_type }, .value = 83 };
static const probe_pad_obj_t probe_reset_pad_obj = { .base = { &probe_pad_type }, .value = 84 };
static const probe_pad_obj_t probe_aref_pad_obj = { .base = { &probe_pad_type }, .value = 85 };

// Nano analog pin pad constants
static const probe_pad_obj_t probe_a0_pad_obj = { .base = { &probe_pad_type }, .value = 86 };
static const probe_pad_obj_t probe_a1_pad_obj = { .base = { &probe_pad_type }, .value = 87 };
static const probe_pad_obj_t probe_a2_pad_obj = { .base = { &probe_pad_type }, .value = 88 };
static const probe_pad_obj_t probe_a3_pad_obj = { .base = { &probe_pad_type }, .value = 89 };
static const probe_pad_obj_t probe_a4_pad_obj = { .base = { &probe_pad_type }, .value = 90 };
static const probe_pad_obj_t probe_a5_pad_obj = { .base = { &probe_pad_type }, .value = 91 };
static const probe_pad_obj_t probe_a6_pad_obj = { .base = { &probe_pad_type }, .value = 92 };
static const probe_pad_obj_t probe_a7_pad_obj = { .base = { &probe_pad_type }, .value = 93 };

// Rail pad constants
static const probe_pad_obj_t probe_top_rail_pad_obj = { .base = { &probe_pad_type }, .value = 101 };
static const probe_pad_obj_t probe_bottom_rail_pad_obj = { .base = { &probe_pad_type }, .value = 102 };
static const probe_pad_obj_t probe_top_rail_gnd_obj = { .base = { &probe_pad_type }, .value = 104 };
static const probe_pad_obj_t probe_bottom_rail_gnd_obj = { .base = { &probe_pad_type }, .value = 126 };

// Arduino Nano pin constants
static const node_obj_t node_d0_obj = { .base = { &node_type }, .value = 70 };
static const node_obj_t node_d1_obj = { .base = { &node_type }, .value = 71 };
static const node_obj_t node_d2_obj = { .base = { &node_type }, .value = 72 };
static const node_obj_t node_d3_obj = { .base = { &node_type }, .value = 73 };
static const node_obj_t node_d4_obj = { .base = { &node_type }, .value = 74 };
static const node_obj_t node_d5_obj = { .base = { &node_type }, .value = 75 };
static const node_obj_t node_d6_obj = { .base = { &node_type }, .value = 76 };
static const node_obj_t node_d7_obj = { .base = { &node_type }, .value = 77 };
static const node_obj_t node_d8_obj = { .base = { &node_type }, .value = 78 };
static const node_obj_t node_d9_obj = { .base = { &node_type }, .value = 79 };
static const node_obj_t node_d10_obj = { .base = { &node_type }, .value = 80 };
static const node_obj_t node_d11_obj = { .base = { &node_type }, .value = 81 };
static const node_obj_t node_d12_obj = { .base = { &node_type }, .value = 82 };
static const node_obj_t node_d13_obj = { .base = { &node_type }, .value = 83 };
static const node_obj_t node_a0_obj = { .base = { &node_type }, .value = 86 };
static const node_obj_t node_a1_obj = { .base = { &node_type }, .value = 87 };
static const node_obj_t node_a2_obj = { .base = { &node_type }, .value = 88 };
static const node_obj_t node_a3_obj = { .base = { &node_type }, .value = 89 };
static const node_obj_t node_a4_obj = { .base = { &node_type }, .value = 90 };
static const node_obj_t node_a5_obj = { .base = { &node_type }, .value = 91 };
static const node_obj_t node_a6_obj = { .base = { &node_type }, .value = 92 };
static const node_obj_t node_a7_obj = { .base = { &node_type }, .value = 93 };

// GPIO pin constants
static const node_obj_t node_gpio1_obj = { .base = { &node_type }, .value = 131 };
static const node_obj_t node_gpio2_obj = { .base = { &node_type }, .value = 132 };
static const node_obj_t node_gpio3_obj = { .base = { &node_type }, .value = 133 };
static const node_obj_t node_gpio4_obj = { .base = { &node_type }, .value = 134 };
static const node_obj_t node_gpio5_obj = { .base = { &node_type }, .value = 135 };
static const node_obj_t node_gpio6_obj = { .base = { &node_type }, .value = 136 };
static const node_obj_t node_gpio7_obj = { .base = { &node_type }, .value = 137 };
static const node_obj_t node_gpio8_obj = { .base = { &node_type }, .value = 138 };
// UART pins
static const node_obj_t node_uart_tx_obj = { .base = { &node_type }, .value = 116 };
static const node_obj_t node_uart_rx_obj = { .base = { &node_type }, .value = 117 };

// ADC pins  
static const node_obj_t node_adc0_obj = { .base = { &node_type }, .value = 110 };
static const node_obj_t node_adc1_obj = { .base = { &node_type }, .value = 111 };
static const node_obj_t node_adc2_obj = { .base = { &node_type }, .value = 112 };
static const node_obj_t node_adc3_obj = { .base = { &node_type }, .value = 113 };
static const node_obj_t node_adc4_obj = { .base = { &node_type }, .value = 114 };
static const node_obj_t node_adc7_obj = { .base = { &node_type }, .value = 115 };

// Current sense pins
static const node_obj_t node_isense_plus_obj = { .base = { &node_type }, .value = 108 };
static const node_obj_t node_isense_minus_obj = { .base = { &node_type }, .value = 109 };

// Buffer pins
static const node_obj_t node_buffer_in_obj = { .base = { &node_type }, .value = 139 };
static const node_obj_t node_buffer_out_obj = { .base = { &node_type }, .value = 140 };

// GPIO State constants
static const gpio_state_obj_t gpio_state_high_obj = { .base = { &gpio_state_type }, .value = GPIO_STATE_HIGH };
static const gpio_state_obj_t gpio_state_low_obj = { .base = { &gpio_state_type }, .value = GPIO_STATE_LOW };
static const gpio_state_obj_t gpio_state_floating_obj = { .base = { &gpio_state_type }, .value = GPIO_STATE_FLOATING };

// GPIO Direction constants (INPUT=0, OUTPUT=1)
static const gpio_direction_obj_t gpio_direction_input_obj = { .base = { &gpio_direction_type }, .value = false };
static const gpio_direction_obj_t gpio_direction_output_obj = { .base = { &gpio_direction_type }, .value = true };

// Helper function to get direction value from various input types
static int get_direction_value(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        // Integer: 0=INPUT, 1=OUTPUT
        return mp_obj_get_int(obj) ? 1 : 0;
    } else if (mp_obj_is_bool(obj)) {
        // Boolean: False=INPUT, True=OUTPUT
        return mp_obj_is_true(obj) ? 1 : 0;
    } else if (mp_obj_is_str(obj)) {
        // String: "INPUT"/"OUTPUT" (case insensitive)
        const char* str = mp_obj_str_get_str(obj);
        if (strcmp(str, "OUTPUT") == 0 || strcmp(str, "output") == 0 || strcmp(str, "OUT") == 0 || strcmp(str, "out") == 0) {
            return 1;
        } else if (strcmp(str, "INPUT") == 0 || strcmp(str, "input") == 0 || strcmp(str, "IN") == 0 || strcmp(str, "in") == 0) {
            return 0;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("Direction string must be 'INPUT' or 'OUTPUT'"));
        }
    } else if (mp_obj_get_type(obj) == &gpio_direction_type) {
        // GPIO Direction object
        gpio_direction_obj_t *dir = MP_OBJ_TO_PTR(obj);
        return dir->value ? 1 : 0;
    } else {
        // Try to convert to int as fallback
        return mp_obj_get_int(obj) ? 1 : 0;
    }
}

// Helper function to get pull value from various input types
static int get_pull_value(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        // Integer: 0=NONE, 1=PULLUP, 2=PULLDOWN
        return mp_obj_get_int(obj);
    } else if (mp_obj_is_bool(obj)) {
        // Boolean: False=NONE, True=PULLUP
        return mp_obj_is_true(obj) ? 1 : 0;
    } else if (mp_obj_is_str(obj)) {
        // String: "NONE"/"PULLUP"/"PULLDOWN" (case insensitive)
        const char* str = mp_obj_str_get_str(obj);
        if (strcmp(str, "PULLUP") == 0 || strcmp(str, "pullup") == 0 || strcmp(str, "UP") == 0 || strcmp(str, "up") == 0) {
            return 1;
        } else if (strcmp(str, "PULLDOWN") == 0 || strcmp(str, "pulldown") == 0 || strcmp(str, "DOWN") == 0 || strcmp(str, "down") == 0) {
            return 2;
        } else if (strcmp(str, "NONE") == 0 || strcmp(str, "none") == 0 || strcmp(str, "OFF") == 0 || strcmp(str, "off") == 0) {
            return 0;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("Pull string must be 'NONE', 'PULLUP', or 'PULLDOWN'"));
        }
    } else {
        // Try to convert to int as fallback
        return mp_obj_get_int(obj);
    }
}

// Helper function to get GPIO state value from various input types
static int get_gpio_state_value(mp_obj_t obj) {
    if (mp_obj_is_int(obj)) {
        // Integer: 0=LOW, 1=HIGH, 2=FLOATING
        return mp_obj_get_int(obj);
    } else if (mp_obj_is_bool(obj)) {
        // Boolean: False=LOW, True=HIGH
        return mp_obj_is_true(obj) ? 1 : 0;
    } else if (mp_obj_is_str(obj)) {
        // String: "HIGH"/"LOW"/"FLOATING" (case insensitive)
        const char* str = mp_obj_str_get_str(obj);
        if (strcmp(str, "HIGH") == 0 || strcmp(str, "high") == 0 || strcmp(str, "1") == 0) {
            return 1;
        } else if (strcmp(str, "LOW") == 0 || strcmp(str, "low") == 0 || strcmp(str, "0") == 0) {
            return 0;
        } else if (strcmp(str, "FLOATING") == 0 || strcmp(str, "floating") == 0 || strcmp(str, "Z") == 0 || strcmp(str, "z") == 0) {
            return 2;
        } else {
            mp_raise_ValueError(MP_ERROR_TEXT("GPIO state string must be 'HIGH', 'LOW', or 'FLOATING'"));
        }
    } else if (mp_obj_get_type(obj) == &gpio_state_type) {
        // GPIO State object
        gpio_state_obj_t *state = MP_OBJ_TO_PTR(obj);
        return state->value;
    } else {
        // Try to convert to int as fallback
        return mp_obj_get_int(obj) ? 1 : 0;
    }
}


// Nodes Help Function
static mp_obj_t jl_help_nodes_func(void) {
    mp_printf(&mp_plat_print, "Jumperless Node Reference\n");
    mp_printf(&mp_plat_print, "========================\n\n");
    
    mp_printf(&mp_plat_print, "NODE TYPES:\n");
    mp_printf(&mp_plat_print, "  Numbered:     1-60 (breadboard)\n");
    mp_printf(&mp_plat_print, "  Arduino:      D0-D13, A0-A7 (nano header)\n");
    mp_printf(&mp_plat_print, "  GPIO:         GPIO_1-GPIO_8 (routable GPIO)\n");
    mp_printf(&mp_plat_print, "  Power:        TOP_RAIL, BOTTOM_RAIL, GND\n");
    mp_printf(&mp_plat_print, "  DAC:          DAC0, DAC1 (analog outputs)\n");
    mp_printf(&mp_plat_print, "  ADC:          ADC0-ADC4, PROBE (analog inputs)\n");
    mp_printf(&mp_plat_print, "  Current:      ISENSE_PLUS, ISENSE_MINUS\n");
    mp_printf(&mp_plat_print, "  UART:         UART_TX, UART_RX\n");
    mp_printf(&mp_plat_print, "  Buffer:       BUFFER_IN, BUFFER_OUT\n\n");
    
    mp_printf(&mp_plat_print, "THREE WAYS TO USE NODES:\n\n");
    
    mp_printf(&mp_plat_print, "1. NUMBERS (direct breadboard holes):\n");
    mp_printf(&mp_plat_print, "   connect(1, 30)                     # Connect holes 1 and 30\n");
    mp_printf(&mp_plat_print, "   connect(15, 42)                    # Any number 1-60\n\n");
    
    mp_printf(&mp_plat_print, "2. STRINGS (case-insensitive names):\n");
    mp_printf(&mp_plat_print, "   connect(\"D13\", \"TOP_RAIL\")         # Arduino pin to power rail\n");
    mp_printf(&mp_plat_print, "   connect(\"gpio_1\", \"adc0\")          # GPIO to ADC (case-insensitive)\n");
    mp_printf(&mp_plat_print, "   connect(\"15\", \"dac1\")              # Mix numbers and names\n\n");
    
    mp_printf(&mp_plat_print, "3. CONSTANTS (pre-defined objects):\n");
    mp_printf(&mp_plat_print, "   connect(TOP_RAIL, D13)            # Using imported constants\n");
    mp_printf(&mp_plat_print, "   connect(GPIO_1, A0)               # No quotes needed\n");
    mp_printf(&mp_plat_print, "   connect(DAC0, 25)                 # Mix constants and numbers\n\n");
    
    mp_printf(&mp_plat_print, "MIXED USAGE:\n");
    mp_printf(&mp_plat_print, "   my_pin = node(\"D13\")              # Create node object from string\n");
    mp_printf(&mp_plat_print, "   connect(my_pin, TOP_RAIL)         # Use node object with constant\n");
    mp_printf(&mp_plat_print, "   oled_print(my_pin)                # Display shows 'D13'\n\n");
    
    mp_printf(&mp_plat_print, "COMMON ALIASES (many names work for same node):\n");
    mp_printf(&mp_plat_print, "   \"TOP_RAIL\" = \"T_R\"\n");
    mp_printf(&mp_plat_print, "   \"GPIO_1\" = \"GPIO1\" = \"GP1\"\n");
    mp_printf(&mp_plat_print, "   \"DAC0\" = \"DAC_0\"\n");
    mp_printf(&mp_plat_print, "   \"UART_TX\" = \"TX\"\n\n");
    


    mp_printf(&mp_plat_print, "NOTES:\n");
    mp_printf(&mp_plat_print, "  - String names are case-insensitive: \"d13\" = \"D13\" = \"nAnO_d13\"\n");
    mp_printf(&mp_plat_print, "  - Constants are case-sensitive: use D13, not d13\n");
    mp_printf(&mp_plat_print, "  - All three methods work in any function\n");
    // mp_printf(&mp_plat_print, "  - Use 'from jumperless_nodes import *' for global constants\n\n");
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_help_nodes_obj, jl_help_nodes_func);

// Help Function
static mp_obj_t jl_help_func(size_t n_args, const mp_obj_t *args) {
    if (n_args == 0) {
        // Show all sections
        mp_printf(&mp_plat_print, "Jumperless Native MicroPython Module\n");
        // mp_printf(&mp_plat_print, "Hardware Control Functions with Formatted Output:\n");
        // mp_printf(&mp_plat_print, "(GPIO functions return formatted strings like HIGH/LOW, INPUT/OUTPUT, PULLUP/NONE, CONNECTED/DISCONNECTED)\n\n");
        jl_cycle_term_color(true, 5.0, 1);
        mp_printf(&mp_plat_print, "Available help sections:\n\n");
          
        mp_printf(&mp_plat_print, "  help() or help(\"all\")     - Show all functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"DAC\")              - DAC functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"ADC\")              - ADC functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"GPIO\")             - GPIO functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"PWM\")              - PWM functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"INA\")              - INA current/power monitor\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"NODES\")            - Node connections\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"OLED\")             - OLED display\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"PROBE\")            - Probe and button functions\n");
        // jl_cycle_term_color(false, 100.0, 1); 
        // mp_printf(&mp_plat_print, "  help(\"CLICKWHEEL\")       - Clickwheel functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"STATUS\")           - Status and debug functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"FILESYSTEM\")       - Filesystem functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        // mp_printf(&mp_plat_print, "  help(\"CORE2\")            - Core2 control functions\n");
        mp_printf(&mp_plat_print, "  help(\"MISC\")             - Miscellaneous functions\n");
        jl_cycle_term_color(false, 100.0, 1); 
        mp_printf(&mp_plat_print, "  help(\"EXAMPLES\")         - Usage examples\n\n");
        
        // Show all sections with colors
        jl_help_section("DAC");
        jl_help_section("ADC");
        jl_help_section("GPIO");
        jl_help_section("PWM");
        jl_help_section("INA");
        jl_help_section("NODES");
        jl_help_section("OLED");
        jl_help_section("PROBE");
        // jl_help_section("CLICKWHEEL");
        jl_help_section("STATUS");
        jl_help_section("FILESYSTEM");
        jl_help_section("MISC");
        jl_help_section("EXAMPLES");
    } else {
        // Show specific section
        const char* section = mp_obj_str_get_str(args[0]);
        jl_help_section(section);
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_help_obj, 0, 1, jl_help_func);

// Help section function implementation
void jl_help_section(const char* section) {
    // Color cycling for different sections (disabled for now)
    static int color_index = 0;
    int colors[] = {31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45}; // ANSI colors
    int color = colors[color_index % 15];
    color_index++;
    jl_cycle_term_color(true, 5.0, 1);
            
    // Convert section to uppercase for comparison
    char section_upper[32];
    strcpy(section_upper, section);
    for (int i = 0; section_upper[i]; i++) {
        section_upper[i] = toupper(section_upper[i]);
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "DAC") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "DAC (Digital-to-Analog Converter):\n\n");
        mp_printf(&mp_plat_print, "   dac_set(channel, voltage)         - Set DAC output voltage\n");
        mp_printf(&mp_plat_print, "   dac_get(channel)                  - Get DAC output voltage\n");
        mp_printf(&mp_plat_print, "   set_dac(channel, voltage)         - Alias for dac_set\n");
        mp_printf(&mp_plat_print, "   get_dac(channel)                  - Alias for dac_get\n\n");
        mp_printf(&mp_plat_print, "          channel: 0-3, DAC0, DAC1, TOP_RAIL, BOTTOM_RAIL\n");    
        mp_printf(&mp_plat_print, "          channel 0/DAC0: DAC 0\n");    
        mp_printf(&mp_plat_print, "          channel 1/DAC1: DAC 1\n");   
        mp_printf(&mp_plat_print, "          channel 2/TOP_RAIL: top rail\n");    
        mp_printf(&mp_plat_print, "          channel 3/BOTTOM_RAIL: bottom rail\n");    
        mp_printf(&mp_plat_print, "          voltage: -8.0 to 8.0V\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    
    if (strcmp(section_upper, "ADC") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "ADC (Analog-to-Digital Converter):\n\n");
        mp_printf(&mp_plat_print, "   adc_get(channel)                  - Read ADC input voltage\n");
        mp_printf(&mp_plat_print, "   get_adc(channel)                  - Alias for adc_get\n\n");
        mp_printf(&mp_plat_print, "                                              channel: 0-4\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "GPIO") == 0 || strcmp(section_upper, "ALL") == 0) {

        mp_printf(&mp_plat_print, "GPIO:\n\n");
        mp_printf(&mp_plat_print, "   gpio_set(pin, value)             - Set GPIO pin state\n");
        mp_printf(&mp_plat_print, "   gpio_get(pin)                    - Read GPIO pin state\n");
        mp_printf(&mp_plat_print, "   gpio_set_dir(pin, direction)     - Set GPIO pin direction\n");
        mp_printf(&mp_plat_print, "   gpio_get_dir(pin)                - Get GPIO pin direction\n");
        mp_printf(&mp_plat_print, "   gpio_set_pull(pin, pull)         - Set GPIO pull-up/down\n");
        mp_printf(&mp_plat_print, "   gpio_get_pull(pin)               - Get GPIO pull-up/down\n\n");
        mp_printf(&mp_plat_print, "  Aliases: set_gpio, get_gpio, set_gpio_dir, get_gpio_dir, etc.\n\n");
        mp_printf(&mp_plat_print, "            pin 1-8: GPIO 1-8\n");
        mp_printf(&mp_plat_print, "            pin   9: UART Tx\n");
        mp_printf(&mp_plat_print, "            pin  10: UART Rx\n");
        mp_printf(&mp_plat_print, "              value: True/False   for HIGH/LOW\n");
        mp_printf(&mp_plat_print, "          direction: True/False   for OUTPUT/INPUT\n");
        mp_printf(&mp_plat_print, "               pull: -1/0/1       for PULL_DOWN/NONE/PULL_UP\n\n");
    }
        jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "PWM") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "PWM (Pulse Width Modulation):\n\n");
        mp_printf(&mp_plat_print, "   pwm(pin, [frequency], [duty])    - Setup PWM on GPIO pin\n");
        mp_printf(&mp_plat_print, "   pwm_set_duty_cycle(pin, duty)    - Set PWM duty cycle\n");
        mp_printf(&mp_plat_print, "   pwm_set_frequency(pin, freq)     - Set PWM frequency\n");
        mp_printf(&mp_plat_print, "   pwm_stop(pin)                    - Stop PWM on pin\n\n");
        // mp_printf(&mp_plat_print, "  Pin: 1-8 (numeric) or GPIO_1-GPIO_8 (constants)\n");
        // mp_printf(&mp_plat_print, "  Frequency: 10Hz to 62.5MHz, Duty: 0.0 to 1.0\n");
        mp_printf(&mp_plat_print, "  Aliases: set_pwm, set_pwm_duty_cycle, set_pwm_frequency, stop_pwm\n\n");
        mp_printf(&mp_plat_print, "             pin: 1-8       GPIO pins only\n");
        mp_printf(&mp_plat_print, "       frequency: 0.001Hz-62.5MHz default 1000Hz\n");
        mp_printf(&mp_plat_print, "      duty_cycle: 0.0-1.0   default 0.5 (50%%)\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "INA") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "INA (Current/Power Monitor):\n\n");
        mp_printf(&mp_plat_print, "   ina_get_current(sensor)          - Read current in amps\n");
        mp_printf(&mp_plat_print, "   ina_get_voltage(sensor)          - Read shunt voltage\n");
        mp_printf(&mp_plat_print, "   ina_get_bus_voltage(sensor)      - Read bus voltage\n");
        mp_printf(&mp_plat_print, "   ina_get_power(sensor)            - Read power in watts\n\n");
        mp_printf(&mp_plat_print, "  Aliases: get_current, get_voltage, get_bus_voltage, get_power\n\n");
        mp_printf(&mp_plat_print, "             sensor: 0 or 1\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "NODES") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "Node Connections:\n\n");
        mp_printf(&mp_plat_print, "   connect(node1, node2)            - Connect two nodes\n");
        mp_printf(&mp_plat_print, "   disconnect(node1, node2)         - Disconnect nodes\n");
        mp_printf(&mp_plat_print, "   is_connected(node1, node2)       - Check if nodes are connected\n");
        mp_printf(&mp_plat_print, "   nodes_clear()                    - Clear all connections\n\n");
        mp_printf(&mp_plat_print, "         set node2 to -1 to disconnect everything connected to node1\n\n");
    }
    // cycleTermColor(false, 100.0, 1);
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "OLED") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "OLED Display:\n\n");
        mp_printf(&mp_plat_print, "   oled_print(\"text\")               - Display text\n");
        mp_printf(&mp_plat_print, "   oled_clear()                     - Clear display\n");
        mp_printf(&mp_plat_print, "   oled_connect()                   - Connect OLED\n");
        mp_printf(&mp_plat_print, "   oled_disconnect()                - Disconnect OLED\n\n");
    }
  
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "PROBE") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "Probe Functions:\n\n");
        mp_printf(&mp_plat_print, "   probe_read([blocking=True])      - Read probe (default: blocking)\n");
        mp_printf(&mp_plat_print, "   read_probe([blocking=True])      - Read probe (default: blocking)\n");
        mp_printf(&mp_plat_print, "   probe_read_blocking()            - Wait for probe touch (explicit)\n");
        mp_printf(&mp_plat_print, "   probe_read_nonblocking()         - Check probe immediately (explicit)\n");
        mp_printf(&mp_plat_print, "   get_button([blocking=True])      - Get button state (default: blocking)\n");
        mp_printf(&mp_plat_print, "   probe_button([blocking=True])    - Get button state (default: blocking)\n");
        mp_printf(&mp_plat_print, "   probe_button_blocking()          - Wait for button press (explicit)\n");
        mp_printf(&mp_plat_print, "   probe_button_nonblocking()       - Check buttons immediately (explicit)\n\n");
        // mp_printf(&mp_plat_print, "  Touch aliases: probe_wait, wait_probe, probe_touch, wait_touch (always blocking)\n");
        // mp_printf(&mp_plat_print, "  Button aliases: button_read, read_button (parameterized)\n");
        // mp_printf(&mp_plat_print, "  Non-blocking only: check_button, button_check\n");
        mp_printf(&mp_plat_print, "       Touch returns: ProbePad object (1-60, D13_PAD, TOP_RAIL_PAD, LOGO_PAD_TOP, etc.)\n");
        mp_printf(&mp_plat_print, "       Button returns: CONNECT, REMOVE, or NONE (front=connect, rear=remove)\n\n");
    }
    // jl_cycle_term_color(false, 100.0, 1);
    // if (strcmp(section_upper, "CLICKWHEEL") == 0) {
    //     mp_printf(&mp_plat_print, "Clickwheel:\n");
    //     mp_printf(&mp_plat_print, "   clickwheel_up([clicks])          - Scroll up\n");
    //     mp_printf(&mp_plat_print, "   clickwheel_down([clicks])        - Scroll down\n");
    //     mp_printf(&mp_plat_print, "   clickwheel_press()               - Press button\n");
    //     mp_printf(&mp_plat_print, "           clicks: number of steps\n\n");
    // }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "STATUS") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "Status:\n\n");
        mp_printf(&mp_plat_print, "   print_bridges()                  - Print all bridges\n");
        mp_printf(&mp_plat_print, "   print_paths()                    - Print path between nodes\n");
        mp_printf(&mp_plat_print, "   print_crossbars()                - Print crossbar array\n");
        mp_printf(&mp_plat_print, "   print_nets()                     - Print nets\n");
        mp_printf(&mp_plat_print, "   print_chip_status()              - Print chip status\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "FILESYSTEM") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "Filesystem:\n\n");
        mp_printf(&mp_plat_print, "  jfs.open(path, mode)              - Open file\n");
        mp_printf(&mp_plat_print, "  jfs.read(file, size)              - Read from file\n");
        mp_printf(&mp_plat_print, "  jfs.write(file, data)             - Write to file\n");
        mp_printf(&mp_plat_print, "  jfs.close(file)                   - Close file\n");
        mp_printf(&mp_plat_print, "  jfs.exists(path)                  - Check if file exists\n");
        mp_printf(&mp_plat_print, "  jfs.listdir(path)                 - List directory\n");
        mp_printf(&mp_plat_print, "  jfs.mkdir(path)                   - Create directory\n");
        mp_printf(&mp_plat_print, "  jfs.remove(path)                  - Remove file\n");
        mp_printf(&mp_plat_print, "  jfs.rename(from, to)              - Rename file\n");
        mp_printf(&mp_plat_print, "  jfs.info()                        - Get filesystem info\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "MISC") == 0 || strcmp(section_upper, "ALL") == 0) {
        mp_printf(&mp_plat_print, "Misc:\n\n");
        mp_printf(&mp_plat_print, "   arduino_reset()                  - Reset Arduino\n");
        mp_printf(&mp_plat_print, "   pause_core2(bool/int/str)        - Pause/unpause Core2\n");
        mp_printf(&mp_plat_print, "   probe_tap(node)                  - Tap probe on node (unimplemented)\n");
        mp_printf(&mp_plat_print, "   run_app(appName)                 - Run app\n");
        mp_printf(&mp_plat_print, "   format_output(True/False)        - Enable/disable formatted output\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "EXAMPLES") == 0 || strcmp(section_upper, "ALL") == 0) {
         mp_printf(&mp_plat_print, "Examples (all functions available globally):\n\n");
        // mp_printf(&mp_plat_print, "  dac_set(TOP_RAIL, 3.3)                     # Set Top Rail to 3.3V using node\n");
        // mp_printf(&mp_plat_print, "  set_dac(3, 3.3)                            # Same as above using alias\n");
        mp_printf(&mp_plat_print, "  dac_set(DAC0, 5.0)                         # Set DAC0 using node constant\n");
        mp_printf(&mp_plat_print, "  voltage = get_adc(1)                       # Read ADC1 using alias\n");
        mp_printf(&mp_plat_print, "  connect(TOP_RAIL, D13)                     # Connect using constants\n");
        // mp_printf(&mp_plat_print, "  connect(\"TOP_RAIL\", 5)                      # Connect using strings\n");
        mp_printf(&mp_plat_print, "  connect(4, 20)                             # Connect using numbers\n");
        mp_printf(&mp_plat_print, "  top_rail = node(\"TOP_RAIL\")                # Create node object\n");
        mp_printf(&mp_plat_print, "  connect(top_rail, D13)                     # Mix objects and constants\n");
        mp_printf(&mp_plat_print, "  oled_print(\"Fuck you!\")                    # Display text\n");
        mp_printf(&mp_plat_print, "  current = get_current(0)                   # Read current using alias\n");
        mp_printf(&mp_plat_print, "  set_gpio(1, True)                          # Set GPIO pin high using alias\n");
        mp_printf(&mp_plat_print, "  pwm(1, 1000, 0.5)                          # 1kHz PWM, 50%% duty cycle on pin 1\n");
        mp_printf(&mp_plat_print, "  pwm(GPIO_2, 0.5, 0.25)                     # 0.5Hz PWM, 25%% duty (LED blink)\n");
        mp_printf(&mp_plat_print, "  pwm_set_duty_cycle(GPIO_1, 0.75)           # Change to 75%% duty cycle\n");
        // mp_printf(&mp_plat_print, "  pwm_set_frequency(2, 0.1)                 # Very slow 0.1Hz frequency\n");
        mp_printf(&mp_plat_print, "  pwm_stop(GPIO_1)                           # Stop PWM on GPIO_1\n");
        mp_printf(&mp_plat_print, "  pad = probe_read()                         # Wait for probe touch\n");
        mp_printf(&mp_plat_print, "  if pad == 25: print('Touched pad 25!')     # Check specific pad\n");
        // mp_printf(&mp_plat_print, "  if pad == D13_PAD: connect(D13, TOP_RAIL)  # Auto-connect Arduino pin\n");
        // mp_printf(&mp_plat_print, "  if pad == TOP_RAIL_PAD: show_voltage()     # Show rail voltage\n");
        mp_printf(&mp_plat_print, "  if pad == LOGO_PAD_TOP: print('Logo!')     # Check logo pad\n");
        mp_printf(&mp_plat_print, "  button = get_button()                      # Wait for button press (blocking)\n");
        mp_printf(&mp_plat_print, "  if button == CONNECT_BUTTON: ...           # Front button pressed\n");
        mp_printf(&mp_plat_print, "  if button == REMOVE_BUTTON: ...            # Rear button pressed\n");
        mp_printf(&mp_plat_print, "  button = check_button()                    # Check buttons immediately\n");
        mp_printf(&mp_plat_print, "  if button == BUTTON_NONE: print('None')    # No button pressed\n");
        // mp_printf(&mp_plat_print, "  pad = wait_touch()                        # Wait for touch\n");
        // mp_printf(&mp_plat_print, "  btn = check_button()                      # Check button immediately\n");
        // mp_printf(&mp_plat_print, "  if pad == D13_PAD and btn == CONNECT_BUTTON: connect(D13, TOP_RAIL)\n\n");
        // mp_printf(&mp_plat_print, "Note: All functions and constants are available globally!\n");
        // mp_printf(&mp_plat_print, "No need for ' ' prefix in REPL or single commands.\n\n");
    }
    jl_cycle_term_color(false, 100.0, 1);
    if (strcmp(section_upper, "ALL") != 0 && 
        strcmp(section_upper, "DAC") != 0 && strcmp(section_upper, "ADC") != 0 && 
        strcmp(section_upper, "GPIO") != 0 && strcmp(section_upper, "PWM") != 0 && 
        strcmp(section_upper, "INA") != 0 && strcmp(section_upper, "NODES") != 0 && 
        strcmp(section_upper, "OLED") != 0 && strcmp(section_upper, "PROBE") != 0 && 
        strcmp(section_upper, "CLICKWHEEL") != 0 && strcmp(section_upper, "STATUS") != 0 && 
        strcmp(section_upper, "FILESYSTEM") != 0 && strcmp(section_upper, "CORE2") != 0 && 
        strcmp(section_upper, "MISC") != 0 && strcmp(section_upper, "EXAMPLES") != 0) {
        mp_printf(&mp_plat_print, "Unknown help section: %s\n", section);
        mp_printf(&mp_plat_print, "Use help() to see available sections.\n\n");
    }
}

// Filesystem Functions
static mp_obj_t jl_fs_exists_func(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    int exists = jl_fs_exists(path);
    return mp_obj_new_bool(exists);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_fs_exists_obj, jl_fs_exists_func);

static mp_obj_t jl_fs_listdir_func(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    char* result = jl_fs_listdir(path);
    if (result == NULL) {
        return mp_const_none;
    }
    
    // Make a copy since strtok modifies the string
    size_t len = strlen(result);
    char* result_copy = malloc(len + 1);
    if (result_copy == NULL) {
        return mp_const_none;
    }
    strcpy(result_copy, result);
    
    // Parse comma-separated string into Python list
    mp_obj_t list_obj = mp_obj_new_list(0, NULL);
    
    char* token = strtok(result_copy, ",");
    while (token != NULL) {
        // Remove any leading/trailing whitespace if needed
        while (*token == ' ') token++; // Skip leading spaces
        
        mp_obj_t item = mp_obj_new_str(token, strlen(token));
        mp_obj_list_append(list_obj, item);
        
        token = strtok(NULL, ",");
    }
    
    free(result_copy);
    return list_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_fs_listdir_obj, jl_fs_listdir_func);

static mp_obj_t jl_fs_read_file_func(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    char* content = jl_fs_read_file(path);
    if (content == NULL) {
        return mp_const_none;
    }
    mp_obj_t content_obj = mp_obj_new_str(content, strlen(content));
    // Note: caller should free content if needed
    return content_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_fs_read_file_obj, jl_fs_read_file_func);

static mp_obj_t jl_fs_write_file_func(mp_obj_t path_obj, mp_obj_t content_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    const char* content = mp_obj_str_get_str(content_obj);
    int result = jl_fs_write_file(path, content);
    return mp_obj_new_bool(result == 1);
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_fs_write_file_obj, jl_fs_write_file_func);

static mp_obj_t jl_fs_get_current_dir_func(void) {
    char* current_dir = jl_fs_get_current_dir();
    if (current_dir == NULL) {
        return mp_obj_new_str("/", 1);
    }
    mp_obj_t dir_obj = mp_obj_new_str(current_dir, strlen(current_dir));
    return dir_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_fs_get_current_dir_obj, jl_fs_get_current_dir_func);

//==============================================================================
// JFS (Jumperless FileSystem) Module - Comprehensive File I/O
//==============================================================================

// File object type
typedef struct _mp_obj_jfs_file_t {
    mp_obj_base_t base;
    void* file_handle;
    bool is_open;
} mp_obj_jfs_file_t;

// File type declaration
const mp_obj_type_t mp_type_jfs_file;

// File object methods
static mp_obj_t jfs_file_read(size_t n_args, const mp_obj_t *args) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->is_open || !self->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    size_t size = 1024; // Default read size
    if (n_args > 1) {
        size = mp_obj_get_int(args[1]);
    }
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        mp_raise_OSError(12); // ENOMEM
    }
    
    int bytes_read = jl_fs_read_bytes(self->file_handle, buffer, size);
    if (bytes_read < 0) {
        free(buffer);
        mp_raise_OSError(5); // EIO
    }
    
    buffer[bytes_read] = '\0';
    mp_obj_t result = mp_obj_new_str(buffer, bytes_read);
    free(buffer);
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_file_read_obj, 1, 2, jfs_file_read);

static mp_obj_t jfs_file_write(mp_obj_t self_in, mp_obj_t data_obj) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_open || !self->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    const char* data = mp_obj_str_get_str(data_obj);
    size_t len = strlen(data);
    
    int bytes_written = jl_fs_write_bytes(self->file_handle, data, len);
    if (bytes_written < 0) {
        mp_raise_OSError(5); // EIO
    }
    
    return mp_obj_new_int(bytes_written);
}
static MP_DEFINE_CONST_FUN_OBJ_2(jfs_file_write_obj, jfs_file_write);

static mp_obj_t jfs_file_seek(size_t n_args, const mp_obj_t *args) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(args[0]);
    if (!self->is_open || !self->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int position = mp_obj_get_int(args[1]);
    int whence = 0; // Default to SEEK_SET
    if (n_args > 2) {
        whence = mp_obj_get_int(args[2]);
    }
    
    int result = jl_fs_seek(self->file_handle, position, whence);
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_file_seek_obj, 2, 3, jfs_file_seek);

static mp_obj_t jfs_file_tell(mp_obj_t self_in) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_open || !self->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int position = jl_fs_position(self->file_handle);
    return mp_obj_new_int(position);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_tell_obj, jfs_file_tell);

static mp_obj_t jfs_file_size(mp_obj_t self_in) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_open || !self->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int size = jl_fs_size(self->file_handle);
    return mp_obj_new_int(size);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_size_obj, jfs_file_size);

static mp_obj_t jfs_file_available(mp_obj_t self_in) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_open || !self->file_handle) {
        return mp_obj_new_int(0);
    }
    
    int available = jl_fs_available(self->file_handle);
    return mp_obj_new_int(available);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_available_obj, jfs_file_available);

static mp_obj_t jfs_file_name(mp_obj_t self_in) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (!self->is_open || !self->file_handle) {
        return mp_const_none;
    }
    
    char* name = jl_fs_name(self->file_handle);
    if (!name) {
        return mp_const_none;
    }
    
    return mp_obj_new_str(name, strlen(name));
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_name_obj, jfs_file_name);

static mp_obj_t jfs_file_close(mp_obj_t self_in) {
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(self_in);
    if (self->is_open && self->file_handle) {
        jl_fs_close_file(self->file_handle);
        self->file_handle = NULL;
        self->is_open = false;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_close_obj, jfs_file_close);

// Context manager methods for 'with' statement support
static mp_obj_t jfs_file_enter(mp_obj_t self_in) {
    // Just return self for context manager
    return self_in;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_file_enter_obj, jfs_file_enter);

static mp_obj_t jfs_file_exit(size_t n_args, const mp_obj_t *args) {
    // args[0] is self, args[1-3] are exception info (exc_type, exc_val, exc_tb)
    mp_obj_jfs_file_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Always close the file when exiting context
    if (self->is_open && self->file_handle) {
        jl_fs_close_file(self->file_handle);
        self->file_handle = NULL;
        self->is_open = false;
    }
    
    // Return False to not suppress any exceptions
    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_file_exit_obj, 4, 4, jfs_file_exit);

// File object locals dict
static const mp_rom_map_elem_t jfs_file_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&jfs_file_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&jfs_file_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&jfs_file_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&jfs_file_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_position), MP_ROM_PTR(&jfs_file_tell_obj) }, // Alias
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&jfs_file_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_available), MP_ROM_PTR(&jfs_file_available_obj) },
    { MP_ROM_QSTR(MP_QSTR_name), MP_ROM_PTR(&jfs_file_name_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&jfs_file_close_obj) },
    
    // Context manager methods for 'with' statement support
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&jfs_file_enter_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&jfs_file_exit_obj) },
};
static MP_DEFINE_CONST_DICT(jfs_file_locals_dict, jfs_file_locals_dict_table);

// File type definition
MP_DEFINE_CONST_OBJ_TYPE(
    mp_type_jfs_file,
    MP_QSTR_JFSFile,
    MP_TYPE_FLAG_NONE,
    locals_dict, &jfs_file_locals_dict
);

// JFS module functions
static mp_obj_t jfs_open(size_t n_args, const mp_obj_t *args) {
    const char* path = mp_obj_str_get_str(args[0]);
    const char* mode = "r"; // Default mode
    if (n_args > 1) {
        mode = mp_obj_str_get_str(args[1]);
    }
    
    void* file_handle = jl_fs_open_file(path, mode);
    if (!file_handle) {
        mp_raise_OSError(2); // ENOENT
    }
    
    mp_obj_jfs_file_t *file_obj = m_new_obj(mp_obj_jfs_file_t);
    file_obj->base.type = &mp_type_jfs_file;
    file_obj->file_handle = file_handle;
    file_obj->is_open = true;
    
    return MP_OBJ_FROM_PTR(file_obj);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_open_obj, 1, 2, jfs_open);

static mp_obj_t jfs_exists(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    int exists = jl_fs_exists(path);
    return mp_obj_new_bool(exists);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_exists_obj, jfs_exists);

static mp_obj_t jfs_listdir(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    char* result = jl_fs_listdir(path);
    if (result == NULL) {
        return mp_obj_new_list(0, NULL);
    }
    
    // Parse comma-separated string into Python list
    mp_obj_t list_obj = mp_obj_new_list(0, NULL);
    
    size_t len = strlen(result);
    char* result_copy = malloc(len + 1);
    if (result_copy == NULL) {
        return mp_obj_new_list(0, NULL);
    }
    strcpy(result_copy, result);
    
    char* token = strtok(result_copy, ",");
    while (token != NULL) {
        while (*token == ' ') token++; // Skip leading spaces
        mp_obj_t item = mp_obj_new_str(token, strlen(token));
        mp_obj_list_append(list_obj, item);
        token = strtok(NULL, ",");
    }
    
    free(result_copy);
    return list_obj;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_listdir_obj, jfs_listdir);

static mp_obj_t jfs_mkdir(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    int result = jl_fs_mkdir(path);
    if (!result) {
        mp_raise_OSError(2); // ENOENT - failed to create directory
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_mkdir_obj, jfs_mkdir);

static mp_obj_t jfs_rmdir(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    int result = jl_fs_rmdir(path);
    if (!result) {
        mp_raise_OSError(2); // ENOENT - failed to remove directory
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_rmdir_obj, jfs_rmdir);

static mp_obj_t jfs_remove(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    int result = jl_fs_remove(path);
    if (!result) {
        mp_raise_OSError(2); // ENOENT - failed to remove file
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_remove_obj, jfs_remove);

static mp_obj_t jfs_rename(mp_obj_t from_obj, mp_obj_t to_obj) {
    const char* from_path = mp_obj_str_get_str(from_obj);
    const char* to_path = mp_obj_str_get_str(to_obj);
    int result = jl_fs_rename(from_path, to_path);
    if (!result) {
        mp_raise_OSError(2); // ENOENT - failed to rename file
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jfs_rename_obj, jfs_rename);

static mp_obj_t jfs_stat(mp_obj_t path_obj) {
    const char* path = mp_obj_str_get_str(path_obj);
    
    // Simple stat implementation - just check if file exists and get basic info
    if (!jl_fs_exists(path)) {
        mp_raise_OSError(2); // ENOENT
    }
    
    // Try to open file to get size info
    void* file_handle = jl_fs_open_file(path, "r");
    int size = 0;
    if (file_handle) {
        size = jl_fs_size(file_handle);
        jl_fs_close_file(file_handle);
    }
    
    // Return a simple tuple with (mode, ino, dev, nlink, uid, gid, size, atime, mtime, ctime)
    mp_obj_t tuple[10];
    tuple[0] = mp_obj_new_int(0x8000); // S_IFREG - regular file
    tuple[1] = mp_obj_new_int(0); // inode
    tuple[2] = mp_obj_new_int(0); // device
    tuple[3] = mp_obj_new_int(1); // nlink
    tuple[4] = mp_obj_new_int(0); // uid
    tuple[5] = mp_obj_new_int(0); // gid
    tuple[6] = mp_obj_new_int(size); // size
    tuple[7] = mp_obj_new_int(0); // atime
    tuple[8] = mp_obj_new_int(0); // mtime
    tuple[9] = mp_obj_new_int(0); // ctime
    
    return mp_obj_new_tuple(10, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_stat_obj, jfs_stat);

static mp_obj_t jfs_info(void) {
    int total = jl_fs_total_bytes();
    int used = jl_fs_used_bytes();
    int free = total - used;
    
    mp_obj_t tuple[3];
    tuple[0] = mp_obj_new_int(total);
    tuple[1] = mp_obj_new_int(used);
    tuple[2] = mp_obj_new_int(free);
    
    return mp_obj_new_tuple(3, tuple);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jfs_info_obj, jfs_info);

// File handle operations as module functions
static mp_obj_t jfs_read(size_t n_args, const mp_obj_t *args) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(args[0]);
    if (!file->is_open || !file->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    size_t size = 1024; // Default read size
    if (n_args > 1) {
        size = mp_obj_get_int(args[1]);
    }
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        mp_raise_OSError(12); // ENOMEM
    }
    
    int bytes_read = jl_fs_read_bytes(file->file_handle, buffer, size);
    if (bytes_read < 0) {
        free(buffer);
        mp_raise_OSError(5); // EIO
    }
    
    buffer[bytes_read] = '\0';
    mp_obj_t result = mp_obj_new_str(buffer, bytes_read);
    free(buffer);
    return result;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_read_obj, 1, 2, jfs_read);

static mp_obj_t jfs_write(mp_obj_t file_obj, mp_obj_t data_obj) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(file_obj);
    if (!file->is_open || !file->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    const char* data = mp_obj_str_get_str(data_obj);
    size_t len = strlen(data);
    
    int bytes_written = jl_fs_write_bytes(file->file_handle, data, len);
    if (bytes_written < 0) {
        mp_raise_OSError(5); // EIO
    }
    
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jfs_write_obj, jfs_write);

static mp_obj_t jfs_close(mp_obj_t file_obj) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(file_obj);
    if (file->is_open && file->file_handle) {
        jl_fs_close_file(file->file_handle);
        file->file_handle = NULL;
        file->is_open = false;
    }
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_close_obj, jfs_close);

static mp_obj_t jfs_seek(size_t n_args, const mp_obj_t *args) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(args[0]);
    if (!file->is_open || !file->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int position = mp_obj_get_int(args[1]);
    int whence = 0; // Default to SEEK_SET
    if (n_args > 2) {
        whence = mp_obj_get_int(args[2]);
    }
    
    int result = jl_fs_seek(file->file_handle, position, whence);
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jfs_seek_obj, 2, 3, jfs_seek);

static mp_obj_t jfs_tell(mp_obj_t file_obj) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(file_obj);
    if (!file->is_open || !file->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int position = jl_fs_position(file->file_handle);
    return mp_obj_new_int(position);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_tell_obj, jfs_tell);

static mp_obj_t jfs_size(mp_obj_t file_obj) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(file_obj);
    if (!file->is_open || !file->file_handle) {
        mp_raise_ValueError("I/O operation on closed file");
    }
    
    int size = jl_fs_size(file->file_handle);
    return mp_obj_new_int(size);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_size_obj, jfs_size);

static mp_obj_t jfs_available(mp_obj_t file_obj) {
    mp_obj_jfs_file_t *file = MP_OBJ_TO_PTR(file_obj);
    if (!file->is_open || !file->file_handle) {
        return mp_obj_new_int(0);
    }
    
    int available = jl_fs_available(file->file_handle);
    return mp_obj_new_int(available);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jfs_available_obj, jfs_available);

// JFS module globals
static const mp_rom_map_elem_t jfs_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jfs) },
    
    // File operations
    { MP_ROM_QSTR(MP_QSTR_open), MP_ROM_PTR(&jfs_open_obj) },
    
    // File handle operations (module-level functions)
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&jfs_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&jfs_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&jfs_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&jfs_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&jfs_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&jfs_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_available), MP_ROM_PTR(&jfs_available_obj) },
    
    // Directory operations
    { MP_ROM_QSTR(MP_QSTR_exists), MP_ROM_PTR(&jfs_exists_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir), MP_ROM_PTR(&jfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir), MP_ROM_PTR(&jfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir), MP_ROM_PTR(&jfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove), MP_ROM_PTR(&jfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename), MP_ROM_PTR(&jfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat), MP_ROM_PTR(&jfs_stat_obj) },
    
    // Filesystem info
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&jfs_info_obj) },
    
    // Constants
    { MP_ROM_QSTR(MP_QSTR_SEEK_SET), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_SEEK_CUR), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_SEEK_END), MP_ROM_INT(2) },
};
static MP_DEFINE_CONST_DICT(jfs_module_globals, jfs_module_globals_table);

const mp_obj_module_t jfs_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&jfs_module_globals,
};

// Function to get current slot for MicroPython
extern int netSlot; // Reference to global netSlot variable

static mp_obj_t jl_get_current_slot(void) {
    return mp_obj_new_int(netSlot);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_get_current_slot_obj, jl_get_current_slot);


//==============================================================================
// Missing MicroPython VFS bridge functions for os module support
//==============================================================================

// Import stat function for module importing
mp_import_stat_t mp_import_stat(const char *path) {
    if (jl_fs_exists(path)) {
        return MP_IMPORT_STAT_FILE;
    }
    return MP_IMPORT_STAT_NO_EXIST;
}

// Simple VFS open function stub for basic file operations
mp_obj_t mp_vfs_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs) {
    // For now, just return None - we can implement file objects later if needed
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_vfs_open_obj, 1, mp_vfs_open);

// Register the modules with MicroPython
MP_REGISTER_MODULE(MP_QSTR_jumperless, jumperless_user_cmodule);
MP_REGISTER_MODULE(MP_QSTR_jfs, jfs_user_cmodule); 

// Lexer function to read Python files for import system
mp_lexer_t *mp_lexer_new_from_file(qstr filename) {
    // Convert qstr to C string
    const char *path = qstr_str(filename);
    
    // Read file contents using our filesystem bridge
    char *content = jl_fs_read_file(path);
    if (content == NULL) {
        mp_raise_OSError(MP_ENOENT);
    }
    
    // Create lexer from the file contents
    // The lexer will take ownership of the content string
    mp_lexer_t *lex = mp_lexer_new_from_str_len(filename, content, strlen(content), 0);
    
    // Note: content should not be freed here as lexer now owns it
    // MicroPython will handle freeing when the lexer is destroyed
    return lex;
}

//=============================================================================
// Enhanced Logic Analyzer Functions
//=============================================================================

// Logic Analyzer Trigger Control
static mp_obj_t jl_la_set_trigger_func(size_t n_args, const mp_obj_t *args) {
    int trigger_type = mp_obj_get_int(args[0]);
    int channel = mp_obj_get_int(args[1]);
    float value = mp_obj_get_float(args[2]);
    
    bool result = jl_la_set_trigger(trigger_type, channel, value);
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_la_set_trigger_obj, 3, 3, jl_la_set_trigger_func);

// Single Sample Capture
static mp_obj_t jl_la_capture_single_sample_func(void) {
    bool result = jl_la_capture_single_sample();
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_la_capture_single_sample_obj, jl_la_capture_single_sample_func);

// Continuous Capture Control
static mp_obj_t jl_la_start_continuous_capture_func(void) {
    bool result = jl_la_start_continuous_capture();
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_la_start_continuous_capture_obj, jl_la_start_continuous_capture_func);

static mp_obj_t jl_la_stop_capture_func(void) {
    bool result = jl_la_stop_capture();
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_la_stop_capture_obj, jl_la_stop_capture_func);

static mp_obj_t jl_la_is_capturing_func(void) {
    bool result = jl_la_is_capturing();
    return mp_obj_new_bool(result);
}
static MP_DEFINE_CONST_FUN_OBJ_0(jl_la_is_capturing_obj, jl_la_is_capturing_func);

// Configuration Functions
static mp_obj_t jl_la_set_sample_rate_func(mp_obj_t rate_obj) {
    uint32_t rate = mp_obj_get_int(rate_obj);
    jl_la_set_sample_rate(rate);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_la_set_sample_rate_obj, jl_la_set_sample_rate_func);

static mp_obj_t jl_la_set_num_samples_func(mp_obj_t samples_obj) {
    uint32_t samples = mp_obj_get_int(samples_obj);
    jl_la_set_num_samples(samples);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_la_set_num_samples_obj, jl_la_set_num_samples_func);

static mp_obj_t jl_la_enable_channel_func(size_t n_args, const mp_obj_t *args) {
    int channel_type = mp_obj_get_int(args[0]);
    int channel = mp_obj_get_int(args[1]);
    bool enable = mp_obj_is_true(args[2]);
    
    jl_la_enable_channel(channel_type, channel, enable);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(jl_la_enable_channel_obj, 3, 3, jl_la_enable_channel_func);

// Control Channel Functions
static mp_obj_t jl_la_set_control_analog_func(mp_obj_t channel_obj, mp_obj_t value_obj) {
    int channel = mp_obj_get_int(channel_obj);
    float value = mp_obj_get_float(value_obj);
    
    jl_la_set_control_analog(channel, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_la_set_control_analog_obj, jl_la_set_control_analog_func);

static mp_obj_t jl_la_set_control_digital_func(mp_obj_t channel_obj, mp_obj_t value_obj) {
    int channel = mp_obj_get_int(channel_obj);
    bool value = mp_obj_is_true(value_obj);
    
    jl_la_set_control_digital(channel, value);
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(jl_la_set_control_digital_obj, jl_la_set_control_digital_func);

static mp_obj_t jl_la_get_control_analog_func(mp_obj_t channel_obj) {
    int channel = mp_obj_get_int(channel_obj);
    float value = jl_la_get_control_analog(channel);
    return mp_obj_new_float(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_la_get_control_analog_obj, jl_la_get_control_analog_func);

static mp_obj_t jl_la_get_control_digital_func(mp_obj_t channel_obj) {
    int channel = mp_obj_get_int(channel_obj);
    bool value = jl_la_get_control_digital(channel);
    return mp_obj_new_bool(value);
}
static MP_DEFINE_CONST_FUN_OBJ_1(jl_la_get_control_digital_obj, jl_la_get_control_digital_func);

//=============================================================================
// Module Definition
//=============================================================================




// Module globals table


static const mp_rom_map_elem_t jumperless_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jumperless) },
    
    // Global variables
    { MP_ROM_QSTR(MP_QSTR_CURRENT_SLOT), MP_ROM_PTR(&jl_get_current_slot_obj) },
    
    // Node creation function
    { MP_ROM_QSTR(MP_QSTR_node), MP_ROM_PTR(&jl_node_obj) },
    
    // GPIO State constants  
    { MP_ROM_QSTR(MP_QSTR_HIGH), MP_ROM_PTR(&gpio_state_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_LOW), MP_ROM_PTR(&gpio_state_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_FLOATING), MP_ROM_PTR(&gpio_state_floating_obj) },
    
    // GPIO Direction constants
    { MP_ROM_QSTR(MP_QSTR_INPUT), MP_ROM_PTR(&gpio_direction_input_obj) },
    { MP_ROM_QSTR(MP_QSTR_OUTPUT), MP_ROM_PTR(&gpio_direction_output_obj) },
    
    // Common node constants
    { MP_ROM_QSTR(MP_QSTR_TOP_RAIL), MP_ROM_PTR(&node_top_rail_obj) },
    { MP_ROM_QSTR(MP_QSTR_T_RAIL), MP_ROM_PTR(&node_top_rail_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOTTOM_RAIL), MP_ROM_PTR(&node_bottom_rail_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOT_RAIL), MP_ROM_PTR(&node_bottom_rail_obj) },
    { MP_ROM_QSTR(MP_QSTR_B_RAIL), MP_ROM_PTR(&node_bottom_rail_obj) },
    { MP_ROM_QSTR(MP_QSTR_GND), MP_ROM_PTR(&node_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_DAC0), MP_ROM_PTR(&node_dac0_obj) },
    { MP_ROM_QSTR(MP_QSTR_DAC_0), MP_ROM_PTR(&node_dac0_obj) },
    { MP_ROM_QSTR(MP_QSTR_DAC1), MP_ROM_PTR(&node_dac1_obj) },
    { MP_ROM_QSTR(MP_QSTR_DAC_1), MP_ROM_PTR(&node_dac1_obj) },
    
    // Current sense pins
    { MP_ROM_QSTR(MP_QSTR_ISENSE_PLUS), MP_ROM_PTR(&node_isense_plus_obj) },
    { MP_ROM_QSTR(MP_QSTR_ISENSE_P), MP_ROM_PTR(&node_isense_plus_obj) },
    { MP_ROM_QSTR(MP_QSTR_I_P), MP_ROM_PTR(&node_isense_plus_obj) },
    { MP_ROM_QSTR(MP_QSTR_CURRENT_SENSE_P), MP_ROM_PTR(&node_isense_plus_obj) },
    { MP_ROM_QSTR(MP_QSTR_CURRENT_SENSE_PLUS), MP_ROM_PTR(&node_isense_plus_obj) },
    { MP_ROM_QSTR(MP_QSTR_ISENSE_MINUS), MP_ROM_PTR(&node_isense_minus_obj) },
    { MP_ROM_QSTR(MP_QSTR_ISENSE_N), MP_ROM_PTR(&node_isense_minus_obj) },
    { MP_ROM_QSTR(MP_QSTR_I_N), MP_ROM_PTR(&node_isense_minus_obj) },
    { MP_ROM_QSTR(MP_QSTR_CURRENT_SENSE_N), MP_ROM_PTR(&node_isense_minus_obj) },
    { MP_ROM_QSTR(MP_QSTR_CURRENT_SENSE_MINUS), MP_ROM_PTR(&node_isense_minus_obj) },
    
    // Buffer pins
    { MP_ROM_QSTR(MP_QSTR_BUFFER_IN), MP_ROM_PTR(&node_buffer_in_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUF_IN), MP_ROM_PTR(&node_buffer_in_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUFFER_OUT), MP_ROM_PTR(&node_buffer_out_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUF_OUT), MP_ROM_PTR(&node_buffer_out_obj) },
    
    // ADC pins
    { MP_ROM_QSTR(MP_QSTR_ADC0), MP_ROM_PTR(&node_adc0_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC1), MP_ROM_PTR(&node_adc1_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC2), MP_ROM_PTR(&node_adc2_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC3), MP_ROM_PTR(&node_adc3_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC4), MP_ROM_PTR(&node_adc4_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC7), MP_ROM_PTR(&node_adc7_obj) },
    
    // UART pins
    { MP_ROM_QSTR(MP_QSTR_UART_TX), MP_ROM_PTR(&node_uart_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_TX), MP_ROM_PTR(&node_uart_tx_obj) },
    { MP_ROM_QSTR(MP_QSTR_UART_RX), MP_ROM_PTR(&node_uart_rx_obj) },
    { MP_ROM_QSTR(MP_QSTR_RX), MP_ROM_PTR(&node_uart_rx_obj) },
    
    // Arduino Nano pins
    { MP_ROM_QSTR(MP_QSTR_D0), MP_ROM_PTR(&node_d0_obj) },
    { MP_ROM_QSTR(MP_QSTR_D1), MP_ROM_PTR(&node_d1_obj) },
    { MP_ROM_QSTR(MP_QSTR_D2), MP_ROM_PTR(&node_d2_obj) },
    { MP_ROM_QSTR(MP_QSTR_D3), MP_ROM_PTR(&node_d3_obj) },
    { MP_ROM_QSTR(MP_QSTR_D4), MP_ROM_PTR(&node_d4_obj) },
    { MP_ROM_QSTR(MP_QSTR_D5), MP_ROM_PTR(&node_d5_obj) },
    { MP_ROM_QSTR(MP_QSTR_D6), MP_ROM_PTR(&node_d6_obj) },
    { MP_ROM_QSTR(MP_QSTR_D7), MP_ROM_PTR(&node_d7_obj) },
    { MP_ROM_QSTR(MP_QSTR_D8), MP_ROM_PTR(&node_d8_obj) },
    { MP_ROM_QSTR(MP_QSTR_D9), MP_ROM_PTR(&node_d9_obj) },
    { MP_ROM_QSTR(MP_QSTR_D10), MP_ROM_PTR(&node_d10_obj) },
    { MP_ROM_QSTR(MP_QSTR_D11), MP_ROM_PTR(&node_d11_obj) },
    { MP_ROM_QSTR(MP_QSTR_D12), MP_ROM_PTR(&node_d12_obj) },
    { MP_ROM_QSTR(MP_QSTR_D13), MP_ROM_PTR(&node_d13_obj) },
    
    // NANO prefixed digital pins
    { MP_ROM_QSTR(MP_QSTR_NANO_D0), MP_ROM_PTR(&node_d0_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D1), MP_ROM_PTR(&node_d1_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D2), MP_ROM_PTR(&node_d2_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D3), MP_ROM_PTR(&node_d3_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D4), MP_ROM_PTR(&node_d4_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D5), MP_ROM_PTR(&node_d5_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D6), MP_ROM_PTR(&node_d6_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D7), MP_ROM_PTR(&node_d7_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D8), MP_ROM_PTR(&node_d8_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D9), MP_ROM_PTR(&node_d9_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D10), MP_ROM_PTR(&node_d10_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D11), MP_ROM_PTR(&node_d11_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D12), MP_ROM_PTR(&node_d12_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_D13), MP_ROM_PTR(&node_d13_obj) },
    
    // Arduino analog pins
    { MP_ROM_QSTR(MP_QSTR_A0), MP_ROM_PTR(&node_a0_obj) },
    { MP_ROM_QSTR(MP_QSTR_A1), MP_ROM_PTR(&node_a1_obj) },
    { MP_ROM_QSTR(MP_QSTR_A2), MP_ROM_PTR(&node_a2_obj) },
    { MP_ROM_QSTR(MP_QSTR_A3), MP_ROM_PTR(&node_a3_obj) },
    { MP_ROM_QSTR(MP_QSTR_A4), MP_ROM_PTR(&node_a4_obj) },
    { MP_ROM_QSTR(MP_QSTR_A5), MP_ROM_PTR(&node_a5_obj) },
    { MP_ROM_QSTR(MP_QSTR_A6), MP_ROM_PTR(&node_a6_obj) },
    { MP_ROM_QSTR(MP_QSTR_A7), MP_ROM_PTR(&node_a7_obj) },
    
    // NANO prefixed analog pins
    { MP_ROM_QSTR(MP_QSTR_NANO_A0), MP_ROM_PTR(&node_a0_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A1), MP_ROM_PTR(&node_a1_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A2), MP_ROM_PTR(&node_a2_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A3), MP_ROM_PTR(&node_a3_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A4), MP_ROM_PTR(&node_a4_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A5), MP_ROM_PTR(&node_a5_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A6), MP_ROM_PTR(&node_a6_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_A7), MP_ROM_PTR(&node_a7_obj) },
    
    // GPIO pins with multiple aliases
    { MP_ROM_QSTR(MP_QSTR_GPIO_1), MP_ROM_PTR(&node_gpio1_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_2), MP_ROM_PTR(&node_gpio2_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_3), MP_ROM_PTR(&node_gpio3_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_4), MP_ROM_PTR(&node_gpio4_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_5), MP_ROM_PTR(&node_gpio5_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_6), MP_ROM_PTR(&node_gpio6_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_7), MP_ROM_PTR(&node_gpio7_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_8), MP_ROM_PTR(&node_gpio8_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP1), MP_ROM_PTR(&node_gpio1_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP2), MP_ROM_PTR(&node_gpio2_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP3), MP_ROM_PTR(&node_gpio3_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP4), MP_ROM_PTR(&node_gpio4_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP5), MP_ROM_PTR(&node_gpio5_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP6), MP_ROM_PTR(&node_gpio6_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP7), MP_ROM_PTR(&node_gpio7_obj) },
    { MP_ROM_QSTR(MP_QSTR_GP8), MP_ROM_PTR(&node_gpio8_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_20), MP_ROM_PTR(&node_gpio1_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_21), MP_ROM_PTR(&node_gpio2_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_22), MP_ROM_PTR(&node_gpio3_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_23), MP_ROM_PTR(&node_gpio4_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_24), MP_ROM_PTR(&node_gpio5_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_25), MP_ROM_PTR(&node_gpio6_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_26), MP_ROM_PTR(&node_gpio7_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_27), MP_ROM_PTR(&node_gpio8_obj) },
    
    // Probe button constants
    { MP_ROM_QSTR(MP_QSTR_BUTTON_NONE), MP_ROM_PTR(&probe_button_none_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_CONNECT), MP_ROM_PTR(&probe_button_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUTTON_REMOVE), MP_ROM_PTR(&probe_button_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_CONNECT_BUTTON), MP_ROM_PTR(&probe_button_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_REMOVE_BUTTON), MP_ROM_PTR(&probe_button_remove_obj) },
    
    // Probe pad constants
    { MP_ROM_QSTR(MP_QSTR_NO_PAD), MP_ROM_PTR(&probe_no_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_LOGO_PAD_TOP), MP_ROM_PTR(&probe_logo_pad_top_obj) },
    { MP_ROM_QSTR(MP_QSTR_LOGO_PAD_BOTTOM), MP_ROM_PTR(&probe_logo_pad_bottom_obj) },
    { MP_ROM_QSTR(MP_QSTR_GPIO_PAD), MP_ROM_PTR(&probe_gpio_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_DAC_PAD), MP_ROM_PTR(&probe_dac_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_ADC_PAD), MP_ROM_PTR(&probe_adc_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUILDING_PAD_TOP), MP_ROM_PTR(&probe_building_pad_top_obj) },
    { MP_ROM_QSTR(MP_QSTR_BUILDING_PAD_BOTTOM), MP_ROM_PTR(&probe_building_pad_bottom_obj) },
    
    // Nano power/control pad constants
    { MP_ROM_QSTR(MP_QSTR_NANO_VIN), MP_ROM_PTR(&probe_nano_vin_obj) },
    { MP_ROM_QSTR(MP_QSTR_VIN_PAD), MP_ROM_PTR(&probe_nano_vin_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_RESET_0), MP_ROM_PTR(&probe_nano_reset_0_obj) },
    { MP_ROM_QSTR(MP_QSTR_RESET_0_PAD), MP_ROM_PTR(&probe_nano_reset_0_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_RESET_1), MP_ROM_PTR(&probe_nano_reset_1_obj) },
    { MP_ROM_QSTR(MP_QSTR_RESET_1_PAD), MP_ROM_PTR(&probe_nano_reset_1_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_GND_1), MP_ROM_PTR(&probe_nano_gnd_1_obj) },
    { MP_ROM_QSTR(MP_QSTR_GND_1_PAD), MP_ROM_PTR(&probe_nano_gnd_1_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_GND_0), MP_ROM_PTR(&probe_nano_gnd_0_obj) },
    { MP_ROM_QSTR(MP_QSTR_GND_0_PAD), MP_ROM_PTR(&probe_nano_gnd_0_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_3V3), MP_ROM_PTR(&probe_nano_3v3_obj) },
    { MP_ROM_QSTR(MP_QSTR_3V3_PAD), MP_ROM_PTR(&probe_nano_3v3_obj) },
    { MP_ROM_QSTR(MP_QSTR_NANO_5V), MP_ROM_PTR(&probe_nano_5v_obj) },
    { MP_ROM_QSTR(MP_QSTR_5V_PAD), MP_ROM_PTR(&probe_nano_5v_obj) },
    
    // Nano digital pin pad constants
    { MP_ROM_QSTR(MP_QSTR_D0_PAD), MP_ROM_PTR(&probe_d0_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D1_PAD), MP_ROM_PTR(&probe_d1_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D2_PAD), MP_ROM_PTR(&probe_d2_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D3_PAD), MP_ROM_PTR(&probe_d3_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D4_PAD), MP_ROM_PTR(&probe_d4_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D5_PAD), MP_ROM_PTR(&probe_d5_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D6_PAD), MP_ROM_PTR(&probe_d6_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D7_PAD), MP_ROM_PTR(&probe_d7_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D8_PAD), MP_ROM_PTR(&probe_d8_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D9_PAD), MP_ROM_PTR(&probe_d9_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D10_PAD), MP_ROM_PTR(&probe_d10_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D11_PAD), MP_ROM_PTR(&probe_d11_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D12_PAD), MP_ROM_PTR(&probe_d12_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_D13_PAD), MP_ROM_PTR(&probe_d13_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_RESET_PAD), MP_ROM_PTR(&probe_reset_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_AREF_PAD), MP_ROM_PTR(&probe_aref_pad_obj) },
    
    // Nano analog pin pad constants
    { MP_ROM_QSTR(MP_QSTR_A0_PAD), MP_ROM_PTR(&probe_a0_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A1_PAD), MP_ROM_PTR(&probe_a1_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A2_PAD), MP_ROM_PTR(&probe_a2_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A3_PAD), MP_ROM_PTR(&probe_a3_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A4_PAD), MP_ROM_PTR(&probe_a4_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A5_PAD), MP_ROM_PTR(&probe_a5_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A6_PAD), MP_ROM_PTR(&probe_a6_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_A7_PAD), MP_ROM_PTR(&probe_a7_pad_obj) },
    
    // Rail pad constants
    { MP_ROM_QSTR(MP_QSTR_TOP_RAIL_PAD), MP_ROM_PTR(&probe_top_rail_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOTTOM_RAIL_PAD), MP_ROM_PTR(&probe_bottom_rail_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOT_RAIL_PAD), MP_ROM_PTR(&probe_bottom_rail_pad_obj) },
    { MP_ROM_QSTR(MP_QSTR_TOP_RAIL_GND), MP_ROM_PTR(&probe_top_rail_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_TOP_GND_PAD), MP_ROM_PTR(&probe_top_rail_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOTTOM_RAIL_GND), MP_ROM_PTR(&probe_bottom_rail_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOT_RAIL_GND), MP_ROM_PTR(&probe_bottom_rail_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOTTOM_GND_PAD), MP_ROM_PTR(&probe_bottom_rail_gnd_obj) },
    { MP_ROM_QSTR(MP_QSTR_BOT_GND_PAD), MP_ROM_PTR(&probe_bottom_rail_gnd_obj) },
    
    // DAC functions
    { MP_ROM_QSTR(MP_QSTR_dac_set), MP_ROM_PTR(&jl_dac_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_dac_get), MP_ROM_PTR(&jl_dac_get_obj) },
    
    // DAC function aliases
    { MP_ROM_QSTR(MP_QSTR_set_dac), MP_ROM_PTR(&jl_dac_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_dac), MP_ROM_PTR(&jl_dac_get_obj) },
    
    // ADC functions
    { MP_ROM_QSTR(MP_QSTR_adc_get), MP_ROM_PTR(&jl_adc_get_obj) },
    
    // ADC function aliases
    { MP_ROM_QSTR(MP_QSTR_get_adc), MP_ROM_PTR(&jl_adc_get_obj) },
    
    // INA functions
    { MP_ROM_QSTR(MP_QSTR_ina_get_current), MP_ROM_PTR(&jl_ina_get_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_voltage), MP_ROM_PTR(&jl_ina_get_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_bus_voltage), MP_ROM_PTR(&jl_ina_get_bus_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_ina_get_power), MP_ROM_PTR(&jl_ina_get_power_obj) },
    
    // INA function aliases
    { MP_ROM_QSTR(MP_QSTR_get_ina_current), MP_ROM_PTR(&jl_ina_get_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ina_voltage), MP_ROM_PTR(&jl_ina_get_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ina_bus_voltage), MP_ROM_PTR(&jl_ina_get_bus_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_ina_power), MP_ROM_PTR(&jl_ina_get_power_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_current), MP_ROM_PTR(&jl_ina_get_current_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_voltage), MP_ROM_PTR(&jl_ina_get_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_bus_voltage), MP_ROM_PTR(&jl_ina_get_bus_voltage_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_power), MP_ROM_PTR(&jl_ina_get_power_obj) },
    
    // GPIO functions
    { MP_ROM_QSTR(MP_QSTR_gpio_set), MP_ROM_PTR(&jl_gpio_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get), MP_ROM_PTR(&jl_gpio_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_set_dir), MP_ROM_PTR(&jl_gpio_set_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get_dir), MP_ROM_PTR(&jl_gpio_get_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_set_pull), MP_ROM_PTR(&jl_gpio_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_gpio_get_pull), MP_ROM_PTR(&jl_gpio_get_pull_obj) },
    
    // GPIO function aliases
    { MP_ROM_QSTR(MP_QSTR_set_gpio), MP_ROM_PTR(&jl_gpio_set_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gpio), MP_ROM_PTR(&jl_gpio_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_gpio_dir), MP_ROM_PTR(&jl_gpio_set_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gpio_dir), MP_ROM_PTR(&jl_gpio_get_dir_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_gpio_pull), MP_ROM_PTR(&jl_gpio_set_pull_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_gpio_pull), MP_ROM_PTR(&jl_gpio_get_pull_obj) }, 
    
    // PWM functions
    { MP_ROM_QSTR(MP_QSTR_pwm), MP_ROM_PTR(&jl_pwm_obj) },
    { MP_ROM_QSTR(MP_QSTR_pwm_set_duty_cycle), MP_ROM_PTR(&jl_pwm_set_duty_cycle_obj) },
    { MP_ROM_QSTR(MP_QSTR_pwm_set_frequency), MP_ROM_PTR(&jl_pwm_set_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_pwm_stop), MP_ROM_PTR(&jl_pwm_stop_obj) },
    
    // PWM function aliases
    { MP_ROM_QSTR(MP_QSTR_set_pwm), MP_ROM_PTR(&jl_pwm_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_duty_cycle), MP_ROM_PTR(&jl_pwm_set_duty_cycle_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_pwm_frequency), MP_ROM_PTR(&jl_pwm_set_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop_pwm), MP_ROM_PTR(&jl_pwm_stop_obj) },

    // Wavegen API
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_output), MP_ROM_PTR(&jl_wavegen_set_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_output), MP_ROM_PTR(&jl_wavegen_set_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_freq), MP_ROM_PTR(&jl_wavegen_set_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_freq), MP_ROM_PTR(&jl_wavegen_set_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_wave), MP_ROM_PTR(&jl_wavegen_set_wave_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_wave), MP_ROM_PTR(&jl_wavegen_set_wave_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_sweep), MP_ROM_PTR(&jl_wavegen_set_sweep_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_sweep), MP_ROM_PTR(&jl_wavegen_set_sweep_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_amplitude), MP_ROM_PTR(&jl_wavegen_set_amplitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_amplitude), MP_ROM_PTR(&jl_wavegen_set_amplitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_set_offset), MP_ROM_PTR(&jl_wavegen_set_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_wavegen_offset), MP_ROM_PTR(&jl_wavegen_set_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_start), MP_ROM_PTR(&jl_wavegen_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_start_wavegen), MP_ROM_PTR(&jl_wavegen_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_stop), MP_ROM_PTR(&jl_wavegen_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop_wavegen), MP_ROM_PTR(&jl_wavegen_stop_obj) },
    // Getters
    { MP_ROM_QSTR(MP_QSTR_wavegen_get_output), MP_ROM_PTR(&jl_wavegen_get_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_wavegen_output), MP_ROM_PTR(&jl_wavegen_get_output_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_get_freq), MP_ROM_PTR(&jl_wavegen_get_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_wavegen_freq), MP_ROM_PTR(&jl_wavegen_get_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_get_wave), MP_ROM_PTR(&jl_wavegen_get_wave_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_wavegen_wave), MP_ROM_PTR(&jl_wavegen_get_wave_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_get_amplitude), MP_ROM_PTR(&jl_wavegen_get_amplitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_wavegen_amplitude), MP_ROM_PTR(&jl_wavegen_get_amplitude_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_get_offset), MP_ROM_PTR(&jl_wavegen_get_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_wavegen_offset), MP_ROM_PTR(&jl_wavegen_get_offset_obj) },
    { MP_ROM_QSTR(MP_QSTR_wavegen_is_running), MP_ROM_PTR(&jl_wavegen_is_running_obj) },
    
    // Node functions
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&jl_nodes_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_disconnect), MP_ROM_PTR(&jl_nodes_disconnect_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_clear), MP_ROM_PTR(&jl_nodes_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_is_connected), MP_ROM_PTR(&jl_nodes_is_connected_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_save), MP_ROM_PTR(&jl_nodes_save_obj) },
    
    // Raw hardware functions
    { MP_ROM_QSTR(MP_QSTR_send_raw), MP_ROM_PTR(&jl_send_raw_obj) },
    { MP_ROM_QSTR(MP_QSTR_switch_slot), MP_ROM_PTR(&jl_switch_slot_obj) },
    
    // Session management functions
    { MP_ROM_QSTR(MP_QSTR_nodes_discard), MP_ROM_PTR(&jl_nodes_discard_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_has_changes), MP_ROM_PTR(&jl_nodes_has_changes_obj) },

    
    // OLED functions
    { MP_ROM_QSTR(MP_QSTR_oled_print), MP_ROM_PTR(&jl_oled_print_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_clear), MP_ROM_PTR(&jl_oled_clear_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_show), MP_ROM_PTR(&jl_oled_show_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_connect), MP_ROM_PTR(&jl_oled_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_oled_disconnect), MP_ROM_PTR(&jl_oled_disconnect_obj) },
    
    // Misc functions
    { MP_ROM_QSTR(MP_QSTR_arduino_reset), MP_ROM_PTR(&jl_arduino_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause_core2), MP_ROM_PTR(&jl_pause_core2_obj) },
    { MP_ROM_QSTR(MP_QSTR_run_app), MP_ROM_PTR(&jl_run_app_obj) },
    { MP_ROM_QSTR(MP_QSTR_change_terminal_color), MP_ROM_PTR(&jl_change_terminal_color_obj) },
    { MP_ROM_QSTR(MP_QSTR_cycle_term_color), MP_ROM_PTR(&jl_cycle_term_color_obj) },

        // Status functions
    { MP_ROM_QSTR(MP_QSTR_print_bridges), MP_ROM_PTR(&jl_nodes_print_bridges_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_paths), MP_ROM_PTR(&jl_nodes_print_paths_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_crossbars), MP_ROM_PTR(&jl_nodes_print_crossbars_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_nets), MP_ROM_PTR(&jl_nodes_print_nets_obj) },
    { MP_ROM_QSTR(MP_QSTR_print_chip_status), MP_ROM_PTR(&jl_nodes_print_chip_status_obj) },
    
    // Probe functions
    { MP_ROM_QSTR(MP_QSTR_probe_tap), MP_ROM_PTR(&jl_probe_tap_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_read_blocking), MP_ROM_PTR(&jl_probe_read_blocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_read_nonblocking), MP_ROM_PTR(&jl_probe_read_nonblocking_obj) },
    
    // Probe button functions
    { MP_ROM_QSTR(MP_QSTR_probe_button_blocking), MP_ROM_PTR(&jl_probe_button_blocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_button_nonblocking), MP_ROM_PTR(&jl_probe_button_nonblocking_obj) },
    
    // Probe touch/read aliases (parameterized versions support blocking=True/False)
    { MP_ROM_QSTR(MP_QSTR_probe_read), MP_ROM_PTR(&jl_probe_read_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_probe), MP_ROM_PTR(&jl_read_probe_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_wait), MP_ROM_PTR(&jl_probe_wait_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait_probe), MP_ROM_PTR(&jl_wait_probe_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_touch), MP_ROM_PTR(&jl_probe_touch_obj) },
    { MP_ROM_QSTR(MP_QSTR_wait_touch), MP_ROM_PTR(&jl_wait_touch_obj) },
    
    // Probe button aliases (parameterized versions support blocking=True/False)
    { MP_ROM_QSTR(MP_QSTR_get_button), MP_ROM_PTR(&jl_get_button_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_button_read), MP_ROM_PTR(&jl_button_read_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_read_button), MP_ROM_PTR(&jl_read_button_param_obj) },
    { MP_ROM_QSTR(MP_QSTR_probe_button), MP_ROM_PTR(&jl_probe_button_param_obj) },
    
    // Probe button non-blocking aliases
    { MP_ROM_QSTR(MP_QSTR_check_button), MP_ROM_PTR(&jl_check_button_obj) },
    { MP_ROM_QSTR(MP_QSTR_button_check), MP_ROM_PTR(&jl_button_check_obj) },
    
    // Clickwheel functions
    { MP_ROM_QSTR(MP_QSTR_clickwheel_up), MP_ROM_PTR(&jl_clickwheel_up_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_down), MP_ROM_PTR(&jl_clickwheel_down_obj) },
    { MP_ROM_QSTR(MP_QSTR_clickwheel_press), MP_ROM_PTR(&jl_clickwheel_press_obj) },

    // Help functions
    { MP_ROM_QSTR(MP_QSTR_help), MP_ROM_PTR(&jl_help_obj) },
    { MP_ROM_QSTR(MP_QSTR_nodes_help), MP_ROM_PTR(&jl_help_nodes_obj) },
    
    // Filesystem functions 
    { MP_ROM_QSTR(MP_QSTR_fs_exists), MP_ROM_PTR(&jl_fs_exists_obj) },
    { MP_ROM_QSTR(MP_QSTR_fs_listdir), MP_ROM_PTR(&jl_fs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_fs_read), MP_ROM_PTR(&jl_fs_read_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_fs_write), MP_ROM_PTR(&jl_fs_write_file_obj) },
    { MP_ROM_QSTR(MP_QSTR_fs_cwd), MP_ROM_PTR(&jl_fs_get_current_dir_obj) },
    
    // JFS Module - Comprehensive filesystem API
    { MP_ROM_QSTR(MP_QSTR_jfs), MP_ROM_PTR(&jfs_user_cmodule) },

    // Waveform constants
    { MP_ROM_QSTR(MP_QSTR_SINE), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_TRIANGLE), MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_SAWTOOTH), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_SQUARE), MP_ROM_INT(3) },
    // Aliases and extras for waveforms
    { MP_ROM_QSTR(MP_QSTR_RAMP), MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_ARBITRARY), MP_ROM_INT(4) },
    
    // Enhanced Logic Analyzer Functions
    { MP_ROM_QSTR(MP_QSTR_la_set_trigger), MP_ROM_PTR(&jl_la_set_trigger_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_capture_single_sample), MP_ROM_PTR(&jl_la_capture_single_sample_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_start_continuous_capture), MP_ROM_PTR(&jl_la_start_continuous_capture_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_stop_capture), MP_ROM_PTR(&jl_la_stop_capture_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_is_capturing), MP_ROM_PTR(&jl_la_is_capturing_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_set_sample_rate), MP_ROM_PTR(&jl_la_set_sample_rate_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_set_num_samples), MP_ROM_PTR(&jl_la_set_num_samples_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_enable_channel), MP_ROM_PTR(&jl_la_enable_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_set_control_analog), MP_ROM_PTR(&jl_la_set_control_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_set_control_digital), MP_ROM_PTR(&jl_la_set_control_digital_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_get_control_analog), MP_ROM_PTR(&jl_la_get_control_analog_obj) },
    { MP_ROM_QSTR(MP_QSTR_la_get_control_digital), MP_ROM_PTR(&jl_la_get_control_digital_obj) },
};

static MP_DEFINE_CONST_DICT(jumperless_module_globals, jumperless_module_globals_table);

const mp_obj_module_t jumperless_user_cmodule = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&jumperless_module_globals,
};
