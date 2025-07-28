#include "FilesystemStuff.h"
#include "micropythonExamples.h"
#include "Graphics.h"
#include "oled.h"
#include <time.h>
#include "RotaryEncoder.h"
#include "config.h"
#include <cstring>
#include "Menus.h"
#include "Python_Proper.h"
#include "FatFS.h"
#include "FileParsing.h"


// External references
extern class oled oled;

// eKilo editor integration
#include "EkiloEditor.h"

// Global flag to signal return to main menu after editing
static bool returnToMainMenu = false;

// Global file manager instance for output area access
static FileManager* globalFileManager = nullptr;

// Message queue for filesystem messages that won't get overwritten
struct FilesystemMessage {
    String message;
    int color;
    unsigned long timestamp;
};

static const int MAX_FS_MESSAGES = 10;
static FilesystemMessage fsMessages[MAX_FS_MESSAGES];
static int fsMessageCount = 0;
static unsigned long lastMessageDisplayTime = 0;

void addFilesystemMessage(const String& message, int color = 248) {
    // Safety check - don't add messages during early startup
    if (!Serial) {
        return;
    }
    
    // If we have a global file manager instance, use its persistent message area
    if (globalFileManager != nullptr) {
        globalFileManager->addPersistentMessage(message, color);
        return;
    }
    
    // Add to traditional message queue for compatibility (fallback)
    if (fsMessageCount < MAX_FS_MESSAGES) {
        fsMessages[fsMessageCount].message = message;
        fsMessages[fsMessageCount].color = color;
        fsMessages[fsMessageCount].timestamp = millis();
        fsMessageCount++;
    } else {
        // Shift messages up and add new one at end
        for (int i = 0; i < MAX_FS_MESSAGES - 1; i++) {
            fsMessages[i] = fsMessages[i + 1];
        }
        fsMessages[MAX_FS_MESSAGES - 1].message = message;
        fsMessages[MAX_FS_MESSAGES - 1].color = color;
        fsMessages[MAX_FS_MESSAGES - 1].timestamp = millis();
    }
    lastMessageDisplayTime = millis();
}

void clearFilesystemMessages() {
    fsMessageCount = 0;
}

void displayFilesystemMessages() {
    // Only show messages for 10 seconds
    if (millis() - lastMessageDisplayTime > 10000) {
        clearFilesystemMessages();
        return;
    }
    
    if (fsMessageCount > 0) {
        // Display messages at the bottom of screen (line 25+)
        for (int i = 0; i < fsMessageCount && i < 3; i++) {  // Show max 3 messages
            Serial.print("\033["); // Move cursor
            Serial.print(DEFAULT_DISPLAY_LINES + i);
            Serial.print(";1H");
            
            changeTerminalColor(fsMessages[fsMessageCount - 1 - i].color, false);
            Serial.print(fsMessages[fsMessageCount - 1 - i].message);
            
            // Clear rest of line
            Serial.print("\033[K");
            changeTerminalColor(-1, false);
        }
        Serial.flush();
    }
}

FileManager::FileManager() {
    currentPath = "/";
    maxFiles = 100;
    
    // Check available memory before allocating file list
    size_t freeHeap = rp2040.getFreeHeap();
    if (freeHeap < sizeof(FileEntry) * maxFiles + 10240) { // Need array + 10KB overhead
        // Reduce maxFiles if memory is limited
        maxFiles = min(50, (int)((freeHeap - 10240) / sizeof(FileEntry)));
        if (maxFiles < 10) maxFiles = 10; // Minimum usable size
    }
    
    fileList = new FileEntry[maxFiles];
    if (!fileList) {
        // Critical error - fallback to minimal allocation
        maxFiles = 10;
        fileList = new FileEntry[maxFiles];
    }
    fileCount = 0;
    selectedIndex = 0;
    displayOffset = 0;
    maxDisplayLines = ::DEFAULT_DISPLAY_LINES; // Use configurable display lines
    textAreaLines = maxDisplayLines - 7; // Reserve lines for headers, footers, and output area
    
    // Initialize OLED batching
    lastInputTime = 0;
    oledUpdatePending = false;
    
    // Initialize encoder state tracking
    lastEncoderDirectionState = NONE;
    lastEncoderButtonState = IDLE;
    
    // Initialize OLED scrolling
    oledHorizontalOffset = 0;
    oledCursorPosition = 0;
    
    // Initialize REPL mode
    replMode = false;
    shouldExitForREPL = false;
    originalCursorRow = 0;
    originalCursorCol = 0;
    startRow = 0;
    linesUsed = 0;
    lastOpenedFileContent = "";
    
    // Initialize filesystem
    initializeFilesystem();
    
    // Initialize output area (positioned after help lines and persistent messages)
    int fileListStartRow = 6;
    int fileListEndRow = fileListStartRow + textAreaLines - 1;
    int helpStartRow = fileListEndRow + 2; // Leave 1 line gap after file list
    
    // Initialize persistent message area (above output area)
    persistentMessageHeight = 3;
    persistentMessageStartRow = helpStartRow + 4; // Start after help lines (2 lines) + border (1 line) + gap (1 line)
    persistentMessageCount = 0;
    
    // Position output area after persistent messages
    outputAreaStartRow = persistentMessageStartRow + persistentMessageHeight + 2; // Add separator and gap
    outputAreaHeight = 3; // Reduced to fit persistent messages
    outputAreaCurrentRow = 0;
    
    // Set global pointer for message routing
    globalFileManager = this;
    
    // Initialize input blocking
    lastDisplayUpdate = 0;
}

FileManager::~FileManager() {
    delete[] fileList;
    
    // Clear global pointer
    if (globalFileManager == this) {
        globalFileManager = nullptr;
    }
}

void FileManager::initializeFilesystem() {
    //Serial.println("[FS] Initializing filesystem...");
    
    // Try to initialize FatFS (it may already be initialized by main firmware)
    bool fs_ok = true;
    
    // Test if we can access the filesystem by checking basic operations
    if (!FatFS.exists("/")) {
        //Serial.println("[FS] Root directory not accessible, trying to initialize FatFS...");
        
        // Try to begin/mount the filesystem
        if (!FatFS.begin()) {
            //Serial.println("[FS] Failed to mount FatFS filesystem");
            fs_ok = false;
        } else {
            //Serial.println("[FS] FatFS mounted successfully");
        }
    } else {
        //Serial.println("[FS] Root directory accessible");
    }
    
    if (fs_ok) {
        // Don't create fake directories - only work with what actually exists
        //Serial.println("[FS] Using existing filesystem structure");
        
        // Test root directory access one more time using correct Dir API
        Dir testRoot = FatFS.openDir("/");
        bool root_accessible = false;
        int test_count = 0;
        while (testRoot.next() && test_count < 3) {
            test_count++;
            root_accessible = true;
        }
        
        if (root_accessible) {
            //Serial.println("[FS] Filesystem initialization successful - root directory accessible");
        } else {
            //Serial.println("[FS] Warning: Root directory not accessible via Dir API");
            //Serial.println("[FS] Will show filesystem unavailable message");
            currentPath = "[NO_FS]";
        }
    } else {
        //Serial.println("[FS] Filesystem not available - file operations will be limited");
        currentPath = "[NO_FS]";
    }
}

FileType FileManager::getFileType(const String& filename) {
    String lower = filename;
    lower.toLowerCase();
    
    if (lower.endsWith(".py") || lower.endsWith(".pyw") || lower.endsWith(".pyi")) {
        return FILE_TYPE_PYTHON;

    } else if (lower.endsWith(".json")) {
        return FILE_TYPE_JSON;
    } else if (lower.endsWith(".cfg") || lower.endsWith(".conf") || lower.startsWith("config") || filename == "config.txt") {
        return FILE_TYPE_CONFIG;
    } else if (lower.startsWith("nodefileslot") && lower.endsWith(".txt")) {
        return FILE_TYPE_NODEFILES;
    } else if (lower.startsWith("netcolorsslot") && lower.endsWith(".txt")) {
        return FILE_TYPE_COLORS;
    } else if (lower.endsWith(".txt") || lower.endsWith(".md") || lower.endsWith(".readme")) {
        return FILE_TYPE_TEXT;
    }
    return FILE_TYPE_UNKNOWN;
}

String FileManager::getFileIcon(FileType type) {
    switch (type) {
        case FILE_TYPE_DIRECTORY: return "⌘";
        case FILE_TYPE_PYTHON: return "𓆚";
        case FILE_TYPE_TEXT: return "⍺";
        case FILE_TYPE_CONFIG: return "⚙";
        case FILE_TYPE_JSON: return "⟐";
        case FILE_TYPE_NODEFILES: return "☊";
        case FILE_TYPE_COLORS: return "⎃";
        default: return "⍺";
    }
}

String FileManager::formatFileSize(size_t size) {
    if (size < 1024) {
        return String(size) + " B";
    } else if (size < 1024 * 1024) {
        return String(size / 1024) + " KB";
    } else {
        return String(size / (1024 * 1024)) + " MB";
    }
}

String FileManager::formatDateTime(time_t timestamp) {
    if (timestamp == 0) {
        return "Unknown";
    }
    
    // Convert to local time structure
    struct tm* timeinfo = localtime(&timestamp);
    if (!timeinfo) {
        return String((long)timestamp);
    }
    
    // Format as MM/DD/YY HH:MM
    char buffer[32];
    strftime(buffer, sizeof(buffer), "%m/%d/%y %H:%M", timeinfo);
    return String(buffer);
}

void FileManager::updateOLEDStatus() {
    if (!oled.oledConnected) return;
    
    // Get current selection info
    const char* selectedFileName = (fileCount > 0 && selectedIndex < fileCount) 
        ? fileList[selectedIndex].name.c_str() 
        : nullptr;
    
    if (!selectedFileName) {
        // No file selected, just show path
        oled.showFileStatus(currentPath.c_str(), fileCount, nullptr);
        return;
    }
    
    // Build full display text and calculate cursor position
    String pathPart = currentPath;
    String fullText;
    
    // Don't add extra "/" if currentPath is already root "/"
    // Add newline after path separator for better display formatting (except root)
    if (currentPath == "/") {
        fullText = pathPart + String(selectedFileName);
    } else {
        fullText = pathPart + "/\n" + String(selectedFileName);
    }
    
    // Cursor position is at the end of the selected filename
    oledCursorPosition = fullText.length();
    
    // Show the full text with cursor indicator (no horizontal scrolling needed)
    oled.showFileStatusScrolled(fullText.c_str(), fileCount, oledCursorPosition);
}

void FileManager::scheduleOLEDUpdate() {
    unsigned long currentTime = millis();
    
    // If no recent input, update immediately for responsiveness
    if (!oledUpdatePending || (currentTime - lastInputTime) > 200) {
        updateOLEDStatus();
        lastInputTime = currentTime;
        oledUpdatePending = false;
    } else {
        // Batch subsequent updates
        lastInputTime = currentTime;
        oledUpdatePending = true;
    }
}

void FileManager::processOLEDUpdate() {
    // Only update OLED if enough time has passed since last input
    if (oledUpdatePending && (millis() - lastInputTime) >= 50) { // 50ms delay
        updateOLEDStatus();
        oledUpdatePending = false;
    }
}

void FileManager::calculateHorizontalScrolling(const String& fullText, int cursorPos) {
    if (!oled.isConnected()) return;
    
    // Better estimate: small font can fit ~25-26 characters for 128px width
    const int maxVisibleChars = 25;
    
    // Calculate visible cursor position relative to current offset
    int visibleCursorPos = cursorPos - oledHorizontalOffset;
    
    // Scroll right if cursor is too close to right edge
    if (visibleCursorPos >= maxVisibleChars - OLED_SCROLL_MARGIN) {
        oledHorizontalOffset = cursorPos - maxVisibleChars + OLED_SCROLL_MARGIN + 1;
    }
    // Scroll left if cursor is too close to left edge  
    else if (visibleCursorPos < OLED_SCROLL_MARGIN) {
        oledHorizontalOffset = cursorPos - OLED_SCROLL_MARGIN;
    }
    
    // Keep offset within bounds
    if (oledHorizontalOffset < 0) {
        oledHorizontalOffset = 0;
    }
    if (oledHorizontalOffset > fullText.length() - maxVisibleChars) {
        oledHorizontalOffset = max(0, (int)fullText.length() - maxVisibleChars);
    }
}

// Calculate directory depth for visual indentation
int FileManager::calculatePathDepth(const String& path) {
    if (path == "/") return 0;
    
    int depth = 0;
    for (int i = 0; i < path.length(); i++) {
        if (path[i] == '/') depth++;
    }
    return depth - 1; // Subtract 1 because root "/" counts as depth 0
}

