#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "mlcio.h"
#include "sd_fat.h"
#include "text.h"
#include "hardware_registers.h"
#include "svc.h"

#define IO_BUFFER_SIZE 0x40000
#define IO_BUFFER_SPARE_SIZE (IO_BUFFER_SIZE+0x2000)

// the IO buffer is put behind everything else because there is no access to this region from IOS-FS it seems
unsigned char io_buffer[IO_BUFFER_SIZE]  __attribute__((aligned(0x40))) __attribute__((section(".io_buffer")));
unsigned char io_buffer_spare[IO_BUFFER_SPARE_SIZE]  __attribute__((aligned(0x40))) __attribute__((section(".io_buffer")));
unsigned long io_buffer_spare_pos;
int io_buffer_spare_status;

//! this one is required for the read function
static void slc_read_callback(int result, int priv)
{
    int *private_data = (int*)priv;
    private_data[1] = result;
    FS_SVC_RELEASEMUTEX(private_data[0]);
}

static int srcRead(void* deviceHandle, void *data_ptr, u32 offset, u32 sectors, int * result_array)
{
    int readResult = slcRead1_original(deviceHandle, 0, offset, sectors, SLC_BYTES_PER_SECTOR, data_ptr, slc_read_callback, (int)result_array);
    if(readResult == 0)
    {
        // wait for process to finish
        FS_SVC_ACQUIREMUTEX(result_array[0], 0);
        readResult = result_array[1];
    }
    return readResult;
}

int slc_dump(void *deviceHandle, const char* device, const char* filename, int y_offset)
{
	//also create a mutex for synchronization with end of operation...
    int sync_mutex = FS_SVC_CREATEMUTEX(1, 1);
    FS_SVC_ACQUIREMUTEX(sync_mutex, 0);

    int result_array[2];
    result_array[0] = sync_mutex;

    u32 offset = 0;
    int readResult = 0;
    int writeResult = 0;
    int result = -1;
    u32 readSize = IO_BUFFER_SPARE_SIZE / SLC_BYTES_PER_SECTOR;

    FS_SLEEP(1000);

    FL_FILE *file = fl_fopen(filename, "w");
	if (!file) {
        _printf(20, y_offset, "Failed to open %s for writing", filename);
		goto error;
	}

    do
    {
		_printf(20, y_offset, "%s     = %05X / 40000", device, offset);

        //! set flash erased byte to buffer
        FS_MEMSET(io_buffer_spare, 0xff, IO_BUFFER_SPARE_SIZE);
		io_buffer_spare_status = 0;
		io_buffer_spare_pos = 0;
        //readResult = readSlc(io_buffer, offset, (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR), deviceHandle);
        readResult = srcRead(deviceHandle, io_buffer, offset, readSize, result_array);

		if (readResult || io_buffer_spare_status || io_buffer_spare_pos != IO_BUFFER_SPARE_SIZE) {
			
			_printf(20, y_offset+10, "Failed to read flash block. read result: 0x%08X spare status: 0x%08X spare pos: 0x%08X", readResult, io_buffer_spare_status, io_buffer_spare_pos);
			goto error;
		}
		//FS_SLEEP(10);
		writeResult = fl_fwrite(io_buffer_spare, 1, readSize * SLC_BYTES_PER_SECTOR, file);
		if (writeResult != readSize * SLC_BYTES_PER_SECTOR) {
			_printf(20, y_offset + 10, "%s: Failed to write %d bytes to file %s (result: %d)!", device, readSize * SLC_BYTES_PER_SECTOR, file, filename, writeResult);
			goto error;
		}
		offset += readSize;
    }
    while (offset < SLC_SECTOR_COUNT);

	result = 0;

error:
    FS_SVC_DESTROYMUTEX(sync_mutex);

	if (file) {
		fl_fclose(file);
	}
    // last print to show "done"
    _printf(20, y_offset, "%s     = %05X / 40000", device, offset);
	return result;
}

