#include <string.h>
#include <stdio.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/nn_nim_functions.h"
#include "nnu.h"

void nnupatcher(void) {
	InitNimFunctionPointers();
	
	u32 *targetfunction, *patchoffset;

	// locate function
	OSDynLoad_FindExport(nn_nim_handle, 0, "NeedsNetworkUpdate__Q2_2nn3nimFPb", &targetfunction);
	
	patchoffset = OSEffectiveToPhysical(targetfunction);
	patchoffset = (u32*)((u32)patchoffset - 0x31000000 + 0xA0000000);
	
	// now patch
	patchoffset[0] = 0x38600000;
	patchoffset[1] = 0x38800000;
	patchoffset[2] = 0x4E800020;
}