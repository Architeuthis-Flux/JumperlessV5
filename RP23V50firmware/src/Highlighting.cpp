#include "Highlighting.h"
#include "Graphics.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "MatrixState.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "ArduinoStuff.h"
#include "Probing.h"
#include "RotaryEncoder.h"
#include "config.h"
#include "hardware/i2c.h"
#include "oled.h"
#include <Arduino.h>
#include <cmath>

// Global variables definitions
rgbColor highlightedOriginalColor;
rgbColor brightenedOriginalColor;
rgbColor warningOriginalColor;

int firstConnection = -1;

int showReadingRow = -1;
int showReadingNet = -1;
int highlightedRow = -1;
int lastNodeHighlighted = -1;
int lastNetPrinted = -1;
int lastPrintedNet = -1;

int currentHighlightedNode = 0;
int currentHighlightedNet = -2;

int warningRow = -1;
int warningNet = -1;
unsigned long warningTimeout = 0;
unsigned long warningTimer = 0;

// Additional highlighting variables that were in LEDs.cpp
int highlightedNet = -1;
int probeConnectHighlight = -1;
int brightenedNode = -1;
int brightenedNet = -1;
int brightenedRail = -1;
int brightenedAmount = 20;
int brightenedNodeAmount = 400;
int brightenedNetAmount = 150;

void clearHighlighting( void ) {

    // netColors[highlightedNet] = highlightedOriginalColor;
    // netColors[brightenedNet] = brightenedOriginalColor;
    // netColors[warningNet] = warningOriginalColor;
    // Serial.println("clearHighlighting");
    // Serial.flush();
    for ( int i = 4; i < numberOfRowAnimations; i++ ) {
        rowAnimations[ i ].row = -1;
        rowAnimations[ i ].net = -1;
    }
    probeConnectHighlight = -1;
    highlightedNet = -1;
    brightenedNet = -1;
    warningNet = -1;
    warningRow = -1;
    brightenedRail = -1;
    brightenedNode = -1;
    highlightedRow = -1;

    firstConnection = -1;

    // Reset highlight timer
    highlightTimer = 0;

    assignNetColors( );
}

int lastReturnNode = -1;
int scrolledRow = -1;

