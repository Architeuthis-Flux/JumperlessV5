// SPDX-License-Identifier: MIT


#include "LEDs.h"
#include "MatrixState.h"
#include "NetsToChipConnections.h"
#include "config.h"
#include "ArduinoStuff.h"
#include "JumperlessDefines.h"
#include "Peripherals.h"

#include "FileParsing.h"
#include "Commands.h"
#include "CH446Q.h"
//#include "FileParsing.h"
Adafruit_USBD_CDC USBSer1;
Adafruit_USBD_CDC USBSer2;

int connectOnBoot1 = 0;
int connectOnBoot2 = 0;
int lockConnection1 = 0;
int lockConnection2 = 0;

int baudRateUSBSer1 = 115200; // for Arduino-Serial
int baudRateUSBSer2 = 115200; // for Routable Serial

volatile int backpowered = 1;

void initArduino(void) // if the UART is set up, the Arduino won't flash from
// it's own USB port
  {

  // Serial1.setRX(17);

  // Serial1.setTX(16);

  // Serial1.begin(115200);

  // delay(1);
  }

void initSecondSerial(void) {
#ifdef USE_TINYUSB

  USBSer1.setStringDescriptor("JL Arduino");  //Not working
  USBSer2.setStringDescriptor("JL Routable"); //Not working

  if (jumperlessConfig.serial_1.function != 0) {
    // pinMode(0, OUTPUT_12MA);
    // pinMode(1, INPUT);
    // pinMode(1, OUTPUT_12MA);
    // digitalWrite(0, HIGH);
    // digitalWrite(1, HIGH);
  //Serial1.setFIFOSize(128);


    //USBSer1.begin(115200, makeSerialConfig(8, 0, 1));


Serial1.setFIFOSize(256);
    USBSer1.begin(baudRateUSBSer1, getSerial1Config());
    Serial1.begin(baudRateUSBSer1, getSerial1Config());
    //Serial1.begin(baudRateUSBSer1, getSerial1Config());
    }
  // }


  if (jumperlessConfig.serial_2.function != 0) {
    USBSer2.begin(baudRateUSBSer2, getSerial2Config());
    Serial2.begin(baudRateUSBSer2, getSerial2Config());
    }

#endif
  }


bool ManualArduinoReset = false;
bool LastArduinoDTR = true;
bool LastRoutableDTR = true;
uint8_t numbitsUSBSer1 = 8;
uint8_t paritytypeUSBSer1 = 0;
uint8_t stopbitsUSBSer1 = 1;
uint8_t numbitsUSBSer2 = 8;
uint8_t paritytypeUSBSer2 = 0;
uint8_t stopbitsUSBSer2 = 1;

unsigned long FirstDTRTime;
bool FirstDTR = true;
bool ESPBoot = false;
unsigned long ESPBootTime;

unsigned long microsPerByteSerial1 = 1000000 / 115200;
unsigned long microsPerByteSerial2 = 1000000 / 115200;


int serConfigChangedUSBSer1 = 0;
int serConfigChangedUSBSer2 = 0;


unsigned long serial1LEDTimer = 0;
unsigned long lastSerial1TxRead = 0;
unsigned long lastSerial1RxRead = 0;
unsigned long lastSerial2TxRead = 0;
unsigned long lastSerial2RxRead = 0;
volatile int arduinoInReset = 0;
unsigned long lastTimeResetArduino = 0;

int printSerial1Passthrough = 0;
int printSerial2Passthrough = 0;

int flashArduinoNextLoop = 0;




int arduinoConnected = 0;

void secondSerialHandler(void) {

  if (jumperlessConfig.serial_1.function != 0 && millis() > 3000) {
    checkForConfigChangesUSBSer1(true);

    }
  if (jumperlessConfig.serial_2.function != 0 && millis() > 3000) {
    checkForConfigChangesUSBSer2(true);
    }


  bool actArduinoDTR = USBSer1.dtr();


  bool actRouteableDTR = 0;//USBSer2.dtr();

  if (millis() - FirstDTRTime >= 5000) FirstDTR = true;
  if (millis() - ESPBootTime >= 5000) ESPBoot = false;

  if (actRouteableDTR != LastRoutableDTR) {
    LastRoutableDTR = actRouteableDTR; //!add some switching logic to allow for a switchable GPIO
    //pinMode(GPIO_2_PIN, OUTPUT);
    //digitalWrite(GPIO_2_PIN, actRouteableDTR);
    //Serial.println("Routeable DTR is true");

      // delay(5);
      // SetArduinoResetLine(HIGH);
      //resetArduino();


    }


  if (ManualArduinoReset) {
    ManualArduinoReset = false;
    SetArduinoResetLine(LOW);
    delay(3);
    SetArduinoResetLine(HIGH);
    }



  if (LastArduinoDTR == 0 && actArduinoDTR == 1) {

    // resetArduino();
    LastArduinoDTR = actArduinoDTR;

    if (millis() > 1500) {

      arduinoConnected = checkIfArduinoIsConnected();
      // Serial.print("Arduino connected: ");
      // Serial.println(arduinoConnected);
      if (arduinoConnected == 0) {
        // flashArduinoNextLoop = 1;
        //connectArduino();
        Serial.println("Arduino not connected (enter A to connect UART)");
        Serial.println("trying to flash anyway\n\r");
        Serial.flush();
        }


      flashArduino(120);


      }
    }

  if ((actArduinoDTR != LastArduinoDTR)) {
    LastArduinoDTR = actArduinoDTR;
    // Serial.println("Arduino DTR is true");
    //     flashArduino(5000);
    if (actArduinoDTR == 0 && FirstDTR) {
      FirstDTRTime = millis();
      FirstDTR = false;
      //resetArduino();

      } else if (actArduinoDTR == 0 && !FirstDTR) {
        // ESPBoot=true;
        // ESPBootTime=millis();
        // ESPReset();
        }
      // if(!ESPBoot){
      //   SetArduinoResetLine(LOW);
      //   delay(1);
      //   SetArduinoResetLine(HIGH);
      // }
    }

  handleSerialPassthrough(2, 0);



  }






void flashArduino(unsigned long timeoutTime) {

  checkForConfigChangesUSBSer1(true);

  char d = 0xdd;
  char c = 0xcc;

  uint8_t usbSer1Buffer[100];
  int usbSer1BufferIndex = 0;

  uint8_t serial1Buffer[100];
  int serial1BufferIndex = 0;

  unsigned long serTimeout = millis();


  while (USBSer1.available() == 0) {
    if (millis() - serTimeout > 1000) {
      // Serial.println("Arduino not connected (enter A to connect UART)");
      // Serial.println("trying to flash anyway\n\r");
      //return;
      break;
      }
    }

  if (USBSer1.peek() == 0x30) {
    resetArduino();
    } else {
    return;
    }

  Serial.println("Flash Arduino started");
  //resetArduino();
  //delay(80);
  lastTimeResetArduino = millis();


  unsigned long flashTimeout = millis();


  timeoutTime = 600;

  int totalBytesTransferred = 0;
  int totalBytesSent = 0;
  int totalBytesReceived = 0;

  while (1) {
    int ret = handleSerialPassthrough(0, 0, printSerial1Passthrough == 2 ? 1 : 0);
    if (ret != 0) {
      totalBytesTransferred += abs(ret);
      if (ret > 0) {
        totalBytesSent += ret;
        } else {
        totalBytesReceived += abs(ret);
        }

      flashTimeout = millis();
      }


    if (millis() - flashTimeout > timeoutTime && totalBytesTransferred > 0) {

      break;
      }

    if (totalBytesTransferred == 0) {
      serTimeout = millis();
      }

    if (millis() - serTimeout > 1200) { //this is a timeout before the arduino wakes up from reset
      break;
      }
    }


  arduinoInReset = 0;
  FirstDTR = true;

  Serial.println("Flash Arduino done");
  Serial.println("\n\r");
  Serial.print("totalBytesTransferred: ");
  Serial.println(totalBytesTransferred);
  Serial.print("totalBytesSent: ");
  Serial.println(totalBytesSent);
  Serial.print("totalBytesReceived: ");
  Serial.println(totalBytesReceived);
  Serial.println();
  Serial.flush();
  }

  int USBSer1Available = 0;
  int Serial1Available = 0;



