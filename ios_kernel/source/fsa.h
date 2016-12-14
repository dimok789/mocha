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
#ifndef FSA_H
#define FSA_H

#include "types.h"

#define NAND_DUMP_SIGNATURE_SECTOR          0x01
#define NAND_MAX_DESC_TYPES                 5

#define NAND_DUMP_SIGNATURE                 0x4841585844554d50ULL // HAXXDUMP

#define NAND_DESC_TYPE_SLC                  0x534c4320 // 'SLC '
#define NAND_DESC_TYPE_SLCCMPT              0x534c4332 // 'SLC2'
#define NAND_DESC_TYPE_MLC                  0x4d4c4320 // 'MLC '
#define NAND_DESC_TYPE_SEEPROM              0x45455052 // 'EEPR'
#define NAND_DESC_TYPE_OTP                  0x4f545020 // 'OTP '

typedef struct _stdio_nand_desc_t
{
    u32 nand_type;                          // nand type
    u32 base_sector;                        // base sector of dump
    u32 sector_count;                       // sector count in SDIO sectors
} __attribute__((packed))stdio_nand_desc_t;

typedef struct _sdio_nand_signature_sector_t
{
    u64 signature;              // HAXXDUMP
    stdio_nand_desc_t nand_descriptions[NAND_MAX_DESC_TYPES];
} __attribute__((packed)) sdio_nand_signature_sector_t;

int FSA_SDReadRawSectors(void *buffer, u32 sector, u32 num_sectors);
int FSA_SDWriteRawSectors(const void *buffer, u32 sector, u32 num_sectors);

#endif
