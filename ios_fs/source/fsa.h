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
#ifndef _FSA_H_
#define _FSA_H_

typedef struct
{
    u32 flag;
    u32 permission;
    u32 owner_id;
    u32 group_id;
    u32 size; // size in bytes
    u32 physblock_size; // physical size on disk in bytes
    u64 quota_size;
    u32 id;
    u32 ctime;
    u32 mtime;
    u32 unk2[0x0D];
}fileStat_s;

#endif
