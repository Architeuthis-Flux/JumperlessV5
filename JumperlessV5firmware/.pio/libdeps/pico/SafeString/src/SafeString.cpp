// !!!!!!!!! WARNING in V2 substring endIdx is EXCLUSIVE !!!!!!!!!! change from V1 inclusive
/*
   The SafeString class V3.0.6
   Note: ESP32 gives warning: "F" redefined which can be ignored

  -----------------  creating SafeStrings ---------------------------------
  See the example sketches SafeString_ConstructorAndDebugging.ino and SafeStringFromCharArray.ino
   and SafeStringFromCharPtr.ino and SafeStringFromCharPtrWithSize.ion

  createSafeString(name, size) and createSafeString(name, size, "initialText")
  are utility macros to create an SafeString of a given name and size and optionally, an initial value

  createSafeString(str, 40);  or  cSF(str, 40);
  expands in the pre-processor to
   char str_SAFEBUFFER[40+1];
   SafeString str(sizeof(str_SAFEBUFFER),str_SAFEBUFFER,"","str");

  createSafeString(str, 40, "test");  or cSF(str, 40, "test");
  expands in the pre-processor to
   char str_SAFEBUFFER[40+1];
   SafeString str(sizeof(str_SAFEBUFFER),str_SAFEBUFFER,"test","str");

  createSafeStringFromCharArray(name, char[]);  or cSFA(name, char[]);
   wraps an existing char[] in a SafeString of the given name
  e.g.
  char charBuffer[15];
  createSafeStringFromCharArray(str,charBuffer); or cSFA(str,charBuffer);
  expands in the pre-processor to
   SafeString str(sizeof(charBuffer),charBuffer, charBuffer, "str", true);

  createSafeStringFromCharPtrWithSize(name, char*, size_t);  or cSFPS(name, char*, size_t);
   wraps an existing char[] pointed to by char* in a SafeString of the given name and sets the capacity to the given size -1
  e.g.
  char charBuffer[15]; // can hold 14 char + terminating '\0'
  char *bufPtr = charBuffer;
  createSafeStringFromCharPtrWithSize(str,bufPtr, 15); or cSFPS(str,bufPtr, 15);
  expands in the pre-processor to
   SafeString str(15,charBuffer, charBuffer, "str", true);
  The capacity of the SafeString is set to 14.

  createSafeStringFromCharPtr(name, char*);  or cSFP(name, char*);
   wraps an existing char[] pointed to by char* in a SafeString of the given name
  createSafeStringFromCharPtr(name, char* s) is the same as   createSafeStringFromCharPtrWithSzie(name, char* s, strlen(s));
  That is the current strlen() is used to set the SafeString capacity.
  e.g.
  char charBuffer[15] = "test";
  char *bufPtr = charBuffer;
  createSafeStringFromCharPtr(str,bufPtr); or cSFP(str,bufPtr);
  expands in the pre-processor to
   SafeString str(0,charBuffer, charBuffer, "str", true);
  and the capacity of the SafeString is set to strlen(charBuffer) and cannot be increased.


****************************************************************************************/

/*  **************************************************
   If str is a SafeString then
   str = .. works for signed/unsigned ints, char*, char, F(".."), SafeString float, double etc
   str.concat(..) and string.prefix(..) also works for those
   str.stoken(..) can be used to split a string in to tokens

   SafeStrings created via createSafeString(..) or cSF(..) are never invalid, even if called with invalid arguments.
   SafeStrings created via createSafeStringFromCharArray(..) or cSFA(..) are valid as long at the underlying char[] is valid
     Usually the only way the char[] can become invalid is if it exists in a struct that is allocated (via calloc/malloc)
     and then freed while the SafeString wrapping it is still in use.
   SafeStrings created via createSafeStringFromCharPtr(..) or cSFP(..) are valid if the char[] pointed to is validly terminated
   SafeStrings created via createSafeStringFromCharWithSize(..) or cSFPS(..) are valid if the char[] and size specified is valid.
   For both createSafeStringFromCharPtr() and createSafeStringFromCharWithSize()
   the SafeStrings created remain valid as long as the underlying char[] is valid.
     Usually the only way the char[] can become invalid is if it was allocated (via calloc/malloc)
     and then freed while the SafeString wrapping it is still in use.
*  ***************************************************/

/*
  SafeString.cpp V2.0.0 static memory SafeString library modified by
  Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  All rights reservered subject to the License below

  extensively modified from

  WString.cpp - SafeString library for Wiring & Arduino
  ...mostly rewritten by Paul Stoffregen...
  Copyright (c) 2009-10 Hernando Barragan.  All rights reserved.
  Copyright 2011, Paul Stoffregen, paul@pjrc.com

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include "SafeString.h"
#include <limits.h>

#if !defined(ARDUINO_ARCH_AVR)
#ifdef __cplusplus
extern "C" {
#endif
#if defined(ARDUINO_TEENSY41) || defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY36) ||defined(ARDUINO_TEENSY35) ||defined(ARDUINO_TEENSY32) ||defined(ARDUINO_TEENSY30) ||defined(ARDUINO_TEENSYLC) || defined(ARDUINO_TEENSY2PP) ||defined(ARDUINO_TEENSY2)
char* dtostrf(float val, int width, unsigned int precision, char *buf);
#elif defined(ESP32) || defined(ESP8266)
#include <stdlib_noniso.h>
//for char* dtostrf( ...);
#else
char* dtostrf(double val, signed char width, unsigned char prec, char *sout);
#endif
#ifdef __cplusplus
}
#endif
#endif

#include "SafeStringNameSpace.h"


// this code is not used at present
#if defined(ARDUINO_AVR_MEGA2560)
// for mega with 8K ram max is half
#define SAFE_STRING_MAX_CSTRING_SIZE 4095
#elif defined(ARDUINO_AVR_UNO)
// for UNO with 2K ram max is half
#define SAFE_STRING_MAX_CSTRING_SIZE 1023
#else
// default max size is 10K
#define SAFE_STRING_MAX_CSTRING_SIZE 10239
#endif

// to remove all the error debug outputs, comment out
// #define SSTRING_DEBUG
// in SafeString.h file
// this saves program space used by the error messages and the ram used by the SafeString object names
//
// usually just leave as is and use SafeString::setOutput(..) to control the debug output
// there will be no debug output is SafeString::setOutput(..) has not been called from your sketch
// SafeString.debug() is always available regardless of the SSTRING_DEBUG define setting
//   but SafeString::setOutput() still needs to be called to set where the output should go.

char SafeString::nullBufferSafeStringBuffer[1] = {'\0'}; // use if buf arg is NULL
char SafeString::emptyDebugRtnBuffer[1] = {'\0'}; // use for debug() returns
Print* SafeString::debugPtr = NULL; // nowhere to send the debug output yet
Print* SafeString::currentOutput = &SafeString::emptyPrint; // nowhere to send Output to yet
SafeString::noDebugPrint SafeString::emptyPrint;
SafeString::DebugPrint SafeString::Output;
bool SafeString::fullDebug = true; // output current contents of SafeString and input arg
bool SafeString::classErrorFlag = false; // set true if any SafeString object has an error.

/*  ********************************************/
/**  Constructor                             */
/*  ********************************************/

