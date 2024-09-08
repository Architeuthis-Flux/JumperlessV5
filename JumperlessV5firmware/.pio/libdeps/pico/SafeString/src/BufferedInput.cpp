#include <Arduino.h>
#include "BufferedInput.h"
#include "SafeString.h"  // for Output and #define SSTRING_DEBUG

#include "SafeStringNameSpace.h"

/**
  BufferedInput.cpp
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
*/

/**
        use
        createBufferedInput(name, size);
        instead of calling the constructor
        add a call to
        bufferedInput.nextByteIn();
        at the top of the loop() to read more chars from the input.  You can add more of these calls through out the loop() code if needed.
        Most BufferedInput methods also read more chars from the input
**/

/**
     use createBufferedOutput(name, size, mode); instead
     BufferedInput(size_t _bufferSize, uint8_t *_buf);

     buf -- the user allocated buffer to store the bytes, must be at least bufferSize long.  Defaults to an internal 8 char buffer if buf is omitted or NULL
     bufferSize -- number of bytes to buffer,max bufferSize is limited to 32766. Defaults to an internal 8 char buffer if bufferSize is < 8 or is omitted
*/

BufferedInput::BufferedInput( size_t _bufferSize, uint8_t _buf[]) {
  rb_buf = NULL;
  rb_bufSize = 0; // prevents access to a NULL buf
  rb_clear();
  streamPtr = NULL;
  maxAvail = 0;
  bufUsed = 0;
  if ((_buf == NULL) || (_bufferSize < 8)) {
    // use default
    rb_init(defaultBuffer, sizeof(defaultBuffer));
  } else {
    rb_init(_buf, _bufferSize);
  }  
}


/**
    void connect(Stream& _stream); // the stream to read from, can also write to
        stream -- the stream to buffer input for
*/
void BufferedInput::connect(Stream& _stream) {
  streamPtr = &_stream;
  rb_clear(); //
}

size_t BufferedInput::getSize() {
  return rb_getSize();
}

int BufferedInput::availableForWrite() {
  if (!streamPtr) {
    return 0;
  }
  nextByteIn();
  return 1;
}

// write through to underlying stream
size_t BufferedInput::write(const uint8_t *buffer, size_t size) {
  if (!streamPtr) {
    return 0;
  }
  nextByteIn();
  return streamPtr->write(buffer, size);
}

// write through to underlying stream
size_t BufferedInput::write(uint8_t c) {
  if (!streamPtr) {
    return 0;
  }
  nextByteIn();
  return streamPtr->write(c);
}

int BufferedInput::maxStreamAvailable() {
  size_t rtn = maxAvail;
  maxAvail = 0;
  return rtn;
}
int BufferedInput::maxBufferUsed() {
  size_t rtn = bufUsed;
  bufUsed = 0;
  return rtn;
}

// NOTE nextByteIn will block if baudRate is set higher then actual i/o baudrate
void BufferedInput::nextByteIn() {
  if (!streamPtr) {
    SafeString::Output.println();
    SafeString::Output.println(F("BufferedInput Error: need to call connect(..) first in setup()"));
    SafeString::Output.println();
    SafeString::Output.flush();
    delay(5000);
    return;
  }
  int avail = streamPtr->available();
  if (avail > maxAvail) {
    maxAvail = avail;
  }
  int rb_avail = rb_availableForWrite();
  if (rb_avail < avail) {
    avail = rb_avail;
  }
  for (int i = 0; i < avail; i++) {
    char c = streamPtr->read();
    rb_internalWrite(c); // checked buffer space above rb_availableForWrite() so will not overflow here
    //    rb_write(c);
  }
  if (rb_available() > bufUsed) {
    bufUsed = rb_available();
  }
}

int BufferedInput::available() {
  if (!streamPtr) {
    return 0;
  }
  nextByteIn();
  return rb_available();
}

int BufferedInput::read() {
  if (!streamPtr) {
    return -1; // -1
  }
  nextByteIn();
  return rb_read();
}

