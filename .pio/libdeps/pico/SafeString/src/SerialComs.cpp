#include "SerialComs.h"
/*
  SerialComs.cpp  a send/receive lines of text between Arduinos via Serial
  by Matthew Ford
  (c)2021 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
**/

/**
  Brief overview.  Text upto the specified sendSize is sent by putting it in the SafeString coms.textToSend
  Any text received, upto the receiveSize appears in coms.textReceived.
  Some Implementation details:-
  One side is set as the controller, using coms.setAsController().
  Only one side at a time can send messages. <XON> (0x11) is used to control sending start/stop
  All utf-8 chars can be sent EXCEPT 0x11.  Any <XON> characters in the message are replaced by space 0x20
  The message format is <messageText><MOD256 checksum as 2 hex chars><XON>
  Empty message format is just <XON> 0x11.
  
  <XON> terminates each message and on being received enables the receiving side to send.
  Sending a message implicitly sets disables the Sender from sending another message until a response is received
  
  On startup, the non-controller, is not connected and remains silent and waits to be 'prompted' by the controller.
  On controller startup, the controller prompts with any empty message (just 0x11 <XON>) and the non-controller responds with 
  either its response message OR just <XON> if there is nothing to send
  The controller then responds with its response message or just <XON> and so on.
  
  If no chars are received for 250ms, the connection times out and isConnected() returns false.
  If the isConnected() is false on the controller side, it is waiting for the non-controller to send its response
  to the last message (or <XON>) the controller sent so the controller prompts the non-controller with <XON>
  If isConnected() is false on the non-controller side, it is waiting for the controller prompt it.
  
  Sync of Send/Receive means non-controller must not send until prompted by controller.
  If after timeout non-controller received prompt (msg) and send response (1byte?) just as controller is resending prompt
  the controller receive may fail when using SoftwareSerial. So limit controller prompts to just one byte to reduce chance of overlap and corruption
  
*/


#define DEBUG SafeString::Output

char SerialComs::emptyCharArray[0];

static const char XON = (char)0x11;

SerialComs::SerialComs(size_t sendSize, size_t receiveSize) : SafeString(1, emptyCharArray, "") {
  _receiveSize = receiveSize;
  _sendSize = sendSize;
  connectionTimeout_ms = 5000; // connection timeout
  outOfMemory = false;
  memoryLow = false;
  connected = false;
  clearToSendFlag = false;
  isController = false;
  notUsingCheckSum = false;
  stream_io_ptr = NULL;
  receive_INPUT_BUFFER = NULL;
  receiver_TOKEN_BUFFER = NULL;
  send_BUFFER = NULL;
  textToSendPtr = NULL;
  textReceivedPtr = NULL;
  receiver_SF_INPUT = NULL;
}

// this MUST be called for one side (and only one side)
// If one side is using SoftwareSerial then set that side as the controller
void SerialComs::setAsController() {
  isController = true;
}

// this disable adding/checking checksum, both sides must have the same setting.
// default is to add and check the checksum
void SerialComs::noCheckSum() {
  notUsingCheckSum = true;
}

// alised to 'variable' textReceived
SafeString& SerialComs::getTextReceived() {
  if ((outOfMemory) || (stream_io_ptr == NULL)) {
    return (*this);
  } // else
  return *textReceivedPtr;
}

// alised to 'variable' textToSend
SafeString& SerialComs::getTextToSend() {
  if ((outOfMemory) || (stream_io_ptr == NULL)) {
    return (*this);
  } // else
  return *textToSendPtr;
}

// this MUST be called every loop()
void SerialComs::sendAndReceive() {
  if (outOfMemory) {
    DEBUG.println(F("Out of Memory creating SerialComs"));
    return; // skip
  }
  if (stream_io_ptr == NULL) {
    DEBUG.println(F("Need to call connect(..) first"));
    return; // skip
  }  
  receiveNextMsg(); // always clears textReceivedPtr will fill textReceivedPtr if any non-empty line of data received.
  // if clearToSend then clear RX buffer so may miss prompt and then timeout when checkConnectionTimeout() called
  checkConnectionTimeout(); // this may timeout, if so sets clearToSend to false so next time around will keep prompt and setConnected and clearToSend
  sendNextMsg(); // send textToSendPtr text if connected and can send, else wait for our turn.
  //clears textToSendPtr after it is sent ready for next one  
}

