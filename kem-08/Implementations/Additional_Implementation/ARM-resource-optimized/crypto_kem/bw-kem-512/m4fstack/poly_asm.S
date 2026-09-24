#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

/******************************************************************************
 * Degree-4 frombytes_mul for BW-KEM-512.
 *
 * Fuses the 12-bit unpacking of the packed sk bytes with the basemul against
 * `b` (already in NTT domain), accumulating across the K=4 secret-key polys in
 * a 32-bit r_tmp buffer (deferred reduction).  Same 3-stage pipeline as the
 * 256-parameter version, but each block is degree-4 (4 coefficients).
 *
 * Per block the products r0..r3 are computed exactly like
 * basemul_asm_opt_16_32 (cached pipeline) - the only difference is that the
 * za1, za2, za3 partials are computed *inline* from sk's deserialized
 * coefficients instead of being pre-cached in an a_prime polyvec.
 *
 * 64 outer iterations, 2 blocks per iteration (paired +zeta / -zeta).
 * q, qa, qinv, the loop counter (and r_tmp ptr for _32_16) are stashed in FPU
 * scratch registers to free GP registers (same trick as bw-kem-256 poly_asm.S).
 ******************************************************************************/

/* Deserialize 6 bytes of packed-12 sk at [aptr] into A01=(a0|a1<<16) (in \A01),
 * A23=(a2|a3<<16) (in \A23). Post-increments aptr by 6.  Clobbers \tA,\tB,\tC. */
