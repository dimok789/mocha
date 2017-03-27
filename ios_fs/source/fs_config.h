#ifndef _FS_CONFIG_H
#define _FS_CONFIG_H

typedef struct {
    int dump_slc;
    int dump_slccmpt;
    int dump_mlc;
    int dump_otp;
    int dump_seeprom;
    char otp_buffer[0x400];
    char seeprom_buffer[0x200];
} fs_config;

#endif