.text

.macro INV_R3_16_CHUNK
	vmovdqu       (%rax), %ymm4
	vmovdqu  (%rax,%r10), %ymm5
	vmovdqu  (%rax,%r11), %ymm6

	vpsubw %ymm4, %ymm5, %ymm7
	MONT16 %ymm7, oaep2592_16xomega_qinv(%rip), oaep2592_16xomega(%rip), %ymm13

	vpsubw %ymm4, %ymm6, %ymm7
	CENTER16 %ymm7
	vpaddw %ymm13, %ymm7, %ymm7
	MONT16_REG %ymm7, %ymm14, %ymm1, %ymm8
	CENTER16 %ymm8

	vpsubw %ymm5, %ymm6, %ymm7
	CENTER16 %ymm7
	vpsubw %ymm13, %ymm7, %ymm7
	MONT16 %ymm7, 32(%r8), 32(%r9), %ymm9
	CENTER16 %ymm9

	vpaddw %ymm5, %ymm4, %ymm7
	CENTER16 %ymm7
	vpaddw %ymm6, %ymm7, %ymm7
	CENTER16 %ymm7

	vmovdqu %ymm7,       (%rax)
	vmovdqu %ymm8,  (%rax,%r10)
	vmovdqu %ymm9,  (%rax,%r11)
.endm

.macro MONT16 src qmem zmem out
	vmovdqa \qmem, %ymm10
	vmovdqa \zmem, %ymm11
	vpmullw %ymm11, \src, %ymm12
	vpmullw %ymm10, \src, %ymm15
	vpmulhw %ymm11, \src, \out
	vpmullw %ymm0, %ymm15, %ymm10
	vpmulhw %ymm0, %ymm15, %ymm15
	vpxor   oaep2592_16xsign(%rip), %ymm10, %ymm10
	vpxor   oaep2592_16xsign(%rip), %ymm12, %ymm12
	vpcmpgtw %ymm12, %ymm10, %ymm10
	vpand   oaep2592_16xone(%rip), %ymm10, %ymm10
	vpsubw  %ymm15, \out, \out
	vpsubw  %ymm10, \out, \out
.endm

.macro MONT16_REG src qreg zreg out
	vpmullw \zreg, \src, %ymm12
	vpmullw \qreg, \src, %ymm15
	vpmulhw \zreg, \src, \out
	vpmullw %ymm0, %ymm15, %ymm10
	vpmulhw %ymm0, %ymm15, %ymm15
	vpxor   oaep2592_16xsign(%rip), %ymm10, %ymm10
	vpxor   oaep2592_16xsign(%rip), %ymm12, %ymm12
	vpcmpgtw %ymm12, %ymm10, %ymm10
	vpand   oaep2592_16xone(%rip), %ymm10, %ymm10
	vpsubw  %ymm15, \out, \out
	vpsubw  %ymm10, \out, \out
.endm

.macro CENTER16 reg
	vpcmpgtw %ymm2, \reg, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpsubw   %ymm15, \reg, \reg
	vpcmpgtw \reg, %ymm3, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpaddw   %ymm15, \reg, \reg
.endm

.macro LOAD_TAIL8 coeff dst
	vpxor \dst, \dst, \dst
	vmovdqa oaep2592_tail48_gather_mask8(%rip), %ymm13
	vpgatherdd %ymm13, (2*(\coeff))(%rsi,%ymm14,1), \dst
	vpslld $16, \dst, \dst
	vpsrad $16, \dst, \dst
	vextracti128 $1, \dst, %xmm15
	vpackssdw %xmm15, %xmm4, %xmm4
.endm

.macro LOAD_TAIL6 coeff dst
	vpxor \dst, \dst, \dst
	vmovdqa oaep2592_tail48_gather_mask6(%rip), %ymm13
	vpgatherdd %ymm13, (2*(\coeff))(%rsi,%ymm14,1), \dst
	vpslld $16, \dst, \dst
	vpsrad $16, \dst, \dst
	vextracti128 $1, \dst, %xmm15
	vpackssdw %xmm15, %xmm4, %xmm4
.endm

.macro STORE_TAIL8 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*96)(%rdi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*96)(%rdi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*96)(%rdi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*96)(%rdi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*96)(%rdi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*96)(%rdi)
	vpextrw $6, %xmm14, (2*(\coeff) + 6*96)(%rdi)
	vpextrw $7, %xmm14, (2*(\coeff) + 7*96)(%rdi)
.endm

