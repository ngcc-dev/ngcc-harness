.syntax unified
.cpu cortex-m4
.thumb

.include "ntt_648_macros.i"

.global ntt_fast_648
.type ntt_fast_648, %function
.align 2
ntt_fast_648:
  push {r4-r11, lr}
  sub sp, sp, #24

  /* Keep the polynomial base so each stage can restart from the block origin. */
  str r0, [sp, #12]

  movw r7, #3457
  movw r8, #52607
  movw r9, #19412

  /* Stage 0: first 324 butterflies, packed 2 coefficients at a time. */
  ldr r0, [sp, #12]
  ldr r5, [r2], #4
  add.w r11, r0, #648
  movw r6, #162
.Lstage0_loop:
  ldr r3, [r0]
  ldr r12, [r11]
  fwd_radix2_first_pair r3, r12, r5, r4, r10, lr, r7, r8
  str r3, [r0], #4
  str r12, [r11], #4
  subs r6, #1
  bne .Lstage0_loop

  /* Stage 1: len=162, 2 groups, packed butterflies. */
  ldr r0, [sp, #12]
  movs r11, #2
.Lstage1_outer:
  ldr r5, [r2], #4
  add.w r4, r0, #324
  movw r6, #81
.Lstage1_inner:
  ldr r3, [r0]
  ldr r12, [r4]
  fwd_radix2_pair r3, r12, r5, lr, r10, r12, r7, r8
  str r12, [r0], #4
  str r3, [r4], #4
  subs r6, #1
  bne .Lstage1_inner
  add.w r0, r0, #324
  subs r11, #1
  bne .Lstage1_outer

  /* Stage 2: len=81, 4 groups, 40 coefficient pairs plus one scalar tail. */
  ldr r0, [sp, #12]
  movs r11, #4
.Lstage2_outer:
  ldr r5, [r2], #4
  add.w r4, r0, #162
  movw r6, #40
.Lstage2_pairs:
  ldr r3, [r0]
  ldr r12, [r4]
  sxth lr, r3
  asr r10, r3, #16
  sxth r3, r12
  asr r12, r12, #16

  fqmul r3, r5, r9, r7, r8
  fqmul r12, r5, r9, r7, r8

  add r9, lr, r3
  sub lr, lr, r3
  add r3, r10, r12
  sub r10, r10, r12
  pkhbt r9, r9, r3, lsl #16
  pkhbt lr, lr, r10, lsl #16
  str r9, [r0], #4
  str lr, [r4], #4
  subs r6, #1
  bne .Lstage2_pairs

  ldrsh r3, [r0]
  ldrsh r12, [r4]
  fqmul r12, r5, lr, r7, r8
  add lr, r3, r12
  sub r3, r3, r12
  strh lr, [r0], #2
  strh r3, [r4], #2

  add.w r0, r0, #162
  subs r11, #1
  bne .Lstage2_outer

  movw r9, #19412
  movw r10, #2571

  /* Stage 3: len=27, eight radix-3 groups using rho = zetas[0]. */
  fwd_radix3_stage 8, 54, 108, 108

  /* Stage 4: len=9, twenty-four radix-3 groups. */
  fwd_radix3_stage 24, 18, 36, 36

  add sp, sp, #24
  pop {r4-r11, pc}

.size ntt_fast_648, .-ntt_fast_648
