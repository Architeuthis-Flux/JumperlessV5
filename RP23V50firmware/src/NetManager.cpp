// SPDX-License-Identifier: MIT

#include "NetManager.h"
//#include "FileParsing.h"
#include "JumperlessDefines.h"
#include <string.h>
#include <math.h>
#include "MatrixState.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "SafeString.h"
#include <Arduino.h>
#include <EEPROM.h>
#include "LEDs.h"
#include "Graphics.h"
#include "Probing.h"
//#include "SerialWrapper.h"
#include "Highlighting.h"

///#define Serial SerialWrap
// Define a struct that holds both the long and short strings as well as the defined value
// struct DefineInfo {
//     const char* shortName;
//     const char* longName;
//     int defineValue;
// };

// Create an array of these structs for all special defines
const DefineInfo specialDefines[] = {
    {"GND",      "GND",         GND},           // 100
    {"TOP_R",    "TOP_RAIL",    TOP_RAIL},      // 101
    {"BOT_R",    "BOTTOM_RAIL", BOTTOM_RAIL},   // 102
    {"3V3",      "SUPPLY_3V3",  SUPPLY_3V3},    // 103
    {"TOP_GND",  "TOP_GND",     TOP_RAIL_GND},  // 104
    {"5V",       "SUPPLY_5V",   SUPPLY_5V},     // 105
    {"DAC_0",    "DAC0",        DAC0},          // 106
    {"DAC_1",    "DAC1",        DAC1},          // 107
    {"I_POS",    "ISENSE_PLUS", ISENSE_PLUS},   // 108
    {"I_NEG",    "ISENSE_MINUS",ISENSE_MINUS},  // 109
    {"ADC_0",    "ADC0",        ADC0},          // 110
    {"ADC_1",    "ADC1",        ADC1},          // 111
    {"ADC_2",    "ADC2",        ADC2},          // 112
    {"ADC_3",    "ADC3",        ADC3},          // 113
    {"ADC_4",    "ADC4",        ADC4},          // 114
    {"ADC_7",    "ADC7",        ADC7},          // 115
    {"UART_Tx",  "RP_UART_Tx",  RP_UART_TX},    // 116
    {"UART_Rx",  "RP_UART_Rx",  RP_UART_RX},    // 117
    {"GP_18",    "RP_GPIO_18",  RP_GPIO_18},    // 118
    {"GP_19",    "RP_GPIO_19",  RP_GPIO_19},    // 119
    {"8V_P",     "8V_POS",      SUPPLY_8V_P},   // 120
    {"8V_N",     "8V_NEG",      SUPPLY_8V_N},   // 121
    {"NONE",     "NONE",        122},          // 122
    {"NONE",     "NONE",        123},          // 123
    {"NONE",     "NONE",        124},          // 124
    {"NONE",     "NONE",        125},          // 125
    {"BOT_GND",  "BOTTOM_GND",  BOTTOM_RAIL_GND}, // 126
    {"EMPTY",    "EMPTY_NET",   EMPTY_NET},     // 127
    {"LOGO_T",   "LOGO_TOP",    LOGO_PAD_TOP},  // 142
    {"LOGO_B",   "LOGO_BOTTOM", LOGO_PAD_BOTTOM}, // 143
    {"GPIO_PAD", "GPIO_PAD",    GPIO_PAD},      // 144
    {"DAC_PAD",  "DAC_PAD",     DAC_PAD},       // 145
    {"ADC_PAD",  "ADC_PAD",     ADC_PAD},       // 146
    {"BLDG_TOP", "BUILDING_TOP", BUILDING_PAD_TOP}, // 147
    {"BLDG_BOT", "BUILDING_BOT", BUILDING_PAD_BOTTOM}, // 148
    {"GP_1",     "RP_GPIO_1",   RP_GPIO_1},     // 131
    {"GP_2",     "RP_GPIO_2",   RP_GPIO_2},     // 132
    {"GP_3",     "RP_GPIO_3",   RP_GPIO_3},     // 133
    {"GP_4",     "RP_GPIO_4",   RP_GPIO_4},     // 134
    {"GP_5",     "RP_GPIO_5",   RP_GPIO_5},     // 135
    {"GP_6",     "RP_GPIO_6",   RP_GPIO_6},     // 136
    {"GP_7",     "RP_GPIO_7",   RP_GPIO_7},     // 137
    {"GP_8",     "RP_GPIO_8",   RP_GPIO_8},     // 138
    {"BUF_IN",   "BUFFER_IN",   ROUTABLE_BUFFER_IN}, // 139
    {"BUF_OUT",  "BUFFER_OUT",  ROUTABLE_BUFFER_OUT} // 140
  };

// Similarly, create an array for Nano defines
const DefineInfo nanoDefines[] = {
    {"VIN",      "NANO_VIN",    NANO_VIN},      // 69
    {"D0",       "NANO_D0",     NANO_D0},       // 70
    {"D1",       "NANO_D1",     NANO_D1},       // 71
    {"D2",       "NANO_D2",     NANO_D2},       // 72
    {"D3",       "NANO_D3",     NANO_D3},       // 73
    {"D4",       "NANO_D4",     NANO_D4},       // 74
    {"D5",       "NANO_D5",     NANO_D5},       // 75
    {"D6",       "NANO_D6",     NANO_D6},       // 76
    {"D7",       "NANO_D7",     NANO_D7},       // 77
    {"D8",       "NANO_D8",     NANO_D8},       // 78
    {"D9",       "NANO_D9",     NANO_D9},       // 79
    {"D10",      "NANO_D10",    NANO_D10},      // 80
    {"D11",      "NANO_D11",    NANO_D11},      // 81
    {"D12",      "NANO_D12",    NANO_D12},      // 82
    {"D13",      "NANO_D13",    NANO_D13},      // 83
    {"RESET",    "NANO_RESET",  NANO_RESET},    // 84
    {"AREF",     "NANO_AREF",   NANO_AREF},     // 85
    {"A0",       "NANO_A0",     NANO_A0},       // 86
    {"A1",       "NANO_A1",     NANO_A1},       // 87
    {"A2",       "NANO_A2",     NANO_A2},       // 88
    {"A3",       "NANO_A3",     NANO_A3},       // 89
    {"A4",       "NANO_A4",     NANO_A4},       // 90
    {"A5",       "NANO_A5",     NANO_A5},       // 91
    {"A6",       "NANO_A6",     NANO_A6},       // 92
    {"A7",       "NANO_A7",     NANO_A7},       // 93
    {"RST0",     "NANO_RST0",   NANO_RESET_0},  // 94
    {"RST1",     "NANO_RST1",   NANO_RESET_1},  // 95
    {"N_GND1",   "NANO_N_GND1", NANO_GND_1},    // 96
    {"N_GND0",   "NANO_N_GND0", NANO_GND_0},    // 97
    {"NANO_3V3", "NANO_3V3",    NANO_3V3},      // 98
    {"NANO_5V",  "NANO_5V",     NANO_5V}        // 99
  };

int16_t newNode1 = -1;
int16_t newNode2 = -1;

int foundNode1Net =
0; // netNumbers where that node is, a node can only be in 1 net (except
// current sense, we'll deal with that separately)
int foundNode2Net =
0; // netNumbers where that node is, a node can only be in 1 net (except
// current sense, we'll deal with that separately)

int foundNode1inSpecialNet = foundNode1Net;
int foundNode2inSpecialNet = foundNode2Net;

// struct pathStruct path[MAX_BRIDGES]; // node1, node2, net, chip[3], x[3],
// y[3]
int newBridge[MAX_BRIDGES][3]; // node1, node2, net
int newBridgeLength = 0;
int newBridgeIndex = 0;
unsigned long timeToNM;

