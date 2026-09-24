// Hand-optimized AArch64 assembly kernels for ZEN_512 PKE operations.
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
	mov	w9, #17408 // =0x4400
Lloh512_kg_chk0:
Lloh512_kg_chk1:
	sub	sp, sp, #4, lsl #12
	sub	sp, sp, #1024
	.cfi_def_cfa_offset 17472
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
	add	x0, sp, #3, lsl #12
	add	x0, x0, #1024
	add	x1, sp, #2, lsl #12
	add	x1, x1, #1024
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_fg
	mov	w23, #0 // =0x0
	mov	w22, #2 // =0x2
	b	Lzen512_kg_check_f
Lzen512_kg_retry_f: // in Loop: Header=kg_check_f Depth=1
	tst	w23, #0x1
	csinc	w22, w22, wzr, ne
	add	x0, sp, #3, lsl #12
	add	x0, x0, #1024
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_gf
	add	w22, w22, #1
	mov	w23, #1 // =0x1
Lzen512_kg_check_f: // =>This Inner Loop Header: Depth=1
	movi	v24.8h, #1
	movi	v29.2d, #0000000000000000
	add	x10, sp, #3, lsl #12
	add	x10, x10, #1024
	add	x11, sp, #1, lsl #12
	mov	x12, #4
Lzen512_kg_parity_chunk:
	ldr	q0, [x10, #0]
	ldr	q1, [x10, #1024]
	ldr	q2, [x10, #2048]
	ldr	q3, [x10, #3072]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #0]
	ldr	q0, [x10, #16]
	ldr	q1, [x10, #1040]
	ldr	q2, [x10, #2064]
	ldr	q3, [x10, #3088]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #16]
	ldr	q0, [x10, #32]
	ldr	q1, [x10, #1056]
	ldr	q2, [x10, #2080]
	ldr	q3, [x10, #3104]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #32]
	ldr	q0, [x10, #48]
	ldr	q1, [x10, #1072]
	ldr	q2, [x10, #2096]
	ldr	q3, [x10, #3120]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #48]
	ldr	q0, [x10, #64]
	ldr	q1, [x10, #1088]
	ldr	q2, [x10, #2112]
	ldr	q3, [x10, #3136]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #64]
	ldr	q0, [x10, #80]
	ldr	q1, [x10, #1104]
	ldr	q2, [x10, #2128]
	ldr	q3, [x10, #3152]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #80]
	ldr	q0, [x10, #96]
	ldr	q1, [x10, #1120]
	ldr	q2, [x10, #2144]
	ldr	q3, [x10, #3168]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #96]
	ldr	q0, [x10, #112]
	ldr	q1, [x10, #1136]
	ldr	q2, [x10, #2160]
	ldr	q3, [x10, #3184]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #112]
	ldr	q0, [x10, #128]
	ldr	q1, [x10, #1152]
	ldr	q2, [x10, #2176]
	ldr	q3, [x10, #3200]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #128]
	ldr	q0, [x10, #144]
	ldr	q1, [x10, #1168]
	ldr	q2, [x10, #2192]
	ldr	q3, [x10, #3216]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #144]
	ldr	q0, [x10, #160]
	ldr	q1, [x10, #1184]
	ldr	q2, [x10, #2208]
	ldr	q3, [x10, #3232]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #160]
	ldr	q0, [x10, #176]
	ldr	q1, [x10, #1200]
	ldr	q2, [x10, #2224]
	ldr	q3, [x10, #3248]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #176]
	ldr	q0, [x10, #192]
	ldr	q1, [x10, #1216]
	ldr	q2, [x10, #2240]
	ldr	q3, [x10, #3264]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #192]
	ldr	q0, [x10, #208]
	ldr	q1, [x10, #1232]
	ldr	q2, [x10, #2256]
	ldr	q3, [x10, #3280]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #208]
	ldr	q0, [x10, #224]
	ldr	q1, [x10, #1248]
	ldr	q2, [x10, #2272]
	ldr	q3, [x10, #3296]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #224]
	ldr	q0, [x10, #240]
	ldr	q1, [x10, #1264]
	ldr	q2, [x10, #2288]
	ldr	q3, [x10, #3312]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v0.16b, v0.16b, v2.16b, v3.16b
	and	v0.16b, v0.16b, v24.16b
	eor	v29.16b, v29.16b, v0.16b
	str	q0, [x11, #240]
	add	x10, x10, #256
	add	x11, x11, #256
	subs	x12, x12, #1
	b.ne	Lzen512_kg_parity_chunk
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
	cbnz	w0, Lzen512_kg_retry_f
	add	x0, sp, #3, lsl #12
	add	x0, x0, #1024
	bl	poly_ntt_mq
	add	x0, sp, #3, lsl #12
	add	x0, x0, #1024
	bl	check_poly_inv_Zq
	cbnz	w0, Lzen512_kg_retry_f
	mov	x0, sp
	add	x1, sp, #3, lsl #12
	add	x1, x1, #1024
	bl	poly_baseinv_ntt
	add	x0, sp, #2, lsl #12
	add	x1, sp, #1, lsl #12
	bl	FastInversion
	tbz	w23, #0, Lzen512_kg_check_g
	add	w23, w22, #1
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_gf
	mov	w22, w23
Lzen512_kg_check_g:
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	bl	poly_ntt_mq
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	bl	check_poly_inv_Zq
	cbz	w0, Lzen512_kg_finish
Lzen512_kg_retry_g: // =>This Inner Loop Header: Depth=1
	add	w23, w22, #1
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	and	w2, w22, #0xff
	mov	x1, x21
	bl	poly_generate_gf
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	bl	poly_ntt_mq
	add	x0, sp, #2, lsl #12
	add	x0, x0, #1024
	bl	check_poly_inv_Zq
	mov	w22, w23
	cbnz	w0, Lzen512_kg_retry_g
Lzen512_kg_finish:
	add	x0, sp, #1, lsl #12
	add	x1, sp, #2, lsl #12
	add	x1, x1, #1024
	mov	x2, sp
	bl	poly_basemul_ntt_mq
	add	x1, sp, #1, lsl #12
	mov	x0, x20
	bl	poly_publickey_pack
	add	x1, sp, #3, lsl #12
	add	x1, x1, #1024
	mov	x0, x19
	bl	poly_secretkey_pack
	add	x0, x19, #2560
	add	x1, sp, #2, lsl #12
	mov	w2, #512 // =0x200
	bl	poly_bit2byte_pack
	add	sp, sp, #4, lsl #12
	add	sp, sp, #1024
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
Lloh512_keygen_rng0:
	adrp	x0, :got:drng_algorithm
Lloh512_keygen_rng1:
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
lCPI512_enc_shifts:
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
	mov	w9, #20480 // =0x5000
Lloh512_enc_chk0:
Lloh512_enc_chk1:
	sub	sp, sp, #5, lsl #12
	.cfi_def_cfa_offset 20528
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
	add	x0, sp, #4, lsl #12
	bl	poly_publickey_unpack
	add	x0, sp, #3, lsl #12
	add	x1, sp, #2, lsl #12
	mov	x2, x21
	mov	w3, #0 // =0x0
	mov	w4, #1 // =0x1
	bl	poly_generate_se_pair
	mov	x8, #64 // =0x40
Lloh512_enc_sh0:
	adrp	x9, lCPI512_enc_shifts
Lloh512_enc_sh1:
	ldr	q0, [x9, :lo12:lCPI512_enc_shifts]
	movi	v1.16b, #129
	fneg	v1.8h, v1.8h
	mov	x9, sp
Lzen512_enc_msg_loop: // =>This Inner Loop Header: Depth=1
	ldrb	w10, [x20], #1
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	shl	v2.8h, v2.8h, #15
	cmlt	v2.8h, v2.8h, #0
	and	v2.16b, v2.16b, v1.16b
	str	q2, [x9]
	str	q2, [x9, #1024]
	str	q2, [x9, #2048]
	str	q2, [x9, #3072]
	add	x9, x9, #16
	subs	x8, x8, #1
	b.ne	Lzen512_enc_msg_loop
	add	x0, sp, #3, lsl #12
	bl	poly_ntt
	add	x20, sp, #1, lsl #12
	add	x0, sp, #1, lsl #12
	add	x1, sp, #4, lsl #12
	add	x2, sp, #3, lsl #12
	bl	poly_basemul_ntt
	add	x0, sp, #1, lsl #12
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #256 // =0x100
	add	x10, sp, #2, lsl #12
	mov	x11, sp
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w12, #769 // =0x301
	dup	v3.8h, w12
Lzen512_enc_mont_loop: // =>This Inner Loop Header: Depth=1
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
	b.ne	Lzen512_enc_mont_loop
	mov	x0, sp
	bl	poly_compress
	mov	x1, sp
	mov	x0, x19
	bl	poly_ciphertext_pack
	add	sp, sp, #5, lsl #12
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
lCPI512_dec_shifts:
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
	mov	w9, #18432 // =0x4800
Lloh512_dec_chk0:
Lloh512_dec_chk1:
	sub	sp, sp, #4, lsl #12
	sub	sp, sp, #2048
	.cfi_def_cfa_offset 18480
	.cfi_offset w30, -8
	.cfi_offset w29, -16
	.cfi_offset w19, -24
	.cfi_offset w20, -32
	.cfi_offset w21, -40
	.cfi_offset w22, -48
	mov	x19, x2
	mov	x20, x1
	mov	x21, x0
	add	x22, x21, #2560
	add	x0, sp, #3, lsl #12
	add	x0, x0, #2048
	mov	x1, x21
	bl	poly_secretkey_unpack
	add	x21, sp, #1, lsl #12
	add	x21, x21, #2048
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2048
	mov	x1, x20
	bl	poly_ciphertext_unpack
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2048
	bl	poly_decompress
	add	x0, sp, #1, lsl #12
	add	x0, x0, #2048
	bl	poly_ntt
	add	x20, sp, #2048
	add	x0, sp, #2048
	add	x1, sp, #1, lsl #12
	add	x1, x1, #2048
	add	x2, sp, #3, lsl #12
	add	x2, x2, #2048
	bl	poly_basemul_ntt
	add	x0, sp, #2048
	bl	poly_intt
	mov	x8, #0 // =0x0
	mov	x9, #128 // =0x80
	movi	v0.4h, #171
	movi	v1.8h, #171
	mvni	v2.8h, #84
	mov	w10, #769 // =0x301
	dup	v3.8h, w10
	movi	v4.8h, #1
Lzen512_dec_fold_loop: // =>This Inner Loop Header: Depth=1
	add	x10, x20, x8
	ldr	q5, [x10]
	ldr	q6, [x10, #2048]
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
	str	q6, [x11, #2048]
	eor	v5.16b, v6.16b, v16.16b
	and	v5.16b, v5.16b, v4.16b
	str	q5, [x10]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	Lzen512_dec_fold_loop
	add	x0, sp, #2048
	mov	x1, x22
	add	x2, sp, #2, lsl #12
	add	x2, x2, #2048
	bl	mul_in_R2_1024_packed512
	mov	x8, #0 // =0x0
	mov	x9, #64 // =0x40
	add	x10, sp, #1, lsl #12
	add	x10, x10, #2048
	movi	v0.8h, #1
	mov	w11, #384 // =0x180
	dup	v1.8h, w11
	mov	x11, sp
Lzen512_dec_simple_loop: // =>This Inner Loop Header: Depth=1
	add	x12, x10, x8
	ldr	q2, [x12]
	ldr	q3, [x12, #1024]
	ldr	q4, [x12, #2048]
	ldr	q5, [x12, #3072]
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
	str	q2, [x12, #1024]
	add	x8, x8, #16
	subs	x9, x9, #1
	b.ne	Lzen512_dec_simple_loop
	mov	x0, sp
	mov	x1, x22
	add	x2, sp, #2048
	bl	mul_in_R2_1024_packed512
	movi	v0.8h, #1
Lloh512_dec_sh0:
	adrp	x8, lCPI512_dec_shifts
Lloh512_dec_sh1:
	ldr	q1, [x8, :lo12:lCPI512_dec_shifts]
	ldr	q2, [sp, #10240]
	ldr	q3, [sp, #2048]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #0]
	ldr	q2, [sp, #10256]
	ldr	q3, [sp, #2064]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #1]
	ldr	q2, [sp, #10272]
	ldr	q3, [sp, #2080]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #2]
	ldr	q2, [sp, #10288]
	ldr	q3, [sp, #2096]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #3]
	ldr	q2, [sp, #10304]
	ldr	q3, [sp, #2112]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #4]
	ldr	q2, [sp, #10320]
	ldr	q3, [sp, #2128]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #5]
	ldr	q2, [sp, #10336]
	ldr	q3, [sp, #2144]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #6]
	ldr	q2, [sp, #10352]
	ldr	q3, [sp, #2160]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #7]
	ldr	q2, [sp, #10368]
	ldr	q3, [sp, #2176]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #8]
	ldr	q2, [sp, #10384]
	ldr	q3, [sp, #2192]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #9]
	ldr	q2, [sp, #10400]
	ldr	q3, [sp, #2208]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #10]
	ldr	q2, [sp, #10416]
	ldr	q3, [sp, #2224]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #11]
	ldr	q2, [sp, #10432]
	ldr	q3, [sp, #2240]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #12]
	ldr	q2, [sp, #10448]
	ldr	q3, [sp, #2256]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #13]
	ldr	q2, [sp, #10464]
	ldr	q3, [sp, #2272]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #14]
	ldr	q2, [sp, #10480]
	ldr	q3, [sp, #2288]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #15]
	ldr	q2, [sp, #10496]
	ldr	q3, [sp, #2304]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #16]
	ldr	q2, [sp, #10512]
	ldr	q3, [sp, #2320]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #17]
	ldr	q2, [sp, #10528]
	ldr	q3, [sp, #2336]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #18]
	ldr	q2, [sp, #10544]
	ldr	q3, [sp, #2352]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #19]
	ldr	q2, [sp, #10560]
	ldr	q3, [sp, #2368]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #20]
	ldr	q2, [sp, #10576]
	ldr	q3, [sp, #2384]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #21]
	ldr	q2, [sp, #10592]
	ldr	q3, [sp, #2400]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #22]
	ldr	q2, [sp, #10608]
	ldr	q3, [sp, #2416]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #23]
	ldr	q2, [sp, #10624]
	ldr	q3, [sp, #2432]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #24]
	ldr	q2, [sp, #10640]
	ldr	q3, [sp, #2448]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #25]
	ldr	q2, [sp, #10656]
	ldr	q3, [sp, #2464]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #26]
	ldr	q2, [sp, #10672]
	ldr	q3, [sp, #2480]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #27]
	ldr	q2, [sp, #10688]
	ldr	q3, [sp, #2496]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #28]
	ldr	q2, [sp, #10704]
	ldr	q3, [sp, #2512]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #29]
	ldr	q2, [sp, #10720]
	ldr	q3, [sp, #2528]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #30]
	ldr	q2, [sp, #10736]
	ldr	q3, [sp, #2544]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #31]
	ldr	q2, [sp, #10752]
	ldr	q3, [sp, #2560]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #32]
	ldr	q2, [sp, #10768]
	ldr	q3, [sp, #2576]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #33]
	ldr	q2, [sp, #10784]
	ldr	q3, [sp, #2592]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #34]
	ldr	q2, [sp, #10800]
	ldr	q3, [sp, #2608]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #35]
	ldr	q2, [sp, #10816]
	ldr	q3, [sp, #2624]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #36]
	ldr	q2, [sp, #10832]
	ldr	q3, [sp, #2640]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #37]
	ldr	q2, [sp, #10848]
	ldr	q3, [sp, #2656]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #38]
	ldr	q2, [sp, #10864]
	ldr	q3, [sp, #2672]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #39]
	ldr	q2, [sp, #10880]
	ldr	q3, [sp, #2688]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #40]
	ldr	q2, [sp, #10896]
	ldr	q3, [sp, #2704]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #41]
	ldr	q2, [sp, #10912]
	ldr	q3, [sp, #2720]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #42]
	ldr	q2, [sp, #10928]
	ldr	q3, [sp, #2736]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #43]
	ldr	q2, [sp, #10944]
	ldr	q3, [sp, #2752]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #44]
	ldr	q2, [sp, #10960]
	ldr	q3, [sp, #2768]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #45]
	ldr	q2, [sp, #10976]
	ldr	q3, [sp, #2784]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #46]
	ldr	q2, [sp, #10992]
	ldr	q3, [sp, #2800]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #47]
	ldr	q2, [sp, #11008]
	ldr	q3, [sp, #2816]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #48]
	ldr	q2, [sp, #11024]
	ldr	q3, [sp, #2832]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #49]
	ldr	q2, [sp, #11040]
	ldr	q3, [sp, #2848]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #50]
	ldr	q2, [sp, #11056]
	ldr	q3, [sp, #2864]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #51]
	ldr	q2, [sp, #11072]
	ldr	q3, [sp, #2880]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #52]
	ldr	q2, [sp, #11088]
	ldr	q3, [sp, #2896]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #53]
	ldr	q2, [sp, #11104]
	ldr	q3, [sp, #2912]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #54]
	ldr	q2, [sp, #11120]
	ldr	q3, [sp, #2928]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #55]
	ldr	q2, [sp, #11136]
	ldr	q3, [sp, #2944]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #56]
	ldr	q2, [sp, #11152]
	ldr	q3, [sp, #2960]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #57]
	ldr	q2, [sp, #11168]
	ldr	q3, [sp, #2976]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #58]
	ldr	q2, [sp, #11184]
	ldr	q3, [sp, #2992]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #59]
	ldr	q2, [sp, #11200]
	ldr	q3, [sp, #3008]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #60]
	ldr	q2, [sp, #11216]
	ldr	q3, [sp, #3024]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #61]
	ldr	q2, [sp, #11232]
	ldr	q3, [sp, #3040]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #62]
	ldr	q2, [sp, #11248]
	ldr	q3, [sp, #3056]
	eor	v2.16b, v3.16b, v2.16b
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	str	b2, [x19, #63]
	add	sp, sp, #4, lsl #12
	add	sp, sp, #2048
	ldp	x29, x30, [sp, #32] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #16] // 16-byte Folded Reload
	ldp	x22, x21, [sp], #48 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: pke_asm_parts/40_pke_dec.inc
