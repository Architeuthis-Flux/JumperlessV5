# Jumperless UI Style Guide

This document defines the visual style standards for all user-facing interface elements in the Jumperless project.

## Box Drawing Characters

### Standard Box Style
Always use **single-line rounded corners** for all boxes and frames:

```
╭───────────────────────────────────────────────────────────────────────────╮
│                                                                           │
╰───────────────────────────────────────────────────────────────────────────╯
```

**Never use double-line boxes:**
```
╔═══════════════════════════════════════════════════════════════════════════╗  ❌ DON'T
║                            WRONG STYLE                                    ║  ❌ DON'T  
╚═══════════════════════════════════════════════════════════════════════════╝  ❌ DON'T
```

### Box Character Set
- `╭` - Top-left rounded corner
- `╮` - Top-right rounded corner
- `╰` - Bottom-left rounded corner
- `╯` - Bottom-right rounded corner
- `│` - Vertical line
- `─` - Horizontal line
- `╌` - Light horizontal separator (for dividers within content)

### Box Alignment
- Always ensure content is properly centered with consistent padding
- Count characters carefully to maintain alignment
- Use consistent width (75 characters is standard for main headers)

## Unicode Characters and Emojis

### No Standard Emojis
Replace all standard emojis with culturally appropriate unicode alternatives:
Don't add emojis or unicode to existing code unless absolutely necessary.

Don't add unicode characters for normal prompts, only use them extremely sparingly or in some visual UI situation 

#### File Type Indicators
- `⌘` - Directory (place of interest sign) with blue ANSI color
- `𓆚` - Python files (Egyptian hieroglyph snake) with green ANSI color  
- `⍺` - Text files (alpha) with white/gray ANSI color
- `⚙` - Config files (gear) with yellow ANSI color
- `☊` - Node files (ascending node) with cyan ANSI color
- `⎃` - Color files (emphasis symbol) with rainbow/multicolor cycling

#### Status Indicators
- `☺` - Success (black diamond) with green ANSI color
- `☹` - Error (white diamond) with red ANSI color  
- `△` - Warning (white up-pointing triangle) with yellow ANSI color
- `○` - Info (white circle) with blue ANSI color

#### Action Indicators
- `↑` - Upload/Send (upwards arrow) with blue ANSI color
- `↓` - Download/Receive (downwards arrow) with green ANSI color
- `⇌` - Connection (rightwards harpoon over leftwards harpoon) with cyan ANSI color
- `⇎` - Disconnection (left right double arrow with stroke) with red ANSI color
- `◐` `◓` `◑` `◒` - Process/Work (rotating circles) with rotating colors and shape (only animate when convenient)
- `●` - Process complete (black circle) with green ANSI color
- `→` `←` `↑` `↓` - Navigation arrows with white ANSI color

#### Editor and Files
- `⎚` - Editing (clear screen symbol) with yellow ANSI color
- `⎙` - Saving (print screen symbol) with green ANSI color
- `▢` - Commands (white square with rounded corners) with cyan ANSI color
- `☞` - Tips (white right pointing index) with blue ANSI color

#### Physical Representations of Things on the Jumperless
- `⚟` - 3 pads next to logo (ADC, DAC, GPIO)
- `✎` - Probe (lower right pencil)
- `✍` - Probe alternative (writing hand)
- `⭤` - Connect (left right triangle-headed arrow)
- `⬾` - Disconnect (leftwards arrow through x)
- 
- 

### Color Usage
- Always use ANSI terminal colors to provide meaning and visual distinction
- Prefer colors that work well on both dark and light terminal backgrounds
- Use color consistently across all interfaces
- Colors should reinforce the semantic meaning of symbols

## Complete Symbol Reference

### Box Drawing (Core)
- `╭` - Top-left rounded corner
- `╮` - Top-right rounded corner
- `╰` - Bottom-left rounded corner
- `╯` - Bottom-right rounded corner
- `│` - Vertical line
- `─` - Horizontal line
- `╌` - Light horizontal separator

### File Types
- `⌘` - Directory (place of interest sign)
- `𓆚` - Python files (Egyptian hieroglyph snake)
- `⍺` - Text files (alpha)
- `⚙` - Config files (gear)
- `☊` - Node files (ascending node)
- `⎃` - Color files (emphasis symbol)

