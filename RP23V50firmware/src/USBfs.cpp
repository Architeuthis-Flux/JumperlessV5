/*
    FatFSUSB - Export onboard FatFS/FTL to host for data movement
    Copyright (c) 2024 Earle F. Philhower, III.  All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
//This is a copy of the FATFSUSB.cpp file from arduino-pico
//I have made some changes to it to make it work with the Jumperless V5

#include "USBfs.h"
#include <FatFS.h>
#include "tusb.h"
#include "Commands.h"
#include "RotaryEncoder.h"  // for netSlot
#include "FileParsing.h"    // for validation functions
#include "LEDs.h"           // for core synchronization variables
#include "FilesystemStuff.h" // for initializeMicroPythonExamples
// #include <class/msc/msc.h>
bool mscModeEnabled = false;
FatFSUSBClass FatFSUSB;

// Periodic maintenance (called from main loop)
void usbPeriodic(void) {
    static unsigned long mscModeRefreshTimer = 0;
    const unsigned long mscModeRefreshInterval = 2000;
    if (mscModeEnabled == true) {
        if (millis() - mscModeRefreshTimer > mscModeRefreshInterval) {
            mscModeRefreshTimer = millis();
            // Optionally refresh or check status here
            // refreshUSBFilesystem();
        }
    }
}

// Ensure we are logged in to the USB framework
void __USBInstallMassStorage() {
    /* dummy */
}

FatFSUSBClass::FatFSUSBClass() {
}

FatFSUSBClass::~FatFSUSBClass() {
    end();
    mscModeEnabled = false;
}

void FatFSUSBClass::onPlug(void (*cb)(uint32_t), uint32_t cbData) {
    _cbPlug = cb;
    _cbPlugData = cbData;
}

void FatFSUSBClass::onUnplug(void (*cb)(uint32_t), uint32_t cbData) {
    _cbUnplug = cb;
    _cbUnplugData = cbData;
    mscModeEnabled = false;
}

void FatFSUSBClass::driveReady(bool (*cb)(uint32_t), uint32_t cbData) {
    _driveReady = cb;
    _driveReadyData = cbData;
}

bool FatFSUSBClass::begin() {
    mscModeEnabled = true;
    if (_started) {
        return false;
    }
    _started = true;
    fatfs::disk_initialize(0);
    fatfs::WORD ss;
    fatfs::disk_ioctl(0, GET_SECTOR_SIZE, &ss);
    _sectSize = ss;
    _sectBuff = new uint8_t[_sectSize];
    _sectNum = -1;
    
    // Set the volume label to "JUMPERLESS" for proper drive naming
    setVolumeLabel();
    
    return true;
}

void FatFSUSBClass::end() {
    if (_started) {
        _started = false;
        delete[] _sectBuff;
    }
}

// These callbacks are now handled by the TinyUSB wrapper system in USBfilesystemStuff.cpp
// to avoid multiple definition conflicts

bool FatFSUSBClass::testUnitReady() {
    bool ret = _started;
    if (_driveReady) {
        ret &= _driveReady(_driveReadyData);
    }
    return ret;
}

// TinyUSB callbacks removed - handled by wrapper system

int32_t FatFSUSBClass::read10(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    fatfs::LBA_t _hddsects;
    fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &_hddsects);

    if (!_started || (lba >= _hddsects)) {
        return -1;
    }

    assert(offset + bufsize <= _sectSize);

    if (_sectNum >= 0) {
        // Flush the temp data out, we need to use the space
        fatfs::disk_write(0, _sectBuff, _sectNum, 1);
        _sectNum = -1;
    }
    fatfs::disk_read(0, _sectBuff, lba, 1);
    memcpy(buffer, _sectBuff + offset, bufsize);
    return bufsize;
}

// TinyUSB callbacks removed - handled by wrapper system

