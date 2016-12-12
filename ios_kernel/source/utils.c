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

// this memcpy is optimized for speed and to work with MEM1 32 bit access alignment requirement
void reverse_memcpy(void* dst, const void* src, unsigned int size)
{
    const unsigned char *src_p;
    unsigned char *dst_p;

    if((size >= 4) && !((dst - src) & 3))
    {
        const unsigned int *src_p32;
        unsigned int *dst_p32;
        unsigned int endDst = ((unsigned int)dst) + size;
        unsigned int endRest = endDst & 3;

        if(endRest)
        {
            src_p = ((const unsigned char*)(src + size)) - 1;
            dst_p = ((unsigned char*)endDst) - 1;
            size -= endRest;

            while(endRest--)
                *dst_p-- = *src_p--;
        }

        src_p32 = ((const unsigned int*)(src + size)) - 1;
        dst_p32 = ((unsigned int*)(dst + size)) - 1;

        unsigned int size32 = size >> 5;
        if(size32)
        {
            size &= 0x1F;

            while(size32--)
            {
                src_p32 -= 8;
                dst_p32 -= 8;

                dst_p32[8] = src_p32[8];
                dst_p32[7] = src_p32[7];
                dst_p32[6] = src_p32[6];
                dst_p32[5] = src_p32[5];
                dst_p32[4] = src_p32[4];
                dst_p32[3] = src_p32[3];
                dst_p32[2] = src_p32[2];
                dst_p32[1] = src_p32[1];
            }
        }

        unsigned int size4 = size >> 2;
        if(size4)
        {
            size &= 3;

            while(size4--)
                *dst_p32-- = *src_p32--;
        }

        dst_p = ((unsigned char*)dst_p32) + 3;
        src_p = ((const unsigned char*)src_p32) + 3;
    }
    else
    {
        dst_p = ((unsigned char*)dst) + size - 1;
        src_p = ((const unsigned char*)src) + size - 1;
    }

    while(size--)
        *dst_p-- = *src_p--;
}
