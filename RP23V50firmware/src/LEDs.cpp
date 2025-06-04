// SPDX-License-Identifier: MIT
#include "LEDs.h"
#include "Commands.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "MatrixState.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "Probing.h"
//#include <Adafruit_GFX.h>
// #include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
// #include <FastLED.h>
#include "Highlighting.h"
// CRGB probeLEDs[1];

// bool splitLEDs;
volatile int hideNets = 0;

///-2 will set to default
volatile uint32_t logoColorOverride = -1;
///-2 will set to default
volatile uint32_t logoColorOverrideTop = -1;
///-2 will set to default
volatile uint32_t logoColorOverrideBottom = -1;

uint32_t logoColorOverrideDefault = 0x4050b0;
uint32_t logoColorOverrideTopDefault = 0x302000;
uint32_t logoColorOverrideBottomDefault = 0x002540;

///-2 will set to default
volatile uint32_t ADCcolorOverride0 = -1;
///-2 will set to default
volatile uint32_t ADCcolorOverride1 = -1;
///-2 will set to default
volatile uint32_t DACcolorOverride0 = -1;
///-2 will set to default
volatile uint32_t DACcolorOverride1 = -1;
///-2 will set to default
volatile uint32_t GPIOcolorOverride0 = -1;
///-2 will set to default
volatile uint32_t GPIOcolorOverride1 = -1;

uint32_t RST0colorOverride = -1;
uint32_t RST1colorOverride = -1;
uint32_t GNDTcolorOverride = -1;
uint32_t GNDBcolorOverride = -1;
uint32_t VINcolorOverride = -1;
uint32_t V3V3colorOverride = -1;
uint32_t V5VcolorOverride = -1;

uint32_t ADCcolorOverride0Default = 0x4050b0;
uint32_t ADCcolorOverride1Default = 0x453080;
uint32_t DACcolorOverride0Default = 0x4050b0;
uint32_t DACcolorOverride1Default = 0x458040;
uint32_t GPIOcolorOverride0Default = 0x4050b0;
uint32_t GPIOcolorOverride1Default = 0x2560a0;

// Highlighting variables moved to Highlighting.cpp


// #if REV < 4
// bool splitLEDs = 0;
// Adafruit_NeoPixel bbleds(LED_COUNT + LED_COUNT_TOP, LED_PIN,
//                          NEO_GRB + NEO_KHZ800);
// Adafruit_NeoPixel topleds(1, -1, NEO_GRB + NEO_KHZ800);
// #elif REV >= 4
bool splitLEDs = 1;
Adafruit_NeoPixel bbleds(LED_COUNT + LED_COUNT_TOP, LED_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel topleds(LED_COUNT_TOP, LED_PIN_TOP, NEO_GRB + NEO_KHZ800);
//#endif

Adafruit_NeoPixel probeLEDs(1, PROBE_LED_PIN, NEO_GRB + NEO_KHZ800);
// Adafruit_NeoPixel probeLEDs(1, 9, NEO_GRB + NEO_KHZ800);

void ledClass::end(void) {
  bbleds.~Adafruit_NeoPixel();
  topleds.~Adafruit_NeoPixel();

  }

void ledClass::begin(void) {

  if (jumperlessConfig.hardware.revision <= 3) {
    splitLEDs = 0;

    } else {
    splitLEDs = 1;
    bbleds.updateLength(LED_COUNT);
    }

  if (splitLEDs == 1) {
    topleds.begin();
    }
  bbleds.begin();
  bbleds.setBrightness(254);
  topleds.setBrightness(254);
  }

void ledClass::show(void) {
  if (splitLEDs == 1) {
    topleds.show();
    }
  bbleds.show();
  }

void ledClass::setPixelColor(uint16_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n >= LED_COUNT && splitLEDs == 1) {
    topleds.setPixelColor(n - LED_COUNT, r, g, b);
    } else {
    bbleds.setPixelColor(n, r, g, b);
    }
  }

void ledClass::setPixelColor(uint16_t n, uint32_t c) {
  if (n >= LED_COUNT && splitLEDs == 1) {

    topleds.setPixelColor(n - LED_COUNT, c);

    } else {
    bbleds.setPixelColor(n, c);
    }
  }

void ledClass::fill(uint32_t c, uint16_t first, uint16_t count) {
  if (splitLEDs == 1) {
    topleds.fill(c, first, count);
    }
  bbleds.fill(c, first, count);
  }

void ledClass::setBrightness(uint8_t b) {
  if (splitLEDs == 1) {
    topleds.setBrightness(b);
    }
  bbleds.setBrightness(b);
  }

void ledClass::clear(void) {
  if (splitLEDs == 1) {
    topleds.clear();
    }
  bbleds.clear();
  }

uint32_t ledClass::getPixelColor(uint16_t n) {
  if (n >= LED_COUNT && splitLEDs == 1) {
    return topleds.getPixelColor(n - LED_COUNT);
    } else {
    return bbleds.getPixelColor(n);
    }
  }

uint16_t ledClass::numPixels(void) {
  if (splitLEDs == 1) {
    return topleds.numPixels();
    }
  return bbleds.numPixels();
  }

ledClass leds;



struct changedNetColors changedNetColors[MAX_NETS];
rgbColor netColors[MAX_NETS] = { 0 };

uint8_t saturation = 254;
volatile uint8_t LEDbrightness = DEFAULTBRIGHTNESS;
volatile uint8_t LEDbrightnessRail = DEFAULTRAILBRIGHTNESS;
volatile uint8_t LEDbrightnessSpecial = DEFAULTSPECIALNETBRIGHTNESS;

int netNumberC2 = 0;
int onOffC2 = 0;
int nodeC2 = 0;
int brightnessC2 = 0;
int hueShiftC2 = 0;
int lightUpNetCore2 = 0;

// More highlighting variables moved to Highlighting.cpp

int logoFlash = 0;
int numberOfShownNets = 0;
#ifdef EEPROMSTUFF
#include <EEPROM.h>
bool debugLEDs = 0; //= EEPROM.read(DEBUG_LEDSADDRESS);

#else
bool debugLEDs = 1;
#endif

int netColorMode = jumperlessConfig.display.net_color_mode;

uint32_t rawSpecialNetColors[8] = // dim
  { 0x000000, 0x001C04, 0x1C0702, 0x1C0107,
   0x231111, 0x230913, 0x232323, 0x232323 };

uint32_t rawOtherColors[15] = { // I'm using headerColors[] now
    0x010006, // headerglow
    0x6000A8, // logo / status
    0x0055AA, // logoflash / statusflash
    0x301A02, // +8V
    0x120932, // -8V
    0x443434, // UART TX
    0x324244, // UART RX
    0x232323,

    0x380303, // ADC inner
    0x166800, // DAC inner
    0x0005E5, // GPIO inner

    0x400048, // ADC outer
    0x453800, // DAC outer
    0x00a045, // GPIO outer

  };

rgbColor specialNetColors[8] = { {00, 00, 00},       {0x00, 0xFF, 0x30},
                                {0xFF, 0x41, 0x14}, {0xFF, 0x10, 0x40},
                                {0xeF, 0x78, 0x7a}, {0xeF, 0x40, 0x7f},
                                {0xFF, 0xff, 0xff}, {0xff, 0xFF, 0xff} };

rgbColor railColors[4] = { {0xFF, 0x32, 0x30},
                          {0x00, 0xFF, 0x30},
                          {0xFF, 0x32, 0x30},
                          {0x00, 0xFF, 0x30} };


//    uint32_t railColorsV5[4][3] = 
// {{0x1C0110, 0x001C04, 0x1C0110}, 
//  {0x210904, 0x001C04, 0x210904},
//  {0x301A02, 0x001C04, 0x120932},
//  {0x1C0110, 0x001C04, 0x1C0110}};

// int x = matrix.width();
int pass = 0;
// #define DATA_PIN 2
void initLEDs(void) {

  leds.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  // delay(1);
  leds.show();

  // if (splitLEDs == 1) {
  //   topleds.begin();
  //   topleds.show();
  // }

  //int claimedSms[2][4] = { {0, 0, 0, 0}, {0, 0, 0, 0} };

  // for (int i = 0; i < 4; i++) {
  //   if (pio_sm_is_claimed(pio0, i)) {
  //     claimedSms[0][i] = 1;
  //   }
  //   Serial.print("SM ");
  //   Serial.print(i);
  //   Serial.print(" is claimed: ");
  //   Serial.println(claimedSms[0][i]);
  // }

  probeLEDs.begin();
  probeLEDs.setPixelColor(0, 0x111111);
  probeLEDs.show();



  // Serial.println("\n\rprobeLEDs.begin()\n\r");

  //   for (int i = 0; i < 4; i++) {
  //   if (pio_sm_is_claimed(pio0,i)) {
  //     claimedSms[1][i] = 1;
  //   }
  //   Serial.print("SM ");
  //   Serial.print(i);
  //   Serial.print(" is claimed: ");
  //   Serial.println(claimedSms[1][i]);
  // }

  // EEPROM.commit();
  // delay(20);
  }

//uint32_t savedLEDcolors[NUM_SLOTS][LED_COUNT + 1];

int slotLEDpositions[20] = {
    418, 419, 420, 421, 422, 423, 424, 425, 426,
  };
int rotaryEncoderPositions[6] = {
    97, // AREF
    95, // D13 (button)
    94, // D12 (encoder A)
    93, // D11 (encoder GND)
    92, // D10 (encoder B)
  };

uint32_t slotSelectionColors[12] = {
    0x084080, // preview
    0x005555, // active
    0x102000, // inactive
    0x000000, // off
    0x881261, // preview+active

    0x253500, // rotary encoder High
    0x000060, // rotary encoder Low

    0x550011, // button High
    0x001C05, // button Low

  };


int colorDistance(rgbColor a, rgbColor b) {
  int dr = (int)a.r - (int)b.r;
  int dg = (int)a.g - (int)b.g;
  int db = (int)a.b - (int)b.b;
  return dr * dr + dg * dg + db * db;
  }

char* colorNameBuffer = (char*)malloc(10);



// Reference palette
///@brief Reference palette for color names and terminal colors
///@param color Full brightness reference color
///@param dimColor Specially calibrated color for dim matching
///@param name Color name
///@param hueStart Start of hue range (0-255)
///@param hueEnd End of hue range (0-255)
///@param termColor256 Terminal color for 256-color mode
///@param termColor16 Terminal color for 16-color mode
static const NamedColor colorNames[] = {
    {0xFF0000, 0x400000, "red       ", 253, 12, 196, 31},  // Red wraps around 0
    {0xFFA500, 0x401000, "orange    ", 13, 28, 208, 91},
    {0xFFBF00, 0x403000, "amber     ", 29, 35, 214, 33},
    {0xFFFF00, 0x404000, "yellow    ", 36, 60, 226, 93},
    {0x7FFF00, 0x104000, "chartreuse", 61, 72, 154, 92},
    {0x00FF00, 0x003000, "green     ", 73, 94, 82, 32},
    {0x2E8B57, 0x042040, "seafoam   ", 95, 109, 84, 96},
    {0x00FFFF, 0x004040, "cyan      ", 110, 135, 86, 96},
    {0x0000FF, 0x000040, "blue      ", 136, 164, 33, 36},
    {0x4169E1, 0x050040, "royal blue", 165, 175, 27, 34},
    {0x8A2BE2, 0x100040, "indigo    ", 176, 190, 21, 34},
    {0x800080, 0x200040, "violet    ", 191, 205, 57, 35},
    {0x800080, 0x200040, "purple    ", 206, 215, 12, 35},
    {0xFFC0CB, 0x400010, "pink      ", 216, 235, 164, 95},
    {0xFF00FF, 0x400020, "magenta   ", 236, 252, 198, 95},
    {0xFFFFFF, 0x404040, "white     ", 0, 0, 15, 97},    // Special case, no hue range
    {0x000000, 0x000000, "black     ", 0, 0, 0, 30},    // Special case, no hue range
    {0x808080, 0x202020, "grey      ", 0, 0, 8, 37}     // Special case, no hue range
  };




// Helper: get the index of the closest palette color for a given hue
int closestPaletteHueIdx(int hue) {
  // First, try to find a direct match using the hue ranges
  for (int i = 0; i < sizeof(colorNames) / sizeof(colorNames[0]); i++) {
    // Skip special cases (white, black, grey)
    if (colorNames[i].hueStart == 0 && colorNames[i].hueEnd == 0) {
      continue;
      }

    // Handle normal range
    if (colorNames[i].hueStart < colorNames[i].hueEnd) {
      if (hue >= colorNames[i].hueStart && hue <= colorNames[i].hueEnd) {
        return i;
        }
      }
    // Handle wrapping range (e.g., red spans 250-10)
    else if (colorNames[i].hueStart > colorNames[i].hueEnd) {
      if (hue >= colorNames[i].hueStart || hue <= colorNames[i].hueEnd) {
        return i;
        }
      }
    }

  // If no direct match found, use the closest hue distance
  int minDist = 256;
  int minIdx = 0;
  // Only consider non-special colors (skip white, black, grey)
  for (int i = 0; i < 14; i++) {
    // Find the center of the hue range for this color
    int centerHue;
    if (colorNames[i].hueStart < colorNames[i].hueEnd) {
      centerHue = (colorNames[i].hueStart + colorNames[i].hueEnd) / 2;
      } else {
      // Handle wrapping range (e.g., red spans 250-10)
      centerHue = (colorNames[i].hueStart + colorNames[i].hueEnd + 255) / 2;
      if (centerHue > 255) centerHue -= 255;
      }

    int dh = abs((int)hue - centerHue);
    if (dh > 127) dh = 255 - dh; // wrap around hue circle

    if (dh < minDist) {
      minDist = dh;
      minIdx = i;
      }
    }
  return minIdx;
  }
int colorToVT100(uint32_t color, int colorDepth) {


  rgbColor input = unpackRgb(color);

  hsvColor inputHsv = RgbToHsv(input);
  //inputHsv.v = 254;
  int hue = inputHsv.h;
  int hueIdx = closestPaletteHueIdx(hue);
  if (colorDepth == 256) {
    return colorNames[hueIdx].termColor256;
    } else {
    return colorNames[hueIdx].termColor16;
    }
  }

///@brief Convert a color to a name
///@param color Color to convert
///@param length Length of the name
///@return char* Name of the color
char* colorToName(uint32_t color, int length)
  {
  int numColors = sizeof(colorNames) / sizeof(colorNames[0]);
  rgbColor input = unpackRgb(color);



  // Only return black if the color is exactly 0x000000
  if (color == 0x000000) {
    const char* black = "black";
    strncpy(colorNameBuffer, black, strlen(black));
    colorNameBuffer[strlen(black)] = '\0';
    return colorNameBuffer;
    }
  // Return white if all channels are equal (and not zero)
  if (input.r == input.g && input.g == input.b && input.r != 0) {
    const char* white = "white";
    strncpy(colorNameBuffer, white, strlen(white));
    colorNameBuffer[strlen(white)] = '\0';
    return colorNameBuffer;
    }

  // Convert input to HSV
  hsvColor inputHsv = RgbToHsv(input);

  // For dim colors or colors that don't match range, use the existing approach
  // Determine if color is dim (low brightness)
  bool isDim = inputHsv.v < 70;
  // Serial.print("\n\r inputHsv.v: ");
  // Serial.print(inputHsv.v);
  // Serial.print("\n\r inputHsv.s: ");
  // Serial.print(inputHsv.s);
  // Serial.print("\n\r inputHsv.h: ");
  // Serial.print(inputHsv.h);
  // Serial.print("\n\r");


  int minDist = 0x7FFFFFFF;
  int minIdx = 0;

  // Check if hue directly falls within a defined range
  bool foundRange = false;
  for (int i = 0; i < numColors; i++) {
    // Skip special cases (white, black, grey) if we have color information
    if (colorNames[i].hueStart == 0 && colorNames[i].hueEnd == 0) {
      if (inputHsv.s > 40 && inputHsv.v > 30) continue;
      }

    // Handle normal range
    if (colorNames[i].hueStart < colorNames[i].hueEnd) {
      if (inputHsv.h >= colorNames[i].hueStart && inputHsv.h <= colorNames[i].hueEnd) {
        minIdx = i;
        foundRange = true;
        break;
        }
      }
    // Handle wrapping range (e.g., red spans 253-12)
    else if (colorNames[i].hueStart > colorNames[i].hueEnd) {
      if (inputHsv.h >= colorNames[i].hueStart || inputHsv.h <= colorNames[i].hueEnd) {
        minIdx = i;
        foundRange = true;
        break;
        }
      }
    }

  // If no range match was found, fall back to distance calculation
  if (!foundRange) {
    for (int i = 0; i < numColors; i++) {
      uint32_t refColor;
      if (isDim) {
        // Use dim reference colors for matching dim input colors
        refColor = colorNames[i].dimColor;
        } else {
        // For brighter colors, compare with standard palette
        refColor = colorNames[i].color;
        }

      // For very dim colors, we'll compare RGB directly 
      if (isDim) {
        rgbColor refRgb = unpackRgb(refColor);
        int dr = (int)input.r - (int)refRgb.r;
        int dg = (int)input.g - (int)refRgb.g;
        int db = (int)input.b - (int)refRgb.b;
        int dist = dr * dr + dg * dg + db * db;

        if (dist < minDist) {
          minDist = dist;
          minIdx = i;
          }
        } else {
        // Force brightness to max for matching to avoid brightness bias
        hsvColor compareHsv = inputHsv;
        compareHsv.v = 254;

        rgbColor refRgb = unpackRgb(refColor);
        hsvColor refHsv = RgbToHsv(refRgb);

        // Compare hue and saturation only
        int dh = (int)compareHsv.h - (int)refHsv.h;
        if (dh > 127) dh = 255 - dh; // wrap around hue circle
        if (dh < -127) dh = 255 + dh;

        int ds = (int)compareHsv.s - (int)refHsv.s;
        int dist = dh * dh + ds * ds;

        if (dist < minDist) {
          minDist = dist;
          minIdx = i;
          }
        }
      }
    }

  const char* src = colorNames[minIdx].name;
  // Serial.print("\n\r");
  // Serial.print(src);
  // Serial.print("\n\r");
  int len = strlen(src);
  if (length == -1) {
    // Trim trailing spaces only
    int end = len - 1;
    while (end >= 0 && src[end] == ' ') end--;
    int trimmedLen = end + 1;
    strncpy(colorNameBuffer, src, trimmedLen);
    colorNameBuffer[trimmedLen] = '\0';
    return colorNameBuffer;
    } else {
    int padLen = length > len ? length : len;
    memset(colorNameBuffer, ' ', padLen);
    strncpy(colorNameBuffer, src, padLen);
    colorNameBuffer[padLen] = '\0';
    return colorNameBuffer;
    }
  }

///@brief Convert a rgbColor to a name
///@param color Color to convert
///@param length Length of the name
///@return char* Name of the color
char* colorToName(rgbColor color, int length) {
  return colorToName(packRgb(color.r, color.g, color.b), length);
  }

