© 2010 Microchip Technology Inc. DS22187E-page 1
MCP4728
Features
• 12-Bit Voltage Output DAC with Four Buffered
Outputs
• On-Board Nonvolatile Memory (EEPROM) for
DAC Codes and I2C™ Address Bits
• Internal or External Voltage Reference Selection
• Output Voltage Range:
- Using Internal VREF (2.048V):
0.000V to 2.048V with Gain Setting = 1
0.000V to 4.096V with Gain Setting = 2
- Using External VREF (VDD):
0.000V to VDD
• ±0.2 Least Significant Bit (LSB) Differential
Nonlinearity (DNL) (typical)
• Fast Settling Time: 6 µs (typical)
• Normal or Power-Down Mode
• Low Power Consumption
• Single-Supply Operation: 2.7V to 5.5V
• I2C Interface:
- Address bits: User Programmable to
EEPROM
- Standard (100 kbps), Fast (400 kbps) and
High Speed (HS) Mode (3.4 Mbps)
• 10-Lead MSOP Package
• Extended Temperature Range: -40°C to +125°C
Applications
• Set Point or Offset Adjustment
• Sensor Calibration
• Closed-Loop Servo Control
• Low Power Portable Instrumentation
• PC Peripherals
• Programmable Voltage and Current Source
• Industrial Process Control
• Instrumentation
• Bias Voltage Adjustment for Power Amplifiers
Description
The MCP4728 device is a quad, 12-bit voltage output
Digital-to-Analog Convertor (DAC) with nonvolatile
memory (EEPROM). Its on-board precision output
amplifier allows it to achieve rail-to-rail analog output
swing.
The DAC input codes, device configuration bits, and
I
2C address bits are programmable to the nonvolatile
memory (EEPROM) by using I2C serial interface
commands. The nonvolatile memory feature enables
the DAC device to hold the DAC input codes during
power-off time, allowing the DAC outputs to be
available immediately after power-up with the saved
settings. This feature is very useful when the DAC
device is used as a supporting device for other devices
in the application’s network.
The MCP4728 device has a high precision internal
voltage reference (VREF = 2.048V). The user can select
the internal reference or external reference (VDD) for
each channel individually.
Each channel can be operated in Normal mode or
Power-Down mode individually by setting the
configuration register bits. In Power-Down mode, most
of the internal circuits in the powered down channel are
turned off for power savings, and the output amplifier
can be configured to present a known low, medium, or
high resistance output load.
The MCP4728 device includes a Power-on Reset
(POR) circuit to ensure reliable power-up and an
on-board charge pump for the EEPROM programming
voltage.
The MCP4728 has a two-wire I2C compatible serial
interface for standard (100 kHz), fast (400 kHz), or high
speed (3.4 MHz) mode.
The MCP4728 DAC is an ideal device for applications
requiring design simplicity with high precision, and for
applications requiring the DAC device settings to be
saved during power-off time.
The MCP4728 device is available in a 10-lead MSOP
package and operates from a single 2.7V to 5.5V
supply voltage.
12-Bit, Quad Digital-to-Analog Converter with EEPROM Memory
MCP4728
DS22187E-page 2 © 2010 Microchip Technology Inc.
Package Type
Functional Block Diagram
1
2
3
4
10
6
7
8
VDD
SCL
SDA
VSS
VOUT D
VOUT C
LDAC
RDY/BSY
9
5
VOUT B
VOUT A
MCP4728
MSOP
VDD
VSS
SCL
SDA
VOUT A
LDAC
INPUT
REGISTER A
INPUT
REGISTER B
INPUT
REGISTER C
INPUT
REGISTER D
OUTPUT
REGISTER A STRING DAC A
EEPROM C
EEPROM D
EEPROM B
EEPROM A
Power Down
 Control
STRING DAC B
STRING DAC C
STRING DAC D
Output
 Logic
OP
AMP A
VOUT B
Power Down
 Control
Output
 Logic
VOUT C
Power Down
 Control
Output
 Logic
VOUT D
Power Down
 Control
Output
 Logic
 Gain
Control
 Gain
Control
 Gain
Control
 Gain
Control
VREF A
VREF B
VREF C
VREF D
VREF Selector VREF Internal VREF (2.048V)
RDY/BSY
I
2C Interface Logic
VDD
(VREF A, VREF B, VREF C, VREF D)
OUTPUT
REGISTER B
OUTPUT
REGISTER C
OUTPUT
REGISTER D
OP
AMP B
OP
AMP C
OP
AMP D
UDAC
UDAC
UDAC
UDAC
© 2010 Microchip Technology Inc. DS22187E-page 3
MCP4728
1.0 ELECTRICAL
CHARACTERISTICS
Absolute Maximum Ratings†
VDD...................................................................................6.5V
All inputs and outputs w.r.t VSS ................. -0.3V to VDD+0.3V
Current at Input Pins ....................................................±2 mA
Current at Supply Pins ............................................. ±110 mA
Current at Output Pins ...............................................±25 mA
Storage Temperature ...................................-65°C to +150°C
Ambient Temp. with Power Applied .............-55°C to +125°C
ESD protection on all pins ................ ≥ 4 kV HBM, ≥ 400V MM
Maximum Junction Temperature (TJ) .........................+150°C
† Notice: Stresses above those listed under “Maximum
ratings” may cause permanent damage to the device. This is
a stress rating only and functional operation of the device at
these or any other conditions above those indicated in the
operation listings of this specification is not implied. Exposure
to maximum rating conditions for extended periods may affect
device reliability.
ELECTRICAL CHARACTERISTICS
Electrical Specifications: Unless otherwise indicated, all parameters apply at VDD = +2.7V to 5.5V, VSS = 0V,
RL =5kΩ, CL = 100 pF, GX = 1, TA = -40°C to +125°C. Typical values are at +25°C, VIH = VDD, VIL = VSS.
Parameter Symbol Min Typical Max Units Conditions
Power Requirements
Operating Voltage VDD 2.7 5.5 V
Supply Current with
External Reference
(VREF = VDD)
(Note 1)
IDD_EXT — 800 1400 µA VREF = VDD, VDD = 5.5V
All 4 channels are in Normal mode.
— 600 — µA 3 channels are in Normal mode,
1 channel is powered down.
— 400 — µA 2 channels are in Normal mode,
2 channel are powered down.
— 200 — µA 1 channel is in Normal mode,
3 channels are powered down.
Power-Down Current with
External Reference
IPD_EXT — 40 — nA All 4 channels are powered down.
(VREF = VDD)
Supply Current with
Internal Reference
(VREF = Internal)
(Note 1)
IDD_INT — 800 1400 µA VREF = Internal Reference
VDD = 5.5V
All 4 channels are in normal mode.
— 600 — µA 3 channels are in Normal mode,
1 channel is powered down.
— 400 — µA 2 channels are in Normal mode,
2 channels are powered down.
— 200 — µA 1 channel is in Normal mode,
3 channels are powered down.
Power-Down Current with
Internal Reference
IPD_INT — 45 60 µA All 4 channels are powered down.
VREF = Internal Reference
Note 1: All digital input pins (SDA, SCL, LDAC) are tied to “High”, Output pins are unloaded, code = 0 x 000.
2: The power-up ramp rate measures the rise of VDD over time.
3: This parameter is ensured by design and not 100% tested.
4: This parameter is ensured by characterization and not 100% tested.
5: Test code range: 100 - 4000 codes, VREF = VDD, VDD = 5.5V.
6: Time delay to settle to a new reference when switching from external to internal reference or vice versa.
7: This parameter is indirectly tested by Offset and Gain error testing.
8: Within 1/2 LSB of the final value when code changes from 1/4 of to 3/4 of full scale.
9: This time delay is measured from the falling edge of ACK pulse in I2C command to the beginning of VOUT.
This time delay is not included in the output settling time specification.
MCP4728
DS22187E-page 4 © 2010 Microchip Technology Inc.
Power-on Reset
Threshold Voltage
VPOR — 2.2 — V All circuits, including EEPROM, are
ready to operate.
Power-Up Ramp Rate VRAMP 1 — — V/s Note 2, Note 4
DC Accuracy
Resolution n 12 — — Bits Code Change: 000h to FFFh
Integral Nonlinearity (INL)
Error
INL — ±2 ±13 LSB Note 5
DNL Error DNL -0.75 ±0.2 ±0.75 LSB Note 5
Offset Error VOS — 5 20 mV Code = 000h
See Figure 2-24
Offset Error Drift ΔVOS/°C — ±0.16 — ppm/°C -45°C to +25°C
— ±0.44 — ppm/°C +25°C to +125°C
Gain Error GE -1.25 0.4 +1.25 % of
FSR
Code = FFFh,
Offset error is not included.
Typical value is at room
temperature
See Figure 2-25
Gain Error Drift ΔGE/°C — -3 — ppm/°C
Internal Voltage Reference (VREF), (Note 3)
Internal Voltage Reference VREF 2.007 2.048 2.089 V
Temperature Coefficient ΔVREF/°C — 125 — ppm/°C -40 to 0°C
— 0.25 — LSB/°C
— 45 — ppm/°C 0 to +125°C
— 0.09 — LSB/°C
Reference Output Noise ENREF — 290 — µVp-p Code = FFFh,
0.1 – 10 Hz, Gx = 1
Output Noise Density eNREF — 1.2 — Code = FFFh, 1 kHz, Gx = 1
— 1.0 — Code = FFFh, 10 kHz, Gx = 1
1/f Corner Frequency fCORNER — 400 — Hz
ELECTRICAL CHARACTERISTICS (CONTINUED)
Electrical Specifications: Unless otherwise indicated, all parameters apply at VDD = +2.7V to 5.5V, VSS = 0V,
RL =5kΩ, CL = 100 pF, GX = 1, TA = -40°C to +125°C. Typical values are at +25°C, VIH = VDD, VIL = VSS.
Parameter Symbol Min Typical Max Units Conditions
Note 1: All digital input pins (SDA, SCL, LDAC) are tied to “High”, Output pins are unloaded, code = 0 x 000.
2: The power-up ramp rate measures the rise of VDD over time.
3: This parameter is ensured by design and not 100% tested.
4: This parameter is ensured by characterization and not 100% tested.
5: Test code range: 100 - 4000 codes, VREF = VDD, VDD = 5.5V.
6: Time delay to settle to a new reference when switching from external to internal reference or vice versa.
7: This parameter is indirectly tested by Offset and Gain error testing.
8: Within 1/2 LSB of the final value when code changes from 1/4 of to 3/4 of full scale.
9: This time delay is measured from the falling edge of ACK pulse in I2C command to the beginning of VOUT.
This time delay is not included in the output settling time specification.
HZ μV
© 2010 Microchip Technology Inc. DS22187E-page 5
MCP4728
Analog Output (Output Amplifier)
Output Voltage Swing VOUT — FSR — V Note 7
Full Scale Range
(Note 7)
FSR — VDD — VVREF = VDD
 FSR = from 0.0V to VDD
— VREF — VVREF = Internal, Gx = 1,
FSR = from 0.0 V to VREF
— 2*VREF — VVREF = Internal, Gx = 2,
FSR = from 0.0V to 2 * VREF
Output Voltage
Settling Time
TSETTLING — 6 — µs Note 8
Analog Output Time Delay
from Power-Down Mode
TdExPD — 4.5 — µs VDD = 5V,
Note 4, Note 9
Time delay to settle to new
reference
(Note 4, Note 6)
TdREF — 26 — µs From External to Internal
Reference
— 44 — µs From Internal to External
Reference
Power Supply Rejection PSRR — -57 — dB VDD = 5V ±10%, VREF = Internal
Capacitive Load Stability CL — — 1000 pF RL =5kΩ
No oscillation, Note 4
Slew Rate SR — 0.55 — V/µs
Phase Margin pM — 66 — Degree
(°)
CL = 400 pF, RL = ∞
Short Circuit Current ISC — 15 24 mA VDD = 5V,
All VOUT Pins = Grounded.
Tested at room temperature.
Short Circuit Current
Duration
TSC_DUR — Infinite — hours Note 4
DC Output Impedance
(Note 4)
ROUT — 1 — Ω Normal mode
— 1 —kΩ Power-Down mode 1
(PD1:PD0 = 0:1), VOUT to VSS
— 100 — kΩ Power-Down mode 2
(PD1:PD0 = 1:0), VOUT to VSS
— 500 — kΩ Power-Down mode 3
(PD1:PD0 = 1:1), VOUT to VSS
ELECTRICAL CHARACTERISTICS (CONTINUED)
Electrical Specifications: Unless otherwise indicated, all parameters apply at VDD = +2.7V to 5.5V, VSS = 0V,
RL =5kΩ, CL = 100 pF, GX = 1, TA = -40°C to +125°C. Typical values are at +25°C, VIH = VDD, VIL = VSS.
Parameter Symbol Min Typical Max Units Conditions
Note 1: All digital input pins (SDA, SCL, LDAC) are tied to “High”, Output pins are unloaded, code = 0 x 000.
2: The power-up ramp rate measures the rise of VDD over time.
3: This parameter is ensured by design and not 100% tested.
4: This parameter is ensured by characterization and not 100% tested.
5: Test code range: 100 - 4000 codes, VREF = VDD, VDD = 5.5V.
6: Time delay to settle to a new reference when switching from external to internal reference or vice versa.
7: This parameter is indirectly tested by Offset and Gain error testing.
8: Within 1/2 LSB of the final value when code changes from 1/4 of to 3/4 of full scale.
9: This time delay is measured from the falling edge of ACK pulse in I2C command to the beginning of VOUT.
This time delay is not included in the output settling time specification.
MCP4728
DS22187E-page 6 © 2010 Microchip Technology Inc.
Dynamic Performance (Note 4)
Major Code Transition
Glitch
— 45 — nV-s 1 LSB code change around major
carry (from 7FFh to 800h)
Digital Feedthrough — <10 — nV-s
Analog Crosstalk — <10 — nV-s
DAC-to-DAC Crosstalk — <10 — nV-s
Digital Interface
Output Low Voltage VOL — — 0.4 V IOL = 3 mA
SDA and RDY/BSY pins
Schmitt Trigger
Low Input
Threshold Voltage
VIL — — 0.3VDD V VDD > 2.7V.
SDA, SCL, LDAC pins
— — 0.2VDD V VDD ≤ 2.7V.
SDA, SCL, LDAC pins
Schmitt Trigger
High Input
Threshold Voltage
VIH 0.7VDD — — V SDA, SCL, LDAC pins
Input Leakage ILI — — ±1 µA SCL = SDA = LDAC = VDD,
SCL = SDA = LDAC = VSS
Pin Capacitance CPIN — — 3 pF Note 4
EEPROM
EEPROM Write Time TWRITE — 25 50 ms EEPROM write time
Data Retention — 200 — Years At +25°C, Note 3
LDAC Input
LDAC Low Time TLDAC 210 — — ns Updates analog outputs (Note 3)
ELECTRICAL CHARACTERISTICS (CONTINUED)
Electrical Specifications: Unless otherwise indicated, all parameters apply at VDD = +2.7V to 5.5V, VSS = 0V,
RL =5kΩ, CL = 100 pF, GX = 1, TA = -40°C to +125°C. Typical values are at +25°C, VIH = VDD, VIL = VSS.
Parameter Symbol Min Typical Max Units Conditions
Note 1: All digital input pins (SDA, SCL, LDAC) are tied to “High”, Output pins are unloaded, code = 0 x 000.
2: The power-up ramp rate measures the rise of VDD over time.
3: This parameter is ensured by design and not 100% tested.
4: This parameter is ensured by characterization and not 100% tested.
5: Test code range: 100 - 4000 codes, VREF = VDD, VDD = 5.5V.
6: Time delay to settle to a new reference when switching from external to internal reference or vice versa.
7: This parameter is indirectly tested by Offset and Gain error testing.
8: Within 1/2 LSB of the final value when code changes from 1/4 of to 3/4 of full scale.
9: This time delay is measured from the falling edge of ACK pulse in I2C command to the beginning of VOUT.
This time delay is not included in the output settling time specification.
© 2010 Microchip Technology Inc. DS22187E-page 7
MCP4728
FIGURE 1-1: I
2C Bus Timing Data.
FIGURE 1-2: LDAC Pin Timing vs. VOUT Update.
TFSCL
SCL
SDA
TSU:STA
TSP
THD:STA
TLOW
THIGH
THD:DAT
TAA
TSU:DAT
TRSCL
TSU:STO
TBUF
0.3VDD
0.7VDD
TFSDA TRSDA
TLDAC LDAC
 VOUT (UDAC = 1)
