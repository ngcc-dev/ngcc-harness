.syntax unified
.cpu cortex-m4
.thumb

.macro load a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
  ldr.w \a0, [\a, \mem0]
  ldr.w \a1, [\a, \mem1]
  ldr.w \a2, [\a, \mem2]
  ldr.w \a3, [\a, \mem3]
.endm

.macro store a, a0, a1, a2, a3, mem0, mem1, mem2, mem3
  str.w \a0, [\a, \mem0]
  str.w \a1, [\a, \mem1]
  str.w \a2, [\a, \mem2]
  str.w \a3, [\a, \mem3]
.endm

.macro doublebarrett_init a, tmp, tmp2, q, barrettconst
  smulbb \tmp, \a, \barrettconst
  smultb \tmp2, \a, \barrettconst
  asr \tmp, \tmp, #26
  asr \tmp2, \tmp2, #26
  smulbb \tmp, \tmp, \q
  smulbb \tmp2, \tmp2, \q
  pkhbt \tmp, \tmp, \tmp2, lsl#16
  usub16 \a, \a, \tmp
.endm

.macro montgomery q, qinv, a, tmp
  smulbt \tmp, \a, \qinv //-q^(-1)
  smlabb \tmp, \q, \tmp, \a
.endm

.macro mul_twiddle tb, a, twiddle, tmp, tmp2, q, qinv
    smulb\tb \tmp, \a, \twiddle
    smult\tb \a, \a, \twiddle
    montgomery \q, \qinv, \tmp, \tmp2 // reduce -> result in tmp2
    montgomery \q, \qinv, \a, \tmp // reduce -> result in tmp
    pkhtb \a, \tmp, \tmp2, asr#16 // combine results from above in one register as 16bit halves
.endm

.macro doubleinvbutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
  usub16 \tmp, \a0, \a1
  uadd16 \a0, \a0, \a1
  doublebarrett_init \tmp, \a1, \tmp2, \q, \twiddle_ptr
  doublebarrett_init \a0, \a1, \tmp2, \q, \twiddle_ptr
  smulb\tb \a1, \tmp, \twiddle
  smult\tb \tmp, \tmp, \twiddle
  montgomery \q, \qinv, \a1, \tmp2
  montgomery \q, \qinv, \tmp, \a1
  pkhtb \a1, \a1, \tmp2, asr#16
.endm

.macro two_doubleinvbutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
  doubleinvbutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
  doubleinvbutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
.endm


.macro _3_layer_double_GS_16_fp_reduce1 c0, c1, c2, c3, c4, c5, c6, c7, xi0, xi1, xi2, xi3, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
  vmov \twiddle, \xi0
  two_doubleinvbutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr

  vmov \twiddle, \xi1
  two_doubleinvbutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
  
  vmov \twiddle, \xi2
  two_doubleinvbutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
  two_doubleinvbutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr

  vmov \twiddle, \xi3
  two_doubleinvbutterfly b, b, \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
  two_doubleinvbutterfly b, b, \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr

.endm


.macro doubleinvbutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv, t1,poly,s23
  movw \poly, #19412
  usub16 \tmp, \a0, \a1

  vmov \s23, \a1
  doublebarrett_init \tmp, \a1, \tmp2, \q, \poly
  vmov \a1, \s23

  smulbt \tmp2, \tmp, \twiddle
  smultt \tmp, \tmp, \twiddle
  montgomery \q, \qinv, \tmp2, \twiddle // twiddle is used as temp register
  montgomery \q, \qinv, \tmp, \tmp2
  pkhtb \tmp2, \tmp2, \twiddle, asr#16 // tmp2 = t

  uadd16 \a0, \a0, \a1
  vmov \s23, \tmp2
  doublebarrett_init \a0, \tmp, \tmp2, \q, \poly
  vmov \tmp2, \s23

  usub16 \a0, \a0, \tmp2
  doublebarrett_init \a0, \tmp, \tmp2, \q, \poly
 

  mul_twiddle b, \a0, \t1, \tmp, \a1, \q, \qinv
  vmov \tmp2, \s23
  smulbt \tmp, \tmp2, \t1
  smultt \tmp2, \tmp2, \t1
  montgomery \q, \qinv, \tmp, \a1
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \a1, \tmp, \a1, asr#16 
.endm


.global invntt_fast_fpu
.type invntt_fast_fpu, %function
.align 2
invntt_fast_fpu:
  push {r4-r11, lr}
  vpush.w {s16-s24}
  poly        .req r0
  twiddle_ptr .req r1
  poly0       .req r2
  poly1       .req r3
  poly2       .req r4
  poly3       .req r5
  poly4       .req r6
  poly5       .req r7
  poly6       .req r8
  root3       .req r8 // new add
  poly7       .req r9
  tmp3        .req r9 // new add
  twiddle     .req r10
  barrettconst .req r10 // new add
  qinv        .req r11
  q           .req r11
  tmp         .req r12
  tmp2        .req r14

  movw q, #3457
  movt qinv, #52607 //-q^(-1),Montgomery Reduction

  
  ### LAYER 7+6+5+4
  .equ distance, 64
  .equ offset, 128
  .equ strincr, 128

#ifdef USE_REPT
  .rept 8
#else
  movw tmp, #8
  1:
    push {tmp}
#endif
    vldm twiddle_ptr!, {s8-s15} //8 float registers, 4+2+1+空 4+2+1+1

#ifdef USE_REPT
    .rept 4
#else
    movw tmp, #4
    2:
      push {tmp}
