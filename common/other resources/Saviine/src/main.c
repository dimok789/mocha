#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/aoc_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/padscore_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "dynamic_libs/ax_functions.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "common/common.h"
#include "game/rpx_rpl_table.h"
#include "game/memory_area_table.h"
#include "saviine.h"
#include "patcher/function_hooks.h"
#include "kernel/kernel_functions.h"

typedef union u_serv_ip
{
    uint8_t  digit[4];
    uint32_t full;
} u_serv_ip;

#define PRINT_TEXT2(x, y, ...) { snprintf(msg, 80, __VA_ARGS__); OSScreenPutFontEx(0, x, y, msg); OSScreenPutFontEx(1, x, y, msg); }

/* Entry point */
int Menu_Main(void)
{
    //!*******************************************************************
    //!                   Initialize function pointers                   *
    //!*******************************************************************
    //! do OS (for acquire) and sockets first so we got logging
    InitOSFunctionPointers();
    InitSocketFunctionPointers();
    InitFSFunctionPointers();
    InitVPadFunctionPointers();
    InitSysFunctionPointers();
    InitAocFunctionPointers();

    log_init("192.168.0.181");
    log_deinit();
    log_init("192.168.0.181");
    log_printf("Started %s\n", cosAppXmlInfoStruct.rpx_name);

    if(strcasecmp("men.rpx", cosAppXmlInfoStruct.rpx_name) == 0)
    {
        return EXIT_RELAUNCH_ON_LOAD;
    }
    else if(strlen(cosAppXmlInfoStruct.rpx_name) > 0 && strcasecmp("ffl_app.rpx", cosAppXmlInfoStruct.rpx_name) != 0)
    {
        StartDumper();
        return EXIT_RELAUNCH_ON_LOAD;
    }

    // initialize memory tables once on start
    memoryInitAreaTable();
    rpxRplTableInit();
    SetupKernelCallback();
    PatchMethodHooks();

    memoryInitialize();

    VPADInit();

    // Prepare screen
    int screen_buf0_size = 0;
    int screen_buf1_size = 0;

    // Init screen and screen buffers
    OSScreenInit();
    screen_buf0_size = OSScreenGetBufferSizeEx(0);
    screen_buf1_size = OSScreenGetBufferSizeEx(1);

    unsigned char *screenBuffer = MEM1_alloc(screen_buf0_size + screen_buf1_size, 0x40);

    OSScreenSetBufferEx(0, screenBuffer);
    OSScreenSetBufferEx(1, (screenBuffer + screen_buf0_size));

    OSScreenEnableEx(0, 1);
    OSScreenEnableEx(1, 1);

    char msg[80];
    uint8_t sel_ip = 3;
    int launchMethod = 0;
    int update_screen = 1;
    int vpadError = -1;
    VPADData vpad_data;
    u_serv_ip ip;
    ip.full = GetServerIp();
    int delay = 0;

    while (1)
    {
        // Read vpad
        VPADRead(0, &vpad_data, 1, &vpadError);

        if(update_screen)
        {
            OSScreenClearBufferEx(0, 0);
            OSScreenClearBufferEx(1, 0);

            // Print message
            PRINT_TEXT2(14, 1, "-- Saviine v1.1 by Maschell --");
            PRINT_TEXT2(0, 5, "1.    Setup IP address of server application.");

            // Print ip digit selector
            uint8_t x_shift = 17 + 4 * sel_ip;
            PRINT_TEXT2(x_shift, 6, "vvv");

            PRINT_TEXT2(0, 7, "      Server IP: %3d.%3d.%3d.%3d", ip.digit[0], ip.digit[1], ip.digit[2], ip.digit[3]);

            PRINT_TEXT2(0, 10, "2.   Press A to install Saviine and try to launch disc.");
            PRINT_TEXT2(0, 11, "  or Press X to install Saviine and return to system menu.");

            PRINT_TEXT2(0, 13, "3.   Start the title to be dumped.");

            PRINT_TEXT2(0, 17, "Press home button to exit ...");


            OSScreenFlipBuffersEx(0);
            OSScreenFlipBuffersEx(1);
        }

        u32 pressedBtns = vpad_data.btns_d | vpad_data.btns_h;

        // Check for buttons
        // Home Button
        if (pressedBtns & VPAD_BUTTON_HOME) {
            launchMethod = 0;
            break;
        }
        // A Button
        if (pressedBtns & VPAD_BUTTON_A) {
            SetServerIp(ip.full);
            launchMethod = 1;
            break;
        }
        // A Button
        if (pressedBtns & VPAD_BUTTON_X) {
            SetServerIp(ip.full);
            launchMethod = 2;
            break;
        }
        // Left/Right Buttons
        if (vpad_data.btns_d & VPAD_BUTTON_LEFT )
        {
            if(sel_ip == 0)
                sel_ip = 3;
            else
                --sel_ip;
        }

        if (vpad_data.btns_d & VPAD_BUTTON_RIGHT)
        {
            sel_ip = ((sel_ip + 1) % 4);
        }

        // Up/Down Buttons
        if (pressedBtns & VPAD_BUTTON_UP)
        {
            if(--delay <= 0) {
                ip.digit[sel_ip]++;
                delay = (vpad_data.btns_d & VPAD_BUTTON_UP) ? 6 : 0;
            }
        }
        else if (pressedBtns & VPAD_BUTTON_DOWN)
        {
            if(--delay <= 0) {
                ip.digit[sel_ip]--;
                delay = (vpad_data.btns_d & VPAD_BUTTON_DOWN) ? 6 : 0;
            }
        }
        else {
            delay = 0;
        }

        // Button pressed ?
        update_screen = (pressedBtns & (VPAD_BUTTON_LEFT | VPAD_BUTTON_RIGHT | VPAD_BUTTON_UP | VPAD_BUTTON_DOWN)) ? 1 : 0;
        usleep(20000);
    }

	MEM1_free(screenBuffer);
	screenBuffer = NULL;

    log_deinit();

    memoryRelease();

    if(launchMethod == 0)
    {
        RestoreInstructions();
        return EXIT_SUCCESS;
    }
    else if(launchMethod == 1)
    {
        char buf_vol_odd[20];
        snprintf(buf_vol_odd, sizeof(buf_vol_odd), "%s", "/vol/storage_odd03");
        _SYSLaunchTitleByPathFromLauncher(buf_vol_odd, 18, 0);
    }
    else
    {
        SYSLaunchMenu();
    }

    return EXIT_RELAUNCH_ON_LOAD;
}

