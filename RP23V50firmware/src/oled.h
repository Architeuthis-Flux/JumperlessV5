#ifndef OLED_H
#define OLED_H

#include "Adafruit_SSD1306.h"
#include "JumperlessDefines.h"
#include "Arduino.h"
#include "Wire.h"
#include <vector>
#include "fonts/Eurostile8pt7b.h"
#include "fonts/Eurostile12pt7b.h"

#include "fonts/Jokerman8pt7b.h"
#include "fonts/Jokerman12pt7b.h"

#include "fonts/Comic_Sans8pt7b.h"
#include "fonts/Comic_Sans12pt7b.h"

#include "fonts/Courier_New8pt7b.h"
#include "fonts/Courier_New12pt7b.h"

#include "fonts/new_science_medium8pt7b.h"
#include "fonts/new_science_medium12pt7b.h"

#include "fonts/new_science_medium_extended8pt7b.h"
#include "fonts/new_science_medium_extended12pt7b.h"


//#include "fonts/JumperlessLowerc12pt7b.h"

class Adafruit_SSD1306;

extern bool oledConnected;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1

// Forward declarations for font bounds structures
struct CharBounds;
struct TextBounds;
struct FontMetrics;

// Font family enumeration for automatic size selection
enum FontFamily {
    FONT_EUROSTILE = 0,
    FONT_JOKERMAN = 1,
    FONT_COMIC_SANS = 2,
    FONT_COURIER_NEW = 3,
    FONT_NEW_SCIENCE_MEDIUM = 4,
    FONT_NEW_SCIENCE_MEDIUM_EXTENDED = 5
};

// Positioning mode enum for simplified positioning system
enum PositionMode {
    POS_AUTO,      // Automatically choose best positioning
    POS_TIGHT,     // Force tight positioning (topmost pixels at exact y)
    POS_BASELINE   // Force baseline positioning (traditional font baseline)
};

// Comprehensive text information structure
struct TextInfo {
    uint8_t textSize;       // Current text size multiplier
    const GFXfont* font;    // Current font pointer
    int16_t height;         // Current text height
    int16_t lineHeight;     // Current line spacing
    int16_t ascent;         // Current ascent
    int16_t cursorX;        // Current cursor X position
    int16_t cursorY;        // Current cursor Y position
};

class oled {
public:
    oled();
    int init();
    void test();
    void testMenuPositioning();
    
    // Basic print functions
    void print(const char* s);
    void print(const String& s);
    void print(int i);
    void print(char c);
    void print(float f);
    void println(const char* s);
    void println(const String& s);
    void println(int i);
    void println(float f);
    
    // Display functions
    void displayBitmap(int x, int y, const unsigned char* bitmap, int width, int height);
    void showJogo32h();
    void clear();
    void show();
    void moveToNextLine();
    
    // Unified cursor positioning
    void setCursor(int16_t x, int16_t y, PositionMode mode);
    void setCursor(int16_t x, int16_t y);  // Auto mode
    
    // Display settings
    void setTextColor(uint32_t color);
    void setTextSize(uint8_t size);
    void invertDisplay(bool inv);
    
    // Connection management
    bool isConnected() const;
    int connect(void);
    void disconnect(void);
    bool checkConnection(void);
    
    // Font management
    int cycleFont(void);
    FontFamily getFontFamily(String fontName);
    int setFont(String fontName, int justGetIndex = 0);
    int setFont(char* fontName, int justGetIndex = 0);
    void setFont(const GFXfont* font);
    void setFont(FontFamily fontFamily);
    void setFont(int fontIndex);
    void setFontForSize(FontFamily family, int textSize);
    String getFontName(FontFamily fontFamily);
    
    // Simplified display functions
    void clearPrintShow(const char* text, int textSize, bool clear = true, bool show = true, bool center = true, int x_pos = -1, int y_pos = -1);
    void clearPrintShow(const char* text, int textSize, FontFamily family, bool clear = true, bool show = true, bool center = true, int x_pos = -1, int y_pos = -1);
    
