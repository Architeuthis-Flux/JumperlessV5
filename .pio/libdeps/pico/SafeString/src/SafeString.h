// !!!!!!!!! WARNING in V2 substring endIdx is EXCLUSIVE !!!!!!!!!! change from V1 inclusive
/*
   The SafeString class V4.1.29


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

  createSafeStringFromCharPtrWithSize(name, char*, unsigned int);  or cSFPS(name, char*, unsigned int);
   wraps an existing char[] pointed to by char* in a SafeString of the given name and sets the capacity to the given size
  e.g.
  char charBuffer[15]; // can hold 14 char + terminating '\0'
  char *bufPtr = charBuffer;
  createSafeStringFromCharPtrWithSize(str,bufPtr, 14); or cSFPS(str,bufPtr, 14);
  expands in the pre-processor to
   SafeString str(14+1,charBuffer, charBuffer, "str", true);
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


   If str is a SafeString then
   str = .. works for signed/unsigned ints, char*, char, F(".."), SafeString float, double etc
   str.concat(..) and string.prefix(..) also works for those
   str.stoken(..) can be used to split a string in to tokens

   SafeStrings created via createSafeString(  ) are never invalid, even if called with invalid arguments.
   SafeStrings created via createSafeStringFromBuffer(  ) are valid as long at the buffer is valid.
     Usually the only way the buffer can become invalid is if it exists in a struct that is allocated (via calloc/malloc)
     and then freed while the SafeString wrapping it is still in use.
*********************************/

/*
  SafeString.h static memory SafeString library modified by
  Matthew Ford
  Mods Copyright(c)2020 Forward Computing and Control Pty. Ltd.
  All rights reservered subject to the License below

  modified from
  WString.h - String library for Wiring & Arduino
  ...mostly rewritten by Paul Stoffregen...
  Copyright (c) 2009-10 Hernando Barragan.  All right reserved.
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
// bool versus unsigned char
// on UNO, ESP8266 and ESP32  sizeof(bool) == 1  i.e. same size as unsigned char
// but bool is safer as  bool + 1 does not compile
// however Arduino uses unsigned char as return value so...
#ifndef SafeString_class_h
#define SafeString_class_h


#ifdef __cplusplus

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_ESP8266)
#include <pgmspace.h>
#elif defined(ARDUINO_ARDUINO_NANO33BLE) || defined(ARDUINO_ARCH_MBED_RP2040)|| defined(ARDUINO_ARCH_RP2040)|| defined(ARDUINO_ARCH_MBED)
#include <api/deprecated-avr-comp/avr/pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

#include <stdint.h>
#include <Print.h>
#include <Printable.h>

// This include handles the rename of Stream for MBED compiles
#if defined(ARDUINO_ARDUINO_NANO33BLE) || defined(ARDUINO_ARCH_SAMD) || defined(ARDUINO_ARCH_MBED_RP2040) || defined(ARDUINO_ARCH_RP2040) || defined(ARDUINO_ARCH_MBED)
#include <Stream.h>
#elif defined( __MBED__ ) || defined( MBED_H )
#include <WStream.h>
#define Stream WStream
#else
#include <Stream.h>
#endif

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

// removed V4.1.29 -- Add these lines back in if your board does not define the F() macro and the class __FlashStringHelper;
//class __FlashStringHelper;
//#define F(string_literal) (reinterpret_cast<const __FlashStringHelper *>(PSTR(string_literal)))

// to remove all the error messages, comment out
#define SSTRING_DEBUG
// this saves program bytes and the ram used by the SafeString object names
//
// Usually just leave as is and use SafeString::setOutput(..) to control the error messages and debug output
// there will be no error messages or debug output if SafeString::setOutput(..) has not been called from your sketch
//
// SafeString.debug() is always available regardless of the SSTRING_DEBUG define setting
//   but SafeString::setOutput() still needs to be called to set where the output should go.

/* -----------------  creating SafeStrings ---------------------------------
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

  createSafeStringFromCharPtr(name, char*);  or cSFP(name, char*);
   wraps an existing char[] pointed to by char* in a SafeString of the given name
  e.g.
  char charBuffer[15];
  char *bufPtr = charBuffer;
  createSafeStringFromCharPtr(str,bufPtr); or cSFP(str,bufPtr);
  expands in the pre-processor to
   SafeString str((unsigned int)-1,charBuffer, charBuffer, "str", true);
  and the capacity of the SafeString is set to strlen(charBuffer) and cannot be increased.

  createSafeStringFromCharPtrWithSize(name, char*, unsigned int);  or cSFPS(name, char*, unsigned int);
   wraps an existing char[] pointed to by char* in a SafeString of the given name and sets the capacity to the given size -1
  e.g.
  char charBuffer[15];
  char *bufPtr = charBuffer;
  createSafeStringFromCharPtrWithSize(str,bufPtr, 15); or cSFPS(str,bufPtr, 15);
  expands in the pre-processor to
   SafeString str(15,charBuffer, charBuffer, "str", true);
  The capacity of the SafeString is set to 14.

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


#ifdef SSTRING_DEBUG
#define createSafeString(name, size,...) char name ## _SAFEBUFFER[(size)+1]; SafeString name(sizeof(name ## _SAFEBUFFER),name ## _SAFEBUFFER,  ""  __VA_ARGS__ , #name);
#define createSafeStringFromCharArray(name, charArray)  SafeString name(sizeof(charArray),charArray, charArray, #name, true, false);
#define createSafeStringFromCharPtr(name, charPtr) SafeString name((unsigned int)-1,charPtr, charPtr, #name, true);
#define createSafeStringFromCharPtrWithSize(name, charPtr, arraySize) SafeString name((arraySize),charPtr, charPtr, #name, true);
#else
#define createSafeString(name, size,...) char name ## _SAFEBUFFER[(size)+1]; SafeString name(sizeof(name ## _SAFEBUFFER),name ## _SAFEBUFFER, ""  __VA_ARGS__);
#define createSafeStringFromCharArray(name,charArray)  SafeString name(sizeof(charArray),charArray, charArray, NULL, true, false);
#define createSafeStringFromCharPtr(name, charPtr) SafeString name((unsigned int)-1,charPtr, charPtr, NULL, true);
#define createSafeStringFromCharPtrWithSize(name, charPtr, arraySize) SafeString name((arraySize),charPtr, charPtr, NULL, true);
#endif

// define typing shortcuts
#define cSF createSafeString
#define cSFA createSafeStringFromCharArray
#define cSFP createSafeStringFromCharPtr
#define cSFPS createSafeStringFromCharPtrWithSize

/**************
  To create SafeStrings use one of the four (4) macros **createSafeString** or **cSF**, **createSafeStringFromCharArray** or **cSFA**, **createSafeStringFromCharPtr** or **cSFP**, **createSafeStringFromCharPtrWithSize** or **cSFPS** see the detailed description. 
  
  There are four (4) macros used to create SafeStrings.<br> 
  createSafeString(name, size); creates a char[size+1] for you and wraps it in a SafeString called name.<br> 
  createSafeStringFromCharArray(name, char[]); wraps an existing char[] in a SafeString called name.<br> 
  createSafeStringFromCharPtr(name, char*);  wraps an existing char[], pointed to by char*, in a SafeString called name.  The capacity of the SafeString is limited to the initial strlen(char*) and cannot be increased<br> 
  createSafeStringFromCharPtrWithSize(name, char*, size);  wraps an existing char[size], pointed to by char*, in a SafeString called name.<br> 
  
  Each of thise macros has a short hand<br> 
  cSF(name,size) for createSafeString(name, size);<br> 
  cSFA(name,char[]) for createSafeStringFromCharArray(name, char[]);<br> 
  cSFP(name,char*) for createSafeStringFromCharPtr(name, char*);<br> 
  cSFPS(name,char*,size) for createSafeStringFromCharPtrWithSize(name, char*, size);
  
<H1>createSafeString and cSF</H1>  
 createSafeString(name, size) or cSF(name,size)<br>
 and<br>
 <code>createSafeString(name, size, "initialText")</code> or <code>cSF(name,size,"initialText")</code><br>
are utility macros to create a SafeString of a given name and size and optionally, an initial value
  
  createSafeString(str, 40);  or  cSF(str, 40);
expands in the pre-processor to<br>
<code>char str_SAFEBUFFER[40+1];</code><br> 
<code>SafeString str(sizeof(str_SAFEBUFFER),str_SAFEBUFFER,"","str");</code>

  createSafeString(str, 40, "test");  or cSF(str, 40, "test"); 
expands in the pre-processor to<br>
<code>char str_SAFEBUFFER[40+1];</code><br>
<code>SafeString str(sizeof(str_SAFEBUFFER),str_SAFEBUFFER,"test","str");</code>


<H1>createSafeStringFromCharArray and cSFA</H1>  
createSafeStringFromCharArray(name, char[]);  or cSFA(name, char[]);
wraps an existing char[] in a SafeString of the given name. e.g.<br>
<code>char charBuffer[15]; </code><br>
<code>createSafeStringFromCharArray(str,charBuffer); or cSFA(str,charBuffer);</code><br> 
expands in the pre-processor to<br>
<code>SafeString str(sizeof(charBuffer),charBuffer, charBuffer, "str", true);</code><br>
  The capacity of the SafeString is set to 14, from the **sizeof(charBuffer)-1** to allow for the terminating '\0'.


<H1>createSafeStringFromCharPtr and cSFP</H1>  
  createSafeStringFromCharPtr(name, char*);  or cSFP(name, char*);
   wraps an existing char[] pointed to by char* in a SafeString of the given name<br>
  The capacity of the SafeString is set to <code>strlen(charBuffer)</code> and cannot be increased.
   e.g.<br>
<code>char charBuffer[15] = "1234567890";</code><br>
<code>char *bufPtr = charBuffer;</code><br>
<code>createSafeStringFromCharPtr(str,bufPtr); or cSFP(str,bufPtr);</code><br>
  expands in the pre-processor to<br>
<code>SafeString str((unsigned int)-1,charBuffer, charBuffer, "str", true);</code><br>
  The capacity of the SafeString is set to 10, from the **strlen(bufPtr)**

<H1>createSafeStringFromCharPtrWithSize and cSFPS</H1>  
  createSafeStringFromCharPtrWithSize(name, char*, unsigned int size);  or cSFPS(name, char*, unsigned int size);
   wraps an existing char[], pointed to by char*, in a SafeString of the given name<br>
   and sets the capacity to the given unsigned int size e.g.<br>
<code>char charBuffer[15]="1234567890";</code><br>
<code>char *bufPtr = charBuffer;</code><br>
<code>createSafeStringFromCharPtrWithSize(str,bufPtr, 15); or cSFPS(str,bufPtr, 15);</code><br>
  expands in the pre-processor to<br>
<code>SafeString str(15,charBuffer, charBuffer, "str", true);</code><br>
  The capacity of the SafeString is set to 14 from **size-1** to allow for the terminating '\0'.

See [The SafeString alternative to Arduino Strings for Beginners](https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html) for a tutorial.<br>

Also see the example sketches included with the library<br>
SafeString_ConstructorAndDebugging.ino, SafeStringFromCharArray.ino, SafeStringFromCharPtr.ino and SafeStringFromCharPtrWithSize.ino

  
****************************************************************************************/
class SafeString : public Printable, public Print {

  public:
/*********************************************
  SafeString Constructor called from <a href="#details">the four (4) macros</a> **createSafeString** or **cSF**, **createSafeStringFromCharArray** or **cSFA**, **createSafeStringFromCharPtr** or **cSFP**, **createSafeStringFromCharPtrWithSize** or **cSFPS**.
  
  This constructor is not designed to called directly by your code. See the <a href="#details">detailed SafeString class description</a> above.
  @param maxLen - the number of chars that can stored not including the terminating '\0'
  @param buf - the fixed sized char buffer to hold the data
  @param cstr - the initial data to be loaded into this SafeString
  @param _name - the code variable name of this SafeString, default NULL
  @param _fromBuffer - true if this SafeString is wrapping an existing char[]
  @param _fromPtr - true if this SafeString's buf parameter was a char* rather then a char[], _fromPtr is not checked unless _fromBuffer is true
 ********************************************/
// In all cases when maxlen != -1, it is the actual size of the array
// if _fromBuffer false (i.e. cSF(sfStr,20); ) then maxLen is the capacity+1 and the macro allocates an char[20+1], (_fromPtr ignored)
// if _fromBuffer true and _fromPtr false (i.e. cSFA(sfStr, strArray); ) then maxLen is the sizeof the strArray and the capacity is maxLen-1, _fromPtr is false
// if _fromBuffer true and _fromPtr true, then from char*, (i.e. cSFP(sfStr,strPtr) or cSFPS(sfStr,strPtr, maxLen) and maxLen is either -1 cSFP( ) the size of the char Array pointed cSFPS 
//    if maxLen == -1 then capacity == strlen(char*)  i.e. cSFP( )
//    else capacity == maxLen-1;   i.e. cSFPS( )
    explicit SafeString(unsigned int maxLen, char *buf, const char* cstr, const char* _name = NULL, bool _fromBuffer = false, bool _fromPtr = true);
    // _fromBuffer true does extra checking before each method execution for SafeStrings created from existing char[] buffers
    // _fromPtr is not checked unless _fromBuffer is true
    // _fromPtr true allows for any array size, if false prevents passing char* by checking sizeof(charArray) != sizeof(char*)

  private: // to force compile errors if function definition of the SafeString argument is not a refernce, i.e. not SafeString&
    SafeString(const SafeString& other ); // You must declare SafeStrings function arguments as a reference, SafeString&,  e.g. void test(SafeString& strIn)
    // NO other constructors, NO conversion constructors

  public:
    /*****************
     Turns on Error msgs and debug( ) output for all SafeStrings.
     
     This also sets output for the debug( ) method.
     @param debugOut - where to send the messages to, usually Serial, e.g. SafeString::setOutput(Serial);
     @param verbose - optional, if missing defaults to true, use false for compact error messages or call setVerbose(false)
    ***************/    
    static void setOutput(Print& debugOut, bool verbose = true);
    // static SafeString::DebugPrint Output;  // a Print object controlled by setOutput() / turnOutputOff() is defined at the bottom

    /*****************
     Turns off all debugging messages, both error messages AND debug() method output.
     
     Errors are still detected and flagged.
    ***************/    
    static void turnOutputOff(void);     // call this to turn all debugging OFF, both error messages AND debug( ) method output

    // use this to control error messages verbose output
    /*****************
     Controls size of error messages, setOutput sets verbose to true
     
     @param verbose - true for detailed error messages, else short error messages.
    ***************/    
    static void setVerbose(bool verbose); // turn verbose error msgs on/off.  setOutput( ) sets verbose to true

    // returns true if error detected, errors are detected even is setOutput has not been called
    // each call to hasError() clears the errorFlag
    /*****************
     Returns non-zero if any error detected for this SafeString, each call clears the internal flag
    ***************/    
    unsigned char hasError();

    // returns true if error detected in any SafeString object, errors are detected even is setOutput has not been called
    // each call to errorDetected() clears the classErrorFlag
    /*****************
     Returns non-zero if any SafeString has detected and error, each call clears the internal global static flag
    ***************/    
    static unsigned char errorDetected();

    // these methods print out info on this SafeString object, iff setOutput has been called
    // setVerbose( ) does NOT effect these methods which have their own verbose argument
    // Each of these debug( ) methods defaults to outputing the string contents.  Set the optional verbose argument to false to suppress outputing string contents
    // NOTE!! all these debug methods return a pointer to an empty string.
    // This is so that if you add .debug() to Serial.println(str);  i.e. Serial.println(str.debug()) will work as expected
    /*****************
     Output the details about the this SafeString to the output specified by setOutput().
     
     @param verbose - true for detailed output including current contents
    ***************/    
    const char* debug(bool verbose = true);
    
    /*****************
     Output the details about the this SafeString to the output specified by setOutput().
     
     @param title - the title to preceed the debug output, a space between this and the debug output
     @param verbose - true for detailed output including current contents
    ***************/    
    const char* debug(const char* title, bool verbose = true);

    /*****************
     Output the details about the this SafeString to the output specified by setOutput().
     
     @param title - the title to preceed the debug output, a space between this and the debug output
     @param verbose - true for detailed output including current contents
    ***************/    
    const char* debug(const __FlashStringHelper *title, bool verbose = true);

    /*****************
     Output the details about the this SafeString to the output specified by setOutput().
     
     @param title - the title to preceed the debug output, a space between this and the debug output
     @param verbose - true for detailed output including current contents
    ***************/    
    const char* debug(SafeString &stitle, bool verbose = true);

    /*****************
     Write (concatinate) a byte to this SafeString, from Print class.
     
     '\0' bytes are not allowed and will not be added to the SafeString and will raise an error.
     
     @param b - the byte to concatinate to this SafeString
    ***************/    
    virtual size_t write(uint8_t b);
    // writes at most length chars to this SafeString,
    // NOTE: write(cstr,length) will set hasError and optionally output errorMsg, if strlen(cstr) < length and nothing will be added to the SafeString
    
    /*****************
     Write (concatinate) bytes to this SafeString, from Print class.
     
     '\0' bytes are not allowed and will be skipped over and will raise an error.
     
     @param buffer - bytes to concatinate to this SafeString
     @param length - number of bytes to concatinate 
    ***************/    
    virtual size_t write(const uint8_t *buffer, size_t length);

    /*****************
     Implements the Printable interface
     
     @param p - where to print to
    ***************/    
    size_t printTo(Print& p) const;

    // reserve returns 0 if _capacity < size
    /*****************
     Checks there is enough free space in this SafeString for the current operation
     
     @param size - total number of chars that will be in the SafeString if the operation completes
    ***************/    
    unsigned char reserve(unsigned int size);

    /*****************
     Number of characters current in the SafeString, excluding the terminating '\0'
    ***************/    
    unsigned int length(void);
    
    /*****************
     The maximum number of characters this SafeString can hold, excluding the terminating '\0'
    ***************/    
    unsigned int capacity(void);
    
    /*****************
     Returns non-zero if the SafeString is full
    ***************/    
    unsigned char isFull(void);
    
    /*****************
     Returns non-zero if the SafeString is empty
    ***************/    
    unsigned char isEmpty(void);
    
    /*****************
     Returns the number chars that can be added to this SafeString before it is full
    ***************/    
    int availableForWrite(void);

    /*****************
     Empties this SafeString.
     
      returns the current SafeString so you can cascade calls e.g.<br>
      <code>sfStr.clear().concat("New output");</code><br>
    ***************/    
    SafeString & clear(void);

  public:
    // support for print
    size_t print(unsigned char, int = DEC);
    size_t print(int, int = DEC);
    size_t print(unsigned int, int = DEC);
    size_t print(long, int = DEC);
    size_t print(unsigned long, int = DEC);
    size_t print(double, int = 2);
    size_t print(const __FlashStringHelper *);
    size_t print(const char*);
    size_t print(char);
    size_t print(SafeString &str);

    size_t println(unsigned char, int = DEC);
    size_t println(int, int = DEC);
    size_t println(unsigned int, int = DEC);
    size_t println(long, int = DEC);
    size_t println(unsigned long, int = DEC);
    size_t println(double, int = 2);
    size_t println(const __FlashStringHelper *);
    size_t println(const char*);
    size_t println(char);
    size_t println(SafeString &str);
    size_t println(void);
    
    // ********** special prints padding and formatting doubles (longs) **************
    // print to SafeString a double (or long) with decs after the decimal point and padd to specified width
    //    width is a signed value, negative for left adjustment, +ve for right padding
    //    by default the + sign is not added, set forceSign argument to true to force the display of the + sign
    //    
    //    If the result exceeds abs(width), reduce the decs after the decmial point to fit into width
    //    If result with decs reduced to 0 is still > abs(width) raise an error and ,optionally, output an error msg
    //
    //    Note decs is quietly limited in this method to < 7 digit after the decimal point.
    
    /*************************************************************
    Prints a double (or long/int) to this SafeString padded with spaces (left or right) and limited to the specified width and adds a trailing CR NL
    
    This methods can also be used for ints and longs by passing 0 for the decs.
    
    @param d - the double to convert to text
    @param decs - the preferred number of decimial places to output (limited to <7).  This will be reduced automatically to fit the fixed width
    @param width - fixed width the output is to padded/limited to
    @param forceSign - optional, defaults to false, if true the + sign is added for +ve numbers
    ****************************************************************************/
    size_t println(double d, int decs, int width, bool forceSign = false);
    
    /*************************************************************
    Prints a double (or long/int) to this SafeString padded with spaces (left or right) and limited to the specified width.
    
    This methods can also be used for ints and longs by passing 0 for the decs.
    
    @param d - the double to convert to text
    @param decs - the preferred number of decimial places to output(limited to <7).  This will be reduced automatically to fit the fixed width
    @param width - fixed width the output is to padded/limited to
    @param forceSign - optional, defaults to false, if true the + sign is added for +ve numbers
    ****************************************************************************/
    size_t print(double d, int decs, int width, bool forceSign = false);



    // Assignment operators **********************************
    //  Set the SafeString to a char version of the assigned value.
    //  For = (const char *) the contents are copied to the SafeString buffer
    //  if the value is null or invalid,
    //  or too large to be fit in the string's internal buffer
    //  the string will be left empty
    
    /*************************************************************
    Clears this SafeString and concatinates a single char.
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param c - the char
    ****************************************************************************/
    SafeString & operator = (char c);
    
    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (unsigned char num);
    
    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (int num);

    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (unsigned int num);

    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (long num);

    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (unsigned long num);

    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (float num);

    /*************************************************************
    Clears this SafeString and concatinates the text version of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param num - the number to convert to text
    ****************************************************************************/
    SafeString & operator = (double num);

    /*************************************************************
    Clears this SafeString and copies and concatinates the contents of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param sfStr - the SafeString to assign
    ****************************************************************************/
    SafeString & operator = (SafeString &sfStr);

    /*************************************************************
    Clears this SafeString and copies and concatinates the contents of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param cstr - the '\0' terminated char string to assign
    ****************************************************************************/
    SafeString & operator = (const char *cstr);

    /*************************************************************
    Clears this SafeString and copies and concatinates the contents of the argument
    
    If the value is null or invalid, or too large to be fit in the string's internal buffer the resulting SafeString will be empty
    
    @param pstr - the '\0' terminated F("..") string to assign
    ****************************************************************************/
    SafeString & operator = (const __FlashStringHelper *pstr); // handle F(" .. ") values

    /********
      prefix methods add to the front of the current SafeString.
      
      returns the current SafeString so you can cascade calls, e.g.<br>
      <code>sfStr.prefix(2).prefix("first");</code><br>
      If there's not enough memory for the to add prefix, the SafeString will be left unchanged<br>
      If the argument is null or invalid, the prefix is considered unsucessful.
    **/
    SafeString & prefix(SafeString &s);
    SafeString & prefix(const char *cstr);
    SafeString & prefix(char c);
    SafeString & prefix(unsigned char c);
    SafeString & prefix(int num);
    SafeString & prefix(unsigned int num);
    SafeString & prefix(long num);
    SafeString & prefix(unsigned long num);
    SafeString & prefix(float num);
    SafeString & prefix(double num);
    SafeString & prefix(const __FlashStringHelper * str);
    SafeString & prefix(const char *cstr, size_t length);
    SafeString & prefix(const __FlashStringHelper * str, size_t length);

    /*******
      concat methods add to the end of the current SafeString.
      
      returns the current SafeString so you can cascade calls e.g.<br>
      <code>sfStr.concat(1).concat("second");</code><br>
      If there's not enough memory for the concatenated value, the SafeString will be left unchanged.<br>
      If the argument is null or invalid, the concatenation is considered unsucessful.
    **/
    SafeString & concat(SafeString &str);
    SafeString & concat(const char *cstr);
    SafeString & concat(char c);
    SafeString & concat(unsigned char c);
    SafeString & concat(int num);
    SafeString & concat(unsigned int num);
    SafeString & concat(long num);
    SafeString & concat(unsigned long num);
    SafeString & concat(float num);
    SafeString & concat(double num);
    SafeString & concat(const __FlashStringHelper * str);
    // ------------------------------------------------------
    // no corresponding methods these three (3) in  prefix, +=, -+
    
    SafeString & concat(const char *cstr, size_t length); // concat at most length chars from cstr
    // NOTE: concat(cstr,length) will set hasError and optionally output errorMsg, if strlen(cstr) < length and nothing will be concatinated.
    
    SafeString & concat(const __FlashStringHelper * str, size_t length); // concat at most length chars
    
    /******
      Adds \\r\\n to this SafeString.
      returns the current SafeString so you can cascade calls e.g.<br>
      <code>sfStr.newline().concat("next line");</code><br>
    *****/
      SafeString & newline(); // append newline \r\n same as concat("\r\n"); same a println()
    // e.g. sfStr.concat("test").newline();

    /* prefix() operator  -=  ******************
      Operator version of prefix( )
      prefix  -=
      To cascade operators use ( )
      e.g. (sfStr -= 'a') -= 5;
     **/
     /***
       -= operator prefixes the SafeString.
       e.g.<br>
       <code> sfStr -= "top";</code><br>
       gives the same SafeString as<br>
       <code> sfStr.prefix("top");<code><br>
     **/  
    SafeString & operator -= (SafeString &rhs) {
      return prefix(rhs);
    }
    SafeString & operator -= (const char *cstr) {
      return prefix(cstr);
    }
    SafeString & operator -= (char c) {
      return prefix(c);
    }
    SafeString & operator -= (unsigned char num) {
      return prefix(num);
    }
    SafeString & operator -= (int num) {
      return prefix(num);
    }
    SafeString & operator -= (unsigned int num) {
      return prefix(num);
    }
    SafeString & operator -= (long num)  {
      return prefix(num);
    }
    SafeString & operator -= (unsigned long num) {
      return prefix(num);
    }
    SafeString & operator -= (float num) {
      return prefix(num);
    }
    SafeString & operator -= (double num) {
      return prefix(num);
    }
    SafeString & operator -= (const __FlashStringHelper *str) {
      return prefix(str);
    }

    /* concat() operator  +=  ******************
      Operator versions of concat( )
      suffix/append +=
      To cascade operators use ( )
      e.g. (sfStr += 'a') += 5;
     **/
     /***
       += operator concatinate to the SafeString.
       e.g.<br>
       <code> sfStr += "end";</code><br>
       gives the same SafeString as<br>
       <code> sfStr.concat("end");<code><br>
     **/  
    SafeString & operator += (SafeString &rhs)  {
      return concat(rhs);
    }
    SafeString & operator += (const char *cstr) {
      return concat(cstr);
    }
    SafeString & operator += (char c) {
      return concat(c);
    }
    SafeString & operator += (unsigned char num) {
      return concat(num);
    }
    SafeString & operator += (int num) {
      return concat(num);
    }
    SafeString & operator += (unsigned int num) {
      return concat(num);
    }
    SafeString & operator += (long num) {
      return concat(num);
    }
    SafeString & operator += (unsigned long num) {
      return concat(num);
    }
    SafeString & operator += (float num) {
      return concat(num);
    }
    SafeString & operator += (double num)  {
      return concat(num);
    }
    SafeString & operator += (const __FlashStringHelper *str) {
      return concat(str);
    }

    /* Comparision methods and operators  ******************
      comparisons only work with SafeStrings and "strings"
      These methods used to be  ... const {
      but now with createSafeStringFromBuffer( ) the SafeString may be modified by cleanUp()
     **/
     /***
       returns -1 if this SafeString is < s, 0 if this SafeString == s and +1 if this SafeString > s
     **/
    int compareTo(SafeString &s) ;
     /***
       returns -1 if this SafeString is < cstr, 0 if this SafeString == cstr and +1 if this SafeString > cst
     **/
    int compareTo(const char *cstr) ;
    
