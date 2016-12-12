#include <stdlib.h>
#include <stdio.h>
#include "svc.h"
#include "fsa.h"

#define BSP_memcpy      ((void *(*)(void*, void*, unsigned int))0xE600EA18)
#define BSP_memset      ((void *(*)(void*, int, unsigned int))0xE600EAB4)
#define BSP_strncpy     ((char *(*)(char*, const char*, unsigned int))0xE600F4AC)

static void* allocIobuf()
{
	void* ptr = svcAlloc(0xCAFF, 0x828);
	BSP_memset(ptr, 0x00, 0x828);

	return ptr;
}

static void freeIobuf(void* ptr)
{
	svcFree(0xCAFF, ptr);
}

int FSA_RawOpen(int fd, const char* device_path, int* outHandle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	BSP_strncpy((char*)&inbuf[0x01], device_path, 0x27F);

	int ret = svcIoctl(fd, 0x6A, inbuf, 0x520, outbuf, 0x293);

	if(outHandle) *outHandle = outbuf[1];

	freeIobuf(iobuf);
	return ret;
}

int FSA_RawClose(int fd, int device_handle)
{
	u8* iobuf = allocIobuf();
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)&iobuf[0x520];

	inbuf[1] = device_handle;

	int ret = svcIoctl(fd, 0x6D, inbuf, 0x520, outbuf, 0x293);

	freeIobuf(iobuf);
	return ret;
}

int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 blocks_offset, int device_handle)
{
	u8* iobuf = allocIobuf();
	u8* inbuf8 = iobuf;
	u8* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	u32* inbuf = (u32*)inbuf8;
	u32* outbuf = (u32*)outbuf8;

	inbuf[0x08 / 4] = (blocks_offset >> 32);
	inbuf[0x0C / 4] = (blocks_offset & 0xFFFFFFFF);
	inbuf[0x10 / 4] = cnt;
	inbuf[0x14 / 4] = size_bytes;
	inbuf[0x18 / 4] = device_handle;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x520;

	iovec[1].ptr = data;
	iovec[1].len = size_bytes * cnt;

	iovec[2].ptr = outbuf;
	iovec[2].len = 0x293;

	int ret = svcIoctlv(fd, 0x6C, 2, 1, iovec);

	freeIobuf(iobuf);
	return ret;
}
