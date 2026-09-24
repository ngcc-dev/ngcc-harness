/*
 * Hand-written x86-64 AVX2 NTT / polynomial assembly for the DKE KEM.
 * Author: Liu Rui (刘锐), proposal-team member.
 * Original contribution of this submission; WITHIN the scope of the B.3
 * reference-and-optimized-implementation ownership statement.
 */
/*
 * AVX2 INVNTT for N=512, Q=7681 (DKE-512).
 * Layers 5+4+3 fused in-register per 8-block group.
 * Layers 2+1 load/compute/store.
 * Final scaling in AVX2.
 *
 * void dke_invntt512_avx2_asm(int16_t r[512], const int16_t zetas[256],
 *                              int16_t f, unsigned int k_start);
 * rdi=r, rsi=zetas, dx=f, ecx=k_start
 *
 * INVNTT zeta order (k decreasing):
 *   Layer 5 (len=16): zetas[k]..zetas[k-15], 16 groups
 *   Layer 4 (len=32): zetas[k-16]..zetas[k-23], 8 groups
 *   Layer 3 (len=64): zetas[k-24]..zetas[k-27], 4 groups
 *   Layer 2 (len=128): zetas[k-28]..zetas[k-29], 2 groups
 *   Layer 1 (len=256): zetas[k-30], 1 group
 *
 * For fused layers 5+4+3 in group G (G=0..3, blocks G*8..G*8+7):
 *   Layer 5 zetas (from k_start counting down):
 *     Group G uses 4 zetas for pairs (0,1)(2,3)(4,5)(6,7)
 *     These are at global positions: k_start - G*4 - 0..3
 *   Layer 4 zetas: k_start - 16 - G*2 - 0..1
 *   Layer 3 zeta:  k_start - 24 - G
 *
 * Actually, the C code processes start=0,32,...,480 for len=16,
 * consuming zetas[k], zetas[k-1], ... in order.
 * Group 0 (start=0, blocks 0-1) uses zetas[k_start].
 * Group 1 (start=32, blocks 2-3) uses zetas[k_start-1].
 * ...
 * For fused processing of blocks 0-7 (groups 0-3 of layer 5):
 *   zetas[k_start], zetas[k_start-1], zetas[k_start-2], zetas[k_start-3]
 * For blocks 8-15 (groups 4-7 of layer 5):
 *   zetas[k_start-4], ..., zetas[k_start-7]
 * etc.
 *
 * Layer 4 (len=32) starts at k_start - 16:
 *   Group 0 (blocks 0-3): zetas[k_start-16]
 *   Group 1 (blocks 4-7): zetas[k_start-17]
 *   ...
 *
 * Layer 3 (len=64) starts at k_start - 24:
 *   Group 0 (blocks 0-7): zetas[k_start-24]
 *   Group 1 (blocks 8-15): zetas[k_start-25]
 *   ...
 */

.text

.macro GS r0, r1
    vpaddw  \r1, \r0, %ymm9
    vpmulhw %ymm13, %ymm9, %ymm10
    vpaddw  %ymm12, %ymm10, %ymm10
    vpsraw  $10, %ymm10, %ymm10
    vpmullw %ymm14, %ymm10, %ymm10
    vpsubw  %ymm10, %ymm9, %ymm9
    vpsubw  \r0, \r1, %ymm10
    vpmullw %ymm10, %ymm8, %ymm11
    vpmulhw %ymm10, %ymm8, %ymm10
    vpmullw %ymm15, %ymm11, %ymm11
    vpmulhw %ymm14, %ymm11, %ymm11
    vpsubw  %ymm11, %ymm10, \r1
    vmovdqa %ymm9, \r0
.endm

/* Load zeta by absolute index */
.macro LZI zr, idx
    movswl  (\idx * 2)(\zr), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
.endm

/* Load zeta sequentially (decrementing) */
.macro LZD zr, kr
    movswl  (\zr, \kr, 2), %eax
    dec     \kr
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
.endm

.macro FQMUL_S a, b, lo, hi, tmp
    vpmullw \b, \a, \lo
    vpmulhw \b, \a, \hi
    vpmullw %ymm15, \lo, \tmp
    vpmulhw %ymm14, \tmp, \tmp
    vpsubw  \tmp, \hi, \hi
.endm