No Update Update
0.3VDD
0.7VDD
MCP4728
DS22187E-page 8 © 2010 Microchip Technology Inc.

I
2C SERIAL TIMING SPECIFICATIONS
Electrical Specifications: Unless otherwise specified, all limits are specified for TA = -40 to +125°C, VSS = 0V,
Standard and Fast Mode: VDD = +2.7V to +5.5V
High Speed Mode: VDD = +4.5V to +5.5V.
Parameters Sym Min Typ Max Units Conditions
Clock Frequency fSCL 0 — 100 kHz Standard Mode
Cb = 400 pF, 2.7V – 5.5V
0 — 400 kHz Fast Mode
Cb = 400 pF, 2.7V – 5.5V
0 — 1.7 MHz High Speed Mode 1.7
Cb = 400 pF, 4.5V – 5.5V
0 — 3.4 MHz High Speed Mode 3.4
Cb = 100 pF, 4.5V – 5.5V
Bus Capacitive Loading Cb — — 400 pF Standard Mode
2.7V – 5.5V
— — 400 pF Fast Mode
2.7V – 5.5V
— — 400 pF High Speed Mode 1.7
4.5V – 5.5V
— — 100 pF High Speed Mode 3.4
4.5V – 5.5V
Start Condition Setup Time
(Start, Repeated Start)
TSU:STA 4700 — ns Standard Mode
600 — — ns Fast Mode
160 — — ns High Speed Mode 1.7
160 — — ns High Speed Mode 3.4
Start Condition Hold Time THD:STA 4000 — ns Standard Mode
600 — — ns Fast Mode
160 — — ns High Speed Mode 1.7
160 — — ns High Speed Mode 3.4
Stop Condition Setup Time TSU:STO 4000 — ns Standard Mode
600 — — ns Fast Mode
160 — — ns High Speed Mode 1.7
160 — — ns High Speed Mode 3.4
Clock High Time THIGH 4000 — — ns Standard Mode
600 — — ns Fast Mode
120 — — ns High Speed Mode 1.7
60 — — ns High Speed Mode 3.4
Clock Low Time TLOW 4700 — — ns Standard Mode
1300 — — ns Fast Mode
320 — — ns High Speed Mode 1.7
160 — — ns High Speed Mode 3.4
Note 1: This parameter is ensured by characterization and is not 100% tested.
2: After a Repeated Start condition or an Acknowledge bit.
3: If this parameter is too short, it can create an unintentional Start or Stop condition to other devices on the I2C bus line. If
this parameter is too long, the Data Input Setup (TSU:DAT) or Clock Low time (TLOW) can be affected.
Data Input: This parameter must be longer than tSP.
Data Output: This parameter is characterized, and tested indirectly by testing TAA parameter.
4: This specification is not a part of the I2C specification. This specification is equivalent to the Data Hold Time (THD:DAT)
plus SDA Fall (or rise) time: TAA = THD:DAT + TFSDA (OR TRSDA).
5: Time between Start and Stop conditions.
© 2010 Microchip Technology Inc. DS22187E-page 9
MCP4728
SCL Rise Time
(Note 1)
TRSCL — — 1000 ns Standard Mode
20 + 0.1Cb — 300 ns Fast Mode
20 — 80 ns High Speed Mode 1.7
20 — 160 ns High Speed Mode 1.7
(Note 2)
10 — 40 ns High Speed Mode 3.4
10 — 80 ns High Speed Mode 3.4
(Note 2)
SDA Rise Time
(Note 1)
TRSDA — — 1000 ns Standard Mode
20 + 0.1Cb — 300 ns Fast Mode
20 — 80 ns High Speed Mode 1.7
10 — 40 ns High Speed Mode 3.4
SCL Fall Time
(Note 1)
TFSCL — — 300 ns Standard Mode
20 + 0.1Cb — 300 ns Fast Mode
20 — 80 ns High Speed Mode 1.7
10 — 40 ns High Speed Mode 3.4
SDA Fall Time
(Note 1)
TFSDA — — 300 ns Standard Mode
20 + 0.1Cb — 300 ns Fast Mode
20 — 160 ns High Speed Mode 1.7
10 — 80 ns High Speed Mode 3.4
Data Input Setup Time TSU:DAT 250 — — ns Standard Mode
100 — — ns Fast Mode
10 — — ns High Speed Mode 1.7
10 — — ns High Speed Mode 3.4
Data Hold Time
(Input, Output)
(Note 3)
THD:DAT 0 — 3450 ns Standard Mode
0 — 900 ns Fast Mode
0 — 150 ns High Speed Mode 1.7
0 — 70 ns High Speed Mode 3.4
Output Valid from Clock
(Note 4)
TAA 0 — 3750 ns Standard Mode
0 — 1200 ns Fast Mode
0 — 310 ns High Speed Mode 1.7
0 — 150 ns High Speed Mode 3.4
I
2C SERIAL TIMING SPECIFICATIONS (CONTINUED)
Electrical Specifications: Unless otherwise specified, all limits are specified for TA = -40 to +125°C, VSS = 0V,
Standard and Fast Mode: VDD = +2.7V to +5.5V
High Speed Mode: VDD = +4.5V to +5.5V.
Parameters Sym Min Typ Max Units Conditions
Note 1: This parameter is ensured by characterization and is not 100% tested.
2: After a Repeated Start condition or an Acknowledge bit.
3: If this parameter is too short, it can create an unintentional Start or Stop condition to other devices on the I2C bus line. If
this parameter is too long, the Data Input Setup (TSU:DAT) or Clock Low time (TLOW) can be affected.
Data Input: This parameter must be longer than tSP.
Data Output: This parameter is characterized, and tested indirectly by testing TAA parameter.
4: This specification is not a part of the I2C specification. This specification is equivalent to the Data Hold Time (THD:DAT)
plus SDA Fall (or rise) time: TAA = THD:DAT + TFSDA (OR TRSDA).
5: Time between Start and Stop conditions.
MCP4728
DS22187E-page 10 © 2010 Microchip Technology Inc.
TEMPERATURE CHARACTERISTICS
Bus Free Time
(Note 5)
TBUF 4700 — — ns Standard Mode
1300 — — ns Fast Mode
— — — ns High Speed Mode 1.7
— — — ns High Speed Mode 3.4
Input Filter
Spike Suppression
(SDA and SCL)
 (Not Tested)
TSP — — — ns Standard Mode
(Not Applicable)
— 50 — ns Fast Mode
— 10 — ns High Speed Mode 1.7
— 10 — ns High Speed Mode 3.4
Electrical Specifications: Unless otherwise indicated, VDD = +2.7V to +5.5V, VSS = GND.
Parameters Symbol Min Typical Max Units Conditions
Temperature Ranges
Specified Temperature Range TA -40 — +125 °C
Operating Temperature Range TA -40 — +125 °C
Storage Temperature Range TA -65 — +150 °C
Thermal Package Resistances
Thermal Resistance, 10L-MSOP θJA — 202 — °C/W
I
2C SERIAL TIMING SPECIFICATIONS (CONTINUED)
Electrical Specifications: Unless otherwise specified, all limits are specified for TA = -40 to +125°C, VSS = 0V,
Standard and Fast Mode: VDD = +2.7V to +5.5V
High Speed Mode: VDD = +4.5V to +5.5V.
Parameters Sym Min Typ Max Units Conditions
Note 1: This parameter is ensured by characterization and is not 100% tested.
2: After a Repeated Start condition or an Acknowledge bit.
3: If this parameter is too short, it can create an unintentional Start or Stop condition to other devices on the I2C bus line. If
this parameter is too long, the Data Input Setup (TSU:DAT) or Clock Low time (TLOW) can be affected.
Data Input: This parameter must be longer than tSP.
Data Output: This parameter is characterized, and tested indirectly by testing TAA parameter.
4: This specification is not a part of the I2C specification. This specification is equivalent to the Data Hold Time (THD:DAT)
plus SDA Fall (or rise) time: TAA = THD:DAT + TFSDA (OR TRSDA).
5: Time between Start and Stop conditions.
© 2010 Microchip Technology Inc. DS22187E-page 11
MCP4728
2.0 TYPICAL PERFORMANCE CURVES
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-1: INL vs. Code (TA = +25°C).
FIGURE 2-2: INL vs. Code (TA = +25°C).
FIGURE 2-3: INL vs. Code (TA = +25°C).
FIGURE 2-4: DNL vs. Code (TA = +25°C).
FIGURE 2-5: DNL vs. Code (TA = +25°C).
FIGURE 2-6: DNL vs. Code (TA = +25°C).
Note: The graphs and tables provided following this note are a statistical summary based on a limited number of
samples and are provided for informational purposes only. The performance characteristics listed herein
are not tested or guaranteed. In some graphs or tables, the data presented may be outside the specified
operating range (e.g., outside specified power supply range) and therefore, outside the warranted range.
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
VDD = 5.5V, VREF = Internal, Gain = x1
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
VDD = 5.5V, VREF = Internal, Gain = x2
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
VDD = 5.5V, VREF = VDD
-0.2
-0.1
0
0.1
0.2
0.3
0 1024 2048 3072 4096
Code
DNL(LSB)
VDD = 5.5V, VREF = Internal, Gain = x1
-0.2
-0.1
0
0.1
0.2
0.3
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 5.5V, VREF = Internal, Gain = x2
-0.1
-0.05
0
0.05
0.1
0.15
0.2
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 5.5V, VREF = VDD
MCP4728
DS22187E-page 12 © 2010 Microchip Technology Inc.
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-7: INL vs. Code (TA = +25°C).
FIGURE 2-8: INL vs. Code (TA = +25°C).
FIGURE 2-9: INL vs. Code and
Temperature.
FIGURE 2-10: DNL vs. Code (TA = +25°C).
FIGURE 2-11: DNL vs. Code (TA = +25°C).
FIGURE 2-12: DNL vs. Code and
Temperature.
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
VDD = 2.7V, VREF = Internal, Gain = x1
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
VDD = 2.7V, VREF = VDD
-10
-8
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
+125o
C
VDD = 5.5V, VREF = Internal, Gain = x1
+85°C
+25o
C
-40o
C
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 2.7V, VREF = Internal, Gain = x1
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 2.7V, VREF = VDD
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0 1024 2048 3072 4096
Code
DNL(LSB)
- 40o
C to +85o +125 C o
C
VDD = 5.5V, VREF = Internal, Gain = x1 
© 2010 Microchip Technology Inc. DS22187E-page 13
MCP4728
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-13: INL vs. Code and
Temperature.
FIGURE 2-14: INL vs. Code and
Temperature.
FIGURE 2-15: INL vs. Code and
Temperature.
FIGURE 2-16: DNL vs. Code and
Temperature.

FIGURE 2-17: DNL vs. Code and
Temperature.
FIGURE 2-18: DNL vs. Code and
Temperature.
-10
-8
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
+125o
C
+85o - 40 C o
C +25o
C
VDD = 5.5V, VREF = Internal, Gain = x2
-10
-8
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
+25o
C
+125o
C
+85o
C
- 40o
C
VDD = 2.7V, VREF = Internal, Gain = x1
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
+125o
C
+85o
C
+25o
C
- 40o
C
VDD = 5.5V, VREF = VDD
-0.3
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 5.5V, VREF = Internal, Gain = x2
+125o
C - 40o
C to +85o
C
-0.3
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0.5
0 1024 2048 3072 4096
Code
DNL (LSB)
+125o
C - 40o
C to +85o
C
VDD = 2.7V, VREF = Internal, Gain = x1
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0 1024 2048 3072 4096
Code
DNL (LSB)
VDD = 5.5V, VREF = VDD
+125o
C - 40o
C to +85o
C
MCP4728
DS22187E-page 14 © 2010 Microchip Technology Inc.
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-19: INL vs. Code and
Temperature.
FIGURE 2-20: Full Scale Error vs.
Temperature (Code = FFFh, VREF = Internal).
FIGURE 2-21: Full Scale Error vs.
Temperature (Code = FFFh, VREF = VDD).
FIGURE 2-22: DNL vs. Code and
Temperature.
FIGURE 2-23: Zero Scale Error vs.
Temperature (Code = 000h, VREF = Internal).
FIGURE 2-24: Offset Error (Zero Scale
Error).
-8
-6
-4
-2
0
2
4
6
0 1024 2048 3072 4096
Code
INL (LSB)
+125o
C
- 40o
C +85o
C
+25o
C
VDD = 2.7V, VREF = VDD
-50
-40
-30
-20
-10
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
Full Scale Error (mV)
VDD = 2.7V, Gain = 1
VDD = 5.5V, Gain = 1
VDD = 5.5V, Gain = 2
10
20
30
40
50
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
Full Scale Error (mV)
VDD = 2.7V, Gain = 1
VDD = 5.5V, Gain = 1
-0.3
-0.2
-0.1
0
0.1
0.2
0.3
0.4
0.5
0 1024 2048 3072 4096
Code
DNL (LSB)
+125o
C - 40o
C to +85o
C
VDD = 2.7V, VREF = VDD
0
1
2
3
4
5
6
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
Offset Error (mV)
VDD = 2.7V, Gain = 1
VDD = 5.5V, Gain = 1
VDD = 5.5V, Gain = 2
0
1
2
3
4
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
Offset Error (mV)
VDD = 2.7V
VDD = 5.5V
© 2010 Microchip Technology Inc. DS22187E-page 15
MCP4728
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.

