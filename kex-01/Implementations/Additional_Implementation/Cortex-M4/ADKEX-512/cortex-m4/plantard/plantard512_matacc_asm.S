#if (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
/*
 * DKE-512 (Q=7681, N=512) matrix accumulation ASM for Cortex-M4 Plantard.
 * Fuses XOF squeeze + 13-bit rejection sampling + Plantard basemul.
 *
 * Adapted from DKE-128 matacc_asm.S:
 *   - 13-bit rejection (2 bytes → 1 candidate) instead of 12-bit (3 → 2)
 *   - Loop count N/4 = 128 instead of 64
 *   - Q=7681, qa=30724, qinv for Q=7681 Plantard
 *
 * Author: Adapted for DKE-512 from Junhao Huang's DKE-128 (Apache 2.0).
 */
#include "matacc512.i"
.extern dke3_xof_squeezeblocks

.syntax unified
.cpu cortex-m4
.thumb

// void matacc_asm_cache512_16_32(int32_t *r_tmp, const int16_t *b, int16_t c[4],
//     unsigned char buf[XOF_BLOCKBYTES+2], const int32_t zetas[128],
//     xof_state *state, int16_t *aprimeptr)
.global matacc_asm_cache512_16_32
.type matacc_asm_cache512_16_32, %function
.align 2
matacc_asm_cache512_16_32:
    push {r0-r11, r14}
    rptr    .req r0
    bptr    .req r1
    cptr    .req r2
    bufptr  .req r3
    zetaptr .req r4
    val0    .req r5
    val1    .req r6
    tmp     .req r7
    tmp2    .req r8
    k       .req r9
    q       .req r10
    qa      .req r11
    qinv    .req r12
    ctr     .req r14

    movw qa, #30724          // 4 * 7681
    movw q, #7681
    // qinv for Q=7681 Plantard: q^{-1} mod 2^32
    movw qinv, #57857        // 0x8B83E201 low
    movt qinv, #35715        // 0x8B83E201 high = 2340676097
    movw k, #0

    ldr.w zetaptr, [sp, #13*4]
    ldr.w tmp, [sp, #14*4]
    vmov s26, tmp
    ldr.w tmp, [sp, #15*4]
    vmov s27, tmp

    movw ctr, #0
    vmov s17, bufptr
1:
    load_val512 val0, bufptr, tmp

    single_if512 doublebasemul_asm_cache512_16_32, tmp, tmp2, val0, rptr, bptr, cptr, bufptr, zetaptr, k, q, qa, qinv, ctr

    squeeze_if512 tmp, tmp2, rptr, bptr, cptr, bufptr, ctr

    cmp ctr, #512/4
    blt.w 1b

    pop {r0-r11, pc}
.size matacc_asm_cache512_16_32, . - matacc_asm_cache512_16_32

// void matacc_asm_cache512_32_32(...)
.global matacc_asm_cache512_32_32
.type matacc_asm_cache512_32_32, %function
.align 2
matacc_asm_cache512_32_32:
    push {r0-r11, r14}

    movw qa, #30724
    movw q, #7681
    movw qinv, #57857
    movt qinv, #35715
    movw k, #0

    ldr.w zetaptr, [sp, #13*4]
    ldr.w tmp, [sp, #14*4]
    vmov s26, tmp
    ldr.w tmp, [sp, #15*4]
    vmov s27, tmp

    movw ctr, #0
    vmov s17, bufptr
1:
    load_val512 val0, bufptr, tmp

    single_if512 doublebasemul_asm_acc_cache512_32_32, tmp, tmp2, val0, rptr, bptr, cptr, bufptr, zetaptr, k, q, qa, qinv, ctr

    squeeze_if512 tmp, tmp2, rptr, bptr, cptr, bufptr, ctr

    cmp ctr, #512/4
    blt.w 1b

    pop {r0-r11, pc}
.size matacc_asm_cache512_32_32, . - matacc_asm_cache512_32_32

// void matacc_asm_cache512_32_16(...)
.global matacc_asm_cache512_32_16
.type matacc_asm_cache512_32_16, %function
.align 2
matacc_asm_cache512_32_16:
    push {r0-r11, r14}

    movw qa, #30724
    movw q, #7681
    movw qinv, #57857
    movt qinv, #35715
    movw k, #0

    ldr.w zetaptr, [sp, #13*4]
    ldr.w tmp, [sp, #14*4]
    vmov s26, tmp
    ldr.w tmp, [sp, #15*4]
    vmov s27, tmp

    vmov s28, rptr
    vmov s29, rptr
    ldr.w rptr, [sp, #16*4]

    movw ctr, #0
    vmov s17, bufptr
1:
    load_val512 val0, bufptr, tmp

    single_if512 doublebasemul_asm_acc_cache512_32_16, tmp, tmp2, val0, rptr, bptr, cptr, bufptr, zetaptr, k, q, qa, qinv, ctr

    squeeze_if512 tmp, tmp2, rptr, bptr, cptr, bufptr, ctr

    cmp ctr, #512/4
    blt.w 1b

    vmov rptr, s29
    pop {r0-r11, pc}
.size matacc_asm_cache512_32_16, . - matacc_asm_cache512_32_16

.unreq rptr
.unreq bptr
.unreq cptr
.unreq bufptr
.unreq zetaptr
.unreq val0
.unreq val1
.unreq tmp
.unreq tmp2
.unreq k
.unreq q
.unreq qa
.unreq qinv
.unreq ctr

#endif /* (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