bool debugNM = EEPROM.read(DEBUG_NETMANAGERADDRESS);
bool debugNMtime = EEPROM.read(TIME_NETMANAGERADDRESS);

void getNodesToConnect() // read in the nodes you'd like to connect
  {

  timeToNM = millis();

  if (debugNM)
    Serial.println("\n\n\rconnecting nodes into nets\n\r");

  for (int i = 0; i < newBridgeLength; i++) {
    newNode1 = path[i].node1;

    newNode2 = path[i].node2;

    if (debugNM)
      printNodeOrName(newNode1);
    if (debugNM)
      Serial.print("-");
    if (debugNM)
      printNodeOrName(newNode2);
    if (debugNM)
      Serial.print("\n\r");

    if (newNode1 <= 0 || newNode2 <= 0) {
      path[i].net = -1;
      } else {
      searchExistingNets(newNode1, newNode2);
      }
    // printBridgeArray();

    newBridgeIndex++; // don't increment this until after the search because
    // we're gonna use it as an index to store the nets
// if (i < 7)
// {
    if (debugNM)
      // listSpecialNets();
      // }

      if (debugNM) {
        // listNets();
        }
    }
  if (debugNM)
    Serial.println("done");

  //  sortPathsByNet();
  }

int searchExistingNets(
    int node1,
    int node2) // search through existing nets for all nodes that match either
  // one of the new nodes (so it will be added to that net)
  {

  foundNode1Net = 0;
  foundNode2Net = 0;

  foundNode1inSpecialNet = node1;
  foundNode2inSpecialNet = node2;

  for (int i = 1; i < MAX_NETS; i++) {
    if (net[i].number <= 0) // stops searching if it gets to an unallocated net
      {
      break;
      }

    for (int j = 0; j < MAX_NODES; j++) {
      if (net[i].nodes[j] <= 0) {
        break;
        }

      if (net[i].nodes[j] == node1) {
        if (i > 7) {
          if (debugNM)
            Serial.print("found Node ");
          if (debugNM)
            printNodeOrName(node1);
          if (debugNM)
            Serial.print(" in Net ");
          if (debugNM)
            Serial.println(i);
          }

        if (net[i].specialFunction > 0) {
          foundNode1Net = i;
          foundNode1inSpecialNet = i;
          } else {
          foundNode1Net = i;
          }
        }
      if (net[i].nodes[j] == node2) {
        if (i > 7) {
          if (debugNM)
            Serial.print("found Node ");
          if (debugNM)
            printNodeOrName(node2);
          if (debugNM)
            Serial.print(" in Net ");
          if (debugNM)
            Serial.println(i);
          }

        if (net[i].specialFunction > 0) {
          foundNode2Net = i;
          foundNode2inSpecialNet = i;
          } else {
          foundNode2Net = i;
          }
        }
      }
    }

  if (foundNode1Net == foundNode2Net &&
      foundNode1Net >
          0) // if both nodes are in the same net, still add the bridge
    {

    addNodeToNet(foundNode1Net,
                 node1); // note that they both connect to node1's net
    addNodeToNet(foundNode1Net, node2);
    addBridgeToNet(foundNode1Net, node1, node2);
    path[newBridgeIndex].net = foundNode1Net;
    return 1;
    } else if ((foundNode1Net > 0 && foundNode2Net > 0) &&
               (foundNode1Net != foundNode2Net)) // if both nodes are in different
      // nets, combine them
      {
      if (foundNode1Net > 5 || foundNode2Net > 5) {
        combineNets(foundNode1Net, foundNode2Net);
        }
      return 2;
      } else if (foundNode1Net > 0 && node2 > 0) // if node1 is in a net and node2
        // is not, add node2 to node1's net
        {
        if (checkDoNotIntersectsByNode(foundNode1Net, node2) == 1) {
          if (debugNM)
            Serial.print("adding Node ");
          if (debugNM)
            printNodeOrName(node2);
          if (debugNM)
            Serial.print(" to Net ");
          if (debugNM)
            Serial.println(foundNode1Net);

          addNodeToNet(foundNode1Net, node2);
          addBridgeToNet(foundNode1Net, node1, node2);
          path[newBridgeIndex].net = foundNode1Net;
          } else {
          createNewNet();
          }

        return 3;
        } else if (foundNode2Net > 0 && node1 > 0) // if node2 is in a net and node1
          // is not, add node1 to node2's net
          {
          if (checkDoNotIntersectsByNode(foundNode2Net, node1) == 1) {
            if (debugNM)
              Serial.print("adding Node ");
            if (debugNM)
              printNodeOrName(node1);
            if (debugNM)
              Serial.print(" to Net ");
            if (debugNM)
              Serial.println(foundNode2Net);

            addNodeToNet(foundNode2Net, node1);
            addBridgeToNet(foundNode2Net, node1, node2);
            path[newBridgeIndex].net = foundNode2Net;
            } else {
            createNewNet();
            }
          return 4;
          }

        else {

    createNewNet(); // if neither node is in a net, create a new one

    return 0;
    }
  }

void combineNets(int foundNode1Net, int foundNode2Net) {
  // Serial.println("combineNets");
  // Serial.println(foundNode1Net);
  // Serial.println(foundNode2Net); ///still need to fix this


  if (checkDoNotIntersectsByNet(foundNode1Net, foundNode2Net) == 1) {
    int swap = 0;
    if ((foundNode2Net <= 5 && foundNode1Net <= 5)) {

      for (int i = 0; i < MAX_DNI; i++) {
        for (int j = 0; j < MAX_DNI; j++) {
          if ((net[foundNode1Net].doNotIntersectNodes[i] == foundNode1Net ||
               net[foundNode2Net].doNotIntersectNodes[j] == foundNode2Net)) {//&&
            //(net[foundNode1Net].doNotIntersectNodes[i] != -1 &&
            // net[foundNode2Net].doNotIntersectNodes[j] != -1)) {

            if (debugNM)
              Serial.print(
                  "can't combine Speeeeeeeecial Nets\n\r"); // maybe have it add a
            // bridge between them if
            // it's allowed?

            path[newBridgeIndex].net = -1;
            return;
            }
          }
        }
      addNodeToNet(foundNode1Net, newNode2);
      addNodeToNet(foundNode2Net, newNode1);
      addBridgeToNet(foundNode1Net, newNode1, newNode2);
      addBridgeToNet(foundNode2Net, newNode1, newNode2);

      if (debugNM)
        Serial.print(
            "can't combine Specccccccial Nets\n\r"); // maybe have it add a bridge
      // between them if it's allowed?

      path[newBridgeIndex].net = -1;
      } else {

      if (foundNode2Net <= 5) {
        swap = foundNode1Net;
        foundNode1Net = foundNode2Net;
        foundNode2Net = swap;
        }
      addNodeToNet(foundNode1Net, newNode1);
      addNodeToNet(foundNode1Net, newNode2);
      addBridgeToNet(foundNode1Net, newNode1, newNode2);
      path[newBridgeIndex].net = foundNode1Net;
      if (debugNM)
        Serial.print("combining Nets ");
      if (debugNM)
        Serial.print(foundNode1Net);
      if (debugNM)
        Serial.print(" and ");
      if (debugNM)
        Serial.println(foundNode2Net);

      for (int i = 0; i < MAX_NODES; i++) {
        if (net[foundNode2Net].nodes[i] == 0) {
          break;
          }

        addNodeToNet(foundNode1Net, net[foundNode2Net].nodes[i]);
        }

      for (int i = 0; i < MAX_BRIDGES; i++) {
        if (net[foundNode2Net].bridges[i][0] == 0) {
          break;
          }

        addBridgeToNet(foundNode1Net, net[foundNode2Net].bridges[i][0],
                       net[foundNode2Net].bridges[i][1]);
        }
      for (int i = 0; i < MAX_DNI; i++) {
        if (net[foundNode2Net].doNotIntersectNodes[i] == 0) {
          break;
          }

        addNodeToNet(foundNode1Net, net[foundNode2Net].doNotIntersectNodes[i]);
        }
      for (int i = 0; i < newBridgeIndex;
           i++) // update the newBridge array to reflect the new net number
        {
        if (path[i].net == foundNode2Net) {
          if (debugNM)
            Serial.print("updating path[");
          if (debugNM)
            Serial.print(i);
          if (debugNM)
            Serial.print("].net from ");
          if (debugNM)
            Serial.print(path[i].net);
          if (debugNM)
            Serial.print(" to ");
          if (debugNM)
            Serial.println(foundNode1Net);

          path[i].net = foundNode1Net;
          }
        }
      if (debugNM)
        printBridgeArray();

      deleteNet(foundNode2Net);
      }
    }
  }