///@brief Convert a hue to a name
///@param hue Hue to convert
///@param length Length of the name
///@return char* Name of the color
char* colorToName(int hue, int length) {
  // Serial.print("\n\n\rhueVersion: ");
  // Serial.print(hue);
  // Special case: black, white, grey
  hue = (hue) % 255;
  if (hue < 0) return colorToName(0x000000, length); // fallback

  // Find the color for this hue using direct range matching
  for (int i = 0; i < sizeof(colorNames) / sizeof(colorNames[0]); i++) {
    // Skip special cases (white, black, grey)
    if (colorNames[i].hueStart == 0 && colorNames[i].hueEnd == 0) {
      continue;
      }

    // Handle normal range
    if (colorNames[i].hueStart < colorNames[i].hueEnd) {
      if (hue >= colorNames[i].hueStart && hue <= colorNames[i].hueEnd) {
        const char* src = colorNames[i].name;
        int len = strlen(src);
        if (length == -1) {
          // Trim trailing spaces only
          int end = len - 1;
          while (end >= 0 && src[end] == ' ') end--;
          int trimmedLen = end + 1;
          strncpy(colorNameBuffer, src, trimmedLen);
          colorNameBuffer[trimmedLen] = '\0';
          return colorNameBuffer;
          } else {
          int padLen = length > len ? length : len;
          memset(colorNameBuffer, ' ', padLen);
          strncpy(colorNameBuffer, src, padLen);
          colorNameBuffer[padLen] = '\0';
          return colorNameBuffer;
          }
        }
      }
    // Handle wrapping range (e.g., red spans 250-10)
    else if (colorNames[i].hueStart > colorNames[i].hueEnd) {
      if (hue >= colorNames[i].hueStart || hue <= colorNames[i].hueEnd) {
        const char* src = colorNames[i].name;
        int len = strlen(src);
        if (length == -1) {
          // Trim trailing spaces only
          int end = len - 1;
          while (end >= 0 && src[end] == ' ') end--;
          int trimmedLen = end + 1;
          strncpy(colorNameBuffer, src, trimmedLen);
          colorNameBuffer[trimmedLen] = '\0';
          return colorNameBuffer;
          } else {
          int padLen = length > len ? length : len;
          memset(colorNameBuffer, ' ', padLen);
          strncpy(colorNameBuffer, src, padLen);
          colorNameBuffer[padLen] = '\0';
          return colorNameBuffer;
          }
        }
      }
    }

  // If we get here, use the closest match
  int idx = closestPaletteHueIdx(hue);
  const char* src = colorNames[idx].name;
  int len = strlen(src);
  if (length == -1) {
    int end = len - 1;
    while (end >= 0 && src[end] == ' ') end--;
    int trimmedLen = end + 1;
    strncpy(colorNameBuffer, src, trimmedLen);
    colorNameBuffer[trimmedLen] = '\0';
    return colorNameBuffer;
    } else {
    int padLen = length > len ? length : len;
    memset(colorNameBuffer, ' ', padLen);
    strncpy(colorNameBuffer, src, padLen);
    colorNameBuffer[padLen] = '\0';
    return colorNameBuffer;
    }
  }


// Declare global variables for scroll acceleration to avoid scope issues
static int scrollLastDirection = 0;  // Previous scrolling direction
static float scrollAccelerationFactor = 1.0f;  // Current acceleration multiplier
static unsigned long scrollAccelerationLastTime = 0;  // Last time we updated acceleration

uint32_t colorPicker(uint8_t startHue, uint8_t brightness) {
  // Static precomputed data for all 256 possible hue values (0-255)
  clearColorOverrides(1, 1, 0);

  unsigned long holdConfirmTime = 1500;

  static bool initialized = false;
  static char allNames[256][12];
  static uint32_t allColorValues[256];
  static bool nameToggleState[256];
  static char emptyStr[2] = " ";

  if (startHue < 0) {
    startHue = 225;
    } else if (startHue > 255) {
      startHue = 225;
      }
    int rangeStart = 0;
    int rangeEnd = 255;
    // One-time initialization of all possible hue data
    if (!initialized) {
      bool toggle = false;
      char* lastName = nullptr;

      // Precompute all color names, colors, and toggle states for all 256 hues
      for (int h = 0; h < 256; h++) {
        // Get color and name
        hsvColor hsv = { (uint8_t)h, 254, brightness };
        allColorValues[h] = HsvToRaw(hsv);

        char* tempName = colorToName(allColorValues[h], 10);
        strncpy(allNames[h], tempName, 11);
        allNames[h][11] = '\0';

        // Calculate toggle state based on name changes
        if (lastName == nullptr || strcmp(allNames[h], lastName) != 0) {
          toggle = !toggle;
          }
        nameToggleState[h] = toggle;
        lastName = allNames[h];
        }

      initialized = true;
      }

    // Check if there's input in Serial to parse a range
    if (Serial.available() > 0) {
      String input = Serial.readStringUntil('\n');
      input.trim();

      // Look for a dash character
      int dashIndex = input.indexOf('-');
      if (dashIndex > 0 && dashIndex < input.length() - 1) {
        // Get the numbers before and after the dash
        String startStr = input.substring(0, dashIndex);
        String endStr = input.substring(dashIndex + 1);

        // Convert to integers
        rangeStart = startStr.toInt();
        rangeEnd = endStr.toInt();

        // Validate the range
        if (rangeStart >= 0 && rangeStart <= 255 && rangeEnd >= 0 && rangeEnd <= 255) {
          Serial.print("Displaying colors for range: ");
          Serial.print(rangeStart);
          Serial.print("-");
          Serial.println(rangeEnd);
          } else {
          Serial.println("Invalid range. Using default range (0-255)");
          rangeStart = 0;
          rangeEnd = 255;
          }
        } else {
        // No dash found or invalid format
        Serial.println("Invalid format. Using default range (0-255)");
        }
      }

    b.clear();
    // int lastNetSlot = netSlot;

    // netSlot = 8;
    // createSlots(netSlot, 1);
  //!dont show nets
    hideNets = 1;
    showLEDsCore2 = -2;
    resetEncoderPosition = 1;

    // Store original range for restoring when zooming out
    static int originalRangeStart = -1;
    static int originalRangeEnd = -1;
    if (originalRangeStart == -1) {
      // Always use full spectrum 0-255 as the base range for consistency
      originalRangeStart = 0;
      originalRangeEnd = 255;
      }

    // Center position and display state

    int centerRow = 45; // Start at middle row (0-59)
    int zoom = 0;       // Initial zoom level (0-255)
    bool zoomOrScroll = false;
    int cursorRow = 0;

    // If startHue is specified, find the row closest to it
    if (startHue >= 0 && startHue <= 255) {
      // Formula to convert hue to row: row = (hue * 60.0f) / 255.0f
      // This is the inverse of the formula: hue = row * 255.0f / 60.0f
      centerRow = round((startHue * 60.0f) / 255.0f);

      // Ensure centerRow stays within valid bounds (0-59)
      if (centerRow < 0) centerRow = 0;
      if (centerRow > 59) centerRow = 59;
      }

    // Use floating-point for precise positioning to avoid integer rounding errors
    float preciseHue = centerRow * 255.0f / 60.0f;
    int centerHue = (int)preciseHue; // Initial hue at center row

    float zoomCursorWidthf = 1.0f;
    int zoomCursorWidth = 1;

    // Acceleration variables - using function local variables not static to avoid linter errors
    int lastDirection = 0;  // Previous scrolling direction
    float accelerationFactor = 1.0f;  // Current acceleration multiplier
    unsigned long lastAccelerationTime = 0;  // Last time we updated acceleration
    const float maxAcceleration = 6.0f;  // Maximum acceleration factor
    const float accelerationRate = 0.0003f;  // How quickly acceleration builds up
    const unsigned long accelerationTimeout = 10;  // Reset acceleration after this many ms of no movement
    int zoomWithProbe = 0;
    Serial.println(" row\t hue\t name\t\t row\t hue\t name\n\r");



    do {
      // Calculate total range and manage zoom
      int originalTotalRange = 255; // Use the full hue range (0-255)

      // Map from row number to hue value - this is the key function
      auto getHueForRow = [&](int row) -> int {
        // Calculate zoom factor - non-linear for better control at low zoom levels
        float zoomFactor = zoom * zoom / 100.0f;

        // Convert zoom factor to visible percentage of the color range
        float visiblePercentage = 1.0f - (zoomFactor / 255.0f);
        if (visiblePercentage < 0.01f) visiblePercentage = 0.01f; // Prevent division by zero

        // Current visible range based on zoom with floating point for smoother transitions
        float zoomedRange = (float)originalTotalRange * visiblePercentage;

        // Calculate relative row position from centerRow
        // Use floating point for the entire calculation to avoid rounding errors
        float rowFraction = (float)(row - centerRow) / 60.0f; // -0.5 to 0.5 based on distance from center

        // Calculate final hue value using floating point precision
        // Use preciseHue as the anchor to ensure exact center positioning
        float hueFloat = preciseHue + (rowFraction * zoomedRange);

        // Wrap around properly in the 0-255 range
        while (hueFloat < 0) hueFloat += 255.0f;
        while (hueFloat >= 255.0f) hueFloat -= 255.0f;

        // Convert to integer without additional rounding
        int hue = (int)hueFloat;

        // Wrapping already handled in floating point, no need for additional wrapping here

        return hue;

        };

      // Get hue ranges from the first and last rows for display
      int displayRangeStart = getHueForRow(0);
      int displayRangeEnd = getHueForRow(59);

      // Render all rows using precomputed data
      for (int row = 0; row < 60; row++) {
        int hue = getHueForRow(row);

        // Get precomputed data for this hue
        uint32_t color = allColorValues[hue];
        bool toggle = nameToggleState[hue];

        // Apply alternating row pattern
        if (toggle != nameToggleState[(row - 1) % 255]) {
          b.printRawRow(0b00001110, row, color, 0xffffff);
          } else {
          b.printRawRow(0b00001110, row, color, 0xffffff);
          b.printRawRow(0b00001110, row, color, 0xffffff);
          }

        // Highlight the center row
        if (zoomOrScroll || zoomWithProbe != 0) {
          // zoomCursorWidth = (zoom + 1 / 20)%60;
          if (row == centerRow) {
            b.printRawRow(0b00011111, row, scaleBrightness(color, 0), 0xffffff);

            logoColorOverride = scaleBrightness(color, 0);

            } else if (row < (centerRow + zoomCursorWidth) && row >(centerRow - zoomCursorWidth)) {
              b.printRawRow(0b00010001, row, scaleBrightness(color, 0), 0xffffff);
              b.printRawRow(0b00001110, row, color, 0xfffffe);
              }
            // else if (row == (centerRow+ zoomCursorWidth)) {
            // b.printRawRow(0b00011111, row, scaleBrightness(color, 200), 0xffffff);
            // } else if (row == (centerRow- zoomCursorWidth)) {
            // b.printRawRow(0b00011111, row, scaleBrightness(color, 200), 0xffffff);
            // }
          } else if (zoomOrScroll == false) {
            if (row == centerRow) {
              b.printRawRow(0b00011111, row, scaleBrightness(color, 300), 0xffffff);
              logoColorOverride = scaleBrightness(color, 0);
              }
            }
        }

      // Clear the previous output
      Serial.print("\033[J");

      // Display information for all rows
      for (int i = 0; i < 30; i++) {
        // Top rows (0-29)
        int topHue = getHueForRow(i);
        int bottomHue = getHueForRow(i + 30);

        // Print row marker for center
        if (i == centerRow && centerRow < 30) {
          Serial.print("> ");
          } else {
          Serial.print("  ");
          }

        // Print top row info
        Serial.print(i + 1);
        Serial.print(":\t ");
        Serial.print(topHue);
        Serial.print("\t");

        // if (!nameToggleState[topHue]) {
        //   Serial.print(" ");
        //   }
        Serial.print(allNames[topHue]);

        // Print bottom row marker for center
        if (i + 30 == centerRow) {
          Serial.print("\t> ");
          } else {
          Serial.print("\t  ");
          }

        // Print bottom row info
        //Serial.print("\t ");
        Serial.print(i + 31);
        Serial.print(":\t  ");
        Serial.print(bottomHue);
        Serial.print("\t");

        // if (!nameToggleState[bottomHue]) {
        //   Serial.print(" ");
        //   }
        Serial.print(allNames[bottomHue]);

        Serial.println();
        }

      // Print status information
      int displayCenterHue = getHueForRow(centerRow);
      Serial.print("Center: ");
      Serial.print(centerRow + 1);
      Serial.print(" (hue: ");
      Serial.print(centerHue); // Display the anchor hue value that stays fixed
      Serial.print(")");

      if (zoom >= 0) {
        // Calculate a smooth zoom percentage for display based on non-linear curve
        // Higher zoom values produce increasingly finer granularity
        float zoomPercentage = 100.0f * (1.0f - (zoom / 255.0f));

        // // Special case for very low zoom values to make initial zoom smoother
        // if (zoom < 10) {
        //   // Slower initial decrease for finest control at beginning
        //   zoomPercentage = 100.0f - (zoom * zoom / 25.0f);
        // }

        Serial.print(" | Zoom: ");
        Serial.print(zoom);
        Serial.print(" (");
        Serial.print((int)zoomPercentage);
        Serial.print("% range: ");

        // Show more helpful range display, especially for wraparound ranges
        if (zoom < 5) {
          Serial.print("FULL SPECTRUM");
          } else if (displayRangeStart > displayRangeEnd) {
            Serial.print(displayRangeStart);
            Serial.print(" - ");
            Serial.print(displayRangeEnd);
            } else {
            Serial.print(displayRangeStart);
            Serial.print("-");
            Serial.print(displayRangeEnd);
            }
          Serial.print(")");
        }

      Serial.print(" | Mode: ");
      Serial.print(zoomOrScroll ? "ZOOM" : "SCROLL");

      Serial.println();
      Serial.println();
      Serial.flush();

      // Handle encoder input for scrolling and zooming
      int oldEncoderPosition = encoderPosition;
      bool redraw = false;
      unsigned long lastChangeTime = millis();
      unsigned long lastMicroAdjust = millis();
      const int encoderSensitivity = 3; // Base sensitivity for scroll mode

      // Use acceleration variables defined at function scope

      while (!redraw) {



        // Poll encoder
        //rotaryEncoderStuff();
        int newPosition = encoderPosition;
        zoomWithProbe = 0;
        // Check for acceleration timeout
        if (millis() - scrollAccelerationLastTime > accelerationTimeout) {
          // Reset acceleration if user hasn't scrolled for a while
          scrollAccelerationFactor = 1.0f;
          scrollLastDirection = 0;
          }

        // Check for serial input to exit
        if (Serial.available() > 0) {
          redraw = true;
          break;
          }

        // Handle button press with debounce
        static unsigned long lastButtonPress = 0;
        static bool buttonWasPressed = false;  // Track if button was previously pressed
        static unsigned long buttonPressStartTime = 0; // Track when button was initially pressed
        static int probeButtonWasPressed = 2; // Track if probe button was previously pressed
        // Get current button state (LOW is pressed)
        bool buttonIsPressed = (digitalRead(BUTTON_ENC) == LOW);

        int probeButtonIsPressed = (checkProbeButton());




        // Detect initial button press
        if ((buttonIsPressed && !buttonWasPressed) || (probeButtonIsPressed > 0 && probeButtonWasPressed == 0)) {
          // Button was just pressed (transition from released to pressed)

          if (buttonIsPressed) {
            zoomOrScroll = !zoomOrScroll;
            }
          lastButtonPress = millis();

          // if (probeButtonIsPressed == 2) {
          //   // Serial.print("probe button pressed: ");
          //   // Serial.println(probeButtonIsPressed);
          //   }

          redraw = true;
          // showLEDsCore2 = 2;
          // waitCore2();
          probeButtonWasPressed = probeButtonIsPressed;
          buttonWasPressed = buttonIsPressed;
          buttonPressStartTime = millis(); // Start timing the press

          if (probeButtonIsPressed > 0)
            {
            if (probeButtonIsPressed == 2) {
              zoomWithProbe = -1;
              } else {
              zoomWithProbe = 1;
              }
            // Serial.print("zoomWithProbe: ");
            // Serial.println(zoomWithProbe);
            holdConfirmTime = 800;
            } else {
            holdConfirmTime = 1500;
            }

          //lastButtonPress = millis();
          while (buttonIsPressed || probeButtonIsPressed > 0) {
            uint32_t color = allColorValues[centerHue];
            uint32_t timerColor = 0x203050;

            buttonIsPressed = (digitalRead(BUTTON_ENC) == LOW);
            probeButtonIsPressed = (checkProbeButton());
            logoColorOverride = scaleBrightness(color, 100);




            if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 5 + 30) {

              ADCcolorOverride1 = scaleBrightness(timerColor, 100);

              } else if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 5) {

                ADCcolorOverride0 = scaleBrightness(timerColor, 100);

                } else if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 3 + 30) {

                  DACcolorOverride1 = scaleBrightness(timerColor, 100);

                  } else if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 3) {

                    DACcolorOverride0 = scaleBrightness(timerColor, 100);

                    } else if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 1 + 30) {

                      GPIOcolorOverride1 = scaleBrightness(timerColor, 100);

                      } else if (millis() - buttonPressStartTime > (holdConfirmTime / 6) * 1) {

                        GPIOcolorOverride0 = scaleBrightness(timerColor, 100);
                        }


                      if (millis() - buttonPressStartTime > holdConfirmTime) {
                        hideNets = 0;
                        showLEDsCore2 = -1;
                        clearColorOverrides(true, true, true);
                        blockProbeButton = 5000;
                        blockProbeButtonTimer = millis();
                        // blockProbing = 1000;
                        // blockProbingTimer = millis();
                        //delay(100);
                        return HsvToRaw(hsvColor{ (uint8_t)(centerHue % 255), 255, brightness });
                        break;
                        }

            }
          clearColorOverrides(false, true, true);


          //break;

          } else {

          if (probeButtonIsPressed == 1 && probeButtonWasPressed == 0) {
            zoomOrScroll = !zoomOrScroll;
            redraw = true;
            // break;
            }
          buttonWasPressed = buttonIsPressed;
          probeButtonWasPressed = probeButtonIsPressed;

          }






        int probeReading = justReadProbe(true);

        // if (probeReading != -1){
        //   // Serial.print("probeReading: ");
        //   // Serial.println(probeReading);
        // }

        if (probeReading != -1 && probeReading > 0 && probeReading <= 60) {
          // Calculate the row difference
          int rowDiff = probeReading - 1 - centerRow;
          //if (zoomOrScroll == 1){
          // Move the centerRow to the tapped position


          if (zoom > 0 && probeReading == 1)
            {
            centerRow = 59;
            } else if (zoom > 0 && probeReading == 60) {
              centerRow = 0;
              } else {
              centerRow = probeReading - 1;
              }

            // Calculate current visible range based on zoom level
            float visiblePercentage = 1.0f - ((zoom * zoom / 100.0f) / 255.0f);
            if (visiblePercentage < 0.01f) visiblePercentage = 0.01f;
            float visibleRange = originalTotalRange * visiblePercentage;

            // Calculate how much the hue needs to shift to keep colors in place
            float hueIncrement = (visibleRange / 60.0f) * rowDiff;

            // Adjust preciseHue to maintain color positions 
            preciseHue += hueIncrement;

            // Ensure proper wrapping of hue values
            while (preciseHue < 0) preciseHue += 255.0f;
            while (preciseHue >= 255.0f) preciseHue -= 255.0f;

            // Update centerHue to match the new preciseHue
            centerHue = (int)preciseHue;
            // } else {



            redraw = true;
            // Serial.print("Probe reading: ");
            // Serial.print(probeReading);

          //}
          }


        // Handle encoder rotation
        if (newPosition != oldEncoderPosition || zoomWithProbe != 0) {
          // Adjust encoder sensitivity based on mode
          int delta;
          if (zoomOrScroll || zoomWithProbe != 0) {
            // Higher sensitivity (smaller steps) for zoom mode
            // Get raw encoder change first
            if (zoomWithProbe != 0) {
              delta = zoomWithProbe * 4;
              } else {
              delta = (newPosition - oldEncoderPosition);
              }

            if (delta > 0 && zoomWithProbe == 0) {
              zoomCursorWidthf -= 0.28f;
              } else if (delta < 0 && zoomWithProbe == 0) {
                zoomCursorWidthf += 0.28f;
                } else if (zoomWithProbe != 0 && delta > 0) {
                  zoomCursorWidthf -= 1.1f;
                  } else if (zoomWithProbe != 0 && delta < 0) {
                    zoomCursorWidthf += 1.1f;
                    }

                  if (zoomCursorWidthf > 60) zoomCursorWidthf = 60;
                  if (zoomCursorWidthf < 2) zoomCursorWidthf = 2;

                  zoomCursorWidth = (int)zoomCursorWidthf % 60;

                  // Progressive zoom rate - faster at beginning, slower at higher zoom
                 // if (zoomWithProbe == 0) {
                  if (zoom >= 100) {
                    // Very slow changes at high zoom for precise control

                    if (zoomWithProbe == 0) {

                      delta = delta / 6;
                      } else {
                      if (delta < 0) {
                        delta = delta / 2;
                        } else {
                        delta = delta * 3;
                        }
                      }

                    if (delta == 0 && (newPosition != oldEncoderPosition)) {
                      delta = (newPosition > oldEncoderPosition) ? 1 : -1;
                      }


                    } else {

                    delta = delta * (((100 - zoom) / 10) + 1);

                    }
                  // } else if (zoom > 50) {
                  //   // Medium changes in the middle-high range
                  //   delta = delta * 4;
                  //   } else if (zoom > 25){
                  //     delta = delta * 6;
                  //   } else {
                  //   // Fast changes at lower zoom levels for quick adjustments
                  //   // Amplify delta for faster initial zooming
                  //   delta = delta * 10;
                  //   // Cap maximum change to prevent huge jumps
                  //   // if (delta > 15) delta = 15;
                  //   // if (delta < -15) delta = -15;
                  //   }
               // }
            } else if (zoomWithProbe == 0) {
              // Normal sensitivity for scroll mode
              int rawDelta = (newPosition - oldEncoderPosition);

              // Determine current scrolling direction
              int currentDirection = (rawDelta > 0) ? 1 : ((rawDelta < 0) ? -1 : 0);

              // Update acceleration based on direction consistency
              if (currentDirection != 0) {
                if (currentDirection == scrollLastDirection) {
                  // Same direction - increase acceleration
                  scrollAccelerationFactor += accelerationRate;
                  if (scrollAccelerationFactor > maxAcceleration) {
                    scrollAccelerationFactor = maxAcceleration;
                    }
                  } else {
                  // Direction changed - reset acceleration
                  scrollAccelerationFactor = 1.0f;
                  }

                // Remember current direction and time

                scrollAccelerationLastTime = millis();
                }
              scrollLastDirection = currentDirection;
              // Apply base sensitivity first
              delta = rawDelta / encoderSensitivity;

              // Then apply acceleration for larger movements

                // Apply non-linear acceleration that affects larger movements more
              float acceleratedDelta = (float)delta * scrollAccelerationFactor;

              // Round to integer and ensure minimum movement of 1
              delta = (int)acceleratedDelta;
              if (delta == 0 && rawDelta != 0) {
                delta = currentDirection;
                }

              }

            if (delta != 0) {

              if (zoomOrScroll || zoomWithProbe != 0) {
                // Zoom mode - much more granular for smoother zooming
                zoom -= delta;

                // Clamp zoom value
                if (zoom < 0) zoom = 0;
                if (zoom > 255) zoom = 255;

                // In zoom mode, we keep the center hue fixed
                // preciseHue stays unchanged, ensuring the center color remains stable

                redraw = true;
                } else if (zoomWithProbe == 0) {
                  // Scroll mode - move one row at a time
                  centerRow -= delta;
                  cursorRow -= delta;

                  if (cursorRow < 0) cursorRow = 59;
                  if (cursorRow > 59) cursorRow = 0;
                  // Handle wrapping
                  if (centerRow < 0) centerRow = 59;
                  if (centerRow > 59) centerRow = 0;

                  // Update preciseHue (and subsequently centerHue) when scrolling
                  // The exact increment depends on the current zoom level

                  // Calculate current visible range based on zoom level
                  float visiblePercentage = 1.0f - ((zoom * zoom / 100.0f) / 255.0f);
                  if (visiblePercentage < 0.01f) visiblePercentage = 0.01f;
                  float visibleRange = originalTotalRange * visiblePercentage;

                  // At full zoom out (zoom near 0), one row = 255/60 hue units
                  // As we zoom in, the amount each row represents becomes smaller
                  float hueIncrement = (visibleRange / 60.0f) * delta;

                  // Update precise hue value using floating point to avoid rounding errors
                  preciseHue -= hueIncrement;

                  // Wrap around the color wheel properly using floating point
                  while (preciseHue < 0) preciseHue += 255.0f;
                  while (preciseHue >= 255.0f) preciseHue -= 255.0f;

                  // Update integer centerHue for display purposes
                  centerHue = (int)preciseHue;

                  redraw = true;
                  }

                oldEncoderPosition = newPosition;
                lastChangeTime = millis();
              }
          }

        // // Adaptive delay to prevent CPU hogging while still being responsive
        // unsigned long now = millis();
        // if (now - lastChangeTime > 200) {
        //   // Long delay when idle
        //   delay(10);
        //   } else if (now - lastChangeTime > 50) {
        //     // Medium delay when recently active
        //     delay(5);
        //     } else {
        //     // Short delay when very active
        //     delayMicroseconds(500);
        //     }
        // }
        }
      // Move cursor up to overwrite previous output
      Serial.print("\033[32A");

      } while (Serial.available() == 0);

    showLEDsCore2 = -1;
    hideNets = 0;
    clearColorOverrides();

    return HsvToRaw(hsvColor{ (uint8_t)(centerHue % 255), 255, brightness });
  }

void clearColorOverrides(bool logo, bool pads, bool header) {
  if (logo) {
    logoColorOverride = -1;
    logoColorOverrideTop = -1;
    logoColorOverrideBottom = -1;
    }
  if (pads) {
    ADCcolorOverride0 = -1;
    ADCcolorOverride1 = -1;
    DACcolorOverride0 = -1;
    DACcolorOverride1 = -1;
    GPIOcolorOverride0 = -1;
    GPIOcolorOverride1 = -1;
    }
  if (header) {
    RST0colorOverride = -1;
    RST1colorOverride = -1;
    GNDTcolorOverride = -1;
    GNDBcolorOverride = -1;
    VINcolorOverride = -1;
    V3V3colorOverride = -1;
    V5VcolorOverride = -1;
    }
  }

void printColorName(int hue) {

  b.clear();
  for (int i = 0; i < 60; i++) {
    hue = i * 4.4;
    hsvColor hsv = { (uint8_t)hue, 254, 10 };
    rgbColor rgb = HsvToRgb(hsv);
    Serial.print(i + 1);
    Serial.print("\t");
    Serial.print(hue % 254);
    Serial.print("\t");
    char colorName[14];
    switch (hue % 255) {
      case 0 ... 11:
        sprintf(colorName, "red");
        break;
      case 12 ... 22:
        sprintf(colorName, "orange");
        break;
      case 23 ... 36:
        sprintf(colorName, "amber");
        break;
      case 37 ... 64:
        sprintf(colorName, "yellow");
        break;
      case 65 ... 79:
        sprintf(colorName, "chartreuse");
        break;
      case 80 ... 94:
        sprintf(colorName, "green");
        break;
      case 95 ... 122:
        sprintf(colorName, "aqua");
        break;
      case 123 ... 145:
        sprintf(colorName, "teal");
        break;
      case 146 ... 175:
        sprintf(colorName, "blue");
        break;
      case 176 ... 191:
        sprintf(colorName, "indigo");
        break;
      case 192 ... 210:
        sprintf(colorName, "purple");
        break;
      case 211 ... 223:
        sprintf(colorName, "violet");
        break;
      case 224 ... 235:
        sprintf(colorName, "pink");
        break;
      case 236 ... 255:
        sprintf(colorName, "magenta");
        break;
        // case 246 ... 255:
        //   sprintf(colorName, "white");
        //   break;

      default:
        sprintf(colorName, "grey");
        break;
      }
    Serial.println(colorName);
    if (i == 29) {
      Serial.println();
      }

    // Serial.print(hue/16);
    // Serial.print("\t");
    // Serial.println(colorNames[hue/16]);
    uint32_t color = packRgb(rgb.r, rgb.g, rgb.b);

    b.printRawRow(0b00011111, i, color, 0xffffff);
    }
  }

int saveRawColors(int slot) {

  // // if (savedLEDcolors[slot][LED_COUNT] == 0xFFFFFF) // put this to say it was
  // // already saved
  // // {
  // return 0;
  // // }

  // if (slot == -1) {
  //   slot = netSlot;
  // }

  // for (int i = 0; i < 300; i++) {
  //   if (i >= slotLEDpositions[0] && i <= slotLEDpositions[NUM_SLOTS - 1]) {
  //     // savedLEDcolors[slot][i] = slotSelectionColors[1];
  //     //  Serial.print(i);
  //     //  Serial.print("\t");

  //     continue;
  //   }
  //   savedLEDcolors[slot][i] = leds.getPixelColor(i);
  // }
  // savedLEDcolors[slot][LED_COUNT] = 0xAAAAAA;
  return 0;
  }

void refreshSavedColors(int slot) {
  // if (slot == -1) {
  //   for (int i = 0; i < NUM_SLOTS; i++) {
  //     savedLEDcolors[i][LED_COUNT] = 0x000000;
  //   }
  // } else {
  //   savedLEDcolors[slot][LED_COUNT] = 0x000000;
  // }
  }

void showSavedColors(int slot) {
  if (slot == -1) {
    slot = netSlot;
    }


  clearAllNTCC();
  openNodeFile(slot, 0);
  // printNodeFile(slot, 0);
  // clearLEDs();
  clearLEDsExceptRails();
  getNodesToConnect();
  bridgesToPaths();
  // leds.clear();
  clearLEDsExceptRails();
  checkChangedNetColors(-1);
  assignNetColors();

  // saveRawColors(slot);



  showLEDsCore2 = -1;
  // leds.show();
  }

void clearChangedNetColors(int saveToFile) {
  for (int i = 0; i < MAX_NETS; i++) {
    changedNetColors[i].net = 0;
    changedNetColors[i].color = 0x000000;
    changedNetColors[i].node1 = 0;
    changedNetColors[i].node2 = 0;
    }
  if (saveToFile == 1) {
    saveChangedNetColorsToFile(netSlot, 0);
    }
  }


int removeChangedNetColors(int node, int saveToFile) {
  int ret = 0;
  for (int i = 0; i < numberOfNets; i++) {
    if (changedNetColors[i].node1 == node || changedNetColors[i].node2 == node) {
      changedNetColors[i].net = 0;
      changedNetColors[i].color = 0x000000;
      changedNetColors[i].node1 = 0;
      changedNetColors[i].node2 = 0;
      ret = 1;
      }
    }
  if (saveToFile == 1 && ret == 1) {
    saveChangedNetColorsToFile(netSlot, 0);
    }
  return ret;
  }

int checkChangedNetColors(int netIndex) {
  bool nodeFound = false;
  int changedNetColorIndex = -1;
  int nodeNetIndex = -1;
  int loop = 0;

  int ret = 0;
  if (netIndex < 0) {
    loop = 1;
    }


  for (int i = 0; i < numberOfNets + 5; i++) {
    nodeFound = false;
    if (loop == 0) {
      i = netIndex;
      }


    for (int k = 0; k < MAX_NODES; k++) {
      if (net[i].nodes[k] <= 0) {
        break;
        }
      if (nodeFound == true) {
        break;
        }
      if (net[i].nodes[k] > 0) {

        for (int j = 5; j < numberOfNets; j++) {

          if (net[i].nodes[k] == changedNetColors[j].node1 && changedNetColors[j].node1 > 0) {
            // Serial.print("node1: ");
            // Serial.println(changedNetColors[j].node1);
            if (changedNetColors[j].node2 > 0) {
              // Serial.print("node2: ");
              // Serial.println(changedNetColors[j].node2);

              for (int l = 0; l < MAX_NODES; l++) {

                if (net[i].nodes[l] <= 0) {
                  break;
                  }

                if (net[i].nodes[l] > 0) {
                  if (net[i].nodes[l] == changedNetColors[j].node2 && changedNetColors[j].node2 > 0) {
                    nodeFound = true;
                    changedNetColorIndex = j;
                    nodeNetIndex = k;
                    if (changedNetColors[j].net != i) {
                      ret = 1;
                      }
                    changedNetColors[j].net = i;
                    break;
                    }
                  }
                }
              } else {
              nodeFound = true;
              changedNetColorIndex = j;
              nodeNetIndex = k;
              if (changedNetColors[j].net != i) {
                ret = 1;
                }
              changedNetColors[j].net = i;
              break;
              }

            }
          }
        }
      }



    if (nodeFound && changedNetColorIndex > 0) {

      //struct changedNetColor tempChangedNetColor = changedNetColors[i];

      if (changedNetColorIndex != i) {
        //changedNetColors[i] = changedNetColors[changedNetColorIndex];
        changedNetColors[i].net = i;
        changedNetColors[i].color = changedNetColors[changedNetColorIndex].color;
        changedNetColors[i].node1 = changedNetColors[changedNetColorIndex].node1;
        changedNetColors[i].node2 = changedNetColors[changedNetColorIndex].node2;
        //changedNetColors[i].net = i;
        changedNetColors[changedNetColorIndex].net = -1;
        changedNetColors[changedNetColorIndex].color = 0x000000;
        changedNetColors[changedNetColorIndex].node1 = 0;
        changedNetColors[changedNetColorIndex].node2 = 0;

        //     Serial.print("swapped changedNetColors[");
        //     Serial.print(changedNetColorIndex);
        //     Serial.print("] with changedNetColors[");
        //     Serial.print(i);
        //     Serial.println("]");


        // Serial.print("node found: ");
        // Serial.println(i);
        // Serial.print("changedNetColors[");
        // Serial.print(i);
        // Serial.print("].node1: ");
        // Serial.println(changedNetColors[i].node1);
        // Serial.print("net[");
        // Serial.print(i);
        // Serial.print("].node: ");
        // Serial.println(net[i].nodes[nodeNetIndex]);
        // Serial.print("changedNetColors[");
        //   Serial.print(i);
        // Serial.print("].net: ");
        // Serial.println(changedNetColors[i].net);
        // Serial.println();
        }


      net[i].color = unpackRgb(changedNetColors[i].color);
      netColors[i] = net[i].color;
      changedNetColors[i].net = i;


      // //nodeFound = true;
      //break;
      } else {
      changedNetColors[i].net = -1;
      changedNetColors[i].color = 0x000000;
      changedNetColors[i].node1 = 0;
      changedNetColors[i].node2 = 0;
      }
    if (loop == 0) {
      break;
      }


    }
  return ret;
  }


uint32_t railNetColors[3] = // dim
  {  0x000f04, 0x0f0202, 0x0f0202 };



