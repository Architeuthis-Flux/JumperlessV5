// SPDX-License-Identifier: MIT
#include "Peripherals.h"
#include <Arduino.h>
#include "CH446Q.h"
#include "FileParsing.h"
#include "JumperlessDefines.h"
#include "LEDs.h"

#include "MatrixState.h"
#include "NetManager.h"

#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/structs/io_bank0.h"
#include "pico/time.h" // For hardware timer support
#include <oled.h>
#include <stdio.h>
//#include <Adafruit_MCP4728.h>  // Old blocking library

#include "PersistentStuff.h"
#include <Wire.h>
#include "Commands.h"
#include "Graphics.h"
#include "Probing.h"
#include "Highlighting.h"
#include "LogicAnalyzer.h"
#include "ArduinoStuff.h"

#include "MCP4728.h"  // New library


int i2cSpeed = 400000;

// Compatibility for clangd - these are provided by Arduino.h at compile time
#ifndef abs
#define abs( x ) ( ( x ) < 0 ? -( x ) : ( x ) )
#endif
#ifndef sin
extern double sin( double );
#endif
#ifndef round
extern double round( double );
#endif

#define CSI Serial.write( "\x1B\x5B" );

#define DAC_RESOLUTION 9

float adcRange[ 8 ][ 2 ] = { { -8, 8 }, { -8, 8 }, { -8, 8 }, { -8, 8 }, { 0, 5 } };
float dacOutput[ 2 ] = { 0, 0 };
float railVoltage[ 2 ] = { 0, 0 };

float dacSpread[ 4 ] = { 20.2, 20.2, 20.2, 20.2 };
int dacZero[ 4 ] = { 1650, 1650, 1650, 1650 };

float adcSpread[ 8 ] = { 18.28, 18.28, 18.28, 18.28, 5.0, 17.28, 17.28, 17.28 };
float adcZero[ 8 ] = { 8.0, 8.0, 8.0, 8.0, 0.0, 8.0, 8.0, 8.0 };

/// 0 = output low, 1 = output high, 2 = input, 3 = input pullup, 4 = input
/// pulldown, 5 = unknown
uint8_t gpioState[ 10 ] = {
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
    0xff,
};
uint8_t gpioReading[ 10 ] = {
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }; // 0 = low, 1 = high 2 = floating 3 = unknown

int gpioNet[ 10 ] = { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };

int revisionNumber = 0;

int showReadings = 0;

float adcReadings[ 8 ] = { 0, 0, 0, 0, 0, 0, 0, 0 };

// int adcNet[8] = { -1, -1, -1, -1, -1, -1, -1, -1 };
int showADCreadings[ 8 ] = { 0, 0, 0, 0, 0, 0, 0, 0 };
uint32_t adcReadingColors[ 8 ] = { 0x050505, 0x050505, 0x050505,
                                   0x050505, 0x050505, 0x050505 };
float adcReadingRanges[ 8 ][ 2 ] = {
    { -8.0, 8.0 },
    { -8.0, 8.0 },
    { -8.0, 8.0 },
    { -8.0, 8.0 },
    { 0.0, 5.0 },
};

int inaConnected = 0;
int showINA0[ 3 ] = { 1, 1, 1 }; // 0 = current, 1 = voltage, 2 = power
int showINA1[ 3 ] = { 0, 0, 0 }; // 0 = current, 1 = voltage, 2 = power

int showDAC0 = 0;
int showDAC1 = 0;

int adcCalibration[ 6 ][ 3 ] = { { 0, 0, 0 },
                                 { 0, 0, 0 },
                                 { 0, 0, 0 },
                                 { 0, 0, 0 } }; // 0 = min, 1 = middle, 2 = max,
int dacCalibration[ 4 ][ 3 ] = { { 0, 0, 0 },
                                 { 0, 0, 0 },
                                 { 0, 0, 0 },
                                 { 0, 0, 0 } }; // 0 = min, 1 = middle, 2 = max,

float freq[ 3 ] = { 1, 1, 0 };
uint32_t period[ 3 ] = { 0, 0, 0 };
uint32_t halvePeriod[ 3 ] = { 0, 0, 0 };

// q = square
// s = sinus
// w = sawtooth
// t = stair
// r = random
char mode[ 3 ] = { 'z', 'z', 'z' };

int dacOn[ 3 ] = { 0, 0, 0 };
int amplitude[ 3 ] = { 0, 0, 0 };
int offset[ 3 ] = { 2048, 1650, 0 };
int calib[ 3 ] = { 0, 0, 0 };

MCP4728 mcp;

// MCP4725_PICO dac0_5V(5.0);
// MCP4725_PICO dac1_8V(railSpread);

// MCP4822 dac_rev3; // A is dac0  B is dac1

INA219 INA0( 0x40 );
INA219 INA1( 0x41 );

uint16_t count;
uint32_t lastTime = 0;

// LOOKUP TABLE SINE
uint16_t sine0[ 360 ];
uint16_t sine1[ 360 ];

// PWM state tracking
float gpioPWMFrequency[ 10 ] = { 1000.0, 1000.0, 1000.0, 1000.0, 1000.0,
                                 1000.0, 1000.0, 1000.0, 1000.0, 1000.0 };
float gpioPWMDutyCycle[ 10 ] = { 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5, 0.5 };
bool gpioPWMEnabled[ 10 ] = { false, false, false, false, false,
                              false, false, false, false, false };

// Slow PWM state tracking (for frequencies below 10Hz)
bool gpioSlowPWMEnabled[ 10 ] = { false, false, false, false, false,
                                  false, false, false, false, false };
