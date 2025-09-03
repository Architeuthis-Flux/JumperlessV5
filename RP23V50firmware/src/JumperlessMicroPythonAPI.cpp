/*
 * Jumperless MicroPython API Wrapper Functions
 * 
 * These functions provide a C-compatible interface for MicroPython
 * to call Jumperless functionality directly without string parsing.
 */

#include "ArduinoStuff.h"
#include "Commands.h"
#include "CH446Q.h"
#include "FileParsing.h"

#include "Graphics.h"
#include "NetsToChipConnections.h"
#include "oled.h"
#include "RotaryEncoder.h"
#include "Peripherals.h"

#include "JumperlessDefines.h"
#include "hardware/gpio.h"
#include "SafeString.h"

#include "LogicAnalyzer.h"

extern LogicAnalyzer logicAnalyzer; // defined in main.cpp

// External declarations
extern SafeString nodeFileString;

#include "CH446Q.h"
#include "NetManager.h"
#include "Apps.h"
#include "Probing.h"
#include "Python_Proper.h"
#include "config.h"
#include "FatFS.h"

#include "JulseView.h"




// Forward declarations
int justReadProbe(bool allowDuplicates);

// C-compatible wrapper functions for MicroPython
extern "C" {
#include "py/mpthread.h"





void jl_pause_core2(bool pause) {
    pauseCore2 = pause;
}


void jl_change_terminal_color(int color, bool flush) {
    changeTerminalColor(color, flush);
}

void jl_cycle_term_color(bool reset, float step, bool flush) {
    cycleTermColor(reset, step, flush);
}

void jl_print_terminal_colors(void) {
    printSpectrumOrderedColorCube();
}




// DAC Functions
void jl_dac_set(int channel, float voltage, int save) {
    // if (channel == 0) {
    //     channel = 2;
    // } else if (channel == 1) {
    //     channel = 3;
    // } else if (channel == 2) {
    //     channel = 0;
    // } else if (channel == 3) {
    //     channel = 1;
    // }
    setDacByNumber(channel, voltage, save, 0, false);
}

float jl_dac_get(int channel) {
    float voltage = 0.0f;

    if (channel == 0) {
        voltage = dacOutput[0];
    } else if (channel == 1) {
        voltage = dacOutput[1];
    } else if (channel == 2) {
        voltage = railVoltage[0];
    } else if (channel == 3) {
        voltage = railVoltage[1];
    }

    return voltage;
}

// ADC Functions  
float jl_adc_get(int channel) {
    return readAdcVoltage(channel, 32);
}

// INA Functions
float jl_ina_get_current(int sensor) {
    if (sensor == 0) {
        return INA0.getCurrent();
    } else if (sensor == 1) {
        return INA1.getCurrent();
    }
    return 0.0f;
}

float jl_ina_get_voltage(int sensor) {
    if (sensor == 0) {
        return INA0.getBusVoltage();
    } else if (sensor == 1) {
        return INA1.getBusVoltage();
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
        return INA0.getPower();
    } else if (sensor == 1) {
        return INA1.getPower();
    }
    return 0.0f;
}

// GPIO Functions
void jl_gpio_set(int pin, int value) {
    if (pin >= 1 && pin <= 10) {
        digitalWrite(gpioDef[pin - 1][0], value);
    } else if (pin >= 20 && pin <= 27) {
        digitalWrite(pin, value);
    }
}

int jl_gpio_get(int pin) {
    if (pin >= 1 && pin <= 10) {
        // Serial.print("jl_gpio_get = ");
        // Serial.println(gpioDef[pin - 1][0]);
        // Serial.print("gpio_get = ");
        // Serial.println(gpio_get((uint)gpioDef[pin - 1][0]));
        // Serial.println("digitalRead = ");
        // Serial.println(digitalRead(gpioDef[pin - 1][0]));
        // return gpio_get(gpioDef[pin - 1][0]);
        while (readingGPIO) {
            delayMicroseconds(1);
        }

        int reading = gpioReadWithFloating(gpioDef[pin - 1][0], 50);
        // Serial.print("gpioReadWithFloating = ");
        // Serial.println(reading);
        return reading;
    
    } else if (pin >= 20 && pin <= 27) {
        return gpio_get(pin);
    }
    return 0;
}

int jl_gpio_set_direction(int pin, int direction) {
    if (pin >= 1 && pin <= 10) {
        jumperlessConfig.gpio.direction[pin - 1] = direction;
        pinMode(gpioDef[pin - 1][0], direction ? OUTPUT : INPUT);
    } else if (pin >= 20 && pin <= 27) {
        jumperlessConfig.gpio.direction[pin - 20] = direction;
        pinMode(pin, direction ? OUTPUT : INPUT);
    }
    return 1;
}

int jl_gpio_get_dir(int pin) {
    if (pin >= 1 && pin <= 10) {
        return gpio_get_dir(gpioDef[pin - 1][0]);
    } else if (pin >= 20 && pin <= 27) {
        return gpio_get_dir(pin);
    }
    return 0;
}

void jl_gpio_set_dir(int pin, int direction) {
    if (pin >= 1 && pin <= 10) {
        gpio_set_dir(gpioDef[pin - 1][0], direction);
        jumperlessConfig.gpio.direction[pin - 1] = direction;
    } else if (pin >= 20 && pin <= 27) {
        gpio_set_dir(pin, direction);
        jumperlessConfig.gpio.direction[pin - 20] = direction;
    } 

}

int jl_gpio_get_pull(int pin) {
    
    if (pin >= 1 && pin <= 10) {
        pin = gpioDef[pin - 1][0];
        bool pull_up = gpio_is_pulled_up(pin);
        bool pull_down = gpio_is_pulled_down(pin);
        if (pull_up && pull_down) {
            return 2; // bus keeper
        } else if (pull_up) {
            return 1; // pullup
        } else if (pull_down) {
            return -1; // pulldown
        } else {
            return 0; // no pull
        }
    } else if (pin >= 20 && pin <= 27) {
        bool pull_up = gpio_is_pulled_up(pin);
        bool pull_down = gpio_is_pulled_down(pin);
        if (pull_up && pull_down) {
            return 2; // bus keeper
        } else if (pull_up) {
            return 1; // pullup
        } else if (pull_down) {
            return -1; // pulldown
        } else {
            return 0; // no pull
        }
    }
    return 0;
}

void jl_gpio_set_pull(int pin, int pull) {

    // Serial.print("jl_gpio_set_pull: ");
    // Serial.println(pull);
    
    bool pull_up = false;
    bool pull_down = false;

    int config_pull = 0;
    if (pull == 0) {
        pull_up = false;
        pull_down = false;
        config_pull = 2; // no pull
    } else if (pull == 1) {
        pull_up = true;
        pull_down = false;
        config_pull = 1; // pullup
    } else if (pull == -1) {
        pull_up = false;
        pull_down = true;
        config_pull = 0; // pulldown
    } else if (pull == 2) {
        pull_up = true;
        pull_down = true; // bus keeper mode
        config_pull = 3; // bus keeper
    }


    if (pin >= 1 && pin <= 10) {
        pin = gpioDef[pin - 1][0];
        
        gpio_set_pulls(pin, pull_up, pull_down);
        
        jumperlessConfig.gpio.pulls[pin - 1] = config_pull;
    } else if (pin >= 20 && pin <= 27) {
        gpio_set_pulls(pin, pull_up, pull_down);
        jumperlessConfig.gpio.pulls[pin - 20] = config_pull;
    }
}

// Node Functions
int jl_nodes_connect(int node1, int node2, int save) {
    if (save) {
        addBridgeToNodeFile(node1, node2, netSlot, 0);
        refreshConnections();
    } else {
        addBridgeToNodeFile(node1, node2, netSlot, 1);
        refreshLocalConnections();
    }
    return 1;
}

int jl_nodes_disconnect(int node1, int node2) {
    removeBridgeFromNodeFile(node1, node2, netSlot, 0);
    refreshConnections(-1);
    return 1;
}

int jl_nodes_clear(void) {
    createSlots(netSlot,  1);
    //delay(2);
    refreshConnections(-1, 1, 1);
    waitCore2();
    return 1;
}

int jl_nodes_is_connected(int node1, int node2) {

    int connected = checkIfBridgeExists(node1, node2, netSlot, 0 );
    // Serial.print("jl_nodes_is_connected = ");
    // Serial.println(connected);
    return connected;
    //return checkIfBridgeExists(node1, node2, netSlot, 0 );
}

int jl_nodes_save(int slot) {
    int target_slot = (slot == -1) ? netSlot : slot;  // Use current slot if -1
    
    // Save the local nodeFileString to the specified slot
    saveLocalNodeFile(target_slot);
    
    // Refresh connections to make sure everything is in sync
    refreshConnections();
    
    return target_slot;  // Return the slot that was saved to
}

void jl_init_micropython_local_copy(void) {
    // Use the generalized backup system to store entry state
    storeNodeFileBackup();
}

void jl_exit_micropython_restore_entry_state(void) {
    // By default, restore to entry state (discard Python changes)
    // This makes Python connections temporary unless explicitly saved
    restoreAndSaveNodeFileBackup();
    
    // Refresh connections to match the restored state
    refreshLocalConnections();
}

void jl_restore_micropython_entry_state(void) {
    // Use the generalized backup system to restore entry state
    restoreAndSaveNodeFileBackup();
    
    // Refresh connections to match the restored state
    refreshLocalConnections();
}

int jl_has_unsaved_changes(void) {
    // Use the generalized backup system to check for changes
    return hasNodeFileChanges() ? 1 : 0;
}

// Helper function to convert chip identifier to chip number
int parseChipIdentifier(const char* chip_str) {
    if (strlen(chip_str) == 1) {
        char c = chip_str[0];
        if (c >= 'A' && c <= 'L') {
            return c - 'A';  // A=0, B=1, ..., L=11
        } else if (c >= 'a' && c <= 'l') {
            return c - 'a';  // a=0, b=1, ..., l=11
        }
    }
    // If not a letter, try to parse as number
    int chip_num = atoi(chip_str);
    if (chip_num >= 0 && chip_num <= 11) {
        return chip_num;
    }
    return -1; // Invalid chip identifier
}

void jl_send_raw(int chip, int x, int y, int setOrClear) {
    // Validate chip number (0-11)
    if (chip < 0 || chip > 11) {
        Serial.print("jl_send_raw: Invalid chip number: ");
        Serial.println(chip);
        return; // Invalid chip number
    }
    
    // Validate x,y coordinates (assuming 0-15 range based on typical crossbar chips)
    if (x < 0 || x > 15 || y < 0 || y > 15) {
        Serial.print("jl_send_raw: Invalid coordinates: ");
        Serial.print(x);
        Serial.print(",");
        Serial.println(y);
        return; // Invalid coordinates
    }
    
    // Call the existing sendXYraw function with setOrClear=1 (set path)
    lastChipXY[chip].connected[x][y] = setOrClear;
    sendXYraw(chip, x, y, setOrClear);
}

void jl_send_raw_str(const char* chip_str, int x, int y, int setOrClear) {
    int chip = parseChipIdentifier(chip_str);
    if (chip >= 0) {
        // Serial.print("jl_send_raw_str: chip = ");
        // Serial.println(chip);
        // Serial.print("jl_send_raw_str: x = ");
        // Serial.println(x);
        // Serial.print("jl_send_raw_str: y = ");
        // Serial.println(y);
        // Serial.print("jl_send_raw_str: setOrClear = ");
        jl_send_raw(chip, x, y, setOrClear);
    }
}

int jl_switch_slot(int slot) {
    // Validate slot number
    if (slot < 0 || slot >= NUM_SLOTS) {
        return -1; // Invalid slot number
    }
    
    // Save current slot if different
    if (netSlot != slot) {
        int old_slot = netSlot;
        netSlot = slot;
        
        // Refresh connections for the new slot
        refreshConnections(-1);
        
        return old_slot; // Return the previous slot number
    }
    
    return slot; // Already in this slot
}



// // Logic Analyzer Functions


void jl_control_set_analog(int channel, float value) {
    // if (channel >= 0 && channel < 4) {
    //     control_A[channel] = value;
    // }
}

void jl_control_set_digital(int channel, bool value) {
    // if (channel >= 0 && channel < 4) {
    //     control_D[channel] = value;
    // }
}

// Enhanced Logic Analyzer Functions
bool jl_la_set_trigger(int trigger_type, int channel, float value) {
    // Triggers not implemented in LogicAnalyzer yet; accept and noop
    (void)trigger_type; (void)channel; (void)value;
    return true;
}

bool jl_la_capture_single_sample(void) {
    if (logicAnalyzer.getIsRunning()) return false;
    logicAnalyzer.num_samples = 1;
    logicAnalyzer.sample_rate_hz = 1000;
    logicAnalyzer.arm();
    logicAnalyzer.run();
    while (logicAnalyzer.getIsRunning()) { delayMicroseconds(100); }
    return true;
}

bool jl_la_start_continuous_capture(void) {
    if (logicAnalyzer.getIsRunning()) return false;
    logicAnalyzer.num_samples = 0; // 0 => continuous not yet supported; use large value
    logicAnalyzer.num_samples = 0x7FFFFFFF;
    logicAnalyzer.sample_rate_hz = 1000000;
    logicAnalyzer.arm();
    logicAnalyzer.run();
    return true;
}

bool jl_la_stop_capture(void) {
    if (!logicAnalyzer.getIsRunning()) return false;
    logicAnalyzer.reset();
    return true;
}

bool jl_la_is_capturing(void) {
    return logicAnalyzer.getIsRunning();
}

void jl_la_set_sample_rate(uint32_t sample_rate) {
    logicAnalyzer.sample_rate_hz = sample_rate;
}

void jl_la_set_num_samples(uint32_t num_samples) {
    logicAnalyzer.num_samples = num_samples;
}

void jl_la_enable_channel(int channel_type, int channel, bool enable) {
    if (channel_type == 0) { // Digital
        if (channel >= 0 && channel < 8) {
            if (enable) logicAnalyzer.d_mask |= (1u << channel);
            else logicAnalyzer.d_mask &= ~(1u << channel);
        }
    } else if (channel_type == 1) { // Analog
        if (channel >= 0 && channel < 8) {
            if (enable) logicAnalyzer.a_mask |= (1u << channel);
            else logicAnalyzer.a_mask &= ~(1u << channel);
        }
    }
}

void jl_la_set_control_analog(int channel, float value) {
    jl_control_set_analog(channel, value);
}

void jl_la_set_control_digital(int channel, bool value) {
    jl_control_set_digital(channel, value);
}

float jl_la_get_control_analog(int channel) {
   // return (channel >= 0 && channel < 4) ? control_A[channel] : 0.0f;
   return 0.0f;
}

bool jl_la_get_control_digital(int channel) {
   // return (channel >= 0 && channel < 4) ? control_D[channel] : false;
   return false;
}

// OLED Functions
int jl_oled_print(const char* text, int size) {
    mp_hal_check_interrupt();
    if (jumperlessConfig.top_oled.enabled == 1 && oled.isConnected()) {
        oled.clearPrintShow(text, size, true, true, true, -1, -1, 1500);
        return 1;
    } else {
        return 0;
    }
}

int jl_oled_clear(void) {
    if (jumperlessConfig.top_oled.enabled == 1 && oled.isConnected()) {
        oled.clear(1000);
        return 1;
    } else {
        return 0;
    }
}

int jl_oled_show(void) {
    if (jumperlessConfig.top_oled.enabled == 1 && oled.isConnected()) {
        oled.show(1000);
        return 1;
    } else {
        return 0;
    }
}

int jl_oled_connect(void) {
    return oled.init();
}

int jl_oled_disconnect(void) {
    oled.disconnect();
    return 1;
}

// Arduino Functions
void jl_arduino_reset(void) {
    resetArduino();
}

// Status Functions
int jl_nodes_print_bridges(void) {
    printPathsCompact();
    return 1;
}

int jl_nodes_print_paths(void) {
    printPathsCompact();
    return 1;
}

int jl_nodes_print_crossbars(void) {
    printChipStateArray();
    return 1;
}

int jl_nodes_print_nets(void) {
    listNets(0);
    return 1;
}

int jl_nodes_print_chip_status(void) {
    printChipStatus();
    return 1;
}

int jl_run_app(char* appName) {
    runApp(-1,appName);
    return 1;
}

// Probe Functions
void jl_probe_tap(int node) {
    // TODO: Implement probe simulation
    // This would simulate tapping the probe on a specific node
}

int jl_probe_read_blocking(void) {
    int pad = -1;
    static int call_count = 0;
    call_count++;
    
    while (pad == -1) {
        mp_hal_check_interrupt();
        
        // Check if interrupt was requested and return special value
        if (mp_interrupt_requested) {
            mp_interrupt_requested = false; // Clear the flag
            Serial.print("DEBUG: Interrupt detected in jl_probe_read_blocking, call #");
            Serial.println(call_count);
            return -999; // Special return value indicating interrupt
        }
        
        pad = justReadProbe(false, 1);
        delay(1); // Small delay to prevent busy waiting
    }
    return pad;
}

int jl_probe_read_nonblocking(void) {
    return justReadProbe(true, 1);
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

// PWM Functions
extern "C" int jl_pwm_setup(int gpio_pin, float frequency, float duty_cycle) {
    return setupPWM(gpio_pin, frequency, duty_cycle);
}

extern "C" int jl_pwm_set_duty_cycle(int gpio_pin, float duty_cycle) {
    return setPWMDutyCycle(gpio_pin, duty_cycle);
}

extern "C" int jl_pwm_set_frequency(int gpio_pin, float frequency) {
    return setPWMFrequency(gpio_pin, frequency);
}

extern "C" int jl_pwm_stop(int gpio_pin) {
    return stopPWM(gpio_pin);
}

// Filesystem Functions
int jl_fs_exists(const char* path) {
    if (!path) return 0;
    return FatFS.exists(path) ? 1 : 0;
}

char* jl_fs_listdir(const char* path) {
    if (!path) return nullptr;
    
    // Use static buffer to avoid memory management issues
    static char listBuffer[2048];
    listBuffer[0] = '\0';
    
    Dir dir = FatFS.openDir(path);
    
    bool first = true;
    while (dir.next()) {
        if (!first) {
            strcat(listBuffer, ",");
        }
        strcat(listBuffer, dir.fileName().c_str());
        if (dir.isDirectory()) {
            strcat(listBuffer, "/");
        }
        first = false;
        
        // Prevent buffer overflow
        if (strlen(listBuffer) > 1900) {
            strcat(listBuffer, "...");
            break;
        }
    }
    
    return listBuffer;
}

char* jl_fs_read_file(const char* path) {
    if (!path) return nullptr;
    
    File file = FatFS.open(path, "r");
    if (!file) {
        return nullptr;
    }
    
    // Use static buffer for file contents
    static char fileBuffer[4096];
    size_t bytesRead = file.readBytes(fileBuffer, sizeof(fileBuffer) - 1);
    fileBuffer[bytesRead] = '\0';
    
    file.close();
    return fileBuffer;
}

int jl_fs_write_file(const char* path, const char* content) {
    if (!path || !content) return 0;
    
    File file = FatFS.open(path, "w");
    if (!file) {
        return 0;
    }
    
    size_t written = file.write((const uint8_t*)content, strlen(content));
    file.close();
    
    return (written == strlen(content)) ? 1 : 0;
}

char* jl_fs_get_current_dir(void) {
    static char currentDir[] = "/";
    return currentDir;
}

// File operations
void* jl_fs_open_file(const char* path, const char* mode) {
    if (!path || !mode) return nullptr;
    
    File* file = new File(FatFS.open(path, mode));
    if (!*file) {
        delete file;
        return nullptr;
    }
    return file;
}

void jl_fs_close_file(void* file_handle) {
    if (file_handle) {
        File* file = (File*)file_handle;
        file->close();
        delete file;
    }
}

int jl_fs_read_bytes(void* file_handle, char* buffer, int size) {
    if (!file_handle || !buffer) return -1;
    File* file = (File*)file_handle;
    return file->readBytes(buffer, size);
}

int jl_fs_write_bytes(void* file_handle, const char* data, int size) {
    if (!file_handle || !data) return -1;
    File* file = (File*)file_handle;
    return file->write((const uint8_t*)data, size);
}

int jl_fs_seek(void* file_handle, int position, int mode) {
    if (!file_handle) return 0;
    File* file = (File*)file_handle;
    SeekMode seekMode = SeekSet;
    if (mode == 1) seekMode = SeekCur;
    else if (mode == 2) seekMode = SeekEnd;
    return file->seek(position, seekMode) ? 1 : 0;
}

int jl_fs_position(void* file_handle) {
    if (!file_handle) return -1;
    File* file = (File*)file_handle;
    return file->position();
}

int jl_fs_size(void* file_handle) {
    if (!file_handle) return -1;
    File* file = (File*)file_handle;
    return file->size();
}

int jl_fs_available(void* file_handle) {
    if (!file_handle) return 0;
    File* file = (File*)file_handle;
    return file->available();
}

char* jl_fs_name(void* file_handle) {
    if (!file_handle) return nullptr;
    File* file = (File*)file_handle;
    static char nameBuffer[256];
    strncpy(nameBuffer, file->name(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';
    return nameBuffer;
}

// Directory operations
int jl_fs_mkdir(const char* path) {
    if (!path) return 0;
    return FatFS.mkdir(path) ? 1 : 0;
}

int jl_fs_rmdir(const char* path) {
    if (!path) return 0;
    return FatFS.rmdir(path) ? 1 : 0;
}

int jl_fs_remove(const char* path) {
    if (!path) return 0;
    return FatFS.remove(path) ? 1 : 0;
}

int jl_fs_rename(const char* pathFrom, const char* pathTo) {
    if (!pathFrom || !pathTo) return 0;
    return FatFS.rename(pathFrom, pathTo) ? 1 : 0;
}

// Get filesystem info
int jl_fs_total_bytes(void) {
    FSInfo info;
    if (FatFS.info(info)) {
        return (int)(info.totalBytes & 0xFFFFFFFF); // Return lower 32 bits
    }
    return -1;
}

int jl_fs_used_bytes(void) {
    FSInfo info;
    if (FatFS.info(info)) {
        return (int)(info.usedBytes & 0xFFFFFFFF); // Return lower 32 bits  
    }
    return -1;
}

} // extern "C" 