int32_t FatFSUSBClass::write10(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    fatfs::LBA_t _hddsects;
    fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &_hddsects);

    if (!_started || (lba >= _hddsects)) {
        return -1;
    }

    assert(offset + bufsize <= _sectSize);

    if ((offset == 0) && (bufsize == _sectSize)) {
        fatfs::disk_write(0, buffer, lba, 1);
        return _sectSize;
    }

    if ((int)_sectNum == (int)lba) {
        memcpy(_sectBuff + offset, buffer, bufsize);
    } else {
        if (_sectNum >= 0) {
            // Need to flush old sector out
            fatfs::disk_write(0, _sectBuff, _sectNum, 1);
        }
        fatfs::disk_read(0, _sectBuff, lba, 1);
        memcpy(_sectBuff + offset, buffer, bufsize);
        _sectNum = lba;
    }

    if (offset + bufsize >= _sectSize) {
        // We've filled up a sector, write it out!
        fatfs::disk_write(0, _sectBuff, lba, 1);
        _sectNum = -1;
    }
    return bufsize;
}

// TinyUSB callbacks removed - handled by wrapper system

// Declare TinyUSB MSC function
extern "C" bool tud_msc_set_sense(uint8_t lun, uint8_t sense_key, uint8_t add_sense_code, uint8_t add_sense_qualifier);

// Declare TinyUSB device control functions for disconnect/connect
extern "C" {
    extern bool tud_disconnect(void) __attribute__((weak));
    extern bool tud_connect(void) __attribute__((weak));
}

// Internal SCSI command handler - called by wrapper system
int32_t FatFSUSBClass::handleSCSICommand(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    const int SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E;
    const int SCSI_CMD_START_STOP_UNIT              = 0x1B;
    const int SCSI_SENSE_ILLEGAL_REQUEST = 0x05;

    void const* response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;
    
    switch (scsi_cmd[0]) {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
        // Host is about to read/write etc ... acknowledge without triggering callbacks
        // The start/stop callback handles mount/eject status properly
        resplen = 0;
        break;
    case SCSI_CMD_START_STOP_UNIT:
        {
        // Host try to eject/safe remove/poweroff us
        // Don't call plug/unplug here - let the start/stop callback handle it
        // This prevents double-processing and potential reenumeration issues
        resplen = 0;
        break;
        }
    default:
        // Set Sense = Invalid Command Operation
        tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);
        // negative means error -> tinyusb could stall and/or response with failed status
        resplen = -1;
        break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) {
        resplen = bufsize;
    }

    if (response && (resplen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, resplen);
        } else {
            // SCSI output
        }
    }
    Serial.println("SCSI command handled");
    return resplen;
}

void FatFSUSBClass::plug() {
    if (_started && _cbPlug) {
        _cbPlug(_cbPlugData);
    }
    Serial.println("Plugged in");
}

void FatFSUSBClass::unplug() {
    if (_started) {
        if (_sectNum >= 0) {
            // Flush the temp data out
            fatfs::disk_write(0, _sectBuff, _sectNum, 1);
            _sectNum = -1;
        }
        fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
        if (_cbUnplug) {
            _cbUnplug(_cbUnplugData);
        }
        Serial.println("Unplugged");
    }
}

void FatFSUSBClass::setVolumeLabel() {
    // Set the volume label to "JUMPERLESS" so it appears correctly in file managers
    // This is more robust than trying to override USB device strings
    
    // Check if FatFS is available first
    if (!FatFS.begin()) {
        Serial.println("Warning: Could not mount FatFS to set volume label");
        return;
    }
    
    // Get current volume label to see if it needs changing
    char currentLabel[12] = {0}; // Volume labels are max 11 chars + null terminator
    fatfs::DWORD serialNum;
    
    fatfs::FRESULT result = fatfs::f_getlabel("", currentLabel, &serialNum);
    if (result == fatfs::FR_OK) {
        // Check if label is already set to JUMPERLESS
        if (strcmp(currentLabel, "JUMPERLESS") == 0) {
           // Serial.println("Volume label already set to 'JUMPERLESS'");
            return;
        }
        
        //Serial.printf("Current volume label: '%s', updating to 'JUMPERLESS'\n", 
                    // strlen(currentLabel) > 0 ? currentLabel : "(none)");
    } else {
      //  Serial.printf("Setting volume label to 'JUMPERLESS' (previous label unknown)\n");
    }
    
    // Set the new volume label
    result = fatfs::f_setlabel("JUMPERLESS");
    if (result == fatfs::FR_OK) {
      //  Serial.println("Volume label successfully set to 'JUMPERLESS'");
        
        // Sync to ensure the change is written to flash
        fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
    } else {
        Serial.printf("Warning: Failed to set volume label (error %d)\n", result);
        
        // Try to provide helpful error messages
        switch (result) {
            case fatfs::FR_WRITE_PROTECTED:
                Serial.println("Filesystem is write-protected");
                break;
            case fatfs::FR_INVALID_NAME:
                Serial.println("Invalid label name format");
                break;
            case fatfs::FR_NOT_READY:
                Serial.println("Filesystem not ready");
                break;
            default:
                Serial.println("Unknown filesystem error");
                break;
        }
    }
}

