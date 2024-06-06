
#include "PersistentStuff.h"
#include "FileParsing.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "NetManager.h"
#include "Probing.h"

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
  }
  delay(4);
  EEPROM.commit();
  delay(8);
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
