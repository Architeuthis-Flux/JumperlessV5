#include "Tui.h"
#include "TuiGlue.h"
#include <Wire.h>
#include "ArduinoStuff.h"
#include "LogicAnalyzer.h"
#include "oled.h"




// -----------------------------------------------------------------------------
// callbacks 
// -----------------------------------------------------------------------------
static void cbI2CScan() {
  auto scanOne = [](TwoWire& bus, const char* tag, uint32_t hz)->uint8_t {
    bus.begin(); bus.setClock(hz);
    uint8_t found = 0;
    TUI::log(String("Scanning ") + tag + " @ " + (int)(hz/1000) + "kHz …");
    for (uint8_t addr = 1; addr < 127; ++addr) {
      bus.beginTransmission(addr);
      uint8_t err = bus.endTransmission();
      if (err == 0) {
        char buf[8]; snprintf(buf, sizeof(buf), "0x%02X", addr);
        TUI::log(String("  ") + tag + ": device at " + buf);
        ++found;
      } else if (err == 4) {
        char buf[8]; snprintf(buf, sizeof(buf), "0x%02X", addr);
        TUI::log(String("  ") + tag + ": unknown error at " + buf);
      }
      delay(2);
    }

    if (!found) 
      TUI::log(String("  ") + tag + ": no devices.");

    return found;
  };

  TUI::setStatus("I2C scan…");
  TUI::log("---------------- I2C Scan ----------------");
  uint8_t f0 = scanOne(Wire , "I2C0/Wire" , 100000);
  uint8_t f1 = scanOne(Wire1, "I2C1/Wire1", 100000);
  uint8_t total = (uint8_t)(f0 + f1);

  if (!total) { TUI::log("Result: no I2C devices found."); TUI::setStatus("I2C: none."); }
  else        { TUI::log(String("Result: ") + (int)total + " device(s) total."); TUI::setStatus(String("I2C: ") + (int)total); }
  TUI::log("------------------------------------------");
}

static void cbLogicAnalyzer() { 
  TUI::setStatus("Logic analyzer (demo)…"); 
  TUI::log("LA: would arm capture and stream logs."); 
}

static void cbFlashArduino()  { 
  TUI::setStatus("Flashing (demo)…"); 
  for (int i=0;i<=100;i+=25) { 
    TUI::log(String("Flash: ")+i+"%"); 

  delay(10);} 
  TUI::setStatus("Flash done (demo)."); 
}

static void cbToggleOLED() {
  if (oled.isConnected() == false) { 
  oled.init();
  } else {
    oled.disconnect();
  }

  delay(100);

  
  oled.clearPrintShow("OLED " + String(TUI::oledEnabled ? "ON" : "OFF"), 1, true, true, true, -1, -1, 1500);


  TUI::setOledEnabled(!TUI::oledEnabled);
  TUI::setStatus(TUI::oledEnabled ? "OLED ON" : "OLED OFF");
}


static void cbAbout()  { 
  TUI::setStatus("About"); 
  TUI::log("Jumperless RP2350B – DOS-style TUI (demo)"); 
}









static void cbResizeToTerminal() {
  uint16_t r=0,c=0;
  if (TUI::probeTerminalSize(r,c)) { 
    TUI::resizeTo(r,c); 
    TUI::fullRedraw(); 
    TUI::log(String("Resized to ")+r+"x"+c+"."); 
  }
  else TUI::log("Resize failed: terminal didn't answer CPR.");
}

bool lastDTR = 0;
static void checkUSBconnection(){

  if (Ser3.dtr() != lastDTR) {
    lastDTR = Ser3.dtr();
   
      Serial.println("USB connection lost");
   

  TUI::fullRedraw();
  }
//  cbResizeToTerminal();
}


static void cbMenuNarrower(){ 
  TUI::L.leftFracWide = max(0.10f, TUI::L.leftFracWide - 0.02f); 
  TUI::resizeTo(TUI::S.rows, TUI::S.cols); 
  TUI::fullRedraw(); 
}

