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
#define HL_JUMPERLESS_FUNC 9
#define HL_JUMPERLESS_TYPE 10
#define HL_JFS_FUNC 11

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
    "tuple|", "type|", "vars|", "zip|", "self|", "cls|",
    
    // Jumperless Functions (marked with ||)
    "dac_set||", "dac_get||", "set_dac||", "get_dac||", "adc_get||", "get_adc||",
    "ina_get_current||", "ina_get_voltage||", "ina_get_bus_voltage||", "ina_get_power||",
    "get_current||", "get_voltage||", "get_bus_voltage||", "get_power||",
    "gpio_set||", "gpio_get||", "gpio_set_dir||", "gpio_get_dir||", "gpio_set_pull||", "gpio_get_pull||",
    "set_gpio||", "get_gpio||", "set_gpio_dir||", "get_gpio_dir||", "set_gpio_pull||", "get_gpio_pull||",
    "connect||", "disconnect||", "is_connected||", "nodes_clear||", "node||",
    "oled_print||", "oled_clear||", "oled_connect||", "oled_disconnect||",
    "clickwheel_up||", "clickwheel_down||", "clickwheel_press||",
    "print_bridges||", "print_paths||", "print_crossbars||", "print_nets||", "print_chip_status||",
    "probe_read||", "read_probe||", "probe_read_blocking||", "probe_read_nonblocking||",
    "get_button||", "probe_button||", "probe_button_blocking||", "probe_button_nonblocking||",
    "probe_wait||", "wait_probe||", "probe_touch||", "wait_touch||", "button_read||", "read_button||",
    "check_button||", "button_check||", "arduino_reset||", "probe_tap||", "run_app||", "format_output||",
    "help_nodes||", "pause_core2||", "send_raw||", "pwm||", "pwm_set_duty_cycle||", "pwm_set_frequency||", "pwm_stop||", "nodes_save||",
    
    // Jumperless Types/Constants (marked with |||)
    "TOP_RAIL|||", "BOTTOM_RAIL|||", "GND|||", "DAC0|||", "DAC1|||", "ADC0|||", "ADC1|||", "ADC2|||", "ADC3|||", "ADC4|||",
    "PROBE|||", "ISENSE_PLUS|||", "ISENSE_MINUS|||", "UART_TX|||", "UART_RX|||", "BUFFER_IN|||", "BUFFER_OUT|||",
    "GPIO_1|||", "GPIO_2|||", "GPIO_3|||", "GPIO_4|||", "GPIO_5|||", "GPIO_6|||", "GPIO_7|||", "GPIO_8|||",
    "D0|||", "D1|||", "D2|||", "D3|||", "D4|||", "D5|||", "D6|||", "D7|||", "D8|||", "D9|||", "D10|||", "D11|||", "D12|||", "D13|||",
    "A0|||", "A1|||", "A2|||", "A3|||", "A4|||", "A5|||", "A6|||", "A7|||",
    "D13_PAD|||", "TOP_RAIL_PAD|||", "BOTTOM_RAIL_PAD|||", "LOGO_PAD_TOP|||", "LOGO_PAD_BOTTOM|||",
    "CONNECT_BUTTON|||", "REMOVE_BUTTON|||", "BUTTON_NONE|||", "CONNECT|||", "REMOVE|||", "NONE|||",
    
    // JFS Functions (marked with ||||)
    "open||||", "read||||", "write||||", "close||||", "seek||||", "tell||||", "size||||", "available||||",
    "exists||||", "listdir||||", "mkdir||||", "rmdir||||", "remove||||", "rename||||", "stat||||", "info||||",
    "SEEK_SET||||", "SEEK_CUR||||", "SEEK_END||||",
    
    // Basic filesystem functions (marked with ||||)
    "fs_exists||||", "fs_listdir||||", "fs_read||||", "fs_write||||", "fs_cwd||||",
    
    nullptr
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
    screenrows = DEFAULT_EDITOR_ROWS;  // Use configurable default
    screencols = DEFAULT_EDITOR_COLS;
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
    
    // Initialize Ctrl+P functionality
    should_launch_repl = false;
    
    // Initialize screen refresh optimization
    screen_dirty = true; // Start with dirty screen to force initial draw
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
    Serial.write(0x0E);
    Serial.flush();
    
    // Try to determine screen size - use conservative defaults for Arduino
    // Reduce available rows by 1 to account for persistent help header
    E.screenrows = DEFAULT_EDITOR_ROWS - 1; // Account for help header
    E.screencols = DEFAULT_EDITOR_COLS;
    
    // Mark screen as dirty for initial draw
    E.screen_dirty = true;
}

// Memory management for dynamic buffer
struct Buffer {
    char* b;
    int len;
};

// Safe memory allocation with size limits
#define MAX_EDITOR_MEMORY (48 * 1024)  // 32KB limit for editor content
#define MIN_FREE_HEAP (3 * 1024)      // Reduce to 5KB free (was 10KB)

bool check_memory_available(size_t needed) {
    size_t freeHeap = rp2040.getFreeHeap();
    return (freeHeap > needed + MIN_FREE_HEAP);
}

void buffer_append(Buffer* ab, const char* s, int len) {
    if (!ab || !s || len <= 0) return;
    
    // Check if we have enough memory
    if (!check_memory_available(len)) {
        ekilo_set_status_message("ERROR: Not enough memory for buffer operation");
        return;
    }
    
    char* new_buf = (char*)realloc(ab->b, ab->len + len);
    if (new_buf == nullptr) {
        ekilo_set_status_message("ERROR: Memory allocation failed");
        return;
    }
    
    memcpy(new_buf + ab->len, s, len);
    ab->b = new_buf;
    ab->len += len;
}

void buffer_free(Buffer* ab) {
    if (!ab) return;
    free(ab->b);
    ab->b = nullptr;
    ab->len = 0;
}

// Memory monitoring and cleanup functions
void ekilo_show_memory_status() {
    size_t freeHeap = rp2040.getFreeHeap();
    size_t usedMemory = 0;
    
    // Calculate memory used by editor content
    for (int i = 0; i < E.numrows; i++) {
        if (E.row[i].chars) usedMemory += E.row[i].size;
        if (E.row[i].render) usedMemory += E.row[i].rsize;
        if (E.row[i].hl) usedMemory += E.row[i].rsize;
    }
    
    ekilo_set_status_message("Memory: %dKB free, %dKB used by editor", 
                           freeHeap / 1024, usedMemory / 1024);
}