// TinyUSB callbacks removed - handled by wrapper system

//==============================================================================
// Global State and TinyUSB Integration
//==============================================================================

// Using volatile to prevent compiler optimization issues
static volatile bool usb_msc_mounted = false;   // Host has mounted the device
static volatile bool usb_msc_ejected = false;   // Host has ejected the device
static volatile bool flash_ready = false;       // Flash is initialized and ready

// Debug control for USB filesystem operations
bool usb_debug_enabled = false;

//==============================================================================
// USB Filesystem Functions
//==============================================================================

void setUSBDebug(bool enable) {
    usb_debug_enabled = enable;
    if (usb_debug_enabled) {
        Serial.println("USB filesystem debug enabled");
    } else {
        Serial.println("USB filesystem debug disabled");
    }
}

String readSlotFileContent(int slot) {
    String filename = "nodeFileSlot" + String(slot) + ".txt";
    String content = "";
    
    // Use core synchronization to prevent flash access conflicts
    while (core2busy == true) {
        // Wait for core2 to finish
    }
    core1busy = true;
    
    if (FatFS.exists(filename)) {
        File slotFile = FatFS.open(filename, "r");
        if (slotFile) {
            content = slotFile.readString();
            slotFile.close();
        }
    }
    
    core1busy = false;
    return content;
}

bool isSlotContentValid(const String& content) {
    // Use the comprehensive validation function from FileParsing.cpp
    // Return true only if validation passes (error code 0)
    return (validateNodeFile(content, false) == 0);
}

void validateAllSlots(bool verbose) {
    if (usb_debug_enabled || verbose) {
        Serial.println("Validating all slot files...");
    }
    
    // Use core synchronization to prevent flash access conflicts
    while (core2busy == true) {
        // Wait for core2 to finish
    }
    core1busy = true;
    
    int valid_slots = 0;
    int invalid_slots = 0;
    int total_connections = 0;
    
    for (int i = 0; i < 8; i++) { // Assuming 8 slots (0-7)
        String filename = "nodeFileSlot" + String(i) + ".txt";
        
        if (!FatFS.exists(filename)) {
            if (usb_debug_enabled && verbose) Serial.println("  Slot " + String(i) + ": File does not exist");
            continue;
        }
        
        int result = validateNodeFileSlot(i, false); // Always non-verbose for this
        
        if (result == 0) {
            valid_slots++;
            if (usb_debug_enabled && verbose) {
                String content = readSlotFileContent(i);
                int connections = 0;
                int commaCount = 0;
                for (int j = 0; j < content.length(); j++) {
                    if (content.charAt(j) == ',') commaCount++;
                }
                if (commaCount > 0) connections = commaCount; // Rough estimate
                total_connections += connections;
                Serial.println("  Slot " + String(i) + ": VALID (" + String(connections) + " connections)");
            }
        } else {
            invalid_slots++;
            if (usb_debug_enabled && verbose) {
                Serial.println("  Slot " + String(i) + ": INVALID - " + String(getNodeFileValidationError(result)));
            }
        }
    }
    
    if (usb_debug_enabled || verbose) {
        Serial.println("Slot validation complete:");
        Serial.println("  - Valid slots: " + String(valid_slots));
        Serial.println("  - Invalid slots: " + String(invalid_slots));
        Serial.println("  - Total connections: ~" + String(total_connections));
    }
    
    core1busy = false;
}

