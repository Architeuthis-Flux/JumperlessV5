// SPDX-License-Identifier: MIT

/*
Kevin Santo Cappuccio
Architeuthis Flux

KevinC@ppucc.io

5/28/2024

*/

#include "hardware/pio.h"
#define PICO_RP2350A 0
// #include <pico/stdlib.h>
#include <Arduino.h>
#include "user_functions.h"

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

bread b;

int supplySwitchPosition = 0;
volatile bool core1busy = false;
volatile bool core2busy = false;

// void lastNetConfirm(int forceLastNet = 0);
void rotaryEncoderStuff( void );
void initRotaryEncoder( void );
void printDirectoryContents( const char* dirname, int level );

void core2stuff( void );

volatile uint8_t pauseCore2 = 0;

volatile int loadingFile = 0;

unsigned long lastNetConfirmTimer = 0;
// int machineMode = 0;

// https://wokwi.com/projects/367384677537829889

volatile bool core2initFinished = false;

volatile bool configLoaded = false;

volatile int startupAnimationFinished = 0;

unsigned long startupTimers[ 10 ];

volatile int dumpLED = 0;
unsigned long dumpLEDTimer = 0;
unsigned long dumpLEDrate = 50;

const char firmwareVersion[] = "5.3.1.4"; //! remember to update this
bool newConfigOptions = false;            //! set to true with new config options //!
                                          

// julseview julseview;
LogicAnalyzer logicAnalyzer;

void setup( ) {
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

    // delay(1000);

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


    Serial.begin( 115200 );

    initDAC( );
    pinMode( PROBE_PIN, OUTPUT_8MA );
    pinMode( BUTTON_PIN, INPUT_PULLDOWN );
    // pinMode(buttonPin, INPUT_PULLDOWN);
    digitalWrite( PROBE_PIN, HIGH );

    routableBufferPower( 1, 1 );
    // digitalWrite(BUTTON_PIN, HIGH);

    startupTimers[ 2 ] = millis( );
    initINA219( );

    startupTimers[ 3 ] = millis( );

    delayMicroseconds( 100 );

    digitalWrite( RESETPIN, LOW );

    while ( core2initFinished == 0 ) {
        // delayMicroseconds(1);
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

    // delay(100);
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

    //  setupLogicAnalyzer();
}

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
    // delay(4);
}

void setup1( ) {
    // flash_safe_execute_core_init();

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

char connectFromArduino = '\0';

int input = '\0';

int serSource = 0;
int readInNodesArduino = 0;

int firstLoop = 1;

volatile int probeActive = 0;

int showExtraMenu = 0;

int lastHighlightedNet = -1;
int lastBrightenedNet = -1;
int lastWarningNet = -1;

int dontShowMenu = 0;

unsigned long timer = 0;
int lastProbeButton = 0;
unsigned long waitTimer = 0;
unsigned long switchTimer = 0;

int attract = 0;

unsigned long switchPositionCheckTimer = 0;


unsigned long mscModeRefreshTimer = 0;
unsigned long mscModeRefreshInterval = 2000;

volatile int core1passthrough = 1;
int switchPosCount = 0;

int shownMenuItems = 0;
int menuItemCount[ 4 ] = { 0, 0, 0, 0 };
int menuItemCounts[ 4 ] = { 14, 22, 37, 46 };

#include <pico/stdlib.h>

#include <hardware/gpio.h>

unsigned long core1Timeout = millis( );

#define SETUP_LOGIC_ANALYZER_ON_BOOT 0


String readLine(uint32_t timeoutMs) {
  String s;
  uint32_t start = millis();

  Serial.flush();

  while (millis() - start < timeoutMs) {
    if (Serial.available() == 0) {
      delay(5);
      continue;
    }

    char c = Serial.read();
    if (c < 0) continue;
    
    // Handle Enter (CR, LF, or CRLF)
    if (c == '\r' || c == '\n') {
      if (c == '\r' && Serial.peek() == '\n') {
        Serial.read();
      }
      break;
    }

    // Normal character
    s += c;
  }

  Serial.flush();
  return s;
}


static void prinFunctionHelp() {

    // Otherwise, list all functions
    Serial.println("Available user functions:");

    for (size_t i = 0; i < USER_FUNCTIONS_COUNT; ++i) {
        const auto& c = USER_FUNCTIONS[i];
        // left-pad names to a fixed width for readability
        const int width = 14;
        int n = strlen(c.name);
        Serial.print("  ");
        Serial.print(c.name);
        if (n < width) for (int k = 0; k < width - n; ++k) Serial.print(' ');
        Serial.print(" - ");
        if (c.help && c.help[0]) 
            Serial.println(c.help);
        else                     
            Serial.println("(no help provided)");
    }

    Serial.println("\nPlease enter your custom function (Press Enter Twice!).");
    Serial.flush();
}

void handleUserFunction() {

    prinFunctionHelp();
   
  
    String userFunc = readLine(USER_FUNCTION_WAIT);
    Serial.print("Function Call: ");
    Serial.println(userFunc);
    Serial.flush();


    if (!uf_dispatch(userFunc)) {

        Serial.println("User Function Not Found!:\n\n");
        Serial.println("User Functions Defined:\n");
        for (size_t i = 0; i < USER_FUNCTIONS_COUNT; ++i) {
                Serial.print("  ");
                Serial.print(USER_FUNCTIONS[i].name);
                if (USER_FUNCTIONS[i].help && USER_FUNCTIONS[i].help[0]) {
                Serial.print(" - ");
                Serial.print(USER_FUNCTIONS[i].help);
            }
            Serial.println();
            Serial.flush();
        }

    }

}

// ---- Tokenize a command line into argc/argv (handles quotes) ----
// Supports: words separated by spaces; quotes "like this"; escaped quotes \" inside quotes.
int tokenize(const String& line, String argv[], int maxTokens) {
  int argc = 0;
  bool inQuotes = false;
  char quoteChar = 0;
  String cur;

  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];

    if (inQuotes) {
      if (c == '\\' && i + 1 < line.length()) { // allow \" and \\ inside quotes
        char n = line[i+1];
        if (n == '"' || n == '\'' || n == '\\') { cur += n; i++; continue; }
      }
      if (c == quoteChar) { inQuotes = false; continue; }
      cur += c;
    } else {
      if (c == '"' || c == '\'') { inQuotes = true; quoteChar = c; continue; }
      if (c == ' ' || c == '\t') {
        if (cur.length()) {
          if (argc < maxTokens) argv[argc++] = cur;
          cur = "";
        }
      } else {
        cur += c;
      }
    }
  }
  if (cur.length() && argc < maxTokens) argv[argc++] = cur;
  return argc;
}

// ---- Parse options: --key value, --key=value, -k value, -abc (bool flags) ----
struct Opt {
  String key;    // without leading dashes
  String value;  // empty => boolean flag
};



int parseOptions(int argc, String argv[], Opt opts[], int maxOpts) {
  int nopt = 0;
  for (int i = 1; i < argc; ++i) {
    String t = argv[i];
    if (t.startsWith("--")) {
      String keyval = t.substring(2);
      int eq = keyval.indexOf('=');
      if (eq >= 0) {
        if (nopt < maxOpts) { opts[nopt++] = { keyval.substring(0, eq), keyval.substring(eq+1) }; }
      } else {
        String val = "";
        if (i + 1 < argc && !argv[i+1].startsWith("-")) { val = argv[++i]; }
        if (nopt < maxOpts) { opts[nopt++] = { keyval, val }; }
      }
    } else if (t.startsWith("-") && t.length() > 1) {
      // short flags can be bundled like -abc or take value as next token (-o out)
      for (int k = 1; k < (int)t.length(); ++k) {
        String key( (char[]){ t[k], 0 } );
        String val = "";
        // if it's the last short flag in this token, allow value in next argv
        if (k == (int)t.length() - 1 && i + 1 < argc && !argv[i+1].startsWith("-")) {
          val = argv[++i];
        }
        if (nopt < maxOpts) { opts[nopt++] = { key, val }; }
      }
    } else {
      // positional argument; treat as key="" with value=t
      if (nopt < maxOpts) { opts[nopt++] = { "", t }; }
    }
  }
  return nopt;
}