### Status & Indicators
- `☺` - Success (smiling face)
- `☹` - Error (frowning face)
- `△` - Warning (white up-pointing triangle)
- `○` - Info (white circle)
- `✓` - Check mark
- `✗` - Cross mark
- `⚠` - Warning sign

### Actions & Navigation
- `↑` - Upload/Send (upwards arrow)
- `↓` - Download/Receive (downwards arrow)
- `→` - Navigate right
- `←` - Navigate left
- `▲` - Up indicator (black up-pointing triangle)
- `▼` - Down indicator (black down-pointing triangle)
- `◀` - Left indicator (black left-pointing triangle)
- `▶` - Right indicator (black right-pointing triangle)
- `⇌` - Connection (rightwards harpoon over leftwards harpoon)
- `⇎` - Disconnection (left right double arrow with stroke)
- `⇄` - Bidirectional (rightwards arrow over leftwards arrow)
- `⇆` - Bidirectional alt (leftwards arrow over rightwards arrow)
- `↺` - Refresh/reload (anticlockwise open circle arrow)
- `↻` - Refresh/reload alt (clockwise open circle arrow)
- `⟲` - Anticlockwise gapped circle arrow
- `⟳` - Clockwise gapped circle arrow

### Process & Work Indicators
- `◐` - Process/Work (circle with left half black)
- `◓` - Process/Work (circle with upper half black)
- `◑` - Process/Work (circle with right half black)
- `◒` - Process/Work (circle with lower half black)
- `●` - Process complete (black circle)

### Editor & Interface
- `⎚` - Editing (clear screen symbol)
- `⎙` - Saving (print screen symbol)
- `▢` - Commands (white square with rounded corners)
- `☞` - Tips (white right pointing index)
- `⎈` - Control/helm symbol
- `⎋` - Escape (broken circle with northwest arrow)
- `⏎` - Return/enter symbol
- `⌫` - Backspace (erase to the left)
- `⌦` - Delete (erase to the right)
- `⇥` - Tab (rightwards arrow to bar)
- `⇤` - Shift tab (leftwards arrow to bar)

### Hardware & Electronics
- `⚟` - ADC/DAC/GPIO pads
- `✎` - Probe (lower right pencil)
- `✍` - Probe alternative (writing hand)
- `⭤` - Connect (left right triangle-headed arrow)
- `⬾` - Disconnect (leftwards arrow through x)
- `⏚` - Electrical ground
- `⏧` - Electrical intersection
- `⚡` - High voltage/power
- `⌇` - Wavy line (signal)
- `⛋` - White diamond in square
- `◉` - Bullseye (target/connection point)
- `⏻` - Power symbol
- `⏼` - Power on-off symbol
- `⏽` - Power on symbol

### Logic & Mathematics
- `⊕` - XOR (circled plus)
- `⊗` - AND (circled times)
- `⊙` - OR (circled dot operator)
- `¬` - NOT (not sign)
- `∧` - Logical AND
- `∨` - Logical OR

### Shapes & Blocks
- `■` - Solid block (black square)
- `□` - Empty block (white square)
- `▪` - Small solid block (black small square)
- `▫` - Small empty block (white small square)
- `⟐` - White diamond with centred dot

### Miscellaneous
- `🔥` - Fire/hot (use sparingly)
- `❄` - Cold/frozen
- `⌬` - Benzene ring (chemistry)
- `⍟` - APL functional symbol circle star
- `⎔` - Software function symbol
- `⚛` - Atom
- `☠` - skull and crossbones
- `☣` - biohazard

## General Design Principles

### Consistency
- Use the same symbols for the same concepts throughout the entire codebase
- Maintain consistent spacing and alignment
- Apply the same color schemes across different modules

### Accessibility  
- Choose unicode characters that display well in terminal environments
- Avoid characters that may not render properly on different systems
- Ensure sufficient contrast between foreground and background colors

### Cultural Sensitivity
- Prefer ancient/historical unicode symbols over modern emoji
- Use geometric shapes and mathematical symbols when appropriate
- Choose symbols that are culturally neutral and universally understandable