int handleSerialPassthrough(int serial, int print, int printPassthroughFlashing) {
  int ret = 0;
  int sent = 0;
  int received = 0;

  if (jumperlessConfig.serial_1.function != 0 && (serial == 0 || serial == 2)) {

    unsigned long serial1Timeout = millis();
    USBSer1Available = USBSer1.available();
    Serial1Available = Serial1.available();
    uint8_t serial1Buffer[500];
    int serial1BufferIndex = 0;


    if (USBSer1Available > 0) {

      serial1Timeout = millis();

      while (USBSer1.available() > 0) {
        uint8_t c = USBSer1.read();

        serial1Buffer[serial1BufferIndex++] = c;

        if (serial1BufferIndex >= sizeof(serial1Buffer)) {
          break;
          }
        // if (millis() - serial1Timeout > microsPerByteSerial1 + 5) {
        //   break;
        //   }
       // delayMicroseconds(microsPerByteSerial1 + 5);

        unsigned long delayTime = micros();
        while (micros() - delayTime < microsPerByteSerial1 + 12 && USBSer1.available() == 0) {
          //wait for the next byte or continue if there is one
          }

        if (millis() - serial1Timeout > 400) {
          break;
          }
        }

      ret += serial1BufferIndex;
      sent = serial1BufferIndex;


      for (int i = 0; i < serial1BufferIndex; i++) {
        Serial1.write(serial1Buffer[i]);
        Serial1.flush();
        }
      // Serial.print("USBSer1: ");
      // Serial.println(c, HEX);
      if (print || printSerial1Passthrough == 1 || printPassthroughFlashing == 1) {

        Serial.print("sent     >> ");
        for (int i = 0; i < serial1BufferIndex; i++) {
          Serial.print(serial1Buffer[i], HEX);
          Serial.print(" ");
          }
        Serial.println();
        // Serial.println(serial1BufferIndex);
        // Serial.println();
        Serial.flush();
        }

      gpioReadingColors[8] = 0x1f1900;
      gpioReading[8] = 1;
      lastSerial1TxRead = millis();
      showLEDsCore2 = 2;
      lastTimeResetArduino = millis();
      //Serial.write(c);
    USBSer1Available = USBSer1.available();
    Serial1Available = Serial1.available();
      return sent;

      }

    if (millis() - lastSerial1TxRead > 50) {
      gpioReadingColors[8] = 0x080501;
      gpioReading[8] = 0;
      //showLEDsCore2 = 2;

      }



    if (Serial1Available > 0) {
      serial1Timeout = millis();

      while (Serial1.available() > 0) {
        uint8_t c = Serial1.read();
        serial1Buffer[serial1BufferIndex++] = c;

        if (serial1BufferIndex >= sizeof(serial1Buffer)) {
          break;
          }
        // if (millis() - serial1Timeout > 10) {
        //   break;
        //   }
        unsigned long delayTime = micros();
        while (micros() - delayTime < microsPerByteSerial1 * 2 && Serial1.available() == 0) {
          //wait for the next byte or continue if there is one
          }

          if (millis() - serial1Timeout > 400) {
            break;
            }
        }

      for (int i = 0; i < serial1BufferIndex; i++) {
        USBSer1.write(serial1Buffer[i]);
        USBSer1.flush();
        }

      ret += serial1BufferIndex;
      received = serial1BufferIndex;

      if (print || printSerial1Passthrough == 1 || printPassthroughFlashing == 1) {
        Serial.print("received << ");
        for (int i = 0; i < serial1BufferIndex; i++) {
          Serial.print(serial1Buffer[i], HEX);
          Serial.print(" ");
          }
        Serial.println();
        // Serial.println(serial1BufferIndex);
        // Serial.println();
        Serial.flush();
        }





      gpioReadingColors[9] = 0x00191f;
      gpioReading[9] = 1;
      lastSerial1RxRead = millis();
      showLEDsCore2 = 2;
      lastTimeResetArduino = millis();
      USBSer1Available = USBSer1.available();
      Serial1Available = Serial1.available();

      return 0 - received;
      //  Serial.print(c);
      }
    if (millis() - lastSerial1RxRead > 50) {
      gpioReadingColors[9] = 0x010508;
      gpioReading[9] = 0;
      //showLEDsCore2 = 2;
      }

    //}
    }

  if (jumperlessConfig.serial_2.function != 0 && (serial == 1 || serial == 2)) {

    unsigned long serial2Timeout = millis();
    int usbSer2Available = USBSer2.available();
    int serial2Available = Serial2.available();
    uint8_t serial2Buffer[100];
    int serial2BufferIndex = 0;


    if (usbSer2Available > 0) {

      serial2Timeout = millis();

      while (USBSer2.available() > 0) {
        uint8_t c = USBSer2.read();

        serial2Buffer[serial2BufferIndex++] = c;

        if (serial2BufferIndex >= sizeof(serial2Buffer)) {
          break;
          }
        if (millis() - serial2Timeout > microsPerByteSerial2 + 5) {
          break;
          }
        }

      ret += serial2BufferIndex;
      sent = serial2BufferIndex;
      for (int i = 0; i < serial2BufferIndex; i++) {
        Serial2.write(serial2Buffer[i]);
        Serial2.flush();
        }
      // Serial.print("USBSer1: ");
      // Serial.println(c, HEX);
      if (print || printSerial2Passthrough == 1 || printPassthroughFlashing == 1) {

        Serial.print("USBSer2: ");
        for (int i = 0; i < serial2BufferIndex; i++) {
          Serial.print(serial2Buffer[i], HEX);
          Serial.print(" ");
          }
        Serial.println();
        // Serial.println(serial2BufferIndex);
        // Serial.println();
        Serial.flush();
        }

      gpioReadingColors[8] = 0x1f1900;
      gpioReading[8] = 1;
      lastSerial2TxRead = millis();
      showLEDsCore2 = 2;
      lastTimeResetArduino = millis();
      //Serial.write(c);

      }

    if (millis() - lastSerial2TxRead > 50) {
      gpioReadingColors[8] = 0x080501;
      gpioReading[8] = 0;
      //showLEDsCore2 = 2;

      }

    if (serial2Available > 0) {
      serial2Timeout = millis();

      while (Serial2.available() > 0) {
        uint8_t c = Serial2.read();
        serial2Buffer[serial2BufferIndex++] = c;
        if (serial2BufferIndex >= sizeof(serial2Buffer)) {
          break;
          }
        // if (millis() - serial1Timeout > 10) {
        //   break;
        //   }
        delayMicroseconds(microsPerByteSerial2 + 5);
        }

      for (int i = 0; i < serial2BufferIndex; i++) {
        USBSer2.write(serial2Buffer[i]);
        USBSer2.flush();
        }

      ret += serial2BufferIndex;
      received = serial2BufferIndex;

      if (print || printSerial2Passthrough == 1 || printPassthroughFlashing == 1) {
        Serial.print("Serial2: ");
        for (int i = 0; i < serial2BufferIndex; i++) {
          Serial.print(serial2Buffer[i], HEX);
          Serial.print(" ");
          }
        Serial.println();
        // Serial.println(serial2BufferIndex);
        // Serial.println();
        Serial.flush();
        }

      gpioReadingColors[9] = 0x00191f; //todo fix this
      gpioReading[9] = 1;
      lastSerial2RxRead = millis();
      showLEDsCore2 = 2;
      lastTimeResetArduino = millis();
      //  Serial.print(c);
      }
    if (millis() - lastSerial2RxRead > 50) {
      gpioReadingColors[9] = 0x010508;
      gpioReading[9] = 0;
      //showLEDsCore2 = 2;
      }

    //}
    }
  // if (sent > 0) {
  //   Serial.print("sent: ");
  //   Serial.println(sent);
  //   Serial.println();
  //   }
  // if (received > 0) {
  //   Serial.print("received: ");
  //   Serial.println(received);
  //   Serial.println();
  //   }
  // if (sent > 0 && received > 0) {
  //   Serial.print("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! !!! total bytes transferred: ");
  //   Serial.println(sent + received);
  //   Serial.println();
  //   }

  return ret;
  }
