#include "main.h"

#define DECL(res, name, ...) \
	extern res name(__VA_ARGS__); \
	res (* real_ ## name)(__VA_ARGS__)  __attribute__((section(".magicptr"))); \
	res my_ ## name(__VA_ARGS__)

static int client_num_alloc(void *pClient) {
    int i;

    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == 0) {
            bss.pClient_fs[i] = pClient;
            return i;
        }
    return -1;
}
static void client_num_free(int client) {
    bss.pClient_fs[client] = 0;
}
static int client_num(void *pClient) {
    int i;
    for (i = 0; i < MAX_CLIENT; i++)
        if (bss.pClient_fs[i] == pClient)
            return i;
    return -1;
}

static int strlen(const char* path) {
    int i = 0;
    while (path[i++])
        ;
    return i;
}
static int is_gamefile(const char *path) {
    if (path[0]  != '/') return 0;
    if (path[1]  != 'v') return 0;
    if (path[2]  != 'o') return 0;
    if (path[3]  != 'l') return 0;
    if (path[4]  != '/') return 0;
    if (path[5]  != 'c') return 0;
    if (path[6]  != 'o') return 0;
    if (path[7]  != 'n') return 0;
    if (path[8]  != 't') return 0;
    if (path[9]  != 'e') return 0;
    if (path[10] != 'n') return 0;
    if (path[11] != 't') return 0;

    return 1;
}
static void compute_new_path(char* new_path, const char* path, int len) {
    int i;
    
    for (i = 0; i < 32; i++)
        new_path[i] = bss.mount_base[i];

    for (i = 0; i < (len - 12); i++)
        new_path[32 + i] = path[12 + i];

    new_path[32 + i] = '\0';
}

// Async
typedef void (*FSAsyncCallback)(void *pClient, void *pCmd, int result, void *context);
typedef struct
{
    FSAsyncCallback userCallback;
    void            *userContext;
    void            *ioMsgQueue;
} FSAsyncParams;

// title id
typedef struct sTitle {
    unsigned int v0 : 4;
    unsigned int v1 : 4;
    unsigned int v2 : 4;
    unsigned int v3 : 4;
    unsigned int v4 : 4;
    unsigned int v5 : 4;
    unsigned int v6 : 4;
    unsigned int v7 : 4;
    unsigned int v8 : 4;
    unsigned int v9 : 4;
    unsigned int vA : 4;
    unsigned int vB : 4;
    unsigned int vC : 4;
    unsigned int vD : 4;
    unsigned int vE : 4;
    unsigned int vF : 4;
} sTitle;
    
typedef union uTitle {
    long long full;
    sTitle    val;
} uTitle;

/* *****************************************************************************
 * Base functions
 * ****************************************************************************/
