"""
╭─────────────────────────────────────────────────────────────────╮
│                    LED Brightness Control Demo                  │
╰─────────────────────────────────────────────────────────────────╯

Control LED brightness using DAC and probe touch detection.
Touch breadboard pads 1-10 to control LED brightness levels.

🔧 Hardware Setup:
1. Connect LED anode to breadboard hole 15
2. Connect LED cathode to GND through current-limiting resistor (220Ω-1kΩ)
3. Touch breadboard pads 1-10 to control brightness

🎮 Controls:
- Touch pad 1: Minimum brightness (0.33V)
- Touch pad 5: Medium brightness (1.65V)  
- Touch pad 10: Maximum brightness (3.3V)
- Ctrl+C: Exit demo

📋 Usage:
  exec(open('examples/micropython_examples/led_brightness_control.py').read())
  led_brightness_control()
"""

import time

def led_brightness_control():
    """Control LED brightness using DAC and probe touch"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                🔆 LED Brightness Control Demo                   │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    print("\n🎮 Touch breadboard pads 1-10 to control LED brightness")
    
    # Setup
    nodes_clear()
    oled_clear()
    oled_print("LED Brightness")
    
    # Connect DAC0 to breadboard for LED control
    connect(DAC0, 15)
    print("  ✓ DAC0 connected to hole 15")
    print("\n📋 Hardware Setup Required:")
    print("  • Connect LED anode to hole 15")
    print("  • Connect LED cathode to GND through resistor")
    print("  • Press Enter when ready...")
    
    try:
        input()
    except:
        pass
    
    print("\n🎮 Controls active - touch pads 1-10 for brightness")
    print("  Pad 1 = Min brightness, Pad 10 = Max brightness")
    
    try:
        while True:
            # Read probe (non-blocking)
            pad = probe_read(False)
            
            if pad and pad != -1:
                try:
                    # Get pad number
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_num = int(str(pad))
                    
                    # Map pad 1-10 to voltage 0-3.3V
                    if 1 <= pad_num <= 10:
                        voltage = (pad_num / 10.0) * 3.3
                        dac_set(DAC0, voltage)
                        
                        print("🔆 Pad " + str(pad_num) + ": " + str(round(voltage, 1)) + "V")
                        oled_clear()
                        oled_print("Bright: " + str(pad_num) + "/10")
                        
                except (ValueError, AttributeError):
                    pass
            
            time.sleep(0.1)
            
    except KeyboardInterrupt:
        print("\n✓ Demo stopped")
        dac_set(DAC0, 0)  # Turn off LED
        oled_clear()
        oled_print("Demo Done")
        print("  LED turned off")

def quick_led_test():
    """Quick LED brightness test without user input"""
    print("⚡ Quick LED Brightness Test")
    
    # Setup
    nodes_clear()
    connect(DAC0, 15)
    oled_clear()
    oled_print("LED Test")
    
    # Test different brightness levels
    levels = [0.5, 1.0, 2.0, 3.0, 3.3, 0.0]
    for voltage in levels:
        dac_set(DAC0, voltage)
        print("  LED voltage: " + str(voltage) + "V")
        oled_clear()
        oled_print(str(voltage) + "V")
        time.sleep(0.8)
    
    print("✓ Quick test complete")

def led_fade_demo():
    """Demonstrate smooth LED fading"""
    print("🌅 LED Fade Demo")
    
    # Setup
    nodes_clear()
    connect(DAC0, 15)
    oled_clear()
    oled_print("LED Fade")
    
    print("  Running fade sequence...")
    
    # Smooth fade up
    for i in range(0, 33):
        voltage = i * 0.1
        dac_set(DAC0, voltage)
        oled_clear()
        oled_print("Fade: " + str(round(voltage, 1)) + "V")
        time.sleep(0.05)
    
    time.sleep(1)
    
    # Smooth fade down
    for i in range(33, -1, -1):
        voltage = i * 0.1
        dac_set(DAC0, voltage)
        oled_clear()
        oled_print("Fade: " + str(round(voltage, 1)) + "V")
        time.sleep(0.05)
    
    print("✓ Fade demo complete")

def run_all_led_demos():
    """Run all LED brightness demos"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                  🔆 All LED Brightness Demos                    │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    demos = [
        ("Quick Test", quick_led_test),
        ("Fade Demo", led_fade_demo),
        ("Interactive Control", led_brightness_control)
    ]
    
    for name, demo_func in demos:
        print("\n🎯 Running: " + name)
        print("─" * 40)
        try:
            demo_func()
        except KeyboardInterrupt:
            print("⏭️  Skipping to next demo...")
            continue
        
        if name != "Interactive Control":  # Don't pause after interactive demo
            print("📋 Press Enter for next demo...")
            try:
                input()
            except KeyboardInterrupt:
                break
    
    print("\n✅ All LED demos complete!")

if __name__ == "__main__":
    print("LED Brightness Control Demo loaded!")
    print("\nAvailable functions:")
    print("  led_brightness_control() - Interactive brightness control")
    print("  quick_led_test()        - Quick brightness test")
    print("  led_fade_demo()         - Smooth fading demo")
    print("  run_all_led_demos()     - Run all LED demos")
else:
    print("LED brightness control loaded. Try led_brightness_control()!") 