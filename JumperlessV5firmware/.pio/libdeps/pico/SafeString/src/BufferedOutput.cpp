#include <Arduino.h>
#include "BufferedOutput.h"

#include "SafeStringNameSpace.h"


/**
  BufferedOutput.cpp
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
*/

// uncomment this next line to output debug to the stream. This output will BLOCK!!
//#define DEBUG
// DEBUG will show the connect( ) settings
// DEBUG will show -1- extra outputs
// If the mode is BLOCK_IF_FULL then -1- will be output each time the output blocks a write, so delaying the loop()

// 2023/01/19 ESP32 V2.0.6 now implements availableForWrite and returns non-zero values
// 2023/01/19 ESP8266 V3.0.2 now implement availableForWrite and returns non-zero values for hardwareSerial
// For ESP32 and ESP8266 serialPtr->availableForWrite() does not seem to return the 'correct' value, i.e. serialPtr->availableForWrite() == 1 still blocks on write()
// so for ALL boards require serialPtr->availableForWrite() > 1 before writing to serialPtr
// MegaTinyCore does not have availableForWrite() in Stream or HardwareSerial

/**
    use
    createBufferedOutput(name, size, mode);
    instead of calling the constructor
    add a call to
    bufferedOutput.nextByteOut();
    at the top of the loop() to release the buffered chars.  You can add more of these calls through out the loop() code if needed
    Most BufferedOutput methods also release the buffered chars
**/

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

BufferedOutput::BufferedOutput( size_t _bufferSize, uint8_t _buf[],  BufferedOutputMode _mode, bool _allOrNothing) {
  rb_buf = NULL;
  rb_bufSize = 0; // prevents access to a NULL buf
  rb_clear();
  serialPtr = NULL;
  streamPtr = NULL; // always non-NULL after connect( )  either set to HardwareSerial OR Stream
  debugOut = NULL;
  txBufferSize = 0;  // if > 0 then serialPtr != NULL, but can have serialPtr != NULL and txBufferSize == 0
  dropMarkWritten = false;
  lastCharWritten = ' ';
  baudRate = 0;
  mode = _mode; // default DROP_IF_FULL if not passed in
  allOrNothingSetting = _allOrNothing;
  allOrNothing = false; // reset after first write(buf,size) // default true if not passed in call.
  waitForEmpty = false; // can write now
  if ((_buf == NULL) || (_bufferSize < 8)) {
    // use default
    rb_init(defaultBuffer, sizeof(defaultBuffer));
  } else {
    rb_init(_buf, _bufferSize);
  }
  us_perByte = 0;
  sendTimerStart = 0;
}

/**
        void connect(HardwareSerial& _serial); // the output to write to, can also read from
            serial -- the HardwareSerial to buffer output to, usually Serial
                     You must call nextByteOut() each loop() in order to release the buffered chars.
*/
//  for HardwareSerial connections serialPtr != NULL else == NULL for Stream connections
//  streamPtr is always != NULL
//  if have availableForWrite >= 2, then txBufferSize != 0
//   else txBufferSize == connection's buffer size
void BufferedOutput::connect(HardwareSerial& _serial) { // the output to write to, can also read from
  serialPtr = &_serial;
  streamPtr = serialPtr;
  debugOut = streamPtr;
  serialPtr->flush(); // try and clear hardware buffer
  delay(10); // wait for a few ms for Tx buffer to clear if flush() does not do it
  int avail = 0;
#if defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF5) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(MEGATINYCORE_MAJOR) 
    while (1) {
      streamPtr->println("This board does not implement availableForWrite()");
      streamPtr->println("You need to use the  bufferedOutput.connect(stream, baudrate) to specify the baudrate");
      streamPtr->println("and add extra calls to nextByteOut() as only one byte is released each call.");
      streamPtr->println();
      streamPtr->flush();
      delay(5000);
    }
#else  // not ARDUINO_SAM_DUE etc
  avail = serialPtr->availableForWrite();
#endif

  if (txBufferSize < avail) {
    txBufferSize = avail;
  }
  baudRate = 0;
  if (txBufferSize <= 2) { // use <= 2 here because below internalStreamAvailableForWrite will subtract 1 from serialPtr->availableForWrite() to allow for bug in ESP32 that blocks when availableForWrite() returns 1
    txBufferSize = 0; // do not use availableForWrite()
    // need baud rate
    while (1) {
      streamPtr->println("availableForWrite() returns 0");
      streamPtr->println("You need to use the  bufferedOutput.connect(stream, baudrate) to specify the baudrate");
      streamPtr->println("and add extra calls to nextByteOut() as only one byte is released each call.");
      streamPtr->println();
      streamPtr->flush();
      delay(5000);
    }
  } // else use avaiableForWrite to throttle I/O
  txBufferSize -= 1; // subtract one to match internalStreamAvailableForWrite()
  us_perByte = 0;
#ifdef DEBUG
  if (debugOut) {
    debugOut->print("BufferedOutput connected to a HardwareSerial  Combined buffer size:"); debugOut->print(getSize()); debugOut->println("");
    debugOut->print(" consisting of BufferedOutput buffer "); debugOut->print(rb_getSize()-4); debugOut->print(" and Serial Tx buffer "); debugOut->println(txBufferSize);
    debugOut->print(" using Serial's availableForWrite to throttle output");
    debugOut->println();
  }
#endif // DEBUG 
  clear();
}

