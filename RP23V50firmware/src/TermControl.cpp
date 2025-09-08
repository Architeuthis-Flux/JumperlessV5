#include "TermControl.h"
#include "Python_Proper.h"  // For ScriptHistory

TermControl::TermControl(Stream* underlying_stream, bool create_own_history)
    : stream(underlying_stream), 
      history(nullptr),
      owns_history(create_own_history),
      line_length(0),
      cursor_position(0),
      line_ready(false),
      echo_enabled(true),
      syntax_highlighter(underlying_stream),
      ansi_state(ANSI_NORMAL) {
    
    // Create our own history instance if requested
    if (create_own_history) {
        history = new ScriptHistory();
        if (history) {
            history->initFilesystem();
        }
    }
    
    // Initialize current line buffer and display buffer
    memset(current_line, 0, sizeof(current_line));
    memset(display_buffer, 0, sizeof(display_buffer));
    display_length = 0;
}

TermControl::~TermControl() {
    // Clean up history if we own it
    if (owns_history && history) {
        delete history;
        history = nullptr;
    }
}

// ============================================================================
// Stream Interface Implementation - Drop-in replacement for Serial
// ============================================================================

int TermControl::available() {
    if (!stream) return 0;
    return stream->available();
}

int TermControl::read() {
    if (!stream) return -1;
    return stream->read();
}

int TermControl::peek() {
    if (!stream) return -1;
    return stream->peek();
}

void TermControl::flush() {
    if (stream) {
        stream->flush();
    }
}

size_t TermControl::write(uint8_t byte) {
    if (!stream) return 0;
    return stream->write(byte);
}

size_t TermControl::write(const uint8_t *buffer, size_t size) {
    if (!stream) return 0;
    return stream->write(buffer, size);
}

// ============================================================================
// Terminal-specific functionality
// ============================================================================

bool TermControl::service() {
    if (!stream) return false;
    
    bool line_was_ready_before = line_ready;
    
    while (stream->available() > 0) {
        char c = (char)stream->read();
        
        // Handle ANSI escape sequences
        switch (ansi_state) {
            case ANSI_NORMAL:
                if (c == '\x1b') {  // ESC
                    ansi_state = ANSI_ESCAPE;
                } else {
                    handleNormalChar(c);
                }
                break;
                
            case ANSI_ESCAPE:
                if (c == '[') {
                    ansi_state = ANSI_BRACKET;
  } else {
                    ansi_state = ANSI_NORMAL;
                    // Handle other escape sequences if needed
                }
                break;
                
            case ANSI_BRACKET:
                ansi_state = ANSI_NORMAL;
                switch (c) {
                    case 'A': handleArrowUp(); break;
                    case 'B': handleArrowDown(); break;
                    case 'C': handleArrowRight(); break;
                    case 'D': handleArrowLeft(); break;
                    // Ignore other bracket sequences
                }
                break;
        }
        
        // Return immediately if a line became ready
        if (!line_was_ready_before && line_ready) {
            return true;
        }
    }
    
    return false;  // No line ready
}

bool TermControl::hasCompletedLine() const {
    return line_ready;
}

String TermControl::getCompletedLine() {
    if (!line_ready) return "";
    
    String result = completed_line;
    completed_line = "";
    line_ready = false;
    return result;
}

String TermControl::peekCompletedLine() const {
    if (!line_ready) return "";
    return completed_line;
}

void TermControl::clearCompletedLine() {
    completed_line = "";
    line_ready = false;
}

const char* TermControl::getCurrentLineBuffer() {
    return current_line;
}

void TermControl::setPrompt(const char* prompt) {
    if (prompt) {
        prompt_text = String(prompt);
    } else {
        prompt_text = "";
    }
}

void TermControl::enableEcho(bool enabled) {
    echo_enabled = enabled;
}

void TermControl::setColoredPrompt(const char* prompt, int color_code) {
    if (prompt) {
        char colored_prompt[128];
        snprintf(colored_prompt, sizeof(colored_prompt), "\x1b[38;5;%dm%s\x1b[0m", color_code, prompt);
        prompt_text = String(colored_prompt);
    } else {
        prompt_text = "";
    }
}


// ============================================================================
// Character and Line Handling
// ============================================================================

