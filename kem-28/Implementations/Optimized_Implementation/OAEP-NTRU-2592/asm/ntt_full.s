.text

.macro MONT32 reg
	vpmulld oaep2592_8xqinv(%rip), \reg, %ymm15
	vpand   oaep2592_8xmask(%rip), %ymm15, %ymm15
	vpslld  $16, %ymm15, %ymm15
	vpsrad  $16, %ymm15, %ymm15
	vpmulld oaep2592_8xq(%rip), %ymm15, %ymm14
	vpsubd  %ymm14, \reg, \reg
	vpsrad  $16, \reg, \reg
.endm

.macro FQMUL32 src mul out
	vpmulld \mul, \src, \out
	MONT32 \out
.endm

.macro BARRETT32 reg
	vpmulld oaep2592_8xbarrett_v(%rip), \reg, %ymm15
	vpaddd  oaep2592_8xbarrett_half(%rip), %ymm15, %ymm15
	vpsrad  $27, %ymm15, %ymm15
	vpmulld oaep2592_8xq(%rip), %ymm15, %ymm15
	vpsubd  %ymm15, \reg, \reg
.endm

.macro STORE8_I32_I16 reg mem
	vpackssdw \reg, \reg, %ymm15
	vpermq $0x08, %ymm15, %ymm15
	vmovdqu %xmm15, \mem
.endm

.macro FWD_R3_32
	FQMUL32 %ymm5, %ymm2, %ymm10
	FQMUL32 %ymm6, %ymm3, %ymm11
	vpsubd  %ymm11, %ymm10, %ymm12
	FQMUL32 %ymm12, %ymm13, %ymm12

	vmovdqa %ymm4, %ymm7
	vpaddd  %ymm10, %ymm7, %ymm7
	vpaddd  %ymm11, %ymm7, %ymm7
	BARRETT32 %ymm7

	vmovdqa %ymm4, %ymm8
	vpsubd  %ymm11, %ymm8, %ymm8
	vpaddd  %ymm12, %ymm8, %ymm8
	BARRETT32 %ymm8

	vmovdqa %ymm4, %ymm9
	vpsubd  %ymm10, %ymm9, %ymm9
	vpsubd  %ymm12, %ymm9, %ymm9
	BARRETT32 %ymm9
.endm

.macro FWD_R3_CHUNK
	vpmovsxwd       (%rax), %ymm4
	vpmovsxwd  (%rax,%r10), %ymm5
	vpmovsxwd  (%rax,%r11), %ymm6
	FWD_R3_32
	STORE8_I32_I16 %ymm7,       "(%rax)"
	STORE8_I32_I16 %ymm8,  "(%rax,%r10)"
	STORE8_I32_I16 %ymm9,  "(%rax,%r11)"
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

.macro BARRETT16 reg
	vpmulhw %ymm1, \reg, %ymm15
	vpsraw  $11, %ymm15, %ymm15
	vpmullw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \reg, \reg
	vpcmpgtw oaep2592_16xhalf(%rip), \reg, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpsubw   %ymm15, \reg, \reg
	vmovdqa  oaep2592_16xneghalf(%rip), %ymm2
	vpcmpgtw \reg, %ymm2, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpaddw   %ymm15, \reg, \reg
.endm

.macro CENTER16 reg
	vpcmpgtw oaep2592_16xhalf(%rip), \reg, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpsubw   %ymm15, \reg, \reg
	vpcmpgtw \reg, %ymm13, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpaddw   %ymm15, \reg, \reg
.endm

.macro R3_16_STORE_LO
	vpmovsxwd %xmm4, %ymm5
	vpmovsxwd %xmm7, %ymm6
	vpmovsxwd %xmm8, %ymm10
	vpmovsxwd %xmm9, %ymm11

	vmovdqa %ymm5, %ymm12
	vpaddd  %ymm6, %ymm12, %ymm12
	vpaddd  %ymm10, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "(%rax)"

	vmovdqa %ymm5, %ymm12
	vpsubd  %ymm10, %ymm12, %ymm12
	vpaddd  %ymm11, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "(%rax,%r10)"

	vmovdqa %ymm5, %ymm12
	vpsubd  %ymm6, %ymm12, %ymm12
	vpsubd  %ymm11, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "(%rax,%r11)"
.endm

