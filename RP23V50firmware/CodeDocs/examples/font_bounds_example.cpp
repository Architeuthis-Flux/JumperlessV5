#include "oled.h"

void fontBoundsExample() {
    // Initialize OLED
    if (!oled.init()) {
        return; // OLED not connected
    }
    
    // Example 1: Basic character and text bounds
    const char* testText = "Hello World!";
    
    // Get bounds for current font
    TextBounds bounds = oled.getTextBounds(testText);
    
    // Display text properties
    oled.clear();
    oled.setTextSize(1);
    FontMetrics metrics = oled.getFontMetrics();
    
    // Use proper baseline positioning without excessive spacing
    int lineY = metrics.ascent + 2; // Start with proper ascent + small margin
    oled.setCursor(0, lineY);
    oled.print("Width: ");
    oled.print(bounds.width);
    oled.print(" Height: ");
    oled.print(bounds.height);
    oled.show();
    delay(2000);
    
    // Example 2: Check if text fits and center it
    const char* longText = "This is a very long text that might not fit";
    
    oled.clear(); // Always clear first
    if (oled.textFits(longText)) {
        // Text fits - center it
        int16_t x, y;
        oled.getCenteredPosition(longText, &x, &y);
        
        // Use safe positioning
        oled.setCursor(x, metrics.ascent + 2); // Use ascent for proper baseline
        oled.print(longText);
        oled.show();
    } else {
        // Text doesn't fit - wrap it
        std::vector<String> lines = oled.wrapText(longText);
        
        // Use natural line spacing from font
        int lineHeight = metrics.lineHeight; // Use font's natural spacing
        int startY = metrics.ascent + 2; // Start with proper ascent
        
        for (int i = 0; i < lines.size() && i < 2; i++) { // Limit to 2 lines for 32px display
            int16_t x;
            oled.getCenteredX(lines[i].c_str(), &x);
            int currentY = startY + (i * lineHeight);
            if (currentY < SCREEN_HEIGHT - metrics.descent) { // Check bounds
                oled.setCursor(x, currentY);
                oled.print(lines[i]);
            }
        }
        oled.show();
    }
    delay(3000);
    
    // Example 3: Compare different fonts
    const char* compareText = "Font Test";
    
    for (int fontIndex = 0; fontIndex < numFonts; fontIndex++) {
        oled.clear(); // Clear before each font
        oled.setFont(fontIndex);
        
        // Get font metrics for this font
        FontMetrics currentMetrics = oled.getFontMetrics();
        TextBounds textBounds = oled.getTextBounds(compareText);
        
        // Only proceed if font will fit on screen
        if (currentMetrics.lineHeight > SCREEN_HEIGHT - 4) {
            oled.setTextSize(1);
            oled.setCursor(0, currentMetrics.ascent + 2);
            oled.print("Font too large!");
            oled.show();
            delay(2000);
            continue;
        }
        
        oled.setTextSize(1);
        
        // Display font info at top
        int lineY = currentMetrics.ascent + 2;
        oled.setCursor(0, lineY);
        oled.print(fontList[fontIndex].shortName);
        
        // Show metrics on next line using natural spacing
        lineY += currentMetrics.lineHeight;
        if (lineY < SCREEN_HEIGHT - currentMetrics.descent - 2) {
            oled.setCursor(0, lineY);
            oled.print("H:");
            oled.print(currentMetrics.lineHeight);
            oled.print(" W:");
            oled.print(textBounds.width);
        }
        
        // Display the test text centered at bottom if it fits
        int16_t x, centerY;
        oled.getCenteredPosition(compareText, &x, &centerY);
        int safeY = SCREEN_HEIGHT - currentMetrics.descent - 2;
        if (safeY > lineY + currentMetrics.lineHeight) {
            oled.setCursor(x, safeY);
            oled.print(compareText);
        }
        
        oled.show();
        delay(2000);
    }
    
    // Example 4: Character by character analysis
    const char* charTest = "Ag";
    oled.setFont(0); // Reset to first font
    
    oled.clear(); // Always clear first
    oled.setTextSize(1);
    FontMetrics fontMetrics = oled.getFontMetrics();
    
    int cursorX = 5; // Start with some margin
    for (int i = 0; charTest[i] != '\0'; i++) {
        oled.clear(); // Clear for each character display
        CharBounds charBounds = oled.getCharBounds(charTest[i]);
        
        // Show character info at top using natural spacing
        int lineY = fontMetrics.ascent + 2;
        oled.setCursor(0, lineY);
        oled.print("Char: ");
        oled.print(charTest[i]);
        
        lineY += fontMetrics.lineHeight; // Use natural line height
        if (lineY < SCREEN_HEIGHT - fontMetrics.descent - 2) {
            oled.setCursor(0, lineY);
            oled.print("W:");
            oled.print(charBounds.width);
            oled.print(" H:");
            oled.print(charBounds.height);
        }
        
        // Draw character at bottom with proper baseline
        int charY = SCREEN_HEIGHT - fontMetrics.descent - 2;
        if (charY > lineY + fontMetrics.lineHeight) {
            oled.setCursor(cursorX + charBounds.xOffset, charY);
            oled.print(charTest[i]);
        }
        
        oled.show();
        delay(1500);
        
        cursorX += charBounds.xAdvance;
    }
    
    // Example 5: Optimal text positioning
    const char* titleText = "Title";
    const char* subtitleText = "Subtitle";
    
    oled.clear(); // Always clear first
    oled.setFont(0); // Use smaller font for this example
    FontMetrics titleMetrics = oled.getFontMetrics();
    
    // Calculate if both lines will fit using natural spacing
    int totalHeight = (titleMetrics.lineHeight * 2) + 2; // Two lines plus small margin
    if (totalHeight < SCREEN_HEIGHT) {
        // Position title at top, centered
        int16_t titleX;
        oled.getCenteredX(titleText, &titleX);
        
        int titleY = titleMetrics.ascent + 2;
        oled.setCursor(titleX, titleY);
        oled.print(titleText);
        
        // Position subtitle below title, centered
        int16_t subtitleX;
        oled.getCenteredX(subtitleText, &subtitleX);
        
        int subtitleY = titleY + titleMetrics.lineHeight; // Natural spacing
        if (subtitleY < SCREEN_HEIGHT - titleMetrics.descent - 2) {
            oled.setCursor(subtitleX, subtitleY);
            oled.print(subtitleText);
        }
    } else {
        // Not enough space, just show title
        int16_t x, y;
        oled.getCenteredPosition(titleText, &x, &y);
        oled.setCursor(x, titleMetrics.ascent + 2);
        oled.print(titleText);
    }
    
    oled.show();
    delay(3000);
    
    // Example 6: Text fitting validation
    const char* testMessages[] = {
        "Short",
        "Medium text",
        "Very long message",
        "Multi Line"
    };
    
    oled.setFont(0); // Use smallest font for this test
    FontMetrics currentMetrics = oled.getFontMetrics();
    
    for (int i = 0; i < 4; i++) {
        oled.clear(); // Clear for each test
        
        // Calculate safe test positions using natural spacing
        int16_t line1Y = currentMetrics.ascent + 2;
        int16_t line2Y = line1Y + currentMetrics.lineHeight;
        int16_t line3Y = line2Y + currentMetrics.lineHeight;
        
        // Test different positions with bounds checking
        int16_t testPositions[][2] = {
            {0, line1Y}, 
            {64, line2Y}, 
            {100, line3Y}
        };
        
        // Show test results
        for (int pos = 0; pos < 3; pos++) {
            if (testPositions[pos][1] < SCREEN_HEIGHT - currentMetrics.descent) {
                bool fits = oled.textFitsAt(testMessages[i], testPositions[pos][0], testPositions[pos][1]);
                
                oled.setCursor(0, testPositions[pos][1]);
                oled.print("Pos ");
                oled.print(pos);
                oled.print(": ");
                oled.print(fits ? "OK" : "NO");
            }
        }
        
        // Show the actual text if it fits anywhere (at bottom)
        if (oled.textFits(testMessages[i])) {
            int16_t x, y;
            oled.getCenteredPosition(testMessages[i], &x, &y);
            int safeY = SCREEN_HEIGHT - currentMetrics.descent - 2;
            if (safeY > line3Y + currentMetrics.lineHeight) {
                oled.setCursor(x, safeY);
                oled.print(testMessages[i]);
            }
        }
        
        oled.show();
        delay(2000);
    }
}

