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

int ACP_FSARawRead_hook(int fd, void* data, u64 offset, u32 cnt, u32 blocksize, int device_handle)
{
    int (*ACP_FSARawRead)(int fd, void* data, u64 offset, u32 cnt, u32 blocksize, int device_handle) = (void*)0xE00BAF74;

    int res = ACP_FSARawRead(fd, data, offset, cnt, blocksize, device_handle);

    //! the PPC side has a way to check for a PC or WFS formatted drive by checking the MBR signature
    //! it's the only place where this is used so we can just fake it with wrong values.
    u8 *buf = (u8*)data;
    if((offset == 0) && (buf[510] == 0x55) && (buf[511] == 0xAA))
    {
        buf[510] = 0xB3;
        buf[511] = 0xDE;
    }
    return res;
}
