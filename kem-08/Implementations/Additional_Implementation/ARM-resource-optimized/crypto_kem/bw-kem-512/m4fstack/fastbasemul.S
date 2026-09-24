#include "macros.i"
.syntax unified
.cpu cortex-m4
.thumb

/******************************************************************************
 * Degree-4 base multiplication for BW-KEM-512.
 *
 * After the (front/back) NTT the coefficient array consists of 128 independent
 * blocks of 4 coefficients, each living in  Z_q[X]/(X^4 - zeta).  Two adjacent
 * blocks share one |zeta|: block 2i uses +zeta, block 2i+1 uses -zeta, exactly
 * like the reference poly_basemul_montgomery().
 *
 * For a = a0+a1X+a2X^2+a3X^3 and b likewise, reduction X^4 -> zeta gives
 *   r0 = a0b0                + zeta(a1b3 + a3b1 + a2b2)
 *   r1 = a0b1 + a1b0         + zeta(a2b3 + a3b2)
 *   r2 = a0b2 + a1b1 + a2b0  + zeta(a3b3)
 *   r3 = a0b3 + a1b2 + a2b1  + a3b0
 *
 * Arithmetic is Plantard (same as the 256-parameter m4fspeed basemul):
 *  - each raw product / sum of products is reduced once with plant_red,
 *  - multiplication by the (32-bit Plantard-encoded) zeta uses smulwt+smlabt,
 *  so every output coefficient carries exactly one uniform Plantard factor -
 *  identical to the 256-bit basemul, hence consumable by the same invntt.
 *
 * Coefficients are packed two-per-word:  A01 = a0 | a1<<16 , A23 = a2 | a3<<16.
 *
 * void basemul_asm_512(int16_t *r, const int16_t *a, const int16_t *b,
 *                      const int32_t zetas[64]);
 ******************************************************************************/

.global basemul_asm_512
.type basemul_asm_512, %function
.align 2
basemul_asm_512:
  push {r4-r11, lr}
  sub sp, sp, #4                 // slot for the loop counter

  rptr    .req r0
  aptr    .req r1
  bptr    .req r2
  zetaptr .req r3
  A01     .req r4
  A23     .req r5
  B01     .req r6
  B23     .req r7
  zeta    .req r8
  t       .req r9
  u       .req r10
  q       .req r11
  qa      .req r12
  qinv    .req r14

  movw qa, #26632                // qa  = 8*q
  movt q, #3329                  // q   in the HIGH half of the register
  movw qinv, #62209
  movt qinv, #27560              // qinv = 0x6ba8f301

  mov t, #64
  str t, [sp]

  // ---- one degree-4 block; \sgn=1 negates the encoded zeta (-zeta blocks) --
  .macro d4block sgn
    ldr A01, [aptr], #4
    ldr A23, [aptr], #4
    ldr B01, [bptr], #4
    ldr B23, [bptr], #4
  .if \sgn
    ldr zeta, [zetaptr], #4
    neg zeta, zeta
  .else
    ldr zeta, [zetaptr]
  .endif

    // r0 = a0b0 + zeta*(a1b3 + a3b1 + a2b2)
    smultt t, A01, B23           // a1*b3
    smlatt t, A23, B01, t        // + a3*b1
    smlabb t, A23, B23, t        // + a2*b2
    plant_red q, qa, qinv, t     // t.hi = (a1b3+a3b1+a2b2) mod q
    smulwt t, zeta, t            // zeta * t.hi
    smlabt t, t, q, qa           // t.hi = zeta*(...) mod q
    smulbb u, A01, B01           // a0*b0
    plant_red q, qa, qinv, u     // u.hi = a0b0 mod q
    asr t, t, #16
    asr u, u, #16
    add t, t, u
    strh t, [rptr], #2           // store r0

    // r1 = (a0b1 + a1b0) + zeta*(a2b3 + a3b2)
    smuadx t, A23, B23           // a2b3 + a3b2
    plant_red q, qa, qinv, t
    smulwt t, zeta, t
    smlabt t, t, q, qa           // t.hi = zeta*(a2b3+a3b2) mod q
    smuadx u, A01, B01           // a0b1 + a1b0
    plant_red q, qa, qinv, u
    asr t, t, #16
    asr u, u, #16
    add t, t, u
    strh t, [rptr], #2           // store r1

    // r2 = (a0b2 + a1b1 + a2b0) + zeta*a3b3
    smulbb t, A01, B23           // a0*b2
    smlatt t, A01, B01, t        // + a1*b1
    smlabb t, A23, B01, t        // + a2*b0
    plant_red q, qa, qinv, t     // t.hi = (a0b2+a1b1+a2b0) mod q
    smultt u, A23, B23           // a3*b3
    plant_red q, qa, qinv, u
    smulwt u, zeta, u
    smlabt u, u, q, qa           // u.hi = zeta*a3b3 mod q
    asr t, t, #16
    asr u, u, #16
    add t, t, u
    strh t, [rptr], #2           // store r2

    // r3 = a0b3 + a1b2 + a2b1 + a3b0
    smuadx t, A01, B23           // a0b3 + a1b2
    smladx t, A23, B01, t        // + a2b1 + a3b0
    plant_red q, qa, qinv, t
    asr t, t, #16
    strh t, [rptr], #2           // store r3
  .endm

  1:
    d4block 0                    // block 2i   : +zeta
    d4block 1                    // block 2i+1 : -zeta

    ldr t, [sp]
    subs t, t, #1
    str t, [sp]
  bne.w 1b

  add sp, sp, #4
  pop {r4-r11, pc}

  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq zetaptr
  .unreq A01
  .unreq A23
  .unreq B01
  .unreq B23
  .unreq zeta
  .unreq t
  .unreq u
  .unreq q
  .unreq qa
  .unreq qinv