/// @brief checks if a bridge exists between two nodes
/// @param node1 
/// @param node2 can be -1 if you only want to check if node1 exists in at all
/// @return 1 if the bridge exists, 0 if it doesn't
int checkIfBridgeExistsLocal(int node1, int node2) {

  for (int i = 1; i < MAX_NETS; i++) {
    if (net[i].number <= 0) {
      break;
      }
    for (int j = 0; j < MAX_NODES; j++) {
      if (net[i].nodes[j] <= 0) {
        break;
        }
      if (net[i].nodes[j] == node1) {
        if (node2 == -1) {
          return 1;
          }
        for (int k = 0; k < MAX_NODES; k++) {
          if (net[i].nodes[k] == node2) {
            return 1;
            }

          }
        }
      }
    }
  return 0;
  }

void deleteNet(int netNumber) // make sure to check special function nets and
// clear connections to it
  {
  shiftNets(netNumber);
  }

int shiftNets(
    int deletedNet) // why in the ever-loving fuck does this work? there's no
  // recursion but somehow it moves all the nets
  {
  int lastNet;

  for (int i = MAX_NETS - 2; i > 0; i--) {
    if (net[i].number != 0) {
      lastNet = i;
      // if(debugNM) Serial.print("last net = ");
      // if(debugNM) Serial.println(lastNet);
      break;
      }
    }
  if (debugNM)
    Serial.print("deleted Net ");
  if (debugNM)
    Serial.println(deletedNet);

  for (int i = deletedNet; i < lastNet; i++) {
    net[i] = net[i + 1];
    net[i].name = netNameConstants[i];
    net[i].number = i;
    }

  net[lastNet].number = 0;
  net[lastNet].name = "       "; // netNameConstants[lastNet];
  net[lastNet].visible = 0;
  net[lastNet].specialFunction = -1;

  for (int i = 0; i < 6; i++) {
    net[lastNet].intersections[i] = 0;
    net[lastNet].doNotIntersectNodes[i] = 0;
    }
  for (int j = 0; j < MAX_NODES; j++) {
    if (net[lastNet].nodes[j] == 0) {
      break;
      }

    net[lastNet].nodes[j] = 0;
    }

  for (int j = 0; j < MAX_BRIDGES; j++) {
    if (net[lastNet].bridges[j][0] == 0) {
      break;
      }

    net[lastNet].bridges[j][0] = 0;
    net[lastNet].bridges[j][1] = 0;
    }
  return lastNet;
  }

int findNodeInNet(int node) {
  // Serial.println("findNodeInNet");
  // Serial.print("node = ");
  //   Serial.println(node);
  for (int i = 1; i < MAX_NETS; i++) {

    for (int j = 0; j < MAX_NODES; j++) {
      if (net[i].nodes[j] == -1) {
        // continue; 
        break;
        }

      if (net[i].nodes[j] == node) {
        // Serial.print("found node ");
        // Serial.print(node);
        // Serial.print(" in net ");
        // Serial.println(net[i].number);
        return net[i].number;
        }
      }
    }
  for (int i = 0; i < 10; i++) {
    if (gpioNet[i] == node) {
      //         Serial.print("found node ");
      // Serial.print(node);
      // Serial.print(" in net ");
      // Serial.println(net[i].number);
      return gpioNet[i];
      }
    }

  for (int i = 0; i < 8; i++) {
    if (showADCreadings[i] == node) {
      return showADCreadings[i];
      }
    }


  return -1;
  }

void createNewNet() // add those nodes to a new net
  {
  int newNetNumber = findFirstUnusedNetIndex();
  net[newNetNumber].number = newNetNumber;

  net[newNetNumber].name =
    netNameConstants[newNetNumber]; // dont need a function for this anymore

  net[newNetNumber].specialFunction = -1;

  addNodeToNet(newNetNumber, newNode1);

  addNodeToNet(newNetNumber, newNode2);

  addBridgeToNet(newNetNumber, newNode1, newNode2);

  path[newBridgeIndex].net = newNetNumber;
  }

void addBridgeToNet(uint16_t netToAddBridge, int16_t node1,
                    int16_t node2) // just add those nodes to the net
  {
  int newBridgeIndex = findFirstUnusedBridgeIndex(netToAddBridge);
  net[netToAddBridge].bridges[newBridgeIndex][0] = node1;
  net[netToAddBridge].bridges[newBridgeIndex][1] = node2;
  }

