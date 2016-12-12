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
#include "elf_patcher.h"
#include "kernel_patches.h"
#include "ios_mcp_patches.h"
#include "ios_fs_patches.h"
#include "ios_bsp_patches.h"
#include "config.h"
#include "fsa.h"
#include "utils.h"

#define USB_PHYS_CODE_BASE      0x101312D0

cfw_config_t cfw_config;

typedef struct
{
    u32 size;
    u8 data[0];
} payload_info_t;

static const char repairData_set_fault_behavior[] = {
	0xE1,0x2F,0xFF,0x1E,0xE9,0x2D,0x40,0x30,0xE5,0x93,0x20,0x00,0xE1,0xA0,0x40,0x00,
	0xE5,0x92,0x30,0x54,0xE1,0xA0,0x50,0x01,0xE3,0x53,0x00,0x01,0x0A,0x00,0x00,0x02,
	0xE1,0x53,0x00,0x00,0xE3,0xE0,0x00,0x00,0x18,0xBD,0x80,0x30,0xE3,0x54,0x00,0x0D,
};
static const char repairData_set_panic_behavior[] = {
	0x08,0x16,0x6C,0x00,0x00,0x00,0x18,0x0C,0x08,0x14,0x40,0x00,0x00,0x00,0x9D,0x70,
	0x08,0x16,0x84,0x0C,0x00,0x00,0xB4,0x0C,0x00,0x00,0x01,0x01,0x08,0x14,0x40,0x00,
	0x08,0x15,0x00,0x00,0x08,0x17,0x21,0x80,0x08,0x17,0x38,0x00,0x08,0x14,0x30,0xD4,
	0x08,0x14,0x12,0x50,0x08,0x14,0x12,0x94,0xE3,0xA0,0x35,0x36,0xE5,0x93,0x21,0x94,
	0xE3,0xC2,0x2E,0x21,0xE5,0x83,0x21,0x94,0xE5,0x93,0x11,0x94,0xE1,0x2F,0xFF,0x1E,
	0xE5,0x9F,0x30,0x1C,0xE5,0x9F,0xC0,0x1C,0xE5,0x93,0x20,0x00,0xE1,0xA0,0x10,0x00,
	0xE5,0x92,0x30,0x54,0xE5,0x9C,0x00,0x00,
};
static const char repairData_usb_root_thread[] = {
	0xE5,0x8D,0xE0,0x04,0xE5,0x8D,0xC0,0x08,0xE5,0x8D,0x40,0x0C,0xE5,0x8D,0x60,0x10,
	0xEB,0x00,0xB2,0xFD,0xEA,0xFF,0xFF,0xC9,0x10,0x14,0x03,0xF8,0x10,0x62,0x4D,0xD3,
	0x10,0x14,0x50,0x00,0x10,0x14,0x50,0x20,0x10,0x14,0x00,0x00,0x10,0x14,0x00,0x90,
	0x10,0x14,0x00,0x70,0x10,0x14,0x00,0x98,0x10,0x14,0x00,0x84,0x10,0x14,0x03,0xE8,
	0x10,0x14,0x00,0x3C,0x00,0x00,0x01,0x73,0x00,0x00,0x01,0x76,0xE9,0x2D,0x4F,0xF0,
	0xE2,0x4D,0xDE,0x17,0xEB,0x00,0xB9,0x92,0xE3,0xA0,0x10,0x00,0xE3,0xA0,0x20,0x03,
	0xE5,0x9F,0x0E,0x68,0xEB,0x00,0xB3,0x20,
};

void kernel_launch_ios(u32 launch_address, u32 L, u32 C, u32 H)
{
    void (*kernel_launch_bootrom)(u32 launch_address, u32 L, u32 C, u32 H) = (void*)0x0812A050;

    if(*(u32*)(launch_address - 0x300 + 0x1AC) == 0x00DFD000)
    {
        int(*disable_interrupts)() = (int(*)())0x0812E778;
        int(*enable_interrupts)(int) = (int(*)(int))0x0812E78C;

        int level = disable_interrupts();
        unsigned int control_register = disable_mmu();

        u32 ios_elf_start = launch_address + 0x804 - 0x300;

        //! try to keep the order of virt. addresses to reduce the memmove amount
        mcp_run_patches(ios_elf_start);
        kernel_run_patches(ios_elf_start);

        if(cfw_config.redNAND)
        {
            fs_run_patches(ios_elf_start);

            if(cfw_config.seeprom_red)
                bsp_run_patches(ios_elf_start);
        }

        restore_mmu(control_register);
        enable_interrupts(level);
    }

    kernel_launch_bootrom(launch_address, L, C, H);
}


int BSP_EEPROM_ReadData(void *buffer, int offset, int size);