repeating_timer_t gpioSlowPWMTimers[ 10 ];
volatile uint32_t gpioSlowPWMCounter[ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile uint32_t gpioSlowPWMPeriod[ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile uint32_t gpioSlowPWMDutyTicks[ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

void initGPIO( void ) {
    for ( int i = 0; i < 8; i++ ) {
        int gpio_pin = 0;
        if ( i < 8 ) {         // Regular GPIO pins 0-7 are on pins 20-27
            gpio_pin = i + 20; // Map GPIO 0-7 to pins 20-27

        } else if ( i == 8 ) { // UART TX (pin 0)
            gpio_pin = 0;
        } else if ( i == 9 ) { // UART RX (pin 1)
            gpio_pin = 1;
        }
        gpio_init( gpio_pin );

        switch ( jumperlessConfig.gpio.direction[ i ] ) {
        case 0:                             // output
            gpio_set_dir( gpio_pin, true ); // Set as output
            break;
        default:                             // input
            gpio_set_dir( gpio_pin, false ); // Set as input
            break;
        }

        switch ( jumperlessConfig.gpio.pulls[ i ] ) {
        case 2:                                       // no pull
            gpio_set_pulls( gpio_pin, false, false ); // No pulls
            break;
        case 1:                                      // pullup
            gpio_set_pulls( gpio_pin, true, false ); // Pull up
            break;
        case 0:                                      // pulldown
            gpio_set_pulls( gpio_pin, false, true ); // Pull down
            break;
        case 3:                                     // bus keeper - weakly retains current logical state when not driven
            gpio_set_pulls( gpio_pin, true, true ); // Both pulls enabled = bus keeper
            break;
        default:
            gpio_set_pulls( gpio_pin, false, true ); // No pulls
            break;
        }
    }
}
int reads2[ 30 ][ 2 ];
int readIndex2 = 0;

int foundRolloff = 0;

int lastReading = 0;
int dacSetting = 0;
int reads[ 30 ][ 2 ];
int readIndex = 0;

void initADC( void ) {
    // Initialize pico-sdk ADC for direct hardware access
    adc_init( );

    // Make sure GPIO pins are set up for ADC

    // Note: ADC4 is temperature sensor (internal)    adc_fifo_setup( false, false, 0, false, false );
    adc_fifo_drain( );

    // Set Arduino ADC resolution back to 12 bits for compatibility
    // analogReadResolution( 12 );
    adc_set_clkdiv( 1.0 );
    adc_select_input( 0 );
    adc_set_round_robin( 0 );
    adc_run( false );

    // Re-enable ADC GPIO pins for normal operation
    for ( int i = 0; i < 8; i++ ) {
        adc_gpio_init( 40 + i ); // Jumperless ADCs on pins 40-47
    }

    // Set Arduino ADC resolution to 12 bits for compatibility
    analogReadResolution( 12 );
}

void initDAC( void ) {

    
    initGPIO( );

    Wire.setSDA( 4 );
    Wire.setSCL( 5 );
    Wire.setClock( 1000000 );
    Wire.begin( );

    delayMicroseconds( 100 );

    
    // Try to initialize!
    if ( !mcp.begin( ) ) {
        delay( 3000 );
        Serial.println( "Failed to find MCP4728 chip" );
    }
    pinMode( LDAC, OUTPUT );

    digitalWrite( LDAC, HIGH );

    // // Vref = MCP_VREF_VDD, value = 0, 0V
    mcp.setChannelValue( MCP4728_CHANNEL_A, 1650 );
    mcp.setChannelValue( MCP4728_CHANNEL_B, 1650 );
    mcp.setChannelValue( MCP4728_CHANNEL_C, 1660 );
    mcp.setChannelValue( MCP4728_CHANNEL_D, 1641 ); // 1650 is roughly 0V
    digitalWrite( LDAC, LOW );

    delayMicroseconds( 1000 );

    setRailsAndDACs( 0 );
    delayMicroseconds( 1000 );
}

int findI2CAddress( int sdaPin, int sclPin, int i2cNumber, int print ) {
    int address = -1;
    for ( int i = 0; i < 128; i++ ) {
        if ( i2cNumber == 0 ) {
            Wire.beginTransmission( i );
        } else {
            Wire1.beginTransmission( i );
        }
        int error = 0;
        if ( i2cNumber == 0 ) {
            error = Wire.endTransmission( );
        } else {
            error = Wire1.endTransmission( );
        }

        if ( error == 0 ) {
            address = i;
            if ( print == 1 ) {
                Serial.print( "Found I2C address: " );
                Serial.println( address );
            }
            break;
        }
    }
    return address;
}

int initI2C( int sdaPin, int sclPin, int speed ) {

    // Serial.println("initI2C");
    static int i2c1Pins[ 3 ] = { 26, 27, 100000 };
    static int i2c0Pins[ 3 ] = { 4, 5, 100000 };

    int gpioI2Cmap[ 15 ][ 3 ] = {
        { 0, 0, 0 },
        { 1, 1, 0 },
        { 4, 0, 0 },
        { 5, 1, 0 },
        { 6, 0, 1 },
        { 7, 1, 1 },
        { 20, 0, 0 },
        { 21, 1, 0 },
        { 22, 0, 1 },
        { 23, 1, 1 },
        { 24, 0, 0 },
        { 25, 1, 0 },
        { 26, 0, 1 },
        { 27, 1, 1 },
    };
    // I2C0 is taken by current sensors and dac
    // maybe bitbang I2C0?
    int sdaFound = 0;
    int sclFound = 0;

    int portFound = 1; // right now only I2C1 is available
    for ( int i = 0; i < 15; i++ ) {
        if ( gpioI2Cmap[ i ][ 0 ] == sdaPin ) { // && gpioI2Cmap[i][2] == 1) {
            sdaFound = i;
        }
        if ( gpioI2Cmap[ i ][ 0 ] == sclPin ) { // && gpioI2Cmap[i][2] == 1) {

            sclFound = i;
        }
        if ( sdaFound != 0 && sclFound != 0 ) {
            break;
        }
    }


    if ( sdaFound == 0 || sclFound == 0 ) {
        // Serial.println("Failed to find I2C pins");
        return -1;
    } else if ( portFound == 0 ) {

        Wire.setSDA( sdaPin );
        Wire.setSCL( sclPin );
        Wire.setClock( speed );
        Wire.begin( );
        if ( i2c0Pins[ 0 ] == sdaPin && i2c0Pins[ 1 ] == sclPin &&
             i2c0Pins[ 2 ] == speed ) {
            return gpioI2Cmap[ sdaFound ][ 2 ] +
                   10; // returns 10 if the pins are already set
        }
        i2c0Pins[ 0 ] = sdaPin;
        i2c0Pins[ 1 ] = sclPin;
        i2c0Pins[ 2 ] = speed;
        i2cSpeed = speed;

        return gpioI2Cmap[ sdaFound ][ 2 ];
    } else if ( portFound == 1 ) {

        gpioState[ gpioDef[ sdaPin - 20 ][ 2 ] ] = 6;
        gpioState[ gpioDef[ sclPin - 20 ][ 2 ] ] = 6;
        gpio_function_map[ gpioDef[ sdaPin - 20 ][ 2 ] ] = GPIO_FUNC_I2C;
        gpio_function_map[ gpioDef[ sclPin - 20 ][ 2 ] ] = GPIO_FUNC_I2C;

        gpio_set_pulls( sdaPin, true, false ); // Enable pull-up on SDA
        gpio_set_pulls( sclPin, true, false ); // Enable pull-up on SCL

        Wire1.setSDA( sdaPin );
        Wire1.setSCL( sclPin );
        Wire1.setClock( speed );
        Wire1.begin( );

        i2cSpeed = speed;

        if ( i2c1Pins[ 0 ] == sdaPin && i2c1Pins[ 1 ] == sclPin &&
             i2c1Pins[ 2 ] == speed ) {
            return gpioI2Cmap[ sdaFound ][ 2 ] +
                   10; // returns 11 if the pins are already set
        }
        i2c1Pins[ 0 ] = sdaPin;
        i2c1Pins[ 1 ] = sclPin;
        i2c1Pins[ 2 ] = speed;

        return gpioI2Cmap[ sdaFound ][ 2 ];
    }

    // Serial.println("Failed to find I2C pins");
    return -1;

    
}

int getGPIOIndexFromPin(int pin) {
    for (int i = 0; i < 10; i++) {
        if (gpioDef[i][0] == pin) {
            return i;
        }
    }
    Serial.print("getGPIOIndexFromPin: ");
    Serial.println(pin);
    Serial.println("Failed to find GPIO index");
    Serial.flush();
    return -1;
}

int convertPullToJumperless(int pull) {
    switch (pull) {
        case 0: return 2; // no pull
        case 1: return 1; // pullup
        case 2: return 0; // pulldown
        case 3: return 3; // bus keeper
    }
    return 2; // no pull
    }


void setGPIO( void ) {
    // Restore GPIO configurations from jumperlessConfig after
    // refreshConnections()
    for ( int i = 0; i < 10; i++ ) {
        uint8_t gpio_pin = gpioDef[ i ][ 0 ];

        // Set direction
        if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
            gpio_set_dir( gpio_pin, true ); // Set as output
        } else {
            gpio_set_dir( gpio_pin, false ); // Set as input
        }

        // Set pull resistors and update gpioState for animation system
        switch ( jumperlessConfig.gpio.pulls[ i ] ) {
        case 0: // pulldown
            gpio_set_pulls( gpio_pin, false, true );
            if ( jumperlessConfig.gpio.direction[ i ] == 1 ) { // input
                gpioState[ i ] = 4;                            // input with pulldown
            }
            break;
        case 1: // pullup
            gpio_set_pulls( gpio_pin, true, false );
            if ( jumperlessConfig.gpio.direction[ i ] == 1 ) { // input
                gpioState[ i ] = 3;                            // input with pullup
            }
            break;
        case 2: // no pull
            gpio_set_pulls( gpio_pin, false, false );
            if ( jumperlessConfig.gpio.direction[ i ] == 1 ) { // input
                gpioState[ i ] = 2;                            // input with no pull
            }
            break;
        case 3: // bus keeper - weakly retains current logical state when not driven
            gpio_set_pulls( gpio_pin, true, true );
            if ( jumperlessConfig.gpio.direction[ i ] == 1 ) { // input
                gpioState[ i ] = 7;                            // bus keeper mode
            }
            break;
        default:
            gpio_set_pulls( gpio_pin, false, false );
            if ( jumperlessConfig.gpio.direction[ i ] == 1 ) { // input
                gpioState[ i ] = 2;                            // input with no pull
            }
            break;
        }

        // Set initial output state for output pins
        if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
            gpio_put( gpio_pin, gpioState[ i ] );
        }
    }
}

int gpioReadWithFloating(
    int pin, unsigned long usDelay ) { // 2 = floating, 1 = high, 0 = low
    enum measuredState state = unknownState;
    int settleDelay = 18;
    int reading = -1;
    int readingPulldown = -1;
    int readingPullup = -1;
    readingGPIO = true;

    if (gpio_get_function(pin) == GPIO_FUNC_I2C || gpio_get_function(pin) == GPIO_FUNC_UART || pin < 2){
        return 0;
        return gpio_get( pin );
    }

    int dir = gpio_get_dir( pin );
    if ( dir == 1 ) { // we'll just quickly set the pin to input and read it and
                      // then set it back to whatever it was
    
        if (pin >1 && gpio_get_function(pin) != GPIO_FUNC_I2C && gpio_get_function(pin) != GPIO_FUNC_UART){
        gpio_set_input_enabled(pin, false);
        gpio_set_input_enabled(pin, true);
        }
        return gpio_get( pin );
    }

    
    int pullupState = 0;
    int pulldownState = 0;
    //if ( gpio_is_pulled_up( pin ) == 0 && gpio_is_pulled_down( pin ) == 0 ) {
        // pullupState = -1;
        // pulldownState = -1;
    //} else 
    if ( gpio_is_pulled_up( pin ) == 1 ) {
        if ( gpio_get( pin ) == 0 ) { /// don't mess with the pullups if the pin is
                                      /// already being pulled down
            // state = high;
            return low;
        }
        pullupState = 1;
        gpio_disable_pulls( pin );
    } else if ( gpio_is_pulled_down( pin ) == 1 ) {
        pulldownState = 1;
        if ( gpio_get( pin ) == 1 ) { /// don't mess with the pullups if the pin is
                                      /// already being pulled up
            if (pin >1 && gpio_get_function(pin) != GPIO_FUNC_I2C && gpio_get_function(pin) != GPIO_FUNC_UART){

            gpio_set_input_enabled(pin, false); //? Aha! the eratta fix!
            delayMicroseconds(1);
            gpio_set_input_enabled(pin, true);

            }
            //gpio_set_pulls(pin, pullupState, pulldownState);
            return high;
        }

        gpio_disable_pulls( pin );
    }

    gpio_set_input_enabled( pin, true );
    delayMicroseconds( settleDelay );

    reading = gpio_get( pin );
    gpio_set_input_enabled( pin, false );

    if ( reading != 0 ) { // if the pin is high, check with pulldowns to make sure
                          // it's not floating high

        gpio_set_pulls( pin, false, true );

        gpio_set_input_enabled( pin, true );
        delayMicroseconds( settleDelay );
        readingPulldown = gpio_get( pin );
        gpio_set_input_enabled( pin, false );

        if ( readingPulldown == 0 ) {
            state = floating;
        } else {
            state = high;
        }

    } else { // if the pin is low, check with pullups to make sure it's not
             // floating low
        gpio_set_pulls( pin, true, false );

        gpio_set_input_enabled( pin, true );
        delayMicroseconds( settleDelay );
        readingPullup = gpio_get( pin );
        gpio_set_input_enabled( pin, false );

        if ( readingPullup == 1 ) {
            state = floating;
        } else {
            state = low;
        }
    }

    gpio_set_pulls( pin, pullupState,
                    pulldownState ); // set the pullups and pulldowns back to
                                     // whatever they were

    if ( dir == 1 ) {
        /// gpio_set_dir(pin, true); //set the pin back to whatever it was
    }

    readingGPIO = false;

    return state;
}

gpio_function_t gpio_function_map[ 10 ] = {
    GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO,
    GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO, GPIO_FUNC_SIO };

gpio_function_name_struct gpio_function_names[ 15 ] = {

    { GPIO_FUNC_HSTX, "HSTX" },
    { GPIO_FUNC_SPI, "SPI" },
    { GPIO_FUNC_UART, "UART" },
    { GPIO_FUNC_I2C, "I2C" },
    { GPIO_FUNC_PWM, "PWM" },
    { GPIO_FUNC_SIO, "SIO" },
    { GPIO_FUNC_PIO0, "PIO0" },
    { GPIO_FUNC_PIO1, "PIO1" },
    { GPIO_FUNC_PIO2, "PIO2" },
    { GPIO_FUNC_GPCK, "GPCK" },
    { GPIO_FUNC_XIP_CS1, "XIP_CS1" },
    { GPIO_FUNC_CORESIGHT_TRACE, "CORESIGHT" },
    { GPIO_FUNC_USB, "USB" },
    { GPIO_FUNC_UART_AUX, "UART_AUX" },
    { GPIO_FUNC_NULL, "NULL" } };

char* gpio_function_name( gpio_function_t function ) {
    char* name = nullptr;
    for ( int i = 0; i < 15; i++ ) {
        if ( gpio_function_names[ i ].function == function ) {
            name = gpio_function_names[ i ].name;
        }
    }
    return name;
}

void printGPIOState( void ) {

    
    Serial.println( );
    Serial.println(
        "   number:\t\b1\t\b2\t\b3\t\b4\t\b5\t\b6\t\b7\t\b8\t\bTx\t\bRx" );
    
    Serial.print( "      net:\t" );
    for ( int i = 0; i < 10; i++ ) {
        if ( gpioNet[ i ] == -1 ) {
            Serial.print( "." );
        } else {
            Serial.print( gpioNet[ i ] );
        }
        Serial.print( "\t" );
    }

    Serial.println( );

    Serial.print( " function:\t" );
    for ( int i = 0; i < 10; i++ ) {
        gpio_function_map[ i ] = gpio_get_function( gpioDef[ i ][ 0 ] );

        Serial.print( gpio_function_names[ gpio_get_function( gpioDef[ i ][ 0 ] ) ].name );

    
        Serial.print( "\t" );
    }
    Serial.println( );
    Serial.print( "set direction:\t" );
    for ( int i = 0; i < 8; i++ ) {
        switch ( jumperlessConfig.gpio.direction[ i ] ) {
        case 0:
            Serial.print( "out" );
            break;
        case 1:
            Serial.print( "in" );
            break;
        }
        Serial.print( "\t" );
    }
    Serial.println( );

    Serial.print( "direction:\t" );
    for ( int i = 0; i < 8; i++ ) {
        switch ( gpio_get_dir( i + 20 ) ) {
        case 1:
            Serial.print( "out" );
            break;
        case 0:
            Serial.print( "in" );
            break;
        }
        Serial.print( "\t" );
    }
    Serial.println( );
    Serial.print( "    pulls:\t" );
    for ( int i = 0; i < 10; i++ ) {
        uint8_t pin = i + 20;
        if ( i == 8 ) {
            pin = 0;
        } else if ( i == 9 ) {
            pin = 1;
        }
        uint8_t pulls;
        bool pullup_enabled = gpio_is_pulled_up( pin );
        bool pulldown_enabled = gpio_is_pulled_down( pin );

        if ( pullup_enabled && pulldown_enabled ) {
            pulls = 3; // bus keeper
        } else if ( pullup_enabled ) {
            pulls = 1; // pullup
        } else if ( pulldown_enabled ) {
            pulls = 0; // pulldown
        } else {
            pulls = 2; // no pull
        }

        switch ( pulls ) {
        case 0:
            Serial.print( "down" );
            break;
        case 1:
            Serial.print( "up" );
            break;
        case 2:
            Serial.print( "none" );
            break;
        case 3:
            Serial.print( "keeper" );
            break;
        }
        Serial.print( "\t" );
    }
    Serial.println( );
    Serial.print( "  reading:\t" );
    for ( int i = 0; i < 10; i++ ) {
        switch ( gpioReading[ i ] ) {
        case 0:
            Serial.print( "low" );
            break;
        case 1:
            Serial.print( "high" );
            break;
        case 2:
            Serial.print( "float" );
            break;
        case 3:
            Serial.print( "?" );
            break;
        }
        Serial.print( "\t" );
    }
    Serial.println( );
}

uint32_t gpioReadingColors[ 10 ] = { 0x050507, 0x050507, 0x050507, 0x050507,
                                     0x050507, 0x050507, 0x050507, 0x050507,
                                     0x050507, 0x050507 };

uint32_t gpioIdleColors[ 10 ] = { 0x050206, 0x050307, 0x040407, 0x040407,
                                  0x040407, 0x040407, 0x040407, 0x040407,
                                  0x040407, 0x040407 };

uint8_t gpioIdleHues[ 10 ] = { 0, 25, 50, 75, 100, 125, 150, 175, 200, 225 };


// this is used to store the output state of the GPIO pins
// 0 = low, 1 = high, 2 = input
int gpioOutput[ 10 ] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
volatile bool readingGPIO = false;
void readGPIO( void ) {

    if ( false ) {
        return;
    }
    for ( int i = 0; i < 10; i++ ) { // if you want to read the UART pins, set this to 10

        // **IMPROVED**: Skip logic analyzer pins during capture, but allow other GPIO reading
        // Logic analyzer uses GPIO 20-27 (i=0 through i=7), leave UART pins (i=8,9) alone
        if ( logicAnalyzer.is_running( ) || logicAnalyzer.is_armed( ) || flashingArduino == true ) {
            // Skip reading logic analyzer pins during capture to avoid interference
            // Keep previous state to avoid display issues
            return;
           continue;
        }

        if (gpio_get_function(gpioDef[i][0]) == GPIO_FUNC_UART) {
            continue;
        }


        uint8_t pin = gpioDef[ i ][ 0 ];
        if ( i == 8 ) {
            pin = gpioDef[ i ][ 0 ];

        } else if ( i == 9 ) {
            pin = gpioDef[ i ][ 0 ];
        }


        if ( gpioNet[ i ] == -1 ) {
            gpioState[ i ] = 4;
            // continue;
        } else if ( gpioNet[ i ] == -2 ) {
            // gpioState[i] = 6;
            continue;
        } else if ( gpioNet[ i ] == -3 ) {
            // gpioState[i] = 5;
            continue;
        } else if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
            
            /*if ( gpioState[ i ] == 0 ) {
                // gpio_set_dir(gpioDef[i][0], true);
                // gpio_put(gpioDef[i][0], 0);
            } else if ( gpioState[ i ] == 1 ) {
                // gpio_set_dir(gpioDef[i][0], true);
                // gpio_put(gpioDef[i][0], 1);
            }
            */
            
            gpioOutput[ i ] = gpioState[ i ];

            // continue;
        }

        if ( gpioNet[ i ] >= 0 ) {
            int reading = 0;

            if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
                reading = gpio_get_out_level( gpioDef[ i ][ 0 ] );

                switch ( reading ) {
                case 0:
                    gpioReading[ i ] = 0;
                    gpioReadingColors[ i ] = 0x052302;
                    break;
                case 1:
                    gpioReading[ i ] = 1;
                    gpioReadingColors[ i ] = 0x230205;
                    break;
                }
                continue;
            } else if ( gpioState[ i ] == 6 ) {
                reading = gpio_get( gpioDef[ i ][ 0 ] );
            } else if ( gpioState[ i ] == 7 ) {
                // Bus keeper mode - just read current state, no floating detection
                // needed
                reading = gpio_get( gpioDef[ i ][ 0 ] );
            } else {
                reading = gpioReadWithFloating(gpioDef[ i ][ 0 ] ); // check if the pin is floating or has a state
            }

            delayMicroseconds( 5 );
            
            switch ( reading ) {

            case 0:
                gpioReading[ i ] = 0;
                gpioReadingColors[ i ] = 0x002004;
                break;
            case 1:

                gpioReading[ i ] = 1;
                gpioReadingColors[ i ] = 0x200400;
                break;
            case 2:

                gpioReading[ i ] = 2;
            
                gpioReadingColors[ i ] = 0x040408; // just in case it isn't

                break;
            }
        } else {
            gpioReading[ i ] = 3;
        }
        
    }
}