/**
        void connect(Stream& _stream, const uint32_t baudRate);  // the stream to write to and how fast to write output, can also read from
            stream -- the stream to buffer output to
            baudRate -- the maximum rate at which the bytes are to be released.  Bytes will be relased slower depending on how long your loop() method takes to execute
                         You must call nextByteOut() each loop() in order to release the buffered chars.
*/
//  for HardwareSerial connections serialPtr != NULL else == NULL for Stream connections
//  streamPtr is always != NULL
//  if have availableForWrite >= 2, then txBufferSize != 0
//   else txBufferSize == connection's buffer size
void BufferedOutput::connect(Stream& _stream, const uint32_t _baudRate) {
  serialPtr = NULL;
  streamPtr = &_stream;
  debugOut = streamPtr;
  us_perByte = 0;
  streamPtr->flush(); // try and clear hardware buffer
  delay(10); // wait for a few ms for Tx buffer to clear if flush() does not do it
  baudRate = _baudRate;
  if (baudRate == 0) {  // no baudrate  use availableForWrite() if it is available
#if defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF5) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(MEGATINYCORE_MAJOR) 
    while (1) {
      streamPtr->println("Print does not implement availableForWrite()");
      streamPtr->println("You need to specify a non-zero I/O baudRate");
      streamPtr->println("and add extra calls to nextByteOut() as only one byte is released each call.");
      streamPtr->println();
      streamPtr->flush();
      delay(5000);
    }
#else  // not ARDUINO_SAM_DUE etc
    int avail = streamPtr->availableForWrite();
    if (txBufferSize < avail) {
      txBufferSize = avail;
    }
    if (txBufferSize <= 2) { // use <= 2 here because below internalStreamAvailableForWrite will subtract 1 from serialPtr->availableForWrite() to allow for bug in ESP32 that blocks when availableForWrite() returns 1
      txBufferSize = 0; // do not use availableForWrite()
      // need baud rate
      while (1) {
        streamPtr->println();
        streamPtr->println("availableForWrite() returns 0");
        streamPtr->println("You need to specify a non-zero I/O baudRate");
        streamPtr->println("and add extra calls to nextByteOut() as only one byte is released each call.");
        streamPtr->println();
        streamPtr->flush();
        delay(5000);
      }
    } // else use avaiableForWrite to throttle I/O
    txBufferSize -= 1; // subtract one to match internalStreamAvailableForWrite()
#ifdef DEBUG
    if (debugOut) {
      debugOut->print("BufferedOutput connected a Stream.  Combined buffer size:"); debugOut->print(getSize()); debugOut->println("");
      debugOut->print(" consisting of BufferedOutput buffer "); debugOut->print(rb_getSize()-4); debugOut->print(" and Serial Tx buffer "); debugOut->println(txBufferSize);
      debugOut->print(" using Stream's availableForWrite() to throttle output");
      debugOut->println();
    }
#endif // DEBUG 
    clear();
    return;
#endif //  ARDUINO_SAM_DUE
  }

  // else   // have a baudrate
  txBufferSize = 0; // ignore stream buffer
  us_perByte = ((unsigned long)(13000000.0 / (float)baudRate));
  if (us_perByte == 0) {
    while (1) {
      streamPtr->println();
      streamPtr->println("BufferedOutput Error: connect(stream,baudRate) needs a uint32_t baudRate variable < 13000000");
      streamPtr->println();
      streamPtr->flush();
      delay(5000);
    }
  }
  us_perByte += 1; // 1sec / (baud/13) in us  baud is in bits
  // => ~13bits/byte, i.e. start+8+parity+2stop+1  may be less if no parity and only 1 stop bit
#ifdef DEBUG
  if (debugOut) {
    debugOut->print("BufferedOutput connected with "); debugOut->print(getSize()); debugOut->println(" byte buffer.");
    debugOut->print(" BaudRate:");  debugOut->print(baudRate);
    debugOut->print(" Send interval "); debugOut->print(us_perByte); debugOut->print("us");
    debugOut->println();
  }
#endif // DEBUG   
  clear();
  sendTimerStart = micros();
}

