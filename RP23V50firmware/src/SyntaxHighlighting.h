#ifndef SYNTAXHIGHLIGHTING_H
#define SYNTAXHIGHLIGHTING_H

#include <Arduino.h>

#define TERM_COLOR_SUPPORT 256

#define HL_NORMAL 0
#define HL_COMMENT 1
#define HL_MLCOMMENT 2
#define HL_KEYWORD1 3
#define HL_KEYWORD2 4
#define HL_STRING 5
#define HL_NUMBER 6
#define HL_MATCH 7
#define HL_JUMPERLESS_FUNC 8
#define HL_JUMPERLESS_TYPE 9
#define HL_JFS_FUNC 10
#define HL_TERMINAL_FUNC 11
#define HL_TERMINAL_NUMBER 12
#define HL_TERMINAL_NODE1 13
#define HL_TERMINAL_NODE2 14
#define HL_TERMINAL_STRING 15

enum SyntaxHighlightingType {
  SYNTAX_PYTHON,
  SYNTAX_JUMPERLESS,
  SYNTAX_JFS,
  SYNTAX_TERMINAL,
  SYNTAX_JUMPERLESS_CONNECTIONS,
  SYNTAX_DEFAULT,
};

class SyntaxHighlighting {
  public:
    SyntaxHighlighting(Stream* stream);
    ~SyntaxHighlighting();
    void syntaxHighlighting();

    Stream* syntaxHighlightingStream;

    void syntaxHighlightingToColor(int hl);
    char* highlightString(const char* string, enum SyntaxHighlightingType type);
    void setStream(Stream* stream);
    String highlightJumperlessConnections(const String& input); // Smart connection highlighting
    
private:
    // Helper methods for smart highlighting
    bool isTerminalCommand(char c);
    String highlightSingleConnection(const String& connection);
    String getNodeFriendlyName(const String& input);
    bool isValidNodeName(const String& name);
    String processConnections(const String& connections); // Process comma-separated connections  
    String highlightConnectionsPreserveSpaces(const String& input); // Simple space-preserving highlighting
    String highlightPythonCode(const String& code); // Python syntax highlighting
};

void displayStringWithSyntaxHighlighting(const String& text, Stream* stream);




















#endif