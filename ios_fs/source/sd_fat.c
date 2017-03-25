#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "text.h"
#include "sd_fat.h"

unsigned char sd_io_buffer[0x40000]  __attribute__((aligned(0x40))) __attribute__((section(".io_buffer")));
unsigned int fat_sector = 0;

#define PARTITION_TYPE_FAT32		0x0c
#define PARTITION_TYPE_FAT32_CHS	0x0b

#define MBR_SIGNATURE				0x55AA
#define EBR_SIGNATURE				MBR_SIGNATURE

#define PARTITION_BOOTABLE			0x80 /* Bootable (active) */
#define PARTITION_NONBOOTABLE		0x00 /* Non-bootable */
#define PARTITION_TYPE_GPT			0xEE /* Indicates that a GPT header is available */

typedef struct _PARTITION_RECORD {
	u8 status;							  /* Partition status; see above */
	u8 chs_start[3];						/* Cylinder-head-sector address to first block of partition */
	u8 type;								/* Partition type; see above */
	u8 chs_end[3];						  /* Cylinder-head-sector address to last block of partition */
	u32 lba_start;						  /* Local block address to first sector of partition */
	u32 block_count;						/* Number of blocks in partition */
} __attribute__((__packed__)) PARTITION_RECORD;


typedef struct _MASTER_BOOT_RECORD {
	u8 code_area[446];					  /* Code area; normally empty */
	PARTITION_RECORD partitions[4];		 /* 4 primary partitions */
	u16 signature;						  /* MBR signature; 0xAA55 */
} __attribute__((__packed__)) MASTER_BOOT_RECORD;

int sd_fat_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
	// It seems that sdcard_readwrite overwrite more bytes than requested with 0xff.... So I use io_buffer to read it.
	if (sector_count * SDIO_BYTES_PER_SECTOR > sizeof(sd_io_buffer)) return 0;
	int result = sdcard_readwrite(SDIO_READ, sd_io_buffer, sector_count, SDIO_BYTES_PER_SECTOR, fat_sector + sector, NULL, DEVICE_ID_SDCARD_PATCHED);
	if (result) return 0;
	memcpy(buffer, sd_io_buffer, sector_count * SDIO_BYTES_PER_SECTOR);
	return 1;
}

int sd_fat_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
	if (sector_count * SDIO_BYTES_PER_SECTOR > sizeof(sd_io_buffer)) return 0;
	int result = sdcard_readwrite(SDIO_WRITE, buffer, sector_count, SDIO_BYTES_PER_SECTOR, fat_sector + sector, NULL, DEVICE_ID_SDCARD_PATCHED);
	return result ? 0 : 1;
}

int InitSDCardFAT32()
{
	MASTER_BOOT_RECORD *mbr = (MASTER_BOOT_RECORD*)sd_io_buffer;
	memset(mbr, 0, SDIO_BYTES_PER_SECTOR);

	int result = sdcard_readwrite(SDIO_READ, mbr, 1, SDIO_BYTES_PER_SECTOR, 0, NULL, DEVICE_ID_SDCARD_PATCHED);
	if(result != 0)
	{
		_printf(20, 40, "SD card read failed %i", result);
		return result;
	}

	if (mbr->signature != MBR_SIGNATURE) {
		_printf(20, 40, "SD not MBR");
		return -1;
	}

	if (mbr->partitions[0].status != PARTITION_BOOTABLE && mbr->partitions[0].status != PARTITION_NONBOOTABLE) {
		_printf(20, 40, "Invalid paritition status");
		return -1;
	}

	if (mbr->partitions[0].type != PARTITION_TYPE_FAT32 && mbr->partitions[0].type != PARTITION_TYPE_FAT32_CHS) {
		_printf(20, 40, "First paritition not FAT32");
		return -1;
	}

	fat_sector = le32(mbr->partitions[0].lba_start);
	
	fl_init();

	result = fl_attach_media(sd_fat_read, sd_fat_write);
	if (result != FAT_INIT_OK)
	{
		_printf(20, 40, "FAT32 attach failed %i", result);
		return result; 
	}

	return 0;
}
