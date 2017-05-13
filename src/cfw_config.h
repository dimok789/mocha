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
#ifndef CFW_CONFIG_H_
#define CFW_CONFIG_H_

#define APP_VERSION         "v0.2"
#define APP_PATH            "sd:/wiiu/apps/mocha"
#define CONFIG_PATH         (APP_PATH "/config.ini")

#include "../common/config_types.h"

void default_config(cfw_config_t * config);
int read_config(cfw_config_t * config);
int write_config(cfw_config_t * config);

#endif
