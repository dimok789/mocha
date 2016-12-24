#include "main.h"

static int recvwait(int sock, void *buffer, int len);
static int recvbyte(int sock);
static int sendwait(int sock, const void *buffer, int len);

static int fs_connect_handshake(int sock);

#define CHECK_ERROR(cond) if (cond) { goto error; }

#define BYTE_NORMAL             0xff
#define BYTE_SPECIAL            0xfe
#define BYTE_OK                 0xfd
#define BYTE_PING               0xfc
#define BYTE_LOG_STR            0xfb

#define BYTE_OPEN_FILE          0x00
#define BYTE_OPEN_FILE_ASYNC    0x01
#define BYTE_OPEN_DIR           0x02
#define BYTE_OPEN_DIR_ASYNC     0x03
#define BYTE_CHANGE_DIR         0x04
#define BYTE_CHANGE_DIR_ASYNC   0x05
#define BYTE_STAT               0x06
#define BYTE_STAT_ASYNC         0x07

#define BYTE_CLOSE_FILE         0x08
#define BYTE_CLOSE_FILE_ASYNC   0x09
#define BYTE_SETPOS             0x0A
#define BYTE_GETPOS             0x0B
#define BYTE_STATFILE           0x0C
#define BYTE_EOF                0x0D
#define BYTE_READ_FILE          0x0E
#define BYTE_READ_FILE_ASYNC    0x0F
#define BYTE_CLOSE_DIR          0x10
#define BYTE_CLOSE_DIR_ASYNC    0x11
#define BYTE_GET_CWD            0x12
#define BYTE_READ_DIR           0x13
#define BYTE_READ_DIR_ASYNC     0x14

#define BYTE_MOUNT_SD           0x30
#define BYTE_MOUNT_SD_OK        0x31
#define BYTE_MOUNT_SD_BAD       0x32


int fs_connect(int *psock) {
    extern unsigned int server_ip;
    struct sockaddr_in addr;
    int sock, ret;

    // No ip means that we don't have any server running, so no logs
    if (server_ip == 0) {
        *psock = -1;
        return 0;
    }

    socket_lib_init();

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    CHECK_ERROR(sock == -1);

    addr.sin_family = AF_INET;
    addr.sin_port = 7332;
    addr.sin_addr.s_addr = server_ip;

    ret = connect(sock, (void *)&addr, sizeof(addr));
    CHECK_ERROR(ret < 0);
    ret = fs_connect_handshake(sock);
    CHECK_ERROR(ret < 0);
    CHECK_ERROR(ret == BYTE_NORMAL);

    *psock = sock;
    return 0;

error:
    if (sock != -1)
        socketclose(sock);

    *psock = -1;
    return -1;
}

void fs_disconnect(int sock) {
    CHECK_ERROR(sock == -1);
    socketclose(sock);
error:
    return;
}

static int fs_connect_handshake(int sock) {
    int ret;
    unsigned char buffer[16];

    memcpy(buffer, &title_id, 16);

    ret = sendwait(sock, buffer, sizeof(buffer));
    CHECK_ERROR(ret < 0);
    ret = recvbyte(sock);
    CHECK_ERROR(ret < 0);
    return ret;
error:
    return ret;
}

int fs_mount_sd(int sock, void* pClient, void* pCmd) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    int is_mounted = 0;
    char buffer[1];

    if (sock != -1) {
        buffer[0] = BYTE_MOUNT_SD;
        sendwait(sock, buffer, 1);
    }

    // mount sdcard
    FSMountSource mountSrc;
    char mountPath[FS_MAX_MOUNTPATH_SIZE];
    if (FSGetMountSource(pClient, pCmd, FS_SOURCETYPE_EXTERNAL, &mountSrc, 0) == 0) {
        if (FSMount(pClient, pCmd, &mountSrc, mountPath, sizeof(mountPath), -1) == 0) {
            is_mounted = 1;
        }
    }

    if (sock != -1) {
        buffer[0] = is_mounted ? BYTE_MOUNT_SD_OK : BYTE_MOUNT_SD_BAD;
        sendwait(sock, buffer, 1);
    }

    bss.lock = 0;
    return is_mounted;
}

void log_fopen_file(int sock, int is_async, const char *path, const char *mode, int *handle) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    int len_path = 0;
    while (path[len_path++]);
    int len_mode = 0;
    while (mode[len_mode++]);

    //
    {
        char buffer[1 + 8 + len_path + len_mode];
        buffer[0] = is_async ? BYTE_OPEN_FILE_ASYNC : BYTE_OPEN_FILE;
        *(int *)(buffer + 1) = len_path;
        *(int *)(buffer + 5) = len_mode;
        for (ret = 0; ret < len_path; ret++)
            buffer[9 + ret] = path[ret];
        for (ret = 0; ret < len_mode; ret++)
            buffer[9 + len_path + ret] = mode[ret];

        ret = sendwait(sock, buffer, 1 + 8 + len_path + len_mode);
    }

error:
    bss.lock = 0;
}

