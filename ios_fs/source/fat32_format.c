#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "text.h"

extern unsigned char io_buffer[0x40000];

#define PARTITION_TYPE_FAT32        0x0c
#define MAX_PARTITIONS				32 /* Maximum number of partitions that can be found */
#define MAX_MOUNTS					10 /* Maximum number of mounts available at one time */
#define MAX_SYMLINK_DEPTH			10 /* Maximum search depth when resolving symbolic links */

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

typedef struct tagFAT_BOOTSECTOR32
{
	// Common fields.
	u8 sJmpBoot[3];
	u8 sOEMName[8];
	u16 wBytsPerSec;
	u8 bSecPerClus;
	u16 wRsvdSecCnt;
	u8 bNumFATs;
	u16 wRootEntCnt;
	u16 wTotSec16;
	u8 bMedia;
	u16 wFATSz16;
	u16 wSecPerTrk;
	u16 wNumHeads;
	u32 dHiddSec;
	u32 dTotSec32;
	// Fat 32/16 only
	u32 dFATSz32;
	u16 wExtFlags;
	u16 wFSVer;
	u32 dRootClus;
	u16 wFSInfo;
	u16 wBkBootSec;
	u8 Reserved[12];
	u8 bDrvNum;
	u8 Reserved1;
	u8 bBootSig; // == 0x29 if next three fields are ok
	u32 dBS_VolID;
	u8 sVolLab[11];
	u8 sBS_FilSysType[8];

} __attribute__((__packed__)) FAT_BOOTSECTOR32;

typedef struct {
	u32 dLeadSig;
	u8 sReserved1[480];
	u32 dStrucSig;
	u32 dFree_Count;
	u32 dNxt_Free;
	u8 sReserved2[12];
	u32 dTrailSig;
} __attribute__((__packed__)) FAT_FSINFO;

static inline u8 get_sectors_per_cluster (u64 DiskSizeBytes)
{
	u8 ret = 0x01; // 1 sector per cluster
	u32 DiskSizeMB = DiskSizeBytes/(1024*1024);

	// 512 MB to 8,191 MB 4 KB
	if (DiskSizeMB > 512)
		ret = 0x8;

	// 8,192 MB to 16,383 MB 8 KB
	if (DiskSizeMB > 8192)
		ret = 0x10;

	// 16,384 MB to 32,767 MB 16 KB
	if (DiskSizeMB > 16384)
		ret = 0x20; // ret = 0x20;

	// Larger than 32,768 MB 32 KB
	if (DiskSizeMB > 32768)
		ret = 0x40;  // ret = 0x40;

	return ret;
}

static inline u32 MakeVolumeID()
{
    // we dont have time yet so for now its fixed
	//time_t rawtime = time(0);
	//struct tm * timeinfo = localtime(&rawtime);

	//u16 hi = le16(timeinfo->tm_mday + (timeinfo->tm_mon << 8) + (timeinfo->tm_sec << 8));
	//u16 lo = le16((timeinfo->tm_hour << 8) + timeinfo->tm_min + timeinfo->tm_year + 1900);
    u16 hi = 0x0BAD;
    u16 lo = 0xBABE;

	return (lo + (hi << 16));
}