void resetArduino(int topBottomBoth) {
  SetArduinoResetLine(LOW, topBottomBoth);
  delayMicroseconds(500);
  SetArduinoResetLine(HIGH, topBottomBoth);

  }

void printMicrosPerByte(void) {
  Serial.println();
  checkForConfigChangesUSBSer1(2);
  Serial.println("uS per byte    = (1000000 /  baud  + 1)     * (numbits + stopbits + paritybits)");

  Serial.print  ("uS per byte    = (1000000 / ");
  Serial.print(baudRateUSBSer1);
  Serial.print(" + 1 = ");
  Serial.print(1000000 / baudRateUSBSer1 + 1);
  Serial.print(") * (   ");
  Serial.print(numbitsUSBSer1);
  Serial.print("     +    ");
  Serial.print(stopbitsUSBSer1);
  Serial.print("    +     ");
  Serial.print(paritytypeUSBSer1 == 0 ? 0 : 1);
  Serial.print("      = ");
  Serial.print(numbitsUSBSer1 + stopbitsUSBSer1 + (paritytypeUSBSer1 == 0 ? 0 : 1));
  Serial.print(")  =  ");
  Serial.println(microsPerByteSerial1);

  Serial.println();
  Serial.print("microsPerByteSerial1: ");
  Serial.println(microsPerByteSerial1);
  Serial.println();


  // Serial.print("microsPerByteSerial2: ");
  // Serial.println(microsPerByteSerial2);
  }


