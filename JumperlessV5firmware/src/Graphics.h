#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <Arduino.h>

class bread {
    public:
    bread();
    void print(const char c);
    void print(const char c, int position);
    void print(const char c, uint32_t color);
    void print(const char c, uint32_t color, int position, int topBottom);
    void print(const char c, uint32_t color, int position);
    void print(const char c, uint32_t color, uint32_t backgroundColor);
    void print(const char c, uint32_t color, uint32_t backgroundColor, int position, int topBottom);
    void print(const char c, uint32_t color, uint32_t backgroundColor, int position, int topBottom, int nudge);
    void print(const char c, uint32_t color, uint32_t backgroundColor, int position, int topBottom, int nudge, int lowercase);
    void print(const char c, uint32_t color, uint32_t backgroundColor, int position);

    void print(const char* s);
    void print(const char* s, int position);
    void print(const char* s, uint32_t color);
    void print(const char* s, uint32_t color, int position);
    void print(const char* s, uint32_t color, int position, int topBottom);
    void print(const char* s, uint32_t color, uint32_t backgroundColor);
    void print(const char* s, uint32_t color, uint32_t backgroundColor, int position, int topBottom);
    void print(const char* s, uint32_t color, uint32_t backgroundColor, int position, int topBottom, int nudge);
    void print(const char* s, uint32_t color, uint32_t backgroundColor, int position, int topBottom, int nudge, int lowercaseNumber);
    void print(const char* s, uint32_t color, uint32_t backgroundColor, int position);


    void print(int i);
    void print(int i, int position);
    void print(int i, uint32_t color);
    void print(int i, uint32_t color, int position);
    void print(int i, uint32_t color, int position, int topBottom);
    void print(int i, uint32_t color, int position, int topBottom, int nudge);
    void print(int i, uint32_t color, int position, int topBottom, int nudge, int lowercase);
    void print(int i, uint32_t color, uint32_t backgroundColor);



void printMenuReminder(int menuDepth, uint32_t color);
void printRawRow(uint8_t data, int row, uint32_t color, uint32_t bg);

    
    void clear(int topBottom = -1);

    // void printChar(char c, uint32_t color, uint32_t backgroundColor, int position, int topBottom);





};


extern uint8_t font[][3];

extern bread b;

void printGraphicsRow(uint8_t data, int row, uint32_t color = 0xFFFFFF, uint32_t bg = 0xFFFFFF);

void scrollFont(void);

void printChar(const char c, uint32_t color = 0xFFFFFF, uint32_t bg = 0xFFFFFF, int position = 0, int topBottom = -1, int nudge = 0, int lowercase = 0);

void printString(const char* s, uint32_t color = 0xFFFFFF, uint32_t bg = 0xFFFFFF, int position = 0, int topBottom = -1, int nudge = 0, int lowercase = 0);

















#endif
