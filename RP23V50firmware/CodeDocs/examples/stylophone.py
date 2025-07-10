"""
Jumperless Stylophone
====================

A musical instrument using the Jumperless probe and GPIO to generate audio tones.
Touch different breadboard pads to play different musical notes!

Hardware Setup:
1. Connect a small speaker or piezo buzzer between breadboard holes 30 and 60
2. The positive lead goes to hole 30 (GPIO output)  
3. The negative lead goes to hole 60 (GND)

Usage:
  exec(open('examples/stylophone.py').read())
  
Then call: stylophone()
"""

import time

# Musical note frequencies in Hz (4th octave)
NOTE_FREQUENCIES = {
    1: 261.63,   # C4
    2: 277.18,   # C#4/Db4
    3: 293.66,   # D4
    4: 311.13,   # D#4/Eb4
    5: 329.63,   # E4
    6: 349.23,   # F4
    7: 369.99,   # F#4/Gb4
    8: 392.00,   # G4
    9: 415.30,   # G#4/Ab4
    10: 440.00,  # A4 (concert pitch)
    11: 466.16,  # A#4/Bb4
    12: 493.88,  # B4
    13: 523.25,  # C5
    14: 554.37,  # C#5/Db5
    15: 587.33,  # D5
    16: 622.25,  # D#5/Eb5
    17: 659.25,  # E5
    18: 698.46,  # F5
    19: 739.99,  # F#5/Gb5
    20: 783.99,  # G5
    21: 830.61,  # G#5/Ab5
    22: 880.00,  # A5
    23: 932.33,  # A#5/Bb5
    24: 987.77,  # B5
    25: 1046.50, # C6
}

# Extended pad mapping for more notes
EXTENDED_NOTE_MAP = {
    # Bottom row (lower octave)
    31: 130.81,  # C3
    32: 138.59,  # C#3
    33: 146.83,  # D3
    34: 155.56,  # D#3
    35: 164.81,  # E3
    36: 174.61,  # F3
    37: 185.00,  # F#3
    38: 196.00,  # G3
    39: 207.65,  # G#3
    40: 220.00,  # A3
    
    # Top row (middle octave) - pads 1-25
    **NOTE_FREQUENCIES,
    
    # Special pads for different octaves
    26: 1108.73, # C#6
    27: 1174.66, # D6
    28: 1244.51, # D#6
    29: 1318.51, # E6
    30: 1396.91, # F6
    
    # Arduino pin pads for special notes/effects
    70: 65.41,   # C2 (D0 pad) - very low
    71: 69.30,   # C#2 (D1 pad)
    72: 73.42,   # D2 (D2 pad)
    73: 77.78,   # D#2 (D3 pad)
    74: 82.41,   # E2 (D4 pad)
    75: 87.31,   # F2 (D5 pad)
    76: 92.50,   # F#2 (D6 pad)
    77: 98.00,   # G2 (D7 pad)
    78: 103.83,  # G#2 (D8 pad)
    79: 110.00,  # A2 (D9 pad)
    80: 116.54,  # A#2 (D10 pad)
    81: 123.47,  # B2 (D11 pad)
    82: 130.81,  # C3 (D12 pad)
    83: 2093.00, # C7 (D13 pad) - very high
}

def setup_audio_output():
    """Set up GPIO for audio output and connect to breadboard"""
    print("🔧 Setting up audio output...")
    
    # Clear any existing connections
    nodes_clear()
    
    # Set GPIO1 as output for audio
    gpio_set_dir(1, True)  # Set as output
    gpio_set(1, False)     # Start low
    
    # Connect GPIO1 to breadboard hole 30 (speaker positive)
    connect(GPIO_1, 30)
    print("  ✓ Connected GPIO1 to hole 30 (speaker +)")
    
    # Connect GND to breadboard hole 60 (speaker negative) 
    connect(GND, 60)
    print("  ✓ Connected GND to hole 60 (speaker -)")
    
    print("\n📢 HARDWARE SETUP REQUIRED:")
    print("  1. Connect a small speaker or piezo buzzer:")
    print("     • Positive lead → Breadboard hole 30")
    print("     • Negative lead → Breadboard hole 60")
    print("  2. Press any key when ready...")
    
    # Wait for user confirmation
    input()
    
    oled_clear()
    oled_print("Stylophone Ready!")
    print("✓ Setup complete!")

