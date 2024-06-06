#include "Graphics.h"
#include "Adafruit_NeoPixel.h"
#include "leds.h"

/* clang-format off */

uint8_t upperCase [30][3] = {{
0x1e, 0x05, 0x1e, },{ 0x1f, 0x15, 0x0a, },{ 
0x1f, 0x11, 0x11, },{ 0x1f, 0x11, 0x0e, },{ 0x1f, 0x15, 0x11, },{ 0x1f, 0x05, 0x01, },{ 
0x0e, 0x15, 0x1d, },{ 0x1f, 0x04, 0x1f, },{ 0x11, 0x1f, 0x11, },{ 0x08, 0x10, 0x0f, },{ 
0x1f, 0x04, 0x1b, },{ 0x1f, 0x10, 0x10, },{ 0x1f, 0x07, 0x1f, },{ 0x1f, 0x01, 0x1f, },{ 
0x1f, 0x11, 0x1f, },{ 0x1f, 0x05, 0x07, },{ 0x0f, 0x09, 0x17, },{ 0x1f, 0x0d, 0x17, },{ 
0x17, 0x15, 0x1d, },{ 0x01, 0x1f, 0x01, },{ 0x1f, 0x10, 0x1f, },{ 0x0f, 0x10, 0x0f, },{ 
0x1f, 0x0c, 0x1f, },{ 0x1b, 0x04, 0x1b, },{ 0x03, 0x1c, 0x03, },{ 0x19, 0x15, 0x13, },{ //Z

}};

uint8_t lowerCase [30][3] = {{
    0x1c, 0x0a, 0x1c, },{ 0x1e, 0x14, 0x08, },{ 0x0c, 0x12, 0x12, },{ 0x08, 0x14, 0x1e, },{ 
0x0e, 0x16, 0x14, },{ 0x1c, 0x0a, 0x02, },{ 0x14, 0x16, 0x0e, },{ 0x1e, 0x04, 0x18, },{ 
0x00, 0x1d, 0x00, },{ 0x10, 0x0d, 0x00, },{ 0x1e, 0x08, 0x16, },{ 0x00, 0x1e, 0x10, },{ 
0x1e, 0x06, 0x1e, },{ 0x1e, 0x02, 0x1c, },{ 0x1e, 0x12, 0x1e, },{ 0x1e, 0x0a, 0x04, },{ 
0x04, 0x0a, 0x1c, },{ 0x1e, 0x02, 0x04, },{ 0x14, 0x1a, 0x0a, },{ 0x04, 0x1e, 0x14, },{ 
0x1e, 0x10, 0x1e, },{ 0x0e, 0x10, 0x0e, },{ 0x1e, 0x18, 0x1e, },{ 0x16, 0x08, 0x16, },{ 
0x06, 0x18, 0x06, },{ 0x12, 0x1a, 0x16, }}; //z

uint8_t fontNumbers [10][3] = {{
0x1f, 0x11, 0x1f, },{ 0x12, 0x1f, 0x10, },{ 0x1d, 0x15, 0x17, },{ 0x11, 0x15, 0x1f, },{ 
0x07, 0x04, 0x1f, },{ 0x17, 0x15, 0x1d, },{ 0x1f, 0x15, 0x1d, },{ 0x19, 0x05, 0x03, },{ 
0x1f, 0x15, 0x1f, },{ 0x17, 0x15, 0x1f, }}; //9

uint8_t symbols [50][3] = {
{ 0x00, 0x17, 0x00, }, //'!'
{ 0x16, 0x1f, 0x0d, }, //$
{ 0x19, 0x04, 0x13, }, //%
{ 0x02, 0x01, 0x02, }, //^
{ 0x02, 0x07, 0x02, }, //'*'
{ 0x10, 0x10, 0x10, }, //_
{ 0x04, 0x04, 0x04, }, //-
{ 0x04, 0x0e, 0x04, }, //+
{ 0x04, 0x15, 0x04, }, //÷
{ 0x0a, 0x04, 0x0a, }, //x
{ 0x0a, 0x0a, 0x0a, }, //=
{ 0x12, 0x17, 0x12, }, //±
{ 0x01, 0x1d, 0x07, }, //?
{ 0x04, 0x0a, 0x11, }, //<
{ 0x11, 0x0a, 0x04, }, //>
{ 0x06, 0x04, 0x0c, }, //~
{ 0x01, 0x02, 0x00, }, //'
{ 0x10, 0x08, 0x00, }, //,
{ 0x00, 0x10, 0x00, }, //.
{ 0x18, 0x04, 0x03, }, // '/'
{ 0x03, 0x04, 0x18, }, // '\'
{ 0x00, 0x0e, 0x11, }, // (
{ 0x11, 0x0e, 0x00, }, // )
{ 0x00, 0x1f, 0x11, }, // [
{ 0x00, 0x11, 0x1f, }, // ]
{ 0x04, 0x0e, 0x1b, }, // {
{ 0x1b, 0x0e, 0x04, }, // }
{ 0x00, 0x1f, 0x00, }, // |
{ 0x10, 0x0a, 0x00, }, // ;
{ 0x00, 0x0a, 0x00, }, // :
{ 0x1e, 0x08, 0x06, }, // µ
{ 0x07, 0x05, 0x07, }, // °
{ 0x04, 0x0e, 0x1f, }, // ❬ thicc <
{ 0x1f, 0x0e, 0x04, }, // ❭ thicc >
{ 0x03, 0x00, 0x03, }, // "
{ 0x00, 0x03, 0x00, }, // '
{ 0x0a, 0x0f, 0x08, }, // 𝟷
{ 0x0d, 0x0b, 0x00, }, // 𝟸
{ 0x09, 0x0b, 0x0f, }};// 𝟹

// char symbolMap[40] = {
// '!', '$', '%', '^', '*', '_', '-', '+', '÷', 'x', '=', '±', '?', '<', '>', '~', '\'', ',', '.', '/', '\\', '(', ')', '[', ']', '{', '}', '|', ';', ':', 'µ', '°', '❬', '❭', '"', '\'', '𝟷', '𝟸', '𝟹'};

wchar_t fontMap[120] = {
'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 
'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
'!', '$', '%', '^', '*', '_', '-', '+', L'÷', 'x', '=', L'±', '?', '<', '>', '~', '\'', ',', '.', '/', '\\', 
'(', ')', '[', ']', '{', '}', '|', ';', ':', L'µ', L'°', L'❬', L'❭', '"', '\'', L'𝟷', L'𝟸', L'𝟹'};



uint8_t font[][3] = // 'JumperlessFontmap', 500x5px
{{
0x1f, 0x11, 0x1f, },{ 0x12, 0x1f, 0x10, },{ 0x1d, 0x15, 0x17, },{ 0x11, 0x15, 0x1f, },{ 
0x07, 0x04, 0x1f, },{ 0x17, 0x15, 0x1d, },{ 0x1f, 0x15, 0x1d, },{ 0x19, 0x05, 0x03, },{ 
0x1f, 0x15, 0x1f, },{ 0x17, 0x15, 0x1f, },{ //9

0x1e, 0x05, 0x1e, },{ 0x1f, 0x15, 0x0a, },{ 
0x1f, 0x11, 0x11, },{ 0x1f, 0x11, 0x0e, },{ 0x1f, 0x15, 0x11, },{ 0x1f, 0x05, 0x01, },{ 
0x0e, 0x15, 0x1d, },{ 0x1f, 0x04, 0x1f, },{ 0x11, 0x1f, 0x11, },{ 0x08, 0x10, 0x0f, },{ 
0x1f, 0x04, 0x1b, },{ 0x1f, 0x10, 0x10, },{ 0x1f, 0x07, 0x1f, },{ 0x1f, 0x01, 0x1f, },{ 
0x1f, 0x11, 0x1f, },{ 0x1f, 0x05, 0x07, },{ 0x0f, 0x09, 0x17, },{ 0x1f, 0x0d, 0x17, },{ 
0x17, 0x15, 0x1d, },{ 0x01, 0x1f, 0x01, },{ 0x1f, 0x10, 0x1f, },{ 0x0f, 0x10, 0x0f, },{ 
0x1f, 0x0c, 0x1f, },{ 0x1b, 0x04, 0x1b, },{ 0x03, 0x1c, 0x03, },{ 0x19, 0x15, 0x13, },{ //Z

0x1c, 0x0a, 0x1c, },{ 0x1e, 0x14, 0x08, },{ 0x0c, 0x12, 0x12, },{ 0x08, 0x14, 0x1e, },{ 
0x0e, 0x16, 0x14, },{ 0x1c, 0x0a, 0x02, },{ 0x14, 0x16, 0x0e, },{ 0x1e, 0x04, 0x18, },{ 
0x00, 0x1d, 0x00, },{ 0x10, 0x0d, 0x00, },{ 0x1e, 0x08, 0x16, },{ 0x00, 0x1e, 0x10, },{ 
0x1e, 0x06, 0x1e, },{ 0x1e, 0x02, 0x1c, },{ 0x1e, 0x12, 0x1e, },{ 0x1e, 0x0a, 0x04, },{ 
0x04, 0x0a, 0x1c, },{ 0x1e, 0x02, 0x04, },{ 0x14, 0x1a, 0x0a, },{ 0x04, 0x1e, 0x14, },{ 
0x1e, 0x10, 0x1e, },{ 0x0e, 0x10, 0x0e, },{ 0x1e, 0x18, 0x1e, },{ 0x16, 0x08, 0x16, },{ 
0x06, 0x18, 0x06, },{ 0x12, 0x1a, 0x16, },{ //z
    
0x00, 0x17, 0x00, },{ 0x16, 0x1f, 0x0d, },{ 
0x19, 0x04, 0x13, },{ 0x02, 0x01, 0x02, },{ 0x02, 0x07, 0x02, },{ 0x10, 0x10, 0x10, },{ 
0x04, 0x04, 0x04, },{ 0x04, 0x0e, 0x04, },{ 0x04, 0x15, 0x04, },{ 0x0a, 0x04, 0x0a, },{ 
0x0a, 0x0a, 0x0a, },{ 0x12, 0x17, 0x12, },{ 0x01, 0x1d, 0x07, },{ 0x04, 0x0a, 0x11, },{ 
0x11, 0x0a, 0x04, },{ 0x06, 0x04, 0x0c, },{ 0x01, 0x02, 0x00, },{ 0x10, 0x08, 0x00, },{ 
0x00, 0x10, 0x00, },{ 0x18, 0x04, 0x03, },{ 0x03, 0x04, 0x18, },{ 0x00, 0x0e, 0x11, },{ 
0x11, 0x0e, 0x00, },{ 0x00, 0x1f, 0x11, },{ 0x00, 0x11, 0x1f, },{ 0x04, 0x0e, 0x1b, },{ 
0x1b, 0x0e, 0x04, },{ 0x00, 0x1f, 0x00, },{ 0x10, 0x0a, 0x00, },{ 0x00, 0x0a, 0x00, },{ 
0x1e, 0x08, 0x06, },{ 0x07, 0x05, 0x07, },{ 0x04, 0x0e, 0x1f, },{ 0x1f, 0x0e, 0x04, },{ 
0x03, 0x00, 0x03, },{ 0x00, 0x03, 0x00, },{ 0x0a, 0x0f, 0x08, },{ 0x0d, 0x0b, 0x00, },{ 
0x09, 0x0b, 0x0f, }};

/* clang-format on */

// class bread {
//     public:
//     bread();
//     void print(char c);
//     void print(char c, uint32_t color);
//     void print(char c, uint32_t color, int position, int topBottom);
//     void print(char c, uint32_t color, uint32_t backgroundColor);
//     void print(char c, uint32_t color, uint32_t backgroundColor, int
//     position, int topBottom); void print(char c, uint32_t color, uint32_t
//     backgroundColor, int position);

//     void print(char* s);
//     void print(char* s, uint32_t color);
//     void print(char* s, uint32_t color, int position, int topBottom);
//     void print(char* s, uint32_t color, uint32_t backgroundColor);
//     void print(char* s, uint32_t color, uint32_t backgroundColor, int
//     position, int topBottom); void print(char* s, uint32_t color, uint32_t
//     backgroundColor, int position);

//     // void printChar(char c);
//     // void printString(char* s);
//     // void printChar(char c, uint32_t color);
//     // void printString(char* s, uint32_t color);
//     // void printChar(char c, uint32_t color, int x, int y);
//     // void printString(char* s, uint32_t color, int x, int y);
//     // void printChar(char c, uint32_t color, int topBottom);
//     // void printString(char* s, uint32_t color, int topBottom);

// }
uint32_t defaultColor = 0x001012;

bread::bread() {
  // defaultColor = 0x060205;
}

void bread::print(const char c) { printChar(c, defaultColor); }

void bread::print(const char c, int position) {
  printChar(c, defaultColor, position);
}

void bread::print(const char c, uint32_t color) { printChar(c, color); }

void bread::print(const char c, uint32_t color, int position, int topBottom) {
  printChar(c, color, position, topBottom);
}

void bread::print(const char c, uint32_t color, int topBottom) {
  printChar(c, color, topBottom);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor) {
  printChar(c, color, backgroundColor);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom) {
  printChar(c, color, backgroundColor, position, topBottom);
}

void bread::print(const char *s) {
  // Serial.println("1");
  printString(s, defaultColor);
}

void bread::print(const char *s, int position) {
  // Serial.println("2");
  printString(s, defaultColor, 0xffffff, position);
}

void bread::print(const char *s, uint32_t color) {
  // Serial.println("3");
  printString(s, color);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor) {
  // Serial.println("4");
  printString(s, color, backgroundColor);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom, nudge);
}

void bread::print(const char *s, uint32_t color, uint32_t backgroundColor,
                  int topBottom) {
  // Serial.println("6");
  printString(s, color, backgroundColor, 0, topBottom);
}

void bread::print(int i) {
  // Serial.println("7");
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, defaultColor);
  // Serial.println(buffer);
}

void bread::print(int i, int position) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, defaultColor, 0xffffff, position);
}

void bread::print(int i, uint32_t color) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color);
}

void bread::print(int i, uint32_t color, int position) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position);
}

void bread::print(int i, uint32_t color, int position, int topBottom) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position, topBottom);
}
void bread::print(int i, uint32_t color, int position, int topBottom,
                  int nudge) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, 0xffffff, position, topBottom, nudge);
}

void bread::print(int i, uint32_t color, uint32_t backgroundColor) {
  char buffer[15];
  itoa(i, buffer, 10);
  printString(buffer, color, backgroundColor);
}



void bread::printMenuReminder(int menuDepth, uint32_t color) {
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};
  uint8_t graphicRow[3] = {0x00, 0x00, 0x00};

  if (menuDepth > 6) {
    menuDepth = 6;
  }

  switch (menuDepth) {
  case 1:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00010000;
    graphicRow[0] = 0b00010000;
    break;
  case 2:
    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00001000;
    graphicRow[0] = 0b00011000;

    break;

  case 3:
    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000100;
    graphicRow[0] = 0b00011100;

    break;

  case 4:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000010;
    graphicRow[0] = 0b00011110;

    break;

  case 5:

    graphicRow[2] = 0b00000000;
    graphicRow[1] = 0b00000001;
    graphicRow[0] = 0b00011111;

    break;

  case 6:

    graphicRow[2] = 0b00000001;
    graphicRow[1] = 0b00000001;
    graphicRow[0] = 0b00011111;

    break;
  }

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }

  for (int i = 0; i < 3; i++) {

    printGraphicsRow(graphicRow[i], i, color);
  }





}
/*

||||||||||||||||||||||||||||||
  |0| |1| |2| |3| |4| |5| |6|
||||||||||||||||||||||||||||||

||||||||||||||||||||||||||||||
  |7| |8| |9| |A| |B| |C| |D|
||||||||||||||||||||||||||||||


*/
void printGraphicsRow(uint8_t data, int row, uint32_t color, uint32_t bg) {
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00010000, 0b00001000, 0b00000100, 0b00000010, 0b00000001};

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }
  if (bg == 0xFFFFFF) {

    for (int j = 4; j >= 0; j--) {
      //Serial.println(((data) & columnMask[j]) != 0 ? "1" : "0");
      if (((data) & columnMask[j]) != 0) {
        
        leds.setPixelColor(((row ) * 5) + j, color);
      } else {
        leds.setPixelColor(((row ) * 5) + j, 0);
      }
    }
    } else {
      for (int j = 4; j >= 0; j--) {
        if (((data) & columnMask[j]) != 0) {
          leds.setPixelColor(((row) * 5) + j, color);
        } else {
          leds.setPixelColor(((row) * 5) + j, bg);
        }
      }
    }
  
}

void printChar(const char c, uint32_t color, uint32_t bg, int position,
               int topBottom, int nudge) {

  int charPosition = position;
  if (topBottom == 1) {
    charPosition = charPosition % 7;
    charPosition += 7;
  }
  if (topBottom == 0) {
    charPosition = charPosition % 7;
  }

  charPosition = charPosition % 14;

  charPosition = charPosition * 4;

  if (charPosition > (6 * 4)) {
    charPosition = charPosition + 2;
  }
  // charPosition = charPosition * 4;
  charPosition = charPosition + 2;

  if (color == 0xFFFFFF) {
    color = defaultColor;
  }
  int fontMapIndex = -1;
  for (int i = 0; i < 100; i++) {
    if (c == fontMap[i]) {
      fontMapIndex = i;
      break;
    }
  }
  if (fontMapIndex == -1) {
    return;
  }
  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};

  if (bg == 0xFFFFFF) {
    for (int i = 0; i < 3; i++) {
      for (int j = 0; j < 5; j++) {
        if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
        } else {
          // leds.setPixelColor((i*5)+j, bg);
        }
      }
    }
  } else {

    for (int i = 0; i < 4; i++) {
      if (i < 3) {
        for (int j = 0; j < 5; j++) {
          if (((font[fontMapIndex][i]) & columnMask[j]) != 0) {
            leds.setPixelColor(((charPosition + i + nudge) * 5) + j, color);
          } else {
            leds.setPixelColor(((charPosition + i + nudge) * 5) + j, bg);
          }
        }
      } else {
        for (int j = 0; j < 5; j++) {
          leds.setPixelColor(((charPosition + i + nudge) * 5) + j, bg);
        }
      }
    }
  }
}

void printString(const char *s, uint32_t color, uint32_t bg, int position,
                 int topBottom, int nudge) {
  // int position = 0;

  for (int i = 0; i < strlen(s); i++) {

    if (topBottom == 1) {
      position = position % 7;

    } else if (topBottom == 0) {
      position = position % 7;

    } else {
      position = position % 14;
    }
    // Serial.print(s[i]);
    // Serial.print(" ");
    // Serial.println(position);
    // if (i > strlen(s))
    // {
    //     printChar(' ', 0x000000, 0x000000, position, topBottom);
    // } else {
    // Serial.println(position);
    printChar(s[i], color, bg, position, topBottom, nudge);
    // }

    position++;
  }
}

void bread::clear() {
  for (int i = 0; i < 60; i++) {
    for (int j = 0; j < 5; j++) {
      leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
    }
  }
  // leds.show();
}

void scrollFont() {
  // pauseCore2 = 1;
  //  scroll font
  //  uint8_t font[] = // 'JumperlessFontmap', 500x5px
  //  {0x1f, 0x11, 0x1f, 0x00, 0x12, 0x1f, 0x10, 0x00, 0x1d, 0x15, 0x17, 0x00,
  //  0x11, 0x15, 0x1f, 0x00,};
  uint32_t color = 0x060205;
  int scrollSpeed = 120;
  int scrollPosition = 0;

  uint8_t columnMask[5] = // 'JumperlessFontmap', 500x5px
      {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000};
  while (Serial.available() == 0) {

    for (int i = 0; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        if ((font[(i + scrollPosition) % 500][0] & columnMask[j]) != 0) {
          // Serial.print("1");
          leds.setPixelColor((i * 5) + j, color);
        } else {
          // Serial.print("0");
          leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
        }
      }
    }
    leds.show();
    delay(scrollSpeed);
    scrollPosition++;
    if (scrollPosition > 499) {
      scrollPosition = 0;
    }
  }
}
