// Hand-optimized AArch64 assembly kernels for ZEN_128 NTT arithmetic.
// Derived from split assembly kernels and manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.

// begin expanded include: ntt_asm_parts/00_text_header.inc
	.text
// end expanded include: ntt_asm_parts/00_text_header.inc
// begin expanded include: ntt_asm_parts/10_montgomery_reduce.inc
	.globl	montgomery_reduce // -- Begin function montgomery_reduce
	.p2align	2
montgomery_reduce: // @montgomery_reduce
	.cfi_startproc
// %bb.0:
	mov	w8, #-50266112 // =0xfd010000
	mul	w8, w0, w8
	asr	w8, w8, #16
	mov	w9, #-769 // =0xfffffcff
	madd	w8, w8, w9, w0
	asr	w0, w8, #16
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/10_montgomery_reduce.inc
// begin expanded include: ntt_asm_parts/20_poly_ntt.inc
	.globl	poly_ntt // -- Begin function poly_ntt
	.p2align	2
poly_ntt: // @poly_ntt
	.cfi_startproc
// %bb.0:
	mov	w9, #1 // =0x1
	mov	w15, #256 // =0x100
	mov	w8, #511 // =0x1ff
	mov	w10, #64769 // =0xfd01
	fmov	s0, w10
	mov	w10, #769 // =0x301
	dup	v1.8h, w10
Lloh0:
	adrp	x10, f_barrett_pairs_0_31
Lloh1:
	add	x10, x10, :lo12:f_barrett_pairs_0_31
LBB1_1: // =>This Loop Header: Depth=1
                                        //     Child Loop BB1_2 Depth 2
                                        //       Child Loop BB1_3 Depth 3
	mov	x12, #0 // =0x0
	mov	x13, x9
	mov	w11, w15
	lsl	w9, w15, #1
	ubfiz	x14, x15, #1, #32
	udiv	w9, w8, w9
	add	w9, w13, w9
	add	w9, w9, #1
	ubfiz	x15, x15, #2, #32
	mov	x16, x0
