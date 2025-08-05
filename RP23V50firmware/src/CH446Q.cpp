// SPDX-License-Identifier: MIT

#include "CH446Q.h"
#include "JumperlessDefines.h"
#include "LEDs.h"
#include "MatrixState.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "JulseView.h"

#include "hardware/pio.h"

#include "ch446.pio.h"
#include "FileParsing.h"

//#include "SerialWrapper.h"

// #include "pio_spi.h"

#define MYNAMEISERIC                                                           \
  0 // on the board I sent to eric, the data and clock lines are bodged to GPIO
    // 18 and 19. To allow for using hardware SPI

// int chipToPinArray[12] = {CS_A, CS_B, CS_C, CS_D, CS_E, CS_F, CS_G, CS_H,
// CS_I, CS_J, CS_K, CS_L};
PIO pio = pio0;

uint sm = pio_claim_unused_sm(pio, true);

uint offset = 0;

volatile int chipSelect = 0;
volatile uint32_t irq_flags = 0;

// struct justXY {
//   bool connected[16][8]; // 16 X values, 8 Y values, stores whether a connection exists
//   };

struct justXY lastChipXY[12];

void isrFromPio(void) {

  // delayMicroseconds(500);
  setCSex(chipSelect, 1);
  //  Serial.println("interrupt from pio  ");
  // Serial.print(chipSelect);
  // Serial.print(" \n\r");
  //delayMicroseconds(10);

  setCSex(chipSelect, 0);

  // Small delay to ensure signal stability
  delayMicroseconds(10);
  chipSelect = -1;

  // Clear the state machine interrupt (not PIO0_IRQ_0)
  // The PIO program uses "irq 1" and "wait 0 irq 1 rel", so we need to clear interrupt 1 for this state machine
  pio_interrupt_clear(pio, 1);  // Clear interrupt 1 (absolute)
  
  // Also clear the PIO IRQ register for good measure
  irq_flags = pio0_hw->irq;
  hw_clear_bits(&pio0_hw->irq, irq_flags);
  

  }

struct pathStruct lastPath[MAX_BRIDGES];
struct pathStruct emptyPath;


//struct pathStruct newPath[MAX_BRIDGES];
int lastPathNumber = 0;
int changedPaths[MAX_BRIDGES];
int changedPathsCount = 0;

// Index array for chip-ordered processing while keeping main path array in net order
int chipOrderedIndex[MAX_BRIDGES];
bool chipOrderValid = false;

// Timeout counter for PIO debugging
int ch446q_timeout_count = 0;

void initCH446Q(void) {

  uint dat = 14;
  uint clk = 15;

  // uint cs = 7;

  // CRITICAL: Initialize GPIO pins for PIO use (required for RP2350B)
  pio_gpio_init(pio, dat);  // Initialize GPIO 14 for PIO0
  pio_gpio_init(pio, clk);  // Initialize GPIO 15 for PIO0

  irq_add_shared_handler(PIO0_IRQ_1, isrFromPio,
                         PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
  irq_set_enabled(PIO0_IRQ_1, true);

   offset = pio_add_program(pio, &spi_ch446_multi_cs_program);
  // uint offsetCS = pio_add_program(pio, &spi_ch446_cs_handler_program);

  // Serial.print("offset: ");
  // Serial.println(offset);

  pio_spi_ch446_multi_cs_init(pio, sm, offset, 8, 2, 0, 1, clk, dat);

  for (int i = 0; i < 12; i++) {
    pinMode(28 + i, OUTPUT_12MA);
    // digitalWrite(28+i, LOW);
    }
  // pio_spi_ch446_cs_handler_init(pio, smCS, offsetCS, 256, 1, 8, 20, 6);
  // pinMode(CS_A, OUTPUT);
  // digitalWrite(CS_A, HIGH);

  // Initialize lastChipXY array (all connections off)
  for (int chip = 0; chip < 12; chip++) {
    for (int x = 0; x < 16; x++) {
      for (int y = 0; y < 8; y++) {
        lastChipXY[chip].connected[x][y] = false;
        }
      }
    }

  for (int i = 0; i < MAX_BRIDGES; i++) {
    lastPath[i].chip[0] = -1;
    lastPath[i].chip[1] = -1;
    lastPath[i].chip[2] = -1;
    lastPath[i].chip[3] = -1;
    lastPath[i].x[0] = -1;
    lastPath[i].x[1] = -1;
    lastPath[i].x[2] = -1;
    lastPath[i].x[3] = -1;
    lastPath[i].y[0] = -1;
    lastPath[i].y[1] = -1;
    lastPath[i].y[2] = -1;
    lastPath[i].y[3] = -1;
    }
  
  chipOrderValid = false; // Initialize chip order as invalid
  emptyPath.node1 = -1;
  emptyPath.node2 = -1;
  emptyPath.net = 0;
  for (int i = 0; i < 4; i++) {
    emptyPath.chip[i] = 13;
    }
  }

void sendPaths(int clean) {
  // if (sendAllPathsCore2 == 1) {
    // digitalWrite(RESETPIN, HIGH);
    // // refreshPaths();
    // delayMicroseconds(10);
    // digitalWrite(RESETPIN, LOW);

  unsigned long wait_start = micros();
  while (core1busy == true) {
    delayMicroseconds(1);  // Small delay to prevent tight loop
    
    // DEBUG: Warn if waiting too long
    if (micros() - wait_start > 1000000) {  // 1 second timeout
      Serial.println("WARNING: CH446Q waiting for core1busy for more than 1 second!");
      break;  // Break out of the loop to prevent infinite hang
    }
  }
  core2busy = true;

  unsigned long pathTimer = micros();

  // Create chip-ordered index for efficient hardware operations while keeping paths in net order
  createChipOrderedIndex();

  // Re-enable PIO program and set IRQ if needed
  if (!irq_is_enabled(PIO0_IRQ_1)) {
   // Serial.println("CH446Q: Re-enabling PIO0_IRQ_1");
    irq_set_enabled(PIO0_IRQ_1, true);
  }
  
  // CRITICAL: Force re-enable state machine if JulseView disabled it
  if (!(pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm)))) {
   // Serial.println("CH446Q: Re-enabling disabled PIO state machine");
    pio_sm_set_enabled(pio, sm, true);
  }
  
  // CRITICAL: Reset PIO state machine after JulseView operations to clear any interference
  // JulseView's intensive DMA operations can cause timing issues that affect PIO0
  pio_sm_restart(pio, sm);
  pio_sm_clear_fifos(pio, sm);
  pio_interrupt_clear(pio, 1);  // Clear interrupt 1
  delayMicroseconds(100);  // Small delay to ensure PIO is stable
  
  // CRITICAL: Re-enable the correct IRQ1 source using proper SDK function
  // SKIP IRQ reset if JulseView is active to prevent interference with logic analyzer
  if (!julseview_active && pio0_hw->inte1 != 0x100) {
    // Serial.print("CH446Q: Re-setting IRQ1 source - INTE1 was 0x");
    // Serial.print(pio0_hw->inte1, HEX);
    // Serial.println();
    pio_set_irq1_source_enabled(pio, pis_interrupt1, false);  // Clear any wrong sources
    pio_set_irq1_source_enabled(pio, pis_interrupt1, true);   // Set correct source for SM0
  }
  
  // // Check if PIO state machine is claimed and enabled
  // if (!pio_sm_is_claimed(pio, sm)) {
  //   Serial.println("CH446Q: PIO state machine not claimed, re-claiming...");
  //   pio_sm_claim(pio, sm);
  // }
  
  // // Check if state machine is actually enabled by checking the hardware register
  // if (!(pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm)))) {
  //   Serial.println("CH446Q: Re-enabling PIO state machine");
  //   pio_sm_set_enabled(pio, sm, true);
  // }
  
  // CRITICAL: Always force complete PIO reset and reload after JulseView operations
  // JulseView's intensive DMA operations can corrupt PIO instruction memory globally
  // SKIP aggressive PIO reset if JulseView is active to prevent interference
  if (!julseview_active) {
    uint dat = 14;
    uint clk = 15;
    
    // CRITICAL: Force complete PIO reset after JulseView operations
    // JulseView's intensive DMA operations can corrupt PIO instruction memory
    pio_sm_set_enabled(pio, sm, false);
    pio_sm_clear_fifos(pio, sm);
    pio_sm_restart(pio, sm);
    pio_interrupt_clear(pio, 1);
    delayMicroseconds(100);
    
    // Remove and re-add program to ensure clean state
    pio_remove_program(pio, &spi_ch446_multi_cs_program, 0);
    uint offset = pio_add_program(pio, &spi_ch446_multi_cs_program);
    pio_spi_ch446_multi_cs_init(pio, sm, offset, 8, 2, 0, 1, clk, dat);
    pio_sm_set_enabled(pio, sm, true);
    // DISABLED: Interrupt setup is handled in pio_spi_ch446_multi_cs_init() to avoid conflicts
    // pio_set_irq0_source_enabled(pio, (pio_interrupt_source)0, true);
    pio_interrupt_clear(pio, sm);
  }
  
  // // Force complete PIO reset if we've had timeout issues
  // if (ch446q_timeout_count > 0) {
  //   Serial.println("CH446Q: Force reloading PIO program due to previous timeouts...");
  //   uint dat = 14;
  //   uint clk = 15;
    
  //   // // Disable and clear everything
  //   // pio_sm_set_enabled(pio, sm, false);
  //   // pio_interrupt_clear(pio, sm);
  //   // // FIX: Use direct interrupt 0 since PIO program uses "irq 0" (absolute)
  //   // // On RP2350B, pis_interrupt0 = 8, but we need interrupt 0
  //   // // pio_set_irq0_source_enabled(pio, (pio_interrupt_source)0, false);
    
  //   // // Remove and re-add program
  //   // pio_remove_program(pio, &spi_ch446_multi_cs_program, 0);
  //   // uint offset = pio_add_program(pio, &spi_ch446_multi_cs_program);
    
  //   // Reinitialize everything
  //   pio_spi_ch446_multi_cs_init(pio, sm, offset, 8, 2, 0, 1, clk, dat);
  //   pio_sm_set_enabled(pio, sm, true);
  //   // FIX: Use direct interrupt 0 since PIO program uses "irq 0" (absolute)
  //   // On RP2350B, pis_interrupt0 = 8, but we need interrupt 0
  //   // pio_set_irq0_source_enabled(pio, (pio_interrupt_source)0, true);
  //   pio_interrupt_clear(pio, sm);
    
  //   ch446q_timeout_count = 0;  // Reset counter
  // }
  


  if (clean == 1) {
    digitalWrite(RESETPIN, HIGH);
    delayMicroseconds(1000);
    digitalWrite(RESETPIN, LOW);
    }
  sendAllPaths(clean);
  //}
  core2busy = false;
  // core2busy = false;
  unsigned long pathTime = micros() - pathTimer;

  // delayMicroseconds(3200);
  //  Serial.print("pathTime = ");
  //  Serial.println(pathTime);
  sendAllPathsCore2 = 0;
  //printChipStateArray();
  // }
  }



void refreshPaths(void) {
  for (int i = 0; i < MAX_BRIDGES; i++) {
    changedPaths[i] = -2;
    }
  lastPathNumber = 0;
  for (int i = 0; i < MAX_BRIDGES; i++) {
    lastPath[i] = emptyPath;
    }
  chipOrderValid = false; // Invalidate chip order index
  sendAllPaths(1);
  }

void sendAllPaths(int clean) // should we sort them by chip? for now, no
  {
  unsigned long startTime = micros();
  if (clean == 1) {
    // Reset the lastChipXY array on clean start
    for (int chip = 0; chip < 12; chip++) {
      for (int x = 0; x < 16; x++) {
        for (int y = 0; y < 8; y++) {
          lastChipXY[chip].connected[x][y] = false;
          }
        }
      }

    // Send all paths in chip order for hardware efficiency, but preserve net order in path array
    for (int i = 0; i < numberOfPaths; i++) {
      int pathIdx = chipOrderValid ? chipOrderedIndex[i] : i;
      sendPath(pathIdx, 1, 0);
      lastPath[pathIdx] = path[pathIdx];

      // Update lastChipXY
      for (int j = 0; j < 4; j++) {
        if (path[pathIdx].chip[j] != -1 && path[pathIdx].x[j] != -1 && path[pathIdx].y[j] != -1) {
          int chip = path[pathIdx].chip[j];
          int x = path[pathIdx].x[j];
          int y = path[pathIdx].y[j];

          if (chip >= 0 && chip < 12 && x >= 0 && x < 16 && y >= 0 && y < 8) {
            lastChipXY[chip].connected[x][y] = true;
            }
          }
        }
      }
    lastPathNumber = numberOfPaths;
    return;
    } else {
    // Only send changed paths
    findDifferentPaths();
    for (int i = 0; i < numberOfPaths; i++) {
      if (changedPaths[i] == 1) {
        sendPath(i, 1, 0);
        lastPath[i] = path[i];

        if (debugNTCC) {
          Serial.print("changed path ");
          Serial.print(i);
          Serial.print(" c: ");
          Serial.print(path[i].chip[0]);
          Serial.print(" x: ");
          Serial.print(path[i].x[0]);
          Serial.print(" y: ");
          Serial.print(path[i].y[0]);
          Serial.print("   -   c:");
          Serial.print(path[i].chip[1]);
          Serial.print(" x: ");
          Serial.print(path[i].x[1]);
          Serial.print(" y: ");
          Serial.print(path[i].y[1]);
          Serial.println();
          }
        }
      }
    lastPathNumber = numberOfPaths;
    }
  // unsigned long endTime = micros();
  // unsigned long duration = endTime - startTime;
  // Serial.print("Time taken: ");
  // Serial.print(duration);
  // Serial.println(" microseconds");
  }


void printChipStateArray(void) {

  Serial.println("Analog Crossbar Array\n\r");


  // for (int i = 0; i < 12; i++) {
  //   Serial.print("chip ");
  //   Serial.print(i);
  //   Serial.print(" ");
  //   for (int j = 0; j < 16; j++) {
  //     Serial.print(xName(i, j));
  //     Serial.print(" ");
  //     }
  //   Serial.println();
  //   }
  // Serial.println();

  // for (int i = 0; i < 12; i++) {
  //   for (int j = 0; j < 8; j++) {
  //     Serial.print(yName(i, j));
  //     Serial.print(" ");
  //     }
  //   Serial.println();
  //   }
  // Serial.println();
  int showX = 1;
  int showY = 1;



  for (int blockRow = 0; blockRow < 3; blockRow++) {
    int startChip = blockRow * 4;
    int endChip = startChip + 4;
    // Print chip headers
    Serial.print("           ");
    for (int chip = startChip; chip < endChip; chip++) {
      Serial.print("  chip ");
      Serial.print(chipNumToChar(chip));
      Serial.print(" ");
      if (chip < endChip - 1) {
        for (int s = 0; s < 25; s++) Serial.print(" "); // spacing between blocks
        }
      }
    Serial.println("");
    // Print Y headers for each chip block
    if (showY) {
    Serial.print("     ");
    } else {
    Serial.print("     ");
      }
    for (int j = 0; j < 4; j++)
      {
      for (int i = 0; i < 8; i++)
        {
        //Serial.print(" ");
        if (showY)
          {
          Serial.print(i);
          Serial.print("  ");
          } else {
            Serial.print("   ");
            }
        }
        if (showY && j < 3)
          {
          Serial.print("          ");
          } else {
          Serial.print("          ");
            }
      }

    Serial.println();
    // Print each X row for all 4 chips in this block row
    for (int x = 0; x < 16; x++) {
      for (int chip = startChip; chip < endChip; chip++) {
        //Serial.print("x");
        if (showX) {
        Serial.print(" ");
        if (x < 10) Serial.print(" ");
        Serial.print(x);
        } else {
          Serial.print("   ");
          }

        Serial.print(" "); // space between chip blocks

        for (int y = 0; y < 8; y++) {
          int verticalLine = 0;
          int horizontalLine = 0;
          for (int i = 0; i < 16; i++) {
            if (lastChipXY[chip].connected[i][y]) {
              verticalLine = 1;
              }
            for (int j = 0; j < 8; j++) {
              if (lastChipXY[chip].connected[x][j]) {
                horizontalLine = 1;
                }
              }
            }
          if (lastChipXY[chip].connected[x][y] == true) {
            Serial.print("─█─");
            Serial.flush();
            } else {
            if (verticalLine && horizontalLine) {
              Serial.print("─┼─");
              Serial.flush();
              } else if (verticalLine) {
                Serial.print(" │ ");
                Serial.flush();
                } else if (horizontalLine) {
                  Serial.print("───");
                  Serial.flush();

                  // Serial.print(lastChipXY[chip].connected[x][y] ? "█─" : "┼─");
                  } else {
                  Serial.print(" . ");
                  Serial.flush();
                  }
            }
          }

        Serial.print(" "); // space between chip blocks
        Serial.print(xName(chip, x));
        Serial.print("  ");
        }

      Serial.println();
      }
    for (int chip = startChip; chip < endChip; chip++) {
      Serial.print("     ");
      //Serial.print("y");
      for (int y = 0; y < 8; y++) {

        Serial.print(yName(chip, y));
        //Serial.print(" ");
        }
      Serial.print("     "); // spacing between blocks
      }
    Serial.println("\n\n\r"); // extra space between block rows
    }
    Serial.flush();
  }