FIGURE 2-25: Absolute DAC Output Error
(VDD = 5.5V).
FIGURE 2-26: Full Scale Settling Time
(VREF = VDD, VDD = 5V, UDAC = 1,
Code Change: 000h to FFFh).
FIGURE 2-27: Half Scale Settling Time
(VREF = VDD, VDD = 5V, UDAC = 1,
Code Change: 000h to 7FFh).
FIGURE 2-28: Full Scale Settling Time
(VREF = Internal, VDD = 5V, UDAC = 1,
Gain = x1, Code Change: 000h to FFFh).

FIGURE 2-29: Full Scale Settling Time
(VREF = VDD, VDD = 5V, UDAC = 1,
Code Change: FFFh to 000h).
FIGURE 2-30: Half Scale Settling Time
(VREF = VDD, VDD = 5V, UDAC = 1,
Code Change: 7FFh to 000h).
0
2
4
6
8
10
12
14
16
0 500 1000 1500 2000 2500 3000 3500
Codes
LSB
VREF = Internal, Gain = x2
2
Temp = +25o
C
Ch. D
Ch. C
Ch. B
Ch. A
VOUT (2V/Div)
LDAC Time (2 µs/Div)
VOUT (2V/Div)
LDAC Time (2 µs/Div)
VOUT (2V/Div)
 Time (2 µs/Div) LDAC
Time (2 µs/Div)
VOUT (2V/Div)
LDAC
Time (2 µs/Div)
VOUT (2V/Div)
LDAC
MCP4728
DS22187E-page 16 © 2010 Microchip Technology Inc.
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-31: Full Scale Settling Time
(VREF = Internal, VDD = 5V, UDAC = 1,
Gain = x1, Code Change: FFFh to 000h).
FIGURE 2-32: Half Scale Settling Time
(VREF = Internal, VDD = 5V, UDAC = 1,
Gain = x1, Code Change: 000h to 7FFh).
FIGURE 2-33: Exiting Power Down Mode
(Code: FFFh, VREF = Internal, VDD = 5V,
Gain = x1, for all Channels.).
FIGURE 2-34: Entering Power Down Mode
(Code: FFFh, VREF = Internal, VDD = 5V,
Gain = x1, PD1= PD0 = 1, No External Load).
FIGURE 2-35: Half Scale Settling Time
(VREF = Internal, VDD = 5V, UDAC = 1,
Gain = x1, Code Change: 7FFh to 000h).
FIGURE 2-36: Exiting Power Down Mode
(Code: FFFh, VREF = VDD, VDD = 5V, for all
Channels).
VOUT (2V/Div)
Time (2 µs/Div) LDAC
VOUT (1V/Div)
LDAC Time (2 µs/Div)
VOUT (1V/Div)
CLK
Time (5 µs/Div)
TdExPD
Last ACK CLK pulse
VOUT (1V/Div)
Time (10 µs/Div)
CLK
Discharging Time due to
internal pull-down resistor (500 kΩ)
Last ACK CLK pulse
VOUT (1V/Div)
LDAC Time (2 µs/Div)
VOUT (2V/Div)
Time (5 µs/Div)
CLK Last ACK CLK pulse
TdExPD
© 2010 Microchip Technology Inc. DS22187E-page 17
MCP4728
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-37: Entering Power Down Mode
(Code: FFFh, VREF = VDD, VDD = 5V,
PD1= PD0 = 1, No External Load).
FIGURE 2-38: VOUT Time Delay when
VREF changes from Internal Reference to VDD.
FIGURE 2-39: VOUT Time Delay when
VREF changes from VDD to Internal Reference.
FIGURE 2-40: Channel Cross Talk
(VREF = VDD, VDD = 5V).

FIGURE 2-41: Code Change Glitch
(VREF = External, VDD = 5V, No External Load),
Code Change: 800h to 7FFh.
FIGURE 2-42: Code Change Glitch
(VREF = Internal, VDD = 5V, Gain = 1, No External
Load), Code Change: 800h to 7FFh.
Time (20 µs/Div)
VOUT (2V/Div)
CLK
Discharging Time due to internal pull-down resistor (500 kΩ)
Last ACK CLK pulse
VOUT (2V/Div)
CLK
Time (10 µs/Div)
Last ACK CLK pulse
VOUT (2V/Div)
CLK
Time (10 µs/Div)
Last ACK CLK pulse
Time (5 µs/Div)
VOUT at Channel D
VOUT at Channel A
(100 mV/Div)
(5V/Div)
LDAC
VOUT (50 mV/Div)
Time (2 µs/Div)
VOUT (50 mV/Div)
Time (2 µs/Div)
MCP4728
DS22187E-page 18 © 2010 Microchip Technology Inc.
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-43: VOUT vs. Resistive Load.
FIGURE 2-44: IDD vs. Temperature
(VREF = VDD, VDD = 5V, Code = FFFh).
FIGURE 2-45: IDD vs. Temperature
(VREF = VDD, VDD = 2.7V, Code = FFFh).
FIGURE 2-46: IDD vs. Temperature
(VREF = VDD, All channels are in Normal Mode,
Code = FFFh).
FIGURE 2-47: IDD vs. Temperature
(VREF = Internal, VREF = 5V, Code = FFFh).
FIGURE 2-48: IDD vs. Temperature
(VREF = Internal, VDD = 2.7V, Code = FFFh).
0
1
2
3
4
5
6
012345
Load Resistance (kΩ)
VOUT (V)
VDD = 5V
VREF = VDD
Code = FFFh
VDD = 5.0V
0
200
400
600
800
1000
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_EXT (µA)
All Channels On
3 Channels On
2 Channels On
1 Channel On
VDD = 2.7V
0
200
400
600
800
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_EXT (µA)
All Channels On
3 Channels On
2 Channels On
1 Channel On
500
600
700
800
900
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_EXT (µA)
VDD = 5.5V
VDD = 3.3V
VDD = 4.5V
VDD = 5V
VDD = 2.7V
All Channels On
VDD = 5.0V
0
200
400
600
800
1000
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_INT (µA)
3 Channels On
2 Channels On
1 Channel On
All Channels On
VDD = 2.7V
0
200
400
600
800
1000
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_INT (µA)
1 Channel On
2 Channels On
3 Channels On
All Channels On
© 2010 Microchip Technology Inc. DS22187E-page 19
MCP4728
Note: Unless otherwise indicated, TA = -40°C to +125°C, VDD = +5.0V, VSS = 0V, RL = 5 kΩ, CL = 100 pF.
FIGURE 2-49: IDD vs. Temperature
(VREF = Internal , All Channels are in Normal
Mode, Code = FFFh).
FIGURE 2-50: IDD vs. Temperature
(VREF = Internal , All Channels are in Powered
Down).
FIGURE 2-51: Source Current Capability
(VREF = VDD, Code = FFFh).
FIGURE 2-52: Sink Current Capability
(VREF = VDD, Code = 000h).
All Channels On
500
600
700
800
900
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDD_INT (µA)
VDD = 2.7V
VDD = 3.3V
VDD = 4.5V
VDD = 5.5V
VDD = 5V
All Channels Off
20
30
40
50
60
-40 -25 -10 5 20 35 50 65 80 95 110 125
Temperature (o
C)
IDDP_INT (µA)
VDD = 2.7V VDD = 3.3V
VDD = 5V
VDD = 4.5V
VDD = 5.5V
Code = FFFh
0
1
2
3
4
5
6
0 2 4 6 8 10 12 14 16
Current (mA)
VOUT (V)
0
1
2
3
4
5
6
0 2 4 6 8 10 12 14
Sink Current (mA)
VOUT (V)
Code = 000h
MCP4728
DS22187E-page 20 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 21
MCP4728
3.0 PIN DESCRIPTIONS
The descriptions of the pins are listed in Table 3-1.
3.1 Supply Voltage Pins (VDD, VSS)
VDD is the power supply pin for the device. The voltage
at the VDD pin is used as a power supply input as well
as a DAC external reference. The power supply at the
VDD pin should be as clean as possible for a good DAC
performance.
It is recommended to use an appropriate bypass
capacitor of about 0.1 µF (ceramic) to ground. An
additional 10 µF capacitor (tantalum) in parallel is also
recommended to further attenuate high-frequency
noise present in application boards. The supply voltage
(VDD) must be maintained in the 2.7V to 5.5V range for
specified operation.
VSS is the ground pin and the current return path of the
device. The user must connect the VSS pin to a ground
plane through a low-impedance connection. If an
analog ground path is available in the application
printed circuit board (PCB), it is highly recommended
that the VSS pin be tied to the analog ground path, or
isolated within an analog ground plane of the circuit
board.
3.2 Serial Clock Pin (SCL)
SCL is the serial clock pin of the I2C interface. The
MCP4728 device acts only as a slave and the SCL pin
accepts only external input serial clocks. The input data
from the Master device is shifted into the SDA pin on
the rising edges of the SCL clock, and output from the
MCP4728 occurs at the falling edges of the SCL clock.
The SCL pin is an open-drain N-channel driver.
Therefore, it needs a pull-up resistor from the VDD line
to the SCL pin.
Refer to Section 5.0 “I2C Serial Interface
Communications” for more details on I2C Serial
Interface communication.
Typical range of the pull-up resistor value for SCL and
SDA is from 5 kΩ to 10 kΩ for Standard (100 kHz) and
Fast (400 kHz) modes, and less than 1 kΩ for High
Speed mode (3.4 MHz).
TABLE 3-1: PIN FUNCTION TABLE
Pin No. Name Pin Type Function
1 VDD P Supply Voltage
2 SCL OI I2C Serial Clock Input (Note 1)
3 SDA OI/OO I2C Serial Data Input and Output (Note 1)
4 LDAC ST This pin is used for two purposes:
(a) Synchronization Input. It is used to transfer the contents of the DAC input
registers to the output registers (VOUT).
(b) Select the device for reading and writing I2C address bits. (Note 2)
5 RDY/BSY OO This pin is a status indicator of EEPROM programming activity. An external pull-up
resistor (about 100 kΩ) is needed from RDY/BSY pin to VDD line. (Note 1)
6 VOUT A AO Buffered analog voltage output of channel A. The output amplifier has rail-to-rail
operation.
7 VOUT B AO Buffered analog voltage output of channel B. The output amplifier has rail-to-rail
operation.
8 VOUT C AO Buffered analog voltage output of channel C. The output amplifier has rail-to-rail
operation.
9 VOUT D AO Buffered analog voltage output of channel D. The output amplifier has rail-to-rail
operation.
10 VSS P Ground reference.
Legend: P = Power, OI = Open-Drain Input, OO = Open-Drain Output, ST = Schmitt Trigger Input Buffer,
 AO = Analog Output
