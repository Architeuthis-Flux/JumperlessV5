//
//    FILE: MCP4911_test.ino
//  AUTHOR: Rob Tillaart
// PURPOSE: test MCP4921 lib
//    DATE: 2021-05-26
//     URL: https://github.com/RobTillaart/MCP4921


#include "MCP_DAC.h"

// MCP4911 MCP(11, 13);  // SW SPI
MCP4911 MCP;  // HW SPI

volatile int x;
uint32_t start, stop;


void setup()
{
  Serial.begin(115200);
  Serial.println(__FILE__);

  Serial.print("SPI:\t");
  Serial.println(MCP.usesHWSPI());

  MCP.begin(10);

  Serial.print("SPI:\t");
  Serial.println(MCP.usesHWSPI());

  Serial.print("MCP_DAC_LIB_VERSION: ");
  Serial.println(MCP_DAC_LIB_VERSION);
  Serial.println();
  Serial.print("CHANNELS:\t");
  Serial.println(MCP.channels());
  Serial.print("MAXVALUE:\t");
  Serial.println(MCP.maxValue());
  delay(100);

  performance_test();
  analogWrite_test();

  Serial.println("\nDone...");
}


void analogWrite_test()
{
  Serial.println();
  Serial.println(__FUNCTION__);
  for (int channel = 0; channel < MCP.channels(); channel++)
  {
    Serial.println(channel);
    for (uint16_t value = 0; value < MCP.maxValue(); value += 0xFF)
    {
      MCP.analogWrite(value, channel);
      Serial.print(value);
      Serial.print("\t");
      Serial.println(analogRead(A0));
      delay(10);
    }
  }
}


void performance_test()
{
  Serial.println();
  Serial.println(__FUNCTION__);

  start = micros();
  for (uint16_t value = 0; value < MCP.maxValue(); value++)
  {
    x = MCP.analogWrite(value, 0);
  }
  stop = micros();
  Serial.print(MCP.maxValue());
  Serial.print(" x MCP.analogWrite():\t");
  Serial.print(stop - start);
  Serial.print("\t");
  Serial.println((stop - start) / (MCP.maxValue() + 1.0) );
  delay(10);

  start = micros();
  for (uint16_t value = 0; value < MCP.maxValue(); value++)
  {
    MCP.fastWriteA(value);
  }
  stop = micros();
  Serial.print(MCP.maxValue());
  Serial.print(" x MCP.fastWriteA():\t");
  Serial.print(stop - start);
  Serial.print("\t");
  Serial.println((stop - start) / (MCP.maxValue() + 1.0) );
  delay(10); 
}


void loop()
{
}


// -- END OF FILE --

