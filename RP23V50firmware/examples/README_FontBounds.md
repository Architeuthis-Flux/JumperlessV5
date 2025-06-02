# Font Bounds System for Jumperless OLED

This system provides comprehensive font measurement and text positioning capabilities for the 128x32 pixel OLED display on the Jumperless V5.

## Features

✅ **Pixel-perfect text positioning**  
✅ **Automatic text centering and alignment**  
✅ **Text fit validation**  
✅ **Smart word wrapping**  
✅ **Multi-font support**  
✅ **Character-level analysis**  
✅ **Font metrics calculation**  

## Quick Start

1. Include the header in your firmware:
```cpp
#include "examples/font_bounds_example.h"
```

2. Basic usage:
```cpp
// Check if text fits
if (oled.textFits("Hello World")) {
    // Center the text
    int16_t x, y;
    oled.getCenteredPosition("Hello World", &x, &y);
    oled.setCursor(x, y);
    oled.print("Hello World");
    oled.show();
}
```

## Core Functions

### Text Measurement

#### `getTextBounds(const char* str)`
Returns detailed measurements for any text string.

```cpp
TextBounds bounds = oled.getTextBounds("Sample Text");
// bounds.width, bounds.height, bounds.x1, bounds.y1, bounds.x2, bounds.y2
// bounds.baseline, bounds.ascent, bounds.descent
```

#### `getCharBounds(char c)`
Get metrics for individual characters.

```cpp
CharBounds bounds = oled.getCharBounds('A');
// bounds.width, bounds.height, bounds.xAdvance, bounds.xOffset, bounds.yOffset
```

### Text Positioning

#### `getCenteredPosition(const char* str, int16_t* x, int16_t* y)`
Calculate perfect center position for text.

```cpp
int16_t x, y;
oled.getCenteredPosition("Centered Text", &x, &y);
oled.setCursor(x, y);
oled.print("Centered Text");
```

#### `getCenteredX(const char* str, int16_t* x)`
Center horizontally only.

```cpp
int16_t x;
oled.getCenteredX("Horizontal Center", &x);
oled.setCursor(x, 16); // Your Y position
oled.print("Horizontal Center");
```

### Text Fitting

#### `textFits(const char* str)`
Check if text fits anywhere on display.

```cpp
if (oled.textFits("Long text message")) {
    // Display normally
} else {
    // Use word wrapping or smaller font
}
```

#### `textFitsAt(const char* str, int16_t x, int16_t y)`
Check if text fits at specific position.

```cpp
if (oled.textFitsAt("Status", 0, 24)) {
    oled.setCursor(0, 24);
    oled.print("Status");
}
```

### Word Wrapping

#### `wrapText(const char* text)`
Automatically break long text into lines.

```cpp
std::vector<String> lines = oled.wrapText("Very long text that needs to be wrapped");
int lineHeight = oled.getFontHeight(oled.currentFont);

for (int i = 0; i < lines.size(); i++) {
    oled.setCursor(0, i * lineHeight);
    oled.print(lines[i]);
}
```

### Font Analysis

#### `getFontMetrics()`
Get comprehensive font information.

```cpp
FontMetrics metrics = oled.getFontMetrics();
// metrics.lineHeight, metrics.ascent, metrics.descent
// metrics.maxWidth, metrics.avgWidth, metrics.spaceWidth
```

#### `getMaxLines()` and `getCharsPerLine()`
Calculate layout constraints.

```cpp
int maxLines = oled.getMaxLines();        // How many lines fit
int charsPerLine = oled.getCharsPerLine(); // Approximate chars per line
```

## Data Structures

### TextBounds
```cpp
struct TextBounds {
    int16_t width, height;    // Total dimensions
    int16_t x1, y1, x2, y2;   // Bounding box corners
    int16_t baseline;         // Baseline position
    int16_t ascent, descent;  // Font metrics
};
```

### CharBounds
```cpp
struct CharBounds {
    int16_t width, height;    // Character dimensions
    int16_t xAdvance;         // Cursor movement
    int16_t xOffset, yOffset; // Position offsets
    int16_t x1, y1, x2, y2;   // Bounding box
};
```

