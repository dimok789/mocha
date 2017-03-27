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
#include "config.h"
#include "utils.h"
#include "redirection_setup.h"
#include "ios_mcp_patches.h"
#include "ios_fs_patches.h"
#include "ios_bsp_patches.h"
#include "instant_patches.h"

#define USB_PHYS_CODE_BASE      0x101312D0

cfw_config_t cfw_config;

unsigned char otp_buffer[0x400];
unsigned char seeprom_buffer[0x400];

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

// based on mini implementation by sven peter
#define	LT_TIMER            0x0d800010
#define LT_GPIO_OUT         0x0d8000e0
#define LT_GPIO_IN          0x0d8000e8

#define EEPROM_SPI_CS       0x400
#define EEPROM_SPI_SCK      0x800
#define EEPROM_SPI_MOSI     0x1000
#define EEPROM_SPI_MISO     0x2000

#define HW_REG(reg)         (*(volatile unsigned int*)(reg))

static inline void _delay(u32 ticks)
{
	u32 now = HW_REG(LT_TIMER);
	while((HW_REG(LT_TIMER) - now) < ticks);
}

static u32 spi_read(int bit_count)
{
	u32 word = 0;
	while(bit_count--)
	{
		word <<= 1;

		HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_SCK;
		_delay(9);

		HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
		_delay(9);

		word |= ((HW_REG(LT_GPIO_IN) & EEPROM_SPI_MISO) != 0);
	}
	return word;
}

static void spi_write(u32 word, int bit_count)
{
	while(bit_count--)
	{
		if(word & (1 << bit_count))
        {
			HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_MOSI;
        }
		else
        {
			HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_MOSI;
        }
		_delay(9);

		HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_SCK;
		_delay(9);

		HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
		_delay(9);
	}
}

static int seeprom_read(u16 *dst, int offset, int size)
{
	int i;
    offset >>= 1;
    size >>= 1;

    HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_SCK;
    HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_CS;
	_delay(9);

	for(i = 0; i < size; ++i, ++offset)
	{
		HW_REG(LT_GPIO_OUT) |= EEPROM_SPI_CS;
		spi_write((0x600 | offset), 11);

		dst[i] = spi_read(16);

		HW_REG(LT_GPIO_OUT) &= ~EEPROM_SPI_CS;
		_delay(9);
	}

	return size;
}

int _main()
{
	void(*invalidate_icache)() = (void(*)())0x0812DCF0;
	void(*invalidate_dcache)(unsigned int, unsigned int) = (void(*)())0x08120164;
	void(*flush_dcache)(unsigned int, unsigned int) = (void(*)())0x08120160;
    int (*orig_kernel_read_otp_internal)(int index, void* out_buf, u32 size) = (void*)0x08120248;

	// We don't have the config yet, so read it anyway
	orig_kernel_read_otp_internal(0, otp_buffer, 0x400);

    flush_dcache(0x081200F0, 0x4001); // giving a size >= 0x4000 flushes all cache

	int level = disable_interrupts();

	// We don't have the config yet, so read it anyway
	seeprom_read(seeprom_buffer, 0, 0x200);

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

    //if(cfw_config.redNAND)
    //{
    kernel_memcpy((void*)fs_get_phys_code_base(), payloads->data, payloads->size);
    payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

    //    if(cfw_config.seeprom_red)
    //    {
    //        kernel_memcpy((void*)bsp_get_phys_code_base(), payloads->data, payloads->size);
    //        payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );
    //    }
    //}

	kernel_memcpy((void*)mcp_get_phys_code_base(), payloads->data, payloads->size);
    payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );

    /*if(cfw_config.launchImage)
    {
	    kernel_memcpy((void*)MCP_LAUNCH_IMG_PHYS_ADDR, payloads->data, payloads->size);
        payloads = (payload_info_t*)( ((char*)payloads) + ALIGN4(sizeof(payload_info_t) + payloads->size) );
    }*/

    // run all instant patches as necessary
    instant_patches_setup();

	*(volatile u32*)(0x1555500) = 0;

	/* REENABLE MMU */
	restore_mmu(control_register);

	invalidate_dcache(0x081298BC, 0x4001); // giving a size >= 0x4000 invalidates all cache
	invalidate_icache();

	enable_interrupts(level);

    /*if(cfw_config.redNAND)
    {
	    redirection_setup();
    }*/

	return 0;
}
