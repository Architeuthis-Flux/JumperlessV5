#include "Apps.h"
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Commands.h"
#include "FileParsing.h"
#include "FilesystemStuff.h"
#include "Graphics.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "MachineCommands.h"
#include "MatrixState.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "Python_Proper.h"
#include "RotaryEncoder.h"
#include "config.h"
#include "configManager.h"
#include "oled.h"

// #include <PNGdec.h>
// #include <PNGDisplay.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>




#define OLED_CONNECTED 0

// Helper function to normalize spaces in a string (strip leading/trailing and reduce multiple spaces to single)
String normalizeSpaces( const char* str ) {
    String normalized = String( str );
    normalized.trim( ); // Remove leading and trailing spaces

    // Replace multiple consecutive spaces with single space
    String result = "";
    bool lastWasSpace = false;
    for ( unsigned int i = 0; i < normalized.length( ); i++ ) {
        if ( normalized[ i ] == ' ' ) {
            if ( !lastWasSpace ) {
                result += ' ';
                lastWasSpace = true;
            }
        } else {
            result += normalized[ i ];
            lastWasSpace = false;
        }
    }

    return result;
}

// Wrapper function for i2cScan to match void(*)(void) signature
void i2cScanWrapper( void ) {
    i2cScan( ); // Call with default parameters
}

struct app apps[ 30 ] = {
    { "Bounce Startup", 0, 1, bounceStartup },
    { "Calib  DACs", 1, 1, calibrateDacs },
    { "I2C    Scan", 2, 1, i2cScanWrapper },
    { "Custom App", 3, 1, customApp },
    { "PNG Image", 4, 1, displayImage },
    { "Scan", 5, 1, scanBoard },
    { "XLSX   GUI", 6, 1, xlsxGui },
    { "Micropython", 7, 1, microPythonREPLapp },
    { "uPythonREPL", 8, 1, microPythonREPLapp },
    { "File   Manager", 9, 1, fileManagerApp },
    { "eKilo Editor", 10, 1, eKiloApp },
    { "Probe  Calib", 11, 1, probeCalibApp },
    { "JDI MIPdisplay", 12, 1, jdiMIPdisplay },
    

    { "DOOM", 16, 1, playDoom },

    // {"Pong", 17, 1, playPong},
    // {"Tetris", 18, 1, playTetris},
    // {"Snake", 19, 1, playSnake},
    // {"Delete", 20, 1, deleteApp},
    // {"Upload", 21, 1, uploadApp},

};

void runApp( int index, char* name ) {

    //   Serial.print("running app: ");
    //   Serial.println(index);
    //   Serial.println(name);
    if ( index != -1 ) {
        Serial.println( normalizeSpaces( apps[ index ].name ) );

    } else if ( name != nullptr ) {
        Serial.println( normalizeSpaces( name ) );
    }
    Serial.println( );
    // Find matching app if only one parameter is given
    if ( index == -1 ) {
        String normalizedSearchName = normalizeSpaces( name );
        for ( int i = 0; i < sizeof( apps ) / sizeof( apps[ 0 ] ); i++ ) {
            String normalizedAppName = normalizeSpaces( apps[ i ].name );
            if ( normalizedAppName.equalsIgnoreCase( normalizedSearchName ) ) {
                index = i;
                break;
            }
        }
    }
    if ( name == nullptr ) {
        name = apps[ index ].name;
    }

    // Run the app based on index
    Serial.println( "Running app: " + normalizeSpaces( name ) );
    Serial.println( "Index: " + String( index ) );

    switch ( index ) {
    case 0:
        bounceStartup( );
        break;
    case 1:
        calibrateDacs( );
        break;
    case 2:
        i2cScan( );
        break;

    case 3:
        customApp( );
        break;
    case 4:
        displayImage( );
        break;

    case 5:
        scanBoard( );
        break;
    case 6:
        xlsxGui( );
        break;
    case 7:
        // micropython();
        microPythonREPLapp( );
        break;
    case 8:
        microPythonREPLapp( );
        break;
    case 9:
        fileManagerApp( );
        break;
    case 10:
        eKiloApp( );
        break;
    case 11:
        probeCalibApp( );
        break;
    case 12:
    Serial.println("JDI_MIP_Display starting");
    Serial.flush();
    delay(1000);
        jdiMIPdisplay( );
        break;
        // case 2: logicAnalyzer(); break;
        // case 3: oscilloscope(); break;
        // case 4: midiSynth(); break;
        // case 5: i2cScanner(); break;
        // case 6: selfDestruct(); break;
        // case 7: eepromDumper(); break;
        // case 8: segmentMapper(); break;
        // case 9: rickRoll(); break;
        // case 10: circuit555(); break;
        // case 11: opAmp(); break;
        // case 12: ic74x109(); break;
        // case 13: ic74x161(); break;
        // case 14: ic74x42(); break;
        // case 15: ic74x595(); break;
    case 16:
        playDoom( );
        break;
        // case 17: playPong(); break;
        // case 18: playTetris(); break;
        // case 19: playSnake(); break;
        // case 20: deleteApp(); break;
        // case 21: uploadApp(); break;
    default:
        break;
    }
}

void microPythonREPLapp( void ) {
    enterMicroPythonREPL( global_mp_stream );
}

void leaveApp( int lastNetSlot ) {

    createSlots( 8, 1 );
    netSlot = lastNetSlot;
    refreshConnections( -1, 0, 1 );
}

void fileManagerApp( void ) {

    filesystemApp( false );
}