.size basemul_asm_512, .-basemul_asm_512


/******************************************************************************
 * Cached-pipeline degree-4 basemul (Stage 1, mirrors bw-kem-256).
 *
 * `a_prime` is built by poly_cache_prime_asm and holds, per degree-4 block k:
 *    aprime_w0 = za1 | za2<<16
 *    aprime_w1 = za3 | 0
 * where za_i = plant(a_i, zeta_block).
 *
 * Per output (using packed words built each iter):
 *   X1 = a0 | za3<<16   (from A01 and aprime_w1)
 *   X2 = za2| za1<<16   (ror aprime_w0,#16)
 *   Y  = za2| za3<<16   (pkhbt X2, aprime_w1, lsl#16)
 *   Z  = a2 | a1<<16    (pkhbt A23, A01)
 *
 *   r0 = smuad (X1 ,B01) + smuad (X2,B23)   (a0b0+za3b1 + za2b2+za1b3)
 *   r1 = smuadx(A01,B01) + smuadx(Y ,B23)   (a0b1+a1b0 + za2b3+za3b2)
 *   r2 = smuad (X1 ,B23) + smuad (Z ,B01)   (a0b2+za3b3 + a2b0+a1b1)
 *   r3 = smuadx(A01,B23) + smuadx(A23,B01)  (a0b3+a1b2 + a2b1+a3b0)
 *
 * 128 blocks; each iter handles 1 block.  ctr in r14, q/qa/qinv only loaded in
 * the _32_16 variant.
 ******************************************************************************/

/* void basemul_asm_opt_16_32(int32_t *r_tmp, const int16_t *a,
 *                            const int16_t *b, const int16_t *a_prime); */
.global basemul_asm_opt_16_32
.type basemul_asm_opt_16_32, %function
.align 2
basemul_asm_opt_16_32:
  push {r4-r11, lr}

  rptr      .req r0
  aptr      .req r1
  bptr      .req r2
  aprimeptr .req r3
  A01       .req r4
  A23       .req r5
  B01       .req r6
  B23       .req r7
  X1        .req r8
  X2        .req r9
  Y         .req r10
  Z         .req r11
  tmp       .req r12
  loop      .req r14

  movw loop, #128

