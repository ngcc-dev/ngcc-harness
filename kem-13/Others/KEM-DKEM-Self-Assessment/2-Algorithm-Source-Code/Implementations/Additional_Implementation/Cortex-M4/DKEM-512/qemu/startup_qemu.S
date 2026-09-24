.syntax unified
.thumb
.cpu cortex-m4
.section .isr_vector, "a"
.word 0x20020000
.word _reset + 1
.word _hang + 1
.word _hang + 1
.word _hang + 1
.word _hang + 1
.word _hang + 1
.word 0, 0, 0, 0
.word _hang + 1
.word 0, 0
.word _hang + 1
.word _hang + 1

.section .text
.thumb_func
.global _reset
_reset:
    ldr sp, =0x20020000
    movw r0, #0xED88
    movt r0, #0xE000
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb
    ldr r0, =__bss_start__
    ldr r1, =__bss_end__
    movs r2, #0
1:  cmp r0, r1
    bge 2f
    str r2, [r0], #4
    b 1b
2:  bl main
    mov r0, #0x18
    ldr r1, =0x20026
    bkpt #0xAB
    b .

.thumb_func
.global _hang
_hang:
    nop
    b _hang