.macro R3_16_STORE_HI
	vextracti128 $1, %ymm4, %xmm4
	vextracti128 $1, %ymm7, %xmm7
	vextracti128 $1, %ymm8, %xmm8
	vextracti128 $1, %ymm9, %xmm9

	vpmovsxwd %xmm4, %ymm5
	vpmovsxwd %xmm7, %ymm6
	vpmovsxwd %xmm8, %ymm10
	vpmovsxwd %xmm9, %ymm11

	vmovdqa %ymm5, %ymm12
	vpaddd  %ymm6, %ymm12, %ymm12
	vpaddd  %ymm10, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "16(%rax)"

	vmovdqa %ymm5, %ymm12
	vpsubd  %ymm10, %ymm12, %ymm12
	vpaddd  %ymm11, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "16(%rax,%r10)"

	vmovdqa %ymm5, %ymm12
	vpsubd  %ymm6, %ymm12, %ymm12
	vpsubd  %ymm11, %ymm12, %ymm12
	BARRETT32 %ymm12
	STORE8_I32_I16 %ymm12, "16(%rax,%r11)"
.endm

.macro FWD_R3_16_CHUNK
	vmovdqu       (%rax), %ymm4
	vmovdqu  (%rax,%r10), %ymm5
	vmovdqu  (%rax,%r11), %ymm6

	MONT16 %ymm5,  0(%r8),  0(%r9), %ymm7
	CENTER16 %ymm7
	MONT16 %ymm6, 32(%r8), 32(%r9), %ymm8
	CENTER16 %ymm8
	vpsubw %ymm8, %ymm7, %ymm9
	MONT16 %ymm9, oaep2592_16xomega_qinv(%rip), oaep2592_16xomega(%rip), %ymm9
	CENTER16 %ymm9

	R3_16_STORE_LO
	R3_16_STORE_HI
.endm

.macro LOAD_TAIL8 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*96)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*96)(%rsi), %xmm15, %xmm15
	vpinsrw $2, (2*(\coeff) + 2*96)(%rsi), %xmm15, %xmm15
	vpinsrw $3, (2*(\coeff) + 3*96)(%rsi), %xmm15, %xmm15
	vpinsrw $4, (2*(\coeff) + 4*96)(%rsi), %xmm15, %xmm15
	vpinsrw $5, (2*(\coeff) + 5*96)(%rsi), %xmm15, %xmm15
	vpinsrw $6, (2*(\coeff) + 6*96)(%rsi), %xmm15, %xmm15
	vpinsrw $7, (2*(\coeff) + 7*96)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro LOAD_TAIL6 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*96)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*96)(%rsi), %xmm15, %xmm15
	vpinsrw $2, (2*(\coeff) + 2*96)(%rsi), %xmm15, %xmm15
	vpinsrw $3, (2*(\coeff) + 3*96)(%rsi), %xmm15, %xmm15
	vpinsrw $4, (2*(\coeff) + 4*96)(%rsi), %xmm15, %xmm15
	vpinsrw $5, (2*(\coeff) + 5*96)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro STORE_TAIL8 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*96)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*96)(%rsi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*96)(%rsi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*96)(%rsi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*96)(%rsi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*96)(%rsi)
	vpextrw $6, %xmm14, (2*(\coeff) + 6*96)(%rsi)
	vpextrw $7, %xmm14, (2*(\coeff) + 7*96)(%rsi)
.endm

.macro STORE_TAIL6 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*96)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*96)(%rsi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*96)(%rsi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*96)(%rsi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*96)(%rsi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*96)(%rsi)
.endm

.macro TAIL_LOAD_STACK coeff STORE
	LOAD_TAIL\STORE \coeff, %ymm4
	vmovdqu %ymm4, (32*(\coeff))(%rsp)
.endm

.macro TAIL_STORE_STACK coeff STORE
	vmovdqu (32*(\coeff))(%rsp), %ymm4
	STORE_TAIL\STORE \coeff, %ymm4
.endm

.macro TAIL_STACK_PAIR coeff step zqreg zreg zoff
	vmovdqu (32*(\coeff))(%rsp), %ymm4
	vmovdqu (32*(\coeff + \step))(%rsp), %ymm5
	MONT16 %ymm5, \zoff(\zqreg), \zoff(\zreg), %ymm5
	CENTER16 %ymm5
	vmovdqa %ymm4, %ymm6
	vpaddw %ymm5, %ymm6, %ymm6
	CENTER16 %ymm6
	vmovdqa %ymm4, %ymm7
	vpsubw %ymm5, %ymm7, %ymm7
	CENTER16 %ymm7
	vmovdqu %ymm6, (32*(\coeff))(%rsp)
	vmovdqu %ymm7, (32*(\coeff + \step))(%rsp)
