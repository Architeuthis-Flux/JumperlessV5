#!/usr/bin/env python3
"""
Test script for the new unified Jumperless data format.
This script demonstrates how to parse the new format.
"""

import struct

# Data format markers (must match firmware)
DIGITAL_ONLY_MARKER = 0xDD
MIXED_SIGNAL_MARKER = 0xDA
ANALOG_ONLY_MARKER = 0xAA
ANALOG_EOF_MARKER = 0xA0

def parse_unified_format(data):
    """Parse data in the new unified format."""
    offset = 0
    samples = []
    
    while offset + 3 <= len(data):
        # Read the 3-byte header
        gpio_byte = data[offset]
        uart_byte = data[offset + 1]
        marker = data[offset + 2]
        offset += 3
        
        sample = {
            'gpio': gpio_byte,
            'uart': uart_byte,
            'marker': hex(marker),
            'digital_channels': [],
            'analog_channels': []
        }
        
        # Parse digital channels from GPIO byte
        for bit in range(8):
            sample['digital_channels'].append((gpio_byte >> bit) & 1)
        
        if marker == DIGITAL_ONLY_MARKER:
            # Digital-only sample: 3 bytes total
            sample['type'] = 'digital_only'
            
        elif marker == MIXED_SIGNAL_MARKER:
            # Mixed-signal sample: 32 bytes total (3 + 28 + 1)
            sample['type'] = 'mixed_signal'
            
            if offset + 29 <= len(data):  # 28 analog + 1 EOF
                # Parse 14 analog channels (2 bytes each, little-endian)
                for ch in range(14):
                    if offset + 2 <= len(data):
                        low_byte = data[offset]
                        high_byte = data[offset + 1]
                        adc_value = low_byte | (high_byte << 8)
                        
                        # Convert to voltage based on channel type
                        if ch < 8:
                            # ADC channels 0-7
                            if ch == 4:
                                # ADC 4 is 0-5V range
                                voltage = (adc_value * 5.0) / 4095.0
                            else:
                                # ADCs 0-3,7 are ±8V range
                                voltage = ((adc_value * 18.28) / 4095.0) - 8.0
                        elif ch < 10:
                            # DAC channels 8-9
                            voltage = ((adc_value * 18.28) / 4095.0) - 8.0
                        else:
                            # INA channels 10-13 (voltage and current)
                            if (ch - 10) % 2 == 0:
                                # Voltage
                                voltage = ((adc_value * 18.28) / 4095.0) - 8.0
                            else:
                                # Current (example scaling)
                                voltage = ((adc_value * 3.3) / 4095.0) - 1.65
                        
                        sample['analog_channels'].append({
                            'channel': ch,
                            'raw': adc_value,
                            'voltage': voltage
                        })
                        
                        offset += 2
                
                # Check EOF marker
                if offset < len(data):
                    eof_marker = data[offset]
                    if eof_marker == ANALOG_EOF_MARKER:
                        sample['eof_valid'] = True
                    else:
                        sample['eof_valid'] = False
                        sample['eof_error'] = hex(eof_marker)
                    offset += 1
                
        elif marker == ANALOG_ONLY_MARKER:
            # Analog-only sample: same as mixed-signal but digital data is dummy
            sample['type'] = 'analog_only'
            sample['digital_channels'] = ['dummy'] * 8  # Mark as dummy data
            
            # Parse analog data same way as mixed-signal
            # ... (same analog parsing code as above)
            
        else:
            # Unknown marker
            sample['type'] = 'unknown'
            sample['error'] = f'Unknown marker: {hex(marker)}'
            break
        
        samples.append(sample)
    
    return samples

def test_digital_only_format():
    """Test parsing digital-only format."""
    print("Testing digital-only format...")
    
    # Create test data: 3 samples of digital-only data
    test_data = bytes([
        0xAA, 0x00, DIGITAL_ONLY_MARKER,  # Sample 1: GPIO=0xAA
        0x55, 0x00, DIGITAL_ONLY_MARKER,  # Sample 2: GPIO=0x55
        0xFF, 0x00, DIGITAL_ONLY_MARKER   # Sample 3: GPIO=0xFF
    ])
    
    samples = parse_unified_format(test_data)
    
    for i, sample in enumerate(samples):
        print(f"  Sample {i+1}: {sample['type']}, GPIO=0x{sample['gpio']:02X}, Channels={sample['digital_channels']}")
    
    assert len(samples) == 3
    assert all(s['type'] == 'digital_only' for s in samples)
    print("  ✅ Digital-only format test passed!")

def test_mixed_signal_format():
    """Test parsing mixed-signal format."""
    print("Testing mixed-signal format...")
    
    # Create test data: 1 sample of mixed-signal data
    gpio_data = 0xAA
    uart_data = 0x00
    
    # Create analog data for 14 channels (28 bytes)
    analog_data = []
    for ch in range(14):
        # Create test ADC values
        if ch == 4:
            # ADC 4: 2.5V (mid-scale for 0-5V range)
            adc_val = 2048
        else:
            # Other channels: 0V (for ±8V range)
            adc_val = 1791
        
        # Add variation
        adc_val += ch * 100
        
        # Store as little-endian
        analog_data.append(adc_val & 0xFF)
        analog_data.append((adc_val >> 8) & 0xFF)
    
    test_data = bytes([gpio_data, uart_data, MIXED_SIGNAL_MARKER] + 
                     analog_data + [ANALOG_EOF_MARKER])
    
    samples = parse_unified_format(test_data)
    
    assert len(samples) == 1
    sample = samples[0]
    
    print(f"  Sample: {sample['type']}, GPIO=0x{sample['gpio']:02X}")
    print(f"  Digital channels: {sample['digital_channels']}")
    print(f"  Analog channels: {len(sample['analog_channels'])}")
    print(f"  EOF valid: {sample.get('eof_valid', False)}")
    
    # Check first few analog channels
    for i, ch_data in enumerate(sample['analog_channels'][:5]):
        print(f"    CH{ch_data['channel']}: {ch_data['voltage']:.3f}V (raw={ch_data['raw']})")
    
    assert sample['type'] == 'mixed_signal'
    assert len(sample['analog_channels']) == 14
    assert sample.get('eof_valid', False) == True
    print("  ✅ Mixed-signal format test passed!")

def test_format_validation():
    """Test format validation and error handling."""
    print("Testing format validation...")
    
    # Test incomplete data
    incomplete_data = bytes([0xAA, 0x00])  # Missing marker
    samples = parse_unified_format(incomplete_data)
    assert len(samples) == 0
    print("  ✅ Incomplete data handled correctly")
    
    # Test unknown marker - parser should stop on unknown marker
    unknown_marker_data = bytes([0xAA, 0x00, 0x99])  # Unknown marker
    samples = parse_unified_format(unknown_marker_data)
    assert len(samples) == 0  # Parser stops on unknown marker
    print("  ✅ Unknown marker handled correctly (parser stops safely)")
    
    print("  ✅ Format validation tests passed!")

if __name__ == '__main__':
    print("🧪 Testing new unified Jumperless data format")
    print("=" * 50)
    
    test_digital_only_format()
    print()
    
    test_mixed_signal_format()
    print()
    
    test_format_validation()
    print()
    
    print("🎉 All tests passed! New format is working correctly.")
    print("\nFormat summary:")
    print("- Digital-only: 3 bytes per sample")
    print("- Mixed-signal: 32 bytes per sample (3 + 28 + 1)")
    print("- Self-describing with marker bytes")
    print("- Robust error detection with EOF markers") 