.arm
.align 4
patch_kernel_domains:
	str r3, [r7,#0x10]
	str r3, [r7,#0x0C]
	str r3, [r7,#0x04]
	str r3, [r7,#0x14]
	str r3, [r7,#0x08]
	str r3, [r7,#0x34]
patch_kernel_domains_end:

.globl kernel_patches_table, kernel_patches_table_end
kernel_patches_table:
#          origin           data                                    size
     .word 0x081253C4,      patch_kernel_domains,                   (kernel_patches_table_end - kernel_patches_table)
kernel_patches_table_end:

