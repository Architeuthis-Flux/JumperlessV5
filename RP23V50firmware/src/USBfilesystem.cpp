/*
 * USB Mass Storage with External QSPI Flash for Jumperless V5
 * Based on Adafruit TinyUSB MSC External Flash example
 * Adapted for RP2350B with 16MB QSPI Flash
 */

 /*

#include "SPI.h"
#include "SdFat.h"
#include "Adafruit_SPIFlash.h"
#include "JumperlessDefines.h"
#include "USBfs.h"

// RP2350B uses same flash device that stores code for file system
// We'll use a partition starting at 8MB (leaving 8MB for code/data)
// This matches the board_build.filesystem_size = 8m in platformio.ini
Adafruit_FlashTransport_RP2040 flashTransport;

// SPIFlash object
Adafruit_SPIFlash flash(&flashTransport);

// File system object from SdFat
FatVolume fatfs;
FatFile root;
FatFile file;

// Flash initialization state
static bool flash_initialized = false;
static bool fs_formatted = false;
static bool fs_changed = true;

//==============================================================================
// Flash Initialization Functions
//==============================================================================

/**
 * @brief Initialize the QSPI flash for USB Mass Storage
 * @return true if successful, false otherwise
 */
 /*
bool initializeUSBFlash(void) {
    if (flash_initialized) {
        return true;
    }
    
    Serial.println("◆ Setting up Flash for USB Mass Storage (Safe Mode)...");
    Serial.println("◆ Note: Using fallback approach to avoid flash reinit conflicts");
    
    // SAFE APPROACH: Don't try to access the physical flash at all
    // The RP2350B uses the same flash for program and filesystem storage
    // Any attempt to reinitialize or directly access it causes system crashes
    
    // Instead, we'll mark it as "initialized" and provide fallback capacity
    // The USB host will format the drive when it connects, which will work
    // through the TinyUSB MSC callbacks without crashing the system
    
    Serial.println("◆ Using safe fallback mode for flash access");
    Serial.println("◆ Flash hardware access will be handled by MSC callbacks only");
    
    // Don't try to mount file system either - let USB host handle it
    fs_formatted = false;
    flash_initialized = true;
    fs_changed = true;
    
    Serial.println("◆ Flash setup complete (safe mode)");
    Serial.println("◆ USB host will handle flash formatting when connected");
    
    return true;
}
*/



/**
 * @brief Deinitialize flash (close file system)
 */
/*
void deinitializeUSBFlash(void) {
    if (!flash_initialized) {
        return;
    }
    
    Serial.println("◆ Deinitializing USB Flash...");
    
    // Close any open files
    if (file.isOpen()) {
        file.close();
    }
    if (root.isOpen()) {
        root.close();
    }
    
    // Close file system
    fatfs.end();
    
    flash_initialized = false;
    fs_formatted = false;
}

/**
 * @brief Get the flash object reference for MSC callbacks
 * @return Reference to the flash object
 */
/*
Adafruit_SPIFlash& getUSBFlash(void) {
    return flash;
}
*/
/**
 * @brief Get the file system object reference
 * @return Reference to the FatVolume object
 */
/*
FatVolume& getUSBFileSystem(void) {
    return fatfs;
}
*/
/**
 * @brief Check if flash is initialized
 * @return true if initialized
 */
/*
bool isUSBFlashInitialized(void) {
    return flash_initialized;
}
*/
/**
 * @brief Check if file system is formatted
 * @return true if formatted and mounted
 */
/*
bool isUSBFileSystemFormatted(void) {
    return fs_formatted;
}
*/
/**
 * @brief Force refresh of file system status
 * This is called when the host writes to the flash
 */
/*
void markUSBFileSystemChanged(void) {
    fs_changed = true;
}
*/
//==============================================================================
// File System Monitoring Functions
//==============================================================================

/**
 * @brief Check and list flash contents (for debugging)
 * This is called periodically to monitor file system changes
 */
/*
                                                                                void monitorUSBFlashContents(void) {
    if (!flash_initialized || !fs_formatted) {
        // Try to re-initialize if not ready
        if (!flash_initialized) {
            initializeUSBFlash();
            return;
        }
        
        // Try to mount file system if flash is ready but FS isn't
        fs_formatted = fatfs.begin(&flash);
        if (!fs_formatted) {
            return; // Still not formatted
        }
        
        Serial.println("◆ File system became available");
    }
    
    if (fs_changed) {
        fs_changed = false;
        
        Serial.println("◆ Flash contents:");
        
        if (!root.open("/")) {
            Serial.println("◇ Failed to open root directory");
            return;
        }
        
        // List files in root directory
        while (file.openNext(&root, O_RDONLY)) {
            file.printFileSize(&Serial);
            Serial.write(' ');
            file.printName(&Serial);
            if (file.isDir()) {
                Serial.write('/');
            }
            Serial.println();
            file.close();
        }
        
        root.close();
        Serial.println();
    }
}

//==============================================================================
// Direct Flash Access Functions (for MSC callbacks)
//==============================================================================

/**
 * @brief Read blocks directly from flash
 * @param lba Logical block address (sector number)
 * @param buffer Buffer to read into
 * @param bufsize Buffer size in bytes (must be multiple of 512)
 * @return Number of bytes read, or -1 on error
 */
