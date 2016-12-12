.section ".init"
.arm
.align 4

.extern _startMainThread
.type _startMainThread, %function

mcpMainThread_hook:
    mov r11, r0
    push {r0-r11,lr}

    bl _startMainThread

    pop {r0-r11,pc}
