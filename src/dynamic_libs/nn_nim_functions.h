#ifndef __NN_NIM_FUNCTIONS_H_
#define __NN_NIM_FUNCTIONS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <gctypes.h>

/* Handle for coreinit */
extern u32 nn_nim_handle;

void InitAcquireNim(void);
void InitNimFunctionPointers(void);

#ifdef __cplusplus
}
#endif

#endif // __NN_NIM_FUNCTIONS_H_