// call this to connect to set the Stream to send to / receive from
bool SerialComs::connect(Stream &io) {
  stream_io_ptr = &io;
  outOfMemory = false;
  memoryLow = false;
  clearToSendFlag = false; // wait for prompt from controller


  // do initial mem check  need ~ 3* 23 + 42 + buffers + 128 for stack safety
  void *initMalloc = malloc(3 * 23 + 42 + 2 * (_receiveSize + 2 + 2) + (_sendSize + 1) + 38 + 128 ); // +38 for names
  if (initMalloc == NULL) {
    deleteSerialComs();
    return false;
  }
  free(initMalloc);

  // allocate buffer on heap
  receive_INPUT_BUFFER = (char*)malloc(_receiveSize + 2 + 2);// 2 for chksum + 2 for \n(delimiter) and '\0'
  if (receive_INPUT_BUFFER == NULL) {
    deleteSerialComs();
    return false;
  }
  receiver_TOKEN_BUFFER = (char*)malloc(_receiveSize + 2 + 2); // 2 for chksum + 2 for \n(delimiter) and '\0'
  if (receiver_TOKEN_BUFFER == NULL) {
    deleteSerialComs();
    return false;
  }

  send_BUFFER = (char*)malloc(_sendSize + 1); // +1 for '\0'
  if (send_BUFFER == NULL) {
    deleteSerialComs();
    return false;
  }

  receiver_SF_INPUT = new SafeString(_receiveSize + 2 + 2, receive_INPUT_BUFFER, "", "receiveBuffer"); // receive_INPUT_BUFFER must be [_receiveSize + 2+2]
  if (receiver_SF_INPUT == NULL) {
    deleteSerialComs();
    return false;
  }

  textReceivedPtr = new SafeStringReader(*receiver_SF_INPUT, _receiveSize + 2 + 2, receiver_TOKEN_BUFFER, "textReceived", XON);
  if (textReceivedPtr == NULL) {
    deleteSerialComs();
    return false;
  }

  textToSendPtr = new SafeString(_sendSize + 1, send_BUFFER, "", "textToSend"); // send_BUFFER must be [_sendSize+1]
  if (textToSendPtr == NULL) {
    deleteSerialComs();
    return false;
  }

  // check free memory after reserve
  // 330 for UNO ,  1910 for MEGA2650 and 2048 for all others
#if defined(ARDUINO_AVR_UNO)
  void *mem = malloc(330);
#elif defined(ARDUINO_AVR_MEGA2560)
  void *mem = malloc(1910); // MEGA2650
#else
  void *mem = malloc(2048); // others 2048
#endif
  if (mem == NULL) {
    memoryLow = true;
    DEBUG.println(F("Warning Low Memory after creating SerialComsPair"));
  } else {
    free(mem);
  }

  if (connectionTimeout_ms > 0) {
    connectionTimeout.start(connectionTimeout_ms);
  }

  // make sure the other side times out
  unsigned long timeout = connectionTimeout_ms/2;
  if (timeout < 1) {
  	  timeout = 1;
  }
  textReceivedPtr->setTimeout(timeout); // set connectionTimeout_ms/2 sec timeout for flushInput and missing XON NOTE: needs to < timeout so prompt from other side does not prevent flush completing
  textReceivedPtr->returnEmptyTokens();
  textReceivedPtr->flushInput();
  textReceivedPtr->connect(*stream_io_ptr); // read from COMS_SERIAL
  DEBUG.println(F(" SerialComs clearing out old data . . ."));
  while(textReceivedPtr->isSkippingToDelimiter()) {
  	  textReceivedPtr->read();
  }
  timeout = connectionTimeout_ms;
  if (timeout > 10) {
  	  timeout = 10;
  }
  if (timeout < 1) {
  	  timeout = 1;
  }
  textReceivedPtr->setTimeout(timeout); // 10ms timeout 10 chars at 9600, 1 char at 960 baud timeout missing XON
  DEBUG.println(F(" SerialComs -  started"));
  return true;
}

void SerialComs::deleteSerialComs() {
  if (outOfMemory) {
    return;// already called
  }
  outOfMemory = true;
  delete [](textToSendPtr);
  delete [](textReceivedPtr);
  delete [] (receiver_SF_INPUT);
  free(send_BUFFER);
  free(receiver_TOKEN_BUFFER);
  free(receive_INPUT_BUFFER);
}