// Simple helper function to demonstrate text measurement
void displayTextInfo(const char* text) {
    TextBounds bounds = oled.getTextBounds(text);
    FontMetrics metrics = oled.getFontMetrics();
    
    oled.clear(); // Always clear first
    oled.setTextSize(1);
    
    // Calculate line positions with natural spacing
    int line1Y = metrics.ascent + 2;
    int line2Y = line1Y + metrics.lineHeight;
    int line3Y = line2Y + metrics.lineHeight;
    int line4Y = line3Y + metrics.lineHeight;
    
    // Display measurements using safe positioning
    oled.setCursor(0, line1Y);
    oled.print("Text: ");
    oled.print(text);
    
    if (line2Y < SCREEN_HEIGHT - metrics.descent - 2) {
        oled.setCursor(0, line2Y);
        oled.print("Size: ");
        oled.print(bounds.width);
        oled.print("x");
        oled.print(bounds.height);
    }
    
    if (line3Y < SCREEN_HEIGHT - metrics.descent - 2) {
        oled.setCursor(0, line3Y);
        oled.print("Font H: ");
        oled.print(metrics.lineHeight);
    }
    
    // Only show if there's enough room
    if (line4Y < SCREEN_HEIGHT - metrics.descent - 2) {
        oled.setCursor(0, line4Y);
        oled.print("Max chars: ");
        oled.print(oled.getCharsPerLine());
    }
    
    oled.show();
}

