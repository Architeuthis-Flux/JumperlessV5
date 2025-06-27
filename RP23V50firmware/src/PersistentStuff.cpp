#include "PersistentStuff.h"
#include "FileParsing.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "NetManager.h"
#include "Probing.h"
#include "Peripherals.h"
#include <EEPROM.h>
#include "Graphics.h"
#include "configManager.h"
#include "config.h"
#include "ArduinoStuff.h"

bool firstStart = false;

void debugFlagInit(int forceDefaults) {

  EEPROM.begin(512);

  if (EEPROM.read(FIRSTSTARTUPADDRESS) != 0xAA || forceDefaults == 1) {

    delay(1000);
    Serial.println("First startup");
    delay(1000);
    firstStart = true;
    EEPROM.write(FIRSTSTARTUPADDRESS, 0xAA);

    EEPROM.write(REVISIONADDRESS, REV);

    EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);
    EEPROM.write(TIME_FILEPARSINGADDRESS, 0);
    EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);
    EEPROM.write(TIME_NETMANAGERADDRESS, 0);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);
    EEPROM.write(DEBUG_LEDSADDRESS, 0);
    EEPROM.write(LEDBRIGHTNESSADDRESS, DEFAULTBRIGHTNESS);
    EEPROM.write(RAILBRIGHTNESSADDRESS, DEFAULTRAILBRIGHTNESS);
    EEPROM.write(SPECIALBRIGHTNESSADDRESS, DEFAULTSPECIALNETBRIGHTNESS);

    EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);
    EEPROM.write(DISPLAYMODE_ADDRESS, 1);
    EEPROM.write(NETCOLORMODE_ADDRESS, 0);
    EEPROM.write(MENUBRIGHTNESS_ADDRESS, 100);
    EEPROM.write(PATH_DUPLICATE_ADDRESS, 2);
    EEPROM.write(DAC_DUPLICATE_ADDRESS, 0);
    EEPROM.write(POWER_DUPLICATE_ADDRESS, 2);
    EEPROM.write(DAC_PRIORITY_ADDRESS, 1);
    EEPROM.write(POWER_PRIORITY_ADDRESS, 1);
    EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 0);

    saveVoltages(0.0f, 0.0f, 3.33f, 0.0f);

    EEPROM.commit();
    delay(5);
    }

