// RP23V50firmware/src/TuiGlue.h
#pragma once




namespace TuiGlue {


  extern Stream* TUIserial;


  void init();
  void openOnDemand();
  bool isActive();
  void loop();
}
