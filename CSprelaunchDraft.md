# Jumperless V5
You're flipping through a comic book and see an ad in the back for some X ray glasses. You put some coins in an envelope and send it off (because it's like 50 years ago in this made up scenario), and eventually you get back some spectacles. You immediately look up at the night sky towards the galactic center for Sagittarius A* and don't see a thing; you've been had. You go back to prototyping your new Eurorack module (because that's what I assume everyone was doing in the early 80's) and forget to take them off. 

You notice that you can now see electrons flowing, capacitors charging, frequencies changing, UART messages passing, all right there on your breadboard. Those aren't X-ray specs(10^16Hz - 10^20Hz), they're electronics specs (0Hz - 10^5Hz), even better.  

![tfhtgh](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/2c2376a0-41c4-471f-a3ac-84daf95a678d)

*Jumperless V5 is that pair of x-ray spectacles.* Like all the fancy test equipment covering our workbenches, it converts the things that our hunter-gatherer MK 1 eyeballs can't perceive into something they can. The difference is that it does it for everything, everywhere, exactly where it's happening with an RGB LED under every hole. Now you're prototyping in God Mode, never again will you spend 2 hours testing every possible issue just to find that your Tx and Rx lines are swapped, or you forgot to hook up the Vcc pin, or some other thing that seems absolutely obvious in retrospect but you simply didn't have the information until you went down the list and checked each thing that could possibly go wrong.

Jumper wires are another time-consuming, flow-breaking thing, so let's get rid of those too. To be able to read and write anything everywhere, it also needs to be fully jumperless, so any rows on the breadboard (and the Arduino Nano header) can be connected to any other, as well as any of the 4 programmable power supplies, 3 analog measurement channels, 2 current/resistance sensors, (supplies and measurement are all good to +-8V), or 8 GPIO (4 are 5V logic, 4 are 3.3V. All 8 can instead be routed to another daisy-chained Jumperless as fully analog connections.) 

