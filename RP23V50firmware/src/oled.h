#ifndef OLED_H
#define OLED_H

#include "Adafruit_SSD1306.h"

#include "JumperlessDefines.h"
#include "Arduino.h"
#include "Wire.h"

class Adafruit_SSD1306;

extern bool oledConnected;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

class oled {
  public:
  oled();
  int init();
  void test();
  void print(const char* s);
  void print(int i);
  void print(const char c);
  void print(const char c, int position);
  void displayBitmap(int x, int y, const unsigned char* bitmap, int width, int height);
  void showJogo32h();
  void clear();
  void setCursor(int x, int y);
  void setTextColor(uint32_t color);
  void setTextSize(uint8_t size);
  void setTextWrap(bool wrap);
  void setRotation(uint8_t rotation);
  void setContrast(uint8_t contrast);
  void show();
  bool isConnected() const;
  void sendCommand(uint8_t cmd);
  void invertDisplay(bool inv);
  int connect(void);
  void disconnect(void);
  void printf(const char* format, ...);
  void clrPrintfsh(const char* format, ...);
  void clearPrintShow(const char* c, int size = 2, int x_pos = 4, int y_pos = 3, bool clear = true);


  // Store config
  int address = -1;
  int sda_pin = -1;
  int scl_pin = -1;
  int sda_row = -1;
  int scl_row = -1;

  bool connected = false;
};

extern const unsigned char jogo255[];
extern const unsigned char jogo32h[];
extern const unsigned char logo[];
extern const unsigned char ColorJumpLogo[];






int initOLED(void);

int connectOLED(void);

void scanI2CAddresses(int startAddress);
int oledTest(int sdaRow =  NANO_D2, int sclRow = NANO_D3, int sdaPin = 26, int sclPin = 27, int leaveConnections = 1);

extern class oled oled;




































#endif


