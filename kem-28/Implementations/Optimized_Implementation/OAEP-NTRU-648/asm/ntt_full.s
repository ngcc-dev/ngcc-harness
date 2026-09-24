.text

.macro MONT_Y src qpair zpair out
	vpbroadcastd \qpair, %ymm2
	vpbroadcastd \zpair, %ymm3
	vpmullw %ymm2, \src, %ymm15
	vpmulhw %ymm3, \src, \out
	vpmulhw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \out, \out
.endm

.macro MONT_Y_VEC src zq z out
	vpmullw \zq, \src, %ymm15
	vpmulhw \z,  \src, \out
	vpmulhw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \out, \out
.endm

.macro BARRETT_Y reg
	vpmulhw %ymm1, \reg, %ymm15
	vpsraw  $11, %ymm15, %ymm15
	vpmullw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \reg, \reg
	vpcmpgtw oaep648_16xhalf(%rip), \reg, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpsubw   %ymm15, \reg, \reg
	vmovdqa  oaep648_16xneghalf(%rip), %ymm2
	vpcmpgtw \reg, %ymm2, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpaddw   %ymm15, \reg, \reg
.endm

.macro FWD_R3_Y
	MONT_Y     %ymm5, (%r8), 4(%r8), %ymm10
	MONT_Y     %ymm6, (%r9), 4(%r9), %ymm11
	vpsubw     %ymm11, %ymm10, %ymm12
	MONT_Y_VEC %ymm12, %ymm13, %ymm14, %ymm12
	vpaddw     %ymm10, %ymm4, %ymm7
	vpaddw     %ymm11, %ymm7, %ymm7
	vpsubw     %ymm11, %ymm4, %ymm8
	vpaddw     %ymm12, %ymm8, %ymm8
	vpsubw     %ymm10, %ymm4, %ymm9
	vpsubw     %ymm12, %ymm9, %ymm9
.endm

.macro STORE_R3_16
	vmovdqu %ymm7,  (%rax)
	vmovdqu %ymm8,  (%rax,%r10)
	vmovdqu %ymm9,  (%rax,%r11)
.endm

.macro STORE_R3_8
	vmovdqu %xmm7,  (%rax)
	vmovdqu %xmm8,  (%rax,%r10)
	vmovdqu %xmm9,  (%rax,%r11)
.endm

.macro STORE_R3_4
	vmovq %xmm7,  (%rax)
	vmovq %xmm8,  (%rax,%r10)
	vmovq %xmm9,  (%rax,%r11)
.endm

.macro LOAD_R3_16
	vmovdqu       (%rax), %ymm4
	vmovdqu  (%rax,%r10), %ymm5
	vmovdqu  (%rax,%r11), %ymm6
.endm

.macro LOAD_R3_8
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vpxor %ymm6, %ymm6, %ymm6
	vmovdqu       (%rax), %xmm4
	vmovdqu  (%rax,%r10), %xmm5
	vmovdqu  (%rax,%r11), %xmm6
.endm

.macro LOAD_R3_4
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vpxor %ymm6, %ymm6, %ymm6
	vmovq       (%rax), %xmm4
	vmovq  (%rax,%r10), %xmm5
	vmovq  (%rax,%r11), %xmm6
.endm

.macro FIRST_LOAD_16 b0 b1 b2
	vmovdqu      (%rsi,%rax), %ymm4
	vmovdqu  216(%rsi,%rax), %ymm5
	vmovdqu  432(%rsi,%rax), %ymm6
	vmovdqu \b0(%rsi,%rax), %ymm7
	vmovdqu \b1(%rsi,%rax), %ymm8
	vmovdqu \b2(%rsi,%rax), %ymm9
.endm

.macro FIRST_LOAD_8 b0 b1 b2
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vpxor %ymm6, %ymm6, %ymm6
	vpxor %ymm7, %ymm7, %ymm7
	vpxor %ymm8, %ymm8, %ymm8
	vpxor %ymm9, %ymm9, %ymm9
	vmovdqu      (%rsi,%rax), %xmm4
	vmovdqu  216(%rsi,%rax), %xmm5
	vmovdqu  432(%rsi,%rax), %xmm6
	vmovdqu \b0(%rsi,%rax), %xmm7
	vmovdqu \b1(%rsi,%rax), %xmm8
	vmovdqu \b2(%rsi,%rax), %xmm9
.endm

.macro FIRST_LOAD_4 b0 b1 b2
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vpxor %ymm6, %ymm6, %ymm6
	vpxor %ymm7, %ymm7, %ymm7
	vpxor %ymm8, %ymm8, %ymm8
	vpxor %ymm9, %ymm9, %ymm9
	vmovq      (%rsi,%rax), %xmm4
	vmovq  216(%rsi,%rax), %xmm5
	vmovq  432(%rsi,%rax), %xmm6
	vmovq \b0(%rsi,%rax), %xmm7
	vmovq \b1(%rsi,%rax), %xmm8
	vmovq \b2(%rsi,%rax), %xmm9
.endm

