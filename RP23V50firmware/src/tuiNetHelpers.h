#pragma once
#include <Arduino.h>
#include <stdlib.h>


/*
  tuiNetHelpers:
  - Parses "N-N;N-N;..." where N can be a number or a well-known label (e.g., GND, TOP_RAIL, 3V3, D10, A3, DAC0, UART_TX, etc.)
  - Allows cross-bank and shared endpoints.
  - Disallows duplicates in the same input.
  - Enforces "forbidden" pairs (e.g., rails vs. GND/DAC/GPIO, supplies vs. GND, DAC0 vs. DAC1, etc.).
  - Provides toLabel() to print canonical names in logs regardless of input spelling.
*/
class tuiNetHelpers {
  
public:
  enum : uint8_t { NET_MIN = 1, NET_MAX = 140, MAX_NET_PAIRS = 64 };

  enum Status : uint8_t {
    OK = 0,
    ERR_EMPTY_SEGMENT,
    ERR_MISSING_FIRST,
    ERR_EXPECT_DASH,
    ERR_MISSING_SECOND,
    ERR_RANGE,
    ERR_EQUAL_ENDPOINTS,
    ERR_DUPLICATE,
    ERR_TRAILING_JUNK,
    ERR_TOO_MANY,
    ERR_FORBIDDEN
  };

  struct Endpoint {
    uint8_t from;    
    uint8_t to;      
    uint8_t index;   
    Status  status;  // OK if usable
    String  message; // empty if OK
  };

  struct Result {
    uint8_t total = 0;
    uint8_t ok = 0;
    uint8_t errors = 0;
    bool    truncated = false;
    Endpoint items[MAX_NET_PAIRS];
  };

  // ---------- Public API ----------

  // Parse input text into a Result structure.
  // Input format: "N-N;N-N;N-N" where N is a number or a known label (e.g., GND, TOP_RAIL, 3V3, D10, A3, DAC0, UART_TX, etc.)
  // Whitespace is ignored. Segments are separated by ';' or ','.
  // Returns a Result with parsed items, status codes, and error messages.
  // Stops at MAX_NET_PAIRS items, marking truncated=true if exceeded.
  // See Status enum for error codes.
  static inline Result parse(const String& input) {
    Result r;

    String s = input;
    s.trim();
    // normalize common dash variants to '-'
    s.replace("–", "-");
    s.replace("—", "-");

    const char* p = s.c_str();
    uint8_t pairIdx = 1;

    while (true) {
      skipWS(p);

      if (*p == '\0')
        break;

      if (*p == ';' || *p == ',') {

        pushErr(r, pairIdx, ERR_EMPTY_SEGMENT, "empty segment.");
        ++p; ++pairIdx;
        continue;

      }

      if (r.total >= MAX_NET_PAIRS) {

        r.truncated = true;
        pushErr(r, pairIdx, ERR_TOO_MANY, "too many pairs.");
        break;

      }

      Endpoint ep{0,0,pairIdx,OK,""};
      long a, b;

      if (!parseOneTokenToUint(p, a)) {
        
        pushErr(r, pairIdx, ERR_MISSING_FIRST, "missing first number/label.");
        recoverToNextSep(p);
        ++pairIdx;
        continue;

      }

      if (*p != '-') {
        
        pushErr(r, pairIdx, ERR_EXPECT_DASH, "use A-B format.");
        recoverToNextSep(p);
        ++pairIdx;
        continue;

      }

      ++p;

      if (!parseOneTokenToUint(p, b)) {

        pushErr(r, pairIdx, ERR_MISSING_SECOND, "missing second number/label.");
        recoverToNextSep(p);
        ++pairIdx;
        
        continue;
      }

      // range check after mapping
      if (!inRange(a) || !inRange(b)) {

        ep.status = ERR_RANGE;
        ep.message = msgOutOfRange(pairIdx);

      } else if (a == b) {

        ep.status = ERR_EQUAL_ENDPOINTS;
        ep.message = msgPairWithWords(pairIdx, "endpoints must differ: ", (uint8_t)a, (uint8_t)b);

      } else {
        
        if (a > b) { 
          long t = a; 
          a = b; 
          b = t; 
        }

        ep.from = (uint8_t)a; 
        ep.to = (uint8_t)b;

        // duplicates within the same parsed batch
        if (containsDuplicate(r, ep.from, ep.to)) {

          ep.status = ERR_DUPLICATE;
          ep.message = msgPairWithWords(pairIdx, "duplicate ", ep.from, ep.to);

        } else if (isForbiddenPair(ep.from, ep.to)) {

          ep.status = ERR_FORBIDDEN;
          ep.message = msgPairWithWords(pairIdx, "forbidden connection ", ep.from, ep.to);

        }
        
      }

      skipWS(p);
      if (*p && *p!=';' && *p!=',') {
        ep.status = ERR_TRAILING_JUNK;
        ep.message = msgChar(pairIdx, "unexpected '", *p, "'. Use ';' to separate.");
        recoverToNextSep(p);
      }

      push(r, ep);

      if (*p == ';' || *p == ',') {
        ++p; skipWS(p);
        if (*p == '\0') {
          pushErr(r, (uint8_t)(pairIdx + 1), ERR_EMPTY_SEGMENT, "empty segment (trailing separator).");
          ++pairIdx;
          break;
        }
      }

      ++pairIdx;
    }

    return r;
  }