void printLastChipStateArray(void) {

}

// New function to update the current chip state array based on paths
void updateChipStateArray() {
  // First, clear all connections
  bool newChipXY[12][16][8] = { {{false}} };

  // Set connections based on current paths
  for (int i = 0; i < MAX_NETS; i++) {
    for (int j = 0; j < 4; j++) {
      if (path[i].chip[j] != -1 && path[i].x[j] != -1 && path[i].y[j] != -1) {
        int chip = path[i].chip[j];
        int x = path[i].x[j];
        int y = path[i].y[j];

        if (chip >= 0 && chip < 12 && x >= 0 && x < 16 && y >= 0 && y < 8) {
          newChipXY[chip][x][y] = true;
          }
        }
      }
    }

  // Now we mark the paths that need changing
  for (int i = 0; i < MAX_BRIDGES; i++) {
    changedPaths[i] = -1; // Mark all as unchanged initially
    }

  // Find paths that have changed from last state
  for (int i = 0; i < MAX_NETS; i++) {
    for (int j = 0; j < 4; j++) {
      if (path[i].chip[j] != -1 && path[i].x[j] != -1 && path[i].y[j] != -1) {
        int chip = path[i].chip[j];
        int x = path[i].x[j];
        int y = path[i].y[j];

        if (lastChipXY[chip].connected[x][y] != newChipXY[chip][x][y]) {
          changedPaths[i] = 1; // Mark path as changed
          break;
          }
        }
      }
    }

  // Also find connections that need to be disconnected
  for (int chip = 0; chip < 12; chip++) {
    for (int x = 0; x < 16; x++) {
      for (int y = 0; y < 8; y++) {
        if (lastChipXY[chip].connected[x][y] && !newChipXY[chip][x][y]) {
          // This connection needs to be disconnected
          // Send a disconnect command
          sendXYraw(chip, x, y, 0);
          }
        }
      }
    }

  // Update lastChipXY array
  for (int chip = 0; chip < 12; chip++) {
    for (int x = 0; x < 16; x++) {
      for (int y = 0; y < 8; y++) {
        lastChipXY[chip].connected[x][y] = newChipXY[chip][x][y];
        }
      }
    }
  }

// Updated findDifferentPaths to use the chip state approach
void findDifferentPaths(void) {
  updateChipStateArray();
  }

void sendPath(int i, int setOrClear, int newOrLast) {

  uint32_t chAddress = 0;

  int chipToConnect = 0;
  int chYdata = 0;
  int chXdata = 0;
  if (newOrLast == 1) {
    for (int chip = 0; chip < 4; chip++) {
      if (lastPath[i].chip[chip] != -1) {
        chipSelect = lastPath[i].chip[chip];

        chipToConnect = lastPath[i].chip[chip];

        if (lastPath[i].y[chip] == -1 || lastPath[i].x[chip] == -1) {
          if (debugNTCC)
            Serial.print("!");

          continue;
          }

        sendXYraw(chipToConnect, lastPath[i].x[chip], lastPath[i].y[chip], 0);
        }
      }
    } else {

    for (int chip = 0; chip < 4; chip++) {
      if (path[i].chip[chip] != -1) {
        chipSelect = path[i].chip[chip];

        chipToConnect = path[i].chip[chip];

        if (path[i].y[chip] == -1 || path[i].x[chip] == -1) {
          if (debugNTCC)
            Serial.print("!");

          continue;
          }

        sendXYraw(chipToConnect, path[i].x[chip], path[i].y[chip], setOrClear);
        }
      }
    }
  }

