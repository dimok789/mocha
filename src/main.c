#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <malloc.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "dynamic_libs/os_functions.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/gx2_functions.h"
#include "dynamic_libs/sys_functions.h"
#include "dynamic_libs/vpad_functions.h"
#include "dynamic_libs/socket_functions.h"
#include "fs/fs_utils.h"
#include "fs/sd_fat_devoptab.h"
#include "system/memory.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "common/common.h"
#include "menu.h"
#include "main.h"
#include "ios_exploit.h"

static int exitToHBLOnLaunch = 0;

int Menu_Main(void)
{
	//!---------INIT---------
	InitOSFunctionPointers();
	InitSysFunctionPointers();
    InitFSFunctionPointers();
    InitSocketFunctionPointers();
    InitVPadFunctionPointers();

    u64 currenTitleId = OSGetTitleID();

    // in case we are not in mii maker or HBL channel but in system menu or another channel we need to exit here
    if (currenTitleId != 0x000500101004A200 && // mii maker eur
        currenTitleId != 0x000500101004A100 && // mii maker usa
        currenTitleId != 0x000500101004A000 && // mii maker jpn
        currenTitleId != 0x0005000013374842)    // HBL channel
    {
        return EXIT_RELAUNCH_ON_LOAD;
    }
    else if(exitToHBLOnLaunch)
    {
        return 0;
    }

    VPADInit();
    int forceMenu = 0;

    {
        VPADData vpad;
        int vpadError = -1;
        VPADRead(0, &vpad, 1, &vpadError);

        if(vpadError == 0)
        {
            forceMenu = (vpad.btns_d | vpad.btns_h) & VPAD_BUTTON_B;
        }
    }

    mount_sd_fat("sd");

    cfw_config_t config;
    default_config(&config);
    read_config(&config);

    int launch = 1;

    if(forceMenu || config.directLaunch == 0)
    {
        launch = ShowMenu(&config);
    }

    int returnCode = 0;

    if(launch)
    {
        int res = ExecuteIOSExploit(&config);
        if(res == 0)
        {
            if(config.noIosReload == 0)
            {
                OSForceFullRelaunch();
                SYSLaunchMenu();
                returnCode = EXIT_RELAUNCH_ON_LOAD;
            }
            else if(config.launchSysMenu)
            {
                SYSLaunchMenu();
                exitToHBLOnLaunch = 1;
                returnCode = EXIT_RELAUNCH_ON_LOAD;
            }
        }
    }

    unmount_sd_fat("sd");

    return returnCode;
}
