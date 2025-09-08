#include "SyntaxHighlighting.h"
#include <string.h>

// Map editor highlight class to 256-color index
static int map_hl_to_color(int hl) {
  switch (hl) {
    case HL_COMMENT: return 34;      // green
    case HL_MLCOMMENT: return 244;   // gray
    case HL_KEYWORD1: return 214;    // orange
    case HL_KEYWORD2: return 79;     // green (builtins)
    case HL_STRING: return 39;       // cyan
    case HL_NUMBER: return 199;      // red
    case HL_MATCH: return 27;        // blue
    case HL_JUMPERLESS_FUNC: return 207; // magenta
    case HL_JUMPERLESS_TYPE: return 105; // purple
    case HL_JFS_FUNC: return 45;     // cyan-blue
    case HL_TERMINAL_FUNC: return 197; // magenta
    case HL_TERMINAL_NUMBER: return 199; // red
    case HL_TERMINAL_NODE1: return 78; // green
    case HL_TERMINAL_NODE2: return 79; // green
    case HL_TERMINAL_STRING: return 39; // cyan
    default: return 255;             // white
  }
}

SyntaxHighlighting::SyntaxHighlighting(Stream* stream) {
  syntaxHighlightingStream = stream;
}

SyntaxHighlighting::~SyntaxHighlighting() {
}

void SyntaxHighlighting::setStream(Stream* stream) {
  syntaxHighlightingStream = stream;
}

void SyntaxHighlighting::syntaxHighlighting() {
}

void SyntaxHighlighting::syntaxHighlightingToColor(int hl) {
  if (!syntaxHighlightingStream) return;
  int color = map_hl_to_color(hl);
  syntaxHighlightingStream->print("\x1b[38;5;");
  syntaxHighlightingStream->print(color);
  syntaxHighlightingStream->print("m");
}

// Terminal command set derived from main.cpp switch cases
static bool is_terminal_command_char(char c) {
  switch (c) {
    case 'z': case '|': case 'w': case 'X': case 'G': case 'S': case 'j': case 'U':
    case 'u': case '/': case 'C': case 'E': case 'k': case 'R': case '>': case 'P':
    case 'p': case '.': case 'c': case '_': case 'g': case '&': case '\'': case 'x':
    case '+': case '-': case '~': case '`': case '^': case '?': case '@': case '$':
    case 'r': case 'A': case 'a': case 'F': case '=': case 'i': case '#': case 'e':
    case 's': case 'v': case '}': case '{': case 'n': case 'b': case 'm': case '!':
    case 'o': case '<': case 'y': case 'f': case 't': case 'T': case 'l': case 'd':
    case ':':
      return true;
    default:
      break;
  }
  if (c >= '0' && c <= '3') return true; // menu choices in Z
  return false;
}

// Returns static buffer containing input string with ANSI sequences injected
char* SyntaxHighlighting::highlightString(const char* string, enum SyntaxHighlightingType type) {
  static char out[1024];  // Increased to handle longer lines with ANSI codes
  if (!string) {
    out[0] = '\0';
    return out;
  }

  size_t len = strlen(string);
  if (len == 0) {
    out[0] = '\0';
    return out;
  }

  // Always keep result bounded
  const size_t max_out = sizeof(out) - 1;
  size_t pos = 0;

  auto push = [&](char ch) {
    if (pos + 1 < max_out) out[pos++] = ch;
  };
  auto push_str = [&](const char* s) {
    while (*s && pos + 1 < max_out) out[pos++] = *s++;
  };
  auto push_color = [&](int color) {
    char seq[24];
    snprintf(seq, sizeof(seq), "\x1b[38;5;%dm", color);
    push_str(seq);
  };
  auto push_reset = [&]() {
    push_str("\x1b[0m");
  };

  if (type == SYNTAX_TERMINAL) {
    // Per-character classification; typically called with single-char input
    for (size_t i = 0; i < len; i++) {
      char c = string[i];
      if (is_terminal_command_char(c)) {
        push_color(map_hl_to_color(HL_JUMPERLESS_FUNC));
        push(c);
        push_reset();
      } else if ((c >= '0' && c <= '9')) {
        push_color(map_hl_to_color(HL_NUMBER));
        push(c);
        push_reset();
      } else if (c == '\'' || c == '"') {
        push_color(map_hl_to_color(HL_STRING));
        push(c);
        push_reset();
      } else if (c == '#') {
        push_color(map_hl_to_color(HL_COMMENT));
        push(c);
        push_reset();
      } else {
        push_color(map_hl_to_color(HL_NORMAL));
        push(c);
        push_reset();
      }
    }
  } else if (type == SYNTAX_JUMPERLESS_CONNECTIONS) {
    // Smart connection highlighting for Jumperless node connections
    String input_str = String(string);
    String highlighted = highlightJumperlessConnections(input_str);
    
    // Copy the highlighted string to output buffer
    const char* highlighted_cstr = highlighted.c_str();
    size_t highlighted_len = highlighted.length();
    for (size_t i = 0; i < highlighted_len && pos + 1 < max_out; i++) {
      out[pos++] = highlighted_cstr[i];
    }
  } else {
    // Default: no transformation for now
    for (size_t i = 0; i < len; i++) push(string[i]);
  }

  out[pos] = '\0';
  return out;
}