// allow 4 for dropMark
size_t BufferedOutput::getSize() {
  return rb_getSize() - 4 + txBufferSize;
}

// prevents current buffer contects from being cleared by clearSpace(), but clear() will still clear the whole buffer
void BufferedOutput::protect() {
  if (rb_lastBufferedByteProtect()) {
    return;
  } else {
    // buffer not empty have something to protect
    // and have not just written protect byte
    rb_write('\0');  // this uses up 1 byte fo the 4 set aside for dropMark
  }
}

// clears space in outgoing (write) buffer, by removing last bytes written, until a protected section reached
//  the Serial Tx buffer is NOT changed
int BufferedOutput::clearSpace(size_t len) {
  waitForEmpty = false;
  allOrNothing = false; // force something next write
  int avail = internalAvailableForWrite(); // already subtracts 4 from rb_buffer
  if (len == 0) {
    return avail; // nothing to do
  }
  if (rb_available() < 8) { // can not allow 8 below in rb_buffer just return now
    return avail;
  }
  // else have at least rb_available() > 8
  len += 8; // should be only 4 for the drop mark but...
  if (((size_t)avail) >= len) {
    return (avail - 8); // have space and avail > 8 because > len+8
  }
  // else len < internalAvailableForWrite() which includes Serial Tx buffer space
  size_t txAvail = internalStreamAvailableForWrite(); // stream available -1 or 0
  if (rb_clearSpace(len - txAvail)) { // allow for space in stream Tx buffer
    dropMarkWritten = false;
    writeDropMark();
  }
  return (internalAvailableForWrite() - 4); // perhaps should be -8??
}

// only clears the BufferedOutput buffer not any HardwareSerial buffer
// clears BufferedOutput buffer even if protected with protect()
void BufferedOutput::clear() {
  bool notEmpty = (rb_available() != 0);
  rb_clear();
  if (notEmpty) {
    dropMarkWritten = false;
    if (!dropMarkWritten) {
      writeDropMark();
    }
  }
  waitForEmpty = false;
  allOrNothing = false; // force something next write
}

// availableForWrite is implemented in HardwareSerial though in  ESP8266
// returns 0 if no availableForWrite() implemented
// else returns stream.availableForWrite()-1 to compensate for ESP32 returning 1 when blocking
//
//  for HardwareSerial connections serialPtr != NULL
//  for Stream connections  streamPtr != NULL
//  if have availableForWrite >= 2, then txBufferSize != 0
//   else txBufferSize == connection's buffer size
int BufferedOutput::internalStreamAvailableForWrite() {
  if (txBufferSize == 0) {
    return 0;
  }
  int avail = 0;
  if (serialPtr) {
  	//  do not have availableForWrite
#if defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF5) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(MEGATINYCORE_MAJOR) 
  	// nothing
#else
    avail = serialPtr->availableForWrite();
#endif
  } else { // streamPtr should always be non-NULL
#if defined(ARDUINO_SAM_DUE) || defined(ARDUINO_ARCH_NRF52) || defined(ARDUINO_ARCH_NRF5) || defined(ARDUINO_ARCH_STM32F1) || defined(ARDUINO_ARCH_STM32F4) || defined(MEGATINYCORE_MAJOR) 
    return 0;    // ARDUINO_SAM_DUE stream does not have availableForWrite
#else
    avail = streamPtr->availableForWrite();
#endif
  }
  //  keep one space in Serial TX, ESP32 blocks if availableForWrite returns 1, needed for ESP8266 which returns 128 max buffer size, but may only be actually 127
  if (avail >= 1) {
    avail -= 1;  // returns 0 if == 1
  }
  return avail;
}

