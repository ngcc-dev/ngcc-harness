/*
 * DKE-512 (Q=7681, N=512) matacc macros for Cortex-M4 Plantard.
 * 13-bit rejection sampling + fused basemul.
 *
 * Key difference from DKE-128 (12-bit):
 *   - load_vals: 2 bytes → 1 × 13-bit candidate (simple, no alignment issue)
 *   - Loop count: N/4 = 128 (vs 64)
 *   - Constants: Q=7681, qa=30724
 *   - XOF block: same SM3/SHAKE squeeze
 */

.macro plant_red_b512 q, qa, qinv, tmp
    mul \tmp, \tmp, \qinv
    smlatb \tmp, \tmp, \q, \qa
.endm

/* Load 1 × 13-bit candidate from XOF buffer.
 * DKE-512 rejection sampling: take 13 bits from 2 bytes, reject if >= Q.
 * Matches the scalar rej_uniform's 2-byte tail path. */
.macro load_val512 val0, bufptr, tmp
    ldrh \val0, [\bufptr], #2
    ubfx \val0, \val0, #0, #13
.endm

/* Rejection check + basemul for single candidate */
.macro single_if512 func, tmp, tmp2, val0, rptr, bptr, cptr, bufptr, zetaptr, k, q, qa, qinv, ctr
    cmp.w \val0, \q
    bhs.w 2f
        strh \val0, [\cptr], #2
        add \k, #1
        cmp.w \k, #4
        bne.w 2f
            sub \cptr, #4*2
            vmov s18, \bufptr
            vmov s19, \ctr
            \func \rptr, \bptr, \cptr, \zetaptr, \bufptr, \k, \val0, \tmp2, \q, \qa, \qinv, \tmp, r14, \ctr
            vmov \bufptr, s18
            vmov \ctr, s19
            add \ctr, #1
            movw \k, #0
    2:
.endm

/* XOF squeeze refill when buffer exhausted */
.macro squeeze_if512 tmp, tmp2, rptr, bptr, cptr, bufptr, ctr
    vmov \tmp, s17
#ifdef USE_KECCAK
    add \tmp, #168
#else
    add \tmp, #192
#endif
    add \tmp2, \bufptr, #2     // need 2 bytes per candidate
    cmp.w \tmp2, \tmp
    ble.w 2f
        cmp.w \ctr, #512/4
        bge.w 2f
            vmov \bufptr, s17
            vmov s16, r12
            vmov s18, \rptr
            vmov s19, \bptr
            vmov s20, \cptr
            vmov s21, \ctr
            mov \rptr, \bufptr
            movw \bptr, #1
            vmov \cptr, s26
            bl dke3_xof_squeezeblocks
            vmov r12, s16
            vmov \rptr, s18
            vmov \bptr, s19
            vmov \cptr, s20
            vmov \ctr, s21
            vmov \bufptr, s17
    2:
.endm

/* Basemul macros: same as DKE-128 but with Q=7681 Plantard constants */
.macro doublebasemul_asm_cache512_16_32 rptr_tmp, aptr, bptr, zetaptr, poly0, poly1, tmp, tmp2, q, qa, qinv, res, aprimeptr, zeta
    vmov \aprimeptr, s27
    ldr \poly0, [\aptr], #4
    ldr \poly1, [\bptr]
    ldr \zeta, [\zetaptr], #4

    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    str \tmp, [\aprimeptr], #4
    smultt \tmp2, \tmp, \poly1
    smlabb \tmp2, \poly0, \poly1, \tmp2
    smuadx \tmp, \poly0, \poly1
    str.w \tmp, [\rptr_tmp, #4]
    str \tmp2, [\rptr_tmp], #8

    neg \zeta, \zeta
    ldr \poly0, [\aptr], #4
    ldr.w \poly1, [\bptr, #4]
    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    str \tmp, [\aprimeptr], #4
    smultt \tmp2, \tmp, \poly1
    smlabb \tmp2, \poly0, \poly1, \tmp2
    smuadx \tmp, \poly0, \poly1
    str.w \tmp, [\rptr_tmp, #4]
    str \tmp2, [\rptr_tmp], #8
    vmov s27, \aprimeptr
.endm

.macro doublebasemul_asm_acc_cache512_32_32 rptr_tmp, aptr, bptr, zetaptr, poly0, poly1, tmp, tmp2, q, qa, qinv, res, aprimeptr, zeta
    vmov \aprimeptr, s27
    ldr \poly0, [\aptr], #4
    ldr \poly1, [\bptr]
    ldr \res, [\rptr_tmp]
    ldr \zeta, [\zetaptr], #4

    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    str \tmp, [\aprimeptr], #4
    smlatt \tmp, \tmp, \poly1, \res
    smlabb \res, \poly0, \poly1, \tmp
    str \res, [\rptr_tmp], #4
    ldr.w \res, [\rptr_tmp]
    smladx \res, \poly0, \poly1, \res
    str.w \res, [\rptr_tmp], #4

    neg \zeta, \zeta
    ldr \poly0, [\aptr], #4
    ldr.w \poly1, [\bptr, #4]
    ldr \res, [\rptr_tmp]
    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    str \tmp, [\aprimeptr], #4
    smlatt \tmp, \tmp, \poly1, \res
    smlabb \res, \poly0, \poly1, \tmp
    str.w \res, [\rptr_tmp], #4
    ldr.w \res, [\rptr_tmp]
    smladx \res, \poly0, \poly1, \res
    str \res, [\rptr_tmp], #4
    vmov s27, \aprimeptr
.endm

.macro doublebasemul_asm_acc_cache512_32_16 rptr_tmp, aptr, bptr, zetaptr, poly0, poly1, tmp, tmp2, q, qa, qinv, res, aprimeptr, zeta
    vmov \aprimeptr, s27
    ldr \poly0, [\aptr], #4
    ldr \poly1, [\bptr]
    ldr \res, [\rptr_tmp], #4
    ldr \zeta, [\zetaptr], #4

    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    str \tmp, [\aprimeptr], #4
    smlatt \tmp, \tmp, \poly1, \res
    smlabb \tmp2, \poly0, \poly1, \tmp
    plant_red_b512 \q, \qa, \qinv, \tmp2
    ldr.w \tmp, [\rptr_tmp], #4
    smladx \tmp, \poly0, \poly1, \tmp
    plant_red_b512 \q, \qa, \qinv, \tmp
    pkhtb \res, \tmp, \tmp2, asr#16
    vmov \tmp2, s28
    str \res, [\tmp2], #4

    neg \zeta, \zeta
    ldr \poly0, [\aptr], #4
    ldr.w \poly1, [\bptr, #4]
    smulwt \tmp, \zeta, \poly0
    smlabb \tmp, \tmp, \q, \qa
    pkhbt \tmp, \poly0, \tmp
    ldr \res, [\rptr_tmp], #4
    str \tmp, [\aprimeptr], #4
    smlatt \tmp, \tmp, \poly1, \res
    smlabb \tmp, \poly0, \poly1, \tmp
    plant_red_b512 \q, \qa, \qinv, \tmp
    ldr \res, [\rptr_tmp], #4
    smladx \res, \poly0, \poly1, \res
    plant_red_b512 \q, \qa, \qinv, \res
    pkhtb \res, \res, \tmp, asr#16
    str \res, [\tmp2], #4
    vmov s28, \tmp2
    vmov s27, \aprimeptr
.endm