void probeCalibApp( void ) {

    b.clear( );

    cycleTerminalColor( true, 5.0, true );
    Serial.println( "Probe Calibration App" );

    cycleTerminalColor( );

    Serial.println( "\n\rTap rows with the probe and rotate the clickweel until they're lighting up the correct row" );

    cycleTerminalColor( );

    Serial.println( "be sure to check nano header rows too" );

    cycleTerminalColor( );

    Serial.println( "Hold the clickwheel when you're done\n\n\r" );

    cycleTerminalColor( );

    bool done = false;

    b.clear( );
    int lastNetSlot = netSlot;
    netSlot = 8;
    createSlots( 8, 1 );

    refreshConnections( -1, 0 );
    routableBufferPower( 1, 1, 1 );
    resetEncoderPosition = true;
    int lastEncoderPosition = encoderPosition;
    int reading = 0;
    int lastReading = -1;

    int probeMax = jumperlessConfig.calibration.probe_max;

    int nodeSelected = -1;
    int lastNodeSelected = -1;

    int nodeSelectedWithOldMapping = -1;
    int lastNodeSelectedWithOldMapping = -1;

    int lastValidProbeRead = -1;
    int lastSwitchPosition = -1;

    int measureOrSelect = 0;

    float measureModeOutputVoltage = jumperlessConfig.calibration.measure_mode_output_voltage;

    while ( done == false ) {

        int probeRead = readProbeRaw( 0, true );

        if ( probeRead != -1 ) {
            lastValidProbeRead = probeRead;
        }

        int rowProbed = map( lastValidProbeRead, jumperlessConfig.calibration.probe_min, jumperlessConfig.calibration.probe_max, 101, 0 );
        int rowProbedWithOldMapping = map( lastValidProbeRead, jumperlessConfig.calibration.probe_min, probeMax, 101, 0 );

        nodeSelected = probeRowMap[ rowProbed ];
        nodeSelectedWithOldMapping = probeRowMapByPad[ rowProbedWithOldMapping ];

        int checkSwitch = checkSwitchPosition( );

        if ( checkSwitch != lastSwitchPosition ) {
            if ( checkSwitch == 0 ) {
                lastSwitchPosition = 0;
                measureOrSelect = 0;
                measureModeOutputVoltage = jumperlessConfig.calibration.measure_mode_output_voltage;
            } else {
                lastSwitchPosition = 1;
                measureOrSelect = 1;
                probeMax = jumperlessConfig.calibration.probe_max;
            }
            resetEncoderPosition = true;
        }

        reading = rowProbed;

        if ( encoderPosition != lastEncoderPosition || reading != lastReading ) {
            lastEncoderPosition = encoderPosition;
            if ( measureOrSelect == 0 ) {

                jumperlessConfig.calibration.measure_mode_output_voltage = measureModeOutputVoltage - ( (float)encoderPosition / 2000.0 );

                if ( jumperlessConfig.calibration.measure_mode_output_voltage < 2.8 ) {
                    jumperlessConfig.calibration.measure_mode_output_voltage = 2.8;
                } else if ( jumperlessConfig.calibration.measure_mode_output_voltage > 4.5 ) {
                    jumperlessConfig.calibration.measure_mode_output_voltage = 5.0;
                }
                setDac0voltage( jumperlessConfig.calibration.measure_mode_output_voltage, 1, 0, false );

            } else {

                jumperlessConfig.calibration.probe_max = probeMax - encoderPosition;
                if ( jumperlessConfig.calibration.probe_max < 15 ) {
                    jumperlessConfig.calibration.probe_max = 15;
                }
            }
            Serial.printf( "                                  \rraw: %d enc: %d reading: %d max: %d node: %s mode: %s v: %0.4f",
                           lastValidProbeRead, encoderPosition, rowProbed, jumperlessConfig.calibration.probe_max, definesToChar( nodeSelected ), measureOrSelect ? "measure" : "select", jumperlessConfig.calibration.measure_mode_output_voltage );
            Serial.flush( );
        }

        if ( reading == -1 ) {
            continue;
        }

        if ( reading != lastReading && reading != -1 ) {
            // b.clear();
            clearLEDsExceptRails( );
            // if (probeRead != -1) {
            //  b.lightUpNode(lastReading, 0x000000);

            //}
            if ( nodeSelected != nodeSelectedWithOldMapping && measureOrSelect == 1 ) {
                // b.lightUpNode(lastNodeSelectedWithOldMapping, 0x000000);
                b.lightUpNode( nodeSelectedWithOldMapping, 0x050205 );
            }
            if ( measureOrSelect == 0 ) {
                b.lightUpNode( nodeSelected, 0x200010 );
            } else {
                b.lightUpNode( nodeSelected, 0x001030 );
            }
        }
        lastReading = reading;
        lastNodeSelected = nodeSelected;
        lastNodeSelectedWithOldMapping = nodeSelectedWithOldMapping;

        if ( encoderButtonState == HELD ) {
            done = true;
        }
    }

    Serial.println( "\n\n\r" );
    Serial.println( "Saving config..." );

    saveConfig( );
    leaveApp( lastNetSlot );
}
// this just does a bunch of random stuff as an example
void customApp( void ) {
    leds.clear( );
    b.clear( );
    delay( 1000 );
    b.print( "This isa demo", (uint32_t)0x002008 );
    showLEDsCore2 = 2;
    delay( 2000 );
    b.clear( );
    b.print( "It will show", (uint32_t)0x002008 );
    // showLEDsCore2 = -2;
    delay( 1000 );
    b.clear( );
    b.print( "random stuff", (uint32_t)0x002008 );
    // showLEDsCore2 = -2;
    delay( 1000 );
    b.clear( );
    b.print( "look atthe FW", (uint32_t)0x002008 );

    delay( 1000 );

    int lastNetSlot = netSlot;
    netSlot = 8; // this is the net slot that will be used for the custom apa

    int leave = 0;

    createSlots( 8, 1 );

    //! add some bridges to the net file
    addBridgeToNodeFile( 12, 25, netSlot, 0, 0 ); // netSlot is the current slot
    addBridgeToNodeFile( TOP_RAIL, 52, netSlot, 0, 0 );
    refreshConnections(
        -1, 1 ); // you need to refresh connections to make the changes take effect

    //! change the top rail voltage
    setTopRail( (float)3.3 ); // this will set the top rail to 3.3V

    //! GPIO
    addBridgeToNodeFile( RP_GPIO_1, 42, netSlot, 0, 0 );
    addBridgeToNodeFile( RP_GPIO_2, 42, netSlot, 0, 0 );
    refreshConnections(
        -1, 0 ); // you need to refresh connections to make the changes take effect

    pinMode( GPIO_1_PIN,
             OUTPUT ); // I need to change this to GPIO 0 (so GPIO 0 = pin 20, 1 =
                       // 21, etc.), but for now they start at 1
    pinMode( GPIO_2_PIN, INPUT );

    // set GPIO 1 high and read it with GPIO 2
    digitalWrite( GPIO_1_PIN, HIGH );
    int reading = digitalRead( GPIO_2_PIN );
    Serial.print( "GPIO 2 Reading: " );
    Serial.println( reading );

    delay( 10 );

    // set GPIO 1 low and read it with GPIO 2
    digitalWrite( GPIO_1_PIN, LOW );
    reading = digitalRead( GPIO_2_PIN );
    Serial.print( "GPIO_2_PIN: " );
    Serial.println( reading );

    removeBridgeFromNodeFile( RP_GPIO_1, 42, netSlot, 0 );
    removeBridgeFromNodeFile( RP_GPIO_2, 42, netSlot, 0 );
    refreshConnections( -1, 0 );

    //! DAC
    // connect the DAC to row 20 and set it to 5.35V
    setDacByNumber( 1, 5.35 );

    addBridgeToNodeFile( DAC1, 20, netSlot, 0, 0 ); // connect it
    refreshConnections( -1, 0 );
    delay( 10 );

    //! ADC
    // connect the ADC to the same row as the DAC and read the voltage
    addBridgeToNodeFile( ADC0, 20, netSlot, 0,
                         0 ); // note that this is the same row as the DAC
    refreshConnections( -1, 0 );

    // delay(100); //give it a sec (refreshConnections() is async on core 2, so it
    // might not be done yet)
    waitCore2( ); // or you could also use this to wait for the core 2 to finish

    float voltage = readAdcVoltage( 0, 8 ); // readADC() will return the 0-4096
    Serial.print( "\n\rADC0: " );
    Serial.println( voltage );

    if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }

    //! print the nets (should show ADC0 and DAC1 connected)
    listNets( );
    listSpecialNets( );
    Serial.println( "\n\n\r" );
    printPathsCompact( ); // print the actual chip connections
    Serial.println( "\n\n\n\n\r" );

    // remove the ADC and DAC bridges
    removeBridgeFromNodeFile( ADC0, 20, netSlot, 0 );
    removeBridgeFromNodeFile( DAC1, 20, netSlot, 0 );
    refreshConnections( -1, 0 );

    //! measure current
    addBridgeToNodeFile( ISENSE_PLUS, 9, netSlot, 0, 0 );
    addBridgeToNodeFile( ISENSE_MINUS, 28, netSlot, 0, 0 );
    refreshConnections( );

    float current_ma = INA0.getCurrent_mA( );

    Serial.print( "current between row 19 and 28: " );
    Serial.print( current_ma );
    Serial.println( " mA\n\r" );

    removeBridgeFromNodeFile( ISENSE_PLUS, 9, netSlot, 0 );
    removeBridgeFromNodeFile( ISENSE_MINUS, 28, netSlot, 0 );
    refreshConnections( -1, 0 );
    delay( 1 );

    if ( encoderButtonState == PRESSED && lastButtonEncoderState == RELEASED ||
         Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }
    //! use INA219 to measure voltage
    // turns out the INA219 is the most accurate voltage measurement on the board
    // too (only for positive voltages though) and is what the ADC are calibrated
    // against
    setTopRail( 4.20F ); // this will set the top rail to 2.3V
    addBridgeToNodeFile( TOP_RAIL, ISENSE_PLUS, netSlot, 0, 0 );
    refreshConnections(
        -1, 0 ); // you need to refresh connections to make the changes take effect
    waitCore2( );
    delay( 10 );
    float voltage2 = INA0.getBusVoltage( );
    Serial.print( "top rail voltage (from INA219): " );
    Serial.print( voltage2, 4 );
    Serial.println( " V\n\r" );

    removeBridgeFromNodeFile( TOP_RAIL, ISENSE_PLUS, netSlot, 0 );
    refreshConnections(
        -1, 0 ); // you need to refresh connections to make the changes take effect

    //! probe tip (in measure mode)
    // get the voltage on the probe tip
    float probeVoltage =
        readAdcVoltage( 7, 8 ); // ADC 7 is hardwired to the probe tip (in measure
                                // mode), so it's this easy
    Serial.print( "Probe voltage: " );
    Serial.print( probeVoltage, 4 );
    Serial.println( " V\n\r" );

    // get the current on the probe tip
    // the probe tip is hardwired to ROUTABLE_BUFFER_IN, so it connects that to
    // DAC 1, sets the DAC 1 to 3.33V (which it does by default right now so you
    // can use the probe to tap rows in measure mode), and then measures the
    // current the reason this exists is to tell the position of the switch
    // (measure or select) by sensing whether DAC 1 is powering the probe LEDs
    float probeCurrent = checkProbeCurrent( );
    Serial.print( "Probe current: " );
    Serial.print( probeCurrent, 2 );
    Serial.println( " mA\n\r" );
    // this will be something around ~1-3mA if it's in select mode (because it's
    // whatever the probe LEDs are drawing)

    if ( encoderButtonState == PRESSED && lastButtonEncoderState == RELEASED ||
         Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }
    //! printing stuff on the breadboard
    showLEDsCore2 =
        -3; // this tells the second core to write to the LEDs (negative numbers
            // clear first, check loop1() in main.cpp to see what it's doing)
    // 3 or -3 will "hold" control of the LEDs (so animations and other stuff
    // aren't drawn)

    b.clear( );              // clear the screen
    clearLEDsExceptRails( ); // these are fairly similar, but this is more direct

    // draw with raw rows
    b.printRawRow( 0b00001, 55, 0x170010, 0xffffff );
    b.printRawRow( 0b00011, 56, 0x150012, 0xffffff );
    b.printRawRow( 0b00111, 57, 0x130013, 0xffffff );
    b.printRawRow( 0b01111, 58, 0x100015, 0xffffff );
    b.printRawRow( 0b11111, 59, 0x080018, 0xffffff );

    // text (also shows the insane way I deal with colors)
    for ( uint8_t i = 0; i < 254; i++ ) {
        hsvColor hsvTextColor = { i, 255, 30 };
        rgbColor rgbTextColor = HsvToRgb( hsvTextColor );
        uint32_t textColor = (uint32_t)rgbTextColor.r << 16 |
                             (uint32_t)rgbTextColor.g << 8 |
                             (uint32_t)rgbTextColor.b;
        b.print( "Fuck    you!", (uint32_t)textColor );

        showLEDsCore2 = -3;
        delayMicroseconds( 200 );
    }

    if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }
    //! check switch position
    int switchPosition = checkSwitchPosition( );
    Serial.print( "Switch position: " );
    if ( switchPosition == 0 ) {
        Serial.println( "Measure" );
    } else {
        Serial.println( "Select" );
    }

    //! get row input from the probe
    b.clear( );
    b.print( " Tap    Rows!", (uint32_t)0x00140a );
    showLEDsCore2 = -3;
    delay( 100 );
    showLEDsCore2 = -1; //-1 will clear and then draw the wires as they are in the
                        // node file (without any extra stuff we drew)

    // check for tapped rows until the probe button is pressed
    int probeRow = -1;
    int lastProbedRow = 0; // store this so we can clear it

    Serial.println( "Click the probe button to exit\n\n\n\r" );

    while ( checkProbeButton( ) == 0 ) {
        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            leaveApp( lastNetSlot );
            return;
        }
        probeRow = justReadProbe( ); // justReadProbe() returns the row number of the
                                     // probe, or -1 if it is not touching anything

        if ( probeRow != -1 ) {
            Serial.print( "\r                 \rProbed row: " );
            printNodeOrName( probeRow ); // this will print names for the rows

            b.printRawRow( 0b11111, lastProbedRow - 1, 0x000000,
                           0xffffff ); // clear the last probed row
            b.printRawRow(
                0b11111, probeRow - 1, 0x172000,
                0xffffff ); // ugh these are off by 1 (because the first row is 0)

            b.print( probeRow, (uint32_t)0x002008 ); // this will print the row number
                                                     // on the breadboard
            // showLEDsCore2 = -2;
            lastProbedRow = probeRow;
            delayMicroseconds( 100 );
        }
    }

    //! remove bridges from the node file
    removeBridgeFromNodeFile( 12, -1, netSlot, 0 );
    removeBridgeFromNodeFile( 52, -1, netSlot,
                              0 ); //-1 means remove all bridges to this node
    refreshConnections(
        -1, 0 ); // you need to refresh connections to make the changes take effect

    //! using the local node file (in RAM)
    // most of the stuff above should have actually been done this way, because
    // we're making connections temporarily so they don't need to be stored in
    // flash writing to flash is slow and needs to pause the other core while it
    // does it's thing, so this is is *way* faster

    addBridgeToNodeFile( 12, 25, netSlot,
                         1 );         // note the last parameter flashOrLocal is 1
    refreshLocalConnections( -1, 0 ); // you still need to refresh connections
    delay( 100 );

    removeBridgeFromNodeFile( 12, -1, netSlot,
                              1 );    // remove all bridges to this node
    refreshLocalConnections( -1, 0 ); // you still need to refresh connections
    delay( 100 );
    // the only difference is that the last parameter is 1 instead of 0 and
    // calling refreshLocalConnections() instead of refreshConnections()

    if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }

    //! speed test: RAM VS flash
    // this isn't really so much about hardware as it is about the my code, I
    // could probably make flash access faster if I put in some effort

    // RAM
    for ( int i = 1; i <= 31; i++ ) {
        removeBridgeFromNodeFile( 1, i - 1, netSlot, 1 );
        addBridgeToNodeFile( 1, i, netSlot, 1 );
        refreshLocalConnections( -1, 0 ); // you still need to refresh connections

        showLEDsCore2 = -1;
        waitCore2( ); // wait for the other core to finish
    }

    removeBridgeFromNodeFile( 30, -1, netSlot, 1 ); // remove that last bridge
    refreshLocalConnections( -1, 0 );               // you still need to refresh connections

    if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
        leaveApp( lastNetSlot );
        return;
    }
    // now let's do the same thing but with flash
    for ( int i = 31; i <= 60; i++ ) {
        removeBridgeFromNodeFile( 31, i - 1, netSlot, 0 );
        addBridgeToNodeFile( 31, i, netSlot, 0 );
        refreshConnections( -1, 0 ); // you still need to refresh connections
        showLEDsCore2 = -1;
        // waitCore2(); //wait for the other core to finish
        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            leaveApp( lastNetSlot );
            return;
        }

        // delayMicroseconds((31 - i) * 100);
    }
    removeBridgeFromNodeFile( 60, -1, netSlot, 1 ); // remove that last bridge
    refreshConnections( -1, 0 );                    // you still need to refresh connections
    delay( 100 );

    //! raw CH446Q connections
    // you could even do this ~10,000 faster by sending raw X and Y connections
    // directly to the CH446Qs you'll need to be staring at the schematic it use
    // this, but for doing stuff like "fake GPIO" by connecting a row to GND and a
    // rail really fast or something, this might be useful

    sendXYraw( CHIP_K, 4, 0, 1 ); // TOP_RAIL to AK
    sendXYraw( CHIP_A, 9, 1, 1 ); // AK to row 1

    // let's see how fast of a PWM we can get on pin 1
    // we can leave AK to row 1 connected and just toggle between TOP_RAIL to AK
    // and GND to AK
    //! fake GPIO
    unsigned long startTime = millis( );
    unsigned long timeout = 5000; // 10 seconds

    while ( millis( ) - startTime < timeout ) {
        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            leaveApp( lastNetSlot );
            return;
        }
        sendXYraw( CHIP_K, 4, 0, 1 ); // connect TOP_RAIL to AK
        delayMicroseconds( 100 );
        sendXYraw( CHIP_K, 4, 0, 0 );  // disconnect TOP_RAIL from AK
        sendXYraw( CHIP_K, 15, 0, 1 ); // connect GND to AK
        delayMicroseconds( 100 );
        sendXYraw( CHIP_K, 15, 0, 0 ); // disconnect GND from AK
    }

    // clean up
    sendXYraw( CHIP_K, 4, 0, 0 );  // disconnect TOP_RAIL from AK
    sendXYraw( CHIP_K, 15, 0, 0 ); // disconnect TOP_RAIL from AK
    sendXYraw( CHIP_A, 9, 1, 0 );  // disconnect AK from row 1

    // netSlot = lastNetSlot;
}

