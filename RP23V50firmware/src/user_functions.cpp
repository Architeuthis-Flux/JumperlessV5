#include "user_functions.h"
#include "JumperlessDefines.h"
#include <Wire.h>



//_____________  Add New User Defined Functions Here _____________________________________

//Exammple: greet --name Romonaga
//Exammple: greet --name 'Romonaga' --mood 'Fucked Off'
static void cmd_greet(int argc, String argv[]) {
    // --name <value>, default = "stranger"
    String name = "stranger";    
    String mood = "happy";    
    String wrk;

    if (uf_getOpt(argc, argv, "name", wrk) && wrk.length() > 0) {
        name = wrk;
    }

    if (uf_getOpt(argc, argv, "mood", wrk) && wrk.length() > 0) {
        mood = wrk;
    }

    Serial.print("Hello, ");
    Serial.print(name);
    Serial.print("! You seem ");
    Serial.print(mood);
    Serial.println(" today.");
    Serial.flush();
}


// Example: turn an LED on/off
static void cmd_led(int argc, String argv[]) {

    String state;
    int pin = 0;
    
    pin = uf_getOptInt(argc, argv, "pin", LED_BUILTIN);  // --pin N
    
    if (!uf_getOpt(argc, argv, "state", state)) state = "on"; // --state on/off

    pinMode(pin, OUTPUT);
    digitalWrite(pin, state == "on" ? HIGH : LOW);

    Serial.print("LED on pin ");
    Serial.print(pin);
    Serial.print(" set to ");
    Serial.println(state);
    Serial.flush();
}

// --- Another sample: blink --pin 13 --ms 250
static void cmd_blink(int argc, String argv[]) {
    int pin = (int)uf_getOptInt(argc, argv, "pin", LED_BUILTIN);
    int ms  = (int)uf_getOptInt(argc, argv, "ms", 500);

    pinMode(pin, OUTPUT);
    Serial.print("blink: pin="); Serial.print(pin); Serial.print(" period(ms)="); Serial.println(ms);
    for (int i = 0; i < 6; ++i) { 
        digitalWrite(pin, HIGH); delay(ms/2); digitalWrite(pin, LOW); delay(ms/2); 
    }
}


int i2cScan(int sdaRow = -1, int sclRow = -1,
            int sdaPin = 26, int sclPin = 27,
            int leaveConnections = 0) 
{
    int found = 0;

    // Choose pins. If caller passed explicit rows, you could map them to pins here.
    // For now we just fall back to pin args.
    int useSda = (sdaRow >= 0) ? sdaRow : sdaPin;
    int useScl = (sclRow >= 0) ? sclRow : sclPin;


    Wire1.end();
    Wire1.begin();
    Wire1.setSDA(sdaPin);
    Wire1.setSCL(sclPin);
    Wire1.begin();
    Wire1.setClock(100000);
    Serial.print("I2C scan: SDA=");
    Serial.print(useSda);
    Serial.print(" SCL=");
    Serial.println(useScl);

    for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
        Serial.print("  Found device at 0x");
        if (address < 16) Serial.print("0");
        Serial.println(address, HEX);
        found++;
    } else if (error == 4) {
        Serial.print("  Unknown error at 0x");
        if (address < 16) Serial.print("0");
        Serial.println(address, HEX);
    }
    }

    if (found == 0) {
    Serial.println("No I2C devices found.");
    } else {
    Serial.print("Done. Devices found: ");
    Serial.println(found);
    }

    if (!leaveConnections) {
        Wire.end();
    
    }

    return found;
}