#ifdef EEPROMSTUFF

  debugFP = EEPROM.read(DEBUG_FILEPARSINGADDRESS);
  debugFPtime = EEPROM.read(TIME_FILEPARSINGADDRESS);

  debugNM = EEPROM.read(DEBUG_NETMANAGERADDRESS);
  debugNMtime = EEPROM.read(TIME_NETMANAGERADDRESS);

  debugNTCC = EEPROM.read(DEBUG_NETTOCHIPCONNECTIONSADDRESS);
  debugNTCC2 = EEPROM.read(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS);

  LEDbrightnessRail = EEPROM.read(RAILBRIGHTNESSADDRESS);
  LEDbrightness = EEPROM.read(LEDBRIGHTNESSADDRESS);
  LEDbrightnessSpecial = EEPROM.read(SPECIALBRIGHTNESSADDRESS);

  debugLEDs = EEPROM.read(DEBUG_LEDSADDRESS);

  rotaryEncoderMode = EEPROM.read(ROTARYENCODER_MODE_ADDRESS);

  //displayMode = EEPROM.read(DISPLAYMODE_ADDRESS);


  netColorMode = EEPROM.read(NETCOLORMODE_ADDRESS);

  revisionNumber = EEPROM.read(REVISIONADDRESS);

  // pathDuplicates = EEPROM.read(PATH_DUPLICATE_ADDRESS);
  // dacDuplicates = EEPROM.read(DAC_DUPLICATE_ADDRESS);
  // powerDuplicates = EEPROM.read(POWER_DUPLICATE_ADDRESS);
  // dacPriority = EEPROM.read(DAC_PRIORITY_ADDRESS);
  // powerPriority = EEPROM.read(POWER_PRIORITY_ADDRESS);

  showProbeCurrent = EEPROM.read(SHOW_PROBE_CURRENT_ADDRESS);



  menuBrightnessSetting = EEPROM.read(MENUBRIGHTNESS_ADDRESS) - 100;

  dacSpread[1] = EEPROM.get(DAC0_SPREAD_ADDRESS, dacSpread[0]);
  dacSpread[1] = EEPROM.get(DAC1_SPREAD_ADDRESS, dacSpread[1]);
  dacSpread[2] = EEPROM.get(TOP_RAIL_SPREAD_ADDRESS, dacSpread[2]);
  dacSpread[3] = EEPROM.get(BOTTOM_RAIL_SPREAD_ADDRESS, dacSpread[3]);

  dacZero[0] = EEPROM.get(DAC0_ZERO_ADDRESS, dacZero[0]);
  dacZero[1] = EEPROM.get(DAC1_ZERO_ADDRESS, dacZero[1]);
  dacZero[2] = EEPROM.get(TOP_RAIL_ZERO_ADDRESS, dacZero[2]);
  dacZero[3] = EEPROM.get(BOTTOM_RAIL_ZERO_ADDRESS, dacZero[3]);

  for (int i = 0; i < 4; i++) {
    if (dacSpread[i] < 12.0 || dacSpread[i] > 28.0 || dacSpread[i] != dacSpread[i]) {
      // delay(2000);

      // Serial.print("dacSpread[");
      // Serial.print(i);
      // Serial.print("] out of range = ");
      // Serial.println(dacSpread[i]);
      dacSpread[i] = 21.0;
      EEPROM.put(DAC0_SPREAD_ADDRESS + (i * 8), dacSpread[i]);
      //EEPROM.put(CALIBRATED_ADDRESS, 0);
      }
    if (dacZero[i] < 1000 || dacZero[i] > 2000) {

      // Serial.print("dacZero[");
      // Serial.print(i);
      // Serial.print("] out of range = ");
      // Serial.println(dacZero[i]);
      dacZero[i] = 1630;
      EEPROM.put(DAC0_ZERO_ADDRESS + (i * 4), 1630);
      // EEPROM.put(CALIBRATED_ADDRESS, 0);
      }
    }
  if (showProbeCurrent != 0 && showProbeCurrent != 1)
    {
    EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 0);
    showProbeCurrent = 0;
    }
  // delay(3000);
  // Serial.print("pathDuplicates: ");
  // Serial.println(pathDuplicates);
  // Serial.print("dacDuplicates: ");
  // Serial.println(dacDuplicates);
  // Serial.print("powerDuplicates: ");
  // Serial.println(powerDuplicates);
  // Serial.print("dacPriority: ");
  // Serial.println(dacPriority);
  // Serial.print("powerPriority: ");
  // Serial.println(powerPriority);

  // if (pathDuplicates < 0 || pathDuplicates > 20) {
  //   Serial.print("pathDuplicates out of range (");
  //   Serial.print(pathDuplicates);
  //   Serial.println("), setting to 3");
  //   EEPROM.write(PATH_DUPLICATE_ADDRESS, 3);
  //   pathDuplicates = 3;
  //   }
  // if (dacDuplicates < 0 || dacDuplicates > 20) {
  //   Serial.print("dacDuplicates out of range (");
  //   Serial.print(dacDuplicates);
  //   Serial.println("), setting to 0");

  //   EEPROM.write(DAC_DUPLICATE_ADDRESS, 0);
  //   dacDuplicates = 0;
  //   }
  // if (powerDuplicates < 0 || powerDuplicates > 20) {
  //   Serial.print("powerDuplicates out of range (");
  //   Serial.print(powerDuplicates);
  //   Serial.println("), setting to 3");


  //   EEPROM.write(POWER_DUPLICATE_ADDRESS, 3);
  //   powerDuplicates = 3;
  //   }
  // if (dacPriority < 1 || dacPriority > 10) {

  //   Serial.print("dacPriority out of range (");
  //   Serial.print(dacPriority);
  //   Serial.println("), setting to 1");

  //   EEPROM.write(DAC_PRIORITY_ADDRESS, 1);
  //   dacPriority = 1;
  //   }
  // if (powerPriority < 1 || powerPriority > 10) {

  //   Serial.print("powerPriority out of range (");
  //   Serial.print(powerPriority);
  //   Serial.println("), setting to 1");

  //   EEPROM.write(POWER_PRIORITY_ADDRESS, 1);
  //   powerPriority = 1;
  //   }


  if (revisionNumber <= 0 || revisionNumber > 10) {


    //delay(5000);
    Serial.print("Revision number out of range (");
    Serial.print(revisionNumber);
    Serial.print("), setting to revision ");
    EEPROM.write(REVISIONADDRESS, REV);
    revisionNumber = REV;

    Serial.println(revisionNumber);
    }


  if (debugFP != 0 && debugFP != 1)
    {
    EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);

    debugFP = false;
    }
  if (debugFPtime != 0 && debugFPtime != 1)
    {
    EEPROM.write(TIME_FILEPARSINGADDRESS, 0);
    debugFPtime = false;
    }

  if (debugNM != 0 && debugNM != 1)
    {
    EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);
    debugNM = false;
    }

  if (debugNMtime != 0 && debugNMtime != 1)
    {
    EEPROM.write(TIME_NETMANAGERADDRESS, 0);
    debugNMtime = false;
    }

  if (debugNTCC != 0 && debugNTCC != 1)
    {
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);
    debugNTCC = false;
    }

  if (debugNTCC2 != 0 && debugNTCC2 != 1)
    {
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);
    debugNTCC2 = false;
    }

  if (debugLEDs != 0 && debugLEDs != 1)
    {

    EEPROM.write(DEBUG_LEDSADDRESS, 0);
    debugLEDs = false;
    }

  if (LEDbrightnessRail < 0 || LEDbrightnessRail > 200) {
    EEPROM.write(RAILBRIGHTNESSADDRESS, DEFAULTRAILBRIGHTNESS);

    LEDbrightnessRail = DEFAULTRAILBRIGHTNESS;
    }
  if (LEDbrightness < 0 || LEDbrightness > 200) {
    EEPROM.write(LEDBRIGHTNESSADDRESS, DEFAULTBRIGHTNESS);
    LEDbrightness = DEFAULTBRIGHTNESS;
    }

  if (LEDbrightnessSpecial < 0 || LEDbrightnessSpecial > 200) {
    EEPROM.write(SPECIALBRIGHTNESSADDRESS, DEFAULTSPECIALNETBRIGHTNESS);
    LEDbrightnessSpecial = DEFAULTSPECIALNETBRIGHTNESS;
    }
  // delay(3000);
  // Serial.print("menuBrightnessSetting out of range = ");
  // Serial.println(menuBrightnessSetting);
  if (menuBrightnessSetting < -100 || menuBrightnessSetting > 100) {


    EEPROM.write(MENUBRIGHTNESS_ADDRESS, 100);
    menuBrightnessSetting = 0;
    }


  if (rotaryEncoderMode != 0 && rotaryEncoderMode != 1) {
    EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);
    rotaryEncoderMode = 0;
    }
  // if (displayMode != 0 && displayMode != 1) {
  //   EEPROM.write(DISPLAYMODE_ADDRESS, 1);
  //   displayMode = 0;
  //   }
  if (netColorMode != 0 && netColorMode != 1) {
    EEPROM.write(NETCOLORMODE_ADDRESS, 0);
    netColorMode = 0;
    }