void setRailsAndDACs( int saveEEPROM ) {
    setTopRail( jumperlessConfig.dacs.top_rail, 1, 0 );
    // delay(10);
    setBotRail( jumperlessConfig.dacs.bottom_rail, 1, 0 );
    // delay(10);
    setDac0voltage( jumperlessConfig.dacs.dac_0, 1, 0 );
    // delay(10);
    setDac1voltage( jumperlessConfig.dacs.dac_1, 1, saveEEPROM );
    // delay(10);
}
void setTopRail( float value, int save, int saveEEPROM ) {

    int dacValue = ( value * 4095 / dacSpread[ 2 ] ) + dacZero[ 2 ];

    if ( dacValue > 4095 ) {
        dacValue = 4095;
    } else if ( dacValue < 0 ) {
        dacValue = 0;
    }

    digitalWrite( LDAC, HIGH );
    mcp.setChannelValue( MCP4728_CHANNEL_C, dacValue );
    digitalWrite( LDAC, LOW );
    if ( save ) {
        jumperlessConfig.dacs.top_rail = value;
        railVoltage[ 0 ] = value; // Keep legacy variable in sync
        configChanged = true;
    }
    if ( saveEEPROM ) {

        saveVoltages( jumperlessConfig.dacs.top_rail,
                      jumperlessConfig.dacs.bottom_rail, jumperlessConfig.dacs.dac_0,
                      jumperlessConfig.dacs.dac_1 );
    }
}

