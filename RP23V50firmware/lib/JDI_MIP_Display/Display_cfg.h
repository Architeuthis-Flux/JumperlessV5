// MIT License
//
// Copyright(c) 2021 Giovanni Bertazzoni <nottheworstdev@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#define DISPLAY_WIDTH   72         // Display width in pixel
#define DISPLAY_HEIGHT  144         // Display height in pixel

// #define USE_ESP32_DMA  // 使用ESP32 DMA
// #define DIFF_LINE_UPDATE   // 差异行更新

//=================================================================
// Wiring details: https://github.com/Gbertaz/JDI_MIP_Display#wiring
//=================================================================

#define SPI_FREQUENCY   3000000     // SPI frequency in Hz
#define SPI_CHANNEL     spi0        // SPI channel number
#define PIN_MISO        24          // SPI Data Signal pin
#define PIN_MOSI        23         // SPI Data Signal pin
#define PIN_SCK         22          // SPI Clock Signal pin
#define PIN_SCS         21          // SPI Chip Select Signal pin
#define PIN_DISP        25          // Display ON/OFF Switching Signal pin
#define PIN_FRONTLIGHT  -1          // Frontlight pin. Optional depending on the display model