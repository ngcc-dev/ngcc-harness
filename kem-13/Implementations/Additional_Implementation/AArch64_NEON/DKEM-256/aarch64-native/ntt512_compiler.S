/* Portable symbol decoration: ELF bare names; Mach-O leading underscore. */
#if defined(__APPLE__)
#define DSYM(x) _##x
#else
#define DSYM(x) x
#endif
/*
 * DKE NTT/INVNTT for N=512, Q=7681 — compiler-generated AArch64 assembly.
 * Extracted from Clang -O2 NEON intrinsics (ntt_neon.c, DKE_MODE=512).
 * Key: subhn for Montgomery, ld2/st2+zip1 for small layers, sqdmulh for Barrett.
 * External: _DKE_zetas, DSYM(DKE_montgomery_reduce)
 * License: Apache-2.0 OR ISC OR MIT
 */

.text

.global DSYM(dke_ntt512_compiler)
.p2align 4
DSYM(dke_ntt512_compiler):
	mov	w11, #0x1
	mov	w15, #0x100
	mov	w9, #0x1ff
#if defined(__APPLE__)
	adrp	x8, _DKE_zetas@GOTPAGE
	ldr	x8, [x8, _DKE_zetas@GOTPAGEOFF]
#else
	adrp	x8, :got:DKE_zetas
	ldr	x8, [x8, :got_lo12:DKE_zetas]
#endif
	mov	w10, #0xe201
	dup	v0.4h, w10
	mov	w10, #0x1e01
	dup	v1.4h, w10
.L24:
	mov	x13, #0x0
	mov	x10, x11
	lsl	w11, w15, #1
	mov	w12, w15
	ubfiz	x14, x15, #1, #32
	udiv	w11, w9, w11
	add	w11, w10, w11
	add	w11, w11, #0x1
	ubfiz	x15, x15, #2, #32
	mov	x16, x0
	mov	x17, x10
.L50:
	add	x1, x13, x12
	ldr	h2, [x8, w17, uxtw #1]
	mov	x2, x16
	mov	x3, x13
.L60:
	ldr	q3, [x2, x14]
	ldr	q4, [x2]
	smull	v5.4s, v3.4h, v2.h[0]
	smull2	v3.4s, v3.8h, v2.h[0]
	xtn	v6.4h, v5.4s
	mul	v6.4h, v6.4h, v0.4h
	xtn	v7.4h, v3.4s
	mul	v7.4h, v7.4h, v0.4h
	smlsl	v5.4s, v6.4h, v1.4h
	smlsl	v3.4s, v7.4h, v1.4h
	uzp2	v3.8h, v5.8h, v3.8h
	add	v5.8h, v3.8h, v4.8h
	str	q5, [x2]
	sub	v3.8h, v4.8h, v3.8h
	str	q3, [x2, x14]
	add	x3, x3, #0x8
	add	x2, x2, #0x10
	cmp	x3, x1
	b.lo	.L60
	add	w17, w17, #0x1
	add	x13, x13, x14
	add	x16, x16, x15
	cmp	w17, w11
	b.ne	.L50
	ubfx	x15, x12, #1, #31
	cmp	w12, #0xf
	b.hi	.L24
	lsl	w9, w12, #1
	mov	w11, #0x1ff
	udiv	w9, w11, w9
	add	w9, w10, w9
	add	w9, w9, #0x2
	add	x10, x0, #0x10
	mov	x11, #-0x10
	mov	w12, #0xe201
	dup	v0.4h, w12
	mov	w12, #0x1e01
	dup	v1.4h, w12
.Lf8:
	sub	w12, w9, #0x1
	add	x12, x8, w12, uxtw #1
	add	x13, x8, w9, uxtw #1
	ldp	d2, d3, [x10, #-0x10]
	ldp	d4, d5, [x10]
	ld1r	{ v6.4h }, [x12]
	smull	v3.4s, v3.4h, v6.4h
	ld1r	{ v6.4h }, [x13]
	smull	v5.4s, v5.4h, v6.4h
	xtn	v6.4h, v3.4s
	mul	v6.4h, v6.4h, v0.4h
	smull	v6.4s, v6.4h, v1.4h
	subhn	v3.4h, v3.4s, v6.4s
	xtn	v6.4h, v5.4s
	mul	v6.4h, v6.4h, v0.4h
	smull	v6.4s, v6.4h, v1.4h
	subhn	v5.4h, v5.4s, v6.4s
	add	v6.4h, v2.4h, v3.4h
	sub	v2.4h, v2.4h, v3.4h
	stp	d6, d2, [x10, #-0x10]
	add	v2.4h, v4.4h, v5.4h
	sub	v3.4h, v4.4h, v5.4h
	stp	d2, d3, [x10], #0x20
	add	w9, w9, #0x2
	add	x11, x11, #0x10
	cmp	x11, #0x1f0
	b.lo	.Lf8
	mov	x9, #-0x8
	mov	w10, #0x81
	mov	w11, #0xe201
	dup	v0.4h, w11
	mov	w11, #0x1e01
	dup	v1.4h, w11
	mov	x11, x0
.L180:
	sub	w12, w10, #0x1
	ldrh	w12, [x8, w12, uxtw #1]
	fmov	s2, w12
	ldrh	w13, [x8, w10, uxtw #1]
	mov	v2.h[1], w12
	ld2	{ v3.2s, v4.2s }, [x11], #16
	mov	v2.h[2], w13
	mov	v2.h[3], w13
	smull	v2.4s, v2.4h, v4.4h
	xtn	v5.4h, v2.4s
	mul	v5.4h, v5.4h, v0.4h
	smull	v5.4s, v5.4h, v1.4h
	subhn	v2.4h, v2.4s, v5.4s
	add	v5.4h, v2.4h, v3.4h
	sub	v2.4h, v3.4h, v2.4h
	zip1	v2.4s, v5.4s, v2.4s
	str	q2, [x0]
	add	x9, x9, #0x8
	add	w10, w10, #0x2
	mov	x0, x11
	cmp	x9, #0x1f8
	b.lo	.L180
	ret

.global DSYM(dke_intt512_compiler)
.p2align 4
DSYM(dke_intt512_compiler):
	stp	x20, x19, [sp, #-0x20]!
	stp	x29, x30, [sp, #0x10]
	add	x29, sp, #0x10
	mov	x19, x0
#if defined(__APPLE__)
	adrp	x8, _DKE_zetas@GOTPAGE
	ldr	x8, [x8, _DKE_zetas@GOTPAGEOFF]
#else
	adrp	x8, :got:DKE_zetas
	ldr	x8, [x8, :got_lo12:DKE_zetas]
#endif
	add	x9, x8, #0x1fc
	mov	x10, #-0x8
	mov	w11, #0x2221
	dup	v0.8h, w11
	mov	w11, #0xe201
	dup	v1.4h, w11
	mov	w11, #0x1e01
	dup	v2.4h, w11
	movi	v3.4h, #0x4, lsl #8
	mvni	v4.4h, #0x1e, lsl #8
	mov	x12, x0
	mov	x11, x0
.L224:
	ldr	s5, [x9], #-0x4
	rev32	v5.4h, v5.4h
	zip1	v5.4h, v5.4h, v5.4h
	ld2	{ v6.2s, v7.2s }, [x11], #16
	add	v16.4h, v6.4h, v7.4h
	mov	v17.16b, v16.16b
	mov	v17.d[1], v16.d[0]
	sqdmulh	v17.8h, v17.8h, v0.8h
	sub	v6.4h, v7.4h, v6.4h
	smull	v5.4s, v5.4h, v6.4h
	xtn	v6.4h, v5.4s
	mul	v6.4h, v6.4h, v1.4h
	smull	v6.4s, v6.4h, v2.4h
	subhn	v5.4h, v5.4s, v6.4s
	add	v6.4h, v17.4h, v3.4h
	sshr	v6.4h, v6.4h, #0xb
	mla	v16.4h, v6.4h, v4.4h
	zip1	v5.4s, v16.4s, v5.4s
	str	q5, [x12]
	add	x10, x10, #0x8
	mov	x12, x11
	cmp	x10, #0x1f8
	b.lo	.L224
	add	x9, x8, #0xfe
	add	x10, x19, #0x10
	mov	x11, #-0x10
	mov	w12, #0x2221
	dup	v0.8h, w12
	movi	v1.8h, #0x4, lsl #8
	mvni	v2.8h, #0x1e, lsl #8
	mov	w12, #0xe201
	dup	v3.4h, w12
	mov	w12, #0x1e01
	dup	v4.4h, w12
.L2ac:
	ldp	d5, d6, [x10, #-0x10]
	ldp	d7, d16, [x10]
	sub	v17.4h, v6.4h, v5.4h
	mov	v5.d[1], v7.d[0]
	mov	v6.d[1], v16.d[0]
	sub	x12, x9, #0x2
	add	v5.8h, v6.8h, v5.8h
	sqdmulh	v6.8h, v5.8h, v0.8h
	add	v6.8h, v6.8h, v1.8h
	sshr	v6.8h, v6.8h, #0xb
	mla	v5.8h, v6.8h, v2.8h
	sub	v6.4h, v16.4h, v7.4h
	ld1r	{ v7.4h }, [x9]
	smull	v7.4s, v17.4h, v7.4h
	xtn	v16.4h, v7.4s
	mul	v16.4h, v16.4h, v3.4h
	smull	v16.4s, v16.4h, v4.4h
	subhn	v7.4h, v7.4s, v16.4s
	ld1r	{ v16.4h }, [x12]
	smull	v6.4s, v6.4h, v16.4h
	xtn	v16.4h, v6.4s
	mul	v16.4h, v16.4h, v3.4h
	smull	v16.4s, v16.4h, v4.4h
	subhn	v6.4h, v6.4s, v16.4s
	stur	d5, [x10, #-0x10]
	stur	d7, [x10, #-0x8]
	ext	v5.16b, v5.16b, v5.16b, #0x8
	str	d5, [x10]
	str	d6, [x10, #0x8]
	sub	x9, x9, #0x4
	add	x11, x11, #0x10
	add	x10, x10, #0x20
	cmp	x11, #0x1f0
	b.lo	.L2ac
	mov	w9, #0x3f
	mov	w10, #0x8
	mov	w11, #0x2221
	dup	v0.8h, w11
	movi	v1.8h, #0x4, lsl #8
	mvni	v2.8h, #0x1e, lsl #8
	mov	w11, #0xe201
	dup	v3.4h, w11
	mov	w11, #0x1e01
	dup	v4.4h, w11
.L35c:
	mov	x12, #0x0
	lsl	x11, x10, #1
	lsl	x13, x10, #2
	mov	x14, x19
.L36c:
	add	x15, x12, x10
	ldr	h5, [x8, w9, uxtw #1]
	mov	x16, x14
	mov	x17, x12
.L37c:
	ldr	q6, [x16]
	ldr	q7, [x16, x11]
	add	v16.8h, v7.8h, v6.8h
	sqdmulh	v17.8h, v16.8h, v0.8h
	add	v17.8h, v17.8h, v1.8h
	sshr	v17.8h, v17.8h, #0xb
	mla	v16.8h, v17.8h, v2.8h
	sub	v6.8h, v7.8h, v6.8h
	smull	v7.4s, v6.4h, v5.h[0]
	smull2	v6.4s, v6.8h, v5.h[0]
	xtn	v17.4h, v7.4s
	mul	v17.4h, v17.4h, v3.4h
	xtn	v18.4h, v6.4s
	mul	v18.4h, v18.4h, v3.4h
	smlsl	v7.4s, v17.4h, v4.4h
	smlsl	v6.4s, v18.4h, v4.4h
	uzp2	v6.8h, v7.8h, v6.8h
	str	q16, [x16]
	str	q6, [x16, x11]
	add	x17, x17, #0x8
	add	x16, x16, #0x10
	cmp	x17, x15
	b.lo	.L37c
	sub	w9, w9, #0x1
	add	x12, x12, x11
	add	x14, x14, x13
	cmp	x12, #0x200
	b.lo	.L36c
	cmp	x10, #0x41
	mov	x10, x11
	b.lo	.L35c
	ldrsh	w8, [x8, w9, uxtw #1]
	mov	w20, #0x778
	mul	w0, w8, w20
.L404:
	bl	DSYM(DKE_montgomery_reduce)
	fmov	s0, w0
	mov	x8, #-0x8
	mov	w9, #0x2221
	dup	v1.8h, w9
	movi	v2.8h, #0x4, lsl #8
	mvni	v3.8h, #0x1e, lsl #8
	dup	v4.8h, w20
	mov	w9, #0xe201
	dup	v5.4h, w9
	mov	w9, #0x1e01
	dup	v6.4h, w9
.L434:
	ldr	q7, [x19]
	ldr	q16, [x19, #0x200]
	add	v17.8h, v16.8h, v7.8h
	sqdmulh	v18.8h, v17.8h, v1.8h
	add	v18.8h, v18.8h, v2.8h
	sshr	v18.8h, v18.8h, #0xb
	mla	v17.8h, v18.8h, v3.8h
	sub	v7.8h, v16.8h, v7.8h
	smull	v16.4s, v17.4h, v4.4h
	smull2	v17.4s, v17.8h, v4.8h
	xtn	v18.4h, v16.4s
	mul	v18.4h, v18.4h, v5.4h
	xtn	v19.4h, v17.4s
	mul	v19.4h, v19.4h, v5.4h
	smlsl	v16.4s, v18.4h, v6.4h
	smlsl	v17.4s, v19.4h, v6.4h
	uzp2	v16.8h, v16.8h, v17.8h
	smull	v17.4s, v7.4h, v0.h[0]
	smull2	v7.4s, v7.8h, v0.h[0]
	xtn	v18.4h, v17.4s
	mul	v18.4h, v18.4h, v5.4h
	xtn	v19.4h, v7.4s
	mul	v19.4h, v19.4h, v5.4h
	smlsl	v17.4s, v18.4h, v6.4h
	smlsl	v7.4s, v19.4h, v6.4h
	uzp2	v7.8h, v17.8h, v7.8h
	str	q16, [x19]
	str	q7, [x19, #0x200]
	add	x8, x8, #0x8
	add	x19, x19, #0x10
	cmp	x8, #0xf8
	b.lo	.L434
	ldp	x29, x30, [sp, #0x10]
	ldp	x20, x19, [sp], #0x20
	ret

#if !defined(__APPLE__)
.section .note.GNU-stack,"",%progbits
#endif