  // Convert a numeric node to its canonical readable label for logs/UI.
  // Numeric breadboard nets 1..60 (and 70.. etc.) are returned as their standard names.
  static inline String toLabel(uint8_t v) {
    // 1..60 plain numbers
    if (v >= 1 && v <= 60) {
      String s; s += v; return s;
    }

    // Arduino-style D0..D13 (70..83)
    if (v >= 70 && v <= 83) {
      String s("D"); s += (v - 70); return s;
    }

    // A0..A7 (86..93)
    if (v >= 86 && v <= 93) {
      String s("A"); s += (v - 86); return s;
    }

    switch (v) {
      case 84: return "RESET";
      case 85: return "AREF";

      // Special rails & supplies / instrumentation
      case 100: return "GND";
      case 101: return "TOP_RAIL";
      case 102: return "BOTTOM_RAIL";
      case 103: return "3V3";
      case 105: return "5V";
      case 120: return "8V_P";
      case 121: return "8V_N";

      // DAC / Current sense / ADC / Probe / UART
      case 106: return "DAC0";
      case 107: return "DAC1";
      case 108: return "I_P";
      case 109: return "I_N";
      case 110: return "ADC0";
      case 111: return "ADC1";
      case 112: return "ADC2";
      case 113: return "ADC3";
      case 114: return "ADC4";
      case 115: return "PROBE_MEASURE";
      case 116: return "UART_TX";
      case 117: return "UART_RX";

      // Extra GPIO-ish
      case 118: return "GPIO_18";
      case 119: return "GPIO_19";

      // RP GPIO (131..138)
      case 131: return "GPIO_1";
      case 132: return "GPIO_2";
      case 133: return "GPIO_3";
      case 134: return "GPIO_4";
      case 135: return "GPIO_5";
      case 136: return "GPIO_6";
      case 137: return "GPIO_7";
      case 138: return "GPIO_8";

      case 139: return "BUFFER_IN";
      case 140: return "BUFFER_OUT";

      // Misc
      case 127: return "EMPTY_NET";
      case 94:  return "RESET_0";
      case 95:  return "RESET_1";
    }

    // fallback to number
    String s; s += v; return s;
  }

private:
  // ---------- token parsing ----------

  static inline void skipWS(const char*& p) {
    while (*p==' '||*p=='\t') ++p;
  }

  static inline void recoverToNextSep(const char*& p) {
    while (*p && *p!=';' && *p!=',') ++p;
    skipWS(p);
  }

  static inline bool inRange(long v) {
    return v >= NET_MIN && v <= NET_MAX;
  }

  // read either a decimal number OR an identifier (word) and map to uint
  static inline bool parseOneTokenToUint(const char*& p, long& out) {
    skipWS(p);
    if (*p == '\0') return false;

    // numeric?
    if (isDigit(*p)) {
      char* end = nullptr;
      long v = strtol(p, &end, 10);
      if (end == p) return false;
      p = end; skipWS(p);
      out = v;
      return true;
    }

    // identifier (letters/digits/underscore/plus/dot) up to a separator or dash
    const char* start = p;
    while (*p && *p!='-' && *p!=';' && *p!=',' && *p!=' ' && *p!='\t')
      ++p;

    if (p == start) return false;

    String token = String(start).substring(0, (int)(p - start));
    token.trim();

    long mapped;
    if (!wordToNumber(token, mapped))
      return false;

    skipWS(p);
    out = mapped;
    return true;
  }

  static inline bool isAlphaNumUnderscorePlusDot(char c) {
    return (c=='_' || c=='+' || c=='.' ||
            (c>='0'&&c<='9') || (c>='A'&&c<='Z') || (c>='a'&&c<='z'));
  }

  // Case-insensitive equality
  static inline bool eqi(const String& a, const char* b) {
    return a.equalsIgnoreCase(b);
  }