// Perform an I2C bus scan. Returns number of devices found.
//
// Args:
//   sdaRow / sclRow: logical row numbers (if <0, ignored)
//   sdaPin / sclPin: actual hardware pin numbers (defaults shown)
//   leaveConnections: if nonzero, leave Wire running; if 0, end() after scan
//   i2cscan                         // use defaults (26,27)
//   i2cscan --sdaRow 21 --sclRow 22 // use row-based override
//   i2cscan --sdaPin 4 --sclPin 5   // explicit pins
//   i2cscan -v                      verbose flag
//   The leave flag will leave the wire open
static void cmd_i2cScan(int argc, String argv[]) {
  int sdaRow = (int)uf_getOptInt(argc, argv, "sdaRow", -1);
  int sclRow = (int)uf_getOptInt(argc, argv, "sclRow", -1);
  int sdaPin = (int)uf_getOptInt(argc, argv, "sdaPin", 26);
  int sclPin = (int)uf_getOptInt(argc, argv, "sclPin", 27);
  bool leaveConnections = uf_hasFlag(argc, argv, "leave");
  bool verbose = uf_hasFlag(argc, argv, "v") || uf_hasFlag(argc, argv, "verbose");

  Serial.print("i2cscan: sdaRow="); Serial.print(sdaRow);
  Serial.print(" sclRow="); Serial.print(sclRow);
  Serial.print(" sdaPin="); Serial.print(sdaPin);
  Serial.print(" sclPin="); Serial.print(sclPin);
  Serial.print(" verbose="); Serial.print(verbose ? "true" : "false");
  Serial.print(" leave="); Serial.println(leaveConnections ? "true" : "false");
  

  // Call scanner with unified defaults
  int found = i2cScan(sdaRow, sclRow, sdaPin, sclPin, leaveConnections);

  if (verbose) {
    Serial.print("i2cscan completed, devices found: ");
    Serial.println(found);
  }

  Serial.flush();
}



// =======================
// Command registry (ADD YOUR COMMANDS HERE)
// =======================

// ---- User-visible table. Add new rows here. ----
const UserFunction USER_FUNCTIONS[] = {
  { "i2cScan",  &cmd_i2cScan, "Scan I2C with --sdaRow N --sclRow N [-v]" },
  { "blink",    &cmd_blink,      "Blink LED: --pin N --ms 250" },
  { "led",      &cmd_led,        "Turn an LED on/off: --pin N --state on/off" },
  { "greet",    &cmd_greet, "Say hello: greet --name Romonaga --mood excited" },

  
};

const size_t USER_FUNCTIONS_COUNT = sizeof(USER_FUNCTIONS) / sizeof(USER_FUNCTIONS[0]);






//_____________________________________________  Suppport Functions _________________________

// Tokenizer (supports quotes and escapes)
int uf_tokenize(const String& line, String argv[], int maxTokens) {
  int argc = 0; bool inQuotes = false; char quote = 0; String cur;
  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];
    if (inQuotes) {
      if (c == '\\' && i + 1 < line.length()) {  // allow \" and \\ in quotes
        char n = line[i+1];
        if (n == '"' || n == '\'' || n == '\\') { cur += n; i++; continue; }
      }
      if (c == quote) { inQuotes = false; continue; }
      cur += c;
    } else {
      if (c == '"' || c == '\'') { inQuotes = true; quote = c; continue; }
      if (c == ' ' || c == '\t') {
        if (cur.length()) { if (argc < maxTokens) argv[argc++] = cur; cur = ""; }
      } else {
        cur += c;
      }
    }
  }
  if (cur.length() && argc < maxTokens) argv[argc++] = cur;
  return argc;
}

// Simple option helpers
// Matches --key value, --key=value, -k value
bool uf_getOpt(int argc, String argv[], const String& key, String& outVal) {
  for (int i = 1; i < argc; ++i) {
    String t = argv[i];
    if (t.startsWith("--")) {
      String kv = t.substring(2);
      int eq = kv.indexOf('=');
      if (eq >= 0) {
        if (kv.substring(0, eq) == key) { outVal = kv.substring(eq+1); return true; }
      } else if (kv == key) {
        if (i + 1 < argc && !argv[i+1].startsWith("-")) { outVal = argv[i+1]; return true; }
        outVal = ""; return true; // boolean-style without value
      }
    } else if (t.startsWith("-") && t.length() >= 2) {
      // short key (first char only)
      if (String(t[1]) == key) {
        if (i + 1 < argc && !argv[i+1].startsWith("-")) { outVal = argv[i+1]; return true; }
        outVal = ""; return true;
      }
    }
  }
  return false;
}