static void cbMenuWider()   { 
  TUI::L.leftFracWide = min(0.80f, TUI::L.leftFracWide + 0.02f); 
  TUI::resizeTo(TUI::S.rows, TUI::S.cols); 
  TUI::fullRedraw(); 
}

static void cbTightenGaps() { 
  if (TUI::L.margin) 
    TUI::L.margin--; 
  if (TUI::L.gap) 
    TUI::L.gap--; 
  TUI::resizeTo(TUI::S.rows, TUI::S.cols); 
  TUI::fullRedraw(); 
  
  }
static void cbLoosenGaps()  { 
  TUI::L.margin++; 

  TUI::L.gap++; 
  TUI::resizeTo(TUI::S.rows, TUI::S.cols); 
  TUI::fullRedraw(); 
}

static void cbScreenResetLayout() {
  TUI::L.leftFracWide   = 0.15f;
  TUI::L.leftFracNarrow = 0.50f;
  TUI::L.margin = 2;
  TUI::L.gap    = 2;
  TUI::resizeTo(TUI::S.rows, TUI::S.cols);
  TUI::fullRedraw();
  TUI::log("Screen layout reset.");
}

// ---------------- Menus ----------------

// --- Settings / Screen / Borders submenu ---
  // ----- Border style helpers/callbacks --------------------------------
static const char* borderStyleName(TUI::BorderStyle s) {
  switch (s) {
    case TUI::BORDER_ASCII:   return "ASCII";
    case TUI::BORDER_LIGHT:   return "Light";
    case TUI::BORDER_ROUNDED: return "Rounded";
    case TUI::BORDER_HEAVY:   return "Heavy";
    case TUI::BORDER_DOUBLE:  return "Double";
    default:                  return "?";
  }
}

static void applyBorder(TUI::BorderStyle s) {
  TUI::setBorderStyle(s);  // does fullRedraw()
  TUI::log(String("Border → ") + borderStyleName(s));
  TUI::setStatus(String("Border: ") + borderStyleName(s));
}


static void cbBorderCycle() {
  using namespace TUI;
  BorderStyle s = THEME.borderStyle;
  switch (s) {
    case BORDER_ASCII:   s = BORDER_LIGHT;   break;
    case BORDER_LIGHT:   s = BORDER_ROUNDED; break;
    case BORDER_ROUNDED: s = BORDER_HEAVY;   break;
    case BORDER_HEAVY:   s = BORDER_DOUBLE;  break;
    case BORDER_DOUBLE:  s = BORDER_ASCII;   break;
    default:             s = BORDER_LIGHT;   break;
  }
  applyBorder(s);
}

static void cbBorderAscii()   { applyBorder(TUI::BORDER_ASCII); }
static void cbBorderLight()   { applyBorder(TUI::BORDER_LIGHT); }
static void cbBorderRounded() { applyBorder(TUI::BORDER_ROUNDED); }
static void cbBorderHeavy()   { applyBorder(TUI::BORDER_HEAVY); }
static void cbBorderDouble()  { applyBorder(TUI::BORDER_DOUBLE); }


static const TUI::MenuItem SETTINGS_SCREEN_BORDER_ITEMS[] = {
  {"&Light  (┌─┐ │ │ └─┘)",   cbBorderLight},
  {"&Rounded(╭─╮ │ │ ╰─╯)",   cbBorderRounded},
  {"&Heavy  (┏━┓ ┃ ┃ ┗━┛)",   cbBorderHeavy},
  {"&Double (╔═╗ ║ ║ ╚═╝)",   cbBorderDouble},
  {"&ASCII  (+-+ | | +-+)",   cbBorderAscii},
  {"C&ycle styles",            cbBorderCycle},
};
static const uint8_t SETTINGS_SCREEN_BORDER_ITEMS_N = TUI_ARRLEN(SETTINGS_SCREEN_BORDER_ITEMS);