DECL(int, FSInit, void) {
    if ((int)bss_ptr == 0x0A000000) {
        bss_ptr = memalign(sizeof(struct bss_t), 0x40);
        memset(bss_ptr, 0, sizeof(struct bss_t));
        
        // set mount base
        bss.mount_base[0]  = '/';
        bss.mount_base[1]  = 'v';
        bss.mount_base[2]  = 'o';
        bss.mount_base[3]  = 'l';
        bss.mount_base[4]  = '/';
        bss.mount_base[5]  = 'e';
        bss.mount_base[6]  = 'x';
        bss.mount_base[7]  = 't';
        bss.mount_base[8]  = 'e';
        bss.mount_base[9]  = 'r';
        bss.mount_base[10] = 'n';
        bss.mount_base[11] = 'a';
        bss.mount_base[12] = 'l';
        bss.mount_base[13] = '0';
        bss.mount_base[14] = '1';
        bss.mount_base[15] = '/';
        
        uTitle t;
        t.full = title_id;
        bss.mount_base[16] = (t.val.v0 < 0xA) ? ('0' + t.val.v0) : ('A' + t.val.v0 - 0xA);
        bss.mount_base[17] = (t.val.v1 < 0xA) ? ('0' + t.val.v1) : ('A' + t.val.v1 - 0xA);
        bss.mount_base[18] = (t.val.v2 < 0xA) ? ('0' + t.val.v2) : ('A' + t.val.v2 - 0xA);
        bss.mount_base[19] = (t.val.v3 < 0xA) ? ('0' + t.val.v3) : ('A' + t.val.v3 - 0xA);
        bss.mount_base[20] = (t.val.v4 < 0xA) ? ('0' + t.val.v4) : ('A' + t.val.v4 - 0xA);
        bss.mount_base[21] = (t.val.v5 < 0xA) ? ('0' + t.val.v5) : ('A' + t.val.v5 - 0xA);
        bss.mount_base[22] = (t.val.v6 < 0xA) ? ('0' + t.val.v6) : ('A' + t.val.v6 - 0xA);
        bss.mount_base[23] = (t.val.v7 < 0xA) ? ('0' + t.val.v7) : ('A' + t.val.v7 - 0xA);
        bss.mount_base[24] = (t.val.v8 < 0xA) ? ('0' + t.val.v8) : ('A' + t.val.v8 - 0xA);
        bss.mount_base[25] = (t.val.v9 < 0xA) ? ('0' + t.val.v9) : ('A' + t.val.v9 - 0xA);
        bss.mount_base[26] = (t.val.vA < 0xA) ? ('0' + t.val.vA) : ('A' + t.val.vA - 0xA);
        bss.mount_base[27] = (t.val.vB < 0xA) ? ('0' + t.val.vB) : ('A' + t.val.vB - 0xA);
        bss.mount_base[28] = (t.val.vC < 0xA) ? ('0' + t.val.vC) : ('A' + t.val.vC - 0xA);
        bss.mount_base[29] = (t.val.vD < 0xA) ? ('0' + t.val.vD) : ('A' + t.val.vD - 0xA);
        bss.mount_base[30] = (t.val.vE < 0xA) ? ('0' + t.val.vE) : ('A' + t.val.vE - 0xA);
        bss.mount_base[31] = (t.val.vF < 0xA) ? ('0' + t.val.vF) : ('A' + t.val.vF - 0xA);
    }
    return real_FSInit();
}
DECL(int, FSShutdown, void) {
    return real_FSShutdown();
}

DECL(int, FSAddClientEx, void *r3, void *r4, void *r5) {
    int res = real_FSAddClientEx(r3, r4, r5);

    if ((int)bss_ptr != 0x0A000000 && res >= 0) {
        int client = client_num_alloc(r3);
        if (client < MAX_CLIENT && client >= 0) {
            if (fs_connect(&bss.socket_fs[client]) != 0)
                client_num_free(client);
        }
    }

    return res;
}
DECL(int, FSDelClient, void *pClient) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            fs_disconnect(bss.socket_fs[client]);
            client_num_free(client);
        }
    }

    return real_FSDelClient(pClient);
}

/* *****************************************************************************
 * Replacement functions
 * ****************************************************************************/
DECL(int, FSGetStat, void *pClient, void *pCmd, const char *path, void *stats, int error) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fstat(bss.socket_fs[client], 0, path, stats);

            // change path if it is a game file
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int ret = real_FSGetStat(pClient, pCmd, new_path, stats, -1);
                if (ret == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return ret;
                }
            }
        }
    }
    return real_FSGetStat(pClient, pCmd, path, stats, error);
}

DECL(int, FSGetStatAsync, void *pClient, void *pCmd, const char *path, void *stats, int error, FSAsyncParams *asyncParams) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fstat(bss.socket_fs[client], 1, path, stats);

            // change path if it is a game file
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSGetStatAsync(pClient, pCmd, new_path, stats, error, asyncParams);
                }
            }
        }
    }
    return real_FSGetStatAsync(pClient, pCmd, path, stats, error, asyncParams);
}

DECL(int, FSOpenFile, void *pClient, void *pCmd, const char *path, const char *mode, int *handle, int error) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fopen_file(bss.socket_fs[client], 0, path, mode, handle);

            // change path if it is a game file
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSOpenFile(pClient, pCmd, new_path, mode, handle, error);
                }
            }
        }
    }

    return real_FSOpenFile(pClient, pCmd, path, mode, handle, error);
}

DECL(int, FSOpenFileAsync, void *pClient, void *pCmd, const char *path, const char *mode, int *handle, int error, FSAsyncParams *asyncParams) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fopen_file(bss.socket_fs[client], 1, path, mode, handle);
            
            // change path if it is a game file
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSOpenFileAsync(pClient, pCmd, new_path, mode, handle, error, asyncParams);
                }
            }
        }
    }
    return real_FSOpenFileAsync(pClient, pCmd, path, mode, handle, error, asyncParams);
}