This isn't meant to abstract away the tactile process of prototyping under a layer of software. In fact, it's just the opposite, not having your circuit hidden under a tangled mass of jumper wires gives you a breadboard that is understandable at a glance and almost reads like a schematic. You can just drop a measurement anywhere and have it displayed on the LEDs under that row so you will immediately see if anything changes in real time, so you can get a much more intuitive sense about *why* things work (or don't). It's about cutting down on the debugging bullshit and letting the grander plans for what you're building flow freely. Ideas happen at their own pace, and getting stuck on the finicky details of circuit design can break the creative momentum. 

Breadboards should be a playground, and now you can attach a debug probe to them.

https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/a3d5ecf4-4665-409d-bab6-2a62fbbe0eb7

Now you have a breadboard with:
- 4 12 bit DACs [MCP4278](https://www.microchip.com/en-us/product/mcp4728) buffered and shifted to ±8V through high current op amps [2 L272D](https://estore.st.com/en/l272d-cpn.html)
- 3 12 bit ADCs (built into the RP2040) double buffered and level shifted (2 LM324) to read ±8V 
- 2 12 bit current/voltage sensors [INA219](https://www.ti.com/product/INA219) which can also be used to measure resistance
- 4 5V logic level GPIO [MCP23S17](https://www.microchip.com/en-us/product/mcp23s17)
- 6 3.3V logic level GPIO [RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html) which can also be hardware I2C, UART, or SPI
- 1 noninverting ±8V op amp buffer [LM324](https://www.ti.com/product/LM324B)
- 2 14 pin daisy chain headers on either side to pass 8 analog signals + SPI + Power to another Jumperless
- A probe to select rows without affecting connections (TRRRS jack with I2C)
- A rotary encoder / switch [SIQ-02FVS3](https://www.lcsc.com/product-detail/Rotary-Encoders_Mitsumi-Electric-SIQ-02FVS3_C2925423.html)
- 445 addressable RGB LEDs [XL-1010RGBC](https://www.lcsc.com/product-detail/Light-Emitting-Diodes-LED_XINGLIGHT-XL-1010RGBC-WS2812B_C5349953.html)

All these things are routable to anywhere on the breadboard or Nano header in a few microseconds under the control of an RP2040. This freedom to script connections/measurements/outputs to everything presumably makes just about any random circuit on the Jumperless V5 Turing complete. There are an infinite number of things you could make this thing do, and the Prime Directive is to make those awesome hacks as simple and straightforward to pull off as possible. 

But for now, I'll only talk about things I've written firmware for and is supported without any hacking required.

Jumperless accepts commands and stores connections in a ridiculously simple, human readable format [f {ADC1-10, 15-48, TOP_RAIL-17, TOP_RAIL>6.5V, D0-UART_Rx, D1-UART_Tx, } ^that's it, that's everything you need to send it to make things happen] that you can just drop onto the board as a text file. You can save a bunch of these net lists into persistent storage and switch between them at a whim. It will even accept connections from whatever's plugged into the Nano header (or anywhere else on the board over UART).

## The App
The desktop app allows you to assign up to 8 [Wokwi](https://wokwi.com/) projects to separate slots that are polled for changes and automatically updated on the Jumperless V5 (within half a second of clicking Save). Or if you don't want to use Wokwi, it simply acts as a regular terminal emulator like PuTTY, xTerm, Serial, etc. You could also just use any of those, the menus are sent and handled on the Jumperless itself, the Jumperless desktop app just adds automatic firmware updates, Wokwi polling, and arduino-cli for flashing code from your Wokwi sketch to the Arduino via a single USB cable.

<img width="300" alt="Screenshot 2024-06-14 at 10 23 31 PM" src="https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/cbb18ee4-1a9d-4453-85b6-f8c31a1ea151">
<img width="300" alt="Screenshot 2024-06-14 at 10 11 18 AM" src="https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/0a502cf6-6f17-4eb9-b38f-3589f6f4f0af">


## Serial/I2C/SPI/MIDI
Jumperless V5 can sniff or write any UART, I2C, SPI, or MIDI signals on the board. It can be set to either print whatever it reads on the breadboard LEDs, turning the Jumperless into a serial monitor and/or show up as 2 USB devices to your computer and give you bidirectional communication through that other port. 
Look at that, Jumperless just saved you $3 on one of those little USB-UART boards, it practically pays for itself!!1!


## Slots/Music
The firmware saves multiple netlists in separate text files. You can save/load/manage the various slots through a serial terminal, the onboard menus, or even crazier things like having the slot change based on a Eurorack CV (control voltage) signal or whatever. So Jumperless can be incorporated into your modular synth setup as a way to switch between a bunch of patches live on the fly. The switchover time from one setup to another is quick enough to be inaudible.


## Voltage/Current Limits
The CMOS analog switches inside these CH446Qs can only block or pass voltages that are between their power supply rails. They're powered with an [LT1054CP](https://www.ti.com/product/LT1054) charge pump voltage doubler/inverter circuit that produces ~±9V. So it can handle any voltages between +9V and -9V and ~100mA per connection.
You may notice that these values are well outside the absolute maximum ratings (14.6V and 10mA per connection) listed in the CH446Q datasheet. That's because they are, all the specifications of the CH446Q get better as the supply voltage increases, so I'm exploiting the arcane weirdness of CMOS to get closer to an ideal switch. All of this is unchanged from the original Jumperless, so it's been well tested. 
![lossm](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/12a3e0c7-4bda-411f-af50-9e27af2528fb)


## On resistance
Okay, here's the main bummer here. As it turns out, we live in a universe governed by both physics and capitalism, so manufacturers only make chips they think people will buy. The [CH446Q](https://www.wch-ic.com/products/CH446.html) (and the [MT8816](https://www.microsemi.com/product-directory/analog-cross-point-switches/4920-mt8816) it was based on) were made with radio, video, and audio in mind, which is much more concerned with frequency response than acting like an idealized jumper wire that can pass a bunch of current. So the total resistance for a jumperless connection is ~90Ω (45Ω for each pass through an overdriven crosspoint switch). For most circuits, this really doesn't have a noticeable effect. The beauty of the Jumperless V5 is that it can show you when that resistance is affecting your circuit and tell you to place an old-school jumper wire on that connection like it's the 1800's.

###### If this campaign does well enough, the next project will be making a general purpose crossbar switch out of beefy MOSFETs and [floating gate](https://en.wikipedia.org/wiki/Floating-gate_MOSFET) drivers to get the voltage limits into the ±50V range and the Ron down to ~1Ω, but custom high-current silicon is a huge, expensive undertaking (that I am 100% committed to doing)

## Frequency Response
The [CH446Q](https://www.wch-ic.com/products/CH446.html) analog crosspoint switches used here have a 3dB rolloff (1/2 the signal amplitude is passed, not a hard limit) of 50MHz, which is well above any breadboard's rolloff frequency (the clip-plastic-clip between each row is effectively a tiny capacitor.) So even for super high speed SPI signals, this really isn't an issue. Modern (last ~30 years) input buffers on ICs are just amazing. TL;DR In regards to frequency, if it works on a regular breadboard, it will work on this.

![Sequence 01 00_00_25_14 Still001](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/56a7ac87-6821-4dcd-8ce5-59f5e6ab4683)