static const TUI::MenuItem SETTINGS_SCREEN_ITEMS[] = {
  TUI_SUBMENU_ENTRY("&Borders", SETTINGS_SCREEN_BORDER_ITEMS, "Settings / Screen / Borders"),
  {"Resize to Terminal", cbResizeToTerminal},
  {"Menu Narrower",      cbMenuNarrower},
  {"Menu Wider",         cbMenuWider},
  {"Tighten Gaps",       cbTightenGaps},
  {"Loosen Gaps",        cbLoosenGaps},
  {"Reset Layout",       cbScreenResetLayout},
};
static const uint8_t SETTINGS_SCREEN_ITEMS_N = TUI_ARRLEN(SETTINGS_SCREEN_ITEMS);


static const TUI::MenuItem SETTINGS_ITEMS[] = {
  TUI_SUBMENU_ENTRY("Screen", SETTINGS_SCREEN_ITEMS, "Settings / Screen"),
  {"Toggle OLED", cbToggleOLED},
  {"About",       cbAbout},
};
static const uint8_t SETTINGS_ITEMS_N = TUI_ARRLEN(SETTINGS_ITEMS);

static const TUI::MenuItem MAIN_ITEMS[] = {
  {"I2C Scanner",    cbI2CScan},
  {"Logic Analyzer", cbLogicAnalyzer},
  {"Flash Arduino",  cbFlashArduino},
  TUI_SUBMENU_ENTRY("Se&ttings", SETTINGS_ITEMS, "Settings"),
};

// ---------------- Glue ----------------
namespace TuiGlue {
  static bool s_modelBuilt = false;
  static bool s_active     = false;
  static bool     s_needDeferredRedraw = false;
  static uint32_t s_deferredAt = 0;

  void init() {
    if (s_modelBuilt) return;
    s_modelBuilt = true;
    
    TUI::S.appTitle = "Jumperless V5 – RP2350B";
    TUI::S.status   = "Ready.";
  
    TUI::pushMenu(MAIN_ITEMS, (uint8_t)TUI_ARRLEN(MAIN_ITEMS), "Main");
  }

  static void activate() {
  if (!s_modelBuilt) 
    init();

  // Give host a moment so CPR works first try
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1000) { delay(10); }

  uint16_t r=0,c=0;
  bool got = TUI::probeTerminalSize(r,c);        // <—
  if (got) TUI::resizeTo(r,c);

  s_active = true;

  TUI::fullRedraw();                              // first frame now

  // schedule an automatic re-probe if the first one missed
  if (!got) {
    s_needDeferredRedraw = true;                  // <—
    s_deferredAt = millis();
  }

  TUI::log("Use \x1b[1mUp/Down\x1b[0m to navigate, \x1b[1mEnter\x1b[0m to select, \x1b[1mESC\x1b[0m to go back.");

  // You can keep your 60ms second-chance; the flag handles it too.
}


  void openOnDemand() { activate(); }
  bool isActive() { return s_active; }

  void loop() {

    if (!s_modelBuilt) 
      TuiGlue::init();   // build the model
  
    if (!s_active)
      activate();    
    
    if (!s_active) 
      return;

    checkUSBconnection();

    if (TUIserial->available() && TUIserial->peek() == 0x04) {
      TUIserial->read();
      uint8_t m = (TUI::getDebugMode() + 1) % 3;
      TUI::setDebugMode(m);
      TUI::log(String("[DBG] mode -> ") + (int)m + (m==1 ? " (silent)" : m==2 ? " (visible)" : " (off)"));
    }

    uint16_t spins = 0;
    while ((TUIserial->available() || TUI::inEscapeSeq()) && spins < 2) {
      (void)TUI::handleInput();
      spins++;
      if ((spins & 0x07) == 0) delayMicroseconds(250);
    }


    // one-shot automatic re-probe/repaint in case the first CPR missed
    if (s_needDeferredRedraw && millis() - s_deferredAt > 80) {
      uint16_t rr=0, cc=0;
      if (TUI::probeTerminalSize(rr, cc)) {
        TUI::resizeTo(rr, cc);
        TUI::fullRedraw();
        s_needDeferredRedraw = false;
      } else {
        s_deferredAt = millis();  // try again next tick until it works
      }
    }


    if (TUI::S.logDirty) TUI::drawLog();


   // delay(100);

    TUIserial->flush();
  }



} // namespace TuiGlue
