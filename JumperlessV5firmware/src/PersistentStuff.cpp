
#include "PersistentStuff.h"
#include "FileParsing.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "NetManager.h"
#include "Probing.h"
#include "Peripherals.h"
#include <EEPROM.h>

void debugFlagInit(int forceDefaults) {

  if (EEPROM.read(FIRSTSTARTUPADDRESS) == 255 || forceDefaults == 1) {
    EEPROM.write(FIRSTSTARTUPADDRESS, 0);
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
    EEPROM.write(PROBESWAPADDRESS, 0);
    EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);
    EEPROM.write(DISPLAYMODE_ADDRESS, 0);
    saveVoltages(0.0f, 0.0f, 0.0f, 0.0f);

    EEPROM.commit();
    delay(5);
  }


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

  displayMode = EEPROM.read(DISPLAYMODE_ADDRESS);

  probeSwap = EEPROM.read(PROBESWAPADDRESS);



//   debugFP = 1;
//   debugFPtime = 1;

//   debugNM = 1;
//   debugNMtime = 1;

//   debugNTCC = 1;
//   debugNTCC2 = 1;

  // debugLEDs = 1;


  if (debugFP != 0 && debugFP != 1)
    EEPROM.write(DEBUG_FILEPARSINGADDRESS, 0);

  if (debugFPtime != 0 && debugFPtime != 1)
    EEPROM.write(TIME_FILEPARSINGADDRESS, 0);

  if (debugNM != 0 && debugNM != 1)
    EEPROM.write(DEBUG_NETMANAGERADDRESS, 0);

  if (debugNMtime != 0 && debugNMtime != 1)
    EEPROM.write(TIME_NETMANAGERADDRESS, 0);

  if (debugNTCC != 0 && debugNTCC != 1)
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSADDRESS, 0);

  if (debugNTCC2 != 0 && debugNTCC2 != 1)
    EEPROM.write(DEBUG_NETTOCHIPCONNECTIONSALTADDRESS, 0);

  if (debugLEDs != 0 && debugLEDs != 1)
    EEPROM.write(DEBUG_LEDSADDRESS, 0);

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
  if (rotaryEncoderMode != 0 && rotaryEncoderMode != 1) {
    EEPROM.write(ROTARYENCODER_MODE_ADDRESS, 0);
    rotaryEncoderMode = 0;
  }
  if (displayMode != 0 && displayMode != 1) {
    EEPROM.write(DISPLAYMODE_ADDRESS, 0);
    displayMode = 0;
  }


readVoltages();
readLogoBindings();
  EEPROM.commit();
  delay(5);
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
    break;
  }

  case 6: {
    flagStatus = EEPROM.read(PROBESWAPADDRESS);

    if (flagStatus == 0) {
      EEPROM.write(PROBESWAPADDRESS, 1);

      probeSwap = true;
    } else {
      EEPROM.write(PROBESWAPADDRESS, 0);

      probeSwap = false;
    }
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

    debugFP = false;
    debugFPtime = false;
    debugNM = false;
    debugNMtime = false;
    debugNTCC = false;
    debugNTCC2 = false;
    debugLEDs = false;

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
    debugFP = true;
    debugFPtime = true;
    debugNM = true;
    debugNMtime = true;
    debugNTCC = true;
    debugNTCC2 = true;
    debugLEDs = true;
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
    
      EEPROM.write(DISPLAYMODE_ADDRESS, displayMode);

      
    
    break;
  }
  }
  delay(4);
  EEPROM.commit();
  delay(8);
  return;
}

void saveVoltages(float top, float bot, float dac0, float dac1) {
  EEPROM.put(TOP_RAIL_ADDRESS0, top);
  EEPROM.put(BOTTOM_RAIL_ADDRESS0, bot);
  EEPROM.put(DAC0_ADDRESS0, dac0);
  EEPROM.put(DAC1_ADDRESS0, dac1);
  EEPROM.commit();
  delay(2);


}

void readVoltages(void) {




   EEPROM.get(TOP_RAIL_ADDRESS0, railVoltage[0]);
   EEPROM.get(BOTTOM_RAIL_ADDRESS0, railVoltage[1]);
   EEPROM.get(DAC0_ADDRESS0, dacOutput[0]);
   EEPROM.get(DAC1_ADDRESS0, dacOutput[1]);
   delay(2);

int needsInit = 0;
if (railVoltage[0] > 8.0f || railVoltage[0] < -8.0f) {
    railVoltage[0] = 0.0f;
    needsInit = 1;
    Serial.println("rail voltage 0 out of range");
  }
  if (railVoltage[1] > 8.0f || railVoltage[1] < -8.0f) {
    railVoltage[1] = 0.0f;
    needsInit = 1;
    Serial.println("rail voltage 1 out of range");
  }
  if (dacOutput[0] > 5.0f || dacOutput[0] < 0.0f) {
    dacOutput[0] = 0.0f;
    needsInit = 1;
    Serial.println("dac 0 out of range");
  }
  if (dacOutput[1] > 8.0f || dacOutput[1] < -8.0f) {
    dacOutput[1] = 0.0f;
    needsInit = 1;
    Serial.println("dac 1 out of range");
  }
saveVoltages(railVoltage[0],railVoltage[1],dacOutput[0],dacOutput[1]);

//Serial.println(sizeof(float));

  Serial.print("top rail: ");
  Serial.println((float)railVoltage[0]);
  Serial.print("bot rail: ");
  Serial.println(railVoltage[1],BIN);
  Serial.print("dac0: ");
  Serial.println(dacOutput[0],BIN);
  Serial.print("dac1: ");
  Serial.println(dacOutput[1],BIN);


// setTopRail(railVoltage[0]);
// setBotRail(railVoltage[1]);
// setDac0_5Vvoltage(dacOutput[0]);
// setDac1_8Vvoltage(dacOutput[1]);
  return;
}

void saveLogoBindings(void) {
  EEPROM.put(LOGO_TOP_ADDRESS0, logoTopSetting[0]);
  EEPROM.put(LOGO_TOP_ADDRESS1, logoTopSetting[1]);
  EEPROM.put(LOGO_BOTTOM_ADDRESS0, logoBottomSetting[0]);
  EEPROM.put(LOGO_BOTTOM_ADDRESS1, logoBottomSetting[1]);
  EEPROM.put(BUILDING_TOP_ADDRESS0, buildingTopSetting[0]);
  EEPROM.put(BUILDING_TOP_ADDRESS1, buildingTopSetting[1]);
  EEPROM.put(BUILDING_BOTTOM_ADDRESS0, buildingBottomSetting[0]);
  EEPROM.put(BUILDING_BOTTOM_ADDRESS1, buildingBottomSetting[1]);
  EEPROM.commit();
  delay(2);
}

void readLogoBindings(void) {
  EEPROM.get(LOGO_TOP_ADDRESS0, logoTopSetting[0]);

  EEPROM.get(LOGO_TOP_ADDRESS1, logoTopSetting[1]);
  if (logoTopSetting[0] == 2) {
    gpioState[logoTopSetting[1]] = 0;
  }
  EEPROM.get(LOGO_BOTTOM_ADDRESS0, logoBottomSetting[0]);
  EEPROM.get(LOGO_BOTTOM_ADDRESS1, logoBottomSetting[1]);
  if (logoBottomSetting[0] == 2) {
    gpioState[logoBottomSetting[1]] = 0;
  }
  EEPROM.get(BUILDING_TOP_ADDRESS0, buildingTopSetting[0]);
  EEPROM.get(BUILDING_TOP_ADDRESS1, buildingTopSetting[1]);

  if (buildingTopSetting[0] == 2) {
    gpioState[buildingTopSetting[1]] = 0;
  }
  EEPROM.get(BUILDING_BOTTOM_ADDRESS0, buildingBottomSetting[0]);
  EEPROM.get(BUILDING_BOTTOM_ADDRESS1, buildingBottomSetting[1]);
  if (buildingBottomSetting[0] == 2) {
    gpioState[buildingBottomSetting[1]] = 0;
  }
  return;
} 

void saveLEDbrightness(int forceDefaults) {
  if (forceDefaults == 1) {
    LEDbrightness = DEFAULTBRIGHTNESS;
    LEDbrightnessRail = DEFAULTRAILBRIGHTNESS;
    LEDbrightnessSpecial = DEFAULTSPECIALNETBRIGHTNESS;
  }
  
    EEPROM.write(LEDBRIGHTNESSADDRESS, LEDbrightness);
    EEPROM.write(RAILBRIGHTNESSADDRESS, LEDbrightnessRail);
    EEPROM.write(SPECIALBRIGHTNESSADDRESS, LEDbrightnessSpecial);
    EEPROM.commit();
    delay(2);
  
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
