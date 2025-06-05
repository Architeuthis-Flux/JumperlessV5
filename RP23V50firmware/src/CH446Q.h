// SPDX-License-Identifier: MIT
#ifndef CH446Q_H
#define CH446Q_H


extern int netNumberC2;
extern int onOffC2;
extern int nodeC2;
extern int brightnessC2;
extern int hueShiftC2;
extern int lightUpNetCore2;


void sendPaths(int clean = 0);
void initCH446Q(void);
void sendXYraw(int chip, int x, int y, int setorclear);

void sendAllPaths(int clean = 0); // should we sort them by chip? for now, no

void sendPath(int path, int setOrClear = 1, int newOrLast = 0);
void findDifferentPaths(void);
void createXYarray(void);
void refreshPaths(void);
void sortPathsByChipXY(void);
void printChipStateArray(void);
void updateChipStateArray(void);
void createChipOrderedIndex(void);
void printLastChipStateArray(void);
#endif
