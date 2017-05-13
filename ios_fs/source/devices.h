#ifndef DEVICES_H_
#define DEVICES_H_

#define DEVICE_ID_SDCARD_REAL               0x43
#define DEVICE_ID_SDCARD_PATCHED            0xDA

#define DEVICE_ID_MLC                       0xAB

#define SDIO_BYTES_PER_SECTOR               512
#define MLC_BYTES_PER_SECTOR                512
#define SLC_BYTES_PER_SECTOR                2048

#define SLC_BASE_SECTORS                    (0x000500)
#define SLCCMPT_BASE_SECTORS                (0x100500)
#define MLC_BASE_SECTORS                    (0x200500)

#define USB_BASE_SECTORS                    (0x2720000)
#define SYSLOG_BASE_SECTORS                 (0x6D00000)
#define DUMPDATA_BASE_SECTORS               (SYSLOG_BASE_SECTORS + (0x40000 / SDIO_BYTES_PER_SECTOR))

#define SLC_SECTOR_COUNT                    0x40000
#define MLC_8GB_SECTOR_COUNT                0xE90000
#define MLC_32GB_SECTOR_COUNT               0x3A3E000  //0x3A20000

#define MLC_NAND_TYPE_32GB                  0
#define MLC_NAND_TYPE_8GB                   1

#define NAND_DUMP_SIGNATURE_SECTOR          0x01
#define NAND_DUMP_SIGNATURE                 0x4841585844554d50ULL // HAXXDUMP

#define NAND_DESC_TYPE_SLC                  0x534c4320 // 'SLC '
#define NAND_DESC_TYPE_SLCCMPT              0x534c4332 // 'SLC2'
#define NAND_DESC_TYPE_MLC                  0x4d4c4320 // 'MLC '

enum NodeTypes
{
    NODE_TYPE_DEV_DF         = 0,
    NODE_TYPE_DEV_ATFS       = 1,
    NODE_TYPE_DEV_ISFS       = 2,
    NODE_TYPE_DEV_WFS        = 3,
    NODE_TYPE_DEV_PCFS       = 4,
    NODE_TYPE_DEV_RBFS       = 5,
    NODE_TYPE_DEV_FAT        = 6,
    NODE_TYPE_DEV_FLA        = 7,
    NODE_TYPE_DEV_UMS        = 8,
    NODE_TYPE_DEV_AHCIMGR    = 9,
    NODE_TYPE_DEV_SHDD       = 10,
    NODE_TYPE_DEV_MD         = 11,
    NODE_TYPE_DEV_SCFM       = 12,
    NODE_TYPE_DEV_MMC        = 13,
    NODE_TYPE_DEV_TIMETRACE  = 14,
    NODE_TYPE_DEV_TCP_PCFS   = 15
};

enum DeviceTypes
{
    DEVICE_TYPE_SI          = 0x01,
    DEVICE_TYPE_ODD         = 0x02,
    DEVICE_TYPE_SLCCMPT     = 0x03,
    DEVICE_TYPE_SLC         = 0x04,
    DEVICE_TYPE_MLC         = 0x05,
    DEVICE_TYPE_SDCARD      = 0x06,
    DEVICE_TYPE_SD          = 0x07,
    DEVICE_TYPE_HFIO        = 0x08,
    DEVICE_TYPE_RAMDISK     = 0x09,
    DEVICE_TYPE_USB         = 0x11,
    DEVICE_TYPE_MLCORIG     = 0x12
};

enum FsTypes
{
    FS_TYPE_RAW             = 0x8003,
    FS_TYPE_FAT             = 0x8004,
    FS_TYPE_WFS             = 0x8005,
    FS_TYPE_ISFS            = 0x8006,
    FS_TYPE_ATFS            = 0x8007
};

typedef struct _fs_attach_info_t
{
    struct _fs_attach_info_t* next;
    u16 fs_type;
    u16 unkn_flags2;
    u32 zeros[2];
    int (*fsAttach)(void *dev_struct, int unkn_one);
    u8 allowed_devices[0x0C];
} fs_attach_info_t;

typedef struct _stdio_nand_desc_t
{
    u32 nand_type;                          // nand type
    u32 base_sector;                        // base sector of dump
    u32 sector_count;                       // sector count in SDIO sectors
} __attribute__((packed))stdio_nand_desc_t;

typedef struct _sdio_nand_signature_sector_t
{
    u64 signature;              // HAXXDUMP
    stdio_nand_desc_t nand_descriptions[3];
} __attribute__((packed)) sdio_nand_signature_sector_t;


typedef void (*read_write_callback_t)(int, int);

int getPhysicalDeviceHandle(u32 device);

int slcRead1_original(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter);
int sdcardRead_original(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter);

#endif // DEVICES_H_
