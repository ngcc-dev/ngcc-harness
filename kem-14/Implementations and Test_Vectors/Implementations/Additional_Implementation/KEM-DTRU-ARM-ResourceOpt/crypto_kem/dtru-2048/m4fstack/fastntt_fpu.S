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

.macro doublebutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
  smulb\tb \tmp, \a1, \twiddle // a1_b * twiddle_tb
  smult\tb \a1, \a1, \twiddle // a1_t * twiddle_tb
  montgomery \q, \qinv, \tmp, \tmp2 // reduce -> result in tmp2
  montgomery \q, \qinv, \a1, \tmp // reduce -> result in tmp
  pkhtb \tmp2, \tmp, \tmp2, asr#16 // combine results from above in one register as 16bit halves
  usub16 \a1, \a0, \tmp2 // a0 - a1 * twiddle (a0, a1 contain 2 coeffs)
  uadd16 \a0, \a0, \tmp2 // a0 + a1 * twiddle (a0, a1 contain 2 coeffs)
  movw \twiddle_ptr, #19412
  doublebarrett_init \a1, \tmp, \tmp2, \q, \twiddle_ptr
  doublebarrett_init \a0, \tmp, \tmp2, \q, \twiddle_ptr
.endm

.macro two_doublebutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
  doublebutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
  doublebutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
.endm

.macro _3_layer_double_CT_16_fp c0, c1, c2, c3, c4, c5, c6, c7, xi01, xi23, xi45, xi67, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    // layer 3
    vmov \twiddle, \xi01
    two_doublebutterfly t, t, \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr //用vmov传进来的twiddle，第0个是0，用不到，所以是t,t
    two_doublebutterfly t, t, \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr 

    // layer 2
    vmov \twiddle, \xi23
    two_doublebutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
    two_doublebutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr

    // layer 1
    vmov \twiddle, \xi45
    two_doublebutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
    vmov \twiddle, \xi67
    two_doublebutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
.endm

.macro doublebutterfly_first_level_k a0, a1, twiddle, tmp, tmp2, q, qinv
  smulbb \tmp, \a1, \twiddle
  smultb \tmp2, \a1, \twiddle
  montgomery \q, \qinv, \tmp, \twiddle // twiddle is used as temp register
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \tmp2, \tmp, \twiddle, asr#16
  uadd16 \a1, \a1, \a0
  usub16 \a1, \a1, \tmp2
  uadd16 \a0, \a0, \tmp2
.endm

.macro doublebutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
    /*smulbb \tmp, \a1, \twiddle
    smultb \tmp2, \a1, \twiddle
    pkhbt \tmp, \tmp, \tmp2, lsl#16
    uadd16 \a1, \a1, \a0
    usub16 \a1, \a1, \tmp2
    uadd16 \a0, \a0, \tmp2*/

    smulbt \tmp, \a1, \twiddle //用vmov传进来的twiddle，第0个是0，用不到
    smultt \tmp2, \a1, \twiddle 
    montgomery \q, \qinv, \tmp, \twiddle_ptr // twiddle_ptr is used as temp register
    montgomery \q, \qinv, \tmp2, \tmp
    pkhtb \tmp2, \tmp, \twiddle_ptr, asr#16
    uadd16 \a1, \a1, \a0
    usub16 \a1, \a1, \tmp2
    uadd16 \a0, \a0, \tmp2
    movw \twiddle_ptr, #19412
    doublebarrett_init \a1, \tmp, \tmp2, \q, \twiddle_ptr
    doublebarrett_init \a0, \tmp, \tmp2, \q, \twiddle_ptr
.endm

.macro two_doublebutterfly_first_level a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv, twiddle_ptr
  doublebutterfly_first_level \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr 
  doublebutterfly_first_level \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr 
.endm

.macro _3_layer_double_CT_16_fp_first_layer c0, c1, c2, c3, c4, c5, c6, c7, xi0, xi1, xi2, xi3, twiddle, q, qinv, tmp, tmp2, twiddle_ptr 
		// layer 3
		vmov \twiddle, \xi0
		// twiddle here acts as a tmp register
    two_doublebutterfly_first_level \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr 
    two_doublebutterfly_first_level \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr 

		// layer 2
		vmov \twiddle, \xi1
    two_doublebutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
    two_doublebutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr

		// layer 1
		vmov \twiddle, \xi2
    two_doublebutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
		vmov \twiddle, \xi3
    two_doublebutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv, \twiddle_ptr
.endm


.global ntt_fast_fpu
.type ntt_fast_fpu, %function
.align 2
ntt_fast_fpu:
  push {r4-r11, r14}
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
  poly7       .req r9
  twiddle     .req r10
  barrettconst .req r10
  qinv        .req r11
  q           .req r11
  tmp         .req r12
  tmp2        .req r14

  movw q, #3457
  movt qinv, #52607 //-q^(-1),Montgomery Reduction

  .equ distance, 2048
  .equ offset, 256
  .equ strincr, 4
  vldm twiddle_ptr!, {s8-s15}
  vmov s23, twiddle_ptr // s23: twiddle addr
#ifdef USE_REPT
  .rept 64
#else
  movw tmp, #64
  1:
    push {tmp}
