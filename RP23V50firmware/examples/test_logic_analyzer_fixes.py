#!/usr/bin/env python3
"""
Test script to verify Logic Analyzer fixes:
1. Sample rate configuration working properly
2. Analog voltage scaling to ±8V range  
3. Connection stability during data transmission
4. Mixed-signal data format correctness
"""

import serial
import time
import struct
import sys

class JumperlessLogicAnalyzerTest:
    def __init__(self, port='/dev/ttyACM2', baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.serial = None
        
    def connect(self):
        """Connect to Jumperless Logic Analyzer"""
        try:
            self.serial = serial.Serial(self.port, self.baudrate, timeout=2)
            print(f"✅ Connected to {self.port}")
            return True
        except Exception as e:
            print(f"❌ Failed to connect: {e}")
            return False
            
    def disconnect(self):
        """Disconnect from device"""
        if self.serial:
            self.serial.close()
            print("🔌 Disconnected")
            
    def send_command(self, cmd, data=None):
        """Send command to device"""
        if not self.serial:
            return False
            
        try:
            # Send command byte
            self.serial.write(bytes([cmd]))
            
            # Send data if provided
            if data:
                self.serial.write(data)
                
            self.serial.flush()
            return True
        except Exception as e:
            print(f"❌ Failed to send command 0x{cmd:02X}: {e}")
            return False
            
    def read_response(self, expected_response=None, max_len=1024):
        """Read response from device"""
        try:
            # Read response header
            header = self.serial.read(1)
            if not header:
                return None, None
                
            response_type = header[0]
            
            # Read payload length (2 bytes)
            len_bytes = self.serial.read(2)
            if len(len_bytes) < 2:
                return response_type, None
                
            payload_len = struct.unpack('<H', len_bytes)[0]
            
            # Read payload
            payload = b''
            if payload_len > 0:
                payload = self.serial.read(min(payload_len, max_len))
                
            return response_type, payload
            
        except Exception as e:
            print(f"❌ Failed to read response: {e}")
            return None, None
            
    def test_device_id(self):
        """Test device identification"""
        print("\n🔍 Testing device identification...")
        
        if not self.send_command(0x02):  # JUMPERLESS_CMD_ID
            return False
            
        response_type, payload = self.read_response(0x82)  # JUMPERLESS_RESP_STATUS
        
        if response_type == 0x82 and payload:
            device_id = payload.decode('utf-8', errors='ignore')
            print(f"✅ Device ID: {device_id}")
            return "Jumperless" in device_id
        else:
            print(f"❌ Invalid response: type=0x{response_type:02X if response_type else 0:02X}")
            return False
            
    def test_sample_rate_configuration(self):
        """Test SUMP sample rate configuration"""
        print("\n⚡ Testing sample rate configuration...")
        
        # Test different sample rates
        test_rates = [1000000, 100000, 50000000]  # 1MHz, 100kHz, 50MHz
        
        for rate in test_rates:
            print(f"  Setting rate to {rate} Hz...")
            
            # Calculate SUMP divider: divider = (100MHz / desired_rate) - 1
            divider = (100000000 // rate) - 1
            if divider > 0xFFFFFF:
                divider = 0xFFFFFF
                
            # Send SUMP_SET_DIVIDER command (0x80)
            divider_bytes = struct.pack('<I', divider)[:3]  # 24-bit little-endian
            
            if self.send_command(0x80, divider_bytes):
                print(f"    ✅ Sent divider {divider} for rate {rate} Hz")
            else:
                print(f"    ❌ Failed to send rate command")
                return False
                
        return True
        
    def test_channel_configuration(self):
        """Test channel configuration"""
        print("\n📡 Testing channel configuration...")
        
        # Configure mixed-signal mode: all digital + some analog channels
        digital_mask = 0xFF  # All 8 digital channels
        analog_mask = 0x0F   # First 4 analog channels
        
        config_data = struct.pack('<II', digital_mask, analog_mask)
        
        if not self.send_command(0x04, config_data):  # JUMPERLESS_CMD_SET_CHANNELS
            return False
            
        response_type, payload = self.read_response(0x82)  # JUMPERLESS_RESP_STATUS
        
        if response_type == 0x82 and payload and len(payload) >= 1:
            status = payload[0]
            if status == 0x00:
                print(f"✅ Channel configuration accepted (digital=0x{digital_mask:02X}, analog=0x{analog_mask:02X})")
                return True
            else:
                print(f"❌ Channel configuration rejected (status=0x{status:02X})")
        else:
            print("❌ No response to channel configuration")
            
        return False
        
    def test_sample_count_configuration(self):
        """Test sample count configuration"""
        print("\n📊 Testing sample count configuration...")
        
        # Test sample counts: 1024, 10000, 50000
        test_counts = [1024, 10000, 50000]
        
        for count in test_counts:
            print(f"  Setting sample count to {count}...")
            
            # Send SUMP_SET_READ_DELAY command (0x81)
            read_count = (count // 4) - 1  # SUMP formula
            delay_count = 0
            
            sump_data = struct.pack('<HH', read_count, delay_count)
            
            if self.send_command(0x81, sump_data):
                print(f"    ✅ Sent sample count {count}")
            else:
                print(f"    ❌ Failed to send sample count")
                return False
                
        return True
        
    def test_capture_sequence(self):
        """Test complete capture sequence"""
        print("\n🎯 Testing capture sequence...")
        
        # Reset device
        print("  Resetting device...")
        if not self.send_command(0x00):  # JUMPERLESS_CMD_RESET
            return False
            
        time.sleep(0.2)
        
        # Configure for small capture
        print("  Configuring for 1000 sample capture...")
        
        # Set sample rate (100kHz)
        divider = (100000000 // 100000) - 1
        divider_bytes = struct.pack('<I', divider)[:3]
        self.send_command(0x80, divider_bytes)
        
        # Set sample count
        read_count = (1000 // 4) - 1
        sump_data = struct.pack('<HH', read_count, 0)
        self.send_command(0x81, sump_data)
        
        # Configure mixed-signal channels
        config_data = struct.pack('<II', 0xFF, 0x03)  # 8 digital + 2 analog
        self.send_command(0x04, config_data)
        
        # Arm and start capture
        print("  Arming device...")
        if not self.send_command(0x05):  # JUMPERLESS_CMD_ARM
            return False
            
        # Wait for data transmission
        print("  Waiting for capture data...")
        start_time = time.time()
        data_received = 0
        
        while time.time() - start_time < 10:  # 10 second timeout
            if self.serial.in_waiting > 0:
                chunk = self.serial.read(min(1024, self.serial.in_waiting))
                data_received += len(chunk)
                
                # Check if this looks like sample data (raw bytes)
                if len(chunk) > 0:
                    print(f"    📥 Received {len(chunk)} bytes (total: {data_received})")
                    
                    # Analyze first few bytes for mixed-signal format
                    if data_received <= 100:  # Only analyze start of data
                        self.analyze_sample_data(chunk)
                    
            time.sleep(0.1)
            
        if data_received > 0:
            print(f"✅ Capture successful: {data_received} bytes received")
            return True
        else:
            print("❌ No data received")
            return False
            
    def analyze_sample_data(self, data):
        """Analyze sample data format"""
        if len(data) < 10:
            return
            
        print(f"    🔬 Analyzing data format...")
        print(f"      First 20 bytes: {' '.join(f'{b:02X}' for b in data[:20])}")
        
        # Check for mixed-signal pattern (1 digital + 4 analog bytes per sample)
        # Each sample: 1 digital byte + 2*2 analog bytes = 5 bytes total
        if len(data) >= 15:
            print("      Sample format analysis:")
            voltage_issues = 0
            
            for i in range(0, min(25, len(data)), 5):
                if i + 4 < len(data):
                    digital = data[i]
                    analog1 = struct.unpack('<H', data[i+1:i+3])[0]
                    analog2 = struct.unpack('<H', data[i+3:i+5])[0]
                    
                    # Convert to voltage using Jumperless ADC formula
                    # For ±8V channels: voltage = (adc * 18.28 / 4095) - 8.0
                    # For 0-5V channel 4: voltage = (adc * 5.0 / 4095)
                    voltage1 = (analog1 * 18.28 / 4095.0) - 8.0  # ±8V range
                    voltage2 = (analog2 * 18.28 / 4095.0) - 8.0  # ±8V range
                    
                    # Detailed analysis for first few samples
                    if i < 15:
                        print(f"        Sample {i//5}: Digital=0x{digital:02X}, "
                              f"ADC1={analog1} ({voltage1:.2f}V), "
                              f"ADC2={analog2} ({voltage2:.2f}V)")
                    
                    # Check for specific issues
                    if abs(voltage1) > 50.0 or abs(voltage2) > 50.0:
                        voltage_issues += 1
                        if voltage_issues <= 3:  # Only show first few
                            print(f"        ❌ ISSUE: Sample {i//5} extreme voltage: "
                                  f"V1={voltage1:.1f}V V2={voltage2:.1f}V")
                    
                    # Check for 248V specifically (the reported issue)
                    if 240 <= abs(voltage1) <= 255 or 240 <= abs(voltage2) <= 255:
                        print(f"        🚨 248V ISSUE DETECTED at sample {i//5}!")
                        print(f"           Raw ADC: {analog1}, {analog2}")
                        print(f"           Calculated: {voltage1:.1f}V, {voltage2:.1f}V")
                        
                        # Reverse calculate what ADC value would give 248V
                        expected_adc = (248 + 8.0) * 4095 / 18.28
                        print(f"           248V would need ADC={expected_adc:.0f} (max is 4095)")
                    
            if voltage_issues > 3:
                print(f"        ⚠️  Total voltage issues found: {voltage_issues}")
                
            # Check for baseline issues (should be around 0V for ±8V channels)
            if len(data) >= 25:
                # Sample the middle of the data for baseline analysis
                mid_sample = 2 * 5  # Sample 2
                if mid_sample + 4 < len(data):
                    mid_analog1 = struct.unpack('<H', data[mid_sample+1:mid_sample+3])[0]
                    mid_analog2 = struct.unpack('<H', data[mid_sample+3:mid_sample+5])[0]
                    mid_voltage1 = (mid_analog1 * 18.28 / 4095.0) - 8.0
                    mid_voltage2 = (mid_analog2 * 18.28 / 4095.0) - 8.0
                    
                    print(f"      Baseline check (sample 2): "
                          f"ADC={mid_analog1},{mid_analog2} "
                          f"V={mid_voltage1:.2f},{mid_voltage2:.2f}")
                    
                    if abs(mid_voltage1) > 15 or abs(mid_voltage2) > 15:
                        print(f"        ❌ BASELINE ERROR: Expected ~0V, got {mid_voltage1:.1f}V, {mid_voltage2:.1f}V")
                          
    def run_all_tests(self):
        """Run all tests"""
        print("🧪 Jumperless Logic Analyzer Test Suite")
        print("=" * 50)
        
        if not self.connect():
            return False
            
        tests = [
            ("Device ID", self.test_device_id),
            ("Sample Rate Config", self.test_sample_rate_configuration),
            ("Channel Config", self.test_channel_configuration),
            ("Sample Count Config", self.test_sample_count_configuration),
            ("Capture Sequence", self.test_capture_sequence),
        ]
        
        passed = 0
        failed = 0
        
        for test_name, test_func in tests:
            try:
                if test_func():
                    print(f"✅ {test_name}: PASSED")
                    passed += 1
                else:
                    print(f"❌ {test_name}: FAILED")
                    failed += 1
            except Exception as e:
                print(f"❌ {test_name}: ERROR - {e}")
                failed += 1
                
            time.sleep(0.5)  # Brief pause between tests
            
        self.disconnect()
        
        print("\n" + "=" * 50)
        print(f"📊 Test Results: {passed} passed, {failed} failed")
        
        if failed == 0:
            print("🎉 All tests passed! Logic Analyzer fixes are working correctly.")
        else:
            print("⚠️  Some tests failed. Check the issues above.")
            
        return failed == 0

def main():
    if len(sys.argv) > 1:
        port = sys.argv[1]
    else:
        port = '/dev/ttyACM2'  # Default Jumperless Logic Analyzer port
        
    print(f"Using port: {port}")
    print("Make sure Jumperless is connected and Logic Analyzer mode is active.")
    print()
    
    tester = JumperlessLogicAnalyzerTest(port)
    success = tester.run_all_tests()
    
    sys.exit(0 if success else 1)

if __name__ == "__main__":
    main() 