#ifndef _FAT32_FORMAT_H_
#define _FAT32_FORMAT_H_

int CheckFAT32PartitionOffset(u8 * mbr, u32 partition_offset);
int FormatSDCard(u32 partition_offset, u32 total_sectors);

#endif // _FAT32_FORMAT_H_
