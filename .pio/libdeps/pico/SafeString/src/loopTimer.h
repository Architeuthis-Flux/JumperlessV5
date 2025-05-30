
#ifndef LOOP_TIMER_H
#define LOOP_TIMER_H
// loopTimer.h
/*
 * (c)2019 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */

#include <Arduino.h>

// download millisDelay from https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html
#include <millisDelay.h>

/**************
  There is a predefined **loopTimer** object that is ready to use, see the detailed description. 
    
  To use the predefined **loopTimer** to measure your loop processing time, insert this code in loop()<br>
  <code>loopTimer.check(Serial);</code><br>
  
  That will print to Serial the accumulated loop times every 5seconds. Sample output is:-<br>
  <code>loop us Latency<br>
 5sec max:7276 avg:12<br>
 sofar max:7276 avg:12 max - prt:15512</code><br>
  
 The prt:15512 is an estimate of the time taken (15512us or 15.5ms) by loopTimer itself to format and print this output.<br>
 
 To time the delay between calls to particular method you can either just move the loopTimer.check(Serial); statement from loop() to that method or
 create an new instance of the **loopTimerClass** with its own name e.g. <br>
 <code>loopTimerClass stepTimer("step");</code><br>
 Then in the step() method insert<br>
 <code>stepTimer.check(Serial);</code><br>
 which will print<br>
  <code>step us Latency<br>
 5sec max:727 avg:23<br>
 sofar max:727 avg:32 max - prt:15612</code><br>
 
 You can also use <br>
 <code>loopTimer.check();</code><br>
 to just accumulate the measurements but not print them and then call<br>
 <code>loopTimer.print(Serial)</code><br>
 when you want to print them.<br>
 
 See [Simple Multitasking Arduino on any board without using an RTOS](https://www.forward.com.au/pfod/ArduinoProgramming/RealTimeArduino/index.html) for examples of using loopTimer.

****************************************************************************************/
class loopTimerClass {
  public:
    loopTimerClass(const char *_name = NULL); // name of this timer, if NULL then "loop" is used as the name
    void check(Print *out = NULL); // if loopTimer.check() called nothing is printed but timing done
    void check(Print &out); // if loopTimer.check() called nothing is printed but timing done
    void print(Print &out); // this prints the latest timings
    void print(Print *out); // this prints the latest timings
    void clear(); // clears all previous data

  private:
    void init();
    const char *name;
    bool initialized; // true after first call.
    unsigned long maxLoop5sec_us; // max in last 5 sec
    unsigned long totalLoop5sec_us; // total for last 5 sec
    unsigned long loopCount5sec;  // count in last 5 sec
    unsigned long lastLoopRun_us; // start us last call

    // print vars
    unsigned long p_avgLoop5sec_us; // last calculated 5 sec average latency
    unsigned long p_maxLoop5sec_us; // last max value in 5 sec

    unsigned long p_maxLoop_us;  // max so far update every 5 sec
    unsigned long p_maxAvgLoop_us; // max avg so far , updated every 5

    unsigned long PRINT_US_DELAY = 5000; // ms calculate and print every 5 sec
    millisDelay print_us_Delay;
};

static loopTimerClass loopTimer;
#endif // LOOP_TIMER_H
