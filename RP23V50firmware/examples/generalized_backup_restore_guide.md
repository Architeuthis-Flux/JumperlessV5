# Generalized NodeFile Backup/Restore System

## Overview
A comprehensive, reusable system for temporarily storing and restoring `nodeFileString` state. This can be used from anywhere in the codebase for operations that need temporary state management.

## Core Functions

### **Storage Functions**
```cpp
void storeNodeFileBackup(void);
```
- Saves current `nodeFileString` state to backup buffer
- Automatically loads from file if `nodeFileString` is empty
- Safe for repeated calls (overwrites previous backup)

### **Restore Functions**
```cpp
void restoreNodeFileBackup(void);
```
- Restores `nodeFileString` from backup (memory only)
- Does NOT save to file automatically
- Clears backup after restore

```cpp
void restoreAndSaveNodeFileBackup(void);
```
- Restores `nodeFileString` from backup
- Automatically saves restored state to current slot
- Clears backup after restore

### **Query Functions**
```cpp
bool hasNodeFileBackup(void);
```
- Returns `true` if backup is stored, `false` otherwise

```cpp
bool hasNodeFileChanges(void);
```
- Returns `true` if current state differs from backup
- Returns `false` if no backup or states are identical

```cpp
const char* getNodeFileBackup(void);
```
- Returns read-only pointer to backup buffer
- Returns `nullptr` if no backup stored
- Use for debugging or advanced operations

### **Utility Functions**
```cpp
void clearNodeFileBackup(void);
```
- Clears backup buffer and marks as unused
- Safe to call even if no backup exists

## Usage Patterns

### **Basic Save/Restore**
```cpp
// Save current state
storeNodeFileBackup();

// Make some temporary changes
// ... modify nodeFileString ...

// Restore to saved state
restoreAndSaveNodeFileBackup();
```

### **Change Detection**
```cpp
storeNodeFileBackup();

// Make modifications
// ... 

if (hasNodeFileChanges()) {
    Serial.println("State has been modified");
    if (userWantsToKeepChanges) {
        clearNodeFileBackup(); // Keep changes
    } else {
        restoreAndSaveNodeFileBackup(); // Discard changes
    }
}
```

### **MicroPython Integration**
```cpp
// Automatically used in MicroPython REPL:
void enterMicroPythonREPL() {
    storeNodeFileBackup();        // Save entry state
    // ... REPL session ...
}

void exitMicroPythonREPL() {
    // User choice: save or discard
    if (saveChanges) {
        clearNodeFileBackup();    // Keep changes
    } else {
        restoreAndSaveNodeFileBackup(); // Discard changes
    }
}
```

## MicroPython API

### **Functions Available in Python**
```python
nodes_has_changes()    # Check if changes since entry
nodes_discard()        # Restore to entry state  
nodes_save()          # Save current state to slot
```

### **Example Python Usage**
```python
# Check for unsaved changes
if nodes_has_changes():
    print("You have unsaved changes!")
    
# Discard all changes since entering REPL
nodes_discard()

# Or save changes to persistent storage
nodes_save()
```

## Use Cases

### **1. Experimental Operations**
```cpp
storeNodeFileBackup();
tryExperimentalRouting();
if (!routingSuccessful) {
    restoreNodeFileBackup();
}
```

### **2. Menu Systems**
```cpp
// When entering a submenu
storeNodeFileBackup();

// User makes changes in menu
// ...

// On menu exit
if (userSelectedSave) {
    clearNodeFileBackup();
} else {
    restoreAndSaveNodeFileBackup();
}
```

### **3. File Import/Export**
```cpp
storeNodeFileBackup();
if (!importNodeFile(filename)) {
    restoreNodeFileBackup(); // Restore on import failure
    return false;
}
clearNodeFileBackup(); // Success - keep imported data
```

### **4. Debugging/Testing**
```cpp
storeNodeFileBackup();

// Run test operations
runTestConnections();

// Always restore after testing
restoreNodeFileBackup();
```

## Technical Details

- **Buffer Size**: 1800 bytes (matches `nodeFileString` capacity)
- **Thread Safety**: Functions are not thread-safe (call from same core)
- **Memory**: Single static buffer, minimal overhead
- **Performance**: Fast string operations, no file I/O during backup/restore
- **Persistence**: Backup is lost on reboot (intentional - temporary storage only)

## Benefits

✅ **Reusable**: Can be used from anywhere in the codebase  
✅ **Simple**: Clean API with clear function names  
✅ **Safe**: Handles edge cases (empty strings, buffer overflow)  
✅ **Fast**: Memory-only operations during backup/restore  
✅ **Flexible**: Multiple restore options (memory-only vs save-to-file)  
✅ **Debuggable**: Query functions for state inspection  

## Integration

The system is automatically used by:
- MicroPython REPL (entry state backup)
- Any other component that includes `FileParsing.h`

Simply call the functions when temporary state management is needed! 