## Implementation Notes

### Terminal Compatibility
- All symbols must render properly in standard terminal emulators
- Test on both Windows, macOS, and Linux terminals
- Consider fallback options for terminals with limited unicode support

### Performance
- Avoid excessive use of ANSI escape sequences
- Cache color calculations where possible
- Use efficient string building for complex displays

### Code Organization
- Define symbol constants in a central header file
- Create utility functions for common display patterns
- Keep style decisions consistent across all modules

## Examples

### Good File Manager Display
```
╭───────────────────────────────────────────────────────────────────────────╮
│                            JUMPERLESS FILE MANAGER                        │
╰───────────────────────────────────────────────────────────────────────────╯

⌘ Current Path: /examples/python
Files: 3  |  Use ↑↓ arrows or encoder to navigate
Enter: Open/Edit  |  Space: File operations  |  h: Help  |  q: Quit
╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌

► ⌘ examples                                                           <DIR>
  𓆚 demo.py                                                           2.1KB
  ◈ readme.txt                                                         890B
```

### Good Status Messages
```
☺ File saved successfully
☹ Error: Cannot save file  
△ Native jumperless module not available
○ Starting eKilo editor...
```

This style guide should be followed for all user-facing interface elements to maintain a cohesive and professional appearance throughout the Jumperless project.

## Testing Symbol Compatibility

To test which Unicode symbols render correctly on your system, run the included test script:

```bash
python examples/quick_symbol_test.py
```

This script will test all symbols defined in this guide and report which ones render properly on your terminal/system.

## Extended Symbol Library

The following symbols are approved for use in future Jumperless UI development. When adding new functionality, choose symbols from this curated collection to maintain consistency and ensure compatibility.