void TermControl::handleNormalChar(char c) {
    switch (c) {
        case '\r':
        case '\n':
            handleEnter();
            break;
            
        case '\b':      // Backspace
        case 0x7F:      // DEL
            handleBackspace();
            break;
            
        case 0x15:      // Ctrl+U - clear line
            handleCtrlU();
            break;
            
        case '\t':      // Tab
            handleTab();
            break;
            
        default:
            if (c >= 32 && c <= 126) {  // Printable characters
                insertCharAtCursor(c);
                if (echo_enabled) {
                    // Just re-render immediately - no special handling
                    renderCurrentLine();
                }
            }
            break;
    }
}

void TermControl::handleBackspace() {
    if (cursor_position > 0) {
        cursor_position--;
        // Delete character at new cursor position (effectively the character before old position)
        // Manually do the deletion to properly track display_length
        if (cursor_position < line_length) {
            memmove(&current_line[cursor_position], 
                    &current_line[cursor_position + 1], 
                    line_length - cursor_position - 1);
        }
        line_length--;
        display_length--;  // Track display length change
        current_line[line_length] = '\0';
        
        if (echo_enabled) {
            renderCurrentLine();
        }
    }
}

void TermControl::handleEnter() {
    if (echo_enabled && stream) {
        stream->print(TERMCONTROL_NEWLINE_OUT);
        stream->flush();
    }
    
    // Add to history if we have a history manager and non-empty line
    if (history && line_length > 0) {
        current_line[line_length] = '\0';  // Ensure null termination
        history->addToHistory(String(current_line));
    }
    
    // Make line available for parsing
    completed_line = String(current_line);
    line_ready = true;
    
    // Reset current line
    clearCurrentLine();
}

void TermControl::handleArrowUp() {
    if (!history) return;
    
    String prev = history->getPreviousCommand();
    if (prev.length() > 0 && prev.length() < TERMCONTROL_MAX_LINE_LENGTH) {
        strcpy(current_line, prev.c_str());
        line_length = prev.length();
        cursor_position = line_length;
        if (echo_enabled) {
            renderCurrentLine();
        }
    }
}

void TermControl::handleArrowDown() {
    if (!history) return;
    
    String next = history->getNextCommand();
    if (next.length() == 0) {
        // Clear line when going past newest command
        clearCurrentLine();
    } else if (next.length() < TERMCONTROL_MAX_LINE_LENGTH) {
        strcpy(current_line, next.c_str());
        line_length = next.length();
        cursor_position = line_length;
    }
    
    if (echo_enabled) {
        renderCurrentLine();
    }
}

void TermControl::handleArrowLeft() {
    if (cursor_position > 0) {
        cursor_position--;
        if (echo_enabled) {
            moveCursorTo(cursor_position);
        }
    }
}

void TermControl::handleArrowRight() {
    if (cursor_position < line_length) {
        cursor_position++;
        if (echo_enabled) {
            moveCursorTo(cursor_position);
        }
    }
}

void TermControl::handleCtrlU() {
    clearCurrentLine();
    if (echo_enabled) {
        renderCurrentLine();
    }
}

void TermControl::handleTab() {
    // Could implement tab completion here in the future
    // For now, just ignore
}

// ============================================================================
// Terminal Rendering and Cursor Management
// ============================================================================

void TermControl::renderCurrentLine() {
    if (!stream || !echo_enabled) return;
    
    // Create clean string from current line buffer
    char clean_line[TERMCONTROL_MAX_LINE_LENGTH + 1];
    memcpy(clean_line, current_line, line_length);
    clean_line[line_length] = '\0';
    
    // Get highlighted version from SyntaxHighlighting
    syntax_highlighter.setStream(stream);
    char* highlighted = syntax_highlighter.highlightString(clean_line, SYNTAX_JUMPERLESS_CONNECTIONS);
    
    // Store in display buffer for cursor calculations
    if (highlighted) {
        strncpy(display_buffer, highlighted, sizeof(display_buffer) - 1);
        display_buffer[sizeof(display_buffer) - 1] = '\0';
    } else {
        strncpy(display_buffer, clean_line, sizeof(display_buffer) - 1);
        display_buffer[sizeof(display_buffer) - 1] = '\0';
    }
    
    // Render the line
    stream->print('\r');
    
    // Print prompt if we have one
    if (prompt_text.length() > 0) {
        stream->print(prompt_text);
    }
    
    // Print the content (either highlighted or clean)
    if (line_length > 0) {
        if (highlighted && strlen(highlighted) > 0) {
            stream->print(highlighted);
            // Update display length to reflect actual visual length after highlighting
            display_length = calculateVisualLength(String(highlighted));
        } else {
            stream->print(clean_line);
            display_length = line_length;  // 1:1 mapping for plain text
        }
    } else {
        display_length = 0;
    }
    
    // Clear to end of line to erase old characters
    stream->print("\x1b[K");
    
    // Position cursor correctly using display length
    moveCursorToPosition(cursor_position);
    
    stream->flush();
}


