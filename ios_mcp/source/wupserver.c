#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "imports.h"
#include "net_ifmgr_ncl.h"
#include "socket.h"
#include "fsa.h"
#include "svc.h"
#include "text.h"
#include "logger.h"
#include "ipc.h"

static int serverKilled;
static int serverSocket;
static u8 threadStack[0x1000] __attribute__((aligned(0x20)));

// overwrites command_buffer with response
// returns length of response (or 0 for no response, negative for error)
static int serverCommandHandler(u32* command_buffer, u32 length)
{
	if(!command_buffer || !length) return -1;

	int out_length = 4;

	switch(command_buffer[0])
	{
		case 0:
			// write
			// [cmd_id][addr]
			{
				void* dst = (void*)command_buffer[1];

				memcpy(dst, &command_buffer[2], length - 8);
			}
			break;
		case 1:
			// read
			// [cmd_id][addr][length]
			{
				void* src = (void*)command_buffer[1];
				length = command_buffer[2];

				memcpy(&command_buffer[1], src, length);
				out_length = length + 4;
			}
			break;
		case 2:
			// svc
			// [cmd_id][svc_id]
			{
				int svc_id = command_buffer[1];
				int size_arguments = length - 8;

				u32 arguments[8];
				memset(arguments, 0x00, sizeof(arguments));
				memcpy(arguments, &command_buffer[2], (size_arguments < 8 * 4) ? size_arguments : (8 * 4));

				// return error code as data
				out_length = 8;
				command_buffer[1] = ((int (*const)(u32, u32, u32, u32, u32, u32, u32, u32))(MCP_SVC_BASE + svc_id * 8))(arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7]);
			}
			break;
		case 3:
			// kill
			// [cmd_id]
			{
				serverKilled = 1;
				ipc_deinit();
			}
			break;
		case 4:
			// memcpy
			// [dst][src][size]
			{
				void* dst = (void*)command_buffer[1];
				void* src = (void*)command_buffer[2];
				int size = command_buffer[3];

				memcpy(dst, src, size);
			}
			break;
		case 5:
			// repeated-write
			// [address][value][n]
			{
				u32* dst = (u32*)command_buffer[1];
				u32* cache_range = (u32*)(command_buffer[1] & ~0xFF);
				u32 value = command_buffer[2];
				u32 n = command_buffer[3];

				u32 old = *dst;
				int i;
				for(i = 0; i < n; i++)
				{
					if(*dst != old)
					{
						if(*dst == 0x0) old = *dst;
						else
						{
							*dst = value;
							svcFlushDCache(cache_range, 0x100);
							break;
						}
					}else
					{
						svcInvalidateDCache(cache_range, 0x100);
						usleep(50);
					}
				}
			}
			break;
		default:
			// unknown command
			return -2;
			break;
	}

	// no error !
	command_buffer[0] = 0x00000000;
	return out_length;
}

static void serverClientHandler(int sock)
{
	u32 command_buffer[0x180];

	while(!serverKilled)
	{
		int ret = recv(sock, command_buffer, sizeof(command_buffer), 0);

		if(ret <= 0) break;

		ret = serverCommandHandler(command_buffer, ret);

		if(ret > 0)
		{
			send(sock, command_buffer, ret, 0);
		}else if(ret < 0)
		{
			send(sock, &ret, sizeof(int), 0);
		}
	}

	closesocket(sock);
}

static void serverListenClients()
{
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in server;

	memset(&server, 0x00, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = 1337;
	server.sin_addr.s_addr = 0;

	if(bind(serverSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        closesocket(serverSocket);
        return;
    }

    if(listen(serverSocket, 1) < 0)
    {
        closesocket(serverSocket);
        return;
    }

	while(!serverKilled)
	{
        int csock = accept(serverSocket, NULL, NULL);
        if(csock < 0)
            break;

        serverClientHandler(csock);
	}

	closesocket(serverSocket);
	serverSocket = -1;
}

static int wupserver_thread(void *arg)
{
	while(ifmgrnclInit() <= 0)
	{
		//print(0, 0, "opening /dev/net/ifmgr/ncl...");
		usleep(1000);
	}

	while(true)
	{
		u16 out0, out1;

		int ret0 = IFMGRNCL_GetInterfaceStatus(0, &out0);
		if(!ret0 && out0 == 1) break;

		int ret1 = IFMGRNCL_GetInterfaceStatus(1, &out1);
		if(!ret1 && out1 == 1) break;

		//print(0, 0, "initializing /dev/net/ifmgr/ncl... %08X %08X %08X %08X ", ret0, ret1, out0, out1);

		usleep(1000);
	}

	while(socketInit() <= 0)
	{
		//print(0, 0, "opening /dev/socket...");
		usleep(1000);
	}

    log_init(0xC0A8B203);

	//print(0, 0, "opened /dev/socket !");
	usleep(5*1000*1000);
	//print(0, 10, "attempting sockets !");

	while(1)
	{
	    if(!serverKilled)
        {
	         serverListenClients();
        }
        else
        {
            break;
        }
		usleep(1000*1000);
	}

	log_deinit();
	return 0;
}

void wupserver_init(void)
{
    serverSocket = -1;
    serverKilled = 0;

    int threadId = svcCreateThread(wupserver_thread, 0, (u32*)(threadStack + sizeof(threadStack)), sizeof(threadStack), 0x78, 1);
    if(threadId >= 0)
        svcStartThread(threadId);
}

void wupserver_deinit(void)
{
    serverKilled = 1;
    shutdown(serverSocket, SHUT_RDWR);
}
