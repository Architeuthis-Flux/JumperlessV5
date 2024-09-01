
![hero](https://github.com/user-attachments/assets/cb08fe97-e3e9-4bd4-80e1-58738aefd0c0)

![JNameLogo](https://github.com/user-attachments/assets/04c9e1be-118f-4960-8744-adef260e2bba)

**Jumperless V5** is a breadboard where any point can be connected to any other with software-defined jumpers. RGB LEDs under each hole turn the breadboard into a display; paired with its assortment of routable analog/digital I/O, Jumperless V5 gives you real-time information about everything happening in your circuit and the ability to change it at a whim.

The four individually programmable ±8 V power supplies, seven voltage/current/resistance measurement channels, and 10 GPIO can all be connected anywhere on the breadboard or the Arduino Nano header, it's pretty much every piece of equipment on an electronics workbench shoved inside a breadboard.

Jumperless V5 is like x-ray specs for electronics enthusiasts—it lets you see electrons flowing, capacitors charging, frequencies changing, and UART messages zipping, all right there on your breadboard.

***Oh yeah, and it runs on the brand spankin' new [RP2350B](https://www.raspberrypi.com/products/rp2350/)!***

## Improving on the Original

V5 is a significant redesign of the original [Jumperless](https://github.com/Architeuthis-Flux/Jumperless). Having a few hundred people out there using Jumperlesses, sharing ideas, and [writing their](https://github.com/nilclass/jlctl) [own apps](https://github.com/nilclass/jumperlab) gave us a long enough list of things to improve and upgrade. Now that the fundamentals are battle-tested, Jumperless V5 can add some even crazier new stuff like an ungodly number (451) of LEDs, a built-in rotary encoder/switch, daisy chain headers, individually programmable power rails, and an isolated, always-on probing system. 



![hero-1](https://github.com/user-attachments/assets/f06a0391-deb9-48fe-b2fc-8d83a38082f8)

This isn't just bolting on a bunch of new features you'll never use. The Prime Directive of V5 is to make getting a circuit from your brain into hardware completely frictionless, and once it's there, make understanding what's going on as simple as looking at it. The additions may seem minor, but they fundamentally change how using a Jumperless *feels*. It's intuitive enough that it quickly just becomes "part of your brain" in the same way your computer does. And it's easy to forget this isn't how prototyping stuff on a breadboard has always been.

## Computer Optional

Jumperless V5 was designed to make using it with a computer completely optional. Anything you could do over the serial terminal can be done on the board itself, using the clickwheel, probe, and LEDs under the breadboard acting as a 14x30 display. Everything you do gets saved to its 16Mb of flash memory, so you can always just plug it into power and pick up where you left off.
[![HandMenus](https://i.ytimg.com/vi/OTovQYVai6I/maxresdefault.jpg)](https://www.youtube.com/watch?v=OTovQYVai6I "HandMenus")

[//]: # (https://github.com/user-attachments/assets/d15c155e-91ff-43bf-bdfb-77dca244222f)

## Programmable as Heck

What's the fun of having software-defined jumpers if we're just gonna use them like regular meatspace ones? The power of the shiny new RP2350B means all the housekeeping stuff runs on a single core, leaving the other core free to run a Python interpreter with a built-in module to control everything with simple calls.

So you can write programs like:
```
jumperless.setTopRail(5.00)
jumperless.connect(top_rail, row_3)
jumperless.connect(gnd, row_7)
jumperless.uartSetup('tx':row_5, 'rx':row_6, 'baud':9600)

response = jumperless.uart('Hello')

if response.length() < 1:
    jumperless.textOnBreadboard("Tx Rx are swapped... idiot")
    jumperless.uartSetup('tx':row_6, 'rx':row_5, 'baud':9600)

while True:
    response = jumperless.uart('Hello')
    jumperless.textOnBreadboard(response)
```
or (for people more into the analog stuff, a Voltage Controlled Oscillator)
```
while True:
    measurement = jumperless.measure(row_10)
    jumperless.outputSine(row_20, 'freq': measurement*1000)
    jumperless.textOnBreadboard(meaurement + "kHz")
```
Drop those onto the Jumperless over USB, or live code from Thonny or whatever, and they'll be stored in your app library so you can run them whenever you like. 

If Arduino-flavored C++ is more your style, it also accepts commands over the routable UART lines, so you can just plop down any vaguely Arduino Nano-shaped board and just control the Jumperless with the same API:
```
Serial.println(jumperless.connect(row_4, row_20));
```
or just send it a whole netlist
```
Serial.println("f 5-gnd, 23-adc_1, 4-20, gpio_2-nano_reset, ");
```



## Features & Specifications

- 4 x 12-bit DACs ([MCP4278](https://www.microchip.com/en-us/product/mcp4728)) buffered and shifted to ±8 V through high current op amps ([L272D](https://estore.st.com/en/l272d-cpn.html))
- 7 x 12-bit ADCs (built into the RP2350) buffered and level shifted ([LM324](https://www.ti.com/product/LM324B)) to read ±8V
- 2 x 12-bit current/voltage sensors ([INA219](https://www.ti.com/product/INA219)) which can also be used to measure resistance
- 10 x Routable GPIO ([RP2350](https://www.raspberrypi.com/products/rp2350/)) which can also be hardware I²C, UART, or SPI
- 2 x 14 pin daisy chain headers on either side to pass eight analog signals + SPI + Power to another Jumperless
- A cool probe to connect and measure stuff (TRRRS jack)
- A rotary encoder / switch ([SIQ-02FVS3](https://www.lcsc.com/product-detail/Rotary-Encoders_Mitsumi-Electric-SIQ-02FVS3_C2925423.html))
- 451 addressable RGB LEDs ([XL-1010RGBC](https://www.lcsc.com/product-detail/Light-Emitting-Diodes-LED_XINGLIGHT-XL-1010RGBC-WS2812B_C5349953.html))

### Included Probe

Jumperless V5 uses a string of 92 precision resistors in a huge voltage divider and an ADC channel to sense which number the probe is poking. The probe has LEDs to show you which mode you're in, 2 buttons to channge between adding or removing connections, and a switch to turn it into a routable analog IO to measure things or quickly inject signals into your circuit.

![newProbe2](https://github.com/user-attachments/assets/1aecc98a-428c-4794-bf05-6d5bcc994c5e)


If you misplace your probe, its design allows you to plug in any random 1/8-inch audio cable and poke out connections with the tip.

### Serial/I²C/SPI/MIDI

Jumperless V5 can sniff or write any UART, I²C, SPI, or MIDI signals on the board. It can be set to either print whatever it reads on the breadboard LEDs, turning the Jumperless into a serial monitor, or show up as two USB devices on your computer and give you bidirectional communication through that other port.


## Comparisons

|                              | Jumperless V5                                                                   | [OG Jumperless][1]                                                  | [Sandwizz™ Breadboard][2]                                   | [Breadboard][6] + [Wires][7] + [Power Supply][8] +  [Multimeter][9] |
|------------------------------|---------------------------------------------------------------------------------|---------------------------------------------------------------------|-------------------------------------------------------------|---------------------------------------------------------------------|
| **Manufacturer** ~active     | Architeuthis Flux ~active                                                       | Architeuthis Flux ~active                                           | Microaware® ~active                                         | A bunch of companies ~active                                        |
| **Microcontroller** ~active  | [RP2350B][11] ~active                                                            | [RP2040][12] ~active                                               | [CY8C58LP PSoC][3] ~active                                  | N/A  ~active                                                        |
| **Switching Method** ~active | Analog [Crossbar][13] [Matrix][14] ~active                                                  | Analog [Crossbar][13] [Matrix][14] ~active                                      | PSoC Internal Mux ~active                     | Jumper wires  ~active                                               |
| **Resistance**               | ~ 85 Ω ~warning                                                                 | ~ 85 Ω ~warning                                                     | ~ 500 Ω ~danger                                             | ~ 0 Ω ~success                                                      |
| **Power Rails**              | Individually adjustable ±8V 300mA ~success                                            | 3.3V, 5V, ±8V Switch ~warning                                       | 2-5V ~danger                                                | External ~active                                                    |
| **Circuit Input**            | Always-on Probe, Wokwi, Thumbwheel, Terminal, Routable UART, Text file, Python ~success | Scanning Probe, Wokwi, Terminal, Routable UART,  Text file ~warning | KiCad schematic capture then guided part placement  ~danger | [🫰][10] ~active                                                    |
| **Rewiring Time**            | < 2 milliseconds ~success                                                     | < 2 milliseconds ~success                                         | ~ 1-5 seconds ~warning                                      | 5 - 30 minutes ~danger                                              |
| **Measurement**              | Voltage, current, resistance, frequency, digital data ~success                  | Voltage, current, frequency, digital data ~warning                  | Voltage, frequency ~danger                                  | Voltage, current, resistance, frequency ~warning                    |
| **On-board Display**         | 451 RGB LEDs (5 per row) ~success                                               | 111 RGBs (1 per row) ~warning                                       | None ~danger                                                | None ~danger                                                        |
| **Daisy Chaining**           | 8 HW Analog connections + 4 Data lines + Power ~success                         | No ~danger                                                          | 4 Data lines + Power ~warning                               | Yes, with jumper wires ~warning                                     |
| **Max Voltage**              | -9V - +9V (with overvoltage protection) ~success                                | -9V - +9V ~warning                                                  | 0V - 5V ~danger                                             | No Limit (depending on how brave you are) ~warning                  |
| **USB Port**                 | Type C ~success                                                                 | USB Mini **¹** ~warning                                             | USB Micro ~danger                                           | N/A ~active                                                         |
| **Open Source**              | HW + SW ~success                                                                | HW + SW ~success                                                    | No ~danger                                                  | Interestingly, [n][4][o][5] ~danger                                 |
| **Price**                    | $349  ~danger                                                                   | $299 ~warning                                                       | $99-$299 ~success                                           | $5-$500 ~active                                                     |

**¹** _I would argue that USB Mini is peak USB design, and I will gladly die on that hill._

[1]: https://www.tindie.com/products/architeuthisflux/jumperless/
[2]: https://www.kickstarter.com/projects/sandwizz/the-sandwizz-breadboard-concept
[3]: https://www.marutsu.co.jp/contents/shop/marutsu/datasheet/548775.pdf
[4]: https://patents.google.com/patent/USD228136S/en
[5]: https://patents.google.com/patent/US9704417B2/en
[6]: https://www.jameco.com/z/WBU-301-R-Jameco-ValuePro-400-Point-Solderless-Breadboard-3-3-Lx2-1-W_20601.html
[7]: https://www.adafruit.com/product/758
[8]: https://www.amazon.com/RD-RD6006W-Power-Supply-assembled/dp/B096548QH5
[9]: https://eevblog.store/products/eevblog-bm235-multimeter
[10]: https://resources.altium.com/sites/default/files/styles/opengraph/public/blogs/Understanding%20the%20Nuances%20Between%20Breadboard%20Projects%20and%20Prototype%20Layouts-34411.jpg?itok=ZK60eIBU
[11]:https://www.raspberrypi.com/products/rp2350/
[12]:https://www.raspberrypi.com/products/rp2040/
[13]:https://en.wikipedia.org/wiki/Crossbar_switch
[14]:https://en.wikipedia.org/wiki/Clos_network


## Openest Source 

Jumperless V5 is designed to be infinitely open-source and hackable. Want to make 10 of these with a mini USB port for you and your friends? Let me know, and I'll double-check everything when you're putting in the PCB order to make sure all the parts are placed correctly. I'll even send you the custom spring clips.

Every single file that goes into this thing is available for anyone's viewing/modifying/cloning pleasure. I'm here to make awesome hardware, not keep secrets. The [schematic](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Hardware/JumperlessV5hw/JumperlessV5hw.kicad_sch), [PCB design](https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Hardware/JumperlessV5hw/JumperlessV5hw.kicad_pcb), [firmware](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/JumperlessV5firmware), [breadboard shell models](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/Hardware/Board%20Shell), [spring clip models](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/Hardware/Spring%20Clips), [Jumperless app code](https://github.com/Architeuthis-Flux/Jumperless/tree/main/Jumperless_Wokwi_Bridge_App/JumperlessWokwiBridge), are  in [the GitHub repo](https://github.com/Architeuthis-Flux/JumperlessV5).


The menus are handled on the Jumperless itself, so it can be controlled from any terminal emulator like [PuTTY](https://www.putty.org/), xTerm, Serial, etc. Or use the [Jumperless desktop app](https://github.com/Architeuthis-Flux/JumperlessV5/releases/latest) to poll your [Wokwi](https://wokwi.com/) projects for changes and automatically update connections within half a second of clicking save. 


## Do Whatever You Want

There is no "prescribed" use case for this thing. Every design decision was meant to keep it as general purpose as possible while staying easy and fun to use. Whether you're a hobbyist, musician, student, hacker, scientist, teacher, engineer, artist, or just want to be the first person to get Doom running on a breadboard, Jumperless was made to give _you_ an entirely new tool for turning your ideas into rad stuff.




## Manufacturing Plan

[Elecrow](https://www.elecrow.com/) will be doing the PCB fabrication and final assembly. They already have experience from the OG Jumperless, so they know all the tricks (and there were quite a few things to get figured out, this is a *dense* board with 5 separate PCBs and a lot of weird fabrication steps to get them all together.) 

The phosphor bronze spring clips are made by [Shenzhen Fulimei Technology](https://fulimei.en.made-in-china.com/) and the stamping/forming molds have already been made, fiddled with, and perfected. The breadboard shells are SLS printed in black PA-12 nylon by [JLC3DP](https://jlc3dp.com/). Packaging is made by [Shenzhen Zhibang Packaging and Printing](https://www.zhibangpackaging.com/).

All the parts get sent to Elecrow for final assembly and shipped to Henderson, NV where I will lovingly pack each one myself. They could totally do the packing for me, but I like to make sure each Jumperless V5 is absolutely perfect before it goes out to you. 

![boxMockup](/Images/boxMockupSm.jpg)

## Fulfillment & Logistics

After our production run is complete, we will package everything up and send it along to Crowd Supply's fulfillment partner, Mouser Electronics, who will handle distribution to backers worldwide. You can learn more about Crowd Supply's fulfillment service under [_Ordering, Paying, and Shipping_](https://www.crowdsupply.com/guide/ordering-paying-shipping-details) in their guide.

## Risks & Challenges

The RP2350B is brand new, so there could be [more silicon errata](https://hackaday.com/2024/08/28/hardware-bug-in-raspberry-pis-rp2350-causes-faulty-pull-down-behavior/). Luckily, this is a friggin' Jumperless, the hardware *is* the firmware, so that one isn't an issue. It just checks for a lockup by connecting an ADC and breifly shorts that pin to ground to overpower the pullup and get out of that state. There's *always* a workaround.

Aside from some huge global catastrophe happening, you *will* get this and it'll be awesome. 


![JumperlessV5back](https://github.com/user-attachments/assets/46948e94-cf79-4662-9bbe-b61d638dd225)

