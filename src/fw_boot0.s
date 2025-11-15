

.syntax unified
.cpu cortex-m0plus
.fpu softvfp
.thumb

.global fw_boot0

.section .text.fw_boot0
.type fw_boot0, %function

fw_boot0:
    ldr r1, [r0]
    msr msp, r1
    ldr r0, [r0, #4]
    bx r0
    b .