int encoderNetHighlight( int print, int mode, int divider ) {

    int lastDivider = rotaryDivider;
    rotaryDivider = divider;
    int returnNode = -1;

    // Serial.print (" highlightedNet: ");
    // Serial.println(highlightedNet);

    // Serial.print (" brightenedRail: ");
    // Serial.println(brightenedRail);

    // Serial.print (" brightenedNode: ");
    // Serial.println(brightenedNode);

    // Serial.flush();
    // if (inClickMenu == 1)
    //   return -1;
    // rotaryEncoderStuff();
    if ( mode == 0 ) {
        if ( encoderDirectionState == UP ) {
            // Serial.println(encoderPosition);
            encoderDirectionState = NONE;
            if ( highlightedNet < 0 ) {
                highlightedNet = -1;
                brightenedNet = -1;
                currentHighlightedNode = 0;
            }
            currentHighlightedNode++;
            if ( highlightedNet >= 0 && highlightedNet < numberOfNets && net[ highlightedNet ].nodes[ currentHighlightedNode ] <= 0 ) {
                currentHighlightedNode = 0;
                highlightedNet++;
                if ( highlightedNet > numberOfNets - 1 ) {
                    highlightedNet = -2;
                    brightenedNet = -2;
                    currentHighlightedNode = 0;
                }
                brightenedNet = highlightedNet;
                if ( highlightedNet >= 0 && highlightedNet < numberOfNets ) {
                    brightenedNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                    if ( highlightedNet != 0 && net[ highlightedNet ].nodes[ currentHighlightedNode ] != 0 ) {
                        returnNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                    }
                } else {
                    brightenedNode = -1;
                }
                highlightNets( 0, highlightedNet, print );
                // Serial.print("highlightedNet: ");
                // Serial.println(highlightedNet);
                // Serial.flush();
            }
            if ( highlightedNet > numberOfNets - 1 ) {
                highlightedNet = -2;
                brightenedNet = -2;
                currentHighlightedNode = 0;
            }

            brightenedNet = highlightedNet;

            if ( highlightedNet >= 0 && highlightedNet < numberOfNets ) {
                brightenedNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                if ( highlightedNet != 0 && net[ highlightedNet ].nodes[ currentHighlightedNode ] != 0 ) {
                    returnNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                }
            } else {

                brightenedNode = -1;
            }
            // Serial.print("returnNode: ");
            // Serial.println(returnNode);
            // Serial.flush();
            highlightNets( 0, highlightedNet, print );
            // Serial.print("highlightedNet: ");
            // Serial.println(highlightedNet);
            // Serial.flush();
            // assignNetColors();
            // assignNetColors();

        } else if ( encoderDirectionState == DOWN ) {
            // Serial.println(encoderPosition);
            encoderDirectionState = NONE;
            if ( highlightedNet == 0 ) {

                highlightedNet = numberOfNets - 1;
                brightenedNet = numberOfNets - 1;
            }

            currentHighlightedNode--;

            if ( currentHighlightedNode < 0 ) {
                highlightedNet--;
                if ( highlightedNet < 0 ) {
                    highlightedNet = numberOfNets - 1;
                    brightenedNet = numberOfNets - 1;
                    currentHighlightedNode = 0;
                }
                currentHighlightedNode = MAX_NODES - 1;
                while ( highlightedNet >= 0 && highlightedNet < numberOfNets && net[ highlightedNet ].nodes[ currentHighlightedNode ] <= 0 ) {
                    currentHighlightedNode--;
                    if ( currentHighlightedNode < 0 ) {
                        highlightedNet--;
                        if ( highlightedNet < 0 ) {
                            highlightedNet = numberOfNets - 1;
                            brightenedNet = numberOfNets - 1;
                            currentHighlightedNode = 0;
                        }
                    }
                }
            }
            brightenedNet = highlightedNet;
            if ( highlightedNet >= 0 && highlightedNet < numberOfNets ) {
                brightenedNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                if ( highlightedNet != 0 && net[ highlightedNet ].nodes[ currentHighlightedNode ] != 0 ) {
                    returnNode = net[ highlightedNet ].nodes[ currentHighlightedNode ];
                }
            } else {
                brightenedNode = -1;
            }
            // Serial.print("returnNode: ");
            // Serial.println(returnNode);
            // Serial.flush();
            highlightNets( 0, highlightedNet, print );
            // Serial.print("highlightedNet: ");
            // Serial.println(highlightedNet);
            // Serial.flush();
            // assignNetColors();
        }
        if ( returnNode != lastNodeHighlighted ) {
            // b.clear();
            // b.printRawRow(0b00000100, lastNodeHighlighted-2, 0x000000, 0x000000);
            // b.printRawRow(0b00000100, lastNodeHighlighted, 0x0000000, 0x000000);

            // b.printRawRow(0b00000100, returnNode-2, 0x0f0f00, 0x000000);
            // b.printRawRow(0b00000100, returnNode, 0x0f0f00, 0x000000);

            lastNodeHighlighted = returnNode;
            // showLEDsCore2 = 2;
        }
        // rotaryDivider = lastDivider;
        return returnNode;
    } else if ( mode == 1 ) {
        // Initialize scrolledRow if needed (start with breadboard row 1)
        // Valid scroll targets: 1..60 (breadboard), TOP_RAIL, BOTTOM_RAIL, NANO_D0..NANO_RESET_1
        if ( scrolledRow == -1 || !(
                 ( scrolledRow >= 1 && scrolledRow <= 60 ) ||
                 ( scrolledRow == GND ) || ( scrolledRow == TOP_RAIL ) || ( scrolledRow == BOTTOM_RAIL ) ||
                 ( scrolledRow >= NANO_D0 && scrolledRow <= NANO_RESET_1 ) ) ) {
            scrolledRow = 1;
        }
        // Serial.print("                      \rscrolledRow = ");
        // Serial.print(scrolledRow);

        // Helper function to increment scrolledRow across breadboard + rails + nano ranges
        auto incrementRow = []( int& row ) {
            if ( row >= 1 && row < 60 ) {
                row++;
            } else if ( row == 60 ) {
                row = GND; // jump from breadboard to GND
            } else if ( row == GND ) {
                row = TOP_RAIL; // then to rails
            } else if ( row == TOP_RAIL ) {
                row = BOTTOM_RAIL;
            } else if ( row == BOTTOM_RAIL ) {
                row = NANO_D0; // jump from rails to nano header
            } else if ( row >= NANO_D0 && row < NANO_RESET_1 ) {
                row++;
            } else if ( row == NANO_RESET_1 ) {
                row = 1; // wrap around to start
            }
        };

        // Helper function to decrement scrolledRow across breadboard + rails + nano ranges
        auto decrementRow = []( int& row ) {
            if ( row > 1 && row <= 60 ) {
                row--;
            } else if ( row == 1 ) {
                row = NANO_RESET_1; // wrap around to end
            } else if ( row > NANO_D0 && row <= NANO_RESET_1 ) {
                row--;
            } else if ( row == NANO_D0 ) {
                row = BOTTOM_RAIL; // jump from nano to rails
            } else if ( row == BOTTOM_RAIL ) {
                row = TOP_RAIL;
            } else if ( row == TOP_RAIL ) {
                row = GND;
            } else if ( row == GND ) {
                row = 60; // jump from GND to breadboard
            }
        };

        if ( encoderDirectionState == UP ) {
            encoderDirectionState = NONE;
            incrementRow( scrolledRow );

            // Find a row with connections starting from current scrolledRow
            int originalRow = scrolledRow;
            do {
                bool foundConnection = false;
                int connectedNet = -1;

                // Check if current scrolledRow has any connections
                for ( int i = 0; i < numberOfPaths; i++ ) {
                    if ( path[ i ].node1 == scrolledRow || path[ i ].node2 == scrolledRow ) {
                        foundConnection = true;
                        connectedNet = path[ i ].net;
                        break;
                    }
                }
                // Allow highlighting rails and GND even without explicit paths
                if ( !foundConnection ) {
                    if ( scrolledRow == GND ) {
                        foundConnection = true;
                        connectedNet = 1;
                    } else if ( scrolledRow == TOP_RAIL ) {
                        foundConnection = true;
                        connectedNet = 2;
                    } else if ( scrolledRow == BOTTOM_RAIL ) {
                        foundConnection = true;
                        connectedNet = 3;
                    }
                }

                if ( foundConnection ) {
                    highlightedNet = connectedNet;
                    brightenedNet = connectedNet;
                    brightenedNode = scrolledRow;
                    brightenedAmount = 80; // Set brightness for highlighting
                    highlightNets( 0, highlightedNet, print );
                    returnNode = scrolledRow;
                    break;
                } else {
                    incrementRow( scrolledRow );
                }
            } while ( scrolledRow != originalRow ); // prevent infinite loop

        } else if ( encoderDirectionState == DOWN ) {
            encoderDirectionState = NONE;
            decrementRow( scrolledRow );

            // Find a row with connections starting from current scrolledRow
            int originalRow = scrolledRow;
            do {
                bool foundConnection = false;
                int connectedNet = -1;

                // Check if current scrolledRow has any connections
                for ( int i = 0; i < numberOfPaths; i++ ) {
                    if ( path[ i ].node1 == scrolledRow || path[ i ].node2 == scrolledRow ) {
                        foundConnection = true;
                        connectedNet = path[ i ].net;
                        break;
                    }
                }
                // Allow highlighting rails and GND even without explicit paths
                if ( !foundConnection ) {
                    if ( scrolledRow == GND ) {
                        foundConnection = true;
                        connectedNet = 1;
                    } else if ( scrolledRow == TOP_RAIL ) {
                        foundConnection = true;
                        connectedNet = 2;
                    } else if ( scrolledRow == BOTTOM_RAIL ) {
                        foundConnection = true;
                        connectedNet = 3;
                    }
                }

                if ( foundConnection ) {
                    highlightedNet = connectedNet;
                    brightenedNet = connectedNet;
                    brightenedNode = scrolledRow;
                    brightenedAmount = 80; // Set brightness for highlighting
                    highlightNets( 0, highlightedNet, print );
                    returnNode = scrolledRow;
                    break;
                } else {
                    decrementRow( scrolledRow );
                }
            } while ( scrolledRow != originalRow ); // prevent infinite loop
        }

        // Ensure we always return a value for mode == 1
        // if (returnNode == -1) {
        //   returnNode = scrolledRow;
        //   Serial.print("returnNode: ");
        //   Serial.println(returnNode);
        //   Serial.flush();
        // }
        if ( returnNode != lastReturnNode ) {
            lastReturnNode = returnNode;
            // Serial.print("returnNode: ");
            // Serial.println(returnNode);
            // Serial.flush();
        }
        return returnNode;
    }

    // Final return for any other modes
    return returnNode;
}

