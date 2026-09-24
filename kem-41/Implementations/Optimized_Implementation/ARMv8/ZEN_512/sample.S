// Hand-optimized AArch64 assembly kernels for ZEN_512 sampling.
// Derived from split assembly kernels and manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.

// begin expanded include: sample_asm_parts/00_text_header.inc
	.text
	.section	.rodata.cst16,"aM",@progbits,16
// end expanded include: sample_asm_parts/00_text_header.inc
// begin expanded include: sample_asm_parts/30_tenary1_8.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function tenary1_8
Lzen512_tenary1_8_shift0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
Lzen512_tenary1_8_shift1:
	.short	65528 // 0xfff8
	.short	65527 // 0xfff7
	.short	65526 // 0xfff6
	.short	65525 // 0xfff5
	.short	65524 // 0xfff4
	.short	65523 // 0xfff3
	.short	65522 // 0xfff2
	.short	65521 // 0xfff1
	.text
	.globl	tenary1_8
	.p2align	2
tenary1_8: // @tenary1_8
	.cfi_startproc
// %bb.0:
	mov	x8, x1
	add	x9, x1, #256
	add	x10, x1, #512
	mov	x12, x0
	mov	x13, #32 // =0x20
	adrp	x11, Lzen512_tenary1_8_shift0
	ldr	q0, [x11, :lo12:Lzen512_tenary1_8_shift0]
	movi	v1.8h, #1
	adrp	x11, Lzen512_tenary1_8_shift1
	ldr	q2, [x11, :lo12:Lzen512_tenary1_8_shift1]
Lzen512_tenary1_8_loop: // =>This Inner Loop Header: Depth=1
	ldr	x14, [x8], #8
	ldr	x15, [x9], #8
	ldr	x16, [x10], #8
	bic	x17, x14, x15
	and	x17, x17, x16
	bic	x14, x15, x14
	and	x14, x14, x16
	dup	v3.8h, w17
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w14
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x12]
	lsr	x17, x17, #16
	lsr	x14, x14, #16
	dup	v3.8h, w17
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w14
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x12, #32]
	lsr	x17, x17, #16
	lsr	x14, x14, #16
	dup	v3.8h, w17
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w14
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x12, #64]
	lsr	x17, x17, #16
	lsr	x14, x14, #16
	dup	v3.8h, w17
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w14
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x12, #96]
	add	x12, x12, #128
	subs	x13, x13, #1
	b.ne	Lzen512_tenary1_8_loop
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/30_tenary1_8.inc
// begin expanded include: sample_asm_parts/40_tenary3_32.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function tenary3_32
Lzen512_tenary3_32_shift0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
Lzen512_tenary3_32_shift1:
	.short	65528 // 0xfff8
	.short	65527 // 0xfff7
	.short	65526 // 0xfff6
	.short	65525 // 0xfff5
	.short	65524 // 0xfff4
	.short	65523 // 0xfff3
	.short	65522 // 0xfff2
	.short	65521 // 0xfff1
	.text
	.globl	tenary3_32
	.p2align	2
tenary3_32: // @tenary3_32
	.cfi_startproc
// %bb.0:
	mov	x8, x1
	add	x9, x1, #256
	add	x10, x1, #512
	add	x11, x1, #768
	add	x12, x1, #1024
	mov	x13, x0
	mov	x14, #32 // =0x20
	adrp	x15, Lzen512_tenary3_32_shift0
	ldr	q0, [x15, :lo12:Lzen512_tenary3_32_shift0]
	movi	v1.8h, #1
	adrp	x15, Lzen512_tenary3_32_shift1
	ldr	q2, [x15, :lo12:Lzen512_tenary3_32_shift1]
Lzen512_tenary3_32_loop: // =>This Inner Loop Header: Depth=1
	ldr	x2, [x8], #8
	ldr	x3, [x9], #8
	ldr	x4, [x10], #8
	ldr	x5, [x11], #8
	ldr	x6, [x12], #8
	and	x2, x2, x3
	and	x2, x2, x6
	and	x4, x4, x5
	and	x4, x4, x6
	dup	v3.8h, w2
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w4
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x13]
	lsr	x2, x2, #16
	lsr	x4, x4, #16
	dup	v3.8h, w2
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w4
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x13, #32]
	lsr	x2, x2, #16
	lsr	x4, x4, #16
	dup	v3.8h, w2
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w4
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x13, #64]
	lsr	x2, x2, #16
	lsr	x4, x4, #16
	dup	v3.8h, w2
	sshl	v4.8h, v3.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	dup	v5.8h, w4
	sshl	v6.8h, v5.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	sub	v4.8h, v4.8h, v6.8h
	sshl	v3.8h, v3.8h, v2.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v5.8h, v5.8h, v2.8h
	and	v5.16b, v5.16b, v1.16b
	sub	v3.8h, v3.8h, v5.8h
	stp	q4, q3, [x13, #96]
	add	x13, x13, #128
	subs	x14, x14, #1
	b.ne	Lzen512_tenary3_32_loop
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/40_tenary3_32.inc