### Box Drawing (Extended)
- `┌` - Light Down and Right
- `┐` - Light Down and Left
- `└` - Light Up and Right
- `┘` - Light Up and Left
- `├` - Light Vertical and Right
- `┤` - Light Vertical and Left
- `┬` - Light Down and Horizontal
- `┴` - Light Up and Horizontal
- `┼` - Light Vertical and Horizontal
- `┍` - Light Down and Heavy Right
- `┎` - Heavy Down and Light Right
- `┑` - Light Down and Heavy Left
- `┒` - Heavy Down and Light Left
- `┕` - Light Up and Heavy Right
- `┖` - Heavy Up and Light Right
- `┙` - Light Up and Heavy Left
- `┚` - Heavy Up and Light Left
- `┝` - Light Vertical and Heavy Right
- `┞` - Heavy Up and Light Right and Light Down
- `┟` - Light Up and Heavy Right and Light Down
- `┠` - Heavy Vertical and Light Right
- `┡` - Light Down and Heavy Right and Light Up
- `┢` - Heavy Down and Light Right and Light Up
- `┣` - Heavy Vertical and Light Right
- `┥` - Light Vertical and Heavy Left
- `┦` - Heavy Up and Light Left and Light Down
- `┧` - Light Up and Heavy Left and Light Down
- `┨` - Heavy Vertical and Light Left
- `┩` - Light Down and Heavy Left and Light Up
- `┪` - Heavy Down and Light Left and Light Up
- `┫` - Heavy Vertical and Light Left
- `┭` - Light Down and Heavy Horizontal
- `┮` - Heavy Down and Light Horizontal
- `┯` - Light Down and Heavy Left and Light Right
- `┰` - Heavy Down and Light Left and Light Right
- `┱` - Light Down and Heavy Right and Light Left
- `┲` - Heavy Down and Light Right and Light Left
- `┳` - Heavy Down and Light Horizontal
- `┵` - Light Up and Heavy Horizontal
- `┶` - Heavy Up and Light Horizontal
- `┷` - Light Up and Heavy Left and Light Right
- `┸` - Heavy Up and Light Left and Light Right
- `┹` - Light Up and Heavy Right and Light Left
- `┺` - Heavy Up and Light Right and Light Left
- `┻` - Heavy Up and Light Horizontal
- `┽` - Light Vertical and Heavy Horizontal
- `┾` - Heavy Vertical and Light Horizontal
- `┿` - Light Up and Heavy Down and Light Horizontal
- `╀` - Heavy Up and Light Down and Light Horizontal
- `╁` - Light Up and Light Down and Heavy Horizontal
- `╂` - Heavy Vertical and Light Horizontal
- `╃` - Light Left and Heavy Right and Light Up and Light Down
- `╄` - Heavy Left and Light Right and Light Up and Light Down
- `╅` - Light Left and Light Right and Heavy Up and Light Down
- `╆` - Light Left and Light Right and Light Up and Heavy Down
- `╇` - Light Left and Light Right and Heavy Up and Heavy Down
- `╈` - Heavy Left and Heavy Right and Light Up and Light Down
- `╉` - Heavy Left and Heavy Right and Light Up and Heavy Down
- `╊` - Heavy Left and Heavy Right and Heavy Up and Light Down
- `╋` - Heavy Vertical and Heavy Horizontal
- `╍` - Heavy Triple Dash Horizontal
- `╎` - Heavy Triple Dash Vertical
- `╏` - Heavy Triple Dash Vertical
- `═` - Double Horizontal
- `║` - Double Vertical
- `╒` - Double Down and Right
- `╓` - Double Down and Heavy Right
- `╔` - Double Down and Right
- `╕` - Double Down and Left
- `╖` - Heavy Down and Double Left
- `╗` - Double Down and Left
- `╘` - Double Up and Right
- `╙` - Heavy Up and Double Right
- `╚` - Double Up and Right
- `╛` - Double Up and Left
- `╜` - Heavy Up and Double Left
- `╝` - Double Up and Left
- `╞` - Double Vertical and Right
- `╟` - Heavy Vertical and Double Right
- `╠` - Double Vertical and Right
- `╡` - Double Vertical and Left
- `╢` - Heavy Vertical and Double Left
- `╣` - Double Vertical and Left
- `╤` - Double Down and Horizontal
- `╥` - Heavy Down and Double Horizontal
- `╦` - Double Down and Horizontal
- `╧` - Double Up and Horizontal
- `╨` - Heavy Up and Double Horizontal
- `╩` - Double Up and Horizontal
- `╪` - Double Vertical and Horizontal
- `╫` - Heavy Vertical and Double Horizontal
- `╬` - Double Vertical and Horizontal
- `╱` - Light Diagonal Upper Right to Lower Left
- `╲` - Light Diagonal Upper Left to Lower Right
- `╳` - Light Diagonal Cross
- `╴` - Light Left
- `╵` - Light Up
- `╶` - Light Right
- `╷` - Light Down
- `╸` - Heavy Left
- `╹` - Heavy Up
- `╺` - Heavy Right
- `╻` - Heavy Down
- `╼` - Light Left and Heavy Right
- `╽` - Light Up and Heavy Down
- `╾` - Heavy Left and Light Right
- `╿` - Heavy Up and Light Down
- `▀` - Upper Half Block
- `▁` - Lower One Eighth Block
- `▂` - Lower One Quarter Block
- `▃` - Lower Three Eighths Block
- `▄` - Lower Half Block
- `▅` - Lower Five Eighths Block
- `▆` - Lower Three Quarters Block
- `▇` - Lower Seven Eighths Block
- `█` - Full Block
- `▉` - Left Seven Eighths Block
- `▊` - Left Three Quarters Block
- `▋` - Left Five Eighths Block
- `▌` - Left Half Block
- `▍` - Left Three Eighths Block
- `▎` - Left One Quarter Block
- `▏` - Left One Eighth Block
- `▐` - Right Half Block
- `░` - Light Shade
- `▒` - Medium Shade
- `▓` - Dark Shade
- `▔` - Upper One Eighth Block
- `▕` - Right One Eighth Block
- `▖` - Quadrant Lower Left
- `▗` - Quadrant Lower Right
- `▘` - Quadrant Upper Left
- `▙` - Quadrant Upper Left and Lower Left and Lower Right
- `▚` - Quadrant Upper Left and Lower Right
- `▛` - Quadrant Upper Left and Upper Right and Lower Left
- `▜` - Quadrant Upper Left and Upper Right and Lower Right
- `▝` - Quadrant Upper Right
- `▞` - Quadrant Upper Right and Lower Left
- `▟` - Quadrant Upper Right and Lower Left and Lower Right