.endm

.macro TAIL_STACK_LOAD_ALL STORE
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
		TAIL_LOAD_STACK \c, \STORE
	.endr
.endm

.macro TAIL_STACK_STORE_ALL STORE
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47
		TAIL_STORE_STACK \c, \STORE
	.endr
.endm

.macro TAIL_STACK_STAGE24
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23
		TAIL_STACK_PAIR \c, 24, %r8, %r9, 0
	.endr
.endm

.macro TAIL_STACK_STAGE12
	.irp c,0,1,2,3,4,5,6,7,8,9,10,11
		TAIL_STACK_PAIR \c, 12, %r10, %r11, 0
	.endr
	.irp c,24,25,26,27,28,29,30,31,32,33,34,35
		TAIL_STACK_PAIR \c, 12, %r10, %r11, 32
	.endr
.endm

.macro TAIL_STACK_STAGE6
	.irp c,0,1,2,3,4,5
		TAIL_STACK_PAIR \c, 6, %rcx, %rdx, 0
	.endr
	.irp c,12,13,14,15,16,17
		TAIL_STACK_PAIR \c, 6, %rcx, %rdx, 32
	.endr
	.irp c,24,25,26,27,28,29
		TAIL_STACK_PAIR \c, 6, %rcx, %rdx, 64
	.endr
	.irp c,36,37,38,39,40,41
		TAIL_STACK_PAIR \c, 6, %rcx, %rdx, 96
	.endr
.endm

.macro TAIL_STACK_STAGE3
	.irp c,0,1,2
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 0
	.endr
	.irp c,6,7,8
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 32
	.endr
	.irp c,12,13,14
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 64
	.endr
	.irp c,18,19,20
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 96
	.endr
	.irp c,24,25,26
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 128
	.endr
	.irp c,30,31,32
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 160
	.endr
	.irp c,36,37,38
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 192
	.endr
	.irp c,42,43,44
		TAIL_STACK_PAIR \c, 3, %r12, %r13, 224
	.endr
.endm

.macro TAIL_STACK_GROUP STORE
	TAIL_STACK_LOAD_ALL \STORE
	TAIL_STACK_STAGE24
	TAIL_STACK_STAGE12
	TAIL_STACK_STAGE6
	TAIL_STACK_STAGE3
	TAIL_STACK_STORE_ALL \STORE
.endm

.global ntt
.type ntt, @function
ntt:
	pushq %r12
	pushq %r13
	subq $1568, %rsp
	vmovdqa oaep2592_8xomega(%rip), %ymm13

	xorl %eax, %eax