// This constructor overwrites any data already in buf with cstr unless  &buf == &cstr  in that case just clear both.
// _fromBuffer true does extra checking before each method execution for SafeStrings created from existing char[] buffers
// _fromPtr is not checked unless _fromBuffer is true
// _fromPtr true allows for any array size, if false it warns when passing char* by checking sizeof(charArray) != sizeof(char*)
/**
  if _fromBuffer false (i.e cSF(sfStr,20); ) then maxLen is the capacity and the macro allocates an char[20+1], i.e. maxLen+1 (_fromPtr ignored)
  if _fromBuffer true and _fromPtr false (i.e. cSFA(sfStr, strArray); ) then maxLen is the sizeof the strArray and the capacity is maxLen-1, _fromPtr is false
  if _fromBuffer true and _fromPtr true, then from char*, (i.e. cSFP(sfStr,strPtr) or cSFPS(sfStr,strPtr, maxLen) and maxLen is either -1 cSFP( ) the size of the char Array pointed cSFPS
    if maxLen == -1 then capacity == strlen(char*)  i.e. cSFP( )
    else capacity == maxLen-1;   i.e. cSFPS( )
*/
SafeString::SafeString(size_t maxLen, char *buf, const char* cstr, const char* _name, bool _fromBuffer, bool _fromPtr) {
   errorFlag = false; // set to true if error detected, cleared on each call to hasError()
  timeoutStart_ms = 0;
  noCharsRead = 0; // number of char read on last call to readUntilToken
  buffer = NULL;          // the actual char array
  _capacity = 0; // the array length minus one (for the '\0')
  len = 0;       // the SafeString length (not counting the '\0')
  name = _name; // save name
  fromBuffer = _fromBuffer;
  timeoutRunning = false;
  bool keepBufferContents = false;  
  if ((buf != NULL) && (cstr != NULL) && (buf == cstr)) {
    keepBufferContents = true;
  }
  if (fromBuffer) {
    if ((!_fromPtr) && (maxLen == sizeof(char*))) {
      buffer = nullBufferSafeStringBuffer;
      _capacity = maxLen;
      len = 0;
      buffer[0] = '\0';
      setError();
#ifdef SSTRING_DEBUG
      if (debugPtr) {
        debugPtr->print(F("Error: cSFA / createSafeStringFromCharArray("));
        outputName(); debugPtr->print(F(", ...) sizeof(charArray) == sizeof(char*). \nCheck you are passing a char[] to createSafeStringFromCharArray OR use char[5] or larger clear this Error\n"
                                        "  To wrap a char* use either createSafeStringFromCharPtr(..), cSFP(..) or createSafeStringFromCharPtrWithSize(..), cSFPS(.. )"));
        debugInternalMsg(fullDebug);
      }
#endif // SSTRING_DEBUG
      return;
    }
  }

  if (buf != NULL) {
    buffer = buf;
    if ((maxLen == 0) || (maxLen == ((size_t) - 1))) { // -1 => find cap from strlen()
      if (fromBuffer && _fromPtr) { // either ..fromCharPtr or ..fromCharPtrWithSize
        if (maxLen == 0) { // ..fromCharPtrWithSize
          buffer = nullBufferSafeStringBuffer;
          _capacity = 0;
          len = 0;
          buffer[0] = '\0';
          setError();
#ifdef SSTRING_DEBUG
          if (debugPtr) {
            debugPtr->print(F("Error: createSafeStringFromCharArrayWithSize("));
            outputName(); debugPtr->print(F(", ..., 0) was passed zero passed for array size"));
            debugInternalMsg(fullDebug);
          }
#endif // SSTRING_DEBUG
          return;
        } else { // -1 ..fromCharPtr  use strlen()
          // calculate capacity from inital contents length. Have check buffer not NULL
          _capacity = strlen(buf);  // this can fail if input c-string buffer is not terminated!!
          /**
                    _capacity = limitedStrLen(buf,SAFE_STRING_MAX_CSTRING_SIZE);  // this can faile if input c-string buffer is not terminated!!
                    if (_capacity >= SAFE_STRING_MAX_CSTRING_SIZE) {
                      buffer = nullBufferSafeStringBuffer;
                      len = 0;
                      buffer[0] = '\0';
                      setError();
            #ifdef SSTRING_DEBUG
                      if (debugPtr) {
                        debugPtr->print(F("Error: cSFPS / createSafeStringFromCharPtr("));
                        outputName(); debugPtr->print(F(", ...) strlen()"));  debugPtr->print(F(" exceeds max allowable size for this board: ")); debugPtr->print(SAFE_STRING_MAX_CSTRING_SIZE);
                        debugInternalMsg(fullDebug);
                      }
            #endif // SSTRING_DEBUG
                     return;
                    } // else
          **/
        }
      } else { // from char[] most likely maxLen == 0 as (size_t)-1 is very very large
        buffer = nullBufferSafeStringBuffer;
        _capacity = 0;
        len = 0;
        buffer[0] = '\0';
        setError();
#ifdef SSTRING_DEBUG
        if (debugPtr) {
          debugPtr->print(F("Error: createSafeStringFromCharArray("));
          outputName(); debugPtr->print(F(", ...) passed a zero length array"));
          debugInternalMsg(fullDebug);
        }
#endif // SSTRING_DEBUG
        return;
      }
    } else { //    if (maxLen > 0) and != (size_t)-1 {
      if (maxLen >= INT_MAX) {
        maxLen = INT_MAX - 1; // limit due to using int for indexOf returns
      }
      /**
            if (maxLen > SAFE_STRING_MAX_CSTRING_SIZE) {
              buffer = nullBufferSafeStringBuffer;
              _capacity = maxLen;
              len = 0;
              buffer[0] = '\0';
              setError();
        #ifdef SSTRING_DEBUG
              if (debugPtr) {
                debugPtr->print(F("Error: cSFPS / createSafeStringFromCharPtrWithSize("));
                outputName(); debugPtr->print(F(", ...) specified size: ")); debugPtr->print(maxLen); debugPtr->print(F(" exceeds max allowable size for this board: ")); debugPtr->print(SAFE_STRING_MAX_CSTRING_SIZE);
                debugInternalMsg(fullDebug);
              }
        #endif // SSTRING_DEBUG
             return;
            } // else
      **/
      _capacity = maxLen - 1; // maxLen is actual array size so -1 to allow for '\0'
    }
    if (!keepBufferContents) {
      len = 0;
      buffer[0] = '\0'; // clears cstr is it is the same as buf !!
    }
  } else {
    buffer = nullBufferSafeStringBuffer;
    _capacity = 0;
    len = 0;
    buffer[0] = '\0';
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error: SafeString("));
      outputName(); debugPtr->print(F(", ...) was passed a NULL pointer for its char array"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }

  if (cstr == NULL) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error: SafeString("));
      outputName(); debugPtr->print(F(", ...) was passed a NULL pointer for initial value."));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }

  // _capacity is set here
  // check available memory
  bool memFail = false;
#if defined(ARDUINO_ARCH_AVR)
  void *mem = malloc(_capacity); // will leave 128 for stack use
#else
  void *mem = malloc(_capacity + 256); // others 256 for stack use
#endif
  if (mem == NULL) {
    memFail = true;
  }
  free(mem);
  if (memFail) {
    setError();
    len = 0;
    buffer[0] = '\0';
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error: SafeString("));
      outputName(); debugPtr->print(F(", ...) request size:")); debugPtr->print(_capacity); debugPtr->print(F(" exceeds available memory. Returning zero capacity SafeString."));
      _capacity = 0;
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    buffer = nullBufferSafeStringBuffer;
    _capacity = 0;
    return;
  }
  size_t cstrLen = limitedStrLen(cstr, _capacity);
  if (cstrLen > _capacity) { // may be unterminated buffer
    if (!keepBufferContents) {
      // done above
      //  len = 0;
      //  buffer[0] = '\0'; // clears cstr is it is the same as buf !!
    } else {
      // does cleanUp for all new objects
      len = _capacity;
      buffer[len] = '\0'; // truncate buffer to given size
    }
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (!keepBufferContents) {
        debugPtr->print(F("Error: SafeString("));
        outputName(); debugPtr->print(F(", ...) needs capacity of ")); debugPtr->print(cstrLen);  debugPtr->print(F(" for initial value."));
        if (fullDebug) {
          debugPtr->println(); debugPtr->print(F("       "));
          debugPtr->print(F(" Initial value arg was '")); debugPtr->print(cstr); debugPtr->print('\'');
        }
        debugInternalMsg(fullDebug);
      } else {
        if (_fromPtr) {
          debugPtr->print(F("Error: createSafeStringFromCharPtrWithSize("));
        } else {
          debugPtr->print(F("Error: createSafeStringFromCharArray("));
        }
        outputName(); debugPtr->print(F(", ...) passed unterminated buffer of strlen >=")); debugPtr->print(cstrLen);
        if (fullDebug) {
          debugPtr->println(); debugPtr->print(F("       "));
          debugPtr->print(F(" Truncated value saved is '")); debugPtr->print(cstr); debugPtr->print('\'');
        }
        debugInternalMsg(fullDebug);
      }
    }
#endif // SSTRING_DEBUG
    return;
  }

  if (!keepBufferContents) {
    memmove(buffer, cstr, cstrLen);
  }
  // does cleanUp for all new objects
  len = cstrLen;
  buffer[len] = '\0';
  buffer[_capacity] = '\0';
  // just let the SafeString fail later :-(
  //  for (size_t i= len+1; i<_capacity; i++) {
  //      buffer[i] ='\0'; // force error here if stack overflow
  //  }
}

// this is private and should never be called.
SafeString::SafeString(const SafeString& other ) {
  //  setError();
  //#ifdef SSTRING_DEBUG
  //    if (debugPtr) {
  //      debugPtr->println(F("Error: SafeString arguments must be declared as references, SafeString&  e.g. void test(SafeString& strIn)"));
  //    }
  //#endif // SSTRING_DEBUG
  (void)(other);
  buffer = nullBufferSafeStringBuffer;
  _capacity = 0;
  len = 0;
  buffer[0] = '\0';
  timeoutRunning = false;
  errorFlag = false;
}
/**  end of Constructor methods ***********/

unsigned char SafeString::errorDetected() {
  bool rtn = classErrorFlag;
  classErrorFlag = false;
  return rtn;
}

void SafeString::setError() {
  classErrorFlag = true;
  errorFlag = true;
}

unsigned char SafeString::hasError() {
  bool rtn = errorFlag;
  errorFlag = false;
  return rtn;
}

// only access upto limit
size_t SafeString::limitedStrLen(const char* p, size_t limit) {
  size_t rtn = 0;
  while ((rtn <= limit) && (*p) ) {
    rtn++;
    p++;
  }
  return rtn;
}

/*********************************************/
/**  Information and utility methods         */
/** clear(), length(), capacity(),           */
/** isFull(), isEmpty(), availableForWrite() */
/**  and the private methods cleanUp()       */
/*********************************************/

// make the SafeString empty
SafeString & SafeString::clear(void) {
  len = 0;
  buffer[len] = '\0';
  return *this;
}

/*****************
  // truncates SafeString to this length
  void SafeString::setLength(unsigned int newLength) {
  cleanUp();
  if (newLength > len) {
    setError();
  #ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error:"));
      outputName();
      debugPtr->print(F(".setLength() newLength ")); debugPtr->print(newLength); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
  #endif // SSTRING_DEBUG
    return;
  }
  buffer[newLength] = '\0';
  len = newLength;
  }
************/
// return the equivalent of strlen( ) for this SafeString
unsigned int SafeString::length(void) {
  cleanUp();
  return len;
}

// return the maximum number of chars that this SafeString can store
unsigned int SafeString::capacity(void) {
  cleanUp();
  return _capacity;
}

// cannot store any more chars in this SafeString
unsigned char SafeString::isFull(void) {
  return (length() == capacity()); // each calls cleanUp()
}

// no chars stored in this SafeString
unsigned char SafeString::isEmpty(void) {
  return length() == 0; // calls cleanUp()
}

// how many chars can be added to this SafeString before it is full
int SafeString::availableForWrite(void) {
  return (capacity() - length());  // each calls cleanUp()
}

/**  private utility method cleanUp() **/
// cleanUp() --  a private utility method only used  when SafeString created
//              from createSafeStringFromBuffer macro
// If this SafeString was created by createSafeStringFromBuffer then
// call cleanUp() before each method to ensure the SafeString len matches the
// actual strlen of the buffer.  Also ensure buffer is still terminated with '\0'
void SafeString::cleanUp() {
  if (!fromBuffer) {
    return; // skip scanning for length changes in the buffer in normal SafeString
  }
  bool bufferOverrun = false;
  if ((_capacity > 0) && (buffer[_capacity] != '\0')) {
    setError(); // buffer overrun
    bufferOverrun = true;
  }
  buffer[_capacity] = '\0'; // make sure the buffer is terminated to prevent memory overruns
  len = strlen(buffer);  // scan for current length to pickup an changes made outside SafeString methods
  if (bufferOverrun) {
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("SafeString cleanUp detected buffer overrun by external code."));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
  }
}
/** end of  Information and utility methods ****************/


/***********************************************/
/**  Output and debug control methods          */
/** setOutput(), turnOutputOff(), setVerbose() */
/***********************************************/

// setOutput( ) -- turns on Error msgs and debug( )
// This static method effects ALL SafeStrings
// use
//   SafeString::setOutput(Serial);
// to output Error msgs and debug to the Serial connection
// use
//   SafeString::turnOutputOff();
// to stop all Error msgs and debug output (sets debugPtr to NULL)
// verbose is an optional argument, if missing defaults to true, use false for compact error messages or call setVerbose(false)
void SafeString::setOutput(Print& debugOut, bool verbose) {
  debugPtr = &debugOut;
  fullDebug = verbose;  // the verbose argument is optional, if missing fullDebug is true
  currentOutput = debugPtr;
}


// turnOutputOff() -- turns off error msgs and debugging output
// This static method effects ALL SafeStrings
// use
//   SafeString::turnOutputOff();
// to stop all Error msgs and debug output (sets debugPtr to NULL)
void SafeString::turnOutputOff() {
  debugPtr = NULL;
  currentOutput = &emptyPrint;
}

// setVerbose( ) -- controls level of error msgs
// This static method effects ALL SafeStrings
// use
//   SafeString::setVerbose(true);
// for detailed error msgs.use this to control error messages verbose output
//   SafeString::setVerbose(false);
// for minimal error msgs
// setVerbose( ) does not effect debug() methods which have their own optional verbose argument
void SafeString::setVerbose(bool verbose) {
  fullDebug = verbose;
}
/** end of Output and debug control methods ***********/

/*********************************************/
/**  debug methods                           */
/*********************************************/

// debug() -- these debug( ) methods print out info on this SafeString object, iff SafeString::setOutput( ) has been called
// setVerbose( ) does NOT effect these methods which have their own verbose argument
// Each of these debug( ) methods defaults to outputting the string contents.
// Set the optional verbose argument to false to suppress outputting string contents
//
// NOTE!! all these debug methods return a pointer to an empty char[].
// This is so that if you add .debug() to Serial.println(str);  i.e.
//    Serial.println(str.debug());
// will work as expected
const char* SafeString::debug(bool verbose) { // verbose optional defaults to true
  debug((const char*) NULL, verbose); // calls cleanUp();
  emptyDebugRtnBuffer[0] = '\0'; // if the last return was changed
  return emptyDebugRtnBuffer;
}

// These three versions print leading text before the debug output.
const char* SafeString::debug(const __FlashStringHelper * pstr, bool verbose) { // verbose optional defaults to true
  cleanUp();
  if (debugPtr) {
    if (pstr) {
      debugPtr->print(pstr);
    } else {
      debugPtr->print(F("SafeString"));
    }
    debugInternal(verbose);
  }
  emptyDebugRtnBuffer[0] = '\0'; // if the last return was changed
  return emptyDebugRtnBuffer;
}

const char* SafeString::debug(const char *title, bool verbose) { // verbose optional defaults to true
  cleanUp();
  if (debugPtr) {
    if (title) {
      debugPtr->print(title);
    } else {
      debugPtr->print(F("SafeString"));
    }
    debugInternal(verbose);
  }
  emptyDebugRtnBuffer[0] = '\0'; // if the last return was changed
  return emptyDebugRtnBuffer;
}

const char* SafeString::debug(SafeString &stitle, bool verbose) { // verbose optional defaults to true
  cleanUp();
  stitle.cleanUp();
  if (debugPtr) {
    if (stitle.len != 0) {
      debugPtr->print(stitle);
    } else {
      debugPtr->print(F("SafeString"));
    }
    debugInternal(verbose);
  }
  emptyDebugRtnBuffer[0] = '\0'; // if the last return was changed
  return emptyDebugRtnBuffer;
}

/*****************  end public debug methods *************************/


/*********************************************/
/**  Printable interface, printTo()          */
/*********************************************/
// this interface allows you to print to a SafeString
// e.g.
//  sfStr.print(55,HEX);
//
size_t SafeString::printTo(Print& p) const {
  SafeString *ptr = (SafeString *)(this); // do this to get around the required const designator
  ptr->cleanUp();
  return p.print(buffer);
}
/***************** end of implementation of Printable interface *************************/

/*************************************************/
/**  Print support, write(), print..() methods   */
/*************************************************/
size_t SafeString::write(uint8_t b) {
  cleanUp();
  if (b == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("write"));
      debugPtr->print(F(" of 0"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  size_t newlen = len + 1;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("write"), newlen, NULL, NULL);
#endif // SSTRING_DEBUG
    return 0;
  }
  concat((char)b);
  return 1;
}

// write at most length chars from buffer into this SafeString
// NOTE: write(cstr,length) will set hasError and optionally output errorMsg, if strlen(cstr) < length and nothing will be added to the SafeString
size_t SafeString::write(const uint8_t *buffer, size_t length) {
  cleanUp();
  if (length == 0) {
    return 0;
  }
  // concat check length so normalize it here
  //  size_t bufferLen = limitedStrLen((char *)buffer, length);
  //  if (bufferLen < length) {
  //      length = bufferLen;
  //  }
  size_t initialLen = len;
  size_t newlen = len + length;

  if (length > limitedStrLen((char *)buffer, length)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("write"));
      debugPtr->print(F(" length ")); debugPtr->print(length); debugPtr->print(F(" > uint8_t* arg strlen."));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was "));
        debugPtr->print(F(" { "));
        for (size_t i = 0; i < length; i++) {
          debugPtr->print(buffer[i]); debugPtr->print(i < (length - 1) ? ',' : ' ');
        }
        debugPtr->print(F("} "));
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }

  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("write"), newlen, (const char*)buffer, NULL, '\0', length);
#endif // SSTRING_DEBUG
    return 0;
  }
  concat((const char*)buffer, length);
  return len - initialLen;
}

size_t SafeString::println() {
  cleanUp();
  size_t newlen = len + 2;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("println"), newlen, NULL, NULL);
#endif // SSTRING_DEBUG
    return 0;
  }
  Print::println();
  return 2;
}

SafeString & SafeString::newline() {
  println(); // calls cleanUp()
  return *this;
}

// used by println
SafeString & SafeString::concatln(char c) {
  cleanUp();
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      printlnErr();
      debugPtr->print(F(" of '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  size_t newlen = len + 1 + 2;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("println"), newlen, NULL, NULL, c);
#endif // SSTRING_DEBUG
    return *this;
  }
  concat(c);
  Print::println();
  return *this;
}

SafeString & SafeString::concatln(const __FlashStringHelper * pstr) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      printlnErr();
      debugPtr->print(F(" was passed a NULL F( ) pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  size_t pLen = strlen_P((PGM_P)pstr);
  size_t newlen = len + pLen + 2;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("println"), newlen, NULL, pstr);
#endif // SSTRING_DEBUG
    return *this;
  }
  concat(pstr, strlen_P((PGM_P)pstr));
  Print::println();
  return *this;
}

SafeString & SafeString::concatln(const char *cstr, size_t length) {
  cleanUp();
  size_t newlen = len + length + 2;
  if (!cstr)  {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      printlnErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }

  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("println"), newlen, cstr, NULL, '\0', length);
#endif // SSTRING_DEBUG
    return *this;
  }
  concat(cstr, length);
  Print::println();
  return *this;
}



//============= public print methods ===========
size_t SafeString::print(unsigned char c, int d) {
  return printInternal((unsigned long)c, d); // calls cleanUp()
}
size_t SafeString::print(int i, int d) {
  return printInternal((long)i, d); // calls cleanUp()
}
size_t SafeString::print(unsigned int u, int d) {
  return printInternal((unsigned long)u, d); // calls cleanUp()
}
size_t SafeString::print(long l, int d) {
  return printInternal(l, d); // calls cleanUp()
}
size_t SafeString::print(unsigned long l, int d) {
  return printInternal(l, d); // calls cleanUp()
}
size_t SafeString::print(double d, int decs) {
  return printInternal(d, decs); // calls cleanUp()
}

/**
  print to SafeString a double (or long) with decs after the decimal point
  and padd to specified width
  width is signed value, negative for left adjustment, +ve for right padding
  by default the + sign is not added, set forceSign argument to true to force the display of the + sign

  If the result exceed abs(width), reduce the decs after the decmial point to fit
  If result with decs == 0 still > abs(width) raise an error and ,optionally, output an error msg

  Note decs is quietly limited in this method to < 7
*/
size_t SafeString::print(double d, int decs, int width, bool forceSign) {
  return printInt(d, decs, width, forceSign, false);
}
size_t SafeString::println(double d, int decs, int width, bool forceSign) {
  return printInt(d, decs, width, forceSign, true);
}