int BufferedInput::peek() {
  if (!streamPtr) {
    return -1; // -1
  }
  nextByteIn();
  return rb_peek();
}

// this blocks!!
void BufferedInput::flush() {
  if (!streamPtr) {
    return;
  }
  nextByteIn();
  streamPtr->flush();
}

//===============  ringBuffer methods ==============
// write() will silently fail if ringbuffer is full

/**
   _buf must be at least _size in length
   _size is limited to 32766
   assumes size_t is atleast 16bits as specified by C spec
*/
void BufferedInput::rb_init(uint8_t* _buf, size_t _size) {
  rb_clear();
  if ((_buf == NULL) || (_size == 0)) {
    rb_buf = _buf;
    rb_bufSize = 0; // prevents access to a NULL buf
  } else {
    // available etc returns int, check that _size fits in int
    // limit _size to max int16_t - 1
    if (_size >= 32766) {
      _size = 32766; // (2^16/2)-1 minus 1 since uint16_t vars used
    }
    rb_buf = _buf;
    rb_bufSize = _size; // buffer_count use to detect buffer full
  }
}

void BufferedInput::rb_clear() {
  rb_buffer_head = 0;
  rb_buffer_tail = rb_buffer_head;
  rb_buffer_count = 0;
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino libraries
*/
// defined in BufferedInput.h in BufferedOutputRingBuffer class declaration
//int BufferedInput::available() { return buffer_count; }


size_t BufferedInput::rb_getSize() {
  return rb_bufSize;
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino libraries
*/
// defined in BufferedInput.h in BufferedOutputRingBuffer class declaration
int BufferedInput::rb_availableForWrite() {
  return (rb_bufSize - rb_buffer_count);
}


int BufferedInput::rb_peek() {
  if (rb_buffer_count == 0) {
    return -1;
  } else {
    return rb_buf[rb_buffer_tail];
  }
}


void BufferedInput::rb_dump(Stream * streamPtr) {
  if (streamPtr == NULL) {
    return;
  }
  size_t idx = rb_buffer_tail;
  size_t count = rb_buffer_count;
  while (count > 0) {
    unsigned char c = rb_buf[idx];
    idx = rb_wrapBufferIdx(idx);
    // if (buffer_count > 0) { checked above
    count--;
    //streamPtr->print(" "); streamPtr->print(idx);  streamPtr->print(":");
    streamPtr->print((char)c);
  }
  streamPtr->println("-");
}

int BufferedInput::rb_read() {
  if (rb_buffer_count == 0) {
    return -1;
  } else {
    unsigned char c = rb_buf[rb_buffer_tail];
    rb_buffer_tail = rb_wrapBufferIdx(rb_buffer_tail);
    // if (buffer_count > 0) { checked above
    rb_buffer_count--;
    return c;
  }
}

// char dropped if buffer full
size_t BufferedInput::rb_write(const uint8_t *_buffer, size_t _size) {
  if (_size > ((size_t)rb_availableForWrite())) { // limit to available space extra are dropped
    _size = rb_availableForWrite();
  }
  for (size_t i = 0; i < _size; i++) {
    rb_internalWrite(_buffer[i]);
  }
  return _size;
}

// char dropped if buffer full
size_t BufferedInput::rb_write(uint8_t b) {
  // check for buffer full
  if (rb_buffer_count >= rb_bufSize) {
    return 0;
  }
  // else
  rb_internalWrite(b);
  return 1;
}

void BufferedInput::rb_internalWrite(uint8_t b) {
  // check for buffer full done by caller
  rb_buf[rb_buffer_head] = b;
  rb_buffer_head = rb_wrapBufferIdx(rb_buffer_head);
  rb_buffer_count++;
}

uint16_t BufferedInput::rb_wrapBufferIdx(uint16_t idx) {
  if (idx >= (rb_bufSize - 1)) {
    // wrap around
    idx = 0;
  } else {
    idx++;
  }
  return idx;
}
