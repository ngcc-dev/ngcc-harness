.syntax unified
.cpu cortex-m4
.thumb

.equ A01_OFF, 0
.equ A23_OFF, 4
.equ A45_OFF, 8
.equ A67_OFF, 12
.equ A12_OFF, 16
.equ A34_OFF, 20
.equ A56_OFF, 24
.equ A78_OFF, 28
.equ D10_OFF, 32
.equ D21_OFF, 36
.equ D32_OFF, 40
.equ D43_OFF, 44
.equ D54_OFF, 48
.equ D65_OFF, 52
.equ D76_OFF, 56
.equ D87_OFF, 60
.equ LOW0_OFF, 64
.equ LOW1_OFF, 66
.equ LOW2_OFF, 68
.equ LOW3_OFF, 70
.equ LOW4_OFF, 72
.equ LOW5_OFF, 74
.equ LOW6_OFF, 76
.equ LOW7_OFF, 78
.equ LOW8_OFF, 80
.equ FRAME_SIZE, 88

/* Montgomery reduction for a raw 32-bit accumulation. */
.macro montred value, tmp
  smulbb \tmp, \value, r10
  smlabb \value, r9, \tmp, \value
  asr \value, \value, #16
.endm

/* fqmul(a, b) using the signed halfword path on Cortex-M4. */
.macro fqmul value, twiddle, tmp
  smulbb \value, \value, \twiddle
  montred \value, \tmp
.endm

/* Barrett reduction matching barrett_reduce(int16_t). */
.macro barrett value, tmp
  sxth \value, \value
  mul \tmp, \value, r11
  asr \tmp, \tmp, #26
  mul \tmp, \tmp, r9
  sub \value, \value, \tmp
.endm

.macro pack_pair dst, low, high
  pkhbt \dst, \low, \high, lsl #16
.endm