int mlc_dump(u32 mlc_end, int y_offset)
{
    u32 offset = 0;

	int result = -1;
    int retry = 0;
    int mlc_result = 0;
    int callback_result = 0;
    int write_result = 0;
    int print_counter = 0;
	int current_file_index = 0;


	char filename[40];
    FL_FILE *file = NULL;
	
    do
    {
		if (!file) {
			FS_SNPRINTF(filename, sizeof(filename), "/mlc.bin.part%02d", ++current_file_index);
			file = fl_fopen(filename, "w");
			if (!file) {
				_printf(20, y_offset, "Failed to open %s for writing", filename);
				goto error;
			}
		}
        //! print only every 4th time
        if(print_counter == 0)
        {
            print_counter = 4;
            _printf(20, y_offset, "mlc         = %08X / %08X, mlc res %08X, retry %d", offset, mlc_end, mlc_result, retry);
        }
        else
        {
            --print_counter;
        }

        //! set flash erased byte to buffer
        FS_MEMSET(io_buffer, 0xff, IO_BUFFER_SIZE);
        mlc_result = sdcard_readwrite(SDIO_READ, io_buffer, (IO_BUFFER_SIZE / MLC_BYTES_PER_SECTOR), MLC_BYTES_PER_SECTOR, offset, &callback_result, DEVICE_ID_MLC);

        if((mlc_result == 0) && (callback_result != 0))
        {
            mlc_result = callback_result;
        }

        //! retry 5 times as there are read failures in several places
        if((mlc_result != 0) && (retry < 5))
        {
            FS_SLEEP(100);
            retry++;
            print_counter = 0; // print errors directly
        }
        else
        {
			write_result = fl_fwrite(io_buffer, 1, IO_BUFFER_SIZE, file);
			if (write_result != IO_BUFFER_SIZE) {
				_printf(20, y_offset + 10, "mlc: Failed to write %d bytes to file %s (result: %d)!", IO_BUFFER_SIZE, file, filename, write_result);
				goto error;
			}
			offset += (IO_BUFFER_SIZE / MLC_BYTES_PER_SECTOR);
			if ((offset % 0x400000) == 0) {
				fl_fclose(file);
				file = NULL;
			}
        }
    }
    while(offset < mlc_end); //! TODO: make define MLC32_SECTOR_COUNT:

	result = 0;

error:
	if (file) {
		fl_fclose(file);
	}
    // last print to show "done"
    _printf(20, y_offset, "mlc         = %08X / %08X, mlc res %08X, retry %d", offset, mlc_end, mlc_result, retry);

	return result;
}

int check_nand_type(void)
{
    //! check if MLC size is > 8GB
    if( FS_MMC_MLC_STRUCT[0x30/4] > 0x1000000)
    {
        return MLC_NAND_TYPE_32GB;
    }
    else
    {
        return MLC_NAND_TYPE_8GB;
    }
}

/*static void wait_format_confirmation(void)
{
    int timeout = 600;
    //"Press the POWER button SD then , else the console will reboot in %u seconds."
    while(1)
    {
        _printf(20, 30, "No NAND dump detected. SD Format and complete NAND dump required.");
        _printf(20, 40, "Press the POWER button to format SD card otherwise the console will reboot in %d seconds.", timeout/10);

        if(svcRead32(LT_GPIO_IN) & GPIO_IN_POWER_BUTTON)
        {
            break;
        }

        if(--timeout == 0)
        {
            FS_SLEEP(1000);
            svcShutdown(SHUTDOWN_TYPE_REBOOT);
        }

        FS_SLEEP(100);
    }

    // clear the lines
    clearLine(30, 0x000000FF);
    clearLine(40, 0x000000FF);
}*/