void assignNetColors(int preview) {
  // numberOfNets = 60;


  uint16_t colorDistance = (254 / (numberOfShownNets));
  if (numberOfShownNets < 4) {
    colorDistance = (254 / (4));
    }

  /* rgbColor specialNetColors[8] =
       {0x000000,
        0x00FF80,
        0xFF4114,
        0xFF0040,
        0xFF7800,
        0xFF4078,
        0xFFC8C8,
        0xC8FFC8};
*/
// leds.setPixelColor(110, rawOtherColors[2]);
// logoFlash = 2;
// showLEDsCore2 = 1;
//  if (debugLEDs) {
// Serial.print("\n\rcolorDistance: ");
// Serial.print(colorDistance);
// Serial.print("\n\r");
// Serial.print("numberOfNets: ");
// Serial.println(numberOfNets);
// Serial.print("numberOfShownNets: ");
// Serial.println(numberOfShownNets);
// Serial.print("\n\rassigning net colors\n\r");
//   Serial.print("\n\rNet\t\tR\tG\tB\t\tH\tS\tV");
//  delay(1);
//  }

  for (int i = 1; i < 6; i++) {
    if (net[i].machine == true) {
      rgbColor specialNetRgb = unpackRgb(rawSpecialNetColors[i]);

      net[i].color = specialNetRgb;
      specialNetColors[i] = specialNetRgb;

      netColors[i] = specialNetRgb;
      // continue;
      } else {

      hsvColor netHsv = RgbToHsv(specialNetColors[i]);

      if (i >= 1 && i <= 3) {
        netHsv.v = LEDbrightnessRail;
        } else if (i >= 4 && i <= 5) {
          netHsv.v = LEDbrightnessSpecial;
          }

        uint32_t railColor;

        switch (i) {
          case 1:
            railColor = 0x000f05;
            // if (brightenedRail == 1 || brightenedRail == 3) {
            //   rgbColor railRgb = unpackRgb(railColor);
            //   hsvColor railHsv = RgbToHsv(railRgb);
            //   railHsv.v += brightenedAmount;
            //   railRgb = HsvToRgb(railHsv);
            //   railColor = packRgb(railRgb.r, railRgb.g, railRgb.b);
            //   }

            // netColors[i] = unpackRgb(railColor);
            // net[i].color = netColors[i];
            netColors[i] = unpackRgb(railNetColors[0]);
            net[i].color = netColors[i];
            specialNetColors[i] = netColors[i];
            break;
          case 2:
            // railColor = logoColors8vSelect[map((long)(railVoltage[0] * 10), -80, 80,
            //                                    0, 59)];
            // netColors[i] = unpackRgb(railColor);
            // net[i].color = netColors[i];
            netColors[i] = unpackRgb(railNetColors[1]);
            net[i].color = netColors[i];
            specialNetColors[i] = netColors[i];
            // Serial.print("railVoltage[0]: ");
            // Serial.println(railVoltage[0]);
            // Serial.print("map: ");
            // Serial.println(map((int)(railVoltage[0]*10), -80, 80, 0, 59));
            // Serial.print("hue: ");
            // Serial.println(netHsv.h);
            break;
          case 3:
            // railColor = logoColors8vSelect[map((long)(railVoltage[1] * 10), -80, 80,
            //                                    0, 59)];
            // netColors[i] = unpackRgb(railColor);
            // net[i].color = netColors[i];
            netColors[i] = unpackRgb(railNetColors[2]);
            net[i].color = netColors[i];
            specialNetColors[i] = netColors[i];
            break;
          case 4:
            railColor =
              logoColors8vSelect[map((long)(dacOutput[0] * 10), -80, 80, 0, 59)];
            netColors[i] = unpackRgb(railColor);
            net[i].color = netColors[i];
            specialNetColors[i] = netColors[i];
            break;
          case 5:
            railColor =
              logoColors8vSelect[map((long)(dacOutput[1] * 10), -80, 80, 0, 59)];
            netColors[i] = unpackRgb(railColor);
            net[i].color = netColors[i];
            specialNetColors[i] = netColors[i];
            break;
          case 6:
           // netHsv.h = 240;
            break;
          case 7:
            // netHsv.h = 300;
            break;
          }

        // rgbColor netRgb = HsvToRgb(netHsv);

        // specialNetColors[i] = netRgb;
        // Serial.print("\n\r");
        // Serial.print(i);
        // Serial.print("\t");
        // Serial.print(netRgb.r, HEX);
        // Serial.print("\t");
        // Serial.print(netRgb.g, HEX);
        // Serial.print("\t");
        // Serial.print(netRgb.b, HEX);

        // netColors[i] = specialNetColors[i];
        // net[i].color = netColors[i];
      }

    // if (debugLEDs) {
    //   Serial.print("\n\r");
    //   int netLength = Serial.print(net[i].name);
    //   if (netLength < 8) {
    //     Serial.print("\t");
    //   }
    //   Serial.print("\t");
    //   Serial.print(net[i].color.r, HEX);
    //   Serial.print("\t");
    //   Serial.print(net[i].color.g, HEX);
    //   Serial.print("\t");
    //   Serial.print(net[i].color.b, HEX);
    //   Serial.print("\t\t");
    //   // Serial.print(netHsv.h);
    //   Serial.print("\t");
    //   // Serial.print(netHsv.s);
    //   Serial.print("\t");
    //   // Serial.print(netHsv.v);
    //   delay(10);
    // }
    //
    }

  int skipSpecialColors = 0;
  uint8_t hue = 1;

  int colorSlots[60] = { -1 };

  int colorSlots1[20] = { -1 };
  int colorSlots2[20] = { -1 };
  int colorSlots3[20] = { -1 };
  // Serial.print("number of nets: ");
  // Serial.println(numberOfNets);
  // Serial.print("number of shown nets: ");
  // Serial.println(numberOfShownNets);
  if (numberOfNets < 60 && numberOfShownNets > 0) {
    for (int i = 0; i <= numberOfShownNets; i++) {

      colorSlots[i] = abs(224 - ((((i)*colorDistance)))) % 254;
      }

    int backIndex = numberOfShownNets;

    int index1 = 0;
    int index2 = 0;
    int index3 = 0;
    //   int third = (numberOfNets - 8) / 3;

    for (int i = 0; i <= (numberOfShownNets); i++) {
      // Serial.print(colorSlots[i]);
      // Serial.print(" ");
      switch (i % 3) {
        case 0:
          colorSlots1[index1] = colorSlots[i];
          index1++;
          break;
        case 1:
          colorSlots2[index2] = colorSlots[i];
          index2++;
          break;
        case 2:
          colorSlots3[index3] = colorSlots[i];
          index3++;
          // backIndex--;
          break;
        }
      }
    //  if (debugLEDs) {
    // Serial.print("\n\n\rnumber of shown nets: ");
    // Serial.println(numberOfShownNets);
    // Serial.print("colorDistance: ");
    // Serial.println(colorDistance);
    // for (int i = 0; i < index1; i++) {
    //   Serial.print(colorSlots1[i]);
    //   Serial.print(" ");
    // }
    // Serial.println();
    // for (int i = 0; i < index2; i++) {
    //   // colorSlots2[i] = colorSlots2[(i + third) % index2];
    //   Serial.print(colorSlots2[i]);
    //   Serial.print(" ");
    // }

    // Serial.println();
    // for (int i = 0; i < index3; i++) {
    //   Serial.print(colorSlots3[i]);
    //   Serial.print(" ");
    // }
    // Serial.println("\n\r");
    //  }
    //   for (int i = 0; i < index1; i++) {
    //     colorSlots[i] = colorSlots1[(i)%index1];
    //   }
    //   for (int i = 0; i < index2; i++) {
    //     colorSlots[i + index1] = colorSlots2[(i+third)%index2];
    //   }
    //   for (int i = 0; i < index3; i++) {
    //     colorSlots[i + index1 + index2] = colorSlots3[(i+third*2)%index3];
    //   }
    int loop1index = 0;
    int loop2index = 0;
    int loop3index = 0;

    if (netColorMode == 0) {
      loop1index = 0;
      loop2index = 0;
      loop3index = 0;
      }
    if (netColorMode == 1) {
      loop1index = 0;
      loop2index = index2 / 2;
      loop3index = index3 - 1;
      }
    // Serial.print("netColorMode: ");
    // Serial.println(netColorMode);

    // Serial.print("loopInecies: ");
    // Serial.println(index1 + index2 + index3);

    for (int i = 0; i < index1 + index2 + index3; i++) {
      switch (i % 3) {
        case 0:
          colorSlots[i] = colorSlots1[loop1index];
          loop1index++;
          loop1index = loop1index % index1;
          break;
        case 1:

          colorSlots[i] = colorSlots2[loop2index];
          loop2index++;
          loop2index = loop2index % index2;
          break;
        case 2:

          colorSlots[i] = colorSlots3[loop3index];
          loop3index++;
          loop3index = loop3index % index3;
          // loop3index = loop3index % index3;
          // backIndex--;
          break;
        }
      }
    }
  //  if (debugLEDs) {
  // for (int i = 0; i < numberOfShownNets; i++) {

  //   Serial.print(colorSlots[i]);
  //   Serial.print(" ");
  // }
  // Serial.println("\n\n\n\r");
  // }
  // for (int i = 0; i < numberOfNets; i++) {
  //   Serial.println(colorSlots[i]);
  // }
  // Serial.println();
  // Serial.println();
  // int lastColor = numberOfNets - 8;
  // for (int i=0; i<(numberOfNets-8)/2; i++){
  //   int tempColor = colorSlots[i];
  //   colorSlots[i] = colorSlots[lastColor];
  //   colorSlots[lastColor] = tempColor;
  //   lastColor--;

  // }
  // for (int i = 0; i < numberOfShownNets; i++) {
  //   Serial.print("colorSlots[");
  //   Serial.print(i);
  //   Serial.print("]: ");
  //   Serial.println(colorSlots[i]);
  // }
  int frontIndex = 0;
  for (int i = 6; i <= numberOfNets; i++) {
    if (net[i].visible == 0) {
      // Serial.print("net ");
      // Serial.print(i);
      // Serial.println(" is not visible");

      continue;
      }

    int showingReading = 0;
    //bool manuallyChanged = false;

    if (preview == 0) {



      if (changedNetColors[i].net == i) {
        net[i].color = unpackRgb(changedNetColors[i].color);
        netColors[i] = net[i].color;
        continue;
        //break;
        }




      for (int a = 0; a < 8; a++) {
        if (i == showADCreadings[a]) {
          // netColors[i] = unpackRgb(rawOtherColors[8]);
          net[i].color = unpackRgb(adcReadingColors[a]);
          netColors[i] = net[i].color;
          showingReading = 1;
          // Serial.print("showing reading: ");
          // Serial.println(i);
          break;
          }
        }
      for (int a = 0; a < 10; a++) {
        if (i == gpioNet[a]) {
          net[i].color = unpackRgb(gpioReadingColors[a]);
          netColors[i] = net[i].color;
          // Serial.print("showing gpio: ");
          // Serial.println(i);
          // Serial.print("gpioReadingColors[");
          // Serial.print(a);
          // Serial.print("]: ");
          // Serial.println(gpioReadingColors[a], HEX);

          showingReading = 1;
          break;
          }
        }
      }
    if (showingReading == 0 || preview != 0) {

      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;

      // int foundColor = 0;
      // Serial.print("\n\ri: ");
      // Serial.println(i);
      // Serial.print("frontIndex: ");
      // Serial.println(frontIndex);

      hue = colorSlots[frontIndex];
      // Serial.print("hue: ");
      // Serial.println(hue);
      frontIndex++;

      hsvColor netHsv = { hue, 255, LEDbrightness };

      //This was the old way, directly using index, prone to errors if nets are removed/added
      // if (changedNetColors[i].uniqueID == net[i].uniqueID) {
      //   hsvColor changedNetHsv = RgbToHsv(unpackRgb(changedNetColors[i].color));
      //   netHsv = changedNetHsv;
      // } else {
      //   netHsv = { hue, 254, LEDbrightness };
      // }


      if (brightenedNet != 0 && i == brightenedNet) {
        netHsv.v += brightenedAmount;
        }

      // if (warningNet != 0 && i == warningNet) {
      //   netHsv.h = netHsv.h /10;
      //   }
      // netHsv.v = 200;


      net[i].color = HsvToRgb(netHsv);
      netColors[i] = net[i].color;

      //  netColors[i] = net[i].color;

        // leds.setPixelColor(i, netColors[i]);

        // net[i].color.r = netColors[i].r;
        // net[i].color.g = netColors[i].g;
        // net[i].color.b = netColors[i].b;
        // if (debugLEDs) {
        //   Serial.print("\n\r");
        //   Serial.print(net[i].name);
        //   Serial.print("\t\t");
        //   Serial.print(net[i].color.r, DEC);
        //   Serial.print("\t");
        //   Serial.print(net[i].color.g, DEC);
        //   Serial.print("\t");
        //   Serial.print(net[i].color.b, DEC);
        //   Serial.print("\t\t");
        //   Serial.print(hue);
        //   Serial.print("\t");
        //   Serial.print(saturation);
        //   Serial.print("\t");
        //   Serial.print(LEDbrightness);
        //   delay(3);
        // }
      }
    }
  // listSpecialNets();
  // listNets();
  // logoFlash = 0;
  }



void lightUpNet(int netNumber, int node, int onOff, int brightness2,
                int hueShift, int dontClear, uint32_t forceColor) {
  uint32_t color;
  int pcbExtinction = 0;
  int colorCorrection = 0;
  int pcbHueShift = 0;
  // Serial.print("netNumber: ");
  // Serial.println(netNumber);

  //     Serial.print(" node: ");
  //     Serial.print(node);
  //     Serial.print(" onOff: ");
  //     Serial.print(onOff);
  // return;


  if (netNumber <= 0 || netNumber >= MAX_NETS) {
    return;
    }
  if (net[netNumber].nodes[1] != 0 &&
      net[netNumber].nodes[1] <= 141) { // NANO_A7) {

    for (int j = 0; j < MAX_NODES; j++) {
      if (net[netNumber].nodes[j] <= 0) {
        break;
        }

      if (net[netNumber].machine == true) {
        Serial.println("machine");
        if (net[netNumber].nodes[j] == node || node == -1) {
          if (onOff == 1) {
            if (nodesToPixelMap[net[netNumber].nodes[j]] > 0) {
              leds.setPixelColor(
                  (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 0,
                  scaleDownBrightness(net[netNumber].rawColor));
              leds.setPixelColor(
                  (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 1,
                  scaleDownBrightness(net[netNumber].rawColor));
              leds.setPixelColor(
                  (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 2,
                  scaleDownBrightness(net[netNumber].rawColor));
              leds.setPixelColor(
                  (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 3,
                  scaleDownBrightness(net[netNumber].rawColor));
              leds.setPixelColor(
                  (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 4,
                  scaleDownBrightness(net[netNumber].rawColor));

              if (debugLEDs) {
                Serial.print("net: ");
                Serial.print(netNumber);
                Serial.print(" node: ");
                Serial.print(net[netNumber].nodes[j]);
                Serial.print(" mapped to LED:");
                Serial.println(nodesToPixelMap[net[netNumber].nodes[j]]);

                Serial.print("rawColor: ");
                Serial.println(net[netNumber].rawColor, HEX);
                }
              }
            }
          }
        } else {

        if (net[netNumber].nodes[j] <= NANO_A7) {

          if (net[netNumber].nodes[j] == node || node == -1) {
            if (onOff == 1) {

              pcbExtinction = 0;
              colorCorrection = 0;
              pcbHueShift = 0;

              if (net[netNumber].nodes[j] >= NANO_D0 &&
                  net[netNumber].nodes[j] <= NANO_A7) {
                // pcbExtinction = PCBEXTINCTION;

                // Serial.println (brightness2);
                // hueShift += PCBHUESHIFT;
                colorCorrection = 1;
                }
              // pcbExtinction += (brightness2-DEFAULTBRIGHTNESS);

              struct rgbColor colorToShift = { net[netNumber].color.r,
                                              net[netNumber].color.g,
                                              net[netNumber].color.b };

              if (forceColor != 0xffffff) {
                // Serial.println("force color");
                colorToShift = unpackRgb(forceColor);
                }

              struct rgbColor shiftedColor =
                shiftHue(colorToShift, hueShift, pcbExtinction, 254);

              if (colorCorrection != 0) {
                // shiftedColor = pcbColorCorrect(shiftedColor);
                //Serial.println("color correction"); 
                uint32_t correctedColor =
                  packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);

                shiftedColor = unpackRgb(scaleBrightness(correctedColor, 100));
                }

              hsvColor shiftedColorHsv = RgbToHsv(shiftedColor);

              if (net[netNumber].specialFunction >= 100 &&
                  net[netNumber].specialFunction <= 105) {
                // Serial.println("rail color");
                if (brightness2 != DEFAULTBRIGHTNESS) {
                  shiftedColorHsv.v = brightness2;
                  } else {
                  shiftedColorHsv.v = LEDbrightnessRail;
                  }
                if (brightenedNet == netNumber) {
                  shiftedColorHsv.v += brightenedAmount;
                  }
                shiftedColor = HsvToRgb(shiftedColorHsv);

                color = packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);
                // color = packRgb((shiftedColor.r * LEDbrightnessRail) >> 8,
                // (shiftedColor.g * LEDbrightnessRail) >> 8, (shiftedColor.b
                // * LEDbrightnessRail) >> 8);
                //  Serial.print("rail color: ");
                //  Serial.print(color, HEX);
                } else if (net[netNumber].specialFunction >= 100 &&
                           net[netNumber].specialFunction <= 120) {
                  // Serial.println("special function");
                  if (brightness2 != DEFAULTBRIGHTNESS) {
                    shiftedColorHsv.v = brightness2;
                    } else {
                    shiftedColorHsv.v = LEDbrightnessSpecial;
                    }
                  if (brightenedNet == netNumber) {
                    shiftedColorHsv.v += brightenedAmount;
                    // shiftedColorHsv.v *= 8;
                    shiftedColorHsv.v =
                      constrain(shiftedColorHsv.v, 0, 255);
                    }
                  shiftedColor = HsvToRgb(shiftedColorHsv);

                  color = packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);
                  // color = packRgb((shiftedColor.r * LEDbrightnessSpecial) >>
                  // 8, (shiftedColor.g * LEDbrightnessSpecial) >> 8,
                  // (shiftedColor.b
                  // * LEDbrightnessSpecial) >> 8);
                  } else {
                  //Serial.println("normal color");
                  // Serial.print("shiftedColorHsv.v: ");
                  // Serial.println(shiftedColorHsv.v);
                  if (brightness2 != DEFAULTBRIGHTNESS) {
                    // shiftedColorHsv.v = brightness2; //maybe put an averaging thing here
                    } else {
                    // shiftedColorHsv.v = LEDbrightness;
                    }
                  if (brightenedNet == netNumber) {
                    shiftedColorHsv.v += brightenedAmount;
                    }
                  shiftedColor = HsvToRgb(shiftedColorHsv);

                  color = packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);

                  // color = packRgb((shiftedColor.r * LEDbrightness) >> 8,
                  // (shiftedColor.g * LEDbrightness) >> 8, (shiftedColor.b *
                  // LEDbrightness) >> 8);
                  }
                int allOnTop = 1;
                if (net[netNumber].nodes[j] >= NANO_D0) {
                  rgbColor colorToShift = unpackRgb(color);
                  // colorToShift = shiftHue(colorToShift, hueShift);
                  hsvColor brighterColor = RgbToHsv(colorToShift);
                  if (brightenedNet == netNumber) {
                    // shiftedColorHsv.v += brightenedAmount;
                    shiftedColorHsv.v = 255;
                    brighterColor.v = 255;
                    // brighterColor.v =
                    //     constrain(shiftedColorHsv.v, 0, 255);
                    // shiftedColorHsv.v =
                    //     constrain(shiftedColorHsv.v, 0, 255);
                    }
                  brighterColor.v += PCBEXTINCTION;
                  rgbColor bright = HsvToRgb(brighterColor);

                  color = packRgb(bright.r, bright.g, bright.b);

                  for (int k = 0; k < MAX_NODES; k++) {
                    if (net[netNumber].nodes[k] < NANO_D0 &&
                        net[netNumber].nodes[k] > 0) {
                      allOnTop = 0;
                      break;
                      }
                    }
                  }
                if (allOnTop == 1) {
                  netColors[netNumber] = unpackRgb(color);
                  }
                // net[netNumber].rawColor = color;
                // net[netNumber].color = unpackRgb(color);

                if (probeHighlight - 1 != (nodesToPixelMap[net[netNumber].nodes[j]])) {
                  // Serial.print("nodesToPixelMap[net[netNumber].nodes[j]] = ");
                  // Serial.println(nodesToPixelMap[net[netNumber].nodes[j]]);
                  if (net[netNumber].nodes[j] >= NANO_D0) {
                    if (brightenedNet == netNumber) {
                      
                      if (brightenedNode == net[netNumber].nodes[j]) {
                        color = scaleBrightness(color, brightenedNodeAmount);
                        } else {
                          color = scaleBrightness(color, brightenedNetAmount);
                        }
                      } else {
                      color = scaleBrightness(color, 0);
                      }
                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) + 320, color);

                    } else {

                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 0,
                        color);
                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 1,
                        color);
                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 2,
                        color);
                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 3,
                        color);
                    leds.setPixelColor(
                        (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 4,
                        color);

                    // if (logoTopSetting[
                    }
                  } else {

                  }


              } else {

              }
            }
          }
        }
      }
    // turnOffSkippedNodes();
    /*                                                            Serial.print("color:
       "); Serial.print(color,HEX); Serial.print(" r: ");
                                                        Serial.print(shiftedColor.r);
                                                        Serial.print(" g: ");
                                                        Serial.print(shiftedColor.g);
                                                        Serial.print(" b: ");
                                                        Serial.print(shiftedColor.b);
                                                        Serial.print("
       hueShift:
       "); Serial.print(hueShift); Serial.print(" pcbExtinction: ");
                                                        Serial.print(pcbExtinction);
                                                        Serial.print("
       brightness2: "); Serial.println(brightness2);*/
    }
  // showRowAnimation(-1, GND);
  //  showLEDsCore2 = 1;
  showSkippedNodes();
  }
