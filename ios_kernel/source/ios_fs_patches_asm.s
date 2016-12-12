.arm

# patch out sdcard deinitialization
patch_mdExit:
    bx lr

# patch out FSRawOpen access
patch_FSRawOpen:
    streq r2, [r1, #0x70]
    .word 0xEAFFFFF9

# nop out hmac memcmp
patch_hmac_check:
    mov r0, #0

# null out references to slcSomething1 and slcSomething2
# (nulling them out is apparently ok; more importantly, i'm not sure what they do and would rather get a crash than unwanted slc-writing)
slcSomething1:
	.word 0x00000000
slcSomething2:
	.word 0x00000000

#syslogOutput_hook:
#   push {r0,lr}
#   bl dump_syslog
#   pop {r0,lr}
#   restore original instruction
#   pop {r4-r8,r10,pc}


.globl fs_patches_table, fs_patches_table_end
fs_patches_table:
#          origin           data                                    size
     .word 0x107BD374,      patch_mdExit,                           4
     .word 0x1070FAE8,      patch_FSRawOpen,                        8
     .word 0x107B96B8,      slcSomething1,                          8
     .word 0x107206F0,      patch_hmac_check,                       4
#     .word 0x107F0B68,      syslogOutput_hook,                      4
fs_patches_table_end:

