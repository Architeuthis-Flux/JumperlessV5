#pragma once
#include <Arduino.h>


// Signature every user function must implement.
// argv[0] is the function name; argv[1..argc-1] are args/tokens.
using UserFn = void (*)(int argc, String argv[]);

struct UserFunction {
  const char*   name;   // function name, e.g. "i2cScanApp"
  UserFn        fn;     // function handler
  const char*   help;   // short help text (optional)
};


// ---- Provided by user_functions.cpp ----
extern const UserFunction USER_FUNCTIONS[];
extern const size_t      USER_FUNCTIONS_COUNT;


// ---- Helpers  ----
int  uf_tokenize(const String& line, String argv[], int maxTokens = 20);

// Option parsing conveniences (work on argv array)
bool uf_getOpt(int argc, String argv[], const String& key, String& outVal); // --key val | --key=val | -k val
bool uf_hasFlag(int argc, String argv[], const String& key);                // --verbose | -v
long uf_getOptInt(int argc, String argv[], const String& key, long deflt = 0);

// Dispatch one command line. Returns true if a command matched and ran.
bool uf_dispatch(const String& line);

// Lookup by name (exact match)
const UserFunction* uf_find(const String& name);
void handleUserFunction();