// Check if character is a terminal command
bool SyntaxHighlighting::isTerminalCommand(char c) {
  switch (c) {
    case '+': case '-': case 'x': case 'c': case 'p': case 'm': case 'n':
    case 'e': case 'U': case 'u': case '/': case '~': case '>':
      return true;
    default:
      return false;
  }
}

// Get friendly name for a node (number -> name mapping)
String SyntaxHighlighting::getNodeFriendlyName(const String& input) {
  String trimmed = input;
  trimmed.trim();
  
  // Handle numeric inputs - map ONLY the actual Arduino pin node numbers
  int nodeNum = trimmed.toInt();
  if (nodeNum > 0 && trimmed == String(nodeNum)) {
    switch (nodeNum) {
      // Arduino digital pins (70-83 range) - DON'T map small numbers!
      case 70: return "D0";   case 71: return "D1";   case 72: return "D2";
      case 73: return "D3";   case 74: return "D4";   case 75: return "D5";
      case 76: return "D6";   case 77: return "D7";   case 78: return "D8";
      case 79: return "D9";   case 80: return "D10";  case 81: return "D11";
      case 82: return "D12";  case 83: return "D13";
      // Analog pins (86-93 range)
      case 86: return "A0";   case 87: return "A1";   case 88: return "A2";
      case 89: return "A3";   case 90: return "A4";   case 91: return "A5";
      case 92: return "A6";   case 93: return "A7";
      // Power and special nodes (100+ range)
      case 100: return "GND"; case 101: return "TOP_RAIL"; case 102: return "BOTTOM_RAIL";
      case 103: return "3V3"; case 105: return "5V";
      case 106: return "DAC0"; case 107: return "DAC1";
      case 108: return "ISENSE_PLUS"; case 109: return "ISENSE_MINUS";
      case 110: return "ADC0"; case 111: return "ADC1"; case 112: return "ADC2"; case 113: return "ADC3";
      default: return trimmed;  // Return as-is (breadboard rows 1-30 stay as numbers)
    }
  }
  
  // Handle common name variants
  if (trimmed == "GND" || trimmed == "GROUND") return "GND";
  if (trimmed == "5V" || trimmed == "+5V") return "5V";
  if (trimmed == "3V3" || trimmed == "3.3V") return "3V3";
  if (trimmed.startsWith("D") && trimmed.length() <= 3) return trimmed;
  if (trimmed.startsWith("A") && trimmed.length() <= 2) return trimmed;
  if (trimmed == "TOP_RAIL" || trimmed == "TOPRAIL") return "TOP_RAIL";
  if (trimmed == "BOTTOM_RAIL" || trimmed == "BOTTOMRAIL") return "BOTTOM_RAIL";
  
  return trimmed;  // Return input if no specific mapping found
}

// Check if a string is a valid node name/number
bool SyntaxHighlighting::isValidNodeName(const String& name) {
  String trimmed = name;
  trimmed.trim();
  
  // Check if it's a number in valid range
  int nodeNum = trimmed.toInt();
  if (nodeNum > 0 && trimmed == String(nodeNum)) {
    return (nodeNum >= 1 && nodeNum <= 150);  // Valid node number range
  }
  
  // Check common node name patterns
  if (trimmed.startsWith("D") || trimmed.startsWith("A") || 
      trimmed == "GND" || trimmed == "5V" || trimmed == "3V3" ||
      trimmed.startsWith("DAC") || trimmed.startsWith("ADC") ||
      trimmed.startsWith("GPIO") || trimmed.indexOf("RAIL") != -1) {
    return true;
  }
  
  return false;
}

// Highlight a single connection (node1-node2)
String SyntaxHighlighting::highlightSingleConnection(const String& connection) {
  String upper_connection = connection;
  upper_connection.toUpperCase();
  upper_connection.trim();
  
  // Look for dash in the connection
  int dash_pos = upper_connection.indexOf('-');
  if (dash_pos <= 0 || dash_pos >= upper_connection.length() - 1) {
    return connection;  // No valid dash found, return as-is
  }
  
  String before_dash = upper_connection.substring(0, dash_pos);
  String after_dash = upper_connection.substring(dash_pos + 1);
  before_dash.trim();
  after_dash.trim();
  
  // Map to friendly names and validate
  String node1_name = getNodeFriendlyName(before_dash);
  String node2_name = getNodeFriendlyName(after_dash);
  
  // Only highlight if both nodes are recognized
  if (isValidNodeName(before_dash) && isValidNodeName(after_dash)) {
    int node1_color = map_hl_to_color(HL_TERMINAL_NODE1);
    int node2_color = map_hl_to_color(HL_TERMINAL_NODE2);
    return "\x1b[38;5;" + String(node1_color) + "m" + node1_name + "\x1b[0m-\x1b[38;5;" + String(node2_color) + "m" + node2_name + "\x1b[0m";
  }
  
  return connection;  // Return original if not recognized
}

// Process comma-separated connections
String SyntaxHighlighting::processConnections(const String& connections) {
  String result = "";
  String working_input = connections;
  working_input.trim();
  
  // Process comma-separated connections
  int comma_pos = 0;
  while ((comma_pos = working_input.indexOf(',')) != -1 || working_input.length() > 0) {
    String connection_part;
    if (comma_pos != -1) {
      connection_part = working_input.substring(0, comma_pos);
      working_input = working_input.substring(comma_pos + 1);
    } else {
      connection_part = working_input;
      working_input = "";
    }
    
    connection_part.trim();
    String highlighted_part = highlightSingleConnection(connection_part);
    
    if (result.length() > 0) {
      result += ", ";
    }
    result += highlighted_part;
    
    if (working_input.length() == 0) break;
  }
  
  return result;
}


// Python syntax highlighting for > commands
String SyntaxHighlighting::highlightPythonCode(const String& code) {
  String result = "";
  int i = 0;
  
//   // Debug: Add some visual feedback that this function is being called
//   Serial.print("[DEBUG: highlightPythonCode called with: '");
//   Serial.print(code);
//   Serial.println("']");
  
  while (i < code.length()) {
    char c = code.charAt(i);
    
    // Handle strings first (highest priority)
    if (c == '"' || c == '\'') {
      int string_color = map_hl_to_color(HL_STRING);
      result += "\x1b[38;5;" + String(string_color) + "m";
      char quote = c;
      result += c;
      i++;
      // Consume entire string including the closing quote
      while (i < code.length() && code.charAt(i) != quote) {
        if (code.charAt(i) == '\\' && i + 1 < code.length()) {
          result += code.charAt(i);  // Backslash
          i++;
          if (i < code.length()) {
            result += code.charAt(i);  // Escaped char
            i++;
          }
        } else {
          result += code.charAt(i);
          i++;
        }
      }
      if (i < code.length()) {
        result += code.charAt(i); // Closing quote
        i++;
      }
      result += "\x1b[0m"; // Reset color
      continue;
    }
    
    // Handle numbers
    if (isdigit(c) || (c == '.' && i + 1 < code.length() && isdigit(code.charAt(i + 1)))) {
      int number_color = map_hl_to_color(HL_NUMBER);
      result += "\x1b[38;5;" + String(number_color) + "m";
      while (i < code.length() && (isdigit(code.charAt(i)) || code.charAt(i) == '.')) {
        result += code.charAt(i);
        i++;
      }
      result += "\x1b[0m"; // Reset color
      continue;
    }
    
    // Handle keywords and identifiers
    if (isalpha(c) || c == '_') {
      int start = i;
      while (i < code.length() && (isalnum(code.charAt(i)) || code.charAt(i) == '_')) {
        i++;
      }
      
      String word = code.substring(start, i);
      int color = 255; // Default white - don't color unknown identifiers
      
      // Check for Python keywords
      if (word == "print" || word == "def" || word == "if" || word == "else" || 
          word == "elif" || word == "for" || word == "while" || word == "import" || 
          word == "from" || word == "return" || word == "class" || word == "try" ||
          word == "except" || word == "with" || word == "as" || word == "pass" ||
          word == "True" || word == "False" || word == "None") {
        color = map_hl_to_color(HL_KEYWORD1); // Orange for keywords
      } else if (word == "len" || word == "str" || word == "int" || word == "float" || 
                 word == "list" || word == "dict" || word == "range" || word == "abs" ||
                 word == "min" || word == "max" || word == "sum") {
        color = map_hl_to_color(HL_KEYWORD2); // Green for builtins
      }
      
      if (color != 255) {
        result += "\x1b[38;5;" + String(color) + "m" + word + "\x1b[0m";
      } else {
        result += word;  // Don't color unknown words
      }
      continue;
    }
    
    // Default character (operators, whitespace, punctuation)
    result += c;
    i++;
  }
  
  return result;
}