Note 1: This pin needs an external pull-up resistor from VDD line. Leave this pin float if it is not used.
2: This pin can be driven by MCU.
MCP4728
DS22187E-page 22 © 2010 Microchip Technology Inc.
3.3 Serial Data Pin (SDA)
SDA is the serial data pin of the I2C interface. The SDA
pin is used to write or read the DAC register and
EEPROM data. Except for Start and Stop conditions,
the data on the SDA pin must be stable during the high
duration of the clock pulse. The High or Low state of the
SDA pin can only change when the clock signal on the
SCL pin is Low.
The SDA pin is an open-drain N-channel driver.
Therefore, it needs a pull-up resistor from the VDD line
to the SDA pin.
Refer to Section 5.0 “I2C Serial Interface
Communications” for more details on the I2C Serial
Interface communication.
3.4 LDAC Pin
This pin can be driven by an external control device
such as an MCU I/O pin. This pin is used to:
a) transfer the contents of the input registers to
their corresponding DAC output registers and
b) select a device of interest when reading or writing I2C address bits.
For more details on reading and writing the I2C address
bits, see Section 5.4.4 “General Call Read Address
Bits” and Section 5.6.8 “Write Command: Write I2C
Address bits (C2=0, C1=1, C0=1)”.
When the logic status of the LDAC pin changes from
“High” to “Low”, the contents of all input registers
(Channels A – D) are transferred to their corresponding
output registers, and all analog voltage outputs are
updated simultaneously.
If this pin is permanently tied to “Low”, the content of
the input register is transferred to its output register
(VOUT) immediately at the last input data byte’s
acknowledge pulse.
The user can also use the UDAC bit instead. However,
the UDAC bit updates a selected channel only. See
Section 4.8 “Output Voltage Update” for more
information on the LDAC pin and UDAC bit functions.
3.5 RDY/BSY Status Indicator Pin
This pin is a status indicator of EEPROM programming
activity. This pin is “High” when the EEPROM has no
programming activity, and “Low” when the EEPROM is
in programming mode. It goes “High” when the
EEPROM program is completed.
The RDY/BSY pin is an open-drain N-channel driver.
Therefore, it needs a pull-up resistor (about 100 kΩ)
from the VDD line to the RDY/BSY pin. Let this pin float
if it is not used.
3.6 Analog Output Voltage Pins
(VOUT A, VOUT B, VOUT C, VOUT D)
The device has four analog voltage output (VOUT) pins.
Each output is driven by its own output buffer with a
gain of 1 or 2, depending on the gain and VREF
selection bit settings. In Normal mode, the DC
impedance of the output pin is about 1Ω. In
Power-Down mode, the output pin is internally
connected to 1 kΩ, 100 kΩ, or 500 kΩ, depending on
the Power-Down selection bit settings.
The VOUT pin can drive up to 1000 pF of capacitive
load. It is recommended to use a load with RL greater
than 5 kΩ.
© 2010 Microchip Technology Inc. DS22187E-page 23
MCP4728
4.0 THEORY OF DEVICE
OPERATION
The MCP4728 device is a 12-bit 4-channel buffered
voltage output DAC with nonvolatile memory
(EEPROM). The user can program the EEPROM with
I
2C address bits, configuration and DAC input data of
each channel. The device has an internal charge pump
circuit to provide the programming voltage of the
EEPROM.
When the device is first powered-up, it automatically
loads the stored data in its EEPROM to the DAC input
and output registers, and provides analog outputs with
the saved settings immediately. This event does not
require an LDAC or UDAC bit condition. After the
device is powered-up, the user can update the input
registers using I2C write commands. The analog
outputs can be updated with new register values if the
LDAC pin or UDAC bit is low. The DAC output of each
channel is buffered with a low power and precision
output amplifier. This amplifier provides a rail-to-rail
output with low offset voltage and low noise.
The device uses a resistor string architecture. The
resistor ladder DAC can be driven from VDD or internal
VREF, depending on the reference selection. The user
can select internal (2.048V) or external reference (VDD)
for each DAC channel individually by software control.
The VDD is used as the external reference. Each
channel is controlled and operated independently.
The device has a Power-Down mode feature. Most of
the circuit in each powered down channel are turned
off. Therefore, operating power can be saved
significantly by putting any unused channel to the
Power-Down mode.
4.1 Power-on Reset (POR)
The device contains an internal Power-on Reset (POR)
circuit that monitors power supply voltage (VDD) during
operation. This circuit ensures correct device start-up
at system power-up and power-down events.
If the power supply voltage is less than the POR
threshold (VPOR = 2V, typical), all circuits are disabled
and there will be no analog output. When the VDD
increases above the VPOR, the device takes a reset
state. During the reset period, each channel uploads all
configuration and DAC input codes from EEPROM,
and analog output (VOUT) will be available accordingly.
This enables the device to return to the same state that
it was at the last write to the EEPROM, before it was
powered off. The POR status is monitored by the POR
status bit by using the I2C read command. See
Figure 5-15 for the details of the POR status bit.
4.2 Reset Conditions
The device can be reset by two independent events:
a) by Power-on Reset
b) by I2C General Call Reset Command
Under the reset conditions, the device uploads the
EEPROM data into both of the DAC input and output
registers simultaneously. The analog output voltage of
each channel is available immediately, regardless of
the LDAC and UDAC bit conditions.
The factory default settings for the EEPROM prior to
the device shipment are shown in Table 4-2.
4.3 Output Amplifier
The DAC output is buffered with a low power precision
amplifier. This amplifier provides low offset voltage and
low noise, as well as rail-to-rail output.
The output amplifier can drive the resistive and high
capacitive loads without oscillation. The amplifier can
provide a maximum load current of 24 mA, which is
enough for most of programmable voltage reference
applications. Refer to Section 1.0 “Electrical
Characteristics” for the specifications of the output
amplifier.
4.3.1 PROGRAMMABLE GAIN BLOCK
The rail-to-rail output amplifier of each channel has
configurable gain option. When the internal voltage
reference is selected, the output amplifier gain has two
selection options: Gain of 1 or Gain of 2.
When the external reference is selected (VREF = VDD),
the Gain of 2 option is disabled, and only the Gain of 1
is used by default.
4.3.1.1 Resistive and Capacitive Loads
The analog output (VOUT) pin is capable of driving
capacitive loads up to 1000 pF in parallel with 5 kΩ
load resistance. Figure 2-43 shows the VOUT vs.
Resistive Load. 
MCP4728
DS22187E-page 24 © 2010 Microchip Technology Inc.
4.4 DAC Input Registers and
Non-Volatile EEPROM Memory
Each channel has its own volatile DAC input register
and EEPROM. The details of the input registers and
EEPROM are shown in Table 4-1 and Table 4-2,
respectively.
TABLE 4-1: INPUT REGISTER MAP (VOLATILE)
Configuration Bits DAC Input Data (12 bits)
Bit
Name
RDY
/BSY
A2 A1 A0 VREF DAC1 DAC0 PD1 PD0 GX D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
Bit
Function
(Note 1)
I
2C
Address Bits
(Note 2)
Ref.
Select
(Note 2)
DAC Channel
(Note 2)
Power-Down
Select
(Note 2)
Gain
Select
(Note 2)
(Note 2)
CH. A
CH. B
CH. C
CH. D
Note 1: EEPROM write status indication bit (flag).
2: Loaded from EEPROM during power-up, or can be updated by the user.
TABLE 4-2: EEPROM MEMORY MAP AND FACTORY DEFAULT SETTINGS
Configuration Bits DAC Input Data (12 bits)
Bit Name A2 A1 A0 VREF PD1 PD0 GX D11 D10 D9 D8 D7 D6 D5 D4 D3 D2 D1 D0
Bit
Function I
2C Address Bits
(Note 1)
Ref.
Select
(Note 2)
Power-Down
Select
Gain
Select
(Note 3)
CH. A 0 0 0 1 0 0 0 0 0 0 00000 0000
CH. B 1 0 0 0 0 0 0 00000 0000
CH. C 1 0 0 0 0 0 0 00000 0000
CH. D 1 0 0 0 0 0 0 00000 0000
Note 1: Device I2C address bits. The user can also specify these bits during the device ordering process. The
factory default setting is “000”. These bits can be reprogrammed by the user using the I2C Address Write
command.
2: Voltage Reference Select: 0 = External VREF (VDD), 1 = Internal VREF (2.048V).
3: Gain Select: 0 = Gain of 1, 1 = Gain of 2. 
© 2010 Microchip Technology Inc. DS22187E-page 25
MCP4728
TABLE 4-3: CONFIGURATION BITS
Bit Name Functions
RDY/BSY This is a status indicator (flag) of EEPROM programming activity:
1 = EEPROM is not in programming mode
0 = EEPROM is in programming mode
Note: RDY/BSY status can also be monitored at the RDY/BSY pin.
(A2, A1, A0) Device I2C address bits. See Section 5.3 “MCP4728 Device Addressing” for more details.
VREF Voltage Reference Selection bit:
0 = VDD
1 = Internal voltage reference (2.048V)
Note: Internal voltage reference circuit is turned off if all channels select external reference
(VREF = VDD).
DAC1, DAC0 DAC Channel Selection bits:
00 = Channel A
01 = Channel B
10 = Channel C
11 = Channel D
PD1, PD0 Power-Down selection bits:
00 = Normal Mode
01 = VOUT is loaded with 1 kΩ resistor to ground. Most of the channel circuits are powered off.
10 = VOUT is loaded with 100 kΩ resistor to ground. Most of the channel circuits are powered
off.
11 = VOUT is loaded with 500 kΩ resistor to ground. Most of the channel circuits are powered
off.
Note: See Table 4-7 and Figure 4-1 for more details.
GX Gain selection bit:
0 = x1 (gain of 1)
1 = x2 (gain of 2)
Note: Applicable only when internal VREF is selected. If VREF = VDD, the device uses a gain of 1
regardless of the gain selection bit setting.
UDAC DAC latch bit. Upload the selected DAC input register to its output register (VOUT):
0 = Upload. Output (VOUT) is updated.
1 = Do not upload.
Note: UDAC bit affects the selected channel only.
MCP4728
DS22187E-page 26 © 2010 Microchip Technology Inc.
4.5 Voltage Reference
The device has a precision internal voltage reference
which provides a nominal voltage of 2.048V. The user
can select the internal voltage reference or VDD as the
voltage reference source of each channel using the
VREF configuration bit. The internal voltage reference
circuit is turned off when all channels select VDD as
their references. However, it stays turned on if any one
of the channels selects the internal reference.
4.6 LSB Size
The LSB is defined as the ideal voltage difference
between two successive codes. LSB sizes of the
MCP4728 device are shown in Table 4-4.
4.7 DAC Output Voltage
Each channel has an independent output associated
with its own configuration bit settings and DAC input
code. When the internal voltage reference is selected
(VREF = internal), it supplies the internal VREF voltage
to the resistor string DAC of the channel. When the
external reference (VREF=VDD) is selected, VDD is used
for the channel’s resistor string DAC.
The VDD needs to be as clean as possible for accurate
DAC performance. When the VDD is selected as the
voltage reference, any variation or noises on the VDD
line can directly affect on the DAC output.
The analog output of each channel has a
programmable gain block. The rail-to-rail output
amplifier has a configurable gain of 1 or 2. But the gain
of 2 is not applicable if VDD is selected for the voltage
reference. The formula for the analog output voltage is
given in Equation 4-1 and Equation 4-2.
4.7.1 OUTPUT VOLTAGE RANGE
The DAC output voltage range varies depending on the
voltage reference selection.
• When the internal reference (VREF=2.048V) is
selected:
- VOUT = 0.000V to 2.048V * 4095/4096 for
Gain of 1
- VOUT = 0.000V to 4.096V * 4095/4096 for
Gain of 2
• When the external reference (VREF=VDD) is
selected:
- VOUT = 0.000V to VDD
EQUATION 4-1: VOUT FOR VREF =
INTERNAL REFERENCE
EQUATION 4-2: VOUT FOR VREF = VDD
4.8 Output Voltage Update
The following events update the output registers
(VOUT):
a. LDAC pin to “Low”: Updates all DAC channels.
b. UDAC bit to “Low”: Updates a selected channel
only.
c. General Call Software Update Command:
Updates all DAC channels.
d. Power-on Reset or General Call Reset
command: Both input and output registers are
updated with EEPROM data. All channels are
affected.
4.8.1 LDAC PIN AND UDAC BIT
The user can use the LDAC pin or UDAC bit to upload
the input DAC register to output DAC register (VOUT).
However, the UDAC affects only the selected channel
while the LDAC affects all channels. The UDAC bit is
not used in the Fast Mode Writing.
Table 4-5 shows the output update vs. LDAC pin and
UDAC bit conditions.
TABLE 4-4: LSB SIZES (EXAMPLE)
VREF
Gain (GX)
Selection LSB Size Condition
 Internal
VREF
(2.048V)
x1 0.5 mV 2.048V/4096
x2 1 mV 4.096V/4096
VDD x1 VDD/4096 (Note 1)
Note 1: LSB size varies with the VDD range.
When VREF = VDD, the device uses
GX = 1 by default. GX = 2 option is
ignored.
Note: The gain selection bit is not applicable
for VREF = VDD. In this case, Gain of 1
is used regardless of the gain selection
bit setting.
Where:
VREF = 2.048V for internal reference selection
Dn = DAC input code
Gx = Gain Setting
 (VREF x Dn ) x Gx 4096