#endif

    load poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
    load poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset
    _3_layer_double_CT_16_fp_first_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr

    // multiply coeffs by layer 8 twiddles for later use
    vmov twiddle, s12 
    mul_twiddle b, poly0, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly1, twiddle, tmp, tmp2, q, qinv

    vmov twiddle, s13
    mul_twiddle b, poly2, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly3, twiddle, tmp, tmp2, q, qinv

    vmov twiddle, s14
    mul_twiddle b, poly4, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly5, twiddle, tmp, tmp2, q, qinv

    vmov twiddle, s15
    mul_twiddle b, poly6, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly7, twiddle, tmp, tmp2, q, qinv

    vmov s0, poly0 // a1
    vmov s1, poly1 // a3
    vmov s2, poly2 // a5
    vmov s3, poly3 // a7
    vmov s4, poly4 // a9
    vmov s5, poly5 // a11
    vmov s6, poly6 // a13
    vmov s7, poly7 // a15

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp_first_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
     
    vmov tmp2, s1 // load a3
    uadd16 tmp, poly1, tmp2
    usub16 poly1, poly1, tmp2
    movw twiddle_ptr, #19412
    vmov s24, poly3
    doublebarrett_init tmp, poly3, tmp2, q, twiddle_ptr
    doublebarrett_init poly1, poly3, tmp2, q, twiddle_ptr
    vmov poly3, s24
    str.w tmp, [poly, #1*distance/4]
    str.w poly1, [poly, #1*distance/4+offset]

    vmov tmp2, s3 // load a7
    uadd16 tmp, poly3, tmp2
    usub16 poly3, poly3, tmp2
    doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
    doublebarrett_init poly3, poly1, tmp2, q, twiddle_ptr
    str.w tmp, [poly, #3*distance/4]
    str.w poly3, [poly, #3*distance/4+offset]
    
    vmov tmp2, s5 // load a11
    uadd16 tmp, poly5, tmp2
    usub16 poly5, poly5, tmp2
    doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
    doublebarrett_init poly5, poly1, tmp2, q, twiddle_ptr
    str.w tmp, [poly, #5*distance/4]
    str.w poly5, [poly, #5*distance/4+offset]
    
    vmov tmp2, s7 // load a15
    uadd16 tmp, poly7, tmp2
    usub16 poly7, poly7, tmp2
    doublebarrett_init tmp, poly1, tmp2, q, twiddle_ptr
    doublebarrett_init poly7, poly1, tmp2, q, twiddle_ptr
    str.w tmp, [poly, #7*distance/4]
    str.w poly7, [poly, #7*distance/4+offset]
    
    // layer 5 - 2    
    // addsub: (a0, a4, a8, a12), (a1, a5, a9, a13)
    vmov poly3, s2 // load a5
    uadd16 tmp, poly2, poly3
    usub16 tmp2, poly2, poly3
    doublebarrett_init tmp, poly5, poly7, q, twiddle_ptr
    doublebarrett_init tmp2, poly5, poly7, q, twiddle_ptr
    str.w tmp, [poly, #2*distance/4]
    str.w tmp2, [poly, #2*distance/4+offset]

    vmov poly5, s4 // load a9
    uadd16 tmp, poly4, poly5
    usub16 tmp2, poly4, poly5
    doublebarrett_init tmp, poly5, poly7, q, twiddle_ptr
    doublebarrett_init tmp2, poly5, poly7, q, twiddle_ptr
    str.w tmp, [poly, #4*distance/4]
    str.w tmp2, [poly, #4*distance/4+offset]

    vmov poly7, s6 // load a13
    uadd16 tmp, poly6, poly7
    usub16 tmp2, poly6, poly7
    doublebarrett_init tmp, poly5, poly7, q, twiddle_ptr
    doublebarrett_init tmp2, poly5, poly7, q, twiddle_ptr
    str.w tmp, [poly, #6*distance/4]
    str.w tmp2, [poly, #6*distance/4+offset]
    
    vmov poly1, s0 // load a1
    uadd16 tmp, poly0, poly1
    usub16 tmp2, poly0, poly1
    doublebarrett_init tmp, poly5, poly7, q, twiddle_ptr
    doublebarrett_init tmp2, poly5, poly7, q, twiddle_ptr
    str.w tmp2, [poly, #offset]    
    str.w tmp, [poly], #4
#ifdef USE_REPT
  .endr
#else
    pop {tmp}
    subs.w tmp, #1
  bne.w 1b
#endif

  vmov twiddle_ptr, s23

  sub.w poly, #64*strincr 
  .equ distance, distance/16
  .equ offset, 16

#ifdef USE_REPT
  .rept 16
#else
  movw tmp, #16
  2:
    push {tmp}
#endif
    vldm twiddle_ptr!, {s8-s11}
    vmov s23, twiddle_ptr // s23: twiddle addr

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

  

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2, twiddle_ptr
    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #(256-4*7)
    vmov twiddle_ptr, s23
#ifdef USE_REPT
  .endr
#else
    pop {tmp}
    subs.w tmp, #1
  bne.w 2b
#endif

  vpop.w {s16-s24}
  pop.w {r4-r11, pc}
.size ntt_fast_fpu, .-ntt_fast_fpu