/*
 * Jumperless MicroPython API Wrapper Functions
 * 
 * These functions provide a C-compatible interface for MicroPython
 * to call Jumperless functionality directly without string parsing.
 */

#include "ArduinoStuff.h"
#include "Commands.h"

#include "Graphics.h"
#include "Oled.h"
#include "RotaryEncoder.h"
#include "Peripherals.h"
#include "FileParsing.h"

#include "JumperlessDefines.h"


// C-compatible wrapper functions for MicroPython
extern "C" {

// DAC Functions
void jl_dac_set(int channel, float voltage, int save) {
    setDacByNumber(channel, voltage, save);
}

float jl_dac_get(int channel) {
    // TODO: Implement DAC read function if available
    return 0.0f; // Placeholder - implement actual DAC read
}

// ADC Functions  
float jl_adc_get(int channel) {
    return readAdcVoltage(channel, 32);
}

// INA Functions
float jl_ina_get_current(int sensor) {
    if (sensor == 0) {
        return INA0.getCurrent_mA();
    } else if (sensor == 1) {
        return INA1.getCurrent_mA();
    }
    return 0.0f;
}

float jl_ina_get_voltage(int sensor) {
    if (sensor == 0) {
        return INA0.getShuntVoltage_mV();
    } else if (sensor == 1) {
        return INA1.getShuntVoltage_mV();
    }
    return 0.0f;
}

float jl_ina_get_bus_voltage(int sensor) {
    if (sensor == 0) {
        return INA0.getBusVoltage();
    } else if (sensor == 1) {
        return INA1.getBusVoltage();
    }
    return 0.0f;
}

float jl_ina_get_power(int sensor) {
    if (sensor == 0) {
        return INA0.getPower_mW();
    } else if (sensor == 1) {
        return INA1.getPower_mW();
    }
    return 0.0f;
}

// GPIO Functions
void jl_gpio_set(int pin, int value) {
    if (pin >= 1 && pin <= 10) {
        digitalWrite(gpioDef[pin][0], value);
    }
}

int jl_gpio_get(int pin) {
    if (pin >= 1 && pin <= 10) {
        return digitalRead(gpioDef[pin][0]);
    }
    return 0;
}

void jl_gpio_set_direction(int pin, int direction) {
    if (pin >= 1 && pin <= 10) {
        pinMode(gpioDef[pin][0], direction ? OUTPUT : INPUT);
    }
}

// Node Functions
void jl_nodes_connect(int node1, int node2, int save) {
    if (save) {
        addBridgeToNodeFile(node1, node2, netSlot, 0);
        refreshConnections();
    } else {
        addBridgeToNodeFile(node1, node2, netSlot, 1);
        refreshLocalConnections();
    }
}

void jl_nodes_disconnect(int node1, int node2) {
    removeBridgeFromNodeFile(node1, node2, netSlot, 0);
    refreshConnections();
}

void jl_nodes_clear(void) {
    clearNodeFile();//!
    refreshConnections();
}

// OLED Functions
void jl_oled_print(const char* text, int size) {
    oled.clearPrintShow(text, size, true, true, true);
}

void jl_oled_clear(void) {
    oled.clear();
}

void jl_oled_show(void) {
    oled.show();
}

int jl_oled_connect(void) {
    return oled.init();
}

void jl_oled_disconnect(void) {
    oled.disconnect();
}

// Arduino Functions
void jl_arduino_reset(void) {
    resetArduino();
}

// Probe Functions
void jl_probe_tap(int node) {
    // TODO: Implement probe simulation
    // This would simulate tapping the probe on a specific node
}

// Clickwheel Functions
void jl_clickwheel_up(int clicks) {
    encoderOverride = 10;
    lastDirectionState = NONE;
    encoderDirectionState = UP;
}

void jl_clickwheel_down(int clicks) {
    encoderOverride = 10;
    lastDirectionState = NONE;
    encoderDirectionState = DOWN;
}

void jl_clickwheel_press(void) {
    encoderOverride = 10;
    lastButtonEncoderState = PRESSED;
    encoderButtonState = RELEASED;
}

} // extern "C" 