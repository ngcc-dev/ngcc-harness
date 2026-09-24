// Hand-optimized AArch64 assembly kernels for ZEN_128 sampling.
// Derived from split assembly kernels and manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.

// begin expanded include: sample_asm_parts/00_text_header.inc
	.text
// end expanded include: sample_asm_parts/00_text_header.inc
// begin expanded include: sample_asm_parts/10_cbd1.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
lCPI0_0:
	.byte	0
	.byte	1
	.byte	255
	.byte	0
	.byte	0
	.byte	1
	.byte	255
	.byte	0
	.byte	0
	.byte	1
	.byte	255
	.byte	0
	.byte	0
	.byte	1
	.byte	255
	.byte	0
lCPI0_1:
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.byte	1
	.byte	1
	.byte	1
	.byte	1
	.byte	255
	.byte	255
	.byte	255
	.byte	255
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.text
	.globl	cbd1 // -- Begin function cbd1
	.p2align	2
cbd1: // @cbd1
	.cfi_startproc
// %bb.0:
Lloh10:
	adrp	x8, lCPI0_0
Lloh11:
	ldr	q0, [x8, :lo12:lCPI0_0]
Lloh12:
	adrp	x8, lCPI0_1
Lloh13:
	ldr	q1, [x8, :lo12:lCPI0_1]
	movi	v2.8b, #15
	mov	x8, #16 // =0x10
LBB0_1: // =>This Inner Loop Header: Depth=1
	ldr	d3, [x1], #8
	and	v4.8b, v3.8b, v2.8b
	ushr	v6.8b, v3.8b, #4
	mov	v5.16b, v4.16b
	mov	v7.16b, v6.16b
	tbl	v4.8b, { v0.16b }, v4.8b
	tbl	v5.8b, { v1.16b }, v5.8b
	tbl	v6.8b, { v0.16b }, v6.8b
	tbl	v7.8b, { v1.16b }, v7.8b
	sshll	v4.8h, v4.8b, #0
	sshll	v5.8h, v5.8b, #0
	sshll	v6.8h, v6.8b, #0
	sshll	v7.8h, v7.8b, #0
	st4	{ v4.8h, v5.8h, v6.8h, v7.8h }, [x0], #64
	subs	x8, x8, #1
	b.ne	LBB0_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/10_cbd1.inc
// begin expanded include: sample_asm_parts/20_cbd2.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0
lCPI1_0:
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	255
	.byte	0
	.byte	0
	.byte	1
	.byte	255
	.byte	0
	.byte	0
	.byte	1
	.byte	254
	.byte	255
	.byte	255
	.byte	0
	.text
	.globl	cbd2 // -- Begin function cbd2
	.p2align	2
cbd2: // @cbd2
	.cfi_startproc
// %bb.0:
Lloh8:
	adrp	x8, lCPI1_0
Lloh9:
	ldr	q0, [x8, :lo12:lCPI1_0]
	movi	v1.8b, #15
	mov	x8, #32 // =0x20
LBB1_1: // =>This Inner Loop Header: Depth=1
	ldr	d2, [x1], #8
	and	v3.8b, v2.8b, v1.8b
	ushr	v4.8b, v2.8b, #4
	tbl	v3.8b, { v0.16b }, v3.8b
	tbl	v4.8b, { v0.16b }, v4.8b
	sshll	v3.8h, v3.8b, #0
	sshll	v4.8h, v4.8b, #0
	st2	{ v3.8h, v4.8h }, [x0], #32
	subs	x8, x8, #1
	b.ne	LBB1_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/20_cbd2.inc
// begin expanded include: sample_asm_parts/30_tenary1_8.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function tenary1_8
lCPI2_0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
lCPI2_1:
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
	add	x9, x1, #64
	add	x10, x1, #128
	mov	x12, x0
	mov	x13, #8 // =0x8
Lloh0:
	adrp	x11, lCPI2_0
Lloh1:
	ldr	q0, [x11, :lo12:lCPI2_0]
	movi	v1.8h, #1
Lloh2:
	adrp	x11, lCPI2_1
Lloh3:
	ldr	q2, [x11, :lo12:lCPI2_1]
LBB2_1: // =>This Inner Loop Header: Depth=1
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
	b.ne	LBB2_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/30_tenary1_8.inc
// begin expanded include: sample_asm_parts/40_tenary3_32.inc
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function tenary3_32
lCPI3_0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
lCPI3_1:
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
	add	x9, x1, #64
	add	x10, x1, #128
	add	x11, x1, #192
	add	x12, x1, #256
	mov	x13, x0
	mov	x14, #8 // =0x8
Lloh4:
	adrp	x15, lCPI3_0
Lloh5:
	ldr	q0, [x15, :lo12:lCPI3_0]
	movi	v1.8h, #1
Lloh6:
	adrp	x15, lCPI3_1
Lloh7:
	ldr	q2, [x15, :lo12:lCPI3_1]
LBB3_1: // =>This Inner Loop Header: Depth=1
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
	b.ne	LBB3_1
// %bb.2:
	ret
	.cfi_endproc
                                        // -- End function
// end expanded include: sample_asm_parts/40_tenary3_32.inc
