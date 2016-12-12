.section ".text"
.arm
.align 4

.global svcAlloc
.type svcAlloc, %function
svcAlloc:
	.word 0xE7F027F0
	bx lr

.global svcAllocAlign
.type svcAllocAlign, %function
svcAllocAlign:
	.word 0xE7F028F0
	bx lr

.global svcFree
.type svcFree, %function
svcFree:
	.word 0xE7F029F0
	bx lr

.global svcOpen
.type svcOpen, %function
svcOpen:
	.word 0xE7F033F0
	bx lr

.global svcClose
.type svcClose, %function
svcClose:
	.word 0xE7F034F0
	bx lr

.global svcIoctl
.type svcIoctl, %function
svcIoctl:
	.word 0xE7F038F0
	bx lr

.global svcIoctlv
.type svcIoctlv, %function
svcIoctlv:
	.word 0xE7F039F0
	bx lr