    unsigned char equals(SafeString &s) ;
    unsigned char equals(const char *cstr) ;
    unsigned char equals(const char c) ;
    unsigned char operator == (SafeString &rhs) {
      return equals(rhs);
    }
    unsigned char operator == (const char *cstr) {
      return equals(cstr);
    }
    unsigned char operator == (const char c) {
      return equals(c);
    }
    unsigned char operator != (SafeString &rhs) {
      return !equals(rhs);
    }
    unsigned char operator != (const char *cstr) {
      return !equals(cstr);
    }
    unsigned char operator != (const char c) {
      return !equals(c);
    }
    unsigned char operator <  (SafeString &rhs) {
      return compareTo(rhs) < 0;
    }
    unsigned char operator >  (SafeString &rhs) {
      return compareTo(rhs) > 0;
    }
    unsigned char operator <= (SafeString &rhs) {
      return compareTo(rhs) <= 0;
    }
    unsigned char operator >= (SafeString &rhs) {
      return compareTo(rhs) >= 0;
    }
    unsigned char operator <  (const char* rhs) {
      return compareTo(rhs) < 0;
    }
    unsigned char operator >  (const char* rhs) {
      return compareTo(rhs) > 0;
    }
    unsigned char operator <= (const char* rhs) {
      return compareTo(rhs) <= 0;
    }
    unsigned char operator >= (const char* rhs) {
      return compareTo(rhs) >= 0;
    }
    unsigned char equalsIgnoreCase(SafeString &s) ;
    unsigned char equalsIgnoreCase(const char *str2) ;
    
    unsigned char equalsConstantTime(SafeString &s) ;

    /* startsWith methods  *******************
      The fromIndex is offset into this SafeString where check is to start
      0 to length() and (unsigned int)(-1) are valid for fromIndex, if fromIndex == length() or -1 false is returned
      if the argument is null or fromIndex > length(), an error is flagged and false returned
    **/
    /** 
      returns non-zero of this SafeString starts this argument looking from fromIndex onwards.
      
      @param c -- char to check for
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWith(const char c, unsigned int fromIndex = 0);
    /** 
      returns non-zero of this SafeString starts this argument looking from fromIndex onwards.
      
      @param str -- string to check for
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWith( const char *str2, unsigned int fromIndex = 0) ;
    /** 
      returns non-zero of this SafeString starts this argument looking from fromIndex onwards.
      
      @param s -- the SafeString to check for
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWith(SafeString &s2, unsigned int fromIndex = 0) ;
    
    /** 
      returns non-zero of this SafeString starts this argument, ignoring case, looking from fromIndex onwards.
      
      @param c -- char to check for, ignoring case
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWithIgnoreCase(const char c, unsigned int fromIndex = 0);
    /** 
      returns non-zero of this SafeString starts this argument, ignoring case, looking from fromIndex onwards.
      
      @param str -- string to check for, ignoring case
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWithIgnoreCase( const char *str2, unsigned int fromIndex = 0) ;
    /** 
      returns non-zero of this SafeString starts this argument, ignoring case, looking from fromIndex onwards.
      
      @param s -- SafeString to check for, ignoring case
      @param fromIndex -- where in the SafeString to start looking, default 0, that is start from beginning
    **/
    unsigned char startsWithIgnoreCase( SafeString &s2, unsigned int fromIndex = 0) ;
    
    /* endsWith methods  *******************/
    /** 
      returns non-zero of this SafeString ends with the argument
      
      @param c -- the char to check for
    **/
    unsigned char endsWith(const char c);
    /** 
      returns non-zero of this SafeString ends with the argument
      
      @param suffix -- SafeString to check for
    **/
    unsigned char endsWith(SafeString &suffix) ;
    /** 
      returns non-zero of this SafeString ends with the argument
      
      @param suffix -- string to check for
    **/
    unsigned char endsWith(const char *suffix) ;
    /** 
      returns non-zero of this SafeString ends any one of the chars in the argument
      
      @param suffix -- the SafeString containing the chars to check for
    **/
    unsigned char endsWithCharFrom(SafeString &suffix) ;
    /** 
      returns non-zero of this SafeString ends any one of the chars in the argument
      
      @param suffix -- the string containing the chars to check for
    **/
    unsigned char endsWithCharFrom(const char *suffix) ;

    /* character acccess methods  *******************
      NOTE: There is no access to modify the underlying char buffer directly
      For these methods 0 to length()-1 is valid for index
      index greater than length() -1 will return 0 and set the error flag and will print errors if debug enabled
    **/
    /**
      returns the char at that location in this SafeString.
      
      @param index - the zero based index of the char to return.<br>
       
      @return the char at that index OR if index >= length() return 0 and raise error 
      **/
    char charAt(unsigned int index) ; // if index >= length() returns 0 and prints a error msg
    /**
      returns the char at that location in this SafeString.
      
      @param index - the zero based index of the char to return.<br>
       
      @return the char at that index OR if index >= length() return 0 and raise error 
      **/
    char operator [] (unsigned int index) ; // if index >= length() returns 0 and prints a error msg

    // setting a char in the SafeString
    // str[..] = c;  is not supported because it allows direct access to modify the underlying char buffer
    /**
      sets the char at that location in this SafeString.
      
      @param index - the zero based index of the char to return.<br>
      @param c - the char to set at that index. '\\0' chars are ignored and an error raised.
      **/
    void setCharAt(unsigned int index, char c); //if index >= length() the error flag is set
    // calls to setCharAt(length(), ..) and  setCharAt(.. , '\0') are ignored and error flag is set

    // returning the underlying buffer
    // returned as a const and should not be changesdor recast to a non-const
    /**
      returns a const char* to the underlying char[ ] in this SafeString.
      <b>Do not make changes this char[ ]</b>
      **/
    const char* c_str();


    /* search methods  *******************
       Arrays are indexed by a unsigned int variable
       See the SafeStringIndexOf.ino example sketch
      All indexOf methods return -1 if not found
    **********************************************/
    // The fromIndex is offset into this SafeString where to start searching (inclusive)
    // 0 to length() and -1 is valid for fromIndex
    // if fromIndex > length(), than the error flag is set and -1 returned and prints an error if debug enabled
    // if fromIndex == (unsigned int)(-1) -1 is returned without error.
    /*
      returns the index
    */
    //int indexOf( char ch ) ;
    /**
      returns the index of the char, searching from fromIndex.
      @param ch - the char to search for
      @param fromIndex - where to start the search from, default 0, that is from begining<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
        
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int indexOf( char ch, unsigned int fromIndex = 0) ;
    //int indexOf( SafeString & str ) ;
    //int indexOf( const char* str ) ;
    /**
      returns the index of the string, searching from fromIndex.
      @param str - the string to search for
      @param fromIndex - where to start the search from, default 0, that is from begining<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int indexOf(const char* str , unsigned int fromIndex = 0) ;
    /**
      returns the index of the SafeString, searching from fromIndex.
      @param str - the SafeString to search for
      @param fromIndex - where to start the search from, default 0, that is from begining<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int indexOf( SafeString & str, unsigned int fromIndex = 0 ) ;

    /**
      returns the last index of the char, searching backwards from fromIndex (inclusive).
      @param c - the char to search for
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf( char ch ) ;

    /**
      returns the last index of the char, searching backwards from fromIndex (inclusive).
      @param c - the char to search for
      @param fromIndex - where to start searching backwards from,<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf( char ch, unsigned int fromIndex) ;

    /**
      returns the last index of the arguement, searching backwards from fromIndex (inclusive).
      @param str - the SafeString to search for
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf( SafeString & str ) ;
    /**
      returns the last index of the char, searching backwards from fromIndex (inclusive).
      @param str - the SafeString to search for
      @param fromIndex - where to start searching backwards from,<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf( SafeString & str, unsigned int fromIndex) ;

    /**
      returns the last index of the arguement, searching backwards from fromIndex (inclusive).
      @param cstr - the string to search for
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf( const char *cstr ) ;

    /**
      returns the last index of the char, searching backwards from fromIndex (inclusive).
      @param cstr - the string to search for
      @param fromIndex - where to start searching backwards from,<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int lastIndexOf(const char* cstr, unsigned int fromIndex);
    
    // first index of the chars listed in chars string
    // loop through chars and look for index of each and return the min index or -1 if none found
    //int indexOfCharFrom(SafeString & str);
    //int indexOfCharFrom(const char* chars);
    // start searching from fromIndex
    /**
      returns the first index of any char from the argument.
      @param str - the SafeString containing the chars to search for
      @param fromIndex - where to start searching from,<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int indexOfCharFrom(SafeString & str, unsigned int fromIndex = 0);

    /**
      returns the first index of any char from the argument
      @param chars - the string containing the chars to search for
      @param fromIndex - where to start searching backwards from,<br> if fromIndex > length() raise an error<br> if fromIndex == -1 OR fromIndex == length(), return -1 without error
      @return -1 if not found, else the index in the range 0 to length()-1
      */
    int indexOfCharFrom(const char* chars, unsigned int fromIndex = 0);


    /* *** substring methods ************/
    // substring is from beginIdx to end of string
    // The result substring is ALWAYS first cleared by this method so it will be empty on errors
    // if beginIdx = length(), an empty result will be returned without error
    // if beginIdx > length(), an empty result will be returned with error flag set on both this SafeString and the result SafeString
    // beginIdx == (unsigned int)(-1) returns an empty result without an error
    // You can take substring of yourself  e.g. str.substring(str,3);
    // if result does not have the capacity to hold the substring, hasError() is set on both this SafeString and the result SafeString
    
    /**
      The result is the substring from the beginIdx to the end of the SafeString
      @param result - the substring
      @param beginIdx - the index of the start of the substring, <br> if beginIdx == length(), the result is empty (no error)<br>
      if beginIdx > length(), the result is empty and an error is raised.<br>
      if beginIdx == (unsigned int)(-1),the result is empty (no error).
      
      @return result
      
      The result SafeString is ALWAYS cleares to start with and so will be empty if there are any errors.<br>
      If the result SafeString does not have the capacity to hold the substring, an empty result is returned and an error raised on both this SafeString and the result.<br>
      You can take a substring of yourself e.g.<br>
      <code>sfStr.substring(sfStr,3);<code>
      
      */
    SafeString & substring(SafeString & result, unsigned int beginIdx);

