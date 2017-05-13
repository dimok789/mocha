
#############################################################################################
# FS main thread hook
#############################################################################################
.extern createDevThread_entry
    .globl createDevThread_hook
createDevThread_hook:
    push {r0,r1,lr}
    ldr r0, [r4, #0x8]
    mov r1, r7
    bl createDevThread_entry
    pop {r0,r1,lr}
#   restore original instruction
    pop {r4-r8,pc}

#############################################################################################
# devices handle hooks
#############################################################################################
.extern getMdDeviceById
    .globl getMdDeviceById_hook
getMdDeviceById_hook:
    mov r4, r0
    push {lr}
    bl getMdDeviceById
    pop {lr}
    cmp r0, #0
    moveq r0, r4
    bxeq lr
    pop {r4,r5,pc}


#############################################################################################
# syslog hook
#############################################################################################
    .globl syslogOutput_hook
syslogOutput_hook:
#   push {r0,lr}
#   bl dump_syslog
#   pop {r0,lr}
#   restore original instruction
    pop {r4-r8,r10,pc}

#############################################################################################
# Original NAND read functions
#############################################################################################
    .globl slcRead1_original
slcRead1_original:
    push {r4-r8,lr}
    ldr r4, [pc]
    bx r4
    .word 0x107B9990

    .globl sdcardRead_original
sdcardRead_original:
    push {r4,lr}
    ldr r4, [pc]
    bx r4
    .word 0x107BDDD4

#############################################################################################
# FSA functions
#############################################################################################
    .globl FSA_MakeQuota_asm_hook
FSA_MakeQuota_asm_hook:
    mov r1, r5
    b FSA_MakeQuota_hook

#############################################################################################
# DEBUG STUFF
#############################################################################################
# # # # # # # # # #
#   DEBUG STUFF   #
# # # # # # # # # #
#mlcRead1_dbg:
#	mlcRead1_dbg_stackframe equ (4*6)
#	mov r12, r0
#	push {r0-r3,r12,lr}
#	adr r0, mlcRead1_dbg_format
#	ldr r1, [sp, #mlcRead1_dbg_stackframe+9*4]
#	bl FS_SYSLOG_OUTPUT
#	pop {r0-r3,lr,pc} # replaces mov lr, r0
#	mlcRead1_dbg_format:
#		.ascii "mlcRead1 : %08X %08X %08X"
#		.byte 0x0a
#		.byte 0x00
#		.align 0x4
#
#mlcRead1_end_hook:
#	mlcRead1_end_hook_stackframe equ (4*10)
#	push {r0}
#	mov r0, #50
#	bl FS_SLEEP
#	ldr r0, =sdcard_read_buffer
#	ldr r1, [sp, #mlcRead1_end_hook_stackframe+4*1]
#	mov r2, #0x200
#	bl FS_MEMCPY
#	ldr r0, =sdcard_read_buffer
#	str r6, [r0]
#	mov r1, #0x200
#	bl dump_data
#	pop {r0,r4-r11,pc}
