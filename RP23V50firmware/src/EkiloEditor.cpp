/*
 * EkiloEditor.cpp - Arduino-compatible eKilo text editor for Jumperless
 * Based on the original eKilo editor by Antonio Foti
 * Adapted for Arduino/embedded systems by removing Unix dependencies
 */

#include "EkiloEditor.h"
#include "Graphics.h"
#include "oled.h"
#include "RotaryEncoder.h"
#include "JumperlessDefines.h"
#include <time.h>

// External references
extern class oled oled;

// Global editor state
static EditorConfig E;

// Key definitions
#define KEY_NULL 0
#define CTRL_C 3
#define CTRL_D 4
#define CTRL_F 6
#define CTRL_H 8
#define TAB 9
#define CTRL_L 12
#define ENTER 13
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_S 19
#define CTRL_U 21
#define ESC 27
#define BACKSPACE 127

// Virtual key codes for special keys
#define ARROW_LEFT 1000
#define ARROW_RIGHT 1001
#define ARROW_UP 1002
#define ARROW_DOWN 1003
#define DEL_KEY 1004
#define HOME_KEY 1005
#define END_KEY 1006
#define PAGE_UP 1007
#define PAGE_DOWN 1008

// Syntax highlighting
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2
#define HL_MLCOMMENT 3
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

// Python syntax highlighting
const char* python_extensions[] = {".py", ".pyw", ".pyi", nullptr};
const char* python_keywords[] = {
    "and", "as", "assert", "break", "class", "continue", "def", "del",
    "elif", "else", "except", "exec", "finally", "for", "from", "global",
    "if", "import", "in", "is", "lambda", "not", "or", "pass", "print",
    "raise", "return", "try", "while", "with", "yield", "async", "await",
    "nonlocal", "True", "False", "None",
    
    // Python Built-ins (marked with |)
    "abs|", "all|", "any|", "bin|", "bool|", "bytes|", "callable|",
    "chr|", "dict|", "dir|", "enumerate|", "eval|", "filter|", "float|",
    "format|", "getattr|", "globals|", "hasattr|", "hash|", "help|", "hex|",
    "id|", "input|", "int|", "isinstance|", "iter|", "len|", "list|",
    "locals|", "map|", "max|", "min|", "next|", "object|", "oct|", "open|",
    "ord|", "pow|", "print|", "range|", "repr|", "reversed|", "round|",
    "set|", "setattr|", "slice|", "sorted|", "str|", "sum|", "super|",
    "tuple|", "type|", "vars|", "zip|", "self|", "cls|", nullptr
};

