.arm

patch_wfs_partition_check:
    mov r0, #0

.globl acp_patches_table, acp_patches_table_end
acp_patches_table:
#          origin           data                                    size
#     .word 0xE00605D0,      patch_wfs_partition_check,              4
acp_patches_table_end:

