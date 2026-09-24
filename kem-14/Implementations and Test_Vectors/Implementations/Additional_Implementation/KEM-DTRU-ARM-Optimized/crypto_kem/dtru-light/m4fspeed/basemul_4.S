.syntax unified
.cpu cortex-m4
.thumb

.macro montgomery q, qinv, a, tmp
  smulbt \tmp, \a, \qinv
  smlabb \tmp, \q, \tmp, \a
.endm
.global doublebasemul_asm
.type doublebasemul_asm, %function
.align 2
doublebasemul_asm:
  push {r4-r11, lr}

  rptr      .req r0
  aptr      .req r1
  bptr      .req r2 // 
  zeta_ptr  .req r3 //
  poly0     .req r4
  poly1     .req r6
  poly2     .req r5
  poly3     .req r7
  tmp     .req r8
  tmp2     .req r9
  tmp3     .req r10
  tmp4     .req r11
  q         .req r12
  qinv      .req r12
  zeta       .req r14

  movw q, #769
  movt qinv, #767 //-q^(-1),Montgomery Reduction

.rept 64
    ldr poly0, [aptr], #4
    ldr poly1, [aptr], #4
    ldr poly2, [bptr], #4
    ldr poly3, [bptr], #4
    ldr zeta, [zeta_ptr], #4

    smultt tmp, poly0, poly3 // a1*b3
    smlabb tmp, poly1, poly3, tmp // a1*b3+a2*b2
    smlatt tmp, poly1, poly2, tmp // a1*b3+a2*b2+a3*b1
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    smlabb tmp2, poly0, poly2, tmp2 // r[0] = a0*b0 + (a1*b3+a2*b2+a3*b1)*zeta
    montgomery q, qinv, tmp2, tmp  // r[0] in upper half of tmp

    smuadx tmp2, poly1, poly3       // a2*b3+a3*b2
    montgomery q, qinv, tmp2, tmp3
    smultb tmp3, tmp3, zeta        
    smladx tmp3, poly0, poly2, tmp3 // r[1] = a0b1+a1b0+(a2*b3+a3*b2)*zeta
    montgomery q, qinv, tmp3, tmp2 // r[1] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[0]和r[1]存入tmp
    str tmp, [rptr], #4

    smultt tmp, poly1, poly3 // a3*b3
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    smlabb tmp2, poly1, poly2, tmp2 // r[2] = a2*b0+ (a3*b3)*zeta
    smlabb tmp2, poly0, poly3, tmp2 // r[2] = a0*b2 +a2*b0+ (a3*b3)*zeta
    smlatt tmp2, poly0, poly2, tmp2 // r[2] = a1*b1+a0*b2 +a2*b0+ (a3*b3)*zeta
    montgomery q, qinv, tmp2, tmp  // r[0] in upper half of tmp

    smuadx tmp3, poly0, poly3       // a0*b3+a3*b0
    smladx tmp3, poly1, poly2, tmp3  // a0*b3+a3*b0+a1*b2+a2*b1
    montgomery q, qinv, tmp3, tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[0]和r[1]存入tmp
    str tmp, [rptr], #4


    ldr poly0, [aptr], #4
    ldr poly1, [aptr], #4
    ldr poly2, [bptr], #4
    ldr poly3, [bptr], #4

    smultt tmp, poly0, poly3 // a1*b3
    smlabb tmp, poly1, poly3, tmp // a1*b3+a2*b2
    smlatt tmp, poly1, poly2, tmp // a1*b3+a2*b2+a3*b1
    montgomery q, qinv, tmp, tmp2
    smultt tmp2, tmp2, zeta
    smlabb tmp2, poly0, poly2, tmp2 // r[0] = a0*b0 + (a1*b3+a2*b2+a3*b1)*zeta
    montgomery q, qinv, tmp2, tmp  // r[0] in upper half of tmp

    smuadx tmp2, poly1, poly3       // a2*b3+a3*b2
    montgomery q, qinv, tmp2, tmp3
    smultt tmp3, tmp3, zeta        
    smladx tmp3, poly0, poly2, tmp3 // r[1] = a0b1+a1b0+(a2*b3+a3*b2)*zeta
    montgomery q, qinv, tmp3, tmp2 // r[1] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[0]和r[1]存入tmp
    str tmp, [rptr], #4

    smultt tmp, poly1, poly3 // a3*b3
    montgomery q, qinv, tmp, tmp2
    smultt tmp2, tmp2, zeta
    smlabb tmp2, poly1, poly2, tmp2 // r[2] = a0*b2 +a2*b0+ (a3*b3)*zeta
    smlabb tmp2, poly0, poly3, tmp2 // r[2] = a0*b2 +a2*b0+ (a3*b3)*zeta
    smlatt tmp2, poly0, poly2, tmp2 // r[2] = a1*b1+a0*b2 +a2*b0+ (a3*b3)*zeta
    montgomery q, qinv, tmp2, tmp  // r[0] in upper half of tmp

    smuadx tmp3, poly0, poly3       // a0*b3+a3*b0
    smladx tmp3, poly1, poly2, tmp3  // a0*b3+a3*b0+a1*b2+a2*b1
    montgomery q, qinv, tmp3, tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[0]和r[1]存入tmp
    str tmp, [rptr], #4

.endr
  pop {r4-r11, pc}

    .unreq rptr      
    .unreq aptr 
    .unreq bptr     
    .unreq zeta_ptr 
    .unreq poly0    
    .unreq poly1    
    .unreq poly2    
    .unreq poly3    
    .unreq q        
    .unreq qinv     
    .unreq tmp      
    .unreq tmp2      
    .unreq tmp3      
    .unreq zeta