// void toggleArduinoResetLine(void){
//   pinMode(ARDUINO_RESET_0_PIN, OUTPUT_12MA);
//   pinMode(ARDUINO_RESET_1_PIN, OUTPUT_12MA);
//   digitalWrite(ARDUINO_RESET_0_PIN, HIGH);
//   digitalWrite(ARDUINO_RESET_1_PIN, HIGH);
//   delay(1);
//   digitalWrite(ARDUINO_RESET_0_PIN, LOW);
//   digitalWrite(ARDUINO_RESET_1_PIN, LOW);
// }

void connectArduino(int flashOrLocal) {

  // removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, flashOrLocal);
  // removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, flashOrLocal);
  addBridgeToNodeFile(RP_UART_RX, NANO_D1, netSlot, flashOrLocal, 0);
  addBridgeToNodeFile(RP_UART_TX, NANO_D0, netSlot, flashOrLocal, 0);
  //ManualArduinoReset = true;
  //goto loadfile;
  refresh(flashOrLocal, -1, 1);
  //refreshBlind(0, 0);
  //sendPaths();
   // waitCore2();
  //sendPaths();
  }

void disconnectArduino(int flashOrLocal) {

  removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, flashOrLocal);
  removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, flashOrLocal);
  refresh(flashOrLocal, -1, 1);
  // if (flashOrLocal == 1) {
  //   refreshLocalConnections(1, 0);
  //   } else {
  //   refreshConnections(1, 0);
  //   }
  //refreshBlind(1, 0);
  //sendPaths();
  //waitCore2();
  //sendPaths();

  }