    void clearPrintShow(const char* text, int textSize = 2);
    void clearPrintShow(const String& text, int textSize);
    void clearPrintShow(const String& text, int textSize, bool clear, bool show, bool center, int x_pos = -1, int y_pos = -1);
    void clearPrintShow(const String& text, int textSize, FontFamily family, bool clear = true, bool show = true, bool center = true, int x_pos = -1, int y_pos = -1);

    
    // Helper for multi-line text
    void displayMultiLineText(const char* text, bool center);
    
    // Debug functions
    void dumpFrameBufferQuarterSize(int clearFirst = 0, int x_pos = 40, int y_pos = 24, int border = 1, Stream* stream = &Serial);
    void dumpFrameBuffer();
    
    // Positioning functions (simplified)
    void getCenteredPosition(const char* str, int16_t* x, int16_t* y, PositionMode mode);
    void getCenteredPosition(const char* str, int16_t* x, int16_t* y);  // Auto mode
    
    // Font bounds and measurement functions
    FontMetrics getFontMetrics();
    TextBounds getTextBounds(const char* str);
    TextBounds getTextBounds(const String& str);
    bool textFits(const char* str);
    
    // Store config
    int address = -1;
    int sda_pin = -1;
    int scl_pin = -1;
    int sda_row = -1;
    int scl_row = -1;
    int textSize = 2;
    int charPos = 0;
    uint8_t currentTextSize = 1; // Track current text size
    
    unsigned long lastConnectionCheck = 0;
    unsigned long connectionCheckInterval = 500;
    
    const GFXfont* currentFont;
    int currentFontIndex = 0;

    int fontIndex = 0;

    FontFamily currentFontFamily = FONT_EUROSTILE;
    
    bool oledConnected = false;
    
    int connectionRetries = 0;
    int maxConnectionRetries = 4;


    char scratchPad[40];
    char* getScratchPad(void);
};

// Global functions
void oledExample(void);
int initOLED(void);
int oledTest(int sdaRow = NANO_D2, int sclRow = NANO_D3, int sdaPin = 26, int sclPin = 27, int leaveConnections = 1);

// Bitmap data
extern const unsigned char jogo255[];
extern const unsigned char jogo32h[];
extern const unsigned char logo[];
extern const unsigned char ColorJumpLogo[];

// Font structure
struct font {
    const GFXfont* font;
    const char* shortName;
    const char* longName;
    int16_t topRowOffset;  // Y offset to apply when text is on top row with multiple lines
    FontFamily family;
};

// Character bounds structure for returning detailed character metrics
struct CharBounds {
    int16_t width;      // Character width in pixels
    int16_t height;     // Character height in pixels
    int16_t xAdvance;   // Horizontal advance (cursor movement)
    int16_t xOffset;    // X offset from cursor position
    int16_t yOffset;    // Y offset from cursor position
    int16_t x1, y1;     // Top-left bounding box corner
    int16_t x2, y2;     // Bottom-right bounding box corner
};

// String bounds structure for returning text measurements
struct TextBounds {
    int16_t width;      // Total string width in pixels
    int16_t height;     // Total string height in pixels
    int16_t x1, y1;     // Top-left bounding box corner
    int16_t x2, y2;     // Bottom-right bounding box corner
    int16_t baseline;   // Baseline position
    int16_t ascent;     // Distance from baseline to top
    int16_t descent;    // Distance from baseline to bottom
};

// Font metrics structure for returning font characteristics
struct FontMetrics {
    int16_t lineHeight;     // Line spacing (yAdvance)
    int16_t ascent;         // Maximum ascent in font
    int16_t descent;        // Maximum descent in font  
    int16_t maxWidth;       // Maximum character width
};

// Font list
extern struct font fontList[];
extern int numFonts;

// Global instance
extern class oled oled;

#endif


