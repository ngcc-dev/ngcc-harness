/* Minimal Cortex-M4 startup for QEMU -M mps2-an386 (ample RAM for ML-DSA-87).
 * Code/vectors at 0x00000000 (ZBT SSRAM1); stack/data at 0x20000000 (ZBT SSRAM2/3).
 * Mirrors the netduino startup but with the MPS2 memory map and a large stack top. */
.syntax unified
.thumb
.cpu cortex-m4
.section .isr_vector, "a"
.word 0x20400000          /* initial SP: top of 4MB data SRAM */
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
    ldr sp, =0x20400000
    movw r0, #0xED88
    movt r0, #0xE000
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)   /* CPACR: enable CP10/CP11 (FPU) */
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