1:
    ldr A01, [aptr], #4
    ldr A23, [aptr], #4
    ldr B01, [bptr], #4
    ldr B23, [bptr], #4

    /* r3 (no cache needed) */
    smuadx tmp, A01, B23
    smladx tmp, A23, B01, tmp
    str tmp, [rptr, #12]

    /* load aprime; temporarily put aw0 in X1 reg and aw1 in Y reg */
    ldr X1, [aprimeptr], #4         /* aw0 = za1|za2<<16 */
    ldr Y,  [aprimeptr], #4         /* aw1 = za3|0       */

    ror   X2, X1, #16               /* X2 = za2|za1<<16  */
    pkhbt X1, A01, Y, lsl#16        /* X1 = a0|za3<<16   (uses aw1 in Y) */
    pkhbt Y,  X2, Y, lsl#16         /* Y  = za2|za3<<16  (consumes aw1)  */
    pkhbt Z,  A23, A01              /* Z  = a2|a1<<16    */

    /* r0 */
    smuad tmp, X1, B01
    smlad tmp, X2, B23, tmp
    str tmp, [rptr], #4

    /* r1 */
    smuadx tmp, A01, B01
    smladx tmp, Y, B23, tmp
    str tmp, [rptr], #4

    /* r2 */
    smuad tmp, X1, B23
    smlad tmp, Z, B01, tmp
    str tmp, [rptr], #4

    /* 3 post-inc stores left rptr at block_start+12; r3 already stored at +12;
     * one more int32 (+4) to reach the next block. */
    add rptr, #4

    subs loop, #1
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq aprimeptr
  .unreq A01
  .unreq A23
  .unreq B01
  .unreq B23
  .unreq X1
  .unreq X2
  .unreq Y
  .unreq Z
  .unreq tmp
  .unreq loop
.size basemul_asm_opt_16_32, .-basemul_asm_opt_16_32


/* void basemul_asm_acc_opt_32_32(int32_t *r_tmp, const int16_t *a,
 *                                const int16_t *b, const int16_t *a_prime); */
.global basemul_asm_acc_opt_32_32
.type basemul_asm_acc_opt_32_32, %function
.align 2
basemul_asm_acc_opt_32_32:
  push {r4-r11, lr}

  rptr      .req r0
  aptr      .req r1
  bptr      .req r2
  aprimeptr .req r3
  A01       .req r4
  A23       .req r5
  B01       .req r6
  B23       .req r7
  X1        .req r8
  X2        .req r9
  Y         .req r10
  Z         .req r11
  tmp       .req r12
  loop      .req r14

  movw loop, #128

1:
    ldr A01, [aptr], #4
    ldr A23, [aptr], #4
    ldr B01, [bptr], #4
    ldr B23, [bptr], #4

    /* r3 accumulate */
    ldr   tmp, [rptr, #12]
    smladx tmp, A01, B23, tmp
    smladx tmp, A23, B01, tmp
    str tmp, [rptr, #12]

    ldr X1, [aprimeptr], #4
    ldr Y,  [aprimeptr], #4
    ror   X2, X1, #16
    pkhbt X1, A01, Y, lsl#16
    pkhbt Y,  X2, Y, lsl#16
    pkhbt Z,  A23, A01

    /* r0 accumulate */
    ldr   tmp, [rptr, #0]
    smlad tmp, X1, B01, tmp
    smlad tmp, X2, B23, tmp
    str tmp, [rptr], #4

    /* r1 accumulate */
    ldr   tmp, [rptr, #0]
    smladx tmp, A01, B01, tmp
    smladx tmp, Y, B23, tmp
    str tmp, [rptr], #4

    /* r2 accumulate */
    ldr   tmp, [rptr, #0]
    smlad tmp, X1, B23, tmp
    smlad tmp, Z, B01, tmp
    str tmp, [rptr], #4

    add rptr, #4                    /* skip r3 slot already updated */

    subs loop, #1
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq aprimeptr
  .unreq A01
  .unreq A23
  .unreq B01
  .unreq B23
  .unreq X1
  .unreq X2
  .unreq Y
  .unreq Z
  .unreq tmp
  .unreq loop
.size basemul_asm_acc_opt_32_32, .-basemul_asm_acc_opt_32_32


/* void basemul_asm_acc_opt_32_16(int16_t *r, const int16_t *a,
 *                                const int16_t *b, const int16_t *a_prime,
 *                                const int32_t *r_tmp);
 *
 * Final stage: accumulate the last poly, plant_red each of the 4 int32 sums to
 * int16, pack pairs into two int32 words per block and store to `r`.
 *
 * Register pressure is tight - the iter counter and r_tmp pointer are stashed
 * in FPU scratch regs (s0,s1) like bw-kem-256's poly_asm.S does.
 */
.global basemul_asm_acc_opt_32_16
.type basemul_asm_acc_opt_32_16, %function
.align 2
basemul_asm_acc_opt_32_16:
  push {r4-r11, lr}

  rptr      .req r0
  aptr      .req r1
  bptr      .req r2
  aprimeptr .req r3
  A01       .req r4
  A23       .req r5
  B01       .req r6
  B23       .req r7
  X1        .req r8
  X2        .req r9
  Y         .req r10
  Z         .req r11
  tmp       .req r12
  q_qa_qinv_loop_rtmp_via_fpu .req r14   /* unused name */

  /* layout: r_tmp = [sp,#9*4] (one stack arg above pushed r4-r11,lr=9 words) */
  ldr   r14, [sp, #9*4]
  vmov  s0, r14                          /* s0 := r_tmp ptr */
  mov   r14, #128
  vmov  s1, r14                          /* s1 := loop counter */

  movw  r14, #26632                      /* qa */
  vmov  s2, r14                          /* s2 := qa */
  movw  r14, #62209
  movt  r14, #27560                      /* qinv = 0x6ba8f301 */
  vmov  s3, r14                          /* s3 := qinv */
  movw  r14, #0
  movt  r14, #3329                       /* q in HIGH half */
  vmov  s4, r14                          /* s4 := q */

  /* plant_red(value in tmp), using q,qa,qinv from FPU stash; needs scratch */
  .macro pr_inline a, scr
    vmov \scr, s3                        /* qinv */
    mul  \a, \a, \scr
    vmov \scr, s4                        /* q */
    vmov r14, s2                         /* qa */
    smlatt \a, \a, \scr, r14             /* a[31:16] = plant_red value */
  .endm

1:
    ldr A01, [aptr], #4
    ldr A23, [aptr], #4
    ldr B01, [bptr], #4
    ldr B23, [bptr], #4

    vmov r14, s0                         /* r_tmp ptr */

    /* compute r3 acc */
    ldr   tmp, [r14, #12]
    smladx tmp, A01, B23, tmp
    smladx tmp, A23, B01, tmp
    pr_inline tmp, X1                    /* X1 used as scratch (will rebuild) */
    /* keep r3 (in tmp[31:16]) - save to FPU temporarily, free tmp */
    vmov s5, tmp

    ldr X1, [aprimeptr], #4
    ldr Y,  [aprimeptr], #4
    ror   X2, X1, #16
    pkhbt X1, A01, Y, lsl#16
    pkhbt Y,  X2, Y, lsl#16
    pkhbt Z,  A23, A01

    /* r0 */
    vmov r14, s0
    ldr   tmp, [r14, #0]
    smlad tmp, X1, B01, tmp
    smlad tmp, X2, B23, tmp
    pr_inline tmp, X2                    /* X2 free after this output */
    /* tmp[31:16] = r0; combine with r1 next */
    vmov s6, tmp                         /* stash r0 word */

    /* r1 */
    vmov r14, s0
    ldr   tmp, [r14, #4]
    smladx tmp, A01, B01, tmp
    smladx tmp, Y, B23, tmp
    pr_inline tmp, Y
    /* pack r0|r1 into one word: r0 in low, r1 in high (matches little-endian
     * int16[0]=r0, int16[1]=r1).  s6 holds r0 in [31:16]; tmp holds r1 in
     * [31:16].  Want word = r0|r1<<16 -> low16 = r0 = s6>>16, high16 = r1. */
    vmov X2, s6
    pkhtb tmp, tmp, X2, asr#16           /* tmp = r1<<16 | r0 */
    str tmp, [rptr], #4

    /* r2 */
    vmov r14, s0
    ldr   tmp, [r14, #8]
    smlad tmp, X1, B23, tmp
    smlad tmp, Z, B01, tmp
    pr_inline tmp, X1
    /* combine with stashed r3 (s5) into word = r2|r3<<16 */
    vmov X1, s5
    pkhtb tmp, X1, tmp, asr#16           /* tmp = r3<<16 | r2 */
    str tmp, [rptr], #4

    /* advance r_tmp ptr by 16 bytes (4 int32) */
    vmov r14, s0
    add  r14, #16
    vmov s0, r14

    /* loop counter */
    vmov r14, s1
    subs r14, #1
    vmov s1, r14
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq aprimeptr
  .unreq A01
  .unreq A23
  .unreq B01
  .unreq B23
  .unreq X1
  .unreq X2
  .unreq Y
  .unreq Z
  .unreq tmp
.size basemul_asm_acc_opt_32_16, .-basemul_asm_acc_opt_32_16
