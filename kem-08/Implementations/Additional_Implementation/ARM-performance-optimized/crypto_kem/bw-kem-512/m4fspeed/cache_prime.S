.syntax unified
.cpu cortex-m4
.thumb

/* void poly_cache_prime_asm(int16_t *dst, const int16_t *src,
 *                           const int32_t *zetas);
 *
 * For each degree-4 block k (k = 0..127) of `src`, compute the Plantard-twisted
 * partial cache:
 *   za1 = plant(a1, zeta), za2 = plant(a2, zeta), za3 = plant(a3, zeta)
 * and store them in `dst` as two int32 words per block:
 *   dst_w0 = za1 | za2<<16
 *   dst_w1 = za3 | 0<<16
 * Adjacent blocks 2i and 2i+1 share |zeta|, with the second using -zeta -
 * same pattern the basemul / cache_prime in bw-kem-256 follows.
 *
 * dst is a poly_t (KYBER_N=512 int16, 1024 bytes = 128 blocks x 8 bytes).
 * zetas has 64 int32 Plantard-encoded constants.
 */
.global poly_cache_prime_asm
.type poly_cache_prime_asm, %function
.align 2
poly_cache_prime_asm:
  push {r4-r11, lr}

  dst     .req r0
  src     .req r1
  zetaptr .req r2
  q       .req r3   /* q in HIGH half (low garbage) */
  qa      .req r4
  ctr     .req r5
  A01     .req r6
  A23     .req r7
  zeta    .req r8
  t1      .req r9
  t2      .req r10
  t3      .req r11
  pk      .req r12

  movw qa, #26632                /* qa = 8*q */
  movt q, #3329                  /* q[31:16] = 3329 */
  mov  ctr, #64                  /* 64 iters x 2 blocks = 128 blocks */

  .macro cp_block neg
    ldr A01, [src], #4           /* (a0|a1<<16) */
    ldr A23, [src], #4           /* (a2|a3<<16) */
  .if \neg
    ldr zeta, [zetaptr], #4
    neg zeta, zeta
  .else
    ldr zeta, [zetaptr]
  .endif

    /* za1 = plant(a1, zeta);   a1 = A01.top */
    smulwt t1, zeta, A01
    smlabt t1, t1, q, qa         /* za1 in t1[31:16] */

    /* za2 = plant(a2, zeta);   a2 = A23.bottom */
    smulwb t2, zeta, A23
    smlabt t2, t2, q, qa         /* za2 in t2[31:16] */

    /* za3 = plant(a3, zeta);   a3 = A23.top */
    smulwt t3, zeta, A23
    smlabt t3, t3, q, qa         /* za3 in t3[31:16] */

    /* w0 = za1 | za2<<16
     *    pkhtb Rd,Rn,Rm,asr#n  ->  Rn[31:16] | (Rm asr n)[15:0]
     *    Rn = t2 (top16 = za2);  Rm asr 16 -> t1 high to low (= za1) */
    pkhtb pk, t2, t1, asr#16
    str pk, [dst], #4

    /* w1 = za3 | 0
     *    lsr  -> unsigned shift, fills high half with zero */
    lsr pk, t3, #16
    str pk, [dst], #4
  .endm

1:
    cp_block 0                   /* +zeta block */
    cp_block 1                   /* -zeta block, advances zetaptr */
    subs ctr, #1
  bne 1b

  pop {r4-r11, pc}

  .unreq dst
  .unreq src
  .unreq zetaptr
  .unreq q
  .unreq qa
  .unreq ctr
  .unreq A01
  .unreq A23
  .unreq zeta
  .unreq t1
  .unreq t2
  .unreq t3
  .unreq pk
.size poly_cache_prime_asm, .-poly_cache_prime_asm
