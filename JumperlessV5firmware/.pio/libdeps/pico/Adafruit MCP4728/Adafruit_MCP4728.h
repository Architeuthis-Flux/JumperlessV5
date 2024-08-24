/*!
 *  @file Adafruit_MCP4728.h
 *
 * 	I2C Driver for the Adafruit MCP4728 4-Channel 12-Bit I2C DAC library
 *
 * 	This is a library for the Adafruit MCP4728 breakout:
 * 	https://www.adafruit.com/products/4470
 *
 * 	Adafruit invests time and resources providing this open source code,
 *  please support Adafruit and open-source hardware by purchasing products from
 * 	Adafruit!
 *
 *
 *	BSD license (see license.txt)
 */

#ifndef _ADAFRUIT_MCP4728_H
#define _ADAFRUIT_MCP4728_H

#include "Arduino.h"
#include <Adafruit_BusIO_Register.h>
#include <Adafruit_I2CDevice.h>
#include <Wire.h>

#define MCP4728_I2CADDR_DEFAULT 0x60 ///< MCP4728 default i2c address

#define MCP4728_MULTI_IR_CMD                                                   \
  0x40 ///< Command to write to the input register only
#define MCP4728_MULTI_EEPROM_CMD                                               \
  0x50 ///< Command to write to the input register and EEPROM
#define MCP4728_FAST_WRITE_CMD                                                 \
  0xC0 ///< Command to write all channels at once with

/**
 * @brief Power status values
 *
 * Allowed values for `setPowerMode`.
 */

typedef enum pd_mode {
  MCP4728_PD_MODE_NORMAL, ///< Normal; the channel outputs the given value as
                          ///< normal.
  MCP4728_PD_MODE_GND_1K, ///< VOUT is loaded with 1 kΩ resistor to ground. Most
                          ///< of the channel circuits are powered off.
  MCP4728_PD_MODE_GND_100K, ///< VOUT is loaded with 100 kΩ resistor to ground.
                            ///< Most of the channel circuits are powered off.
  MCP4728_PD_MODE_GND_500K, ///< VOUT is loaded with 500 kΩ resistor to ground.
                            ///< Most of the channel circuits are powered off.
} MCP4728_pd_mode_t;

/**
 * @brief Example enum values
 *
 * Allowed values for `setGain`.
 */
typedef enum gain {
  MCP4728_GAIN_1X,
  MCP4728_GAIN_2X,
} MCP4728_gain_t;

/**
 * @brief Ex
 *
 * Allowed values for `setVref`.
 */
typedef enum vref {
  MCP4728_VREF_VDD,
  MCP4728_VREF_INTERNAL,
} MCP4728_vref_t;

/**
 * @brief Example enum values
 *
 * Allowed values for `setChannelGain`.
 */
typedef enum channel {
  MCP4728_CHANNEL_A,
  MCP4728_CHANNEL_B,
  MCP4728_CHANNEL_C,
  MCP4728_CHANNEL_D,
} MCP4728_channel_t;

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the MCP4728 I2C Digital Potentiometer
 */
class Adafruit_MCP4728 {
public:
  Adafruit_MCP4728();
  bool begin(uint8_t i2c_address = MCP4728_I2CADDR_DEFAULT,
             TwoWire *wire = &Wire);

  bool setChannelValue(MCP4728_channel_t channel, uint16_t new_value,
                       MCP4728_vref_t new_vref = MCP4728_VREF_VDD,
                       MCP4728_gain_t new_gain = MCP4728_GAIN_1X,
                       MCP4728_pd_mode_t new_pd_mode = MCP4728_PD_MODE_NORMAL,
                       bool udac = false);

  bool fastWrite(uint16_t channel_a_value, uint16_t channel_b_value,
                 uint16_t channel_c_value, uint16_t channel_d_value);
  bool saveToEEPROM(void);

private:
  bool _init(void);

  Adafruit_I2CDevice *i2c_dev;
};

#endif
