// SPDX-License-Identifier: MIT
#ifndef ARDUINOSTUFF_H
#define ARDUINOSTUFF_H

#include <Arduino.h>


extern int debugArduino;
extern volatile bool flashingArduino; 


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

extern unsigned long lastPortInquiry;

#ifdef USE_TINYUSB
#include "usb_interface_config.h"

// #if USB_CDC_ENABLE_COUNT >= 2
extern Adafruit_USBD_CDC USBSer1;
// #endif

// #if USB_CDC_ENABLE_COUNT >= 3
extern Adafruit_USBD_CDC USBSer2;
// #endif

// #if USB_CDC_ENABLE_COUNT >= 4
extern Adafruit_USBD_CDC USBSer3;
// #endif

#endif




extern volatile int arduinoInReset;
extern volatile bool flashingArduino;


extern char arduinoCommandStrings[10][50];
extern char serialCommandBuffer[512];
extern volatile int serialCommandBufferIndex;
extern volatile int serialCommandReady;

int checkForArduinoCommands(char serialBuffer[], int serialBufferIndex);
void initArduino(void);

void initSecondSerial(void);
uint16_t getSerial1Config(void);
uint16_t getSerial2Config(void);

void checkForConfigChangesUSBSer1(int print = 0);
void checkForConfigChangesUSBSer2(int print = 0);
int secondSerialHandler(void);
bool checkForDTRpulse(void);
bool checkForSync(void);
uint16_t makeSerialConfig(uint8_t numbits = 8, uint8_t paritytype = 0,
                          uint8_t stopbits = 1);
void arduinoPrint(void);
void uploadArduino(void);
void setBaudRate(int baudRate);
void ESPReset(void);
void SetArduinoResetLine(bool state, int topBottomBoth = 2);
void flashArduino(unsigned long timeout);

void resetArduino(int topBottomBoth = 2, unsigned long holdMicroseconds = 8500);
void connectArduino(int flashOrLocal = 1, int refreshConnections = 1);
void disconnectArduino(int flashOrLocal =  1);
int checkIfArduinoIsConnected(void);

int handleSerialPassthrough(int serial = 2, int print = 0, int printPassthroughFlashing = 0, int checkForCommands = 1);

void printMicrosPerByte(void);
void printUSBInterfaceNames(void);
void replyWithSerialInfo(void);

#endif