### FontMetrics
```cpp
struct FontMetrics {
    int16_t lineHeight;   // Line spacing
    int16_t ascent;       // Max height above baseline
    int16_t descent;      // Max depth below baseline
    int16_t maxWidth;     // Widest character
    int16_t avgWidth;     // Average character width
    int16_t spaceWidth;   // Space character width
};
```

## Practical Examples

### Smart Status Display
```cpp
void displayStatus(const char* title, const char* status, int value) {
    oled.clear();
    
    // Centered title at top
    int16_t x, y;
    oled.getCenteredX(title, &x);
    TextBounds titleBounds = oled.getTextBounds(title);
    oled.setCursor(x, titleBounds.ascent + 2);
    oled.print(title);
    
    // Left-aligned status
    oled.setCursor(0, 16);
    oled.print(status);
    
    // Right-aligned value
    String valueStr = String(value);
    TextBounds valueBounds = oled.getTextBounds(valueStr.c_str());
    oled.setCursor(SCREEN_WIDTH - valueBounds.width, 16);
    oled.print(valueStr);
    
    oled.show();
}
```

### Adaptive Font Selection
```cpp
void smartDisplay(const char* message) {
    // Try fonts from largest to smallest
    bool displayed = false;
    
    for (int fontIndex = numFonts - 1; fontIndex >= 0 && !displayed; fontIndex--) {
        oled.setFont(fontIndex);
        
        if (oled.textFits(message)) {
            int16_t x, y;
            oled.getCenteredPosition(message, &x, &y);
            oled.setCursor(x, y);
            oled.print(message);
            displayed = true;
        }
    }
    
    if (!displayed) {
        // Use word wrapping with smallest font
        oled.setFont(0);
        std::vector<String> lines = oled.wrapText(message);
        // ... display wrapped lines
    }
}
```

### Menu with Proper Spacing
```cpp
void displayMenu(const char* items[], int count, int selected) {
    oled.clear();
    
    FontMetrics metrics = oled.getFontMetrics();
    int lineHeight = metrics.lineHeight;
    int maxItems = SCREEN_HEIGHT / lineHeight;
    
    for (int i = 0; i < count && i < maxItems; i++) {
        int y = i * lineHeight;
        
        // Highlight selected item
        if (i == selected) {
            oled.setCursor(0, y);
            oled.print(">");
        }
        
        oled.setCursor(8, y);
        oled.print(items[i]);
    }
    
    oled.show();
}
```

## Integration into Firmware

### Option 1: Direct Integration
Add to your main firmware files:
```cpp
#include "examples/font_bounds_example.h"

void setup() {
    // Your setup code...
    
    // Quick test
    if (quickFontTest()) {
        // Font bounds working
    }
}

void loop() {
    // Use font bounds functions as needed
    if (oled.textFits(currentMessage)) {
        int16_t x, y;
        oled.getCenteredPosition(currentMessage, &x, &y);
        oled.setCursor(x, y);
        oled.print(currentMessage);
    }
}
```

### Option 2: App Integration
Add as an app in your app system:
```cpp
void fontBoundsApp() {
    fontBoundsMenu();
    
    // Handle encoder input for menu selection
    // Run different demonstrations based on selection
}
```

## Example Functions Available

- `fontBoundsExample()` - Complete demonstration
- `testAllFonts()` - Font comparison
- `displayTextInfo(const char* text)` - Text analysis
- `quickFontTest()` - Validation test
- `fontBoundsMenu()` - Interactive menu
- `demonstrateTextWrapping(const char* text)` - Wrapping demo
- `demonstrateTextPositioning(const char* text)` - Alignment demo

## Tips for Best Results

1. **Always check fit before displaying**: Use `textFits()` or `textFitsAt()`
2. **Use appropriate fonts**: Different fonts for different purposes
3. **Consider word wrapping**: For long messages
4. **Center important text**: Use `getCenteredPosition()`
5. **Account for baselines**: Use `TextBounds.ascent` for proper positioning
6. **Test with different content**: Fonts behave differently with different characters

## Display Constraints

- **Screen**: 128x32 pixels
- **Available fonts**: 4 (Eurostile, Berkeley, Jokerman, Jumperless)
- **Font sizes**: Various heights from ~10px to ~47px
- **Max lines**: Depends on font (2-3 lines typically)
- **Max characters**: Varies by font (~10-20 chars per line)

This system ensures your text always displays perfectly on the OLED, regardless of content length or font choice! 