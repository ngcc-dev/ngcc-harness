.syntax unified
.cpu cortex-m4
.thumb

/* Coefficient-wise poly add/sub for KYBER_N = 512 (256 32-bit words).
 * 64 iterations of 4 words = 8 int16 each. SIMD uadd16/usub16, no reduction
 * (matches the reference poly_add/poly_sub semantics). */

.align 2
.global pointwise_add
.type pointwise_add, %function
pointwise_add:
  push {r4-r10, lr}
  movw r12, #64
1:
  ldm r1!, {r3-r6}
  ldm r2!, {r7-r10}
  uadd16 r3, r3, r7
  uadd16 r4, r4, r8
  uadd16 r5, r5, r9
  uadd16 r6, r6, r10
  stm r0!, {r3-r6}
  subs.w r12, #1
  bne.w 1b
  pop {r4-r10, pc}
.size pointwise_add, .-pointwise_add

.align 2
.global pointwise_sub
.type pointwise_sub, %function
pointwise_sub:
  push {r4-r10, lr}
  movw r12, #64
1:
  ldm r1!, {r3-r6}
  ldm r2!, {r7-r10}
  usub16 r3, r3, r7
  usub16 r4, r4, r8
  usub16 r5, r5, r9
  usub16 r6, r6, r10
  stm r0!, {r3-r6}
  subs.w r12, #1
  bne.w 1b
  pop {r4-r10, pc}
.size pointwise_sub, .-pointwise_sub
