// SPDX-License-Identifier: MIT

#include <Arduino.h>
#include "JumperlessDefines.h"
#include "MatrixState.h"
#include "config.h"

#include "CH446Q.h"
#include "SerialWrapper.h"





struct netStruct net[MAX_NETS] = { //these are the special function nets that will always be made
  //netNumber,       ,netName          ,memberNodes[]         ,memberBridges[][2]     ,specialFunction        ,intsctNet[] ,doNotIntersectNodes[]                 ,priority (unused)
      {     127      ,"Empty Net"      ,{EMPTY_NET}           ,{{}}                   ,EMPTY_NET              ,{}          ,{EMPTY_NET,EMPTY_NET,EMPTY_NET,EMPTY_NET,EMPTY_NET,EMPTY_NET,EMPTY_NET} , 0},
      {     1        ,"GND"            ,{GND}                 ,{{}}                   ,GND                    ,{}          ,{BOTTOM_RAIL,TOP_RAIL,DAC0,DAC1}    , 1},
      {     2        ,"Top Rail"       ,{TOP_RAIL}            ,{{}}                   ,TOP_RAIL               ,{}          ,{GND, BOTTOM_RAIL, DAC0, DAC1}                               , 1},
      {     3        ,"Bottom Rail"    ,{BOTTOM_RAIL}         ,{{}}                   ,BOTTOM_RAIL            ,{}          ,{GND, TOP_RAIL, DAC0, DAC1}                               , 1},
      {     4        ,"DAC 0"          ,{DAC0}                ,{{}}                   ,DAC0                   ,{}          ,{GND, TOP_RAIL, BOTTOM_RAIL, DAC1}                               , 1},
      {     5        ,"DAC 1"          ,{DAC1}                ,{{}}                   ,DAC1                   ,{}          ,{GND, TOP_RAIL, BOTTOM_RAIL, DAC0}                               , 1},
      // {     6        ,"I Sense +"      ,{ISENSE_PLUS}         ,{{}}                   ,ISENSE_PLUS            ,{}          ,{ISENSE_MINUS}                      , 2},
      // {     7        ,"I Sense -"      ,{ISENSE_MINUS}        ,{{}}                   ,ISENSE_MINUS           ,{}          ,{ISENSE_PLUS}                       , 2},
  };

char* netNameConstants[MAX_NETS] = { (char*)"Net 0",(char*)"Net 1",(char*)"Net 2",(char*)"Net 3",(char*)"Net 4",(char*)"Net 5",(char*)"Net 6",(char*)"Net 7",(char*)"Net 8",(char*)"Net 9",(char*)"Net 10",(char*)"Net 11",(char*)"Net 12",(char*)"Net 13",(char*)"Net 14",(char*)"Net 15",(char*)"Net 16",(char*)"Net 17",(char*)"Net 18",(char*)"Net 19",(char*)"Net 20",(char*)"Net 21",(char*)"Net 22",(char*)"Net 23",(char*)"Net 24",(char*)"Net 25",(char*)"Net 26",(char*)"Net 27",(char*)"Net 28",(char*)"Net 29",(char*)"Net 30",(char*)"Net 31",(char*)"Net 32",(char*)"Net 33",(char*)"Net 34",(char*)"Net 35",(char*)"Net 36",(char*)"Net 37",(char*)"Net 38",(char*)"Net 39",(char*)"Net 40",(char*)"Net 41",(char*)"Net 42",(char*)"Net 43",(char*)"Net 44",(char*)"Net 45",(char*)"Net 46",(char*)"Net 47",(char*)"Net 48",(char*)"Net 49" };//,(char*)"Net 50",(char*)"Net 51",(char*)"Net 52",(char*)"Net 53",(char*)"Net 54",(char*)"Net 55",(char*)"Net 56",(char*)"Net 57",(char*)"Net 58",(char*)"Net 59",(char*)"Net 60",(char*)"Net 61",(char*)"Net 62"};//,{"Net 63",(char*)"Net 64",(char*)"Net 65",(char*)"Net 66",(char*)"Net 67",(char*)"Net 68",(char*)"Net 69",(char*)"Net 70",(char*)"Net 71",(char*)"Net 72",(char*)"Net 73",(char*)"Net 74",(char*)"Net 75",(char*)"Net 76",(char*)"Net 77",(char*)"Net 78",(char*)"Net 79",(char*)"Net 80",(char*)"Net 81",(char*)"Net 82",(char*)"Net 83",(char*)"Net 84",(char*)"Net 85",(char*)"Net 86",(char*)"Net 87",(char*)"Net 88",(char*)"Net 89",(char*)"Net 90",(char*)"Net 91",(char*)"Net 92",(char*)"Net 93",(char*)"Net 94",(char*)"Net 95",(char*)"Net 96",(char*)"Net 97",(char*)"Net 98",(char*)"Net 99",(char*)"Net 100",(char*)"Net 101",(char*)"Net 102",(char*)"Net 103",(char*)"Net 104",(char*)"Net 105",(char*)"Net 106",(char*)"Net 107",(char*)"Net 108",(char*)"Net 109",(char*)"Net 110",(char*)"Net 111",(char*)"Net 112",(char*)"Net 113",(char*)"Net 114",(char*)"Net 115",(char*)"Net 116",(char*)"Net 117",(char*)"Net 118",(char*)"Net 119",(char*)"Net 120",(char*)"Net 121",(char*)"Net 122",(char*)"Net 123",(char*)"Net 124",(char*)"Net 125",(char*)"Net 126",(char*)"Net 127"}};
//char (*netNamesPtr) = netNameConstants[0];


//note that the bottom row is shifted from the schematic by one, so the nodes are what's written on the board
const int bbNodesToChip[62] = { 0,
CHIP_A,CHIP_A,CHIP_A,CHIP_A,CHIP_A,CHIP_A,CHIP_A,  // 1, 2, 3, 4, 5, 6, 7, 8
CHIP_B,CHIP_B,CHIP_B,CHIP_B,CHIP_B,CHIP_B,CHIP_B,  // 9,10,11,12,13,14,15
CHIP_C,CHIP_C,CHIP_C,CHIP_C,CHIP_C,CHIP_C,CHIP_C,  //16,17,18,19,20,21,22
CHIP_D,CHIP_D,CHIP_D,CHIP_D,CHIP_D,CHIP_D,CHIP_D,  //23,24,25,26,27,28,29
CHIP_K,CHIP_L,                                     //30,31
CHIP_E,CHIP_E,CHIP_E,CHIP_E,CHIP_E,CHIP_E,CHIP_E,  //32,33,34,35,36,37,38
CHIP_F,CHIP_F,CHIP_F,CHIP_F,CHIP_F,CHIP_F,CHIP_F,  //39,40,41,42,43,44,45
CHIP_G,CHIP_G,CHIP_G,CHIP_G,CHIP_G,CHIP_G,CHIP_G,  //46,47,48,49,50,51,52
CHIP_H,CHIP_H,CHIP_H,CHIP_H,CHIP_H,CHIP_H,CHIP_H,  //53,54,55,56,57,58,59
CHIP_K, CHIP_L };                                           //60


// const int xHopMap[12][12][16] =  //[chip] [other chip][x]
// {


int indexByChip[MAX_BRIDGES] = {0};
int indexByNet[MAX_BRIDGES] = {0};

//#include <pico/rand.h>


// void initPaths(void) {
//   for (int i = 0; i < MAX_BRIDGES; i++) {
//     path[i].net = 0;
//     path[i].node1 = 0;
//     path[i].node2 = 0;
//     path[i].duplicate = 0;
//     path[i].pathType = BBtoBB;
//     path[i].nodeType[0] = BB;
//     path[i].nodeType[1] = BB;
//     path[i].nodeType[2] = BB;
//     path[i].sameChip = false;
//     path[i].skip = false;
//     path[i].altPathNeeded = false;


//   }
// }


void initNets(void) {
  for (int i = 0; i < 6; i++) {
    net[i].priority = 1;
    
    //net[i].uniqueID = i;
  }
  // net[0].name = "Empty Net";
  // net[1].name = "GND";
  // net[1].doNotIntersectNodes[0] = BOTTOM_RAIL;
  // net[1].doNotIntersectNodes[1] = TOP_RAIL;
  // net[1].doNotIntersectNodes[2] = DAC0;
  // net[1].doNotIntersectNodes[3] = DAC1;
  


  // net[2].name = "Top Rail";
  // net[2].doNotIntersectNodes[0] = GND;
  // net[2].doNotIntersectNodes[1] = BOTTOM_RAIL;
  // net[2].doNotIntersectNodes[2] = DAC0;
  // net[2].doNotIntersectNodes[3] = DAC1;
  // net[2].doNotIntersectNodes[4] = RP_GPIO_1;
  // net[2].doNotIntersectNodes[5] = RP_GPIO_2;
  // net[2].doNotIntersectNodes[6] = RP_GPIO_3;
  // net[2].doNotIntersectNodes[7] = RP_GPIO_4;
  // net[2].doNotIntersectNodes[8] = RP_GPIO_5;
  // net[2].doNotIntersectNodes[9] = RP_GPIO_6;
  // net[2].doNotIntersectNodes[10] = RP_GPIO_7;
  // net[2].doNotIntersectNodes[11] = RP_GPIO_8;

  // net[3].name = "Bottom Rail";
  // net[3].doNotIntersectNodes[0] = GND;
  // net[3].doNotIntersectNodes[1] = TOP_RAIL;
  // net[3].doNotIntersectNodes[2] = DAC0;
  // net[3].doNotIntersectNodes[3] = DAC1;
  // net[3].doNotIntersectNodes[4] = RP_GPIO_1;
  // net[3].doNotIntersectNodes[5] = RP_GPIO_2;
  // net[3].doNotIntersectNodes[6] = RP_GPIO_3;
  // net[3].doNotIntersectNodes[7] = RP_GPIO_4;
  // net[3].doNotIntersectNodes[8] = RP_GPIO_5;
  // net[3].doNotIntersectNodes[9] = RP_GPIO_6;
  // net[3].doNotIntersectNodes[10] = RP_GPIO_7;
  // net[3].doNotIntersectNodes[11] = RP_GPIO_8;

  // net[4].name = "DAC 0";
  // net[4].doNotIntersectNodes[0] = GND;
  // net[4].doNotIntersectNodes[1] = TOP_RAIL;
  // net[4].doNotIntersectNodes[2] = BOTTOM_RAIL;
  // net[4].doNotIntersectNodes[3] = DAC1;
  // net[4].doNotIntersectNodes[4] = RP_GPIO_1;
  // net[4].doNotIntersectNodes[5] = RP_GPIO_2;
  // net[4].doNotIntersectNodes[6] = RP_GPIO_3;
  // net[4].doNotIntersectNodes[7] = RP_GPIO_4;
  // net[4].doNotIntersectNodes[8] = RP_GPIO_5;
  // net[4].doNotIntersectNodes[9] = RP_GPIO_6;
  // net[4].doNotIntersectNodes[10] = RP_GPIO_7;
  // net[4].doNotIntersectNodes[11] = RP_GPIO_8;

  // net[5].name = "DAC 1";
  // net[5].doNotIntersectNodes[0] = GND;
  // net[5].doNotIntersectNodes[1] = TOP_RAIL;
  // net[5].doNotIntersectNodes[2] = BOTTOM_RAIL;
  // net[5].doNotIntersectNodes[3] = DAC0;
  // net[5].doNotIntersectNodes[4] = RP_GPIO_1;
  // net[5].doNotIntersectNodes[5] = RP_GPIO_2;
  // net[5].doNotIntersectNodes[6] = RP_GPIO_3;
  // net[5].doNotIntersectNodes[7] = RP_GPIO_4;
  // net[5].doNotIntersectNodes[8] = RP_GPIO_5;
  // net[5].doNotIntersectNodes[9] = RP_GPIO_6;
  // net[5].doNotIntersectNodes[10] = RP_GPIO_7;
  // net[5].doNotIntersectNodes[11] = RP_GPIO_8;


  for (int i = 6; i < MAX_NETS; i++) {
    // uint16_t   uniqueID = net[i].uniqueID;

    net[i] = {0, " ", {}, {{}}, 0, {}, {}, 0, 0, 0, 0, false};
    net[i].priority = 1;
    net[i].termColor = 15; // white


  }
  // for (int i = 6; i < MAX_NETS; i++) {
  //   net[i].priority = 1;
  //   net[i].visible = 0;
  //   net[i].name = "";
  //   for (int j = 0; j < MAX_DUPLICATE; j++) {
  //     net[i].duplicatePaths[j] = -1;
  //   }
  //   net[i].machine = false;
  //   net[i].numberOfDuplicates = 0;
  //   net[i].termColor = 15;


  // }
}