void xlsxGui( void ) {
    // this is just a test for the xlsx parser
    // it will read the serial data sent from the xlsx gui and print the contents
    // to the serial monitor
    //  Serial.print(); is the Jumperless serial (seen in the terminal)
    //  USBSer1.print(); is the USB serial reply (seen on the second com port)

    // Global variables
    unsigned long startTime = millis( );
    unsigned long messageTime = startTime; // Time to print a message
    unsigned long timeout = 1000000;       // 1000 seconds

    String string_from_UART = "";
    String command = "", args = "";
    String queuedCommand = "  "; // TODO: check if this is still needed
    bool isEchoEnabled = true;   // true causes serial commands to be echoed to the
                                 // Jumperless serial, false mutes the echo
    bool isJumperlessReply = false;

    int DT = 10; // Delay time in milliseconds Excel needs

    // One-time setup
    // USBSer1.ignoreFlowControl(true);
    USBSer1.println( F( "Message,Jumperless GUI started" ) );
    USBSer1.flush( );

    // Do continuously until timeout
    while ( millis( ) - startTime < timeout ) {
        uint32_t currentMillis = millis( );

        // Exit the app if the button is pressed or serial data is available
        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            Serial.println( F( "Exiting XLSX GUI early" ) );
            break;
        }

        // non-blocking test print
        if ( currentMillis > messageTime ) {    // every second
            messageTime = currentMillis + 1000; // set next message time
            USBSer1.dtr( );
            USBSer1.print( F( "Message,Jumperless GUI is running..." ) );
            USBSer1.print( F( "\n" ) ); // print a new line
            USBSer1.flush( );
            USBSer1.println( F( "|Message,,Jumperless GUI is running..." ) );
            USBSer1.flush( );
        }

        // Read serial data
        if ( USBSer1.available( ) ) {
            char c = USBSer1.read( );
            if ( isEchoEnabled ) {
                Serial.print( c );
                Serial.flush( ); // maybe not needed?
            }
            if ( ( c != '\n' ) && ( c != '\r' ) && ( c != '|' ) ) {
                string_from_UART += c;
            } else {
                // Process the command
                string_from_UART.trim( );
                if ( string_from_UART.length( ) > 0 ) {
                    delay( DT ); // just to make sure it's not under the minimum timing for
                                 // Excel
                    // Parse the command and arguments
                    int index_of_first_comma = string_from_UART.indexOf( ',' );
                    bool isExcelCommand = false;
                    // !!!! Drop leading ","s so that multiple commands can be sent from
                    // Excel !!!!
                    if ( index_of_first_comma ==
                         0 ) { // If the first char is a comma, strip it and use the
                               // remainder as the string
                        isExcelCommand = true;
                        string_from_UART =
                            string_from_UART.substring( 1, string_from_UART.length( ) );
                        index_of_first_comma = string_from_UART.indexOf(
                            ',' ); // update the "1st" comma location
                    }
                    // If there is a comma, split the command and arguments
                    if ( index_of_first_comma != -1 ) {
                        command = string_from_UART.substring( 0, index_of_first_comma );
                        args = string_from_UART.substring( index_of_first_comma + 1 );
                    } else {
                        command = string_from_UART;
                        args = "";
                    }

                    // Print the command and arguments
                    USBSer1.print( F( "Command: " ) );
                    USBSer1.println( command );
                    USBSer1.print( F( "Arguments: " ) );
                    USBSer1.println( args );
                    USBSer1.flush( );

                    // Process <command> and <args>
                    if ( command == "h" || command == "help" ) {
                        // Message dictionary:
                        delay( DT );
                        USBSer1.println( F( "" ) );
                        delay( DT );
                        USBSer1.println( F( "|'h' or 'help',, Prints this list" ) );
                        delay( DT );
                        USBSer1.println( F(
                            "|'echo,<val>' ,, Prints the echo setting; <val> (optional)" ) );
                        delay( DT );
                        USBSer1.println( F( "|             ,, is 'on' or 'off' to set the "
                                            "echo setting accordingly" ) );
                        delay( DT );
                        USBSer1.println( F( "|'TakeOnMe'   ,, Plays Take On Me from pin 11" ) );
                        delay( DT );
                        USBSer1.println( F( "|'dance'      ,, progresses the 'dance' LED" ) );
                        delay( DT );
                        USBSer1.println( F( "|             ,, pattern by one step" ) );
                        delay( DT );
                        USBSer1.println(
                            F( "|'send,<val>' ,, Sends a single-character command " ) );
                        delay( DT );
                        USBSer1.println(
                            F( "|             ,, to Jumperless (<val> = n, q, or Q)" ) );
                        delay( DT );
                        USBSer1.println( F( "|'P0_?,<val>' ,, Sends <val> as 'f' command to "
                                            "Jumperless; prints" ) );
                        delay( DT );
                        USBSer1.println(
                            F( "|             ,, an abridged result; then sends 'n' to" ) );
                        delay( DT );
                        USBSer1.println( F( "|             ,, Jumperless and prints an "
                                            "Excel-compatible result;" ) );
                        delay( DT );
                        USBSer1.println(
                            F( "|             ,, <val> = '' will clear all connections" ) );
                        delay( DT );
                        USBSer1.println( F( "|             ,, P0_H/P0_L sets GPIO 0 HIGH/LOW "
                                            "respectively" ) );
                        delay( DT );
                        USBSer1.println( F( "|             ,, TBD..." ) );
                    } else if ( command == "echo" ) {
                        if ( args == "" ) {
                            USBSer1.print( F( "Serial echo is currently " ) );
                            if ( isEchoEnabled ) {
                                USBSer1.println( F( "enabled" ) );
                            } else {
                                USBSer1.println( F( "disabled" ) );
                            }
                        } else if ( args == "on" ) {
                            isEchoEnabled = true;
                        } else if ( args == "off" ) {
                            isEchoEnabled = false;
                        } else { // If there is an argument, but it's not one of the known
                                 // ones...
                            USBSer1.println( F( "The echo command only accepts the arguments "
                                                "'on' and 'off'." ) );
                        }
                        delay( DT );
                    }
                    // else if( command == "TakeOnMe" ) {
                    //     if( args == "" ) {
                    //         playSong(11);
                    //     }
                    //     else if( args == "3" || args == "5" || args == "6" || args ==
                    //     "9" || args == "10" || args == "11" ) {
                    //         uint8_t selectedPin = args.toInt();
                    //         playSong(selectedPin);
                    //     }
                    //     else {
                    //         USBSer1.print(F("Invalid argument following TakeOnMe
                    //         command"));
                    //     }
                    // }
                    else {
                        USBSer1.print( F( "Unknown command: " ) );
                        if ( isExcelCommand ) {
                            USBSer1.print( F( "," ) ); // Added just for Excel
                        }
                        USBSer1.println( string_from_UART );
                        delay( DT );
                    }
                    // Force a flush to ensure any reply is sent
                    USBSer1.flush( );
                    // Clear the string for the next command
                    string_from_UART = "";
                }
            }
        }
    }
    // Force a flush to ensure any reply is sent
    Serial.println( F( "XLSX GUI Done!" ) );
    Serial.flush( );
    USBSer1.println( F( "XLSX GUI Done!" ) );
    USBSer1.flush( );
    return;
}