    // The result substring is ALWAYS first cleared by this method so it will be empty on errors
    // if beginIdx = length(), an empty result will be returned without error
    // if beginIdx > length(), an empty result will be returned with error flag set on both this SafeString and the result SafeString
    // if beginIdx > endIdx, beginIdx and endIdx will be swapped so that beginIdx <= endIdx and the error flag is set on both this SafeString and the result SafeString
    // if endIdx > length(), endIdx is set to length(); and the error flag is set on both this SafeString and the result SafeString
    // endIdx == (unsigned int)(-1) is treated as endIdx == length() returns a result without an error
    // substring is from beginIdx to endIdx-1, endIdx is exclusive
    // You can take substring of yourself  e.g. str.substring(str,3,6);
    // if result does not have the capacity to hold the substring, and empty result is returned and hasError() is set on both this SafeString and the result SafeString
    /**
      The result is the substring from the beginIdx to endIdx (exclusive), that is the endIdx is NOT included
      @param result - the substring
      @param beginIdx - the index of the start of the substring, <br> if beginIdx == length(), the result is empty (no error)<br>
      if beginIdx > length(), the result is empty and an error is raised.<br>
      if beginIdx == (unsigned int)(-1),the result is empty (no error).<br>
      if beginIdx > endIdx, beginIdx and endIdx will be swapped so that beginIdx <= endIdx and the error flag is set on both this SafeString and the result SafeString
      @param endIdx - the index after the end of the substring, that is endIdx is NOT included<br>
      if endIdx > length(), endIdx is set to length(); and the error flag is set on both this SafeString and the result SafeString<br>
      if endIdx == (unsigned int)(-1) is treated as endIdx == length() returns a result without an error
      
      @return result
      
      The result SafeString is ALWAYS cleares to start with and so will be empty if there are any errors.<br>
      If the result SafeString does not have the capacity to hold the substring, an empty result is returned and an error raised on both this SafeString and the result.<br>
      You can take a substring of yourself e.g.<br>
      <code>sfStr.substring(sfStr,3);<code>
      */
    SafeString & substring(SafeString & result, unsigned int beginIdx, unsigned int endIdx);

    /* *** SafeString modification methods ************/

    /* *** replace ************/
    /**
      replace the findChar with the replaceChar
      @param findChar - the char to be replaced
      @param replaceChar - the char to replace it
      */
    void replace(char findChar, char replaceChar);

    /**
      replace the findChar with the replace string
      @param findChar - the char to be replaced
      @param replaceStr - the string to replace it
      */
    void replace(const char findChar, const char *replaceStr);

    /**
      replace the findChar with the sfReplace SafeString contents
      @param findChar - the char to be replaced
      @param sfReplace - the SafeString whose contents will replace it
      */
    void replace(const char findChar, SafeString& sfReplace);

    /**
      replace the findStr string with the replace string
      @param findStr - the string to be replaced
      @param replaceStr - the string to replace it
      */
    void replace(const char* findStr, const char *replaceStr);

    /**
      replace the occurances of the sfFind string, with the sfReplace SafeString contents
      @param sfFind - the SafeString containing the string of chars to be replaced
      @param sfReplace - the SafeString whose contents will replace it
      */
    void replace(SafeString & sfFind, SafeString & sfReplace);

    /* *** remove ************/
    // remove from index to end of SafeString
    // 0 to length() and (unsigned int)(-1) are valid for index,
    // -1 => length() for processing and just returns without error
    /**
      remove all chars from startIndex to the end of the SafeString (inclusive)
      @param startIndex - the index of the first char to be removed<br>
      Valid startIndex is 0 to length(), and -1 which is treated as length()<br>
      Other startIndex raise an error.
      */
    void removeFrom(unsigned int startIndex);

    // remove from 0 to startIdx (excluding startIdx)
    // 0 to length() and (unsigned int)(-1) are valid for index,
    // -1 => length() for processing
    /**
      remove all chars from 0 to startIndex (exclusive), that is the char at startIndex is NOT removed
      @param startIndex - the index of the first char NOT to be removed<br>
      Valid startIndex is 0 to length(), and -1 which is treated as length()<br>
      Other startIndex raise an error.
      */
    void removeBefore(unsigned int startIndex);

    // remove from index to end of SafeString
    // 0 to length() and (unsigned int)(-1) are valid for index,
    // -1 => length() for processing and just returns without error
    /**
      remove all chars from index to the end of the SafeString (inclusive)
      @param index - the index of the first char to be removed<br>
      Valid index is 0 to length(), and -1 which is treated as length()<br>
      Other index raise an error.
      */
    void remove(unsigned int index);

    // remove count chars starting from index
    // 0 to length() and unsigned int(-1) are valid for index
    // -1 just returns without error
    // 0 to (length()- index) is valid for count, larger values set the error flag and remove from idx to end of string
    /**
      remove count chars starting from index
      @param index - the index of the first char to be removed<br>
      Valid index is 0 to length(), and -1 which is treated as length()<br>
      Other index raise an error.
      @param count - the number of chars to remove<br> 
      if count > length() - index, count is set to length() - index, and a error raised.
      */
    void remove(unsigned int index, unsigned int count);

    // remove the last 'count' chars
    // 0 to length() is valid for count,
    // count >= length() clears the SafeString
    // count > length() set the error flag
    /**
      remove the last count chars
      @param count - the number of chars to remove<br>
      0 to length() is valid for count<br>
      count >= length() clears the SafeString<br>
      count > length() raises and error
      */
    void removeLast(unsigned int count);

    // keep last 'count' number of chars remove the rest
    // 0 to length() is valid for count, passing in count == 0 clears the SafeString
    // count > length() sets error flag and returns SafeString unchanged
    /**
      keep the last count chars and remove the rest
      @param count - the number of chars to keep<br>
      0 to length() is valid for count<br>
      count == 0 clears the SafeString<br>
      count > length() raises and error and leaves the SafeString unchanged
      */
    void keepLast(unsigned int count);


    /* *** change case ************/
    /**
      convert this SafeString to all lower case
      */
    void toLowerCase(void);
    /**
      convert this SafeString to all lower case
      */
    void toUpperCase(void);

    /* *** remove white space from front and back of SafeString ************/
    // the method isspace( ) is used to.  For the 'C' local the following are trimmed
    //    ' '     (0x20)  space (SPC)
    //    '\t'  (0x09)  horizontal tab (TAB)
    //    '\n'  (0x0a)  newline (LF)
    //    '\v'  (0x0b)  vertical tab (VT)
    //    '\f'  (0x0c)  feed (FF)
    //    '\r'  (0x0d)  carriage return (CR)
    /**
      remove all white space from the front and back of this SafeString.
      
      The C method isspace() is used. For the 'C' local the following are trimmed<br>
        ' '     (0x20)  space (SPC)<br>
        '\\t'  (0x09)  horizontal tab (TAB)<br>
        '\\n'  (0x0a)  newline (LF)<br>
        '\\v'  (0x0b)  vertical tab (VT)<br>
        '\\f'  (0x0c)  feed (FF)<br>
        '\\r'  (0x0d)  carriage return (CR)<br>
      */
    void trim(void); // trims front and back

    // processBackspaces recursively remove backspaces, '\b' and the preceeding char
    // use for processing inputs from terminal (Telent) connections
    /**
      recursively remove backspaces, '\\b' and the preceeding char.
      
      useful for processing inputs from terminal (Telent) connections
      */
    void processBackspaces(void);

    /* *** numgber parsing/conversion ************/
    // convert numbers
    // If the SafeString is a valid number update the argument with the result
    // else leave the argument unchanged
    // SafeString conversions are stricter than the Arduino String version
    // trailing chars can only be white space
    /**
      convert the SafeString to an int.
      @param i -- int reference, where the result is stored. i is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char toInt(int & i) ;
    /**
      convert the SafeString to a long.
      @param l -- long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char toLong(long & l) ;

    /**
      convert the SafeString to a long assuming the SafeString in binary (0/1).
      @param l -- long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char binToLong(long & l) ;
    /**
      convert the SafeString to a long assuming the SafeString in octal (0 to 7).
      @param l -- long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char octToLong(long & l) ;
    /**
      convert the SafeString to a long assuming the SafeString in HEX (0 to f or 0 to F).
      @param l -- long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char hexToLong(long & l) ;
    /**
      convert the SafeString to an unsigned long.
      @param l -- long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char toUnsignedLong(unsigned long & l) ;
    /**
      convert the SafeString to an unsigned long assuming the SafeString in binary (0/1).
      @param l -- unsigned long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char binToUnsignedLong(unsigned long & l) ;
    /**
      convert the SafeString to an unsigned long assuming the SafeString in octal (0 to 7).
      @param l -- unsigned long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char octToUnsignedLong(unsigned long & l) ;
    /**
      convert the SafeString to an unsigned long assuming the SafeString in HEX (0 to f or 0 to F).
      @param l -- unsigned long reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */         
    unsigned char hexToUnsignedLong(unsigned long & l) ;
    /**
      convert the SafeString to a float assuming the SafeString in the decimal format (not scientific)
      @param f -- float reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */          
    unsigned char toFloat(float  & f) ;
    /**
      convert the SafeString to a float assuming the SafeString in the decimal format (not scientific)
      @param d -- double reference, where the result is stored. l is only updated if the conversion is successful
      @return -- 0 if the SafeString is not a valid int, else return non-zero<br>
        leading and trailing white space is allowed around a valid int
     */          
    unsigned char toDouble(double & d) ;