int FormatToFAT32(u32 lba, u32 sec_count)
{
	if(sec_count < 0xFFFF)
	{
	    _printf(20, 40, "Not enough sectors for FAT32");
		return -1;
	}

	int BytesPerSect = SDIO_BYTES_PER_SECTOR;
	u16 ReservedSectCount = 32;
	u8 NumFATs = 2;

	memset(io_buffer, 0, BytesPerSect*18);

	FAT_BOOTSECTOR32 * FAT32BootSect = (FAT_BOOTSECTOR32 *) (io_buffer+16*BytesPerSect);
	FAT_FSINFO * FAT32FsInfo = (FAT_FSINFO*) (io_buffer+17*BytesPerSect);

	// fill out the boot sector and fs info
	FAT32BootSect->sJmpBoot[0] = 0xEB;
	FAT32BootSect->sJmpBoot[1] = 0x5A;
	FAT32BootSect->sJmpBoot[2] = 0x90;
	memcpy(FAT32BootSect->sOEMName, "MSWIN4.1", 8);

	FAT32BootSect->wBytsPerSec = le16(BytesPerSect);

	u8 SectorsPerCluster = get_sectors_per_cluster((u64) sec_count * (u64) BytesPerSect);

	FAT32BootSect->bSecPerClus = SectorsPerCluster;
	FAT32BootSect->wRsvdSecCnt = le16(ReservedSectCount);
	FAT32BootSect->bNumFATs = NumFATs;
	FAT32BootSect->wRootEntCnt = 0;
	FAT32BootSect->wTotSec16 = 0;
	FAT32BootSect->bMedia = 0xF8;
	FAT32BootSect->wFATSz16 = 0;
	FAT32BootSect->wSecPerTrk = le16(63); //SectorsPerTrack;
	FAT32BootSect->wNumHeads = le16(255); //TracksPerCylinder;
	FAT32BootSect->dHiddSec = le32(lba); //HiddenSectors;
	FAT32BootSect->dTotSec32 = le32(sec_count);

	// This is based on
	// http://hjem.get2net.dk/rune_moeller_barnkob/filesystems/fat.html
	u32 FatSize = (4*(sec_count-ReservedSectCount)/((SectorsPerCluster*BytesPerSect)+(4*NumFATs)))+1;

	FAT32BootSect->dFATSz32 = le32(FatSize);
	FAT32BootSect->wExtFlags = 0;
	FAT32BootSect->wFSVer = 0;
	FAT32BootSect->dRootClus = le32(2);
	FAT32BootSect->wFSInfo = le16(1);
	FAT32BootSect->wBkBootSec = le16(6); //BackupBootSect
	FAT32BootSect->bDrvNum = 0x80;
	FAT32BootSect->Reserved1 = 0;
	FAT32BootSect->bBootSig = 0x29;

	FAT32BootSect->dBS_VolID = MakeVolumeID();
	memcpy(FAT32BootSect->sVolLab, "NO NAME	", 11);
	memcpy(FAT32BootSect->sBS_FilSysType, "FAT32   ", 8);
	((u8 *)FAT32BootSect)[510] = 0x55; //Boot Record Signature
	((u8 *)FAT32BootSect)[511] = 0xAA; //Boot Record Signature

	// FSInfo sect signatures
	FAT32FsInfo->dLeadSig = le32(0x41615252);
	FAT32FsInfo->dStrucSig = le32(0x61417272);
	FAT32FsInfo->dTrailSig = le32(0xaa550000);
	((u8 *)FAT32FsInfo)[510] = 0x55; //Boot Record Signature
	((u8 *)FAT32FsInfo)[511] = 0xAA; //Boot Record Signature

	// First FAT Sector
	u32 FirstSectOfFat[3];
	FirstSectOfFat[0] = le32(0x0ffffff8);  // Reserved cluster 1 media id in low byte
	FirstSectOfFat[1] = le32(0x0fffffff);  // Reserved cluster 2 EOC
	FirstSectOfFat[2] = le32(0x0fffffff);  // end of cluster chain for root dir

	u32 UserAreaSize = sec_count - ReservedSectCount - (NumFATs*FatSize);
	u32 ClusterCount = UserAreaSize/SectorsPerCluster;

	if (ClusterCount > 0x0FFFFFFF)
	{
		_printf(20, 40, "This drive has more than 2^28 clusters. Partition might be too small.");
		return -1;
	}

	if (ClusterCount < 65536)
	{
		_printf(20, 40, "FAT32 must have at least 65536 clusters");
		return -1;
	}

	u32 FatNeeded = (ClusterCount * 4 + (BytesPerSect-1))/BytesPerSect;
	if (FatNeeded > FatSize)
	{
		_printf(20, 40, "This drive is too big, %u > %u", FatNeeded, FatSize);
		return -1;
	}

	// fix up the FSInfo sector
	FAT32FsInfo->dFree_Count = le32((UserAreaSize/SectorsPerCluster)-1);
	FAT32FsInfo->dNxt_Free = le32(3); // clusters 0-1 resered, we used cluster 2 for the root dir

	/** Now all is done and we start writting **/

	// First zero out ReservedSect + FatSize * NumFats + SectorsPerCluster
	u32 SystemAreaSize = (ReservedSectCount+(NumFATs*FatSize) + SectorsPerCluster);
	u32 done = 0;
	// Read the first sector on the device
	while(SystemAreaSize > 0)
	{
		int write = SystemAreaSize < 16 ? SystemAreaSize : 16;

        int result = sdcard_readwrite(SDIO_WRITE, io_buffer, write, SDIO_BYTES_PER_SECTOR, lba+done, NULL, DEVICE_ID_SDCARD_PATCHED);
		if(result != 0)
		{
			_printf(20, 40, "Cannot write to the drive.");
			return -1;
		}
		SystemAreaSize -= write;
		done += write;
	}

	for (int i = 0; i < 2; i++)
	{
		u32 SectorStart = (i == 0) ? lba : lba+6; //BackupBootSect

        int result = sdcard_readwrite(SDIO_WRITE, FAT32BootSect, 1, SDIO_BYTES_PER_SECTOR, SectorStart, NULL, DEVICE_ID_SDCARD_PATCHED);
		if(result != 0)
		{
			_printf(20, 40, "Cannot write to the drive.");
			return -1;
		}
        result = sdcard_readwrite(SDIO_WRITE, FAT32FsInfo, 1, SDIO_BYTES_PER_SECTOR, SectorStart+1, NULL, DEVICE_ID_SDCARD_PATCHED);
		if(result != 0)
		{
			_printf(20, 40, "Cannot write to the drive.");
			return -1;
		}
	}

	memcpy(io_buffer, FirstSectOfFat, sizeof(FirstSectOfFat));

	// Write the first fat sector in the right places
	for (int i = 0; i < NumFATs; i++)
	{
		u32 SectorStart = lba + ReservedSectCount + (i * FatSize);

        int result = sdcard_readwrite(SDIO_WRITE, io_buffer, 1, SDIO_BYTES_PER_SECTOR, SectorStart, NULL, DEVICE_ID_SDCARD_PATCHED);
		if(result != 0)
		{
			_printf(20, 40, "Cannot write to the drive.");
			return -1;
		}
	}

	return 0;
}