void dump_nand_complete()
{
	int offset_y = 30;
    _printf(20, offset_y, "Init SD card....");
	if ( InitSDCardFAT32() != 0 ) {
        FS_SLEEP(3000);
        svcShutdown(SHUTDOWN_TYPE_REBOOT);
	}
    _printf(20, offset_y, "Init SD card.... Success!");
	offset_y += 10;

	int * dumpSlc =  (int *)(0x107f8200 - 4);
	int * dumpSlccmpt =  (int *)(0x107f8200 - 8);
	int * dumpMlc =  (int *)(0x107f8200 - 12);


    //wait_format_confirmation();

	u32 mlc_sector_count = 0;
	if (*dumpMlc) {
		mlc_init();
		FS_SLEEP(1000);

		int nand_type = check_nand_type();
		//u32 sdio_sector_count = FS_MMC_SDCARD_STRUCT[0x30/4];
		mlc_sector_count = FS_MMC_MLC_STRUCT[0x30/4];
		//u32 fat32_partition_offset = (MLC_BASE_SECTORS + mlc_sector_count);

		_printf(20, offset_y, "Detected %d GB MLC NAND type.", (nand_type == MLC_NAND_TYPE_8GB) ? 8 : 32);
		offset_y += 10;
	}
	offset_y += 10;

    /*if(sdio_sector_count < fat32_partition_offset)
    {
        _printf(20, 40, "SD card too small! Required sectors %u > available %u.", fat32_partition_offset, sdio_sector_count);
        FS_SLEEP(3000);
        svcShutdown(SHUTDOWN_TYPE_REBOOT);
    }

    if( FormatSDCard(fat32_partition_offset, sdio_sector_count) < 0 )
    {
        FS_SLEEP(3000);
        svcShutdown(SHUTDOWN_TYPE_REBOOT);
    }*/

	if (*dumpSlc) {
		if (slc_dump(FS_SLC_PHYS_DEV_STRUCT,     "slc    ", "/slc.bin", offset_y))
			goto error;
		offset_y += 10;
	}
	if (*dumpSlccmpt) {
		if (slc_dump(FS_SLCCMPT_PHYS_DEV_STRUCT, "slccmpt", "/slccmpt.bin", offset_y))
			goto error;
		offset_y += 10;
	}
	if (*dumpMlc) {
		if (mlc_dump(mlc_sector_count, offset_y))
			goto error;
		offset_y += 10;
	}
	offset_y += 20;

    _printf(20, offset_y, "Complete! -> rebooting into sysNAND...");

    FS_SLEEP(3000);
    svcShutdown(SHUTDOWN_TYPE_REBOOT);

error:
	offset_y += 20;
    _printf(20, offset_y, "Error! -> rebooting into sysNAND...");

    FS_SLEEP(3000);
    svcShutdown(SHUTDOWN_TYPE_REBOOT);
}

#if 0
// debug and not used at the moment
void dump_data(void* data_ptr, u32 size)
{
    static u32 dumpdata_offset = 0;

    u32 num_sectors = size >> 9; // size / SDIO_BYTES_PER_SECTOR but faster ;)
    if (num_sectors == 0)
        num_sectors = 1;

    sdcard_readwrite(SDIO_WRITE, data_ptr, num_sectors, SDIO_BYTES_PER_SECTOR, DUMPDATA_BASE_SECTORS + dumpdata_offset, NULL, DEVICE_ID_SDCARD_PATCHED);
    dumpdata_offset += num_sectors;
}

void dump_lots_data(u8* addr, u32 size)
{
    u32 cur_size;
    u32 size_remaining = size;
    u8* cur_addr = addr;
    do
    {
        cur_size = sizeof(io_buffer);
        if (cur_size > size_remaining)
            cur_size = size_remaining;

        FS_MEMCPY(io_buffer, cur_addr, cur_size);
        dump_data(io_buffer, cur_size);

        cur_addr += cur_size;
        size_remaining -= cur_size;
    }
    while (cur_size != 0);
}

void dump_syslog()
{
    FS_MEMCPY(io_buffer, *(void**)0x05095ECC, sizeof(io_buffer));
    sdcard_readwrite(SDIO_WRITE, io_buffer, sizeof(io_buffer) / SDIO_BYTES_PER_SECTOR, SDIO_BYTES_PER_SECTOR, SYSLOG_BASE_SECTORS, NULL, DEVICE_ID_SDCARD_PATCHED);
}
#endif
