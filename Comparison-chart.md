|                     | Jumperless V5          | [OG Jumperless][1]         | [Sandwizz™ Breadboard][2] | [Breadboard][6] + [Wires][7] + [Power Supply][8] +  [Multimeter][9]|
| ------------------- | -------------------    | -------------------        | ------------------------  | --------------------- |
| **Manufacturer**    | Architeuthis Flux      | Architeuthis Flux          | Microaware®               | A bunch of companies  |
| **Circuit Input**   | Always-on Probe, Wokwi, Thumbwheel, Terminal, Routable UART,   Text file |  Scanning Probe, Wokwi, Terminal, Routable UART,  Text file | KiCad schematic capture then guided part placement| [Fingers (with opposable thumbs)][10]|
| **Rewiring Time**   | < 0.5 milliseconds     | < 0.5 milliseconds         | ~ 1-5 seconds             | 5 - 30 minutes        |
| **Switching Method**| Analog Crossbar Matrix | Analog Crossbar Matrix     | [CY8C58LP PSoC][3] Internal Mux| Jumper wires     |
| **Power Rails**     | Individually adjustable ±8V | 3.3V, 5V, ±8V Switch  | 2-5V                      | External              |
| **Measurement**     | Voltage, current, resistance, frequency, digital data | Voltage, current, frequency, digital data| Voltage, frequency| Voltage, current, resistance, frequency |
| **On-board Display**| 445 RGB LEDs (5 per row)| 111 RGBs (1 per row)      | None                      | None                  |
| **Daisy Chaining**  | 8 HW Analog connections + 4 Data lines + Power | No |  4 Data lines + Power     | Yes, with jumper wires|
| **Resistance**      | ~ 85Ω                  | ~ 85Ω                      | ~ 500Ω                    | ~ 0Ω                  |
| **Max Voltage**     | -9V - +9V (with overvoltage protection) | -9V - +9V | 0V - 5V                   | No Limit (depending on how brave you are)|
| **USB Port**        | Type C                 | Mini                       | Micro                     | None                  | 
| **Open Source**     | HW + SW                | HW + SW                    | No                        | Interestingly, [n][4][o][5]     |
| **Price**           | $349                   | $299                       | $99-$299                  | $5-$500               | 

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