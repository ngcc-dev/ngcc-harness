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

.macro MONT_PAIR src pair out
	vpbroadcastd \pair(%rip), %ymm2
	vpbroadcastd \pair+4(%rip), %ymm3
	vpmullw %ymm2, \src, %ymm15
	vpmulhw %ymm3, \src, \out
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

.macro INV_R3_DYN
	vpsubw %ymm4, %ymm5, %ymm10
	MONT_MEMVEC %ymm10, oaep1296_16xomega_qinv(%rip), oaep1296_16xomega(%rip), %ymm10

	vpsubw %ymm4, %ymm6, %ymm11
	vpaddw %ymm10, %ymm11, %ymm11
	MONT_ZMEM %ymm11, (%r9), %ymm11

	vpsubw %ymm5, %ymm6, %ymm12
	vpsubw %ymm10, %ymm12, %ymm12
	MONT_ZMEM %ymm12, 2(%r9), %ymm12

	vpaddw %ymm5, %ymm4, %ymm7
	vpaddw %ymm6, %ymm7, %ymm7
	BARRETT_Y %ymm7
.endm

.macro INV_R3_LAYER stepb startz1 l16 tail8
	movq $\stepb, %r10
	movq $(2 * \stepb), %r11
	leaq zetas + 2 * \startz1(%rip), %r9
	movq %rdi, %rcx
	leaq 2592(%rdi), %r8
.p2align 5
L_inv_r3_group_\@:
	movq %rcx, %rax
	.if \l16
	movl $\l16, %edx
.p2align 5
L_inv_r3_loop16_\@:
	LOAD_R3_16
	INV_R3_DYN
	STORE_INV_R3_16
	addq $32, %rax
	decl %edx
	jnz L_inv_r3_loop16_\@
	.endif
	.if \tail8
	LOAD_R3_8
	INV_R3_DYN
	STORE_INV_R3_8
	.endif
	addq $(3 * \stepb), %rcx
	subq $4, %r9
	cmpq %r8, %rcx
	jb L_inv_r3_group_\@
.endm

.macro LOAD_IN8 coeff dst
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

.macro LOAD_IN2 coeff dst
	vpxor   %xmm15, %xmm15, %xmm15
	vpinsrw $0, (2*(\coeff) + 0*16)(%rsi), %xmm15, %xmm15
	vpinsrw $1, (2*(\coeff) + 1*16)(%rsi), %xmm15, %xmm15
	vpxor \dst, \dst, \dst
	vinserti128 $0, %xmm15, \dst, \dst
.endm

.macro STORE_OUT8 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*16)(%rax)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*16)(%rax)
	vpextrw $2, %xmm14, (2*(\coeff) + 2*16)(%rax)
	vpextrw $3, %xmm14, (2*(\coeff) + 3*16)(%rax)
	vpextrw $4, %xmm14, (2*(\coeff) + 4*16)(%rax)
	vpextrw $5, %xmm14, (2*(\coeff) + 5*16)(%rax)
	vpextrw $6, %xmm14, (2*(\coeff) + 6*16)(%rax)
	vpextrw $7, %xmm14, (2*(\coeff) + 7*16)(%rax)
.endm

.macro STORE_OUT2 coeff src
	vextracti128 $0, \src, %xmm14
	vpextrw $0, %xmm14, (2*(\coeff) + 0*16)(%rax)
	vpextrw $1, %xmm14, (2*(\coeff) + 1*16)(%rax)
.endm

.macro INV_HEAD_ROW coeff lanes
	LOAD_IN\lanes \coeff, %ymm4
	LOAD_IN\lanes "(\coeff + 2)", %ymm5
	LOAD_IN\lanes "(\coeff + 4)", %ymm6
	LOAD_IN\lanes "(\coeff + 6)", %ymm7

	vpaddw %ymm5, %ymm4, %ymm8
	BARRETT_Y %ymm8
	vpsubw %ymm4, %ymm5, %ymm9
	MONT_MEMVEC %ymm9, (%r9), (%rcx), %ymm9

	vpaddw %ymm7, %ymm6, %ymm10
	BARRETT_Y %ymm10
	vpsubw %ymm6, %ymm7, %ymm12
	MONT_MEMVEC %ymm12, (%r10), (%rdx), %ymm12

	vpaddw %ymm10, %ymm8, %ymm4
	BARRETT_Y %ymm4
	vpsubw %ymm8, %ymm10, %ymm5
	MONT_MEMVEC %ymm5, (%r8), (%r11), %ymm5

	vpaddw %ymm12, %ymm9, %ymm6
	BARRETT_Y %ymm6
	vpsubw %ymm9, %ymm12, %ymm7
	MONT_MEMVEC %ymm7, (%r8), (%r11), %ymm7

	STORE_OUT\lanes \coeff, %ymm4
	STORE_OUT\lanes "(\coeff + 4)", %ymm5
	STORE_OUT\lanes "(\coeff + 2)", %ymm6
	STORE_OUT\lanes "(\coeff + 6)", %ymm7
