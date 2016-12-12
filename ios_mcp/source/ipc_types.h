#ifndef _IPC_TYPES_H_
#define _IPC_TYPES_H_

#include "types.h"

#define IOS_COMMAND_INVALID     0x00
#define IOS_OPEN                0x01
#define IOS_CLOSE               0x02
#define IOS_READ                0x03
#define IOS_WRITE               0x04
#define IOS_SEEK                0x05
#define IOS_IOCTL               0x06
#define IOS_IOCTLV              0x07
#define IOS_REPLY               0x08
#define IOS_IPC_MSG0            0x09
#define IOS_IPC_MSG1            0x0A
#define IOS_IPC_MSG2            0x0B
#define IOS_SUSPEND             0x0C
#define IOS_RESUME              0x0D
#define IOS_SVCMSG              0x0E


/* IPC message */
typedef struct ipcmessage
{
	u32 command;
	u32 result;
	u32 fd;
	u32 flags;
	u32 client_cpu;
	u32 client_pid;
	u64 client_gid;
	u32 server_handle;

	union
	{
	    u32 args[5];

		struct
		{
			char *device;
			u32   mode;
			u32   resultfd;
		} open;

		struct
		{
			void *data;
			u32   length;
		} read, write;

		struct
		{
			s32 offset;
			s32 origin;
		} seek;

		struct
		{
			u32 command;

			u32 *buffer_in;
			u32  length_in;
			u32 *buffer_io;
			u32  length_io;
		} ioctl;
		struct _ioctlv
		{
			u32 command;

			u32 num_in;
			u32 num_io;
			struct _ioctlv *vector;
		} ioctlv;
	};

	u32 prev_command;
	u32 prev_fd;
	u32 virt0;
	u32 virt1;
} __attribute__((packed)) ipcmessage;

#endif
