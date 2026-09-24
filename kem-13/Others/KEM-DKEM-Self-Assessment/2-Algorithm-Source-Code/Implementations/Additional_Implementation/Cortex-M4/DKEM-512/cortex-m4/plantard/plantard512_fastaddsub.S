#if (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD)   /* DKE config gating (auto-added) */
.syntax unified
.cpu cortex-m4
.thumb

.align 2
.global pointwise_sub
.type pointwise_sub, %function
pointwise_sub:
  push {r4-r11, lr}

  movw r14, #51
  1:
    ldm r1!, {r3-r7}
    ldm r2!, {r8-r12}
    usub16 r3, r3, r8
    usub16 r4, r4, r9
    usub16 r5, r5, r10
    usub16 r6, r6, r11
    usub16 r7, r7, r12
    stm r0!, {r3-r7}

    subs.w r14, #1
  bne.w 1b

  ldr r3, [r1], #4
  ldr r4, [r2], #4
  usub16 r3, r3, r4
  str r3, [r0], #4
  pop {r4-r11, pc}


.align 2
.global pointwise_add
.type pointwise_add, %function
pointwise_add:
  push {r4-r11, lr}

  movw r14, #51
  1:
    ldm r1!, {r3-r7}
    ldm r2!, {r8-r12}
    uadd16 r3, r3, r8
    uadd16 r4, r4, r9
    uadd16 r5, r5, r10
    uadd16 r6, r6, r11
    uadd16 r7, r7, r12
    stm r0!, {r3-r7}

    subs.w r14, #1
  bne.w 1b

  ldr r3, [r1], #4
  ldr r4, [r2], #4
  uadd16 r3, r3, r4
  str r3, [r0], #4
  pop {r4-r11, pc}



// 4-bit *7681/16=4-bit *480=4-bit
.macro expand_signal_byte out, byte, lo, hi, q
	and.w \lo, \byte, #0x0f
	lsr.w \hi, \byte, #4
	smulbt \lo, \lo, \q
	smulbt \hi, \hi, \q
	lsr \lo, #4
	pkhbt \out, \lo, \hi, lsl #12
.endm

.align 2
.global poly_decompress_floor_asm
.type poly_decompress_floor_asm, %function
poly_decompress_floor_asm:
	push {r4-r10, lr}

	rptr .req r0
	aptr .req r1
	in0  .req r2
	in1  .req r3
	out0 .req r4
	out1 .req r5
	out2 .req r6
	out3 .req r7
	lo   .req r8
	hi   .req r9
	q    .req r10
	loop .req r12

	movw loop, #64
	movt q, #7681
1:
	ldr.w in0, [aptr], #4

	uxtb.w in1, in0
	expand_signal_byte out0, in1, lo, hi, q

	ubfx.w in1, in0, #8, #8
	expand_signal_byte out1, in1, lo, hi, q

	ubfx.w in1, in0, #16, #8
	expand_signal_byte out2, in1, lo, hi, q

	ubfx.w in1, in0, #24, #8
	expand_signal_byte out3, in1, lo, hi, q

	stm rptr!, {out0-out3}
	subs.w loop, loop, #1
	bne.w 1b

	pop {r4-r10, pc}
.size poly_decompress_floor_asm, . - poly_decompress_floor_asm

.unreq rptr
.unreq aptr
.unreq in0
.unreq in1
.unreq out0
.unreq out1
.unreq out2
.unreq out3
.unreq lo
.unreq hi
.unreq loop
.unreq q
#endif /* (DKE_MODE == 512) && defined(DKE_USE_CORTEX_M4_PLANTARD) -- DKE config gating (auto-added) */
