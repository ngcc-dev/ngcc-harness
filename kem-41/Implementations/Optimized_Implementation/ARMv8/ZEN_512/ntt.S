// Hand-optimized AArch64 assembly kernels for ZEN_512 NTT arithmetic.
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
	add	x8, x0, #16
	mov	w9, #1 // =0x1
	mov	w10, #1024 // =0x400
	mov	w11, #64769 // =0xfd01
	fmov	s0, w11
	mov	w11, #769 // =0x301
	dup	v1.8h, w11
Lloh0:
	adrp	x11, f
Lloh1:
	add	x11, x11, :lo12:f
LBB1_1: // =>This Loop Header: Depth=1
                                        //     Child Loop BB1_2 Depth 2
                                        //       Child Loop BB1_3 Depth 3
	mov	x12, #0 // =0x0
	lsl	x13, x10, #1
LBB1_2: // Parent Loop BB1_1 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB1_3 Depth 3
	ldr	h2, [x11, w9, uxtw #1]
	mul	v3.8h, v2.8h, v0.8h
	dup	v3.8h, v3.h[0]
	add	x14, x12, x10
	add	x15, x8, x12, lsl #1
LBB1_3: // Parent Loop BB1_1 Depth=1
                                        //     Parent Loop BB1_2 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x15, #-16]
	add	x16, x15, x13
	ldp	q6, q7, [x16, #-16]
	smull	v16.4s, v6.4h, v2.h[0]
	smull2	v17.4s, v6.8h, v2.h[0]
	mul	v6.8h, v6.8h, v3.8h
	smull	v18.4s, v6.4h, v1.4h
	smull2	v6.4s, v6.8h, v1.8h
	subhn	v16.4h, v16.4s, v18.4s
	subhn2	v16.8h, v17.4s, v6.4s
	smull	v6.4s, v7.4h, v2.h[0]
	smull2	v17.4s, v7.8h, v2.h[0]
	mul	v7.8h, v7.8h, v3.8h
	smull	v18.4s, v7.4h, v1.4h
	smull2	v7.4s, v7.8h, v1.8h
	subhn	v6.4h, v6.4s, v18.4s
	subhn2	v6.8h, v17.4s, v7.4s
	add	v7.8h, v4.8h, v16.8h
	add	v17.8h, v5.8h, v6.8h
	stp	q7, q17, [x15, #-16]
	sub	v4.8h, v4.8h, v16.8h
	sub	v5.8h, v5.8h, v6.8h
	stp	q4, q5, [x16, #-16]
	add	x12, x12, #16
	add	x15, x15, #32
	cmp	x12, x14
	b.lo	LBB1_3
// %bb.4:                                ;   in Loop: Header=BB1_2 Depth=2
	add	w9, w9, #1
	add	x14, x10, x12
	mov	w12, w14
	cmp	w14, #2048
	b.lo	LBB1_2
// %bb.5:                                ;   in Loop: Header=BB1_1 Depth=1
	lsr	x12, x10, #1
	cmp	w10, #31
	mov	x10, x12
	b.hi	LBB1_1
// %bb.6:
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
	add	x8, x0, #16
	mov	w9, #1 // =0x1
	mov	w10, #1024 // =0x400
	mov	w11, #64769 // =0xfd01
	fmov	s0, w11
	mov	w11, #769 // =0x301
	dup	v1.8h, w11
Lloh2:
	adrp	x11, f
Lloh3:
	add	x11, x11, :lo12:f
LBB2_1: // =>This Loop Header: Depth=1
                                        //     Child Loop BB2_2 Depth 2
                                        //       Child Loop BB2_3 Depth 3
	mov	x12, #0 // =0x0
	lsl	x13, x10, #1
LBB2_2: // Parent Loop BB2_1 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB2_3 Depth 3
	ldr	h2, [x11, w9, uxtw #1]
	mul	v3.8h, v2.8h, v0.8h
	dup	v3.8h, v3.h[0]
	add	x14, x12, x10
	add	x15, x8, x12, lsl #1
LBB2_3: // Parent Loop BB2_1 Depth=1
                                        //     Parent Loop BB2_2 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x15, #-16]
	add	x16, x15, x13
	ldp	q6, q7, [x16, #-16]
	smull	v16.4s, v6.4h, v2.h[0]
	smull2	v17.4s, v6.8h, v2.h[0]
	mul	v6.8h, v6.8h, v3.8h
	smull	v18.4s, v6.4h, v1.4h
	smull2	v6.4s, v6.8h, v1.8h
	subhn	v16.4h, v16.4s, v18.4s
	subhn2	v16.8h, v17.4s, v6.4s
	smull	v6.4s, v7.4h, v2.h[0]
	smull2	v17.4s, v7.8h, v2.h[0]
	mul	v7.8h, v7.8h, v3.8h
	smull	v18.4s, v7.4h, v1.4h
	smull2	v7.4s, v7.8h, v1.8h
	subhn	v6.4h, v6.4s, v18.4s
	subhn2	v6.8h, v17.4s, v7.4s
	add	v7.8h, v4.8h, v16.8h
	add	v17.8h, v5.8h, v6.8h
	stp	q7, q17, [x15, #-16]
	sub	v4.8h, v4.8h, v16.8h
	sub	v5.8h, v5.8h, v6.8h
	stp	q4, q5, [x16, #-16]
	add	x12, x12, #16
	add	x15, x15, #32
	cmp	x12, x14
	b.lo	LBB2_3
// %bb.4:                                ;   in Loop: Header=BB2_2 Depth=2
	add	w9, w9, #1
	add	x14, x10, x12
	mov	w12, w14
	cmp	w14, #2048
	b.lo	LBB2_2
// %bb.5:                                ;   in Loop: Header=BB2_1 Depth=1
	lsr	x12, x10, #1
	cmp	w10, #31
	mov	x10, x12
	b.hi	LBB2_1
// %bb.6:
	mov	x8, #-8 // =0xfffffffffffffff8
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w9, #769 // =0x301
	dup	v3.8h, w9
LBB2_7: // =>This Inner Loop Header: Depth=1
	ldr	q4, [x0]
	smull	v5.4s, v4.4h, v0.4h
	smull2	v6.4s, v4.8h, v1.8h
	mul	v4.8h, v4.8h, v2.8h
	smull	v7.4s, v4.4h, v3.4h
	smull2	v16.4s, v4.8h, v3.8h
	subhn	v7.4h, v5.4s, v7.4s
	smlsl	v5.4s, v4.4h, v3.4h
	subhn2	v7.8h, v6.4s, v16.4s
	smlsl2	v6.4s, v4.8h, v3.8h
	cmlt	v4.4s, v6.4s, #0
	cmlt	v5.4s, v5.4s, #0
	uzp1	v4.8h, v5.8h, v4.8h
	and	v4.16b, v4.16b, v3.16b
	add	v4.8h, v4.8h, v7.8h
	str	q4, [x0], #16
	add	x8, x8, #8
	cmp	x8, #2040
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
	mov	w8, #0 // =0x0
	add	x9, x0, #16
	mov	w10, #16 // =0x10
	mov	w11, #64769 // =0xfd01
	fmov	s0, w11
	mov	w11, #769 // =0x301
	dup	v1.8h, w11
Lloh4:
	adrp	x11, fn
Lloh5:
	add	x11, x11, :lo12:fn
LBB3_1: // =>This Loop Header: Depth=1
                                        //     Child Loop BB3_2 Depth 2
                                        //       Child Loop BB3_3 Depth 3
	mov	x13, #0 // =0x0
	lsl	x12, x10, #1
LBB3_2: // Parent Loop BB3_1 Depth=1
                                        // =>  This Loop Header: Depth=2
                                        //       Child Loop BB3_3 Depth 3
	ldr	h2, [x11, w8, uxtw #1]
	mul	v3.8h, v2.8h, v0.8h
	dup	v3.8h, v3.h[0]
	add	x14, x13, x10
	add	x15, x9, x13, lsl #1
LBB3_3: // Parent Loop BB3_1 Depth=1
                                        //     Parent Loop BB3_2 Depth=2
                                        // =>    This Inner Loop Header: Depth=3
	ldp	q4, q5, [x15, #-16]
	add	x16, x15, x12
	ldp	q6, q7, [x16, #-16]
	add	v16.8h, v6.8h, v4.8h
	add	v17.8h, v7.8h, v5.8h
	stp	q16, q17, [x15, #-16]
	sub	v4.8h, v4.8h, v6.8h
	smull	v6.4s, v4.4h, v2.h[0]
	smull2	v16.4s, v4.8h, v2.h[0]
	mul	v4.8h, v4.8h, v3.8h
	smull	v17.4s, v4.4h, v1.4h
	smull2	v4.4s, v4.8h, v1.8h
	subhn	v6.4h, v6.4s, v17.4s
	subhn2	v6.8h, v16.4s, v4.4s
	sub	v4.8h, v5.8h, v7.8h
	smull	v5.4s, v4.4h, v2.h[0]
	smull2	v7.4s, v4.8h, v2.h[0]
	mul	v4.8h, v4.8h, v3.8h
	smull	v16.4s, v4.4h, v1.4h
	smull2	v4.4s, v4.8h, v1.8h
	subhn	v5.4h, v5.4s, v16.4s
	subhn2	v5.8h, v7.4s, v4.4s
	stp	q6, q5, [x16, #-16]
	add	x13, x13, #16
	add	x15, x15, #32
	cmp	x13, x14
	b.lo	LBB3_3
// %bb.4:                                ;   in Loop: Header=BB3_2 Depth=2
	add	w8, w8, #1
	add	x14, x10, x13
	mov	w13, w14
	cmp	w14, #2048
	b.lo	LBB3_2
// %bb.5:                                ;   in Loop: Header=BB3_1 Depth=1
	cmp	w10, #512
	mov	x10, x12
	b.lo	LBB3_1
// %bb.6:
	ldr	h0, [x11, w8, uxtw #1]
	mov	w8, #64769 // =0xfd01
	fmov	s1, w8
	mul	v1.8h, v0.8h, v1.8h
	dup	v1.8h, v1.h[0]
	mov	x8, #-16 // =0xfffffffffffffff0
	mov	w9, #769 // =0x301
	dup	v2.8h, w9
	mov	w9, #655 // =0x28f
	dup	v3.8h, w9
	mov	w9, #21903 // =0x558f
	dup	v4.8h, w9
LBB3_7: // =>This Inner Loop Header: Depth=1
	ldp	q16, q5, [x0]
	ldr	q17, [x0, #2048]
	ldr	q6, [x0, #2064]
	sub	v7.8h, v16.8h, v17.8h
	smull	v18.4s, v7.4h, v0.h[0]
	smull2	v19.4s, v7.8h, v0.h[0]
	mul	v7.8h, v7.8h, v1.8h
	smull	v20.4s, v7.4h, v2.4h
	smull2	v21.4s, v7.8h, v2.8h
	subhn	v18.4h, v18.4s, v20.4s
	subhn	v7.4h, v19.4s, v21.4s
	smull	v20.4s, v18.4h, v3.4h
	subhn2	v18.8h, v19.4s, v21.4s
	sub	v19.8h, v5.8h, v6.8h
	smull	v21.4s, v19.4h, v0.h[0]
	smull2	v22.4s, v19.8h, v0.h[0]
	mul	v19.8h, v19.8h, v1.8h
	smull	v23.4s, v19.4h, v2.4h
	smull2	v19.4s, v19.8h, v2.8h
	subhn	v21.4h, v21.4s, v23.4s
	subhn	v23.4h, v22.4s, v19.4s
	smull	v24.4s, v21.4h, v3.4h
	subhn2	v21.8h, v22.4s, v19.4s
	add	v16.8h, v17.8h, v16.8h
	smull	v17.4s, v16.4h, v3.4h
	smull2	v19.4s, v16.8h, v3.8h
	mul	v16.8h, v16.8h, v4.8h
	smull	v22.4s, v16.4h, v2.4h
	smull2	v16.4s, v16.8h, v2.8h
	subhn	v17.4h, v17.4s, v22.4s
	subhn2	v17.8h, v19.4s, v16.4s
	add	v5.8h, v6.8h, v5.8h
	smull	v6.4s, v5.4h, v3.4h
	smull2	v16.4s, v5.8h, v3.8h
	mul	v5.8h, v5.8h, v4.8h
	smull	v19.4s, v5.4h, v2.4h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v6.4h, v6.4s, v19.4s
	subhn2	v6.8h, v16.4s, v5.4s
	stp	q17, q6, [x0]
	smull	v5.4s, v7.4h, v3.4h
	mul	v6.8h, v18.8h, v4.8h
	smull	v7.4s, v6.4h, v2.4h
	smull2	v6.4s, v6.8h, v2.8h
	subhn	v7.4h, v20.4s, v7.4s
	subhn2	v7.8h, v5.4s, v6.4s
	str	q7, [x0, #2048]
	smull	v5.4s, v23.4h, v3.4h
	mul	v6.8h, v21.8h, v4.8h
	smull	v7.4s, v6.4h, v2.4h
	smull2	v6.4s, v6.8h, v2.8h
	subhn	v7.4h, v24.4s, v7.4s
	subhn2	v7.8h, v5.4s, v6.4s
	str	q7, [x0, #2064]
	add	x8, x8, #16
	add	x0, x0, #32
	cmp	x8, #1008
	b.lo	LBB3_7
// %bb.8:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/40_poly_intt.inc
// begin expanded include: ntt_asm_parts/50_poly_basemul_ntt.inc
	.globl	poly_basemul_ntt // -- Begin function poly_basemul_ntt
	.p2align	2
poly_basemul_ntt: // @poly_basemul_ntt
	.cfi_startproc
// %bb.0:
	stp	d15, d14, [sp, #-80]! // 16-byte Folded Spill
	stp	d13, d12, [sp, #16] // 16-byte Folded Spill
	stp	d11, d10, [sp, #32] // 16-byte Folded Spill
	stp	d9, d8, [sp, #48] // 16-byte Folded Spill
	stp	x28, x27, [sp, #64] // 16-byte Folded Spill
	sub	sp, sp, #1296
	.cfi_def_cfa_offset 1376
	.cfi_offset w27, -8
	.cfi_offset w28, -16
	.cfi_offset b8, -24
	.cfi_offset b9, -32
	.cfi_offset b10, -40
	.cfi_offset b11, -48
	.cfi_offset b12, -56
	.cfi_offset b13, -64
	.cfi_offset b14, -72
	.cfi_offset b15, -80
	add	x8, x1, #128
	add	x9, x2, #128
	mov	w10, #64769 // =0xfd01
	dup	v0.8h, w10
	str	q0, [sp, #944] // 16-byte Folded Spill
	mov	w10, #769 // =0x301
	dup	v0.8h, w10
	str	q0, [sp] // 16-byte Folded Spill
	add	x10, x0, #128
	mov	x11, #-4 // =0xfffffffffffffffc
Lloh6:
	adrp	x12, f+128
Lloh7:
	add	x12, x12, :lo12:f+128
LBB4_1: // =>This Inner Loop Header: Depth=1
	ldr	d0, [x12], #8
	ldp	q2, q3, [x8, #-128]
	ldp	q4, q5, [x8, #-96]
	ldp	q6, q7, [x8, #-64]
	ldp	q16, q17, [x8, #-32]
	ldp	q18, q20, [x8]
	neg	v19.4h, v0.4h
	ldp	q22, q24, [x8, #32]
	ldp	q23, q25, [x8, #64]
	zip1	v1.4h, v0.4h, v19.4h
	str	d1, [sp, #1160] // 8-byte Folded Spill
	ldp	q26, q27, [x8, #96]
	trn1	v28.8h, v2.8h, v4.8h
	trn2	v2.8h, v2.8h, v4.8h
	trn1	v4.8h, v6.8h, v16.8h
	zip2	v1.4h, v0.4h, v19.4h
	str	d1, [sp, #1240] // 8-byte Folded Spill
	trn2	v6.8h, v6.8h, v16.8h
	trn1	v16.8h, v18.8h, v22.8h
	trn2	v18.8h, v18.8h, v22.8h
	trn1	v22.8h, v23.8h, v26.8h
	trn2	v26.8h, v23.8h, v26.8h
	zip1	v0.8h, v0.8h, v19.8h
	str	q0, [sp, #1184] // 16-byte Folded Spill
	trn1	v31.4s, v28.4s, v4.4s
	trn2	v0.4s, v28.4s, v4.4s
	str	q0, [sp, #1200] // 16-byte Folded Spill
	trn1	v23.4s, v2.4s, v6.4s
	trn2	v0.4s, v2.4s, v6.4s
	str	q0, [sp, #1216] // 16-byte Folded Spill
	trn1	v8.4s, v16.4s, v22.4s
	trn2	v9.4s, v16.4s, v22.4s
	trn1	v29.4s, v18.4s, v26.4s
	trn1	v2.8h, v3.8h, v5.8h
	trn2	v5.8h, v3.8h, v5.8h
	trn1	v4.8h, v7.8h, v17.8h
	trn2	v6.8h, v7.8h, v17.8h
	trn2	v30.4s, v18.4s, v26.4s
	trn1	v7.8h, v20.8h, v24.8h
	trn2	v0.8h, v20.8h, v24.8h
	trn1	v16.8h, v25.8h, v27.8h
	trn2	v3.8h, v25.8h, v27.8h
	trn1	v1.4s, v2.4s, v4.4s
	str	q1, [sp, #1024] // 16-byte Folded Spill
	trn2	v1.4s, v2.4s, v4.4s
	str	q1, [sp, #1056] // 16-byte Folded Spill
	trn1	v1.4s, v5.4s, v6.4s
	str	q1, [sp, #1040] // 16-byte Folded Spill
	ldp	q17, q4, [x9, #-128]
	ldp	q18, q19, [x9, #-96]
	trn2	v1.4s, v5.4s, v6.4s
	str	q1, [sp, #656] // 16-byte Folded Spill
	ldp	q5, q6, [x9, #-64]
	ldp	q20, q2, [x9, #-32]
	ldp	q22, q13, [x9]
	trn1	v1.4s, v7.4s, v16.4s
	str	q1, [sp, #1136] // 16-byte Folded Spill
	trn1	v24.8h, v17.8h, v18.8h
	trn2	v17.8h, v17.8h, v18.8h
	ldp	q18, q14, [x9, #32]
	trn2	v1.4s, v7.4s, v16.4s
	str	q1, [sp, #1008] // 16-byte Folded Spill
	trn1	v7.8h, v5.8h, v20.8h
	trn2	v5.8h, v5.8h, v20.8h
	trn1	v16.8h, v22.8h, v18.8h
	trn2	v18.8h, v22.8h, v18.8h
	ldp	q20, q15, [x9, #64]
	ldp	q22, q21, [x9, #96]
	trn1	v11.8h, v20.8h, v22.8h
	trn2	v20.8h, v20.8h, v22.8h
	trn1	v1.4s, v0.4s, v3.4s
	str	q1, [sp, #928] // 16-byte Folded Spill
	trn2	v0.4s, v0.4s, v3.4s
	str	q0, [sp, #1120] // 16-byte Folded Spill
	trn1	v10.4s, v24.4s, v7.4s
	trn2	v1.4s, v24.4s, v7.4s
	trn1	v24.4s, v17.4s, v5.4s
	trn2	v28.4s, v17.4s, v5.4s
	trn1	v5.4s, v16.4s, v11.4s
	trn2	v22.4s, v16.4s, v11.4s
	trn1	v0.8h, v4.8h, v19.8h
	trn2	v3.8h, v4.8h, v19.8h
	trn1	v4.8h, v6.8h, v2.8h
	trn2	v7.8h, v6.8h, v2.8h
	trn1	v27.4s, v18.4s, v20.4s
	trn2	v26.4s, v18.4s, v20.4s
	trn1	v16.8h, v13.8h, v14.8h
	trn2	v17.8h, v13.8h, v14.8h
	trn1	v18.8h, v15.8h, v21.8h
	trn2	v19.8h, v15.8h, v21.8h
	trn1	v2.4s, v0.4s, v4.4s
	str	q2, [sp, #688] // 16-byte Folded Spill
	trn2	v4.4s, v0.4s, v4.4s
	trn1	v12.4s, v3.4s, v7.4s
	trn2	v2.4s, v3.4s, v7.4s
	trn1	v3.4s, v16.4s, v18.4s
	trn2	v25.4s, v16.4s, v18.4s
	trn1	v20.4s, v17.4s, v19.4s
	trn2	v14.4s, v17.4s, v19.4s
	ext	v0.16b, v24.16b, v24.16b, #8
	str	q0, [sp, #448] // 16-byte Folded Spill
	mov	v11.16b, v23.16b
	smull	v7.4s, v11.4h, v0.4h
	str	q1, [sp, #1088] // 16-byte Folded Spill
	ext	v0.16b, v1.16b, v1.16b, #8
	str	q0, [sp, #432] // 16-byte Folded Spill
	str	q31, [sp, #1280] // 16-byte Folded Spill
	smlal	v7.4s, v31.4h, v0.4h
	str	q27, [sp, #768] // 16-byte Folded Spill
	ext	v0.16b, v27.16b, v27.16b, #8
	str	q0, [sp, #464] // 16-byte Folded Spill
	mov	v13.16b, v29.16b
	smull	v16.4s, v13.4h, v0.4h
	str	q22, [sp, #896] // 16-byte Folded Spill
	ext	v0.16b, v22.16b, v22.16b, #8
	str	q0, [sp, #544] // 16-byte Folded Spill
	str	q8, [sp, #1248] // 16-byte Folded Spill
	smlal	v16.4s, v8.4h, v0.4h
	str	q10, [sp, #1072] // 16-byte Folded Spill
	ext	v0.16b, v10.16b, v10.16b, #8
	str	q0, [sp, #320] // 16-byte Folded Spill
	ldr	q21, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v21.4h, v0.4h
	str	q5, [sp, #1168] // 16-byte Folded Spill
	ext	v0.16b, v5.16b, v5.16b, #8
	str	q0, [sp, #304] // 16-byte Folded Spill
	str	q9, [sp, #1264] // 16-byte Folded Spill
	smlal	v16.4s, v9.4h, v0.4h
	ldr	q19, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v19.4h, v28.4h
	stp	q30, q29, [sp, #784] // 32-byte Folded Spill
	smlal	v16.4s, v30.4h, v26.4h
	ext	v0.16b, v31.16b, v31.16b, #8
	str	q0, [sp, #736] // 16-byte Folded Spill
	smlal	v7.4s, v0.4h, v1.4h
	ext	v0.16b, v8.16b, v8.16b, #8
	str	q0, [sp, #288] // 16-byte Folded Spill
	smlal	v16.4s, v0.4h, v22.4h
	ext	v0.16b, v23.16b, v23.16b, #8
	str	q0, [sp, #624] // 16-byte Folded Spill
	str	q23, [sp, #832] // 16-byte Folded Spill
	mov	v31.16b, v24.16b
	str	q24, [sp, #960] // 16-byte Folded Spill
	smlal	v7.4s, v0.4h, v31.4h
	ext	v0.16b, v29.16b, v29.16b, #8
	str	q0, [sp, #416] // 16-byte Folded Spill
	smlal	v16.4s, v0.4h, v27.4h
	ext	v0.16b, v21.16b, v21.16b, #8
	str	q0, [sp, #384] // 16-byte Folded Spill
	smlal	v7.4s, v0.4h, v10.4h
	ext	v0.16b, v9.16b, v9.16b, #8
	str	q0, [sp, #528] // 16-byte Folded Spill
	smlal	v16.4s, v0.4h, v5.4h
	uzp1	v17.8h, v7.8h, v16.8h
	ldr	q0, [sp, #944] // 16-byte Folded Reload
	mul	v17.8h, v17.8h, v0.8h
	mov	v23.16b, v0.16b
	ldr	q10, [sp] // 16-byte Folded Reload
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	mov	v24.16b, v4.16b
	stp	q12, q4, [sp, #848] // 32-byte Folded Spill
	ext	v1.16b, v4.16b, v4.16b, #8
	str	q1, [sp, #704] // 16-byte Folded Spill
	ldr	q9, [sp, #1024] // 16-byte Folded Reload
	smull	v17.4s, v9.4h, v1.4h
	mov	v29.16b, v2.16b
	str	q2, [sp, #1104] // 16-byte Folded Spill
	smlal2	v17.4s, v19.8h, v2.8h
	str	q25, [sp, #672] // 16-byte Folded Spill
	ext	v1.16b, v25.16b, v25.16b, #8
	str	q1, [sp, #752] // 16-byte Folded Spill
	ldr	q4, [sp, #1136] // 16-byte Folded Reload
	smull	v18.4s, v4.4h, v1.4h
	smlal2	v18.4s, v30.8h, v14.8h
	ext	v1.16b, v12.16b, v12.16b, #8
	str	q1, [sp, #592] // 16-byte Folded Spill
	ldr	q19, [sp, #1040] // 16-byte Folded Reload
	smlal	v17.4s, v19.4h, v1.4h
	mov	v6.16b, v20.16b
	str	q20, [sp, #816] // 16-byte Folded Spill
	ext	v1.16b, v20.16b, v20.16b, #8
	str	q1, [sp, #608] // 16-byte Folded Spill
	ldr	q20, [sp, #928] // 16-byte Folded Reload
	smlal	v18.4s, v20.4h, v1.4h
	ldr	q5, [sp, #688] // 16-byte Folded Reload
	ext	v1.16b, v5.16b, v5.16b, #8
	str	q1, [sp, #560] // 16-byte Folded Spill
	ldr	q22, [sp, #1056] // 16-byte Folded Reload
	smlal	v17.4s, v22.4h, v1.4h
	str	q3, [sp, #912] // 16-byte Folded Spill
	ext	v1.16b, v3.16b, v3.16b, #8
	str	q1, [sp, #576] // 16-byte Folded Spill
	ldr	q30, [sp, #1008] // 16-byte Folded Reload
	smlal	v18.4s, v30.4h, v1.4h
	ldr	q27, [sp, #656] // 16-byte Folded Reload
	smlal	v17.4s, v27.4h, v29.4h
	ldr	q29, [sp, #1120] // 16-byte Folded Reload
	smlal	v18.4s, v29.4h, v14.4h
	ext	v1.16b, v9.16b, v9.16b, #8
	str	q1, [sp, #256] // 16-byte Folded Spill
	smlal	v17.4s, v1.4h, v24.4h
	ext	v0.16b, v4.16b, v4.16b, #8
	str	q0, [sp, #720] // 16-byte Folded Spill
	smlal	v18.4s, v0.4h, v25.4h
	ext	v0.16b, v19.16b, v19.16b, #8
	str	q0, [sp, #352] // 16-byte Folded Spill
	mov	v8.16b, v19.16b
	smlal	v17.4s, v0.4h, v12.4h
	ext	v0.16b, v20.16b, v20.16b, #8
	str	q0, [sp, #368] // 16-byte Folded Spill
	mov	v4.16b, v20.16b
	smlal	v18.4s, v0.4h, v6.4h
	ext	v0.16b, v22.16b, v22.16b, #8
	str	q0, [sp, #336] // 16-byte Folded Spill
	smlal	v17.4s, v0.4h, v5.4h
	ext	v0.16b, v30.16b, v30.16b, #8
	str	q0, [sp, #512] // 16-byte Folded Spill
	smlal	v18.4s, v0.4h, v3.4h
	smlal2	v17.4s, v27.8h, v28.8h
	smlal2	v18.4s, v29.8h, v26.8h
	mov	v12.16b, v29.16b
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v23.8h
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	ldr	q0, [sp, #1184] // 16-byte Folded Reload
	mul	v0.8h, v0.8h, v23.8h
	str	q0, [sp, #1184] // 16-byte Folded Spill
	mov	v6.16b, v23.16b
	ldr	d15, [sp, #1240] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v15.4h
	mul	v19.8h, v20.8h, v0.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	ldr	d25, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v25.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v3.4h, #171
	smull	v17.4s, v7.4h, v3.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v0.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v3.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v0.8h, v16.4s, v7.4s
	str	q0, [sp, #192] // 16-byte Folded Spill
	ldp	q0, q29, [sp, #432] // 32-byte Folded Reload
	smull	v7.4s, v11.4h, v0.4h
	str	q28, [sp, #976] // 16-byte Folded Spill
	ext	v0.16b, v28.16b, v28.16b, #8
	str	q0, [sp, #400] // 16-byte Folded Spill
	ldr	q2, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v0.4h
	ldr	q0, [sp, #544] // 16-byte Folded Reload
	smull	v16.4s, v13.4h, v0.4h
	str	q26, [sp, #880] // 16-byte Folded Spill
	ext	v0.16b, v26.16b, v26.16b, #8
	str	q0, [sp, #240] // 16-byte Folded Spill
	ldr	q1, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v0.4h
	smlal	v7.4s, v21.4h, v29.4h
	ldr	q0, [sp, #464] // 16-byte Folded Reload
	ldr	q1, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v0.4h
	ldr	q3, [sp, #1216] // 16-byte Folded Reload
	ldp	q2, q24, [sp, #304] // 32-byte Folded Reload
	smlal	v7.4s, v3.4h, v24.4h
	ldp	q23, q1, [sp, #768] // 32-byte Folded Reload
	smlal	v16.4s, v1.4h, v2.4h
	ldr	q0, [sp, #736] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v28.4h
	ldr	q28, [sp, #288] // 16-byte Folded Reload
	smlal	v16.4s, v28.4h, v26.4h
	ldr	q0, [sp, #1088] // 16-byte Folded Reload
	ldr	q5, [sp, #624] // 16-byte Folded Reload
	smlal	v7.4s, v5.4h, v0.4h
	ldr	q0, [sp, #896] // 16-byte Folded Reload
	ldr	q5, [sp, #416] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v0.4h
	ldr	q0, [sp, #384] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v31.4h
	ldr	q0, [sp, #528] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v23.4h
	ext	v0.16b, v3.16b, v3.16b, #8
	str	q0, [sp, #208] // 16-byte Folded Spill
	ldr	q3, [sp, #1072] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v3.4h
	ext	v0.16b, v1.16b, v1.16b, #8
	str	q0, [sp, #224] // 16-byte Folded Spill
	ldr	q3, [sp, #1168] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v3.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mov	v26.16b, v6.16b
	mul	v17.8h, v17.8h, v6.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	ldr	q0, [sp, #704] // 16-byte Folded Reload
	smull	v17.4s, v8.4h, v0.4h
	ldr	q20, [sp, #1104] // 16-byte Folded Reload
	ext	v0.16b, v20.16b, v20.16b, #8
	str	q0, [sp, #272] // 16-byte Folded Spill
	smlal	v17.4s, v9.4h, v0.4h
	ldr	q0, [sp, #752] // 16-byte Folded Reload
	smull	v18.4s, v4.4h, v0.4h
	str	q14, [sp, #992] // 16-byte Folded Spill
	ext	v0.16b, v14.16b, v14.16b, #8
	str	q0, [sp, #480] // 16-byte Folded Spill
	ldr	q11, [sp, #1136] // 16-byte Folded Reload
	smlal	v18.4s, v11.4h, v0.4h
	ldp	q6, q9, [sp, #592] // 32-byte Folded Reload
	smlal	v17.4s, v22.4h, v6.4h
	smlal	v18.4s, v30.4h, v9.4h
	mov	v22.16b, v27.16b
	ldp	q3, q4, [sp, #560] // 32-byte Folded Reload
	smlal	v17.4s, v22.4h, v3.4h
	mov	v19.16b, v12.16b
	smlal	v18.4s, v19.4h, v4.4h
	ldr	q8, [sp, #256] // 16-byte Folded Reload
	smlal	v17.4s, v8.4h, v20.4h
	ldr	q0, [sp, #720] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v14.4h
	ldp	q14, q12, [sp, #352] // 32-byte Folded Reload
	ldp	q5, q21, [sp, #848] // 32-byte Folded Reload
	smlal	v17.4s, v14.4h, v21.4h
	ldr	q13, [sp, #672] // 16-byte Folded Reload
	smlal	v18.4s, v12.4h, v13.4h
	ldr	q0, [sp, #336] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v5.4h
	ldr	q27, [sp, #816] // 16-byte Folded Reload
	ldr	q0, [sp, #512] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v27.4h
	ext	v0.16b, v22.16b, v22.16b, #8
	str	q0, [sp, #640] // 16-byte Folded Spill
	ldr	q22, [sp, #688] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v22.4h
	ext	v0.16b, v19.16b, v19.16b, #8
	str	q0, [sp, #496] // 16-byte Folded Spill
	ldr	q19, [sp, #912] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v19.4h
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v26.8h
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	smull	v18.4s, v18.4h, v15.4h
	ldr	q0, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v0.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	smull	v17.4s, v17.4h, v25.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	mov	v17.16b, v7.16b
	mov	v17.d[1], v16.d[0]
	movi	v19.4h, #171
	smull	v7.4s, v7.4h, v19.4h
	mvni	v0.8h, #84
	mul	v17.8h, v17.8h, v0.8h
	smull	v18.4s, v17.4h, v10.4h
	subhn	v0.4h, v7.4s, v18.4s
	smull	v7.4s, v16.4h, v19.4h
	smull2	v16.4s, v17.8h, v10.8h
	subhn2	v0.8h, v7.4s, v16.4s
	str	q0, [sp, #176] // 16-byte Folded Spill
	ldr	q15, [sp, #832] // 16-byte Folded Reload
	smull	v7.4s, v15.4h, v24.4h
	ldr	q0, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v29.4h
	ldr	q29, [sp, #800] // 16-byte Folded Reload
	smull	v16.4s, v29.4h, v2.4h
	ldr	q25, [sp, #464] // 16-byte Folded Reload
	ldr	q0, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v25.4h
	ldr	q26, [sp, #1200] // 16-byte Folded Reload
	ldp	q17, q30, [sp, #960] // 32-byte Folded Reload
	smlal	v7.4s, v26.4h, v30.4h
	ldr	q24, [sp, #1264] // 16-byte Folded Reload
	ldp	q31, q20, [sp, #880] // 32-byte Folded Reload
	smlal	v16.4s, v24.4h, v31.4h
	ldr	q0, [sp, #1216] // 16-byte Folded Reload
	ldr	q19, [sp, #1088] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v19.4h
	smlal	v16.4s, v1.4h, v20.4h
	ldr	q2, [sp, #736] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v17.4h
	smlal	v16.4s, v28.4h, v23.4h
	ldr	q28, [sp, #624] // 16-byte Folded Reload
	ldr	q2, [sp, #1072] // 16-byte Folded Reload
	smlal	v7.4s, v28.4h, v2.4h
	ldr	q23, [sp, #416] // 16-byte Folded Reload
	ldr	q2, [sp, #1168] // 16-byte Folded Reload
	smlal	v16.4s, v23.4h, v2.4h
	uzp1	v17.8h, v7.8h, v16.8h
	ldr	q2, [sp, #944] // 16-byte Folded Reload
	mul	v17.8h, v17.8h, v2.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	smull2	v17.4s, v0.8h, v21.8h
	ldr	q0, [sp, #1104] // 16-byte Folded Reload
	smlal2	v17.4s, v26.8h, v0.8h
	smull2	v18.4s, v1.8h, v13.8h
	ldr	q26, [sp, #992] // 16-byte Folded Reload
	smlal2	v18.4s, v24.8h, v26.8h
	ldr	q1, [sp, #1024] // 16-byte Folded Reload
	smlal	v17.4s, v1.4h, v6.4h
	smlal	v18.4s, v11.4h, v9.4h
	ldr	q9, [sp, #1040] // 16-byte Folded Reload
	smlal	v17.4s, v9.4h, v3.4h
	ldr	q1, [sp, #928] // 16-byte Folded Reload
	smlal	v18.4s, v1.4h, v4.4h
	ldr	q1, [sp, #1056] // 16-byte Folded Reload
	smlal	v17.4s, v1.4h, v0.4h
	ldr	q0, [sp, #1008] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v26.4h
	ldr	q6, [sp, #656] // 16-byte Folded Reload
	smlal	v17.4s, v6.4h, v21.4h
	ldr	q3, [sp, #1120] // 16-byte Folded Reload
	smlal	v18.4s, v3.4h, v13.4h
	smlal	v17.4s, v8.4h, v5.4h
	ldr	q4, [sp, #720] // 16-byte Folded Reload
	smlal	v18.4s, v4.4h, v27.4h
	mov	v8.16b, v22.16b
	smlal	v17.4s, v14.4h, v8.4h
	ldr	q22, [sp, #912] // 16-byte Folded Reload
	smlal	v18.4s, v12.4h, v22.4h
	smlal2	v17.4s, v1.8h, v30.8h
	mov	v21.16b, v1.16b
	smlal2	v18.4s, v0.8h, v31.8h
	mov	v13.16b, v0.16b
	smlal2	v17.4s, v6.8h, v19.8h
	mov	v5.16b, v19.16b
	smlal2	v18.4s, v3.8h, v20.8h
	mov	v4.16b, v20.16b
	mov	v12.16b, v3.16b
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v2.8h
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	ldr	d6, [sp, #1240] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v6.4h
	ldr	q27, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v27.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	ldr	d26, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v26.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v1.4h, #171
	smull	v17.4s, v7.4h, v1.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v0.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v1.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v0.8h, v16.4s, v7.4s
	str	q0, [sp, #160] // 16-byte Folded Spill
	ldr	q0, [sp, #400] // 16-byte Folded Reload
	smull	v7.4s, v15.4h, v0.4h
	ldr	q2, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v8.4h
	ldr	q0, [sp, #240] // 16-byte Folded Reload
	smull	v16.4s, v29.4h, v0.4h
	ldr	q17, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v17.4h, v22.4h
	mov	v20.16b, v22.16b
	ldp	q11, q0, [sp, #432] // 32-byte Folded Reload
	ldr	q1, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v1.4h, v11.4h
	ldr	q15, [sp, #544] // 16-byte Folded Reload
	smlal	v16.4s, v24.4h, v15.4h
	ldr	q3, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v3.4h, v0.4h
	ldr	q24, [sp, #784] // 16-byte Folded Reload
	smlal	v16.4s, v24.4h, v25.4h
	ldr	q22, [sp, #1072] // 16-byte Folded Reload
	smlal2	v7.4s, v2.8h, v22.8h
	ldr	q2, [sp, #1168] // 16-byte Folded Reload
	smlal2	v16.4s, v17.8h, v2.8h
	smlal	v7.4s, v28.4h, v30.4h
	smlal	v16.4s, v23.4h, v31.4h
	ldr	q30, [sp, #384] // 16-byte Folded Reload
	smlal	v7.4s, v30.4h, v5.4h
	ldr	q5, [sp, #528] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v4.4h
	ldr	q0, [sp, #208] // 16-byte Folded Reload
	ldp	q28, q3, [sp, #944] // 32-byte Folded Reload
	smlal	v7.4s, v0.4h, v3.4h
	ldr	q0, [sp, #224] // 16-byte Folded Reload
	ldr	q3, [sp, #768] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v3.4h
	ldr	q29, [sp, #1024] // 16-byte Folded Reload
	smlal	v7.4s, v29.4h, v22.4h
	ldr	q0, [sp, #1136] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v2.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mul	v17.8h, v17.8h, v28.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	ldr	q2, [sp, #704] // 16-byte Folded Reload
	smull	v18.4s, v21.4h, v2.4h
	ldr	q2, [sp, #272] // 16-byte Folded Reload
	smlal	v18.4s, v9.4h, v2.4h
	ldr	q2, [sp, #752] // 16-byte Folded Reload
	smull	v19.4s, v13.4h, v2.4h
	ldr	q2, [sp, #480] // 16-byte Folded Reload
	ldr	q9, [sp, #928] // 16-byte Folded Reload
	smlal	v19.4s, v9.4h, v2.4h
	ldr	q4, [sp, #656] // 16-byte Folded Reload
	ldp	q2, q3, [sp, #592] // 32-byte Folded Reload
	smlal	v18.4s, v4.4h, v2.4h
	smlal	v19.4s, v12.4h, v3.4h
	mov	v25.16b, v8.16b
	smlal2	v18.4s, v29.8h, v8.8h
	mov	v23.16b, v20.16b
	smlal2	v19.4s, v0.8h, v20.8h
	ldr	q0, [sp, #1104] // 16-byte Folded Reload
	smlal	v18.4s, v14.4h, v0.4h
	ldr	q0, [sp, #992] // 16-byte Folded Reload
	ldr	q2, [sp, #368] // 16-byte Folded Reload
	smlal	v19.4s, v2.4h, v0.4h
	ldr	q0, [sp, #336] // 16-byte Folded Reload
	ldp	q14, q2, [sp, #848] // 32-byte Folded Reload
	smlal	v18.4s, v0.4h, v2.4h
	ldp	q13, q3, [sp, #496] // 32-byte Folded Reload
	ldr	q2, [sp, #672] // 16-byte Folded Reload
	smlal	v19.4s, v3.4h, v2.4h
	ldr	q2, [sp, #640] // 16-byte Folded Reload
	smlal	v18.4s, v2.4h, v14.4h
	ldr	q31, [sp, #816] // 16-byte Folded Reload
	smlal	v19.4s, v13.4h, v31.4h
	uzp1	v20.8h, v18.8h, v19.8h
	mul	v20.8h, v20.8h, v28.8h
	mov	v8.16b, v28.16b
	smull	v21.4s, v20.4h, v10.4h
	subhn	v18.4h, v18.4s, v21.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	smull2	v17.4s, v20.8h, v10.8h
	mov	v20.16b, v18.16b
	subhn2	v20.8h, v19.4s, v17.4s
	subhn	v17.4h, v19.4s, v17.4s
	smull	v17.4s, v17.4h, v6.4h
	mul	v19.8h, v20.8h, v27.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v17.4h, v17.4s, v20.4s
	smull	v18.4s, v18.4h, v26.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v18.4h, v18.4s, v19.4s
	add	v7.4h, v18.4h, v7.4h
	add	v16.4h, v17.4h, v16.4h
	movi	v2.4h, #171
	smull	v17.4s, v7.4h, v2.4h
	mov	v7.d[1], v16.d[0]
	mvni	v6.8h, #84
	mul	v7.8h, v7.8h, v6.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v17.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v2.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v17.8h, v16.4s, v7.4s
	str	q17, [sp, #144] // 16-byte Folded Spill
	ldr	q17, [sp, #832] // 16-byte Folded Reload
	smull	v7.4s, v17.4h, v25.4h
	ldr	q6, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v6.4h, v14.4h
	ldr	q21, [sp, #800] // 16-byte Folded Reload
	mov	v20.16b, v23.16b
	smull	v16.4s, v21.4h, v20.4h
	ldr	q19, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v19.4h, v31.4h
	ldr	q2, [sp, #400] // 16-byte Folded Reload
	smlal	v7.4s, v1.4h, v2.4h
	ldr	q12, [sp, #240] // 16-byte Folded Reload
	ldr	q1, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v12.4h
	ldr	q2, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v11.4h
	smlal	v16.4s, v24.4h, v15.4h
	ldp	q18, q1, [sp, #960] // 32-byte Folded Reload
	smlal2	v7.4s, v6.8h, v18.8h
	ldr	q15, [sp, #768] // 16-byte Folded Reload
	smlal2	v16.4s, v19.8h, v15.8h
	smlal2	v7.4s, v17.8h, v22.8h
	mov	v23.16b, v17.16b
	ldr	q6, [sp, #1168] // 16-byte Folded Reload
	smlal2	v16.4s, v21.8h, v6.8h
	smlal	v7.4s, v30.4h, v1.4h
	ldp	q1, q28, [sp, #880] // 32-byte Folded Reload
	smlal	v16.4s, v5.4h, v1.4h
	ldp	q5, q27, [sp, #208] // 32-byte Folded Reload
	ldr	q24, [sp, #1088] // 16-byte Folded Reload
	smlal	v7.4s, v5.4h, v24.4h
	smlal	v16.4s, v27.4h, v28.4h
	mov	v26.16b, v29.16b
	smlal	v7.4s, v26.4h, v18.4h
	ldr	q19, [sp, #1136] // 16-byte Folded Reload
	smlal	v16.4s, v19.4h, v15.4h
	ldr	q29, [sp, #1040] // 16-byte Folded Reload
	smlal	v7.4s, v29.4h, v22.4h
	smlal	v16.4s, v9.4h, v6.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mov	v1.16b, v8.16b
	mul	v17.8h, v17.8h, v8.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	ldp	q11, q6, [sp, #688] // 32-byte Folded Reload
	smull	v17.4s, v4.4h, v6.4h
	ldr	q6, [sp, #272] // 16-byte Folded Reload
	ldr	q4, [sp, #1056] // 16-byte Folded Reload
	smlal	v17.4s, v4.4h, v6.4h
	ldr	q4, [sp, #1120] // 16-byte Folded Reload
	ldr	q18, [sp, #752] // 16-byte Folded Reload
	smull	v18.4s, v4.4h, v18.4h
	ldr	q8, [sp, #480] // 16-byte Folded Reload
	ldr	q4, [sp, #1008] // 16-byte Folded Reload
	smlal	v18.4s, v4.4h, v8.4h
	mov	v22.16b, v14.16b
	smlal2	v17.4s, v26.8h, v14.8h
	smlal2	v18.4s, v19.8h, v31.8h
	smlal2	v17.4s, v29.8h, v11.8h
	smlal2	v18.4s, v9.8h, v20.8h
	mov	v9.16b, v20.16b
	ldr	q4, [sp, #1104] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v4.4h
	ldr	q0, [sp, #992] // 16-byte Folded Reload
	smlal	v18.4s, v3.4h, v0.4h
	ldr	q30, [sp, #640] // 16-byte Folded Reload
	ldr	q14, [sp, #864] // 16-byte Folded Reload
	smlal	v17.4s, v30.4h, v14.4h
	ldr	q25, [sp, #672] // 16-byte Folded Reload
	smlal	v18.4s, v13.4h, v25.4h
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v1.8h
	mov	v13.16b, v1.16b
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	ldr	d1, [sp, #1240] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v1.4h
	ldr	q0, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v0.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	ldr	d29, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v29.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v3.4h, #171
	smull	v17.4s, v7.4h, v3.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v0.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v3.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v0.8h, v16.4s, v7.4s
	str	q0, [sp, #128] // 16-byte Folded Spill
	mov	v0.16b, v23.16b
	smull	v7.4s, v0.4h, v22.4h
	ldr	q23, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v23.4h, v14.4h
	mov	v22.16b, v21.16b
	smull	v16.4s, v22.4h, v31.4h
	ldr	q19, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v19.4h, v25.4h
	mov	v3.16b, v25.16b
	ldr	q17, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v17.4h, v11.4h
	ldr	q21, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v21.4h, v9.4h
	mov	v4.16b, v9.16b
	ldr	q18, [sp, #400] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v18.4h
	ldr	q2, [sp, #784] // 16-byte Folded Reload
	smlal	v16.4s, v2.4h, v12.4h
	mov	v20.16b, v24.16b
	smlal2	v7.4s, v23.8h, v24.8h
	mov	v24.16b, v28.16b
	smlal2	v16.4s, v19.8h, v28.8h
	ldp	q25, q23, [sp, #960] // 32-byte Folded Reload
	smlal2	v7.4s, v0.8h, v25.8h
	mov	v12.16b, v0.16b
	mov	v28.16b, v15.16b
	smlal2	v16.4s, v22.8h, v15.8h
	ldr	q18, [sp, #1072] // 16-byte Folded Reload
	smlal2	v7.4s, v17.8h, v18.8h
	ldr	q0, [sp, #1168] // 16-byte Folded Reload
	smlal2	v16.4s, v21.8h, v0.8h
	smlal	v7.4s, v5.4h, v23.4h
	ldr	q26, [sp, #880] // 16-byte Folded Reload
	smlal	v16.4s, v27.4h, v26.4h
	ldp	q22, q5, [sp, #1008] // 32-byte Folded Reload
	smlal	v7.4s, v5.4h, v20.4h
	ldr	q31, [sp, #1136] // 16-byte Folded Reload
	smlal	v16.4s, v31.4h, v24.4h
	ldr	q19, [sp, #1040] // 16-byte Folded Reload
	smlal	v7.4s, v19.4h, v25.4h
	ldr	q20, [sp, #928] // 16-byte Folded Reload
	smlal	v16.4s, v20.4h, v28.4h
	ldr	q24, [sp, #1056] // 16-byte Folded Reload
	smlal	v7.4s, v24.4h, v18.4h
	smlal	v16.4s, v22.4h, v0.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mov	v0.16b, v13.16b
	mul	v17.8h, v17.8h, v13.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	ldr	q25, [sp, #656] // 16-byte Folded Reload
	smull	v17.4s, v25.4h, v6.4h
	mov	v15.16b, v14.16b
	smlal2	v17.4s, v5.8h, v14.8h
	ldr	q21, [sp, #1120] // 16-byte Folded Reload
	smull	v18.4s, v21.4h, v8.4h
	mov	v28.16b, v3.16b
	smlal2	v18.4s, v31.8h, v3.8h
	ldr	q13, [sp, #848] // 16-byte Folded Reload
	smlal2	v17.4s, v19.8h, v13.8h
	mov	v8.16b, v19.16b
	ldr	q9, [sp, #816] // 16-byte Folded Reload
	smlal2	v18.4s, v20.8h, v9.8h
	mov	v14.16b, v20.16b
	smlal2	v17.4s, v24.8h, v11.8h
	mov	v27.16b, v4.16b
	smlal2	v18.4s, v22.8h, v4.8h
	ldr	q24, [sp, #1104] // 16-byte Folded Reload
	smlal	v17.4s, v30.4h, v24.4h
	ldr	q3, [sp, #992] // 16-byte Folded Reload
	ldr	q4, [sp, #496] // 16-byte Folded Reload
	smlal	v18.4s, v4.4h, v3.4h
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v0.8h
	mov	v30.16b, v0.16b
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	smull	v18.4s, v18.4h, v1.4h
	ldr	q4, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v4.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	smull	v17.4s, v17.4h, v29.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v1.4h, #171
	smull	v17.4s, v7.4h, v1.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v0.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v1.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v0.8h, v16.4s, v7.4s
	str	q0, [sp, #112] // 16-byte Folded Spill
	smull	v7.4s, v12.4h, v15.4h
	ldr	q18, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v18.4h, v24.4h
	ldr	q29, [sp, #800] // 16-byte Folded Reload
	smull	v16.4s, v29.4h, v28.4h
	ldr	q6, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v6.4h, v3.4h
	ldr	q17, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v17.4h, v13.4h
	ldr	q20, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v20.4h, v9.4h
	ldr	q0, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v11.4h
	mov	v22.16b, v11.16b
	mov	v1.16b, v2.16b
	smlal	v16.4s, v1.4h, v27.4h
	mov	v2.16b, v27.16b
	mov	v27.16b, v23.16b
	smlal2	v7.4s, v18.8h, v23.8h
	smlal2	v16.4s, v6.8h, v26.8h
	ldr	q18, [sp, #1088] // 16-byte Folded Reload
	smlal2	v7.4s, v12.8h, v18.8h
	ldr	q19, [sp, #896] // 16-byte Folded Reload
	smlal2	v16.4s, v29.8h, v19.8h
	ldr	q29, [sp, #960] // 16-byte Folded Reload
	smlal2	v7.4s, v17.8h, v29.8h
	ldr	q11, [sp, #768] // 16-byte Folded Reload
	smlal2	v16.4s, v20.8h, v11.8h
	ldr	q17, [sp, #1072] // 16-byte Folded Reload
	smlal2	v7.4s, v0.8h, v17.8h
	ldr	q23, [sp, #1168] // 16-byte Folded Reload
	smlal2	v16.4s, v1.8h, v23.8h
	mov	v0.16b, v5.16b
	smlal	v7.4s, v0.4h, v27.4h
	mov	v27.16b, v31.16b
	smlal	v16.4s, v27.4h, v26.4h
	smlal	v7.4s, v8.4h, v18.4h
	smlal	v16.4s, v14.4h, v19.4h
	mov	v26.16b, v19.16b
	ldr	q19, [sp, #1056] // 16-byte Folded Reload
	smlal	v7.4s, v19.4h, v29.4h
	ldr	q5, [sp, #1008] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v11.4h
	mov	v12.16b, v25.16b
	smlal	v7.4s, v12.4h, v17.4h
	mov	v20.16b, v21.16b
	smlal	v16.4s, v20.4h, v23.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mov	v11.16b, v30.16b
	mul	v17.8h, v17.8h, v30.8h
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	smull2	v17.4s, v8.8h, v15.8h
	mov	v23.16b, v8.16b
	mov	v21.16b, v24.16b
	smlal2	v17.4s, v0.8h, v24.8h
	smull2	v18.4s, v14.8h, v28.8h
	mov	v29.16b, v28.16b
	mov	v28.16b, v14.16b
	smlal2	v18.4s, v31.8h, v3.8h
	mov	v8.16b, v3.16b
	smlal2	v17.4s, v19.8h, v13.8h
	mov	v14.16b, v13.16b
	mov	v13.16b, v19.16b
	smlal2	v18.4s, v5.8h, v9.8h
	mov	v25.16b, v22.16b
	smlal2	v17.4s, v12.8h, v22.8h
	mov	v3.16b, v12.16b
	smlal2	v18.4s, v20.8h, v2.8h
	mov	v24.16b, v2.16b
	mov	v30.16b, v20.16b
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v11.8h
	mov	v22.16b, v11.16b
	smull	v20.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	ldr	d0, [sp, #1240] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v0.4h
	mul	v19.8h, v20.8h, v4.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	ldr	d0, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v0.4h
	smull	v19.4s, v19.4h, v10.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v1.4h, #171
	smull	v17.4s, v7.4h, v1.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v10.4h
	subhn	v0.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v1.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v0.8h, v16.4s, v7.4s
	str	q0, [sp, #96] // 16-byte Folded Spill
	smull2	v7.4s, v13.8h, v15.8h
	mov	v4.16b, v21.16b
	mov	v11.16b, v23.16b
	smlal2	v7.4s, v23.8h, v21.8h
	mov	v0.16b, v29.16b
	mov	v12.16b, v5.16b
	smull2	v16.4s, v5.8h, v29.8h
	mov	v5.16b, v8.16b
	mov	v20.16b, v28.16b
	smlal2	v16.4s, v28.8h, v8.8h
	mov	v1.16b, v14.16b
	mov	v23.16b, v3.16b
	smlal2	v7.4s, v3.8h, v14.8h
	mov	v21.16b, v30.16b
	smlal2	v16.4s, v30.8h, v9.8h
	uzp1	v17.8h, v7.8h, v16.8h
	mul	v17.8h, v17.8h, v22.8h
	smull	v18.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v18.4s
	ldr	q3, [sp, #832] // 16-byte Folded Reload
	smull	v18.4s, v3.4h, v4.4h
	ldr	q6, [sp, #1280] // 16-byte Folded Reload
	ldr	q4, [sp, #560] // 16-byte Folded Reload
	smlal	v18.4s, v6.4h, v4.4h
	ldp	q8, q30, [sp, #784] // 32-byte Folded Reload
	smull	v19.4s, v30.4h, v5.4h
	ldr	q5, [sp, #1248] // 16-byte Folded Reload
	ldr	q4, [sp, #576] // 16-byte Folded Reload
	smlal	v19.4s, v5.4h, v4.4h
	ldr	q4, [sp, #1200] // 16-byte Folded Reload
	smlal	v18.4s, v4.4h, v15.4h
	ldr	q2, [sp, #1264] // 16-byte Folded Reload
	smlal	v19.4s, v2.4h, v0.4h
	ldr	q0, [sp, #1216] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v1.4h
	smlal	v19.4s, v8.4h, v9.4h
	ldr	q29, [sp, #736] // 16-byte Folded Reload
	smlal	v18.4s, v29.4h, v25.4h
	ldp	q14, q22, [sp, #288] // 32-byte Folded Reload
	smlal	v19.4s, v14.4h, v24.4h
	ldp	q24, q27, [sp, #960] // 32-byte Folded Reload
	smlal2	v18.4s, v3.8h, v27.8h
	ldr	q1, [sp, #880] // 16-byte Folded Reload
	smlal2	v19.4s, v30.8h, v1.8h
	ldr	q25, [sp, #1088] // 16-byte Folded Reload
	smlal2	v18.4s, v4.8h, v25.8h
	smlal2	v19.4s, v2.8h, v26.8h
	smlal2	v18.4s, v0.8h, v24.8h
	ldr	q15, [sp, #768] // 16-byte Folded Reload
	smlal2	v19.4s, v8.8h, v15.8h
	mov	v28.16b, v8.16b
	ldr	q9, [sp, #320] // 16-byte Folded Reload
	ldr	q31, [sp, #1024] // 16-byte Folded Reload
	smlal	v18.4s, v31.4h, v9.4h
	ldr	q8, [sp, #1136] // 16-byte Folded Reload
	smlal	v19.4s, v8.4h, v22.4h
	smlal	v18.4s, v11.4h, v27.4h
	mov	v0.16b, v27.16b
	smlal	v19.4s, v20.4h, v1.4h
	smlal	v18.4s, v13.4h, v25.4h
	mov	v2.16b, v25.16b
	smlal	v19.4s, v12.4h, v26.4h
	mov	v13.16b, v26.16b
	smlal	v18.4s, v23.4h, v24.4h
	mov	v26.16b, v24.16b
	smlal	v19.4s, v21.4h, v15.4h
	ldr	q23, [sp, #256] // 16-byte Folded Reload
	ldr	q25, [sp, #1072] // 16-byte Folded Reload
	smlal	v18.4s, v23.4h, v25.4h
	ldr	q11, [sp, #720] // 16-byte Folded Reload
	ldr	q24, [sp, #1168] // 16-byte Folded Reload
	smlal	v19.4s, v11.4h, v24.4h
	uzp1	v20.8h, v18.8h, v19.8h
	ldr	q27, [sp, #944] // 16-byte Folded Reload
	mul	v20.8h, v20.8h, v27.8h
	smull2	v21.4s, v20.8h, v10.8h
	subhn	v19.4h, v19.4s, v21.4s
	smull2	v17.4s, v17.8h, v10.8h
	mov	v21.16b, v7.16b
	subhn2	v21.8h, v16.4s, v17.4s
	subhn	v16.4h, v16.4s, v17.4s
	smull	v17.4s, v20.4h, v10.4h
	subhn	v17.4h, v18.4s, v17.4s
	ldr	d3, [sp, #1240] // 8-byte Folded Reload
	smull	v16.4s, v16.4h, v3.4h
	ldr	q3, [sp, #1184] // 16-byte Folded Reload
	mul	v18.8h, v21.8h, v3.8h
	smull2	v20.4s, v18.8h, v10.8h
	subhn	v16.4h, v16.4s, v20.4s
	ldr	d3, [sp, #1160] // 8-byte Folded Reload
	smull	v7.4s, v7.4h, v3.4h
	smull	v18.4s, v18.4h, v10.4h
	subhn	v7.4h, v7.4s, v18.4s
	add	v7.4h, v7.4h, v17.4h
	add	v16.4h, v16.4h, v19.4h
	mov	v17.16b, v7.16b
	mov	v17.d[1], v16.d[0]
	movi	v3.4h, #171
	smull	v7.4s, v7.4h, v3.4h
	mvni	v4.8h, #84
	mul	v17.8h, v17.8h, v4.8h
	smull	v18.4s, v17.4h, v10.4h
	subhn	v4.4h, v7.4s, v18.4s
	smull	v7.4s, v16.4h, v3.4h
	smull2	v16.4s, v17.8h, v10.8h
	subhn2	v4.8h, v7.4s, v16.4s
	str	q4, [sp, #80] // 16-byte Folded Spill
	ldr	q12, [sp, #832] // 16-byte Folded Reload
	smull	v7.4s, v12.4h, v0.4h
	smlal	v7.4s, v6.4h, v9.4h
	smull	v16.4s, v30.4h, v1.4h
	mov	v21.16b, v5.16b
	smlal	v16.4s, v21.4h, v22.4h
	ldr	q4, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v4.4h, v2.4h
	ldr	q1, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v13.4h
	ldr	q0, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v26.4h
	mov	v3.16b, v26.16b
	mov	v6.16b, v15.16b
	smlal	v16.4s, v28.4h, v6.4h
	smlal	v7.4s, v29.4h, v25.4h
	mov	v15.16b, v14.16b
	smlal	v16.4s, v15.4h, v24.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mul	v17.8h, v17.8h, v27.8h
	mov	v9.16b, v27.16b
	smull2	v18.4s, v17.8h, v10.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v10.4h
	subhn	v7.4h, v7.4s, v17.4s
	ldp	q19, q20, [sp, #848] // 32-byte Folded Reload
	smull2	v17.4s, v4.8h, v20.8h
	ldr	q4, [sp, #1104] // 16-byte Folded Reload
	smlal2	v17.4s, v12.8h, v4.8h
	ldr	q22, [sp, #672] // 16-byte Folded Reload
	smull2	v18.4s, v1.8h, v22.8h
	ldp	q1, q25, [sp, #992] // 32-byte Folded Reload
	smlal2	v18.4s, v30.8h, v1.8h
	smlal2	v17.4s, v0.8h, v19.8h
	ldr	q29, [sp, #816] // 16-byte Folded Reload
	smlal2	v18.4s, v28.8h, v29.8h
	ldp	q5, q2, [sp, #560] // 32-byte Folded Reload
	smlal	v17.4s, v31.4h, v5.4h
	smlal	v18.4s, v8.4h, v2.4h
	ldr	q24, [sp, #1040] // 16-byte Folded Reload
	smlal	v17.4s, v24.4h, v4.4h
	ldp	q14, q27, [sp, #912] // 32-byte Folded Reload
	smlal	v18.4s, v27.4h, v1.4h
	ldr	q1, [sp, #1056] // 16-byte Folded Reload
	smlal	v17.4s, v1.4h, v20.4h
	smlal	v18.4s, v25.4h, v22.4h
	ldr	q0, [sp, #656] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v19.4h
	mov	v4.16b, v19.16b
	ldr	q19, [sp, #1120] // 16-byte Folded Reload
	smlal	v18.4s, v19.4h, v29.4h
	mov	v22.16b, v29.16b
	ldr	q29, [sp, #688] // 16-byte Folded Reload
	smlal	v17.4s, v23.4h, v29.4h
	smlal	v18.4s, v11.4h, v14.4h
	ldr	q26, [sp, #976] // 16-byte Folded Reload
	smlal2	v17.4s, v24.8h, v26.8h
	ldr	q23, [sp, #880] // 16-byte Folded Reload
	smlal2	v18.4s, v27.8h, v23.8h
	mov	v11.16b, v27.16b
	ldr	q8, [sp, #1088] // 16-byte Folded Reload
	smlal2	v17.4s, v1.8h, v8.8h
	mov	v1.16b, v13.16b
	smlal2	v18.4s, v25.8h, v13.8h
	smlal2	v17.4s, v0.8h, v3.8h
	mov	v31.16b, v3.16b
	mov	v13.16b, v0.16b
	smlal2	v18.4s, v19.8h, v6.8h
	mov	v28.16b, v6.16b
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v9.8h
	mov	v9.16b, v10.16b
	smull	v20.4s, v19.4h, v9.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v10.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	ldr	d0, [sp, #1240] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v0.4h
	ldr	q0, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v0.8h
	smull2	v20.4s, v19.8h, v10.8h
	subhn	v18.4h, v18.4s, v20.4s
	ldr	d0, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v0.4h
	smull	v19.4s, v19.4h, v9.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v3.4h, #171
	smull	v17.4s, v7.4h, v3.4h
	mov	v7.d[1], v16.d[0]
	mvni	v0.8h, #84
	mul	v7.8h, v7.8h, v0.8h
	smull	v18.4s, v7.4h, v9.4h
	subhn	v6.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v3.4h
	smull2	v7.4s, v7.8h, v10.8h
	subhn2	v6.8h, v16.4s, v7.4s
	str	q6, [sp, #64] // 16-byte Folded Spill
	smull	v7.4s, v12.4h, v5.4h
	ldr	q12, [sp, #592] // 16-byte Folded Reload
	ldr	q0, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v12.4h
	smull	v16.4s, v30.4h, v2.4h
	ldp	q30, q10, [sp, #608] // 32-byte Folded Reload
	smlal	v16.4s, v21.4h, v30.4h
	ldr	q0, [sp, #1200] // 16-byte Folded Reload
	ldr	q6, [sp, #1104] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v6.4h
	ldr	q2, [sp, #1264] // 16-byte Folded Reload
	ldr	q27, [sp, #992] // 16-byte Folded Reload
	smlal	v16.4s, v2.4h, v27.4h
	ldr	q19, [sp, #864] // 16-byte Folded Reload
	ldr	q17, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v17.4h, v19.4h
	ldr	q20, [sp, #672] // 16-byte Folded Reload
	ldr	q3, [sp, #784] // 16-byte Folded Reload
	smlal	v16.4s, v3.4h, v20.4h
	ldr	q5, [sp, #736] // 16-byte Folded Reload
	smlal	v7.4s, v5.4h, v4.4h
	smlal	v16.4s, v15.4h, v22.4h
	smlal	v7.4s, v10.4h, v29.4h
	ldr	q22, [sp, #416] // 16-byte Folded Reload
	smlal	v16.4s, v22.4h, v14.4h
	smlal2	v7.4s, v0.8h, v26.8h
	smlal2	v16.4s, v2.8h, v23.8h
	mov	v21.16b, v8.16b
	smlal2	v7.4s, v17.8h, v8.8h
	smlal2	v16.4s, v3.8h, v1.8h
	ldp	q3, q2, [sp, #448] // 32-byte Folded Reload
	ldr	q14, [sp, #1024] // 16-byte Folded Reload
	smlal	v7.4s, v14.4h, v3.4h
	ldr	q29, [sp, #1136] // 16-byte Folded Reload
	smlal	v16.4s, v29.4h, v2.4h
	ldp	q4, q0, [sp, #304] // 32-byte Folded Reload
	smlal	v7.4s, v24.4h, v0.4h
	smlal	v16.4s, v11.4h, v4.4h
	ldr	q24, [sp, #1056] // 16-byte Folded Reload
	smlal	v7.4s, v24.4h, v26.4h
	smlal	v16.4s, v25.4h, v23.4h
	smlal	v7.4s, v13.4h, v21.4h
	ldr	q21, [sp, #1120] // 16-byte Folded Reload
	smlal	v16.4s, v21.4h, v1.4h
	ldr	q0, [sp, #256] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v31.4h
	ldr	q0, [sp, #720] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v28.4h
	ldr	q5, [sp, #1072] // 16-byte Folded Reload
	ldr	q0, [sp, #352] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v5.4h
	ldr	q5, [sp, #368] // 16-byte Folded Reload
	ldr	q17, [sp, #1168] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v17.4h
	uzp1	v17.8h, v7.8h, v16.8h
	ldr	q1, [sp, #944] // 16-byte Folded Reload
	mul	v17.8h, v17.8h, v1.8h
	smull2	v18.4s, v17.8h, v9.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull2	v18.4s, v13.8h, v19.8h
	mov	v0.16b, v19.16b
	smlal2	v18.4s, v24.8h, v6.8h
	mov	v23.16b, v24.16b
	smull2	v19.4s, v21.8h, v20.8h
	mov	v8.16b, v20.16b
	smlal2	v19.4s, v25.8h, v27.8h
	uzp1	v20.8h, v18.8h, v19.8h
	mul	v20.8h, v20.8h, v1.8h
	smull	v21.4s, v20.4h, v9.4h
	subhn	v18.4h, v18.4s, v21.4s
	smull	v17.4s, v17.4h, v9.4h
	subhn	v7.4h, v7.4s, v17.4s
	smull2	v17.4s, v20.8h, v9.8h
	mov	v20.16b, v18.16b
	subhn2	v20.8h, v19.4s, v17.4s
	subhn	v17.4h, v19.4s, v17.4s
	ldr	d15, [sp, #1240] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v15.4h
	ldr	q1, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v1.8h
	smull2	v20.4s, v19.8h, v9.8h
	subhn	v17.4h, v17.4s, v20.4s
	ldr	d24, [sp, #1160] // 8-byte Folded Reload
	smull	v18.4s, v18.4h, v24.4h
	smull	v19.4s, v19.4h, v9.4h
	subhn	v18.4h, v18.4s, v19.4s
	add	v7.4h, v18.4h, v7.4h
	add	v16.4h, v17.4h, v16.4h
	movi	v1.4h, #171
	smull	v17.4s, v7.4h, v1.4h
	mov	v7.d[1], v16.d[0]
	mvni	v18.8h, #84
	mul	v7.8h, v7.8h, v18.8h
	smull	v18.4s, v7.4h, v9.4h
	subhn	v17.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v1.4h
	smull2	v7.4s, v7.8h, v9.8h
	subhn2	v17.8h, v16.4s, v7.4s
	str	q17, [sp, #48] // 16-byte Folded Spill
	ldr	q1, [sp, #832] // 16-byte Folded Reload
	smull	v7.4s, v1.4h, v12.4h
	ldr	q1, [sp, #704] // 16-byte Folded Reload
	ldr	q16, [sp, #1280] // 16-byte Folded Reload
	smlal	v7.4s, v16.4h, v1.4h
	ldp	q18, q12, [sp, #784] // 32-byte Folded Reload
	smull	v16.4s, v12.4h, v30.4h
	ldr	q17, [sp, #752] // 16-byte Folded Reload
	ldr	q1, [sp, #1248] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v17.4h
	ldp	q19, q17, [sp, #560] // 32-byte Folded Reload
	ldr	q1, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v1.4h, v19.4h
	ldr	q31, [sp, #1264] // 16-byte Folded Reload
	smlal	v16.4s, v31.4h, v17.4h
	ldr	q28, [sp, #1216] // 16-byte Folded Reload
	mov	v19.16b, v6.16b
	smlal	v7.4s, v28.4h, v19.4h
	mov	v20.16b, v27.16b
	smlal	v16.4s, v18.4h, v20.4h
	ldr	q1, [sp, #736] // 16-byte Folded Reload
	smlal	v7.4s, v1.4h, v0.4h
	ldr	q1, [sp, #288] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v8.4h
	ldr	q1, [sp, #848] // 16-byte Folded Reload
	smlal	v7.4s, v10.4h, v1.4h
	ldr	q1, [sp, #816] // 16-byte Folded Reload
	smlal	v16.4s, v22.4h, v1.4h
	ldr	q27, [sp, #384] // 16-byte Folded Reload
	ldr	q1, [sp, #688] // 16-byte Folded Reload
	smlal	v7.4s, v27.4h, v1.4h
	ldr	q6, [sp, #528] // 16-byte Folded Reload
	ldr	q1, [sp, #912] // 16-byte Folded Reload
	smlal	v16.4s, v6.4h, v1.4h
	ldr	q17, [sp, #976] // 16-byte Folded Reload
	smlal2	v7.4s, v28.8h, v17.8h
	ldr	q8, [sp, #880] // 16-byte Folded Reload
	smlal2	v16.4s, v18.8h, v8.8h
	mov	v26.16b, v18.16b
	ldr	q1, [sp, #432] // 16-byte Folded Reload
	smlal	v7.4s, v14.4h, v1.4h
	ldr	q6, [sp, #544] // 16-byte Folded Reload
	smlal	v16.4s, v29.4h, v6.4h
	ldr	q1, [sp, #1040] // 16-byte Folded Reload
	smlal	v7.4s, v1.4h, v3.4h
	ldr	q1, [sp, #928] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v2.4h
	ldp	q21, q6, [sp, #320] // 32-byte Folded Reload
	smlal	v7.4s, v23.4h, v21.4h
	smlal	v16.4s, v25.4h, v4.4h
	smlal	v7.4s, v13.4h, v17.4h
	mov	v3.16b, v17.16b
	ldr	q1, [sp, #1120] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v8.4h
	mov	v11.16b, v8.16b
	ldr	q23, [sp, #256] // 16-byte Folded Reload
	ldr	q22, [sp, #1088] // 16-byte Folded Reload
	smlal	v7.4s, v23.4h, v22.4h
	ldr	q2, [sp, #896] // 16-byte Folded Reload
	ldr	q0, [sp, #720] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v2.4h
	ldr	q30, [sp, #352] // 16-byte Folded Reload
	ldr	q0, [sp, #960] // 16-byte Folded Reload
	smlal	v7.4s, v30.4h, v0.4h
	ldr	q14, [sp, #768] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v14.4h
	ldr	q8, [sp, #1072] // 16-byte Folded Reload
	smlal	v7.4s, v6.4h, v8.4h
	ldr	q5, [sp, #512] // 16-byte Folded Reload
	ldr	q10, [sp, #1168] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v10.4h
	uzp1	v17.8h, v7.8h, v16.8h
	ldr	q5, [sp, #944] // 16-byte Folded Reload
	mul	v17.8h, v17.8h, v5.8h
	smull2	v18.4s, v17.8h, v9.8h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v17.4s, v17.4h, v9.4h
	subhn	v7.4h, v7.4s, v17.4s
	smull2	v17.4s, v13.8h, v19.8h
	smull2	v18.4s, v1.8h, v20.8h
	uzp1	v19.8h, v17.8h, v18.8h
	mul	v19.8h, v19.8h, v5.8h
	smull	v20.4s, v19.4h, v9.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v9.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v18.4s, v19.4s
	subhn	v18.4h, v18.4s, v19.4s
	smull	v18.4s, v18.4h, v15.4h
	ldr	q1, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v1.8h
	smull2	v20.4s, v19.8h, v9.8h
	subhn	v18.4h, v18.4s, v20.4s
	smull	v17.4s, v17.4h, v24.4h
	smull	v19.4s, v19.4h, v9.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v7.4h, v17.4h, v7.4h
	add	v16.4h, v18.4h, v16.4h
	movi	v4.4h, #171
	smull	v17.4s, v7.4h, v4.4h
	mov	v7.d[1], v16.d[0]
	mvni	v1.8h, #84
	mul	v7.8h, v7.8h, v1.8h
	smull	v18.4s, v7.4h, v9.4h
	subhn	v1.4h, v17.4s, v18.4s
	smull	v16.4s, v16.4h, v4.4h
	smull2	v7.4s, v7.8h, v9.8h
	subhn2	v1.8h, v16.4s, v7.4s
	str	q1, [sp, #32] // 16-byte Folded Spill
	ldr	q4, [sp, #832] // 16-byte Folded Reload
	smull	v7.4s, v4.4h, v22.4h
	ldr	q29, [sp, #1280] // 16-byte Folded Reload
	mov	v20.16b, v3.16b
	smlal	v7.4s, v29.4h, v20.4h
	smull	v16.4s, v12.4h, v2.4h
	ldr	q25, [sp, #1248] // 16-byte Folded Reload
	mov	v15.16b, v11.16b
	smlal	v16.4s, v25.4h, v15.4h
	mov	v11.16b, v0.16b
	ldr	q0, [sp, #1200] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v11.4h
	smlal	v16.4s, v31.4h, v14.4h
	smlal	v7.4s, v28.4h, v8.4h
	mov	v19.16b, v26.16b
	smlal	v16.4s, v19.4h, v10.4h
	uzp1	v17.8h, v7.8h, v16.8h
	mul	v17.8h, v17.8h, v5.8h
	smull2	v18.4s, v17.8h, v9.8h
	subhn	v1.4h, v16.4s, v18.4s
	smull	v16.4s, v17.4h, v9.4h
	subhn	v2.4h, v7.4s, v16.4s
	stp	d2, d1, [sp, #16] // 16-byte Folded Spill
	smull	v10.4s, v6.4h, v11.4h
	ldr	q5, [sp, #640] // 16-byte Folded Reload
	smlal	v10.4s, v5.4h, v8.4h
	mov	v26.16b, v8.16b
	smlal	v10.4s, v30.4h, v22.4h
	mov	v7.16b, v22.16b
	smlal	v10.4s, v23.4h, v20.4h
	smlal	v10.4s, v13.4h, v21.4h
	ldp	q6, q5, [sp, #688] // 32-byte Folded Reload
	mov	v8.16b, v0.16b
	smull	v16.4s, v8.4h, v5.4h
	ldr	q12, [sp, #272] // 16-byte Folded Reload
	smlal	v16.4s, v4.4h, v12.4h
	ldr	q1, [sp, #592] // 16-byte Folded Reload
	smlal	v16.4s, v28.4h, v1.4h
	smlal2	v16.4s, v29.8h, v6.8h
	ldr	q0, [sp, #1104] // 16-byte Folded Reload
	ldr	q1, [sp, #624] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v0.4h
	mov	v18.16b, v27.16b
	ldp	q30, q22, [sp, #848] // 32-byte Folded Reload
	smlal	v16.4s, v18.4h, v22.4h
	ldr	q31, [sp, #208] // 16-byte Folded Reload
	smlal	v16.4s, v31.4h, v30.4h
	ldr	q23, [sp, #1024] // 16-byte Folded Reload
	smlal	v16.4s, v23.4h, v6.4h
	ldr	q2, [sp, #400] // 16-byte Folded Reload
	ldr	q27, [sp, #1040] // 16-byte Folded Reload
	smlal	v16.4s, v27.4h, v2.4h
	ldr	q24, [sp, #1056] // 16-byte Folded Reload
	ldp	q21, q3, [sp, #432] // 32-byte Folded Reload
	smlal	v16.4s, v24.4h, v21.4h
	smlal	v16.4s, v13.4h, v3.4h
	smlal	v10.4s, v24.4h, v3.4h
	smull	v17.4s, v28.4h, v5.4h
	mov	v5.16b, v12.16b
	smlal	v17.4s, v8.4h, v5.4h
	mov	v1.16b, v30.16b
	smlal2	v17.4s, v29.8h, v30.8h
	mov	v30.16b, v29.16b
	mov	v29.16b, v4.16b
	smlal2	v17.4s, v4.8h, v6.8h
	smlal	v17.4s, v18.4h, v0.4h
	mov	v12.16b, v22.16b
	mov	v22.16b, v31.16b
	smlal	v17.4s, v22.4h, v12.4h
	mov	v3.16b, v23.16b
	smlal	v17.4s, v3.4h, v1.4h
	mov	v18.16b, v27.16b
	smlal	v17.4s, v18.4h, v6.4h
	smlal	v17.4s, v24.4h, v2.4h
	mov	v4.16b, v2.16b
	mov	v2.16b, v21.16b
	smlal	v17.4s, v13.4h, v2.4h
	smlal	v10.4s, v18.4h, v2.4h
	mov	v23.16b, v27.16b
	smull	v18.4s, v28.4h, v5.4h
	mov	v5.16b, v30.16b
	smlal2	v18.4s, v30.8h, v12.8h
	smlal2	v18.4s, v29.8h, v1.8h
	mov	v30.16b, v8.16b
	smlal2	v18.4s, v8.8h, v6.8h
	smlal	v18.4s, v22.4h, v0.4h
	smlal	v18.4s, v3.4h, v12.4h
	smlal	v18.4s, v23.4h, v1.4h
	smlal	v18.4s, v24.4h, v6.4h
	mov	v2.16b, v4.16b
	smlal	v18.4s, v13.4h, v2.4h
	smlal	v10.4s, v3.4h, v2.4h
	smlal	v10.4s, v22.4h, v6.4h
	smull2	v21.4s, v29.8h, v12.8h
	smlal2	v21.4s, v5.8h, v0.8h
	mov	v4.16b, v5.16b
	smlal2	v21.4s, v8.8h, v1.8h
	smlal2	v21.4s, v28.8h, v6.8h
	mov	v2.16b, v6.16b
	smlal	v21.4s, v3.4h, v0.4h
	smlal	v21.4s, v23.4h, v12.4h
	smlal	v21.4s, v24.4h, v1.4h
	smlal	v21.4s, v13.4h, v2.4h
	smlal2	v21.4s, v3.8h, v20.8h
	mov	v8.16b, v3.16b
	smlal2	v21.4s, v27.8h, v7.8h
	mov	v7.16b, v27.16b
	smlal2	v21.4s, v24.8h, v11.8h
	smlal2	v21.4s, v13.8h, v26.8h
	ldr	q3, [sp, #672] // 16-byte Folded Reload
	ldp	q31, q0, [sp, #800] // 32-byte Folded Reload
	smull2	v2.4s, v31.8h, v3.8h
	ldr	q5, [sp, #992] // 16-byte Folded Reload
	smlal2	v2.4s, v25.8h, v5.8h
	ldr	q6, [sp, #1264] // 16-byte Folded Reload
	smlal2	v2.4s, v6.8h, v0.8h
	ldp	q20, q12, [sp, #912] // 32-byte Folded Reload
	smlal2	v2.4s, v19.8h, v20.8h
	ldr	q1, [sp, #1136] // 16-byte Folded Reload
	smlal	v2.4s, v1.4h, v5.4h
	smlal	v2.4s, v12.4h, v3.4h
	ldr	q5, [sp, #1008] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v0.4h
	ldr	q0, [sp, #1120] // 16-byte Folded Reload
	smlal	v2.4s, v0.4h, v20.4h
	smlal2	v2.4s, v1.8h, v15.8h
	ldr	q1, [sp, #896] // 16-byte Folded Reload
	smlal2	v2.4s, v12.8h, v1.8h
	smlal2	v2.4s, v5.8h, v14.8h
	ldr	q22, [sp, #1168] // 16-byte Folded Reload
	smlal2	v2.4s, v0.8h, v22.8h
	uzp1	v5.8h, v21.8h, v2.8h
	ldr	q27, [sp, #944] // 16-byte Folded Reload
	mul	v5.8h, v5.8h, v27.8h
	mov	v15.16b, v9.16b
	smull	v23.4s, v5.4h, v15.4h
	subhn	v21.4h, v21.4s, v23.4s
	smull2	v5.4s, v5.8h, v9.8h
	mov	v23.16b, v21.16b
	subhn2	v23.8h, v2.4s, v5.4s
	subhn	v2.4h, v2.4s, v5.4s
	ldr	d0, [sp, #1240] // 8-byte Folded Reload
	smull	v2.4s, v2.4h, v0.4h
	ldr	q9, [sp, #1184] // 16-byte Folded Reload
	mul	v5.8h, v23.8h, v9.8h
	smull2	v23.4s, v5.8h, v15.8h
	subhn	v2.4h, v2.4s, v23.4s
	ldr	d25, [sp, #1160] // 8-byte Folded Reload
	smull	v21.4s, v21.4h, v25.4h
	smull	v5.4s, v5.4h, v15.4h
	subhn	v5.4h, v21.4s, v5.4s
	ldr	d0, [sp, #16] // 8-byte Folded Reload
	add	v5.4h, v5.4h, v0.4h
	ldr	d0, [sp, #24] // 8-byte Folded Reload
	add	v19.4h, v2.4h, v0.4h
	movi	v0.4h, #171
	smull	v2.4s, v5.4h, v0.4h
	mov	v5.d[1], v19.d[0]
	mvni	v20.8h, #84
	mul	v5.8h, v5.8h, v20.8h
	smull	v20.4s, v5.4h, v15.4h
	subhn	v2.4h, v2.4s, v20.4s
	smull	v19.4s, v19.4h, v0.4h
	smull2	v5.4s, v5.8h, v15.8h
	mov	v21.16b, v15.16b
	subhn2	v2.8h, v19.4s, v5.4s
	str	q2, [sp, #688] // 16-byte Folded Spill
	ldr	q0, [sp, #384] // 16-byte Folded Reload
	ldp	q5, q2, [sp, #848] // 32-byte Folded Reload
	smlal	v10.4s, v0.4h, v5.4h
	ldr	q0, [sp, #624] // 16-byte Folded Reload
	smlal	v10.4s, v0.4h, v2.4h
	ldr	q0, [sp, #736] // 16-byte Folded Reload
	ldr	q2, [sp, #1104] // 16-byte Folded Reload
	smlal	v10.4s, v0.4h, v2.4h
	ldr	q0, [sp, #560] // 16-byte Folded Reload
	smlal	v10.4s, v28.4h, v0.4h
	mov	v20.16b, v11.16b
	smull	v0.4s, v29.4h, v20.4h
	ldr	q23, [sp, #1088] // 16-byte Folded Reload
	smlal	v0.4s, v4.4h, v23.4h
	mov	v2.16b, v31.16b
	smull	v4.4s, v2.4h, v14.4h
	ldr	q29, [sp, #1248] // 16-byte Folded Reload
	mov	v28.16b, v1.16b
	smlal	v4.4s, v29.4h, v28.4h
	smlal	v0.4s, v30.4h, v26.4h
	mov	v11.16b, v22.16b
	mov	v19.16b, v6.16b
	smlal	v4.4s, v19.4h, v11.4h
	uzp1	v5.8h, v0.8h, v4.8h
	mul	v5.8h, v5.8h, v27.8h
	smull2	v6.4s, v5.8h, v15.8h
	subhn	v4.4h, v4.4s, v6.4s
	smull	v5.4s, v5.4h, v21.4h
	subhn	v0.4h, v0.4s, v5.4s
	smlal2	v18.4s, v8.8h, v23.8h
	smlal2	v18.4s, v7.8h, v20.8h
	smlal2	v18.4s, v24.8h, v26.8h
	ldr	q26, [sp, #480] // 16-byte Folded Reload
	ldr	q15, [sp, #784] // 16-byte Folded Reload
	smull	v5.4s, v15.4h, v26.4h
	mov	v13.16b, v3.16b
	smlal2	v5.4s, v29.8h, v3.8h
	ldr	q29, [sp, #816] // 16-byte Folded Reload
	smlal2	v5.4s, v31.8h, v29.8h
	ldr	q6, [sp, #912] // 16-byte Folded Reload
	smlal2	v5.4s, v19.8h, v6.8h
	ldp	q31, q24, [sp, #224] // 32-byte Folded Reload
	ldp	q23, q2, [sp, #992] // 32-byte Folded Reload
	smlal	v5.4s, v31.4h, v23.4h
	ldr	q20, [sp, #1136] // 16-byte Folded Reload
	smlal	v5.4s, v20.4h, v13.4h
	smlal	v5.4s, v12.4h, v29.4h
	smlal	v5.4s, v2.4h, v6.4h
	ldr	q22, [sp, #1120] // 16-byte Folded Reload
	smlal	v5.4s, v22.4h, v24.4h
	smlal2	v5.4s, v20.8h, v1.8h
	mov	v7.16b, v1.16b
	mov	v28.16b, v20.16b
	smlal2	v5.4s, v12.8h, v14.8h
	mov	v20.16b, v14.16b
	smlal2	v5.4s, v2.8h, v11.8h
	ldr	q1, [sp, #976] // 16-byte Folded Reload
	ldr	q3, [sp, #640] // 16-byte Folded Reload
	smlal	v18.4s, v3.4h, v1.4h
	ldr	q8, [sp, #496] // 16-byte Folded Reload
	ldr	q1, [sp, #880] // 16-byte Folded Reload
	smlal	v5.4s, v8.4h, v1.4h
	uzp1	v6.8h, v18.8h, v5.8h
	mul	v6.8h, v6.8h, v27.8h
	smull	v19.4s, v6.4h, v21.4h
	subhn	v18.4h, v18.4s, v19.4s
	smull2	v6.4s, v6.8h, v21.8h
	mov	v19.16b, v18.16b
	subhn2	v19.8h, v5.4s, v6.4s
	subhn	v5.4h, v5.4s, v6.4s
	ldr	d3, [sp, #1240] // 8-byte Folded Reload
	smull	v5.4s, v5.4h, v3.4h
	mul	v6.8h, v19.8h, v9.8h
	smull2	v19.4s, v6.8h, v21.8h
	subhn	v5.4h, v5.4s, v19.4s
	smull	v18.4s, v18.4h, v25.4h
	smull	v6.4s, v6.4h, v21.4h
	subhn	v6.4h, v18.4s, v6.4s
	add	v0.4h, v6.4h, v0.4h
	add	v4.4h, v5.4h, v4.4h
	movi	v18.4h, #171
	smull	v5.4s, v0.4h, v18.4h
	mov	v0.d[1], v4.d[0]
	mvni	v3.8h, #84
	mul	v6.8h, v0.8h, v3.8h
	smull	v0.4s, v6.4h, v21.4h
	subhn	v0.4h, v5.4s, v0.4s
	smull	v4.4s, v4.4h, v18.4h
	smull2	v5.4s, v6.8h, v21.8h
	subhn2	v0.8h, v4.4s, v5.4s
	str	q0, [sp, #1216] // 16-byte Folded Spill
	ldr	q0, [sp, #592] // 16-byte Folded Reload
	smlal	v10.4s, v30.4h, v0.4h
	ldr	q14, [sp, #512] // 16-byte Folded Reload
	smull	v4.4s, v14.4h, v20.4h
	smlal	v4.4s, v8.4h, v11.4h
	ldr	q30, [sp, #368] // 16-byte Folded Reload
	smlal	v4.4s, v30.4h, v7.4h
	ldr	q0, [sp, #720] // 16-byte Folded Reload
	smlal	v4.4s, v0.4h, v1.4h
	mov	v8.16b, v1.16b
	ldr	q0, [sp, #304] // 16-byte Folded Reload
	mov	v7.16b, v22.16b
	smlal	v4.4s, v7.4h, v0.4h
	ldr	q25, [sp, #752] // 16-byte Folded Reload
	ldr	q9, [sp, #1264] // 16-byte Folded Reload
	smull	v5.4s, v9.4h, v25.4h
	mov	v3.16b, v26.16b
	ldr	q22, [sp, #800] // 16-byte Folded Reload
	smlal	v5.4s, v22.4h, v3.4h
	mov	v0.16b, v15.16b
	ldr	q1, [sp, #608] // 16-byte Folded Reload
	smlal	v5.4s, v0.4h, v1.4h
	ldr	q19, [sp, #912] // 16-byte Folded Reload
	ldr	q15, [sp, #1248] // 16-byte Folded Reload
	smlal2	v5.4s, v15.8h, v19.8h
	ldr	q26, [sp, #416] // 16-byte Folded Reload
	mov	v1.16b, v23.16b
	smlal	v5.4s, v26.4h, v1.4h
	ldr	q27, [sp, #528] // 16-byte Folded Reload
	smlal	v5.4s, v27.4h, v13.4h
	mov	v18.16b, v31.16b
	smlal	v5.4s, v18.4h, v29.4h
	smlal	v5.4s, v28.4h, v19.4h
	mov	v20.16b, v24.16b
	smlal	v5.4s, v12.4h, v20.4h
	mov	v23.16b, v2.16b
	ldr	q2, [sp, #544] // 16-byte Folded Reload
	smlal	v5.4s, v23.4h, v2.4h
	ldr	q24, [sp, #464] // 16-byte Folded Reload
	smlal	v4.4s, v23.4h, v24.4h
	smlal	v5.4s, v7.4h, v24.4h
	smull	v6.4s, v0.4h, v25.4h
	mov	v31.16b, v0.16b
	smlal	v6.4s, v9.4h, v3.4h
	smlal2	v6.4s, v15.8h, v29.8h
	smlal2	v6.4s, v22.8h, v19.8h
	mov	v25.16b, v22.16b
	smlal	v6.4s, v27.4h, v1.4h
	smlal	v6.4s, v18.4h, v13.4h
	smlal	v6.4s, v28.4h, v29.4h
	smlal	v6.4s, v12.4h, v19.4h
	smlal	v6.4s, v23.4h, v20.4h
	smlal	v6.4s, v7.4h, v2.4h
	smlal	v4.4s, v12.4h, v2.4h
	smlal	v4.4s, v28.4h, v20.4h
	smlal	v4.4s, v18.4h, v19.4h
	smlal	v4.4s, v27.4h, v29.4h
	smlal	v4.4s, v26.4h, v13.4h
	ldr	q0, [sp, #288] // 16-byte Folded Reload
	smlal	v4.4s, v0.4h, v1.4h
	ldr	q0, [sp, #576] // 16-byte Folded Reload
	smlal	v4.4s, v31.4h, v0.4h
	ldr	q18, [sp, #1072] // 16-byte Folded Reload
	ldr	q1, [sp, #1024] // 16-byte Folded Reload
	smlal2	v16.4s, v1.8h, v18.8h
	ldp	q2, q3, [sp, #960] // 32-byte Folded Reload
	ldr	q0, [sp, #352] // 16-byte Folded Reload
	smlal	v16.4s, v0.4h, v3.4h
	smlal2	v5.4s, v28.8h, v11.8h
	smlal	v5.4s, v30.4h, v8.4h
	smlal2	v17.4s, v1.8h, v2.8h
	ldr	q1, [sp, #1040] // 16-byte Folded Reload
	smlal2	v17.4s, v1.8h, v18.8h
	mov	v24.16b, v18.16b
	ldr	q1, [sp, #336] // 16-byte Folded Reload
	smlal	v17.4s, v1.4h, v3.4h
	ldr	q7, [sp, #1088] // 16-byte Folded Reload
	smlal	v16.4s, v1.4h, v7.4h
	ldr	q3, [sp, #768] // 16-byte Folded Reload
	smlal2	v6.4s, v28.8h, v3.8h
	smlal2	v6.4s, v12.8h, v11.8h
	smlal	v6.4s, v14.4h, v8.4h
	ldr	q18, [sp, #896] // 16-byte Folded Reload
	smlal	v5.4s, v14.4h, v18.4h
	ldr	q0, [sp, #640] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v7.4h
	smlal	v16.4s, v0.4h, v2.4h
	ldr	q0, [sp, #496] // 16-byte Folded Reload
	smlal	v6.4s, v0.4h, v18.4h
	smlal	v5.4s, v0.4h, v3.4h
	mov	v19.16b, v3.16b
	ldr	q3, [sp, #832] // 16-byte Folded Reload
	ldr	q0, [sp, #704] // 16-byte Folded Reload
	smlal	v10.4s, v3.4h, v0.4h
	smull	v3.4s, v3.4h, v24.4h
	ldr	q23, [sp, #1280] // 16-byte Folded Reload
	smlal	v3.4s, v23.4h, v2.4h
	smull	v18.4s, v25.4h, v11.4h
	smlal	v18.4s, v15.4h, v19.4h
	uzp1	v19.8h, v3.8h, v18.8h
	ldr	q1, [sp, #944] // 16-byte Folded Reload
	mul	v19.8h, v19.8h, v1.8h
	mov	v7.16b, v21.16b
	smull2	v20.4s, v19.8h, v21.8h
	subhn	v18.4h, v18.4s, v20.4s
	smull	v19.4s, v19.4h, v7.4h
	subhn	v3.4h, v3.4s, v19.4s
	uzp1	v19.8h, v17.8h, v6.8h
	mul	v19.8h, v19.8h, v1.8h
	smull	v20.4s, v19.4h, v7.4h
	subhn	v17.4h, v17.4s, v20.4s
	smull2	v19.4s, v19.8h, v21.8h
	mov	v20.16b, v17.16b
	subhn2	v20.8h, v6.4s, v19.4s
	subhn	v6.4h, v6.4s, v19.4s
	ldr	d21, [sp, #1240] // 8-byte Folded Reload
	smull	v6.4s, v6.4h, v21.4h
	ldr	q2, [sp, #1184] // 16-byte Folded Reload
	mul	v19.8h, v20.8h, v2.8h
	smull2	v20.4s, v19.8h, v7.8h
	subhn	v6.4h, v6.4s, v20.4s
	ldr	d20, [sp, #1160] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v20.4h
	smull	v19.4s, v19.4h, v7.4h
	subhn	v17.4h, v17.4s, v19.4s
	add	v3.4h, v17.4h, v3.4h
	add	v17.4h, v6.4h, v18.4h
	ldr	q0, [sp, #608] // 16-byte Folded Reload
	smlal	v4.4s, v9.4h, v0.4h
	movi	v0.4h, #171
	smull	v6.4s, v3.4h, v0.4h
	mov	v3.d[1], v17.d[0]
	mvni	v18.8h, #84
	mul	v3.8h, v3.8h, v18.8h
	smull	v18.4s, v3.4h, v7.4h
	subhn	v6.4h, v6.4s, v18.4s
	smull	v17.4s, v17.4h, v0.4h
	smull2	v3.4s, v3.8h, v7.8h
	subhn2	v6.8h, v17.4s, v3.4s
	mov	v3.16b, v23.16b
	ldr	q17, [sp, #272] // 16-byte Folded Reload
	smlal	v10.4s, v3.4h, v17.4h
	smull	v3.4s, v3.4h, v24.4h
	smull	v17.4s, v15.4h, v11.4h
	ldr	q18, [sp, #752] // 16-byte Folded Reload
	smlal	v4.4s, v25.4h, v18.4h
	uzp1	v18.8h, v3.8h, v17.8h
	mul	v18.8h, v18.8h, v1.8h
	smull2	v19.4s, v18.8h, v7.8h
	subhn	v17.4h, v17.4s, v19.4s
	uzp1	v19.8h, v16.8h, v5.8h
	smull	v18.4s, v18.4h, v7.4h
	mul	v19.8h, v19.8h, v1.8h
	subhn	v3.4h, v3.4s, v18.4s
	smull	v18.4s, v19.4h, v7.4h
	subhn	v16.4h, v16.4s, v18.4s
	smull	v18.4s, v16.4h, v20.4h
	smull2	v19.4s, v19.8h, v7.8h
	subhn2	v16.8h, v5.4s, v19.4s
	subhn	v5.4h, v5.4s, v19.4s
	smull	v5.4s, v5.4h, v21.4h
	mul	v16.8h, v16.8h, v2.8h
	smull2	v19.4s, v16.8h, v7.8h
	subhn	v5.4h, v5.4s, v19.4s
	smull	v16.4s, v16.4h, v7.4h
	mov	v19.16b, v7.16b
	subhn	v16.4h, v18.4s, v16.4s
	add	v3.4h, v16.4h, v3.4h
	add	v5.4h, v5.4h, v17.4h
	mov	v16.16b, v3.16b
	mov	v16.d[1], v5.d[0]
	smull	v3.4s, v3.4h, v0.4h
	ldr	q2, [sp, #480] // 16-byte Folded Reload
	smlal	v4.4s, v15.4h, v2.4h
	uzp1	v17.8h, v10.8h, v4.8h
	mvni	v2.8h, #84
	mul	v16.8h, v16.8h, v2.8h
	smull	v18.4s, v16.4h, v19.4h
	subhn	v18.4h, v3.4s, v18.4s
	mul	v3.8h, v17.8h, v1.8h
	smull	v17.4s, v3.4h, v19.4h
	subhn	v7.4h, v10.4s, v17.4s
	mov	v17.16b, v7.16b
	smull2	v3.4s, v3.8h, v19.8h
	subhn2	v17.8h, v4.4s, v3.4s
	subhn	v4.4h, v4.4s, v3.4s
	smull	v3.4s, v7.4h, v0.4h
	smull	v5.4s, v5.4h, v0.4h
	smull2	v7.4s, v16.8h, v19.8h
	mul	v16.8h, v17.8h, v2.8h
	subhn2	v18.8h, v5.4s, v7.4s
	smull	v5.4s, v16.4h, v19.4h
	subhn	v3.4h, v3.4s, v5.4s
	smull	v4.4s, v4.4h, v0.4h
	smull2	v5.4s, v16.8h, v19.8h
	subhn2	v3.8h, v4.4s, v5.4s
	trn1	v4.8h, v18.8h, v6.8h
	ldr	q0, [sp, #688] // 16-byte Folded Reload
	ldr	q1, [sp, #1216] // 16-byte Folded Reload
	trn1	v5.8h, v1.8h, v0.8h
	trn2	v0.8h, v1.8h, v0.8h
	ldp	q1, q7, [sp, #160] // 32-byte Folded Reload
	ldr	q2, [sp, #64] // 16-byte Folded Reload
	trn1	v16.8h, v2.8h, v1.8h
	trn2	v17.8h, v2.8h, v1.8h
	ldr	q1, [sp, #192] // 16-byte Folded Reload
	trn1	v19.8h, v1.8h, v7.8h
	trn2	v2.8h, v18.8h, v6.8h
	trn2	v6.8h, v1.8h, v7.8h
	trn1	v18.4s, v4.4s, v5.4s
	trn2	v7.4s, v4.4s, v5.4s
	trn1	v20.4s, v2.4s, v0.4s
	trn1	v21.4s, v16.4s, v19.4s
	trn2	v2.4s, v2.4s, v0.4s
	trn2	v16.4s, v16.4s, v19.4s
	trn1	v19.4s, v17.4s, v6.4s
	ext	v0.16b, v18.16b, v18.16b, #8
	mov	v4.16b, v18.16b
	mov	v4.d[1], v21.d[0]
	trn2	v17.4s, v17.4s, v6.4s
	ext	v5.16b, v20.16b, v20.16b, #8
	mov	v6.16b, v20.16b
	ext	v18.16b, v21.16b, v21.16b, #8
	mov	v6.d[1], v19.d[0]
	ext	v19.16b, v19.16b, v19.16b, #8
	ldp	q21, q1, [sp, #128] // 32-byte Folded Reload
	trn1	v20.8h, v1.8h, v21.8h
	trn2	v21.8h, v1.8h, v21.8h
	ldp	q23, q1, [sp, #96] // 32-byte Folded Reload
	trn1	v22.8h, v1.8h, v23.8h
	mov	v0.d[1], v18.d[0]
	trn2	v23.8h, v1.8h, v23.8h
	ldr	q1, [sp, #80] // 16-byte Folded Reload
	ldr	q18, [sp, #48] // 16-byte Folded Reload
	trn1	v24.8h, v1.8h, v18.8h
	trn2	v18.8h, v1.8h, v18.8h
	ldr	q1, [sp, #32] // 16-byte Folded Reload
	trn1	v25.8h, v1.8h, v3.8h
	mov	v5.d[1], v19.d[0]
	trn1	v26.4s, v20.4s, v22.4s
	trn1	v27.4s, v24.4s, v25.4s
	ext	v19.16b, v26.16b, v26.16b, #8
	mov	v26.d[1], v27.d[0]
	ext	v27.16b, v27.16b, v27.16b, #8
	mov	v19.d[1], v27.d[0]
	trn2	v3.8h, v1.8h, v3.8h
	trn2	v20.4s, v20.4s, v22.4s
	ext	v22.16b, v7.16b, v7.16b, #8
	mov	v7.d[1], v16.d[0]
	ext	v16.16b, v16.16b, v16.16b, #8
	mov	v22.d[1], v16.d[0]
	trn1	v16.4s, v21.4s, v23.4s
	trn2	v21.4s, v21.4s, v23.4s
	trn2	v23.4s, v24.4s, v25.4s
	ext	v24.16b, v16.16b, v16.16b, #8
	trn1	v25.4s, v18.4s, v3.4s
	mov	v16.d[1], v25.d[0]
	ext	v25.16b, v25.16b, v25.16b, #8
	mov	v24.d[1], v25.d[0]
	ext	v25.16b, v2.16b, v2.16b, #8
	mov	v2.d[1], v17.d[0]
	ext	v17.16b, v17.16b, v17.16b, #8
	mov	v25.d[1], v17.d[0]
	trn2	v3.4s, v18.4s, v3.4s
	ext	v17.16b, v20.16b, v20.16b, #8
	mov	v20.d[1], v23.d[0]
	ext	v18.16b, v23.16b, v23.16b, #8
	mov	v17.d[1], v18.d[0]
	ext	v18.16b, v21.16b, v21.16b, #8
	mov	v21.d[1], v3.d[0]
	ext	v3.16b, v3.16b, v3.16b, #8
	mov	v18.d[1], v3.d[0]
	stp	q4, q26, [x10, #-128]
	stp	q6, q16, [x10, #-96]
	stp	q7, q20, [x10, #-64]
	stp	q2, q21, [x10, #-32]
	stp	q0, q19, [x10]
	stp	q5, q24, [x10, #32]
	stp	q22, q17, [x10, #64]
	stp	q25, q18, [x10, #96]
	add	x11, x11, #4
	add	x8, x8, #256
	add	x9, x9, #256
	add	x10, x10, #256
	cmp	x11, #60
	b.lo	LBB4_1
// %bb.2:
	add	sp, sp, #1296
	ldp	x28, x27, [sp, #64] // 16-byte Folded Reload
	ldp	d9, d8, [sp, #48] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #32] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #16] // 16-byte Folded Reload
	ldp	d15, d14, [sp], #80 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/50_poly_basemul_ntt.inc
// begin expanded include: ntt_asm_parts/60_poly_basemul_ntt_mq.inc
	.globl	poly_basemul_ntt_mq // -- Begin function poly_basemul_ntt_mq
	.p2align	2
poly_basemul_ntt_mq: // @poly_basemul_ntt_mq
	.cfi_startproc
// %bb.0:
	stp	d15, d14, [sp, #-80]! // 16-byte Folded Spill
	stp	d13, d12, [sp, #16] // 16-byte Folded Spill
	stp	d11, d10, [sp, #32] // 16-byte Folded Spill
	stp	d9, d8, [sp, #48] // 16-byte Folded Spill
	stp	x28, x27, [sp, #64] // 16-byte Folded Spill
	sub	sp, sp, #1456
	.cfi_def_cfa_offset 1536
	.cfi_offset w27, -8
	.cfi_offset w28, -16
	.cfi_offset b8, -24
	.cfi_offset b9, -32
	.cfi_offset b10, -40
	.cfi_offset b11, -48
	.cfi_offset b12, -56
	.cfi_offset b13, -64
	.cfi_offset b14, -72
	.cfi_offset b15, -80
	add	x8, x1, #128
	add	x9, x2, #128
	add	x10, x0, #128
	mov	x11, #-4 // =0xfffffffffffffffc
	mov	w12, #64769 // =0xfd01
	dup	v0.8h, w12
	str	q0, [sp, #944] // 16-byte Folded Spill
	mov	w12, #769 // =0x301
	dup	v1.8h, w12
	mov	w13, #50963 // =0xc713
Lloh8:
	adrp	x12, f+128
Lloh9:
	add	x12, x12, :lo12:f+128
	dup	v0.8h, w13
	stp	q0, q1, [sp, #288] // 32-byte Folded Spill
LBB5_1: // =>This Inner Loop Header: Depth=1
	ldr	d3, [x12], #8
	ldp	q4, q0, [x8, #-128]
	neg	v5.4h, v3.4h
	ldp	q6, q2, [x8, #-96]
	trn1	v7.8h, v4.8h, v6.8h
	trn2	v4.8h, v4.8h, v6.8h
	ldp	q16, q6, [x8, #-64]
	ldp	q17, q18, [x8, #-32]
	trn1	v19.8h, v16.8h, v17.8h
	trn2	v16.8h, v16.8h, v17.8h
	zip1	v1.4h, v3.4h, v5.4h
	str	d1, [sp, #1104] // 8-byte Folded Spill
	ldp	q17, q20, [x8]
	ldp	q21, q22, [x8, #32]
	trn1	v23.8h, v17.8h, v21.8h
	zip2	v1.4h, v3.4h, v5.4h
	str	d1, [sp, #1112] // 8-byte Folded Spill
	trn2	v17.8h, v17.8h, v21.8h
	ldp	q21, q24, [x8, #64]
	ldp	q25, q27, [x8, #96]
	zip1	v1.8h, v3.8h, v5.8h
	str	q1, [sp, #928] // 16-byte Folded Spill
	trn1	v3.8h, v21.8h, v25.8h
	trn2	v5.8h, v21.8h, v25.8h
	trn1	v29.4s, v7.4s, v19.4s
	trn2	v28.4s, v7.4s, v19.4s
	trn1	v31.4s, v4.4s, v16.4s
	trn2	v1.4s, v4.4s, v16.4s
	str	q1, [sp, #1040] // 16-byte Folded Spill
	trn1	v11.4s, v23.4s, v3.4s
	trn2	v14.4s, v23.4s, v3.4s
	trn1	v3.8h, v0.8h, v2.8h
	trn2	v0.8h, v0.8h, v2.8h
	trn1	v4.8h, v6.8h, v18.8h
	trn1	v26.4s, v17.4s, v5.4s
	trn2	v6.8h, v6.8h, v18.8h
	trn1	v7.8h, v20.8h, v22.8h
	trn2	v2.8h, v20.8h, v22.8h
	trn1	v16.8h, v24.8h, v27.8h
	trn2	v18.8h, v24.8h, v27.8h
	trn2	v24.4s, v17.4s, v5.4s
	trn1	v5.4s, v3.4s, v4.4s
	str	q5, [sp, #1376] // 16-byte Folded Spill
	trn2	v3.4s, v3.4s, v4.4s
	str	q3, [sp, #1312] // 16-byte Folded Spill
	trn1	v12.4s, v0.4s, v6.4s
	trn2	v0.4s, v0.4s, v6.4s
	str	q0, [sp, #1360] // 16-byte Folded Spill
	ldp	q5, q0, [x9, #-128]
	trn1	v15.4s, v7.4s, v16.4s
	ldp	q6, q3, [x9, #-96]
	ldp	q17, q4, [x9, #-64]
	trn2	v1.4s, v7.4s, v16.4s
	str	q1, [sp, #1408] // 16-byte Folded Spill
	trn1	v7.8h, v5.8h, v6.8h
	trn2	v5.8h, v5.8h, v6.8h
	ldp	q6, q16, [x9, #-32]
	trn1	v19.8h, v17.8h, v6.8h
	trn1	v1.4s, v2.4s, v18.4s
	str	q1, [sp, #992] // 16-byte Folded Spill
	trn2	v6.8h, v17.8h, v6.8h
	ldp	q17, q20, [x9]
	ldp	q21, q25, [x9, #32]
	trn2	v2.4s, v2.4s, v18.4s
	str	q2, [sp, #1440] // 16-byte Folded Spill
	trn1	v2.8h, v17.8h, v21.8h
	trn2	v17.8h, v17.8h, v21.8h
	ldp	q18, q21, [x9, #64]
	ldp	q22, q30, [x9, #96]
	trn1	v27.8h, v18.8h, v22.8h
	trn2	v18.8h, v18.8h, v22.8h
	trn1	v22.4s, v7.4s, v19.4s
	trn2	v7.4s, v7.4s, v19.4s
	str	q7, [sp, #1248] // 16-byte Folded Spill
	trn1	v9.4s, v5.4s, v6.4s
	trn2	v13.4s, v5.4s, v6.4s
	trn1	v5.8h, v0.8h, v3.8h
	trn2	v0.8h, v0.8h, v3.8h
	trn1	v3.8h, v4.8h, v16.8h
	trn2	v4.8h, v4.8h, v16.8h
	trn1	v10.4s, v2.4s, v27.4s
	trn2	v1.4s, v2.4s, v27.4s
	str	q1, [sp, #1184] // 16-byte Folded Spill
	trn1	v2.8h, v20.8h, v25.8h
	trn2	v7.8h, v20.8h, v25.8h
	trn1	v16.8h, v21.8h, v30.8h
	trn2	v19.8h, v21.8h, v30.8h
	trn1	v8.4s, v17.4s, v18.4s
	trn2	v23.4s, v17.4s, v18.4s
	trn1	v27.4s, v5.4s, v3.4s
	trn2	v18.4s, v5.4s, v3.4s
	trn1	v25.4s, v0.4s, v4.4s
	trn2	v5.4s, v0.4s, v4.4s
	trn1	v17.4s, v2.4s, v16.4s
	trn2	v21.4s, v2.4s, v16.4s
	trn1	v1.4s, v7.4s, v19.4s
	trn2	v7.4s, v7.4s, v19.4s
	smull	v0.4s, v29.4h, v22.4h
	str	q22, [sp, #1232] // 16-byte Folded Spill
	mov	v6.16b, v11.16b
	smull	v2.4s, v6.4h, v10.4h
	str	q10, [sp, #1168] // 16-byte Folded Spill
	uzp1	v3.8h, v0.8h, v2.8h
	ldr	q4, [sp, #944] // 16-byte Folded Reload
	mul	v3.8h, v3.8h, v4.8h
	mov	v19.16b, v4.16b
	ldr	q16, [sp, #304] // 16-byte Folded Reload
	smull2	v4.4s, v3.8h, v16.8h
	subhn	v2.4h, v2.4s, v4.4s
	str	d2, [sp, #368] // 8-byte Folded Spill
	smull	v2.4s, v3.4h, v16.4h
	subhn	v0.4h, v0.4s, v2.4s
	str	d0, [sp, #352] // 8-byte Folded Spill
	str	q31, [sp, #1072] // 16-byte Folded Spill
	smull	v0.4s, v31.4h, v22.4h
	smlal	v0.4s, v29.4h, v9.4h
	mov	v22.16b, v29.16b
	str	q29, [sp, #1136] // 16-byte Folded Spill
	smull	v2.4s, v26.4h, v10.4h
	smlal	v2.4s, v6.4h, v8.4h
	str	q11, [sp, #1296] // 16-byte Folded Spill
	uzp1	v3.8h, v0.8h, v2.8h
	mul	v3.8h, v3.8h, v19.8h
	smull2	v4.4s, v3.8h, v16.8h
	subhn	v2.4h, v2.4s, v4.4s
	str	d2, [sp, #336] // 8-byte Folded Spill
	smull	v2.4s, v3.4h, v16.4h
	subhn	v0.4h, v0.4s, v2.4s
	str	d0, [sp, #240] // 8-byte Folded Spill
	mov	v19.16b, v18.16b
	str	q18, [sp, #1392] // 16-byte Folded Spill
	ext	v0.16b, v18.16b, v18.16b, #8
	str	q0, [sp, #800] // 16-byte Folded Spill
	str	q28, [sp, #704] // 16-byte Folded Spill
	smull	v0.4s, v28.4h, v0.4h
	mov	v3.16b, v5.16b
	str	q5, [sp, #1344] // 16-byte Folded Spill
	ext	v10.16b, v5.16b, v5.16b, #8
	smlal	v0.4s, v31.4h, v10.4h
	str	q10, [sp, #768] // 16-byte Folded Spill
	str	q21, [sp, #1328] // 16-byte Folded Spill
	ext	v30.16b, v21.16b, v21.16b, #8
	str	q14, [sp, #1088] // 16-byte Folded Spill
	smull	v2.4s, v14.4h, v30.4h
	str	q30, [sp, #576] // 16-byte Folded Spill
	mov	v16.16b, v7.16b
	str	q7, [sp, #1424] // 16-byte Folded Spill
	ext	v29.16b, v7.16b, v7.16b, #8
	str	q26, [sp, #1024] // 16-byte Folded Spill
	smlal	v2.4s, v26.4h, v29.4h
	str	q29, [sp, #896] // 16-byte Folded Spill
	str	q25, [sp, #1216] // 16-byte Folded Spill
	ext	v4.16b, v25.16b, v25.16b, #8
	str	q4, [sp, #416] // 16-byte Folded Spill
	ldr	q7, [sp, #1040] // 16-byte Folded Reload
	smlal	v0.4s, v7.4h, v4.4h
	str	q1, [sp, #1280] // 16-byte Folded Spill
	ext	v5.16b, v1.16b, v1.16b, #8
	str	q5, [sp, #528] // 16-byte Folded Spill
	mov	v4.16b, v24.16b
	smlal	v2.4s, v4.4h, v5.4h
	str	q27, [sp, #1200] // 16-byte Folded Spill
	smlal2	v0.4s, v22.8h, v27.8h
	str	q17, [sp, #1264] // 16-byte Folded Spill
	smlal2	v2.4s, v11.8h, v17.8h
	ext	v5.16b, v31.16b, v31.16b, #8
	str	q5, [sp, #656] // 16-byte Folded Spill
	smlal	v0.4s, v5.4h, v3.4h
	ext	v5.16b, v26.16b, v26.16b, #8
	str	q5, [sp, #448] // 16-byte Folded Spill
	smlal	v2.4s, v5.4h, v16.4h
	ext	v11.16b, v28.16b, v28.16b, #8
	smlal	v0.4s, v11.4h, v19.4h
	str	q11, [sp, #384] // 16-byte Folded Spill
	ext	v16.16b, v14.16b, v14.16b, #8
	smlal	v2.4s, v16.4h, v21.4h
	mov	v26.16b, v16.16b
	str	q16, [sp, #608] // 16-byte Folded Spill
	ext	v24.16b, v7.16b, v7.16b, #8
	mov	v14.16b, v7.16b
	smlal	v0.4s, v24.4h, v25.4h
	ext	v22.16b, v4.16b, v4.16b, #8
	mov	v16.16b, v4.16b
	str	q4, [sp, #1120] // 16-byte Folded Spill
	smlal	v2.4s, v22.4h, v1.4h
	stp	q24, q22, [sp, #720] // 32-byte Folded Spill
	ldr	q31, [sp, #1376] // 16-byte Folded Reload
	smlal	v0.4s, v31.4h, v27.4h
	str	q15, [sp, #1152] // 16-byte Folded Spill
	smlal	v2.4s, v15.4h, v17.4h
	ext	v28.16b, v13.16b, v13.16b, #8
	str	q12, [sp, #1056] // 16-byte Folded Spill
	smlal	v0.4s, v12.4h, v28.4h
	str	q28, [sp, #832] // 16-byte Folded Spill
	mov	v17.16b, v23.16b
	ext	v27.16b, v23.16b, v23.16b, #8
	ldr	q18, [sp, #992] // 16-byte Folded Reload
	smlal	v2.4s, v18.4h, v27.4h
	str	q27, [sp, #752] // 16-byte Folded Spill
	ldr	q19, [sp, #1248] // 16-byte Folded Reload
	ext	v23.16b, v19.16b, v19.16b, #8
	ldr	q6, [sp, #1312] // 16-byte Folded Reload
	smlal	v0.4s, v6.4h, v23.4h
	str	q23, [sp, #400] // 16-byte Folded Spill
	ldr	q3, [sp, #1184] // 16-byte Folded Reload
	ext	v21.16b, v3.16b, v3.16b, #8
	ldr	q25, [sp, #1408] // 16-byte Folded Reload
	smlal	v2.4s, v25.4h, v21.4h
	str	q21, [sp, #432] // 16-byte Folded Spill
	mov	v5.16b, v9.16b
	ext	v4.16b, v9.16b, v9.16b, #8
	str	q4, [sp, #640] // 16-byte Folded Spill
	ldr	q9, [sp, #1360] // 16-byte Folded Reload
	smlal	v0.4s, v9.4h, v4.4h
	mov	v20.16b, v8.16b
	ext	v4.16b, v8.16b, v8.16b, #8
	str	q4, [sp, #912] // 16-byte Folded Spill
	ldr	q8, [sp, #1440] // 16-byte Folded Reload
	smlal	v2.4s, v8.4h, v4.4h
	ldr	q1, [sp, #1232] // 16-byte Folded Reload
	smlal2	v0.4s, v31.8h, v1.8h
	ldr	q1, [sp, #1168] // 16-byte Folded Reload
	smlal2	v2.4s, v15.8h, v1.8h
	ext	v4.16b, v12.16b, v12.16b, #8
	str	q4, [sp, #560] // 16-byte Folded Spill
	smlal	v0.4s, v4.4h, v13.4h
	mov	v12.16b, v13.16b
	str	q13, [sp, #976] // 16-byte Folded Spill
	ext	v4.16b, v18.16b, v18.16b, #8
	stp	q4, q17, [sp, #864] // 32-byte Folded Spill
	smlal	v2.4s, v4.4h, v17.4h
	mov	v31.16b, v17.16b
	ext	v4.16b, v6.16b, v6.16b, #8
	mov	v18.16b, v6.16b
	smlal	v0.4s, v4.4h, v19.4h
	mov	v6.16b, v4.16b
	str	q4, [sp, #544] // 16-byte Folded Spill
	ext	v4.16b, v25.16b, v25.16b, #8
	smlal	v2.4s, v4.4h, v3.4h
	mov	v7.16b, v4.16b
	str	q4, [sp, #848] // 16-byte Folded Spill
	ext	v3.16b, v9.16b, v9.16b, #8
	smlal	v0.4s, v3.4h, v5.4h
	mov	v13.16b, v5.16b
	mov	v5.16b, v3.16b
	str	q3, [sp, #512] // 16-byte Folded Spill
	ext	v3.16b, v8.16b, v8.16b, #8
	mov	v1.16b, v8.16b
	smlal	v2.4s, v3.4h, v20.4h
	mov	v17.16b, v3.16b
	str	q3, [sp, #816] // 16-byte Folded Spill
	uzp1	v3.8h, v0.8h, v2.8h
	ldr	q15, [sp, #944] // 16-byte Folded Reload
	mul	v3.8h, v3.8h, v15.8h
	ldr	q4, [sp, #304] // 16-byte Folded Reload
	smull	v4.4s, v3.4h, v4.4h
	subhn	v19.4h, v0.4s, v4.4s
	str	q19, [sp, #224] // 16-byte Folded Spill
	ldr	q0, [sp, #800] // 16-byte Folded Reload
	smull	v0.4s, v14.4h, v0.4h
	ldr	q4, [sp, #704] // 16-byte Folded Reload
	smlal	v0.4s, v4.4h, v10.4h
	smull	v4.4s, v16.4h, v30.4h
	ldr	q16, [sp, #1088] // 16-byte Folded Reload
	smlal	v4.4s, v16.4h, v29.4h
	ldr	q9, [sp, #1216] // 16-byte Folded Reload
	ldr	q16, [sp, #1136] // 16-byte Folded Reload
	smlal2	v0.4s, v16.8h, v9.8h
	ldr	q8, [sp, #1280] // 16-byte Folded Reload
	ldr	q16, [sp, #1296] // 16-byte Folded Reload
	smlal2	v4.4s, v16.8h, v8.8h
	ldr	q10, [sp, #1200] // 16-byte Folded Reload
	ldr	q14, [sp, #1072] // 16-byte Folded Reload
	smlal2	v0.4s, v14.8h, v10.8h
	ldr	q29, [sp, #1264] // 16-byte Folded Reload
	ldr	q30, [sp, #1024] // 16-byte Folded Reload
	smlal2	v4.4s, v30.8h, v29.8h
	ldr	q16, [sp, #1344] // 16-byte Folded Reload
	smlal	v0.4s, v11.4h, v16.4h
	ldr	q16, [sp, #1424] // 16-byte Folded Reload
	smlal	v4.4s, v26.4h, v16.4h
	ldr	q16, [sp, #1392] // 16-byte Folded Reload
	smlal	v0.4s, v24.4h, v16.4h
	ldr	q16, [sp, #1328] // 16-byte Folded Reload
	smlal	v4.4s, v22.4h, v16.4h
	ldr	q22, [sp, #1376] // 16-byte Folded Reload
	smlal	v0.4s, v22.4h, v9.4h
	ldr	q24, [sp, #1152] // 16-byte Folded Reload
	smlal	v4.4s, v24.4h, v8.4h
	ldr	q16, [sp, #1056] // 16-byte Folded Reload
	smlal	v0.4s, v16.4h, v10.4h
	ldr	q26, [sp, #992] // 16-byte Folded Reload
	smlal	v4.4s, v26.4h, v29.4h
	smlal	v0.4s, v18.4h, v28.4h
	smlal	v4.4s, v25.4h, v27.4h
	ldr	q11, [sp, #1360] // 16-byte Folded Reload
	smlal	v0.4s, v11.4h, v23.4h
	smlal	v4.4s, v1.4h, v21.4h
	mov	v25.16b, v13.16b
	smlal2	v0.4s, v22.8h, v13.8h
	smlal2	v4.4s, v24.8h, v20.8h
	mov	v18.16b, v20.16b
	ldr	q24, [sp, #1232] // 16-byte Folded Reload
	smlal2	v0.4s, v16.8h, v24.8h
	mov	v23.16b, v16.16b
	ldr	q16, [sp, #1168] // 16-byte Folded Reload
	smlal2	v4.4s, v26.8h, v16.8h
	mov	v28.16b, v26.16b
	smlal	v0.4s, v6.4h, v12.4h
	smlal	v4.4s, v7.4h, v31.4h
	ldr	q21, [sp, #1248] // 16-byte Folded Reload
	smlal	v0.4s, v5.4h, v21.4h
	ldr	q29, [sp, #1184] // 16-byte Folded Reload
	smlal	v4.4s, v17.4h, v29.4h
	uzp1	v5.8h, v0.8h, v4.8h
	mul	v5.8h, v5.8h, v15.8h
	ldr	q6, [sp, #304] // 16-byte Folded Reload
	smull	v7.4s, v5.4h, v6.4h
	subhn	v0.4h, v0.4s, v7.4s
	str	q0, [sp, #672] // 16-byte Folded Spill
	smull2	v3.4s, v3.8h, v6.8h
	subhn2	v19.8h, v2.4s, v3.4s
	str	q19, [sp, #176] // 16-byte Folded Spill
	subhn	v1.4h, v2.4s, v3.4s
	str	d1, [sp, #208] // 8-byte Folded Spill
	smull2	v2.4s, v5.8h, v6.8h
	mov	v3.16b, v0.16b
	subhn2	v3.8h, v4.4s, v2.4s
	subhn	v2.4h, v4.4s, v2.4s
	ldr	q1, [sp, #928] // 16-byte Folded Reload
	mul	v0.8h, v1.8h, v15.8h
	str	q0, [sp, #784] // 16-byte Folded Spill
	mov	v5.16b, v15.16b
	ldr	d26, [sp, #1112] // 8-byte Folded Reload
	smull	v2.4s, v2.4h, v26.4h
	mul	v15.8h, v3.8h, v0.8h
	smull2	v3.4s, v15.8h, v6.8h
	mov	v1.16b, v6.16b
	subhn	v2.4h, v2.4s, v3.4s
	str	d2, [sp, #320] // 8-byte Folded Spill
	mov	v10.16b, v14.16b
	smull	v2.4s, v10.4h, v25.4h
	mov	v22.16b, v13.16b
	ldr	q8, [sp, #1136] // 16-byte Folded Reload
	smlal	v2.4s, v8.4h, v21.4h
	smull	v3.4s, v30.4h, v18.4h
	mov	v9.16b, v30.16b
	mov	v12.16b, v20.16b
	ldr	q18, [sp, #1296] // 16-byte Folded Reload
	smlal	v3.4s, v18.4h, v29.4h
	ldr	q7, [sp, #704] // 16-byte Folded Reload
	smlal	v2.4s, v7.4h, v24.4h
	mov	v0.16b, v24.16b
	ldr	q17, [sp, #1088] // 16-byte Folded Reload
	smlal	v3.4s, v17.4h, v16.4h
	mov	v14.16b, v16.16b
	uzp1	v4.8h, v2.8h, v3.8h
	mov	v27.16b, v5.16b
	mul	v4.8h, v4.8h, v5.8h
	smull2	v5.4s, v4.8h, v6.8h
	subhn	v3.4h, v3.4s, v5.4s
	str	d3, [sp, #928] // 8-byte Folded Spill
	smull	v3.4s, v4.4h, v1.4h
	subhn	v2.4h, v2.4s, v3.4s
	str	d2, [sp, #688] // 8-byte Folded Spill
	ldr	q13, [sp, #1040] // 16-byte Folded Reload
	ldr	q2, [sp, #768] // 16-byte Folded Reload
	smull	v2.4s, v13.4h, v2.4h
	ldr	q24, [sp, #1392] // 16-byte Folded Reload
	smlal2	v2.4s, v8.8h, v24.8h
	ldr	q3, [sp, #896] // 16-byte Folded Reload
	ldr	q31, [sp, #1120] // 16-byte Folded Reload
	smull	v3.4s, v31.4h, v3.4h
	ldr	q30, [sp, #1328] // 16-byte Folded Reload
	smlal2	v3.4s, v18.8h, v30.8h
	ldr	q6, [sp, #1216] // 16-byte Folded Reload
	smlal2	v2.4s, v10.8h, v6.8h
	ldr	q5, [sp, #1280] // 16-byte Folded Reload
	smlal2	v3.4s, v9.8h, v5.8h
	ldr	q20, [sp, #1200] // 16-byte Folded Reload
	smlal2	v2.4s, v7.8h, v20.8h
	ldr	q16, [sp, #1264] // 16-byte Folded Reload
	smlal2	v3.4s, v17.8h, v16.8h
	ldr	q4, [sp, #720] // 16-byte Folded Reload
	ldr	q19, [sp, #1344] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v19.4h
	ldr	q4, [sp, #736] // 16-byte Folded Reload
	ldr	q19, [sp, #1424] // 16-byte Folded Reload
	smlal	v3.4s, v4.4h, v19.4h
	ldr	q19, [sp, #1376] // 16-byte Folded Reload
	smlal	v2.4s, v19.4h, v24.4h
	ldr	q24, [sp, #1152] // 16-byte Folded Reload
	smlal	v3.4s, v24.4h, v30.4h
	mov	v25.16b, v23.16b
	smlal	v2.4s, v25.4h, v6.4h
	mov	v23.16b, v28.16b
	smlal	v3.4s, v23.4h, v5.4h
	ldr	q4, [sp, #1312] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v20.4h
	ldr	q5, [sp, #1408] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v16.4h
	ldr	q6, [sp, #832] // 16-byte Folded Reload
	smlal	v2.4s, v11.4h, v6.4h
	ldr	q20, [sp, #752] // 16-byte Folded Reload
	ldr	q6, [sp, #1440] // 16-byte Folded Reload
	smlal	v3.4s, v6.4h, v20.4h
	mov	v20.16b, v21.16b
	smlal2	v2.4s, v19.8h, v21.8h
	mov	v28.16b, v29.16b
	smlal2	v3.4s, v24.8h, v29.8h
	mov	v19.16b, v24.16b
	mov	v11.16b, v22.16b
	str	q22, [sp, #960] // 16-byte Folded Spill
	smlal2	v2.4s, v25.8h, v22.8h
	str	q12, [sp, #1008] // 16-byte Folded Spill
	smlal2	v3.4s, v23.8h, v12.8h
	mov	v25.16b, v0.16b
	smlal2	v2.4s, v4.8h, v0.8h
	mov	v16.16b, v14.16b
	smlal2	v3.4s, v5.8h, v14.8h
	ldr	q29, [sp, #976] // 16-byte Folded Reload
	ldr	q0, [sp, #512] // 16-byte Folded Reload
	smlal	v2.4s, v0.4h, v29.4h
	ldr	q21, [sp, #880] // 16-byte Folded Reload
	ldr	q0, [sp, #816] // 16-byte Folded Reload
	smlal	v3.4s, v0.4h, v21.4h
	uzp1	v4.8h, v2.8h, v3.8h
	mul	v4.8h, v4.8h, v27.8h
	smull	v5.4s, v4.4h, v1.4h
	subhn	v2.4h, v2.4s, v5.4s
	ldr	d22, [sp, #1104] // 8-byte Folded Reload
	ldr	q0, [sp, #672] // 16-byte Folded Reload
	smull	v0.4s, v0.4h, v22.4h
	smull	v5.4s, v15.4h, v1.4h
	subhn	v0.4h, v0.4s, v5.4s
	str	d0, [sp, #192] // 8-byte Folded Spill
	smull2	v0.4s, v4.8h, v1.8h
	mov	v4.16b, v2.16b
	subhn2	v4.8h, v3.4s, v0.4s
	subhn	v0.4h, v3.4s, v0.4s
	smull	v0.4s, v0.4h, v26.4h
	ldr	q14, [sp, #784] // 16-byte Folded Reload
	mul	v3.8h, v4.8h, v14.8h
	smull2	v4.4s, v3.8h, v1.8h
	subhn	v0.4h, v0.4s, v4.4s
	str	d0, [sp, #672] // 8-byte Folded Spill
	smull	v2.4s, v2.4h, v22.4h
	smull	v3.4s, v3.4h, v1.4h
	mov	v0.16b, v1.16b
	subhn	v1.4h, v2.4s, v3.4s
	str	d1, [sp, #624] // 8-byte Folded Spill
	smull	v2.4s, v10.4h, v20.4h
	smlal	v2.4s, v8.4h, v29.4h
	mov	v24.16b, v29.16b
	smull	v3.4s, v9.4h, v28.4h
	smlal	v3.4s, v18.4h, v21.4h
	mov	v26.16b, v21.16b
	mov	v20.16b, v7.16b
	smlal	v2.4s, v20.4h, v11.4h
	mov	v29.16b, v17.16b
	smlal	v3.4s, v29.4h, v12.4h
	smlal	v2.4s, v13.4h, v25.4h
	mov	v1.16b, v13.16b
	mov	v12.16b, v25.16b
	smlal	v3.4s, v31.4h, v16.4h
	uzp1	v4.8h, v2.8h, v3.8h
	mov	v17.16b, v27.16b
	mul	v4.8h, v4.8h, v27.8h
	smull2	v5.4s, v4.8h, v0.8h
	subhn	v3.4h, v3.4s, v5.4s
	smull	v4.4s, v4.4h, v0.4h
	subhn	v2.4h, v2.4s, v4.4s
	ldr	q28, [sp, #1392] // 16-byte Folded Reload
	smull2	v4.4s, v10.8h, v28.8h
	ldr	q22, [sp, #1344] // 16-byte Folded Reload
	smlal2	v4.4s, v8.8h, v22.8h
	mov	v13.16b, v8.16b
	smull2	v5.4s, v9.8h, v30.8h
	mov	v27.16b, v9.16b
	ldr	q7, [sp, #1424] // 16-byte Folded Reload
	smlal2	v5.4s, v18.8h, v7.8h
	mov	v15.16b, v18.16b
	ldr	q6, [sp, #1216] // 16-byte Folded Reload
	smlal2	v4.4s, v20.8h, v6.8h
	ldr	q21, [sp, #1280] // 16-byte Folded Reload
	smlal2	v5.4s, v29.8h, v21.8h
	ldr	q18, [sp, #1200] // 16-byte Folded Reload
	smlal2	v4.4s, v1.8h, v18.8h
	ldr	q1, [sp, #1264] // 16-byte Folded Reload
	smlal2	v5.4s, v31.8h, v1.8h
	ldr	q9, [sp, #1376] // 16-byte Folded Reload
	smlal	v4.4s, v9.4h, v22.4h
	smlal	v5.4s, v19.4h, v7.4h
	ldr	q8, [sp, #1056] // 16-byte Folded Reload
	smlal	v4.4s, v8.4h, v28.4h
	smlal	v5.4s, v23.4h, v30.4h
	ldr	q22, [sp, #1312] // 16-byte Folded Reload
	smlal	v4.4s, v22.4h, v6.4h
	ldr	q7, [sp, #1408] // 16-byte Folded Reload
	smlal	v5.4s, v7.4h, v21.4h
	ldr	q21, [sp, #1360] // 16-byte Folded Reload
	smlal	v4.4s, v21.4h, v18.4h
	ldr	q18, [sp, #1440] // 16-byte Folded Reload
	smlal	v5.4s, v18.4h, v1.4h
	smlal2	v4.4s, v9.8h, v24.8h
	smlal2	v5.4s, v19.8h, v26.8h
	mov	v25.16b, v19.16b
	ldr	q1, [sp, #1248] // 16-byte Folded Reload
	smlal2	v4.4s, v8.8h, v1.8h
	ldr	q6, [sp, #1184] // 16-byte Folded Reload
	smlal2	v5.4s, v23.8h, v6.8h
	ldr	q11, [sp, #960] // 16-byte Folded Reload
	smlal2	v4.4s, v22.8h, v11.8h
	ldr	q30, [sp, #1008] // 16-byte Folded Reload
	smlal2	v5.4s, v7.8h, v30.8h
	smlal2	v4.4s, v21.8h, v12.8h
	mov	v22.16b, v16.16b
	smlal2	v5.4s, v18.8h, v16.8h
	uzp1	v18.8h, v4.8h, v5.8h
	mul	v18.8h, v18.8h, v17.8h
	mov	v21.16b, v17.16b
	smull	v19.4s, v18.4h, v0.4h
	subhn	v4.4h, v4.4s, v19.4s
	ldr	d7, [sp, #688] // 8-byte Folded Reload
	ldr	d16, [sp, #624] // 8-byte Folded Reload
	add	v16.4h, v16.4h, v7.4h
	str	q16, [sp, #272] // 16-byte Folded Spill
	smull2	v16.4s, v18.8h, v0.8h
	mov	v17.16b, v4.16b
	subhn2	v17.8h, v5.4s, v16.4s
	subhn	v5.4h, v5.4s, v16.4s
	ldr	d7, [sp, #1112] // 8-byte Folded Reload
	smull	v5.4s, v5.4h, v7.4h
	mul	v16.8h, v17.8h, v14.8h
	smull2	v17.4s, v16.8h, v0.8h
	subhn	v5.4h, v5.4s, v17.4s
	ldr	d7, [sp, #1104] // 8-byte Folded Reload
	smull	v4.4s, v4.4h, v7.4h
	smull	v16.4s, v16.4h, v0.4h
	mov	v7.16b, v0.16b
	subhn	v4.4h, v4.4s, v16.4s
	ldr	d0, [sp, #928] // 8-byte Folded Reload
	ldr	d16, [sp, #672] // 8-byte Folded Reload
	add	v0.4h, v16.4h, v0.4h
	str	q0, [sp, #256] // 16-byte Folded Spill
	add	v0.4h, v4.4h, v2.4h
	str	q0, [sp, #128] // 16-byte Folded Spill
	add	v0.4h, v5.4h, v3.4h
	str	q0, [sp, #144] // 16-byte Folded Spill
	ext	v2.16b, v12.16b, v12.16b, #8
	str	q2, [sp, #624] // 16-byte Folded Spill
	mov	v4.16b, v12.16b
	smull	v0.4s, v10.4h, v24.4h
	smlal	v0.4s, v13.4h, v2.4h
	mov	v19.16b, v22.16b
	ext	v3.16b, v22.16b, v22.16b, #8
	str	q3, [sp, #928] // 16-byte Folded Spill
	smull	v2.4s, v27.4h, v26.4h
	smlal	v2.4s, v15.4h, v3.4h
	smlal	v0.4s, v20.4h, v1.4h
	ldr	q5, [sp, #1088] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v6.4h
	mov	v1.16b, v6.16b
	ldr	q24, [sp, #1040] // 16-byte Folded Reload
	smlal	v0.4s, v24.4h, v11.4h
	mov	v17.16b, v11.16b
	smlal	v2.4s, v31.4h, v30.4h
	mov	v16.16b, v30.16b
	ext	v3.16b, v13.16b, v13.16b, #8
	str	q3, [sp, #672] // 16-byte Folded Spill
	smlal	v0.4s, v3.4h, v4.4h
	ext	v3.16b, v15.16b, v15.16b, #8
	str	q3, [sp, #496] // 16-byte Folded Spill
	mov	v12.16b, v15.16b
	smlal	v2.4s, v3.4h, v19.4h
	uzp1	v3.8h, v0.8h, v2.8h
	mov	v15.16b, v21.16b
	mul	v3.8h, v3.8h, v21.8h
	smull2	v4.4s, v3.8h, v7.8h
	subhn	v2.4h, v2.4s, v4.4s
	smull	v3.4s, v3.4h, v7.4h
	subhn	v3.4h, v0.4s, v3.4s
	mov	v14.16b, v28.16b
	smull2	v0.4s, v20.8h, v28.8h
	mov	v18.16b, v20.16b
	ldr	q20, [sp, #1344] // 16-byte Folded Reload
	smlal2	v0.4s, v10.8h, v20.8h
	ldr	q13, [sp, #1328] // 16-byte Folded Reload
	smull2	v4.4s, v5.8h, v13.8h
	mov	v30.16b, v5.16b
	ldr	q19, [sp, #1424] // 16-byte Folded Reload
	smlal2	v4.4s, v27.8h, v19.8h
	mov	v11.16b, v27.16b
	ldr	q6, [sp, #1216] // 16-byte Folded Reload
	smlal2	v0.4s, v24.8h, v6.8h
	mov	v28.16b, v24.16b
	ldr	q29, [sp, #1280] // 16-byte Folded Reload
	smlal2	v4.4s, v31.8h, v29.8h
	ldr	q21, [sp, #1200] // 16-byte Folded Reload
	ext	v5.16b, v21.16b, v21.16b, #8
	str	q5, [sp, #592] // 16-byte Folded Spill
	mov	v24.16b, v9.16b
	smlal	v0.4s, v24.4h, v5.4h
	ldr	q31, [sp, #1264] // 16-byte Folded Reload
	ext	v5.16b, v31.16b, v31.16b, #8
	str	q5, [sp, #464] // 16-byte Folded Spill
	mov	v22.16b, v25.16b
	smlal	v4.4s, v22.4h, v5.4h
	smlal	v0.4s, v8.4h, v20.4h
	smlal	v4.4s, v23.4h, v19.4h
	ldr	q27, [sp, #1312] // 16-byte Folded Reload
	smlal	v0.4s, v27.4h, v14.4h
	ldr	q20, [sp, #1408] // 16-byte Folded Reload
	smlal	v4.4s, v20.4h, v13.4h
	ldr	q13, [sp, #1360] // 16-byte Folded Reload
	smlal	v0.4s, v13.4h, v6.4h
	ldr	q6, [sp, #1440] // 16-byte Folded Reload
	smlal	v4.4s, v6.4h, v29.4h
	ext	v5.16b, v9.16b, v9.16b, #8
	str	q5, [sp, #480] // 16-byte Folded Spill
	smlal	v0.4s, v5.4h, v21.4h
	ext	v5.16b, v25.16b, v25.16b, #8
	str	q5, [sp, #688] // 16-byte Folded Spill
	smlal	v4.4s, v5.4h, v31.4h
	ldr	q25, [sp, #976] // 16-byte Folded Reload
	smlal2	v0.4s, v8.8h, v25.8h
	mov	v14.16b, v8.16b
	mov	v19.16b, v26.16b
	smlal2	v4.4s, v23.8h, v26.8h
	mov	v8.16b, v23.16b
	ldr	q31, [sp, #1248] // 16-byte Folded Reload
	smlal2	v0.4s, v27.8h, v31.8h
	mov	v23.16b, v1.16b
	smlal2	v4.4s, v20.8h, v1.8h
	mov	v29.16b, v17.16b
	smlal2	v0.4s, v13.8h, v17.8h
	mov	v27.16b, v16.16b
	smlal2	v4.4s, v6.8h, v16.8h
	uzp1	v5.8h, v0.8h, v4.8h
	mul	v5.8h, v5.8h, v15.8h
	mov	v1.16b, v7.16b
	smull	v7.4s, v5.4h, v1.4h
	subhn	v7.4h, v0.4s, v7.4s
	smull2	v0.4s, v5.8h, v1.8h
	mov	v5.16b, v7.16b
	subhn2	v5.8h, v4.4s, v0.4s
	subhn	v4.4h, v4.4s, v0.4s
	ldr	d26, [sp, #1112] // 8-byte Folded Reload
	ldr	d0, [sp, #208] // 8-byte Folded Reload
	smull	v16.4s, v0.4h, v26.4h
	ldr	q20, [sp, #784] // 16-byte Folded Reload
	ldr	q0, [sp, #176] // 16-byte Folded Reload
	mul	v0.8h, v0.8h, v20.8h
	str	q0, [sp, #96] // 16-byte Folded Spill
	smull2	v17.4s, v0.8h, v1.8h
	subhn	v6.4h, v16.4s, v17.4s
	str	d6, [sp, #160] // 8-byte Folded Spill
	smull	v4.4s, v4.4h, v26.4h
	fmov	d0, d26
	mul	v5.8h, v5.8h, v20.8h
	smull2	v16.4s, v5.8h, v1.8h
	subhn	v4.4h, v4.4s, v16.4s
	ldr	d6, [sp, #1104] // 8-byte Folded Reload
	smull	v7.4s, v7.4h, v6.4h
	smull	v5.4s, v5.4h, v1.4h
	subhn	v5.4h, v7.4s, v5.4s
	add	v3.4h, v5.4h, v3.4h
	str	q3, [sp, #176] // 16-byte Folded Spill
	add	v2.4h, v4.4h, v2.4h
	str	q2, [sp, #208] // 16-byte Folded Spill
	ldp	q2, q20, [sp, #624] // 32-byte Folded Reload
	smull	v2.4s, v10.4h, v2.4h
	ldr	q21, [sp, #1136] // 16-byte Folded Reload
	smlal	v2.4s, v21.4h, v20.4h
	ldp	q26, q3, [sp, #912] // 32-byte Folded Reload
	smull	v3.4s, v11.4h, v3.4h
	smlal	v3.4s, v12.4h, v26.4h
	mov	v11.16b, v18.16b
	smlal	v2.4s, v11.4h, v25.4h
	mov	v13.16b, v25.16b
	mov	v10.16b, v30.16b
	smlal	v3.4s, v10.4h, v19.4h
	smlal	v2.4s, v28.4h, v31.4h
	mov	v16.16b, v31.16b
	ldr	q17, [sp, #1120] // 16-byte Folded Reload
	smlal	v3.4s, v17.4h, v23.4h
	mov	v6.16b, v23.16b
	ldr	q4, [sp, #672] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v29.4h
	ldr	q4, [sp, #496] // 16-byte Folded Reload
	smlal	v3.4s, v4.4h, v27.4h
	ldr	q4, [sp, #656] // 16-byte Folded Reload
	ldr	q5, [sp, #1232] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v5.4h
	ldr	q29, [sp, #448] // 16-byte Folded Reload
	ldr	q4, [sp, #1168] // 16-byte Folded Reload
	smlal	v3.4s, v29.4h, v4.4h
	uzp1	v4.8h, v2.8h, v3.8h
	mul	v4.8h, v4.8h, v15.8h
	smull2	v5.4s, v4.8h, v1.8h
	subhn	v18.4h, v3.4s, v5.4s
	smull	v4.4s, v4.4h, v1.4h
	subhn	v2.4h, v2.4s, v4.4s
	ldr	q7, [sp, #1392] // 16-byte Folded Reload
	smull2	v4.4s, v28.8h, v7.8h
	mov	v25.16b, v28.16b
	ldr	q30, [sp, #1344] // 16-byte Folded Reload
	smlal2	v4.4s, v11.8h, v30.8h
	ldr	q23, [sp, #1328] // 16-byte Folded Reload
	smull2	v5.4s, v17.8h, v23.8h
	ldr	q27, [sp, #1424] // 16-byte Folded Reload
	smlal2	v5.4s, v10.8h, v27.8h
	mov	v3.16b, v10.16b
	ldr	q9, [sp, #416] // 16-byte Folded Reload
	smlal	v4.4s, v24.4h, v9.4h
	ldr	q17, [sp, #528] // 16-byte Folded Reload
	smlal	v5.4s, v22.4h, v17.4h
	ldr	q17, [sp, #592] // 16-byte Folded Reload
	smlal	v4.4s, v14.4h, v17.4h
	ldr	q28, [sp, #464] // 16-byte Folded Reload
	smlal	v5.4s, v8.4h, v28.4h
	ldr	q8, [sp, #1312] // 16-byte Folded Reload
	smlal	v4.4s, v8.4h, v30.4h
	ldr	q31, [sp, #1408] // 16-byte Folded Reload
	smlal	v5.4s, v31.4h, v27.4h
	ldr	q17, [sp, #1360] // 16-byte Folded Reload
	smlal	v4.4s, v17.4h, v7.4h
	ldr	q12, [sp, #1440] // 16-byte Folded Reload
	smlal	v5.4s, v12.4h, v23.4h
	ldr	q30, [sp, #480] // 16-byte Folded Reload
	ldr	q7, [sp, #1216] // 16-byte Folded Reload
	smlal	v4.4s, v30.4h, v7.4h
	ldr	q23, [sp, #688] // 16-byte Folded Reload
	ldr	q7, [sp, #1280] // 16-byte Folded Reload
	smlal	v5.4s, v23.4h, v7.4h
	ldr	q7, [sp, #560] // 16-byte Folded Reload
	ldr	q22, [sp, #1200] // 16-byte Folded Reload
	smlal	v4.4s, v7.4h, v22.4h
	ldr	q7, [sp, #864] // 16-byte Folded Reload
	ldr	q22, [sp, #1264] // 16-byte Folded Reload
	smlal	v5.4s, v7.4h, v22.4h
	smlal2	v4.4s, v8.8h, v13.8h
	mov	v27.16b, v8.16b
	smlal2	v5.4s, v31.8h, v19.8h
	mov	v24.16b, v16.16b
	smlal2	v4.4s, v17.8h, v16.8h
	mov	v10.16b, v6.16b
	smlal2	v5.4s, v12.8h, v6.8h
	uzp1	v7.8h, v4.8h, v5.8h
	mul	v7.8h, v7.8h, v15.8h
	mov	v17.16b, v15.16b
	mov	v6.16b, v1.16b
	smull	v16.4s, v7.4h, v6.4h
	subhn	v4.4h, v4.4s, v16.4s
	smull2	v7.4s, v7.8h, v1.8h
	mov	v16.16b, v4.16b
	subhn2	v16.8h, v5.4s, v7.4s
	subhn	v5.4h, v5.4s, v7.4s
	smull	v5.4s, v5.4h, v0.4h
	ldr	q12, [sp, #784] // 16-byte Folded Reload
	mul	v7.8h, v16.8h, v12.8h
	smull2	v16.4s, v7.8h, v1.8h
	subhn	v5.4h, v5.4s, v16.4s
	ldr	d0, [sp, #1104] // 8-byte Folded Reload
	smull	v4.4s, v4.4h, v0.4h
	smull	v7.4s, v7.4h, v6.4h
	subhn	v4.4h, v4.4s, v7.4s
	add	v2.4h, v4.4h, v2.4h
	str	q2, [sp, #112] // 16-byte Folded Spill
	ldr	q2, [sp, #224] // 16-byte Folded Reload
	smull	v2.4s, v2.4h, v0.4h
	ldr	q0, [sp, #96] // 16-byte Folded Reload
	smull	v0.4s, v0.4h, v6.4h
	subhn	v0.4h, v2.4s, v0.4s
	str	d0, [sp, #80] // 8-byte Folded Spill
	add	v0.4h, v5.4h, v18.4h
	str	q0, [sp, #96] // 16-byte Folded Spill
	ldr	q0, [sp, #1072] // 16-byte Folded Reload
	smull	v0.4s, v0.4h, v20.4h
	ldr	q20, [sp, #400] // 16-byte Folded Reload
	smlal	v0.4s, v21.4h, v20.4h
	ldr	q21, [sp, #1024] // 16-byte Folded Reload
	smull	v2.4s, v21.4h, v26.4h
	ldr	q22, [sp, #432] // 16-byte Folded Reload
	ldr	q4, [sp, #1296] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v22.4h
	ldr	q1, [sp, #624] // 16-byte Folded Reload
	smlal	v0.4s, v11.4h, v1.4h
	ldr	q1, [sp, #928] // 16-byte Folded Reload
	smlal	v2.4s, v3.4h, v1.4h
	smlal	v0.4s, v25.4h, v13.4h
	mov	v26.16b, v13.16b
	ldr	q5, [sp, #1120] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v19.4h
	mov	v7.16b, v19.16b
	ldp	q15, q1, [sp, #656] // 32-byte Folded Reload
	smlal	v0.4s, v1.4h, v24.4h
	ldr	q14, [sp, #496] // 16-byte Folded Reload
	smlal	v2.4s, v14.4h, v10.4h
	ldr	q3, [sp, #960] // 16-byte Folded Reload
	smlal	v0.4s, v15.4h, v3.4h
	ldr	q3, [sp, #1008] // 16-byte Folded Reload
	smlal	v2.4s, v29.4h, v3.4h
	ldr	q11, [sp, #384] // 16-byte Folded Reload
	ldr	q3, [sp, #1232] // 16-byte Folded Reload
	smlal	v0.4s, v11.4h, v3.4h
	ldr	q29, [sp, #608] // 16-byte Folded Reload
	ldr	q3, [sp, #1168] // 16-byte Folded Reload
	smlal	v2.4s, v29.4h, v3.4h
	uzp1	v3.8h, v0.8h, v2.8h
	mov	v1.16b, v17.16b
	mul	v3.8h, v3.8h, v17.8h
	smull2	v4.4s, v3.8h, v6.8h
	subhn	v2.4h, v2.4s, v4.4s
	smull	v3.4s, v3.4h, v6.4h
	subhn	v0.4h, v0.4s, v3.4s
	ldr	q8, [sp, #800] // 16-byte Folded Reload
	ldr	q3, [sp, #1376] // 16-byte Folded Reload
	smull	v3.4s, v3.4h, v8.4h
	ldr	q17, [sp, #1344] // 16-byte Folded Reload
	smlal2	v3.4s, v25.8h, v17.8h
	ldr	q4, [sp, #576] // 16-byte Folded Reload
	ldr	q16, [sp, #1152] // 16-byte Folded Reload
	smull	v4.4s, v16.4h, v4.4h
	ldr	q16, [sp, #1424] // 16-byte Folded Reload
	smlal2	v4.4s, v5.8h, v16.8h
	mov	v10.16b, v5.16b
	ldr	q5, [sp, #1056] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v9.4h
	ldr	q19, [sp, #992] // 16-byte Folded Reload
	ldr	q5, [sp, #528] // 16-byte Folded Reload
	smlal	v4.4s, v19.4h, v5.4h
	ldr	q5, [sp, #592] // 16-byte Folded Reload
	smlal	v3.4s, v27.4h, v5.4h
	smlal	v4.4s, v31.4h, v28.4h
	ldr	q5, [sp, #1360] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v17.4h
	ldr	q28, [sp, #1440] // 16-byte Folded Reload
	smlal	v4.4s, v28.4h, v16.4h
	ldr	q16, [sp, #1392] // 16-byte Folded Reload
	smlal	v3.4s, v30.4h, v16.4h
	ldr	q16, [sp, #1328] // 16-byte Folded Reload
	smlal	v4.4s, v23.4h, v16.4h
	ldp	q30, q24, [sp, #544] // 32-byte Folded Reload
	ldr	q18, [sp, #1216] // 16-byte Folded Reload
	smlal	v3.4s, v24.4h, v18.4h
	ldr	q13, [sp, #864] // 16-byte Folded Reload
	ldr	q16, [sp, #1280] // 16-byte Folded Reload
	smlal	v4.4s, v13.4h, v16.4h
	ldr	q17, [sp, #1200] // 16-byte Folded Reload
	smlal	v3.4s, v30.4h, v17.4h
	ldr	q17, [sp, #848] // 16-byte Folded Reload
	ldr	q23, [sp, #1264] // 16-byte Folded Reload
	smlal	v4.4s, v17.4h, v23.4h
	smlal2	v3.4s, v5.8h, v26.8h
	mov	v27.16b, v7.16b
	smlal2	v4.4s, v28.8h, v7.8h
	uzp1	v5.8h, v3.8h, v4.8h
	mul	v5.8h, v5.8h, v1.8h
	smull	v7.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v7.4s
	smull2	v5.4s, v5.8h, v6.8h
	mov	v7.16b, v3.16b
	subhn2	v7.8h, v4.4s, v5.4s
	subhn	v4.4h, v4.4s, v5.4s
	ldr	d5, [sp, #240] // 8-byte Folded Reload
	ldr	d17, [sp, #192] // 8-byte Folded Reload
	add	v5.4h, v17.4h, v5.4h
	str	q5, [sp, #240] // 16-byte Folded Spill
	ldr	d31, [sp, #1112] // 8-byte Folded Reload
	smull	v4.4s, v4.4h, v31.4h
	mul	v5.8h, v7.8h, v12.8h
	smull2	v7.4s, v5.8h, v6.8h
	subhn	v4.4h, v4.4s, v7.4s
	ldr	d12, [sp, #1104] // 8-byte Folded Reload
	smull	v3.4s, v3.4h, v12.4h
	smull	v5.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v5.4s
	add	v0.4h, v3.4h, v0.4h
	str	q0, [sp, #32] // 16-byte Folded Spill
	add	v0.4h, v4.4h, v2.4h
	str	q0, [sp, #48] // 16-byte Folded Spill
	ldr	q0, [sp, #1072] // 16-byte Folded Reload
	smull	v0.4s, v0.4h, v20.4h
	ldr	q28, [sp, #832] // 16-byte Folded Reload
	ldr	q1, [sp, #1136] // 16-byte Folded Reload
	smlal	v0.4s, v1.4h, v28.4h
	smull	v2.4s, v21.4h, v22.4h
	ldr	q23, [sp, #752] // 16-byte Folded Reload
	ldr	q1, [sp, #1296] // 16-byte Folded Reload
	smlal	v2.4s, v1.4h, v23.4h
	ldr	q9, [sp, #704] // 16-byte Folded Reload
	ldp	q3, q1, [sp, #624] // 32-byte Folded Reload
	smlal	v0.4s, v9.4h, v1.4h
	ldr	q20, [sp, #1088] // 16-byte Folded Reload
	ldp	q4, q1, [sp, #912] // 32-byte Folded Reload
	smlal	v2.4s, v20.4h, v4.4h
	smlal	v0.4s, v25.4h, v3.4h
	smlal	v2.4s, v10.4h, v1.4h
	ldr	q1, [sp, #672] // 16-byte Folded Reload
	smlal	v0.4s, v1.4h, v26.4h
	smlal	v2.4s, v14.4h, v27.4h
	ldr	q1, [sp, #1248] // 16-byte Folded Reload
	smlal	v0.4s, v15.4h, v1.4h
	ldr	q10, [sp, #448] // 16-byte Folded Reload
	ldr	q1, [sp, #1184] // 16-byte Folded Reload
	smlal	v2.4s, v10.4h, v1.4h
	ldp	q15, q1, [sp, #944] // 32-byte Folded Reload
	smlal	v0.4s, v11.4h, v1.4h
	ldr	q1, [sp, #1008] // 16-byte Folded Reload
	smlal	v2.4s, v29.4h, v1.4h
	ldp	q27, q29, [sp, #720] // 32-byte Folded Reload
	ldr	q1, [sp, #1232] // 16-byte Folded Reload
	smlal	v0.4s, v27.4h, v1.4h
	ldr	q1, [sp, #1168] // 16-byte Folded Reload
	smlal	v2.4s, v29.4h, v1.4h
	uzp1	v3.8h, v0.8h, v2.8h
	mul	v3.8h, v3.8h, v15.8h
	smull2	v4.4s, v3.8h, v6.8h
	subhn	v2.4h, v2.4s, v4.4s
	smull	v3.4s, v3.4h, v6.4h
	subhn	v0.4h, v0.4s, v3.4s
	ldr	q1, [sp, #1056] // 16-byte Folded Reload
	smull	v3.4s, v1.4h, v8.4h
	ldr	q8, [sp, #768] // 16-byte Folded Reload
	ldr	q1, [sp, #1376] // 16-byte Folded Reload
	smlal	v3.4s, v1.4h, v8.4h
	ldr	q21, [sp, #576] // 16-byte Folded Reload
	smull	v4.4s, v19.4h, v21.4h
	ldr	q11, [sp, #896] // 16-byte Folded Reload
	ldr	q1, [sp, #1152] // 16-byte Folded Reload
	smlal	v4.4s, v1.4h, v11.4h
	ldr	q26, [sp, #416] // 16-byte Folded Reload
	ldr	q17, [sp, #1312] // 16-byte Folded Reload
	smlal	v3.4s, v17.4h, v26.4h
	ldr	q25, [sp, #528] // 16-byte Folded Reload
	ldr	q7, [sp, #1408] // 16-byte Folded Reload
	smlal	v4.4s, v7.4h, v25.4h
	ldr	q1, [sp, #592] // 16-byte Folded Reload
	ldr	q5, [sp, #1360] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v1.4h
	ldr	q1, [sp, #464] // 16-byte Folded Reload
	ldr	q5, [sp, #1440] // 16-byte Folded Reload
	smlal	v4.4s, v5.4h, v1.4h
	ldr	q5, [sp, #480] // 16-byte Folded Reload
	ldr	q1, [sp, #1344] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v1.4h
	ldr	q1, [sp, #1424] // 16-byte Folded Reload
	ldr	q5, [sp, #688] // 16-byte Folded Reload
	smlal	v4.4s, v5.4h, v1.4h
	ldr	q1, [sp, #1392] // 16-byte Folded Reload
	smlal	v3.4s, v24.4h, v1.4h
	ldr	q14, [sp, #1328] // 16-byte Folded Reload
	smlal	v4.4s, v13.4h, v14.4h
	smlal	v3.4s, v30.4h, v18.4h
	ldr	q19, [sp, #848] // 16-byte Folded Reload
	smlal	v4.4s, v19.4h, v16.4h
	ldr	q30, [sp, #512] // 16-byte Folded Reload
	ldr	q1, [sp, #1200] // 16-byte Folded Reload
	smlal	v3.4s, v30.4h, v1.4h
	ldr	q22, [sp, #816] // 16-byte Folded Reload
	ldr	q24, [sp, #1264] // 16-byte Folded Reload
	smlal	v4.4s, v22.4h, v24.4h
	uzp1	v5.8h, v3.8h, v4.8h
	mul	v5.8h, v5.8h, v15.8h
	smull	v16.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v16.4s
	smull2	v5.4s, v5.8h, v6.8h
	mov	v16.16b, v3.16b
	subhn2	v16.8h, v4.4s, v5.4s
	subhn	v4.4h, v4.4s, v5.4s
	smull	v4.4s, v4.4h, v31.4h
	ldr	q15, [sp, #784] // 16-byte Folded Reload
	mul	v5.8h, v16.8h, v15.8h
	smull2	v16.4s, v5.8h, v6.8h
	subhn	v4.4h, v4.4s, v16.4s
	ldr	d16, [sp, #336] // 8-byte Folded Reload
	ldr	d31, [sp, #320] // 8-byte Folded Reload
	add	v16.4h, v31.4h, v16.4h
	str	q16, [sp, #224] // 16-byte Folded Spill
	smull	v3.4s, v3.4h, v12.4h
	smull	v5.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v5.4s
	add	v0.4h, v3.4h, v0.4h
	str	q0, [sp, #320] // 16-byte Folded Spill
	add	v0.4h, v4.4h, v2.4h
	str	q0, [sp, #336] // 16-byte Folded Spill
	ldr	q0, [sp, #1072] // 16-byte Folded Reload
	smull	v0.4s, v0.4h, v28.4h
	mov	v5.16b, v1.16b
	ldr	q4, [sp, #1136] // 16-byte Folded Reload
	smlal	v0.4s, v4.4h, v5.4h
	ldr	q1, [sp, #1024] // 16-byte Folded Reload
	smull	v2.4s, v1.4h, v23.4h
	ldr	q23, [sp, #1296] // 16-byte Folded Reload
	smlal	v2.4s, v23.4h, v24.4h
	mov	v16.16b, v24.16b
	ldr	q18, [sp, #400] // 16-byte Folded Reload
	smlal	v0.4s, v9.4h, v18.4h
	mov	v13.16b, v9.16b
	ldr	q31, [sp, #432] // 16-byte Folded Reload
	smlal	v2.4s, v20.4h, v31.4h
	ldr	q24, [sp, #1040] // 16-byte Folded Reload
	ldr	q3, [sp, #640] // 16-byte Folded Reload
	smlal	v0.4s, v24.4h, v3.4h
	ldr	q1, [sp, #912] // 16-byte Folded Reload
	ldr	q3, [sp, #1120] // 16-byte Folded Reload
	smlal	v2.4s, v3.4h, v1.4h
	ldr	q3, [sp, #1232] // 16-byte Folded Reload
	smlal2	v0.4s, v4.8h, v3.8h
	ldr	q28, [sp, #1168] // 16-byte Folded Reload
	smlal2	v2.4s, v23.8h, v28.8h
	ldr	q4, [sp, #656] // 16-byte Folded Reload
	ldr	q1, [sp, #976] // 16-byte Folded Reload
	smlal	v0.4s, v4.4h, v1.4h
	ldr	q1, [sp, #880] // 16-byte Folded Reload
	smlal	v2.4s, v10.4h, v1.4h
	ldr	q1, [sp, #384] // 16-byte Folded Reload
	ldr	q4, [sp, #1248] // 16-byte Folded Reload
	smlal	v0.4s, v1.4h, v4.4h
	ldr	q4, [sp, #608] // 16-byte Folded Reload
	ldr	q20, [sp, #1184] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v20.4h
	ldp	q10, q4, [sp, #944] // 32-byte Folded Reload
	smlal	v0.4s, v27.4h, v4.4h
	ldr	q27, [sp, #1008] // 16-byte Folded Reload
	smlal	v2.4s, v29.4h, v27.4h
	ldr	q29, [sp, #1376] // 16-byte Folded Reload
	smlal	v0.4s, v29.4h, v3.4h
	ldr	q23, [sp, #1152] // 16-byte Folded Reload
	smlal	v2.4s, v23.4h, v28.4h
	uzp1	v3.8h, v0.8h, v2.8h
	mul	v3.8h, v3.8h, v10.8h
	smull2	v4.4s, v3.8h, v6.8h
	subhn	v2.4h, v2.4s, v4.4s
	smull	v3.4s, v3.4h, v6.4h
	subhn	v0.4h, v0.4s, v3.4s
	ldr	q3, [sp, #800] // 16-byte Folded Reload
	smull	v3.4s, v17.4h, v3.4h
	ldr	q17, [sp, #1056] // 16-byte Folded Reload
	smlal	v3.4s, v17.4h, v8.4h
	smull	v4.4s, v7.4h, v21.4h
	ldr	q7, [sp, #992] // 16-byte Folded Reload
	smlal	v4.4s, v7.4h, v11.4h
	ldr	q20, [sp, #1360] // 16-byte Folded Reload
	smlal	v3.4s, v20.4h, v26.4h
	ldr	q20, [sp, #1440] // 16-byte Folded Reload
	smlal	v4.4s, v20.4h, v25.4h
	smlal2	v3.4s, v29.8h, v5.8h
	mov	v8.16b, v5.16b
	mov	v29.16b, v16.16b
	smlal2	v4.4s, v23.8h, v16.8h
	ldr	q5, [sp, #560] // 16-byte Folded Reload
	ldr	q16, [sp, #1344] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v16.4h
	ldr	q26, [sp, #1424] // 16-byte Folded Reload
	ldr	q5, [sp, #864] // 16-byte Folded Reload
	smlal	v4.4s, v5.4h, v26.4h
	ldr	q5, [sp, #544] // 16-byte Folded Reload
	ldr	q16, [sp, #1392] // 16-byte Folded Reload
	smlal	v3.4s, v5.4h, v16.4h
	smlal	v4.4s, v19.4h, v14.4h
	ldr	q9, [sp, #1216] // 16-byte Folded Reload
	smlal	v3.4s, v30.4h, v9.4h
	ldr	q25, [sp, #1280] // 16-byte Folded Reload
	smlal	v4.4s, v22.4h, v25.4h
	uzp1	v5.8h, v3.8h, v4.8h
	mul	v5.8h, v5.8h, v10.8h
	mov	v19.16b, v10.16b
	smull	v16.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v16.4s
	smull2	v5.4s, v5.8h, v6.8h
	mov	v16.16b, v3.16b
	subhn2	v16.8h, v4.4s, v5.4s
	subhn	v4.4h, v4.4s, v5.4s
	ldr	d5, [sp, #1112] // 8-byte Folded Reload
	smull	v4.4s, v4.4h, v5.4h
	mul	v5.8h, v16.8h, v15.8h
	mov	v10.16b, v15.16b
	smull2	v16.4s, v5.8h, v6.8h
	subhn	v4.4h, v4.4s, v16.4s
	smull	v3.4s, v3.4h, v12.4h
	smull	v5.4s, v5.4h, v6.4h
	subhn	v3.4h, v3.4s, v5.4s
	add	v0.4h, v3.4h, v0.4h
	ldr	d3, [sp, #352] // 8-byte Folded Reload
	ldr	d5, [sp, #80] // 8-byte Folded Reload
	add	v3.4h, v5.4h, v3.4h
	str	q3, [sp, #192] // 16-byte Folded Spill
	add	v2.4h, v4.4h, v2.4h
	movi	v3.4h, #19
	smull	v14.4s, v0.4h, v3.4h
	mov	v0.d[1], v2.d[0]
	smull	v12.4s, v2.4h, v3.4h
	movi	v21.4h, #19
	ldr	q2, [sp, #288] // 16-byte Folded Reload
	mul	v23.8h, v0.8h, v2.8h
	mov	v11.16b, v2.16b
	smull	v0.4s, v23.4h, v6.4h
	subhn	v2.4h, v14.4s, v0.4s
	smull2	v0.4s, v23.8h, v6.8h
	subhn2	v2.8h, v12.4s, v0.4s
	str	q2, [sp, #16] // 16-byte Folded Spill
	ldr	q22, [sp, #1072] // 16-byte Folded Reload
	smull	v0.4s, v22.4h, v8.4h
	mov	v15.16b, v8.16b
	ldr	q16, [sp, #1136] // 16-byte Folded Reload
	smlal	v0.4s, v16.4h, v9.4h
	mov	v30.16b, v9.16b
	ldr	q8, [sp, #1024] // 16-byte Folded Reload
	smull	v2.4s, v8.4h, v29.4h
	ldr	q4, [sp, #1296] // 16-byte Folded Reload
	smlal	v2.4s, v4.4h, v25.4h
	mov	v9.16b, v25.16b
	ldr	q3, [sp, #832] // 16-byte Folded Reload
	smlal	v0.4s, v13.4h, v3.4h
	ldr	q5, [sp, #1088] // 16-byte Folded Reload
	ldr	q3, [sp, #752] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v3.4h
	smlal	v0.4s, v24.4h, v18.4h
	ldr	q3, [sp, #1120] // 16-byte Folded Reload
	smlal	v2.4s, v3.4h, v31.4h
	ldp	q24, q3, [sp, #960] // 32-byte Folded Reload
	smlal2	v0.4s, v16.8h, v24.8h
	smlal2	v2.4s, v4.8h, v27.8h
	ldr	q18, [sp, #1232] // 16-byte Folded Reload
	smlal2	v0.4s, v22.8h, v18.8h
	mov	v5.16b, v28.16b
	smlal2	v2.4s, v8.8h, v28.8h
	smlal	v0.4s, v1.4h, v3.4h
	ldr	q3, [sp, #608] // 16-byte Folded Reload
	ldr	q4, [sp, #880] // 16-byte Folded Reload
	smlal	v2.4s, v3.4h, v4.4h
	ldr	q1, [sp, #720] // 16-byte Folded Reload
	ldr	q3, [sp, #1248] // 16-byte Folded Reload
	smlal	v0.4s, v1.4h, v3.4h
	ldr	q1, [sp, #1184] // 16-byte Folded Reload
	ldr	q3, [sp, #736] // 16-byte Folded Reload
	smlal	v2.4s, v3.4h, v1.4h
	ldr	q25, [sp, #1376] // 16-byte Folded Reload
	smlal	v0.4s, v25.4h, v24.4h
	ldr	q22, [sp, #1152] // 16-byte Folded Reload
	smlal	v2.4s, v22.4h, v27.4h
	mov	v31.16b, v17.16b
	smlal	v0.4s, v31.4h, v18.4h
	smlal	v2.4s, v7.4h, v5.4h
	uzp1	v3.8h, v0.8h, v2.8h
	mov	v27.16b, v19.16b
	mul	v3.8h, v3.8h, v19.8h
	smull2	v5.4s, v3.8h, v6.8h
	subhn	v16.4h, v2.4s, v5.4s
	ldr	d2, [sp, #368] // 8-byte Folded Reload
	ldr	d1, [sp, #160] // 8-byte Folded Reload
	add	v2.4h, v1.4h, v2.4h
	str	q2, [sp, #160] // 16-byte Folded Spill
	smull	v2.4s, v3.4h, v6.4h
	subhn	v19.4h, v0.4s, v2.4s
	ldp	q18, q1, [sp, #320] // 32-byte Folded Reload
	smull	v5.4s, v18.4h, v21.4h
	mov	v18.d[1], v1.d[0]
	mul	v3.8h, v18.8h, v11.8h
	smull	v2.4s, v3.4h, v6.4h
	subhn	v0.4h, v5.4s, v2.4s
	smull	v4.4s, v1.4h, v21.4h
	smull2	v17.4s, v3.8h, v6.8h
	subhn2	v0.8h, v4.4s, v17.4s
	str	q0, [sp, #64] // 16-byte Folded Spill
	ldr	q0, [sp, #800] // 16-byte Folded Reload
	ldr	q1, [sp, #1360] // 16-byte Folded Reload
	smull	v17.4s, v1.4h, v0.4h
	ldr	q0, [sp, #768] // 16-byte Folded Reload
	ldr	q1, [sp, #1312] // 16-byte Folded Reload
	smlal	v17.4s, v1.4h, v0.4h
	ldr	q0, [sp, #576] // 16-byte Folded Reload
	smull	v18.4s, v20.4h, v0.4h
	ldr	q0, [sp, #1408] // 16-byte Folded Reload
	ldr	q1, [sp, #896] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v1.4h
	mov	v20.16b, v30.16b
	smlal2	v17.4s, v25.8h, v30.8h
	smlal2	v18.4s, v22.8h, v9.8h
	mov	v28.16b, v9.16b
	mov	v22.16b, v15.16b
	smlal2	v17.4s, v31.8h, v15.8h
	mov	v13.16b, v29.16b
	smlal2	v18.4s, v7.8h, v29.8h
	ldr	q0, [sp, #544] // 16-byte Folded Reload
	ldr	q1, [sp, #1344] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v1.4h
	ldr	q0, [sp, #848] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v26.4h
	ldr	q31, [sp, #1392] // 16-byte Folded Reload
	ldr	q0, [sp, #512] // 16-byte Folded Reload
	smlal	v17.4s, v0.4h, v31.4h
	ldr	q25, [sp, #1328] // 16-byte Folded Reload
	ldr	q0, [sp, #816] // 16-byte Folded Reload
	smlal	v18.4s, v0.4h, v25.4h
	uzp1	v7.8h, v17.8h, v18.8h
	mul	v7.8h, v7.8h, v27.8h
	smull	v26.4s, v7.4h, v6.4h
	subhn	v0.4h, v17.4s, v26.4s
	smull2	v7.4s, v7.8h, v6.8h
	mov	v26.16b, v0.16b
	subhn2	v26.8h, v18.4s, v7.4s
	subhn	v2.4h, v18.4s, v7.4s
	ldp	q7, q1, [sp, #32] // 32-byte Folded Reload
	smull	v8.4s, v7.4h, v21.4h
	mov	v7.d[1], v1.d[0]
	mul	v29.8h, v7.8h, v11.8h
	smull	v18.4s, v29.4h, v6.4h
	subhn	v24.4h, v8.4s, v18.4s
	smull	v27.4s, v1.4h, v21.4h
	smull2	v30.4s, v29.8h, v6.8h
	subhn2	v24.8h, v27.4s, v30.4s
	str	q24, [sp, #80] // 16-byte Folded Spill
	ldr	d15, [sp, #1112] // 8-byte Folded Reload
	smull	v2.4s, v2.4h, v15.4h
	mov	v9.16b, v10.16b
	mul	v26.8h, v26.8h, v10.8h
	smull2	v30.4s, v26.8h, v6.8h
	subhn	v2.4h, v2.4s, v30.4s
	ldr	d10, [sp, #1104] // 8-byte Folded Reload
	smull	v0.4s, v0.4h, v10.4h
	smull	v26.4s, v26.4h, v6.4h
	subhn	v0.4h, v0.4s, v26.4s
	add	v1.4h, v0.4h, v19.4h
	ldp	q7, q19, [sp, #96] // 32-byte Folded Reload
	smull	v0.4s, v19.4h, v21.4h
	str	q0, [sp, #336] // 16-byte Folded Spill
	mov	v19.d[1], v7.d[0]
	mul	v24.8h, v19.8h, v11.8h
	smull	v26.4s, v24.4h, v6.4h
	subhn	v30.4h, v0.4s, v26.4s
	smull	v0.4s, v7.4h, v21.4h
	str	q0, [sp, #320] // 16-byte Folded Spill
	smull2	v26.4s, v24.8h, v6.8h
	subhn2	v30.8h, v0.4s, v26.4s
	str	q30, [sp, #96] // 16-byte Folded Spill
	add	v2.4h, v2.4h, v16.4h
	smull	v16.4s, v1.4h, v21.4h
	mov	v1.d[1], v2.d[0]
	mul	v0.8h, v1.8h, v11.8h
	smull	v26.4s, v0.4h, v6.4h
	subhn	v26.4h, v16.4s, v26.4s
	smull	v2.4s, v2.4h, v21.4h
	smull2	v30.4s, v0.8h, v6.8h
	subhn2	v26.8h, v2.4s, v30.4s
	mov	v1.16b, v6.16b
	smlsl	v5.4s, v3.4h, v1.4h
	smlsl2	v4.4s, v3.8h, v6.8h
	smlsl	v14.4s, v23.4h, v1.4h
	smlsl2	v12.4s, v23.8h, v6.8h
	cmlt	v3.4s, v12.4s, #0
	cmlt	v6.4s, v14.4s, #0
	uzp1	v23.8h, v6.8h, v3.8h
	smlsl	v16.4s, v0.4h, v1.4h
	smlsl2	v2.4s, v0.8h, v1.8h
	ldr	q0, [sp, #176] // 16-byte Folded Reload
	smull	v6.4s, v0.4h, v21.4h
	str	q6, [sp, #352] // 16-byte Folded Spill
	ldr	q30, [sp, #208] // 16-byte Folded Reload
	mov	v0.d[1], v30.d[0]
	mul	v19.8h, v0.8h, v11.8h
	mov	v14.16b, v11.16b
	smull	v3.4s, v19.4h, v1.4h
	subhn	v6.4h, v6.4s, v3.4s
	smull	v0.4s, v30.4h, v21.4h
	str	q0, [sp, #368] // 16-byte Folded Spill
	smull2	v3.4s, v19.8h, v1.8h
	subhn2	v6.8h, v0.4s, v3.4s
	str	q6, [sp, #112] // 16-byte Folded Spill
	cmlt	v2.4s, v2.4s, #0
	cmlt	v3.4s, v16.4s, #0
	uzp1	v2.8h, v3.8h, v2.8h
	cmlt	v3.4s, v4.4s, #0
	cmlt	v4.4s, v5.4s, #0
	uzp1	v0.8h, v4.8h, v3.8h
	str	q0, [sp, #48] // 16-byte Folded Spill
	and	v2.16b, v2.16b, v1.16b
	add	v2.8h, v2.8h, v26.8h
	str	q2, [sp, #208] // 16-byte Folded Spill
	ldr	q26, [sp, #1072] // 16-byte Folded Reload
	smull	v2.4s, v26.4h, v20.4h
	mov	v18.16b, v20.16b
	ldr	q6, [sp, #1136] // 16-byte Folded Reload
	smlal	v2.4s, v6.4h, v31.4h
	mov	v12.16b, v31.16b
	ldr	q31, [sp, #1024] // 16-byte Folded Reload
	mov	v20.16b, v28.16b
	smull	v3.4s, v31.4h, v20.4h
	ldr	q16, [sp, #1296] // 16-byte Folded Reload
	smlal	v3.4s, v16.4h, v25.4h
	mov	v0.16b, v25.16b
	ldr	q21, [sp, #704] // 16-byte Folded Reload
	smlal	v2.4s, v21.4h, v22.4h
	mov	v17.16b, v22.16b
	ldr	q22, [sp, #1088] // 16-byte Folded Reload
	smlal	v3.4s, v22.4h, v13.4h
	mov	v7.16b, v13.16b
	ldr	q4, [sp, #832] // 16-byte Folded Reload
	ldr	q5, [sp, #1040] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v4.4h
	ldr	q4, [sp, #1120] // 16-byte Folded Reload
	ldr	q5, [sp, #752] // 16-byte Folded Reload
	smlal	v3.4s, v4.4h, v5.4h
	ldr	q4, [sp, #1248] // 16-byte Folded Reload
	smlal2	v2.4s, v6.8h, v4.8h
	ldr	q11, [sp, #1184] // 16-byte Folded Reload
	smlal2	v3.4s, v16.8h, v11.8h
	ldp	q30, q6, [sp, #960] // 32-byte Folded Reload
	smlal2	v2.4s, v26.8h, v30.8h
	ldp	q16, q13, [sp, #992] // 32-byte Folded Reload
	smlal2	v3.4s, v31.8h, v13.8h
	ldr	q31, [sp, #1232] // 16-byte Folded Reload
	smlal2	v2.4s, v21.8h, v31.8h
	ldr	q28, [sp, #1168] // 16-byte Folded Reload
	smlal2	v3.4s, v22.8h, v28.8h
	ldr	q5, [sp, #720] // 16-byte Folded Reload
	smlal	v2.4s, v5.4h, v6.4h
	ldr	q5, [sp, #880] // 16-byte Folded Reload
	ldr	q6, [sp, #736] // 16-byte Folded Reload
	smlal	v3.4s, v6.4h, v5.4h
	ldr	q25, [sp, #1376] // 16-byte Folded Reload
	smlal	v2.4s, v25.4h, v4.4h
	ldr	q6, [sp, #1152] // 16-byte Folded Reload
	smlal	v3.4s, v6.4h, v11.4h
	ldr	q26, [sp, #1056] // 16-byte Folded Reload
	smlal	v2.4s, v26.4h, v30.4h
	mov	v21.16b, v30.16b
	smlal	v3.4s, v16.4h, v13.4h
	ldr	q11, [sp, #1312] // 16-byte Folded Reload
	smlal	v2.4s, v11.4h, v31.4h
	ldr	q30, [sp, #1408] // 16-byte Folded Reload
	smlal	v3.4s, v30.4h, v28.4h
	uzp1	v4.8h, v2.8h, v3.8h
	ldr	q22, [sp, #944] // 16-byte Folded Reload
	mul	v4.8h, v4.8h, v22.8h
	smull2	v5.4s, v4.8h, v1.8h
	subhn	v3.4h, v3.4s, v5.4s
	and	v5.16b, v23.16b, v1.16b
	ldr	q23, [sp, #16] // 16-byte Folded Reload
	add	v5.8h, v5.8h, v23.8h
	str	q5, [sp, #176] // 16-byte Folded Spill
	smull	v4.4s, v4.4h, v1.4h
	subhn	v2.4h, v2.4s, v4.4s
	ldr	q31, [sp, #1360] // 16-byte Folded Reload
	ldr	q4, [sp, #768] // 16-byte Folded Reload
	smull	v4.4s, v31.4h, v4.4h
	smlal2	v4.4s, v25.8h, v12.8h
	ldr	q5, [sp, #896] // 16-byte Folded Reload
	ldr	q23, [sp, #1440] // 16-byte Folded Reload
	smull	v5.4s, v23.4h, v5.4h
	smlal2	v5.4s, v6.8h, v0.8h
	smlal2	v4.4s, v26.8h, v18.8h
	mov	v13.16b, v26.16b
	smlal2	v5.4s, v16.8h, v20.8h
	smlal2	v4.4s, v11.8h, v17.8h
	mov	v17.16b, v11.16b
	smlal2	v5.4s, v30.8h, v7.8h
	ldr	q26, [sp, #512] // 16-byte Folded Reload
	ldr	q18, [sp, #1344] // 16-byte Folded Reload
	smlal	v4.4s, v26.4h, v18.4h
	ldr	q6, [sp, #1424] // 16-byte Folded Reload
	ldr	q16, [sp, #816] // 16-byte Folded Reload
	smlal	v5.4s, v16.4h, v6.4h
	uzp1	v6.8h, v4.8h, v5.8h
	mul	v6.8h, v6.8h, v22.8h
	smull	v16.4s, v6.4h, v1.4h
	subhn	v4.4h, v4.4s, v16.4s
	smull2	v6.4s, v6.8h, v1.8h
	mov	v16.16b, v4.16b
	subhn2	v16.8h, v5.4s, v6.4s
	subhn	v5.4h, v5.4s, v6.4s
	ldp	q6, q22, [sp, #128] // 32-byte Folded Reload
	movi	v23.4h, #19
	smull	v0.4s, v6.4h, v23.4h
	mov	v6.d[1], v22.d[0]
	mul	v6.8h, v6.8h, v14.8h
	smull	v20.4s, v6.4h, v1.4h
	subhn	v28.4h, v0.4s, v20.4s
	mov	v30.16b, v0.16b
	smull	v0.4s, v22.4h, v23.4h
	smull2	v20.4s, v6.8h, v1.8h
	subhn2	v28.8h, v0.4s, v20.4s
	mov	v22.16b, v0.16b
	str	q28, [sp, #128] // 16-byte Folded Spill
	smull	v5.4s, v5.4h, v15.4h
	mul	v16.8h, v16.8h, v9.8h
	smull2	v20.4s, v16.8h, v1.8h
	subhn	v5.4h, v5.4s, v20.4s
	smull	v4.4s, v4.4h, v10.4h
	smull	v16.4s, v16.4h, v1.4h
	subhn	v4.4h, v4.4s, v16.4s
	add	v2.4h, v4.4h, v2.4h
	add	v3.4h, v5.4h, v3.4h
	mov	v4.16b, v2.16b
	mov	v4.d[1], v3.d[0]
	movi	v0.4h, #19
	smull	v2.4s, v2.4h, v0.4h
	mul	v4.8h, v4.8h, v14.8h
	smull	v5.4s, v4.4h, v1.4h
	subhn	v5.4h, v2.4s, v5.4s
	smull	v3.4s, v3.4h, v0.4h
	smull2	v16.4s, v4.8h, v1.8h
	subhn2	v5.8h, v3.4s, v16.4s
	smlsl	v8.4s, v29.4h, v1.4h
	smlsl2	v27.4s, v29.8h, v1.8h
	cmlt	v7.4s, v27.4s, #0
	cmlt	v16.4s, v8.4s, #0
	uzp1	v0.8h, v16.8h, v7.8h
	str	q0, [sp] // 16-byte Folded Spill
	ldr	q0, [sp, #352] // 16-byte Folded Reload
	smlsl	v0.4s, v19.4h, v1.4h
	str	q0, [sp, #352] // 16-byte Folded Spill
	ldr	q0, [sp, #368] // 16-byte Folded Reload
	smlsl2	v0.4s, v19.8h, v1.8h
	str	q0, [sp, #368] // 16-byte Folded Spill
	smlsl	v2.4s, v4.4h, v1.4h
	smlsl2	v3.4s, v4.8h, v1.8h
	smlsl	v30.4s, v6.4h, v1.4h
	smlsl2	v22.4s, v6.8h, v1.8h
	stp	q22, q30, [sp, #16] // 32-byte Folded Spill
	cmlt	v0.4s, v3.4s, #0
	cmlt	v2.4s, v2.4s, #0
	uzp1	v0.8h, v2.8h, v0.8h
	ldr	q2, [sp, #336] // 16-byte Folded Reload
	smlsl	v2.4s, v24.4h, v1.4h
	str	q2, [sp, #336] // 16-byte Folded Spill
	ldr	q2, [sp, #320] // 16-byte Folded Reload
	smlsl2	v2.4s, v24.8h, v1.8h
	str	q2, [sp, #320] // 16-byte Folded Spill
	and	v0.16b, v0.16b, v1.16b
	add	v0.8h, v0.8h, v5.8h
	str	q0, [sp, #144] // 16-byte Folded Spill
	ldp	q12, q11, [sp, #544] // 32-byte Folded Reload
	smull	v7.4s, v12.4h, v21.4h
	ldr	q15, [sp, #1232] // 16-byte Folded Reload
	smlal	v7.4s, v26.4h, v15.4h
	ldr	q6, [sp, #1248] // 16-byte Folded Reload
	smlal	v7.4s, v11.4h, v6.4h
	ldr	q28, [sp, #976] // 16-byte Folded Reload
	ldr	q10, [sp, #480] // 16-byte Folded Reload
	smlal	v7.4s, v10.4h, v28.4h
	ldp	q27, q25, [sp, #624] // 32-byte Folded Reload
	smlal	v7.4s, v31.4h, v27.4h
	mov	v26.16b, v17.16b
	smlal	v7.4s, v26.4h, v25.4h
	ldp	q19, q23, [sp, #384] // 32-byte Folded Reload
	mov	v30.16b, v13.16b
	smlal	v7.4s, v30.4h, v23.4h
	ldr	q0, [sp, #832] // 16-byte Folded Reload
	ldr	q22, [sp, #1376] // 16-byte Folded Reload
	smlal	v7.4s, v22.4h, v0.4h
	ldr	q29, [sp, #1200] // 16-byte Folded Reload
	ldr	q0, [sp, #720] // 16-byte Folded Reload
	smlal	v7.4s, v0.4h, v29.4h
	ldr	q3, [sp, #1072] // 16-byte Folded Reload
	ldr	q2, [sp, #592] // 16-byte Folded Reload
	smull	v17.4s, v3.4h, v2.4h
	ldr	q5, [sp, #1136] // 16-byte Folded Reload
	ldr	q0, [sp, #416] // 16-byte Folded Reload
	smlal	v17.4s, v5.4h, v0.4h
	ldr	q13, [sp, #1216] // 16-byte Folded Reload
	smlal	v7.4s, v19.4h, v13.4h
	ldr	q24, [sp, #1392] // 16-byte Folded Reload
	ldp	q20, q9, [sp, #656] // 32-byte Folded Reload
	smlal	v7.4s, v20.4h, v24.4h
	mov	v14.16b, v18.16b
	smlal	v7.4s, v9.4h, v14.4h
	ldr	q4, [sp, #1040] // 16-byte Folded Reload
	smlal	v7.4s, v4.4h, v2.4h
	mov	v8.16b, v2.16b
	ldr	q2, [sp, #704] // 16-byte Folded Reload
	smlal	v7.4s, v2.4h, v0.4h
	smull	v16.4s, v3.4h, v0.4h
	ldr	q0, [sp, #800] // 16-byte Folded Reload
	smlal	v16.4s, v5.4h, v0.4h
	smlal	v7.4s, v3.4h, v0.4h
	ldr	q0, [sp, #768] // 16-byte Folded Reload
	smlal	v7.4s, v5.4h, v0.4h
	smull	v0.4s, v3.4h, v24.4h
	smlal	v0.4s, v5.4h, v14.4h
	smlal	v0.4s, v2.4h, v13.4h
	smlal	v0.4s, v4.4h, v29.4h
	smlal2	v0.4s, v5.8h, v28.8h
	smull	v18.4s, v3.4h, v14.4h
	smlal	v18.4s, v5.4h, v8.4h
	smlal	v18.4s, v2.4h, v24.4h
	smlal2	v0.4s, v3.8h, v6.8h
	smlal	v18.4s, v4.4h, v13.4h
	smlal	v18.4s, v9.4h, v29.4h
	smlal2	v18.4s, v3.8h, v28.8h
	smlal	v16.4s, v2.4h, v8.4h
	smlal2	v0.4s, v2.8h, v21.8h
	smlal2	v18.4s, v2.8h, v6.8h
	smlal	v17.4s, v2.4h, v14.4h
	smlal	v17.4s, v4.4h, v24.4h
	smlal	v17.4s, v9.4h, v13.4h
	mov	v3.16b, v20.16b
	smlal	v17.4s, v3.4h, v29.4h
	smlal2	v17.4s, v2.8h, v28.8h
	smlal	v16.4s, v4.4h, v14.4h
	smlal	v16.4s, v9.4h, v24.4h
	smlal	v16.4s, v3.4h, v13.4h
	smlal	v16.4s, v19.4h, v29.4h
	ldp	q2, q3, [sp, #320] // 32-byte Folded Reload
	cmlt	v2.4s, v2.4s, #0
	cmlt	v3.4s, v3.4s, #0
	uzp1	v2.8h, v3.8h, v2.8h
	str	q2, [sp, #832] // 16-byte Folded Spill
	smlal2	v0.4s, v4.8h, v15.8h
	smlal2	v18.4s, v4.8h, v21.8h
	mov	v3.16b, v21.16b
	smlal2	v17.4s, v4.8h, v6.8h
	smlal2	v16.4s, v4.8h, v28.8h
	mov	v20.16b, v22.16b
	smlal	v16.4s, v20.4h, v23.4h
	smlal	v17.4s, v20.4h, v25.4h
	smlal	v16.4s, v30.4h, v25.4h
	smlal	v18.4s, v20.4h, v27.4h
	smlal	v17.4s, v30.4h, v27.4h
	smlal	v16.4s, v26.4h, v27.4h
	ldp	q2, q5, [sp, #48] // 32-byte Folded Reload
	and	v2.16b, v2.16b, v1.16b
	add	v2.8h, v2.8h, v5.8h
	str	q2, [sp, #1136] // 16-byte Folded Spill
	mov	v2.16b, v28.16b
	smlal	v0.4s, v20.4h, v2.4h
	smlal	v18.4s, v30.4h, v2.4h
	smlal	v17.4s, v26.4h, v2.4h
	smlal	v16.4s, v31.4h, v2.4h
	smlal	v0.4s, v30.4h, v6.4h
	smlal	v18.4s, v26.4h, v6.4h
	smlal	v17.4s, v31.4h, v6.4h
	smlal	v16.4s, v10.4h, v6.4h
	smlal	v18.4s, v31.4h, v3.4h
	mov	v2.16b, v15.16b
	smlal	v18.4s, v10.4h, v2.4h
	smlal	v17.4s, v10.4h, v3.4h
	smlal	v0.4s, v26.4h, v3.4h
	smlal	v16.4s, v11.4h, v3.4h
	smlal	v17.4s, v11.4h, v2.4h
	smlal	v16.4s, v12.4h, v2.4h
	mov	v4.16b, v15.16b
	ldp	q3, q2, [sp, #352] // 32-byte Folded Reload
	cmlt	v2.4s, v2.4s, #0
	cmlt	v3.4s, v3.4s, #0
	uzp1	v2.8h, v3.8h, v2.8h
	str	q2, [sp, #976] // 16-byte Folded Spill
	smlal	v0.4s, v31.4h, v4.4h
	ldp	q15, q8, [sp, #992] // 32-byte Folded Reload
	ldr	q2, [sp, #848] // 16-byte Folded Reload
	smull	v4.4s, v2.4h, v8.4h
	ldr	q2, [sp, #816] // 16-byte Folded Reload
	ldr	q30, [sp, #1168] // 16-byte Folded Reload
	smlal	v4.4s, v2.4h, v30.4h
	ldr	q12, [sp, #1184] // 16-byte Folded Reload
	ldp	q3, q2, [sp, #864] // 32-byte Folded Reload
	smlal	v4.4s, v3.4h, v12.4h
	ldr	q3, [sp, #688] // 16-byte Folded Reload
	smlal	v4.4s, v3.4h, v2.4h
	ldr	q3, [sp, #1440] // 16-byte Folded Reload
	ldr	q5, [sp, #928] // 16-byte Folded Reload
	smlal	v4.4s, v3.4h, v5.4h
	ldr	q3, [sp, #1408] // 16-byte Folded Reload
	ldr	q5, [sp, #912] // 16-byte Folded Reload
	smlal	v4.4s, v3.4h, v5.4h
	ldr	q14, [sp, #432] // 16-byte Folded Reload
	smlal	v4.4s, v15.4h, v14.4h
	ldr	q3, [sp, #752] // 16-byte Folded Reload
	ldr	q13, [sp, #1152] // 16-byte Folded Reload
	smlal	v4.4s, v13.4h, v3.4h
	ldr	q10, [sp, #1264] // 16-byte Folded Reload
	ldr	q3, [sp, #736] // 16-byte Folded Reload
	smlal	v4.4s, v3.4h, v10.4h
	ldr	q28, [sp, #1328] // 16-byte Folded Reload
	ldr	q23, [sp, #1024] // 16-byte Folded Reload
	smull	v21.4s, v23.4h, v28.4h
	ldr	q24, [sp, #1296] // 16-byte Folded Reload
	ldr	q26, [sp, #1424] // 16-byte Folded Reload
	smlal	v21.4s, v24.4h, v26.4h
	ldr	q6, [sp, #1088] // 16-byte Folded Reload
	ldr	q31, [sp, #1280] // 16-byte Folded Reload
	smlal	v21.4s, v6.4h, v31.4h
	ldr	q22, [sp, #1120] // 16-byte Folded Reload
	smlal	v21.4s, v22.4h, v10.4h
	smlal2	v21.4s, v24.8h, v2.8h
	mov	v25.16b, v2.16b
	smull	v2.4s, v23.4h, v26.4h
	ldp	q11, q20, [sp, #448] // 32-byte Folded Reload
	smlal	v2.4s, v24.4h, v20.4h
	smlal2	v21.4s, v23.8h, v12.8h
	smlal	v2.4s, v6.4h, v28.4h
	mov	v5.16b, v6.16b
	smlal	v2.4s, v22.4h, v31.4h
	mov	v19.16b, v22.16b
	ldr	q27, [sp, #496] // 16-byte Folded Reload
	smlal	v2.4s, v27.4h, v10.4h
	smull	v9.4s, v23.4h, v20.4h
	ldr	q6, [sp, #528] // 16-byte Folded Reload
	smlal	v9.4s, v24.4h, v6.4h
	ldr	q29, [sp, #608] // 16-byte Folded Reload
	smlal	v4.4s, v29.4h, v31.4h
	smlal	v4.4s, v11.4h, v28.4h
	smlal	v4.4s, v27.4h, v26.4h
	smlal	v4.4s, v19.4h, v20.4h
	mov	v3.16b, v5.16b
	smlal	v4.4s, v3.4h, v6.4h
	smull	v6.4s, v23.4h, v6.4h
	mov	v22.16b, v25.16b
	smlal2	v2.4s, v23.8h, v25.8h
	ldr	q5, [sp, #576] // 16-byte Folded Reload
	smlal	v4.4s, v23.4h, v5.4h
	smlal	v6.4s, v24.4h, v5.4h
	smlal	v6.4s, v3.4h, v20.4h
	smlal2	v21.4s, v3.8h, v8.8h
	smlal	v9.4s, v3.4h, v26.4h
	mov	v25.16b, v12.16b
	smlal2	v2.4s, v3.8h, v12.8h
	smlal	v9.4s, v19.4h, v28.4h
	smlal	v9.4s, v27.4h, v31.4h
	mov	v5.16b, v11.16b
	smlal	v9.4s, v5.4h, v10.4h
	smlal2	v9.4s, v3.8h, v22.8h
	ldr	q3, [sp] // 16-byte Folded Reload
	and	v3.16b, v3.16b, v1.16b
	ldr	q20, [sp, #80] // 16-byte Folded Reload
	add	v3.8h, v3.8h, v20.8h
	str	q3, [sp, #1248] // 16-byte Folded Spill
	mov	v3.16b, v19.16b
	smlal	v6.4s, v3.4h, v26.4h
	mov	v12.16b, v26.16b
	smlal	v6.4s, v27.4h, v28.4h
	smlal	v6.4s, v5.4h, v31.4h
	smlal	v6.4s, v29.4h, v10.4h
	smlal2	v21.4s, v19.8h, v30.8h
	mov	v26.16b, v30.16b
	smlal2	v2.4s, v19.8h, v8.8h
	smlal2	v9.4s, v19.8h, v25.8h
	smlal2	v6.4s, v19.8h, v22.8h
	smlal	v6.4s, v13.4h, v14.4h
	ldp	q19, q29, [sp, #912] // 32-byte Folded Reload
	smlal	v9.4s, v13.4h, v19.4h
	smlal	v6.4s, v15.4h, v19.4h
	smlal	v2.4s, v13.4h, v29.4h
	smlal	v9.4s, v15.4h, v29.4h
	mov	v27.16b, v15.16b
	ldr	q11, [sp, #1408] // 16-byte Folded Reload
	smlal	v6.4s, v11.4h, v29.4h
	ldp	q29, q3, [sp, #256] // 32-byte Folded Reload
	movi	v20.4h, #19
	smull	v24.4s, v3.4h, v20.4h
	str	q24, [sp, #1088] // 16-byte Folded Spill
	mov	v3.d[1], v29.d[0]
	ldr	q30, [sp, #288] // 16-byte Folded Reload
	mul	v28.8h, v3.8h, v30.8h
	str	q28, [sp, #1040] // 16-byte Folded Spill
	smull	v3.4s, v28.4h, v1.4h
	subhn	v24.4h, v24.4s, v3.4s
	smull	v20.4s, v29.4h, v20.4h
	str	q20, [sp, #1072] // 16-byte Folded Spill
	smull2	v3.4s, v28.8h, v1.8h
	subhn2	v24.8h, v20.4s, v3.4s
	str	q24, [sp, #1120] // 16-byte Folded Spill
	mov	v3.16b, v22.16b
	smlal	v21.4s, v13.4h, v3.4h
	smlal	v2.4s, v27.4h, v3.4h
	smlal	v9.4s, v11.4h, v3.4h
	ldr	q22, [sp, #1440] // 16-byte Folded Reload
	smlal	v6.4s, v22.4h, v3.4h
	ldr	q3, [sp, #832] // 16-byte Folded Reload
	and	v3.16b, v3.16b, v1.16b
	ldr	q20, [sp, #96] // 16-byte Folded Reload
	add	v3.8h, v3.8h, v20.8h
	str	q3, [sp, #1232] // 16-byte Folded Spill
	mov	v3.16b, v25.16b
	smlal	v21.4s, v27.4h, v3.4h
	smlal	v2.4s, v11.4h, v3.4h
	smlal	v9.4s, v22.4h, v3.4h
	ldr	q24, [sp, #688] // 16-byte Folded Reload
	smlal	v6.4s, v24.4h, v3.4h
	smlal	v2.4s, v22.4h, v8.4h
	mov	v5.16b, v22.16b
	smlal	v2.4s, v24.4h, v26.4h
	smlal	v9.4s, v24.4h, v8.4h
	smlal	v21.4s, v11.4h, v8.4h
	mov	v24.16b, v11.16b
	ldp	q3, q23, [sp, #848] // 32-byte Folded Reload
	smlal	v6.4s, v23.4h, v8.4h
	ldp	q20, q22, [sp, #16] // 32-byte Folded Reload
	cmlt	v20.4s, v20.4s, #0
	cmlt	v22.4s, v22.4s, #0
	uzp1	v20.8h, v22.8h, v20.8h
	str	q20, [sp, #1184] // 16-byte Folded Spill
	smlal	v9.4s, v23.4h, v26.4h
	smlal	v6.4s, v3.4h, v26.4h
	smlal	v21.4s, v5.4h, v26.4h
	mov	v25.16b, v5.16b
	uzp1	v20.8h, v0.8h, v21.8h
	ldr	q14, [sp, #944] // 16-byte Folded Reload
	mul	v20.8h, v20.8h, v14.8h
	smull2	v23.4s, v20.8h, v1.8h
	subhn	v21.4h, v21.4s, v23.4s
	smull	v20.4s, v20.4h, v1.4h
	subhn	v0.4h, v0.4s, v20.4s
	ldr	q8, [sp, #1392] // 16-byte Folded Reload
	ldr	q31, [sp, #1056] // 16-byte Folded Reload
	smull2	v20.4s, v31.8h, v8.8h
	ldr	q28, [sp, #1344] // 16-byte Folded Reload
	ldr	q5, [sp, #1376] // 16-byte Folded Reload
	smlal2	v20.4s, v5.8h, v28.8h
	ldr	q5, [sp, #1328] // 16-byte Folded Reload
	smull2	v23.4s, v15.8h, v5.8h
	smlal2	v23.4s, v13.8h, v12.8h
	ldr	q11, [sp, #1312] // 16-byte Folded Reload
	ldr	q29, [sp, #1216] // 16-byte Folded Reload
	smlal2	v20.4s, v11.8h, v29.8h
	ldr	q26, [sp, #1360] // 16-byte Folded Reload
	ldr	q19, [sp, #1200] // 16-byte Folded Reload
	smlal2	v20.4s, v26.8h, v19.8h
	ldr	q10, [sp, #1280] // 16-byte Folded Reload
	smlal2	v23.4s, v24.8h, v10.8h
	mov	v22.16b, v24.16b
	ldr	q19, [sp, #1264] // 16-byte Folded Reload
	smlal2	v23.4s, v25.8h, v19.8h
	uzp1	v24.8h, v20.8h, v23.8h
	mul	v24.8h, v24.8h, v14.8h
	smull	v25.4s, v24.4h, v1.4h
	subhn	v20.4h, v20.4s, v25.4s
	smull2	v24.4s, v24.8h, v1.8h
	mov	v25.16b, v20.16b
	subhn2	v25.8h, v23.4s, v24.4s
	subhn	v23.4h, v23.4s, v24.4s
	ldr	d19, [sp, #1112] // 8-byte Folded Reload
	smull	v23.4s, v23.4h, v19.4h
	ldr	q12, [sp, #784] // 16-byte Folded Reload
	mul	v24.8h, v25.8h, v12.8h
	smull2	v25.4s, v24.8h, v1.8h
	subhn	v23.4h, v23.4s, v25.4s
	ldr	d13, [sp, #1104] // 8-byte Folded Reload
	smull	v20.4s, v20.4h, v13.4h
	smull	v24.4s, v24.4h, v1.4h
	subhn	v20.4h, v20.4s, v24.4s
	add	v0.4h, v20.4h, v0.4h
	add	v20.4h, v23.4h, v21.4h
	movi	v23.4h, #19
	smull	v21.4s, v0.4h, v23.4h
	mov	v0.d[1], v20.d[0]
	smull	v20.4s, v20.4h, v23.4h
	movi	v15.4h, #19
	mul	v0.8h, v0.8h, v30.8h
	smull	v23.4s, v0.4h, v1.4h
	subhn	v23.4h, v21.4s, v23.4s
	smull2	v24.4s, v0.8h, v1.8h
	subhn2	v23.8h, v20.4s, v24.4s
	smlsl	v21.4s, v0.4h, v1.4h
	smlsl2	v20.4s, v0.8h, v1.8h
	cmlt	v0.4s, v21.4s, #0
	cmlt	v20.4s, v20.4s, #0
	uzp1	v0.8h, v0.8h, v20.8h
	and	v0.16b, v0.16b, v1.16b
	add	v0.8h, v0.8h, v23.8h
	str	q0, [sp, #1376] // 16-byte Folded Spill
	uzp1	v20.8h, v18.8h, v2.8h
	mul	v20.8h, v20.8h, v14.8h
	smull2	v21.4s, v20.8h, v1.8h
	subhn	v2.4h, v2.4s, v21.4s
	smull	v20.4s, v20.4h, v1.4h
	subhn	v21.4h, v18.4s, v20.4s
	smull2	v18.4s, v11.8h, v8.8h
	smlal2	v18.4s, v31.8h, v28.8h
	mov	v8.16b, v28.16b
	smull2	v23.4s, v22.8h, v5.8h
	ldr	q3, [sp, #1424] // 16-byte Folded Reload
	smlal2	v23.4s, v27.8h, v3.8h
	smlal2	v18.4s, v26.8h, v29.8h
	mov	v28.16b, v26.16b
	ldp	q0, q20, [sp, #224] // 32-byte Folded Reload
	smull	v27.4s, v20.4h, v15.4h
	mov	v20.d[1], v0.d[0]
	mul	v26.8h, v20.8h, v30.8h
	smull	v20.4s, v26.4h, v1.4h
	subhn	v5.4h, v27.4s, v20.4s
	smull	v11.4s, v0.4h, v15.4h
	smull2	v24.4s, v26.8h, v1.8h
	subhn2	v5.8h, v11.4s, v24.4s
	str	q5, [sp, #1264] // 16-byte Folded Spill
	ldr	q29, [sp, #1440] // 16-byte Folded Reload
	smlal2	v23.4s, v29.8h, v10.8h
	uzp1	v24.8h, v18.8h, v23.8h
	mul	v24.8h, v24.8h, v14.8h
	smull	v25.4s, v24.4h, v1.4h
	subhn	v18.4h, v18.4s, v25.4s
	smull2	v24.4s, v24.8h, v1.8h
	mov	v25.16b, v18.16b
	subhn2	v25.8h, v23.4s, v24.4s
	subhn	v23.4h, v23.4s, v24.4s
	smull	v23.4s, v23.4h, v19.4h
	fmov	d10, d19
	mul	v24.8h, v25.8h, v12.8h
	mov	v20.16b, v12.16b
	smull2	v25.4s, v24.8h, v1.8h
	subhn	v23.4h, v23.4s, v25.4s
	smull	v18.4s, v18.4h, v13.4h
	fmov	d0, d13
	smull	v24.4s, v24.4h, v1.4h
	subhn	v24.4h, v18.4s, v24.4s
	ldr	q5, [sp, #976] // 16-byte Folded Reload
	and	v18.16b, v5.16b, v1.16b
	ldr	q5, [sp, #112] // 16-byte Folded Reload
	add	v5.8h, v18.8h, v5.8h
	str	q5, [sp, #1280] // 16-byte Folded Spill
	add	v24.4h, v24.4h, v21.4h
	add	v2.4h, v23.4h, v2.4h
	ldr	q21, [sp, #192] // 16-byte Folded Reload
	smull	v25.4s, v21.4h, v15.4h
	ldr	q5, [sp, #160] // 16-byte Folded Reload
	mov	v21.d[1], v5.d[0]
	mov	v18.16b, v30.16b
	mul	v30.8h, v21.8h, v30.8h
	smull	v21.4s, v30.4h, v1.4h
	subhn	v21.4h, v25.4s, v21.4s
	smull	v31.4s, v5.4h, v15.4h
	smull2	v23.4s, v30.8h, v1.8h
	subhn2	v21.8h, v31.4s, v23.4s
	smull	v23.4s, v24.4h, v15.4h
	mov	v24.d[1], v2.d[0]
	mul	v24.8h, v24.8h, v18.8h
	smull	v12.4s, v24.4h, v1.4h
	subhn	v12.4h, v23.4s, v12.4s
	smull	v2.4s, v2.4h, v15.4h
	smull2	v13.4s, v24.8h, v1.8h
	subhn2	v12.8h, v2.4s, v13.4s
	ldr	q5, [sp, #1184] // 16-byte Folded Reload
	and	v22.16b, v5.16b, v1.16b
	ldr	q5, [sp, #128] // 16-byte Folded Reload
	add	v22.8h, v22.8h, v5.8h
	smlsl	v23.4s, v24.4h, v1.4h
	smlsl2	v2.4s, v24.8h, v1.8h
	cmlt	v2.4s, v2.4s, #0
	cmlt	v23.4s, v23.4s, #0
	uzp1	v2.8h, v23.8h, v2.8h
	and	v2.16b, v2.16b, v1.16b
	add	v2.8h, v2.8h, v12.8h
	uzp1	v23.8h, v17.8h, v9.8h
	mul	v23.8h, v23.8h, v14.8h
	smull2	v24.4s, v23.8h, v1.8h
	subhn	v24.4h, v9.4s, v24.4s
	smull	v23.4s, v23.4h, v1.4h
	subhn	v17.4h, v17.4s, v23.4s
	ldr	q19, [sp, #1392] // 16-byte Folded Reload
	smull2	v23.4s, v28.8h, v19.8h
	mov	v19.16b, v8.16b
	ldr	q8, [sp, #1312] // 16-byte Folded Reload
	smlal2	v23.4s, v8.8h, v19.8h
	ldr	q8, [sp, #1328] // 16-byte Folded Reload
	smull2	v9.4s, v29.8h, v8.8h
	ldr	q8, [sp, #1408] // 16-byte Folded Reload
	smlal2	v9.4s, v8.8h, v3.8h
	uzp1	v12.8h, v23.8h, v9.8h
	mul	v12.8h, v12.8h, v14.8h
	smull	v13.4s, v12.4h, v1.4h
	subhn	v23.4h, v23.4s, v13.4s
	smull2	v12.4s, v12.8h, v1.8h
	mov	v13.16b, v23.16b
	subhn2	v13.8h, v9.4s, v12.4s
	subhn	v9.4h, v9.4s, v12.4s
	smull	v9.4s, v9.4h, v10.4h
	mul	v12.8h, v13.8h, v20.8h
	smull2	v13.4s, v12.8h, v1.8h
	subhn	v9.4h, v9.4s, v13.4s
	smull	v23.4s, v23.4h, v0.4h
	smull	v12.4s, v12.4h, v1.4h
	subhn	v23.4h, v23.4s, v12.4s
	add	v17.4h, v23.4h, v17.4h
	add	v23.4h, v9.4h, v24.4h
	movi	v5.4h, #19
	smull	v24.4s, v17.4h, v5.4h
	mov	v17.d[1], v23.d[0]
	smull	v23.4s, v23.4h, v5.4h
	mul	v17.8h, v17.8h, v18.8h
	smull	v9.4s, v17.4h, v1.4h
	subhn	v9.4h, v24.4s, v9.4s
	smull2	v12.4s, v17.8h, v1.8h
	subhn2	v9.8h, v23.4s, v12.4s
	smlsl	v24.4s, v17.4h, v1.4h
	smlsl2	v23.4s, v17.8h, v1.8h
	cmlt	v17.4s, v24.4s, #0
	cmlt	v23.4s, v23.4s, #0
	uzp1	v17.8h, v17.8h, v23.8h
	uzp1	v23.8h, v16.8h, v6.8h
	mul	v23.8h, v23.8h, v14.8h
	smull2	v24.4s, v23.8h, v1.8h
	subhn	v6.4h, v6.4s, v24.4s
	smull	v23.4s, v23.4h, v1.4h
	subhn	v16.4h, v16.4s, v23.4s
	smull2	v23.4s, v28.8h, v19.8h
	smull2	v24.4s, v29.8h, v3.8h
	uzp1	v12.8h, v23.8h, v24.8h
	mul	v12.8h, v12.8h, v14.8h
	smull	v13.4s, v12.4h, v1.4h
	subhn	v23.4h, v23.4s, v13.4s
	ldr	q3, [sp, #1088] // 16-byte Folded Reload
	ldr	q28, [sp, #1040] // 16-byte Folded Reload
	smlsl	v3.4s, v28.4h, v1.4h
	ldr	q19, [sp, #1072] // 16-byte Folded Reload
	smlsl2	v19.4s, v28.8h, v1.8h
	cmlt	v19.4s, v19.4s, #0
	cmlt	v28.4s, v3.4s, #0
	uzp1	v19.8h, v28.8h, v19.8h
	smull	v28.4s, v23.4h, v0.4h
	smull2	v29.4s, v12.8h, v1.8h
	subhn2	v23.8h, v24.4s, v29.4s
	subhn	v24.4h, v24.4s, v29.4s
	smlsl	v27.4s, v26.4h, v1.4h
	smlsl2	v11.4s, v26.8h, v1.8h
	smull	v24.4s, v24.4h, v10.4h
	mul	v23.8h, v23.8h, v20.8h
	cmlt	v29.4s, v11.4s, #0
	cmlt	v27.4s, v27.4s, #0
	uzp1	v27.8h, v27.8h, v29.8h
	smull2	v29.4s, v23.8h, v1.8h
	subhn	v24.4h, v24.4s, v29.4s
	smull	v23.4s, v23.4h, v1.4h
	subhn	v23.4h, v28.4s, v23.4s
	add	v16.4h, v23.4h, v16.4h
	and	v17.16b, v17.16b, v1.16b
	add	v23.4h, v24.4h, v6.4h
	smull	v24.4s, v16.4h, v5.4h
	mov	v16.d[1], v23.d[0]
	and	v19.16b, v19.16b, v1.16b
	add	v6.8h, v17.8h, v9.8h
	mul	v16.8h, v16.8h, v18.8h
	smull	v17.4s, v16.4h, v1.4h
	subhn	v17.4h, v24.4s, v17.4s
	smull	v23.4s, v23.4h, v5.4h
	ldr	q0, [sp, #1120] // 16-byte Folded Reload
	add	v19.8h, v19.8h, v0.8h
	smull2	v26.4s, v16.8h, v1.8h
	subhn2	v17.8h, v23.4s, v26.4s
	smlsl	v24.4s, v16.4h, v1.4h
	smlsl2	v23.4s, v16.8h, v1.8h
	cmlt	v16.4s, v24.4s, #0
	cmlt	v23.4s, v23.4s, #0
	uzp1	v16.8h, v16.8h, v23.8h
	smlsl	v25.4s, v30.4h, v1.4h
	smlsl2	v31.4s, v30.8h, v1.8h
	cmlt	v23.4s, v31.4s, #0
	cmlt	v24.4s, v25.4s, #0
	and	v25.16b, v27.16b, v1.16b
	and	v16.16b, v16.16b, v1.16b
	add	v16.8h, v16.8h, v17.8h
	ldr	q0, [sp, #896] // 16-byte Folded Reload
	ldr	q3, [sp, #1296] // 16-byte Folded Reload
	smlal	v4.4s, v3.4h, v0.4h
	uzp1	v17.8h, v7.8h, v4.8h
	mul	v17.8h, v17.8h, v14.8h
	uzp1	v24.8h, v24.8h, v23.8h
	smull	v23.4s, v17.4h, v1.4h
	subhn	v7.4h, v7.4s, v23.4s
	smull2	v17.4s, v17.8h, v1.8h
	subhn	v26.4h, v4.4s, v17.4s
	smull	v27.4s, v7.4h, v5.4h
	ldr	q0, [sp, #1264] // 16-byte Folded Reload
	add	v20.8h, v25.8h, v0.8h
	subhn2	v7.8h, v4.4s, v17.4s
	mul	v4.8h, v7.8h, v18.8h
	smull	v7.4s, v4.4h, v1.4h
	subhn	v23.4h, v27.4s, v7.4s
	and	v7.16b, v24.16b, v1.16b
	smull	v17.4s, v26.4h, v5.4h
	smull2	v24.4s, v4.8h, v1.8h
	subhn2	v23.8h, v17.4s, v24.4s
	smlsl2	v17.4s, v4.8h, v1.8h
	cmlt	v17.4s, v17.4s, #0
	add	v7.8h, v7.8h, v21.8h
	smlsl	v27.4s, v4.4h, v1.4h
	cmlt	v4.4s, v27.4s, #0
	uzp1	v4.8h, v4.8h, v17.8h
	and	v21.16b, v4.16b, v1.16b
	trn1	v4.8h, v19.8h, v22.8h
	trn1	v17.8h, v7.8h, v20.8h
	trn2	v19.8h, v19.8h, v22.8h
	ldr	q0, [sp, #1232] // 16-byte Folded Reload
	ldr	q1, [sp, #1280] // 16-byte Folded Reload
	trn1	v22.8h, v1.8h, v0.8h
	trn2	v24.8h, v1.8h, v0.8h
	ldr	q0, [sp, #1136] // 16-byte Folded Reload
	ldr	q1, [sp, #1248] // 16-byte Folded Reload
	trn1	v25.8h, v1.8h, v0.8h
	trn2	v26.8h, v1.8h, v0.8h
	trn2	v3.8h, v7.8h, v20.8h
	trn1	v18.4s, v17.4s, v4.4s
	trn2	v7.4s, v17.4s, v4.4s
	trn1	v17.4s, v3.4s, v19.4s
	trn2	v20.4s, v3.4s, v19.4s
	ext	v3.16b, v18.16b, v18.16b, #8
	ext	v4.16b, v17.16b, v17.16b, #8
	ext	v5.16b, v7.16b, v7.16b, #8
	mov	v19.16b, v7.16b
	ext	v7.16b, v20.16b, v20.16b, #8
	trn1	v27.4s, v22.4s, v25.4s
	trn1	v28.4s, v24.4s, v26.4s
	mov	v18.d[1], v27.d[0]
	mov	v17.d[1], v28.d[0]
	add	v21.8h, v21.8h, v23.8h
	trn2	v25.4s, v22.4s, v25.4s
	trn2	v26.4s, v24.4s, v26.4s
	ext	v22.16b, v27.16b, v27.16b, #8
	mov	v3.d[1], v22.d[0]
	ldr	q0, [sp, #208] // 16-byte Folded Reload
	ldr	q1, [sp, #176] // 16-byte Folded Reload
	trn1	v23.8h, v1.8h, v0.8h
	ldr	q30, [sp, #144] // 16-byte Folded Reload
	ldr	q31, [sp, #1376] // 16-byte Folded Reload
	trn1	v24.8h, v30.8h, v31.8h
	trn1	v22.4s, v23.4s, v24.4s
	mov	v29.16b, v22.16b
	mov	v19.d[1], v25.d[0]
	mov	v20.d[1], v26.d[0]
	ext	v27.16b, v28.16b, v28.16b, #8
	ext	v25.16b, v25.16b, v25.16b, #8
	mov	v4.d[1], v27.d[0]
	mov	v5.d[1], v25.d[0]
	ext	v25.16b, v26.16b, v26.16b, #8
	trn1	v26.8h, v2.8h, v6.8h
	mov	v7.d[1], v25.d[0]
	trn1	v27.8h, v16.8h, v21.8h
	trn1	v25.4s, v26.4s, v27.4s
	mov	v29.d[1], v25.d[0]
	stp	q18, q29, [x10, #-128]
	trn2	v18.8h, v1.8h, v0.8h
	trn2	v0.8h, v30.8h, v31.8h
	trn2	v2.8h, v2.8h, v6.8h
	trn2	v6.8h, v16.8h, v21.8h
	trn1	v16.4s, v18.4s, v0.4s
	mov	v21.16b, v16.16b
	trn2	v23.4s, v23.4s, v24.4s
	trn1	v24.4s, v2.4s, v6.4s
	mov	v21.d[1], v24.d[0]
	stp	q17, q21, [x10, #-96]
	trn2	v17.4s, v26.4s, v27.4s
	mov	v21.16b, v23.16b
	mov	v21.d[1], v17.d[0]
	stp	q19, q21, [x10, #-64]
	trn2	v0.4s, v18.4s, v0.4s
	trn2	v2.4s, v2.4s, v6.4s
	mov	v6.16b, v0.16b
	mov	v6.d[1], v2.d[0]
	stp	q20, q6, [x10, #-32]
	ext	v6.16b, v22.16b, v22.16b, #8
	ext	v18.16b, v25.16b, v25.16b, #8
	mov	v6.d[1], v18.d[0]
	stp	q3, q6, [x10]
	ext	v3.16b, v16.16b, v16.16b, #8
	ext	v6.16b, v24.16b, v24.16b, #8
	mov	v3.d[1], v6.d[0]
	stp	q4, q3, [x10, #32]
	ext	v3.16b, v23.16b, v23.16b, #8
	ext	v4.16b, v17.16b, v17.16b, #8
	mov	v3.d[1], v4.d[0]
	stp	q5, q3, [x10, #64]
	ext	v0.16b, v0.16b, v0.16b, #8
	ext	v2.16b, v2.16b, v2.16b, #8
	mov	v0.d[1], v2.d[0]
	stp	q7, q0, [x10, #96]
	add	x11, x11, #4
	add	x8, x8, #256
	add	x9, x9, #256
	add	x10, x10, #256
	cmp	x11, #60
	b.lo	LBB5_1
// %bb.2:
	add	sp, sp, #1456
	ldp	x28, x27, [sp, #64] // 16-byte Folded Reload
	ldp	d9, d8, [sp, #48] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #32] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #16] // 16-byte Folded Reload
	ldp	d15, d14, [sp], #80 // 16-byte Folded Reload
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
	stp	d15, d14, [sp, #-128]! // 16-byte Folded Spill
	stp	d13, d12, [sp, #16] // 16-byte Folded Spill
	stp	d11, d10, [sp, #32] // 16-byte Folded Spill
	stp	d9, d8, [sp, #48] // 16-byte Folded Spill
	stp	x24, x23, [sp, #64] // 16-byte Folded Spill
	stp	x22, x21, [sp, #80] // 16-byte Folded Spill
	stp	x20, x19, [sp, #96] // 16-byte Folded Spill
	stp	x29, x30, [sp, #112] // 16-byte Folded Spill
	sub	sp, sp, #1296
	.cfi_def_cfa_offset 1424
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	.cfi_offset b8, -72
	.cfi_offset b9, -80
	.cfi_offset b10, -88
	.cfi_offset b11, -96
	.cfi_offset b12, -104
	.cfi_offset b13, -112
	.cfi_offset b14, -120
	.cfi_offset b15, -128
Lloh22:
	adrp	x8, :got:__stack_chk_guard
Lloh23:
	ldr	x8, [x8, :got_lo12:__stack_chk_guard]
Lloh24:
	ldr	x8, [x8]
	str	x8, [sp, #1288]
	add	x19, x1, #128
	add	x20, x0, #128
	mov	x21, #-4 // =0xfffffffffffffffc
Lloh25:
	adrp	x22, baseinv_zetas_pm
Lloh26:
	add	x22, x22, :lo12:baseinv_zetas_pm
	mov	w8, #64769 // =0xfd01
	dup	v1.8h, w8
	mov	w8, #769 // =0x301
	dup	v0.8h, w8
	stp	q0, q1, [sp, #368] // 32-byte Folded Spill
	mov	w8, #342 // =0x156
	dup	v0.4h, w8
	str	d0, [sp] // 8-byte Folded Spill
Lloh27:
	adrp	x23, qinv
Lloh28:
	add	x23, x23, :lo12:qinv
LBB6_1: // =>This Inner Loop Header: Depth=1
	ldr	q0, [x22], #16
	ldp	q2, q17, [x19, #-128]
	ldp	q3, q18, [x19, #-96]
	trn1	v4.8h, v2.8h, v3.8h
	trn2	v5.8h, v2.8h, v3.8h
	ldp	q2, q22, [x19, #-64]
	ldp	q3, q23, [x19, #-32]
	trn1	v6.8h, v2.8h, v3.8h
	trn2	v7.8h, v2.8h, v3.8h
	ldp	q2, q24, [x19]
	ldp	q3, q25, [x19, #32]
	trn1	v16.8h, v2.8h, v3.8h
	trn2	v19.8h, v2.8h, v3.8h
	ldp	q20, q2, [x19, #64]
	ldp	q21, q3, [x19, #96]
	trn1	v26.8h, v20.8h, v21.8h
	trn2	v20.8h, v20.8h, v21.8h
	trn1	v21.4s, v4.4s, v6.4s
	trn2	v27.4s, v4.4s, v6.4s
	trn1	v28.4s, v5.4s, v7.4s
	trn2	v29.4s, v5.4s, v7.4s
	trn1	v30.4s, v16.4s, v26.4s
	trn2	v31.4s, v16.4s, v26.4s
	trn1	v8.4s, v19.4s, v20.4s
	trn2	v26.4s, v19.4s, v20.4s
	ext	v4.16b, v21.16b, v21.16b, #8
	mov	v6.16b, v21.16b
	mov	v6.d[1], v30.d[0]
	ext	v5.16b, v28.16b, v28.16b, #8
	mov	v7.16b, v28.16b
	mov	v7.d[1], v8.d[0]
	ext	v16.16b, v27.16b, v27.16b, #8
	mov	v20.16b, v27.16b
	mov	v20.d[1], v31.d[0]
	ext	v19.16b, v29.16b, v29.16b, #8
	mov	v21.16b, v29.16b
	mov	v21.d[1], v26.d[0]
	ext	v27.16b, v30.16b, v30.16b, #8
	mov	v4.d[1], v27.d[0]
	ext	v27.16b, v8.16b, v8.16b, #8
	mov	v5.d[1], v27.d[0]
	ext	v27.16b, v31.16b, v31.16b, #8
	mov	v16.d[1], v27.d[0]
	ext	v26.16b, v26.16b, v26.16b, #8
	mov	v19.d[1], v26.d[0]
	trn1	v26.8h, v17.8h, v18.8h
	trn2	v27.8h, v17.8h, v18.8h
	trn1	v18.8h, v22.8h, v23.8h
	trn2	v28.8h, v22.8h, v23.8h
	trn1	v22.8h, v24.8h, v25.8h
	trn2	v17.8h, v24.8h, v25.8h
	trn1	v24.8h, v2.8h, v3.8h
	trn1	v23.4s, v26.4s, v18.4s
	trn2	v25.4s, v26.4s, v18.4s
	trn1	v26.4s, v27.4s, v28.4s
	ext	v18.16b, v23.16b, v23.16b, #8
	mov	v29.16b, v23.16b
	ext	v23.16b, v26.16b, v26.16b, #8
	trn2	v27.4s, v27.4s, v28.4s
	ext	v28.16b, v25.16b, v25.16b, #8
	ext	v30.16b, v27.16b, v27.16b, #8
	trn2	v2.8h, v2.8h, v3.8h
	trn1	v3.4s, v22.4s, v24.4s
	trn2	v22.4s, v22.4s, v24.4s
	str	q6, [sp, #1152]
	str	q20, [sp, #1168]
	trn1	v6.4s, v17.4s, v2.4s
	mov	v29.d[1], v3.d[0]
	trn2	v2.4s, v17.4s, v2.4s
	str	q7, [sp, #1024]
	str	q21, [sp, #1040]
	str	q4, [sp, #1184]
	str	q16, [sp, #1200]
	str	q5, [sp, #1056]
	str	q19, [sp, #1072]
	mov	v26.d[1], v6.d[0]
	mov	v25.d[1], v22.d[0]
	mov	v27.d[1], v2.d[0]
	fmov	d10, d0
	ext	v3.16b, v3.16b, v3.16b, #8
	mov	v18.d[1], v3.d[0]
	str	q29, [sp, #1216]
	str	q25, [sp, #1232]
	ext	v3.16b, v6.16b, v6.16b, #8
	mov	v23.d[1], v3.d[0]
	ext	v3.16b, v22.16b, v22.16b, #8
	mov	v28.d[1], v3.d[0]
	ext	v11.16b, v0.16b, v0.16b, #8
	ext	v2.16b, v2.16b, v2.16b, #8
	mov	v30.d[1], v2.d[0]
	str	q26, [sp, #1088]
	str	q27, [sp, #1104]
	str	q18, [sp, #1248]
	str	q28, [sp, #1264]
	ldr	q1, [sp, #384] // 16-byte Folded Reload
	mul	v0.8h, v0.8h, v1.8h
	str	q0, [sp, #400] // 16-byte Folded Spill
	str	q23, [sp, #1120]
	str	q30, [sp, #1136]
	add	x0, sp, #656
	add	x1, sp, #1152
	add	x2, sp, #1152
	bl	vec_poly_mul8
	add	x0, sp, #416
	add	x1, sp, #1024
	add	x2, sp, #1024
	bl	vec_poly_mul8
	ldp	q0, q1, [sp, #784]
	ldp	q3, q2, [sp, #512]
	sub	v0.8h, v0.8h, v2.8h
	smull	v2.4s, v0.4h, v10.4h
	ext	v4.16b, v0.16b, v0.16b, #8
	smull	v4.4s, v4.4h, v11.4h
	ldr	q6, [sp, #400] // 16-byte Folded Reload
	mul	v0.8h, v0.8h, v6.8h
	ldr	q7, [sp, #368] // 16-byte Folded Reload
	smull	v5.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v2.4h, v2.4s, v5.4s
	subhn2	v2.8h, v4.4s, v0.4s
	ldp	q0, q4, [sp, #656]
	sub	v17.8h, v0.8h, v2.8h
	ldp	q0, q2, [sp, #544]
	sub	v0.8h, v1.8h, v0.8h
	smull	v1.4s, v0.4h, v10.4h
	ext	v5.16b, v0.16b, v0.16b, #8
	smull	v5.4s, v5.4h, v11.4h
	mul	v0.8h, v0.8h, v6.8h
	mov	v15.16b, v6.16b
	smull	v6.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v1.4h, v1.4s, v6.4s
	subhn2	v1.8h, v5.4s, v0.4s
	ldp	q0, q5, [sp, #416]
	add	v0.8h, v0.8h, v1.8h
	sub	v16.8h, v4.8h, v0.8h
	ldp	q0, q1, [sp, #816]
	sub	v0.8h, v0.8h, v2.8h
	smull	v2.4s, v0.4h, v10.4h
	ext	v4.16b, v0.16b, v0.16b, #8
	smull	v4.4s, v4.4h, v11.4h
	mul	v0.8h, v0.8h, v15.8h
	smull	v6.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v2.4h, v2.4s, v6.4s
	subhn2	v2.8h, v4.4s, v0.4s
	add	v0.8h, v5.8h, v2.8h
	ldp	q2, q4, [sp, #688]
	sub	v20.8h, v2.8h, v0.8h
	ldp	q0, q2, [sp, #576]
	sub	v0.8h, v1.8h, v0.8h
	smull	v1.4s, v0.4h, v10.4h
	ext	v5.16b, v0.16b, v0.16b, #8
	smull	v5.4s, v5.4h, v11.4h
	mul	v0.8h, v0.8h, v15.8h
	smull	v6.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v1.4h, v1.4s, v6.4s
	subhn2	v1.8h, v5.4s, v0.4s
	ldp	q0, q5, [sp, #448]
	add	v0.8h, v0.8h, v1.8h
	sub	v18.8h, v4.8h, v0.8h
	ldp	q0, q1, [sp, #848]
	sub	v0.8h, v0.8h, v2.8h
	smull	v2.4s, v0.4h, v10.4h
	ext	v4.16b, v0.16b, v0.16b, #8
	smull	v4.4s, v4.4h, v11.4h
	mul	v0.8h, v0.8h, v15.8h
	smull	v6.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v2.4h, v2.4s, v6.4s
	subhn2	v2.8h, v4.4s, v0.4s
	add	v0.8h, v5.8h, v2.8h
	ldp	q2, q4, [sp, #720]
	sub	v21.8h, v2.8h, v0.8h
	ldp	q0, q2, [sp, #608]
	sub	v0.8h, v1.8h, v0.8h
	smull	v1.4s, v0.4h, v10.4h
	ext	v5.16b, v0.16b, v0.16b, #8
	smull	v5.4s, v5.4h, v11.4h
	mul	v0.8h, v0.8h, v15.8h
	smull	v6.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v1.4h, v1.4s, v6.4s
	subhn2	v1.8h, v5.4s, v0.4s
	ldp	q0, q5, [sp, #480]
	add	v0.8h, v0.8h, v1.8h
	sub	v19.8h, v4.8h, v0.8h
	ldr	q0, [sp, #880]
	sub	v0.8h, v0.8h, v2.8h
	smull	v1.4s, v0.4h, v10.4h
	ext	v2.16b, v0.16b, v0.16b, #8
	smull	v2.4s, v2.4h, v11.4h
	mul	v0.8h, v0.8h, v15.8h
	smull	v4.4s, v0.4h, v7.4h
	smull2	v0.4s, v0.8h, v7.8h
	subhn	v1.4h, v1.4s, v4.4s
	subhn2	v1.8h, v2.4s, v0.4s
	add	v0.8h, v5.8h, v1.8h
	ldp	q1, q2, [sp, #752]
	sub	v5.8h, v1.8h, v0.8h
	sub	v0.8h, v2.8h, v3.8h
	ldr	q1, [sp, #640]
	smull	v2.4s, v1.4h, v10.4h
	ext	v3.16b, v1.16b, v1.16b, #8
	smull	v3.4s, v3.4h, v11.4h
	mul	v1.8h, v1.8h, v15.8h
	smull	v4.4s, v1.4h, v7.4h
	smull2	v1.4s, v1.8h, v7.8h
	subhn	v2.4h, v2.4s, v4.4s
	subhn2	v2.8h, v3.4s, v1.4s
	add	v22.8h, v0.8h, v2.8h
	smull	v0.4s, v20.4h, v20.4h
	smull2	v1.4s, v20.8h, v20.8h
	mul	v2.8h, v20.8h, v20.8h
	ldr	q12, [sp, #384] // 16-byte Folded Reload
	mul	v2.8h, v2.8h, v12.8h
	smull	v3.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v26.4h, v0.4s, v3.4s
	subhn2	v26.8h, v1.4s, v2.4s
	add	v2.8h, v20.8h, v17.8h
	str	q2, [sp, #80] // 16-byte Folded Spill
	str	q20, [sp, #112] // 16-byte Folded Spill
	smull	v0.4s, v2.4h, v2.4h
	smull2	v1.4s, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v3.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v28.4h, v0.4s, v3.4s
	subhn2	v28.8h, v1.4s, v2.4s
	smull	v0.4s, v21.4h, v21.4h
	smull2	v1.4s, v21.8h, v21.8h
	mul	v2.8h, v21.8h, v21.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v3.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v27.4h, v0.4s, v3.4s
	subhn2	v27.8h, v1.4s, v2.4s
	smull	v0.4s, v5.4h, v5.4h
	smull2	v1.4s, v5.8h, v5.8h
	mul	v2.8h, v5.8h, v5.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v3.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v29.4h, v0.4s, v3.4s
	subhn2	v29.8h, v1.4s, v2.4s
	add	v2.8h, v5.8h, v21.8h
	str	q21, [sp, #160] // 16-byte Folded Spill
	str	q2, [sp, #272] // 16-byte Folded Spill
	str	q5, [sp, #352] // 16-byte Folded Spill
	smull	v0.4s, v2.4h, v2.4h
	smull2	v1.4s, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v3.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v3.4h, v0.4s, v3.4s
	subhn2	v3.8h, v1.4s, v2.4s
	add	v4.8h, v21.8h, v17.8h
	add	v5.8h, v5.8h, v20.8h
	smull	v0.4s, v4.4h, v4.4h
	smull2	v1.4s, v4.8h, v4.8h
	mul	v2.8h, v4.8h, v4.8h
	mov	v21.16b, v4.16b
	str	q4, [sp, #208] // 16-byte Folded Spill
	mul	v2.8h, v2.8h, v12.8h
	smull	v4.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v31.4h, v0.4s, v4.4s
	subhn2	v31.8h, v1.4s, v2.4s
	smull	v0.4s, v5.4h, v5.4h
	smull2	v1.4s, v5.8h, v5.8h
	mul	v2.8h, v5.8h, v5.8h
	str	q5, [sp, #240] // 16-byte Folded Spill
	mul	v2.8h, v2.8h, v12.8h
	smull	v4.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v6.4h, v0.4s, v4.4s
	subhn2	v6.8h, v1.4s, v2.4s
	add	v2.8h, v5.8h, v21.8h
	str	q2, [sp, #304] // 16-byte Folded Spill
	smull	v0.4s, v2.4h, v2.4h
	smull2	v1.4s, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v4.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v13.4h, v0.4s, v4.4s
	subhn2	v13.8h, v1.4s, v2.4s
	smull	v0.4s, v16.4h, v16.4h
	smull2	v1.4s, v16.8h, v16.8h
	mul	v2.8h, v16.8h, v16.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v4.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v14.4h, v0.4s, v4.4s
	subhn2	v14.8h, v1.4s, v2.4s
	smull	v0.4s, v18.4h, v18.4h
	smull2	v1.4s, v18.8h, v18.8h
	mul	v2.8h, v18.8h, v18.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v4.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v0.4h, v0.4s, v4.4s
	subhn2	v0.8h, v1.4s, v2.4s
	add	v2.8h, v18.8h, v16.8h
	mov	v23.16b, v16.16b
	stp	q16, q18, [sp, #128] // 32-byte Folded Spill
	str	q2, [sp, #320] // 16-byte Folded Spill
	mov	v21.16b, v18.16b
	smull	v1.4s, v2.4h, v2.4h
	smull2	v4.4s, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v2.8h
	mul	v2.8h, v2.8h, v12.8h
	smull	v5.4s, v2.4h, v7.4h
	smull2	v16.4s, v2.8h, v7.8h
	subhn	v2.4h, v1.4s, v5.4s
	subhn2	v2.8h, v4.4s, v16.4s
	smull	v1.4s, v19.4h, v19.4h
	smull2	v4.4s, v19.8h, v19.8h
	mul	v5.8h, v19.8h, v19.8h
	str	q19, [sp, #192] // 16-byte Folded Spill
	mul	v5.8h, v5.8h, v12.8h
	smull	v16.4s, v5.4h, v7.4h
	smull2	v5.4s, v5.8h, v7.8h
	subhn	v1.4h, v1.4s, v16.4s
	subhn2	v1.8h, v4.4s, v5.4s
	add	v16.8h, v22.8h, v19.8h
	str	q16, [sp, #256] // 16-byte Folded Spill
	smull	v4.4s, v16.4h, v16.4h
	smull2	v5.4s, v16.8h, v16.8h
	mul	v16.8h, v16.8h, v16.8h
	mul	v16.8h, v16.8h, v12.8h
	smull	v18.4s, v16.4h, v7.4h
	smull2	v16.4s, v16.8h, v7.8h
	subhn	v18.4h, v4.4s, v18.4s
	subhn2	v18.8h, v5.4s, v16.4s
	add	v19.8h, v19.8h, v23.8h
	smull	v4.4s, v19.4h, v19.4h
	mul	v5.8h, v19.8h, v19.8h
	mul	v5.8h, v5.8h, v12.8h
	smull	v16.4s, v5.4h, v7.4h
	subhn	v4.4h, v4.4s, v16.4s
	smull2	v16.4s, v19.8h, v19.8h
	mov	v23.16b, v19.16b
	str	q19, [sp, #224] // 16-byte Folded Spill
	smull2	v5.4s, v5.8h, v7.8h
	subhn2	v4.8h, v16.4s, v5.4s
	add	v21.8h, v22.8h, v21.8h
	str	q21, [sp, #288] // 16-byte Folded Spill
	mul	v5.8h, v21.8h, v21.8h
	mul	v16.8h, v5.8h, v12.8h
	smull	v5.4s, v21.4h, v21.4h
	smull	v19.4s, v16.4h, v7.4h
	subhn	v5.4h, v5.4s, v19.4s
	smull2	v16.4s, v16.8h, v7.8h
	smull2	v19.4s, v21.8h, v21.8h
	subhn2	v5.8h, v19.4s, v16.4s
	add	v21.8h, v21.8h, v23.8h
	mul	v16.8h, v21.8h, v21.8h
	mul	v19.8h, v16.8h, v12.8h
	smull	v16.4s, v21.4h, v21.4h
	mov	v23.16b, v21.16b
	str	q21, [sp, #336] // 16-byte Folded Spill
	smull	v21.4s, v19.4h, v7.4h
	subhn	v16.4h, v16.4s, v21.4s
	smull2	v19.4s, v19.8h, v7.8h
	smull2	v21.4s, v23.8h, v23.8h
	subhn2	v16.8h, v21.4s, v19.4s
	add	v19.8h, v29.8h, v27.8h
	sub	v3.8h, v3.8h, v19.8h
	mov	v24.16b, v22.16b
	str	q22, [sp, #176] // 16-byte Folded Spill
	mul	v19.8h, v22.8h, v22.8h
	mul	v19.8h, v19.8h, v12.8h
	smull	v21.4s, v24.4h, v24.4h
	smull	v22.4s, v19.4h, v7.4h
	subhn	v21.4h, v21.4s, v22.4s
	smull2	v23.4s, v19.8h, v7.8h
	smull2	v24.4s, v24.8h, v24.8h
	mov	v22.16b, v21.16b
	subhn2	v22.8h, v24.4s, v23.4s
	add	v19.8h, v0.8h, v22.8h
	add	v19.8h, v3.8h, v19.8h
	add	v25.8h, v1.8h, v5.8h
	sub	v25.8h, v19.8h, v25.8h
	smull	v19.4s, v25.4h, v10.4h
	mul	v30.8h, v25.8h, v15.8h
	smull	v8.4s, v30.4h, v7.4h
	subhn	v19.4h, v19.4s, v8.4s
	ext	v25.16b, v25.16b, v25.16b, #8
	smull	v25.4s, v25.4h, v11.4h
	smull2	v30.4s, v30.8h, v7.8h
	subhn2	v19.8h, v25.4s, v30.4s
	add	v25.8h, v22.8h, v1.8h
	sub	v25.8h, v18.8h, v25.8h
	sub	v30.8h, v29.8h, v25.8h
	smull	v18.4s, v30.4h, v10.4h
	mul	v8.8h, v30.8h, v15.8h
	smull	v9.4s, v8.4h, v7.4h
	subhn	v18.4h, v18.4s, v9.4s
	ext	v30.16b, v30.16b, v30.16b, #8
	smull	v30.4s, v30.4h, v11.4h
	smull2	v8.4s, v8.8h, v7.8h
	subhn2	v18.8h, v30.4s, v8.4s
	subhn	v23.4h, v24.4s, v23.4s
	smull	v21.4s, v21.4h, v10.4h
	mul	v22.8h, v22.8h, v15.8h
	smull	v24.4s, v22.4h, v7.4h
	subhn	v21.4h, v21.4s, v24.4s
	smull	v23.4s, v23.4h, v11.4h
	smull2	v22.4s, v22.8h, v7.8h
	subhn2	v21.8h, v23.4s, v22.4s
	mul	v22.8h, v17.8h, v17.8h
	mul	v22.8h, v22.8h, v12.8h
	smull	v23.4s, v17.4h, v17.4h
	smull	v24.4s, v22.4h, v7.4h
	subhn	v23.4h, v23.4s, v24.4s
	smull2	v22.4s, v22.8h, v7.8h
	smull2	v24.4s, v17.8h, v17.8h
	mov	v30.16b, v23.16b
	subhn2	v30.8h, v24.4s, v22.4s
	subhn	v22.4h, v24.4s, v22.4s
	add	v24.8h, v26.8h, v30.8h
	sub	v24.8h, v28.8h, v24.8h
	add	v28.8h, v0.8h, v14.8h
	sub	v2.8h, v2.8h, v28.8h
	add	v28.8h, v4.8h, v6.8h
	add	v28.8h, v28.8h, v27.8h
	add	v29.8h, v26.8h, v29.8h
	sub	v28.8h, v28.8h, v29.8h
	add	v25.8h, v2.8h, v25.8h
	add	v5.8h, v25.8h, v5.8h
	add	v5.8h, v28.8h, v5.8h
	sub	v5.8h, v5.8h, v16.8h
	smull	v16.4s, v5.4h, v10.4h
	mul	v25.8h, v5.8h, v15.8h
	smull	v28.4s, v25.4h, v7.4h
	subhn	v16.4h, v16.4s, v28.4s
	sub	v23.4h, v23.4h, v16.4h
	ext	v5.16b, v5.16b, v5.16b, #8
	smull	v5.4s, v5.4h, v11.4h
	smull2	v25.4s, v25.8h, v7.8h
	subhn2	v16.8h, v5.4s, v25.4s
	subhn	v5.4h, v5.4s, v25.4s
	sub	v25.8h, v30.8h, v16.8h
	add	v16.8h, v30.8h, v27.8h
	add	v2.8h, v16.8h, v2.8h
	add	v2.8h, v2.8h, v18.8h
	add	v16.8h, v26.8h, v31.8h
	sub	v30.8h, v16.8h, v2.8h
	add	v2.8h, v14.8h, v19.8h
	sub	v20.8h, v24.8h, v2.8h
	add	v2.8h, v3.8h, v24.8h
	add	v3.8h, v31.8h, v6.8h
	add	v0.8h, v3.8h, v0.8h
	add	v0.8h, v2.8h, v0.8h
	add	v1.8h, v14.8h, v1.8h
	add	v1.8h, v13.8h, v1.8h
	add	v0.8h, v0.8h, v4.8h
	sub	v0.8h, v1.8h, v0.8h
	add	v27.8h, v0.8h, v21.8h
	smull	v0.4s, v23.4h, v23.4h
	sub	v31.4h, v22.4h, v5.4h
	smull	v1.4s, v31.4h, v31.4h
	mul	v2.8h, v25.8h, v12.8h
	mul	v3.8h, v2.8h, v25.8h
	mov	v6.16b, v25.16b
	smull	v4.4s, v3.4h, v7.4h
	smull2	v3.4s, v3.8h, v7.8h
	subhn	v0.4h, v0.4s, v4.4s
	subhn	v1.4h, v1.4s, v3.4s
	smull	v3.4s, v30.4h, v30.4h
	ext	v16.16b, v30.16b, v30.16b, #8
	smull2	v5.4s, v30.8h, v30.8h
	mul	v4.8h, v30.8h, v30.8h
	mul	v4.8h, v4.8h, v12.8h
	smull	v18.4s, v4.4h, v7.4h
	smull2	v19.4s, v4.8h, v7.8h
	subhn	v4.4h, v3.4s, v18.4s
	subhn	v5.4h, v5.4s, v19.4s
	smull	v3.4s, v23.4h, v30.4h
	fmov	d13, d23
	smull	v16.4s, v31.4h, v16.4h
	mul	v2.8h, v30.8h, v2.8h
	smull	v18.4s, v2.4h, v7.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v3.4h, v3.4s, v18.4s
	ldr	d23, [sp] // 8-byte Folded Reload
	smull	v18.4s, v3.4h, v23.4h
	subhn2	v3.8h, v16.4s, v2.4s
	subhn	v2.4h, v16.4s, v2.4s
	smull	v2.4s, v2.4h, v23.4h
	mvni	v9.8h, #169
	mul	v3.8h, v3.8h, v9.8h
	smull	v16.4s, v3.4h, v7.4h
	smull2	v19.4s, v3.8h, v7.8h
	subhn	v3.4h, v18.4s, v16.4s
	subhn	v2.4h, v2.4s, v19.4s
	smull	v16.4s, v20.4h, v20.4h
	mul	v19.8h, v20.8h, v12.8h
	mul	v18.8h, v19.8h, v20.8h
	smull	v21.4s, v18.4h, v7.4h
	subhn	v16.4h, v16.4s, v21.4s
	smull2	v18.4s, v18.8h, v7.8h
	smull2	v21.4s, v20.8h, v20.8h
	subhn	v18.4h, v21.4s, v18.4s
	mul	v21.8h, v27.8h, v27.8h
	mul	v21.8h, v21.8h, v12.8h
	smull	v22.4s, v27.4h, v27.4h
	smull	v24.4s, v21.4h, v7.4h
	subhn	v22.4h, v22.4s, v24.4s
	smull2	v21.4s, v21.8h, v7.8h
	smull2	v24.4s, v27.8h, v27.8h
	mov	v25.16b, v22.16b
	subhn2	v25.8h, v24.4s, v21.4s
	subhn	v21.4h, v24.4s, v21.4s
	mul	v19.8h, v27.8h, v19.8h
	smull	v24.4s, v20.4h, v27.4h
	smull	v28.4s, v19.4h, v7.4h
	subhn	v24.4h, v24.4s, v28.4s
	smull2	v19.4s, v19.8h, v7.8h
	smull2	v28.4s, v20.8h, v27.8h
	mov	v8.16b, v24.16b
	subhn2	v8.8h, v28.4s, v19.4s
	subhn	v19.4h, v28.4s, v19.4s
	smull	v24.4s, v24.4h, v23.4h
	mul	v28.8h, v8.8h, v9.8h
	smull	v8.4s, v28.4h, v7.4h
	subhn	v24.4h, v24.4s, v8.4s
	smull	v19.4s, v19.4h, v23.4h
	smull2	v28.4s, v28.8h, v7.8h
	subhn	v19.4h, v19.4s, v28.4s
	sub	v4.4h, v4.4h, v24.4h
	sub	v5.4h, v5.4h, v19.4h
	smull	v19.4s, v4.4h, v10.4h
	mov	v4.d[1], v5.d[0]
	mul	v4.8h, v4.8h, v15.8h
	smull	v24.4s, v4.4h, v7.4h
	subhn	v19.4h, v19.4s, v24.4s
	smull	v5.4s, v5.4h, v11.4h
	smull2	v4.4s, v4.8h, v7.8h
	subhn	v5.4h, v5.4s, v4.4s
	sub	v4.4h, v0.4h, v19.4h
	sub	v0.4h, v1.4h, v5.4h
	sub	v1.4h, v3.4h, v16.4h
	smull	v3.4s, v22.4h, v10.4h
	sub	v5.4h, v2.4h, v18.4h
	mul	v2.8h, v25.8h, v15.8h
	smull	v16.4s, v2.4h, v7.4h
	subhn	v16.4h, v3.4s, v16.4s
	mov	v3.16b, v4.16b
	mov	v3.d[1], v0.d[0]
	smull	v18.4s, v21.4h, v11.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn	v18.4h, v18.4s, v2.4s
	add	v2.4h, v1.4h, v16.4h
	add	v1.4h, v5.4h, v18.4h
	smull	v5.4s, v4.4h, v4.4h
	mul	v16.8h, v3.8h, v3.8h
	mul	v16.8h, v16.8h, v12.8h
	smull	v18.4s, v16.4h, v7.4h
	subhn	v18.4h, v5.4s, v18.4s
	mov	v5.16b, v2.16b
	mov	v5.d[1], v1.d[0]
	smull2	v16.4s, v16.8h, v7.8h
	smull	v19.4s, v0.4h, v0.4h
	subhn2	v18.8h, v19.4s, v16.4s
	mul	v16.8h, v5.8h, v5.8h
	mul	v16.8h, v16.8h, v12.8h
	mov	v26.16b, v12.16b
	smull	v19.4s, v2.4h, v2.4h
	smull	v21.4s, v16.4h, v7.4h
	subhn	v19.4h, v19.4s, v21.4s
	smull2	v16.4s, v16.8h, v7.8h
	smull	v21.4s, v1.4h, v1.4h
	mov	v22.16b, v19.16b
	subhn2	v22.8h, v21.4s, v16.4s
	subhn	v16.4h, v21.4s, v16.4s
	smull	v19.4s, v19.4h, v10.4h
	mul	v21.8h, v22.8h, v15.8h
	mov	v23.16b, v15.16b
	smull	v22.4s, v21.4h, v7.4h
	subhn	v19.4h, v19.4s, v22.4s
	smull	v16.4s, v16.4h, v11.4h
	smull2	v21.4s, v21.8h, v7.8h
	subhn2	v19.8h, v16.4s, v21.4s
	add	v16.8h, v18.8h, v19.8h
	cmlt	v18.8h, v16.8h, #0
	and	v18.16b, v18.16b, v7.16b
	add	v16.8h, v18.8h, v16.8h
	umov	w8, v16.h[0]
	ldrh	w8, [x23, x8, lsl #1]
	umov	w9, v16.h[1]
	ldrh	w9, [x23, x9, lsl #1]
	umov	w10, v16.h[2]
	ldrh	w10, [x23, x10, lsl #1]
	umov	w11, v16.h[3]
	ldrh	w11, [x23, x11, lsl #1]
	umov	w12, v16.h[4]
	ldrh	w12, [x23, x12, lsl #1]
	umov	w13, v16.h[5]
	ldrh	w13, [x23, x13, lsl #1]
	umov	w14, v16.h[6]
	ldrh	w14, [x23, x14, lsl #1]
	umov	w15, v16.h[7]
	ldrh	w15, [x23, x15, lsl #1]
	fmov	s16, w8
	mov	v18.16b, v16.16b
	mov	v18.h[1], w9
	mov	v18.h[2], w10
	mov	v18.h[3], w11
	mov	v18.h[4], w12
	mov	v18.h[5], w13
	mov	v18.h[6], w14
	mov	v18.h[7], w15
	mov	v16.h[1], w9
	mov	v16.h[2], w10
	mov	v16.h[3], w11
	smull	v4.4s, v16.4h, v4.4h
	fmov	s19, w12
	mov	v19.h[1], w13
	mov	v19.h[2], w14
	mov	v19.h[3], w15
	smull	v0.4s, v19.4h, v0.4h
	mul	v18.8h, v18.8h, v12.8h
	mul	v3.8h, v18.8h, v3.8h
	smlsl2	v0.4s, v3.8h, v7.8h
	smull	v3.4s, v3.4h, v7.4h
	subhn	v15.4h, v4.4s, v3.4s
	smull	v2.4s, v16.4h, v2.4h
	smull	v1.4s, v19.4h, v1.4h
	mul	v3.8h, v18.8h, v5.8h
	smull	v4.4s, v3.4h, v7.4h
	smull2	v3.4s, v3.8h, v7.8h
	subhn	v2.4h, v2.4s, v4.4s
	subhn	v4.4h, v1.4s, v3.4s
	mvni	v16.4h, #170
	smull	v5.4s, v2.4h, v16.4h
	subhn2	v2.8h, v1.4s, v3.4s
	smull	v1.4s, v4.4h, v16.4h
	movi	v3.8h, #85
	mul	v2.8h, v2.8h, v3.8h
	smull	v3.4s, v2.4h, v7.4h
	smlsl2	v1.4s, v2.8h, v7.8h
	subhn	v9.4h, v5.4s, v3.4s
	ushr	v1.4s, v1.4s, #16
	uzp1	v4.8h, v0.8h, v1.8h
	ext	v29.16b, v4.16b, v4.16b, #8
	smull	v1.4s, v9.4h, v30.4h
	mov	v2.16b, v9.16b
	mov	v2.d[1], v29.d[0]
	mul	v28.8h, v2.8h, v12.8h
	mul	v2.8h, v28.8h, v30.8h
	smull	v5.4s, v2.4h, v7.4h
	subhn	v1.4h, v1.4s, v5.4s
	smull2	v2.4s, v2.8h, v7.8h
	smull2	v5.4s, v4.8h, v30.8h
	mov	v16.16b, v1.16b
	subhn2	v16.8h, v5.4s, v2.4s
	subhn	v2.4h, v5.4s, v2.4s
	smull	v1.4s, v1.4h, v10.4h
	mul	v5.8h, v16.8h, v23.8h
	smull	v16.4s, v5.4h, v7.4h
	subhn	v18.4h, v1.4s, v16.4s
	smull	v1.4s, v2.4h, v11.4h
	smull2	v2.4s, v5.8h, v7.8h
	subhn	v2.4h, v1.4s, v2.4s
	ushr	v0.4s, v0.4s, #16
	uzp1	v16.8h, v0.8h, v0.8h
	ext	v0.16b, v16.16b, v16.16b, #8
	mov	v1.16b, v15.16b
	mov	v1.d[1], v0.d[0]
	mul	v3.8h, v1.8h, v12.8h
	fmov	d22, d13
	smull	v5.4s, v15.4h, v13.4h
	mov	v12.16b, v6.16b
	mul	v19.8h, v3.8h, v6.8h
	smull	v21.4s, v19.4h, v7.4h
	subhn	v5.4h, v5.4s, v21.4s
	smull	v0.4s, v0.4h, v31.4h
	smull2	v19.4s, v19.8h, v7.8h
	subhn	v0.4h, v0.4s, v19.4s
	sub	v8.4h, v5.4h, v18.4h
	sub	v14.4h, v0.4h, v2.4h
	mov	v13.16b, v8.16b
	mov	v13.d[1], v14.d[0]
	smull	v0.4s, v8.4h, v17.4h
	mul	v1.8h, v13.8h, v26.8h
	str	q1, [sp, #48] // 16-byte Folded Spill
	mul	v2.8h, v1.8h, v17.8h
	mov	v19.16b, v7.16b
	smull	v5.4s, v2.4h, v19.4h
	subhn	v1.4h, v0.4s, v5.4s
	ext	v0.16b, v17.16b, v17.16b, #8
	smull	v0.4s, v14.4h, v0.4h
	smull2	v2.4s, v2.8h, v7.8h
	subhn2	v1.8h, v0.4s, v2.4s
	str	q1, [sp, #96] // 16-byte Folded Spill
	smull	v0.4s, v9.4h, v27.4h
	mul	v2.8h, v28.8h, v27.8h
	smull	v5.4s, v2.4h, v19.4h
	subhn	v0.4h, v0.4s, v5.4s
	smull2	v2.4s, v2.8h, v7.8h
	smull2	v5.4s, v4.8h, v27.8h
	mov	v7.16b, v0.16b
	subhn2	v7.8h, v5.4s, v2.4s
	subhn	v2.4h, v5.4s, v2.4s
	smull	v0.4s, v0.4h, v10.4h
	mul	v5.8h, v7.8h, v23.8h
	smull	v7.4s, v5.4h, v19.4h
	subhn	v0.4h, v0.4s, v7.4s
	smull	v2.4s, v2.4h, v11.4h
	smull2	v5.4s, v5.8h, v19.8h
	subhn	v5.4h, v2.4s, v5.4s
	smull	v2.4s, v15.4h, v20.4h
	mul	v7.8h, v3.8h, v20.8h
	smull	v18.4s, v7.4h, v19.4h
	subhn	v2.4h, v2.4s, v18.4s
	smull2	v7.4s, v7.8h, v19.8h
	mov	v1.16b, v19.16b
	smull2	v18.4s, v16.8h, v20.8h
	subhn	v7.4h, v18.4s, v7.4s
	sub	v24.4h, v0.4h, v2.4h
	sub	v25.4h, v5.4h, v7.4h
	mov	v23.16b, v24.16b
	mov	v23.d[1], v25.d[0]
	ldr	q0, [sp, #112] // 16-byte Folded Reload
	smull	v7.4s, v24.4h, v0.4h
	mul	v2.8h, v23.8h, v26.8h
	str	q2, [sp, #32] // 16-byte Folded Spill
	mul	v18.8h, v2.8h, v0.8h
	smull	v19.4s, v18.4h, v1.4h
	subhn	v2.4h, v7.4s, v19.4s
	ext	v7.16b, v0.16b, v0.16b, #8
	smull	v7.4s, v25.4h, v7.4h
	smull2	v17.4s, v18.8h, v1.8h
	subhn2	v2.8h, v7.4s, v17.4s
	str	q2, [sp, #64] // 16-byte Folded Spill
	add	v7.8h, v23.8h, v13.8h
	mul	v2.8h, v7.8h, v26.8h
	str	q2, [sp, #16] // 16-byte Folded Spill
	add	v5.4h, v24.4h, v8.4h
	str	d5, [sp, #8] // 8-byte Folded Spill
	ldr	q0, [sp, #80] // 16-byte Folded Reload
	smull	v7.4s, v5.4h, v0.4h
	mul	v18.8h, v2.8h, v0.8h
	smull	v17.4s, v18.4h, v1.4h
	subhn	v2.4h, v7.4s, v17.4s
	ext	v6.16b, v0.16b, v0.16b, #8
	add	v17.4h, v25.4h, v14.4h
	smull	v6.4s, v17.4h, v6.4h
	smull2	v7.4s, v18.8h, v1.8h
	subhn2	v2.8h, v6.4s, v7.4s
	str	q2, [sp, #112] // 16-byte Folded Spill
	smull	v6.4s, v15.4h, v30.4h
	mul	v7.8h, v3.8h, v30.8h
	smull	v18.4s, v7.4h, v1.4h
	subhn	v6.4h, v6.4s, v18.4s
	smull2	v18.4s, v16.8h, v30.8h
	smull2	v7.4s, v7.8h, v1.8h
	subhn	v7.4h, v18.4s, v7.4s
	smull	v18.4s, v29.4h, v31.4h
	smull	v19.4s, v9.4h, v22.4h
	mul	v21.8h, v28.8h, v12.8h
	smull	v22.4s, v21.4h, v1.4h
	subhn	v19.4h, v19.4s, v22.4s
	smull2	v21.4s, v21.8h, v1.8h
	subhn	v18.4h, v18.4s, v21.4s
	add	v29.4h, v19.4h, v6.4h
	add	v30.4h, v18.4h, v7.4h
	mov	v7.16b, v29.16b
	mov	v7.d[1], v30.d[0]
	ldr	q0, [sp, #160] // 16-byte Folded Reload
	smull	v6.4s, v29.4h, v0.4h
	mul	v12.8h, v7.8h, v26.8h
	mul	v18.8h, v12.8h, v0.8h
	mov	v5.16b, v1.16b
	smull	v19.4s, v18.4h, v5.4h
	subhn	v1.4h, v6.4s, v19.4s
	ext	v6.16b, v0.16b, v0.16b, #8
	smull	v6.4s, v30.4h, v6.4h
	smull2	v18.4s, v18.8h, v5.8h
	subhn2	v1.8h, v6.4s, v18.4s
	str	q1, [sp, #160] // 16-byte Folded Spill
	smull	v6.4s, v15.4h, v27.4h
	smull2	v16.4s, v16.8h, v27.8h
	mul	v1.8h, v3.8h, v27.8h
	smull	v18.4s, v1.4h, v5.4h
	subhn	v6.4h, v6.4s, v18.4s
	smull2	v1.4s, v1.8h, v5.8h
	subhn	v1.4h, v16.4s, v1.4s
	smull	v16.4s, v9.4h, v20.4h
	smull2	v4.4s, v4.8h, v20.8h
	mul	v3.8h, v28.8h, v20.8h
	smull	v18.4s, v3.4h, v5.4h
	subhn	v16.4h, v16.4s, v18.4s
	smull2	v3.4s, v3.8h, v5.8h
	subhn	v3.4h, v4.4s, v3.4s
	add	v4.4h, v16.4h, v6.4h
	add	v1.4h, v3.4h, v1.4h
	mov	v3.16b, v4.16b
	mov	v3.d[1], v1.d[0]
	mvni	v0.4h, #170
	smull	v4.4s, v4.4h, v0.4h
	smull	v1.4s, v1.4h, v0.4h
	movi	v0.8h, #85
	mul	v3.8h, v3.8h, v0.8h
	smlsl2	v1.4s, v3.8h, v5.8h
	smull	v3.4s, v3.4h, v5.4h
	subhn	v20.4h, v4.4s, v3.4s
	ushr	v1.4s, v1.4s, #16
	uzp1	v19.8h, v0.8h, v1.8h
	ext	v2.16b, v19.16b, v19.16b, #8
	mov	v27.16b, v20.16b
	mov	v27.d[1], v2.d[0]
	add	v3.8h, v27.8h, v7.8h
	mul	v31.8h, v3.8h, v26.8h
	add	v6.4h, v20.4h, v29.4h
	ldr	q0, [sp, #272] // 16-byte Folded Reload
	smull	v3.4s, v6.4h, v0.4h
	mul	v16.8h, v31.8h, v0.8h
	smull	v18.4s, v16.4h, v5.4h
	subhn	v15.4h, v3.4s, v18.4s
	ext	v18.16b, v0.16b, v0.16b, #8
	add	v4.4h, v2.4h, v30.4h
	smull	v18.4s, v4.4h, v18.4h
	smull2	v16.4s, v16.8h, v5.8h
	subhn2	v15.8h, v18.4s, v16.4s
	add	v18.8h, v7.8h, v13.8h
	add	v13.4h, v29.4h, v8.4h
	ldr	q0, [sp, #208] // 16-byte Folded Reload
	smull	v7.4s, v13.4h, v0.4h
	mul	v9.8h, v18.8h, v26.8h
	mul	v16.8h, v9.8h, v0.8h
	smull	v21.4s, v16.4h, v5.4h
	subhn	v1.4h, v7.4s, v21.4s
	ext	v21.16b, v0.16b, v0.16b, #8
	add	v7.4h, v30.4h, v14.4h
	smull	v21.4s, v7.4h, v21.4h
	smull2	v16.4s, v16.8h, v5.8h
	subhn2	v1.8h, v21.4s, v16.4s
	str	q1, [sp, #272] // 16-byte Folded Spill
	add	v3.8h, v27.8h, v23.8h
	add	v0.4h, v20.4h, v24.4h
	str	d0, [sp, #208] // 8-byte Folded Spill
	ldr	q1, [sp, #240] // 16-byte Folded Reload
	smull	v16.4s, v0.4h, v1.4h
	mul	v21.8h, v3.8h, v26.8h
	mul	v0.8h, v21.8h, v1.8h
	smull	v22.4s, v0.4h, v5.4h
	subhn	v22.4h, v16.4s, v22.4s
	ext	v1.16b, v1.16b, v1.16b, #8
	add	v16.4h, v2.4h, v25.4h
	smull	v1.4s, v16.4h, v1.4h
	smull2	v0.4s, v0.8h, v5.8h
	subhn2	v22.8h, v1.4s, v0.4s
	str	q22, [sp, #240] // 16-byte Folded Spill
	add	v18.8h, v3.8h, v18.8h
	ldr	q0, [sp, #304] // 16-byte Folded Reload
	smull	v3.4s, v18.4h, v0.4h
	mul	v22.8h, v18.8h, v26.8h
	mul	v1.8h, v22.8h, v0.8h
	smull	v2.4s, v1.4h, v5.4h
	subhn	v2.4h, v3.4s, v2.4s
	smull2	v0.4s, v18.8h, v0.8h
	smull2	v1.4s, v1.8h, v5.8h
	subhn2	v2.8h, v0.4s, v1.4s
	str	q2, [sp, #304] // 16-byte Folded Spill
	ldr	q2, [sp, #128] // 16-byte Folded Reload
	ext	v0.16b, v2.16b, v2.16b, #8
	smull	v0.4s, v14.4h, v0.4h
	smull	v1.4s, v8.4h, v2.4h
	ldr	q3, [sp, #48] // 16-byte Folded Reload
	mul	v2.8h, v3.8h, v2.8h
	smull	v8.4s, v2.4h, v5.4h
	subhn	v1.4h, v1.4s, v8.4s
	smull2	v2.4s, v2.8h, v5.8h
	subhn2	v1.8h, v0.4s, v2.4s
	ldr	q8, [sp, #144] // 16-byte Folded Reload
	ext	v0.16b, v8.16b, v8.16b, #8
	smull	v2.4s, v25.4h, v0.4h
	smull	v0.4s, v24.4h, v8.4h
	ldr	q3, [sp, #32] // 16-byte Folded Reload
	mul	v23.8h, v3.8h, v8.8h
	smull	v24.4s, v23.4h, v5.4h
	subhn	v0.4h, v0.4s, v24.4s
	smull2	v23.4s, v23.8h, v5.8h
	subhn2	v0.8h, v2.4s, v23.4s
	ldr	q3, [sp, #320] // 16-byte Folded Reload
	ext	v2.16b, v3.16b, v3.16b, #8
	smull	v2.4s, v17.4h, v2.4h
	ldr	d17, [sp, #8] // 8-byte Folded Reload
	smull	v17.4s, v17.4h, v3.4h
	ldr	q23, [sp, #16] // 16-byte Folded Reload
	mul	v23.8h, v23.8h, v3.8h
	smull	v24.4s, v23.4h, v5.4h
	subhn	v17.4h, v17.4s, v24.4s
	smull2	v23.4s, v23.8h, v5.8h
	subhn2	v17.8h, v2.4s, v23.4s
	ldr	q3, [sp, #192] // 16-byte Folded Reload
	ext	v2.16b, v3.16b, v3.16b, #8
	smull	v23.4s, v30.4h, v2.4h
	smull	v2.4s, v29.4h, v3.4h
	mul	v24.8h, v12.8h, v3.8h
	smull	v28.4s, v24.4h, v5.4h
	subhn	v2.4h, v2.4s, v28.4s
	smull2	v24.4s, v24.8h, v5.8h
	subhn2	v2.8h, v23.4s, v24.4s
	ldr	q24, [sp, #256] // 16-byte Folded Reload
	ext	v23.16b, v24.16b, v24.16b, #8
	smull	v3.4s, v4.4h, v23.4h
	mov	v23.16b, v24.16b
	smull	v4.4s, v6.4h, v23.4h
	mul	v6.8h, v31.8h, v24.8h
	smull	v23.4s, v6.4h, v5.4h
	subhn	v23.4h, v4.4s, v23.4s
	ldr	q28, [sp, #352] // 16-byte Folded Reload
	smull	v4.4s, v20.4h, v28.4h
	smull2	v6.4s, v6.8h, v5.8h
	subhn2	v23.8h, v3.4s, v6.4s
	smull2	v24.4s, v19.8h, v28.8h
	mul	v3.8h, v27.8h, v26.8h
	mul	v6.8h, v3.8h, v28.8h
	smull	v28.4s, v6.4h, v5.4h
	smull2	v27.4s, v6.8h, v5.8h
	subhn	v29.4h, v4.4s, v28.4s
	mov	v28.16b, v29.16b
	subhn2	v28.8h, v24.4s, v27.4s
	ldp	q31, q25, [sp, #160] // 32-byte Folded Reload
	add	v4.8h, v28.8h, v31.8h
	sub	v6.8h, v15.8h, v4.8h
	smull	v20.4s, v20.4h, v25.4h
	smull2	v4.4s, v19.8h, v25.8h
	mul	v26.8h, v3.8h, v25.8h
	smull	v3.4s, v26.4h, v5.4h
	subhn	v3.4h, v20.4s, v3.4s
	ldr	q25, [sp, #224] // 16-byte Folded Reload
	ext	v20.16b, v25.16b, v25.16b, #8
	smull	v30.4s, v7.4h, v20.4h
	smull	v7.4s, v13.4h, v25.4h
	mul	v19.8h, v9.8h, v25.8h
	smull	v20.4s, v19.4h, v5.4h
	subhn	v7.4h, v7.4s, v20.4s
	smull2	v20.4s, v26.8h, v5.8h
	smull2	v19.4s, v19.8h, v5.8h
	subhn2	v7.8h, v30.4s, v19.4s
	mov	v19.16b, v3.16b
	subhn2	v19.8h, v4.4s, v20.4s
	add	v26.8h, v19.8h, v2.8h
	sub	v26.8h, v23.8h, v26.8h
	ldr	q25, [sp, #288] // 16-byte Folded Reload
	ext	v23.16b, v25.16b, v25.16b, #8
	smull	v23.4s, v16.4h, v23.4h
	ldr	q12, [sp, #64] // 16-byte Folded Reload
	add	v16.8h, v28.8h, v12.8h
	sub	v16.8h, v31.8h, v16.8h
	mov	v13.16b, v31.16b
	ldr	q14, [sp, #240] // 16-byte Folded Reload
	add	v30.8h, v16.8h, v14.8h
	ldr	d16, [sp, #208] // 8-byte Folded Reload
	smull	v16.4s, v16.4h, v25.4h
	mul	v21.8h, v21.8h, v25.8h
	mov	v25.16b, v5.16b
	smull	v31.4s, v21.4h, v25.4h
	smull2	v21.4s, v21.8h, v5.8h
	subhn	v16.4h, v16.4s, v31.4s
	subhn2	v16.8h, v23.4s, v21.4s
	ldr	q23, [sp, #336] // 16-byte Folded Reload
	smull	v21.4s, v18.4h, v23.4h
	smull2	v5.4s, v18.8h, v23.8h
	mul	v18.8h, v22.8h, v23.8h
	smull	v23.4s, v18.4h, v25.4h
	smull2	v31.4s, v18.8h, v25.8h
	subhn	v18.4h, v21.4s, v23.4s
	subhn2	v18.8h, v5.4s, v31.4s
	add	v5.8h, v19.8h, v0.8h
	sub	v5.8h, v2.8h, v5.8h
	add	v21.8h, v5.8h, v16.8h
	smull	v5.4s, v30.4h, v10.4h
	ext	v23.16b, v30.16b, v30.16b, #8
	smull	v23.4s, v23.4h, v11.4h
	ldr	q8, [sp, #400] // 16-byte Folded Reload
	mul	v30.8h, v30.8h, v8.8h
	smull	v31.4s, v30.4h, v25.4h
	smull2	v30.4s, v30.8h, v25.8h
	subhn	v5.4h, v5.4s, v31.4s
	subhn2	v5.8h, v23.4s, v30.4s
	smull	v23.4s, v21.4h, v10.4h
	ext	v30.16b, v21.16b, v21.16b, #8
	smull	v30.4s, v30.4h, v11.4h
	mul	v21.8h, v21.8h, v8.8h
	mov	v22.16b, v8.16b
	smull	v31.4s, v21.4h, v25.4h
	smull2	v8.4s, v21.8h, v25.8h
	subhn	v21.4h, v23.4s, v31.4s
	subhn2	v21.8h, v30.4s, v8.4s
	smull	v23.4s, v6.4h, v10.4h
	ext	v30.16b, v6.16b, v6.16b, #8
	smull	v31.4s, v30.4h, v11.4h
	mul	v30.8h, v6.8h, v22.8h
	smull	v8.4s, v30.4h, v25.4h
	smull2	v9.4s, v30.8h, v25.8h
	subhn	v30.4h, v23.4s, v8.4s
	subhn2	v30.8h, v31.4s, v9.4s
	smull	v23.4s, v26.4h, v10.4h
	mul	v8.8h, v26.8h, v22.8h
	smull	v31.4s, v8.4h, v25.4h
	subhn	v31.4h, v23.4s, v31.4s
	ext	v23.16b, v26.16b, v26.16b, #8
	smull	v23.4s, v23.4h, v11.4h
	smull2	v8.4s, v8.8h, v25.8h
	subhn2	v31.8h, v23.4s, v8.4s
	smull	v23.4s, v29.4h, v10.4h
	subhn	v24.4h, v24.4s, v27.4s
	mul	v27.8h, v28.8h, v22.8h
	smull	v28.4s, v27.4h, v25.4h
	subhn	v23.4h, v23.4s, v28.4s
	smull	v24.4s, v24.4h, v11.4h
	smull2	v27.4s, v27.8h, v25.8h
	subhn2	v23.8h, v24.4s, v27.4s
	add	v24.8h, v0.8h, v1.8h
	sub	v17.8h, v17.8h, v24.8h
	subhn	v4.4h, v4.4s, v20.4s
	add	v20.8h, v17.8h, v7.8h
	add	v16.8h, v20.8h, v16.8h
	add	v16.8h, v26.8h, v16.8h
	smull	v3.4s, v3.4h, v10.4h
	sub	v16.8h, v18.8h, v16.8h
	smull	v4.4s, v4.4h, v11.4h
	mul	v18.8h, v19.8h, v22.8h
	smull	v19.4s, v18.4h, v25.4h
	smull2	v18.4s, v18.8h, v25.8h
	subhn	v3.4h, v3.4s, v19.4s
	subhn2	v3.8h, v4.4s, v18.4s
	mvni	v8.4h, #170
	smull	v4.4s, v16.4h, v8.4h
	movi	v18.8h, #85
	mul	v18.8h, v16.8h, v18.8h
	smull	v19.4s, v18.4h, v25.4h
	subhn	v4.4h, v4.4s, v19.4s
	mvni	v19.8h, #170
	smull2	v16.4s, v16.8h, v19.8h
	smull2	v18.4s, v18.8h, v25.8h
	subhn2	v4.8h, v16.4s, v18.4s
	ldp	q19, q18, [sp, #96] // 32-byte Folded Reload
	add	v16.8h, v12.8h, v19.8h
	sub	v16.8h, v18.8h, v16.8h
	ldr	q22, [sp, #272] // 16-byte Folded Reload
	add	v18.8h, v16.8h, v22.8h
	add	v18.8h, v18.8h, v14.8h
	add	v6.8h, v6.8h, v18.8h
	sub	v5.8h, v19.8h, v5.8h
	sub	v18.8h, v21.8h, v1.8h
	stp	q5, q18, [sp, #896]
	sub	v5.8h, v16.8h, v30.8h
	sub	v16.8h, v31.8h, v17.8h
	stp	q5, q16, [sp, #928]
	add	v5.8h, v19.8h, v13.8h
	add	v16.8h, v12.8h, v22.8h
	add	v5.8h, v5.8h, v23.8h
	sub	v5.8h, v16.8h, v5.8h
	add	v1.8h, v1.8h, v2.8h
	add	v0.8h, v0.8h, v7.8h
	sub	v0.8h, v1.8h, v0.8h
	add	v0.8h, v0.8h, v3.8h
	stp	q5, q0, [sp, #960]
	ldr	q0, [sp, #304] // 16-byte Folded Reload
	sub	v0.8h, v0.8h, v6.8h
	stp	q0, q4, [sp, #992]
	add	x0, sp, #656
	add	x1, sp, #896
	add	x2, sp, #1152
	bl	vec_poly_mul8
	add	x0, sp, #416
	add	x1, sp, #896
	add	x2, sp, #1024
	bl	vec_poly_mul8
	ldp	q0, q1, [sp, #784]
	smull	v2.4s, v0.4h, v10.4h
	ext	v3.16b, v0.16b, v0.16b, #8
	smull	v4.4s, v3.4h, v11.4h
	ldr	q30, [sp, #400] // 16-byte Folded Reload
	mul	v0.8h, v0.8h, v30.8h
	ldr	q29, [sp, #368] // 16-byte Folded Reload
	smull	v3.4s, v0.4h, v29.4h
	ldp	q6, q7, [sp, #544]
	smull	v16.4s, v6.4h, v10.4h
	ext	v5.16b, v6.16b, v6.16b, #8
	smull	v5.4s, v5.4h, v11.4h
	smull2	v17.4s, v0.8h, v29.8h
	mul	v0.8h, v6.8h, v30.8h
	smull	v18.4s, v0.4h, v29.4h
	smull2	v6.4s, v0.8h, v29.8h
	smull	v0.4s, v1.4h, v10.4h
	ext	v19.16b, v1.16b, v1.16b, #8
	subhn	v2.4h, v2.4s, v3.4s
	smull	v19.4s, v19.4h, v11.4h
	mul	v1.8h, v1.8h, v30.8h
	smull	v21.4s, v1.4h, v29.4h
	smull2	v20.4s, v1.8h, v29.8h
	smull	v22.4s, v7.4h, v10.4h
	subhn	v3.4h, v16.4s, v18.4s
	ext	v1.16b, v7.16b, v7.16b, #8
	smull	v16.4s, v1.4h, v11.4h
	mul	v1.8h, v7.8h, v30.8h
	smull	v7.4s, v1.4h, v29.4h
	smull2	v18.4s, v1.8h, v29.8h
	subhn	v1.4h, v0.4s, v21.4s
	subhn	v0.4h, v22.4s, v7.4s
	ldp	q7, q22, [sp, #816]
	smull	v21.4s, v7.4h, v10.4h
	ext	v23.16b, v7.16b, v7.16b, #8
	subhn2	v2.8h, v4.4s, v17.4s
	smull	v23.4s, v23.4h, v11.4h
	mul	v4.8h, v7.8h, v30.8h
	smull	v7.4s, v4.4h, v29.4h
	smull2	v24.4s, v4.8h, v29.8h
	subhn	v4.4h, v21.4s, v7.4s
	subhn2	v3.8h, v5.4s, v6.4s
	ldp	q6, q7, [sp, #656]
	ldp	q17, q5, [sp, #416]
	subhn2	v1.8h, v19.4s, v20.4s
	ldp	q21, q20, [sp, #576]
	smull	v19.4s, v21.4h, v10.4h
	ext	v25.16b, v21.16b, v21.16b, #8
	smull	v25.4s, v25.4h, v11.4h
	mul	v21.8h, v21.8h, v30.8h
	subhn2	v0.8h, v16.4s, v18.4s
	smull	v16.4s, v21.4h, v29.4h
	smull2	v26.4s, v21.8h, v29.8h
	subhn	v18.4h, v19.4s, v16.4s
	ldp	q21, q16, [sp, #688]
	subhn2	v4.8h, v23.4s, v24.4s
	smull	v19.4s, v22.4h, v10.4h
	ext	v23.16b, v22.16b, v22.16b, #8
	smull	v23.4s, v23.4h, v11.4h
	mul	v22.8h, v22.8h, v30.8h
	smull	v24.4s, v22.4h, v29.4h
	subhn2	v18.8h, v25.4s, v26.4s
	smull2	v22.4s, v22.8h, v29.8h
	subhn	v19.4h, v19.4s, v24.4s
	smull	v24.4s, v20.4h, v10.4h
	ext	v25.16b, v20.16b, v20.16b, #8
	smull	v26.4s, v25.4h, v11.4h
	subhn2	v19.8h, v23.4s, v22.4s
	mul	v20.8h, v20.8h, v30.8h
	smull	v22.4s, v20.4h, v29.4h
	smull2	v23.4s, v20.8h, v29.8h
	subhn	v20.4h, v24.4s, v22.4s
	ldp	q25, q22, [sp, #448]
	subhn2	v20.8h, v26.4s, v23.4s
	ldp	q23, q26, [sp, #848]
	smull	v24.4s, v23.4h, v10.4h
	ext	v27.16b, v23.16b, v23.16b, #8
	sub	v6.8h, v6.8h, v2.8h
	smull	v27.4s, v27.4h, v11.4h
	mul	v2.8h, v23.8h, v30.8h
	smull	v23.4s, v2.4h, v29.4h
	smull2	v28.4s, v2.8h, v29.8h
	subhn	v2.4h, v24.4s, v23.4s
	sub	v17.8h, v3.8h, v17.8h
	subhn2	v2.8h, v27.4s, v28.4s
	ldp	q3, q27, [sp, #608]
	smull	v24.4s, v3.4h, v10.4h
	ext	v28.16b, v3.16b, v3.16b, #8
	sub	v23.8h, v7.8h, v1.8h
	smull	v1.4s, v28.4h, v11.4h
	mul	v3.8h, v3.8h, v30.8h
	smull	v7.4s, v3.4h, v29.4h
	smull2	v3.4s, v3.8h, v29.8h
	subhn	v7.4h, v24.4s, v7.4s
	sub	v24.8h, v0.8h, v5.8h
	subhn2	v7.8h, v1.4s, v3.4s
	ldp	q0, q1, [sp, #720]
	smull	v3.4s, v26.4h, v10.4h
	ext	v5.16b, v26.16b, v26.16b, #8
	sub	v21.8h, v21.8h, v4.8h
	smull	v4.4s, v5.4h, v11.4h
	mul	v5.8h, v26.8h, v30.8h
	smull	v26.4s, v5.4h, v29.4h
	smull2	v5.4s, v5.8h, v29.8h
	subhn	v26.4h, v3.4s, v26.4s
	sub	v25.8h, v18.8h, v25.8h
	subhn2	v26.8h, v4.4s, v5.4s
	smull	v3.4s, v27.4h, v10.4h
	ext	v4.16b, v27.16b, v27.16b, #8
	smull	v4.4s, v4.4h, v11.4h
	mul	v5.8h, v27.8h, v30.8h
	sub	v16.8h, v16.8h, v19.8h
	smull	v18.4s, v5.4h, v29.4h
	smull2	v5.4s, v5.8h, v29.8h
	subhn	v18.4h, v3.4s, v18.4s
	subhn2	v18.8h, v4.4s, v5.4s
	sub	v19.8h, v20.8h, v22.8h
	ldp	q4, q20, [sp, #480]
	ldr	q5, [sp, #880]
	smull	v22.4s, v5.4h, v10.4h
	ext	v3.16b, v5.16b, v5.16b, #8
	smull	v27.4s, v3.4h, v11.4h
	sub	v3.8h, v0.8h, v2.8h
	mul	v0.8h, v5.8h, v30.8h
	smull	v2.4s, v0.4h, v29.4h
	smull2	v0.4s, v0.8h, v29.8h
	subhn	v2.4h, v22.4s, v2.4s
	subhn2	v2.8h, v27.4s, v0.4s
	sub	v5.8h, v7.8h, v4.8h
	ldr	q0, [sp, #640]
	smull	v4.4s, v0.4h, v10.4h
	ext	v7.16b, v0.16b, v0.16b, #8
	smull	v22.4s, v7.4h, v11.4h
	mul	v0.8h, v0.8h, v30.8h
	sub	v7.8h, v1.8h, v26.8h
	smull	v1.4s, v0.4h, v29.4h
	smull2	v0.4s, v0.8h, v29.8h
	subhn	v4.4h, v4.4s, v1.4s
	subhn2	v4.8h, v22.4s, v0.4s
	ldp	q1, q0, [sp, #752]
	sub	v18.8h, v18.8h, v20.8h
	ldp	q20, q22, [sp, #512]
	smull	v26.4s, v22.4h, v8.4h
	mvni	v27.8h, #170
	smull2	v27.4s, v22.8h, v27.8h
	sub	v2.8h, v1.8h, v2.8h
	movi	v1.8h, #85
	mul	v1.8h, v22.8h, v1.8h
	smull	v22.4s, v1.4h, v29.4h
	smull2	v28.4s, v1.8h, v29.8h
	subhn	v1.4h, v26.4s, v22.4s
	subhn2	v1.8h, v27.4s, v28.4s
	sub	v4.8h, v4.8h, v20.8h
	trn1	v20.8h, v6.8h, v17.8h
	trn2	v6.8h, v6.8h, v17.8h
	trn1	v17.8h, v23.8h, v24.8h
	trn2	v22.8h, v23.8h, v24.8h
	trn1	v23.8h, v21.8h, v25.8h
	trn2	v25.8h, v21.8h, v25.8h
	trn1	v24.8h, v16.8h, v19.8h
	trn2	v28.8h, v16.8h, v19.8h
	trn1	v19.4s, v20.4s, v17.4s
	trn2	v20.4s, v20.4s, v17.4s
	trn1	v21.4s, v6.4s, v22.4s
	trn2	v29.4s, v6.4s, v22.4s
	ext	v6.16b, v19.16b, v19.16b, #8
	ext	v16.16b, v21.16b, v21.16b, #8
	ext	v17.16b, v20.16b, v20.16b, #8
	mov	v22.16b, v20.16b
	trn1	v27.4s, v23.4s, v24.4s
	trn2	v24.4s, v23.4s, v24.4s
	trn1	v26.4s, v25.4s, v28.4s
	trn2	v25.4s, v25.4s, v28.4s
	ext	v20.16b, v29.16b, v29.16b, #8
	mov	v23.16b, v29.16b
	trn1	v29.8h, v3.8h, v5.8h
	trn1	v30.8h, v7.8h, v18.8h
	trn1	v31.4s, v29.4s, v30.4s
	ext	v28.16b, v31.16b, v31.16b, #8
	mov	v19.d[1], v27.d[0]
	trn2	v5.8h, v3.8h, v5.8h
	trn2	v7.8h, v7.8h, v18.8h
	trn1	v18.4s, v5.4s, v7.4s
	mov	v21.d[1], v26.d[0]
	ext	v3.16b, v18.16b, v18.16b, #8
	trn2	v29.4s, v29.4s, v30.4s
	ext	v30.16b, v29.16b, v29.16b, #8
	mov	v22.d[1], v24.d[0]
	ext	v27.16b, v27.16b, v27.16b, #8
	trn2	v5.4s, v5.4s, v7.4s
	ext	v7.16b, v5.16b, v5.16b, #8
	mov	v6.d[1], v27.d[0]
	mov	v23.d[1], v25.d[0]
	ext	v26.16b, v26.16b, v26.16b, #8
	mov	v16.d[1], v26.d[0]
	ext	v24.16b, v24.16b, v24.16b, #8
	ext	v25.16b, v25.16b, v25.16b, #8
	trn1	v26.8h, v2.8h, v4.8h
	trn2	v2.8h, v2.8h, v4.8h
	mov	v17.d[1], v24.d[0]
	trn1	v4.8h, v0.8h, v1.8h
	trn1	v24.4s, v26.4s, v4.4s
	mov	v31.d[1], v24.d[0]
	stp	q19, q31, [x20, #-128]
	trn2	v0.8h, v0.8h, v1.8h
	trn1	v1.4s, v2.4s, v0.4s
	mov	v18.d[1], v1.d[0]
	stp	q21, q18, [x20, #-96]
	mov	v20.d[1], v25.d[0]
	trn2	v4.4s, v26.4s, v4.4s
	mov	v29.d[1], v4.d[0]
	stp	q22, q29, [x20, #-64]
	trn2	v0.4s, v2.4s, v0.4s
	mov	v5.d[1], v0.d[0]
	stp	q23, q5, [x20, #-32]
	ext	v2.16b, v24.16b, v24.16b, #8
	mov	v28.d[1], v2.d[0]
	ext	v1.16b, v1.16b, v1.16b, #8
	stp	q6, q28, [x20]
	mov	v3.d[1], v1.d[0]
	ext	v1.16b, v4.16b, v4.16b, #8
	mov	v30.d[1], v1.d[0]
	stp	q16, q3, [x20, #32]
	ext	v0.16b, v0.16b, v0.16b, #8
	mov	v7.d[1], v0.d[0]
	stp	q17, q30, [x20, #64]
	stp	q20, q7, [x20, #96]
	add	x21, x21, #4
	add	x19, x19, #256
	add	x20, x20, #256
	cmp	x21, #60
	b.lo	LBB6_1
// %bb.2:
	ldr	x8, [sp, #1288]
Lloh29:
	adrp	x9, :got:__stack_chk_guard
Lloh30:
	ldr	x9, [x9, :got_lo12:__stack_chk_guard]
Lloh31:
	ldr	x9, [x9]
	cmp	x9, x8
	b.ne	LBB6_4
// %bb.3:
	add	sp, sp, #1296
	ldp	x29, x30, [sp, #112] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #96] // 16-byte Folded Reload
	ldp	x22, x21, [sp, #80] // 16-byte Folded Reload
	ldp	x24, x23, [sp, #64] // 16-byte Folded Reload
	ldp	d9, d8, [sp, #48] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #32] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #16] // 16-byte Folded Reload
	ldp	d15, d14, [sp], #128 // 16-byte Folded Reload
	ret
LBB6_4:
	bl	__stack_chk_fail
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/70_poly_baseinv_ntt.inc
// begin expanded include: ntt_asm_parts/80_vec_poly_mul8.inc
	.p2align	2 // -- Begin function vec_poly_mul8
vec_poly_mul8: // @vec_poly_mul8
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #192
	stp	d15, d14, [sp, #128] // 16-byte Folded Spill
	stp	d13, d12, [sp, #144] // 16-byte Folded Spill
	stp	d11, d10, [sp, #160] // 16-byte Folded Spill
	stp	d9, d8, [sp, #176] // 16-byte Folded Spill
	.cfi_def_cfa_offset 192
	.cfi_offset b8, -8
	.cfi_offset b9, -16
	.cfi_offset b10, -24
	.cfi_offset b11, -32
	.cfi_offset b12, -40
	.cfi_offset b13, -48
	.cfi_offset b14, -56
	.cfi_offset b15, -64
	mov	w8, #64769 // =0xfd01
	dup	v16.8h, w8
	mov	w8, #769 // =0x301
	dup	v2.8h, w8
	ldp	q18, q20, [x1, #96]
	ldp	q19, q17, [x2, #96]
	smull	v0.4s, v18.4h, v19.4h
	mul	v3.8h, v18.8h, v19.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	smull2	v5.4s, v18.8h, v19.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn	v0.4h, v0.4s, v4.4s
	subhn2	v0.8h, v5.4s, v3.4s
	str	q0, [sp, #112] // 16-byte Folded Spill
	ldp	q8, q28, [x1, #64]
	ldp	q9, q29, [x2, #64]
	smull	v0.4s, v28.4h, v29.4h
	mul	v3.8h, v28.8h, v29.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	smull2	v5.4s, v28.8h, v29.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn	v0.4h, v0.4s, v4.4s
	subhn2	v0.8h, v5.4s, v3.4s
	str	q0, [sp, #96] // 16-byte Folded Spill
	smull	v0.4s, v20.4h, v17.4h
	mul	v3.8h, v20.8h, v17.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	smull2	v5.4s, v20.8h, v17.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn	v0.4h, v0.4s, v4.4s
	subhn2	v0.8h, v5.4s, v3.4s
	str	q0, [sp, #80] // 16-byte Folded Spill
	add	v3.8h, v20.8h, v28.8h
	add	v4.8h, v17.8h, v29.8h
	smull	v0.4s, v3.4h, v4.4h
	mul	v5.8h, v3.8h, v4.8h
	mul	v5.8h, v5.8h, v16.8h
	smull	v6.4s, v5.4h, v2.4h
	smull2	v7.4s, v3.8h, v4.8h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v0.4h, v0.4s, v6.4s
	ldp	q11, q30, [x1, #16]
	ldp	q12, q31, [x2, #16]
	ldr	q25, [x1, #48]
	ldr	q27, [x2, #48]
	subhn2	v0.8h, v7.4s, v5.4s
	str	q0, [sp, #64] // 16-byte Folded Spill
	add	v0.8h, v18.8h, v8.8h
	add	v5.8h, v3.8h, v0.8h
	add	v3.8h, v19.8h, v9.8h
	add	v4.8h, v4.8h, v3.8h
	smull	v6.4s, v5.4h, v4.4h
	mul	v7.8h, v5.8h, v4.8h
	mul	v7.8h, v7.8h, v16.8h
	smull	v21.4s, v7.4h, v2.4h
	smull2	v4.4s, v5.8h, v4.8h
	smull2	v5.4s, v7.8h, v2.8h
	subhn	v1.4h, v6.4s, v21.4s
	subhn2	v1.8h, v4.4s, v5.4s
	str	q1, [sp, #48] // 16-byte Folded Spill
	add	v4.8h, v20.8h, v18.8h
	add	v5.8h, v17.8h, v19.8h
	smull	v6.4s, v4.4h, v5.4h
	mul	v7.8h, v4.8h, v5.8h
	mul	v7.8h, v7.8h, v16.8h
	smull	v21.4s, v7.4h, v2.4h
	smull2	v4.4s, v4.8h, v5.8h
	smull2	v5.4s, v7.8h, v2.8h
	subhn	v1.4h, v6.4s, v21.4s
	subhn2	v1.8h, v4.4s, v5.4s
	str	q1, [sp, #32] // 16-byte Folded Spill
	add	v4.8h, v28.8h, v8.8h
	add	v5.8h, v29.8h, v9.8h
	smull	v6.4s, v4.4h, v5.4h
	mul	v7.8h, v4.8h, v5.8h
	mul	v7.8h, v7.8h, v16.8h
	smull	v21.4s, v7.4h, v2.4h
	smull2	v4.4s, v4.8h, v5.8h
	smull2	v5.4s, v7.8h, v2.8h
	subhn	v1.4h, v6.4s, v21.4s
	subhn2	v1.8h, v4.4s, v5.4s
	str	q1, [sp, #16] // 16-byte Folded Spill
	smull	v4.4s, v8.4h, v9.4h
	mul	v5.8h, v8.8h, v9.8h
	mul	v5.8h, v5.8h, v16.8h
	smull	v6.4s, v5.4h, v2.4h
	smull2	v7.4s, v8.8h, v9.8h
	smull2	v5.4s, v5.8h, v2.8h
	subhn	v10.4h, v4.4s, v6.4s
	subhn2	v10.8h, v7.4s, v5.4s
	smull	v4.4s, v0.4h, v3.4h
	mul	v5.8h, v0.8h, v3.8h
	mul	v5.8h, v5.8h, v16.8h
	smull	v7.4s, v5.4h, v2.4h
	smull2	v0.4s, v0.8h, v3.8h
	smull2	v3.4s, v5.8h, v2.8h
	subhn	v24.4h, v4.4s, v7.4s
	subhn2	v24.8h, v0.4s, v3.4s
	smull	v0.4s, v30.4h, v31.4h
	mul	v3.8h, v30.8h, v31.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	smull2	v5.4s, v30.8h, v31.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn	v23.4h, v0.4s, v4.4s
	subhn2	v23.8h, v5.4s, v3.4s
	smull	v0.4s, v11.4h, v12.4h
	mul	v3.8h, v11.8h, v12.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	smull2	v5.4s, v11.8h, v12.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn	v26.4h, v0.4s, v4.4s
	subhn2	v26.8h, v5.4s, v3.4s
	smull	v0.4s, v25.4h, v27.4h
	mul	v3.8h, v25.8h, v27.8h
	mul	v3.8h, v3.8h, v16.8h
	smull	v4.4s, v3.4h, v2.4h
	subhn	v22.4h, v0.4s, v4.4s
	smull2	v0.4s, v25.8h, v27.8h
	smull2	v3.4s, v3.8h, v2.8h
	subhn2	v22.8h, v0.4s, v3.4s
	add	v0.8h, v25.8h, v11.8h
	add	v7.8h, v27.8h, v12.8h
	smull	v3.4s, v0.4h, v7.4h
	mul	v4.8h, v0.8h, v7.8h
	mul	v4.8h, v4.8h, v16.8h
	smull	v5.4s, v4.4h, v2.4h
	subhn	v1.4h, v3.4s, v5.4s
	smull2	v3.4s, v4.8h, v2.8h
	smull2	v4.4s, v0.8h, v7.8h
	subhn2	v1.8h, v4.4s, v3.4s
	str	q1, [sp] // 16-byte Folded Spill
	ldr	q4, [x1]
	add	v6.8h, v30.8h, v4.8h
	add	v13.8h, v0.8h, v6.8h
	ldr	q3, [x2]
	add	v5.8h, v31.8h, v3.8h
	add	v7.8h, v7.8h, v5.8h
	smull	v14.4s, v13.4h, v7.4h
	mul	v15.8h, v13.8h, v7.8h
	smull2	v7.4s, v13.8h, v7.8h
	mul	v15.8h, v15.8h, v16.8h
	smull	v13.4s, v15.4h, v2.4h
	subhn	v13.4h, v14.4s, v13.4s
	smull2	v14.4s, v15.8h, v2.8h
	subhn2	v13.8h, v7.4s, v14.4s
	add	v7.8h, v25.8h, v30.8h
	add	v14.8h, v27.8h, v31.8h
	smull	v15.4s, v7.4h, v14.4h
	mul	v0.8h, v7.8h, v14.8h
	smull2	v7.4s, v7.8h, v14.8h
	mul	v0.8h, v0.8h, v16.8h
	smull	v14.4s, v0.4h, v2.4h
	subhn	v14.4h, v15.4s, v14.4s
	smull2	v0.4s, v0.8h, v2.8h
	subhn2	v14.8h, v7.4s, v0.4s
	add	v0.8h, v11.8h, v4.8h
	add	v7.8h, v12.8h, v3.8h
	smull	v15.4s, v0.4h, v7.4h
	mul	v1.8h, v0.8h, v7.8h
	smull2	v0.4s, v0.8h, v7.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v7.4s, v1.4h, v2.4h
	subhn	v7.4h, v15.4s, v7.4s
	smull2	v1.4s, v1.8h, v2.8h
	subhn2	v7.8h, v0.4s, v1.4s
	mul	v0.8h, v4.8h, v3.8h
	mul	v0.8h, v0.8h, v16.8h
	smull	v1.4s, v4.4h, v3.4h
	smull	v15.4s, v0.4h, v2.4h
	subhn	v15.4h, v1.4s, v15.4s
	smull2	v0.4s, v0.8h, v2.8h
	smull2	v1.4s, v4.8h, v3.8h
	subhn2	v15.8h, v1.4s, v0.4s
	smull	v0.4s, v6.4h, v5.4h
	mul	v1.8h, v6.8h, v5.8h
	smull2	v6.4s, v6.8h, v5.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v5.4s, v1.4h, v2.4h
	subhn	v5.4h, v0.4s, v5.4s
	smull2	v0.4s, v1.8h, v2.8h
	subhn2	v5.8h, v6.4s, v0.4s
	add	v0.8h, v8.8h, v4.8h
	add	v9.8h, v9.8h, v3.8h
	mul	v1.8h, v0.8h, v9.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v3.4s, v0.4h, v9.4h
	smull	v4.4s, v1.4h, v2.4h
	subhn	v8.4h, v3.4s, v4.4s
	smull2	v1.4s, v1.8h, v2.8h
	smull2	v3.4s, v0.8h, v9.8h
	subhn2	v8.8h, v3.4s, v1.4s
	add	v11.8h, v28.8h, v11.8h
	add	v12.8h, v29.8h, v12.8h
	mul	v1.8h, v11.8h, v12.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v3.4s, v11.4h, v12.4h
	smull	v4.4s, v1.4h, v2.4h
	subhn	v28.4h, v3.4s, v4.4s
	smull2	v1.4s, v1.8h, v2.8h
	smull2	v3.4s, v11.8h, v12.8h
	subhn2	v28.8h, v3.4s, v1.4s
	add	v1.8h, v11.8h, v0.8h
	add	v3.8h, v12.8h, v9.8h
	smull	v4.4s, v1.4h, v3.4h
	smull2	v6.4s, v1.8h, v3.8h
	mul	v1.8h, v1.8h, v3.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v3.4s, v1.4h, v2.4h
	subhn	v29.4h, v4.4s, v3.4s
	smull2	v1.4s, v1.8h, v2.8h
	subhn2	v29.8h, v6.4s, v1.4s
	add	v3.8h, v18.8h, v30.8h
	add	v4.8h, v19.8h, v31.8h
	mul	v1.8h, v3.8h, v4.8h
	mul	v1.8h, v1.8h, v16.8h
	smull	v6.4s, v3.4h, v4.4h
	smull	v18.4s, v1.4h, v2.4h
	subhn	v21.4h, v6.4s, v18.4s
	smull2	v1.4s, v1.8h, v2.8h
	smull2	v6.4s, v3.8h, v4.8h
	subhn2	v21.8h, v6.4s, v1.4s
	add	v6.8h, v20.8h, v25.8h
	add	v20.8h, v17.8h, v27.8h
	mul	v1.8h, v6.8h, v20.8h
	mul	v17.8h, v1.8h, v16.8h
	smull	v1.4s, v6.4h, v20.4h
	smull	v19.4s, v17.4h, v2.4h
	subhn	v1.4h, v1.4s, v19.4s
	smull2	v17.4s, v17.8h, v2.8h
	smull2	v19.4s, v6.8h, v20.8h
	subhn2	v1.8h, v19.4s, v17.4s
	add	v17.8h, v6.8h, v3.8h
	add	v19.8h, v20.8h, v4.8h
	smull	v25.4s, v17.4h, v19.4h
	smull2	v27.4s, v17.8h, v19.8h
	mul	v17.8h, v17.8h, v19.8h
	mul	v17.8h, v17.8h, v16.8h
	smull	v19.4s, v17.4h, v2.4h
	subhn	v19.4h, v25.4s, v19.4s
	smull2	v17.4s, v17.8h, v2.8h
	subhn2	v19.8h, v27.4s, v17.4s
	add	v31.8h, v3.8h, v0.8h
	add	v4.8h, v4.8h, v9.8h
	mul	v0.8h, v31.8h, v4.8h
	mul	v0.8h, v0.8h, v16.8h
	smull	v3.4s, v31.4h, v4.4h
	smull	v17.4s, v0.4h, v2.4h
	subhn	v17.4h, v3.4s, v17.4s
	smull2	v0.4s, v0.8h, v2.8h
	smull2	v3.4s, v31.8h, v4.8h
	subhn2	v17.8h, v3.4s, v0.4s
	add	v6.8h, v6.8h, v11.8h
	add	v9.8h, v20.8h, v12.8h
	mul	v0.8h, v6.8h, v9.8h
	mul	v0.8h, v0.8h, v16.8h
	smull	v3.4s, v6.4h, v9.4h
	smull	v20.4s, v0.4h, v2.4h
	subhn	v25.4h, v3.4s, v20.4s
	smull2	v0.4s, v0.8h, v2.8h
	smull2	v3.4s, v6.8h, v9.8h
	subhn2	v25.8h, v3.4s, v0.4s
	ldp	q11, q27, [sp, #80] // 32-byte Folded Reload
	ldr	q20, [sp, #112] // 16-byte Folded Reload
	add	v0.8h, v11.8h, v20.8h
	ldr	q3, [sp, #32] // 16-byte Folded Reload
	sub	v18.8h, v3.8h, v0.8h
	add	v3.8h, v27.8h, v10.8h
	ldr	q0, [sp, #16] // 16-byte Folded Reload
	sub	v30.8h, v0.8h, v3.8h
	add	v3.8h, v27.8h, v11.8h
	mov	v0.16b, v27.16b
	sub	v3.8h, v20.8h, v3.8h
	mov	v12.16b, v20.16b
	ldr	q20, [sp, #64] // 16-byte Folded Reload
	add	v3.8h, v3.8h, v20.8h
	add	v20.8h, v18.8h, v20.8h
	add	v27.8h, v30.8h, v24.8h
	add	v20.8h, v20.8h, v27.8h
	ldr	q27, [sp, #48] // 16-byte Folded Reload
	sub	v27.8h, v27.8h, v20.8h
	add	v20.8h, v12.8h, v10.8h
	sub	v20.8h, v0.8h, v20.8h
	add	v20.8h, v20.8h, v24.8h
	add	v6.8h, v6.8h, v31.8h
	add	v4.8h, v9.8h, v4.8h
	smull	v24.4s, v6.4h, v4.4h
	smull2	v31.4s, v6.8h, v4.8h
	mul	v4.8h, v6.8h, v4.8h
	mul	v4.8h, v4.8h, v16.8h
	smull	v6.4s, v4.4h, v2.4h
	smull2	v2.4s, v4.8h, v2.8h
	subhn	v4.4h, v24.4s, v6.4s
	subhn2	v4.8h, v31.4s, v2.4s
	add	v2.8h, v26.8h, v22.8h
	sub	v2.8h, v23.8h, v2.8h
	ldr	q0, [sp] // 16-byte Folded Reload
	add	v2.8h, v2.8h, v0.8h
	add	v6.8h, v22.8h, v23.8h
	sub	v6.8h, v14.8h, v6.8h
	add	v16.8h, v26.8h, v15.8h
	sub	v7.8h, v7.8h, v16.8h
	add	v16.8h, v6.8h, v0.8h
	add	v24.8h, v7.8h, v5.8h
	add	v16.8h, v16.8h, v24.8h
	sub	v16.8h, v13.8h, v16.8h
	add	v23.8h, v23.8h, v15.8h
	sub	v23.8h, v26.8h, v23.8h
	add	v5.8h, v23.8h, v5.8h
	add	v23.8h, v28.8h, v8.8h
	sub	v23.8h, v29.8h, v23.8h
	add	v24.8h, v1.8h, v21.8h
	sub	v19.8h, v19.8h, v24.8h
	stp	q15, q7, [x0]
	add	v24.8h, v10.8h, v15.8h
	sub	v24.8h, v2.8h, v24.8h
	add	v24.8h, v24.8h, v8.8h
	add	v7.8h, v30.8h, v7.8h
	sub	v7.8h, v6.8h, v7.8h
	add	v7.8h, v7.8h, v23.8h
	stp	q24, q7, [x0, #64]
	add	v7.8h, v20.8h, v5.8h
	add	v24.8h, v8.8h, v21.8h
	add	v7.8h, v7.8h, v24.8h
	add	v24.8h, v22.8h, v28.8h
	sub	v7.8h, v24.8h, v7.8h
	add	v7.8h, v7.8h, v17.8h
	add	v23.8h, v23.8h, v19.8h
	add	v17.8h, v23.8h, v17.8h
	stp	q5, q16, [x0, #32]
	add	v5.8h, v16.8h, v27.8h
	add	v16.8h, v17.8h, v25.8h
	add	v5.8h, v5.8h, v16.8h
	sub	v4.8h, v4.8h, v5.8h
	stp	q7, q4, [x0, #96]
	add	v4.8h, v10.8h, v21.8h
	add	v2.8h, v3.8h, v2.8h
	add	v5.8h, v28.8h, v1.8h
	add	v2.8h, v2.8h, v5.8h
	sub	v2.8h, v4.8h, v2.8h
	add	v2.8h, v2.8h, v25.8h
	add	v4.8h, v18.8h, v6.8h
	sub	v4.8h, v30.8h, v4.8h
	add	v4.8h, v4.8h, v19.8h
	stp	q2, q4, [x0, #128]
	add	v2.8h, v11.8h, v22.8h
	sub	v2.8h, v20.8h, v2.8h
	add	v1.8h, v2.8h, v1.8h
	stp	q1, q27, [x0, #160]
	stp	q3, q18, [x0, #192]
	str	q11, [x0, #224]
	ldp	d9, d8, [sp, #176] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #160] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #144] // 16-byte Folded Reload
	ldp	d15, d14, [sp, #128] // 16-byte Folded Reload
	add	sp, sp, #192
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: ntt_asm_parts/80_vec_poly_mul8.inc
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

	.p2align	4, 0x0 // @baseinv_zetas_pm
baseinv_zetas_pm:
	.short	-746, 746, -112, 112, -618, 618, -635, 635
	.short	-717, 717, -621, 621, -227, 227, -232, 232
	.short	-698, 698, -212, 212, -236, 236, -21, 21
	.short	-341, 341, -379, 379, -567, 567, -549, 549
	.short	-352, 352, -292, 292, -238, 238, -145, 145
	.short	-194, 194, -493, 493, -70, 70, -495, 495
	.short	-117, 117, -333, 333, -66, 66, -247, 247
	.short	-532, 532, -686, 686, -517, 517, -525, 525
	.short	-331, 331, -528, 528, -167, 167, -357, 357
	.short	-414, 414, -291, 291, -411, 411, -105, 105
	.short	-654, 654, -560, 560, -14, 14, -99, 99
	.short	-509, 509, -29, 29, -366, 366, -391, 391
	.short	-451, 451, -278, 278, -353, 353, -354, 354
	.short	-585, 585, -127, 127, -330, 330, -466, 466
	.short	-222, 222, -691, 691, -421, 421, -725, 725
	.short	-201, 201, -158, 158, -350, 350, -168, 168

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
// end expanded include: ntt_asm_parts/90_constants.inc