### Arrows & Directional
- `↚` - Leftwards Arrow with Stroke
- `↛` - Rightwards Arrow with Stroke
- `↜` - Leftwards Wave Arrow
- `↝` - Rightwards Wave Arrow
- `↞` - Leftwards Two Headed Arrow
- `↟` - Upwards Two Headed Arrow
- `↠` - Rightwards Two Headed Arrow
- `↡` - Downwards Two Headed Arrow
- `↢` - Leftwards Arrow with Tail
- `↣` - Rightwards Arrow with Tail
- `↤` - Leftwards Arrow from Bar
- `↥` - Upwards Arrow from Bar
- `↦` - Rightwards Arrow from Bar
- `↧` - Downwards Arrow from Bar
- `↫` - Leftwards Arrow with Loop
- `↬` - Rightwards Arrow with Loop
- `↭` - Left Right Wave Arrow
- `↮` - Left Right Arrow with Stroke
- `↯` - Downwards Zigzag Arrow
- `↰` - Upwards Arrow with Tip Leftwards
- `↱` - Upwards Arrow with Tip Rightwards
- `↲` - Downwards Arrow with Tip Leftwards
- `↳` - Downwards Arrow with Tip Rightwards
- `↴` - Rightwards Arrow with Corner Downwards
- `↵` - Downwards Arrow with Corner Leftwards
- `↶` - Anticlockwise Top Semicircle Arrow
- `↷` - Clockwise Top Semicircle Arrow
- `↸` - North West Arrow To Long Bar
- `↹` - Leftwards Arrow To Bar Over Rightwards Arrow To Bar
- `⇀` - Rightwards Harpoon with Barb Upwards
- `⇁` - Rightwards Harpoon with Barb Downwards
- `⇂` - Downwards Harpoon with Barb Rightwards
- `⇃` - Downwards Harpoon with Barb Leftwards
- `⇇` - Leftwards Paired Arrows
- `⇈` - Upwards Paired Arrows
- `⇉` - Rightwards Paired Arrows
- `⇊` - Downwards Paired Arrows
- `⇋` - Leftwards Harpoon Over Rightwards Harpoon
- `⇍` - Leftwards Double Arrow with Stroke
- `⇏` - Rightwards Double Arrow with Stroke
- `⇐` - Leftwards Double Arrow
- `⇑` - Upwards Double Arrow
- `⇒` - Rightwards Double Arrow
- `⇓` - Downwards Double Arrow
- `⇔` - Left Right Double Arrow
- `⇕` - Up Down Double Arrow
- `⇖` - North West Double Arrow
- `⇗` - North East Double Arrow
- `⇘` - South East Double Arrow
- `⇙` - South West Double Arrow
- `⇚` - Leftwards Triple Arrow
- `⇛` - Rightwards Triple Arrow
- `⇜` - Leftwards Squiggle Arrow
- `⇝` - Rightwards Squiggle Arrow
- `⇞` - Upwards Arrow with Double Stroke
- `⇟` - Downwards Arrow with Double Stroke
- `⇠` - Leftwards Dashed Arrow
- `⇡` - Upwards Dashed Arrow
- `⇢` - Rightwards Dashed Arrow
- `⇣` - Downwards Dashed Arrow
- `⇦` - Leftwards White Arrow
- `⇧` - Upwards White Arrow
- `⇨` - Rightwards White Arrow
- `⇩` - Downwards White Arrow
- `⇪` - Upwards White Arrow from Bar
- `⇫` - Upwards White Arrow On Pedestal
- `⇬` - Upwards White Arrow On Pedestal with Horizontal Bar
- `⇭` - Upwards White Arrow On Pedestal with Vertical Bar
- `⇮` - Upwards White Double Arrow
- `⇯` - Upwards White Double Arrow On Pedestal
- `⇰` - Rightwards White Arrow from Wall
- `⇱` - North West Arrow To Corner
- `⇲` - South East Arrow To Corner
- `⇳` - Up Down White Arrow
- `⇴` - Right Arrow with Small Circle
- `⇵` - Downwards Arrow Leftwards of Upwards Arrow
- `⇶` - Three Rightwards Arrows
- `⇷` - Leftwards Arrow with Vertical Stroke
- `⇸` - Rightwards Arrow with Vertical Stroke
- `⇹` - Left Right Arrow with Vertical Stroke
- `⇺` - Leftwards Arrow with Double Vertical Stroke
- `⇻` - Rightwards Arrow with Double Vertical Stroke
- `⇼` - Left Right Arrow with Double Vertical Stroke
- `⇽` - Leftwards Open-Headed Arrow
- `⇾` - Rightwards Open-Headed Arrow
- `⇿` - Left Right Open-Headed Arrow
- `⭠` - Leftwards Triangle-Headed Arrow
- `⭡` - Upwards Triangle-Headed Arrow
- `⭢` - Rightwards Triangle-Headed Arrow
- `⭣` - Downwards Triangle-Headed Arrow
- `⭥` - Up Down Triangle-Headed Arrow
- `⮂` - Rightwards Triangle-Headed Arrow Over Leftwards Triangle-Headed Arrow
- `⮃` - Downwards Triangle-Headed Arrow Leftwards of Upwards Triangle-Headed Arrow