int brightenNet( int node, int addBrightness ) {

    if ( node == -1 ) {
        netColors[ brightenedNet ] = brightenedOriginalColor;
        brightenedNode = -1;
        brightenedNet = 0;
        brightenedRail = -1;
        return -1;
    }
    addBrightness = 0;

    for ( int i = 0; i < numberOfPaths; i++ ) {

        if ( node == path[ i ].node1 || node == path[ i ].node2 && path[ i ].duplicate == 0 ) {
            /// if (brightenedNet != i) {
            brightenedNet = path[ i ].net;
            brightenedNode = node;
            // Serial.print("\n\n\rbrightenedNet: ");
            // Serial.println(brightenedNet);
            // Serial.print("net ");
            // Serial.print(path[i].net);
            if ( brightenedNet == 1 ) {
                brightenedRail = 1;
                // lightUpRail(-1, 1, 1, addBrightness);
            } else if ( brightenedNet == 2 ) {
                brightenedRail = 0;
                // lightUpRail(-1, 0, 1, addBrightness);
            } else if ( brightenedNet == 3 ) {
                brightenedRail = 2;
                // lightUpRail(-1, 2, 1, addBrightness);
            } else {
                brightenedRail = -1;
                // lightUpNet(brightenedNet, addBrightness);
            }
            // Serial.print("\n\rbrightenedNet = ");
            // Serial.println(brightenedNet);
            brightenedOriginalColor = netColors[ brightenedNet ];
            assignNetColors( );
            return brightenedNet;
        }
    }
    switch ( node ) {
    case ( GND ): {
        //  Serial.print("\n\rGND");
        brightenedNet = 1;
        brightenedRail = 1;
        // lightUpRail(-1, 1, 1, addBrightness);
        return 1;
    }
    case ( TOP_RAIL ): {
        // Serial.print("\n\rTOP_RAIL");
        brightenedNet = 2;
        brightenedRail = 0;
        // lightUpRail(-1, 0, 1, addBrightness);
        return 2;
    }
    case ( BOTTOM_RAIL ): {
        // Serial.print("\n\rBOTTOM_RAIL");
        brightenedNet = 3;
        brightenedRail = 2;
        // lightUpRail(-1, 2, 1, addBrightness);
        return 3;
    }
    }

    return -1;
}

