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
#ifndef _ELF_PATCHER_H
#define _ELF_PATCHER_H

#include "types.h"

#define ARM_B(addr, func)       (0xEA000000 | ((((u32)(func) - (u32)(addr) - 8) >> 2) & 0x00FFFFFF))                                                        // +-32MB
#define ARM_BL(addr, func)      (0xEB000000 | ((((u32)(func) - (u32)(addr) - 8) >> 2) & 0x00FFFFFF))                                                        // +-32MB
#define THUMB_B(addr, func)     ((0xE000 | ((((u32)(func) - (u32)(addr) - 4) >> 1) & 0x7FF)))                                                               // +-2KB
#define THUMB_BL(addr, func)    ((0xF000F800 | ((((u32)(func) - (u32)(addr) - 4) >> 1) & 0x0FFF)) | ((((u32)(func) - (u32)(addr) - 4) << 4) & 0x7FFF000))   // +-4MB

typedef struct
{
    u32 address;
    void* data;
    u32 size;
} patch_table_t;

void section_write(u32 ios_elf_start, u32 address, const void *data, u32 size);
void section_write_bss(u32 ios_elf_start, u32 address, u32 size);

static inline void section_write_word(u32 ios_elf_start, u32 address, u32 word)
{
    section_write(ios_elf_start, address, &word, sizeof(word));
}


static inline void patch_table_entries(u32 ios_elf_start, const patch_table_t * patch_table, u32 patch_count)
{
    u32 i;
    for(i = 0; i < patch_count; i++)
    {
        section_write(ios_elf_start, patch_table[i].address, patch_table[i].data, patch_table[i].size);
    }
}


#endif