DECL(int, FSOpenDir, void *pClient, void* pCmd, const char *path, int *handle, int error) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fopen_dir(bss.socket_fs[client], 0, path, handle);
            
            // change path if it is a game folder
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSOpenDir(pClient, pCmd, new_path, handle, error);
                }
            }
        }
    }
    return real_FSOpenDir(pClient, pCmd, path, handle, error);
}

DECL(int, FSOpenDirAsync, void *pClient, void* pCmd, const char *path, int *handle, int error, FSAsyncParams *asyncParams) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fopen_dir(bss.socket_fs[client], 1, path, handle);
            
            // change path if it is a game folder
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSOpenDirAsync(pClient, pCmd, new_path, handle, error, asyncParams);
                }
            }
        }
    }
    return real_FSOpenDirAsync(pClient, pCmd, path, handle, error, asyncParams);
}

DECL(int, FSChangeDir, void *pClient, void *pCmd, const char *path, int error) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fchange_dir(bss.socket_fs[client], 0, path);

            // change path if it is a game folder
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSChangeDir(pClient, pCmd, new_path, error);
                }
            }
        }
    }
    return real_FSChangeDir(pClient, pCmd, path, error);
}

DECL(int, FSChangeDirAsync, void *pClient, void *pCmd, const char *path, int error, FSAsyncParams *asyncParams) {
    if ((int)bss_ptr != 0x0A000000) {
        int client = client_num(pClient);
        if (client < MAX_CLIENT && client >= 0) {
            // log
            if (bss.socket_fs[client] != -1)
                log_fchange_dir(bss.socket_fs[client], 1, path);
            
            // change path if it is a game folder
            if (is_gamefile(path)) {
                int len = strlen(path);
                char new_path[len + 20 + 1];
                compute_new_path(new_path, path, len);

                // mount sd
                if (!bss.sd_mount[client])
                    bss.sd_mount[client] = fs_mount_sd(bss.socket_fs[client], pClient, pCmd);

                // return function with new_path if path exists
                int tmp_stats[25];
                if (real_FSGetStat(pClient, pCmd, new_path, &tmp_stats, -1) == 0) {
                    // log new path
                    if (bss.socket_fs[client] != -1)
                        log_string(bss.socket_fs[client], new_path);
                    return real_FSChangeDirAsync(pClient, pCmd, new_path, error, asyncParams);
                }
            }
        }
    }
    return real_FSChangeDirAsync(pClient, pCmd, path, error, asyncParams);
}

/* *****************************************************************************
 * Log functions
 * ****************************************************************************/
