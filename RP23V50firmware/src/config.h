#ifndef CONFIG_H
#define CONFIG_H

//Jumperless config
#include "JumperlessDefinesRP2040.h"


// extern int hwRevision;
// extern int probeRevision;


extern struct config jumperlessConfig;
// Forward declarations of nested structs
struct hardware_version;
struct dac_settings;
struct debug_flags;
struct routing_settings;
struct calibration;
struct logo_pad_settings;
struct display_settings;
struct gpio;
struct serial;

struct config {
    struct hardware_version {
        int generation = 5;
        int hardware_revision = 4;
        int probe_version = 4;
    } hardware_version;

    struct dac_settings {
        float top_rail = 0.00;
        float bottom_rail = 0.00;
        float dac_0 = 0.00;
        float dac_1 = 0.00;
        bool set_dacs_on_startup = false;
        bool set_rails_on_startup = true;
        float limit_max = 8.00;
        float limit_min = -8.00;
    } dac_settings;

    struct debug_flags {
        bool file_parsing = false;
        bool net_manager = false;
        bool net_to_chip_connections = false;
        bool net_to_chip_connections_alt = false;
        bool leds = false;
    } debug_flags;

    struct routing_settings {
        int stack_paths = 2;
        int stack_rails = 2;
        int stack_dacs = 1;
        int rail_priority = 2;
    } routing_settings;

    struct calibration {
        int top_rail_zero = 1650;
        float top_rail_spread = 21.5;
        int bottom_rail_zero = 1650;
        float bottom_rail_spread = 21.5;
        int dac_0_zero = 1650;
        float dac_0_spread = 21.5;
        int dac_1_zero = 1650;
        float dac_1_spread = 21.5;
        int probe_max = 4028;
        int probe_min = 26;
    } calibration;

    struct logo_pad_settings {
        int top_guy = 0; // 0 = uart tx, 1 = uart rx, others as I think of them
        int bottom_guy = 1;
        int building_pad_top = 2;
        int building_pad_bottom = 3;
    } logo_pad_settings;

    struct display_settings {
        int lines_wires = 1;
        int menu_brightness = 100;
        int led_brightness = 10;
        int rail_brightness = 15;
        int special_net_brightness = 20;
        int net_color_mode = 0;
    } display_settings;

    struct gpio {
        int direction[8] = {
            1, //gpio_0 1 = input 0 = output
            1, //gpio_1
            1, //gpio_2
            1, //gpio_3
            1, //gpio_4
            1, //gpio_5
            1, //gpio_6
            1, //gpio_7
        };
        int pulls[8] = {
            2, //gpio_0 0 = pull up 1 = pull down 2 = none
            2, //gpio_1
            2, //gpio_2
            2, //gpio_3
            2, //gpio_4
            2, //gpio_5
            2, //gpio_6
            2, //gpio_7
        };
        int uart_tx_pin = 0;
        int uart_rx_pin = 1;
        int uart_tx_pull = 2;
        int uart_rx_pull = 2;
        int uart_tx_function = 0; //0 = tx, 1 = rx, 2 = gpio_in, 3 = gpio_out
        int uart_rx_function = 1;
    } gpio;

    struct serial {
        struct serial_1 {
            int function = 1; 
            int baud_rate = 115200;
        } serial_1;
        struct serial_2 {
            int function = 0; // 0 = off
            int baud_rate = 115200;
        } serial_2;
    } serial;
};

#endif // CONFIG_H

/*
 if (EEPROM.read(FIRSTSTARTUPADDRESS) != 0xAA || forceDefaults == 1) {
   EEPROM.write(FIRSTSTARTUPADDRESS, 0xAA);
   
   EEPROM.write(REVISIONADDRESS, REV);

   EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);
   EEPROM.write(TIME_FILEPARSINGADDRESS, 0);
   EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);
   EEPROM.write(TIME_NETMANAGERADDRESS, 0);
   EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);
   EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);
   EEPROM.write(DEBUG_LEDSADDRESS, 0);
   EEPROM.write(LEDBRIGHTNESSADDRESS, DEFAULTBRIGHTNESS);
   EEPROM.write(RAILBRIGHTNESSADDRESS, DEFAULTRAILBRIGHTNESS);
   EEPROM.write(SPECIALBRIGHTNESSADDRESS, DEFAULTSPECIALNETBRIGHTNESS);
   EEPROM.write(PROBESWAPADDRESS, 0);
   EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);
   EEPROM.write(DISPLAYMODE_ADDRESS, 1);
   EEPROM.write(NETCOLORMODE_ADDRESS, 0);
   EEPROM.write(MENUBRIGHTNESS_ADDRESS, 100);
   EEPROM.write(PATH_DUPLICATE_ADDRESS, 2);
   EEPROM.write(DAC_DUPLICATE_ADDRESS, 0);
   EEPROM.write(POWER_DUPLICATE_ADDRESS, 2);
   EEPROM.write(DAC_PRIORITY_ADDRESS, 1);
   EEPROM.write(POWER_PRIORITY_ADDRESS, 1);
   EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 0);

   saveVoltages(0.0f, 0.0f, 3.33f, 0.0f);

*/