void promptRefreshConnections() {
    // Check if the current slot might have changed on disk
    String filename = "nodeFileSlot" + String(netSlot) + ".txt";
    if (FatFS.exists(filename)) {
        Serial.println("USB drive mounted - slot files may have been modified");
        Serial.println("Type 'y' to refresh connections, or any other key to skip");
        Serial.flush();
    }
}

void manualRefreshFromUSB() {
    //Serial.println("Manually refreshing connections from USB changes...");
    
    // Multi-level cache invalidation approach based on research:
    // 1. Sync filesystem to ensure USB host changes are visible
    fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
    
    // 2. Add memory barrier to prevent compiler optimizations
    __sync_synchronize();
    
    // 3. Force close any open files to invalidate file handles
    // This ensures we're not reading from cached file handles
    while (core2busy == true) {
        // Wait for core2 to finish
    }
    core1busy = true;
    
    // Force close any potentially open nodeFile handles and refresh filesystem
    extern File nodeFile;
    if (nodeFile) {
        nodeFile.close();
    }
    
    // Force a filesystem "refresh" by attempting to remount
    // This invalidates internal FatFS caches
    FatFS.end();
    delay(10);  // Brief delay for hardware to settle
    FatFS.begin();
    
    core1busy = false;
    
    // 4. Clear cached file content to force re-read from disk
    clearNodeFileString();
    
    // 5. Small delay to address USB timing window issues
    // Research shows this helps with high-speed USB transfer timing
    delayMicroseconds(100);
    
    // 6. Additional memory barrier after all cache operations
    __sync_synchronize();
    
    // 7. Validate current slot
    int validation_result = validateNodeFileSlot(netSlot, usb_debug_enabled);
    if (validation_result == 0) {
        if (usb_debug_enabled) {
            Serial.println("Slot " + String(netSlot) + " validated successfully");
        }
        refreshConnections(-1);
        //Serial.println("Connections refreshed from slot " + String(netSlot));
    } else {
        //Serial.println("Slot " + String(netSlot) + " validation failed: " + String(getNodeFileValidationError(validation_result)));
        //Serial.println("Connections not refreshed due to invalid node file");
    }
}



//==============================================================================
// Public Interface Functions
//==============================================================================

bool initUSBMassStorage(void) {
   // delay(1000);
    if (usb_debug_enabled) {
        Serial.println("Initializing USB Mass Storage with direct FatFS access...");
    }
    
    // Initialize FatFSUSB class
    if (!FatFSUSB.begin()) {
        Serial.println("Failed to initialize FatFSUSB");
        return false;
    }
    
    // Automatically create MicroPython examples if needed
    //initializeMicroPythonExamples();
    
    if (usb_debug_enabled) {
        Serial.println("FatFSUSB initialized successfully");
        Serial.println("Device will appear as 'JUMPERLESS' drive (direct FatFS access)");
        Serial.println("Host can read/write actual flash filesystem directly");
    }
    
    // Set flash ready
    __sync_synchronize();
    flash_ready = true;
    __sync_synchronize();
    
    if (usb_debug_enabled) {
        Serial.println("Direct FatFS access ready - no virtual filesystem needed!");
    }
    return true;
}

bool disableUSBMassStorage(void) {
    if (usb_debug_enabled) {
        Serial.println("◆ Disabling USB Mass Storage...");
    }
    
    // Force disconnect from host perspective first
    Serial.println("◆ Disconnecting USB device from host...");
    
    // Disconnect the USB device - this will force the host to see it as ejected
    if (tud_disconnect) {
        tud_disconnect();
        delay(100);  // Give host time to process the disconnect
    }
    
    // End FatFSUSB operations
    FatFSUSB.end();
    
    // Clear status flags
    __sync_synchronize();
    mscModeEnabled = false;
    usb_msc_mounted = false;
    usb_msc_ejected = false;
    flash_ready = false;
    __sync_synchronize();
    
    // Reconnect without MSC interface active
    if (tud_connect) {
        tud_connect();
    }
    
    if (usb_debug_enabled) {
        Serial.println("◆ USB Mass Storage disabled - device disconnected from host");
    } else {
        Serial.println("◆ USB Mass Storage disabled - device disconnected from host");
    }
    
    return true;
}

