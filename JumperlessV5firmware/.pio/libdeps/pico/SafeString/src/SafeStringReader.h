#ifndef SAFE_STRING_READER_H
#define SAFE_STRING_READER_H
/*
  SafeStringReader.h  a tokenizing Stream reader
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
**/
#ifdef __cplusplus
#include <Arduino.h>
#include "SafeString.h"
// SafeString.h includes defines for Stream

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

/**
  createSafeStringReader( )
  params
    name - name of this SafeStringReader variable (DO NOT use " " just use the plain name see the examples)
    size - the maximum length of the delimited token that can be read, NOT including the delimiter
           If more then size chars read without finding a delimiter they are discarded and all chars upto the next delimiter are also discarded.
           I.e. tokens longer than size are igorned and not returned.
    delimiters - either a char ('\n') or a string of delimiters ("\r\n,.")
    skipToDelimiterFlag - true if all data upto the first delimiter received should be ignored, default false
    echoInput - true if all chars read (including delimiters) are to be echoed back to the Stream being read from, default false
    timeout_ms - the ms timeout, if no more chars read for this timeout, the current input is returned as the token. In this case getDelimiter() returns -1,  default 0 i.e. never times out must read a delimiter
    
    example
    createSafeStringReader(sfReader,20,'\n',false,true,100);
    This creates a SafeStringReader sfReader which can read delimited tokens upto 20 chars long (not including the delimiter), no initial skipToDelimiter, echo is ON, timeout is 100ms
    If no '\n' is received and no new chars read for 100ms the currently read char will be returned as the token
  
*/
#define createSafeStringReader(name, size, ...) \
  char name ## _INPUT_BUFFER[(size)+2]; \
  char name ## _TOKEN_BUFFER[(size)+2]; \
  SafeString name ## _SF_INPUT((size)+2,  name ## _INPUT_BUFFER, "", #name "_InputBuffer"); \
  SafeStringReader name(name ## _SF_INPUT, (size)+2, name ## _TOKEN_BUFFER, #name, __VA_ARGS__ );


// size + 1 for delimiter, + 1 for '\0'  actually token only ever size+1 for '\0' as delimiter not returned
// size is the maximum size of the token to be read ignoring the delimiter
/**************
  To create a SafeStringReader use the macro **createSafeStringReader**  see the detailed description. 
  
  The createSafeStringReader macro takes 3 or more arguments.<br> 
  createSafeStringReader(name, size, delimiters); creates a SafeStringReader called <i>name</i> which can read tokens upto <i>size</i> chars and will return tokens delimited by <i>delimiters</i>.<br>
  e.g. to create a SafeStringReader called sfReader to handle tokens upto 80char long and return tokens delimited by comma or newline or carriageReturn use<br>
 <code>createSafeStringReader(sfReader, 80, ",\n\r")</code><br>
 
  Other optional arguments for createSafeStringReader are better set via method calls.  The examples below use the created SafeStringReader called, <i><b>sfReader</b></i> from above<br>
  To control echoing the input back to the input stream ( default echoOff() ) use<br>
    <code>sfReader.echoOn();</code><br>
    and<br>
    <code>sfReader.echoOff();</code><br>
    
  To set the read timeout, in ms, after which the current input will be returned as a token even if no delimiter has been found (default 0, never timeout) use<br>
  <code>sfReader.setTimeout(ms);</code><br>
  
  To clear/flush all the pending input upto the next delimiter or timeout, use<br>
  <code>sfReader.setTimeout(1000);<br>
  sfReader.flush();<br>
  sfReader.setTimeout(0);</code><br>
  
  While flushing, **isSkipToDelimiter()** will return true until a delimiter is found or the read times out, either of which terminated the **flush()**.<br>
  
  See [SafeStringReader for Text Input](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html#SafeStringReader) for examples
  
****************************************************************************************/
class SafeStringReader : public SafeString {
  public:
  	  // here buffSize is max size of the token + 1 for delimiter + 1 for terminating '\0;
    explicit SafeStringReader(SafeString& _sfInput, size_t bufSize, char *tokenBuf, const char* _name, const char* delimiters, bool skipToDelimiterFlag = false, uint8_t echoInput = false, unsigned long timeout_ms = 0 );
    explicit SafeStringReader(SafeString& _sfInput, size_t bufSize, char *tokenBuf, const char* _name, const char delimiter, bool skipToDelimiterFlag = false, uint8_t echoInput = false, unsigned long timeout_ms = 0 );

    /**
          connect(Stream& stream)
          specifies the Stream to read chars from
          params
          stream -- the Stream to read from
    */
    void connect(Stream& stream); // clears getReadCount() as well

    /**
      setTimeout
      sets the timeout to wait for more chars.
      If no chars are received for the timeout_ms then a virtual delimiter (-1) is implied and the currently buffered text is returned as a token
      getDelimiter() will return -1 in this case and be used to detect a timeout.

      default is 0, i.e. no timeout set. Only a delimiter will trigger the return of a token
    */
    void setTimeout(unsigned long ms);


    /**
      read()
      returns true if a delimited token has been read from the stream.
      sets this SafeStringReader to that token's text.  The delimiter is not returned as part of the token.
      Use getDelimiter() to check which char delimited this token.
      returnEmptyTokens() controls if empty tokens are returned. Default is to not return empty tokens, i.e. skip multiple consecutive delimiters.
      NOTE: this call always clears the SafeStringReader so no need to call clear() on sfReader at end of processing.
    */
    bool read();

    /**
      getDelimiter()
      returns the delimiter that terminated the last token
      only valid when read() returns true
      will return -1 is there is none, e.g. timed out or argument error
    */
    int getDelimiter();

    /**
      echoOn(), echoOff() control echoing back to the input Stream all chars read
      default if echoOff();
    */
    void echoOn();
    void echoOff();

    /**
      flushInput()
      clears any buffered input and Stream RX buffer then sets skipToDelimiterFlag true
      Once the next delimiter is read or if the timeout is set, the input times out,
      then the SafeStringReader goes back to normal input processing.
      Note: This differs from skipToDelimiter in that is clears any buffered Stream RX first
    */
    void flushInput();

    /**
      returnEmptyTokens
      By default empty tokens are not returned, i.e. multiple consecutive delimiters are skipped
      calling
         returnEmptyTokens() or returnEmptyTokens(true) will return a token for every delimiter found (and every timeout)
         returnEmptyTokens(false) restores the default
    */
    void returnEmptyTokens(bool flag = true);

    /**
        end()
        returns true if have another token, terminates last token if any,
        disconnect from stream, turn echo off, set timeout to 0 and clear skipToDelimiter,
        clears getReadCount()
    */
    bool end();

    /**
      getReadCount()
      The SafeStringReader counts the number of chars read since the last connect( ) call.
      This can be used to terminate reading http response body when the response length is reached.
      end() clears the read count.
    */
    size_t getReadCount();

    /**
      skipToDelimiter()
      discards the next token read
      Once the next delimiter is read or if the timeout is set, the input times out,
      then the SafeStringReader goes back to normal input processing.
    */
    void skipToDelimiter();
    /**
      isSkippingToDelimiter returns true if currently skipping to next delimiter
      */
    bool isSkippingToDelimiter();

    /* Assignment operators **********************************
      Set the SafeString to a char version of the assigned value.
      For = (const char *) the contents are copied to the SafeString buffer
      if the value is null or invalid,
      or too large to be fit in the string's internal buffer
      the string will be left empty
    */
    SafeStringReader & operator = (char c);
    SafeStringReader & operator = (unsigned char c);
    SafeStringReader & operator = (int num);
    SafeStringReader & operator = (unsigned int num);
    SafeStringReader & operator = (long num);
    SafeStringReader & operator = (unsigned long num);
    SafeStringReader & operator = (float num);
    SafeStringReader & operator = (double num);
    SafeStringReader & operator = (SafeString &rhs);
    SafeStringReader & operator = (const char *cstr);
    SafeStringReader & operator = (const __FlashStringHelper *str); // handle F(" .. ") values

    /**
      debugInputBuffer
      These methods let you print out the current contents of the input buffer that the Stream is read into while
      waiting to read a delimiter to terminate the current token.
    */
    const char* debugInputBuffer(bool verbose = true);
    const char* debugInputBuffer(const char* title, bool verbose = true);
    const char* debugInputBuffer(const __FlashStringHelper *title, bool verbose = true);
    const char* debugInputBuffer(SafeString &stitle, bool verbose = true);

  private:
    SafeStringReader(const SafeStringReader& other);
    void init(SafeString& _sfInput, const char* delimiters, bool skipToDelimiterFlag, uint8_t echoInput, unsigned long timeout_ms);
    //  void bufferInput(); // get more input
    SafeString* sfInputPtr;
    const char* delimiters;
    bool skipToDelimiterFlag;
    bool echoInput;
    bool emptyTokensReturned; // default false
    bool flagFlushInput; // true if flushing
    unsigned long timeout_ms;
    bool haveToken; // true if have token but read() not called yet
    Stream *streamPtr;
    size_t charCounter; // counts bytes read, useful for http streams
    char internalCharDelimiter[2]; // used if char delimiter passed
};

#include "SafeStringNameSpaceEnd.h"

#endif  // __cplusplus
#endif // SAFE_STRING_READER_H