#!/usr/bin/env python3
"""
Calculate high-saturation spectrum-ordered terminal color cube
Only includes colors with saturation above a threshold for better spectrum representation
"""

import colorsys
import math

def rgb_to_hsv(r, g, b):
    """Convert RGB (0-255) to HSV (h: 0-360, s: 0-1, v: 0-1)"""
    h, s, v = colorsys.rgb_to_hsv(r/255.0, g/255.0, b/255.0)
    return h * 360.0, s, v

def generate_high_saturation_colors(min_saturation=0.5, min_value=0.2):
    """Generate high-saturation colors from the 6x6x6 cube"""
    # RGB levels for the 6x6x6 cube
    rgb_levels = [0, 95, 135, 175, 215, 255]
    
    colors = []
    
    for r_idx in range(6):
        for g_idx in range(6):
            for b_idx in range(6):
                # Calculate terminal color number
                term_color = 16 + 36 * r_idx + 6 * g_idx + b_idx
                
                # Get actual RGB values
                r = rgb_levels[r_idx]
                g = rgb_levels[g_idx] 
                b = rgb_levels[b_idx]
                
                # Calculate HSV
                hue, saturation, value = rgb_to_hsv(r, g, b)
                
                # Filter for high saturation and reasonable brightness
                # Skip pure black (value too low) and gray colors (saturation too low)
                if saturation >= min_saturation and value >= min_value:
                    colors.append({
                        'term_color': term_color,
                        'r_idx': r_idx,
                        'g_idx': g_idx, 
                        'b_idx': b_idx,
                        'r': r,
                        'g': g,
                        'b': b,
                        'hue': hue,
                        'saturation': saturation,
                        'value': value
                    })
    
    return colors

def sort_by_hue(colors):
    """Sort colors by hue to create spectrum order"""
    return sorted(colors, key=lambda x: x['hue'])

def format_c_array(sorted_colors, array_name="highSaturationSpectrumColors"):
    """Format the sorted colors as a C array"""
    lines = []
    lines.append(f"// High-saturation colors from 6x6x6 cube arranged in spectrum order")
    lines.append(f"// Filtered for saturation >= 0.5 and value >= 0.2")
    lines.append(f"// Total colors: {len(sorted_colors)} out of 216 possible")
    lines.append(f"const int {array_name}[{len(sorted_colors)}] = {{")
    
    # Group by approximate hue ranges for comments
    current_hue_range = -1
    hue_ranges = [
        (0, 30, "Red"),
        (30, 60, "Orange"), 
        (60, 90, "Yellow"),
        (90, 120, "Yellow-Green"),
        (120, 150, "Green"),
        (150, 180, "Green-Cyan"),
        (180, 210, "Cyan"),
        (210, 240, "Cyan-Blue"),
        (240, 270, "Blue"),
        (270, 300, "Blue-Magenta"),
        (300, 330, "Magenta"),
        (330, 360, "Red-Magenta")
    ]
    
    colors_per_line = 12
    line_items = []
    
    for i, color in enumerate(sorted_colors):
        hue = color['hue']
        
        # Check if we need a new hue range comment
        for start, end, name in hue_ranges:
            if start <= hue < end and current_hue_range != start:
                if line_items:
                    # Finish the current line
                    lines.append("  " + ", ".join(line_items) + ",")
                    line_items = []
                
                lines.append(f"  // {name} hues ({start}-{end}°)")
                current_hue_range = start
                break
        
        line_items.append(str(color['term_color']))
        
        # Add line break every colors_per_line items or at the end
        if len(line_items) >= colors_per_line or i == len(sorted_colors) - 1:
            if i == len(sorted_colors) - 1:
                lines.append("  " + ", ".join(line_items))  # No comma on last line
            else:
                lines.append("  " + ", ".join(line_items) + ",")
            line_items = []
    
    lines.append("};")
    lines.append("")
    lines.append(f"const int {array_name}Count = {len(sorted_colors)};")
    return "\n".join(lines)

def print_color_analysis(colors):
    """Print analysis of the filtered colors"""
    print(f"\nColor Analysis:")
    print(f"Total high-saturation colors: {len(colors)}")
    
    # Analyze by hue ranges
    hue_ranges = [
        (0, 30, "Red"),
        (30, 60, "Orange"), 
        (60, 90, "Yellow"),
        (90, 120, "Yellow-Green"),
        (120, 150, "Green"),
        (150, 180, "Green-Cyan"),
        (180, 210, "Cyan"),
        (210, 240, "Cyan-Blue"),
        (240, 270, "Blue"),
        (270, 300, "Blue-Magenta"),
        (300, 330, "Magenta"),
        (330, 360, "Red-Magenta")
    ]
    
    for start, end, name in hue_ranges:
        count = len([c for c in colors if start <= c['hue'] < end])
        if count > 0:
            print(f"  {name:12s} ({start:3d}-{end:3d}°): {count:2d} colors")

def main():
    print("Generating high-saturation spectrum-ordered colors...")
    
    # Try different saturation thresholds
    thresholds = [0.3, 0.4, 0.5, 0.6, 0.7]
    
    for threshold in thresholds:
        colors = generate_high_saturation_colors(min_saturation=threshold)
        print(f"\nSaturation threshold {threshold}: {len(colors)} colors")
    
    # Use a moderate threshold for good color representation
    colors = generate_high_saturation_colors(min_saturation=0.85, min_value=0.7)
    sorted_colors = sort_by_hue(colors)
    
    print_color_analysis(sorted_colors)
    
    # Show some example colors
    print("\nFirst 10 high-saturation colors:")
    for i in range(min(10, len(sorted_colors))):
        c = sorted_colors[i]
        print(f"  {c['term_color']:3d}: RGB({c['r']:3d},{c['g']:3d},{c['b']:3d}) H={c['hue']:5.1f}° S={c['saturation']:.2f} V={c['value']:.2f}")
    
    print("\nLast 10 high-saturation colors:")
    for i in range(max(0, len(sorted_colors)-10), len(sorted_colors)):
        c = sorted_colors[i]
        print(f"  {c['term_color']:3d}: RGB({c['r']:3d},{c['g']:3d},{c['b']:3d}) H={c['hue']:5.1f}° S={c['saturation']:.2f} V={c['value']:.2f}")
    
    # Generate C array
    c_array = format_c_array(sorted_colors)
    
    print("\n" + "="*80)
    print("C ARRAY OUTPUT:")
    print("="*80)
    print(c_array)
    
    # Also save to file
    with open("high_saturation_spectrum.c", "w") as f:
        f.write(c_array)
    print(f"\nArray also saved to high_saturation_spectrum.c")

if __name__ == "__main__":
    main() 