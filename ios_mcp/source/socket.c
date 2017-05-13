#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"
#include "svc.h"
#include "text.h"
#include "imports.h"

static int socket_handle = 0;

int socketInit()
{
	if(socket_handle) return socket_handle;

	int ret = svcOpen("/dev/socket", 0);

	if(ret > 0)
	{
		socket_handle = ret;
		return socket_handle;
	}

	return ret;
}

int socketExit()
{
	int ret = svcClose(socket_handle);

	socket_handle = 0;

	return ret;
}

static void* allocIobuf(u32 size)
{
	void* ptr = svcAlloc(0xCAFF, size);

	if(ptr) memset(ptr, 0x00, size);

	return ptr;
}

static void freeIobuf(void* ptr)
{
	svcFree(0xCAFF, ptr);
}

int	socket(int domain, int type, int protocol)
{
	u8* iobuf = allocIobuf(0xC);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = domain;
	inbuf[1] = type;
	inbuf[2] = protocol;

	int ret = svcIoctl(socket_handle, 0x11, inbuf, 0xC, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int	closesocket(int sockfd)
{
	u8* iobuf = allocIobuf(0x4);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = sockfd;

	int ret = svcIoctl(socket_handle, 0x3, inbuf, 0x4, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	u8* iobuf = allocIobuf(0x18);
	u32* inbuf = (u32*)iobuf;
	u32* outbuf = (u32*)inbuf;

	inbuf[0] = sockfd;

	int ret = -1;

	if(addr && addrlen && *addrlen == 0x10)
	{
		inbuf[5] = *addrlen;

		ret = svcIoctl(socket_handle, 0x1, inbuf, 0x18, outbuf, 0x18);

		if(ret >= 0)
		{
			memcpy(addr, &outbuf[1], outbuf[5]);
			*addrlen = outbuf[5];
		}
	}else{
		inbuf[5] = 0x10;

		ret = svcIoctl(socket_handle, 0x1, inbuf, 0x18, outbuf, 0x18);
	}

	freeIobuf(iobuf);
	return ret;
}

int	bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if(addrlen != 0x10) return -1;

	u8* iobuf = allocIobuf(0x18);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = sockfd;
	memcpy(&inbuf[1], addr, addrlen);
	inbuf[5] = addrlen;

	int ret = svcIoctl(socket_handle, 0x2, inbuf, 0x18, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int	connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if(addrlen != 0x10) return -1;

	u8* iobuf = allocIobuf(0x18);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = sockfd;
	memcpy(&inbuf[1], addr, addrlen);
	inbuf[5] = addrlen;

	int ret = svcIoctl(socket_handle, 0x4, inbuf, 0x18, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int listen(int sockfd, int backlog)
{
	u8* iobuf = allocIobuf(0x8);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = sockfd;
	inbuf[1] = backlog;

	int ret = svcIoctl(socket_handle, 0xA, inbuf, 0x8, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int shutdown(int sockfd, int how)
{
	u8* iobuf = allocIobuf(0x8);
	u32* inbuf = (u32*)iobuf;

	inbuf[0] = sockfd;
	inbuf[1] = how;

	int ret = svcIoctl(socket_handle, 0x10, inbuf, 0x8, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int recv(int sockfd, void *buf, size_t len, int flags)
{
	if(!len) return -101;

	// TODO : size checks, split up data into multiple vectors if necessary
	void* data_buf = svcAllocAlign(0xCAFF, len, 0x40);
	if(!data_buf) return -100;

	u8* iobuf = allocIobuf(0x38);
	iovec_s* iovec = (iovec_s*)iobuf;
	u32* inbuf = (u32*)&iobuf[0x30];

	inbuf[0] = sockfd;
	inbuf[1] = flags;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x8;
	iovec[1].ptr = (void*)data_buf;
	iovec[1].len = len;

	int ret = svcIoctlv(socket_handle, 0xC, 1, 3, iovec);

	if(ret > 0 && buf)
	{
		memcpy(buf, data_buf, ret);
	}

	freeIobuf(data_buf);
	freeIobuf(iobuf);
	return ret;
}

int send(int sockfd, const void *buf, size_t len, int flags)
{
	if(!buf || !len) return -101;

	// TODO : size checks, split up data into multiple vectors if necessary
	void* data_buf = svcAllocAlign(0xCAFF, len, 0x40);
	if(!data_buf) return -100;

	u8* iobuf = allocIobuf(0x38);
	iovec_s* iovec = (iovec_s*)iobuf;
	u32* inbuf = (u32*)&iobuf[0x30];

	memcpy(data_buf, buf, len);

	inbuf[0] = sockfd;
	inbuf[1] = flags;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x8;
	iovec[1].ptr = (void*)data_buf;
	iovec[1].len = len;

	int ret = svcIoctlv(socket_handle, 0xE, 4, 0, iovec);

	freeIobuf(data_buf);
	freeIobuf(iobuf);
	return ret;
}
