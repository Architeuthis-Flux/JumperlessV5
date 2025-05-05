// SPDX-License-Identifier: MIT
#ifndef ARDUINOSTUFF_H
#define ARDUINOSTUFF_H

#include <Arduino.h>

extern int connectOnBoot1;
extern int connectOnBoot2;
extern int lockConnection1;
extern int lockConnection2;

extern int printSerial1Passthrough;
extern int printSerial2Passthrough;

extern int USBSer1Available;
extern int Serial1Available;

extern unsigned long microsPerByteSerial1;
extern unsigned long microsPerByteSerial2;

extern bool ManualArduinoReset;
extern uint8_t numbitsUSBSer1, numbitsUSBSer2;
extern uint8_t paritytypeUSBSer1, paritytypeUSBSer2;
extern uint8_t stopbitsUSBSer1, stopbitsUSBSer2;
extern int baudRateUSBSer1, baudRateUSBSer2;
extern volatile int backpowered;
#ifdef USE_TINYUSB
extern Adafruit_USBD_CDC USBSer1;
extern Adafruit_USBD_CDC USBSer2;
#endif
extern volatile int arduinoInReset;

void initArduino(void);

void initSecondSerial(void);
uint16_t getSerial1Config(void);
uint16_t getSerial2Config(void);

void checkForConfigChangesUSBSer1(int print = 0);
void checkForConfigChangesUSBSer2(int print = 0);
void secondSerialHandler(void);
uint16_t makeSerialConfig(uint8_t numbits = 8, uint8_t paritytype = 0,
                          uint8_t stopbits = 1);
void arduinoPrint(void);
void uploadArduino(void);
void setBaudRate(int baudRate);
void ESPReset(void);
void SetArduinoResetLine(bool state, int topBottomBoth = 2);
void flashArduino(unsigned long timeout);

void resetArduino(int topBottomBoth = 2);
void connectArduino(int flashOrLocal = 1);
void disconnectArduino(int flashOrLocal =  1);
int checkIfArduinoIsConnected(void);

int handleSerialPassthrough(int serial = 2, int print = 0, int printPassthroughFlashing = 0);

void printMicrosPerByte(void);

#endif