### Pointing & Hand Symbols
- `☚` - Black Left Pointing Index
- `☛` - Black Right Pointing Index
- `☜` - White Left Pointing Index
- `☟` - White Down Pointing Index

### Stars & Celestial
- `★` - Black Star
- `☆` - White Star
- `☇` - Lightning
- `☈` - Thunderstorm
- `☉` - Sun
- `☌` - Conjunction
- `☍` - Opposition
- `✦` - Black Four Pointed Star
- `✧` - White Four Pointed Star
- `✩` - Stress Outlined White Star
- `✪` - Circled White Star
- `✫` - Open Centre Black Star
- `✬` - Black Centre White Star
- `✭` - Outlined Black Star
- `✮` - Heavy Outlined Black Star
- `✯` - Pinwheel Star
- `✰` - Shadowed White Star
- `✱` - Heavy Asterisk
- `✲` - Open Centre Asterisk
- `✵` - Eight Pointed Pinwheel Star
- `✶` - Six Pointed Black Star
- `✷` - Eight Pointed Rectilinear Black Star
- `✸` - Heavy Eight Pointed Rectilinear Black Star
- `✹` - Twelve Pointed Black Star
- `✺` - Sixteen Pointed Asterisk
- `✻` - Teardrop-Spoked Asterisk
- `✼` - Open Centre Teardrop-Spoked Asterisk
- `✽` - Heavy Teardrop-Spoked Asterisk
- `❂` - Circled Open Centre Eight Pointed Star
- `❃` - Heavy Teardrop-Spoked Pinwheel Asterisk

### Crosses & Religious
- `✙` - Outlined Greek Cross
- `✚` - Heavy Greek Cross
- `✛` - Open Centre Cross
- `✜` - Heavy Open Centre Cross
- `✞` - Shadowed White Latin Cross
- `✟` - Outlined Latin Cross
- `✠` - Maltese Cross

### Dice & Gaming
- `⚀` - Die Face-1
- `⚁` - Die Face-2
- `⚂` - Die Face-3
- `⚃` - Die Face-4
- `⚄` - Die Face-5
- `⚅` - Die Face-6
- `⚆` - White Circle with Dot Right
- `⚇` - White Circle with Two Dots
- `⚈` - Black Circle with White Dot Right
- `⚉` - Black Circle with Two White Dots

### Flags & Symbols
- `⚐` - White Flag
- `⚑` - Black Flag
- `⚝` - Outlined White Star
- `⚭` - Marriage Symbol
- `⚮` - Divorce Symbol
- `⚯` - Unmarried Partnership Symbol

### Pentagrams & Mystical
- `⛤` - Pentagram
- `⛥` - Right-Handed Interlaced Pentagram
- `⛦` - Left-Handed Interlaced Pentagram
- `⛧` - Inverted Pentagram