// Emergency cleanup function
void ekilo_emergency_cleanup() {
    // Free all allocated memory to prevent further crashes
    for (int i = 0; i < E.numrows; i++) {
        ekilo_free_row(&E.row[i]);
    }
    free(E.row);
    E.row = nullptr;
    E.numrows = 0;
    
    free(E.filename);
    E.filename = nullptr;
    
    ekilo_set_status_message("Emergency cleanup completed - editor reset");
}

// Arduino-compatible key reading
int ekilo_read_key() {
    if (!Serial.available()) return 0;
    
    char c = Serial.read();
    
    // Handle escape sequences (arrow keys, etc.)
    if (c == ESC) {
        // Wait a bit for the rest of the sequence
        unsigned long start = millis();
        // while (millis() - start < 12 && !Serial.available()) {
        //    // delayMicroseconds(1);
        // }
        
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
    if (!row || row->rsize <= 0) return;
    
    unsigned char* new_hl = (unsigned char*)realloc(row->hl, row->rsize);
    if (!new_hl) {
        // If realloc fails, we can still function without syntax highlighting
        free(row->hl);
        row->hl = nullptr;
        return;
    }
    row->hl = new_hl;
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
                int highlight_type = HL_KEYWORD1;
                
                // Check for different keyword types based on suffix
                if (klen >= 4 && !strncmp(&keywords[j][klen - 4], "||||", 4)) {
                    highlight_type = HL_JFS_FUNC;
                    klen -= 4;
                } else if (klen >= 3 && !strncmp(&keywords[j][klen - 3], "|||", 3)) {
                    highlight_type = HL_JUMPERLESS_TYPE;
                    klen -= 3;
                } else if (klen >= 2 && !strncmp(&keywords[j][klen - 2], "||", 2)) {
                    highlight_type = HL_JUMPERLESS_FUNC;
                    klen -= 2;
                } else if (klen >= 1 && keywords[j][klen - 1] == '|') {
                    highlight_type = HL_KEYWORD2;
                    klen--;
                }
                
                if (!strncmp(&row->render[i], keywords[j], klen) &&
                    is_separator(row->render[i + klen])) {
                    memset(&row->hl[i], highlight_type, klen);
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

// Convert syntax highlighting to ANSI color codes (256-color mode)
int ekilo_syntax_to_color(int hl) {
    switch (hl) {
        case HL_COMMENT: return 34; // Green
        case HL_MLCOMMENT: return 244; // Light gray (much more subtle than bright cyan)
        case HL_KEYWORD1: return 214;  // Orange (vibrant and distinct)
        case HL_KEYWORD2: return 79;   // Forest green (rich green for built-ins)
        case HL_STRING: return 39;    
        case HL_NUMBER: return 199;    
        case HL_MATCH: return 27;      
        case HL_JUMPERLESS_FUNC: return 207;  
        case HL_JUMPERLESS_TYPE: return 105; 
        case HL_JFS_FUNC: return 45;   // Cyan-blue (distinct for filesystem functions)
        default: return 255;           // Bright white (default text)
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
    if (!row) return;
    
    int tabs = 0;
    for (int j = 0; j < row->size; j++) {
        if (row->chars[j] == '\t') tabs++;
    }
    
    size_t needed_size = row->size + tabs * 7 + 1;
    if (!check_memory_available(needed_size)) {
        ekilo_set_status_message("ERROR: Not enough memory for row update");
        return;
    }
    
    free(row->render);
    row->render = (char*)malloc(needed_size);
    if (!row->render) {
        ekilo_set_status_message("ERROR: Memory allocation failed for row render");
        return;
    }
    
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
    if (!s) return;
    
    // Check memory limits
    size_t needed_memory = sizeof(EditorRow) * (E.numrows + 1) + len + 1;
    if (!check_memory_available(needed_memory)) {
        ekilo_set_status_message("ERROR: Not enough memory to insert row");
        return;
    }
    
    // Check total content size limit
    size_t total_content = 0;
    for (int i = 0; i < E.numrows; i++) {
        total_content += E.row[i].size;
    }
    if (total_content + len > MAX_EDITOR_MEMORY) {
        ekilo_set_status_message("ERROR: File too large (max %dKB)", MAX_EDITOR_MEMORY / 1024);
        return;
    }
    
    EditorRow* new_rows = (EditorRow*)realloc(E.row, sizeof(EditorRow) * (E.numrows + 1));
    if (!new_rows) {
        ekilo_set_status_message("ERROR: Memory allocation failed for row array");
        return;
    }
    E.row = new_rows;
    
    memmove(&E.row[at + 1], &E.row[at], sizeof(EditorRow) * (E.numrows - at));
    for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    
    E.row[at].idx = at;
    E.row[at].size = len;
    E.row[at].chars = (char*)malloc(len + 1);
    if (!E.row[at].chars) {
        ekilo_set_status_message("ERROR: Memory allocation failed for row content");
        // Revert the row array changes
        memmove(&E.row[at], &E.row[at + 1], sizeof(EditorRow) * (E.numrows - at));
        for (int j = at; j < E.numrows; j++) E.row[j].idx--;
        return;
    }
    
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
    if (!row || at < 0 || at > row->size) {
        if (row && (at < 0 || at > row->size)) at = row->size;
        else return;
    }
    
    if (!check_memory_available(row->size + 2)) {
        ekilo_set_status_message("ERROR: Not enough memory to insert character");
        return;
    }
    
    char* new_chars = (char*)realloc(row->chars, row->size + 2);
    if (!new_chars) {
        ekilo_set_status_message("ERROR: Memory allocation failed for character insertion");
        return;
    }
    row->chars = new_chars;
    
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
    if (!row || !s || len == 0) return;
    
    if (!check_memory_available(row->size + len + 1)) {
        ekilo_set_status_message("ERROR: Not enough memory to append string");
        return;
    }
    
    char* new_chars = (char*)realloc(row->chars, row->size + len + 1);
    if (!new_chars) {
        ekilo_set_status_message("ERROR: Memory allocation failed for string append");
        return;
    }
    row->chars = new_chars;
    
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
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
        
        // When splitting a line, also update the new row to ensure proper syntax highlighting
        if (E.cy + 1 < E.numrows) {
            ekilo_update_row(&E.row[E.cy + 1]);
            
            // Update syntax highlighting for subsequent rows since line split can affect
            // multiline comments and other context-dependent highlighting
            for (int i = E.cy + 2; i < E.numrows; i++) {
                ekilo_update_row(&E.row[i]);
            }
        }
    }
    E.cy++;
    E.cx = 0;
    
    // Reset horizontal scrolling when moving to new line
    E.oled_horizontal_offset = 0;
    
    // Schedule OLED update after newline insertion
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
        // Joining lines - need to update syntax highlighting for subsequent lines
        E.cx = E.row[E.cy - 1].size;
        ekilo_row_append_string(&E.row[E.cy - 1], row->chars, row->size);
        ekilo_del_row(E.cy);
        E.cy--;
        // Reset horizontal scrolling when moving to previous line
        E.oled_horizontal_offset = 0;
        
        // When joining lines, update syntax highlighting for subsequent rows since 
        // line joining can affect multiline comments and other context-dependent highlighting
        for (int i = E.cy; i < E.numrows; i++) {
            ekilo_update_row(&E.row[i]);
        }
    }
    
    // Schedule OLED update after character deletion
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
}

// Set status message
void ekilo_set_status_message(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = millis();
    
    // Make error messages persist much longer (30 seconds vs 5 seconds)
    if (strstr(E.statusmsg, "ERROR:") || strstr(E.statusmsg, "WARNING:")) {
        E.statusmsg_time = millis() - 25000; // Show for 30 seconds instead of 5
    }
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
                
                // Auto-scroll if cursor moves above visible area
                if (E.cy < E.rowoff) {
                    E.rowoff = E.cy;
                }
            }
            break;
        case ARROW_DOWN:
            if (E.cy < E.numrows) {
                E.cy++;
                // Reset horizontal scrolling when changing lines
                E.oled_horizontal_offset = 0;
                
                // Auto-scroll if cursor moves below visible area
                int available_rows = E.repl_mode ? E.screenrows - 3 : E.screenrows - 4;
                if (E.cy >= E.rowoff + available_rows) {
                    E.rowoff = E.cy - available_rows + 1;
                }
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
    
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
}

// Open file
int ekilo_open(const char* filename) {
    if (!filename) return -1;

    
    // Check file exists and get size
    File file = FatFS.open(filename, "r");
    if (!file) {
        ekilo_set_status_message("ERROR: Cannot open file '%s'", filename);
        return -1;
    }
    
    size_t file_size = file.size();
    file.close();
    
    // Check file size limits
    if (file_size > MAX_EDITOR_MEMORY) {
        ekilo_set_status_message("ERROR: File too large (%d KB, max %d KB)", 
                                file_size / 1024, MAX_EDITOR_MEMORY / 1024);
        return -1;
    }
    
    // Check available memory
    if (!check_memory_available(file_size + 1024)) { // Extra 1KB for overhead
        size_t freeHeap = rp2040.getFreeHeap();
        ekilo_set_status_message("ERROR: Not enough memory (%dKB free, need %dKB+%dKB reserve)", 
                                freeHeap / 1024, (file_size + 1024) / 1024, MIN_FREE_HEAP / 1024);
        return -1;
    }
    
    // Free existing filename and allocate new one
    free(E.filename);
    E.filename = strdup(filename);
    if (!E.filename) {
        ekilo_set_status_message("ERROR: Memory allocation failed for filename");
        return -1;
    }
    
    ekilo_select_syntax_highlight(filename);
    
    // Re-open file for reading
    file = FatFS.open(filename, "r");
    if (!file) {
        ekilo_set_status_message("ERROR: Cannot reopen file for reading");
        free(E.filename);
        E.filename = nullptr;
        return -1;
    }
    
    // Track memory usage while loading
    size_t total_loaded = 0;
    size_t line_count = 0;
    const size_t MAX_LINE_LENGTH = 1024; // Reasonable line length limit
    
    while (file.available() && total_loaded < file_size) {
        String line = file.readStringUntil('\n');
        
        // Check line length
        if (line.length() > MAX_LINE_LENGTH) {
            file.close();
            ekilo_set_status_message("ERROR: Line %d too long (max %d chars)", 
                                    line_count + 1, MAX_LINE_LENGTH);
            return -1;
        }
        
        // Only remove trailing carriage return, preserve leading whitespace for indentation
        if (line.endsWith("\r")) {
            line.remove(line.length() - 1);
        }
        
        ekilo_insert_row(E.numrows, line.c_str(), line.length());
        total_loaded += line.length() + 1; // +1 for newline
        line_count++;
        
        // Check memory every 10 lines and show progress for large files
        if (line_count % 10 == 0) {
            if (!check_memory_available(1024)) { // Keep 1KB free
                file.close();
                size_t freeHeap = rp2040.getFreeHeap();
                ekilo_set_status_message("ERROR: Out of memory at line %d (%dKB free)", line_count, freeHeap / 1024);
                return -1;
            }
            
            // Show loading progress for files with many lines
            if (line_count > 100 && line_count % 50 == 0) {
                ekilo_set_status_message("Loading... %d lines (%d bytes)", line_count, total_loaded);
                // Force a quick screen update to show progress
                if (E.screen_dirty) {
                    ekilo_refresh_screen();
                    E.screen_dirty = false;
                }
            }
        }
        
        // Prevent infinite loops on corrupted files
        if (line_count > 10000) { // Reasonable file size limit in lines
            file.close();
            ekilo_set_status_message("ERROR: File too many lines (max 10000)");
            return -1;
        }
    }
    
    file.close();
    E.dirty = 0;
    
    // Mark screen as dirty for refresh after file load
    E.screen_dirty = true;
    
    ekilo_set_status_message("Loaded %d lines (%d bytes)", line_count, total_loaded);
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
    
    // Check memory availability before allocating large buffer
    if (!check_memory_available(len)) {
        ekilo_set_status_message("ERROR: Not enough memory to save file (%d KB needed)", len / 1024);
        return 0;
    }
    
    char* buf = (char*)malloc(len);
    if (!buf) {
        ekilo_set_status_message("ERROR: Memory allocation failed for save buffer");
        return 0;
    }
    
    char* p = buf;
    for (int j = 0; j < E.numrows; j++) {
        if (E.row[j].chars && E.row[j].size > 0) {
            memcpy(p, E.row[j].chars, E.row[j].size);
            p += E.row[j].size;
        }
        *p = '\n';
        p++;
    }
    
    File file = FatFS.open(E.filename, "w");
    if (file) {
        file.write((uint8_t*)buf, len);
        file.close();
        
        // If in REPL mode, store content for return (only if reasonable size)
        if (E.repl_mode && len < 8192) { // Limit stored content to 8KB
            E.saved_file_content = String(buf, len);
            E.should_quit = 1; // Auto-exit after save in REPL mode
            ekilo_set_status_message("File saved: %s (%d bytes) - content stored for REPL", E.filename, len);
        } else if (E.repl_mode) {
            // File too large for REPL return, just signal completion
            E.should_quit = 1;
            ekilo_set_status_message("File saved: %s (too large for REPL)", E.filename);
        }
        
        free(buf);
        E.dirty = 0;
        ekilo_set_status_message("%d bytes written to flash", len);
        E.screen_dirty = true; // Status message already marks dirty, but be explicit
        return len;
    } else {
        free(buf);
        ekilo_set_status_message("Can't save! I/O error: %s", "File write failed");
        return 0;
    }
}

// Write buffer in chunks for Windows compatibility
void ekilo_write_buffer_chunked(Buffer* ab) {
    if (!ab) return;
    
    if (!ab->b || ab->len == 0) {
        buffer_free(ab);
        return;
    }
    
    const int CHUNK_SIZE = 64; // Write in 64-byte chunks
    const int DELAY_MICROS = 10; // 10us delay between chunks
    
    int bytes_written = 0;
    while (bytes_written < ab->len) {
        int chunk_size = min(CHUNK_SIZE, ab->len - bytes_written);
        
        // Validate chunk boundaries
        if (chunk_size <= 0 || bytes_written + chunk_size > ab->len) {
            break; // Prevent buffer overrun
        }
        
        // Write this chunk
        Serial.write((uint8_t*)(ab->b + bytes_written), chunk_size);
        Serial.flush();
        
        bytes_written += chunk_size;
        
        // Add delay between chunks (except for the last one)
        if (bytes_written < ab->len) {
            delayMicroseconds(DELAY_MICROS);
        }
    }
    
    buffer_free(ab);
}

// Refresh screen - Buffered approach with chunked output for Windows compatibility
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
        // In REPL mode, clear and position at top-left
        // No header in REPL mode for cleaner interface
        buffer_append(&ab, "\x1b[2J\x1b[H", 7);
    } else {
        buffer_append(&ab, "\x1b[2J\x1b[H", 7);
        
        // Add persistent help header that stays visible (only in non-REPL mode)
        buffer_append(&ab, "\x1b[48;5;199m\x1b[38;5;236m", 27); // Blue background, white text
        char help_header[128];
        snprintf(help_header, sizeof(help_header), 
                     "                    Jumperless Kilo Text Editor                      ");
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
                        int clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%dm", color);
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
        
        // Clear to end of line and add newline
        buffer_append(&ab, "\x1b[K\r\n", 6);
    }
    
    // Status bar with reverse video
    buffer_append(&ab, "\x1b[7m", 4);
    char status[120], rstatus[80];
    
    // Calculate available space for filename
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.numrows);
    const char* suffix = E.dirty ? " (modified)" : "";
    const char* menu_suffix = E.in_menu_mode ? " [MENU MODE]" : "";
    
    // Calculate space needed for " - XX lines" + suffix + menu + right status + padding
    int fixed_space = strlen(" - ") + 10 + strlen(" lines") + strlen(suffix) + strlen(menu_suffix) + rlen + 2;
    int available_for_filename = E.screencols - fixed_space;
    
    // Ensure we have at least some space for filename
    if (available_for_filename < 10) {
        available_for_filename = 10;
    }
    
    const char* display_filename = E.filename ? E.filename : "[No Name]";
    
    // Build status string with dynamic filename length
    int len = snprintf(status, sizeof(status), "%.*s - %d lines%s%s",
        available_for_filename, display_filename, E.numrows, suffix, menu_suffix);
    
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
    // Show error/warning messages for 30 seconds, normal messages for 5 seconds
    uint32_t timeout = (strstr(E.statusmsg, "ERROR:") || strstr(E.statusmsg, "WARNING:")) ? 30000 : 5000;
    if (msglen && millis() - E.statusmsg_time < timeout) {
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
             "Ctrl-S = Save │ Ctrl-Q = Quit │ ↑/↓ = Navigate | CTRL-P = Save and load in MicroPython");
    snprintf(help_line2, sizeof(help_line2),
             "Tab = Indent │ Backspace = Delete │ Wheel=Move/Type │ Ctrl-U = Memory Status");
    
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
    
    // Write buffer in chunks for Windows compatibility
    ekilo_write_buffer_chunked(&ab);
    
    // Schedule OLED update with context around cursor
    ekilo_schedule_oled_update();
    
    // Note: Don't set screen_dirty here - we only call this when screen is already dirty
    // OLED update will be processed later when screen is clean and serial buffer is empty
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
            ekilo_save();
            break;
            
        case CTRL_P:
            // Save and launch MicroPython REPL
            ekilo_save();
            E.should_launch_repl = true;
            E.should_quit = 1;
            break;
            
        case CTRL_U:
            // Show detailed memory status including logic analyzer and MicroPython usage
            {
                size_t freeHeap = rp2040.getFreeHeap();
                size_t usedByEditor = 0;
                
                // Calculate memory used by editor content
                for (int i = 0; i < E.numrows; i++) {
                    if (E.row[i].chars) usedByEditor += E.row[i].size;
                    if (E.row[i].render) usedByEditor += E.row[i].rsize;
                    if (E.row[i].hl) usedByEditor += E.row[i].rsize;
                }
                
                ekilo_set_status_message("Memory: %dKB free, Editor:%dKB, MP:64KB, LA:~24KB, Need:%dKB reserve", 
                                       freeHeap / 1024, usedByEditor / 1024, MIN_FREE_HEAP / 1024);
            }
            E.screen_dirty = true;
            break;
            
        case HOME_KEY:
            E.cx = 0;
            E.oled_horizontal_offset = 0; // Reset scrolling when going to start of line
            ekilo_schedule_oled_update();
            E.screen_dirty = true;
            break;
            
        case END_KEY:
            if (E.cy < E.numrows)
                E.cx = E.row[E.cy].size;
            // Don't reset horizontal scrolling for END - let it scroll to show end of line
            ekilo_schedule_oled_update();
            E.screen_dirty = true;
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
            E.screen_dirty = true;
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
                E.screen_dirty = true;
            }
            break;
            
        default:
            ekilo_insert_char(c);
            break;
    }
    
    quit_times = 1;
}

