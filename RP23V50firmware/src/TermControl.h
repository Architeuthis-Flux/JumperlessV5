#ifndef TERMCONTROL_H
#define TERMCONTROL_H

#include <Arduino.h>
#include "SyntaxHighlighting.h"

// Forward declarations
class ScriptHistory;

#define TERMCONTROL_MAX_LINE_LENGTH 512
#define TERMCONTROL_NEWLINE_OUT "\r\n"

/**
 * TermControl - A simplified drop-in replacement for Serial that provides:
 * - Line buffering with editing capabilities
 * - ANSI escape sequence handling (arrows, backspace, etc.)
 * - History management via ScriptHistory
 * - Standard Stream interface for compatibility
 */
class TermControl : public Stream {
public:
    TermControl(Stream* underlying_stream, bool create_own_history = true);
    ~TermControl();

    // Standard Stream interface - drop-in replacement for Serial
    virtual int available() override;
    virtual int read() override;
    virtual int peek() override;
    virtual void flush() override;
    virtual size_t write(uint8_t byte) override;
    virtual size_t write(const uint8_t *buffer, size_t size) override;

    // Terminal-specific functionality
    bool service();                     // Call this in main loop to handle input - returns true when line ready
    bool hasCompletedLine() const;      // True when a complete line is ready
    String getCompletedLine();          // Get the completed line (consumes it)
    String peekCompletedLine() const;   // Get the completed line without consuming it
    void clearCompletedLine();          // Clear completed line without consuming
    const char* getCurrentLineBuffer(); // Get current line being edited (read-only)
    void setPrompt(const char* prompt); // Set prompt string
    void enableEcho(bool enabled);      // Enable/disable character echoing
    ScriptHistory* getHistory() { return history; } // Get access to history instance
    void setColoredPrompt(const char* prompt, int color_code = 79); // Set colored prompt
    bool hasInput() const { return hasCompletedLine(); } // Alias for compatibility with other code

private:
    Stream* stream;                     // Underlying stream (usually Serial)
    ScriptHistory* history;             // History manager
    bool owns_history;                  // True if we created the history instance
    
    // Current line being edited
    char current_line[TERMCONTROL_MAX_LINE_LENGTH];      // Clean input buffer (no ANSI codes)
    char display_buffer[TERMCONTROL_MAX_LINE_LENGTH * 4]; // Display buffer with ANSI codes
    int display_length;
    int line_length;
    int cursor_position;
    
    // Completed line ready for external parsing
    String completed_line;
    bool line_ready;
    
    // Settings
    String prompt_text;
    bool echo_enabled;
    SyntaxHighlighting syntax_highlighter;
    
    // ANSI escape sequence state
    enum AnsiState {
        ANSI_NORMAL,
        ANSI_ESCAPE,
        ANSI_BRACKET
    } ansi_state;
    
    // Internal methods
    void handleNormalChar(char c);
    void handleBackspace();
    void handleEnter();
    void handleArrowUp();
    void handleArrowDown();
    void handleArrowLeft();
    void handleArrowRight();
    void handleCtrlU();                 // Clear line
    void handleTab();                   // Could be used for completion later
    
    void renderCurrentLine();
    void clearCurrentLine();
    void moveCursorTo(int position);
    void moveCursorToPosition(int position); // New cursor positioning using display_length
    void insertCharAtCursor(char c);
    void deleteCharAtCursor();
    int calculateVisualLength(const String& text); // Calculate length excluding ANSI codes
};

#endif