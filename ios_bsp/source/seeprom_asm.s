.section ".text"
.arm

.globl orig_EEPROM_SPI_ReadWord
orig_EEPROM_SPI_ReadWord:
    cmp r0, #0
    ldr r3, [pc]
    bx r3
    .word 0xE600D090