// OLED update batching functions
void ekilo_schedule_oled_update() {
    unsigned long currentTime = millis();
    
    // Always mark as pending - we'll process it when conditions are right
    E.oled_last_input_time = currentTime;
    E.oled_update_pending = true;
}

void ekilo_process_oled_update() {
    // Only update OLED if:
    // 1. There's a pending update
    // 2. Screen is clean (not dirty) for better responsiveness
    // 3. No serial input pending (to avoid interrupting input processing)
    // 4. Enough time has passed since last input
    if (E.oled_update_pending && 
        !E.screen_dirty && 
        !Serial.available() && 
        (millis() - E.oled_last_input_time) >= 50) { // 50ms delay
        
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
    
    // Schedule OLED update when entering character selection mode
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
}

// Exit character selection mode
void ekilo_exit_char_selection() {
    E.char_selection_mode = false;
    ekilo_set_status_message("Character selection cancelled");
    
    // Schedule OLED update when exiting character selection mode
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
    
    // Schedule OLED update in character selection mode for responsiveness
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
    
    // Schedule OLED update after character insertion
    ekilo_schedule_oled_update();
    
    // Mark screen as dirty for refresh
    E.screen_dirty = true;
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
                    ekilo_schedule_oled_update();
                    E.screen_dirty = true;
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
                ekilo_schedule_oled_update();
                E.screen_dirty = true;
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
        if (ekilo_open(filename) != 0) {
            // File opening failed - show error and continue with empty editor
            ekilo_set_status_message("Failed to open file - starting with empty editor");
        }
    }
    
    ekilo_set_status_message("HELP: Ctrl-S = save | Ctrl-P = save & REPL | Ctrl-Q = quit | Clickwheel = move/type");
    
    // Initialize encoder position tracking
    E.last_encoder_position = encoderPosition;
    E.last_encoder_update = millis();
    
    // Initialize button state
    E.last_button_state = digitalRead(BUTTON_ENC);
    E.button_debounce_time = millis();
    
    // Initial OLED update
    ekilo_schedule_oled_update();
    
    // Memory monitoring
    unsigned long last_memory_check = millis();
    const unsigned long MEMORY_CHECK_INTERVAL = 5000; // Check every 5 seconds
    
    while (!E.should_quit) {
        // Periodic memory monitoring
        // unsigned long current_time = millis();
        // if (current_time - last_memory_check > MEMORY_CHECK_INTERVAL) {
        //     size_t freeHeap = rp2040.getFreeHeap();
        //     if (freeHeap < MIN_FREE_HEAP) {
        //         ekilo_set_status_message("WARNING: Low memory (%dKB free) - save your work!", freeHeap / 1024);
        //         E.screen_dirty = true;
        //     }
        //     last_memory_check = current_time;
        // }
        
        // Process any pending OLED updates
        //ekilo_process_oled_update();
        ekilo_process_keypress();
        // Process encoder input for cursor movement and character selection
        //ekilo_process_encoder_input();
        
        // Only refresh screen if something changed
        if (E.screen_dirty) {
            ekilo_refresh_screen();
            E.screen_dirty = false;
        }
        
        
        //delayMicroseconds(1); 
    }
    
    // Check if Ctrl+P was pressed (save and launch REPL)
    int result = E.should_launch_repl ? 2 : 0; // 2 = launch REPL, 0 = normal exit
    
    // Cleanup
    for (int i = 0; i < E.numrows; i++) {
        ekilo_free_row(&E.row[i]);
    }
    free(E.row);
    free(E.filename);
    
    return result;
}

