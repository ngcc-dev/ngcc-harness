// Hand-optimized AArch64 assembly kernels for ZEN_256 PKE operations.
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
	mov	w9, #8704 // =0x2200
Lloh256_kg_chk0:
Lloh256_kg_chk1:
	sub	sp, sp, #2, lsl #12
	sub	sp, sp, #512
	.cfi_def_cfa_offset 8768
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
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2560
	add	x1, sp, #1, lsl #12
	add	x1, x1, #512
	mov	x2, x21
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_fg
	mov	w22, #2 // =0x2
	mov	w24, #0 // =0x0
	b	Lzen256_kg_check_f
Lzen256_kg_retry_f: // in Loop: Header=kg_check_f Depth=1
	tst	w24, #0x1
	csinc	w22, w22, wzr, ne
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2560
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_f
	add	w22, w22, #1
	mov	w24, #1 // =0x1
Lzen256_kg_check_f: // =>This Inner Loop Header: Depth=1
	movi	v24.8h, #1
	movi	v29.2d, #0000000000000000
	add	x10, sp, #1, lsl #12
	add	x10, x10, #2560
	add	x11, sp, #2048
	mov	x12, #2
Lzen256_kg_parity_chunk:
	ldr	q0, [x10, #0]
	ldr	q1, [x10, #512]
	ldr	q2, [x10, #1024]
	ldr	q3, [x10, #1536]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #0]
	ldr	q0, [x10, #16]
	ldr	q1, [x10, #528]
	ldr	q2, [x10, #1040]
	ldr	q3, [x10, #1552]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #16]
	ldr	q0, [x10, #32]
	ldr	q1, [x10, #544]
	ldr	q2, [x10, #1056]
	ldr	q3, [x10, #1568]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #32]
	ldr	q0, [x10, #48]
	ldr	q1, [x10, #560]
	ldr	q2, [x10, #1072]
	ldr	q3, [x10, #1584]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #48]
	ldr	q0, [x10, #64]
	ldr	q1, [x10, #576]
	ldr	q2, [x10, #1088]
	ldr	q3, [x10, #1600]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #64]
	ldr	q0, [x10, #80]
	ldr	q1, [x10, #592]
	ldr	q2, [x10, #1104]
	ldr	q3, [x10, #1616]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #80]
	ldr	q0, [x10, #96]
	ldr	q1, [x10, #608]
	ldr	q2, [x10, #1120]
	ldr	q3, [x10, #1632]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #96]
	ldr	q0, [x10, #112]
	ldr	q1, [x10, #624]
	ldr	q2, [x10, #1136]
	ldr	q3, [x10, #1648]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #112]
	ldr	q0, [x10, #128]
	ldr	q1, [x10, #640]
	ldr	q2, [x10, #1152]
	ldr	q3, [x10, #1664]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #128]
	ldr	q0, [x10, #144]
	ldr	q1, [x10, #656]
	ldr	q2, [x10, #1168]
	ldr	q3, [x10, #1680]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #144]
	ldr	q0, [x10, #160]
	ldr	q1, [x10, #672]
	ldr	q2, [x10, #1184]
	ldr	q3, [x10, #1696]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #160]
	ldr	q0, [x10, #176]
	ldr	q1, [x10, #688]
	ldr	q2, [x10, #1200]
	ldr	q3, [x10, #1712]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #176]
	ldr	q0, [x10, #192]
	ldr	q1, [x10, #704]
	ldr	q2, [x10, #1216]
	ldr	q3, [x10, #1728]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #192]
	ldr	q0, [x10, #208]
	ldr	q1, [x10, #720]
	ldr	q2, [x10, #1232]
	ldr	q3, [x10, #1744]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #208]
	ldr	q0, [x10, #224]
	ldr	q1, [x10, #736]
	ldr	q2, [x10, #1248]
	ldr	q3, [x10, #1760]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #224]
	ldr	q0, [x10, #240]
	ldr	q1, [x10, #752]
	ldr	q2, [x10, #1264]
	ldr	q3, [x10, #1776]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #240]
	add	x10, x10, #256
	add	x11, x11, #256
	subs	x12, x12, #1
	b.ne	Lzen256_kg_parity_chunk
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
	cbnz	w0, Lzen256_kg_retry_f
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2560
	bl	poly_ntt_mq
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2560
	bl	check_poly_inv_Zq
	cbnz	w0, Lzen256_kg_retry_f
	mov	x0, sp
	add	x1, sp, #1, lsl #12
	add	x1, x1, #2560
	bl	poly_baseinv_ntt
	add	x0, sp, #1, lsl #12
	add	x1, sp, #2048
	bl	FastInversion
	tbz	w24, #0, Lzen256_kg_check_g
Lzen256_kg_retry_g: // =>This Inner Loop Header: Depth=1
	add	w23, w22, #1
	add	x0, sp, #1, lsl #12
	add	x0, x0, #512
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_g
	mov	w22, w23
Lzen256_kg_check_g:
	add	x0, sp, #1, lsl #12
	add	x0, x0, #512
	bl	poly_ntt_mq
	add	x0, sp, #1, lsl #12
	add	x0, x0, #512
	bl	check_poly_inv_Zq
	cbnz	w0, Lzen256_kg_retry_g
	add	x0, sp, #2048
	add	x1, sp, #1, lsl #12
	add	x1, x1, #512
	mov	x2, sp
	bl	poly_basemul_ntt_mq
	add	x1, sp, #2048
	mov	x0, x20
	bl	poly_publickey_pack
	add	x1, sp, #1, lsl #12
	add	x1, x1, #2560
	mov	x0, x19
	bl	poly_secretkey_pack
	add	x0, x19, #1280
	add	x1, sp, #1, lsl #12
	mov	w2, #256 // =0x100
	bl	poly_bit2byte_pack
	add	sp, sp, #2, lsl #12
	add	sp, sp, #512
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
Lloh256_keygen_rng0:
	adrp	x0, :got:drng_algorithm
Lloh256_keygen_rng1:
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
lCPI256_enc_shifts:
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
	mov	w9, #10240 // =0x2800
Lloh256_enc_chk0:
Lloh256_enc_chk1:
	sub	sp, sp, #2, lsl #12
	sub	sp, sp, #2048
	.cfi_def_cfa_offset 10288
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
	add	x0, sp, #2, lsl #12
	bl	poly_publickey_unpack
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2048
	add	x1, sp, #1, lsl #12
	mov	x2, x21
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_se_pair
	mov	x8, #32 // =0x20
Lloh256_enc_sh0:
	adrp	x9, lCPI256_enc_shifts
Lloh256_enc_sh1:
	ldr	q0, [x9, :lo12:lCPI256_enc_shifts]
	movi	v1.16b, #129
	fneg	v1.8h, v1.8h
	mov	x9, sp
Lzen256_enc_msg_loop: // =>This Inner Loop Header: Depth=1
	ldrb	w10, [x20], #1
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	shl	v2.8h, v2.8h, #15
	cmlt	v2.8h, v2.8h, #0
	and	v2.16b, v2.16b, v1.16b
	str	q2, [x9]
	str	q2, [x9, #512]
	str	q2, [x9, #1024]
	str	q2, [x9, #1536]
	add	x9, x9, #16
	subs	x8, x8, #1
	b.ne	Lzen256_enc_msg_loop
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2048
	bl	poly_ntt
	add	x20, sp, #2048
	add	x0, sp, #2048
	add	x1, sp, #2, lsl #12
	add	x2, sp, #1, lsl #12
	add	x2, x2, #2048
	bl	poly_basemul_ntt
	add	x0, sp, #2048
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #128 // =0x80
	add	x10, sp, #1, lsl #12
	mov	x11, sp
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w12, #769 // =0x301
	dup	v3.8h, w12
Lzen256_enc_mont_loop: // =>This Inner Loop Header: Depth=1
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
	b.ne	Lzen256_enc_mont_loop
	mov	x0, sp
	bl	poly_compress
	mov	x1, sp
	mov	x0, x19
	bl	poly_ciphertext_pack
	add	sp, sp, #2, lsl #12
	add	sp, sp, #2048
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
lCPI256_dec_shifts:
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
	mov	w9, #9216 // =0x2400
Lloh256_dec_chk0:
Lloh256_dec_chk1:
	sub	sp, sp, #2, lsl #12
	sub	sp, sp, #1024
	.cfi_def_cfa_offset 9264
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x2
	mov	x20, x1
	mov	x21, x0
	add	x22, x21, #1280
	add	x0, sp, #1, lsl #12
	add	x0, x0, #3072
	mov	x1, x21
	bl	poly_secretkey_unpack
	add	x21, sp, #3072
	add	x0, sp, #3072
	mov	x1, x20
	bl	poly_ciphertext_unpack
	add	x0, sp, #3072
	bl	poly_decompress
	add	x0, sp, #3072
	bl	poly_ntt
	add	x20, sp, #1024
	add	x0, sp, #1024
	add	x1, sp, #3072
	add	x2, sp, #1, lsl #12
	add	x2, x2, #3072
	bl	poly_basemul_ntt
	add	x0, sp, #1024
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #64 // =0x40
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w10, #769 // =0x301
	dup	v3.8h, w10
	movi	v4.8h, #1
Lzen256_dec_fold_loop: // =>This Inner Loop Header: Depth=1
	add	x10, x20, x8
	ldr	q5, [x10]
	ldr	q6, [x10, #1024]
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
	str	q6, [x11, #1024]
	eor	v5.16b, v6.16b, v16.16b
	and	v5.16b, v5.16b, v4.16b
	str	q5, [x10]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	Lzen256_dec_fold_loop
	add	x0, sp, #1024
	mov	x1, x22
	add	x2, sp, #1, lsl #12
	add	x2, x2, #1024
	bl	mul_in_R2_512_packed256
	mov	x8, #0 // =0x0
	mov	x9, #32 // =0x20
	add	x10, sp, #3072
	movi	v0.8h, #1
	mov	w11, #384 // =0x180
	dup	v1.8h, w11
	mov	x11, sp
Lzen256_dec_simple_loop: // =>This Inner Loop Header: Depth=1
	add	x12, x10, x8
	ldr	q2, [x12]
	ldr	q3, [x12, #512]
	ldr	q4, [x12, #1024]
	ldr	q5, [x12, #1536]
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
	str	q2, [x12, #512]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	Lzen256_dec_simple_loop
	mov	x0, sp
	mov	x1, x22
	add	x2, sp, #1024
	bl	mul_in_R2_512_packed256
	movi	v0.8h, #1
Lloh256_dec_sh0:
	adrp	x8, lCPI256_dec_shifts
Lloh256_dec_sh1:
	ldr	q1, [x8, :lo12:lCPI256_dec_shifts]
	ldr	q2, [sp, #5120]
	ldr	q3, [sp, #1024]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #0]
	ldr	q2, [sp, #5136]
	ldr	q3, [sp, #1040]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #1]
	ldr	q2, [sp, #5152]
	ldr	q3, [sp, #1056]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #2]
	ldr	q2, [sp, #5168]
	ldr	q3, [sp, #1072]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #3]
	ldr	q2, [sp, #5184]
	ldr	q3, [sp, #1088]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #4]
	ldr	q2, [sp, #5200]
	ldr	q3, [sp, #1104]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #5]
	ldr	q2, [sp, #5216]
	ldr	q3, [sp, #1120]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #6]
	ldr	q2, [sp, #5232]
	ldr	q3, [sp, #1136]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #7]
	ldr	q2, [sp, #5248]
	ldr	q3, [sp, #1152]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #8]
	ldr	q2, [sp, #5264]
	ldr	q3, [sp, #1168]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #9]
	ldr	q2, [sp, #5280]
	ldr	q3, [sp, #1184]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #10]
	ldr	q2, [sp, #5296]
	ldr	q3, [sp, #1200]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #11]
	ldr	q2, [sp, #5312]
	ldr	q3, [sp, #1216]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #12]
	ldr	q2, [sp, #5328]
	ldr	q3, [sp, #1232]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #13]
	ldr	q2, [sp, #5344]
	ldr	q3, [sp, #1248]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #14]
	ldr	q2, [sp, #5360]
	ldr	q3, [sp, #1264]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #15]
	ldr	q2, [sp, #5376]
	ldr	q3, [sp, #1280]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #16]
	ldr	q2, [sp, #5392]
	ldr	q3, [sp, #1296]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #17]
	ldr	q2, [sp, #5408]
	ldr	q3, [sp, #1312]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #18]
	ldr	q2, [sp, #5424]
	ldr	q3, [sp, #1328]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #19]
	ldr	q2, [sp, #5440]
	ldr	q3, [sp, #1344]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #20]
	ldr	q2, [sp, #5456]
	ldr	q3, [sp, #1360]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #21]
	ldr	q2, [sp, #5472]
	ldr	q3, [sp, #1376]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #22]
	ldr	q2, [sp, #5488]
	ldr	q3, [sp, #1392]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #23]
	ldr	q2, [sp, #5504]
	ldr	q3, [sp, #1408]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #24]
	ldr	q2, [sp, #5520]
	ldr	q3, [sp, #1424]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #25]
	ldr	q2, [sp, #5536]
	ldr	q3, [sp, #1440]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #26]
	ldr	q2, [sp, #5552]
	ldr	q3, [sp, #1456]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #27]
	ldr	q2, [sp, #5568]
	ldr	q3, [sp, #1472]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #28]
	ldr	q2, [sp, #5584]
	ldr	q3, [sp, #1488]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #29]
	ldr	q2, [sp, #5600]
	ldr	q3, [sp, #1504]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #30]
	ldr	q2, [sp, #5616]
	ldr	q3, [sp, #1520]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #31]
	add	sp, sp, #2, lsl #12
	add	sp, sp, #1024
	ldp	x29, x30, [sp, #32] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #16] // 16-byte Folded Reload
	ldp	x22, x21, [sp], #48 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/40_pke_dec.inc