.macro STORE_TAIL6 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*96)(%rdi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*96)(%rdi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*96)(%rdi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*96)(%rdi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*96)(%rdi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*96)(%rdi)
.endm

.macro STACK_LOAD coeff STORE
	LOAD_TAIL\STORE \coeff, %ymm4
	vmovdqu %ymm4, (32*(\coeff))(%rsp)
.endm

.macro STACK_STORE coeff STORE
	vmovdqu (32*(\coeff))(%rsp), %ymm4
	STORE_TAIL\STORE \coeff, %ymm4
.endm

.macro TRANSPOSE_STORE8 base offset
	vmovdqu (32*(\base + 0))(%rsp), %xmm4
	vmovdqu (32*(\base + 1))(%rsp), %xmm5
	vmovdqu (32*(\base + 2))(%rsp), %xmm6
	vmovdqu (32*(\base + 3))(%rsp), %xmm7
	vmovdqu (32*(\base + 4))(%rsp), %xmm8
	vmovdqu (32*(\base + 5))(%rsp), %xmm9
	vmovdqu (32*(\base + 6))(%rsp), %xmm10
	vmovdqu (32*(\base + 7))(%rsp), %xmm11

	vpunpcklwd %xmm5, %xmm4, %xmm12
	vpunpckhwd %xmm5, %xmm4, %xmm13
	vpunpcklwd %xmm7, %xmm6, %xmm14
	vpunpckhwd %xmm7, %xmm6, %xmm15
	vpunpcklwd %xmm9, %xmm8, %xmm0
	vpunpckhwd %xmm9, %xmm8, %xmm1
	vpunpcklwd %xmm11, %xmm10, %xmm2
	vpunpckhwd %xmm11, %xmm10, %xmm3

	vpunpckldq %xmm14, %xmm12, %xmm4
	vpunpckhdq %xmm14, %xmm12, %xmm5
	vpunpckldq %xmm15, %xmm13, %xmm6
	vpunpckhdq %xmm15, %xmm13, %xmm7
	vpunpckldq %xmm2, %xmm0, %xmm8
	vpunpckhdq %xmm2, %xmm0, %xmm9
	vpunpckldq %xmm3, %xmm1, %xmm10
	vpunpckhdq %xmm3, %xmm1, %xmm11

	vpunpcklqdq %xmm8, %xmm4, %xmm12
	vpunpckhqdq %xmm8, %xmm4, %xmm13
	vpunpcklqdq %xmm9, %xmm5, %xmm14
	vpunpckhqdq %xmm9, %xmm5, %xmm15
	vmovdqu %xmm12, (2*(\base) + \offset + 0*96)(%rdi)
	vmovdqu %xmm13, (2*(\base) + \offset + 1*96)(%rdi)
	vmovdqu %xmm14, (2*(\base) + \offset + 2*96)(%rdi)
	vmovdqu %xmm15, (2*(\base) + \offset + 3*96)(%rdi)

	vpunpcklqdq %xmm10, %xmm6, %xmm12
	vpunpckhqdq %xmm10, %xmm6, %xmm13
	vpunpcklqdq %xmm11, %xmm7, %xmm14
	vpunpckhqdq %xmm11, %xmm7, %xmm15
	vmovdqu %xmm12, (2*(\base) + \offset + 4*96)(%rdi)
	vmovdqu %xmm13, (2*(\base) + \offset + 5*96)(%rdi)
	vmovdqu %xmm14, (2*(\base) + \offset + 6*96)(%rdi)
	vmovdqu %xmm15, (2*(\base) + \offset + 7*96)(%rdi)
.endm

.macro TRANSPOSE_STORE16_8 base
	TRANSPOSE_STORE8 \base, 0
	TRANSPOSE_STORE8 "(\base + 8)", 0
.endm

