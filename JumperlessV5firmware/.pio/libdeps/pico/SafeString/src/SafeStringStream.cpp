/*
  SafeStringStream.h  a Stream wrapper for a SafeString
  by Matthew Ford
  (c)2020 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
**/

#include "SafeStringStream.h"

#include "SafeStringNameSpace.h"

/**  SafeStringStream methods **/
// Use this constructor to set an rxBuffer to use insted of the internal 8 byte buffer
// the sf set here can be replaced in the begin( ) call
SafeStringStream::SafeStringStream(SafeString& sf, SafeString& sfRxBuffer) {
  init();
  sfPtr = &sf;
  sfRxBufferPtr = &sfRxBuffer;
}

SafeStringStream::SafeStringStream(SafeString& sf) {
  init();
  sfPtr = &sf;
}

SafeStringStream::SafeStringStream() {
  init();
}

// private and so never called
SafeStringStream::SafeStringStream(const SafeStringStream& other) {
  (void)(other); // to suppress unused warning
  init();
}

void SafeStringStream::init() {
  sfPtr = NULL;
  baudRate = (uint32_t) - 1; // not started yet
  sfRxBufferPtr = NULL;
  missedCharsCount = 0;
  sendTimerStart = 0;
  us_perByte = 0;
  Rx_BUFFER[0] = '\0';
}


void SafeStringStream::begin(const uint32_t _baudRate) {
  baudRate = _baudRate;
  us_perByte = 0;
  if ((baudRate != 0) && (baudRate != ((uint32_t) - 1)) ) {
    // => ~13bits/byte, i.e. start+8+parity+2stop+1  may be less if no parity and only 1 stop bit
    us_perByte = ((unsigned long)(13000000.0 / (float)baudRate)); // 1sec / (baud/13) in us  baud is in bits
    if (us_perByte == 0) {
	  while(1) {
        SafeString::Output.println();
        SafeString::Output.println(F("SafeStringStream Error: baudRate must be a uint32_t variable < 13000000."));
        SafeString::Output.println();
        SafeString::Output.flush();
        delay(5000);
      }
    }
    us_perByte += 1;	
    sendTimerStart = micros();
  }
}


// this begin replaces any previous sf with the sf passed here.
void SafeStringStream::begin(SafeString &sf, const uint32_t baudRate) {
  // start to release at this baud rate, 0 means infinite baudRate
  sfPtr = &sf;
  begin(baudRate);
}

// number of chars dropped due to SafeStringStream Rx buffer overflow
// count is reset to zero at the end of this call 
size_t SafeStringStream::RxBufferOverflow() {
	size_t rtn = missedCharsCount;
	missedCharsCount = 0;
	return rtn;
}

int SafeStringStream::availableForWrite() {
  if (sfPtr == NULL) {
    return 0;
  }
  int rtn = sfPtr->availableForWrite();
  return rtn;
}

size_t SafeStringStream::write(uint8_t b) {
  if (sfPtr == NULL) {
    return 0;
  }
  unsigned long excessTime = releaseNextByte();
  size_t rtn = sfPtr->write(b);
  sendTimerStart = micros() - excessTime; // allow for this processing time
  return rtn;
}

int SafeStringStream::available() {
  if ((sfPtr == NULL) || (baudRate == ((uint32_t) - 1)) ) {
    SafeString::Output.println();
    SafeString::Output.println(F("SafeStringStream Error: need to call begin(..) first in setup()"));
    SafeString::Output.println();
    SafeString::Output.flush();
    delay(5000);
    return 0;
  }
  if (baudRate == 0) {
    return sfPtr->length();
  } // else
  unsigned long excessTime = releaseNextByte();
  cSFA(sfRxBuffer, Rx_BUFFER);
  SafeString *rxBuf = &sfRxBuffer;
  if (sfRxBufferPtr != NULL) {
    rxBuf = sfRxBufferPtr;
  }
  sendTimerStart = micros() - excessTime; // allow for this processing time

  return rxBuf->length();
}

int SafeStringStream::read() {
  if ((sfPtr == NULL) || (baudRate == ((uint32_t) - 1)) ) {
    SafeString::Output.println();
    SafeString::Output.println(F("SafeStringStream Error: need to call begin(..) first in setup()"));
    SafeString::Output.println();
    SafeString::Output.flush();
    delay(5000);
    return -1;
  }
  if (baudRate == 0) {
    if (sfPtr->isEmpty()) {
      return -1;
    } // else
    unsigned char c = (unsigned char)sfPtr->charAt(0);
    sfPtr->remove(0, 1);
    return c;
  } // else

  unsigned long excessTime = releaseNextByte();
  cSFA(sfRxBuffer, Rx_BUFFER);
  SafeString *rxBuf = &sfRxBuffer;
  if (sfRxBufferPtr != NULL) {
    rxBuf = sfRxBufferPtr;
  }
  if (rxBuf->isEmpty()) {
    sendTimerStart = micros() - excessTime; // allow for this processing time
    return -1;
  } // else
  unsigned char c = (unsigned char)rxBuf->charAt(0);
  rxBuf->remove(0, 1);
  sendTimerStart = micros() - excessTime; // allow for this processing time
  return c;
}

int SafeStringStream::peek() {
  if ((sfPtr == NULL) || (baudRate == ((uint32_t) - 1)) ) {
    return -1;
  }
  if (baudRate == 0) {
    if (sfPtr->isEmpty()) {
      return -1;
    } // else
    return (unsigned char)sfPtr->charAt(0);
  } // else

  unsigned long excessTime = releaseNextByte();
  cSFA(sfRxBuffer, Rx_BUFFER);
  SafeString *rxBuf = &sfRxBuffer;
  if (sfRxBufferPtr != NULL) {
    rxBuf = sfRxBufferPtr;
  }
  if (rxBuf->isEmpty()) {
    sendTimerStart = micros() - excessTime; // allow for this processing time
    return -1;
  } // else
  sendTimerStart = micros() - excessTime; // allow for this processing time
  return  (unsigned char)rxBuf->charAt(0);
}

void SafeStringStream::SafeStringStream::flush() {
  unsigned long excessTime = releaseNextByte();
  sendTimerStart = micros() - excessTime; // allow for this processing time
}

// note built in Rx buffer is only 8 chars
unsigned long SafeStringStream::releaseNextByte() {
  unsigned long us = micros();
  if ((sfPtr == NULL) || (baudRate == ((uint32_t) - 1)) ) {
    SafeString::Output.println();
    SafeString::Output.println(F("SafeStringStream Error: need to call begin(..) first in setup()"));
    SafeString::Output.println();
    SafeString::Output.flush();
    delay(5000);
    return 0;
  }
  if (baudRate == 0) {
    return 0;
  }
  if (sfPtr->length() == 0) {
    return 0; // nothing connected or nothing to do
  }
  // micros() has 8us resolution on 8Mhz systems, 4us on 16Mhz system
  unsigned long excessTime = (us - sendTimerStart);
  unsigned long noOfCharToRelease = excessTime / us_perByte;
  if (noOfCharToRelease > 0) {
    excessTime -= (noOfCharToRelease * us_perByte);
  } else {
    return excessTime; // nothing to do
  }
  // noOfCharToRelease  limit to available chars
  cSFA(sfRxBuffer, Rx_BUFFER);
  SafeString *rxBuf = &sfRxBuffer;
  if (sfRxBufferPtr != NULL) {
    rxBuf = sfRxBufferPtr;
  }
  if (noOfCharToRelease > sfPtr->length()) {
    noOfCharToRelease = sfPtr->length(); // limit to char left in SF
  }
  for (size_t i = 0; i < noOfCharToRelease; i++) {
    if (!rxBuf->availableForWrite()) {
      // make space
      rxBuf->remove(0, 1);
      missedCharsCount++;
    }
    rxBuf->concat(sfPtr->charAt(0));
    sfPtr->remove(0, 1);
  }
  return excessTime;
}