bool uf_hasFlag(int argc, String argv[], const String& key) {
  String dummy;
  if (key.length() == 1) { // -v
    for (int i = 1; i < argc; ++i) {
      String t = argv[i];
      if (t.startsWith("-") && !t.startsWith("--")) {
        for (int k = 1; k < (int)t.length(); ++k)
          if (String(t[k]) == key) return true;
      }
    }
  }
  // --verbose or -v handled by uf_getOpt fallback
  return uf_getOpt(argc, argv, key, dummy) && dummy.length() == 0;
}

long uf_getOptInt(int argc, String argv[], const String& key, long deflt) {
  String v;
  if (uf_getOpt(argc, argv, key, v) && v.length()) return v.toInt();
  return deflt;
}


// Lookup + dispatch
const UserFunction* uf_find(const String& name) {
  for (size_t i = 0; i < USER_FUNCTIONS_COUNT; ++i) {
    if (name.equals(USER_FUNCTIONS[i].name)) 
        return &USER_FUNCTIONS[i];
  }
  return nullptr;
}

bool uf_dispatch(const String& line) {
  String argv[20];
  int argc = uf_tokenize(line, argv, 20);
  if (argc == 0) return false;

  const UserFunction* cmd = uf_find(argv[0]);
  if (!cmd) return false;

  cmd->fn(argc, argv);
  return true;
}

String readLine(uint32_t timeoutMs) {
  String s;
  uint32_t start = millis();

  Serial.flush();

  while (millis() - start < timeoutMs) {
    if (Serial.available() == 0) {
      delay(5);
      continue;
    }

    char c = Serial.read();
    if (c < 0) continue;
    
    // Handle Enter (CR, LF, or CRLF)
    if (c == '\r' || c == '\n') {
      if (c == '\r' && Serial.peek() == '\n') {
        Serial.read();
      }
      break;
    }

    // Normal character
    s += c;
  }

  Serial.flush();
  return s;
}


static void prinFunctionHelp() {

    // Otherwise, list all functions
    Serial.println("Available user functions:");

    for (size_t i = 0; i < USER_FUNCTIONS_COUNT; ++i) {
        const auto& c = USER_FUNCTIONS[i];
        // left-pad names to a fixed width for readability
        const int width = 14;
        int n = strlen(c.name);
        Serial.print("  ");
        Serial.print(c.name);
        if (n < width) for (int k = 0; k < width - n; ++k) Serial.print(' ');
        Serial.print(" - ");
        if (c.help && c.help[0]) 
            Serial.println(c.help);
        else                     
            Serial.println("(no help provided)");
    }

    Serial.println("\nPlease enter your custom function (Press Enter Twice!).");
    Serial.flush();
}

void handleUserFunction() {

    prinFunctionHelp();
   
  
    String userFunc = readLine(USER_FUNCTION_WAIT);
    Serial.print("Function Call: ");
    Serial.println(userFunc);
    Serial.flush();


    if (!uf_dispatch(userFunc)) {

        Serial.println("User Function Not Found!:\n\n");
        Serial.println("User Functions Defined:\n");
        for (size_t i = 0; i < USER_FUNCTIONS_COUNT; ++i) {
                Serial.print("  ");
                Serial.print(USER_FUNCTIONS[i].name);
                if (USER_FUNCTIONS[i].help && USER_FUNCTIONS[i].help[0]) {
                Serial.print(" - ");
                Serial.print(USER_FUNCTIONS[i].help);
            }
            Serial.println();
            Serial.flush();
        }

    }

}

