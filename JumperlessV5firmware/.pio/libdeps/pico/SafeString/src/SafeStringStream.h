#ifndef SAFE_STRING_STREAM_H
#define SAFE_STRING_STREAM_H
/*
  SafeStringStream.h  a Stream wrapper for a SafeString
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
**/
#ifdef __cplusplus
#include <Arduino.h>
#include "SafeString.h"

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

/*****************
  The SafeStringStream class allows you to test your code with data read from a given SafeString as though it was a serial stream with a given baud rate, see the detailed description. 
  The data is released at the baud rate specified in the begin( ) to the RX buffer.<br>
  The default RX buffer size is 8 chars. A SafeString can be specified in the constructor to use as the RX buffer.<br> 
  The data to be read can be written to the SafeStringStream, if the the SafeStringReader has echoON()<br>
  Any char written to the stream is appended to the back of the current data, if there is room in the SafeString the data is being read from.<br>
  
  See [SafeStringStream, Automated Text Input Testing](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html#SafeStringStream) for an example of its use.<br>
  
***************/    
class SafeStringStream : public Stream {
  public:
    /*****************
     SafeStringStream empty constructor, nothing to read yet.
     Sets a default 8 char RX buffer.
    ***************/    
    explicit SafeStringStream(); // nothing to send yet
    /*****************
     SafeStringStream constructor with data to be read from this stream.
     Sets a default 8 char RX buffer.

     @param sf - the SafeString containing the chars to be read from this stream. This sf can be replaced with begin( )<br>
     You can specify any empty SafeString and the later write the data to be read to the SafeStringStream.

    ***************/    
    explicit SafeStringStream(SafeString &sf);

    // Use this constructor to set an rxBuffer to use insted of the internal 8 byte buffer
    // the sf set here can be replaced in the begin( ) call
    /*****************
     SafeStringStream constructor with data to be read from this stream and an RX buffer to be used.
     If the data is not available yet and you just want to increase the RX buffer, you can specify a small data SafeString and replace it later in begin( ) with the real data<br>
     OR you can specify the final data SafeString and fill it later when the data is available.<br>
     The data to be read can be written to the SafeStringStream.

     @param sf - the SafeString containing the chars to be read from this stream. This sf can be replaced with begin( )
     @param sfRxBuffer -- the SafeString to use a the RX buffer for this stream
    ***************/    
    explicit SafeStringStream(SafeString &sf, SafeString &sfRxBuffer);
    
    
    /*****************
     Enable SafeStringStream to start delivering data when read() called.

     @param baudRate - how fast to make the chars available, default 0 means infinitely fast, i.e. all data can be read immediately
    ***************/    
    void begin(const uint32_t baudRate = 0); // start to release at this baud rate, 0 means infinite baudRate //uint32_t 
    /*****************
     Enable SafeStringStream to start delivering data from this SafeString when read() called.

     @param sf - the SafeString containing the chars to be read from this stream. This replaces any data set in the constructor
     @param baudRate - how fast to make the chars available, default 0 means infinitely fast, i.e. all data can be read immediately
    ***************/    
    void begin(SafeString &sf, const uint32_t baudRate = 0); // start to release sf contents at this baud rate, 0 means infinite baudRate
    // this begin replaces any previous sf with the sf passed here.
    
    /*****************
     Write a byte to this stream, the data is appended to the SafeString that is supplying data for this stream's read().
     If the SafeString if full the byte is dropped and an error raised.

     @param b - the byte of data to write
    ***************/    
    size_t write(uint8_t b);
    /*****************
     How many bytes are currently available to be read.
     This is limited by the size of the RX buffer and the baud rate set
    ***************/    
    int available();
    /*****************
     Read the next byte, returns -1 if none available.
    ***************/    
    int read();
    /*****************
     Peek at the next byte, returns -1 if none available.
    ***************/    
    int peek();
    /*****************
     flush any buffered writes, does nothing here except release next byte at current baud rate.
    ***************/    
    void flush(); // for ESP32 etc
    /*****************
     How may bytes can be written to this stream before its underlying SafeString is full.
    ***************/    
    int availableForWrite();
    
    // number of chars dropped due to SafeStringStream Rx buffer overflow
    // count is reset to zero at the end of this call 
    /*****************
     How may bytes have been dropped due to this stream's RX buffer being full.
     The count is reset to zero by this call.
    ***************/    
    size_t RxBufferOverflow();    

  private:
  	SafeStringStream(const SafeStringStream& other);
  	void init();
    unsigned long us_perByte; // == 1000000 / (baudRate/10) == 10000000 / baudRate
    uint32_t baudRate;
    unsigned long releaseNextByte();
    unsigned long sendTimerStart;
    char Rx_BUFFER[9]; // 8char + null
    SafeString* sfRxBufferPtr;
    size_t missedCharsCount;
  protected:
  	 SafeString *sfPtr;
};

#include "SafeStringNameSpaceEnd.h"

#endif  // __cplusplus
#endif  // SAFE_STRING_STREAM_H