bool FileManager::changeDirectory(const String& path) {
    String newPath = path;
    
    // Handle relative paths
    if (!newPath.startsWith("/")) {
        if (currentPath.endsWith("/")) {
            newPath = currentPath + newPath;
        } else {
            newPath = currentPath + "/" + newPath;
        }
    }
    
    // Clean up path (remove double slashes, etc.)
    newPath.replace("//", "/");
    
    // Check if directory exists - handle root directory specially
    bool dirExists = false;
    if (newPath == "/") {
        // For root directory, use openDir instead of exists
        Dir testRoot = FatFS.openDir("/");
        int test_count = 0;
        while (testRoot.next() && test_count < 3) {
            test_count++;
            dirExists = true;
            break;
        }
    } else {
        dirExists = FatFS.exists(newPath.c_str());
    }
    
    if (dirExists) {
        currentPath = newPath;
        selectedIndex = 0;
        displayOffset = 0;
        refreshListing();
        scheduleOLEDUpdate();
        return true;
    }
    
    changeTerminalColor(FileColors::ERROR, false);
    Serial.println("Directory not found: " + newPath);
    changeTerminalColor(-1, false); // Reset colors
    return false;
}

bool FileManager::goUp() {
    if (currentPath == "/") return false;
    
    int lastSlash = currentPath.lastIndexOf('/');
    if (lastSlash == 0) {
        return changeDirectory("/");
    } else {
        return changeDirectory(currentPath.substring(0, lastSlash));
    }
}

bool FileManager::goHome() {
    return changeDirectory("/");
}

void FileManager::refreshListing() {
    static int recursion_depth = 0;
    
    // Prevent infinite recursion
    if (recursion_depth > 3) {
        Serial.println("[FS] Too many recursion attempts, marking filesystem as unavailable");
        currentPath = "[NO_FS]";
        recursion_depth = 0;
        // Fall through to handle [NO_FS] case
    }
    
    fileCount = 0;
    
    // Handle special case where filesystem is not available
    if (currentPath == "[NO_FS]") {
        recursion_depth = 0; // Reset counter
        // Create virtual entries to show filesystem unavailable message
        if (maxFiles > 0) {
            fileList[0].name = "** FILESYSTEM NOT AVAILABLE **";
            fileList[0].path = "[ERROR]";
            fileList[0].isDirectory = false;
            fileList[0].size = 0;
            fileList[0].type = FILE_TYPE_UNKNOWN;
            fileCount = 1;
            
            if (maxFiles > 1) {
                fileList[1].name = "Directory access failed after mount";
                fileList[1].path = "[INFO]";
                fileList[1].isDirectory = false;
                fileList[1].size = 0;
                fileList[1].type = FILE_TYPE_UNKNOWN;
                fileCount = 2;
                
                if (maxFiles > 2) {
                    fileList[2].name = "Press 'q' to quit file manager";
                    fileList[2].path = "[HELP]";
                    fileList[2].isDirectory = false;
                    fileList[2].size = 0;
                    fileList[2].type = FILE_TYPE_UNKNOWN;
                    fileCount = 3;
                }
            }
        }
        return;
    }
    
    // Use FatFS directory API instead of file API
    Dir dir = FatFS.openDir(currentPath);
    
    // Check if directory exists by testing if we can read from it
    bool dir_accessible = false;
    int test_entries = 0;
    
    // Test directory accessibility by trying to read a few entries
    // BUT: Empty directories are still valid and accessible!
    while (dir.next() && test_entries < 5) {
        test_entries++;
        dir_accessible = true;
    }
    
    // If we couldn't read any entries, the directory might be empty OR inaccessible
    // Try to distinguish between empty and actually broken directories
    if (!dir_accessible) {
        // For empty directories, we should still consider them accessible
        // Check if the directory path exists as a directory
        if (FatFS.exists(currentPath.c_str())) {
            // Directory exists, so it's accessible even if empty
            dir_accessible = true;
        } else {
            // Directory actually failed to open - this is a real error
            changeTerminalColor(FileColors::ERROR, false);
            Serial.println("Failed to open directory: " + currentPath);
            changeTerminalColor(-1, false); // Reset colors
            
            recursion_depth++;
            
            // Try to recover by going to root or alternative directory (only if not too deep)
            if (recursion_depth <= 2 && currentPath != "/") {
                Serial.println("[FS] Attempting to return to root directory...");
                currentPath = "/";
                // Try root directory - it should exist
                if (FatFS.exists("/")) {
                    refreshListing(); // Recursive call to try root
                    recursion_depth--; // Decrement on successful path
                    return;
                }
                
                // No alternative directories to try - filesystem issues
            }
            
            // If all fails, mark filesystem as unavailable
            Serial.println("[FS] All recovery attempts failed, filesystem unavailable");
            currentPath = "[NO_FS]";
            recursion_depth = 0; // Reset for next attempt
            refreshListing(); // Show error message
            return;
        }
    }
    
    // Successfully opened directory, reset recursion counter
    recursion_depth = 0;
    
    // Add ".." entry if not at root
    if (currentPath != "/") {
        FileEntry& parentEntry = fileList[fileCount];
        parentEntry.name = "..";
        parentEntry.path = "[UP]"; // Special marker for parent directory
        parentEntry.isDirectory = true;
        parentEntry.size = 0;
        parentEntry.lastModified = 0;
        parentEntry.type = FILE_TYPE_DIRECTORY;
        fileCount++;
    }
    
    // Re-open directory to start from beginning for actual listing
    dir = FatFS.openDir(currentPath);
    
    while (dir.next() && fileCount < maxFiles) {
        String fileName = dir.fileName();
        
        // Skip hidden files (files starting with '.') except for ".." navigation
        if (fileName.startsWith(".") && fileName != "..") {
            continue;
        }
        
        FileEntry& entry = fileList[fileCount];
        entry.name = fileName;
        entry.path = getFullPath(currentPath, entry.name);
        entry.isDirectory = dir.isDirectory();
        entry.size = dir.isDirectory() ? 0 : dir.fileSize();
        entry.lastModified = dir.fileCreationTime(); // Use creation time
        
        if (entry.isDirectory) {
            entry.type = FILE_TYPE_DIRECTORY;
        } else {
            entry.type = getFileType(entry.name);
        }
        
        fileCount++;
    }
    
    // Sort directories first, then files alphabetically
    // But keep ".." at the very top if it exists
    int startSort = 0;
    if (fileCount > 0 && fileList[0].name == "..") {
        startSort = 1; // Skip the ".." entry in sorting
    }
    
    for (int i = startSort; i < fileCount - 1; i++) {
        for (int j = i + 1; j < fileCount; j++) {
            bool shouldSwap = false;
            
            if (fileList[i].isDirectory && !fileList[j].isDirectory) {
                shouldSwap = false; // Keep directory first
            } else if (!fileList[i].isDirectory && fileList[j].isDirectory) {
                shouldSwap = true; // Move directory up
            } else {
                // Same type, sort alphabetically
                shouldSwap = fileList[i].name > fileList[j].name;
            }
            
            if (shouldSwap) {
                FileEntry temp = fileList[i];
                fileList[i] = fileList[j];
                fileList[j] = temp;
            }
        }
    }
}

void FileManager::showCurrentListing(bool showHeader) {
    if (showHeader) {
        changeTerminalColor(FileColors::HEADER, true);
        Serial.println("\n╭───────────────────────────────────────────────────────────────────────────╮");
        Serial.println("│                            JUMPERLESS FILE MANAGER                        │");
        Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
        
        changeTerminalColor(FileColors::STATUS, false);
        Serial.print("⌘ Current Path: ");
        printColoredPath(currentPath);
        Serial.println();
        
        changeTerminalColor(FileColors::STATUS, false);
        Serial.println("Files: " + String(fileCount) + "  |  Use ↑↓ arrows or rotary encoder to navigate");
        Serial.println("Enter/Click: Open/Edit  |  Space: File operations  |  h: Help  |  q: Quit");
        Serial.println("╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌");
    }
    
    // Calculate display range
    int startIdx = displayOffset;
    int endIdx = min(displayOffset + maxDisplayLines, fileCount);
    
    for (int i = startIdx; i < endIdx; i++) {
        bool isSelected = (i == selectedIndex);
        FileEntry& entry = fileList[i];
        
        // Position cursor for this file entry
        moveCursor(6 + (i - startIdx), 3);
        clearCurrentLine();
        
        // Calculate indentation based on directory depth
        int currentDepth = calculatePathDepth(currentPath);
        String indentPrefix = "";
        for (int d = 0; d < currentDepth; d++) {
            indentPrefix += "  "; // 2 spaces per level
        }
        
        // Selection indicator
        if (isSelected) {
            changeTerminalColor(226, false); // Bright yellow background
            Serial.print("► ");
        } else {
            Serial.print("  ");
        }
        
        // Add visual indentation
        Serial.print(indentPrefix);
        
        // File type color and icon
        int color = FileColors::UNKNOWN;
        switch (entry.type) {
            case FILE_TYPE_DIRECTORY: color = FileColors::DIRECTORY; break;
            case FILE_TYPE_PYTHON: color = FileColors::PYTHON; break;
            case FILE_TYPE_TEXT: color = FileColors::TEXT; break;
            case FILE_TYPE_CONFIG: color = FileColors::CONFIG; break;
            case FILE_TYPE_JSON: color = FileColors::JSON; break;
            case FILE_TYPE_NODEFILES: color = FileColors::NODEFILES; break;
            case FILE_TYPE_COLORS: color = FileColors::COLORS; break;
            case FILE_TYPE_UNKNOWN: color = FileColors::TEXT; break;

        }
        
        changeTerminalColor(color, false);
        
        // Special handling for ".." entry
        if (entry.name == ".." && entry.path == "[UP]") {
            Serial.print("⌘ ");
            Serial.print("..");
            
            // Add padding to align with size column (adjusted for indentation)
            int usedSpace = 2 + indentPrefix.length() + 2 + 2; // selector + indent + icon + ".."
            int padding = 50 - usedSpace;
            for (int p = 0; p < padding && p >= 0; p++) Serial.print(" ");
        } else {
            Serial.print(getFileIcon(entry.type) + " ");
            
            // Filename
            String displayName = entry.name;
            int maxNameLength = 45 - (currentDepth * 2); // Adjust for indentation
            if (displayName.length() > maxNameLength) {
                displayName = displayName.substring(0, maxNameLength - 3) + "...";
            }
            Serial.print(displayName);
            
            // Padding for size column (adjusted for indentation)
            int usedSpace = 2 + indentPrefix.length() + 2 + displayName.length(); // selector + indent + icon + name
            int padding = 50 - usedSpace;
            for (int p = 0; p < padding && p >= 0; p++) Serial.print(" ");
        }
        
        changeTerminalColor(248, false); // Light grey for size
        if (entry.name == ".." && entry.path == "[UP]") {
            Serial.print("     <UP>");
        } else if (entry.isDirectory) {
            Serial.print("    <DIR>");
        } else {
            String sizeStr = formatFileSize(entry.size);
            int sizeWidth = 10 - sizeStr.length();
            for (int s = 0; s < sizeWidth && s >= 0; s++) Serial.print(" ");
            Serial.print(sizeStr);
        }
        
        Serial.println();
    }
    
    // Show scroll indicator if needed
    if (fileCount > maxDisplayLines) {
        changeTerminalColor(FileColors::STATUS, true);
        Serial.println("╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌");
        Serial.println("Showing " + String(startIdx + 1) + "-" + String(endIdx) + " of " + String(fileCount) + " files");
    }
    
    changeTerminalColor(-1, false); // Reset colors
}

