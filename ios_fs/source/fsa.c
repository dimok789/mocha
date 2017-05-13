/***************************************************************************
 * Copyright (C) 2016
 * by Dimok
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any
 * damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any
 * purpose, including commercial applications, and to alter it and
 * redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you
 * must not claim that you wrote the original software. If you use
 * this software in a product, an acknowledgment in the product
 * documentation would be appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and
 * must not be misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 * distribution.
 ***************************************************************************/
#include <stdio.h>
#include "types.h"
#include "devices.h"
#include "imports.h"
#include "fsa.h"

//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//! FSA redirection
//!-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
char* FSA_AttachVolume_FillDescription_hook(char *dst, const char *src, int size)
{
    char* ret = FS_STRNCPY(dst, src, size);

    u8 *volDescr = (u8 *)(dst - 0x1C);

    // copy in a fake volume name and enable flag that it is set
    if( (FS_STRNCMP((char*)(volDescr + 0x1C), "usb", 3) == 0) && (FS_STRNCMP((char*)(volDescr + 0x24), "fat", 3) == 0) )
    {
        FS_STRNCPY((char*)(volDescr + 0xAC), "usbfat1", 8);
        *volDescr |= 0x40;
    }
    // let's do this for SD card to, for future SD card title loading maybe
    /*
    if( (FS_STRNCMP((char*)(volDescr + 0x1C), "sdcard", 3) == 0) && (FS_STRNCMP((char*)(volDescr + 0x24), "fat", 3) == 0) )
    {
        FS_STRNCPY(volDescr + 0xAC, "sdfat", 7);
        *volDescr |= 0x40;
    }
    */


    return ret;
}

int FSA_AsyncCommandCallback_hook(int result, int reply)
{
    int (*FSA_ConvertErrorCode)(int result, int reply) = (void*)0x1071209C;
    int res = FSA_ConvertErrorCode(result, reply);

    if(reply && (*(u32*)(reply + 0x42C) != 0))
    {
        u32 devDescr = *(u32*)(reply + 0x42C);
        int deviceType = *(u32*)(devDescr + 0x70);

        if(deviceType == DEVICE_TYPE_USB) // TODO: verify it is FAT USB and not WFS USB
        {
            u32 command = *(u32*)(reply + 0x1C);

            if(res < 0)
            {
                switch(command)
                {
                case 0x05: // FSMakeQuota     -> 0x1D
                case 0x1A: // FSFlushQuota    -> 0x1E
                case 0x1B: // FSRollbackQuota -> 0x1F
                case 0x1C: // FSChangeOwner   -> 0x70
                case 0x1D: // FSChangeMode    -> 0x20
                case 0x1E: // FSRemoveQuota   -> 0x72 // TODO: this actually removes the directory. we need to do something about it
                case 0x20: // Unkn            -> 0x74
                case 0x21: // FSMakeLinkAsync -> 0x75 // TODO: this is an issue on FAT. maybe create some file which has the name of the link in it?
                case 0x16: // unkn but required on install
                    res = 0;
                    break;
                case 0x04:
                    if(res == -196642)
                    {
                        // FS_SYSLOG_OUTPUT("FSA INVALID CHARACTERS IN CREATEDIR\n");
                        res = 0;
                    }
                    break;
                default:
                    break;
                }
            }
            else
            {
                switch(command)
                {/*
                case 0x10: // FSGetStatFile
                {
                    fileStat_s * fsStat = (fileStat_s*)(*(u32*)(reply + 0x2C));
                    FS_SYSLOG_OUTPUT("FSGetStatFile: %08X %08X %08X %08X\n", fsStat->flag, fsStat->owner_id, fsStat->group_id, fsStat->size);
                    break;
                }*/
                case 0x19: // FSGetInfo      -> 0x18
                {
                    switch(*(u32*)(reply + 0x228))
                    {
                    case 0x05: // FSGetStat
                    {
                        fileStat_s * fsStat = (fileStat_s*)(*(u32*)(reply + 0x22C));
                        if((fsStat->flag & 0xF0000000) == 0x80000000)
                        {
                            // just make every directory a quota -> probably wrong :P
                            fsStat->flag |= 0xE0000000;
                            fsStat->quota_size = 0x4000000; // max quota size
                        }
                        break;
                    }
                    default:
                        break;
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
    }

    if(res < 0 && (*(u32*)(reply + 0x1C) != 0x19))
        FS_SYSLOG_OUTPUT("FSA TEST2: res %d %08X %08X %08X\n", res, result, reply, *(u32*)(reply + 0x1C));


    return res;
}

int FSA_MakeQuota_hook(u32 * devDescr, u32 *commandStruct)
{
    int (*resolveNode)(u32 * devDescr) = (void*)0x1070EEDC;
    int res = resolveNode(devDescr);

    if(devDescr[0x70/4] == DEVICE_TYPE_USB)
    {
        commandStruct[0x1C/4] = 4;
    }
    else
    {
        commandStruct[0x1C/4] = 5;
    }

    return res;
}

/*
int FSMakeQuota(int fsa, int client, const char *path, int mode, int unkn, unsigned int size)
{
    int (*callFunc)(int fsa, int client, const char *path, int mode, int unkn, unsigned int size) = (void*)0x1070BC9C;

    if(FS_STRNCMP(path, "/vol/mcp_devmgr", 15) == 0)
    {
        mode = 0x666;
    }
    int res = callFunc(fsa, client, path, mode, unkn, size);
    FS_SYSLOG_OUTPUT("FSMakeQuota: res %d %08X %08X %20s %08X %08X %08X\n", res, fsa, client, path, mode, unkn, size);
    return res;
}

int FSCreateDir(int fsa, int client, const char *path, int mode)
{
    int (*callFunc)(int fsa, int client, const char *path, int mode) = (void*)0x1070BEBC;

    if(FS_STRNCMP(path, "/vol/mcp_devmgr", 15) == 0)
    {
        mode = 0x666;
    }
    int res = callFunc(fsa, client, path, mode);
    FS_SYSLOG_OUTPUT("FSCreateDir: res %d %08X %08X %s %08X\n", res, fsa, client, path, mode);
    return res;
}

int FSChangeDir(int a1, char *dir)
{
    int (*callFunc)(int a1, char* a2) = (void*)0x1070EB7C;
    int res = callFunc(a1, dir);

    FS_SYSLOG_OUTPUT("FSChangeDir: res %d %s\n", res, dir);

    return res;
}

int FSOpenFile(int a1, int a2, char *dir, char *mode, int a3, int a4, int a5, int a6)
{
    int (*callFunc)(int a1, int a2, char *dir, char *mode, int a3, int a4, int a5, int a6) = (void*)0x1070AF08;

    if(FS_STRNCMP(dir, "/vol/mcp_devmgr", 15) == 0)
    {
        a4 = 0x666;
    }

    int res = callFunc(a1, a2, dir, mode, a3, a4, a5, a6);

        FS_SYSLOG_OUTPUT("FSOpenFile: res %d %s %s %08X %08X %08X\n", res, dir, mode, a4, a5, a6);

    return res;
}

int FSWriteFileIssueCommand(int a1, int a2, int a3, int a4, signed int a5, int a6, int a7, int a8)
{
    int (*callFunc)(int a1, int a2, int a3, int a4, signed int a5, int a6, int a7, int a8) = (void*)0x1070A7A4;

    int res = callFunc(a1, a2, a3, a4, a5, a6, a7, a8);

    FS_SYSLOG_OUTPUT("FSWriteFile: res %d %08X %08X %08X %08X %08X %08X\n", res, a3, a4, a5, a6, a7, a8);

    return res;
}
*/