  // Normalize token: uppercase, strip spaces, translate common variants
  static inline String norm(const String& in) {
    String t = in;
    t.trim();

    // uppercase
    for (int i=0;i<t.length();++i) {
      char c = t[i];
      if (c >= 'a' && c <= 'z') t.setCharAt(i, c - 32);
    }

    // remove spaces
    t.replace(" ", "");

    // unify some punctuation
    t.replace("-", "_");

    // normalize + and dots for volt strings
    t.replace("+", "");
    t.replace(".", "");

    return t;
  }

  // Map well-known words (and families) to their numeric IDs
  static inline bool wordToNumber(const String& raw, long& out) {
    String w = norm(raw);

    // Common rails / ground / supplies
    if (eqi(w, "GND") || eqi(w,"GROUND")) { out = 100; return true; }

    if (eqi(w, "TOP_RAIL") || eqi(w,"TOPRAIL") || eqi(w,"TR") || eqi(w,"T_R") || eqi(w,"TOP_R")) { out = 101; return true; }
    if (eqi(w, "BOTTOM_RAIL") || eqi(w,"BOT_RAIL") || eqi(w,"BOTTOMRAIL") || eqi(w,"BOTRAIL") || eqi(w,"BR") || eqi(w,"B_R") || eqi(w,"BOT_R")) { out = 102; return true; }

    if (eqi(w,"3V3") || eqi(w,"33V") || eqi(w,"SUPPLY_3V3")) { out = 103; return true; }
    if (eqi(w,"5V")  || eqi(w,"SUPPLY_5V")) { out = 105; return true; }
    if (eqi(w,"8V_P") || eqi(w,"8VP")) { out = 120; return true; }
    if (eqi(w,"8V_N") || eqi(w,"8VN")) { out = 121; return true; }

    // DAC
    if (eqi(w,"DAC0") || eqi(w,"DAC_0") || eqi(w,"DAC05V")) { out = 106; return true; }
    if (eqi(w,"DAC1") || eqi(w,"DAC_1") || eqi(w,"DAC18V")) { out = 107; return true; }

    // Current sense
    if (eqi(w,"I_P") || eqi(w,"INAP") || eqi(w,"ISENSEPLUS") || eqi(w,"ISENSEPOSITIVE") || eqi(w,"ISENSEPOS") || eqi(w,"ISENSEP")) { out = 108; return true; }
    if (eqi(w,"I_N") || eqi(w,"INAN") || eqi(w,"ISENSEMINUS") || eqi(w,"ISENSENEGATIVE") || eqi(w,"ISENSENEG") || eqi(w,"ISENSEN")) { out = 109; return true; }

    // ADC / Probe
    if (eqi(w,"ADC0") || eqi(w,"ADC_0") || eqi(w,"ADC08V")) { out = 110; return true; }
    if (eqi(w,"ADC1") || eqi(w,"ADC_1") || eqi(w,"ADC18V")) { out = 111; return true; }
    if (eqi(w,"ADC2") || eqi(w,"ADC_2")) { out = 112; return true; }
    if (eqi(w,"ADC3") || eqi(w,"ADC_3")) { out = 113; return true; }
    if (eqi(w,"ADC4") || eqi(w,"ADC_4") || eqi(w,"ADC45V")) { out = 114; return true; }
    if (eqi(w,"PROBE_MEASURE") || eqi(w,"PROBEMEASURE")) { out = 115; return true; }

    // UART
    if (eqi(w,"UART_TX") || eqi(w,"RPUARTTX") || eqi(w,"TX")) { out = 116; return true; }
    if (eqi(w,"UART_RX") || eqi(w,"RPUARTRX") || eqi(w,"RX")) { out = 117; return true; }

    // Buffer
    if (eqi(w,"BUFFER_IN") || eqi(w,"BUF_IN") || eqi(w,"BUFF_IN") || eqi(w,"BUFFIN")) { out = 139; return true; }
    if (eqi(w,"BUFFER_OUT")|| eqi(w,"BUF_OUT")|| eqi(w,"BUFF_OUT")|| eqi(w,"BUFFOUT")) { out = 140; return true; }

    // Empty-net marker
    if (eqi(w,"EMPTY_NET") || eqi(w,"EMPTYNET")) { out = 127; return true; }

    // Specific GPIO_18 / GPIO_19 before the generic GPIO_#
    if (eqi(w,"GPIO_18") || eqi(w,"GPIO18")) { out = 118; return true; }
    if (eqi(w,"GPIO_19") || eqi(w,"GPIO19")) { out = 119; return true; }

    // RP GPIO_1..8 families
    if (w.startsWith("GPIO_") || w.startsWith("GPIO") || w.startsWith("GP_") || w.startsWith("GP") || w.startsWith("RPGPIO_") || w.startsWith("RPGPIO")) {
      int idx = -1;
      // extract trailing number
      for (int i=(int)w.length()-1; i>=0; --i) {
        if (w[i]<'0'||w[i]>'9') { // first non-digit from the end
          String num = w.substring(i+1);
          if (num.length()>0) idx = num.toInt();
          break;
        }
      }
      if (idx >= 1 && idx <= 8) { out = (130 + idx); return true; }  // 131..138
    }

    // D0..D13
    if (w.startsWith("D")) {
      int n = w.substring(1).toInt();
      if (n >= 0 && n <= 13) { out = (70 + n); return true; }
    }

    // A0..A7
    if (w.startsWith("A")) {
      int n = w.substring(1).toInt();
      if (n >= 0 && n <= 7) { out = (86 + n); return true; }
    }

    // RESET/AREF aliases
    if (eqi(w,"RESET")) { out = 84; return true; }
    if (eqi(w,"AREF"))  { out = 85; return true; }

    // RESET_0 / RESET_1
    if (eqi(w,"RESET_0")) { out = 94; return true; }
    if (eqi(w,"RESET_1")) { out = 95; return true; }

    return false;
  }

