#include <string.h>
#include <malloc.h>
#include "common/common.h"
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "net.h"

static volatile int iLock = 0;

#define CHECK_ERROR(cond) if (cond) { goto error; }

void cafiine_connect(int *psock,u32 server_ip) {
    struct sockaddr_in addr;
    int sock, ret;

    socket_lib_init();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(sock == -1);

    addr.sin_family = AF_INET;
    addr.sin_port = 7332;
    addr.sin_addr.s_addr = server_ip;

    ret = connect(sock, (void *)&addr, sizeof(addr));
    CHECK_ERROR(ret < 0);
    ret = cafiine_handshake(sock);
    CHECK_ERROR(ret < 0);
    CHECK_ERROR(ret == BYTE_NORMAL);

    *psock = sock;
    return;
error:
    if (sock != -1)
        socketclose(sock);
    *psock = -1;
    return;
}

void cafiine_disconnect(int sock) {
    CHECK_ERROR(sock == -1);
    socketclose(sock);
error:
    return;
}

int cafiine_handshake(int sock) {
    int ret;

    unsigned char buffer[16];

    u64 title_id = OSGetTitleID();
    memcpy(buffer, &title_id, 16);

    ret = sendwait(sock, buffer, sizeof(buffer));
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);
    return ret;
error:
    return ret;
}

int getMode(int sock,int * result)
{
    while (iLock)
        usleep(5000);
    iLock = 1;
	int ret = 0;
    CHECK_ERROR(sock == -1);

    // create and send buffer with : [cmd id][fd][size][buffer data ...]
    {
       ret = sendbyte(sock, BYTE_G_MODE);

        // wait reply
        ret = recvbyte(sock);
        CHECK_ERROR(ret < 0);
		if(ret == BYTE_MODE_D) *result = BYTE_MODE_D;
		if(ret == BYTE_MODE_I) *result = BYTE_MODE_I;
		ret = 1;
    }
error:
    iLock = 0;
    return ret;
}

int cafiine_fsetpos(int sock, int *result, int fd, int set) {
    while (iLock)
        usleep(5000);
    iLock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    char buffer[1 + 8];
    buffer[0] = BYTE_SETPOS;
    *(int *)(buffer + 1) = fd;
    *(int *)(buffer + 5) = set;
    ret = sendwait(sock, buffer, 1 + 8);
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);
    CHECK_ERROR(ret == BYTE_NORMAL);
    ret = recvwait(sock, result, 4);
    CHECK_ERROR(ret < 0);

    iLock = 0;
    return 0;
error:
    iLock = 0;
    return -1;
}

int cafiine_send_handle(int sock, const char *path, int handle)
{
    while (iLock)
        usleep(5000);
    iLock = 1;

    CHECK_ERROR(sock == -1);

    // create and send buffer with : [cmd id][handle][path length][path data ...]
    {
        int ret;
        int len_path = 0;
        while (path[len_path++]);
        char buffer[1 + 4 + 4 + len_path];

        buffer[0] = BYTE_HANDLE;
        *(int *)(buffer + 1) = handle;
        *(int *)(buffer + 5) = len_path;
        for (ret = 0; ret < len_path; ret++)
            buffer[9 + ret] = path[ret];

        // send buffer, wait for reply
        ret = sendwait(sock, buffer, 1 + 4 + 4 + len_path);
        CHECK_ERROR(ret < 0);

		 // wait reply
        ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret == BYTE_SPECIAL);
		if(ret == BYTE_REQUEST){
			ret = 1;
		}else{
			ret = 2;
		}
        // wait reply
        int special_ret = recvbyte(sock);
		CHECK_ERROR(special_ret < 0);
        CHECK_ERROR(special_ret != BYTE_SPECIAL);
		iLock = 0;
		return ret;
    }

error:
   iLock = 0;
    return -1;
}