.thumb_func
basemul_9x9_asm:
  push {r4-r8, lr}
  sub sp, sp, #FRAME_SIZE

  mov r5, r1
  mov r6, r2
  mov r7, r3
  mov r8, r0

  /* Build the forward adjacent a-pairs [a0,a1] [a2,a3] [a4,a5] [a6,a7]. */
  ldrh r0, [r5, #0]
  ldrh r1, [r5, #2]
  pack_pair r2, r0, r1
  str r2, [sp, #A01_OFF]

  ldrh r0, [r5, #4]
  ldrh r1, [r5, #6]
  pack_pair r2, r0, r1
  str r2, [sp, #A23_OFF]

  ldrh r0, [r5, #8]
  ldrh r1, [r5, #10]
  pack_pair r2, r0, r1
  str r2, [sp, #A45_OFF]

  ldrh r0, [r5, #12]
  ldrh r1, [r5, #14]
  pack_pair r2, r0, r1
  str r2, [sp, #A67_OFF]

  /* Build the shifted a-pairs [a1,a2] [a3,a4] [a5,a6] [a7,a8]. */
  ldrh r0, [r5, #2]
  ldrh r1, [r5, #4]
  pack_pair r2, r0, r1
  str r2, [sp, #A12_OFF]

  ldrh r0, [r5, #6]
  ldrh r1, [r5, #8]
  pack_pair r2, r0, r1
  str r2, [sp, #A34_OFF]

  ldrh r0, [r5, #10]
  ldrh r1, [r5, #12]
  pack_pair r2, r0, r1
  str r2, [sp, #A56_OFF]

  ldrh r0, [r5, #14]
  ldrh r1, [r5, #16]
  pack_pair r2, r0, r1
  str r2, [sp, #A78_OFF]

  /* Build descending b-pairs [b1,b0] .. [b8,b7]. */
  ldrh r0, [r6, #2]
  ldrh r1, [r6, #0]
  pack_pair r2, r0, r1
  str r2, [sp, #D10_OFF]

  ldrh r0, [r6, #4]
  ldrh r1, [r6, #2]
  pack_pair r2, r0, r1
  str r2, [sp, #D21_OFF]

  ldrh r0, [r6, #6]
  ldrh r1, [r6, #4]
  pack_pair r2, r0, r1
  str r2, [sp, #D32_OFF]

  ldrh r0, [r6, #8]
  ldrh r1, [r6, #6]
  pack_pair r2, r0, r1
  str r2, [sp, #D43_OFF]

  ldrh r0, [r6, #10]
  ldrh r1, [r6, #8]
  pack_pair r2, r0, r1
  str r2, [sp, #D54_OFF]

  ldrh r0, [r6, #12]
  ldrh r1, [r6, #10]
  pack_pair r2, r0, r1
  str r2, [sp, #D65_OFF]

  ldrh r0, [r6, #14]
  ldrh r1, [r6, #12]
  pack_pair r2, r0, r1
  str r2, [sp, #D76_OFF]

  ldrh r0, [r6, #16]
  ldrh r1, [r6, #14]
  pack_pair r2, r0, r1
  str r2, [sp, #D87_OFF]

  /* Low half r[0..8]. */
  ldrh r0, [r5, #0]
  ldrh r1, [r6, #0]
  smulbb r0, r0, r1
  montred r0, r1
  strh r0, [sp, #LOW0_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D10_OFF]
  smuad r0, r0, r1
  montred r0, r1
  strh r0, [sp, #LOW1_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D21_OFF]
  smuad r0, r0, r1
  ldrh r2, [r5, #4]
  ldrh r3, [r6, #0]
  smlabb r0, r2, r3, r0
  montred r0, r1
  strh r0, [sp, #LOW2_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D32_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D10_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW3_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D43_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D21_OFF]
  smlad r0, r1, r2, r0
  ldrh r1, [r5, #8]
  ldrh r2, [r6, #0]
  smlabb r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW4_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D54_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D32_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A45_OFF]
  ldr r2, [sp, #D10_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW5_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D65_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D43_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A45_OFF]
  ldr r2, [sp, #D21_OFF]
  smlad r0, r1, r2, r0
  ldrh r1, [r5, #12]
  ldrh r2, [r6, #0]
  smlabb r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW6_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D76_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D54_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A45_OFF]
  ldr r2, [sp, #D32_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A67_OFF]
  ldr r2, [sp, #D10_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW7_OFF]

  ldr r0, [sp, #A01_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A23_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A45_OFF]
  ldr r2, [sp, #D43_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A67_OFF]
  ldr r2, [sp, #D21_OFF]
  smlad r0, r1, r2, r0
  ldrh r1, [r5, #16]
  ldrh r2, [r6, #0]
  smlabb r0, r1, r2, r0
  montred r0, r1
  strh r0, [sp, #LOW8_OFF]

  /* High half r[9..16], folded on the fly as c[0..7]. */
  ldr r0, [sp, #A12_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A34_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A56_OFF]
  ldr r2, [sp, #D43_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A78_OFF]
  ldr r2, [sp, #D21_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW0_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #0]

  ldr r0, [sp, #A23_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A45_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A67_OFF]
  ldr r2, [sp, #D43_OFF]
  smlad r0, r1, r2, r0
  ldrh r1, [r5, #16]
  ldrh r2, [r6, #4]
  smlabb r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW1_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #2]

  ldr r0, [sp, #A34_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A56_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  ldr r1, [sp, #A78_OFF]
  ldr r2, [sp, #D43_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW2_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #4]

  ldr r0, [sp, #A45_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A67_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  ldrh r1, [r5, #16]
  ldrh r2, [r6, #8]
  smlabb r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW3_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #6]

  ldr r0, [sp, #A56_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldr r1, [sp, #A78_OFF]
  ldr r2, [sp, #D65_OFF]
  smlad r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW4_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #8]

  ldr r0, [sp, #A67_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  ldrh r1, [r5, #16]
  ldrh r2, [r6, #12]
  smlabb r0, r1, r2, r0
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW5_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #10]

  ldr r0, [sp, #A78_OFF]
  ldr r1, [sp, #D87_OFF]
  smuad r0, r0, r1
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW6_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #12]

  ldrh r0, [r5, #16]
  ldrh r1, [r6, #16]
  smulbb r0, r0, r1
  montred r0, r1
  fqmul r0, r7, r1
  ldrsh r1, [sp, #LOW7_OFF]
  add r0, r0, r1
  barrett r0, r1
  strh r0, [r8, #14]

  ldrsh r0, [sp, #LOW8_OFF]
  barrett r0, r1
  strh r0, [r8, #16]

  add sp, sp, #FRAME_SIZE
  pop {r4-r8, pc}

.global triplebasemul_asm
.type triplebasemul_asm, %function
.align 2
triplebasemul_asm:
  push {r4-r11, lr}

  mov r4, r0
  mov r5, r1
  mov r6, r2
  mov r7, r3
  movs r8, #24

  movw r9,  #3457
  movw r10, #52607
  movw r11, #19412

.Ltriplebasemul_loop:
  /* One 27-coefficient block is three independent 9x9 base multiplications. */
  mov r0, r4
  mov r1, r5
  mov r2, r6
  ldrsh r3, [r7]
  bl basemul_9x9_asm

  add.w r0, r4, #18
  add.w r1, r5, #18
  add.w r2, r6, #18
  ldrsh r3, [r7, #2]
  bl basemul_9x9_asm

  add.w r0, r4, #36
  add.w r1, r5, #36
  add.w r2, r6, #36
  ldrsh r3, [r7, #4]
  bl basemul_9x9_asm

  add.w r4, r4, #54
  add.w r5, r5, #54
  add.w r6, r6, #54
  add.w r7, r7, #6
  subs r8, r8, #1
  bne .Ltriplebasemul_loop

  pop {r4-r11, pc}

.size triplebasemul_asm, .-triplebasemul_asm