SerialComs::~SerialComs() {
  if (outOfMemory) {
    return;// already called
  }
  deleteSerialComs();
}

bool SerialComs::isConnected() {
  return connected;
}

// ===================================  SerialComsPair support methods ===============================


void SerialComs::receiveNextMsg() {
  textReceivedPtr->clear(); // always
  
	// Note if get connection timeout on non-controller then
	// clearToSendFlag -> false so we do not clear RX buffer after timeout
  if (clearToSendFlag) { // we should be sending so ignore any input
    clearIO_Available();
//    DEBUG.println(F(" clearIO_Available clearToSendFlag."));
    return;
  }
  // do this check second so that if non-controller takes a long time to process textToSendPtr and generate response
  // the above code will first clear all previous XON prompts
  // then this code will timeout and set non-connected and clearToSendFlag == false for next prompt
 // checkConnectionTimeout(); // if CONTROLLER the other side if not connected and this side is the controller

  if (!textReceivedPtr->read()) { // have not got a line of data ALWAYS call this to handle read timeouts
    return;
  }
  if (textReceivedPtr->hasError()) { // previous input length exceeded or read 0
    DEBUG.println(F(" textReceived hasError. Read '\\0' or Input overflowed."));
  }
  if (textReceivedPtr->getDelimiter() == - 1) { // no delimiter so timed out
    DEBUG.println(F("textReceived timeout without receiving terminating XON (0x13)"));
    textReceivedPtr->debug(" ");
    textReceivedPtr->clear(); // skip the invalid line
    return;
  }
  // Only one message allowed at a time so clear any following data
  clearIO_Available();
 //  DEBUG.println(F(" clearIO_Available after read."));
  // only reach here is got msg with delimiter
  
  // for both controller and non-controller,
  bool wasConnected = isConnected();
  setConnected();  // calls resetConnectionTimer,  controller is ALWAYS connected
  if (textReceivedPtr->isEmpty()) {
  	if (isController) {
  		// empty msg
  	} else {
  	  if (!wasConnected) { // was not already connected
  	   DEBUG.println(F("Got prompted by Controller"));
  	  }
  	}
  } else {
   size_t len = textReceivedPtr->length();
   if (len > 2) {
   	   len -=2;
   }
   DEBUG.print(F("Received '")); DEBUG.write((const uint8_t*)textReceivedPtr->c_str(),len); DEBUG.println("'");
  }
  clearToSendFlag = true; // can send more data now
  if (textReceivedPtr->isEmpty()) {
    // just an empty line  valid response
    return;
  } // else non-empty return true to process
  if (!checkCheckSum(*textReceivedPtr)) {   // check check sum
  	textReceivedPtr->clear(); // invalid
    lostConnection(); // sets clearToSendFlag = false;  controller stays in connected state
    return; // do not reply => will timeout
  } // else check sum OK or empty response
  return; // valid textToSendPtr
}

void SerialComs::sendNextMsg() {
//  checkConnectionTimeout();
  if ((clearToSendFlag) && (isConnected() || isController)) { // isController is ALWAYS connected
    clearToSendFlag = false;  // only send one message per textToSendPtr received
    if (!textToSendPtr->isEmpty() && isConnected()) {
      textToSendPtr->replace(XON, ' ');    // replace \n with space
      DEBUG.print(F("Sending '")); DEBUG.print(*textToSendPtr); DEBUG.println("'");
      cSF(ckSum, 2);
      calcCheckSum(*textToSendPtr, ckSum); // calculated checksum returned in SafeString ckSum
      stream_io_ptr->print(*textToSendPtr); stream_io_ptr->print(ckSum);
    }
    stream_io_ptr->print(XON);
    textToSendPtr->clear(); // clear it for next cmd
  }
}

void SerialComs::resetConnectionTimer() {
  if (connectionTimeout_ms > 0) {
    connectionTimeout.restart(); // we are talking
  }
}

void SerialComs::setConnected() {
  resetConnectionTimer();
  if (!isConnected()) {
    DEBUG.println(F(" Made Connection."));
    connected = true;
  }
}

void SerialComs::lostConnection() {
  if (isConnected()) {
    DEBUG.println(F("Connection timed out"));
  }
  if (!isController) {
    clearToSendFlag = false; // wait for prompt from controller
  }
  connected = false;
}

