[![arduino-library-badge](https://www.ardu-badge.com/badge/JDI_MIP_Display.svg?)](https://www.ardu-badge.com/JDI_MIP_Display)

# Japan Display Inc. library for Arduino
Arduino library to drive the Memory In Pixel Displays produced by Japan Display Inc.  

The library should also work with the Memory In Pixel Displays produced by Sharp (such as **LS027B7DH01** or **LS044Q7DH01**) because they seem to share the same protocol. The only difference with the JDI Displays is that the latter are 8 bit colors while the Sharp ones are monochrome. 

Please note that the library has been tested only with the JDI display model **LPM027M128B** as well as the models listed in [Other supported displays section](#other-supported-displays). If you have a different display model feel free to test the library and send PRs.  

Reflective displays are readable under direct sunlight and require less power because in bright environments they don't need a backlight! However if you also want to use them in dark environments you may need a display with a backlight panel (like the model **LPM027M128C** or **LPM013M126C**).

There is also a different kind of display lighting produced by Azumo: they apply a special thin and transparent film to the front. Check out all the [Azumo displays](https://www.azumotech.com/products/). Some of them are based on Sharp display, others on JDI display. [This](https://www.azumotech.com/products/2-7-color-display-12380-06/) is the exact display which I bought and used to test the library.

Based on [Tadayuki Okamoto's library](https://os.mbed.com/users/KURETA90/code/ColorMemLCD/).

***

# Breakout board

Both the Sharp and JDI Displays are ultra slim and compact. They have a 10 pins FPC connector which can be difficult to handle. Some of them have an extra 2 pin connector for the back/front light panel. I have designed a [Breakout board](https://github.com/Gbertaz/JDI_MIP_Display/blob/master/images/breakout_v1.0.png) to make it easy to test the display and turn on/off the light.  

If you want to make the same test board please check the [Schematic, BOM and Gerber](https://github.com/Gbertaz/JDI_MIP_Display/tree/master/breakout/v1.0) files.

Please note that the Breakout board version 1.0 is specifically designed for the JDI MIP Display model **LPM027M128B** so the position of the FPC connectors might change according to the display model.  

This breakout has the following issues:

1) R3 in the schematic is 839 Ohm but it is too big therefore the light is weak. I have replaced it by soldering a smaller resistor (27 Ohm).
2) The display is powered from 3.3v which is also used to power the frontlight. However there should be a different 5v source to power the frontlight.
3) Capacitors C1 and C2 according to the display datasheet should be 0.1uF. They were out of stock when I ordered the board, I then used 100nF instead which should not be a problem but keep that in mind.


![BreakoutImage](https://github.com/Gbertaz/JDI_MIP_Display/blob/master/images/breakout_v1.0_display.jpg)

Here is a short [Demonstration video](https://twitter.com/NotTheWorstDev/status/1467655365672292356?s=20) of the display in action.

# Wiring


| PIN | Description  | Connect to |
| ------------- | ------------- | ------------- |
| SCLK | SPI Serial Clock Signal | Connect to MCU SPI Clock |
| SI/MOSI | SPI Serial Data Input Signal also known as MOSI (Master Out Slave In) | Connect to MCU SPI MOSI |
| SCS | SPI Chip Select Signal | Connect to MCU SPI Chip select pin |
| EXTCOMIN | COM Inversion Signal Input | Not connected |
| DISP | Display ON/OFF Switching Signal | Connect to any digital pin |
| EXTMODE | COM Inversion Mode Select Terminal | Not connected |
| FRONTLIGHT | Frontlight ON/OFF Switching Signal | Connect to any digital pin |

### Examples

| PIN | Teensy 4.1 | Esp8266 | Esp32 |
| ------------- | ------------- | ------------- | ------------- |
| SCLK | 13 | D5 GPIO14 | 18 |
| SI/MOSI | 11 | D7 GPIO13 | 23 |
| SCS | 10 | D8 GPIO15 | 5 |
| EXTCOMIN | Not connected | Not connected | Not connected |
| DISP | 22 | D2 GPIO4 | 21 |
| EXTMODE | Not connected | Not connected | Not connected |
| FRONTLIGHT | 23 | D1 GPIO5 | 22 |

***

# Prerequisites

This library depends on [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library) and [Arduino SPI](https://github.com/arduino/ArduinoCore-avr/tree/master/libraries/SPI) libraries. So make sure to install those first.

# Memory requirements

In order to maximize the number of Frame Per Second, the library allocates 2 buffers to efficiently draw only the lines that change between two screen refresh. **Depending on the display size you may not have enough memory on the microcontroller.**  

For example for a 400x240 pixel display it is necessary to allocate ~107Kb of dynamic memory. If your microcontroller (for example Esp8266) doesn't have such memory you can disable the different lines update (using only one buffer) by commenting the following line in JDI_MIP_Display.h:

```
#define DIFF_LINE_UPDATE
```

**By commenting that line the display will be fully updated at every refresh.** The FPS will drammatically drop but the required memory will be ~62Kb (400x240 pixel display).

# Installation

The library is available from the Arduino Library Manager: load the Arduino IDE, then use the menu at the top to select Sketch -> Include Library -> Manage Libraries. Type **JDI_MIP_Display** in the search box.

Click the following badge for a complete installation guide

[![arduino-library-badge](https://www.ardu-badge.com/badge/JDI_MIP_Display.svg?)](https://www.ardu-badge.com/JDI_MIP_Display)

# Usage

### Step 1

Configure the pins according to your wiring scheme by opening [Display_cfg.h](https://github.com/Gbertaz/JDI_MIP_Display/blob/master/Display_cfg.h) and define the following:

```
#define PIN_SCS         10          // SPI Chip Select Signal pin
#define PIN_DISP        22          // Display ON/OFF Switching Signal pin
#define PIN_FRONTLIGHT  23          // Frontlight pin. Optional depending on the display model
```

Also define the width and height in pixel according to your display model:

```
#define DISPLAY_WIDTH   400         // Display width in pixel
#define DISPLAY_HEIGHT  240         // Display height in pixel
```

### Step 2

Include the library:

```
#include <JDI_MIP_Display.h>
```

### Step 3

Create the instance of the display class:

```
JDI_MIP_Display jdi_display;
```

### Step 4

Initialize the display by calling the *begin* function which initializes the buffers and SPI interface. Then turn the display on. Depending on the display model you can also turn the frontlight on or off.

```
jdi_display.begin();
delay(50);
jdi_display.displayOn();
jdi_display.frontlightOn();   // Optional depending on the display model
```

### Step 5

You can now use all the functions defined in the [Adafruit_GFX](https://github.com/adafruit/Adafruit-GFX-Library) library to draw shapes, text, bitmaps and so on. Please refer to the Adrafruit library documentation.  

Keep in mind that all the drawing functions like *fillCircle*, *drawCircle*, *fillRect*, *print*, *drawBitmap* and so on... just write in the memory buffer, they **don't actually update the display**. In order to update the display you need to call the *refresh* function like so:

```
jdi_display.refresh();
```
***

Please check the [Examples](https://github.com/Gbertaz/JDI_MIP_Display/tree/master/examples)

# Other supported displays
## LPM013M126C
Tested by [@testudor](https://github.com/testudor)

Set both `DISPLAY_WIDTH` and `DISPLAY_HEIGHT` to `176` in [Display_cfg.h](https://github.com/Gbertaz/JDI_MIP_Display/blob/master/Display_cfg.h)

A breakout board for this display can be found [here](https://github.com/testudor/LPM013M126C-breakout)

## LPM009M360A
Tested by [@andelf](https://github.com/andelf)

Set `DISPLAY_WIDTH` to `72` and `DISPLAY_HEIGHT` to `144` in [Display_cfg.h](https://github.com/Gbertaz/JDI_MIP_Display/blob/master/Display_cfg.h)

This is a 0.850 inch display with a 8 pins FPC or a BM20B(0.8)-10DS-0.4V 10 pins connector.