int cafiine_fopen(int sock, int *result, const char *path, const char *mode, int *handle) {
    while (iLock)
        usleep(5000);
    iLock = 1;

	int final_result = -1;
    CHECK_ERROR(sock == -1);

    int ret;
    int len_path = 0;
    while (path[len_path++]);
    int len_mode = 0;
    while (mode[len_mode++]);

    //
    {
        char buffer[1 + 8 + len_path + len_mode];
        buffer[0] = BYTE_OPEN;
        *(int *)(buffer + 1) = len_path;
        *(int *)(buffer + 5) = len_mode;
        for (ret = 0; ret < len_path; ret++)
            buffer[9 + ret] = path[ret];
        for (ret = 0; ret < len_mode; ret++)
            buffer[9 + len_path + ret] = mode[ret];

        ret = sendwait(sock, buffer, 1 + 8 + len_path + len_mode);
		CHECK_ERROR(ret < 0);

		 ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret == BYTE_NORMAL);

		ret = recvwait(sock, result, 4);
		CHECK_ERROR(ret < 0);
		ret = recvwait(sock, handle, 4);
		CHECK_ERROR(ret < 0);
    }
	final_result = 0;


error:
    iLock = 0;
    return final_result;
}

void cafiine_send_file(int sock, char *file, int size, int fd) {
    while (iLock)
        usleep(5000);
    iLock = 1;

    CHECK_ERROR(sock == -1);

    int ret;

    // create and send buffer with : [cmd id][fd][size][buffer data ...]
    {
        char buffer[1 + 4 + 4 + size];

        buffer[0] = BYTE_DUMP;
        *(int *)(buffer + 1) = fd;
        *(int *)(buffer + 5) = size;
        for (ret = 0; ret < size; ret++)
            buffer[9 + ret] = file[ret];

        // send buffer, wait for reply
        ret = sendwait(sock, buffer, 1 + 4 + 4 + size);
		CHECK_ERROR(ret < 0);
        // wait reply
        ret = recvbyte(sock);
        CHECK_ERROR(ret != BYTE_SPECIAL);
    }

error:
    iLock = 0;
    return;
}

int cafiine_fread(int sock, int *result, void *ptr, int size, int fd) {
    while (iLock)
        usleep(5000);
    iLock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    char buffer[1 + 8];
    buffer[0] = BYTE_READ;
    *(int *)(buffer + 1) = size;
    *(int *)(buffer + 5) = fd;
    ret = sendwait(sock, buffer, 1 + 8);
	CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret == BYTE_NORMAL);
    int sz;
    ret = recvwait(sock, &sz, 4);
	CHECK_ERROR(ret < 0);
    ret = recvwaitlen(sock, ptr, sz);
	*result = sz - ret;
    ret = sendbyte(sock, BYTE_OK);
    CHECK_ERROR(ret < 0);

    iLock = 0;
    return 0;
error:
    iLock = 0;
    return -1;
}

int cafiine_fclose(int sock, int *result, int fd,int dumpclose) {
    while (iLock)
        usleep(5000);
    iLock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    char buffer[1 + 4];
    buffer[0] = BYTE_CLOSE;
	if(dumpclose)buffer[0] = BYTE_CLOSE_DUMP;
    *(int *)(buffer + 1) = fd;
    ret = sendwait(sock, buffer, 1 + 4);
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret == BYTE_NORMAL);
    ret = recvwait(sock, result, 4);
    CHECK_ERROR(ret < 0);

    iLock = 0;
    return 0;
error:
    iLock = 0;
    return -1;
}

int saviine_start_injection(int sock, long persistentID,int * mask){
    while (iLock)
        usleep(5000);
    iLock = 1;

	int result = 0;
    CHECK_ERROR(sock == -1);
    int ret;
	{
        char buffer[1+4];

        buffer[0] = BYTE_INJECTSTART;
		*(long *)(buffer + 1) = persistentID;
		ret = sendwait(sock, buffer, 1 + 4);
		CHECK_ERROR(ret < 0);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
        CHECK_ERROR(ret != BYTE_SPECIAL);

		ret = recvwait(sock, mask, 4);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR((*mask & MASK_NORMAL) != MASK_NORMAL);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret != BYTE_SPECIAL);
		result = 1;
	}
error:
	iLock = 0;
    return result;
}

int saviine_end_injection(int sock){
    while (iLock)
        usleep(5000);
    iLock = 1;

	int result = 0;
    CHECK_ERROR(sock == -1);
    int ret;
	{
        ret = sendbyte(sock, BYTE_INJECTEND);
		CHECK_ERROR(ret < 0);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret != BYTE_OK);
		result = 1;
	}
error:
	iLock = 0;
    return result;
}