void setBotRail( float value, int save, int saveEEPROM ) {

    int dacValue = ( value * 4095 / dacSpread[ 3 ] ) + dacZero[ 3 ];

    if ( dacValue > 4095 ) {
        dacValue = 4095;
    } else if ( dacValue < 0 ) {
        dacValue = 0;
    }

    digitalWrite( LDAC, HIGH );
    mcp.setChannelValue( MCP4728_CHANNEL_D, dacValue );
    digitalWrite( LDAC, LOW );
    if ( save ) {
        jumperlessConfig.dacs.bottom_rail = value;
        railVoltage[ 1 ] = value; // Keep legacy variable in sync
        configChanged = true;
    }
    if ( saveEEPROM ) {

        saveVoltages( jumperlessConfig.dacs.top_rail,
                      jumperlessConfig.dacs.bottom_rail, jumperlessConfig.dacs.dac_0,
                      jumperlessConfig.dacs.dac_1 );
    }
}


float getDacVoltage( int dac ) {
    if ( dac == 0 ) {
        return jumperlessConfig.dacs.dac_0;
    } else if ( dac == 1 ) {
        return jumperlessConfig.dacs.dac_1;
    } else if ( dac == 2 ) {
        return jumperlessConfig.dacs.top_rail;
    } else if ( dac == 3 ) {
        return jumperlessConfig.dacs.bottom_rail;
    }
    return 0;
}

void setDac0voltage( float voltage, int save, int saveEEPROM,
                     bool checkProbePower ) {
    // int dacValue = (voltage * 4095 / 19.8) + 1641;
    int dacValue = ( voltage * 4095 / dacSpread[ 0 ] ) + dacZero[ 0 ];

    if ( checkProbePower && probePowerDAC == 0 &&
         ( voltage > 5.0 || voltage < -0.01 ) ) {
        Serial.println(
            "DAC 0 connected to probe LEDs, swapping LED power to DAC 1" );
        // removeBridgeFromNodeFile(DAC0, ROUTABLE_BUFFER_IN, netSlot, 0, 0);
        probePowerDAC = 1;
        probePowerDACChanged = true;
        routableBufferPower( 1, 0 );
    }

    if ( dacValue > 4095 ) {
        dacValue = 4095;
    }
    if ( dacValue < 0 ) {
        dacValue = 0;
    }

    digitalWrite( LDAC, HIGH );
    // delay(10);
    if ( mcp.setChannelValue( MCP4728_CHANNEL_A, dacValue ) == false ) {
        // delay(3000);
        Serial.println( "Failed to set DAC0 value" );
    }
    // delay(10);
    digitalWrite( LDAC, LOW );
    // if (save) {

    dacOutput[ 0 ] = voltage;
    // }

    if ( saveEEPROM ) {

        saveVoltages( railVoltage[ 0 ], railVoltage[ 1 ], dacOutput[ 0 ], dacOutput[ 1 ] );
    }
}

void setDac0voltage( uint16_t inputCode ) {
    digitalWrite( LDAC, HIGH );
    mcp.setChannelValue( MCP4728_CHANNEL_A, inputCode );
    digitalWrite( LDAC, LOW );
}

void setDac1voltage( float voltage, int save, int saveEEPROM,
                     bool checkProbePower ) {

    int dacValue = ( voltage * 4095 / dacSpread[ 1 ] ) + dacZero[ 1 ];

    if ( dacValue > 4095 ) {
        dacValue = 4095;
    }
    if ( dacValue < 0 ) {
        dacValue = 0;
    }
    if ( checkProbePower && probePowerDAC == 1 &&
         ( voltage > 5.0 || voltage < -0.01 ) ) {

        Serial.println(
            "DAC 1 connected to probe LEDs, \n\rswapping LED power to DAC 0" );
        // removeBridgeFromNodeFile(DAC0, ROUTABLE_BUFFER_IN, netSlot, 0, 0);
        probePowerDAC = 0;
        probePowerDACChanged = true;
        routableBufferPower( 1, 0 );
    }

    digitalWrite( LDAC, HIGH );
    mcp.setChannelValue( MCP4728_CHANNEL_B, dacValue );
    digitalWrite( LDAC, LOW );
    // if (save) {

    dacOutput[ 1 ] = voltage;
    //  }

    if ( saveEEPROM ) {

        saveVoltages( railVoltage[ 0 ], railVoltage[ 1 ], dacOutput[ 0 ], dacOutput[ 1 ] );
    }
}

void setDac1voltage( uint16_t inputCode ) {
    digitalWrite( LDAC, HIGH );
    mcp.setChannelValue( MCP4728_CHANNEL_B, inputCode );
    digitalWrite( LDAC, LOW );
}

uint8_t csToPin[ 16 ] = { 8, 9, 10, 11, 12, 13, 14, 15, 0, 1, 2, 3, 4, 5, 6, 7 };
// uint8_t csToPin[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
// 15};
uint16_t chipMask[ 16 ] = {
    0b0000000000000001, 0b0000000000000010, 0b0000000000000100,
    0b0000000000001000, 0b0000000000010000, 0b0000000000100000,
    0b0000000001000000, 0b0000000010000000, 0b0000000100000000,
    0b0000001000000000, 0b0000010000000000, 0b0000100000000000,
    0b0001000000000000, 0b0010000000000000, 0b0100000000000000,
    0b1000000000000000 };

void setCSex( int chip, int value ) {

    if ( chip > 11 ) {
        return;
    }

    if ( value > 0 ) {
        gpio_put( chip + 28, 1 );
        // digitalWrite(chip + 28, HIGH);
        //  Serial.println(chip+28);
    } else {
        gpio_put( chip + 28, 0 );
        // digitalWrite(chip + 28, LOW);
        //  Serial.println(chip+28);
    }
}

void erattaClearGPIO( int gpio ) {

    int freeYL = -1;
    if ( gpio == -1 ) {

        // check free y connections on chip L
        for ( int i = 0; i < 8; i++ ) {

            if ( ch[ 11 ].yStatus[ i ] == -1 ) {
                freeYL = i;
                break;
            }
        }

        if ( freeYL == -1 ) {
            // idk do something else if all y connections are taken
            return;
        }

        // we should check if thse connections are already made

        sendXYraw( 11, 15, freeYL, 1 );

        for ( int i = 0; i < 8; i++ ) {
            if ( gpio_get( gpioDef[ i ][ 0 ] ) == 0 ) {
                continue;
            }
            sendXYraw( 11, 4 + i, freeYL, 1 );


            sendXYraw( 11, 4 + i, freeYL, 0 );
        }

        sendXYraw( 11, 15, freeYL, 0 );
    }
}

void writeGPIOex( int value, uint8_t pin ) {}

void initINA219( void ) {

    if ( !INA0.begin( ) || !INA1.begin( ) ) {
        // Remove blocking delay - just log the error
        Serial.println( "Failed to find INA219 chip" );
    }


    INA0.setShuntSamples(2);
    INA1.setShuntSamples(2);

    INA0.setMaxCurrentShunt( 1, 2.0 );
    INA1.setMaxCurrentShunt( 1, 2.0 );

    INA0.setBusVoltageRange( 16 );
    INA1.setBusVoltageRange( 16 );


}

void setDacByNumber( int dac, float voltage, int save, int saveEEPROM,
                     bool checkProbePower ) {
    switch ( dac ) {
    case 0:
        setDac0voltage( voltage, save, saveEEPROM, checkProbePower );
        break;
    case 1:
        setDac1voltage( voltage, save, saveEEPROM, checkProbePower );
        break;
    case 2:
        setTopRail( voltage, save, saveEEPROM );
        break;
    case 3:
        setBotRail( voltage, save, saveEEPROM );
        break;
    }
}

void dacSine( int resolution ) {
    uint16_t i;
    switch ( resolution ) {
    case 0 ... 5:
        for ( i = 0; i < 32; i++ ) {
            setDac1voltage( DACLookup_FullSine_5Bit[ i ] );
        }
        break;
    case 6:
        for ( i = 0; i < 64; i++ ) {
            setDac1voltage( DACLookup_FullSine_6Bit[ i ] );
        }
        break;
    case 7:
        for ( i = 0; i < 128; i++ ) {
            setDac1voltage( DACLookup_FullSine_7Bit[ i ] );
        }
        break;

    case 8:
        for ( i = 0; i < 256; i++ ) {
            setDac1voltage( DACLookup_FullSine_8Bit[ i ] );
        }
        break;

    case 9 ... 12:
        for ( i = 0; i < 512; i++ ) {
            setDac1voltage( DACLookup_FullSine_9Bit[ i ] );
        }
        break;
    }
}

