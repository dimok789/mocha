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
#include "types.h"
#include "elf_abi.h"
#include "utils.h"

static Elf32_Phdr * get_section(u32 data, u32 vaddr)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) data;

    if (   !IS_ELF (*ehdr)
        || (ehdr->e_type != ET_EXEC)
        || (ehdr->e_machine != EM_ARM))
    {
        return 0;
    }

    Elf32_Phdr *phdr = 0;

    u32 i;
    for(i = 0; i < ehdr->e_phnum; i++)
    {
        phdr = (Elf32_Phdr *) (data + ehdr->e_phoff + ehdr->e_phentsize * i);

        if((vaddr >= phdr[0].p_vaddr) && ((i == ehdr->e_phnum) || (vaddr < phdr[1].p_vaddr)))
        {
            break;
        }
    }
    return phdr;
}

void section_write_bss(u32 ios_elf_start, u32 address, u32 size)
{
    Elf32_Phdr *phdr = get_section(ios_elf_start, address);
    if(!phdr)
        return;

    if((address - phdr->p_vaddr + size) > phdr->p_memsz)
    {
        phdr->p_memsz = (address - phdr->p_vaddr + size);
    }
}

void section_write(u32 ios_elf_start, u32 address, const void *data, u32 size)
{
    Elf32_Phdr *phdr = get_section(ios_elf_start, address);
    if(!phdr)
        return;

    u32 *addr = (u32*)(ios_elf_start + address - phdr->p_vaddr + phdr->p_offset);

    if((address - phdr->p_vaddr + size) > phdr->p_filesz)
    {
        u32 additionalSize = address - phdr->p_vaddr + size - phdr->p_filesz;

        Elf32_Ehdr *ehdr = (Elf32_Ehdr *) ios_elf_start;
        Elf32_Phdr * tmpPhdr;
        u32 i;
        for(i = (ehdr->e_phnum-1); i >= 0; i--)
        {
            tmpPhdr = (Elf32_Phdr *) (ios_elf_start + ehdr->e_phoff + ehdr->e_phentsize * i);

            if(phdr->p_offset < tmpPhdr->p_offset)
            {
                reverse_memcpy((u8*)ios_elf_start + tmpPhdr->p_offset + additionalSize, (u8*)ios_elf_start + tmpPhdr->p_offset, tmpPhdr->p_filesz);
                tmpPhdr->p_offset += additionalSize;
            }
            else {
                break;
            }
        }
        phdr->p_filesz += additionalSize;
        if(phdr->p_memsz < phdr->p_filesz)
        {
            phdr->p_memsz = phdr->p_filesz;
        }
    }

    // in most cases only a word is copied to an aligned address so do a short cut for performance
    if(size == 4 && !((unsigned int)addr & 3) && !((unsigned int)data & 3))
    {
        *(u32*)addr = *(u32*)data;
    }
    else
    {
        kernel_memcpy(addr, data, size);
    }
}
