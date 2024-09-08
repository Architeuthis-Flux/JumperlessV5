// millisDelay.h
// see the tutorial https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html

#ifndef MILLIS_DELAY_H
#define MILLIS_DELAY_H

/*
 * (c)2018 Forward Computing and Control Pty. Ltd.
 * NSW Australia, www.forward.com.au
 * This code is not warranted to be fit for any purpose. You may only use it at your own risk.
 * This generated code may be freely used for both private and commercial use
 * provided this copyright is maintained.
 */
 
/**************
  **millisDelay** implements a non-blocking, repeatable delay, see the detailed description. 
    
  To use **millisDelay**, create a global instance for each delay you need e.g.<br>
  <code>millisDelay ledDelay;</code><br>
  Then start the delay running with with say a 1sec (1000ms) delay i.e. <br>
  <code>ledDelay.start(1000);</code><br>
  This is often done in setup()<br>
  Then in loop() check if the delay has timed out with<br>
  <code>if (ledDelay.justFinished()) {<br>
   . . .  do stuff here when delay has timed out<br>
   }</code><br>
   
   The **justFinished()** method only returns true once the first time it is check after the delay has timeout.<br>
   <b>NOTE:</b> It is very important that <code>if (ledDelay.justFinished()) {</code> is called every loop and that it is at the outer most level of the loop() method.<br>
   That is must not be inside another if() while() case() etc statement.  It can be inside a method call as long as that method is called every loop.<br>
   
  See [How to code Timers and Delays in Arduino](https://www.forward.com.au/pfod/ArduinoProgramming/TimingDelaysInArduino.html) for more examples
  
****************************************************************************************/
class millisDelay {
  public:

    millisDelay();

    /**
      Start a delay of this many milliseconds
      @param delay in millisconds, 0 means ifFinished() return true on first call
    */
    void start(unsigned long delay);

    /**
       Stop the delay
       justFinished() will now never return true
       until after start(),restart() or repeat() called again
    */
    void stop();

    /**
      repeat()
      Do same delay again but allow for a possible delay in calling justFinished()
    */
    void repeat();

    /**
      restart()
      Start the same delay again starting from now
      Note: use repeat() when justFinished() returns true, if you want a regular repeating delay
    */
    void restart();

    /**
       Force delay to end now
    */
    void finish();

    /**
      Has the delay ended/expired or has finish() been called?
      justFinished() returns true just once when delay first exceeded or the first time it is called after finish() called
    */
    bool justFinished();

    /**
      Is the delay running, i.e. justFinished() will return true at some time in the future
    */
    bool isRunning();

    /**
      Returns the last time this delay was started, in ms, by calling start(), repeat() or restart()
      Returns 0 if it has never been started
    */
    unsigned long getStartTime();

    /**
      How many ms remaining until delay finishes
      Returns 0 if finished or stopped
    */
    unsigned long remaining();

    /**
      The delay set in start
    */
    unsigned long delay();

  private:
    unsigned long ms_delay;
    unsigned long startTime;
    bool running; // true if delay running false when ended
    bool finishNow; // true if finish() called to finish delay early, false after justFinished() returns true
};
#endif
