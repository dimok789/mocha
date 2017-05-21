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
#include "ios_fs_patches.h"
#include "config.h"
#include "../../ios_fs/ios_fs_syms.h"

#define FS_PHYS_DIFF                                0

#define FS_SYSLOG_OUTPUT                            0x107F0C84
#define FS_PRINTF_SYSLOG                            0x107F5720
#define CALL_FS_REGISTERMDPHYSICALDEVICE            0x107BD81C
#define FS_GETMDDEVICEBYID                          0x107187C4
#define FS_CREATEDEVTHREAD_HOOK                     0x10700294
#define FS_USB_READ                                 0x1077F1C0
#define FS_USB_WRITE                                0x1077F35C
#define FS_SLC_READ1                                0x107B998C
#define FS_SLC_READ2                                0x107B98FC
#define FS_SLC_WRITE1                               0x107B9870
#define FS_SLC_WRITE2                               0x107B97E4
#define FS_MLC_READ1                                0x107DC760
#define FS_MLC_READ2                                0x107DCDE4
#define FS_MLC_WRITE1                               0x107DC0C0
#define FS_MLC_WRITE2                               0x107DC73C
#define FS_SDCARD_READ1                             0x107BDDD0
#define FS_SDCARD_WRITE1                            0x107BDD60

extern const patch_table_t fs_patches_table[];
extern const patch_table_t fs_patches_table_end[];

u32 fs_get_phys_code_base(void)
{
    return _text_start + FS_PHYS_DIFF;
}

void fs_run_patches(u32 ios_elf_start)
{
    // write wupserver code and bss
    section_write(ios_elf_start, _text_start, (void*)fs_get_phys_code_base(), _text_end - _text_start);
    section_write_bss(ios_elf_start, _bss_start, _bss_end - _bss_start);

    // patch FS logging
    section_write_word(ios_elf_start, FS_PRINTF_SYSLOG, ARM_B(FS_PRINTF_SYSLOG, FS_SYSLOG_OUTPUT));

    if(cfw_config.redNAND)
    {
        section_write_word(ios_elf_start, CALL_FS_REGISTERMDPHYSICALDEVICE, ARM_BL(CALL_FS_REGISTERMDPHYSICALDEVICE, registerMdDevice_hook));
        section_write_word(ios_elf_start, FS_GETMDDEVICEBYID + 8, ARM_BL((FS_GETMDDEVICEBYID + 8), getMdDeviceById_hook));

        section_write_word(ios_elf_start, FS_SDCARD_READ1, ARM_B(FS_SDCARD_READ1, sdcardRead_patch));
        section_write_word(ios_elf_start, FS_SDCARD_WRITE1, ARM_B(FS_SDCARD_WRITE1, sdcardWrite_patch));

        section_write_word(ios_elf_start, FS_SLC_READ1, ARM_B(FS_SLC_READ1, slcRead1_patch));
        section_write_word(ios_elf_start, FS_SLC_READ2, ARM_B(FS_SLC_READ2, slcRead2_patch));
        section_write_word(ios_elf_start, FS_SLC_WRITE1, ARM_B(FS_SLC_WRITE1, slcWrite1_patch));
        section_write_word(ios_elf_start, FS_SLC_WRITE2, ARM_B(FS_SLC_WRITE2, slcWrite2_patch));

        //section_write_word(ios_elf_start, FS_USB_READ, ARM_B(FS_USB_READ, usbRead_patch));
        //section_write_word(ios_elf_start, FS_USB_WRITE, ARM_B(FS_USB_WRITE, usbWrite_patch));
    }

    //section_write_word(ios_elf_start, 0x1070F87C, ARM_BL(0x1070F87C, FSA_AttachVolume_FillDescription_hook));
    //section_write_word(ios_elf_start, 0x10700EFC, ARM_BL(0x10700EFC, FSA_AsyncCommandCallback_hook));
    // patch mounting FAT and allow all devices instead of only SD card
    //section_write_word(ios_elf_start, 0x1078E074, 0xEA000002);
    // patch FSA_MakeQuota to not store command -> command is modified depending on wether it is USB FAT or not
    //section_write_word(ios_elf_start, 0x1070BE0C, 0xE1A00000);
    //section_write_word(ios_elf_start, 0x1070BE00, ARM_BL(0x1070BE00, FSA_MakeQuota_asm_hook));

    section_write_word(ios_elf_start, FS_CREATEDEVTHREAD_HOOK, ARM_B(FS_CREATEDEVTHREAD_HOOK, createDevThread_hook));

    u32 patch_count = (u32)(((u8*)fs_patches_table_end) - ((u8*)fs_patches_table)) / sizeof(patch_table_t);
    patch_table_entries(ios_elf_start, fs_patches_table, patch_count);

    //section_write_word(ios_elf_start, 0x10701F6C, ARM_BL(0x10701F6C, FSMakeQuota));
    //section_write_word(ios_elf_start, 0x10702764, ARM_BL(0x10702764, FSCreateDir));
    //section_write_word(ios_elf_start, 0x1070278C, ARM_BL(0x1070278C, FSChangeDir));
    //section_write_word(ios_elf_start, 0x107024B4, ARM_BL(0x107024B4, FSOpenFile));
    //section_write_word(ios_elf_start, 0x10703F4C, ARM_BL(0x10703F4C, FSWriteFileIssueCommand));
}