### Traffic & Warning
- `⛕` - Alternate One-Way Left Way Traffic
- `⛖` - Black Two-Way Left Way Traffic
- `⛗` - White Two-Way Left Way Traffic
- `⛘` - Black Left Lane Merge
- `⛙` - White Left Lane Merge
- `⛚` - Drive Slow Sign
- `⛛` - Heavy White Down-Pointing Triangle
- `⛜` - Left Closed Entry
- `⛝` - Squared Saltire
- `⛞` - Falling Diagonal In White Circle In Black Square
- `⛟` - Black Truck
- `⛠` - Restricted Left Entry-1
- `⛡` - Restricted Left Entry-2
- `⛌` - Crossing Lanes
- `⛍` - Disabled Car
- `⛐` - Car Sliding

### Weather & Elements
- `❄` - Snowflake
- `❅` - Tight Trifoliate Snowflake
- `❆` - Heavy Chevron Snowflake
- `❈` - Heavy Sparkle
- `❉` - Balloon-Spoked Asterisk
- `❊` - Eight Teardrop-Spoked Propeller Asterisk
- `❋` - Heavy Eight Teardrop-Spoked Propeller Asterisk

### Shapes Extended
- `▣` - White Square Containing Black Small Square
- `▤` - Square with Horizontal Fill
- `▥` - Square with Vertical Fill
- `▦` - Square with Orthogonal Crosshatch Fill
- `▧` - Square with Upper Left To Lower Right Fill
- `▨` - Square with Upper Right To Lower Left Fill
- `▩` - Square with Diagonal Crosshatch Fill
- `▬` - Black Rectangle
- `▭` - White Rectangle
- `▮` - Black Vertical Rectangle
- `▯` - White Vertical Rectangle
- `▰` - Black Parallelogram
- `▱` - White Parallelogram
- `▴` - Black Up-Pointing Small Triangle
- `▵` - White Up-Pointing Small Triangle
- `▸` - Black Right-Pointing Small Triangle
- `▹` - White Right-Pointing Small Triangle
- `▻` - White Right-Pointing Pointer
- `▾` - Black Down-Pointing Small Triangle
- `▿` - White Down-Pointing Small Triangle
- `◁` - White Left-Pointing Triangle
- `◂` - Black Left-Pointing Small Triangle
- `◃` - White Left-Pointing Small Triangle
- `◄` - Black Left-Pointing Pointer
- `◅` - White Left-Pointing Pointer
- `◈` - White Diamond Containing Black Small Diamond
- `◊` - Lozenge
- `◌` - Dotted Circle
- `◍` - Circle with Vertical Fill
- `◎` - Bullseye
- `◔` - Circle with Upper Right Quadrant Black
- `◕` - Circle with All But Upper Left Quadrant Black
- `◖` - Left Half Black Circle
- `◗` - Right Half Black Circle
- `◘` - Inverse Bullet
- `◙` - Inverse White Circle
- `◚` - Upper Half Inverse White Circle
- `◛` - Lower Half Inverse White Circle
- `◜` - Upper Left Quadrant Circular Arc
- `◝` - Upper Right Quadrant Circular Arc
- `◞` - Lower Right Quadrant Circular Arc
- `◟` - Lower Left Quadrant Circular Arc
- `◠` - Upper Half Circle
- `◡` - Lower Half Circle
- `◢` - Black Lower Right Triangle
- `◣` - Black Lower Left Triangle
- `◤` - Black Upper Left Triangle
- `◥` - Black Upper Right Triangle
- `◦` - White Bullet
- `◧` - Square with Left Half Black
- `◨` - Square with Right Half Black
- `◩` - Square with Upper Left Diagonal Half Black
- `◪` - Square with Lower Right Diagonal Half Black
- `◫` - White Square with Vertical Bisecting Line
- `◬` - White Up-Pointing Triangle with Dot
- `◭` - Up-Pointing Triangle with Left Half Black
- `◮` - Up-Pointing Triangle with Right Half Black
- `◯` - Large Circle
- `◰` - White Square with Upper Left Quadrant
- `◱` - White Square with Lower Left Quadrant
- `◲` - White Square with Lower Right Quadrant
- `◳` - White Square with Upper Right Quadrant
- `◴` - White Circle with Upper Left Quadrant
- `◵` - White Circle with Lower Left Quadrant
- `◶` - White Circle with Lower Right Quadrant
- `◷` - White Circle with Upper Right Quadrant
- `◸` - Upper Left Triangle
- `◹` - Upper Right Triangle
- `◺` - Lower Left Triangle
- `◿` - Lower Right Triangle
- `⭘` - Heavy Circle


