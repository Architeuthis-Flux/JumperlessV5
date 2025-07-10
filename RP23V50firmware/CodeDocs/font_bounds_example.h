#ifndef FONT_BOUNDS_EXAMPLE_H
#define FONT_BOUNDS_EXAMPLE_H

#include "oled.h"

/**
 * @brief Comprehensive demonstration of all font bounds functions
 * 
 * This function runs through multiple examples showing:
 * - Basic character and text bounds measurement
 * - Text fitting validation and automatic centering
 * - Font comparison across all available fonts
 * - Character-by-character analysis
 * - Optimal text positioning with title/subtitle layout
 * - Text fitting validation at different positions
 * 
 * Requires OLED to be connected and initialized.
 */
void fontBoundsExample();

/**
 * @brief Display detailed information about a text string
 * 
 * Shows text measurements including:
 * - Text dimensions (width x height)
 * - Font line height
 * - Maximum characters per line for current font
 * 
 * @param text The text string to analyze and display info for
 */
void displayTextInfo(const char* text);

/**
 * @brief Test all available fonts with sample text
 * 
 * Cycles through all fonts in the fontList array and displays:
 * - Font name (long name)
 * - Sample text centered on display
 * - Font metrics (height and characters per line)
 * 
 * Useful for comparing fonts and their characteristics.
 */
void testAllFonts();

/**
 * @brief Quick test to verify font bounds functionality
 * 
 * Simple validation function that tests basic font bounds operations
 * and displays results. Good for troubleshooting.
 * 
 * @return true if all tests pass, false if any fail
 */
bool quickFontTest();

/**
 * @brief Display a menu of font bounds example options
 * 
 * Shows an interactive menu allowing selection of different
 * font bounds demonstrations. Use with encoder/button input.
 */
void fontBoundsMenu();

/**
 * @brief Demonstrate automatic text wrapping
 * 
 * Shows how the wrapText function handles long text strings
 * by breaking them into multiple lines that fit the display.
 * 
 * @param longText The text to wrap and display
 */
void demonstrateTextWrapping(const char* longText);

/**
 * @brief Show text positioning examples
 * 
 * Demonstrates different text alignment options:
 * - Left aligned
 * - Center aligned  
 * - Right aligned
 * - Top, middle, bottom vertical positioning
 * 
 * @param text The text to position
 */
void demonstrateTextPositioning(const char* text);

#endif // FONT_BOUNDS_EXAMPLE_H 