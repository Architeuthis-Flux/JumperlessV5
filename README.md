# Jumperless V5
###### The next generation [jumperless](https://github.com/Architeuthis-Flux/Jumperless) breadboard

Jumperless V5 is a breadboard where any point can be connected to any other with software-defined jumpers. RGB LEDs under each hole turn the breadboard itself into a display; paired with its assortment of routable analog/digial IO, Jumperless V5 gives you real-time information about everything that's happening in your circuit and the ability to change it at a whim.


![JumperlessV5front](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/60e26150-3dce-4d52-812f-8f1f99c58989)

## You can get one on [Crowd Supply](https://www.crowdsupply.com/architeuthis-flux/jumperless-v5).



https://github.com/user-attachments/assets/703db848-5871-4152-bb03-244a400a1bb8




The 4 individually programmable ±8V power supplies, 5 voltage/current/resistance measurement channels, and 10 GPIO can all be connected anywhere on the breadboard or the Arduino Nano header. Connections made with its matrix of 12 analog crossbar switches can be changed in just a few microseconds, so measurements and GPIO can multiplexed to cover the entire board at once.

V5 is a major redesign of the original [Jumperless](https://github.com/Architeuthis-Flux/Jumperless). Having a few hundred people out there using Jumperlesses, sharing ideas, and [writing their](https://github.com/nilclass/jlctl) [own apps](https://github.com/nilclass/jumperlab) gave me a long enough list of things I wish I had done that I felt it was time to push the design even further. Now that the fundamentals are battle-tested (switching matrix, power supply, routing algorithm), Jumperless V5 can add some even crazier new stuff like; an ungodly number (445) of LEDs, a built in rotary encoder/switch, daisy chain headers, individually programmable power rails, and an isolated, always-on probing system. 





![JumperlessV5back](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/615c90cc-c19b-41ef-a5fb-0dfb2a027084)


These additions may seem somewhat minor, but they fundamentally change the way using a Jumperless V5 *feels*. It's intuitive enough that it quickly just becomes "part of your brain" in the same way your computer does. And it's easy to forget this isn't how prototyping stuff on a breadboard always has been.



https://github.com/user-attachments/assets/c78493ee-f2cd-4704-bab7-26346f978c54

https://github.com/user-attachments/assets/168cb387-934b-4a25-b30d-b696312ee0e1


Jumperless V5 will cost roughly $360 (with free shipping worldwide)




# For People Who Like Reading (btw all of this is a bit dated and some details have changed since then)
You're flipping through a comic book and see an ad in the back for some X-ray glasses. You put some coins in an envelope and send it off (because it's like 50 years ago in this made up scenario) and get back some spectacles. You immediately look up at the night sky towards the galactic center for Sagittarius A* and don't see a thing; you've been had. You go back to prototyping your new Eurorack module (because that's what I assume everyone was doing in the early 80's) and forget to take them off. 

You notice that you can now see electrons flowing, capacitors charging, frequencies changing, UART messages passing, all right there on your breadboard. Those aren't X-ray specs, they're electron specs, even better.  

![tfhtgh](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/2c2376a0-41c4-471f-a3ac-84daf95a678d)

###### //When the prototypes get here on Monday, I'll take proper photos and these will be replaced

*Jumperless V5 is that pair of X-ray spectacles.* Like all the fancy test equipment covering our workbenches, it converts the things that our hunter-gatherer-ass MK 1 eyeballs can't perceive into something they can. The difference is that it does it for everything, everywhere, exactly where it's happening with an RGB LED under every hole. 

Now you're prototyping in God Mode, never again will you spend 2 hours testing every possible issue just to find that your Tx and Rx lines are swapped, or you forgot to hook up the Vcc pin, or some other thing that seems absolutely obvious in retrospect but you simply didn't have the information until you went down the list and checked each thing that could possibly go wrong.

