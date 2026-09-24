.text

.macro MONT_X src qpair zpair out
	vpbroadcastd \qpair, %xmm2
	vpbroadcastd \zpair, %xmm3
	vpmullw %xmm2, \src, %xmm12
	vpmulhw %xmm3, \src, \out
	vpmulhw %xmm0, %xmm12, %xmm12
	vpsubw  %xmm12, \out, \out
.endm

.macro BARRETT_X reg
	vpmulhw %xmm1, \reg, %xmm15
	vpsraw  $11, %xmm15, %xmm15
	vpmullw %xmm0, %xmm15, %xmm15
	vpsubw  %xmm15, \reg, \reg
	vpcmpgtw oaep648_16xhalf(%rip), \reg, %xmm15
	vpand    %xmm0, %xmm15, %xmm15
	vpsubw   %xmm15, \reg, \reg
	vmovdqa  oaep648_16xneghalf(%rip), %xmm14
	vpcmpgtw \reg, %xmm14, %xmm15
	vpand    %xmm0, %xmm15, %xmm15
	vpaddw   %xmm15, \reg, \reg
.endm

.macro STEP3_6_X src pair out
	vpsrldq $6, \src, %xmm13
	vpaddw  %xmm13, \src, %xmm11
	vpsubw  \src, %xmm13, %xmm13
	MONT_X %xmm13, (\pair), 4(\pair), %xmm13
	vpslldq $6, %xmm13, %xmm13
	vpblendw $0x38, %xmm13, %xmm11, \out
.endm

.macro STORE6_X src offset
	vmovq \src, \offset(%rdi)
	vpextrd $2, \src, (8 + \offset)(%rdi)
.endm

.macro LOAD_HI_FAST_X
	vmovdqu 12(%rsi), %xmm5
.endm

.macro LOAD_HI_LAST_X
	vpxor %xmm5, %xmm5, %xmm5
	vmovq 12(%rsi), %xmm5
	vpinsrd $2, 20(%rsi), %xmm5, %xmm5
.endm

.macro INV_HEAD_BLOCK_X LOAD_HI
	vmovdqu   (%rsi), %xmm4
	\LOAD_HI

	STEP3_6_X %xmm4, %r8, %xmm6
	STEP3_6_X %xmm5, %r9, %xmm7

	vpaddw %xmm7, %xmm6, %xmm8
	BARRETT_X %xmm8

	vpsubw %xmm6, %xmm7, %xmm9
	MONT_X %xmm9, (%r10), 4(%r10), %xmm9

	STORE6_X %xmm8, 0
	STORE6_X %xmm9, 12
.endm

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

.macro INV_R3_Y
	vpsubw %ymm4, %ymm5, %ymm10
	MONT_Y_VEC %ymm10, %ymm13, %ymm14, %ymm10

	vpsubw %ymm4, %ymm6, %ymm11
	vpaddw %ymm10, %ymm11, %ymm11
	MONT_Y %ymm11, (%r8), 4(%r8), %ymm11

	vpsubw %ymm5, %ymm6, %ymm12
	vpsubw %ymm10, %ymm12, %ymm12
	MONT_Y %ymm12, (%r9), 4(%r9), %ymm12

	vpaddw %ymm5, %ymm4, %ymm7
	vpaddw %ymm6, %ymm7, %ymm7
	BARRETT_Y %ymm7
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

.macro STORE_INV_R3_16
	vmovdqu %ymm7,  (%rax)
	vmovdqu %ymm11, (%rax,%r10)
	vmovdqu %ymm12, (%rax,%r11)
.endm

.macro STORE_INV_R3_8
	vmovdqu %xmm7,  (%rax)
	vmovdqu %xmm11, (%rax,%r10)
	vmovdqu %xmm12, (%rax,%r11)
.endm

.macro STORE_INV_R3_4
	vmovq %xmm7,  (%rax)
	vmovq %xmm11, (%rax,%r10)
	vmovq %xmm12, (%rax,%r11)
.endm

.macro FINAL_LOAD16
	vmovdqu     (%rax), %ymm4
	vmovdqu 648(%rax), %ymm5
.endm

.macro FINAL_LOAD4
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vmovq     (%rax), %xmm4
	vmovq 648(%rax), %xmm5
.endm

.macro FINAL_STORE16
	vmovdqu %ymm9,     (%rax)
	vmovdqu %ymm10, 648(%rax)
.endm

.macro FINAL_STORE4
	vmovq %xmm9,     (%rax)
	vmovq %xmm10, 648(%rax)
.endm

.macro FINAL_STEP lowpair highpair
	vpaddw %ymm5, %ymm4, %ymm7
	vpsubw %ymm5, %ymm4, %ymm8
	MONT_Y %ymm8, oaep648_final_c2394_pair(%rip), oaep648_final_c2394_pair+4(%rip), %ymm8
	vpsubw %ymm8, %ymm7, %ymm9
	MONT_Y %ymm9, \lowpair(%rip), \lowpair+4(%rip), %ymm9
	MONT_Y %ymm8, \highpair(%rip), \highpair+4(%rip), %ymm10