.p2align 5
L_ntt_first_loop8:
	vpmovsxwd       (%rsi,%rax), %ymm4
	vpmovsxwd   864(%rsi,%rax), %ymm5
	vpmovsxwd  1728(%rsi,%rax), %ymm6
	vpmovsxwd  2592(%rsi,%rax), %ymm7
	vpmovsxwd  3456(%rsi,%rax), %ymm8
	vpmovsxwd  4320(%rsi,%rax), %ymm9

	vpbroadcastd oaep2592_first_zetas32+4(%rip), %ymm2
	FQMUL32 %ymm7, %ymm2, %ymm10
	FQMUL32 %ymm8, %ymm2, %ymm11
	FQMUL32 %ymm9, %ymm2, %ymm12

	vpaddd %ymm10, %ymm4, %ymm14
	BARRETT32 %ymm14
	vpaddd %ymm7, %ymm4, %ymm7
	vpsubd %ymm10, %ymm7, %ymm7
	BARRETT32 %ymm7
	vmovdqa %ymm14, %ymm4

	vpaddd %ymm11, %ymm5, %ymm14
	BARRETT32 %ymm14
	vpaddd %ymm8, %ymm5, %ymm8
	vpsubd %ymm11, %ymm8, %ymm8
	BARRETT32 %ymm8
	vmovdqa %ymm14, %ymm5

	vpaddd %ymm12, %ymm6, %ymm14
	BARRETT32 %ymm14
	vpaddd %ymm9, %ymm6, %ymm9
	vpsubd %ymm12, %ymm9, %ymm9
	BARRETT32 %ymm9
	vmovdqa %ymm14, %ymm6

	vmovdqa %ymm7, %ymm0
	vmovdqa %ymm8, %ymm1
	vmovdqu %ymm9, 1536(%rsp)

	vpbroadcastd oaep2592_first_zetas32+8(%rip), %ymm2
	vpbroadcastd oaep2592_first_zetas32+12(%rip), %ymm3
	FWD_R3_32
	STORE8_I32_I16 %ymm7,       "(%rdi,%rax)"
	STORE8_I32_I16 %ymm8,   "864(%rdi,%rax)"
	STORE8_I32_I16 %ymm9,  "1728(%rdi,%rax)"

	vmovdqa %ymm0, %ymm4
	vmovdqa %ymm1, %ymm5
	vmovdqu 1536(%rsp), %ymm6
	vpbroadcastd oaep2592_first_zetas32+16(%rip), %ymm2
	vpbroadcastd oaep2592_first_zetas32+20(%rip), %ymm3
	FWD_R3_32
	STORE8_I32_I16 %ymm7,  "2592(%rdi,%rax)"
	STORE8_I32_I16 %ymm8,  "3456(%rdi,%rax)"
	STORE8_I32_I16 %ymm9,  "4320(%rdi,%rax)"

	addq $16, %rax
	cmpq $864, %rax
	jb L_ntt_first_loop8

	vmovdqa oaep2592_16xq(%rip), %ymm0
	vmovdqa oaep2592_16xv(%rip), %ymm1
	vmovdqa oaep2592_16xneghalf(%rip), %ymm13
	leaq oaep2592_middle_qinv+192(%rip), %r8
	leaq oaep2592_middle_16+192(%rip), %r9
	movq $288, %r10
	movq $576, %r11
	movq %rdi, %rcx
	leaq 5184(%rdi), %rsi
.p2align 5
L_ntt_step144_group:
	movq %rcx, %rax
	leaq 288(%rcx), %rdx
.p2align 5
L_ntt_step144_loop:
	FWD_R3_16_CHUNK
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_ntt_step144_loop
	addq $864, %rcx
	addq $64, %r8
	addq $64, %r9
	cmpq %rsi, %rcx
	jb L_ntt_step144_group

	leaq oaep2592_middle_qinv+576(%rip), %r8
	leaq oaep2592_middle_16+576(%rip), %r9
	movq $96, %r10
	movq $192, %r11
	movq %rdi, %rcx
	leaq 5184(%rdi), %rsi
.p2align 5
L_ntt_step48_group:
	movq %rcx, %rax
	leaq 96(%rcx), %rdx
.p2align 5
L_ntt_step48_loop:
	FWD_R3_16_CHUNK
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_ntt_step48_loop
	addq $288, %rcx
	addq $64, %r8
	addq $64, %r9
	cmpq %rsi, %rcx
	jb L_ntt_step48_group

	vmovdqa oaep2592_16xq(%rip), %ymm0
	vmovdqa oaep2592_16xv(%rip), %ymm1
	leaq oaep2592_ntt_tail_z24_qinv(%rip), %r8
	leaq oaep2592_ntt_tail_z24_16(%rip), %r9
	leaq oaep2592_ntt_tail_z12_qinv(%rip), %r10
	leaq oaep2592_ntt_tail_z12_16(%rip), %r11
	leaq oaep2592_ntt_tail_z6_qinv(%rip), %rcx
	leaq oaep2592_ntt_tail_z6_16(%rip), %rdx
	leaq oaep2592_ntt_tail_z3_qinv(%rip), %r12
	leaq oaep2592_ntt_tail_z3_16(%rip), %r13
	movq %rdi, %rsi
	movl $6, %eax
.p2align 5
L_ntt_tail_stack_loop8:
	TAIL_STACK_GROUP 8
	addq $768, %rsi
	addq $32, %r8
	addq $32, %r9
	addq $64, %r10
	addq $64, %r11
	addq $128, %rcx
	addq $128, %rdx
	addq $256, %r12
	addq $256, %r13
	decl %eax
	jnz L_ntt_tail_stack_loop8
	TAIL_STACK_GROUP 6

	vzeroupper
	addq $1568, %rsp
	popq %r13
	popq %r12
	ret

	.size ntt, .-ntt

.section .note.GNU-stack,"",@progbits