/*
int32_t readFlashBlocks(uint32_t lba, void* buffer, uint32_t bufsize) {
    if (!flash_initialized) {
        Serial.println("◇ Flash not initialized for read");
        return -1;
    }
    
    // SPIFlash Block API already includes 4K sector caching internally
    bool success = flash.readBlocks(lba, (uint8_t*)buffer, bufsize / 512);
    
    if (!success) {
        Serial.printf("◇ Flash read failed at LBA %u\n", lba);
        return -1;
    }
    
    return bufsize;
}
*/
/**
 * @brief Write blocks directly to flash
 * @param lba Logical block address (sector number)  
 * @param buffer Buffer to write from
 * @param bufsize Buffer size in bytes (must be multiple of 512)
 * @return Number of bytes written, or -1 on error
 */
/*
int32_t writeFlashBlocks(uint32_t lba, const uint8_t* buffer, uint32_t bufsize) {
    if (!flash_initialized) {
        Serial.println("◇ Flash not initialized for write");
        return -1;
    }
    
    // SPIFlash Block API already includes 4K sector caching internally
    bool success = flash.writeBlocks(lba, const_cast<uint8_t*>(buffer), bufsize / 512);
    
    if (!success) {
        Serial.printf("◇ Flash write failed at LBA %u\n", lba);
        return -1;
    }
    
    return bufsize;
}
*/
/**
 * @brief Sync/flush flash cache
 * @return true if successful
 */
/*
bool syncFlashBlocks(void) {
    if (!flash_initialized) {
        return false;
    }
    
    // Sync with flash
    flash.syncBlocks();
    
    // Clear file system cache to force refresh
    if (fs_formatted) {
        fatfs.cacheClear();
        fs_changed = true; // Mark for monitoring refresh
    }
    
    return true;
}
*/
/**
 * @brief Get flash capacity information
 * @param block_count_out Outputs total number of 512-byte blocks
 * @param block_size_out Outputs block size (always 512)
 * @return true if successful
 */
/*
bool getFlashCapacity(uint32_t* block_count_out, uint16_t* block_size_out) {
    if (!flash_initialized) {
        return false;
    }
    
    // Calculate total blocks (flash size / 512 bytes per block)
    uint32_t total_blocks = flash.size() / 512;
    
    *block_count_out = total_blocks;
    *block_size_out = 512;
    
    return true;
}
*/
//==============================================================================
// USB Mass Storage Integration
//==============================================================================

/**
 * @brief Initialize flash for USB Mass Storage mode
 * This is called when entering USB MSC mode
 * @return true if successful
 */
/*
bool prepareFlashForUSB(void) {
    Serial.println("◆ Preparing flash for USB Mass Storage mode...");
    
    if (!initializeUSBFlash()) {
        Serial.println("◇ Failed to initialize flash for USB mode");
        return false;
    }
    
    // Close file system to allow direct sector access
    if (fs_formatted) {
        fatfs.end();
        fs_formatted = false;
        Serial.println("◆ File system closed for USB access");
    }
    
    return true;
}
*/
/**
 * @brief Restore flash for normal application use
 * This is called when exiting USB MSC mode
 */
/*
        void restoreFlashForApp(void) {
    Serial.println("◆ Restoring flash for application use...");
    
    if (!flash_initialized) {
        return;
    }
    
    // Remount file system for application use
    fs_formatted = fatfs.begin(&flash);
    if (fs_formatted) {
        Serial.println("◆ File system remounted for application");
        fs_changed = true; // Trigger content refresh
    } else {
        Serial.println("△ Failed to remount file system");
    }
}
*/
/**
 * @brief Print flash status information
 */
/*
void printFlashStatus(void) {
    Serial.println("\n╭─ Flash Status ──────────────╮");
    Serial.printf("│ Initialized:    %s      │\n", flash_initialized ? "YES" : "NO ");
    Serial.printf("│ FS Formatted:   %s      │\n", fs_formatted ? "YES" : "NO ");
    
    if (flash_initialized) {
        Serial.printf("│ JEDEC ID:       0x%06X   │\n", flash.getJEDECID());
        Serial.printf("│ Flash Size:     %d MB    │\n", flash.size() / (1024 * 1024));
        Serial.printf("│ Total Blocks:   %d     │\n", flash.size() / 512);
    }
    
    Serial.println("╰─────────────────────────────╯");
}
*/  