.globl dke_invntt512_avx2_asm
.type dke_invntt512_avx2_asm, @function
.align 32
dke_invntt512_avx2_asm:
    push    %rbx
    push    %r12
    push    %r13
    push    %r14

    mov     %rdi, %r12          /* r */
    mov     %rsi, %r13          /* zetas */
    movswl  %dx, %r14d          /* f */
    mov     %ecx, %ebx          /* k = k_start */

    mov     $7681, %eax
    vmovd   %eax, %xmm14
    vpbroadcastw %xmm14, %ymm14
    mov     $-7679, %eax
    vmovd   %eax, %xmm15
    vpbroadcastw %xmm15, %ymm15
    mov     $8737, %eax
    vmovd   %eax, %xmm13
    vpbroadcastw %xmm13, %ymm13
    mov     $512, %eax
    vmovd   %eax, %xmm12
    vpbroadcastw %xmm12, %ymm12

    /* ===== Layers 5+4+3 FUSED: 4 groups of 8 blocks =====
     * k_start = ebx. Layer 5 uses zetas[k_start .. k_start-15].
     * Layer 4 uses zetas[k_start-16 .. k_start-23].
     * Layer 3 uses zetas[k_start-24 .. k_start-27].
     *
     * For group G (blocks G*8..G*8+7):
     *   Layer 5: 4 zetas at indices k_start - G*4 - {0,1,2,3}
     *   Layer 4: 2 zetas at indices k_start - 16 - G*2 - {0,1}
     *   Layer 3: 1 zeta  at index  k_start - 24 - G
     */
    .irp G, 0,1,2,3
    .set BASE, \G * 8

    /* Load 8 blocks */
    vmovdqa ((BASE + 0) * 32)(%r12), %ymm0
    vmovdqa ((BASE + 1) * 32)(%r12), %ymm1
    vmovdqa ((BASE + 2) * 32)(%r12), %ymm2
    vmovdqa ((BASE + 3) * 32)(%r12), %ymm3
    vmovdqa ((BASE + 4) * 32)(%r12), %ymm4
    vmovdqa ((BASE + 5) * 32)(%r12), %ymm5
    vmovdqa ((BASE + 6) * 32)(%r12), %ymm6
    vmovdqa ((BASE + 7) * 32)(%r12), %ymm7

    /* Layer 5 (len=16): pairs (0,1)(2,3)(4,5)(6,7), 4 zetas */
    /* zeta indices: k_start - G*4 - 0, -1, -2, -3 */
    .set Z5BASE, -(\G * 4)
    movswl  ((Z5BASE + 0) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm0, %ymm1

    movswl  ((Z5BASE - 1) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm2, %ymm3

    movswl  ((Z5BASE - 2) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm4, %ymm5

    movswl  ((Z5BASE - 3) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm6, %ymm7

    /* Layer 4 (len=32): pairs (0,2)(1,3) and (4,6)(5,7), 2 zetas */
    .set Z4BASE, -(16 + \G * 2)
    movswl  ((Z4BASE + 0) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm0, %ymm2
    GS %ymm1, %ymm3

    movswl  ((Z4BASE - 1) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm4, %ymm6
    GS %ymm5, %ymm7

    /* Layer 3 (len=64): pairs (0,4)(1,5)(2,6)(3,7), 1 zeta */
    .set Z3IDX, -(24 + \G)
    movswl  ((Z3IDX) * 2)(%r13, %rbx, 2), %eax
    vmovd   %eax, %xmm8
    vpbroadcastw %xmm8, %ymm8
    GS %ymm0, %ymm4
    GS %ymm1, %ymm5
    GS %ymm2, %ymm6
    GS %ymm3, %ymm7

    /* Store 8 blocks */
    vmovdqa %ymm0, ((BASE + 0) * 32)(%r12)
    vmovdqa %ymm1, ((BASE + 1) * 32)(%r12)
    vmovdqa %ymm2, ((BASE + 2) * 32)(%r12)
    vmovdqa %ymm3, ((BASE + 3) * 32)(%r12)
    vmovdqa %ymm4, ((BASE + 4) * 32)(%r12)
    vmovdqa %ymm5, ((BASE + 5) * 32)(%r12)
    vmovdqa %ymm6, ((BASE + 6) * 32)(%r12)
    vmovdqa %ymm7, ((BASE + 7) * 32)(%r12)
    .endr

    /* Advance k past layers 5+4+3: consumed 16+8+4 = 28 zetas */
    sub     $28, %ebx

    /* ===== Layer 2 (len=128): 2 zetas ===== */
    LZD %r13, %rbx
    .irp I, 0,1,2,3,4,5,6,7
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    GS %ymm0, %ymm1
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    .endr
    LZD %r13, %rbx
    .irp I, 16,17,18,19,20,21,22,23
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 8) * 32)(%r12), %ymm1
    GS %ymm0, %ymm1
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 8) * 32)(%r12)
    .endr

    /* ===== Layer 1 (len=256): 1 zeta ===== */
    LZD %r13, %rbx
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
    vmovdqa (\I * 32)(%r12), %ymm0
    vmovdqa ((\I + 16) * 32)(%r12), %ymm1
    GS %ymm0, %ymm1
    vmovdqa %ymm0, (\I * 32)(%r12)
    vmovdqa %ymm1, ((\I + 16) * 32)(%r12)
    .endr

    /* ===== Final scaling ===== */
    vmovd   %r14d, %xmm8
    vpbroadcastw %xmm8, %ymm8
    .irp I, 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    vmovdqa (\I * 32)(%r12), %ymm0
    FQMUL_S %ymm0, %ymm8, %ymm9, %ymm10, %ymm11
    vmovdqa %ymm10, (\I * 32)(%r12)
    .endr

    vzeroupper
    pop     %r14
    pop     %r13
    pop     %r12
    pop     %rbx
    ret
.size dke_invntt512_avx2_asm, .-dke_invntt512_avx2_asm

.section .note.GNU-stack,"",@progbits
