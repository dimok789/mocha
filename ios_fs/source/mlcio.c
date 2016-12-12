#include "types.h"
#include "imports.h"

void mlc_init(void)
{
    FS_MMC_MLC_STRUCT[0x24/4] = FS_MMC_MLC_STRUCT[0x24/4] | 0x20;
    FS_MMC_MLC_STRUCT[0x28/4] = FS_MMC_MLC_STRUCT[0x28/4] & (~0x04);
}



