#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "common/common.h"
#include "common/fs_defs.h"
#include "common/loader_defs.h"
#include "game/rpx_rpl_table.h"
#include "dynamic_libs/fs_functions.h"
#include "dynamic_libs/os_functions.h"
#include "kernel/kernel_functions.h"
#include "function_hooks.h"
#include "saviine.h"

#define LIB_CODE_RW_BASE_OFFSET                         0xC1000000
#define CODE_RW_BASE_OFFSET                             0x00000000

#define USE_EXTRA_LOG_FUNCTIONS   0

#define DECL(res, name, ...) \
        res (* real_ ## name)(__VA_ARGS__) __attribute__((section(".data"))); \
        res my_ ## name(__VA_ARGS__)

/* *****************************************************************************
 * Creates function pointer array
 * ****************************************************************************/
#define MAKE_MAGIC(x, lib) { (unsigned int) my_ ## x, (unsigned int) &real_ ## x, lib, # x }

static const struct hooks_magic_t {
    const unsigned int replaceAddr;
    const unsigned int replaceCall;
    const unsigned int library;
    const char functionName[30];
} method_hooks[] = {
    // LOADER function
    //MAKE_MAGIC(LiWaitOneChunk,              LIB_LOADER),

};

//! buffer to store our 2 instructions needed for our replacements
//! the code will be placed in the address of that buffer - CODE_RW_BASE_OFFSET
//! avoid this buffer to be placed in BSS and reset on start up
volatile unsigned int fs_method_calls[sizeof(method_hooks) / sizeof(struct hooks_magic_t) * 2] __attribute__((section(".data")));

void PatchMethodHooks(void)
{
    restore_instructions_t * restore = (restore_instructions_t *)(RESTORE_INSTR_ADDR);
    //! check if it is already patched
    if(restore->magic == RESTORE_INSTR_MAGIC)
        return;

    restore->magic = RESTORE_INSTR_MAGIC;
    restore->instr_count = 0;

    bat_table_t table;
    KernelSetDBATs(&table);

    /* Patch branches to it. */
    volatile unsigned int *space = &fs_method_calls[0];

    int method_hooks_count = sizeof(method_hooks) / sizeof(struct hooks_magic_t);

    for(int i = 0; i < method_hooks_count; i++)
    {
        unsigned int repl_addr = (unsigned int)method_hooks[i].replaceAddr;
        unsigned int call_addr = (unsigned int)method_hooks[i].replaceCall;

        unsigned int real_addr = 0;

        if(strcmp(method_hooks[i].functionName, "OSDynLoad_Acquire") == 0)
        {
            memcpy(&real_addr, &OSDynLoad_Acquire, 4);
        }
        else if(strcmp(method_hooks[i].functionName, "LiWaitOneChunk") == 0)
        {
            memcpy(&real_addr, &addr_LiWaitOneChunk, 4);
        }
        else
        {
            OSDynLoad_FindExport(coreinit_handle, 0, method_hooks[i].functionName, &real_addr);
        }

        // fill the restore instruction section
        restore->data[restore->instr_count].addr = real_addr;
        restore->data[restore->instr_count].instr = *(volatile unsigned int *)(LIB_CODE_RW_BASE_OFFSET + real_addr);
        restore->instr_count++;

        // set pointer to the real function
        *(volatile unsigned int *)(call_addr) = (unsigned int)(space) - CODE_RW_BASE_OFFSET;
        DCFlushRange((void*)(call_addr), 4);

        // fill the instruction of the real function
        *space = *(volatile unsigned int*)(LIB_CODE_RW_BASE_OFFSET + real_addr);
        space++;

        // jump to real function skipping the first/replaced instruction
        *space = 0x48000002 | ((real_addr + 4) & 0x03fffffc);
        space++;
        DCFlushRange((void*)(space - 2), 8);
        ICInvalidateRange((unsigned char*)(space - 2) - CODE_RW_BASE_OFFSET, 8);

        unsigned int replace_instr = 0x48000002 | (repl_addr & 0x03fffffc);
        *(volatile unsigned int *)(LIB_CODE_RW_BASE_OFFSET + real_addr) = replace_instr;
        DCFlushRange((void*)(LIB_CODE_RW_BASE_OFFSET + real_addr), 4);
        ICInvalidateRange((void*)(real_addr), 4);
    }

    KernelRestoreDBATs(&table);
}

/* ****************************************************************** */
/*                  RESTORE ORIGINAL INSTRUCTIONS                     */
/* ****************************************************************** */
void RestoreInstructions(void)
{
    bat_table_t table;
    KernelSetDBATs(&table);

    restore_instructions_t * restore = (restore_instructions_t *)(RESTORE_INSTR_ADDR);
    if(restore->magic == RESTORE_INSTR_MAGIC)
    {
        for(unsigned int i = 0; i < restore->instr_count; i++)
        {
            *(volatile unsigned int *)(LIB_CODE_RW_BASE_OFFSET + restore->data[i].addr) = restore->data[i].instr;
            DCFlushRange((void*)(LIB_CODE_RW_BASE_OFFSET + restore->data[i].addr), 4);
            ICInvalidateRange((void*)restore->data[i].addr, 4);
        }

    }
    restore->magic = 0;
    restore->instr_count = 0;

    KernelRestoreDBATs(&table);
    KernelRestoreInstructions();
}