// internal print method called by other print methods
size_t SafeString::printInt(double d, int decs, int width, bool forceSign, bool addNL) {
  // if addNL need to allow 2 for nl in SafeString, width does not change
  size_t nlExtra = addNL ? 2 : 0;

  size_t absWidth = 0;
  if (width < 0) {
    absWidth = (-width);
  } else {
    absWidth = width;
  }
  // need to force a + sign
  if (forceSign) {
    if (d < 0) { // nothing to do
      forceSign = false;
    } else {
      d = -d; // this will add a - which will be replaced with + at the end
    }
  }

  if ((absWidth == 0) || ((absWidth == 1) && (forceSign))) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (addNL) {
        errorMethod(F("println"));
      } else {
        errorMethod(F("print"));
      }
      debugPtr->print(F(" width:")); debugPtr->print(width); debugPtr->print(F(" too small to display even just the integer part of "));  debugPtr->print(d);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }

  if (decs < 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (addNL) {
        errorMethod(F("println"));
      } else {
        errorMethod(F("print"));
      }
      debugPtr->print(F(" number places after the decimal point, ")); debugPtr->print(decs); debugPtr->print(F(" < 0"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  if ((absWidth + nlExtra) > (_capacity - len)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (addNL) {
        errorMethod(F("println"));
      } else {
        errorMethod(F("print"));
      }
      debugPtr->print(F(" needs capacity of ")); debugPtr->print(len + absWidth + nlExtra);  debugPtr->print(F(" too add number formatted to fixed width:")); debugPtr->print(width);
      if (addNL) {
        debugPtr->print(F(" and newline"));
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  // max chars are 11 for integer part including sign (-4294967040), + 1 (.) + 7 prec (prec limited to 7) = 19 + '\0' => 20
  // ESP32
  // max chars are 19 for integer part including sign (-9223372036854775807L), + 1 (.) + 7 prec (prec limited to 7) = 19 +1 + 7 + '\0' => 27
  char result[33]; // 19 +1 + 7 + '\0' => 27 for extra
  result[0] = '\0';
  if (isnan(d)) {
    strcpy(result, "nan");
  } else if (isinf(d)) {
    strcpy(result, "inf");
  } else if (d >= 4294967039.0) {
    strcpy(result, "ovf"); // constant determined empirically
  } else if (d <= -4294967039.0) {
    strcpy(result, "-ovf"); // constant determined empirically
  } else {
    if (decs > 7) {
      decs = 7; // seems to be the limit for print
    }
    dtostrf(d, width, decs, result);
    while ((strlen(result) > absWidth) && (decs > 0)) {
      // try reducing precision
      decs--;
      dtostrf(d, width, decs, result);
    }
  }
  size_t resultLen = strlen(result);
  if ((resultLen) > absWidth) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (addNL) {
        errorMethod(F("println"));
      } else {
        errorMethod(F("print"));
      }
      debugPtr->print(F(" width:")); debugPtr->print(width); debugPtr->print(F(" too small to display even just the integer part of "));  debugPtr->print(d);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    // return 0;
    result[0] = '\0'; // clear result and just padd below
  }
  // else may need to padd nan etc
  cSFA(sfResult, result);
  while (sfResult.length() < absWidth) {
    if (width < 0) {
      sfResult.prefix(' ');
    } else {
      sfResult.concat(' ');
    }
  }
  if (forceSign) { // replace - with +
    sfResult.replace('-', '+');
    // note 0.0 does not have + so handle that here
    if (sfResult.indexOf('+') < 0) {
      // still need to add +
      // find first digit
      int idx = sfResult.indexOfCharFrom("0123456789.");
      if (idx > 0) {
        sfResult.setCharAt(idx - 1, '+');
      } else {            // should not happen
        setError();
#ifdef SSTRING_DEBUG
        if (debugPtr) {
          if (addNL) {
            errorMethod(F("println"));
          } else {
            errorMethod(F("print"));
          }
          debugPtr->print(F(" Internal Error: no space to add + sign with width:")); debugPtr->print(width); debugPtr->print(F(" d:"));  debugPtr->print(d);
          debugInternalMsg(fullDebug);
        }
#endif // SSTRING_DEBUG          
      }
    }
  }

  // already padded just concat
  if (nlExtra == 0) {
    concat(result);
    return resultLen;
  } else {
    concatln(result, resultLen);
    return (resultLen + 2);
  }
}

size_t SafeString::print(SafeString &str) {
  str.cleanUp();
  cleanUp();
  size_t addLen = str.len;
  size_t newlen = len + addLen;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("print"), newlen, str.buffer, NULL);
#endif // SSTRING_DEBUG
    return 0;
  }
  concat(str);
  return addLen;
}

size_t SafeString::print(const char* cstr) {
  cleanUp();
  if (!cstr)  {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("print"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  size_t str_len = strlen(cstr);
  size_t newlen = len + str_len;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("print"), newlen, cstr, NULL);
#endif // SSTRING_DEBUG
    return 0;
  }
  concat(cstr, str_len);
  return str_len;
}

size_t SafeString::print(char c) {
  cleanUp();
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("print"));
      debugPtr->print(F(" of '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }

  size_t newlen = len + 1;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("print"), newlen, NULL, NULL, c);
#endif // SSTRING_DEBUG
    return 0;
  }
  concat(c);
  return 1;
}

size_t SafeString::print(const __FlashStringHelper *pstr) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("print"));
      debugPtr->print(F(" was passed a NULL F( ) pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  size_t newlen = len + strlen_P((PGM_P)pstr);
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      capError(F("print"), newlen, NULL, pstr);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  concat(pstr);
  return (strlen_P((PGM_P)pstr));
}

// =========================================

// ============ protected internal print methods =============

size_t SafeString::printInternal(long num, int base, bool assignOp) {
  cleanUp();
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  size_t newlen = len + temp.length();
  if (assignOp) {
    newlen = temp.length();
  }
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (assignOp) {
      assignError(newlen, temp.buffer, NULL, '\0', true);
    } else {
      capError(F("print"), newlen, temp.buffer, NULL);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  if (assignOp) {
    clear(); // clear first
  }
  concat(temp);
  return n;
}

size_t SafeString::printInternal(unsigned long num, int base, bool assignOp) {
  cleanUp();
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  size_t newlen = len + temp.length();
  if (assignOp) {
    newlen = temp.length();
  }
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (assignOp) {
      assignError(newlen, temp.buffer, NULL,  '\0', true);
    } else {
      capError(F("print"), newlen, temp.buffer, NULL);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  if (assignOp) {
    clear(); // clear first
  }
  concat(temp);
  return n;
}

size_t SafeString::printInternal(double num, int digits, bool assignOp) {
  cleanUp();
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  if (digits > 7) {
    digits = 7; // seems to be the limit for print
  }
  size_t n = temp.Print::print(num, digits);
  size_t newlen = len + temp.length();
  if (assignOp) {
    newlen = temp.length();
  }
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (assignOp) {
      assignError(newlen, temp.buffer, NULL,  '\0', true);
    } else {
      capError(F("print"), newlen, temp.buffer, NULL);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  if (assignOp) {
    clear(); // clear first
  }
  concat(temp);
  return n;
}

// =========================================================================

// ============ println =========================================

size_t SafeString::println(SafeString &str) {
  str.cleanUp();
  cleanUp();
  size_t newlen = len + str.len + 2;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("println"), newlen, str.buffer);
#endif // SSTRING_DEBUG
    return 0;
  }
  size_t addLen = str.len + 2;
  concat(str);
  Print::println();
  return addLen;
}

size_t SafeString::println(const __FlashStringHelper *pstr) {
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("println"));
      debugPtr->print(F(" was passed a NULL F( ) pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  concatln(pstr); // calls cleanUp()
  return (strlen_P((PGM_P)pstr) + 2);
}

size_t SafeString::println(const char* cstr) {
  if (!cstr)  {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      printlnErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 0;
  }
  size_t length = strlen(cstr);
  concatln(cstr, length);
  return length + 2;
}

size_t SafeString::println(char c) {
  concatln(c); // calls cleanUp()
  return 3;
}

size_t SafeString::println(unsigned char b, int base) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(b, base);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

size_t SafeString::println(int num, int base) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

size_t SafeString::println(unsigned int num, int base) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

size_t SafeString::println(long num, int base) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

size_t SafeString::println(unsigned long num, int base) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  size_t n = temp.Print::print(num, base);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

size_t SafeString::println(double num, int digits) {
  createSafeString(temp, 8 * sizeof(long) + 4); // null + sign + nl
  if (digits > 18) {
    digits = 18;
  }
  size_t n = temp.Print::print(num, digits);
  concatln(temp.buffer, temp.len); // calls cleanUp()
  return n + 2;
}

/*** end of write() and print..() methods for Print support ***********/


/*********************************************/
/* No Memory Management methods              */
/*********************************************/
// just checks there is enough spare space
unsigned char SafeString::reserve(unsigned int size) {
  if (_capacity >= size) { // buffer never NULL
    return true;
  }
  return false; // error
}
/**** end of memory mamagement methods *****/

/*************************************************/
/**  assignment operator methods                 */
/*************************************************/
SafeString & SafeString::operator = (SafeString &rhs) {
  rhs.cleanUp();
  cleanUp();
  if (buffer == rhs.buffer) { // allow for same buffer in different SafeStrings
    return *this;
  }
  return concatInternal(rhs.buffer, true);
}

SafeString & SafeString::operator = (char c) {
  return concatInternal(c, true);
}

SafeString & SafeString::operator = (const char *cstr) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error:"));
      outputName();
      debugPtr->print(F(" = NULL pointer "));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  } // else {
  if (cstr == buffer) { // assign to itself
    return *this;
  }
  return concatInternal(cstr, strlen(cstr), true);
}

SafeString & SafeString::operator = (const __FlashStringHelper *pstr) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error:"));
      outputName();
      debugPtr->print(F(" = NULL F( ) ptr "));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  } // else
  return concatInternal(pstr, true);
}

SafeString & SafeString::operator = (unsigned char c) {
  printInternal((unsigned long)c, DEC, true);
  return *this;
}

SafeString & SafeString::operator = (int num) {
  printInternal((long)num, DEC, true);
  return *this;
}

SafeString & SafeString::operator = (unsigned int num) {
  printInternal((unsigned long)num, DEC, true);
  return *this;
}

SafeString & SafeString::operator = (long num) {
  printInternal(num, DEC, true);
  return *this;
}

SafeString & SafeString::operator = (unsigned long num) {
  printInternal(num, DEC, true);
  return *this;
}

SafeString & SafeString::operator = (float num) {
  printInternal(num, 2, true);
  return *this;
}

SafeString & SafeString::operator = (double num) {
  printInternal(num, 2, true);
  return *this;
}
/**********  assignment operator methods *************/


/*************************************************/
/**  prefix methods                              */
/** for the -= prefix operator methods           */
/**    see the SafeString.h file                 */
/*************************************************/
SafeString & SafeString::prefix(char c) {
  cleanUp();
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" of '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  char buf[2];
  buf[0] = c;
  buf[1] = 0;
  return prefix(buf, 1);
}

SafeString & SafeString::prefix(const char *cstr, size_t length) {
  cleanUp();
  size_t newlen = len + length;
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  if (length == 0) {
    return *this;
  }
  if (length > limitedStrLen(cstr, length)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" length ")); debugPtr->print(length); debugPtr->print(F(" > char* arg strlen."));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(cstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }

  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("prefix"), newlen, cstr, NULL, '\0', length);
#endif // SSTRING_DEBUG
    return *this;
  }
  memmove(buffer + length, buffer, len);
  if (cstr == buffer) {
    // prepending to ourselves so stop here
  } else {
    memmove(buffer, cstr, length);
  }
  len = newlen;
  buffer[len] = '\0';
  return *this;
}

SafeString & SafeString::prefix(const __FlashStringHelper * pstr) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  return prefix(pstr, strlen_P((PGM_P)pstr));
}

SafeString & SafeString::prefix(const __FlashStringHelper * pstr, size_t length) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  if (length == 0) {
    return *this;
  }
  if (length > strlen_P((PGM_P)pstr)) { // what if not null terminated
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" length ")); debugPtr->print(length); debugPtr->print(F(" > F() arg strlen."));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(pstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }

  size_t newlen = len + length;
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    capError(F("prefix"), newlen, NULL, pstr, '\0', length);
#endif // SSTRING_DEBUG
    return *this;
  }
  memmove(buffer + length, buffer, len);
  memcpy_P(buffer, (const char *) pstr, length);
  len = newlen;
  buffer[len] = '\0';
  return *this;
}

// you can prefix to your self if there is enough room,  i.e. str.prefix(str);
SafeString & SafeString::prefix(SafeString &s) {
  s.cleanUp();
  cleanUp();
  return prefix(s.buffer, s.len);
}

SafeString & SafeString::prefix(const char *cstr) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      prefixErr();
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  } // else
  return prefix(cstr, strlen(cstr));
}

SafeString & SafeString::prefix(unsigned char num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned char));
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(int num) {
  createSafeString(temp, 2 + 3 * sizeof(int));
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(unsigned int num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned int));
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(long num) {
  createSafeString(temp, 2 + 3 * sizeof(long));
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(unsigned long num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned long));
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(float num) {
  createSafeString(temp, 22);
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}

SafeString & SafeString::prefix(double num) {
  createSafeString(temp, 22);
  temp.print(num);
  return prefix(temp); // calls cleanUp()
}
/******** end of prefix methods **************************/


/*************************************************/
/**  concat methods                              */
/** for the += concat operator methods           */
/**    see the SafeString.h file                 */
/*************************************************/
SafeString & SafeString::concat(char c) {
  return concatInternal(c);
}
SafeString & SafeString::concat(const char *cstr) {
  return concatInternal(cstr);
}

SafeString & SafeString::concat(const char *cstr, size_t length) {
  return concatInternal(cstr, length, false); // calls cleanUp()
}

// you can concat to yourself if there is enough room, i.e. str.concat(str);
SafeString & SafeString::concat(SafeString &s) {
  s.cleanUp();
  return concat(s.buffer, s.len);
}


