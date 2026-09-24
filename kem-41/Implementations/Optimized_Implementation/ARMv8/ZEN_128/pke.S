// Hand-optimized AArch64 assembly kernels for ZEN_128 PKE operations.
// Derived from split assembly kernels and manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.

// begin expanded include: pke_asm_parts/00_text_header.inc
	.text
// end expanded include: pke_asm_parts/00_text_header.inc
// begin expanded include: pke_asm_parts/10_pke_keygen_derand.inc
	.globl	pke_keygen_derand // -- Begin function pke_keygen_derand
	.p2align	2
pke_keygen_derand: // @pke_keygen_derand
	.cfi_startproc
// %bb.0:
	stp	x24, x23, [sp, #-64]! // 16-byte Folded Spill
	stp	x22, x21, [sp, #16] // 16-byte Folded Spill
	stp	x20, x19, [sp, #32] // 16-byte Folded Spill
	stp	x29, x30, [sp, #48] // 16-byte Folded Spill
	mov	w9, #4352 // =0x1100
Lloh0:
Lloh1:
	sub	sp, sp, #1, lsl #12 // =4096
	sub	sp, sp, #256
	.cfi_def_cfa_offset 4416
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	.cfi_offset w23, -56
	.cfi_offset w24, -64
	mov	x21, x2
	mov	x19, x1
	mov	x20, x0
	add	x0, sp, #3328
	add	x1, sp, #2304
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_fg
	mov	w23, #0 // =0x0
	mov	w22, #2 // =0x2
	b	LBB0_2
LBB0_1: // in Loop: Header=BB0_2 Depth=1
	tst	w23, #0x1
	csinc	w22, w22, wzr, ne
	add	x0, sp, #3328
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_f
	add	w22, w22, #1
	mov	w23, #1 // =0x1
LBB0_2: // =>This Inner Loop Header: Depth=1
	ldr	q0, [sp, #3328]
	ldr	q1, [sp, #3344]
	ldr	q2, [sp, #3584]
	ldr	q3, [sp, #3600]
	eor	v0.16b, v2.16b, v0.16b
	ldr	q2, [sp, #3840]
	ldr	q4, [sp, #3856]
	ldr	q5, [sp, #4096]
	ldr	q6, [sp, #4112]
	eor3	v0.16b, v0.16b, v2.16b, v5.16b
	eor	v1.16b, v3.16b, v1.16b
	eor3	v1.16b, v1.16b, v4.16b, v6.16b
	ldr	q2, [sp, #3360]
	ldr	q3, [sp, #3376]
	ldr	q4, [sp, #3616]
	ldr	q5, [sp, #3632]
	eor	v2.16b, v4.16b, v2.16b
	ldr	q4, [sp, #3872]
	ldr	q6, [sp, #3888]
	ldr	q7, [sp, #4128]
	ldr	q16, [sp, #4144]
	eor3	v2.16b, v2.16b, v4.16b, v7.16b
	eor	v3.16b, v5.16b, v3.16b
	eor3	v3.16b, v3.16b, v6.16b, v16.16b
	ldr	q4, [sp, #3392]
	ldr	q5, [sp, #3408]
	ldr	q6, [sp, #3648]
	ldr	q7, [sp, #3664]
	eor	v4.16b, v6.16b, v4.16b
	ldr	q6, [sp, #3904]
	ldr	q16, [sp, #3920]
	ldr	q17, [sp, #4160]
	ldr	q18, [sp, #4176]
	eor3	v4.16b, v4.16b, v6.16b, v17.16b
	eor	v5.16b, v7.16b, v5.16b
	eor3	v5.16b, v5.16b, v16.16b, v18.16b
	ldr	q6, [sp, #3424]
	ldr	q7, [sp, #3440]
	ldr	q16, [sp, #3680]
	ldr	q17, [sp, #3696]
	eor	v6.16b, v16.16b, v6.16b
	ldr	q16, [sp, #3936]
	ldr	q18, [sp, #3952]
	ldr	q19, [sp, #4192]
	ldr	q20, [sp, #4208]
	eor3	v6.16b, v6.16b, v16.16b, v19.16b
	eor	v7.16b, v17.16b, v7.16b
	eor3	v7.16b, v7.16b, v18.16b, v20.16b
	ldr	q16, [sp, #3456]
	ldr	q17, [sp, #3472]
	ldr	q18, [sp, #3712]
	ldr	q19, [sp, #3728]
	eor	v16.16b, v18.16b, v16.16b
	ldr	q18, [sp, #3968]
	ldr	q20, [sp, #3984]
	ldr	q21, [sp, #4224]
	ldr	q22, [sp, #4240]
	eor3	v16.16b, v16.16b, v18.16b, v21.16b
	eor	v17.16b, v19.16b, v17.16b
	eor3	v17.16b, v17.16b, v20.16b, v22.16b
	ldr	q18, [sp, #3488]
	ldr	q19, [sp, #3504]
	ldr	q20, [sp, #3744]
	ldr	q21, [sp, #3760]
	eor	v18.16b, v20.16b, v18.16b
	ldr	q20, [sp, #4000]
	ldr	q22, [sp, #4016]
	ldr	q23, [sp, #4256]
	ldr	q24, [sp, #4272]
	eor3	v18.16b, v18.16b, v20.16b, v23.16b
	eor	v19.16b, v21.16b, v19.16b
	eor3	v19.16b, v19.16b, v22.16b, v24.16b
	ldr	q20, [sp, #3520]
	ldr	q21, [sp, #3536]
	ldr	q22, [sp, #3776]
	ldr	q23, [sp, #3792]
	eor	v20.16b, v22.16b, v20.16b
	ldr	q22, [sp, #4032]
	ldr	q24, [sp, #4048]
	ldr	q25, [sp, #4288]
	ldr	q26, [sp, #4304]
	eor3	v20.16b, v20.16b, v22.16b, v25.16b
	eor	v21.16b, v23.16b, v21.16b
	eor3	v21.16b, v21.16b, v24.16b, v26.16b
	ldr	q22, [sp, #3552]
	ldr	q23, [sp, #3568]
	ldr	q24, [sp, #3808]
	ldr	q25, [sp, #3824]
	eor	v22.16b, v24.16b, v22.16b
	ldr	q24, [sp, #4064]
	ldr	q26, [sp, #4080]
	ldr	q27, [sp, #4320]
	ldr	q28, [sp, #4336]
	eor3	v22.16b, v22.16b, v24.16b, v27.16b
	movi	v24.8h, #1
	movi	v29.2d, #0000000000000000
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v1.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1024]
	str	q1, [sp, #1040]
	and	v0.16b, v2.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v3.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1056]
	str	q1, [sp, #1072]
	and	v0.16b, v4.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v5.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1088]
	str	q1, [sp, #1104]
	and	v0.16b, v6.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v7.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1120]
	str	q1, [sp, #1136]
	and	v0.16b, v16.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v17.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1152]
	str	q1, [sp, #1168]
	and	v0.16b, v18.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v19.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1184]
	str	q1, [sp, #1200]
	and	v0.16b, v20.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	and	v1.16b, v21.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	str	q0, [sp, #1216]
	str	q1, [sp, #1232]
	eor	v0.16b, v25.16b, v23.16b
	and	v1.16b, v22.16b, v24.16b
	eor	v29.16b, v29.16b, v1.16b
	eor3	v0.16b, v0.16b, v26.16b, v28.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q1, [sp, #1248]
	str	q0, [sp, #1264]
	ext	v30.16b, v29.16b, v29.16b, #8
	eor	v29.8b, v29.8b, v30.8b
	dup	v30.4h, v29.h[1]
	dup	v31.4h, v29.h[2]
	eor	v30.8b, v30.8b, v31.8b
	eor	v30.8b, v30.8b, v29.8b
	dup	v29.4h, v29.h[3]
	cmeq	v29.4h, v30.4h, v29.4h
	umov	w0, v29.h[0]
	and	w0, w0, #0x1
	cbnz	w0, LBB0_1
// %bb.3:                                ;   in Loop: Header=BB0_2 Depth=1
	add	x0, sp, #3328
	bl	poly_ntt_mq
	add	x0, sp, #3328
	bl	check_poly_inv_Zq
	cbnz	w0, LBB0_1
// %bb.4:
	mov	x0, sp
	add	x1, sp, #3328
	bl	poly_baseinv_ntt
	add	x0, sp, #2048
	add	x1, sp, #1024
	bl	FastInversion
	tbz	w23, #0, LBB0_6
// %bb.5:
	add	w23, w22, #1
	add	x0, sp, #2304
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_g
	mov	x22, x23
LBB0_6:
	add	x0, sp, #2304
	bl	poly_ntt_mq
	add	x0, sp, #2304
	bl	check_poly_inv_Zq
	cbz	w0, LBB0_8
LBB0_7: // =>This Inner Loop Header: Depth=1
	add	w23, w22, #1
	add	x0, sp, #2304
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_g
	add	x0, sp, #2304
	bl	poly_ntt_mq
	add	x0, sp, #2304
	bl	check_poly_inv_Zq
	mov	x22, x23
	cbnz	w0, LBB0_7
LBB0_8:
	add	x0, sp, #1024
	add	x1, sp, #2304
	mov	x2, sp
	bl	poly_basemul_ntt_mq
	add	x1, sp, #1024
	mov	x0, x20
	bl	poly_publickey_pack
	add	x1, sp, #3328
	mov	x0, x19
	bl	poly_secretkey_pack
	add	x0, x19, #640
	add	x1, sp, #2048
	mov	w2, #128 // =0x80
	bl	poly_bit2byte_pack
	add	sp, sp, #1, lsl #12 // =4096
	add	sp, sp, #256
	ldp	x29, x30, [sp, #48] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #32] // 16-byte Folded Reload
	ldp	x22, x21, [sp, #16] // 16-byte Folded Reload
	ldp	x24, x23, [sp], #64 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/10_pke_keygen_derand.inc
// begin expanded include: pke_asm_parts/20_pke_keygen.inc
	.globl	pke_keygen // -- Begin function pke_keygen
	.p2align	2
pke_keygen: // @pke_keygen
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #96
	stp	x20, x19, [sp, #64] // 16-byte Folded Spill
	stp	x29, x30, [sp, #80] // 16-byte Folded Spill
	.cfi_def_cfa_offset 96
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	mov	x19, x1
	mov	x20, x0
Lloh2:
	adrp	x0, :got:drng_algorithm
Lloh3:
	ldr	x0, [x0, :got_lo12:drng_algorithm]
	mov	x1, sp
	mov	w2, #512 // =0x200
	bl	get_random_number
	mov	x2, sp
	mov	x0, x20
	mov	x1, x19
	bl	pke_keygen_derand
	ldp	x29, x30, [sp, #80] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #64] // 16-byte Folded Reload
	add	sp, sp, #96
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/20_pke_keygen.inc
// begin expanded include: pke_asm_parts/30_pke_enc.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function pke_enc
lCPI2_0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
	.text
	.globl	pke_enc
	.p2align	2
pke_enc: // @pke_enc
	.cfi_startproc
// %bb.0:
	stp	x22, x21, [sp, #-48]! // 16-byte Folded Spill
	stp	x20, x19, [sp, #16] // 16-byte Folded Spill
	stp	x29, x30, [sp, #32] // 16-byte Folded Spill
	mov	w9, #5120 // =0x1400
Lloh4:
Lloh5:
	sub	sp, sp, #1, lsl #12 // =4096
	sub	sp, sp, #1024
	.cfi_def_cfa_offset 5168
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x3
	mov	x21, x2
	mov	x20, x1
	mov	x1, x0
	add	x0, sp, #1, lsl #12 // =4096
	bl	poly_publickey_unpack
	add	x0, sp, #3072
	add	x1, sp, #2048
	mov	x2, x21
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_se_pair
	mov	x8, #16 // =0x10
Lloh6:
	adrp	x9, lCPI2_0
Lloh7:
	ldr	q0, [x9, :lo12:lCPI2_0]
	movi	v1.16b, #129
	fneg	v1.8h, v1.8h
	add	x9, sp, #256
LBB2_1: // =>This Inner Loop Header: Depth=1
	ldrb	w10, [x20], #1
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	shl	v2.8h, v2.8h, #15
	cmlt	v2.8h, v2.8h, #0
	and	v2.16b, v2.16b, v1.16b
	stur	q2, [x9, #-256]
	str	q2, [x9]
	str	q2, [x9, #256]
	str	q2, [x9, #512]
	add	x9, x9, #16
	subs	x8, x8, #1
	b.ne	LBB2_1
// %bb.2:
	add	x0, sp, #3072
	bl	poly_ntt
	add	x20, sp, #1024
	add	x0, sp, #1024
	add	x1, sp, #1, lsl #12 // =4096
	add	x2, sp, #3072
	bl	poly_basemul_ntt
	add	x0, sp, #1024
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #64 // =0x40
	add	x10, sp, #2048
	mov	x11, sp
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w12, #769 // =0x301
	dup	v3.8h, w12
LBB2_3: // =>This Inner Loop Header: Depth=1
	ldr	q4, [x20, x8]
	ldr	q5, [x10, x8]
	add	v4.8h, v5.8h, v4.8h
	ldr	q5, [x11, x8]
	add	v4.8h, v4.8h, v5.8h
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
	str	q4, [x11, x8]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	LBB2_3
// %bb.4:
	mov	x0, sp
	bl	poly_compress
	mov	x1, sp
	mov	x0, x19
	bl	poly_ciphertext_pack
	add	sp, sp, #1, lsl #12 // =4096
	add	sp, sp, #1024
	ldp	x29, x30, [sp, #32] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #16] // 16-byte Folded Reload
	ldp	x22, x21, [sp], #48 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/30_pke_enc.inc
// begin expanded include: pke_asm_parts/40_pke_dec.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function pke_dec
lCPI3_0:
	.short	0 // 0x0
	.short	1 // 0x1
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
	.short	6 // 0x6
	.short	7 // 0x7
	.text
	.globl	pke_dec
	.p2align	2
pke_dec: // @pke_dec
	.cfi_startproc
// %bb.0:
	stp	x22, x21, [sp, #-48]! // 16-byte Folded Spill
	stp	x20, x19, [sp, #16] // 16-byte Folded Spill
	stp	x29, x30, [sp, #32] // 16-byte Folded Spill
	mov	w9, #4608 // =0x1200
Lloh8:
Lloh9:
	sub	sp, sp, #1, lsl #12 // =4096
	sub	sp, sp, #512
	.cfi_def_cfa_offset 4656
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x2
	mov	x20, x1
	mov	x21, x0
	add	x22, x21, #640
	add	x0, sp, #3584
	mov	x1, x21
	bl	poly_secretkey_unpack
	add	x21, sp, #1536
	add	x0, sp, #1536
	mov	x1, x20
	bl	poly_ciphertext_unpack
	add	x0, sp, #1536
	bl	poly_decompress
	add	x0, sp, #1536
	bl	poly_ntt
	add	x20, sp, #512
	add	x0, sp, #512
	add	x1, sp, #1536
	add	x2, sp, #3584
	bl	poly_basemul_ntt
	add	x0, sp, #512
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #32 // =0x20
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w10, #769 // =0x301
	dup	v3.8h, w10
	movi	v4.8h, #1
LBB3_1: // =>This Inner Loop Header: Depth=1
	add	x10, x20, x8
	ldr	q5, [x10]
	ldr	q6, [x10, #512]
	sub	v7.8h, v6.8h, v5.8h
	smull	v16.4s, v7.4h, v0.4h
	smull2	v17.4s, v7.8h, v1.8h
	mul	v7.8h, v7.8h, v2.8h
	smull	v18.4s, v7.4h, v3.4h
	smull2	v7.4s, v7.8h, v3.8h
	subhn	v16.4h, v16.4s, v18.4s
	subhn2	v16.8h, v17.4s, v7.4s
	add	v5.8h, v6.8h, v5.8h
	smull	v6.4s, v5.4h, v0.4h
	smull2	v7.4s, v5.8h, v1.8h
	mul	v5.8h, v5.8h, v2.8h
	smull	v17.4s, v5.4h, v3.4h
	smull2	v5.4s, v5.8h, v3.8h
	subhn	v6.4h, v6.4s, v17.4s
	subhn2	v6.8h, v7.4s, v5.4s
	add	x11, x21, x8
	str	q16, [x11]
	str	q6, [x11, #512]
	eor	v5.16b, v6.16b, v16.16b
	and	v5.16b, v5.16b, v4.16b
	str	q5, [x10]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	LBB3_1
// %bb.2:
	add	x0, sp, #512
	mov	x1, x22
	add	x2, sp, #2560
	bl	mul_in_R2_256_packed128
	mov	x8, #0 // =0x0
	mov	x9, #16 // =0x10
	add	x10, sp, #1536
	movi	v0.8h, #1
	mov	w11, #384 // =0x180
	dup	v1.8h, w11
	mov	x11, sp
LBB3_3: // =>This Inner Loop Header: Depth=1
	add	x12, x10, x8
	ldr	q2, [x12]
	ldr	q3, [x12, #256]
	ldr	q4, [x12, #512]
	ldr	q5, [x12, #768]
	eor	v6.16b, v3.16b, v2.16b
	eor3	v6.16b, v6.16b, v4.16b, v5.16b
	and	v6.16b, v6.16b, v0.16b
	abs	v2.8h, v2.8h
	sub	v2.8h, v1.8h, v2.8h
	abs	v4.8h, v4.8h
	sub	v4.8h, v1.8h, v4.8h
	smin	v2.8h, v2.8h, v4.8h
	abs	v3.8h, v3.8h
	sub	v3.8h, v1.8h, v3.8h
	abs	v4.8h, v5.8h
	sub	v4.8h, v1.8h, v4.8h
	smin	v3.8h, v3.8h, v4.8h
	cmgt	v2.8h, v2.8h, v3.8h
	bic	v3.16b, v6.16b, v2.16b
	add	x12, x11, x8
	str	q3, [x12]
	and	v2.16b, v6.16b, v2.16b
	str	q2, [x12, #256]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	LBB3_3
// %bb.4:
	mov	x0, sp
	mov	x1, x22
	add	x2, sp, #512
	bl	mul_in_R2_256_packed128
	ldr	q0, [sp, #2560]
	ldr	q2, [sp, #2576]
	ldp	q1, q3, [sp, #512]
	eor	v1.16b, v1.16b, v0.16b
	movi	v0.8h, #1
	and	v4.16b, v1.16b, v0.16b
Lloh10:
	adrp	x8, lCPI3_0
Lloh11:
	ldr	q1, [x8, :lo12:lCPI3_0]
	sshl	v4.8h, v4.8h, v1.8h
	addp	v4.8h, v4.8h, v4.8h
	addp	v4.8h, v4.8h, v4.8h
	addp	v4.8h, v4.8h, v4.8h
	str	b4, [x19]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #1]
	ldr	q2, [sp, #2592]
	ldr	q3, [sp, #2608]
	ldp	q4, q5, [sp, #544]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #2]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #3]
	ldr	q2, [sp, #2624]
	ldr	q3, [sp, #2640]
	ldp	q4, q5, [sp, #576]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #4]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #5]
	ldr	q2, [sp, #2656]
	ldr	q3, [sp, #2672]
	ldp	q4, q5, [sp, #608]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #6]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #7]
	ldr	q2, [sp, #2688]
	ldr	q3, [sp, #2704]
	ldp	q4, q5, [sp, #640]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #8]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #9]
	ldr	q2, [sp, #2720]
	ldr	q3, [sp, #2736]
	ldp	q4, q5, [sp, #672]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #10]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #11]
	ldr	q2, [sp, #2752]
	ldr	q3, [sp, #2768]
	ldp	q4, q5, [sp, #704]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #12]
	eor	v2.16b, v5.16b, v3.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #13]
	ldr	q2, [sp, #2784]
	ldr	q3, [sp, #2800]
	ldp	q4, q5, [sp, #736]
	eor	v2.16b, v4.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #14]
	eor	v2.16b, v5.16b, v3.16b
	and	v0.16b, v2.16b, v0.16b
	sshl	v0.8h, v0.8h, v1.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	str	b0, [x19, #15]
	add	sp, sp, #1, lsl #12 // =4096
	add	sp, sp, #512
	ldp	x29, x30, [sp, #32] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #16] // 16-byte Folded Reload
	ldp	x22, x21, [sp], #48 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/40_pke_dec.inc
