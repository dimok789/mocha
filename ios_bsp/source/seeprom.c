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
#include "svc.h"
#include "fsa.h"

#define SD_SEEPROM_SECTOR           0x4FF

#define BSP_MEMCPY  ((void * (*)(void *, void *, unsigned int size))0xE600EA18)

static int writeEnabled = 0;
static int dirty = 0;

unsigned char seeprom_buffer[512] __attribute__((section(".seeprom_buffer")));

extern int orig_EEPROM_SPI_ReadWord(int handle_index, unsigned char index, unsigned short *outbuf);

static int SD_EEPROM_WriteAll(void)
{
    int fsa = svcOpen("/dev/fsa", 0);
    if(fsa < 0)
        return fsa;

    int fd;
    int res = FSA_RawOpen(fsa, "/dev/sdcard01", &fd);
    if(res >= 0)
    {
        void *buffer = svcAllocAlign(0xCAFF, 0x200, 0x40);
        if(buffer)
        {
            // user global buffer for FSA to be able to access it
            BSP_MEMCPY(buffer, seeprom_buffer, 0x200);
            res = FSA_RawWrite(fsa, buffer, 0x200, 1, SD_SEEPROM_SECTOR, fd);
            svcFree(0xCAFF, buffer);
        }
        else
            res = -1;

        FSA_RawClose(fsa, fd);
    }
    svcClose(fsa);
    return res;
}

static void EEPROM_InitializeCache(int handle_index)
{
    int i;
    for(i = 0; i < 0x100; i++)
    {
        orig_EEPROM_SPI_ReadWord(handle_index, i, (unsigned short*)(seeprom_buffer + (i << 1)));
    }
}

int EEPROM_SPI_ReadWord(int handle_index, unsigned char index, unsigned short *outbuf)
{
    unsigned int offset = ((unsigned int)index) << 1;

    // check for valid eeprom dump and initialize if none was on sd card
    if(*(u32*)(seeprom_buffer + 0x20) != 0x70010201) // PPC PVR
    {
        EEPROM_InitializeCache(handle_index); // could actually just use 0 for handle index
        dirty = 1;
    }

    // don't redirect the drive key as it is specific for the drive on the wii u
    // the seeprom key is the same for all wiiu's it seems so nothing to re-encrypt here
    if(offset >= 0x80 && offset < 0x90)
    {
        return orig_EEPROM_SPI_ReadWord(handle_index, index, outbuf);
    }

    if(!outbuf || (offset >= 512))
    {
        return -5;
    }

    *outbuf = *(unsigned short*)(seeprom_buffer + offset);

    if(dirty && SD_EEPROM_WriteAll() == 0)
    {
        dirty = 0;
    }
    return 0;
}

int EEPROM_SPI_WriteWord(int handle_index, unsigned char index, unsigned short data)
{
    if(writeEnabled == 0)
    {
        return -5;
    }

    // check for valid eeprom dump and initialize if none was on sd card
    if(*(u32*)(seeprom_buffer + 0x20) != 0x70010201) // PPC PVR
    {
        EEPROM_InitializeCache(handle_index); // could actually just use 0 for handle index
    }

    unsigned int offset = ((unsigned int)index) << 1;

    if(offset >= 512)
    {
        return -5;
    }

    *(unsigned short*)(seeprom_buffer + offset) = data;
    dirty = 1;

    if(SD_EEPROM_WriteAll() == 0)
    {
        dirty = 0;
    }

    return 0;
}

int EEPROM_WriteControl(int handle_index, int type)
{
    if(type == 1)
    {
        writeEnabled = 0;
    }
    else if(type == 2)
    {
        writeEnabled = 1;
    }
    else if(type == 3)
    {
        // erase all -> skip that part...its actually never used but would be only a memset with 0xFF
    }
    else
    {
        return -4;
    }
    return 0;
}