int checkIfArduinoIsConnected(void) {
  int connected = removeBridgeFromNodeFile(NANO_D1, RP_UART_RX, netSlot, 0, 1);
  connected += removeBridgeFromNodeFile(NANO_D0, RP_UART_TX, netSlot, 0, 1);

  if (connected == 2) {
    return 1;
    }
  return 0;
  }

void SetArduinoResetLine(bool state, int topBottomBoth) {
  if (state == LOW) {
    //Serial.println("Setting Arduino Reset Line to LOW");
    if (topBottomBoth == 1 || topBottomBoth == 2) {
      pinMode(ARDUINO_RESET_0_PIN, OUTPUT_12MA);
      digitalWrite(ARDUINO_RESET_0_PIN, LOW);
      rstColors[1] = 0x002a10;
      }
    if (topBottomBoth == 0 || topBottomBoth == 2) {
      pinMode(ARDUINO_RESET_1_PIN, OUTPUT_12MA);
      digitalWrite(ARDUINO_RESET_1_PIN, LOW);
      rstColors[0] = 0x002a10;
      }
    delayMicroseconds(100);




    showLEDsCore2 = 2;

    } else if (state == HIGH) {
      //Serial.println("Setting Arduino Reset Line to HIGH");
        // digitalWrite(ARDUINO_RESET_0_PIN, HIGH);
        // digitalWrite(ARDUINO_RESET_1_PIN, HIGH);

      if (topBottomBoth == 1 || topBottomBoth == 2) {
        pinMode(ARDUINO_RESET_0_PIN, INPUT);
        }
      if (topBottomBoth == 0 || topBottomBoth == 2) {
        pinMode(ARDUINO_RESET_1_PIN, INPUT);
        }
      // headerColors[0] = 0x2000b9;
      // headerColors[1] = 0x0020f9;
      // showLEDsCore2 = 2;
      }
  }

void ESPReset() {
  Serial.println("ESP Boot Mode");
  pinMode(ARDUINO_RESET_0_PIN, OUTPUT);
  pinMode(ARDUINO_RESET_1_PIN, OUTPUT);
  digitalWrite(ARDUINO_RESET_0_PIN, LOW);
  digitalWrite(ARDUINO_RESET_1_PIN, LOW);
  delay(1);
  digitalWrite(ARDUINO_RESET_1_PIN, HIGH);
  delay(2);
  digitalWrite(ARDUINO_RESET_0_PIN, HIGH);
  }
void setBaudRate(int baudRate) { }

void arduinoPrint(void) { }

void uploadArduino(void) { }