/// @brief  mark a net as warning
/// @param -1 to clear warning
/// @return warningNet
int warnNet( int node ) {
    // Serial.print("warnNet node = ");
    // Serial.println(node);
    // Serial.flush();
    if ( node == -1 ) {
        netColors[ warningNet ] = warningOriginalColor;

        warningNet = -1;
        warningRow = -1;
        // Serial.print("warningNet = ");
        // Serial.println(warningNet);
        // Serial.flush();
        // brightenedRail = -1;
        return -1;
    }
    // addBrightness = 0;
    warningRow = bbPixelToNodesMap[ node ];

    for ( int i = 0; i < numberOfPaths; i++ ) {

        if ( node == path[ i ].node1 || node == path[ i ].node2 ) {
            /// if (brightenedNet != i) {
            warningNet = path[ i ].net;

            // Serial.print("warningNet = ");
            // Serial.println(warningNet);
            // Serial.flush();

            if ( warningNet == 1 ) {
                // brightenedRail = 1;
                // lightUpRail(-1, 1, 1, addBrightness);
            } else if ( warningNet == 2 ) {
                // brightenedRail = 0;
                // lightUpRail(-1, 0, 1, addBrightness);
            } else if ( warningNet == 3 ) {
                // brightenedRail = 2;
                // lightUpRail(-1, 2, 1, addBrightness);
            } else {
                // brightenedRail = -1;
                // lightUpNet(brightenedNet, addBrightness);
            }

            warningOriginalColor = netColors[ warningNet ];
            assignNetColors( );
            warningTimer = millis( );
            return warningNet;
        }
    }

    return -1;
}

unsigned long lastWarningTimer = 0;
unsigned long lastHighlightTimer = 0;
unsigned long highlightTimeout = 1800;    // 3 seconds timeout for highlighted nets
unsigned long dacHighlightTimeout = 6000; // 8 seconds timeout for DAC nets
unsigned long highlightTimer = 0;

unsigned long lastFirstConnectionTimer = 0;

void warnNetTimeout( int clearAll ) {
    // Serial.print("warningTimer = ");
    // Serial.println(warningTimer);
    // Serial.print("warningTimeout = ");
    // Serial.println(warningTimeout);
    // Serial.flush();
    if ( lastWarningTimer == 0 ) {
        lastWarningTimer = millis( );
    }

    if ( lastHighlightTimer == 0 ) {
        lastHighlightTimer = millis( );
    }

    // Check for warning timeout
    if ( warningTimer > 0 && millis( ) - warningTimer > warningTimeout ) {
        // warningTimeout = 0;
        // Serial.println("warningTimer timeout");
        if ( clearAll == 1 ) {
            clearHighlighting( );
        } else {
            // netColors[warningNet] = warningOriginalColor;

            warningNet = -1;
            warningRow = -1;
        }
        lastWarningTimer = millis( );
        warningTimer = 0;

        assignNetColors( );
    } else {
        lastWarningTimer = millis( ) - lastWarningTimer;
        // Serial.print("lastWarningTimer = ");
        // Serial.println(lastWarningTimer);
        // Serial.flush();
        // warningTimer = millis();
    }

    // Check for highlighted net timeout
    // Skip timeout if highlighting a GPIO output, use longer timeout for DACs
    bool skipTimeout = false;
    unsigned long currentTimeout = highlightTimeout;

    if ( highlightedNet == 4 || highlightedNet == 5 ) {
        // DAC nets use longer timeout (8 seconds)
        currentTimeout = dacHighlightTimeout;
    } else if ( highlightedNet > 0 && anyGpioOutputConnected( highlightedNet ) != -1 ) {
        // GPIO output nets should never timeout
        skipTimeout = true;
    }

    if ( !skipTimeout && highlightTimer > 0 && millis( ) - highlightTimer > currentTimeout ) {
        clearHighlighting( ); // Clear all highlighting when timeout expires
        highlightTimer = 0;
        lastHighlightTimer = millis( );
    }
}