unsigned long lastSkippedNodesTime = 0;
int toggleSkippedNodes = 0;
void showSkippedNodes(uint32_t onColor, uint32_t offColor) {
  // return;
  // onColor = 0x1f1f2f;
  // uint32_t offColor = 0x0f000f;

  static int colorCycleOn = 0;
  colorCycleOn++;
  if (colorCycleOn > 254) {
    colorCycleOn = 0;
    }
  static int colorCycleOff = 200;
  colorCycleOff--;
  if (colorCycleOff < 0) {
    colorCycleOff = 254;
    }

  rgbColor onColorRgb = unpackRgb(onColor);
  rgbColor offColorRgb = unpackRgb(offColor);
  hsvColor onColorHsv = RgbToHsv(onColorRgb);
  hsvColor offColorHsv = RgbToHsv(offColorRgb);
  onColorHsv.h = (onColorHsv.h + colorCycleOn) % 254;
  offColorHsv.h = (offColorHsv.h + colorCycleOff) % 254;
  onColorRgb = HsvToRgb(onColorHsv);
  offColorRgb = HsvToRgb(offColorHsv);
  onColor = packRgb(onColorRgb.r, onColorRgb.g, onColorRgb.b);
  offColor = packRgb(offColorRgb.r, offColorRgb.g, offColorRgb.b);

  if (millis() - lastSkippedNodesTime > (1111)) {
    // Serial.println("skipped nodes");

    toggleSkippedNodes = !toggleSkippedNodes;

    lastSkippedNodesTime = millis();

    } else {
    // onColor = 0x1f1f2f;
    // onColor = 0;
    // onColor = 0x1f1f2f;
    // onColor = 0;
    // onColor = 0x1f1f2f;
    }

  for (int i = 0; i < numberOfPaths; i++) {

    if (path[i].skip == true) {
      // colorCycleOff = (colorCycleOff) % 254;
      // colorCycleOn = (colorCycleOn + (numberOfUnconnectablePaths)) % 254;
      if (path[i].node1 > 0 && path[i].node1 <= 60) {

        if ((toggleSkippedNodes == 1 && path[i].node1 % 2 == 0) || (toggleSkippedNodes == 0 && path[i].node1 % 2 == 1)) {

          leds.setPixelColor((path[i].node1 - 1) * 5 + 0, onColor);

          // leds.setPixelColor((path[i].node1 - 1) * 5 + 0, offColor);

          // leds.setPixelColor((path[i].node1 - 1) * 5 + 1, onColor);

          leds.setPixelColor((path[i].node1 - 1) * 5 + 1, offColor);

          leds.setPixelColor((path[i].node1 - 1) * 5 + 2, onColor);

          // leds.setPixelColor((path[i].node1 - 1) * 5 + 2, offColor);

          // leds.setPixelColor((path[i].node1 - 1) * 5 + 3, onColor);

          leds.setPixelColor((path[i].node1 - 1) * 5 + 3, offColor);

          leds.setPixelColor((path[i].node1 - 1) * 5 + 4, onColor);

          // leds.setPixelColor((path[i].node1 - 1) * 5 + 4, offColor);
          //toggleSkippedNodes = !toggleSkippedNodes;

          } else if ((toggleSkippedNodes == 0 && path[i].node1 % 2 == 0) || (toggleSkippedNodes == 1 && path[i].node1 % 2 == 1)) {

            // leds.setPixelColor((path[i].node1 - 1) * 5 + 0, onColor);

            leds.setPixelColor((path[i].node1 - 1) * 5 + 0, offColor);

            leds.setPixelColor((path[i].node1 - 1) * 5 + 1, onColor);

            // leds.setPixelColor((path[i].node1 - 1) * 5 + 1, offColor);

            // leds.setPixelColor((path[i].node1 - 1) * 5 + 2, onColor);

            leds.setPixelColor((path[i].node1 - 1) * 5 + 2, offColor);

            leds.setPixelColor((path[i].node1 - 1) * 5 + 3, onColor);

            // leds.setPixelColor((path[i].node1 - 1) * 5 + 3, offColor);

            // leds.setPixelColor((path[i].node1 - 1) * 5 + 4, onColor);

            leds.setPixelColor((path[i].node1 - 1) * 5 + 4, offColor);
            // toggleSkippedNodes = !toggleSkippedNodes;
             //}
            }

        } else if (path[i].node1 >= NANO_D0 && path[i].node1 <= NANO_5V) {
          hsvColor onColorHsv = RgbToHsv(onColorRgb);
          hsvColor offColorHsv = RgbToHsv(offColorRgb);
          onColorHsv.h = (onColorHsv.h + colorCycleOn + 40) % 254;
          offColorHsv.h = (offColorHsv.h + colorCycleOff + 40) % 254;
          onColorHsv.v += 90;
          offColorHsv.v += 40;
          onColorHsv.s = 80;
          offColorHsv.s = 120;
          onColorRgb = HsvToRgb(onColorHsv);
          offColorRgb = HsvToRgb(offColorHsv);
          uint32_t onColorHeader = packRgb(onColorRgb.r / 3, onColorRgb.g / 2, onColorRgb.b);
          uint32_t offColorHeader = packRgb(offColorRgb.r / 3, offColorRgb.g / 2, offColorRgb.b);
          if (toggleSkippedNodes == 1) {
            for (int j = 0; j < 35; j++) {
              if (bbPixelToNodesMapV5[j][0] == path[i].node1) {
                leds.setPixelColor(bbPixelToNodesMapV5[j][1], onColorHeader);
                }
              }
            } else {
            for (int j = 0; j < 35; j++) {
              if (bbPixelToNodesMapV5[j][0] == path[i].node1) {
                leds.setPixelColor(bbPixelToNodesMapV5[j][1], offColorHeader);
                }
              }
            }
          }

        if (path[i].node2 > 0 && path[i].node2 <= 60) {
          if (toggleSkippedNodes == 0) {

            leds.setPixelColor((path[i].node2 - 1) * 5 + 0, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 0, offColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 1, onColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 1, offColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 2, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 2, offColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 3, onColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 3, offColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 4, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 4, offColor);

            } else {

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 0, onColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 0, offColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 1, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 1, offColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 2, onColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 2, offColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 3, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 3, offColor);

            leds.setPixelColor((path[i].node2 - 1) * 5 + 4, onColor);

            // leds.setPixelColor((path[i].node2 - 1) * 5 + 4, offColor);
            //}
            }

          } else if (path[i].node1 >= NANO_D0 && path[i].node2 <= NANO_5V) {
            hsvColor onColorHsv = RgbToHsv(onColorRgb);
            hsvColor offColorHsv = RgbToHsv(offColorRgb);
            onColorHsv.h = (onColorHsv.h + colorCycleOn + 40) % 254;
            offColorHsv.h = (offColorHsv.h + colorCycleOff + 40) % 254;
            onColorHsv.v += 90;
            offColorHsv.v += 40;
            onColorHsv.s = 80;
            offColorHsv.s = 120;
            onColorRgb = HsvToRgb(onColorHsv);
            offColorRgb = HsvToRgb(offColorHsv);
            uint32_t onColorHeader = packRgb(onColorRgb.r / 3, onColorRgb.g / 2, onColorRgb.b);
            uint32_t offColorHeader = packRgb(offColorRgb.r / 3, offColorRgb.g / 2, offColorRgb.b);
            if (toggleSkippedNodes == 0) {
              for (int j = 0; j < 35; j++) {
                if (bbPixelToNodesMapV5[j][0] == path[i].node2) {
                  leds.setPixelColor(bbPixelToNodesMapV5[j][1], onColorHeader);
                  }
                }
              } else {
              for (int j = 0; j < 35; j++) {
                if (bbPixelToNodesMapV5[j][0] == path[i].node2) {
                  leds.setPixelColor(bbPixelToNodesMapV5[j][1], offColorHeader);
                  }
                }
              }
            }

          // leds.show();
      }
    }
  }

uint32_t scaleBrightness(uint32_t hexColor, int scaleFactor) {

  if (scaleFactor == 0) {
    return hexColor;
    }
  float scaleFactorF = scaleFactor / 100.0;

  // if (scaleFactor > 0) {
  scaleFactorF += 1.0;
  //}
  // Serial.print("scaleFactorF: ");
  // Serial.println(scaleFactorF);
  // Serial.print("hexColor: ");
  // Serial.println(hexColor, HEX);

  hsvColor colorToShiftHsv = RgbToHsv(unpackRgb(hexColor));

  float hsvF = colorToShiftHsv.v * scaleFactorF;

  colorToShiftHsv.v = (unsigned char)hsvF;

  rgbColor colorToShiftRgb = HsvToRgb(colorToShiftHsv);
  // Serial.print("shifted: ");
  // Serial.println(packRgb(colorToShiftRgb.r, colorToShiftRgb.g,
  // colorToShiftRgb.b), HEX);
  return packRgb(colorToShiftRgb.r, colorToShiftRgb.g, colorToShiftRgb.b);
  }

uint32_t scaleDownBrightness(uint32_t hexColor, int scaleFactor,
                             int maxBrightness) {
  int maxR = maxBrightness;
  int maxG = maxBrightness;
  int maxB = maxBrightness;

  int r = (hexColor >> 16) & 0xFF;
  int g = (hexColor >> 8) & 0xFF;
  int b = hexColor & 0xFF;

  int scaledBrightness = hexColor;

  if (r > maxR || g > maxG || b > maxB) {
    scaledBrightness = 0;
    r = r / scaleFactor;
    g = g / scaleFactor;
    b = b / scaleFactor;

    scaledBrightness = scaledBrightness | (r << 16);
    scaledBrightness = scaledBrightness | (g << 8);
    scaledBrightness = scaledBrightness | b;
    }

  return scaledBrightness;
  }

uint32_t scaleUpBrightness(uint32_t hexColor, int scaleFactor,
                           int minBrightness) {
  int minR = minBrightness;
  int minG = minBrightness;
  int minB = minBrightness;

  int r = (hexColor >> 16) & 0xFF;
  int g = (hexColor >> 8) & 0xFF;
  int b = hexColor & 0xFF;

  int scaledBrightness = hexColor;

  if (r < minR && g < minG && b < minB) {
    scaledBrightness = 0;
    r = r * scaleFactor;
    g = g * scaleFactor;
    b = b * scaleFactor;

    if (r > 254) {
      r = 254;
      }
    if (g > 254) {
      g = 254;
      }
    if (b > 254) {
      b = 254;
      }

    scaledBrightness = scaledBrightness | (r << 16);
    scaledBrightness = scaledBrightness | (g << 8);
    scaledBrightness = scaledBrightness | b;
    }

  return scaledBrightness;
  }

struct rgbColor pcbColorCorrect(rgbColor colorToShift) { //unused

  uint8_t redShift = 0;
  uint8_t greenShift = 0;
  uint8_t blueShift = 0;

  int testNeg = 0;

  struct rgbColor colorToShiftRgb = colorToShift;

  struct hsvColor colorToShiftHsv = RgbToHsv(colorToShiftRgb);

  colorToShiftHsv.v += PCBEXTINCTION;

  if (colorToShiftHsv.h > 100 && colorToShiftHsv.h < 150) {

    // Serial.print("hue: ");
    // Serial.print(colorToShiftHsv.h);
    // Serial.print("\tcolorToShift.r: ");
    // Serial.print(colorToShift.r);
    // Serial.print("\tcolorToShift.g: ");
    // Serial.print(colorToShift.g);
    // Serial.print("\tcolorToShift.b: ");
    // Serial.print(colorToShift.b);

    if (PCBREDSHIFTBLUE < 0) {
      testNeg = colorToShiftRgb.r;
      testNeg -= abs(PCBREDSHIFTBLUE);

      if (testNeg < 0) {
        colorToShiftRgb.r = 0;
        } else {

        colorToShiftRgb.r = colorToShiftRgb.r - abs(PCBREDSHIFTBLUE);
        }
      } else {

      colorToShiftRgb.r = colorToShiftRgb.r + abs(PCBREDSHIFTBLUE);

      if (colorToShiftRgb.r > 254) {
        colorToShiftRgb.r = 254;
        }
      }

    if (PCBGREENSHIFTBLUE < 0) {

      testNeg = colorToShiftRgb.g;
      testNeg -= abs(PCBGREENSHIFTBLUE);

      if (testNeg < 0) {
        colorToShiftRgb.g = 0;
        } else {
        colorToShiftRgb.g = colorToShiftRgb.g - abs(PCBGREENSHIFTBLUE);
        }
      } else {
      colorToShiftRgb.g = colorToShiftRgb.g + abs(PCBGREENSHIFTBLUE);
      if (colorToShiftRgb.g > 254) {
        colorToShiftRgb.g = 254;
        }
      }

    if (PCBBLUESHIFTBLUE < 0) {

      testNeg = colorToShiftRgb.b;

      testNeg -= abs(PCBBLUESHIFTBLUE);

      if (testNeg < 0) {
        colorToShiftRgb.b = 0;
        } else {
        colorToShiftRgb.b = colorToShiftRgb.b - abs(PCBBLUESHIFTBLUE);
        }
      } else {
      colorToShiftRgb.b = colorToShiftRgb.b + abs(PCBBLUESHIFTBLUE);
      if (colorToShiftRgb.b > 254) {
        colorToShiftRgb.b = 254;
        }
      }

    // Serial.print("\t\tShifted.r: ");

    // Serial.print(colorToShiftRgb.r);
    // Serial.print("\tShifted.g: ");
    // Serial.print(colorToShiftRgb.g);
    // Serial.print("\tShifted.b: ");
    // Serial.println(colorToShiftRgb.b);
    // Serial.print("\n\n\r");
    } else if (colorToShiftHsv.h >= 150 && colorToShiftHsv.h < 255) {

      // Serial.print("hue: ");
      // Serial.print(colorToShiftHsv.h);
      // Serial.print("\tcolorToShift.r: ");
      // Serial.print(colorToShift.r);
      // Serial.print("\tcolorToShift.g: ");
      // Serial.print(colorToShift.g);
      // Serial.print("\tcolorToShift.b: ");
      // Serial.print(colorToShift.b);

      if (PCBREDSHIFTPINK < 0) {
        testNeg = colorToShiftRgb.r;
        testNeg -= abs(PCBREDSHIFTPINK);

        if (testNeg < 0) {
          colorToShiftRgb.r = 0;
          } else {

          colorToShiftRgb.r = colorToShiftRgb.r - abs(PCBREDSHIFTPINK);
          }
        } else {

        colorToShiftRgb.r = colorToShiftRgb.r + abs(PCBREDSHIFTPINK);

        if (colorToShiftRgb.r > 254) {
          colorToShiftRgb.r = 254;
          }
        }

      if (PCBGREENSHIFTPINK < 0) {

        testNeg = colorToShiftRgb.g;
        testNeg -= abs(PCBGREENSHIFTPINK);

        if (testNeg < 0) {
          colorToShiftRgb.g = 0;
          } else {
          colorToShiftRgb.g = colorToShiftRgb.g - abs(PCBGREENSHIFTPINK);
          }
        } else {
        colorToShiftRgb.g = colorToShiftRgb.g + abs(PCBGREENSHIFTPINK);
        if (colorToShiftRgb.g > 254) {
          colorToShiftRgb.g = 254;
          }
        }

      if (PCBBLUESHIFTPINK < 0) {

        testNeg = colorToShiftRgb.b;

        testNeg -= abs(PCBBLUESHIFTPINK);

        if (testNeg < 0) {
          colorToShiftRgb.b = 0;
          } else {
          colorToShiftRgb.b = colorToShiftRgb.b - abs(PCBBLUESHIFTPINK);
          }
        } else {
        colorToShiftRgb.b = colorToShiftRgb.b + abs(PCBBLUESHIFTPINK);
        if (colorToShiftRgb.b > 254) {
          colorToShiftRgb.b = 254;
          }
        }
      }
    return colorToShiftRgb;
  }

struct rgbColor shiftHue(struct rgbColor colorToShift, int hueShift,
  int brightnessShift, int saturationShift,
                         int specialFunction)

  {

  struct hsvColor colorToShiftHsv = RgbToHsv(colorToShift);

  colorToShiftHsv.h = colorToShiftHsv.h + hueShift;
  colorToShiftHsv.s = colorToShiftHsv.s + saturationShift;

  colorToShiftHsv.v = colorToShiftHsv.v + brightnessShift;

  if (colorToShiftHsv.v > 255) {
    colorToShiftHsv.v = 255;
    }

  if (colorToShiftHsv.s > 255) {
    colorToShiftHsv.s = 255;
    }

  if (colorToShiftHsv.h > 255) {
    colorToShiftHsv.h = colorToShiftHsv.h - 255;
    }

  struct rgbColor colorToShiftRgb = HsvToRgb(colorToShiftHsv);

  return colorToShiftRgb;
  }
bool photos = true; //this lets me adjust the brightness for better photos

void lightUpNode(int node, uint32_t color) {

  leds.setPixelColor(nodesToPixelMap[node], color);
  showLEDsCore2 = 1;
  }
uint32_t dimLogoColor(uint32_t color, int brightness) {
  // return color;
  rgbColor dimColor = unpackRgb(color);
  // if (dimColor.b != 0)
  // {
  //     dimColor.b = dimColor.b * 2;
  // }

  hsvColor colorHsv = RgbToHsv(dimColor);

  colorHsv.v = brightness;
  if (photos == true || true) {
    return packRgb(HsvToRgb(colorHsv).r / 3, HsvToRgb(colorHsv).g / 3,
                   HsvToRgb(colorHsv).b);
    } else {
    return packRgb(HsvToRgb(colorHsv).r, HsvToRgb(colorHsv).g * 2,
                   HsvToRgb(colorHsv).b * 3);
    }
  }

uint32_t logoColors[LOGO_COLOR_LENGTH + 11] = {
    0x800058, 0x750053, 0x700068, 0x650063, 0x600078, 0x550073, 0x500088,
    0x450083, 0x400098, 0x350093, 0x3000A8, 0x2500A3, 0x2000B8, 0x1500B3,
    0x1000C8, 0x0502C3, 0x0204D8, 0x0007E3, 0x0010E8, 0x0015F3, 0x0020F8,
    0x0025FA, 0x0030FF, 0x0035E0, 0x0240BF, 0x0545A0, 0x10509F, 0x15558F,
    0x20607F, 0x25656F, 0x30705F, 0x35754F, 0x40803F, 0x45722F, 0x506518,
    0x55481A, 0x603A2A, 0x653332, 0x702538, 0x751948, 0x791052, 0x7E0562,

  };
uint32_t logoColorsCold[LOGO_COLOR_LENGTH + 1];

uint32_t logoColorsHot[LOGO_COLOR_LENGTH + 1];

uint32_t logoColorsPink[LOGO_COLOR_LENGTH + 1];

uint32_t logoColorsGreen[LOGO_COLOR_LENGTH + 1];

uint32_t logoColorsYellow[LOGO_COLOR_LENGTH + 1];

uint32_t logoColors8vSelect[LOGO_COLOR_LENGTH + 11];

uint32_t logoColorsAll[8][LOGO_COLOR_LENGTH + 11];//0=all, 1=cold, 2=hot, 3=pink, 4=yellow, 5=green, 

uint8_t eightSelectHues[LOGO_COLOR_LENGTH + 11] = {
    195, 191, 187, 183, 179, 175, 171, 168, 166, 164, 162, 160, 158, 156, 153,
    148, 140, 130, 120, 111, 104, 99,  96,  93,  91,  89,  87,  85,  83,  81,
    80,  79,  78,  76,  73,  70,  67,  64,  59,  54,  49,  44,  39,  35,  31,
    27,  23,  19,  16,  13,  10,  7,   4,   3,   2,   1,   0,   254, 253, 251,
    248, 242, 236, 230, 224, 218, 212, 207, 202, 199, 197 };

