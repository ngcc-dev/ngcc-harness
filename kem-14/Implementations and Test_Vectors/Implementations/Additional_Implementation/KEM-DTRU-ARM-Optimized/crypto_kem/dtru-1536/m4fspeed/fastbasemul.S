.syntax unified
.cpu cortex-m4
.thumb

.macro montgomery q, qinv, a, tmp
  smulbt \tmp, \a, \qinv
  smlabb \tmp, \q, \tmp, \a
.endm

.global triplebasemul_asm
.type triplebasemul_asm, %function
.align 2
triplebasemul_asm:
  push {r4-r11, lr}

  rptr      .req r0
  aptr      .req r1
  bptr      .req r2
  zeta_ptr  .req r3
  poly0     .req r4
  poly1     .req r5
  poly2     .req r6
  poly3     .req r7
  tmp0      .req r8
  tmp1      .req r9
  tmp2      .req r10
  tmp3      .req r11
  zeta      .req r12
  tmp4      .req r14
  q         .req r1
  qinv      .req r2

#ifdef USE_REPT
  .rept 384
#else
  movw tmp0, #384
  1:
    push {tmp0}
#endif

  ldr poly0, [aptr], #4
  ldr poly1, [bptr], #4
  ldr poly2, [aptr], #4
  ldr poly3, [bptr], #4
  ldr.w zeta, [zeta_ptr], #2

  vmov s0, aptr
  vmov s1, bptr
  movw q, #3457
  movt qinv, #52607  
  //重排列tmp0{a0a2} tmp1{a3a1} tmp2{b0b2} tmp3{b3b1}来使用smuadx计算a[x]b[y]+a[y]b[x]
  pkhbt tmp0, poly0, poly2, lsl#16
  pkhtb tmp1, poly0, poly2, asr#16
  pkhbt tmp2, poly1, poly3, lsl#16
  pkhtb tmp3, poly1, poly3, asr#16
  //c0 = a0b0 + (a1b3+a3b1+a2b2)*zeta
  smuadx tmp4, tmp1, tmp3
  //tmp1与tmp3可释放作为临时寄存器
  smlatt tmp1, tmp0, tmp2, tmp4
  montgomery q, qinv, tmp1, tmp4
  smultb tmp4, tmp4, zeta
  smlabb tmp4, tmp0, tmp2, tmp4
  //c0在tmp1的高半字
  montgomery q, qinv, tmp4, tmp1

  //c1 = a0b1+a1b0+(a2b3+a3b2)*zeta
  smuadx tmp4, poly2, poly3
  montgomery q, qinv, tmp4, tmp3
  smuadx tmp4, poly0, poly1
  smlatb tmp4, tmp3, zeta, tmp4
  //c1在tmp的高半字
  montgomery q, qinv, tmp4, tmp3
  //保存c[0]c[1]
  pkhtb tmp1, tmp3, tmp1, asr#16
  str tmp1, [rptr], #4

  //c2 = a0b2+a2b0 + a1b1 + a3b3*zeta
  //计算a0b2+a2b0
  smuadx tmp1, tmp0, tmp2
  //计算a3b3*zeta
  smultt tmp4, poly2, poly3
  montgomery q, qinv, tmp4, tmp3
  smlatb tmp1, tmp3, zeta, tmp1
  //c2在tmp4的高半字
  smlatt tmp1, poly0, poly1, tmp1
  montgomery q, qinv, tmp1, tmp4
  
  //c3 = a0b3+a3b0+a1b2+a2b1
  //重排列tmp0{a0a3} tmp1 {a2a1} tmp2{b0b3} tmp3{b2b1}
  pkhbt tmp0, poly0, poly2
  pkhtb tmp1, poly0, poly2
  pkhbt tmp2, poly1, poly3
  pkhtb tmp3, poly1, poly3

  smuadx poly0, tmp0, tmp2
  smuadx poly1, tmp1, tmp3
  add poly0, poly1, poly0
  //c3在poly1的高半字
  montgomery q, qinv, poly0, poly1

  pkhtb tmp4, poly1, tmp4, asr#16
  str tmp4, [rptr], #4

  vmov aptr, s0
  vmov bptr, s1

#ifdef USE_REPT
  .endr
#else
    pop {tmp0}
    subs.w tmp0, #1
  bne.w 1b
#endif

  pop {r4-r11, pc}

  





