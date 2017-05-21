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
#include "elf_patcher.h"
#include "config.h"
#include "ios_mcp_patches.h"
#include "../../ios_mcp/ios_mcp.bin.h"
#include "../../ios_mcp/ios_mcp_syms.h"

#define MCP_CODE_BASE_PHYS_ADDR     (-0x05100000 + 0x13D80000)

extern const patch_table_t mcp_patches_table[];
extern const patch_table_t mcp_patches_table_end[];

u32 mcp_get_phys_code_base(void)
{
    return _text_start + MCP_CODE_BASE_PHYS_ADDR;
}

void mcp_run_patches(u32 ios_elf_start)
{
    // write ios_mcp code and bss
    section_write_bss(ios_elf_start, _bss_start, _bss_end - _bss_start);
    section_write(ios_elf_start, _text_start, (void*)mcp_get_phys_code_base(), _text_end - _text_start);

    section_write_word(ios_elf_start, 0x05056718, ARM_BL(0x05056718, _text_start));

    section_write_word(ios_elf_start, 0x05002BBE, THUMB_BL(0x05002BBE, patch_SD_access_check));

    if(cfw_config.syshaxXml)
    {
        section_write(ios_elf_start, 0x050600DC, "/vol/system/config/syshax.xml", 0x20);
        section_write(ios_elf_start, 0x050600FC, "/vol/system_slc/config/syshax.xml", 0x24);
    }

    u32 patch_count = (u32)(((u8*)mcp_patches_table_end) - ((u8*)mcp_patches_table)) / sizeof(patch_table_t);
    patch_table_entries(ios_elf_start, mcp_patches_table, patch_count);

    // patch MCP syslogs
    //section_write_word(ios_elf_start, 0x05055438, ARM_B(0x05055438, 0x0503DCF8));
    //section_write_word(ios_elf_start, 0x05056C2C, ARM_B(0x05056C2C, 0x0503DCF8));
    //section_write_word(ios_elf_start, 0x0500A4D2, THUMB_BL(0x0500A4D2, mcpThumb2ArmLog));
}
