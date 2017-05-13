#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "wupserver.h"
#include "ipc.h"
#include "svc.h"
#include "text.h"
#include "../../common/config_types.h"
#include "../../common/kernel_commands.h"

static int threadsStarted = 0;

int _startMainThread(void)
{
    if(threadsStarted == 0)
    {
        threadsStarted = 1;
        cfw_config_t cfw_config;
        memset(&cfw_config, 0, sizeof(cfw_config));
        svcCustomKernelCommand(KERNEL_GET_CFW_CONFIG, &cfw_config);

        if(cfw_config.launchImage)
        {
            drawSplashScreen();
        }

        wupserver_init();
        ipc_init();
    }
    return 0;
}

void patch_SD_access_check(void) {
    __asm__ volatile(
        ".thumb\n"
        //clobbered instructions
        "add r0, r7, r2\n"
        //app.permissions.r2.mask seems to be 0xFFFFFFFFFFFFFFFF for every application
        "ldr r1, =0x32\n"
        "sub r3, r3, #7\n"
        "strb r1, [r3]\n"
        //this instruction was also clobbered but we use r1 so we do it after our patch stuff
        "movs r1, #0\n"
        "bx lr");
}