SafeString & SafeString::concat(unsigned char num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned char));
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(int num) {
  createSafeString(temp, 2 + 3 * sizeof(int));
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(unsigned int num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned int));
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(long num) {
  createSafeString(temp, 2 + 3 * sizeof(long));
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(unsigned long num) {
  createSafeString(temp, 2 + 3 * sizeof(unsigned long));
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(float num) {
  createSafeString(temp, 22);
  temp.print(num);
  return concat(temp); // calls cleanUp()
}

SafeString & SafeString::concat(double num) {
  createSafeString(temp, 22);
  temp.print(num);
  return concat(temp); // calls cleanUp()
}
SafeString & SafeString::concat(const __FlashStringHelper * pstr) {
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("concat"));
      debugPtr->print(F(" was passed a NULL F( ) pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  return concatInternal(pstr);
}

// concat at most length chars
SafeString & SafeString::concat(const __FlashStringHelper * pstr, size_t length) {
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("concat"));
      debugPtr->print(F(" was passed a NULL F( ) pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return *this;
  }
  return concatInternal(pstr, length, false);
}

// ============== internal concat methods
// concat at most length chars from cstr
// this method applies assignOp
SafeString & SafeString::concatInternal(const char *cstr, size_t length, bool assignOp) {

  cleanUp();
  size_t newlen = len + length;
  if (assignOp) {
    newlen = length;
  }
  if (!cstr)  {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        assignErrorMethod();
      }  else {
        concatErr();
      }
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (length == 0) {
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (length > limitedStrLen(cstr, length)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        assignErrorMethod();
      }  else {
        concatErr();
      }
      debugPtr->print(F(" length ")); debugPtr->print(length); debugPtr->print(F(" > char* arg strlen."));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(cstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (assignOp) {
      assignError(newlen, cstr, NULL);
    }  else {
      capError(F("concat"), newlen, cstr, NULL, '\0', length);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (assignOp) {
    clear();
  }
  memmove(buffer + len, cstr, length);
  len = newlen;
  buffer[len] = '\0';
  return *this;
}

// concat at most length chars
// this method applies assignOp
SafeString & SafeString::concatInternal(const __FlashStringHelper * pstr, size_t length, bool assignOp) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        assignErrorMethod();
      }  else {
        concatErr();
      }
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (length == 0) {
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (length > strlen_P((PGM_P)pstr)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        assignErrorMethod();
      }  else {
        concatErr();
      }
      debugPtr->print(F(" length ")); debugPtr->print(length); debugPtr->print(F(" > F() arg strlen."));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(pstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }

  size_t newlen = len + length;
  if (assignOp) {
    newlen = length;
  }
  if (!reserve(newlen)) {
    setError();
#ifdef SSTRING_DEBUG
    if (assignOp) {
      assignError(newlen, NULL, pstr);
    }  else {
      capError(F("concat"), newlen, NULL, pstr, '\0', length);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if (assignOp) {
    clear();
  }
  memcpy_P(buffer + len, (PGM_P)pstr, length);
  len = newlen;
  buffer[len] = '\0';
  return *this;
}

SafeString & SafeString::concatInternal(char c, bool assignOp) {
  cleanUp();
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        concatAssignError();
      } else {
        concatErr();
      }
      debugPtr->print(F(" of '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  if ((assignOp) && (_capacity < 1)) { // && (c != '\0')) {
    setError();
#ifdef SSTRING_DEBUG
    assignError(1, NULL, NULL, c);
#endif
    if (assignOp) {
      clear();
    }
    return *this;
  }
  char buf[2];
  buf[0] = c;
  buf[1] = 0;
  return concatInternal(buf, strlen(buf), assignOp);
}


SafeString & SafeString::concatInternal(const char *cstr, bool assignOp) {
  cleanUp();

  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        concatAssignError();
      } else {
        concatErr();
      }
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  } // else
  return concatInternal(cstr, strlen(cstr), assignOp);
}

SafeString & SafeString::concatInternal(const __FlashStringHelper * pstr, bool assignOp) {
  cleanUp();
  if (!pstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      if (assignOp) {
        concatAssignError();
      } else {
        concatErr();
      }
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    if (assignOp) {
      clear();
    }
    return *this;
  }
  return concatInternal(pstr, strlen_P((PGM_P)pstr), assignOp);
}


/******** end of concat methods **************************/

/*********************************************/
/**  Concatenate                             */
/*********************************************/
// sfStr = str + 5 etc not supported because of the need to allocate/reallocat buffers
// BUT
//   (sfStr = str) += 5;
// works because = and += and -= all return a reference to the updated SafeString
// i.e. (sfStr = str) += 5; is equvalent to pair of statements
//   sfStr = str;
//   sfStr += 5;

/*************************************************/
/** Comparison methods                           */
/** for the > >= < <= == !=  operator methods    */
/**    see the SafeString.h file                 */
/*************************************************/
int SafeString::compareTo(SafeString &s) {
  s.cleanUp();
  cleanUp();
  // do a quick check of the lengths
  if (len < s.len) {
    return -1;
  } else if (len > s.len) {
    return +1;
  } //else
  return strcmp(buffer, s.buffer);
}

int SafeString::compareTo(const char* cstr) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("compareTo"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return 1; // > for NULL
  }
  return strcmp(buffer, cstr);
}

unsigned char SafeString::equals(SafeString &s2) {
  s2.cleanUp();
  cleanUp();
  return ((len == s2.len) && (strcmp(buffer, s2.buffer) == 0));
}

// error if cstr is NULL
unsigned char SafeString::equals(const char *cstr) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("equals"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // false NULL never matched any non-null SafeString since buffer is never NULL
  }
  if (len == 0) {
    return (*cstr == 0);
  } // else
  return strcmp(buffer, cstr) == 0;
}

// compare string to char
unsigned char SafeString::equals(const char c) {
  cleanUp();
  if (c == '\0') {
    if (len == 0) {
      return true;
    } else {
      return false;
    }
  } else {
    if (len != 1) {
      return false;
    }
  }
  // else
  return buffer[0] == c;
}

unsigned char SafeString::equalsIgnoreCase(const char *str2) {
  cleanUp();
  if (!str2) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("equalsIgnoreCase"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // false for null
  }
  if (buffer == str2) {
    return true; // same as buffer
  }
  size_t str2Len = strlen(str2);
  if (len != str2Len) {
    return false;
  }
  if (len == 0) {
    return true;
  }
  const char *p1 = buffer;
  const char *p2 = str2;
  while (*p1) {
    if (tolower(*p1++) != tolower(*p2++)) {
      return false;
    }
  }
  return true;
}

unsigned char SafeString::equalsIgnoreCase( SafeString &s2 ) {
  s2.cleanUp();
  cleanUp();
  if (buffer == s2.buffer) { // allow for same buffer in different SafeStrings
    return true;
  }
  if (len != s2.len) {
    return false;
  }
  if (len == 0) {
    return true;
  }
  const char *p1 = buffer;
  const char *p2 = s2.buffer;
  while (*p1) {
    if (tolower(*p1++) != tolower(*p2++)) {
      return false;
    }
  }
  return true;
}

unsigned char SafeString::equalsConstantTime(SafeString &s2) {
  s2.cleanUp();
  cleanUp();
  // To avoid possible time-based attacks present function
  // compares given strings in a constant time.
  if (len != s2.len) {
    return false;
  }
  //at this point lengths are the same
  if (len == 0) {
    return true;
  }
  //at this point lenghts are the same and non-zero
  const char *p1 = buffer;
  const char *p2 = s2.buffer;
  size_t equalchars = 0;
  size_t diffchars = 0;
  while (*p1) {
    if (*p1 == *p2)
      ++equalchars;
    else
      ++diffchars;
    ++p1;
    ++p2;
  }
  //the following should force a constant time eval of the condition without a compiler "logical shortcut"
  bool equalcond = (equalchars == len);
  bool diffcond = (diffchars == 0);
  return (unsigned char)(equalcond & diffcond); //bitwise AND
}
/******** end of comparison methods **************************/

/*********************************************/
/** startsWith(), endsWith()  methods        */
/*********************************************/
unsigned char SafeString::startsWith(const char c, unsigned int fromIndex) {
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWith"));
      debugPtr->print(F(" was passed \\0"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  char str[2];
  str[0] = c;
  str[1] = '\0';
  return startsWith(str,fromIndex);
}


unsigned char SafeString::startsWith( const char *str2, unsigned int fromIndex ) {
  cleanUp();
  if (!str2) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWith"));
      debugPtr->print(F(" was passed a NULL pointer"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if (fromIndex == ((unsigned int)(-1))) {
    fromIndex = len;
  }
  size_t str2Len = strlen(str2);
  // zero length is OK
  /*********
    if (str2Len == 0) {
      setError();
    #ifdef SSTRING_DEBUG
      if (debugPtr) {
        errorMethod(F("startsWith"));
        debugPtr->print(F(" was passed an empty char array"));
        outputFromIndexIfFullDebug(fromIndex);
        debugInternalMsg(fullDebug);
      }
    #endif // SSTRING_DEBUG
      return false;
    }
  *********/
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWith"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(str2); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if ((fromIndex + str2Len) > len ) {
    return false;
  }
  if (str2Len == 0) {
    if (fromIndex == len) {
      return true;
    } else {
      return false;
    }
  } else {
    // str2Len != 0
    //(fromIndex + str2Len) > len check for fromIndex == len
  }
  return strncmp( &buffer[fromIndex], str2, str2Len ) == 0;
}

unsigned char SafeString::startsWith( SafeString &s2, unsigned int fromIndex ) {
  s2.cleanUp();
  cleanUp();
  // zero length is OK
  /**
    if (s2.len == 0) {
    setError();
    #ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWith"));
      debugPtr->print(F(" was passed an empty SafeString ")); s2.outputName();
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
    #endif // SSTRING_DEBUG
    return false;
    }
  **/
  if (fromIndex == ((unsigned int)(-1))) {
    fromIndex = len;
  }

  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWith"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(s2.buffer); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if ((fromIndex + s2.len) > len ) {
    return false;
  }
  if (s2.len == 0) {
    if (fromIndex == len) {
      return true;
    } else {
      return false;
    }
  } else {
    //  s2.len != 0
    //  (fromIndex + s2.len) > len  checks for fromIdx == len
  }

  return strncmp( &buffer[fromIndex], s2.buffer, s2.len ) == 0;
}

unsigned char SafeString::startsWithIgnoreCase(const char c, unsigned int fromIndex) {
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWithIgnoreCase"));
      debugPtr->print(F(" was passed \\0"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  char str[2];
  str[0] = c;
  str[1] = '\0';
  return startsWithIgnoreCase(str,fromIndex);
}

unsigned char SafeString::startsWithIgnoreCase( SafeString &s2, unsigned int fromIndex ) {
  s2.cleanUp();
  cleanUp();
  // zero length is OK
  /**
    if (s2.len == 0) {
    setError();
    #ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWithIgnoreCase"));
      debugPtr->print(F(" was passed an empty SafeString ")); s2.outputName();
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
    #endif // SSTRING_DEBUG
    return false;
    }
  */
  if (fromIndex == ((unsigned int)(-1))) {
    fromIndex = len;
  }

  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWithIgnoreCase"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(s2.buffer); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if ((fromIndex + s2.len) > len) {
    return false;
  }
  if (s2.len == 0) {
    if (fromIndex == len) {
      return true;
    } else {
      return false;
    }
  } else {
    // s2.len != 0
    // and (fromIndex + str2Len) > len checks for fromIdx == len
  }

  const char *p1 = &buffer[fromIndex];
  const char *p2 = s2.buffer;
  while (*p2) { // loop through str2 have check lengths above
    if (tolower(*p1++) != tolower(*p2++)) {
      return false;
    }
  }
  return true;
}

// return 0 false 1 true
unsigned char SafeString::startsWithIgnoreCase( const char *str2, unsigned int fromIndex ) {
  cleanUp();
  if (!str2) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWithIgnoreCase"));
      debugPtr->print(F(" was passed a NULL pointer"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }

  size_t str2Len = strlen(str2);
  if (fromIndex == ((unsigned int)(-1))) {
    fromIndex = len;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("startsWithIgnoreCase"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(str2); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if ((fromIndex + str2Len) > len) {
    return false;
  }
  if (str2Len == 0) {
    if (fromIndex == len) {
      return true;
    } else {
      return false;
    }
  } else {
    // str2Len != 0
    // and (fromIndex + str2Len) > len checks for fromIdx == len
  }

  const char *p1 = &buffer[fromIndex];
  const char *p2 = str2;
  while (*p2) { // loop through str2 have check lengths above
    if (tolower(*p1++) != tolower(*p2++)) {
      return false;
    }
  }
  return true;
}

unsigned char SafeString::endsWith( SafeString &s2 ) {
  s2.cleanUp();
  cleanUp();
  if (buffer == s2.buffer) {
    return true; // same SafeString
  }
  if ( len < s2.len ) {
    return false;
  }
  // strcmp works with empty versus empty
  if (s2.len == 0) {
    if (len == 0) {
      return true;
    } else {
      return false;
    }
  }
  /**
    if (len == 0) {
    if (s2.len == 0) {
      return true;
    } else {
      return false;
    }
    }
  **/
  return strcmp(&buffer[len - s2.len], s2.buffer) == 0;
}

unsigned char SafeString::endsWith(const char c) {
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("endsWith"));
      debugPtr->print(F(" was passed \\0"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  char str[2];
  str[0] = c;
  str[1] = '\0';
  return endsWith(str);
}

unsigned char SafeString::endsWith(const char *suffix) {
  cleanUp();
  if (!suffix) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("endsWith"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  size_t str2Len = strlen(suffix);
  if (len < str2Len ) {
    return false;
  }
  // strcmp works with empty versus empty

  if (str2Len == 0) {
    if (len == 0) {
      return true;
    } else {
      return false;
    }
  }
  /**
    if (len == 0) {
    if (str2Len == 0) {
      return true;
    } else {
      return false;
    }
    }
  **/
  return strcmp(&buffer[len - str2Len], suffix) == 0;
}

unsigned char SafeString::endsWithCharFrom(SafeString &s2) {
  s2.cleanUp();
  cleanUp();
  return endsWithCharFrom(s2.buffer);
}

unsigned char SafeString::endsWithCharFrom(const char *suffix) {
  cleanUp();
  if (!suffix) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("endsWithCharFrom"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  size_t str2Len = strlen(suffix);
  if (len == 0) {
    if (str2Len == 0) {
      return true;
    } else {
      return false;
    }
  } else {
    // else len != 0
    if (str2Len == 0) {
      return false;
    }
  }
  char c = buffer[len - 1];
  if (strchr(suffix, c) != NULL) {
    return true;
  } // else
  return false;
}

/*******  end of startsWith(), endsWith() methods ***********/

/*********************************************/
/**  Character Access                        */
/*********************************************/
char SafeString::charAt(unsigned int index) {
  cleanUp();
  if (index >= len ) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error: "));
      outputName();
      debugPtr->print(F(".charAt() index ")); debugPtr->print(index); debugPtr->print(F(" >= ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return '\0';
  }
  return buffer[index];
}

void SafeString::setCharAt(unsigned int index, char c) {
  cleanUp();
  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error:"));
      outputName();
      debugPtr->print(F(".setCharAt("));
      debugPtr->print(index); debugPtr->print(F(",'\\0');"));
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));  debugPtr->print(F(" Setting character to '\\0' not allowed."));
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  if (index >= len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error:"));
      outputName();
      debugPtr->print(F(".setCharAt() index ")); debugPtr->print(index); debugPtr->print(F(" >= ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(c); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  buffer[index] = c;
  return;
}

char SafeString::operator[]( unsigned int index ) {
  cleanUp();
  if (index >= len ) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      debugPtr->print(F("Error: "));
      outputName();
      debugPtr->print(F("[] index ")); debugPtr->print(index); debugPtr->print(F(" >= ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return '\0';
  }
  return buffer[index];
}

const char* SafeString::c_str() {
  cleanUp();
  // mark as subject to external modification now as we have exposed the internal buffer
  fromBuffer = true;  // added V3.0.4 will check overflow in return cleanUp and set errorFlag
  return buffer;
}

/****  end if character access methods *******************/

/*************************************************/
/**  Search methods  indexOf() lastIndexOf()     */
/*************************************************/
/**
    Search
       Arrays are indexed by a unsigned int variable
       See the SafeStringIndexOf.ino example sketch
      All indexOf methods return -1 if not found or on error
**********************************************/
// The fromIndex is offset into this SafeString where to start searching (inclusive)
// 0 to length() and -1 is valid for fromIndex
// if fromIndex > length(), than the error flag is set and -1 returned and prints an error if debug enabled
// if fromIndex == (unsigned int)(-1) -1 is returned without error.
//int SafeString::indexOf(char c) {
//  return indexOf(c, 0); // calls cleanUp()
//}

int SafeString::indexOf( char c, unsigned int fromIndex ) {
  cleanUp();

  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" >= ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(c); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  if (c == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" char arg was '\\0'"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
  }

  const char* temp = strchr(buffer + fromIndex, c);
  if (temp == NULL) {
    return -1; // not found
  }
  return temp - buffer;
}

/**
int SafeString::indexOf(SafeString &s2) {
  s2.cleanUp();
  cleanUp();
  if (s2.len == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" was passed an empty SafeString ")); s2.outputName();
      outputFromIndexIfFullDebug(0);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  //  if ((buffer == s2.buffer) && (len != 0)) {
  //    return 0; // same SafeString
  //  }
  return indexOf(s2, 0);
}

**/

int SafeString::indexOf(SafeString &s2, unsigned int fromIndex) {
  s2.cleanUp();
  cleanUp();
  if (s2.len == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" was passed an empty SafeString ")); s2.outputName();
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    //return -1;
  }

  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(s2.buffer); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  if (len == 0)  {
    return -1;
  }

  const char *found = strstr(buffer + fromIndex, s2.buffer);
  if (found == NULL) {
    return -1;
  }
  return found - buffer;
}

/**
int SafeString::indexOf( const char* str ) {
  return indexOf(str, 0); // calls cleanUp()
}
**/

int SafeString::indexOf(const char* cstr , unsigned int fromIndex) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" was passed a NULL pointer"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  size_t cstrLen = strlen(cstr);
  if (cstrLen == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" was passed an empty char array"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    // return -1;
  }

  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(cstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  if (len == 0)  {
    return -1;
  }

  const char *found = strstr(buffer + fromIndex, cstr);
  if (found == NULL) {
    return -1;
  }
  return (int)(found - buffer);
}

int SafeString::lastIndexOf( char theChar ) {
  return lastIndexOf(theChar, len - 1); // calls cleanUp() // if len == 0, len-1 == (unsigned int)-1
}

int SafeString::lastIndexOf(char ch, unsigned int fromIndex) {
  cleanUp();
  if (ch == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" char arg was '\\0'"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    //return -1; // probabily an error
  }
  if (len == 0) {
    return -1;
  }

  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(ch); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  char tempchar = buffer[fromIndex + 1];
  buffer[fromIndex + 1] = '\0';
  char* temp = strrchr( buffer, ch );
  buffer[fromIndex + 1] = tempchar;
  if (temp == NULL) {
    return -1;
  }
  return temp - buffer;
}


int SafeString::lastIndexOf(SafeString &s2) {
  s2.cleanUp();
  cleanUp();
  //  if (len < s2.len) {
  //    return -1;
  //  } // else len - s2.len is valid
  return lastIndexOf(s2, len - s2.len);
}


int SafeString::lastIndexOf( const char *cstr ) {
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" was passed a NULL pointer"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  size_t cstrlen = strlen(cstr);
  //  if (len < cstrlen) {
  //    return -1;
  //  } // else len - strlen(cstr) is valid
  return lastIndexOf(cstr, len - cstrlen);
}


int SafeString::lastIndexOf(SafeString &s2, unsigned int fromIndex) {
  s2.cleanUp();
  cleanUp();
  if (s2.len == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" was passed an empty SafeString ")); s2.outputName();
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  if (len == 0) {
    return -1;
  }
  //    if (s2.len == 0 || len == 0 || s2.len > len) return -1;
  //  if (s2.len > len) { // check below
  //      return -1;
  //  }
  if (fromIndex == (unsigned int)(-1)) {
    fromIndex = len;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(s2.buffer); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    //    return -1;
  }
  if (fromIndex >= len) fromIndex = len - 1; // len == 0 handled above
  if (s2.len > len) {
    return -1;
  }
  const char* cstr = s2.buffer;
  int found = -1;
  for (char *p = buffer; p <= buffer + fromIndex; p++) {
    p = strstr(p, cstr);
    if (!p) { // not found
      break;
    } // else
    if ((unsigned int)(p - buffer) <= fromIndex) {
      found = p - buffer;
    }
  }
  return found;
}

int SafeString::lastIndexOf(const char* cstr, unsigned int fromIndex) {
  cleanUp();
  if (!cstr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" was passed a NULL pointer"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  size_t cstrlen = strlen(cstr);
  if (cstrlen == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" was passed an empty char array"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  if (len == 0) {
    return -1;
  }
  //    if (s2.len == 0 || len == 0 || s2.len > len) return -1;
  //  if (cstrlen> len) { // check below
  //      return -1;
  //  }
  if (fromIndex == (unsigned int)(-1)) {
    fromIndex = len;
  }

  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("lastIndexOf"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(cstr); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }

  if (fromIndex >= len) fromIndex = len - 1; // len == 0 handled above
  if (cstrlen > len) {
    return -1;
  }

  int found = -1;
  for (char *p = buffer; p <= buffer + fromIndex; p++) {
    p = strstr(p, cstr);
    if (!p) { // not found
      break;
    } // else
    if ((unsigned int)(p - buffer) <= fromIndex) {
      found = p - buffer;
    }
  }
  return found;
}

/*
  find first index of one of the chars in the arg

int SafeString::indexOfCharFrom(SafeString &str) {
  str.cleanUp();
  return indexOfCharFrom(str.buffer, 0); // calls cleanUp()
}
**/

int SafeString::indexOfCharFrom(SafeString &str, unsigned int fromIndex) {
  str.cleanUp();
  return indexOfCharFrom(str.buffer, fromIndex); // calls cleanUp()
}

/**
int SafeString::indexOfCharFrom(const char* chars) {
  return indexOfCharFrom(chars, 0); // calls cleanUp()
}
**/

int SafeString::indexOfCharFrom(const char* chars, unsigned int fromIndex) {
  cleanUp();
  if (!chars) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOfCharFrom"));
      debugPtr->print(F(" was passed a NULL pointer"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  size_t charsLen = strlen(chars);
  if (charsLen == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOfCharFrom"));
      debugPtr->print(F(" was passed an empty set of chars"));
      outputFromIndexIfFullDebug(fromIndex);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1;
  }
  if (fromIndex > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("indexOfCharFrom"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(chars); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  if (len == 0)  {
    return -1;
  }
  int minIdx = len; // not found
  const char* cPtr = chars;
  while (*cPtr) {
    int idx = indexOf(*cPtr, fromIndex);
    if (idx == 0) {
      return 0; // found min
    } else if (idx > 0)  {
      if (idx < minIdx) {
        minIdx = idx; // update new min
      } // else nothing
    } // else idx < 0 not found
    cPtr++;
  }
  if (minIdx == (int)len) {
    return -1;
  }
  return minIdx;
}

/****  end of Search methods  *******************************/

/*************************************************/
/**  substring methods                           */
/*************************************************/
// substring is from beginIdx to end of string
// The result substring is ALWAYS cleared by this method
// if beginIdx = length(), an empty result will be returned with out error
// if beginIdx > length(), an empty result will be returned with error flag set on both this SafeString and the result SafeString
// beginIdx == (unsigned int)(-1) returns an empty result without an error
// can take substring of yourself  e.g. str.substring(str,3);
// if result does not have the capacity to hold the substring, hasError() is set on both this SafeString and the result SafeString
SafeString & SafeString::substring(SafeString &result, unsigned int beginIdx) {
  result.cleanUp();
  cleanUp();
  if ( (beginIdx == (unsigned int)(-1)) || ((len == 0) && (beginIdx == 0)) ) {
    result.clear();
    return result;
  }

  if (beginIdx > len) { //== len is OK
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("substring"));
      debugPtr->print(F(" beginIdx ")); debugPtr->print(beginIdx); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    result.clear();
    return result; // no change
  }
  // else  (len > 0) {
  return substring(result, beginIdx, len);
}


// The result substring is ALWAYS cleared by this method
// if beginIdx >= length(), an empty result will be returned
// beginIdx and endIdx will be swapped so that beginIdx <= endIdx and the error flag is set on both this SafeString and the result SafeString
// if endIdx > length(), endIdx is set to length(); and the error flag is set on both this SafeString and the result SafeString
// endIdx == (unsigned int)(-1) is treated as endIdx == length() returns a result without an error
// substring is from beginIdx to endIdx-1, endIdx is exclusive
// can take substring of yourself  e.g. str.substring(str,3,6);
// if result does not have the capacity to hold the substring, and empty result is returned and hasError() is set on both this SafeString and the result SafeString
SafeString & SafeString::substring(SafeString &result, unsigned int beginIdx, unsigned int endIdx) {
  result.cleanUp();
  cleanUp();
  if ((len == 0) && (beginIdx == 0) && (endIdx == 0)) {
    result.clear();
    return result;
  }

  if (beginIdx == (unsigned int)(-1)) {
    beginIdx = len;
  }
  if (endIdx == (unsigned int)(-1)) {
    endIdx = len;
  }

  if (beginIdx > endIdx) {
    unsigned int temp = endIdx;
    endIdx = beginIdx;
    beginIdx = temp;
    setError();
    result.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      warningMethod(F("substring"));
      debugPtr->print(F(" SafeString")); outputName(); debugPtr->print(F(" beginIdx > endIdx "));
      debugInternalResultMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
  }
  // continue

  if (endIdx > len) {
    setError();
    result.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      warningMethod(F("substring"));
      debugPtr->print(F(" SafeString")); outputName(); debugPtr->print(F(" endIdx > length() "));
      debugInternalResultMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    endIdx = len;
    if (beginIdx > len) {
      beginIdx = len;
    }
  }

  // here beginIdx <= endIdx AND endidx <= len
  if ((beginIdx == len) || (beginIdx == endIdx)) {
    result.clear();
    return result; // cleared
  }
  // copy to result
  size_t copyLen = endIdx - beginIdx; // endIdx is exclusive 5,5 copies 0 chars 5,6 copies 1 char char[5].
  if (copyLen > result.capacity()) {
    setError();
    result.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("substring"));
      debugPtr->print(F(" result SafeString")); result.outputName(); debugPtr->print(F(" needs capacity of "));
      debugPtr->print(copyLen);
      result.debugInternalResultMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    result.clear();
    return result; // cleared
  }
  // memmove incase result and SafeString are the same
  memmove(result.buffer, buffer + beginIdx, copyLen);
  result.len = copyLen;
  result.buffer[result.len] = '\0';
  return result;
}
/*****  end of substring methods  *************************/


/*********************************************************/
/**  Modification replace(), remove(), removeLast()      */
/*********************************************************/

/*****  replace(), methods ***********/
// replace single char with another
void SafeString::replace(char f, char r) {
  cleanUp();
  if (len == 0) {
    return;
  }
  if (f == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find char is '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  if (r == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" replace char is '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  for (char *p = buffer; *p; p++) {
    if (*p == f) {
      *p = r;
    }
  }
  return;
}

// replace sequence of chars with another sequence (case sensitive)
void SafeString::replace(const char findChar, SafeString& r) {
  r.cleanUp();
  cleanUp();
  if (findChar == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find char is '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }

  char findChars[] = {findChar, 0};
  replace(findChars, r.buffer);
}

// replace sequence of chars with another sequence (case sensitive)
void SafeString::replace(SafeString& f, SafeString& r) {
  f.cleanUp();
  r.cleanUp();
  cleanUp();
  if (f.len == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find SafeString")); f.outputName(); debugPtr->print(F(" is empty."));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  replace(f.buffer, r.buffer);
}

void SafeString::replace(const char findChar, const char *replacePtr) {
  if (findChar == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find char is '\\0'"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  char findChars[] = {findChar, 0};
  replace(findChars, replacePtr);
}

void SafeString::replace(const char* findStr, const char *replacePtr) {
  cleanUp();
  if (!findStr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find arg is NULL"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  if (!replacePtr) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" replace arg is NULL"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }

  size_t findLen = strlen(findStr);
  size_t replaceLen = strlen(replacePtr);
  if (len == 0) {
    return;
  }
  if (replacePtr == buffer) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" replace arg is same SafeString"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  if (findLen == 0) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("replace"));
      debugPtr->print(F(" find ")); debugPtr->print(F(" is empty."));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  /**
    if (findStr == buffer) {
      setError();
    #ifdef SSTRING_DEBUG
      if (debugPtr) {
        errorMethod(F("replace"));
        debugPtr->print(F(" find arg is same SafeString"));
        debugInternalMsg(fullDebug);
      }
    #endif // SSTRING_DEBUG
      return;
    }
  **/
  int diff = replaceLen - findLen;
  char *_readFrom = buffer;
  char *foundAt;
  if (diff == 0) {
    while ((foundAt = strstr(_readFrom, findStr)) != NULL) {
      memmove(foundAt, replacePtr, replaceLen);
      _readFrom = foundAt + replaceLen; // prevents replacing the replace
    }
  } else if (diff < 0) {
    char *writeTo = buffer;
    while ((foundAt = strstr(_readFrom, findStr)) != NULL) {
      size_t n = foundAt - _readFrom;
      memmove(writeTo, _readFrom, n);
      writeTo += n;
      memmove(writeTo, replacePtr, replaceLen);
      writeTo += replaceLen;
      _readFrom = foundAt + findLen; // prevents replacing the replace
      len += diff;
    }
    memmove(writeTo, _readFrom, strlen(_readFrom) + 1);
  } else {
    size_t newlen = len; // compute size needed for result
    while ((foundAt = strstr(_readFrom, findStr)) != NULL) {
      _readFrom = foundAt + findLen;
      newlen += diff;
    }
    if (!reserve(newlen)) {
      setError();
#ifdef SSTRING_DEBUG
      if (debugPtr) {
        capError(F("replace"), newlen, findStr);
        if (fullDebug) {
          debugPtr->print(F("       "));
          debugPtr->print(F(" Replace arg was '")); debugPtr->print(replacePtr); debugPtr->println('\'');
        }
      }
#endif // SSTRING_DEBUG
      return;
    }

    size_t index = len - 1; // len checked for != above
    while ((index = lastIndexOf(findStr, index)) < len) {
      _readFrom = buffer + index + findLen;
      memmove(_readFrom + diff, _readFrom, len - (_readFrom - buffer));
      int newLen = len + diff;
      memmove(buffer + index, replacePtr, replaceLen);
      len = newLen;
      buffer[newLen] = 0;
      if (index == 0) {
        break; // at front of string
      } // else
      index--;
    }
    len = newlen;
    buffer[newlen] = 0;
  }
  return;
}
/***** end of  replace(), methods ***********/


/***** removeFrom(), keepFrom() remove(), remooveLast(), keepLast() methods ***********/
// remove from index to end of SafeString
// 0 to length() and (unsigned int)(-1) are valid for index,
// -1 => length() for processing and just returns without error
void SafeString::removeFrom(unsigned int startIndex) {
  remove(startIndex);
}

// remove from 0 to startIdx (excluding startIdx)
// 0 to length() and (unsigned int)(-1) are valid for index,
void SafeString::removeBefore(unsigned int index) {
  if (index == (unsigned int)(-1)) {
    clear();
    return;
    /**
        setError();
      #ifdef SSTRING_DEBUG
        if (debugPtr) {
          errorMethod(F("removeBefore"));
          debugPtr->print(F(" index ")); debugPtr->print(index); debugPtr->print(F(" == -1 "));
          debugInternalMsg(fullDebug);
        }
      #endif // SSTRING_DEBUG
        return;
    **/
  }
  remove(0, index); // calls cleanUp()
}

// remove from index to end of SafeString
// 0 to length() and (unsigned int)(-1) are valid for index,
// -1 => length() for processing and just returns without error
void SafeString::remove(unsigned int index) {
  if (index == (unsigned int)(-1)) {
    //index = length();
    return;
  }
  if (index > length()) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("remove"));
      debugPtr->print(F(" index ")); debugPtr->print(index); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  // else
  remove(index, length() - index);
}

// remove count chars starting from index
// 0 to length() and (unsigned int)(-1) are valid for index
// -1 just returns without error
// 0 to (length()- index) is valid for count, larger values set the error flag and remove from idx to end of string
void SafeString::remove(unsigned int index, unsigned int count) {
  cleanUp();
  if (index == (unsigned int)(-1)) {
    index = len;
  }

  if (index > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("remove"));
      debugPtr->print(F(" index ")); debugPtr->print(index); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    index = len;
  }

  if (count > (len - index)) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("remove"));
      debugPtr->print(F(" count ")); debugPtr->print(count); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len); debugPtr->print(F(" - index ")); debugPtr->print(index) ;
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    count = len - index;
  }
  if (count == 0) {
    return;
  }
  char *writeTo = buffer + index;
  memmove(writeTo, buffer + index + count, len - index - count + 1);
  len -= count;
  buffer[len] = 0;
}

// remove the last 'count' chars
// 0 to length() is valid for count,
// count >= length() clears the SafeString
// count > length() set the error flag
void SafeString::removeLast(unsigned int count) {
  cleanUp();
  // else
  if (count > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("removeLast"));
      debugPtr->print(F(" count ")); debugPtr->print(count); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
  }
  if (count >= len) {
    // remove all
    clear();
    return;
  }
  // else
  remove(len - count, count);
}

// keep last 'count' number of chars remove the rest
// 0 to length() is valid for count, passing in count == 0 clears the SafeString
// count > length() sets error flag and returns SafeString unchanged
void SafeString::keepLast(unsigned int count) {
  cleanUp();
  if (count == 0) {
    //just clear string
    clear();
    return;
  }
  // else
  if (count == len) {
    // keep whole string
    return;
  }
  if (count > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("keepLast"));
      debugPtr->print(F(" count ")); debugPtr->print(count); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return;
  }
  // else
  remove(0, len - count);
}

/***** end of removeFrom(), keepFrom() remove(), remooveLast(), keepLast() methods ***********/

/*****  end of Modification replace(), remove(), removeLast()  *******/

/******************************************************/
/** Change case methods, toLowerCase(), toUpperCase() */
/******************************************************/
void SafeString::toLowerCase(void) {
  cleanUp();
  for (char *p = buffer; *p; p++) {
    *p = tolower(*p);
  }
  return;
}

void SafeString::toUpperCase(void) {
  cleanUp();
  for (char *p = buffer; *p; p++) {
    *p = toupper(*p);
  }
  return;
}
/** end of Change case methods, toLowerCase(), toUpperCase() *******/

/******************************************************/
/** trim()                                             */
/******************************************************/
// trim() -- remove white space from front and back of SafeString
// the method isspace( ) is used to.  For the 'C' local the following are trimmed
//    ' '     (0x20)  space (SPC)
//    '\t'  (0x09)  horizontal tab (TAB)
//    '\n'  (0x0a)  newline (LF)
//    '\v'  (0x0b)  vertical tab (VT)
//    '\f'  (0x0c)  feed (FF)
//    '\r'  (0x0d)  carriage return (CR)
void SafeString::trim(void) {
  cleanUp();
  if ( len == 0) {
    return;
  }
  char *begin = buffer;
  while (isspace(*begin)) {
    begin++;
  }
  char *end = buffer + len - 1;
  while (isspace(*end) && end >= begin) {
    end--;
  }
  len = end + 1 - begin;
  if (begin > buffer) {
    memmove(buffer, begin, len);
  }
  buffer[len] = 0;
  return;
}
/** end of trim()  *****************************/

/******************************************************/
/** processBackspaces()                               */
/******************************************************/
// processBackspaces -- recursively remove backspaces, '\b' and the preceeding char
// use for processing inputs from terminal (Telent) connections
void SafeString::processBackspaces(void) {
  cleanUp();
  if ( len == 0) {
    return;
  }
  size_t idx = 0;
  while ((!isEmpty()) && (idx < length())) {
    if (charAt(idx) == '\b') {
      if (idx == 0) {
        remove(idx, 1); // no previous char just remove backspace
      } else {
        idx = idx - 1; // remove previous char and this backspace
        remove(idx, 2);
      }
    } else {
      idx++;
    }
  }
  return;
}
/** end of processBackspaces() **************************/

/*********************************************/
/**  Number Parsing / Conversion  methods    */
/*********************************************/
// convert numbers
// If the SafeString is a valid number update the argument with the result
// else leave the argument unchanged
// SafeString conversions are stricter than the Arduino String version
// trailing chars can only be white space

// convert decimal number to int, arg i unchanged if no valid number found
unsigned char SafeString::toInt(int &i) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  long result = strtol(buffer, &endPtr, 10); // handles 123 (use 0 for 0xAF and 037 (octal))
  if (result > INT_MAX) {
    return false;
  }
  if (result < INT_MIN) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  i = result;
  return true; // OK
}

// convert decimal number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::toLong(long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  long result = strtol(buffer, &endPtr, 10); // handles 123 (use 0 for 0xAF and 037 (octal))
  if (result == LONG_MAX) {
    return false;
  }
  if (result == LONG_MIN) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert binary number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::binToLong(long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  long result = strtol(buffer, &endPtr, 2);
  if (result == LONG_MAX) {
    return false;
  }
  if (result == LONG_MIN) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert octal number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::octToLong(long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  long result = strtol(buffer, &endPtr, 8);
  if (result == LONG_MAX) {
    return false;
  }
  if (result == LONG_MIN) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert hex number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::hexToLong(long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  long result = strtol(buffer, &endPtr, 16); //
  if (result == LONG_MAX) {
    return false;
  }
  if (result == LONG_MIN) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert decimal number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::toUnsignedLong(unsigned long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  unsigned long result = strtoul(buffer, &endPtr, 10); // handles 123 (use 0 for 0xAF and 037 (octal))
  if (result == ULONG_MAX) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert binary number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::binToUnsignedLong(unsigned long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  unsigned long result = strtoul(buffer, &endPtr, 2);
  if (result == ULONG_MAX) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert octal number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::octToUnsignedLong(unsigned long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  unsigned long result = strtoul(buffer, &endPtr, 8);
  if (result == ULONG_MAX) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

// convert hex number to long, arg 1 unchanged if no valid number found
unsigned char SafeString::hexToUnsignedLong(unsigned long &l) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  unsigned long result = strtoul(buffer, &endPtr, 16); //
  if (result == ULONG_MAX) {
    return false;
  }
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  // else all OK
  l = result;
  return true; // OK
}

/**  possible alternative
// convert float number, returns 0.0 and sets error flag not a valid number
float SafeString::toFloat() {
  cleanUp();
  double d;
  if (toDouble(d)) {
    return (float)d; // need to ckeck size here
  } // else
  setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("toFloat()"));
      debugPtr->print(F(" invalid float "));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG  
  return 0.0;
}
****/

// convert float number , arg f unchanged if no valid number found
unsigned char SafeString::toFloat(float  &f) {
  cleanUp();
  double d;
  if (toDouble(d)) {
    f = (float)d; // need to ckeck size here
    return true;
  } // else
  return false;
}

// convert double number , arg d unchanged if no valid number found
unsigned char SafeString::toDouble(double  &d) {
  cleanUp();
  if (len == 0) {
    return false; // not found
  }
  char* endPtr;
  double result = strtod(buffer, &endPtr); // handles 123 (use 0 for 0xAF and 037 (octal))
  // check endPtr to see if number valid 5a is invalid,  5. is valid
  if (endPtr == buffer)  { // no numbers found at all
    return false;
  } // else
  // else check for trailing white space
  while (*endPtr != '\0') {
    if (!isspace(*endPtr)) { // number terminated by white space
      return false;
    }
    endPtr++;
  }
  d = result;
  return true; // OK
}

/** end of Number Parsing / Conversion  methods *****************/


/*******************************************************/
/**  Tokenizing methods,  stoken(), nextToken()        */
/** Differences between stoken() and nextToken
   stoken() leaves the SafeString unchanged, nextToken() removes the token (and leading delimiters) from the SafeString giving space to add more input
   In stoken() the end of the SafeString is always treated as a delimiter, i.e. the last token is returned even if it is not followed by one of the delimiters
   In nextToken() the end of the SafeString is a delimiter by default, but setting returnLastNonDelimitedToken =  false will leave last token that is not terminated in the SafeString
   Setting returnLastNonDelimitedToken = false this allows partial tokens to be read from a Stream and kept until the full token and delimiter is read
*/
/*******************************************************/
/*
     stoken  -- The SafeString itself is not changed
     stoken breaks into the SafeString into tokens using chars in delimiters string and the end of the SafeString as delimiters.
     Any leading delimiters are first stepped over and then the delimited token is return in the token argument (less the delimiter).
     The token argument is always cleared at the start of the stoken().
     if there are any argument errors or the token does not have the capacity to hold the substring, hasError() is set on both this SafeString and the token SafeString

     params
     token - the SafeString to return the token in, it is cleared if no delimited token found or if there are errors
             The found delimited token (less the delimiter) is returned in the token SafeString argument if there is capacity.
             The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.
             If the token's capacity is < the next token, then token is returned empty and an error messages printed if debug is enabled.
             In this case the return (nextIndex) is still updated.
     fromIndex -- where to start the search from  0 to length() and -1 is valid for fromIndex,  -1 => length() for processing
     delimiters - the characters that any one of which can delimit a token. The end of the SafeString is always a delimiter.
     returnEmptyFields -- default false, if true only skip one leading delimiter after each call. If the fromIndex is 0 and there is a delimiter at the beginning of the SafeString, an empty token will be returned
     useAsDelimiters - default true, if false then token consists only of chars in the delimiters and any other char terminates the token

     return -- nextIndex, the next index in this SafeString after the end of the token just found, -1 if this is the last token
              Use this as the fromIndex for the next call
              NOTE: if there are no delimiters then -1 is returned and the whole SafeString returned in token if the SafeString token argument is large enough
              If the token's capacity is < the next token, the token returned is empty and an error messages printed if debug is enabled.
              In this case the returned nextIndex is still updated to end of the token just found so that that the program will not be stuck in an infinite loop testing for nextIndex >=0
              while being consistent with the SafeString's all or nothing insertion rule

     Input argument errors return -1 and an empty token and hasError() is set on both this SafeString and the token SafeString.
**/


int SafeString::stoken(SafeString & token, unsigned int fromIndex, const char delimiter, bool returnEmptyFields, bool useAsDelimiters) {
  token.clear(); // no need to clean up token
  if (!delimiter) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("stoken"));
      debugPtr->print(F(" was passed a '\\0' delimiter"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  return stokenInternal(token, fromIndex, NULL, delimiter, returnEmptyFields,  useAsDelimiters);
}

int SafeString::stoken(SafeString &token, unsigned int fromIndex, SafeString &delimiters, bool returnEmptyFields, bool useAsDelimiters) {
  token.clear(); // no need to clean up token
  delimiters.cleanUp();
  return stoken(token, fromIndex, delimiters.buffer, returnEmptyFields, useAsDelimiters); // calls cleanUp()
}

int SafeString::stoken(SafeString &token, unsigned int fromIndex, const char* delimiters, bool returnEmptyFields, bool useAsDelimiters) {
  token.clear(); // no need to clean up token
  if (!delimiters) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("stoken"));
      debugPtr->print(F(" was passed a NULL pointer for delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  if (*delimiters == '\0') {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("stoken"));
      debugPtr->print(F(" was passed a empty list of delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  return stokenInternal(token, fromIndex, delimiters, '\0', returnEmptyFields,  useAsDelimiters);
}

int SafeString::stokenInternal(SafeString &token, unsigned int fromIndex, const char* delimitersIn, char delimiterIn, bool returnEmptyFields, bool useAsDelimiters) {
  cleanUp();
  token.clear(); // no need to clean up token
  char charDelim[2];
  charDelim[0] = delimiterIn;
  charDelim[1] = '\0';
  const char *delimiters = delimitersIn;
  if (delimiters == NULL) {
    delimiters = charDelim;
  }
  if ((fromIndex == (unsigned int)(-1)) || (fromIndex == len)) {
    return -1; // reached end of input return empty token and -1
    // this is a common case when stepping over delimiters
  }
  // else invalid fromIndex
  if (fromIndex > len) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("stoken"));
      debugPtr->print(F(" fromIndex ")); debugPtr->print(fromIndex); debugPtr->print(F(" > ")); outputName(); debugPtr->print(F(".length() : ")); debugPtr->print(len);
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return -1;
  }
  size_t count = 0;
  // skip leading delimiters  (prior to V2.0.2 leading delimiters not skipped)
  // count will == len-fromIndex if no delimiters found
  if (useAsDelimiters) {
    count = strspn(buffer + fromIndex, delimiters); // count chars ONLY in delimiters
  } else {
    count = strcspn(buffer + fromIndex, delimiters); // count chars NOT in delimiters
  }
  if (returnEmptyFields) {
    // only step over one
    if (count > 0) {
      if (fromIndex == 0) {
        return 1; // leading empty token
      } // else skip over only one of the last delimiters
      count = 1;
    }
  }
  fromIndex += count;
  if (fromIndex == len) {
    return -1; // reached end of input scaning for non-delimiters, return empty token and -1
  }
  // find length of token
  if (useAsDelimiters) {
    count = strcspn(buffer + fromIndex, delimiters); // count chars NOT in delimiters, i.e. the token
  } else {
    count = strspn(buffer + fromIndex, delimiters); // count chars ONLY in delimiters, i.e. the delimiters are the token
  }
  if (count > token._capacity) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("stoken"));
      debugPtr->print(F(" token SafeString ")); token.outputName(); debugPtr->print(F(" needs capacity of "));
      debugPtr->print(count); debugPtr->print(F(" for token '")); debugPtr->write((uint8_t*)(buffer + fromIndex), count); debugPtr->print('\'');
      token.debugInternalResultMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    size_t rtn = fromIndex + count;  // used to be len + 1; prior to V2.0.3
    if (rtn >= len) {
      return -1;
    } else {
      return rtn;
    }
  }
  // else get substring
  substring(token, fromIndex, fromIndex + count);
  size_t rtn = fromIndex + count;  // used to be len + 1; prior to V2.0.3
  if (rtn >= len) {
    return -1;
  } else {
    return rtn;
  }
}
/** end of stoken methods *******************/

    /* nextToken -- The token is removed from the SafeString ********************
      nextToken -- Any leading delimiters are first removed, then the delimited token found is removed from the SafeString.
                   See returnEmptyFields and returnLastNonDelimitedToken arguments below for controls on this.
                   The following delimiters remain in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!
      The token argument is always cleared at the start of the firstToken() and nextToken().
      IMPORTANT!! Changed V4.0.4 By default un-delimited tokens at the end of the SafeString are returned
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.

      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then nextToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiters - the delimiting characters, any one of which can delimit a token
      @param returnEmptyFields -- default false, if true, nextToken() will return true, and an empty token for each consecuative delimiters
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      @param firstToken -- default false, a leading delimiter will be stepped over before looking for a delimited token<br>
      if set to true, a leading delimiter will delimit an empty token which will be returned only if returnEmptyFields is true otherwise it is skipped over.<br>
      NOTE: if returnEmptyFields == false this firstToken argument has no effect.
      NOTE: since the last delimiter is left in the SafeString, you must set firstToken to be false (or omit it) after the first call.

      @return -- true if nextToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br.
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/

unsigned char SafeString::nextToken(SafeString& token, const char delimiter, bool returnEmptyFields, bool returnLastNonDelimitedToken, bool firstToken) {
  cleanUp();
  token.clear();
  if (!delimiter) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("nextToken"));
      debugPtr->print(F(" was passed a '\\0' delimiter"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if (isEmpty()) {
  	  // empty token returned
     return false;
  }
  if (firstToken && (delimiter == charAt(0)) && returnEmptyFields) {
  	  // empty token returned
  	  return  true;
  }
  return nextTokenInternal(token, NULL, delimiter, returnEmptyFields, returnLastNonDelimitedToken);
}

unsigned char SafeString::nextToken(SafeString& token, SafeString &delimiters, bool returnEmptyFields, bool returnLastNonDelimitedToken, bool firstToken) {
  delimiters.cleanUp();
  return nextToken(token, delimiters.buffer, returnEmptyFields, returnLastNonDelimitedToken, firstToken); // calls cleanUp()
}

unsigned char SafeString::nextToken(SafeString& token, const char* delimiters, bool returnEmptyFields, bool returnLastNonDelimitedToken, bool firstToken) {
  cleanUp();
  token.clear();
  if (!delimiters) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("nextToken"));
      debugPtr->print(F(" was passed a NULL pointer for delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if (*delimiters == '\0') {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("nextToken"));
      debugPtr->print(F(" was passed a empty list of delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false;
  }
  if (isEmpty()) {
  	  // empty token returned
     return false;
  }
  if (firstToken && startsWith(delimiters) && returnEmptyFields) {
  	  // empty token returned
  	  return  true; // true if return empty fileds
  }
  return nextTokenInternal(token, delimiters, '\0', returnEmptyFields, returnLastNonDelimitedToken);
}

bool SafeString::nextTokenInternal(SafeString& token, const char* delimitersIn, const char delimiterIn, bool returnEmptyFields, bool returnLastNonDelimitedToken) {
  cleanUp();
  token.clear();
  if (isEmpty()) {
    return false;
  }
  char charDelim[2];
  charDelim[0] = delimiterIn;
  charDelim[1] = '\0';
  const char *delimiters = delimitersIn;
  if (delimiters == NULL) {
    delimiters = charDelim;
  }

  // remove leading delimiters
  size_t delim_count = 0;
  // skip leading delimiters  (prior to V2.0.2 leading delimiters not skipped)
  delim_count = strspn(buffer, delimiters); // count char ONLY in delimiters
  if ((returnEmptyFields) && (delim_count > 1)) {
    // only remove one delimiter
    delim_count = 1;
  }
  remove(0, delim_count); // remove leading delimiters
  if (len == 0) {
    // nothing left after last delimiter
    return (returnEmptyFields && returnLastNonDelimitedToken);
  }
  // check for token
  // find first char not in delimiters
  size_t token_count = 0;
  token_count = strcspn(buffer, delimiters);
  if ((token_count) == len) {
    // no trailing delimiter
    if (!returnLastNonDelimitedToken) {
      return false; // delimited token not found
    }
  } // return last one
  if (token_count > token._capacity) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("nextToken"));
      debugPtr->print(F(" token SafeString ")); token.outputName(); debugPtr->print(F(" needs capacity of ")); debugPtr->print(token_count);
      debugPtr->print(F(" for token '")); debugPtr->write((uint8_t*)(buffer), token_count); debugPtr->print('\'');
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    remove(0, token_count); // remove token but not following delimiters
    return true; // but token is empty => error // prior to V2.0.3 returned false
  }

  substring(token, 0, token_count); // do not return trailing delimiters.
  remove(0, token_count); // remove token but not following delimiters
  return true;
}
/** end of nextToken methods *******************/
/**** end of   Tokenizing methods,  stoken(), nextToken()  ****************/


/****************************************************************/
/**  ReadFrom from SafeString, writeTo SafeString               */
/****************************************************************/
/*
   reads from the SafeString argument, starting at startIdx, into this SafeString.
   
   The read stops when the end of the SafeString argument is reached or the calling SafeString is full
   Note: if the SafeString is already full, then nothing will be read and startIdx will be returned<br>
   <br>
   Note: to limit the number of chars read in from sfInput (starting at 0), use<br>
   <code>sfStr.readFrom(sfInput.c_str(),maxCharsToRead);</code>
   
   @param  sfInput - the SafeString to read from
   @param  startIdx - where to start reading from, defaults to 0,
              if startIdx >= sfInput.length(), nothing read and sfInput.length() returned
   @return - the new startIdx
**/
unsigned int SafeString::readFrom(SafeString & input, unsigned int startIdx) {
  input.cleanUp();
  cleanUp();
  if (startIdx > input.len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readFrom"));
      debugPtr->print(F(" startIdx:"));  debugPtr->print(startIdx);
      debugPtr->print(F(" > input.length():"));  debugPtr->print(input.len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Input arg was '")); debugPtr->print(input); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif
    return input.len; // nothing to read
  }
  if (len == _capacity) {
    // not room to read
    return startIdx; // no change
  }
  size_t readLen = input.len - startIdx;
  if (readLen > (_capacity - len)) {
    readLen = (_capacity - len); // limit to space available
  }
  memmove(buffer + len, input.buffer + startIdx, readLen);
  len += readLen;
  buffer[len] = '\0';
  size_t newStartIdx = startIdx + readLen;
  if (newStartIdx >= input.len) {
    return input.len;
  } else {
    return newStartIdx;
  }
}

/*
     reads from the const char* argument, starting at 0 and read up to maxCharToRead, into this SafeString.
     
     This lets you read from a char* into a SafeString without errors if the strlen(char*) or maxCharsToRead are larger than the SafeString capacity
     Use sfResult.clear(); to empty the SafeString first and then sfResult.readFrom(strPtr); to read a much as you can
     The read stops at first '\0' or the calling SafeString is full or when maxCharsToRead have been read.
     Note: if the SafeString is already full, then nothing will be read
     
     @param  strPtr - pointer char array to read from
     @param  maxCharsToRead -- the maximum chars to read into the SafeString, defaults to ((unsigned int)-1) i.e. max unsigned int.
	 
     @return - the number of chars read
*/
unsigned int SafeString::readFrom(const char* strPtr, unsigned int maxCharsToRead) {
	  cleanUp();
	  if ((!strPtr) || (!(*strPtr))) {
	  	len = 0;
        buffer[len] = '\0'; // terminate
        return 0;
	  }	  
	  // check reading from ourselves
	  if (strPtr == buffer) {
	  	return 0; // nothing new read
	  }
    // else
    unsigned int i = 0;
    while((len < _capacity) && (*strPtr) && (i < maxCharsToRead)){
    	// have space and not end of input
    	buffer[len++] = *strPtr++;
    	i++;
    }
    buffer[len] = '\0'; // terminate 
    return i;
}	

/*
   writeTo(SafeString & output, unsigned int startIdx = 0)  writes from SafeString, starting at startIdx to output
   params
     output - the SafeString to write to
     startIdx - where to start writing from calling SafeString, defaults to 0,
                if startIdx >= length(), nothing written and length() returned

   returns new startIdx for next write
   write stops when the end if the calling SafeString is reached or the output is full
   Note: if the sfOutput is already full, then nothing will be written and startIdx will be returned    
**/
unsigned int SafeString::writeTo(SafeString & output, unsigned int startIdx) {
  output.cleanUp();
  cleanUp();
  if (startIdx > len) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("writeTo"));
      debugPtr->print(F(" startIdx:"));  debugPtr->print(startIdx);
      debugPtr->print(F(" > length():"));  debugPtr->print(len);
      if (fullDebug) {
        debugPtr->println(); debugPtr->print(F("       "));
        debugPtr->print(F(" Output arg was '")); debugPtr->print(output); debugPtr->print('\'');
      }
      debugInternalMsg(fullDebug);
    }
#endif
    return len; // nothing written
  }
  if (output.len == output._capacity) {
    // not room to write
    return startIdx; // no change
  }
  size_t writeLen = len - startIdx;
  if (writeLen > (output._capacity - output.len)) {
    writeLen = (output._capacity - output.len); // limit to space available
  }
  memmove(output.buffer + output.len, buffer + startIdx, writeLen);
  output.len += writeLen;
  output.buffer[output.len] = '\0';
  size_t newStartIdx = startIdx + writeLen;
  if (newStartIdx >= len) {
    return len;
  } else {
    return newStartIdx;
  }
}

/** end of  ReadFrom from SafeString, writeTo SafeString ************/

/****************************************************************/
/**  NON-Blocking reads from Stream,  read() and readUntil()    */
/****************************************************************/

/* read(Stream& input) --  reads from the Stream (if chars available) into the SafeString
      The is NON-BLOCKING and returns immediately if nothing available to be read
      returns true if something added to string else false
      Note: if the SafeString is already full, then nothing will be read and false will be returned
*/
unsigned char SafeString::read(Stream& input) {
  cleanUp();
  bool rtn = false;
  noCharsRead = 0;
  while (input.available() && (len < _capacity)) {
    int c = input.read();
    noCharsRead++;
    if (c != '\0') { // skip any nulls read
      concat((char)c);
      rtn = true;
    } else {
      setError(); // found '\0' in input
      if (debugPtr) {
        debugPtr->println(); debugPtr->print(F("!! Error:")); outputName();
        debugPtr->println(F(" -- read '\\0' from Stream."));
      }
    }
  }
  return rtn; // true if something added to string
}

/*
  readUntil( ) ---  returns true if delimiter found or string filled, found else false
      NON-blocking readUntil of Stream, if chars are available
      returns true if delimiter found or string filled, found else false
      if a delimiter is found it is included in the return

      params
        input - the Stream object to read from
        delimiters - string of valid delimieters
      return true if SafeString is full or a delimiter is read, else false
      Any delimiter read is returned.  Only at most one delimiter is added per call
     Multiple sucessive delimiters require multiple calls to read them
**/
unsigned char SafeString::readUntil(Stream& input, const char delimiter) {
  if (!delimiter) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntil"));
      debugPtr->print(F(" was passed a '\\0' delimiter"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return len + 1;
  }
  return readUntilInternal(input, NULL, delimiter);
}

unsigned char SafeString::readUntil(Stream& input, SafeString &delimiters) {
  delimiters.cleanUp();
  return readUntil(input, delimiters.buffer); // calls cleanUp()
}

unsigned char SafeString::readUntil(Stream& input, const char* delimiters) {
  if (!delimiters) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntil"));
      debugPtr->print(F(" was passed a NULL pointer for delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // no match
  }
  if (*delimiters == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntil"));
      debugPtr->print(F(" was passed a empty list of delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // no match
  }
  return readUntilInternal(input, delimiters, '\0');
}


bool SafeString::readUntilInternal(Stream& input, const char* delimitersIn, const char delimiterIn) {
  cleanUp();
  char charDelim[2];
  charDelim[0] = delimiterIn;
  charDelim[1] = '\0';
  const char *delimiters = delimitersIn;
  if (delimiters == NULL) {
    delimiters = charDelim;
  }
  noCharsRead = 0;
  while (input.available() && (len < (capacity()))) {
    int c = input.read();
    noCharsRead++;
    if (c == '\0') {
      setError(); // found '\0' in input
#ifdef SSTRING_DEBUG
      if (debugPtr) {
        debugPtr->println(); debugPtr->print(F("!! Error:"));; outputName();
        debugPtr->println(F(" -- read '\\0' from Stream."));
      }
#endif // SSTRING_DEBUG
      continue; // skip nulls
    }
    concat((char)c); // add char may be delimiter
    if (strchr(delimiters, c) != NULL) {
      return true; // found delimiter return true
    }
  }
  if (isFull()) {
    return true;
  } // else
  return false;
}


/*
      NON-blocking readUntilToken
      returns true if a delimited token is found, else false
      ONLY delimited tokens of length less than this SafeString's capacity will return true with a non-empty token.
      Streams of chars that overflow this SafeString's capacity are ignored and return an empty token on the next delimiter or timeout
      That is this SafeString's capacity should be at least 1 more then the largest expected token.
      If this SafeString OR the SafeString & token return argument is too small to hold the result, the token is returned empty and an error message output if debugging is enabled.
      The delimiter is NOT included in the SafeString & token return.  It will the first char of the this SafeString when readUntilToken returns true
      It is recommended that the capacity of the SafeString & token argument be >= this SafeString's capacity
      Each call to this method removes any leading delimiters so if you need to check the delimiter do it BEFORE the next call to readUntilToken()
      if token does not have the capacity to hold the substring, hasError() is set on both this SafeString and the token SafeString
      If this SafeString is empty and received just a delimiter, then return an empty token and leave delimiter in the SafeString
        On the next call may get just another delimiter in that case will skip over both, return empty token and add back last delimiter received
        Result is that multiple consecutive delimiters will return multiple empty tokens.
      Reading of the input stream stops at the first delimiter read, so any following data is still in the stream's RX buffer

      params
        input - the Stream object to read from
        token - the SafeString to return the token found if any, always cleared at the start of this method
        delimiters - string of valid delimieters
        skipToDelimiter - a bool variable to hold the skipToDelimiter state between calls
        echoInput - defaults to true to echo the chars read
        timeout_ms - defaults to never timeout, pass a non-zero ms to autoterminate the last token if no new chars received for that time.

      returns true if a delimited series of chars found that fit in this SafeString else false
      If this SafeString OR the SafeString & token argument is too small to hold the result, the returned token is returned empty
      The delimiter is NOT included in the SafeString & token return. It will the first char of the this SafeString when readUntilToken returns true
 **/

unsigned char SafeString::readUntilToken(Stream & input, SafeString& token, const char delimiter, bool & skipToDelimiter, uint8_t echoInput, unsigned long timeout_ms) {
  if (!delimiter) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntilToken"));
      debugPtr->print(F(" was passed a '\\0' delimiter"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return len + 1;
  }
  return readUntilTokenInternal(input, token, NULL, delimiter, skipToDelimiter, echoInput, timeout_ms);
}

unsigned char SafeString::readUntilToken(Stream & input, SafeString& token, SafeString& delimiters, bool & skipToDelimiter, uint8_t echoInput, unsigned long timeout_ms) {
  delimiters.cleanUp();
  return readUntilToken(input, token, delimiters.buffer, skipToDelimiter, echoInput, timeout_ms); // calls cleanUp()
}

unsigned char SafeString::readUntilToken(Stream & input, SafeString& token, const char* delimiters, bool & skipToDelimiter, uint8_t echoInput, unsigned long timeout_ms) {
  if (!delimiters) {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntilToken"));
      debugPtr->print(F(" was passed a NULL pointer for delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // no match
  }
  if (*delimiters == '\0') {
    setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntilToken"));
      debugPtr->print(F(" was passed a empty list of delimiters"));
      debugInternalMsg(fullDebug);
    }
#endif // SSTRING_DEBUG
    return false; // no match
  }
  return readUntilTokenInternal(input, token, delimiters, '\0', skipToDelimiter, echoInput, timeout_ms);
}

bool SafeString::readUntilTokenInternal(Stream & input, SafeString& token, const char* delimitersIn, const char delimiterIn, bool & skipToDelimiter, uint8_t echoInput, unsigned long timeout_ms) {
  token.clear(); // always
  if ((echoInput != 0) && (echoInput != 1) && (timeout_ms == 0)) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntilToken"));
      debugPtr->println(F(" was passed timeout for the echo setting, method format is:-"));
      debugPtr->println(F("   readTokenUntil(stream,token,delimiters,skipToDelimiter,echoOn,timeout_ms)"));
    }
#endif // SSTRING_DEBUG
    timeout_ms = echoInput; // swap to timeout
    echoInput = false; // use default
  }

  if (capacity() < 2) {
    setError();
    token.setError();
#ifdef SSTRING_DEBUG
    if (debugPtr) {
      errorMethod(F("readUntilToken"));
      debugPtr->println(F(" SafeString needs capacity of at least 2, one char + one delimiter"));
    }
#endif // SSTRING_DEBUG
    return false;
  }

  cleanUp();
  char charDelim[2];
  charDelim[0] = delimiterIn;
  charDelim[1] = '\0';
  const char *delimiters = delimitersIn;
  if (delimiters == NULL) {
    delimiters = charDelim;
  }
  // remove leading delimiters
  size_t delim_count = 0;
  // skip leading delimiters  (prior to V2.0.2 leading delimiters not skipped)
  delim_count = strspn(buffer, delimiters); // count char ONLY in delimiters
  remove(0, delim_count); // remove leading delimiters

  // NOTE: this method's contract says you can set skipToDelimiter true at any time
  // so here stop reading on first delimiter and process results
  // loop here until either isFill() OR no more chars avail OR found delimiter
  // read at most capacity() each time if skipping
  // this prevent infinite loop if using SafeStringStream with echoOn and rx buffer overflow has dropped all the delimiters
  //size_t charRead = 0;
  noCharsRead = 0;
  if ((timeout_ms > 0) && (!timeoutRunning) && skipToDelimiter) {
    timeoutRunning = true;
    timeoutStart_ms = millis(); // start timer
  }
  char haveReadDelimiter = '\0';
  while (input.available() && (len < capacity()) && (noCharsRead < capacity()) ) {
    int c = input.read();
    noCharsRead++;
    //    if (debugPtr) {
    //      debugPtr->print(F("read:")); debugPtr->println(c));
    //    }
    if (c == '\0') {
      setError(); // found '\0' in input
      token.setError();
      if (debugPtr) {
        debugPtr->println(); debugPtr->print(F("!! Error:")); outputName();
        debugPtr->println(F(" -- read '\\0' from Stream."));
      }
      continue; // skip nulls  // don't update timer on null chars
    }
    if (echoInput) {
      input.print((char) c);
    }
    if (timeout_ms > 0) {
      // got new char reset timeout
      timeoutRunning = true;
      timeoutStart_ms = millis(); // start timer
    }
    if (!skipToDelimiter) {
      concat((char)c); // add char may be delimiter
    }
    if (strchr(delimiters, c) != NULL) {
      if (skipToDelimiter) {
        // if skipToDelimiter then started with empty SafeString
        skipToDelimiter = false; // found next delimiter not added above because skipToDelimiter
        clear();
        concat((char)c); // is a delimiter
        return false; // empty token
      } else {
        // added c above
        // if (!haveReadDelimiter) {
        haveReadDelimiter = c;
        //}
        break; // process this token
      }
    }
  }
  // here either isFill() OR no more chars avail OR found delimiter
  // skipToDelimiter may still be true here if no delimiter found above
  // skip multiple delimiters and do not return last non-delimited token
  if (nextToken(token, delimiters, false, false)) { // removes leading delimiters if any
    // returns true only if have found delimited token, returns false if full and no delimiter
    // IF found delimited token, delimiter was add just now and so timer was reset
    // skipToDelimiter is false here since found delimiter
    return true; // not full and not timeout and have new token
  } else if (haveReadDelimiter) {
    //  this SafeString was empty and just got only a delimiter, so nextToken removed it and returns false
    // so add back the single delimiter just received and return an empty token
    concat(haveReadDelimiter); // is a delimiter
    return true; // empty token,
  }


  // else
  if (isFull()) { // note if full here then > max token as capacity needs to allow for one delimiter
    // SafeString is full of chars but no delimiter
    // discard the chars and skip input until get next delimiter
    setError();
    token.setError();
    if (debugPtr) {
      debugPtr->println(); debugPtr->print(F("!! Error:")); outputName();
      debugPtr->print(F(" -- input length exceeds capacity "));
      debugInternalMsg(fullDebug);
    }
    clear();  // not full now
    skipToDelimiter = true;
    return false; // will do timeout check next call.  No token found return false
  }

  // else no token found AND not full AND last char NOT a delimiter
  if (timeoutRunning) { // here not full because called nextToken OR checked for full
    if ((millis() - timeoutStart_ms) > timeout_ms) {
      // no new chars for timeout add terminator
      timeoutRunning = false;
      // put in delimiter
      if ((len != 0) || skipToDelimiter) { // have something to delimit or had somthing, else just stop timer
        //          if (debugPtr) { // input timeout is normal if timeout set
        //            debugPtr->println(); debugPtr->print("!! "); outputName();
        //            debugPtr->println(F(" -- Input timed out."));
        //          }
        if (skipToDelimiter) {
          clear();  // not full now
          skipToDelimiter = false;
          return false;
        } // else pick up token
        // len > 0 so token only empty if too small
        //concat(delimiters[0]);   // certainly NOT full from above
        // skip multiple delimiters, and return last one (default
        nextToken(token, delimiters, false, true); // collect this token just delimited, this will clear input
        // remove delimiter just added
        //remove(0, 1);
        return true;
      }
    }
  }
  return false; // no token
}

size_t SafeString::getLastReadCount() {
  return noCharsRead;
}

/** end of NON-Blocking reads from Stream,  read() and readUntil() *******************/

/*******************************************************/
/** Private methods for Debug and Error support           */
/*******************************************************/
void SafeString::debugInternal(bool verbose) const {
  if (debugPtr) {
    if (name) {
      debugPtr->print(' ');
      debugPtr->print(name);
    } // else no name set
    debugPtr->print(F(" cap:")); debugPtr->print(_capacity);
    debugPtr->print(F(" len:")); debugPtr->print(len);
    if (verbose) { // print SafeString current contents
      debugPtr->print(F(" '")); debugPtr->print(buffer); debugPtr->print('\'');
    }
    debugPtr->println();
  }
}

// this internal msg debug does not add line indent if not fullDebug
// always need to add debugPtr->println() at end of debug output
void SafeString::debugInternalMsg(bool verbose) const {
  (void)(verbose);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    if (verbose) {
      debugPtr->println(); // terminate first line
      debugPtr->print(F("       "));
    } else {
      debugPtr->print(F(" --- ")); // not full debug
    }
    if (name) {
      debugPtr->print(' ');
      debugPtr->print(name);
    } // else no name set
    debugPtr->print(F(" cap:")); debugPtr->print(_capacity);
    debugPtr->print(F(" len:")); debugPtr->print(len);
    if (verbose) { // print SafeString current contents
      debugPtr->print(F(" '")); debugPtr->print(buffer); debugPtr->print('\'');
    }
    debugPtr->println();
  }
#endif // SSTRING_DEBUG
}

void SafeString::debugInternalResultMsg(bool verbose) const {
  (void)(verbose);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    if (verbose) {
      debugPtr->println(); // terminate first line
      debugPtr->print(F("       "));
    } else {
      debugPtr->print(F(" --- ")); // not full debug
    }
    if (name) {
      debugPtr->print(' ');
      debugPtr->print(name);
    } // else no name set
    debugPtr->print(F(" cap:")); debugPtr->print(_capacity);
    debugPtr->print(F(" len:")); debugPtr->print(len);
    if (verbose) { // print SafeString current contents
      debugPtr->print(F(" '")); debugPtr->print(buffer); debugPtr->print('\'');
    }
    debugPtr->println();
  }
#endif // SSTRING_DEBUG
}

void SafeString::outputName() const {
  if (debugPtr) {
    debugPtr->print(' ');
    if (name) {
      debugPtr->print(name);
    } else {
      debugPtr->print(F("SafeString"));
    }
  }
}


void SafeString::capError(const __FlashStringHelper * methodName, size_t neededCap, const char* cstr, const __FlashStringHelper * pstr, char c, size_t length) const {
  (void)(methodName); (void)(neededCap);   (void)(cstr);   (void)(pstr);   (void)(c);   (void)(length);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    errorMethod(methodName); // checks for NULL
    debugPtr->print(F(" needs capacity of "));
    debugPtr->print(neededCap);
    if (fromBuffer) {
      debugPtr->print(F("(i.e. char[")); debugPtr->print(neededCap + 1);  debugPtr->print(F("])"));
    }
    if (length != 0) {
      debugPtr->print(F(" for the first ")); debugPtr->print(length); debugPtr->print(F(" chars of the input."));
    }
    if (!fullDebug) {
      debugInternalMsg(fullDebug);
    } else {
      debugPtr->println();
      debugPtr->print(F("       ")); // indent next line
      if (cstr || pstr || (c != '\0')) {
        debugPtr->print(F(" Input arg was "));
        if (cstr) {
          debugPtr->print('\'');
          debugPtr->print(cstr);
          debugPtr->print('\'');
        } else if (pstr) {
          debugPtr->print(F("F(\""));
          debugPtr->print(pstr);
          debugPtr->print("\")");
        } else if (c != '\0') {
          debugPtr->print('\'');
          debugPtr->print(c);
          debugPtr->print('\'');
        }
      }
      debugInternalMsg(fullDebug);
    }
  }
#endif
}

void SafeString::assignError(size_t neededCap, const char* cstr, const __FlashStringHelper * pstr, char c, bool numberFlag) const {
  (void)(neededCap);   (void)(cstr);   (void)(pstr);   (void)(c);   (void)(numberFlag);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    debugPtr->print(F("Error:"));
    outputName();
    debugPtr->print(F(" = "));
    if (cstr || pstr || (c != '\0')) {
      if (cstr) {
        if (!numberFlag) {
          debugPtr->print('\"');
        }
        debugPtr->print(cstr);
        if (!numberFlag) {
          debugPtr->print('\"');
        }
      } else if (pstr) {
        debugPtr->print(F("F(\""));
        debugPtr->print(pstr);
        debugPtr->print("\")");
      } else if (c != '\0') {
        debugPtr->print('\'');
        debugPtr->print(c);
        debugPtr->print('\'');
      }
    }
    debugPtr->println();
    debugPtr->print(F(" needs capacity of "));
    debugPtr->print(neededCap);
    debugInternalMsg(fullDebug);
  }
#endif
}

void SafeString::errorMethod(const __FlashStringHelper * methodName) const {
  (void)(methodName);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    debugPtr->print(F("Error:"));
    outputName();
    debugPtr->print('.');
    if (methodName) {
      debugPtr->print(methodName);
    }
    debugPtr->print(F("()"));
  }
#endif
}

void SafeString::warningMethod(const __FlashStringHelper * methodName) const {
  (void)(methodName);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    debugPtr->print(F("Warning:"));
    outputName();
    debugPtr->print('.');
    if (methodName) {
      debugPtr->print(methodName);
    }
    debugPtr->print(F("()"));
  }
#endif
}

void SafeString::outputFromIndexIfFullDebug(unsigned int fromIndex) const {
  (void)(fromIndex);
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    if (fullDebug) {
      debugPtr->println(); debugPtr->print(F("       "));
      debugPtr->print(F(" fromIndex is ")); debugPtr->print(fromIndex);
    }
  }
#endif
}

void SafeString::concatErr() const {
#ifdef SSTRING_DEBUG
  errorMethod(F("concat"));
#endif
}

void SafeString::assignErrorMethod() const {
#ifdef SSTRING_DEBUG
  if (debugPtr) {
    debugPtr->print(F("Error:"));
    outputName();
    debugPtr->print(F(" = "));
  }
#endif
}

void SafeString::concatAssignError() const {
#ifdef SSTRING_DEBUG
  assignErrorMethod();
#endif
}

void SafeString::printlnErr() const {
#ifdef SSTRING_DEBUG
  errorMethod(F("println"));
#endif
}

void SafeString::prefixErr() const {
#ifdef SSTRING_DEBUG
  errorMethod(F("prefix"));
#endif
}
/*****************  end of private internal debug support methods *************************/

/**
// dtostrf for those boards that don't have it
static char *dtostrf (double val, signed char width, unsigned char prec, char *sout) {
  asm(".global _printf_float");

  char fmt[20];
  sprintf(fmt, "%%%d.%df", width, prec);
  sprintf(sout, fmt, val);
  return sout;
}
**/