### Miscellaneous Symbols
- `❍` - Shadowed White Circle
- `❏` - Lower Right Drop-Shadowed White Square
- `❐` - Upper Right Drop-Shadowed White Square
- `❑` - Lower Right Shadowed White Square
- `❒` - Upper Right Shadowed White Square
- `❖` - Black Diamond Minus White X
- `❘` - Light Vertical Bar
- `❙` - Medium Vertical Bar
- `❚` - Heavy Vertical Bar

### Fleurons & Decorative
- `✾` - Six Petalled Black and White Florette
- `✿` - Black Florette
- `❀` - White Florette
- `❁` - Eight Petalled Outlined Black Florette

### Pencils & Writing
- `✎` - Lower Right Pencil
- `✐` - Upper Right Pencil
- `✑` - White Nib

### Keyboard & Interface Extended
- `⮐` - Return Left
- `⮑` - Return Right

### Ancient Scripts & Symbols
- `֎` - Decorative Symbol
- `؍` - Arabic Date Separator
- `۞` - Arabic Start of Rub El Hizb
- `۝` - Arabic End of Ayah
- `۩` - Arabic Place of Sajdah
- `ߝ` - NKo Letter FA
- `߷` - NKo Symbol
- `⛻` - Three Lines Converging Right
- `৪` - Bengali Currency Numerator Four
- `൜` - Malayalam Letter Vocalic RR
- `෴` - Sinhala Punctuation Kunddaliya
- `༼` - Tibetan Mark Gug Rtags Gyon
- `༽` - Tibetan Mark Gug Rtags Gyas
- `༄` - Tibetan Mark Initial Yig Mgo Mdun Ma
- `༅` - Tibetan Mark Closing Yig Mgo Sgab Ma
- `་` - Tibetan Mark Intersyllabic Tsheg
- `⁜` - Double Exclamation Mark
- `₪` - New Sheqel Sign
- `꧁` - Javanese Left Rerenggan
- `꧂` - Javanese Right Rerenggan
- `꙱` - Cyrillic Small Letter Iotified A
- `꙲` - Cyrillic Small Letter Iotified A (duplicate)
- `༒` - Tibetan Mark Rgya Gram Shad
- `⁉` - Exclamation Question Mark
- `⁈` - Question Exclamation Mark
- `⁇` - Double Question Mark
- `⁅` - Left Square Bracket with Quill
- `⁆` - Right Square Bracket with Quill
- `℡` - Telephone Sign
- `⅏` - Turned Capital F
- `༟` - Tibetan Mark Tsheg Shad (ox)
- `༞` - Tibetan Mark Nyis Tsheg (xx)
- `༛` - Tibetan Mark Rgyang Shad (oo)
- `༚` - Tibetan Mark Kuru Yig Mgo (o)
- `༜` - Tibetan Mark Kuru Yig Mgo Bzhi Mig Can (ooo)
- `༝` - Tibetan Mark Gter Tsheg (x)
- `⁂` - Asterism (xxx)
- `ᚙ` - Ogham Letter Ceirt (crossbar switch representation)

### Connection & Technical Symbols  
- `●` - Black Circle (general connection)
- `⛥` - Right-Handed Interlaced Pentagram
- `⎆` - Enter Symbol  
- `▢` - White Square with Rounded Corners
- `⌇` - Wavy Line
- `⛓` - Chain
- `᯼` - Batak Symbol Bindu Pinarboras
- `᯾` - Batak Symbol Bindu Judul
- `᯽` - Batak Symbol Bindu Pangolat
- `₮` - Tugrik Sign
- `₪` - New Sheqel Sign





This extended library provides hundreds of additional Unicode symbols that have been tested for compatibility and visual appeal. When implementing new features in Jumperless, select symbols from this collection to maintain consistency with the established design language. 