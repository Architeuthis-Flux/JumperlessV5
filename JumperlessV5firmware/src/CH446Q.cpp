// SPDX-License-Identifier: MIT

#include "CH446Q.h"
#include "JumperlessDefinesRP2040.h"
#include "LEDs.h"
#include "MatrixStateRP2040.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "Graphics.h"

#include "hardware/pio.h"

//#include "pio_spi.h"
#include "spi.pio.h"

#define MYNAMEISERIC                                                           \
  0 // on the board I sent to eric, the data and clock lines are bodged to GPIO
    // 18 and 19. To allow for using hardware SPI

// int chipToPinArray[12] = {CS_A, CS_B, CS_C, CS_D, CS_E, CS_F, CS_G, CS_H,
// CS_I, CS_J, CS_K, CS_L};

PIO pio = pio0;

uint sm = pio_claim_unused_sm(pio, true);

volatile int chipSelect = 0;
volatile uint32_t irq_flags = 0;
unsigned long timeCheck[100];

volatile bool isrFired = false;

void pathHandler(void) {

    

    digitalWrite(RESETPIN, HIGH);
    delayMicroseconds(25);
    digitalWrite(RESETPIN, LOW);
    delayMicroseconds(120);
    //unsigned long howLong = micros();
    cancel_repeating_timer(&timerStruct);
    
    sendAllPaths();
   ///add_repeating_timer_ms(10, ledUpdate, NULL, &timerStruct);

    for(int i = 0; i < numberOfPaths; i++)
    {

        Serial.print("path ");
        Serial.print(i);
        Serial.print(" took ");
        Serial.print(timeCheck[i]);
        Serial.println(" us");
      
    }

    core2busy = false;
    sendAllPathsCore2 = 0;
  


}


void strobeCS(void) {
  // chipSelect = 0;
  //hardware_alarm_cancel(0);
  //delay(10);
  setCSex(chipSelect, 1);
  // // //  Serial.println("interrupt from pio  ");
  // // // Serial.print(chipSelect);
  // // // Serial.print(" \n\r");

  delayMicroseconds(80);

  setCSex(chipSelect, 0);
  delayMicroseconds(80);
  // pio_interrupt_clear(pio, 0);
  // delay(1);
  //  setCSex(0, 1);
  //  setCSex(0, 0);
}

void initCH446Q(void) {

  uint dat = 14;
  uint clk = 15;

  uint cs = 7;

  // irq_add_shared_handler(PIO0_IRQ_0, isrFromPio,0xff);
  // irq_set_exclusive_handler(PIO0_IRQ_0, isrFromPio);
  // irq_set_enabled(PIO0_IRQ_0, true);

  uint offset = pio_add_program(pio, &spi_ch446_multi_cs_program);
  // uint offsetCS = pio_add_program(pio, &spi_ch446_cs_handler_program);

  // Serial.print("offset: ");
  // Serial.println(offset);

  pio_spi_ch446_multi_cs_init(pio, sm, offset, 8, 1, 0, 1, clk, dat);
  // pio_spi_ch446_cs_handler_init(pio, smCS, offsetCS, 256, 1, 8, 20, 6);
  // pinMode(CS_A, OUTPUT);
  // digitalWrite(CS_A, HIGH);

  // pinMode(CS_A, OUTPUT_8MA);
  // pinMode(CS_B, OUTPUT_8MA);
  // pinMode(CS_C, OUTPUT_8MA);
  // pinMode(CS_D, OUTPUT_8MA);
  // pinMode(CS_E, OUTPUT_8MA);
  // pinMode(CS_F, OUTPUT_8MA);
  // pinMode(CS_G, OUTPUT_8MA);
  // pinMode(CS_H, OUTPUT_8MA);
  // pinMode(CS_I, OUTPUT_8MA);
  // pinMode(CS_J, OUTPUT_8MA);
  // pinMode(CS_K, OUTPUT_8MA);
  // pinMode(CS_L, OUTPUT_8MA);

  // digitalWrite(CS_A, LOW);
  // digitalWrite(CS_B, LOW);
  // digitalWrite(CS_C, LOW);
  // digitalWrite(CS_D, LOW);
  // digitalWrite(CS_E, LOW);
  // digitalWrite(CS_F, LOW);
  // digitalWrite(CS_G, LOW);
  // digitalWrite(CS_H, LOW);
  // digitalWrite(CS_I, LOW);
  // digitalWrite(CS_J, LOW);
  // digitalWrite(CS_K, LOW);
  // digitalWrite(CS_L, LOW);

  delay(3);
  /// digitalWrite(RESETPIN, LOW);
}

