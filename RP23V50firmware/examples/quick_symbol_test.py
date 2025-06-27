#!/usr/bin/env python3
"""
Quick Unicode Symbol Test for Jumperless UI

This script prints all symbols from the Jumperless UI Style Guide
in a compact format for quick visual verification.
"""

def main():
    # All symbols from the Jumperless UI Style Guide
    symbols = [
        # Box drawing
        '╭', '╮', '╰', '╯', '│', '─', '╌',
        
        # File types  
        '⌘', '𓆚', '⍺', '⚙', '☊', '⎃',
        
        # Status & indicators
        '☺', '☹', '△', '○', '✓', '✗', '⚠',
        
        # Navigation & actions
        '↑', '↓', '→', '←', '▲', '▼', '◀', '▶',
        '⇌', '⇎', '⇄', '⇆', '↺', '↻', '⟲', '⟳',
        
        # Process indicators
        '◐', '◓', '◑', '◒', '●',
        
        # Editor & interface
        '⎚', '⎙', '▢', '☞', '⎈', '⎋', '⏎', '⌫', '⌦', '⇥', '⇤',
        
        # Hardware & electronics
        '⚟', '✎', '✍', '⭤', '⬾', '⏚', '⏧', '⚡', '⌇', '⛋', '◉',
        '⏻', '⏼', '⏽',
        
        # Logic & math
        '⊕', '⊗', '⊙', '¬', '∧', '∨',
        
        # Shapes & blocks
        '■', '□', '▪', '▫', '⟐',
        
        # Miscellaneous
        '🔥', '❄', '⌬', '⍟', '⎔'
    ]
    
    print("Jumperless UI Symbol Visual Test")
    print("=" * 50)
    print(f"Testing {len(symbols)} symbols:\n")
    
    # Print symbols in rows of 10
    for i in range(0, len(symbols), 10):
        row = symbols[i:i+10]
        print(' '.join(f"{sym:2}" for sym in row))
    
    print(f"\nAll {len(symbols)} symbols displayed above.")
    print("If you see question marks, boxes, or missing characters,")
    print("those symbols may not be supported on your system.")
    
    # Test a few specific ones that are most likely to have issues
    print("\nKey symbols test:")
    print(f"Egyptian snake (Python): {symbols[8]}")  # 𓆚
    print(f"Text files (alpha): {symbols[9]}")  # ⍺
    print(f"Success/Error: {symbols[13]} {symbols[14]}")  # ☺ ☹
    print(f"Connection arrows: {symbols[28]} {symbols[29]}")  # ⇌ ⇎  
    print(f"Electrical ground: {symbols[57]}")  # ⏚
    print(f"Process indicators: {' '.join(symbols[36:41])}")  # ◐◓◑◒●

if __name__ == "__main__":
    main() 