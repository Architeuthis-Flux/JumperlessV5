"""
Jumperless Stylophone - Ultra Conservative
Musical instrument using probe and GPIO to generate audio tones.

Hardware Setup:
1. Connect speaker between holes 30 (positive) and 60 (negative)

Ultra-conservative version to prevent crashes on 32KB heap.

Usage:
  exec(open('examples/micropython_examples/stylophone.py').read())
"""

import time
import gc

# Minimal note frequencies (integers only)
NOTES = [262, 294, 330, 349, 392, 440, 494, 523]  # C, D, E, F, G, A, B, C

def setup_audio():
    """Ultra-safe audio setup with error handling"""
    try:
        print("Setting up audio...")
        nodes_clear()
        print("Nodes cleared")
        
        gpio_set_dir(1, True)
        print("GPIO direction set")
        
        gpio_set(1, False)
        print("GPIO initialized")
        
        connect(GPIO_1, 30)
        print("Connected to hole 30")
        
        connect(GND, 60)
        print("Connected to hole 60")
        
        print("Audio ready")
        return True
    except Exception as e:
        print("Setup failed: " + str(e))
        return False

def play_tone_simple(freq, duration=100):
    """Ultra-simple tone generation using regular sleep"""
    if freq <= 0:
        return
    
    try:
        # Use simple timing with regular sleep
        period_ms = 1000.0 / freq
        half_period_ms = period_ms / 2.0
        cycles = int(duration / period_ms)
        
        # Limit cycles to prevent long execution
        if cycles > 100:
            cycles = 100
        
        for i in range(cycles):
            gpio_set(1, True)
            time.sleep(half_period_ms / 1000.0)  # Convert to seconds
            gpio_set(1, False)
            time.sleep(half_period_ms / 1000.0)
            
            # Break early if too many cycles
            if i > 50:
                break
                
    except Exception as e:
        print("Tone error: " + str(e))
        gpio_set(1, False)

def stylophone_safe():
    """Ultra-safe stylophone with extensive error handling"""
    print("Safe Stylophone Starting")
    
    # Setup with error checking
    if not setup_audio():
        print("Setup failed - aborting")
        return
    
    try:
        oled_clear()
        oled_print("Touch 1-8")
        print("Ready for input")
        
        last_pad = 0
        loop_count = 0
        
        while True:
            loop_count += 1
            
            # Garbage collect every 25 loops (more frequent)
            if loop_count % 25 == 0:
                gc.collect()
                print("GC: " + str(gc.mem_free()) + " bytes free")
                loop_count = 0
            
            try:
                pad = probe_read(False)
                
                if pad and pad != -1:
                    pad_num = int(str(pad))
                    
                    if 1 <= pad_num <= 8 and pad_num != last_pad:
                        print("Playing note " + str(pad_num))
                        
                        freq = NOTES[pad_num - 1]
                        play_tone_simple(freq, 150)
                        
                        oled_clear()
                        oled_print("Note " + str(pad_num))
                        
                        last_pad = pad_num
                        
                    elif pad_num > 8:
                        # Higher pads - simpler approach
                        note_idx = (pad_num - 1) % 8
                        freq = NOTES[note_idx]
                        play_tone_simple(freq, 100)
                        
                        oled_clear()
                        oled_print("Hi " + str(pad_num))
                        last_pad = pad_num
                        
                else:
                    if last_pad != 0:
                        last_pad = 0
                        oled_clear()
                        oled_print("Touch 1-8")
                    
                    # Longer delay when idle to reduce CPU load
                    time.sleep(0.05)  # 50ms
                    
            except Exception as e:
                print("Loop error: " + str(e))
                time.sleep(0.1)  # Pause on error
                continue
                
    except KeyboardInterrupt:
        print("Stopped by user")
    except Exception as e:
        print("Main error: " + str(e))
    finally:
        try:
            gpio_set(1, False)
            oled_clear()
            oled_print("Done")
            print("Cleanup complete")
        except:
            print("Cleanup failed")
        gc.collect()

def test_hardware():
    """Test basic hardware functionality"""
    print("Testing hardware...")
    
    try:
        # Test GPIO
        gpio_set_dir(1, True)
        gpio_set(1, True)
        time.sleep(0.1)
        gpio_set(1, False)
        print("GPIO test: OK")
        
        # Test connections
        nodes_clear()
        result = connect(GPIO_1, 30)
        print("Connect test: " + str(result))
        
        # Test OLED
        oled_clear()
        oled_print("Test")
        print("OLED test: OK")
        
        return True
        
    except Exception as e:
        print("Hardware test failed: " + str(e))
        return False

def minimal_stylophone():
    """Absolute minimal version for debugging"""
    print("Minimal Stylophone")
    
    try:
        nodes_clear()
        gpio_set_dir(1, True)
        print("Basic setup done")
        
        oled_clear()
        oled_print("Touch pads")
        
        for i in range(100):  # Limited loop for testing
            pad = probe_read(False)
            if pad and pad != -1:
                print("Pad: " + str(pad))
                # No audio, just detection
            time.sleep(0.1)
            
    except Exception as e:
        print("Minimal error: " + str(e))
    finally:
        print("Minimal done")

def test_memory():
    """Test memory availability"""
    gc.collect()
    free = gc.mem_free()
    print("Free memory: " + str(free) + " bytes")
    return free

if __name__ == "__main__":
    print("Ultra-Safe Stylophone")
    
    # Test memory first
    free_mem = test_memory()
    
    if free_mem < 5000:
        print("Very low memory - running minimal version")
        minimal_stylophone()
    elif free_mem < 15000:
        print("Low memory - testing hardware first")
        if test_hardware():
            minimal_stylophone()
        else:
            print("Hardware test failed")
    else:
        print("Normal memory - running safe version")
        stylophone_safe() 