int CheckFAT32PartitionOffset(u8 * mbr_buf, u32 partition_offset)
{
    MASTER_BOOT_RECORD *mbr = (MASTER_BOOT_RECORD*)mbr_buf;
	return (mbr->signature == MBR_SIGNATURE) && (le32(mbr->partitions[0].lba_start) >= partition_offset);
}

int FormatSDCard(u32 partition_offset, u32 total_sectors)
{
    _printf(20, 40, "Formatting SD card....");

    MASTER_BOOT_RECORD *mbr = (MASTER_BOOT_RECORD*)io_buffer;
    memset(mbr, 0, SDIO_BYTES_PER_SECTOR);

    int result = sdcard_readwrite(SDIO_READ, mbr, 1, SDIO_BYTES_PER_SECTOR, 0, NULL, DEVICE_ID_SDCARD_PATCHED);
    if(result != 0)
    {
        _printf(20, 40, "SD card read failed %i", result);
        return result;
    }

    u32 lba_start = partition_offset;

    result = FormatToFAT32(lba_start, total_sectors - partition_offset);
    if(result != 0)
        return result;

    memset(mbr, 0, sizeof(MASTER_BOOT_RECORD));
    mbr->signature = MBR_SIGNATURE;

    // setup primary FAT32 partition
	mbr->partitions[0].status = PARTITION_BOOTABLE; // set activate
	mbr->partitions[0].chs_start[0] = mbr->partitions[0].chs_end[0] = 0xFE;
	mbr->partitions[0].chs_start[1] = mbr->partitions[0].chs_end[1] = 0xFF;
	mbr->partitions[0].chs_start[2] = mbr->partitions[0].chs_end[2] = 0xFF;
	mbr->partitions[0].type = PARTITION_TYPE_FAT32;
	mbr->partitions[0].lba_start = le32(lba_start);
	mbr->partitions[0].block_count = le32((total_sectors - partition_offset));


    result = sdcard_readwrite(SDIO_WRITE, mbr, 1, SDIO_BYTES_PER_SECTOR, 0, NULL, DEVICE_ID_SDCARD_PATCHED);
    if(result != 0)
    {
        _printf(20, 40, "SD card write failed %i", result);
    }
    else
    {
        _printf(20, 40, "Format of SD card finished successfully", result);
    }

    return result;
}