// Function to test all fonts with sample text
void testAllFonts() {
    const char* sampleText = "Sample 123";
    
    for (int i = 0; i < numFonts; i++) {
        oled.clear(); // Clear before each font
        oled.setFont(i);
        FontMetrics metrics = oled.getFontMetrics();
        
        // Check if font is too large for display
        if (metrics.lineHeight > SCREEN_HEIGHT - 6) {
            oled.setTextSize(1);
            oled.setCursor(0, 8);
            oled.print("Font too large!");
            oled.show();
            delay(2000);
            continue;
        }
        
        oled.setTextSize(1);
        
        // Show font name at top
        int line1Y = metrics.ascent + 2;
        oled.setCursor(0, line1Y);
        oled.print("Font: ");
        oled.print(fontList[i].longName);
        
        // Center the sample text in middle if there's space
        int16_t x, y;
        oled.getCenteredPosition(sampleText, &x, &y);
        TextBounds sampleBounds = oled.getTextBounds(sampleText);
        
        int centerY = (SCREEN_HEIGHT / 2);
        if (centerY > line1Y + metrics.lineHeight + 2 && 
            centerY < SCREEN_HEIGHT - metrics.descent - 2) {
            oled.setCursor(x, centerY);
            oled.print(sampleText);
        }
        
        // Show metrics at bottom if there's space
        int bottomY = SCREEN_HEIGHT - metrics.descent - 2;
        if (bottomY > centerY + metrics.lineHeight + 2) {
            oled.setCursor(0, bottomY);
            oled.print("H:");
            oled.print(metrics.lineHeight);
            oled.print(" CPL:");
            oled.print(oled.getCharsPerLine());
        }
        
        oled.show();
        delay(2000);
    }
}

// Quick test to verify font bounds functionality
bool quickFontTest() {
    if (!oled.init()) {
        return false; // OLED not connected
    }
    
    // Test basic functionality
    const char* testText = "Test";
    TextBounds bounds = oled.getTextBounds(testText);
    FontMetrics metrics = oled.getFontMetrics();
    
    // Basic sanity checks
    bool passed = true;
    
    if (bounds.width <= 0 || bounds.height <= 0) {
        passed = false;
    }
    
    if (metrics.lineHeight <= 0 || metrics.avgWidth <= 0) {
        passed = false;
    }
    
    // Display test result
    oled.clear(); // Clear first
    oled.setTextSize(1);
    
    int line1Y = metrics.ascent + 2;
    oled.setCursor(0, line1Y);
    oled.print("Font Test: ");
    oled.print(passed ? "PASS" : "FAIL");
    
    if (passed) {
        int line2Y = line1Y + metrics.lineHeight; // Natural spacing
        if (line2Y < SCREEN_HEIGHT - metrics.descent - 2) {
            oled.setCursor(0, line2Y);
            oled.print("W:");
            oled.print(bounds.width);
            oled.print(" H:");
            oled.print(bounds.height);
        }
    }
    
    oled.show();
    return passed;
}

// Display a menu of font bounds example options
void fontBoundsMenu() {
    oled.clear(); // Clear first
    oled.setTextSize(1);
    FontMetrics metrics = oled.getFontMetrics();
    
    // Menu title
    int16_t x;
    oled.getCenteredX("Font Examples", &x);
    int line1Y = metrics.ascent + 2;
    oled.setCursor(x, line1Y);
    oled.print("Font Examples");
    
    // Menu options
    const char* options[] = {
        "1: Full Demo",
        "2: Font Test", 
        "3: Text Info",
        "4: Quick Test"
    };
    
    int startLineY = line1Y + metrics.lineHeight + 1; // Small gap after title
    for (int i = 0; i < 4; i++) {
        int currentY = startLineY + (i * metrics.lineHeight); // Natural spacing
        if (currentY < SCREEN_HEIGHT - metrics.descent - 2) {
            oled.setCursor(0, currentY);
            oled.print(options[i]);
        }
    }
    
    oled.show();
    
    // Note: In actual use, you'd add encoder/button handling here
    // For now, just display the menu
}

// Demonstrate automatic text wrapping
void demonstrateTextWrapping(const char* longText) {
    if (!oled.init()) {
        return;
    }
    
    oled.clear(); // Clear first
    oled.setTextSize(1);
    FontMetrics metrics = oled.getFontMetrics();
    
    // Show original text length first
    TextBounds originalBounds = oled.getTextBounds(longText);
    int line1Y = metrics.ascent + 2;
    oled.setCursor(0, line1Y);
    oled.print("Original: ");
    oled.print(originalBounds.width);
    oled.print("px");
    oled.show();
    delay(2000);
    
    // Now show wrapped version
    std::vector<String> lines = oled.wrapText(longText);
    
    oled.clear(); // Clear for wrapped display
    oled.setCursor(0, line1Y);
    oled.print("Wrapped to ");
    oled.print((int)lines.size());
    oled.print(" lines:");
    
    int startY = line1Y + metrics.lineHeight + 1; // Small gap after header
    int lineHeight = metrics.lineHeight; // Natural spacing
    
    // Show max 2-3 lines for 32px display
    int maxLines = (SCREEN_HEIGHT - startY - metrics.descent - 2) / lineHeight;
    maxLines = min(maxLines, (int)lines.size());
    maxLines = min(maxLines, 2); // Limit for small display
    
    for (int i = 0; i < maxLines; i++) {
        int currentY = startY + (i * lineHeight);
        if (currentY < SCREEN_HEIGHT - metrics.descent - 2) {
            oled.setCursor(0, currentY);
            oled.print(lines[i]);
        }
    }
    
    if (lines.size() > maxLines) {
        int dotsY = startY + (maxLines * lineHeight);
        if (dotsY < SCREEN_HEIGHT - metrics.descent - 2) {
            oled.setCursor(0, dotsY);
            oled.print("...");
        }
    }
    
    oled.show();
}

// Show text positioning examples
void demonstrateTextPositioning(const char* text) {
    if (!oled.init()) {
        return;
    }
    
    const char* alignments[] = {"LEFT", "CENTER", "RIGHT"};
    
    for (int align = 0; align < 3; align++) {
        oled.clear(); // Clear for each alignment test
        oled.setTextSize(1);
        FontMetrics metrics = oled.getFontMetrics();
        
        // Show alignment type at top
        int line1Y = metrics.ascent + 2;
        oled.setCursor(0, line1Y);
        oled.print("Align: ");
        oled.print(alignments[align]);
        
        // Position the test text in middle area with natural spacing
        TextBounds bounds = oled.getTextBounds(text);
        int16_t x;
        int testY = line1Y + metrics.lineHeight + 2; // Small gap after header
        
        // Make sure text will fit
        if (testY < SCREEN_HEIGHT - metrics.descent - 6) {
            switch (align) {
                case 0: // LEFT
                    x = 0;
                    break;
                case 1: // CENTER
                    oled.getCenteredX(text, &x);
                    break;
                case 2: // RIGHT
                    x = SCREEN_WIDTH - bounds.width;
                    if (x < 0) x = 0; // Prevent overflow
                    break;
            }
            
            oled.setCursor(x, testY);
            oled.print(text);
            
            // Show position info at bottom if there's space
            int bottomY = SCREEN_HEIGHT - metrics.descent - 2;
            if (bottomY > testY + metrics.lineHeight + 1) {
                oled.setCursor(0, bottomY);
                oled.print("X:");
                oled.print(x);
                oled.print(" W:");
                oled.print(bounds.width);
            }
        }
        
        oled.show();
        delay(2000);
    }
    
    // Vertical positioning demo
    const char* verticals[] = {"TOP", "MIDDLE", "BOTTOM"};
    
    for (int vert = 0; vert < 3; vert++) {
        oled.clear(); // Clear for each vertical test
        oled.setTextSize(1);
        FontMetrics metrics = oled.getFontMetrics();
        
        TextBounds bounds = oled.getTextBounds(text);
        int16_t x;
        int y;
        
        // Center horizontally, vary vertically
        oled.getCenteredX(text, &x);
        
        switch (vert) {
            case 0: // TOP
                y = metrics.ascent + 4; // Near top with proper ascent
                break;
            case 1: // MIDDLE
                y = (SCREEN_HEIGHT / 2);
                break;
            case 2: // BOTTOM
                y = SCREEN_HEIGHT - metrics.descent - 4; // Near bottom with descent
                break;
        }
        
        // Make sure position is valid
        if (y > metrics.ascent && y < SCREEN_HEIGHT - metrics.descent) {
            oled.setCursor(x, y);
            oled.print(text);
        }
        
        // Show vertical alignment type at a safe position
        int labelY = (vert == 0) ? (SCREEN_HEIGHT - metrics.descent - 2) : (metrics.ascent + 2);
        if (labelY != y && labelY > metrics.ascent && labelY < SCREEN_HEIGHT - metrics.descent) {
            oled.setCursor(0, labelY);
            oled.print("Vert: ");
            oled.print(verticals[vert]);
        }
        
        oled.show();
        delay(2000);
    }
} 