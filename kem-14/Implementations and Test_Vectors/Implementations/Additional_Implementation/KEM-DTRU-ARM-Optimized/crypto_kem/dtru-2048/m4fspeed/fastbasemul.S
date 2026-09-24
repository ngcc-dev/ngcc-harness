.syntax unified
.cpu cortex-m4
.thumb

//not include "macros.i", define montgomery
.macro montgomery q, qinv, a, tmp
  smulbt \tmp, \a, \qinv
  smlabb \tmp, \q, \tmp, \a
.endm

.global doublebasemul_asm_8x8
.type doublebasemul_asm_8x8, %function
.align 2
doublebasemul_asm_8x8:
  push {r4-r11, lr}

  rptr      .req r0
  tmp2      .req r1 // aptr
  aptr      .req r1
  bptr      .req r2 // 
  tmp3      .req r2
  zeta_ptr  .req r3 //
  zeta      .req r3
  poly0     .req r4
  poly1     .req r6
  poly2     .req r5
  poly3     .req r7
  poly4     .req r8
  poly5     .req r9
  poly6     .req r10
  poly7     .req r11
  q         .req r12
  qinv      .req r12
  tmp       .req r14

  movw q, #3457
  movt qinv, #52607 //-q^(-1),Montgomery Reduction

    // poly0-poly7里各load两个系数，放完后指针后移4个系数
    ldr poly0, [aptr], #4  // (a1,a0)
    ldr poly1, [aptr], #4  // (a3,a2)
    ldr poly2, [aptr], #4  // (a5,a4)
    ldr poly3, [aptr], #4  // (a7,a6)
    ldr poly4, [bptr], #4  // (b1,b0)
    ldr poly5, [bptr], #4  // (b3,b2)
    ldr poly6, [bptr], #4  // (b5,b4)
    ldr poly7, [bptr], #4  // (b7,b6)
    @ ldm aptr, {poly0-poly3}
    @ ldm bptr, {poly4-poly7}
    @ vmov s2, s3, aptr, bptr

    smultt tmp, poly0, poly7 // a1*b7
    smlabb tmp, poly1, poly7, tmp // a1*b7+a2*b6
    smlatt tmp, poly1, poly6, tmp // a1*b7+a2*b6+a3*b5
    smlabb tmp, poly2, poly6, tmp // a1*b7+a2*b6+a3*b5+a4*b4
    smlatt tmp, poly2, poly5, tmp // a1*b7+a2*b6+a3*b5+a4*b4+a5*b3
    smlabb tmp, poly3, poly5, tmp // a1*b7+a2*b6+a3*b5+a4*b4+a5*b3+a6*b2
    smlatt tmp, poly3, poly4, tmp // a1*b7+a2*b6+a3*b5+a4*b4+a5*b3+a6*b2+a7*b1
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    smlabb tmp2, poly0, poly4, tmp2 // r[0] = a0*b0 + (a1*b7+a2*b6+a3*b5+a4*b4+a5*b3+a6*b2+a7*b1)*zeta
    montgomery q, qinv, tmp2, tmp  // r[0] in upper half of tmp

    smuadx tmp2, poly1, poly7       // a2*b7+a3*b6
    smladx tmp2, poly2, poly6, tmp2  // a2*b7+a3*b6+a4*b5+a5*b4
    smladx tmp2, poly3, poly5, tmp2  // a2*b7+a3*b6+a4*b5+a5*b4+a6*b3+a7*b2
    montgomery q, qinv, tmp2, tmp3
    smultb tmp3, tmp3, zeta        
    smladx tmp3, poly0, poly4, tmp3 // r[1] = a0b1+a1b0+(a2*b7+a3*b6+a4*b5+a5*b4+a6*b3+a7*b2)*zeta
    montgomery q, qinv, tmp3, tmp2 // r[1] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[0]和r[1]存入tmp
    str tmp, [rptr], #4

    smultt tmp, poly1, poly7       // a3*b7
    smlabb tmp, poly2, poly7, tmp  // a3*b7+a4*b6
    smlatt tmp, poly2, poly6, tmp  // a3*b7+a4*b6+a5*b5
    smlabb tmp, poly3, poly6, tmp  // a3*b7+a4*b6+a5*b5+a6*b4
    smlatt tmp, poly3, poly5, tmp  // a3*b7+a4*b6+a5*b5+a6*b4+a7*b3
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    smlabb tmp2, poly0, poly5, tmp2
    smlatt tmp2, poly0, poly4, tmp2 // !!!!!!!!!!!!!!!!!!!!!!!!!
    smlabb tmp2, poly1, poly4, tmp2
    montgomery q, qinv, tmp2, tmp  // r[2] in upper half of tmp

    smuadx tmp2, poly2, poly7       // a4*b7+a5*b6
    smladx tmp2, poly3, poly6, tmp2  // a4*b7+a5*b6+a6*b5+a5*b6
    montgomery q, qinv, tmp2, tmp3
    smultb tmp3, tmp3, zeta 
    smladx tmp3, poly0, poly5, tmp3  // a0*b3+a1*b2+(a4*b7+a5*b6+a6*b5+a5*b6)*zeta
    smladx tmp3, poly1, poly4, tmp3  // a0*b3+a1*b2+a2*b1+a3*b0+(a4*b7+a5*b6+a6*b5+a5*b6)*zeta
    montgomery q, qinv, tmp3, tmp2 // r[3] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[2]和r[3]存入tmp
    str tmp, [rptr], #4

    smultt tmp, poly2, poly7       // a5*b7
    smlabb tmp, poly3, poly7, tmp  // a5*b7+a6*b6
    smlatt tmp, poly3, poly6, tmp  // a5*b7+a6*b6+a7*b5
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    smlabb tmp2, poly0, poly6, tmp2 // a0*b4+(a5*b7+a6*b6+a7*b5)*zeta
    smlatt tmp2, poly0, poly5, tmp2 // a0*b4+a1*b3+(a5*b7+a6*b6+a7*b5)*zeta
    smlabb tmp2, poly1, poly5, tmp2 // a0*b4+a1*b3+a2*b2+(a5*b7+a6*b6+a7*b5)*zeta
    smlatt tmp2, poly1, poly4, tmp2 // a0*b4+a1*b3+a2*b2+a3*b1+(a5*b7+a6*b6+a7*b5)*zeta
    smlabb tmp2, poly2, poly4, tmp2 // a0*b4+a1*b3+a2*b2+a3*b1+a4*b0+(a5*b7+a6*b6+a7*b5)*zeta
    montgomery q, qinv, tmp2, tmp  // r[4] in upper half of tmp

    smuadx tmp2, poly3, poly7       // a6*b7+a7*b6
    montgomery q, qinv, tmp2, tmp3
    smultb tmp3, tmp3, zeta 
    smladx tmp3, poly0, poly6, tmp3
    smladx tmp3, poly1, poly5, tmp3
    smladx tmp3, poly2, poly4, tmp3
    montgomery q, qinv, tmp3, tmp2 // r[5] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[4]和r[5]存入tmp
    str tmp, [rptr], #4

    smultt tmp, poly3, poly7        // a7*b7
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta         // a7*b7*zeta
    smlabb tmp2, poly0, poly7, tmp2
    smlatt tmp2, poly0, poly6, tmp2
    smlabb tmp2, poly1, poly6, tmp2
    smlatt tmp2, poly1, poly5, tmp2
    smlabb tmp2, poly2, poly5, tmp2
    smlatt tmp2, poly2, poly4, tmp2
    smlabb tmp2, poly3, poly4, tmp2
    montgomery q, qinv, tmp2, tmp  // r[6] in upper half of tmp

    smuadx tmp3, poly0, poly7       // a0*b7+a1*b6
    smladx tmp3, poly1, poly6, tmp3
    smladx tmp3, poly2, poly5, tmp3
    smladx tmp3, poly3, poly4, tmp3
    montgomery q, qinv, tmp3, tmp2 // r[7] in upper half of tmp2
    pkhtb tmp, tmp2, tmp, asr#16 // r[6]和r[7]存入tmp
    str tmp, [rptr]
    vmov aptr, bptr, s2, s3

  pop {r4-r11, pc}

