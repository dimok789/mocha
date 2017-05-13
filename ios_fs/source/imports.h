#ifndef IMPORTS_H_
#define IMPORTS_H_

#define FS_IOS_SHUTDOWN                     ((void (*)(int))0x107F6C94)

#define FS_SVC_CREATEMUTEX                  ((int (*)(int, int))0x107F6BBC)
#define FS_SVC_ACQUIREMUTEX                 ((int (*)(int, int))0x107F6BC4)
#define FS_SVC_RELEASEMUTEX                 ((int (*)(int))0x107F6BCC)
#define FS_SVC_DESTROYMUTEX                 ((int (*)(int))0x107F6BD4)

#define FS_SLEEP                            ((void (*)(int))0x1071D668)
#define FS_MEMCPY                           ((void* (*)(void*, const void*, u32))0x107F4F7C)
#define FS_MEMSET                           ((void* (*)(void*, int, u32))0x107F5018)
#define FS_VSNPRINTF                        ((int (*)(char * s, u32 n, const char * format, va_list arg))0x107F5F68)
#define FS_SNPRINTF                         ((int (*)(char * s, u32 n, const char * format, ...))0x107F5FB4)
#define FS_STRNCMP                          ((int (*)(const char *s1, const char *s2, u32 size))0x107F6138)
#define FS_STRNCPY                          ((char* (*)(char *s1, const char *s2, u32 size))0x107F60DC)
#define FS_SYSLOG_OUTPUT                    ((void (*)(const char *format, ...))0x107F0C84)
#define FS_RAW_READ1                        ((int (*)(int handle, u32 offset_high, u32 offset_low, u32 size, void* buf, void *callback, int callback_arg))0x10732BC0)
#define FS_SDIO_DOREADWRITECOMMAND          ((int (*)(int, void*, u32, void*, void*))0x10718A8C)

#define FS_REGISTERMDPHYSICALDEVICE         ((int (*)(void*, int, int))0x10718860)

#define memcpy                              FS_MEMCPY
#define memset                              FS_MEMSET

#define FS_MMC_SDCARD_STRUCT                ((vu32*)0x1089B9F8)
#define FS_MMC_MLC_STRUCT                   ((vu32*)0x1089B948)

#define FS_MLC_PHYS_DEV_STRUCT              ((void*)0x11C3A14C)
#define FS_SLC_PHYS_DEV_STRUCT              ((void*)0x11C381CC)
#define FS_SLCCMPT_PHYS_DEV_STRUCT          ((void*)0x11C37668)

#define le16(i)                             ((((u16) ((i) & 0xFF)) << 8) | ((u16) (((i) & 0xFF00) >> 8)))
#define le32(i)                             ((((u32)le16((i) & 0xFFFF)) << 16) | ((u32)le16(((i) & 0xFFFF0000) >> 16)))
#define le64(i)                             ((((u64)le32((i) & 0xFFFFFFFFLL)) << 32) | ((u64)le32(((i) & 0xFFFFFFFF00000000LL) >> 32)))

#endif // IMPORTS_H_
