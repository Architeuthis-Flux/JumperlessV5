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

#include <JDI_MIP_Display.h>

#ifdef USE_ESP32_DMA
    #include "driver/spi_master.h"
    spi_device_handle_t dmaHAL; // DMA SPA句柄
    spi_host_device_t spi_host = (spi_host_device_t)1; // 绘制一次然后冻结
#endif

JDI_MIP_Display::JDI_MIP_Display() : Adafruit_GFX(DISPLAY_WIDTH, DISPLAY_HEIGHT), _pSPIx(&SPI), _spi_settings(SPI_FREQUENCY, MSBFIRST, SPI_MODE0)
{
    _mosi = PIN_MOSI;
    _miso = PIN_MISO;
    _sck = PIN_SCK;
    _scs = PIN_SCS;
    _disp = PIN_DISP;
    _freq = SPI_FREQUENCY;
    _frontlight = PIN_FRONTLIGHT;
}

void JDI_MIP_Display::selectSPI(SPIClass& spi, SPISettings spi_settings)
{
  _pSPIx = &spi;
  _spi_settings = spi_settings;
}

void JDI_MIP_Display::begin()
{
    _background = COLOR_BLACK;
    digitalWrite(_scs, LOW);
    pinMode(_scs, OUTPUT);
    pinMode(_disp, OUTPUT);
    pinMode(_frontlight, OUTPUT);
    memset(&_backBuffer[0], (char)((_background & 0x0F) | ((_background & 0x0F) << 4)), sizeof(_backBuffer));
#ifdef DIFF_LINE_UPDATE
    memset(&_dispBuffer[0], (char)((_background & 0x0F) | ((_background & 0x0F) << 4)), sizeof(_dispBuffer));
#endif

    // 是否开启DMA Is DMA enabled
#ifdef USE_ESP32_DMA
    initDMA(_sck, _miso, _mosi, _scs, _freq); 
#else
    _pSPIx->begin();
#endif
}

void JDI_MIP_Display::refresh()
{
    for (int i = 0; i < HEIGHT; i++)
    {
        int lineIdx = HALF_WIDTH * i;
        char *line_cmd;
#ifdef DIFF_LINE_UPDATE
        if (compareBuffersLine(lineIdx) == true) continue;
        memcpy(&_dispBuffer[lineIdx], &_backBuffer[lineIdx], HALF_WIDTH);
        line_cmd = &_dispBuffer[lineIdx];
#else
        line_cmd = &_backBuffer[lineIdx];
#endif
        sendLineCommand(line_cmd, i);
    }
}


bool JDI_MIP_Display::compareBuffersLine(int lineIndex)
{
#ifdef DIFF_LINE_UPDATE
    for (int i = 0; i < HALF_WIDTH; i++)
    {
        int pixelIdx = lineIndex + i;
        if (_backBuffer[pixelIdx] != _dispBuffer[pixelIdx])
            return false;
    }
#endif
    return true;
}

void JDI_MIP_Display::clearScreen()
{
#ifdef USE_ESP32_DMA
    if (DMA_Enabled)
    {
        uint8_t buf[2];
        buf[0] = CMD_ALL_CLEAR;
        buf[1] = 0x00;
        _pushPixelsDMA(buf, 2);
    }
    else
    {
#endif
        _pSPIx->beginTransaction(_spi_settings);
        digitalWrite(_scs, HIGH);
        _pSPIx->transfer(CMD_ALL_CLEAR);
        _pSPIx->transfer(0x00);
        digitalWrite(_scs, LOW);
        _pSPIx->endTransaction();
#ifdef USE_ESP32_DMA
    }
#endif
}

