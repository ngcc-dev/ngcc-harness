// Hand-optimized AArch64 PMULL assembly for ZEN_256 FastInversion.
// Implements the packed-bit inversion path manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function FastInversion
lCPI3_0:
	.short	0 // 0x0
	.short	1 // 0x1
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
	.short	6 // 0x6
	.short	7 // 0x7
lCPI3_1:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
	.text
	.globl	FastInversion
	.p2align	2
FastInversion: // @FastInversion
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #352
	stp	x28, x27, [sp, #256] // 16-byte Folded Spill
	stp	x26, x25, [sp, #272] // 16-byte Folded Spill
	stp	x24, x23, [sp, #288] // 16-byte Folded Spill
	stp	x22, x21, [sp, #304] // 16-byte Folded Spill
	stp	x20, x19, [sp, #320] // 16-byte Folded Spill
	stp	x29, x30, [sp, #336] // 16-byte Folded Spill
	.cfi_def_cfa_offset 352
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset w25, -72
	.cfi_offset w26, -80
	.cfi_offset w27, -88
	.cfi_offset w28, -96
	str	x0, [sp, #8] // 8-byte Folded Spill
	str	xzr, [sp, #216]
	movi	v0.2d, #0000000000000000
	stur	q0, [sp, #200]
	movi	v0.8h, #1
	ldp	q1, q2, [x1]
	and	v3.16b, v1.16b, v0.16b
Lloh2:
	adrp	x8, lCPI3_0
Lloh3:
	ldr	q1, [x8, :lo12:lCPI3_0]
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v17.8h, v3.8h, v3.8h
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v16.8h, v2.8h, v2.8h
	ldp	q2, q3, [x1, #32]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v6.8h, v3.8h, v3.8h
	ldp	q3, q4, [x1, #64]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v7.8h, v3.8h, v3.8h
	and	v3.16b, v4.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v4.8h, v3.8h, v3.8h
	ldp	q3, q18, [x1, #96]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v5.8h, v3.8h, v3.8h
	and	v3.16b, v18.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	ldp	q18, q19, [x1, #128]
	and	v18.16b, v18.16b, v0.16b
	sshl	v18.8h, v18.8h, v1.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	umov	w8, v17.b[0]
	addp	v18.8h, v18.8h, v18.8h
	fmov	s17, w8
	umov	w8, v18.b[0]
	mov	v17.s[1], w8
	and	v18.16b, v19.16b, v0.16b
	sshl	v18.8h, v18.8h, v1.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	umov	w8, v16.b[0]
	addp	v18.8h, v18.8h, v18.8h
	fmov	s16, w8
	umov	w8, v18.b[0]
	mov	v16.s[1], w8
	ldp	q18, q19, [x1, #160]
	and	v18.16b, v18.16b, v0.16b
	sshl	v18.8h, v18.8h, v1.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	umov	w8, v2.b[0]
	fmov	s2, w8
	umov	w8, v18.b[0]
	mov	v2.s[1], w8
	and	v18.16b, v19.16b, v0.16b
	sshl	v18.8h, v18.8h, v1.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	addp	v18.8h, v18.8h, v18.8h
	ldp	q19, q20, [x1, #192]
	and	v19.16b, v19.16b, v0.16b
	sshl	v19.8h, v19.8h, v1.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	and	v20.16b, v20.16b, v0.16b
	sshl	v20.8h, v20.8h, v1.8h
	addp	v20.8h, v20.8h, v20.8h
	addp	v20.8h, v20.8h, v20.8h
	addp	v20.8h, v20.8h, v20.8h
	ldp	q21, q22, [x1, #224]
	and	v21.16b, v21.16b, v0.16b
	sshl	v21.8h, v21.8h, v1.8h
	addp	v21.8h, v21.8h, v21.8h
	addp	v21.8h, v21.8h, v21.8h
	addp	v21.8h, v21.8h, v21.8h
	and	v22.16b, v22.16b, v0.16b
	sshl	v22.8h, v22.8h, v1.8h
	addp	v22.8h, v22.8h, v22.8h
	addp	v22.8h, v22.8h, v22.8h
	addp	v22.8h, v22.8h, v22.8h
	sli	v17.2s, v16.2s, #8
	ushll	v16.2d, v17.2s, #0
	ushll	v17.2d, v2.2s, #0
	movi	v2.2d, #0x000000000000ff
	and	v17.16b, v17.16b, v2.16b
	sli	v16.2d, v17.2d, #16
	umov	w8, v6.b[0]
	fmov	s6, w8
	umov	w8, v18.b[0]
	mov	v6.s[1], w8
	ushll	v6.2d, v6.2s, #0
	and	v6.16b, v6.16b, v2.16b
	umov	w8, v7.b[0]
	shl	v6.2d, v6.2d, #24
	fmov	s7, w8
	umov	w8, v19.b[0]
	mov	v7.s[1], w8
	orr	v6.16b, v6.16b, v16.16b
	ushll	v7.2d, v7.2s, #0
	and	v7.16b, v7.16b, v2.16b
	shl	v7.2d, v7.2d, #32
	umov	w8, v4.b[0]
	fmov	s4, w8
	umov	w8, v20.b[0]
	mov	v4.s[1], w8
	ushll	v4.2d, v4.2s, #0
	and	v4.16b, v4.16b, v2.16b
	shl	v4.2d, v4.2d, #40
	orr	v4.16b, v4.16b, v7.16b
	orr	v4.16b, v4.16b, v6.16b
	umov	w8, v5.b[0]
	fmov	s5, w8
	umov	w8, v21.b[0]
	mov	v5.s[1], w8
	ushll	v5.2d, v5.2s, #0
	and	v5.16b, v5.16b, v2.16b
	shl	v5.2d, v5.2d, #48
	umov	w8, v3.b[0]
	umov	w9, v22.b[0]
	fmov	s3, w8
	mov	v3.s[1], w9
	ushll	v3.2d, v3.2s, #0
	shl	v3.2d, v3.2d, #56
	orr	v3.16b, v5.16b, v3.16b
	orr	v24.16b, v3.16b, v4.16b
	str	q24, [sp, #112] // 16-byte Folded Spill
	ldp	q3, q4, [x1, #256]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v18.8h, v3.8h, v3.8h
	and	v3.16b, v4.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v17.8h, v3.8h, v3.8h
	ldp	q3, q4, [x1, #288]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v16.8h, v3.8h, v3.8h
	and	v3.16b, v4.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v6.8h, v3.8h, v3.8h
	ldp	q3, q4, [x1, #320]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v7.8h, v3.8h, v3.8h
	and	v3.16b, v4.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v5.8h, v3.8h, v3.8h
	ldp	q3, q19, [x1, #352]
	and	v3.16b, v3.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v4.8h, v3.8h, v3.8h
	and	v3.16b, v19.16b, v0.16b
	sshl	v3.8h, v3.8h, v1.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	ldp	q19, q20, [x1, #384]
	and	v19.16b, v19.16b, v0.16b
	sshl	v19.8h, v19.8h, v1.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	umov	w8, v18.b[0]
	umov	w9, v19.b[0]
	fmov	s18, w8
	mov	v18.s[1], w9
	and	v19.16b, v20.16b, v0.16b
	sshl	v19.8h, v19.8h, v1.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	umov	w8, v17.b[0]
	umov	w9, v19.b[0]
	fmov	s17, w8
	mov	v17.s[1], w9
	ldp	q19, q20, [x1, #416]
	and	v19.16b, v19.16b, v0.16b
	sshl	v19.8h, v19.8h, v1.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	umov	w8, v16.b[0]
	fmov	s16, w8
	umov	w8, v19.b[0]
	mov	v16.s[1], w8
	and	v19.16b, v20.16b, v0.16b
	sshl	v19.8h, v19.8h, v1.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	addp	v19.8h, v19.8h, v19.8h
	ldp	q20, q21, [x1, #448]
	and	v20.16b, v20.16b, v0.16b
	sshl	v20.8h, v20.8h, v1.8h
	addp	v20.8h, v20.8h, v20.8h
	addp	v20.8h, v20.8h, v20.8h
	addp	v20.8h, v20.8h, v20.8h
	and	v21.16b, v21.16b, v0.16b
	sshl	v21.8h, v21.8h, v1.8h
	addp	v21.8h, v21.8h, v21.8h
	addp	v21.8h, v21.8h, v21.8h
	addp	v21.8h, v21.8h, v21.8h
	ldp	q22, q23, [x1, #480]
	and	v22.16b, v22.16b, v0.16b
	sshl	v22.8h, v22.8h, v1.8h
	addp	v22.8h, v22.8h, v22.8h
	addp	v22.8h, v22.8h, v22.8h
	addp	v22.8h, v22.8h, v22.8h
	and	v0.16b, v23.16b, v0.16b
	sshl	v0.8h, v0.8h, v1.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	sli	v18.2s, v17.2s, #8
	ushll	v1.2d, v18.2s, #0
	ushll	v16.2d, v16.2s, #0
	and	v16.16b, v16.16b, v2.16b
	sli	v1.2d, v16.2d, #16
	umov	w8, v6.b[0]
	fmov	s6, w8
	umov	w8, v19.b[0]
	mov	v6.s[1], w8
	ushll	v6.2d, v6.2s, #0
	and	v6.16b, v6.16b, v2.16b
	shl	v6.2d, v6.2d, #24
	orr	v1.16b, v6.16b, v1.16b
	umov	w8, v7.b[0]
	umov	w9, v20.b[0]
	fmov	s6, w8
	mov	v6.s[1], w9
	ushll	v6.2d, v6.2s, #0
	umov	w8, v5.b[0]
	and	v5.16b, v6.16b, v2.16b
	fmov	s6, w8
	umov	w8, v21.b[0]
	mov	v6.s[1], w8
	shl	v5.2d, v5.2d, #32
	ushll	v6.2d, v6.2s, #0
	and	v6.16b, v6.16b, v2.16b
	shl	v6.2d, v6.2d, #40
	umov	w8, v4.b[0]
	orr	v4.16b, v6.16b, v5.16b
	fmov	s5, w8
	umov	w8, v22.b[0]
	mov	v5.s[1], w8
	orr	v1.16b, v4.16b, v1.16b
	ushll	v4.2d, v5.2s, #0
	and	v2.16b, v4.16b, v2.16b
	shl	v2.2d, v2.2d, #48
	umov	w8, v3.b[0]
	fmov	s3, w8
	umov	w8, v0.b[0]
	mov	v3.s[1], w8
	ushll	v0.2d, v3.2s, #0
	shl	v0.2d, v0.2d, #56
	orr	v0.16b, v2.16b, v0.16b
	orr	v16.16b, v0.16b, v1.16b
	str	q16, [sp, #96] // 16-byte Folded Spill
	fmov	x8, d24
	eor	x9, x8, x8, lsl #1
	eor	x9, x9, x9, lsl #2
	eor	x9, x9, x9, lsl #4
	eor	x9, x9, x9, lsl #8
	eor	x9, x9, x9, lsl #16
	eor	x12, x9, x9, lsl #32
	umov	x16, v24.d[1]
	eor	x14, x16, x8
	fmov	x9, d16
	ext	v2.16b, v24.16b, v16.16b, #8
	mov	w10, #1 // =0x1
	dup	v0.2d, x10
	add	v3.2d, v2.2d, v2.2d
	eor	v4.16b, v3.16b, v2.16b
	shl	v5.2d, v4.2d, #2
	eor	x11, x9, x14
	fmov	d1, x11
	cnt	v1.8b, v1.8b
	uaddlv	h1, v1.8b
	fmov	w11, s1
	and	w11, w11, #0x1
	umov	x17, v16.d[1]
	eor	x13, x17, x17, lsl #1
	eor	x13, x13, x13, lsl #2
	eor	x13, x13, x13, lsl #4
	eor	x13, x13, x13, lsl #8
	eor	x13, x13, x13, lsl #16
	neg	x11, x11
	eor	x11, x11, x13, lsl #32
	eor	x13, x11, x13
	mov	v1.16b, v24.16b
	str	x14, [sp, #80] // 8-byte Folded Spill
	mov	v1.d[1], x14
	cnt	v6.16b, v1.16b
	movi	v1.16b, #1
	movi	v7.2d, #0000000000000000
	udot	v7.4s, v1.16b, v6.16b
	uaddlp	v6.2d, v7.4s
	and	v6.16b, v6.16b, v0.16b
	eor3	v3.16b, v3.16b, v2.16b, v5.16b
	shl	v7.2d, v3.2d, #4
	eor3	v4.16b, v5.16b, v4.16b, v7.16b
	shl	v5.2d, v4.2d, #8
	eor3	v3.16b, v7.16b, v3.16b, v5.16b
	shl	v3.2d, v3.2d, #16
	eor3	v3.16b, v5.16b, v4.16b, v3.16b
	shl	v4.2d, v3.2d, #32
	neg	v5.2d, v6.2d
	eor	v6.16b, v4.16b, v5.16b
	eor3	v4.16b, v4.16b, v5.16b, v3.16b
	fmov	x11, d4
	umov	x14, v4.d[1]
	eor	x11, x11, x12
	eor	x11, x11, x13
	eor	x11, x11, x14
	fmov	d5, x11
	cnt	v5.8b, v5.8b
	uaddlv	h5, v5.8b
	fmov	w11, s5
	and	w11, w11, #0x1
	neg	x14, x11
	and	x15, x8, x14
	dup	v5.2d, x14
	and	v2.16b, v2.16b, v5.16b
	eor3	v3.16b, v6.16b, v3.16b, v2.16b
	and	x14, x17, x14
	eor	x13, x14, x13
	add	v5.2d, v3.2d, v3.2d
	eor	x12, x15, x12
	eor	x14, x12, x12, lsl #1
	eor	x14, x14, x14, lsl #2
	eor	x14, x14, x14, lsl #4
	eor	x14, x14, x14, lsl #8
	eor	x14, x14, x14, lsl #16
	eor	x14, x14, x14, lsl #32
	str	x14, [sp, #224]
	eor3	v2.16b, v2.16b, v4.16b, v5.16b
	shl	v4.2d, v2.2d, #2
	eor3	v5.16b, v5.16b, v3.16b, v4.16b
	umov	x14, v3.d[1]
	fmov	x15, d3
	eor	x15, x15, x12
	fmov	d3, x12
	mov	v3.d[1], x15
	cnt	v3.16b, v3.16b
	movi	v6.2d, #0000000000000000
	udot	v6.4s, v1.16b, v3.16b
	uaddlp	v1.2d, v6.4s
	and	v0.16b, v1.16b, v0.16b
	shl	v1.2d, v5.2d, #4
	eor3	v2.16b, v4.16b, v2.16b, v1.16b
	shl	v3.2d, v2.2d, #8
	eor3	v1.16b, v1.16b, v5.16b, v3.16b
	shl	v1.2d, v1.2d, #16
	eor3	v1.16b, v3.16b, v2.16b, v1.16b
	shl	v2.2d, v1.2d, #32
	neg	v0.2d, v0.2d
	eor3	v0.16b, v2.16b, v0.16b, v1.16b
	stur	q0, [sp, #232]
	eor	x12, x15, x14
	fmov	d0, x12
	cnt	v0.8b, v0.8b
	uaddlv	h0, v0.8b
	fmov	w12, s0
	and	w12, w12, #0x1
	eor	x13, x13, x13, lsl #1
	eor	x13, x13, x13, lsl #2
	eor	x13, x13, x13, lsl #4
	eor	x13, x13, x13, lsl #8
	eor	x13, x13, x13, lsl #16
	neg	x12, x12
	eor	x12, x12, x13, lsl #32
	eor	x12, x12, x13
	str	x12, [sp, #248]
	cmp	x11, #0
	cinc	x25, x10, ne
	str	x25, [sp, #192]
	eor	x10, x17, x9
	stp	x10, x17, [sp, #64] // 16-byte Folded Spill
	eor	x8, x9, x8
	str	x16, [sp, #88] // 8-byte Folded Spill
	eor	x28, x17, x16
	str	x8, [sp, #56] // 8-byte Folded Spill
	eor	x8, x28, x8
	str	x8, [sp, #48] // 8-byte Folded Spill
	ldp	x23, x21, [sp, #200]
	ldr	x22, [sp, #216]
	ext	v0.16b, v24.16b, v24.16b, #8
	zip1	v1.2d, v0.2d, v16.2d
	mov	w20, #2 // =0x2
	mov	v0.d[0], v16.d[1]
	stp	q1, q0, [sp, #16] // 32-byte Folded Spill
	add	x24, sp, #224
	mov	w26, #64 // =0x40
	b	LBB3_3
LBB3_1: // in Loop: Header=BB3_3 Depth=1
	ldr	x9, [sp, #136]
	eor	x23, x23, x9
	eor	x21, x21, x8
	stp	x23, x21, [sp, #200]
	eor	x22, x22, x9
	str	x22, [sp, #216]
LBB3_2: // in Loop: Header=BB3_3 Depth=1
	cmp	w20, #128
	mov	x20, x19
	b.hs	LBB3_32
LBB3_3: // =>This Loop Header: Depth=1
                                        //     Child Loop BB3_23 Depth 2
                                        //     Child Loop BB3_25 Depth 2
                                        //     Child Loop BB3_30 Depth 2
                                        //     Child Loop BB3_19 Depth 2
	movi	v0.2d, #0000000000000000
	stp	q0, q0, [sp, #160]
	and	w8, w20, #0xff
	lsl	w19, w20, #1
	cmp	w8, #64
	b.eq	LBB3_6
// %bb.4:                                ;   in Loop: Header=BB3_3 Depth=1
	cmp	w8, #128
	b.ne	LBB3_16
// %bb.5:                                ;   in Loop: Header=BB3_3 Depth=1
	ldp	q0, q1, [sp, #224]
	eor	v0.16b, v1.16b, v0.16b
	str	q0, [sp, #160]
	b	LBB3_7
LBB3_6: // in Loop: Header=BB3_3 Depth=1
	ldp	q1, q0, [sp, #224]
	eor	v0.16b, v1.16b, v0.16b
	ext	v1.16b, v0.16b, v0.16b, #8
	eor	v0.8b, v0.8b, v1.8b
	str	d0, [sp, #160]
LBB3_7: // in Loop: Header=BB3_3 Depth=1
	add	x0, sp, #128
	add	x1, sp, #192
	add	x2, sp, #160
	mov	x3, x20
	bl	poly_mul_mod_bits
LBB3_8: // in Loop: Header=BB3_3 Depth=1
	ldp	x8, x9, [sp, #128]
	eor	x10, x9, x8
	ldp	x11, x12, [sp, #144]
	eor	x13, x12, x9
	fmov	d7, x9
	fmov	d0, x10
	eor	x9, x12, x11
	eor	x10, x11, x8
	fmov	d1, x11
	fmov	d6, x8
	ldp	d2, d16, [sp, #80] // 16-byte Folded Reload
	pmull	v17.1q, v0.1d, v2.1d
	ldr	q0, [sp, #96] // 16-byte Folded Reload
	pmull	v0.1q, v1.1d, v0.1d
	ldp	d18, d1, [sp, #64] // 16-byte Folded Reload
	fmov	d2, x12
	pmull	v1.1q, v2.1d, v1.1d
	fmov	d4, x9
	eor	x9, x13, x10
	fmov	d2, x10
	ldp	d19, d3, [sp, #48] // 16-byte Folded Reload
	pmull	v2.1q, v2.1d, v3.1d
	fmov	d3, x13
	fmov	d5, x28
	pmull	v5.1q, v3.1d, v5.1d
	fmov	d3, x9
	pmull	v3.1q, v3.1d, v19.1d
	dup	v19.2d, v0.d[1]
	dup	v20.2d, v1.d[0]
	pmull	v4.1q, v4.1d, v18.1d
	eor	v4.16b, v19.16b, v4.16b
	mov	v18.16b, v0.16b
	mov	v18.d[1], v1.d[1]
	eor3	v4.16b, v4.16b, v20.16b, v18.16b
	ext	v4.16b, v4.16b, v4.16b, #8
	ext	v18.16b, v3.16b, v17.16b, #8
	ldr	q19, [sp, #112] // 16-byte Folded Reload
	pmull	v6.1q, v6.1d, v19.1d
	eor	v18.16b, v6.16b, v18.16b
	zip2	v17.2d, v17.2d, v6.2d
	pmull	v7.1q, v7.1d, v16.1d
	eor	v16.16b, v7.16b, v17.16b
	dup	v17.2d, v7.d[0]
	mov	v19.16b, v0.16b
	mov	v19.d[1], v6.d[0]
	zip2	v20.2d, v2.2d, v7.2d
	dup	v21.2d, v5.d[1]
	mov	v6.d[1], v2.d[1]
	mov	v0.d[1], v2.d[0]
	mov	v2.d[1], v5.d[0]
	mov	v5.d[1], v1.d[1]
	ext	v22.16b, v16.16b, v16.16b, #8
	eor	v16.8b, v16.8b, v22.8b
	fmov	x9, d16
	mov	v17.d[0], x9
	eor3	v17.16b, v17.16b, v18.16b, v19.16b
	eor3	v5.16b, v20.16b, v5.16b, v21.16b
	eor3	v5.16b, v5.16b, v17.16b, v4.16b
	zip2	v1.2d, v1.2d, v17.2d
	mov	v3.d[1], v7.d[1]
	eor	v1.16b, v1.16b, v3.16b
	ext	v3.16b, v1.16b, v1.16b, #8
	eor	v1.8b, v1.8b, v3.8b
	fmov	x9, d1
	mov	v16.d[1], x9
	eor3	v0.16b, v16.16b, v6.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v4.16b
LBB3_9: // in Loop: Header=BB3_3 Depth=1
	ldp	q1, q2, [sp, #224]
	eor	v1.16b, v1.16b, v5.16b
	eor	v0.16b, v2.16b, v0.16b
	stp	q1, q0, [sp, #224]
	and	w9, w20, #0xff
	cmp	w9, #64
	b.eq	LBB3_12
// %bb.10:                               ;   in Loop: Header=BB3_3 Depth=1
	cmp	w9, #128
	b.ne	LBB3_18
// %bb.11:                               ;   in Loop: Header=BB3_3 Depth=1
	eor	v2.16b, v0.16b, v1.16b
	fmov	x9, d2
	umov	x10, v1.d[1]
	b	LBB3_13
LBB3_12: // in Loop: Header=BB3_3 Depth=1
	dup	v2.2d, v1.d[1]
	eor	v3.16b, v2.16b, v1.16b
	str	d3, [sp, #232]
	eor3	v1.16b, v2.16b, v1.16b, v0.16b
	fmov	x9, d1
	mov	x10, x9
LBB3_13: // in Loop: Header=BB3_3 Depth=1
	umov	x11, v0.d[1]
	eor	x10, x10, x11
	stp	x9, x10, [sp, #240]
LBB3_14: // in Loop: Header=BB3_3 Depth=1
	eor	x25, x25, x8
	str	x25, [sp, #192]
	cmp	w20, #64
	b.ne	LBB3_1
// %bb.15:                               ;   in Loop: Header=BB3_3 Depth=1
	eor	x23, x23, x8
	str	x23, [sp, #200]
	b	LBB3_2
LBB3_16: // in Loop: Header=BB3_3 Depth=1
	mov	x8, #-1 // =0xffffffffffffffff
	lsl	x8, x8, x20
	mvn	x27, x8
	cmp	w20, #255
	b.ls	LBB3_22
// %bb.17:                               ;   in Loop: Header=BB3_3 Depth=1
	mov	w8, #0 // =0x0
	mov	x9, #0 // =0x0
	b	LBB3_25
LBB3_18: // in Loop: Header=BB3_3 Depth=1
	mov	w9, w20
	mov	x10, #-1 // =0xffffffffffffffff
	lsl	x11, x10, x20
	mvn	x10, x11
	fmov	x12, d1
	bic	x11, x12, x11
	mov	x12, x20
LBB3_19: // Parent Loop BB3_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	lsr	w13, w12, #6
	ldr	x14, [x24, w13, uxtw #3]
	and	w15, w12, #0x3e
	lsr	x16, x14, x15
	and	x16, x16, x10
	eor	x11, x16, x11
	lsl	x16, x10, x15
	bic	x14, x14, x16
	lsl	x15, x11, x15
	orr	x14, x15, x14
	str	x14, [x24, w13, uxtw #3]
	add	w12, w12, w20
	cmp	w12, #256
	b.lo	LBB3_19
// %bb.20:                               ;   in Loop: Header=BB3_3 Depth=1
	cmp	w20, #63
	b.hi	LBB3_14
// %bb.21:                               ;   in Loop: Header=BB3_3 Depth=1
	and	x8, x8, x10
	lsl	x9, x8, x9
	orr	x8, x9, x8
	eor	x25, x8, x25
	str	x25, [sp, #192]
	b	LBB3_2
LBB3_22: // in Loop: Header=BB3_3 Depth=1
	mov	w10, #0 // =0x0
	mov	x9, #0 // =0x0
	mov	x12, #0 // =0x0
	mov	w8, #255 // =0xff
	udiv	w8, w8, w20
	add	w11, w8, #1
	and	w13, w11, #0x1fe
	mul	w8, w13, w20
	mov	x14, x13
LBB3_23: // Parent Loop BB3_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	add	w15, w20, w10
	lsr	w16, w10, #6
	lsr	w17, w15, #6
	ldr	x16, [x24, w16, uxtw #3]
	ldr	x17, [x24, w17, uxtw #3]
	and	w0, w10, #0x3c
	and	w15, w15, #0x3e
	lsr	x16, x16, x0
	lsr	x15, x17, x15
	and	x16, x16, x27
	and	x15, x15, x27
	eor	x9, x16, x9
	eor	x12, x15, x12
	add	w10, w10, w19
	subs	w14, w14, #2
	b.ne	LBB3_23
// %bb.24:                               ;   in Loop: Header=BB3_3 Depth=1
	eor	x9, x12, x9
	cmp	w11, w13
	b.eq	LBB3_26
LBB3_25: // Parent Loop BB3_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	lsr	w10, w8, #6
	ldr	x10, [x24, w10, uxtw #3]
	and	w11, w8, #0x3e
	lsr	x10, x10, x11
	and	x10, x10, x27
	eor	x9, x10, x9
	add	w8, w8, w20
	cmp	w8, #256
	b.lo	LBB3_25
LBB3_26: // in Loop: Header=BB3_3 Depth=1
	str	x9, [sp, #160]
	add	x0, sp, #128
	add	x1, sp, #192
	add	x2, sp, #160
	mov	x3, x20
	bl	poly_mul_mod_bits
	cmp	w20, #32
	b.hi	LBB3_8
// %bb.27:                               ;   in Loop: Header=BB3_3 Depth=1
	ldr	x8, [sp, #128]
	ands	x9, x8, x27
	ldp	q7, q6, [sp, #96] // 32-byte Folded Reload
	movi	v5.2d, #0000000000000000
	movi	v0.2d, #0000000000000000
	b.eq	LBB3_9
// %bb.28:                               ;   in Loop: Header=BB3_3 Depth=1
	ldp	q17, q16, [sp, #16] // 32-byte Folded Reload
	b	LBB3_30
LBB3_29: // in Loop: Header=BB3_30 Depth=2
	eor	v5.16b, v5.16b, v6.16b
	eor	v0.16b, v0.16b, v7.16b
	sub	x10, x9, #1
	ands	x9, x10, x9
	b.eq	LBB3_9
LBB3_30: // Parent Loop BB3_3 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	rbit	x10, x9
	clz	x10, x10
	cbz	x10, LBB3_29
// %bb.31:                               ;   in Loop: Header=BB3_30 Depth=2
	sub	x11, x26, x10
	dup	v1.2d, x10
	ushl	v2.2d, v6.2d, v1.2d
	dup	v3.2d, x11
	ushl	v1.2d, v7.2d, v1.2d
	neg	v3.2d, v3.2d
	ushl	v4.2d, v17.2d, v3.2d
	eor3	v0.16b, v1.16b, v4.16b, v0.16b
	ushl	v1.2d, v16.2d, v3.2d
	eor3	v5.16b, v2.16b, v1.16b, v5.16b
	sub	x10, x9, #1
	ands	x9, x10, x9
	b.ne	LBB3_30
	b	LBB3_9
LBB3_32:
	and	w8, w25, #0xff
	dup	v1.8h, w8
	ubfx	w8, w25, #8, #8
	dup	v2.8h, w8
	ubfx	w8, w25, #16, #8
	dup	v3.8h, w8
	ubfx	x8, x25, #24, #8
	dup	v4.8h, w8
	ubfx	x8, x25, #32, #8
	dup	v5.8h, w8
	ubfx	x8, x25, #40, #8
	dup	v6.8h, w8
	ubfx	x8, x25, #48, #8
	dup	v7.8h, w8
Lloh4:
	adrp	x8, lCPI3_1
Lloh5:
	ldr	q0, [x8, :lo12:lCPI3_1]
	sshl	v16.8h, v1.8h, v0.8h
	lsr	x8, x25, #56
	dup	v17.8h, w8
	movi	v1.8h, #1
	and	w8, w23, #0xff
	dup	v18.8h, w8
	ubfx	w8, w23, #8, #8
	dup	v19.8h, w8
	and	v16.16b, v16.16b, v1.16b
	ubfx	w8, w23, #16, #8
	dup	v20.8h, w8
	ubfx	x8, x23, #24, #8
	dup	v21.8h, w8
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	ldr	x11, [sp, #8] // 8-byte Folded Reload
	stp	q16, q2, [x11]
	and	v2.16b, v3.16b, v1.16b
	ubfx	x8, x23, #32, #8
	dup	v3.8h, w8
	sshl	v4.8h, v4.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	sshl	v5.8h, v5.8h, v0.8h
	stp	q2, q4, [x11, #32]
	ubfx	x8, x23, #40, #8
	dup	v2.8h, w8
	and	v4.16b, v5.16b, v1.16b
	sshl	v5.8h, v6.8h, v0.8h
	and	v5.16b, v5.16b, v1.16b
	stp	q4, q5, [x11, #64]
	sshl	v4.8h, v7.8h, v0.8h
	ubfx	x8, x23, #48, #8
	dup	v5.8h, w8
	and	v4.16b, v4.16b, v1.16b
	sshl	v6.8h, v17.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	stp	q4, q6, [x11, #96]
	sshl	v4.8h, v18.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	sshl	v6.8h, v19.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	stp	q4, q6, [x11, #128]
	sshl	v4.8h, v20.8h, v0.8h
	and	v4.16b, v4.16b, v1.16b
	sshl	v6.8h, v21.8h, v0.8h
	and	v6.16b, v6.16b, v1.16b
	stp	q4, q6, [x11, #160]
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	stp	q3, q2, [x11, #192]
	lsr	x8, x23, #56
	dup	v2.8h, w8
	sshl	v3.8h, v5.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	stp	q3, q2, [x11, #224]
	ldp	x9, x8, [sp, #208]
	and	w10, w9, #0xff
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	w10, w9, #8, #8
	dup	v3.8h, w10
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #256]
	ubfx	w10, w9, #16, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x10, x9, #24, #8
	dup	v3.8h, w10
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #288]
	ubfx	x10, x9, #32, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x10, x9, #40, #8
	dup	v3.8h, w10
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #320]
	ubfx	x10, x9, #48, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	lsr	x9, x9, #56
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #352]
	and	w9, w8, #0xff
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	w9, w8, #8, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #384]
	ubfx	w9, w8, #16, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #24, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #416]
	ubfx	x9, x8, #32, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #40, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x11, #448]
	ubfx	x9, x8, #48, #8
	lsr	x8, x8, #56
	dup	v2.8h, w9
	dup	v3.8h, w8
	sshl	v2.8h, v2.8h, v0.8h
	sshl	v0.8h, v3.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	and	v0.16b, v0.16b, v1.16b
	stp	q2, q0, [x11, #480]
	ldp	x29, x30, [sp, #336] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #320] // 16-byte Folded Reload
	ldp	x22, x21, [sp, #304] // 16-byte Folded Reload
	ldp	x24, x23, [sp, #288] // 16-byte Folded Reload
	ldp	x26, x25, [sp, #272] // 16-byte Folded Reload
	ldp	x28, x27, [sp, #256] // 16-byte Folded Reload
	add	sp, sp, #352
	ret
	.cfi_endproc
                                        // -- End function
	.p2align	2 // -- Begin function poly_mul_mod_bits
poly_mul_mod_bits: // @poly_mul_mod_bits
	.cfi_startproc
// %bb.0:
                                        // kill: def $w3 killed $w3 def $x3
	movi	v0.2d, #0000000000000000
	stp	q0, q0, [x0]
	ror	w8, w3, #6
	cmp	w8, #1
	b.gt	LBB4_5
// %bb.1:
	cbz	w8, LBB4_26
// %bb.2:
	cmp	w8, #1
	b.ne	LBB4_8
// %bb.3:
	ldr	d0, [x2]
	ldr	d1, [x1]
	pmull	v0.1q, v1.1d, v0.1d
	dup	v1.2d, v0.d[1]
	eor	v0.16b, v1.16b, v0.16b
	str	d0, [x0]
LBB4_4:
	ret
LBB4_5:
	cmp	w8, #2
	b.eq	LBB4_14
// %bb.6:
	cmp	w8, #4
	b.ne	LBB4_8
// %bb.7:
	ldp	x8, x9, [x1]
	ldp	x10, x11, [x2]
	eor	x12, x11, x10
	ldp	x13, x14, [x2, #16]
	eor	x15, x13, x10
	fmov	d5, x10
	eor	x10, x9, x8
	ldp	x16, x17, [x1, #16]
	eor	x1, x16, x8
	fmov	d7, x8
	eor	x8, x14, x11
	fmov	d16, x11
	eor	x11, x17, x9
	fmov	d17, x9
	fmov	d0, x12
	fmov	d1, x10
	pmull	v18.1q, v1.1d, v0.1d
	eor	x9, x14, x13
	fmov	d0, x13
	eor	x10, x17, x16
	fmov	d1, x16
	pmull	v0.1q, v1.1d, v0.1d
	fmov	d1, x14
	fmov	d2, x17
	pmull	v1.1q, v2.1d, v1.1d
	fmov	d4, x9
	fmov	d19, x10
	eor	x9, x8, x15
	fmov	d2, x15
	eor	x10, x11, x1
	fmov	d3, x1
	pmull	v2.1q, v3.1d, v2.1d
	fmov	d3, x8
	fmov	d6, x11
	pmull	v6.1q, v6.1d, v3.1d
	fmov	d3, x9
	fmov	d20, x10
	dup	v21.2d, v0.d[1]
	pmull	v3.1q, v20.1d, v3.1d
	pmull	v4.1q, v19.1d, v4.1d
	eor	v4.16b, v21.16b, v4.16b
	dup	v19.2d, v1.d[0]
	mov	v20.16b, v0.16b
	mov	v20.d[1], v1.d[1]
	eor3	v4.16b, v4.16b, v19.16b, v20.16b
	ext	v4.16b, v4.16b, v4.16b, #8
	ext	v19.16b, v3.16b, v18.16b, #8
	pmull	v5.1q, v7.1d, v5.1d
	eor	v7.16b, v5.16b, v19.16b
	zip2	v18.2d, v18.2d, v5.2d
	pmull	v16.1q, v17.1d, v16.1d
	eor	v17.16b, v16.16b, v18.16b
	dup	v18.2d, v16.d[0]
	mov	v19.16b, v0.16b
	mov	v19.d[1], v5.d[0]
	zip2	v20.2d, v2.2d, v16.2d
	dup	v21.2d, v6.d[1]
	mov	v5.d[1], v2.d[1]
	mov	v0.d[1], v2.d[0]
	mov	v2.d[1], v6.d[0]
	mov	v6.d[1], v1.d[1]
	ext	v22.16b, v17.16b, v17.16b, #8
	eor	v17.8b, v17.8b, v22.8b
	fmov	x8, d17
	mov	v18.d[0], x8
	eor3	v7.16b, v18.16b, v7.16b, v19.16b
	eor3	v6.16b, v20.16b, v6.16b, v21.16b
	eor3	v6.16b, v6.16b, v7.16b, v4.16b
	zip2	v1.2d, v1.2d, v7.2d
	mov	v3.d[1], v16.d[1]
	eor	v1.16b, v1.16b, v3.16b
	ext	v3.16b, v1.16b, v1.16b, #8
	eor	v1.8b, v1.8b, v3.8b
	fmov	x8, d1
	mov	v17.d[1], x8
	eor3	v0.16b, v17.16b, v5.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v4.16b
	stp	q6, q0, [x0]
	ret
LBB4_8:
	add	w8, w3, #63
	lsr	w12, w8, #6
	mov	x8, #-1 // =0xffffffffffffffff
	lsl	x11, x8, x3
	cmp	w3, #64
	csinv	x8, x8, x11, hs
	cmp	w3, #63
	b.hi	LBB4_15
// %bb.9:
	mov	x9, #0 // =0x0
	mov	x10, #0 // =0x0
	mvn	x11, x11
	b	LBB4_11
LBB4_10: // in Loop: Header=BB4_11 Depth=1
	add	x10, x10, #1
	cmp	x10, x12
	b.eq	LBB4_25
LBB4_11: // =>This Loop Header: Depth=1
                                        //     Child Loop BB4_13 Depth 2
	ldr	x13, [x1, x10, lsl #3]
	ands	x13, x13, x8
	b.eq	LBB4_10
// %bb.12:                               ;   in Loop: Header=BB4_11 Depth=1
	lsl	w14, w10, #6
LBB4_13: // Parent Loop BB4_11 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	rbit	x15, x13
	clz	x15, x15
	orr	w15, w14, w15
	ldr	x16, [x2]
	and	x16, x16, x11
	lsl	x17, x16, x15
	sub	w4, w3, w15
	lsr	x4, x16, x4
	orr	x17, x17, x4
	and	x17, x17, x11
	cmp	w15, #0
	csel	x15, x16, x17, eq
	eor	x9, x9, x15
	str	x9, [x0]
	sub	x15, x13, #1
	ands	x13, x15, x13
	b.ne	LBB4_13
	b	LBB4_10
LBB4_14:
	ldp	x8, x9, [x1]
	ldp	x10, x11, [x2]
	eor	x12, x11, x10
	fmov	d0, x10
	eor	x10, x9, x8
	fmov	d1, x8
	pmull	v0.1q, v1.1d, v0.1d
	fmov	d1, x11
	fmov	d2, x9
	fmov	d3, x12
	dup	v4.2d, v0.d[1]
	fmov	d5, x10
	pmull	v1.1q, v2.1d, v1.1d
	eor	v0.16b, v0.16b, v1.16b
	dup	v0.2d, v0.d[0]
	pmull	v2.1q, v5.1d, v3.1d
	eor	v2.16b, v4.16b, v2.16b
	dup	v1.2d, v1.d[1]
	eor3	v0.16b, v0.16b, v2.16b, v1.16b
	ext	v0.16b, v0.16b, v0.16b, #8
	str	q0, [x0]
	ret
LBB4_15:
	stp	x20, x19, [sp, #-16]! // 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset w19, -8
	.cfi_offset w20, -16
	mov	x9, #0 // =0x0
	lsr	w10, w3, #6
	sub	w11, w10, #1
	cmp	w12, #1
	csinc	w12, w12, wzr, hi
	mov	w13, #64 // =0x40
	b	LBB4_17
LBB4_16: // in Loop: Header=BB4_17 Depth=1
	add	x9, x9, #1
	cmp	x9, x12
	b.eq	LBB4_24
LBB4_17: // =>This Loop Header: Depth=1
                                        //     Child Loop BB4_19 Depth 2
                                        //       Child Loop BB4_21 Depth 3
                                        //       Child Loop BB4_23 Depth 3
	ldr	x14, [x1, x9, lsl #3]
	ands	x14, x14, x8
	b.ne	LBB4_19
	b	LBB4_16
LBB4_18: // in Loop: Header=BB4_19 Depth=2
	sub	x15, x14, #1
	ands	x14, x15, x14
	b.eq	LBB4_16
LBB4_19: // Parent Loop BB4_17 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB4_21 Depth 3
                                        //       Child Loop BB4_23 Depth 3
	rbit	x15, x14
	clz	x15, x15
	cbz	x15, LBB4_22
// %bb.20:                               ;   in Loop: Header=BB4_19 Depth=2
	sub	x16, x13, x15
	mov	x17, x9
	mov	x4, x2
	mov	x5, x10
LBB4_21: // Parent Loop BB4_17 Depth=1
                                        //     Parent Loop BB4_19 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	and	w6, w17, w11
	ldr	x7, [x4], #8
	lsl	x19, x7, x15
	ldr	x20, [x0, w6, uxtw #3]
	eor	x19, x20, x19
	str	x19, [x0, w6, uxtw #3]
	add	w6, w6, #1
	and	w6, w6, w11
	ldr	x19, [x0, w6, uxtw #3]
	lsr	x7, x7, x16
	eor	x7, x19, x7
	str	x7, [x0, w6, uxtw #3]
	add	w17, w17, #1
	subs	x5, x5, #1
	b.ne	LBB4_21
	b	LBB4_18
LBB4_22: // in Loop: Header=BB4_19 Depth=2
	mov	x15, x9
	mov	x16, x2
	mov	x17, x10
LBB4_23: // Parent Loop BB4_17 Depth=1
                                        //     Parent Loop BB4_19 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	and	w4, w15, w11
	ldr	x5, [x16], #8
	ldr	x6, [x0, w4, uxtw #3]
	eor	x5, x6, x5
	str	x5, [x0, w4, uxtw #3]
	add	w15, w15, #1
	subs	x17, x17, #1
	b.ne	LBB4_23
	b	LBB4_18
LBB4_24:
	ldp	x20, x19, [sp], #16 // 16-byte Folded Reload
LBB4_25:
	cmp	w3, #63
	b.hi	LBB4_4
LBB4_26:
	mov	x8, #-1 // =0xffffffffffffffff
	lsl	x8, x8, x3
	ldr	x9, [x0]
	bic	x8, x9, x8
	str	x8, [x0]
	ret
	.cfi_endproc
                                        // -- End function
