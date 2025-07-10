/*
 * EkiloEditor.h - Arduino-compatible eKilo text editor for Jumperless
 * Based on the original eKilo editor by Antonio Foti
 * Adapted for Arduino/embedded systems
 */

#ifndef EKILO_EDITOR_H
#define EKILO_EDITOR_H

#include <Arduino.h>
#include <FatFS.h>
#include <cstdarg>
#include <cstdlib>
#include <cstring>

// Forward declarations
struct EditorRow;
struct SyntaxDefinition;

// Editor configuration - make screen size configurable
#define DEFAULT_EDITOR_ROWS 35  // Increased from smaller default to 25 rows
#define DEFAULT_EDITOR_COLS 80

// Editor configuration structure
struct EditorConfig {
    int cx, cy;           // Cursor position
    int rx;               // Render cursor position (for tabs)
    int rowoff;           // Row offset for scrolling
    int coloff;           // Column offset for scrolling
    int screenrows;       // Number of rows on screen
    int screencols;       // Number of columns on screen
    int numrows;          // Number of rows in file
    EditorRow* row;        // Array of editor rows
    int dirty;            // File modified flag
    char* filename;       // Current filename
    char statusmsg[80];   // Status message
    unsigned long statusmsg_time; // Status message timestamp
    SyntaxDefinition* syntax; // Current syntax highlighting
    
    // Search functionality
    char* search_query;
    int search_direction;
    int search_last_match;
    
    // Character insertion mode tracking
    bool char_selection_mode;
    int selected_char_index;
    unsigned long char_selection_timer;
    static const unsigned long CHAR_SELECTION_TIMEOUT = 5000; // 5 seconds timeout for character selection
    
    // OLED update batching - optimized for responsiveness
    unsigned long oled_last_input_time;
    bool oled_update_pending;
    static const unsigned long OLED_UPDATE_DELAY = 350000; // 350 milliseconds for responsive updates
    // Note: OLED updates only happen when screen is clean and serial buffer is empty
    
    // OLED horizontal scrolling for all visible lines
    int oled_horizontal_offset;
    int oled_cursor_position;
    static const int OLED_SCROLL_MARGIN = 4; // Start scrolling when within 4 chars of edge
    
    // Encoder position tracking for smooth movement
    long last_encoder_position;
    unsigned long last_encoder_update;
    
    // Direct button state tracking
    bool last_button_state;
    unsigned long button_debounce_time;
    
    // Special menu navigation (save/exit options)
    bool in_menu_mode;
    int menu_selection; // 0 = save, 1 = exit
    
    // REPL mode and positioning
    bool repl_mode;
    int original_cursor_row;
    int original_cursor_col;
    int start_row;
    int lines_used;
    String saved_file_content; // Content of saved file for REPL return
    
    // Ctrl+P functionality - save and launch MicroPython REPL
    bool should_launch_repl;
    
    // Quit flag
    bool should_quit;
    
    // Screen refresh optimization
    bool screen_dirty;     // Flag to track when screen needs refresh
    
    EditorConfig();
    ~EditorConfig();
};

// Syntax highlighting definition
struct SyntaxDefinition {
    const char** filematch;           // File extension matches
    const char** keywords;            // Keywords to highlight
    const char* singleline_comment_start; // Single line comment start
    const char* multiline_comment_start;   // Multi-line comment start
    const char* multiline_comment_end;     // Multi-line comment end
    int flags;                        // Highlighting flags
};

// Main editor functions
void ekilo_init();
int ekilo_main(const char* filename); // Returns: 0=normal exit, 2=save and launch REPL
String ekilo_main_repl(const char* filename); // REPL mode - returns saved content, "[LAUNCH_REPL]" prefix if Ctrl+P
String ekilo_inline_edit(const String& initial_content = ""); // Inline editing mode - returns content directly without file operations
int ekilo_open(const char* filename);
int ekilo_save();
void ekilo_refresh_screen();
void ekilo_process_keypress();
void ekilo_set_status_message(const char* fmt, ...);

// REPL mode functions
void ekilo_init_repl_mode();
void ekilo_store_cursor_position();
void ekilo_restore_cursor_position();
void ekilo_cleanup_repl_mode();

// Text manipulation functions
void ekilo_insert_char(int c);
void ekilo_insert_newline();
void ekilo_del_char();
void ekilo_move_cursor(int key);

// Row management functions
void ekilo_insert_row(int at, const char* s, size_t len);
void ekilo_del_row(int at);
void ekilo_free_row(EditorRow* row);

// Input handling
int ekilo_read_key();

// Syntax highlighting
void ekilo_select_syntax_highlight(const char* filename);
void ekilo_update_syntax(EditorRow* row);
int ekilo_syntax_to_color(int hl);

// OLED display functions
void ekilo_update_oled_context();
void ekilo_calculate_oled_scrolling();
void ekilo_schedule_oled_update();
void ekilo_process_oled_update();

// Clickwheel input functions
void ekilo_process_encoder_input();
void ekilo_enter_char_selection();
void ekilo_exit_char_selection();
void ekilo_cycle_character(int direction);
void ekilo_confirm_character();
char ekilo_get_character_from_index(int index);

// Memory monitoring and safety functions
bool check_memory_available(size_t needed);
void ekilo_show_memory_status();
void ekilo_emergency_cleanup();

#endif // EKILO_EDITOR_H 