// REPL mode functions
void ekilo_init_repl_mode() {
    E.repl_mode = true;
    // No need to store cursor position - XTerm alternate screen handles this
    E.screenrows = DEFAULT_EDITOR_ROWS; // Use configurable screen size in alternate buffer
    Serial.write(0x0E);
    Serial.flush();    
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
        // Try to open existing file, but if it fails, still set the filename for new files
        if (ekilo_open(filename) != 0) {
            // File doesn't exist - set the filename for saving new file
            free(E.filename);
            E.filename = strdup(filename);
            if (!E.filename) {
                ekilo_set_status_message("ERROR: Memory allocation failed for filename");
            } else {
                // Ensure the directory exists for the new file
                String file_path = String(filename);
                int last_slash = file_path.lastIndexOf('/');
                if (last_slash > 0) {
                    String dir_path = file_path.substring(0, last_slash);
                    if (!FatFS.exists(dir_path.c_str())) {
                        FatFS.mkdir(dir_path.c_str());
                    }
                }
                // Enable Python syntax highlighting for new files
                ekilo_select_syntax_highlight(filename);
                ekilo_set_status_message("Creating new file: %s", filename);
                E.screen_dirty = true; // Force initial screen refresh with new filename
            }
        }
    }
    
    if (E.filename) {
        ekilo_set_status_message("Editing: %s | Ctrl-S: Save & Load | Ctrl-P: Save & Launch REPL | Ctrl-Q: Exit", E.filename);
    } else {
        ekilo_set_status_message("REPL Mode | Ctrl-S: Save & Load | Ctrl-P: Save & Launch REPL | Ctrl-Q: Exit");
    }
    
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

        ekilo_process_keypress();

        
        
        // Process encoder input for cursor movement and character selection
        ekilo_process_encoder_input();
        ekilo_process_oled_update();
        // Only refresh screen if something changed
        if (E.screen_dirty) {
            ekilo_refresh_screen();
            E.screen_dirty = false;
        }
        
        
       // delayMicroseconds(1);
    }
    
    // If content was saved, return it with Ctrl+P prefix if needed
    String savedContent = E.saved_file_content;
    
    // If Ctrl+P was pressed, prepend a special marker to indicate REPL should be launched
    if (E.should_launch_repl && savedContent.length() > 0) {
        savedContent = "[LAUNCH_REPL]" + savedContent;
    }
    
    // Cleanup
    for (int i = 0; i < E.numrows; i++) {
        ekilo_free_row(&E.row[i]);
    }
    free(E.row);
    free(E.filename);
    
    ekilo_cleanup_repl_mode();
    
    return savedContent;
}