// ignores waitForWrite includes allowance for 4 bytes for ~~\r\n
// but only in rb_buffer
int BufferedOutput::internalAvailableForWrite() {
  if (!streamPtr) {
    return 0;
  }
  int rtn = internalStreamAvailableForWrite();
  int ringAvail = rb_availableForWrite();
  if (ringAvail <= 4) {
    ringAvail = 0;
  } else {
    ringAvail -= 4;
  }
  rtn += ringAvail;
  return rtn;
}

// allow 4 for dropMark and return if waitForEmpty
int BufferedOutput::availableForWrite() {
  if (!streamPtr) {
    return 0;
  }
  nextByteOut(); // try sending first to free some buffer space
  if (waitForEmpty) {
    return 0;
  } // else

  return internalAvailableForWrite();
}

// note should not get \r without \n because if
// \r\n trucated to \r the will have \r~~\r\n due to dropMark
size_t BufferedOutput::terminateLastLine() {
  if (lastCharWritten != '\n') {
    if (internalAvailableForWrite() >= 2) {
      return write((const uint8_t*)"\r\n", 2);
    } else {
      return write('\n'); // may only be one space left
    }
  }
  return 0; // nothing written
}

// NOTE: if DROP_UNTIL_EMPTY and allOrNothing == true,
//      then when buffer, pretend allOrNothing == false so that will get some output
size_t BufferedOutput::write(const uint8_t *buffer, size_t size) {
  if (!streamPtr) {
    return 0;
  }
  nextByteOut(); // sets waitForEmpty false if !DROP_UNTIL_EMPTY
  if (mode == BLOCK_IF_FULL) { // ignores all or nothing
    for (size_t i = 0; i < size; i++) {
      lastCharWritten = buffer[i];
      write(lastCharWritten); // sets dropMarkWritten = false; and calls nextByteOut each time
    }
    return size;
  } // else not BLOCK_IF_FULL

  // else not BLOCK_IF_FULL so either DROP_IF_FULL or DROP_UNTIL_EMPTY
  size_t btbs = bytesToBeSent(); // DOES NOT call nextByteOut
  if (waitForEmpty && btbs) {
    if (!dropMarkWritten) {
      writeDropMark();
    }
    allOrNothing = allOrNothingSetting; // cleared on clear() and makeSpace, reset to input setting
    return 0;
  }
  // check for full writes only
  if ( (btbs != 0) && allOrNothing &&
       // availableForWrite returns 0 if waitForEmpty
       (availableForWrite() < ((int)(size))) ) { // leave 4 for next write attempt drop mark
    if (!dropMarkWritten) {
      writeDropMark();
    }
    waitForEmpty = true;
    return 0;
  } // else  writing a partial at least

  // reduce size to fit
  size_t initSize = size;
  size_t strWriteLen = 0; // nothing written yet
  if (rb_available() == 0) { // nothing in the ringBuffer
    size_t avail = internalStreamAvailableForWrite(); // includes -1
    strWriteLen = size; // try to write it all
    if (avail < strWriteLen) { // only write some of it
      strWriteLen = avail;
    }
    if (strWriteLen > 0) { // write directly to Serial Tx Buffer
      streamPtr->write(buffer, strWriteLen);
      lastCharWritten = buffer[strWriteLen - 1]; // strWriteLen > 0 here
      dropMarkWritten = false;
      buffer += strWriteLen; // update buffer ptr for what has been written
      size -= strWriteLen;  // reduce size left to be written
    }
  }
  size_t rbWriteLen = size; // try to write what is left (may be 0) to ringBuffer
  if (rbWriteLen != 0) {
    if (rb_availableForWrite() < ((int)(rbWriteLen + 4))) { // leave 4 for next write attempt drop mark
      // reduce size to get partial write
      if (rb_availableForWrite() < 4) {
        rbWriteLen = 0;
      } else {
        rbWriteLen = rb_availableForWrite() - 4;
      }
    }
    if (rbWriteLen > 0) {
      dropMarkWritten = false;
      lastCharWritten = buffer[rbWriteLen - 1];
    }
    for (size_t i = 0; i < rbWriteLen; i++) {
      rb_write(buffer[i]);
    }
  } // else all written to Serial Tx buffer and so ringBuffer is empty

  size_t rtnLen = rbWriteLen + strWriteLen;
  if (rtnLen < initSize) { // dropped something
    if (!dropMarkWritten) {
      writeDropMark();
    }
    waitForEmpty = true;
  }
  allOrNothing = allOrNothingSetting; // cleared on clear() and makeSpace, reset to input setting
  return rtnLen;
}