int saviine_start_dump(int sock, long persistentID,int * mask){
    while (iLock)
        usleep(5000);
    iLock = 1;

	int result = 0;
    CHECK_ERROR(sock == -1);
    int ret;
	{
        char buffer[1+4];

        buffer[0] = BYTE_DUMPSTART;
		*(long *)(buffer + 1) = persistentID;
		ret = sendwait(sock, buffer, 1 + 4);
		CHECK_ERROR(ret < 0);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
        CHECK_ERROR(ret != BYTE_SPECIAL);

		ret = recvwait(sock, mask, 4);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR((*mask & MASK_NORMAL) != MASK_NORMAL);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret != BYTE_SPECIAL);
		result = 1;
	}
error:
	iLock = 0;
    return result;
}

int saviine_end_dump(int sock){
    while (iLock)
        usleep(5000);
    iLock = 1;

	int result = 0;
    CHECK_ERROR(sock == -1);
    int ret;
	{
        ret = sendbyte(sock, BYTE_DUMPEND);
		CHECK_ERROR(ret < 0);

		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret != BYTE_OK);
		result = 1;
	}
error:
    iLock = 0;
    return result;
}

int saviine_readdir(int sock, char * path,char * resultname, int * resulttype, int * filesize){
    while (iLock)
        usleep(5000);
    iLock = 1;

	int result = 0;
    CHECK_ERROR(sock == -1);
    int ret;
    // create and send buffer with : [cmd id][len_path][path][filesize]
    {
		int size = 0;
        while (path[size++]);
        char buffer[1+4+size];

        buffer[0] = BYTE_READ_DIR;
		*(int *)(buffer + 1) = size;
		for (ret = 0; ret < size; ret++)
            buffer[5 + ret] = path[ret];

        // send buffer, wait for reply
		ret = sendwait(sock, buffer, 1+4+size);
		CHECK_ERROR(ret < 0);

        // wait reply
        ret = recvbyte(sock);
        CHECK_ERROR(ret != BYTE_OK);

		ret = recvbyte(sock);
		CHECK_ERROR(ret != BYTE_FILE && ret != BYTE_FOLDER);
		*resulttype = ret;
		size = 0;
		ret = recvwait(sock, &size, 4);
		CHECK_ERROR(ret < 0);

		ret = recvwait(sock, resultname, size+1);
		CHECK_ERROR(ret < 0);

		size = 0;
		ret = recvwait(sock, &size, 4);
		CHECK_ERROR(ret < 0);
		*filesize = size;
		ret = recvbyte(sock);
		CHECK_ERROR(ret < 0);
		CHECK_ERROR(ret != BYTE_SPECIAL);
		result = 1;

	}
error:
	iLock = 0;
    return result;
}

void cafiine_send_ping(int sock, int val1, int val2) {
    while (iLock)
        usleep(5000);
    iLock = 1;

    int ret;
    char buffer[1 + 4 + 4];
    buffer[0] = BYTE_PING;
    *(int *)(buffer + 1) = val1;
    *(int *)(buffer + 5) = val2;

    ret = sendwait(sock, buffer, 1 + 4 + 4);
    CHECK_ERROR(ret < 0);

    error:
    iLock = 0;
    return;
}

 int recvwait(int sock, void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = recv(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}

 int recvwaitlen(int sock, void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = recv(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return len;
}


 int recvbyte(int sock) {
    unsigned char buffer[1];
    int ret;

    ret = recvwait(sock, buffer, 1);
    if (ret < 0) return ret;
    return buffer[0];
}

 int sendwait(int sock, const void *buffer, int len) {
    int ret;
    while (len > 0) {
        ret = send(sock, buffer, len, 0);
        CHECK_ERROR(ret < 0);
        len -= ret;
        buffer += ret;
    }
    return 0;
error:
    return ret;
}

void log_string(int sock, const char* str, char flag_byte) {
    if(sock == -1) {
		return;
	}
     while (iLock)
        usleep(5000);
    iLock = 1;

    int i;
    int len_str = 0;
    while (str[len_str++]);

    //
    {
        char buffer[1 + 4 + len_str];
        buffer[0] = flag_byte;
        *(int *)(buffer + 1) = len_str;
        for (i = 0; i < len_str; i++)
            buffer[5 + i] = str[i];

        buffer[5 + i] = 0;

        sendwait(sock, buffer, 1 + 4 + len_str);
    }

    iLock = 0;
}

int sendbyte(int sock, unsigned char byte) {
    unsigned char buffer[1];

    buffer[0] = byte;
    return sendwait(sock, buffer, 1);
}
