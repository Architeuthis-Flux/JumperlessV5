#ifndef BufferedInput_h
#define BufferedInput_h
#ifdef __cplusplus

/**
  BufferedInput.h
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

#include "SafeString.h" // for SSTRING_DEBUG and SafeString::Output and stream support

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

#define createBufferedInput(name, size) uint8_t name ## _INPUT_BUFFER[(size)]; BufferedInput name(sizeof(name ## _INPUT_BUFFER),name ## _INPUT_BUFFER);

/**************
  To create a BufferedInput use the macro **createBufferedInput**  see the detailed description. 
  
  The createBufferedInput macro takes 2 arguments.<br> 
  createBufferedInput(name, bufferSize); creates a BufferedInput called <i>name</i> with a buffer size of <i>bufferSize</i>.<br>
  e.g. to create a BufferedInput called bufferedInput with a buffer size of 128 use<br>
 <code>createBufferedInput(bufferedInput, 128)</code><br>
 
  Add a call to <br>
  <code>bufferedInput.nextByteIn();</code><br>
  at the top of the loop() to read more chars from the input.  You can add more of these calls through out the loop() code if needed.<br>
  Most BufferedInput methods also read more chars from the input<br> 
  
  See [Arduino Serial I/O for the Real World - BufferedInput](https://www.forward.com.au/pfod/ArduinoProgramming/Serial_IO/index.html#BufferedInput) for an example of its use.
***************************************************************************************/
class BufferedInput : public Stream {
  public:
    /**
         use createBufferedOutput(name, size); instead
         BufferedInput(size_t _bufferSize, uint8_t *_buf);

         buf -- the user allocated buffer to store the bytes, must be at least bufferSize long.  Defaults to an internal 8 char buffer if buf is omitted or NULL
         bufferSize -- number of bytes to buffer,max bufferSize is limited to 32766. Defaults to an internal 8 char buffer if bufferSize is < 8 or is omitted
    */
    BufferedInput(size_t _bufferSize, uint8_t *_buf);

    /**
        void connect(Stream& _stream); // the stream to read from, can also write to
            stream -- the stream to buffer input for
    */
    void connect(Stream& _stream);

    void nextByteIn();
    virtual size_t write(uint8_t);
    virtual size_t write(const uint8_t *buf, size_t size);
    virtual int available();
    virtual int read();
    virtual int peek();
    virtual void flush();
    virtual int availableForWrite();
    size_t getSize(); // returns buffer size

    // Counts when number of chars dropped due to full inputBuffer
    // count is reset to zero at the end of this call
    int maxStreamAvailable();
    int maxBufferUsed();

  private:
    Stream* streamPtr;
    int maxAvail;
    int bufUsed;
    uint8_t defaultBuffer[8]; // if buffer passed in too small or NULL

    // ringBuffer methods
    /**
       _buf must be at least _size in length
       _size is limited to 32766
    */
    void rb_init(uint8_t* _buf, size_t _size);
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
    void rb_clear();
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
#endif // BufferedInput_h