void log_fopen_dir(int sock, int is_async, const char *path, int *handle) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    int len_path = 0;
    while (path[len_path++]);

    //
    {
        char buffer[1 + 4 + len_path];
        buffer[0] = is_async ? BYTE_OPEN_DIR_ASYNC : BYTE_OPEN_DIR;
        *(int *)(buffer + 1) = len_path;
        for (ret = 0; ret < len_path; ret++)
            buffer[5 + ret] = path[ret];
        
        ret = sendwait(sock, buffer, 1 + 4 + len_path);
    }

error:
    bss.lock = 0;
}

void log_fchange_dir(int sock, int is_async, const char *path) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);
    
    int ret;
    int len_path = 0;
    while (path[len_path++]);
    
    //
    {
        char buffer[1 + 4 + len_path];
        buffer[0] = (is_async) ? BYTE_CHANGE_DIR_ASYNC : BYTE_CHANGE_DIR;
        *(int*)(buffer + 1) = len_path;
        for (ret = 0; ret < len_path; ret++)
            buffer[5 + ret] = path[ret];

        sendwait(sock, buffer, 1 + 4 + len_path);
    }

error:
    bss.lock = 0;
}

void log_fstat(int sock, int is_async, const char *path, void *ptr) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;

    CHECK_ERROR(sock == -1);

    int ret;
    int len_path = 0;
    while (path[len_path++]);

    //
    {
        char buffer[1 + 4 + len_path];
        buffer[0] = is_async ? BYTE_STAT_ASYNC : BYTE_STAT;
        *(int *)(buffer + 1) = len_path;
        for (ret = 0; ret < len_path; ret++)
            buffer[5 + ret] = path[ret];
        
        ret = sendwait(sock, buffer, 1 + 4 + len_path);
    }

error:
    bss.lock = 0;
}

//void log_fread_file(int sock, int is_async, void *ptr, int size, int count, int fd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 12];
//    buffer[0] = is_async ? BYTE_READ_FILE_ASYNC : BYTE_READ_FILE;
//    *(int *)(buffer + 1) = size;
//    *(int *)(buffer + 5) = count;
//    *(int *)(buffer + 9) = fd;
//    sendwait(sock, buffer, 1 + 12);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fread_dir(int sock, int is_async, void *dir_entry, int fd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = is_async ? BYTE_READ_DIR_ASYNC : BYTE_READ_DIR;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fclose_file(int sock, int is_async, int fd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = is_async ? BYTE_CLOSE_FILE_ASYNC : BYTE_CLOSE_FILE;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fclose_dir(int sock, int is_async, int fd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = is_async ? BYTE_CLOSE_DIR_ASYNC : BYTE_CLOSE_DIR;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fget_cwd(int sock, void *pwd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1];
//    buffer[0] = BYTE_GET_CWD;
//    sendwait(sock, buffer, 1);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fsetpos(int sock, int fd, int pos) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 8];
//    buffer[0] = BYTE_SETPOS;
//    *(int *)(buffer + 1) = fd;
//    *(int *)(buffer + 5) = pos;
//    sendwait(sock, buffer, 1 + 8);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fgetpos(int sock, int fd, int *pos) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = BYTE_GETPOS;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//void log_fstat_file(int sock, int fd, void *ptr) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = BYTE_STATFILE;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//
//
//void log_feof(int sock, int fd) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//
//    CHECK_ERROR(sock == -1);
//
//    char buffer[1 + 4];
//    buffer[0] = BYTE_EOF;
//    *(int *)(buffer + 1) = fd;
//    sendwait(sock, buffer, 1 + 4);
//
//error:
//    bss.lock = 0;
//}
//
//void log_send_ping(int sock, int val1, int val2) {
//    while (bss.lock) GX2WaitForVsync();
//    bss.lock = 1;
//    
//    int ret;
//    char buffer[1 + 4 + 4];
//    buffer[0] = BYTE_PING;
//    *(int *)(buffer + 1) = val1;
//    *(int *)(buffer + 5) = val2;
//
//    ret = sendwait(sock, buffer, 1 + 4 + 4);
//    CHECK_ERROR(ret < 0);
//
//error:
//    bss.lock = 0;
//    return;
//}

void log_string(int sock, const char* str) {
    while (bss.lock) GX2WaitForVsync();
    bss.lock = 1;
    
    CHECK_ERROR(sock == -1);

    int i;
    int len_str = 0;
    while (str[len_str++]);

    //
    {
        char buffer[1 + 4 + len_str];
        buffer[0] = BYTE_LOG_STR;
        *(int *)(buffer + 1) = len_str;
        for (i = 0; i < len_str; i++)
            buffer[5 + i] = str[i];
        
        sendwait(sock, buffer, 1 + 4 + len_str);
    }
    
error:
    bss.lock = 0;
}

static int recvwait(int sock, void *buffer, int len) {
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

static int recvbyte(int sock) {
    unsigned char buffer[1];
    int ret;

    ret = recvwait(sock, buffer, 1);
    if (ret < 0) return ret;
    return buffer[0];
}

static int sendwait(int sock, const void *buffer, int len) {
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