void bounceStartup( void ) {

    Serial.print( "\n\rPress any key to exit\n\n\r" );
    leds.clear( );
    pauseCore2 = 1;

    int bounceDelay = 300;
    resetEncoderPosition = true;
    while ( 1 ) {

        drawAnimatedImage( 0, bounceDelay );
        // rotaryEncoderStuff();

        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            // leaveApp(lastNetSlot);
            break;
        }
        delayMicroseconds( bounceDelay );
        drawAnimatedImage( 1, bounceDelay );
        //  delay(100);
        if ( digitalRead( BUTTON_ENC ) == 0 || Serial.available( ) > 0 ) {
            // leaveApp(lastNetSlot);
            break;
        }
        // clearLEDsExceptRails();
        // showLEDsCore2 = 1;

        // delay(2000);
        // rainbowBounce(3);
    }
    pauseCore2 = 0;
    // showNets();
    // lightUpRail(-1, -1, 1);
    showLEDsCore2 = -1;
    waitCore2( );
}

void scanBoard( void ) {
    int countLoop = 0;
    int countMult = 18;
    // measureModeActive = 1;
    refreshConnections( -1, 0 );

    Serial.println( "\n\n\r" );
    // showLEDsCore2 = -1;

    int lastRow = 0;
    int lastFloat = 0;
    int lastNode = 0;
    int leave = 0;

    while ( Serial.available( ) == 0 && leave == 0 ) {

        for ( int i = 1; i < 96; i++ ) {

            if ( i == 84 || i == NANO_RESET_0 || i == NANO_RESET_1 ) {
                continue;
            }
            if ( i > 60 && i < 70 || i == 17 ) {
                continue;
            }
            struct rowLEDs currentRow = getRowLEDdata( i );

            // b.printRawRow(0b00100, i - 1, 0x100010, 0xFFFFFF);
            //  addBridgeToNodeFile(i, 2, netSlot, 1);
            //  printNodeOrName(i);
            //  Serial.println();
            float measuredVoltage = measureVoltage( 2, i, true );

            if ( measuredVoltage == 0xFFFFFFFF ) {
                if ( lastFloat == -1 || i == 1 ) {
                    lastFloat = i;
                    printNodeOrName( i );
                    Serial.print( " - " );

                } else {
                }
            } else {

                printNodeOrName( lastRow );
                Serial.println( "\tfloating" );
                lastFloat = -1;
                Serial.print( "\t\t\t" );

                int len = printNodeOrName( i );
                for ( int j = 0; j < 3 - len; j++ ) {
                    Serial.print( " " );
                }

                Serial.print( " = " );
                Serial.print( measuredVoltage );
                Serial.println( " V" );
                Serial.flush( );
            }
            //
            // delay(50);

            setRowLEDdata( i, currentRow );

            // rotaryEncoderStuff();
            if ( Serial.available( ) > 0 ) {
                leave = 1;
                break;
            }
            if ( encoderButtonState == PRESSED ) { //|| digitalRead(BUTTON_PIN) == 0) {
                // break;
                leave = 1;
                break;
            }

            showLEDsCore2 = 2;
            // waitCore2();
            delay( 3 );
            lastRow = i;

            // if (leave == 1) {
            //     break;
            // }
        }

        Serial.println( "\r                   \r\n\n\r" );
        Serial.flush( );
        // printNodeOrName(NANO_A7);
        // Serial.println("\tfloating\n\n\r");
        countLoop++;
        if ( countLoop * countMult > 95 ) {
            // break;
            countLoop = 0;
            countMult -= 2;
        }
        if ( leave == 1 ) {
            break;
        }
    }
    // refreshConnections(-1, 0);

    // measureModeActive = 0;
}

int i2cScan( int sdaRow, int sclRow, int sdaPin, int sclPin,
             int leaveConnections ) {
    // If no rows specified, return error

    if ( sdaRow < 0 || sclRow < 0 ) {
        // Serial.println("Error: Must specify both SDA and SCL rows");
        // return -1;
        // sda = 26;
        // sclRow = 27;
        Serial.println( "defaulting to \n\n\rGPIO 7 = SDA\n\rGPIO 8 = SCL" );

    } else {

        // Remove any existing connections from the GPIO pins
        // removeBridgeFromNodeFile(RP_GPIO_22, -1, netSlot, 0);
        // removeBridgeFromNodeFile(RP_GPIO_23, -1, netSlot, 0);

        // Connect the GPIO pins to the specified rows
        addBridgeToNodeFile( RP_GPIO_26, sdaRow, netSlot, 0, 0 ); // SDA
        addBridgeToNodeFile( RP_GPIO_27, sclRow, netSlot, 0, 0 ); // SCL

        // Refresh the hardware connections
        refreshConnections( -1, 1 );
        waitCore2( ); // Wait for connections to be established
    }

    //    if(   oled.clearPrintShow("GPIO 7 = SDA", 1, 0, 0, true, true, false) ==
    //    1) {
    //     oled.clearPrintShow("GPIO 8 = SCL", 1, 0, 16, true, true, false);
    //     delay(20);
    //     }

    oled.clear( );
    oled.print( "GPIO 7 = SDA\n\r" );
    oled.print( "GPIO 8 = SCL" );
    oled.show( );
    delay( 20 );
    // Initialize I2C hardware
    Wire1.end( ); // Stop any existing I2C communication
    Wire1.setSDA( sdaPin );
    Wire1.setSCL( sclPin );
    Wire1.begin( );
    Wire1.setClock( 100000 ); // Standard 100kHz I2C speed

    // Scan all possible I2C addresses
    Serial.println( "\nScanning I2C bus..." );
    Serial.println(
        "    _0  _1  _2  _3  _4  _5  _6  _7  _8  _9  _A  _B  _C  _D  _E  _F " );

    int nDevices = 0;

    uint8_t addressesFound[ 128 ];
    int addressesFoundCount = 0;
    int addressesFoundIndicies[ 128 ];

    for ( int baseAddr = 0; baseAddr < 128; baseAddr += 16 ) {
        Serial.printf( "\n\r%02X:", baseAddr / 16 );

        for ( int addr = 0; addr < 16; addr++ ) {
            int deviceAddr = baseAddr + addr;
            if ( deviceAddr > 0 &&
                 deviceAddr < 127 ) { // Skip address 0 (general call) and 127
                Wire1.beginTransmission( deviceAddr );
                byte error = Wire1.endTransmission( );

                if ( error == 0 ) {
                    Serial.printf( " %X ", deviceAddr ); // Device found
                    nDevices++;
                    addressesFound[ addressesFoundCount ] = deviceAddr;
                    addressesFoundIndicies[ addressesFoundCount ] = baseAddr + addr;
                    addressesFoundCount++;
                } else {
                    Serial.print( " -- " ); // No device
                }
            } else {
                Serial.print( " -- " ); // Skip reserved addresses
            }
        }
    }
    // delay(100);

    // Wire1.end();
    // Wire1.begin();

    // for (int i = 0; i < addressesFoundCount; i++) {
    //     char hexStr[5];
    //     sprintf(hexStr, "%02X", addressesFound[i]);
    //     Serial.printf("\n\n\r0x%s at %d\n", hexStr, addressesFoundIndicies[i]);
    //     }

    // Print summary
    Serial.println( "\n\nI2C Scan Results:" );
    if ( nDevices == 0 ) {
        Serial.println( "No I2C devices found" );
        showLEDsCore2 = -3;
        b.clear( );

        b.print( "No I2C  Found", (uint32_t)0x070003 );
        delayWithButton( 2000 );
        b.clear( );
        showLEDsCore2 = -1;
    } else {
        Serial.printf( "Found %d I2C device(s)\n", nDevices );

        // b.print("0x", (uint32_t)0x000904 );
        showLEDsCore2 = -3;
        b.clear( );
        showLEDsCore2 = -3;
        if ( addressesFoundCount == 1 ) {
            b.print( "Found", (uint32_t)0x000b01, (uint32_t)0x000000, 0, 0, 3 );
            b.print( addressToHexString( addressesFound[ 0 ] ), (uint32_t)0x000a05,
                     (uint32_t)0x000000, 1, 1 );
            // if (oledConnected == true) {

        } else if ( addressesFoundCount > 1 ) {
            b.print( addressToHexString( addressesFound[ 0 ] ), (uint32_t)0x000a05,
                     (uint32_t)0x000000, 1, 1 );
            b.print( addressToHexString( addressesFound[ 1 ] ), (uint32_t)0x000808,
                     (uint32_t)0x000000, 1, 1 );
        }
        // showLEDsCore2 = 3;
        // delay(2000);
        delayWithButton( 2000 );
        showLEDsCore2 = -1;
    }

    // Clean up connections
    if ( leaveConnections == 0 && sdaRow != -1 && sclRow != -1 ) {
        removeBridgeFromNodeFile( RP_GPIO_26, sdaRow, netSlot, 0 );
        removeBridgeFromNodeFile( RP_GPIO_27, sclRow, netSlot, 0 );
        refreshConnections( -1, 1 );
    }

    Wire1.end( );
    Wire1.begin( );
    if ( oled.oledConnected == true ) {
        delay( 500 );
        oled.clear( );
        oled.print( "Found = " );
        oled.setTextSize( 1 );
        oled.print( addressToHexString( addressesFound[ 0 ] ) );
        oled.show( );
    }

    return nDevices;
}