.macro TRANSPOSE_STORE8_6 base offset
	vmovdqu (32*(\base + 0))(%rsp), %xmm4
	vmovdqu (32*(\base + 1))(%rsp), %xmm5
	vmovdqu (32*(\base + 2))(%rsp), %xmm6
	vmovdqu (32*(\base + 3))(%rsp), %xmm7
	vmovdqu (32*(\base + 4))(%rsp), %xmm8
	vmovdqu (32*(\base + 5))(%rsp), %xmm9
	vmovdqu (32*(\base + 6))(%rsp), %xmm10
	vmovdqu (32*(\base + 7))(%rsp), %xmm11

	vpunpcklwd %xmm5, %xmm4, %xmm12
	vpunpckhwd %xmm5, %xmm4, %xmm13
	vpunpcklwd %xmm7, %xmm6, %xmm14
	vpunpckhwd %xmm7, %xmm6, %xmm15
	vpunpcklwd %xmm9, %xmm8, %xmm0
	vpunpckhwd %xmm9, %xmm8, %xmm1
	vpunpcklwd %xmm11, %xmm10, %xmm2
	vpunpckhwd %xmm11, %xmm10, %xmm3

	vpunpckldq %xmm14, %xmm12, %xmm4
	vpunpckhdq %xmm14, %xmm12, %xmm5
	vpunpckldq %xmm15, %xmm13, %xmm6
	vpunpckhdq %xmm15, %xmm13, %xmm7
	vpunpckldq %xmm2, %xmm0, %xmm8
	vpunpckhdq %xmm2, %xmm0, %xmm9
	vpunpckldq %xmm3, %xmm1, %xmm10
	vpunpckhdq %xmm3, %xmm1, %xmm11

	vpunpcklqdq %xmm8, %xmm4, %xmm12
	vpunpckhqdq %xmm8, %xmm4, %xmm13
	vpunpcklqdq %xmm9, %xmm5, %xmm14
	vpunpckhqdq %xmm9, %xmm5, %xmm15
	vmovdqu %xmm12, (2*(\base) + \offset + 0*96)(%rdi)
	vmovdqu %xmm13, (2*(\base) + \offset + 1*96)(%rdi)
	vmovdqu %xmm14, (2*(\base) + \offset + 2*96)(%rdi)
	vmovdqu %xmm15, (2*(\base) + \offset + 3*96)(%rdi)

	vpunpcklqdq %xmm10, %xmm6, %xmm12
	vpunpckhqdq %xmm10, %xmm6, %xmm13
	vmovdqu %xmm12, (2*(\base) + \offset + 4*96)(%rdi)
	vmovdqu %xmm13, (2*(\base) + \offset + 5*96)(%rdi)
.endm

.macro TRANSPOSE_STORE16_6 base
	TRANSPOSE_STORE8_6 \base, 0
	TRANSPOSE_STORE8_6 "(\base + 8)", 0
.endm

.macro INV_STACK_PAIR coeff step qreg zreg off
	vmovdqu (32*(\coeff))(%rsp), %ymm4
	vmovdqu (32*(\coeff + \step))(%rsp), %ymm5
	vpaddw %ymm5, %ymm4, %ymm6
	CENTER16 %ymm6
	vpsubw %ymm4, %ymm5, %ymm7
	MONT16 %ymm7, \off(\qreg), \off(\zreg), %ymm7
	CENTER16 %ymm7
	vmovdqu %ymm6, (32*(\coeff))(%rsp)
	vmovdqu %ymm7, (32*(\coeff + \step))(%rsp)
.endm

.macro INV_STACK_PAIR_ZREG coeff step
	vmovdqu (32*(\coeff))(%rsp), %ymm4
	vmovdqu (32*(\coeff + \step))(%rsp), %ymm5
	vpaddw %ymm5, %ymm4, %ymm6
	CENTER16 %ymm6
	vpsubw %ymm4, %ymm5, %ymm7
	MONT16_REG %ymm7, %ymm8, %ymm9, %ymm7
	CENTER16 %ymm7
	vmovdqu %ymm6, (32*(\coeff))(%rsp)
	vmovdqu %ymm7, (32*(\coeff + \step))(%rsp)
.endm

.macro INV_STACK_STAGE3
	vmovdqa 0(%r12), %ymm8
	vmovdqa 0(%r13), %ymm9
	.irp c,0,1,2
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 32(%r12), %ymm8
	vmovdqa 32(%r13), %ymm9
	.irp c,6,7,8
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 64(%r12), %ymm8
	vmovdqa 64(%r13), %ymm9
	.irp c,12,13,14
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 96(%r12), %ymm8
	vmovdqa 96(%r13), %ymm9
	.irp c,18,19,20
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 128(%r12), %ymm8
	vmovdqa 128(%r13), %ymm9
	.irp c,24,25,26
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 160(%r12), %ymm8
	vmovdqa 160(%r13), %ymm9
	.irp c,30,31,32
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 192(%r12), %ymm8
	vmovdqa 192(%r13), %ymm9
	.irp c,36,37,38
		INV_STACK_PAIR_ZREG \c, 3
	.endr
	vmovdqa 224(%r12), %ymm8
	vmovdqa 224(%r13), %ymm9
	.irp c,42,43,44
		INV_STACK_PAIR_ZREG \c, 3
	.endr