.macro fm_deser aptr, A01, A23, tA, tB, tC
  ldrb.w \tA, [\aptr, #2]
  ldrh.w \tB, [\aptr, #3]
  ldrb.w \tC, [\aptr, #5]
  ldrh.w \A01, [\aptr], #6
  ubfx.w \A23, \A01, #12, #4
  ubfx.w \A01, \A01, #0, #12
  orr    \A23, \A23, \tA, lsl #4
  orr    \A01, \A01, \A23, lsl #16   /* A01 = a0 | a1<<16 */
  ubfx.w \A23, \tB, #12, #4
  ubfx.w \tA,  \tB, #0, #12
  orr    \A23, \A23, \tC, lsl #4
  orr    \A23, \tA,  \A23, lsl #16   /* A23 = a2 | a3<<16 */
.endm

/* Compute za1, za2, za3 (in the HIGH halves of out regs) from A01,A23,zeta.
 * Uses q,qa for the Plantard finish - loaded from FPU into \qreg,\qareg first.
 * `tmp` is just a name for the second working reg of the plant_mul step.
 */
.macro fm_zas zeta, A01, A23, za1, za2, za3, qreg, qareg
  smulwt \za1, \zeta, \A01
  smlabt \za1, \za1, \qreg, \qareg     /* za1 in [31:16] */
  smulwb \za2, \zeta, \A23
  smlabt \za2, \za2, \qreg, \qareg     /* za2 in [31:16] */
  smulwt \za3, \zeta, \A23
  smlabt \za3, \za3, \qreg, \qareg     /* za3 in [31:16] */
.endm

/* The four-output computation body shared by all three variants.  The variant
 * supplies how to combine each smlad/smladx result with the current r_tmp
 * value (just store / accumulate / accumulate+plant_red+pack-and-store).
 *
 * Inputs after the prologue (registers as named):
 *   A01,A23,B01,B23,zeta loaded; za1,za2,za3 computed; q/qa in qreg/qareg.
 * Scratch: scr1, scr2 (also used for the SMUAD results / packed words).
 */

/* ------------------------ frombytes_mul_asm_16_32 ------------------------ */
/* void frombytes_mul_asm_16_32(int32_t *r_tmp, const int16_t *b,
 *                              const uint8_t *sk, const int32_t *zetas); */
.global frombytes_mul_asm_16_32
.type frombytes_mul_asm_16_32, %function
.align 2
frombytes_mul_asm_16_32:
  push {r4-r11, lr}

  rptr_tmp .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3

  /* FPU stashes: s0=q, s1=qa, s2=ctr */
  movw  r4, #0
  movt  r4, #3329
  vmov  s0, r4               /* q (high half = 3329) */
  movw  r4, #26632
  vmov  s1, r4               /* qa */
  mov   r4, #64
  vmov  s2, r4               /* ctr */

  .macro fm16_block neg
    /* deserialize -> r4=A01, r5=A23 */
    fm_deser aptr, r4, r5, r6, r7, r8
    /* load b */
    ldr  r6, [bptr], #4      /* B01 */
    ldr  r7, [bptr], #4      /* B23 */
    /* load zeta */
  .if \neg
    ldr  r8, [zetaptr], #4
    neg  r8, r8
  .else
    ldr  r8, [zetaptr]
  .endif

    /* r3 (no za required) */
    smuadx r9, r4, r7
    smladx r9, r5, r6, r9
    str    r9, [rptr_tmp, #12]

    /* za1, za2, za3 ; q/qa loaded into r12/r14 (reused across the 3 muls) */
    vmov   r12, s0           /* q */
    vmov   r14, s1           /* qa */
    fm_zas r8, r4, r5, r9, r10, r11, r12, r14

    /* X1 = a0|za3<<16   (pkhbt Rn.lo | Rm.hi<<16) */
    pkhbt  r12, r4, r11
    /* X2 = za2|za1<<16  (pkhtb Rn.hi | (Rm asr 16).lo) */
    pkhtb  r14, r9, r10, asr#16

    /* r0 = smuad(X1,B01) + smuad(X2,B23) */
    smuad  r9, r12, r6
    smlad  r9, r14, r7, r9
    str    r9, [rptr_tmp, #0]

    /* Y = za2|za3<<16 */
    pkhtb  r14, r11, r10, asr#16
    /* r1 = smuadx(A01,B01) + smuadx(Y,B23) */
    smuadx r9, r4, r6
    smladx r9, r14, r7, r9
    str    r9, [rptr_tmp, #4]

    /* Z = a2|a1<<16 */
    pkhbt  r14, r5, r4
    /* r2 = smuad(X1,B23) + smuad(Z,B01) */
    smuad  r9, r12, r7
    smlad  r9, r14, r6, r9
    str    r9, [rptr_tmp, #8]

    add rptr_tmp, #16
  .endm

1:
    fm16_block 0                     /* +zeta */
    fm16_block 1                     /* -zeta, advances zetaptr */

    vmov  r4, s2
    subs  r4, #1
    vmov  s2, r4
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr_tmp
  .unreq bptr
  .unreq aptr
  .unreq zetaptr
.size frombytes_mul_asm_16_32, .-frombytes_mul_asm_16_32


/* ---------------------- frombytes_mul_asm_acc_32_32 ---------------------- */
/* void frombytes_mul_asm_acc_32_32(int32_t *r_tmp, const int16_t *b,
 *                                  const uint8_t *sk, const int32_t *zetas); */
.global frombytes_mul_asm_acc_32_32
.type frombytes_mul_asm_acc_32_32, %function
.align 2
frombytes_mul_asm_acc_32_32:
  push {r4-r11, lr}

  rptr_tmp .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3

  movw  r4, #0
  movt  r4, #3329
  vmov  s0, r4
  movw  r4, #26632
  vmov  s1, r4
  mov   r4, #64
  vmov  s2, r4

  .macro fm32_block neg
    fm_deser aptr, r4, r5, r6, r7, r8
    ldr  r6, [bptr], #4
    ldr  r7, [bptr], #4
  .if \neg
    ldr  r8, [zetaptr], #4
    neg  r8, r8
  .else
    ldr  r8, [zetaptr]
  .endif

    /* r3 accumulate */
    ldr    r9, [rptr_tmp, #12]
    smladx r9, r4, r7, r9
    smladx r9, r5, r6, r9
    str    r9, [rptr_tmp, #12]

    vmov   r12, s0
    vmov   r14, s1
    fm_zas r8, r4, r5, r9, r10, r11, r12, r14

    pkhbt  r12, r4, r11               /* X1 */
    pkhtb  r14, r9, r10, asr#16       /* X2 */

    /* r0 accumulate */
    ldr    r9, [rptr_tmp, #0]
    smlad  r9, r12, r6, r9
    smlad  r9, r14, r7, r9
    str    r9, [rptr_tmp, #0]

    pkhtb  r14, r11, r10, asr#16      /* Y */
    /* r1 accumulate */
    ldr    r9, [rptr_tmp, #4]
    smladx r9, r4, r6, r9
    smladx r9, r14, r7, r9
    str    r9, [rptr_tmp, #4]

    pkhbt  r14, r5, r4                /* Z */
    /* r2 accumulate */
    ldr    r9, [rptr_tmp, #8]
    smlad  r9, r12, r7, r9
    smlad  r9, r14, r6, r9
    str    r9, [rptr_tmp, #8]

    add rptr_tmp, #16
  .endm

1:
    fm32_block 0
    fm32_block 1
    vmov  r4, s2
    subs  r4, #1
    vmov  s2, r4
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr_tmp
  .unreq bptr
  .unreq aptr
  .unreq zetaptr
.size frombytes_mul_asm_acc_32_32, .-frombytes_mul_asm_acc_32_32


/* ---------------------- frombytes_mul_asm_acc_32_16 ---------------------- */
/* Final stage: accumulate, plant_red each int32 sum to int16, pack pairs into
 * two int32 words per block and store to r.
 *
 *   void frombytes_mul_asm_acc_32_16(int16_t *r, const int16_t *b,
 *                                    const uint8_t *sk, const int32_t *zetas,
 *                                    const int32_t *r_tmp);
 *
 * r_tmp is the 5th argument (on stack at [sp,#9*4] after our push).
 *
 * Additional FPU stashes: s3=qinv, s4=r_tmp ptr. */
.global frombytes_mul_asm_acc_32_16
.type frombytes_mul_asm_acc_32_16, %function
.align 2
frombytes_mul_asm_acc_32_16:
  push {r4-r11, lr}

  rptr     .req r0
  bptr     .req r1
  aptr     .req r2
  zetaptr  .req r3

  movw  r4, #0
  movt  r4, #3329
  vmov  s0, r4                       /* q  */
  movw  r4, #26632
  vmov  s1, r4                       /* qa */
  mov   r4, #64
  vmov  s2, r4                       /* ctr */
  movw  r4, #62209
  movt  r4, #27560
  vmov  s3, r4                       /* qinv = 0x6ba8f301 */
  ldr   r4, [sp, #9*4]
  vmov  s4, r4                       /* r_tmp ptr */

  /* plant_red on `\v` (32-bit int) with q from s0, qa from s1, qinv from s3.
   * Caller MUST pass two distinct scratch registers (\scr1 holds qinv then q,
   * \scr2 holds qa); reusing one of them as `\v` (or passing the same reg as
   * both) silently overwrites q with qa and yields 0 mod q. */
  .macro fm16_plant v, scr1, scr2
    vmov  \scr1, s3                  /* qinv */
    mul   \v, \v, \scr1
    vmov  \scr1, s0                  /* q */
    vmov  \scr2, s1                  /* qa */
    smlatt \v, \v, \scr1, \scr2      /* result in v[31:16] */
  .endm

  .macro fmfinal_block neg
    fm_deser aptr, r4, r5, r6, r7, r8
    ldr  r6, [bptr], #4
    ldr  r7, [bptr], #4
  .if \neg
    ldr  r8, [zetaptr], #4
    neg  r8, r8
  .else
    ldr  r8, [zetaptr]
  .endif

    vmov   r14, s4                   /* r_tmp ptr (per-iter; advances at end) */

    /* r3 accumulate + plant_red */
    ldr    r9, [r14, #12]
    smladx r9, r4, r7, r9
    smladx r9, r5, r6, r9
    fm16_plant r9, r12, r10          /* r9[31:16] = r3 (q->r12, qa->r10; both free pre-fm_zas) */
    /* stash r3 to FPU s5 to free a reg later */
    vmov   s5, r9

    /* compute za's; q in r12, qa in r14 will be re-loaded inside plant via vmov */
    vmov   r12, s0                   /* q  */
    vmov   r14, s1                   /* qa */
    fm_zas r8, r4, r5, r9, r10, r11, r12, r14

    pkhbt  r12, r4, r11              /* X1 */
    pkhtb  r14, r9, r10, asr#16      /* X2 */

    /* r0 acc + plant_red */
    vmov   r9, s4
    ldr    r9, [r9, #0]
    smlad  r9, r12, r6, r9
    smlad  r9, r14, r7, r9
    fm16_plant r9, r8, r14           /* r9[31:16] = r0 (zeta no longer needed) */
    vmov   s6, r9                    /* stash r0 word */

    pkhtb  r14, r11, r10, asr#16     /* Y */

    /* r1 acc + plant_red */
    vmov   r9, s4
    ldr    r9, [r9, #4]
    smladx r9, r4, r6, r9
    smladx r9, r14, r7, r9
    fm16_plant r9, r8, r14           /* r9[31:16] = r1 */
    /* combine with r0 (s6) into one word: r0 in low, r1 in high */
    vmov   r10, s6
    pkhtb  r9, r9, r10, asr#16       /* r9 = r1<<16 | r0 */
    str    r9, [rptr], #4

    pkhbt  r14, r5, r4               /* Z */

    /* r2 acc + plant_red */
    vmov   r9, s4
    ldr    r9, [r9, #8]
    smlad  r9, r12, r7, r9
    smlad  r9, r14, r6, r9
    fm16_plant r9, r8, r14           /* r9[31:16] = r2 */
    /* combine with r3 (s5) into word = r2 in low, r3 in high */
    vmov   r10, s5
    pkhtb  r9, r10, r9, asr#16       /* r9 = r3<<16 | r2 */
    str    r9, [rptr], #4

    /* advance r_tmp ptr by 16 */
    vmov  r9, s4
    add   r9, #16
    vmov  s4, r9
  .endm

1:
    fmfinal_block 0
    fmfinal_block 1

    vmov  r4, s2
    subs  r4, #1
    vmov  s2, r4
  bne 1b

  pop {r4-r11, pc}

  .unreq rptr
  .unreq bptr
  .unreq aptr
  .unreq zetaptr
.size frombytes_mul_asm_acc_32_16, .-frombytes_mul_asm_acc_32_16


.macro fm_d4np sgn, acc
  /* deserialize 6 bytes of sk -> A01=r4, A23=r5 (advances r2 by 6) */
  fm_deser r2, r4, r5, r6, r7, r8
  ldr r6, [r1], #4                 /* B01 */
  ldr r7, [r1], #4                 /* B23 */
.if \sgn
  ldr r8, [r3], #4
  neg r8, r8
.else
  ldr r8, [r3]
.endif

  /* r0 = a0b0 + zeta*(a1b3 + a3b1 + a2b2) */
  smultt r9, r4, r7
  smlatt r9, r5, r6, r9
  smlabb r9, r5, r7, r9
  plant_red r11, r12, r14, r9
  smulwt r9, r8, r9
  smlabt r9, r9, r11, r12
  smulbb r10, r4, r6
  plant_red r11, r12, r14, r10
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r1 = (a0b1 + a1b0) + zeta*(a2b3 + a3b2) */
  smuadx r9, r5, r7
  plant_red r11, r12, r14, r9
  smulwt r9, r8, r9
  smlabt r9, r9, r11, r12
  smuadx r10, r4, r6
  plant_red r11, r12, r14, r10
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r2 = (a0b2 + a1b1 + a2b0) + zeta*a3b3 */
  smulbb r9, r4, r7
  smlatt r9, r4, r6, r9
  smlabb r9, r5, r6, r9
  plant_red r11, r12, r14, r9
  smultt r10, r5, r7
  plant_red r11, r12, r14, r10
  smulwt r10, r8, r10
  smlabt r10, r10, r11, r12
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r3 = a0b3 + a1b2 + a2b1 + a3b0 */
  smuadx r9, r4, r7
  smladx r9, r5, r6, r9
  plant_red r11, r12, r14, r9
  asr r9, r9, #16
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2
.endm


/* void frombytes_mul_asm_16_16(int16_t *r, const int16_t *b,
 *                              const uint8_t *sk, const int32_t zetas[64]); */
.global frombytes_mul_asm_16_16
.type frombytes_mul_asm_16_16, %function
.align 2
frombytes_mul_asm_16_16:
  push {r4-r11, lr}
  sub sp, sp, #4

  movw r12, #26632
  movw r11, #0
  movt r11, #3329
  movw r14, #62209
  movt r14, #27560

  mov r9, #64
  str r9, [sp]
1:
  fm_d4np 0, 0
  fm_d4np 1, 0
  ldr r9, [sp]
  subs r9, r9, #1
  str r9, [sp]
  bne.w 1b

  add sp, sp, #4
  pop {r4-r11, pc}
.size frombytes_mul_asm_16_16, .-frombytes_mul_asm_16_16


/* void frombytes_mul_asm_acc_16_16(int16_t *r, const int16_t *b,
 *                                  const uint8_t *sk, const int32_t zetas[64]); */
.global frombytes_mul_asm_acc_16_16
.type frombytes_mul_asm_acc_16_16, %function
.align 2
frombytes_mul_asm_acc_16_16:
  push {r4-r11, lr}
  sub sp, sp, #4

  movw r12, #26632
  movw r11, #0
  movt r11, #3329
  movw r14, #62209
  movt r14, #27560

  mov r9, #64
  str r9, [sp]
1:
  fm_d4np 0, 1
  fm_d4np 1, 1
  ldr r9, [sp]
  subs r9, r9, #1
  str r9, [sp]
  bne.w 1b

  add sp, sp, #4
  pop {r4-r11, pc}
.size frombytes_mul_asm_acc_16_16, .-frombytes_mul_asm_acc_16_16
