// loopTimer.cpp
/*
 * (c)2019 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
#include "loopTimer.h"

// name of this timer, if NULL then "loop" is used as the name
loopTimerClass::loopTimerClass(const char * _name) {
  initialized = false;
  name = _name;
  init();
}

void loopTimerClass::check(Print &out) {
	check(&out);
}

// if loopTimer.check() called nothing is printed each 5sec but timing done
void loopTimerClass::check(Print *out) {
  uint32_t us = micros(); // stop timing <<<<<<<<<<<<<<<<
  if (initialized) {
    // not first time
    uint32_t d_us = us - lastLoopRun_us;
    if (d_us > maxLoop5sec_us) {
      maxLoop5sec_us = d_us;
    }
    loopCount5sec++;
    totalLoop5sec_us += d_us;
  } else {
    init();
  }

  // every 5 sec do the calcs
  if (print_us_Delay.justFinished()) {
    print_us_Delay.restart(); // this may drift
    // calculate print vars
    p_avgLoop5sec_us = totalLoop5sec_us / loopCount5sec; // last calculated 5 sec average latency
    p_maxLoop5sec_us = maxLoop5sec_us; // last max value in 5 sec
    if (maxLoop5sec_us > p_maxLoop_us) {
      p_maxLoop_us = maxLoop5sec_us;  // max so far update every 5 sec
    }
    if (p_avgLoop5sec_us > p_maxAvgLoop_us) {
      p_maxAvgLoop_us = p_avgLoop5sec_us; // max avg so far , updated every 5
    }
    maxLoop5sec_us = loopCount5sec = totalLoop5sec_us = 0; // clear for next 5 sec
    print(out); // print results if out != NULL
  }

  // ignore time spent checking and printing above
  lastLoopRun_us = micros(); // start timing again <<<<<<<<<<<<<<<<
}

void loopTimerClass::print(Print &out) {
	print(&out);
}

// this prints the latest timings
void loopTimerClass::print(Print * out) {
  if (out != NULL) {
  	unsigned long us = micros();  
    if (name) {
      out->print(name);
    } else {
      out->print("loop");
    }
    out->println(" us Latency");
    out->print(" 5sec max:"); out->print(p_maxLoop5sec_us); out->print(" avg:"); out->print(p_avgLoop5sec_us);
    out->println();
    out->print(" sofar max:"); out->print(p_maxLoop_us); out->print(" avg:"); out->print(p_maxAvgLoop_us);
    out->print(" max - prt:");
    // skip the print time
    out->println((micros() - us));
    lastLoopRun_us += (micros() - us) ; // move start of last loop in by this amount
  }
}

void loopTimerClass::clear() {
	initialized = false;
}

void loopTimerClass::init() {
  // initialize
  maxLoop5sec_us = 0; // max in last 5 sec
  totalLoop5sec_us = 0; // total for last 5 sec
  loopCount5sec = 0;  // count in last 5 sec
  lastLoopRun_us = 0; // start us last call
  // print vars
  p_avgLoop5sec_us = 0; // last calculated 5 sec average latency
  p_maxLoop5sec_us = 0; // last max value in 5 sec
  p_maxLoop_us = 0;  // max so far update every 5 sec
  p_maxAvgLoop_us = 0; // max avg so far , updated every 5
  PRINT_US_DELAY = 5000;
  print_us_Delay.start(PRINT_US_DELAY);
  initialized = true;
}