size_t BufferedOutput::write(uint8_t c) {
  if (!streamPtr) {
    return 0;
  }
#ifdef DEBUG
  bool showDelay = true;
#endif // DEBUG    
  nextByteOut(); // sets waitForEmpty false if !DROP_UNTIL_EMPTY
  if (mode != BLOCK_IF_FULL) {
    if ((waitForEmpty) || (rb_availableForWrite() <= 4)) {
      if (!dropMarkWritten) {
        writeDropMark();
      }
      allOrNothing = allOrNothingSetting; // cleared on clear() and makeSpace, reset to input setting
      return 0;
    }
    // else have some ringBuffer space
    if ((rb_available() == 0)) { //(txBufferSize) &&
      if (internalStreamAvailableForWrite()) {
        lastCharWritten = c;
        streamPtr->write(lastCharWritten);
        dropMarkWritten = false;
        allOrNothing = allOrNothingSetting; // cleared on clear() and makeSpace, reset to input setting
        return 1;
      }
    }
    // else no stream Tx buffer space
    if (rb_availableForWrite() > 4) { // leave space for next |\r\n
      dropMarkWritten = false;
      lastCharWritten = c;
      return rb_write(lastCharWritten);
    } else {
      if (!dropMarkWritten) {
        writeDropMark();
      }
      waitForEmpty = true;
      allOrNothing = allOrNothingSetting; // cleared on clear() and makeSpace, reset to input setting
      return 0;
    }

  } else { // may block but no drop marks here
    dropMarkWritten = false; // something will be written!!
    if (rb_availableForWrite() != 0) {
      lastCharWritten = c;
      return rb_write(lastCharWritten);
    } else { // block here
      while (rb_availableForWrite() == 0) {
        // spin
#ifdef DEBUG
        if (showDelay) {
          showDelay = false; // only show this once per write
          if (debugOut) {
            debugOut->print("-1-"); // indicate write( ) is delaying the loop()
          }
        }
#endif // DEBUG    
        delay(1); // wait 1ms, expect this to call yield() for those boards that need it e.g. ESP8266 and ESP32
        nextByteOut(); // try sending first to free some buffer space
      }
      lastCharWritten = c;
      return rb_write(lastCharWritten);
    }
  }
}

// private so no need to check streamPtr here
// updates txBufferSize if txBufferSize not set 0
// nextByteOut(); NOT CALLED HERE don't call this here as may loop
size_t BufferedOutput::bytesToBeSent() {
  size_t btbs = (size_t)rb_available();
  if (txBufferSize) { // using Serial Tx buffer
    int avail = internalStreamAvailableForWrite(); // includes -1
    if (txBufferSize < avail) {
      txBufferSize = avail;
    }
    btbs += (txBufferSize - avail); // now always >= 0
  }
  return btbs;
}