uint16_t makeSerialConfig(uint8_t numbits, uint8_t paritytype,
                          uint8_t stopbits) {
  uint16_t config = 0;

  //   #define SERIAL_PARITY_EVEN   (0x1ul)
  // #define SERIAL_PARITY_ODD    (0x2ul)
  // #define SERIAL_PARITY_NONE   (0x3ul)
  // #define SERIAL_PARITY_MARK   (0x4ul)
  // #define SERIAL_PARITY_SPACE  (0x5ul)
  // #define SERIAL_PARITY_MASK   (0xFul)

  // #define SERIAL_STOP_BIT_1    (0x10ul)
  // #define SERIAL_STOP_BIT_1_5  (0x20ul)
  // #define SERIAL_STOP_BIT_2    (0x30ul)
  // #define SERIAL_STOP_BIT_MASK (0xF0ul)

  // #define SERIAL_DATA_5        (0x100ul)
  // #define SERIAL_DATA_6        (0x200ul)
  // #define SERIAL_DATA_7        (0x300ul)
  // #define SERIAL_DATA_8        (0x400ul)
  // #define SERIAL_DATA_MASK     (0xF00ul)
  // #define SERIAL_5N1           (SERIAL_STOP_BIT_1 | SERIAL_PARITY_NONE  |
  // SERIAL_DATA_5)

  unsigned long parity = 0x3ul;
  unsigned long stop = 0x10ul;
  unsigned long data = 0x400ul;

  switch (numbits) {
    case 5:
      data = 0x100ul;
      break;
    case 6:
      data = 0x200ul;
      break;
    case 7:
      data = 0x300ul;
      break;
    case 8:
      data = 0x400ul;
      break;
    default:
      data = 0x400ul;
      break;
    }

  switch (paritytype) {
    case 0:
      parity = 0x3ul;
      break;
    case 2:
      parity = 0x1ul;
      break;
    case 1:
      parity = 0x2ul;
      break;
    case 3:
      parity = 0x3ul;
      break;
    case 4:
      parity = 0x4ul;
      break;
    case 5:
      parity = 0x5ul;
      break;
    default:
      parity = 0x3ul;
      break;
    }

  switch (stopbits) {
    case 1:
      stop = 0x10ul;
      break;
    case 2:
      stop = 0x30ul;
      break;
    default:
      stop = 0x10ul;
      break;
    }

  config = data | parity | stop;

  return config;
  }

uint16_t getSerial1Config(void) {


  uint8_t numbits = USBSer1.numbits();
  uint8_t paritytype = USBSer1.paritytype();
  uint8_t stopbits = USBSer1.stopbits();

  return makeSerialConfig(numbits, paritytype, stopbits);
  }

uint16_t getSerial2Config(void) {

  uint8_t numbits = USBSer2.numbits();
  uint8_t paritytype = USBSer2.paritytype();
  uint8_t stopbits = USBSer2.stopbits();

  return makeSerialConfig(numbits, paritytype, stopbits);
  }

void checkForConfigChangesUSBSer1(int print) {

  if (USBSer1.numbits() != numbitsUSBSer1) {
    numbitsUSBSer1 = USBSer1.numbits();
    serConfigChangedUSBSer1 = 1;
    }

  if (USBSer1.paritytype() != paritytypeUSBSer1) {
    paritytypeUSBSer1 = USBSer1.paritytype();
    serConfigChangedUSBSer1 = 1;
    }

  if (USBSer1.stopbits() != stopbitsUSBSer1) {
    stopbitsUSBSer1 = USBSer1.stopbits();
    serConfigChangedUSBSer1 = 1;
    }


  if (USBSer1.baud() != baudRateUSBSer1) {
    baudRateUSBSer1 = USBSer1.baud();
    //microsPerByteSerial1 = (1000000 / baudRateUSBSer1 + 1) * (numbitsUSBSer1 + stopbitsUSBSer1 + paritytypeUSBSer1==0?0:1);
    // USBSer1.begin(baudRate);
    serConfigChangedUSBSer1 = 1;
    }




  if (serConfigChangedUSBSer1 == 1) {
    USBSer1.begin(baudRateUSBSer1, makeSerialConfig(numbitsUSBSer1, paritytypeUSBSer1, stopbitsUSBSer1));
    Serial1.begin(baudRateUSBSer1, makeSerialConfig(numbitsUSBSer1, paritytypeUSBSer1, stopbitsUSBSer1));
    microsPerByteSerial1 = (1000000 / baudRateUSBSer1 + 1) * (numbitsUSBSer1 + stopbitsUSBSer1 + (paritytypeUSBSer1 == 0 ? 0 : 1));
    serConfigChangedUSBSer1 = 0;

    if (print > 0) {
      if (print == 1) {
        Serial.print("Serial1 config changed to ");
        } else if (print == 2) {
          Serial.print("Serial1 config = ");
          }
        Serial.print(baudRateUSBSer1);
        Serial.print(" ");

        Serial.print(numbitsUSBSer1);
        switch (paritytypeUSBSer1) {
          case 0:
            Serial.print("N");
            break;
          case 1:
            Serial.print("O");
            break;
          case 2:
            Serial.print("E");
            break;
          case 3:
            Serial.print("M");
            break;
          case 4:
            Serial.print("S");
            break;
          default:
            Serial.print("N");
            break;
          }

        Serial.println(stopbitsUSBSer1);
        Serial.flush();
      }


    // delay(1);
    // } else if (serConfigChangedUSBSer1 == 1) {
    //   serConfigChangedUSBSer1 = 2;
    //   delay(1);
    //   } else if (serConfigChangedUSBSer1 == 2) {
    //     serConfigChangedUSBSer1 = 3;
    //     delay(1);
    } else if (print == 2) {
      Serial.print("Serial1 config = ");

      Serial.print(baudRateUSBSer1);
      Serial.print(" ");

      Serial.print(numbitsUSBSer1);
      switch (paritytypeUSBSer1) {
        case 0:
          Serial.print("N");
          break;
        case 1:
          Serial.print("O");
          break;
        case 2:
          Serial.print("E");
          break;
        case 3:
          Serial.print("M");
          break;
        case 4:
          Serial.print("S");
          break;
        default:
          Serial.print("N");
          break;
        }

      Serial.println(stopbitsUSBSer1);
      Serial.flush();
      }
  }

void checkForConfigChangesUSBSer2(int print) {

  if (USBSer2.numbits() != numbitsUSBSer2) {
    numbitsUSBSer2 = USBSer2.numbits();
    serConfigChangedUSBSer2 = 1;
    }

  if (USBSer2.paritytype() != paritytypeUSBSer2) {
    paritytypeUSBSer2 = USBSer2.paritytype();
    serConfigChangedUSBSer2 = 1;
    }

  if (USBSer2.stopbits() != stopbitsUSBSer2) {
    stopbitsUSBSer2 = USBSer2.stopbits();
    serConfigChangedUSBSer2 = 1;
    }


  if (USBSer2.baud() != baudRateUSBSer2) {
    baudRateUSBSer2 = USBSer2.baud();
    //microsPerByteSerial2 = 1000000 / baudRateUSBSer2 + 1;
    // USBSer1.begin(baudRate);
    serConfigChangedUSBSer2 = 1;

    }



  if (serConfigChangedUSBSer2 == 1) {
    USBSer2.begin(baudRateUSBSer2, makeSerialConfig(numbitsUSBSer2, paritytypeUSBSer2, stopbitsUSBSer2));
    Serial2.begin(baudRateUSBSer2, makeSerialConfig(numbitsUSBSer2, paritytypeUSBSer2, stopbitsUSBSer2));
    microsPerByteSerial2 = (1000000 / baudRateUSBSer2 + 1) * (numbitsUSBSer2 + stopbitsUSBSer2 + (paritytypeUSBSer2 == 0 ? 0 : 1));
    serConfigChangedUSBSer2 = 0;

    if (print) {
      Serial.print("Serial2 config changed to ");
      Serial.print(baudRateUSBSer2);
      Serial.print(" ");

      Serial.print(numbitsUSBSer2);
      switch (paritytypeUSBSer2) {
        case 0:
          Serial.print("N");
          break;
        case 1:
          Serial.print("O");
          break;
        case 2:
          Serial.print("E");
          break;
        case 3:
          Serial.print("M");
          break;
        case 4:
          Serial.print("S");
          break;
        default:
          Serial.print("N");
          break;
        }
      Serial.println(stopbitsUSBSer2);
      Serial.flush();
      }
    ///delay(10);
    } else if (serConfigChangedUSBSer2 == 1) {
      serConfigChangedUSBSer2 = 2;
      ///delay(10);
      } else if (serConfigChangedUSBSer2 == 2) {
        serConfigChangedUSBSer2 = 3;
        ///delay(10);
        }
  }