// ---- Tokenize a command line into argc/argv (handles quotes) ----
// Supports: words separated by spaces; quotes "like this"; escaped quotes \" inside quotes.
int tokenize(const String& line, String argv[], int maxTokens) {
  int argc = 0;
  bool inQuotes = false;
  char quoteChar = 0;
  String cur;

  for (size_t i = 0; i < line.length(); ++i) {
    char c = line[i];

    if (inQuotes) {
      if (c == '\\' && i + 1 < line.length()) { // allow \" and \\ inside quotes
        char n = line[i+1];
        if (n == '"' || n == '\'' || n == '\\') { cur += n; i++; continue; }
      }
      if (c == quoteChar) { inQuotes = false; continue; }
      cur += c;
    } else {
      if (c == '"' || c == '\'') { inQuotes = true; quoteChar = c; continue; }
      if (c == ' ' || c == '\t') {
        if (cur.length()) {
          if (argc < maxTokens) argv[argc++] = cur;
          cur = "";
        }
      } else {
        cur += c;
      }
    }
  }
  if (cur.length() && argc < maxTokens) argv[argc++] = cur;
  return argc;
}

// ---- Parse options: --key value, --key=value, -k value, -abc (bool flags) ----
struct Opt {
  String key;    // without leading dashes
  String value;  // empty => boolean flag
};



int parseOptions(int argc, String argv[], Opt opts[], int maxOpts) {
  int nopt = 0;
  for (int i = 1; i < argc; ++i) {
    String t = argv[i];
    if (t.startsWith("--")) {
      String keyval = t.substring(2);
      int eq = keyval.indexOf('=');
      if (eq >= 0) {
        if (nopt < maxOpts) { opts[nopt++] = { keyval.substring(0, eq), keyval.substring(eq+1) }; }
      } else {
        String val = "";
        if (i + 1 < argc && !argv[i+1].startsWith("-")) { val = argv[++i]; }
        if (nopt < maxOpts) { opts[nopt++] = { keyval, val }; }
      }
    } else if (t.startsWith("-") && t.length() > 1) {
      // short flags can be bundled like -abc or take value as next token (-o out)
      for (int k = 1; k < (int)t.length(); ++k) {
        String key( (char[]){ t[k], 0 } );
        String val = "";
        // if it's the last short flag in this token, allow value in next argv
        if (k == (int)t.length() - 1 && i + 1 < argc && !argv[i+1].startsWith("-")) {
          val = argv[++i];
        }
        if (nopt < maxOpts) { opts[nopt++] = { key, val }; }
      }
    } else {
      // positional argument; treat as key="" with value=t
      if (nopt < maxOpts) { opts[nopt++] = { "", t }; }
    }
  }
  return nopt;
}

// ---- Example dispatcher: call functions by name ----
void callFunctionByName(const String& name, int nopt, Opt opts[]) {
  // Stub: replace with your real registry
  if (name == "i2cScanApp" || name == "I2CScanApp") {
    // find args: --sdaRow, --sclRow, -v
    int sda = -1, scl = -1; bool verbose = false;
    for (int i = 0; i < nopt; ++i) {
      if (opts[i].key == "sdaRow") sda = opts[i].value.toInt();
      else if (opts[i].key == "sclRow") scl = opts[i].value.toInt();
      else if (opts[i].key == "v") verbose = true;
      else if (opts[i].key == "" && sda < 0) sda = opts[i].value.toInt(); // positional example
    }
    Serial.print("Calling i2cScanApp with sdaRow="); Serial.print(sda);
    Serial.print(" sclRow="); Serial.print(scl);
    Serial.print(" verbose="); Serial.println(verbose ? "true" : "false");
    // i2cScanApp(sda, scl, verbose); // <-- your actual function
  } else {
    Serial.print("Unknown function: "); Serial.println(name);
  }
}
