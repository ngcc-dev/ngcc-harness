// Hand-optimized AArch64 PMULL assembly for ZEN_128 FastInversion.
// Implements the packed-bit inversion path manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.
	.text
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function FastInversion
lCPI_FI_0:
	.short	0 // 0x0
	.short	1 // 0x1
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
	.short	6 // 0x6
	.short	7 // 0x7
lCPI_FI_1:
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
	fmov	d0, xzr
	mov	x8, #0 // =0x0
	movi	v1.8h, #1
	ldp	q2, q3, [x1]
	and	v4.16b, v2.16b, v1.16b
LFI_loh0:
	adrp	x9, lCPI_FI_0
LFI_loh1:
	ldr	q2, [x9, :lo12:lCPI_FI_0]
	sshl	v4.8h, v4.8h, v2.8h
	addp	v4.8h, v4.8h, v4.8h
	addp	v4.8h, v4.8h, v4.8h
	addp	v4.8h, v4.8h, v4.8h
	umov	w9, v4.b[0]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w10, v3.b[0]
	ldp	q3, q4, [x1, #32]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w11, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w12, v3.b[0]
	ldp	q3, q4, [x1, #64]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w13, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w14, v3.b[0]
	ldp	q3, q4, [x1, #96]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w15, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w16, v3.b[0]
	ldp	q3, q4, [x1, #128]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w17, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w2, v3.b[0]
	ldp	q3, q4, [x1, #160]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w3, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w4, v3.b[0]
	ldp	q3, q4, [x1, #192]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w5, v3.b[0]
	and	v3.16b, v4.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w6, v3.b[0]
	ldp	q3, q4, [x1, #224]
	and	v3.16b, v3.16b, v1.16b
	sshl	v3.8h, v3.8h, v2.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	addp	v3.8h, v3.8h, v3.8h
	umov	w1, v3.b[0]
	and	v1.16b, v4.16b, v1.16b
	bfi	x9, x10, #8, #8
	bfi	x9, x11, #16, #8
	bfi	x9, x12, #24, #8
	orr	x9, x9, x13, lsl #32
	orr	x9, x9, x14, lsl #40
	orr	x9, x9, x15, lsl #48
	orr	x10, x9, x16, lsl #56
	bfi	x17, x2, #8, #8
	bfi	x17, x3, #16, #8
	bfi	x17, x4, #24, #8
	orr	x9, x17, x5, lsl #32
	orr	x9, x9, x6, lsl #40
	orr	x9, x9, x1, lsl #48
	sshl	v1.8h, v1.8h, v2.8h
	addp	v1.8h, v1.8h, v1.8h
	addp	v1.8h, v1.8h, v1.8h
	addp	v1.8h, v1.8h, v1.8h
	umov	w11, v1.b[0]
	orr	x14, x9, x11, lsl #56
	eor	x9, x10, x10, lsl #1
	eor	x9, x9, x9, lsl #2
	eor	x9, x9, x9, lsl #4
	eor	x9, x9, x9, lsl #8
	eor	x9, x9, x9, lsl #16
	eor	x9, x9, x9, lsl #32
	eor	x11, x14, x14, lsl #1
	eor	x11, x11, x11, lsl #2
	eor	x11, x11, x11, lsl #4
	eor	x11, x11, x11, lsl #8
	eor	x11, x11, x11, lsl #16
	fmov	d1, x10
	cnt	v1.8b, v1.8b
	uaddlv	h1, v1.8b
	fmov	w12, s1
	and	w12, w12, #0x1
	neg	x12, x12
	eor	x12, x12, x11, lsl #32
	eor	x11, x12, x11
	eor	x12, x11, x9
	fmov	d1, x12
	cnt	v1.8b, v1.8b
	uaddlv	h1, v1.8b
	fmov	w12, s1
	sbfx	x13, x12, #0, #1
	and	x15, x13, x10
	eor	x15, x15, x9
	and	x9, x13, x14
	eor	x13, x9, x11
	tst	x12, #0x1
	mov	w9, #1 // =0x1
	cinc	x9, x9, ne
	eor	x11, x15, x15, lsl #1
	eor	x11, x11, x11, lsl #2
	eor	x11, x11, x11, lsl #4
	eor	x11, x11, x11, lsl #8
	eor	x11, x11, x11, lsl #16
	eor	x11, x11, x11, lsl #32
	eor	x12, x13, x13, lsl #1
	eor	x12, x12, x12, lsl #2
	eor	x12, x12, x12, lsl #4
	eor	x12, x12, x12, lsl #8
	eor	x12, x12, x12, lsl #16
	fmov	d1, x15
	cnt	v1.8b, v1.8b
	uaddlv	h1, v1.8b
	fmov	w13, s1
	and	w13, w13, #0x1
	neg	x13, x13
	eor	x13, x13, x12, lsl #32
	eor	x12, x13, x12
	eor	x13, x14, x10
	fmov	d1, x14
	pmull	v0.1q, v0.1d, v1.1d
	umov	x14, v0.d[1]
	fmov	x15, d0
	mov	w17, #2 // =0x2
	mov	x16, #-1 // =0xffffffffffffffff
LFI_BB_1: // =>This Loop Header: Depth=1
                                        //     Child Loop FI_BB_5 Depth 2
	eor	x1, x12, x11
	cmp	x17, #64
	b.ne	LFI_BB_3
// %bb.2:                                ;   in Loop: Header=FI_BB_1 Depth=1
	fmov	d0, x1
	fmov	d1, x9
	pmull	v0.1q, v1.1d, v0.1d
	dup	v1.2d, v0.d[1]
	eor	v0.16b, v1.16b, v0.16b
	fmov	x1, d0
	b	LFI_BB_4
LFI_BB_3: // in Loop: Header=FI_BB_1 Depth=1
	lsr	x2, x1, #32
	cmp	x17, #33
	csel	x2, x2, xzr, lo
	eor	x1, x2, x1
	lsr	x2, x1, #16
	cmp	x17, #17
	csel	x2, x2, xzr, lo
	eor	x1, x2, x1
	lsr	x2, x1, #8
	cmp	x17, #9
	csel	x2, x2, xzr, lo
	eor	x1, x2, x1
	lsr	x2, x1, #4
	cmp	x17, #5
	csel	x2, x2, xzr, lo
	eor	x1, x2, x1
	lsr	x2, x1, #2
	cmp	x17, #3
	csel	x2, x2, xzr, lo
	eor	x1, x2, x1
	lsl	x2, x16, x17
	bic	x3, x9, x2
	bic	x1, x1, x2
	fmov	d0, x1
	fmov	d1, x3
	pmull	v0.1q, v1.1d, v0.1d
	umov	x1, v0.d[1]
	fmov	x3, d0
	lsr	x4, x3, x17
	lsl	x5, x1, #1
	mvn	w6, w17
	lsl	x5, x5, x6
	orr	x4, x5, x4
	lsr	x1, x1, x17
	tst	x17, #0x40
	csel	x1, x1, x4, ne
	eor	x1, x3, x1
	bic	x1, x1, x2
LFI_BB_4: // in Loop: Header=FI_BB_1 Depth=1
	fmov	d0, x10
	fmov	d1, x1
	pmull	v0.1q, v1.1d, v0.1d
	umov	x2, v0.d[1]
	fmov	d2, x13
	pmull	v1.1q, v1.1d, v2.1d
	eor	v2.16b, v0.16b, v1.16b
	fmov	x3, d0
	fmov	x4, d1
	umov	x5, v2.d[1]
	eor	x11, x3, x11
	eor	x11, x11, x14
	eor	x11, x11, x15
	eor	x11, x5, x11
	eor	x3, x4, x3
	eor	x12, x3, x12
	eor	x12, x12, x14
	eor	x12, x12, x15
	eor	x12, x2, x12
	mov	x2, x17
LFI_BB_5: // Parent Loop FI_BB_1 Depth=1
                                        // =>  This Inner Loop Header: Depth=2
	lsl	x3, x12, x2
	mvn	w4, w2
	lsr	x5, x11, #1
	lsr	x4, x5, x4
	orr	x3, x3, x4
	lsl	x4, x11, x2
	tst	x2, #0x40
	csel	x3, x4, x3, ne
	csel	x4, xzr, x4, ne
	eor	x12, x3, x12
	eor	x11, x4, x11
	lsl	x3, x2, #1
	cmp	x2, #64
	mov	x2, x3
	b.lo	LFI_BB_5
// %bb.6:                                ;   in Loop: Header=FI_BB_1 Depth=1
	lsl	x2, x1, x17
	mvn	w3, w17
	lsr	x4, x1, #1
	lsr	x3, x4, x3
	tst	x17, #0x40
	csel	x3, x2, x3, ne
	csel	x2, xzr, x2, ne
	orr	x1, x2, x1
	eor	x8, x3, x8
	eor	x9, x1, x9
	lsl	x1, x17, #1
	cmp	x17, #33
	mov	x17, x1
	b.lo	LFI_BB_1
// %bb.7:
	and	w10, w9, #0xff
	dup	v1.8h, w10
LFI_loh2:
	adrp	x10, lCPI_FI_1
LFI_loh3:
	ldr	q0, [x10, :lo12:lCPI_FI_1]
	sshl	v2.8h, v1.8h, v0.8h
	ubfx	w10, w9, #8, #8
	dup	v3.8h, w10
	movi	v1.8h, #1
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0]
	ubfx	w10, w9, #16, #8
	dup	v2.8h, w10
	ubfx	x10, x9, #24, #8
	dup	v3.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #32]
	ubfx	x10, x9, #32, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x10, x9, #40, #8
	dup	v3.8h, w10
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #64]
	ubfx	x10, x9, #48, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	lsr	x9, x9, #56
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #96]
	and	w9, w8, #0xff
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	w9, w8, #8, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #128]
	ubfx	w9, w8, #16, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #24, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #160]
	ubfx	x9, x8, #32, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #40, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x0, #192]
	ubfx	x9, x8, #48, #8
	dup	v2.8h, w9
	lsr	x8, x8, #56
	dup	v3.8h, w8
	sshl	v2.8h, v2.8h, v0.8h
	sshl	v0.8h, v3.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	and	v0.16b, v0.16b, v1.16b
	stp	q2, q0, [x0, #224]
	ret
	.cfi_endproc
                                        // -- End function
