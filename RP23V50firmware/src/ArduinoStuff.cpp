// SPDX-License-Identifier: MIT

#include "ArduinoStuff.h"
#include "LEDs.h"
#include "MatrixStateRP2040.h"
#include "NetsToChipConnections.h"

Adafruit_USBD_CDC USBSer1;

int baudRate = 115200; // for routable USB-serial

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
  USBSer1.setStringDescriptor("Jumperless USB Serial");

  USBSer1.begin(baudRate, getSerialConfig());

  Serial1.begin(baudRate, getSerialConfig());
#endif
}

int dtrFire = 0;
uint8_t numbits = 8;
uint8_t paritytype = 0;
uint8_t stopbits = 1;

int serConfigChanged = 0;

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
uint16_t getSerialConfig(void) {

  uint8_t numbits = USBSer1.numbits();
  uint8_t paritytype = USBSer1.paritytype();
  uint8_t stopbits = USBSer1.stopbits();

  return makeSerialConfig(numbits, paritytype, stopbits);
}

void checkForConfigChanges(bool print) {

  if (USBSer1.numbits() != numbits) {
    numbits = USBSer1.numbits();
    serConfigChanged = 1;
  }

  if (USBSer1.paritytype() != paritytype) {
    paritytype = USBSer1.paritytype();
    serConfigChanged = 1;
  }

  if (USBSer1.stopbits() != stopbits) {
    stopbits = USBSer1.stopbits();
    serConfigChanged = 1;
  }

  if (USBSer1.baud() != baudRate) {
    baudRate = USBSer1.baud();
    // USBSer1.begin(baudRate);
    serConfigChanged = 1;
  }

  if (serConfigChanged == 3) {
    USBSer1.begin(baudRate, makeSerialConfig(numbits, paritytype, stopbits));
    Serial1.begin(baudRate, makeSerialConfig(numbits, paritytype, stopbits));

    serConfigChanged = 0;

    Serial.print("Serial1 config changed to ");
    Serial.print(baudRate);
    Serial.print(" ");

    Serial.print(numbits);
    switch (paritytype) {
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
    Serial.println(stopbits);
    delay(10);
  } else if (serConfigChanged == 1) {
    serConfigChanged = 2;
    delay(10);
  } else if (serConfigChanged == 2) {
    serConfigChanged = 3;
    delay(10);
  }
}

void secondSerialHandler(void) {

  checkForConfigChanges();

  if ((USBSer1.dtr() == 0 && dtrFire == 0) || dtrFire == 3) {
    // Serial.println("DTR");
    if (dtrFire == 3) {
      dtrFire = 2;
    } else {
      dtrFire = 1;
    }
    
    pinMode(GPIO_2_PIN, OUTPUT);
    digitalWrite(GPIO_2_PIN, LOW);
    pinMode(18, OUTPUT_8MA);
    pinMode(19, OUTPUT_8MA);
    digitalWrite(18, LOW);
    digitalWrite(19, LOW);
    delay(2);
    // Serial1.print('U');

  } else if ((USBSer1.dtr() != 0 && dtrFire == 1) || dtrFire == 2) {
    digitalWrite(GPIO_2_PIN, HIGH);
    digitalWrite(18, HIGH);
    // pinMode(18, INPUT_PULLUP);
    digitalWrite(19, HIGH);
    // pinMode(19, INPUT_PULLUP);
    dtrFire = 0;
  } else {
    pinMode(18, INPUT);
    pinMode(19, INPUT);
    //  digitalWrite(GPIO_2_PIN, LOW);
  }

  if (USBSer1.available()) {
    char c = USBSer1.read();
    Serial1.write(c);
    // Serial1.print(c);
  }
  if (Serial1.available()) {
    char c = Serial1.read();
    USBSer1.write(c);
    //  Serial.print(c);
  }
}

void setBaudRate(int baudRate) {}

void arduinoPrint(void) {}

void uploadArduino(void) {}
