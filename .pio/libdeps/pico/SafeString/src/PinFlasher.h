#ifndef _PIN_FLASHER_H_
#define _PIN_FLASHER_H_
/*
   PinFlasher.h
   by Matthew Ford,  2021/12/06
   (c)2021 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.
*/
#include <Arduino.h>
#include "millisDelay.h"
/**
   Flashes the io_pin.
*/
/**
    PIN_ON is a 'magic' number that turns the output ON when setOnOff(PIN_ON) called
*/
extern const int PIN_ON;
/**
    PIN_OFF is a 'magic' number that turns the output OFF when setOnOff(PIN_ON) called
*/
extern const int PIN_OFF;

/**************

 The **PinFlasher** class inherits from **millisDelay** to provide non-blocking repeating on/off toggle of the specified pin, see the detailed description. 
    
  To use **PinFlasher**, create a global instance for each pin you want to flash e.g.<br>
  <code>PinFlasher ledFlasher(13);</code><br>
  if the led is turned ON with a HIGH output or set the optional <b><i>invert</i></b> argument true i.e. <br>
  <code>PinFlasher ledFlasher(13,true);</code><br>
  if the led is turned ON with a LOW output<br>
  
  Then add to the loop() code the statement<br>
  <code>ledFlasher.update();</code><br>
  
  You can then control the led state and flash rate with<br>
  <code>ledFlasher.setOnOff(1000);</code><br>
  to flash the led on for 1sec (1000ms) and off for 1sec (1000ms) or<br>
  <code>ledFlasher.setOnOff(PIN_ON);</code><br>
  turn the led hard ON or<br>
  <code>ledFlasher.setOnOff(PIN_OFF);</code><br>
  to turn the led hard OFF<br>
  
  When you created the pinFlasher you specified the output level for logical ON so PIN_ON will turn the led on and PIN_OFF will turn it off.<br>
  
****************************************************************************************/
class PinFlasher: protected millisDelay {
  public:
    /**
       Constructor.
       if pin >= 0 it is initally set to output and OFF<br>
       @param pin -- the pin number to flash, default -1 (not set)<br>
       @param invert -- true to make pin LOW for on, false (default) to make pin HIGH for on.
    */
    PinFlasher(int pin = -1, bool invert = false);
    
    ~PinFlasher(); // sets pin back to input
    
    /**
       check if output should be changed.
       update() should be called often, atleast every loop()
    */
    void update();

    /**
       Set the output pin to flash.
       Call setOnOff( ) to start flashing, after calling setPin()<br>
       Multiple calls to this method with the same pinNo are ignored and do not interfere with flashing<br>
       If pinNo changes, stop any current flashing, set pin to output and OFF<br>
       else ignore this call<br>
       @param pin -- the pin number to flash
    */
    void setPin(int pin);

    /**
      Set the On and Off length, the period is twice this setting.
      This call does nothing if the on/off length is the same as the existing setting.<br>
      This simplifies the calling logic.<br>
      @param onOff_ms -- ms for on and also for off, i.e. half the period, duty cycle 50%<br>
      PIN_OFF (0) turns off the output<br>
      PIN_ON (-1) turns the output on <br>
      other values turn the output on for that length of time and then off for the same time
    */
    void setOnOff(unsigned long onOff_ms);

    /**
        Normally pin output is LOW for off, HIGH for on.
        This inverts the current setting for on/off<br>
        @return -- the current setting, true if on == LOW, false if on == HIGH<br>
        e.g. <br>
        PinFlasher f(2,true);  // pin 2, inverted, i.e. On is LOW, off is HIGH<br>
        f.setOnOff(100); // set flash 1/10sec on then 1/10sec off<br>
        ...<br>
        f.setOnOff(PIN_ON); // set output on, i.e. HIGH<br>
        f.invertOutput(); // now pin 2 still on but now is LOW,<br>
        ...<br>
        f.setOnOff(PIN_OFF);  // set output OFF, i.e. HIGH because of invertOutput above
    */
    bool invertOutput();
    
  protected:
    /**
       set the output based on io_pin, io_pin_on and outputInverted
    */
    virtual void setOutput();
    int io_pin; // initially -1, not set
    bool io_pin_on;//initially false/ off;
    bool outputInverted; // initially false, not inverted, i.e. off is LOW, on is HIGH

  private:
    unsigned long half_period; // initially 0, off
};

#endif
