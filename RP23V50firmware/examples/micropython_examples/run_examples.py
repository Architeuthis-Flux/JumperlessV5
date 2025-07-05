"""
Jumperless MicroPython Examples Runner
=====================================

This script provides a simple menu interface to run the various
Jumperless MicroPython examples. It allows you to:
- See a list of available examples
- Run individual examples
- Run all examples in sequence
- Get help on specific examples

Usage:
  exec(open('micropython_examples/run_examples.py').read())
"""

import time

def show_menu():
    """Display the main examples menu"""
    
    oled_clear()
    oled_print("Example Menu")
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                        JUMPERLESS MICROPYTHON EXAMPLES                      │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    print("\nAvailable Examples:")
    print("  1. DAC Basics          - Digital-to-Analog Converter operations")
    print("  2. ADC Basics          - Analog-to-Digital Converter operations") 
    print("  3. GPIO Basics         - General Purpose Input/Output operations")
    print("  4. Node Connections    - Breadboard node connection operations")
    print("")
    print("  a. Run All Examples    - Execute all examples in sequence")
    print("  h. Help               - Show detailed help")
    print("  q. Quit               - Exit example runner")
    print("\nEnter your choice (1-4, a, h, q): ")

def run_dac_example():
    """Run the DAC basics example"""
    
    print("\n☺ Loading DAC Basics Example...")
    oled_clear()
    oled_print("Loading DAC Example")
    
    try:
        exec(open('micropython_examples/01_dac_basics.py').read())
        print("✓ DAC example completed successfully")
    except Exception as e:
        print("❌ Error running DAC example: " + str(e))
    
    time.sleep(2)

def run_adc_example():
    """Run the ADC basics example"""
    
    print("\n☺ Loading ADC Basics Example...")
    oled_clear()
    oled_print("Loading ADC Example")
    
    try:
        exec(open('micropython_examples/02_adc_basics.py').read())
        print("✓ ADC example completed successfully")
    except Exception as e:
        print("❌ Error running ADC example: " + str(e))
    
    time.sleep(2)

def run_gpio_example():
    """Run the GPIO basics example"""
    
    print("\n☺ Loading GPIO Basics Example...")
    oled_clear()
    oled_print("Loading GPIO Example")
    
    try:
        exec(open('micropython_examples/03_gpio_basics.py').read())
        print("✓ GPIO example completed successfully")
    except Exception as e:
        print("❌ Error running GPIO example: " + str(e))
    
    time.sleep(2)

def run_connections_example():
    """Run the node connections example"""
    
    print("\n☺ Loading Node Connections Example...")
    oled_clear()
    oled_print("Loading Connections")
    
    try:
        exec(open('micropython_examples/04_node_connections.py').read())
        print("✓ Node connections example completed successfully")
    except Exception as e:
        print("❌ Error running connections example: " + str(e))
    
    time.sleep(2)

def run_all_examples():
    """Run all examples in sequence"""
    
    print("\n🚀 Running All Examples in Sequence")
    print("═" * 75)
    
    oled_clear()
    oled_print("Running All Examples")
    
    examples = [
        ("DAC Basics", run_dac_example),
        ("ADC Basics", run_adc_example),
        ("GPIO Basics", run_gpio_example),
        ("Node Connections", run_connections_example)
    ]
    
    for i, (name, func) in enumerate(examples):
        print("\n📍 Example " + str(i + 1) + "/4: " + name)
        print("─" * 50)
        
        try:
            func()
            print("✓ " + name + " completed")
        except Exception as e:
            print("❌ " + name + " failed: " + str(e))
        
        if i < len(examples) - 1:
            print("\n⏳ Pausing 3 seconds before next example...")
            time.sleep(3)
    
    oled_clear()
    oled_print("All Examples Complete!")
    print("\n🎉 All examples completed!")
    print("═" * 75)

