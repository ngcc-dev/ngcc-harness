.text

.macro MONT_ZMEM src zmem out
	vpbroadcastw \zmem, %ymm3
	vpmullw oaep1296_16xqinv(%rip), %ymm3, %ymm2
	vpmullw %ymm2, \src, %ymm15
	vpmulhw %ymm3, \src, \out
	vpmulhw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \out, \out
.endm

.macro MONT_MEMVEC src zq z out
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
	vpcmpgtw oaep1296_16xhalf(%rip), \reg, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpsubw   %ymm15, \reg, \reg
	vmovdqa  oaep1296_16xneghalf(%rip), %ymm2
	vpcmpgtw \reg, %ymm2, %ymm15
	vpand    %ymm0, %ymm15, %ymm15
	vpaddw   %ymm15, \reg, \reg
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

.macro FWD_R3_DYN
	MONT_ZMEM  %ymm5, (%r9), %ymm10
	MONT_ZMEM  %ymm6, 2(%r9), %ymm11
	vpsubw     %ymm11, %ymm10, %ymm12
	MONT_MEMVEC %ymm12, oaep1296_16xomega_qinv(%rip), oaep1296_16xomega(%rip), %ymm12
	vpaddw     %ymm10, %ymm4, %ymm7
	vpaddw     %ymm11, %ymm7, %ymm7
	vpsubw     %ymm11, %ymm4, %ymm8
	vpaddw     %ymm12, %ymm8, %ymm8
	vpsubw     %ymm10, %ymm4, %ymm9
	vpsubw     %ymm12, %ymm9, %ymm9
	BARRETT_Y  %ymm7
	BARRETT_Y  %ymm8
	BARRETT_Y  %ymm9
.endm

.macro FWD_R3_LAYER stepb startz l16 tail8
	movq $\stepb, %r10
	movq $(2 * \stepb), %r11
	leaq zetas + 2 * \startz(%rip), %r9
	movq %rdi, %rcx
	leaq 2592(%rdi), %r8
.p2align 5
L_fwd_r3_group_\@:
	movq %rcx, %rax
	.if \l16
	movl $\l16, %edx
.p2align 5
L_fwd_r3_loop16_\@:
	LOAD_R3_16
	FWD_R3_DYN
	STORE_R3_16
	addq $32, %rax
	decl %edx
	jnz L_fwd_r3_loop16_\@
	.endif
	.if \tail8
	LOAD_R3_8
	FWD_R3_DYN
	STORE_R3_8
	.endif
	addq $(3 * \stepb), %rcx
	addq $4, %r9
	cmpq %r8, %rcx
	jb L_fwd_r3_group_\@
.endm

.macro HALF_LOAD_16
	vmovdqu       (%rsi,%rax), %ymm4
	vmovdqu  1296(%rsi,%rax), %ymm5
.endm

.macro HALF_LOAD_8
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vmovdqu       (%rsi,%rax), %xmm4
	vmovdqu  1296(%rsi,%rax), %xmm5
.endm

.macro HALF_STORE_16
	vmovdqu %ymm7,       (%rdi,%rax)
	vmovdqu %ymm8,  1296(%rdi,%rax)
.endm

.macro HALF_STORE_8
	vmovdqu %xmm7,       (%rdi,%rax)
	vmovdqu %xmm8,  1296(%rdi,%rax)
.endm

.macro HALF_STEP
	MONT_ZMEM %ymm5, zetas + 2(%rip), %ymm10
	vpaddw %ymm10, %ymm4, %ymm7
	vpaddw %ymm5,  %ymm4, %ymm8
	vpsubw %ymm10, %ymm8, %ymm8
	BARRETT_Y %ymm7
	BARRETT_Y %ymm8
.endm

.macro TAIL_MONT src zq z out
	vpmullw \zq, \src, %ymm15
	vpmulhw \z,  \src, \out
	vpmulhw %ymm0, %ymm15, %ymm15
	vpsubw  %ymm15, \out, \out
.endm

