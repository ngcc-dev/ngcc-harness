// Hand-optimized AArch64 PMULL assembly for ZEN_512 FastInversion.
// Implements the packed-bit inversion path manually tuned for this parameter set.
// GNU as / ELF syntax for the ARM performance-optimized submission.
	.text
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function FastInversion
lCPI0_0:
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	0 // 0x0
	.byte	1 // 0x1
	.byte	8 // 0x8
	.byte	9 // 0x9
	.byte	16 // 0x10
	.byte	17 // 0x11
	.byte	24 // 0x18
	.byte	25 // 0x19
lCPI0_1:
	.byte	0 // 0x0
	.byte	1 // 0x1
	.byte	8 // 0x8
	.byte	9 // 0x9
	.byte	16 // 0x10
	.byte	17 // 0x11
	.byte	24 // 0x18
	.byte	25 // 0x19
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
	.byte	254 // 0xfe
	.byte	255 // 0xff
lCPI0_6:
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
	.short	6 // 0x6
	.short	7 // 0x7
	.short	8 // 0x8
	.short	9 // 0x9
lCPI0_7:
	.short	4 // 0x4
	.short	8 // 0x8
	.short	16 // 0x10
	.short	32 // 0x20
	.short	64 // 0x40
	.short	128 // 0x80
	.short	256 // 0x100
	.short	512 // 0x200
lCPI0_10:
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
	.short	65528 // 0xfff8
lCPI0_12:
	.long	24 // 0x18
	.long	25 // 0x19
	.long	26 // 0x1a
	.long	27 // 0x1b
lCPI0_13:
	.long	28 // 0x1c
	.long	29 // 0x1d
	.long	30 // 0x1e
	.long	31 // 0x1f
lCPI0_14:
	.long	16 // 0x10
	.long	17 // 0x11
	.long	18 // 0x12
	.long	19 // 0x13
lCPI0_15:
	.long	20 // 0x14
	.long	21 // 0x15
	.long	22 // 0x16
	.long	23 // 0x17
lCPI0_16:
	.quad	-18 // 0xffffffffffffffee
	.quad	-19 // 0xffffffffffffffed
lCPI0_17:
	.quad	-16 // 0xfffffffffffffff0
	.quad	-17 // 0xffffffffffffffef
lCPI0_18:
	.quad	-22 // 0xffffffffffffffea
	.quad	-23 // 0xffffffffffffffe9
lCPI0_19:
	.quad	-20 // 0xffffffffffffffec
	.quad	-21 // 0xffffffffffffffeb
lCPI0_20:
	.quad	-26 // 0xffffffffffffffe6
	.quad	-27 // 0xffffffffffffffe5
lCPI0_21:
	.quad	-24 // 0xffffffffffffffe8
	.quad	-25 // 0xffffffffffffffe7
lCPI0_22:
	.quad	-30 // 0xffffffffffffffe2
	.quad	-31 // 0xffffffffffffffe1
lCPI0_23:
	.quad	-28 // 0xffffffffffffffe4
	.quad	-29 // 0xffffffffffffffe3
lCPI0_24:
	.quad	4 // 0x4
	.quad	5 // 0x5
lCPI0_25:
	.quad	0 // 0x0
	.quad	1 // 0x1
lCPI0_26:
	.quad	6 // 0x6
	.quad	7 // 0x7
lCPI0_27:
	.quad	2 // 0x2
	.quad	3 // 0x3
lCPI0_28:
	.quad	12 // 0xc
	.quad	13 // 0xd
lCPI0_29:
	.quad	8 // 0x8
	.quad	9 // 0x9
lCPI0_30:
	.quad	14 // 0xe
	.quad	15 // 0xf
lCPI0_31:
	.quad	10 // 0xa
	.quad	11 // 0xb
lCPI0_32:
	.quad	20 // 0x14
	.quad	21 // 0x15
lCPI0_33:
	.quad	16 // 0x10
	.quad	17 // 0x11
lCPI0_34:
	.quad	22 // 0x16
	.quad	23 // 0x17
lCPI0_35:
	.quad	18 // 0x12
	.quad	19 // 0x13
lCPI0_36:
	.quad	28 // 0x1c
	.quad	29 // 0x1d
lCPI0_37:
	.quad	24 // 0x18
	.quad	25 // 0x19
lCPI0_38:
	.quad	30 // 0x1e
	.quad	31 // 0x1f
lCPI0_39:
	.quad	26 // 0x1a
	.quad	27 // 0x1b
lCPI0_40:
	.quad	34 // 0x22
	.quad	35 // 0x23
lCPI0_41:
	.quad	38 // 0x26
	.quad	39 // 0x27
lCPI0_42:
	.quad	32 // 0x20
	.quad	33 // 0x21
lCPI0_43:
	.quad	36 // 0x24
	.quad	37 // 0x25
lCPI0_44:
	.quad	42 // 0x2a
	.quad	43 // 0x2b
lCPI0_45:
	.quad	46 // 0x2e
	.quad	47 // 0x2f
lCPI0_46:
	.quad	40 // 0x28
	.quad	41 // 0x29
lCPI0_47:
	.quad	44 // 0x2c
	.quad	45 // 0x2d
lCPI0_48:
	.quad	52 // 0x34
	.quad	53 // 0x35
lCPI0_49:
	.quad	48 // 0x30
	.quad	49 // 0x31
lCPI0_50:
	.quad	54 // 0x36
	.quad	55 // 0x37
lCPI0_51:
	.quad	50 // 0x32
	.quad	51 // 0x33
lCPI0_52:
	.quad	60 // 0x3c
	.quad	61 // 0x3d
lCPI0_53:
	.quad	56 // 0x38
	.quad	57 // 0x39
lCPI0_54:
	.quad	62 // 0x3e
	.quad	63 // 0x3f
lCPI0_55:
	.quad	58 // 0x3a
	.quad	59 // 0x3b
lCPI0_56:
	.quad	-34 // 0xffffffffffffffde
	.quad	-35 // 0xffffffffffffffdd
lCPI0_57:
	.quad	-32 // 0xffffffffffffffe0
	.quad	-33 // 0xffffffffffffffdf
lCPI0_58:
	.quad	-38 // 0xffffffffffffffda
	.quad	-39 // 0xffffffffffffffd9
lCPI0_59:
	.quad	-36 // 0xffffffffffffffdc
	.quad	-37 // 0xffffffffffffffdb
lCPI0_60:
	.quad	-42 // 0xffffffffffffffd6
	.quad	-43 // 0xffffffffffffffd5
lCPI0_61:
	.quad	-40 // 0xffffffffffffffd8
	.quad	-41 // 0xffffffffffffffd7
lCPI0_62:
	.quad	-46 // 0xffffffffffffffd2
	.quad	-47 // 0xffffffffffffffd1
lCPI0_63:
	.quad	-44 // 0xffffffffffffffd4
	.quad	-45 // 0xffffffffffffffd3
lCPI0_64:
	.quad	-50 // 0xffffffffffffffce
	.quad	-51 // 0xffffffffffffffcd
lCPI0_65:
	.quad	-48 // 0xffffffffffffffd0
	.quad	-49 // 0xffffffffffffffcf
lCPI0_66:
	.quad	-54 // 0xffffffffffffffca
	.quad	-55 // 0xffffffffffffffc9
lCPI0_67:
	.quad	-52 // 0xffffffffffffffcc
	.quad	-53 // 0xffffffffffffffcb
lCPI0_68:
	.quad	-58 // 0xffffffffffffffc6
	.quad	-59 // 0xffffffffffffffc5
lCPI0_69:
	.quad	-56 // 0xffffffffffffffc8
	.quad	-57 // 0xffffffffffffffc7
lCPI0_70:
	.quad	-2 // 0xfffffffffffffffe
	.quad	-3 // 0xfffffffffffffffd
lCPI0_71:
	.quad	0 // 0x0
	.quad	-1 // 0xffffffffffffffff
lCPI0_72:
	.quad	-6 // 0xfffffffffffffffa
	.quad	-7 // 0xfffffffffffffff9
lCPI0_73:
	.quad	-4 // 0xfffffffffffffffc
	.quad	-5 // 0xfffffffffffffffb
lCPI0_74:
	.quad	-10 // 0xfffffffffffffff6
	.quad	-11 // 0xfffffffffffffff5
lCPI0_75:
	.quad	-8 // 0xfffffffffffffff8
	.quad	-9 // 0xfffffffffffffff7
lCPI0_76:
	.quad	-14 // 0xfffffffffffffff2
	.quad	-15 // 0xfffffffffffffff1
lCPI0_77:
	.quad	-12 // 0xfffffffffffffff4
	.quad	-13 // 0xfffffffffffffff3
lCPI0_78:
	.quad	-62 // 0xffffffffffffffc2
	.quad	-63 // 0xffffffffffffffc1
lCPI0_79:
	.quad	-60 // 0xffffffffffffffc4
	.quad	-61 // 0xffffffffffffffc3
lCPI0_80:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
	.section	.rodata.cst8,"aM",@progbits,8
	.p2align	3, 0x0
lCPI0_2:
	.long	4294967294 // 0xfffffffe
	.long	4294967293 // 0xfffffffd
lCPI0_3:
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
lCPI0_4:
	.short	4 // 0x4
	.short	8 // 0x8
	.short	16 // 0x10
	.short	32 // 0x20
lCPI0_5:
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
lCPI0_8:
	.short	10 // 0xa
	.short	11 // 0xb
	.short	12 // 0xc
	.short	13 // 0xd
lCPI0_9:
	.short	1024 // 0x400
	.short	2048 // 0x800
	.short	4096 // 0x1000
	.short	8192 // 0x2000
lCPI0_11:
	.short	65527 // 0xfff7
	.short	65526 // 0xfff6
	.short	65525 // 0xfff5
	.short	65524 // 0xfff4
	.text
	.globl	FastInversion
	.p2align	2
FastInversion: // @FastInversion
	.cfi_startproc
