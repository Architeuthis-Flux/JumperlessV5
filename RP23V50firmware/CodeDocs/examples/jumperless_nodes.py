# Jumperless Node Constants - Import this to get node names in global scope
# Usage: from jumperless_nodes import *

import jumperless

# Make all node constants available globally
TOP_RAIL = jumperless.TOP_RAIL
BOTTOM_RAIL = jumperless.BOTTOM_RAIL  
BOT_RAIL = jumperless.BOTTOM_RAIL
T_RAIL = jumperless.TOP_RAIL
B_RAIL = jumperless.BOTTOM_RAIL  

GND = jumperless.GND

DAC0 = jumperless.DAC0
DAC_0 = jumperless.DAC0
DAC1 = jumperless.DAC1
DAC_1 = jumperless.DAC1

# Current Sense Pins

CURRENT_SENSE_P = jumperless.CURRENT_SENSE_P
CURRENT_SENSE_N = jumperless.CURRENT_SENSE_N

CURRENT_SENSE_PLUS = jumperless.CURRENT_SENSE_P
CURRENT_SENSE_MINUS = jumperless.CURRENT_SENSE_N

ISENSE_PLUS = jumperless.ISENSE_PLUS
ISENSE_P = jumperless.ISENSE_PLUS
I_P = jumperless.ISENSE_PLUS

ISENSE_MINUS = jumperless.ISENSE_MINUS
ISENSE_N = jumperless.ISENSE_N
I_N = jumperless.ISENSE_MINUS

BUFFER_IN = jumperless.BUFFER_IN
BUF_IN = jumperless.BUFFER_IN
BUFFER_OUT = jumperless.BUFFER_OUT
BUF_OUT = jumperless.BUFFER_OUT

#ADC Pins
ADC0 = jumperless.ADC0
ADC1 = jumperless.ADC1
ADC2 = jumperless.ADC2
ADC3 = jumperless.ADC3
ADC4 = jumperless.ADC4
ADC7 = jumperless.ADC7

# Arduino Digital Pins
D0 = jumperless.D0
D1 = jumperless.D1
D2 = jumperless.D2
D3 = jumperless.D3
D4 = jumperless.D4
D5 = jumperless.D5
D6 = jumperless.D6
D7 = jumperless.D7
D8 = jumperless.D8
D9 = jumperless.D9
D10 = jumperless.D10
D11 = jumperless.D11
D12 = jumperless.D12
D13 = jumperless.D13

NANO_D0 = jumperless.D0
NANO_D1 = jumperless.D1
NANO_D2 = jumperless.D2
NANO_D3 = jumperless.D3
NANO_D4 = jumperless.D4
NANO_D5 = jumperless.D5
NANO_D6 = jumperless.D6
NANO_D7 = jumperless.D7
NANO_D8 = jumperless.D8
NANO_D9 = jumperless.D9
NANO_D10 = jumperless.D10
NANO_D11 = jumperless.D11
NANO_D12 = jumperless.D12
NANO_D13 = jumperless.D13

# Arduino Analog Pins
A0 = jumperless.A0
A1 = jumperless.A1
A2 = jumperless.A2
A3 = jumperless.A3
A4 = jumperless.A4
A5 = jumperless.A5
A6 = jumperless.A6
A7 = jumperless.A7

NANO_A0 = jumperless.A0
NANO_A1 = jumperless.A1
NANO_A2 = jumperless.A2
NANO_A3 = jumperless.A3
NANO_A4 = jumperless.A4
NANO_A5 = jumperless.A5
NANO_A6 = jumperless.A6
NANO_A7 = jumperless.A7

# GPIO Pins
GPIO_1 = jumperless.GPIO1
GPIO_2 = jumperless.GPIO2
GPIO_3 = jumperless.GPIO3
GPIO_4 = jumperless.GPIO4
GPIO_5 = jumperless.GPIO5
GPIO_6 = jumperless.GPIO6
GPIO_7 = jumperless.GPIO7
GPIO_8 = jumperless.GPIO8

GP1 = jumperless.GPIO1
GP2 = jumperless.GPIO2
GP3 = jumperless.GPIO3
GP4 = jumperless.GPIO4
GP5 = jumperless.GPIO5
GP6 = jumperless.GPIO6
GP7 = jumperless.GPIO7
GP8 = jumperless.GPIO8

GPIO_20 = jumperless.GPIO1
GPIO_21 = jumperless.GPIO2
GPIO_22 = jumperless.GPIO3
GPIO_23 = jumperless.GPIO4
GPIO_24 = jumperless.GPIO5
GPIO_25 = jumperless.GPIO6
GPIO_26 = jumperless.GPIO7
GPIO_27 = jumperless.GPIO8

UART_TX = jumperless.UART_TX
TX = jumperless.UART_TX
UART_RX = jumperless.UART_RX
RX = jumperless.UART_RX



# Alternative: Create nodes from strings (works even without constants)
def node(name):
    """Helper function to create nodes from strings"""
    return jumperless.node(name)

# Example usage:
if __name__ == "__main__":
    print("Node constants loaded!")
    print(f"TOP_RAIL = {TOP_RAIL}")
    print(f"D13 = {D13}")
    print(f"GND = {GND}")
    
    # Test connections
    jumperless.connect(TOP_RAIL, D13)
    print("Connected TOP_RAIL to D13") 