Jumper wires are another time-consuming, flow-breaking thing, so let's get rid of those too. To be able to read and write anything everywhere, it also needs to be fully jumperless, so any rows on the breadboard (and the Arduino Nano header) can be connected to any other, as well as any of the 4 programmable power supplies, 3 analog measurement channels, 2 current/resistance sensors, (supplies and measurement are all good to +-8V), or 10 GPIO (4 are 5V logic, 6 are 3.3V. All 10 can instead be routed to another daisy-chained Jumperless as fully analog connections.) 

This isn't meant to abstract away the tactile process of prototyping under a layer of software. In fact, it's just the opposite, not having your circuit hidden under a tangled mass of jumper wires gives you a breadboard that is understandable at a glance and almost reads like a schematic. You can just drop a measurement anywhere and have it displayed on the LEDs under that row so you will immediately see if anything changes in real time, so you can get a much more intuitive sense about *why* things work (or don't). It's about cutting down on the debugging bullshit and letting the grander plans for what you're building flow freely. Ideas happen at their own pace, and getting stuck on the finicky details of circuit design can break the creative momentum. 

The probe, rotary encoder, and onboard menus make connecting to a computer optional. You can do everything with only USB C power. Your past projects are saved into onboard flash as simple text files that can be uploaded to your computer and shared.

Breadboards should be a playground, and now you can attach a debug probe to them.

https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/a3d5ecf4-4665-409d-bab6-2a62fbbe0eb7

Now you have a breadboard with:

- 4 x 12 bit DACs ([MCP4278](https://www.microchip.com/en-us/product/mcp4728)) buffered and shifted to ±8V through high current op amps ([2 L272D](https://estore.st.com/en/l272d-cpn.html))
- 3 x 12 bit ADCs (built into the RP2040) double buffered and level shifted (2 LM324) to read ±8V 
- 2 x 12 bit current/voltage sensors ([INA219](https://www.ti.com/product/INA219)) which can also be used to measure resistance
- 4 x 5V logic level GPIO ([MCP23S17](https://www.microchip.com/en-us/product/mcp23s17))
- 6 x 3.3V logic level GPIO ([RP2040](https://www.raspberrypi.com/documentation/microcontrollers/rp2040.html)) which can also be hardware I2C, UART, or SPI
- 1 x ±8V op amp noninverting buffer ([LM324](https://www.ti.com/product/LM324B))
- 2 x 14 pin daisy chain headers on either side to pass 8 analog signals + SPI + Power to another Jumperless
- A probe to select rows without affecting connections (TRRRS jack with I2C)
- A rotary encoder / switch ([SIQ-02FVS3](https://www.lcsc.com/product-detail/Rotary-Encoders_Mitsumi-Electric-SIQ-02FVS3_C2925423.html))
- 445 addressable RGB LEDs ([XL-1010RGBC](https://www.lcsc.com/product-detail/Light-Emitting-Diodes-LED_XINGLIGHT-XL-1010RGBC-WS2812B_C5349953.html))


You can see how all this stuff is connected in the [schematic](https://kicanvas.org/?github=https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Hardware/JumperlessV5hw/JumperlessV5hw.kicad_sch).

Or check out the [PCB](https://kicanvas.org/?github=https://github.com/Architeuthis-Flux/JumperlessV5/blob/main/Hardware/JumperlessV5hw/JumperlessV5hw.kicad_pcb) and [GitHub repo](https://github.com/Architeuthis-Flux/JumperlessV5)

All these things are routable to anywhere on the breadboard or Nano header in a few microseconds under the control of the onboard RP2040. This freedom to script connections and measurements and route outputs anywhere presumably makes just about any random circuit on the Jumperless V5 Turing complete. There are an infinite number of things you could make this thing do, and the Prime Directive is to make those awesome hacks as simple and straightforward to pull off as possible. 

But for now, I'll only talk about things I've written firmware for that is currently supported without any hacking required.

Jumperless V5 accepts commands and stores connections in a ridiculously simple, human readable format that you can just drop onto the board as a text file. You can save a bunch of these net lists into persistent storage and switch between them at your whim. It will even accept connections from whatever's plugged into the Nano header (or anywhere else on the board over UART).

```
f {ADC1-10, 15-48, TOP_RAIL-17, TOP_RAIL>6.5V, D0-UART_Rx, D1-UART_Tx, }
```
⬆︎ *that's it, that's everything you need to send it to make things happen*

You can see how all that firmware works in the [firmware section of the GitHub repo]([https://github.com/Architeuthis-Flux/JumperlessV5](https://github.com/Architeuthis-Flux/JumperlessV5/tree/main/JumperlessV5firmware))

## The Probe
Jumperless V5 uses a string of 92 ±0.1% precision resistors in a huge voltage divider and one of the ADC channels to sense which number the probe is poking. The probe itself is very simple, just a button, an LED, and a pokey needle to touch the sensing pads. It connects to the main board with a TRRRS 1/8" audio jack. It also has I2C and power available on there for future hacking purposes (in case someone wants to stick an OLED on the probe, the code already does send signals for an SSD1306 display). 

If you happen to misplace your probe, the way it's designed allows you to just plug in any random 1/8" audio cable and poke out connections with the tip.

![DSC01819-3](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/b48b3448-29d8-4961-aaad-c2f262ceb448)

###### Just pretend this has an audio cable stuck to the back and it's sitting on a V5

## The App
The desktop app allows you to assign up to 8 [Wokwi](https://wokwi.com/) projects to separate slots that are polled for changes and automatically updated on the Jumperless V5 (within half a second of clicking Save). Or if you don't want to use Wokwi, it simply acts as a regular terminal emulator like PuTTY, xTerm, Serial, etc. You could also just use any of those, the menus are sent and handled on the Jumperless itself, the Jumperless desktop app just adds automatic firmware updates, Wokwi polling, and arduino-cli for flashing code from your Wokwi sketch to the Arduino via a single USB cable.

<img width="300" alt="Screenshot 2024-06-14 at 10 23 31 PM" src="https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/cbb18ee4-1a9d-4453-85b6-f8c31a1ea151">
<img width="300" alt="Screenshot 2024-06-14 at 10 11 18 AM" src="https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/0a502cf6-6f17-4eb9-b38f-3589f6f4f0af">


## Serial/I2C/SPI/MIDI
Jumperless V5 can sniff or write any UART, I2C, SPI, or MIDI signals on the board. It can be set to either print whatever it reads on the breadboard LEDs, turning the Jumperless into a serial monitor and/or show up as 2 USB devices to your computer and give you bidirectional communication through that other port. 
Look at that, Jumperless just saved you $3 on one of those little USB-UART boards, it practically pays for itself!!1!

![lossm-5](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/dc569c44-6b25-495a-891e-34616fd231ba)



## Slots/Music
The firmware saves multiple netlists in separate text files. You can save/load/manage the various slots through a serial terminal, the onboard menus, or even crazier things like having the slot change based on a Eurorack CV (control voltage) signal or whatever. So Jumperless can be incorporated into your modular synth setup as a way to switch between a bunch of patches live on the fly. The switchover time from one setup to another is quick enough to be inaudible.


## Voltage/Current Limits
The CMOS analog switches inside these CH446Qs can only block or pass voltages that are between their power supply rails. They're powered with an [LT1054CP](https://www.ti.com/product/LT1054) charge pump voltage doubler/inverter circuit that produces ~±9V. So it can handle any voltages between +9V and -9V and ~100mA per connection.
You may notice that these values are well outside the absolute maximum ratings (14.6V and 10mA per connection) listed in the CH446Q datasheet. That's because they are, all the specifications of the CH446Q get better as the supply voltage increases, so I'm exploiting the arcane weirdness of CMOS to get closer to an ideal switch. All of this is unchanged from the original Jumperless, so it's been well tested. 

![lossm](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/12a3e0c7-4bda-411f-af50-9e27af2528fb)


## On resistance
Okay, here's the main bummer here. As it turns out, we live in a universe governed by both physics and capitalism, so manufacturers only make chips they think people will buy. The [CH446Q](https://www.wch-ic.com/products/CH446.html) (and the [MT8816](https://www.microsemi.com/product-directory/analog-cross-point-switches/4920-mt8816) it was based on) were made with radio, video, and audio in mind, which is much more concerned with frequency response than acting like an idealized jumper wire that can pass a bunch of current. So the total resistance for a jumperless connection is ~90Ω (45Ω for each pass through an overdriven crosspoint switch). For most circuits, this really doesn't have a noticeable effect. The beauty of the Jumperless V5 is that it can show you when that resistance is affecting your circuit and tell you to place an old-school jumper wire on that connection like it's the 1800s.

###### If this campaign does well enough, the next project will be making a general purpose crossbar switch out of beefy MOSFETs and [floating gate](https://en.wikipedia.org/wiki/Floating-gate_MOSFET) drivers to get the voltage limits into the ±50V range and the R<sub>on</sub> down to ~1Ω, but custom high-current silicon is a huge, expensive undertaking (that I am 100% committed to doing)

## Frequency Response
The [CH446Q](https://www.wch-ic.com/products/CH446.html) analog crosspoint switches used here have a 3dB rolloff (1/2 the signal amplitude is passed, not a hard limit) of 50MHz, which is well above any breadboard's rolloff frequency (the clip-plastic-clip between each row is effectively a tiny capacitor.) So even for super high speed SPI signals, this really isn't an issue. Modern (last ~30 years) input buffers on ICs are just amazing. TL;DR In regards to frequency, if it works on a regular breadboard, it will work on this.

## Repairability
If you're worried about messing up one of the a spring clips and ruining your Jumperless, fear not! The breadboard shell is removable, so you can easily remove the shell and replace the offending clip (5-6 spares will be included). 

![JumperlessV5boardoff](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/448d4c7b-01eb-44f2-b1fb-f901f95a15cd)


This is really just for peace of mind. These clips are made of spring-tempered phosphor bronze, which is what they use for super high quality breadboards, and they're pretty hard to permanently bend out of shape. 

The other cool bonus of having removable shells is that you can print your own in any color/material you like and swap it out. The stock shell is 3D printed in black SLS PA-12 Nylon.

When you buy a Jumperless, you're also buying the right so have a *working* Jumperless forever. There's literally nothing you can do to break it so badly that I won't happily fix or replace it for free. If you want to fix it yourself, I will send you any parts you need and walk you through the repair. 


## Open Sourciness
This thing infinitely open source and hackable. I'm kind of a zealot about that stuff. Want to make 10 of these with a USB Mini port for you and your friends? Let me know and I'll double check everything when you're putting in the PCB order to make sure all the parts are placed correctly and stuff (and I'd totally buy one, I love USB Mini). I'll even send you the custom spring clips.

If you have some feature you want to add, I'm happy to help. Either by just adding it myself or walking you through the relevant bits of code to make it happen.
![JumperlessV5hw](https://github.com/Architeuthis-Flux/JumperlessV5/assets/20519442/ce409b68-03f8-4ee0-b19b-1aba168de07f)


https://github.com/user-attachments/assets/f4ce75e4-14b6-48c8-96ba-9c4a3180d78c



https://github.com/user-attachments/assets/7bf4f20c-272e-4586-8e50-9da7c20dedf6

https://github.com/user-attachments/assets/13d6d9ab-8ea0-4387-aee6-343be06bf343




![SBCadapters-4](https://github.com/user-attachments/assets/b3c5f8c6-84f3-4f9e-b543-64ebc18004e0)
![SBCadapters-3](https://github.com/user-attachments/assets/cd5504ac-27c4-4623-8025-be4615748875)
![SBCadapters-2](https://github.com/user-attachments/assets/dc7d4b75-3351-447b-a035-1eed026571bf)
![SBCadapters-1](https://github.com/user-attachments/assets/983a8943-3e70-4f71-a25c-db8b70e866cc)