void setupSwirlColors(void) {
  rgbColor logoColorsRGB[LOGO_COLOR_LENGTH + 12];
  int fudgeMult = 1;

  for (int i = 0; i < (LOGO_COLOR_LENGTH / 2) + 1; i++) {

    hsvColor connectHSV;
    connectHSV.h = (i * 2 + 130) % 255;
    connectHSV.s = 254;
    connectHSV.v = 254;
    rgbColor connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColorsCold[i] = packRgb(logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8,
                                logoColorsRGB[i].b / 8);
    logoColorsCold[LOGO_COLOR_LENGTH - i] = logoColorsCold[i];
    logoColorsAll[1][LOGO_COLOR_LENGTH - i] = logoColorsCold[i];
    logoColorsAll[1][i] = logoColorsCold[i];

    connectHSV.h = (i * 2 + 230) % 255;
    connectHSV.s = 254;
    connectHSV.v = 254;
    connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColorsHot[i] = packRgb(logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8,
                               logoColorsRGB[i].b / 8);
    logoColorsHot[LOGO_COLOR_LENGTH - i] = logoColorsHot[i];
    logoColorsAll[2][LOGO_COLOR_LENGTH - i] = logoColorsHot[i];
    logoColorsAll[2][i] = logoColorsHot[i];

    connectHSV.h = (i * 2 + 155) % 255;
    connectHSV.s = 254;
    connectHSV.v = 254;
    connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColorsPink[i] = packRgb(logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8,
                                logoColorsRGB[i].b / 8);
    logoColorsPink[LOGO_COLOR_LENGTH - i] = logoColorsPink[i];
    logoColorsAll[3][LOGO_COLOR_LENGTH - i] = logoColorsPink[i];
    logoColorsAll[3][i] = logoColorsPink[i];

    connectHSV.h = (i * 2 + 55) % 255;
    connectHSV.s = 254;
    connectHSV.v = 254;
    connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColorsYellow[i] = packRgb(
        logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8, logoColorsRGB[i].b / 8);

    logoColorsYellow[LOGO_COLOR_LENGTH - i] = logoColorsYellow[i];
    logoColorsAll[4][LOGO_COLOR_LENGTH - i] = logoColorsYellow[i];
    logoColorsAll[4][i] = logoColorsYellow[i];

    connectHSV.h = (i * 2 + 85) % 255;
    connectHSV.s = 254;
    connectHSV.v = 254;
    connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColorsGreen[i] = packRgb(logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8,
                                 logoColorsRGB[i].b / 8);
    logoColorsGreen[LOGO_COLOR_LENGTH - i] = logoColorsGreen[i];
    logoColorsAll[5][LOGO_COLOR_LENGTH - i] = logoColorsGreen[i];
    logoColorsAll[5][i] = logoColorsGreen[i];
    }

  for (int i = 0; i <= LOGO_COLOR_LENGTH + 10; i++) {

    hsvColor connectHSV;
    connectHSV.h = i * (255 / LOGO_COLOR_LENGTH);
    connectHSV.s = 254;
    connectHSV.v = 254;
    rgbColor connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColors[i] = packRgb(logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8,
                            logoColorsRGB[i].b / 8);
    logoColorsAll[0][i] = logoColors[i];
    connectHSV.h = eightSelectHues[i];

    // connectHSV.h = (connectHSV.h - 8);
    // connectHSV.h<0?connectHSV.h = 254 - connectHSV.h:connectHSV.h;
    connectHSV.s = 254;
    connectHSV.v = 254;
    connectRGB = HsvToRgb(connectHSV);
    logoColorsRGB[i] = connectRGB;

    logoColors8vSelect[i] = packRgb(
        logoColorsRGB[i].r / 8, logoColorsRGB[i].g / 8, logoColorsRGB[i].b / 8);

    logoColorsAll[6][(LOGO_COLOR_LENGTH + 10) - i] = logoColors8vSelect[i];
    //  logoColors8vSelect[LOGO_COLOR_LENGTH - i] =   logoColors8vSelect[i];
    }

  //   //delay(2000);
  //   for (int i = 0; i < LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColorsCold: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColorsCold[i], HEX);

  //   }
  //   Serial.println(" ");
  //     for (int i = 0; i <= LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColorsHot: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColorsHot[i], HEX);

  //   }

  //     for (int i = 0; i < LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColorsPink: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColorsPink[i], HEX);

  //   }

  //     for (int i = 0; i < LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColorsGreen: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColorsGreen[i], HEX);

  //   }

  //     for (int i = 0; i < LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColorsYellow: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColorsYellow[i], HEX);

  //   }

  //     for (int i = 0; i < LOGO_COLOR_LENGTH; i++) {
  // Serial.print("logoColors8vSelect: ");
  // Serial.print(i);
  // Serial.print(" ");
  // Serial.println(logoColors8vSelect[i], HEX);

  //   }
  }