def play_tone(frequency, duration_ms=100):
    """Generate a tone by toggling GPIO at the specified frequency"""
    if frequency <= 0:
        return
        
    # Calculate timing for frequency
    period_s = 1.0 / frequency            # Period in seconds
    half_period_s = period_s / 2          # Half period for 50% duty cycle
    
    # Convert duration to number of cycles
    cycles = int((duration_ms / 1000.0) / period_s)
    
    # Generate the tone by toggling GPIO
    for _ in range(cycles):
        gpio_set(1, True)
        time.sleep(half_period_s)
        gpio_set(1, False) 
        time.sleep(half_period_s)

def play_startup_melody():
    """Play a welcome melody"""
    print("🎵 Playing startup melody...")
    oled_clear()
    oled_print("Welcome!")
    
    # Simple ascending scale
    notes = [261.63, 293.66, 329.63, 349.23, 392.00, 440.00, 493.88, 523.25]
    for freq in notes:
        play_tone(freq, 200)
        time.sleep(0.05)
    
    time.sleep(0.5)

def show_note_map():
    """Display the note mapping"""
    print("\n🎹 STYLOPHONE NOTE MAP:")
    print("="*50)
    print("📍 Breadboard Pads (1-30):")
    print("  Pads 1-12:  C4-B4 (middle octave)")
    print("  Pads 13-25: C5-C6 (higher octave)")
    print("  Pads 26-30: Extended high notes")
    print("  Pads 31-40: C3-A3 (lower octave)")
    
    print("\n🔌 Arduino Pin Pads:")
    print("  D0-D12 pads: Very low notes (C2-C3)")
    print("  D13 pad:     Very high note (C7)")
    
    print("\n🎵 Special Notes:")
    print(f"  Pad 10: A4 (Concert pitch - {NOTE_FREQUENCIES[10]:.1f} Hz)")
    print(f"  Pad 1:  C4 (Middle C - {NOTE_FREQUENCIES[1]:.1f} Hz)")
    print(f"  Pad 25: C6 (High C - {EXTENDED_NOTE_MAP[25]:.1f} Hz)")

def stylophone():
    """Main stylophone function"""
    print("╭─────────────────────────────────────────────────────────────────╮")
    print("│                    JUMPERLESS STYLOPHONE                        │")
    print("╰─────────────────────────────────────────────────────────────────╯")
    
    # Setup
    setup_audio_output()
    play_startup_melody()
    show_note_map()
    
    print("\n🎼 STYLOPHONE CONTROLS:")
    print("  • Touch breadboard pads to play notes")
    print("  • Touch and hold for sustained notes")
    print("  • Touch D13 pad for special high note")
    print("  • Press Ctrl+Q to exit")
    print("\n🎹 Now playing! Touch the pads...")
    
    oled_clear()
    oled_print("Touch pads!")
    
    current_note = None
    last_pad = None
    note_start_time = 0
    
    try:
        while True:
            # Read probe non-blocking
            pad = probe_read_nonblocking()
            
            # Check if we have a valid touch
            if pad and pad != -1:
                # Try to get the pad number - could be the object itself or its string representation
                try:
                    if hasattr(pad, 'value'):
                        pad_num = pad.value
                    else:
                        pad_str = str(pad)
                        if pad_str.isdigit():
                            pad_num = int(pad_str)
                        else:
                            pad_num = int(pad)
                except (ValueError, AttributeError):
                    continue  # Skip this iteration if we can't get a valid pad number
                
                # Get frequency for this pad
                frequency = EXTENDED_NOTE_MAP.get(pad_num, 0)
                
                if frequency > 0 and pad_num != last_pad:
                    # New note touched
                    print(f"🎵 Playing pad {pad_num}: {frequency:.1f} Hz")
                    
                    # Display on OLED
                    oled_clear()
                    if pad_num in NOTE_FREQUENCIES:
                        # Show note name for standard notes
                        note_names = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']
                        note_index = (pad_num - 1) % 12
                        octave = 4 + (pad_num - 1) // 12
                        note_name = f"{note_names[note_index]}{octave}"
                        oled_print(f"{note_name} ({frequency:.0f}Hz)")
                    else:
                        oled_print(f"Pad {pad_num}")
                    
                    current_note = frequency
                    last_pad = pad_num
                    note_start_time = time.time()
                
                # Continue playing the current note
                if current_note:
                    play_tone(current_note, 50)
                    
            else:
                # No touch detected
                if current_note:
                    # Note released
                    duration = (time.time() - note_start_time) * 1000  # Convert to ms
                    print(f"  Note released after {duration:.0f}ms")
                    current_note = None
                    last_pad = None
                    
                    oled_clear()
                    oled_print("Touch pads!")
                
                # Small delay when not playing
                time.sleep(0.01)
                
    except KeyboardInterrupt:
        print("\n🎵 Stylophone stopped")
        
    finally:
        # Cleanup
        gpio_set(1, False)  # Make sure audio is off
        oled_clear()
        oled_print("Goodbye!")
        print("✓ Audio output disabled")

