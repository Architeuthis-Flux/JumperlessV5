// SPDX-License-Identifier: MIT
#include "LEDs.h"
#include "FileParsing.h"
#include "Graphics.h"
#include "MatrixStateRP2040.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "Probing.h"
#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
// MATRIX DECLARATION:
// Parameter 1 = width of NeoPixel matrix
// Parameter 2 = height of matrix
// Parameter 3 = pin number (most are valid)
// Parameter 4 = matrix layout flags, add together as needed:
//   NEO_MATRIX_TOP, NEO_MATRIX_BOTTOM, NEO_MATRIX_LEFT, NEO_MATRIX_RIGHT:
//     Position of the FIRST LED in the matrix; pick two, e.g.
//     NEO_MATRIX_TOP + NEO_MATRIX_LEFT for the top-left corner.
//   NEO_MATRIX_ROWS, NEO_MATRIX_COLUMNS: LEDs are arranged in horizontal
//     rows or in vertical columns, respectively; pick one or the other.
//   NEO_MATRIX_PROGRESSIVE, NEO_MATRIX_ZIGZAG: all rows/columns proceed
//     in the same order, or alternate lines reverse direction; pick one.
//   See example below for these values in action.
// Parameter 5 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_GRBW    Pixels are wired for GRBW bitstream (RGB+W NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)

// Example for NeoPixel Shield.  In this application we'd like to use it
// as a 5x8 tall matrix, with the USB port positioned at the top of the
// Arduino.  When held that way, the first pixel is at the top right, and
// lines are arranged in columns, progressive order.  The shield uses
// 800 KHz (v2) pixels that expect GRB color data.

