#ifndef FILESYSTEMSTUFF_H
#define FILESYSTEMSTUFF_H

#include <Arduino.h>
#include <FatFS.h>
#include "oled.h"

// Forward declarations
class FileManager;

// File types for icons and handling
enum FileType {
    FILE_TYPE_UNKNOWN = 0,
    FILE_TYPE_PYTHON = 1,
    FILE_TYPE_TEXT = 2,
    FILE_TYPE_CONFIG = 3,
    FILE_TYPE_JSON = 4,
    FILE_TYPE_DIRECTORY = 5,
    FILE_TYPE_NODEFILES = 6,
    FILE_TYPE_COLORS = 7
};

// File entry structure
struct FileEntry {
    String name;
    String path;
    FileType type;
    size_t size;
    bool isDirectory;
    time_t lastModified;
};

// Display configuration constants
static const int DEFAULT_DISPLAY_LINES = 25; // Increased from ~15 to 25 lines

// Color scheme for filesystem display
struct FileColors {
    static const int DIRECTORY = 51;     // Cyan
    static const int PYTHON = 155;       // Green 
    static const int TEXT = 221;         // Yellow
    static const int CONFIG = 207;       // Magenta
    static const int JSON = 69;          // Blue
    static const int NODEFILES = 202;    // Orange
    static const int COLORS = 199;       // Pink
    static const int UNKNOWN = 248;      // Light grey
    static const int HEADER = 38;        // Bright green
    static const int STATUS = 226;       // Bright yellow
    static const int ERROR = 196;        // Red
};

class FileManager {
private:
    String currentPath;
    FileEntry* fileList;
    int fileCount;
    int maxFiles;
    int selectedIndex;
    int displayOffset;
    int maxDisplayLines;
    
    // OLED update batching
    unsigned long lastInputTime;
    bool oledUpdatePending;
    static const unsigned long OLED_UPDATE_DELAY = 50000; // 50 milliseconds for responsive updates
    
    // Rotary encoder state tracking
    int lastEncoderDirectionState;
    int lastEncoderButtonState;
    
    // OLED horizontal scrolling
    int oledHorizontalOffset;
    int oledCursorPosition;
    static const int OLED_SCROLL_MARGIN = 3; // Start scrolling when within 3 chars of edge
    
    // REPL mode and positioning
    bool replMode = false;
    bool shouldExitForREPL = false;  // Flag to exit file manager when content is ready for REPL
    int originalCursorRow;
    int originalCursorCol;
    int startRow;
    int linesUsed;
    
    // File content storage for REPL mode
    String lastOpenedFileContent;
    
    // Output area for prompts and messages
    int outputAreaStartRow;
    int outputAreaHeight;
    int outputAreaCurrentRow;
    
    // Persistent filesystem messages that survive interface refreshes
    struct PersistentMessage {
        String message;
        int color;
        unsigned long timestamp;
    };
    static const int MAX_PERSISTENT_MESSAGES = 5;
    PersistentMessage persistentMessages[MAX_PERSISTENT_MESSAGES];
    int persistentMessageCount;
    int persistentMessageStartRow;
    int persistentMessageHeight;
    
    // Input blocking to prevent accidental double-presses
    unsigned long lastDisplayUpdate;
    static const unsigned long INPUT_BLOCK_TIME = 200; // 200ms block after display updates
    
    // Display configuration
    int textAreaLines; // Lines available for text (total - headers - footers)
    
    // Helper functions
    FileType getFileType(const String& filename);
    String getFileIcon(FileType type);
    String formatFileSize(size_t size);
    String formatDateTime(time_t timestamp);
    void updateOLEDStatus();
    void scheduleOLEDUpdate();
    void processOLEDUpdate();
    void calculateHorizontalScrolling(const String& fullText, int cursorPos);
    void showFileOperationMenu(const String& filename);
    bool confirmAction(const String& action, const String& target);
    void initializeFilesystem();
    int calculatePathDepth(const String& path);
    
public:
    FileManager();
    ~FileManager();
    
