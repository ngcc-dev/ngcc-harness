#include "macros.i"

.syntax unified
.cpu cortex-m4
.thumb

.macro d4np sgn, acc
  ldr r4, [r1], #4                 /* A01 = a0|a1<<16 */
  ldr r5, [r1], #4                 /* A23 = a2|a3<<16 */
  ldr r6, [r2], #4                 /* B01 */
  ldr r7, [r2], #4                 /* B23 */
.if \sgn
  ldr r8, [r3], #4
  neg r8, r8
.else
  ldr r8, [r3]
.endif

  /* r0 = a0b0 + zeta*(a1b3 + a3b1 + a2b2) */
  smultt r9, r4, r7
  smlatt r9, r5, r6, r9
  smlabb r9, r5, r7, r9
  plant_red r11, r12, r14, r9
  smulwt r9, r8, r9
  smlabt r9, r9, r11, r12
  smulbb r10, r4, r6
  plant_red r11, r12, r14, r10
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r1 = (a0b1 + a1b0) + zeta*(a2b3 + a3b2) */
  smuadx r9, r5, r7
  plant_red r11, r12, r14, r9
  smulwt r9, r8, r9
  smlabt r9, r9, r11, r12
  smuadx r10, r4, r6
  plant_red r11, r12, r14, r10
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r2 = (a0b2 + a1b1 + a2b0) + zeta*a3b3 */
  smulbb r9, r4, r7
  smlatt r9, r4, r6, r9
  smlabb r9, r5, r6, r9
  plant_red r11, r12, r14, r9
  smultt r10, r5, r7
  plant_red r11, r12, r14, r10
  smulwt r10, r8, r10
  smlabt r10, r10, r11, r12
  asr r9, r9, #16
  asr r10, r10, #16
  add r9, r9, r10
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2

  /* r3 = a0b3 + a1b2 + a2b1 + a3b0 */
  smuadx r9, r4, r7
  smladx r9, r5, r6, r9
  plant_red r11, r12, r14, r9
  asr r9, r9, #16
.if \acc
  ldrh r10, [r0]
  add r9, r9, r10
.endif
  strh r9, [r0], #2
.endm


/* void basemul_asm_opt_16_16_noprime(int16_t *r, const int16_t *a,
 *                                    const int16_t *b, const int32_t zetas[64]); */
.global basemul_asm_opt_16_16_noprime
.type basemul_asm_opt_16_16_noprime, %function
.align 2
basemul_asm_opt_16_16_noprime:
  push {r4-r11, lr}
  sub sp, sp, #4                   /* loop counter slot */

  movw r12, #26632                 /* qa */
  movw r11, #0
  movt r11, #3329                  /* q in HIGH half */
  movw r14, #62209
  movt r14, #27560                 /* qinv = 0x6ba8f301 */

  mov r9, #64
  str r9, [sp]
1:
  d4np 0, 0                        /* block 2i   : +zeta */
  d4np 1, 0                        /* block 2i+1 : -zeta */
  ldr r9, [sp]
  subs r9, r9, #1
  str r9, [sp]
  bne.w 1b

  add sp, sp, #4
  pop {r4-r11, pc}
.size basemul_asm_opt_16_16_noprime, .-basemul_asm_opt_16_16_noprime


/* void basemul_asm_acc_opt_16_16_noprime(int16_t *r, const int16_t *a,
 *                                        const int16_t *b, const int32_t zetas[64]); */
.global basemul_asm_acc_opt_16_16_noprime
.type basemul_asm_acc_opt_16_16_noprime, %function
.align 2
basemul_asm_acc_opt_16_16_noprime:
  push {r4-r11, lr}
  sub sp, sp, #4

  movw r12, #26632
  movw r11, #0
  movt r11, #3329
  movw r14, #62209
  movt r14, #27560

  mov r9, #64
  str r9, [sp]
1:
  d4np 0, 1
  d4np 1, 1
  ldr r9, [sp]
  subs r9, r9, #1
  str r9, [sp]
  bne.w 1b

  add sp, sp, #4
  pop {r4-r11, pc}
.size basemul_asm_acc_opt_16_16_noprime, .-basemul_asm_acc_opt_16_16_noprime
