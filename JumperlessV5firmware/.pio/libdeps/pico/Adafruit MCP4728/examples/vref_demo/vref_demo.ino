// Demo for configuring the Vref of the MCP4728 4-Channel 12-bit I2C DAC
#include <Adafruit_MCP4728.h>
#include <Wire.h>

Adafruit_MCP4728 mcp;

void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MCP4728 test!");

  // Try to initialize!
  if (!mcp.begin()) {
    Serial.println("Failed to find MCP4728 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MCP4728 Found!");
  /*
   * @param channel The channel to update
   * @param new_value The new value to assign
   * @param new_vref Optional vref setting - Defaults to `MCP4728_VREF_VDD`
   * @param new_gain Optional gain setting - Defaults to `MCP4728_GAIN_1X`
   * @param new_pd_mode Optional power down mode setting - Defaults to
   * `MCP4728_PD_MOOE_NORMAL`
   * @param udac Optional UDAC setting - Defaults to `false`, latching (nearly).
   * Set to `true` to latch when the UDAC pin is pulled low
   *
   */

  // Vref = MCP_VREF_VDD, value = 0, 0V
  mcp.setChannelValue(MCP4728_CHANNEL_A, 0);

  // value is vref/2, with 2.048V internal Vref and 1X gain
  // = 2.048/2 = 1.024V
  mcp.setChannelValue(MCP4728_CHANNEL_B, 2048, MCP4728_VREF_INTERNAL,
                      MCP4728_GAIN_1X);

  // value is vref/2, with 2.048V internal vref and *2X gain*
  // = 4.096/2 = 2.048V
  mcp.setChannelValue(MCP4728_CHANNEL_C, 2048, MCP4728_VREF_INTERNAL,
                      MCP4728_GAIN_2X);

  // value is vref/2, Vref is MCP4728_VREF_VDD(default), the power supply
  // voltage (usually 3.3V or 5V) For Vdd/Vref = 5V, voltage = 2.5V For 3.3V,
  // voltage = 1.65V Values will vary depending on the actual Vref/Vdd
  mcp.setChannelValue(MCP4728_CHANNEL_D, 2048);

  mcp.saveToEEPROM();
}

void loop() { delay(1000); }