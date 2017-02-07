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