Adafruit_NeoPixel leds(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoMatrix matrix =
    Adafruit_NeoMatrix(30, 5, LED_PIN,
                       NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_COLUMNS +
                           NEO_MATRIX_PROGRESSIVE,
                       NEO_GRB + NEO_KHZ800);

const uint16_t colors[] = {matrix.Color(70, 0, 50), matrix.Color(0, 30, 0),
                           matrix.Color(0, 0, 8)};

rgbColor netColors[MAX_NETS] = {0};

uint8_t saturation = 254;
volatile uint8_t LEDbrightness = DEFAULTBRIGHTNESS;
volatile uint8_t LEDbrightnessRail = DEFAULTRAILBRIGHTNESS;
volatile uint8_t LEDbrightnessSpecial = DEFAULTSPECIALNETBRIGHTNESS;

int showLEDsCore2 = 0;

int netNumberC2 = 0;
int onOffC2 = 0;
int nodeC2 = 0;
int brightnessC2 = 0;
int hueShiftC2 = 0;
int lightUpNetCore2 = 0;

int logoFlash = 0;

#ifdef EEPROMSTUFF
#include <EEPROM.h>
bool debugLEDs = EEPROM.read(DEBUG_LEDSADDRESS);

#else
bool debugLEDs = 1;
#endif

uint32_t rawSpecialNetColors[8] = // dim
    {0x000000, 0x001C04, 0x1C0702, 0x1C0107,
     0x231111, 0x230913, 0x232323, 0x232323};

uint32_t rawOtherColors[12] = {
    0x010006, // headerglow
    0x6000A8, // logo / status
    0x0055AA, // logoflash / statusflash
    0x301A02, // +8V
    0x120932, // -8V
    0x443434, // UART TX
    0x324244, // UART RX
    0x232323,
    0x380303, // ADC
    0x253803, // DAC
    0x0000E5, // GPIO
};

uint32_t logoColors[43] = {
    0x800058, 0x750053, 0x700068, 0x650063, 0x600078, 0x550073, 0x500088,
    0x450083, 0x400098, 0x350093, 0x3000A8, 0x2500A3, 0x2000B8, 0x1500B3,
    0x1000C8, 0x0502C3, 0x0204D8, 0x0007E3, 0x0010E8, 0x0015F3, 0x0020F8,
    0x0025FA, 0x0030FF, 0x0035E0, 0x0240BF, 0x0545A0, 0x10509F, 0x15558F,
    0x20607F, 0x25656F, 0x30705F, 0x35754F, 0x40803F, 0x45722F, 0x506518,
    0x55481A, 0x603A2A, 0x653332, 0x702538, 0x751948, 0x791052, 0x7E0562,

};
uint32_t logoColorsConnect[43] = {
    0xff2200, 0xee1133, 0xee0055, 0xdd0066, 0xcc0077, 0xaa0088, 0x880099,
    0x660099, 0x551188, 0x444499, 0x3355AA, 0x4466bb, 0x5577cc, 0x4477DD,
    0x3355dd, 0x4422dd, 0x5500ee, 0x4400ee, 0x4400dd, 0x5500cc, 0x6600bb,
    0x7700aa, 0x880099, 0x990088, 0xaa0077, 0xbb0066, 0xcc0077, 0xcc0088,
    0xdd0077, 0xdd0066, 0xdd0044, 0xee2222, 0xee4411, 0xee5500, 0xee6600,
    0xdd7711, 0xee8811, 0xff7711, 0xff5500, 0xff5500, 0xff4400, 0xff3300,

};

uint32_t logoColorsClear[43] = {
    0x5500ff, 0x4400ff, 0x3311ff, 0x2233ff, 0x1144ff, 0x0055ff, 0x0088ee,
    0x0066dd, 0x0077aa, 0x009999, 0x118866, 0x339933, 0x44aa22, 0x669922,
    0x559944, 0x227766, 0x009988, 0x00aa88, 0x00aa77, 0x009966, 0x007777,
    0x005588, 0x0033aa, 0x0033dd, 0x0011ff, 0x0000ff, 0x0022ff, 0x1122ff,
    0x2222ff, 0x3311ee, 0x4411ee, 0x4400ee, 0x5500ee, 0x6600ee, 0x6600dd,
    0x7700ee, 0xdd00ff, 0xff00ff, 0xee00ff, 0xaa00ff, 0x9900ff, 0x6600ff,

};
rgbColor specialNetColors[8] = {{00, 00, 00},       {0x00, 0xFF, 0x30},
                                {0xFF, 0x41, 0x14}, {0xFF, 0x10, 0x40},
                                {0xeF, 0x78, 0x7a}, {0xeF, 0x40, 0x7f},
                                {0xFF, 0xff, 0xff}, {0xff, 0xFF, 0xff}};

rgbColor railColors[4] = {{0xFF, 0x32, 0x30},
                          {0x00, 0xFF, 0x30},
                          {0xFF, 0x32, 0x30},
                          {0x00, 0xFF, 0x30}};

uint32_t rawRailColors[3][4] = // depends on supplySwitchPosition 0 = 3.3V, 1 =
                               // 5V, 2 = +-8V
    {{0x1C0110, 0x001C04, 0x1C0110, 0x001C04},

     {0x210904, 0x001C04, 0x210904, 0x001C04},

     {0x301A02, 0x001C04, 0x120932, 0x001C04}};

int x = matrix.width();
int pass = 0;

void initLEDs(void) {
  debugLEDs = EEPROM.read(DEBUG_LEDSADDRESS);
  if (debugLEDs != 0 && debugLEDs != 1) {
    debugLEDs = 1;
  }
  EEPROM.write(DEBUG_LEDSADDRESS, debugLEDs);

  pinMode(LED_PIN, OUTPUT);
  // delay(1);
  leds.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
                // delay(1);
  // leds.show();
  // delay(2);
  //  leds.setBrightness(100);
  // delay(2);
  EEPROM.commit();
  // delay(20);
}

uint32_t savedLEDcolors[NUM_SLOTS][LED_COUNT + 1];

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

int saveRawColors(int slot) {

  // if (savedLEDcolors[slot][LED_COUNT] == 0xFFFFFF) // put this to say it was
  // already saved
  // {
  //     return 0;
  // }

  if (slot == -1) {
    slot = netSlot;
  }

  for (int i = 0; i < 300; i++) {
    if (i >= slotLEDpositions[0] && i <= slotLEDpositions[NUM_SLOTS - 1]) {
      // savedLEDcolors[slot][i] = slotSelectionColors[1];
      //  Serial.print(i);
      //  Serial.print("\t");

      continue;
    }
    savedLEDcolors[slot][i] = leds.getPixelColor(i);
  }
  savedLEDcolors[slot][LED_COUNT] = 0xAAAAAA;
  return 1;
}

void refreshSavedColors(int slot) {
  if (slot == -1) {
    for (int i = 0; i < NUM_SLOTS; i++) {
      savedLEDcolors[i][LED_COUNT] = 0x000000;
    }
  } else {
    savedLEDcolors[slot][LED_COUNT] = 0x000000;
  }
}

void showSavedColors(int slot) {
  if (slot == -1) {
    slot = netSlot;
  }
  // Serial.println(savedLEDcolors[slot][0], HEX);

  // if (savedLEDcolors[slot][110] == 0) // checking a nano header LED because
  // it should always dimly lit
  // if (savedLEDcolors[slot][LED_COUNT] != 0xAAAAAA) // put this to say it was
  // already saved
  if (1) {
    // Serial.println("saving colors\n\r");
    clearAllNTCC();
    openNodeFile(slot);
    // printNodeFile(slot, 0);
    // clearLEDs();
    clearLEDsExceptRails();
    getNodesToConnect();
    bridgesToPaths();
    // leds.clear();
    clearLEDsExceptRails();

    assignNetColors();

    saveRawColors(slot);

  } else {
    // Serial.println("using saved colors\n\r");
    //  leds.clear();
    //  Serial.println(leds.getPixelColor(0), HEX);
    for (int i = 0; i < LED_COUNT; i++) {
      // if (i >= slotLEDpositions[0] && i <= slotLEDpositions[NUM_SLOTS - 1])
      // {
      //     // savedLEDcolors[slot][i] = slotSelectionColors[1];
      //     //  Serial.print(i);
      //     //  Serial.print("\t");

      //     continue;
      // }
      leds.setPixelColor(i, savedLEDcolors[slot][i]);
    }
    // Serial.println(leds.getPixelColor(3), HEX);
  }

  showLEDsCore2 = 1;
  // leds.show();
}

void assignNetColors(void) {
  // numberOfNets = 60;

  uint16_t colorDistance = (255 / (numberOfNets - 8));

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
  logoFlash = 2;
  // showLEDsCore2 = 1;
  if (debugLEDs) {
    Serial.print("\n\rcolorDistance: ");
    Serial.print(colorDistance);
    Serial.print("\n\r");
    Serial.print("numberOfNets: ");
    Serial.print(numberOfNets);
    Serial.print("\n\rassigning net colors\n\r");
    Serial.print("\n\rNet\t\tR\tG\tB\t\tH\tS\tV");
    delay(6);
  }

  for (int i = 1; i < 8; i++) {
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
      } else if (i >= 4 && i <= 7) {
        netHsv.v = LEDbrightnessSpecial;
      }

      rgbColor netRgb = HsvToRgb(netHsv);

      specialNetColors[i] = netRgb;

      netColors[i] = specialNetColors[i];
      net[i].color = netColors[i];
    }

    if (debugLEDs) {
      Serial.print("\n\r");
      int netLength = Serial.print(net[i].name);
      if (netLength < 8) {
        Serial.print("\t");
      }
      Serial.print("\t");
      Serial.print(net[i].color.r, HEX);
      Serial.print("\t");
      Serial.print(net[i].color.g, HEX);
      Serial.print("\t");
      Serial.print(net[i].color.b, HEX);
      Serial.print("\t\t");
      // Serial.print(netHsv.h);
      Serial.print("\t");
      // Serial.print(netHsv.s);
      Serial.print("\t");
      // Serial.print(netHsv.v);
      delay(10);
    }
    //
  }

  int skipSpecialColors = 0;
  uint8_t hue = 1;

  int colorSlots[60] = {-1};
  int netColorMode = 1; // 0 = rainbow, 1 = shuffle
  int colorSlots1[20] = {-1};
  int colorSlots2[20] = {-1};
  int colorSlots3[20] = {-1};

  if (numberOfNets < 60) {
    for (int i = 0; i < numberOfNets - 8; i++) {

      colorSlots[i] = abs(230 - ((((i)*colorDistance)))) % 254;
    }
    if (netColorMode == 1) {
      int backIndex = numberOfNets - 9;

      int index1 = 0;
      int index2 = 0;
      int index3 = 0;
      //   int third = (numberOfNets - 8) / 3;

      for (int i = 0; i < (numberOfNets - 8); i++) {
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
      if (debugLEDs) {
        Serial.print("number of nets - special nets: ");
        Serial.println(numberOfNets - 8);
        Serial.println();
        for (int i = 0; i < index1; i++) {
          Serial.print(colorSlots1[i]);
          Serial.print(" ");
        }
        Serial.println();
        for (int i = 0; i < index2; i++) {
          // colorSlots2[i] = colorSlots2[(i + third) % index2];
          Serial.print(colorSlots2[i]);
          Serial.print(" ");
        }

        Serial.println();
        for (int i = 0; i < index3; i++) {
          Serial.print(colorSlots3[i]);
          Serial.print(" ");
        }
        Serial.println();
      }
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
      int loop2index = index2 / 2;
      int loop3index = index3 - 1;

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
  }
  if (debugLEDs) {
    for (int i = 0; i < numberOfNets - 8; i++) {

      Serial.print(colorSlots[i]);
      Serial.print(" ");
    }
  }
  int frontIndex = 0;
  for (int i = 8; i < numberOfNets; i++) {
    // if(net[i].nodes[0] == -1){
    //     continue;
    // }

    if (net[i].machine == true) {
      // Serial.println("number of nets: ");
      // Serial.println(numberOfNets);
      // rgbColor specialNetRgb = unpackRgb(rawSpecialNetColors[i]);

      // net[i].color = specialNetRgb;
      // specialNetColors[i] = specialNetRgb;

      // netColors[i] = specialNetRgb;
      // continue;
      // leds.setPixelColor(nodesToPixelMap[i], net[i].rawColor);
      netColors[i] = unpackRgb(net[i].rawColor);
    } else {

      uint8_t r = 0;
      uint8_t g = 0;
      uint8_t b = 0;

      int foundColor = 0;

      // if (i%2 == 0) {
      hue = colorSlots[frontIndex];
      frontIndex++;
      // hue = (((i - 8) * colorDistance) - (colorDistance / 4)) % 255;
      //    } else {
      //      hue = 255-((i - 8) * colorDistance);
      //    }

      hsvColor netHsv = {hue, 254, LEDbrightness};
      // netHsv.v = 200;
      netColors[i] = HsvToRgb(netHsv);

      // leds.setPixelColor(i, netColors[i]);

      net[i].color.r = netColors[i].r;
      net[i].color.g = netColors[i].g;
      net[i].color.b = netColors[i].b;
      if (debugLEDs) {
        Serial.print("\n\r");
        Serial.print(net[i].name);
        Serial.print("\t\t");
        Serial.print(net[i].color.r, DEC);
        Serial.print("\t");
        Serial.print(net[i].color.g, DEC);
        Serial.print("\t");
        Serial.print(net[i].color.b, DEC);
        Serial.print("\t\t");
        Serial.print(hue);
        Serial.print("\t");
        Serial.print(saturation);
        Serial.print("\t");
        Serial.print(LEDbrightness);
        delay(3);
      }
    }
  }
  // logoFlash = 0;
}

uint32_t packRgb(uint8_t r, uint8_t g, uint8_t b) {
  return (uint32_t)r << 16 | (uint32_t)g << 8 | b;
}

void lightUpNet(int netNumber, int node, int onOff, int brightness2,
                int hueShift, int dontClear) {
  uint32_t color;
  int pcbExtinction = 0;
  int colorCorrection = 0;
  int pcbHueShift = 0;
  // Serial.print("netNumber: ");
  // Serial.print(netNumber);

  //     Serial.print(" node: ");
  //     Serial.print(node);
  //     Serial.print(" onOff: ");
  //     Serial.print(onOff);
  if (net[netNumber].nodes[1] != 0 &&
      net[netNumber].nodes[1] <= 127) { // NANO_A7) {

    for (int j = 0; j < MAX_NODES; j++) {
      if (net[netNumber].nodes[j] <= 0) {
        break;
      }

      if (net[netNumber].machine == true) {

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
            // leds.setPixelColor(nodesToPixelMap[net[netNumber].nodes[j]],
            // net[netNumber].color.r, net[netNumber].color.g,
            // net[netNumber].color.b);
          } else {
            // Serial.print("net: ");
            // Serial.print(netNumber);
            // Serial.print("  onOff:  ");
            // Serial.println(onOff);

            // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]]) *
            // 5
            // + 0, 0);
            // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]]) *
            // 5
            // + 1, 0);
            // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]]) *
            // 5
            // + 2, 0);
            // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]]) *
            // 5
            // + 3, 0);
            // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]]) *
            // 5
            // + 4, 0);
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
                pcbExtinction = PCBEXTINCTION;

                // Serial.println (brightness2);
                // hueShift += PCBHUESHIFT;
                // colorCorrection = 1;
              }
              // pcbExtinction += (brightness2-DEFAULTBRIGHTNESS);

              struct rgbColor colorToShift = {net[netNumber].color.r,
                                              net[netNumber].color.g,
                                              net[netNumber].color.b};

              struct rgbColor shiftedColor =
                  shiftHue(colorToShift, hueShift, pcbExtinction, 254);

              if (colorCorrection != 0) {
                shiftedColor = pcbColorCorrect(shiftedColor);
              }

              hsvColor shiftedColorHsv = RgbToHsv(shiftedColor);

              if (net[netNumber].specialFunction >= 100 &&
                  net[netNumber].specialFunction <= 105) {
                if (brightness2 != DEFAULTBRIGHTNESS) {
                  shiftedColorHsv.v = brightness2;
                } else {
                  shiftedColorHsv.v = LEDbrightnessRail;
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
                if (brightness2 != DEFAULTBRIGHTNESS) {
                  shiftedColorHsv.v = brightness2;
                } else {
                  shiftedColorHsv.v = LEDbrightnessSpecial;
                }

                shiftedColor = HsvToRgb(shiftedColorHsv);

                color = packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);
                // color = packRgb((shiftedColor.r * LEDbrightnessSpecial) >>
                // 8, (shiftedColor.g * LEDbrightnessSpecial) >> 8,
                // (shiftedColor.b
                // * LEDbrightnessSpecial) >> 8);
              } else {
                if (brightness2 != DEFAULTBRIGHTNESS) {
                  shiftedColorHsv.v = brightness2;
                } else {
                  shiftedColorHsv.v = LEDbrightness;
                }

                shiftedColor = HsvToRgb(shiftedColorHsv);

                color = packRgb(shiftedColor.r, shiftedColor.g, shiftedColor.b);

                // color = packRgb((shiftedColor.r * LEDbrightness) >> 8,
                // (shiftedColor.g * LEDbrightness) >> 8, (shiftedColor.b *
                // LEDbrightness) >> 8);
              }

              if (net[netNumber].nodes[j] >= NANO_D0) {
                rgbColor colorToShift = unpackRgb(color);
                // colorToShift = shiftHue(colorToShift, hueShift);
                hsvColor brighterColor = RgbToHsv(colorToShift);
                brighterColor.v += PCBEXTINCTION;
                rgbColor bright = HsvToRgb(brighterColor);

                color = packRgb(bright.r, bright.g, bright.b);
              }
              netColors[netNumber] = unpackRgb(color);
              net[netNumber].rawColor = color;
              if (net[netNumber].nodes[j] >= NANO_D0) {
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) + 320, color);
              } else {
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 0, color);
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 1, color);
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 2, color);
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 3, color);
                leds.setPixelColor(
                    (nodesToPixelMap[net[netNumber].nodes[j]]) * 5 + 4, color);
              }

              if (debugLEDs) {
                Serial.print("net: ");
                Serial.print(netNumber);
                Serial.print(" node: ");
                Serial.print(net[netNumber].nodes[j]);
                Serial.print(" mapped to LED:");

                Serial.println(nodesToPixelMap[net[netNumber].nodes[j]]);
              }
            } else {
              // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]])
              // * 5 + 0, 0);
              // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]])
              // * 5 + 1, 0);
              // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]])
              // * 5 + 2, 0);
              // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]])
              // * 5 + 3, 0);
              // leds.setPixelColor((nodesToPixelMap[net[netNumber].nodes[j]])
              // * 5 + 4, 0);
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
  // showLEDsCore2 = 1;
}