void refreshUSBFilesystem() {
    if (usb_debug_enabled) {
        Serial.println("Refreshing USB filesystem...");
    }
    
    // With direct FatFS access, the host sees changes immediately
    // Just ensure FatFS is properly synced
    fatfs::disk_ioctl(0, CTRL_SYNC, nullptr);
    
    if (usb_debug_enabled) {
        Serial.println("FatFS synchronized - changes are immediately visible to host");
        Serial.println("No USB reconnection needed - host will see current filesystem state");
        Serial.println("Manual refresh available when needed");
    }
}

void debugUSBFilesystem() {
    Serial.println("╭─────────────────────────────────────╮");
    Serial.println("│    USB Filesystem Debug Info        │");
    Serial.println("│    (FatFSUSB Direct Access)         │");
    Serial.println("├─────────────────────────────────────┤");
    
    // Test FatFS basic functionality
    Serial.print("│ FatFS available: ");
    Serial.println(FatFS.exists("/") ? "YES" : "NO");
    
    // Get FatFS capacity
    fatfs::LBA_t sectorCount;
    fatfs::WORD sectorSize;
    fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &sectorCount);
    fatfs::disk_ioctl(0, GET_SECTOR_SIZE, &sectorSize);
    
    Serial.printf("│ Disk capacity: %u sectors × %u bytes\n", sectorCount, sectorSize);
    Serial.printf("│ Total size: %u KB\n", (sectorCount * sectorSize) / 1024);
    
    // Test directory opening and list some files
    Serial.println("│ Root directory contents:");
    Dir testRoot = FatFS.openDir("/");
    int fileCount = 0;
    
    while (testRoot.next() && fileCount < 8) {
        fileCount++;
        Serial.printf("│   %s %s (%u bytes)\n", 
                     testRoot.isDirectory() ? "⌘" : "⍺",
                     testRoot.fileName().c_str(),
                     testRoot.fileSize());
    }
    
    if (fileCount == 8) {
        // Count remaining files
        int remainingCount = 0;
        while (testRoot.next()) {
            remainingCount++;
        }
        if (remainingCount > 0) {
            Serial.printf("│   ... and %d more files\n", remainingCount);
        }
    }
    
    if (fileCount == 0) {
        Serial.println("│   (No files found)");
    }
    
    // Test FatFSUSB status
    Serial.println("├─────────────────────────────────────┤");
    Serial.printf("│ FatFSUSB ready: %s\n", FatFSUSB.testUnitReady() ? "YES" : "NO");
    Serial.printf("│ USB mounted: %s\n", usb_msc_mounted ? "YES" : "NO");
    Serial.printf("│ USB ejected: %s\n", usb_msc_ejected ? "YES" : "NO");
    
    Serial.println("│ Direct access mode: Host reads/writes");
    Serial.println("│ actual FatFS flash sectors directly");
    Serial.println("╰─────────────────────────────────────╯");
}

bool isUSBMassStorageActive(void) {
    // Check if USB MSC is enabled and ready, even if host hasn't mounted yet
    bool msc_active = (mscModeEnabled && FatFSUSB.testUnitReady());
    if (usb_debug_enabled) {
        Serial.printf("isUSBMassStorageActive() called: mscModeEnabled=%s, FatFSUSB.testUnitReady()=%s, usb_msc_mounted=%s, returning %s\n", 
                     mscModeEnabled ? "true" : "false",
                     FatFSUSB.testUnitReady() ? "true" : "false",
                     usb_msc_mounted ? "true" : "false",
                     msc_active ? "true" : "false");
    }
    return msc_active;
}

bool isUSBMassStorageMounted(void) {
    // Keep the old function for compatibility, but now it checks if host has actually mounted
    if (usb_debug_enabled) {
        Serial.printf("isUSBMassStorageMounted() called: returning %s\n", usb_msc_mounted ? "true" : "false");
    }
    return usb_msc_mounted;
}