#endif


  readVoltages();
  readLogoBindings();
  EEPROM.commit();
  delayMicroseconds(100);

  //loadConfig();

  }

void saveDacCalibration(void)
  {
  // Save to EEPROM
  EEPROM.put(DAC0_SPREAD_ADDRESS, dacSpread[0]);
  EEPROM.put(DAC1_SPREAD_ADDRESS, dacSpread[1]);
  EEPROM.put(TOP_RAIL_SPREAD_ADDRESS, dacSpread[2]);
  EEPROM.put(BOTTOM_RAIL_SPREAD_ADDRESS, dacSpread[3]);

  EEPROM.put(DAC0_ZERO_ADDRESS, dacZero[0]);
  EEPROM.put(DAC1_ZERO_ADDRESS, dacZero[1]);
  EEPROM.put(TOP_RAIL_ZERO_ADDRESS, dacZero[2]);
  EEPROM.put(BOTTOM_RAIL_ZERO_ADDRESS, dacZero[3]);

  EEPROM.put(CALIBRATED_ADDRESS, 0x55);

  EEPROM.commit();
  delayMicroseconds(100);

  // Also save to config file
  jumperlessConfig.calibration.dac_0_spread = dacSpread[0];
  jumperlessConfig.calibration.dac_1_spread = dacSpread[1];
  jumperlessConfig.calibration.top_rail_spread = dacSpread[2];
  jumperlessConfig.calibration.bottom_rail_spread = dacSpread[3];
  jumperlessConfig.calibration.dac_0_zero = dacZero[0];
  jumperlessConfig.calibration.dac_1_zero = dacZero[1];
  jumperlessConfig.calibration.top_rail_zero = dacZero[2];
  jumperlessConfig.calibration.bottom_rail_zero = dacZero[3];
  jumperlessConfig.calibration.probe_max = 0.0f;
  jumperlessConfig.calibration.probe_min = 0.0f;

  saveConfig();
  //Serial.println("DAC calibration saved to both EEPROM and config file");
  }