void TermControl::clearCurrentLine() {
    memset(current_line, 0, sizeof(current_line));
    memset(display_buffer, 0, sizeof(display_buffer));
    line_length = 0;
    display_length = 0;
    cursor_position = 0;
}

void TermControl::moveCursorTo(int position) {
    if (!stream || !echo_enabled) return;
    
    // Use tracked display length instead of calculating
    int visual_prompt_len = calculateVisualLength(prompt_text);
    int target_column = visual_prompt_len + position + 1;  // +1 for 1-based column numbering
    
    // Move cursor to absolute column position
    stream->print('\r');  // Go to beginning of line
    if (target_column > 1) {
        stream->print("\x1b[");
        stream->print(target_column);
        stream->print('G');  // Move to absolute column
    }
    stream->flush();
}

// Calculate visual length of string (excluding ANSI escape sequences)
int TermControl::calculateVisualLength(const String& text) {
    int visual_len = 0;
    bool in_ansi = false;
    
    for (int i = 0; i < text.length(); i++) {
        char c = text.charAt(i);
        if (c == '\x1b') {
            in_ansi = true;
        } else if (in_ansi && c == 'm') {
            in_ansi = false;
        } else if (!in_ansi) {
            visual_len++;
        }
    }
    
    return visual_len;
}

// New cursor positioning method using tracked display length
void TermControl::moveCursorToPosition(int position) {
    if (!stream || !echo_enabled) return;
    
    // Use the actual visual prompt length and tracked display information
    int visual_prompt_len = calculateVisualLength(prompt_text);
    
    // Calculate cursor position within the displayed content
    // We need to map from clean buffer position to visual position in highlighted content
    int visual_position = position;  // Start with clean buffer position
    
    // For now, use the simple mapping - position in clean buffer should correspond to 
    // visual position in highlighted output (since we preserve spacing)
    int target_column = visual_prompt_len + visual_position + 1;  // +1 for 1-based column
    
    // Move cursor to absolute column position
    stream->print('\r');  // Go to beginning of line
    if (target_column > 1) {
        stream->print("\x1b[");
        stream->print(target_column);
        stream->print('G');  // Move to absolute column
    }
    stream->flush();
}

void TermControl::insertCharAtCursor(char c) {
    if (line_length >= TERMCONTROL_MAX_LINE_LENGTH - 1) {
        // Visual feedback when buffer is full
        if (stream && echo_enabled) {
            stream->print("\x07");  // Bell sound
            stream->flush();
        }
        return;  // Buffer full
    }
    
    // Shift characters right from cursor position
    if (cursor_position < line_length) {
        memmove(&current_line[cursor_position + 1], 
                &current_line[cursor_position], 
                line_length - cursor_position);
    }
    
    // Insert new character
    current_line[cursor_position] = c;
    cursor_position++;
    line_length++;
    
    // Update display length - for now assume 1:1 mapping, will be corrected in renderCurrentLine
    display_length++;
}

void TermControl::deleteCharAtCursor() {
    if (cursor_position >= line_length) {
        return;  // Nothing to delete
    }
    
    // Shift characters left
    memmove(&current_line[cursor_position], 
            &current_line[cursor_position + 1], 
            line_length - cursor_position - 1);
    
    line_length--;
    display_length--;  // Track display length change
    
    // Ensure buffer is clean
    current_line[line_length] = '\0';
}