void chooseShownReadings( void ) {

    showADCreadings[ 0 ] = 0;
    showADCreadings[ 1 ] = 0;
    showADCreadings[ 2 ] = 0;
    showADCreadings[ 3 ] = 0;
    showADCreadings[ 4 ] = 0;
    showADCreadings[ 5 ] = 0;
    showADCreadings[ 6 ] = 0;
    showADCreadings[ 7 ] = 0;

    showINA0[ 0 ] = 0;
    showINA0[ 1 ] = 0;
    showINA0[ 2 ] = 0;

    inaConnected = 0;

    for ( int i = 0; i < numberOfPaths; i++ ) {

        if ( path[ i ].node1 == ADC0 || path[ i ].node2 == ADC0 ) {
            showADCreadings[ 0 ] = path[ i ].net;
        }

        if ( path[ i ].node1 == ADC1 || path[ i ].node2 == ADC1 ) {
            showADCreadings[ 1 ] = path[ i ].net;
        }

        if ( path[ i ].node1 == ADC2 || path[ i ].node2 == ADC2 ) {
            showADCreadings[ 2 ] = path[ i ].net;
        }

        if ( path[ i ].node1 == ADC3 || path[ i ].node2 == ADC3 ) {
            showADCreadings[ 3 ] = path[ i ].net;
        }

        if ( path[ i ].node1 == ADC4 || path[ i ].node2 == ADC4 ) {
            showADCreadings[ 4 ] = path[ i ].net;
        }

        if ( path[ i ].node1 == ISENSE_PLUS || path[ i ].node1 == ISENSE_PLUS ||
             path[ i ].node2 == ISENSE_MINUS || path[ i ].node2 == ISENSE_MINUS ) {
            // Serial.println(showReadings);

            inaConnected = 1;

            showINA0[ 0 ] = 1;
            showINA0[ 1 ] = 0;
            showINA0[ 2 ] = 0;
        }
    }
    if ( inaConnected == 0 ) {
        showINA0[ 0 ] = 0;
        showINA0[ 1 ] = 0;
        showINA0[ 2 ] = 0;
        // showReadings = 3;
    }

    for ( int i = 0; i < 10; i++ ) {

        gpio_function_t fun = gpio_get_function( gpioDef[ i ][ 0 ] );

        if ( fun != gpio_function_map[ i ] ) {
            gpio_function_map[ i ] = fun;
        }
    }
}

int handleHighlights( int probeReading ) {

    if ( probeReading <= 0 ) {
        return probeReading;
    }

    if ( brightenedNet > 0 ) {
        return probeToggle( );
    }

    return probeReading;
}

unsigned long gpioToggleFrequency = 250; // ms

int highlightInteractable[ 10 ] = { RP_GPIO_0, RP_GPIO_1, RP_GPIO_2,
                                    RP_GPIO_3, RP_GPIO_4, RP_GPIO_5,
                                    RP_GPIO_6, RP_GPIO_7, RP_GPIO_8 };

int probeToggle( void ) {

    int buttonState = checkProbeButton( );

    if ( buttonState == 0 ) {
        return -1; // no button pressed
    }

    // Handle DAC voltage control for nets 4 and 5
    if ( brightenedNet == 4 || brightenedNet == 5 ) {
        float currentVoltage = getDacVoltage( brightenedNet == 4 ? 0 : 1 );
        float dacStep = 0.25; // Default step size
        float newVoltage = currentVoltage;

        if ( buttonState == 2 ) { // Connect button - increase voltage
            newVoltage = currentVoltage + dacStep;
            if ( newVoltage > 8.0 )
                newVoltage = 8.0;        // Clamp to max
        } else if ( buttonState == 1 ) { // Disconnect button - decrease voltage
            newVoltage = currentVoltage - dacStep;
            if ( newVoltage < -8.0 )
                newVoltage = -8.0; // Clamp to min
        }

        // Set the new voltage
        if ( brightenedNet == 4 ) {
            setDac0voltage( newVoltage, 1, 0, true );
            jumperlessConfig.dacs.dac_0 = newVoltage;
        } else {
            setDac1voltage( newVoltage, 1, 0, true );
            jumperlessConfig.dacs.dac_1 = newVoltage;
        }

        // Reset the highlight timer to keep DAC highlighted during adjustment
        highlightTimer = millis( );

        // Update display - show only current voltage
        Serial.print( "\r                                 \r" );
        Serial.printf( "DAC %d   %0.2f V", brightenedNet == 4 ? 0 : 1, newVoltage );
        Serial.flush( );

        char oledString[ 30 ];
        sprintf( oledString, "DAC %d\n%0.2f V", brightenedNet == 4 ? 0 : 1,
                 newVoltage );
        oled.clearPrintShow( oledString, 2, true, true, true );

        return brightenedNet; // Return the DAC net number
    }

    if ( buttonState == 2 ) { // connect button
        int toggleResult = toggleGPIO( 2, -1 );

        if ( toggleResult >= 0 ) {
            return toggleResult;
        } else {
            if ( brightenedNet > 0 ) {
                return -6; // no net highlighted - connect button pressed
            }
            return -3; // no gpio connected - connect button pressed
        }
    }

    if ( buttonState == 1 ) { // disconnect button

        if ( brightenedNet > 0 ) {
            int toggleResult = toggleGPIO( 2, -1, 1 );
            if ( toggleResult != -2 ) { //-2 means gpio is not connected
                brightenedNet = 0;
                return -4; // no gpio connected - disconnect button pressed
            } else {
                return -5; // gpio is connected - disconnect button pressed
            }
        } else {

            return -2; // no net highlighted - disconnect button pressed
        }
    }

    return -1;
}

int toggleGPIO( int lowHigh, int gpio, int onlyCheck ) {

    int gpioOutputFound = -2;
    if ( gpio < 0 || gpio > 9 ) {
        for ( int i = 0; i < 10; i++ ) {
            if ( gpioNet[ i ] == brightenedNet ) {
                if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
                    gpio = gpioDef[ i ][ 0 ];
                    gpioOutputFound = i;
                }
                break;
            }
        }
        if ( gpioOutputFound == -2 ) {
            return -2;
        }
        // return -2;
    }

    if ( onlyCheck == 1 ) {
        return gpioOutputFound;
    }

    if ( jumperlessConfig.gpio.direction[ gpioOutputFound ] == 0 ) {

        if ( lowHigh == 0 ) {
            gpio_put( gpio, 0 );
            gpioState[ gpioDef[ gpioOutputFound ][ 2 ] ] = 0;
            Serial.print( "\r                      \r" );
            Serial.print( " gpio " );
            Serial.print( gpioDef[ gpioOutputFound ][ 2 ] + 1 );
            // printNodeOrName(gpioDef[gpioOutputFound][1], 1);
            Serial.print( "\t" );
            Serial.print( gpio_get_out_level( gpio ) ? "high" : "low" );
            Serial.print( " > " );
            Serial.print( "low" );
            Serial.flush( );
            return 0;
        } else if ( lowHigh == 1 ) {
            gpio_put( gpio, 1 );
            gpioState[ gpioDef[ gpioOutputFound ][ 2 ] ] = 1;
            Serial.print( "\r                      \r" );
            Serial.print( " gpio " );
            // Serial.print(gpioDef[gpioOutputFound][0]);
            Serial.print( gpioDef[ gpioOutputFound ][ 2 ] + 1 );
            Serial.print( "\t" );
            Serial.print( gpio_get_out_level( gpio ) ? "high" : "low" );
            Serial.print( " > " );
            Serial.print( "high" );
            // Serial.print();
            Serial.flush( );
            return 1;
        } else {
            bool currentState = gpio_get_out_level( gpio );
            gpioState[ gpioDef[ gpioOutputFound ][ 2 ] ] = !currentState;
            gpio_put( gpio, !currentState );

            Serial.print( " gpio " );
            Serial.print( gpioDef[ gpioOutputFound ][ 2 ] + 1 );
            Serial.print( "\t " );
            Serial.print( currentState ? "high" : "low" );
            Serial.print( " > " );
            Serial.print( !currentState ? "high" : "low" );
            Serial.println( );
            Serial.flush( );
            return !currentState;
        }
    }
    return -1;
}

float railSpread = 17.88;

void showLEDmeasurements( void ) {

    for ( int i = 0; i < 8; i++ ) {
        int samples = 16;

        float adcReading;

        int bs = 0;

        int numReadings = 0;

        uint32_t color = 0x000000;

        if ( showADCreadings[ i ] > 0 && showADCreadings[ i ] <= numberOfNets ) {
            numReadings++;
            adcReading = readAdcVoltage( i, samples );
            adcReadings[ i ] = adcReading;
            color = measurementToColor( adcReading, adcRange[ i ][ 0 ], adcRange[ i ][ 1 ] );

            int brightness =
                LEDbrightnessSpecial +
                (int)abs( adcReading * 2.0 ); // map(abs((adcReading*10)), 0, 80,
                                              // -LEDbrightnessSpecial, 150);
            if ( brightness <= 4 ) {
                brightness = 4;
            } else if ( brightness > 100 ) {
                brightness = 100;
            }

            if ( jumperlessConfig.display.lines_wires == 0 ||
                 numberOfShownNets > MAX_NETS_FOR_WIRES ) {
                lightUpNet( showADCreadings[ i ], -1, 1, brightness, 0, 0, color );
            }
            // Serial.println(brightness);
            int scaleVoltage = map( (int)abs( adcReading ), 0, 8, -30, 70 );
            // Serial.println(scaleVoltage);
            int scaledBrightness = map( brightness, LEDbrightnessSpecial,
                                        LEDbrightnessSpecial + 45, -50, 50 );

            color = scaleBrightness( color, scaleVoltage );

            // Serial.println(scaledBrightness);
            //  Serial.println((color, map(brightness, LEDbrightnessSpecial,
            //  LEDbrightnessSpecial+45, -90, 100)));
            netColors[ showADCreadings[ i ] ] = unpackRgb( color );
            adcReadingColors[ i ] = color;

            net[ showADCreadings[ i ] ].color = unpackRgb( color );
            // drawWires(showADCreadings[0]);
            // showLEDsCore2 = 2;
        }
    }
}