// %bb.0:
	stp	d15, d14, [sp, #-160]! // 16-byte Folded Spill
	stp	d13, d12, [sp, #16] // 16-byte Folded Spill
	stp	d11, d10, [sp, #32] // 16-byte Folded Spill
	stp	d9, d8, [sp, #48] // 16-byte Folded Spill
	stp	x28, x27, [sp, #64] // 16-byte Folded Spill
	stp	x26, x25, [sp, #80] // 16-byte Folded Spill
	stp	x24, x23, [sp, #96] // 16-byte Folded Spill
	stp	x22, x21, [sp, #112] // 16-byte Folded Spill
	stp	x20, x19, [sp, #128] // 16-byte Folded Spill
	stp	x29, x30, [sp, #144] // 16-byte Folded Spill
	sub	sp, sp, #4080
	.cfi_def_cfa_offset 4240
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
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	.cfi_offset b10, -120
	.cfi_offset b11, -128
	.cfi_offset b12, -136
	.cfi_offset b13, -144
	.cfi_offset b14, -152
	.cfi_offset b15, -160
	mov	x20, x1
	mov	x19, x0
	add	x22, sp, #2992
	add	x24, x22, #258
	add	x0, sp, #1968
	mov	w1, #1024 // =0x400
	bl	bzero
	add	x21, sp, #880
	add	x1, sp, #880
	mov	x0, x20
	bl	pack_binary_256_neon
	add	x0, x20, #512
	add	x1, x21, #32
	bl	pack_binary_256_neon
	ldrh	w8, [x20]
	strh	w8, [sp, #2992]
	mov	w9, #2 // =0x2
LBB0_1: // =>This Inner Loop Header: Depth=1
	ldrh	w10, [x20, x9]
	eor	w8, w8, w10
	strh	w8, [x22, x9]
	add	x9, x9, #2
	cmp	x9, #1024
	b.ne	LBB0_1
// %bb.2:
	mov	x0, x19
	mov	w1, #1024 // =0x400
	bl	bzero
	mov	x10, #0 // =0x0
	mov	w25, #0 // =0x0
	mov	w8, #1 // =0x1
	strh	w8, [x19]
	add	x11, sp, #2992
LBB0_3: // =>This Inner Loop Header: Depth=1
	mov	x9, x25
	ldrh	w8, [x11, x10]
	eor	w25, w25, w8
	add	x10, x10, #2
	cmp	x10, #1024
	b.ne	LBB0_3
// %bb.4:
	dup	v0.8h, w25
	add	x10, x20, #32
	mov	w11, #48 // =0x30
	add	x12, sp, #2992
LBB0_5: // =>This Inner Loop Header: Depth=1
	ldp	q1, q2, [x10, #-32]
	ldp	q3, q4, [x10], #64
	mul	v1.8h, v1.8h, v0.8h
	mul	v2.8h, v2.8h, v0.8h
	mul	v3.8h, v3.8h, v0.8h
	mul	v4.8h, v4.8h, v0.8h
	add	x13, x12, x11
	ldp	q5, q6, [x13, #-48]
	ldp	q7, q16, [x13, #-16]
	eor	v1.16b, v1.16b, v5.16b
	eor	v2.16b, v2.16b, v6.16b
	eor	v3.16b, v3.16b, v7.16b
	eor	v4.16b, v4.16b, v16.16b
	stp	q1, q2, [x13, #-48]
	stp	q3, q4, [x13, #-16]
	add	x11, x11, #64
	cmp	x11, #1072
	b.ne	LBB0_5
// %bb.6:
	ldrh	w10, [sp, #2992]
	mov	w11, #2 // =0x2
	add	x12, sp, #2992
LBB0_7: // =>This Inner Loop Header: Depth=1
	ldrh	w13, [x12, x11]
	eor	w10, w10, w13
	strh	w10, [x12, x11]
	add	x11, x11, #2
	cmp	x11, #1024
	b.ne	LBB0_7
// %bb.8:
	and	w10, w9, #0xffff
	and	w9, w25, #0x1
	ldr	q0, [sp, #3104]
	ldr	q1, [sp, #3088]
	ldr	q2, [sp, #3232]
	ldr	q3, [sp, #3216]
	eor	v1.16b, v1.16b, v3.16b
	eor	v0.16b, v0.16b, v2.16b
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #3472]
	ldr	q5, [sp, #3488]
	eor3	v3.16b, v0.16b, v3.16b, v5.16b
	eor3	v0.16b, v1.16b, v2.16b, v4.16b
	ldr	q1, [sp, #3616]
	ldr	q2, [sp, #3600]
	ldr	q4, [sp, #3744]
	ldr	q5, [sp, #3728]
	eor3	v0.16b, v0.16b, v2.16b, v5.16b
	eor3	v1.16b, v3.16b, v1.16b, v4.16b
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #3056]
	ldr	q5, [sp, #3072]
	ldr	q6, [sp, #3184]
	ldr	q7, [sp, #3200]
	eor	v5.16b, v5.16b, v7.16b
	eor	v4.16b, v4.16b, v6.16b
	ldr	q6, [sp, #3328]
	ldr	q7, [sp, #3312]
	ldr	q16, [sp, #3456]
	ldr	q17, [sp, #3440]
	eor3	v4.16b, v4.16b, v7.16b, v17.16b
	eor3	v5.16b, v5.16b, v6.16b, v16.16b
	ldr	q6, [sp, #3568]
	ldr	q7, [sp, #3584]
	ldr	q16, [sp, #3696]
	ldr	q17, [sp, #3712]
	eor3	v5.16b, v5.16b, v7.16b, v17.16b
	eor3	v4.16b, v4.16b, v6.16b, v16.16b
	ldr	q6, [sp, #3840]
	ldr	q7, [sp, #3824]
	ldr	q16, [sp, #3040]
	ldr	q17, [sp, #3024]
	ldr	q18, [sp, #3168]
	ldr	q19, [sp, #3152]
	eor	v17.16b, v17.16b, v19.16b
	eor	v16.16b, v16.16b, v18.16b
	ldr	q18, [sp, #3280]
	ldr	q19, [sp, #3296]
	ldr	q20, [sp, #3408]
	ldr	q21, [sp, #3424]
	eor3	v16.16b, v16.16b, v19.16b, v21.16b
	eor3	v17.16b, v17.16b, v18.16b, v20.16b
	ldr	q18, [sp, #3552]
	ldr	q19, [sp, #3536]
	ldr	q20, [sp, #3680]
	ldr	q21, [sp, #3664]
	eor3	v17.16b, v17.16b, v19.16b, v21.16b
	eor3	v16.16b, v16.16b, v18.16b, v20.16b
	ldr	q18, [sp, #3792]
	ldr	q19, [sp, #3808]
	ldr	q20, [sp, #3008]
	ldr	q21, [sp, #2992]
	ldr	q22, [sp, #3136]
	ldr	q23, [sp, #3120]
	ldr	q24, [sp, #3264]
	ldr	q25, [sp, #3248]
	eor3	v21.16b, v21.16b, v23.16b, v25.16b
	eor3	v20.16b, v20.16b, v22.16b, v24.16b
	ldr	q22, [sp, #3376]
	ldr	q23, [sp, #3392]
	ldr	q24, [sp, #3504]
	ldr	q25, [sp, #3520]
	eor3	v20.16b, v20.16b, v23.16b, v25.16b
	eor3	v21.16b, v21.16b, v22.16b, v24.16b
	ldr	q22, [sp, #3648]
	ldr	q23, [sp, #3632]
	ldr	q24, [sp, #3776]
	ldr	q25, [sp, #3760]
	eor3	v21.16b, v21.16b, v23.16b, v25.16b
	eor3	v20.16b, v20.16b, v22.16b, v24.16b
	eor3	v16.16b, v16.16b, v19.16b, v20.16b
	eor3	v17.16b, v17.16b, v18.16b, v21.16b
	eor3	v4.16b, v4.16b, v7.16b, v17.16b
	eor3	v5.16b, v5.16b, v6.16b, v16.16b
	eor3	v1.16b, v1.16b, v3.16b, v5.16b
	eor3	v0.16b, v0.16b, v2.16b, v4.16b
	uzp1	v0.8h, v0.8h, v1.8h
	ext	v1.16b, v0.16b, v0.16b, #8
	eor	v0.8b, v0.8b, v1.8b
	fmov	x11, d0
	eor	x11, x11, x11, lsr #32
	lsr	x12, x11, #16
	ldrh	w13, [sp, #3888]
	eor	w11, w11, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3892]
	ldrh	w13, [sp, #3896]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3900]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3904]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3908]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3912]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3916]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3920]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3924]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3928]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3932]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3936]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3940]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3944]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3948]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3952]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3956]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3960]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3964]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3968]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3972]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3976]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3980]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3984]
	ldrh	w13, [sp, #3988]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3992]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3996]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4000]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4004]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4008]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4012]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldur	q0, [x22, #114]
	ldur	q1, [x22, #98]
	ldur	q2, [x22, #242]
	ldur	q3, [x22, #226]
	eor	v1.16b, v1.16b, v3.16b
	eor	v0.16b, v0.16b, v2.16b
	ldp	q2, q3, [x24, #96]
	ldp	q4, q5, [x24, #224]
	eor3	v3.16b, v0.16b, v3.16b, v5.16b
	eor3	v0.16b, v1.16b, v2.16b, v4.16b
	ldp	q2, q1, [x24, #352]
	ldp	q5, q4, [x24, #480]
	eor3	v0.16b, v0.16b, v2.16b, v5.16b
	eor3	v1.16b, v3.16b, v1.16b, v4.16b
	ldp	q2, q3, [x24, #608]
	ldur	q4, [x22, #66]
	ldur	q5, [x22, #82]
	ldur	q6, [x22, #194]
	ldur	q7, [x22, #210]
	eor	v5.16b, v5.16b, v7.16b
	eor	v4.16b, v4.16b, v6.16b
	ldp	q7, q6, [x24, #64]
	ldp	q17, q16, [x24, #192]
	eor3	v4.16b, v4.16b, v7.16b, v17.16b
	eor3	v5.16b, v5.16b, v6.16b, v16.16b
	ldp	q6, q7, [x24, #320]
	ldp	q16, q17, [x24, #448]
	eor3	v5.16b, v5.16b, v7.16b, v17.16b
	eor3	v4.16b, v4.16b, v6.16b, v16.16b
	ldp	q7, q6, [x24, #576]
	ldur	q16, [x22, #50]
	ldur	q17, [x22, #34]
	ldur	q18, [x22, #178]
	ldur	q19, [x22, #162]
	eor	v17.16b, v17.16b, v19.16b
	eor	v16.16b, v16.16b, v18.16b
	ldp	q18, q19, [x24, #32]
	ldp	q20, q21, [x24, #160]
	eor3	v16.16b, v16.16b, v19.16b, v21.16b
	eor3	v17.16b, v17.16b, v18.16b, v20.16b
	ldp	q19, q18, [x24, #288]
	ldp	q21, q20, [x24, #416]
	eor3	v17.16b, v17.16b, v19.16b, v21.16b
	eor3	v16.16b, v16.16b, v18.16b, v20.16b
	ldp	q18, q19, [x24, #544]
	ldur	q20, [x22, #18]
	ldur	q21, [x22, #2]
	ldur	q22, [x22, #146]
	ldur	q23, [x22, #130]
	ldp	q25, q24, [x24]
	eor3	v21.16b, v21.16b, v23.16b, v25.16b
	eor3	v20.16b, v20.16b, v22.16b, v24.16b
	ldp	q22, q23, [x24, #128]
	ldp	q24, q25, [x24, #256]
	eor3	v20.16b, v20.16b, v23.16b, v25.16b
	eor3	v21.16b, v21.16b, v22.16b, v24.16b
	ldp	q23, q22, [x24, #384]
	ldp	q25, q24, [x24, #512]
	eor3	v21.16b, v21.16b, v23.16b, v25.16b
	eor3	v20.16b, v20.16b, v22.16b, v24.16b
	eor3	v16.16b, v16.16b, v19.16b, v20.16b
	eor3	v17.16b, v17.16b, v18.16b, v21.16b
	eor3	v4.16b, v4.16b, v7.16b, v17.16b
	eor3	v5.16b, v5.16b, v6.16b, v16.16b
	eor3	v1.16b, v1.16b, v3.16b, v5.16b
	eor3	v0.16b, v0.16b, v2.16b, v4.16b
	uzp1	v0.8h, v0.8h, v1.8h
	ext	v1.16b, v0.16b, v0.16b, #8
	eor	v0.8b, v0.8b, v1.8b
	fmov	x12, d0
	eor	x12, x12, x12, lsr #32
	lsr	x13, x12, #16
	ldrh	w14, [sp, #3890]
	eor	w12, w12, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3894]
	ldrh	w14, [sp, #3898]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3902]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3906]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3910]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3914]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3918]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3922]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3926]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3930]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3934]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3938]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3942]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3946]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3950]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3954]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3958]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3962]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3966]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3970]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3974]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3978]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3982]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3986]
	ldrh	w14, [sp, #3990]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3994]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3998]
	eor	w13, w13, w14
	ldrh	w14, [sp, #4002]
	eor	w13, w13, w14
	ldrh	w14, [sp, #4006]
	eor	w13, w13, w14
	ldrh	w14, [sp, #4010]
	eor	w13, w13, w14
	ldrh	w14, [sp, #4014]
	eor	w13, w13, w14
	eor	w12, w12, w13
	and	w13, w11, #0x1
	bfi	w13, w12, #1, #1
	cmp	w10, w8
	csel	w8, w13, wzr, eq
	cset	w21, eq
	add	x10, sp, #944
	orr	x10, x10, #0x2
	str	x10, [sp, #832] // 8-byte Folded Spill
	strh	w11, [sp, #944]
	and	w10, w12, #0x1
	orr	w10, w10, w13, lsl #1
	strh	w12, [sp, #946]
	neg	w9, w9
	and	w9, w10, w9
	eor	w8, w9, w8
	and	w20, w8, #0x1
	strh	w20, [sp, #1968]
	ubfx	w26, w8, #1, #1
	strh	w26, [sp, #1970]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	x8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldrh	w9, [sp, #2992]
	add	x10, sp, #2992
	ldrh	w11, [sp, #2994]
LBB0_9: // =>This Inner Loop Header: Depth=1
	add	x12, x10, x8, lsl #1
	ldrh	w13, [x12, #4]
	eor	w9, w9, w13
	strh	w9, [x12, #4]
	ldrh	w13, [x12, #6]
	eor	w11, w11, w13
	strh	w11, [x12, #6]
	add	x8, x8, #2
	cmp	w8, #510
	b.lo	LBB0_9
// %bb.10:
	ldr	q0, [sp, #3200]
	ldr	q1, [sp, #3184]
	ldr	q2, [sp, #3232]
	ldr	q3, [sp, #3216]
	ldr	q5, [sp, #3456]
	ldr	q6, [sp, #3440]
	ldr	q7, [sp, #3488]
	ldr	q4, [sp, #3472]
	eor	v4.16b, v3.16b, v4.16b
	eor	v2.16b, v2.16b, v7.16b
	eor	v3.16b, v1.16b, v6.16b
	eor	v0.16b, v0.16b, v5.16b
	ldr	q7, [sp, #3152]
	ldr	q6, [sp, #3168]
	ldr	q5, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q16, [sp, #3408]
	ldr	q17, [sp, #3424]
	ldr	q18, [sp, #3376]
	ldr	q19, [sp, #3392]
	eor	v1.16b, v1.16b, v19.16b
	eor	v5.16b, v5.16b, v18.16b
	eor	v6.16b, v6.16b, v17.16b
	eor	v7.16b, v7.16b, v16.16b
	ldr	q16, [sp, #3072]
	ldr	q17, [sp, #3056]
	ldr	q18, [sp, #3104]
	ldr	q19, [sp, #3088]
	ldr	q20, [sp, #3360]
	ldr	q21, [sp, #3344]
	eor	v18.16b, v18.16b, v20.16b
	ldr	q20, [sp, #3040]
	ldr	q22, [sp, #3024]
	ldr	q23, [sp, #3296]
	ldr	q24, [sp, #3280]
	ldr	q25, [sp, #3552]
	ldr	q26, [sp, #3536]
	eor3	v22.16b, v22.16b, v24.16b, v26.16b
	ldr	q24, [sp, #3008]
	ldr	q26, [sp, #2992]
	eor3	v20.16b, v20.16b, v23.16b, v25.16b
	ldr	q23, [sp, #3248]
	ldr	q25, [sp, #3504]
	eor3	v23.16b, v26.16b, v23.16b, v25.16b
	ldr	q25, [sp, #3328]
	ldr	q26, [sp, #3312]
	eor	v17.16b, v17.16b, v26.16b
	ldr	q26, [sp, #3264]
	eor	v16.16b, v16.16b, v25.16b
	ldr	q25, [sp, #3520]
	eor3	v24.16b, v24.16b, v26.16b, v25.16b
	ldr	q25, [sp, #3568]
	ldr	q26, [sp, #3584]
	eor3	v16.16b, v16.16b, v26.16b, v24.16b
	ldr	q24, [sp, #3728]
	ldr	q26, [sp, #3744]
	eor3	v17.16b, v17.16b, v25.16b, v23.16b
	ldr	q23, [sp, #3600]
	ldr	q25, [sp, #3616]
	eor3	v18.16b, v18.16b, v25.16b, v20.16b
	ldr	q20, [sp, #3696]
	ldr	q25, [sp, #3712]
	eor	v19.16b, v19.16b, v21.16b
	eor3	v19.16b, v19.16b, v23.16b, v22.16b
	ldr	q21, [sp, #3680]
	ldr	q22, [sp, #3664]
	eor3	v7.16b, v7.16b, v22.16b, v19.16b
	ldr	q19, [sp, #3648]
	ldr	q22, [sp, #3632]
	eor3	v6.16b, v6.16b, v21.16b, v18.16b
	eor3	v5.16b, v5.16b, v22.16b, v17.16b
	eor3	v1.16b, v1.16b, v19.16b, v16.16b
	eor3	v1.16b, v0.16b, v25.16b, v1.16b
	eor3	v0.16b, v3.16b, v20.16b, v5.16b
	eor3	v3.16b, v2.16b, v26.16b, v6.16b
	eor3	v2.16b, v4.16b, v24.16b, v7.16b
	ldur	q4, [x22, #210]
	ldur	q5, [x22, #194]
	ldur	q6, [x22, #242]
	ldur	q7, [x22, #226]
	ldp	q18, q17, [x24, #192]
	ldp	q16, q19, [x24, #224]
	eor	v16.16b, v7.16b, v16.16b
	eor	v6.16b, v6.16b, v19.16b
	eor	v7.16b, v5.16b, v18.16b
	eor	v4.16b, v4.16b, v17.16b
	ldur	q18, [x22, #162]
	ldur	q17, [x22, #178]
	ldur	q5, [x22, #130]
	ldp	q19, q20, [x24, #160]
	ldr	q21, [x24, #128]
	eor	v5.16b, v5.16b, v21.16b
	eor	v17.16b, v17.16b, v20.16b
	eor	v18.16b, v18.16b, v19.16b
	ldur	q19, [x22, #82]
	ldur	q20, [x22, #66]
	ldur	q21, [x22, #2]
	ldur	q22, [x22, #50]
	ldur	q23, [x22, #34]
	ldr	q24, [x24, #32]
	ldp	q26, q25, [x24, #288]
	eor3	v23.16b, v23.16b, v24.16b, v26.16b
	ldr	q24, [x24]
	ldr	q26, [x24, #48]
	eor3	v22.16b, v22.16b, v26.16b, v25.16b
	ldp	q26, q25, [x24, #256]
	eor3	v21.16b, v21.16b, v24.16b, v26.16b
	ldur	q24, [x22, #18]
	ldr	q26, [x24, #16]
	eor3	v24.16b, v24.16b, v26.16b, v25.16b
	ldur	q25, [x22, #114]
	ldr	q26, [x24, #80]
	eor	v19.16b, v19.16b, v26.16b
	ldr	q26, [x24, #336]
	eor3	v19.16b, v19.16b, v26.16b, v24.16b
	ldur	q24, [x22, #98]
	ldp	q27, q26, [x24, #96]
	eor	v24.16b, v24.16b, v27.16b
	eor	v25.16b, v25.16b, v26.16b
	ldr	q26, [x24, #64]
	eor	v20.16b, v20.16b, v26.16b
	ldr	q26, [x24, #320]
	eor3	v20.16b, v20.16b, v26.16b, v21.16b
	ldp	q21, q26, [x24, #352]
	eor3	v22.16b, v25.16b, v26.16b, v22.16b
	ldp	q25, q26, [x24, #480]
	eor3	v21.16b, v24.16b, v21.16b, v23.16b
	ldp	q24, q23, [x24, #416]
	eor3	v18.16b, v18.16b, v24.16b, v21.16b
	ldp	q21, q24, [x24, #448]
	eor3	v17.16b, v17.16b, v23.16b, v22.16b
	ldp	q23, q22, [x24, #384]
	eor3	v20.16b, v5.16b, v23.16b, v20.16b
	ldur	q5, [x22, #146]
	ldr	q23, [x24, #144]
	eor	v5.16b, v5.16b, v23.16b
	eor3	v5.16b, v5.16b, v22.16b, v19.16b
	eor3	v5.16b, v4.16b, v24.16b, v5.16b
	eor3	v4.16b, v7.16b, v21.16b, v20.16b
	eor3	v7.16b, v6.16b, v26.16b, v17.16b
	eor3	v6.16b, v16.16b, v25.16b, v18.16b
	ldur	q16, [x22, #212]
	ldur	q17, [x22, #196]
	ldur	q18, [x22, #244]
	ldur	q19, [x22, #228]
	ldur	q21, [x24, #210]
	ldur	q22, [x24, #194]
	ldur	q23, [x24, #242]
	ldur	q20, [x24, #226]
	eor	v20.16b, v19.16b, v20.16b
	eor	v18.16b, v18.16b, v23.16b
	eor	v19.16b, v17.16b, v22.16b
	eor	v16.16b, v16.16b, v21.16b
	ldur	q17, [x22, #84]
	ldur	q21, [x22, #4]
	ldur	q22, [x22, #52]
	ldur	q23, [x22, #36]
	ldur	q24, [x24, #34]
	add	x8, x24, #258
	ldp	q26, q25, [x8, #32]
	eor3	v23.16b, v23.16b, v24.16b, v26.16b
	ldur	q24, [x24, #2]
	ldur	q26, [x24, #50]
	eor3	v22.16b, v22.16b, v26.16b, v25.16b
	ldp	q26, q25, [x8]
	eor3	v21.16b, v21.16b, v24.16b, v26.16b
	ldur	q24, [x22, #20]
	ldur	q26, [x24, #18]
	eor3	v24.16b, v24.16b, v26.16b, v25.16b
	ldur	q25, [x22, #68]
	ldur	q26, [x24, #82]
	eor	v17.16b, v17.16b, v26.16b
	add	x8, x24, #322
	ldr	q26, [x8, #16]
	eor3	v17.16b, v17.16b, v26.16b, v24.16b
	ldur	q24, [x22, #116]
	ldur	q26, [x24, #114]
	eor	v24.16b, v24.16b, v26.16b
	ldur	q26, [x24, #66]
	eor	v25.16b, v25.16b, v26.16b
	ldr	q26, [x8]
	eor3	v21.16b, v25.16b, v26.16b, v21.16b
	ldp	q25, q26, [x8, #32]
	eor3	v22.16b, v24.16b, v26.16b, v22.16b
	ldur	q24, [x22, #100]
	ldur	q26, [x24, #98]
	eor	v24.16b, v24.16b, v26.16b
	ldur	q26, [x22, #164]
	eor3	v23.16b, v24.16b, v25.16b, v23.16b
	ldur	q24, [x22, #180]
	ldur	q25, [x24, #162]
	eor	v25.16b, v26.16b, v25.16b
	add	x8, x24, #386
	ldr	q26, [x8, #32]
	eor3	v23.16b, v25.16b, v26.16b, v23.16b
	ldur	q25, [x22, #132]
	ldur	q26, [x24, #130]
	eor	v25.16b, v25.16b, v26.16b
	ldur	q26, [x24, #178]
	eor	v24.16b, v24.16b, v26.16b
	ldr	q26, [x8, #48]
	add	x9, x24, #450
	eor3	v22.16b, v24.16b, v26.16b, v22.16b
	ldp	q26, q24, [x8]
	eor3	v21.16b, v25.16b, v26.16b, v21.16b
	ldur	q25, [x22, #148]
	ldur	q26, [x24, #146]
	eor	v25.16b, v25.16b, v26.16b
	eor3	v17.16b, v25.16b, v24.16b, v17.16b
	ldp	q24, q25, [x9]
	eor3	v17.16b, v16.16b, v25.16b, v17.16b
	ldp	q25, q26, [x9, #32]
	eor3	v16.16b, v19.16b, v24.16b, v21.16b
	eor3	v19.16b, v18.16b, v26.16b, v22.16b
	eor3	v18.16b, v20.16b, v25.16b, v23.16b
	ldur	q21, [x22, #214]
	ldur	q20, [x22, #198]
	ldur	q22, [x24, #212]
	ldur	q23, [x24, #196]
	eor	v20.16b, v20.16b, v23.16b
	eor	v21.16b, v21.16b, v22.16b
	ldur	q22, [x22, #86]
	ldur	q23, [x24, #84]
	eor	v22.16b, v22.16b, v23.16b
	add	x8, x24, #324
	ldur	q23, [x22, #6]
	ldur	q24, [x24, #4]
	add	x9, x24, #260
	ldp	q26, q25, [x9]
	eor3	v23.16b, v23.16b, v24.16b, v26.16b
	ldur	q24, [x22, #22]
	ldur	q26, [x24, #20]
	eor3	v24.16b, v24.16b, v26.16b, v25.16b
	ldp	q25, q26, [x8]
	eor3	v22.16b, v22.16b, v26.16b, v24.16b
	ldur	q24, [x22, #70]
	ldur	q26, [x24, #68]
	eor	v24.16b, v24.16b, v26.16b
	ldur	q26, [x22, #166]
	eor3	v23.16b, v24.16b, v25.16b, v23.16b
	ldur	q24, [x22, #134]
	ldur	q25, [x24, #132]
	eor	v24.16b, v24.16b, v25.16b
	add	x10, x24, #388
	ldr	q25, [x10]
	eor3	v23.16b, v24.16b, v25.16b, v23.16b
	ldur	q24, [x22, #150]
	ldur	q25, [x24, #148]
	eor	v24.16b, v24.16b, v25.16b
	ldur	q25, [x24, #164]
	eor	v25.16b, v26.16b, v25.16b
	ldr	q26, [x10, #16]
	add	x11, x24, #452
	eor3	v22.16b, v24.16b, v26.16b, v22.16b
	ldp	q24, q26, [x11]
	eor3	v22.16b, v21.16b, v26.16b, v22.16b
	ldur	q26, [x22, #38]
	eor3	v21.16b, v20.16b, v24.16b, v23.16b
	ldur	q20, [x24, #36]
	ldp	q24, q23, [x9, #32]
	eor3	v20.16b, v26.16b, v20.16b, v24.16b
	ldur	q24, [x22, #54]
	ldur	q26, [x24, #52]
	eor3	v23.16b, v24.16b, v26.16b, v23.16b
	ldur	q24, [x22, #118]
	ldur	q26, [x24, #116]
	eor	v24.16b, v24.16b, v26.16b
	ldr	q26, [x8, #48]
	eor3	v23.16b, v24.16b, v26.16b, v23.16b
	ldur	q24, [x22, #102]
	ldur	q26, [x24, #100]
	eor	v24.16b, v24.16b, v26.16b
	ldr	q26, [x8, #32]
	eor3	v20.16b, v24.16b, v26.16b, v20.16b
	ldp	q26, q24, [x10, #32]
	eor3	v20.16b, v25.16b, v26.16b, v20.16b
	ldur	q25, [x22, #182]
	ldur	q26, [x24, #180]
	eor	v25.16b, v25.16b, v26.16b
	ldur	q26, [x22, #246]
	eor3	v23.16b, v25.16b, v24.16b, v23.16b
	ldur	q24, [x22, #230]
	ldur	q25, [x24, #228]
	eor	v24.16b, v24.16b, v25.16b
	ldur	q25, [x24, #244]
	eor	v25.16b, v26.16b, v25.16b
	ldr	q26, [x11, #48]
	eor3	v26.16b, v25.16b, v26.16b, v23.16b
	ldr	q23, [x11, #32]
	eor3	v25.16b, v24.16b, v23.16b, v20.16b
Lloh0:
	adrp	x8, lCPI0_0
Lloh1:
	ldr	q20, [x8, :lo12:lCPI0_0]
	tbl	v3.16b, { v2.16b, v3.16b }, v20.16b
	tbl	v6.16b, { v6.16b, v7.16b }, v20.16b
	tbl	v7.16b, { v18.16b, v19.16b }, v20.16b
	tbl	v2.16b, { v25.16b, v26.16b }, v20.16b
Lloh2:
	adrp	x8, lCPI0_1
Lloh3:
	ldr	q18, [x8, :lo12:lCPI0_1]
	tbl	v1.16b, { v0.16b, v1.16b }, v18.16b
	tbl	v4.16b, { v4.16b, v5.16b }, v18.16b
	tbl	v5.16b, { v16.16b, v17.16b }, v18.16b
	tbl	v0.16b, { v21.16b, v22.16b }, v18.16b
	ext	v3.16b, v3.16b, v3.16b, #8
	eor	v1.8b, v1.8b, v3.8b
	fmov	x8, d1
	ext	v1.16b, v6.16b, v6.16b, #8
	eor	v1.8b, v4.8b, v1.8b
	fmov	x9, d1
	ext	v1.16b, v7.16b, v7.16b, #8
	eor	v1.8b, v5.8b, v1.8b
	fmov	x10, d1
	ldr	w11, [sp, #1968]
	str	w11, [sp, #944]
	eor	w27, w11, w21
	lsr	w11, w11, #16
	eor	w25, w25, w11
	ldrh	w11, [x19, #4]
	eor	w21, w11, w20
	eor	x8, x8, x8, lsr #32
	lsr	x11, x8, #16
	ldrh	w12, [sp, #3760]
	eor	w8, w8, w12
	eor	w8, w8, w11
	ldrh	w11, [sp, #3768]
	ldrh	w12, [sp, #3776]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3784]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3792]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3800]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3808]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3816]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3824]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3832]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3840]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3848]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3856]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3864]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3872]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3880]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3888]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3896]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3904]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3912]
	eor	w11, w11, w12
	eor	w8, w8, w11
	ldrh	w11, [sp, #3920]
	ldrh	w12, [sp, #3928]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3936]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3944]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3952]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3960]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3968]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3976]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3984]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3992]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4000]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4008]
	eor	w11, w11, w12
	eor	w8, w8, w11
	eor	x9, x9, x9, lsr #32
	lsr	x11, x9, #16
	ldrh	w12, [sp, #3762]
	eor	w9, w9, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3770]
	ldrh	w12, [sp, #3778]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3786]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3794]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3802]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3810]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3818]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3826]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3834]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3842]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3850]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3858]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3866]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3874]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3882]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3890]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3898]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3906]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3914]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3922]
	ldrh	w12, [sp, #3930]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3938]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3946]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3954]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3962]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3970]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3978]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3986]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3994]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4002]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4010]
	eor	w11, w11, w12
	eor	w9, w9, w11
	eor	x10, x10, x10, lsr #32
	lsr	x11, x10, #16
	ldrh	w12, [sp, #3764]
	eor	w10, w10, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3772]
	ldrh	w12, [sp, #3780]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3788]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3796]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3804]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3812]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3820]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3828]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3836]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3844]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3852]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3860]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3868]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3876]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3884]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3892]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3900]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3908]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3916]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3924]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3932]
	ldrh	w12, [sp, #3940]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3948]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3956]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3964]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3972]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3980]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3988]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3996]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4004]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4012]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ext	v1.16b, v2.16b, v2.16b, #8
	eor	v0.8b, v0.8b, v1.8b
	fmov	x11, d0
	eor	x11, x11, x11, lsr #32
	lsr	x12, x11, #16
	ldrh	w13, [sp, #3766]
	eor	w11, w11, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3774]
	ldrh	w13, [sp, #3782]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3790]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3798]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3806]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3814]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3822]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3830]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3838]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3846]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3854]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3862]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3870]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3878]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3886]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3894]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3902]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3910]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3918]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3926]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3934]
	ldrh	w13, [sp, #3942]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3950]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3958]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3966]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3974]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3982]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3990]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3998]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4006]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4014]
	eor	w12, w12, w13
	eor	w11, w11, w12
	and	w12, w8, #0x1
	bfi	w12, w9, #1, #1
	lsl	w13, w10, #2
	bfi	w12, w10, #2, #1
	and	w14, w11, #0x1
	orr	w14, w14, w12, lsl #1
	bfi	w12, w11, #3, #1
	and	w15, w27, #0x1
	neg	w15, w15
	and	w12, w12, w15
	and	w15, w25, #0x1
	neg	w15, w15
	and	w15, w14, w15
	eor	w12, w15, w12
	ubfx	w13, w13, #2, #1
	orr	w13, w13, w14, lsl #1
	and	w14, w13, #0xf
	and	w15, w21, #0x1
	neg	w15, w15
	and	w14, w14, w15
	ldrh	w15, [x19, #6]
	eor	w28, w15, w26
	eor	w12, w12, w14
	lsl	w14, w9, #1
	ubfx	w14, w14, #1, #1
	bfi	w14, w13, #1, #4
	and	w13, w28, #0x1
	neg	w13, w13
	and	w13, w14, w13
	eor	w12, w12, w13
	dup	v0.2s, w12
	movi	d1, #0x00ffff0000ffff
	and	v0.8b, v0.8b, v1.8b
Lloh4:
	adrp	x13, lCPI0_2
Lloh5:
	ldr	d1, [x13, :lo12:lCPI0_2]
	ushl	v0.2s, v0.2s, v1.2s
	movi	v1.2s, #1
	and	v0.8b, v0.8b, v1.8b
	strh	w20, [sp, #948]
	add	x13, sp, #944
	orr	x13, x13, #0x4
	str	x13, [sp, #848] // 8-byte Folded Spill
	strh	w26, [sp, #950]
	strh	w27, [x19]
	strh	w25, [x19, #2]
	strh	w21, [x19, #4]
	strh	w28, [x19, #6]
	strh	w8, [sp, #944]
	strh	w9, [sp, #946]
	strh	w10, [sp, #948]
	strh	w11, [sp, #950]
	and	w20, w12, #0x1
	strh	w20, [sp, #1968]
	ubfx	w26, w12, #1, #1
	strh	w26, [sp, #1970]
	fmov	w23, s0
	strh	w23, [sp, #1972]
	str	q0, [sp, #816] // 16-byte Folded Spill
	umov	w24, v0.s[1]
	strh	w24, [sp, #1974]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	w8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	mov	w9, #8 // =0x8
	add	x10, sp, #2992
	ldr	d0, [sp, #2992]
LBB0_11: // =>This Inner Loop Header: Depth=1
	ldr	d1, [x10, x9]
	eor	v0.8b, v0.8b, v1.8b
	str	d0, [x10, x9]
	add	w8, w8, #4
	add	x9, x9, #8
	cmp	w8, #508
	b.lo	LBB0_11
// %bb.12:
	ldr	x8, [sp, #1968]
	str	x8, [sp, #944]
	add	x8, sp, #944
	orr	x8, x8, #0x8
	str	x8, [sp, #864] // 8-byte Folded Spill
	strh	w20, [sp, #952]
	strh	w26, [sp, #954]
	strh	w23, [sp, #956]
	strh	w24, [sp, #958]
	ldr	q0, [sp, #944]
	ldr	d1, [x19, #8]
	fmov	s2, w27
	mov	v2.h[1], w25
	mov	v2.h[2], w21
	mov	v2.h[3], w28
	mov	v2.d[1], v1.d[0]
	ldr	q3, [sp, #816] // 16-byte Folded Reload
	umov	w8, v3.s[1]
	mov	v1.h[4], w20
	mov	v1.h[5], w26
	mov	v1.h[6], v3.h[0]
	mov	v1.h[7], w8
	mov	v0.d[1], v1.d[1]
	eor	v0.16b, v2.16b, v0.16b
	ldr	x8, [sp, #832] // 8-byte Folded Reload
	stur	xzr, [x8, #6]
	str	xzr, [x8]
	ldrh	w8, [sp, #2992]
	ldrh	w9, [sp, #3008]
	eor	w8, w8, w9
	ldrh	w9, [sp, #3024]
	add	x10, sp, #2992
	add	x23, x10, #48
	ldrh	w11, [sp, #3040]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3056]
	ldrh	w11, [sp, #3072]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3088]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3104]
	ldrh	w11, [sp, #3120]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3136]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3152]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3168]
	ldrh	w11, [sp, #3184]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3200]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3216]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3232]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3248]
	ldrh	w11, [sp, #3264]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3280]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3296]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3312]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3328]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3344]
	ldrh	w11, [sp, #3360]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3376]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3392]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3408]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3424]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3440]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3456]
	ldrh	w11, [sp, #3472]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3488]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3504]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3520]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3536]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3552]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3568]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3584]
	ldrh	w11, [sp, #3600]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3616]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3632]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3648]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3664]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3680]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3696]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3712]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3728]
	ldrh	w11, [sp, #3744]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3760]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3776]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3792]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3808]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3824]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3840]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3856]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3872]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #3888]
	ldrh	w11, [sp, #3904]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3920]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3936]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3952]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3968]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3984]
	eor	w9, w9, w11
	ldrh	w11, [sp, #4000]
	eor	w9, w9, w11
	eor	w8, w8, w9
	ldrh	w9, [sp, #946]
	ldrh	w11, [sp, #2994]
	eor	w9, w9, w11
	ldrh	w11, [sp, #3010]
	ldrh	w12, [sp, #3026]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3042]
	ldrh	w12, [sp, #3058]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3074]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3090]
	ldrh	w12, [sp, #3106]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3122]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3138]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3154]
	ldrh	w12, [sp, #3170]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3186]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3202]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3218]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3234]
	ldrh	w12, [sp, #3250]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3266]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3282]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3298]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3314]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3330]
	ldrh	w12, [sp, #3346]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3362]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3378]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3394]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3410]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3426]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3442]
	ldrh	w12, [sp, #3458]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3474]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3490]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3506]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3522]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3538]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3554]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3570]
	ldrh	w12, [sp, #3586]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3602]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3618]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3634]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3650]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3666]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3682]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3698]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3714]
	ldrh	w12, [sp, #3730]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3746]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3762]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3778]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3794]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3810]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3826]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3842]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3858]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #3874]
	ldrh	w12, [sp, #3890]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3906]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3922]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3938]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3954]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3970]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3986]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4002]
	eor	w11, w11, w12
	eor	w9, w9, w11
	ldrh	w11, [sp, #948]
	orr	x10, x10, #0x4
	str	x10, [sp, #832] // 8-byte Folded Spill
	ldrh	w10, [sp, #2996]
	eor	w10, w11, w10
	ldrh	w11, [sp, #3012]
	ldrh	w12, [sp, #3028]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3044]
	ldrh	w12, [sp, #3060]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3076]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3092]
	ldrh	w12, [sp, #3108]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3124]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3140]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3156]
	ldrh	w12, [sp, #3172]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3188]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3204]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3220]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3236]
	ldrh	w12, [sp, #3252]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3268]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3284]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3300]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3316]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3332]
	ldrh	w12, [sp, #3348]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3364]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3380]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3396]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3412]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3428]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3444]
	ldrh	w12, [sp, #3460]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3476]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3492]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3508]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3524]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3540]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3556]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3572]
	ldrh	w12, [sp, #3588]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3604]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3620]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3636]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3652]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3668]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3684]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3700]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3716]
	ldrh	w12, [sp, #3732]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3748]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3764]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3780]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3796]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3812]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3828]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3844]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3860]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #3876]
	ldrh	w12, [sp, #3892]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3908]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3924]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3940]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3956]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3972]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3988]
	eor	w11, w11, w12
	ldrh	w12, [sp, #4004]
	eor	w11, w11, w12
	eor	w10, w10, w11
	ldrh	w11, [sp, #950]
	ldrh	w12, [sp, #2998]
	eor	w11, w11, w12
	ldrh	w12, [sp, #3014]
	ldrh	w13, [sp, #3030]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3046]
	ldrh	w13, [sp, #3062]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3078]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3094]
	ldrh	w13, [sp, #3110]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3126]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3142]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3158]
	ldrh	w13, [sp, #3174]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3190]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3206]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3222]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3238]
	ldrh	w13, [sp, #3254]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3270]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3286]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3302]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3318]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3334]
	ldrh	w13, [sp, #3350]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3366]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3382]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3398]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3414]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3430]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3446]
	ldrh	w13, [sp, #3462]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3478]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3494]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3510]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3526]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3542]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3558]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3574]
	ldrh	w13, [sp, #3590]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3606]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3622]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3638]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3654]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3670]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3686]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3702]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3718]
	ldrh	w13, [sp, #3734]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3750]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3766]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3782]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3798]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3814]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3830]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3846]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3862]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #3878]
	ldrh	w13, [sp, #3894]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3910]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3926]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3942]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3958]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3974]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3990]
	eor	w12, w12, w13
	ldrh	w13, [sp, #4006]
	eor	w12, w12, w13
	eor	w11, w11, w12
	ldrh	w12, [sp, #952]
	ldrh	w13, [sp, #3000]
	eor	w12, w12, w13
	ldrh	w13, [sp, #3016]
	ldrh	w14, [sp, #3032]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3048]
	ldrh	w14, [sp, #3064]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3080]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3096]
	ldrh	w14, [sp, #3112]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3128]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3144]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3160]
	ldrh	w14, [sp, #3176]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3192]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3208]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3224]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3240]
	ldrh	w14, [sp, #3256]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3272]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3288]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3304]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3320]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3336]
	ldrh	w14, [sp, #3352]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3368]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3384]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3400]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3416]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3432]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3448]
	ldrh	w14, [sp, #3464]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3480]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3496]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3512]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3528]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3544]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3560]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3576]
	ldrh	w14, [sp, #3592]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3608]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3624]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3640]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3656]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3672]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3688]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3704]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3720]
	ldrh	w14, [sp, #3736]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3752]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3768]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3784]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3800]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3816]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3832]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3848]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3864]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #3880]
	ldrh	w14, [sp, #3896]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3912]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3928]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3944]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3960]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3976]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3992]
	eor	w13, w13, w14
	ldrh	w14, [sp, #4008]
	eor	w13, w13, w14
	eor	w12, w12, w13
	ldrh	w13, [sp, #954]
	ldrh	w14, [sp, #3002]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3018]
	ldrh	w15, [sp, #3034]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3050]
	ldrh	w15, [sp, #3066]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3082]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3098]
	ldrh	w15, [sp, #3114]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3130]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3146]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3162]
	ldrh	w15, [sp, #3178]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3194]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3210]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3226]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3242]
	ldrh	w15, [sp, #3258]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3274]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3290]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3306]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3322]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3338]
	ldrh	w15, [sp, #3354]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3370]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3386]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3402]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3418]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3434]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3450]
	ldrh	w15, [sp, #3466]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3482]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3498]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3514]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3530]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3546]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3562]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3578]
	ldrh	w15, [sp, #3594]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3610]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3626]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3642]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3658]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3674]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3690]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3706]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3722]
	ldrh	w15, [sp, #3738]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3754]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3770]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3786]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3802]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3818]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3834]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3850]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3866]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3882]
	ldrh	w15, [sp, #3898]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3914]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3930]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3946]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3962]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3978]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3994]
	eor	w14, w14, w15
	ldrh	w15, [sp, #4010]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #956]
	ldrh	w15, [sp, #3004]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3020]
	ldrh	w16, [sp, #3036]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3052]
	ldrh	w16, [sp, #3068]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3084]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3100]
	ldrh	w16, [sp, #3116]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3132]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3148]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3164]
	ldrh	w16, [sp, #3180]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3196]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3212]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3228]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3244]
	ldrh	w16, [sp, #3260]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3276]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3292]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3308]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3324]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3340]
	ldrh	w16, [sp, #3356]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3372]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3388]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3404]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3420]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3436]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3452]
	ldrh	w16, [sp, #3468]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3484]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3500]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3516]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3532]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3548]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3564]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3580]
	ldrh	w16, [sp, #3596]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3612]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3628]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3644]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3660]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3676]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3692]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3708]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3724]
	ldrh	w16, [sp, #3740]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3756]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3772]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3788]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3804]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3820]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3836]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3852]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3868]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #3884]
	ldrh	w16, [sp, #3900]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3916]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3932]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3948]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3964]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3980]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3996]
	eor	w15, w15, w16
	ldrh	w16, [sp, #4012]
	eor	w15, w15, w16
	eor	w14, w14, w15
	ldrh	w15, [sp, #958]
	ldrh	w16, [sp, #3006]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3022]
	ldrh	w17, [sp, #3038]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3054]
	ldrh	w17, [sp, #3070]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3086]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3102]
	ldrh	w17, [sp, #3118]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3134]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3150]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3166]
	ldrh	w17, [sp, #3182]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3198]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3214]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3230]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3246]
	ldrh	w17, [sp, #3262]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3278]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3294]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3310]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3326]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3342]
	ldrh	w17, [sp, #3358]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3374]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3390]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3406]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3422]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3438]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3454]
	ldrh	w17, [sp, #3470]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3486]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3502]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3518]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3534]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3550]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3566]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3582]
	ldrh	w17, [sp, #3598]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3614]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3630]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3646]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3662]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3678]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3694]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3710]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3726]
	ldrh	w17, [sp, #3742]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3758]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3774]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3790]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3806]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3822]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3838]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3854]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3870]
	eor	w16, w16, w17
	eor	w15, w15, w16
	ldrh	w16, [sp, #3886]
	ldrh	w17, [sp, #3902]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3918]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3934]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3950]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3966]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3982]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3998]
	eor	w16, w16, w17
	ldrh	w17, [sp, #4014]
	eor	w16, w16, w17
	eor	w15, w15, w16
	strh	w8, [sp, #944]
	and	w8, w8, #0xffff
	and	w8, w8, #0x1
	strh	w9, [sp, #946]
	and	w9, w9, #0xffff
	ubfiz	w9, w9, #1, #1
	strh	w10, [sp, #948]
	strh	w11, [sp, #950]
	strh	w12, [sp, #952]
	strh	w13, [sp, #954]
	add	x10, sp, #693
	ldur	d1, [x10, #255]
Lloh6:
	adrp	x10, lCPI0_3
Lloh7:
	ldr	d2, [x10, :lo12:lCPI0_3]
	ushl	v1.4h, v1.4h, v2.4h
Lloh8:
	adrp	x10, lCPI0_4
Lloh9:
	ldr	d2, [x10, :lo12:lCPI0_4]
	and	v1.8b, v1.8b, v2.8b
	fmov	x10, d1
	orr	x10, x10, x10, lsr #32
	lsr	x11, x10, #16
	orr	w10, w10, w11
	orr	w9, w10, w9
	strh	w14, [sp, #956]
	and	w11, w14, #0xffff
	bfi	w8, w11, #6, #1
	orr	w11, w9, w8
	and	w12, w11, #0xffc0
	orr	w8, w11, w15, lsl #7
	str	q0, [x19]
	ldrh	w13, [x19]
	sbfx	w13, w13, #0, #1
	and	w8, w13, w8
	and	w8, w8, #0xff
	and	w13, w15, #0x1
	bfi	w13, w11, #1, #7
	add	x11, x19, #2
	str	x11, [sp, #800] // 8-byte Folded Spill
	ldrh	w11, [x19, #2]
	and	w11, w11, #0x1
	neg	w11, w11
	and	w11, w13, w11
	ubfiz	w13, w13, #1, #7
	ubfx	w14, w10, #5, #1
	ubfx	w16, w10, #4, #1
	ubfx	w17, w10, #3, #1
	orr	w12, w13, w12, lsr #6
	add	x13, x19, #4
	str	x13, [sp, #816] // 8-byte Folded Spill
	bfi	w14, w12, #1, #7
	bfi	w16, w14, #1, #7
	bfi	w17, w16, #1, #7
	movi	v0.4h, #1
	ldur	d1, [x19, #4]
	str	q1, [sp, #784] // 16-byte Folded Spill
	and	v0.8b, v1.8b, v0.8b
	movi	v2.4h, #1
	ushll	v0.4s, v0.4h, #0
	neg	v0.4s, v0.4s
	fmov	s1, w12
	mov	v1.s[1], w14
	mov	v1.s[2], w16
	mov	v1.s[3], w17
	and	v0.16b, v1.16b, v0.16b
	ubfx	w10, w10, #2, #1
	bfi	w10, w17, #1, #7
	ldrh	w20, [x19, #12]
	and	w12, w20, #0x1
	neg	w12, w12
	and	w12, w10, w12
	ubfx	w9, w9, #1, #1
	orr	w9, w9, w10, lsl #1
	strh	w15, [sp, #958]
	ldrh	w27, [x19, #14]
	and	w10, w27, #0x1
	neg	w10, w10
	and	w9, w9, w10
	ext	v1.16b, v0.16b, v0.16b, #8
	eor	v0.8b, v0.8b, v1.8b
	fmov	x10, d0
	lsr	x13, x10, #32
	eor	w10, w10, w11
	eor	w10, w10, w13
	eor	w8, w8, w12
	eor	w8, w8, w9
	eor	w8, w10, w8
	and	w28, w8, #0x1
	dup	v0.4h, w8
	strh	w28, [sp, #1968]
Lloh10:
	adrp	x9, lCPI0_5
Lloh11:
	ldr	d1, [x9, :lo12:lCPI0_5]
	ushl	v0.4h, v0.4h, v1.4h
	and	v0.8b, v0.8b, v2.8b
	str	q0, [sp, #768] // 16-byte Folded Spill
	add	x24, sp, #1968
	stur	d0, [x24, #2]
	ubfx	w25, w8, #5, #1
	strh	w25, [sp, #1978]
	ubfx	w26, w8, #6, #1
	strh	w26, [sp, #1980]
	ubfx	w21, w8, #7, #1
	strh	w21, [sp, #1982]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldp	q2, q3, [sp, #944]
	eor	v0.16b, v2.16b, v0.16b
	eor	v1.16b, v3.16b, v1.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	ldr	q1, [sp, #3024]
	ldr	q0, [sp, #3008]
	ldr	q2, [sp, #1968]
	str	q2, [sp, #944]
	ldr	q2, [sp, #976]
	ldr	q3, [sp, #944]
	eor	v1.16b, v2.16b, v1.16b
	ldr	q2, [sp, #2992]
	eor	v6.16b, v2.16b, v0.16b
	stp	q6, q3, [sp, #736] // 32-byte Folded Spill
	str	q1, [sp, #3024]
	ldp	q1, q3, [sp, #992]
	ldr	q4, [sp, #3040]
	ldr	q5, [sp, #3024]
	eor	v1.16b, v1.16b, v4.16b
	eor3	v7.16b, v2.16b, v0.16b, v5.16b
	str	q1, [sp, #3040]
	ldr	q0, [sp, #1024]
	ldr	q1, [sp, #1040]
	ldr	q2, [sp, #3056]
	ldr	q4, [sp, #3040]
	eor	v2.16b, v3.16b, v2.16b
	eor3	v5.16b, v6.16b, v5.16b, v4.16b
	stp	q5, q7, [sp, #704] // 32-byte Folded Spill
	str	q2, [sp, #3056]
	ldr	q2, [sp, #3072]
	ldr	q3, [sp, #3056]
	eor	v0.16b, v0.16b, v2.16b
	eor3	v6.16b, v7.16b, v4.16b, v3.16b
	str	q0, [sp, #3072]
	ldr	q0, [sp, #3088]
	ldr	q2, [sp, #3072]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v7.16b, v5.16b, v3.16b, v2.16b
	stp	q7, q6, [sp, #672] // 32-byte Folded Spill
	str	q0, [sp, #3088]
	ldr	q0, [sp, #1056]
	ldr	q1, [sp, #1072]
	ldr	q3, [sp, #3104]
	ldr	q4, [sp, #3088]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v6.16b, v6.16b, v2.16b, v4.16b
	str	q0, [sp, #3104]
	ldr	q0, [sp, #1088]
	ldr	q2, [sp, #1104]
	ldr	q3, [sp, #3120]
	ldr	q5, [sp, #3104]
	eor	v1.16b, v1.16b, v3.16b
	eor3	v4.16b, v7.16b, v4.16b, v5.16b
	stp	q4, q6, [sp, #640] // 32-byte Folded Spill
	str	q1, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q3, [sp, #3120]
	eor	v0.16b, v0.16b, v1.16b
	eor3	v5.16b, v6.16b, v5.16b, v3.16b
	str	q0, [sp, #3136]
	ldr	q0, [sp, #3152]
	ldr	q1, [sp, #3136]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v6.16b, v4.16b, v3.16b, v1.16b
	stp	q6, q5, [sp, #608] // 32-byte Folded Spill
	str	q0, [sp, #3152]
	ldr	q0, [sp, #1120]
	ldr	q2, [sp, #1136]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #3152]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v1.16b, v4.16b
	str	q0, [sp, #3168]
	ldr	q0, [sp, #1152]
	ldr	q1, [sp, #1168]
	ldr	q3, [sp, #3184]
	ldr	q5, [sp, #3168]
	eor	v2.16b, v2.16b, v3.16b
	eor3	v4.16b, v6.16b, v4.16b, v5.16b
	stp	q4, q7, [sp, #576] // 32-byte Folded Spill
	str	q2, [sp, #3184]
	ldr	q2, [sp, #3200]
	ldr	q3, [sp, #3184]
	eor	v0.16b, v0.16b, v2.16b
	eor3	v5.16b, v7.16b, v5.16b, v3.16b
	str	q0, [sp, #3200]
	ldr	q0, [sp, #3216]
	ldr	q2, [sp, #3200]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v6.16b, v4.16b, v3.16b, v2.16b
	stp	q6, q5, [sp, #544] // 32-byte Folded Spill
	str	q0, [sp, #3216]
	ldr	q0, [sp, #1184]
	ldr	q1, [sp, #1200]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #3216]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v2.16b, v4.16b
	str	q0, [sp, #3232]
	ldr	q0, [sp, #1216]
	ldr	q2, [sp, #1232]
	ldr	q3, [sp, #3248]
	ldr	q5, [sp, #3232]
	eor	v1.16b, v1.16b, v3.16b
	eor3	v4.16b, v6.16b, v4.16b, v5.16b
	stp	q4, q7, [sp, #512] // 32-byte Folded Spill
	str	q1, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q3, [sp, #3248]
	eor	v0.16b, v0.16b, v1.16b
	eor3	v5.16b, v7.16b, v5.16b, v3.16b
	str	q0, [sp, #3264]
	ldr	q0, [sp, #3280]
	ldr	q1, [sp, #3264]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v6.16b, v4.16b, v3.16b, v1.16b
	stp	q6, q5, [sp, #480] // 32-byte Folded Spill
	str	q0, [sp, #3280]
	ldr	q0, [sp, #1248]
	ldr	q2, [sp, #1264]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #3280]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v1.16b, v4.16b
	str	q0, [sp, #3296]
	ldr	q0, [sp, #1280]
	ldr	q1, [sp, #1296]
	ldr	q3, [sp, #3312]
	ldr	q5, [sp, #3296]
	eor	v2.16b, v2.16b, v3.16b
	eor3	v4.16b, v6.16b, v4.16b, v5.16b
	stp	q4, q7, [sp, #448] // 32-byte Folded Spill
	str	q2, [sp, #3312]
	ldr	q2, [sp, #3328]
	ldr	q3, [sp, #3312]
	eor	v0.16b, v0.16b, v2.16b
	eor3	v5.16b, v7.16b, v5.16b, v3.16b
	str	q0, [sp, #3328]
	ldr	q0, [sp, #3344]
	ldr	q2, [sp, #3328]
	eor	v0.16b, v1.16b, v0.16b
	eor3	v6.16b, v4.16b, v3.16b, v2.16b
	stp	q6, q5, [sp, #416] // 32-byte Folded Spill
	str	q0, [sp, #3344]
	ldr	q0, [sp, #1312]
	ldr	q1, [sp, #1328]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #3344]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v2.16b, v4.16b
	str	q0, [sp, #3360]
	ldr	q0, [sp, #1344]
	ldr	q2, [sp, #1360]
	ldr	q3, [sp, #3376]
	ldr	q5, [sp, #3360]
	eor	v1.16b, v1.16b, v3.16b
	eor3	v4.16b, v6.16b, v4.16b, v5.16b
	stp	q4, q7, [sp, #384] // 32-byte Folded Spill
	str	q1, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q3, [sp, #3376]
	eor	v0.16b, v0.16b, v1.16b
	eor3	v5.16b, v7.16b, v5.16b, v3.16b
	str	q0, [sp, #3392]
	ldr	q0, [sp, #3408]
	ldr	q1, [sp, #3392]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v6.16b, v4.16b, v3.16b, v1.16b
	stp	q6, q5, [sp, #352] // 32-byte Folded Spill
	str	q0, [sp, #3408]
	ldr	q0, [sp, #1376]
	ldr	q2, [sp, #1392]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #3408]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v1.16b, v4.16b
	str	q0, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3424]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v5.16b, v6.16b, v4.16b, v1.16b
	stp	q5, q7, [sp, #320] // 32-byte Folded Spill
	str	q0, [sp, #3440]
	ldr	q0, [sp, #1408]
	ldr	q2, [sp, #1424]
	ldr	q3, [sp, #3456]
	ldr	q4, [sp, #3440]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v6.16b, v7.16b, v1.16b, v4.16b
	str	q0, [sp, #3456]
	ldr	q0, [sp, #3472]
	ldr	q1, [sp, #3456]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v7.16b, v5.16b, v4.16b, v1.16b
	stp	q7, q6, [sp, #288] // 32-byte Folded Spill
	str	q0, [sp, #3472]
	ldr	q0, [sp, #1440]
	ldr	q2, [sp, #1456]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #3472]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v5.16b, v6.16b, v1.16b, v4.16b
	str	q0, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3488]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v6.16b, v7.16b, v4.16b, v1.16b
	stp	q6, q5, [sp, #256] // 32-byte Folded Spill
	str	q0, [sp, #3504]
	ldr	q0, [sp, #1472]
	ldr	q2, [sp, #1488]
	ldr	q3, [sp, #3520]
	ldr	q4, [sp, #3504]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v5.16b, v1.16b, v4.16b
	str	q0, [sp, #3520]
	ldr	q0, [sp, #3536]
	ldr	q1, [sp, #3520]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v5.16b, v6.16b, v4.16b, v1.16b
	stp	q5, q7, [sp, #224] // 32-byte Folded Spill
	str	q0, [sp, #3536]
	ldr	q0, [sp, #1504]
	ldr	q2, [sp, #1520]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #3536]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v15.16b, v7.16b, v1.16b, v4.16b
	str	q0, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3552]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v14.16b, v5.16b, v4.16b, v1.16b
	str	q0, [sp, #3568]
	ldr	q0, [sp, #1536]
	ldr	q2, [sp, #1552]
	ldr	q3, [sp, #3584]
	ldr	q4, [sp, #3568]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v13.16b, v15.16b, v1.16b, v4.16b
	str	q0, [sp, #3584]
	ldr	q0, [sp, #3600]
	ldr	q1, [sp, #3584]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v12.16b, v14.16b, v4.16b, v1.16b
	str	q0, [sp, #3600]
	ldr	q0, [sp, #1568]
	ldr	q2, [sp, #1584]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #3600]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v11.16b, v13.16b, v1.16b, v4.16b
	str	q0, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3616]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v10.16b, v12.16b, v4.16b, v1.16b
	str	q0, [sp, #3632]
	ldr	q0, [sp, #1600]
	ldr	q2, [sp, #1616]
	ldr	q3, [sp, #3648]
	ldr	q4, [sp, #3632]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v9.16b, v11.16b, v1.16b, v4.16b
	str	q0, [sp, #3648]
	ldr	q0, [sp, #3664]
	ldr	q1, [sp, #3648]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v8.16b, v10.16b, v4.16b, v1.16b
	str	q0, [sp, #3664]
	ldr	q0, [sp, #1632]
	ldr	q2, [sp, #1648]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #3664]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v31.16b, v9.16b, v1.16b, v4.16b
	str	q0, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3680]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v30.16b, v8.16b, v4.16b, v1.16b
	str	q0, [sp, #3696]
	ldr	q0, [sp, #1664]
	ldr	q2, [sp, #1680]
	ldr	q3, [sp, #3712]
	ldr	q4, [sp, #3696]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v29.16b, v31.16b, v1.16b, v4.16b
	str	q0, [sp, #3712]
	ldr	q0, [sp, #3728]
	ldr	q1, [sp, #3712]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v28.16b, v30.16b, v4.16b, v1.16b
	str	q0, [sp, #3728]
	ldr	q0, [sp, #1696]
	ldr	q2, [sp, #1712]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #3728]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v27.16b, v29.16b, v1.16b, v4.16b
	str	q0, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3744]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v26.16b, v28.16b, v4.16b, v1.16b
	str	q0, [sp, #3760]
	ldr	q0, [sp, #1728]
	ldr	q2, [sp, #1744]
	ldr	q3, [sp, #3776]
	ldr	q4, [sp, #3760]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v25.16b, v27.16b, v1.16b, v4.16b
	str	q0, [sp, #3776]
	ldr	q0, [sp, #3792]
	ldr	q1, [sp, #3776]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v24.16b, v26.16b, v4.16b, v1.16b
	str	q0, [sp, #3792]
	ldr	q0, [sp, #1760]
	ldr	q2, [sp, #1776]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #3792]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v23.16b, v25.16b, v1.16b, v4.16b
	str	q0, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3808]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v22.16b, v24.16b, v4.16b, v1.16b
	str	q0, [sp, #3824]
	ldr	q0, [sp, #1792]
	ldr	q2, [sp, #1808]
	ldr	q3, [sp, #3840]
	ldr	q4, [sp, #3824]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v21.16b, v23.16b, v1.16b, v4.16b
	str	q0, [sp, #3840]
	ldr	q0, [sp, #3856]
	ldr	q1, [sp, #3840]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v20.16b, v22.16b, v4.16b, v1.16b
	str	q0, [sp, #3856]
	ldr	q0, [sp, #1824]
	ldr	q2, [sp, #1840]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #3856]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v19.16b, v21.16b, v1.16b, v4.16b
	str	q0, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3872]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v18.16b, v20.16b, v4.16b, v1.16b
	str	q0, [sp, #3888]
	ldr	q0, [sp, #1856]
	ldr	q2, [sp, #1872]
	ldr	q3, [sp, #3904]
	ldr	q4, [sp, #3888]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v17.16b, v19.16b, v1.16b, v4.16b
	str	q0, [sp, #3904]
	ldr	q0, [sp, #3920]
	ldr	q1, [sp, #3904]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v16.16b, v18.16b, v4.16b, v1.16b
	str	q0, [sp, #3920]
	ldr	q0, [sp, #1888]
	ldr	q2, [sp, #1904]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #3920]
	eor	v0.16b, v0.16b, v3.16b
	eor3	v7.16b, v17.16b, v1.16b, v4.16b
	str	q0, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3936]
	eor	v0.16b, v2.16b, v0.16b
	eor3	v6.16b, v16.16b, v4.16b, v1.16b
	str	q0, [sp, #3952]
	ldr	q2, [sp, #1920]
	ldr	q4, [sp, #1936]
	ldr	q3, [sp, #3968]
	ldr	q0, [sp, #3952]
	eor	v2.16b, v2.16b, v3.16b
	eor3	v5.16b, v7.16b, v1.16b, v0.16b
	str	q2, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #3968]
	eor	v4.16b, v4.16b, v2.16b
	eor3	v2.16b, v6.16b, v0.16b, v3.16b
	str	q4, [sp, #3984]
	ldr	q4, [sp, #1952]
	ldr	q0, [sp, #4000]
	ldr	q1, [sp, #3984]
	eor	v0.16b, v4.16b, v0.16b
	ldr	q4, [sp, #736] // 16-byte Folded Reload
	str	q4, [sp, #3008]
	ldr	q4, [sp, #720] // 16-byte Folded Reload
	str	q4, [sp, #3024]
	ldr	q4, [sp, #704] // 16-byte Folded Reload
	str	q4, [sp, #3040]
	ldr	q4, [sp, #688] // 16-byte Folded Reload
	str	q4, [sp, #3056]
	ldr	q4, [sp, #672] // 16-byte Folded Reload
	str	q4, [sp, #3072]
	ldr	q4, [sp, #656] // 16-byte Folded Reload
	str	q4, [sp, #3088]
	ldr	q4, [sp, #640] // 16-byte Folded Reload
	str	q4, [sp, #3104]
	ldr	q4, [sp, #624] // 16-byte Folded Reload
	str	q4, [sp, #3120]
	ldr	q4, [sp, #608] // 16-byte Folded Reload
	str	q4, [sp, #3136]
	ldr	q4, [sp, #592] // 16-byte Folded Reload
	str	q4, [sp, #3152]
	ldr	q4, [sp, #576] // 16-byte Folded Reload
	str	q4, [sp, #3168]
	ldr	q4, [sp, #560] // 16-byte Folded Reload
	str	q4, [sp, #3184]
	ldr	q4, [sp, #544] // 16-byte Folded Reload
	str	q4, [sp, #3200]
	ldr	q4, [sp, #528] // 16-byte Folded Reload
	str	q4, [sp, #3216]
	ldr	q4, [sp, #512] // 16-byte Folded Reload
	str	q4, [sp, #3232]
	ldr	q4, [sp, #496] // 16-byte Folded Reload
	str	q4, [sp, #3248]
	ldr	q4, [sp, #480] // 16-byte Folded Reload
	str	q4, [sp, #3264]
	ldr	q4, [sp, #464] // 16-byte Folded Reload
	str	q4, [sp, #3280]
	ldr	q4, [sp, #448] // 16-byte Folded Reload
	str	q4, [sp, #3296]
	ldr	q4, [sp, #432] // 16-byte Folded Reload
	str	q4, [sp, #3312]
	ldr	q4, [sp, #416] // 16-byte Folded Reload
	str	q4, [sp, #3328]
	ldr	q4, [sp, #400] // 16-byte Folded Reload
	str	q4, [sp, #3344]
	ldr	q4, [sp, #384] // 16-byte Folded Reload
	str	q4, [sp, #3360]
	ldr	q4, [sp, #368] // 16-byte Folded Reload
	str	q4, [sp, #3376]
	ldr	q4, [sp, #352] // 16-byte Folded Reload
	str	q4, [sp, #3392]
	ldr	q4, [sp, #336] // 16-byte Folded Reload
	str	q4, [sp, #3408]
	ldr	q4, [sp, #320] // 16-byte Folded Reload
	str	q4, [sp, #3424]
	ldr	q4, [sp, #304] // 16-byte Folded Reload
	str	q4, [sp, #3440]
	ldr	q4, [sp, #288] // 16-byte Folded Reload
	str	q4, [sp, #3456]
	ldr	q4, [sp, #272] // 16-byte Folded Reload
	str	q4, [sp, #3472]
	ldr	q4, [sp, #256] // 16-byte Folded Reload
	str	q4, [sp, #3488]
	ldr	q4, [sp, #240] // 16-byte Folded Reload
	str	q4, [sp, #3504]
	ldr	q4, [sp, #224] // 16-byte Folded Reload
	str	q4, [sp, #3520]
	str	q15, [sp, #3536]
	str	q14, [sp, #3552]
	str	q13, [sp, #3568]
	str	q12, [sp, #3584]
	str	q11, [sp, #3600]
	str	q10, [sp, #3616]
	str	q9, [sp, #3632]
	str	q8, [sp, #3648]
	str	q31, [sp, #3664]
	str	q30, [sp, #3680]
	str	q29, [sp, #3696]
	str	q28, [sp, #3712]
	str	q27, [sp, #3728]
	str	q26, [sp, #3744]
	str	q25, [sp, #3760]
	str	q24, [sp, #3776]
	str	q23, [sp, #3792]
	str	q22, [sp, #3808]
	str	q21, [sp, #3824]
	str	q20, [sp, #3840]
	str	q19, [sp, #3856]
	str	q18, [sp, #3872]
	str	q17, [sp, #3888]
	str	q16, [sp, #3904]
	str	q7, [sp, #3920]
	str	q6, [sp, #3936]
	str	q5, [sp, #3952]
	str	q2, [sp, #3968]
	eor3	v3.16b, v5.16b, v3.16b, v1.16b
	str	q0, [sp, #4000]
	str	q3, [sp, #3984]
	ldr	q0, [sp, #4000]
	eor3	v0.16b, v2.16b, v1.16b, v0.16b
	str	q0, [sp, #4000]
	strh	w28, [sp, #960]
	ldp	q2, q0, [sp, #752] // 32-byte Folded Reload
	str	h0, [sp, #962]
	add	x9, sp, #944
	add	x8, x9, #20
	st1	{v0.h}[1], [x8]
	add	x8, x9, #22
	st1	{v0.h}[2], [x8]
	add	x8, x9, #24
	st1	{v0.h}[3], [x8]
	mov	v1.16b, v0.16b
	strh	w25, [sp, #970]
	strh	w26, [sp, #972]
	ldr	q0, [sp, #784] // 16-byte Folded Reload
	ext	v0.16b, v0.16b, v0.16b, #12
	ld1	{v0.h}[0], [x19]
	ldr	x8, [sp, #800] // 8-byte Folded Reload
	ld1	{v0.h}[1], [x8]
	mov	v0.h[6], w20
	strh	w21, [sp, #974]
	mov	v0.h[7], w27
	eor	v0.16b, v0.16b, v2.16b
	str	q0, [x19]
	ldur	d0, [x19, #18]
	eor	v0.8b, v0.8b, v1.8b
	stur	d0, [x19, #18]
	ldr	x8, [sp, #848] // 8-byte Folded Reload
	str	wzr, [x8, #24]
	stp	xzr, xzr, [x8, #8]
	str	xzr, [x8]
	ldr	d4, [sp, #2992]
	ldrh	w8, [sp, #950]
	ldrh	w9, [sp, #2998]
	eor	w8, w8, w9
	ldr	x9, [sp, #832] // 8-byte Folded Reload
	ld1	{v4.h}[2], [x9]
	mov	v4.h[3], w8
	ldr	d2, [sp, #3024]
	ldr	d0, [sp, #3032]
	ldr	d3, [sp, #3056]
	ldr	d1, [sp, #3064]
	eor	v3.8b, v2.8b, v3.8b
	ldr	d5, [sp, #3088]
	ldr	d2, [sp, #3096]
	eor	v5.8b, v3.8b, v5.8b
	ldr	d6, [sp, #3120]
	ldr	d3, [sp, #3128]
	eor	v6.8b, v5.8b, v6.8b
	ldr	d7, [sp, #3152]
	ldr	d5, [sp, #3160]
	eor	v7.8b, v6.8b, v7.8b
	ldr	d16, [sp, #3184]
	ldr	d6, [sp, #3192]
	eor	v16.8b, v7.8b, v16.8b
	ldr	d17, [sp, #3216]
	ldr	d7, [sp, #3224]
	eor	v17.8b, v16.8b, v17.8b
	ldr	d18, [sp, #3248]
	ldr	d16, [sp, #3256]
	eor	v17.8b, v17.8b, v18.8b
	eor	v4.8b, v4.8b, v17.8b
	ldr	d19, [sp, #3280]
	ldr	d17, [sp, #3288]
	ldr	d20, [sp, #3312]
	ldr	d18, [sp, #3320]
	eor	v20.8b, v19.8b, v20.8b
	ldr	d21, [sp, #3344]
	ldr	d19, [sp, #3352]
	eor	v21.8b, v20.8b, v21.8b
	ldr	d22, [sp, #3376]
	ldr	d20, [sp, #3384]
	eor	v22.8b, v21.8b, v22.8b
	ldr	d23, [sp, #3408]
	ldr	d21, [sp, #3416]
	eor	v23.8b, v22.8b, v23.8b
	ldr	d24, [sp, #3440]
	ldr	d22, [sp, #3448]
	eor	v24.8b, v23.8b, v24.8b
	ldr	d25, [sp, #3472]
	ldr	d23, [sp, #3480]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3504]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3536]
	eor	v24.8b, v24.8b, v25.8b
	eor	v4.8b, v4.8b, v24.8b
	ldr	d24, [sp, #3568]
	ldr	d25, [sp, #3600]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3632]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3664]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3696]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3728]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3760]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3792]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3824]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3856]
	eor	v24.8b, v24.8b, v25.8b
	eor	v4.8b, v4.8b, v24.8b
	ldr	d24, [sp, #3888]
	ldr	d25, [sp, #3920]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3952]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3984]
	eor	v24.8b, v24.8b, v25.8b
	eor	v24.8b, v4.8b, v24.8b
	ldr	d25, [sp, #3000]
	ldr	d26, [sp, #952]
	ldr	d4, [sp, #960]
	eor	v25.8b, v26.8b, v25.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v0.8b, v25.8b, v0.8b
	eor	v1.8b, v2.8b, v3.8b
	eor	v1.8b, v1.8b, v5.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v1.8b, v6.8b, v7.8b
	eor	v1.8b, v1.8b, v16.8b
	eor	v1.8b, v1.8b, v17.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v1.8b, v18.8b, v19.8b
	eor	v1.8b, v1.8b, v20.8b
	eor	v1.8b, v1.8b, v21.8b
	eor	v1.8b, v1.8b, v22.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3512]
	eor	v1.8b, v23.8b, v1.8b
	ldr	d2, [sp, #3544]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3576]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3608]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3640]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3672]
	ldr	d2, [sp, #3704]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3736]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3768]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3800]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3832]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3864]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3896]
	ldr	d2, [sp, #3928]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3960]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3992]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	str	d24, [sp, #944]
	str	d0, [sp, #952]
	ldr	d1, [sp, #3008]
	ldr	d0, [sp, #3016]
	eor	v3.8b, v4.8b, v1.8b
	ldr	d4, [sp, #3040]
	ldr	d1, [sp, #3048]
	ldr	d5, [sp, #3072]
	ldr	d2, [sp, #3080]
	eor	v4.8b, v4.8b, v5.8b
	eor	v6.8b, v3.8b, v4.8b
	ldr	d5, [sp, #3104]
	ldr	d3, [sp, #3112]
	ldr	d7, [sp, #3136]
	ldr	d4, [sp, #3144]
	eor	v7.8b, v5.8b, v7.8b
	ldr	d16, [sp, #3168]
	ldr	d5, [sp, #3176]
	eor	v7.8b, v7.8b, v16.8b
	eor	v16.8b, v6.8b, v7.8b
	ldr	d6, [sp, #3200]
	ldr	d7, [sp, #3208]
	ldr	d18, [sp, #3232]
	ldr	d17, [sp, #3240]
	eor	v6.8b, v6.8b, v18.8b
	ldr	d18, [sp, #3264]
	ldr	d19, [sp, #3272]
	eor	v18.8b, v6.8b, v18.8b
	ldr	d20, [sp, #3296]
	ldr	d6, [sp, #3304]
	eor	v18.8b, v18.8b, v20.8b
	eor	v23.8b, v16.8b, v18.8b
	ldr	d20, [sp, #3328]
	ldr	d16, [sp, #3336]
	ldr	d21, [sp, #3360]
	ldr	d18, [sp, #3368]
	eor	v21.8b, v20.8b, v21.8b
	ldr	d22, [sp, #3392]
	ldr	d20, [sp, #3400]
	eor	v22.8b, v21.8b, v22.8b
	ldr	d24, [sp, #3424]
	ldr	d21, [sp, #3432]
	eor	v24.8b, v22.8b, v24.8b
	ldr	d25, [sp, #3456]
	ldr	d22, [sp, #3464]
	eor	v24.8b, v24.8b, v25.8b
	eor	v23.8b, v23.8b, v24.8b
	ldr	d24, [sp, #3520]
	ldr	d25, [sp, #3488]
	ldr	d26, [sp, #3496]
	eor	v24.8b, v25.8b, v24.8b
	ldr	d25, [sp, #3552]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3584]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3616]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3648]
	eor	v24.8b, v24.8b, v25.8b
	eor	v23.8b, v23.8b, v24.8b
	ldr	d24, [sp, #3680]
	ldr	d25, [sp, #3712]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3744]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3776]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3808]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3840]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3872]
	eor	v24.8b, v24.8b, v25.8b
	eor	v23.8b, v23.8b, v24.8b
	ldr	d24, [sp, #3904]
	ldr	d25, [sp, #3936]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #3968]
	eor	v24.8b, v24.8b, v25.8b
	ldr	d25, [sp, #4000]
	eor	v24.8b, v24.8b, v25.8b
	eor	v23.8b, v23.8b, v24.8b
	ldr	d24, [sp, #968]
	eor	v0.8b, v24.8b, v0.8b
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v1.8b, v3.8b, v4.8b
	eor	v1.8b, v1.8b, v5.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v1.8b, v7.8b, v17.8b
	eor	v1.8b, v1.8b, v19.8b
	ldrh	w8, [x19, #16]
	eor	w8, w8, w28
	strh	w8, [x19, #16]
	ldrh	w8, [x19, #26]
	eor	w8, w8, w25
	strh	w8, [x19, #26]
	ldrh	w8, [x19, #28]
	eor	w8, w8, w26
	strh	w8, [x19, #28]
	ldrh	w8, [x19, #30]
	eor	w8, w8, w21
	strh	w8, [x19, #30]
	eor	v1.8b, v1.8b, v6.8b
	eor	v0.8b, v0.8b, v1.8b
	eor	v1.8b, v16.8b, v18.8b
	eor	v1.8b, v1.8b, v20.8b
	eor	v1.8b, v1.8b, v21.8b
	eor	v1.8b, v1.8b, v22.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3528]
	eor	v1.8b, v26.8b, v1.8b
	ldr	d2, [sp, #3560]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3592]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3624]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3656]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3688]
	ldr	d2, [sp, #3720]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3752]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3784]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3816]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3848]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3880]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	ldr	d1, [sp, #3912]
	ldr	d2, [sp, #3944]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #3976]
	eor	v1.8b, v1.8b, v2.8b
	ldr	d2, [sp, #4008]
	eor	v1.8b, v1.8b, v2.8b
	eor	v0.8b, v0.8b, v1.8b
	str	d23, [sp, #960]
	str	d0, [sp, #968]
	ldrh	w8, [sp, #944]
	and	w8, w8, #0x1
	ldrh	w9, [sp, #946]
	ubfiz	w9, w9, #1, #1
	add	x10, sp, #693
	ldur	q1, [x10, #255]
Lloh12:
	adrp	x10, lCPI0_6
Lloh13:
	ldr	q2, [x10, :lo12:lCPI0_6]
	str	q2, [sp, #848] // 16-byte Folded Spill
	ushl	v1.8h, v1.8h, v2.8h
Lloh14:
	adrp	x10, lCPI0_7
Lloh15:
	ldr	q2, [x10, :lo12:lCPI0_7]
	str	q2, [sp, #832] // 16-byte Folded Spill
	and	v1.16b, v1.16b, v2.16b
	add	x10, sp, #709
	ldur	d2, [x10, #255]
Lloh16:
	adrp	x10, lCPI0_8
Lloh17:
	ldr	d10, [x10, :lo12:lCPI0_8]
	ushl	v2.4h, v2.4h, v10.4h
Lloh18:
	adrp	x10, lCPI0_9
Lloh19:
	ldr	d9, [x10, :lo12:lCPI0_9]
	and	v2.8b, v2.8b, v9.8b
	fmov	x10, d2
	ldrh	w11, [sp, #972]
	ext	v2.16b, v1.16b, v1.16b, #8
	orr	v1.8b, v1.8b, v2.8b
	fmov	x12, d1
	orr	x12, x12, x12, lsr #32
	lsr	x13, x12, #16
	orr	x10, x10, x10, lsr #32
	lsr	x14, x10, #16
	orr	w10, w10, w14
	orr	w10, w12, w10
	orr	w10, w10, w13
	orr	w9, w10, w9
	bfi	w8, w11, #14, #1
	orr	w11, w9, w8
	and	x8, x11, #0xffff
	umov	w12, v0.h[3]
	and	w13, w12, #0x1
	orr	w13, w13, w8, lsl #1
	ldrh	w8, [x19, #2]
	and	w8, w8, #0x1
	neg	w8, w8
	and	w8, w13, w8
	ubfx	x14, x11, #14, #2
	ubfiz	w13, w13, #1, #15
	ubfx	w15, w10, #13, #1
	ubfx	w16, w10, #12, #1
	ubfx	w17, w10, #11, #1
	ubfx	w0, w10, #10, #1
	ubfx	w1, w10, #9, #1
	ubfx	w2, w10, #8, #1
	ldur	d0, [x19, #10]
	orr	w11, w11, w12, lsl #15
	orr	w12, w13, w14
	bfi	w15, w12, #1, #15
	bfi	w16, w15, #1, #15
	bfi	w17, w16, #1, #15
	bfi	w0, w17, #1, #15
	bfi	w1, w0, #1, #15
	bfi	w2, w1, #1, #15
	ldur	s1, [x19, #6]
	ldr	h2, [x19]
	ldr	x13, [sp, #816] // 8-byte Folded Reload
	ld1	{v2.h}[1], [x13]
	zip1	v1.2s, v2.2s, v1.2s
	mov	v1.d[1], v0.d[0]
	movi	v0.8h, #1
	and	v1.16b, v1.16b, v0.16b
	fmov	s2, w11
	mov	v2.h[1], w12
	mov	v2.h[2], w15
	mov	v2.h[3], w16
	mov	v2.h[4], w17
	mov	v2.h[5], w0
	mov	v2.h[6], w1
	mov	v2.h[7], w2
	cmeq	v1.8h, v1.8h, #0
	bic	v1.16b, v2.16b, v1.16b
	ubfx	w11, w10, #7, #1
	ubfx	w12, w10, #6, #1
	ubfx	w13, w10, #5, #1
	ubfx	w14, w10, #4, #1
	bfi	w11, w2, #1, #15
	bfi	w12, w11, #1, #15
	bfi	w13, w12, #1, #15
	bfi	w14, w13, #1, #15
	ldur	d2, [x19, #18]
	fmov	s3, w11
	mov	v3.h[1], w12
	movi	v4.4h, #1
	and	v2.8b, v2.8b, v4.8b
	mov	v3.h[2], w13
	mov	v3.h[3], w14
	cmeq	v2.4h, v2.4h, #0
	bic	v2.8b, v3.8b, v2.8b
	fmov	x11, d2
	ubfx	w12, w10, #3, #1
	bfi	w12, w14, #1, #15
	ldrh	w13, [x19, #26]
	and	w13, w13, #0x1
	neg	w13, w13
	and	w13, w12, w13
	ubfx	w10, w10, #2, #1
	bfi	w10, w12, #1, #15
	ldrh	w12, [x19, #28]
	and	w12, w12, #0x1
	neg	w12, w12
	and	w12, w10, w12
	ubfx	w9, w9, #1, #1
	orr	w9, w9, w10, lsl #1
	ldrh	w10, [x19, #30]
	and	w10, w10, #0x1
	neg	w10, w10
	and	w9, w9, w10
	ext	v2.16b, v1.16b, v1.16b, #8
	eor	v1.8b, v1.8b, v2.8b
	fmov	x10, d1
	eor	x10, x10, x10, lsr #32
	lsr	x14, x10, #16
	eor	x11, x11, x11, lsr #32
	lsr	x15, x11, #16
	eor	w11, w11, w15
	eor	w10, w10, w11
	eor	w10, w10, w14
	eor	w11, w13, w12
	eor	w8, w8, w11
	eor	w8, w8, w9
	eor	w8, w8, w10
	and	w9, w8, #0x1
	strh	w9, [sp, #1968]
	dup	v1.8h, w8
Lloh20:
	adrp	x9, lCPI0_10
Lloh21:
	ldr	q2, [x9, :lo12:lCPI0_10]
	str	q2, [sp, #736] // 16-byte Folded Spill
	ushl	v2.8h, v1.8h, v2.8h
	and	v0.16b, v2.16b, v0.16b
	stur	q0, [x24, #2]
Lloh22:
	adrp	x9, lCPI0_11
Lloh23:
	ldr	d8, [x9, :lo12:lCPI0_11]
	ushl	v0.4h, v1.4h, v8.4h
	bic	v0.4h, #126
	stur	d0, [x24, #18]
	ubfx	w9, w8, #13, #1
	strh	w9, [sp, #1994]
	ubfx	w9, w8, #14, #1
	strh	w9, [sp, #1996]
	ubfx	w8, w8, #15, #1
	strh	w8, [sp, #1998]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	w8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldp	q2, q3, [sp, #944]
	eor	v0.16b, v2.16b, v0.16b
	eor	v1.16b, v3.16b, v1.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	ldur	q0, [x22, #8]
	ldr	q1, [sp, #3024]
	ldr	q2, [sp, #3040]
	ldp	q3, q4, [sp, #976]
	eor	v1.16b, v3.16b, v1.16b
	eor	v2.16b, v4.16b, v2.16b
	str	q1, [sp, #3024]
	str	q2, [sp, #3040]
	ldr	q1, [sp, #3056]
	ldr	q2, [sp, #3072]
	ldr	q3, [sp, #3088]
	ldr	q4, [sp, #3104]
	ldp	q5, q6, [sp, #1008]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3056]
	str	q2, [sp, #3072]
	str	q3, [sp, #3088]
	str	q4, [sp, #3104]
	ldr	q1, [sp, #3120]
	ldr	q2, [sp, #3136]
	ldr	q3, [sp, #3152]
	ldr	q4, [sp, #3168]
	ldr	q5, [sp, #1072]
	ldr	q6, [sp, #1088]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3120]
	str	q2, [sp, #3136]
	str	q3, [sp, #3152]
	str	q4, [sp, #3168]
	ldr	q1, [sp, #3184]
	ldr	q2, [sp, #3200]
	ldr	q3, [sp, #3216]
	ldr	q4, [sp, #3232]
	ldr	q5, [sp, #1136]
	ldr	q6, [sp, #1152]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3184]
	str	q2, [sp, #3200]
	str	q3, [sp, #3216]
	str	q4, [sp, #3232]
	ldr	q1, [sp, #3248]
	ldr	q2, [sp, #3264]
	ldr	q3, [sp, #3280]
	ldr	q4, [sp, #3296]
	ldr	q5, [sp, #1200]
	ldr	q6, [sp, #1216]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3248]
	str	q2, [sp, #3264]
	str	q3, [sp, #3280]
	str	q4, [sp, #3296]
	ldr	q1, [sp, #3312]
	ldr	q2, [sp, #3328]
	ldr	q3, [sp, #3344]
	ldr	q4, [sp, #3360]
	ldr	q5, [sp, #1264]
	ldr	q6, [sp, #1280]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3312]
	str	q2, [sp, #3328]
	str	q3, [sp, #3344]
	str	q4, [sp, #3360]
	ldr	q1, [sp, #3376]
	ldr	q2, [sp, #3392]
	ldr	q3, [sp, #3408]
	ldr	q4, [sp, #3424]
	ldr	q5, [sp, #1328]
	ldr	q6, [sp, #1344]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3376]
	str	q2, [sp, #3392]
	str	q3, [sp, #3408]
	str	q4, [sp, #3424]
	ldr	q1, [sp, #3440]
	ldr	q2, [sp, #3456]
	ldr	q3, [sp, #3472]
	ldr	q4, [sp, #3488]
	ldr	q5, [sp, #1392]
	ldr	q6, [sp, #1408]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3440]
	str	q2, [sp, #3456]
	str	q3, [sp, #3472]
	str	q4, [sp, #3488]
	ldr	q1, [sp, #3504]
	ldr	q2, [sp, #3520]
	ldr	q3, [sp, #3536]
	ldr	q4, [sp, #3552]
	ldr	q5, [sp, #1456]
	ldr	q6, [sp, #1472]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3504]
	str	q2, [sp, #3520]
	str	q3, [sp, #3536]
	str	q4, [sp, #3552]
	ldr	q1, [sp, #3568]
	ldr	q2, [sp, #3584]
	ldr	q3, [sp, #3600]
	ldr	q4, [sp, #3616]
	ldr	q5, [sp, #1520]
	ldr	q6, [sp, #1536]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3568]
	str	q2, [sp, #3584]
	str	q3, [sp, #3600]
	str	q4, [sp, #3616]
	ldr	q1, [sp, #3632]
	ldr	q2, [sp, #3648]
	ldr	q3, [sp, #3664]
	ldr	q4, [sp, #3680]
	ldr	q5, [sp, #1584]
	ldr	q6, [sp, #1600]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3632]
	str	q2, [sp, #3648]
	str	q3, [sp, #3664]
	str	q4, [sp, #3680]
	ldr	q1, [sp, #3696]
	ldr	q2, [sp, #3712]
	ldr	q3, [sp, #3728]
	ldr	q4, [sp, #3744]
	ldr	q5, [sp, #1648]
	ldr	q6, [sp, #1664]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3696]
	str	q2, [sp, #3712]
	str	q3, [sp, #3728]
	str	q4, [sp, #3744]
	ldr	q1, [sp, #3760]
	ldr	q2, [sp, #3776]
	ldr	q3, [sp, #3792]
	ldr	q4, [sp, #3808]
	ldr	q5, [sp, #1712]
	ldr	q6, [sp, #1728]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3760]
	str	q2, [sp, #3776]
	str	q3, [sp, #3792]
	str	q4, [sp, #3808]
	ldr	q1, [sp, #3824]
	ldr	q2, [sp, #3840]
	ldr	q3, [sp, #3856]
	ldr	q4, [sp, #3872]
	ldr	q5, [sp, #1776]
	ldr	q6, [sp, #1792]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3824]
	str	q2, [sp, #3840]
	str	q3, [sp, #3856]
	str	q4, [sp, #3872]
	ldr	q1, [sp, #3888]
	ldr	q2, [sp, #3904]
	ldr	q3, [sp, #3920]
	ldr	q4, [sp, #3936]
	ldr	q5, [sp, #1840]
	ldr	q6, [sp, #1856]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3888]
	str	q2, [sp, #3904]
	str	q3, [sp, #3920]
	str	q4, [sp, #3936]
	ldr	q1, [sp, #3952]
	ldr	q2, [sp, #3968]
	ldr	q3, [sp, #3984]
	ldr	q4, [sp, #4000]
	ldr	q5, [sp, #1904]
	ldr	q6, [sp, #1920]
	eor	v1.16b, v5.16b, v1.16b
	ldr	q5, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v5.16b, v3.16b
	eor	v4.16b, v7.16b, v4.16b
	str	q1, [sp, #3952]
	str	q2, [sp, #3968]
	str	q3, [sp, #3984]
	str	q4, [sp, #4000]
LBB0_13: // =>This Inner Loop Header: Depth=1
	ldur	d1, [x23, #-48]
	mov	v1.d[1], v0.d[0]
	ldp	q2, q3, [x23, #-16]
	eor	v1.16b, v1.16b, v2.16b
	ldur	d2, [x23, #-24]
	ext	v0.16b, v0.16b, v0.16b, #8
	mov	v0.d[1], v2.d[0]
	eor	v0.16b, v0.16b, v3.16b
	stp	q1, q0, [x23, #-16]
	ext	v0.16b, v1.16b, v0.16b, #8
	add	x23, x23, #32
	add	w8, w8, #16
	cmp	w8, #496
	b.lo	LBB0_13
// %bb.14:
	mov	x9, #0 // =0x0
	mov	x8, #0 // =0x0
	ldr	q2, [sp, #1968]
	ldr	q0, [sp, #1968]
	ldr	q3, [sp, #1984]
	ldr	q1, [sp, #1984]
	stp	q2, q3, [sp, #944]
	stp	q0, q1, [sp, #976]
	ldp	q2, q3, [sp, #944]
	movi	v4.2d, #0000000000000000
	ldr	x10, [sp, #864] // 8-byte Folded Reload
	stp	q4, q4, [x10, #16]
	str	xzr, [x10, #48]
	ldr	q5, [sp, #2992]
	ldr	d6, [sp, #3000]
	mov	v5.d[1], v6.d[0]
	str	q4, [x10]
	ldr	q4, [sp, #3056]
	ldr	q6, [sp, #3072]
	eor	v4.16b, v5.16b, v4.16b
	ldr	q5, [sp, #3120]
	ldr	q7, [sp, #3136]
	ldr	q16, [sp, #3184]
	ldr	q17, [sp, #3200]
	eor3	v4.16b, v4.16b, v5.16b, v16.16b
	ldr	q5, [sp, #3248]
	ldr	q16, [sp, #3264]
	ldr	q18, [sp, #3312]
	ldr	q19, [sp, #3328]
	eor3	v4.16b, v4.16b, v5.16b, v18.16b
	ldr	q5, [sp, #3376]
	ldr	q18, [sp, #3392]
	ldr	q20, [sp, #3440]
	ldr	q21, [sp, #3456]
	eor3	v4.16b, v4.16b, v5.16b, v20.16b
	ldr	q5, [sp, #3504]
	ldr	q20, [sp, #3520]
	ldr	q22, [sp, #3568]
	ldr	q23, [sp, #3584]
	eor3	v4.16b, v4.16b, v5.16b, v22.16b
	ldr	q5, [sp, #3632]
	ldr	q22, [sp, #3648]
	ldr	q24, [sp, #3696]
	ldr	q25, [sp, #3712]
	eor3	v4.16b, v4.16b, v5.16b, v24.16b
	ldr	q5, [sp, #3760]
	ldr	q24, [sp, #3776]
	ldr	q26, [sp, #3824]
	ldr	q27, [sp, #3840]
	eor3	v4.16b, v4.16b, v5.16b, v26.16b
	ldr	q5, [sp, #3888]
	ldr	q26, [sp, #3904]
	ldr	q28, [sp, #3952]
	ldr	q29, [sp, #3968]
	eor3	v4.16b, v4.16b, v5.16b, v28.16b
	ldp	q5, q28, [sp, #960]
	ldr	q30, [sp, #3008]
	ldr	q31, [sp, #3024]
	eor3	v5.16b, v5.16b, v30.16b, v6.16b
	eor3	v5.16b, v5.16b, v7.16b, v17.16b
	eor3	v5.16b, v5.16b, v16.16b, v19.16b
	eor3	v5.16b, v5.16b, v18.16b, v21.16b
	eor3	v5.16b, v5.16b, v20.16b, v23.16b
	eor3	v5.16b, v5.16b, v22.16b, v25.16b
	eor3	v5.16b, v5.16b, v24.16b, v27.16b
	ldp	q6, q7, [x19]
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	eor3	v5.16b, v5.16b, v26.16b, v29.16b
	ldp	q6, q7, [x19, #32]
	eor	v0.16b, v6.16b, v0.16b
	eor	v1.16b, v7.16b, v1.16b
	stp	q4, q5, [sp, #944]
	ldr	q4, [sp, #3088]
	eor3	v4.16b, v28.16b, v31.16b, v4.16b
	ldr	q5, [sp, #3152]
	ldr	q6, [sp, #3216]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3280]
	ldr	q6, [sp, #3344]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3408]
	ldr	q6, [sp, #3472]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3536]
	ldr	q6, [sp, #3600]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3664]
	ldr	q6, [sp, #3728]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3792]
	ldr	q6, [sp, #3856]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	ldr	q5, [sp, #3920]
	ldr	q6, [sp, #3984]
	eor3	v4.16b, v4.16b, v5.16b, v6.16b
	str	q4, [sp, #976]
	ldr	d4, [sp, #992]
	ldr	d5, [sp, #3040]
	eor	v4.8b, v4.8b, v5.8b
	ldr	d5, [sp, #3104]
	ldr	d6, [sp, #3168]
	eor	v5.8b, v5.8b, v6.8b
	eor	v4.8b, v4.8b, v5.8b
	ldr	d5, [sp, #3232]
	ldr	d6, [sp, #3296]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3360]
	eor	v5.8b, v5.8b, v6.8b
	eor	v4.8b, v4.8b, v5.8b
	ldr	d5, [sp, #3424]
	ldr	d6, [sp, #3488]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3552]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3616]
	eor	v5.8b, v5.8b, v6.8b
	eor	v4.8b, v4.8b, v5.8b
	ldr	d5, [sp, #3680]
	ldr	d6, [sp, #3744]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3808]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3872]
	eor	v5.8b, v5.8b, v6.8b
	ldr	d6, [sp, #3936]
	eor	v5.8b, v5.8b, v6.8b
	eor	v4.8b, v4.8b, v5.8b
	ldr	d5, [sp, #4000]
	eor	v4.8b, v4.8b, v5.8b
	add	x10, sp, #693
	ldur	q5, [x10, #255]
	ldp	q6, q7, [sp, #832] // 32-byte Folded Reload
	ushl	v5.8h, v5.8h, v7.8h
	and	v5.16b, v5.16b, v6.16b
	add	x10, sp, #709
	ldur	d6, [x10, #255]
	ushl	v6.4h, v6.4h, v10.4h
	add	x10, sp, #944
	add	x12, x10, #56
	add	x11, x10, #58
	add	x10, x10, #62
	ldrh	w13, [sp, #1000]
	ldrh	w14, [sp, #3048]
	eor	w13, w13, w14
	ldrh	w14, [sp, #3112]
	ldrh	w15, [sp, #3176]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3240]
	ldrh	w15, [sp, #3304]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3368]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3432]
	ldrh	w15, [sp, #3496]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3560]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3624]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #3688]
	ldrh	w15, [sp, #3752]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3816]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3880]
	eor	w14, w14, w15
	ldrh	w15, [sp, #3944]
	eor	w14, w14, w15
	eor	w13, w13, w14
	ldrh	w14, [sp, #4008]
	eor	w14, w13, w14
	ldrh	w13, [sp, #1002]
	ldrh	w15, [sp, #3050]
	eor	w13, w13, w15
	ldrh	w15, [sp, #3114]
	ldrh	w16, [sp, #3178]
	eor	w15, w15, w16
	eor	w13, w13, w15
	ldrh	w15, [sp, #3242]
	ldrh	w16, [sp, #3306]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3370]
	eor	w15, w15, w16
	eor	w13, w13, w15
	ldrh	w15, [sp, #3434]
	ldrh	w16, [sp, #3498]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3562]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3626]
	eor	w15, w15, w16
	eor	w13, w13, w15
	ldrh	w15, [sp, #3690]
	ldrh	w16, [sp, #3754]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3818]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3882]
	eor	w15, w15, w16
	ldrh	w16, [sp, #3946]
	eor	w15, w15, w16
	eor	w13, w13, w15
	ldrh	w15, [sp, #4010]
	eor	w15, w13, w15
	ldrh	w13, [sp, #1004]
	ldrh	w16, [sp, #3052]
	eor	w13, w13, w16
	ldrh	w16, [sp, #3116]
	ldrh	w17, [sp, #3180]
	eor	w16, w16, w17
	eor	w13, w13, w16
	ldrh	w16, [sp, #3244]
	ldrh	w17, [sp, #3308]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3372]
	eor	w16, w16, w17
	eor	w13, w13, w16
	ldrh	w16, [sp, #3436]
	ldrh	w17, [sp, #3500]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3564]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3628]
	eor	w16, w16, w17
	eor	w13, w13, w16
	ldrh	w16, [sp, #3692]
	ldrh	w17, [sp, #3756]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3820]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3884]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3948]
	eor	w16, w16, w17
	eor	w13, w13, w16
	ldrh	w16, [sp, #4012]
	eor	w13, w13, w16
	ldrh	w16, [sp, #1006]
	ldrh	w17, [sp, #3054]
	eor	w16, w16, w17
	ldrh	w17, [sp, #3118]
	ldrh	w0, [sp, #3182]
	eor	w17, w17, w0
	eor	w16, w16, w17
	ldrh	w17, [sp, #3246]
	ldrh	w0, [sp, #3310]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3374]
	eor	w17, w17, w0
	eor	w16, w16, w17
	ldrh	w17, [sp, #3438]
	ldrh	w0, [sp, #3502]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3566]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3630]
	eor	w17, w17, w0
	eor	w16, w16, w17
	ldrh	w17, [sp, #3694]
	ldrh	w0, [sp, #3758]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3822]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3886]
	eor	w17, w17, w0
	ldrh	w0, [sp, #3950]
	eor	w17, w17, w0
	eor	w16, w16, w17
	add	x17, sp, #2992
	add	x17, x17, #1022
	ldrh	w0, [sp, #944]
	and	w0, w0, #0x1
	ldrh	w1, [sp, #946]
	ubfiz	w1, w1, #1, #1
	str	d4, [sp, #992]
	and	v4.8b, v6.8b, v9.8b
	fmov	x2, d4
	ext	v4.16b, v5.16b, v5.16b, #8
	orr	v4.8b, v5.8b, v4.8b
	orr	x2, x2, x2, lsr #32
	lsr	x3, x2, #16
	orr	w2, w2, w3
	fmov	x3, d4
	strh	w14, [sp, #1000]
	strh	w15, [sp, #1002]
	ldrh	w14, [sp, #972]
	ldrh	w15, [sp, #974]
	orr	x3, x3, x3, lsr #32
	orr	w2, w3, w2
	lsr	x3, x3, #16
	orr	w2, w2, w3
	bfi	w0, w14, #14, #1
	orr	w14, w1, w0
	orr	w14, w2, w14
	orr	w14, w14, w15, lsl #15
	ldr	q4, [sp, #976]
	movi	v5.8h, #1
	movi	v6.8h, #1
	ldr	d7, [sp, #992]
	ld1	{v7.h}[4], [x12]
	ld1	{v7.h}[5], [x11]
	mov	v7.h[6], w13
	and	v4.16b, v4.16b, v5.16b
	and	v5.16b, v7.16b, v5.16b
	mov	v6.h[7], w16
	ld1	{v7.h}[7], [x17]
	eor	v6.16b, v6.16b, v7.16b
	mov	v5.h[7], v6.h[7]
	ushll2	v7.4s, v5.8h, #0
	stp	q2, q3, [x19]
	ushll	v2.4s, v5.4h, #0
	ushll2	v3.4s, v4.8h, #0
	ushll	v4.4s, v4.4h, #0
Lloh24:
	adrp	x11, lCPI0_12
Lloh25:
	ldr	q5, [x11, :lo12:lCPI0_12]
	ushl	v2.4s, v2.4s, v5.4s
Lloh26:
	adrp	x11, lCPI0_13
Lloh27:
	ldr	q5, [x11, :lo12:lCPI0_13]
	ushl	v5.4s, v7.4s, v5.4s
Lloh28:
	adrp	x11, lCPI0_14
Lloh29:
	ldr	q7, [x11, :lo12:lCPI0_14]
	ushl	v4.4s, v4.4s, v7.4s
Lloh30:
	adrp	x11, lCPI0_15
Lloh31:
	ldr	q7, [x11, :lo12:lCPI0_15]
	ushl	v3.4s, v3.4s, v7.4s
	orr	v3.16b, v3.16b, v5.16b
	orr	v2.16b, v4.16b, v2.16b
	orr	v2.16b, v2.16b, v3.16b
	ext	v3.16b, v2.16b, v2.16b, #8
	orr	v2.8b, v2.8b, v3.8b
	stp	q0, q1, [x19, #32]
	fmov	x11, d2
	lsr	x12, x11, #32
	strh	w13, [sp, #1004]
	and	w13, w14, #0xffff
	orr	w11, w11, w13
	orr	w11, w11, w12
	st1	{v6.h}[7], [x10]
LBB0_15: // =>This Inner Loop Header: Depth=1
	mov	w10, w11
	ldrh	w12, [x19, x9]
	and	x12, x12, #0x1
	neg	x12, x12
	and	x10, x10, x12
	eor	x8, x10, x8
	ubfx	x10, x11, #31, #1
	bfi	x10, x11, #1, #32
	add	x9, x9, #2
	mov	x11, x10
	cmp	x9, #64
	b.ne	LBB0_15
// %bb.16:
	and	w9, w8, #0x1
	strh	w9, [sp, #1968]
	dup	v0.8h, w8
	ldr	q1, [sp, #736] // 16-byte Folded Reload
	ushl	v1.8h, v0.8h, v1.8h
	movi	v2.8h, #1
	and	v1.16b, v1.16b, v2.16b
	stur	q1, [x24, #2]
	ushl	v0.4h, v0.4h, v8.4h
	bic	v0.4h, #126
	stur	d0, [x24, #18]
	ubfx	w9, w8, #13, #1
	strh	w9, [sp, #1994]
	ubfx	w9, w8, #14, #1
	strh	w9, [sp, #1996]
	ubfx	w9, w8, #15, #1
	strh	w9, [sp, #1998]
	dup	v0.2d, x8
Lloh32:
	adrp	x8, lCPI0_16
Lloh33:
	ldr	q1, [x8, :lo12:lCPI0_16]
	str	q1, [sp, #848] // 16-byte Folded Spill
	ushl	v1.2d, v0.2d, v1.2d
Lloh34:
	adrp	x8, lCPI0_17
Lloh35:
	ldr	q3, [x8, :lo12:lCPI0_17]
	str	q3, [sp, #816] // 16-byte Folded Spill
	ushl	v3.2d, v0.2d, v3.2d
Lloh36:
	adrp	x8, lCPI0_18
Lloh37:
	ldr	q4, [x8, :lo12:lCPI0_18]
	str	q4, [sp, #768] // 16-byte Folded Spill
	ushl	v4.2d, v0.2d, v4.2d
Lloh38:
	adrp	x8, lCPI0_19
Lloh39:
	ldr	q5, [x8, :lo12:lCPI0_19]
	str	q5, [sp, #752] // 16-byte Folded Spill
	ushl	v5.2d, v0.2d, v5.2d
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v1.4s, v3.4s, v1.4s
	uzp1	v1.8h, v1.8h, v4.8h
	and	v1.16b, v1.16b, v2.16b
Lloh40:
	adrp	x8, lCPI0_20
Lloh41:
	ldr	q3, [x8, :lo12:lCPI0_20]
	str	q3, [sp, #864] // 16-byte Folded Spill
	ushl	v3.2d, v0.2d, v3.2d
Lloh42:
	adrp	x8, lCPI0_22
Lloh43:
	ldr	q4, [x8, :lo12:lCPI0_22]
	str	q4, [sp, #832] // 16-byte Folded Spill
	ushl	v4.2d, v0.2d, v4.2d
Lloh44:
	adrp	x8, lCPI0_23
Lloh45:
	ldr	q5, [x8, :lo12:lCPI0_23]
	str	q5, [sp, #800] // 16-byte Folded Spill
	ushl	v5.2d, v0.2d, v5.2d
	uzp1	v4.4s, v5.4s, v4.4s
Lloh46:
	adrp	x8, lCPI0_21
Lloh47:
	ldr	q5, [x8, :lo12:lCPI0_21]
	str	q5, [sp, #784] // 16-byte Folded Spill
	ushl	v0.2d, v0.2d, v5.2d
	uzp1	v0.4s, v0.4s, v3.4s
	uzp1	v0.8h, v0.8h, v4.8h
	and	v0.16b, v0.16b, v2.16b
	str	q1, [sp, #2000]
	str	q0, [sp, #2016]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	w8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	add	x9, sp, #2992
	add	x9, x9, #64
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
LBB0_17: // =>This Inner Loop Header: Depth=1
	ldp	q0, q1, [x9]
	ldp	q2, q3, [x9, #-64]
	eor	v0.16b, v2.16b, v0.16b
	eor	v1.16b, v3.16b, v1.16b
	stp	q0, q1, [x9]
	ldp	q0, q1, [x9, #32]
	ldp	q2, q3, [x9, #-32]
	eor	v0.16b, v2.16b, v0.16b
	eor	v1.16b, v3.16b, v1.16b
	stp	q0, q1, [x9, #32]
	add	x9, x9, #64
	add	w8, w8, #32
	cmp	w8, #480
	b.lo	LBB0_17
// %bb.18:
	mov	x8, #0 // =0x0
	ldr	q0, [sp, #1968]
	ldr	q1, [sp, #1968]
	ldr	q2, [sp, #1984]
	ldr	q3, [sp, #1984]
	stp	q0, q2, [sp, #944]
	ldr	q0, [sp, #2000]
	ldr	q2, [sp, #2000]
	ldr	q4, [sp, #2016]
	ldr	q5, [sp, #2016]
	stp	q0, q4, [sp, #976]
	stp	q1, q3, [sp, #1008]
	str	q2, [sp, #1040]
	str	q5, [sp, #1056]
	ldp	q0, q4, [sp, #944]
	ldp	q6, q7, [sp, #976]
	ldp	q16, q17, [x19]
	ldp	q18, q19, [x19, #32]
	eor	v0.16b, v16.16b, v0.16b
	eor	v4.16b, v17.16b, v4.16b
	eor	v6.16b, v18.16b, v6.16b
	eor	v7.16b, v19.16b, v7.16b
	stp	q0, q4, [x19]
	stp	q6, q7, [x19, #32]
	ldp	q0, q4, [x19, #64]
	ldp	q6, q7, [x19, #96]
	eor	v0.16b, v0.16b, v1.16b
	eor	v1.16b, v4.16b, v3.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v5.16b
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	movi	v0.2d, #0000000000000000
	str	q0, [sp, #1056]
	str	q0, [sp, #1040]
	stp	q0, q0, [sp, #1008]
	stp	q0, q0, [sp, #976]
	add	x11, sp, #2992
	add	x9, sp, #944
	mov	x10, x11
	stp	q0, q0, [sp, #944]
	b	LBB0_20
LBB0_19: // in Loop: Header=BB0_20 Depth=1
	strh	w12, [x9, x8, lsl #1]
	add	x8, x8, #1
	mov	x11, x10
	cmp	x8, #64
	b.eq	LBB0_28
LBB0_20: // =>This Inner Loop Header: Depth=1
	ldrh	w12, [x9, x8, lsl #1]
	ldrh	w13, [x10], #2
	eor	w12, w12, w13
	cmp	x8, #447
	b.hi	LBB0_19
// %bb.21:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #128]
	eor	w12, w12, w13
	cmp	x8, #383
	b.hi	LBB0_19
// %bb.22:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #256]
	eor	w12, w12, w13
	cmp	x8, #319
	b.hi	LBB0_19
// %bb.23:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #384]
	eor	w12, w12, w13
	cmp	x8, #255
	b.hi	LBB0_19
// %bb.24:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #512]
	eor	w12, w12, w13
	cmp	x8, #191
	b.hi	LBB0_19
// %bb.25:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #640]
	eor	w12, w12, w13
	cmp	x8, #127
	b.hi	LBB0_19
// %bb.26:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w13, [x11, #768]
	eor	w12, w12, w13
	cmp	x8, #63
	b.hi	LBB0_19
// %bb.27:                               ;   in Loop: Header=BB0_20 Depth=1
	ldrh	w11, [x11, #896]
	eor	w12, w12, w11
	b	LBB0_19
LBB0_28:
	mov	x9, #0 // =0x0
	mov	x8, #0 // =0x0
	movi	v2.8h, #1
	ldp	q0, q1, [sp, #944]
	and	v0.16b, v0.16b, v2.16b
	and	v1.16b, v1.16b, v2.16b
	ushll	v3.4s, v0.4h, #0
	ushll2	v4.2d, v3.4s, #0
	ushll2	v0.4s, v0.8h, #0
	ushll2	v5.2d, v0.4s, #0
	ushll	v3.2d, v3.2s, #0
	ushll	v0.2d, v0.2s, #0
	ushll	v6.4s, v1.4h, #0
	ushll2	v7.2d, v6.4s, #0
	ushll2	v1.4s, v1.8h, #0
	ushll2	v16.2d, v1.4s, #0
	ushll	v6.2d, v6.2s, #0
	ushll	v17.2d, v1.2s, #0
Lloh48:
	adrp	x10, lCPI0_24
Lloh49:
	ldr	q1, [x10, :lo12:lCPI0_24]
	str	q1, [sp, #64] // 16-byte Folded Spill
	ushl	v0.2d, v0.2d, v1.2d
Lloh50:
	adrp	x10, lCPI0_25
Lloh51:
	ldr	q1, [x10, :lo12:lCPI0_25]
	str	q1, [sp, #80] // 16-byte Folded Spill
	ushl	v1.2d, v3.2d, v1.2d
Lloh52:
	adrp	x10, lCPI0_26
Lloh53:
	ldr	q3, [x10, :lo12:lCPI0_26]
	str	q3, [sp, #96] // 16-byte Folded Spill
	ushl	v3.2d, v5.2d, v3.2d
Lloh54:
	adrp	x10, lCPI0_27
Lloh55:
	ldr	q5, [x10, :lo12:lCPI0_27]
	str	q5, [sp, #112] // 16-byte Folded Spill
	ushl	v4.2d, v4.2d, v5.2d
Lloh56:
	adrp	x10, lCPI0_28
Lloh57:
	ldr	q5, [x10, :lo12:lCPI0_28]
	str	q5, [sp, #160] // 16-byte Folded Spill
	ushl	v5.2d, v17.2d, v5.2d
Lloh58:
	adrp	x10, lCPI0_29
Lloh59:
	ldr	q17, [x10, :lo12:lCPI0_29]
	str	q17, [sp, #176] // 16-byte Folded Spill
	ushl	v18.2d, v6.2d, v17.2d
Lloh60:
	adrp	x10, lCPI0_30
Lloh61:
	ldr	q6, [x10, :lo12:lCPI0_30]
	str	q6, [sp, #192] // 16-byte Folded Spill
	ushl	v19.2d, v16.2d, v6.2d
Lloh62:
	adrp	x10, lCPI0_31
Lloh63:
	ldr	q6, [x10, :lo12:lCPI0_31]
	str	q6, [sp, #224] // 16-byte Folded Spill
	ushl	v20.2d, v7.2d, v6.2d
	ldp	q6, q7, [sp, #976]
	and	v6.16b, v6.16b, v2.16b
	and	v7.16b, v7.16b, v2.16b
	ushll	v16.4s, v6.4h, #0
	ushll2	v17.2d, v16.4s, #0
	ushll2	v6.4s, v6.8h, #0
	ushll2	v21.2d, v6.4s, #0
	ushll	v16.2d, v16.2s, #0
	ushll	v6.2d, v6.2s, #0
	ushll	v22.4s, v7.4h, #0
	ushll2	v23.2d, v22.4s, #0
	ushll2	v7.4s, v7.8h, #0
	ushll2	v24.2d, v7.4s, #0
	ushll	v22.2d, v22.2s, #0
	ushll	v7.2d, v7.2s, #0
Lloh64:
	adrp	x10, lCPI0_32
Lloh65:
	ldr	q25, [x10, :lo12:lCPI0_32]
	str	q25, [sp, #368] // 16-byte Folded Spill
	ushl	v6.2d, v6.2d, v25.2d
Lloh66:
	adrp	x10, lCPI0_33
Lloh67:
	ldr	q25, [x10, :lo12:lCPI0_33]
	str	q25, [sp, #352] // 16-byte Folded Spill
	ushl	v25.2d, v16.2d, v25.2d
Lloh68:
	adrp	x10, lCPI0_34
Lloh69:
	ldr	q16, [x10, :lo12:lCPI0_34]
	str	q16, [sp, #336] // 16-byte Folded Spill
	ushl	v16.2d, v21.2d, v16.2d
Lloh70:
	adrp	x10, lCPI0_35
Lloh71:
	ldr	q21, [x10, :lo12:lCPI0_35]
	str	q21, [sp, #288] // 16-byte Folded Spill
	ushl	v17.2d, v17.2d, v21.2d
Lloh72:
	adrp	x10, lCPI0_36
Lloh73:
	ldr	q21, [x10, :lo12:lCPI0_36]
	str	q21, [sp, #320] // 16-byte Folded Spill
	ushl	v21.2d, v7.2d, v21.2d
Lloh74:
	adrp	x10, lCPI0_37
Lloh75:
	ldr	q7, [x10, :lo12:lCPI0_37]
	str	q7, [sp, #304] // 16-byte Folded Spill
	ushl	v22.2d, v22.2d, v7.2d
Lloh76:
	adrp	x10, lCPI0_38
Lloh77:
	ldr	q7, [x10, :lo12:lCPI0_38]
	str	q7, [sp, #272] // 16-byte Folded Spill
	ushl	v24.2d, v24.2d, v7.2d
	orr	v17.16b, v17.16b, v4.16b
Lloh78:
	adrp	x10, lCPI0_39
Lloh79:
	ldr	q4, [x10, :lo12:lCPI0_39]
	str	q4, [sp, #256] // 16-byte Folded Spill
	ushl	v4.2d, v23.2d, v4.2d
	orr	v16.16b, v16.16b, v3.16b
	orr	v7.16b, v25.16b, v1.16b
	orr	v6.16b, v6.16b, v0.16b
	orr	v4.16b, v4.16b, v20.16b
	orr	v3.16b, v24.16b, v19.16b
	orr	v1.16b, v22.16b, v18.16b
	orr	v0.16b, v21.16b, v5.16b
	ldp	q5, q18, [sp, #1008]
	and	v19.16b, v5.16b, v2.16b
	and	v5.16b, v18.16b, v2.16b
	ushll	v18.4s, v19.4h, #0
	ushll2	v20.2d, v18.4s, #0
Lloh80:
	adrp	x10, lCPI0_40
Lloh81:
	ldr	q21, [x10, :lo12:lCPI0_40]
	str	q21, [sp, #400] // 16-byte Folded Spill
	ushl	v20.2d, v20.2d, v21.2d
	ldr	q21, [sp, #1040]
	ldr	q22, [sp, #1056]
	and	v21.16b, v21.16b, v2.16b
	and	v2.16b, v22.16b, v2.16b
	ushll	v22.4s, v21.4h, #0
Lloh82:
	adrp	x10, lCPI0_51
	ushll2	v23.2d, v22.4s, #0
Lloh83:
	ldr	q24, [x10, :lo12:lCPI0_51]
	str	q24, [sp, #512] // 16-byte Folded Spill
	ushl	v23.2d, v23.2d, v24.2d
	orr	v20.16b, v23.16b, v20.16b
	orr	v17.16b, v20.16b, v17.16b
	ushll2	v19.4s, v19.8h, #0
Lloh84:
	adrp	x10, lCPI0_41
	ushll2	v20.2d, v19.4s, #0
Lloh85:
	ldr	q23, [x10, :lo12:lCPI0_41]
	str	q23, [sp, #240] // 16-byte Folded Spill
	ushl	v20.2d, v20.2d, v23.2d
	ushll2	v21.4s, v21.8h, #0
Lloh86:
	adrp	x10, lCPI0_50
	ushll2	v23.2d, v21.4s, #0
Lloh87:
	ldr	q24, [x10, :lo12:lCPI0_50]
	str	q24, [sp, #496] // 16-byte Folded Spill
	ushl	v23.2d, v23.2d, v24.2d
	orr	v20.16b, v23.16b, v20.16b
	orr	v16.16b, v20.16b, v16.16b
	ushll	v18.2d, v18.2s, #0
Lloh88:
	adrp	x10, lCPI0_42
Lloh89:
	ldr	q20, [x10, :lo12:lCPI0_42]
	str	q20, [sp, #208] // 16-byte Folded Spill
	ushl	v18.2d, v18.2d, v20.2d
	ushll	v20.2d, v22.2s, #0
Lloh90:
	adrp	x10, lCPI0_49
Lloh91:
	ldr	q22, [x10, :lo12:lCPI0_49]
	str	q22, [sp, #480] // 16-byte Folded Spill
	ushl	v20.2d, v20.2d, v22.2d
	orr	v18.16b, v20.16b, v18.16b
	orr	v7.16b, v18.16b, v7.16b
	ushll	v18.2d, v19.2s, #0
Lloh92:
	adrp	x10, lCPI0_43
Lloh93:
	ldr	q19, [x10, :lo12:lCPI0_43]
	str	q19, [sp, #144] // 16-byte Folded Spill
	ushl	v18.2d, v18.2d, v19.2d
	ushll	v19.2d, v21.2s, #0
Lloh94:
	adrp	x10, lCPI0_48
Lloh95:
	ldr	q20, [x10, :lo12:lCPI0_48]
	str	q20, [sp, #448] // 16-byte Folded Spill
	ushl	v19.2d, v19.2d, v20.2d
	orr	v18.16b, v19.16b, v18.16b
	orr	v6.16b, v18.16b, v6.16b
Lloh96:
	adrp	x10, lCPI0_44
	ushll	v18.4s, v5.4h, #0
	ushll2	v19.2d, v18.4s, #0
Lloh97:
	ldr	q20, [x10, :lo12:lCPI0_44]
	str	q20, [sp, #128] // 16-byte Folded Spill
	ushl	v19.2d, v19.2d, v20.2d
Lloh98:
	adrp	x10, lCPI0_55
	ushll	v20.4s, v2.4h, #0
	ushll2	v21.2d, v20.4s, #0
Lloh99:
	ldr	q22, [x10, :lo12:lCPI0_55]
	str	q22, [sp, #464] // 16-byte Folded Spill
	ushl	v21.2d, v21.2d, v22.2d
	orr	v19.16b, v21.16b, v19.16b
	orr	v4.16b, v19.16b, v4.16b
	ushll2	v5.4s, v5.8h, #0
Lloh100:
	adrp	x10, lCPI0_45
	ushll2	v19.2d, v5.4s, #0
Lloh101:
	ldr	q21, [x10, :lo12:lCPI0_45]
	str	q21, [sp, #48] // 16-byte Folded Spill
	ushl	v19.2d, v19.2d, v21.2d
	ushll2	v2.4s, v2.8h, #0
Lloh102:
	adrp	x10, lCPI0_54
	ushll2	v21.2d, v2.4s, #0
Lloh103:
	ldr	q22, [x10, :lo12:lCPI0_54]
	str	q22, [sp, #432] // 16-byte Folded Spill
	ushl	v21.2d, v21.2d, v22.2d
	orr	v19.16b, v21.16b, v19.16b
	orr	v3.16b, v19.16b, v3.16b
	ushll	v18.2d, v18.2s, #0
Lloh104:
	adrp	x10, lCPI0_46
Lloh105:
	ldr	q19, [x10, :lo12:lCPI0_46]
	str	q19, [sp, #32] // 16-byte Folded Spill
	ushl	v18.2d, v18.2d, v19.2d
	ushll	v19.2d, v20.2s, #0
Lloh106:
	adrp	x10, lCPI0_53
Lloh107:
	ldr	q20, [x10, :lo12:lCPI0_53]
	str	q20, [sp, #416] // 16-byte Folded Spill
	ushl	v19.2d, v19.2d, v20.2d
	orr	v18.16b, v19.16b, v18.16b
	orr	v1.16b, v18.16b, v1.16b
	ushll	v5.2d, v5.2s, #0
Lloh108:
	adrp	x10, lCPI0_47
Lloh109:
	ldr	q18, [x10, :lo12:lCPI0_47]
	str	q18, [sp, #16] // 16-byte Folded Spill
	ushl	v5.2d, v5.2d, v18.2d
	ushll	v2.2d, v2.2s, #0
Lloh110:
	adrp	x10, lCPI0_52
Lloh111:
	ldr	q18, [x10, :lo12:lCPI0_52]
	str	q18, [sp, #384] // 16-byte Folded Spill
	ushl	v2.2d, v2.2d, v18.2d
	orr	v2.16b, v2.16b, v5.16b
	orr	v0.16b, v2.16b, v0.16b
	orr	v0.16b, v0.16b, v6.16b
	orr	v1.16b, v1.16b, v7.16b
	orr	v2.16b, v3.16b, v16.16b
	orr	v3.16b, v4.16b, v17.16b
	orr	v2.16b, v3.16b, v2.16b
	orr	v0.16b, v1.16b, v0.16b
	orr	v0.16b, v0.16b, v2.16b
	ext	v1.16b, v0.16b, v0.16b, #8
	orr	v0.8b, v0.8b, v1.8b
	fmov	x10, d0
LBB0_29: // =>This Inner Loop Header: Depth=1
	ldrh	w11, [x19, x9]
	and	x11, x11, #0x1
	neg	x11, x11
	and	x11, x10, x11
	eor	x8, x11, x8
	ror	x10, x10, #63
	add	x9, x9, #2
	cmp	x9, #128
	b.ne	LBB0_29
// %bb.30:
	and	w9, w8, #0x1
	strh	w9, [sp, #1968]
	dup	v1.8h, w8
	ldr	q0, [sp, #736] // 16-byte Folded Reload
	ushl	v2.8h, v1.8h, v0.8h
	movi	v0.8h, #1
	and	v2.16b, v2.16b, v0.16b
	stur	q2, [x24, #2]
	ushl	v1.4h, v1.4h, v8.4h
	bic	v1.4h, #126
	stur	d1, [x24, #18]
	ubfx	w9, w8, #13, #1
	strh	w9, [sp, #1994]
	ubfx	w9, w8, #14, #1
	dup	v1.2d, x8
	strh	w9, [sp, #1996]
	ubfx	w9, w8, #15, #1
	strh	w9, [sp, #1998]
	ldr	q2, [sp, #848] // 16-byte Folded Reload
	ushl	v2.2d, v1.2d, v2.2d
	ldp	q6, q3, [sp, #800] // 32-byte Folded Reload
	ushl	v3.2d, v1.2d, v3.2d
	ldp	q5, q4, [sp, #752] // 32-byte Folded Reload
	ushl	v4.2d, v1.2d, v4.2d
	ushl	v5.2d, v1.2d, v5.2d
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v2.4s, v3.4s, v2.4s
	uzp1	v2.8h, v2.8h, v4.8h
	and	v2.16b, v2.16b, v0.16b
	ldr	q3, [sp, #864] // 16-byte Folded Reload
	ushl	v3.2d, v1.2d, v3.2d
	ldr	q4, [sp, #784] // 16-byte Folded Reload
	ushl	v4.2d, v1.2d, v4.2d
	ldr	q5, [sp, #832] // 16-byte Folded Reload
	ushl	v5.2d, v1.2d, v5.2d
	ushl	v6.2d, v1.2d, v6.2d
	uzp1	v5.4s, v6.4s, v5.4s
	uzp1	v3.4s, v4.4s, v3.4s
	uzp1	v3.8h, v3.8h, v5.8h
	and	v3.16b, v3.16b, v0.16b
	str	q2, [sp, #2000]
	str	q3, [sp, #2016]
Lloh112:
	adrp	x9, lCPI0_56
Lloh113:
	ldr	q2, [x9, :lo12:lCPI0_56]
	str	q2, [sp, #608] // 16-byte Folded Spill
	ushl	v2.2d, v1.2d, v2.2d
Lloh114:
	adrp	x9, lCPI0_57
Lloh115:
	ldr	q3, [x9, :lo12:lCPI0_57]
	str	q3, [sp, #576] // 16-byte Folded Spill
	ushl	v3.2d, v1.2d, v3.2d
Lloh116:
	adrp	x9, lCPI0_58
Lloh117:
	ldr	q4, [x9, :lo12:lCPI0_58]
	str	q4, [sp, #544] // 16-byte Folded Spill
	ushl	v4.2d, v1.2d, v4.2d
Lloh118:
	adrp	x9, lCPI0_59
Lloh119:
	ldr	q5, [x9, :lo12:lCPI0_59]
	str	q5, [sp, #528] // 16-byte Folded Spill
	ushl	v5.2d, v1.2d, v5.2d
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v2.4s, v3.4s, v2.4s
	uzp1	v2.8h, v2.8h, v4.8h
	and	v2.16b, v2.16b, v0.16b
Lloh120:
	adrp	x9, lCPI0_60
Lloh121:
	ldr	q3, [x9, :lo12:lCPI0_60]
	str	q3, [sp, #704] // 16-byte Folded Spill
	ushl	v3.2d, v1.2d, v3.2d
Lloh122:
	adrp	x9, lCPI0_61
Lloh123:
	ldr	q4, [x9, :lo12:lCPI0_61]
	str	q4, [sp, #672] // 16-byte Folded Spill
	ushl	v4.2d, v1.2d, v4.2d
Lloh124:
	adrp	x9, lCPI0_62
Lloh125:
	ldr	q5, [x9, :lo12:lCPI0_62]
	str	q5, [sp, #592] // 16-byte Folded Spill
	ushl	v5.2d, v1.2d, v5.2d
Lloh126:
	adrp	x9, lCPI0_63
Lloh127:
	ldr	q6, [x9, :lo12:lCPI0_63]
	str	q6, [sp, #560] // 16-byte Folded Spill
	ushl	v6.2d, v1.2d, v6.2d
	uzp1	v5.4s, v6.4s, v5.4s
	uzp1	v3.4s, v4.4s, v3.4s
	uzp1	v3.8h, v3.8h, v5.8h
	and	v3.16b, v3.16b, v0.16b
	str	q2, [sp, #2032]
	str	q3, [sp, #2048]
Lloh128:
	adrp	x9, lCPI0_64
Lloh129:
	ldr	q2, [x9, :lo12:lCPI0_64]
	str	q2, [sp, #688] // 16-byte Folded Spill
	ushl	v2.2d, v1.2d, v2.2d
Lloh130:
	adrp	x9, lCPI0_65
Lloh131:
	ldr	q3, [x9, :lo12:lCPI0_65]
	str	q3, [sp, #656] // 16-byte Folded Spill
	ushl	v3.2d, v1.2d, v3.2d
Lloh132:
	adrp	x9, lCPI0_66
Lloh133:
	ldr	q4, [x9, :lo12:lCPI0_66]
	str	q4, [sp, #640] // 16-byte Folded Spill
	ushl	v4.2d, v1.2d, v4.2d
Lloh134:
	adrp	x9, lCPI0_67
Lloh135:
	ldr	q5, [x9, :lo12:lCPI0_67]
	str	q5, [sp, #624] // 16-byte Folded Spill
	ushl	v5.2d, v1.2d, v5.2d
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v2.4s, v3.4s, v2.4s
	uzp1	v2.8h, v2.8h, v4.8h
	and	v0.16b, v2.16b, v0.16b
	str	q0, [sp, #2064]
Lloh136:
	adrp	x9, lCPI0_68
Lloh137:
	ldr	q0, [x9, :lo12:lCPI0_68]
	str	q0, [sp, #736] // 16-byte Folded Spill
	ushl	v0.2d, v1.2d, v0.2d
Lloh138:
	adrp	x9, lCPI0_69
Lloh139:
	ldr	q2, [x9, :lo12:lCPI0_69]
	str	q2, [sp, #720] // 16-byte Folded Spill
	ushl	v1.2d, v1.2d, v2.2d
	uzp1	v0.4s, v1.4s, v0.4s
	xtn	v0.4h, v0.4s
	bic	v0.4h, #254
	str	d0, [sp, #2080]
	ubfx	x9, x8, #60, #1
	strh	w9, [sp, #2088]
	ubfx	x9, x8, #61, #1
	strh	w9, [sp, #2090]
	ubfx	x9, x8, #62, #1
	strh	w9, [sp, #2092]
	lsr	x8, x8, #63
	strh	w8, [sp, #2094]
	add	x20, sp, #944
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	x8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #2992]
	ldr	q5, [sp, #3008]
	ldr	q6, [sp, #3024]
	ldr	q7, [sp, #3040]
	eor	v16.16b, v4.16b, v0.16b
	eor	v17.16b, v5.16b, v1.16b
	eor	v18.16b, v6.16b, v2.16b
	eor	v19.16b, v7.16b, v3.16b
	str	q16, [sp, #3120]
	str	q17, [sp, #3136]
	str	q18, [sp, #3152]
	str	q19, [sp, #3168]
	ldr	q20, [sp, #3184]
	ldr	q21, [sp, #3200]
	ldr	q22, [sp, #3216]
	ldr	q23, [sp, #3232]
	ldr	q24, [sp, #3056]
	ldr	q25, [sp, #3072]
	ldr	q26, [sp, #3088]
	ldr	q27, [sp, #3104]
	eor	v28.16b, v24.16b, v20.16b
	eor	v29.16b, v25.16b, v21.16b
	eor	v30.16b, v26.16b, v22.16b
	eor	v31.16b, v27.16b, v23.16b
	str	q28, [sp, #3184]
	str	q29, [sp, #3200]
	str	q30, [sp, #3216]
	str	q31, [sp, #3232]
	ldr	q8, [sp, #3248]
	ldr	q9, [sp, #3264]
	ldr	q10, [sp, #3280]
	ldr	q11, [sp, #3296]
	eor3	v0.16b, v4.16b, v0.16b, v8.16b
	eor3	v1.16b, v5.16b, v1.16b, v9.16b
	eor3	v2.16b, v6.16b, v2.16b, v10.16b
	eor3	v3.16b, v7.16b, v3.16b, v11.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q4, [sp, #3312]
	ldr	q5, [sp, #3328]
	ldr	q6, [sp, #3344]
	ldr	q7, [sp, #3360]
	eor3	v20.16b, v24.16b, v20.16b, v4.16b
	eor3	v21.16b, v25.16b, v21.16b, v5.16b
	eor3	v22.16b, v26.16b, v22.16b, v6.16b
	eor3	v23.16b, v27.16b, v23.16b, v7.16b
	str	q20, [sp, #3312]
	str	q21, [sp, #3328]
	str	q22, [sp, #3344]
	str	q23, [sp, #3360]
	ldr	q24, [sp, #3376]
	ldr	q25, [sp, #3392]
	ldr	q26, [sp, #3408]
	ldr	q27, [sp, #3424]
	eor3	v16.16b, v16.16b, v8.16b, v24.16b
	eor3	v17.16b, v17.16b, v9.16b, v25.16b
	eor3	v18.16b, v18.16b, v10.16b, v26.16b
	eor3	v19.16b, v19.16b, v11.16b, v27.16b
	str	q16, [sp, #3376]
	str	q17, [sp, #3392]
	str	q18, [sp, #3408]
	str	q19, [sp, #3424]
	ldr	q8, [sp, #3440]
	ldr	q9, [sp, #3456]
	ldr	q10, [sp, #3472]
	ldr	q11, [sp, #3488]
	eor3	v4.16b, v28.16b, v4.16b, v8.16b
	eor3	v5.16b, v29.16b, v5.16b, v9.16b
	eor3	v6.16b, v30.16b, v6.16b, v10.16b
	eor3	v7.16b, v31.16b, v7.16b, v11.16b
	str	q4, [sp, #3440]
	str	q5, [sp, #3456]
	str	q6, [sp, #3472]
	str	q7, [sp, #3488]
	ldr	q28, [sp, #3504]
	ldr	q29, [sp, #3520]
	ldr	q30, [sp, #3536]
	ldr	q31, [sp, #3552]
	eor3	v0.16b, v0.16b, v24.16b, v28.16b
	eor3	v1.16b, v1.16b, v25.16b, v29.16b
	eor3	v2.16b, v2.16b, v26.16b, v30.16b
	eor3	v3.16b, v3.16b, v27.16b, v31.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q24, [sp, #3568]
	ldr	q25, [sp, #3584]
	ldr	q26, [sp, #3600]
	ldr	q27, [sp, #3616]
	eor3	v20.16b, v20.16b, v8.16b, v24.16b
	eor3	v21.16b, v21.16b, v9.16b, v25.16b
	eor3	v22.16b, v22.16b, v10.16b, v26.16b
	eor3	v23.16b, v23.16b, v11.16b, v27.16b
	str	q20, [sp, #3568]
	str	q21, [sp, #3584]
	str	q22, [sp, #3600]
	str	q23, [sp, #3616]
	ldr	q8, [sp, #3632]
	ldr	q9, [sp, #3648]
	ldr	q10, [sp, #3664]
	ldr	q11, [sp, #3680]
	eor3	v16.16b, v16.16b, v28.16b, v8.16b
	eor3	v17.16b, v17.16b, v29.16b, v9.16b
	eor3	v18.16b, v18.16b, v30.16b, v10.16b
	eor3	v19.16b, v19.16b, v31.16b, v11.16b
	str	q16, [sp, #3632]
	str	q17, [sp, #3648]
	str	q18, [sp, #3664]
	str	q19, [sp, #3680]
	ldr	q28, [sp, #3696]
	ldr	q29, [sp, #3712]
	ldr	q30, [sp, #3728]
	ldr	q31, [sp, #3744]
	eor3	v4.16b, v4.16b, v24.16b, v28.16b
	eor3	v5.16b, v5.16b, v25.16b, v29.16b
	eor3	v6.16b, v6.16b, v26.16b, v30.16b
	eor3	v7.16b, v7.16b, v27.16b, v31.16b
	str	q4, [sp, #3696]
	str	q5, [sp, #3712]
	str	q6, [sp, #3728]
	str	q7, [sp, #3744]
	ldr	q24, [sp, #3760]
	ldr	q25, [sp, #3776]
	ldr	q26, [sp, #3792]
	ldr	q27, [sp, #3808]
	eor3	v0.16b, v0.16b, v8.16b, v24.16b
	eor3	v1.16b, v1.16b, v9.16b, v25.16b
	eor3	v2.16b, v2.16b, v10.16b, v26.16b
	eor3	v3.16b, v3.16b, v11.16b, v27.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	eor3	v20.16b, v20.16b, v28.16b, v0.16b
	eor3	v21.16b, v21.16b, v29.16b, v1.16b
	eor3	v22.16b, v22.16b, v30.16b, v2.16b
	eor3	v23.16b, v23.16b, v31.16b, v3.16b
	str	q20, [sp, #3824]
	str	q21, [sp, #3840]
	str	q22, [sp, #3856]
	str	q23, [sp, #3872]
	ldr	q20, [sp, #3888]
	ldr	q21, [sp, #3904]
	ldr	q22, [sp, #3920]
	ldr	q23, [sp, #3936]
	eor3	v16.16b, v16.16b, v24.16b, v20.16b
	eor3	v17.16b, v17.16b, v25.16b, v21.16b
	eor3	v18.16b, v18.16b, v26.16b, v22.16b
	eor3	v19.16b, v19.16b, v27.16b, v23.16b
	str	q16, [sp, #3888]
	str	q17, [sp, #3904]
	str	q18, [sp, #3920]
	str	q19, [sp, #3936]
	ldr	q16, [sp, #3952]
	ldr	q17, [sp, #3968]
	ldr	q18, [sp, #3984]
	ldr	q19, [sp, #4000]
	eor3	v0.16b, v4.16b, v0.16b, v16.16b
	eor3	v1.16b, v5.16b, v1.16b, v17.16b
	eor3	v2.16b, v6.16b, v2.16b, v18.16b
	eor3	v3.16b, v7.16b, v3.16b, v19.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldr	q0, [sp, #2000]
	ldr	q1, [sp, #2000]
	ldr	q2, [sp, #2016]
	ldr	q3, [sp, #2016]
	stp	q0, q2, [sp, #976]
	ldr	q0, [sp, #1968]
	ldr	q2, [sp, #1968]
	ldr	q4, [sp, #1984]
	ldr	q5, [sp, #1984]
	stp	q0, q4, [sp, #944]
	ldr	q0, [sp, #2064]
	ldr	q4, [sp, #2064]
	ldr	q6, [sp, #2080]
	ldr	q7, [sp, #2080]
	str	q6, [sp, #1056]
	str	q0, [sp, #1040]
	ldr	q0, [sp, #2032]
	ldr	q6, [sp, #2032]
	ldr	q16, [sp, #2048]
	ldr	q17, [sp, #2048]
	stp	q0, q16, [sp, #1008]
	str	q2, [sp, #1072]
	str	q5, [sp, #1088]
	str	q1, [sp, #1104]
	str	q3, [sp, #1120]
	str	q6, [sp, #1136]
	str	q17, [sp, #1152]
	str	q4, [sp, #1168]
	str	q7, [sp, #1184]
	ldp	q0, q1, [sp, #944]
	ldp	q2, q3, [sp, #976]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [sp, #1008]
	ldr	q2, [sp, #1040]
	ldr	q3, [sp, #1056]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldr	q0, [sp, #1072]
	ldr	q1, [sp, #1088]
	ldr	q2, [sp, #1104]
	ldr	q3, [sp, #1120]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldr	q0, [sp, #1136]
	ldr	q1, [sp, #1152]
	ldr	q2, [sp, #1168]
	ldr	q3, [sp, #1184]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	movi	v0.2d, #0000000000000000
	str	q0, [sp, #1184]
	str	q0, [sp, #1168]
	str	q0, [sp, #1152]
	str	q0, [sp, #1136]
	str	q0, [sp, #1120]
	str	q0, [sp, #1104]
	str	q0, [sp, #1088]
	str	q0, [sp, #1072]
	str	q0, [sp, #1056]
	str	q0, [sp, #1040]
	stp	q0, q0, [sp, #1008]
	stp	q0, q0, [sp, #976]
	add	x10, sp, #2992
	mov	x9, x10
	stp	q0, q0, [sp, #944]
	b	LBB0_32
LBB0_31: // in Loop: Header=BB0_32 Depth=1
	strh	w11, [x20, x8, lsl #1]
	add	x8, x8, #1
	mov	x10, x9
	cmp	x8, #128
	b.eq	LBB0_36
LBB0_32: // =>This Inner Loop Header: Depth=1
	ldrh	w11, [x20, x8, lsl #1]
	ldrh	w12, [x9], #2
	eor	w11, w11, w12
	cmp	x8, #383
	b.hi	LBB0_31
// %bb.33:                               ;   in Loop: Header=BB0_32 Depth=1
	ldrh	w12, [x10, #256]
	eor	w11, w11, w12
	cmp	x8, #255
	b.hi	LBB0_31
// %bb.34:                               ;   in Loop: Header=BB0_32 Depth=1
	ldrh	w12, [x10, #512]
	eor	w11, w11, w12
	cmp	x8, #127
	b.hi	LBB0_31
// %bb.35:                               ;   in Loop: Header=BB0_32 Depth=1
	ldrh	w10, [x10, #768]
	eor	w11, w11, w10
	b	LBB0_31
LBB0_36:
	mov	x10, #0 // =0x0
	mov	x8, #0 // =0x0
	mov	x9, #0 // =0x0
	movi	v23.8h, #1
	ldp	q1, q6, [sp, #944]
	and	v2.16b, v1.16b, v23.16b
	ushll2	v3.4s, v2.8h, #0
	ushll	v1.2d, v3.2s, #0
	ldp	q7, q0, [sp, #64] // 32-byte Folded Reload
	ushl	v5.2d, v1.2d, v7.2d
	ldr	q1, [sp, #1072]
	ldr	q16, [sp, #1088]
	and	v4.16b, v1.16b, v23.16b
	ushll2	v17.4s, v4.8h, #0
	ushll	v1.2d, v17.2s, #0
	ushl	v1.2d, v1.2d, v7.2d
	ushll	v18.4s, v2.4h, #0
	ushll	v2.2d, v18.2s, #0
	ushl	v7.2d, v2.2d, v0.2d
	ushll	v4.4s, v4.4h, #0
	ushll	v2.2d, v4.2s, #0
	ushl	v2.2d, v2.2d, v0.2d
	ushll2	v3.2d, v3.4s, #0
	ldr	q0, [sp, #96] // 16-byte Folded Reload
	ushl	v19.2d, v3.2d, v0.2d
	ushll2	v3.2d, v17.4s, #0
	ushl	v3.2d, v3.2d, v0.2d
	ushll2	v17.2d, v18.4s, #0
	ldr	q0, [sp, #112] // 16-byte Folded Reload
	ushl	v17.2d, v17.2d, v0.2d
	ushll2	v4.2d, v4.4s, #0
	ushl	v4.2d, v4.2d, v0.2d
	and	v18.16b, v6.16b, v23.16b
	ushll2	v20.4s, v18.8h, #0
	ushll	v6.2d, v20.2s, #0
	ldr	q0, [sp, #160] // 16-byte Folded Reload
	ushl	v6.2d, v6.2d, v0.2d
	and	v16.16b, v16.16b, v23.16b
	ushll2	v21.4s, v16.8h, #0
	ushll	v22.2d, v21.2s, #0
	ushl	v25.2d, v22.2d, v0.2d
	ushll	v18.4s, v18.4h, #0
	ushll	v22.2d, v18.2s, #0
	ldr	q0, [sp, #176] // 16-byte Folded Reload
	ushl	v29.2d, v22.2d, v0.2d
	ushll	v16.4s, v16.4h, #0
	ushll	v22.2d, v16.2s, #0
	ushl	v26.2d, v22.2d, v0.2d
	ushll2	v20.2d, v20.4s, #0
	ldr	q0, [sp, #192] // 16-byte Folded Reload
	ushl	v30.2d, v20.2d, v0.2d
	ushll2	v20.2d, v21.4s, #0
	ushl	v27.2d, v20.2d, v0.2d
	ushll2	v18.2d, v18.4s, #0
	ldr	q0, [sp, #224] // 16-byte Folded Reload
	ushl	v18.2d, v18.2d, v0.2d
	ushll2	v16.2d, v16.4s, #0
	ushl	v28.2d, v16.2d, v0.2d
	ldp	q16, q20, [sp, #976]
	and	v16.16b, v16.16b, v23.16b
	ushll	v21.4s, v16.4h, #0
	ushll2	v22.2d, v21.4s, #0
	ldr	q11, [sp, #288] // 16-byte Folded Reload
	ushl	v22.2d, v22.2d, v11.2d
	orr	v24.16b, v22.16b, v17.16b
	ushll2	v16.4s, v16.8h, #0
	ushll2	v17.2d, v16.4s, #0
	ldp	q8, q31, [sp, #336] // 32-byte Folded Reload
	ushl	v17.2d, v17.2d, v8.2d
	orr	v19.16b, v17.16b, v19.16b
	ushll	v17.2d, v21.2s, #0
	ushl	v17.2d, v17.2d, v31.2d
	orr	v22.16b, v17.16b, v7.16b
	ushll	v7.2d, v16.2s, #0
	ldr	q0, [sp, #368] // 16-byte Folded Reload
	ushl	v7.2d, v7.2d, v0.2d
	orr	v21.16b, v7.16b, v5.16b
	and	v5.16b, v20.16b, v23.16b
	ushll	v7.4s, v5.4h, #0
	ushll2	v16.2d, v7.4s, #0
	ldp	q13, q12, [sp, #256] // 32-byte Folded Reload
	ushl	v16.2d, v16.2d, v13.2d
	orr	v20.16b, v16.16b, v18.16b
	ushll2	v5.4s, v5.8h, #0
	ushll2	v16.2d, v5.4s, #0
	ushl	v16.2d, v16.2d, v12.2d
	orr	v16.16b, v16.16b, v30.16b
	ushll	v7.2d, v7.2s, #0
	ldp	q10, q9, [sp, #304] // 32-byte Folded Reload
	ushl	v7.2d, v7.2d, v10.2d
	orr	v18.16b, v7.16b, v29.16b
	ushll	v5.2d, v5.2s, #0
	ushl	v5.2d, v5.2d, v9.2d
	orr	v5.16b, v5.16b, v6.16b
	str	q5, [sp, #224] // 16-byte Folded Spill
	ldr	q5, [sp, #1104]
	ldr	q6, [sp, #1120]
	and	v5.16b, v5.16b, v23.16b
	ushll2	v7.4s, v5.8h, #0
	ushll	v17.2d, v7.2s, #0
	ushl	v29.2d, v17.2d, v0.2d
	ushll	v5.4s, v5.4h, #0
	ushll	v17.2d, v5.2s, #0
	ushl	v30.2d, v17.2d, v31.2d
	ushll2	v7.2d, v7.4s, #0
	ushl	v7.2d, v7.2d, v8.2d
	ushll2	v5.2d, v5.4s, #0
	ushl	v5.2d, v5.2d, v11.2d
	and	v6.16b, v6.16b, v23.16b
	ushll2	v17.4s, v6.8h, #0
	ushll	v31.2d, v17.2s, #0
	ushl	v31.2d, v31.2d, v9.2d
	ushll	v6.4s, v6.4h, #0
	ushll	v8.2d, v6.2s, #0
	ushl	v8.2d, v8.2d, v10.2d
	ushll2	v17.2d, v17.4s, #0
	ushl	v9.2d, v17.2d, v12.2d
	ushll2	v6.2d, v6.4s, #0
	ushl	v10.2d, v6.2d, v13.2d
	orr	v17.16b, v5.16b, v4.16b
	orr	v7.16b, v7.16b, v3.16b
	orr	v6.16b, v30.16b, v2.16b
	orr	v2.16b, v29.16b, v1.16b
	orr	v0.16b, v10.16b, v28.16b
	stp	q2, q0, [sp, #304] // 32-byte Folded Spill
	orr	v1.16b, v9.16b, v27.16b
	orr	v0.16b, v8.16b, v26.16b
	stp	q1, q0, [sp, #336] // 32-byte Folded Spill
	orr	v0.16b, v31.16b, v25.16b
	str	q0, [sp, #368] // 16-byte Folded Spill
	ldp	q1, q29, [sp, #1008]
	and	v1.16b, v1.16b, v23.16b
	ushll	v27.4s, v1.4h, #0
	ushll2	v25.2d, v27.4s, #0
	ldr	q0, [sp, #400] // 16-byte Folded Reload
	ushl	v9.2d, v25.2d, v0.2d
	ldr	q25, [sp, #1136]
	ldr	q30, [sp, #1152]
	and	v26.16b, v25.16b, v23.16b
	ushll	v28.4s, v26.4h, #0
	ushll2	v25.2d, v28.4s, #0
	ushl	v25.2d, v25.2d, v0.2d
	ushll2	v1.4s, v1.8h, #0
	ushll2	v31.2d, v1.4s, #0
	ldr	q0, [sp, #240] // 16-byte Folded Reload
	ushl	v10.2d, v31.2d, v0.2d
	ushll2	v31.4s, v26.8h, #0
	ushll2	v26.2d, v31.4s, #0
	ushl	v26.2d, v26.2d, v0.2d
	ushll	v27.2d, v27.2s, #0
	ldr	q0, [sp, #208] // 16-byte Folded Reload
	ushl	v11.2d, v27.2d, v0.2d
	ushll	v27.2d, v28.2s, #0
	ushl	v27.2d, v27.2d, v0.2d
	ushll	v1.2d, v1.2s, #0
	ldp	q0, q2, [sp, #128] // 32-byte Folded Reload
	ushl	v12.2d, v1.2d, v2.2d
	ushll	v1.2d, v31.2s, #0
	ushl	v28.2d, v1.2d, v2.2d
	and	v1.16b, v29.16b, v23.16b
	ushll	v31.4s, v1.4h, #0
	ushll2	v29.2d, v31.4s, #0
	ushl	v13.2d, v29.2d, v0.2d
	and	v30.16b, v30.16b, v23.16b
	ushll	v8.4s, v30.4h, #0
	ushll2	v29.2d, v8.4s, #0
	ushl	v29.2d, v29.2d, v0.2d
	ushll2	v1.4s, v1.8h, #0
	ushll2	v14.2d, v1.4s, #0
	ldp	q2, q3, [sp, #32] // 32-byte Folded Reload
	ushl	v14.2d, v14.2d, v3.2d
	ushll2	v0.4s, v30.8h, #0
	ushll2	v30.2d, v0.4s, #0
	ushl	v30.2d, v30.2d, v3.2d
	ushll	v31.2d, v31.2s, #0
	ushl	v15.2d, v31.2d, v2.2d
	ushll	v31.2d, v8.2s, #0
	ushl	v31.2d, v31.2d, v2.2d
	ushll	v1.2d, v1.2s, #0
	ldr	q2, [sp, #16] // 16-byte Folded Reload
	ushl	v3.2d, v1.2d, v2.2d
	ushll	v0.2d, v0.2s, #0
	ushl	v8.2d, v0.2d, v2.2d
	ldr	q0, [sp, #1040]
	ldr	q2, [sp, #1056]
	and	v0.16b, v0.16b, v23.16b
	ushll	v1.4s, v0.4h, #0
	ushll2	v4.2d, v1.4s, #0
	ldr	q5, [sp, #512] // 16-byte Folded Reload
	ushl	v4.2d, v4.2d, v5.2d
	orr	v4.16b, v4.16b, v9.16b
	orr	v24.16b, v4.16b, v24.16b
	ushll2	v4.4s, v0.8h, #0
	ushll2	v0.2d, v4.4s, #0
	ldr	q9, [sp, #496] // 16-byte Folded Reload
	ushl	v0.2d, v0.2d, v9.2d
	orr	v0.16b, v0.16b, v10.16b
	orr	v0.16b, v0.16b, v19.16b
	ushll	v1.2d, v1.2s, #0
	ldr	q10, [sp, #480] // 16-byte Folded Reload
	ushl	v1.2d, v1.2d, v10.2d
	orr	v1.16b, v1.16b, v11.16b
	orr	v22.16b, v1.16b, v22.16b
	ushll	v1.2d, v4.2s, #0
	ldr	q11, [sp, #448] // 16-byte Folded Reload
	ushl	v1.2d, v1.2d, v11.2d
	orr	v1.16b, v1.16b, v12.16b
	orr	v21.16b, v1.16b, v21.16b
	and	v1.16b, v2.16b, v23.16b
	ushll	v2.4s, v1.4h, #0
	ushll2	v4.2d, v2.4s, #0
	ldr	q12, [sp, #464] // 16-byte Folded Reload
	ushl	v4.2d, v4.2d, v12.2d
	orr	v4.16b, v4.16b, v13.16b
	orr	v20.16b, v4.16b, v20.16b
	ushll2	v1.4s, v1.8h, #0
	ushll2	v4.2d, v1.4s, #0
	ldr	q13, [sp, #432] // 16-byte Folded Reload
	ushl	v4.2d, v4.2d, v13.2d
	orr	v4.16b, v4.16b, v14.16b
	orr	v19.16b, v4.16b, v16.16b
	ushll	v2.2d, v2.2s, #0
	ldr	q14, [sp, #416] // 16-byte Folded Reload
	ushl	v2.2d, v2.2d, v14.2d
	orr	v2.16b, v2.16b, v15.16b
	orr	v18.16b, v2.16b, v18.16b
	ushll	v1.2d, v1.2s, #0
	ldr	q15, [sp, #384] // 16-byte Folded Reload
	ushl	v1.2d, v1.2d, v15.2d
	orr	v1.16b, v1.16b, v3.16b
	ldr	q2, [sp, #224] // 16-byte Folded Reload
	orr	v16.16b, v1.16b, v2.16b
	ldr	q1, [sp, #1168]
	ldr	q2, [sp, #1184]
	and	v1.16b, v1.16b, v23.16b
	and	v2.16b, v2.16b, v23.16b
	ushll2	v3.4s, v1.8h, #0
	ushll	v4.2d, v3.2s, #0
	ushl	v4.2d, v4.2d, v11.2d
	ushll	v1.4s, v1.4h, #0
	ushll	v23.2d, v1.2s, #0
	ushl	v23.2d, v23.2d, v10.2d
	ushll2	v3.2d, v3.4s, #0
	ushl	v3.2d, v3.2d, v9.2d
	ushll2	v1.2d, v1.4s, #0
	ushl	v1.2d, v1.2d, v5.2d
	ushll2	v9.4s, v2.8h, #0
	ushll	v10.2d, v9.2s, #0
	ushl	v10.2d, v10.2d, v15.2d
	ushll	v2.4s, v2.4h, #0
	ushll	v11.2d, v2.2s, #0
	ushl	v11.2d, v11.2d, v14.2d
	ushll2	v9.2d, v9.4s, #0
	ushl	v9.2d, v9.2d, v13.2d
	ushll2	v2.2d, v2.4s, #0
	ushl	v2.2d, v2.2d, v12.2d
	orr	v1.16b, v1.16b, v25.16b
	orr	v1.16b, v1.16b, v17.16b
	orr	v3.16b, v3.16b, v26.16b
	orr	v3.16b, v3.16b, v7.16b
	orr	v7.16b, v23.16b, v27.16b
	orr	v6.16b, v7.16b, v6.16b
	orr	v4.16b, v4.16b, v28.16b
	ldp	q7, q5, [sp, #304] // 32-byte Folded Reload
	orr	v4.16b, v4.16b, v7.16b
	orr	v2.16b, v2.16b, v29.16b
	orr	v2.16b, v2.16b, v5.16b
	orr	v5.16b, v9.16b, v30.16b
	ldp	q7, q17, [sp, #336] // 32-byte Folded Reload
	orr	v5.16b, v5.16b, v7.16b
	orr	v7.16b, v11.16b, v31.16b
	orr	v7.16b, v7.16b, v17.16b
	orr	v17.16b, v10.16b, v8.16b
	ldr	q23, [sp, #368] // 16-byte Folded Reload
	orr	v17.16b, v17.16b, v23.16b
	orr	v4.16b, v17.16b, v4.16b
	orr	v6.16b, v7.16b, v6.16b
	orr	v3.16b, v5.16b, v3.16b
	orr	v1.16b, v2.16b, v1.16b
	orr	v1.16b, v1.16b, v3.16b
	orr	v2.16b, v6.16b, v4.16b
	orr	v1.16b, v2.16b, v1.16b
	ext	v2.16b, v1.16b, v1.16b, #8
	orr	v1.8b, v1.8b, v2.8b
	fmov	x11, d1
	orr	v1.16b, v16.16b, v21.16b
	orr	v2.16b, v18.16b, v22.16b
	orr	v0.16b, v19.16b, v0.16b
	orr	v3.16b, v20.16b, v24.16b
	orr	v0.16b, v3.16b, v0.16b
	orr	v1.16b, v2.16b, v1.16b
	orr	v0.16b, v1.16b, v0.16b
	ext	v1.16b, v0.16b, v0.16b, #8
	orr	v0.8b, v0.8b, v1.8b
	fmov	x12, d0
LBB0_37: // =>This Inner Loop Header: Depth=1
	mov	x13, x12
	ldrh	w12, [x19, x10]
	and	x12, x12, #0x1
	neg	x12, x12
	and	x14, x13, x12
	eor	x9, x14, x9
	and	x12, x11, x12
	eor	x8, x12, x8
	extr	x12, x13, x11, #63
	extr	x11, x11, x13, #63
	add	x10, x10, #2
	cmp	x10, #128
	b.ne	LBB0_37
LBB0_38: // =>This Inner Loop Header: Depth=1
	ldrh	w13, [x19, x10]
	and	x13, x13, #0x1
	neg	x13, x13
	and	x14, x12, x13
	eor	x9, x14, x9
	and	x13, x11, x13
	extr	x14, x12, x11, #63
	eor	x8, x13, x8
	extr	x11, x11, x12, #63
	add	x10, x10, #2
	mov	x12, x14
	cmp	x10, #256
	b.ne	LBB0_38
// %bb.39:
	dup	v0.2d, x8
	dup	v1.2d, x9
Lloh140:
	adrp	x8, lCPI0_70
Lloh141:
	ldr	q2, [x8, :lo12:lCPI0_70]
	ushl	v3.2d, v1.2d, v2.2d
Lloh142:
	adrp	x8, lCPI0_71
Lloh143:
	ldr	q4, [x8, :lo12:lCPI0_71]
	ushl	v5.2d, v1.2d, v4.2d
Lloh144:
	adrp	x8, lCPI0_72
Lloh145:
	ldr	q6, [x8, :lo12:lCPI0_72]
	ushl	v7.2d, v1.2d, v6.2d
Lloh146:
	adrp	x8, lCPI0_73
Lloh147:
	ldr	q16, [x8, :lo12:lCPI0_73]
	ushl	v17.2d, v1.2d, v16.2d
	uzp1	v7.4s, v17.4s, v7.4s
	uzp1	v3.4s, v5.4s, v3.4s
	uzp1	v3.8h, v3.8h, v7.8h
	ushl	v2.2d, v0.2d, v2.2d
	ushl	v4.2d, v0.2d, v4.2d
	ushl	v5.2d, v0.2d, v6.2d
	ushl	v6.2d, v0.2d, v16.2d
	uzp1	v5.4s, v6.4s, v5.4s
	uzp1	v2.4s, v4.4s, v2.4s
	uzp1	v4.8h, v2.8h, v5.8h
Lloh148:
	adrp	x8, lCPI0_74
Lloh149:
	ldr	q5, [x8, :lo12:lCPI0_74]
	ushl	v2.2d, v1.2d, v5.2d
Lloh150:
	adrp	x8, lCPI0_76
Lloh151:
	ldr	q6, [x8, :lo12:lCPI0_76]
	ushl	v7.2d, v1.2d, v6.2d
Lloh152:
	adrp	x8, lCPI0_77
Lloh153:
	ldr	q16, [x8, :lo12:lCPI0_77]
	ushl	v17.2d, v1.2d, v16.2d
	uzp1	v7.4s, v17.4s, v7.4s
Lloh154:
	adrp	x8, lCPI0_75
Lloh155:
	ldr	q17, [x8, :lo12:lCPI0_75]
	ushl	v18.2d, v1.2d, v17.2d
	uzp1	v2.4s, v18.4s, v2.4s
	uzp1	v7.8h, v2.8h, v7.8h
	movi	v2.8h, #1
	and	v3.16b, v3.16b, v2.16b
	and	v7.16b, v7.16b, v2.16b
	str	q3, [sp, #1968]
	str	q7, [sp, #1984]
	ushl	v3.2d, v0.2d, v6.2d
	ushl	v6.2d, v0.2d, v16.2d
	uzp1	v3.4s, v6.4s, v3.4s
	ushl	v5.2d, v0.2d, v5.2d
	ushl	v6.2d, v0.2d, v17.2d
	uzp1	v5.4s, v6.4s, v5.4s
	and	v4.16b, v4.16b, v2.16b
	uzp1	v3.8h, v5.8h, v3.8h
	and	v3.16b, v3.16b, v2.16b
	str	q4, [sp, #2096]
	str	q3, [sp, #2112]
	ldr	q6, [sp, #848] // 16-byte Folded Reload
	ushl	v3.2d, v1.2d, v6.2d
	ldp	q17, q16, [sp, #752] // 32-byte Folded Reload
	ushl	v4.2d, v1.2d, v16.2d
	ushl	v5.2d, v1.2d, v17.2d
	uzp1	v4.4s, v5.4s, v4.4s
	ldp	q18, q7, [sp, #800] // 32-byte Folded Reload
	ushl	v5.2d, v1.2d, v7.2d
	uzp1	v3.4s, v5.4s, v3.4s
	uzp1	v3.8h, v3.8h, v4.8h
	ushl	v4.2d, v0.2d, v6.2d
	ushl	v5.2d, v0.2d, v7.2d
	ushl	v6.2d, v0.2d, v16.2d
	ushl	v7.2d, v0.2d, v17.2d
	uzp1	v6.4s, v7.4s, v6.4s
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v4.8h, v4.8h, v6.8h
	ldr	q16, [sp, #864] // 16-byte Folded Reload
	ushl	v5.2d, v1.2d, v16.2d
	ldr	q17, [sp, #832] // 16-byte Folded Reload
	ushl	v6.2d, v1.2d, v17.2d
	ushl	v7.2d, v1.2d, v18.2d
	uzp1	v6.4s, v7.4s, v6.4s
	ldr	q19, [sp, #784] // 16-byte Folded Reload
	ushl	v7.2d, v1.2d, v19.2d
	uzp1	v5.4s, v7.4s, v5.4s
	uzp1	v5.8h, v5.8h, v6.8h
	and	v3.16b, v3.16b, v2.16b
	and	v5.16b, v5.16b, v2.16b
	str	q3, [sp, #2000]
	str	q5, [sp, #2016]
	ushl	v3.2d, v0.2d, v17.2d
	ushl	v5.2d, v0.2d, v18.2d
	uzp1	v3.4s, v5.4s, v3.4s
	ushl	v5.2d, v0.2d, v16.2d
	ushl	v6.2d, v0.2d, v19.2d
	uzp1	v5.4s, v6.4s, v5.4s
	and	v4.16b, v4.16b, v2.16b
	uzp1	v3.8h, v5.8h, v3.8h
	and	v3.16b, v3.16b, v2.16b
	str	q4, [sp, #2128]
	str	q3, [sp, #2144]
	ldp	q18, q6, [sp, #592] // 32-byte Folded Reload
	ushl	v3.2d, v1.2d, v6.2d
	ldp	q17, q16, [sp, #528] // 32-byte Folded Reload
	ushl	v4.2d, v1.2d, v16.2d
	ushl	v5.2d, v1.2d, v17.2d
	uzp1	v4.4s, v5.4s, v4.4s
	ldp	q19, q7, [sp, #560] // 32-byte Folded Reload
	ushl	v5.2d, v1.2d, v7.2d
	uzp1	v3.4s, v5.4s, v3.4s
	uzp1	v3.8h, v3.8h, v4.8h
	ushl	v4.2d, v0.2d, v6.2d
	ushl	v5.2d, v0.2d, v7.2d
	ushl	v6.2d, v0.2d, v16.2d
	ushl	v7.2d, v0.2d, v17.2d
	uzp1	v6.4s, v7.4s, v6.4s
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v4.8h, v4.8h, v6.8h
	ldr	q16, [sp, #704] // 16-byte Folded Reload
	ushl	v5.2d, v1.2d, v16.2d
	ushl	v6.2d, v1.2d, v18.2d
	ushl	v7.2d, v1.2d, v19.2d
	uzp1	v6.4s, v7.4s, v6.4s
	ldr	q17, [sp, #672] // 16-byte Folded Reload
	ushl	v7.2d, v1.2d, v17.2d
	uzp1	v5.4s, v7.4s, v5.4s
	uzp1	v5.8h, v5.8h, v6.8h
	and	v3.16b, v3.16b, v2.16b
	and	v5.16b, v5.16b, v2.16b
	str	q3, [sp, #2032]
	str	q5, [sp, #2048]
	ushl	v3.2d, v0.2d, v18.2d
	ushl	v5.2d, v0.2d, v19.2d
	uzp1	v3.4s, v5.4s, v3.4s
	ushl	v5.2d, v0.2d, v16.2d
	ushl	v6.2d, v0.2d, v17.2d
	uzp1	v5.4s, v6.4s, v5.4s
	and	v4.16b, v4.16b, v2.16b
	uzp1	v3.8h, v5.8h, v3.8h
	and	v3.16b, v3.16b, v2.16b
	str	q4, [sp, #2160]
	str	q3, [sp, #2176]
	ldr	q6, [sp, #688] // 16-byte Folded Reload
	ushl	v3.2d, v1.2d, v6.2d
	ldp	q17, q16, [sp, #624] // 32-byte Folded Reload
	ushl	v4.2d, v1.2d, v16.2d
	ushl	v5.2d, v1.2d, v17.2d
	uzp1	v4.4s, v5.4s, v4.4s
	ldr	q7, [sp, #656] // 16-byte Folded Reload
	ushl	v5.2d, v1.2d, v7.2d
	uzp1	v3.4s, v5.4s, v3.4s
	uzp1	v3.8h, v3.8h, v4.8h
	and	v3.16b, v3.16b, v2.16b
	ushl	v4.2d, v0.2d, v6.2d
	ushl	v5.2d, v0.2d, v7.2d
	ushl	v6.2d, v0.2d, v16.2d
	ushl	v7.2d, v0.2d, v17.2d
	uzp1	v6.4s, v7.4s, v6.4s
	uzp1	v4.4s, v5.4s, v4.4s
	uzp1	v4.8h, v4.8h, v6.8h
	and	v2.16b, v4.16b, v2.16b
Lloh156:
	adrp	x8, lCPI0_78
Lloh157:
	ldr	q4, [x8, :lo12:lCPI0_78]
	ushl	v5.2d, v1.2d, v4.2d
Lloh158:
	adrp	x8, lCPI0_79
Lloh159:
	ldr	q6, [x8, :lo12:lCPI0_79]
	ushl	v7.2d, v1.2d, v6.2d
	uzp1	v5.4s, v7.4s, v5.4s
	ldp	q17, q16, [sp, #720] // 32-byte Folded Reload
	ushl	v7.2d, v1.2d, v16.2d
	ushl	v1.2d, v1.2d, v17.2d
	uzp1	v1.4s, v1.4s, v7.4s
	uzp1	v1.8h, v1.8h, v5.8h
	bic	v1.8h, #254
	str	q3, [sp, #2064]
	str	q1, [sp, #2080]
	ushl	v1.2d, v0.2d, v16.2d
	ushl	v3.2d, v0.2d, v17.2d
	ushl	v4.2d, v0.2d, v4.2d
	ushl	v0.2d, v0.2d, v6.2d
	uzp1	v0.4s, v0.4s, v4.4s
	uzp1	v1.4s, v3.4s, v1.4s
	uzp1	v0.8h, v1.8h, v0.8h
	bic	v0.8h, #254
	str	q2, [sp, #2192]
	str	q0, [sp, #2208]
	add	x20, sp, #944
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	mov	x8, #0 // =0x0
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #2992]
	ldr	q5, [sp, #3008]
	ldr	q6, [sp, #3024]
	ldr	q7, [sp, #3040]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #3056]
	ldr	q5, [sp, #3072]
	ldr	q6, [sp, #3088]
	ldr	q7, [sp, #3104]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #3120]
	ldr	q5, [sp, #3136]
	ldr	q6, [sp, #3152]
	ldr	q7, [sp, #3168]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #3184]
	ldr	q5, [sp, #3200]
	ldr	q6, [sp, #3216]
	ldr	q7, [sp, #3232]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #3248]
	ldr	q5, [sp, #3264]
	ldr	q6, [sp, #3280]
	ldr	q7, [sp, #3296]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #3312]
	ldr	q5, [sp, #3328]
	ldr	q6, [sp, #3344]
	ldr	q7, [sp, #3360]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #3376]
	ldr	q5, [sp, #3392]
	ldr	q6, [sp, #3408]
	ldr	q7, [sp, #3424]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #3440]
	ldr	q5, [sp, #3456]
	ldr	q6, [sp, #3472]
	ldr	q7, [sp, #3488]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #3504]
	ldr	q5, [sp, #3520]
	ldr	q6, [sp, #3536]
	ldr	q7, [sp, #3552]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #3568]
	ldr	q5, [sp, #3584]
	ldr	q6, [sp, #3600]
	ldr	q7, [sp, #3616]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #3632]
	ldr	q5, [sp, #3648]
	ldr	q6, [sp, #3664]
	ldr	q7, [sp, #3680]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #3696]
	ldr	q5, [sp, #3712]
	ldr	q6, [sp, #3728]
	ldr	q7, [sp, #3744]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldr	q2, [sp, #2000]
	ldr	q0, [sp, #2000]
	ldr	q3, [sp, #2016]
	ldr	q1, [sp, #2016]
	stp	q2, q3, [sp, #976]
	ldr	q2, [sp, #1968]
	ldr	q3, [sp, #1968]
	ldr	q4, [sp, #1984]
	ldr	q5, [sp, #1984]
	stp	q2, q4, [sp, #944]
	ldr	q2, [sp, #2064]
	ldr	q4, [sp, #2064]
	ldr	q6, [sp, #2080]
	ldr	q7, [sp, #2080]
	str	q6, [sp, #1056]
	str	q2, [sp, #1040]
	ldr	q2, [sp, #2032]
	ldr	q6, [sp, #2032]
	ldr	q16, [sp, #2048]
	ldr	q17, [sp, #2048]
	stp	q2, q16, [sp, #1008]
	ldr	q2, [sp, #2128]
	ldr	q16, [sp, #2128]
	ldr	q18, [sp, #2144]
	ldr	q19, [sp, #2144]
	str	q18, [sp, #1120]
	str	q2, [sp, #1104]
	ldr	q2, [sp, #2096]
	ldr	q18, [sp, #2096]
	ldr	q20, [sp, #2112]
	ldr	q21, [sp, #2112]
	str	q20, [sp, #1088]
	str	q2, [sp, #1072]
	ldr	q2, [sp, #2192]
	ldr	q20, [sp, #2192]
	ldr	q22, [sp, #2208]
	ldr	q23, [sp, #2208]
	str	q22, [sp, #1184]
	str	q2, [sp, #1168]
	ldr	q2, [sp, #2160]
	ldr	q22, [sp, #2160]
	ldr	q24, [sp, #2176]
	ldr	q25, [sp, #2176]
	str	q24, [sp, #1152]
	str	q2, [sp, #1136]
	str	q3, [sp, #1200]
	str	q5, [sp, #1216]
	str	q0, [sp, #1232]
	str	q1, [sp, #1248]
	str	q6, [sp, #1264]
	str	q17, [sp, #1280]
	str	q4, [sp, #1296]
	str	q7, [sp, #1312]
	str	q18, [sp, #1328]
	str	q21, [sp, #1344]
	str	q16, [sp, #1360]
	str	q19, [sp, #1376]
	str	q22, [sp, #1392]
	str	q25, [sp, #1408]
	str	q20, [sp, #1424]
	str	q23, [sp, #1440]
	ldp	q0, q1, [sp, #944]
	ldp	q2, q3, [sp, #976]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [sp, #1008]
	ldr	q2, [sp, #1040]
	ldr	q3, [sp, #1056]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldr	q0, [sp, #1072]
	ldr	q1, [sp, #1088]
	ldr	q2, [sp, #1104]
	ldr	q3, [sp, #1120]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldr	q0, [sp, #1136]
	ldr	q1, [sp, #1152]
	ldr	q2, [sp, #1168]
	ldr	q3, [sp, #1184]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldr	q0, [sp, #1200]
	ldr	q1, [sp, #1216]
	ldr	q2, [sp, #1232]
	ldr	q3, [sp, #1248]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldr	q0, [sp, #1264]
	ldr	q1, [sp, #1280]
	ldr	q2, [sp, #1296]
	ldr	q3, [sp, #1312]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldr	q0, [sp, #1328]
	ldr	q1, [sp, #1344]
	ldr	q2, [sp, #1360]
	ldr	q3, [sp, #1376]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldr	q0, [sp, #1392]
	ldr	q1, [sp, #1408]
	ldr	q2, [sp, #1424]
	ldr	q3, [sp, #1440]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
	movi	v0.2d, #0000000000000000
	str	q0, [sp, #1440]
	str	q0, [sp, #1424]
	str	q0, [sp, #1408]
	str	q0, [sp, #1392]
	str	q0, [sp, #1376]
	str	q0, [sp, #1360]
	str	q0, [sp, #1344]
	str	q0, [sp, #1328]
	str	q0, [sp, #1312]
	str	q0, [sp, #1296]
	str	q0, [sp, #1280]
	str	q0, [sp, #1264]
	str	q0, [sp, #1248]
	str	q0, [sp, #1232]
	str	q0, [sp, #1216]
	str	q0, [sp, #1200]
	str	q0, [sp, #1184]
	str	q0, [sp, #1168]
	str	q0, [sp, #1152]
	str	q0, [sp, #1136]
	str	q0, [sp, #1120]
	str	q0, [sp, #1104]
	str	q0, [sp, #1088]
	str	q0, [sp, #1072]
	str	q0, [sp, #1056]
	str	q0, [sp, #1040]
	stp	q0, q0, [sp, #1008]
	stp	q0, q0, [sp, #976]
	add	x9, sp, #2992
	stp	q0, q0, [sp, #944]
	b	LBB0_41
LBB0_40: // in Loop: Header=BB0_41 Depth=1
	strh	w10, [x20, x8, lsl #1]
	add	x8, x8, #1
	cmp	x8, #256
	b.eq	LBB0_43
LBB0_41: // =>This Inner Loop Header: Depth=1
	ldrh	w10, [x20, x8, lsl #1]
	ldrh	w11, [x9, x8, lsl #1]
	eor	w10, w10, w11
	cmp	x8, #255
	b.hi	LBB0_40
// %bb.42:                               ;   in Loop: Header=BB0_41 Depth=1
	add	x11, x9, x8, lsl #1
	ldrh	w11, [x11, #512]
	eor	w10, w10, w11
	b	LBB0_40
LBB0_43:
	add	x1, sp, #4048
	mov	x0, x19
	bl	pack_binary_256_neon
	add	x0, sp, #944
	add	x1, sp, #4016
	bl	pack_binary_256_neon
	ldr	x9, [sp, #4048]
	ldr	x8, [sp, #4056]
	ldr	x11, [sp, #4016]
	ldr	x10, [sp, #4024]
	eor	x12, x10, x11
	ldr	x16, [sp, #4032]
	ldr	x13, [sp, #4040]
	eor	x15, x16, x11
	fmov	d0, x11
	eor	x11, x8, x9
	ldr	x1, [sp, #4064]
	ldr	x2, [sp, #4072]
	eor	x0, x1, x9
	fmov	d1, x9
	pmull	v0.1q, v1.1d, v0.1d
	eor	x17, x13, x10
	fmov	d1, x10
	eor	x10, x2, x8
	fmov	d2, x8
	fmov	d3, x12
	fmov	d4, x11
	pmull	v3.1q, v4.1d, v3.1d
	zip2	v4.2d, v3.2d, v0.2d
	pmull	v1.1q, v2.1d, v1.1d
	eor	v2.16b, v1.16b, v4.16b
	eor	v4.8b, v0.8b, v1.8b
	ext	v5.16b, v0.16b, v0.16b, #8
	fmov	x9, d0
	umov	x8, v1.d[1]
	eor	v0.8b, v5.8b, v3.8b
	eor	v0.8b, v4.8b, v0.8b
	fmov	x11, d0
	ext	v0.16b, v2.16b, v2.16b, #8
	eor	v0.8b, v2.8b, v0.8b
	fmov	x14, d0
	eor	x12, x13, x16
	fmov	d0, x16
	eor	x16, x2, x1
	fmov	d1, x1
	pmull	v0.1q, v1.1d, v0.1d
	fmov	d1, x13
	fmov	d2, x2
	fmov	d3, x12
	fmov	d4, x16
	pmull	v3.1q, v4.1d, v3.1d
	zip2	v4.2d, v3.2d, v0.2d
	pmull	v1.1q, v2.1d, v1.1d
	eor	v2.16b, v1.16b, v4.16b
	eor	v4.8b, v0.8b, v1.8b
	ext	v5.16b, v0.16b, v0.16b, #8
	fmov	x13, d0
	umov	x16, v1.d[1]
	eor	v0.8b, v5.8b, v3.8b
	eor	v0.8b, v4.8b, v0.8b
	fmov	x1, d0
	ext	v0.16b, v2.16b, v2.16b, #8
	eor	v0.8b, v2.8b, v0.8b
	fmov	x2, d0
	eor	x12, x17, x15
	fmov	d0, x15
	eor	x15, x10, x0
	fmov	d1, x0
	pmull	v0.1q, v1.1d, v0.1d
	umov	x0, v0.d[1]
	fmov	x3, d0
	fmov	d0, x17
	fmov	d1, x10
	pmull	v0.1q, v1.1d, v0.1d
	umov	x10, v0.d[1]
	fmov	x17, d0
	fmov	d0, x12
	fmov	d1, x15
	pmull	v0.1q, v1.1d, v0.1d
	umov	x12, v0.d[1]
	fmov	x15, d0
	eor	x12, x9, x12
	eor	x12, x12, x14
	eor	x4, x13, x0
	eor	x4, x4, x17
	eor	x4, x4, x10
	eor	x12, x12, x4
	eor	x12, x12, x2
	eor	x4, x11, x8
	eor	x10, x16, x10
	eor	x10, x4, x10
	eor	x10, x10, x1
	eor	x9, x14, x9
	eor	x13, x13, x3
	eor	x9, x9, x13
	eor	x9, x9, x2
	eor	x8, x8, x15
	eor	x8, x8, x11
	eor	x11, x16, x0
	eor	x8, x8, x11
	eor	x11, x3, x17
	eor	x11, x11, x1
	eor	x8, x8, x11
	and	w11, w12, #0xff
	dup	v1.8h, w11
Lloh160:
	adrp	x11, lCPI0_80
Lloh161:
	ldr	q0, [x11, :lo12:lCPI0_80]
	sshl	v2.8h, v1.8h, v0.8h
	ubfx	w11, w12, #8, #8
	dup	v3.8h, w11
	movi	v1.8h, #1
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #1968]
	str	q3, [sp, #1984]
	ubfx	w11, w12, #16, #8
	dup	v2.8h, w11
	ubfx	x11, x12, #24, #8
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2000]
	str	q3, [sp, #2016]
	ubfx	x11, x12, #32, #8
	dup	v2.8h, w11
	ubfx	x11, x12, #40, #8
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2032]
	str	q3, [sp, #2048]
	ubfx	x11, x12, #48, #8
	dup	v2.8h, w11
	lsr	x11, x12, #56
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2064]
	str	q3, [sp, #2080]
	and	w11, w10, #0xff
	dup	v2.8h, w11
	ubfx	w11, w10, #8, #8
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2096]
	str	q3, [sp, #2112]
	ubfx	w11, w10, #16, #8
	dup	v2.8h, w11
	ubfx	x11, x10, #24, #8
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2128]
	str	q3, [sp, #2144]
	ubfx	x11, x10, #32, #8
	dup	v2.8h, w11
	ubfx	x11, x10, #40, #8
	dup	v3.8h, w11
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2160]
	str	q3, [sp, #2176]
	ubfx	x11, x10, #48, #8
	dup	v2.8h, w11
	lsr	x10, x10, #56
	dup	v3.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2192]
	str	q3, [sp, #2208]
	and	w10, w9, #0xff
	dup	v2.8h, w10
	ubfx	w10, w9, #8, #8
	dup	v3.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2224]
	str	q3, [sp, #2240]
	ubfx	w10, w9, #16, #8
	dup	v2.8h, w10
	ubfx	x10, x9, #24, #8
	dup	v3.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2256]
	str	q3, [sp, #2272]
	ubfx	x10, x9, #32, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x10, x9, #40, #8
	dup	v3.8h, w10
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2288]
	str	q3, [sp, #2304]
	ubfx	x10, x9, #48, #8
	dup	v2.8h, w10
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	lsr	x9, x9, #56
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2320]
	str	q3, [sp, #2336]
	and	w9, w8, #0xff
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	w9, w8, #8, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2352]
	str	q3, [sp, #2368]
	ubfx	w9, w8, #16, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #24, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2384]
	str	q3, [sp, #2400]
	ubfx	x9, x8, #32, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x9, x8, #40, #8
	dup	v3.8h, w9
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	str	q2, [sp, #2416]
	str	q3, [sp, #2432]
	ubfx	x9, x8, #48, #8
	dup	v2.8h, w9
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	lsr	x8, x8, #56
	dup	v3.8h, w8
	sshl	v0.8h, v3.8h, v0.8h
	and	v0.16b, v0.16b, v1.16b
	str	q2, [sp, #2448]
	str	q0, [sp, #2464]
	add	x0, sp, #1968
	add	x1, sp, #880
	add	x2, sp, #944
	bl	mul_in_R2_512_words_pmull
	ldr	q0, [sp, #2992]
	ldr	q1, [sp, #3008]
	ldr	q2, [sp, #3024]
	ldr	q3, [sp, #3040]
	ldp	q4, q5, [sp, #944]
	ldp	q6, q7, [sp, #976]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #2992]
	str	q1, [sp, #3008]
	str	q2, [sp, #3024]
	str	q3, [sp, #3040]
	ldr	q0, [sp, #3056]
	ldr	q1, [sp, #3072]
	ldr	q2, [sp, #3088]
	ldr	q3, [sp, #3104]
	ldp	q4, q5, [sp, #1008]
	ldr	q6, [sp, #1040]
	ldr	q7, [sp, #1056]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3056]
	str	q1, [sp, #3072]
	str	q2, [sp, #3088]
	str	q3, [sp, #3104]
	ldr	q0, [sp, #3120]
	ldr	q1, [sp, #3136]
	ldr	q2, [sp, #3152]
	ldr	q3, [sp, #3168]
	ldr	q4, [sp, #1072]
	ldr	q5, [sp, #1088]
	ldr	q6, [sp, #1104]
	ldr	q7, [sp, #1120]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3120]
	str	q1, [sp, #3136]
	str	q2, [sp, #3152]
	str	q3, [sp, #3168]
	ldr	q0, [sp, #3184]
	ldr	q1, [sp, #3200]
	ldr	q2, [sp, #3216]
	ldr	q3, [sp, #3232]
	ldr	q4, [sp, #1136]
	ldr	q5, [sp, #1152]
	ldr	q6, [sp, #1168]
	ldr	q7, [sp, #1184]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3184]
	str	q1, [sp, #3200]
	str	q2, [sp, #3216]
	str	q3, [sp, #3232]
	ldr	q0, [sp, #3248]
	ldr	q1, [sp, #3264]
	ldr	q2, [sp, #3280]
	ldr	q3, [sp, #3296]
	ldr	q4, [sp, #1200]
	ldr	q5, [sp, #1216]
	ldr	q6, [sp, #1232]
	ldr	q7, [sp, #1248]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3248]
	str	q1, [sp, #3264]
	str	q2, [sp, #3280]
	str	q3, [sp, #3296]
	ldr	q0, [sp, #3312]
	ldr	q1, [sp, #3328]
	ldr	q2, [sp, #3344]
	ldr	q3, [sp, #3360]
	ldr	q4, [sp, #1264]
	ldr	q5, [sp, #1280]
	ldr	q6, [sp, #1296]
	ldr	q7, [sp, #1312]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3312]
	str	q1, [sp, #3328]
	str	q2, [sp, #3344]
	str	q3, [sp, #3360]
	ldr	q0, [sp, #3376]
	ldr	q1, [sp, #3392]
	ldr	q2, [sp, #3408]
	ldr	q3, [sp, #3424]
	ldr	q4, [sp, #1328]
	ldr	q5, [sp, #1344]
	ldr	q6, [sp, #1360]
	ldr	q7, [sp, #1376]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3376]
	str	q1, [sp, #3392]
	str	q2, [sp, #3408]
	str	q3, [sp, #3424]
	ldr	q0, [sp, #3440]
	ldr	q1, [sp, #3456]
	ldr	q2, [sp, #3472]
	ldr	q3, [sp, #3488]
	ldr	q4, [sp, #1392]
	ldr	q5, [sp, #1408]
	ldr	q6, [sp, #1424]
	ldr	q7, [sp, #1440]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3440]
	str	q1, [sp, #3456]
	str	q2, [sp, #3472]
	str	q3, [sp, #3488]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #1456]
	ldr	q5, [sp, #1472]
	ldr	q6, [sp, #1488]
	ldr	q7, [sp, #1504]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #1520]
	ldr	q5, [sp, #1536]
	ldr	q6, [sp, #1552]
	ldr	q7, [sp, #1568]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #1584]
	ldr	q5, [sp, #1600]
	ldr	q6, [sp, #1616]
	ldr	q7, [sp, #1632]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #1648]
	ldr	q5, [sp, #1664]
	ldr	q6, [sp, #1680]
	ldr	q7, [sp, #1696]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #1712]
	ldr	q5, [sp, #1728]
	ldr	q6, [sp, #1744]
	ldr	q7, [sp, #1760]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #1776]
	ldr	q5, [sp, #1792]
	ldr	q6, [sp, #1808]
	ldr	q7, [sp, #1824]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #1840]
	ldr	q5, [sp, #1856]
	ldr	q6, [sp, #1872]
	ldr	q7, [sp, #1888]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #1904]
	ldr	q5, [sp, #1920]
	ldr	q6, [sp, #1936]
	ldr	q7, [sp, #1952]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	ldr	q0, [sp, #3504]
	ldr	q1, [sp, #3520]
	ldr	q2, [sp, #3536]
	ldr	q3, [sp, #3552]
	ldr	q4, [sp, #2992]
	ldr	q5, [sp, #3008]
	ldr	q6, [sp, #3024]
	ldr	q7, [sp, #3040]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3504]
	str	q1, [sp, #3520]
	str	q2, [sp, #3536]
	str	q3, [sp, #3552]
	ldr	q0, [sp, #3568]
	ldr	q1, [sp, #3584]
	ldr	q2, [sp, #3600]
	ldr	q3, [sp, #3616]
	ldr	q4, [sp, #3056]
	ldr	q5, [sp, #3072]
	ldr	q6, [sp, #3088]
	ldr	q7, [sp, #3104]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3568]
	str	q1, [sp, #3584]
	str	q2, [sp, #3600]
	str	q3, [sp, #3616]
	ldr	q0, [sp, #3632]
	ldr	q1, [sp, #3648]
	ldr	q2, [sp, #3664]
	ldr	q3, [sp, #3680]
	ldr	q4, [sp, #3120]
	ldr	q5, [sp, #3136]
	ldr	q6, [sp, #3152]
	ldr	q7, [sp, #3168]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3632]
	str	q1, [sp, #3648]
	str	q2, [sp, #3664]
	str	q3, [sp, #3680]
	ldr	q0, [sp, #3696]
	ldr	q1, [sp, #3712]
	ldr	q2, [sp, #3728]
	ldr	q3, [sp, #3744]
	ldr	q4, [sp, #3184]
	ldr	q5, [sp, #3200]
	ldr	q6, [sp, #3216]
	ldr	q7, [sp, #3232]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3696]
	str	q1, [sp, #3712]
	str	q2, [sp, #3728]
	str	q3, [sp, #3744]
	ldr	q0, [sp, #3760]
	ldr	q1, [sp, #3776]
	ldr	q2, [sp, #3792]
	ldr	q3, [sp, #3808]
	ldr	q4, [sp, #3248]
	ldr	q5, [sp, #3264]
	ldr	q6, [sp, #3280]
	ldr	q7, [sp, #3296]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3760]
	str	q1, [sp, #3776]
	str	q2, [sp, #3792]
	str	q3, [sp, #3808]
	ldr	q0, [sp, #3824]
	ldr	q1, [sp, #3840]
	ldr	q2, [sp, #3856]
	ldr	q3, [sp, #3872]
	ldr	q4, [sp, #3312]
	ldr	q5, [sp, #3328]
	ldr	q6, [sp, #3344]
	ldr	q7, [sp, #3360]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3824]
	str	q1, [sp, #3840]
	str	q2, [sp, #3856]
	str	q3, [sp, #3872]
	ldr	q0, [sp, #3888]
	ldr	q1, [sp, #3904]
	ldr	q2, [sp, #3920]
	ldr	q3, [sp, #3936]
	ldr	q4, [sp, #3376]
	ldr	q5, [sp, #3392]
	ldr	q6, [sp, #3408]
	ldr	q7, [sp, #3424]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3888]
	str	q1, [sp, #3904]
	str	q2, [sp, #3920]
	str	q3, [sp, #3936]
	ldr	q0, [sp, #3952]
	ldr	q1, [sp, #3968]
	ldr	q2, [sp, #3984]
	ldr	q3, [sp, #4000]
	ldr	q4, [sp, #3440]
	ldr	q5, [sp, #3456]
	ldr	q6, [sp, #3472]
	ldr	q7, [sp, #3488]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	str	q0, [sp, #3952]
	str	q1, [sp, #3968]
	str	q2, [sp, #3984]
	str	q3, [sp, #4000]
	add	x0, sp, #944
	add	x1, sp, #1968
	mov	w2, #512 // =0x200
	bl	memcpy
	ldr	q0, [sp, #1968]
	ldr	q1, [sp, #1984]
	ldr	q2, [sp, #2000]
	ldr	q3, [sp, #2016]
	str	q0, [sp, #1456]
	str	q1, [sp, #1472]
	str	q2, [sp, #1488]
	str	q3, [sp, #1504]
	ldr	q0, [sp, #2032]
	ldr	q1, [sp, #2048]
	ldr	q2, [sp, #2064]
	ldr	q3, [sp, #2080]
	str	q0, [sp, #1520]
	str	q1, [sp, #1536]
	str	q2, [sp, #1552]
	str	q3, [sp, #1568]
	ldr	q0, [sp, #2096]
	ldr	q1, [sp, #2112]
	ldr	q2, [sp, #2128]
	ldr	q3, [sp, #2144]
	str	q0, [sp, #1584]
	str	q1, [sp, #1600]
	str	q2, [sp, #1616]
	str	q3, [sp, #1632]
	ldr	q0, [sp, #2160]
	ldr	q1, [sp, #2176]
	ldr	q2, [sp, #2192]
	ldr	q3, [sp, #2208]
	str	q0, [sp, #1648]
	str	q1, [sp, #1664]
	str	q2, [sp, #1680]
	str	q3, [sp, #1696]
	ldr	q0, [sp, #2224]
	ldr	q1, [sp, #2240]
	ldr	q2, [sp, #2256]
	ldr	q3, [sp, #2272]
	str	q0, [sp, #1712]
	str	q1, [sp, #1728]
	str	q2, [sp, #1744]
	str	q3, [sp, #1760]
	ldr	q0, [sp, #2288]
	ldr	q1, [sp, #2304]
	ldr	q2, [sp, #2320]
	ldr	q3, [sp, #2336]
	str	q0, [sp, #1776]
	str	q1, [sp, #1792]
	str	q2, [sp, #1808]
	str	q3, [sp, #1824]
	ldr	q0, [sp, #2352]
	ldr	q1, [sp, #2368]
	ldr	q2, [sp, #2384]
	ldr	q3, [sp, #2400]
	str	q0, [sp, #1840]
	str	q1, [sp, #1856]
	str	q2, [sp, #1872]
	str	q3, [sp, #1888]
	ldr	q0, [sp, #2416]
	ldr	q1, [sp, #2432]
	ldr	q2, [sp, #2448]
	ldr	q3, [sp, #2464]
	str	q0, [sp, #1904]
	str	q1, [sp, #1920]
	str	q2, [sp, #1936]
	str	q3, [sp, #1952]
	ldp	q0, q1, [sp, #944]
	ldp	q2, q3, [sp, #976]
	ldp	q4, q5, [x19]
	ldp	q6, q7, [x19, #32]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19]
	stp	q2, q3, [x19, #32]
	ldp	q0, q1, [sp, #1008]
	ldr	q2, [sp, #1040]
	ldr	q3, [sp, #1056]
	ldp	q4, q5, [x19, #64]
	ldp	q6, q7, [x19, #96]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #64]
	stp	q2, q3, [x19, #96]
	ldr	q0, [sp, #1072]
	ldr	q1, [sp, #1088]
	ldr	q2, [sp, #1104]
	ldr	q3, [sp, #1120]
	ldp	q4, q5, [x19, #128]
	ldp	q6, q7, [x19, #160]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #128]
	stp	q2, q3, [x19, #160]
	ldr	q0, [sp, #1136]
	ldr	q1, [sp, #1152]
	ldr	q2, [sp, #1168]
	ldr	q3, [sp, #1184]
	ldp	q4, q5, [x19, #192]
	ldp	q6, q7, [x19, #224]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #192]
	stp	q2, q3, [x19, #224]
	ldr	q0, [sp, #1200]
	ldr	q1, [sp, #1216]
	ldr	q2, [sp, #1232]
	ldr	q3, [sp, #1248]
	ldp	q4, q5, [x19, #256]
	ldp	q6, q7, [x19, #288]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #256]
	stp	q2, q3, [x19, #288]
	ldr	q0, [sp, #1264]
	ldr	q1, [sp, #1280]
	ldr	q2, [sp, #1296]
	ldr	q3, [sp, #1312]
	ldp	q4, q5, [x19, #320]
	ldp	q6, q7, [x19, #352]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #320]
	stp	q2, q3, [x19, #352]
	ldr	q0, [sp, #1328]
	ldr	q1, [sp, #1344]
	ldr	q2, [sp, #1360]
	ldr	q3, [sp, #1376]
	ldp	q4, q5, [x19, #384]
	ldp	q6, q7, [x19, #416]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #384]
	stp	q2, q3, [x19, #416]
	ldr	q0, [sp, #1392]
	ldr	q1, [sp, #1408]
	ldr	q2, [sp, #1424]
	ldr	q3, [sp, #1440]
	ldp	q4, q5, [x19, #448]
	ldp	q6, q7, [x19, #480]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #448]
	stp	q2, q3, [x19, #480]
	ldr	q0, [sp, #1456]
	ldr	q1, [sp, #1472]
	ldr	q2, [sp, #1488]
	ldr	q3, [sp, #1504]
	ldp	q4, q5, [x19, #512]
	ldp	q6, q7, [x19, #544]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #512]
	stp	q2, q3, [x19, #544]
	ldr	q0, [sp, #1520]
	ldr	q1, [sp, #1536]
	ldr	q2, [sp, #1552]
	ldr	q3, [sp, #1568]
	ldp	q4, q5, [x19, #576]
	ldp	q6, q7, [x19, #608]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #576]
	stp	q2, q3, [x19, #608]
	ldr	q0, [sp, #1584]
	ldr	q1, [sp, #1600]
	ldr	q2, [sp, #1616]
	ldr	q3, [sp, #1632]
	ldp	q4, q5, [x19, #640]
	ldp	q6, q7, [x19, #672]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #640]
	stp	q2, q3, [x19, #672]
	ldr	q0, [sp, #1648]
	ldr	q1, [sp, #1664]
	ldr	q2, [sp, #1680]
	ldr	q3, [sp, #1696]
	ldp	q4, q5, [x19, #704]
	ldp	q6, q7, [x19, #736]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #704]
	stp	q2, q3, [x19, #736]
	ldr	q0, [sp, #1712]
	ldr	q1, [sp, #1728]
	ldr	q2, [sp, #1744]
	ldr	q3, [sp, #1760]
	ldp	q4, q5, [x19, #768]
	ldp	q6, q7, [x19, #800]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #768]
	stp	q2, q3, [x19, #800]
	ldr	q0, [sp, #1776]
	ldr	q1, [sp, #1792]
	ldr	q2, [sp, #1808]
	ldr	q3, [sp, #1824]
	ldp	q4, q5, [x19, #832]
	ldp	q6, q7, [x19, #864]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #832]
	stp	q2, q3, [x19, #864]
	ldr	q0, [sp, #1840]
	ldr	q1, [sp, #1856]
	ldr	q2, [sp, #1872]
	ldr	q3, [sp, #1888]
	ldp	q4, q5, [x19, #896]
	ldp	q6, q7, [x19, #928]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #896]
	stp	q2, q3, [x19, #928]
	ldr	q0, [sp, #1904]
	ldr	q1, [sp, #1920]
	ldr	q2, [sp, #1936]
	ldr	q3, [sp, #1952]
	ldp	q4, q5, [x19, #960]
	ldp	q6, q7, [x19, #992]
	eor	v0.16b, v4.16b, v0.16b
	eor	v1.16b, v5.16b, v1.16b
	eor	v2.16b, v6.16b, v2.16b
	eor	v3.16b, v7.16b, v3.16b
	stp	q0, q1, [x19, #960]
	stp	q2, q3, [x19, #992]
	add	sp, sp, #4080
	ldp	x29, x30, [sp, #144] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #128] // 16-byte Folded Reload
	ldp	x22, x21, [sp, #112] // 16-byte Folded Reload
	ldp	x24, x23, [sp, #96] // 16-byte Folded Reload
	ldp	x26, x25, [sp, #80] // 16-byte Folded Reload
	ldp	x28, x27, [sp, #64] // 16-byte Folded Reload
	ldp	d9, d8, [sp, #48] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #32] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #16] // 16-byte Folded Reload
	ldp	d15, d14, [sp], #160 // 16-byte Folded Reload
	ret
	.cfi_endproc
                                        // -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function mul_in_R2_512_words_pmull
lCPI1_0:
	.short	0 // 0x0
	.short	65535 // 0xffff
	.short	65534 // 0xfffe
	.short	65533 // 0xfffd
	.short	65532 // 0xfffc
	.short	65531 // 0xfffb
	.short	65530 // 0xfffa
	.short	65529 // 0xfff9
	.text
	.p2align	2
mul_in_R2_512_words_pmull: // @mul_in_R2_512_words_pmull
	.cfi_startproc
// %bb.0:
	sub	sp, sp, #288
	stp	d15, d14, [sp, #128] // 16-byte Folded Spill
	stp	d13, d12, [sp, #144] // 16-byte Folded Spill
	stp	d11, d10, [sp, #160] // 16-byte Folded Spill
	stp	d9, d8, [sp, #176] // 16-byte Folded Spill
	stp	x28, x27, [sp, #192] // 16-byte Folded Spill
	stp	x26, x25, [sp, #208] // 16-byte Folded Spill
	stp	x24, x23, [sp, #224] // 16-byte Folded Spill
	stp	x22, x21, [sp, #240] // 16-byte Folded Spill
	stp	x20, x19, [sp, #256] // 16-byte Folded Spill
	stp	x29, x30, [sp, #272] // 16-byte Folded Spill
	.cfi_def_cfa_offset 288
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
	.cfi_offset b8, -104
	.cfi_offset b9, -112
	.cfi_offset b10, -120
	.cfi_offset b11, -128
	.cfi_offset b12, -136
	.cfi_offset b13, -144
	.cfi_offset b14, -152
	.cfi_offset b15, -160
	mov	x19, x2
	mov	x20, x1
	mov	x21, x0
	add	x22, sp, #64
	add	x1, sp, #64
	bl	pack_binary_256_neon
	add	x0, x21, #512
	add	x1, x22, #32
	bl	pack_binary_256_neon
	mov	x8, #0 // =0x0
	ldp	x9, x15, [sp, #64]
	ldp	x14, x10, [x20]
	eor	x0, x10, x14
	ldp	x1, x2, [x20, #16]
	eor	x21, x1, x14
	ldp	x12, x11, [x20, #32]
	eor	x13, x12, x14
	fmov	d0, x14
	eor	x4, x15, x9
	ldp	x22, x23, [sp, #80]
	eor	x24, x22, x9
	ldp	x7, x3, [sp, #96]
	eor	x14, x7, x9
	fmov	d1, x9
	pmull	v0.1q, v1.1d, v0.1d
	eor	x9, x2, x10
	eor	x16, x11, x10
	fmov	d1, x10
	eor	x10, x23, x15
	eor	x17, x3, x15
	fmov	d2, x15
	fmov	d7, x0
	fmov	d20, x4
	dup	v21.2d, v0.d[1]
	eor	x25, x2, x1
	ldp	x5, x4, [x20, #48]
	eor	x15, x5, x1
	fmov	d3, x1
	eor	x26, x23, x22
	ldp	x20, x6, [sp, #112]
	eor	x0, x20, x22
	fmov	d4, x22
	eor	x1, x4, x2
	fmov	d5, x2
	eor	x2, x6, x23
	fmov	d6, x23
	fmov	d16, x25
	fmov	d17, x26
	eor	x25, x9, x21
	fmov	d18, x21
	eor	x26, x10, x24
	fmov	d19, x24
	eor	x21, x20, x7
	eor	x27, x6, x3
	eor	x23, x5, x12
	eor	x24, x4, x11
	eor	x22, x24, x23
	fmov	d22, x24
	eor	x24, x27, x21
	fmov	d23, x27
	pmull	v7.1q, v20.1d, v7.1d
	eor	v21.16b, v21.16b, v7.16b
	zip2	v7.2d, v7.2d, v0.2d
	pmull	v1.1q, v2.1d, v1.1d
	eor	v20.16b, v1.16b, v7.16b
	eor3	v21.16b, v21.16b, v1.16b, v0.16b
	pmull	v2.1q, v23.1d, v22.1d
	dup	v7.2d, v2.d[0]
	mov	v7.d[0], v21.d[0]
	fmov	d22, x9
	fmov	x9, d21
	ext	v21.16b, v20.16b, v20.16b, #8
	eor	v20.8b, v20.8b, v21.8b
	fmov	d21, x10
	fmov	x10, d20
	pmull	v3.1q, v4.1d, v3.1d
	pmull	v30.1q, v6.1d, v5.1d
	pmull	v28.1q, v17.1d, v16.1d
	pmull	v4.1q, v19.1d, v18.1d
	pmull	v18.1q, v21.1d, v22.1d
	fmov	d5, x25
	fmov	d6, x26
	pmull	v19.1q, v6.1d, v5.1d
	eor	x25, x11, x12
	fmov	d5, x12
	eor	x12, x3, x7
	fmov	d6, x7
	pmull	v6.1q, v6.1d, v5.1d
	fmov	d5, x11
	fmov	d16, x3
	fmov	d17, x25
	fmov	d20, x12
	pmull	v17.1q, v20.1d, v17.1d
	zip2	v20.2d, v17.2d, v6.2d
	pmull	v25.1q, v16.1d, v5.1d
	eor	v5.16b, v25.16b, v20.16b
	eor	v16.8b, v6.8b, v25.8b
	ext	v20.16b, v6.16b, v6.16b, #8
	eor	v17.8b, v20.8b, v17.8b
	eor	v16.8b, v16.8b, v17.8b
	fmov	x3, d16
	ext	v16.16b, v5.16b, v5.16b, #8
	eor	v5.8b, v5.8b, v16.8b
	fmov	x11, d5
	eor	x12, x4, x5
	fmov	d5, x5
	eor	x5, x6, x20
	fmov	d16, x20
	pmull	v26.1q, v16.1d, v5.1d
	fmov	d5, x4
	fmov	d16, x6
	pmull	v5.1q, v16.1d, v5.1d
	fmov	d16, x12
	fmov	d17, x5
	pmull	v10.1q, v17.1d, v16.1d
	fmov	d16, x23
	fmov	d17, x21
	pmull	v16.1q, v17.1d, v16.1d
	fmov	d17, x22
	fmov	d20, x24
	pmull	v27.1q, v20.1d, v17.1d
	eor	x12, x16, x13
	eor	x4, x15, x13
	fmov	d17, x13
	eor	x13, x17, x14
	eor	x5, x0, x14
	fmov	d20, x14
	pmull	v17.1q, v20.1d, v17.1d
	eor	x6, x1, x16
	fmov	d20, x16
	eor	x16, x2, x17
	fmov	d21, x17
	fmov	d22, x12
	fmov	d23, x13
	pmull	v23.1q, v23.1d, v22.1d
	zip2	v22.2d, v23.2d, v17.2d
	pmull	v20.1q, v21.1d, v20.1d
	eor	v21.16b, v20.16b, v22.16b
	ext	v22.16b, v21.16b, v21.16b, #8
	eor	v21.8b, v21.8b, v22.8b
	fmov	x12, d21
	eor	x13, x1, x15
	fmov	d21, x15
	eor	x14, x2, x0
	fmov	d22, x0
	pmull	v22.1q, v22.1d, v21.1d
	fmov	d21, x1
	fmov	d24, x2
	pmull	v12.1q, v24.1d, v21.1d
	fmov	d21, x13
	fmov	d24, x14
	pmull	v21.1q, v24.1d, v21.1d
	eor	v24.16b, v22.16b, v21.16b
	fmov	x14, d21
	umov	x13, v24.d[1]
	eor	x15, x6, x4
	fmov	d21, x4
	eor	x17, x16, x5
	fmov	d24, x5
	pmull	v21.1q, v24.1d, v21.1d
	fmov	d24, x6
	fmov	d29, x16
	pmull	v9.1q, v29.1d, v24.1d
	zip2	v24.2d, v3.2d, v26.2d
	mov	v29.16b, v28.16b
	mov	v29.d[1], v10.d[1]
	eor	v31.16b, v24.16b, v29.16b
	eor3	v11.16b, v1.16b, v30.16b, v18.16b
	mov	v13.16b, v5.16b
	mov	v13.d[1], v30.d[0]
	mov	v8.16b, v30.16b
	mov	v8.d[1], v5.d[0]
	eor3	v29.16b, v24.16b, v29.16b, v8.16b
	mov	v14.16b, v3.16b
	mov	v14.d[1], v5.d[1]
	eor3	v24.16b, v31.16b, v8.16b, v14.16b
	dup	v8.2d, v18.d[0]
	zip2	v15.2d, v26.2d, v3.2d
	mov	v31.16b, v16.16b
	mov	v31.d[1], v18.d[1]
	mov	v18.d[1], v26.d[0]
	mov	v26.d[1], v30.d[1]
	dup	v11.2d, v11.d[1]
	eor3	v11.16b, v29.16b, v14.16b, v11.16b
	fmov	d29, x15
	fmov	d14, x17
	pmull	v29.1q, v14.1d, v29.1d
	fmov	x15, d11
	mov	v10.d[1], v28.d[1]
	eor	v28.16b, v15.16b, v10.16b
	eor3	v10.16b, v15.16b, v10.16b, v13.16b
	dup	v11.2d, v16.d[1]
	mov	v11.d[1], x10
	eor3	v28.16b, v28.16b, v13.16b, v26.16b
	mov	v13.16b, v27.16b
	mov	v13.d[1], v19.d[1]
	eor	v14.16b, v11.16b, v13.16b
	mov	v15.16b, v2.16b
	mov	v15.d[1], v3.d[0]
	eor3	v13.16b, v11.16b, v13.16b, v15.16b
	mov	v11.16b, v4.16b
	mov	v11.d[1], v2.d[1]
	eor3	v2.16b, v25.16b, v5.16b, v2.16b
	ext	v30.16b, v30.16b, v6.16b, #8
	umov	x17, v12.d[1]
	fmov	x1, d12
	ext	v12.16b, v12.16b, v17.16b, #8
	umov	x16, v9.d[1]
	fmov	x0, d9
	dup	v9.2d, v9.d[0]
	zip2	v25.2d, v25.2d, v4.2d
	eor3	v14.16b, v14.16b, v15.16b, v25.16b
	fmov	x2, d0
	umov	x4, v20.d[1]
	umov	x5, v22.d[1]
	fmov	x6, d22
	eor	x14, x14, x6
	eor	x14, x14, x1
	eor	x14, x5, x14
	eor	x1, x1, x17
	eor	x13, x13, x1
	umov	x5, v21.d[1]
	mov	v8.d[0], x3
	eor3	v25.16b, v13.16b, v25.16b, v8.16b
	eor3	v8.16b, v14.16b, v8.16b, v31.16b
	dup	v13.2d, v4.d[1]
	mov	v13.d[1], x11
	mov	v19.d[1], v27.d[1]
	eor3	v25.16b, v25.16b, v31.16b, v28.16b
	eor	v27.16b, v13.16b, v19.16b
	eor3	v19.16b, v13.16b, v19.16b, v18.16b
	zip2	v1.2d, v1.2d, v16.2d
	eor3	v18.16b, v27.16b, v18.16b, v1.16b
	eor3	v1.16b, v19.16b, v1.16b, v7.16b
	eor3	v7.16b, v18.16b, v7.16b, v11.16b
	eor3	v1.16b, v1.16b, v11.16b, v24.16b
	dup	v2.2d, v2.d[1]
	eor3	v2.16b, v10.16b, v26.16b, v2.16b
	fmov	x1, d2
	eor	x15, x3, x15
	eor3	v2.16b, v8.16b, v28.16b, v29.16b
	dup	v2.2d, v2.d[1]
	eor3	v2.16b, v2.16b, v6.16b, v22.16b
	fmov	x3, d2
	umov	x6, v1.d[1]
	eor	x3, x3, x6
	eor	x0, x5, x0
	eor	x4, x15, x4
	eor	x17, x1, x17
	eor	x17, x17, x16
	eor	x17, x4, x17
	ext	v2.16b, v5.16b, v0.16b, #8
	eor3	v2.16b, v30.16b, v2.16b, v12.16b
	eor3	v2.16b, v8.16b, v28.16b, v2.16b
	eor	x0, x0, x2
	eor	x0, x0, x12
	eor	x16, x0, x16
	eor	x16, x16, x13
	eor	x16, x3, x16
	eor	x0, x14, x9
	eor	x17, x17, x0
	stp	x16, x17, [sp]
	eor3	v2.16b, v7.16b, v24.16b, v2.16b
	stur	q2, [sp, #24]
	mov	v3.d[1], v17.d[1]
	dup	v2.2d, v23.d[0]
	mov	v2.d[0], x10
	mov	v0.d[1], v20.d[0]
	eor	v2.16b, v2.16b, v3.16b
	mov	v4.d[1], v17.d[0]
	eor3	v0.16b, v2.16b, v0.16b, v4.16b
	dup	v2.2d, v29.d[0]
	mov	v2.d[0], x11
	umov	x10, v0.d[1]
	eor	x9, x9, x15
	zip1	v3.2d, v6.2d, v25.2d
	eor	v4.16b, v2.16b, v3.16b
	zip1	v1.2d, v16.2d, v1.2d
	eor3	v2.16b, v2.16b, v3.16b, v1.16b
	zip2	v3.2d, v28.2d, v20.2d
	eor3	v1.16b, v4.16b, v1.16b, v3.16b
	eor3	v2.16b, v2.16b, v3.16b, v26.16b
	mov	v17.d[1], v21.d[1]
	mov	v22.d[1], v21.d[0]
	mov	v9.d[0], x12
	eor	v3.16b, v17.16b, v22.16b
	eor	v3.16b, v9.16b, v3.16b
	mov	v21.d[1], x14
	dup	v4.2d, v24.d[1]
	eor3	v1.16b, v1.16b, v26.16b, v4.16b
	fmov	x11, d1
	zip2	v1.2d, v24.2d, v5.2d
	eor3	v1.16b, v3.16b, v21.16b, v1.16b
	eor3	v1.16b, v1.16b, v0.16b, v2.16b
	fmov	x12, d0
	eor	x11, x11, x12
	eor	x11, x11, x13
	str	x11, [sp, #16]
	eor	x9, x1, x9
	eor	x9, x9, x10
	str	x9, [sp, #40]
	str	q1, [sp, #48]
	add	x9, x19, #64
	mov	x10, sp
Lloh162:
	adrp	x11, lCPI1_0
Lloh163:
	ldr	q0, [x11, :lo12:lCPI1_0]
	movi	v1.8h, #1
LBB1_1: // =>This Inner Loop Header: Depth=1
	ldr	x11, [x10, x8]
	and	w12, w11, #0xff
	dup	v2.8h, w12
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	w12, w11, #8, #8
	dup	v3.8h, w12
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x9, #-64]
	ubfx	w12, w11, #16, #8
	dup	v2.8h, w12
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x12, x11, #24, #8
	dup	v3.8h, w12
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x9, #-32]
	ubfx	x12, x11, #32, #8
	dup	v2.8h, w12
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	ubfx	x12, x11, #40, #8
	dup	v3.8h, w12
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x9]
	ubfx	x12, x11, #48, #8
	dup	v2.8h, w12
	sshl	v2.8h, v2.8h, v0.8h
	and	v2.16b, v2.16b, v1.16b
	lsr	x11, x11, #56
	dup	v3.8h, w11
	sshl	v3.8h, v3.8h, v0.8h
	and	v3.16b, v3.16b, v1.16b
	stp	q2, q3, [x9, #32]
	add	x8, x8, #8
	add	x9, x9, #128
	cmp	x8, #64
	b.ne	LBB1_1
// %bb.2:
	ldp	x29, x30, [sp, #272] // 16-byte Folded Reload
	ldp	x20, x19, [sp, #256] // 16-byte Folded Reload
	ldp	x22, x21, [sp, #240] // 16-byte Folded Reload
	ldp	x24, x23, [sp, #224] // 16-byte Folded Reload
	ldp	x26, x25, [sp, #208] // 16-byte Folded Reload
	ldp	x28, x27, [sp, #192] // 16-byte Folded Reload
	ldp	d9, d8, [sp, #176] // 16-byte Folded Reload
	ldp	d11, d10, [sp, #160] // 16-byte Folded Reload
	ldp	d13, d12, [sp, #144] // 16-byte Folded Reload
	ldp	d15, d14, [sp, #128] // 16-byte Folded Reload
	add	sp, sp, #288
	ret
	.cfi_endproc
                                        // -- End function
	.section	.rodata.cst16,"aM",@progbits,16
	.p2align	4, 0x0 // -- Begin function pack_binary_256_neon
lCPI2_0:
	.short	0 // 0x0
	.short	1 // 0x1
	.short	2 // 0x2
	.short	3 // 0x3
	.short	4 // 0x4
	.short	5 // 0x5
	.short	6 // 0x6
	.short	7 // 0x7
	.text
	.p2align	2
pack_binary_256_neon: // @pack_binary_256_neon
	.cfi_startproc
// %bb.0:
	stp	xzr, xzr, [x1]
	ldr	q1, [x0]
	movi	v0.8h, #1
	and	v2.16b, v1.16b, v0.16b
Lloh164:
	adrp	x8, lCPI2_0
Lloh165:
	ldr	q1, [x8, :lo12:lCPI2_0]
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w8, v2.b[0]
	str	x8, [x1]
	ldr	q2, [x0, #16]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #8, #8
	str	x8, [x1]
	ldr	q2, [x0, #32]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #16, #8
	str	x8, [x1]
	ldr	q2, [x0, #48]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #24, #8
	str	x8, [x1]
	ldr	q2, [x0, #64]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #32
	str	x8, [x1]
	ldr	q2, [x0, #80]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #40
	str	x8, [x1]
	ldr	q2, [x0, #96]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #48
	str	x8, [x1]
	ldr	q2, [x0, #112]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #56
	str	x8, [x1]
	ldr	q2, [x0, #128]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w8, v2.b[0]
	ldr	x9, [x1, #8]
	orr	x8, x9, x8
	str	x8, [x1, #8]
	ldr	q2, [x0, #144]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #8
	str	x8, [x1, #8]
	ldr	q2, [x0, #160]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #16
	str	x8, [x1, #8]
	ldr	q2, [x0, #176]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #24
	str	x8, [x1, #8]
	ldr	q2, [x0, #192]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #32
	str	x8, [x1, #8]
	ldr	q2, [x0, #208]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #40
	str	x8, [x1, #8]
	ldr	q2, [x0, #224]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #48
	str	x8, [x1, #8]
	ldr	q2, [x0, #240]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #56
	stp	xzr, xzr, [x1, #16]
	str	x8, [x1, #8]
	ldr	q2, [x0, #256]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w8, v2.b[0]
	str	x8, [x1, #16]
	ldr	q2, [x0, #272]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #8, #8
	str	x8, [x1, #16]
	ldr	q2, [x0, #288]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #16, #8
	str	x8, [x1, #16]
	ldr	q2, [x0, #304]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	bfi	x8, x9, #24, #8
	str	x8, [x1, #16]
	ldr	q2, [x0, #320]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #32
	str	x8, [x1, #16]
	ldr	q2, [x0, #336]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #40
	str	x8, [x1, #16]
	ldr	q2, [x0, #352]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #48
	str	x8, [x1, #16]
	ldr	q2, [x0, #368]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #56
	str	x8, [x1, #16]
	ldr	q2, [x0, #384]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w8, v2.b[0]
	ldr	x9, [x1, #24]
	orr	x8, x9, x8
	str	x8, [x1, #24]
	ldr	q2, [x0, #400]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #8
	str	x8, [x1, #24]
	ldr	q2, [x0, #416]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #16
	str	x8, [x1, #24]
	ldr	q2, [x0, #432]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #24
	str	x8, [x1, #24]
	ldr	q2, [x0, #448]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #32
	str	x8, [x1, #24]
	ldr	q2, [x0, #464]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #40
	str	x8, [x1, #24]
	ldr	q2, [x0, #480]
	and	v2.16b, v2.16b, v0.16b
	sshl	v2.8h, v2.8h, v1.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	addp	v2.8h, v2.8h, v2.8h
	umov	w9, v2.b[0]
	orr	x8, x8, x9, lsl #48
	str	x8, [x1, #24]
	ldr	q2, [x0, #496]
	and	v0.16b, v2.16b, v0.16b
	sshl	v0.8h, v0.8h, v1.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	addp	v0.8h, v0.8h, v0.8h
	umov	w9, v0.b[0]
	orr	x8, x8, x9, lsl #56
	str	x8, [x1, #24]
	ret
	.cfi_endproc
                                        // -- End function