  // ---------- policy: forbidden pairs ----------

  static inline bool isGND(uint8_t n) { return n == 100; }
  static inline bool isRail(uint8_t n){ return n == 101 || n == 102; }       // TOP_RAIL, BOTTOM_RAIL
  static inline bool isSupply(uint8_t n){ return n == 103 || n == 105 || n == 120 || n == 121; } // 3V3, 5V, 8V_P, 8V_N
  static inline bool isDAC(uint8_t n){ return n == 106 || n == 107; }        // DAC0, DAC1
  static inline bool isRpGpio(uint8_t n){ return n >= 131 && n <= 138; }     // GPIO_1..GPIO_8

  static inline bool isForbiddenPair(uint8_t a, uint8_t b) {
    // symmetric
    const uint8_t x = (a < b) ? a : b;
    const uint8_t y = (a < b) ? b : a;

    // GND must not connect to rails/supplies/DACs
    if (isGND(x) && (isRail(y) || isSupply(y) || isDAC(y))) return true;

    // Rails may not connect to each other, to DACs, or to RP GPIO_1..8
    if (isRail(x) && isRail(y)) return true;                  // TOP_RAIL vs BOTTOM_RAIL
    if (isRail(x) && isDAC(y)) return true;
    if (isRail(x) && isRpGpio(y)) return true;

    // DAC0 vs DAC1 not allowed, and DAC vs RP GPIO_1..8 not allowed
    if (isDAC(x) && isDAC(y)) return true;
    if (isDAC(x) && isRpGpio(y)) return true;

    // (extend here if more old rules get resurrected)
    return false;
  }

  // ---------- bookkeeping helpers ----------

  static inline void push(Result& r, const Endpoint& e) {
    if (r.total < MAX_NET_PAIRS) {
      r.items[r.total++] = e;
      if (e.status == OK) ++r.ok; else ++r.errors;
    } else {
      r.truncated = true;
    }
  }

  static inline void pushErr(Result& r, uint8_t idx, Status st, const char* text) {
    Endpoint e{0,0,idx,st, msg(idx, text)};
    push(r, e);
  }

  static inline bool containsDuplicate(const Result& r, uint8_t a, uint8_t b) {
    for (uint8_t i=0;i<r.total;++i)
      if (r.items[i].status==OK && r.items[i].from==a && r.items[i].to==b)
        return true;
    return false;
  }

  // ---------- message builders (use words) ----------

  static inline String msg(uint8_t idx, const char* text) {
    String m; m.reserve(64);
    m += "Pair #"; m += idx; m += ": "; m += text;
    return m;
  }

  static inline String msgOutOfRange(uint8_t idx) {
    String m; m.reserve(64);
    m += "Pair #"; m += idx; m += ": out of range; expected ";
    m += NET_MIN; m += "-"; m += NET_MAX; m += ".";
    return m;
  }

  static inline String msgPairWithWords(uint8_t idx, const char* prefix, uint8_t a, uint8_t b) {
    String m; m.reserve(64);
    m += "Pair #"; m += idx; m += ": "; m += prefix;
    m += toLabel(a); m += "-"; m += toLabel(b); m += ".";
    return m;
  }

  static inline String msgChar(uint8_t idx, const char* prefix, char ch, const char* suffix) {
    String m; m.reserve(64);
    m += "Pair #"; m += idx; m += ": "; m += prefix; m += ch; m += suffix;
    return m;
  }
};