// NOTE nextByteOut will block if baudRate is set higher then actual i/o baudrate
void BufferedOutput::nextByteOut() {
  if (!streamPtr) {
    SafeString::Output.println();
    SafeString::Output.println(F("BufferedOutput Error: need to call connect(..) first in setup()"));
    SafeString::Output.println();
    SafeString::Output.flush();
    delay(5000);
    return;
  }
  if (mode != DROP_UNTIL_EMPTY) {
    waitForEmpty = false; // always skips a lot of the code below
  }

  int serialAvail = internalStreamAvailableForWrite();
  if (rb_available() == 0) { // nothing to send from ringBuffer
    if (txBufferSize != 0) {
      if (serialAvail >= txBufferSize) { // works is txBufferSize == 0 also
        txBufferSize = serialAvail;
        waitForEmpty = false; // both buffers empty
      }
    } else { // no txBuffer
      waitForEmpty = false; // ringBuffer empty
      // no txBuffer so using baudrate to release bytes instead of availableForWrite()
      sendTimerStart = micros(); // restart baudrate release timer
    }
    return; // nothing to release
  }

  // here have something to release
  //  serialAvail set above
  bool serialBytesWritten = false;
  if (txBufferSize != 0) { // common case use internalStreamAvailableForWrite() to throttle output
    // check if space available and fill from ringBuffer  some boards return 0 for availableForWrite
    if (serialAvail > 0) { // have at least 1 space have already adjusted for ESP32 bug in internalStreamAvailableForWrite
      int toWrite = serialAvail;
      int rbAvail = rb_available();
      if (rbAvail < toWrite) { // limit by number of ringBuffer chars
        toWrite = rbAvail;
      }
      serialBytesWritten = (toWrite > 0); //set once here
      for (int i = 0; i < toWrite; i++) {
        uint8_t b = (uint8_t)rb_read();
        if (b) {
          streamPtr->write(b);
        }
        // else  skip protect bytes '\0'
      }
    }
    // here have either filled txBuffer OR emptied rb_buffer
    // if serialBytesWritten then wrote to txBuffer
    if ((!waitForEmpty) || serialBytesWritten || rb_available() ) { // if just wrote somthing or still have something to write then => waitForEmpty unchanged,
      //  also if waitForEmpty false no need to check (wrong mode)
      return; // not empty
    }
    // here waitForEmpty true && noBytesWritten to txBuffer && rb_buffer empty

    // else need to check for waitForEmpty -> false
    // else waitForEmpty && !serialBytesWritten && !rb has bytes (i.e.ringBuffer is empty)
    // using Serial tx buffer
    int avail = internalStreamAvailableForWrite();  // bytesToBeSent() updates txBufferSize
    if (avail >= txBufferSize) { // empty
      waitForEmpty = false;
    }
    return;
  } // else no txBuffer release on timer

  // txBufferSize == 0 so use timer to throttle output
  // sendTimerStart will have been set above

  unsigned long us = micros();
  // micros() has 8us resolution on 8Mhz systems, 4us on 16Mhz system
  // NOTE throw away any excess of (us - sendTimerStart) > us_perByte
  // output will be slower then specified
  if ((us - sendTimerStart) < us_perByte) {
    return; // nothing to do not time to release next byte
  }
  // else send next byte
  sendTimerStart = us; //releasing next byte, restart timer
  uint8_t b = (uint8_t)rb_read();
  if (b) {
    streamPtr->write(b);  // may block if set baudRate higher then actual I/O baud rate
  }
  // else skip protect bytes '\0' This also skips this release baud rate interval
  if (rb_available() == 0) {
    waitForEmpty = false;
  }
}

// always expect there to be at least 4 spaces available in the ringBuffer when this is called
void BufferedOutput::writeDropMark() {
  if (rb_availableForWrite() < 4) {
    rb_write((const uint8_t*)"~~\n", 3); // skip the \r if not enough space in rb_buf due to protect byte
  } else {
    rb_write((const uint8_t*)"~~\r\n", 4); // will truncate if not enough room
  }
  dropMarkWritten = true;
}


int BufferedOutput::available() {
  if (!streamPtr) {
    return 0;
  }
  nextByteOut();
  return streamPtr->available();
}

int BufferedOutput::read() {
  if (!streamPtr) {
    return -1; // -1
  }
  nextByteOut();
  return streamPtr->read();
}

int BufferedOutput::peek() {
  if (!streamPtr) {
    return -1; // -1
  }
  nextByteOut();
  return streamPtr->peek();
}

// this blocks!!
void BufferedOutput::flush() {
  if (!streamPtr) {
    return;
  }
  while (bytesToBeSent() != 0) {
    nextByteOut();
  }
}