≤ VDD VOUT =
VOUT
VDD Dn ( ) ×
4096 = ----------------------------
Where:
Dn = DAC input code
© 2010 Microchip Technology Inc. DS22187E-page 27
MCP4728
4.9 DAC Input Code Vs. DAC Analog
Output
Table 4-6 shows an example of the DAC input data
code vs. analog output. The MSB of the input data is
always transmitted first and the format is unipolar
binary.
TABLE 4-5: LDAC AND UDAC
CONDITIONS VS. OUTPUT
UPDATE
LDAC Pin UDAC Bit DAC Output (VOUT)
0 0 Update all DAC channel
outputs
0 1 Update all DAC channel
outputs
1 0 Update a selected DAC
channel output
1 1 No update
TABLE 4-6: DAC INPUT CODE VS. ANALOG OUTPUT (VOUT)
DAC Input Code
VREF = Internal (2.048 V) VREF = VDD
Gain
Selection
Nominal Output Voltage (V)
(See Note 1)
Gain
Selection Nominal Output Voltage (V)
111111111111 x1 VREF - 1 LSB Ignored VDD - 1 LSB
x2 2*VREF - 1 LSB
111111111110 x1 VREF - 2 LSB VDD - 2 LSB
x2 2*VREF - 2 LSB
000000000010 x1 2 LSB 2 LSB
x2 2 LSB
000000000001 x1 1 LSB 1 LSB
x2 1 LSB
000000000000 x1 0 0
x2 0
Note 1: (a) LSB with gain of 1 = 0.5 mV, and (b) LSB with gain of 2 = 1 mV. 
MCP4728
DS22187E-page 28 © 2010 Microchip Technology Inc.
4.10 Normal and Power-Down Modes
Each channel has two modes of operation: (a) Normal
mode where analog voltage is available and (b)
Power-Down mode which turns off most of the internal
circuits for power savings.
The user can select the operating mode of each
channel individually by setting the Power-Down
selection bits (PD1 and PD0). For example, the user
can select Normal mode for channel A while selecting
Power-Down mode for all other channels.
See Section 5.6 “Write Commands for DAC
Registers and EEPROM” for more details on the
writing the power-down bits.
Most of the internal circuit in the powered down
channel are turned off. However, the internal voltage
reference circuit is not affected by the Power-Down
mode. The internal voltage reference circuit is turned
off only if all channels select external reference (VREF
= VDD).
Device actions during Power-Down mode:
• The powered down channel stays in a
power-saving condition by turning off most of its
circuits
• No analog voltage output at the powered down
channel
• The output (VOUT) pin of the powered down
channel is switched to a known resistive load. The
value of the resistive load is determined by the
state of the Power-Down bits (PD1 and PD0).
Table 4-7 shows the outcome of the Power-Down
bit settings
• The contents of both the DAC registers and
EEPROM are not changed
• Draws less than 40 nA (typical) when all four
channels are powered down and VDD is selected
as the voltage reference
Circuits that are not affected during Power-Down
mode:
• The I2C serial interface circuits remain active in
order to receive any command from the Master
• The internal voltage reference circuit stays
turned-on if it is selected as reference by at least
one channel
Exiting Power-Down Mode:
The device exits Power-Down mode immediately by
the following commands:
• Any write command for normal mode. Only
selected channel is affected
• I2C General Call Wake-Up Command. All
channels are affected
• I2C General Call Reset Command. This is a
conditional case. The device exits Power-Down
mode, depending on the Power-Down bit settings
in EEPROM as the configuration bits and DAC
input codes are uploaded from EEPROM. All
channels are affected
When the DAC operation mode is changed from the
Power-Down to Normal mode, there will be a time
delay until the analog output is available. Typical time
delay for the output voltage is approximately 4.5 µs.
This time delay is measured from the acknowledge
pulse of the I2C serial communication command to the
beginning of the analog output (VOUT). This time delay
is not included in the output settling time specification.
See Section 2.0 “Typical Performance Curves” for
more details.
FIGURE 4-1: Output Stage for
Power-Down Mode.
TABLE 4-7: POWER-DOWN BITS
PD1 PD0 Function
0 0 Normal Mode
0 1 1 kΩ resistor to ground (Note 1)
1 0 100 kΩ resistor to ground
(Note 1)
1 1 500 kΩ resistor to ground
(Note 1)
Note 1: In Power-Down mode: VOUT is off and
most of internal circuits in the selected
channel are disabled.
1 kΩ 100 kΩ 500 kΩ
Power-Down
Control Circuit
Resistive
Load
VOUT
OP
Amp
Resistor String DAC
© 2010 Microchip Technology Inc. DS22187E-page 29
MCP4728
5.0 I2C SERIAL INTERFACE
COMMUNICATIONS
The MCP4728 device uses a two-wire I2C serial
interface. When the device is connected to the I2C bus
line, the device works as a slave device. The device
supports standard, fast and high speed modes.
The following sections describe how to communicate
with the MCP4728 device using the I2C serial interface
commands.
5.1 Overview of I2C Serial Interface
Communications
An example of the hardware connection diagram is
shown in Figure 7-1. A device that sends data onto the
bus is defined as the transmitter, and a device receiving
data, as the receiver. The bus has to be controlled by a
master (MCU) device which generates the serial clock
(SCL), controls the bus access and generates the
START and STOP conditions. Both master (MCU) and
slave (MCP4728) can operate as transmitter or
receiver, but the master device determines which mode
is activated.
Communication is initiated by the master (MCU) which
sends the START bit, followed by the slave (MCP4728)
address byte. The first byte transmitted is always the
slave (MCP4728) address byte, which contains the
device code (1100), the address bits (A2, A1, A0), and
the R/W bit. The device code for the MCP4728 device
is 1100, and the address bits are user-writable.
When the MCP4728 device receives a Read command
(R/W = 1), it transmits the contents of the DAC input
registers and EEPROM sequentially. When writing to
the device (R/W = 0), the device will expect Write
command type bits in the following byte. The reading
and various writing commands are explained in the
following sections.
The MCP4728 device supports all three I2C serial
communication operating modes:
• Standard Mode: bit rates up to 100 kbit/s
• Fast Mode: bit rates up to 400 kbit/s
• High Speed Mode (HS mode): bit rates up to
3.4 Mbit/s
Refer to the Philips I2C document for more details of
the I2C specifications.
5.1.1 HIGH-SPEED (HS) MODE
The I2C specification requires that a high-speed mode
device must be ‘activated’ to operate in High-Speed
(3.4 Mbit/s) mode. This is done by sending a special
address byte of 00001XXX following the START bit.
The XXX bits are unique to the high-speed mode
Master. This byte is referred to as the high-speed
Master Mode Code (HSMMC). The MCP4728 device
does not acknowledge this byte. However, upon
receiving this command, the device switches to HS
mode and can communicate at up to 3.4 Mbit/s on SDA
and SCL lines. The device will switch out of the HS
mode on the next STOP condition.
For more information on the HS mode, or other I2C
modes, please refer to the Philips I2C specification.
5.2 I2C BUS CHARACTERISTICS
The specification of the I2C serial communication
defines the following bus protocol:
• Data transfer may be initiated only when the bus
is not busy
• During data transfer, the data line must remain
stable whenever the clock line is HIGH. Changes
in the data line while the clock line is HIGH will be
interpreted as a START or STOP condition
Accordingly, the following bus conditions have been
defined using Figure 5-1.
5.2.1 BUS NOT BUSY (A)
Both data and clock lines remain HIGH.
5.2.2 START DATA TRANSFER (B)
A HIGH to LOW transition of the SDA line, while the
clock (SCL) is HIGH, determines a START condition.
All commands must be preceded by a START
condition.
5.2.3 STOP DATA TRANSFER (C)
A LOW to HIGH transition of the SDA line, while the
clock (SCL) is HIGH, determines a STOP condition. All
operations must be ended with a STOP condition.
5.2.4 DATA VALID (D)
The state of the data line represents valid data when,
after a START condition, the data line is stable for the
duration of the HIGH period of the clock signal.
The data on the line must be changed during the LOW
period of the clock signal. There is one clock pulse per
bit of data.
Each data transfer is initiated with a START condition
and terminated with a STOP condition.
MCP4728
DS22187E-page 30 © 2010 Microchip Technology Inc.
5.2.5 ACKNOWLEDGE
Each receiving device, when addressed, is obliged to
generate an acknowledge after the reception of each
byte. The master device must generate an extra clock
pulse, which is associated with this acknowledge bit.
The device that acknowledges has to pull down the
SDA line during the acknowledge clock pulse in such a
way that the SDA line is stable LOW during the HIGH
period of the acknowledge related clock pulse. Of
course, setup and hold times must be taken into
account. During reads, a master must send an end of
data to the slave by not generating an acknowledge bit
on the last byte that has been clocked out of the slave.
In this case, the slave (MCP4728) will leave the data
line HIGH to enable the master to generate the STOP
condition.
FIGURE 5-1: Data Transfer Sequence On The Serial Bus.
5.3 MCP4728 Device Addressing
The address byte is the first byte received following the
START condition from the master device. The first part
of the address byte consists of a 4-bit device code,
which is set to 1100 for the MCP4728 device. The
device code is followed by three I2C address bits (A2,
A1, A0) which are programmable by the users.
Although the three address bits are programmable at
the user’s application PCB, the user can also specify
the address bits during the product ordering process. If
there is no user’s request, the factory default setting of
the three address bits is “000”, programmed into the
EEPROM. The three address bits allow eight unique
addresses.
FIGURE 5-2: Device Addressing.
5.3.1 PROGRAMMING OF I2C ADDRESS
BITS
When the customer first receives any new MCP4728
device, its default address bit setting is “000” if the
address bit programming was not requested. The
customer can reprogram the I2C address bits into the
EEPROM by using “Write Address Bit” command. This
write command needs current address bits. If the
address bits are unknown, the user can find them by
sending “General Call Read Address” Command. The
LDAC pin is also used to select the device of interest to
be programmed or to read the current address.
The following steps are needed for the I2C address
programming.
(a) Read the address bits using “General Call Read
Address” Command. (This is the case when the
address is unknown.)
(b) Write I2C address bits using “Write I2C Address
Bits” Command.
The Write Address command will replace the current
address with a new address in both input registers and
EEPROM.
See Section 5.4.4 “General Call Read Address Bits”
for the details of reading the address bits, and
Section 5.6.8 “Write Command: Write I2C Address
bits (C2=0, C1=1, C0=1)” for writing the address bits.
SCL
SDA
(A) (B) (D) (D) (A) (C)
START
CONDITION
ADDRESS OR
ACKNOWLEDGE
VALID
DATA
ALLOWED
TO CHANGE
STOP
CONDITION
Start bit Read/Write bit
Address Byte
R/W ACK
Acknowledge bit
Slave Address
110 0
Slave Address for MCP4728
A2 A1 A0
Device Code: Programmed (hard-wired) at the
factory.
Address Bits: Reprogrammable into EEPROM by
the user.
Device Code Address Bits
© 2010 Microchip Technology Inc. DS22187E-page 31
MCP4728
5.4 I2C General Call Commands
The device acknowledges the general call address
command (0x00 in the first byte). The meaning of the
general call address is always specified in the second
byte. The I2C specification does not allow the use of
“00000000” (00h) in the second byte. Refer to the
Philips I2C document for more details of the General
Call specifications.
The MCP4728 device supports the following I2C
General Calls:
• General Call Reset
• General Call Wake-Up
• General Call Software Update
• General Call Read Address Bits
5.4.1 GENERAL CALL RESET
The General Call Reset occurs if the second byte is
“00000110” (06h). At the acknowledgement of this
byte, the device will abort the current conversion and
perform the following tasks:
• Internal Reset similar to a Power-on Reset (POR).
The contents of the EEPROM are loaded into
each DAC input and output registers immediately
• VOUT will be available immediately regardless of
the LDAC pin condition
FIGURE 5-3: General Call Reset.
5.4.2 GENERAL CALL WAKE-UP
If the second byte is “00001001” (09h), the device will
reset the Power-Down bits (PD1, PD0 = 0,0).
FIGURE 5-4: General Call Wake-Up.
1st Byte
Note 1
1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9
Start Stop
2nd Byte
ACK (MCP4728)
(General Call Command) (Command Type = General Call Reset)
Clock Pulse (CLK Line)
Data (SDA Line)
Note 1: At this falling edge of the last ACK clock bit:
a. Startup Timer starts a reset sequence and
b. EEPROM data is loaded into the DAC Input and Output Registers immediately.
1st Byte Note 1
1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9
Start Stop
2nd Byte
ACK (MCP4728)
(General Call Command) (Command Type = General Call Wake-Up)
Clock Pulse (CLK Line)
Data (SDA Line)
Note 1: Resets Power-Down bits at this falling edge of the last ACK clock bit.
MCP4728
DS22187E-page 32 © 2010 Microchip Technology Inc.
5.4.3 GENERAL CALL SOFTWARE
UPDATE
If the second byte is “00001000” (08h), the device
updates all DAC analog outputs (VOUT) at the same
time.
FIGURE 5-5: General Call Software Update.
1st Byte
Note 1
1 2 3 4 5 6 7 8 9 1 2 3 4 5 6 7 8 9
ACK (MCP4728)
Start Stop
2nd Byte
(Command Type = General Call Software Update) (General Call Command)
Clock Pulse (CLK Line)
Data (SDA Line)
Note 1: At this falling edge of the last ACK clock bit, VOUT A, VOUT B, VOUT C, VOUT D are updated.
© 2010 Microchip Technology Inc. DS22187E-page 33
MCP4728
5.4.4 GENERAL CALL READ ADDRESS
BITS
This command is used to read the I2C address bits of
the device. If the second byte is “00001100” (0Ch), the
device will output its address bits stored in EEPROM
and register. This command uses the LDAC pin to
select the device of interest to read on the I2C bus. The
LDAC pin needs a logic transition from “High” to “Low”
during the negative pulse of the 8th clock of the second
byte, and stays “Low” until the end of the 3rd byte. The
maximum clock rate for this command is 400 kHz.

FIGURE 5-6: General Call Read I2C Address.
ACK (MCP4728)
Restart
(General Call Address)
1st Byte 2nd Byte Address Bits
in
Address Bits
in Input
Note 3 (Notes 1, 2, 3)
LDAC Pin
4th Byte
ACK (Master)
Start
3rd Byte
 Note 3
ACK Clock
Clock Pulse
LDAC Pin
2nd Byte 3rd Byte 4th Byte
Note 2 (a)
ACK Clock
Note 2(b)
Stay “Low” until the end of the 3rd Byte
Note 2(b, c)
Restart Clock
(CLK Line)
Reading Address Bits
Reading Address Bits
Stop
6 7 8 9 Sr 1 2 3 4 5 6 7 8 9 1 2 3
Note 1: Clock Pulse and LDAC Transition Details.
2: LDAC pin events at the 2nd and 3rd bytes.
a. Keep LDAC pin “High” until the end of the positive pulse of the 8th clock of the 2nd byte.
b. LDAC pin makes a transition from “High” to “Low” during the negative pulse of the 8th clock of the 2nd
byte (just before the rising edge of the 9th clock) and stays “Low” until the rising edge of clock 9 of the
3rd byte.
c. The MCP4728 device does not acknowledge the 3rd byte if the conditions (a) and (b) are not met.
3: LDAC pin resumes its normal function after “Stop” bit.
S00000000 A00001100 A Sr 1 1 0 0 X X X 1 A A2 A1 A0 1 A2 A1 A0 0 A P
Restart Byte EEPROM Register
Clock and LDAC Transition Details:
MCP4728
DS22187E-page 34 © 2010 Microchip Technology Inc.
5.5 Writing and Reading Registers
and EEPROM
The Master (MCU) can write or read the DAC input
registers or EEPROM using the I2C interface
command.
The following sections describe the communication
examples to write and read the DAC registers and
EEPROM using the I2C interface.
5.6 Write Commands for DAC
Registers and EEPROM
Table 5-1 summarizes the write command types and
their functions.The write command is defined by using
three write command type bits (C2, C1, C0) and two
write function bits (W1, W0). The register selection bits
(DAC1, DAC0) are used to select the DAC channel.
TABLE 5-1: WRITE COMMAND TYPES
Command Field Write
Function Command Name Function
C2 C1 C0 W1 W0
Fast Mode Write
0 0 X Not Used Fast Write for DAC
Input Registers
This command writes to the DAC input registers sequentially with
limited configuration bits. The data is sent sequentially from channels A
to D. The input register is written at the acknowledge clock pulse of the
channel’s last input data byte. EEPROM is not affected. (Note 1)
Write DAC Input Register and EEPROM
0 1 0 0 0 Multi-Write for DAC
Input Registers
This command writes to multiple DAC input registers, one DAC input
register at a time. The writing channel register is defined by the DAC
selection bits (DAC1, DAC0). EEPROM is not affected. (Note 2)
1 0 Sequential Write for
DAC Input Registers
and EEPROM
This command writes to both the DAC input registers and EEPROM
sequentially. The sequential writing is carried out from a starting
channel to channel D. The starting channel is defined by the DAC
selection bits (DAC1 and DAC0).
The input register is written at the acknowledge clock pulse of the last
input data byte of each register. However, the EEPROM data is written
altogether at the same time sequentially at the end of the last byte.
(Note 2),(Note 3)
1 1 Single Write for DAC
Input Register and
EEPROM
This command writes to a single selected DAC input register and its
EEPROM. Both the input register and EEPROM are written at the
acknowledge clock pulse of the last input data byte. The writing
channel is defined by the DAC selection bits (DAC1 and DAC0).
(Note 2),(Note 3)
Write I2C Address Bits (A2, A1, A0)
 0 1 1 Not Used Write I2C Address Bits This command writes new I2C address bits (A2, A1, A0) to the DAC
input register and EEPROM.
Write VREF, Gain, and Power-Down Select Bits (Note 4)
 1 0 0 Not Used Write Reference
(VREF) selection bits
to Input Registers
This command writes Reference (VREF) selection bits of each channel.
 1 1 0 Not Used Write Gain selection
bits to Input Registers
This command writes Gain selection bits of each channel.
 1 0 1 Not Used Write Power-Down
