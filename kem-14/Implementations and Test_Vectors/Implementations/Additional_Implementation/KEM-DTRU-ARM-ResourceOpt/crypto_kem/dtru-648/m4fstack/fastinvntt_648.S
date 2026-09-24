.syntax unified
.cpu cortex-m4
.thumb

.include "ntt_648_macros.i"

.global invntt_fast_648
.type invntt_fast_648, %function
.align 2
invntt_fast_648:
  push {r4-r11, lr}
  sub sp, sp, #32

  /* Keep the polynomial base so each stage can restart from the block origin. */
  str r0, [sp, #12]
  str r2, [sp, #24]
  str r3, [sp, #28]

  movw r7, #3457
  movw r8, #52607
  movw r9, #19412
  movw r10, #1033

  /* Stage 0: len=9, twenty-four radix-3 groups. */
  inv_radix3_stage 24, 18, 36, 36

  /* Stage 1: len=27, eight radix-3 groups. */
  inv_radix3_stage 8, 54, 108, 108

  /* Stage 2: len=81, 4 radix-2 groups. */
  ldr r0, [sp, #12]
  ldr r5, [sp, #24]
  movs r11, #4
.Lstage2_outer:
  ldr r10, [r5], #4
  add.w r2, r0, #162
  movw r6, #40
.Lstage2_pairs:
  ldr r3, [r0]
  ldr r12, [r2]
  uadd16 r4, r3, r12
  usub16 r3, r3, r12
  str r4, [r0], #4
  fqmul_pair r3, r3, r10, r4, r12, r7, r8
  str r3, [r2], #4
  subs r6, #1
  bne .Lstage2_pairs

  ldrsh r3, [r0]
  ldrsh r12, [r2]
  add r4, r3, r12
  sub r12, r3, r12
  strh r4, [r0]
  fqmul r12, r10, r4, r7, r8
  strh r12, [r2]
  adds r0, #2
  adds r2, #2
  add.w r0, r0, #162
  subs r11, #1
  bne .Lstage2_outer

  /* Stage 3: len=162, 2 radix-2 groups. */
  ldr r0, [sp, #12]
  ldr r5, [sp, #24]
  add.w r5, r5, #16
  movs r11, #2
.Lstage3_outer:
  ldr r10, [r5], #4
  add.w r2, r0, #324
  movw r6, #81
.Lstage3_inner:
  ldr r3, [r0]
  ldr r12, [r2]
  uadd16 r4, r3, r12
  usub16 r3, r3, r12
  str r4, [r0], #4
  fqmul_pair r3, r3, r10, r4, r12, r7, r8
  str r3, [r2], #4
  subs r6, #1
  bne .Lstage3_inner
  add.w r0, r0, #324
  subs r11, #1
  bne .Lstage3_outer

  /* Stage 4: final 324 butterflies plus the two output scaling constants. */
  ldr r0, [sp, #12]
  add.w r2, r0, #648
  ldr r3, [sp, #24]
  ldr r10, [r3, #24]
  ldr r3, [sp, #28]
  ldr r11, [r3]
  ldr r6, [r3, #4]
  movw r5, #162
.Lstage4_loop:
  ldr r3, [r0]
  ldr r12, [r2]
  uadd16 lr, r3, r12
  usub16 r4, r3, r12
  fqmul_pair r4, r4, r10, r3, r12, r7, r8

  usub16 r3, lr, r4
  fqmul_pair r3, r3, r11, r1, r12, r7, r8
  str r3, [r0], #4

  fqmul_pair r4, r4, r6, r1, r12, r7, r8
  str r4, [r2], #4

  subs r5, #1
  bne .Lstage4_loop

  add sp, sp, #32
  pop {r4-r11, pc}

.size invntt_fast_648, .-invntt_fast_648
