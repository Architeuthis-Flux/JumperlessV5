/*
   PinFlasher.cpp
   by Matthew Ford,  2021/12/06
   (c)2021 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.
*/
#include "PinFlasher.h"
/** 
 *  PIN_ON is a 'magic' number that turns the output ON when setOnOff(PIN_ON) called
 */
const int PIN_ON = -1; 
/** 
 *  PIN_OFF is a 'magic' number that turns the output OFF when setOnOff(PIN_ON) called
 */
const int PIN_OFF = 0; 
/**
   Constructor.
   if pin >=0 it is initally set to output and OFF<br>
   @param pin -- the pin number to flash, default -1 (not set)<br>
   @param invert -- true to make pin LOW for on, false (default) to make pin HIGH for on.
*/
PinFlasher::PinFlasher(int pin, bool invert) {
  outputInverted = invert; // set this befor calling setPin( ) so off is correct logic level
  half_period = PIN_OFF; // off
  io_pin_on = false;
  io_pin = pin; // don't call setPin() here as that enables setOutput before all the globals have finished construction
  // causes problems for ESP32C etc using ws2812
  if(io_pin >=0) {
    pinMode(io_pin, OUTPUT); // io_pin >=0 here
  }
}

PinFlasher::~PinFlasher() {
	setPin(-1); // set pin back to input
}

/**
   check if output should be changed now.
   update() should be called often, atleast every loop()
*/
void PinFlasher::update() {
  if (!isRunning()) {
    return;
  }
  if (justFinished()) {
    if (half_period == PIN_OFF) {  // should not happen
      io_pin_on = false; // stay off
      stop(); // stop flash timer
    } else if (half_period == (unsigned long)(PIN_ON)) { // should not happen
      io_pin_on = true;       // stay on
      stop(); // stop flash timer
    } else { //restart flash
      restart(); // slips time
      io_pin_on = !io_pin_on;
    }
    setOutput(); // off does nothing if io_pin < 0
  }
}

/**
   Set the output pin to flash.
   Call setOnOff( ) to start flashing, after calling setPin()<br>
   Multiple calls to this method with the same pinNo are ignored and do not interfere with flashing<br>
   If pinNo changes, stop any current flashing, set pin to output and OFF<br>
   else ignore this call<br>
   @param pin -- the pin number to flash
*/
void PinFlasher::setPin(int pin) {
  if (pin < 0) { //all -ve inputs forced to -1
    pin = -1;
  }
  if (io_pin == pin) { // all -ve inputs will match and return
  	update();
    return;
  }
  // else pin changed re-init
  // set existing pin back to input
  int prev_pin = io_pin;
  io_pin = pin;
  stop(); // stop flash timer
  half_period = PIN_OFF; // off
  io_pin_on = false;
  if(io_pin >= 0) {
    pinMode(io_pin, OUTPUT); // io_pin >=0 here
    setOutput();
  }
  if(prev_pin >=0) {
    pinMode(prev_pin, INPUT); // reset previous output
  }
}

/**
    Set the On and Off length, the period is twice this setting.
    This call does nothing is the on/off length is the same as the existing setting.<br>
    i.e. Multiple calls to this method with the same arguement are ignored and do not interfere with flashing<br>
    This simplifies the calling logic.<br>
    @param onOff_ms -- ms for on and also for off, i.e. half the period, duty cycle 50%<br>
    PIN_OFF (0) turns off the output<br>
    PIN_ON (-1) turns the output on <br>
    other values turn the output on for that length of time and then off for the same time
    */
void PinFlasher::setOnOff(unsigned long onOff_ms) {
	if (half_period == onOff_ms) {
      update(); // update if called with no change
      return;
	}
  half_period = onOff_ms;
  if (half_period == PIN_OFF) { // stay off
    io_pin_on = false;
    stop(); // stop flash timer
  } else if (half_period == (unsigned long)(PIN_ON)) {  // stay on
    io_pin_on = true;
    stop(); // stop flash timer
  } else { //restart flash
    io_pin_on = true;
    if (io_pin >=0 ) { // if have a pin
      start(half_period);  // restart
    } else {
      stop(); // no output so stop timer
    }
  }
  setOutput();
}


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
bool PinFlasher::invertOutput() {
  outputInverted = !outputInverted;
  setOutput();
  return outputInverted;
}

/**
   set the output based on io_pin, io_pin_on and outputInverted.
   This is a non-public helper method
*/
void PinFlasher::setOutput() { // uses class vars io_pin and io_pin_on
  if (io_pin < 0) {
    return;
  }
  if (io_pin_on) {
    if (!outputInverted) {
      digitalWrite(io_pin, HIGH); // on
    } else {
      digitalWrite(io_pin, LOW); // on inverted
    }
  } else { // off
    if (!outputInverted) {
      digitalWrite(io_pin, LOW); // off
    } else {
      digitalWrite(io_pin, HIGH); // off inverted
    }
  }
}