//DECL(int, FSCloseFile, void *pClient, void *pCmd, int fd, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fclose_file(bss.socket_fs[client], 0, fd);
//        }
//    }
//
//    return real_FSCloseFile(pClient, pCmd, fd, error);
//}
//
//DECL(int, FSCloseFileAsync, void *pClient, void *pCmd, int fd, int error, void *asyncParams) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fclose_file(bss.socket_fs[client], 1, fd);
//        }
//    }
//    return real_FSCloseFileAsync(pClient, pCmd, fd, error, asyncParams);
//}
//
//DECL(int, FSSetPosFile, void *pClient, void *pCmd, int fd, int pos, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fsetpos(bss.socket_fs[client], fd, pos);
//        }
//    }
//
//    return real_FSSetPosFile(pClient, pCmd, fd, pos, error);
//}
//DECL(int, FSGetPosFile, void *pClient, void *pCmd, int fd, int *pos, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fgetpos(bss.socket_fs[client], fd, pos);
//        }
//    }
//
//    return real_FSGetPosFile(pClient, pCmd, fd, pos, error);
//}
//DECL(int, FSGetStatFile, void *pClient, void *pCmd, int fd, void *buffer, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fstat_file(bss.socket_fs[client], fd, buffer);
//        }
//    }
//
//    return real_FSGetStatFile(pClient, pCmd, fd, buffer, error);
//}
//DECL(int, FSIsEof, void *pClient, void *pCmd, int fd, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_feof(bss.socket_fs[client], fd);
//        }
//    }
//
//    return real_FSIsEof(pClient, pCmd, fd, error);
//}
//
//DECL(int, FSReadFile, void *pClient, void *pCmd, void *buffer, int size, int count, int fd, int flag, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fread_file(bss.socket_fs[client], 0, buffer, size, count, fd);
//        }
//    }
//
//    return real_FSReadFile(pClient, pCmd, buffer, size, count, fd, flag, error);
//}
//DECL(int, FSReadFileWithPos, void *pClient, void *pCmd, void *buffer, int size, int count, int pos, int fd, int flag, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fsetpos(bss.socket_fs[client], fd, pos);
//
//            // just log
//            log_fread_file(bss.socket_fs[client], 0, buffer, size, count, fd);
//        }
//    }
//
//    return real_FSReadFileWithPos(pClient, pCmd, buffer, size, count, pos, fd, flag, error);
//}
//
//DECL(int, FSReadFileAsync, void *pClient, void *pCmd, void *buffer, int size, int count, int fd, int flag, int error, FSAsyncParams *asyncParams) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fread_file(bss.socket_fs[client], 1, buffer, size, count, fd);
//        }
//    }
//    return real_FSReadFileAsync(pClient, pCmd, buffer, size, count, fd, flag, error, asyncParams);
//}
//
//DECL(int, FSCloseDir, void *pClient, void *pCmd, int fd, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fclose_dir(bss.socket_fs[client], 0, fd);
//        }
//    }
//    return real_FSCloseDir(pClient, pCmd, fd, error);
//}
//
//DECL(int, FSGetCwd, void *pClient, void *pCmd, void *buffer, int bytes, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fget_cwd(bss.socket_fs[client], buffer);
//        }
//    }
//    return real_FSGetCwd(pClient, pCmd, buffer, bytes, error);
//}
//
//DECL(int, FSReadDir, void *pClient, void *pCmd, int fd, void *dir_entry, int error) {
//    if ((int)bss_ptr != 0x0a000000) {
//        int client = client_num(pClient);
//        if (client < MAX_CLIENT && client >= 0) {
//            // just log
//            log_fread_dir(bss.socket_fs[client], 0, dir_entry, fd);
//        }
//    }
//    return real_FSReadDir(pClient, pCmd, fd, dir_entry, error);
//}

/* *****************************************************************************
 * Creates function pointer array
 * ****************************************************************************/

#define TYPE_ALL        0xff
#define TYPE_REPLACE    0x01
#define TYPE_LOG        0x02

#define MAKE_MAGIC(x, type) { x, my_ ## x, &real_ ## x, type }

struct magic_t {
    const void *real;
    const void *replacement;
    const void *call;
    uint        type;
} methods[] __attribute__((section(".magic"))) = {
    MAKE_MAGIC(FSInit,              TYPE_ALL),
    MAKE_MAGIC(FSShutdown,          TYPE_ALL),
    MAKE_MAGIC(FSAddClientEx,       TYPE_ALL),
    MAKE_MAGIC(FSDelClient,         TYPE_ALL),
    MAKE_MAGIC(FSGetStat,           TYPE_ALL),
    MAKE_MAGIC(FSGetStatAsync,      TYPE_ALL),
    MAKE_MAGIC(FSOpenFile,          TYPE_ALL),
    MAKE_MAGIC(FSOpenFileAsync,     TYPE_ALL),
    MAKE_MAGIC(FSOpenDir,           TYPE_ALL),
    MAKE_MAGIC(FSOpenDirAsync,      TYPE_ALL),
    MAKE_MAGIC(FSChangeDir,         TYPE_ALL),
    MAKE_MAGIC(FSChangeDirAsync,    TYPE_ALL),

//    MAKE_MAGIC(FSCloseFile,         TYPE_LOG),
//    MAKE_MAGIC(FSCloseFileAsync,    TYPE_LOG),
//    MAKE_MAGIC(FSSetPosFile,        TYPE_LOG),
//    MAKE_MAGIC(FSGetPosFile,        TYPE_LOG),
//    MAKE_MAGIC(FSGetStatFile,       TYPE_LOG),
//    MAKE_MAGIC(FSIsEof,             TYPE_LOG),
//    MAKE_MAGIC(FSReadFile,          TYPE_LOG),
//    MAKE_MAGIC(FSReadFileWithPos,   TYPE_LOG),
//    MAKE_MAGIC(FSReadFileAsync,     TYPE_LOG),
//    MAKE_MAGIC(FSCloseDir,          TYPE_LOG),
//    MAKE_MAGIC(FSGetCwd,            TYPE_LOG),
//    MAKE_MAGIC(FSReadDir,           TYPE_LOG),
};