.macro FIRST_LO STORE
	MONT_Y %ymm7, 8*1(%rdx), 8*1+4(%rdx), %ymm10
	MONT_Y %ymm8, 8*1(%rdx), 8*1+4(%rdx), %ymm11
	MONT_Y %ymm9, 8*1(%rdx), 8*1+4(%rdx), %ymm12
	vpaddw %ymm7, %ymm4, %ymm2
	vpaddw %ymm8, %ymm5, %ymm3
	vpaddw %ymm9, %ymm6, %ymm15
	vpsubw %ymm10, %ymm2, %ymm2
	vpsubw %ymm11, %ymm3, %ymm3
	vpsubw %ymm12, %ymm15, %ymm15
	vmovdqu %ymm2, (%rsp)
	vmovdqu %ymm3, 32(%rsp)
	vmovdqu %ymm15, 64(%rsp)
	vpaddw %ymm10, %ymm4, %ymm4
	vpaddw %ymm11, %ymm5, %ymm5
	vpaddw %ymm12, %ymm6, %ymm6
	leaq 8*2(%rdx), %r8
	leaq 8*3(%rdx), %r9
	FWD_R3_Y
	.if \STORE == 16
		vmovdqu %ymm7,      (%rdi,%rax)
		vmovdqu %ymm8,  216(%rdi,%rax)
		vmovdqu %ymm9,  432(%rdi,%rax)
	.elseif \STORE == 8
		vmovdqu %xmm7,      (%rdi,%rax)
		vmovdqu %xmm8,  216(%rdi,%rax)
		vmovdqu %xmm9,  432(%rdi,%rax)
	.else
		vmovq %xmm7,      (%rdi,%rax)
		vmovq %xmm8,  216(%rdi,%rax)
		vmovq %xmm9,  432(%rdi,%rax)
	.endif
.endm

.macro FIRST_HI STORE
	vmovdqu (%rsp), %ymm4
	vmovdqu 32(%rsp), %ymm5
	vmovdqu 64(%rsp), %ymm6
	leaq 8*4(%rdx), %r8
	leaq 8*5(%rdx), %r9
	FWD_R3_Y
	.if \STORE == 16
		vmovdqu %ymm7,  648(%rdi,%rax)
		vmovdqu %ymm8,  864(%rdi,%rax)
		vmovdqu %ymm9, 1080(%rdi,%rax)
	.elseif \STORE == 8
		vmovdqu %xmm7,  648(%rdi,%rax)
		vmovdqu %xmm8,  864(%rdi,%rax)
		vmovdqu %xmm9, 1080(%rdi,%rax)
	.else
		vmovq %xmm7,  648(%rdi,%rax)
		vmovq %xmm8,  864(%rdi,%rax)
		vmovq %xmm9, 1080(%rdi,%rax)
	.endif
.endm

.macro TAIL_MONT src zq z out
	vpmullw \zq, \src, %ymm15
	vpmulhw \z, \src, \out
	vpmulhw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \out, \out
.endm

.macro LOAD_TAIL8 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*24)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*24)(%rsi), %xmm15, %xmm15
	vpinsrw $2, (2*(\coeff) + 2*24)(%rsi), %xmm15, %xmm15
	vpinsrw $3, (2*(\coeff) + 3*24)(%rsi), %xmm15, %xmm15
	vpinsrw $4, (2*(\coeff) + 4*24)(%rsi), %xmm15, %xmm15
	vpinsrw $5, (2*(\coeff) + 5*24)(%rsi), %xmm15, %xmm15
	vpinsrw $6, (2*(\coeff) + 6*24)(%rsi), %xmm15, %xmm15
	vpinsrw $7, (2*(\coeff) + 7*24)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro LOAD_TAIL6 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*24)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*24)(%rsi), %xmm15, %xmm15
	vpinsrw $2, (2*(\coeff) + 2*24)(%rsi), %xmm15, %xmm15
	vpinsrw $3, (2*(\coeff) + 3*24)(%rsi), %xmm15, %xmm15
	vpinsrw $4, (2*(\coeff) + 4*24)(%rsi), %xmm15, %xmm15
	vpinsrw $5, (2*(\coeff) + 5*24)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro STORE_TAIL8 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*24)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*24)(%rsi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*24)(%rsi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*24)(%rsi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*24)(%rsi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*24)(%rsi)
	vpextrw $6, %xmm14, (2*(\coeff) + 6*24)(%rsi)
	vpextrw $7, %xmm14, (2*(\coeff) + 7*24)(%rsi)
.endm

.macro STORE_TAIL6 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*24)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*24)(%rsi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*24)(%rsi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*24)(%rsi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*24)(%rsi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*24)(%rsi)
.endm

