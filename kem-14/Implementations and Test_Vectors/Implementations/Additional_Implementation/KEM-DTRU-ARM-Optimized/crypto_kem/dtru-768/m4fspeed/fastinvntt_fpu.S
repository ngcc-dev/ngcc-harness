#include "macros.i"
.syntax unified
.cpu cortex-m4
.thumb

.macro mul_twiddle tb, a, twiddle, tmp, tmp2, q, qinv
    smulb\tb \tmp, \a, \twiddle
    smult\tb \a, \a, \twiddle
    montgomery \q, \qinv, \tmp, \tmp2 // reduce -> result in tmp2
    montgomery \q, \qinv, \a, \tmp // reduce -> result in tmp
    pkhtb \a, \tmp, \tmp2, asr#16 // combine results from above in one register as 16bit halves
.endm


.macro doubleinvbutterfly_last_level a0, a1, a2, twiddle, tmp, tmp2, q, qinv, tmp3, root3
  usub16 \tmp, \a1, \a2
  mul_twiddle b, \tmp, \root3, \tmp2, \tmp3, \q, \qinv //tmp=tpho
  usub16 \tmp2, \a0, \a1
  usub16 \tmp2, \tmp2, \tmp //tmp2=tb
  usub16 \tmp3, \a0, \a2
  uadd16 \tmp3, \tmp3, \tmp //tmp3=tc

  uadd16 \a0, \a0, \a1
  uadd16 \a0, \a0, \a2

  smulbb \tmp, \tmp2, \twiddle
  smultb \a1, \tmp2, \twiddle
  montgomery \q, \qinv, \tmp, \tmp2
  montgomery \q, \qinv, \a1, \tmp
  pkhtb \a1, \tmp, \tmp2, asr#16 

  smulbt \tmp, \tmp3, \twiddle
  smultt \a2, \tmp3, \twiddle
  montgomery \q, \qinv, \tmp, \tmp2
  montgomery \q, \qinv, \a2, \tmp
  pkhtb \a2, \tmp, \tmp2, asr#16 
.endm

.macro doubleinvbutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv
  usub16 \tmp, \a0, \a1
  uadd16 \a0, \a0, \a1
  smulb\tb \a1, \tmp, \twiddle
  smult\tb \tmp, \tmp, \twiddle
  montgomery \q, \qinv, \a1, \tmp2
  montgomery \q, \qinv, \tmp, \a1
  pkhtb \a1, \a1, \tmp2, asr#16
.endm

.macro two_doubleinvbutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv
  doubleinvbutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv
  doubleinvbutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv
.endm


.macro _3_layer_double_GS_16_fp_reduce1 c0, c1, c2, c3, c4, c5, c6, c7, xi0, xi1, xi2, xi3, twiddle, q, qinv, tmp, tmp2, xi4, xi5, xi6
  vmov \twiddle, \xi0
  two_doubleinvbutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
  vmov \twiddle, \xi1
  two_doubleinvbutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv
  
  vmov.w \twiddle, \xi4// barrettconst
  vmov.w \xi6, \c7 //暂存c7
  vmov.w \c7, \xi5 //2^15
  doublebarrett_fast \c0, \tmp, \tmp2, \q, \twiddle, \c7
  doublebarrett_fast \c2, \tmp, \tmp2, \q, \twiddle, \c7
  doublebarrett_fast \c4, \tmp, \tmp2, \q, \twiddle, \c7
  doublebarrett_fast \c6, \tmp, \tmp2, \q, \twiddle, \c7
  vmov.w \c7, \xi6

  vmov \twiddle, \xi2
  two_doubleinvbutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
  two_doubleinvbutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv

  vmov \twiddle, \xi3
  two_doubleinvbutterfly b, b, \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv
  two_doubleinvbutterfly b, b, \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv
.endm