def demo_all_notes():
    """Play all available notes in sequence"""
    print("🎼 Playing all notes...")
    setup_audio_output()
    
    oled_clear()
    oled_print("Note Demo")
    
    # Play notes in order
    for pad_num in sorted(EXTENDED_NOTE_MAP.keys()):
        frequency = EXTENDED_NOTE_MAP[pad_num]
        print(f"Pad {pad_num}: {frequency:.1f} Hz")
        
        oled_clear()
        oled_print(f"Pad {pad_num}")
        
        play_tone(frequency, 300)
        time.sleep(0.2)
    
    print("✓ Note demo complete")

def chord_demo():
    """Demonstrate playing chords by rapidly alternating notes"""
    print("🎼 Chord demonstration...")
    setup_audio_output()
    
    # Define some chords (as frequency lists)
    chords = {
        "C Major": [261.63, 329.63, 392.00],      # C-E-G
        "F Major": [349.23, 440.00, 523.25],      # F-A-C
        "G Major": [392.00, 493.88, 587.33],      # G-B-D
        "A Minor": [440.00, 523.25, 659.25],      # A-C-E
    }
    
    for chord_name, frequencies in chords.items():
        print(f"Playing {chord_name}...")
        oled_clear()
        oled_print(chord_name)
        
        # Play chord by rapidly alternating between notes
        for _ in range(20):  # 20 cycles of the chord
            for freq in frequencies:
                play_tone(freq, 30)  # Short duration per note
        
        time.sleep(0.5)
    
    print("✓ Chord demo complete")

def test_audio_setup():
    """Test the audio setup with simple tones"""
    print("🔧 Testing audio setup...")
    
    setup_audio_output()
    
    test_frequencies = [440, 880, 1320]  # A4, A5, E6
    
    for i, freq in enumerate(test_frequencies):
        print(f"Test tone {i+1}: {freq} Hz")
        oled_clear() 
        oled_print(f"Test {freq}Hz")
        play_tone(freq, 1000)  # 1 second tone
        time.sleep(0.5)
    
    print("✓ Audio test complete")

# Quick access functions
def quick_stylophone():
    """Quick start stylophone with minimal setup"""
    setup_audio_output()
    stylophone()

if __name__ == "__main__":
    print("Jumperless Stylophone loaded!")
    print("\nAvailable functions:")
    print("  stylophone()      - Start the full stylophone")
    print("  quick_stylophone() - Quick start")
    print("  demo_all_notes()  - Play all notes in sequence")
    print("  chord_demo()      - Demonstrate chord playing")
    print("  test_audio_setup() - Test audio output")
    print("  show_note_map()   - Show pad-to-note mapping")
else:
    print("Stylophone module loaded. Call stylophone() to start playing!") 