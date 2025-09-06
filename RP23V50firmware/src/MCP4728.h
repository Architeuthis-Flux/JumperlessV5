/*!
 *  @file MCP4728.h
 *
 *  Async I2C Driver for the MCP4728 4-Channel 12-Bit I2C DAC
 *  Based on Adafruit_MCP4728 but using Earle's async Wire API
 *
 *  This library uses the Arduino-Pico async Wire API for non-blocking
 *  high-speed I2C communication, perfect for continuous waveform generation.
 */

#ifndef _MCP4728_H
#define _MCP4728_H

#include "Arduino.h"
#include <Wire.h>

#define MCP4728_I2CADDR_DEFAULT 0x60 ///< MCP4728 default i2c address

/**
 * @brief Power status values
 */
typedef enum pd_mode {
  MCP4728_PD_MODE_NORMAL,    ///< Normal operation
  MCP4728_PD_MODE_GND_1K,    ///< VOUT loaded with 1kΩ to ground
  MCP4728_PD_MODE_GND_100K,  ///< VOUT loaded with 100kΩ to ground
  MCP4728_PD_MODE_GND_500K,  ///< VOUT loaded with 500kΩ to ground
} MCP4728_pd_mode_t;

/**
 * @brief Gain values
 */
typedef enum gain {
  MCP4728_GAIN_1X,
  MCP4728_GAIN_2X,
} MCP4728_gain_t;

/**
 * @brief Reference voltage values
 */
typedef enum vref {
  MCP4728_VREF_VDD,      ///< Use VDD as reference
  MCP4728_VREF_INTERNAL, ///< Use internal 2.048V reference
} MCP4728_vref_t;

/**
 * @brief Channel identifiers
 */
typedef enum channel {
  MCP4728_CHANNEL_A,
  MCP4728_CHANNEL_B,
  MCP4728_CHANNEL_C,
  MCP4728_CHANNEL_D,
} MCP4728_channel_t;

struct last_settings {
  MCP4728_channel_t channel;
  uint16_t value;
  MCP4728_vref_t vref;
  MCP4728_gain_t gain;
  MCP4728_pd_mode_t pd_mode;
};
extern struct last_settings last_settings;

/*!
 *    @brief  Class for async MCP4728 I2C Digital-to-Analog Converter
 */
class MCP4728 {
public:
  MCP4728();
  
  // Initialization
  bool begin(uint8_t i2c_address = MCP4728_I2CADDR_DEFAULT, TwoWire *wire = &Wire);
  void end();
  
  // Async fast write - the main function for waveform generation
  bool fastWriteAsync(uint16_t channel_a_value, uint16_t channel_b_value,
                      uint16_t channel_c_value, uint16_t channel_d_value);
  
  // Check if async operation is complete
  bool finishedAsync();
  
  // Set callback for when async operation completes
  void onFinishedAsync(void(*function)(void));
  
  // Abort any pending async operation
  void abortAsync();
  
  // Synchronous operations (for setup/configuration)
  bool setChannelValue(MCP4728_channel_t channel, uint16_t new_value,
                       MCP4728_vref_t new_vref = MCP4728_VREF_VDD,
                       MCP4728_gain_t new_gain = MCP4728_GAIN_1X,
                       MCP4728_pd_mode_t new_pd_mode = MCP4728_PD_MODE_NORMAL,
                       bool udac = false);


  bool sendI2Cstart();
  bool sendI2Cend(bool sendStop = true);
  bool sendI2Cdata(uint8_t *data, size_t length);
  bool quickSetChannelValue(MCP4728_channel_t channel, uint16_t new_value, bool sendStart, bool sendEnd);
  // Optimized single-sample write: START + 3 bytes + repeated START
  bool writeSampleRepeatedStart(MCP4728_channel_t channel, uint16_t value);
  
  // Get status
  bool isInitialized() const { return _initialized; }
  uint8_t getAddress() const { return _i2c_address; }
  TwoWire* getWire() const { return _wire; }
  uint32_t getClockHz() const { return _clock_hz; }
volatile bool started;
volatile bool ended;

private:
  bool _initialized;
  uint8_t _i2c_address;
  TwoWire *_wire;
  uint32_t _clock_hz = 1000000; // track configured I2C clock
  
  // Buffer for async operations (4-byte aligned for DMA)
  alignas(4) uint8_t _async_buffer[8];
};

#endif