void sendXYraw(int chip, int x, int y, int setOrClear) {
  uint32_t chAddress = 0;
  chipSelect = chip;

  // Serial.print("sendXYraw: chip = ");
  // Serial.print(chip);
  // Serial.print(", x = ");
  // Serial.print(x);
  // Serial.print(", y = ");
  // Serial.print(y);
  // Serial.print(", setOrClear = ");
  // Serial.println(setOrClear);

  //unsigned long start = micros();

  int chYdata = y;
  int chXdata = x;

  chYdata = chYdata << 5;
  chYdata = chYdata & 0b11100000;

  chXdata = chXdata << 1;
  chXdata = chXdata & 0b00011110;

  chAddress = chYdata | chXdata;

  if (setOrClear == 1) {
    chAddress = chAddress | 0b00000001; // this last bit determines whether we set or unset the path
    }

  chAddress = chAddress << 24;

  //delayMicroseconds(10);
  // Serial.print("fifo: ");
  // Serial.println(pio_sm_get_tx_fifo_level(pio, sm));

  pio_sm_put(pio, sm, chAddress);


  // while (pio_interrupt_get(pio, PIO0_IRQ_0) == 0){
  //   delayMicroseconds(1);
  //   // Serial.print(".");
  //   // Serial.flush();
  //   }



  // while (pio_interrupt_get(pio, PIO0_IRQ_0)){
  //   tight_loop_contents();
  //   Serial.print("x");
  //   Serial.flush();
  //   }

  unsigned long wait_start = micros();
  while (chipSelect != -1) {
    //delayMicroseconds(1);
    tight_loop_contents();
    
    // DEBUG: Warn if waiting too long
    if (micros() - wait_start > 1000000) {  // 1 second timeout
      ch446q_timeout_count++;
      Serial.print("WARNING: CH446Q sendXYraw waiting for chipSelect for more than 1 second! (timeout #");
      Serial.print(ch446q_timeout_count);
      Serial.println(")");
      Serial.println("CH446Q: Attempting emergency PIO reset...");
      
      // Emergency reset: force chipSelect to -1 and reset PIO
      chipSelect = -1;
      pio_sm_set_enabled(pio, sm, false);
      delayMicroseconds(100);
      pio_sm_set_enabled(pio, sm, true);
      pio_interrupt_clear(pio, sm);
      
      // Debug PIO state after reset
      Serial.print("CH446Q: After emergency reset - SM enabled: ");
      Serial.print((pio->ctrl & (1u << (PIO_CTRL_SM_ENABLE_LSB + sm))) ? "YES" : "NO");
      Serial.print(", IRQ enabled: ");
      Serial.print(irq_is_enabled(PIO0_IRQ_1) ? "YES" : "NO");
      Serial.print(", chipSelect: ");
      Serial.println(chipSelect);
      isrFromPio();
      break;  // Break out of the loop to prevent infinite hang
    }
  }

    // unsigned long end = micros();
    // Serial.print("time: ");
    // Serial.println(end - start);
    // Serial.flush();

  // while (pio_interrupt_get(pio, sm)) {
  //   //delayMicroseconds(1);
  //   tight_loop_contents();
  //   }

  //delayMicroseconds(10);
  // isrFromPio();
  }

void createXYarray(void) { }

// Creates an index array sorted by chip, x, y while keeping main path array in net order
void createChipOrderedIndex() {
  // Initialize index array
  for (int i = 0; i < numberOfPaths; i++) {
    chipOrderedIndex[i] = i;
    }
  
  // Sort the index array based on chip, x, y values of the paths they point to
  for (int i = 0; i < numberOfPaths - 1; i++) {
    for (int j = 0; j < numberOfPaths - i - 1; j++) {
      bool swap = false;
      for (int k = 0; k < 4; k++) {
        int idx1 = chipOrderedIndex[j];
        int idx2 = chipOrderedIndex[j + 1];
        
        if (path[idx1].chip[k] < path[idx2].chip[k]) break;
        if (path[idx1].chip[k] > path[idx2].chip[k]) { swap = true; break; }
        if (path[idx1].x[k] < path[idx2].x[k]) break;
        if (path[idx1].x[k] > path[idx2].x[k]) { swap = true; break; }
        if (path[idx1].y[k] < path[idx2].y[k]) break;
        if (path[idx1].y[k] > path[idx2].y[k]) { swap = true; break; }
        }
      if (swap) {
        int temp = chipOrderedIndex[j];
        chipOrderedIndex[j] = chipOrderedIndex[j + 1];
        chipOrderedIndex[j + 1] = temp;
        }
      }
    }
  chipOrderValid = true;
  }

// Legacy function name kept for compatibility, but now creates index instead of sorting
void sortPathsByChipXY() {
  createChipOrderedIndex();
  }

