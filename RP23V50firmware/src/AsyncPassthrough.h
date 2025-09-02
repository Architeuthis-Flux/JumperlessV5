#ifndef AsyncPassthrough_h
#define AsyncPassthrough_h

#include "Arduino.h"

#define ASYNC_PASSTHROUGH_ENABLED 1

extern bool asyncPassthroughEnabled;
extern unsigned long microsPerByteSerial1;
extern unsigned long serial1baud;
extern volatile bool s_line_coding_override;

#if ASYNC_PASSTHROUGH_ENABLED == 1
namespace AsyncPassthrough {
    // Initialize the CDC1 <-> UART passthrough (pico-sdk uart with HW FIFO)
    void begin(unsigned long baud = 115200);

    // Call frequently from loop() to move data in both directions and
    // apply any pending line-coding changes safely outside ISRs
    void task();

    // Expose UART received ring for other modules
    extern uint8_t uartReceived[4096];
    extern volatile uint16_t uartReceivedHead;
    extern volatile uint16_t uartReceivedTail;

    // Register command prefixes to forward UART payload to main Serial
    // Returns true on success, false if registry full
    bool registerForwardPrefix(const char* prefix);

    // Remove a previously-registered prefix (exact string match)
    bool unregisterForwardPrefix(const char* prefix);

    // List current prefixes; returns number written into out (may be truncated)
    size_t listForwardPrefixes(const char** out, size_t max);

    // Register/remove/list end tokens that terminate forwarding sessions
    bool registerForwardEnd(const char* token);
    bool unregisterForwardEnd(const char* token);
    size_t listForwardEnds(const char** out, size_t max);

    // Control whether newline (\n or \r) also ends forwarding (default true)
    void setForwardEndOnNewline(bool enable);
    
    // Apply a new UART line coding immediately (baud/data/parity/stop)
    // Keeps passthrough active while updating hardware and timing
    void applyLineCodingOverride(uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);
}

#endif

// Interop C hooks to coordinate UART0 ownership with MicroPython
// Only effective when JL_UART0_INTEROP_MODE == 1
#ifdef __cplusplus
extern "C" {
#endif
void jl_asyncpassthrough_suspend_uart0( void );
void jl_asyncpassthrough_resume_uart0( void );

// Allow MicroPython to update UART0 line coding while integrating with passthrough
void jl_asyncpassthrough_override_line_coding( uint32_t baud, uint8_t data_bits, uint8_t parity, uint8_t stop_bits );
#ifdef __cplusplus
}
#endif

#endif