    // float toFloat(); possible alternative

    /* Tokenizeing methods,  stoken(), nextToken()/firstToken() ************************/
    /* Differences between stoken() and nextToken
       stoken() leaves the SafeString unchanged, nextToken() removes the token (and leading delimiters) from the SafeString giving space to add more input
       In stoken() the end of the SafeString is always treated as a delimiter, i.e. the last token is returned even if it is not followed by one of the delimiters
       In nextToken() the end of the SafeString is a delimiter by default, but setting returnLastNonDelimitedToken =  false will leave last token that is not terminated in the SafeString
       Setting returnLastNonDelimitedToken = false this allows partial tokens to be read from a Stream and kept until the full token and delimiter is read
    */
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

    /**
         break into the SafeString into tokens using the char delimiter, the end of the SafeString is always a delimiter
         
         Any leading delimiters are first stepped over and then the delimited token is return in the token argument (less the delimiter).<br>
         The token argument is always cleared at the start of the stoken().<br>
         if there are any argument errors or the token does not have the capacity to hold the substring, hasError() is set on both this SafeString and the token SafeString<br>

         @param token - the SafeString to return the token in, it is cleared if no delimited token found or if there are errors<br>
                 The found delimited token (less the delimiter) is returned in the token SafeString argument if there is capacity.<br>
                 The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
                 If the token's capacity is < the next token, then token is returned empty and an error messages printed if debug is enabled.<br>
                 in this case the return (nextIndex) is still updated.<br>
         @param fromIndex - where to start the search from  0 to length() and -1 is valid for fromIndex,  -1 => length() for processing
         @param delimiter - the char that delimits a token. The end of the SafeString is always a delimiter.
         @param returnEmptyFields - default false, if true only skip one leading delimiter after each call.<br>
             If the fromIndex is 0 and there is a delimiter at the beginning of the SafeString, an empty token will be returned
         @param useAsDelimiters - default true, if false then token will consists of only chars in the delimiters and any other char terminates the token

         @return - nextIndex, the next index in this SafeString after the end of the token just found, -1 if this is the last token<br>
                  Use this as the fromIndex for the next call<br>
                  NOTE: if there are no delimiters then -1 is returned and the whole SafeString returned in token if the SafeString token argument is large enough<br>
                  If the token's capacity is < the next token, the token returned is empty and an error messages printed if debug is enabled.<br>
                  In this case the returned nextIndex is still updated to end of the token just found so that that the program will not be stuck in an infinite loop testing for nextIndex >=0<br>
                  while being consistent with the SafeString's all or nothing insertion rule<br>
                  Input argument errors return -1 and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    int stoken(SafeString & token, unsigned int fromIndex, const char delimiter, bool returnEmptyFields = false, bool useAsDelimiters = true);

    /**
         break into the SafeString into tokens using the delimiters, the end of the SafeString is always a delimiter
         
         Any leading delimiters are first stepped over and then the delimited token is return in the token argument (less the delimiter).<br>
         The token argument is always cleared at the start of the stoken().<br>
         if there are any argument errors or the token does not have the capacity to hold the substring, hasError() is set on both this SafeString and the token SafeString<br>

         @param token - the SafeString to return the token in, it is cleared if no delimited token found or if there are errors<br>
                 The found delimited token (less the delimiter) is returned in the token SafeString argument if there is capacity.<br>
                 The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
                 If the token's capacity is < the next token, then token is returned empty and an error messages printed if debug is enabled.<br>
                 in this case the return (nextIndex) is still updated.<br>
         @param fromIndex - where to start the search from  0 to length() and -1 is valid for fromIndex,  -1 => length() for processing
         @param delimiters - the string containing the characters that any one of which can delimit a token. The end of the SafeString is always a delimiter.
         @param returnEmptyFields - default false, if true only skip one leading delimiter after each call.<br>
             If the fromIndex is 0 and there is a delimiter at the beginning of the SafeString, an empty token will be returned
         @param useAsDelimiters - default true, if false then token will consists of only chars in the delimiters and any other char terminates the token

         @return - nextIndex, the next index in this SafeString after the end of the token just found, -1 if this is the last token<br>
                  Use this as the fromIndex for the next call<br>
                  NOTE: if there are no delimiters then -1 is returned and the whole SafeString returned in token if the SafeString token argument is large enough<br>
                  If the token's capacity is < the next token, the token returned is empty and an error messages printed if debug is enabled.<br>
                  In this case the returned nextIndex is still updated to end of the token just found so that that the program will not be stuck in an infinite loop testing for nextIndex >=0<br>
                  while being consistent with the SafeString's all or nothing insertion rule<br>
                  Input argument errors return -1 and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    int stoken(SafeString & token, unsigned int fromIndex, const char* delimiters, bool returnEmptyFields = false, bool useAsDelimiters = true);

    /**
         break into the SafeString into tokens using the delimiters, the end of the SafeString is always a delimiter
         
         Any leading delimiters are first stepped over and then the delimited token is return in the token argument (less the delimiter).<br>
         The token argument is always cleared at the start of the stoken().<br>
         if there are any argument errors or the token does not have the capacity to hold the substring, hasError() is set on both this SafeString and the token SafeString<br>

         @param token - the SafeString to return the token in, it is cleared if no delimited token found or if there are errors<br>
                 The found delimited token (less the delimiter) is returned in the token SafeString argument if there is capacity.<br>
                 The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
                 If the token's capacity is < the next token, then token is returned empty and an error messages printed if debug is enabled.<br>
                 in this case the return (nextIndex) is still updated.<br>
         @param fromIndex - where to start the search from  0 to length() and -1 is valid for fromIndex,  -1 => length() for processing
         @param delimiters - the SafeString containing the characters that any one of which can delimit a token. The end of the SafeString is always a delimiter.
         @param returnEmptyFields - default false, if true only skip one leading delimiter after each call.<br>
             If the fromIndex is 0 and there is a delimiter at the beginning of the SafeString, an empty token will be returned
         @param useAsDelimiters - default true, if false then token will consists of only chars in the delimiters and any other char terminates the token

         @return - nextIndex, the next index in this SafeString after the end of the token just found, -1 if this is the last token<br>
                  Use this as the fromIndex for the next call<br>
                  NOTE: if there are no delimiters then -1 is returned and the whole SafeString returned in token if the SafeString token argument is large enough<br>
                  If the token's capacity is < the next token, the token returned is empty and an error messages printed if debug is enabled.<br>
                  In this case the returned nextIndex is still updated to end of the token just found so that that the program will not be stuck in an infinite loop testing for nextIndex >=0<br>
                  while being consistent with the SafeString's all or nothing insertion rule<br>
                  Input argument errors return -1 and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    int stoken(SafeString & token, unsigned int fromIndex, SafeString & delimiters, bool returnEmptyFields = false, bool useAsDelimiters = true);

    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token when nextToken() is called<br>
      this firstToken() method overrides this and returns true and an empty token if the first char is a delimiter
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>
      
      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then nextToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiter - the char which can delimit a token
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      NOTE: since the last delimiter is left in the SafeString, you should only use firstToken() for the first call.

      @return -- true if firstToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    inline unsigned char firstToken(SafeString & token, char delimiter, bool returnLastNonDelimitedToken = true) {
    	return nextToken(token,delimiter,true,returnLastNonDelimitedToken,true);
    }

    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token, the firstToken argument can override this<br>
      setting the firstToken arguemnent = true, suppresses this so that an empty first token can be returned if the first char is a delimiter AND if returnEmptyFields is true
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>
      Setting firstToken = true suppressed skipping over a leading delimiter so that an empty first field can be returned if the first char is a delimiter

      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then firstToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiter - the char which can delimit a token
      @param returnEmptyFields -- default false, if true, nextToken() will return true, and an empty token for each consecutive delimiters
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      @param firstToken -- default false, a leading delimiter will be stepped over before looking for a delimited token<br>
      if set to true, a leading delimiter will delimit an empty token which will be returned only if returnEmptyFields is true otherwise it is skipped over.<br>
      NOTE: if returnEmptyFields == false this firstToken argument has no effect.<br>
      NOTE: since the last delimiter is left in the SafeString, you must set firstToken to be false (or omit it) after the first call.

      @return -- true if nextToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    unsigned char nextToken(SafeString & token, char delimiter, bool returnEmptyFields = false, bool returnLastNonDelimitedToken = true, bool firstToken = false);

    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token when nextToken() is called<br>
      this firstToken() method overrides this and returns true and an empty token if the first char is a delimiter
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>
      
      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then nextToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiters - the SafeString containing the delimiting characters, any one of which can delimit a token
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      NOTE: since the last delimiter is left in the SafeString, you should only use firstToken() for the first call.

      @return -- true if firstToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    inline unsigned char firstToken(SafeString & token, SafeString delimiters, bool returnLastNonDelimitedToken = true) {
    	return nextToken(token,delimiters,true,returnLastNonDelimitedToken,true);
    }
    
    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token, the firstToken argument can override this<br>
      setting the firstToken arguemnent = true, suppresses this so that an empty first token can be returned if the first char is a delimiter AND if returnEmptyFields is true
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>
      Setting firstToken = true suppressed skipping over a leading delimiter so that an empty first field can be returned if the first char is a delimiter

      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then firstToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiters - the SafeString containing the delimiting characters, any one of which can delimit a token
      @param returnEmptyFields -- default false, if true, nextToken() will return true, and an empty token for each consecutive delimiters
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      @param firstToken -- default false, a leading delimiter will be stepped over before looking for a delimited token<br>
      if set to true, a leading delimiter will delimit an empty token which will be returned only if returnEmptyFields is true otherwise it is skipped over.<br>
      NOTE: if returnEmptyFields == false this firstToken argument has no effect.<br>
      NOTE: since the last delimiter is left in the SafeString, you must set firstToken to be false (or omit it) after the first call.

      @return -- true if nextToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    unsigned char nextToken(SafeString & token, SafeString & delimiters, bool returnEmptyFields = false, bool returnLastNonDelimitedToken = true, bool firstToken = false);

    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token when nextToken() is called<br>
      this firstToken() method overrides this and returns true and an empty token if the first char is a delimiter
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>

      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then firstToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiters - the string containing the delimiting characters, any one of which can delimit a token
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      NOTE: since the last delimiter is left in the SafeString, you should only use firstToken() for the first call.

      @return -- true if firstToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    inline unsigned char firstToken(SafeString & token, const char* delimiters, bool returnLastNonDelimitedToken = true) {
    	return nextToken(token,delimiters,true,returnLastNonDelimitedToken,true);
    }
    
    /**
      returns true if a delimited token is found, removes the first delimited token from this SafeString and returns it in the token argument<br>
      by default a leading delimiter is stepped over before scanning for a delimited token, the firstToken argument can override this<br>
      setting the firstToken arguemnent = true, suppresses this so that an empty first token can be returned if the first char is a delimiter AND if returnEmptyFields is true
       
      The delimiter is not returned and remains in the SafeString so you can test which delimiter terminated the token, provided this SafeString is not empty!<br>
      The token argument is always cleared at the start of the nexttToken().<br>
      IMPORTANT!! Changed V4.0.4 Now by default un-delimited tokens at the end of the SafeString are returned<br>
      To leave partial un-delimited tokens on the end of the SafeString, set returnLastNonDelimitedToken = false.<br>
      Setting returnLastNonDelimitedToken = false allows the SafeString to hold partial tokens when reading from an input stream one char at a time.<br>
      Setting firstToken = true suppressed skipping over a leading delimiter so that an empty first field can be returned if the first char is a delimiter

      @param token - the SafeString to return the token in, it is always cleared first and will be empty if no delimited token is found or if there are errors<br>
              The token's capacity should be >= this SafeString's capacity incase the entire SafeString needs to be returned.<br>
              If the token's capacity is < the next token, then nextToken() returns true, but the returned token argument is empty and an error messages printed if debug is enabled.<br>
              In this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()
      @param delimiters - the string containing the delimiting characters, any one of which can delimit a token
      @param returnEmptyFields -- default false, if true, nextToken() will return true, and an empty token for each consecutive delimiters
      @param returnLastNonDelimitedToken -- default true, will return last part of SafeString even if not delimited. If set false, will keep it for further input to be added to this SafeString
      @param firstToken -- default false, a leading delimiter will be stepped over before looking for a delimited token<br>
      if set to true, a leading delimiter will delimit an empty token which will be returned only if returnEmptyFields is true otherwise it is skipped over.<br>
      NOTE: if returnEmptyFields == false this firstToken argument has no effect.<br>
      NOTE: since the last delimiter is left in the SafeString, you must set firstToken to be false (or omit it) after the first call.

      @return -- true if nextToken() finds a token in this SafeString that is terminated by one of the delimiters, else false<br>
                If the return is true, but hasError() is true then the SafeString token argument did not have the capacity to hold the next token.<br>
                in this case to next token is still removed from the SafeString so that the program will not be stuck in an infinite loop calling nextToken()<br>
                while being consistent with the SafeString's all or nothing insertion rule<br>
               Input argument errors return false and an empty token and hasError() is set on both this SafeString and the token SafeString.
    **/
    unsigned char nextToken(SafeString & token, const char* delimiters, bool returnEmptyFields = false, bool returnLastNonDelimitedToken = true, bool firstToken = false);


