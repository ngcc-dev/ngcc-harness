.syntax unified
.cpu cortex-m4
.thumb

//not include "macros.i", define montgomery
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
  bptr      .req r2
  zeta_ptr  .req r3
  poly0     .req r4
  poly1     .req r6
  poly2     .req r5
  poly3     .req r7
  q         .req r8
  qinv      .req r8
  tmp       .req r9
  tmp2      .req r10
  tmp3      .req r11
  zeta      .req r12

  movw q, #3457
  movt qinv, #52607 //-q^(-1),Montgomery Reduction

@ #ifdef USE_REPT
  .rept 192
@ #else
@   movw tmp, #192
@   1:
@     push {tmp}
@ #endif

    // poly0-poly4里各load两个系数，放完后指针后移4个系数
    @ ldrd poly0, poly2, [aptr], #8
    @ ldrd poly1, poly3, [bptr], #8 // It is slower than below by 64 cycles.
    // 以上是论文说的，实际放在大文件里跑也是上面的慢一点，对整体影响在几百个cycles
    ldr poly0, [aptr], #4
    ldr poly1, [bptr], #4
    ldr poly2, [aptr], #4
    ldr poly3, [bptr], #4
    ldr.w zeta, [zeta_ptr], #4

    // basemul(r->coeffs + 4 * i, a->coeffs + 4 * i, b->coeffs + 4 * i, zetas_base[i]);
    // a1*b1*zeta
    smultt tmp, poly0, poly1
    montgomery q, qinv, tmp, tmp2
    smultb tmp2, tmp2, zeta
    // r[0] = a0*b0 + a1*b1*zeta
    smlabb tmp2, poly0, poly1, tmp2
    montgomery q, qinv, tmp2, tmp
    // r[0] in upper half of tmp

    // smuadx实现交叉相乘 a0*b1 + a1*b0
    smuadx tmp2, poly0, poly1
    montgomery q, qinv, tmp2, tmp3
    // r[1] in upper half of tmp3
    // r[0]和r[1]存入tmp
    pkhtb tmp, tmp3, tmp, asr#16
    // tmp存入rptr后，rptr向后移两个系数
    str tmp, [rptr], #4

    // neg: Negative Morphosyntax, 0-zeta
    // neg zeta, zeta

    // basemul(r->coeffs + 4 * i + 2, a->coeffs + 4 * i + 2, b->coeffs + 4 * i + 2, zetas_base[i+1]);
    smultt tmp, poly2, poly3
    montgomery q, qinv, tmp, tmp2
    smultt tmp2, tmp2, zeta
    smlabb tmp2, poly2, poly3, tmp2
    montgomery q, qinv, tmp2, tmp
    // r[0] in upper half of tmp

    smuadx tmp2, poly2, poly3
    montgomery q, qinv, tmp2, tmp3
    // r[1] in upper half of tmp3
    pkhtb tmp, tmp3, tmp, asr#16
    str tmp, [rptr], #4

@ #ifdef USE_REPT
  .endr
@ #else
@     pop {tmp}
@     subs.w tmp, #1
@   bne.w 1b
@ #endif

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