.macro doubleinvbutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv, t1
  usub16 \tmp, \a0, \a1
  smulbt \tmp2, \tmp, \twiddle
  smultt \tmp, \tmp, \twiddle
  montgomery \q, \qinv, \tmp2, \twiddle // twiddle is used as temp register
  montgomery \q, \qinv, \tmp, \tmp2
  pkhtb \tmp2, \tmp2, \twiddle, asr#16 // tmp2 = t
  uadd16 \a0, \a0, \a1 
  usub16 \a0, \a0, \tmp2
  mul_twiddle b, \a0, \t1, \tmp, \a1, \q, \qinv
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

  movw barrettconst, #0x0ae3
  movt barrettconst, #0xffed
  vmov s17, barrettconst
  movw barrettconst, #32768 //2^15
  vmov s18, barrettconst
  movw barrettconst, #19412
  vmov s19, barrettconst //barrett_init
  
  
  ### the last level
  movw root3, #1033
#ifdef USE_REPT
  .rept 64
#else
  add.w tmp, poly, #1536
  vmov s24, tmp //s24:1536
  1:
#endif
    ldm poly, {poly0-poly5}
    @ ldr.w poly0, [poly]
    @ ldr.w poly1, [poly, #4]
    @ ldr.w poly2, [poly, #8]
    @ ldr.w poly3, [poly, #12]
    @ ldr.w poly4, [poly, #16]
    @ ldr.w poly5, [poly, #20]

    ldr.w twiddle, [twiddle_ptr], #4 //twiddle(t,b)=(zeta2,zeta1) 
    // (a0,a1) (a2,a3) (a4,a5)
    doubleinvbutterfly_last_level poly0, poly1, poly2, twiddle, tmp, tmp2, q, qinv, tmp3, root3

    ldr.w twiddle, [twiddle_ptr], #4
    // (a6,a7) (a8,a9) (a10,a11)
    doubleinvbutterfly_last_level poly3, poly4, poly5, twiddle, tmp, tmp2, q, qinv, tmp3, root3

    stm poly!, {poly0-poly5}
    @ str.w poly0, [poly], #4
    @ str.w poly1, [poly], #4
    @ str.w poly2, [poly], #4
    @ str.w poly3, [poly], #4
    @ str.w poly4, [poly], #4
    @ str.w poly5, [poly], #4

#ifdef USE_REPT
  .endr
#else
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 1b
#endif

  sub.w poly, #1536
  
  ### LAYER 7+6+5+4
  .equ distance, 48
  .equ offset, 96
  .equ strincr, 96

#ifdef USE_REPT
  .rept 8
#else
  2:
#endif

    vldm twiddle_ptr!, {s8-s15} //8 float registers, 4+2+1+空 4+2+1+1

#ifdef USE_REPT
    .rept 3
#else
    add.w tmp2, poly, #distance/4 //12
    vmov s23, tmp2
    3:
#endif

      // load a0, a2, ..., a14
      // a0, a6, a12, a18
      load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
      // a24, a30, a36, a42
      load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

      //s17:barrettconst s18:2^15 s20:中间值
      _3_layer_double_GS_16_fp_reduce1 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, s17, s18, s20

      vmov s0, poly0 //a0
      vmov s1, poly1 //a2
      vmov s2, poly2 //a4
      vmov s3, poly3 //a6
      vmov s4, poly4 //a8
      vmov s5, poly5 //a10
      vmov s6, poly6 //a12
      vmov s7, poly7 //a14

      // load a1, a3, ..., a15
      // a48, a54, a60, a66
      load poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
      // a72, a78, a84, a90
      load poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset

      _3_layer_double_GS_16_fp_reduce1 poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s12, s13, s14, s15, twiddle, q, qinv, tmp, tmp2, s17, s18, s20

      // addsub: (a2, a6, a10, a14), (a3, a7, a11, a15)
      vmov twiddle, s15

      vmov tmp2, s1 // load a2
      uadd16 tmp, tmp2, poly1
      usub16 poly1, tmp2, poly1
      vmov s20, poly //s20:暂存poly
      mul_twiddle t, poly1, twiddle, poly, tmp2, q, qinv
      vmov poly, s20
      str.w poly1, [poly, #1*distance/4+offset]
      vmov poly1, s19 //poly1里放的是barrett_init constant
      vmov s20, poly //s20:暂存poly
      doublebarrett_init tmp, poly, tmp2, q, poly1
      vmov poly, s20
      str.w tmp, [poly, #1*distance/4]

      vmov tmp2, s3 // load a6
      uadd16 tmp, tmp2, poly3
      usub16 poly3, tmp2, poly3
      mul_twiddle t, poly3, twiddle, poly1, tmp2, q, qinv
      str.w poly3, [poly, #3*distance/4+offset]
      doublebarrett_init tmp, poly3, tmp2, q, poly1
      str.w tmp, [poly, #3*distance/4]

      vmov tmp2, s5 // load a10
      uadd16 tmp, tmp2, poly5
      usub16 poly5, tmp2, poly5
      mul_twiddle t, poly5, twiddle, poly3, tmp2, q, qinv
      str.w poly5, [poly, #5*distance/4+offset]
      vmov poly1, s17
      vmov poly3, s18
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w tmp, [poly, #5*distance/4]

      vmov tmp2, s7 // load a14
      uadd16 tmp, tmp2, poly7
      usub16 poly7, tmp2, poly7
      mul_twiddle t, poly7, twiddle, poly5, tmp2, q, qinv
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w poly7, [poly, #7*distance/4+offset]
      str.w tmp, [poly, #7*distance/4]

      // addsub: (a0, a4, a8, a12), (a1, a5, a9, a13)
      vmov poly5, s2 // load a4
      uadd16 tmp, poly5, poly2
      usub16 poly2, poly5, poly2
      mul_twiddle t, poly2, twiddle, poly5, tmp2, q, qinv
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w poly2, [poly, #2*distance/4+offset]  
      str.w tmp, [poly, #2*distance/4]

      vmov poly5, s4 // load a8
      uadd16 tmp, poly5, poly4
      usub16 poly4, poly5, poly4
      mul_twiddle t, poly4, twiddle, poly5, tmp2, q, qinv 
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w poly4, [poly, #4*distance/4+offset] 
      str.w tmp, [poly, #4*distance/4]
      
      vmov poly7, s6 // load a12
      uadd16 tmp, poly7, poly6
      usub16 poly6, poly7, poly6
      mul_twiddle t, poly6, twiddle, poly5, tmp2, q, qinv
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w poly6, [poly, #6*distance/4+offset]  
      str.w tmp, [poly, #6*distance/4]

      vmov poly5, s0 // load a0
      uadd16 tmp, poly5, poly0
      usub16 poly0, poly5, poly0
      mul_twiddle t, poly0, twiddle, poly5, tmp2, q, qinv
      doublebarrett_fast tmp, poly5, tmp2, q, poly1, poly3
      str.w poly0, [poly, #offset]   
      str.w tmp, [poly], #4
#ifdef USE_REPT
    .endr
#else
      vmov tmp2, s23
      cmp.w poly, tmp2
    bne.w 3b
#endif

    add poly, #(96-6)*2
#ifdef USE_REPT
  .endr
#else   
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 2b
#endif


  ### LAYER 3+2 + the first level
  .equ distance, 768
  sub.w poly, #1536
  vldm twiddle_ptr, {s8-s12} // 5 float registers, 10 zetas
#ifdef USE_REPT
  .rept 48
#else   
  add.w tmp, poly, #192
  vmov s24, tmp
  4:
#endif
    // a0,   a96,  a192, a288
    // a384, a480, a576, a672
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    vmov twiddle, s8
    two_doubleinvbutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv
    vmov twiddle, s9
    two_doubleinvbutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv

    vmov twiddle, s10
    two_doubleinvbutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
    two_doubleinvbutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv

    vmov twiddle, s11
    vmov twiddle_ptr, s12 //twiddle_ptr不再用作索引，用于存储n1,n2
    doubleinvbutterfly_first_level poly0, poly4, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly1, poly5, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly2, poly6, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
    vmov twiddle, s11
    doubleinvbutterfly_first_level poly3, poly7, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
    
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

#ifdef USE_REPT
  .endr
#else
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 4b
#endif

  vpop.w {s16-s24}
  pop {r4-r11, pc}