LBB1_2: // Parent Loop BB1_1 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB1_3 Depth 3
	ldr	s2, [x10, w13, uxtw #2]
	add	x17, x12, x11
	mov	x1, x16
	mov	x2, x12
LBB1_3: // Parent Loop BB1_1 Depth=1
                                        //     Parent Loop BB1_2 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x1]
	add	x3, x1, x14
	ldp	q6, q7, [x3]
	sqrdmulh	v16.8h, v6.8h, v2.h[1]
	sqrdmulh	v17.8h, v7.8h, v2.h[1]
	mul	v6.8h, v6.8h, v2.h[0]
	mul	v7.8h, v7.8h, v2.h[0]
	mls	v6.8h, v16.8h, v1.8h
	mls	v7.8h, v17.8h, v1.8h
	add	v16.8h, v4.8h, v6.8h
	add	v17.8h, v5.8h, v7.8h
	stp	q16, q17, [x1], #32
	sub	v4.8h, v4.8h, v6.8h
	sub	v5.8h, v5.8h, v7.8h
	stp	q4, q5, [x3]
	add	x2, x2, #16
	cmp	x2, x17
	b.lo	LBB1_3
// %bb.4:                                ;   in Loop: Header=BB1_2 Depth=2
	add	w13, w13, #1
	add	x12, x12, x14
	add	x16, x16, x15
	cmp	w13, w9
	b.ne	LBB1_2
// %bb.5:                                ;   in Loop: Header=BB1_1 Depth=1
	ubfx	x15, x11, #1, #31
                                        // kill: def $w15 killed $w15 killed $x15 def $x15
	cmp	w11, #31
	b.hi	LBB1_1
// %bb.6:
Lloh2:
	adrp	x8, f+64
Lloh3:
	add	x8, x8, :lo12:f+64
	add	x9, x8, #66
	add	x10, x0, #16
	mov	x11, #-16 // =0xfffffffffffffff0
	mov	w12, #64769 // =0xfd01
	dup	v0.8h, w12
	fmov	s1, w12
	mov	w12, #769 // =0x301
	dup	v2.8h, w12
LBB1_7: // =>This Inner Loop Header: Depth=1
	ldrh	w12, [x8], #2
	dup	v3.8h, w12
	fmov	s4, w12
	mul	v4.8h, v4.8h, v1.8h
	sub	x12, x9, #2
	ld1r	{ v5.4h }, [x12]
	ld1r	{ v6.8h }, [x9]
	ldp	q7, q16, [x10, #-16]
	smull	v17.4s, v16.4h, v3.4h
	smull2	v3.4s, v16.8h, v3.8h
	mul	v4.8h, v16.8h, v4.h[0]
	smull	v16.4s, v4.4h, v2.4h
	smull2	v4.4s, v4.8h, v2.8h
	subhn	v16.4h, v17.4s, v16.4s
	subhn2	v16.8h, v3.4s, v4.4s
	add	v3.8h, v7.8h, v16.8h
	sub	v4.8h, v7.8h, v16.8h
	zip2	v7.2d, v3.2d, v4.2d
	smull	v16.4s, v7.4h, v5.4h
	mov	v5.d[1], v6.d[0]
	smull2	v6.4s, v7.8h, v6.8h
	mul	v5.8h, v5.8h, v0.8h
	mul	v5.8h, v5.8h, v7.8h
	smull	v7.4s, v5.4h, v2.4h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v7.4h, v16.4s, v7.4s
	subhn	v5.4h, v6.4s, v5.4s
	add	v6.4h, v3.4h, v7.4h
	sub	v3.4h, v3.4h, v7.4h
	mov	v6.d[1], v3.d[0]
	add	v3.4h, v4.4h, v5.4h
	sub	v4.4h, v4.4h, v5.4h
	mov	v3.d[1], v4.d[0]
	stp	q6, q3, [x10, #-16]
	add	x9, x9, #4
	add	x11, x11, #16
	add	x10, x10, #32
	cmp	x11, #496
	b.lo	LBB1_7
// %bb.8:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/20_poly_ntt.inc
// begin expanded include: ntt_asm_parts/30_poly_ntt_mq.inc
	.globl	poly_ntt_mq // -- Begin function poly_ntt_mq
	.p2align	2
poly_ntt_mq: // @poly_ntt_mq
	.cfi_startproc
// %bb.0:
	mov	w9, #1 // =0x1
	mov	w15, #256 // =0x100
	mov	w8, #511 // =0x1ff
	mov	w10, #64769 // =0xfd01
	fmov	s0, w10
	mov	w10, #769 // =0x301
	dup	v1.8h, w10
Lloh4:
	adrp	x10, f_barrett_pairs_0_31
Lloh5:
	add	x10, x10, :lo12:f_barrett_pairs_0_31
LBB2_1: // =>This Loop Header: Depth=1
                                        //     Child Loop BB2_2 Depth 2
                                        //       Child Loop BB2_3 Depth 3
	mov	x12, #0 // =0x0
	mov	x13, x9
	mov	w11, w15
	lsl	w9, w15, #1
	ubfiz	x14, x15, #1, #32
	udiv	w9, w8, w9
	add	w9, w13, w9
	add	w9, w9, #1
	ubfiz	x15, x15, #2, #32
	mov	x16, x0
LBB2_2: // Parent Loop BB2_1 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB2_3 Depth 3
	ldr	s2, [x10, w13, uxtw #2]
	add	x17, x12, x11
	mov	x1, x16
	mov	x2, x12
LBB2_3: // Parent Loop BB2_1 Depth=1
                                        //     Parent Loop BB2_2 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x1]
	add	x3, x1, x14
	ldp	q6, q7, [x3]
	sqrdmulh	v16.8h, v6.8h, v2.h[1]
	sqrdmulh	v17.8h, v7.8h, v2.h[1]
	mul	v6.8h, v6.8h, v2.h[0]
	mul	v7.8h, v7.8h, v2.h[0]
	mls	v6.8h, v16.8h, v1.8h
	mls	v7.8h, v17.8h, v1.8h
	add	v16.8h, v4.8h, v6.8h
	add	v17.8h, v5.8h, v7.8h
	stp	q16, q17, [x1], #32
	sub	v4.8h, v4.8h, v6.8h
	sub	v5.8h, v5.8h, v7.8h
	stp	q4, q5, [x3]
	add	x2, x2, #16
	cmp	x2, x17
	b.lo	LBB2_3
// %bb.4:                                ;   in Loop: Header=BB2_2 Depth=2
	add	w13, w13, #1
	add	x12, x12, x14
	add	x16, x16, x15
	cmp	w13, w9
	b.ne	LBB2_2
// %bb.5:                                ;   in Loop: Header=BB2_1 Depth=1
	ubfx	x15, x11, #1, #31
                                        // kill: def $w15 killed $w15 killed $x15 def $x15
	cmp	w11, #31
	b.hi	LBB2_1
// %bb.6:
Lloh6:
	adrp	x8, f+64
Lloh7:
	add	x8, x8, :lo12:f+64
	add	x9, x8, #66
	add	x10, x0, #16
	mov	x11, #-16 // =0xfffffffffffffff0
	mov	w12, #64769 // =0xfd01
	dup	v0.8h, w12
	fmov	s1, w12
	mov	w12, #769 // =0x301
	dup	v2.8h, w12
	mov	w12, #43 // =0x2b
	dup	v24.8h, w12
LBB2_7: // =>This Inner Loop Header: Depth=1
	ldrh	w12, [x8], #2
	dup	v3.8h, w12
	fmov	s4, w12
	mul	v4.8h, v4.8h, v1.8h
	sub	x12, x9, #2
	ld1r	{ v5.4h }, [x12]
	ld1r	{ v6.8h }, [x9]
	ldp	q7, q16, [x10, #-16]
	smull	v17.4s, v16.4h, v3.4h
	smull2	v3.4s, v16.8h, v3.8h
	mul	v4.8h, v16.8h, v4.h[0]
	smull	v16.4s, v4.4h, v2.4h
	smull2	v4.4s, v4.8h, v2.8h
	subhn	v16.4h, v17.4s, v16.4s
	subhn2	v16.8h, v3.4s, v4.4s
	add	v3.8h, v7.8h, v16.8h
	sub	v4.8h, v7.8h, v16.8h
	zip2	v7.2d, v3.2d, v4.2d
	smull	v16.4s, v7.4h, v5.4h
	mov	v5.d[1], v6.d[0]
	smull2	v6.4s, v7.8h, v6.8h
	mul	v5.8h, v5.8h, v0.8h
	mul	v5.8h, v5.8h, v7.8h
	smull	v7.4s, v5.4h, v2.4h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v7.4h, v16.4s, v7.4s
	subhn	v5.4h, v6.4s, v5.4s
	add	v6.4h, v3.4h, v7.4h
	sub	v3.4h, v3.4h, v7.4h
	mov	v6.d[1], v3.d[0]
	add	v3.4h, v4.4h, v5.4h
	sub	v4.4h, v4.4h, v5.4h
	mov	v3.d[1], v4.d[0]
	sqrdmulh	v16.8h, v6.8h, v24.8h
	mls	v6.8h, v16.8h, v2.8h
	cmlt	v16.8h, v6.8h, #0
	mls	v6.8h, v16.8h, v2.8h
	sqrdmulh	v16.8h, v3.8h, v24.8h
	mls	v3.8h, v16.8h, v2.8h
	cmlt	v16.8h, v3.8h, #0
	mls	v3.8h, v16.8h, v2.8h
	stp	q6, q3, [x10, #-16]
	add	x9, x9, #4
	add	x11, x11, #16
	add	x10, x10, #32
	cmp	x11, #496
	b.lo	LBB2_7
// %bb.8:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/30_poly_ntt_mq.inc
// begin expanded include: ntt_asm_parts/40_poly_intt.inc
	.globl	poly_intt // -- Begin function poly_intt
	.p2align	2
poly_intt: // @poly_intt
	.cfi_startproc
// %bb.0:
	stp	d9, d8, [sp, #-16]! // 16-byte Folded Spill
	.cfi_def_cfa_offset 16
	.cfi_offset b8, -8
	.cfi_offset b9, -16
Lloh8:
	adrp	x8, fn+2
Lloh9:
	add	x8, x8, :lo12:fn+2
	add	x9, x8, #126
	add	x10, x0, #16
	mov	x11, #-16 // =0xfffffffffffffff0
	mov	w12, #64769 // =0xfd01
	dup	v0.8h, w12
	fmov	s1, w12
	mov	w12, #769 // =0x301
	dup	v2.8h, w12
LBB3_1: // =>This Inner Loop Header: Depth=1
	sub	x12, x8, #2
	ld1r	{ v3.4h }, [x12]
	ld1r	{ v4.4h }, [x8]
	mov	v5.16b, v3.16b
	mov	v5.d[1], v4.d[0]
	ldp	q6, q7, [x10, #-16]
	ext	v16.16b, v6.16b, v6.16b, #8
	sub	v17.4h, v6.4h, v16.4h
	smull	v3.4s, v17.4h, v3.4h
	ext	v18.16b, v7.16b, v7.16b, #8
	sub	v19.4h, v7.4h, v18.4h
	mov	v17.d[1], v19.d[0]
	mul	v5.8h, v5.8h, v17.8h
	ldrh	w12, [x9], #2
	dup	v17.4h, w12
	fmov	s20, w12
	mul	v8.8h, v20.8h, v1.8h
	add	v6.4h, v6.4h, v16.4h
	add	v7.4h, v7.4h, v18.4h
	smull	v4.4s, v19.4h, v4.4h
	mul	v5.8h, v5.8h, v0.8h
	smull	v16.4s, v5.4h, v2.4h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v18.4h, v3.4s, v16.4s
	sub	v19.4h, v6.4h, v7.4h
	subhn2	v6.8h, v3.4s, v16.4s
	subhn	v3.4h, v4.4s, v5.4s
	subhn2	v7.8h, v4.4s, v5.4s
	add	v4.8h, v7.8h, v6.8h
	sub	v5.8h, v6.8h, v7.8h
	smull	v6.4s, v19.4h, v17.4h
	sub	v3.4h, v18.4h, v3.4h
	smull	v3.4s, v3.4h, v17.4h
	mul	v5.8h, v5.8h, v8.h[0]
	smull	v7.4s, v5.4h, v2.4h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v6.4h, v6.4s, v7.4s
	subhn2	v6.8h, v3.4s, v5.4s
	stp	q4, q6, [x10, #-16]
	add	x8, x8, #4
	add	x11, x11, #16
	add	x10, x10, #32
	cmp	x11, #496
	b.lo	LBB3_1
// %bb.2:
	mov	w11, #96 // =0x60
	mov	w10, #16 // =0x10
Lloh10:
	adrp	x8, fn_barrett_pairs_96_125
Lloh11:
	add	x8, x8, :lo12:fn_barrett_pairs_96_125
	mov	w9, #511 // =0x1ff
	mov	w12, #64769 // =0xfd01
	fmov	s0, w12
	mov	w12, #769 // =0x301
	dup	v1.8h, w12
LBB3_3: // =>This Loop Header: Depth=1
                                        //     Child Loop BB3_4 Depth 2
                                        //       Child Loop BB3_5 Depth 3
	mov	x12, #0 // =0x0
	mov	x14, x11
                                        // kill: def $w10 killed $w10 def $x10
	mov	w13, w10
	ubfiz	x15, x10, #1, #32
	ubfiz	x16, x10, #2, #32
	lsl	w10, w10, #1
	udiv	w11, w9, w10
	add	w11, w14, w11
	add	w11, w11, #1
	mov	x17, x0
LBB3_4: // Parent Loop BB3_3 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB3_5 Depth 3
	sub	w5, w14, #96
	ldr	s2, [x8, w5, uxtw #2]
	add	x1, x12, x13
	mov	x2, x17
	mov	x3, x12
LBB3_5: // Parent Loop BB3_3 Depth=1
                                        //     Parent Loop BB3_4 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x2]
	add	x4, x2, x15
	ldp	q6, q7, [x4]
	add	v16.8h, v6.8h, v4.8h
	add	v17.8h, v7.8h, v5.8h
	stp	q16, q17, [x2], #32
	sub	v4.8h, v4.8h, v6.8h
	sub	v5.8h, v5.8h, v7.8h
	sqrdmulh	v6.8h, v4.8h, v2.h[1]
	sqrdmulh	v7.8h, v5.8h, v2.h[1]
	mul	v4.8h, v4.8h, v2.h[0]
	mul	v5.8h, v5.8h, v2.h[0]
	mls	v4.8h, v6.8h, v1.8h
	mls	v5.8h, v7.8h, v1.8h
	stp	q4, q5, [x4]
	add	x3, x3, #16
	cmp	x3, x1
	b.lo	LBB3_5
// %bb.6:                                ;   in Loop: Header=BB3_4 Depth=2
	add	w14, w14, #1
	add	x12, x12, x15
	add	x17, x17, x16
	cmp	w14, w11
	b.ne	LBB3_4
// %bb.7:                                ;   in Loop: Header=BB3_3 Depth=1
	cmp	w13, #128
	b.lo	LBB3_3
// %bb.8:
	mov	x8, #-16 // =0xfffffffffffffff0
	movi	v0.4h, #164
	movi	v1.8h, #164
	mov	w9, #5284 // =0x14a4
	dup	v2.8h, w9
	mov	w9, #769 // =0x301
	dup	v3.8h, w9
	mov	w9, #655 // =0x28f
	dup	v4.8h, w9
	mov	w9, #21903 // =0x558f
	dup	v5.8h, w9
LBB3_9: // =>This Inner Loop Header: Depth=1
	ldp	q17, q6, [x0]
	ldp	q18, q7, [x0, #512]
	sub	v16.8h, v17.8h, v18.8h
	smull	v19.4s, v16.4h, v0.4h
	smull2	v20.4s, v16.8h, v1.8h
	mul	v16.8h, v16.8h, v2.8h
	smull	v21.4s, v16.4h, v3.4h
	smull2	v22.4s, v16.8h, v3.8h
	subhn	v19.4h, v19.4s, v21.4s
	subhn	v16.4h, v20.4s, v22.4s
	smull	v21.4s, v19.4h, v4.4h
	subhn2	v19.8h, v20.4s, v22.4s
	sub	v20.8h, v6.8h, v7.8h
	smull	v22.4s, v20.4h, v0.4h
	smull2	v23.4s, v20.8h, v1.8h
	mul	v20.8h, v20.8h, v2.8h
	smull	v24.4s, v20.4h, v3.4h
	smull2	v20.4s, v20.8h, v3.8h
	subhn	v22.4h, v22.4s, v24.4s
	subhn	v24.4h, v23.4s, v20.4s
	smull	v25.4s, v22.4h, v4.4h
	subhn2	v22.8h, v23.4s, v20.4s
	add	v17.8h, v18.8h, v17.8h
	smull	v18.4s, v17.4h, v4.4h
	smull2	v20.4s, v17.8h, v4.8h
	mul	v17.8h, v17.8h, v5.8h
	smull	v23.4s, v17.4h, v3.4h
	smull2	v17.4s, v17.8h, v3.8h
	subhn	v18.4h, v18.4s, v23.4s
	subhn2	v18.8h, v20.4s, v17.4s
	add	v6.8h, v7.8h, v6.8h
	smull	v7.4s, v6.4h, v4.4h
	smull2	v17.4s, v6.8h, v4.8h
	mul	v6.8h, v6.8h, v5.8h
	smull	v20.4s, v6.4h, v3.4h
	smull2	v6.4s, v6.8h, v3.8h
	subhn	v7.4h, v7.4s, v20.4s
	subhn2	v7.8h, v17.4s, v6.4s
	stp	q18, q7, [x0]
	smull	v6.4s, v16.4h, v4.4h
	mul	v7.8h, v19.8h, v5.8h
	smull	v16.4s, v7.4h, v3.4h
	smull2	v7.4s, v7.8h, v3.8h
	subhn	v16.4h, v21.4s, v16.4s
	subhn2	v16.8h, v6.4s, v7.4s
	smull	v6.4s, v24.4h, v4.4h
	mul	v7.8h, v22.8h, v5.8h
	smull	v17.4s, v7.4h, v3.4h
	smull2	v7.4s, v7.8h, v3.8h
	subhn	v17.4h, v25.4s, v17.4s
	subhn2	v17.8h, v6.4s, v7.4s
	stp	q16, q17, [x0, #512]
	add	x8, x8, #16
	add	x0, x0, #32
	cmp	x8, #240
	b.lo	LBB3_9
// %bb.10:
	ldp	d9, d8, [sp], #16 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/40_poly_intt.inc
// begin expanded include: ntt_asm_parts/50_poly_basemul_ntt.inc
// opt-5 lazy kernel for poly_basemul_ntt.
// This is the higher-reward candidate: pre-multiply b1/b2/b3 by the ordinary
// centered twiddle, form four wide dot products, then Montgomery-reduce only
// the final four outputs.  It is intended to be equivalent modulo 769; the
// benchmark self-test compares residues, while KAT remains the byte-level gate.
	.globl	poly_basemul_ntt // -- Begin function poly_basemul_ntt
	.p2align	2
poly_basemul_ntt: // @poly_basemul_ntt
	.cfi_startproc
// %bb.0:
	mov	x8, #0 // =0x0
	mvni	v0.8h, #3, lsl #8 // -769
	mov	w9, #64769 // =0xfd01
	dup	v1.8h, w9
	mov	w9, #769 // =0x301
	dup	v2.8h, w9
Lloh50_lazy_ztab_page:
	adrp	x3, poly_basemul_ntt_lazy_ztab
Lloh50_lazy_ztab_off:
	add	x3, x3, :lo12:poly_basemul_ntt_lazy_ztab
LBB50_lazy_1: // =>This Inner Loop Header: Depth=1
	add	x9, x2, x8
	ld4	{ v3.8h, v4.8h, v5.8h, v6.8h }, [x9]
	add	x9, x1, x8
	ld4	{ v16.8h, v17.8h, v18.8h, v19.8h }, [x9]
	add	x9, x0, x8
	ldp	q7, q21, [x3], #32
	sqrdmulh	v22.8h, v4.8h, v21.8h
	mul	v20.8h, v7.8h, v4.8h
	mla	v20.8h, v22.8h, v0.8h
	sqrdmulh	v22.8h, v5.8h, v21.8h
	mul	v24.8h, v7.8h, v5.8h
	sqrdmulh	v21.8h, v6.8h, v21.8h
	mla	v24.8h, v22.8h, v0.8h
	mul	v25.8h, v7.8h, v6.8h
	mla	v25.8h, v21.8h, v0.8h
	smull	v7.4s, v17.4h, v25.4h
	smlal	v7.4s, v16.4h, v3.4h
	smull2	v22.4s, v17.8h, v25.8h
	smlal2	v22.4s, v16.8h, v3.8h
	smull	v21.4s, v17.4h, v3.4h
	smlal	v21.4s, v16.4h, v4.4h
	smlal	v21.4s, v18.4h, v25.4h
	smlal	v21.4s, v19.4h, v24.4h
	smull2	v23.4s, v17.8h, v3.8h
	smlal	v7.4s, v18.4h, v24.4h
	smlal2	v23.4s, v16.8h, v4.8h
	smlal2	v23.4s, v18.8h, v25.8h
	smlal2	v23.4s, v19.8h, v24.8h
	smull	v26.4s, v17.4h, v4.4h
	smlal	v26.4s, v16.4h, v5.4h
	smlal2	v22.4s, v18.8h, v24.8h
	smlal	v26.4s, v18.4h, v3.4h
	smlal	v26.4s, v19.4h, v25.4h
	smull2	v24.4s, v17.8h, v4.8h
	smlal2	v24.4s, v16.8h, v5.8h
	smlal2	v24.4s, v18.8h, v3.8h
	smlal	v7.4s, v19.4h, v20.4h
	smlal2	v24.4s, v19.8h, v25.8h
	smull	v25.4s, v17.4h, v5.4h
	smlal	v25.4s, v16.4h, v6.4h
	smlal	v25.4s, v18.4h, v4.4h
	smlal	v25.4s, v19.4h, v3.4h
	smlal2	v22.4s, v19.8h, v20.8h
	smull2	v20.4s, v17.8h, v5.8h
	smlal2	v20.4s, v16.8h, v6.8h
	smlal2	v20.4s, v18.8h, v4.8h
	smlal2	v20.4s, v19.8h, v3.8h
	uzp1	v3.8h, v7.8h, v22.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v7.4s, v3.4h, v2.4h
	smlsl2	v22.4s, v3.8h, v2.8h
	uzp1	v3.8h, v21.8h, v23.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v21.4s, v3.4h, v2.4h
	uzp2	v4.8h, v7.8h, v22.8h
	smlsl2	v23.4s, v3.8h, v2.8h
	uzp1	v3.8h, v26.8h, v24.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v26.4s, v3.4h, v2.4h
	smlsl2	v24.4s, v3.8h, v2.8h
	uzp2	v5.8h, v21.8h, v23.8h
	uzp2	v6.8h, v26.8h, v24.8h
	uzp1	v3.8h, v25.8h, v20.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v25.4s, v3.4h, v2.4h
	smlsl2	v20.4s, v3.8h, v2.8h
	uzp2	v7.8h, v25.8h, v20.8h
	st4	{ v4.8h, v5.8h, v6.8h, v7.8h }, [x9]
	add	x8, x8, #64
	cmp	w8, #1024
	b.ne	LBB50_lazy_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/50_poly_basemul_ntt.inc
// begin expanded include: ntt_asm_parts/60_poly_basemul_ntt_mq.inc
// lazy poly_basemul_ntt_mq.
// Reuses the lazy dot-product structure from poly_basemul_ntt, then applies
// the mq post-step as a 16-bit Barrett multiply by 19*R^-1 = 171 followed by
// one negative correction into [0, q).  KAT remains the byte-level gate.
	.globl	poly_basemul_ntt_mq // -- Begin function poly_basemul_ntt_mq
	.p2align	2
poly_basemul_ntt_mq: // @poly_basemul_ntt_mq
	.cfi_startproc
// %bb.0:
	mov	x8, #0 // =0x0
	mvni	v0.8h, #3, lsl #8 // -769
	mov	w9, #64769 // =0xfd01
	dup	v1.8h, w9
	mov	w9, #769 // =0x301
	dup	v2.8h, w9
	mov	w9, #171 // =0xab
	dup	v30.8h, w9
	mov	w9, #7287 // =0x1c77
	dup	v31.8h, w9
Lloh60_lazy_ztab_page:
	adrp	x3, poly_basemul_ntt_lazy_ztab
Lloh60_lazy_ztab_off:
	add	x3, x3, :lo12:poly_basemul_ntt_lazy_ztab
LBB60_lazy_1: // =>This Inner Loop Header: Depth=1
	add	x9, x2, x8
	ld4	{ v3.8h, v4.8h, v5.8h, v6.8h }, [x9]
	add	x9, x1, x8
	ld4	{ v16.8h, v17.8h, v18.8h, v19.8h }, [x9]
	add	x9, x0, x8
	ldp	q7, q21, [x3], #32
	sqrdmulh	v22.8h, v4.8h, v21.8h
	mul	v20.8h, v7.8h, v4.8h
	mla	v20.8h, v22.8h, v0.8h
	sqrdmulh	v22.8h, v5.8h, v21.8h
	mul	v24.8h, v7.8h, v5.8h
	sqrdmulh	v21.8h, v6.8h, v21.8h
	mla	v24.8h, v22.8h, v0.8h
	mul	v25.8h, v7.8h, v6.8h
	mla	v25.8h, v21.8h, v0.8h
	smull	v7.4s, v17.4h, v25.4h
	smlal	v7.4s, v16.4h, v3.4h
	smull2	v22.4s, v17.8h, v25.8h
	smlal2	v22.4s, v16.8h, v3.8h
	smull	v21.4s, v17.4h, v3.4h
	smlal	v21.4s, v16.4h, v4.4h
	smlal	v21.4s, v18.4h, v25.4h
	smlal	v21.4s, v19.4h, v24.4h
	smull2	v23.4s, v17.8h, v3.8h
	smlal	v7.4s, v18.4h, v24.4h
	smlal2	v23.4s, v16.8h, v4.8h
	smlal2	v23.4s, v18.8h, v25.8h
	smlal2	v23.4s, v19.8h, v24.8h
	smull	v26.4s, v17.4h, v4.4h
	smlal	v26.4s, v16.4h, v5.4h
	smlal2	v22.4s, v18.8h, v24.8h
	smlal	v26.4s, v18.4h, v3.4h
	smlal	v26.4s, v19.4h, v25.4h
	smull2	v24.4s, v17.8h, v4.8h
	smlal2	v24.4s, v16.8h, v5.8h
	smlal2	v24.4s, v18.8h, v3.8h
	smlal	v7.4s, v19.4h, v20.4h
	smlal2	v24.4s, v19.8h, v25.8h
	smull	v25.4s, v17.4h, v5.4h
	smlal	v25.4s, v16.4h, v6.4h
	smlal	v25.4s, v18.4h, v4.4h
	smlal	v25.4s, v19.4h, v3.4h
	smlal2	v22.4s, v19.8h, v20.8h
	smull2	v20.4s, v17.8h, v5.8h
	smlal2	v20.4s, v16.8h, v6.8h
	smlal2	v20.4s, v18.8h, v4.8h
	smlal2	v20.4s, v19.8h, v3.8h
	uzp1	v3.8h, v7.8h, v22.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v7.4s, v3.4h, v2.4h
	smlsl2	v22.4s, v3.8h, v2.8h
	uzp1	v3.8h, v21.8h, v23.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v21.4s, v3.4h, v2.4h
	uzp2	v4.8h, v7.8h, v22.8h
	smlsl2	v23.4s, v3.8h, v2.8h
	uzp1	v3.8h, v26.8h, v24.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v26.4s, v3.4h, v2.4h
	smlsl2	v24.4s, v3.8h, v2.8h
	uzp2	v5.8h, v21.8h, v23.8h
	uzp2	v6.8h, v26.8h, v24.8h
	uzp1	v3.8h, v25.8h, v20.8h
	mul	v3.8h, v3.8h, v1.8h
	smlsl	v25.4s, v3.4h, v2.4h
	smlsl2	v20.4s, v3.8h, v2.8h
	uzp2	v7.8h, v25.8h, v20.8h

	sqrdmulh	v16.8h, v4.8h, v31.8h
	mul	v4.8h, v4.8h, v30.8h
	mls	v4.8h, v16.8h, v2.8h
	cmlt	v16.8h, v4.8h, #0
	mls	v4.8h, v16.8h, v2.8h

	sqrdmulh	v16.8h, v5.8h, v31.8h
	mul	v5.8h, v5.8h, v30.8h
	mls	v5.8h, v16.8h, v2.8h
	cmlt	v16.8h, v5.8h, #0
	mls	v5.8h, v16.8h, v2.8h

	sqrdmulh	v16.8h, v6.8h, v31.8h
	mul	v6.8h, v6.8h, v30.8h
	mls	v6.8h, v16.8h, v2.8h
	cmlt	v16.8h, v6.8h, #0
	mls	v6.8h, v16.8h, v2.8h

	sqrdmulh	v16.8h, v7.8h, v31.8h
	mul	v7.8h, v7.8h, v30.8h
	mls	v7.8h, v16.8h, v2.8h
	cmlt	v16.8h, v7.8h, #0
	mls	v7.8h, v16.8h, v2.8h

	st4	{ v4.8h, v5.8h, v6.8h, v7.8h }, [x9]
	add	x8, x8, #64
	cmp	w8, #1024
	b.ne	LBB60_lazy_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/60_poly_basemul_ntt_mq.inc
// begin expanded include: ntt_asm_parts/70_poly_baseinv_ntt.inc
	.globl	poly_baseinv_ntt // -- Begin function poly_baseinv_ntt
	.p2align	2
poly_baseinv_ntt: // @poly_baseinv_ntt
	.cfi_startproc
// %bb.0:
	stp	d15, d14, [sp, #-64]! // 16-byte Folded Spill
	stp	d13, d12, [sp, #16] // 16-byte Folded Spill
	stp	d11, d10, [sp, #32] // 16-byte Folded Spill
	stp	d9, d8, [sp, #48] // 16-byte Folded Spill
	.cfi_def_cfa_offset 64
	.cfi_offset b8, -8
	.cfi_offset b9, -16
	.cfi_offset b10, -24
	.cfi_offset b11, -32
	.cfi_offset b12, -40
	.cfi_offset b13, -48
	.cfi_offset b14, -56
	.cfi_offset b15, -64
Lloh70_fast_zeta_page:
	adrp	x8, baseinv_zetas_pm
Lloh70_fast_zeta_addr:
	add	x8, x8, :lo12:baseinv_zetas_pm
	mov	x9, #-4 // =0xfffffffffffffffc
	movi	v0.4h, #19
	movi	v1.8h, #19
	mov	w10, #64769 // =0xfd01
	dup	v2.8h, w10
	mov	w10, #769 // =0x301
	dup	v3.8h, w10
Lloh70_fast_qinv_page:
	adrp	x10, qinv_r2
Lloh70_fast_qinv_addr:
	add	x10, x10, :lo12:qinv_r2
	mov	w11, #384 // =0x180
	dup	v4.8h, w11
	mov	w11, #65152 // =0xfe80
	dup	v5.8h, w11
	movi	v6.2d, #0xffffffffffffffff
	mov	w11, #65151 // =0xfe7f
	dup	v7.8h, w11
LBB70_fast_loop: // =>This Inner Loop Header: Depth=1
	ld4	{ v16.8h, v17.8h, v18.8h, v19.8h }, [x1], #64
	ldr	q20, [x8], #16
	smull	v21.4s, v16.4h, v0.4h
	smull2	v22.4s, v16.8h, v1.8h
	uzp1	v23.8h, v21.8h, v22.8h
	mul	v23.8h, v23.8h, v2.8h
	smull	v24.4s, v23.4h, v3.4h
	smull2	v23.4s, v23.8h, v3.8h
	smull	v25.4s, v17.4h, v0.4h
	smull2	v27.4s, v17.8h, v1.8h
	uzp1	v26.8h, v25.8h, v27.8h
	mul	v26.8h, v26.8h, v2.8h
	smlsl	v25.4s, v26.4h, v3.4h
	subhn	v29.4h, v21.4s, v24.4s
	smlsl2	v27.4s, v26.8h, v3.8h
	smull	v24.4s, v18.4h, v0.4h
	smull2	v26.4s, v18.8h, v1.8h
	uzp1	v21.8h, v24.8h, v26.8h
	mul	v28.8h, v21.8h, v2.8h
	subhn	v21.4h, v22.4s, v23.4s
	smlsl	v24.4s, v28.4h, v3.4h
	smlsl2	v26.4s, v28.8h, v3.8h
	ushr	v24.4s, v24.4s, #16
	ushr	v30.4s, v26.4s, #16
	smull	v31.4s, v19.4h, v0.4h
	ushr	v26.4s, v25.4s, #16
	smull2	v8.4s, v19.8h, v1.8h
	uzp1	v25.8h, v31.8h, v8.8h
	mul	v25.8h, v25.8h, v2.8h
	smull	v9.4s, v25.4h, v3.4h
	smlsl2	v8.4s, v25.8h, v3.8h
	ushr	v28.4s, v27.4s, #16
	subhn	v25.4h, v31.4s, v9.4s
	uzp1	v24.8h, v24.8h, v30.8h
	smull	v31.4s, v24.4h, v20.4h
	smull2	v9.4s, v24.8h, v20.8h
	uzp1	v27.8h, v31.8h, v9.8h
	ushr	v8.4s, v8.4s, #16
	mul	v10.8h, v27.8h, v2.8h
	smull	v27.4s, v10.4h, v3.4h
	subhn	v27.4h, v31.4s, v27.4s
	smull	v30.4s, v27.4h, v24.4h
	smlal	v30.4s, v29.4h, v29.4h
	mov	v11.16b, v29.16b
	smlsl	v31.4s, v10.4h, v3.4h
	smlsl2	v9.4s, v10.8h, v3.8h
	ushr	v9.4s, v9.4s, #16
	smull	v10.4s, v25.4h, v20.4h
	uzp1	v29.8h, v0.8h, v8.8h
	subhn2	v11.8h, v22.4s, v23.4s
	smull2	v22.4s, v29.8h, v20.8h
	uzp1	v23.8h, v10.8h, v22.8h
	mul	v23.8h, v23.8h, v2.8h
	smull	v8.4s, v23.4h, v3.4h
	subhn	v8.4h, v10.4s, v8.4s
	ushr	v31.4s, v31.4s, #16
	smlsl	v10.4s, v23.4h, v3.4h
	smlsl2	v22.4s, v23.8h, v3.8h
	ushr	v23.4s, v10.4s, #16
	ushr	v10.4s, v22.4s, #16
	uzp1	v12.8h, v0.8h, v10.8h
	ext	v22.16b, v12.16b, v12.16b, #8
	shl	v31.4s, v31.4s, #16
	shl	v13.4s, v9.4s, #16
	sshr	v13.4s, v13.4s, #16
	uzp1	v9.8h, v0.8h, v9.8h
	ext	v14.16b, v9.16b, v9.16b, #8
	sshr	v31.4s, v31.4s, #16
	add	v11.8h, v11.8h, v11.8h
	uzp1	v26.8h, v26.8h, v28.8h
	smull	v28.4s, v11.4h, v24.4h
	smlsl	v28.4s, v26.4h, v26.4h
	smlsl	v28.4s, v8.4h, v25.4h
	mov	v27.d[1], v14.d[0]
	mul	v25.8h, v27.8h, v2.8h
	mov	v8.d[1], v22.d[0]
	smull	v22.4s, v25.4h, v3.4h
	smlsl2	v13.4s, v25.8h, v3.8h
	subhn	v22.4h, v31.4s, v22.4s
	shl	v23.4s, v23.4s, #16
	sshr	v25.4s, v23.4s, #16
	ushr	v23.4s, v13.4s, #16
	shl	v27.4s, v10.4s, #16
	sshr	v27.4s, v27.4s, #16
	mul	v31.8h, v8.8h, v2.8h
	smull	v10.4s, v31.4h, v3.4h
	add	v8.8h, v8.8h, v8.8h
	smlsl2	v27.4s, v31.8h, v3.8h
	smull	v31.4s, v21.4h, v21.4h
	smlal2	v31.4s, v9.8h, v24.8h
	smlsl	v30.4s, v8.4h, v26.4h
	smlsl2	v31.4s, v8.8h, v26.8h
	uzp1	v8.8h, v30.8h, v31.8h
	subhn	v21.4h, v25.4s, v10.4s
	mul	v25.8h, v8.8h, v2.8h
	smull	v8.4s, v25.4h, v3.4h
	smull2	v9.4s, v11.8h, v24.8h
	smlsl2	v9.4s, v26.8h, v26.8h
	smlsl2	v9.4s, v12.8h, v29.8h
	smull2	v26.4s, v25.8h, v3.8h
	uzp1	v24.8h, v28.8h, v9.8h
	mul	v24.8h, v24.8h, v2.8h
	smull	v25.4s, v24.4h, v3.4h
	smlsl2	v9.4s, v24.8h, v3.8h
	subhn	v24.4h, v28.4s, v25.4s
	subhn	v25.4h, v30.4s, v8.4s
	ushr	v28.4s, v9.4s, #16
	smull	v29.4s, v24.4h, v20.4h
	uzp1	v28.8h, v0.8h, v28.8h
	smull2	v20.4s, v28.8h, v20.8h
	uzp1	v30.8h, v29.8h, v20.8h
	subhn	v26.4h, v31.4s, v26.4s
	mul	v30.8h, v30.8h, v2.8h
	smull	v31.4s, v30.4h, v3.4h
	smlsl2	v20.4s, v30.8h, v3.8h
	subhn	v29.4h, v29.4s, v31.4s
	ushr	v20.4s, v20.4s, #16
	smull	v30.4s, v25.4h, v25.4h
	smull	v31.4s, v26.4h, v26.4h
	smlsl	v30.4s, v29.4h, v24.4h
	uzp1	v20.8h, v0.8h, v20.8h
	smlsl2	v31.4s, v20.8h, v28.8h
	uzp1	v29.8h, v30.8h, v31.8h
	ushr	v20.4s, v27.4s, #16
	mul	v27.8h, v29.8h, v2.8h
	smull	v29.4s, v27.4h, v3.4h
	smull2	v8.4s, v27.8h, v3.8h
	subhn	v29.4h, v30.4s, v29.4s
	smlsl	v30.4s, v27.4h, v3.4h
	subhn2	v29.8h, v31.4s, v8.4s
	smlsl2	v31.4s, v27.8h, v3.8h
	cmlt	v27.4s, v31.4s, #0
	cmlt	v30.4s, v30.4s, #0
	uzp1	v27.8h, v30.8h, v27.8h
	and	v30.16b, v27.16b, v3.16b
	ext	v27.16b, v28.16b, v28.16b, #8
	add	v28.8h, v30.8h, v29.8h
	umov	w11, v28.h[0]
	umov	w12, v28.h[1]
	add	x12, x10, w12, uxth #1
	umov	w13, v28.h[2]
	add	x13, x10, w13, uxth #1
	umov	w14, v28.h[3]
	add	x14, x10, w14, uxth #1
	umov	w15, v28.h[4]
	umov	w16, v28.h[5]
	ldr	h29, [x10, x11, lsl #1]
	add	x11, x10, w16, uxth #1
	umov	w16, v28.h[6]
	add	x16, x10, w16, uxth #1
	ld1	{v29.h}[1], [x12]
	ldr	h30, [x10, x15, lsl #1]
	umov	w12, v28.h[7]
	ld1	{v29.h}[2], [x13]
	ld1	{v30.h}[1], [x11]
	ld1	{v30.h}[2], [x16]
	ld1	{v29.h}[3], [x14]
	add	x11, x10, w12, uxth #1
	ld1	{v30.h}[3], [x11]
	smull	v25.4s, v25.4h, v29.4h
	smull	v26.4s, v26.4h, v30.4h
	uzp1	v28.8h, v25.8h, v26.8h
	mul	v28.8h, v28.8h, v2.8h
	smull	v31.4s, v28.4h, v3.4h
	smlsl2	v26.4s, v28.8h, v3.8h
	smull	v28.4s, v24.4h, v29.4h
	smull	v27.4s, v27.4h, v30.4h
	subhn	v24.4h, v25.4s, v31.4s
	uzp1	v25.8h, v28.8h, v27.8h
	mul	v25.8h, v25.8h, v2.8h
	smull	v29.4s, v25.4h, v3.4h
	smull2	v25.4s, v25.8h, v3.8h
	subhn	v28.4h, v28.4s, v29.4s
	ushr	v26.4s, v26.4s, #16
	subhn2	v28.8h, v27.4s, v25.4s
	neg	v25.8h, v28.8h
	smull	v27.4s, v22.4h, v25.4h
	uzp1	v22.8h, v0.8h, v23.8h
	smull2	v23.4s, v22.8h, v25.8h
	smlal	v27.4s, v16.4h, v24.4h
	uzp1	v22.8h, v0.8h, v26.8h
	smlal2	v23.4s, v16.8h, v22.8h
	uzp1	v26.8h, v27.8h, v23.8h
	mul	v26.8h, v26.8h, v2.8h
	smull	v28.4s, v26.4h, v3.4h
	smull2	v26.4s, v26.8h, v3.8h
	subhn	v27.4h, v27.4s, v28.4s
	subhn2	v27.8h, v23.4s, v26.4s
	cmgt	v23.8h, v27.8h, v4.8h
	smull	v21.4s, v21.4h, v25.4h
	smlal	v21.4s, v17.4h, v24.4h
	bic	v23.8h, #3, lsl #8
	uzp1	v20.8h, v0.8h, v20.8h
	smull2	v20.4s, v20.8h, v25.8h
	smlal2	v20.4s, v17.8h, v22.8h
	uzp1	v26.8h, v21.8h, v20.8h
	mul	v26.8h, v26.8h, v2.8h
	add	v23.8h, v23.8h, v27.8h
	smull	v27.4s, v26.4h, v3.4h
	smull2	v26.4s, v26.8h, v3.8h
	subhn	v21.4h, v21.4s, v27.4s
	subhn2	v21.8h, v20.4s, v26.4s
	add	v20.8h, v21.8h, v6.8h
	cmgt	v26.8h, v5.8h, v23.8h
	cmgt	v20.8h, v7.8h, v20.8h
	bic	v20.8h, #3, lsl #8
	sub	v20.8h, v20.8h, v21.8h
	smull	v21.4s, v18.4h, v24.4h
	smlal	v21.4s, v16.4h, v25.4h
	cmgt	v27.8h, v5.8h, v20.8h
	smull2	v28.4s, v18.8h, v22.8h
	smlal2	v28.4s, v16.8h, v25.8h
	uzp1	v29.8h, v21.8h, v28.8h
	mul	v29.8h, v29.8h, v2.8h
	smull	v30.4s, v29.4h, v3.4h
	and	v26.16b, v26.16b, v3.16b
	smull2	v29.4s, v29.8h, v3.8h
	subhn	v21.4h, v21.4s, v30.4s
	subhn2	v21.8h, v28.4s, v29.4s
	cmgt	v28.8h, v21.8h, v4.8h
	bic	v28.8h, #3, lsl #8
	add	v8.8h, v26.8h, v23.8h
	add	v21.8h, v28.8h, v21.8h
	cmgt	v23.8h, v5.8h, v21.8h
	and	v23.16b, v23.16b, v3.16b
	smull	v24.4s, v19.4h, v24.4h
	smlal	v24.4s, v17.4h, v25.4h
	and	v26.16b, v27.16b, v3.16b
	smull2	v22.4s, v19.8h, v22.8h
	smlal2	v22.4s, v17.8h, v25.8h
	uzp1	v16.8h, v24.8h, v22.8h
	mul	v16.8h, v16.8h, v2.8h
	smull	v17.4s, v16.4h, v3.4h
	add	v9.8h, v26.8h, v20.8h
	smull2	v16.4s, v16.8h, v3.8h
	subhn	v17.4h, v24.4s, v17.4s
	subhn2	v17.8h, v22.4s, v16.4s
	add	v16.8h, v17.8h, v6.8h
	cmgt	v16.8h, v7.8h, v16.8h
	add	v10.8h, v23.8h, v21.8h
	bic	v16.8h, #3, lsl #8
	sub	v16.8h, v16.8h, v17.8h
	cmgt	v17.8h, v5.8h, v16.8h
	and	v17.16b, v17.16b, v3.16b
	add	v11.8h, v17.8h, v16.8h
	st4	{ v8.8h, v9.8h, v10.8h, v11.8h }, [x0], #64
	add	x9, x9, #4
	cmp	x9, #60
	b.lo	LBB70_fast_loop
// %bb.2:
	ldp	d9, d8, [sp, #48] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #32] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #16] // 16-byte Folded Reload
	ldp	d15, d14, [sp], #64 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/70_poly_baseinv_ntt.inc
// begin expanded include: ntt_asm_parts/90_constants.inc
	.section	.rodata,"a",@progbits
	.p2align	1, 0x0 // @f
f:
	.short	171 // 0xab
	.short	605 // 0x25d
	.short	688 // 0x2b0
	.short	361 // 0x169
	.short	186 // 0xba
	.short	766 // 0x2fe
	.short	519 // 0x207
	.short	649 // 0x289
	.short	461 // 0x1cd
	.short	129 // 0x81
	.short	753 // 0x2f1
	.short	546 // 0x222
	.short	407 // 0x197
	.short	626 // 0x272
	.short	131 // 0x83
	.short	432 // 0x1b0
	.short	693 // 0x2b5
	.short	671 // 0x29f
	.short	36 // 0x24
	.short	694 // 0x2b6
	.short	430 // 0x1ae
	.short	514 // 0x202
	.short	282 // 0x11a
	.short	566 // 0x236
	.short	735 // 0x2df
	.short	199 // 0xc7
	.short	178 // 0xb2
	.short	270 // 0x10e
	.short	759 // 0x2f7
	.short	149 // 0x95
	.short	369 // 0x171
	.short	577 // 0x241
	.short	147 // 0x93
	.short	655 // 0x28f
	.short	497 // 0x1f1
	.short	54 // 0x36
	.short	767 // 0x2ff
	.short	645 // 0x285
	.short	689 // 0x2b1
	.short	423 // 0x1a7
	.short	86 // 0x56
	.short	718 // 0x2ce
	.short	364 // 0x16c
	.short	267 // 0x10b
	.short	161 // 0xa1
	.short	754 // 0x2f2
	.short	288 // 0x120
	.short	169 // 0xa9
	.short	191 // 0xbf
	.short	307 // 0x133
	.short	719 // 0x2cf
	.short	745 // 0x2e9
	.short	599 // 0x257
	.short	226 // 0xe2
	.short	121 // 0x79
	.short	581 // 0x245
	.short	389 // 0x185
	.short	279 // 0x117
	.short	180 // 0xb4
	.short	394 // 0x18a
	.short	612 // 0x264
	.short	263 // 0x107
	.short	641 // 0x281
	.short	523 // 0x20b
	.short	746 // 0x2ea
	.short	112 // 0x70
	.short	618 // 0x26a
	.short	635 // 0x27b
	.short	717 // 0x2cd
	.short	621 // 0x26d
	.short	227 // 0xe3
	.short	232 // 0xe8
	.short	698 // 0x2ba
	.short	212 // 0xd4
	.short	236 // 0xec
	.short	21 // 0x15
	.short	341 // 0x155
	.short	379 // 0x17b
	.short	567 // 0x237
	.short	549 // 0x225
	.short	352 // 0x160
	.short	292 // 0x124
	.short	238 // 0xee
	.short	145 // 0x91
	.short	194 // 0xc2
	.short	493 // 0x1ed
	.short	70 // 0x46
	.short	495 // 0x1ef
	.short	117 // 0x75
	.short	333 // 0x14d
	.short	66 // 0x42
	.short	247 // 0xf7
	.short	532 // 0x214
	.short	686 // 0x2ae
	.short	517 // 0x205
	.short	525 // 0x20d
	.short	331 // 0x14b
	.short	528 // 0x210
	.short	167 // 0xa7
	.short	357 // 0x165
	.short	414 // 0x19e
	.short	291 // 0x123
	.short	411 // 0x19b
	.short	105 // 0x69
	.short	654 // 0x28e
	.short	560 // 0x230
	.short	14 // 0xe
	.short	99 // 0x63
	.short	509 // 0x1fd
	.short	29 // 0x1d
	.short	366 // 0x16e
	.short	391 // 0x187
	.short	451 // 0x1c3
	.short	278 // 0x116
	.short	353 // 0x161
	.short	354 // 0x162
	.short	585 // 0x249
	.short	127 // 0x7f
	.short	330 // 0x14a
	.short	466 // 0x1d2
	.short	222 // 0xde
	.short	691 // 0x2b3
	.short	421 // 0x1a5
	.short	725 // 0x2d5
	.short	201 // 0xc9
	.short	158 // 0x9e
	.short	350 // 0x15e
	.short	168 // 0xa8

	.p2align	2, 0x0 // @f_barrett_pairs_0_31
f_barrett_pairs_0_31:
	.short	1, 43
	.short	62, 2642
	.short	40, 1704
	.short	173, 7372
	.short	136, 5795
	.short	-27, -1151
	.short	57, 2429
	.short	-311, -13252
	.short	304, 12954
	.short	-377, -16064
	.short	-144, -6136
	.short	300, 12783
	.short	-182, -7755
	.short	251, 10695
	.short	-359, -15297
	.short	43, 1832
	.short	85, 3622
	.short	-113, -4815
	.short	324, 13806
	.short	94, 4005
	.short	25, 1065
	.short	12, 511
	.short	231, 9843
	.short	-289, -12315
	.short	-306, -13039
	.short	253, 10781
	.short	64, 2727
	.short	123, 5241
	.short	-90, -3835
	.short	-197, -8394
	.short	245, 10440
	.short	-190, -8096

	.p2align	4, 0x0 // @baseinv_zetas_pm
baseinv_zetas_pm:
	.short	746, -746, 112, -112, 618, -618, 635, -635
	.short	717, -717, 621, -621, 227, -227, 232, -232
	.short	698, -698, 212, -212, 236, -236, 21, -21
	.short	341, -341, 379, -379, 567, -567, 549, -549
	.short	352, -352, 292, -292, 238, -238, 145, -145
	.short	194, -194, 493, -493, 70, -70, 495, -495
	.short	117, -117, 333, -333, 66, -66, 247, -247
	.short	532, -532, 686, -686, 517, -517, 525, -525
	.short	331, -331, 528, -528, 167, -167, 357, -357
	.short	414, -414, 291, -291, 411, -411, 105, -105
	.short	654, -654, 560, -560, 14, -14, 99, -99
	.short	509, -509, 29, -29, 366, -366, 391, -391
	.short	451, -451, 278, -278, 353, -353, 354, -354
	.short	585, -585, 127, -127, 330, -330, 466, -466
	.short	222, -222, 691, -691, 421, -421, 725, -725
	.short	201, -201, 158, -158, 350, -350, 168, -168

	.p2align	1, 0x0 // @fn
fn:
	.short	601 // 0x259
	.short	419 // 0x1a3
	.short	611 // 0x263
	.short	568 // 0x238
	.short	44 // 0x2c
	.short	348 // 0x15c
	.short	78 // 0x4e
	.short	547 // 0x223
	.short	303 // 0x12f
	.short	439 // 0x1b7
	.short	642 // 0x282
	.short	184 // 0xb8
	.short	415 // 0x19f
	.short	416 // 0x1a0
	.short	491 // 0x1eb
	.short	318 // 0x13e
	.short	378 // 0x17a
	.short	403 // 0x193
	.short	740 // 0x2e4
	.short	260 // 0x104
	.short	670 // 0x29e
	.short	755 // 0x2f3
	.short	209 // 0xd1
	.short	115 // 0x73
	.short	664 // 0x298
	.short	358 // 0x166
	.short	478 // 0x1de
	.short	355 // 0x163
	.short	412 // 0x19c
	.short	602 // 0x25a
	.short	241 // 0xf1
	.short	438 // 0x1b6
	.short	244 // 0xf4
	.short	252 // 0xfc
	.short	83 // 0x53
	.short	237 // 0xed
	.short	522 // 0x20a
	.short	703 // 0x2bf
	.short	436 // 0x1b4
	.short	652 // 0x28c
	.short	274 // 0x112
	.short	699 // 0x2bb
	.short	276 // 0x114
	.short	575 // 0x23f
	.short	624 // 0x270
	.short	531 // 0x213
	.short	477 // 0x1dd
	.short	417 // 0x1a1
	.short	220 // 0xdc
	.short	202 // 0xca
	.short	390 // 0x186
	.short	428 // 0x1ac
	.short	748 // 0x2ec
	.short	533 // 0x215
	.short	557 // 0x22d
	.short	71 // 0x47
	.short	537 // 0x219
	.short	542 // 0x21e
	.short	148 // 0x94
	.short	52 // 0x34
	.short	134 // 0x86
	.short	151 // 0x97
	.short	657 // 0x291
	.short	23 // 0x17
	.short	246 // 0xf6
	.short	128 // 0x80
	.short	506 // 0x1fa
	.short	157 // 0x9d
	.short	375 // 0x177
	.short	589 // 0x24d
	.short	490 // 0x1ea
	.short	380 // 0x17c
	.short	188 // 0xbc
	.short	648 // 0x288
	.short	543 // 0x21f
	.short	170 // 0xaa
	.short	24 // 0x18
	.short	50 // 0x32
	.short	462 // 0x1ce
	.short	578 // 0x242
	.short	600 // 0x258
	.short	481 // 0x1e1
	.short	15 // 0xf
	.short	608 // 0x260
	.short	502 // 0x1f6
	.short	405 // 0x195
	.short	51 // 0x33
	.short	683 // 0x2ab
	.short	346 // 0x15a
	.short	80 // 0x50
	.short	124 // 0x7c
	.short	2 // 0x2
	.short	715 // 0x2cb
	.short	272 // 0x110
	.short	114 // 0x72
	.short	622 // 0x26e
	.short	192 // 0xc0
	.short	400 // 0x190
	.short	620 // 0x26c
	.short	10 // 0xa
	.short	499 // 0x1f3
	.short	591 // 0x24f
	.short	570 // 0x23a
	.short	34 // 0x22
	.short	203 // 0xcb
	.short	487 // 0x1e7
	.short	255 // 0xff
	.short	339 // 0x153
	.short	75 // 0x4b
	.short	733 // 0x2dd
	.short	98 // 0x62
	.short	76 // 0x4c
	.short	337 // 0x151
	.short	638 // 0x27e
	.short	143 // 0x8f
	.short	362 // 0x16a
	.short	223 // 0xdf
	.short	16 // 0x10
	.short	640 // 0x280
	.short	308 // 0x134
	.short	120 // 0x78
	.short	250 // 0xfa
	.short	3 // 0x3
	.short	583 // 0x247
	.short	408 // 0x198
	.short	81 // 0x51
	.short	164 // 0xa4
	.short	655 // 0x28f

	.p2align	2, 0x0 // @fn_barrett_pairs_96_125
fn_barrett_pairs_96_125:
	.short	190, 8096
	.short	-245, -10440
	.short	197, 8394
	.short	90, 3835
	.short	-123, -5241
	.short	-64, -2727
	.short	-253, -10781
	.short	306, 13039
	.short	289, 12315
	.short	-231, -9843
	.short	-12, -511
	.short	-25, -1065
	.short	-94, -4005
	.short	-324, -13806
	.short	113, 4815
	.short	-85, -3622
	.short	-43, -1832
	.short	359, 15297
	.short	-251, -10695
	.short	182, 7755
	.short	-300, -12783
	.short	144, 6136
	.short	377, 16064
	.short	-304, -12954
	.short	311, 13252
	.short	-57, -2429
	.short	27, 1151
	.short	-136, -5795
	.short	-173, -7372
	.short	-40, -1704

	.p2align	1, 0x0 // @qinv
qinv:
	.short	0 // 0x0
	.short	1 // 0x1
	.short	385 // 0x181
	.short	513 // 0x201
	.short	577 // 0x241
	.short	154 // 0x9a
	.short	641 // 0x281
	.short	110 // 0x6e
	.short	673 // 0x2a1
	.short	171 // 0xab
	.short	77 // 0x4d
	.short	70 // 0x46
	.short	705 // 0x2c1
	.short	355 // 0x163
	.short	55 // 0x37
	.short	564 // 0x234
	.short	721 // 0x2d1
	.short	181 // 0xb5
	.short	470 // 0x1d6
	.short	81 // 0x51
	.short	423 // 0x1a7
	.short	293 // 0x125
	.short	35 // 0x23
	.short	535 // 0x217
	.short	737 // 0x2e1
	.short	646 // 0x286
	.short	562 // 0x232
	.short	57 // 0x39
	.short	412 // 0x19c
	.short	716 // 0x2cc
	.short	282 // 0x11a
	.short	645 // 0x285
	.short	745 // 0x2e9
	.short	536 // 0x218
	.short	475 // 0x1db
	.short	22 // 0x16
	.short	235 // 0xeb
	.short	291 // 0x123
	.short	425 // 0x1a9
	.short	631 // 0x277
	.short	596 // 0x254
	.short	694 // 0x2b6
	.short	531 // 0x213
	.short	465 // 0x1d1
	.short	402 // 0x192
	.short	188 // 0xbc
	.short	652 // 0x28c
	.short	180 // 0xb4
	.short	753 // 0x2f1
	.short	565 // 0x235
	.short	323 // 0x143
	.short	573 // 0x23d
	.short	281 // 0x119
	.short	740 // 0x2e4
	.short	413 // 0x19d
	.short	14 // 0xe
	.short	206 // 0xce
	.short	27 // 0x1b
	.short	358 // 0x166
	.short	378 // 0x17a
	.short	141 // 0x8d
	.short	353 // 0x161
	.short	707 // 0x2c3
	.short	354 // 0x162
	.short	757 // 0x2f5
	.short	71 // 0x47
	.short	268 // 0x10c
	.short	264 // 0x108
	.short	622 // 0x26e
	.short	691 // 0x2b3
	.short	11 // 0xb
	.short	65 // 0x41
	.short	502 // 0x1f6
	.short	611 // 0x263
	.short	530 // 0x212
	.short	728 // 0x2d8
	.short	597 // 0x255
	.short	10 // 0xa
	.short	700 // 0x2bc
	.short	623 // 0x26f
	.short	298 // 0x12a
	.short	19 // 0x13
	.short	347 // 0x15b
	.short	454 // 0x1c6
	.short	650 // 0x28a
	.short	190 // 0xbe
	.short	617 // 0x269
	.short	495 // 0x1ef
	.short	201 // 0xc9
	.short	553 // 0x229
	.short	94 // 0x5e
	.short	600 // 0x258
	.short	326 // 0x146
	.short	215 // 0xd7
	.short	90 // 0x5a
	.short	170 // 0xaa
	.short	761 // 0x2f9
	.short	111 // 0x6f
	.short	667 // 0x29b
	.short	435 // 0x1b3
	.short	546 // 0x222
	.short	434 // 0x1b2
	.short	671 // 0x29f
	.short	112 // 0x70
	.short	525 // 0x20d
	.short	520 // 0x208
	.short	370 // 0x172
	.short	115 // 0x73
	.short	591 // 0x24f
	.short	127 // 0x7f
	.short	7 // 0x7
	.short	97 // 0x61
	.short	103 // 0x67
	.short	245 // 0xf5
	.short	398 // 0x18e
	.short	107 // 0x6b
	.short	179 // 0xb3
	.short	723 // 0x2d3
	.short	189 // 0xbd
	.short	685 // 0x2ad
	.short	455 // 0x1c7
	.short	286 // 0x11e
	.short	561 // 0x231
	.short	744 // 0x2e8
	.short	738 // 0x2e2
	.short	283 // 0x11b
	.short	177 // 0xb1
	.short	109 // 0x6d
	.short	763 // 0x2fb
	.short	155 // 0x9b
	.short	420 // 0x1a4
	.short	317 // 0x13d
	.short	134 // 0x86
	.short	451 // 0x1c3
	.short	132 // 0x84
	.short	319 // 0x13f
	.short	311 // 0x137
	.short	595 // 0x253
	.short	730 // 0x2da
	.short	426 // 0x1aa
	.short	390 // 0x186
	.short	60 // 0x3c
	.short	417 // 0x1a1
	.short	242 // 0xf2
	.short	251 // 0xfb
	.short	297 // 0x129
	.short	690 // 0x2b2
	.short	701 // 0x2bd
	.short	265 // 0x109
	.short	160 // 0xa0
	.short	364 // 0x16c
	.short	494 // 0x1ee
	.short	683 // 0x2ab
	.short	191 // 0xbf
	.short	5 // 0x5
	.short	129 // 0x81
	.short	350 // 0x15e
	.short	529 // 0x211
	.short	696 // 0x2b8
	.short	503 // 0x1f7
	.short	149 // 0x95
	.short	406 // 0x196
	.short	394 // 0x18a
	.short	184 // 0xb8
	.short	558 // 0x22e
	.short	261 // 0x105
	.short	227 // 0xe3
	.short	571 // 0x23b
	.short	325 // 0x145
	.short	678 // 0x2a6
	.short	95 // 0x5f
	.short	9 // 0x9
	.short	693 // 0x2b5
	.short	729 // 0x2d9
	.short	632 // 0x278
	.short	312 // 0x138
	.short	485 // 0x1e5
	.short	126 // 0x7e
	.short	661 // 0x295
	.short	116 // 0x74
	.short	47 // 0x2f
	.short	17 // 0x11
	.short	300 // 0x12c
	.short	374 // 0x176
	.short	163 // 0xa3
	.short	212 // 0xd4
	.short	492 // 0x1ec
	.short	366 // 0x16e
	.short	45 // 0x2d
	.short	118 // 0x76
	.short	85 // 0x55
	.short	153 // 0x99
	.short	765 // 0x2fd
	.short	514 // 0x202
	.short	440 // 0x1b8
	.short	280 // 0x118
	.short	718 // 0x2ce
	.short	324 // 0x144
	.short	602 // 0x25a
	.short	228 // 0xe4
	.short	273 // 0x111
	.short	88 // 0x58
	.short	217 // 0xd9
	.short	322 // 0x142
	.short	720 // 0x2d0
	.short	754 // 0x2f2
	.short	56 // 0x38
	.short	743 // 0x2e7
	.short	647 // 0x287
	.short	287 // 0x11f
	.short	260 // 0x104
	.short	605 // 0x25d
	.short	185 // 0xb9
	.short	278 // 0x116
	.short	442 // 0x1ba
	.short	93 // 0x5d
	.short	680 // 0x2a8
	.short	202 // 0xca
	.short	448 // 0x1c0
	.short	460 // 0x1cc
	.short	388 // 0x184
	.short	428 // 0x1ac
	.short	433 // 0x1b1
	.short	669 // 0x29d
	.short	436 // 0x1b4
	.short	499 // 0x1f3
	.short	507 // 0x1fb
	.short	166 // 0xa6
	.short	199 // 0xc7
	.short	497 // 0x1f1
	.short	438 // 0x1b6
	.short	516 // 0x204
	.short	474 // 0x1da
	.short	736 // 0x2e0
	.short	746 // 0x2ea
	.short	36 // 0x24
	.short	479 // 0x1df
	.short	464 // 0x1d0
	.short	727 // 0x2d7
	.short	695 // 0x2b7
	.short	612 // 0x264
	.short	351 // 0x15f
	.short	143 // 0x8f
	.short	519 // 0x207
	.short	665 // 0x299
	.short	113 // 0x71
	.short	372 // 0x174
	.short	302 // 0x12e
	.short	369 // 0x171
	.short	664 // 0x298
	.short	526 // 0x20e
	.short	144 // 0x90
	.short	473 // 0x1d9
	.short	538 // 0x21a
	.short	439 // 0x1b7
	.short	576 // 0x240
	.short	766 // 0x2fe
	.short	386 // 0x182
	.short	462 // 0x1ce
	.short	481 // 0x1e1
	.short	210 // 0xd2
	.short	165 // 0xa5
	.short	543 // 0x21f
	.short	500 // 0x1f4
	.short	67 // 0x43
	.short	148 // 0x94
	.short	610 // 0x262
	.short	697 // 0x2b9
	.short	66 // 0x42
	.short	506 // 0x1fa
	.short	544 // 0x220
	.short	437 // 0x1b5
	.short	540 // 0x21c
	.short	200 // 0xc8
	.short	682 // 0x2aa
	.short	618 // 0x26a
	.short	365 // 0x16d
	.short	583 // 0x247
	.short	213 // 0xd5
	.short	328 // 0x148
	.short	195 // 0xc3
	.short	52 // 0x34
	.short	30 // 0x1e
	.short	125 // 0x7d
	.short	593 // 0x251
	.short	313 // 0x139
	.short	121 // 0x79
	.short	209 // 0xd1
	.short	510 // 0x1fe
	.short	463 // 0x1cf
	.short	533 // 0x215
	.short	37 // 0x25
	.short	345 // 0x159
	.short	21 // 0x15
	.short	735 // 0x2df
	.short	537 // 0x219
	.short	517 // 0x205
	.short	145 // 0x91
	.short	80 // 0x50
	.short	751 // 0x2ef
	.short	182 // 0xb6
	.short	396 // 0x18c
	.short	247 // 0xf7
	.short	401 // 0x191
	.short	726 // 0x2d6
	.short	532 // 0x214
	.short	480 // 0x1e0
	.short	511 // 0x1ff
	.short	387 // 0x183
	.short	550 // 0x226
	.short	449 // 0x1c1
	.short	136 // 0x88
	.short	175 // 0xaf
	.short	285 // 0x11d
	.short	649 // 0x289
	.short	686 // 0x2ae
	.short	348 // 0x15c
	.short	131 // 0x83
	.short	636 // 0x27c
	.short	135 // 0x87
	.short	459 // 0x1cb
	.short	551 // 0x227
	.short	203 // 0xcb
	.short	50 // 0x32
	.short	197 // 0xc5
	.short	168 // 0xa8
	.short	92 // 0x5c
	.short	555 // 0x22b
	.short	279 // 0x117
	.short	575 // 0x23f
	.short	515 // 0x203
	.short	539 // 0x21b
	.short	498 // 0x1f2
	.short	545 // 0x221
	.short	670 // 0x29e
	.short	668 // 0x29c
	.short	547 // 0x223
	.short	429 // 0x1ad
	.short	339 // 0x153
	.short	338 // 0x152
	.short	432 // 0x1b0
	.short	548 // 0x224
	.short	389 // 0x185
	.short	630 // 0x276
	.short	731 // 0x2db
	.short	292 // 0x124
	.short	749 // 0x2ed
	.short	82 // 0x52
	.short	316 // 0x13c
	.short	639 // 0x27f
	.short	156 // 0x9c
	.short	241 // 0xf1
	.short	627 // 0x273
	.short	61 // 0x3d
	.short	63 // 0x3f
	.short	13 // 0xd
	.short	715 // 0x2cb
	.short	741 // 0x2e5
	.short	58 // 0x3a
	.short	392 // 0x188
	.short	408 // 0x198
	.short	409 // 0x199
	.short	393 // 0x189
	.short	608 // 0x260
	.short	150 // 0x96
	.short	276 // 0x114
	.short	187 // 0xbb
	.short	725 // 0x2d5
	.short	466 // 0x1d2
	.short	248 // 0xf8
	.short	106 // 0x6a
	.short	655 // 0x28f
	.short	246 // 0xf6
	.short	468 // 0x1d4
	.short	183 // 0xb7
	.short	607 // 0x25f
	.short	407 // 0x197
	.short	410 // 0x19a
	.short	59 // 0x3b
	.short	629 // 0x275
	.short	427 // 0x1ab
	.short	549 // 0x225
	.short	461 // 0x1cd
	.short	512 // 0x200
	.short	767 // 0x2ff
	.short	2 // 0x2
	.short	257 // 0x101
	.short	308 // 0x134
	.short	220 // 0xdc
	.short	342 // 0x156
	.short	140 // 0x8c
	.short	710 // 0x2c6
	.short	359 // 0x167
	.short	362 // 0x16a
	.short	162 // 0xa2
	.short	586 // 0x24a
	.short	301 // 0x12d
	.short	523 // 0x20b
	.short	114 // 0x72
	.short	663 // 0x297
	.short	521 // 0x209
	.short	303 // 0x12f
	.short	44 // 0x2c
	.short	582 // 0x246
	.short	493 // 0x1ed
	.short	619 // 0x26b
	.short	161 // 0xa1
	.short	376 // 0x178
	.short	360 // 0x168
	.short	361 // 0x169
	.short	377 // 0x179
	.short	711 // 0x2c7
	.short	28 // 0x1c
	.short	54 // 0x36
	.short	756 // 0x2f4
	.short	706 // 0x2c2
	.short	708 // 0x2c4
	.short	142 // 0x8e
	.short	528 // 0x210
	.short	613 // 0x265
	.short	130 // 0x82
	.short	453 // 0x1c5
	.short	687 // 0x2af
	.short	20 // 0x14
	.short	477 // 0x1dd
	.short	38 // 0x26
	.short	139 // 0x8b
	.short	380 // 0x17c
	.short	221 // 0xdd
	.short	337 // 0x151
	.short	431 // 0x1af
	.short	430 // 0x1ae
	.short	340 // 0x154
	.short	222 // 0xde
	.short	101 // 0x65
	.short	99 // 0x63
	.short	224 // 0xe0
	.short	271 // 0x10f
	.short	230 // 0xe6
	.short	254 // 0xfe
	.short	194 // 0xc2
	.short	490 // 0x1ea
	.short	214 // 0xd6
	.short	677 // 0x2a5
	.short	601 // 0x259
	.short	572 // 0x23c
	.short	719 // 0x2cf
	.short	566 // 0x236
	.short	218 // 0xda
	.short	310 // 0x136
	.short	634 // 0x27a
	.short	133 // 0x85
	.short	638 // 0x27e
	.short	421 // 0x1a5
	.short	83 // 0x53
	.short	120 // 0x78
	.short	484 // 0x1e4
	.short	594 // 0x252
	.short	633 // 0x279
	.short	320 // 0x140
	.short	219 // 0xdb
	.short	382 // 0x17e
	.short	258 // 0x102
	.short	289 // 0x121
	.short	237 // 0xed
	.short	43 // 0x2b
	.short	368 // 0x170
	.short	522 // 0x20a
	.short	373 // 0x175
	.short	587 // 0x24b
	.short	18 // 0x12
	.short	689 // 0x2b1
	.short	624 // 0x270
	.short	252 // 0xfc
	.short	232 // 0xe8
	.short	34 // 0x22
	.short	748 // 0x2ec
	.short	424 // 0x1a8
	.short	732 // 0x2dc
	.short	236 // 0xec
	.short	306 // 0x132
	.short	259 // 0x103
	.short	560 // 0x230
	.short	648 // 0x288
	.short	456 // 0x1c8
	.short	176 // 0xb0
	.short	644 // 0x284
	.short	739 // 0x2e3
	.short	717 // 0x2cd
	.short	574 // 0x23e
	.short	441 // 0x1b9
	.short	556 // 0x22c
	.short	186 // 0xba
	.short	404 // 0x194
	.short	151 // 0x97
	.short	87 // 0x57
	.short	569 // 0x239
	.short	229 // 0xe5
	.short	332 // 0x14c
	.short	225 // 0xe1
	.short	263 // 0x107
	.short	703 // 0x2bf
	.short	72 // 0x48
	.short	159 // 0x9f
	.short	621 // 0x26d
	.short	702 // 0x2be
	.short	269 // 0x10d
	.short	226 // 0xe2
	.short	604 // 0x25c
	.short	559 // 0x22f
	.short	288 // 0x120
	.short	307 // 0x133
	.short	383 // 0x17f
	.short	3 // 0x3
	.short	193 // 0xc1
	.short	330 // 0x14a
	.short	231 // 0xe7
	.short	296 // 0x128
	.short	625 // 0x271
	.short	243 // 0xf3
	.short	105 // 0x69
	.short	400 // 0x190
	.short	467 // 0x1d3
	.short	397 // 0x18d
	.short	656 // 0x290
	.short	104 // 0x68
	.short	250 // 0xfa
	.short	626 // 0x272
	.short	418 // 0x1a2
	.short	157 // 0x9d
	.short	74 // 0x4a
	.short	42 // 0x2a
	.short	305 // 0x131
	.short	290 // 0x122
	.short	733 // 0x2dd
	.short	23 // 0x17
	.short	33 // 0x21
	.short	295 // 0x127
	.short	253 // 0xfd
	.short	331 // 0x14b
	.short	272 // 0x110
	.short	570 // 0x23a
	.short	603 // 0x25b
	.short	262 // 0x106
	.short	270 // 0x10e
	.short	333 // 0x14d
	.short	100 // 0x64
	.short	336 // 0x150
	.short	341 // 0x155
	.short	381 // 0x17d
	.short	309 // 0x135
	.short	321 // 0x141
	.short	567 // 0x237
	.short	89 // 0x59
	.short	676 // 0x2a4
	.short	327 // 0x147
	.short	491 // 0x1eb
	.short	584 // 0x248
	.short	164 // 0xa4
	.short	509 // 0x1fd
	.short	482 // 0x1e2
	.short	122 // 0x7a
	.short	26 // 0x1a
	.short	713 // 0x2c9
	.short	15 // 0xf
	.short	49 // 0x31
	.short	447 // 0x1bf
	.short	552 // 0x228
	.short	681 // 0x2a9
	.short	496 // 0x1f0
	.short	541 // 0x21d
	.short	167 // 0xa7
	.short	445 // 0x1bd
	.short	51 // 0x33
	.short	489 // 0x1e9
	.short	329 // 0x149
	.short	255 // 0xff
	.short	4 // 0x4
	.short	616 // 0x268
	.short	684 // 0x2ac
	.short	651 // 0x28b
	.short	724 // 0x2d4
	.short	403 // 0x193
	.short	277 // 0x115
	.short	557 // 0x22d
	.short	606 // 0x25e
	.short	395 // 0x18b
	.short	469 // 0x1d5
	.short	752 // 0x2f0
	.short	722 // 0x2d2
	.short	653 // 0x28d
	.short	108 // 0x6c
	.short	643 // 0x283
	.short	284 // 0x11c
	.short	457 // 0x1c9
	.short	137 // 0x89
	.short	40 // 0x28
	.short	76 // 0x4c
	.short	760 // 0x2f8
	.short	674 // 0x2a2
	.short	91 // 0x5b
	.short	444 // 0x1bc
	.short	198 // 0xc6
	.short	542 // 0x21e
	.short	508 // 0x1fc
	.short	211 // 0xd3
	.short	585 // 0x249
	.short	375 // 0x177
	.short	363 // 0x16b
	.short	620 // 0x26c
	.short	266 // 0x10a
	.short	73 // 0x49
	.short	240 // 0xf0
	.short	419 // 0x1a3
	.short	640 // 0x280
	.short	764 // 0x2fc
	.short	578 // 0x242
	.short	86 // 0x56
	.short	275 // 0x113
	.short	405 // 0x195
	.short	609 // 0x261
	.short	504 // 0x1f8
	.short	68 // 0x44
	.short	79 // 0x4f
	.short	472 // 0x1d8
	.short	518 // 0x206
	.short	527 // 0x20f
	.short	352 // 0x160
	.short	709 // 0x2c5
	.short	379 // 0x17b
	.short	343 // 0x157
	.short	39 // 0x27
	.short	174 // 0xae
	.short	458 // 0x1ca
	.short	450 // 0x1c2
	.short	637 // 0x27d
	.short	318 // 0x13e
	.short	635 // 0x27b
	.short	452 // 0x1c4
	.short	349 // 0x15d
	.short	614 // 0x266
	.short	6 // 0x6
	.short	660 // 0x294
	.short	592 // 0x250
	.short	486 // 0x1e6
	.short	31 // 0x1f
	.short	25 // 0x19
	.short	208 // 0xd0
	.short	483 // 0x1e3
	.short	314 // 0x13a
	.short	84 // 0x54
	.short	580 // 0x244
	.short	46 // 0x2e
	.short	590 // 0x24e
	.short	662 // 0x296
	.short	371 // 0x173
	.short	524 // 0x20c
	.short	666 // 0x29a
	.short	672 // 0x2a0
	.short	762 // 0x2fa
	.short	642 // 0x282
	.short	178 // 0xb2
	.short	654 // 0x28e
	.short	399 // 0x18f
	.short	249 // 0xf9
	.short	244 // 0xf4
	.short	657 // 0x291
	.short	98 // 0x62
	.short	335 // 0x14f
	.short	223 // 0xdf
	.short	334 // 0x14e
	.short	102 // 0x66
	.short	658 // 0x292
	.short	8 // 0x8
	.short	599 // 0x257
	.short	679 // 0x2a7
	.short	554 // 0x22a
	.short	443 // 0x1bb
	.short	169 // 0xa9
	.short	675 // 0x2a3
	.short	216 // 0xd8
	.short	568 // 0x238
	.short	274 // 0x112
	.short	152 // 0x98
	.short	579 // 0x243
	.short	119 // 0x77
	.short	315 // 0x13b
	.short	422 // 0x1a6
	.short	750 // 0x2ee
	.short	471 // 0x1d7
	.short	146 // 0x92
	.short	69 // 0x45
	.short	759 // 0x2f7
	.short	172 // 0xac
	.short	41 // 0x29
	.short	239 // 0xef
	.short	158 // 0x9e
	.short	267 // 0x10b
	.short	704 // 0x2c0
	.short	758 // 0x2f6
	.short	78 // 0x4e
	.short	147 // 0x93
	.short	505 // 0x1f9
	.short	501 // 0x1f5
	.short	698 // 0x2ba
	.short	12 // 0xc
	.short	415 // 0x19f
	.short	62 // 0x3e
	.short	416 // 0x1a0
	.short	628 // 0x274
	.short	391 // 0x187
	.short	411 // 0x19b
	.short	742 // 0x2e6
	.short	563 // 0x233
	.short	755 // 0x2f3
	.short	356 // 0x164
	.short	29 // 0x1d
	.short	488 // 0x1e8
	.short	196 // 0xc4
	.short	446 // 0x1be
	.short	204 // 0xcc
	.short	16 // 0x10
	.short	589 // 0x24d
	.short	117 // 0x75
	.short	581 // 0x245
	.short	367 // 0x16f
	.short	304 // 0x130
	.short	238 // 0xee
	.short	75 // 0x4b
	.short	173 // 0xad
	.short	138 // 0x8a
	.short	344 // 0x158
	.short	478 // 0x1de
	.short	534 // 0x216
	.short	747 // 0x2eb
	.short	294 // 0x126
	.short	233 // 0xe9
	.short	24 // 0x18
	.short	124 // 0x7c
	.short	487 // 0x1e7
	.short	53 // 0x35
	.short	357 // 0x165
	.short	712 // 0x2c8
	.short	207 // 0xcf
	.short	123 // 0x7b
	.short	32 // 0x20
	.short	234 // 0xea
	.short	734 // 0x2de
	.short	476 // 0x1dc
	.short	346 // 0x15a
	.short	688 // 0x2b0
	.short	299 // 0x12b
	.short	588 // 0x24c
	.short	48 // 0x30
	.short	205 // 0xcd
	.short	714 // 0x2ca
	.short	414 // 0x19e
	.short	64 // 0x40
	.short	699 // 0x2bb
	.short	692 // 0x2b4
	.short	598 // 0x256
	.short	96 // 0x60
	.short	659 // 0x293
	.short	128 // 0x80
	.short	615 // 0x267
	.short	192 // 0xc0
	.short	256 // 0x100
	.short	384 // 0x180
	.short	768 // 0x300

	.p2align	1, 0x0 // @qinv_r2
qinv_r2:
	.short	0, 19, 394, 519, 197, 619, 644, 552, 483, 173, 694, 561, 322, 593, 276, 719
	.short	626, 363, 471, 1, 347, 184, 665, 168, 161, 739, 681, 314, 138, 531, 744, 720
	.short	313, 187, 566, 418, 620, 146, 385, 454, 558, 113, 92, 376, 717, 496, 84, 344
	.short	465, 738, 754, 121, 725, 218, 157, 266, 69, 513, 650, 261, 372, 555, 360, 574
	.short	541, 580, 478, 402, 283, 56, 209, 466, 310, 74, 73, 759, 577, 190, 227, 302
	.short	279, 361, 441, 167, 46, 534, 188, 177, 743, 510, 248, 634, 42, 240, 172, 154
	.short	617, 571, 369, 575, 377, 556, 445, 590, 747, 652, 109, 647, 463, 106, 133, 305
	.short	419, 41, 641, 495, 325, 664, 515, 711, 186, 51, 662, 294, 180, 763, 287, 533
	.short	655, 638, 290, 640, 239, 110, 201, 678, 526, 539, 28, 404, 489, 371, 233, 753
	.short	155, 260, 37, 246, 421, 733, 764, 158, 673, 553, 95, 144, 498, 54, 151, 329
	.short	524, 24, 565, 420, 605, 345, 468, 83, 23, 578, 267, 171, 94, 9, 473, 545
	.short	756, 87, 255, 666, 124, 323, 317, 185, 21, 183, 120, 33, 86, 704, 77, 600
	.short	693, 538, 670, 706, 569, 4, 672, 487, 573, 134, 278, 735, 607, 484, 295, 275
	.short	758, 70, 326, 729, 439, 668, 708, 229, 616, 762, 53, 281, 451, 442, 537, 407
	.short	594, 253, 405, 78, 705, 215, 632, 576, 547, 142, 332, 684, 642, 357, 740, 132
	.short	93, 517, 410, 633, 331, 609, 147, 355, 90, 312, 766, 429, 528, 225, 651, 178
	.short	712, 413, 319, 680, 145, 59, 320, 272, 504, 505, 55, 170, 485, 386, 339, 613
	.short	263, 724, 654, 207, 14, 311, 202, 80, 629, 219, 570, 68, 501, 564, 761, 126
	.short	462, 338, 130, 703, 403, 399, 123, 206, 595, 448, 751, 427, 382, 603, 79, 698
	.short	721, 111, 661, 481, 432, 453, 72, 277, 249, 32, 27, 730, 460, 182, 549, 258
	.short	262, 472, 12, 181, 667, 116, 210, 548, 687, 159, 557, 244, 234, 358, 426, 388
	.short	396, 461, 289, 270, 518, 415, 470, 435, 47, 165, 389, 20, 621, 606, 657, 734
	.short	378, 390, 428, 247, 512, 237, 333, 527, 62, 81, 546, 17, 543, 630, 477, 702
	.short	395, 98, 476, 141, 60, 433, 401, 767, 43, 100, 352, 416, 423, 434, 300, 500
	.short	731, 38, 269, 469, 335, 346, 353, 417, 669, 726, 2, 368, 336, 709, 628, 293
	.short	671, 374, 67, 292, 139, 226, 752, 223, 688, 707, 242, 436, 532, 257, 522, 341
	.short	379, 391, 35, 112, 163, 148, 749, 380, 604, 722, 334, 299, 354, 251, 499, 480
	.short	308, 373, 381, 343, 411, 535, 525, 212, 610, 82, 221, 559, 653, 102, 588, 757
	.short	297, 507, 511, 220, 587, 309, 39, 742, 737, 520, 492, 697, 316, 337, 288, 108
	.short	658, 48, 71, 690, 166, 387, 342, 18, 321, 174, 563, 646, 370, 366, 66, 639
	.short	431, 307, 643, 8, 205, 268, 701, 199, 550, 140, 689, 567, 458, 755, 562, 115
	.short	45, 506, 156, 430, 383, 284, 599, 714, 264, 265, 497, 449, 710, 624, 89, 450
	.short	356, 57, 591, 118, 544, 241, 340, 3, 457, 679, 414, 622, 160, 438, 136, 359
	.short	252, 676, 637, 29, 412, 127, 85, 437, 627, 222, 193, 137, 554, 64, 691, 364
	.short	516, 175, 362, 232, 327, 318, 488, 716, 7, 153, 540, 61, 101, 330, 40, 443
	.short	699, 11, 494, 474, 285, 162, 34, 491, 635, 196, 282, 97, 765, 200, 63, 99
	.short	231, 76, 169, 692, 65, 683, 736, 649, 586, 748, 584, 452, 446, 645, 103, 514
	.short	682, 13, 224, 296, 760, 675, 598, 502, 191, 746, 686, 301, 424, 164, 349, 204
	.short	745, 245, 440, 618, 715, 271, 625, 674, 216, 96, 611, 5, 36, 348, 523, 732
	.short	509, 614, 16, 536, 398, 280, 365, 741, 230, 243, 91, 568, 659, 530, 129, 479
	.short	131, 114, 236, 482, 6, 589, 475, 107, 718, 583, 58, 254, 105, 444, 274, 128
	.short	728, 350, 464, 636, 663, 306, 122, 660, 117, 22, 179, 324, 213, 392, 194, 400
	.short	198, 152, 615, 597, 529, 727, 135, 521, 259, 26, 592, 581, 235, 723, 602, 328
	.short	408, 490, 467, 542, 579, 192, 10, 696, 695, 459, 303, 560, 713, 486, 367, 291
	.short	189, 228, 195, 409, 214, 397, 508, 119, 256, 700, 503, 612, 551, 44, 648, 15
	.short	31, 304, 425, 685, 273, 52, 393, 677, 656, 211, 315, 384, 623, 149, 351, 203
	.short	582, 456, 49, 25, 238, 631, 455, 88, 30, 608, 601, 104, 585, 422, 768, 298
	.short	406, 143, 50, 493, 176, 447, 208, 75, 596, 286, 217, 125, 150, 572, 250, 375
	.short	750

	.p2align	4, 0x0 // @poly_basemul_ntt_lazy_ztab
poly_basemul_ntt_lazy_ztab:
	// block 0
	.short	-207, 207, 239, -239, 179, -179, 332, -332
	.short	-8821, 8821, 10184, -10184, 7627, -7627, 14147, -14147
	// block 1
	.short	301, -301, 206, -206, -264, 264, -219, 219
	.short	12826, -12826, 8778, -8778, -11249, 11249, -9332, 9332
	// block 2
	.short	130, -130, 370, -370, -183, 183, 189, -189
	.short	5539, -5539, 15766, -15766, -7798, 7798, 8054, -8054
	// block 3
	.short	-7, 7, 335, -335, -280, 280, 327, -327
	.short	-298, 298, 14275, -14275, -11931, 11931, 13934, -13934
	// block 4
	.short	92, -92, 321, -321, -165, 165, -233, 233
	.short	3920, -3920, 13678, -13678, -7031, 7031, -9928, 9928
	// block 5
	.short	208, -208, -177, 177, -139, 139, -159, 159
	.short	8863, -8863, -7542, 7542, -5923, 5923, -6775, 6775
	// block 6
	.short	284, -284, -79, 79, -175, 175, -84, 84
	.short	12102, -12102, -3366, 3366, -7457, 7457, -3579, 3579
	// block 7
	.short	174, -174, 22, -22, 39, -39, 111, -111
	.short	7414, -7414, 937, -937, 1662, -1662, 4730, -4730
	// block 8
	.short	-97, 97, 138, -138, -35, 35, 137, -137
	.short	-4133, 4133, 5880, -5880, -1491, 1491, 5838, -5838
	// block 9
	.short	-119, 119, 312, -312, -146, 146, 176, -176
	.short	-5071, 5071, 13295, -13295, -6221, 6221, 7500, -7500
	// block 10
	.short	-266, 266, -343, 343, 126, -126, 122, -122
	.short	-11335, 11335, -14616, 14616, 5369, -5369, 5199, -5199
	// block 11
	.short	-33, 33, 261, -261, 218, -218, -326, 326
	.short	-1406, 1406, 11122, -11122, 9289, -9289, -13891, 13891
	// block 12
	.short	214, -214, 195, -195, 101, -101, 110, -110
	.short	9119, -9119, 8309, -8309, 4304, -4304, 4687, -4687
	// block 13
	.short	-118, 118, 374, -374, -106, 106, 349, -349
	.short	-5028, 5028, 15937, -15937, -4517, 4517, 14871, -14871
	// block 14
	.short	-309, 309, 67, -67, -56, 56, 373, -373
	.short	-13167, 13167, 2855, -2855, -2386, 2386, 15894, -15894
	// block 15
	.short	271, -271, -116, 116, 74, -74, -26, 26
	.short	11548, -11548, -4943, 4943, 3153, -3153, -1108, 1108
// end expanded include: ntt_asm_parts/90_constants.inc
