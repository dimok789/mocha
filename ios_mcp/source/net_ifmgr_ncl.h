#ifndef NET_IFMGR_NCL
#define NET_IFMGR_NCL

#include "types.h"

int ifmgrnclInit();
int ifmgrnclExit();

int	IFMGRNCL_GetInterfaceStatus(u16 interface_id, u16* out_status);

#endif