void debugFlagSet(int flag) {
  int flagStatus;
  switch (flag) {
    case 1: {
    flagStatus = EEPROM.read(DEBUG_FILEPARSINGADDRESS);
    if (flagStatus == 0) {
      EEPROM.write(DEBUG_FILEPARSINGADDRESS, 1);

      debugFP = true;
      } else {
      EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);

      debugFP = false;
      }
    jumperlessConfig.debug.file_parsing = debugFP;
    break;
    }

    case 2: {
    flagStatus = EEPROM.read(DEBUG_NETMANAGERADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(DEBUG_NETMANAGERADDRESS, 1);

      debugNM = true;
      } else {
      EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);

      debugNM = false;
      }
    jumperlessConfig.debug.net_manager = debugNM;
    break;
    }

    case 3: {
    flagStatus = EEPROM.read(DEBUG_NETTOCHIPCONNECTIONSADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 1);

      debugNTCC = true;
      } else {
      EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);

      debugNTCC = false;
      }
    jumperlessConfig.debug.nets_to_chips = debugNTCC;
    break;
    }
    case 4: {
    flagStatus = EEPROM.read(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 1);

      debugNTCC2 = true;
      } else {
      EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);

      debugNTCC2 = false;
      }
    jumperlessConfig.debug.nets_to_chips_alt = debugNTCC2;
    break;
    }

    case 5: {
    flagStatus = EEPROM.read(DEBUG_LEDSADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(DEBUG_LEDSADDRESS, 1);

      debugLEDs = true;
      } else {
      EEPROM.write(DEBUG_LEDSADDRESS, 0);

      debugLEDs = false;
      }
    jumperlessConfig.debug.leds = debugLEDs;
    break;
    }


    case 6: {
    flagStatus = EEPROM.read(SHOW_PROBE_CURRENT_ADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 1);

      showProbeCurrent = 1;
      } else {
      EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 0);

      showProbeCurrent = 0;
      }
    //jumperlessConfig.debug_flags.show_probe_current = showProbeCurrent;
    break;
    }

    case 7: {
    if (jumperlessConfig.serial_1.print_passthrough == 0) {
      jumperlessConfig.serial_1.print_passthrough = 2;
      } else if (jumperlessConfig.serial_1.print_passthrough == 1) {
        jumperlessConfig.serial_1.print_passthrough = 0;
        } else if (jumperlessConfig.serial_1.print_passthrough == 2) {
          jumperlessConfig.serial_1.print_passthrough = 1;
          }
        // printSerial1Passthrough = jumperlessConfig.serial.serial_1.print_passthrough;
        break;
    }
    case 8: {
    if (jumperlessConfig.serial_2.print_passthrough == 0) {
      jumperlessConfig.serial_2.print_passthrough = 2;
      } else if (jumperlessConfig.serial_2.print_passthrough == 1) {
        jumperlessConfig.serial_2.print_passthrough = 0;
        } else if (jumperlessConfig.serial_2.print_passthrough == 2) {
          jumperlessConfig.serial_2.print_passthrough = 1;
          }
        //printSerial2Passthrough = jumperlessConfig.serial.serial_2.print_passthrough;
        break;
    }


    case 0: {
    EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);
    EEPROM.write(TIME_FILEPARSINGADDRESS, 0);
    EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);
    EEPROM.write(TIME_NETMANAGERADDRESS, 0);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);
    EEPROM.write(DEBUG_LEDSADDRESS, 0);
    EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 0);

    debugFP = false;
    debugFPtime = false;
    debugNM = false;
    debugNMtime = false;
    debugNTCC = false;
    debugNTCC2 = false;
    debugLEDs = false;
    showProbeCurrent = 0;
    jumperlessConfig.debug.file_parsing = false;
    jumperlessConfig.debug.net_manager = false;
    jumperlessConfig.debug.nets_to_chips = false;
    jumperlessConfig.debug.nets_to_chips_alt = false;
    jumperlessConfig.debug.leds = false;
    //jumperlessConfig.debug_flags.show_probe_current = false;

    break;
    }

    case 9: {
    EEPROM.write(DEBUG_FILEPARSINGADDRESS, 1);
    EEPROM.write(TIME_FILEPARSINGADDRESS, 1);
    EEPROM.write(DEBUG_NETMANAGERADDRESS, 1);
    EEPROM.write(TIME_NETMANAGERADDRESS, 1);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 1);
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 1);
    EEPROM.write(DEBUG_LEDSADDRESS, 1);
    EEPROM.write(SHOW_PROBE_CURRENT_ADDRESS, 1);
    debugFP = true;
    debugFPtime = true;
    debugNM = true;
    debugNMtime = true;
    debugNTCC = true;
    debugNTCC2 = true;
    debugLEDs = true;
    showProbeCurrent = 1;
    jumperlessConfig.debug.file_parsing = true;
    jumperlessConfig.debug.net_manager = true;
    jumperlessConfig.debug.nets_to_chips = true;
    jumperlessConfig.debug.nets_to_chips_alt = true;
    jumperlessConfig.debug.leds = true;
    //jumperlessConfig.debug_flags.show_probe_current = true;
    break;
    }
    case 10: {
        {
        EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);

        rotaryEncoderMode = 0;
        }
        break;
    }
    case 11: {
        {
        EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 1);

        rotaryEncoderMode = 1;
        }
        break;
    }
    case 12: {

    //  EEPROM.write(DISPLAYMODE_ADDRESS, displayMode);
    //jumperlessConfig.display_settings.display_mode = displayMode;


    break;
    }
    case 13:
    {
    EEPROM.write(NETCOLORMODE_ADDRESS, netColorMode);
    //jumperlessConfig.display_settings.net_color_mode = netColorMode;

    }
    break;
    }
  delayMicroseconds(100);
  EEPROM.commit();
  delayMicroseconds(100);
  return;
  }

void saveVoltages(float top, float bot, float dac0, float dac1) {
  //#ifdef EEPROMSTUFF
  // EEPROM.put(TOP_RAIL_ADDRESS0, top);
  // EEPROM.put(BOTTOM_RAIL_ADDRESS0, bot);
  // EEPROM.put(DAC0_ADDRESS0, dac0);
  // EEPROM.put(DAC1_ADDRESS0, dac1);
  // EEPROM.commit();
  // delayMicroseconds(100);
  // //#endif

      // Save to config file
  jumperlessConfig.dacs.top_rail = top;
  jumperlessConfig.dacs.bottom_rail = bot;
  jumperlessConfig.dacs.dac_0 = dac0;
  jumperlessConfig.dacs.dac_1 = dac1;

  configChanged = true;
  // saveConfig();
  }