.macro LOAD_TAIL8 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*16)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*16)(%rsi), %xmm15, %xmm15
	vpinsrw $2, (2*(\coeff) + 2*16)(%rsi), %xmm15, %xmm15
	vpinsrw $3, (2*(\coeff) + 3*16)(%rsi), %xmm15, %xmm15
	vpinsrw $4, (2*(\coeff) + 4*16)(%rsi), %xmm15, %xmm15
	vpinsrw $5, (2*(\coeff) + 5*16)(%rsi), %xmm15, %xmm15
	vpinsrw $6, (2*(\coeff) + 6*16)(%rsi), %xmm15, %xmm15
	vpinsrw $7, (2*(\coeff) + 7*16)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro LOAD_TAIL2 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*16)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*16)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro STORE_TAIL8 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*16)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*16)(%rsi)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*16)(%rsi)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*16)(%rsi)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*16)(%rsi)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*16)(%rsi)
	vpextrw $6, %xmm14, (2*(\coeff) + 6*16)(%rsi)
	vpextrw $7, %xmm14, (2*(\coeff) + 7*16)(%rsi)
.endm

.macro STORE_TAIL2 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*16)(%rsi)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*16)(%rsi)
.endm

.macro NTT_TAIL_ROW coeff lanes
	LOAD_TAIL\lanes \coeff, %ymm4
	LOAD_TAIL\lanes "(\coeff + 4)", %ymm5
	TAIL_MONT %ymm5, (%r8), (%r11), %ymm5
	vpaddw %ymm5, %ymm4, %ymm6
	vpsubw %ymm5, %ymm4, %ymm7

	LOAD_TAIL\lanes "(\coeff + 2)", %ymm8
	LOAD_TAIL\lanes "(\coeff + 6)", %ymm9
	TAIL_MONT %ymm9, (%r8), (%r11), %ymm9
	vpaddw %ymm9, %ymm8, %ymm10
	vpsubw %ymm9, %ymm8, %ymm12

	TAIL_MONT %ymm10, (%r9), (%rcx), %ymm10
	vpaddw %ymm10, %ymm6, %ymm4
	vpsubw %ymm10, %ymm6, %ymm5
	BARRETT_Y %ymm4
	BARRETT_Y %ymm5

	TAIL_MONT %ymm12, (%r10), (%rdx), %ymm12
	vpaddw %ymm12, %ymm7, %ymm8
	vpsubw %ymm12, %ymm7, %ymm9
	BARRETT_Y %ymm8
	BARRETT_Y %ymm9

	STORE_TAIL\lanes \coeff, %ymm4
	STORE_TAIL\lanes "(\coeff + 2)", %ymm5
	STORE_TAIL\lanes "(\coeff + 4)", %ymm8
	STORE_TAIL\lanes "(\coeff + 6)", %ymm9
.endm

.global ntt
.type ntt, @function
ntt:
	vmovdqa oaep1296_16xq(%rip), %ymm0
	vmovdqa oaep1296_16xv(%rip), %ymm1

	xorl %eax, %eax
.p2align 5
L_ntt_half_loop16:
	HALF_LOAD_16
	HALF_STEP
	HALF_STORE_16
	addq $32, %rax
	cmpq $1280, %rax
	jb L_ntt_half_loop16

	HALF_LOAD_8
	HALF_STEP
	HALF_STORE_8

	FWD_R3_LAYER 432, 2, 13, 1
	FWD_R3_LAYER 144, 6, 4, 1
	FWD_R3_LAYER 48, 18, 1, 1
	FWD_R3_LAYER 16, 54, 0, 1

	leaq oaep1296_ntt_tail_z4_qinv(%rip), %r8
	leaq oaep1296_ntt_tail_z2_lo_qinv(%rip), %r9
	leaq oaep1296_ntt_tail_z2_hi_qinv(%rip), %r10
	leaq oaep1296_ntt_tail_z4_16(%rip), %r11
	leaq oaep1296_ntt_tail_z2_lo_16(%rip), %rcx
	leaq oaep1296_ntt_tail_z2_hi_16(%rip), %rdx
	movq %rdi, %rsi
	movl $20, %eax
.p2align 5
L_ntt_tail_loop8:
	NTT_TAIL_ROW 0, 8
	NTT_TAIL_ROW 1, 8
	addq $128, %rsi
	addq $32, %r8
	addq $32, %r9
	addq $32, %r10
	addq $32, %r11
	addq $32, %rcx
	addq $32, %rdx
	decl %eax
	jnz L_ntt_tail_loop8

	NTT_TAIL_ROW 0, 2
	NTT_TAIL_ROW 1, 2

	vzeroupper
	ret

.size ntt, .-ntt

.section .note.GNU-stack,"",@progbits
