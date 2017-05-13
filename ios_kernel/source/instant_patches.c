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
#include "config.h"
#include "utils.h"
#include "types.h"
#include "elf_patcher.h"
#include "kernel_patches.h"
#include "ios_mcp_patches.h"
#include "../../ios_mcp/ios_mcp_syms.h"

typedef struct
{
    u32 paddr;
    u32 vaddr;
    u32 size;
    u32 domain;
    u32 type;
    u32 cached;
} ios_map_shared_info_t;

void instant_patches_setup(void)
{
    // apply IOS ELF launch hook
	*(volatile u32*)0x0812A120 = ARM_BL(0x0812A120, kernel_launch_ios);

    // patch FSA raw access
	*(volatile u32*)0x1070FAE8 = 0x05812070;
	*(volatile u32*)0x1070FAEC = 0xEAFFFFF9;

    if(cfw_config.noIosReload)
    {
        int (*_iosMapSharedUserExecution)(void *descr) = (void*)0x08124F88;

        // patch kernel dev node registration
        *(volatile u32*)0x081430B4 = 1;

        // fix 10 minute timeout that crashes MCP after 10 minutes of booting
        *(volatile u32*)(0x05022474 - 0x05000000 + 0x081C0000) = 0xFFFFFFFF;    // NEW_TIMEOUT

        // patch cached cert check
        // start our MCP thread directly on first title change
        kernel_memset((void*)(0x050BD000 - 0x05000000 + 0x081C0000), 0, 0x3000);
        *(volatile u32*)(0x05054D6C - 0x05000000 + 0x081C0000) = ARM_B(0x05054D6C, _startMainThread);

        // patch MCP authentication check
        *(volatile u32*)(0x05014CAC - 0x05000000 + 0x081C0000) = 0x20004770;    // mov r0, #0; bx lr

        // patch IOSC_VerifyPubkeySign to always succeed
        *(volatile u32*)(0x05052C44 - 0x05000000 + 0x081C0000) = 0xE3A00000;    // mov r0, #0
        *(volatile u32*)(0x05052C48 - 0x05000000 + 0x081C0000) = 0xE12FFF1E;    // bx lr

        // allow custom bootLogoTex and bootMovie.h264
        *(volatile u32*)(0xE0030D68 - 0xE0000000 + 0x12900000) = 0xE3A00000;    // mov r0, #0
        *(volatile u32*)(0xE0030D34 - 0xE0000000 + 0x12900000) = 0xE3A00000;    // mov r0, #0

        // allow any region title launch
        *(volatile u32*)(0xE0030498 - 0xE0000000 + 0x12900000) = 0xE3A00000;    // mov r0, #0

        // set zero to start thread directly on first title change
        *(volatile u32*)(0x050BC580 - 0x05000000 + 0x081C0000) = 0;
        // down display launch image at this state
        *(volatile u32*)(_text_start - 4 - 0x05100000 + 0x13D80000) = 0;

        // patch the read position for the cos xml's p4.mask(ios_fs) to read 0xFFFFFFFFFFFFFFFF
        *(volatile u32*)(0x05002BBE - 0x05000000 + 0x081C0000) = (volatile u32*)THUMB_BL(0x05002BBE, patch_SD_access_check);

        ios_map_shared_info_t map_info;
        map_info.paddr = 0x050BD000 - 0x05000000 + 0x081C0000;
        map_info.vaddr = 0x050BD000;
        map_info.size = 0x3000;
        map_info.domain = 1;            // MCP
        map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read/write
        map_info.cached = 0xFFFFFFFF;
        _iosMapSharedUserExecution(&map_info);  // actually a bss section but oh well it will have read/write

        map_info.paddr = 0x05116000 - 0x05100000 + 0x13D80000;
        map_info.vaddr = 0x05116000;
        map_info.size = 0x4000;
        map_info.domain = 1;            // MCP
        map_info.type = 3;              // 0 = undefined, 1 = kernel only, 2 = read only, 3 = read write
        map_info.cached = 0xFFFFFFFF;
        _iosMapSharedUserExecution(&map_info);
    }
}