.macro NTT_TAIL_ROW coeff STORE
	LOAD_TAIL\STORE \coeff, %ymm4
	LOAD_TAIL\STORE "(\coeff + 6)", %ymm5
	TAIL_MONT %ymm5, (%r8), (%r11), %ymm5
	vpaddw %ymm5, %ymm4, %ymm6
	vpsubw %ymm5, %ymm4, %ymm7

	LOAD_TAIL\STORE "(\coeff + 3)", %ymm8
	LOAD_TAIL\STORE "(\coeff + 9)", %ymm9
	TAIL_MONT %ymm9, (%r8), (%r11), %ymm9
	vpaddw %ymm9, %ymm8, %ymm10
	vpsubw %ymm9, %ymm8, %ymm11

	TAIL_MONT %ymm10, (%r9), (%rcx), %ymm10
	vpaddw %ymm10, %ymm6, %ymm4
	vpsubw %ymm10, %ymm6, %ymm5
	BARRETT_Y %ymm4
	BARRETT_Y %ymm5

	TAIL_MONT %ymm11, (%r10), (%rdx), %ymm11
	vpaddw %ymm11, %ymm7, %ymm8
	vpsubw %ymm11, %ymm7, %ymm9
	BARRETT_Y %ymm8
	BARRETT_Y %ymm9

	STORE_TAIL\STORE \coeff, %ymm4
	STORE_TAIL\STORE "(\coeff + 3)", %ymm5
	STORE_TAIL\STORE "(\coeff + 6)", %ymm8
	STORE_TAIL\STORE "(\coeff + 9)", %ymm9
.endm

.global ntt
.type ntt, @function
ntt:
	subq $96, %rsp
	vmovdqa oaep648_16xq(%rip), %ymm0
	vmovdqa oaep648_16xv(%rip), %ymm1
	vmovdqa oaep648_16xomega_qinv(%rip), %ymm13
	vmovdqa oaep648_16xomega(%rip), %ymm14
	leaq oaep648_zetas_pair(%rip), %rdx

	xorl %eax, %eax
.p2align 5
L_ntt_first_loop16:
	FIRST_LOAD_16 648, 864, 1080
	FIRST_LO 16
	FIRST_HI 16
	addq $32, %rax
	cmpq $192, %rax
	jb L_ntt_first_loop16

	FIRST_LOAD_8 648, 864, 1080
	FIRST_LO 8
	FIRST_HI 8
	addq $16, %rax

	FIRST_LOAD_4 648, 864, 1080
	FIRST_LO 4
	FIRST_HI 4

	movq $72, %r10
	movq $144, %r11
	leaq 8*6(%rdx), %r8
	leaq 8*7(%rdx), %r9
	movq %rdi, %rcx
	leaq 1296(%rdi), %rsi
.p2align 5
L_ntt_step36_group:
	movq %rcx, %rax
	leaq 64(%rcx), %rdx
.p2align 5
L_ntt_step36_loop16:
	LOAD_R3_16
	FWD_R3_Y
	STORE_R3_16
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_ntt_step36_loop16
	LOAD_R3_4
	FWD_R3_Y
	STORE_R3_4
	addq $216, %rcx
	addq $16, %r8
	addq $16, %r9
	cmpq %rsi, %rcx
	jb L_ntt_step36_group

	leaq oaep648_zetas_pair(%rip), %rdx
	movq $24, %r10
	movq $48, %r11
	leaq 8*18(%rdx), %r8
	leaq 8*19(%rdx), %r9
	movq %rdi, %rcx
	leaq 1296(%rdi), %rsi
.p2align 5
L_ntt_step12_group:
	movq %rcx, %rax
	LOAD_R3_8
	FWD_R3_Y
	BARRETT_Y %ymm7
	BARRETT_Y %ymm8
	BARRETT_Y %ymm9
	STORE_R3_8
	addq $16, %rax
	LOAD_R3_4
	FWD_R3_Y
	BARRETT_Y %ymm7
	BARRETT_Y %ymm8
	BARRETT_Y %ymm9
	STORE_R3_4
	addq $72, %rcx
	addq $16, %r8
	addq $16, %r9
	cmpq %rsi, %rcx
	jb L_ntt_step12_group

	leaq oaep648_ntt_tail_z6_qinv(%rip), %r8
	leaq oaep648_ntt_tail_z3_lo_qinv(%rip), %r9
	leaq oaep648_ntt_tail_z3_hi_qinv(%rip), %r10
	leaq oaep648_ntt_tail_z6_16(%rip), %r11
	leaq oaep648_ntt_tail_z3_lo_16(%rip), %rcx
	leaq oaep648_ntt_tail_z3_hi_16(%rip), %rdx
	movq %rdi, %rsi
	movl $6, %eax

.p2align 5
L_ntt_tail_loop8:
	NTT_TAIL_ROW 0, 8
	NTT_TAIL_ROW 1, 8
	NTT_TAIL_ROW 2, 8

	addq $192, %rsi
	addq $32, %r8
	addq $32, %r9
	addq $32, %r10
	addq $32, %r11
	addq $32, %rcx
	addq $32, %rdx
	decl %eax
	jnz L_ntt_tail_loop8

	NTT_TAIL_ROW 0, 6
	NTT_TAIL_ROW 1, 6
	NTT_TAIL_ROW 2, 6

	vzeroupper
	addq $96, %rsp
	ret

.size ntt, .-ntt

.section .note.GNU-stack,"",@progbits
