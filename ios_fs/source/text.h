#ifndef TEXT_H
#define TEXT_H

#include "types.h"

void clearScreen(u32 color);
void clearLine(int y, u32 color);
void drawString(char* str, int x, int y);
void _printf(int x, int y, const char *format, ...);

#endif