    /* *** ReadFrom from SafeString, writeTo SafeString ************************/
    /**
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
    unsigned int readFrom(SafeString & sfInput, unsigned int startIdx = 0);

    /**
       reads from the const char* argument, starting at 0 and read up to maxCharToRead, into this SafeString.
       
       This lets you read from a char* into a SafeString without errors if the strlen(char*) or maxCharsToRead are larger than the SafeString capacity
       Use sfResult.clear(); to empty the SafeString first and then sfResult.readFrom(strPtr); to read a much as you can
       The read stops at first '\0' or the calling SafeString is full or when maxCharsToRead have been read.
       Note: if the SafeString is already full, then nothing will be read
       
       @param  strPtr - pointer char array to read from
       @param  maxCharsToRead -- the maximum chars to read into the SafeString, defaults to ((unsigned int)-1) i.e. max unsigned int.
	   
       @return - the number of chars read
    **/
    unsigned int readFrom(const char* strPtr, unsigned int maxCharsToRead = ((unsigned int)-1));
    
    /**
       writes from this SafeString, starting from startIdx, into the SafeString output arguement.
       
       The write stops when the end if the calling SafeString is reached or the output is full
       Note: if the output is already full, then nothing will be written and startIdx will be returned unchanged
       
       @param  output - the SafeString to write to
       @param  startIdx - where to start writing from in the calling SafeString, defaults to 0,
                    if startIdx >= length(), nothing written and length() returned

       @return new startIdx for next write
    **/
    unsigned int writeTo(SafeString & output, unsigned int startIdx = 0);

    /* *** NON-blocking reads from Stream ************************/
    
    /**
      reads from the Stream (if chars available) into the SafeString.
      
      The is NON-BLOCKING and returns immediately if nothing available to be read
      Note: if the SafeString is already full, then nothing will be read and false will be returned

      @param - the Stream reference to read from
      @return true if something added to this SafeString, else false
     **/
    unsigned char read(Stream & input);

    /**
      reads chars into this SafeString until either it is full OR a delimiter is read OR there are no more chars available<br>
      returns true if a delimiter found or SafeString is full, else false<br>
      if a delimiter is found it is returned at the end of the SafeString<br>
      Only at most one delimiter is added per call<br>
      Multiple sucessive delimiters require multiple calls to read them

      @param  input - the Stream reference to read from
      @param  delimiter - the char which is the delimieter
      @return true if SafeString is full or a delimiter is read, else false<br>
    **/
    unsigned char readUntil(Stream & input, const char delimiter);
    /**
      reads chars into this SafeString until either it is full OR a delimiter is read OR there are no more chars available<br>
      returns true if a delimiter found or SafeString is full, else false<br>
      if a delimiter is found it is returned at the end of the SafeString<br>
      Only at most one delimiter is added per call<br>
      Multiple sucessive delimiters require multiple calls to read them

      @param  input - the Stream reference to read from
      @param  delimiters - the string containing the characters any one of which can be a delimieter
      @return true if SafeString is full or a delimiter is read, else false<br>
    **/
    unsigned char readUntil(Stream & input, const char* delimiters);
    /**
      reads chars into this SafeString until either it is full OR a delimiter is read OR there are no more chars available<br>
      returns true if a delimiter found or SafeString is full, else false<br>
      if a delimiter is found it is returned at the end of the SafeString<br>
      Only at most one delimiter is added per call<br>
      Multiple sucessive delimiters require multiple calls to read them

      @param  input - the Stream reference to read from
      @param  delimiters - the SafeString containing the characters any one of which can be a delimieter
      @return true if SafeString is full or a delimiter is read, else false<br>
    **/
    unsigned char readUntil(Stream & input, SafeString & delimiters);

    /**
      returns true if a delimited token is found, else false<br>
      ONLY delimited tokens of length less than this SafeString's capacity will return true with a non-empty token.<br>
      Streams of chars that overflow this SafeString's capacity are ignored and return an empty token on the next delimiter<br>
      A timeout can be specified to return the last un-delimited token after chars stop arriving.<br>
      The input read can be echoed back to the input Stream<br>
      You can force a skip to the next delimiter to discard partial tokens<br>
      
      That is this SafeString's capacity should be at least 1 more then the largest expected token.<br>
      If this SafeString OR the SafeString& token return argument is too small to hold the result, the token is returned empty and an error message output if debugging is enabled.<br>
      The delimiter is NOT included in the SafeString& token return.  It will the first char of the this SafeString when readUntilToken returns true<br>
      It is recommended that the capacity of the SafeString& token argument be >= this SafeString's capacity<br>
      Each call to this method removes any leading delimiters so if you need to check the delimiter do it BEFORE the next call to readUntilToken()

      @param input - the Stream reference to read from
      @param token - the SafeString to return the token found if any, this always cleared at the start of this method
      @param delimiter - the char which is the delimieter
      @param skipToDelimiter - a bool reference variable to hold the skipToDelimiter state between calls<br>
      If this is true all chars upto the next delimiter (or timeout) will be discarded and false returned with an empty token
      @param echoInput - defaults to false, pass non-zero (true) to echo the chars read back to the input Stream
      @param timeout_ms - defaults to never timeout, pass a non-zero ms to auto-terminate the last token if no new chars received for that time.

      @return - true if a delimited series of chars found that fit in this SafeString else false<br>
      If this SafeString OR the SafeString& token argument is too small to hold the result, the token is returned empty<br>
      If a delimited token is found that fits in this SafeString but is too large for the token then true is returned and an empty token returned and an error raised on both this SafeString and the token<br>
      The delimiter is NOT included in the SafeString& token return. It will the first char of the this SafeString when readUntilToken returns true
    **/
    unsigned char readUntilToken(Stream & input, SafeString & token, const char delimiter, bool & skipToDelimiter, uint8_t echoInput = false, unsigned long timeout_ms = 0);