def show_help():
    """Show detailed help information"""
    
    oled_clear()
    oled_print("Help Information")
    
    print("\n╭─────────────────────────────────────────────────────────────────────────────╮")
    print("│                              EXAMPLE HELP                                   │")
    print("╰─────────────────────────────────────────────────────────────────────────────╯")
    print("\n📚 Example Descriptions:")
    print("")
    print("1. DAC Basics (01_dac_basics.py)")
    print("   • Set voltages on DAC channels (DAC0, DAC1, power rails)")
    print("   • Read back DAC values for verification")
    print("   • Demonstrate voltage sweeping and power rail control")
    print("   • Functions: dac_set(), dac_get()")
    print("")
    print("2. ADC Basics (02_adc_basics.py)")
    print("   • Read analog voltages from ADC inputs")
    print("   • Monitor multiple channels simultaneously")
    print("   • Voltage range detection and statistics")
    print("   • Functions: adc_get()")
    print("")
    print("3. GPIO Basics (03_gpio_basics.py)")
    print("   • Control digital input/output pins")
    print("   • Set pin directions and pull resistors")
    print("   • Blinking patterns and binary counting")
    print("   • Functions: gpio_set(), gpio_get(), gpio_set_dir(), etc.")
    print("")
    print("4. Node Connections (04_node_connections.py)")
    print("   • Connect breadboard holes to special nodes")
    print("   • Use numbers, strings, or constants for nodes")
    print("   • Power rail setup and signal testing")
    print("   • Functions: connect(), disconnect(), is_connected()")
    print("")
    print("💡 Tips:")
    print("   • Watch both OLED display and console output")
    print("   • Each example includes verification steps")
    print("   • Examples can be stopped with Ctrl+C")
    print("   • Files are in micropython_examples/ directory")
    print("")
    print("🔧 Manual Execution:")
    print("   exec(open('micropython_examples/01_dac_basics.py').read())")
    print("")

def example_runner():
    """Main example runner with interactive menu"""
    
    print("🚀 Jumperless MicroPython Examples Runner")
    print("═" * 50)
    
    while True:
        show_menu()
        
        try:
            # Simple input simulation (since input() might not work in all environments)
            print("Waiting for choice... (this demo will auto-run all examples)")
            choice = 'a'  # Auto-run all for demo purposes
            
            # In a real implementation, you would use:
            # choice = input().strip().lower()
            
            if choice == '1':
                run_dac_example()
            elif choice == '2':
                run_adc_example()
            elif choice == '3':
                run_gpio_example()
            elif choice == '4':
                run_connections_example()
            elif choice == 'a':
                run_all_examples()
                break  # Exit after running all examples
            elif choice == 'h':
                show_help()
                time.sleep(5)  # Give time to read help
            elif choice == 'q':
                print("\n👋 Goodbye!")
                oled_clear()
                oled_print("Goodbye!")
                break
            else:
                print("\n❌ Invalid choice. Please try again.")
                time.sleep(1)
                
        except KeyboardInterrupt:
            print("\n\n⏹️ Example runner stopped by user")
            oled_clear()
            oled_print("Stopped")
            break
        except Exception as e:
            print("\n❌ Error: " + str(e))
            time.sleep(2)

def quick_demo():
    """Run a quick demonstration of key features"""
    
    print("\n🎯 Quick Feature Demonstration")
    print("═" * 40)
    
    oled_clear()
    oled_print("Quick Demo")
    
    # Quick DAC demo
    print("\n☺ DAC Demo: Setting TOP_RAIL to 3.3V")
    dac_set(TOP_RAIL, 3.3)
    voltage = dac_get(TOP_RAIL)
    print("  TOP_RAIL: " + str(voltage) + "V")
    
    oled_clear()
    oled_print("DAC: " + str(voltage) + "V")
    time.sleep(2)
    
    # Quick ADC demo
    print("\n☺ ADC Demo: Reading ADC0")
    adc_voltage = adc_get(0)
    print("  ADC0: " + str(adc_voltage) + "V")
    
    oled_clear()
    oled_print("ADC: " + str(adc_voltage) + "V")
    time.sleep(2)
    
    # Quick GPIO demo
    print("\n☺ GPIO Demo: Blinking GPIO1")
    gpio_set_dir(1, True)  # Set as output
    
    for i in range(3):
        gpio_set(1, True)
        state = gpio_get(1)
        print("  GPIO1: " + state)
        
        oled_clear()
        oled_print("GPIO1: " + state)
        time.sleep(0.5)
        
        gpio_set(1, False)
        state = gpio_get(1)
        time.sleep(0.5)
    
    # Quick connection demo
    print("\n☺ Connection Demo: Connecting holes 1-30")
    result = connect(1, 30)
    print("  Connect 1-30: " + result)
    
    connected = is_connected(1, 30)
    print("  Verification: " + connected)
    
    oled_clear()
    oled_print("Connected 1-30")
    time.sleep(2)
    
    # Cleanup
    disconnect(1, 30)
    nodes_clear()
    dac_set(TOP_RAIL, 0.0)
    
    print("\n✓ Quick demo complete!")
    oled_clear()
    oled_print("Demo Complete!")

# Auto-run when imported/executed
if __name__ == "__main__":
    print("Starting Jumperless MicroPython Examples...")
    quick_demo()
    time.sleep(2)
    example_runner()
else:
    print("Jumperless Examples Runner loaded.")
    print("Run quick_demo() for a quick test or example_runner() for the full menu.") 