// calcuates the checksum to add to the end of textToSendPtr as 2 hex chars
/*** this is an XOR check sum. use the ADD check sum below
void SerialComs::calcCheckSum(SafeString& msg, SafeString& chkHexStr) {
  chkHexStr.clear();
  if ((msg.isEmpty()) || (!(!notUsingCheckSum))) {
    return;
  }
  uint8_t chksum = 0;
  for (size_t i = 0; i < msg.length(); i++) {
    chksum ^= msg[i];
  }
  if (chksum < 16) {
    chkHexStr += '0';
  }
  chkHexStr.print(chksum, HEX); // add as hex
}
******************/

/** in python
# returns total mod 256 as checksum
# input - string
def checksum256(st):
    return reduce(lambda x,y:x+y, map(ord, st)) % 256
    
    OR
print(hex(sum(str.encode('ascii')) % 256)) #need to strip the 0x    
*/
// add msg length as well??
// calcuates the checksum to add to the end of textToSendPtr as 2 hex chars
// this is an ADD check sum. (MOD256) It is better than the XOR because it will detect repeated byte errors like 0x01,0x01,0x01 
// where as the XOR does not see https://erg.abdn.ac.uk/users/gorry/eg3576/checksums.html
void SerialComs::calcCheckSum(SafeString& msg, SafeString& chkHexStr) {
  chkHexStr.clear();
  if ((msg.isEmpty()) || (!(!notUsingCheckSum))) {
    return;
  }
  int chksum = 0;
  for (size_t i = 0; i < msg.length(); i++) {
    chksum += msg[i];
  }
  chksum = chksum % 256; // keep last 1 byte only
  if (chksum < 16) {
    chkHexStr += '0';
  }
  chkHexStr.print(chksum, HEX); // add as hex char digits
}

// removes checksum 2 hex chars at end of textToSendPtr
// calcuates checksum char for this message and compares them
bool SerialComs::checkCheckSum(SafeString& msg) {
  if (msg.isEmpty()) {
  	  return true; // ok
  }
  if (!(!notUsingCheckSum)) {
    return true; // don't check checksum
  }
  size_t len = msg.length();
  if (len < 3) {
    //DEBUG.print(F(" CheckSum failed for msg '")); DEBUG.print(*textReceivedPtr); DEBUG.println("'");
    DEBUG.print(F(" CheckSum failed -- "));
    DEBUG.print(F(" Need at least 3 chars in msg"));
    DEBUG.println();
    return false; // 2 Hex for checksum + at least one char for msg
    // empty lines don't have checksums
  }
  cSF(msgChksum, 2);
  msg.substring(msgChksum, len - 2, len);
  msg.removeLast(2);
  cSF(chksum, 2);
  calcCheckSum(msg, chksum); // calculate this msg checksum as two hex char
  if (chksum == msgChksum) {
    return true;
  } // else
//  DEBUG.print(F(" CheckSum failed for msg '")); DEBUG.print(*textReceivedPtr); DEBUG.println("'");
  DEBUG.print(F(" CheckSum failed -- "));
  DEBUG.print(F(" CheckSum Hex received '")); DEBUG.print(msgChksum); DEBUG.print("'");
  DEBUG.print(F(" calculated '")); DEBUG.print(chksum); DEBUG.print("'");
  DEBUG.println();
  return false; // check sum failed
}


void SerialComs::checkConnectionTimeout() {
  if (connectionTimeout.justFinished()) {
    connectionTimeout.restart();
    if ((!isConnected()) && (!clearToSendFlag) && isController && (!stream_io_ptr->available())) {
      // controller is waiting for other side to send and the other side has not started sending
      // so prompt it every connectionTimeOut_ms
      // after connectionTimeout, controller times out but skips this section since connected is still true
      // next time around i.e. 2*connectionTimeOut this section send a prompt.
      DEBUG.println(F("Prompt other side to connect"));
      stream_io_ptr->write(XON); // we are still alive
    }
    if (!stream_io_ptr->available()) { // both controller and other side
      lostConnection();
    }
  }
}

void SerialComs::clearIO_Available() {
  while (stream_io_ptr->available()) {
    stream_io_ptr->read(); // clear any following data
  }
}