.endm

.macro INV_STACK_STAGE6
	vmovdqa 0(%r14), %ymm8
	vmovdqa 0(%r15), %ymm9
	.irp c,0,1,2,3,4,5
		INV_STACK_PAIR_ZREG \c, 6
	.endr
	vmovdqa 32(%r14), %ymm8
	vmovdqa 32(%r15), %ymm9
	.irp c,12,13,14,15,16,17
		INV_STACK_PAIR_ZREG \c, 6
	.endr
	vmovdqa 64(%r14), %ymm8
	vmovdqa 64(%r15), %ymm9
	.irp c,24,25,26,27,28,29
		INV_STACK_PAIR_ZREG \c, 6
	.endr
	vmovdqa 96(%r14), %ymm8
	vmovdqa 96(%r15), %ymm9
	.irp c,36,37,38,39,40,41
		INV_STACK_PAIR_ZREG \c, 6
	.endr
.endm

.macro INV_STACK_STAGE12
	vmovdqa 0(%r8), %ymm8
	vmovdqa 0(%r9), %ymm9
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11
		INV_STACK_PAIR_ZREG \c, 12
	.endr
	vmovdqa 32(%r8), %ymm8
	vmovdqa 32(%r9), %ymm9
	.irp c,24,25,26,27,28,29,30,31,32,33,34,35
		INV_STACK_PAIR_ZREG \c, 12
	.endr
.endm

.macro INV_STACK_STAGE24
	vmovdqa 0(%r10), %ymm8
	vmovdqa 0(%r11), %ymm9
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
		INV_STACK_PAIR_ZREG \c, 24
	.endr
.endm

.macro INV_STACK_GROUP STORE
	vmovdqa oaep2592_16xq(%rip), %ymm0
	vmovdqa oaep2592_16xv(%rip), %ymm1
	vmovdqa oaep2592_16xhalf(%rip), %ymm2
	vmovdqa oaep2592_16xneghalf(%rip), %ymm3
	vmovdqa oaep2592_tail48_gather_idx(%rip), %ymm14
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15
		STACK_LOAD \c, \STORE
	.endr
	.irp c,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
		STACK_LOAD \c, \STORE
	.endr
	.irp c,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
		STACK_LOAD \c, \STORE
	.endr
	INV_STACK_STAGE3
	INV_STACK_STAGE6
	INV_STACK_STAGE12
	INV_STACK_STAGE24
	.if \STORE == 8
		TRANSPOSE_STORE16_8 0
		TRANSPOSE_STORE16_8 16
		TRANSPOSE_STORE16_8 32
	.else
		TRANSPOSE_STORE16_6 0
		TRANSPOSE_STORE16_6 16
		TRANSPOSE_STORE16_6 32
	.endif
.endm

.macro INV_BODY c0qmem c0mem c1qmem c1mem
	pushq %rbx
	pushq %r12
	pushq %r13
	pushq %r14
	pushq %r15
	subq $1536, %rsp
	movq %rdi, %rbx
	vmovdqa oaep2592_16xq(%rip), %ymm0
	vmovdqa oaep2592_16xv(%rip), %ymm1
	vmovdqa oaep2592_16xhalf(%rip), %ymm2
	vmovdqa oaep2592_16xneghalf(%rip), %ymm3

	leaq oaep2592_inv_head_z3_qinv(%rip), %r12
	leaq oaep2592_inv_head_z3_16(%rip), %r13
	leaq oaep2592_inv_head_z6_qinv(%rip), %r14
	leaq oaep2592_inv_head_z6_16(%rip), %r15
	leaq oaep2592_inv_head_z12_qinv(%rip), %r8
	leaq oaep2592_inv_head_z12_16(%rip), %r9
	leaq oaep2592_inv_head_z24_qinv(%rip), %r10
	leaq oaep2592_inv_head_z24_16(%rip), %r11
	movl $6, %eax
.p2align 5
L_inv_head_loop8_\@:
	INV_STACK_GROUP 8
	addq $768, %rsi
	addq $768, %rdi
	addq $256, %r12
	addq $256, %r13
	addq $128, %r14
	addq $128, %r15
	addq $64, %r8
	addq $64, %r9
	addq $32, %r10
	addq $32, %r11
	decl %eax
	jnz L_inv_head_loop8_\@
	INV_STACK_GROUP 6

	movq %rbx, %rdi
	vmovdqa oaep2592_16xq(%rip), %ymm0
	vmovdqa oaep2592_16xhalf(%rip), %ymm2
	vmovdqa oaep2592_16xneghalf(%rip), %ymm3
	leaq oaep2592_middle_qinv+1664(%rip), %r8
	leaq oaep2592_middle_16+1664(%rip), %r9
	movq $96, %r10
	movq $192, %r11
	movq %rdi, %rcx
	leaq 5184(%rdi), %rsi
