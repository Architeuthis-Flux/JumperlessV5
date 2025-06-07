# Local File Monitoring for Jumperless

The Jumperless Bridge App now supports monitoring local .ino Arduino sketch files for changes and automatically flashing them to your Arduino when modified.

## How to Use

### 1. Assign a Local File to a Slot

1. Run the Jumperless Bridge App
2. Type `menu` to open the app menu
3. Type `slots` to manage slot assignments
4. Select a slot number (0-7)
5. When prompted for "link/path >", enter the path to your .ino file:
   
   **Examples:**
   - `./example_sketch.ino` (relative path)
   - `/Users/username/Documents/my_sketch.ino` (absolute path on macOS/Linux)
   - `C:\Users\username\Documents\my_sketch.ino` (absolute path on Windows)

### 2. Automatic Monitoring

Once assigned, the app will:
- Monitor the file for changes (modification time and content)
- Automatically compile and flash the Arduino when the file is modified
- Display status messages showing when flashing occurs
- Handle errors gracefully and continue monitoring

### 3. Slot Display

The slot assignment display now shows:
- `[LOCAL] filename.ino (valid)` for valid local files
- `[LOCAL] filename.ino (missing)` for files that can't be found
- `[WOKWI] https://wokwi.com/projects/...` for Wokwi projects

### 4. Requirements

- Arduino CLI must be available (automatically installed by the app)
- Arduino flashing must be enabled (use `arduino` command in menu to toggle)
- Valid Arduino port must be configured
- The .ino file must contain valid Arduino code

### 5. Example Workflow

1. Create a new Arduino sketch file: `my_project.ino`
2. Assign it to slot 0 in the Jumperless app
3. Edit the file in your favorite editor (VS Code, Arduino IDE, etc.)
4. Save the file
5. The Jumperless app automatically detects the change and flashes your Arduino

### 6. Debugging

- Use the `debug` command in the menu to enable debug output
- Check the `status` command to verify Arduino CLI and port configuration
- Ensure your .ino file has valid Arduino syntax

### 7. Mixed Usage

You can mix local files and Wokwi projects:
- Slot 0: Local file `sensor_code.ino`
- Slot 1: Wokwi project `https://wokwi.com/projects/123456`
- Slot 2: Local file `motor_control.ino`
- etc.

## Benefits

- **Fast Development**: No need to manually compile and upload
- **Real-time Testing**: Changes are automatically deployed
- **Editor Freedom**: Use any text editor or IDE you prefer
- **Version Control**: Keep your sketches in Git repositories
- **Offline Development**: No internet required for local files 