void JDI_MIP_Display::sendLineCommand(char *line_cmd, int line)
{
    if ((line < 0) || (line >= HEIGHT))
    {
        return;
    }
#ifdef USE_ESP32_DMA
    if (DMA_Enabled)
    {
        uint8_t bufNum = HALF_WIDTH + 4;
        uint8_t buf[bufNum];
        buf[0] = CMD_UPDATE;
        buf[1] = line + 1;
        for (int i = 0; i < HALF_WIDTH; i++)
        {
            buf[i + 2] = line_cmd[i];
        }
        buf[HALF_WIDTH + 2] = 0x00;
        buf[HALF_WIDTH + 3] = 0x00;
        _pushPixelsDMA(buf, bufNum);
    }
    else
    {
#endif
        _pSPIx->beginTransaction(_spi_settings);
        digitalWrite(_scs, HIGH);
        _pSPIx->transfer(CMD_UPDATE);
        _pSPIx->transfer(line + 1);

        for(int i = 0; i < HALF_WIDTH; i++){
            _pSPIx->transfer(line_cmd[i]);
        }

        _pSPIx->transfer(0x00);
        _pSPIx->transfer(0x00);
        digitalWrite(_scs, LOW);
        _pSPIx->endTransaction();
#ifdef USE_ESP32_DMA
    }
#endif
}

void JDI_MIP_Display::drawBufferedPixel(int16_t x, int16_t y, uint16_t color)
{
    drawPixel(x, y, color); // 调用私有的drawPixel函数
}

void JDI_MIP_Display::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    if(x < 0 || x >= width() || y < 0 || y >= height()){
        return;
    }

    int pixelIdx = ((width() / 2) * y) + (x / 2);

    if(x % 2 == 0){
        _backBuffer[pixelIdx] &= 0x0F;
        _backBuffer[pixelIdx] |= (color & 0x0F) << 4;
    }
    else{
        _backBuffer[pixelIdx] &= 0xF0;
        _backBuffer[pixelIdx] |= color & 0x0F;
    }
}

void JDI_MIP_Display::setBackgroundColor(uint16_t color)
{
    _background = color;
}

void JDI_MIP_Display::displayOn()
{
    digitalWrite(_disp, HIGH);
}

void JDI_MIP_Display::displayOff()
{
    digitalWrite(_disp, LOW);
}

void JDI_MIP_Display::frontlightOn()
{
    digitalWrite(_frontlight, HIGH);
}

void JDI_MIP_Display::frontlightOff()
{
    digitalWrite(_frontlight, LOW);
}
#ifdef USE_ESP32_DMA
/***************************************************************************************
** 函数名称：dmaBusy            Function name: dmaBusy
** 描述：检查DMA是否繁忙         Description: Check if DMA is busy
***************************************************************************************/
bool JDI_MIP_Display::dmaBusy(void)
{
  if (!DMA_Enabled || !spiBusyCheck) return false;

  spi_transaction_t *rtrans;
  esp_err_t ret;
  uint8_t checks = spiBusyCheck;
  for (int i = 0; i < checks; ++i)
  {
    ret = spi_device_get_trans_result(dmaHAL, &rtrans, 0);
    if (ret == ESP_OK) spiBusyCheck--;
  }

  // Serial.print("spiBusyCheck=");Serial.println(spiBusyCheck);
  if (spiBusyCheck == 0)  return false;
  return true;
}

/***************************************************************************************
**函数名称：dmaWait          Function name: dmaWait
**描述：等待DMA结束（阻塞！）  Description: Waiting for DMA to end (blocking!)
***************************************************************************************/
void JDI_MIP_Display::dmaWait(void)
{
  if (!DMA_Enabled || !spiBusyCheck)  return;
  spi_transaction_t *rtrans;
  esp_err_t ret;
  for (int i = 0; i < spiBusyCheck; ++i)
  {
    ret = spi_device_get_trans_result(dmaHAL, &rtrans, portMAX_DELAY);
    assert(ret == ESP_OK);
  }
  spiBusyCheck = 0;
}

