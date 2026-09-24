.intel_syntax noprefix

#include "ntt_avx_macros.inc"

.macro pointwise_raw f, g
    vmovshdup ymm6, ymm\f
    vmovshdup ymm7, ymm\g
    vpmuldq ymm8, ymm\f, ymm\g
    vpmuldq ymm9, ymm6, ymm7
.endm

.macro montreduce_acc out, acc_e, acc_o
    vpmuldq ymm10, ymm\acc_e, ymm1
    vpmuldq ymm11, ymm\acc_o, ymm1
    vpmuldq ymm10, ymm10, ymm0
    vpmuldq ymm11, ymm11, ymm0
    vpaddq ymm\acc_e, ymm\acc_e, ymm10
    vpaddq ymm\acc_o, ymm\acc_o, ymm11
    vmovshdup ymm\acc_e, ymm\acc_e
    vpblendd ymm\out, ymm\acc_e, ymm\acc_o, 0xaa
.endm

.text
.globl ring_mul_Awin_row_avx
.type ring_mul_Awin_row_avx, @function
ring_mul_Awin_row_avx:
    vmovd xmm0, r8d
    vpbroadcastd ymm0, xmm0
    vmovd xmm1, r9d
    vpbroadcastd ymm1, xmm1

    xor eax, eax
.Lrow_block_loop:
    vpxor ymm2, ymm2, ymm2
    vpxor ymm3, ymm3, ymm3
    mov r10, rsi
    add r10, rax
    mov r11, rdx
    add r11, rax
    mov r8d, ecx

.Lrow_acc_loop:
    vmovdqu ymm4, YMMWORD PTR [r10]
    vmovdqu ymm5, YMMWORD PTR [r11]
    pointwise_raw 4, 5
    vpaddq ymm2, ymm2, ymm8
    vpaddq ymm3, ymm3, ymm9
    add r10, 4096
    add r11, 4096
    dec r8d
    jne .Lrow_acc_loop

    montreduce_acc 2, 2, 3
    reduce_low_oct 2, 12, 14
    vmovdqu YMMWORD PTR [rdi + rax], ymm2
    add rax, 32
    cmp rax, 4096
    jb .Lrow_block_loop

    vzeroupper
    ret
.size ring_mul_Awin_row_avx, .-ring_mul_Awin_row_avx

.section .note.GNU-stack,"",@progbits
