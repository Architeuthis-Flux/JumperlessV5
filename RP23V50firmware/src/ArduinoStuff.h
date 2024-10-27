// SPDX-License-Identifier: MIT
#ifndef ARDUINOSTUFF_H
#define ARDUINOSTUFF_H

#include <Arduino.h>



extern int dtrFire;
extern uint8_t numbits;
extern uint8_t paritytype;
extern uint8_t stopbits;
extern int baudRate;

#ifdef USE_TINYUSB
extern Adafruit_USBD_CDC USBSer1;
#endif


void initArduino(void);

void initSecondSerial(void);

void checkForConfigChanges(void);
void secondSerialHandler(void);
uint16_t makeSerialConfig(uint8_t numbits = 8, uint8_t paritytype = 0,
                          uint8_t stopbits = 1);
void arduinoPrint(void);
void uploadArduino(void);
void setBaudRate(int baudRate);







#endif