int highlightNets( int probeReading, int encoderNetHighlighted, int print ) {
    // Serial.print("justReadProbe = ");
    // Serial.println(probeReading);
    // delay(100);

    //  Serial.println("probeReading: ");
    //  Serial.println(probeReading);
    //  Serial.flush();

    int netHighlighted;

    if ( encoderNetHighlighted > 0 ) {

        netHighlighted = encoderNetHighlighted;

    } else {

        netHighlighted = brightenNet( probeReading );
    }

    if ( netHighlighted > 0 ) {

        highlightedOriginalColor = netColors[ netHighlighted ];
        highlightedNet = netHighlighted;

        // Start the highlight timer
        highlightTimer = millis( );

        // Serial.print("netHighlighted = ");
        // Serial.println(netHighlighted);
        if ( print == 1 ) {
            Serial.print( "\r                                               \r" );
            Serial.flush( );
            oled.setTextSize( 1 );
        }
        clearColorOverrides( 1, 1, 0 );
        brightenedRail = -1;
        lastPrintedNet = -1;
        switch ( netHighlighted ) {
        case 0:
            break;
        case 1:
            if ( lastPrintedNet != netHighlighted ) {
                if ( print == 1 ) {
                    Serial.print( "GND" );
                    Serial.flush( );
                    char oledString[ 30 ];
                    sprintf( oledString, "GND" );

                    //oled.clear( );
                    oled.clearPrintShow( oledString, 2, true, true, true );
                    //oled.show( );
                }
                lastPrintedNet = netHighlighted;
            }
            brightenedRail = 1;
            break;
        case 2:
            if ( lastPrintedNet != netHighlighted ) {
                lastPrintedNet = netHighlighted;
                if ( print == 1 ) {
                    Serial.print( "Top Rail  " );

                    Serial.print( railVoltage[ 0 ] );
                    Serial.print( " V" );
                    Serial.flush( );

                    char oledString[ 30 ];
                    sprintf( oledString, "Top Rail\n%0.2f V", (float)railVoltage[ 0 ] );

                    //oled.clear( );
                    oled.clearPrintShow( oledString, 2, true, true, true );
                    //oled.show( );
                }
            }
            brightenedRail = 0;
            break;
        case 3:
            if ( lastPrintedNet != netHighlighted ) {
                lastPrintedNet = netHighlighted;
                if ( print == 1 ) {
                    Serial.print( "Bottom Rail  " );

                    Serial.print( railVoltage[ 1 ] );
                    Serial.print( " V" );
                    Serial.flush( );

                    char oledString[ 30 ];
                    sprintf( oledString, "Bottom Rail\n%0.2f V", (float)railVoltage[ 1 ] );

                    //oled.clear( );
                    oled.clearPrintShow( oledString, 2, true, true, true );
                    //oled.show( );
                }
            }
            brightenedRail = 2;
            break;
        case 4:
            if ( lastPrintedNet != netHighlighted ) {

                DACcolorOverride0 = -2;
                DACcolorOverride1 = 0x000000;
                if ( print == 1 ) {
                    Serial.print( "DAC 0  " );
                    Serial.print( dacOutput[ 0 ] );
                    Serial.print( " V" );
                    Serial.flush( );

                    char oledString[ 30 ];
                    sprintf( oledString, "DAC 0\n%0.2f V", (float)dacOutput[ 0 ] );

                    //  oled.clear( );
                    oled.clearPrintShow( oledString, 2, true, true, true );
                    //oled.show( );
                }
                lastPrintedNet = netHighlighted;
            }
            break;
        case 5:
            if ( lastPrintedNet != netHighlighted ) {

                DACcolorOverride0 = 0x000000;
                DACcolorOverride1 = -2;
                if ( print == 1 ) {
                    Serial.print( "DAC 1  " );
                    Serial.print( dacOutput[ 1 ] );
                    Serial.print( " V" );
                    Serial.flush( );

                    char oledString[ 30 ];
                    sprintf( oledString, "DAC 1\n%0.2f V", (float)dacOutput[ 1 ] );

                    //oled.clear( );
                    oled.clearPrintShow( oledString, 2, true, true, true );
                    //oled.show( );
                }
                lastPrintedNet = netHighlighted;
            }
            break;
        default: {

            if ( print == 1 ) {
                Serial.print( "\r                                          \r" );
                Serial.flush( );
            }

            // Serial.print("  \t ");
            int specialPrint = 0;

            // Detect alternate functions on this net
            bool uartTxOnNet = false;
            bool uartRxOnNet = false;
            bool i2cOnNet = false;
            bool pwmOnNet = false;
            int functionOnNetIndex = -1; // any other function index 0..9
            if ( netHighlighted > 0 ) {
                // gpio indices 8 and 9 correspond to UART TX (pin 0) and RX (pin 1)
                if ( gpioNet[ 8 ] == netHighlighted && gpio_function_map[ 8 ] == GPIO_FUNC_UART ) {
                    uartTxOnNet = true;
                }
                if ( gpioNet[ 9 ] == netHighlighted && gpio_function_map[ 9 ] == GPIO_FUNC_UART ) {
                    uartRxOnNet = true;
                }
                // I2C can be on any pins configured as I2C and tied to this net
                for ( int i = 0; i < 10; i++ ) {
                    if ( gpioNet[ i ] == netHighlighted ) {
                        if ( gpio_get_function( gpioDef[ i ][ 0 ] ) == GPIO_FUNC_I2C || gpio_function_map[ i ] == GPIO_FUNC_I2C ) {
                            i2cOnNet = true;
                            functionOnNetIndex = i;
                            break;
                        } else if ( gpio_get_function( gpioDef[ i ][ 0 ] ) == GPIO_FUNC_PWM || gpio_function_map[ i ] == GPIO_FUNC_PWM ) {
                            pwmOnNet = true;
                            functionOnNetIndex = i;
                            break;
                        } else if ( gpio_get_function( gpioDef[ i ][ 0 ] ) != GPIO_FUNC_SIO ) {
                            functionOnNetIndex = i; // some other function
                        }
                    }
                }
            }

            int adc = anyAdcConnected( netHighlighted );
            int gpioInputNumber = anyGpioInputConnected( netHighlighted );
            int gpioOutputNumber = anyGpioOutputConnected( netHighlighted );

            // for ( int i = 0; i < 10; i++ ) {
            //     if ( gpioNet[ i ] == netHighlighted ) {
            //         Serial.print( "gpioNet[i] = " );
            //         Serial.println( gpioNet[ i ] );
            //         Serial.print( "gpio_function_map[i] = " );
            //         Serial.print( gpio_function_map[ i ] );
            //     }
            // }

            // Serial.print("adc = ");
            // Serial.println(adc);
            // Serial.print("gpioInputNumber = ");
            // Serial.println(gpioInputNumber);
            // Serial.print("gpioOutputNumber = ");
            // Serial.println(gpioOutputNumber);

            if ( uartTxOnNet || uartRxOnNet ) {

                if ( lastPrintedNet != netHighlighted ) {
                    if ( print == 1 ) {
                        // Build UART config string like "115200 8N1"
                        int baud = USBSer1.baud( );
                        int bits = USBSer1.numbits( );
                        int stopbits = USBSer1.stopbits( ) + 1; // API returns 0->1 stop, 1->2 stops
                        int parity = USBSer1.paritytype( );

                        char parityChar = 'N';
                        switch ( parity ) {
                        case 1:
                            parityChar = 'O';
                            break;
                        case 2:
                            parityChar = 'E';
                            break;
                        case 3:
                            parityChar = 'M';
                            break;
                        case 4:
                            parityChar = 'S';
                            break;
                        default:
                            parityChar = 'N';
                            break;
                        }

                        Serial.print( uartTxOnNet && uartRxOnNet ? "UART Tx/Rx  " : ( uartTxOnNet ? "UART Tx    " : "UART Rx    " ) );
                        Serial.print( baud );
                        Serial.print( " " );
                        Serial.print( bits );
                        Serial.print( parityChar );
                        Serial.print( stopbits );
                        Serial.flush( );

                        char oledString[ 32 ];
                        sprintf( oledString, "UART %s\n%d %d%c%d",
                                 ( uartTxOnNet && uartRxOnNet ) ? "Tx/Rx" : ( uartTxOnNet ? "Tx" : "Rx" ),
                                 baud, bits, parityChar, stopbits );
                        oled.clear( );
                        oled.clearPrintShow( oledString, 1, true, true, true );
                        oled.show( );
                    }
                    lastPrintedNet = netHighlighted;
                }
                specialPrint = 1;

            } else if ( i2cOnNet ) {

                if ( lastPrintedNet != netHighlighted ) {
                    if ( print == 1 ) {
                        // Simple I2C label without scanning
                        Serial.print( "I2C  " );
                        //Serial.flush( );
                        const char* line = "I2C";
                        if ( functionOnNetIndex >= 0 ) {
                            int pin = gpioDef[ functionOnNetIndex ][ 0 ];
                            if ( pin == jumperlessConfig.top_oled.sda_pin ) 
                            {
                              line = "I2C  SDA";
                              Serial.print("SDA   ");
                              Serial.flush();
                            }
                            else if ( pin == jumperlessConfig.top_oled.scl_pin ) 
                            {
                              line = "I2C  SCL";
                              Serial.print("SCL   ");
                              Serial.flush();
                            }
                        }
                        char oledString[ 32 ];

                       // Serial.print("i2cSpeed = ");
                        Serial.print(i2cSpeed/1000);
                        Serial.print(" KHz");
                        Serial.flush();
               
                        snprintf( oledString, sizeof( oledString ), "%s\n%d KHz", line, i2cSpeed/1000 );
                        oled.clearPrintShow( oledString, 1, true, true, true );
                       // oled.show( );
                    }
                    lastPrintedNet = netHighlighted;
                }
                specialPrint = 1;

            } else if ( pwmOnNet ) {

                if ( lastPrintedNet != netHighlighted ) {
                    if ( print == 1 ) {
                        float freq = gpioPWMFrequency[ functionOnNetIndex ];
                        float duty = gpioPWMDutyCycle[ functionOnNetIndex ] * 100.0f;
                        Serial.print( "PWM  " );
                        Serial.print( freq, 2 );
                        Serial.print( " Hz  " );
                        Serial.print( duty, 1 );
                        Serial.print( "%" );
                        Serial.flush( );

                        char oledString[ 32 ];
                        sprintf( oledString, "PWM\n%0.0f Hz  %0.0f%%", freq, (duty) );
                        //oled.clear( );
                        oled.clearPrintShow( oledString, 1, true, true, true );
                        //oled.show( );
                    }
                    lastPrintedNet = netHighlighted;
                }
                specialPrint = 1;

            } else if ( functionOnNetIndex != -1 ) {

                if ( lastPrintedNet != netHighlighted ) {
                    if ( print == 1 ) {
                        // Generic function print using gpio_function_names
                        gpio_function_t fun = gpio_function_map[ functionOnNetIndex ];
                        const char* fname = gpio_function_names[ fun ].name;
                        Serial.print( fname );
                        Serial.flush( );

                        char oledString[ 32 ];
                        sprintf( oledString, "%s", fname );
                        //oled.clear( );
                        oled.clearPrintShow( oledString, 1, true, true, true );
                        //oled.show( );
                    }
                    lastPrintedNet = netHighlighted;
                }
                specialPrint = 1;

            } else if ( ( adc != -1 || gpioInputNumber != -1 || gpioOutputNumber != -1 ) ) {

                if ( lastPrintedNet != netHighlighted ) {

                    if ( adc != -1 ) {
                        ADCcolorOverride0 = -2;
                        ADCcolorOverride1 = -2;
                        logoOverrideMap[ 0 ].colorOverride = logoOverrideMap[ 0 ].defaultOverride;
                        logoOverrideMap[ 1 ].colorOverride = logoOverrideMap[ 1 ].defaultOverride;
                        logoOverriden = true;
                        if ( print == 1 ) {
                            Serial.print( "ADC " );
                            Serial.print( adc );
                            Serial.print( "   " );

                            Serial.print( (float)readAdcVoltage( adc, 32 ) );
                            Serial.print( " V" );
                            Serial.flush( );

                            char oledString[ 30 ];
                            sprintf( oledString, "ADC %d\n  %0.2f V", adc, (float)readAdcVoltage( adc, 32 ) );

                            //oled.clear();
                            oled.clearPrintShow( oledString, 2, true, true, true );
                            //oled.show();
                        }
                        specialPrint = 1;
                    }

                    if ( gpioInputNumber != -1 ) {
                        GPIOcolorOverride0 = -2;
                        GPIOcolorOverride1 = -2;
                        logoOverrideMap[ 4 ].colorOverride = logoOverrideMap[ 4 ].defaultOverride;
                        logoOverrideMap[ 5 ].colorOverride = logoOverrideMap[ 5 ].defaultOverride;
                        logoOverriden = true;
                        if ( print == 1 ) {
                            Serial.print( "GPIO " );
                            Serial.print( gpioInputNumber + 1 );
                            Serial.print( " input " );
                            Serial.flush( );

                            int gpioInputState = gpioReadWithFloating( gpioDef[ gpioInputNumber ][ 0 ] );
                            char stateString[ 10 ];
                            switch ( gpioInputState ) {
                            case 0:
                                Serial.print( "low" );
                                strcpy( stateString, "low" );
                                break;
                            case 1:
                                Serial.print( "high" );
                                strcpy( stateString, "high" );
                                break;
                            case 2:
                                Serial.print( "floating" );
                                strcpy( stateString, "floating" );
                                break;
                            default:
                                Serial.print( "?" );
                                strcpy( stateString, "?" );
                                break;
                            }

                            char oledString[ 30 ];
                            sprintf( oledString, "GPIO %d input\n %s", gpioInputNumber + 1, stateString );
                            //oled.clear( );
                            oled.clearPrintShow( oledString, 1, true, true, true );
                            //oled.show( );
                            // Serial.println();
                        }
                        specialPrint = 1;
                    }

                    if ( gpioOutputNumber != -1 ) {
                        GPIOcolorOverride0 = -2;
                        GPIOcolorOverride1 = -2;
                        logoOverrideMap[ 4 ].colorOverride = logoOverrideMap[ 4 ].defaultOverride;
                        logoOverrideMap[ 5 ].colorOverride = logoOverrideMap[ 5 ].defaultOverride;
                        logoOverriden = true;
                        if ( print == 1 ) {
                            Serial.print( "GPIO " );
                            Serial.print( gpioOutputNumber + 1 );
                            Serial.print( " output " );
                            Serial.flush( );

                            char stateString[ 7 ];
                            int gpioOutputState = gpio_get_out_level( gpioDef[ gpioOutputNumber ][ 0 ] );

                            if ( gpioOutputState == 0 ) {
                                Serial.print( "low" );
                                strcpy( stateString, "low" );
                            } else {
                                Serial.print( "high" );
                                strcpy( stateString, "high" );
                            }

                            char oledString[ 30 ];
                            sprintf( oledString, "GPIO %d output\n %s", gpioOutputNumber + 1, stateString );

                            oled.clearPrintShow( oledString, 1, true, true, true );

                            // Serial.println();
                        }
                        specialPrint = 1;
                    }
                }
            } else {
                if ( netHighlighted > 0 ) {
                    int length = 0;
                    if ( print == 1 ) {
                        Serial.print( "Net " );
                        Serial.print( netHighlighted );
                        Serial.print( "\t " );
                        Serial.print( "row " );
                        length = printNodeOrName( brightenedNode );
                        Serial.flush( );

                        char oledString[ 30 ];
                        sprintf( oledString, "Net %d       \n  row %s", netHighlighted, definesToChar( brightenedNode, 0 ) );
                        //oled.clear( );
                        oled.clearPrintShow( oledString, 1, true, true, true );
                        //oled.show( );

                        // for (int i = 0; i < 8 - length; i++) {
                        //   Serial.print(" ");
                    }
                    Serial.flush( );
                }
                if ( specialPrint == 0 ) {
                    // Serial.println();
                }
            }
            lastPrintedNet = netHighlighted;
        }
            Serial.flush( );
        }
    }
    // showLEDsCore2 = 1;

    // Serial.println("netHighlighted: ");
    // Serial.println(netHighlighted);
    // Serial.flush();

    return netHighlighted;
}

