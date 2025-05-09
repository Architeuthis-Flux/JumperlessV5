// SPDX-License-Identifier: MIT

#include "NetManager.h"
//#include "FileParsing.h"
#include "MatrixState.h"
#include "NetsToChipConnections.h"
#include "Peripherals.h"
#include "SafeString.h"
#include <Arduino.h>
#include <EEPROM.h>

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
               net[foundNode2Net].doNotIntersectNodes[j] == foundNode2Net) ){//&&
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

int findNodeInNet(uint16_t node) {
// Serial.println("findNodeInNet");
// Serial.print("node = ");
//     Serial.println(node);
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

  switch (node) {
  case 135:
    if (gpioNet[0] != -2) {
    gpioNet[0] = net;
    foundGPIO = 1;
    }
    break;
  case 136:
    if (gpioNet[1] != -2) {
    gpioNet[1] = net;
    foundGPIO = 1;
    }
    break;
  case 137:
    if (gpioNet[2] != -2) {
    gpioNet[2] = net;
    foundGPIO = 1;
    }
    break;
  case 138:
    if (gpioNet[3] != -2) {
    gpioNet[3] = net;
    foundGPIO = 1;
    }
    break;
  case 122:
    if (gpioNet[4] != -2) {
    gpioNet[4] = net;
    foundGPIO = 1;
    }
    break;
  case 123:
    if (gpioNet[5] != -2) {
    gpioNet[5] = net;
    foundGPIO = 1;
    }
    break;
  case 124:
    if (gpioNet[6] != -2) {
    gpioNet[6] = net;
    foundGPIO = 1;
    }
    break;
  case 125:
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

int checkDoNotIntersectsByNet(
    int netToCheck1, int netToCheck2) // If you're searching DNIs by net, there
                                      // won't be any valid ways to make a new
                                      // net with both nodes, so its skipped
{

  for (int i = 0; i <= MAX_DNI; i++) {
    if (net[netToCheck1].doNotIntersectNodes[i] == 0) {
      break;
    }

    for (int j = 0; j <= MAX_NODES;
         j++) { 
          
          if(debugNM) Serial.print
                (net[netToCheck1].doNotIntersectNodes[i]);
      if(debugNM) Serial.print ("-");
      if(debugNM) Serial.println (net[netToCheck2].nodes[j]);

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

void listNets(void) // list nets doesnt care about debugNM, don't call it if you
                    // don't want it to print
{
  if (net[6].number == 0) {
    // Serial.print("No nets to list\n\r");
    // return;
  } else {
    Serial.print("\n\rIndex\tName\t\t\t    Nodes\t");

    int tabs = 0;
    for (int i = 6; i < MAX_NETS; i++) {
      if (net[i].number == 0 ||
          net[i].nodes[0] ==
              -1) // stops searching if it gets to an unallocated net
      {
        // Serial.print("Done listing nets");
        break;
      }

      Serial.print("\n\r ");
      Serial.print(i);
      Serial.print("\t ");
      int netNameLength = Serial.print(net[i].name);
      if (netNameLength < 8) {
        Serial.print("\t");
      }
      Serial.print("\t");
     // Serial.print(net[i].number);
      Serial.print("\t     ");

      // hsvColor color = RgbToHsv(net[i].color);

      // int hue = (color.h)/38;
      // Serial.print(color.h);
      // Serial.print(" ");
      // Serial.print(hue);
      // Serial.print(" ");

      // Serial.print(colorNamesSim[hue]);
      //  Serial.print("\t");

      // Serial.print("r");
      // if (net[i].color.r < 16)
      // {
      //     Serial.print("0");
      // }
      // netNameLength = Serial.print(net[i].color.r, HEX);

      // Serial.print(" g");
      // if (net[i].color.g < 16)
      // {
      //     Serial.print("0");
      // }
      // netNameLength = Serial.print(net[i].color.g, HEX);

      // Serial.print(" b");
      // if (net[i].color.b < 16)
      // {
      //     Serial.print("0");
      // }
      // netNameLength = Serial.print(net[i].color.b, HEX);

      // if (netNameLength < 6)
      // {
      //     Serial.print("\t");
      // }
      // Serial.print("\t");

      tabs = 0;
      for (int j = 0; j < MAX_NODES; j++) {

        tabs += printNodeOrName(net[i].nodes[j]);

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

      tabs = 0;
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

          if (net[i].doNotIntersectNodes[j + 1] == 0)
          {
              break;
          }
          else
          {

              tabs += Serial.print(",");
          }
      }
      */
    }
  }
  Serial.print("\n\r");
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

int printNodeOrName(
    int node,
    int longOrShort) // returns number of characters printed (for tabs)
{
  // Serial.print("node = ");
  // Serial.println(node);

  if (node >= 100) {
    return Serial.print(definesToChar(node, longOrShort));
  } else if (node >= NANO_D0) {
    return Serial.print(definesToChar(node, longOrShort));
  } else {
    return Serial.print(node);
  }
  // Serial.print("node = ");
  // Serial.println(node);
}

  const char *defNanoToCharShort[35] = {
      "VIN",  "D0",   "D1",   "D2",     "D3",     "D4",       "D5",     "D6",
      "D7",   "D8",   "D9",   "D10",    "D11",    "D12",      "D13",    "RESET",
      "AREF", "A0",   "A1",   "A2",     "A3",     "A4",       "A5",     "A6",
      "A7",   "RST0", "RST1", "N_GND1", "N_GND0", "NANO_3V3", "NANO_5V"};

  const char *defSpecialToCharShort[49] = {
      "GND",      "TOP_R",   "BOT_R",   "3V3",         "TOP_GND",  "5V",
      "DAC_0",    "DAC_1",   "I_POS",   "I_NEG",       "ADC_0",    "ADC_1",
      "ADC_2",    "ADC_3",   "ADC_4",    "ADC_7", "UART_Tx",  "UART_Rx",
      "GP_18",    "GP_19",   "8V_P",    "8V_N",        "GP_5",     "GP_6",
      "GP_7",     "GP_8",    "BOT_GND", "EMPTY",       "LOGO_T",   "LOGO_B",
      "GPIO_PAD", "DAC_PAD", "ADC_PAD", "BLDG_TOP",    "BLDG_BOT", "GP_1",
      "GP_2",     "GP_3",    "GP_4",    "BUF_IN",      "BUF_OUT",  "fuck",
      "fuck2",    "fuck3"};

  const char *defNanoToCharLong[35] = {
      "NANO_VIN",   "NANO_D0",   "NANO_D1",     "NANO_D2",     "NANO_D3",
      "NANO_D4",    "NANO_D5",   "NANO_D6",     "NANO_D7",     "NANO_D8",
      "NANO_D9",    "NANO_D10",  "NANO_D11",    "NANO_D12",    "NANO_D13",
      "NANO_RESET", "NANO_AREF", "NANO_A0",     "NANO_A1",     "NANO_A2",
      "NANO_A3",    "NANO_A4",   "NANO_A5",     "NANO_A6",     "NANO_A7",
      "NANO_RST0",  "NANO_RST1", "NANO_N_GND1", "NANO_N_GND0", "NANO_3V3",
      "NANO_5V"};

  const char *defSpecialToCharLong[49] = {
      "GND",         "TOP_RAIL",     "BOTTOM_RAIL",  "SUPPLY_3V3",
      "TOP_GND",     "SUPPLY_5V",    "DAC0",         "DAC1",
      "ISENSE_PLUS", "ISENSE_MINUS", "ADC0",         "ADC1",
      "ADC2",        "ADC3",         "ADC4",    "PROBE",
      "RP_UART_Tx",  "RP_UART_Rx",   "RP_GPIO_18",   "RP_GPIO_19",
      "8V_POS",      "8V_NEG",       "GPIO_5",       "GPIO_6",
      "GPIO_7",      "GPIO_8",       "BOTTOM_GND",   "EMPTY_NET",
      "LOGO_TOP",    "LOGO_BOTTOM",  "GPIO_PAD",     "DAC_PAD",
      "ADC_PAD",     "BUILDING_TOP", "BUILDING_BOT", "GPIO_1",
      "GPIO_2",      "GPIO_3",       "GPIO_4",       "BUFFER_IN",
      "BUFFER_OUT",  "fuck",         "fuck2",        "Fuck3"};

  const char *emptyNet[3] = {"EMPTY_NET", "?"};

char same[12] = "           ";
const char *definesToChar(int defined,
              int longOrShort) // converts the internally used #defined numbers
                               // into human readable strings
{
  // Serial.print("defined = ");
  // Serial.println(defined);



  if (defined >= 70 && defined <= 99) {
    if (longOrShort == 1) {
      return defNanoToCharLong[defined - 69];
    } else {
      return defNanoToCharShort[defined - 69];
    }

  } else if (defined >= 100 && defined <= 140) {

    if (longOrShort == 1) {
      return defSpecialToCharLong[defined - 100];
    } else {
      return defSpecialToCharShort[defined - 100];
    }

  } else if (defined == EMPTY_NET) {
    return emptyNet[0];
  } else {
    // Serial.println("!!!!!!!!!!!!!!!!!!!!");

    itoa(defined, same, 10);
    return same;
  }
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