void turnOffSkippedNodes(void) {
  return;

  for (int i = 0; i < numberOfPaths; i++) {

    if (path[i].skip == true) {
      leds.setPixelColor(nodesToPixelMap[path[i].node1] + 0, 0);
      // leds.setPixelColor(nodesToPixelMap[path[i].node1]+1, 0);
      leds.setPixelColor(nodesToPixelMap[path[i].node1] + 2, 0);
      // leds.setPixelColor(nodesToPixelMap[path[i].node1]+3, 0);
      leds.setPixelColor(nodesToPixelMap[path[i].node1] + 4, 0);

      leds.setPixelColor(nodesToPixelMap[path[i].node2], 0);
      // leds.setPixelColor(nodesToPixelMap[path[i].node2]+1, 0);
      leds.setPixelColor(nodesToPixelMap[path[i].node2] + 2, 0);
      // leds.setPixelColor(nodesToPixelMap[path[i].node2]+3, 0);
      leds.setPixelColor(nodesToPixelMap[path[i].node2] + 4, 0);
    }
  }
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

struct rgbColor pcbColorCorrect(rgbColor colorToShift) {

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

void lightUpNode(int node, uint32_t color) {

  leds.setPixelColor(nodesToPixelMap[node], color);
  showLEDsCore2 = 1;
}
uint32_t dimLogoColor(uint32_t color, int brightness) {
  rgbColor dimColor = unpackRgb(color);
  // if (dimColor.b != 0)
  // {
  //     dimColor.b = dimColor.b * 2;
  // }

  hsvColor colorHsv = RgbToHsv(dimColor);

  colorHsv.v = brightness;
  return packRgb(HsvToRgb(colorHsv).r, HsvToRgb(colorHsv).g * 2,
                 HsvToRgb(colorHsv).b * 3);
}

void logoSwirl(int start, int spread, int probe) {
  if (probe == 1) {
    if (connectOrClearProbe == 0) {

      leds.setPixelColor(436, dimLogoColor(logoColorsConnect[start % 42]));
      leds.setPixelColor(
          437, dimLogoColor(logoColorsConnect[(start + (spread)) % 42]));
      leds.setPixelColor(
          438, dimLogoColor(logoColorsConnect[(start + (spread * 2)) % 42]));
      leds.setPixelColor(
          439, dimLogoColor(logoColorsConnect[(start + (spread * 3)) % 42]));
      leds.setPixelColor(
          440, dimLogoColor(logoColorsConnect[(start + (spread * 4)) % 42]));
      leds.setPixelColor(
          441, dimLogoColor(logoColorsConnect[(start + (spread * 5)) % 42]));
      leds.setPixelColor(
          442, dimLogoColor(logoColorsConnect[(start + (spread * 6)) % 42]));
      leds.setPixelColor(
          443, dimLogoColor(logoColorsConnect[(start + (spread * 7)) % 42]));
    } else {
      leds.setPixelColor(436, dimLogoColor(logoColorsClear[start % 42]));
      leds.setPixelColor(
          437, dimLogoColor(logoColorsClear[(start + (spread)) % 42]));
      leds.setPixelColor(
          438, dimLogoColor(logoColorsClear[(start + (spread * 2)) % 42]));
      leds.setPixelColor(
          439, dimLogoColor(logoColorsClear[(start + (spread * 3)) % 42]));
      leds.setPixelColor(
          440, dimLogoColor(logoColorsClear[(start + (spread * 4)) % 42]));
      leds.setPixelColor(
          441, dimLogoColor(logoColorsClear[(start + (spread * 5)) % 42]));
      leds.setPixelColor(
          442, dimLogoColor(logoColorsClear[(start + (spread * 6)) % 42]));
      leds.setPixelColor(
          443, dimLogoColor(logoColorsClear[(start + (spread * 7)) % 42]));
    }

  } else {
    leds.setPixelColor(436, dimLogoColor(logoColors[start % 42]));
    leds.setPixelColor(437, dimLogoColor(logoColors[(start + (spread)) % 42]));
    leds.setPixelColor(438,
                       dimLogoColor(logoColors[(start + (spread * 2)) % 42]));
    leds.setPixelColor(439,
                       dimLogoColor(logoColors[(start + (spread * 3)) % 42]));
    leds.setPixelColor(440,
                       dimLogoColor(logoColors[(start + (spread * 4)) % 42]));
    leds.setPixelColor(441,
                       dimLogoColor(logoColors[(start + (spread * 5)) % 42]));
    leds.setPixelColor(442,
                       dimLogoColor(logoColors[(start + (spread * 6)) % 42]));
    leds.setPixelColor(443,
                       dimLogoColor(logoColors[(start + (spread * 7)) % 42]));
  }
  // Serial.println(start % 42);
  // Serial.println((start + 5) % 42);
  // Serial.println((start + 10) % 42);
  // Serial.println((start + 15) % 42);
  // Serial.println((start + 20) % 42);
  // Serial.println((start + 25) % 42);
  // Serial.println((start + 30) % 42);
  // Serial.println((start + 35) % 42);
  // Serial.println("\n\r");
  // leds.show();
  // delay(200);
  //  showLEDsCore2 = 1;
}

void lightUpRail(int logo, int rail, int onOff, int brightness2,
                 int switchPosition) {
  /*
  brightness2 = (uint8_t)LEDbrightnessRail;
  Serial.print("\n\rbrightness2: ");
  Serial.print(brightness2);
  Serial.print("\n\r");
  Serial.print("\n\rled brightness: ");
  Serial.print(LEDbrightness);
*/

  brightness2 = LEDbrightnessRail;

  if (logo == -1 && logoFlash == 0) {
    // leds.setPixelColor(436, rawOtherColors[1]);
    // leds.setPixelColor(437, rawOtherColors[1]);
    // leds.setPixelColor(438, rawOtherColors[1]);
    // leds.setPixelColor(439, rawOtherColors[1]);
    // leds.setPixelColor(440, rawOtherColors[1]);
    // leds.setPixelColor(441, rawOtherColors[1]);
    // leds.setPixelColor(442, rawOtherColors[1]);
    // leds.setPixelColor(443, rawOtherColors[1]);
    // Serial.println(RgbToHsv(unpackRgb(0x550008)).v);
  }

  //   for (int i = 400; i <= 429; i++) {
  //     if (leds.getPixelColor(i) == 0 &&
  //         leds.getPixelColor(i) != rawOtherColors[0]) {
  //       leds.setPixelColor(i, rawOtherColors[0]);
  //     }
  //   }

  leds.setPixelColor(403, scaleDownBrightness(rawSpecialNetColors[1], 5, 45));
  leds.setPixelColor(428, scaleDownBrightness(rawSpecialNetColors[1], 5, 45));
  leds.setPixelColor(429, scaleDownBrightness(rawSpecialNetColors[2], 5, 45));
  leds.setPixelColor(416, scaleDownBrightness(rawSpecialNetColors[3], 5, 45));
  leds.setPixelColor(426, scaleDownBrightness(rawSpecialNetColors[2], 5, 45));

  if (sfProbeMenu == 1) {
    leds.setPixelColor(430, scaleDownBrightness(rawOtherColors[8], 2, 45));
    leds.setPixelColor(431, scaleDownBrightness(rawOtherColors[8], 2, 45));
    leds.setPixelColor(432, 0);
    leds.setPixelColor(433, 0);
    leds.setPixelColor(434, 0);
    leds.setPixelColor(435, 0);

  } else if (sfProbeMenu == 2) {
    leds.setPixelColor(430, 0);
    leds.setPixelColor(431, 0);

    leds.setPixelColor(432, scaleDownBrightness(rawOtherColors[9], 5, 45));
    leds.setPixelColor(433, scaleDownBrightness(rawOtherColors[9], 5, 45));

    leds.setPixelColor(434, 0);
    leds.setPixelColor(435, 0);
  } else if (sfProbeMenu == 3) {
    leds.setPixelColor(430, 0);
    leds.setPixelColor(431, 0);
    leds.setPixelColor(432, 0);
    leds.setPixelColor(433, 0);
    leds.setPixelColor(434, scaleDownBrightness(rawOtherColors[10], 2, 45));
    leds.setPixelColor(435, scaleDownBrightness(rawOtherColors[10], 2, 45));

  } else {
    leds.setPixelColor(430, scaleDownBrightness(rawOtherColors[8], 2, 45));
    leds.setPixelColor(431, scaleDownBrightness(rawOtherColors[8], 2, 45));
    leds.setPixelColor(432, scaleDownBrightness(rawOtherColors[9], 5, 45));
    leds.setPixelColor(433, scaleDownBrightness(rawOtherColors[9], 5, 45));
    leds.setPixelColor(434, scaleDownBrightness(rawOtherColors[10], 2, 45));
    leds.setPixelColor(435, scaleDownBrightness(rawOtherColors[10], 2, 45));
  }
  if (switchPosition == 2) //+-8V
  {
    rawRailColors[switchPosition][0] = scaleDownBrightness(rawOtherColors[3]);
    rawRailColors[switchPosition][2] = scaleDownBrightness(rawOtherColors[4]);
  }
  for (int j = 0; j < 4; j++) {
    if (j == rail || rail == -1) {
      // rgbColor rgbRail = railColors[j];
      // hsvColor hsvRail = RgbToHsv(rgbRail);
      // hsvRail.v = brightness2;
      // Serial.println (rawOtherColors[0], HEX);
      // rgbRail = HsvToRgb(hsvRail);
      // Serial.println (hsvRail.h);
      // Serial.println (hsvRail.s);
      // Serial.println (hsvRail.v);
      // Serial.println ("\n\r");

      uint32_t color = rawRailColors[switchPosition][j];
      for (int i = 0; i < 25; i++) {

        if (onOff == 1) {
          // uint32_t color = packRgb((railColors[j].r * brightness2) >> 8,
          // (railColors[j].g * brightness2) >> 8, (railColors[j].b *
          // brightness2) >> 8);

          /// Serial.println(color,HEX);
          leds.setPixelColor(railsToPixelMap[j][i],
                             scaleDownBrightness(color, 5, 25));
        } else {
          leds.setPixelColor(railsToPixelMap[j][i], 0);
        }
      }
    }
  }
  // leds.show();
  // showLEDsCore2 = 1;
  // delay(3);
}
int displayMode = 0; // 0 = lines 1= wires

void showNets(void) {

  if (displayMode == 0) {
    for (int i = 0; i <= numberOfNets; i++) {
      // Serial.print(i);

      lightUpNet(i);
    }

  } else if (displayMode == 1) {
    drawWires();
  }
  // showLEDsCore2 = 1;
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

void randomColors(uint32_t color, int wait) {

  int count = 0;

  for (int i = 0; i < leds.numPixels(); i++) {

    count = random(0, 10);
    // if (i > 80)
    // {
    //     count = random(0, 22);
    // }

    byte colorValR = random(2, 0x10);
    byte colorValG = random(2, 0x10);
    byte colorValB = random(2, 0x10);

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
    default:
      color = color & 0x000000;
      break;
    }
    // color = color | (color >> 1);

    leds.setPixelColor(i, color); //  Set pixel's color (in RAM)
    // lightUpRail(-1, -1, 1, LEDbrightnessRail);
    showLEDsCore2 = 2; //  Update strip to match
                       //  Pause for a moment
  }
  // delay(500);
  delay(wait);
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
  clearLEDs();
  // lightUpRail();
  //  showLEDsCore2 = 1;
}

uint32_t chillinColors[LED_COUNT] = {
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
    0x050E03, 0x07000F, 0x030015, 0x000000};

void startupColorsV5(void) {
  int logo = 1;

  hsvColor hsv;
  int bounce = 0;
  // leds.clear();
  //   for (long j = 150; j < 340; j += 1) {

  //     for (int i = 0; i < LED_COUNT - 9; i++) {
  //       float huef;
  //       float i2 = i;
  //       float j2 = j;

  //       huef = sinf((i2 / (j2)) * 1.5f); //*(sinf((j2/i2))*19.0);
  //       // hsv.h = (j*(i*j))%255;
  //       // hsv.h = (j*(int((sin(j+i)*4))))%254;
  //       hsv.h = ((int)(huef * 255)) % 254;
  //       hsv.s = 254;
  //       if (i < 400) {
  //         hsv.v = 6;
  //       } else {
  //         hsv.v = 16;
  //       }
  //       // hsv.v = 6;
  //       rgbColor rgb = HsvToRgb(hsv);
  //       uint32_t rgbPacked = packRgb(rgb.r, rgb.g, rgb.b);
  //       // rgbPacked = rgbPacked * i
  //       if (i < 300 && logo == 1) {
  //         if (jumperlessText[textMap[i]] == 0) {
  //           leds.setPixelColor(i, rgbPacked);
  //         } else {
  //           leds.setPixelColor(i, 0);
  //         }
  //       } else {
  //         leds.setPixelColor(i, rgbPacked);
  //       }
  //     }

  //     showLEDsCore2 = 3;
  //     delayMicroseconds(500);
  //   }
  //   //delay(1000);
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
void clearLEDs(void) {
  for (int i = 0; i <= 445; i++) { // For each pixel in strip...

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
    if (i != 403 && i != 428 && i != 429 && i != 416 && i != 426) {
      leds.setPixelColor(i, 0);
    }
  }
  // leds.setPixelColor(430, 0x051010);
}
