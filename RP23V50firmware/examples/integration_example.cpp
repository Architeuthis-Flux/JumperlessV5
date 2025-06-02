/*
 * Integration Example for Font Bounds Functions
 * 
 * This file shows how to integrate the font bounds example functions
 * into your main Jumperless firmware.
 */

#include "oled.h"
#include "font_bounds_example.h"
#include "RotaryEncoder.h"  // Assuming you have encoder support

// Example integration into your main loop or app system
void runFontBoundsDemo() {
    // Quick validation first
    if (!quickFontTest()) {
        // Font bounds not working properly
        oled.clear();
        oled.setCursor(0, 10);
        oled.print("Font Error!");
        oled.show();
        delay(2000);
        return;
    }
    
    // Run the full demonstration
    fontBoundsExample();
}

// Example of using font bounds in a practical application
void displayStatus(const char* title, const char* status, int value) {
    oled.clear();
    
    // Title at top, centered
    int16_t x, y;
    oled.setFont(2); // Use Jokerman font for title
    oled.getCenteredX(title, &x);
    TextBounds titleBounds = oled.getTextBounds(title);
    oled.setCursor(x, titleBounds.ascent + 2);
    oled.print(title);
    
    // Status text in middle, left aligned
    oled.setFont(0); // Switch to smaller font
    oled.setCursor(0, 16);
    oled.print(status);
    
    // Value on same line, right aligned
    String valueStr = String(value);
    TextBounds valueBounds = oled.getTextBounds(valueStr.c_str());
    oled.setCursor(SCREEN_WIDTH - valueBounds.width, 16);
    oled.print(valueStr);
    
    oled.show();
}

// Example menu using font bounds for proper spacing
void displayMenu(const char* items[], int itemCount, int selectedIndex) {
    oled.clear();
    oled.setFont(0);
    
    FontMetrics metrics = oled.getFontMetrics();
    int lineHeight = metrics.lineHeight;
    int maxVisibleItems = SCREEN_HEIGHT / lineHeight;
    
    // Calculate scroll offset if needed
    int scrollOffset = 0;
    if (selectedIndex >= maxVisibleItems) {
        scrollOffset = selectedIndex - maxVisibleItems + 1;
    }
    
    // Display menu items
    for (int i = 0; i < itemCount && i < maxVisibleItems; i++) {
        int itemIndex = i + scrollOffset;
        if (itemIndex >= itemCount) break;
        
        int y = i * lineHeight;
        
        // Highlight selected item
        if (itemIndex == selectedIndex) {
            oled.setCursor(0, y);
            oled.print(">");
            oled.setCursor(8, y);
        } else {
            oled.setCursor(8, y);
        }
        
        // Check if text fits, truncate if needed
        if (oled.textFitsAt(items[itemIndex], 8, y)) {
            oled.print(items[itemIndex]);
        } else {
            // Truncate text to fit
            String truncated = String(items[itemIndex]);
            while (!oled.textFitsAt(truncated.c_str(), 8, y) && truncated.length() > 1) {
                truncated = truncated.substring(0, truncated.length() - 1);
            }
            oled.print(truncated);
        }
    }
    
    oled.show();
}

// Example of smart text display that adapts to content
void smartTextDisplay(const char* message) {
    oled.clear();
    
    // Try different fonts to find best fit
    bool displayed = false;
    
    for (int fontIndex = 0; fontIndex < numFonts && !displayed; fontIndex++) {
        oled.setFont(fontIndex);
        
        if (oled.textFits(message)) {
            // Text fits with this font - center it
            int16_t x, y;
            oled.getCenteredPosition(message, &x, &y);
            oled.setCursor(x, y);
            oled.print(message);
            displayed = true;
        }
    }
    
    if (!displayed) {
        // Text too long even for smallest font - wrap it
        oled.setFont(0); // Use smallest font
        std::vector<String> lines = oled.wrapText(message);
        
        int lineHeight = oled.getFontHeight(oled.currentFont);
        int startY = (SCREEN_HEIGHT - (lines.size() * lineHeight)) / 2;
        if (startY < 0) startY = 0;
        
        for (int i = 0; i < lines.size() && i < oled.getMaxLines(); i++) {
            int16_t x;
            oled.getCenteredX(lines[i].c_str(), &x);
            oled.setCursor(x, startY + (i * lineHeight));
            oled.print(lines[i]);
        }
    }
    
    oled.show();
}

// Example encoder-driven font browser
void fontBrowser() {
    static int currentFont = 0;
    static int lastEncoderPos = 0;
    
    // Check encoder for font selection
    if (encoderPosition != lastEncoderPos) {
        int delta = encoderPosition - lastEncoderPos;
        currentFont += delta;
        
        // Wrap around
        if (currentFont >= numFonts) currentFont = 0;
        if (currentFont < 0) currentFont = numFonts - 1;
        
        lastEncoderPos = encoderPosition;
        
        // Display current font
        oled.setFont(currentFont);
        oled.clear();
        
        // Font name at top
        oled.setCursor(0, 0);
        oled.setTextSize(1);
        oled.print(fontList[currentFont].longName);
        
        // Sample text centered
        const char* sample = "Sample ABC 123";
        int16_t x, y;
        oled.getCenteredPosition(sample, &x, &y + 8);
        oled.setCursor(x, y);
        oled.print(sample);
        
        // Font info at bottom
        FontMetrics metrics = oled.getFontMetrics();
        oled.setCursor(0, SCREEN_HEIGHT - 8);
        oled.print("H:");
        oled.print(metrics.lineHeight);
        oled.print(" CPL:");
        oled.print(oled.getCharsPerLine());
        
        oled.show();
    }
    
    // Button press to select font
    if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {
        // Font selected - could save to config or use immediately
        displayTextInfo("Font Selected!");
        delay(1000);
    }
}

// Integration with your existing app system
void fontBoundsApp() {
    // This could be called from your main app switcher
    fontBoundsMenu();
    
    // In a real implementation, you'd handle encoder input here
    // For example:
    /*
    while (appRunning) {
        switch (encoderPosition % 4) {
            case 0:
                quickFontTest();
                break;
            case 1:
                testAllFonts();
                break;
            case 2:
                displayTextInfo("Test String");
                break;
            case 3:
                fontBoundsExample();
                break;
        }
        
        if (encoderButtonState == RELEASED && lastButtonEncoderState == PRESSED) {
            appRunning = false; // Exit app
        }
        
        delay(100);
    }
    */
} 