#ifndef BufferedOutput_h
#define BufferedOutput_h
#ifdef __cplusplus

/**
  BufferedOutput.h 
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
*/

/***
  Usage:
     // modes DROP_IF_FULL or DROP_UNTIL_EMPTY or BLOCK_IF_FULL.  BLOCK_IF_FULL will delay loop() when buffer fills up
  createBufferedOutput( output, 64, DROP_IF_FULL); // buffered out called output with buffer size 64 and mode drop chars if buffer full
  OR
  createBufferedOutput( output, 64, DROP_IF_FULL, false);  // for partial output of print(...)

  Then in setup()
  output.connect(Serial,9600); // connect the buffered output to Serial releasing bytes at 9600 baud.

  Then in loop()  use output instead of Serial e.g.
  void loop() {
    // put this line at the top of the loop, must be called each loop to release the buffered bytes
    output.nextByteOut(); // send a byte to Serial if it is time i.e. release at 9600baud
   ...
    output.print(" this is the msg"); // print to output instead of Serial
   ...
    output.read(); // can also read from output, not buffered reads directly from Serial.
   ...
   }
*/

#include <Print.h>
#include <Printable.h>
#include "SafeString.h"  // for Output and #define SSTRING_DEBUG and stream support

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

#define createBufferedOutput(name, size, ...) uint8_t name ## _OUTPUT_BUFFER[(size)+4]; BufferedOutput name(sizeof(name ## _OUTPUT_BUFFER),name ## _OUTPUT_BUFFER,  __VA_ARGS__ ); // add 4 for dropMark

typedef enum {BLOCK_IF_FULL, DROP_UNTIL_EMPTY, DROP_IF_FULL } BufferedOutputMode;
/**************
  To create a BufferedOutput use the macro **createBufferedOutput**  see the detailed description. 
    
  The createBufferedOutput macro takes 2, 3 or 4 arguments.<br>
  
  createBufferedOutput(name, size); creates a BufferedOutput called <i>name</i> which can buffer upto <i>size</i> chars without blocking and then will block once the buffer fills up.<br>
  This default blocking when the buffer is full is not recommended.<br>

  Add a call to <br> 
  <code>bufferedOutput.nextByteOut();</code><br>
  at the top of the loop() to release the buffered chars.  You can add more of these calls through out the loop() code if needed.<br>
  Most BufferedOutput methods also release the buffered chars.<br>

  createBufferedOutput(name, size, mode ); creates a BufferedOutput called <i>name</i> which can buffer upto <i>size</i> chars without blocking and mode determines what to do when the buffer is full.<br>
  <b>mode</b> can be one of **BLOCK_IF_FULL**, **DROP_UNTIL_EMPTY** or **DROP_IF_FULL**<br>
  <b>BLOCK_IF_FULL</b> just blocks until some chars can be sent to the output stream, so freeing up space in the buffer to accept more output. This mode is not recommended, but can be used for testing to force ALL output to be sent.<br>
  <b>DROP_UNTIL_EMPTY</b> will drop further output until all the output currently in the full buffer is sent to the output stream.<br> 
  <b>DROP_IF_FULL</b> will drop further output until enough chars have been sent to the output stream to free up space for the output printed to the buffer.<br>
  
  If any chars are dropped then <b>~~</b> is added to the output sent so you can see where there is missing output.
  
  This 3 argument version uses the default AllOrNothing == true setting.  If the whole print(..) cannot fit in the buffer none of it is sent. 
  
  createBufferedOutput(name, size, mode, allOrNothing ); creates a BufferedOutput called <i>name</i> which can buffer upto <i>size</i> chars without blocking
  and mode determines what to do when the buffer is full and how to handle prints that do not entirely fit in the buffer.<br>
  AllOrNothing true will drop the entire print( ) if it will not completely fit in the buffer.<br>
  AllOrNothing false will only drop the part of the print( ) that will not fit in the buffer.<br>
  
  See [Arduino Serial I/O for the Real World - BufferedOutput](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html#bufferedOutput) for an example of its use.
  
****************************************************************************************/