    /**
      returns true if a delimited token is found, else false<br>
      ONLY delimited tokens of length less than this SafeString's capacity will return true with a non-empty token.<br>
      Streams of chars that overflow this SafeString's capacity are ignored and return an empty token on the next delimiter<br>
      A timeout can be specified to return the last un-delimited token after chars stop arriving.<br>
      The input read can be echoed back to the input Stream<br>
      You can force a skip to the next delimiter to discard partial tokens<br>
      
      That is this SafeString's capacity should be at least 1 more then the largest expected token.<br>
      If this SafeString OR the SafeString& token return argument is too small to hold the result, the token is returned empty and an error message output if debugging is enabled.<br>
      The delimiter is NOT included in the SafeString& token return.  It will the first char of the this SafeString when readUntilToken returns true<br>
      It is recommended that the capacity of the SafeString& token argument be >= this SafeString's capacity<br>
      Each call to this method removes any leading delimiters so if you need to check the delimiter do it BEFORE the next call to readUntilToken()

      @param input - the Stream reference to read from
      @param token - the SafeString to return the token found if any, this always cleared at the start of this method
      @param delimiters - the string containing the characters any one of which can be a delimieter
      @param skipToDelimiter - a bool reference variable to hold the skipToDelimiter state between calls<br>
      If this is true all chars upto the next delimiter (or timeout) will be discarded and false returned with an empty token
      @param echoInput - defaults to false, pass non-zero (true) to echo the chars read back to the input Stream
      @param timeout_ms - defaults to never timeout, pass a non-zero ms to auto-terminate the last token if no new chars received for that time.

      @return - true if a delimited series of chars found that fit in this SafeString else false<br>
      If this SafeString OR the SafeString& token argument is too small to hold the result, the token is returned empty<br>
      If a delimited token is found that fits in this SafeString but is too large for the token then true is returned and an empty token returned and an error raised on both this SafeString and the token<br>
      The delimiter is NOT included in the SafeString& token return. It will the first char of the this SafeString when readUntilToken returns true
    **/
    unsigned char readUntilToken(Stream & input, SafeString & token, const char* delimiters, bool & skipToDelimiter, uint8_t echoInput = false, unsigned long timeout_ms = 0);

    /**
      returns true if a delimited token is found, else false<br>
      ONLY delimited tokens of length less than this SafeString's capacity will return true with a non-empty token.<br>
      Streams of chars that overflow this SafeString's capacity are ignored and return an empty token on the next delimiter<br>
      A timeout can be specified to return the last un-delimited token after chars stop arriving.<br>
      The input read can be echoed back to the input Stream<br>
      You can force a skip to the next delimiter to discard partial tokens<br>
      
      That is this SafeString's capacity should be at least 1 more then the largest expected token.<br>
      If this SafeString OR the SafeString& token return argument is too small to hold the result, the token is returned empty and an error message output if debugging is enabled.<br>
      The delimiter is NOT included in the SafeString& token return.  It will the first char of the this SafeString when readUntilToken returns true<br>
      It is recommended that the capacity of the SafeString& token argument be >= this SafeString's capacity<br>
      Each call to this method removes any leading delimiters so if you need to check the delimiter do it BEFORE the next call to readUntilToken()

      @param input - the Stream reference to read from
      @param token - the SafeString to return the token found if any, this always cleared at the start of this method
      @param delimiters - the SafeString containing the characters any one of which can be a delimieter
      @param skipToDelimiter - a bool reference variable to hold the skipToDelimiter state between calls<br>
      If this is true all chars upto the next delimiter (or timeout) will be discarded and false returned with an empty token
      @param echoInput - defaults to false, pass non-zero (true) to echo the chars read back to the input Stream
      @param timeout_ms - defaults to never timeout, pass a non-zero ms to auto-terminate the last token if no new chars received for that time.

      @return - true if a delimited series of chars found that fit in this SafeString else false<br>
      If this SafeString OR the SafeString& token argument is too small to hold the result, the token is returned empty<br>
      If a delimited token is found that fits in this SafeString but is too large for the token then true is returned and an empty token returned and an error raised on both this SafeString and the token<br>
      The delimiter is NOT included in the SafeString& token return. It will the first char of the this SafeString when readUntilToken returns true
    **/
    unsigned char readUntilToken(Stream & input, SafeString & token, SafeString & delimiters, bool & skipToDelimiter, uint8_t echoInput = false, unsigned long timeout_ms = 0);

    /**
      returns the number of chars read on previous calls to read, readUntil or readUntilToken (includes '\0' read if any).
      
      Each call read, readUntil, readUntilToken first clears this count
      @returns - the char count read since the last call to this method.
      */
    size_t getLastReadCount(); 
    
    /* *** END OF PUBLIC METHODS ************/

  protected:
    static Print* debugPtr;
    static bool fullDebug;
    char *buffer;          // the actual char array
    size_t _capacity; // the array length minus one (for the '\0')
    size_t len;       // the SafeString length (not counting the '\0')

    class noDebugPrint : public Print {
      public:
        inline size_t write(uint8_t b) {
          (void)(b);
          return 0;
        }
        inline size_t write(const uint8_t *buffer, size_t length) {
          (void)(buffer);
          (void)(length);
          return 0;
        };
        void flush() { }
    };

    static SafeString::noDebugPrint emptyPrint;

    static Print* currentOutput;// = &emptyPrint;

    class DebugPrint : public Print {
      public:
        size_t write(uint8_t b) {
          return currentOutput->write(b);
        }
        size_t write(const uint8_t *buffer, size_t length) {
          return currentOutput->write(buffer, length);
        };
        void flush() {
#if defined(ESP_PLATFORM) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF5) || defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(ARDUINO_NRF52832_FEATHER) ||  defined(MEGATINYCORE_MAJOR)
          // ESP32 has no flush in Print!! but ESP8266 has
#else
          currentOutput->flush();
#endif
        }
    };

  public:
    static SafeString::DebugPrint Output; // a Print object controlled by setOutput() / turnOutputOff()

  protected:
    SafeString & concatln(const __FlashStringHelper * pstr);
    SafeString & concatln(char c);
    SafeString & concatln(const char *cstr, size_t length);
    void outputName() const ;
    SafeString & concatInternal(const char *cstr, size_t length, bool assignOp = false); // concat at most length chars from cstr
    SafeString & concatInternal(const __FlashStringHelper * str, size_t length, bool assignOp = false); // concat at most length chars

    SafeString & concatInternal(const char *cstr, bool assignOp = false);
    SafeString & concatInternal(char c, bool assignOp = false);
    SafeString & concatInternal(const __FlashStringHelper * str, bool assignOp = false);
    size_t printInternal(long, int = DEC, bool assignOp = false);
    size_t printInternal(unsigned long, int = DEC, bool assignOp = false);
    size_t printInternal(double, int = 2, bool assignOp = false);
    void setError();
    void printlnErr()const ;
    void debugInternalMsg(bool _fullDebug) const ;
    size_t limitedStrLen(const char* p, size_t limit);
    size_t printInt(double d, int decs, int width, bool forceSign, bool addNL);

  private:
    bool readUntilTokenInternal(Stream & input, SafeString & token, const char* delimitersIn, char delimiterIn, bool & skipToDelimiter, uint8_t echoInput, unsigned long timeout_ms);
    bool readUntilInternal(Stream & input, const char* delimitersIn, char delimiterIn);
    bool nextTokenInternal(SafeString & token, const char* delimitersIn, char delimiterIn, bool returnEmptyFields, bool returnLastNonDelimitedToken);
    int stokenInternal(SafeString &token, unsigned int fromIndex, const char* delimitersIn, char delimiterIn, bool returnEmptyFields, bool useAsDelimiters);
    bool fromBuffer; // true if createSafeStringFromBuffer created this object
    bool errorFlag; // set to true if error detected, cleared on each call to hasError()
    static bool classErrorFlag; // set to true if any error detected in any SafeString, cleared on each call to SafeString::errorDetected()
    void cleanUp(); // reterminates buffer at capacity and resets len to current strlen
    const char *name;
    unsigned long timeoutStart_ms;
    bool timeoutRunning;
    size_t noCharsRead; // number of char read on last call to readUntilToken
    static char nullBufferSafeStringBuffer[1];
    static char emptyDebugRtnBuffer[1];
    void debugInternal(bool _fullDebug) const ;
    void debugInternalResultMsg(bool _fullDebug) const ;
    void concatErr()const ;
    void concatAssignError() const;
    void prefixErr()const ;
    void capError(const __FlashStringHelper * methodName, size_t neededCap, const char* cstr, const __FlashStringHelper *pstr = NULL, char c = '\0', size_t length = 0)const ;
    void assignError(size_t neededCap, const char* cstr, const __FlashStringHelper *pstr = NULL, char c = '\0', bool numberFlag = false) const;
    void errorMethod(const __FlashStringHelper * methodName) const ;
    void warningMethod(const __FlashStringHelper * methodName) const ;
    void assignErrorMethod() const ;
    void outputFromIndexIfFullDebug(unsigned int fromIndex) const ;
};

#include "SafeStringNameSpaceEnd.h"

#endif  // __cplusplus
#endif  // SafeString_class_h