/* string.h */
#define NULL ((void *)0)

typedef unsigned int uint;

void *memcpy(void *dst, const void *src, int bytes);
void *memset(void *dst, int val, int bytes);

/* malloc.h */
extern void *(* const MEMAllocFromDefaultHeapEx)(int size, int align);
#define memalign (*MEMAllocFromDefaultHeapEx)
/* socket.h */
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6

extern void socket_lib_init();
extern int socket(int domain, int type, int protocol);
extern int socketclose(int socket);
extern int connect(int socket, void *addr, int addrlen);
extern int send(int socket, const void *buffer, int size, int flags);
extern int recv(int socket, void *buffer, int size, int flags);

extern void theExit();
extern int socketlasterr();

extern void GX2WaitForVsync(void);

struct in_addr {
    unsigned int s_addr;
};
struct sockaddr_in {
    short sin_family;
    unsigned short sin_port;
    struct in_addr sin_addr;
    char sin_zero[8];
};

/* OS stuff */
extern const long long title_id;

/* SDCard stuff */
#define FS_MAX_LOCALPATH_SIZE           511
#define FS_MAX_MOUNTPATH_SIZE           128
#define FS_MAX_FULLPATH_SIZE            (FS_MAX_LOCALPATH_SIZE + FS_MAX_MOUNTPATH_SIZE)
#define FS_SOURCETYPE_EXTERNAL          0

typedef int FSStatus;
typedef uint FSRetFlag;
typedef uint FSSourceType;

typedef struct
{
    FSSourceType          type;
    char                  path[FS_MAX_FULLPATH_SIZE];
} FSMountSource;

extern FSStatus FSGetMountSource(void *pClient, void *pCmd, FSSourceType type, FSMountSource *source, FSRetFlag errHandling);
extern FSStatus FSMount(void *pClient, void *pCmd, FSMountSource *source, char *target, uint bytes, FSRetFlag errHandling);

/* Forward declarations */
#define MAX_CLIENT 32
#define MASK_FD 0x0FFF00FF

struct bss_t {
    int socket_fs[MAX_CLIENT];
    void *pClient_fs[MAX_CLIENT];
    volatile int lock;
    int sd_mount[MAX_CLIENT];
    char mount_base[32]; // example : /vol/external01/000500101004A000
};

#define bss_ptr (*(struct bss_t **)0x100000E4)
#define bss (*bss_ptr)

int  fs_connect(int *socket);
void fs_disconnect(int socket);
int  fs_mount_sd(int sock, void* pClient, void* pCmd);
void log_fopen_file(int socket, int is_async, const char *path, const char *mode, int *handle);
void log_fopen_dir(int socket, int is_async, const char *path, int *handle);
void log_fread_file(int socket, int is_async, void *buffer, int size, int count, int fd);
void log_fread_dir(int socket, int is_async, void *dir_entry, int fd);
void log_fclose_file(int sock, int is_async, int fd);
void log_fclose_dir(int socket, int is_async, int fd);
void log_fchange_dir(int socket, int is_async, const char *path);
void log_fget_cwd(int socket, void *buffer);
void log_fsetpos(int socket, int fd, int pos);
void log_fgetpos(int socket, int fd, int *pos);
void log_fstat(int sock, int is_async, const char *path, void *ptr);
void log_fstat_file(int sock, int fd, void *ptr);
void log_feof(int sock, int fd);
void log_send_ping(int sock, int val1, int val2);
void log_string(int sock, const char* str);