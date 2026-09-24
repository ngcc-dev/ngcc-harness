/* Shared Cortex-M4 helpers for the DTRU-648 mixed-radix NTT. */

.macro fqmul dst, twiddle, tmp, q, qinv
  smulbb \dst, \dst, \twiddle
  smulbb \tmp, \dst, \qinv
  smlabb \dst, \q, \tmp, \dst
  asr \dst, \dst, #16
.endm

.macro fqmul_pair dst, src, twiddle, tmp_lo, tmp_hi, q, qinv
  smulbb \tmp_lo, \src, \twiddle
  smultt \tmp_hi, \src, \twiddle
  smulbb \dst, \tmp_lo, \qinv
  smlabb \tmp_lo, \q, \dst, \tmp_lo
  asr \tmp_lo, \tmp_lo, #16
  smulbb \dst, \tmp_hi, \qinv
  smlabb \tmp_hi, \q, \dst, \tmp_hi
  asr \tmp_hi, \tmp_hi, #16
  pkhbt \dst, \tmp_lo, \tmp_hi, lsl #16
.endm

.macro barrett value, tmp, q, barrett_v
  sxth \value, \value
  mul \tmp, \value, \barrett_v
  asr \tmp, \tmp, #26
  mul \tmp, \tmp, \q
  sub \value, \value, \tmp
.endm

.macro fwd_radix2_first_pair a0, a1, twiddle, prod, tmp_lo, tmp_hi, q, qinv
  fqmul_pair \prod, \a1, \twiddle, \tmp_lo, \tmp_hi, \q, \qinv
  uadd16 \tmp_hi, \a0, \a1
  usub16 \a1, \tmp_hi, \prod
  uadd16 \a0, \a0, \prod
.endm

.macro fwd_radix2_pair a0, a1, twiddle, prod, tmp_lo, tmp_hi, q, qinv
  fqmul_pair \prod, \a1, \twiddle, \tmp_lo, \tmp_hi, \q, \qinv
  uadd16 \a1, \a0, \prod
  usub16 \a0, \a0, \prod
.endm

.macro inv_radix2_pair a0, a1, twiddle, tmp_lo, tmp_hi, q, qinv
  uadd16 \tmp_lo, \a0, \a1
  usub16 \a0, \a0, \a1
  mov \a1, \tmp_lo
  fqmul_pair \a0, \a0, \twiddle, \tmp_lo, \tmp_hi, \q, \qinv
.endm

.macro fwd_radix3_stage groups, off1, off2, advance
  ldr r0, [sp, #12]
  movs r11, #\groups
  str r11, [sp, #20]
.Lfwd_radix3_outer_\@:
  ldrsh r5, [r1]
  adds r1, #2
  ldrsh r6, [r1]
  adds r1, #2
  add.w r12, r0, #\off1
  str r12, [sp, #16]
.Lfwd_radix3_inner_\@:
  ldrsh r4, [r0]
  ldrsh lr, [r0, #\off1]
  fqmul lr, r5, r12, r7, r8
  ldrsh r2, [r0, #\off2]
  fqmul r2, r6, r12, r7, r8

  sub r3, lr, r2
  fqmul r3, r10, r12, r7, r8

  add r11, r4, r3
  sub r11, r11, r2
  barrett r11, r12, r7, r9
  strh r11, [r0, #\off1]

  sub r11, r4, r3
  sub r11, r11, lr
  barrett r11, r12, r7, r9
  strh r11, [r0, #\off2]

  add r11, r4, lr
  add r11, r11, r2
  barrett r11, r12, r7, r9
  strh r11, [r0]

  adds r0, #2
  ldr r12, [sp, #16]
  cmp r0, r12
  bne .Lfwd_radix3_inner_\@
  add.w r0, r0, #\advance
  ldr r11, [sp, #20]
  subs r11, r11, #1
  str r11, [sp, #20]
  bne .Lfwd_radix3_outer_\@
.endm

.macro inv_radix3_stage groups, off1, off2, advance
  ldr r0, [sp, #12]
  movs r11, #\groups
  str r11, [sp, #20]
.Linv_radix3_outer_\@:
  ldrsh r5, [r1]
  adds r1, #2
  ldrsh r6, [r1]
  adds r1, #2
  add.w r12, r0, #\off1
  str r12, [sp, #16]
.Linv_radix3_inner_\@:
  ldrsh r4, [r0]
  ldrsh lr, [r0, #\off1]
  ldrsh r2, [r0, #\off2]

  sub r3, lr, r2
  fqmul r3, r10, r12, r7, r8

  add r11, r4, r3
  sub r11, r11, r2
  fqmul r11, r5, r12, r7, r8
  strh r11, [r0, #\off1]

  sub r11, r4, r3
  sub r11, r11, lr
  fqmul r11, r6, r12, r7, r8
  strh r11, [r0, #\off2]

  add r11, r4, lr
  add r11, r11, r2
  barrett r11, r12, r7, r9
  strh r11, [r0]

  adds r0, #2
  ldr r12, [sp, #16]
  cmp r0, r12
  bne .Linv_radix3_inner_\@
  add.w r0, r0, #\advance
  ldr r11, [sp, #20]
  subs r11, r11, #1
  str r11, [sp, #20]
  bne .Linv_radix3_outer_\@
.endm
