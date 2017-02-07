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
#ifndef _UTILS_H
#define _UTILS_H

#define ALIGN4(x)                   (((x) + 3) & ~3)

#define kernel_memcpy               ((void * (*)(void*, const void*, int))0x08131D04)
#define kernel_memset               ((void *(*)(void*, int, unsigned int))0x08131DA0)
#define kernel_strncpy              ((char *(*)(char*, const char*, unsigned int))0x081329B8)
#define disable_interrupts          ((int(*)())0x0812E778)
#define enable_interrupts           ((int(*)(int))0x0812E78C)
#define kernel_bsp_command_5        ((int (*)(const char*, int offset, const char*, int size, void *buffer))0x0812EC40)

void reverse_memcpy(void* dest, const void* src, unsigned int size);

static inline unsigned int disable_mmu(void)
{
	unsigned int control_register = 0;
	asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (control_register));
	asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register & 0xFFFFEFFA));
	return control_register;
}

static inline void restore_mmu(unsigned int control_register)
{
	asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register));
}

static inline void set_domain_register(unsigned int domain_register)
{
	asm volatile("MCR p15, 0, %0, c3, c0, 0" : : "r" (domain_register));
}

#endif
