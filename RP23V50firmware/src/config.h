#ifndef CONFIG_H
#define CONFIG_H

//Jumperless config
#include "JumperlessDefines.h"


// extern int hwRevision;
// extern int probeRevision;


extern struct config jumperlessConfig;
// Forward declarations of nested structs
struct hardware;
struct dacs;
struct debug;
struct routing;
struct calibration;
struct logo_pads;
struct display_settings;
struct gpio;
struct serial_1;
struct serial_2;
struct top_oled;

struct config {
    struct hardware {
        int generation = 5;
        int revision = 5;
        int probe_revision = 5;
    } hardware;

    struct dacs {
        float top_rail = 0.00;
        float bottom_rail = 0.00;
        float dac_0 = 3.33;
        float dac_1 = 0.00;
        bool set_dacs_on_boot = false;
        bool set_rails_on_boot = true;
        int probe_power_dac = 0;
        float limit_max = 8.00;
        float limit_min = -8.00;
    } dacs;

    struct debug {
        bool file_parsing = false;
        bool net_manager = false;
        bool nets_to_chips = false;
        bool nets_to_chips_alt = false;
        bool leds = false;
        bool probing = false;
        bool oled = false;
        bool logo_pads = false;
    } debug;

    struct routing {
        int stack_paths = 3;
        int stack_rails = 3;
        int stack_dacs = 0;
        int rail_priority = 1;
    } routing;

    struct calibration {
        int top_rail_zero = 1650;
        float top_rail_spread = 21.5;
        int bottom_rail_zero = 1650;
        float bottom_rail_spread = 21.5;
        int dac_0_zero = 1650;
        float dac_0_spread = 21.5;
        int dac_1_zero = 1650;
        float dac_1_spread = 21.5;
        int probe_max = 4060;
        int probe_min = 15;
    } calibration;

    struct logo_pads {
        int top_guy = 0; // 0 = uart tx, 1 = uart rx, others as I think of them
        int bottom_guy = 1;
        int building_pad_top = -1;
        int building_pad_bottom = -1;
        int repeat_ms = 100;
    } logo_pads;

    struct display {
        int lines_wires = 1;
        int menu_brightness = -10;
        int led_brightness = 10;
        int rail_brightness = 55;
        int special_net_brightness = 20;
        int net_color_mode = 0;
    } display;

    struct gpio {
        int direction[10] = {
            1, //gpio_0 1 = input 0 = output
            1, //gpio_1
            1, //gpio_2
            1, //gpio_3
            1, //gpio_4
            1, //gpio_5
            1, //gpio_6
            1, //gpio_7
            0, //uart_tx
            1, //uart_rx
        };
        int pulls[10] = {
            0, //gpio_0 0 = pull down 1 = pull up 2 = none
            0, //gpio_1
            0, //gpio_2
            0, //gpio_3
            0, //gpio_4
            0, //gpio_5
            0, //gpio_6
            0, //gpio_7
            2, //uart_tx - no pull
            2, //uart_rx - no pull
        };
        // int uart_tx_pin = 0;
        // int uart_rx_pin = 1;
        // int uart_tx_pull = 2;
        // int uart_rx_pull = 2;
        int uart_tx_function = 0; //0 = tx, 1 = rx, 2 = gpio_in, 3 = gpio_out
        int uart_rx_function = 1;
    } gpio;

    
        struct serial_1 {
            int function = 1; 
            int baud_rate = 115200;
            int print_passthrough = 0;
            int connect_on_boot = 0;
            int lock_connection = 0;
        } serial_1;

        
        struct serial_2 {
            int function = 0; // 0 = off
            int baud_rate = 115200;
            int print_passthrough = 0;
            int connect_on_boot = 0;
            int lock_connection = 0;
        } serial_2;

        struct top_oled {
            int enabled = 0;
            int i2c_address = 0x3C;
            int width = 128;
            int height = 32;
            int sda_pin = 26;
            int scl_pin = 27;
            int gpio_sda = RP_GPIO_26;
            int gpio_scl = RP_GPIO_27;
            int sda_row = NANO_D2;
            int scl_row = NANO_D3;
            int connect_on_boot = 0;
            int lock_connection = 0;
        } top_oled;
    
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