//===============  ringBuffer methods ==============
// write() will silently fail if ringbuffer is full

/**
   _buf must be at least _size in length
   _size is limited to 32766
   assumes size_t is atleast 16bits as specified by C spec
*/
void BufferedOutput::rb_init(uint8_t* _buf, size_t _size) {
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

void BufferedOutput::rb_clear() {
  rb_buffer_head = 0;
  rb_buffer_tail = rb_buffer_head;
  rb_buffer_count = 0;
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino libraries
*/
// defined in BufferedOutput.h in BufferedOutputRingBuffer class declaration
//int BufferedOutput::available() { return buffer_count; }


size_t BufferedOutput::rb_getSize() {
  return rb_bufSize;
}

/*
   This should return size_t,
   but someone stuffed it up in the Arduino libraries
*/
// defined in BufferedOutput.h in BufferedOutputRingBuffer class declaration
int BufferedOutput::rb_availableForWrite() {
  return (rb_bufSize - rb_buffer_count);
}


int BufferedOutput::rb_peek() {
  if (rb_buffer_count == 0) {
    return -1;
  } else {
    return rb_buf[rb_buffer_tail];
  }
}


void BufferedOutput::rb_dump(Stream * streamPtr) {
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

int BufferedOutput::rb_read() {
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

size_t BufferedOutput::rb_write(const uint8_t *_buffer, size_t _size) {
  if (_size > ((size_t)rb_availableForWrite())) {
    _size = rb_availableForWrite();
  }
  for (size_t i = 0; i < _size; i++) {
    rb_internalWrite(_buffer[i]);
  }
  return _size;
}

size_t BufferedOutput::rb_write(uint8_t b) {
  // check for buffer full
  if (rb_buffer_count >= rb_bufSize) {
    return 0;
  }
  // else
  rb_internalWrite(b);
  return 1;
}

void BufferedOutput::rb_internalWrite(uint8_t b) {
  // check for buffer full done by caller
  rb_buf[rb_buffer_head] = b;
  rb_buffer_head = rb_wrapBufferIdx(rb_buffer_head);
  rb_buffer_count++;
}

uint16_t BufferedOutput::rb_wrapBufferIdx(uint16_t idx) {
  if (idx >= (rb_bufSize - 1)) {
    // wrap around
    idx = 0;
  } else {
    idx++;
  }
  return idx;
}

// clears space in outgoing (write) buffer, by removing last bytes written,  if len == 0 clear whole buffer, Serial Tx buffer is NOT changed
// returns true if some output dropped
bool BufferedOutput::rb_clearSpace(size_t len) {
  if (len == 0) {
    return false; // nothing dropped
  }
  if (len >= rb_bufSize) {
    rb_clear();
    return true;
  }

  int avail = rb_availableForWrite();
  if (((long)len) <= avail) {
    return false;
  }
  // else avail < len
  size_t tobedropped = len - (size_t)(avail);
  // for (; tobedropped > 0; tobedropped--) {
  //    rb_unWrite(); // this stops unWriting at first '\0'
  //  }
  for (; tobedropped > 0; tobedropped--) {
    if (rb_buffer_count == 0) {
      return true; // empty
    }
    // else
    size_t head = rb_buffer_head - 1; // will wrap to very large number if idx == 0
    if (head > rb_bufSize) { // actually (head > (rb_bufSize-1) but ESP8266 complains about this so just assume head will be very big
      head = (rb_bufSize - 1);
    }
    if (!rb_buf[head]) { // true if == '\0' else false
      return true; // stop at '\0'
    }
    // else update for this unWrite
    rb_buffer_head = head;
    rb_buffer_count--;
  }
  return true;
}

// returns true is last byte still in ringBuffer is '\0' else false
bool BufferedOutput::rb_lastBufferedByteProtect() {
  if (rb_buffer_count == 0) {
    return true; // empty so no need to write another one here as nothing to protect
  }
  // else
  size_t head = rb_buffer_head - 1; // will wrap to very large number if idx == 0
  if (head > rb_bufSize) { // actually (head > (rb_bufSize-1) but ESP8266 complains about this so just assume head will be very big
    head = (rb_bufSize - 1);
  }
  return (!rb_buf[head]);  // true if == '\0' else false
}


