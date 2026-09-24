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

.macro doublebutterfly tb, a0, a1, twiddle, tmp, tmp2, q, qinv
  smulb\tb \tmp, \a1, \twiddle // a1_b * twiddle_tb
  smult\tb \a1, \a1, \twiddle // a1_t * twiddle_tb
  montgomery \q, \qinv, \tmp, \tmp2 // reduce -> result in tmp2
  montgomery \q, \qinv, \a1, \tmp // reduce -> result in tmp
  pkhtb \tmp2, \tmp, \tmp2, asr#16 // combine results from above in one register as 16bit halves
  usub16 \a1, \a0, \tmp2 // a0 - a1 * twiddle (a0, a1 contain 2 coeffs)
  uadd16 \a0, \a0, \tmp2 // a0 + a1 * twiddle (a0, a1 contain 2 coeffs)
.endm

.macro two_doublebutterfly tb1, tb2, a0, a1, a2, a3, twiddle, tmp, tmp2, q, qinv
  doublebutterfly \tb1, \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv
  doublebutterfly \tb2, \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv
.endm

@ .macro _3_layer_double_CT_16 c0, c1, c2, c3, c4, c5, c6, c7, twiddle, twiddle_ptr, q, qinv, tmp, tmp2
@     // layer 3
@     ldrh.w \twiddle, [\twiddle_ptr]
@     two_doublebutterfly b, b, \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv
@     two_doublebutterfly b, b, \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv

@     // layer 2
@     ldr.w \twiddle, [\twiddle_ptr, #2]
@     two_doublebutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
@     two_doublebutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv

@     // layer 1
@     ldr.w \twiddle, [\twiddle_ptr, #6]
@     two_doublebutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
@     ldr.w \twiddle, [\twiddle_ptr, #10]
@     two_doublebutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv
@ .endm

.macro _3_layer_double_CT_16_fp c0, c1, c2, c3, c4, c5, c6, c7, xi01, xi23, xi45, xi67, twiddle, q, qinv, tmp, tmp2
    // layer 3
    vmov \twiddle, \xi01
    two_doublebutterfly t, t, \c0, \c4, \c1, \c5, \twiddle, \tmp, \tmp2, \q, \qinv //用vmov传进来的twiddle，第0个是0，用不到，所以是t,t
    two_doublebutterfly t, t, \c2, \c6, \c3, \c7, \twiddle, \tmp, \tmp2, \q, \qinv 

    // layer 2
    vmov \twiddle, \xi23
    two_doublebutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
    two_doublebutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv

    // layer 1
    vmov \twiddle, \xi45
    two_doublebutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
    vmov \twiddle, \xi67
    two_doublebutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv
.endm

.macro doublebutterfly_first_level a0, a1, twiddle, tmp, tmp2, q, qinv
    smulbb \tmp2, \a1, \twiddle
    smultb \tmp, \a1, \twiddle 
    montgomery \q, \qinv, \tmp, \twiddle
    montgomery \q, \qinv, \tmp2, \tmp
    pkhtb \tmp, \twiddle, \tmp, asr#16
    uadd16 \a1, \a1, \a0
    usub16 \a1, \a1, \tmp
    uadd16 \a0, \a0, \tmp
.endm

.macro two_doublebutterfly_first_level a0, a1, a2, a3, xi, twiddle, tmp, tmp2, q, qinv
  vmov \twiddle, \xi
  doublebutterfly_first_level \a0, \a1, \twiddle, \tmp, \tmp2, \q, \qinv
  vmov \twiddle, \xi
  doublebutterfly_first_level \a2, \a3, \twiddle, \tmp, \tmp2, \q, \qinv
.endm


.macro _3_layer_double_CT_16_fp_first_layer c0, c1, c2, c3, c4, c5, c6, c7, xi0, xi1, xi2, xi3, twiddle, q, qinv, tmp, tmp2
		// layer 3
		// twiddle here acts as a tmp register
    two_doublebutterfly_first_level \c0, \c4, \c1, \c5, \xi0, \twiddle, \tmp, \tmp2, \q, \qinv
    two_doublebutterfly_first_level \c2, \c6, \c3, \c7, \xi0, \twiddle, \tmp, \tmp2, \q, \qinv

		// layer 2
		vmov \twiddle, \xi1
    two_doublebutterfly b, b, \c0, \c2, \c1, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
    two_doublebutterfly t, t, \c4, \c6, \c5, \c7, \twiddle, \tmp, \tmp2, \q, \qinv

		// layer 1
		vmov \twiddle, \xi2
    two_doublebutterfly b, t, \c0, \c1, \c2, \c3, \twiddle, \tmp, \tmp2, \q, \qinv
		vmov \twiddle, \xi3
    two_doublebutterfly b, t, \c4, \c5, \c6, \c7, \twiddle, \tmp, \tmp2, \q, \qinv
.endm

// the last level
.macro doublebutterfly_last_level a0, a1, a2, twiddle, tmp, tmp2, q, qinv, tmp3, root3
  smulbb \tmp, \a1, \twiddle
  smultb \tmp2, \a1, \twiddle
  montgomery \q, \qinv, \tmp, \tmp3
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \a1, \tmp, \tmp3, asr#16 //a1=tb
  smulbt \tmp, \a2, \twiddle
  smultt \tmp2, \a2, \twiddle
  montgomery \q, \qinv, \tmp, \tmp3
  montgomery \q, \qinv, \tmp2, \tmp
  pkhtb \a2, \tmp, \tmp3, asr#16 //a2=tc
  usub16 \tmp, \a1, \a2 
  smulbb \tmp2, \tmp, \root3
  smultb \tmp3, \tmp, \root3
  montgomery \q, \qinv, \tmp2, \tmp
  montgomery \q, \qinv, \tmp3, \tmp2
  pkhtb \tmp, \tmp2, \tmp, asr#16 //tmp=tpho
  uadd16 \tmp2, \a1, \tmp //tmp2=tb+tpho
  usub16 \tmp3, \a2, \tmp //tmp3=tc-tpho
  uadd16 \tmp, \a1, \a2 //tmp=tb+tc
  usub16 \a2, \a0, \tmp2
  usub16 \a1, \a0, \tmp3
  uadd16 \a0, \a0, \tmp
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
  root3       .req r8 // new add
  poly7       .req r9
  tmp3        .req r9 // new add
  twiddle     .req r10
  barrettconst .req r10
  qinv        .req r11
  q           .req r11
  tmp         .req r12
  tmp2        .req r14

  movw q, #3457
  movt qinv, #52607 //-q^(-1),Montgomery Reduction
  movw barrettconst, #0x0ae3
  movt barrettconst, #0xffed //-(2^32/q)
  vmov s17, barrettconst
  movw barrettconst, #32768 //2^15
  vmov s18, barrettconst

  ### the first level + LAYER 7+6+5
  .equ distance, 1536
  .equ offset, 192
  .equ strincr, 4 //4*48=192
  // pre-load 15 twiddle factors to 15 FPU registers
  // s0-s7 used to temporary store 16 16-bit polys.
  vldm twiddle_ptr!, {s8-s15} //8 float registers, zetas_fpu[0]=0 + 15 twiddles
@ vmov s23, twiddle_ptr // s23: twiddle addr
#ifdef USE_REPT
  .rept 48
#else
  add tmp, poly, #strincr*48 //192
  // s24: tmp  
  vmov s24, tmp  
  1:
#endif

    // a0:(a0,...,a95) // each group has 96 coefficients
    // a1:(a48,...,a95)
    // a2:(a96,...,a143)
    // ... ... ... ...

    // load a1, a3, ..., a15
    // a48, a144, a240, a336
    load poly, poly0, poly1, poly2, poly3, #offset, #distance/4+offset, #2*distance/4+offset, #3*distance/4+offset
    // a432, a528, a624, a720
    load poly, poly4, poly5, poly6, poly7, #distance+offset, #5*distance/4+offset, #6*distance/4+offset, #7*distance/4+offset

    // 8-NTT on a1, a3, ..., a15
    _3_layer_double_CT_16_fp_first_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

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

    // load a0, a2, ..., a14
    // a0, a96, a192, a288
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    // a384, a480, a576, a672
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    
    // 8-NTT on a0, a2, ..., a14
    _3_layer_double_CT_16_fp_first_layer poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2 
    
    // layer 5 - 1
    // addsub: (a2, a6, a10, a14), (a3, a7, a11, a15)
    vmov tmp2, s1 // load a3
    uadd16 tmp, poly1, tmp2
    usub16 poly1, poly1, tmp2
    str.w tmp, [poly, #1*distance/4]
    str.w poly1, [poly, #1*distance/4+offset]

    vmov tmp2, s3 // load a7
    uadd16 tmp, poly3, tmp2
    usub16 poly3, poly3, tmp2
    str.w tmp, [poly, #3*distance/4]
    str.w poly3, [poly, #3*distance/4+offset]
    
    vmov tmp2, s5 // load a11
    uadd16 tmp, poly5, tmp2
    usub16 poly5, poly5, tmp2
    str.w tmp, [poly, #5*distance/4]
    str.w poly5, [poly, #5*distance/4+offset]
    
    vmov tmp2, s7 // load a15
    uadd16 tmp, poly7, tmp2
    usub16 poly7, poly7, tmp2
    str.w tmp, [poly, #7*distance/4]
    str.w poly7, [poly, #7*distance/4+offset]
    
    // layer 5 - 2    
    // addsub: (a0, a4, a8, a12), (a1, a5, a9, a13)
    vmov poly3, s2 // load a5
    uadd16 tmp, poly2, poly3
    usub16 tmp2, poly2, poly3
    str.w tmp, [poly, #2*distance/4]
    str.w tmp2, [poly, #2*distance/4+offset]

    vmov poly5, s4 // load a9
    uadd16 tmp, poly4, poly5
    usub16 tmp2, poly4, poly5
    str.w tmp, [poly, #4*distance/4]
    str.w tmp2, [poly, #4*distance/4+offset]

    vmov poly7, s6 // load a13
    uadd16 tmp, poly6, poly7
    usub16 tmp2, poly6, poly7
    str.w tmp, [poly, #6*distance/4]
    str.w tmp2, [poly, #6*distance/4+offset]
    
    vmov poly1, s0 // load a1
    uadd16 tmp, poly0, poly1
    usub16 tmp2, poly0, poly1
    str.w tmp2, [poly, #offset]    
    str.w tmp, [poly], #4

#ifdef USE_REPT
  .endr
#else
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 1b
#endif

	sub.w poly, #48*strincr
@ vmov twiddle_ptr, s23
  ### LAYER 4+3+2
  .equ distance, distance/16  //96
  .equ offset, 12
  .equ strincr, 192
#ifdef USE_REPT
  .rept 16
#else
  add.w tmp, poly, #16*strincr
  vmov s24, tmp
  2:
#endif
    vldm twiddle_ptr!, {s8-s11}
    // a0, a12, a24, a36
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    // a48, a60, a72, a84
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4


    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    _3_layer_double_CT_16_fp poly0, poly1, poly2, poly3, poly4, poly5, poly6, poly7, s8, s9, s10, s11, twiddle, q, qinv, tmp, tmp2

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #172

#ifdef USE_REPT
  .endr
#else
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 2b
#endif

  sub.w poly, #16*strincr
  ### the last level
  movw root3, #1033 //root3和poly6是一个寄存器，最后一层不用poly6
#ifdef USE_REPT
  .rept 128
#else
  add.w tmp, poly, #3072
  vmov s24, tmp
  4:
#endif
    ldm poly, {poly0-poly5}
    
    ldr.w twiddle, [twiddle_ptr], #4 //twiddle(t,b)=(zeta2,zeta1) 
    // (a0,a1) (a4,a5) (a8,a9)
    doublebutterfly_last_level poly0, poly2, poly4, twiddle, tmp, tmp2, q, qinv, tmp3, root3

    // (a2,a3) (a6,a7) (a10,a11)
    doublebutterfly_last_level poly1, poly3, poly5, twiddle, tmp, tmp2, q, qinv, tmp3, root3

    @ vmov barrettconst, s17 //barrettconst1
    @ vmov tmp3, s18 //barrettconst2 tmp3和poly7是一个寄存器
    @ doublebarrett_fast poly0, tmp, tmp2, q, barrettconst, tmp3
    @ doublebarrett_fast poly1, tmp, tmp2, q, barrettconst, tmp3
    @ doublebarrett_fast poly2, tmp, tmp2, q, barrettconst, tmp3
    @ doublebarrett_fast poly3, tmp, tmp2, q, barrettconst, tmp3
    @ doublebarrett_fast poly4, tmp, tmp2, q, barrettconst, tmp3
    @ doublebarrett_fast poly5, tmp, tmp2, q, barrettconst, tmp3

    stm poly!, {poly0-poly5}

#ifdef USE_REPT
  .endr
#else
    vmov tmp, s24
    cmp.w poly, tmp
  bne.w 4b
#endif 
  
  vpop.w {s16-s24}
  pop {r4-r11, pc}