bool isUSBMassStorageEjected(void) {
    return usb_msc_ejected;
}

void printUSBMassStorageStatus(void) {
    Serial.println("╭─────────────────────────────────╮");
    Serial.println("│    USB Mass Storage Status      │");
    Serial.println("├─────────────────────────────────┤");
    Serial.printf("│ Flash Ready:   %-16s │\n", flash_ready ? "YES" : "NO");
    Serial.printf("│ Host Mounted:  %-16s │\n", usb_msc_mounted ? "YES" : "NO");
    Serial.printf("│ Host Ejected:  %-16s │\n", usb_msc_ejected ? "YES" : "NO");
    Serial.printf("│ FatFSUSB:      %-16s │\n", FatFSUSB.testUnitReady() ? "READY" : "NOT READY");
    Serial.printf("│ Debug Mode:    %-16s │\n", usb_debug_enabled ? "ENABLED" : "DISABLED");
    Serial.println("├─────────────────────────────────┤");
    Serial.printf("│ Current Slot:  %-16d │\n", netSlot);
    Serial.println("│ Manual refresh: type 'y' after  │");
    Serial.println("│ mounting to refresh connections │");
    Serial.println("╰─────────────────────────────────╯");
}

void testUSBMassStorage(void) {
    Serial.println("Testing USB Mass Storage with FatFSUSB...");
    
    // Test FatFSUSB functionality
    Serial.printf("FatFSUSB ready: %s\n", FatFSUSB.testUnitReady() ? "YES" : "NO");
    Serial.printf("Flash ready: %s\n", flash_ready ? "YES" : "NO");
    Serial.printf("USB mounted: %s\n", tud_mounted() ? "YES" : "NO");
    
    // Test FatFS capacity through FatFSUSB
    fatfs::LBA_t sectorCount;
    fatfs::WORD sectorSize;
    fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &sectorCount);
    fatfs::disk_ioctl(0, GET_SECTOR_SIZE, &sectorSize);
    
    Serial.printf("FatFS capacity: %u sectors × %u bytes = %u KB\n", 
                  sectorCount, sectorSize, (sectorCount * sectorSize) / 1024);
    
    // Test TinyUSB MSC callbacks through our wrappers
    Serial.println("Testing TinyUSB MSC callback integration:");
    
    uint32_t block_count = 0;
    uint16_t block_size = 0;
    tud_msc_capacity_cb(0, &block_count, &block_size);
    Serial.printf("  - Capacity callback: %u blocks × %u bytes\n", block_count, block_size);
    
    bool ready = tud_msc_test_unit_ready_cb(0);
    Serial.printf("  - Test unit ready: %s\n", ready ? "READY" : "NOT READY");
    
    uint8_t vendor[8] = {0}, product[16] = {0}, rev[4] = {0};
    tud_msc_inquiry_cb(0, vendor, product, rev);
    Serial.printf("  - Device ID: %.8s %.16s %.4s\n", vendor, product, rev);
    
    // Test a simple read operation
    Serial.println("Testing direct flash read:");
    uint8_t testBuffer[512];
    int32_t readResult = FatFSUSB.read10(0, 0, testBuffer, 512);
    Serial.printf("  - Read sector 0: %s (%d bytes)\n", 
                  readResult > 0 ? "SUCCESS" : "FAILED", readResult);
    
    if (readResult > 0) {
        Serial.print("  - Boot sector signature: ");
        if (testBuffer[510] == 0x55 && testBuffer[511] == 0xAA) {
            Serial.println("VALID (0x55AA)");
        } else {
            Serial.printf("INVALID (0x%02X%02X)\n", testBuffer[510], testBuffer[511]);
        }
    }
    
    Serial.println("FatFSUSB integration test complete!");
    Serial.println("Host should now see actual FatFS filesystem directly");
}

bool getUSBMountedStatus(void) {
    return tud_mounted();
}

//==============================================================================
// TinyUSB MSC Callbacks - Integrated into USBfs
//==============================================================================

