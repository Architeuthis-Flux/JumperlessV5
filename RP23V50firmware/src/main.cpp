#include "TuiGlue.h"
#include "hardware/pio.h"
#define PICO_RP2350A 0

#include <Arduino.h>

#ifdef USE_TINYUSB
    #include "tusb.h" // For tud_task() function
    #include <Adafruit_TinyUSB.h>
#endif

#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Wire.h>
#include "ArduinoStuff.h"
#include "CH446Q.h"
#include "Commands.h"

#include "Apps.h"
#include "ArduinoStuff.h"
#include "Debugs.h"
#include "FileParsing.h"
#include "FilesystemStuff.h"
#include "Graphics.h"
#include "HelpDocs.h"
#include "Highlighting.h"
#include "JulseView.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "LogicAnalyzer.h"
#include "MatrixState.h"
#include "Menus.h"
#include "NetManager.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "PersistentStuff.h"
#include "Probing.h"
#include "Python_Proper.h"
#include "RotaryEncoder.h"
#include "USBfs.h"
#include "configManager.h"
#include "oled.h"
#include <hardware/adc.h>
#include "AsyncPassthrough.h"
#include "user_functions.h"



bread b;
unsigned long startupTimers[ 10 ];


volatile int dumpLED = 0;
unsigned long dumpLEDTimer = 0;
unsigned long dumpLEDrate = 50;

const char firmwareVersion[] = "5.3.1.4";
bool newConfigOptions = false;            //! set to true with new config options //!

volatile bool core2initFinished = false;
volatile bool configLoaded = false;
volatile int startupAnimationFinished = 0;


unsigned long startupCore2timers[ 10 ];



void setupCore2stuff( ) {
    // delay(2000);
    startupCore2timers[ 0 ] = millis( );
    initCH446Q( );
    startupCore2timers[ 1 ] = millis( );
    // delay(1);

    while ( configLoaded == 0 ) {
        delayMicroseconds( 1 );
    }

    initLEDs( );
    startupCore2timers[ 2 ] = millis( );
    initRowAnimations( );
    startupCore2timers[ 3 ] = millis( );
    setupSwirlColors( );
    startupCore2timers[ 4 ] = millis( );

    startupCore2timers[ 5 ] = millis( );
    initRotaryEncoder( );
    startupCore2timers[ 6 ] = millis( );
    initSecondSerial( );

}


void setup1( ) {
 
    setupCore2stuff( );
    core2initFinished = 1;

    while ( startupAnimationFinished == 0 ) {
        // delayMicroseconds(1);
        // if (Serial.available() > 0) {
        //   char c = Serial.read();
        //  // Serial.print(c);
        //   //Serial.flush();
        //   }
    }

    startupCore2timers[ 7 ] = millis( );
}

void setup() { 
    Serial.begin(115200); 

    pinMode( RESETPIN, OUTPUT_12MA );
    digitalWrite( RESETPIN, HIGH );

    // FatFS.begin();
    if ( !FatFS.begin( ) ) {
        Serial.println( "Failed to initialize FatFS" );
    } else {
        Serial.println( "FatFS initialized successfully" );
    }

    startupTimers[ 0 ] = millis( );

    loadConfig( );
    configLoaded = 1;
    startupTimers[ 1 ] = millis( );
    delayMicroseconds( 200 );

    initNets( );
    backpowered = 0;
    if ( jumperlessConfig.serial_1.function >= 5 &&
         jumperlessConfig.serial_1.function <= 6 ) {
        dumpLED = 1;
    }
    
    if ( jumperlessConfig.serial_2.function >= 5 &&
         jumperlessConfig.serial_2.function <= 6 ) {
        dumpLED = 1;
    }

    if ( jumperlessConfig.serial_1.function == 4 ||
         jumperlessConfig.serial_1.function == 6 ) {
        jumperlessConfig.top_oled.show_in_terminal = 2;
    }
    if ( jumperlessConfig.serial_2.function == 4 ||
         jumperlessConfig.serial_2.function == 6 ) {
        jumperlessConfig.top_oled.show_in_terminal = 3;
    }

    initDAC( );
    pinMode( PROBE_PIN, OUTPUT_8MA );
    pinMode( BUTTON_PIN, INPUT_PULLDOWN );
    digitalWrite( PROBE_PIN, HIGH );

    routableBufferPower( 1, 1 );
    startupTimers[ 2 ] = millis( );
    initINA219( );
    startupTimers[ 3 ] = millis( );

    delayMicroseconds( 100 );
    digitalWrite( RESETPIN, LOW );

    while ( core2initFinished == 0 ) {
        Serial.println("Inside this loop");
    }

     if (jumperlessConfig.serial_1.async_passthrough == true) {
        AsyncPassthrough::begin(115200);
    }

    drawAnimatedImage( 0 );
    
    startupAnimationFinished = 1;
    startupTimers[ 4 ] = millis( );
    clearAllNTCC( );

    startupTimers[ 5 ] = millis( );

    delayMicroseconds( 100 );
    initArduino( );

    initMenu( );
    startupTimers[ 6 ] = millis( );
    initADC( );
    startupTimers[ 7 ] = millis( );


    getNothingTouched( );

    checkProbeCurrentZero();
    startupTimers[ 8 ] = millis( );
    createSlots( -1, 0 );
    initializeNetColorTracking( );   // Initialize net color tracking after slots are
                                     // created
    initializeValidationTracking( ); // Initialize validation tracking
    startupTimers[ 9 ] = millis( );

    TuiGlue::init(); 
}



void loop() { 
    
    TuiGlue::loop(); 
}



void loop1() {


    // You probably want to put some stuff here
}