// Inline editing mode - provides eKilo-style editing without screen clearing or file operations
String ekilo_inline_edit(const String& initial_content) {
    // Initialize a minimal editor state without clearing screen
    EditorConfig inline_editor;
    inline_editor.cx = 0;
    inline_editor.cy = 0;
    inline_editor.rowoff = 0;
    inline_editor.coloff = 0;
    inline_editor.numrows = 0;
    inline_editor.row = nullptr;
    inline_editor.dirty = 0;
    inline_editor.filename = nullptr;
    inline_editor.syntax = &syntax_db[0]; // Set Python syntax highlighting
    inline_editor.should_quit = 0;
    strcpy(inline_editor.statusmsg, "");
    inline_editor.statusmsg_time = 0;
    
    // Helper functions for inline syntax highlighting
    auto inline_update_syntax = [&](EditorRow* row) {
        if (!row || row->rsize <= 0 || !inline_editor.syntax) return;
        
        unsigned char* new_hl = (unsigned char*)realloc(row->hl, row->rsize);
        if (!new_hl) {
            free(row->hl);
            row->hl = nullptr;
            return;
        }
        row->hl = new_hl;
        memset(row->hl, HL_NORMAL, row->rsize);
        
        const char** keywords = inline_editor.syntax->keywords;
        const char* scs = inline_editor.syntax->singleline_comment_start;
        int scs_len = scs ? strlen(scs) : 0;
        
        int prev_sep = 1;
        int in_string = 0;
        
        int i = 0;
        while (i < row->rsize) {
            char c = row->render[i];
            unsigned char prev_hl = (i > 0) ? row->hl[i - 1] : HL_NORMAL;
            
            // Handle comments
            if (scs_len && !in_string) {
                if (!strncmp(&row->render[i], scs, scs_len)) {
                    memset(&row->hl[i], HL_COMMENT, row->rsize - i);
                    break;
                }
            }
            
            // Handle strings
            if (inline_editor.syntax->flags & HL_HIGHLIGHT_STRINGS) {
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
            
            // Handle numbers
            if (inline_editor.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
                if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                    (c == '.' && prev_hl == HL_NUMBER)) {
                    row->hl[i] = HL_NUMBER;
                    i++;
                    prev_sep = 0;
                    continue;
                }
            }
            
                            // Handle keywords
                if (prev_sep) {
                    int j;
                    for (j = 0; keywords[j]; j++) {
                        int klen = strlen(keywords[j]);
                        int highlight_type = HL_KEYWORD1;
                        
                        // Check for different keyword types based on suffix
                        if (klen >= 3 && !strncmp(&keywords[j][klen - 3], "|||", 3)) {
                            highlight_type = HL_JUMPERLESS_TYPE;
                            klen -= 3;
                        } else if (klen >= 2 && !strncmp(&keywords[j][klen - 2], "||", 2)) {
                            highlight_type = HL_JUMPERLESS_FUNC;
                            klen -= 2;
                        } else if (klen >= 1 && keywords[j][klen - 1] == '|') {
                            highlight_type = HL_KEYWORD2;
                            klen--;
                        }
                        
                        if (!strncmp(&row->render[i], keywords[j], klen) &&
                            is_separator(row->render[i + klen])) {
                            memset(&row->hl[i], highlight_type, klen);
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
    };
    
    // Helper function to display a line with syntax highlighting
    auto display_line_with_syntax = [&](const char* line, int line_length, bool is_current_line) {
        if (!line || line_length == 0) return;
        
        // Create temporary render buffer for this line
        char* render = (char*)malloc(line_length + 1);
        unsigned char* hl = (unsigned char*)malloc(line_length);
        if (!render || !hl) {
            free(render);
            free(hl);
            Serial.print(line);
            return;
        }
        
        // Simple render (no tabs for inline editor)
        memcpy(render, line, line_length);
        render[line_length] = '\0';
        memset(hl, HL_NORMAL, line_length);
        
        // Apply syntax highlighting
        if (inline_editor.syntax) {
            const char** keywords = inline_editor.syntax->keywords;
            const char* scs = inline_editor.syntax->singleline_comment_start;
            int scs_len = scs ? strlen(scs) : 0;
            
            int prev_sep = 1;
            int in_string = 0;
            
            for (int i = 0; i < line_length; i++) {
                char c = render[i];
                unsigned char prev_hl = (i > 0) ? hl[i - 1] : HL_NORMAL;
                
                // Handle comments
                if (scs_len && !in_string) {
                    if (i <= line_length - scs_len && !strncmp(&render[i], scs, scs_len)) {
                        for (int j = i; j < line_length; j++) hl[j] = HL_COMMENT;
                        break;
                    }
                }
                
                // Handle strings
                if (inline_editor.syntax->flags & HL_HIGHLIGHT_STRINGS) {
                    if (in_string) {
                        hl[i] = HL_STRING;
                        if (c == '\\' && i + 1 < line_length) {
                            hl[i + 1] = HL_STRING;
                            i++;
                            continue;
                        }
                        if (c == in_string) in_string = 0;
                        prev_sep = 1;
                        continue;
                    } else {
                        if (c == '"' || c == '\'') {
                            in_string = c;
                            hl[i] = HL_STRING;
                            continue;
                        }
                    }
                }
                
                // Handle numbers
                if (inline_editor.syntax->flags & HL_HIGHLIGHT_NUMBERS) {
                    if ((isdigit(c) && (prev_sep || prev_hl == HL_NUMBER)) ||
                        (c == '.' && prev_hl == HL_NUMBER)) {
                        hl[i] = HL_NUMBER;
                        prev_sep = 0;
                        continue;
                    }
                }
                
                // Handle keywords
                if (prev_sep) {
                    int j;
                    for (j = 0; keywords[j]; j++) {
                        int klen = strlen(keywords[j]);
                        int highlight_type = HL_KEYWORD1;
                        
                        // Check for different keyword types based on suffix
                        if (klen >= 4 && !strncmp(&keywords[j][klen - 4], "||||", 4)) {
                            highlight_type = HL_JFS_FUNC;
                            klen -= 4;
                        } else if (klen >= 3 && !strncmp(&keywords[j][klen - 3], "|||", 3)) {
                            highlight_type = HL_JUMPERLESS_TYPE;
                            klen -= 3;
                        } else if (klen >= 2 && !strncmp(&keywords[j][klen - 2], "||", 2)) {
                            highlight_type = HL_JUMPERLESS_FUNC;
                            klen -= 2;
                        } else if (klen >= 1 && keywords[j][klen - 1] == '|') {
                            highlight_type = HL_KEYWORD2;
                            klen--;
                        }
                        
                        if (i <= line_length - klen && !strncmp(&render[i], keywords[j], klen) &&
                            (i + klen >= line_length || is_separator(render[i + klen]))) {
                            for (int k = 0; k < klen; k++) {
                                hl[i + k] = highlight_type;
                            }
                            i += klen - 1;
                            break;
                        }
                    }
                    if (keywords[j] != nullptr) {
                        prev_sep = 0;
                        continue;
                    }
                }
                
                prev_sep = is_separator(c);
            }
        }
        
        // Display the line with colors
        int current_color = -1;
        for (int i = 0; i < line_length; i++) {
            int color = ekilo_syntax_to_color(hl[i]);
            if (color != current_color) {
                if (current_color != -1) {
                    Serial.print("\x1b[0m"); // Reset
                }
                current_color = color;
                Serial.print("\x1b[38;5;");
                Serial.print(color);
                Serial.print("m");
            }
            Serial.write(render[i]);
        }
        if (current_color != -1) {
            Serial.print("\x1b[0m"); // Reset at end
        }
        
        free(render);
        free(hl);
    };
    
    // Helper function to calculate Python autoindent
    auto calculate_python_indent = [&](const String& line) -> String {
        // Calculate current indentation level
        int current_indent = 0;
        for (int i = 0; i < line.length(); i++) {
            if (line.charAt(i) == ' ') {
                current_indent++;
            } else {
                break;
            }
        }
        
        String trimmed_line = line;
        trimmed_line.trim();
        
        String indent_spaces = "";
        if (trimmed_line.endsWith(":")) {
            // Increase indentation level by 4 spaces for Python blocks
            for (int i = 0; i < current_indent + 4; i++) {
                indent_spaces += " ";
            }
        } else if (current_indent > 0) {
            // Maintain current indentation level
            for (int i = 0; i < current_indent; i++) {
                indent_spaces += " ";
            }
        }
        
        return indent_spaces;
    };
    
    // Don't clear screen - we're working inline
    Serial.println("--- Python Inline Editor Mode ---");
    Serial.println("Ctrl+S: Accept and return | Ctrl+Q: Cancel | Arrow keys: Navigate");
    // Serial.println("Auto-indent and syntax highlighting enabled");
    Serial.println();
    
    // Load initial content if provided
    if (initial_content.length() > 0) {
        // Split content by lines and insert each row
        int start = 0;
        int pos = 0;
        while (pos <= initial_content.length()) {
            if (pos == initial_content.length() || initial_content[pos] == '\n') {
                String line = initial_content.substring(start, pos);
                
                // Insert this line into the editor
                if (inline_editor.numrows == 0) {
                    inline_editor.row = (EditorRow*)malloc(sizeof(EditorRow));
                } else {
                    inline_editor.row = (EditorRow*)realloc(inline_editor.row, sizeof(EditorRow) * (inline_editor.numrows + 1));
                }
                
                if (inline_editor.row) {
                    EditorRow* new_row = &inline_editor.row[inline_editor.numrows];
                    new_row->idx = inline_editor.numrows;
                    new_row->size = line.length();
                    new_row->chars = (char*)malloc(line.length() + 1);
                    if (new_row->chars) {
                        memcpy(new_row->chars, line.c_str(), line.length());
                        new_row->chars[line.length()] = '\0';
                        new_row->rsize = 0;
                        new_row->render = nullptr;
                        new_row->hl = nullptr;
                        new_row->hl_oc = 0;
                        
                        // Update render for this row
                        int tabs = 0;
                        for (int j = 0; j < new_row->size; j++) {
                            if (new_row->chars[j] == '\t') tabs++;
                        }
                        
                        new_row->render = (char*)malloc(new_row->size + tabs * 7 + 1);
                        if (new_row->render) {
                            int idx = 0;
                            for (int j = 0; j < new_row->size; j++) {
                                if (new_row->chars[j] == '\t') {
                                    new_row->render[idx++] = ' ';
                                    while (idx % 8 != 0) new_row->render[idx++] = ' ';
                                } else {
                                    new_row->render[idx++] = new_row->chars[j];
                                }
                            }
                            new_row->render[idx] = '\0';
                            new_row->rsize = idx;
                        }
                        
                        inline_editor.numrows++;
                    }
                }
                start = pos + 1;
            }
            pos++;
        }
        
        // Position cursor at end of content
        if (inline_editor.numrows > 0) {
            inline_editor.cy = inline_editor.numrows - 1;
            inline_editor.cx = inline_editor.row[inline_editor.cy].size;
        }
    }
    
    // Display current content with syntax highlighting
    for (int i = 0; i < inline_editor.numrows; i++) {
        if (inline_editor.row[i].chars) {
            Serial.print("... ");
            display_line_with_syntax(inline_editor.row[i].chars, inline_editor.row[i].size, false);
            Serial.println();
        }
    }
    
    // Show current prompt
    if (inline_editor.numrows == 0) {
        Serial.print(">>> ");
    } else {
        Serial.print("... ");
    }
    Serial.flush();
    
    // Simple editing loop - much simpler than full eKilo
    String current_line = "";
    if (inline_editor.numrows > 0 && inline_editor.cy < inline_editor.numrows) {
        current_line = String(inline_editor.row[inline_editor.cy].chars);
    }
    
    while (!inline_editor.should_quit) {
        if (Serial.available()) {
            int c = Serial.read();
            
            // Handle escape sequences (arrow keys) - consume completely
            if (c == 27) { // ESC
                // Wait for more characters and consume the entire sequence
                unsigned long start_time = millis();
                while (millis() - start_time < 10) { // Wait up to 10ms
                    if (Serial.available() >= 2) break;
                    delayMicroseconds(100);
                }
                
                if (Serial.available() >= 2) {
                    char seq1 = Serial.read();
                    char seq2 = Serial.read();
                    if (seq1 == '[') {
                        switch (seq2) {
                            case 'A': // Up arrow - navigate to previous line
                                if (inline_editor.cy > 0) {
                                    // Save current line
                                    if (inline_editor.cy < inline_editor.numrows) {
                                        free(inline_editor.row[inline_editor.cy].chars);
                                        inline_editor.row[inline_editor.cy].chars = (char*)malloc(current_line.length() + 1);
                                        if (inline_editor.row[inline_editor.cy].chars) {
                                            memcpy(inline_editor.row[inline_editor.cy].chars, current_line.c_str(), current_line.length());
                                            inline_editor.row[inline_editor.cy].chars[current_line.length()] = '\0';
                                            inline_editor.row[inline_editor.cy].size = current_line.length();
                                        }
                                    }
                                    
                                    // Move to previous line
                                    inline_editor.cy--;
                                    current_line = String(inline_editor.row[inline_editor.cy].chars);
                                    
                                    // Redraw current line
                                    Serial.print("\r... ");
                                    display_line_with_syntax(current_line.c_str(), current_line.length(), true);
                                }
                                break;
                                
                            case 'B': // Down arrow - navigate to next line
                                if (inline_editor.cy < inline_editor.numrows - 1) {
                                    // Save current line
                                    if (inline_editor.cy < inline_editor.numrows) {
                                        free(inline_editor.row[inline_editor.cy].chars);
                                        inline_editor.row[inline_editor.cy].chars = (char*)malloc(current_line.length() + 1);
                                        if (inline_editor.row[inline_editor.cy].chars) {
                                            memcpy(inline_editor.row[inline_editor.cy].chars, current_line.c_str(), current_line.length());
                                            inline_editor.row[inline_editor.cy].chars[current_line.length()] = '\0';
                                            inline_editor.row[inline_editor.cy].size = current_line.length();
                                        }
                                    }
                                    
                                    // Move to next line
                                    inline_editor.cy++;
                                    current_line = String(inline_editor.row[inline_editor.cy].chars);
                                    
                                    // Redraw current line
                                    Serial.print("\r... ");
                                    display_line_with_syntax(current_line.c_str(), current_line.length(), true);
                                }
                                break;
                                
                            case 'C': // Right arrow - ignore for now
                            case 'D': // Left arrow - ignore for now
                                break;
                        }
                    }
                } else {
                    // Consume any remaining escape sequence characters
                    while (Serial.available() > 0) {
                        Serial.read();
                    }
                }
                continue;
            }
            
            switch (c) {
                case 19: // Ctrl+S - Accept and return content
                {
                    Serial.println();
                    Serial.println("--- Content Accepted ---");
                    
                    // Build final content string
                    String final_content = "";
                    
                    // Add all rows except the last one
                    for (int i = 0; i < inline_editor.numrows; i++) {
                        if (i == inline_editor.cy) {
                            // Use the edited current line
                            final_content += current_line;
                        } else if (inline_editor.row[i].chars) {
                            final_content += String(inline_editor.row[i].chars);
                        }
                        if (i < inline_editor.numrows - 1) {
                            final_content += "\n";
                        }
                    }
                    
                    // If we're on a new line or have no rows, add current line
                    if (inline_editor.numrows == 0 || inline_editor.cy >= inline_editor.numrows) {
                        if (final_content.length() > 0) final_content += "\n";
                        final_content += current_line;
                    }
                    
                    // Cleanup
                    for (int i = 0; i < inline_editor.numrows; i++) {
                        free(inline_editor.row[i].chars);
                        free(inline_editor.row[i].render);
                        free(inline_editor.row[i].hl);
                    }
                    free(inline_editor.row);
                    
                    return final_content;
                }
                
                case 17: // Ctrl+Q - Cancel
                    Serial.println();
                    Serial.println("--- Editing Cancelled ---");
                    
                    // Cleanup
                    for (int i = 0; i < inline_editor.numrows; i++) {
                        free(inline_editor.row[i].chars);
                        free(inline_editor.row[i].render);
                        free(inline_editor.row[i].hl);
                    }
                    free(inline_editor.row);
                    
                    return "";
                
                case '\r':
                case '\n': // Enter - new line with autoindent
                {
                    Serial.println();
                    
                    // Calculate autoindent for the next line based on current line
                    String auto_indent = calculate_python_indent(current_line);
                    
                    // Add current line to editor
                    if (inline_editor.numrows == 0) {
                        inline_editor.row = (EditorRow*)malloc(sizeof(EditorRow));
                    } else {
                        inline_editor.row = (EditorRow*)realloc(inline_editor.row, sizeof(EditorRow) * (inline_editor.numrows + 1));
                    }
                    
                    if (inline_editor.row) {
                        EditorRow* new_row = &inline_editor.row[inline_editor.numrows];
                        new_row->idx = inline_editor.numrows;
                        new_row->size = current_line.length();
                        new_row->chars = (char*)malloc(current_line.length() + 1);
                        if (new_row->chars) {
                            memcpy(new_row->chars, current_line.c_str(), current_line.length());
                            new_row->chars[current_line.length()] = '\0';
                            new_row->rsize = current_line.length();
                            new_row->render = (char*)malloc(current_line.length() + 1);
                            if (new_row->render) {
                                memcpy(new_row->render, current_line.c_str(), current_line.length());
                                new_row->render[current_line.length()] = '\0';
                            }
                            new_row->hl = nullptr;
                            new_row->hl_oc = 0;
                            inline_update_syntax(new_row);
                            inline_editor.numrows++;
                        }
                    }
                    
                    // Move to new line with autoindent
                    inline_editor.cy = inline_editor.numrows;
                    current_line = auto_indent;
                    
                    // Show prompt with autoindent
                    Serial.print("... ");
                    if (auto_indent.length() > 0) {
                        Serial.print("\x1b[90m"); // Dark gray for indent
                        Serial.print(auto_indent);
                        Serial.print("\x1b[0m"); // Reset color
                    }
                    break;
                }
                
                case '\b':
                case 127: // Backspace
                    if (current_line.length() > 0) {
                        current_line = current_line.substring(0, current_line.length() - 1);
                        Serial.print("\b \b"); // Erase character
                    }
                    break;
                
                case '\t': // Tab - smart Python indent (4 spaces)
                    current_line += "    ";
                    Serial.print("\x1b[90m    \x1b[0m"); // Show indent in gray
                    break;
                
                default:
                    if (c >= 32 && c <= 126) { // Printable characters
                        current_line += (char)c;
                        
                        // For real-time syntax highlighting, redraw the current line
                        // but only every few characters to avoid flickering
                        static int char_count = 0;
                        char_count++;
                        
                        if (char_count % 3 == 0 || c == ':' || c == '"' || c == '\'' || c == '#') {
                            // Redraw line with syntax highlighting for important characters
                            Serial.print("\r... ");
                            display_line_with_syntax(current_line.c_str(), current_line.length(), true);
                        } else {
                            // Just echo the character for performance
                            Serial.write(c);
                        }
                    }
                    break;
            }
            Serial.flush();
        }
        delayMicroseconds(100);
    }
    
    // Cleanup (shouldn't reach here normally)
    for (int i = 0; i < inline_editor.numrows; i++) {
        free(inline_editor.row[i].chars);
        free(inline_editor.row[i].render);
        free(inline_editor.row[i].hl);
    }
    free(inline_editor.row);
    
    return "";
} 