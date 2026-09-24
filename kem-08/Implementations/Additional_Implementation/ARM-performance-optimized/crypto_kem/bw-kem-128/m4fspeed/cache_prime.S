.syntax unified
.cpu cortex-m4
.thumb

.global poly_cache_prime_asm
.type poly_cache_prime_asm, %function
.align 2
poly_cache_prime_asm:
  push {r4-r8, lr}

  dst     .req r0
  src     .req r1
  zetaptr .req r2
  q       .req r3
  qa      .req r4
  ctr     .req r5
  poly    .req r6
  zeta    .req r7
  tmp     .req r8

  movw q,  #3329       // KYBER_Q
  movw qa, #26632      // qa = (q << 16 mod 2^17) / 2 (matches asm Plant)
  mov  ctr, #64

1:
  ldr  poly, [src], #4
  ldr  zeta, [zetaptr], #4

  smulwt tmp, zeta, poly          // tmp = (zeta * (int16)poly_hi) >> 16
  smlabb tmp, tmp, q, qa          // tmp = qa + (int16)tmp * q
  pkhbt  tmp, poly, tmp           // tmp = (poly_lo | tmp_hi << 16)
  str    tmp, [dst], #4

  ldr  poly, [src], #4
  neg  zeta, zeta

  smulwt tmp, zeta, poly
  smlabb tmp, tmp, q, qa
  pkhbt  tmp, poly, tmp
  str    tmp, [dst], #4

  subs ctr, #1
  bne  1b

  pop {r4-r8, pc}
.size poly_cache_prime_asm, . - poly_cache_prime_asm