/// @brief check if any measurements or gpio outputs are connected
/// @return  0 = gpio output, 1 = gpio input, 2 = adc, -1 if no measurements or
/// outputs are connected
int anythingInteractiveConnected( int net ) {

    if ( anyAdcConnected( net ) != -1 ) {
        // Serial.print("adc connected");
        return 2;
    }
    if ( anyGpioOutputConnected( net ) != -1 ) {
        // Serial.print("gpio output connected");
        return 0;
    }
    if ( anyGpioInputConnected( net ) != -1 ) {
        // Serial.print("gpio input connected");
        return 1;
    }
    // Serial.print("no interactive connected");
    return -1;
}

/// @brief check if any gpio outputs are connected
/// @return  gpio number if a gpio output is connected, -1 if no outputs are
/// connected (remember user facing gpio numbers are 1-8, this will return 0-10)
int anyGpioOutputConnected( int net ) {
    if ( net == -1 ) {
        for ( int i = 0; i < 10; i++ ) {
            if ( gpioNet[ i ] > 0 && gpioNet[ i ] <= numberOfNets ) {
                if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
                    // Only treat as GPIO output if pin function is SIO
                    if ( gpio_function_map[ i ] == GPIO_FUNC_SIO ) {
                        return i;
                    }
                }
            }
        }
    } else {
        for ( int i = 0; i < 10; i++ ) {
            if ( gpioNet[ i ] == net ) {
                if ( jumperlessConfig.gpio.direction[ i ] == 0 ) {
                    // Only treat as GPIO output if pin function is SIO
                    if ( gpio_function_map[ i ] == GPIO_FUNC_SIO ) {
                        return i;
                    }
                }
            }
        }
    }
    return -1;
}




/// @brief check if any gpio inputs are connected
/// @return  gpio number if a gpio input is connected, -1 if no inputs are
/// connected (remember user facing gpio numbers are 1-8, this will return 0-10)
int anyGpioInputConnected( int net ) {
    if ( net == -1 ) {
        for ( int i = 0; i < 10; i++ ) {
            if ( gpioNet[ i ] > 0 && gpioNet[ i ] <= numberOfNets ) {
                if ( jumperlessConfig.gpio.direction[ i ] == 1 ) {
                    if ( gpio_function_map[ i ] == GPIO_FUNC_SIO ) {
                        return i;
                    }
                }
            }
        }
        return -1;
    } else {
        for ( int i = 0; i < 10; i++ ) {
            if ( gpioNet[ i ] == net ) {
                if ( jumperlessConfig.gpio.direction[ i ] == 1 ) {
                    if ( gpio_function_map[ i ] == GPIO_FUNC_SIO ) {
                        return i;
                    }
                }
            }
        }
    }
    return -1;
}

/// @brief check if any measurements are connected
/// @return -1 if no measurements are connected, return adc number if a
/// measurement is connected
int anyAdcConnected( int net ) {
    if ( net == -1 ) {
        for ( int i = 0; i < 8; i++ ) {
            // Serial.print("showADCreadings[i]: ");
            // Serial.println(showADCreadings[i]);

            if ( showADCreadings[ i ] > 0 && showADCreadings[ i ] <= numberOfNets &&
                 i != 7 ) {
                return i;
            }
        }
    } else {
        for ( int i = 0; i < 8; i++ ) {
            if ( showADCreadings[ i ] == net ) {
                return i;
            }
        }
    }
    return -1;
}

uint32_t measurementToColor( float measurement, float min, float max ) {
    uint32_t color = 0;
    hsvColor hsv;
    int minInt = -80;
    int maxInt = 80;
    int measurementInt = measurement * 10;

    if ( measurement < jumperlessConfig.dacs.limit_min ) {
        measurement = jumperlessConfig.dacs.limit_min;
    } else if ( measurement > jumperlessConfig.dacs.limit_max ) {
        measurement = jumperlessConfig.dacs.limit_max;
    }

    int shift = 228;
    hsv.h = map( measurementInt, minInt, maxInt, 210, 10 );
    hsv.h += shift;
    hsv.s = 255;

    if ( measurement < -0.7 ) {
        hsv.h += 10;

        if ( measurement < -5.4 ) {
            hsv.s -= abs( measurement + 5.4 ) * 48;
            if ( hsv.s < 0 ) {
                hsv.s = 0;
            }
            // Serial.println(measurement + 5.4);
            // Serial.println(hsv.s);
        } else {
            hsv.s = 230;
        }
    } else if ( measurement > 0.5 ) {
        hsv.h += 239;

        if ( measurement > 5.4 ) {
            hsv.s -= ( measurement - 5.4 ) * 48;

            if ( hsv.s < 0 ) {
                hsv.s = 0;
            }
        }
    } else {
        // hsv.h += 254 - (measurement - 0.0) * 64;
    }
    // hsv.s = 255;

    hsv.h = hsv.h % 256;
    hsv.v = jumperlessConfig.display.special_net_brightness;
    // Serial.println(hsv.h);
    //  int measurementInt = measurement * 10;
    //  int distance = abs(0 - measurementInt);
    //  hsv.v = map(distance, min*11, max*11, 0, 100);
    rgbColor rgb = HsvToRgb( hsv );
    color = packRgb( rgb );

    return color;
}

void showMeasurements( int samples, int printOrBB, int oneShot ) {
    unsigned long startMillis = millis( );
    int printInterval = 150;
    static unsigned long lastPrintTime = 0;
    // while (Serial.available() == 0 && Serial1.available() == 0 &&
    //        checkProbeButton() == 0)

    //   {

    if ( millis( ) - lastPrintTime < printInterval ) {
        return;
    }
    Serial.print( "\r                                                             "
                  "         \r" );
    lastPrintTime = millis( );
    // Serial.flush();
    int adc0ReadingUnscaled;
    float adc0Reading;

    int adc1ReadingUnscaled;
    float adc1Reading;

    int adc2ReadingUnscaled;
    float adc2Reading;

    int adc3ReadingUnscaled;
    float adc3Reading;

    int adc4ReadingUnscaled;
    float adc4Reading;

    int adc7ReadingUnscaled;
    float adc7Reading;

    int bs = 0;

    if ( showADCreadings[ 0 ] != 0 ) {
        //       if (readFloatingOrState(ADC0_PIN, -1) == floating)//this doesn't
        //       work because it's buffered
        // {

        //  bs += Serial.print("Floating  ");
        // }
        // adc0ReadingUnscaled = readAdc(0, samples);

        // adc0Reading = (adc0ReadingUnscaled) * (railSpread / 4095);
        // adc0Reading -= railSpread / 2; // offset
        adc0Reading = readAdcVoltage( 0, samples );
        bs += Serial.print( "ADC 0: " );
        bs += Serial.print( adc0Reading );
        bs += Serial.print( "V\t" );
        // int mappedAdc0Reading = map(adc0ReadingUnscaled, 0, 4095, -40, 40);
        // int hueShift = 0;

        // if (mappedAdc0Reading < 0) {
        //   hueShift = map(mappedAdc0Reading, -40, 0, 0, 200);
        //   mappedAdc0Reading = abs(mappedAdc0Reading);
        // }
    }
    if ( showADCreadings[ 1 ] != 0 ) {
        //       if (readFloatingOrState(ADC0_PIN, -1) == floating)//this doesn't
        //       work because it's buffered
        // {

        //  bs += Serial.print("Floating  ");
        // }
        // adc1ReadingUnscaled = readAdc(1, samples);

        // adc1Reading = (adc1ReadingUnscaled) * (railSpread / 4095);
        // adc1Reading -= railSpread / 2; // offset

        adc1Reading = readAdcVoltage( 1, samples );
        bs += Serial.print( "ADC 1: " );
        bs += Serial.print( adc1Reading );
        bs += Serial.print( "V\t" );
        // int mappedAdc1Reading = map(adc1ReadingUnscaled, 0, 4095, -40, 40);
        // int hueShift = 0;

        // if (mappedAdc1Reading < 0) {
        //   hueShift = map(mappedAdc1Reading, -40, 0, 0, 200);
        //   mappedAdc1Reading = abs(mappedAdc1Reading);
        // }
    }

    if ( showADCreadings[ 2 ] != 0 ) {

        adc2ReadingUnscaled = readAdc( 2, samples );
        adc2Reading = ( adc2ReadingUnscaled ) * ( railSpread / 4095 );
        adc2Reading -= railSpread / 2; // offset
        bs += Serial.print( "ADC 2: " );
        bs += Serial.print( adc2Reading );
        bs += Serial.print( "V\t" );
        int mappedAdc2Reading = map( adc2ReadingUnscaled, 0, 4095, -40, 40 );
        int hueShift = 0;

        if ( mappedAdc2Reading < 0 ) {
            hueShift = map( mappedAdc2Reading, -40, 0, 0, 200 );
            mappedAdc2Reading = abs( mappedAdc2Reading );
        }
    }

    if ( showADCreadings[ 3 ] != 0 ) {

        adc3ReadingUnscaled = readAdc( 3, samples );
        adc3Reading = ( adc3ReadingUnscaled ) * ( railSpread / 4095 );
        adc3Reading -= railSpread / 2; // offset
        bs += Serial.print( "ADC 3: " );
        bs += Serial.print( adc3Reading );
        bs += Serial.print( "V\t" );
        int mappedAdc3Reading = map( adc3ReadingUnscaled, 0, 4095, -40, 40 );
        int hueShift = 0;

        if ( mappedAdc3Reading < 0 ) {
            hueShift = map( mappedAdc3Reading, -40, 0, 0, 200 );
            mappedAdc3Reading = abs( mappedAdc3Reading );
        }
    }

    if ( showADCreadings[ 7 ] != 0 ) {

        adc7ReadingUnscaled = readAdc( 7, samples );
        adc7Reading = ( adc7ReadingUnscaled ) * ( railSpread / 4095 );
        adc7Reading -= railSpread / 2; // offset
        bs += Serial.print( "ADC 7: " );
        bs += Serial.print( adc7Reading );
        bs += Serial.print( "V\t" );
        int mappedAdc7Reading = map( adc7ReadingUnscaled, 0, 4095, -40, 40 );
        int hueShift = 0;

        if ( mappedAdc7Reading < 0 ) {
            hueShift = map( mappedAdc7Reading, -40, 0, 0, 200 );
            mappedAdc7Reading = abs( mappedAdc7Reading );
        }
    }

    if ( showADCreadings[ 4 ] != 0 ) {

        adc4ReadingUnscaled = readAdc( 4, samples );
        adc4Reading = ( adc4ReadingUnscaled ) * ( 5.0 / 4095 );
        // adc1Reading -= 0.1; // offset
        bs += Serial.print( "ADC 4: " );
        bs += Serial.print( adc4Reading );
        bs += Serial.print( "V\t" );
    }

    // if (showINA0[0] == 1 || showINA0[1] == 1 || showINA0[2] == 1) {
    //   bs += Serial.print("   INA219: ");
    // }

    if ( showINA0[ 0 ] == 1 ) {
        bs += Serial.print( "INA 0: " );
        bs += Serial.print( INA0.getCurrent_mA( ) );
        bs += Serial.print( "mA\t" );
        // bs += Serial.print("\tINA 1: ");
        // bs += Serial.print(INA1.getCurrent_mA());
        // bs += Serial.print("mA\t");
    }

    if ( showINA0[ 1 ] == 1 ) {
        bs += Serial.print( " V: " );
        bs += Serial.print( INA0.getBusVoltage( ) );
        bs += Serial.print( "V\t" );
    }
    if ( showINA0[ 2 ] == 1 ) {
        bs += Serial.print( "P: " );
        bs += Serial.print( INA0.getPower_mW( ) );
        bs += Serial.print( "mW\t" );
    }
    // Serial.print(digitalRead(buttonPin));
    bs += Serial.print( "      \r" );
    // rotaryEncoderStuff();
    // if (encoderButtonState != IDLE) {
    //   // showReadings = 0;
    //   return;
    //   }
    // while (millis() - startMillis < printInterval &&
    //        (Serial.available() == 0 && checkProbeButton() == 0)) {

    //   showLEDmeasurements();
    //   delayMicroseconds(5000);
    //   }
    startMillis = millis( );
    Serial.flush( );
}

void printPIOStateMachines( ) {
    Serial.println( "=== PIO STATE MACHINE STATUS ===" );

    // Check all PIO instances (pio0, pio1, pio2)
    PIO pio_instances[] = { pio0, pio1, pio2 };
    const char* pio_names[] = { "PIO0", "PIO1", "PIO2" };

    for ( int pio_idx = 0; pio_idx < 3; pio_idx++ ) {
        PIO current_pio = pio_instances[ pio_idx ];
        Serial.printf( "%s:\n\r", pio_names[ pio_idx ] );

        // Check all 4 state machines per PIO
        for ( int sm = 0; sm < 4; sm++ ) {
            bool is_claimed = pio_sm_is_claimed( current_pio, sm );

            if ( is_claimed ) {
                Serial.printf( "  SM%d: CLAIMED\n\r", sm );
            } else {
                Serial.printf( "  SM%d: FREE\n\r", sm );
            }
        }
    }
}

float readAdcVoltage( int channel, int samples ) {

    if ( channel < 0 || channel > 7 ) {
        return 0;
    }
    int adcReadingUnscaled = readAdc( channel, samples );

    float adcReading = ( adcReadingUnscaled ) * ( adcSpread[ channel ] / 4095 );
    if ( channel != 4 && channel != 5 ) {
        adcReading -= adcZero[ channel ]; // offset - use calibrated zero value
    }

    return adcReading;
}

int readAdc( int channel, int samples ) {
    unsigned long adcReadingAverage = 0;
    // if (channel == 0) { // I have no fucking idea why this works //future me:
    // the op amps were untamed

    //   pinMode(ADC1_PIN, OUTPUT);
    //   digitalWrite(ADC1_PIN, LOW);
    // }

    if ( channel > 8 ) {
        return 0;
    }
    unsigned long timeoutTimer = micros( );

    int actualSamples = 0;
    adc_select_input( channel );
    for ( int i = 0; i < samples; i++ ) {
        if ( micros( ) - timeoutTimer > 5000 ) {
            break;
        }

        adcReadingAverage += adc_read( );
        // adcReadingAverage += analogRead(ADC0_PIN + channel); //(int)adc_read();
        actualSamples++;
        delayMicroseconds( 6 );
    }

    int adcReading =
        ( actualSamples > 0 ) ? ( adcReadingAverage / actualSamples ) : 0;

    // float adc3Voltage = (adc3Reading - 2528) / 220.0; // painstakingly measured

    // if (channel == 0) {
    //   pinMode(ADC1_PIN, INPUT);
    // }
    // Serial.println(adcReading);
    // Serial.print("adcReading:               ");
    // Serial.println(adcReading, DEC);
    // Serial.flush();
    // Serial.print("adcReading & 0xFFF0 >> 4: ");
    // Serial.println((adcReading & 0xFFF0) >> 4, DEC);
    // Serial.flush();
    return adcReading;
}

// Slow PWM Functions (for frequencies below 10Hz)
// Hardware timer callback for slow PWM
bool __not_in_flash_func( slowPWMTimerCallback )( repeating_timer_t* rt ) {
    // Get the GPIO index from the user data
    int gpio_index = (int)(uintptr_t)rt->user_data;

    // Increment counter
    gpioSlowPWMCounter[ gpio_index ]++;

    // Check if we've reached the period
    if ( gpioSlowPWMCounter[ gpio_index ] >= gpioSlowPWMPeriod[ gpio_index ] ) {
        gpioSlowPWMCounter[ gpio_index ] = 0; // Reset counter
    }

    // Set pin state based on duty cycle
    int physical_pin = gpioDef[ gpio_index ][ 0 ];
    if ( gpioSlowPWMCounter[ gpio_index ] < gpioSlowPWMDutyTicks[ gpio_index ] ) {
        gpio_put( physical_pin, 1 ); // HIGH
    } else {
        gpio_put( physical_pin, 0 ); // LOW
    }

    return true; // Continue timer
}

// Setup slow PWM using hardware timer
int setupSlowPWM( int gpio_pin, float frequency, float duty_cycle ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate frequency (0.001Hz to 10Hz for slow PWM)
    if ( frequency < 0.001 || frequency > 10.0 ) {
        return -2; // Invalid frequency
    }

    // Validate duty cycle (0.0 to 1.0)
    if ( duty_cycle < 0.0 || duty_cycle > 1.0 ) {
        return -3; // Invalid duty cycle
    }

    int gpio_index = gpio_pin - 1;                 // Convert to 0-based index
    int physical_pin = gpioDef[ gpio_index ][ 0 ]; // Get physical pin number

    // Stop any existing slow PWM on this pin
    if ( gpioSlowPWMEnabled[ gpio_index ] ) {
        cancel_repeating_timer( &gpioSlowPWMTimers[ gpio_index ] );
        gpioSlowPWMEnabled[ gpio_index ] = false;
    }

    // Configure pin as output
    gpio_set_function( physical_pin, GPIO_FUNC_SIO );
    gpio_set_dir( physical_pin, true );
    gpio_put( physical_pin, 0 ); // Start low

    // Calculate timer parameters
    // Use 1ms timer resolution for good precision
    uint32_t timer_interval_us = 1000; // 1ms intervals
    uint32_t period_ms = (uint32_t)( 1000.0f / frequency );
    uint32_t duty_ms = (uint32_t)( period_ms * duty_cycle );

    // Convert to timer ticks
    gpioSlowPWMPeriod[ gpio_index ] = period_ms;  // Period in ms
    gpioSlowPWMDutyTicks[ gpio_index ] = duty_ms; // Duty cycle in ms
    gpioSlowPWMCounter[ gpio_index ] = 0;

    // Start the repeating timer
    bool success = add_repeating_timer_ms( -1, slowPWMTimerCallback,
                                           (void*)(uintptr_t)gpio_index,
                                           &gpioSlowPWMTimers[ gpio_index ] );
    if ( !success ) {
        return -4; // Timer setup failed
    }

    // Update state tracking
    gpioPWMFrequency[ gpio_index ] = frequency;
    gpioPWMDutyCycle[ gpio_index ] = duty_cycle;
    gpioSlowPWMEnabled[ gpio_index ] = true;
    gpioPWMEnabled[ gpio_index ] = false; // Not using hardware PWM

    // Update config
    jumperlessConfig.gpio.pwm_frequency[ gpio_index ] = frequency;
    jumperlessConfig.gpio.pwm_duty_cycle[ gpio_index ] = duty_cycle;
    jumperlessConfig.gpio.pwm_enabled[ gpio_index ] = true;

    return 0; // Success
}

// Set slow PWM duty cycle
int setSlowPWMDutyCycle( int gpio_pin, float duty_cycle ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate duty cycle (0.0 to 1.0)
    if ( duty_cycle < 0.0 || duty_cycle > 1.0 ) {
        return -2; // Invalid duty cycle
    }

    int gpio_index = gpio_pin - 1; // Convert to 0-based index

    // Check if slow PWM is enabled
    if ( !gpioSlowPWMEnabled[ gpio_index ] ) {
        // Set up slow PWM with default frequency if not already enabled
        float default_freq = ( gpioPWMFrequency[ gpio_index ] < 0.01 )
                                 ? 1.0
                                 : gpioPWMFrequency[ gpio_index ];
        return setupSlowPWM( gpio_pin, default_freq, duty_cycle );
    }

    // Update duty cycle
    uint32_t period_ms = gpioSlowPWMPeriod[ gpio_index ];
    uint32_t duty_ms = (uint32_t)( period_ms * duty_cycle );
    gpioSlowPWMDutyTicks[ gpio_index ] = duty_ms;
    gpioPWMDutyCycle[ gpio_index ] = duty_cycle;
    jumperlessConfig.gpio.pwm_duty_cycle[ gpio_index ] = duty_cycle;

    return 0; // Success
}

// Set slow PWM frequency
int setSlowPWMFrequency( int gpio_pin, float frequency ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate frequency (0.001Hz to 10Hz for slow PWM)
    if ( frequency < 0.001 || frequency > 10.0 ) {
        return -2; // Invalid frequency
    }

    int gpio_index = gpio_pin - 1; // Convert to 0-based index

    // Check if slow PWM is enabled
    if ( !gpioSlowPWMEnabled[ gpio_index ] ) {
        // Set up slow PWM with default duty cycle if not already enabled
        float default_duty = ( gpioPWMDutyCycle[ gpio_index ] < 0.0 ||
                               gpioPWMDutyCycle[ gpio_index ] > 1.0 )
                                 ? 0.5
                                 : gpioPWMDutyCycle[ gpio_index ];
        return setupSlowPWM( gpio_pin, frequency, default_duty );
    }

    // Re-setup slow PWM with new frequency
    return setupSlowPWM( gpio_pin, frequency, gpioPWMDutyCycle[ gpio_index ] );
}

// Stop slow PWM
int stopSlowPWM( int gpio_pin ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    int gpio_index = gpio_pin - 1;                 // Convert to 0-based index
    int physical_pin = gpioDef[ gpio_index ][ 0 ]; // Get physical pin number

    // Stop the timer
    if ( gpioSlowPWMEnabled[ gpio_index ] ) {
        cancel_repeating_timer( &gpioSlowPWMTimers[ gpio_index ] );
        gpioSlowPWMEnabled[ gpio_index ] = false;
    }

    // Set pin low and back to input
    gpio_put( physical_pin, 0 );
    gpio_set_function( physical_pin, GPIO_FUNC_SIO );
    gpio_set_dir( physical_pin, false );

    // Update state tracking
    gpioPWMEnabled[ gpio_index ] = false;
    jumperlessConfig.gpio.pwm_enabled[ gpio_index ] = false;

    return 0; // Success
}

// PWM Functions
int setupPWM( int gpio_pin, float frequency, float duty_cycle ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate frequency (0.01Hz to 62.5MHz)
    if ( frequency < 0.01 || frequency > 62500000.0 ) {
        return -2; // Invalid frequency
    }

    // For frequencies below 10Hz, use slow PWM with hardware timer
    if ( frequency < 10.0 ) {
        return setupSlowPWM( gpio_pin, frequency, duty_cycle );
    }

    int gpio_index = gpio_pin - 1;                 // Convert to 0-based index
    int physical_pin = gpioDef[ gpio_index ][ 0 ]; // Get physical pin number

    // Set up PWM
    gpio_set_function( physical_pin, GPIO_FUNC_PWM );
    gpio_function_map[ gpio_index ] = GPIO_FUNC_PWM;

    // Find out which PWM slice is connected to this GPIO
    uint slice_num = pwm_gpio_to_slice_num( physical_pin );

    // Calculate PWM parameters
    // System clock is 150MHz by default
    float clock_freq = 150000000.0f;
    uint32_t divider = (uint32_t)( clock_freq / ( frequency * 65536 ) ) + 1;
    if ( divider > 255 )
        divider = 255;

    uint32_t wrap = (uint32_t)( clock_freq / ( frequency * divider ) ) - 1;
    if ( wrap > 65535 )
        wrap = 65535;

    // Set the PWM parameters
    pwm_set_clkdiv( slice_num, divider );
    pwm_set_wrap( slice_num, wrap );

    // Set duty cycle
    uint32_t level = (uint32_t)( duty_cycle * ( wrap + 1 ) );
    pwm_set_gpio_level( physical_pin, level );

    // Enable PWM
    pwm_set_enabled( slice_num, true );

    // Update state tracking
    gpioPWMFrequency[ gpio_index ] = frequency;
    gpioPWMDutyCycle[ gpio_index ] = duty_cycle;
    gpioPWMEnabled[ gpio_index ] = true;

    // Update config
    jumperlessConfig.gpio.pwm_frequency[ gpio_index ] = frequency;
    jumperlessConfig.gpio.pwm_duty_cycle[ gpio_index ] = duty_cycle;
    jumperlessConfig.gpio.pwm_enabled[ gpio_index ] = true;

    return 0; // Success
}

int setPWMDutyCycle( int gpio_pin, float duty_cycle ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate duty cycle (0.0 to 1.0)
    if ( duty_cycle < 0.0 || duty_cycle > 1.0 ) {
        return -2; // Invalid duty cycle
    }

    int gpio_index = gpio_pin - 1;                 // Convert to 0-based index
    int physical_pin = gpioDef[ gpio_index ][ 0 ]; // Get physical pin number

    // Check if slow PWM is enabled
    if ( gpioSlowPWMEnabled[ gpio_index ] ) {
        return setSlowPWMDutyCycle( gpio_pin, duty_cycle );
    }

    // Check if regular PWM is enabled
    if ( !gpioPWMEnabled[ gpio_index ] ) {
        // Set up PWM with default frequency if not already enabled
        float default_freq = ( gpioPWMFrequency[ gpio_index ] < 0.01 )
                                 ? 1000.0
                                 : gpioPWMFrequency[ gpio_index ];
        return setupPWM( gpio_pin, default_freq, duty_cycle );
    }

    // Re-setup PWM with the new duty cycle (simpler approach)
    return setupPWM( gpio_pin, gpioPWMFrequency[ gpio_index ], duty_cycle );
}

int setPWMFrequency( int gpio_pin, float frequency ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    // Validate frequency (0.01Hz to 62.5MHz)
    if ( frequency < 0.01 || frequency > 62500000.0 ) {
        return -2; // Invalid frequency
    }

    int gpio_index = gpio_pin - 1; // Convert to 0-based index

    // Check if slow PWM is enabled
    if ( gpioSlowPWMEnabled[ gpio_index ] ) {
        return setSlowPWMFrequency( gpio_pin, frequency );
    }

    // Check if regular PWM is enabled
    if ( !gpioPWMEnabled[ gpio_index ] ) {
        // Set up PWM with default duty cycle if not already enabled
        float default_duty = ( gpioPWMDutyCycle[ gpio_index ] < 0.0 ||
                               gpioPWMDutyCycle[ gpio_index ] > 1.0 )
                                 ? 0.5
                                 : gpioPWMDutyCycle[ gpio_index ];
        return setupPWM( gpio_pin, frequency, default_duty );
    }

    // Re-setup PWM with new frequency
    return setupPWM( gpio_pin, frequency, gpioPWMDutyCycle[ gpio_index ] );
}

int stopPWM( int gpio_pin ) {
    // Validate GPIO pin number (1-8 for regular GPIO pins)
    if ( gpio_pin < 1 || gpio_pin > 8 ) {
        return -1; // Invalid pin
    }

    int gpio_index = gpio_pin - 1;                 // Convert to 0-based index
    int physical_pin = gpioDef[ gpio_index ][ 0 ]; // Get physical pin number

    // Check if slow PWM is enabled
    if ( gpioSlowPWMEnabled[ gpio_index ] ) {
        return stopSlowPWM( gpio_pin );
    }

    // Regular hardware PWM
    // Find out which PWM slice is connected to this GPIO
    uint slice_num = pwm_gpio_to_slice_num( physical_pin );

    // Disable PWM
    pwm_set_enabled( slice_num, false );

    // Set pin back to SIO function
    gpio_set_function( physical_pin, GPIO_FUNC_SIO );

    // Update state tracking
    gpioPWMEnabled[ gpio_index ] = false;
    jumperlessConfig.gpio.pwm_enabled[ gpio_index ] = false;

    return 0; // Success
}

void printPWMState( void ) {
    Serial.println( "\n   PWM State:" );
    Serial.println( "   number:\t\b1\t\b2\t\b3\t\b4\t\b5\t\b6\t\b7\t\b8" );

    Serial.print( "  enabled:\t" );
    for ( int i = 0; i < 8; i++ ) {
        bool is_enabled = gpioPWMEnabled[ i ] || gpioSlowPWMEnabled[ i ];
        Serial.print( is_enabled ? "yes" : "no" );
        Serial.print( "\t" );
    }
    Serial.println( );

    Serial.print( "frequency:\t" );
    for ( int i = 0; i < 8; i++ ) {
        if ( gpioPWMEnabled[ i ] || gpioSlowPWMEnabled[ i ] ) {
            Serial.print( gpioPWMFrequency[ i ], 1 );
            if ( gpioSlowPWMEnabled[ i ] ) {
                Serial.print( "(S)" ); // Mark slow PWM
            }
        } else {
            Serial.print( "-" );
        }
        Serial.print( "\t" );
    }
    Serial.println( );

    Serial.print( "duty_cycle:\t" );
    for ( int i = 0; i < 8; i++ ) {
        if ( gpioPWMEnabled[ i ] || gpioSlowPWMEnabled[ i ] ) {
            Serial.print( gpioPWMDutyCycle[ i ], 2 );
        } else {
            Serial.print( "-" );
        }
        Serial.print( "\t" );
    }
    Serial.println( );

    Serial.print( "    type:\t" );
    for ( int i = 0; i < 8; i++ ) {
        if ( gpioSlowPWMEnabled[ i ] ) {
            Serial.print( "slow" );
        } else if ( gpioPWMEnabled[ i ] ) {
            Serial.print( "hw" );
        } else {
            Serial.print( "-" );
        }
        Serial.print( "\t" );
    }
    Serial.println( );
}
