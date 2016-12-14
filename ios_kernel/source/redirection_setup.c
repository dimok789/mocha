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
#include "config.h"
#include "utils.h"
#include "fsa.h"
#include "kernel_patches.h"
#include "ios_bsp_patches.h"

void redirection_setup(void)
{
    int seepromDumpFound = 0;
    u32 seepromDumpBaseSector = 0x4FF;
    int otpDumpFound = 0;
    u32 otpDumpBaseSector = 0x4FD;
    int writeInfoSector = 0;
    sdio_nand_signature_sector_t *infoSector = (sdio_nand_signature_sector_t*)0x00141000;
    kernel_memset(infoSector, 0x00, 0x200);

    int result = FSA_SDReadRawSectors(infoSector, NAND_DUMP_SIGNATURE_SECTOR, 1);
    if(result < 0)
        return;

    if(infoSector->signature == NAND_DUMP_SIGNATURE)
    {
        int i;
        for(i = 0; i < NAND_MAX_DESC_TYPES; i++)
        {
            if(infoSector->nand_descriptions[i].nand_type == NAND_DESC_TYPE_SEEPROM)
            {
                seepromDumpFound = 1;
                seepromDumpBaseSector = infoSector->nand_descriptions[i].base_sector;
            }
            if(infoSector->nand_descriptions[i].nand_type == NAND_DESC_TYPE_OTP)
            {
                otpDumpFound = 1;
                otpDumpBaseSector = infoSector->nand_descriptions[i].base_sector;
            }
        }
    }

    if(cfw_config.seeprom_red)
    {
        bsp_init_seeprom_buffer(seepromDumpBaseSector, seepromDumpFound);

        if(seepromDumpFound == 0)
        {
            infoSector->nand_descriptions[3].nand_type = NAND_DESC_TYPE_SEEPROM;
            infoSector->nand_descriptions[3].base_sector = seepromDumpBaseSector;
            infoSector->nand_descriptions[3].sector_count = 1;
            writeInfoSector++;
        }
    }

    if(cfw_config.otp_red)
    {
        kernel_init_otp_buffer(otpDumpBaseSector, otpDumpFound);

        if(otpDumpFound == 0)
        {
            infoSector->nand_descriptions[4].nand_type = NAND_DESC_TYPE_OTP;
            infoSector->nand_descriptions[4].base_sector = otpDumpBaseSector;
            infoSector->nand_descriptions[4].sector_count = 2;
            writeInfoSector++;
        }
    }

    if(writeInfoSector > 0)
    {
        FSA_SDWriteRawSectors(infoSector, NAND_DUMP_SIGNATURE_SECTOR, 1);
    }
}
