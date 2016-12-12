#ifndef FSA_H
#define FSA_H

#include "types.h"

int FSA_RawOpen(int fd, const char* device_path, int* outHandle);
int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
int FSA_RawClose(int fd, int device_handle);

#endif