void saveDuplicateSettings(int forceDefaults) {
// #ifdef EEPROMSTUFF
//   if (forceDefaults == 1) {
//     EEPROM.write(PATH_DUPLICATE_ADDRESS, 2);
//     EEPROM.write(DAC_DUPLICATE_ADDRESS, 0);
//     EEPROM.write(POWER_DUPLICATE_ADDRESS, 3);
//     EEPROM.write(DAC_PRIORITY_ADDRESS, 1);
//     EEPROM.write(POWER_PRIORITY_ADDRESS, 1);
//     EEPROM.commit();
//     delayMicroseconds(100);
//     // return;
//     } else {
//     EEPROM.write(PATH_DUPLICATE_ADDRESS, pathDuplicates);
//     EEPROM.write(DAC_DUPLICATE_ADDRESS, dacDuplicates);
//     EEPROM.write(POWER_DUPLICATE_ADDRESS, powerDuplicates);
//     EEPROM.write(DAC_PRIORITY_ADDRESS, dacPriority);
//     EEPROM.write(POWER_PRIORITY_ADDRESS, powerPriority);
//     EEPROM.commit();
//     delayMicroseconds(100);
//     }
// #endif

  // // Save to config file
  // jumperlessConfig.routing.stack_paths = pathDuplicates;
  // jumperlessConfig.routing.stack_rails = dacDuplicates;
  // jumperlessConfig.routing.stack_dacs = powerDuplicates;
  // jumperlessConfig.routing.rail_priority = dacPriority;

  configChanged = true;

  // saveConfig();
  }

void readVoltages(void) {


  // delay(1000);

  // Serial.println("readVoltages");
  // Serial.println(jumperlessConfig.dacs.top_rail);
  // Serial.println(jumperlessConfig.dacs.bottom_rail);
  // Serial.println(jumperlessConfig.dacs.dac_0);
  // Serial.println(jumperlessConfig.dacs.dac_1);

  // delay(1000);
  // #ifdef EEPROMSTUFF
  // delayMicroseconds(200);
  // EEPROM.get(TOP_RAIL_ADDRESS0, railVoltage[0]);


  // EEPROM.get(BOTTOM_RAIL_ADDRESS0, railVoltage[1]);
  // EEPROM.get(DAC0_ADDRESS0, dacOutput[0]);
  // EEPROM.get(DAC1_ADDRESS0, dacOutput[1]);
  // delayMicroseconds(200);

  // #endif

  // jumperlessConfig.dac_settings.top_rail = railVoltage[0];
  // jumperlessConfig.dac_settings.bottom_rail = railVoltage[1];
  // jumperlessConfig.dac_settings.dac_0 = dacOutput[0];
  // jumperlessConfig.dac_settings.dac_1 = dacOutput[1];

  // int needsInit = 0;
  // if (railVoltage[0] > 8.0f || railVoltage[0] < -8.0f) {//|| (uint32_t)railVoltage[0] == 0x00000000 || (uint32_t)railVoltage[0] == 0xFFFFFFFF) {
  //   Serial.println(railVoltage[0]);

  //   railVoltage[0] = 0.0f;
  //   needsInit = 1;

  //   //Serial.println("rail voltage 0 out of range");
  //   }
  // if (railVoltage[1] > 8.0f || railVoltage[1] < -8.0f) {// || (uint32_t)railVoltage[1] == 0x00000000 || (uint32_t)railVoltage[1] == 0xFFFFFFFF) {
  //   railVoltage[1] = 0.0f;
  //   needsInit = 1;
  //   //Serial.println("rail voltage 1 out of range");
  //   }
  // if (dacOutput[0] > 5.0f || dacOutput[0] < 0.0f) {// || (uint32_t)dacOutput[0] == 0x00000000 || (uint32_t)dacOutput[0] == 0xFFFFFFFF) {
  //   dacOutput[0] = 0.0f;
  //   needsInit = 1;
  //   //Serial.println("dac 0 out of range");
  //   }
  // if (dacOutput[1] > 8.0f || dacOutput[1] < -8.0f) {// || (uint32_t)dacOutput[1] == 0x00000000 || (uint32_t)dacOutput[1] == 0xFFFFFFFF) {
  //   dacOutput[1] = 0.0f;
  //   needsInit = 1;
  //   //Serial.println("dac 1 out of range");
  //   }

  // // Only update config values if they are invalid or zero (fallback to EEPROM)
  // // Otherwise, preserve the config file values
  // if (jumperlessConfig.dacs.top_rail == 0.0f && railVoltage[0] != 0.0f) {
  //   jumperlessConfig.dacs.top_rail = railVoltage[0];
  //   configChanged = true;
  // }
  // if (jumperlessConfig.dacs.bottom_rail == 0.0f && railVoltage[1] != 0.0f) {
  //   jumperlessConfig.dacs.bottom_rail = railVoltage[1];
  //   configChanged = true;
  // }
  // if (jumperlessConfig.dacs.dac_0 == 0.0f && dacOutput[0] != 0.0f) {
  //   jumperlessConfig.dacs.dac_0 = dacOutput[0];
  //   configChanged = true;
  // }
  // if (jumperlessConfig.dacs.dac_1 == 0.0f && dacOutput[1] != 0.0f) {
  //   jumperlessConfig.dacs.dac_1 = dacOutput[1];
  //   configChanged = true;
  // }

  // Don't call saveVoltages() here anymore - it would overwrite config values
  // The config values will be copied to legacy variables in readSettingsFromConfig()

//Serial.println(sizeof(float));

  // Serial.print("top rail: ");
  // Serial.println((float)railVoltage[0]);
  // Serial.print("bot rail: ");
  // Serial.println(railVoltage[1],BIN);
  // Serial.print("dac0: ");
  // Serial.println(dacOutput[0],BIN);
  // Serial.print("dac1: ");
  // Serial.println(dacOutput[1],BIN);


// setTopRail(railVoltage[0]);
// setBotRail(railVoltage[1]);
// setDac0_5Vvoltage(dacOutput[0]);
// setDac1_8Vvoltage(dacOutput[1]);
  return;
  }

