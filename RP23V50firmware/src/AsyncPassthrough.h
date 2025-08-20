#ifndef AsyncPassthrough_h
#define AsyncPassthrough_h

#include "Arduino.h"

#define ASYNC_PASSTHROUGH_ENABLED 1

extern bool asyncPassthroughEnabled;

#if ASYNC_PASSTHROUGH_ENABLED == 1
namespace AsyncPassthrough {
    // Initialize the CDC1 <-> Serial1 passthrough. Optional: set pins/baud first
    static inline void begin(unsigned long baud = 115200) {
        // Ensure Serial1 is started; TinyUSB CDC is initialized by core
        if (!Serial1) {
            Serial1.begin(baud);
        }
    }

    // Call frequently from loop() to move data in both directions and
    // apply any pending line-coding changes safely outside ISRs
    void task();
}

#endif

#endif