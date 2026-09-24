#include "macros.i"
#include "macros_ntt.i"

.syntax unified
.cpu cortex-m4
.thumb

.global ntt_fast
.type ntt_fast, %function
.align 2
ntt_fast:
  push {r4-r11, r14}

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

  movw q, #769
  movt qinv, #767 //-q^(-1),Montgomery Reduction
  .equ barrett_constant, 21817 //doublebarret_init
  .equ distance, 512

  vmov s17, barrettconst
  movw barrettconst, #32768 //2^15
  vmov s18, barrettconst


### the first level + 5+4
#ifdef USE_REPT
  .rept 32
#else
  movw tmp, #32
  1:
    push {tmp}
#endif

    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    ldrh twiddle, [twiddle_ptr]
    two_doublebutterfly b, b, poly0, poly4, poly1, poly5, twiddle, tmp,tmp2,q, qinv
    two_doublebutterfly b, b, poly2, poly6, poly3, poly7, twiddle, tmp,tmp2,q, qinv


    ldr.w twiddle, [twiddle_ptr, #2]
    two_doublebutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
    two_doublebutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv


    ldr.w twiddle, [twiddle_ptr, #6]
    two_doublebutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv


    ldr.w twiddle, [twiddle_ptr, #10]
    two_doublebutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #4

#ifdef USE_REPT
  .endr
#else
    pop {tmp}
    subs.w tmp, #1
  bne.w 1b
#endif
  sub.w poly, #128
  add.w twiddle_ptr, #14
  .equ distance, 64




  ### LAYER 3+2+1
  // .rept 8
#ifdef USE_REPT
    .rept 8
#else
    movw tmp, #8
    2:
    push {tmp}
#endif

#ifdef USE_REPT
    .rept 4
#else
    movw tmp, #4
    3:
      push {tmp}
#endif
      // a0,  a12, a24, a36
      // a48, a60, a72, a84
      load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
      load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4


      ldrh twiddle, [twiddle_ptr]
      two_doublebutterfly b, b, poly0, poly4, poly1, poly5, twiddle, tmp, tmp2, q, qinv
      two_doublebutterfly b, b, poly2, poly6, poly3, poly7, twiddle, tmp, tmp2, q, qinv



      ldr.w twiddle, [twiddle_ptr, #2]
      two_doublebutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
      two_doublebutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv


      ldr.w twiddle, [twiddle_ptr, #6]
      two_doublebutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv


      ldr.w twiddle, [twiddle_ptr, #10]
      two_doublebutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv

//    vmov s0,poly
//    vmov barrettconst, s17 //barrettconst1
//    vmov poly, s18 //barrettconst2 tmp3和poly7是一个寄存器
//    doublebarrett_fast poly0, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly1, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly2, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly3, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly4, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly5, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly6, tmp, tmp2, q, barrettconst, poly
//    doublebarrett_fast poly7, tmp, tmp2, q, barrettconst, poly
//    vmov poly,s0
      movw barrettconst, #21817
      doublebarrett_init poly0, tmp, tmp2, q, barrettconst
      doublebarrett_init poly1, tmp, tmp2, q, barrettconst
      doublebarrett_init poly2, tmp, tmp2, q, barrettconst
      doublebarrett_init poly3, tmp, tmp2, q, barrettconst
      doublebarrett_init poly4, tmp, tmp2, q, barrettconst
      doublebarrett_init poly5, tmp, tmp2, q, barrettconst
      doublebarrett_init poly6, tmp, tmp2, q, barrettconst
      doublebarrett_init poly7, tmp, tmp2, q, barrettconst

      store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
      str.w poly1, [poly, #distance/4]
      str.w poly2, [poly, #2*distance/4]
      str.w poly3, [poly, #3*distance/4]
      str.w poly0, [poly], #4

#ifdef USE_REPT
    .endr
#else
      pop {tmp}
      subs.w tmp, #1
      bne.w 3b
#endif
    add.w poly, #112
    add.w twiddle_ptr, #14
//.endr
#ifdef USE_REPT
    .endr
#else
    pop {tmp}
    subs.w tmp, #1
    bne.w 2b
#endif


.equ distance, 16 // 将distance从64改为16
sub.w poly, #1024 //将poly的地址减1024，回到第一个参数的位置


#ifdef USE_REPT
    .rept 32
#else
    movw tmp, #32
    4:
    push {tmp}
#endif
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    ldr.w twiddle, [twiddle_ptr]
    two_doublebutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
    two_doublebutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv

    store poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4
    str.w poly1, [poly, #distance/4]
    str.w poly2, [poly, #2*distance/4]
    str.w poly3, [poly, #3*distance/4]
    str.w poly0, [poly], #32

    add.w twiddle_ptr, #4

#ifdef USE_REPT
    .endr
#else
    pop {tmp}
    subs.w tmp, #1
    bne.w 4b
#endif

    pop {r4-r11, pc}


