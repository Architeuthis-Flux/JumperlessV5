#ifndef SERIAL_COMS_H
#define SERIAL_COMS_H
/*
  SerialComs.h  a send/receive lines of text between Arduinos via Serial
  by Matthew Ford
  (c)2021 Forward Computing and Control Pty. Ltd.
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.
  This code may be freely used for both private and commercial use.
  Provide this copyright is maintained.
**/
// SafeStringReader.h include defines for Stream
#include "SafeStringReader.h"
#include "millisDelay.h"


// This class uses the SafeString::Output for debug and error messages.
// call SafeString::setOutput(Stream..); to set where the debug and error msgs are to be sent to
//  NOT to the stream pass to the connect( ) :-(

// these define allow you to access the SafeString& like they were class variables
#define textToSend getTextToSend()
#define textReceived getTextReceived()

// handle namespace arduino
#include "SafeStringNameSpaceStart.h"

/*****
  The **SerialComs** class sends/recieves lines of text between Arduinos via Serial, see the detailed description.
<code>  
    #include "SerialComs.h"<br>
    SerialComs coms;  // instantsiate the SerialComs object default is 60 chars sendSize, 60 chars receiveSize<br>
    //// sendSize and receiveSize limits are actual number of message chars that can be handled<br>
    //// excluding the checksum,XON and terminating '\0' which are handled internally<br>
</code>  
  
  in setup() add<br>
  
<code> void setup() {<br>
&nbsp;&nbsp;    Serial.begin(115200);<br>
&nbsp;&nbsp;    
&nbsp;&nbsp;    // comment this line out once everything is running to suppress the SerialComs debug output<br>
&nbsp;&nbsp;    SafeString::setOutput(Serial); // for SafeString error messages and SerialComs debug output<br>
&nbsp;&nbsp;    
&nbsp;&nbsp;    // set one side (and ONLY one side) as the controller.<br> 
&nbsp;&nbsp;    // If one side is using SoftwareSerial it MUST be set as the controller<br>
&nbsp;&nbsp;    coms.setAsController();<br>
&nbsp;&nbsp;    
&nbsp;&nbsp;    // connect the coms to the stream to send/receive from<br>
&nbsp;&nbsp;    // ALWAYS check the return<br>
&nbsp;&nbsp;    if (!coms.connect(Serial1)) {<br>
&nbsp;&nbsp;&nbsp;&nbsp;      while(1) {<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;        Serial.println(F("Out-Of-Memory"));<br>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;        delay(3000);<br>
&nbsp;&nbsp;      }<br>
    }</code><br>
    
  in loop()<br>
  
<code> void loop() {<br>
&nbsp;&nbsp;    // sendAndReceive() MUST be called every loop!!<br>
&nbsp;&nbsp;    coms.sendAndReceive();  // clears coms.textReceived every time before checking for new received message<br>
&nbsp;&nbsp;    // .  . . <br>
&nbsp;&nbsp;    if (!coms.textReceived.isEmpty()) {<br>
&nbsp;&nbsp;&nbsp;&nbsp;      // got a message from the other side process it here<br>
&nbsp;&nbsp;    }<br>
&nbsp;&nbsp;    // .  .  .<br>
&nbsp;&nbsp;    if (coms.textToSend.isEmpty()) {<br>
&nbsp;&nbsp;&nbsp;&nbsp;      // last msg has been sent, can set new message to be sent<br>
&nbsp;&nbsp;&nbsp;&nbsp;      coms.textToSend.print("... ");<br>
&nbsp;&nbsp;&nbsp;&nbsp;      coms.textToSend.print(number); // etc<br>
&nbsp;&nbsp;    }</code><br>
  
    
  coms.textToSend and coms.textReceived are both SafeStrings of fixed capacity.<br>
  coms.textToSend has sendSize capacity and will not accept any more than that many chars<br>
  Once the text is sent the coms.textToSend is cleared.<br>
  
  Any text received, upto the receiveSize appears in coms.textReceived.<br>
  coms.textRecieved is cleared each time sendAndReceive() is called<br>
  
  See [Arduino to Arduino via Serial](https://www.forward.com.au/pfod/ArduinoProgramming/SerialComs/index.html) for an example of using SerialComs
  
***/
class SerialComs : public SafeString {
  public:
    // SerialComs coms();  // creates default coms with send/recieve fo 60char each
    // this default will tolerate very long delays() on either side with out losing data
    // SerialComs coms(120,200); // set sendSize to 120 and receiveSize to 200
    //   on the other side you need SerialComs coms(200,120) to match send <-> receive
    SerialComs(size_t sendSize = 60, size_t receiveSize = 60);

    // One side (and only one side) must be set as the controller
    // SoftwareSerial board should be set as the controller
    void setAsController(); // default no the controller

    // connect sets the Stream to send to/receive from
    // connect returns false if there is not enough memory for the linesizes specified in the constructor
    // you should always check the return.
    bool connect(Stream &io);

    // these three methods are all that you need to use in the loop() code.

    // sendAndReceive() MUST be called every loop.
    // is sends any data if we can send, receives any data if we are waiting for the other side
    // each call to sendAndReceive() first clears the textReceived SafeString, so you must process the received text in the loop() it is received.
    // sendAndReceive() clears the textToSend SafeString once it is sent.
    void sendAndReceive();

    // in the loop() you can use
    // coms.textReceived and coms.textToSend to access the SafeStrings that hold the received text and the text to send.
    // instead of coms.getTextReceived() and coms.getTextToSend()
    SafeString& getTextReceived(); // alias textReceived
    SafeString& getTextToSend();   // alias textToSend

    bool isConnected();

    // default is to add checkSum and check incoming checksum
    // calling noCheckSum() disables both of these.
    // need to do the same on the other side as well.
    void noCheckSum();

    ~SerialComs(); // frees memory
  private:
    char *receive_INPUT_BUFFER;
    char *receiver_TOKEN_BUFFER;
    char *send_BUFFER;
    SafeString* textToSendPtr;
    SafeStringReader* textReceivedPtr;
    SafeString* receiver_SF_INPUT;

    void receiveNextMsg();
    void sendNextMsg();
    void deleteSerialComs();
    void clearIO_Available();
    void checkConnectionTimeout();
    bool checkCheckSum(SafeString& msg);
    void calcCheckSum(SafeString& msg, SafeString& chkHexStr);
    void lostConnection();
    void setConnected();
    void resetConnectionTimer();

    bool connected;
    bool clearToSendFlag;
    bool isController;
    bool notUsingCheckSum;

    millisDelay connectionTimeout;
    bool outOfMemory;
    bool memoryLow;
    size_t _receiveSize;
    size_t _sendSize;
    Stream *stream_io_ptr;
    unsigned long connectionTimeout_ms;// = 250; // 0.25 sec
    static char emptyCharArray[0];
};

#include "SafeStringNameSpaceEnd.h"

#endif // SERIAL_COMS_H
