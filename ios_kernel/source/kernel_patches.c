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
#include "kernel_patches.h"
#include "exception_handler.h"
#include "fsa.h"
#include "config.h"
#include "utils.h"

extern void __KERNEL_CODE_START(void);
extern void __KERNEL_CODE_END(void);

extern const patch_table_t kernel_patches_table[];
extern const patch_table_t kernel_patches_table_end[];

static u8 otp_buffer[0x400];

static const u32 mcpIoMappings_patch[] =
{
    // vaddr    paddr       size        ?           ?           ?
    0x0D000000, 0x0D000000, 0x001C0000, 0x00000000, 0x00000003, 0x00000000,   // mapping 1
    0x0D800000, 0x0D800000, 0x001C0000, 0x00000000, 0x00000003, 0x00000000,   // mapping 2
    0x0C200000, 0x0C200000, 0x00100000, 0x00000000, 0x00000003, 0x00000000    // mapping 3
};

static const u32 KERNEL_MCP_IOMAPPINGS_STRUCT[] =
{
	(u32)mcpIoMappings_patch,   // ptr to iomapping structs
	0x00000003,                 // number of iomappings
	0x00000001                  // pid (MCP)
};

static u32 kernel_syscall_0x81(u32 address)
{
    return *(volatile u32*)address;
}

static int kernel_read_otp_internal(int index, void* out_buf, u32 size)
{
    kernel_memcpy(out_buf, otp_buffer + (index << 2), size);
    return 0;
}

int kernel_init_otp_buffer(u32 sd_sector, int tagValid)
{
    int res;

    if(tagValid)
    {
        res = FSA_SDReadRawSectors(otp_buffer, sd_sector, 2);
    }
    else
    {
        int (*orig_kernel_read_otp_internal)(int index, void* out_buf, u32 size) = (void*)0x08120248;
        res = orig_kernel_read_otp_internal(0, otp_buffer, 0x400);
    }

    if((res == 0) && !tagValid)
    {
        FSA_SDWriteRawSectors(otp_buffer, sd_sector, 2);
    }
    return res;
}

void kernel_run_patches(u32 ios_elf_start)
{
    section_write(ios_elf_start, (u32)__KERNEL_CODE_START, __KERNEL_CODE_START, __KERNEL_CODE_END - __KERNEL_CODE_START);
    section_write_word(ios_elf_start, 0x0812A120, ARM_BL(0x0812A120, kernel_launch_ios));

    section_write(ios_elf_start, 0x08140DE0, KERNEL_MCP_IOMAPPINGS_STRUCT, sizeof(KERNEL_MCP_IOMAPPINGS_STRUCT));

    section_write_word(ios_elf_start, 0x0812A134, ARM_BL(0x0812A134, crash_handler_prefetch));
    section_write_word(ios_elf_start, 0x0812A1AC, ARM_BL(0x0812A1AC, crash_handler_data));
    section_write_word(ios_elf_start, 0x08129E50, ARM_BL(0x08129E50, crash_handler_undef_instr));

    section_write_word(ios_elf_start, 0x0812CD2C, ARM_B(0x0812CD2C, kernel_syscall_0x81));

    if(cfw_config.redNAND && cfw_config.otp_red)
    {
        section_write(ios_elf_start, (u32)otp_buffer, otp_buffer, 0x400);
        section_write_word(ios_elf_start, 0x08120248, ARM_B(0x08120248, kernel_read_otp_internal));
    }

    u32 patch_count = (u32)(((u8*)kernel_patches_table_end) - ((u8*)kernel_patches_table)) / sizeof(patch_table_t);
    patch_table_entries(ios_elf_start, kernel_patches_table, patch_count);
}