    // Navigation
    bool changeDirectory(const String& path);
    bool goUp();
    bool goHome();
    void refreshListing();
    
    // Display
    void showCurrentListing(bool showHeader = true);
    void showFileInfo(const FileEntry& file);
    void showHelp();
    
    // File operations
    bool createFile(const String& filename);
    bool createDirectory(const String& dirname);
    bool deleteFile(const String& filename);
    bool renameFile(const String& oldName, const String& newName);
    bool copyFile(const String& source, const String& dest);
    
    // File viewing/editing
    bool viewFile(const String& filename);
    bool editFile(const String& filename);
    bool editFileWithEkilo(const String& filename);
    
    // Navigation controls
    void moveSelection(int direction);
    void selectCurrentFile();
    
    // Main interface
    void run();
    
    // Interactive mode functions
    void initInteractiveMode();
    void drawInterface(bool fullScreen = true);
    void updateFileListDisplay();
    void updateStatusLine();
    void clearScreen();
    void moveCursor(int row, int col);
    void hideCursor();
    void showCursor();
    void clearCurrentLine();
    void showInteractiveHelp();
    void showInteractiveFileView(const String& filename);
    void showInteractiveFileInfo(const FileEntry& file);
    
    // File content tracking
    String getLastSavedFileContent();
    void setREPLMode(bool isREPLMode = false) { replMode = isREPLMode; }
    
    // Getters
    String getCurrentPath() const { return currentPath; }
    int getFileCount() const { return fileCount; }
    FileEntry* getCurrentFile();
    bool getShouldExitForREPL() const { return shouldExitForREPL; }
    
    // Interactive input helpers
    String promptForFilename(const String& prompt);
    
    // Output area management
    void clearOutputArea();
    void outputToArea(const String& text, int color = 248);
    void showOutputAreaBorder();
    String promptInOutputArea(const String& prompt);
    
    // Persistent filesystem message management
    void addPersistentMessage(const String& message, int color = 248);
    void clearPersistentMessages();
    void displayPersistentMessages();
    void initializePersistentMessageArea();
    
    // Input management
    void blockInputBriefly();
    void clearBufferedInput(bool allowCtrlQ = true);
    bool isInputBlocked();
};

// Global functions
void filesystemApp(bool waitForEnter = true);
void filesystemAppPythonScripts(); // Start file manager in python_scripts directory
String filesystemAppPythonScriptsREPL(); // REPL mode - returns content if file saved
void eKiloApp();
String launchEkilo(const char* filename, bool replMode); // Unified eKilo launcher
void launchEkiloStandalone(const char* filename = nullptr); // Legacy wrapper for standalone mode
String launchEkiloREPL(const char* filename = nullptr); // Legacy wrapper for REPL mode
String launchInlineEkilo(const String& initial_content = ""); // Inline editing mode - no screen clearing, no file operations
String generateNextScriptName(); // Helper to generate next available script name

// Utility functions
String getFullPath(const String& basePath, const String& filename);
bool isValidFilename(const String& filename);

// File cleanup functions
void closeAllOpenFiles(void); // Comprehensive file cleanup across all systems

// Global utility functions for external use (e.g., USB filesystem)
FileType getFileTypeFromFilename(const String& filename);
String getFileIconFromType(FileType type);
String formatFileSizeForUSB(size_t size);
void printColoredPath(const String& path);

// Initialize MicroPython examples on boot
void initializeMicroPythonExamples(bool forceInitialization = false);

// Verify MicroPython examples exist
bool verifyMicroPythonExamples();

// Display configuration functions
int getConfiguredDisplayLines();
int getConfiguredEditorLines();

// Filesystem utility functions
bool deleteDirectoryContents(const String& path);

#endif // FILESYSTEMSTUFF_H 