SyntaxDefinition syntax_db[] = {
    {
        python_extensions,
        python_keywords,
        "#", "", "",  // Single line comment, no multi-line for Python
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};

// Editor row structure
typedef struct EditorRow {
    int idx;
    int size;
    int rsize;
    char* chars;
    char* render;
    unsigned char* hl;
    int hl_oc;
} EditorRow;

EditorConfig::EditorConfig() {
    cx = cy = 0;
    rowoff = coloff = 0;
    screenrows = 20;  // Conservative default
    screencols = 80;
    numrows = 0;
    row = nullptr;
    dirty = 0;
    filename = nullptr;
    syntax = nullptr;
    should_quit = 0;
    strcpy(statusmsg, "");
    statusmsg_time = 0;
    
    // Initialize OLED horizontal scrolling
    oled_horizontal_offset = 0;
    
    // Initialize OLED batching
    oled_last_input_time = 0;
    oled_update_pending = false;
    
    // Initialize clickwheel character selection
    char_selection_mode = false;
    selected_char_index = 0;
    char_selection_timer = 0;
    
    // Initialize encoder position tracking
    last_encoder_position = 0;
    last_encoder_update = 0;
    
    // Initialize button state tracking
    last_button_state = true; // Button is active low
    button_debounce_time = 0;
    
    // Initialize menu navigation
    in_menu_mode = false;
    menu_selection = 0;
    
    // Initialize REPL mode
    repl_mode = false;
    original_cursor_row = 0;
    original_cursor_col = 0;
    start_row = 0;
    lines_used = 0;
    saved_file_content = "";
}

EditorConfig::~EditorConfig() {
    // Cleanup will be handled by the editor
}

// Initialize the editor
void ekilo_init() {
    E.cx = 0;
    E.cy = 0;
    E.rowoff = 0;
    E.coloff = 0;
    E.numrows = 0;
    E.row = nullptr;
    E.dirty = 0;
    E.filename = nullptr;
    E.syntax = nullptr;
    E.should_quit = 0;
    strcpy(E.statusmsg, "");
    E.statusmsg_time = 0;
    
    // Try to determine screen size - use conservative defaults for Arduino
    // Reduce available rows by 1 to account for persistent help header
    E.screenrows = 19; // Was 20, now 19 to account for help header
    E.screencols = 80;
}

// Memory management for dynamic buffer
struct Buffer {
    char* b;
    int len;
};

void buffer_append(Buffer* ab, const char* s, int len) {
    char* new_buf = (char*)realloc(ab->b, ab->len + len);
    if (new_buf == nullptr) return;
    
    memcpy(new_buf + ab->len, s, len);
    ab->b = new_buf;
    ab->len += len;
}

void buffer_free(Buffer* ab) {
    free(ab->b);
    ab->b = nullptr;
    ab->len = 0;
}

// Arduino-compatible key reading
int ekilo_read_key() {
    if (!Serial.available()) return 0;
    
    char c = Serial.read();
    
    // Handle escape sequences (arrow keys, etc.)
    if (c == ESC) {
        // Wait a bit for the rest of the sequence
        unsigned long start = millis();
        while (millis() - start < 50 && !Serial.available()) {
            delayMicroseconds(1);
        }
        
        if (!Serial.available()) return ESC;
        
        char seq[3];
        seq[0] = Serial.read();
        if (!Serial.available()) return ESC;
        seq[1] = Serial.read();
        
        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (!Serial.available()) return ESC;
                char seq2 = Serial.read();
                if (seq2 == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return ESC;
    } else {
        return c;
    }
}

// Check if character is a separator for syntax highlighting
int is_separator(int c) {
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != nullptr;
}

// Update syntax highlighting for a row
void ekilo_update_syntax(EditorRow* row) {
    row->hl = (unsigned char*)realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);
    
    if (E.syntax == nullptr) return;
    
    const char** keywords = E.syntax->keywords;
    const char* scs = E.syntax->singleline_comment_start;
    const char* mcs = E.syntax->multiline_comment_start;
    const char* mce = E.syntax->multiline_comment_end;
    
    int scs_len = scs ? strlen(scs) : 0;
    int mcs_len = mcs ? strlen(mcs) : 0;
    int mce_len = mce ? strlen(mce) : 0;
    
    int prev_sep = 1;
    int in_string = 0;
    int in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_oc);
    
    int i = 0;
    while (i < row->rsize) {
        char c = row->render[i];
        unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
        
        if (scs_len && !in_string && !in_comment) {
            if (!strncmp(&row->render[i], scs, scs_len)) {
                memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                break;
            }
        }
        
        if (mcs_len && mce_len && !in_string) {
            if (in_comment) {
                row->hl[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len)) {
                    memset(&row->hl[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_sep = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                memset(&row->hl[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }
        
        if (E.syntax->flags & HL_HIGHLIGHT_STRINGS) {
            if (in_string) {
                row->hl[i] = HL_STRING;
                if (c == '\\' && i + 1 < row->rsize) {
                    row->hl[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }
                if (c == in_string) in_string = 0;
                i++;
                prev_sep = 1;
                continue;
            } else {
                if (c == '"' || c == '\'') {
                    in_string = c;
                    row->hl[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }
        
        if (E.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                (c == '.' && prev_hl == HL_NUMBER)) {
                row->hl[i] = HL_NUMBER;
                i++;
                prev_sep = 0;
                continue;
            }
        }
        
        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;
                
                if (!strncmp(&row->render[i], keywords[j], klen) &&
                    is_separator(row->render[i + klen])) {
                    memset(&row->hl[i], kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    i += klen;
                    break;
                }
            }
            if (keywords[j] != nullptr) {
                prev_sep = 0;
                continue;
            }
        }
        
        prev_sep = is_separator(c);
        i++;
    }
    
    int changed = (row->hl_oc != in_comment);
    row->hl_oc = in_comment;
    if (changed && row->idx + 1 < E.numrows)
        ekilo_update_syntax(&E.row[row->idx + 1]);
}

// Convert syntax highlighting to ANSI color codes
int ekilo_syntax_to_color(int hl) {
    switch (hl) {
        case HL_COMMENT:
        case HL_MLCOMMENT: return 36; // Cyan
        case HL_KEYWORD1: return 33;  // Yellow
        case HL_KEYWORD2: return 32;  // Green
        case HL_STRING: return 35;    // Magenta
        case HL_NUMBER: return 31;    // Red
        case HL_MATCH: return 34;     // Blue
        default: return 37;           // White
    }
}

// Select syntax highlighting based on filename
void ekilo_select_syntax_highlight(const char* filename) {
    E.syntax = nullptr;
    if (filename == nullptr) return;
    
    const char* ext = strrchr(filename, '.');
    if (ext == nullptr) return;
    
    for (unsigned int j = 0; j < sizeof(syntax_db) / sizeof(syntax_db[0]); j++) {
        SyntaxDefinition* s = &syntax_db[j];
        for (int i = 0; s->filematch[i]; i++) {
            int is_ext = (s->filematch[i][0] == '.');
            if ((is_ext && !strcmp(ext, s->filematch[i])) ||
                (!is_ext && strstr(filename, s->filematch[i]))) {
                E.syntax = s;
                
                // Re-highlight all rows
                for (int filerow = 0; filerow < E.numrows; filerow++) {
                    ekilo_update_syntax(&E.row[filerow]);
                }
                return;
            }
        }
    }
}

// Convert tabs to spaces and calculate render string
void ekilo_update_row(EditorRow* row) {
    int tabs = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }
    
    free(row->render);
    row->render = (char*)malloc(row->size + tabs * 7 + 1);
    
    int idx = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') {
            row->render[idx++] = ' ';
            while (idx % 8 != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->render[idx] = '\0';
    row->rsize = idx;
    
    ekilo_update_syntax(row);
}

// Insert a row at the specified position
void ekilo_insert_row(int at, const char* s, size_t len) {
    if (at < 0 || at > E.numrows) return;
    
    E.row = (EditorRow*)realloc(E.row, sizeof(EditorRow) * (E.numrows + 1));
    memmove(&E.row[at + 1], &E.row[at], sizeof(EditorRow) * (E.numrows - at));
    for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    
    E.row[at].idx = at;
    E.row[at].size = len;
    E.row[at].chars = (char*)malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    
    E.row[at].rsize = 0;
    E.row[at].render = nullptr;
    E.row[at].hl = nullptr;
    E.row[at].hl_oc = 0;
    ekilo_update_row(&E.row[at]);
    
    E.numrows++;
    E.dirty++;
}

// Free a row's memory
void ekilo_free_row(EditorRow* row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

// Delete a row
void ekilo_del_row(int at) {
    if (at < 0 || at >= E.numrows) return;
    
    ekilo_free_row(&E.row[at]);
    memmove(&E.row[at], &E.row[at + 1], sizeof(EditorRow) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx--;
    E.numrows--;
    E.dirty++;
}

// Insert character in a row
void ekilo_row_insert_char(EditorRow* row, int at, int c) {
    if (at < 0 || at > row->size) at = row->size;
    
    row->chars = (char*)realloc(row->chars, row->size + 2);
    memmove(&row->chars[at + 1], &row->chars[at], row->size - at + 1);
    row->size++;
    row->chars[at] = c;
    ekilo_update_row(row);
    E.dirty++;
}

// Delete character from a row
void ekilo_row_del_char(EditorRow* row, int at) {
    if (at < 0 || at >= row->size) return;
    
    memmove(&row->chars[at], &row->chars[at + 1], row->size - at);
    row->size--;
    ekilo_update_row(row);
    E.dirty++;
}

// Append string to a row
void ekilo_row_append_string(EditorRow* row, char* s, size_t len) {
    row->chars = (char*)realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], s, len);
    row->size += len;
    row->chars[row->size] = '\0';
    ekilo_update_row(row);
    E.dirty++;
}

// Insert character at cursor
void ekilo_insert_char(int c) {
    if (E.cy == E.numrows) {
        ekilo_insert_row(E.numrows, "", 0);
    }
    ekilo_row_insert_char(&E.row[E.cy], E.cx, c);
    E.cx++;
    
    // Schedule OLED update after character insertion
    ekilo_schedule_oled_update();
}

// Insert newline
void ekilo_insert_newline() {
    if (E.cx == 0) {
        ekilo_insert_row(E.cy, "", 0);
    } else {
        EditorRow* row = &E.row[E.cy];
        ekilo_insert_row(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        ekilo_update_row(row);
    }
    E.cy++;
    E.cx = 0;
    
    // Reset horizontal scrolling when moving to new line
    E.oled_horizontal_offset = 0;
    
    // Schedule OLED update after newline insertion
    ekilo_schedule_oled_update();
}

// Delete character
void ekilo_del_char() {
    if (E.cy == E.numrows) return;
    if (E.cx == 0 && E.cy == 0) return;
    
    EditorRow* row = &E.row[E.cy];
    if (E.cx > 0) {
        ekilo_row_del_char(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        ekilo_row_append_string(&E.row[E.cy - 1], row->chars, row->size);
        ekilo_del_row(E.cy);
        E.cy--;
        // Reset horizontal scrolling when moving to previous line
        E.oled_horizontal_offset = 0;
    }
    
    // Schedule OLED update after character deletion
    ekilo_schedule_oled_update();
}

// Set status message
void ekilo_set_status_message(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = millis();
}

// Move cursor based on key
void ekilo_move_cursor(int key) {
    EditorRow* row = (E.cy >= E.numrows) ? nullptr : &E.row[E.cy];
    
    switch (key) {
        case ARROW_LEFT:
            if (E.cx != 0) {
                E.cx--;
            } else if (E.cy > 0) {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;
        case ARROW_RIGHT:
            if (row && E.cx < row->size) {
                E.cx++;
            } else if (row && E.cx == row->size) {
                E.cy++;
                E.cx = 0;
            }
            break;
        case ARROW_UP:
            if (E.cy != 0) {
                E.cy--;
                // Reset horizontal scrolling when changing lines
                E.oled_horizontal_offset = 0;
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
                // Reset horizontal scrolling when changing lines
                E.oled_horizontal_offset = 0;
            }
            break;
    }
    
    row = (E.cy >= E.numrows) ? nullptr : &E.row[E.cy];
    int rowlen = row ? row->size : 0;
    if (E.cx > rowlen) {
        E.cx = rowlen;
    }
    
    // Schedule OLED update when cursor moves
    ekilo_schedule_oled_update();
}

// Open file
int ekilo_open(const char* filename) {
    free(E.filename);
    E.filename = strdup(filename);
    
    ekilo_select_syntax_highlight(filename);
    
    File file = FatFS.open(filename, "r");
    if (!file) return -1;
    
    while (file.available()) {
        String line = file.readStringUntil('\n');
        // Only remove trailing carriage return, preserve leading whitespace for indentation
        if (line.endsWith("\r")) {
            line.remove(line.length() - 1);
        }
        ekilo_insert_row(E.numrows, line.c_str(), line.length());
    }
    file.close();
    E.dirty = 0;
    return 0;
}

// Save file
int ekilo_save() {
    if (E.filename == nullptr) {
        // TODO: Implement save-as functionality
        ekilo_set_status_message("Save aborted");
        return 0;
    }
    
    int len = 0;
    for (int j = 0; j < E.numrows; j++)
        len += E.row[j].size + 1;
    
    char* buf = (char*)malloc(len);
    char* p = buf;
    for (int j = 0; j < E.numrows; j++) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }
    
    File file = FatFS.open(E.filename, "w");
    if (file) {
        file.write((uint8_t*)buf, len);
        file.close();
        
        // If in REPL mode, store content for return
        if (E.repl_mode) {
            E.saved_file_content = String(buf, len);
            E.should_quit = 1; // Auto-exit after save in REPL mode
        }
        
        free(buf);
        E.dirty = 0;
        ekilo_set_status_message("%d bytes written to flash", len);
        return len;
    } else {
        free(buf);
        ekilo_set_status_message("Can't save! I/O error: %s", "File write failed");
        return 0;
    }
}

// Refresh screen - Jumperless-compatible version
void ekilo_refresh_screen() {
    // Scroll handling
    if (E.cy < E.rowoff) {
        E.rowoff = E.cy;
    }
    if (E.cy >= E.rowoff + E.screenrows) {
        E.rowoff = E.cy - E.screenrows + 1;
    }
    if (E.cx < E.coloff) {
        E.coloff = E.cx;
    }
    if (E.cx >= E.coloff + E.screencols) {
        E.coloff = E.cx - E.screencols + 1;
    }
    
    Buffer ab = {nullptr, 0};
    
    // Clear screen and position cursor - use absolute positioning for both modes
    if (E.repl_mode) {
        // In REPL mode with XTerm alternate screen, clear and position at top-left
        // No header in REPL mode for cleaner interface
        buffer_append(&ab, "\x1b[2J\x1b[H", 7);
    } else {
        buffer_append(&ab, "\x1b[2J\x1b[H", 7);
        
        // Add persistent help header that stays visible (only in non-REPL mode)
        buffer_append(&ab, "\x1b[44m\x1b[200m", 10); // Blue background, white text
        char help_header[120];
        snprintf(help_header, sizeof(help_header), 
                     "eKilo Editor │ Ctrl-S/P=save │ Ctrl-Q=quit │ Wheel=navigate │ Scroll past end→menu");
        int help_len = strlen(help_header);
        if (help_len > E.screencols) help_len = E.screencols;
        buffer_append(&ab, help_header, help_len);
        
        // Pad help header to full width
        while (help_len < E.screencols) {
            buffer_append(&ab, " ", 1);
            help_len++;
        }
        buffer_append(&ab, "\x1b[0m\r\n", 6); // Reset formatting and newline
    }
    
    // Draw rows (adjust available rows for header and help lines)
    int available_rows = E.repl_mode ? E.screenrows - 3 : E.screenrows - 4; // -3 for status+message+help, -4 for header+status+message+help
    for (int y = 0; y < available_rows; y++) {
        int filerow = E.rowoff + y;
        
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == available_rows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "eKilo editor -- version 1.0.0");
                if (welcomelen > E.screencols) welcomelen = E.screencols;
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) {
                    buffer_append(&ab, "~", 1);
                    padding--;
                }
                while (padding--) buffer_append(&ab, " ", 1);
                buffer_append(&ab, welcome, welcomelen);
            } else {
                buffer_append(&ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].rsize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            char* c = &E.row[filerow].render[E.coloff];
            unsigned char* hl = &E.row[filerow].hl[E.coloff];
            int current_color = -1;
            for (int j = 0; j < len; j++) {
                if (iscntrl(c[j])) {
                    // Show control chars as inverted symbols
                    char sym = (c[j] <= 26) ? '@' + c[j] : '?';
                    buffer_append(&ab, "\x1b[7m", 4);
                    buffer_append(&ab, &sym, 1);
                    buffer_append(&ab, "\x1b[0m", 4); // Use \x1b[0m instead of \x1b[m
                    if (current_color != -1) {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        buffer_append(&ab, buf, clen);
                    }
                } else if (hl[j] == HL_NORMAL) {
                    if (current_color != -1) {
                        buffer_append(&ab, "\x1b[0m", 4); // Reset all formatting
                        current_color = -1;
                    }
                    buffer_append(&ab, &c[j], 1);
                } else {
                    int color = ekilo_syntax_to_color(hl[j]);
                    if (color != current_color) {
                        current_color = color;
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                        buffer_append(&ab, buf, clen);
                    }
                    buffer_append(&ab, &c[j], 1);
                }
            }
            // Reset colors at end of line
            if (current_color != -1) {
                buffer_append(&ab, "\x1b[0m", 4);
            }
        }
        
        // Pad line to full width and add newline
        // Calculate current line length and pad with spaces instead of using \x1b[K
        int current_len = 0;
        if (filerow < E.numrows) {
            int display_len = E.row[filerow].rsize - E.coloff;
            if (display_len > 0 && display_len <= E.screencols) {
                current_len = display_len;
            }
        } else if (filerow >= E.numrows && y != (E.screenrows - 1) / 3) {
            current_len = 1; // For the "~" character
        }
        
        // Pad with spaces to clear rest of line
        while (current_len < E.screencols) {
            buffer_append(&ab, " ", 1);
            current_len++;
        }
        buffer_append(&ab, "\r\n", 2);
    }
    
    // Status bar with reverse video
    buffer_append(&ab, "\x1b[7m", 4);
    char status[80], rstatus[80];
    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E.filename ? E.filename : "[No Name]", E.numrows,
        E.dirty ? "(modified)" : "");
    
    // Add menu mode indicator to status bar
    if (E.in_menu_mode) {
        int remaining = sizeof(status) - strlen(status) - 1;
        strncat(status, " [MENU MODE]", remaining);
        len = strlen(status);
    }
    
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d",
        E.cy + 1, E.numrows);
    if (len > E.screencols) len = E.screencols;
    buffer_append(&ab, status, len);
    while (len < E.screencols) {
        if (E.screencols - len == rlen) {
            buffer_append(&ab, rstatus, rlen);
            break;
        } else {
            buffer_append(&ab, " ", 1);
            len++;
        }
    }
    buffer_append(&ab, "\x1b[0m", 4); // Reset formatting
    buffer_append(&ab, "\r\n", 2);
    
    // Message bar (first line - status or temp messages)
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && millis() - E.statusmsg_time < 5000) {
        buffer_append(&ab, E.statusmsg, msglen);
    }
    // Pad message bar to full width
    while (msglen < E.screencols) {
        buffer_append(&ab, " ", 1);
        msglen++;
    }
    buffer_append(&ab, "\r\n", 2);
    
    // Help bar (second line - comprehensive commands in magenta)
    buffer_append(&ab, "\x1b[35m", 5); // Magenta color
    char help_line1[120];
    char help_line2[120];
    snprintf(help_line1, sizeof(help_line1), 
             "Ctrl-S/P=Save │ Ctrl-Q=Quit │ Arrows=Navigate │ Home/End=Line start/end │ PgUp/PgDn=Page");
    snprintf(help_line2, sizeof(help_line2),
             "Tab=Indent │ Backspace/Del=Delete │ Wheel=Move/Type │ ESC=Cancel │ All printable chars");
    
    int help1_len = strlen(help_line1);
    if (help1_len > E.screencols) help1_len = E.screencols;
    buffer_append(&ab, help_line1, help1_len);
    // Pad first help line to full width
    while (help1_len < E.screencols) {
        buffer_append(&ab, " ", 1);
        help1_len++;
    }
    buffer_append(&ab, "\r\n", 2);
    
    int help2_len = strlen(help_line2);
    if (help2_len > E.screencols) help2_len = E.screencols;
    buffer_append(&ab, help_line2, help2_len);
    // Pad second help line to full width
    while (help2_len < E.screencols) {
        buffer_append(&ab, " ", 1);
        help2_len++;
    }
    buffer_append(&ab, "\x1b[0m", 4); // Reset color
    
    // Position cursor - use absolute positioning for both modes
    char buf[32];
    if (E.repl_mode) {
        // In REPL mode with XTerm alternate screen, use absolute positioning
        // No header offset since we skip the header in REPL mode
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, // +1 for content area (no header)
                                                    (E.cx - E.coloff) + 1);
        buffer_append(&ab, buf, strlen(buf));
        
        // Track total lines for XTerm cleanup (though XTerm handles this automatically)
        E.lines_used = E.screenrows; // Use full screen in XTerm alternate buffer
    } else {
        snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 2, // +2 for help header
                                                    (E.cx - E.coloff) + 1);
        buffer_append(&ab, buf, strlen(buf));
    }
    
    // Write buffer (no cursor hide/show to avoid compatibility issues)
    Serial.write((uint8_t*)ab.b, ab.len);
    Serial.flush(); // Ensure data is sent immediately
    buffer_free(&ab);
    
    // Schedule OLED update with context around cursor
    ekilo_schedule_oled_update();
}

// Process keypress
void ekilo_process_keypress() {
    static int quit_times = 3;
    
    int c = ekilo_read_key();
    if (c == 0) return; // No key available
    
    switch (c) {
        case '\n':
        
        case ENTER:
            ekilo_insert_newline();
            break;
            
        case CTRL_Q:
            if (E.dirty && quit_times > 0) {
                ekilo_set_status_message("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-Q %d more times to quit.", quit_times);
                quit_times--;
                return;
            }
            E.should_quit = 1;
            break;
            
        case CTRL_S:
        case CTRL_P:
            ekilo_save();
            break;
            
        case HOME_KEY:
            E.cx = 0;
            E.oled_horizontal_offset = 0; // Reset scrolling when going to start of line
            ekilo_schedule_oled_update();
            break;
            
        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            // Don't reset horizontal scrolling for END - let it scroll to show end of line
            ekilo_schedule_oled_update();
            break;
            
        case BACKSPACE:
        case CTRL_H:
        case DEL_KEY:
            if (c == DEL_KEY) ekilo_move_cursor(ARROW_RIGHT);
            ekilo_del_char();
            break;
            
        case PAGE_UP:
        case PAGE_DOWN: {
            if (c == PAGE_UP) {
                E.cy = E.rowoff;
            } else if (c == PAGE_DOWN) {
                E.cy = E.rowoff + E.screenrows - 1;
                if (E.cy > E.numrows) E.cy = E.numrows;
            }
            
            int times = E.screenrows;
            while (times--)
                ekilo_move_cursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            break;
        }
        case TAB:
            ekilo_insert_char(' ');
            ekilo_insert_char(' ');
            ekilo_insert_char(' '); 
            ekilo_insert_char(' ');
            break;
        
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            ekilo_move_cursor(c);
            break;
            
        case CTRL_L:
        case ESC:
            if (E.in_menu_mode) {
                // Exit menu mode with ESC
                E.in_menu_mode = false;
                if (E.numrows > 0) {
                    E.cy = E.numrows - 1; // Go to last line
                    E.cx = E.row[E.cy].size; // Go to end of line
                }
                ekilo_set_status_message("Menu mode cancelled");
                ekilo_schedule_oled_update();
            }
            break;
            
        default:
            ekilo_insert_char(c);
            break;
    }
    
    quit_times = 2;
}

// OLED update batching functions
void ekilo_schedule_oled_update() {
    unsigned long currentTime = millis();
    
    // If no recent input, update immediately for responsiveness
    if (!E.oled_update_pending || (currentTime - E.oled_last_input_time) > 200) {
        ekilo_update_oled_context();
        E.oled_last_input_time = currentTime;
        E.oled_update_pending = false;
    } else {
        // Batch subsequent updates
        E.oled_last_input_time = currentTime;
        E.oled_update_pending = true;
    }
}

void ekilo_process_oled_update() {
    if (E.oled_update_pending && (millis() - E.oled_last_input_time) >= 50) { // 50ms delay
        ekilo_update_oled_context();
        E.oled_update_pending = false;
    }
}

// Calculate horizontal scrolling for OLED display of current line
void ekilo_calculate_oled_scrolling() {
    if (!oled.isConnected() || E.numrows == 0) return;
    
    // Get current line text
    String currentLineText = "";
    if (E.cy < E.numrows && E.row[E.cy].chars) {
        currentLineText = String(E.row[E.cy].chars);
    }
    
    // Cursor position within the current line
    int cursorPos = E.cx;
    
    // Calculate characters that fit based on actual font metrics
    int charWidth = oled.getCharacterWidth();
    const int maxVisibleChars = charWidth > 0 ? (128 / charWidth) : 21;
    
    // Calculate visible cursor position relative to current offset
    int visibleCursorPos = cursorPos - E.oled_horizontal_offset;
    
    // Ensure cursor has 2 characters of padding from right edge for adding characters
    const int CURSOR_RIGHT_PADDING = 2;
    
    // Scroll right if cursor is too close to right edge (accounting for padding)
    if (visibleCursorPos >= maxVisibleChars - CURSOR_RIGHT_PADDING) {
        E.oled_horizontal_offset = cursorPos - maxVisibleChars + CURSOR_RIGHT_PADDING + 1;
    }
    // Scroll left if cursor is too close to left edge  
    else if (visibleCursorPos < E.OLED_SCROLL_MARGIN) {
        E.oled_horizontal_offset = cursorPos - E.OLED_SCROLL_MARGIN;
    }
    
    // Keep offset within bounds
    if (E.oled_horizontal_offset < 0) {
        E.oled_horizontal_offset = 0;
    }
    
    // Allow scrolling past end of line to provide space for adding characters
    // Don't restrict offset based on line length - let cursor have padding space
}

// Update OLED with 4 lines around cursor
void ekilo_update_oled_context() {
    if (!oled.isConnected()) {
        return;
    }
    
    // Calculate horizontal scrolling for current line
    if (E.numrows > 0) {
        ekilo_calculate_oled_scrolling();
    }
    
    // Calculate current file row position
    int currentRow = E.cy;
    
    // Determine which 3 lines to show (1 before, 2 after, adjusting for edges)
    int startRow = currentRow - 1;
    int endRow = currentRow + 1;
    
    // Adjust for edges
    if (startRow < 0) {
        startRow = 0;
        endRow = min(2, E.numrows - 1);
    } else if (endRow >= E.numrows) {
        endRow = E.numrows - 1;
        startRow = max(0, endRow - 2);
    }
    
    // Build the display string
    String displayText = "";
    int cursorLineInDisplay = -1; // Track which line in display has cursor
    int displayLineCount = 0;
    
    // Handle empty file case
    if (E.numrows == 0) {
        displayText = "[Empty File]";
        displayLineCount = 1;
        cursorLineInDisplay = 0;
    } else {
        for (int i = startRow; i <= endRow && i < E.numrows; i++) {
            String line = "";
            
            // No prefix - just track which line is current
            if (i == currentRow) {
                cursorLineInDisplay = displayLineCount;
            }
            
            // Add line content - apply horizontal scrolling to all visible lines
            if (E.row[i].chars) {
                String rowText = String(E.row[i].chars);
                
                // Apply horizontal scrolling to all visible lines to keep relative positions
                int charWidth = oled.getCharacterWidth();
                const int maxVisibleChars = charWidth > 0 ? (128 / charWidth) : 21;
                int startPos = E.oled_horizontal_offset;
                int endPos = min(startPos + maxVisibleChars, (int)rowText.length());
                if (startPos < rowText.length()) {
                    rowText = rowText.substring(startPos, endPos);
                } else {
                    rowText = ""; // Past end of line
                }
                
                line += rowText;
            }
            
            displayText += line;
            if (i < endRow && i < E.numrows - 1) {
                displayText += "\n";
            }
            displayLineCount++;
        }
    }
    
    // Display on OLED using framebuffer approach
    oled.clearFramebuffer();
    oled.setSmallFont(SMALL_FONT_ANDALE_MONO);
    
    // Draw each line at precise positions
    int lineY = 8; // Start position with proper baseline
    const int lineHeight = 12; // Line spacing for small font (increased to 12 pixels for better readability)
    
    String lines[3]; // Changed from 4 to 3 lines
    int lineCount = 0;
    
    // Split displayText into lines
    int lastStart = 0;
    for (int i = 0; i <= displayText.length(); i++) {
        if (i == displayText.length() || displayText[i] == '\n') {
            if (lineCount < 3) { // Changed from 4 to 3
                lines[lineCount] = displayText.substring(lastStart, i);
                lineCount++;
            }
            lastStart = i + 1;
        }
    }
    
    // Draw each line
    for (int i = 0; i < lineCount; i++) {
        oled.drawText(0, lineY + (i * lineHeight), lines[i].c_str());
    }
    
    // Add cursor position indicator if current line is visible
    if (cursorLineInDisplay >= 0 && E.numrows > 0) {
        // Calculate cursor position within the visible text of current line
        int visibleCursorPos = E.cx - E.oled_horizontal_offset;
        
        // Show cursor if it's visible on screen (including beyond end of line)
        int charWidth = oled.getCharacterWidth();
        const int maxVisibleChars = charWidth > 0 ? (128 / charWidth) : 21;
        
        if (visibleCursorPos >= 0 && visibleCursorPos < maxVisibleChars) {
            // Get the character at cursor position
            char cursorChar = ' '; // Default to space
            
            if (visibleCursorPos < lines[cursorLineInDisplay].length()) {
                cursorChar = lines[cursorLineInDisplay][visibleCursorPos];
                if (cursorChar == '\0' || cursorChar == '\n') {
                    cursorChar = ' '; // Show space for end of line
                }
            }
            // If cursor is beyond the line, cursorChar remains space
            
            int cursorX = visibleCursorPos * charWidth;
            int cursorY = lineY + (cursorLineInDisplay * lineHeight);
            
            // Draw highlighted character instead of underline
            oled.drawHighlightedChar(cursorX, cursorY, cursorChar);
        }
    }
    
    // Show character selection mode if active
    if (E.char_selection_mode) {
        // Draw character selection indicator in top-right corner with larger text
        char selectionText[5];
        char selected = ekilo_get_character_from_index(E.selected_char_index);
        if (selected == '\n') {
            strcpy(selectionText, "\\n");
        } else if (selected == '\b') {
            strcpy(selectionText, "BS");
        } else if (selected == '\t') {
            strcpy(selectionText, "TAB");
        } else {
            snprintf(selectionText, sizeof(selectionText), "%c", selected);
        }
        
        // Use larger font for character indicator
        oled.setFontForSize(FONT_ANDALE_MONO, 2);
        int largeCharWidth = oled.getCharacterWidth();
        
        // Position in top-right corner
        int textWidth = strlen(selectionText) * largeCharWidth;
        int x = 128 - textWidth - 2; // 2 pixel margin
        int y = 12; // Adjusted for larger font baseline
        
        // Draw background box
        oled.fillRect(x - 1, y - 11, textWidth + 2, 14, SSD1306_WHITE);
        
        // Draw text in black on white background
        oled.setTextColor(SSD1306_BLACK);
        oled.drawText(x, y, selectionText);
        oled.setTextColor(SSD1306_WHITE); // Restore default
        
        // Restore small font for rest of display
        oled.setSmallFont(SMALL_FONT_ANDALE_MONO);
    }
    
    // Draw save/exit menu 2 lines below file content
    int menuY = lineY + (lineCount * lineHeight) + 6; // 6 pixel gap between file and menu
    
    // Save and cancel options with proper UI styling
    String saveText = "Save";
    String cancelText = "Cancel";
    
    // Always show menu buttons, but only highlight when in menu mode
    if (E.in_menu_mode) {
        // Clear display and just show menu options
        oled.clearFramebuffer();
        
        // Calculate positions for side-by-side layout in center of screen
        int charWidth = oled.getCharacterWidth();
        int saveWidth = saveText.length() * charWidth;
        int cancelWidth = cancelText.length() * charWidth;
        int buttonHeight = 12;
        int buttonPadding = 4;
        int gapBetweenButtons = 8;
        
        // Total width needed: saveButton + gap + cancelButton
        int totalWidth = (saveWidth + buttonPadding * 2) + gapBetweenButtons + (cancelWidth + buttonPadding * 2);
        int startX = (128 - totalWidth) / 2; // Center horizontally
        int buttonY = 10; // Center vertically on 32px tall screen
        
        int saveButtonX = startX;
        int cancelButtonX = startX + (saveWidth + buttonPadding * 2) + gapBetweenButtons;
        
        // Draw Save button
        if (E.menu_selection == 0) { // Save selected
            oled.fillRect(saveButtonX, buttonY, saveWidth + buttonPadding * 2, buttonHeight, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
            oled.drawText(saveButtonX + buttonPadding, buttonY + 8, saveText.c_str());
            oled.setTextColor(SSD1306_WHITE);
        } else {
            oled.drawText(saveButtonX + buttonPadding, buttonY + 8, saveText.c_str());
        }
        
        // Draw Cancel button
        if (E.menu_selection == 1) { // Cancel selected
            oled.fillRect(cancelButtonX, buttonY, cancelWidth + buttonPadding * 2, buttonHeight, SSD1306_WHITE);
            oled.setTextColor(SSD1306_BLACK);
            oled.drawText(cancelButtonX + buttonPadding, buttonY + 8, cancelText.c_str());
            oled.setTextColor(SSD1306_WHITE);
        } else {
            oled.drawText(cancelButtonX + buttonPadding, buttonY + 8, cancelText.c_str());
        }
    } else {
        // Show dimmed menu options when not in menu mode
        oled.drawText(2, menuY, saveText.c_str());
        oled.drawText(2, menuY + 12, cancelText.c_str());
    }
    
    // Flush to display
    oled.flushFramebuffer();
}

// Character selection array for clickwheel input
const char* character_set = " abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ!@#$%^&*()[]{}|\\:;\"'<>,.?/+-=_~`\t\n\b";

// Get character from index in character set
char ekilo_get_character_from_index(int index) {
    int len = strlen(character_set);
    if (index < 0) index = len - 1;
    if (index >= len) index = 0;
    return character_set[index];
}

// Enter character selection mode
void ekilo_enter_char_selection() {
    E.char_selection_mode = true;
    E.selected_char_index = 1; // Start with 'A' instead of space
    E.char_selection_timer = millis();
    ekilo_set_status_message("CHAR SELECT: %c (Wheel=cycle, Click=confirm, Wait=exit)", 
                             ekilo_get_character_from_index(E.selected_char_index));
    
    // Force immediate OLED update when entering character selection mode
    ekilo_update_oled_context();
}

// Exit character selection mode
void ekilo_exit_char_selection() {
    E.char_selection_mode = false;
    ekilo_set_status_message("Character selection cancelled");
    
    // Force immediate OLED update when exiting character selection mode
    ekilo_update_oled_context();
}

// Cycle through characters
void ekilo_cycle_character(int direction) {
    if (!E.char_selection_mode) return;
    
    E.selected_char_index -= direction;
    int len = strlen(character_set);
    
    // Wrap around
    if (E.selected_char_index < 0) E.selected_char_index = len - 1;
    if (E.selected_char_index >= len) E.selected_char_index = 0;
    
    E.char_selection_timer = millis(); // Reset timeout
    
    char selected = ekilo_get_character_from_index(E.selected_char_index);
    if (selected == '\n') {
        ekilo_set_status_message("CHAR SELECT: \\n (Wheel=cycle, Click=confirm, Wait=exit)");
    } else if (selected == '\b') {
        ekilo_set_status_message("CHAR SELECT: BKSP (Wheel=cycle, Click=confirm, Wait=exit)");
    } else {
        ekilo_set_status_message("CHAR SELECT: %c (Wheel=cycle, Click=confirm, Wait=exit)", selected);
    }
    
    // Force immediate OLED update in character selection mode for responsiveness
    ekilo_update_oled_context();
}

// Confirm character selection and insert it
void ekilo_confirm_character() {
    if (!E.char_selection_mode) return;
    
    char selected = ekilo_get_character_from_index(E.selected_char_index);
    E.char_selection_mode = false;
    
    // If cursor is beyond the end of line, move to end first
    if (E.cy < E.numrows && E.cx > E.row[E.cy].size) {
        E.cx = E.row[E.cy].size;
    }
    
    if (selected == '\t') {
        ekilo_insert_char(' ');
        ekilo_insert_char(' ');
        ekilo_insert_char(' ');
        ekilo_insert_char(' ');
        ekilo_set_status_message("Inserted tab");
    } else if (selected == '\n') {
        ekilo_insert_newline();
        ekilo_set_status_message("Inserted newline");
    } else if (selected == '\b') {
        ekilo_del_char();
        ekilo_set_status_message("Backspace");
    } else {
        ekilo_insert_char(selected);
        ekilo_set_status_message("Inserted '%c'", selected);
    }
    
    // Force immediate OLED update after character insertion
    ekilo_update_oled_context();
}

// Process encoder input for cursor movement and character selection
void ekilo_process_encoder_input() {
    // Check for character selection timeout
    if (E.char_selection_mode && (millis() - E.char_selection_timer > E.CHAR_SELECTION_TIMEOUT)) {
        ekilo_exit_char_selection();
        return;
    }
    
    unsigned long currentTime = millis();
    
    // Read current encoder position
    long currentPosition = encoderPosition;
    
    // Calculate position delta
    long deltaPosition = (currentPosition - E.last_encoder_position)/4;
    deltaPosition = -deltaPosition;
    
    // Only process if there's a significant change and enough time has passed
    if (deltaPosition != 0 && (currentTime - E.last_encoder_update >= 20)) { // 20ms minimum between updates for responsiveness
        E.last_encoder_position = currentPosition;
        E.last_encoder_update = currentTime;
        
        if (E.char_selection_mode) {
            // In character selection mode: cycle through characters
            // Use smaller delta for precise character selection
            int steps = max(1, min(abs(deltaPosition), 3)); // 1-3 characters per rotation for faster cycling
            int direction = (deltaPosition > 0) ? -1 : 1;
            
            for (int i = 0; i < steps; i++) {
                ekilo_cycle_character(direction);
            }
        } else {
            // Normal mode: move cursor or navigate menu
            if (E.in_menu_mode) {
                // In menu mode: navigate between save/exit options or loop to file
                if (deltaPosition != 0) {
                    int direction = (deltaPosition > 0) ? 1 : -1;  // Match cursor movement direction
                    
                    if (direction > 0) {  // Right scroll in menu (clockwise)
                        // Scrolling right in menu: Save -> Cancel -> loop to beginning of file
                        E.menu_selection++;
                        if (E.menu_selection > 1) {
                            // Past exit option - loop to beginning of file
                            E.in_menu_mode = false;
                            E.cy = 0;
                            E.cx = 0;
                            ekilo_set_status_message("Looped to beginning of file");
                        } else {
                            // Update menu selection
                            if (E.menu_selection == 0) {
                                ekilo_set_status_message("Save selected - click to save file");
                            } else {
                                ekilo_set_status_message("Cancel selected - click to cancel without saving");
                            }
                        }
                    } else {  // Left scroll in menu (counter-clockwise)
                        // Scrolling left in menu: Cancel -> Save -> back to end of file
                        E.menu_selection--;
                        if (E.menu_selection < 0) {
                            // Before save option - exit menu to end of file
                            E.in_menu_mode = false;
                            if (E.numrows > 0) {
                                E.cy = E.numrows - 1;
                                E.cx = E.row[E.cy].size;
                            } else {
                                E.cy = 0;
                                E.cx = 0;
                            }
                            ekilo_set_status_message("Back to end of file");
                        } else {
                            // Update menu selection
                            if (E.menu_selection == 0) {
                                ekilo_set_status_message("Save selected - click to save file");
                            } else {
                                ekilo_set_status_message("Cancel selected - click to cancel without saving");
                            }
                        }
                    }
                    ekilo_update_oled_context();
                }
            } else {
                // Use direct delta for responsive cursor movement (horizontal only)
                int steps = max(1, min(abs(deltaPosition), 4)); // 1-4 characters per rotation
                int direction = (deltaPosition > 0) ? 1 : -1;  // Fixed: positive delta = right, negative = left
                
                for (int i = 0; i < steps; i++) {
                    if (direction > 0) {  // Right movement (clockwise scroll)
                        // Moving right - check if we should enter menu mode
                        if (E.cy >= E.numrows || (E.cy == E.numrows - 1 && E.cx >= E.row[E.cy].size)) {
                            // At or past end of file - enter menu mode
                            if (!E.in_menu_mode) {
                                E.in_menu_mode = true;
                                E.menu_selection = 0; // Start with save option
                                ekilo_set_status_message("Menu mode: Use wheel to select Save/Exit, click to confirm");
                                break; // Stop processing more steps
                            }
                        } else {
                            ekilo_move_cursor(ARROW_RIGHT);
                        }
                    } else {
                        // Moving left - check if we should loop to menu
                        if (E.cy == 0 && E.cx == 0) {
                            // At beginning of file - loop to menu
                            E.in_menu_mode = true;
                            E.menu_selection = 1; // Start with cancel (last option)
                            ekilo_set_status_message("Looped to menu from beginning");
                            break;
                        } else {
                            ekilo_move_cursor(ARROW_LEFT);
                        }
                    }
                }
                ekilo_update_oled_context();
            }
        }
    }
    
    // Handle button press with direct digitalRead and debouncing
    bool current_button_state = digitalRead(BUTTON_ENC);
    
    // Check for button press (HIGH to LOW transition) with debouncing
    if (!current_button_state && E.last_button_state && (currentTime - E.button_debounce_time > 50)) {
        E.button_debounce_time = currentTime;
        
        if (E.char_selection_mode) {
            // In character selection mode: confirm character
            ekilo_confirm_character();
        } else if (E.in_menu_mode) {
            // In menu mode: handle menu selection
            if (E.menu_selection == 0) { // Save
                ekilo_save();
                E.in_menu_mode = false; // Exit menu after save
                ekilo_update_oled_context();
            } else if (E.menu_selection == 1) { // Cancel
                E.should_quit = 1;
            }
        } else {
            // Normal mode: enter character selection
            ekilo_enter_char_selection();
            // Initialize encoder position tracking when entering char selection
            E.last_encoder_position = encoderPosition;
        }
    }
    
    E.last_button_state = current_button_state;
}

// Main editor function
int ekilo_main(const char* filename) {
    ekilo_init();
    
    if (filename) {
        ekilo_open(filename);
    }
    
    ekilo_set_status_message("HELP: Ctrl-S/Ctrl-P = save | Ctrl-Q = quit | Clickwheel = move/type");
    
    // Initialize encoder position tracking
    E.last_encoder_position = encoderPosition;
    E.last_encoder_update = millis();
    
    // Initialize button state
    E.last_button_state = digitalRead(BUTTON_ENC);
    E.button_debounce_time = millis();
    
    // Initial OLED update
    ekilo_schedule_oled_update();
    
    while (!E.should_quit) {
        // Process any pending OLED updates
        ekilo_process_oled_update();
        
        // Process encoder input for cursor movement and character selection
        ekilo_process_encoder_input();
        
        ekilo_refresh_screen();
        ekilo_process_keypress();
        delayMicroseconds(1); 
    }
    
    // Cleanup
    for (int i = 0; i < E.numrows; i++) {
        ekilo_free_row(&E.row[i]);
    }
    free(E.row);
    free(E.filename);
    
    return 0;
}

// REPL mode functions
void ekilo_init_repl_mode() {
    E.repl_mode = true;
    // No need to store cursor position - XTerm alternate screen handles this
    E.screenrows = 24; // Use full screen in alternate buffer
    
    // Clear the alternate screen and position at top-left
    Serial.print("\x1b[2J\x1b[H");
    
    // Print a simple header once when entering REPL mode
    Serial.println("eKilo Editor | Ctrl-S/Ctrl-P=save & load | Ctrl-Q=quit | Wheel=navigate");
    Serial.flush();
}

void ekilo_store_cursor_position() {
    // Save current cursor position using terminal escape sequence
    Serial.print("\033[s"); // Save cursor position
    Serial.flush();
    E.lines_used = 0;
}

void ekilo_restore_cursor_position() {
    if (E.repl_mode) {
        // Restore saved cursor position
        Serial.print("\033[u"); // Restore cursor position
        Serial.flush();
    }
}

void ekilo_cleanup_repl_mode() {
    if (!E.repl_mode) return;
    
    // XTerm alternate screen buffer handles all cleanup automatically
    // when we call restoreScreenState() in the calling function
    // No manual cleanup needed here
    
    E.repl_mode = false;
}

// REPL mode main function - returns saved content
String ekilo_main_repl(const char* filename) {
    ekilo_init();
    ekilo_init_repl_mode();
    
    if (filename != nullptr) {
        ekilo_open(filename);
    }
    
    ekilo_set_status_message("REPL Mode | Ctrl-S: Save & Load | Ctrl-Q: Exit");
    
    // Initialize encoder position tracking
    E.last_encoder_position = encoderPosition;
    E.last_encoder_update = millis();
    
    // Initialize button state
    E.last_button_state = digitalRead(BUTTON_ENC);
    E.button_debounce_time = millis();
    
    // Initial OLED update
    ekilo_schedule_oled_update();
    
    // Editor loop with REPL positioning
    while (!E.should_quit) {
        // Process any pending OLED updates
        ekilo_process_oled_update();
        
        // Process encoder input for cursor movement and character selection
        ekilo_process_encoder_input();
        
        ekilo_refresh_screen();
        ekilo_process_keypress();
        delayMicroseconds(1);
    }
    
    // If content was saved, return it
    String savedContent = E.saved_file_content;
    
    // Cleanup
    for (int i = 0; i < E.numrows; i++) {
        ekilo_free_row(&E.row[i]);
    }
    free(E.row);
    free(E.filename);
    
    ekilo_cleanup_repl_mode();
    
    return savedContent;
} 