/***************************************************************************************
** 函数名称：pushPixelsDMA                  Function name: pushPixelsDMA
** 说明：将像素推送到TFT（len必须小于32767）  Explanation: Push pixels to TFT (len must be less than 32767)
***************************************************************************************/
void JDI_MIP_Display::pushPixelsDMA(uint8_t *image, uint32_t len)
{
    _pushPixelsDMA(image, len);
}
void JDI_MIP_Display::_pushPixelsDMA(uint8_t *image, uint32_t len)
{
    if ((len == 0) || (!DMA_Enabled)) return;

    dmaWait();

    /*if (_swapBytes)
    {
      for (uint32_t i = 0; i < len; i++)
        (image[i] = image[i] << 8 | image[i] >> 8);
    }*/

    esp_err_t ret;
    static spi_transaction_t trans;

    memset(&trans, 0, sizeof(spi_transaction_t));

    trans.user = (void *)1;
    trans.tx_buffer = image; // 数据指针
    trans.length = len * 8;  // 数据长度，以位为单位 uint8_t = 8bit uint16_t = 16bit
    trans.flags = 0;         // SPI_TRANS_USE_TXDATA标志
    trans.rx_buffer = NULL;  // 指向接收缓冲区的指针，或NULL表示无MISO阶段。如果使用DMA，则以4字节为单位写入。

    ret = spi_device_queue_trans(dmaHAL, &trans, portMAX_DELAY);
    assert(ret == ESP_OK);
    // Serial.print("ret:"); Serial.println(ret);
    spiBusyCheck++;
}

/***************************************************************************************
** 函数名称：initDMA                            Function name: initDMA
** 描述：初始化DMA引擎-如果初始化正常则返回true   Description: Initialize DMA engine - returns true if initialization is normal
***************************************************************************************/
bool JDI_MIP_Display::initDMA(int sck, int miso, int mosi, int ss, int fre)
{
    if (DMA_Enabled) return false;

    /*Serial.print("sck:"); Serial.println(sck);
    Serial.print("miso:"); Serial.println(miso);
    Serial.print("mosi:"); Serial.println(mosi);
    Serial.print("ss:"); Serial.println(ss);
    Serial.print("fre:"); Serial.println(fre);*/

    esp_err_t ret;
    // 指向spi_bus_config_t结构的指针，该结构指定应如何初始化主机
    spi_bus_config_t buscfg = {
        .mosi_io_num = mosi,
        .miso_io_num = miso,
        .sclk_io_num = sck,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .data4_io_num = -1,
        .data5_io_num = -1,
        .data6_io_num = -1,
        .data7_io_num = -1,
        .max_transfer_sz = 1024, // TFT屏幕尺寸 DISPLAY_WIDTH * DISPLAY_HEIGHT * 2 + 8
        .flags = 0,
        .intr_flags = 0};

    // 是否存在输入的cs引脚
    int8_t pin = -1;
    if (ss) pin = ss;

    // 设备的SPI接口协议配置
    spi_device_interface_config_t devcfg = {
        .command_bits = 0, // 命令阶段
        .address_bits = 0, // 地址阶段
        .dummy_bits = 0,
        .mode = SPI_MODE0, // 模式
        .duty_cycle_pos = 0,
        .cs_ena_pretrans = 0,
        .cs_ena_posttrans = 0,
        .clock_speed_hz = fre, // 频率
        .input_delay_ns = 0,
        .spics_io_num = pin,
        .flags = SPI_DEVICE_POSITIVE_CS, // 0, SPI_DEVICE_NO_DUMMY
        .queue_size = 2,
        .pre_cb = 0, // dc_callback，//回调处理D/C行
        .post_cb = 0};

    // spi总线初始化 esp32c3只能auto DMA 通道
    ret = spi_bus_initialize(spi_host, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    // Serial.print("ret0:"); Serial.println(ret);
    ret = spi_bus_add_device(spi_host, &devcfg, &dmaHAL); // spi总线添加设备
    ESP_ERROR_CHECK(ret);
    // Serial.print("ret1:"); Serial.println(ret);

    DMA_Enabled = true;
    spiBusyCheck = 0;
    return true;
}

/***************************************************************************************
** 函数名称：deInitDMA          Function name: deInitDMA
** 说明：断开DMA引擎与SPI的连接  Description: Disconnect the DMA engine from SPI
***************************************************************************************/
void JDI_MIP_Display::deInitDMA(void)
{
  if (!DMA_Enabled) return;
  spi_bus_remove_device(dmaHAL); // spi总线移除装置
  spi_bus_free(spi_host);        // spi总线自由
  DMA_Enabled = false;
}
#endif