void saveLogoBindings(void) {
#ifdef EEPROMSTUFF
  EEPROM.put(LOGO_TOP_ADDRESS0, logoTopSetting[0]);
  EEPROM.put(LOGO_TOP_ADDRESS1, logoTopSetting[1]);
  EEPROM.put(LOGO_BOTTOM_ADDRESS0, logoBottomSetting[0]);
  EEPROM.put(LOGO_BOTTOM_ADDRESS1, logoBottomSetting[1]);
  EEPROM.put(BUILDING_TOP_ADDRESS0, buildingTopSetting[0]);
  EEPROM.put(BUILDING_TOP_ADDRESS1, buildingTopSetting[1]);
  EEPROM.put(BUILDING_BOTTOM_ADDRESS0, buildingBottomSetting[0]);
  EEPROM.put(BUILDING_BOTTOM_ADDRESS1, buildingBottomSetting[1]);
  EEPROM.commit();
  delayMicroseconds(100);
#endif

  // Save to config file
  jumperlessConfig.logo_pads.top_guy = logoTopSetting[0];
  jumperlessConfig.logo_pads.bottom_guy = logoBottomSetting[0];
  jumperlessConfig.logo_pads.building_pad_top = buildingTopSetting[0];
  jumperlessConfig.logo_pads.building_pad_bottom = buildingBottomSetting[0];

  configChanged = true;

  //  saveConfig();
  }

void readLogoBindings(void) {
  EEPROM.get(LOGO_TOP_ADDRESS0, logoTopSetting[0]);

  EEPROM.get(LOGO_TOP_ADDRESS1, logoTopSetting[1]);
  if (logoTopSetting[0] == 2) {
    //gpioState[logoTopSetting[1]] = 0;
    }
  EEPROM.get(LOGO_BOTTOM_ADDRESS0, logoBottomSetting[0]);
  EEPROM.get(LOGO_BOTTOM_ADDRESS1, logoBottomSetting[1]);
  if (logoBottomSetting[0] == 2) {
    // gpioState[logoBottomSetting[1]] = 0;
    }
  EEPROM.get(BUILDING_TOP_ADDRESS0, buildingTopSetting[0]);
  EEPROM.get(BUILDING_TOP_ADDRESS1, buildingTopSetting[1]);

  if (buildingTopSetting[0] == 2) {
    //gpioState[buildingTopSetting[1]] = 0;
    }
  EEPROM.get(BUILDING_BOTTOM_ADDRESS0, buildingBottomSetting[0]);
  EEPROM.get(BUILDING_BOTTOM_ADDRESS1, buildingBottomSetting[1]);
  if (buildingBottomSetting[0] == 2) {
    //gpioState[buildingBottomSetting[1]] = 0;
    }
  return;
  }

void saveLEDbrightness(int forceDefaults) {
#ifdef EEPROMSTUFF
  if (forceDefaults == 1) {
    LEDbrightness = DEFAULTBRIGHTNESS;
    LEDbrightnessRail = DEFAULTRAILBRIGHTNESS;
    LEDbrightnessSpecial = DEFAULTSPECIALNETBRIGHTNESS;
    menuBrightnessSetting = 0;
    }

  EEPROM.write(MENUBRIGHTNESS_ADDRESS, menuBrightnessSetting + 100);
  EEPROM.write(LEDBRIGHTNESSADDRESS, LEDbrightness);
  EEPROM.write(RAILBRIGHTNESSADDRESS, LEDbrightnessRail);
  EEPROM.write(SPECIALBRIGHTNESSADDRESS, LEDbrightnessSpecial);
  EEPROM.commit();
  delayMicroseconds(100);
#endif

  // Save to config file
  jumperlessConfig.display.led_brightness = LEDbrightness;
  jumperlessConfig.display.rail_brightness = LEDbrightnessRail;
  jumperlessConfig.display.special_net_brightness = LEDbrightnessSpecial;
  jumperlessConfig.display.menu_brightness = menuBrightnessSetting;

  configChanged = true;
  //saveConfig();
  }


void updateStateFromGPIOConfig(void) {
  for (int i = 0; i < 10; i++) {  // Changed from 8 to 10 to include UART pins
    // Map gpioState to direction and pull settings

    int gpio_pin = gpioDef[i][0];  // Map GPIO 0-7 to pins 20-27
    if (gpio_function_map[i] == GPIO_FUNC_SIO) {

      switch (jumperlessConfig.gpio.direction[i]) {
        case 0: // output low
          gpioState[i] = 0;
          gpio_set_dir(gpio_pin, true);  // Set as output
          gpio_set_pulls(gpio_pin, false, false);  // No pulls
          // gpio_put(gpio_pin, 0);
          break;
        case 1: // output high
          switch (jumperlessConfig.gpio.pulls[i]) {
            case 0: // pulldown
              gpioState[i] = 4;
              gpio_set_dir(gpio_pin, false);  // Set as input
              gpio_set_pulls(gpio_pin, false, true);  // Pull down
              break;
            case 1: // pullup
              gpioState[i] = 3;
              gpio_set_dir(gpio_pin, false);  // Set as input
              gpio_set_pulls(gpio_pin, true, false);  // Pull up
              break;
            case 2: // no pull
              gpioState[i] = 2;
              gpio_set_dir(gpio_pin, false);  // Set as input
              gpio_set_pulls(gpio_pin, false, false);  // No pulls
              break;

            }
          break;

        }

      break;
      }



    }
  }

