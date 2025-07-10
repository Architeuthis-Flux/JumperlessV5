# Quick Start Guide: Async OLED Display

## TL;DR - Quick Migration

Replace blocking calls with async versions:

```cpp
// OLD (blocking)
oled.clearPrintShow("Message", 2);

// NEW (non-blocking)
oled.clearPrintShowAsync("Message", 2);
while (!oled.isAsyncComplete()) {
    // Do other work or delay(1)
}
```

## Available New Methods

### Basic Async Display
```cpp
bool showAsync();                           // Non-blocking version of show()
bool isAsyncComplete();                     // Check if operation completed
```

### Async Text Display
```cpp
// Basic usage
bool clearPrintShowAsync(const char* text, int textSize = 2, bool clear = true, bool center = true);

// With font selection
bool clearPrintShowAsync(const char* text, int textSize, FontFamily family, bool clear = true, bool center = true);

// String versions also available
bool clearPrintShowAsync(const String& text, ...);
```

## Common Usage Patterns

### 1. Simple Replacement
```cpp
void updateDisplay(const char* message) {
    if (oled.clearPrintShowAsync(message, 2)) {
        // Display started successfully
        Serial.println("Display updating...");
    }
}
```

### 2. Wait for Completion
```cpp
void displayAndWait(const char* message) {
    if (oled.clearPrintShowAsync(message, 2)) {
        while (!oled.isAsyncComplete()) {
            delay(1);  // Or do other work
        }
        Serial.println("Display updated!");
    }
}
```

### 3. In Main Loop (Continuous Updates)
```cpp
static unsigned long lastUpdate = 0;
static int counter = 0;

void loop() {
    // Update display every 500ms
    if (millis() - lastUpdate > 500 && oled.isAsyncComplete()) {
        String msg = "Count: " + String(counter++);
        if (oled.clearPrintShowAsync(msg, 2)) {
            lastUpdate = millis();
        }
    }
    
    // Continue with other work
    handleUserInput();
    updateSensors();
    processNetwork();
}
```

### 4. Error Handling with Fallback
```cpp
void robustDisplay(const char* message) {
    bool started = oled.clearPrintShowAsync(message, 2);
    if (!started) {
        // Fallback to blocking display
        Serial.println("Async failed, using blocking display");
        oled.clearPrintShow(message, 2, true, true, true, -1, -1, 1000);
    }
}
```

## Integration Examples

### Replace Existing Code
```cpp
// BEFORE
void showStatus() {
    oled.clearPrintShow("Status: OK", 2);  // Blocks for ~10-20ms
    processNextTask();                      // Delayed by display
}

// AFTER  
void showStatus() {
    oled.clearPrintShowAsync("Status: OK", 2);  // Returns immediately
    processNextTask();                           // Runs while display updates
}
```

### Real-time Responsive UI
```cpp
void handleEncoder() {
    static long lastPosition = 0;
    
    if (encoderPosition != lastPosition) {
        lastPosition = encoderPosition;
        
        // Update display without blocking encoder response
        String value = "Value: " + String(encoderPosition);
        oled.clearPrintShowAsync(value, 2);
        
        // Encoder remains responsive during display update
    }
}
```

### Multiple Display Updates
```cpp
void showSequence() {
    const char* messages[] = {"Step 1", "Step 2", "Step 3", "Done!"};
    
    for (int i = 0; i < 4; i++) {
        // Wait for previous display to complete
        while (!oled.isAsyncComplete()) {
            delay(1);
        }
        
        // Start next display
        oled.clearPrintShowAsync(messages[i], 2);
        
        // Do other work for this step
        performStep(i);
        
        delay(1000);  // Pause between steps
    }
}
```

## Performance Benefits

### Timing Comparison
```cpp
// Blocking version timing
unsigned long start = micros();
oled.clearPrintShow("Test", 2);           // ~15-25ms blocked
unsigned long blocked = micros() - start;

// Async version timing  
start = micros();
oled.clearPrintShowAsync("Test", 2);      // ~1-2ms to start
unsigned long startup = micros() - start;

// Total time is the same, but main code doesn't wait
```

### Responsiveness Test
```cpp
void responsiveTest() {
    // This loop stays responsive even during display updates
    for (int i = 0; i < 100; i++) {
        // Start display update
        String msg = "Count: " + String(i);
        oled.clearPrintShowAsync(msg, 2);
        
        // Handle input immediately (doesn't wait for display)
        if (digitalRead(BUTTON_PIN) == LOW) {
            Serial.println("Button pressed at count: " + String(i));
            break;
        }
        
        delay(10);  // Fast loop
    }
}
```

## Best Practices

### 1. Always Check Return Value
```cpp
if (oled.clearPrintShowAsync("Message", 2)) {
    // Success - display is updating
} else {
    // Failed - handle error or try again
}
```

### 2. Don't Start New Operations Too Quickly
```cpp
// GOOD: Check completion first
if (oled.isAsyncComplete()) {
    oled.clearPrintShowAsync("New Message", 2);
}

// BAD: Don't spam async calls
// oled.clearPrintShowAsync("Message 1", 2);
// oled.clearPrintShowAsync("Message 2", 2);  // Will fail
```

### 3. Timeout Protection
```cpp
void displayWithTimeout(const char* message) {
    if (oled.clearPrintShowAsync(message, 2)) {
        unsigned long start = millis();
        while (!oled.isAsyncComplete() && (millis() - start < 100)) {
            delay(1);
        }
        
        if (!oled.isAsyncComplete()) {
            Serial.println("Display timeout - may have failed");
        }
    }
}
```

## Migration Checklist

- [ ] Replace `oled.clearPrintShow()` calls with `oled.clearPrintShowAsync()`
- [ ] Add completion checks with `oled.isAsyncComplete()`
- [ ] Test for improved responsiveness in real-time operations
- [ ] Add error handling for failed async operations
- [ ] Verify timing-critical code still works correctly

## When NOT to Use Async

- **Critical error messages**: Use blocking display for immediate visibility
- **Single display updates**: Little benefit if nothing else needs to run
- **Simple programs**: Blocking might be simpler if responsiveness isn't important

## Troubleshooting

### Display Not Updating
```cpp
// Check if operation started
bool started = oled.clearPrintShowAsync("Test", 2);
if (!started) {
    Serial.println("Failed to start async display");
}

// Check completion status
if (!oled.isAsyncComplete()) {
    Serial.println("Display still updating...");
}
```

### Performance Issues
```cpp
// Monitor async operation timing
unsigned long start = millis();
oled.clearPrintShowAsync("Test", 2);
while (!oled.isAsyncComplete()) {
    delay(1);
}
Serial.print("Total display time: ");
Serial.println(millis() - start);
```

This async implementation gives you the best of both worlds: keep all your existing graphics code unchanged while gaining the performance benefits of non-blocking display updates! 