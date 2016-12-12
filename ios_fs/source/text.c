#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "imports.h"
#include "types.h"
#include "font_bin.h"

#define FRAMEBUFFER_ADDRESS (0x14000000+0x38C0000)
#define FRAMEBUFFER_STRIDE (0xE00)
#define FRAMEBUFFER_STRIDE_WORDS (FRAMEBUFFER_STRIDE >> 2)

#define CHAR_SIZE_X (8)
#define CHAR_SIZE_Y (8)

u32* const framebuffer = (u32*)FRAMEBUFFER_ADDRESS;

void clearScreen(u32 color)
{
	int i;
	for(i = 0; i < ((FRAMEBUFFER_STRIDE * 504)/4); i++)
	{
		framebuffer[i] = color;
	}
}

void clearLine(int y, u32 color)
{
	u32* fb = &framebuffer[y * FRAMEBUFFER_STRIDE_WORDS];
	u32* fb_end = &framebuffer[(y+CHAR_SIZE_Y) * FRAMEBUFFER_STRIDE_WORDS];

    while(fb < fb_end)
    {
        *fb = color;
        fb++;
    }
}

void drawCharacter(char c, int x, int y)
{
	if(c < 32)return;
	c -= 32;
	u8* charData = (u8*)&font_bin[(CHAR_SIZE_X * CHAR_SIZE_Y * c) / 8];
	u32* fb = &framebuffer[x + y * FRAMEBUFFER_STRIDE_WORDS];
	int i, j;
	for(i = 0; i < CHAR_SIZE_Y; i++)
	{
		u8 v= *(charData++);
		for(j = 0; j < CHAR_SIZE_X; j++)
		{
			if(v & 1) *fb = 0x00000000;
			else *fb = 0xFFFFFFFF;
			v >>= 1;
			fb++;
		}
		fb += FRAMEBUFFER_STRIDE_WORDS - CHAR_SIZE_X;
	}
}

void drawString(char* str, int x, int y)
{
	if(!str) return;
	int k;
	int dx = 0, dy = 0;
	for(k = 0; str[k]; k++)
	{
		if(str[k] >= 32 && str[k] < 128) drawCharacter(str[k], x + dx, y + dy);

		dx += 8;

		if(str[k] == '\n')
		{
			dx = 0;
			dy -= 8;
		}
	}
}

void _printf(int x, int y, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char buffer[0x100];

    FS_VSNPRINTF(buffer, sizeof(buffer), format, args);
    drawString(buffer, x, y);

    va_end(args);
}