bits to Input Registers
This command writes Power-Down bits of each channel.
Note 1: The analog output is updated when LDAC pin is (or changes to) “Low”. UDAC bit is not used for this command.
2: The DAC output is updated when LDAC pin or UDAC bit is “Low”.
3: The device starts writing to the EEPROM on the acknowledge clock pulse of the last channel. The device does not
execute any command until RDY/BSY bit comes back to “High”.
4: The input and output registers are updated at the acknowledge clock pulse of the last byte. The update does not require
LDAC pin or UDAC bit conditions. EEPROM is not affected.
© 2010 Microchip Technology Inc. DS22187E-page 35
MCP4728
5.6.1 FAST WRITE COMMAND
(C2=0, C1=0, C0=X, X = DON’T
CARE)
The Fast Write command is used to update the input
DAC registers from channels A to D sequentially. The
EEPROM data is not affected by this command. This
command is called “Fast Write” because it updates the
input registers with only limited data bits. Only the
Power-Down mode selection bits (PD1 and PD0) and
12 bits of DAC input data are writable.
The input register is updated at the acknowledge pulse
of each channel’s last data byte. Figure 5-7 shows an
example of the Fast Write command.
Updating Analog Outputs:
a. When the LDAC pin is “High” before the last byte
of the channel D, all analog outputs are updated
simultaneously by bringing down the LDAC pin
to “Low” any time.
b. If the command starts with the LDAC pin “Low”,
the channel’s analog output is updated at the
falling edge of the acknowledge clock pulse of
the channel’s last byte.
c. Send the General Call Software Update
command: This command updates all channels
simultaneously.
5.6.2 MULTI-WRITE COMMAND: WRITE
DAC INPUT REGISTERS
(C2=0, C1=1, C0=0; W1=0, W0=0)
This command is used to write DAC input register, one
at a time. The EEPROM data is not affected by this
command.
The DAC selection bits (DAC1, DAC0) select the DAC
channel to write. Only a selected channel is affected.
Repeated bytes are used to write more multiple DAC
registers.
The D11 - D0 bits in the third and fourth bytes are the
DAC input data of the selected DAC channel.
Bytes 2 - 4 can be repeated for the other channels.
Figure 5-8 shows an example of the Multi-Write
command.
Updating Analog Outputs:
The analog outputs can be updated by one of the
following events after the falling edge of the
acknowledge clock pulse of the 4th byte.
a. When the LDAC pin or UDAC bit is “Low”.
b. If UDAC bit is “High”, bringing down the LDAC
pin to “Low” any time.
c. By sending the General Call Software Update
command.
Note: The UDAC bit is not used in this
command.
Note: The UDAC bit can be used effectively to
upload the input register to the output
register, but it affects only a selected
channel only, while the LDAC pin and
General Call Software Update command
affect all channels.
MCP4728
DS22187E-page 36 © 2010 Microchip Technology Inc.
5.6.3 SEQUENTIAL WRITE COMMAND:
WRITE DAC INPUT REGISTERS
AND EEPROM SEQUENTIALLY
FROM STARTING CHANNEL TO
CHANNEL D
(C2=0, C1=1, C0=0; W1=1, W0=0)
When the device receives this command, it writes the
input data to the DAC input registers sequentially from
the starting channel to channel D, and also writes to
EEPROM sequentially. The starting channel is
determined by the DAC1 and DAC0 bits. Table 5-2
shows the functions of the channel selection bits for the
sequential write command.
When the device is writing EEPROM, the RDY/BSY bit
stays “Low” until the EEPROM write operation is
completed. The state of the RDY/BSY bit flag can be
monitored by a read command or at the RDY/BSY pin.
Any new command received during the EEPROM write
operation (RDY/BSY bit is “Low”) is ignored. Figure 5-9
shows an example of the sequential write command.
Updating Analog Outputs:
The analog outputs can be updated by one of the
following events after the falling edge of the
acknowledge clock pulse of the 4th byte.
a. When the LDAC pin or UDAC bit is “Low”.
b. If UDAC bit is “High”, bringing down the LDAC
pin to “Low” any time.
c. By sending the General Call Software Update
command.
5.6.4 SINGLE WRITE COMMAND: WRITE
A SINGLE DAC INPUT REGISTER
AND EEPROM
(C2=0, C1=1, C0=0; W1=1, W0=1)
When the device receives this command, it writes the
input data to a selected single DAC input register and
also to its EEPROM. The channel is selected by the
channel selection bits (DAC1 and DAC0). See
Table 5-2 for the channel selection bit function.
Figure 5-10 shows an example of the single write
command.
Updating Analog Outputs:
The analog outputs can be updated by one of the
following events after the falling edge of the
acknowledge clock pulse of the 4th byte.
a. When the LDAC pin or UDAC bit is “Low”.
b. If UDAC bit is “High”, bringing down the LDAC
pin to “Low” any time.
c. By sending the General Call Software Update
command.
Note: The UDAC bit can be used effectively to
upload the input register to the output
register, but it affects only a selected
channel only, while the LDAC pin and
General Call Software Update command
affect all channels.
TABLE 5-2: DAC CHANNEL SELECTION
BITS FOR SEQUENTIAL
WRITE COMMAND
DAC1 DAC0 Channels
0 0 Ch. A - Ch. D
0 1 Ch. B - Ch. D
1 0 Ch. C - Ch. D
1 1 Ch. D
Note: The UDAC bit can be used effectively to
upload the input register to the output
register, but it affects only a selected
channel only, while the LDAC pin and
General Call Software Update command
affect all channels.
© 2010 Microchip Technology Inc. DS22187E-page 37
MCP4728
5.6.5 WRITE COMMAND: SELECT VREF
BIT (C2=1, C1=0, C0=0)
When the device receives this command, it updates the
DAC voltage reference selection bit (VREF) of each
channel. The EEPROM data is not affected by this
command. The affected channel’s analog output is
updated after the acknowledge pulse of the last byte.
Figure 5-12 shows an example of the write command
for Select VREF bits.
5.6.6 WRITE COMMAND: SELECT
POWER-DOWN BITS (C2=1, C1=0,
C0=1)
When the device receives this command, it updates the
Power-Down selection bits (PD1, PD0) of each
channel. The EEPROM data is not affected by this
command. The affected channel is updated after the
acknowledge pulse of the last byte. Figure 5-13 shows
an example of the write command for the Select
Power-Down bits.
5.6.7 WRITE COMMAND: SELECT GAIN
BIT (C2=1, C1=1, C0=0)
When the device receives this command, it updates the
gain selection bits (GX) of each channel. The EEPROM
data is not affected by this command. The analog
output is updated after the acknowledge pulse of the
last byte. Figure 5-14 shows an example of the write
command for select gain bits.
5.6.8 WRITE COMMAND: WRITE I2C
ADDRESS BITS (C2=0, C1=1, C0=1)
This command writes new I2C address bits (A2, A1,
A0) to the DAC input registers and EEPROM. When
the device receives this command, it overwrites the
current address bits with the new address bits.
This command is valid only when the LDAC pin makes
a transition from “High” to “Low” at the low time of the
last bit (8th clock) of the second byte, and stays “Low”
until the end of the third byte. The update occurs after
“Stop” bit, if the conditions are met. The LDAC pin is
used to select a device of interest to write. The highest
clock rate of this command is 400 kHz. Figure 5-11
shows the details of the address write command.
5.6.9 READ COMMAND
If the R/W bit is set to a logic “High” in the I2C serial
communications command, the device enters a
reading mode and reads out the input registers and
EEPROM. Figure 5-15 shows the details of the read
command.
Note: To write a new device address, the current
address of the device is also required. If
the current address is not known, it can be
read out by sending General Call Read
Address Bits command. See 5.4.4
“General Call Read Address Bits” for
more details of reading the I2C address
bits.
Note: The device address bits are read by using
General Call Read Address Bits
command. 
MCP4728
DS22187E-page 38 © 2010 Microchip Technology Inc.

FIGURE 5-7: Fast Write Command: Write DAC Input Registers Sequentially from Channel A to D.
1st byte
DAC Input Register of Channel A
2nd Byte
R/W
Device Addressing
ACK (MCP4728)
(C2 C1) 3rd Byte
Update Channel A DAC Input Register at this ACK pulse.
Repeat Bytes
Start
Stop
Fast Write
Note 1: X is a don’t care bit. VOUT can be updated after the last byte’s ACK pulse is issued and by bringing down the LDAC
pin to “Low”.
2nd Byte
ACK (MCP4728)
3rd Byte
Update Channel B DAC Input Register at this ACK pulse.
DAC Input Register of Channel B
S 1 1 0 0 A2 A1 A0 0 A 0 0 PD1 PD0 D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
Command Type Bits: C2=0 C1=0 C0=X
Command
X X PD1 PD0 D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
2nd Byte
ACK (MCP4728)
3rd Byte
Update Channel C DAC Input Register at this ACK pulse.
DAC Input Register of Channel C
X X PD1 PD0 D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
2nd Byte
ACK (MCP4728)
3rd Byte
Update Channel D DAC Input Register at this ACK pulse.
DAC Input Register of Channel D
X X PD1 PD0 D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
P
© 2010 Microchip Technology Inc. DS22187E-page 39
MCP4728

FIGURE 5-8: Multi-Write Command: Write Multiple DAC Input Registers.
(C2 C1 C0 W1 W2) 3rd Byte 4th Byte 2nd Byte
0 1 0 0 0 DAC1 DAC0 UDAC A VREF PD1 PD0 Gx D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
ACK (MCP4728)
1st byte
DAC Input Register of Selected Channel
R/W
Device Addressing
ACK (MCP4728)
Repeat Bytes of the 2nd - 4th Bytes
Start
Stop
Multi-Write
S 1 1 0 0 A2 A1 A0 0 A
Command
Note 1: VOUT Update:
If UDAC = 0 or LDAC Pin = 0: VOUT is updated after the 4th byte’s ACK is issued.
2: The user can write to the other channels by sending repeated bytes with new channel selection bits (DAC1, DAC0).
3: X is don’t care bit.
Command Type Bits: C2=0 C1=1 C0=0 W1=0 W0=0
Channel
Select
X X X X X DAC1 DAC0 UDAC A VREF PD1 PD0 Gx D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
ACK (MCP4728)
DAC Input Register of Selected Channel Note 3
2nd byte 3rd Byte 4th Byte
Repeat Bytes of the 2nd - 4th Bytes
Note 1
Note 1
P
Note 2
Channel
Select
MCP4728
DS22187E-page 40 © 2010 Microchip Technology Inc.

FIGURE 5-9: Sequential Write Command: Write DAC Input Registers and EEPROM Sequentially
from Starting Channel to Channel D. The sequential input register starts with the "Starting Channel" and
ends at Channel D. For example, if DAC1:DAC0 = 00, then it starts with channel A and ends at channel D.
If DAC1:DAC0 = 01, then it starts with channel B and ends at Channel D. Note that this command can
send up to 10 bytes including the device addressing and command bytes. Any byte after the 10th byte is
ignored.
(C2 C1 C0 W1 W2) 3rd Byte 4th Byte 2nd Byte
0 1 0 1 0 DAC1 DAC0 UDAC A VREF PD1 PD0 Gx D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A
ACK (MCP4728)
1st byte
DAC Input Register of Starting Channel
R/W
Device Addressing
ACK (MCP4728)
Start
Sequential Write
S 1 1 0 0 A2 A1 A0 0 A
Command
Command Type Bits: C2=0 C1=1 C0=0 W1=1 W0=0
Sequential Write
Starting Channel
VREF PD1 PD0 Gx D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
ACK (MCP4728)
DAC Input Register of Channel D
3rd Byte 4th Byte
Repeat Bytes of the 3rd - 4th Bytes
Note 1
Notes 1 and 2
Note 1: VOUT Update:
If UDAC = 0 or LDAC Pin = 0: VOUT is updated after the 4th byte’s ACK is issued.
2: EEPROM Write:
The MCP4728 device starts writing EEPROM at the falling edge of the 4th byte’s ACK pulse.
Stop
 Select
for the Starting Channel + 1, ... until Channel D.
(Last Channel)
© 2010 Microchip Technology Inc. DS22187E-page 41
MCP4728

FIGURE 5-10: Single Write Command: Write to a Single DAC Input Register and EEPROM.
C2 C1 C0 W1 W0 3rd Byte 4th Byte 2nd Byte
0 1 0 1 1 DAC1 DAC0 UDAC A VREF PD1 PD0 Gx D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
ACK (MCP4728)
1st byte
DAC Input Register of Selected Channel
R/W
Device Addressing
ACK (MCP4728)
Start
Single Write
S 1 1 0 0 A2 A1 A0 0 A
Command
Command Type Bits: C2=0 C1=1 C0=0 W1=1 W0=1
Channel
Select
Note 1 and Note 2
Note 1: VOUT Update:
If UDAC = 0 or LDAC Pin = 0: VOUT is updated after the 4th byte’s ACK is issued.
2: EEPROM Write:
The MCP4728 device starts writing EEPROM at the falling edge of the 4th byte’s ACK pulse.
Stop
MCP4728
DS22187E-page 42 © 2010 Microchip Technology Inc.

FIGURE 5-11: Write Command: Write I2C Address Bits to the DAC Registers and EEPROM.
Command Type Bits: C2=0 C1=1 C0=1
1st Byte 2nd Byte
Command New Address Bits
(for confirmation)
Note 3 (Notes 1, 2, 3)
LDAC Pin
Start 3rd Byte 4th Byte Stop
S1100 A2 A1 A0 0 A011 A2 A1 A0 0 1 A011 A2 A1 A0 1 0 A0 1 1 A2 A1 A0 1 1 A P
Clock and LDAC Transition Details:
Note 1: Clock Pulse and LDAC Transition Details.
2: LDAC pin events at the 2nd and 3rd bytes:
a. Keep LDAC pin “High” until the end of the positive pulse of the 8th clock of the 2nd byte.
b. LDAC pin makes a transition from “High” to “Low” during the negative pulse of the 8th clock of the 2nd byte
(just before the rising edge of the 9th clock), and stays “Low” until the rising edge of the 9th clock of the 3rd
byte.
c. The MCP4728 device does not acknowledge the 3rd byte if the conditions (a) and (b) are not met.
3: LDAC pin resumes its normal function after “Stop” bit.
4: EEPROM Write:
a. Charge Pump initiates the EEPROM write sequence at the falling edge of the 4th byte’s ACK pulse.
b. The RDY/BSY bit (pin) goes “Low” at the falling edge of this ACK clock and back to “High” immediately after
the EEPROM write is completed.
Type
Device R/W Current
Type Address Bits
Current Command
Code Address Bits
New
Type Address Bits
Command
Note 4
 Note 3
Clock Pulse
LDAC Pin
2nd Byte 3rd Byte 4th Byte
Note 2 (a)
ACK (MCP4728)
Note 2(b)
Stay “Low” during this 3rd byte
Note 2(b)
 (CLK Line)
5 6 7 8 9 1 2 3 4 5 6 7 8 9 1 ----- 9 P
Stop
Note 4
(C2 C1 C0)
Note: The I2C address bits can also be programmed at the factory for customers. See the Product Identification System
on page 65 for details.
© 2010 Microchip Technology Inc. DS22187E-page 43
MCP4728
FIGURE 5-12: Write Command: Write Voltage Reference Selection Bit (VREF) to the DAC Input
Registers.