.p2align 5
L_inv_step48_group_\@:
	movq %rcx, %rax
	leaq 96(%rcx), %rdx
	vmovdqa (%r8), %ymm14
	vmovdqa (%r9), %ymm1
.p2align 5
L_inv_step48_loop_\@:
	INV_R3_16_CHUNK
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_inv_step48_loop_\@
	addq $288, %rcx
	subq $64, %r8
	subq $64, %r9
	cmpq %rsi, %rcx
	jb L_inv_step48_group_\@

	leaq oaep2592_middle_qinv+512(%rip), %r8
	leaq oaep2592_middle_16+512(%rip), %r9
	movq $288, %r10
	movq $576, %r11
	movq %rdi, %rcx
	leaq 5184(%rdi), %rsi
.p2align 5
L_inv_step144_group_\@:
	movq %rcx, %rax
	leaq 288(%rcx), %rdx
	vmovdqa (%r8), %ymm14
	vmovdqa (%r9), %ymm1
.p2align 5
L_inv_step144_loop_\@:
	INV_R3_16_CHUNK
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_inv_step144_loop_\@
	addq $864, %rcx
	subq $64, %r8
	subq $64, %r9
	cmpq %rsi, %rcx
	jb L_inv_step144_group_\@

	leaq oaep2592_middle_qinv+128(%rip), %r8
	leaq oaep2592_middle_16+128(%rip), %r9
	movq $864, %r10
	movq $1728, %r11
	movq %rdi, %rcx
	leaq 5184(%rdi), %rsi
.p2align 5
L_inv_step432_group_\@:
	movq %rcx, %rax
	leaq 864(%rcx), %rdx
	vmovdqa (%r8), %ymm14
	vmovdqa (%r9), %ymm1
.p2align 5
L_inv_step432_loop_\@:
	INV_R3_16_CHUNK
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_inv_step432_loop_\@
	addq $2592, %rcx
	subq $64, %r8
	subq $64, %r9
	cmpq %rsi, %rcx
	jb L_inv_step432_group_\@

	xorl %eax, %eax
	vmovdqa oaep2592_16xfinal_z_qinv(%rip), %ymm1
	vmovdqa oaep2592_16xfinal_z(%rip), %ymm6
	vmovdqa \c0qmem(%rip), %ymm13
	vmovdqa \c0mem(%rip), %ymm14
.p2align 5
L_inv_final_loop_\@:
	vmovdqu       (%rdi,%rax), %ymm4
	vmovdqu  2592(%rdi,%rax), %ymm5
	vpaddw %ymm5, %ymm4, %ymm7
	CENTER16 %ymm7
	vpsubw %ymm5, %ymm4, %ymm8
	MONT16_REG %ymm8, %ymm1, %ymm6, %ymm8
	vpsubw %ymm8, %ymm7, %ymm9
	MONT16_REG %ymm9, %ymm13, %ymm14, %ymm9
	MONT16 %ymm8, \c1qmem(%rip), \c1mem(%rip), %ymm5
	vmovdqu %ymm9,       (%rdi,%rax)
	vmovdqu %ymm5, 2592(%rdi,%rax)
	addq $32, %rax
	cmpq $2592, %rax
	jb L_inv_final_loop_\@

	vzeroupper
	addq $1536, %rsp
	popq %r15
	popq %r14
	popq %r13
	popq %r12
	popq %rbx
	ret
.endm

.global invntt
.type invntt, @function
invntt:
	INV_BODY oaep2592_16xfinal_inv_c0_qinv, oaep2592_16xfinal_inv_c0, oaep2592_16xfinal_inv_c1_qinv, oaep2592_16xfinal_inv_c1
.size invntt, .-invntt

.global invntt_normalized
.type invntt_normalized, @function
invntt_normalized:
	INV_BODY oaep2592_16xfinal_norm_c0_qinv, oaep2592_16xfinal_norm_c0, oaep2592_16xfinal_norm_c1_qinv, oaep2592_16xfinal_norm_c1
.size invntt_normalized, .-invntt_normalized

.section .note.GNU-stack,"",@progbits
