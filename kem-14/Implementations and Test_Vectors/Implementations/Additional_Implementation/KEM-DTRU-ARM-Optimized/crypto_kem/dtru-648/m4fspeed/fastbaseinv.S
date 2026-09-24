.syntax unified
.cpu cortex-m4
.thumb

.macro center_reduce value, lo, hi
  smull \lo, \hi, \value, r11
  lsr \lo, \lo, #26
  orr \lo, \lo, \hi, lsl #6
  mls \value, \lo, r10, \value
  cmp \value, r12
  it gt
  subgt \value, \value, r10
  cmp \value, r12
  it gt
  subgt \value, \value, r10
  rsb \lo, r12, #0
  cmp \value, \lo
  it lt
  addlt \value, \value, r10
  cmp \value, \lo
  it lt
  addlt \value, \value, r10
.endm

.global basemul3_mod_y3_zeta_asm
.type basemul3_mod_y3_zeta_asm, %function
.align 2
basemul3_mod_y3_zeta_asm:
  push {r4-r11, lr}
  sub sp, sp, #8
  str r0, [sp, #0]

  movw r10, #3457
  movw r11, #19412
  movw r12, #1728

  ldrsh r4, [r1, #0]   /* a0 */
  ldrsh r5, [r1, #2]   /* a1 */
  ldrsh r6, [r1, #4]   /* a2 */
  ldrsh r7, [r2, #0]   /* b0 */
  ldrsh r8, [r2, #2]   /* b1 */
  ldrsh r9, [r2, #4]   /* b2 */

  /* c0 = a0*b0 + lambda*(a1*b2 + a2*b1) */
  mul lr, r4, r7
  center_reduce lr, r0, r2

  pkhbt r1, r5, r6, lsl #16
  pkhbt r2, r8, r9, lsl #16
  smuadx r1, r1, r2
  center_reduce r1, r0, r2
  mul r1, r1, r3
  center_reduce r1, r0, r2

  add lr, lr, r1
  center_reduce lr, r0, r2
  ldr r0, [sp, #0]
  strh lr, [r0, #0]

  /* c1 = a0*b1 + a1*b0 + lambda*a2*b2 */
  pkhbt r1, r4, r5, lsl #16
  pkhbt r2, r7, r8, lsl #16
  smuadx lr, r1, r2
  center_reduce lr, r0, r2

  mul r2, r6, r9
  center_reduce r2, r0, r1
  mul r2, r2, r3
  center_reduce r2, r0, r1

  add lr, lr, r2
  center_reduce lr, r0, r2
  ldr r0, [sp, #0]
  strh lr, [r0, #2]

  /* c2 = a0*b2 + a1*b1 + a2*b0 */
  pkhbt r1, r4, r5, lsl #16
  pkhbt r2, r9, r8, lsl #16
  smuad lr, r1, r2
  mla lr, r6, r7, lr
  center_reduce lr, r0, r2
  ldr r0, [sp, #0]
  strh lr, [r0, #4]

  add sp, sp, #8
  pop {r4-r11, pc}

.size basemul3_mod_y3_zeta_asm, .-basemul3_mod_y3_zeta_asm