struct chipStatus ch[12] = { //this is the revision 5 chip status (default now)
  {0,'A',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_I, CHIP_J, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_K, CHIP_F, CHIP_F, CHIP_G, CHIP_L, CHIP_H, CHIP_H},//X MAP constant
  {BOUNCE_NODE, TOP_1, TOP_2,TOP_3, TOP_4, TOP_5, TOP_6, TOP_7}},  // Y MAP constant

  {1,'B',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_A, CHIP_I, CHIP_J, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_K, CHIP_G, CHIP_G, CHIP_H, CHIP_L},
  {BOUNCE_NODE, TOP_8, TOP_9,TOP_10,TOP_11,TOP_12,TOP_13,TOP_14}},//yMap

  {2,'C',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_I, CHIP_J, CHIP_D, CHIP_D, CHIP_E, CHIP_L, CHIP_F, CHIP_F, CHIP_G, CHIP_K, CHIP_H, CHIP_H},
  {BOUNCE_NODE,TOP_15, TOP_16,TOP_17,TOP_18,TOP_19,TOP_20,TOP_21}},

  {3,'D',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_I, CHIP_J, CHIP_E, CHIP_E, CHIP_F, CHIP_L, CHIP_G, CHIP_G, CHIP_H, CHIP_K},
  {BOUNCE_NODE,TOP_22, TOP_23,TOP_24,TOP_25,TOP_26,TOP_27,TOP_28}},

  {4,'E',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_K, CHIP_B, CHIP_B, CHIP_C, CHIP_L, CHIP_D, CHIP_D, CHIP_I, CHIP_J, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
  {BOUNCE_NODE, BOTTOM_1,  BOTTOM_2, BOTTOM_3, BOTTOM_4, BOTTOM_5, BOTTOM_6, BOTTOM_7}},

  {5,'F',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_A, CHIP_B, CHIP_K, CHIP_C, CHIP_C, CHIP_D, CHIP_L, CHIP_E, CHIP_E, CHIP_I, CHIP_J, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
  {BOUNCE_NODE, BOTTOM_8,  BOTTOM_9, BOTTOM_10,BOTTOM_11,BOTTOM_12,BOTTOM_13,BOTTOM_14}},

  {6,'G',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_L, CHIP_B, CHIP_B, CHIP_C, CHIP_K, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_I, CHIP_J, CHIP_H, CHIP_H},
  {BOUNCE_NODE,BOTTOM_15, BOTTOM_16,BOTTOM_17,BOTTOM_18,BOTTOM_19,BOTTOM_20,BOTTOM_21}},

  {7,'H',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {CHIP_A, CHIP_A, CHIP_B, CHIP_L, CHIP_C, CHIP_C, CHIP_D, CHIP_K, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_I, CHIP_J},
  {BOUNCE_NODE,BOTTOM_22,  BOTTOM_23,BOTTOM_24,BOTTOM_25,BOTTOM_26,BOTTOM_27,BOTTOM_28}},

  {8,'I',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {NANO_A0, NANO_D1, NANO_A2, NANO_D3, NANO_A4, NANO_D5, NANO_A6, NANO_D7, NANO_D11, NANO_D9, NANO_D13,ISENSE_PLUS/*NANO_RESET_0*/ , CHIP_L, CHIP_J, CHIP_K,RP_UART_RX }, //this is for V5r1 change this for V5r2
  {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

  {9,'J',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {NANO_D0, NANO_A1, NANO_D2, NANO_A3, NANO_D4, NANO_A5, NANO_D6, NANO_A7, NANO_D8, NANO_D10, NANO_D12, ISENSE_MINUS/*NANO_RESET_1*/ , CHIP_L, CHIP_I, CHIP_K, RP_UART_TX},  //this is for V5r1 change this for V5r2
  {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

  {10,'K',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {29, 59, ROUTABLE_BUFFER_IN, NANO_AREF, TOP_RAIL, BOTTOM_RAIL, DAC1, DAC0, ADC0, ADC1, ADC2, ADC3, CHIP_L, CHIP_I, CHIP_J, GND}, //this is for V5r1 change this for V5r2
  {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

  {11,'L',
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
  {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
  {30, 60, ROUTABLE_BUFFER_OUT, ADC4_5V, RP_GPIO_20, RP_GPIO_21, RP_GPIO_22, RP_GPIO_23, RP_GPIO_24, RP_GPIO_25, RP_GPIO_26, RP_GPIO_27, CHIP_I, CHIP_J, CHIP_K, GND}, //this is for V5r1 change this for V5r2
  {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}}
  };
//#endif


int rev5plusXmap[12][16] = {
    {CHIP_I, CHIP_J, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_K, CHIP_F, CHIP_F, CHIP_G, CHIP_L, CHIP_H, CHIP_H},//X MAP constant
    {CHIP_A, CHIP_A, CHIP_I, CHIP_J, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_K, CHIP_G, CHIP_G, CHIP_H, CHIP_L},
    {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_I, CHIP_J, CHIP_D, CHIP_D, CHIP_E, CHIP_L, CHIP_F, CHIP_F, CHIP_G, CHIP_K, CHIP_H, CHIP_H},
    {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_I, CHIP_J, CHIP_E, CHIP_E, CHIP_F, CHIP_L, CHIP_G, CHIP_G, CHIP_H, CHIP_K},
    {CHIP_A, CHIP_K, CHIP_B, CHIP_B, CHIP_C, CHIP_L, CHIP_D, CHIP_D, CHIP_I, CHIP_J, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
    {CHIP_A, CHIP_A, CHIP_B, CHIP_K, CHIP_C, CHIP_C, CHIP_D, CHIP_L, CHIP_E, CHIP_E, CHIP_I, CHIP_J, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
    {CHIP_A, CHIP_L, CHIP_B, CHIP_B, CHIP_C, CHIP_K, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_I, CHIP_J, CHIP_H, CHIP_H},
    {CHIP_A, CHIP_A, CHIP_B, CHIP_L, CHIP_C, CHIP_C, CHIP_D, CHIP_K, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_I, CHIP_J},
    {NANO_A0, NANO_D1, NANO_A2, NANO_D3, NANO_A4, NANO_D5, NANO_A6, NANO_D7, NANO_D11, NANO_D9, NANO_D13,ISENSE_PLUS/*NANO_RESET_0*/ , CHIP_L, CHIP_J, CHIP_K,RP_UART_RX },
    {NANO_D0, NANO_A1, NANO_D2, NANO_A3, NANO_D4, NANO_A5, NANO_D6, NANO_A7, NANO_D8, NANO_D10, NANO_D12, ISENSE_MINUS/*NANO_RESET_1*/ , CHIP_L, CHIP_I, CHIP_K, RP_UART_TX},
    {29, 59, ROUTABLE_BUFFER_IN, NANO_AREF, TOP_RAIL, BOTTOM_RAIL, DAC1, DAC0, ADC0, ADC1, ADC2, ADC3, CHIP_L, CHIP_I, CHIP_J, GND},
    {30, 60, ROUTABLE_BUFFER_OUT, ADC4_5V, RP_GPIO_20, RP_GPIO_21, RP_GPIO_22, RP_GPIO_23, RP_GPIO_24, RP_GPIO_25, RP_GPIO_26, RP_GPIO_27, CHIP_I, CHIP_J, CHIP_K, GND},
  };

int rev4minusXmap[12][16] = {
  {CHIP_I, CHIP_J, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_K, CHIP_F, CHIP_F, CHIP_G, CHIP_L, CHIP_H, CHIP_H},//X MAP constant
  {CHIP_A, CHIP_A, CHIP_I, CHIP_J, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_K, CHIP_G, CHIP_G, CHIP_H, CHIP_L},
  {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_I, CHIP_J, CHIP_D, CHIP_D, CHIP_E, CHIP_L, CHIP_F, CHIP_F, CHIP_G, CHIP_K, CHIP_H, CHIP_H},
  {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_I, CHIP_J, CHIP_E, CHIP_E, CHIP_F, CHIP_L, CHIP_G, CHIP_G, CHIP_H, CHIP_K},
  {CHIP_A, CHIP_K, CHIP_B, CHIP_B, CHIP_C, CHIP_L, CHIP_D, CHIP_D, CHIP_I, CHIP_J, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
  {CHIP_A, CHIP_A, CHIP_B, CHIP_K, CHIP_C, CHIP_C, CHIP_D, CHIP_L, CHIP_E, CHIP_E, CHIP_I, CHIP_J, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
  {CHIP_A, CHIP_L, CHIP_B, CHIP_B, CHIP_C, CHIP_K, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_I, CHIP_J, CHIP_H, CHIP_H},
  {CHIP_A, CHIP_A, CHIP_B, CHIP_L, CHIP_C, CHIP_C, CHIP_D, CHIP_K, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_I, CHIP_J},
  {NANO_A0, NANO_D1, NANO_A2, NANO_D3, NANO_A4, NANO_D5, NANO_A6, NANO_D7, NANO_D11, NANO_D9, NANO_D13,ISENSE_PLUS/*NANO_RESET_0*/ , CHIP_L, CHIP_J, CHIP_K,RP_UART_RX }, //this is for V5r1 change this for V5r2
  {NANO_D0, NANO_A1, NANO_D2, NANO_A3, NANO_D4, NANO_A5, NANO_D6, NANO_A7, NANO_D8, NANO_D10, NANO_D12, ISENSE_MINUS/*NANO_RESET_1*/ , CHIP_L, CHIP_I, CHIP_K, RP_UART_TX},  //this is for V5r1 change this for V5r2
  {29, 59, ROUTABLE_BUFFER_OUT, NANO_AREF, TOP_RAIL, BOTTOM_RAIL, DAC1, DAC0, ADC0, ADC1, ADC2, ADC3, CHIP_L, CHIP_I, CHIP_J, GND}, //this is for V5r1 change this for V5r2
  {30, 60, ROUTABLE_BUFFER_IN, ADC4_5V, RP_GPIO_20, RP_GPIO_21, RP_GPIO_22, RP_GPIO_23, RP_GPIO_24, RP_GPIO_25, RP_GPIO_26, RP_GPIO_27, CHIP_I, CHIP_J, CHIP_K, GND},

  };

const char* connectionNamesX[12][16] = {
  { "AI",  "AJ",  "AB0", "AB1", "AC0",  "AC1",  "AD0",  "AD1",  "AE0",  "AK",   "AF0",  "AF1",  "AG0",  "AL",   "AH0",  "AH1"  }, // A
  { "AB0", "AB1", "BI",  "BJ",  "BC0",  "BC1",  "BD0",  "BD1",  "BE0",  "BE1",  "BF0",  "BK",   "BG0",  "BG1",  "BH0",  "BL"   }, // B
  { "AC0", "AC1", "BC0", "BC1", "CI",   "CJ",   "CD0",  "CD1",  "CE0",  "CL",   "CF0",  "CF1",  "CG0",  "CK",   "CH0",  "CH1"  }, // C
  { "AD0", "AD1", "BD0", "BD1", "CD0",  "CD1",  "DI",   "DJ",   "DE0",  "DE1",  "DF0",  "DL",   "DG0",  "DK",   "DH0",  "DH1"  }, // D
  { "AE0", "EK",  "BE0", "BE1", "CE0",  "EL",   "DE0",  "DE1",  "EI",   "EJ",   "EF0",  "EF1",  "EG0",  "EG1",  "EH0",  "EH1"  }, // E
  { "AF0", "AF1", "BF0", "FK",  "CF0",  "CF1",  "DF0",  "FL",   "EF0",  "EF1",  "FI",   "FJ",   "FG0",  "FG1",  "FH0",  "FH1"  }, // F
  { "AG0", "GL",  "BG0", "BG1", "CG0",  "GK",   "DG0",  "DG1",  "EG0",  "EG1",  "FG0",  "FG1",  "GI",   "GJ",   "GH0",  "GH1"  }, // G
  { "AH0", "AH1", "BH0", "HL",  "CH0",  "CH1",  "DH0",  "HK",   "EH0",  "EH1",  "FH0",  "FH1",  "GH0",  "GH1",  "HI",   "HJ"   }, // H

  { "nA0", "nD1", "nA2", "nD3", "nA4",  "nD5",  "nA6",  "nD7",  "nD11", "nD9",  "nD13", "I+",   "IL",   "IJ",   "IK",   "uRX"  }, // I
  { "nD0", "nA1", "nD2", "nA3", "nD4",  "nA5",  "nD6",  "nA7",  "nD8",  "nD10", "nD12", "I-",   "JL",   "IJ",   "JK",   "uTX"  }, // J
  { "29",  "59",  "BFi", "ARF", "TRl",  "BRl",  "Da1",  "Da0",  "Ad0",  "Ad1",  "Ad2",  "Ad3",  "KL",   "KI",   "KJ",   "GND"  }, // K
  { "30",  "60",  "BFo",  "5V", "GP1",  "GP2",  "GP3",  "GP4",  "GP5",  "GP6",  "GP7",  "GP8",  "LI",   "LJ",   "LK",   "GND"  }  // L
  };

const char* connectionNamesY[12][8] = {
  { "u", "1",  "2",  "3",  "4",  "5",  "6",  "7"  },  // A
  { "u", "8",  "9",  "10", "11", "12", "13", "14" },  // B
  { "u", "15", "16", "17", "18", "19", "20", "21" },  // C
  { "u", "22", "23", "24", "25", "26", "27", "28" },  // D
  { "u", "31", "32", "33", "34", "35", "36", "37" },  // E
  { "u", "38", "39", "40", "41", "42", "43", "44" },  // F
  { "u", "45", "46", "47", "48", "49", "50", "51" },  // G
  { "u", "52", "53", "54", "55", "56", "57", "58" },  // H
  // Fill in the rest as needed for I, J, K, L
  { "AI", "BI", "CI", "DI", "EI", "FI", "GI", "HI" }, // I
  { "AJ", "BJ", "CJ", "DJ", "EJ", "FJ", "GJ", "HJ" }, // J
  { "AK", "BK", "CK", "DK", "EK", "FK", "GK", "HK" }, // K
  { "AL", "BL", "CL", "DL", "EL", "FL", "GL", "HL" }  // L
  };

char returnName[4] = "  ";

char* yName(int chip, int y) {
  returnName[0] = ' ';
  returnName[1] = ' ';
  returnName[2] = ' ';
  returnName[3] = '\0';
  if (chip >= 0 && chip < 12) {
    if (y >= 0 && y < 8) {
      const char* src = connectionNamesY[chip][y];
      int i = 0;
      for (; i < 3 && src[i] != '\0'; ++i) {
        returnName[i] = src[i];
        }
      // Pad with spaces if shorter than 3
      for (; i < 3; ++i) {
        returnName[i] = ' ';
        }
      returnName[3] = '\0';
      return returnName;
      }
    }
  return returnName;
  }

char* xName(int chip, int x) {
  returnName[0] = ' ';
  returnName[1] = ' ';
  returnName[2] = ' ';
  returnName[3] = '\0';
  if (chip >= 0 && chip < 12) {
    if (x >= 0 && x < 16) {
      const char* src = connectionNamesX[chip][x];
      int i = 0;
      for (; i < 3 && src[i] != '\0'; ++i) {
        returnName[i] = src[i];
        }
      // Pad with spaces if shorter than 3
      for (; i < 3; ++i) {
        returnName[i] = ' ';
        }
      returnName[3] = '\0';
      return returnName;
      }
    }
  return returnName;
  }

#include "NetManager.h"



  struct nodeStruct nodeNames[30] = {
    {"gnd", GND}, {"top_rail", TOP_RAIL}, {"bot_rail", BOTTOM_RAIL}, {"dac_0", DAC0}, {"dac_1", DAC1}, 
    {"isense_p", ISENSE_PLUS}, {"isense_n", ISENSE_MINUS}, {"buf_in", ROUTABLE_BUFFER_IN}, {"buf_out", ROUTABLE_BUFFER_OUT}, 
    {"adc_0", ADC0}, {"adc_1", ADC1}, {"adc_2", ADC2}, {"adc_3", ADC3}, {"adc_4", ADC4}, {"probe", ADC7_PROBE}, 
    {"uart_tx", RP_UART_TX}, {"uart_rx", RP_UART_RX}, 
    {"gpio_1", RP_GPIO_1}, {"gpio_2", RP_GPIO_2}, {"gpio_3", RP_GPIO_3}, {"gpio_4", RP_GPIO_4}, 
    {"gpio_5", RP_GPIO_5}, {"gpio_6", RP_GPIO_6}, {"gpio_7", RP_GPIO_7}, {"gpio_8", RP_GPIO_8}
  };

int printOrder[30] = {
  0, 9, 15, 
  1, 10, 16, 
  2, 11, 17, 
  3, 12, 18, 
  4, 13, 19, 
  5, 14, 20, 
  6, -1, 21, 
  7, -1, 22,
   8,-1, 23, 
  -1,-1,24};

   void printAllConnectableNodes(void) {

    //for (int j = 0; j < 3; j++) {
int columnWidth = 16;
    Serial.println("         connectable nodes");
    Serial.println("    (there are a ton of aliases,");
    Serial.println("     most variations will work.)");
    Serial.println();
    
    for (int i = 0; i < 30; i++) {
      int numPrinted = 0;
      if (printOrder[i] == -1) {
        numPrinted += Serial.print("    ");
      }else{
        
      numPrinted += Serial.print(nodeNames[printOrder[i]].name); 
      }
      for (int j = 0; j < columnWidth - numPrinted; j++) {
        Serial.print(" ");
      }
      

      if (i % 3 == 2) {
        Serial.println();
      }
      //Serial.println(nodeNames[i].define);

    }
    Serial.println();
    //}
   }

  void printAllConnectableNodes(int actuallyCheck) {
    for (int i = 100; i < 150; i++) {
      if (isConnectable(i)) {
       // Serial.print(i);
        Serial.print("\"");
        char name[11] = "        ";
        strcpy(name, definesToChar(i, 0));
        for (int j = 0; j < 11; j++) {
          name[j] = tolower(name[j]);
        }

        Serial.print(name);
        Serial.print("\", ");
       // Serial.print(i);
       // Serial.println();
      }
    }
    Serial.println();
  }


int globalDoNotIntersects[60][2] = {
  {GND, TOP_RAIL},
  {GND, BOTTOM_RAIL},
  {GND, DAC1},
  {GND, DAC0},
  {TOP_RAIL, DAC1},
  {TOP_RAIL, DAC0},
  {BOTTOM_RAIL, DAC1},
  {BOTTOM_RAIL, DAC0},
  {TOP_RAIL, BOTTOM_RAIL},
  {DAC0, DAC1},
  {RP_GPIO_20, TOP_RAIL},
  {RP_GPIO_20, BOTTOM_RAIL},
  {RP_GPIO_20, DAC1}, //maybe
  {RP_GPIO_20, DAC0},
  {RP_GPIO_21, TOP_RAIL},
  {RP_GPIO_21, BOTTOM_RAIL},
  {RP_GPIO_21, DAC1}, //maybe
  {RP_GPIO_21, DAC0},
  {RP_GPIO_22, TOP_RAIL},
  {RP_GPIO_22, BOTTOM_RAIL},
  {RP_GPIO_22, DAC1}, //maybe
  {RP_GPIO_22, DAC0},
  {RP_GPIO_23, TOP_RAIL},
  {RP_GPIO_23, BOTTOM_RAIL},
  {RP_GPIO_23, DAC1}, //maybe
  {RP_GPIO_23, DAC0},
  {RP_GPIO_24, TOP_RAIL},
  {RP_GPIO_24, BOTTOM_RAIL},
  {RP_GPIO_24, DAC1}, //maybe
  {RP_GPIO_24, DAC0},
  {RP_GPIO_25, TOP_RAIL},
  {RP_GPIO_25, BOTTOM_RAIL},
  {RP_GPIO_25, DAC1}, //maybe
  {RP_GPIO_25, DAC0},
  {RP_GPIO_26, TOP_RAIL},
  {RP_GPIO_26, BOTTOM_RAIL},
  {RP_GPIO_26, DAC1}, //maybe
  {RP_GPIO_26, DAC0},
  {RP_GPIO_27, TOP_RAIL},
  {RP_GPIO_27, BOTTOM_RAIL},
  {RP_GPIO_27, DAC1}, //maybe
  {RP_GPIO_27, DAC0},
  {RP_UART_RX, TOP_RAIL},
  {RP_UART_RX, BOTTOM_RAIL},
  {RP_UART_RX, DAC1}, //maybe
  {RP_UART_RX, DAC0},
  {RP_UART_TX, TOP_RAIL},
  {RP_UART_TX, BOTTOM_RAIL},
  {RP_UART_TX, DAC1}, //maybe
  {RP_UART_TX, DAC0}, 
  {ROUTABLE_BUFFER_OUT, TOP_RAIL},
  {ROUTABLE_BUFFER_OUT, BOTTOM_RAIL},
  {ROUTABLE_BUFFER_OUT, DAC1}, //maybe
  {ROUTABLE_BUFFER_OUT, DAC0},

  
  };



/// @brief checks if a connection is allowed by checking global doNotIntersect rules
/// @param node1 the first node
/// @param node2 the second node
/// @return true if the connection is allowed, false otherwise
bool connectionAllowed(int node1, int node2) {

  for (int i = 0; i < 60; i++) {
    if (globalDoNotIntersects[i][0] == node1 && globalDoNotIntersects[i][1] == node2) {
      return false;
      }
    if (globalDoNotIntersects[i][0] == node2 && globalDoNotIntersects[i][1] == node1) {
      return false;
      }
    }


    return true;
  }

/// @brief checks if a node is connectable by checking if it's in the crossbar matrix
/// @param node the node to check
/// @return true if the node is connectable, false otherwise
bool isConnectable(int node) {
  if (node == -1) {
    return false;
    }
  for (int i = 0; i < 12; i++) {
    if (i < 8) {
      for (int j = 0; j < 8; j++) {
        if (ch[i].yMap[j] == node) {
          return true;
        }
      }
    }
    else {
      for (int j = 0; j < 16; j++) {
        if (ch[i].xMap[j] == node) {
          return true;
        }
      }
    }
  }
  return false;
  }






//#if PROTOTYPE_VERSION <= 4
void initChipStatus(void) {

  if (jumperlessConfig.hardware.revision <= 4) {
    for (int i = 0; i < 12; i++) {
      for (int j = 0; j < 16; j++) {
        ch[i].xMap[j] = rev4minusXmap[i][j];
        }
      }
    //#endif
    // Serial.println("initChipStatus");
    // Serial.println(jumperlessConfig.hardware.revision);
    // Serial.println(jumperlessConfig.hardware.probe_revision);

    //#if PROTOTYPE_VERSION > 4

    }
  }


/*

//!rev4
struct chipStatus ch[12] = {
      {0,'A',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_I, CHIP_J, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_K, CHIP_F, CHIP_F, CHIP_G, CHIP_L, CHIP_H, CHIP_H},//X MAP constant
      {BOUNCE_NODE, TOP_1, TOP_2,TOP_3, TOP_4, TOP_5, TOP_6, TOP_7}},  // Y MAP constant

      {1,'B',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_A, CHIP_I, CHIP_J, CHIP_C, CHIP_C, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_K, CHIP_G, CHIP_G, CHIP_H, CHIP_L},
      {BOUNCE_NODE, TOP_8, TOP_9,TOP_10,TOP_11,TOP_12,TOP_13,TOP_14}},//yMap

      {2,'C',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_I, CHIP_J, CHIP_D, CHIP_D, CHIP_E, CHIP_L, CHIP_F, CHIP_F, CHIP_G, CHIP_K, CHIP_H, CHIP_H},
      {BOUNCE_NODE,TOP_15, TOP_16,TOP_17,TOP_18,TOP_19,TOP_20,TOP_21}},

      {3,'D',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_A, CHIP_B, CHIP_B, CHIP_C, CHIP_C, CHIP_I, CHIP_J, CHIP_E, CHIP_E, CHIP_F, CHIP_L, CHIP_G, CHIP_G, CHIP_H, CHIP_K},
      {BOUNCE_NODE,TOP_22, TOP_23,TOP_24,TOP_25,TOP_26,TOP_27,TOP_28}},

      {4,'E',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_K, CHIP_B, CHIP_B, CHIP_C, CHIP_L, CHIP_D, CHIP_D, CHIP_I, CHIP_J, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
      {BOUNCE_NODE, BOTTOM_1,  BOTTOM_2, BOTTOM_3, BOTTOM_4, BOTTOM_5, BOTTOM_6, BOTTOM_7}},

      {5,'F',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_A, CHIP_B, CHIP_K, CHIP_C, CHIP_C, CHIP_D, CHIP_L, CHIP_E, CHIP_E, CHIP_I, CHIP_J, CHIP_G, CHIP_G, CHIP_H, CHIP_H},
      {BOUNCE_NODE, BOTTOM_8,  BOTTOM_9, BOTTOM_10,BOTTOM_11,BOTTOM_12,BOTTOM_13,BOTTOM_14}},

      {6,'G',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_L, CHIP_B, CHIP_B, CHIP_C, CHIP_K, CHIP_D, CHIP_D, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_I, CHIP_J, CHIP_H, CHIP_H},
      {BOUNCE_NODE,BOTTOM_15, BOTTOM_16,BOTTOM_17,BOTTOM_18,BOTTOM_19,BOTTOM_20,BOTTOM_21}},

      {7,'H',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {CHIP_A, CHIP_A, CHIP_B, CHIP_L, CHIP_C, CHIP_C, CHIP_D, CHIP_K, CHIP_E, CHIP_E, CHIP_F, CHIP_F, CHIP_G, CHIP_G, CHIP_I, CHIP_J},
      {BOUNCE_NODE,BOTTOM_22,  BOTTOM_23,BOTTOM_24,BOTTOM_25,BOTTOM_26,BOTTOM_27,BOTTOM_28}},

      {8,'I',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {NANO_A0, NANO_D1, NANO_A2, NANO_D3, NANO_A4, NANO_D5, NANO_A6, NANO_D7, NANO_D11, NANO_D9, NANO_D13,ISENSE_PLUS or NANO_RESET_0  , CHIP_L, CHIP_J, CHIP_K,RP_UART_RX }, //this is for V5r1 change this for V5r2
      {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

      {9,'J',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {NANO_D0, NANO_A1, NANO_D2, NANO_A3, NANO_D4, NANO_A5, NANO_D6, NANO_A7, NANO_D8, NANO_D10, NANO_D12, ISENSE_MINUS or NANO_RESET_1, CHIP_L, CHIP_I, CHIP_K, RP_UART_TX},  //this is for V5r1 change this for V5r2
      {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

      {10,'K',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {29, 59, ROUTABLE_BUFFER_OUT, NANO_AREF, TOP_RAIL, BOTTOM_RAIL, DAC1, DAC0, ADC0, ADC1, ADC2, ADC3, CHIP_L, CHIP_I, CHIP_J, GND}, //this is for V5r1 change this for V5r2
      {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}},

      {11,'L',
      {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}, // x status
      {-1,-1,-1,-1,-1,-1,-1,-1}, //y status
      {30, 60, ROUTABLE_BUFFER_IN, ADC4_5V, RP_GPIO_20, RP_GPIO_21, RP_GPIO_22, RP_GPIO_23, RP_GPIO_24, RP_GPIO_25, RP_GPIO_26, RP_GPIO_27, CHIP_I, CHIP_J, CHIP_K, GND}, //this is for V5r1 change this for V5r2
      {CHIP_A,CHIP_B,CHIP_C,CHIP_D,CHIP_E,CHIP_F,CHIP_G,CHIP_H}}
      };
      */

enum nanoPinsToIndex { NANO_PIN_D0, NANO_PIN_D1, NANO_PIN_D2, NANO_PIN_D3, NANO_PIN_D4, NANO_PIN_D5, NANO_PIN_D6, NANO_PIN_D7, NANO_PIN_D8, NANO_PIN_D9, NANO_PIN_D10, NANO_PIN_D11, NANO_PIN_D12, NANO_PIN_D13, NANO_PIN_RST, NANO_PIN_REF, NANO_PIN_A0, NANO_PIN_A1, NANO_PIN_A2, NANO_PIN_A3, NANO_PIN_A4, NANO_PIN_A5, NANO_PIN_A6, NANO_PIN_A7 };


struct nanoStatus nano = {  //there's only one of these so ill declare and initalize together unlike above

  //all these arrays should line up (both by index and visually) so one index will give you all this data

//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        | 
{   " D0",   " D1",   " D2",   " D3",   " D4",   " D5",   " D6",   " D7",   " D8",   " D9",    "D10",    "D11",     "D12",    "D13",      "RST",     "REF",   " A0",   " A1",   " A2",   " A3",   " A4",   " A5",   " A6",   " A7"},// String with readable name //padded to 3 chars (space comes before chars)
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |                         
{ NANO_D0, NANO_D1, NANO_D2, NANO_D3, NANO_D4, NANO_D5, NANO_D6, NANO_D7, NANO_D8, NANO_D9, NANO_D10, NANO_D11,  NANO_D12, NANO_D13, NANO_RESET, NANO_AREF, NANO_A0, NANO_A1, NANO_A2, NANO_A3, NANO_A4, NANO_A5, NANO_A6, NANO_A7},//Array index to internal arbitrary #defined number
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |    
{  1     , 1      , 1      , 1      , 1      , 1      , 1      , 1      , 1      , 1      , 1       , 1       ,  1       , 1       , 1         , 1        , 1      , 1      , 1      , 1      , 1      , 1      , 1      , 1      },//Whether this pin has 1 or 2 connections to special function chips    (OR maybe have it be a map like i = 2  j = 3  k = 4  l = 5 if there's 2 it's the product of them ij = 6  ik = 8  il = 10 jk = 12 jl = 15 kl = 20 we're trading minuscule amounts of CPU for RAM)  
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |    
{ CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I  ,  CHIP_J  , CHIP_I  , CHIP_I    ,  CHIP_K  , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J , CHIP_I , CHIP_J },//Since there's no overlapping connections between Chip I and J, this holds which of those 2 chips has a connection at that index, if numConns is 1, you only need to check this one
{ -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1      ,  -1      , -1      , -1        , -1       , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     },//Since there's no overlapping connections between Chip K and L, this holds which of those 2 chips has a connection at that index, -1 for no connection
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |   
{ -1     , 1      , -1     , 3      , -1     , 5      , -1     , 7      , -1     , 9      , -1     , 8       ,  -1      , 10      , 11        , -1       , 0      , -1     , 2      , -1     , 4      , -1     , 6      , -1     },//holds which X pin is connected to the index on Chip I, -1 for none
{ -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0       ,  -1      , 0       , 0         , -1       , 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1     },//-1 for not connected to that chip, 0 for available, >0 means it's connected and the netNumber is stored here
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |    
{ 0      , -1     , 2      , -1     , 4      , -1     , 6      , -1     , 8      , -1     , 9      , -1      ,  10      , -1      , -1        , 11       , -1     , 1      , -1     , 3      , -1     , 5      , -1     , 7      },//holds which X pin is connected to the index on Chip J, -1 for none
{ 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0      , -1      , 0        , 0       , -1        , 0        , -1     , 0      , -1     , 0      , -1     , 0      , -1     , 0      },//-1 for not connected to that chip, 0 for available, >0 means it's connected and the netNumber is stored here
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |    
{ -1     , -1     , 4      , 5      , 6      , 7      , 8      , 9      , 10     , 11     , 12     , 13      ,  14      , -1      , -1        , -1       , 0      , 1      , 2      , 3      , -1     , -1     , -1     , -1     },//holds which X pin is connected to the index on Chip K, -1 for none
{ -1     , -1     , 0      , 0      , 0      , 0      , 0      , 0      , 0      , 0      , 0      , 0       ,  0       , -1      , -1        , -1       , 0      , 0      , 0      , 0      , -1     , -1     , -1     , -1     },//-1 for not connected to that chip, 0 for available, >0 means it's connected and the netNumber is stored here
//                         |        |        |        |        |        |        |        |        |         |          |         |         |          |         |           |          |        |        |        |        |        |        |        |        |    
{ -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1      ,  -1      , -1      , -1        , -1       , -1     , -1     , -1     , -1     , 12     , 13     , -1     , -1     },//holds which X pin is connected to the index on Chip L, -1 for none
{ -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1     , -1      ,  -1      , -1      , -1        , -1       , -1     , -1     , -1     , -1     , 0      , 0      , -1     , -1     },//-1 for not connected to that chip, 0 for available, >0 means it's connected and the netNumber is stored here

// mapIJKL[]     will tell you whethher there's a connection from that nano pin to the corresponding special function chip 
// xMapIJKL[]    will tell you the X pin that it's connected to on that sf chip
// xStatusIJKL[] says whether that x pin is being used (this should be the same as mt[8-10].xMap[] if theyre all stacked on top of each other)
//              I haven't decided whether to make this just a flag, or store that signal's destination
{NANO_D0, NANO_D1, NANO_D2, NANO_D3, NANO_D4, NANO_D5, NANO_D6, NANO_D7, NANO_D8, NANO_D9, NANO_D10, NANO_D11, NANO_D12, NANO_D13, NANO_RESET, NANO_AREF, NANO_A0, NANO_A1, NANO_A2, NANO_A3, NANO_A4, NANO_A5, NANO_A6, NANO_A7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,GND,101,102,SUPPLY_3V3,104,SUPPLY_5V,DAC0,DAC1,ISENSE_PLUS,ISENSE_MINUS}

  };

struct pathStruct path[MAX_BRIDGES]; // node1, node2, net, chip[3], x[3], y[3]



SFmapPair sfMappings[100] = {
    {"GND", GND},
    {"GROUND", GND},
    {"SUPPLY_5V", SUPPLY_5V},
    {"SUPPLY_3V3", SUPPLY_3V3},
    {"DAC0_5V", DAC0},
    {"DAC1_8V", DAC1},
    {"DAC0", DAC0},
    {"DAC1", DAC1},
    {"INA_N", ISENSE_MINUS},
    {"INA_P", ISENSE_PLUS},
    {"I_N", ISENSE_MINUS},
    {"I_P", ISENSE_PLUS},
    {"ISENSE_MINUS", ISENSE_MINUS},
    {"ISENSE_PLUS", ISENSE_PLUS},
    {"CURRENT_SENSE_MINUS", ISENSE_MINUS},
    {"CURRENT_SENSE_PLUS", ISENSE_PLUS},
    {"EMPTY_NET", EMPTY_NET},
    {"ADC0_5V", ADC0},
    {"ADC1_5V", ADC1},
    {"ADC2_5V", ADC2},
    {"ADC3_8V", ADC3},
    {"ADC0", ADC0},
    {"ADC1", ADC1},
    {"ADC2", ADC2},
    {"ADC3", ADC3},
    {"+5V", SUPPLY_5V},
    {"5V", SUPPLY_5V},
    {"3.3V", SUPPLY_3V3},
    {"3V3", SUPPLY_3V3},
    {"RP_GPIO_0", RP_GPIO_0},
    {"RP_UART_TX", RP_UART_TX},
    {"RP_UART_RX", RP_UART_RX},
    {"RP_GPIO_1", RP_GPIO_1},
    {"RP_GPIO_2", RP_GPIO_2},
    {"RP_GPIO_3", RP_GPIO_3},
    {"RP_GPIO_4", RP_GPIO_4},
    {"RP_GPIO_5", RP_GPIO_5},
    {"RP_GPIO_6", RP_GPIO_6},
    {"RP_GPIO_7", RP_GPIO_7},
    {"RP_GPIO_8", RP_GPIO_8},
    {"GPIO_0", RP_GPIO_0},
    {"UART_TX", RP_UART_TX},
    {"UART_RX", RP_UART_RX},
    {"NANO_RESET", NANO_RESET},
    {"NANO_AREF", NANO_AREF},
    {"NANO_D0", NANO_D0},
    {"NANO_D1", NANO_D1},
    {"NANO_D2", NANO_D2},
    {"NANO_D3", NANO_D3},
    {"NANO_D4", NANO_D4},
    {"NANO_D5", NANO_D5},
    {"NANO_D6", NANO_D6},
    {"NANO_D7", NANO_D7},
    {"NANO_D8", NANO_D8},
    {"NANO_D9", NANO_D9},
    {"NANO_D10", NANO_D10},
    {"NANO_D11", NANO_D11},
    {"NANO_D12", NANO_D12},
    {"NANO_D13", NANO_D13},
    {"NANO_A0", NANO_A0},
    {"NANO_A1", NANO_A1},
    {"NANO_A2", NANO_A2},
    {"NANO_A3", NANO_A3},
    {"NANO_A4", NANO_A4},
    {"NANO_A5", NANO_A5},
    {"NANO_A6", NANO_A6},
    {"NANO_A7", NANO_A7},
    {"RESET", NANO_RESET},
    {"AREF", NANO_AREF},
    {"D0", NANO_D0},
    {"D1", NANO_D1},
    {"D2", NANO_D2},
    {"D3", NANO_D3},
    {"D4", NANO_D4},
    {"D5", NANO_D5},
    {"D6", NANO_D6},
    {"D7", NANO_D7},
    {"D8", NANO_D8},
    {"D9", NANO_D9},
    {"D10", NANO_D10},
    {"D11", NANO_D11},
    {"D12", NANO_D12},
    {"D13", NANO_D13},
    {"A0", NANO_A0},
    {"A1", NANO_A1},
    {"A2", NANO_A2},
    {"A3", NANO_A3},
    {"A4", NANO_A4},
    {"A5", NANO_A5},
    {"A6", NANO_A6},
    {"A7", NANO_A7}
  };