int checkForReadingChanges( void ) {
    // Static variables to store previous measurement values
    static float prevAdcReading = 0.0;
    static int prevGpioInputState = -1;
    static int prevGpioOutputState = -1;
    static float prevDacVoltage = 0.0;
    static float prevRailVoltage = 0.0;
    static int lastMeasuredNet = -1;
    static unsigned long lastUpdateTime = 0;

    // Don't update too frequently
    unsigned long currentTime = millis( );
    if ( currentTime - lastUpdateTime < 50 ) { // Minimum 50ms between updates
        return -1;
    }

    // Update showReadingNet to track brightenedNet (but don't clear on timeout)
    if ( brightenedNet > 0 ) {
        showReadingNet = brightenedNet;
    }

    // Check if there's a net to show readings for
    if ( showReadingNet <= 0 ) {
        lastMeasuredNet = -1;
        return -1;
    }

    // Reset stored values if we switched to a different net
    if ( lastMeasuredNet != showReadingNet ) {
        prevAdcReading = 0.0;
        prevGpioInputState = -1;
        prevGpioOutputState = -1;
        prevDacVoltage = 0.0;
        prevRailVoltage = 0.0;
        lastMeasuredNet = showReadingNet;
        lastUpdateTime = currentTime;
        return -1;
    }

    bool displayUpdated = false;
    char oledString[ 30 ];

    // Check for ADC measurements
    int adcChannel = anyAdcConnected( showReadingNet );
    if ( adcChannel != -1 ) {
        float currentAdcReading = readAdcVoltage( adcChannel, 64 );

        // Check if change is significant (>0.05V dead zone)
        if ( fabs( currentAdcReading - prevAdcReading ) > 0.009 ) {
            prevAdcReading = currentAdcReading;

            sprintf( oledString, "ADC %d\n  %0.2f V", adcChannel, currentAdcReading );
            // oled.clear();
            oled.clearPrintShow( oledString, 2, true, true, true );
            // oled.show();
            Serial.print( "\r                                 \r" );

            Serial.printf( "ADC %d   %0.2f V", adcChannel, currentAdcReading );
            Serial.flush( );

            displayUpdated = true;
        }
        showReadingRow = showReadingNet;
    }

    // Check for GPIO input
    int gpioInputNumber = anyGpioInputConnected( showReadingNet );
    if ( gpioInputNumber != -1 ) {
        int currentGpioInputState = gpioReading[ gpioInputNumber ];

        // Update if state changed
        if ( currentGpioInputState != prevGpioInputState ) {
            prevGpioInputState = currentGpioInputState;

            char stateString[ 10 ];
            switch ( currentGpioInputState ) {
            case 0:
                strcpy( stateString, "low" );
                break;
            case 1:
                strcpy( stateString, "high" );
                break;
            case 2:
                strcpy( stateString, "floating" );
                break;
            default:
                strcpy( stateString, "?" );
                break;
            }

            sprintf( oledString, "GPIO %d input\n %s", gpioInputNumber + 1, stateString );
            // oled.clear();
            oled.clearPrintShow( oledString, 1, true, true, true );
            // oled.show();

            Serial.print( "\r                                 \r" );
            Serial.printf( "GPIO %d input %s", gpioInputNumber + 1, stateString );
            Serial.flush( );

            displayUpdated = true;
        }
        showReadingRow = showReadingNet;
    }

    // Check for GPIO output
    int gpioOutputNumber = anyGpioOutputConnected( showReadingNet );
    if ( gpioOutputNumber != -1 ) {
        int currentGpioOutputState = gpio_get_out_level( gpioDef[ gpioOutputNumber ][ 0 ] );

        // Update if state changed
        if ( currentGpioOutputState != prevGpioOutputState ) {
            prevGpioOutputState = currentGpioOutputState;

            char stateString[ 7 ];
            if ( currentGpioOutputState == 0 ) {
                strcpy( stateString, "low" );
            } else {
                strcpy( stateString, "high" );
            }

            sprintf( oledString, "GPIO %d output\n %s", gpioOutputNumber + 1, stateString );
            // oled.clear();
            oled.clearPrintShow( oledString, 1, true, true, true );
            // oled.show();

            displayUpdated = true;
        }
        showReadingRow = showReadingNet;
    }

    // Check for DAC connections (nets 4 and 5)
    if ( showReadingNet == 4 ) { // DAC 0
        float currentDacVoltage = getDacVoltage( 0 );

        // Check if change is significant (>0.05V dead zone)
        if ( fabs( currentDacVoltage - prevDacVoltage ) > 0.05 ) {
            prevDacVoltage = currentDacVoltage;

            sprintf( oledString, "DAC 0\n%0.2f V", currentDacVoltage );
            // oled.clear();
            oled.clearPrintShow( oledString, 2, true, true, true );
            // oled.show();

            displayUpdated = true;
        }
    } else if ( showReadingNet == 5 ) { // DAC 1
        float currentDacVoltage = getDacVoltage( 1 );

        // Check if change is significant (>0.05V dead zone)
        if ( fabs( currentDacVoltage - prevDacVoltage ) > 0.05 ) {
            prevDacVoltage = currentDacVoltage;

            sprintf( oledString, "DAC 1\n%0.2f V", currentDacVoltage );
            // oled.clear();
            oled.clearPrintShow( oledString, 2, true, true, true );
            // oled.show();

            displayUpdated = true;
        }
        showReadingRow = showReadingNet;
    }

    // Check for rail connections (nets 1, 2, 3)
    if ( showReadingNet == 2 ) { // Top Rail
        float currentRailVoltage = railVoltage[ 0 ];

        // Check if change is significant (>0.05V dead zone)
        if ( fabs( currentRailVoltage - prevRailVoltage ) > 0.05 ) {
            prevRailVoltage = currentRailVoltage;

            sprintf( oledString, "Top Rail\n%0.2f V", currentRailVoltage );
            // oled.clear();
            oled.clearPrintShow( oledString, 2, true, true, true );
            // oled.show();

            displayUpdated = true;
        }
    } else if ( showReadingNet == 3 ) { // Bottom Rail
        float currentRailVoltage = railVoltage[ 1 ];

        // Check if change is significant (>0.05V dead zone)
        if ( fabs( currentRailVoltage - prevRailVoltage ) > 0.05 ) {
            prevRailVoltage = currentRailVoltage;

            sprintf( oledString, "Bottom Rail\n%0.2f V", currentRailVoltage );
            //  oled.clear();
            oled.clearPrintShow( oledString, 2, true, true, true );
            // oled.show();

            displayUpdated = true;
        }
        showReadingRow = showReadingNet;
    }

    if ( displayUpdated ) {
        lastUpdateTime = currentTime;
        return 1; // Indicates display was updated
    }

    return -1; // No updates
}