void logoSwirl(int start, int spread, int probe) {

  // int fiddyNine = 58;






  if (probe == 1) {
    int selectionBrightness = 33;


    if (connectOrClearProbe == 1 && node1or2 == 0) {
      // Serial.println("connectOrClearProbe == 1 && node1or2 == 0");

      leds.setPixelColor(
          LOGO_LED_START, dimLogoColor(logoColorsCold[start % (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(
          LOGO_LED_START + 1,
          dimLogoColor(
            logoColorsCold[(start + (spread)) % (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 2,
                         dimLogoColor(logoColorsCold[(start + (spread * 2)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 3,
                         dimLogoColor(logoColorsCold[(start + (spread * 3)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 4,
                         dimLogoColor(logoColorsCold[(start + (spread * 4)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 5,
                         dimLogoColor(logoColorsCold[(start + (spread * 5)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 6,
                         dimLogoColor(logoColorsCold[(start + (spread * 6)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      leds.setPixelColor(LOGO_LED_START + 7,
                         dimLogoColor(logoColorsCold[(start + (spread * 7)) %
                                      (LOGO_COLOR_LENGTH - 1)]));
      } else if (connectOrClearProbe == 1 && node1or2 != 0) {
        leds.setPixelColor(
            LOGO_LED_START, dimLogoColor(logoColorsPink[start % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 1, dimLogoColor(
              logoColorsPink[(start + (spread)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 2,
            dimLogoColor(
              logoColorsPink[(start + (spread * 2)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 3,
            dimLogoColor(
              logoColorsPink[(start + (spread * 3)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 4,
            dimLogoColor(
              logoColorsPink[(start + (spread * 4)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 5,
            dimLogoColor(
              logoColorsPink[(start + (spread * 5)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 6,
            dimLogoColor(
              logoColorsPink[(start + (spread * 6)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));
        leds.setPixelColor(
            LOGO_LED_START + 7,
            dimLogoColor(
              logoColorsPink[(start + (spread * 7)) % (LOGO_COLOR_LENGTH - 1)],
              selectionBrightness));

        } else {


        leds.setPixelColor(
            LOGO_LED_START, dimLogoColor(logoColorsHot[start % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 1,
            dimLogoColor(
              logoColorsHot[(start + (spread)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 2,
            dimLogoColor(
              logoColorsHot[(start + (spread * 2)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 3,
            dimLogoColor(
              logoColorsHot[(start + (spread * 3)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 4,
            dimLogoColor(
              logoColorsHot[(start + (spread * 4)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 5,
            dimLogoColor(
              logoColorsHot[(start + (spread * 5)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 6,
            dimLogoColor(
              logoColorsHot[(start + (spread * 6)) % (LOGO_COLOR_LENGTH - 1)]));
        leds.setPixelColor(
            LOGO_LED_START + 7,
            dimLogoColor(
              logoColorsHot[(start + (spread * 7)) % (LOGO_COLOR_LENGTH - 1)]));
        }

    } else {
    //     for (int i = 0; i < (LOGO_COLOR_LENGTH-1); i++) {

    //  // b.printRawRow(0b11111111, i, 0, 0xffffff);
    // }
   
    leds.setPixelColor(
        LOGO_LED_START, dimLogoColor(logoColors[start % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 1,
        dimLogoColor(logoColors[(start + (spread)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 2, dimLogoColor(
          logoColors[(start + (spread * 2)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 3, dimLogoColor(
          logoColors[(start + (spread * 3)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 4, dimLogoColor(
          logoColors[(start + (spread * 4)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 5, dimLogoColor(
          logoColors[(start + (spread * 5)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 6, dimLogoColor(
          logoColors[(start + (spread * 6)) % (LOGO_COLOR_LENGTH - 1)]));
    leds.setPixelColor(
        LOGO_LED_START + 7, dimLogoColor(
          logoColors[(start + (spread * 7)) % (LOGO_COLOR_LENGTH - 1)]));
    }

  if (logoColorOverride != -1) {
    leds.setPixelColor(LOGO_LED_START, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 1, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 2, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 3, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 4, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 5, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 6, logoColorOverride);
    leds.setPixelColor(LOGO_LED_START + 7, logoColorOverride);
    //return;
    }


  if (logoColorOverrideTop == -2) {
    leds.setPixelColor(LOGO_LED_START + 0, logoColorOverrideTopDefault);
    leds.setPixelColor(LOGO_LED_START + 1, logoColorOverrideTopDefault);
    leds.setPixelColor(LOGO_LED_START + 2, logoColorOverrideTopDefault);
    } else if (logoColorOverrideTop != -1) {
      leds.setPixelColor(LOGO_LED_START + 0, logoColorOverrideTop);
      leds.setPixelColor(LOGO_LED_START + 1, logoColorOverrideTop);
      leds.setPixelColor(LOGO_LED_START + 2, logoColorOverrideTop);
      // leds.setPixelColor(LOGO_LED_START + 6, 0);

      }

    if (logoColorOverrideBottom == -2) {
      leds.setPixelColor(LOGO_LED_START + 3, logoColorOverrideBottomDefault);
      leds.setPixelColor(LOGO_LED_START + 4, logoColorOverrideBottomDefault);
      leds.setPixelColor(LOGO_LED_START + 5, logoColorOverrideBottomDefault);
      } else if (logoColorOverrideBottom != -1) {
        leds.setPixelColor(LOGO_LED_START + 3, logoColorOverrideBottom);
        leds.setPixelColor(LOGO_LED_START + 4, logoColorOverrideBottom);
        leds.setPixelColor(LOGO_LED_START + 5, logoColorOverrideBottom);
        //leds.setPixelColor(LOGO_LED_START + 6, 0);
        }


  }





bool lightUpName = false;

// rgbColor highlightedOriginalColor;
// rgbColor brightenedOriginalColor;
// rgbColor warningOriginalColor;

// int highlightedRow = -1;


// void clearHighlighting(void) {

//   // netColors[highlightedNet] = highlightedOriginalColor;
//   // netColors[brightenedNet] = brightenedOriginalColor;
//   // netColors[warningNet] = warningOriginalColor;

//   for (int i = 4; i < numberOfRowAnimations; i++) {
//     rowAnimations[i].row = -1;
//     rowAnimations[i].net = -1;
//     }
//   probeConnectHighlight = -1;
//   highlightedNet = -1;
//   brightenedNet = -1;
//   warningNet = -1;
//   warningRow = -1;
//   brightenedRail = -1;
//   brightenedNode = -1;
//   highlightedRow = -1;

//   assignNetColors();
//   }



// int lastNodeHighlighted = -1;
// int lastNetPrinted = -1;

// int currentHighlightedNode = 0;
// int currentHighlightedNet = -2;

// int encoderNetHighlight(int print) {
//   int lastDivider = rotaryDivider;
//   rotaryDivider = 4;
//   int returnNode = -1;

//   // if (inClickMenu == 1)
//   //   return -1;
//   //rotaryEncoderStuff();
//   if (encoderDirectionState == UP) {
//     //Serial.println(encoderPosition);
//     encoderDirectionState = NONE;
//     if (highlightedNet < 0) {
//       highlightedNet = -1;
//       brightenedNet = -1;
//       currentHighlightedNode = 0;
//       }
//     currentHighlightedNode++;
//     if (highlightedNet >= 0 && highlightedNet < numberOfNets && net[highlightedNet].nodes[currentHighlightedNode] <= 0) {
//       currentHighlightedNode = 0;
//       highlightedNet++;
//       if (highlightedNet > numberOfNets - 1) {
//         highlightedNet = -2;
//         brightenedNet = -2;
//         currentHighlightedNode = 0;
//         }
//       brightenedNet = highlightedNet;
//       if (highlightedNet >= 0 && highlightedNet < numberOfNets) {
//         brightenedNode = net[highlightedNet].nodes[currentHighlightedNode];
//         if (highlightedNet != 0 && net[highlightedNet].nodes[currentHighlightedNode] != 0) {
//           returnNode = net[highlightedNet].nodes[currentHighlightedNode];
//           }
//         } else {
//         brightenedNode = -1;
//         }
//       highlightNets(0, highlightedNet, print);
//       // Serial.print("highlightedNet: ");
//       // Serial.println(highlightedNet);
//       // Serial.flush();
//       }
//     if (highlightedNet > numberOfNets - 1) {
//       highlightedNet = -2;
//       brightenedNet = -2;
//       currentHighlightedNode = 0;
//       }
//     brightenedNet = highlightedNet;
//     if (highlightedNet >= 0 && highlightedNet < numberOfNets) {
//       brightenedNode = net[highlightedNet].nodes[currentHighlightedNode];
//       if (highlightedNet != 0 && net[highlightedNet].nodes[currentHighlightedNode] != 0) {
//         returnNode = net[highlightedNet].nodes[currentHighlightedNode];
//         }
//       } else {
//       brightenedNode = -1;
//       }
//     // Serial.print("returnNode: ");
//     // Serial.println(returnNode);
//     // Serial.flush();
//     highlightNets(0, highlightedNet, print);
//     // Serial.print("highlightedNet: ");
//     // Serial.println(highlightedNet);
//     // Serial.flush();
//     //assignNetColors();
//    // assignNetColors();

//     } else if (encoderDirectionState == DOWN) {
//       //Serial.println(encoderPosition);
//       encoderDirectionState = NONE;
//       if (highlightedNet == 0) {

//         highlightedNet = numberOfNets - 1;
//         brightenedNet = numberOfNets - 1;
//         }

//       currentHighlightedNode--;

//       if (currentHighlightedNode < 0) {
//         highlightedNet--;
//         if (highlightedNet < 0) {
//           highlightedNet = numberOfNets - 1;
//           brightenedNet = numberOfNets - 1;
//           currentHighlightedNode = 0;
//           }
//         currentHighlightedNode = MAX_NODES - 1;
//         while (highlightedNet >= 0 && highlightedNet < numberOfNets && net[highlightedNet].nodes[currentHighlightedNode] <= 0) {
//           currentHighlightedNode--;
//           if (currentHighlightedNode < 0) {
//             highlightedNet--;
//             if (highlightedNet < 0) {
//               highlightedNet = numberOfNets - 1;
//               brightenedNet = numberOfNets - 1;
//               currentHighlightedNode = 0;
//               }

//             }
//           }
//         }
//       brightenedNet = highlightedNet;
//       if (highlightedNet >= 0 && highlightedNet < numberOfNets) {
//         brightenedNode = net[highlightedNet].nodes[currentHighlightedNode];
//         if (highlightedNet != 0 && net[highlightedNet].nodes[currentHighlightedNode] != 0) {
//           returnNode = net[highlightedNet].nodes[currentHighlightedNode];
//           }
//         } else {
//         brightenedNode = -1;
//         }
//       // Serial.print("returnNode: ");
//       // Serial.println(returnNode);
//       // Serial.flush();
//       highlightNets(0, highlightedNet, print);
//       // Serial.print("highlightedNet: ");
//       // Serial.println(highlightedNet);
//       // Serial.flush();
//       // assignNetColors();



//       }
//     if (returnNode != lastNodeHighlighted) {
//       // b.clear();
//      // b.printRawRow(0b00000100, lastNodeHighlighted-2, 0x000000, 0x000000);
//      // b.printRawRow(0b00000100, lastNodeHighlighted, 0x0000000, 0x000000);

//      // b.printRawRow(0b00000100, returnNode-2, 0x0f0f00, 0x000000);
//      // b.printRawRow(0b00000100, returnNode, 0x0f0f00, 0x000000);

//       lastNodeHighlighted = returnNode;
//       // showLEDsCore2 = 2;
//       }
//     //rotaryDivider = lastDivider;
//     return returnNode;
//   }







// int brightenNet(int node, int addBrightness) {

//   if (node == -1) {
//     netColors[brightenedNet] = brightenedOriginalColor;
//     brightenedNode = -1;
//     brightenedNet = 0;
//     brightenedRail = -1;
//     return -1;
//     }
//   addBrightness = 0;

//   for (int i = 0; i <= numberOfPaths; i++) {

//     if (node == path[i].node1 || node == path[i].node2) {
//       /// if (brightenedNet != i) {
//       brightenedNet = path[i].net;
//       brightenedNode = node;
//       // Serial.print("\n\n\rbrightenedNet: ");
//       // Serial.println(brightenedNet);
//       // Serial.print("net ");
//       // Serial.print(path[i].net);
//       if (brightenedNet == 1) {
//         brightenedRail = 1;
//         // lightUpRail(-1, 1, 1, addBrightness);
//         } else if (brightenedNet == 2) {
//           brightenedRail = 0;
//           // lightUpRail(-1, 0, 1, addBrightness);
//           } else if (brightenedNet == 3) {
//             brightenedRail = 2;
//             // lightUpRail(-1, 2, 1, addBrightness);
//             } else {
//             brightenedRail = -1;
//             // lightUpNet(brightenedNet, addBrightness);
//             }
//           // Serial.print("\n\rbrightenedNet = ");
//           // Serial.println(brightenedNet);
//           brightenedOriginalColor = netColors[brightenedNet];
//           assignNetColors();
//           return brightenedNet;
//       }
//     }
//   switch (node) {
//     case (GND): {
//     //  Serial.print("\n\rGND");
//     brightenedNet = 1;
//     brightenedRail = 1;
//     // lightUpRail(-1, 1, 1, addBrightness);
//     return 1;
//     }
//     case (TOP_RAIL): {
//     // Serial.print("\n\rTOP_RAIL");
//     brightenedNet = 2;
//     brightenedRail = 0;
//     // lightUpRail(-1, 0, 1, addBrightness);
//     return 2;
//     }
//     case (BOTTOM_RAIL): {
//     //Serial.print("\n\rBOTTOM_RAIL");
//     brightenedNet = 3;
//     brightenedRail = 2;
//     // lightUpRail(-1, 2, 1, addBrightness);
//     return 3;
//     }
//     }


//   return -1;
//   }

// int warningRow = -1;
// int warningNet = -1;
// unsigned long warningTimeout = 0;
// unsigned long warningTimer = 0;

// /// @brief  mark a net as warning
// /// @param -1 to clear warning
// /// @return warningNet
// int warnNet(int node) {
//   // Serial.print("warnNet node = ");
//   // Serial.println(node);
//   // Serial.flush();
//   if (node == -1) {
//     netColors[warningNet] = warningOriginalColor;

//     warningNet = -1;
//     warningRow = -1;
//     // Serial.print("warningNet = ");
//     // Serial.println(warningNet);
//     // Serial.flush();
//     // brightenedRail = -1;
//     return -1;
//     }
//   // addBrightness = 0;
//   warningRow = bbPixelToNodesMap[node];

//   for (int i = 0; i <= numberOfPaths; i++) {

//     if (node == path[i].node1 || node == path[i].node2) {
//       /// if (brightenedNet != i) {
//       warningNet = path[i].net;

//       // Serial.print("warningNet = ");
//       // Serial.println(warningNet);
//       // Serial.flush();


//       if (warningNet == 1) {
//         // brightenedRail = 1;
//         // lightUpRail(-1, 1, 1, addBrightness);
//         } else if (warningNet == 2) {
//           // brightenedRail = 0;
//           // lightUpRail(-1, 0, 1, addBrightness);
//           } else if (warningNet == 3) {
//             // brightenedRail = 2;
//             // lightUpRail(-1, 2, 1, addBrightness);
//             } else {
//             // brightenedRail = -1;
//             // lightUpNet(brightenedNet, addBrightness);
//             }

//           warningOriginalColor = netColors[warningNet];
//           assignNetColors();
//           warningTimer = millis();
//           return warningNet;
//       }
//     }



//   return -1;
//   }
// unsigned long lastWarningTimer = 0;
// unsigned long lastHighlightTimer = 0;
// void warnNetTimeout(int clearAll) {
//   // Serial.print("warningTimer = ");
//   // Serial.println(warningTimer);
//   // Serial.print("warningTimeout = ");
//   // Serial.println(warningTimeout);
//   // Serial.flush();
//   if (lastWarningTimer == 0) {
//     lastWarningTimer = millis();
//     }

//   if (lastHighlightTimer == 0) {
//     lastHighlightTimer = millis();
//     }

//   if (warningTimer > 0 && millis() - warningTimer > warningTimeout) {
//     //warningTimeout = 0;
//     if (clearAll == 1) {
//       clearHighlighting();
//       } else {
//       // netColors[warningNet] = warningOriginalColor;


//       warningNet = -1;
//       warningRow = -1;
//       }
//     lastWarningTimer = millis();
//     warningTimer = 0;

//     assignNetColors();
//     } else {
//     lastWarningTimer = millis() - lastWarningTimer;
//     // Serial.print("lastWarningTimer = ");  
//     // Serial.println(lastWarningTimer);
//     // Serial.flush();
//    // warningTimer = millis();
//     }
//   }
// uint32_t rawSpecialNetColors[8] = // dim
//     {0x000000, 0x001C04, 0x1C0702, 0x1C0107,
//      0x231111, 0x230913, 0x232323, 0x232323};
uint32_t rstColors[2] = { 0x2000b9, 0x0020f9, };

///0 = rst_b 1 = rst_t  2 = vin  3 = 3v3  4 = 5v  5 = gnd_t  6 = gnd_b
uint32_t headerColors[7] = { 0x2000b9, 0x0020f9, 0xa0a000,  0x1C0107,0x1C0702, 0x001C04,0x001C04 };
unsigned long colorFlash[2] = { 0,0 };

// uint32_t railColors[4] = {0x2000b9, 0x0020f9, 0xa0a000, 0x1C0107};

void lightUpHeader(void) {


  if (rstColors[0] != headerColors[0] && colorFlash[0] == 0) {
    colorFlash[0] = millis();
    } else if (rstColors[0] != headerColors[0] && millis() - colorFlash[0] > 150) {
      colorFlash[0] = 0;
      rstColors[0] = headerColors[0];
      }
    if (rstColors[1] != headerColors[1] && colorFlash[1] == 0) {
      colorFlash[1] = millis();
      } else if (rstColors[1] != headerColors[1] && millis() - colorFlash[1] > 150) {
        colorFlash[1] = 0;
        rstColors[1] = headerColors[1];
        }

      if (RST0colorOverride != -1) {
        leds.setPixelColor(RST_0_LED, RST0colorOverride);
        } else {
        leds.setPixelColor(RST_0_LED, scaleDownBrightness(rstColors[0], 4, 55));
        }
      if (RST1colorOverride != -1) {
        leds.setPixelColor(RST_1_LED, RST1colorOverride);
        } else {
        leds.setPixelColor(RST_1_LED, scaleDownBrightness(rstColors[1], 4, 55));
        }
      if (GNDTcolorOverride != -1) {
        leds.setPixelColor(GND_T_LED, GNDTcolorOverride);
        } else {
        leds.setPixelColor(GND_T_LED, scaleDownBrightness(headerColors[5], 2, 35));
        }
      if (GNDBcolorOverride != -1) {
        leds.setPixelColor(GND_B_LED, GNDBcolorOverride);
        } else {
        leds.setPixelColor(GND_B_LED, scaleDownBrightness(headerColors[6], 2, 35));
        }
      if (VINcolorOverride != -1) {
        leds.setPixelColor(VIN_LED, VINcolorOverride);
        } else {
        leds.setPixelColor(VIN_LED, scaleDownBrightness(headerColors[2], 5, 35));
        }
      if (V3V3colorOverride != -1) {
        leds.setPixelColor(V3V3_LED, V3V3colorOverride);
        } else {
        leds.setPixelColor(V3V3_LED, scaleDownBrightness(headerColors[3], 5, 35));
        }
      if (V5VcolorOverride != -1) {
        leds.setPixelColor(V5V_LED, V5VcolorOverride);
        } else {
        leds.setPixelColor(V5V_LED, scaleDownBrightness(headerColors[4], 5, 35));
        }

      if (sfProbeMenu == 1) {
        leds.setPixelColor(ADC_LED_0, scaleBrightness(rawOtherColors[8], -40));
        leds.setPixelColor(ADC_LED_1, scaleBrightness(rawOtherColors[11], -40));
        leds.setPixelColor(DAC_LED_0, 0);
        leds.setPixelColor(DAC_LED_1, 0);
        leds.setPixelColor(GPIO_LED_0, 0);
        leds.setPixelColor(GPIO_LED_1, 0);

        } else if (sfProbeMenu == 2) {
          leds.setPixelColor(ADC_LED_0, 0);
          leds.setPixelColor(ADC_LED_1, 0);
          leds.setPixelColor(DAC_LED_0, scaleBrightness(rawOtherColors[9], -40));
          leds.setPixelColor(DAC_LED_1, scaleBrightness(rawOtherColors[12], -40));
          leds.setPixelColor(GPIO_LED_0, 0);
          leds.setPixelColor(GPIO_LED_1, 0);
          } else if (sfProbeMenu == 3) {
            leds.setPixelColor(ADC_LED_0, 0);
            leds.setPixelColor(ADC_LED_1, 0);
            leds.setPixelColor(DAC_LED_0, 0);
            leds.setPixelColor(DAC_LED_1, 0);
            leds.setPixelColor(GPIO_LED_0, scaleBrightness(rawOtherColors[10], -40));
            leds.setPixelColor(GPIO_LED_1, scaleBrightness(rawOtherColors[13], -40));

            } else {
            if (photos == true && false) {
              leds.setPixelColor(ADC_LED_0, scaleBrightness(rawOtherColors[8], -40));
              leds.setPixelColor(ADC_LED_1, scaleBrightness(rawOtherColors[11], -40));
              leds.setPixelColor(DAC_LED_0, scaleBrightness(rawOtherColors[9], -40));
              leds.setPixelColor(DAC_LED_1, scaleBrightness(rawOtherColors[12], -40));
              leds.setPixelColor(GPIO_LED_0, scaleBrightness(rawOtherColors[10], -40));
              leds.setPixelColor(GPIO_LED_1, scaleBrightness(rawOtherColors[13], -40));
              } else {

              if (ADCcolorOverride0 == -1) {
                leds.setPixelColor(ADC_LED_0, scaleBrightness(rawOtherColors[8], -40));
                } else if (ADCcolorOverride0 == -2) {
                  leds.setPixelColor(ADC_LED_0, ADCcolorOverride0Default);
                  } else {
                  leds.setPixelColor(ADC_LED_0, ADCcolorOverride0);
                  }

                if (ADCcolorOverride1 == -1) {
                  leds.setPixelColor(ADC_LED_1, scaleBrightness(rawOtherColors[11], -40));
                  } else if (ADCcolorOverride1 == -2) {
                    leds.setPixelColor(ADC_LED_1, ADCcolorOverride1Default);
                    } else {
                    leds.setPixelColor(ADC_LED_1, ADCcolorOverride1);
                    }

                  if (DACcolorOverride0 == -1) {
                    leds.setPixelColor(DAC_LED_0, scaleBrightness(rawOtherColors[9], -40));
                    } else if (DACcolorOverride0 == -2) {
                      leds.setPixelColor(DAC_LED_0, DACcolorOverride0Default);
                      } else {
                      leds.setPixelColor(DAC_LED_0, DACcolorOverride0);
                      }

                    if (DACcolorOverride1 == -1) {
                      leds.setPixelColor(DAC_LED_1, scaleBrightness(rawOtherColors[12], -40));
                      } else if (DACcolorOverride1 == -2) {
                        leds.setPixelColor(DAC_LED_1, DACcolorOverride1Default);
                        } else {
                        leds.setPixelColor(DAC_LED_1, DACcolorOverride1);
                        }

                      if (GPIOcolorOverride0 == -1) {
                        leds.setPixelColor(GPIO_LED_0, scaleBrightness(rawOtherColors[10], -40));
                        } else if (GPIOcolorOverride0 == -2) {
                          leds.setPixelColor(GPIO_LED_0, GPIOcolorOverride0Default);
                          } else {
                          leds.setPixelColor(GPIO_LED_0, GPIOcolorOverride0);
                          }

                        if (GPIOcolorOverride1 == -1) {
                          leds.setPixelColor(GPIO_LED_1, scaleBrightness(rawOtherColors[13], -40));
                          } else if (GPIOcolorOverride1 == -2) {
                            leds.setPixelColor(GPIO_LED_1, GPIOcolorOverride1Default);
                            } else {
                            leds.setPixelColor(GPIO_LED_1, GPIOcolorOverride1);
                            }
              }
            }

  }


uint32_t rawRailColors[3][4] =
  { {0x1C0110, 0x001C04, 0x1C0110, 0x001C04},

   {0x210904, 0x001C04, 0x210904, 0x001C04},

   {0x301A02, 0x001C04, 0x120932, 0x001C04} };


uint32_t railColorsV5[4][5] = {
  {0x1b010b, 0x21030b, 0x301A02, 0x210806, 0x16051b},//top dim, top bright, top danger, top brightened, top negative
  {0x001C05, 0x002C14, 0x0f2C0f, 0x0f2C0f, 0x002C14},//gnd dim, gnd bright, gnd danger, gnd brightened, gnd negative
  {0x1a020e, 0x20040a, 0x301A02, 0x210a08, 0x15061a},//bottom dim, bottom bright, bottom danger, bottom brightened, bottom negative
  {0x001C05, 0x002514, 0x0f2C0f, 0x0a2C0a, 0x002C14}//gnd dim, gnd bright, gnd danger, gnd brightened, gnd negative
  };



uint32_t dotColor = 0x06061f;
uint32_t dangerDot = 0x10101f;
uint32_t negDot = 0x1f0006;


int scaleScale(int value) {
  int ledBrightnessDelta = LEDbrightnessRail - (DEFAULTRAILBRIGHTNESS);

  int scaleFactor = ledBrightnessDelta;
  // Serial.print("scaleFactor ");
  // Serial.println(scaleFactor);
  // Serial.print("value ");
  // Serial.println(value);

  int scaled = value;
  //scaled = value / (abs(scaleFactor) + 1);

  scaled = value + (int)(scaleFactor * (abs((float)value) / 8.0));


  // Serial.print("\n\rscaled ");
  // Serial.println(scaled);

  if (scaled < -94) {
    scaled = -94;
    } else if (scaled > 400) {
      scaled = 400;
      }


    return scaled;


  }


void lightUpRail(int logo, int rail, int onOff, int brightness2,
                 int switchPosition) {


  lightUpHeader();
  for (int j = 0; j < 4; j++) {
    if (j == rail || rail == -1) {


      uint32_t color = railColorsV5[j][0];



      for (int i = 0; i < 25; i++) {

        if (onOff == 1) {
          // uint32_t color = packRgb((railColors[j].r * brightness2) >> 8,
          // (railColors[j].g * brightness2) >> 8, (railColors[j].b *
          // brightness2) >> 8);

          /// Serial.println(color,HEX);
          // Serial.print("brightenedRail ");
          // Serial.println(brightenedRail);
          if (brightenedRail == j || ((j == 1 || j == 3) && (brightenedRail == 1 || brightenedRail == 3))) { //this is just when the rails are highlighted
            // color = scaleBrightness(color, brightness2);
            if (j % 2 == 0) { //only the top and bottom rails 
              int powerRail = j / 2;

              if (railVoltage[powerRail] < 0.0) { //flipped when the voltage is negative
                if ((i == 24 - (abs((int)(railVoltage[powerRail] * 5)))) && (abs((int)railVoltage[powerRail]) <= 5.0)) {

                  leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dotColor, 250));

                  } else if (i == 49 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                    leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dangerDot, 280));


                    } else if (i > 24 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) < 5.0)) {

                      leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][3], scaleScale(0)));

                      } else if (i > 49 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                        leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][2], scaleScale(30)));

                        } else if (i < 49 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][3], scaleScale(0)));

                          } else {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][3], scaleScale(-40)));

                          }



                } else if ((i == abs((int)((railVoltage[powerRail] - 0.1) * 5))) && (abs((int)railVoltage[powerRail]) <= 5.0)) {

                  leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dotColor, scaleScale(250)));

                  } else if ((i == (abs((int)((railVoltage[powerRail] - 5.1) * 5)))) && (abs((int)railVoltage[powerRail]) >= 5.0)) {

                    leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dangerDot, scaleScale(280)));

                    } else if (i < abs((int)(((railVoltage[powerRail] + 0.1) * 5) - 1)) && (abs((int)railVoltage[powerRail]) < 5.0)) {

                      leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][3], scaleScale(15)));

                      } else if (i < (abs((int)(((railVoltage[powerRail] - 4.9) * 5) - 1))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                        leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][2], scaleScale(30)));
                        } else if (i > (abs((int)(((railVoltage[powerRail] - 4.9) * 5) - 1))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][3], scaleScale(15)));

                          } else {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][1], scaleScale(-40)));
                          }
              } else {
              leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][1], scaleScale(-50)));
              }

            } else { //this is when the rails are not highlighted
            // Serial.println("brightness2");
            // Serial.println(brightness2);
            if (j % 2 == 0) { //only the top and bottom rails 
              int powerRail = j / 2;

              if (railVoltage[powerRail] < -0.1) { //flipped when the voltage is negative
                if ((i == 25 - (abs((int)(railVoltage[powerRail] * 5)))) && (abs((int)railVoltage[powerRail]) < 5.0)) {

                  leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(negDot, 250));

                  } else if (i == 50 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                    leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dangerDot, 250));


                    } else if (i > 25 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) < 5.0)) {

                      leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][4], scaleScale(-20)));

                      } else if (i > 50 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                        leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][2], scaleScale(-30)));

                        } else if (i < 50 - (abs((int)((railVoltage[powerRail] * 5)))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][4], scaleScale(-20)));

                          } else {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][0], scaleScale(-85)));

                          }



                } else if ((i == abs((int)((railVoltage[powerRail] - 0.1) * 5))) && (abs((int)railVoltage[powerRail]) < 5.0)) {

                  leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dotColor, scaleScale(150)));

                  } else if ((i == (abs((int)((railVoltage[powerRail] - 5.1) * 5)))) && (abs((int)railVoltage[powerRail]) >= 5.0)) {

                    leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(dangerDot, scaleScale(250)));

                    } else if (i < abs((int)(((railVoltage[powerRail] + 0.1) * 5) - 1)) && (abs((int)railVoltage[powerRail]) < 5.0)) {

                      leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][1], scaleScale(-20)));

                      } else if (i < (abs((int)(((railVoltage[powerRail] - 4.9) * 5) - 1))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                        leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][2], scaleScale(-30)));

                        } else if (i > (abs((int)(((railVoltage[powerRail] - 4.9) * 5) - 1))) && (abs(railVoltage[powerRail]) >= 5.0)) {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][1], scaleScale(-20)));

                          } else {

                          leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][0], scaleScale(-85)));
                          }
              } else {
              leds.setPixelColor(railsToPixelMap[j][i], scaleBrightness(railColorsV5[j][0], scaleScale(-80)));
              }
            }

          } else {
          leds.setPixelColor(railsToPixelMap[j][i], 0);
          }
        }
      }
    }


  if (lightUpName == true) {
    leds.setPixelColor(railsToPixelMap[0][20], 0x0010a0);
    leds.setPixelColor(railsToPixelMap[0][21], 0x1f0050);
    leds.setPixelColor(railsToPixelMap[0][22], 0x1d0030);
    leds.setPixelColor(railsToPixelMap[0][23], 0x1e0020);
    leds.setPixelColor(railsToPixelMap[0][24], 0x2f0010);

    leds.setPixelColor(railsToPixelMap[1][20], 0x0040f0);
    leds.setPixelColor(railsToPixelMap[1][21], 0x0f0050);
    leds.setPixelColor(railsToPixelMap[1][22], 0x0d0030);
    leds.setPixelColor(railsToPixelMap[1][23], 0x0e0020);
    leds.setPixelColor(railsToPixelMap[1][24], 0x2f0000);
    }
  // leds.show();
  // showLEDsCore2 = 1;
  // delay(3);
  }
//int displayMode = jumperlessConfig.display.lines_wires; // 0 = lines 1= wires

void showNets(void) {
  // Serial.println(rp2040.cpuid());
  // core2busy = true;
  //                if (debugNTCC > 0) {
  //      Serial.println(debugNTCC);
  //    }
  //  if (rp2040.cpuid() == 0) {
  //    core1busy = true;
  //   } else {
  core2busy = true;
  int skipShow = 0;
  //}
  // if (debugNTCC > 0) {
  //   Serial.println(debugNTCC);
  // }
  //displayMode = jumperlessConfig.display.lines_wires;

    if (jumperlessConfig.display.lines_wires == 0 || numberOfShownNets > MAX_NETS_FOR_WIRES) {
    assignNetColors();
    for (int i = 0; i <= numberOfNets; i++) {
      // Serial.print(i);
      skipShow = 0;
      for (int j = 0; j < 5; j++) {
        if (showADCreadings[j] == i) {
          //skipShow = 1;
          continue;
          }
        for (int k = 0; k < 10; k++) {
          if (gpioNet[k] == i) {
            //skipShow = 1;
            continue;
            }
          }
        }
      if (skipShow == 1) {
        continue;
        }

      lightUpNet(i);
      }
      } else if (jumperlessConfig.display.lines_wires == 1) {
      // if (i==1){
      //   continue;
      // }
      drawWires();
      }
    //               if (debugNTCC > 0) {
    //   Serial.println(debugNTCC);
    // }
    // showLEDsCore2 = 1;
    //     if (rp2040.cpuid() == 0) {
    //   core1busy = false;
    //  } else {
    core2busy = false;
    // }
  }

rgbColor HsvToRgb(hsvColor hsv) {
  rgbColor rgb;
  unsigned char region, p, q, t;
  unsigned int h, s, v, remainder;

  if (hsv.s == 0) {
    rgb.r = hsv.v;
    rgb.g = hsv.v;
    rgb.b = hsv.v;
    return rgb;
    }

  // converting to 16 bit to prevent overflow
  h = hsv.h;
  s = hsv.s;
  v = hsv.v;

  region = h / 43;
  remainder = (h - (region * 43)) * 6;

  p = (v * (255 - s)) >> 8;
  q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:
      rgb.r = v;
      rgb.g = t;
      rgb.b = p;
      break;
    case 1:
      rgb.r = q;
      rgb.g = v;
      rgb.b = p;
      break;
    case 2:
      rgb.r = p;
      rgb.g = v;
      rgb.b = t;
      break;
    case 3:
      rgb.r = p;
      rgb.g = q;
      rgb.b = v;
      break;
    case 4:
      rgb.r = t;
      rgb.g = p;
      rgb.b = v;
      break;
    default:
      rgb.r = v;
      rgb.g = p;
      rgb.b = q;
      break;
    }

  return rgb;
  }

uint32_t HsvToRaw(hsvColor hsv) {
  rgbColor rgb = HsvToRgb(hsv);
  return rgb.r << 16 | rgb.g << 8 | rgb.b;
  }

hsvColor RgbToHsv(rgbColor rgb) {
  hsvColor hsv;
  unsigned char rgbMin, rgbMax;

  rgbMin = rgb.r < rgb.g ? (rgb.r < rgb.b ? rgb.r : rgb.b)
    : (rgb.g < rgb.b ? rgb.g : rgb.b);
  rgbMax = rgb.r > rgb.g ? (rgb.r > rgb.b ? rgb.r : rgb.b)
    : (rgb.g > rgb.b ? rgb.g : rgb.b);

  hsv.v = rgbMax;
  if (hsv.v == 0) {
    hsv.h = 0;
    hsv.s = 0;
    return hsv;
    }

  hsv.s = 255 * ((long)(rgbMax - rgbMin)) / hsv.v;
  if (hsv.s == 0) {
    hsv.h = 0;
    return hsv;
    }

  if (rgbMax == rgb.r)
    hsv.h = 0 + 43 * (rgb.g - rgb.b) / (rgbMax - rgbMin);
  else if (rgbMax == rgb.g)
    hsv.h = 85 + 43 * (rgb.b - rgb.r) / (rgbMax - rgbMin);
  else
    hsv.h = 171 + 43 * (rgb.r - rgb.g) / (rgbMax - rgbMin);

  return hsv;
  }

hsvColor RgbToHsv(uint32_t color) {
  rgbColor rgb;
  rgb.r = (color >> 16) & 0xFF;
  rgb.g = (color >> 8) & 0xFF;
  rgb.b = color & 0xFF;
  return RgbToHsv(rgb);
  }

void randomColors(void) {

  int count = 0;
  uint32_t color = 0;
  for (int i = 0; i < 445; i++) {

    count = random(0, 25);
    // if (i > 80)
    // {
    //     count = random(0, 22);
    // }

    byte colorValR = random(0, 52);
    byte colorValG = random(0, 52);
    byte colorValB = random(0, 52);

    color = colorValR << 16 | colorValG << 8 | colorValB;
    switch (count) {
      case 0:
        color = color & 0x00ffff;
        break;
      case 1:
        color = color & 0xff00ff;
        break;
      case 2:
        color = color & 0xffff00;
        break;
      case 3:
        color = color & 0x0000ff;
        break;
      case 4:
        color = color & 0x00ff00;
        break;
      case 5:
        color = color & 0xff0000;
        break;
      case 6:
        color = color & 0x000f0f;
        break;
      case 7:
        color = color & 0x0f000f;
        break;
      case 8:
        color = color & 0x0f0f00;
        break;
      case 9:
        color = color & 0x0000f0;
        break;
      case 10:
        color = color & 0x00f000;
        break;
      case 11:
        color = color & 0xf00000;
        break;
      case 12:
        color = color & 0xaaaaaa;
        break;
      case 13:
        color = color & 0x555555;
        break;
      case 14:
        break;


      default:
        color = color & 0x000000;
        break;
      }
    // color = color | (color >> 1);

    leds.setPixelColor(i, color); //  Set pixel's color (in RAM)
    // lightUpRail(-1, -1, 1, LEDbrightnessRail);
    showLEDsCore2 = 3; //  Update strip to match
    //  Pause for a moment
    }
  // delay(500);
 // delay(wait);
  }

void rainbowy(int saturation, int brightness, int wait) {

  hsvColor hsv;
  int bounce = 0;
  int offset = 1;

  for (long j = 0; j < 60; j += 1) {

    for (int i = 0; i < LED_COUNT; i++) {
      float huef;
      float i2 = i;
      float j2 = j;

      // huef = ((i * j)) * 0.1f; //*254.1;
      //  hsv.h = (j*(i*j))%255;
      // hsv.h = (j*(int((sin(j+i)*4))))%254;
      //
      huef = sinf((i2 * (j2)) * 3.0f); //*(sinf((j2*i2))*19.0);
      hsv.h = ((int)(huef)) % 254;

      hsv.s = 254;
      hsv.v = 80;
      rgbColor rgb = HsvToRgb(hsv);
      uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);
      // rgbPacked = rgbPacked * i
      leds.setPixelColor((i + offset) % LED_COUNT, rgbPacked);
      }

    offset += 1;
    // offset = offset % 80;
    showLEDsCore2 = 1;
    delayMicroseconds((wait * 1000)); //*((j/20.0)));
    }
  }

void startupColors(void) {
  hsvColor hsv;
  int bounce = 0;
  int offset = 1;
  int fade = 0;
  int done = 0;
  // int logoColor = 66;
  // int logoStep = 255/66;

  for (long j = 4; j < 162; j += 2) {
    if (j < DEFAULTBRIGHTNESS / 3) {
      fade = j * 3;
      } else {
      int fadeout = j - DEFAULTBRIGHTNESS;
      if (fadeout < 0) {
        fadeout = 0;
        }
      if (fadeout > DEFAULTBRIGHTNESS) {
        fadeout = DEFAULTBRIGHTNESS;
        done = 1;
        // Serial.println(j);
        //  break;
        }
      fade = DEFAULTBRIGHTNESS - fadeout;
      }

    for (int i = 0; i < LED_COUNT; i++) {
      float huef;
      float i2 = i;
      float j2 = j + 50;

      huef = ((i2 * j2)) * 0.1f; //*254.1;
      // hsv.h = (j*(i*j))%255;
      // hsv.h = (j*(int((sin(j+i)*4))))%254;
      hsv.h = ((int)(huef)) % 254;
      hsv.s = 254;
      // if (((i + offset) % LED_COUNT) == 110)
      // {
      //     hsv.v = 85;
      //     hsv.h = (189 + j);
      // }
      // else
      // {

      //     hsv.v = fade;
      // }
      rgbColor rgb = HsvToRgb(hsv);
      uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);
      // rgbPacked = rgbPacked * i

      leds.setPixelColor((i + offset) % LED_COUNT, rgbPacked);
      }

    offset += 1;
    // offset = offset % 80;
    // lightUpRail(1);
    // showLEDsCore2 = 1;
    leds.show();
    if (done == 0) {
      delayMicroseconds((14000)); //*((j/20.0)));
      } else {
      break;
      }
    // Serial.println(j);
    }
  // clearLEDs();
  //  lightUpRail();
  //   showLEDsCore2 = 1;
  }

uint32_t chillinColors[LED_COUNT + 200] = {
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000,
    0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x000000, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003, 0x050003,
    0x050003, 0x050003, 0x050003, 0x050003, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500, 0x000500,
    0x000500, 0x010006, 0x010006, 0x010006, 0x001C04, 0x010006, 0x010006,
    0x010006, 0x010006, 0x010006, 0x010006, 0x010006, 0x010006, 0x010006,
    0x010006, 0x010006, 0x010006, 0x1C0107, 0x010006, 0x010006, 0x010006,
    0x010006, 0x010006, 0x010006, 0x010006, 0x010006, 0x010006, 0x1C0702,
    0x010006, 0x001C04, 0x1C0702, 0x380303, 0x380303, 0x253803, 0x253803,
    0x0000E5, 0x0000E5, 0x07000F, 0x030015, 0x000015, 0x000015, 0x010815,
    0x050E03, 0x07000F, 0x030015, 0x000000 };

void startupColorsV5(void) {
  int logo = 1;

  hsvColor hsv;
  int bounce = 0;

  long jStart = 250;
  for (long j = 250; j >= 0; j -= 1) {

    for (int i = 0; i < LED_COUNT - 9; i++) {
      float huef;

      float i2 = i;
      float j2 = j;

      if (i >= 300) {
        i2 = (LED_COUNT - 9) - i;
        j2 = jStart - j;
        }

      huef = sinf((i2 / (j2)) * 2.0f); //*(sinf((j2/i2))*19.0);
      // hsv.h = (j*(i*j))%255;
      // hsv.h = (j*(int((sin(j+i)*4))))%254;
      hsv.h = ((int)(huef * 255)) % 254;
      hsv.s = 254;
      if (i <= 400) {
        hsv.v = 6;
        } else {
        hsv.v = 16;
        }
      rgbColor rgb = HsvToRgb(hsv);
      if (i >= 300) {

        rgbColor chillin = unpackRgb(chillinColors[i]);
        rgb.r = (((rgb.r) * (j)) + (chillin.r) * ((jStart - j))) / (jStart);
        rgb.g = (((rgb.g) * (j)) + (chillin.g) * ((jStart - j))) / (jStart);
        rgb.b = (((rgb.b) * (j)) + (chillin.b) * ((jStart - j))) / (jStart);
        } else if (j < 60) {
          rgb.r = ((rgb.r * j)) / 60;
          rgb.g = ((rgb.g * j)) / 60;
          rgb.b = ((rgb.b * j)) / 60;

          //   rgb.g = rgb.g;
          //   rgb.b = rgb.b;
          }
        //   rgb.r = (rgb.r + chillin.r) / 2;

        //     rgb.g = (rgb.g + chillin.g) / 2;

        //     rgb.b = (rgb.b + chillin.b) / 2;

        //   rgb.r = ((rgb.r * (jStart - j)) + (chillin.r * j) / jStart) / 2;

        //   rgb.g = ((rgb.g * j) + (chillin.g * (jStart - j)) / jStart) / 2;

        //   rgb.b = ((rgb.b * j) + (chillin.b * (jStart - j)) / jStart) / 2;

        uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);

        // rgbPacked = rgbPacked * i
        if (i < 300 && logo == 1) {

          if (jumperlessText[textMap[i]] == 0) {
            leds.setPixelColor(i, rgbPacked);
            } else {
            leds.setPixelColor(i, 0);
            }
          } else {
          leds.setPixelColor(i, rgbPacked);
          }
        // leds.setPixelColor(i, rgbPacked);
      }
    // leds.setPixelColor(399, 0x151515);

    showLEDsCore2 = 3;
    delayMicroseconds((12));
    }
  // lightUpRail();
  showLEDsCore2 = 1;
  // delay(1000);
  }
void rainbowBounce(int wait, int logo) {
  hsvColor hsv;
  int bounce = 0;
  // leds.clear();
  for (long j = 0; j < 140; j += 1) {

    for (int i = 0; i < LED_COUNT - 9; i++) {
      float huef;
      float i2 = i;
      float j2 = j;

      huef = sinf((i2 / (j2)) * 1.5f); //*(sinf((j2/i2))*19.0);
      // hsv.h = (j*(i*j))%255;
      // hsv.h = (j*(int((sin(j+i)*4))))%254;
      hsv.h = ((int)(huef * 255)) % 254;
      hsv.s = 254;
      if (i < 400) {
        hsv.v = 6;
        } else {
        hsv.v = 16;
        }
      // hsv.v = 6;
      rgbColor rgb = HsvToRgb(hsv);
      uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);
      // rgbPacked = rgbPacked * i
      if (i < 300 && logo == 1) {
        if (jumperlessText[textMap[i]] == 0) {
          leds.setPixelColor(i, rgbPacked);
          } else {
          leds.setPixelColor(i, 0);
          }
        } else {
        leds.setPixelColor(i, rgbPacked);
        }
      }

    showLEDsCore2 = 3;
    delayMicroseconds((wait * 1000) * ((j / 20.0)));
    }
  for (long j = 140; j >= 0; j -= 1) {

    for (int i = 0; i < LED_COUNT - 9; i++) {
      float huef;
      float i2 = i;
      float j2 = j;

      huef = sinf((i2 / (j2)) * 1.5f); //*(sinf((j2/i2))*19.0);
      // hsv.h = (j*(i*j))%255;
      // hsv.h = (j*(int((sin(j+i)*4))))%254;
      hsv.h = ((int)(huef * 255)) % 254;
      hsv.s = 254;
      if (i < 400) {
        hsv.v = 6;
        } else {
        hsv.v = 16;
        }
      rgbColor rgb = HsvToRgb(hsv);
      uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);
      // rgbPacked = rgbPacked * i
      if (i < 300 && logo == 1) {
        if (jumperlessText[textMap[i]] == 0) {
          leds.setPixelColor(i, rgbPacked);
          } else {
          leds.setPixelColor(i, 0);
          }
        } else {
        leds.setPixelColor(i, rgbPacked);
        }
      // leds.setPixelColor(i, rgbPacked);
      }

    showLEDsCore2 = 3;
    delayMicroseconds((wait * 1000) * ((j / 20.0)));
    }
  }

struct rgbColor unpackRgb(uint32_t color) {
  struct rgbColor rgb;
  rgb.r = (color >> 16) & 0xFF;
  rgb.g = (color >> 8) & 0xFF;
  rgb.b = color & 0xFF;
  /* Serial.print("r: ");
   Serial.print(rgb.r);
   Serial.print(" g: ");
   Serial.print(rgb.g);
   Serial.print(" b: ");
   Serial.println(rgb.b);*/
  return rgb;
  }

// uint32_t packRgb(uint8_t r, uint8_t g, uint8_t b) {
//   return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
// }
uint32_t packRgb(rgbColor rgb) {
  return ((uint32_t)rgb.r << 16) | ((uint32_t)rgb.g << 8) | rgb.b;
  }

uint32_t packRgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

void clearLEDs(void) {
  for (int i = 0; i <= 436; i++) { // For each pixel in strip...

    leds.setPixelColor(i, 0); //  Set pixel's color (in RAM)
    //  Update strip to match
    // showLEDsCore2 = 1;
    }
  // lightUpRail();
  //  for (int i = 80; i <= 109; i++)
  //  { // For each pixel in strip...
  //      if (nodesToPixelMap[i] > NANO_D0 && nodesToPixelMap[i] < NANO_A7)
  //      {
  //          leds.setPixelColor(i, 0); //  Set pixel's color (in RAM)
  //      }
  //      // leds.setPixelColor(i, 0); //  Set pixel's color (in RAM)
  //      //  Update strip to match
  //  }

  // showLEDsCore2 = 1;
  }

void clearLEDsExceptRails(void) {
  for (int i = 0; i < 300; i++) {

    leds.setPixelColor(i, 0);
    }

  for (int i = 400; i < 430; i++) {
    if (i != 403 && i != 402 && i != 428 && i != 429 && i != 416 && i != 426 &&
        i != 427) {
      leds.setPixelColor(i, 0);
      }
    }
  // leds.setPixelColor(430, 0x051010);
  }

void clearLEDsMiddle(int start, int end) {
  for (int i = start - 1; i < end; i++) {
    if (i > 60) {
      return;
      }
    leds.setPixelColor(i * 5 + 2, 0);
    }
  }

void clearLEDsExceptMiddle(int start, int end) {

  if (end != -1) {
    for (int i = start; i < end; i++) {

      leds.setPixelColor(i * 5, 0);
      leds.setPixelColor(i * 5 + 1, 0);
      leds.setPixelColor(i * 5 + 3, 0);
      leds.setPixelColor(i * 5 + 4, 0);
      }
    } else {

    if (start > 0 && start <= 60)
      start = start - 1;
    leds.setPixelColor(start * 5, 0);
    leds.setPixelColor(start * 5 + 1, 0);
    leds.setPixelColor(start * 5 + 3, 0);
    leds.setPixelColor(start * 5 + 4, 0);
    }
  }

// char* colorToName(uint32_t color, int length)
// {
//     int numColors = sizeof(colorNames) / sizeof(colorNames[0]);
//     rgbColor input = unpackRgb(color);
//     // Only return black if the color is exactly 0x000000
//     if (color == 0x000000) {
//         const char* black = "black";
//         strncpy(colorNameBuffer, black, strlen(black));
//         colorNameBuffer[strlen(black)] = '\0';
//         return colorNameBuffer;
//     }
//     // Return white if all channels are equal (and not zero)
//     if (input.r == input.g && input.g == input.b && input.r != 0) {
//         const char* white = "white";
//         strncpy(colorNameBuffer, white, strlen(white));
//         colorNameBuffer[strlen(white)] = '\0';
//         return colorNameBuffer;
//     }
//     // Convert input to HSV
//     hsvColor inputHsv = RgbToHsv(input);
//     int minDist = 0x7FFFFFFF;
//     int minIdx = 0;
//     for (int i = 0; i < numColors; i++) {
//         rgbColor refRgb = unpackRgb(colorNames[i].color);
//         hsvColor refHsv = RgbToHsv(refRgb);
//         // Compare hue and saturation only
//         int dh = (int)inputHsv.h - (int)refHsv.h;
//         int ds = (int)inputHsv.s - (int)refHsv.s;
//         int dist = dh * dh + ds * ds;
//         if (dist < minDist) {
//             minDist = dist;
//             minIdx = i;
//         }
//     }
//     const char* src = colorNames[minIdx].name;
//     int len = strlen(src);
//     if (length == -1) {
//         // Trim trailing spaces only
//         int end = len - 1;
//         while (end >= 0 && src[end] == ' ') end--;
//         int trimmedLen = end + 1;
//         strncpy(colorNameBuffer, src, trimmedLen);
//         colorNameBuffer[trimmedLen] = '\0';
//         return colorNameBuffer;
//     } else {
//         int padLen = length > len ? length : len;
//         memset(colorNameBuffer, ' ', padLen);
//         strncpy(colorNameBuffer, src, padLen);
//         colorNameBuffer[padLen] = '\0';
//         return colorNameBuffer;
//     }
// }

// Function to parse range from Serial and display color names
void checkSerialForColorRange() {
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim();

    // Look for a dash character
    int dashIndex = input.indexOf('-');
    if (dashIndex > 0 && dashIndex < input.length() - 1) {
      // Get the numbers before and after the dash
      String startStr = input.substring(0, dashIndex);
      String endStr = input.substring(dashIndex + 1);

      // Convert to integers
      int rangeStart = startStr.toInt();
      int rangeEnd = endStr.toInt();

      // Validate the range
      if (rangeStart >= 0 && rangeStart <= 255 && rangeEnd >= 0 && rangeEnd <= 255) {
        Serial.print("Displaying colors for range: ");
        Serial.print(rangeStart);
        Serial.print("-");
        Serial.println(rangeEnd);

        // Call the demo function with the parsed range
        colorPicker();
        } else {
        Serial.println("Invalid range. Please use format: 0-255 (values between 0-255)");
        }
      } else {
      // No dash found or invalid format
      Serial.println("Invalid format. Please use: start-end (e.g., 0-255)");
      }
    }
  }