// ---- Example dispatcher: call functions by name ----
void callFunctionByName(const String& name, int nopt, Opt opts[]) {
  // Stub: replace with your real registry
  if (name == "i2cScanApp" || name == "I2CScanApp") {
    // find args: --sdaRow, --sclRow, -v
    int sda = -1, scl = -1; bool verbose = false;
    for (int i = 0; i < nopt; ++i) {
      if (opts[i].key == "sdaRow") sda = opts[i].value.toInt();
      else if (opts[i].key == "sclRow") scl = opts[i].value.toInt();
      else if (opts[i].key == "v") verbose = true;
      else if (opts[i].key == "" && sda < 0) sda = opts[i].value.toInt(); // positional example
    }
    Serial.print("Calling i2cScanApp with sdaRow="); Serial.print(sda);
    Serial.print(" sclRow="); Serial.print(scl);
    Serial.print(" verbose="); Serial.println(verbose ? "true" : "false");
    // i2cScanApp(sda, scl, verbose); // <-- your actual function
  } else {
    Serial.print("Unknown function: "); Serial.println(name);
  }
}




void loop( ) {

menu:
    if ( firstLoop == 1 ) {

        if ( firstStart == true || autoCalibrationNeeded == true ) {
            if ( autoCalibrationNeeded == true ) {
                Serial.println( "New calibration options detected in config.txt. "
                                "Running automatic calibration..." );
                delay( 2000 );
            }
            calibrateDacs( );
            firstStart = false;
        }
        firstLoop = 2;

        // Serial.println("--------------------------------");
        loadChangedNetColorsFromFile( netSlot, 0 );

        // routableBufferPower(1, 1);
        if ( attract == 1 ) {
            defconDisplay = 0;
            netSlot = -1;
        } else {

            defconDisplay = -1;
        }

        goto loadfile;
    }

    if ( firstLoop == 2 ) {

        if ( jumperlessConfig.top_oled.connect_on_boot == 1 ) {
            // Serial.println("Initializing OLED");
            oled.init( );
        }

       // runApp(-1, "jdi MIPdisplay");

        firstLoop = 0;
#if SETUP_LOGIC_ANALYZER_ON_BOOT == 1
        goto setupla;
#endif
    }


    if ( Serial.available( ) >
         20 ) { // this is so if you dump a lot of data into the serial buffer, it
                // will consume it and not keep looping
        while ( Serial.available( ) > 0 ) {
            char c = Serial.read( );
            // Serial.print(c);
            // Serial.flush();
        }
    }

    if ( lastProbePowerDAC != probePowerDAC ) {
        probePowerDACChanged = true;
        // delay(1000);
        Serial.print( "probePowerDACChanged = " );
        Serial.println( probePowerDACChanged );
        routableBufferPower( 1, 1 );
    }

    clearHighlighting( );

    if ( dontShowMenu == 0 ) {
        forceprintmenu:


        int numberOfMenuItems = menuItemCounts[ showExtraMenu ];
        float steps =
            (float)highSaturationBrightColorsCount / ( (float)numberOfMenuItems );
        // Serial.print("steps = ");
        // Serial.println(steps);
        shownMenuItems = 0;
        // printSpectrumOrderedColorCube();
        cycleTerminalColor( true, steps, true, &Serial );
        shownMenuItems += printMenuLine( "\n\n\r\t\tMenu\n\r\n\r" );
        shownMenuItems += printMenuLine( "\t'help' for docs or [command]?\n\r" );
        shownMenuItems += printMenuLine( "\n\r" );
        shownMenuItems += printMenuLine( "\tm = show this menu\n\r" );

        shownMenuItems += printMenuLine( showExtraMenu, 0, "\te = show extra options (%d)\n\r", showExtraMenu );

        //  Serial.println();

        shownMenuItems += printMenuLine( showExtraMenu, 0, "\tn = show net list\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\tb = show bridge array\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\tc = show crossbar status\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\ts = show all slot files\n\r" );
        if ( showExtraMenu >= 0 ) {
            Serial.println( );
        }

        // Serial.println();

        shownMenuItems += printMenuLine( showExtraMenu, 2, "\t? = show firmware version\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\t' = show startup animation\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\td = set debug flags\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tl = LED brightness / test\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t\b\b`/~ = edit / print config\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\tp = microPython REPL\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t> = send Python formatted command\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t/ = show filesystem\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t\b\bU/u = enable/disable USB Mass Storage\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\tw = enable logic analyzer\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\tX = resource status\n\r" );
        // Serial.print("\tu = disable USB Mass Storage drive\n\r");
        // cycleTerminalColor();

        shownMenuItems += printMenuLine( showExtraMenu, 2, "\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\ty = refresh connections\n\r" );
        // shownMenuItems++;
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\t< = cycle slots\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tG = reload config.txt\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\to = load node file by slot\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tP = deinitialize MicroPython (free memory)\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\tF = cycle font\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\t_ = print micros per byte\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\t@ = scan I2C (@[sda],[scl] or @[row])\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\t$ = calibrate DACs\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\t= = dump oled frame buffer\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tk = show oled in terminal\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tR = show board LEDs\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\t% = list all filesystem contents\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\tE = don't show this menu\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 3, "\tC = disable terminal colors\n\r" );

        if ( showExtraMenu >= 2 ) {

            // Serial.print("\n\r");
        }
        Serial.println( );
        // shownMenuItems += printMenuLine(showExtraMenu, 1, "\n\r");
        //  Serial.print("\t$ = calibrate DACs\n\r");
        if ( probePowerDAC == 0 ) {
            shownMenuItems += printMenuLine( showExtraMenu, 3, "\t^ = set DAC 1 voltage\n\r" );
        } else if ( probePowerDAC == 1 ) {
            shownMenuItems += printMenuLine( showExtraMenu, 3, "\t^ = set DAC 0 voltage\n\r" );
        }
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\tv = get ADC reading\n\r" );
        // Serial.println();

        shownMenuItems += printMenuLine( showExtraMenu, 3, "\t# = print text from menu\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tg = print gpio state\n\r" );
        // Serial.print("\t\b\b\b\b[0-9] = run app by index\n\r");
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\t. = connect oled\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 2, "\tr = reset Arduino (rt/rb)\n\r" );

        shownMenuItems += printMenuLine( showExtraMenu, 1, "\t\b\ba/A = dis/connect UART to D0/D1\n\r" );


        shownMenuItems += printMenuLine( showExtraMenu, 1, "\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 1, "\tf = load node file\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\tx = clear all connections\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t+ = add connections\n\r" );
        shownMenuItems += printMenuLine( showExtraMenu, 0, "\t- = remove connections\n\r" );
        // Serial.print("\te = extra menu options\n\r");
        // Serial.println();

        Serial.println( );

        Serial.flush( );
    }
    if ( configChanged == true && millis( ) > 2000 ) {
        // Serial.print("config changed, saving...");
        saveConfig( );
        // Serial.println("\r                             \rconfig saved!\n\r");
        // Serial.flush();
        configChanged = false;
    }
    menuItemCount[ showExtraMenu ] = shownMenuItems;


dontshowmenu:

    connectFromArduino = '\0';
    firstConnection = -1;
    core1passthrough = 1;



    //! This is the main busy wait loop waiting for input
    while ( Serial.available( ) == 0 && connectFromArduino == '\0' &&
            slotChanged == 0 ) {

        unsigned long busyTimer = millis( );

        if ( logicAnalyzer.is_running( ) == true || logicAnalyzer.is_armed( ) == true ) {
            // julseview.check_heartbeat_watchdog();
            delay( 100 );


            continue;
        }


        // watchdog core loop should be called here

        int encoderNetHighlighted = encoderNetHighlight( );

        // Serial.println("encoderNetHighlighted: " + String(encoderNetHighlighted));

        // Serial.println("enc position: " + String(encoderPosition));
        if ( encoderNetHighlighted != -1 ) {
            firstConnection = encoderNetHighlighted;

        } else {
            // firstConnection = -1;
        }
        
        checkPads( );
        tud_task( );
        
        if ( clickMenu( ) >= 0 ) {
            core1passthrough = 0;
            goto loadfile;
        }


        int probeReading = justReadProbe( true );

        checkForReadingChanges( );

        warnNetTimeout( 1 );
        if ( probeReading > 0 ) {
            if ( highlightNets( probeReading ) > 0 ) {

                firstConnection = probeReading;
            }
        }

        if ( brightenedNet > 0 ) {
            int probeToggleResult = probeToggle( );
            if ( probeToggleResult != -1 ) {

            }
            if ( probeToggleResult >= 0 && brightenedNet > 0 ) {

                blockProbeButton = gpioToggleFrequency;
                blockProbeButtonTimer = millis( );

            } else if ( probeToggleResult == -5 ) {

                if ( firstConnection > 0 ) {
                    if ( warningNet == brightenedNet && warningTimeout > 0 ) {
                        // warningNet = -1;
                        // brightenedNet = 0;
                        warningTimeout = 0;
                        // warningTimer = 0;
                        connectOrClearProbe = 0;
                        showProbeLEDs = 2;
                        probeActive = 1;
                        input = '{';
                        probingTimer = millis( );
                        startupTimers[ 0 ] = millis( );
                        // Serial.println("probing\n\r");

                        goto skipinput;
                        // warnNet(-1);
                    } else {
                        warnNet( firstConnection );
                        warningTimeout = 3800;
                        warningTimer = millis( );
                    }
                }

                blockProbeButton = 800;
                blockProbeButtonTimer = millis( );
            } else if ( probeToggleResult == -3 ) {
                blockProbeButton = 800;
                blockProbeButtonTimer = millis( );

            } else if ( probeToggleResult == -2 ) {
                blockProbeButton = 800;
                blockProbeButtonTimer = millis( );

            } else if ( probeToggleResult == -4 ) {
                // warnNet(firstConnection);
                // assignNetColors();
                // showLEDsCore2 = 1;

                // Serial.print("-4 warningNet = ");
                // Serial.println(warningNet);
                // Serial.flush();
                // clearHighlighting();

                firstConnection = -1;
                blockProbeButton = 500;
                blockProbeButtonTimer = millis( );
            }
        } else {
            firstConnection = -1;
        }

        if ( ( millis( ) - waitTimer ) > 12 ) {
            waitTimer = millis( );

            int probeButton = checkProbeButton( );

            if ( probeButton != lastProbeButton ) {

                lastProbeButton = probeButton;

                // if (switchPosition == 1) {
                if ( probeButton > 0 ) {

                    if ( probeButton == 2 ) {

                        connectOrClearProbe = 1;
                        probeActive = 1;
                        showProbeLEDs = 1;
                        input = '}';
                        probingTimer = millis( );
                        brightenedNet = 0;
                        core1passthrough = 0;
                        goto skipinput;

                    } else if ( probeButton == 1 ) {
                        // getNothingTouched();
                        startupTimers[ 0 ] = millis( );
                        connectOrClearProbe = 0;
                        showProbeLEDs = 2;
                        probeActive = 1;
                        input = '{';
                        probingTimer = millis( );
                        // Serial.println("probing\n\r");
                        brightenedNet = 0;
                        core1passthrough = 0;
                        goto skipinput;
                    }
                }

            } else if ( probeButton > 0 && lastProbeButton > 0 &&
                        probeButton == lastProbeButton ) {

            }
        } else {
            checkSwitchPosition( ); 
        }

        if ( lastHighlightedNet != highlightedNet ) {

            lastHighlightedNet = highlightedNet;
        } else if ( lastBrightenedNet != brightenedNet ) {

            lastBrightenedNet = brightenedNet;
        } else if ( lastWarningNet != warningNet ) {

            lastWarningNet = warningNet;
        }

        // Serial.println(showReadings);
        if ( showReadings >= 1 ) {
            // chooseShownReadings();
            showMeasurements( 16, 0, 0 );
        }

        if ( mscModeEnabled == true ) {
            usbPeriodic();
        }

        oled.oledPeriodic();

        
    }

    input = Serial.read( );

    timer = millis( );
    // Serial.print("input = ");
    // Serial.println(input);
    // Serial.flush();

    // Handle multi-character help commands
    if ( input == 'h' ) {
        // Check if next character is available (for "help" command)
        unsigned long helpTimer = millis( );
        while ( Serial.available( ) == 0 && millis( ) - helpTimer < 100 ) {
            // Small timeout for typing "help"
        }
        if ( Serial.available( ) > 0 ) {
            String helpString = "h";
            while ( Serial.available( ) > 0 && helpString.length( ) < 50 ) {
                char c = Serial.read( );
                if ( c == '\n' || c == '\r' )
                    break;
                helpString += c;
            }
            if ( helpString == "help" ) {
                showGeneralHelp( );
                goto dontshowmenu;
            } else if ( helpString.startsWith( "help " ) ) {
                String category = helpString.substring( 5 );
                category.trim( );
                showCategoryHelp( category.c_str( ) );
                goto dontshowmenu;
            }
        } else {
            // Just 'h' alone, let it fall through to normal processing
        }
    }

    // Handle command? help requests
    if ( input != '\n' && input != '\r' && input != ' ' ) {
        // Check if next character is '?' for command-specific help
        unsigned long helpTimer = millis( );
        while ( Serial.available( ) == 0 && millis( ) - helpTimer < 100 ) {
            // Small timeout for typing command?
        }
        if ( Serial.available( ) > 0 ) {
            char nextChar = Serial.peek( );
            if ( nextChar == '?' ) {
                Serial.read( ); // consume the '?'
                showCommandHelp( input );
                goto dontshowmenu;
            }
        }
    }

    if ( input == ' ' || input == '\n' || input == '\r' ) {

        // Serial.print(input);
        // Serial.flush();
        goto dontshowmenu;
    }
skipinput:

    switch ( input ) {

    case 'z':{        
        handleUserFunction();
        goto dontshowmenu;
        break;
    }

      case '|': {
        erattaClearGPIO(-1);
        Serial.println("Eratta cleared");
        Serial.flush();
        goto dontshowmenu;
      }

    case 'w': //! w - Setup logic analyzer
    {
        // int tempReading = adc_read_blocking(8);
    setupla:

        if ( la_enabled ) {
            Serial.println( "Logic analyzer disabled, deinitializing..." );
            // julseview.deinit();
            la_enabled = false;
            goto dontshowmenu;
        } else {
            changeTerminalColor( 196, true, &Serial );
            Serial.println( "Logic analyzer enabled" );
            Serial.println( "Note: Logic analyzer is not yet fully functional and can make a mess of your memory" );
            Serial.println( "Make sure to save anything important on the file system before playing with it" );
            Serial.println( "Worst case, you can use this to nuke the flash and start fresh:" );
            changeTerminalColor( 39, true, &Serial );
            Serial.println( "https://github.com/Gadgetoid/pico-universal-flash-nuke/releases/latest" );

            changeTerminalColor( -1, true, &Serial );
            la_enabled = true;
        }

        
        break;
    }

    case 'X': { //! X - Resource Status
        Serial.println( "Resource Allocation Status:" );
        Serial.println( "==========================" );

        Serial.print( "Free Heap: " );
        Serial.println( rp2040.getFreeHeap( ) );

        Serial.println( "Total memory: " + String( rp2040.getTotalHeap( ) ) );
        Serial.println( "Free memory: " + String( rp2040.getFreeHeap( ) ) );

        // Check rotary encoder status
        if ( isRotaryEncoderInitialized( ) ) {
            Serial.println( "✓ Rotary Encoder: Initialized" );
            printRotaryEncoderStatus( );
        } else {
            Serial.println( "✗ Rotary Encoder: Not initialized" );
        }

        // Check for conflicts
        Serial.println( "\nConflict Detection:" );
        Serial.println( "Logic Analyzer conflicts: N/A (removed)" );

        printPIOStateMachines( );
        Serial.print( "rotary divider = " );
        Serial.println( rotaryDivider );

        
        Serial.println( "gpio    up dn\tfunction\tfunction_hex" );
        for ( int i = 0; i < 48; i++ ) {
            int pull = gpio_is_pulled_up( i );
            Serial.print( "gpio " );
            Serial.print( i );
            Serial.print( ":  " );
            if ( i < 10 ) {
                Serial.print( " " );
            }
            Serial.print( pull );
            Serial.print( "  " );

            pull = gpio_is_pulled_down( i );
            Serial.print( pull );

            Serial.print( "\t" );
            Serial.print( gpio_function_names[ gpio_get_function( i ) ].name );
            Serial.print( "\t" );
            Serial.print( gpio_get_function( i ), HEX );
            Serial.println( );
            Serial.flush( );
        }
        Serial.println( );
        break;
    }

    case 'G': { //! G - Load config.txt changes
        Serial.println( "Reloading config.txt..." );
        configChanged = true;

        break;
    }
    case 'S': { //! S - raw speed test
        Serial.println( "Raw speed test..." );
        Serial.println( "Read frequency on row 29\n\n\r" );

        pauseCore2 = true;
        unsigned long cycles = 1000000;
        unsigned long start = micros( );
        sendXYraw( 10, 0, 4, 1 );
        for ( int i = 0; i < cycles; i++ ) {
            sendXYraw( 10, 0, 0, 1 );
            sendXYraw( 10, 0, 0, 0 );
        
        }
        unsigned long end = micros( );
        Serial.print( "Time for " );
        Serial.print( cycles );
        Serial.print( " on off cycles: " );
        Serial.print( end - start );
        Serial.println( " microseconds" );
        Serial.print( "Time per cycle: " );
        Serial.print( ( end - start ) / cycles );
        Serial.println( " microseconds" );
        Serial.print( "Frequency: " );
        Serial.print( ( (float)cycles / (float)( end - start ) ) * 1000 );
        Serial.println( " kHz\n\r" );
        Serial.flush( );
        pauseCore2 = false;

        break;
    }

    case 'j':

        for ( int i = 0; i < highSaturationSpectrumColorsCount; i++ ) {
            changeTerminalColorHighSat( i, true, &Serial, 0 );
            Serial.print( i );
            Serial.print( ": " );
            if ( i < 10 ) {
                Serial.print( " " );
            }
            Serial.print( highSaturationSpectrumColors[ i ] );

            Serial.print( "\t\t" );
            if ( i < highSaturationBrightColorsCount ) {
                changeTerminalColorHighSat( i, true, &Serial, 1 );
                Serial.print( i );
                Serial.print( ": " );
                if ( i < 10 ) {
                    Serial.print( " " );
                }
                Serial.print( highSaturationBrightColors[ i ] );
            }
            Serial.println( );
        }

        goto dontshowmenu;
        break;

    case 'U': { //! U - Enable USB Mass Storage drive\n

        if ( mscModeEnabled == false ) {
            Serial.println( "Enabling USB Mass Storage drive..." );
            if ( initUSBMassStorage( ) ) {
                Serial.println( "USB Mass Storage enabled - device will appear as "
                                "'JUMPERLESS' drive\n\r" );
                Serial.println( "\tu = disable USB Mass Storage" );
                Serial.println( "\tG = reload config.txt" );
                Serial.println( "\ty = refresh connections when files change" );
                Serial.println( "\tS = show status" );
                Serial.println( "\n\r" );
                Serial.flush( );
                delay( 3000 );
            } else {
                Serial.println( "USB Mass Storage initialization failed" );
                Serial.flush( );
            }
        } else {
            Serial.println( "USB Mass Storage is already enabled" );
            printUSBMassStorageStatus( );
            refreshConnections( -1 );
            Serial.flush( );
        }

        delay( 3000 );
        unsigned long mscModeTimer = millis( );
        while ( mscModeEnabled == true ) {
            while ( Serial.available( ) == 0 ) {
                // if (millis() - mscModeTimer > 3000) {
                //   manualRefreshFromUSB();
                //   refreshConnections(-1);
                //   mscModeTimer = millis();
                // }
            }
            if ( Serial.available( ) > 0 ) {
                char c = Serial.read( );
                if ( c == 'u' ) {
                    Serial.println( "Disabling USB Mass Storage" );
                    disableUSBMassStorage( );
                    mscModeEnabled = false;
                    Serial.flush( );
                    refreshConnections( -1, 1, 1 );
                }
                if ( c == 'U' ) {
                    Serial.println( "Enabling USB Mass Storage" );
                    initUSBMassStorage( );
                    printUSBMassStorageStatus( );
                    Serial.flush( );
                }
                if ( c == 'y' || c == 'Y' ) {
                    Serial.println( "Refreshing connections" );
                    manualRefreshFromUSB( );
                    delay( 100 );
                    refreshConnections( -1 );

                    Serial.flush( );
                }
                if ( c == 'G' || c == 'g' ) {
                    Serial.println( "Reloading config.txt" );

                    Serial.flush( );
                    manualRefreshFromUSB( );
                    delay( 100 );
                    loadConfig( );
                    Serial.flush( );
                }
                if ( c == 's' || c == 'S' ) {
                    Serial.println( "Showing status" );
                    printUSBMassStorageStatus( );
                    Serial.flush( );
                }
            }
        }
        goto dontshowmenu;
        break;
    }

    case 'Z': { //! Z - Toggle USB debug mode
        Serial.println( "╭─────────────────────────────────╮" );
        Serial.println( "│        USB Debug Control        │" );
        Serial.println( "├─────────────────────────────────┤" );
        Serial.println( "│ 1. Toggle USB debug mode        │" );
        Serial.println( "│ 2. Manual refresh from USB      │" );
        Serial.println( "│ 3. Validate all slots           │" );
        Serial.println( "│ Any other key - Cancel          │" );
        Serial.println( "╰─────────────────────────────────╯" );
        Serial.print( "Choose option: " );
        Serial.flush( );

        // Wait for input
        while ( Serial.available( ) == 0 ) {
            delay( 1 );
        }
        char choice = Serial.read( );
        Serial.println( choice );

        switch ( choice ) {
        case '1':
            Serial.println( "\nToggling USB debug mode..." );
            setUSBDebug( !usb_debug_enabled );
            break;
        case '2':
            if ( isUSBMassStorageMounted( ) ) {
                Serial.println( "\nPerforming manual refresh from USB..." );
                manualRefreshFromUSB( );
            } else {
                Serial.println( "\nUSB drive not mounted" );
            }
            break;
        case '3':
            Serial.println( "\nValidating all slot files..." );
            // validateAllSlots(true);
            break;
        default:
            Serial.println( "\nCancelled" );
            break;
        }

        Serial.flush( );
        goto dontshowmenu;
        break;
    }

    case 'u': { //! u - Disable USB Mass Storage drive
        if ( mscModeEnabled == true ) {
            Serial.println( "Disabling USB Mass Storage drive..." );
            if ( disableUSBMassStorage( ) ) {
                Serial.println(
                    "USB Mass Storage disabled - device no longer appears as drive" );
                Serial.println( "Use 'U' command to re-enable when needed" );
            } else {
                Serial.println( "USB Mass Storage disable failed" );
            }
        } else {
            Serial.println( "USB Mass Storage is already disabled" );
            Serial.println( "Use 'U' command to enable" );
        }

        goto dontshowmenu;
        break;
    }

    case '/': { //!  /

        runApp( -1, (char*)"File Manager" );
        Serial.write( 0x0F );
        Serial.flush( );
        break;
    }

    case 'C': { //!  C
        disableTerminalColors = !disableTerminalColors;
        if ( disableTerminalColors ) {
            Serial.println( "Terminal colors disabled" );
        } else {
            Serial.println( "Terminal colors enabled" );
        }
        Serial.flush( );
        break;
    }
    case 'E': { //!  E
        if ( dontShowMenu == 0 ) {
            dontShowMenu = 1;
        } else {
            dontShowMenu = 0;
        }
        break;
    }
    case 'k': { 

        // Call the demo function directly - it will check for range input itself
        // Serial.println("Displaying color names (enter range like '10-200' for
        // specific range)"); colorPicker(0, 255);
        if ( jumperlessConfig.top_oled.show_in_terminal > 0 ) {
            jumperlessConfig.top_oled.show_in_terminal = 1;
        } else {
            jumperlessConfig.top_oled.show_in_terminal = 0;
        }
        configChanged = true;
        break;
    }

    case 0x10: { //! DLE
        input = '\0';
        dumpLED = 0;
        goto dontshowmenu;
    }
    case 'R': { //!  R
                // printWireStatus();

        // for (int i = 0; i < 10; i++) {
        if ( dumpLED == 1 ) {
            dumpLED = 0;
        } else {
            dumpLED = 1;
        }
        // }
        // printSerial1stuff();
        // printAllRLEimageData();
        goto dontshowmenu;
        break;
    }

        // Add this case for single Python command
    case '>': { //! > - Execute single Python command
        // readPythonCommand();
        getMicroPythonCommandFromStream( );
        Serial.flush( );
        goto dontshowmenu;
        break;
    }

    // Modify the existing P case for Python command mode
    case 'P': { //! P - Deinitialize MicroPython to free memory
        Serial.println(
            "Deinitializing MicroPython to free memory... Total memory: " +
            String( rp2040.getTotalHeap( ) ) );
        Serial.println( "Free memory: " + String( rp2040.getFreeHeap( ) ) );
        deinitMicroPythonProper( );
        Serial.println( "MicroPython deinitialized. Memory freed." );

        Serial.println( "Total memory: " + String( rp2040.getTotalHeap( ) ) );
        Serial.println( "Free memory: " + String( rp2040.getFreeHeap( ) ) );
        Serial.println( "Use 'p' to reinitialize and enter REPL again." );
        goto dontshowmenu;
        break;
    }

    case 'p': { 
        enterMicroPythonREPL( );

        refreshConnections( -1, 1, 1 );
        Serial.write( 0x0F );
        Serial.flush( );
        // printAllConnectableNodes();
        break;
    }
    case '.': { //!  .
        // initOLED();
        if ( jumperlessConfig.top_oled.enabled == 0 ) {
            Serial.println( "oled enabled" );
            oled.init( );
            jumperlessConfig.top_oled.enabled = 1;
            configChanged = true;
        } else {
            oled.disconnect( );
            jumperlessConfig.top_oled.enabled = 0;
            oled.oledConnected = false;

            configChanged = true;
            Serial.println( "oled disconnected" );
        }

        goto dontshowmenu;
        break;
    }

    case 'c': { //!  c
        printChipStateArray( );
        goto dontshowmenu;
        break;
    }

    case '_': { //!  _
        printMicrosPerByte( );
        goto dontshowmenu;
        break;
    }

    case 'g': { //!  g
        printGPIOState( );
        break;
    }
    case '&': { //!  &
        loadChangedNetColorsFromFile( netSlot, 0 );
        goto dontshowmenu;
        break;
        int node1 = -1;
        int node2 = -1;
        while ( Serial.available( ) == 0 ) {
        }
        // char c = Serial.read();
        node1 = Serial.parseInt( );
        node2 = Serial.parseInt( );
        Serial.print( "node1 = " );
        Serial.println( node1 );
        Serial.print( "node2 = " );
        Serial.println( node2 );
        Serial.print( "checkIfBridgeExistsLocal(node1, node2) = " );
        long unsigned int timer = micros( );
        Serial.println( checkIfBridgeExistsLocal( node1, node2 ) );
        Serial.print( "time taken = " );
        Serial.print( micros( ) - timer );
        Serial.println( " microseconds" );

        Serial.flush( );


        break;
    }

    case '\'': { //!  '
        pauseCore2 = 1;
        delay( 1 );
        drawAnimatedImage( 0 );
        pauseCore2 = 0;
        goto dontshowmenu;
        break;
    }
    case 'x': { //!  x
        digitalWrite( RESETPIN, HIGH );
        delay( 1 );
        refreshPaths( );
        clearAllNTCC( );
        // oled.oledConnected = false;

        clearNodeFile( netSlot, 0 );
        refreshConnections( -1, 1, 1 );
        digitalWrite( RESETPIN, LOW );

        Serial.println( "Cleared all connections" );

        goto dontshowmenu;

        break;
    }

    case '+': { //!  +

        readStringFromSerial( 0, 0 );
        goto loadfile;

        break;
    }

    case '-': { //!  -
        readStringFromSerial( 0, 1 );
        goto loadfile;
        break;
    }

    case '~': { //!  ~
        core1busy = 1;
        waitCore2( );
        printConfigToSerial( );
        core1busy = 0;
        Serial.flush( );
        goto dontshowmenu;
        break;
    }
    case '`': { //!  `
        core1busy = 1;
        waitCore2( );
        readConfigFromSerial( );
        core1busy = 0;
        Serial.flush( );
        goto dontshowmenu;
        break;
    }
        // case '2': {
        // runApp(2);
        // break;
        // }

    case '^': { //!  ^
        // doomOn = 1;
        // Serial.println(yesNoMenu());
        // break;
        char f[ 8 ] = { ' ' };
        int index = 0;
        float f1 = 0.0;
        unsigned long timer = millis( );
        while ( Serial.available( ) == 0 && millis( ) - timer < 1000 ) {
        }
        while ( index < 8 ) {
            f[ index ] = Serial.read( );
            index++;
        }

        f1 = atof( f );
        // Serial.print("f = ");
        // Serial.println(f1);
        if ( probePowerDAC == 1 ) {
            setDac0voltage( f1, 1, 1 );
        } else if ( probePowerDAC == 0 ) {
            setDac1voltage( f1, 1, 1 );
        }
        configChanged = true;
        Serial.printf( "DAC %d = %0.2f V\n", !probePowerDAC, f1 );
        Serial.flush( );
        goto dontshowmenu;
        break;
    }

    case '?': { //!  ?
        Serial.print( "Jumperless firmware version: " );
        Serial.println( firmwareVersion );
        Serial.flush( );
        goto dontshowmenu;
        break;
    }
    case '@': { //!  @
        Serial.flush( );

        if ( Serial.available( ) > 0 ) {
            String input = Serial.readString( );
            input.trim( ); // Remove whitespace

            if ( input.indexOf( ',' ) != -1 ) {
                // Format: @5,10 - SDA at row 5, SCL at row 10
                int commaIndex = input.indexOf( ',' );
                int sdaRow = input.substring( 0, commaIndex ).toInt( );
                int sclRow = input.substring( commaIndex + 1 ).toInt( );

                changeTerminalColor( 69, true );
                Serial.print( "I2C scan with SDA=" );
                Serial.print( sdaRow );
                Serial.print( ", SCL=" );
                Serial.println( sclRow );
                changeTerminalColor( 38, true );

                if ( i2cScan( sdaRow, sclRow, 26, 27, 1 ) > 0 ) {
                    Serial.println( "Found devices" );
                    return;
                } else {
                    removeBridgeFromNodeFile( RP_GPIO_26, sdaRow, netSlot, 0 );
                    removeBridgeFromNodeFile( RP_GPIO_27, sclRow, netSlot, 0 );
                    refreshConnections( -1, 1 );
                }
            } else if ( input.length( ) > 0 && isdigit( input[ 0 ] ) ) {
                // Format: @5 - try all 4 combinations around row 5
                int baseRow = input.toInt( );

                changeTerminalColor( 69, true );
                Serial.print( "I2C scan trying all combinations around row " );
                Serial.println( baseRow );
                changeTerminalColor( 38, true );

                // Try all 4 combinations: SDA=base SCL=base+1, SDA=base+1 SCL=base,
                // SDA=base SCL=base-1, SDA=base-1 SCL=base
                int combinations[ 4 ][ 2 ] = {
                    { baseRow, baseRow + 1 }, // SDA=base, SCL=base+1
                    { baseRow + 1, baseRow }, // SDA=base+1, SCL=base
                    { baseRow, baseRow - 1 }, // SDA=base, SCL=base-1
                    { baseRow - 1, baseRow }  // SDA=base-1, SCL=base
                };

                for ( int i = 0; i < 4; i++ ) {
                    int sdaRow = combinations[ i ][ 0 ];
                    int sclRow = combinations[ i ][ 1 ];

                    // // Skip invalid row numbers (must be 1-60)
                    // if (sdaRow < 1 || sdaRow > 60 || sclRow < 1 || sclRow > 60) {
                    //   continue;
                    // }

                    changeTerminalColor( 202, true );
                    Serial.print( "\nTrying SDA=" );
                    Serial.print( sdaRow );
                    Serial.print( ", SCL=" );
                    Serial.print( sclRow );
                    Serial.println( ":" );
                    changeTerminalColor( 38, true );
                    int devicesFound = i2cScan( sdaRow, sclRow, 26, 27, 0 );
                    if ( devicesFound > 0 ) {
                        changeTerminalColor( 199, true );
                        Serial.printf(
                            "\n\rfound %d devices: SDA at row %d, SCL at row %d\n\r",
                            devicesFound, sdaRow, sclRow );
                        changeTerminalColor( -1 );
                        return;
                    }
                    delay( 1 ); // Small delay between scans
                }
            } 
        } else {
            // Interactive mode - prompt for SDA and SCL
            Serial.print( "Enter SDA row: " );
            Serial.flush( );
            while ( Serial.available( ) == 0 ) {
            }
            int rowSDA = Serial.parseInt( );
            Serial.print( "Enter SCL row: " );
            Serial.flush( );
            while ( Serial.available( ) == 0 ) {
            }
            int rowSCL = Serial.parseInt( );

            changeTerminalColor( 69, true );
            Serial.print( "I2C scan with SDA=" );
            Serial.print( rowSDA );
            Serial.print( ", SCL=" );
            Serial.println( rowSCL );
            changeTerminalColor( 38, true );

            if ( i2cScan( rowSDA, rowSCL, 26, 27, 1 ) > 0 ) {
                // Serial.println("Found devices");
            } else {
                removeBridgeFromNodeFile( RP_GPIO_26, rowSDA, netSlot, 0 );
                removeBridgeFromNodeFile( RP_GPIO_27, rowSCL, netSlot, 0 );
                refreshConnections( -1, 1 );
            }
        }

        goto dontshowmenu;
        break;
    }
    case '$': { //!  $
        // return current slot number
        for ( int d = 0; d < 4; d++ ) {
            Serial.print( "dacSpread[" );
            Serial.print( d );
            Serial.print( "] = " );
            Serial.println( dacSpread[ d ] );
        }

        for ( int d = 0; d < 4; d++ ) {
            Serial.print( "dacZero[" );
            Serial.print( d );
            Serial.print( "] = " );
            Serial.println( dacZero[ d ] );
        }

        calibrateDacs( );
        // Serial.println(netSlot);
        break;
    }
    case 'r': { //!  r
        if ( Serial.available( ) > 0 ) {
            char c = Serial.read( );
            if ( c == '0' || c == '2' || c == 't' ) {
                resetArduino( 0 );
            }
            if ( c == '1' || c == '2' || c == 'b' ) {
                resetArduino( 1 );
            }
        } else {
            resetArduino( );
        }
        goto dontshowmenu;
        break;
    }

    case 'A': { //!  A
        // delay(100);
        int justAsk = 0;
        if ( Serial.available( ) > 0 ) {
            // Serial.print("checking for arduino connection");
            char c = Serial.read( );
            // if (c == ' ') {
            //   continue;
            //   }
            if ( c == '?' ) {
                if ( checkIfArduinoIsConnected( ) == 1 ) {
                    justAsk = 1;
                    Serial.println( "Y" );
                    Serial.flush( );
                    // break;
                } else {
                    justAsk = 1;
                    Serial.println( "n" );
                    Serial.flush( );
                    // break;
                }
            } else {
                // break;
            }
        }
        if ( justAsk == 0 ) {
            connectArduino( 0 );
            Serial.println( "UART connected to Arduino D0 and D1" );
            Serial.flush( );
            
        }
        goto dontshowmenu;
        break;
    }
    case 'a': { //!  a
        // delay(100);
        int justAsk = 0;
        while ( Serial.available( ) > 0 ) {
            // Serial.print("checking for arduino connection");
            char c = Serial.read( );
            // if (c == ' ') {
            //   continue;
            //   }
            if ( c == '?' ) {
                if ( checkIfArduinoIsConnected( ) == 1 ) {
                    justAsk = 1;
                    Serial.println( "Y" );
                    Serial.flush( );
                    // break;
                } else {
                    justAsk = 1;
                    Serial.println( "n" );
                    Serial.flush( );
                    // break;
                }
            } else {
                // break;
            }
        }
        if ( justAsk == 0 ) {
            disconnectArduino( 0 );
            Serial.println( "UART disconnected from Arduino D0 and D1" );
            Serial.flush( );
           
        }
        // goto loadfile;
        goto dontshowmenu;
        break;
    }

    case 'F': //!  F
        oled.cycleFont( );
        break;

        

    case '=': { 
        Serial.println( "\n\r" );
        
        oled.dumpFrameBuffer( );

        goto dontshowmenu;
        break;
    }

    case 'i': { //!  i
        if ( oled.isConnected( ) == false ) {
            if ( oled.init( ) == false ) {
                Serial.println( "Failed to initialize OLED" );
                break;
            }
        }


        break;
    }

    case '#': { 

        while ( Serial.available( ) == 0 && slotChanged == 0 ) {
            if ( slotChanged == 1 ) {
                // b.print("Jumperless", 0x101000, 0x020002, 0);
                // delay(100);
                goto menu;
            }
        }
        printTextFromMenu( );

        clearLEDs( );
        showLEDsCore2 = 1;
        defconDisplay = -1;
        
        break;
    }
    case 'e': { //!  e
        showExtraMenu++;
        if ( showExtraMenu > 3 ) {
            showExtraMenu = 0;
        }
        break;
    }

    case 's': { //!  s
        printSlots( -1 );

        break;
    }
    case 'v': { //!  v
        if ( Serial.available( ) > 0 ) {
            char c = Serial.read( );

            if ( isdigit( c ) == 1 ) {
                int adc = c - '0';
                if ( adc >= 0 && adc <= 4 ) {
                    Serial.print( " adc" );
                    Serial.print( adc );
                    Serial.print( " = " );
                    float adcVoltage = readAdcVoltage( adc, 32 );
                    if ( adcVoltage > 0.00 ) {
                        Serial.print( " " );
                    }
                    Serial.println( adcVoltage );
                } else if ( c == 'p' ) {
                    Serial.print( " probe = " );
                    float probeVoltage = readAdcVoltage( 7, 32 );
                    if ( probeVoltage > 0.00 ) {
                        Serial.print( " " );
                    }
                    Serial.println( probeVoltage );
                }
            } else if ( c == 'i' ) {
                if ( Serial.available( ) > 0 ) {
                    char c = Serial.read( );
                    if ( c == '1' ) {
                        float iSense = INA1.getCurrent_mA( );
                        Serial.print( "ina1 = " );
                        Serial.print( iSense );
                        Serial.println( "mA" );
                    }
                } else {
                    float iSense = INA0.getCurrent_mA( );
                    Serial.print( "ina0 = " );
                    Serial.print( iSense );
                    Serial.print( "mA \t" );

                    iSense = INA0.getBusVoltage( );
                    Serial.print( iSense );
                    Serial.print( "V \t" );

                    iSense = INA0.getPower_mW( );
                    Serial.print( iSense );
                    Serial.println( "mW" );
                }
            } else if ( c == 'l' ) {

                if ( showReadings == 1 ) {
                    showReadings = 0;
                    Serial.println( "showReadings = 0" );
                } else {
                    showReadings = 1;
                    Serial.println( "showReadings = 1" );
                }
                chooseShownReadings( );
            }
            Serial.flush( );
        } else {
            Serial.println( );
            for ( int i = 0; i < 5; i++ ) {
                Serial.print( "adc" );
                Serial.print( i );
                Serial.print( " = " );
                float adcVoltage = readAdcVoltage( i, 32 );
                if ( adcVoltage > 0.00 ) {
                    Serial.print( " " );
                }
                Serial.println( adcVoltage );
            }
            Serial.print( "probe = " );
            float probeVoltage = readAdcVoltage( 7, 32 );
            if ( probeVoltage > 0.00 ) {
                Serial.print( " " );
            }
            Serial.println( probeVoltage );
        }
        Serial.flush( );
        goto dontshowmenu;
        break;

        if ( showReadings >= 3 || ( inaConnected == 0 && showReadings >= 1 ) ) {
            showReadings = 0;
            break;
        } else {
            showReadings++;

            chooseShownReadings( );
        

            goto dontshowmenu;
            break;
        }
    }
    case '}': {
        
        blockProbeButton = 300;
        blockProbeButtonTimer = millis( );
        probeMode( 1, firstConnection );
        
        probeActive = 0;


        clearHighlighting( );
        
        goto menu;
        // break;
    }
    case '{': {
        
        blockProbeButton = 300;
        blockProbeButtonTimer = millis( );
        int probeReturn = probeMode( 0, firstConnection );

        
        probeActive = 0;
        clearHighlighting( );        
        goto menu;
        // break;
    }
    case 'n':
        couldntFindPath( 1 );
        core1passthrough = 0;
        Serial.print( "\n\n\rnetlist\n\r" );
        
        listNets( anythingInteractiveConnected( -1 ) );

        break;
    case 'b': {
        int showDupes = 1;
        char in = Serial.read( );
        if ( in == '0' ) {
        } else if ( in == '2' ) {
            showDupes = 2;
        }
        Serial.print( "\n\rpathDuplicates: " );
        Serial.println( jumperlessConfig.routing.stack_paths );
        Serial.print( "dacDuplicates: " );
        Serial.println( jumperlessConfig.routing.stack_dacs );
        Serial.print( "railsDuplicates: " );
        Serial.println( jumperlessConfig.routing.stack_rails );
        Serial.print( "railPriority: " );
        Serial.println( jumperlessConfig.routing.rail_priority );
        couldntFindPath( 1 );
        Serial.print( "\n\rBridge Array\n\r" );
        printBridgeArray( );
        Serial.print( "\n\n\n\rPaths\n\r" );
        printPathsCompact( showDupes );
        Serial.print( "\n\n\rChip Status\n\r" );
        printChipStatus( );
        Serial.print( "\n\n\r" );
        
        Serial.print( "\n\n\r" );
        break;
    }
    case 'm':
        goto forceprintmenu;
        break;

    case '!':
        printNodeFile( netSlot, 0, 0, 0, true );
        break;

    case 'o': {
        // probeActive = 1;
        inputNodeFileList( rotaryEncoderMode );
        showSavedColors( netSlot );
        // input = ' ';
        showLEDsCore2 = -1;
        // probeActive = 0;
        goto loadfile;
        // goto dontshowmenu;
        break;
    }


    case '<': {

        if ( netSlot == 0 ) {
            netSlot = NUM_SLOTS - 1;
        } else {
            netSlot--;
        }
        Serial.print( "Slot " );
        Serial.println( netSlot );
        slotPreview = netSlot;
        slotChanged = 1;

        goto loadfile;
    }
    case 'y': {
    loadfile:
        loadingFile = 1;
        
        if ( slotChanged == 1 ) {
            // clearChangedNetColors(0);
            loadChangedNetColorsFromFile( netSlot, 0 );
        
        }

        slotChanged = 0;
        loadingFile = 0;

        
        refreshConnections( -1 );
        
        break;
    }
    case 'f': {

        probeActive = 1;
        readInNodesArduino = 1;
        
        savePreformattedNodeFile( serSource, netSlot, rotaryEncoderMode );

        // Validate the saved node file
        int validation_result = validateNodeFileSlot( netSlot, false );
        if ( validation_result == 0 ) {
            if (debugFP) {
                Serial.println( "NodeFile validated successfully" );    
            }
            refreshConnections( -1 );
        } else {
            if (debugFP) {
            Serial.println( "NodeFile validation failed: " +
                            String( getNodeFileValidationError( validation_result ) ) );
                Serial.println( "Connections not refreshed due to invalid node file" );
            }
        }

        
        input = ' ';

        probeActive = 0;
        if ( connectFromArduino != '\0' ) {
            connectFromArduino = '\0';
            input = ' ';
            readInNodesArduino = 0;

            goto dontshowmenu;
        }
        

        connectFromArduino = '\0';
        readInNodesArduino = 0;
        break;
    }

       

    case 't': { //! t - Test MSC callbacks
        // Test function disabled
        goto dontshowmenu;
        break;
    }

    case 'T': { //! T - Show netlist info
#ifdef FSSTUFF
        openNodeFile( );
        getNodesToConnect( );
#endif
        Serial.println( "\n\n\rnetlist\n\n\r" );

        bridgesToPaths( );

        listSpecialNets( );
        listNets( );
        printBridgeArray( );
        Serial.print( "\n\n\r" );
        Serial.print( numberOfNets );

        Serial.print( "\n\n\r" );
        Serial.print( numberOfPaths );
        checkChangedNetColors( -1 );

        assignNetColors( );
#ifdef PIOSTUFF
        sendAllPaths( );
#endif

        break;
    }

    case 'l':
        if ( LEDbrightnessMenu( ) == '!' ) {
            clearLEDs( );
            delayMicroseconds( 9200 );
            sendAllPathsCore2 = 1;
        }
        break;

        goto dontshowmenu;

        break;

    case 'd': {
        // debugFlagInit();
        debugFlagsMenu( );
    }

    case ':':

        if ( Serial.read( ) == ':' ) {
            // Serial.print("\n\r");
            // Serial.print("entering machine mode\n\r");
            // machineMode();
            showLEDsCore2 = 1;
            goto dontshowmenu;
            break;
        } else {
            break;
        }

    default:
        while ( Serial.available( ) > 0 ) {
            int f = Serial.read( );
            // delayMicroseconds(30);
        }

        break;
    }
    delayMicroseconds( 1000 );
    while ( Serial.available( ) > 5 ) {
        Serial.read( );
        delayMicroseconds( 1000 );
    }
    Serial.flush( );
    goto menu;
}

