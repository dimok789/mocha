#include "types.h"
#include "imports.h"

static int sdcard_access_mutex = 0;
static u32 dumpdata_offset = 0;

typedef struct _sd_command_block_t
{
    u32 cnt;
    u32 block_size;
    u32 command_type;
    void * data_ptr;
    u64 offset;
    void *callback;
    void *callback_arg;
    int minus_one;
} __attribute__((packed)) sd_command_block_t;

void sdcard_init(void)
{
	// this should run *after* /dev/mmc thread is created
	// first we create our synchronization stuff
    sdcard_access_mutex = FS_SVC_CREATEMUTEX(1, 1);

    dumpdata_offset = 0;

	// then we sleep until /dev/mmc is done initializing sdcard (TODO : better synchronization here)
	FS_SLEEP(1000);

	// finally we set some flags to indicate sdcard is ready for use
    FS_MMC_SDCARD_STRUCT[0x24/4] = FS_MMC_SDCARD_STRUCT[0x24/4] | 0x20;
    FS_MMC_SDCARD_STRUCT[0x28/4] = FS_MMC_SDCARD_STRUCT[0x28/4] & (~0x04);
}

static void sdcard_readwrite_callback(void *priv_data, int result)
{
    int *private_data = (int*)priv_data;

    private_data[1] = result;

    FS_SVC_RELEASEMUTEX(private_data[0]);
}

void sdcard_lock_mutex(void)
{
    FS_SVC_ACQUIREMUTEX(sdcard_access_mutex, 0);
}

void sdcard_unlock_mutex(void)
{
    FS_SVC_RELEASEMUTEX(sdcard_access_mutex);
}

int sdcard_readwrite(int is_read, void *data, u32 cnt, u32 block_size, u32 offset_blocks, int * out_callback_arg, int device_id)
{
	// first of all, grab sdcard mutex
    sdcard_lock_mutex();

	//also create a mutex for synchronization with end of operation...
    int sync_mutex = FS_SVC_CREATEMUTEX(1, 1);

    // ...and acquire it
    FS_SVC_ACQUIREMUTEX(sync_mutex, 0);

    // block_size needs to be equal to sector_size (0x200)
    while(block_size > 0x200)
    {
        block_size >>= 1;
        cnt <<= 1;
        offset_blocks <<= 1;
    }

	// build rw command paramstruct
    sd_command_block_t command;
    command.cnt = cnt;
    command.block_size = block_size;
    command.command_type = (is_read ? 0x03 : 0x00);
    command.data_ptr = data;
    command.offset = offset_blocks;
    command.callback = 0x00;
    command.callback_arg = 0x00;
    command.minus_one = (u32)-1;

    // setup parameters
    int private_data[2];
    private_data[0] = sync_mutex;
    private_data[1] = 0;

    // call readwrite function
    int result = FS_SDIO_DOREADWRITECOMMAND(device_id, &command, offset_blocks, sdcard_readwrite_callback, (void*)private_data);
    if(result == 0)
    {
        // wait for callback to give the go-ahead
        FS_SVC_ACQUIREMUTEX(sync_mutex, 0);

        if(out_callback_arg)
        {
            *out_callback_arg = private_data[1];
        }
    }

    // finally, release sdcard mutexes
    FS_SVC_DESTROYMUTEX(sync_mutex);
    sdcard_unlock_mutex();

    return result;
}