FIGURE 5-13: Write Command: Write Power-Down Selection Bits (PD1, PD0) to the DAC Input
Registers. See Table 4-7 for the power-down bit setting.
Note 1: VREF = 0: VDD
 = 1: Internal Reference (2.048V)
VREF A = Voltage reference of Channel A
VREF B = Voltage reference of Channel B
VREF C = Voltage reference of Channel C
VREF D = Voltage reference of Channel D
2: X is don’t care bit.
Command Type Bits: C2=1 C1=0 C0=0
1st byte
Note 1
2nd Byte
R/W
Device Addressing
ACK (MCP4728)
Start (C2 C1 C0)
Write
S 1 1 0 0 A2 A1 A0 0 A 1 0 0 XVREF AVREF BVREF CVREF D A P
Command Registers and VOUT are updated
at this falling edge of ACK pulse.
Stop
Note 1: X is don’t care bit.
(C2 C1 C0) 3rd Byte 2nd Byte
1 0 1 X PD1 A PD0 A PD1 B PD0 B A PD1 C PD0 C PD1 D PD0 D X X X X A P
ACK (MCP4728)
1st byte
Channel C
R/W
Device Addressing
ACK (MCP4728)
Start
Write
S 1 1 0 0 A2 A1 A0 0 A
Command
Command Type Bits: C2=1 C1=0 C0=1
Stop
Registers and VOUT are updated
at this falling edge of ACK pulse.
Channel A Channel B Channel D
for Power-Down
 Selection Bits
MCP4728
DS22187E-page 44 © 2010 Microchip Technology Inc.
FIGURE 5-14: Write Command: Write Gain Selection Bit (GX) to the DAC Input Registers.
Start 1st Byte (C2 C1 C0) 2nd Byte Stop
S 1 1 0 0 A2 A1 A0 0 A 1 1 0 XGX AGX BGX CGX D A P
Note 1: GX A = Gain Selection for Channel A
GX B = Gain Selection for Channel B
GX C = Gain Selection for Channel C
GX D = Gain Selection for Channel D
Ex: GX A = 0: Gain of 1 for Channel A
 = 1: Gain of 2 for Channel A
2: X is don’t care bit.
ACK (MCP4728)
R/W
Device Addressing Write Command
Command Type Bits: C2=1 C1=1 C0=0
Registers and VOUT are updated
at this falling edge of ACK pulse.
Note 1
for Gain Selection Bits
© 2010 Microchip Technology Inc. DS22187E-page 45
MCP4728

FIGURE 5-15: Read Command and Device Outputs.
Note 1: The 2nd - 4th bytes are the contents of the DAC Input Register and the 5th - 7th bytes are the EEPROM contents.
The device outputs sequentially from channel A to D.
POR Bit: 1 = Set (Device is powered on with VDD > VPOR), 0 = Powered off state.
R/W
Device Code
ACK (MCP4728)
Start
S 1 1 0 0 A2 A1 A0 1 A
Read Command
Address Bits
2nd Byte
Channel A DAC Input Register
3rd Byte 4th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
ACK (MASTER)
5th Byte
Channel A DAC EEPROM
6th Byte 7th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
2nd Byte
Channel B DAC Input Register
3rd Byte 4th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
5th Byte
Channel B DAC EEPROM
6th Byte 7th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
2nd Byte
Channel C DAC Input Register
3rd Byte 4th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
5th Byte
Channel C DAC EEPROM
6th Byte 7th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
2nd Byte
Channel D DAC Input Register
3rd Byte 4th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
5th Byte
Channel D DAC EEPROM
6th Byte 7th Byte Stop
RDY/
BSY POR DAC1 DAC 0 0 A2 A1 A0 A VREF PD1 PD0 GX D11 D10 D9 D8 A D7 D6 D5 D4 D3 D2 D1 D0 A P
Repeat
MCP4728
DS22187E-page 46 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 47
MCP4728
6.0 TERMINOLOGY
6.1 Resolution
The resolution is the number of DAC output states that
divide the full scale range. For the 12-bit DAC, the
resolution is 212, meaning the DAC code ranges from 0
to 4095.
6.2 Least Significant Bit (LSB)
The least significant bit is the ideal voltage difference
between two successive codes.
EQUATION 6-1:
6.3 Integral Nonlinearity (INL)
Integral nonlinearity (INL) error is the maximum
deviation of an actual transfer function from an ideal
transfer function (straight line). In the MCP4728, INL is
calculated using two end-points (zero and full scale).
INL can be expressed as a percentage of full scale
range (FSR) or in fractions of an LSB. INL is also called
relative accuracy. Equation 6-2 shows how to calculate
the INL error in LSB and Figure 6-1 shows an example
of INL accuracy.
EQUATION 6-2: INL ERROR

FIGURE 6-1: INL Accuracy.
6.4 Differential Nonlinearity (DNL)
Differential nonlinearity (DNL) error (see Figure 6-2) is
the measure of step size between codes in actual
transfer function. The ideal step size between codes is
1 LSB. A DNL error of zero would imply that every code
is exactly 1 LSB wide. If the DNL error is less than
1 LSB, the DAC guarantees monotonic output and no
missing codes. The DNL error between any two
adjacent codes is calculated as follows:
EQUATION 6-3: DNL ERROR
LSB
VREF
2
n = ------------
Where:
VREF = VDD If external reference is
selected
= 2.048V If internal reference is
selected
n = The number of digital input bits,
n = 12 for MCP4728
VFull Scale VZero Scale ( ) –
4095 = ---------------------------------------------------------
VFull Scale VZero Scale ( ) –
2
12 – 1
= ---------------------------------------------------------
INL
VOUT VIdeal ( ) –
LSB = ---------------------------------------
Where:
INL is expressed in LSB
VIdeal = Code*LSB
VOUT = The output voltage measured at
the given input code
000 001 010
Analog
Output
(LSB)
DAC Input Code
011 111 100 101
1
2
3
4
5
6
0
7
110
Ideal Transfer Function
Actual Transfer Function
INL = < -1 LSB
INL = 0.5 LSB
INL = - 1 LSB
DNL
ΔVOUT – LSB
LSB = ----------------------------------
Where:
DNL is expressed in LSB.
ΔVOUT = The measured DAC output
voltage difference between two
adjacent input codes
MCP4728
DS22187E-page 48 © 2010 Microchip Technology Inc.
FIGURE 6-2: DNL Accuracy.
6.5 Offset Error
Offset error (see Figure 6-3) is the deviation from zero
voltage output when the digital input code is zero (zero
scale voltage). This error affects all codes by the same
amount. For the MCP4728 device, the offset error is
not trimmed at the factory. However, it can be calibrated
by software in application circuits.
FIGURE 6-3: Offset Error.
6.6 Gain Error
Gain error (see Figure 6-4) is the difference between
the actual full scale output voltage from the ideal output
voltage of the DAC transfer curve. The gain error is
calculated after nullifying the offset error, or full scale
error minus the offset error.
The gain error indicates how well the slope of the actual
transfer function matches the slope of the ideal transfer
function. The gain error is usually expressed as percent
of full scale range (% of FSR) or in LSB.
For the MCP4728 device, the gain error is not
calibrated at the factory and most of the gain error is
contributed by the output buffer (op amp) saturation
near the code range beyond 4000. For applications that
need the gain error specification less than 1%
maximum, a user may consider using the DAC code
range between 100 and 4000 instead of using full code
range (code 0 to 4095). The DAC output of the code
range between 100 and 4000 is much more linear than
full scale range (0 to 4095). The gain error can be
calibrated out by using applications’ software.
6.7 Full Scale Error (FSE)
Full scale error (see Figure 6-4) is the sum of offset
error plus gain error. It is the difference between the
ideal and measured DAC output voltage with all bits set
to one (DAC input code = FFFh).
EQUATION 6-4:
FIGURE 6-4: Gain Error and Full Scale
Error.
6.8 Gain Error Drift
Gain error drift is the variation in gain error due to a
change in ambient temperature. The gain error drift is
typically expressed in ppm/°C.
000 001 010
Analog
Output
(LSB)
DAC Input Code
011 100 101 111
1
2
3
4
5
6
0
7
DNL = 2 LSB
DNL = 0.5 LSB
110
Ideal Transfer Function
Actual Transfer Function
Analog
Output
Ideal Transfer Function
Actual Transfer Function
DAC Input Code 0
Offset
Error
FSE
VOUT VIdeal ( ) –
LSB = ---------------------------------------
Where:
FSE is expressed in LSB.
VIdeal = (VREF) (1 - 2-n) - Offset Voltage (VOS)
VREF = Voltage Reference
Analog
Output
Actual Transfer Function
Actual Transfer Function
DAC Input Code 0
Gain Error
Ideal Transfer Function
after Offset Error is removed
Full Scale
 Error 
© 2010 Microchip Technology Inc. DS22187E-page 49
MCP4728
6.9 Offset Error Drift
Offset error drift is the variation in offset error due to a
change in ambient temperature. The offset error drift is
typically expressed in ppm/oC.
6.10 Settling Time
The Settling time is the time delay required for the DAC
output to settle to its new output value from the start of
code transition, within specified accuracy. In the
MCP4728 device, the settling time is a measure of the
time delay until the DAC output reaches its final value
within 0.5 LSB when the DAC code changes from 400h
to C00h.
6.11 Major-Code Transition Glitch
Major-code transition glitch is the impulse energy
injected into the DAC analog output when the code in
the DAC register changes state. It is normally specified
as the area of the glitch in nV-Sec. and is measured
when the digital code is changed by 1 LSB at the major
carry transition (Example: 011...111 to 100...
000, or 100... 000 to 011... 111).
6.12 Digital Feedthrough
Digital feedthrough is a glitch that appears at the
analog output caused by coupling from the digital input
pins of the device. The area of the glitch is expressed
in nV-Sec, and is measured with a full scale change
(Example: all 0s to all 1s and vice versa) on the digital
input pins. The digital feedthrough is measured when
the DAC is not being written to the output register. This
condition can be created by writing the input register
with both the UDAC bit and the LDAC pin high.
6.13 Analog Crosstalk
Analog crosstalk is a glitch that appears at the output of
one DAC due to a change in the output of the other
DAC. The area of the glitch is expressed in nV-Sec,
and measured by loading one of the input registers with
a full scale code change (all 0s to all 1s and vice versa)
while keeping both the UDAC bit and the LDAC pin
high. Then bring down the LDAC pin to low and measure the output of the DAC whose digital code was not
changed.
6.14 DAC-to-DAC Crosstalk
DAC-to-DAC crosstalk is the glitch that appears at the
output of one DAC due to an input code change and
subsequent output change of the other DAC. This
includes both digital and analog crosstalks. The area of
the glitch is expressed in nV-Sec, and measured by
loading one of the input registers with a full scale code
change (all 0s to all 1s and vice versa) while keeping
UDAC bit or LDAC pin low.
6.15 Power-Supply Rejection Ratio
(PSRR)
PSRR indicates how the output of the DAC is affected
by changes in the supply voltage. PSRR is the ratio of
the change in VOUT to a change in VDD for full scale
output of the DAC. It is measured on one DAC that is
using an internal VREF while the VDD is varied ±10%,
and expressed in dB or µV/V.
MCP4728
DS22187E-page 50 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 51
MCP4728
7.0 TYPICAL APPLICATIONS
The MCP4728 device is a part of Microchip’s latest
DAC family with nonvolatile EEPROM memory. The
device is a general purpose resistor string DAC
intended to be used in applications where a precise
and low power DAC, with moderate bandwidth, is
required.
Since the device includes nonvolatile EEPROM
memory, the user can use this device for applications
that require the output to return to the previous set-up
value on subsequent power-ups.
Applications generally suited for the MCP4728 device
family include:
• Set Point or Offset Trimming
• Sensor Calibration
• Portable Instrumentation (Battery Powered)
• Motor Speed Control
7.1 Connecting to I2C BUS Using
Pull-Up Resistors
The SCL, SDA, and RDY/BSY pins of the MCP4728
device are open-drain configurations. These pins
require a pull-up resistor, as shown in Figure 7-1. The
LDAC pin has a Schmitt trigger input configuration and
it can be driven by an external MCU I/O pin.
The pull-up resistor values (R1 and R2) for SCL and
SDA pins depend on the operating speed (standard,
fast, and high speed) and loading capacitance of the
I
2C bus line. Higher value of pull-up resistor consumes
less power, but increases the signal transition time
(higher RC time constant) on the bus line. Therefore, it
can limit the bus operating speed. A lower resistor
value, on the other hand, consumes higher power, but
allows for higher operating speed. If the bus line has
higher capacitance due to long metal traces or multiple
device connections to the bus line, a smaller pull-up
resistor is needed to compensate for the long RC time
constant. The pull-up resistor is typically chosen
between 1 kΩ and 10 kΩ range for standard and fast
modes, and less than 1 kΩ for high speed mode.
FIGURE 7-1: Example of the MCP4728 Device Connection.
Analog Outputs
VDD
1
2
3
4
10
6
7
8
VDD
SCL
SDA
VSS
VOUT D
VOUT C
LDAC
RDY/BSY
9
5
VOUT B
VOUT A
MCP4728
R1
R2
R3
To MCU
R1 and R2 = Pull-up resistors for I2C Serial Communications
=
=
5 kΩ - 10 kΩ for fSCL = 100 kHz to 400 kHz
~700Ω for fSCL = 3.4 MHz
R3 = (a) Pull-up resistor to monitor RDY/BSY bit = ~ 100 kΩ
(b) Let this pin float when not used
 C1 = 0.1 µF, Ceramic capacitor
 C2 = 10 µF, Tantalum capacitor