void populateSpecialFunctions(int net, int node) {
  int foundGPIO = 0;
  // Serial.println("populating special functions\n\r");
  // Serial.print("node = ");
  //   Serial.println(node);
  // for (int i = 0; i < 10; i++) {
  //   if (gpioNet[i] != -2) {
  //     gpioNet[i] = -1;
  //     }
  //   }

  switch (node) {
    case RP_GPIO_1:
      if (gpioNet[0] != -2) {
        gpioNet[0] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_2:
      if (gpioNet[1] != -2) {
        gpioNet[1] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_3:
      if (gpioNet[2] != -2) {
        gpioNet[2] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_4:
      if (gpioNet[3] != -2) {
        gpioNet[3] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_5:
      if (gpioNet[4] != -2) {
        gpioNet[4] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_6:
      if (gpioNet[5] != -2) {
        gpioNet[5] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_7:
      if (gpioNet[6] != -2) {
        gpioNet[6] = net;
        foundGPIO = 1;
        }
      break;
    case RP_GPIO_8:
      if (gpioNet[7] != -2) {
        gpioNet[7] = net;
        foundGPIO = 1;
        }
      break;
    case RP_UART_TX:
      if (gpioNet[8] != -2) {
        gpioNet[8] = net;
        foundGPIO = 1;
        }
      break;
    case RP_UART_RX:
      if (gpioNet[9] != -2) {
        gpioNet[9] = net;
        foundGPIO = 1;
        }
      break;

    }
  if (foundGPIO == 1) {
    //     for (int i = 0; i < 8; i++)
    //     {
    // // Serial.print("gpioNet[");
    // // Serial.print(i);
    // // Serial.print("] = ");
    // // Serial.println(gpioNet[i]);

    //     }
    }
  }
void addNodeToNet(int netToAddNode, int node) {
  int newNodeIndex = findFirstUnusedNodeIndex(
      netToAddNode); // using a function lets us add more error checking later
  // and maybe shift the nodes down so they're left justified
  populateSpecialFunctions(netToAddNode, node);
  for (int i = 0; i < MAX_NODES; i++) {
    if (net[netToAddNode].nodes[i] == 0) {
      break;
      }

    if (net[netToAddNode].nodes[i] == node) {
      if (debugNM)
        Serial.print("Node ");
      if (debugNM)
        printNodeOrName(node);
      if (debugNM)
        Serial.print(" is already in Net ");
      if (debugNM)
        Serial.print(netToAddNode);
      if (debugNM)
        Serial.print(", still adding to net\n\r");
      return;
      // break;
      }
    }

  net[netToAddNode].nodes[newNodeIndex] = node;
  }

int findFirstUnusedNetIndex() // search for a free net[]
  {
  for (int i = 0; i < MAX_NETS; i++) {
    if (net[i].nodes[0] <= 0) {
      if (debugNM)
        Serial.print("found unused Net ");
      if (debugNM)
        Serial.println(i);

      return i;
      break;
      }
    }
  return 0x7f;
  }

int findFirstUnusedBridgeIndex(int netNumber) {
  for (int i = 0; i < MAX_BRIDGES; i++) {
    if (net[netNumber].bridges[i][0] == 0) {
      // if(debugNM) Serial.print("found unused bridge ");
      // if(debugNM) Serial.println(i);

      return i;
      break;
      }
    }
  return 0x7f;
  }

int findFirstUnusedNodeIndex(int netNumber) // search for a free net[]
  {
  for (int i = 0; i < MAX_NODES; i++) {
    if (net[netNumber].nodes[i] == 0) {
      // if(debugNM) Serial.printf("found unused node index net[%d].
      // node[%d]\n\r", netNumber, i);
      //  if(debugNM) Serial.println(i);

      return i;
      break;
      }
    }
  return 0x7f;
  }

/// @brief checks if a connection is allowed by checking doNotIntersect rules
/// @param netToCheck1 the first net
/// @param netToCheck2 the second net
/// @return true if the connection is allowed, false otherwise
int checkDoNotIntersectsByNet(int netToCheck1, int netToCheck2) // If you're searching DNIs by net, there
// won't be any valid ways to make a new
// net with both nodes, so its skipped
  {

  for (int i = 0; i <= MAX_DNI; i++) {
    if (net[netToCheck1].doNotIntersectNodes[i] == 0) {
      break;
      }

    for (int j = 0; j <= MAX_NODES;
         j++) {

      if (debugNM) Serial.print
      (net[netToCheck1].doNotIntersectNodes[i]);
      if (debugNM) Serial.print("-");
      if (debugNM) Serial.println(net[netToCheck2].nodes[j]);

      if (net[netToCheck2].nodes[j] == 0) {
        break;
        }

      if (net[netToCheck1].doNotIntersectNodes[i] ==
          net[netToCheck2].nodes[j]) {
        if (debugNM)
          Serial.print("Net ");
        if (debugNM)
          printNodeOrName(netToCheck2);
        if (debugNM)
          Serial.print(" can't be combined with Net ");
        if (debugNM)
          Serial.print(netToCheck1);
        if (debugNM)
          Serial.print(" due to Do Not Intersect rules, skipping\n\r");
        path[newBridgeIndex].skip = true;
        path[newBridgeIndex].net = -1;
        return 0;
        }
      }
    // if(debugNM) Serial.println (" ");
    }

  for (int i = 0; i <= MAX_DNI; i++) {
    if (net[netToCheck2].doNotIntersectNodes[i] == 0) {
      break;
      }

    for (int j = 0; j <= MAX_NODES; j++) {
      if (net[netToCheck1].nodes[j] == 0) {
        break;
        }

      if (net[netToCheck2].doNotIntersectNodes[i] ==
          net[netToCheck1].nodes[j]) {
        if (debugNM)
          Serial.print("Net ");
        printNodeOrName(netToCheck2);
        if (debugNM)
          Serial.print(" can't be combined with Net ");
        if (debugNM)
          Serial.print(netToCheck1);
        if (debugNM)
          Serial.print(" due to Do Not Intersect rules, skipping\n\r");
        path[newBridgeIndex].net = -1;
        return 0;
        }
      }
    }

  return 1; // return 1 if it's ok to connect these nets
  }

/// @brief checks if a connection is allowed by checking doNotIntersect rules
/// @param netToCheck the net to check
/// @param nodeToCheck the node to check
/// @return true if the connection is allowed, false otherwise
int checkDoNotIntersectsByNode(
    int netToCheck,
    int nodeToCheck) // make sure none of the nodes on the net violate
  // doNotIntersect rules, exit and warn
  {

  for (int i = 0; i < MAX_DNI; i++) {
    if (net[netToCheck].doNotIntersectNodes[i] == 0) {
      break;
      }

    if (net[netToCheck].doNotIntersectNodes[i] == nodeToCheck) {
      if (debugNM)
        Serial.print("Node ");
      if (debugNM)
        printNodeOrName(nodeToCheck);
      if (debugNM)
        Serial.print(" is not allowed on Net ");
      if (debugNM)
        Serial.print(netToCheck);
      if (debugNM)
        Serial.print(
            " due to Do Not Intersect rules, a new net will be created\n\r");
      return 0;
      }
    }

  return 1; // return 1 if it's ok to connect these nets
  }
int floatingTermColors[10] = { 203, 215, 221, 192, 117, 75, 176,213, 180, 147 };

int railTermColors[5] = { 48, 197, 199, 222, 116 };


void assignTermColor(void) {
#ifdef TERM_COLOR_NETS


  for (int i = 1; i < 6; i++) {
    net[i].termColor = railTermColors[i - 1];
    // changeTerminalColor(net[i].termColor);
    // Serial.print("net[");
    // Serial.print(i);
    // Serial.print("].termColor = ");
    // Serial.println(net[i].termColor);
    // changeTerminalColor();
    }

  for (int i = 6; i < numberOfNets; i++) {
    if (net[i].nodes[0] > 0) {
      net[i].termColor = colorToVT100(packRgb(netColors[i]));
      }
    // changeTerminalColor(net[i].termColor);
    // Serial.print("net[");
    // Serial.print(i);
    // Serial.print("].termColor = ");
    // Serial.println(net[i].termColor);
    // changeTerminalColor();
    }
#endif

  }




/// @brief list all nets
/// @param liveUpdate 0 = no live update, 1 = live update
void listNets(int liveUpdate)
  {
  // Serial.print("liveUpdate: ");
  // Serial.println(liveUpdate);
  int boldNode = highlightedRow;
  int boldNet = highlightedNet;

  int lastGPIO[10];
  float lastADC[8];
  for (int i = 0; i < 10; i++) {
    lastGPIO[i] = gpioReading[i];
    }
  for (int i = 0; i < 8; i++) {
    lastADC[i] = adcReadings[i];
    }


  if (liveUpdate < 0) {
    liveUpdate = 0;
    } else if (liveUpdate >= 0) {
      liveUpdate = 1;
      }

    // Serial.print("liveUpdate: ");
    // Serial.println(liveUpdate);

    ///0 = none, 1 = adc, 2 = gpio input, 3 = gpio output, 4 = uart tx, 5 = uart rx, 6 = other
    int netsShowingSpecial[MAX_NETS];
    // int netsShowingSpecialIndex = 0;
    int showVoltage = 0;
    int showGPIO = 0;
    for (int i = 0; i <= numberOfNets; i++) {
      netsShowingSpecial[i] = 0;
      }
    // First scan all nets to check for ADC and GPIO nodes
    for (int i = 6; i <= numberOfNets; i++) {
      if (net[i].nodes[0] <= 0) continue; // Skip empty nets

      for (int j = 0; j < MAX_NODES; j++) {
        if (net[i].nodes[j] <= 0) break; // End of nodes for this net

        // Check for ADC nodes (ADC0-ADC7)
        // Values from JumperlessDefines.h: ADC0(110), ADC1(111), ADC2(112), ADC3(113), ADC4(114), ADC7(115)
        if ((net[i].nodes[j] >= ADC0 && net[i].nodes[j] <= ADC4) ||
            net[i].nodes[j] == ADC7) {
          // Serial.print("adc  ");
          // Serial.println(net[i].nodes[j]);
          showVoltage = 1;
          netsShowingSpecial[i] = 1;
          }

        // Check for GPIO nodes (RP_GPIO_1-RP_GPIO_8)
        // Values from JumperlessDefines.h: RP_GPIO_1(131)-RP_GPIO_4(134), RP_GPIO_5(135)-RP_GPIO_8(138)
        if ((net[i].nodes[j] >= RP_GPIO_1 && net[i].nodes[j] <= RP_GPIO_4) ||
            (net[i].nodes[j] >= RP_GPIO_5 && net[i].nodes[j] <= RP_GPIO_8)) {
          // Serial.print("gpio");
          // Serial.println(net[i].nodes[j]);
          showGPIO = 1;
          if (gpioState[net[i].nodes[j] - RP_GPIO_1] == 0) {
            netsShowingSpecial[i] = 3;
            } else {
            netsShowingSpecial[i] = 2;
            }
          }

        // Check for UART pins which are also GPIO pins
        if (net[i].nodes[j] == RP_UART_TX) {
          showGPIO = 1;
          netsShowingSpecial[i] = 4;
          }
        if (net[i].nodes[j] == RP_UART_RX) {
          showGPIO = 1;
          netsShowingSpecial[i] = 5;
          }

        // Early exit if we found both types
        // if (showVoltage && showGPIO) break;
        }

      // Early exit from outer loop if we found both types
      // if (showVoltage && showGPIO) break;
      }
    int maxNodes = 0;
    int maxChars = 0;

    for (int i = 6; i < numberOfNets; i++) {
      if (net[i].nodes[0] > 0) {
        int nodeCount = 0;
        int charCount = 0;
        // Count all nodes in this net
        for (int j = 0; j < MAX_NODES; j++) {
          if (net[i].nodes[j] > 0) {
            nodeCount++;
            // Calculate characters for this node using definesToChar
            if (j > 0) {
              charCount += 1; // Add 1 for comma
              }

            // Get character length based on node value
            if (net[i].nodes[j] >= 100) {
              charCount += strlen(definesToChar(net[i].nodes[j], 0)); // Short form
              } else if (net[i].nodes[j] >= NANO_D0) {
                charCount += strlen(definesToChar(net[i].nodes[j], 0)); // Short form
                } else {
                // For numeric nodes, calculate digits
                int node = net[i].nodes[j];
                if (node == 0) charCount += 1;
                else {
                  int digits = 0;
                  while (node > 0) {
                    digits++;
                    node /= 10;
                    }
                  charCount += digits;
                  }
                }
            } else {
            // Once we hit a zero, we've reached the end of nodes for this net
            break;
            }
          }
        // Update maxNodes if this net has more nodes
        if (nodeCount > maxNodes) {
          maxNodes = nodeCount;
          }
        // Update maxChars if this net has more characters
        if (charCount > maxChars) {
          maxChars = charCount;
          }
        }
      }

    // Serial.print("maxNodes: ");
    // Serial.println(maxNodes);
    // Serial.print("maxChars: ");
    // Serial.println(maxChars);
    // Serial.print("showVoltage: ");
    // Serial.println(showVoltage);
    // Serial.print("showGPIO: ");
    // Serial.println(showGPIO);



    int lineCount = 0;

    // if (liveUpdate == 1) {
    //   Serial.println("\tlive update mode");
    //   Serial.println("\t  (send any character to exit)");
    //   lineCount+=2;
    //   }

    Serial.print("\n\rIndex\tName\t\tVoltage\t    Nodes\t\n\r");




    do {



      int tabs = 0;

      int showingSpecial = 0;

      for (int i = 1; i < numberOfNets; i++) {

        int floatingTermColor = -1;

        if (i == 6) {
          Serial.printf("\033[0m");
          Serial.print("\n\rIndex\tName\t\tColor\t    Nodes");

          lineCount += 2;

          if (showGPIO == 1 && showVoltage == 1) {
            for (int i = 0; i < maxChars; i++) {
              Serial.print(" ");
              }
            Serial.print("ADC / GPIO");
            }

          else if (showVoltage == 1) {
            for (int i = 0; i < maxChars; i++) {
              Serial.print(" ");
              }
            Serial.print(" Voltage");
            }


          else if (showGPIO == 1) {
            for (int i = 0; i < maxChars; i++) {
              Serial.print(" ");
              }
            Serial.print(" GPIO");
            }

          Serial.println(" ");
          lineCount += 1;
          }
        for (int j = 0; j < 10; j++) {
          if (gpioNet[j] == i) {
            floatingTermColor = floatingTermColors[j];
            Serial.printf("\033[38;5;%dm", floatingTermColors[j]);
            break;
            }
          }

        if (floatingTermColor == -1 && i >= 6) {
          Serial.printf("\033[38;5;%dm", colorToVT100(packRgb(netColors[i])));
          } else if (floatingTermColor == -1 && i < 6) {
            Serial.printf("\033[38;5;%dm", railTermColors[i - 1]);
            }

          if (net[i].number == 0 ||
              net[i].nodes[0] ==
                  -1) // stops searching if it gets to an unallocated net
            {
            // Serial.print("Done listing nets");
            break;
            }

          int gpioOrAdcNumber = 0;

          if (netsShowingSpecial[i] != 0) {
            for (int j = 0; j < MAX_NODES; j++) {
              if (net[i].nodes[j] > 0) {
                for (int k = 0; k < 8; k++) {
                  if (net[i].nodes[j] == ADC0 + k || net[i].nodes[j] == RP_GPIO_1 + k) {
                    gpioOrAdcNumber = k;
                    break;
                    }
                  }
                break;
                }
              }
            }

          // if (gpioOrAdcNumber != 0) {
          //   Serial.println(gpioOrAdcNumber);
          // }



       // Serial.print("\n\r ");
          Serial.print(i);
          Serial.print("\t ");
          int netNameLength = Serial.print(net[i].name);
          if (netNameLength < 8) {
            Serial.print("\t");
            }
          Serial.print("\t ");

          if (netsShowingSpecial[i] == 2 || netsShowingSpecial[i] == 3) {
            Serial.print("\b\b* ");
            if (gpioReading[gpioOrAdcNumber] == 0) {

              if (TERM_SUPPORTS_RGB == 0 && TERM_SUPPORTS_ANSI_COLORS == 1)
                {
                Serial.printf("\033[38;5;%dm%s", colorToVT100(packRgb(netColors[i])), "green  - l");
                } else if (TERM_SUPPORTS_RGB == 1 && TERM_SUPPORTS_ANSI_COLORS == 1)
                  {
                  Serial.printf("\033[38;2;0;255;0m%s\033[0m", "green  - l");
                  } else {
                  Serial.print("green  - l");
                  }

              } else if (gpioReading[gpioOrAdcNumber] == 1) {
                if (TERM_SUPPORTS_RGB == 0 && TERM_SUPPORTS_ANSI_COLORS == 1)
                  {
                  Serial.printf("\033[38;5;%dm%s", colorToVT100(packRgb(netColors[i])), "red    - h");
                  } else if (TERM_SUPPORTS_RGB == 1 && TERM_SUPPORTS_ANSI_COLORS == 1)
                    {
                    Serial.printf("\033[38;2;255;0;0m%s", "red    - h");
                    } else {
                    Serial.print("red    - h");
                    }
                } else {
                int length = 0;
                if (TERM_SUPPORTS_RGB == 0 && TERM_SUPPORTS_ANSI_COLORS == 1)
                  {
                  hsvColor hsv;
                  hsv.h = gpioAnimationBaseHues[gpioOrAdcNumber];
                  hsv.s = 20;
                  hsv.v = 255;

                  uint32_t color = HsvToRaw(hsv);
                  length = Serial.printf("\033[38;5;%dm%-7s- f", floatingTermColors[gpioOrAdcNumber], colorToName(gpioAnimationBaseHues[gpioOrAdcNumber], -1));

                  } else if (TERM_SUPPORTS_RGB == 1 && TERM_SUPPORTS_ANSI_COLORS == 1)
                    {
                    length = Serial.printf("\033[38;2;255;255;30m%s - f", colorToName(gpioAnimationBaseHues[gpioOrAdcNumber], -1));
                    } else {

                    length = Serial.print(colorToName(gpioAnimationBaseHues[gpioOrAdcNumber], -1));
                    length += Serial.print(" - f");
                    }
                  for (int i = 0; i < 10 - length; i++) {
                    Serial.print(" ");
                    }

                }
            } else {


            //itoa(colorToVT100(packRgb(netColors[i])), colorString, 10);
            if (TERM_SUPPORTS_RGB == 0 && TERM_SUPPORTS_ANSI_COLORS == 1)
              {
              if (i < 6) {




                Serial.printf("\033[38;5;%dm", railTermColors[i - 1]);
                int spaces = 0;
                switch (i) {
                  case 1:
                    spaces = Serial.print("0 V       ");
                    break;
                  case 2:
                    spaces = Serial.printf("%-.2f V", railVoltage[0]);

                    break;
                  case 3:
                    spaces = Serial.printf("%-.2f V", railVoltage[1]);

                    break;
                  case 4:
                    spaces = Serial.printf("%-.2f V", dacOutput[0]);
                    break;
                  case 5:
                    spaces = Serial.printf("%-.2f V", dacOutput[1]);
                    break;
                  }

                for (int i = 0; i < 10 - spaces; i++) {
                  Serial.print(" ");
                  }



                } else {
                Serial.printf("\033[38;5;%dm%s", colorToVT100(packRgb(netColors[i])), colorToName(netColors[i], 10));
                }
              } else if (TERM_SUPPORTS_RGB == 1 && TERM_SUPPORTS_ANSI_COLORS == 1)
                {
                hsvColor color = RgbToHsv(netColors[i]);
                color.v = 255;

                rgbColor rgb = HsvToRgb(color);
                char colorString[10];

                char colorR[4];
                char colorG[4];
                char colorB[4];

                itoa(rgb.r, colorR, 10);
                itoa(rgb.g, colorG, 10);
                itoa(rgb.b, colorB, 10);
                // Serial.print(colorR);
                // Serial.print(",");
                // Serial.print(colorG);
                // Serial.print(",");
                // Serial.print(colorB);
                // Serial.print(" ");

                Serial.printf("\033[38;2;%s;%s;%sm%s", colorR, colorG, colorB, colorToName(netColors[i], 10));
                } else {
                Serial.print(colorToName(netColors[i], 10));
                }

              // Serial.print(colorToVT100(packRgb(netColors[i])));
    //lineCount+=1;
              // for (int c = 0; c < 128 ; c++) {
              //   Serial.printf("\033[%dm%d\033[0m", c, c);
              //   Serial.print(" ");
              // }
            }


          Serial.print("  ");

          int showVoltage = 0;

          tabs = 0;
          for (int j = 0; j < MAX_NODES; j++) {
            if (brightenedNode == net[i].nodes[j]) {
              Serial.printf("\033[7m");
              }
            tabs += printNodeOrName(net[i].nodes[j]);
            if (brightenedNode == net[i].nodes[j]) {
              Serial.printf("\033[27m");
              }
            // if (brightenedNode == net[i].nodes[j]) {
            //   if (floatingTermColor != -1) {
            //     Serial.printf("\033[38;5;%dm", floatingTermColors[gpioOrAdcNumber]);
            //     } else {
            //     Serial.printf("\033[38;5;%dm", colorToVT100(packRgb(netColors[i])));
            //     }
              //Serial.printf("\033[0m");



            if (net[i].nodes[j + 1] == 0) {
              break;
              } else {

              tabs += Serial.print(",");
              }
            }

          for (int i = tabs; i < maxChars; i++) {
            Serial.print(" ");
            }
          Serial.print("\t    ");

          if (netsShowingSpecial[i] != 0) {
            if (netsShowingSpecial[i] == 1) {
              float voltage = adcReadings[gpioOrAdcNumber];
              if (voltage < 0.03 && voltage > -0.03) {
                voltage = 0.00;
                }
              if (voltage < 0.0) {
                Serial.print("\b");

                }
              Serial.print(voltage, 2);
              Serial.print(" V");

              } else if (netsShowingSpecial[i] == 2) {
                if (gpioReading[gpioOrAdcNumber] == 0) {
                  Serial.print("input - low");
                  } else if (gpioReading[gpioOrAdcNumber] == 1) {
                    Serial.print("input - high");
                    } else {
                    Serial.print("input - floating");
                    }
                } else if (netsShowingSpecial[i] == 3) {
                  if (gpioState[gpioOrAdcNumber] == 0) {
                    Serial.print("output - high");
                    } else if (gpioState[gpioOrAdcNumber] == 1) {
                      Serial.print("output - low");
                      }
                  }
            }
          // Serial.print("netsShowingSpecial[");
          // Serial.print(i);
          // Serial.print("]: ");
          // Serial.println(netsShowingSpecial[i]);


          tabs = 0;

          // for (int i = 0; i < 3 - (tabs / 8); i++) {
          //   Serial.print("\t");
          //   }
          //Serial.print(changedNetColors[i].color, HEX);
          Serial.println();
          lineCount += 1;
        }


      Serial.printf("\033[0m");


      Serial.flush();



      // if (checkProbeButton() != 0) {
      //   blockProbeButton = 500;
      //   blockProbeButtonTimer = millis();
      //   liveUpdate = 0;
      //   }

        //     Serial.print("liveUpdate = ");
        // Serial.println(liveUpdate);
        // Serial.flush();

      if (liveUpdate == 1) {
        // Serial.println(lineCount);


        int changed = 0;

        unsigned long startTime = millis();
        //Serial.print("\033[2J\033[H");
        while (Serial.available() == 0 && liveUpdate == 1 && changed == 0) {
          //Serial.println("waiting for serial");
          for (int i = 0; i < 10; i++) {
            if (lastGPIO[i] != gpioReading[i]) {
              changed = 1;
              lastGPIO[i] = gpioReading[i];
              }
            }
          for (int i = 0; i < 8; i++) {
            if (fabs(lastADC[i] - adcReadings[i]) > 0.02) {
              changed = 1;
              lastADC[i] = adcReadings[i];
              }
            }
          if (millis() - startTime > 100) {
            if (checkProbeButton() != 0) {
              blockProbeButton = 500;
              blockProbeButtonTimer = millis();
              liveUpdate = 0;
              break;
              }
            if (digitalRead(BUTTON_ENC) == 0) {
              liveUpdate = 0;
              break;
              }
            startTime = millis();
            }

          if (Serial.available() > 0) {
            liveUpdate = 0;
            break;
            }


          // if (digitalRead(BUTTON_ENC) == 0) {
          //   liveUpdate = 0;
          //   break;
          //   }

          int newBoldNode = encoderNetHighlight(0,0);
          if (newBoldNode != boldNode) {
            boldNode = newBoldNode;
            changed = 1;
            }


          }


        if (Serial.available() > 0 || liveUpdate == 0) {
          liveUpdate = 0;
          Serial.println();
          Serial.flush();
          return;
          }





        // Serial.flush();
        //delay(10);
        // for (int i = 0; i < lineCount; i++) {
        if (liveUpdate == 1) {
          Serial.printf("\033[%dA", lineCount - 1);
          //Serial.print("   ffdflkj;ldfkj ");
          Serial.printf("\033[J");
          Serial.flush();
          lineCount = 0;
          }
        } else {
        break;
        }


      //Serial.println("done");



      } while (liveUpdate == 1);

    // while (Serial.available() == 0) {

    Serial.print("\n\r");
    Serial.flush();
    // while (Serial.available() > 0) {
    //   Serial.read();
    //   }
    //  

  }

void listSpecialNets() {
  Serial.print(
      "\n\rIndex\tName\t\tVoltage\t    Nodes\t"); //\t\t\t\tColor\t\tDo
  //Not Intersects");
  int tabs = 0;

  for (int i = 1; i < 6; i++) {
    int spaces = 0;
    if (net[i].number == 0) // stops searching if it gets to an unallocated net
      {
      // Serial.print("Done listing nets");
      break;
      }

    Serial.print("\n\r ");
    Serial.print(net[i].number);
    Serial.print("\t ");

    int netNameLength = Serial.print(net[i].name);
    // if (netNameLength < 8)
    // {
    //     Serial.print("\t");
    // }
    for (int i = 0; i < 8 - netNameLength; i++) {
      Serial.print(" ");
      }

    Serial.print("\t ");

    switch (i) {
      case 1:
        spaces += Serial.print("0V");
        break;
      case 2:
        spaces += Serial.print(railVoltage[0]);
        spaces += Serial.print("V");
        break;
      case 3:
        spaces += Serial.print(railVoltage[1]);
        spaces += Serial.print("V");
        break;
      case 4:
        spaces += Serial.print(dacOutput[0]);
        spaces += Serial.print("V");
        break;
      case 5:
        spaces += Serial.print(dacOutput[1]);
        // for (int i = 0; i < 32; i++)
        // {
        //     uint32_t dacMask = dacOutput[1];
        //     Serial.println(dacMask, BIN);
        // }
        spaces += Serial.print("V");
        break;
      default:
        spaces += Serial.print("N/A");
        // Serial.print("V");
        break;
      }
    // for (int i = 0; i < 8 - spaces; i++) {
    //   Serial.print(" ");
    // }
    // Serial.print("   ");

    // Serial.print("r");
    // if (net[i].color.r < 16) {
    //   Serial.print("0");
    // }
    // netNameLength = Serial.print(net[i].color.r, HEX);

    // Serial.print(" g");
    // if (net[i].color.g < 16) {
    //   Serial.print("0");
    // }
    // netNameLength = Serial.print(net[i].color.g, HEX);

    // Serial.print(" b");
    // if (net[i].color.b < 16) {
    //   Serial.print("0");
    // }
    // netNameLength = Serial.print(net[i].color.b, HEX);

    // if (netNameLength < 6)
    // {
    //     Serial.print("\t");
    // }
    Serial.print("\t     ");

    tabs = 0;
    for (int j = 0; j < MAX_NODES; j++) {
      tabs += printNodeOrName(net[i].nodes[j]);
      // tabs += Serial.print(definesToChar(net[i].nodes[j]));

      if (net[i].nodes[j + 1] == 0) {
        break;
        } else {

        tabs += Serial.print(",");
        }
      }

    for (int i = 0; i < 3 - (tabs / 8); i++) {
      Serial.print("\t");
      }

    // Serial.print("{");

    // tabs = 0;
    // for (int j = 0; j < MAX_BRIDGES; j++)
    // {

    //     tabs += printNodeOrName(net[i].bridges[j][0]);
    //     tabs += Serial.print("-");
    //     tabs += printNodeOrName(net[i].bridges[j][1]);
    //     // Serial.print(",");

    //     if (net[i].bridges[j + 1][0] == 0)
    //     {
    //         break;
    //     }
    //     else
    //     {

    //         tabs += Serial.print(",");
    //     }
    // }
    // tabs += Serial.print("}\t");

    for (int i = 0; i < 3 - (tabs / 8); i++) {
      Serial.print("\t");
      }
    /*
            Serial.print(net[i].colorName);
    Serial.print("\t\t");

            for (int j = 0; j < MAX_DNI; j++)
            {

                tabs += printNodeOrName(net[i].doNotIntersectNodes[j]);

                if (net[i].doNotIntersectNodes[j + 1] == 0 || i == 0)
                {
                    break;
                }
                else
                {

                    tabs += Serial.print(",");
                }
            }*/
    }
  Serial.print("\n\r");
  }

void printBridgeArray(void) {

  Serial.print("\n\r");
  int tabs = 0;
  int lineCount = 0;
  for (int i = 0; i < numberOfPaths; i++) {
    tabs += Serial.print(i);
    if (i < 10) {
      tabs += Serial.print(" ");
      }
    if (i < 100) {
      tabs += Serial.print(" ");
      }
    tabs += Serial.print("[");
    tabs += printNodeOrName(path[i].node1);
    tabs += Serial.print(",");
    tabs += printNodeOrName(path[i].node2);
    tabs += Serial.print(",Net ");
    tabs += printNodeOrName(path[i].net);
    tabs += Serial.print("],");
    lineCount++;
    // Serial.print(tabs);
    for (int i = 0; i < 24 - (tabs); i++) {
      Serial.print(" ");
      }
    tabs = 0;

    if (lineCount == 4) {
      Serial.print("\n\r");
      lineCount = 0;
      }
    }
  if (debugNMtime)
    Serial.println("\n\r");
  if (debugNMtime)
    timeToNM = millis() - timeToNM;
  if (debugNMtime)
    Serial.print("\n\rtook ");
  if (debugNMtime)
    Serial.print(timeToNM);
  if (debugNMtime)
    Serial.print("ms to run net manager\n\r");
  }


//returns the number of characters printed (for tabs)
int printNodeOrName(
    int node,
    int longOrShort) // returns number of characters printed (for tabs)
  {
  if (node >= 100) {
    return Serial.print(definesToChar(node, longOrShort));
    } else if (node >= NANO_D0) {
      return Serial.print(definesToChar(node, longOrShort));
      } else {
      return Serial.print(node);
      }
  }

const char* defNanoToCharShort[35] = {
    "VIN",  "D0",   "D1",   "D2",     "D3",     "D4",       "D5",     "D6",
    "D7",   "D8",   "D9",   "D10",    "D11",    "D12",      "D13",    "RESET",
    "AREF", "A0",   "A1",   "A2",     "A3",     "A4",       "A5",     "A6",
    "A7",   "RST0", "RST1", "N_GND1", "N_GND0", "NANO_3V3", "NANO_5V" };

const char* defSpecialToCharShort[49] = {
    "GND",      "TOP_R",   "BOT_R",   "3V3",       "TOP_GND",  "5V",
    "DAC_0",    "DAC_1",   "I_POS",   "I_NEG",     "ADC_0",    "ADC_1",
    "ADC_2",    "ADC_3",   "ADC_4",   "ADC_7",     "UART_Tx",  "UART_Rx",
    "GP_18",    "GP_19",   "8V_P",    "8V_N",      "NONE",     "NONE",
    "NONE",     "NONE",    "BOT_GND", "EMPTY",     "LOGO_T",   "LOGO_B",
    "GP_1",     "GP_2",    "GP_3",    "GP_4",      "GP_5",     "GP_6",
    "GP_7",     "GP_8",    "GPIO_PAD","DAC_PAD",   "ADC_PAD",  "BLDG_TOP",
    "BLDG_BOT", "BUF_IN",  "BUF_OUT"
  };

const char* defNanoToCharLong[35] = {
    "NANO_VIN",   "NANO_D0",   "NANO_D1",     "NANO_D2",     "NANO_D3",
    "NANO_D4",    "NANO_D5",   "NANO_D6",     "NANO_D7",     "NANO_D8",
    "NANO_D9",    "NANO_D10",  "NANO_D11",    "NANO_D12",    "NANO_D13",
    "NANO_RESET", "NANO_AREF", "NANO_A0",     "NANO_A1",     "NANO_A2",
    "NANO_A3",    "NANO_A4",   "NANO_A5",     "NANO_A6",     "NANO_A7",
    "NANO_RST0",  "NANO_RST1", "NANO_N_GND1", "NANO_N_GND0", "NANO_3V3",
    "NANO_5V" };

const char* defSpecialToCharLong[49] = {
    "GND",         "TOP_RAIL",     "BOTTOM_RAIL",  "SUPPLY_3V3",
    "TOP_GND",     "SUPPLY_5V",    "DAC0",         "DAC1",
    "ISENSE_PLUS", "ISENSE_MINUS", "ADC0",         "ADC1",
    "ADC2",        "ADC3",         "ADC4",         "ADC7",
    "RP_UART_Tx",  "RP_UART_Rx",   "RP_GPIO_18",   "RP_GPIO_19",
    "8V_POS",      "8V_NEG",       "NONE",         "NONE",
    "NONE",        "NONE",         "BOTTOM_GND",   "EMPTY_NET",
    "LOGO_TOP",    "LOGO_BOTTOM",  "RP_GPIO_1",    "RP_GPIO_2",
    "RP_GPIO_3",   "RP_GPIO_4",    "RP_GPIO_5",    "RP_GPIO_6",
    "RP_GPIO_7",   "RP_GPIO_8",    "GPIO_PAD",     "DAC_PAD",
    "ADC_PAD",     "BUILDING_TOP", "BUILDING_BOT", "BUFFER_IN",
    "BUFFER_OUT"
  };

const char* emptyNet[3] = { "EMPTY_NET", "?" };

char same[12] = "           ";
const char* definesToChar(int defined,
              int longOrShort) // converts the internally used #defined numbers
  // into human readable strings
  {
  // Try finding the define using our lookup function
  const DefineInfo* info = findDefineInfoByValue(defined);
  if (info) {
    return (longOrShort == 1) ? info->longName : info->shortName;
    }

  // If not found, fall back to the old approach
  if (defined >= 70 && defined <= 99) {
    // Fallback to index-based lookup if define not found in array
    int index = defined - 69;
    if (index >= 0 && index < 31) {
      return (longOrShort == 1) ? defNanoToCharLong[index] : defNanoToCharShort[index];
      }
    } else if (defined >= 100 && defined <= 148) {
      // Fallback to index-based lookup if define not found in array
      int index = defined - 100;
      if (index >= 0 && index < 49) {
        return (longOrShort == 1) ? defSpecialToCharLong[index] : defSpecialToCharShort[index];
        }
      } else if (defined == EMPTY_NET) {
        return emptyNet[0];
        } else {
        // Serial.println("!!!!!!!!!!!!!!!!!!!!");
        itoa(defined, same, 10);
        return same;
        }

      // If nothing matched, return empty string
      return "";
  }

void clearAllPaths(void) {
  digitalWrite(RESETPIN, HIGH);
  delayMicroseconds(600);
  digitalWrite(RESETPIN, LOW);

  for (int i = 0; i < MAX_BRIDGES; i++) {
    path[i].node1 = 0;
    path[i].node2 = 0;
    path[i].net = 0;
    }
  }

// Helper function to find a DefineInfo by its define value
const DefineInfo* findDefineInfoByValue(int defineValue) {
  // Check special defines first
  for (size_t i = 0; i < sizeof(specialDefines) / sizeof(specialDefines[0]); i++) {
    if (specialDefines[i].defineValue == defineValue) {
      return &specialDefines[i];
      }
    }

  // Check nano defines if not found in special defines
  for (size_t i = 0; i < sizeof(nanoDefines) / sizeof(nanoDefines[0]); i++) {
    if (nanoDefines[i].defineValue == defineValue) {
      return &nanoDefines[i];
      }
    }

  // Return null if not found
  return nullptr;
  }

// Test function for verifying the struct-based define lookup
void testDefineInfoStructs() {
  Serial.println("\n\r--- Testing DefineInfo structs ---");

  // Test a few examples from specialDefines
  Serial.print("GND(100) Short: ");
  Serial.println(specialDefines[0].shortName);
  Serial.print("GND(100) Long: ");
  Serial.println(specialDefines[0].longName);
  Serial.print("GND(100) Value: ");
  Serial.println(specialDefines[0].defineValue);

  // Test lookup by define value
  const DefineInfo* info = findDefineInfoByValue(ADC7);
  if (info) {
    Serial.print("Found by value ADC7(115): ");
    Serial.print(info->shortName);
    Serial.print(" / ");
    Serial.println(info->longName);
    }

  // Test ADC7
  Serial.print("ADC7(115) Short: ");
  Serial.println(definesToChar(ADC7, 0));
  Serial.print("ADC7(115) Long: ");
  Serial.println(definesToChar(ADC7, 1));

  // Test a few examples from nanoDefines
  Serial.print("NANO_D5(75) Short: ");
  Serial.println(definesToChar(NANO_D5, 0));
  Serial.print("NANO_D5(75) Long: ");
  Serial.println(definesToChar(NANO_D5, 1));

  // Test finding a nonexistent value
  if (!findDefineInfoByValue(999)) {
    Serial.println("Successfully returned null for nonexistent value 999");
    }

  Serial.println("--- End of test ---\n\r");
  }

/*


void checkDoNotIntersects(); //make sure none of the nodes on the net violate
doNotIntersect rules, exit and warn

void combineNets(); //combine those 2 nets into a single net, probably call
addNodesToNet and deleteNet and just expand the lower numbered one. Should we
shift nets down? or just reuse the newly emply space for the next net

void deleteNet(); //make sure to check special function nets and clear
connections to it

void deleteBridge();

void deleteNode(); //disconnects everything connected to that one node

void checkIfNodesAreABridge(); //if both of those nodes make up a
memberBridge[][] pair. if not, warn and exit

void deleteBridgeAndShift(); //shift the remaining bridges over so they're left
justified and we don't need to search the entire memberBridges[] every time

void deleteNodesAndShift(); //shift the remaining nodes over so they're left
justified and we don't need to search the entire memberNodes[MAX_NODES] every
time

void deleteAllBridgesConnectedToNode(); //search bridges for node and delete any
that contain it

void deleteNodesAndShift(); //shift the remaining nodes over so they're left
justified and we don't need to search the entire memberNodes[MAX_NODES] every
time

void checkForSplitNets(); //if the newly deleted nodes wold split a net into 2
or more non-intersecting nets, we'll need to split them up. return
numberOfNewNets check memberBridges[][]
https://www.geeksforgeeks.org/check-removing-given-edge-disconnects-given-graph/#

void copySplitNetIntoNewNet(); //find which nodes and bridges belong in a new
net

void deleteNodesAndShift(); //delete the nodes and bridges that were copied from
the original net

void leftShiftNodesBridgesNets();*/
