#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

.macro doublebasemul_np_16_16 rptr, bptr, zeta, poly0, poly1, poly3, res0, tmp, q, qa, qinv
  ldr \poly0, [\bptr], #4

  smulwt \tmp, \zeta, \poly1
  smlabt \tmp, \tmp, \q, \qa
  smultt \tmp, \poly0, \tmp
  smlabb \tmp, \poly0, \poly1, \tmp
  plant_red \q, \qa, \qinv, \tmp

  smuadx \res0, \poly0, \poly1
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  str \res0, [\rptr], #4

  neg \zeta, \zeta

  ldr \poly0, [\bptr], #4

  smulwt \tmp, \zeta, \poly3
  smlabt \tmp, \tmp, \q, \qa
  smultt \tmp, \poly0, \tmp
  smlabb \tmp, \poly0, \poly3, \tmp
  plant_red \q, \qa, \qinv, \tmp

  smuadx \res0, \poly0, \poly3
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  str \res0, [\rptr], #4
.endm

.macro doublebasemul_np_acc_16_16 rptr, bptr, zeta, poly0, poly1, poly3, res0, tmp, q, qa, qinv
  ldr \poly0, [\bptr], #4

  smulwt \tmp, \zeta, \poly1
  smlabt \tmp, \tmp, \q, \qa
  smultt \tmp, \poly0, \tmp
  smlabb \tmp, \poly0, \poly1, \tmp
  plant_red \q, \qa, \qinv, \tmp

  smuadx \res0, \poly0, \poly1
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  ldr \tmp, [\rptr]
  uadd16 \tmp, \tmp, \res0
  str \tmp, [\rptr], #4

  neg \zeta, \zeta

  ldr \poly0, [\bptr], #4

  smulwt \tmp, \zeta, \poly3
  smlabt \tmp, \tmp, \q, \qa
  smultt \tmp, \poly0, \tmp
  smlabb \tmp, \poly0, \poly3, \tmp
  plant_red \q, \qa, \qinv, \tmp

  smuadx \res0, \poly0, \poly3
  plant_red \q, \qa, \qinv, \res0

  pkhtb \res0, \res0, \tmp, asr#16
  ldr \tmp, [\rptr]
  uadd16 \tmp, \tmp, \res0
  str \tmp, [\rptr], #4
.endm


// void basemul_asm_opt_16_16_noprime(int16_t *r, const int16_t *a, const int16_t *b, const int32_t zetas[64])
.global basemul_asm_opt_16_16_noprime
.type basemul_asm_opt_16_16_noprime, %function
.align 2
basemul_asm_opt_16_16_noprime:
  push {r4-r11, r14}

  rptr     .req r0
  aptr     .req r1
  bptr     .req r2
  zetaptr  .req r3
  t0       .req r4
  t1       .req r5
  poly0    .req r6
  res0     .req r7
  tmp      .req r8
  q        .req r9
  qa       .req r10
  qinv     .req r11
  zeta     .req r12
  ctr      .req r14

  movw qa, #26632
  movt  q, #3329
  movw qinv, #62209
  movt qinv, #27560

  add ctr, rptr, #64*2*4   // 64 iters * 4 int16 (8 bytes) = 512 bytes
  1:
    ldr.w zeta, [zetaptr], #4
    ldr t0, [aptr], #4
    ldr t1, [aptr], #4

    doublebasemul_np_16_16 rptr, bptr, zeta, poly0, t0, t1, res0, tmp, q, qa, qinv

    cmp.w rptr, ctr
    bne.w 1b

  pop {r4-r11, pc}
  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq zetaptr
  .unreq t0
  .unreq t1
  .unreq poly0
  .unreq res0
  .unreq tmp
  .unreq q
  .unreq qa
  .unreq qinv
  .unreq zeta
  .unreq ctr
.size basemul_asm_opt_16_16_noprime, . - basemul_asm_opt_16_16_noprime

// void basemul_asm_acc_opt_16_16_noprime(int16_t *r, const int16_t *a, const int16_t *b, const int32_t zetas[64])
.global basemul_asm_acc_opt_16_16_noprime
.type basemul_asm_acc_opt_16_16_noprime, %function
.align 2
basemul_asm_acc_opt_16_16_noprime:
  push {r4-r11, r14}

  rptr     .req r0
  aptr     .req r1
  bptr     .req r2
  zetaptr  .req r3
  t0       .req r4
  t1       .req r5
  poly0    .req r6
  res0     .req r7
  tmp      .req r8
  q        .req r9
  qa       .req r10
  qinv     .req r11
  zeta     .req r12
  ctr      .req r14

  movw qa, #26632
  movt  q, #3329
  movw qinv, #62209
  movt qinv, #27560

  add ctr, rptr, #64*2*4
  1:
    ldr.w zeta, [zetaptr], #4
    ldr t0, [aptr], #4
    ldr t1, [aptr], #4

    doublebasemul_np_acc_16_16 rptr, bptr, zeta, poly0, t0, t1, res0, tmp, q, qa, qinv

    cmp.w rptr, ctr
    bne.w 1b

  pop {r4-r11, pc}
  .unreq rptr
  .unreq aptr
  .unreq bptr
  .unreq zetaptr
  .unreq t0
  .unreq t1
  .unreq poly0
  .unreq res0
  .unreq tmp
  .unreq q
  .unreq qa
  .unreq qinv
  .unreq zeta
  .unreq ctr
.size basemul_asm_acc_opt_16_16_noprime, . - basemul_asm_acc_opt_16_16_noprime
