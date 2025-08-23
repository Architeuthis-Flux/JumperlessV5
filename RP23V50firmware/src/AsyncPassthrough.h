#ifndef AsyncPassthrough_h
#define AsyncPassthrough_h

#include "Arduino.h"

#define ASYNC_PASSTHROUGH_ENABLED 1

extern bool asyncPassthroughEnabled;
extern unsigned long microsPerByteSerial1;
extern unsigned long serial1baud;

#if ASYNC_PASSTHROUGH_ENABLED == 1
namespace AsyncPassthrough {
    // Initialize the CDC1 <-> UART passthrough (pico-sdk uart with HW FIFO)
    void begin(unsigned long baud = 115200);

    // Call frequently from loop() to move data in both directions and
    // apply any pending line-coding changes safely outside ISRs
    void task();
}

#endif

#endif