class BufferedOutput : public Stream {
  public:
    /**
         use createBufferedOutput(name, size, mode); instead
         BufferedOutput(size_t _bufferSize, uint8_t *_buf, BufferedOutputMode = BLOCK_IF_FULL, bool allOrNothing = true);
         
         buf -- the user allocated buffer to store the bytes, must be at least bufferSize long.  Defaults to an internal 8 char buffer if buf is omitted or NULL
         bufferSize -- number of bytes to buffer,max bufferSize is limited to 32766. Defaults to an internal 8 char buffer if bufferSize is < 8 or is omitted
         mode -- BLOCK_IF_FULL (default), DROP_UNTIL_EMPTY or DROP_IF_FULL
                 BLOCK_IF_FULL,    like normal print, but with a buffer. Use this to see ALL the output, but will block the loop() when the output buffer fills
                 DROP_UNTIL_EMPTY, when the output buffer is full, drop any more chars until it completely empties.  ~~<CR><NL> is inserted in the output to show chars were dropped.
                                     Useful when there too much output.  It allow multiple prints to be output consecutively to give meaning full output
                                     avaliableForWrite() will return 0 from when the buffer fills until is empties
                 DROP_IF_FULL,     when the output buffer is full, drop any more chars until here is space.  ~~<CR><NL> is inserted in the output to show chars were dropped.
         allOrNothing -- defaults to true,  If true AND output buffer not empty then if write(buf,size) will not all fit don't output any of it.
                                        Else if false OR output buffer is empty then write(buf,size) will output partial data to fill output buffer.
                         allOrNothing setting is ignored if mode is BLOCK_IF_FULL
    */
    BufferedOutput(size_t _bufferSize, uint8_t *_buf, BufferedOutputMode = BLOCK_IF_FULL, bool allOrNothing = true);

    /**
        void connect(HardwareSerial& _serial); // the output to write to, can also read from
            serial -- the HardwareSerial to buffer output to, usually Serial.
                     You must call nextByteOut() each loop() in order to release the buffered chars. 
    */
    void connect(HardwareSerial& _serial); 
    
    
    /**
        void connect(Stream& _stream, const uint32_t baudRate); // the stream to write to and how fast to write output, can also read from
            stream -- the stream to buffer output to
            baudRate -- the maximum rate at which the bytes are to be released.  Bytes will be relased slower depending on how long your loop() method takes to execute
                         You must call nextByteOut() each loop() in order to release the buffered chars. 
    */
    void connect(Stream& _stream, const uint32_t baudRate=0);
    
    void nextByteOut();
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush(); // this blocks until write buffer empty
    virtual int availableForWrite();
    size_t getSize(); // returns buffer size + any hardwareSerial buffer size found on connect
    int clearSpace(size_t len); // clears space in outgoing (write) buffer, by removing last bytes written,  Serial Tx buffer is NOT changed, returns available space in buffer + Tx
    void protect(); // prevents current buffer contects from being cleared by clearSpace(), but clear() will still clear the whole buffer
    void clear(); // clears outgoing (write) buffer, even if protected.
    size_t terminateLastLine(); // adds a newline if one not already there

  private:
    int internalAvailableForWrite();
    int internalStreamAvailableForWrite(); // returns 0 if no availableForWrite else connection.availableForWrite()-1 to allow for ESP blocking on 1
    void writeDropMark();
    size_t bytesToBeSent(); // bytes in this buffer to be sent, // this ignores any data in the HardwareSerial buffer
    BufferedOutputMode mode; // = 0;
    bool allOrNothing; // = true current setting reset to allOrNothingSetting after each write(buf,size)
    bool allOrNothingSetting; // = true as passed in to constructor
    uint8_t defaultBuffer[8]; // if buffer passed in too small or NULL
    unsigned long us_perByte; // == 1000000 / (baudRate/10) == 10000000 / baudRate
    Stream* streamPtr;
    HardwareSerial* serialPtr; // non-null if HardwareSerial and availableForWrite returns non zero
    uint32_t baudRate;
    unsigned long sendTimerStart;
    bool waitForEmpty;
    Print* debugOut; // only used if #define DEBUG uncomment in BufferedOutput.cpp
    int txBufferSize; // serial tx buffer, if any OR set to zero to only use ringBuffer
    bool dropMarkWritten;
    uint8_t lastCharWritten; // check for \n

    // ringBuffer methods
    /**
       _buf must be at least _size in length
       _size is limited to 32766
    */
    void rb_init(uint8_t* _buf, size_t _size);
    void rb_clear();
    bool rb_clearSpace(size_t len); //returns true if some output dropped, clears space in outgoing (write) buffer, by removing last bytes written
    // from Stream
    inline int rb_available() {
      return rb_buffer_count;
    }
    int rb_peek();
    int rb_read();
    size_t rb_write(uint8_t b); // does not block, drops bytes if buffer full
    size_t rb_write(const uint8_t *buffer, size_t size); // does not block, drops bytes if buffer full
    int rb_availableForWrite(); // {   return (bufSize - buffer_count); }
    size_t rb_getSize(); // size of ring buffer
    bool rb_lastBufferedByteProtect();
    void rb_dump(Stream* streamPtr);

    uint8_t* rb_buf;
    uint16_t rb_bufSize;
    uint16_t rb_buffer_head;
    uint16_t rb_buffer_tail;
    uint16_t rb_buffer_count;
    uint16_t rb_wrapBufferIdx(uint16_t idx);
    void rb_internalWrite(uint8_t b);
};

#include "SafeStringNameSpaceEnd.h"

#endif  // __cplusplus
#endif // BufferedOutput_h