C1 C2
MCP4728
DS22187E-page 52 © 2010 Microchip Technology Inc.
7.1.1 DEVICE CONNECTION TEST
The user can test the presence of the MCP4728 device
on the I2C bus line without performing a data
conversion. This test can be achieved by checking an
acknowledge response from the MCP4728 device after
sending a read or write command. Figure 7-2 shows an
example with a read command:
a. Set the R/W bit “High” or “Low” in the address
byte.
b. Check the ACK pulse after sending the address
byte.
If the device acknowledges (ACK = 0) the
command, then the device is connected,
otherwise it is not connected.
c. Send Stop Bit.
FIGURE 7-2: I
2C Bus Connection Test.
7.2 Layout Considerations
Inductively-coupled AC transients and digital switching
noise from other devices can affect DAC performance
and DAC output signal integrity. Careful board layout
will minimize these effects. Bench testing has shown
that a multi-layer board utilizing a low-inductance
ground plane, isolated inputs, isolated outputs and
proper decoupling are critical to achieving good DAC
performance.
Separate digital and analog ground planes are
recommended. In this case, the VSS pin and the ground
pins of the VDD capacitors of the MCP4728 should be
terminated to the analog ground plane.
7.3 Power Supply Considerations
The power source should be as clean as possible. The
power supply to the device is used for both VDD and
DAC voltage reference by selecting VREF = VDD. Any
noise induced on the VDD line can affect DAC
performance. A typical application will require a bypass
capacitor in order to filter out high-frequency noise on
the VDD line. The noise can be induced onto the power
supply’s traces or as a result of changes on the DAC
output. The bypass capacitor helps to minimize the
effect of these noise sources on signal integrity.
Figure 7-1 shows an example of using two bypass
capacitors (a 10 µF tantalum capacitor and a 0.1 µF
ceramic capacitor) in parallel on the VDD line. These
capacitors should be placed as close to the VDD pin as
possible (within 4 mm). If the application circuit has
separate digital and analog power supplies, the VDD
and VSS pins of the MCP4728 device should reside on
the analog plane.
7.4 Using Power Saving Feature
The device consumes very little power when it is in
Power-Down (shut-down) mode. During the
Power-Down mode, most circuits in the selected
channel are turned off. It is recommended to power
down any unused channel.
The device consumes the least amount of power if it
enters the Power-Down mode after the internal voltage
reference is disabled. This can be achieved by
selecting VDD as the voltage reference for all 4
channels, and then issuing the Power-Down mode for
all channels.
7.5 Using Nonvolatile EEPROM
Memory
The user can store the I2C device address bits,
configuration bits and DAC input code of each channel
in the on-board nonvolatile EEPROM memory using
the I2C write command. The contents of EEPROM are
readable and writable using the I2C command.
When the MCP4728 device is first powered-up or
receives General Call Reset Command, it uploads the
EEPROM contents to the DAC output registers
automatically and provides analog outputs immediately
with the saved settings in EEPROM. This feature is
very useful in applications where the MCP4728 device
is used to provide set points or calibration data for other
devices in the application systems. The MCP4728
device can save important system parameters when
the application system experiences power failure. See
Section 5.5 “Writing and Reading Registers and
EEPROM” for more details on using the nonvolatile
EEPROM memory.
SCL 123456789
SDA 1 1 0 1 A2 A1 A0 1
Start
Bit
Address Byte
Device Code Address bits
R/W
Stop
Bit
MCP4728 ACK
Response 
© 2010 Microchip Technology Inc. DS22187E-page 53
MCP4728
7.6 Application Examples
The MCP4728 device is a rail-to-rail output DAC
designed to operate with a VDD range of 2.7V to 5.5V.
Its output amplifier of each channel is robust enough to
drive common, small-signal loads directly, thus
eliminating the cost and size of external buffers for
most applications. Since each channel has its own
configuration bits for selecting the voltage reference,
gain, power-down, etc., the MCP4728 device offers
great simplicity and flexibility to use for various DAC
applications.
7.6.1 DC SET POINT OR CALIBRATION
VOLTAGE SETTINGS
A common application for the MCP4728 device is a
digitally-controlled set point or a calibration of variable
parameters such as sensor offset or bias point.
Figure 7-3 shows an example of the set point settings.
Let us consider that the application requires different
trip voltages (Trip 1 - Trip 4). Assuming the DAC output
voltage requirements are given as shown in Table 7-1,
examples of sending the Sequential Write and Fast
Write commands are shown in Figure 7-4 and
Figure 7-5.
TABLE 7-1: EXAMPLE: SETTING VOUT OF
EACH CHANNEL
DAC Channel Voltage
Reference
DAC Output
(VOUT)
VOUT A VDD VDD/2
VOUT B VDD VDD - 1 LSB
VOUT C Internal 2.048V
VOUT D Internal 4.096V
MCP4728
DS22187E-page 54 © 2010 Microchip Technology Inc.
FIGURE 7-3: Using the MCP4728 for Set Point or Threshold Calibration.
0.1 µF 10 µF
Analog Outputs
VDD
1
2
3
4
10
6
7
8
VDD
SCL
SDA
VSS
VOUT D
VOUT C
LDAC
RDY/BSY
9
5
VOUT B
VOUT A
MCP4728
R1
R2
R3
R4
To MCU
VDD
Comparator 1
R1
R2 0.1 µF
VTRIP1
RSENSE
Light
VDD
Comparator 2
R1
R2 0.1 µF
VTRIP2
RSENSE
Light
VDD
Comparator 3
R1
R2 0.1 µF
VTRIP3
RSENSE
Light
VDD
Comparator 4
R1
R2 0.1 µF
VTRIP4
RSENSE
Light
Dn = Input Code (0 to 4095)
VOUT VREF
Dn
4096 -----------Gx = ×
VTRIP VOUT
R2
R1 R + 2
------------------- ⎝ ⎠ ⎛ ⎞ =
© 2010 Microchip Technology Inc. DS22187E-page 55
MCP4728

FIGURE 7-4: Sequential Write Command for Setting Test Points in Figure 7-3.
Expected Output Voltage at Each Channel:
VOUT A VDD
Dn
4096 ----------- VDD
2048
4096 ----------- VDD
2 = × = × = ---------- ( ) V
VOUT B VDD
Dn
4096 ----------- VDD
4095
4096 ----------- VDD = × = × = ( ) – LSB ( ) V
VOUT C VREF
Dn
4096 ----------- Gx 2.048 2048
4096 = × = × ----------- × 2 2.048 V = ( )
VOUT D VREF
Dn
4096 ----------- Gx 2.048 4095
4096 = × = × ----------- × 2 4.096 V = ( )
S11000000 A01010000 A00001000 A00000000 A
1st Byte
Channel A as
Sequential Write Selecting
Device Addressing Command
Dn = 211 = 2048
UDAC VREF
ACK (MCP4728)
R/W GX
Starting Channel Update DAC A Input Register at this ACK pulse.
00001111 A11111111 A
ACK (MCP4728)
VREF GX
Dn = 4095
10011000 A00000000 A
ACK (MCP4728)
VREF GX
Dn = 2048
Update DAC B Input Register at this ACK pulse.
Update DAC C Input Register at this ACK pulse.
10011111 A11111111 A P
ACK (MCP4728)
VREF GX
Dn = 4095
Update DAC D Input Register at this ACK pulse.
Stop
Start
for Writing
MCP4728
DS22187E-page 56 © 2010 Microchip Technology Inc.
FIGURE 7-5: Example of Writing Fast Write Command for Various VOUT. VREF = VDD For All Channels.
1st Byte 2nd Byte
DAC A
Start 3rd Byte Stop
S 1 1 0 0 A2 A1 A0 0 A 0 1 1 A2 A1 A0 0 1 A 0 1 1 A2 A1 A0 1 0 A . . . . . . . P
Address Byte
Write Command
Fast Mode
Next DAC Channels
VOUT
VREF Dn ( ) ×
4096 ----------------------------------Gx =
VOUT A
VDD ( ) × 4095
4096 ----------------------------------- VDD
4096 1 –
4096 -------------------- ⎝ ⎠ ⎛ ⎞ VDD 1 1
4096 – ----------- ⎝ ⎠ ⎛ ⎞ VDD = = = = – LSB
Dn for Channel A = 0FFF (hex) = 4095 (decimal)
(A) Channel A Output:
(B) Channel B Output:
Dn for Channel B = 07FF (hex) = 2047 (decimal)
VOUT B
VDD ( ) × 2047
4096 ----------------------------------- VDD
2048 1 –
4096 -------------------- ⎝ ⎠ ⎛ ⎞ VDD
2 ------------ 1 2
4096 – ----------- ⎝ ⎠ ⎛ ⎞ VDD
2 = = = = ------------ LSB –
(C) Channel C Output:
Dn for Channel C = 03FF (hex) = 1023 (decimal)
VOUT C
VDD × 1023
4096 ---------------------------------- VDD
1024 1 –
4096 -------------------- ⎝ ⎠ ⎛ ⎞ VDD
4 ------------ 1 4
4096 – ----------- ⎝ ⎠ ⎛ ⎞ VDD
4 = = = = ------------ LSB –
(D) Channel D Output:
Dn for Channel D = 01FF (hex) = 511 (decimal)
VOUT D
VDD × 511
4096 ------------------------------- VDD
512 1 –
4096 ----------------- ⎝ ⎠ ⎛ ⎞ VDD
8 ------------ 1 8
4096 – ----------- ⎝ ⎠ ⎛ ⎞ VDD
8 = = = = ------------ LSB –
DAC A Input Code = 001111-11111111
DAC B Input Code = 000111-11111111
DAC C Input Code = 000011-11111111
DAC D Input Code = 000001-11111111
The following example shows the expected analog outputs with the corresponding DAC input codes:
© 2010 Microchip Technology Inc. DS22187E-page 57
MCP4728
8.0 DEVELOPMENT SUPPORT
8.1 Evaluation & Demonstration
Boards
The MCP4728 Evaluation Board is available from
Microchip Technology Inc. This board works with
Microchip’s PICkit™ Serial Analyzer. The user can
easily program the DAC input registers and EEPROM
using the PICkit Serial Analyzer, and test out the DAC
analog output voltages.The PICkit Serial Analyzer uses
the PC Graphic User Interface software. Refer to
www.microchip.com for further information on this
product’s capabilities and availability.
FIGURE 8-1: MCP4728 Evaluation
Board.
FIGURE 8-2: Setup for the MCP4728
Evaluation Board with PICkit™ Serial Analyzer.
FIGURE 8-3: Example of PICkit™ Serial User Interface.
MCP4728
DS22187E-page 58 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 59
MCP4728
9.0 PACKAGING INFORMATION
9.1 Package Marking Information
Legend: XX...X Customer-specific information
Y Year code (last digit of calendar year)
YY Year code (last 2 digits of calendar year)
WW Week code (week of January 1 is week ‘01’)
NNN Alphanumeric traceability code
 Pb-free JEDEC designator for Matte Tin (Sn)
* This package is Pb-free. The Pb-free JEDEC designator ( )
can be found on the outer packaging for this package.
Note: In the event the full Microchip part number cannot be marked on one line, it will
be carried over to the next line, thus limiting the number of available
characters for customer-specific information.
e3
e3
10-Lead MSOP
XXXXXX
YWWNNN
Example
 4728UN
007256
Device Code
MCP4728-E/UN 4728UN
MCP4728T-E/UN 4728UN
MCP4728A0-E/UN 4728A0
MCP4728A0T-E/UN 4728A0
MCP4728A1-E/UN 4728A1
MCP4728A1T-E/UN 4728A1
MCP4728A2-E/UN 4728A2
MCP4728A2T-E/UN 4728A2
MCP4728A3-E/UN 4728A3
MCP4728A3T-E/UN 4728A3
MCP4728A4-E/UN 4728A4
MCP4728A4T-E/UN 4728A4
MCP4728A5-E/UN 4728A5
MCP4728A5T-E/UN 4728A5
MCP4728A6-E/UN 4728A6
MCP4728A6T-E/UN 4728A6
MCP4728A7-E/UN 4728A7
MCP4728A7T-E/UN 4728A7
MCP4728
DS22187E-page 60 © 2010 Microchip Technology Inc.
	
				
		


  !"#$%!&'(!%&! %(%")%%%"
 &  "*"%!"&"$ 
%!  "$ 
%!   %#"+&&
 "
, & "%
*-+
./0 . & %#%! ))%!%% 
*10 $& '! !)%!%%'$$&%
!
  
 1%& %!%
2") '
  %
2
$%%"%
%%
033)))&
&3
2
4% 55**
& 5&% 6 67 8
6!&($ 6 
%  +./
79%  : : 
""22  + ;+ +
%"$$   : +
7<"% * ./
""2<"% * ,./
75%  ,./
1%5% 5  = ;
1%
% 5 +*1
1%  > : ;>
5"2  ; : ,
5"<"% ( + : ,,
D
E
E1
N
NOTE 1
1 2
b e
A
A1
A2 c
L
L1
φ

  ) /	.
© 2010 Microchip Technology Inc. DS22187E-page 61
MCP4728
10-Lead Plastic Micro Small Outline Package (UN) [MSOP]
Note: For the most current package drawings, please see the Microchip Packaging Specification located at
http://www.microchip.com/packaging
MCP4728
DS22187E-page 62 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 63
MCP4728
APPENDIX A: REVISION HISTORY
Revision E (October 2010)
The following is the list of modifications:
1. Corrected values in I
2C Serial Timing
Specifications table (SCL Fall Time, SDA Fall
Time, Data Hold Time, Output Valid from Clock).
2. Updated the Package Marking Information table
in the “Packaging Information” section.
3. Updated the information in the section
“Product Identification System”.
Revision D (October 2009)
The following is the list of modifications:
1. Front page - Applications: Added new item:
Bias Voltage Adjustment for Power Amplifiers.
2. Electrical Characteristics: Changed typical,
max values for Offset Error.
3. Electrical Characteristics: Changed Min, Max
values for Gain Error.
4. Section 2.0 Typical Performance Curves:
Added new Figure 2-25: Absolute Gain Error.
5. Page 45, Figure 5-15: Changed ACK
(MCP4728) to ACK (MASTER).
Revision C (September 2009)
The following is the list of modifications:
6. Updated Figure 5-11 and Figure 7-4.
Revision B (August 2009)
The following is the list of modifications:
7. Updated Figure 2-25 to Figure 2-41 in
Section 2.0 “Typical Performance Curves”.
8. Updated Figure 5-7, Figure 5-8 and Figure 5-11.
Revision A (June 2009)
• Original Release of this Document.
MCP4728
DS22187E-page 64 © 2010 Microchip Technology Inc.
NOTES:
© 2010 Microchip Technology Inc. DS22187E-page 65
MCP4728
PRODUCT IDENTIFICATION SYSTEM
To order or obtain information, e.g., on pricing or delivery, refer to the factory or the listed sales office.

Device: MCP4728: 12-bit, Quad Digital-to-Analog Convertor with
EEPROM memory
Address Options: XX A2 A1 A0
A0 * = 0 0 0
A1 = 0 0 1
A2 = 0 1 0
A3 = 0 1 1
A4 = 1 0 0
A5 = 1 0 1
A6 = 1 1 0
A7 = 1 1 1
* Default option. Contact Microchip factory for other address
options
Note: These address bits are reprogrammable by the
user.
Tape and Reel: T = Tape and Reel
Temperature Range: E = -40°C to +125°C
Package: UN = Plastic Micro Small Outline Transistor, 10-lead
PART NO. -X /XX
Temperature Package
Range
Device
Examples:
