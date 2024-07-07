#include "Graphics.h"
#include "Adafruit_NeoPixel.h"
#include "JumperlessDefinesRP2040.h"
#include "MatrixStateRP2040.h"
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
'(', ')', '[', ']', '{', '}', '|', ';', ':', L'µ', L'°', L'❬', L'❭', '"', '\'', L'𝟷', L'𝟸', L'𝟹', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};



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
0x11, 0x0a, 0x04, },{ 0x12, 0x17, 0x12, },{ 0x01, 0x02, 0x00, },{ 0x10, 0x08, 0x00, },{ 
0x00, 0x10, 0x00, },{ 0x18, 0x04, 0x03, },{ 0x03, 0x04, 0x18, },{ 0x00, 0x0e, 0x11, },{ 
0x11, 0x0e, 0x00, },{ 0x00, 0x1f, 0x11, },{ 0x00, 0x11, 0x1f, },{ 0x04, 0x0e, 0x1b, },{ 
0x1b, 0x0e, 0x04, },{ 0x00, 0x1f, 0x00, },{ 0x10, 0x0a, 0x00, },{ 0x00, 0x0a, 0x00, },{ 
0x1e, 0x08, 0x06, },{ 0x07, 0x05, 0x07, },{ 0x04, 0x0e, 0x1f, },{ 0x1f, 0x0e, 0x04, },{ 
0x03, 0x00, 0x03, },{ 0x00, 0x03, 0x00, },{ 0x0a, 0x0f, 0x08, },{ 0x0d, 0x0b, 0x00, },{ 
0x09, 0x0b, 0x0f, },{

0x1e, 0x12, 0x1e, },{ 0x14, 0x1e, 0x10, },{ 0x1a, 0x12, 0x16, },{ 0x12, 0x16, 0x1e, },{ //lowercase Numbers
0x0e, 0x08, 0x1e, },{ 0x16, 0x12, 0x1a, },{ 0x1e, 0x1a, 0x1a, },{ 0x12, 0x0a, 0x06, },{ 
0x1e, 0x1a, 0x1e, },{ 0x16, 0x16, 0x1e, }

};


int wireStatus[62][5]; // row, led (net stored)

/* clang-format on */



void drawWires(int net) {
  // int fillSequence[6] = {0,2,4,1,3,};
  assignNetColors();
  int fillSequence[6] = {0, 1, 2, 3, 4};
  int fillIndex = 0;
  int filledPaths[60][3] = {-1}; // node1 node2 rowfilled

  for (int i = 0; i < 62; i++) {
    for (int j = 0; j < 5; j++) {
      wireStatus[i][j] = 0;
    }
  }
  if (net == -1) {

    for (int i = 0; i < numberOfNets; i++) {

      int sameLevel = 0;
      int bothOnTop = 0;
      int bothOnBottom = 0;
      int bothOnBB = 0;
      int whichIsLarger = 0;

      if (path[i].node1 != -1 && path[i].node2 != -1 &&
          path[i].node1 != path[i].node2) {
        if (path[i].node1 <= 60 && path[i].node2 <= 60) {
          bothOnBB = 1;
          if (path[i].node1 > 0 && path[i].node1 <= 30 && path[i].node2 > 0 &&
              path[i].node2 <= 30) {
            bothOnTop = 1;
            sameLevel = 1;
            if (path[i].node1 > path[i].node2) {
              whichIsLarger = 1;
            } else {
              whichIsLarger = 2;
            }
          } else if (path[i].node1 > 30 && path[i].node1 <= 60 &&
                     path[i].node2 > 30 && path[i].node2 <= 60) {
            bothOnBottom = 1;
            sameLevel = 1;
            if (path[i].node1 > path[i].node2) {
              whichIsLarger = 1;
            } else {
              whichIsLarger = 2;
            }
          }
        } else {
          lightUpNet(path[i].net);
        }

        if (sameLevel == 1) {
          int range = 0;
          int first = 0;
          int last = 0;
          if (whichIsLarger == 1) {
            range = path[i].node1 - path[i].node2;
            first = path[i].node2;
            last = path[i].node1;
          } else {
            range = path[i].node2 - path[i].node1;
            first = path[i].node1;
            last = path[i].node2;
          }

          // Serial.print("\nfirst = ");
          // Serial.println(first);
          // Serial.print("last = ");
          // Serial.println(last);
          // Serial.print("range = ");
          // Serial.println(range);
          // Serial.print("net = ");
          // Serial.println(path[i].net);
          int inside = 0;
          int largestFillIndex = 0;

          for (int j = first; j <= first + range; j++) {
            // Serial.print("j = ");
            // Serial.println(j);
            for (int w = 0; w < 5; w++) {
              if ((wireStatus[j][w] == path[i].net || wireStatus[j][w] == 0) &&
                  w >= largestFillIndex) {

                // wireStatus[j][w] = path[i].net;
                if (w > largestFillIndex) {
                  largestFillIndex = w;
                }
                // Serial.print("j = ");
                // Serial.println(j);
                // if (first > 30) {
                //   Serial.print("bottom ");
                // }
                // Serial.print("largestFillIndex = ");
                // Serial.println(largestFillIndex);
                break;
              }
            }
          }

          for (int j = first; j <= first + range; j++) {
            if (j == first || j == last) {
              for (int k = largestFillIndex; k < 5; k++) {

                wireStatus[j][k] = path[i].net;
                // wireStatus[j][largestFillIndex] = path[i].net;
              }
            } else {
              wireStatus[j][largestFillIndex] = path[i].net;
            }
          }

          fillIndex = largestFillIndex;


          filledPaths[i][0] = first;
          filledPaths[i][1] = last;
          filledPaths[i][2] = fillSequence[fillIndex];
          // showLEDsCore2 = 1;
        } else {
          for (int j = 0; j < 5; j++) {
            
              wireStatus[path[i].node1][j] = path[i].net;
              wireStatus[path[i].node2][j] = path[i].net;
              
            }
          }


      }
    }
    for (int i = 0; i <= 60; i++) {
      for (int j = 0; j < 4; j++) {
        if (wireStatus[i][j] != 0) {
          if (wireStatus[i][j+1] != wireStatus[i][j] && wireStatus[i][j+1] != 0 && wireStatus[i][4] == wireStatus[i][j]) {
            wireStatus[i][j+1] = wireStatus[i][j];
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
          } else {
            // leds.setPixelColor((i * 5) + fillSequence[j], 0x100010);
          }

        }
      }
    }
    for (int i = 1; i <= 60; i++) {
      if (i <= 30){

      for (int j = 0; j < 5; j++) {

        uint32_t color3 = 0x100010;

        rgbColor colorRGB = netColors[wireStatus[i][j]];

        hsvColor colorHSV = RgbToHsv(colorRGB);

        // colorHSV.v = colorHSV.v * 0.25;
        // colorHSV.s = colorHSV.s * 0.5;
        colorRGB = HsvToRgb(colorHSV);

        uint32_t color = packRgb(colorRGB.r, colorRGB.g, colorRGB.b);

        if (wireStatus[i][j] == 0) {
          // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
        } else {
          leds.setPixelColor((((i - 1) * 5) + j), color);
        }
      }} else {
            for (int j = 0; j < 5; j++) {

        uint32_t color3 = 0x100010;

        rgbColor colorRGB = netColors[wireStatus[i][j]];

        hsvColor colorHSV = RgbToHsv(colorRGB);

        // colorHSV.v = colorHSV.v * 0.25;
        // colorHSV.s = colorHSV.s * 0.5;
        colorRGB = HsvToRgb(colorHSV);

        uint32_t color = packRgb(colorRGB.r, colorRGB.g, colorRGB.b);

        if (wireStatus[i][j] == 0) {
          // leds.setPixelColor((i * 5) + fillSequence[j], 0x000000);
        } else {
          leds.setPixelColor((((i - 1) * 5) + (4-j)), color);
        }
      }
    }
    }
  }
}
void printWireStatus(void)
{


          for (int s = 1; s <= 30; s++) {
            Serial.print(s);
            Serial.print(" ");
            if (s < 9) {
              Serial.print(" ");
            }
          }
          Serial.println();

          int level = 1;
          for (int r = 0; r < 5; r++) {
            for (int s = 1; s <= 30; s++) {
              Serial.print(wireStatus[s][r]);
              Serial.print(" ");
              if (wireStatus[s][r] < 10) {
                Serial.print(" ");
              }
            }
            Serial.println();
          }
          Serial.println("\n\n");
          for (int s = 31; s <= 60; s++) {
            Serial.print(s);
            Serial.print(" ");
            if (s < 9) {
              Serial.print(" ");
            }
          }
          Serial.println();
          for (int r = 0; r < 5; r++) {
            for (int s = 31; s <= 60; s++) {
              Serial.print(wireStatus[s][r]);
              Serial.print(" ");
              if (wireStatus[s][r] < 10) {
                Serial.print(" ");
              }
            }
            Serial.println();
          }

}
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

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge) {
  printChar(c, color, backgroundColor, position, topBottom, nudge);
}

void bread::print(const char c, uint32_t color, uint32_t backgroundColor,
                  int position, int topBottom, int nudge, int lowercaseNumber) {
  printChar(c, color, backgroundColor, position, topBottom, nudge,
            lowercaseNumber);
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
                  int position, int topBottom, int nudge, int lowercaseNumber) {
  // Serial.println("5");
  printString(s, color, backgroundColor, position, topBottom, nudge,
              lowercaseNumber);
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

// void bread::print(int i, int position) {
//   char buffer[15];
//   itoa(i, buffer, 10);
//   printString(buffer, defaultColor, 0xffffff, position);
// }

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
void bread::print(int i, uint32_t color, int position, int topBottom, int nudge,
                  int lowercase) {
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

void bread::printRawRow(uint8_t data, int row, uint32_t color, uint32_t bg) {
  printGraphicsRow(data, row, color, bg);
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
      // Serial.println(((data) & columnMask[j]) != 0 ? "1" : "0");
      if (((data)&columnMask[j]) != 0) {

        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        leds.setPixelColor(((row) * 5) + j, 0);
      }
    }
  } else if (bg == 0xFFFFFE) {

    for (int j = 4; j >= 0; j--) {
      // Serial.println(((data) & columnMask[j]) != 0 ? "1" : "0");
      if (((data)&columnMask[j]) != 0) {

        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        // leds.getPixelColor(((row) * 5) + j);
        // leds.setPixelColor(((row) * 5) + j, 0);
      }
    }
  } else {
    for (int j = 4; j >= 0; j--) {
      if (((data)&columnMask[j]) != 0) {
        leds.setPixelColor(((row) * 5) + j, color);
      } else {
        leds.setPixelColor(((row) * 5) + j, bg);
      }
    }
  }
}

void printChar(const char c, uint32_t color, uint32_t bg, int position,
               int topBottom, int nudge, int lowercaseNumber) {

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
  int start = 0;

  if (lowercaseNumber > 0) {
    start = 90;
  }

  for (int i = start; i < 120; i++) {
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
    if (charPosition + nudge != 0) {
      for (int j = 0; j < 5; j++) {

        leds.setPixelColor(((charPosition + nudge - 1) * 5) + j, bg);
      }
    }
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
                 int topBottom, int nudge, int lowercaseNumber) {
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
    printChar(s[i], color, bg, position, topBottom, nudge, lowercaseNumber);
    // }

    position++;
  }
  // Serial.println();
}

void bread::clear(int topBottom) {
  if (topBottom == -1) {
    for (int i = 0; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
    }
  } else if (topBottom == 0) {
    for (int i = 0; i < 30; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
    }
  } else if (topBottom == 1) {
    for (int i = 30; i < 60; i++) {
      for (int j = 0; j < 5; j++) {
        leds.setPixelColor((i * 5) + j, 0x00, 0x00, 0x00);
      }
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