void resetArduino(void) {
  int lastPath = MAX_BRIDGES - 1;
  path[lastPath].chip[0] = CHIP_I;
  path[lastPath].chip[1] = CHIP_I;
  path[lastPath].x[0] = 11;
  path[lastPath].y[0] = 0;
  path[lastPath].x[1] = 15;
  path[lastPath].y[1] = 0;

  sendPath(lastPath, 1);
  delay(15);
  sendPath(lastPath, 0);
}
void sendAllPaths(void) // should we sort them by chip? for now, no
{
  //   digitalWrite(RESETPIN, HIGH);
  // delay(1);
  // digitalWrite(RESETPIN, LOW);
  //   delay(10);
  // MCPIO.write16(0);
// dontSwirl = true;
//   pio_sm_clear_fifos(pio, sm);
//   pio_sm_restart(pio, sm);
  Serial.print("num paths ");
  Serial.println(numberOfPaths);
  for (int i = 0; i < numberOfPaths; i++) {

    // if (path[i].skip == true)
    // {
    //   continue;
    // }
    timeCheck[i] = micros();
       
    sendPath(i, 1);
    timeCheck[i] = micros() - timeCheck[i];
    // delay(1);
    if (debugNTCC)
    // if(1)
    {
      Serial.print("path ");
      Serial.print(i);
      Serial.print(" \t");
      printPathType(i);
      Serial.print(" \n\r");
      for (int j = 0; j < 4; j++) {
        printChipNumToChar(path[i].chip[j]);
        Serial.print("  x[");
        Serial.print(j);
        Serial.print("]:");
        Serial.print(path[i].x[j]);
        Serial.print("   y[");
        Serial.print(j);
        Serial.print("]:");
        Serial.print(path[i].y[j]);
        Serial.println(" \t ");
      }
      Serial.print("\n\n\r");
    }
    
  }
  dontSwirl = false;  
  
}

void sendPath(int pathToSend, int setOrClear) {

  uint32_t chAddress = 0;

  int chipToConnect = 0;
  int chYdata = 0;
  int chXdata = 0;

  for (int chipp = 0; chipp < 4; chipp++) {
    if (path[pathToSend].chip[pathToSend] >= 0 &&
        path[pathToSend].chip[chipp] < 12) {

      chipSelect = path[pathToSend].chip[chipp];

      chipToConnect = path[pathToSend].chip[chipp];

      if (path[pathToSend].y[chipp] == -1 || path[pathToSend].x[chipp] == -1) {
        if (debugNTCC)
          Serial.print("!");

        continue;
      }

      Serial.print("chip: ");
      Serial.print(chipToConnect);
      Serial.print(" x: ");
      Serial.print(path[pathToSend].x[chipp]);
      Serial.print(" y: ");
      Serial.println(path[pathToSend].y[chipp]);
      // delay(1000);
       
      // pio_interrupt_clear(pio, 0);
      sendXYraw(chipToConnect, path[pathToSend].x[chipp],
                path[pathToSend].y[chipp], setOrClear);
      // Serial.print("sendXYraw >");
        
      // delay(1000);
      //  chYdata = path[i].y[chip];
      //  chXdata = path[i].x[chip];

      // chYdata = chYdata << 5;
      // chYdata = chYdata & 0b11100000;

      // chXdata = chXdata << 1;
      // chXdata = chXdata & 0b00011110;

      // chAddress = chYdata | chXdata;

      // if (setOrClear == 1)
      // {
      //   chAddress = chAddress | 0b00000001; // this last bit determines
      //   whether we set or unset the path
      // }

      // chAddress = chAddress << 24;

      // // delayMicroseconds(50);

      // delayMicroseconds(20);

      // pio_sm_put(pio, sm, chAddress);

      // delayMicroseconds(40);
      // //}
    }
  }
}

void sendXYraw(int chip, int x, int y, int setOrClear) {
  uint32_t chAddress = 0;
  chipSelect = chip;

  int chYdata = y;
  int chXdata = x;

  chYdata = chYdata << 5;
  chYdata = chYdata & 0b11100000;

  chXdata = chXdata << 1;
  chXdata = chXdata & 0b00011110;

  chAddress = chYdata | chXdata;

  if (setOrClear == 1) {
    chAddress =
        chAddress |
        0b00000001; // this last bit determines whether we set or unset the path
  }

  chAddress = chAddress << 24;

  //  delayMicroseconds(100);
  // while (pio_interrupt_get(pio, 0) == true) {
  // }

  //int timeCheck2 = micros();
  pio_sm_put_blocking(pio, sm, chAddress);
  //Serial.print("put >");
  //Serial.println(micros() - timeCheck2);

  // Serial.println(micros() - timeCheck);

  // timeCheck = micros();
   delayMicroseconds(3);
  // isrFired = pio_interrupt_get(pio, 0);
  //   while (pio_interrupt_get(pio,0) == 0) {

  // //   Serial.println("waiting for interrupt");
  // //   delay(100);
  //  }
 // unsigned long timeCheck3 = micros();

  strobeCS();
     pio_interrupt_clear(pio, 0);
  //Serial.print("strobeCS >");
 // Serial.println(micros() - timeCheck3);
//delayMicroseconds(1000);
  //   //
  // while (isrFired == 1) {
  // isrFired = pio_interrupt_get(pio,0);
  // }

  // Serial.println(isrFired);
  // int time = micros() - timeCheck;

  //   Serial.println(time);
  //   Serial.println("^");
  //   delay(100);

  //   }

  // isrFired = false;

  // timeCheck = micros();
  // while (pio_sm_get_tx_fifo_level(pio, sm) > 1) {// for some weird reason, if
  // I just do this in the interrupt callback, it takes like 15 ms to fire
  //   // Serial.println(pio_sm_get_tx_fifo_level(pio, sm));
  // }
  // int time = micros() - timeCheck;
  // delayMicroseconds(2);
  //
  //  strobeCS();
  //  //delayMicroseconds(20);
  //  // delayMicroseconds(500);
  //  irq_flags = pio0_hw->irq;
  //   pio_interrupt_clear(pio, PIO0_IRQ_0);
  //  hw_clear_bits(&pio0_hw->irq, irq_flags);

  // pio_sm_clear_fifos(pio, sm);

  // pio_sm_restart(pio, sm);
  // irq_flags = pio0_hw->irq;
  // Serial.println(time);
  // Serial.println("^");
}
