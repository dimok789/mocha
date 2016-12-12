#include <stdio.h>
#include "types.h"
#include "devices.h"
#include "imports.h"
#include "sdio.h"
#include "text.h"

void * getMdDeviceById(int deviceId)
{
    if(deviceId == DEVICE_ID_SDCARD_PATCHED)
    {
        return (void*)FS_MMC_SDCARD_STRUCT;
    }
    else if(deviceId == DEVICE_ID_MLC)
    {
        return (void*)FS_MMC_MLC_STRUCT;
    }
    return NULL;
}

int registerMdDevice_hook(void * md, int arg2, int arg3)
{
    u32 *mdStruct = (u32*)md;

    if((md != 0) && (mdStruct[2] == (u32)FS_MMC_SDCARD_STRUCT))
    {
        sdcard_lock_mutex();
        FS_MMC_SDCARD_STRUCT[0x24/4] = FS_MMC_SDCARD_STRUCT[0x24/4] & (~0x20);

        int result = FS_REGISTERMDPHYSICALDEVICE(md, arg2, arg3);

        sdcard_unlock_mutex();

        return result;
    }

    return FS_REGISTERMDPHYSICALDEVICE(md, arg2, arg3);
}

int getPhysicalDeviceHandle(u32 device)
{
    u32 handleSize = 0x204;
    u8 *handleBase = (u8*)(0x1091C2EC + device * handleSize);
    u16 adrLow = (*(u16*)&handleBase[6]);
    return ((device << 16) | adrLow);
}

//! read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
int readWriteCallback_patch(int is_read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    int result_arg = 0;
    int result = sdcard_readwrite(is_read, data_outptr, cnt, block_size, offset_offset + offset_low, &result_arg, DEVICE_ID_SDCARD_PATCHED);

    if((result == 0) && (callback != 0))
    {
        callback(result_arg, callback_parameter);
    }
    return result;
}

//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! USB redirection
//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int usbReadWrite_patch(int is_read, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return readWriteCallback_patch(is_read, USB_BASE_SECTORS, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int usbRead_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return usbReadWrite_patch(SDIO_READ, offset_high, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int usbWrite_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return usbReadWrite_patch(SDIO_WRITE, offset_high, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! SDIO redirection
//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int sdcardReadWrite_patch(void *physical_device_info, int is_read, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    u32 offset_offset;
    u32 *phys_dev = (u32*)physical_device_info;

    if(phys_dev[0x14/4] != DEVICE_TYPE_SDCARD)
    {
        offset_offset = MLC_BASE_SECTORS;
    }
    else
    {
        offset_offset = 0;
    }

    return readWriteCallback_patch(is_read, offset_offset, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int sdcardRead_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return sdcardReadWrite_patch(physical_device_info, SDIO_READ, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int sdcardWrite_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return sdcardReadWrite_patch(physical_device_info, SDIO_WRITE, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! SLC redirection
//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static int slcReadWrite_patch(void *physical_device_info, int is_read, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    u32 offset_offset;
    u32 *phys_dev = (u32*)physical_device_info;

    if(phys_dev[1] != 0)
    {
        // physical_device_info = 0x11C381CC
        offset_offset = (u32)(((u64)SLC_BASE_SECTORS * (u64)SDIO_BYTES_PER_SECTOR) / SLC_BYTES_PER_SECTOR);
    }
    else
    {
        // physical_device_info = 0x11C37668
        offset_offset = (u32)(((u64)SLCCMPT_BASE_SECTORS * (u64)SDIO_BYTES_PER_SECTOR) / SLC_BYTES_PER_SECTOR);
    }

    return readWriteCallback_patch(is_read, offset_offset, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int slcRead1_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return slcReadWrite_patch(physical_device_info, SDIO_READ, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int slcWrite1_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    return slcReadWrite_patch(physical_device_info, SDIO_WRITE, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int slcRead2_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, int ukn1, void *data_outptr, int ukn2, read_write_callback_t callback, int callback_parameter)
{
    return slcReadWrite_patch(physical_device_info, SDIO_READ, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

int slcWrite2_patch(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, int ukn1, void *data_outptr, int ukn2, read_write_callback_t callback, int callback_parameter)
{
    return slcReadWrite_patch(physical_device_info, SDIO_WRITE, offset_low, cnt, block_size, data_outptr, callback, callback_parameter);
}