// Smart highlighting for Jumperless connections
String SyntaxHighlighting::highlightJumperlessConnections(const String& input) {
  // DON'T trim - preserve exact spacing to maintain cursor alignment
  String working_input = input;
  
  // Check for single terminal commands first
  if (working_input.length() == 1) {
    char cmd = working_input.charAt(0);
    if (isTerminalCommand(cmd)) {
      int cmd_color = map_hl_to_color(HL_TERMINAL_FUNC);
      return "\x1b[38;5;" + String(cmd_color) + "m" + working_input + "\x1b[0m";
    }
  }
  
  // Handle commands followed by data
  if (working_input.length() > 1 && isTerminalCommand(working_input.charAt(0))) {
    String result = "";
    char cmd = working_input.charAt(0);
    int cmd_color = map_hl_to_color(HL_TERMINAL_FUNC);
    
    // Debug output
    // Serial.print("[DEBUG: Command detected: '");
    // Serial.print(cmd);
    // Serial.print("' with data: '");
    // Serial.print(working_input.substring(1));
    // Serial.println("']");
    
    // Highlight the command character
    result += "\x1b[38;5;" + String(cmd_color) + "m" + String(cmd) + "\x1b[0m";
    
    // Process the rest EXACTLY as typed (preserve all spacing)
    String command_part = working_input.substring(1);
    if (command_part.length() > 0) {
      if (cmd == '>') {
        // Python command - apply Python syntax highlighting
      //  Serial.println("[DEBUG: Processing Python command]");
        result += highlightPythonCode(command_part);
      } else {
        // Connection command - apply highlighting but preserve spacing
        //Serial.println("[DEBUG: Processing connection command]");
        result += highlightConnectionsPreserveSpaces(command_part);
      }
    }
    
    return result;
  }
  
  // Fall back to processing as pure connections with preserved spacing
  return highlightConnectionsPreserveSpaces(working_input);
}

// Highlight connections while preserving exact input spacing
String SyntaxHighlighting::highlightConnectionsPreserveSpaces(const String& input) {
  String result = "";
  int i = 0;
  
  while (i < input.length()) {
    char c = input.charAt(i);
    
    // Look for potential node patterns (numbers or names)
    if (isdigit(c) || isalpha(c)) {
      int start = i;
      // Collect the whole word/number
      while (i < input.length() && (isalnum(input.charAt(i)) || input.charAt(i) == '_')) {
        i++;
    }
    
      String token = input.substring(start, i);
      
      // Check if this is a valid node that should be highlighted
      if (isValidNodeName(token)) {
        String friendly_name = getNodeFriendlyName(token);
        int node_color = map_hl_to_color(HL_TERMINAL_NODE1);
        result += "\x1b[38;5;" + String(node_color) + "m" + friendly_name + "\x1b[0m";
      } else {
        result += token;  // Not recognized, keep as-is
      }
    } else {
      // Non-alphanumeric character - just copy as-is
      result += c;
      i++;
    }
  }
  
  return result;
}

// Helper function for Python syntax highlighting (used elsewhere)
void displayStringWithSyntaxHighlighting(const String& text, Stream* stream) {
  if (text.length() == 0) return;
  
  // Simple highlighting - can be enhanced later
  // For now just apply basic keyword highlighting
  
  String working_text = text;
  
  // Handle strings first
  int i = 0;
  while (i < working_text.length()) {
    char c = working_text.charAt(i);
    
    if (c == '"' || c == '\'') {
      stream->print("\x1b[38;5;39m");  // Cyan for strings
      char quote = c;
      stream->print(c);
      i++;
      while (i < working_text.length() && working_text.charAt(i) != quote) {
        stream->print(working_text.charAt(i));
        i++;
      }
      if (i < working_text.length()) {
        stream->print(working_text.charAt(i)); // Closing quote
        i++;
      }
      stream->print("\x1b[0m");  // Reset
      continue;
    }
    
    // Check for keywords
    if (isalpha(c) || c == '_') {
      int start = i;
      while (i < working_text.length() && (isalnum(working_text.charAt(i)) || working_text.charAt(i) == '_')) {
        i++;
      }
      
      String word = working_text.substring(start, i);
      if (word == "print" || word == "def" || word == "if" || word == "else" || word == "for" || word == "while") {
        stream->print("\x1b[38;5;214m");  // Orange for keywords
        stream->print(word);
        stream->print("\x1b[0m");
      } else {
        stream->print(word);
      }
      continue;
    }
    
    // Default character
    stream->print(c);
    i++;
  }
}
