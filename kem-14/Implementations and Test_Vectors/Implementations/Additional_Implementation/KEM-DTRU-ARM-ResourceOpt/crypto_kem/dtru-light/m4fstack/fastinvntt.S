#include "macros.i"
#include "macros_invntt.i"

.syntax unified
.cpu cortex-m4
.thumb

.global invntt_fast
.type invntt_fast, %function
.align 2
invntt_fast:
  push {r4-r11, lr}

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
  barrettconst   .req r10
  qinv        .req r11
  q           .req r11
  tmp         .req r12
  tmp2        .req r14

  movw q, #769
  movt qinv, #767
  .equ barrett_constant, 21817 //doublebarret_init
  .equ distance, 16
    movw barrettconst, #0x0ae3
    movt barrettconst, #0xffed
    vmov s17, barrettconst
    movw barrettconst, #32768 //2^15
    vmov s18, barrettconst


#ifdef USE_REPT
  .rept 32
#else
  movw tmp, #32
  8:
    push {tmp}
#endif

      load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
      load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

      ldr.w twiddle, [twiddle_ptr]
      two_doubleinvbutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
      two_doubleinvbutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv

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
  bne.w 8b
#endif


.equ distance, 64
sub.w poly, #1024 //将poly的地址减1024，回到第一个参数的位置



  ### LAYER 6+5+4
#ifdef USE_REPT
  .rept 8
#else
  movw tmp, #8
  7:
    push {tmp}
#endif

#ifdef USE_REPT
    .rept 4
#else
    movw tmp, #4
    5:
      push {tmp}
#endif

      // a0,  a12, a24, a36
      // a48, a60, a72, a84
      load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
      load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

      ldr.w twiddle, [twiddle_ptr]
      two_doubleinvbutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv


      ldr.w twiddle, [twiddle_ptr, #4]
      two_doubleinvbutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv


      ldr.w twiddle, [twiddle_ptr, #8]
      two_doubleinvbutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
      two_doubleinvbutterfly t, t, poly4 , poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv


      ldrh twiddle, [twiddle_ptr, #12]
      two_doubleinvbutterfly b, b, poly0, poly4, poly1, poly5, twiddle, tmp, tmp2, q, qinv
      two_doubleinvbutterfly b, b, poly2, poly6, poly3, poly7, twiddle, tmp, tmp2, q, qinv


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
    bne.w 5b
#endif

    add.w poly, #112
    add.w twiddle_ptr, #14

#ifdef USE_REPT
  .endr
#else
    pop {tmp}
    subs.w tmp, #1
  bne.w 7b
#endif


  sub.w poly, #1024
  .equ distance, 512


  ### LAYER 3+2 + the first level
#ifdef USE_REPT
  .rept 32
#else
  movw tmp, #32
  6:
    push {tmp}
#endif
    // a0,   a96,  a192, a288
    // a384, a480, a576, a672
    load poly, poly0, poly1, poly2, poly3, #0, #distance/4, #2*distance/4, #3*distance/4
    load poly, poly4, poly5, poly6, poly7, #distance, #5*distance/4, #6*distance/4, #7*distance/4

    ldr.w twiddle, [twiddle_ptr]
    two_doubleinvbutterfly b, t, poly0, poly1, poly2, poly3, twiddle, tmp, tmp2, q, qinv

    ldr.w twiddle, [twiddle_ptr, #4]
    two_doubleinvbutterfly b, t, poly4, poly5, poly6, poly7, twiddle, tmp, tmp2, q, qinv


    ldr.w twiddle, [twiddle_ptr, #8]
    two_doubleinvbutterfly b, b, poly0, poly2, poly1, poly3, twiddle, tmp, tmp2, q, qinv
    two_doubleinvbutterfly t, t, poly4, poly6, poly5, poly7, twiddle, tmp, tmp2, q, qinv

    movw barrettconst, #21817
    doublebarrett_init poly0, tmp, tmp2, q, barrettconst
    doublebarrett_init poly1, tmp, tmp2, q, barrettconst
    doublebarrett_init poly2, tmp, tmp2, q, barrettconst
    doublebarrett_init poly3, tmp, tmp2, q, barrettconst
    doublebarrett_init poly4, tmp, tmp2, q, barrettconst
    doublebarrett_init poly5, tmp, tmp2, q, barrettconst
    doublebarrett_init poly6, tmp, tmp2, q, barrettconst
    doublebarrett_init poly7, tmp, tmp2, q, barrettconst


    ldr.w twiddle, [twiddle_ptr, #12]
    two_doubleinvbutterfly b, b, poly0, poly4, poly1, poly5, twiddle, tmp, tmp2, q, qinv
    two_doubleinvbutterfly b, b, poly2, poly6, poly3, poly7, twiddle, tmp, tmp2, q, qinv

    mul_twiddle t, poly0, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly1, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly2, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly3, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly4, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly5, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly6, twiddle, tmp, tmp2, q, qinv
    mul_twiddle t, poly7, twiddle, tmp, tmp2, q, qinv

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
  bne.w 6b
#endif

  pop {r4-r11, pc}