.endm

.macro INV_BODY lowpair highpair
	vmovdqa oaep648_16xq(%rip), %xmm0
	vmovdqa oaep648_16xv(%rip), %xmm1
	leaq oaep648_inv_head_z3_lo_pair(%rip), %r8
	leaq oaep648_inv_head_z3_hi_pair(%rip), %r9
	leaq oaep648_inv_head_z6_pair(%rip), %r10
	movl $53, %ecx

.p2align 5
L_inv_head_loop_\@:
	INV_HEAD_BLOCK_X LOAD_HI_FAST_X
	addq $24, %rsi
	addq $24, %rdi
	addq $8, %r8
	addq $8, %r9
	addq $8, %r10
	decl %ecx
	jnz L_inv_head_loop_\@
	INV_HEAD_BLOCK_X LOAD_HI_LAST_X

	subq $1272, %rdi
	vmovdqa oaep648_16xq(%rip), %ymm0
	vmovdqa oaep648_16xv(%rip), %ymm1
	vmovdqa oaep648_16xomega_qinv(%rip), %ymm13
	vmovdqa oaep648_16xomega(%rip), %ymm14
	leaq oaep648_zetas_pair(%rip), %rdx

	movq $24, %r10
	movq $48, %r11
	leaq 8*52(%rdx), %r8
	leaq 8*53(%rdx), %r9
	movq %rdi, %rcx
	leaq 1296(%rdi), %rsi
.p2align 5
L_inv_step12_group_\@:
	movq %rcx, %rax
	LOAD_R3_8
	INV_R3_Y
	STORE_INV_R3_8
	addq $16, %rax
	LOAD_R3_4
	INV_R3_Y
	STORE_INV_R3_4
	addq $72, %rcx
	subq $16, %r8
	subq $16, %r9
	cmpq %rsi, %rcx
	jb L_inv_step12_group_\@

	movq $72, %r10
	movq $144, %r11
	leaq 8*16(%rdx), %r8
	leaq 8*17(%rdx), %r9
	movq %rdi, %rcx
	leaq 1296(%rdi), %rsi
.p2align 5
L_inv_step36_group_\@:
	movq %rcx, %rax
	leaq 64(%rcx), %rdx
.p2align 5
L_inv_step36_loop16_\@:
	LOAD_R3_16
	INV_R3_Y
	STORE_INV_R3_16
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_inv_step36_loop16_\@
	LOAD_R3_4
	INV_R3_Y
	STORE_INV_R3_4
	addq $216, %rcx
	subq $16, %r8
	subq $16, %r9
	cmpq %rsi, %rcx
	jb L_inv_step36_group_\@

	leaq oaep648_zetas_pair(%rip), %rdx
	movq $216, %r10
	movq $432, %r11
	leaq 8*4(%rdx), %r8
	leaq 8*5(%rdx), %r9
	movq %rdi, %rcx
	leaq 1296(%rdi), %rsi
.p2align 5
L_inv_step108_group_\@:
	movq %rcx, %rax
	leaq 192(%rcx), %rdx
.p2align 5
L_inv_step108_loop16_\@:
	LOAD_R3_16
	INV_R3_Y
	STORE_INV_R3_16
	addq $32, %rax
	cmpq %rdx, %rax
	jb L_inv_step108_loop16_\@
	LOAD_R3_8
	INV_R3_Y
	STORE_INV_R3_8
	addq $16, %rax
	LOAD_R3_4
	INV_R3_Y
	STORE_INV_R3_4
	addq $648, %rcx
	subq $16, %r8
	subq $16, %r9
	cmpq %rsi, %rcx
	jb L_inv_step108_group_\@

	movq %rdi, %rax
	leaq 640(%rdi), %rsi
.p2align 5
L_inv_final_loop16_\@:
	FINAL_LOAD16
	FINAL_STEP \lowpair, \highpair
	FINAL_STORE16
	addq $32, %rax
	cmpq %rsi, %rax
	jb L_inv_final_loop16_\@
	FINAL_LOAD4
	FINAL_STEP \lowpair, \highpair
	FINAL_STORE4

	vzeroupper
	ret
.endm

.global invntt
.type invntt, @function
invntt:
	INV_BODY oaep648_final_c2383_pair, oaep648_final_cm2363_pair
.size invntt, .-invntt

.global invntt_normalized
.type invntt_normalized, @function
invntt_normalized:
	INV_BODY oaep648_final_cm2601_pair, oaep648_final_c1927_pair
.size invntt_normalized, .-invntt_normalized

.section .note.GNU-stack,"",@progbits