void calibrateDacs( void ) {







    if ( firstStart == 1 ) {
        Serial.println( "\n\rFirst startup calibration\n\n\r" );

        // Ensure routing system is properly initialized after filesystem wipe
        Serial.println( "Initializing routing system for first startup..." );
        Serial.flush();
        initChipStatus( ); // Initialize chip mappings based on hardware revision
        clearAllNTCC( );   // Clear and reinitialize routing state
        delay( 100 );      // Give system time to stabilize
        Serial.println( "Routing system initialized." );
    } else {
        // Serial.println("Calibration");
        clearAllNTCC( );
    }
    // delay(3000);
    float setVoltage = 0.0;

    uint32_t dacColors[ 4 ] = { 0x150003, 0x101000, 0x001204, 0x000512 };
    // sendAllPathsCore2 = 1;
    INA0.setBusADC( 0x0e );
    INA1.setBusADC( 0x0e );
    int lastNetSlot = netSlot;
    netSlot = 8;

    int failedToConverge = 0;

    createSlots( 8, 1 );
    // for (int i = 0; i < 4; i++) {

    // Serial.print("netSlot: ");
    // Serial.println(netSlot);
    // printPathsCompact();
    // printChipStatus();
    b.print( "Calib", 0x001010, 0x000000, 0, -1, -2 );
    Serial.println( "\n\r\t\tCalibrating\n\r" );

    Serial.println( "This tweaks the zero and full scale values for the DACs when "
                    "converting floats to a 12-bit value for the DAC\n\r" );
    Serial.println( "Were using the INA219 to measure the DAC output, which can "
                    "only measure positive voltages\n\r" );
    Serial.println(
        "The DAC outputs 0 to 4.096 V, which is scaled and shifted to +-8V\n\r" );
    Serial.println(
        "This is the float voltage to 12-bit DAC formula:\n\n\r\tint dacValue = "
        "(voltage * 4095 / dacSpread[n]) + dacZero[n];\n\n\r" );

    Serial.println( "**UNPLUG ANYTHING CONNECTED TO THE RAILS**\n\n\r" );

    Serial.flush();
    int skipZeroing = 0;
    if ( skipZeroing == 0 ) {
        for ( int d = 0; d < 4; d++ ) {

            //     removeBridgeFromNodeFile(ISENSE_PLUS, -1, netSlot);
            // removeBridgeFromNodeFile(ADC0+d, -1, netSlot);
            b.clear( );

            b.print( "Zero  ", 0x001010, 0x000000, 0, -1, -1 );

            for ( int j = d; j >= 0; j-- ) {
                // b.print(".", dacColors[j], 0xfffffe, 5, 0, (j*2)-2);
                if ( j != d ) {
                    b.printRawRow( 0b00010001, 22 + ( j * 2 ), dacColors[ j ], 0xfffffe );
                }

                b.printRawRow( 0b00000001, 22 + ( j * 2 ), dacColors[ j ], 0xfffffe );
            }

            b.print( "DAC ", dacColors[ d ], 0x000000, 0, 1, -1 );

            b.print( d, dacColors[ d ], 5, 1, -1 );
            refreshPaths( );
            clearAllNTCC( );
            createSlots( netSlot, 1 );
            refreshConnections( 0, 0, 1 );

            if ( firstStart == 1 ) {
                delay( 1 );
            } else {
                delay( 8 );
            }
            // delay(10);

            switch ( d ) {
            case 0:

                addBridgeToNodeFile( DAC0, ISENSE_PLUS, netSlot );
                // addBridgeToNodeFile(DAC0, ROUTABLE_BUFFER_IN, netSlot);
                addBridgeToNodeFile( DAC0, ADC0, netSlot );
                Serial.println( "\n\n\r\tDAC 0" );
                break;
            case 1:

                addBridgeToNodeFile( DAC1, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( DAC1, ADC1, netSlot );
                Serial.println( "\n\n\r\tDAC 1" );
                break;
            case 2:

                addBridgeToNodeFile( TOP_RAIL, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( TOP_RAIL, ADC2, netSlot );
                Serial.println( "\n\n\r\tTop Rail" );
                break;
            case 3:

                addBridgeToNodeFile( BOTTOM_RAIL, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( BOTTOM_RAIL, ADC3, netSlot );
                Serial.println( "\n\n\r\tBottom Rail" );
                break;
            }

            refreshConnections( 0, 0, 1 );
            if ( firstStart == 1 ) {
                delay( 20 );
            } else {
                delay( 18 );
            }
            printPathsCompact( );

           
            // Serial.print("\n\n\r\tDAC ");
            // Serial.println(d);

            Serial.println( "\n\r\t\tzeroing DAC" );

            int zeroFound = 0;
            float zeroTolerance = 2.1;

            int counter = 0;
            dacZero[ d ] = dacZero[ d ] + 5;

            while ( zeroFound < 2 && counter < 80 ) {
                setVoltage = 0.0;
                setDacByNumber( d, setVoltage, 0 );
                if ( firstStart == 1 ) {
                    delay( 32 );
                } else {
                    delay( 38 );
                }
                float reading = INA0.getBusVoltage_mV( );

                while ( INA0.getConversionFlag( ) == 0 ) {

                    // Serial.print(".");
                    delayMicroseconds( 400 );
                }

                reading = INA0.getBusVoltage_mV( );

                // delay(20);
                //  for (int i = 0; i < 1; i++) {
                //    delay(10);
                //    //INA0.getConversionFlag();
                //    float sample = INA0.getBusVoltage_mV();
                //    reading += sample;
                //    Serial.println(sample);

                // }
                //  reading = reading / 1;

                if ( reading < zeroTolerance && reading > -zeroTolerance ) {
                    zeroFound++;
                } else if ( reading < 2.0 ) {
                    dacZero[ d ] = dacZero[ d ] + 1;
                } else if ( reading > 2.0 ) {
                    dacZero[ d ] = dacZero[ d ] - 1;
                }

                // if (reading < 20.0 && reading > -20.0) // prevent the loop from
                // running
                //                                        // forever if it never finds
                //                                        zero
                // {
                counter++;
                //}

                if ( counter > 80 ) {
                    zeroFound++;
                    failedToConverge++;
                }

                Serial.print( "dacZero: " );
                Serial.print( dacZero[ d ] );

                Serial.print( "\t\tmeasured: " );
                Serial.print( reading );
                Serial.println( " mV" );
                // zeroFound = 1;
            }

            int spreadFound = 0;
            float tolerance = 4.1; // mV
            int giveUp = 0;
            Serial.println( "\n\n\rfinding spread\n\r" );
            b.print( "Spred", 0x080010, 0x000000, 0, -1, -1 );

            for ( int j = d; j >= 0; j-- ) {
                // b.print(".", dacColors[j], 0xfffffe, 5, 0, (j*2)-2);

                b.printRawRow( 0b00010001, 22 + ( j * 2 ), dacColors[ j ], 0xfffffe );
            }

            while ( spreadFound < 2 && giveUp < 40 ) {

                setVoltage = 5.0;
                giveUp++;
                float setMillivoltage = setVoltage * 1000;

                if ( dacSpread[ d ] < 18.0 || dacSpread[ d ] > 25.0 ||
                     dacSpread[ d ] != dacSpread[ d ] ) {
                    dacSpread[ d ] = 20.1;
                }
                setDacByNumber( d, setVoltage, 0 );
                if ( firstStart == 1 ) {
                    delay( 1 );
                } else {
                    delay( 8 );
                }
                // delay(20 * (spreadFound + 1));

                float reading = INA0.getBusVoltage_mV( );
                while ( INA0.getConversionFlag( ) == 0 ) {
                    delayMicroseconds( 100 );
                }

                reading = INA0.getBusVoltage_mV( );
                Serial.print( "Set: " );
                Serial.print( setVoltage );
                Serial.print( " V\t" );
                Serial.print( "dacSpread: " );
                Serial.print( dacSpread[ d ], 3 );
                Serial.print( " V\tmeasured: " );
                Serial.print( reading, 2 );
                Serial.println( " mV" );

                if ( reading <= ( setMillivoltage + tolerance ) &&
                     reading >= ( setMillivoltage - tolerance ) ) {
                    spreadFound++;
                } else if ( reading <= setMillivoltage - 14.5 ) {
                    dacSpread[ d ] = dacSpread[ d ] - 0.1;
                    // dacSpread[d] = dacSpread[d] - (abs((reading / 1000) - setVoltage));
                } else if ( reading >= setMillivoltage + 14.5 ) {
                    dacSpread[ d ] = dacSpread[ d ] + 0.1;
                    // dacSpread[d] = dacSpread[d] + (abs((reading / 1000) - setVoltage));
                } else if ( reading <= setMillivoltage - 4.5 ) {
                    dacSpread[ d ] = dacSpread[ d ] - 0.03;
                    // dacSpread[d] = dacSpread[d] - (abs((reading / 1000) - setVoltage));
                } else if ( reading >= setMillivoltage + 4.5 ) {
                    dacSpread[ d ] = dacSpread[ d ] + 0.03;
                    // dacSpread[d] = dacSpread[d] + (abs((reading / 1000) - setVoltage));
                }
            }

            if (giveUp >= 40) failedToConverge++;
        }

        // ADC calibration - use DAC 1 to calibrate all ADCs
        Serial.println( "\n\n\rCalibrating ADCs against INA readings using DAC 1\n\r" );

        int adcChannels[] = { 0, 1, 2, 3, 4, 7 }; // ADC channels to calibrate
        int numAdcChannels = 6;

        for ( int adcIdx = 0; adcIdx < numAdcChannels; adcIdx++ ) {
            int d = adcChannels[ adcIdx ];
            b.clear( );
            b.print( "calib", 0x100010, 0x000000, 0, 0, -1 );
            // b.print("ADC", dacColors[d], 0x000000, 3, -1, -2);
            char adcName[ 10 ] = "";

            switch ( d ) {
            case 0:
                strcpy( adcName, "ADC 0" );
                break;
            case 1:
                strcpy( adcName, "ADC 1" );

                break;
            case 2:
                strcpy( adcName, "ADC 2" );
                break;
            case 3:
                strcpy( adcName, "ADC 3" );
                break;
            case 4:
                strcpy( adcName, "ADC 4" );
                break;
            case 7:
                strcpy( adcName, "Probe" );
                break;
            }

            b.print( adcName, dacColors[ d % 4 ], 0x000000, 0, 1, 3 );

            // Use DAC 1 to calibrate all ADCs (it's working properly)
            clearAllNTCC( );
            createSlots( netSlot, 1 );
            // refreshConnections(0, 0, 1);

            // Always use DAC 1 as the voltage source
            addBridgeToNodeFile( DAC1, ISENSE_PLUS, netSlot );

            // Connect to the appropriate ADC
            switch ( d ) {
            case 0:
                addBridgeToNodeFile( DAC1, ADC0, netSlot );
                Serial.println( "\n\n\r\tADC 0 calibration (using DAC 1)" );
                break;
            case 1:
                addBridgeToNodeFile( DAC1, ADC1, netSlot );
                Serial.println( "\n\n\r\tADC 1 calibration (using DAC 1)" );
                break;
            case 2:
                addBridgeToNodeFile( DAC1, ADC2, netSlot );
                Serial.println( "\n\n\r\tADC 2 calibration (using DAC 1)" );
                break;
            case 3:
                addBridgeToNodeFile( DAC1, ADC3, netSlot );
                Serial.println( "\n\n\r\tADC 3 calibration (using DAC 1)" );
                break;
            case 4:
                addBridgeToNodeFile( DAC1, ADC4, netSlot );
                Serial.println( "\n\n\r\tADC 4 calibration (0-5V range, using DAC 1)" );
                break;
            case 7:
                addBridgeToNodeFile( DAC1, ROUTABLE_BUFFER_IN, netSlot );
                Serial.println( "\n\n\r\tADC 7 calibration (Probe tip, using DAC 1)" );
                break;
            }

            refreshConnections( 0, 1, 1 );
            delay( 250 );
            printPathsCompact( );

            // Calibrate ADC using multiple voltage points
            float voltagePoints[ 8 ];
            int numPoints;

            if ( d == 4 ) {
                // ADC 4 has 0-5V range, use points within this range
                float adc4Points[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 4.5 };
                numPoints = 6;
                for ( int i = 0; i < numPoints; i++ ) {
                    voltagePoints[ i ] = adc4Points[ i ];
                }
            } else {
                // ADCs 0-3 and 7 have ±8V range, but use 0-6V for calibration (INA can't measure negative)
                float standardPoints[] = { 0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0 };
                numPoints = 7;
                for ( int i = 0; i < numPoints; i++ ) {
                    voltagePoints[ i ] = standardPoints[ i ];
                }
            }
            float totalError = 0.0;
            float sumINA = 0.0;
            float sumADC = 0.0;
            float sumADCSquared = 0.0;
            float sumADCxINA = 0.0;
            int validPoints = 0;

            for ( int p = 0; p < numPoints; p++ ) {
                float testVoltage = voltagePoints[ p ];
                setDacByNumber( 1, testVoltage, 0 ); // Always use DAC 1
                delay( 100 );

                // Get INA reading
                float inaReading = INA0.getBusVoltage( );

                while ( INA0.getConversionFlag( ) == 0 ) {
                    delayMicroseconds( 100 );
                }
                inaReading = INA0.getBusVoltage( );

                // Get raw ADC reading for the specific channel
                int rawADC = readAdc( d, 64 );

                // Only use positive voltages for calibration (INA can't measure negative)
                if ( inaReading > 0.1 ) {
                    sumINA += inaReading;
                    sumADC += rawADC;
                    sumADCSquared += rawADC * rawADC;
                    sumADCxINA += rawADC * inaReading;
                    validPoints++;

                    Serial.print( "Set: " );
                    Serial.print( testVoltage );
                    Serial.print( "V, INA: " );
                    Serial.print( inaReading );
                    Serial.print( "V, raw ADC" );
                    Serial.print( d );
                    Serial.print( ": " );
                    Serial.println( rawADC );
                }
            }

            // Calculate calibration using linear regression
            if ( validPoints >= 2 ) {
                // Use linear regression to find: INA_voltage = slope * rawADC + intercept
                float n = validPoints;
                float slope = ( n * sumADCxINA - sumADC * sumINA ) / ( n * sumADCSquared - sumADC * sumADC );
                float intercept = ( sumINA - slope * sumADC ) / n;

                // Handle different ADC formulas
                if ( d == 4 ) {
                    // ADC 4 formula: voltage = (rawADC * adcSpread / 4095) (no offset)
                    // We want: INA_voltage = slope * rawADC + intercept
                    // So: adcSpread/4095 = slope, and intercept should be 0
                    adcSpread[ d ] = slope * 4095.0;
                    adcZero[ d ] = 0.0; // No offset for ADC 4
                } else {
                    // Other ADCs formula: voltage = (rawADC * adcSpread / 4095) - adcZero
                    // We want: INA_voltage = slope * rawADC + intercept
                    // Matching coefficients: adcSpread/4095 = slope, so adcSpread = slope * 4095
                    // And: -adcZero = intercept, so adcZero = -intercept
                    adcSpread[ d ] = slope * 4095.0;
                    adcZero[ d ] = -intercept; // This is what gets subtracted in the offset
                }

                if ( abs( slope ) > 0.0001 ) {

                    // Clamp to reasonable values based on ADC type
                    if ( d == 4 ) {
                        // ADC 4 is 0-5V range
                        if ( adcSpread[ d ] < 3.0 || adcSpread[ d ] > 8.0 ) {
                            Serial.print( "ADC4 spread out of range: " );
                            Serial.print( adcSpread[ d ] );
                            Serial.println( ", using default" );
                            adcSpread[ d ] = 5.0; // Default for 0-5V
                            adcZero[ d ] = 0.0;
                        }
                    } else {
                        // Other ADCs are ±8V range
                        if ( adcSpread[ d ] < 10.0 || adcSpread[ d ] > 30.0 ) {
                            Serial.print( "adcSpread out of range: " );
                            Serial.print( adcSpread[ d ] );
                            Serial.println( ", using default" );
                            adcSpread[ d ] = 18.28; // Default value
                            adcZero[ d ] = 8.0;
                        } else if ( abs( adcZero[ d ] ) > 50.0 ) {
                            Serial.print( "adcZero out of range: " );
                            Serial.print( adcZero[ d ] );
                            Serial.println( ", using default" );
                            adcZero[ d ] = 8.0; // Default value
                        }
                    }

                    // Print calibration results if values are reasonable
                    Serial.print( "ADC " );
                    Serial.print( d );
                    Serial.print( " calibration: spread=" );
                    Serial.print( adcSpread[ d ], 2 );
                    Serial.print( ", zero=" );
                    Serial.print( adcZero[ d ], 2 );
                    Serial.print( " (slope=" );
                    Serial.print( slope, 6 );
                    Serial.print( ", intercept=" );
                    Serial.print( intercept, 2 );
                    Serial.println( ")" );
                } else {
                    Serial.print( "ADC calibration failed - slope too small: " );
                    Serial.println( slope, 6 );
                    // Keep default values
                }
            } else {
                Serial.println( "ADC calibration failed - insufficient valid points" );
            }

            setDacByNumber( 1, 0.0, 0 ); // Reset DAC 1 to 0V
        }

        Serial.println( "\n\n\tCalibration Values\n\n\r" );
        Serial.print( "            DAC Zero\tDAC Spread\tADC Zero\tADC Spread\n\r" );
        for ( int i = 0; i < 4; i++ ) {

            switch ( i ) {
            case 0:
                Serial.print( "DAC 0       " );
                break;
            case 1:
                Serial.print( "DAC 1       " );
                break;
            case 2:
                Serial.print( "Top Rail    " );
                break;
            case 3:
                Serial.print( "Bottom Rail " );
                break;
            }
            Serial.print( dacZero[ i ] );
            // Serial.print("\tdacSpread[");
            Serial.print( "\t" );

            Serial.print( dacSpread[ i ] );

            Serial.print( "\t\t" );

            Serial.print( adcZero[ i ] );

            Serial.print( "\t\t" );

            Serial.println( adcSpread[ i ] );
        }

        Serial.println( "\n\r            ADC Zero\tADC Spread" );
        Serial.print( "ADC 4 (0-5V)   " );
        Serial.print( adcZero[ 4 ] );
        Serial.print( "\t" );
        Serial.println( adcSpread[ 4 ] );
        Serial.print( "ADC 7 (Probe)  " );
        Serial.print( adcZero[ 7 ] );
        Serial.print( "\t" );
        Serial.println( adcSpread[ 7 ] );
        delay(10);
        checkProbeCurrentZero();
        saveDacCalibration( );
    }
    setRailsAndDACs( );

    if (failedToConverge == 0) {
        changeTerminalColor(84, true); // Green
    } else {
        changeTerminalColor(196, true); // Red
    }
  

    // Print big block text for PASS or FAIL
    if (failedToConverge == 0) {
        Serial.println("\r\n");
        Serial.println("███████   █████   ███████ ███████");
        Serial.println("██    ██ ██   ██  ██      ██     ");
        Serial.println("███████  ███████  ███████ ███████");
        Serial.println("██       ██   ██       ██      ██");
        Serial.println("██       ██   ██  ███████ ███████");
        Serial.println("\r\n");
    } else {
        Serial.println("\r\n");
        Serial.println("███████  █████  ██ ██     ");
        Serial.println("██      ██   ██ ██ ██     ");
        Serial.println("█████   ███████ ██ ██     ");
        Serial.println("██      ██   ██ ██ ██     ");
        Serial.println("██      ██   ██ ██ ███████");
        Serial.println("\r\n");
    }
    Serial.println();
    Serial.print("\n\n\rFailedToConverge = ");
    Serial.println(failedToConverge);
    changeTerminalColor(-1);

    


    Serial.println( "\n\n\rrun test? (y/n)\n\n\rmake damn sure nothing is "
                    "physically connected to the rails\n\r" );

    b.clear( );
    b.print( "Test?", 0x0a0a00, 0x000000, 1, -1, -1 );
    int yesNo;
    if ( firstStart == 1 ) {
        yesNo = 1; // yesNoMenu(800);
    } else {
        yesNo = yesNoMenu( 4000 );
    }

    //   char input = ' ';
    //   unsigned long timeout = millis();

    //   while (1) {
    //     if (millis() - timeout > 15000) {
    //       break;
    //     }
    //     if (Serial.available() > 0) {
    //       input = Serial.read();
    //       break;
    //     }

    //   }

    failedToConverge = 0;

    float tolerance = 0.6;
    if ( yesNo == 1 ) {

        b.clear( );
        setDacByNumber( 0, 0.0, 0 );
        setDacByNumber( 1, 0.0, 0 );
        setDacByNumber( 2, 0.0, 0 );
        setDacByNumber( 3, 0.0, 0 );
        for ( int d = 0; d < 4; d++ ) {
            b.clear( 0 );

            //     removeBridgeFromNodeFile(ISENSE_PLUS, -1, netSlot);

            // removeBridgeFromNodeFile(ADC0+d, -1, netSlot);

            clearAllNTCC( );
            createSlots( netSlot, 1 );
            // refreshConnections(0, 0, 1);
            if ( firstStart == 1 ) {
                delay( 8 );
            } else {
                delay( 8 );
            }
            switch ( d ) {
            case 0:

                // addBridgeToNodeFile(DAC0, ISENSE_PLUS, netSlot);

                addBridgeToNodeFile( DAC0, ROUTABLE_BUFFER_IN, netSlot );
                // addBridgeToNodeFile(DAC0, ADC0, netSlot);
                Serial.println( "\n\n\r\tDAC 0 test" );
                b.print( "DAC 0", dacColors[ d ], 0x000000, 1, -1, -1 );
                break;
            case 1:
                /// removeBridgeFromNodeFile(ADC0+d, -1, netSlot);
                addBridgeToNodeFile( DAC1, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( DAC1, ADC1, netSlot );
                Serial.println( "\n\n\r\tDAC 1 test" );
                b.print( "DAC 1", dacColors[ d ], 0x000000, 1, -1, -1 );
                break;
            case 2:
                // removeBridgeFromNodeFile(ADC0+d, -1, netSlot);
                addBridgeToNodeFile( TOP_RAIL, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( TOP_RAIL, ADC2, netSlot );
                Serial.println( "\n\n\r\tTop Rail test" );
                // b.print("Top Ral", dacColors[d], 0x000000, 0, -1, -1);
                b.print( "Top", dacColors[ d ], 0x000000, 0, -1, -1 );
                b.print( "Ra", dacColors[ d ], 0xfffffe, 4, -1, -2 );
                b.print( "i", dacColors[ d ], 0xfffffe, 6, 0, -3 );
                b.print( "l", dacColors[ d ], 0xfffffe, 6, 0, -1 );
                break;
            case 3:
                // removeBridgeFromNodeFile(ADC0+d, -1, netSlot);
                addBridgeToNodeFile( BOTTOM_RAIL, ISENSE_PLUS, netSlot );
                addBridgeToNodeFile( BOTTOM_RAIL, ADC3, netSlot );
                Serial.println( "\n\n\r\tBottom Rail test" );
                b.print( "Bot", dacColors[ d ], 0x000000, 0, -1, -1 );
                b.print( "Ra", dacColors[ d ], 0xfffffe, 4, -1, -2 );
                b.print( "i", dacColors[ d ], 0xfffffe, 6, 0, -3 );
                b.print( "l", dacColors[ d ], 0xfffffe, 6, 0, -1 );
                break;
            }

            refreshConnections( 0, 1, 1 );
            // refreshBlind(1, 0);
            if ( firstStart == 1 ) {
                delay( 1 );
            } else {
                delay( 8 );
            }
            printPathsCompact( );
            Serial.println( " " );

            int nextRow = 0;

            for ( int i = -1; i <= 8; i++ ) {
                setVoltage = i * 1.0;
                setDacByNumber( d, setVoltage, 0 );
                Serial.print( "set : " );
                Serial.printf( "%*.3f", 6, setVoltage );
                Serial.print( " V\t" );
                if ( firstStart == 1 ) {
                    delay( 25 );
                } else {
                    delay( 150 );
                }
                float reading = 0.0;

                int voltage = map( i, -3, 8, 0, 4 );

                b.printRawRow( 0b00000001 << voltage, nextRow + 30 + ( d * 6 ),
                               dacColors[ d ], 0xfffffe );
                nextRow++;

                if ( firstStart == 1 ) {
                    delay( 5 );
                } else {
                    delay( 8 );
                }
                if ( d == 0 ) {
                    reading = readAdcVoltage( 7, 64 );
                } else {
                    reading = readAdcVoltage( d, 64 );
                }
                Serial.print( "\tADC measured: " );

                if (abs(reading - setVoltage) > tolerance && i <= 7) {
                    failedToConverge++;
                }
                // if (i < 0) {
                //  Serial.print(setVoltage); // + random(-4, 4) / 100.0);

                // } else if (i > 8) {
                //   Serial.print(setVoltage); // + random(-4, 4) / 100.0);
                // } else {
                Serial.printf( "%*.3f", 6, reading );
                //}
                Serial.print( " V" );
                if ( firstStart == 1 ) {
                    delay( 3 );
                } else {
                    delay( 8 );
                }

                if ( d == 0 ) {
                    reading = INA1.getBusVoltage( );

                    while ( INA1.getConversionFlag( ) == 0 ) {
                        // Serial.print(".");
                        // delay(1);
                        delayMicroseconds( 100 );
                    }

                    reading = INA1.getBusVoltage( );

                    if (abs(reading - setVoltage) > tolerance && i  >= 0) {
                        failedToConverge++;
                    }

                } else {

                    reading = INA0.getBusVoltage( );

                    while ( INA0.getConversionFlag( ) == 0 ) {
                        // Serial.print(".");
                        // delay(1);
                        delayMicroseconds( 100 );
                    }

                    reading = INA0.getBusVoltage( );

                    if (abs(reading - setVoltage) > tolerance && i >= 0) {
                        failedToConverge++;
                    }
                }

                Serial.print( "\t     INA measured: " );
                Serial.printf( "%*.3f", 6, reading );
                Serial.println( " V" );

                // dacCalibration[0][i] = reading;
            }
            setDacByNumber( d, 0.0, 0 );
            // setDacByNumber(d, d < 2 ? dacOutput[d] : railVoltage[d - 2], 0);
        }
    }
    unsigned long timeout = millis( );
    Serial.println( "\n\r" );
    while ( 1 ) {
        if ( millis( ) - timeout > 1000 ) {
            break;
        }
        if ( Serial.available( ) > 0 ) {
            break;
        }
        if ( encoderButtonState == PRESSED ) {
            encoderButtonState = IDLE;
            break;
        }
    }
    // delay(5000);
    INA0.setBusADC( 0x0b );
    INA1.setBusADC( 0x0b );
    // removeBridgeFromNodeFile(ISENSE_PLUS, -1, netSlot);
    createSlots( netSlot, 1 );
    clearAllNTCC( );
    netSlot = lastNetSlot;
    if (failedToConverge == 0) {
       
    } else {
        
    }
  

    // Print big block text for PASS or FAIL
    if (failedToConverge < 7) {
        changeTerminalColor(84, true); // Green
        Serial.println("\r\n");
        Serial.println("███████   █████   ███████ ███████");
        Serial.println("██    ██ ██   ██  ██      ██     ");
        Serial.println("███████  ███████  ███████ ███████");
        Serial.println("██       ██   ██       ██      ██");
        Serial.println("██       ██   ██  ███████ ███████");
        Serial.println("\r\n");
    } else {
        changeTerminalColor(196, true); // Red
        Serial.println("\r\n");
        Serial.println("███████  █████  ██ ██     ");
        Serial.println("██      ██   ██ ██ ██     ");
        Serial.println("█████   ███████ ██ ██     ");
        Serial.println("██      ██   ██ ██ ██     ");
        Serial.println("██      ██   ██ ██ ███████");
        Serial.println("\r\n");
    }
    //Serial.println();
    Serial.print("\n\n\rFailedToConverge = ");
    Serial.println(failedToConverge);
    Serial.println();
    Serial.println();
    changeTerminalColor(-1, true);

    

    refreshConnections( -1 );
    routableBufferPower( 1, 0, 1 );
    // showProbeLEDs = 1;
    refreshConnections( -1 );
    configChanged = true;
    if ( firstStart == 1 ) {
        initializeMicroPythonExamples( true );

        delay(1300);
        rp2040.restart();

    }


    // printPathsCompact();
}

void printSerial1stuff( void ) {

    Serial.println( "Printing Serial1 stuff" );
    Serial.flush( );

    // USBSer1.begin(115200);

    oled.clear( );

    while ( 1 ) {
        if ( Serial.available( ) > 0 ) {
            char c = Serial.read( );
            USBSer1.write( c );
            // USBSer1.print(c);
            USBSer1.flush( );

            oled.print( c );
            oled.show( );

            Serial.print( "sent: " );
            Serial.println( c );
            Serial.flush( );
            if ( c == 'x' ) {
                break;
            }
        }
        if ( USBSer1.available( ) > 0 ) {
            char c = USBSer1.read( );
            Serial.print( "received: " );
            Serial.println( c );
            Serial.flush( );
        }
    }
}

// void writeImage(PNGDRAW *pDraw) { return; }

void displayImage( void ) { return; }

const char* addressToHexString( uint8_t address ) {
    static char hexStr[ 6 ]; // static so it persists after function returns
    sprintf( hexStr, "0x%02X", address );
    return hexStr;
}



#include <JDI_MIP_Display.h>
#include <Display_cfg.h>

#include "Images.h"


// // JDI MIP (LPM009M360A 72x144) pin/config
// #define PIN_SCS         21
// #define PIN_DISP        20
// #define PIN_FRONTLIGHT  25

#define DISPLAY_WIDTH   72
#define DISPLAY_HEIGHT  144

#define DIFF_LINE_UPDATE
#define HALF_WIDTH (DISPLAY_WIDTH / 2)

#define COLOR_BLACK             0x00
#define COLOR_BLUE              0x02
#define COLOR_GREEN             0x04
#define COLOR_CYAN              0x06
#define COLOR_RED               0x08
#define COLOR_MAGENTA           0x0a
#define COLOR_YELLOW            0x0c
#define COLOR_WHITE             0x0e

#define CMD_NO_UPDATE           0x00
#define CMD_BLINKING_BLACK      0x10
#define CMD_BLINKING_INVERSION  0x14
#define CMD_BLINKING_WHITE      0x18
#define CMD_ALL_CLEAR           0x20
#define CMD_VCOM                0x40
#define CMD_UPDATE              0x90



// NOTE: moved to include/Display_cfg.h; do not duplicate here
 JDI_MIP_Display jdi_display;

void jdiMIPdisplay( void ) {

   


    





jdi_display.begin();
jdi_display.displayOn();
jdi_display.clearScreen();
jdi_display.refresh();

cycleTerminalColor(true, 5.3, true);
int colors[10] = {COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE, COLOR_BLACK, COLOR_BLUE, COLOR_GREEN};
Serial.println("\n\rThis is meant to run with a JDI LPM009M360A 72x144 display");
Serial.println("It will bounce the startup animation and then exit\n\r");
cycleTerminalColor();
Serial.println("Connections: \n\r");

Serial.println("  ┃  8  7  6  5  4  3  2  1  ┃  ");
Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
Serial.println("  ┃  ▌     ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
Serial.println("  ┃  ▌  ╳  ▌  ▌  ▌  ▌  ▌  ▌  ┃  ");
Serial.println("  ┗━━━━━━━━━━━━━━━━━━━━━━━━━━┛  ");cycleTerminalColor();
Serial.println("     G  N  3  D  E  C  M  C     ");
Serial.println("     N  C  V  i  x  S  O  L     ");
Serial.println("     D     3  s  t  .  S  K     ");
Serial.println("     ⏚     +  p     .  I  .     ");
Serial.println("              .     .  .  .     ");cycleTerminalColor();
Serial.println("    GPIO      6     2  4  3     ");

Serial.println();

cycleTerminalColor();
Serial.println("GPIO 2 - CS");
Serial.println("GPIO 3 - SCK");
Serial.println("GPIO 4 - MOSI");
Serial.println("GPIO 6 - DISP");

cycleTerminalColor();
Serial.println("\n\rPress the encoder button or any key to exit\n\n\r");
cycleTerminalColor();

Serial.println("Paste this in if you have the display soldered to the adapter board's SMD footptint");
Serial.println("f { 139-106,136-79,101-80,82-100,132-77,134-76,133-75, } ");
cycleTerminalColor();


Serial.flush();



// for (int i = 0; i < 10; i++){
// jdi_display.fillCircle(jdi_display.width() / 2, jdi_display.height() / 2, 30, colors[i %10]);
// jdi_display.refresh();
// delay(20);
// }

    const int srcW = 32;
    const int srcH = 21;
    const int rotW = srcH;  // 90° rotation width
    const int rotH = srcW;  // 90° rotation height

    for (int outer = 0; outer < 100; outer++){
        for (int fi = 0; fi < startupFrameLEN; fi++){
            const uint32_t* src = startupFrameArray[fi];

            // Render full-screen, rotated 90° CW, scaled to 72x144
            for (int dy = 0; dy < DISPLAY_HEIGHT; dy++){
                int uy = (dy * rotH) / DISPLAY_HEIGHT; // 0..rotH-1
                for (int dx = 0; dx < DISPLAY_WIDTH; dx++){
                    int ux = (dx * rotW) / DISPLAY_WIDTH; // 0..rotW-1

                    // Rotate 90° CW: (sx, sy) derived from (ux, uy)
                    int sx = srcW - 1 - uy;
                    int sy = ux;

                    uint32_t c = src[ sy * srcW + sx ]; // 0x00RRGGBB
                    uint8_t r = ( c >> 16 ) & 0xFF;
                    uint8_t g = ( c >> 8 ) & 0xFF;
                    uint8_t b = ( c ) & 0xFF;

                    // Map to JDI 8-color palette nibble
                    uint16_t col = 0;
                    if ( r >= 0x50 ) col |= COLOR_RED;
                    if ( g >= 0x80 ) col |= COLOR_GREEN;
                    if ( b >= 0x60 ) col |= COLOR_BLUE;

                    jdi_display.drawBufferedPixel( DISPLAY_WIDTH - 1 - dx, DISPLAY_HEIGHT - 1 - dy, col );
                }
            }

            jdi_display.refresh();
        }
        for (int fi = startupFrameLEN-1; fi >= 0; fi--){
            const uint32_t* src = startupFrameArray[fi];

            // Render full-screen, rotated 90° CW, scaled to 72x144
            for (int dy = 0; dy < DISPLAY_HEIGHT; dy++){
                int uy = (dy * rotH) / DISPLAY_HEIGHT; // 0..rotH-1
                for (int dx = 0; dx < DISPLAY_WIDTH; dx++){
                    int ux = (dx * rotW) / DISPLAY_WIDTH; // 0..rotW-1

                    // Rotate 90° CW: (sx, sy) derived from (ux, uy)
                    int sx = srcW - 1 - uy;
                    int sy = ux;

                    uint32_t c = src[ sy * srcW + sx ]; // 0x00RRGGBB
                    uint8_t r = ( c >> 16 ) & 0xFF;
                    uint8_t g = ( c >> 8 ) & 0xFF;
                    uint8_t b = ( c ) & 0xFF;

                    // Map to JDI 8-color palette nibble
                    uint16_t col = 0;
                    if ( r >= 0x50 ) col |= COLOR_RED;
                    if ( g >= 0x80 ) col |= COLOR_GREEN;
                    if ( b >= 0x50 ) col |= COLOR_BLUE;

                    jdi_display.drawBufferedPixel( DISPLAY_WIDTH - 1 - dx, DISPLAY_HEIGHT - 1 - dy, col );
                }
            }

            jdi_display.refresh();
        }

        if (encoderButtonState == PRESSED || Serial.available() > 0){
            break;
        }
        delay(10);
    }
    return;

    // // Convert a small startup image to RGB111 (8 colors) and render centered


    // jdi_display.frontlightOff();
  //  jdi_display.displayOff();
//#else
   // Serial.println("\n\rJDI_MIP_Display library not found at compile time.");
   // Serial.println("Install 'JDI_MIP_Display' via Library Manager or platformio.ini lib_deps.");
//#endif
}