unsigned long lastSwirlTime = 0;

int swirlCount = 42;
int spread = 13;

int readcounter = 0;
unsigned long schedulerTimer = 0;
unsigned long schedulerUpdateTime = 6300;

int swirled = 0;
int countsss = 0;

int probeCycle = 0;
int netUpdateRefreshCount = 0;

int tempDD = 0;
int clearBeforeSend = 0;

unsigned long tempTimer = 0;
int lastTemp = 0;

int passthroughStatus = 0;

unsigned long serialInfoTimer = 0;

unsigned long la_timer = 0;
unsigned long uartTaskTimer = 0;

void loop1( ) {

    while ( pauseCore2 == true ) {
        tight_loop_contents( );
    }

    // Only call logic analyzer if it's enabled and there's USB activity
    static uint32_t last_la_check = 0;
    uint32_t current_time = millis( );

    // ENHANCED STATE-BASED HANDLER CALLING
    // Use the new state variables to make smarter decisions about when to call the handler
    bool should_call_handler = false;

    // Route PulseView traffic to the new logic analyzer
    if ( ( millis( ) - last_la_check >= 20 ) || ( millis( ) - logicAnalyzer.last_command_time < 3000 ) ) {
        last_la_check = millis( );
        logicAnalyzer.handler( );
    }



    if ( doomOn == 1 ) {
        playDoom( );
        doomOn = 0;
    } else if ( pauseCore2 == 0 && logicAnalyzer.getIsRunning( ) == false ) {
        core2stuff( );
    }


    if (millis() - uartTaskTimer > 10) {
        uartTaskTimer = millis();
        if (jumperlessConfig.serial_1.async_passthrough == true) {
            AsyncPassthrough::task();
        }
    
        passthroughStatus = secondSerialHandler( );
    }

    replyWithSerialInfo( );

    if ( dumpLED == 1 ) {

        if ( millis( ) - dumpLEDTimer > dumpLEDrate ) {
            if ( core1busy == false ) {
                core2busy = true;
                core1busy = true;
                delayMicroseconds( 2000 );
                dumpLEDs( );
                delayMicroseconds( 1000 );
                core2busy = false;
                core1busy = false;
            }

            dumpLEDTimer = millis( );
        }
    }

    if ( blockProbingTimer > 0 ) {
        if ( millis( ) - blockProbingTimer > blockProbing ) {
            blockProbing = 0;
            blockProbingTimer = 0;
    
        }
    }
}