#endif
    
      load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
      load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
      vmov s21, twiddle_ptr
      movw twiddle_ptr, #19412
      _3_layer_double_GS_16_fp_reduce1 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
      vmov twiddle_ptr, s21


    
      vmov s0, poly0 //a0
      vmov s1, poly1 //a2
      vmov s2, poly2 //a4
      vmov s3, poly3 //a6
      vmov s4, poly4 //a8
      vmov s5, poly5 //a10
      vmov s6, poly6 //a12
      vmov s7, poly7 //a14

      load poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
      load poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset
      vmov s21, twiddle_ptr
      movw twiddle_ptr, #19412
      _3_layer_double_GS_16_fp_reduce1 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s12, s13, s14, s15, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
      vmov twiddle_ptr, s21

      vmov twiddle, s15

      vmov s21, twiddle_ptr
      movw twiddle_ptr, #19412

      vmov tmp2, s1 // load a2
      uadd16 tmp, tmp2, poly1
      usub16 poly1, tmp2, poly1

      vmov s20, tmp
      doublebarrett_init poly1, tmp, tmp2, q, twiddle_ptr
      mul_twiddle t, poly1, twiddle, tmp, tmp2, q, qinv
      vmov tmp, s20
      str.w poly1, [poly, #1*distance/4+offset]

      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      str.w tmp, [poly, #1*distance/4]

      vmov tmp2, s3 // load a6
      uadd16 tmp, tmp2, poly3
      usub16 poly3, tmp2, poly3
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      str.w tmp, [poly, #3*distance/4]

      doublebarrett_init poly3, tmp, tmp2, q, twiddle_ptr
      mul_twiddle t, poly3, twiddle, poly1, tmp2, q, qinv
      str.w poly3, [poly, #3*distance/4+offset]

      vmov tmp2, s5 // load a10
      uadd16 tmp, tmp2, poly5
      usub16 poly5, tmp2, poly5
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly5, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly5, twiddle, poly3, tmp2, q, qinv
      str.w poly5, [poly, #5*distance/4+offset]
      str.w tmp, [poly, #5*distance/4]

      vmov tmp2, s7 // load a14
      uadd16 tmp, tmp2, poly7
      usub16 poly7, tmp2, poly7
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly7, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly7, twiddle, poly5, tmp2, q, qinv
      str.w poly7, [poly, #7*distance/4+offset]
      str.w tmp, [poly, #7*distance/4]

      vmov poly5, s2 // load a4
      uadd16 tmp, poly5, poly2
      usub16 poly2, poly5, poly2
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly2, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly2, twiddle, poly5, tmp2, q, qinv
      str.w poly2, [poly, #2*distance/4+offset]  
      str.w tmp, [poly, #2*distance/4]

      vmov poly5, s4 // load a8
      uadd16 tmp, poly5, poly4
      usub16 poly4, poly5, poly4
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly4, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly4, twiddle, poly5, tmp2, q, qinv 
      str.w poly4, [poly, #4*distance/4+offset] 
      str.w tmp, [poly, #4*distance/4]
      
      vmov poly7, s6 // load a12
      uadd16 tmp, poly7, poly6
      usub16 poly6, poly7, poly6
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly6, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly6, twiddle, poly5, tmp2, q, qinv
      str.w poly6, [poly, #6*distance/4+offset]  
      str.w tmp, [poly, #6*distance/4]

      vmov poly5, s0 // load a0
      uadd16 tmp, poly5, poly0
      usub16 poly0, poly5, poly0
      doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
      doublebarrett_init poly0, poly3, tmp2, q, twiddle_ptr
      mul_twiddle t, poly0, twiddle, poly5, tmp2, q, qinv
      str.w poly0, [poly, #offset]   
      str.w tmp, [poly], #4
      vmov twiddle_ptr, s21
      
#ifdef USE_REPT
    .endr
#else
      pop {tmp}
      subs.w tmp, #1
    bne.w 2b
#endif
    add poly, #(128-8)*2
#ifdef USE_REPT
  .endr
#else
    pop {tmp}
    subs.w tmp, #1
  bne.w 1b
#endif

  

  ### LAYER 3+2 + the first level
  .equ distance, 1024
  sub.w poly, #2048
  vldm twiddle_ptr, {s8-s12} // 5 float registers, 10 zetas
  .rept 64

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    movw twiddle_ptr, #19412
    vmov twiddle, s8
    two_doubleinvbutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv,twiddle_ptr
    vmov twiddle, s9
    two_doubleinvbutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv,twiddle_ptr

    vmov twiddle, s10
    two_doubleinvbutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv,twiddle_ptr
    two_doubleinvbutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv,twiddle_ptr

    vmov twiddle, s11
    vmov twiddle_ptr, s12 //twiddle_ptr不再用作索引，用于存储n1,n2
    vmov s22, poly
    doubleinvbutterfly_first_level poly0, poly4, twiddle, tmp, tmp2, q, qinv, twiddle_ptr,poly,s23
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly1, poly5, twiddle, tmp, tmp2, q, qinv, twiddle_ptr,poly,s23
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly2, poly6, twiddle, tmp, tmp2, q, qinv, twiddle_ptr,poly,s23
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly3, poly7, twiddle, tmp, tmp2, q, qinv, twiddle_ptr,poly,s23
    vmov poly, s22
    
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4
  .endr

  vpop.w {s16-s24}
  pop {r4-r11, pc}
.size invntt_fast_fpu, .-invntt_fast_fpu