void updateGPIOConfigFromState(void) {
  // Serial.println("updateGPIOConfigFromState");
  // Serial.flush();
  // return;
  int changed = 0;
  for (int i = 0; i < 10; i++) {  // Changed from 8 to 10 to include UART pins
    // Map gpioState to direction and pull settings

    int gpio_pin = gpioDef[i][0];  // Map GPIO 0-7 to pins 20-27

    if (gpio_function_map[i] == GPIO_FUNC_SIO) {

      switch (gpioState[i]) {
        case 0: // output low
          if (jumperlessConfig.gpio.direction[i] != 0 || jumperlessConfig.gpio.pulls[i] != 2) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 0; // output
          jumperlessConfig.gpio.pulls[i] = 2; // no pull
          gpio_set_dir(gpio_pin, true);  // Set as output
          gpio_set_pulls(gpio_pin, false, false);  // No pulls
          break;
        case 1: // output high
          if (jumperlessConfig.gpio.direction[i] != 0 || jumperlessConfig.gpio.pulls[i] != 2) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 0; // output
          jumperlessConfig.gpio.pulls[i] = 2;
          gpio_set_dir(gpio_pin, true);  // Set as output
          gpio_set_pulls(gpio_pin, false, false);  // No pulls
          break;
        case 2: // input
          if (jumperlessConfig.gpio.direction[i] != 1 || jumperlessConfig.gpio.pulls[i] != 2) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 1; // input
          jumperlessConfig.gpio.pulls[i] = 2; // no pull
          gpio_set_dir(gpio_pin, false);  // Set as input
          gpio_set_pulls(gpio_pin, false, false);  // No pulls
          break;
        case 3: // input pullup
          if (jumperlessConfig.gpio.direction[i] != 1 || jumperlessConfig.gpio.pulls[i] != 1) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 1; // input
          jumperlessConfig.gpio.pulls[i] = 1; // pullup
          gpio_set_dir(gpio_pin, false);  // Set as input
          gpio_set_pulls(gpio_pin, true, false);  // Pull up
          break;
        case 4: // input pulldown
          if (jumperlessConfig.gpio.direction[i] != 1 || jumperlessConfig.gpio.pulls[i] != 0) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 1; // input
          jumperlessConfig.gpio.pulls[i] = 0; // pulldown
          gpio_set_dir(gpio_pin, false);  // Set as input
          gpio_set_pulls(gpio_pin, false, true);  // Pull down
          break;
        case 5: // unknown
          if (jumperlessConfig.gpio.direction[i] != 1 || jumperlessConfig.gpio.pulls[i] != 0) {
            changed = 1;
            }
          jumperlessConfig.gpio.direction[i] = 1; // default to input
          jumperlessConfig.gpio.pulls[i] = 0; // default to pulldown
          gpio_set_dir(gpio_pin, false);  // Set as input
          gpio_set_pulls(gpio_pin, false, true);  // Pull down
          break;
        case 6: // do nothing
          break;
        }
      }
    }


  if (changed == 1) {
    configChanged = true; // Mark config as changed so it will be saved
    }

  }

void runCommandAfterReset(char command) {
  if (EEPROM.read(CLEARBEFORECOMMANDADDRESS) == 1) {
    return;
    } else {

    EEPROM.write(CLEARBEFORECOMMANDADDRESS, 1);
    EEPROM.write(LASTCOMMANDADDRESS, command);
    EEPROM.commit();

    digitalWrite(RESETPIN, HIGH);
    delay(1);
    digitalWrite(RESETPIN, LOW);

    AIRCR_Register = 0x5FA0004; // hard reset
    }
  }

char lastCommandRead(void) {

  Serial.print("last command: ");

  Serial.println((char)EEPROM.read(LASTCOMMANDADDRESS));

  return EEPROM.read(LASTCOMMANDADDRESS);
  }
void lastCommandWrite(char lastCommand) {

  EEPROM.write(LASTCOMMANDADDRESS, lastCommand);
  }

void readSettingsFromConfig() {
  // Debug flags
  debugFP = jumperlessConfig.debug.file_parsing;
  //debugFPtime = jumperlessConfig.debug_flags.file_parsing_time;
  debugNM = jumperlessConfig.debug.net_manager;
  //debugNMtime = jumperlessConfig.debug_flags.net_manager_time;
  debugNTCC = jumperlessConfig.debug.nets_to_chips;
  debugNTCC2 = jumperlessConfig.debug.nets_to_chips_alt;
  debugLEDs = jumperlessConfig.debug.leds;
  // showProbeCurrent = jumperlessConfig.debug_flags.show_probe_current;

  // Display settings
  LEDbrightness = jumperlessConfig.display.led_brightness;
  LEDbrightnessRail = jumperlessConfig.display.rail_brightness;
  LEDbrightnessSpecial = jumperlessConfig.display.special_net_brightness;
  menuBrightnessSetting = jumperlessConfig.display.menu_brightness;
  netColorMode = jumperlessConfig.display.net_color_mode;

  // // Routing settings
  // pathDuplicates = jumperlessConfig.routing.stack_paths;
  // powerDuplicates = jumperlessConfig.routing.stack_rails;  // powerDuplicates is used for rail stacking
  // dacDuplicates = jumperlessConfig.routing.stack_dacs;    // dacDuplicates is used for DAC stacking
  // dacPriority = jumperlessConfig.routing.rail_priority;

  // DAC calibration
  dacSpread[0] = jumperlessConfig.calibration.dac_0_spread;
  dacSpread[1] = jumperlessConfig.calibration.dac_1_spread;
  dacSpread[2] = jumperlessConfig.calibration.top_rail_spread;
  dacSpread[3] = jumperlessConfig.calibration.bottom_rail_spread;

  dacZero[0] = jumperlessConfig.calibration.dac_0_zero;
  dacZero[1] = jumperlessConfig.calibration.dac_1_zero;
  dacZero[2] = jumperlessConfig.calibration.top_rail_zero;
  dacZero[3] = jumperlessConfig.calibration.bottom_rail_zero;


  // DAC voltages
  railVoltage[0] = jumperlessConfig.dacs.top_rail;
  railVoltage[1] = jumperlessConfig.dacs.bottom_rail;
  dacOutput[0] = jumperlessConfig.dacs.dac_0;
  dacOutput[1] = jumperlessConfig.dacs.dac_1;

  // Serial.print("railVoltage[0]: ");
  // Serial.println(railVoltage[0]);
  // Serial.print("railVoltage[1]: ");
  // Serial.println(railVoltage[1]);
  // Serial.print("dacOutput[0]: ");
  // Serial.println(dacOutput[0]);
  // Serial.print("dacOutput[1]: ");
  // Serial.println(dacOutput[1]);

  probePowerDAC = jumperlessConfig.dacs.probe_power_dac;

  //GPIO settings
  for (int i = 0; i < 10; i++) {  // Changed from 8 to 10 to include UART pins

    // Combine direction and pull settings into a single value
    // 0 = output low, 1 = output high, 2 = input, 3 = input pullup, 4 = input pulldown

    int gpio_pin = gpioDef[i][0];
    // if (i == 8) {
    //   gpio_pin = 0; // UART TX
    //   } else if (i == 9) {
    //     gpio_pin = 1; // UART RX
    //     }

   
   // gpio_init(gpio_pin);

   // if (gpio_get_function(gpio_pin) == GPIO_FUNC_SIO) {
    if (jumperlessConfig.gpio.direction[i] == 0) { // output
      //gpioState[i] = jumperlessConfig.gpio.pulls[i] ? 1 : 0; // 1 for high, 0 for low
      gpio_set_dir(gpio_pin, true);
      gpioState[i] = 0;
      // Serial.print("gpio_pin: ");
      // Serial.print(gpio_pin);
      // Serial.print(" gpioState[i]: ");
      // Serial.print(gpioState[i]);
      // Serial.print(" actual: ");
      // Serial.println(gpio_get_dir(gpio_pin));
      //Serial.flush();
      } else if (jumperlessConfig.gpio.direction[i] == 1) { // input
        gpio_set_dir(gpio_pin, false);
        gpioState[i] = 2;
        //         Serial.print("gpio_pin: ");
        // Serial.print(gpio_pin);
        // Serial.print(" gpioState[i]: ");
        // Serial.print(gpioState[i]);
        // Serial.print(" actual: ");
        // Serial.println(gpio_get_dir(gpio_pin));
        // Serial.flush();
        if (jumperlessConfig.gpio.pulls[i] == 2) { // no pull
          //  gpioState[i] = 2;
          gpio_set_pulls(gpio_pin, false, false);
          } else if (jumperlessConfig.gpio.pulls[i] == 1) { // pullup
            gpioState[i] = 3;
            gpio_set_pulls(gpio_pin, true, false);
            } else if (jumperlessConfig.gpio.pulls[i] == 0) { // pulldown
              gpioState[i] = 4;
              gpio_set_pulls(gpio_pin, false, true);
              } else {
              gpioState[i] = 5; // unknown
              gpio_set_pulls(gpio_pin, false, false);
              }
        } else {
        gpioState[i] = 5; // unknown
        // gpio_set_dir(gpio_pin, false);
        // gpio_set_pulls(gpio_pin, false, false);
        }
     // }
    }

  // Serial
  baudRateUSBSer1 = jumperlessConfig.serial_1.baud_rate;
  baudRateUSBSer2 = jumperlessConfig.serial_2.baud_rate;
  printSerial1Passthrough = jumperlessConfig.serial_1.print_passthrough;
  printSerial2Passthrough = jumperlessConfig.serial_2.print_passthrough;
  connectOnBoot1 = jumperlessConfig.serial_1.connect_on_boot;
  connectOnBoot2 = jumperlessConfig.serial_2.connect_on_boot;
  lockConnection1 = jumperlessConfig.serial_1.lock_connection;
  lockConnection2 = jumperlessConfig.serial_2.lock_connection;





  }
/// 0 = output low, 1 = output high, 2 = input, 3 = input pullup, 4 = input pulldown, 5 = unknown