void FileManager::showFileInfo(const FileEntry& file) {
    changeTerminalColor(FileColors::HEADER, true);
    Serial.println("\n╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                                FILE INFO                                  │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    
    changeTerminalColor(FileColors::STATUS, false);
    Serial.println("Name: " + file.name);
    Serial.println("Path: " + file.path);
    Serial.print("Type: ");
    Serial.println(file.isDirectory ? "Directory" : "File");
    if (!file.isDirectory) {
        Serial.println("Size: " + formatFileSize(file.size));
    }
    if (file.lastModified != 0) {
        Serial.println("Modified: " + formatDateTime(file.lastModified));
    }
    Serial.println("╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌");
    
    changeTerminalColor(-1, false); // Reset colors
    Serial.println("\nPress any key to continue...");
}

void FileManager::showHelp() {
    changeTerminalColor(FileColors::HEADER, true);
    Serial.println("\n╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                          FILE MANAGER HELP                                │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("\n⌘ NAVIGATION:");
    changeTerminalColor(221, false); // Yellow
    Serial.print("  ↑/↓ or encoder  ");
    changeTerminalColor(248, false);
    Serial.println("- Move selection up/down");
    changeTerminalColor(221, false);
    Serial.print("  Enter           ");
    changeTerminalColor(248, false);
    Serial.println("- Open directory or edit file");
    changeTerminalColor(221, false);
    Serial.print("  ..              ");
    changeTerminalColor(248, false);
    Serial.println("- Go up one directory");
    changeTerminalColor(221, false);
    Serial.print("  /               ");
    changeTerminalColor(248, false);
    Serial.println("- Go to root directory");
    
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("\n⍺ FILE OPERATIONS:");
    changeTerminalColor(155, false); // Green
    Serial.print("  v               ");
    changeTerminalColor(248, false);
    Serial.println("- View file contents");
    changeTerminalColor(155, false);
    Serial.print("  e               ");
    changeTerminalColor(248, false);
    Serial.println("- Edit with eKilo editor");
    changeTerminalColor(155, false);
    Serial.print("  n               ");
    changeTerminalColor(248, false);
    Serial.println("- Create new file");
    changeTerminalColor(155, false);
    Serial.print("  d               ");
    changeTerminalColor(248, false);
    Serial.println("- Create new directory");
    changeTerminalColor(155, false);
    Serial.print("  r               ");
    changeTerminalColor(248, false);
    Serial.println("- Rename file/directory");
    changeTerminalColor(155, false);
    Serial.print("  x               ");
    changeTerminalColor(248, false);
    Serial.println("- Delete file/directory");
    
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("\n⟐ OTHER:");
    changeTerminalColor(207, false); // Magenta
    Serial.print("  i               ");
    changeTerminalColor(248, false);
    Serial.println("- Show file info");
    changeTerminalColor(207, false);
    Serial.print("  h               ");
    changeTerminalColor(248, false);
    Serial.println("- Show this help");
    changeTerminalColor(207, false);
    Serial.print("  q               ");
    changeTerminalColor(248, false);
    Serial.println("- Quit file manager");
    changeTerminalColor(207, false);
    Serial.print("  t               ");
    changeTerminalColor(248, false);
    Serial.println("- Format/indent Python files in directory");
    
    changeTerminalColor(FileColors::ERROR, true);
    Serial.println("\n○ FILE TYPE COLORS:");
    changeTerminalColor(FileColors::DIRECTORY, false);
    Serial.print("⌘ Directories  ");
    changeTerminalColor(FileColors::PYTHON, false);
    Serial.print("𓆚 Python  ");
    changeTerminalColor(FileColors::TEXT, false);
    Serial.print("⍺ Text  ");
    changeTerminalColor(FileColors::CONFIG, false);
    Serial.print("⚙ Config  ");
    changeTerminalColor(FileColors::NODEFILES, false);
    Serial.print("☊ NodeFiles  ");
    changeTerminalColor(FileColors::COLORS, false);
    Serial.println("⎃ Colors");
    
    changeTerminalColor(-1, false); // Reset colors
    Serial.println("\nPress any key to continue...");
}

void FileManager::moveSelection(int direction) {
    if (fileCount == 0) return;
    
    selectedIndex += direction;
    
    // Wrap around
    if (selectedIndex < 0) {
        selectedIndex = fileCount - 1;
    } else if (selectedIndex >= fileCount) {
        selectedIndex = 0;
    }
    
    // Reset horizontal scrolling when selection changes
    oledHorizontalOffset = 0;
    oledCursorPosition = 0;
    
    // Adjust display offset if needed (use configurable visible lines)
    if (selectedIndex < displayOffset) {
        displayOffset = selectedIndex;
    } else if (selectedIndex >= displayOffset + textAreaLines) {
        displayOffset = selectedIndex - textAreaLines + 1;
    }
    
    // Update display in place
    updateStatusLine();
    updateFileListDisplay();
    scheduleOLEDUpdate();
}

FileEntry* FileManager::getCurrentFile() {
    if (selectedIndex >= 0 && selectedIndex < fileCount) {
        return &fileList[selectedIndex];
    }
    return nullptr;
}

void FileManager::selectCurrentFile() {
    FileEntry* file = getCurrentFile();
    if (!file) return;
    
    // Handle special ".." entry
    if (file->name == ".." && file->path == "[UP]") {
        if (goUp()) {
            // Successfully went up - redraw the interface
            drawInterface();
        }
        return;
    }
    
    if (file->isDirectory) {
        if (changeDirectory(file->path)) {
            // Successfully changed directory - redraw the interface
            drawInterface();
        }
    } else {
        editFile(file->path);
        refreshListing(); // Refresh in case file was modified
        drawInterface(); // Redraw entire interface after editing
    }
}

bool FileManager::createFile(const String& filename) {
    String fullPath = getFullPath(currentPath, filename);
    
    if (FatFS.exists(fullPath.c_str())) {
        outputToArea("File already exists: " + filename, FileColors::ERROR);
        return false;
    }
    
    File file = FatFS.open(fullPath.c_str(), "w");
    if (file) {
        file.close();
        outputToArea("Created file: " + filename, FileColors::STATUS);
        refreshListing();
        return true;
    } else {
        outputToArea("Failed to create file: " + filename, FileColors::ERROR);
        return false;
    }
}

bool FileManager::deleteFile(const String& filename) {
    String fullPath = getFullPath(currentPath, filename);
    
    if (FatFS.remove(fullPath.c_str())) {
        outputToArea("Deleted: " + filename, FileColors::STATUS);
        refreshListing();
        return true;
    } else {
        outputToArea("Failed to delete: " + filename, FileColors::ERROR);
        return false;
    }
}

bool FileManager::editFile(const String& filename) {
    changeTerminalColor(FileColors::STATUS, false);
    Serial.println("\n\n\rOpening " + filename + " in text editor...");
    changeTerminalColor(-1, false); // Reset colors
    
    return editFileWithEkilo(filename);
}

bool FileManager::editFileWithEkilo(const String& filename) {
    // Check available memory before opening file
    size_t freeHeap = rp2040.getFreeHeap();
    if (freeHeap < 2048) { // Require at least 2KB free for editor
        outputToArea("ERROR: Not enough memory to open editor (" + String(freeHeap / 1024) + "KB free)", FileColors::ERROR);
        return false;
    }
    
    if (replMode || true) {
        // In REPL mode - use launchEkiloREPL and store returned content
        // Serial.println("REPLMODE");
        // delay(1000);
        String content = launchEkiloREPL(filename.c_str());

//Serial.print("return\n\nn\n\n\n\n\n\n\n\n\n\n\n\n\r");
        
        if (content.length() > 0) {
            // Check if this is the special marker indicating REPL was launched
            if (content == "[REPL_LAUNCHED]") {
                // REPL was launched with Ctrl+P but no content was executed
                // Still exit the file manager to return to main menu
                lastOpenedFileContent = "";
               // shouldExitForREPL = true; // Signal to exit file manager
            } else {
                // Check content size before storing
                if (content.length() > 8192) { // Limit to 8KB
                    outputToArea("WARNING: File content too large for REPL mode, truncating", FileColors::ERROR);
                    content = content.substring(0, 8192);
                }
                // User saved new content
                lastOpenedFileContent = content;
               // shouldExitForREPL = true; // Signal to exit file manager
            }
        } else {
            // User didn't save - try to load existing file content if it exists
            File file = FatFS.open(filename.c_str(), "r");
            if (file) {
                size_t fileSize = file.size();
                if (fileSize > 8192) { // Limit file size for REPL mode
                    file.close();
                    outputToArea("WARNING: File too large for REPL mode (" + String(fileSize / 1024) + "KB)", FileColors::ERROR);
                    return false;
                }
                
                String existingContent = file.readString();
                file.close();
                if (existingContent.length() > 0) {
                    lastOpenedFileContent = existingContent;
                   // shouldExitForREPL = true; // Signal to exit file manager
                }
            }
        }
    } else {
        // Serial.println("NORMALMODE");
        // delay(1000);
        // Normal mode - use regular launchEkilo
        launchEkilo(filename.c_str());
    }
    return true;
}

bool FileManager::viewFile(const String& filename) {
    File file = FatFS.open(filename.c_str(), "r");
    if (!file) {
        changeTerminalColor(FileColors::ERROR, false);
        Serial.println("Failed to open file: " + filename);
        changeTerminalColor(-1, false); // Reset colors
        return false;
    }
    
    changeTerminalColor(FileColors::HEADER, true);
    Serial.println("\n╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                              FILE VIEWER                                  │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    Serial.println("File: " + filename);
    Serial.println("╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌");
    
    changeTerminalColor(248, true); // Light grey for content
    
    int lineCount = 0;
    while (file.available() && lineCount < 50) { // Limit to 50 lines for viewing
        String line = file.readStringUntil('\n');
        Serial.println(line);
        lineCount++;
    }
    
    if (file.available()) {
        changeTerminalColor(FileColors::STATUS, false);
        Serial.println("\n... (file continues - use 'e' to edit full file)");
    }
    
    file.close();
    changeTerminalColor(-1, false); // Reset colors
    Serial.println("\nPress any key to continue...");
    
    while (Serial.available() == 0) {
        delayMicroseconds(100); // Use normal delay for blocking wait
    }
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    return true;
}

void FileManager::run() {
    refreshListing();
    bool running = true;
    
    // Initialize interactive mode
    initInteractiveMode();
    
    // Initial display
    drawInterface();
    
    while (running) {
        // In REPL mode, check if we should exit after content is ready
        if (replMode && shouldExitForREPL) {
            // Close any open files before exiting
            closeAllFiles();
            // Clear screen completely before exiting to avoid weird state
            clearScreen();
            running = false;
            break;
        }
        
        // Process any pending OLED updates
        processOLEDUpdate();
        
        // Display any legacy filesystem messages at bottom of screen (fallback only)
        displayFilesystemMessages();
        
        // Note: Removed returnToMainMenu check - editor returns directly to file manager now
        
        // Handle rotary encoder input
       // rotaryEncoderStuff();
        
        // Check for encoder direction changes
        if (encoderDirectionState != lastEncoderDirectionState) {
            if (encoderDirectionState == UP) {
                encoderDirectionState = NONE;
                moveSelection(-1);
                scheduleOLEDUpdate();
                lastInputTime = micros(); // Record input time
            } else if (encoderDirectionState == DOWN) {
                encoderDirectionState = NONE;
                moveSelection(1);
                scheduleOLEDUpdate();
                lastInputTime = micros(); // Record input time
            }
            lastEncoderDirectionState = encoderDirectionState;
        }
        
        // Check for encoder button presses
        if (encoderButtonState != lastEncoderButtonState) {
            if (encoderButtonState == PRESSED && lastEncoderButtonState == IDLE) {
                selectCurrentFile();
                scheduleOLEDUpdate();
                lastInputTime = micros(); // Record input time
                blockInputBriefly(); // Block input briefly after interface changes
                // Note: selectCurrentFile() now handles interface redrawing internally
            }
            lastEncoderButtonState = encoderButtonState;
        }
        
        // Check if input is blocked (except Ctrl+Q)
        char input = 0;
        if (Serial.available()) {
            char c = Serial.peek(); // Look at next character without reading it
            
            // Allow Ctrl+Q through even during input blocking
            if (c == 17 || !isInputBlocked()) { // Ctrl+Q or not blocked
                input = Serial.read();
                lastInputTime = micros(); // Record input time for any serial input
                scheduleOLEDUpdate(); // Schedule OLED update for any input
            } else {
                // Input is blocked, clear buffer except Ctrl+Q
                clearBufferedInput(true);
                delayMicroseconds(100);
                continue;
            }
        } else {
            delayMicroseconds(100);
            continue;
        }
        
        // Handle input
        switch (input) {
            case 'q':
            case 'Q':
            case 17: // Ctrl-Q
                // Close any open files before exiting
                closeAllFiles();
                running = false;
                break;
                
            case 16: // Ctrl-P
                // In file manager, Ctrl-P doesn't make sense (no content to save)
                // Ignore silently to prevent crashes
                outputToArea("Ctrl-P: No action in file manager", FileColors::STATUS);
                break;
                
            case '\r':
            case '\n':
                selectCurrentFile();
                blockInputBriefly(); // Block input briefly after interface changes
                // Note: selectCurrentFile() now handles interface redrawing internally
                break;
                
            case 'h':
            case 'H':
                showInteractiveHelp();
                break;
                
            case 'v':
            case 'V': {
                FileEntry* file = getCurrentFile();
                if (file && !file->isDirectory) {
                    showInteractiveFileView(file->path);
                }
                break;
            }
            
            case 'e':
            case 'E': {
                FileEntry* file = getCurrentFile();
                if (file && !file->isDirectory) {
                    editFile(file->path);
                    refreshListing(); // Refresh in case file was modified
                    drawInterface(); // Redraw entire interface
                    blockInputBriefly(); // Block input after interface refresh
                }
                break;
            }
            
            case 'i':
            case 'I': {
                FileEntry* file = getCurrentFile();
                if (file) {
                    showInteractiveFileInfo(*file);
                }
                break;
            }
            
            case 'n':
            case 'N': {
                String filename = promptForFilename("Enter filename (with extension): ");
                if (filename.length() > 0 && isValidFilename(filename)) {
                    createFile(filename);
                    refreshListing();
                    drawInterface();
                    blockInputBriefly(); // Block input after interface refresh
                }
                break;
            }
            
            case 'd':
            case 'D': {
                String dirname = promptForFilename("Enter directory name: ");
                if (dirname.length() > 0 && isValidFilename(dirname)) {
                    createDirectory(dirname);
                    refreshListing();
                    drawInterface();
                    blockInputBriefly(); // Block input after interface refresh
                }
                break;
            }
            
            case 'x':
            case 'X': {
                FileEntry* file = getCurrentFile();
                if (file && !(file->name == ".." && file->path == "[UP]")) {
                    String confirmPrompt;
                    if (file->isDirectory) {
                        confirmPrompt = "Delete directory '" + file->name + "'? (y/N): ";
                    } else {
                        confirmPrompt = "Delete file '" + file->name + "'? (y/N): ";
                    }
                    
                    // Show prompt in output area
                    if (outputAreaCurrentRow == 0) {
                        showOutputAreaBorder();
                    }
                    
                    // Show the prompt WITHOUT automatic newline
                    moveCursor(outputAreaStartRow + outputAreaCurrentRow, 1);
                    clearCurrentLine();
                    changeTerminalColor(FileColors::ERROR, false);
                    Serial.print(confirmPrompt);
                    changeTerminalColor(-1, false);
                    
                    // Show cursor for input
                    Serial.print("\033[?25h");
                    Serial.flush();
                    
                    // Wait for confirmation and echo the character
                    char confirm = 0;
                    while (confirm == 0) {
                        if (Serial.available()) {
                            confirm = Serial.read();
                            // Echo the character immediately
                            Serial.print(confirm);
                            Serial.flush();
                        } else {
                            delayMicroseconds(100);
                        }
                    }
                    
                    Serial.print("\n\r");
                    Serial.print("\033[?25l"); // Hide cursor
                    Serial.flush();
                    
                    outputAreaCurrentRow++; // Move to next line
                    
                    if (confirm == 'y' || confirm == 'Y') {
                        outputToArea("Deleting...", FileColors::STATUS);
                        deleteFile(file->name);
                        refreshListing();
                        drawInterface();
                        blockInputBriefly(); // Block input after interface refresh
                    } else {
                        outputToArea("Delete cancelled", FileColors::STATUS);
                    }
                    
                }
                break;
            }
            
            case 27: { // Escape sequence (arrow keys)
                if (Serial.available()) {
                    char seq1 = Serial.read();
                    if (seq1 == '[' && Serial.available()) {
                        char seq2 = Serial.read();
                        switch (seq2) {
                            case 'A': moveSelection(-1); break; // Up arrow
                            case 'B': moveSelection(1); break;  // Down arrow
                        }
                    }
                }
                break;
            }
            
            case '/':
                if (goHome()) {
                    drawInterface();
                    blockInputBriefly(); // Block input after interface refresh
                }
                break;
                
            case '.':
                if (currentPath != "/") {
                    if (goUp()) {
                        drawInterface();
                        blockInputBriefly(); // Block input after interface refresh
                    }
                }
                break;
                
            case 'r':
            case 'R': {
                // Refresh directory listing
                refreshListing();
                updateStatusLine();
                updateFileListDisplay();
                blockInputBriefly(); // Block input after interface refresh
                break;
            }
            
            case 'u':
            case 'U': {
                // Show memory status
                size_t freeHeap = rp2040.getFreeHeap();
                outputToArea("Memory: " + String(freeHeap / 1024) + "KB free, " + 
                           String(maxFiles) + " max files", FileColors::STATUS);
                break;
            }
            
            case 'm':
            case 'M': {
                // Manual MicroPython examples initialization
                clearFilesystemMessages(); // Clear old messages first
                addFilesystemMessage("Manual initialization requested...", 155);
                initializeMicroPythonExamples(true); // Force initialization
                refreshListing();
                drawInterface();
                blockInputBriefly(); // Block input after interface refresh
                break;
            }
            
            default:
                // Handle unrecognized characters (including other control chars)
                if (input >= 1 && input <= 31 && input != 17 && input != 16 && input != 27) {
                    // Control character that we don't handle - show debug info
                    outputToArea("Unhandled control char: Ctrl+" + String(char('A' + input - 1)), FileColors::STATUS);
                }
                // For regular printable characters, just ignore silently
                break;

        }
    }
    
    // Clean up interactive mode
    clearScreen();
    showCursor(); // Ensure cursor is visible when exiting
    
    // Restore normal font if we were using small fonts
    if (oled.oledConnected) {
        oled.restoreNormalFont();
    }
    
    // Only exit interactive mode if we're NOT returning to main menu after REPL launch
    if (!shouldExitForREPL) {
        // Exit Jumperless interactive mode (normal file manager exit)
        Serial.write(0x0F);
        Serial.flush();
        delay(100); // Give system time to switch modes
    }
    // If shouldExitForREPL is true, we're returning to main menu and want to keep interactive mode on
    
    // Close any open files before exiting file manager
    closeAllFiles();
    
    changeTerminalColor(FileColors::STATUS, false);
    Serial.println("Exiting File Manager...");
    changeTerminalColor(-1, false); // Reset colors
    
    // Print main menu when returning from editor
    //printMainMenu(0);
}

// Interactive mode implementation
void FileManager::initInteractiveMode() {
    // Enter Jumperless interactive mode
    Serial.write(0x0E);
    Serial.flush();
    delay(100); // Give system time to switch modes
    
    // Show cursor by default - only hide during drawing
    showCursor();
}

void FileManager::clearScreen() {
    Serial.print("\033[2J");    // Clear entire screen
    Serial.print("\033[H");     // Move cursor to home (1,1)
    Serial.flush();
}

void FileManager::moveCursor(int row, int col) {
    Serial.print("\033[");
    Serial.print(row);
    Serial.print(";");
    Serial.print(col);
    Serial.print("H");
    Serial.flush();
}

void FileManager::hideCursor() {
    Serial.print("\033[?25l");
    Serial.flush();
}

void FileManager::showCursor() {
    Serial.print("\033[?25h");
    Serial.flush();
}

void FileManager::clearCurrentLine() {
    Serial.print("\033[2K");    // Clear entire line
    Serial.print("\r");         // Return to beginning of line
    Serial.flush();
}

void FileManager::drawInterface(bool fullScreen) {
    // Always use full interface - no special REPL mode anymore
    // Full screen mode (original behavior)
    
    // Hide cursor during drawing for clean interface
    hideCursor();
    
    // Clear output area when redrawing interface
    clearOutputArea();
    
    // Position for header
    if (fullScreen) {
        moveCursor(1, 1);
    }
    // Draw header
    changeTerminalColor(FileColors::HEADER, false);
    Serial.println("╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                            JUMPERLESS FILE MANAGER                        │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    changeTerminalColor(-1, false); // Reset colors
    
    // Draw status line
    updateStatusLine();
    
    // Draw file listing
    updateFileListDisplay();
    
    // Draw comprehensive help lines at bottom (positioned after file list area)
    int fileListStartRow = 6;
    int fileListEndRow = fileListStartRow + textAreaLines - 1;
    int helpStartRow = fileListEndRow + 2; // Leave 1 line gap after file list
    if (fullScreen) {
        moveCursor(helpStartRow, 1);
    }
    changeTerminalColor(125, false); 
    Serial.print(" [enter] = open   │ h = help │ v = quick view │ ↑↓/wheel = nav │ . = up dir   |");
    if (fullScreen) {
        moveCursor(helpStartRow + 1, 1);
    }
    changeTerminalColor(89, false); 
    Serial.print(" CTRL + q = quit  │ e = edit │ n = new file   │ d = new dir    │ u = memory   |");
     
    changeTerminalColor(-1, false); // Reset colors
    
    // Show output area border
    showOutputAreaBorder();
    
    // Initialize and display persistent filesystem messages
    initializePersistentMessageArea();
    displayPersistentMessages();
    
    Serial.flush();
    
    // Show cursor after drawing is complete
    showCursor();
}

void FileManager::updateStatusLine() {
    hideCursor(); // Hide cursor during status update
    moveCursor(4, 1);
    clearCurrentLine();
    
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("⌘ Current Path: ");
    changeTerminalColor(FileColors::DIRECTORY, false);
    
    // Truncate path if too long
    String displayPath = currentPath;
    if (displayPath.length() > 50) {
        displayPath = "..." + displayPath.substring(displayPath.length() - 47);
    }
    Serial.print(displayPath);
    
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("  |  Files: ");
    Serial.print(fileCount);
    if (fileCount > 0) {
        Serial.print("  |  Selected: ");
        Serial.print(selectedIndex + 1);
        Serial.print("/");
        Serial.print(fileCount);
    }
    
    Serial.print("\x1b[0m"); // Reset colors
    Serial.flush();
    
    showCursor(); // Show cursor after status update
}

void FileManager::updateFileListDisplay() {
    // Always use full interface - no special REPL mode anymore
    // Temporarily hide cursor during update for clean display
    hideCursor();
    
    // Full screen mode (original behavior)
    // Clear file listing area (dynamically sized)
    int fileListStartRow = 6;
    int fileListEndRow = fileListStartRow + textAreaLines - 1;
    for (int i = fileListStartRow; i <= fileListEndRow; i++) {
        moveCursor(i, 1);
        clearCurrentLine();
    }
        
        // Handle special case where filesystem is not available
        if (currentPath == "[NO_FS]") {
            moveCursor(8, 5);
            changeTerminalColor(FileColors::ERROR, false);
            Serial.print("** FILESYSTEM NOT AVAILABLE **");
            
            moveCursor(10, 5);
            changeTerminalColor(FileColors::STATUS, false);
            Serial.print("Directory access failed after mount");
            
            moveCursor(12, 5);
            Serial.print("Press 'q' to quit file manager");
            
            changeTerminalColor(0, false);
            Serial.flush();
            delayMicroseconds(100); // Short delay to show error
            return;
        }
        
        // Calculate display range
        int startIdx = displayOffset;
        int endIdx = min(displayOffset + maxDisplayLines, fileCount);
        endIdx = min(endIdx, startIdx + textAreaLines); // Use configurable lines
        
        for (int i = startIdx; i < endIdx; i++) {
            bool isSelected = (i == selectedIndex);
            FileEntry& entry = fileList[i];
            
            // Position cursor for this file entry
            moveCursor(fileListStartRow + (i - startIdx), 3);
            clearCurrentLine();
            
            // Calculate indentation based on directory depth
            int currentDepth = calculatePathDepth(currentPath);
            String indentPrefix = "";
            for (int d = 0; d < currentDepth; d++) {
                indentPrefix += "  "; // 2 spaces per level
            }
            
            // Selection indicator
            if (isSelected) {
                changeTerminalColor(226, false); // Bright yellow background
                Serial.print("► ");
            } else {
                Serial.print("  ");
            }
            
            // Add visual indentation
            Serial.print(indentPrefix);
            
            // File type color and icon
            int color = FileColors::UNKNOWN;
            switch (entry.type) {
                case FILE_TYPE_DIRECTORY: color = FileColors::DIRECTORY; break;
                case FILE_TYPE_PYTHON: color = FileColors::PYTHON; break;
                case FILE_TYPE_TEXT: color = FileColors::TEXT; break;
                case FILE_TYPE_CONFIG: color = FileColors::CONFIG; break;
                case FILE_TYPE_JSON: color = FileColors::JSON; break;
                case FILE_TYPE_NODEFILES: color = FileColors::NODEFILES; break;
                case FILE_TYPE_COLORS: color = FileColors::COLORS; break;
                case FILE_TYPE_UNKNOWN: color = FileColors::TEXT; break;
            }
            
            changeTerminalColor(color, false);
            
            // Special handling for ".." entry
            if (entry.name == ".." && entry.path == "[UP]") {
                Serial.print("⌘ ");
                Serial.print("..");
                
                // Add padding to align with size column (adjusted for indentation)
                int usedSpace = 2 + indentPrefix.length() + 2 + 2; // selector + indent + icon + ".."
                int padding = 50 - usedSpace;
                for (int p = 0; p < padding && p >= 0; p++) Serial.print(" ");
            } else {
                Serial.print(getFileIcon(entry.type) + " ");
                
                // Filename
                String displayName = entry.name;
                int maxNameLength = 45 - (currentDepth * 2); // Adjust for indentation
                if (displayName.length() > maxNameLength) {
                    displayName = displayName.substring(0, maxNameLength - 3) + "...";
                }
                Serial.print(displayName);
                
                // Padding for size column (adjusted for indentation)
                int usedSpace = 2 + indentPrefix.length() + 2 + displayName.length(); // selector + indent + icon + name
                int padding = 50 - usedSpace;
                for (int p = 0; p < padding && p >= 0; p++) Serial.print(" ");
            }
            
            changeTerminalColor(248, false); // Light grey for size
            if (entry.name == ".." && entry.path == "[UP]") {
                Serial.print("     <UP>");
            } else if (entry.isDirectory) {
                Serial.print("    <DIR>");
            } else {
                String sizeStr = formatFileSize(entry.size);
                int sizeWidth = 10 - sizeStr.length();
                for (int s = 0; s < sizeWidth && s >= 0; s++) Serial.print(" ");
                Serial.print(sizeStr);
            }
            
            changeTerminalColor(0, false);
        }
        
        // Show scroll indicator if needed
        if (fileCount > textAreaLines) {
            moveCursor(fileListEndRow + 1, 3);
            changeTerminalColor(FileColors::STATUS, false);
            Serial.print("Showing ");
            Serial.print(startIdx + 1);
            Serial.print("-");
            Serial.print(min(endIdx, fileCount));
            Serial.print(" of ");
            Serial.print(fileCount);
            Serial.print(" files");
            changeTerminalColor(0, false);
        }
        
        Serial.flush();
        
        // Show cursor after update is complete
        showCursor();
}

void FileManager::showInteractiveHelp() {
    // Save current screen
    clearScreen();
    
    // Draw help screen
    moveCursor(1, 1);
    changeTerminalColor(FileColors::HEADER, false);
    Serial.println("╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                          FILE MANAGER HELP                                │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    
    moveCursor(4, 3);
    changeTerminalColor(FileColors::STATUS, false);
            Serial.println("⌘ NAVIGATION:");
    moveCursor(5, 5);
    changeTerminalColor(221, false); // Yellow
    Serial.print("↑/↓ or encoder  ");
    changeTerminalColor(248, false);
    Serial.println("- Move selection up/down");
    moveCursor(6, 5);
    changeTerminalColor(221, false);
    Serial.print("Enter/Click     ");
    changeTerminalColor(248, false);
    Serial.println("- Open directory or edit file");
    moveCursor(7, 5);
    changeTerminalColor(221, false);
    Serial.print("..              ");
    changeTerminalColor(248, false);
    Serial.println("- Go up one directory");
    moveCursor(8, 5);
    changeTerminalColor(221, false);
    Serial.print("/               ");
    changeTerminalColor(248, false);
    Serial.println("- Go to root directory");
    
    moveCursor(10, 3);
    changeTerminalColor(FileColors::STATUS, false);
            Serial.println("⍺ FILE OPERATIONS:");
    moveCursor(11, 5);
    changeTerminalColor(155, false); // Green
    Serial.print("v               ");
    changeTerminalColor(248, false);
    Serial.println("- View file contents");
    moveCursor(12, 5);
    changeTerminalColor(155, false);
    Serial.print("e               ");
    changeTerminalColor(248, false);
    Serial.println("- Edit with eKilo editor");
    moveCursor(13, 5);
    changeTerminalColor(155, false);
    Serial.print("i               ");
    changeTerminalColor(248, false);
    Serial.println("- Show file information");
    
    moveCursor(15, 3);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.println("⚙ CREATE/DELETE:");
    moveCursor(16, 5);
    changeTerminalColor(155, false); // Green
    Serial.print("n               ");
    changeTerminalColor(248, false);
    Serial.println("- Create new file");
    moveCursor(17, 5);
    changeTerminalColor(155, false);
    Serial.print("d               ");
    changeTerminalColor(248, false);
    Serial.println("- Create new directory");
    moveCursor(18, 5);
    changeTerminalColor(207, false); // Magenta (red)
    Serial.print("x               ");
    changeTerminalColor(248, false);
    Serial.println("- Delete file/directory");
    
    moveCursor(20, 3);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.println("⟐ OTHER:");
    moveCursor(21, 5);
    changeTerminalColor(207, false); // Magenta
    Serial.print("r               ");
    changeTerminalColor(248, false);
    Serial.println("- Refresh directory listing");
    moveCursor(22, 5);
    changeTerminalColor(207, false);
    Serial.print("u               ");
    changeTerminalColor(248, false);
    Serial.println("- Show memory status");
    moveCursor(23, 5);
    changeTerminalColor(207, false);
    Serial.print("m               ");
    changeTerminalColor(248, false);
    Serial.println("- Force initialize MicroPython examples");
    moveCursor(24, 5);
    changeTerminalColor(207, false);
    Serial.print("h               ");
    changeTerminalColor(248, false);
    Serial.println("- Show this help");
    moveCursor(25, 5);
    changeTerminalColor(207, false);
    Serial.print("CTRL+q          ");
    changeTerminalColor(248, false);
    Serial.println("- Quit file manager");
    
    moveCursor(26, 3);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("Press any key to return...");
    changeTerminalColor(0, false);
    Serial.flush();
    
    // Wait for keypress
    while (Serial.available() == 0) delayMicroseconds(100);
    while (Serial.available() > 0) Serial.read();
    
    // Redraw main interface
    drawInterface();
}

void FileManager::showInteractiveFileView(const String& filename) {
    // Save current screen  
    clearScreen();
    
    File file = FatFS.open(filename.c_str(), "r");
    if (!file) {
        moveCursor(5, 5);
        changeTerminalColor(FileColors::ERROR, false);
        Serial.print("Failed to open file: " + filename);
        changeTerminalColor(0, false);
        Serial.flush();
        delayMicroseconds(2000);
        drawInterface();
        return;
    }
    
    // Draw header
    moveCursor(1, 1);
    changeTerminalColor(FileColors::HEADER, false);
    Serial.println("╭───────────────────────────────────────────────────────────────────────────╮");
    Serial.println("│                              FILE VIEWER                                  │");
    Serial.println("╰───────────────────────────────────────────────────────────────────────────╯");
    
    moveCursor(4, 3);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("File: ");
    Serial.println(filename);
    
    // Show file content (first 15 lines)
    changeTerminalColor(248, false); // Light grey for content
    int lineCount = 0;
    int row = 6;
    while (file.available() && lineCount < 15 && row < 21) {
        String line = file.readStringUntil('\n');
        moveCursor(row, 3);
        Serial.print(line.substring(0, 75)); // Truncate long lines
        row++;
        lineCount++;
    }
    
    if (file.available()) {
        moveCursor(21, 3);
        changeTerminalColor(FileColors::STATUS, false);
        Serial.print("... (file continues - press 'e' to edit full file)");
    }
    
    file.close();
    
    moveCursor(23, 3);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("Press any key to return...");
    changeTerminalColor(0, false);
    Serial.flush();
    
    // Wait for keypress
    while (Serial.available() == 0) delayMicroseconds(100);
    while (Serial.available() > 0) Serial.read();
    
    // Redraw main interface
    drawInterface();
}

void FileManager::showInteractiveFileInfo(const FileEntry& file) {
    // Use a small info popup instead of full screen
    moveCursor(18, 5);
    clearCurrentLine();
    changeTerminalColor(FileColors::HEADER, false);
    Serial.print("Info: ");
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print(file.name);
    Serial.print(" | Size: ");
    if (file.isDirectory) {
        Serial.print("<DIR>");
    } else {
        Serial.print(formatFileSize(file.size));
    }
    if (file.lastModified != 0) {
        Serial.print(" | Modified: ");
        Serial.print(formatDateTime(file.lastModified));
    }
    changeTerminalColor(0, false);
    Serial.flush();
    
    // Also show on OLED if connected
    if (oled.oledConnected) {
        String infoText = file.name;
        if (file.isDirectory) {
            infoText += " <DIR>";
        } else {
            infoText += " " + formatFileSize(file.size);
        }
        oled.showMultiLineSmallText(infoText.c_str(), true);
        oled.restoreNormalFont();
    }
    
    // Clear info after 3 seconds
    delay(100); // Shortened from 3000ms
    moveCursor(18, 5);
    clearCurrentLine();
    Serial.flush();
}

// Utility functions
String getFullPath(const String& basePath, const String& filename) {
    if (basePath.endsWith("/")) {
        return basePath + filename;
    } else {
        return basePath + "/" + filename;
    }
}

bool isValidFilename(const String& filename) {
    if (filename.length() == 0) return false;
    
    // Check for invalid characters
    String invalidChars = "<>:\"|?*";
    for (int i = 0; i < invalidChars.length(); i++) {
        if (filename.indexOf(invalidChars[i]) >= 0) {
            return false;
        }
    }
    
    return true;
}

void printColoredPath(const String& path) {
    String segments = path;
    segments.replace("/", " > ");
    
    changeTerminalColor(FileColors::DIRECTORY, false);
    Serial.print(segments);
    changeTerminalColor(0, false);
}

// App entry points
void filesystemApp() {

    bool showOledInTerminal = jumperlessConfig.top_oled.show_in_terminal;
    jumperlessConfig.top_oled.show_in_terminal = false;
    
    
    changeTerminalColor(FileColors::HEADER, true);
    Serial.println("\n  Starting Jumperless File Manager...");
    Serial.println("   Navigate files and directories with colorful interface");
    Serial.println("   Create, edit, and manage files using eKilo editor\n\n\r");
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("   Press Enter to launch File Manager...");
    changeTerminalColor(8, true);
    FileManager manager;
    manager.initInteractiveMode();

        

    
    // Wait for user to press enter to break the input loop
    while (Serial.available() == 0) {
        delayMicroseconds(100);
    }
    // Clear the enter keypress
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Save current screen state and switch to alternate screen buffer
    saveScreenState(&Serial);
    
    manager.clearScreen();
    manager.hideCursor();
    
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("   Initializing filesystem...");
    changeTerminalColor(8, true);
    delayMicroseconds(100); // Give time for initialization messages to be seen
    
    // Automatically create MicroPython examples if needed
    initializeMicroPythonExamples();
    
    // Check if filesystem initialization was successful
    if (manager.getCurrentPath() == "[NO_FS]") {
        changeTerminalColor(FileColors::ERROR, true);
        Serial.println();
        Serial.println("   FILESYSTEM INITIALIZATION FAILED"); 
        changeTerminalColor(FileColors::STATUS, false);
        Serial.println("   The file manager cannot access the FatFS filesystem.");
        Serial.println("   This may be because:");
        Serial.println("   • FatFS is not properly initialized in the main firmware");
        Serial.println("   • No filesystem has been formatted on the flash storage");
        Serial.println("   • There is a hardware issue with the flash memory");
        Serial.println();
        Serial.println("   You can still use the file manager interface, but");
        Serial.println("   file operations will not be available until the");
        Serial.println("   filesystem is properly set up.");
        Serial.println();
        changeTerminalColor(FileColors::STATUS, true);
        Serial.print("   Press 'q' to quit or any other key to continue...");
        changeTerminalColor(0, false);
        
        while (Serial.available() == 0) {
            delayMicroseconds(10);
        }
        char c = Serial.read();
        Serial.println();
        
        if (c == 'q' || c == 'Q') {
            Serial.println("  File Manager cancelled.");
            return;
        }
    }
    
    manager.run();

    // Close any open files before exiting file manager
    closeAllFiles();

    // Restore original screen state with all scrollback intact
    restoreScreenState(&Serial);

    jumperlessConfig.top_oled.show_in_terminal = showOledInTerminal;
}

void eKiloApp() {
    changeTerminalColor(FileColors::HEADER, true);
    Serial.println("\n  eKilo Text Editor");
    Serial.println("   Full-featured terminal text editor with syntax highlighting");
    Serial.println("   Perfect for editing MicroPython scripts and config files");
    changeTerminalColor(FileColors::STATUS, true);
    Serial.println("\n   Press Enter to launch eKilo Editor...");
    changeTerminalColor(-1, true);
    
    // Wait for user to press enter to break the input loop
    while (Serial.available() == 0) {
        delayMicroseconds(100);
    }
    // Clear the enter keypress
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Save current screen state and switch to alternate screen buffer
    saveScreenState(&Serial);
    
    launchEkilo(nullptr);
    
    // Restore original screen state with all scrollback intact
    restoreScreenState(&Serial);
    
    // Windows-specific: Add extra delay to ensure proper display
    Serial.flush();
    delay(100);
}

void launchEkilo(const char* filename) {
    // Store if we're called from file manager or standalone
    static bool calledFromFileManager = false;
    calledFromFileManager = (filename != nullptr);
    
    changeTerminalColor(FileColors::HEADER, false);
    if (filename) {
        Serial.println("\n\n\r⍺ Opening " + String(filename) + " in eKilo editor...");
    } else {
        Serial.println("\n\n\r⍺ Starting eKilo editor...");
    }
    Serial.print("\x1b[0m"); // Reset colors instead of setting to black
    
    // Flush any pending serial data before launching editor
    Serial.flush();
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Brief pause to let user see the message
    delayMicroseconds(100);
    
    // Clear screen and prepare for full-screen editor
    Serial.print("\x1b[2J\x1b[H");
    
    // Launch eKilo editor
    int result = ekilo_main(filename);
    
    // Close any files that might have been opened by the editor
    closeAllFiles();
    
    // Check if Ctrl+P was pressed (save and launch REPL)
    if (result == 2) {
        // Clear screen and launch MicroPython REPL
        Serial.print("\x1b[2J\x1b[H");
        changeTerminalColor(FileColors::STATUS, false);
        Serial.println("☺ File saved successfully");
        Serial.println("🐍 Launching MicroPython REPL...");
        changeTerminalColor(-1, false); // Reset colors
        
        // Brief pause to let user see the message
        delay(500);
        
        // Launch MicroPython REPL
        enterMicroPythonREPL(&Serial);
        
        // After REPL exits, handle return appropriately
        if (calledFromFileManager) {
            // If called from file manager, just return (file manager will handle display)
            return;
        } else {
            // If called standalone, return to main menu
            Serial.print("\x1b[2J\x1b[H");
            return;
        }
    }
    
    // Editor has exited normally - clean up and prepare for return
    // Clear screen immediately to prepare for next interface
    Serial.print("\x1b[2J\x1b[H");
    
    // Brief status message
    changeTerminalColor(FileColors::STATUS, false);
    if (result == 0) {
        Serial.println("☺ Editor session completed successfully");
        if (calledFromFileManager) {
            Serial.println("⌘ Returning to file manager...");
        } else {
            Serial.println("⌘ Returning to main menu...");
        }
    } else {
        Serial.println("☹ Editor session ended with error");
        if (calledFromFileManager) {
            Serial.println("⌘ Returning to file manager...");
        } else {
            Serial.println("⌘ Returning to main menu...");
        }
    }
    changeTerminalColor(-1, false); // Reset colors
    
    // Different handling based on how we were called
    if (calledFromFileManager) {
        // Called from file manager - just set flag, don't exit to main menu
        delayMicroseconds(100); // Give user time to see the message
    } else {
        // Called standalone (from Apps menu) - wait for user input
        Serial.println("Press any key to continue...");
        while (Serial.available() == 0) {
            delayMicroseconds(100);
        }
        while (Serial.available() > 0) {
            Serial.read();
        }
    }
    
    // Clear screen one more time to ensure clean return
    Serial.print("\x1b[2J\x1b[H");
}

// REPL mode version of eKilo - returns content if file was saved
String launchEkiloREPL(const char* filename) {
    String finalFilename = "";
    
    if (filename) {
        finalFilename = String(filename);
        changeTerminalColor(FileColors::HEADER, false);
        Serial.println("Opening " + finalFilename + " in eKilo editor...");
    } else {
        // Auto-generate filename using the same logic as ScriptHistory
        finalFilename = generateNextScriptName();
        changeTerminalColor(FileColors::HEADER, false);
        Serial.println("Creating " + finalFilename + " in eKilo editor...");
    }
    Serial.print("\x1b[0m"); // Reset colors
    
    // Save current screen state and switch to alternate screen buffer
    saveScreenState(&Serial);
    
    // Launch eKilo editor in REPL mode
    String savedContent = ekilo_main_repl(finalFilename.c_str());
    
    // Close any files that might have been opened by the editor
    closeAllFiles();
    
    // Restore original screen state with all scrollback intact
    restoreScreenState(&Serial);
    
    // Windows-specific: Add extra delay and clear to ensure proper display
    Serial.flush();
    delay(100);
    
    // Check if Ctrl+P was pressed (launch REPL directly)
    if (savedContent.startsWith("[LAUNCH_REPL]")) {
        // Remove the prefix and get the actual content
        String actualContent = savedContent.substring(13); // Remove "[LAUNCH_REPL]" prefix
        
        changeTerminalColor(FileColors::STATUS, false);
        Serial.println("☺ File saved as " + finalFilename);
        Serial.println("🐍 Launching MicroPython REPL with script file...");
        changeTerminalColor(-1, false); // Reset colors
        
        // Brief pause to let user see the message
       // delay(500);
        
        // Clear screen and launch MicroPython REPL with file
        Serial.print("\x1b[2J\x1b[H");
        enterMicroPythonREPLWithFile(&Serial, finalFilename);
       // Serial.print("return\n\nn\n\n\n\n\n\n\n\n\n\n\n\n\r");
        //Serial.write(0x0F); // turn off interactive mode
        //Serial.flush();
        
        // Restore interactive mode after REPL
        Serial.write(0x0E); // turn on interactive mode
        Serial.print("\x1b[2J\x1b[H");
        Serial.flush();
        
        // When REPL was launched with Ctrl+P, always return content to signal file manager to exit
        // Even if actualContent is empty, return a special marker so file manager knows to quit
        if (actualContent.length() > 0) {
            return actualContent;
        } else {
            return "[REPL_LAUNCHED]"; // Special marker to signal file manager should exit
        }
    }
    
    changeTerminalColor(FileColors::STATUS, false);
    if (savedContent.length() > 0) {
        Serial.println("☺ File saved as " + finalFilename);
    } else {
        Serial.println("☺ Editor session completed");
    }
    changeTerminalColor(-1, false); // Reset colors
    
    // Restore interactive mode if using Serial
    Serial.write(0x0E); // turn on interactive mode
    Serial.print("\x1b[2J\x1b[H");
    Serial.flush();
    
    return savedContent;
}

// Helper function to generate next script name
String generateNextScriptName() {
    String scripts_dir = "/python_scripts";
    
    // Create python_scripts directory if it doesn't exist
    if (!FatFS.exists(scripts_dir)) {
        FatFS.mkdir(scripts_dir);
    }
    
    // Find next available script number
    int next_script_number = 1;
    for (int i = 1; i <= 100; i++) {
        String test_script = scripts_dir + "/script_" + String(i) + ".py";
        if (FatFS.exists(test_script)) {
            next_script_number = i + 1;
        } else {
            break; // Found first gap, use it
        }
    }
    
    return scripts_dir + "/script_" + String(next_script_number) + ".py";
}


bool FileManager::createDirectory(const String& dirname) {
    String fullPath = getFullPath(currentPath, dirname);
    
    if (FatFS.exists(fullPath.c_str())) {
        outputToArea("Directory already exists: " + dirname, FileColors::ERROR);
        return false;
    }
    
    if (FatFS.mkdir(fullPath.c_str())) {
        outputToArea("Created directory: " + dirname, FileColors::STATUS);
        refreshListing();
        return true;
    } else {
        outputToArea("Failed to create directory: " + dirname, FileColors::ERROR);
        return false;
    }
}

String FileManager::promptForFilename(const String& prompt) {
    return promptInOutputArea(prompt);
}

// void filesystemAppPythonScripts() {
//     bool showOledInTerminal = jumperlessConfig.top_oled.show_in_terminal;
//     jumperlessConfig.top_oled.show_in_terminal = false;
    
//     changeTerminalColor(FileColors::HEADER, true);
//     Serial.println("\n\r  Starting Jumperless File Manager (Python Scripts)...");
//     Serial.println("   Navigate Python scripts with colorful interface");
//     Serial.println("   Create, edit, and manage Python files using eKilo editor\n\n\r");
//     changeTerminalColor(FileColors::STATUS, true);
//     Serial.println("   Press Enter to launch File Manager...");
//     changeTerminalColor(8, true);
    
//     FileManager manager;
//     manager.initInteractiveMode();
    
//     // Wait for user to press enter to break the input loop
//     while (Serial.available() == 0) {
//         delayMicroseconds(100);
//     }
//     // Clear the enter keypress
//     while (Serial.available() > 0) {
//         Serial.read();
//     }
    
//     // Save current screen state and switch to alternate screen buffer
//     saveScreenState(&Serial);
    
//     //manager.clearScreen();
//     //manager.hideCursor();
    
//     changeTerminalColor(FileColors::STATUS, true);
//     Serial.println("   Initializing filesystem...");
//     changeTerminalColor(8, true);
//     delayMicroseconds(100); // Give time for initialization messages to be seen
    
//     // Automatically create MicroPython examples if needed
//     initializeMicroPythonExamples();
    
//     // Check if filesystem initialization was successful
//     if (manager.getCurrentPath() == "[NO_FS]") {
//         changeTerminalColor(FileColors::ERROR, true);
//         Serial.println();
//         Serial.println("   FILESYSTEM INITIALIZATION FAILED"); 
//         changeTerminalColor(FileColors::STATUS, false);
//         Serial.println("   The file manager cannot access the FatFS filesystem.");
//         Serial.println("   This may be because:");
//         Serial.println("   • FatFS is not properly initialized in the main firmware");
//         Serial.println("   • No filesystem has been formatted on the flash storage");
//         Serial.println("   • There is a hardware issue with the flash memory");
//         Serial.println();
//         Serial.println("   You can still use the file manager interface, but");
//         Serial.println("   file operations will not be available until the");
//         Serial.println("   filesystem is properly set up.");
//         Serial.println();
//         changeTerminalColor(FileColors::STATUS, true);
//         Serial.print("   Press 'q' to quit or any other key to continue...");
//         changeTerminalColor(0, false);
        
//         while (Serial.available() == 0) {
//             delayMicroseconds(10);
//         }
//         char c = Serial.read();
//         Serial.println();
        
//         if (c == 'q' || c == 'Q') {
//             Serial.println("  File Manager cancelled.");
//             jumperlessConfig.top_oled.show_in_terminal = showOledInTerminal;
//             return;
//         }
//     } else {
//         // Try to navigate to python_scripts directory
//         changeTerminalColor(FileColors::STATUS, true);
//         //Serial.println("   Navigating to python_scripts directory...");
//         changeTerminalColor(8, true);
        
//         // Create python_scripts directory if it doesn't exist
//         if (!FatFS.exists("/python_scripts")) {
//             Serial.println("   Creating python_scripts directory...");
//             if (FatFS.mkdir("/python_scripts")) {
//                 changeTerminalColor(FileColors::HEADER, false);
//                 Serial.println("   ☺ Created /python_scripts directory");
//                 changeTerminalColor(8, true);
//             } else {
//                 changeTerminalColor(FileColors::ERROR, false);
//                 Serial.println("   ☹ Failed to create /python_scripts directory");
//                 Serial.println("   Starting in root directory instead...");
//                 changeTerminalColor(8, true);
//             }
//         }
        
//         // Navigate to python_scripts directory
//         if (FatFS.exists("/python_scripts")) {
//             if (manager.changeDirectory("/python_scripts")) {
//                 changeTerminalColor(FileColors::HEADER, false);
//                 //Serial.println("   ☺ Navigated to /python_scripts");
//                 changeTerminalColor(8, true);
//             } else {
//                 changeTerminalColor(FileColors::ERROR, false);
//                 Serial.println("   ☹ Failed to navigate to /python_scripts");
//                 Serial.println("   Starting in root directory instead...");
//                 changeTerminalColor(8, true);
//             }
//         }
//     }
    
//     manager.run();
    
//     // Restore original screen state with all scrollback intact
//     restoreScreenState(&Serial);
    
//     jumperlessConfig.top_oled.show_in_terminal = showOledInTerminal;
// }

// REPL mode version - returns content if file was saved
String filesystemAppPythonScriptsREPL() {
    // Clear screen for file manager interface
    saveScreenState(&Serial);
    
    FileManager manager;
    // Set REPL mode so the manager knows to use launchEkiloREPL
    manager.setREPLMode(true);
    
    // Use normal interactive mode instead of special REPL mode
    manager.initInteractiveMode();
    
    // Create python_scripts directory if it doesn't exist
    if (!FatFS.exists("/python_scripts")) {
        FatFS.mkdir("/python_scripts");
    }
    
    // Automatically create MicroPython examples if needed
    initializeMicroPythonExamples();
    
    // Navigate to python_scripts directory
    if (FatFS.exists("/python_scripts")) {
        manager.changeDirectory("/python_scripts");
    }
    
    manager.run();
    String content = manager.getLastSavedFileContent();
    
    // Close any open files before exiting REPL mode file manager
    closeAllFiles();
    
    // If we exited because REPL was launched, don't restore screen state
    // as the REPL has already modified the display
    if (manager.getShouldExitForREPL()) {
        // Clear screen completely for clean return to main menu
        Serial.print("\x1b[2J\x1b[H");
        Serial.flush();
        // Ensure interactive mode is enabled for main menu
        Serial.write(0x0E);
        Serial.flush();
    } else {
        // Normal exit - restore screen state
        restoreScreenState(&Serial);
    }
    manager.setREPLMode(false);
    
    return content;
}

String FileManager::getLastSavedFileContent() {
    return lastOpenedFileContent;
}

//==============================================================================
// Global Utility Functions for External Use (e.g., USB filesystem)
//==============================================================================

FileType getFileTypeFromFilename(const String& filename) {
    String lower = filename;
    lower.toLowerCase();
    
    if (lower.endsWith(".py") || lower.endsWith(".pyw") || lower.endsWith(".pyi")) {
        return FILE_TYPE_PYTHON;
    } else if (lower.endsWith(".json")) {
        return FILE_TYPE_JSON;
    } else if (lower.endsWith(".cfg") || lower.endsWith(".conf") || lower.startsWith("config") || filename == "config.txt") {
        return FILE_TYPE_CONFIG;
    } else if (lower.startsWith("nodefileslot") && lower.endsWith(".txt")) {
        return FILE_TYPE_NODEFILES;
    } else if (lower.startsWith("netcolorsslot") && lower.endsWith(".txt")) {
        return FILE_TYPE_COLORS;
    } else if (lower.endsWith(".txt") || lower.endsWith(".md") || lower.endsWith(".readme")) {
        return FILE_TYPE_TEXT;
    }
    return FILE_TYPE_UNKNOWN;
}

String getFileIconFromType(FileType type) {
    switch (type) {
        case FILE_TYPE_DIRECTORY: return "⌘";
        case FILE_TYPE_PYTHON: return "𓆚";
        case FILE_TYPE_TEXT: return "⍺";
        case FILE_TYPE_CONFIG: return "⚙";
        case FILE_TYPE_JSON: return "⟐";
        case FILE_TYPE_NODEFILES: return "☊";
        case FILE_TYPE_COLORS: return "⎃";
        default: return "⍺";
    }
}

String formatFileSizeForUSB(size_t size) {
    if (size < 1024) {
        return String(size) + " B";
    } else if (size < 1024 * 1024) {
        return String(size / 1024) + " KB";
    } else {
        return String(size / (1024 * 1024)) + " MB";
    }
}

//==============================================================================
// Display Configuration Functions
//==============================================================================

int getConfiguredDisplayLines() {
    return DEFAULT_DISPLAY_LINES;
}

int getConfiguredEditorLines() {
    // Editor gets slightly fewer lines due to headers and footers
    return DEFAULT_DISPLAY_LINES - 1;
}

//==============================================================================
// Filesystem Utility Functions
//==============================================================================

// Recursively delete all contents of a directory
bool deleteDirectoryContents(const String& path) {
    Dir dir = FatFS.openDir(path);
    bool success = true;
    
    while (dir.next()) {
        String fileName = dir.fileName();
        String fullPath;
        
        // Build full path
        if (path == "/") {
            fullPath = "/" + fileName;
        } else {
            fullPath = path + "/" + fileName;
        }
        
        if (dir.isDirectory()) {
            // Recursively delete subdirectory contents first
            if (!deleteDirectoryContents(fullPath)) {
                success = false;
                Serial.print("Failed to delete contents of directory: ");
                Serial.println(fullPath);
                continue;
            }
            
            // Then delete the empty directory
            if (!FatFS.rmdir(fullPath.c_str())) {
                success = false;
                Serial.print("Failed to remove directory: ");
                Serial.println(fullPath);
            } else {
                Serial.print("Removed directory: ");
                Serial.println(fullPath);
            }
        } else {
            // Delete file
            if (!FatFS.remove(fullPath.c_str())) {
                success = false;
                Serial.print("Failed to delete file: ");
                Serial.println(fullPath);
            } else {
                Serial.print("Deleted file: ");
                Serial.println(fullPath);
            }
        }
        
        // Add small delay to prevent system overload
        delayMicroseconds(100);
        
        // Process any pending serial data to keep system responsive
        if (Serial.available()) {
            while (Serial.available()) {
                Serial.read();
            }
        }
    }
    
    return success;
}



//==============================================================================
// Helper Functions for MicroPython Examples
//==============================================================================

// Simple version for startup - no message queue
bool writeStringToFileSimple(const char* filename, const char* content) {
    if (!filename || !content) {
        return false;
    }
    
    size_t contentLength = strlen(content);
    if (contentLength == 0) {
        return false;
    }
    
    File file = FatFS.open(filename, "w");
    if (!file) {
        return false;
    }
    
    size_t totalBytesWritten = 0;
    const size_t chunkSize = 64; // Write in 512-byte chunks to avoid memory issues
    
    while (totalBytesWritten < contentLength) {
        size_t bytesToWrite = min(chunkSize, contentLength - totalBytesWritten);
        size_t bytesWritten = file.write((const uint8_t*)(content + totalBytesWritten), bytesToWrite);
        
        if (bytesWritten != bytesToWrite) {
            file.close();
            return false;
        }
        
        totalBytesWritten += bytesWritten;
    }
    
    file.close();
    return true;
}

// Interactive version with message queue
bool writeStringToFile(const char* filename, const char* content) {
    if (!filename || !content) {
        addFilesystemMessage("ERROR: Invalid filename or content", 196);
        return false;
    }
    
    size_t contentLength = strlen(content);
    if (contentLength == 0) {
        addFilesystemMessage("ERROR: Empty content for " + String(filename), 196);
        return false;
    }
    
    // Check memory availability and file size limits
    size_t freeHeap = rp2040.getFreeHeap();
    if (freeHeap < contentLength + 2048) { // Need content size + 2KB overhead
        addFilesystemMessage("ERROR: Not enough memory to write file (" + String(contentLength / 1024) + "KB needed)", 196);
        return false;
    }
    
    if (contentLength > 32768) { // Limit individual files to 32KB
        addFilesystemMessage("ERROR: File too large (" + String(contentLength / 1024) + "KB, max 32KB)", 196);
        return false;
    }
    
    // Reduce verbosity - only show messages for errors or final verification
    File file = FatFS.open(filename, "w");
    if (!file) {
        addFilesystemMessage("ERROR: Cannot open " + String(filename) + " for writing", 196);
        return false;
    }
    
    size_t totalBytesWritten = 0;
    const size_t chunkSize = 256; // Reduce chunk size to avoid memory issues
    
    while (totalBytesWritten < contentLength) {
        size_t bytesToWrite = min(chunkSize, contentLength - totalBytesWritten);
        size_t bytesWritten = file.write((const uint8_t*)(content + totalBytesWritten), bytesToWrite);
        
        if (bytesWritten != bytesToWrite) {
            addFilesystemMessage("ERROR: Write failed at byte " + String(totalBytesWritten), 196);
            file.close();
            return false;
        }
        
        totalBytesWritten += bytesWritten;
        
        // Add small delay every few chunks to prevent system overload
        if ((totalBytesWritten % (chunkSize * 4)) == 0) {
            delay(10);
            
            // Process any pending serial data to keep connection alive
            if (Serial.available()) {
                while (Serial.available()) {
                    Serial.read();
                }
            }
        }
    }
    
    file.close();
    
    // Quick verification - don't spend too much time on this
    File verifyFile = FatFS.open(filename, "r");
    if (verifyFile) {
        size_t actualSize = verifyFile.size();
        verifyFile.close();
        if (actualSize != contentLength) {
            addFilesystemMessage("ERROR: Size mismatch " + String(actualSize) + " vs " + String(contentLength), 196);
            return false;
        }
        // Silent verification - only report errors
    } else {
        // Don't fail on verification errors - the file might still be usable
        // Just continue silently
    }
    
    return true;
}

//==============================================================================
// Initialize MicroPython Examples Function - Conditional Compilation
//==============================================================================

void initializeMicroPythonExamples(bool forceInitialization) {
    // Safety check - don't do anything if Serial is not available
    if (!Serial) {
        return;
    }
    
    // Build arrays dynamically based on enabled examples
    struct ExampleInfo {
        const char* path;
        const char* content;
        const char* name;
    };
    
    // Create array of enabled examples
    ExampleInfo examples[] = {
#ifdef INCLUDE_DAC_BASICS
        {"/python_scripts/examples/01_dac_basics.py", DAC_BASICS_PY, "01_dac_basics.py"},
#endif
#ifdef INCLUDE_ADC_BASICS
        {"/python_scripts/examples/02_adc_basics.py", ADC_BASICS_PY, "02_adc_basics.py"},
#endif
#ifdef INCLUDE_GPIO_BASICS
        {"/python_scripts/examples/03_gpio_basics.py", GPIO_BASICS_PY, "03_gpio_basics.py"},
#endif
#ifdef INCLUDE_NODE_CONNECTIONS
        {"/python_scripts/examples/04_node_connections.py", NODE_CONNECTIONS_PY, "04_node_connections.py"},
#endif
#ifdef INCLUDE_README
        {"/python_scripts/examples/README.md", README_MD, "README.md"},
#endif
#ifdef INCLUDE_TEST_RUNNER
        {"/python_scripts/examples/test_examples.py", TEST_RUNNER_PY, "test_examples.py"},
#endif
#ifdef INCLUDE_LED_BRIGHTNESS_CONTROL
        {"/python_scripts/examples/led_brightness_control.py", LED_BRIGHTNESS_CONTROL_PY, "led_brightness_control.py"},
#endif
#ifdef INCLUDE_VOLTAGE_MONITOR
        {"/python_scripts/examples/voltage_monitor.py", VOLTAGE_MONITOR_PY, "voltage_monitor.py"},
#endif
#ifdef INCLUDE_STYLOPHONE
        {"/python_scripts/examples/stylophone.py", STYLOPHONE_PY, "stylophone.py"},
#endif
    };
    
    int totalExamples = sizeof(examples) / sizeof(examples[0]);
    
    // If no examples are enabled, exit early
    if (totalExamples == 0) {
        if (globalFileManager != nullptr) {
            //globalFileManager->outputToArea("[INIT] No examples enabled at compile time", 155);
        } else {
            addFilesystemMessage("[INIT] No examples enabled at compile time", 155);
        }
        return;
    }
    
    // Quick check - if all enabled files exist and not forced, do nothing silently
    if (!forceInitialization) {
        bool allFilesExist = true;
        for (int i = 0; i < totalExamples; i++) {
            if (!FatFS.exists(examples[i].path)) {
                allFilesExist = false;
                break;
            }
        }
        
        // If all enabled files exist and not forced, exit silently
        if (allFilesExist && FatFS.exists("/python_scripts") && FatFS.exists("/python_scripts/examples")) {
            return;
        }
    }
    
    // Only provide feedback if we're actually doing work
    bool useOutputArea = (globalFileManager != nullptr);
    
    String initAction = forceInitialization ? "[FORCE INIT]" : "[INIT]";
    String initMsg = initAction + " Initializing " + String(totalExamples) + " MicroPython examples...";
    if (useOutputArea) {
        globalFileManager->outputToArea(initMsg, 155);
    } else {
        addFilesystemMessage(initMsg, 155);
    }
    
    // Clean up old location if it exists (migration from previous version)
    if (FatFS.exists("/micropython_examples")) {
        if (useOutputArea) {
            globalFileManager->outputToArea("[MIGRATION] Cleaning up old location...", 155);
        } else {
            addFilesystemMessage("[MIGRATION] Cleaning up old location...", 155);
        }
        
        // Try to remove files in the old directory first
        Dir oldDir = FatFS.openDir("/micropython_examples");
        while (oldDir.next()) {
            String fileName = oldDir.fileName();
            String fullPath = "/micropython_examples/" + fileName;
            FatFS.remove(fullPath.c_str());
        }
        
        // Then try to remove the directory itself
        FatFS.rmdir("/micropython_examples");
        
        if (useOutputArea) {
            globalFileManager->outputToArea("Old location cleaned up", 155);
        } else {
            addFilesystemMessage("Old location cleaned up", 155);
        }
    }
    
    // First ensure python_scripts directory exists
    if (!FatFS.exists("/python_scripts")) {
        if (useOutputArea) {
            globalFileManager->outputToArea("[CREATE] Creating /python_scripts directory...", 155);
        } else {
            addFilesystemMessage("[CREATE] Creating /python_scripts directory...", 155);
        }
        
        if (!FatFS.mkdir("/python_scripts")) {
            if (useOutputArea) {
                globalFileManager->outputToArea("ERROR: Failed to create /python_scripts", 196);
            } else {
                addFilesystemMessage("ERROR: Failed to create /python_scripts", 196);
            }
            return;
        }
        
        if (useOutputArea) {
            globalFileManager->outputToArea("Created /python_scripts directory", 155);
        } else {
            addFilesystemMessage("Created /python_scripts directory", 155);
        }
    }
    
    // Ensure examples directory exists inside python_scripts
    if (!FatFS.exists("/python_scripts/examples")) {
        if (useOutputArea) {
            globalFileManager->outputToArea("[CREATE] Creating examples directory...", 155);
        } else {
            addFilesystemMessage("[CREATE] Creating examples directory...", 155);
        }
        
        // Create the examples directory
        if (!FatFS.mkdir("/python_scripts/examples")) {
            if (useOutputArea) {
                globalFileManager->outputToArea("ERROR: Failed to create examples directory", 196);
            } else {
                addFilesystemMessage("ERROR: Failed to create examples directory", 196);
            }
            return;
        }
        
        if (useOutputArea) {
            globalFileManager->outputToArea("Created examples directory", 155);
        } else {
            addFilesystemMessage("Created examples directory", 155);
        }
    }
    
    // Check available memory before creating files
    size_t freeHeap = rp2040.getFreeHeap();
    if (freeHeap < 20000) {  // Require at least 20KB free heap
        String errorMsg = "ERROR: Low memory (" + String(freeHeap) + " bytes), skipping file creation";
        if (useOutputArea) {
            globalFileManager->outputToArea(errorMsg, 196);
        } else {
            addFilesystemMessage(errorMsg, 196);
        }
        return;
    }
    
    int filesToCreate = 0;
    int filesCreated = 0;
    int filesSkipped = 0;
    
    if (useOutputArea) {
        globalFileManager->outputToArea("[FILES] Checking example files...", 155);
    } else {
        addFilesystemMessage("[FILES] Checking example files...", 155);
    }
    
    // First pass - count files that need to be created
    for (int i = 0; i < totalExamples; i++) {
        if (!FatFS.exists(examples[i].path) || forceInitialization) {
            filesToCreate++;
        } else {
            filesSkipped++;
        }
    }
    
    if (filesToCreate > 0) {
        String action = forceInitialization ? "Overwriting" : "Creating";
        String startMsg = action + " " + String(filesToCreate) + " example files...";
        if (useOutputArea) {
            globalFileManager->outputToArea(startMsg, 155);
        } else {
            addFilesystemMessage(startMsg, 155);
        }
    }
    
    // Safety timeout - don't spend more than 30 seconds creating files
    unsigned long startTime = millis();
    const unsigned long maxTime = 30000; // 30 seconds
    
    for (int i = 0; i < totalExamples; i++) {
        // Check timeout to prevent system lockup
        if (millis() - startTime > maxTime) {
            String timeoutMsg = "TIMEOUT: File creation aborted after 30s";
            if (useOutputArea) {
                globalFileManager->outputToArea(timeoutMsg, 196);
            } else {
                addFilesystemMessage(timeoutMsg, 196);
            }
            break;
        }
        
        if (!FatFS.exists(examples[i].path) || forceInitialization) {
            // Show progress for each file
            String action = forceInitialization ? "Overwriting" : "Creating";
            String progressMsg = action + " " + String(examples[i].name) + " (" + String(filesCreated + 1) + "/" + String(filesToCreate) + ")";
            if (useOutputArea) {
                globalFileManager->outputToArea(progressMsg, 155);
            } else {
                addFilesystemMessage(progressMsg, 155);
            }
            
            bool success = writeStringToFile(examples[i].path, examples[i].content);
            
            if (success) {
                filesCreated++;
                String successMsg = forceInitialization ? "✓ Overwrote " : "✓ Created ";
                successMsg += String(examples[i].name);
                if (useOutputArea) {
                    globalFileManager->outputToArea(successMsg, 155);
                } else {
                    addFilesystemMessage(successMsg, 155);
                }
            } else {
                String action = forceInitialization ? "overwrite" : "create";
                String errorMsg = "✗ Failed to " + action + " " + String(examples[i].name);
                if (useOutputArea) {
                    globalFileManager->outputToArea(errorMsg, 196);
                } else {
                    addFilesystemMessage(errorMsg, 196);
                }
                // Continue with other files even if one fails
            }
            
            // Add delay between file writes to prevent system overload
            delay(100);
            
            // Process any pending serial data to keep connection alive
            if (Serial.available()) {
                while (Serial.available()) {
                    Serial.read();
                }
            }
        }
    }
    
    // Summary message
    String summary;
    if (filesToCreate > 0) {
        if (forceInitialization) {
            if (filesCreated == filesToCreate) {
                summary = "✓ All " + String(filesCreated) + " examples updated successfully";
            } else if (filesCreated > 0) {
                summary = "⚠ Updated " + String(filesCreated) + "/" + String(filesToCreate) + " examples";
            } else {
                summary = "✗ Failed to update example files";
            }
        } else {
            if (filesCreated == filesToCreate) {
                summary = "✓ Created " + String(filesCreated) + " example files";
            } else if (filesCreated > 0) {
                summary = "⚠ Created " + String(filesCreated) + "/" + String(filesToCreate) + " examples";
            } else {
                summary = "✗ Failed to create example files";
            }
        }
    } else {
        summary = "✓ " + String(filesSkipped) + " examples already exist";
    }
    
    // Always use addFilesystemMessage for consistency (it routes to persistent messages when file manager is active)
    addFilesystemMessage(summary, filesCreated > 0 || filesToCreate == 0 ? 155 : 196);
    
    // Clear older messages to prevent memory buildup
    if (fsMessageCount > 5) {
        fsMessageCount = 3;  // Keep only the last 3 messages
    }
}



bool verifyMicroPythonExamples() {
    // Check if all expected files exist
    const char* expectedFiles[] = {
        "/python_scripts/examples/01_dac_basics.py",
        "/python_scripts/examples/02_adc_basics.py", 
        "/python_scripts/examples/03_gpio_basics.py",
        "/python_scripts/examples/04_node_connections.py",
        "/python_scripts/examples/README.md",
        "/python_scripts/examples/test_examples.py",
        "/python_scripts/examples/led_brightness_control.py",
        "/python_scripts/examples/voltage_monitor.py",
        "/python_scripts/examples/stylophone.py"
    };
    
    bool allFilesExist = true;
    int fileCount = sizeof(expectedFiles) / sizeof(expectedFiles[0]);
    
    Serial.println("Verifying MicroPython examples...");
    
    for (int i = 0; i < fileCount; i++) {
        if (!FatFS.exists(expectedFiles[i])) {
            Serial.print("  MISSING: ");
            Serial.println(expectedFiles[i]);
            allFilesExist = false;
        } else {
            // Check file size to ensure it's not empty
            File checkFile = FatFS.open(expectedFiles[i], "r");
            if (checkFile) {
                size_t fileSize = checkFile.size();
                checkFile.close();
                if (fileSize > 0) {
                    Serial.print("  OK: ");
                    Serial.print(expectedFiles[i]);
                    Serial.print(" (");
                    Serial.print(fileSize);
                    Serial.println(" bytes)");
                } else {
                    Serial.print("  EMPTY: ");
                    Serial.println(expectedFiles[i]);
                    allFilesExist = false;
                }
            } else {
                Serial.print("  UNREADABLE: ");
                Serial.println(expectedFiles[i]);
                allFilesExist = false;
            }
        }
    }
    
    if (allFilesExist) {
        Serial.println("All MicroPython example files verified successfully!");
    } else {
        Serial.println("Some MicroPython example files are missing or corrupted!");
    }
    
    return allFilesExist;
}

//==============================================================================
// Output Area Management Functions
//==============================================================================

void FileManager::clearOutputArea() {
    // Clear the output area
    for (int i = 0; i < outputAreaHeight; i++) {
        moveCursor(outputAreaStartRow + i, 1);
        clearCurrentLine();
    }
    outputAreaCurrentRow = 0;
}

void FileManager::outputToArea(const String& text, int color) {
    if (outputAreaCurrentRow >= outputAreaHeight) {
        // Scroll up by shifting all lines up
        for (int i = 0; i < outputAreaHeight - 1; i++) {
            moveCursor(outputAreaStartRow + i, 1);
            clearCurrentLine();
        }
        outputAreaCurrentRow = outputAreaHeight - 1;
    }
    
    moveCursor(outputAreaStartRow + outputAreaCurrentRow, 1);
    clearCurrentLine();
    
    changeTerminalColor(color, false);
    Serial.print(text);
    Serial.print("\n\r"); // Use proper line ending
    changeTerminalColor(-1, false); // Reset colors
    
    outputAreaCurrentRow++;
}

void FileManager::showOutputAreaBorder() {
    moveCursor(outputAreaStartRow - 1, 1);
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print("╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌╌\n\r");
    changeTerminalColor(-1, false); // Reset colors
}

String FileManager::promptInOutputArea(const String& prompt) {
    // Show the border if this is the first use
    if (outputAreaCurrentRow == 0) {
        showOutputAreaBorder();
    }
    
    // Show the prompt WITHOUT the automatic newline from outputToArea
    moveCursor(outputAreaStartRow + outputAreaCurrentRow, 1);
    clearCurrentLine();
    
    changeTerminalColor(FileColors::STATUS, false);
    Serial.print(prompt);
    changeTerminalColor(-1, false); // Reset colors
    
    // Position cursor right after the prompt text for input
    int inputStartCol = prompt.length() + 1;
    
    // Show cursor for input
    Serial.print("\033[?25h");
    Serial.flush();
    
    String input = "";
    while (true) {
        if (Serial.available()) {
            char c = Serial.read();
            if (c == '\r' || c == '\n') {
                Serial.print("\n\r");
                outputAreaCurrentRow++; // Move to next line after input
                break;
            } else if (c == 8 || c == 127) { // Backspace
                if (input.length() > 0) {
                    input.remove(input.length() - 1);
                    // Move cursor back, print space to clear character, move cursor back again
                    Serial.print("\b \b");
                    Serial.flush();
                }
            } else if (c == 27) { // ESC - cancel
                Serial.print("\n\r");
                outputAreaCurrentRow++; // Move to next line
                moveCursor(outputAreaStartRow + outputAreaCurrentRow, 1);
                clearCurrentLine();
                changeTerminalColor(FileColors::ERROR, false);
                Serial.print("[Cancelled]\n\r");
                changeTerminalColor(-1, false);
                outputAreaCurrentRow++;
                Serial.print("\033[?25l"); // Hide cursor
                Serial.flush();
                return "";
            } else if (c >= 32 && c <= 126) { // Printable characters
                input += c;
                // Echo the character immediately
                Serial.print(c);
                Serial.flush(); // Ensure immediate display
            }
            // Ignore other control characters
        }
        delayMicroseconds(10);
    }
    
    // Hide cursor
    Serial.print("\033[?25l");
    Serial.flush();
    
    return input;
}

//==============================================================================
// Input Management Functions
//==============================================================================

void FileManager::blockInputBriefly() {
    lastDisplayUpdate = millis();
}

void FileManager::clearBufferedInput(bool allowCtrlQ) {
    char ctrlQFound = 0;
    while (Serial.available()) {
        char c = Serial.read();
        // Save Ctrl+Q if we want to allow it
        if (allowCtrlQ && c == 17) { // Ctrl+Q
            ctrlQFound = c;
        }
    }
    
    // Put Ctrl+Q back by sending it to Serial - this is a simple approach
    // In practice, the main loop will need to handle this specially
    if (ctrlQFound != 0) {
        // We can't easily put it back, so we'll handle this in isInputBlocked()
        // by allowing Ctrl+Q to pass through even during blocking
    }
}

bool FileManager::isInputBlocked() {
    if (lastDisplayUpdate == 0) return false;
    
    unsigned long elapsed = millis() - lastDisplayUpdate;
    return elapsed < INPUT_BLOCK_TIME;
}

// Inline editing mode - provides enhanced multiline editing without screen clearing
String launchInlineEkilo(const String& initial_content) {
    // Flush any pending serial data before launching editor
    Serial.flush();
    while (Serial.available() > 0) {
        Serial.read();
    }
    
    // Launch inline editor
    String result = ekilo_inline_edit(initial_content);
    
    // Restore interactive mode if using Serial
    Serial.write(0x0E); // turn on interactive mode
    Serial.flush();
    
    return result;
}

//==============================================================================
// Persistent Filesystem Message Management Functions
//==============================================================================

void FileManager::addPersistentMessage(const String& message, int color) {
    if (persistentMessageCount >= MAX_PERSISTENT_MESSAGES) {
        // Shift messages up and add new one at end
        for (int i = 0; i < MAX_PERSISTENT_MESSAGES - 1; i++) {
            persistentMessages[i] = persistentMessages[i + 1];
        }
        persistentMessageCount = MAX_PERSISTENT_MESSAGES - 1;
    }
    
    persistentMessages[persistentMessageCount].message = message;
    persistentMessages[persistentMessageCount].color = color;
    persistentMessages[persistentMessageCount].timestamp = millis();
    persistentMessageCount++;
    
    // Immediately display the updated messages
    displayPersistentMessages();
}

void FileManager::clearPersistentMessages() {
    persistentMessageCount = 0;
    
    // Clear the persistent message area
    for (int i = 0; i < persistentMessageHeight; i++) {
        moveCursor(persistentMessageStartRow + i, 1);
        clearCurrentLine();
    }
}

void FileManager::displayPersistentMessages() {
    // Auto-cleanup old messages (older than 30 seconds)
    unsigned long currentTime = millis();
    int validMessages = 0;
    for (int i = 0; i < persistentMessageCount; i++) {
        if (currentTime - persistentMessages[i].timestamp < 30000) { // 30 seconds
            if (validMessages != i) {
                persistentMessages[validMessages] = persistentMessages[i];
            }
            validMessages++;
        }
    }
    persistentMessageCount = validMessages;
    
    // Clear the persistent message area first
    for (int i = 0; i < persistentMessageHeight; i++) {
        moveCursor(persistentMessageStartRow + i, 1);
        clearCurrentLine();
    }
    
    // Display messages (newest first, up to available lines)
    int messagesToShow = min(persistentMessageCount, persistentMessageHeight);
    for (int i = 0; i < messagesToShow; i++) {
        moveCursor(persistentMessageStartRow + i, 1);
        
        // Show the most recent messages first
        int messageIndex = persistentMessageCount - messagesToShow + i;
        
        changeTerminalColor(persistentMessages[messageIndex].color, false);
        Serial.print("⟨ ");
        Serial.print(persistentMessages[messageIndex].message);
        changeTerminalColor(-1, false); // Reset colors
    }
    
    Serial.flush();
}

void FileManager::initializePersistentMessageArea() {
    // Draw separator line above persistent messages
    moveCursor(persistentMessageStartRow - 1, 1);
    changeTerminalColor(240, false); // Dark gray
    Serial.print("⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯⋯\n\r");
    changeTerminalColor(-1, false); // Reset colors
}