.endm

.macro FINAL_LOAD_16
	vmovdqu       (%rax), %ymm4
	vmovdqu  1296(%rax), %ymm5
.endm

.macro FINAL_LOAD_8
	vpxor %ymm4, %ymm4, %ymm4
	vpxor %ymm5, %ymm5, %ymm5
	vmovdqu       (%rax), %xmm4
	vmovdqu  1296(%rax), %xmm5
.endm

.macro FINAL_STORE_16
	vmovdqu %ymm9,       (%rax)
	vmovdqu %ymm10, 1296(%rax)
.endm

.macro FINAL_STORE_8
	vmovdqu %xmm9,       (%rax)
	vmovdqu %xmm10, 1296(%rax)
.endm

.macro FINAL_STEP lowpair highpair
	vpaddw %ymm5, %ymm4, %ymm7
	vpsubw %ymm5, %ymm4, %ymm8
	MONT_PAIR %ymm8, oaep1296_final_cm7621_pair, %ymm8
	vpsubw %ymm8, %ymm7, %ymm9
	MONT_PAIR %ymm9, \lowpair, %ymm9
	MONT_PAIR %ymm8, \highpair, %ymm10
.endm

.macro INV_BODY lowpair highpair
	pushq %rbx
	vmovdqa oaep1296_16xq(%rip), %ymm0
	vmovdqa oaep1296_16xv(%rip), %ymm1

	leaq oaep1296_inv_head_z4_qinv(%rip), %r8
	leaq oaep1296_inv_head_z2_lo_qinv(%rip), %r9
	leaq oaep1296_inv_head_z2_hi_qinv(%rip), %r10
	leaq oaep1296_inv_head_z4_16(%rip), %r11
	leaq oaep1296_inv_head_z2_lo_16(%rip), %rcx
	leaq oaep1296_inv_head_z2_hi_16(%rip), %rdx
	movq %rdi, %rax
	movl $20, %ebx
.p2align 5
L_inv_head_loop8_\@:
	INV_HEAD_ROW 0, 8
	INV_HEAD_ROW 1, 8
	addq $128, %rsi
	addq $128, %rax
	addq $32, %r8
	addq $32, %r9
	addq $32, %r10
	addq $32, %r11
	addq $32, %rcx
	addq $32, %rdx
	decl %ebx
	jnz L_inv_head_loop8_\@

	INV_HEAD_ROW 0, 2
	INV_HEAD_ROW 1, 2

	INV_R3_LAYER 16, 160, 0, 1
	INV_R3_LAYER 48, 52, 1, 1
	INV_R3_LAYER 144, 16, 4, 1
	INV_R3_LAYER 432, 4, 13, 1

	movq %rdi, %rax
	leaq 1280(%rdi), %rsi
.p2align 5
L_inv_final_loop16_\@:
	FINAL_LOAD_16
	FINAL_STEP \lowpair, \highpair
	FINAL_STORE_16
	addq $32, %rax
	cmpq %rsi, %rax
	jb L_inv_final_loop16_\@

	FINAL_LOAD_8
	FINAL_STEP \lowpair, \highpair
	FINAL_STORE_8

	vzeroupper
	popq %rbx
	ret
.endm

.global invntt
.type invntt, @function
invntt:
	INV_BODY oaep1296_final_cm2463_pair, oaep1296_final_cm4926_pair
.size invntt, .-invntt

.global invntt_normalized
.type invntt_normalized, @function
invntt_normalized:
	INV_BODY oaep1296_final_cm2275_pair, oaep1296_final_cm4550_pair
.size invntt_normalized, .-invntt_normalized

.section .note.GNU-stack,"",@progbits
