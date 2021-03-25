#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "text.h"
#include "sd_fat.h"

unsigned char sd_io_buffer[0x40000]  __attribute__((aligned(0x40))) __attribute__((section(".io_buffer")));

int sd_fat_read(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
    // It seems that sdcard_readwrite overwrite more bytes than requested with 0xff.... So I use io_buffer to read it.
    if (sector_count * SDIO_BYTES_PER_SECTOR > sizeof(sd_io_buffer)) return 0;
    int result = sdcard_readwrite(SDIO_READ, sd_io_buffer, sector_count, SDIO_BYTES_PER_SECTOR, sector, NULL, DEVICE_ID_SDCARD_PATCHED);
    if (result) return 0;
    memcpy(buffer, sd_io_buffer, sector_count * SDIO_BYTES_PER_SECTOR);
    return 1;
}

int sd_fat_write(unsigned long sector, unsigned char *buffer, unsigned long sector_count)
{
    if (sector_count * SDIO_BYTES_PER_SECTOR > sizeof(sd_io_buffer)) return 0;
    int result = sdcard_readwrite(SDIO_WRITE, buffer, sector_count, SDIO_BYTES_PER_SECTOR, sector, NULL, DEVICE_ID_SDCARD_PATCHED);
    return result ? 0 : 1;
}

int InitSDCardFAT32()
{
    fl_init();

    int result = fl_attach_media(sd_fat_read, sd_fat_write);
    if (result != FAT_INIT_OK)
    {
        _printf(20, 40, "FAT32 attach failed %d", result);
        return result; 
    }

    return 0;
}