void core2stuff( ) // core 2 handles the LEDs and the CH446Q8
{
    core2busy = false;

    if ( showLEDsCore2 < 0 ) {
        showLEDsCore2 = abs( showLEDsCore2 );
        // Serial.println("clearBeforeSend = 1");

        clearBeforeSend = 1;
    }

   
    if ( micros( ) - schedulerTimer > schedulerUpdateTime || showLEDsCore2 == 3 ||
         showLEDsCore2 == 4 ||
         showLEDsCore2 == 6 && core1busy == false && core1request == 0 ) {

        if ( ( ( ( showLEDsCore2 >= 1 && loadingFile == 0 ) || showLEDsCore2 == 3 ||
                 ( swirled == 1 ) && sendAllPathsCore2 == 0 ) ||
               showProbeLEDs != lastProbeLEDs ) &&
             sendAllPathsCore2 == 0 ) {

            if ( showLEDsCore2 == 6 ) {
                showLEDsCore2 = 1;
            }

            int rails =
                showLEDsCore2; // 3 doesn't show nets and keeps control of the LEDs

            if ( rails != 3 ) {
                core2busy = true;
                lightUpRail( -1, -1, 1 );
                logoSwirl( swirlCount, spread, probeActive );
                core2busy = false;
            }

            if ( rails == 5 || rails == 3 ) {
                core2busy = true;

                logoSwirl( swirlCount, spread, probeActive );
                core2busy = false;
            }

            if ( rails != 2 && rails != 5 && rails != 3 && inClickMenu == 0 &&
                 inPadMenu == 0 && hideNets == 0 ) {

                if ( defconDisplay >= 0 && probeActive == 0 ) {

                    // core2busy = true;
                    defcon( swirlCount, spread, defconDisplay );
                    // core2busy = false;
                } else {

                    while ( core1busy == true ) {
                        // core2busy = false;
                    }
                    core2busy = true;

                    if ( clearBeforeSend == 1 ) {
                        clearLEDsExceptRails( );
                        // Serial.println("clearing");
                        clearBeforeSend = 0;
                    }

                    readGPIO( ); // if want, I can make this update the LEDs like 10 times
                                 // faster by putting outside this loop,
                    showLEDmeasurements( );

                    showNets( );

                    showAllRowAnimations( );

                    core2busy = false;
                    netUpdateRefreshCount = 0;
                }
            }

            core2busy = true;

            leds.show( );

            // probeLEDs.clear();

            if ( checkingButton == 0 || showProbeLEDs == 2 ) {
                probeLEDhandler( );
                // core2busy = false;
            }
            core2busy = false;
            if ( rails != 3 && swirled == 0 ) {
                showLEDsCore2 = 0;

                // delayMicroseconds(3200);
            }

            swirled = 0;
            if ( inClickMenu == 1 ) {
                rotaryEncoderStuff( );
            }
            core2busy = false;

        } else if ( sendAllPathsCore2 != 0 ) {

            if ( sendAllPathsCore2 == 1 ) {
                sendPaths( 0 );
            } else if ( sendAllPathsCore2 == -1 ) {
                sendPaths( 1 );
            } else {
                sendPaths( sendAllPathsCore2 );
            }
            sendAllPathsCore2 = 0;

        } else if ( millis( ) - lastSwirlTime > 51 && loadingFile == 0 &&
                    showLEDsCore2 == 0 && core1busy == false ) {
            readcounter++;


            lastSwirlTime = millis( );

            if ( swirlCount >= LOGO_COLOR_LENGTH - 1 ) {
                swirlCount = 0;
            } else {
                swirlCount++;
            }

            if ( swirlCount % 20 == 0 ) {
                countsss++;
            }

            if ( showLEDsCore2 == 0 ) {
                swirled = 1;
            }

            // leds.show();
        } else if ( inClickMenu == 0 && probeActive == 0 ) {

            if ( ( ( countsss > 8 && defconDisplay >= 0 ) || countsss > 10 ) &&
                 defconDisplay != -1 ) {
                countsss = 0;

                if ( defconDisplay != -1 ) {
                    tempDD++;

                    if ( tempDD > 6 ) {
                        tempDD = 0;
                    }
                    defconDisplay = tempDD;
                } 

                if ( defconDisplay > 5 ) {
                    defconDisplay = 0;
                }
            }

            if ( readcounter > 100 ) {
                readcounter = 0;
                if ( probeCycle > 4 ) {
                    probeCycle = 1;
                }
            }

            rotaryEncoderStuff( );

        } else {
            rotaryEncoderStuff( );
        }
        schedulerTimer = micros( );
        core2busy = false;

    }
}
