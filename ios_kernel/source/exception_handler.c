/***************************************************************************
 * Copyright (C) 2016
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include "text.h"
#include "types.h"

void crash_handler(unsigned int *context, int type)
{
    clearScreen(0xFFFFFFFF);

    if(type == 0)
    {
        _printf(0, 0, "GURU MEDITATION ERROR (prefetch abort)");
    }
    else if(type == 1)
    {
        _printf(0, 0, "GURU MEDITATION ERROR (data abort)");
    }
    else
    {
        _printf(0, 0, "GURU MEDITATION ERROR (undefined instruction)");
    }

    int reg = 0;
    while(reg < 16)
    {
        if(reg < 10)
        {
            _printf(20, 40 + reg * 20, "r%d  = %08X", reg, context[1 + reg]);
        }
        else
        {
            _printf(20, 40 + reg * 20, "r%d = %08X", reg, context[1 + reg]);
        }

        reg++;
    }

    _printf(400, 20, "%08X", *(u32*)context[0x10]);

    for(;;);
}