extern "C" {

// Declare the real MSC callback functions for linker wrapping (if they exist)
extern void __real_tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) __attribute__((weak));
extern bool __real_tud_msc_test_unit_ready_cb(uint8_t lun) __attribute__((weak));
extern void __real_tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count_out, uint16_t* block_size_out) __attribute__((weak));

// Use linker wrapping like the USB descriptors
__attribute__((used)) void __wrap_tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    (void) lun;
    
    // One-time success message to show MSC enumeration 
    static bool first_inquiry = true;
    if (first_inquiry) {
        first_inquiry = false;
        Serial.println("USB Mass Storage successfully enumerated by host!");
    }
    
    // Provide device identification
    memcpy(vendor_id, "ARCHFLUX", 8);
    memcpy(product_id, "JUMPERLESS      ", 16); 
    memcpy(product_rev, "5.0 ", 4);
}

__attribute__((used)) bool __wrap_tud_msc_test_unit_ready_cb(uint8_t lun) {
    (void) lun;
    return FatFSUSB.testUnitReady();  // Use FatFSUSB to determine readiness
}

__attribute__((used)) void __wrap_tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count_out, uint16_t* block_size_out) {
    (void) lun;
    
    if (block_count_out && block_size_out) {
        // Get capacity from FatFS directly
        fatfs::LBA_t block_count;
        fatfs::WORD block_size;
        fatfs::disk_ioctl(0, GET_SECTOR_COUNT, &block_count);
        fatfs::disk_ioctl(0, GET_SECTOR_SIZE, &block_size);
        
        *block_count_out = block_count;
        *block_size_out = block_size;
    }
}

// Also provide the regular callback names as aliases in case both are needed
void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) __attribute__((alias("__wrap_tud_msc_inquiry_cb")));
bool tud_msc_test_unit_ready_cb(uint8_t lun) __attribute__((alias("__wrap_tud_msc_test_unit_ready_cb")));
void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count_out, uint16_t* block_size_out) __attribute__((alias("__wrap_tud_msc_capacity_cb")));

// Add wrapping for the remaining MSC callbacks
__attribute__((used)) bool __wrap_tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) {
    (void) lun;
    (void) power_condition;
    
    // Track mount/eject status
    if (load_eject) {
        if (start) {
            // Host is mounting the drive
            if (usb_debug_enabled) {
                Serial.printf("USB start_stop_cb: Setting usb_msc_mounted = true (start=%d, load_eject=%d)\n", start, load_eject);
            }
            usb_msc_mounted = true;
            usb_msc_ejected = false;
            if (usb_debug_enabled) {
                Serial.println("USB drive mounted by host");
            }
            
            // Notify FatFSUSB (but don't reenumerate)
            FatFSUSB.plug();
            
            // Prompt user for manual refresh
            promptRefreshConnections();
        } else {
            // Host is requesting to stop/eject the drive
            // Only disable USB MSC mode if this is a true user eject
            // The standard pattern is load_eject=true and start=false for user eject
            if (load_eject && !start) {
                if (usb_debug_enabled) {
                    Serial.printf("USB start_stop_cb: True eject detected (start=%d, load_eject=%d) - disabling USB MSC mode\n", start, load_eject);
                    Serial.println("◆ USB drive ejected by host - disabling USB MSC mode");
                }
                
                // Use the public unplug method for proper cleanup first
                // This handles sector flushing, disk sync, and callbacks
                FatFSUSB.unplug();
                
                // Then disable USB MSC mode completely
                // Note: This will also force disconnect/reconnect to ensure host sees the change
                disableUSBMassStorage();
            } else {
                // Temporary stop during file operations - just flush and sync, keep USB MSC active
                if (usb_debug_enabled) {
                    Serial.printf("USB drive temporary stop (power_condition=%d, start=%d, load_eject=%d) - flushing data\n", 
                                 power_condition, start, load_eject);
                }
                
                // Flush any pending data but don't disable USB MSC mode
                FatFSUSB.unplug();  // This handles flushing and sync
                
                // Keep USB MSC mode active for subsequent operations
                if (usb_debug_enabled) {
                    Serial.printf("USB start_stop_cb: Maintaining usb_msc_mounted = true after temporary stop\n");
                }
                usb_msc_mounted = true;  // Maintain mounted state
                usb_msc_ejected = false;
            }
        }
    }
    
    return true;
}

bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject) __attribute__((alias("__wrap_tud_msc_start_stop_cb")));

__attribute__((used)) int32_t __wrap_tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    (void) lun;

    // Use FatFSUSB to handle read operations directly from flash
    return FatFSUSB.read10(lba, offset, buffer, bufsize);
}

int32_t tud_msc_read10_cb(uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) __attribute__((alias("__wrap_tud_msc_read10_cb")));

__attribute__((used)) int32_t __wrap_tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    (void) lun;

    // Use FatFSUSB to handle write operations directly to flash
    return FatFSUSB.write10(lba, offset, buffer, bufsize);
}

__attribute__((used)) int32_t __wrap_tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) {
    // Use FatFSUSB to handle SCSI commands including start/stop for mount/eject
    return FatFSUSB.handleSCSICommand(lun, scsi_cmd, buffer, bufsize);
}

__attribute__((used)) bool __wrap_tud_msc_is_writable_cb(uint8_t lun) {
    (void) lun;
    return true;  // Allow writes to the virtual disk
}

// Provide regular callback aliases
int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) __attribute__((alias("__wrap_tud_msc_write10_cb")));
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize) __attribute__((alias("__wrap_tud_msc_scsi_cb")));
bool tud_msc_is_writable_cb(uint8_t lun) __attribute__((alias("__wrap_tud_msc_is_writable_cb")));

} // extern "C"

/*/ FatFS + FatFSUSB listFiles example

#include <FatFS.h>
#include <FatFSUSB.h>

volatile bool updated = false;
volatile bool driveConnected = false;
volatile bool inPrinting = false;

// Called by FatFSUSB when the drive is released.  We note this, restart FatFS, and tell the main loop to rescan.
void unplug(uint32_t i) {
  (void) i;
  driveConnected = false;
  updated = true;
  FatFS.begin();
}

// Called by FatFSUSB when the drive is mounted by the PC.  Have to stop FatFS, since the drive data can change, note it, and continue.
void plug(uint32_t i) {
  (void) i;
  driveConnected = true;
  FatFS.end();
}

// Called by FatFSUSB to determine if it is safe to let the PC mount the USB drive.  If we're accessing the FS in any way, have any Files open, etc. then it's not safe to let the PC mount the drive.
bool mountable(uint32_t i) {
  (void) i;
  return !inPrinting;
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(1);
  }
  delay(5000);

  if (!FatFS.begin()) {
    Serial.println("FatFS initialization failed!");
    while (1) {
      delay(1);
    }
  }
  Serial.println("FatFS initialization done.");

  inPrinting = true;
  printDirectory("/", 0);
  inPrinting = false;

  // Set up callbacks
  FatFSUSB.onUnplug(unplug);
  FatFSUSB.onPlug(plug);
  FatFSUSB.driveReady(mountable);
  // Start FatFS USB drive mode
  FatFSUSB.begin();
  Serial.println("FatFSUSB started.");
  Serial.println("Connect drive via USB to upload/erase files and re-display");
}

void loop() {
  if (updated && !driveConnected) {
    inPrinting = true;
    Serial.println("\n\nDisconnected, new file listing:");
    printDirectory("/", 0);
    updated = false;
    inPrinting = false;
  }
}

void printDirectory(String dirName, int numTabs) {
  Dir dir = FatFS.openDir(dirName);

  while (true) {

    if (!dir.next()) {
      // no more files
      break;
    }
    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }
    Serial.print(dir.fileName());
    if (dir.isDirectory()) {
      Serial.println("/");
      printDirectory(dirName + "/" + dir.fileName(), numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.print(dir.fileSize(), DEC);
      time_t cr = dir.fileCreationTime();
      struct tm* tmstruct = localtime(&cr);
      Serial.printf("\t%d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
  }
}
*/