int _main()
{
	int(*disable_interrupts)() = (int(*)())0x0812E778;
	int(*enable_interrupts)(int) = (int(*)(int))0x0812E78C;
	void(*invalidate_icache)() = (void(*)())0x0812DCF0;
	void(*invalidate_dcache)(unsigned int, unsigned int) = (void(*)())0x08120164;
	void(*flush_dcache)(unsigned int, unsigned int) = (void(*)())0x08120160;

	flush_dcache(0x081200F0, 0x4001); // giving a size >= 0x4000 flushes all cache

	int level = disable_interrupts();

	unsigned int control_register = disable_mmu();

	/* Save the request handle so we can reply later */
	*(volatile u32*)0x0012F000 = *(volatile u32*)0x1016AD18;

	/* Patch kernel_error_handler to BX LR immediately */
	*(volatile u32*)0x08129A24 = 0xE12FFF1E;

	void * pset_fault_behavior = (void*)0x081298BC;
	kernel_memcpy(pset_fault_behavior, (void*)repairData_set_fault_behavior, sizeof(repairData_set_fault_behavior));

	void * pset_panic_behavior = (void*)0x081296E4;
	kernel_memcpy(pset_panic_behavior, (void*)repairData_set_panic_behavior, sizeof(repairData_set_panic_behavior));

	void * pusb_root_thread = (void*)0x10100174;
	kernel_memcpy(pusb_root_thread, (void*)repairData_usb_root_thread, sizeof(repairData_usb_root_thread));

    payload_info_t *payloads = (payload_info_t*)0x00148000;

	kernel_memcpy((void*)&cfw_config, payloads->data, payloads->size);
    payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

	kernel_memcpy((void*)USB_PHYS_CODE_BASE, payloads->data, payloads->size);
    payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

    if(cfw_config.redNAND)
    {
        kernel_memcpy((void*)fs_get_phys_code_base(), payloads->data, payloads->size);
        payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

        if(cfw_config.seeprom_red)
        {
            kernel_memcpy((void*)bsp_get_phys_code_base(), payloads->data, payloads->size);
            payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );
        }
    }

	kernel_memcpy((void*)mcp_get_phys_code_base(), payloads->data, payloads->size);
    payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

    if(cfw_config.launchImage)
    {
	    kernel_memcpy((void*)MCP_LAUNCH_IMG_PHYS_ADDR, payloads->data, payloads->size);
        payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );
    }
    else
    {
        *(u32*)MCP_LAUNCH_IMG_PHYS_ADDR = 0;
    }

    // patch FSA raw access
	*(volatile u32*)0x1070FAE8 = 0x05812070;
	*(volatile u32*)0x1070FAEC = 0xEAFFFFF9;

	*(volatile u32*)0x0812A120 = ARM_BL(0x0812A120, kernel_launch_ios);
	*(volatile u32*)(0x1555500) = 0;

	/* REENABLE MMU */
	restore_mmu(control_register);

	invalidate_dcache(0x081298BC, 0x4001); // giving a size >= 0x4000 invalidates all cache
	invalidate_icache();

	enable_interrupts(level);

    if(cfw_config.redNAND)
    {
        int seepromDumpFound = 0;
        u32 seepromDumpBaseSector = 0x4FF;
        int otpDumpFound = 0;
        u32 otpDumpBaseSector = 0x4FD;
        int writeInfoSector = 0;
        sdio_nand_signature_sector_t *infoSector = (sdio_nand_signature_sector_t*)0x00141000;
        kernel_memset(infoSector, 0x00, 0x200);

        FSA_SDReadRawSectors(infoSector, NAND_DUMP_SIGNATURE_SECTOR, 1);

        if(infoSector->signature == NAND_DUMP_SIGNATURE)
        {
            int i;
            for(i = 0; i < 6; i++)
            {
                if(infoSector->nand_descriptions[i].nand_type == NAND_DESC_TYPE_SEEPROM)
                {
                    seepromDumpFound = 1;
                    seepromDumpBaseSector = infoSector->nand_descriptions[i].base_sector;
                }
                if(infoSector->nand_descriptions[i].nand_type == NAND_DESC_TYPE_OTP)
                {
                    otpDumpFound = 1;
                    otpDumpBaseSector = infoSector->nand_descriptions[i].base_sector;
                }
            }
        }

        if(cfw_config.seeprom_red)
        {
            bsp_init_seeprom_buffer(seepromDumpBaseSector, seepromDumpFound);

            if(seepromDumpBaseSector == 0)
            {
                infoSector->nand_descriptions[3].nand_type = NAND_DESC_TYPE_SEEPROM;
                infoSector->nand_descriptions[3].base_sector = seepromDumpBaseSector;
                infoSector->nand_descriptions[3].sector_count = 1;
                writeInfoSector++;
            }
        }

        if(cfw_config.otp_red)
        {
            kernel_init_otp_buffer(otpDumpBaseSector, otpDumpFound);

            if(otpDumpFound == 0)
            {
                infoSector->nand_descriptions[4].nand_type = NAND_DESC_TYPE_OTP;
                infoSector->nand_descriptions[4].base_sector = otpDumpBaseSector;
                infoSector->nand_descriptions[4].sector_count = 2;
                writeInfoSector++;
            }
        }


        if(writeInfoSector > 1)
        {
            FSA_SDWriteRawSectors(infoSector, NAND_DUMP_SIGNATURE_SECTOR, 1);
        }
    }

	return 0;
}
