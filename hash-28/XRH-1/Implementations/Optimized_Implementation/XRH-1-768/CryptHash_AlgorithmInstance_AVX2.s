	.file	"CryptHash_AlgorithmInstance.c"
	.text
# XRH1280(R0, R1, state) is implemented in CryptHash_AlgorithmInstance.c.
# Keeping the variable-round helper in C avoids a 9000-line compiler-unrolled
# block here while the hot EDMC path below remains AVX2-specialized.
	.p2align 4
	.globl	XRH_edmc_permute
	.type	XRH_edmc_permute, @function
XRH_edmc_permute:
	movq	%rdi, %rdx
	pushq	%r15
	pushq	%r14
	pushq	%r13
	movabsq	$-8608480567731124088, %r13
	pushq	%r12
	movabsq	$8608480567731124087, %r12
	pushq	%rbx
	vmovdqu	80(%rdi), %ymm0
	vmovdqu	(%rdi), %ymm6
	vpunpcklqdq	120(%rdi), %ymm0, %ymm1
	vpunpcklqdq	40(%rdi), %ymm6, %ymm2
	vpunpckhqdq	120(%rdi), %ymm0, %ymm4
	vpunpckhqdq	40(%rdi), %ymm6, %ymm8
	vmovq	112(%rdi), %xmm7
	vperm2i128	$32, %ymm1, %ymm2, %ymm13
	vperm2i128	$49, %ymm1, %ymm2, %ymm14
	vpinsrq	$1, 152(%rdi), %xmm7, %xmm9
	vmovq	32(%rdi), %xmm10
	vpinsrq	$1, 72(%rdi), %xmm10, %xmm11
	vperm2i128	$32, %ymm4, %ymm8, %ymm3
	vperm2i128	$49, %ymm4, %ymm8, %ymm15
	vpxor	%ymm3, %ymm13, %ymm12
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vinserti128	$0x1, %xmm9, %ymm11, %ymm6
	vpxor	%ymm4, %ymm12, %ymm5
	vpor	%ymm14, %ymm13, %ymm9
	vpxor	%ymm6, %ymm14, %ymm8
	vpxor	%ymm6, %ymm13, %ymm12
	vpand	%ymm6, %ymm3, %ymm11
	vpxor	%ymm15, %ymm5, %ymm2
	vpandn	%ymm6, %ymm14, %ymm1
	vpand	%ymm12, %ymm5, %ymm7
	vpxor	%ymm8, %ymm11, %ymm5
	vpand	%ymm2, %ymm14, %ymm0
	vpxor	%ymm7, %ymm1, %ymm10
	vpand	%ymm6, %ymm13, %ymm13
	vpxor	%ymm15, %ymm5, %ymm7
	vpand	%ymm15, %ymm8, %ymm15
	vpxor	%ymm0, %ymm10, %ymm1
	vpxor	%ymm9, %ymm13, %ymm6
	vpor	%ymm7, %ymm3, %ymm10
	vpor	%ymm12, %ymm2, %ymm8
	vpxor	%ymm15, %ymm14, %ymm14
	vpxor	%ymm9, %ymm3, %ymm3
	vpxor	%ymm10, %ymm6, %ymm5
	vpxor	%ymm14, %ymm8, %ymm2
	vpxor	%ymm15, %ymm3, %ymm9
	vpunpcklqdq	%ymm5, %ymm1, %ymm12
	vpunpcklqdq	%ymm9, %ymm2, %ymm11
	vpunpckhqdq	%ymm9, %ymm2, %ymm13
	vpxor	%ymm7, %ymm0, %ymm7
	vpunpckhqdq	%ymm5, %ymm1, %ymm1
	vperm2i128	$32, %ymm11, %ymm12, %ymm10
	vperm2i128	$32, %ymm13, %ymm1, %ymm5
	vpextrq	$1, %xmm7, %rax
	vmovq	%xmm7, %rsi
	vpxor	%ymm5, %ymm10, %ymm14
	vperm2i128	$49, %ymm11, %ymm12, %ymm6
	vextracti128	$0x1, %ymm7, %xmm8
	xorq	%rax, %rsi
	vextracti128	$0x1, %ymm14, %xmm2
	vperm2i128	$49, %ymm13, %ymm1, %ymm15
	vpermq	$144, %ymm14, %ymm11
	movq	%rsi, %r10
	vmovq	%rsi, %xmm12
	vpextrq	$1, %xmm8, %rcx
	vpextrq	$1, %xmm2, %rdi
	vmovdqa	.LC0(%rip), %ymm2
	vpbroadcastq	%xmm12, %ymm1
	vmovq	%xmm8, %r9
	vpxor	%ymm15, %ymm6, %ymm3
	xorq	%rcx, %r9
	vpxor	%ymm3, %ymm5, %ymm9
	vpand	%ymm2, %ymm1, %ymm10
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm1, %ymm11, %ymm13
	vextracti128	$0x1, %ymm9, %xmm6
	vpermq	$144, %ymm9, %ymm8
	xorq	%r9, %rax
	vpxor	%ymm13, %ymm15, %ymm5
	vmovq	%rax, %xmm7
	vmovq	%rcx, %xmm13
	vpxor	%ymm5, %ymm10, %ymm15
	vpbroadcastq	%xmm7, %ymm0
	vpbroadcastq	%xmm13, %ymm10
	vpermq	$144, %ymm15, %ymm5
	vpblendd	$3, %ymm0, %ymm8, %ymm9
	vpand	%ymm2, %ymm0, %ymm12
	vpextrq	$1, %xmm6, %r8
	vpblendd	$3, %ymm10, %ymm5, %ymm6
	vpxor	%ymm9, %ymm12, %ymm11
	vpand	%ymm2, %ymm10, %ymm7
	vextracti128	$0x1, %ymm15, %xmm1
	vpxor	%ymm6, %ymm3, %ymm3
	xorq	%r8, %r10
	vpxor	%ymm3, %ymm7, %ymm8
	vpxor	%ymm11, %ymm14, %ymm14
	vpextrq	$1, %xmm1, %rbx
	xorq	%r10, %rcx
	vpxor	%ymm8, %ymm11, %ymm0
	vpxor	%ymm14, %ymm15, %ymm15
	vmovq	%r10, %xmm7
	xorq	%r9, %rbx
	vpunpckhqdq	%ymm0, %ymm14, %ymm12
	vpunpcklqdq	%ymm15, %ymm8, %ymm11
	vpunpckhqdq	%ymm15, %ymm8, %ymm1
	xorq	%rbx, %r8
	vpunpcklqdq	%ymm0, %ymm14, %ymm9
	vmovq	%rbx, %xmm13
	vperm2i128	$32, %ymm1, %ymm12, %ymm6
	movabsq	$-1229782938247303442, %r10
	vperm2i128	$32, %ymm11, %ymm9, %ymm5
	vpinsrq	$1, %rcx, %xmm13, %xmm10
	vpinsrq	$1, %r8, %xmm7, %xmm8
	movabsq	$1229782938247303441, %r9
	vinserti128	$0x1, %xmm10, %ymm8, %ymm15
	vpxor	%ymm6, %ymm5, %ymm0
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	movabsq	$-3689348814741910324, %rbx
	vperm2i128	$49, %ymm1, %ymm12, %ymm3
	vpxor	%ymm4, %ymm0, %ymm11
	vpxor	%ymm15, %ymm5, %ymm12
	vpxor	%ymm15, %ymm14, %ymm9
	vpxor	%ymm3, %ymm11, %ymm0
	vpandn	%ymm15, %ymm14, %ymm1
	vpand	%ymm12, %ymm11, %ymm13
	vpand	%ymm15, %ymm6, %ymm11
	vpand	%ymm0, %ymm14, %ymm7
	vpxor	%ymm13, %ymm1, %ymm8
	vpxor	%ymm9, %ymm11, %ymm13
	vpor	%ymm14, %ymm5, %ymm10
	vpxor	%ymm7, %ymm8, %ymm1
	vpand	%ymm3, %ymm9, %ymm9
	vpxor	%ymm3, %ymm13, %ymm8
	vpand	%ymm15, %ymm5, %ymm5
	vpor	%ymm12, %ymm0, %ymm3
	vpor	%ymm8, %ymm6, %ymm11
	vpxor	%ymm10, %ymm5, %ymm15
	vpxor	%ymm9, %ymm3, %ymm0
	vpxor	%ymm10, %ymm6, %ymm6
	vpxor	%ymm11, %ymm15, %ymm13
	vpxor	%ymm9, %ymm6, %ymm12
	vpxor	%ymm14, %ymm0, %ymm14
	vpunpcklqdq	%ymm13, %ymm1, %ymm10
	vpxor	%ymm8, %ymm7, %ymm7
	vpunpckhqdq	%ymm12, %ymm14, %ymm15
	vpunpckhqdq	%ymm13, %ymm1, %ymm1
	vpunpcklqdq	%ymm12, %ymm14, %ymm5
	vpextrq	$1, %xmm7, %r11
	vperm2i128	$32, %ymm15, %ymm1, %ymm13
	vpand	.LC1(%rip), %ymm13, %ymm0
	vpand	.LC2(%rip), %ymm13, %ymm12
	vperm2i128	$49, %ymm5, %ymm10, %ymm9
	vperm2i128	$32, %ymm5, %ymm10, %ymm11
	leaq	(%r11,%r11), %rax
	shrq	$3, %r11
	vextracti128	$0x1, %ymm7, %xmm3
	andq	%r9, %r11
	vmovq	%xmm7, %r15
	andq	%r10, %rax
	vpand	.LC4(%rip), %ymm9, %ymm5
	vpsrlq	$3, %ymm0, %ymm6
	vperm2i128	$49, %ymm15, %ymm1, %ymm14
	orq	%r11, %rax
	vpsllq	$1, %ymm12, %ymm10
	vmovq	%r10, %xmm7
	xorq	%rax, %r15
	movabsq	$3689348814741910323, %r11
	vmovq	%r9, %xmm0
	vpor	%ymm6, %ymm10, %ymm15
	vmovq	%xmm3, %rdi
	vpbroadcastq	%xmm0, %ymm6
	vpand	.LC3(%rip), %ymm9, %ymm1
	vpxor	%ymm15, %ymm11, %ymm11
	vpextrq	$1, %xmm3, %rcx
	vpsllq	$2, %ymm5, %ymm9
	vpbroadcastq	%xmm7, %ymm5
	vpand	%ymm5, %ymm14, %ymm8
	vpand	%ymm6, %ymm14, %ymm14
	vmovq	%r15, %xmm7
	leaq	0(,%rcx,8), %rsi
	leaq	0(,%rdi,4), %r14
	shrq	%rcx
	shrq	$2, %rdi
	andq	%r12, %rcx
	andq	%r11, %rdi
	andq	%r13, %rsi
	andq	%rbx, %r14
	vpsrlq	$1, %ymm8, %ymm3
	vpsllq	$3, %ymm14, %ymm12
	orq	%rcx, %rsi
	orq	%rdi, %r14
	vpsrlq	$2, %ymm1, %ymm13
	vpor	%ymm3, %ymm12, %ymm10
	vpbroadcastq	%xmm7, %ymm8
	xorq	%rsi, %r14
	vpor	%ymm13, %ymm9, %ymm1
	vpermq	$144, %ymm11, %ymm3
	vextracti128	$0x1, %ymm11, %xmm9
	xorq	%r14, %rax
	vpxor	%ymm10, %ymm1, %ymm13
	vpblendd	$3, %ymm8, %ymm3, %ymm0
	vpextrq	$1, %xmm9, %rdi
	vpand	%ymm2, %ymm8, %ymm14
	vpxor	%ymm13, %ymm15, %ymm15
	vpxor	%ymm0, %ymm10, %ymm12
	xorq	%rsi, %rdi
	vpxor	%ymm12, %ymm14, %ymm10
	vextracti128	$0x1, %ymm15, %xmm1
	vmovq	%rax, %xmm9
	vmovq	%rdi, %xmm12
	vpextrq	$1, %xmm1, %r8
	vpbroadcastq	%xmm9, %ymm7
	vpbroadcastq	%xmm12, %ymm1
	vpermq	$144, %ymm10, %ymm9
	vpermq	$144, %ymm15, %ymm15
	movq	%r8, %rax
	vpblendd	$3, %ymm7, %ymm15, %ymm8
	vpand	%ymm2, %ymm7, %ymm3
	vextracti128	$0x1, %ymm10, %xmm0
	xorq	%r8, %r15
	vpblendd	$3, %ymm1, %ymm9, %ymm7
	vpand	%ymm2, %ymm1, %ymm15
	vpxor	%ymm8, %ymm3, %ymm14
	xorq	%r15, %rdi
	vpxor	%ymm7, %ymm13, %ymm13
	vpextrq	$1, %xmm0, %rcx
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm3
	xorq	%r14, %rcx
	vpxor	%ymm11, %ymm10, %ymm10
	vpxor	%ymm3, %ymm14, %ymm14
	xorq	%rcx, %rax
	vpand	.LC3(%rip), %ymm3, %ymm7
	vpand	%ymm5, %ymm14, %ymm8
	leaq	0(,%rax,8), %rsi
	shrq	%rax
	vpand	.LC4(%rip), %ymm3, %ymm13
	vpand	%ymm6, %ymm14, %ymm12
	vpsrlq	$1, %ymm8, %ymm0
	andq	%r13, %rsi
	andq	%r12, %rax
	vpand	.LC1(%rip), %ymm10, %ymm8
	vpsllq	$3, %ymm12, %ymm1
	vpand	.LC2(%rip), %ymm10, %ymm10
	orq	%rsi, %rax
	vpsrlq	$2, %ymm7, %ymm15
	vpsllq	$2, %ymm13, %ymm3
	vpor	%ymm0, %ymm1, %ymm9
	vpsllq	$1, %ymm10, %ymm12
	vpsrlq	$3, %ymm8, %ymm0
	vpor	%ymm15, %ymm3, %ymm14
	leaq	0(,%rcx,4), %r14
	shrq	$2, %rcx
	vpxor	.LC7(%rip), %ymm11, %ymm11
	andq	%r11, %rcx
	leaq	(%rdi,%rdi), %r11
	andq	%rbx, %r14
	shrq	$3, %rdi
	vpor	%ymm0, %ymm12, %ymm7
	orq	%rcx, %r14
	andq	%r9, %rdi
	andq	%r10, %r11
	vpunpcklqdq	%ymm9, %ymm11, %ymm15
	vpunpcklqdq	%ymm7, %ymm14, %ymm1
	vpunpckhqdq	%ymm7, %ymm14, %ymm3
	orq	%rdi, %r11
	vpunpckhqdq	%ymm9, %ymm11, %ymm13
	vmovq	%r14, %xmm14
	vmovq	%r15, %xmm7
	vperm2i128	$32, %ymm3, %ymm13, %ymm9
	vperm2i128	$32, %ymm1, %ymm15, %ymm8
	vpinsrq	$1, %r11, %xmm14, %xmm12
	vpinsrq	$1, %rax, %xmm7, %xmm11
	vperm2i128	$49, %ymm1, %ymm15, %ymm10
	vpxor	%ymm9, %ymm8, %ymm15
	vinserti128	$0x1, %xmm12, %ymm11, %ymm7
	vperm2i128	$49, %ymm3, %ymm13, %ymm0
	vpxor	%ymm4, %ymm15, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm0, %ymm11, %ymm1
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpextrq	$1, %xmm3, %rcx
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm3, %r15
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	xorq	%rcx, %r15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%r15, %xmm8
	vpxor	%ymm10, %ymm13, %ymm12
	vpextrq	$1, %xmm9, %rax
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpextrq	$1, %xmm7, %r8
	vpand	%ymm2, %ymm0, %ymm3
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	xorq	%rax, %rdi
	xorq	%rdi, %rcx
	vpxor	%ymm1, %ymm3, %ymm11
	vpextrq	$1, %xmm9, %r14
	xorq	%r8, %rax
	vmovq	%rcx, %xmm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	xorq	%r14, %r15
	vmovq	%rax, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vextracti128	$0x1, %ymm11, %xmm3
	xorq	%r15, %rax
	vpbroadcastq	%xmm1, %ymm7
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpand	%ymm2, %ymm8, %ymm13
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpxor	%ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm7, %ymm12
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rsi
	vpxor	%ymm10, %ymm12, %ymm13
	xorq	%rdi, %rsi
	vpxor	%ymm14, %ymm11, %ymm11
	movabsq	$-4222189076152336, %rdi
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpcklqdq	%ymm11, %ymm13, %ymm3
	vpunpckhqdq	%ymm11, %ymm13, %ymm1
	xorq	%rsi, %r14
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpckhqdq	%ymm0, %ymm14, %ymm9
	vmovq	%rsi, %xmm12
	vmovq	%r15, %xmm14
	vperm2i128	$32, %ymm1, %ymm9, %ymm8
	vperm2i128	$32, %ymm3, %ymm15, %ymm7
	movabsq	$-1152657617789587456, %r15
	vpinsrq	$1, %rax, %xmm12, %xmm13
	vpinsrq	$1, %r14, %xmm14, %xmm11
	vperm2i128	$49, %ymm3, %ymm15, %ymm10
	movabsq	$4222189076152335, %r14
	vperm2i128	$49, %ymm1, %ymm9, %ymm0
	vpxor	%ymm8, %ymm7, %ymm15
	vinserti128	$0x1, %xmm13, %ymm11, %ymm1
	vpxor	%ymm4, %ymm15, %ymm11
	vpxor	%ymm1, %ymm7, %ymm15
	vpandn	%ymm1, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm1, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm1, %ymm8, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm1, %ymm7, %ymm7
	vpand	%ymm0, %ymm13, %ymm13
	vpxor	%ymm0, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm0
	vpand	%ymm9, %ymm10, %ymm3
	vpor	%ymm11, %ymm8, %ymm1
	vpxor	%ymm13, %ymm0, %ymm9
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm1, %ymm7, %ymm7
	vmovq	%r15, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vperm2i128	$49, %ymm12, %ymm15, %ymm1
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vpbroadcastq	.LC32(%rip), %ymm12
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	%xmm9, %ymm9
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpand	%ymm9, %ymm10, %ymm14
	vpand	%ymm12, %ymm10, %ymm13
	vpxor	%ymm11, %ymm3, %ymm3
	vpsrlq	$12, %ymm14, %ymm15
	vpsllq	$4, %ymm13, %ymm10
	vextracti128	$0x1, %ymm3, %xmm0
	vpextrq	$1, %xmm3, %r11
	vmovq	%xmm3, %rsi
	vpor	%ymm15, %ymm10, %ymm3
	movq	%r11, %r8
	shrq	$12, %r11
	vmovq	%xmm0, %rcx
	vpxor	%ymm3, %ymm8, %ymm8
	andq	%r14, %r11
	vpextrq	$1, %xmm0, %rax
	salq	$4, %r8
	vpshufb	.LXRH_bswap16(%rip), %ymm1, %ymm15
	andq	%rdi, %r8
	movq	%rax, %rdi
	shrq	$4, %rax
	vpbroadcastq	.LC35(%rip), %ymm11
	orq	%r11, %r8
	salq	$12, %rdi
	movabsq	$71777214294589695, %r14
	andq	%r15, %rdi
	xorq	%r8, %rsi
	movabsq	$1152657617789587455, %r11
	vpand	%ymm11, %ymm7, %ymm14
	andq	%r11, %rax
	movabsq	$-71777214294589696, %r11
	vpsrlq	$4, %ymm14, %ymm0
	orq	%rdi, %rax
	movq	%rcx, %rdi
	salq	$8, %rdi
	shrq	$8, %rcx
	vpbroadcastq	.LC36(%rip), %ymm14
	andq	%r14, %rcx
	andq	%r11, %rdi
	vpand	%ymm14, %ymm7, %ymm7
	orq	%rcx, %rdi
	vpsllq	$12, %ymm7, %ymm7
	xorq	%rax, %rdi
	vpor	%ymm0, %ymm7, %ymm7
	vmovq	%rsi, %xmm0
	xorq	%rdi, %r8
	vpxor	%ymm7, %ymm15, %ymm1
	vpxor	%ymm1, %ymm3, %ymm15
	vextracti128	$0x1, %ymm8, %xmm3
	vpextrq	$1, %xmm3, %rcx
	vpbroadcastq	%xmm0, %ymm3
	vpermq	$144, %ymm8, %ymm0
	vpblendd	$3, %ymm3, %ymm0, %ymm0
	vpand	%ymm2, %ymm3, %ymm3
	xorq	%rcx, %rax
	vpxor	%ymm0, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm0
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm3, %ymm3
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm0, %rcx
	vpbroadcastq	%xmm7, %ymm0
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm0, %ymm15, %ymm7
	vextracti128	$0x1, %ymm3, %xmm15
	vpand	%ymm2, %ymm0, %ymm0
	vpextrq	$1, %xmm15, %r8
	vpxor	%ymm7, %ymm0, %ymm0
	vmovq	%rax, %xmm7
	xorq	%rsi, %rax
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm3, %ymm15
	vpxor	%ymm0, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm2, %ymm7, %ymm7
	movq	%rcx, %r8
	shrq	$4, %rcx
	vpxor	%ymm15, %ymm1, %ymm1
	vpxor	%ymm8, %ymm3, %ymm3
	vpxor	%ymm1, %ymm7, %ymm15
	salq	$12, %r8
	vpand	%ymm9, %ymm3, %ymm9
	vpand	%ymm12, %ymm3, %ymm12
	vpxor	%ymm15, %ymm0, %ymm0
	andq	%r15, %r8
	vpand	%ymm11, %ymm0, %ymm11
	vpand	%ymm14, %ymm0, %ymm14
	movabsq	$1152657617789587455, %r15
	andq	%r15, %rcx
	vpsrlq	$4, %ymm11, %ymm7
	vpxor	.LC14(%rip), %ymm8, %ymm8
	vpsllq	$12, %ymm14, %ymm1
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm14
	orq	%r8, %rcx
	movq	%rdi, %r8
	salq	$8, %r8
	shrq	$8, %rdi
	vpor	%ymm7, %ymm1, %ymm0
	andq	%r14, %rdi
	vpunpcklqdq	%ymm0, %ymm8, %ymm10
	andq	%r11, %r8
	vpsrlq	$12, %ymm9, %ymm7
	vpunpckhqdq	%ymm0, %ymm8, %ymm0
	orq	%rdi, %r8
	movq	%rax, %rdi
	vpsllq	$4, %ymm12, %ymm3
	movabsq	$-4222189076152336, %r14
	shrq	$12, %rax
	salq	$4, %rdi
	vpor	%ymm7, %ymm3, %ymm1
	vmovq	%r8, %xmm15
	movabsq	$4222189076152335, %r11
	andq	%r11, %rax
	vpunpcklqdq	%ymm1, %ymm14, %ymm11
	vpunpckhqdq	%ymm1, %ymm14, %ymm13
	andq	%r14, %rdi
	vmovq	%rsi, %xmm7
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm0, %ymm9
	orq	%rax, %rdi
	vpinsrq	$1, %rdi, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm7, %xmm12
	vpxor	%ymm9, %ymm8, %ymm3
	vinserti128	$0x1, %xmm14, %ymm12, %ymm7
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vperm2i128	$49, %ymm13, %ymm0, %ymm0
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm3, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rcx
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	xorq	%rsi, %rcx
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%rcx, %xmm8
	vpextrq	$1, %xmm9, %rax
	vpxor	%ymm10, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpextrq	$1, %xmm7, %r8
	xorq	%rax, %rdi
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm0, %ymm3
	xorq	%r8, %rax
	xorq	%rdi, %rsi
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	vpxor	%ymm1, %ymm3, %ymm11
	vmovq	%rsi, %xmm7
	vmovq	%rax, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vpextrq	$1, %xmm9, %r14
	vpbroadcastq	%xmm1, %ymm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm13
	xorq	%r14, %rcx
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpand	%ymm2, %ymm7, %ymm12
	xorq	%rcx, %rax
	vpxor	%ymm0, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm10, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r11
	xorq	%rdi, %r11
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpcklqdq	%ymm11, %ymm13, %ymm8
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	xorq	%r11, %r14
	vpunpckhqdq	%ymm0, %ymm14, %ymm1
	vmovq	%r11, %xmm12
	vmovq	%rcx, %xmm14
	vperm2i128	$32, %ymm3, %ymm1, %ymm9
	vperm2i128	$32, %ymm8, %ymm15, %ymm7
	vpinsrq	$1, %rax, %xmm12, %xmm13
	vpinsrq	$1, %r14, %xmm14, %xmm11
	vperm2i128	$49, %ymm3, %ymm1, %ymm1
	vpxor	%ymm9, %ymm7, %ymm0
	vinserti128	$0x1, %xmm13, %ymm11, %ymm3
	vperm2i128	$49, %ymm8, %ymm15, %ymm10
	vpxor	%ymm4, %ymm0, %ymm11
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm1, %ymm11, %ymm0
	vpandn	%ymm3, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm9, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm0, %ymm10, %ymm8
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpor	%ymm11, %ymm9, %ymm3
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vpxor	%ymm10, %ymm0, %ymm10
	vpxor	%ymm8, %ymm12, %ymm12
	vpunpcklqdq	%ymm14, %ymm10, %ymm3
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm7
	vperm2i128	$32, %ymm7, %ymm12, %ymm10
	vperm2i128	$49, %ymm3, %ymm15, %ymm1
	vperm2i128	$49, %ymm7, %ymm12, %ymm0
	vperm2i128	$32, %ymm3, %ymm15, %ymm13
	vpextrq	$1, %xmm8, %rsi
	vmovq	%xmm8, %rdi
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm12

	vextracti128	$0x1, %ymm8, %xmm9
	rorx	$48, %rsi, %rax
	xorq	%rax, %rdi
	vpshufb	.LXRH_rot48(%rip), %ymm0, %ymm11

	vpshufd	$177, %ymm1, %ymm0
	vpxor	%ymm12, %ymm13, %ymm13
	vmovq	%xmm9, %r8
	vpextrq	$1, %xmm9, %rcx
	vpxor	%ymm11, %ymm0, %ymm9
	vpermq	$144, %ymm13, %ymm7
	rorx	$32, %r8, %r11
	vmovq	%rdi, %xmm15
	vpxor	%ymm9, %ymm12, %ymm14
	vextracti128	$0x1, %ymm13, %xmm8
	rorx	$16, %rcx, %r14
	vpbroadcastq	%xmm15, %ymm12
	xorq	%r14, %r11
	vpextrq	$1, %xmm8, %r8
	vpblendd	$3, %ymm12, %ymm7, %ymm10
	vpand	%ymm2, %ymm12, %ymm1
	xorq	%r14, %r8
	xorq	%r11, %rax
	vpxor	%ymm10, %ymm11, %ymm3
	vextracti128	$0x1, %ymm14, %xmm0
	vmovq	%rax, %xmm8
	vpxor	%ymm3, %ymm1, %ymm11
	vmovq	%r8, %xmm3
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm0, %rsi
	vpermq	$144, %ymm11, %ymm8
	vpbroadcastq	%xmm3, %ymm0
	vpermq	$144, %ymm14, %ymm14
	vpand	%ymm2, %ymm15, %ymm7
	vextracti128	$0x1, %ymm11, %xmm1
	xorq	%rsi, %rdi
	vpblendd	$3, %ymm15, %ymm14, %ymm12
	vpblendd	$3, %ymm0, %ymm8, %ymm15
	vpand	%ymm2, %ymm0, %ymm14
	xorq	%rdi, %r8
	vpxor	%ymm12, %ymm7, %ymm10
	vpxor	%ymm15, %ymm9, %ymm9
	vpextrq	$1, %xmm1, %rcx
	vpxor	%ymm9, %ymm14, %ymm12
	vpxor	%ymm10, %ymm13, %ymm13
	xorq	%r11, %rcx
	rorq	$48, %r8
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm12, %ymm10, %ymm7
	xorq	%rcx, %rsi
	rorx	$32, %rcx, %rax
	vpshufb	.LXRH_rot48(%rip), %ymm7, %ymm3

	rorq	$16, %rsi
	vpxor	.LC15(%rip), %ymm13, %ymm13
	vpshufd	$177, %ymm12, %ymm15

	vpshufb	.LXRH_rot16(%rip), %ymm11, %ymm12

	vpunpcklqdq	%ymm3, %ymm13, %ymm11
	vpunpckhqdq	%ymm3, %ymm13, %ymm7
	vpunpckhqdq	%ymm12, %ymm15, %ymm1
	vpunpcklqdq	%ymm12, %ymm15, %ymm10
	vmovq	%rax, %xmm3
	vmovq	%rdi, %xmm14
	vperm2i128	$32, %ymm1, %ymm7, %ymm9
	vperm2i128	$32, %ymm10, %ymm11, %ymm8
	vpinsrq	$1, %r8, %xmm3, %xmm15
	vpinsrq	$1, %rsi, %xmm14, %xmm12
	vperm2i128	$49, %ymm10, %ymm11, %ymm10
	vperm2i128	$49, %ymm1, %ymm7, %ymm0
	vpxor	%ymm9, %ymm8, %ymm11
	vinserti128	$0x1, %xmm15, %ymm12, %ymm7
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rcx
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	xorq	%rsi, %rcx
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%rcx, %xmm8
	vpextrq	$1, %xmm9, %r14
	vpxor	%ymm10, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpextrq	$1, %xmm7, %r8
	xorq	%r14, %rdi
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm0, %ymm3
	xorq	%r8, %r14
	xorq	%rdi, %rsi
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	vpxor	%ymm1, %ymm3, %ymm11
	vmovq	%rsi, %xmm7
	vmovq	%r14, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vpextrq	$1, %xmm9, %rax
	vpbroadcastq	%xmm1, %ymm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm13
	xorq	%rax, %rcx
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpand	%ymm2, %ymm7, %ymm12
	xorq	%rcx, %r14
	vpxor	%ymm0, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm10, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r11
	xorq	%rdi, %r11
	vpxor	%ymm14, %ymm11, %ymm3
	vpxor	%ymm13, %ymm15, %ymm11
	vpunpcklqdq	%ymm11, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm14, %ymm0
	vpunpcklqdq	%ymm3, %ymm13, %ymm1
	xorq	%r11, %rax
	vpunpckhqdq	%ymm3, %ymm13, %ymm7
	vmovq	%r11, %xmm12
	vmovq	%rcx, %xmm14
	movabsq	$3689348814741910323, %r11
	vperm2i128	$32, %ymm7, %ymm0, %ymm9
	vperm2i128	$32, %ymm1, %ymm15, %ymm8
	vpinsrq	$1, %r14, %xmm12, %xmm13
	vpinsrq	$1, %rax, %xmm14, %xmm3
	vperm2i128	$49, %ymm1, %ymm15, %ymm10
	vpxor	%ymm9, %ymm8, %ymm11
	vperm2i128	$49, %ymm7, %ymm0, %ymm1
	vinserti128	$0x1, %xmm13, %ymm3, %ymm7
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm1, %ymm11, %ymm0
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm0, %ymm10, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm0, %ymm15
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpextrq	$1, %xmm3, %rdi
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	leaq	(%rdi,%rdi), %rax
	shrq	$3, %rdi
	vextracti128	$0x1, %ymm3, %xmm9
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm0
	andq	%r9, %rdi
	andq	%r10, %rax
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vperm2i128	$32, %ymm7, %ymm10, %ymm1
	vpand	.LC1(%rip), %ymm13, %ymm14
	orq	%rdi, %rax
	vmovq	%xmm9, %r8
	vpand	.LC2(%rip), %ymm13, %ymm12
	vmovq	%xmm3, %rcx
	vpand	.LC3(%rip), %ymm15, %ymm8
	vpextrq	$1, %xmm9, %rsi
	vpand	%ymm5, %ymm0, %ymm9
	xorq	%rax, %rcx
	vpsrlq	$3, %ymm14, %ymm10
	vpand	.LC4(%rip), %ymm15, %ymm15
	vpand	%ymm6, %ymm0, %ymm0
	vpsllq	$1, %ymm12, %ymm7
	leaq	0(,%rsi,8), %rdi
	shrq	%rsi
	leaq	0(,%r8,4), %r14
	vpor	%ymm10, %ymm7, %ymm13
	shrq	$2, %r8
	andq	%r12, %rsi
	andq	%r13, %rdi
	vpxor	%ymm13, %ymm1, %ymm1
	andq	%r11, %r8
	andq	%rbx, %r14
	orq	%rsi, %rdi
	vpsrlq	$2, %ymm8, %ymm3
	vpsllq	$2, %ymm15, %ymm11
	vmovq	%rcx, %xmm15
	orq	%r8, %r14
	vpsrlq	$1, %ymm9, %ymm14
	vpsllq	$3, %ymm0, %ymm10
	vpor	%ymm3, %ymm11, %ymm7
	xorq	%rdi, %r14
	vpor	%ymm14, %ymm10, %ymm12
	vpbroadcastq	%xmm15, %ymm11
	vpermq	$144, %ymm1, %ymm9
	xorq	%r14, %rax
	vextracti128	$0x1, %ymm1, %xmm3
	vpxor	%ymm12, %ymm7, %ymm8
	vpblendd	$3, %ymm11, %ymm9, %ymm14
	vpextrq	$1, %xmm3, %rsi
	vpand	%ymm2, %ymm11, %ymm0
	vpxor	%ymm8, %ymm13, %ymm13
	xorq	%rdi, %rsi
	vpxor	%ymm14, %ymm12, %ymm10
	vextracti128	$0x1, %ymm13, %xmm7
	vpxor	%ymm10, %ymm0, %ymm12
	vmovq	%rax, %xmm3
	vmovq	%rsi, %xmm10
	vpextrq	$1, %xmm7, %r8
	vpbroadcastq	%xmm3, %ymm15
	vpbroadcastq	%xmm10, %ymm7
	vextracti128	$0x1, %ymm12, %xmm0
	vpermq	$144, %ymm12, %ymm3
	vpermq	$144, %ymm13, %ymm13
	movq	%r8, %rax
	vpblendd	$3, %ymm15, %ymm13, %ymm11
	vpand	%ymm2, %ymm15, %ymm9
	vpextrq	$1, %xmm0, %rdi
	vpblendd	$3, %ymm7, %ymm3, %ymm15
	vpxor	%ymm11, %ymm9, %ymm14
	vpand	%ymm2, %ymm7, %ymm2
	xorq	%r14, %rdi
	vpxor	%ymm15, %ymm8, %ymm8
	xorq	%r8, %rcx
	vpxor	%ymm14, %ymm1, %ymm1
	xorq	%rdi, %rax
	vpxor	%ymm8, %ymm2, %ymm13
	vpxor	%ymm1, %ymm12, %ymm12
	xorq	%rcx, %rsi
	vpxor	%ymm13, %ymm14, %ymm11
	vpand	.LC3(%rip), %ymm13, %ymm0
	vpand	.LC4(%rip), %ymm13, %ymm7
	vpand	.LC1(%rip), %ymm12, %ymm2
	leaq	0(,%rax,8), %r14
	vpand	%ymm5, %ymm11, %ymm5
	shrq	%rax
	vpand	.LC2(%rip), %ymm12, %ymm12
	vpand	%ymm6, %ymm11, %ymm6
	andq	%r13, %r14
	andq	%r12, %rax
	leaq	0(,%rdi,4), %r13
	shrq	$2, %rdi
	leaq	(%rsi,%rsi), %r12
	orq	%rax, %r14
	andq	%r11, %rdi
	vpsrlq	$1, %ymm5, %ymm9
	andq	%rbx, %r13
	shrq	$3, %rsi
	vpsllq	$3, %ymm6, %ymm14
	vpsrlq	$2, %ymm0, %ymm3
	orq	%rdi, %r13
	andq	%r10, %r12
	vpxor	.LC16(%rip), %ymm1, %ymm1
	vpsllq	$2, %ymm7, %ymm15
	vpor	%ymm9, %ymm14, %ymm10
	andq	%r9, %rsi
	vpsrlq	$3, %ymm2, %ymm8
	vpsllq	$1, %ymm12, %ymm11
	vpor	%ymm3, %ymm15, %ymm13
	orq	%r12, %rsi
	vpor	%ymm8, %ymm11, %ymm5
	vpunpcklqdq	%ymm10, %ymm1, %ymm6
	vpunpckhqdq	%ymm10, %ymm1, %ymm14
	movq	%r15, %r12
	vpunpcklqdq	%ymm5, %ymm13, %ymm10
	vpunpckhqdq	%ymm5, %ymm13, %ymm0
	vmovq	%r13, %xmm7
	movabsq	$-1152657617789587456, %r13
	vmovq	%rcx, %xmm8
	vperm2i128	$32, %ymm10, %ymm6, %ymm9
	vperm2i128	$32, %ymm0, %ymm14, %ymm3
	vpinsrq	$1, %rsi, %xmm7, %xmm2
	vpinsrq	$1, %r14, %xmm8, %xmm12
	vperm2i128	$49, %ymm10, %ymm6, %ymm15
	vinserti128	$0x1, %xmm2, %ymm12, %ymm1
	vpxor	%ymm3, %ymm9, %ymm5
	vperm2i128	$49, %ymm0, %ymm14, %ymm13
	vpxor	%ymm1, %ymm15, %ymm11
	vpxor	%ymm1, %ymm9, %ymm7
	vpxor	%ymm4, %ymm5, %ymm4
	vpand	%ymm1, %ymm3, %ymm5
	vpxor	%ymm13, %ymm4, %ymm10
	vpandn	%ymm1, %ymm15, %ymm6
	vpand	%ymm7, %ymm4, %ymm2
	vpxor	%ymm11, %ymm5, %ymm4
	vpor	%ymm15, %ymm9, %ymm14
	vpxor	%ymm2, %ymm6, %ymm8
	vpand	%ymm1, %ymm9, %ymm9
	vpxor	%ymm13, %ymm4, %ymm6
	vpand	%ymm13, %ymm11, %ymm13
	vpor	%ymm7, %ymm10, %ymm11
	vpand	%ymm10, %ymm15, %ymm0
	vpxor	%ymm14, %ymm9, %ymm1
	vpor	%ymm6, %ymm3, %ymm2
	vpxor	%ymm13, %ymm11, %ymm10
	vpxor	%ymm14, %ymm3, %ymm3
	vpxor	%ymm6, %ymm0, %ymm6
	vpxor	%ymm0, %ymm8, %ymm12
	vpxor	%ymm13, %ymm3, %ymm14
	vpxor	%ymm2, %ymm1, %ymm8
	vpxor	%ymm15, %ymm10, %ymm15
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm14, %ymm15, %ymm5
	vpunpcklqdq	%ymm8, %ymm12, %ymm9
	vpextrq	$1, %xmm6, %rbx
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm6, %r10
	vperm2i128	$32, %ymm5, %ymm12, %ymm4
	vperm2i128	$32, %ymm7, %ymm9, %ymm2
	vextracti128	$0x1, %ymm6, %xmm1
	xorq	%rbx, %r10
	vpxor	%ymm4, %ymm2, %ymm11
	vmovq	%r10, %xmm14
	vperm2i128	$49, %ymm7, %ymm9, %ymm8
	movq	%r10, %rdi
	vperm2i128	$49, %ymm5, %ymm12, %ymm13
	vpbroadcastq	%xmm14, %ymm9
	vextracti128	$0x1, %ymm11, %xmm3
	vmovdqa	.LC0(%rip), %ymm2
	vpextrq	$1, %xmm1, %rcx
	vpermq	$144, %ymm11, %ymm12
	vmovq	%xmm1, %r9
	movabsq	$-4222189076152336, %r10
	xorq	%rcx, %r9
	vpxor	%ymm13, %ymm8, %ymm10
	vpblendd	$3, %ymm9, %ymm12, %ymm7
	vpextrq	$1, %xmm3, %r11
	vpxor	%ymm10, %ymm4, %ymm15
	vpxor	%ymm7, %ymm13, %ymm5
	xorq	%r9, %rbx
	vpand	%ymm2, %ymm9, %ymm4
	vmovq	%rbx, %xmm6
	vpermq	$144, %ymm15, %ymm1
	xorq	%r11, %rcx
	vpxor	%ymm5, %ymm4, %ymm8
	vmovq	%rcx, %xmm12
	vpbroadcastq	%xmm6, %ymm0
	movabsq	$-71777214294589696, %rbx
	vpbroadcastq	%xmm12, %ymm7
	vpermq	$144, %ymm8, %ymm4
	vpand	%ymm2, %ymm0, %ymm3
	movabsq	$71777214294589695, %r11
	vpblendd	$3, %ymm7, %ymm4, %ymm5
	vextracti128	$0x1, %ymm15, %xmm13
	vextracti128	$0x1, %ymm8, %xmm9
	vpblendd	$3, %ymm0, %ymm1, %ymm15
	vpextrq	$1, %xmm13, %rsi
	vpxor	%ymm5, %ymm10, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpand	%ymm2, %ymm7, %ymm13
	vpextrq	$1, %xmm9, %r8
	xorq	%rsi, %rdi
	vpxor	%ymm10, %ymm13, %ymm6
	vpxor	%ymm14, %ymm11, %ymm11
	xorq	%r9, %r8
	xorq	%rdi, %rcx
	vpxor	%ymm6, %ymm14, %ymm0
	vpxor	%ymm11, %ymm8, %ymm8
	vmovq	%r8, %xmm12
	xorq	%r8, %rsi
	vpunpcklqdq	%ymm0, %ymm11, %ymm15
	vpunpckhqdq	%ymm0, %ymm11, %ymm3
	vpunpcklqdq	%ymm8, %ymm6, %ymm14
	movabsq	$4222189076152335, %r9
	vpunpckhqdq	%ymm8, %ymm6, %ymm1
	vmovq	%rdi, %xmm4
	vperm2i128	$32, %ymm14, %ymm15, %ymm5
	vperm2i128	$32, %ymm1, %ymm3, %ymm6
	vpinsrq	$1, %rcx, %xmm12, %xmm7
	vpinsrq	$1, %rsi, %xmm4, %xmm13
	vperm2i128	$49, %ymm14, %ymm15, %ymm9
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vperm2i128	$49, %ymm1, %ymm3, %ymm15
	vpxor	%ymm6, %ymm5, %ymm8
	vinserti128	$0x1, %xmm7, %ymm13, %ymm3
	vpor	%ymm9, %ymm5, %ymm10
	vpxor	%ymm4, %ymm8, %ymm14
	vpxor	%ymm3, %ymm5, %ymm12
	vpandn	%ymm3, %ymm9, %ymm1
	vpand	%ymm12, %ymm14, %ymm13
	vpxor	%ymm3, %ymm9, %ymm11
	vpxor	%ymm15, %ymm14, %ymm0
	vpxor	%ymm13, %ymm1, %ymm8
	vpand	%ymm3, %ymm6, %ymm1
	vpand	%ymm0, %ymm9, %ymm7
	vpxor	%ymm11, %ymm1, %ymm13
	vpxor	%ymm7, %ymm8, %ymm14
	vpand	%ymm3, %ymm5, %ymm5
	vpxor	%ymm15, %ymm13, %ymm8
	vpand	%ymm15, %ymm11, %ymm15
	vpor	%ymm12, %ymm0, %ymm11
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm10, %ymm5, %ymm3
	vpor	%ymm8, %ymm6, %ymm1
	vpextrq	$1, %xmm7, %r14
	vpxor	%ymm15, %ymm11, %ymm0
	vpxor	%ymm10, %ymm6, %ymm6
	vpxor	%ymm1, %ymm3, %ymm13
	vpxor	%ymm15, %ymm6, %ymm12
	vpxor	%ymm9, %ymm0, %ymm9
	movq	%r14, %r15
	salq	$4, %r15
	shrq	$12, %r14
	vpunpckhqdq	%ymm12, %ymm9, %ymm3
	vpunpcklqdq	%ymm13, %ymm14, %ymm10
	vextracti128	$0x1, %ymm7, %xmm0
	vpunpckhqdq	%ymm13, %ymm14, %ymm14
	andq	%r9, %r14
	andq	%r10, %r15
	vpunpcklqdq	%ymm12, %ymm9, %ymm5
	vperm2i128	$32, %ymm3, %ymm14, %ymm15
	vpextrq	$1, %xmm0, %rsi
	orq	%r14, %r15
	vperm2i128	$49, %ymm5, %ymm10, %ymm11
	vperm2i128	$32, %ymm5, %ymm10, %ymm13
	vpand	.LC8(%rip), %ymm15, %ymm9
	movq	%rsi, %r14
	vpand	.LC9(%rip), %ymm15, %ymm12
	shrq	$4, %rsi
	vmovq	%xmm0, %r8
	salq	$12, %r14
	vpsrlq	$12, %ymm9, %ymm6
	vperm2i128	$49, %ymm3, %ymm14, %ymm1
	movq	%rsi, %rax
	andq	%r13, %r14
	vpsllq	$4, %ymm12, %ymm10
	vmovq	%r9, %xmm9
	andq	%r12, %rax
	vmovq	%xmm7, %rdi
	vmovq	%r10, %xmm7
	vpor	%ymm6, %ymm10, %ymm14
	movq	%r14, %rsi
	vpbroadcastq	%xmm9, %ymm6
	vpshufb	.LXRH_bswap16(%rip), %ymm11, %ymm3
	movq	%r8, %r14
	orq	%rax, %rsi
	vpbroadcastq	%xmm7, %ymm5
	xorq	%r15, %rdi
	salq	$8, %r14
	vpxor	%ymm14, %ymm13, %ymm13
	vpand	%ymm5, %ymm1, %ymm8
	shrq	$8, %r8
	vpand	%ymm6, %ymm1, %ymm1
	andq	%rbx, %r14
	andq	%r11, %r8
	vmovq	%rdi, %xmm7
	vpsrlq	$4, %ymm8, %ymm0
	vpsllq	$12, %ymm1, %ymm12
	vpbroadcastq	%xmm7, %ymm8
	orq	%r8, %r14
	vpor	%ymm0, %ymm12, %ymm10
	vpermq	$144, %ymm13, %ymm0
	xorq	%rsi, %r14
	vextracti128	$0x1, %ymm13, %xmm11
	vpand	%ymm2, %ymm8, %ymm12
	xorq	%r14, %r15
	vpxor	%ymm10, %ymm3, %ymm15
	vpblendd	$3, %ymm8, %ymm0, %ymm9
	vpextrq	$1, %xmm11, %rcx
	vpxor	%ymm15, %ymm14, %ymm14
	vpxor	%ymm9, %ymm10, %ymm1
	vmovq	%r15, %xmm11
	xorq	%rsi, %rcx
	vpxor	%ymm1, %ymm12, %ymm10
	vextracti128	$0x1, %ymm14, %xmm3
	vmovq	%rcx, %xmm1
	vpbroadcastq	%xmm11, %ymm7
	vpextrq	$1, %xmm3, %rax
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm1, %ymm3
	vpermq	$144, %ymm14, %ymm14
	vpand	%ymm2, %ymm7, %ymm0
	xorq	%rax, %rdi
	vpblendd	$3, %ymm7, %ymm14, %ymm8
	vpblendd	$3, %ymm3, %ymm11, %ymm7
	vpand	%ymm2, %ymm3, %ymm14
	xorq	%rdi, %rcx
	vextracti128	$0x1, %ymm10, %xmm9
	vpxor	%ymm7, %ymm15, %ymm15
	vpxor	%ymm8, %ymm0, %ymm12
	movq	%rcx, %rsi
	vpextrq	$1, %xmm9, %r15
	vpxor	%ymm15, %ymm14, %ymm9
	vpxor	%ymm12, %ymm13, %ymm13
	salq	$4, %rsi
	vpxor	%ymm9, %ymm12, %ymm0
	xorq	%r14, %r15
	vpxor	%ymm13, %ymm10, %ymm10
	shrq	$12, %rcx
	xorq	%r15, %rax
	vpand	%ymm5, %ymm0, %ymm8
	vpand	%ymm6, %ymm0, %ymm1
	movq	%r15, %r14
	movq	%rax, %r8
	salq	$8, %r14
	andq	%r9, %rcx
	shrq	$8, %r15
	vpsrlq	$4, %ymm8, %ymm12
	andq	%rbx, %r14
	salq	$12, %r8
	andq	%r11, %r15
	vpsllq	$12, %ymm1, %ymm3
	shrq	$4, %rax
	andq	%r13, %r8
	vpor	%ymm12, %ymm3, %ymm11
	orq	%r15, %r14
	andq	%r12, %rax
	vpand	.LC8(%rip), %ymm10, %ymm0
	andq	%r10, %rsi
	orq	%r8, %rax
	vpand	.LC9(%rip), %ymm10, %ymm10
	vpxor	.LC17(%rip), %ymm13, %ymm13
	orq	%rcx, %rsi
	vpsrlq	$12, %ymm0, %ymm8
	vpsllq	$4, %ymm10, %ymm1
	vpshufb	.LXRH_bswap16(%rip), %ymm9, %ymm12
	vpunpcklqdq	%ymm11, %ymm13, %ymm7
	vpor	%ymm8, %ymm1, %ymm3
	vpunpckhqdq	%ymm11, %ymm13, %ymm11
	vpunpcklqdq	%ymm3, %ymm12, %ymm14
	vpunpckhqdq	%ymm3, %ymm12, %ymm15
	vmovq	%r14, %xmm12
	vmovq	%rdi, %xmm3
	vperm2i128	$32, %ymm14, %ymm7, %ymm8
	vperm2i128	$32, %ymm15, %ymm11, %ymm9
	vpinsrq	$1, %rsi, %xmm12, %xmm1
	vpinsrq	$1, %rax, %xmm3, %xmm13
	vperm2i128	$49, %ymm14, %ymm7, %ymm10
	vperm2i128	$49, %ymm15, %ymm11, %ymm0
	vinserti128	$0x1, %xmm1, %ymm13, %ymm7
	vpxor	%ymm9, %ymm8, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %r15
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rcx
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	xorq	%r15, %rcx
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%rcx, %xmm8
	vpextrq	$1, %xmm9, %rax
	vpxor	%ymm10, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpextrq	$1, %xmm7, %r8
	xorq	%rax, %rdi
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm0, %ymm3
	xorq	%rdi, %r15
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	xorq	%r8, %rax
	vpxor	%ymm1, %ymm3, %ymm11
	vmovq	%r15, %xmm7
	vmovq	%rax, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vpextrq	$1, %xmm9, %r14
	vpbroadcastq	%xmm1, %ymm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm13
	xorq	%r14, %rcx
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpand	%ymm2, %ymm7, %ymm12
	xorq	%rcx, %rax
	vpxor	%ymm0, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm10, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rsi
	xorq	%rdi, %rsi
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpcklqdq	%ymm11, %ymm13, %ymm8
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	xorq	%rsi, %r14
	vpunpckhqdq	%ymm0, %ymm14, %ymm1
	vmovq	%rsi, %xmm12
	vmovq	%rcx, %xmm14
	vperm2i128	$32, %ymm3, %ymm1, %ymm9
	vperm2i128	$32, %ymm8, %ymm15, %ymm7
	vpinsrq	$1, %rax, %xmm12, %xmm13
	vpinsrq	$1, %r14, %xmm14, %xmm11
	vperm2i128	$49, %ymm3, %ymm1, %ymm1
	vpxor	%ymm9, %ymm7, %ymm0
	vinserti128	$0x1, %xmm13, %ymm11, %ymm3
	vperm2i128	$49, %ymm8, %ymm15, %ymm10
	vpxor	%ymm4, %ymm0, %ymm11
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm1, %ymm11, %ymm0
	vpandn	%ymm3, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm9, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm0, %ymm10, %ymm8
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpor	%ymm11, %ymm9, %ymm3
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vpxor	%ymm10, %ymm0, %ymm10
	vpxor	%ymm8, %ymm12, %ymm12
	vpunpcklqdq	%ymm14, %ymm10, %ymm3
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm7
	vperm2i128	$32, %ymm7, %ymm12, %ymm10
	vperm2i128	$49, %ymm3, %ymm15, %ymm1
	vperm2i128	$49, %ymm7, %ymm12, %ymm0
	vperm2i128	$32, %ymm3, %ymm15, %ymm13
	vpextrq	$1, %xmm8, %rdi
	vmovq	%xmm8, %r15
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm12

	vextracti128	$0x1, %ymm8, %xmm9
	rorx	$48, %rdi, %rax
	xorq	%rax, %r15
	vpshufb	.LXRH_rot48(%rip), %ymm0, %ymm11

	vpshufd	$177, %ymm1, %ymm0
	vpxor	%ymm12, %ymm13, %ymm13
	vmovq	%xmm9, %r8
	vpextrq	$1, %xmm9, %rcx
	vpxor	%ymm11, %ymm0, %ymm9
	vpermq	$144, %ymm13, %ymm7
	rorx	$32, %r8, %rdi
	vmovq	%r15, %xmm15
	vpxor	%ymm9, %ymm12, %ymm14
	vextracti128	$0x1, %ymm13, %xmm8
	rorx	$16, %rcx, %r14
	vpbroadcastq	%xmm15, %ymm12
	xorq	%r14, %rdi
	vpextrq	$1, %xmm8, %r8
	vpblendd	$3, %ymm12, %ymm7, %ymm10
	vpand	%ymm2, %ymm12, %ymm1
	xorq	%r14, %r8
	xorq	%rdi, %rax
	vpxor	%ymm10, %ymm11, %ymm3
	vextracti128	$0x1, %ymm14, %xmm0
	vmovq	%rax, %xmm8
	vpxor	%ymm3, %ymm1, %ymm11
	vmovq	%r8, %xmm3
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm0, %rsi
	vpermq	$144, %ymm11, %ymm8
	vpbroadcastq	%xmm3, %ymm0
	vpermq	$144, %ymm14, %ymm14
	vpand	%ymm2, %ymm15, %ymm7
	vextracti128	$0x1, %ymm11, %xmm1
	xorq	%rsi, %r15
	vpblendd	$3, %ymm15, %ymm14, %ymm12
	vpblendd	$3, %ymm0, %ymm8, %ymm15
	vpand	%ymm2, %ymm0, %ymm14
	xorq	%r15, %r8
	vpxor	%ymm12, %ymm7, %ymm10
	vpxor	%ymm15, %ymm9, %ymm9
	vpextrq	$1, %xmm1, %rcx
	vpxor	%ymm9, %ymm14, %ymm12
	vpxor	%ymm10, %ymm13, %ymm13
	xorq	%rdi, %rcx
	rorq	$48, %r8
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm12, %ymm10, %ymm7
	xorq	%rcx, %rsi
	rorx	$32, %rcx, %rax
	vpshufb	.LXRH_rot48(%rip), %ymm7, %ymm3

	rorq	$16, %rsi
	vpxor	.LC18(%rip), %ymm13, %ymm13
	vpshufd	$177, %ymm12, %ymm15

	vpshufb	.LXRH_rot16(%rip), %ymm11, %ymm12

	vpunpcklqdq	%ymm3, %ymm13, %ymm11
	vpunpckhqdq	%ymm3, %ymm13, %ymm7
	vpunpckhqdq	%ymm12, %ymm15, %ymm1
	vpunpcklqdq	%ymm12, %ymm15, %ymm10
	vmovq	%rax, %xmm3
	vmovq	%r15, %xmm14
	vperm2i128	$32, %ymm1, %ymm7, %ymm9
	vperm2i128	$32, %ymm10, %ymm11, %ymm8
	vpinsrq	$1, %r8, %xmm3, %xmm15
	vpinsrq	$1, %rsi, %xmm14, %xmm12
	vperm2i128	$49, %ymm10, %ymm11, %ymm10
	vperm2i128	$49, %ymm1, %ymm7, %ymm0
	vpxor	%ymm9, %ymm8, %ymm11
	vinserti128	$0x1, %xmm15, %ymm12, %ymm7
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %r15
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	xorq	%rsi, %r15
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%r15, %xmm8
	vpextrq	$1, %xmm9, %r14
	vpxor	%ymm10, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rcx
	vpextrq	$1, %xmm7, %r8
	xorq	%r14, %rcx
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm0, %ymm3
	xorq	%r8, %r14
	xorq	%rcx, %rsi
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	vpxor	%ymm1, %ymm3, %ymm11
	vmovq	%rsi, %xmm7
	vmovq	%r14, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vpextrq	$1, %xmm9, %rax
	vpbroadcastq	%xmm1, %ymm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm13
	xorq	%rax, %r15
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpand	%ymm2, %ymm7, %ymm12
	xorq	%r15, %r14
	vpxor	%ymm0, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm10, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rdi
	xorq	%rcx, %rdi
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpcklqdq	%ymm11, %ymm13, %ymm3
	vpunpckhqdq	%ymm11, %ymm13, %ymm1
	xorq	%rdi, %rax
	vpunpckhqdq	%ymm0, %ymm14, %ymm9
	vmovq	%rdi, %xmm12
	vmovq	%r15, %xmm14
	movabsq	$-8608480567731124088, %r15
	vperm2i128	$32, %ymm1, %ymm9, %ymm8
	vperm2i128	$32, %ymm3, %ymm15, %ymm7
	vpinsrq	$1, %r14, %xmm12, %xmm13
	movabsq	$1229782938247303441, %rdi
	vpinsrq	$1, %rax, %xmm14, %xmm11
	vperm2i128	$49, %ymm3, %ymm15, %ymm10
	vperm2i128	$49, %ymm1, %ymm9, %ymm0
	movabsq	$-1229782938247303442, %r14
	vinserti128	$0x1, %xmm13, %ymm11, %ymm1
	vpxor	%ymm8, %ymm7, %ymm15
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm4, %ymm15, %ymm11
	vpxor	%ymm1, %ymm7, %ymm15
	vpandn	%ymm1, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm1, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm1, %ymm8, %ymm11
	vpand	%ymm1, %ymm7, %ymm7
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm13
	vpand	%ymm9, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm0
	vpxor	%ymm14, %ymm7, %ymm7
	vpor	%ymm11, %ymm8, %ymm1
	vpxor	%ymm13, %ymm0, %ymm9
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm1, %ymm7, %ymm7
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vmovq	%r15, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vperm2i128	$49, %ymm12, %ymm15, %ymm1
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	.LC37(%rip), %ymm12
	vpbroadcastq	%xmm9, %ymm9
	vpxor	%ymm11, %ymm3, %ymm3
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpand	%ymm9, %ymm10, %ymm14
	vpand	%ymm12, %ymm10, %ymm13
	vextracti128	$0x1, %ymm3, %xmm0
	vpsrlq	$3, %ymm14, %ymm15
	vpsllq	$1, %ymm13, %ymm10
	vmovq	%xmm3, %rsi
	vmovq	%xmm0, -8(%rsp)
	vpextrq	$1, %xmm3, %rcx
	vpor	%ymm15, %ymm10, %ymm3
	vpextrq	$1, %xmm0, %rax
	vpbroadcastq	.LC38(%rip), %ymm10
	leaq	(%rcx,%rcx), %r8
	shrq	$3, %rcx
	vpxor	%ymm3, %ymm8, %ymm8
	vpand	%ymm10, %ymm1, %ymm11
	andq	%rdi, %rcx
	andq	%r14, %r8
	movabsq	$3689348814741910323, %r14
	vpsrlq	$2, %ymm11, %ymm15
	orq	%rcx, %r8
	vmovq	%r14, %xmm0
	vpbroadcastq	.LC39(%rip), %ymm11
	leaq	0(,%rax,8), %rdi
	shrq	%rax
	vpbroadcastq	%xmm0, %ymm13
	xorq	%r8, %rsi
	vpand	%ymm11, %ymm7, %ymm14
	andq	%r15, %rdi
	vpand	%ymm13, %ymm1, %ymm1
	movabsq	$8608480567731124087, %rcx
	vpsrlq	$1, %ymm14, %ymm0
	vpsllq	$2, %ymm1, %ymm1
	andq	%rcx, %rax
	vpbroadcastq	.LC40(%rip), %ymm14
	orq	%rdi, %rax
	movq	-8(%rsp), %rdi
	vpor	%ymm15, %ymm1, %ymm15
	movabsq	$-3689348814741910324, %rcx
	vpand	%ymm14, %ymm7, %ymm7
	leaq	0(,%rdi,4), %rdi
	vpsllq	$3, %ymm7, %ymm7
	andq	%rcx, %rdi
	movq	-8(%rsp), %rcx
	vpor	%ymm0, %ymm7, %ymm7
	vmovq	%rsi, %xmm0
	vpxor	%ymm7, %ymm15, %ymm1
	shrq	$2, %rcx
	vpxor	%ymm1, %ymm3, %ymm15
	vextracti128	$0x1, %ymm8, %xmm3
	andq	%r14, %rcx
	orq	%rcx, %rdi
	vpextrq	$1, %xmm3, %rcx
	vpbroadcastq	%xmm0, %ymm3
	vpermq	$144, %ymm8, %ymm0
	xorq	%rax, %rdi
	xorq	%rcx, %rax
	vpblendd	$3, %ymm3, %ymm0, %ymm0
	xorq	%rdi, %r8
	vpand	%ymm2, %ymm3, %ymm3
	vpxor	%ymm0, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm0
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm3, %ymm3
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm0, %rcx
	vpbroadcastq	%xmm7, %ymm0
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm0, %ymm15, %ymm7
	vextracti128	$0x1, %ymm3, %xmm15
	vpand	%ymm2, %ymm0, %ymm0
	vpextrq	$1, %xmm15, %r8
	vpxor	%ymm7, %ymm0, %ymm0
	vmovq	%rax, %xmm7
	xorq	%rsi, %rax
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm3, %ymm15
	vpxor	%ymm0, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm2, %ymm7, %ymm7
	leaq	0(,%rcx,8), %r8
	vpxor	%ymm15, %ymm1, %ymm1
	shrq	%rcx
	vpxor	%ymm8, %ymm3, %ymm3
	vpxor	%ymm1, %ymm7, %ymm15
	vpand	%ymm9, %ymm3, %ymm9
	vpand	%ymm12, %ymm3, %ymm12
	andq	%r15, %r8
	vpxor	%ymm15, %ymm0, %ymm0
	vpand	%ymm10, %ymm15, %ymm10
	vpand	%ymm13, %ymm15, %ymm13
	movabsq	$8608480567731124087, %r15
	vpand	%ymm11, %ymm0, %ymm11
	vpsllq	$2, %ymm13, %ymm15
	vpand	%ymm14, %ymm0, %ymm14
	andq	%r15, %rcx
	orq	%r8, %rcx
	leaq	0(,%rdi,4), %r8
	movabsq	$-3689348814741910324, %r15
	shrq	$2, %rdi
	vpsrlq	$1, %ymm11, %ymm7
	vpsllq	$3, %ymm14, %ymm1
	andq	%r14, %rdi
	andq	%r15, %r8
	orq	%rdi, %r8
	leaq	(%rax,%rax), %rdi
	vpor	%ymm7, %ymm1, %ymm0
	movabsq	$-1229782938247303442, %r14
	vpxor	.LC19(%rip), %ymm8, %ymm8
	vpsrlq	$2, %ymm10, %ymm11
	andq	%r14, %rdi
	shrq	$3, %rax
	vpsrlq	$3, %ymm9, %ymm7
	vpsllq	$1, %ymm12, %ymm3
	vpor	%ymm11, %ymm15, %ymm14
	movabsq	$1229782938247303441, %r14
	vpor	%ymm7, %ymm3, %ymm1
	vpunpcklqdq	%ymm0, %ymm8, %ymm10
	vmovq	%r8, %xmm15
	andq	%r14, %rax
	vpunpcklqdq	%ymm1, %ymm14, %ymm11
	vpunpckhqdq	%ymm1, %ymm14, %ymm13
	vpunpckhqdq	%ymm0, %ymm8, %ymm0
	orq	%rax, %rdi
	vmovq	%rsi, %xmm7
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm0, %ymm9
	vpinsrq	$1, %rdi, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm7, %xmm12
	vpxor	%ymm9, %ymm8, %ymm3
	vinserti128	$0x1, %xmm14, %ymm12, %ymm7
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vperm2i128	$49, %ymm13, %ymm0, %ymm0
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm3, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rcx
	vperm2i128	$32, %ymm7, %ymm10, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	xorq	%rsi, %rcx
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm10
	vmovq	%rcx, %xmm8
	vpextrq	$1, %xmm9, %rax
	vpxor	%ymm10, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpextrq	$1, %xmm7, %r8
	xorq	%rax, %rdi
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpand	%ymm2, %ymm0, %ymm3
	xorq	%r8, %rax
	xorq	%rdi, %rsi
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	vpxor	%ymm1, %ymm3, %ymm11
	vmovq	%rsi, %xmm7
	vmovq	%rax, %xmm1
	vpbroadcastq	%xmm7, %ymm8
	vpextrq	$1, %xmm9, %r14
	vpbroadcastq	%xmm1, %ymm7
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm13
	xorq	%r14, %rcx
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpblendd	$3, %ymm7, %ymm9, %ymm8
	vpand	%ymm2, %ymm7, %ymm12
	xorq	%rcx, %rax
	vpxor	%ymm0, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm10, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rsi
	xorq	%rdi, %rsi
	vpxor	%ymm14, %ymm11, %ymm3
	vpxor	%ymm13, %ymm15, %ymm11
	vpunpcklqdq	%ymm11, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm14, %ymm0
	vpunpcklqdq	%ymm3, %ymm13, %ymm1
	xorq	%rsi, %r14
	vpunpckhqdq	%ymm3, %ymm13, %ymm7
	vmovq	%rsi, %xmm12
	vmovq	%rcx, %xmm14
	vperm2i128	$32, %ymm7, %ymm0, %ymm9
	vperm2i128	$32, %ymm1, %ymm15, %ymm8
	vpinsrq	$1, %rax, %xmm12, %xmm13
	vpinsrq	$1, %r14, %xmm14, %xmm3
	vperm2i128	$49, %ymm1, %ymm15, %ymm10
	vpxor	%ymm9, %ymm8, %ymm11
	vperm2i128	$49, %ymm7, %ymm0, %ymm1
	vinserti128	$0x1, %xmm13, %ymm3, %ymm7
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm1, %ymm11, %ymm0
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpand	%ymm0, %ymm10, %ymm3
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpxor	%ymm11, %ymm3, %ymm3
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm0, %ymm15
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm7, %ymm8, %ymm8
	vpextrq	$1, %xmm3, %rdi
	vpunpcklqdq	%ymm8, %ymm12, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	movq	%rdi, %rax
	vpunpcklqdq	%ymm14, %ymm15, %ymm7
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm0
	salq	$4, %rax
	vextracti128	$0x1, %ymm3, %xmm9
	shrq	$12, %rdi
	andq	%r10, %rax
	vperm2i128	$49, %ymm7, %ymm10, %ymm15
	vpand	.LC8(%rip), %ymm13, %ymm14
	andq	%r9, %rdi
	vpand	.LC9(%rip), %ymm13, %ymm12
	vpextrq	$1, %xmm9, %r14
	vperm2i128	$32, %ymm7, %ymm10, %ymm1
	vmovq	%xmm3, %rcx
	orq	%rdi, %rax
	vpsrlq	$12, %ymm14, %ymm10
	vpsllq	$4, %ymm12, %ymm7
	movq	%r14, %rdi
	xorq	%rax, %rcx
	vpor	%ymm10, %ymm7, %ymm13
	salq	$12, %rdi
	shrq	$4, %r14
	vpxor	%ymm13, %ymm1, %ymm1
	vmovq	%xmm9, %r8
	andq	%r13, %rdi
	vpand	%ymm5, %ymm0, %ymm9
	andq	%r12, %r14
	vpand	%ymm6, %ymm0, %ymm0
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm7
	vmovq	%rcx, %xmm15
	orq	%r14, %rdi
	vpsrlq	$4, %ymm9, %ymm14
	vpbroadcastq	%xmm15, %ymm11
	movq	%r8, %r14
	vpsllq	$12, %ymm0, %ymm10
	vpermq	$144, %ymm1, %ymm9
	salq	$8, %r14
	vpand	%ymm2, %ymm11, %ymm0
	vpor	%ymm14, %ymm10, %ymm12
	shrq	$8, %r8
	andq	%rbx, %r14
	vextracti128	$0x1, %ymm1, %xmm3
	vpblendd	$3, %ymm11, %ymm9, %ymm14
	vpxor	%ymm12, %ymm7, %ymm8
	andq	%r11, %r8
	vpxor	%ymm14, %ymm12, %ymm10
	orq	%r8, %r14
	vpxor	%ymm8, %ymm13, %ymm13
	vpxor	%ymm10, %ymm0, %ymm12
	xorq	%rdi, %r14
	vextracti128	$0x1, %ymm13, %xmm7
	vextracti128	$0x1, %ymm12, %xmm0
	vpextrq	$1, %xmm3, %rsi
	vpermq	$144, %ymm13, %ymm13
	xorq	%r14, %rax
	xorq	%rdi, %rsi
	vpextrq	$1, %xmm7, %r8
	vpextrq	$1, %xmm0, %rdi
	xorq	%r14, %rdi
	vmovq	%rax, %xmm3
	movq	%r8, %rax
	xorq	%r8, %rcx
	xorq	%rdi, %rax
	vmovq	%rsi, %xmm10
	vpbroadcastq	%xmm3, %ymm15
	xorq	%rcx, %rsi
	vpbroadcastq	%xmm10, %ymm7
	vpermq	$144, %ymm12, %ymm3
	vpand	%ymm2, %ymm15, %ymm9
	movq	%rax, %r14
	salq	$12, %r14
	vpblendd	$3, %ymm15, %ymm13, %ymm11
	vpblendd	$3, %ymm7, %ymm3, %ymm15
	shrq	$4, %rax
	vpxor	%ymm11, %ymm9, %ymm14
	vpand	%ymm2, %ymm7, %ymm13
	vpxor	%ymm15, %ymm8, %ymm8
	andq	%r13, %r14
	movq	%rdi, %r13
	vpxor	%ymm8, %ymm13, %ymm11
	vpxor	%ymm14, %ymm1, %ymm1
	shrq	$8, %rdi
	salq	$8, %r13
	vpxor	%ymm11, %ymm14, %ymm9
	vpxor	%ymm1, %ymm12, %ymm12
	andq	%r12, %rax
	andq	%rbx, %r13
	movq	%rsi, %rbx
	andq	%r11, %rdi
	orq	%rax, %r14
	vpand	%ymm5, %ymm9, %ymm5
	vpand	%ymm6, %ymm9, %ymm6
	salq	$4, %rbx
	orq	%rdi, %r13
	vpshufb	.LXRH_bswap16(%rip), %ymm11, %ymm8
	andq	%r10, %rbx
	shrq	$12, %rsi
	vpand	.LC8(%rip), %ymm12, %ymm11
	vpand	.LC9(%rip), %ymm12, %ymm12
	andq	%r9, %rsi
	vpxor	.LC20(%rip), %ymm1, %ymm1
	vpsrlq	$4, %ymm5, %ymm14
	orq	%rbx, %rsi
	vpsllq	$12, %ymm6, %ymm0
	vpsrlq	$12, %ymm11, %ymm9
	vpor	%ymm14, %ymm0, %ymm10
	vpsllq	$4, %ymm12, %ymm5
	vpunpcklqdq	%ymm10, %ymm1, %ymm6
	vpor	%ymm9, %ymm5, %ymm14
	vpunpckhqdq	%ymm10, %ymm1, %ymm0
	vmovq	%r13, %xmm3
	vpunpckhqdq	%ymm14, %ymm8, %ymm7
	vpunpcklqdq	%ymm14, %ymm8, %ymm10
	vmovq	%rcx, %xmm12
	vperm2i128	$32, %ymm7, %ymm0, %ymm13
	vperm2i128	$32, %ymm10, %ymm6, %ymm15
	vpinsrq	$1, %rsi, %xmm3, %xmm11
	vpinsrq	$1, %r14, %xmm12, %xmm5
	vperm2i128	$49, %ymm10, %ymm6, %ymm8
	vpxor	%ymm13, %ymm15, %ymm6
	vinserti128	$0x1, %xmm11, %ymm5, %ymm14
	vperm2i128	$49, %ymm7, %ymm0, %ymm9
	vpxor	%ymm4, %ymm6, %ymm7
	vpxor	%ymm14, %ymm15, %ymm4
	vpxor	%ymm14, %ymm8, %ymm1
	vpxor	%ymm9, %ymm7, %ymm3
	vpandn	%ymm14, %ymm8, %ymm11
	vpand	%ymm4, %ymm7, %ymm12
	vpand	%ymm14, %ymm13, %ymm7
	vpxor	%ymm12, %ymm11, %ymm5
	vpxor	%ymm1, %ymm7, %ymm11
	vpor	%ymm8, %ymm15, %ymm10
	vpxor	%ymm9, %ymm11, %ymm7
	vpand	%ymm14, %ymm15, %ymm15
	vpand	%ymm9, %ymm1, %ymm9
	vpor	%ymm4, %ymm3, %ymm1
	vpand	%ymm3, %ymm8, %ymm0
	vpxor	%ymm10, %ymm15, %ymm14
	vpor	%ymm7, %ymm13, %ymm12
	vpxor	%ymm9, %ymm1, %ymm3
	vpxor	%ymm10, %ymm13, %ymm13
	vpxor	%ymm9, %ymm13, %ymm10
	vpxor	%ymm7, %ymm0, %ymm7
	vpxor	%ymm0, %ymm5, %ymm6
	vpxor	%ymm8, %ymm3, %ymm11
	vpxor	%ymm12, %ymm14, %ymm5
	vpextrq	$1, %xmm7, %r11
	vpunpcklqdq	%ymm5, %ymm6, %ymm15
	vpunpcklqdq	%ymm10, %ymm11, %ymm8
	vpunpckhqdq	%ymm5, %ymm6, %ymm6
	vpunpckhqdq	%ymm10, %ymm11, %ymm14
	vmovq	%xmm7, %r10
	vperm2i128	$32, %ymm8, %ymm15, %ymm12
	vperm2i128	$32, %ymm14, %ymm6, %ymm4
	vperm2i128	$49, %ymm8, %ymm15, %ymm9
	vextracti128	$0x1, %ymm7, %xmm5
	xorq	%r11, %r10
	vpxor	%ymm4, %ymm12, %ymm13
	vmovq	%r10, %xmm15
	vperm2i128	$49, %ymm14, %ymm6, %ymm11
	movq	%r10, %rax
	vpbroadcastq	%xmm15, %ymm6
	vpextrq	$1, %xmm5, %rcx
	vextracti128	$0x1, %ymm13, %xmm3
	vpermq	$144, %ymm13, %ymm8
	vmovq	%xmm5, %r9
	vpxor	%ymm11, %ymm9, %ymm10
	xorq	%rcx, %r9
	vpblendd	$3, %ymm6, %ymm8, %ymm14
	vpextrq	$1, %xmm3, %rsi
	vpxor	%ymm10, %ymm4, %ymm1
	xorq	%r9, %r11
	vpand	%ymm2, %ymm6, %ymm12
	xorq	%rsi, %rcx
	vpxor	%ymm14, %ymm11, %ymm4
	vmovq	%r11, %xmm7
	vmovq	%rcx, %xmm8
	vpxor	%ymm4, %ymm12, %ymm9
	vpbroadcastq	%xmm7, %ymm0
	vpbroadcastq	%xmm8, %ymm14
	vpermq	$144, %ymm9, %ymm12
	vpermq	$144, %ymm1, %ymm5
	vpand	%ymm2, %ymm0, %ymm3
	vpblendd	$3, %ymm14, %ymm12, %ymm4
	vextracti128	$0x1, %ymm1, %xmm11
	vpblendd	$3, %ymm0, %ymm5, %ymm1
	vpxor	%ymm1, %ymm3, %ymm15
	vextracti128	$0x1, %ymm9, %xmm6
	vpand	%ymm2, %ymm14, %ymm2
	vpxor	%ymm4, %ymm10, %ymm10
	vpxor	%ymm15, %ymm13, %ymm13
	vpextrq	$1, %xmm11, %r8
	vpextrq	$1, %xmm6, %rdi
	vpxor	%ymm10, %ymm2, %ymm11
	vpxor	%ymm13, %ymm9, %ymm9
	xorq	%r8, %rax
	xorq	%r9, %rdi
	vpxor	%ymm11, %ymm15, %ymm7
	vpunpcklqdq	%ymm9, %ymm11, %ymm0
	xorq	%rax, %rcx
	vpunpcklqdq	%ymm7, %ymm13, %ymm5
	vpunpckhqdq	%ymm7, %ymm13, %ymm15
	vpunpckhqdq	%ymm9, %ymm11, %ymm1
	xorq	%rdi, %r8
	vmovq	%rdi, %xmm8
	vmovq	%rax, %xmm4
	vperm2i128	$32, %ymm0, %ymm5, %ymm3
	vperm2i128	$32, %ymm1, %ymm15, %ymm6
	vpinsrq	$1, %rcx, %xmm8, %xmm12
	vpinsrq	$1, %r8, %xmm4, %xmm10
	vinserti128	$0x1, %xmm12, %ymm10, %ymm13
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vperm2i128	$49, %ymm0, %ymm5, %ymm14
	vpxor	%ymm6, %ymm3, %ymm9
	vperm2i128	$49, %ymm1, %ymm15, %ymm2
	vpxor	%ymm13, %ymm14, %ymm11
	vpxor	%ymm4, %ymm9, %ymm0
	vpxor	%ymm13, %ymm3, %ymm1
	vpand	%ymm13, %ymm6, %ymm9
	vpxor	%ymm2, %ymm0, %ymm5
	vpandn	%ymm13, %ymm14, %ymm7
	vpand	%ymm1, %ymm0, %ymm12
	vpxor	%ymm11, %ymm9, %ymm0
	vpor	%ymm14, %ymm3, %ymm15
	vpand	%ymm5, %ymm14, %ymm8
	vpxor	%ymm12, %ymm7, %ymm10
	vpand	%ymm13, %ymm3, %ymm3
	vpxor	%ymm2, %ymm0, %ymm12
	vpand	%ymm2, %ymm11, %ymm2
	vpor	%ymm1, %ymm5, %ymm11
	vpxor	%ymm8, %ymm10, %ymm7
	vpxor	%ymm15, %ymm3, %ymm13
	vpor	%ymm12, %ymm6, %ymm10
	vpxor	%ymm2, %ymm11, %ymm5
	vpxor	%ymm15, %ymm6, %ymm6
	vpxor	%ymm10, %ymm13, %ymm9
	vpxor	%ymm14, %ymm5, %ymm14
	vpxor	%ymm2, %ymm6, %ymm15
	vpunpcklqdq	%ymm9, %ymm7, %ymm13
	vpunpckhqdq	%ymm9, %ymm7, %ymm7
	vpunpckhqdq	%ymm15, %ymm14, %ymm10
	vpxor	%ymm12, %ymm8, %ymm8
	vpunpcklqdq	%ymm15, %ymm14, %ymm0
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vpextrq	$1, %xmm8, %r13
	vmovq	%xmm8, %r14
	vperm2i128	$32, %ymm0, %ymm13, %ymm3
	rorx	$48, %r13, %r11
	vpshufb	.LXRH_rot16(%rip), %ymm1, %ymm15
	xorq	%r11, %r14

	vperm2i128	$49, %ymm0, %ymm13, %ymm9
	vperm2i128	$49, %ymm10, %ymm7, %ymm11
	vextracti128	$0x1, %ymm8, %xmm2
	vmovq	%r14, %xmm12
	vpxor	%ymm15, %ymm3, %ymm3
	vpbroadcastq	%xmm12, %ymm5
	vmovq	%xmm2, %rbx
	vpextrq	$1, %xmm2, %r10
	vpshufd	$177, %ymm9, %ymm1
	vpermq	$144, %ymm3, %ymm2
	rorx	$32, %rbx, %r9

	vpshufb	.LXRH_rot48(%rip), %ymm11, %ymm10
	vpblendd	$3, %ymm5, %ymm2, %ymm14
	rorx	$16, %r10, %rcx

	vmovdqa	.LC0(%rip), %ymm2
	xorq	%rcx, %r9
	vextracti128	$0x1, %ymm3, %xmm8
	xorq	%r9, %r11
	vpxor	%ymm10, %ymm1, %ymm9
	vpextrq	$1, %xmm8, %rsi
	vpxor	%ymm14, %ymm10, %ymm13
	vpxor	%ymm9, %ymm15, %ymm11
	vmovq	%r11, %xmm7
	vpand	%ymm2, %ymm5, %ymm15
	xorq	%rcx, %rsi
	vpxor	%ymm13, %ymm15, %ymm10
	vpbroadcastq	%xmm7, %ymm0
	vmovq	%rsi, %xmm14
	vpermq	$144, %ymm11, %ymm1
	vpand	%ymm2, %ymm0, %ymm8
	vpbroadcastq	%xmm14, %ymm15
	vextracti128	$0x1, %ymm11, %xmm6
	vpermq	$144, %ymm10, %ymm13
	vpblendd	$3, %ymm0, %ymm1, %ymm11
	vpxor	%ymm11, %ymm8, %ymm12
	vpextrq	$1, %xmm6, %r8
	vpblendd	$3, %ymm15, %ymm13, %ymm6
	vpand	%ymm2, %ymm15, %ymm7
	vpxor	%ymm12, %ymm3, %ymm3
	vpxor	%ymm6, %ymm9, %ymm9
	xorq	%r8, %r14
	vpxor	%ymm9, %ymm7, %ymm11
	vextracti128	$0x1, %ymm10, %xmm5
	vpxor	%ymm3, %ymm10, %ymm10
	xorq	%r14, %rsi
	vpxor	%ymm11, %ymm12, %ymm0
	vpshufd	$177, %ymm11, %ymm6
	vpextrq	$1, %xmm5, %rax
	vmovdqu	80(%rdx), %ymm5

	xorq	%r9, %rax
	rorx	$48, %rsi, %rbx

	rorx	$32, %rax, %r13
	vpand	.LC33(%rip), %ymm5, %ymm9
	vpxor	.LC21(%rip), %ymm3, %ymm11
	vpshufb	.LXRH_rot48(%rip), %ymm0, %ymm12
	vmovdqu	40(%rdx), %ymm0
	vpand	.LC42(%rip), %ymm0, %ymm0
	vpxor	%ymm0, %ymm12, %ymm12
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm3
	xorq	%rax, %r8

	vpxor	120(%rdx), %ymm3, %ymm0
	vpxor	%ymm6, %ymm9, %ymm10
	xorq	112(%rdx), %r13
	xorq	152(%rdx), %rbx
	vmovq	%r13, %xmm13
	rorq	$16, %r8
	xorq	72(%rdx), %r8
	vpunpcklqdq	%ymm12, %ymm11, %ymm1
	vpunpckhqdq	%ymm12, %ymm11, %ymm8
	vpunpckhqdq	%ymm0, %ymm10, %ymm14
	movabsq	$-1229782938247303442, %r13
	vpunpcklqdq	%ymm0, %ymm10, %ymm12
	vmovq	%r14, %xmm6
	vperm2i128	$32, %ymm14, %ymm8, %ymm5
	vperm2i128	$32, %ymm12, %ymm1, %ymm15
	vpinsrq	$1, %rbx, %xmm13, %xmm11
	vpinsrq	$1, %r8, %xmm6, %xmm10
	movabsq	$-8608480567731124088, %rbx
	vperm2i128	$49, %ymm12, %ymm1, %ymm9
	vpxor	%ymm5, %ymm15, %ymm3
	vinserti128	$0x1, %xmm11, %ymm10, %ymm1
	vpxor	%ymm1, %ymm15, %ymm13
	vpxor	%ymm4, %ymm3, %ymm11
	vperm2i128	$49, %ymm14, %ymm8, %ymm7
	vpand	%ymm13, %ymm11, %ymm6
	vpandn	%ymm1, %ymm9, %ymm8
	vpxor	%ymm1, %ymm9, %ymm12
	vpxor	%ymm7, %ymm11, %ymm3
	vpxor	%ymm6, %ymm8, %ymm10
	vpand	%ymm1, %ymm5, %ymm8
	vpand	%ymm3, %ymm9, %ymm0
	vpxor	%ymm12, %ymm8, %ymm6
	vpor	%ymm9, %ymm15, %ymm14
	vpxor	%ymm0, %ymm10, %ymm11
	vpand	%ymm1, %ymm15, %ymm15
	vpxor	%ymm7, %ymm6, %ymm10
	vpand	%ymm7, %ymm12, %ymm7
	vpor	%ymm13, %ymm3, %ymm12
	vpxor	%ymm14, %ymm15, %ymm8
	vpor	%ymm10, %ymm5, %ymm1
	vpxor	%ymm7, %ymm12, %ymm3
	vpxor	%ymm14, %ymm5, %ymm5
	vpxor	%ymm1, %ymm8, %ymm6
	vpxor	%ymm7, %ymm5, %ymm14
	vpxor	%ymm10, %ymm0, %ymm10
	vpxor	%ymm9, %ymm3, %ymm9
	vpunpcklqdq	%ymm6, %ymm11, %ymm13
	vpextrq	$1, %xmm10, %r11
	vpunpcklqdq	%ymm14, %ymm9, %ymm15
	vpunpckhqdq	%ymm6, %ymm11, %ymm11
	vpunpckhqdq	%ymm14, %ymm9, %ymm8
	vmovq	%xmm10, %r10
	vperm2i128	$32, %ymm8, %ymm11, %ymm12
	vperm2i128	$32, %ymm15, %ymm13, %ymm7
	vperm2i128	$49, %ymm8, %ymm11, %ymm9
	vpxor	%ymm12, %ymm7, %ymm3
	vextracti128	$0x1, %ymm10, %xmm1
	xorq	%r11, %r10
	vmovq	%r10, %xmm11
	vperm2i128	$49, %ymm15, %ymm13, %ymm6
	vpermq	$144, %ymm3, %ymm8
	movq	%r10, %rax
	vpbroadcastq	%xmm11, %ymm15
	vpextrq	$1, %xmm1, %rcx
	vextracti128	$0x1, %ymm3, %xmm5
	movabsq	$1229782938247303441, %r10
	vmovq	%xmm1, %r9
	vpxor	%ymm9, %ymm6, %ymm14
	vpblendd	$3, %ymm15, %ymm8, %ymm7
	xorq	%rcx, %r9
	vpextrq	$1, %xmm5, %r8
	vpxor	%ymm14, %ymm12, %ymm13
	xorq	%r9, %r11
	vpand	%ymm2, %ymm15, %ymm12
	vpxor	%ymm7, %ymm9, %ymm6
	xorq	%r8, %rcx
	vpxor	%ymm6, %ymm12, %ymm9
	vmovq	%r11, %xmm0
	vmovq	%rcx, %xmm7
	movabsq	$8608480567731124087, %r11
	vpbroadcastq	%xmm0, %ymm5
	vpbroadcastq	%xmm7, %ymm12
	vextracti128	$0x1, %ymm13, %xmm10
	vpermq	$144, %ymm9, %ymm6
	vpermq	$144, %ymm13, %ymm1
	vpand	%ymm2, %ymm5, %ymm11
	vpblendd	$3, %ymm5, %ymm1, %ymm13
	vpextrq	$1, %xmm10, %r14
	vpblendd	$3, %ymm12, %ymm6, %ymm10
	vpxor	%ymm13, %ymm11, %ymm15
	vpand	%ymm2, %ymm12, %ymm0
	vextracti128	$0x1, %ymm9, %xmm8
	xorq	%r14, %rax
	vpxor	%ymm10, %ymm14, %ymm14
	vpxor	%ymm15, %ymm3, %ymm3
	vpextrq	$1, %xmm8, %rdi
	xorq	%rax, %rcx
	vpxor	%ymm14, %ymm0, %ymm1
	vpxor	%ymm3, %ymm9, %ymm9
	vmovq	%rax, %xmm0
	xorq	%r9, %rdi
	vpxor	%ymm1, %ymm15, %ymm5
	vpunpckhqdq	%ymm9, %ymm1, %ymm12
	vpunpcklqdq	%ymm9, %ymm1, %ymm15
	xorq	%rdi, %r14
	vpunpckhqdq	%ymm5, %ymm3, %ymm11
	vpunpcklqdq	%ymm5, %ymm3, %ymm13
	vmovq	%rdi, %xmm10
	movabsq	$3689348814741910323, %r9
	vperm2i128	$32, %ymm12, %ymm11, %ymm7
	vperm2i128	$32, %ymm15, %ymm13, %ymm3
	vpinsrq	$1, %rcx, %xmm10, %xmm14
	vpinsrq	$1, %r14, %xmm0, %xmm1
	vpxor	%ymm7, %ymm3, %ymm9
	vperm2i128	$49, %ymm15, %ymm13, %ymm8
	vinserti128	$0x1, %xmm14, %ymm1, %ymm5
	vperm2i128	$49, %ymm12, %ymm11, %ymm6
	vpxor	%ymm4, %ymm9, %ymm12
	vpxor	%ymm5, %ymm3, %ymm13
	vpxor	%ymm5, %ymm8, %ymm15
	vpxor	%ymm6, %ymm12, %ymm14
	vpandn	%ymm5, %ymm8, %ymm0
	vpand	%ymm13, %ymm12, %ymm10
	vpand	%ymm5, %ymm7, %ymm12
	vpxor	%ymm10, %ymm0, %ymm9
	vpxor	%ymm15, %ymm12, %ymm0
	vpor	%ymm8, %ymm3, %ymm11
	vpxor	%ymm6, %ymm0, %ymm10
	vpand	%ymm5, %ymm3, %ymm3
	vpand	%ymm6, %ymm15, %ymm6
	vpor	%ymm13, %ymm14, %ymm15
	vpand	%ymm14, %ymm8, %ymm1
	vpor	%ymm10, %ymm7, %ymm12
	vpxor	%ymm6, %ymm15, %ymm14
	vpxor	%ymm11, %ymm3, %ymm5
	vpxor	%ymm11, %ymm7, %ymm7
	vpxor	%ymm6, %ymm7, %ymm11
	vpxor	%ymm1, %ymm9, %ymm9
	vpxor	%ymm12, %ymm5, %ymm5
	vpxor	%ymm8, %ymm14, %ymm8
	vpxor	%ymm10, %ymm1, %ymm1
	vpunpcklqdq	%ymm5, %ymm9, %ymm13
	vpunpcklqdq	%ymm11, %ymm8, %ymm12
	vpunpckhqdq	%ymm5, %ymm9, %ymm3
	vpunpckhqdq	%ymm11, %ymm8, %ymm0
	vperm2i128	$32, %ymm12, %ymm13, %ymm9
	vperm2i128	$49, %ymm12, %ymm13, %ymm14
	vextracti128	$0x1, %ymm1, %xmm5
	vpextrq	$1, %xmm1, %rsi
	vmovq	%rbx, %xmm6
	vmovq	%r11, %xmm13
	vperm2i128	$32, %ymm0, %ymm3, %ymm15
	vmovq	%xmm5, %r8
	vpextrq	$1, %xmm5, %rcx
	leaq	(%rsi,%rsi), %rax
	vpbroadcastq	%xmm6, %ymm5
	shrq	$3, %rsi
	vpbroadcastq	%xmm13, %ymm6
	vperm2i128	$49, %ymm0, %ymm3, %ymm11
	vpand	%ymm5, %ymm15, %ymm8
	vpand	%ymm6, %ymm15, %ymm3
	andq	%r10, %rsi
	andq	%r13, %rax
	vpsrlq	$3, %ymm8, %ymm7
	vmovq	%xmm1, %rdi
	orq	%rsi, %rax
	vpand	.LC3(%rip), %ymm14, %ymm0
	vpand	.LC5(%rip), %ymm11, %ymm8
	vpand	.LC4(%rip), %ymm14, %ymm14
	vpand	.LC6(%rip), %ymm11, %ymm11
	xorq	%rax, %rdi
	vpsllq	$1, %ymm3, %ymm12
	leaq	0(,%rcx,8), %rsi
	shrq	%rcx
	leaq	0(,%r8,4), %r14
	vpor	%ymm7, %ymm12, %ymm15
	shrq	$2, %r8
	andq	%r11, %rcx
	andq	%rbx, %rsi
	vpxor	%ymm15, %ymm9, %ymm12
	andq	%r9, %r8
	andq	%r15, %r14
	orq	%rcx, %rsi
	vpsrlq	$2, %ymm0, %ymm1
	vpsllq	$2, %ymm14, %ymm10
	vmovq	%rdi, %xmm14
	orq	%r8, %r14
	vpsrlq	$1, %ymm8, %ymm7
	vpsllq	$3, %ymm11, %ymm13
	vpor	%ymm1, %ymm10, %ymm9
	xorq	%rsi, %r14
	vpor	%ymm7, %ymm13, %ymm3
	vpbroadcastq	%xmm14, %ymm10
	vpermq	$144, %ymm12, %ymm8
	xorq	%r14, %rax
	vextracti128	$0x1, %ymm12, %xmm1
	vpxor	%ymm3, %ymm9, %ymm0
	vpblendd	$3, %ymm10, %ymm8, %ymm7
	vpextrq	$1, %xmm1, %rcx
	vpand	%ymm2, %ymm10, %ymm11
	vpxor	%ymm7, %ymm3, %ymm13
	vpxor	%ymm0, %ymm15, %ymm15
	vpxor	%ymm13, %ymm11, %ymm1
	vmovq	%rax, %xmm9
	xorq	%rsi, %rcx
	vextracti128	$0x1, %ymm15, %xmm3
	vmovq	%rcx, %xmm13
	vpbroadcastq	%xmm9, %ymm14
	vpextrq	$1, %xmm3, %r8
	vpermq	$144, %ymm1, %ymm9
	vpbroadcastq	%xmm13, %ymm3
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm2, %ymm14, %ymm8
	vextracti128	$0x1, %ymm1, %xmm7
	xorq	%r8, %rdi
	vpblendd	$3, %ymm14, %ymm15, %ymm10
	vpblendd	$3, %ymm3, %ymm9, %ymm14
	vpand	%ymm2, %ymm3, %ymm15
	movq	%r8, %rax
	vpxor	%ymm14, %ymm0, %ymm0
	vpxor	%ymm10, %ymm8, %ymm11
	vpextrq	$1, %xmm7, %rsi
	xorq	%rdi, %rcx
	vpxor	%ymm0, %ymm15, %ymm10
	vpxor	%ymm11, %ymm12, %ymm12
	xorq	%r14, %rsi
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	.LC5(%rip), %ymm11, %ymm8
	vpxor	%ymm12, %ymm1, %ymm1
	xorq	%rsi, %rax
	vpand	.LC6(%rip), %ymm11, %ymm13
	vpand	.LC3(%rip), %ymm10, %ymm14
	leaq	0(,%rsi,4), %r8
	shrq	$2, %rsi
	vpand	.LC4(%rip), %ymm10, %ymm0
	vpsrlq	$1, %ymm8, %ymm7
	vpand	%ymm5, %ymm1, %ymm8
	andq	%r9, %rsi
	vpsllq	$3, %ymm13, %ymm3
	vpand	%ymm6, %ymm1, %ymm1
	vpxor	.LC22(%rip), %ymm12, %ymm12
	andq	%r15, %r8
	vpor	%ymm7, %ymm3, %ymm9
	vpsrlq	$2, %ymm14, %ymm15
	leaq	(%rcx,%rcx), %r15
	orq	%rsi, %r8
	vpsllq	$2, %ymm0, %ymm10
	vpsrlq	$3, %ymm8, %ymm7
	shrq	$3, %rcx
	leaq	0(,%rax,8), %r14
	vpsllq	$1, %ymm1, %ymm13
	shrq	%rax
	vpor	%ymm15, %ymm10, %ymm11
	andq	%r13, %r15
	vpor	%ymm7, %ymm13, %ymm14
	andq	%r11, %rax
	andq	%rbx, %r14
	andq	%r10, %rcx
	vpunpcklqdq	%ymm9, %ymm12, %ymm15
	vpunpcklqdq	%ymm14, %ymm11, %ymm3
	vpunpckhqdq	%ymm9, %ymm12, %ymm9
	orq	%rax, %r14
	vpunpckhqdq	%ymm14, %ymm11, %ymm0
	vmovq	%rdi, %xmm7
	vperm2i128	$32, %ymm3, %ymm15, %ymm8
	orq	%r15, %rcx
	vmovq	%r8, %xmm11
	vperm2i128	$32, %ymm0, %ymm9, %ymm1
	vpinsrq	$1, %r14, %xmm7, %xmm14
	movabsq	$4222189076152335, %r15
	vpinsrq	$1, %rcx, %xmm11, %xmm13
	vpxor	%ymm1, %ymm8, %ymm12
	vperm2i128	$49, %ymm3, %ymm15, %ymm10
	vinserti128	$0x1, %xmm13, %ymm14, %ymm7
	vpxor	%ymm4, %ymm12, %ymm11
	vperm2i128	$49, %ymm0, %ymm9, %ymm0
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm0, %ymm11, %ymm9
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm1, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm9, %ymm10, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm9, %ymm13
	vpor	%ymm11, %ymm1, %ymm7
	vpxor	%ymm0, %ymm13, %ymm9
	vpxor	%ymm14, %ymm1, %ymm1
	vpxor	%ymm0, %ymm1, %ymm14
	vpxor	%ymm10, %ymm9, %ymm15
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpextrq	$1, %xmm3, %rcx
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm3, %r10
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm7, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm10, %ymm7, %ymm9
	vperm2i128	$49, %ymm8, %ymm12, %ymm15
	xorq	%rcx, %r10
	vextracti128	$0x1, %ymm3, %xmm1
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm15, %ymm9, %ymm12
	vmovq	%r10, %xmm8
	vpxor	%ymm12, %ymm13, %ymm10
	vpextrq	$1, %xmm1, %rdi
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm7
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm1, %r14
	vpblendd	$3, %ymm0, %ymm13, %ymm9
	vpextrq	$1, %xmm7, %rsi
	vpand	%ymm2, %ymm0, %ymm3
	vpxor	%ymm9, %ymm15, %ymm15
	vextracti128	$0x1, %ymm10, %xmm1
	xorq	%rdi, %r14
	xorq	%r14, %rcx
	vpxor	%ymm15, %ymm3, %ymm11
	vpermq	$144, %ymm10, %ymm10
	xorq	%rsi, %rdi
	vmovq	%rcx, %xmm7
	vmovq	%rdi, %xmm15
	vpextrq	$1, %xmm1, %r8
	vpbroadcastq	%xmm7, %ymm8
	vpbroadcastq	%xmm15, %ymm1
	vpermq	$144, %ymm11, %ymm7
	xorq	%r8, %r10
	vpblendd	$3, %ymm8, %ymm10, %ymm0
	vpand	%ymm2, %ymm8, %ymm13
	vpblendd	$3, %ymm1, %ymm7, %ymm8
	xorq	%r10, %rdi
	vpxor	%ymm0, %ymm13, %ymm9
	vextracti128	$0x1, %ymm11, %xmm3
	vpand	%ymm2, %ymm1, %ymm0
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm9, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rax
	vpxor	%ymm12, %ymm0, %ymm13
	xorq	%r14, %rax
	vpxor	%ymm14, %ymm11, %ymm11
	movq	%r12, %r14
	vpxor	%ymm13, %ymm9, %ymm9
	vpunpcklqdq	%ymm11, %ymm13, %ymm1
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	xorq	%rax, %r8
	vpunpckhqdq	%ymm9, %ymm14, %ymm15
	vpunpcklqdq	%ymm9, %ymm14, %ymm10
	vmovq	%rax, %xmm12
	vmovq	%r10, %xmm14
	vperm2i128	$32, %ymm1, %ymm10, %ymm7
	vperm2i128	$32, %ymm3, %ymm15, %ymm8
	movabsq	$-1152657617789587456, %r10
	vpinsrq	$1, %rdi, %xmm12, %xmm13
	vpinsrq	$1, %r8, %xmm14, %xmm11
	vperm2i128	$49, %ymm1, %ymm10, %ymm10
	vpxor	%ymm8, %ymm7, %ymm9
	vinserti128	$0x1, %xmm13, %ymm11, %ymm1
	vperm2i128	$49, %ymm3, %ymm15, %ymm0
	vpxor	%ymm4, %ymm9, %ymm11
	vpxor	%ymm1, %ymm7, %ymm15
	vpandn	%ymm1, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm1, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm1, %ymm8, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm1, %ymm7, %ymm7
	vpand	%ymm0, %ymm13, %ymm13
	vpxor	%ymm0, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm0
	vpand	%ymm9, %ymm10, %ymm3
	vpor	%ymm11, %ymm8, %ymm1
	vpxor	%ymm13, %ymm0, %ymm9
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm1, %ymm7, %ymm7
	vmovq	%r10, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vperm2i128	$49, %ymm12, %ymm15, %ymm1
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vmovq	%r12, %xmm12
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	%xmm9, %ymm9
	movabsq	$-4222189076152336, %r12
	vpbroadcastq	%xmm12, %ymm12
	vpand	%ymm9, %ymm10, %ymm14
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpand	%ymm12, %ymm10, %ymm13
	vpsrlq	$12, %ymm14, %ymm15
	vpxor	%ymm11, %ymm3, %ymm3
	vpsllq	$4, %ymm13, %ymm10
	vextracti128	$0x1, %ymm3, %xmm0
	vpextrq	$1, %xmm3, %rdi
	vmovq	%xmm3, %rsi
	vpor	%ymm15, %ymm10, %ymm3
	movq	%rdi, %r8
	salq	$4, %r8
	shrq	$12, %rdi
	vpextrq	$1, %xmm0, %rax
	andq	%r15, %rdi
	vmovq	%xmm0, %rcx
	andq	%r12, %r8
	vpshufb	.LXRH_bswap16(%rip), %ymm1, %ymm15
	orq	%rdi, %r8
	movq	%rax, %rdi
	shrq	$4, %rax
	salq	$12, %rdi
	andq	%r14, %rax
	xorq	%r8, %rsi
	andq	%r10, %rdi
	vpxor	%ymm3, %ymm8, %ymm8
	movq	%r10, %r12
	vpbroadcastq	.LC35(%rip), %ymm11
	orq	%rdi, %rax
	movq	%rcx, %rdi
	vpbroadcastq	.LC36(%rip), %ymm14
	salq	$8, %rdi
	shrq	$8, %rcx
	movabsq	$-71777214294589696, %r15
	vpand	%ymm11, %ymm7, %ymm0
	andq	%r15, %rdi
	movabsq	$71777214294589695, %r10
	vpand	%ymm14, %ymm7, %ymm7
	vpsrlq	$4, %ymm0, %ymm0
	andq	%r10, %rcx
	vpsllq	$12, %ymm7, %ymm7
	orq	%rcx, %rdi
	vpor	%ymm0, %ymm7, %ymm7
	vmovq	%rsi, %xmm0
	xorq	%rax, %rdi
	vpxor	%ymm7, %ymm15, %ymm1
	xorq	%rdi, %r8
	vpxor	%ymm1, %ymm3, %ymm15
	vextracti128	$0x1, %ymm8, %xmm3
	vpextrq	$1, %xmm3, %rcx
	vpbroadcastq	%xmm0, %ymm3
	vpermq	$144, %ymm8, %ymm0
	vpblendd	$3, %ymm3, %ymm0, %ymm0
	vpand	%ymm2, %ymm3, %ymm3
	xorq	%rcx, %rax
	vpxor	%ymm0, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm0
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm3, %ymm3
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm0, %rcx
	vpbroadcastq	%xmm7, %ymm0
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm0, %ymm15, %ymm7
	vextracti128	$0x1, %ymm3, %xmm15
	vpand	%ymm2, %ymm0, %ymm0
	vpxor	%ymm7, %ymm0, %ymm0
	vpextrq	$1, %xmm15, %r8
	vmovq	%rax, %xmm7
	xorq	%rsi, %rax
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm3, %ymm15
	vpxor	%ymm0, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm2, %ymm7, %ymm7
	vpxor	%ymm15, %ymm1, %ymm1
	movq	%rcx, %r8
	shrq	$4, %rcx
	vpxor	%ymm8, %ymm3, %ymm3
	vpxor	%ymm1, %ymm7, %ymm15
	salq	$12, %r8
	andq	%r14, %rcx
	vpand	%ymm9, %ymm3, %ymm9
	vpxor	%ymm15, %ymm0, %ymm0
	andq	%r12, %r8
	movq	%rdi, %r12
	movq	%rax, %r14
	vpand	%ymm11, %ymm0, %ymm11
	vpand	%ymm14, %ymm0, %ymm14
	orq	%r8, %rcx
	salq	$8, %r12
	shrq	$8, %rdi
	vpand	%ymm12, %ymm3, %ymm12
	andq	%r10, %rdi
	vpsrlq	$4, %ymm11, %ymm7
	andq	%r15, %r12
	salq	$4, %r14
	vpsllq	$12, %ymm14, %ymm1
	vpxor	.LC23(%rip), %ymm8, %ymm8
	orq	%rdi, %r12
	shrq	$12, %rax
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm14
	vpor	%ymm7, %ymm1, %ymm0
	movabsq	$-4222189076152336, %rdi
	vpsrlq	$12, %ymm9, %ymm7
	vpunpcklqdq	%ymm0, %ymm8, %ymm10
	andq	%rdi, %r14
	vpsllq	$4, %ymm12, %ymm3
	vpunpckhqdq	%ymm0, %ymm8, %ymm0
	vmovq	%r12, %xmm15
	movabsq	$4222189076152335, %r10
	vpor	%ymm7, %ymm3, %ymm1
	andq	%r10, %rax
	vmovq	%rsi, %xmm9
	vpunpcklqdq	%ymm1, %ymm14, %ymm11
	vpunpckhqdq	%ymm1, %ymm14, %ymm13
	orq	%rax, %r14
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm0, %ymm1
	vpinsrq	$1, %r14, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm9, %xmm7
	vpxor	%ymm1, %ymm8, %ymm12
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vinserti128	$0x1, %xmm14, %ymm7, %ymm7
	vperm2i128	$49, %ymm13, %ymm0, %ymm0
	vpxor	%ymm4, %ymm12, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm0, %ymm11, %ymm9
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm1, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm9, %ymm10, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm9, %ymm13
	vpor	%ymm11, %ymm1, %ymm7
	vpxor	%ymm0, %ymm13, %ymm9
	vpxor	%ymm14, %ymm1, %ymm1
	vpxor	%ymm0, %ymm1, %ymm14
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm10, %ymm9, %ymm15
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpcklqdq	%ymm8, %ymm12, %ymm0
	vpextrq	$1, %xmm3, %rax
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm3, %rcx
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm0, %ymm7
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm10, %ymm0, %ymm9
	vperm2i128	$49, %ymm8, %ymm12, %ymm15
	xorq	%rax, %rcx
	vextracti128	$0x1, %ymm3, %xmm1
	vpxor	%ymm13, %ymm7, %ymm14
	vpxor	%ymm15, %ymm9, %ymm12
	vmovq	%rcx, %xmm8
	vpxor	%ymm12, %ymm13, %ymm10
	vpextrq	$1, %xmm1, %r12
	vpbroadcastq	%xmm8, %ymm7
	vextracti128	$0x1, %ymm14, %xmm0
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm1, %r8
	vpblendd	$3, %ymm7, %ymm13, %ymm9
	vpextrq	$1, %xmm0, %rsi
	vpand	%ymm2, %ymm7, %ymm3
	vpxor	%ymm9, %ymm15, %ymm15
	vextracti128	$0x1, %ymm10, %xmm1
	xorq	%r12, %r8
	xorq	%r8, %rax
	vpxor	%ymm15, %ymm3, %ymm11
	vpextrq	$1, %xmm1, %r15
	xorq	%rsi, %r12
	vmovq	%rax, %xmm0
	vpermq	$144, %ymm11, %ymm1
	vpermq	$144, %ymm10, %ymm10
	xorq	%r15, %rcx
	vmovq	%r12, %xmm15
	vpbroadcastq	%xmm0, %ymm8
	vextracti128	$0x1, %ymm11, %xmm3
	xorq	%rcx, %r12
	vpbroadcastq	%xmm15, %ymm0
	vpblendd	$3, %ymm8, %ymm10, %ymm7
	vpand	%ymm2, %ymm8, %ymm13
	vpblendd	$3, %ymm0, %ymm1, %ymm8
	vpxor	%ymm7, %ymm13, %ymm9
	vpand	%ymm2, %ymm0, %ymm7
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm9, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r14
	vpxor	%ymm12, %ymm7, %ymm13
	xorq	%r8, %r14
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm9, %ymm9
	vpunpcklqdq	%ymm11, %ymm13, %ymm1
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	xorq	%r14, %r15
	vpunpckhqdq	%ymm9, %ymm14, %ymm15
	vpunpcklqdq	%ymm9, %ymm14, %ymm10
	vmovq	%r14, %xmm8
	vmovq	%rcx, %xmm13
	vperm2i128	$32, %ymm1, %ymm10, %ymm7
	vperm2i128	$32, %ymm3, %ymm15, %ymm0
	vpinsrq	$1, %r12, %xmm8, %xmm12
	vpinsrq	$1, %r15, %xmm13, %xmm14
	vperm2i128	$49, %ymm1, %ymm10, %ymm10
	vpxor	%ymm0, %ymm7, %ymm11
	vperm2i128	$49, %ymm3, %ymm15, %ymm1
	vinserti128	$0x1, %xmm12, %ymm14, %ymm3
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpandn	%ymm3, %ymm10, %ymm12
	vpxor	%ymm1, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm0, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpand	%ymm9, %ymm10, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm9, %ymm13
	vpor	%ymm11, %ymm0, %ymm3
	vpxor	%ymm1, %ymm13, %ymm9
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm0, %ymm0
	vpxor	%ymm10, %ymm9, %ymm15
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm1, %ymm0, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm7
	vperm2i128	$49, %ymm10, %ymm3, %ymm15
	vperm2i128	$32, %ymm10, %ymm3, %ymm1
	vperm2i128	$32, %ymm7, %ymm12, %ymm13
	vperm2i128	$49, %ymm7, %ymm12, %ymm0
	vpextrq	$1, %xmm8, %rdi
	vpshufb	.LXRH_rot16(%rip), %ymm13, %ymm12
	vmovq	%xmm8, %r14
	rorx	$48, %rdi, %r8
	xorq	%r8, %r14
	movq	%r8, %rdi
	vpshufb	.LXRH_rot48(%rip), %ymm0, %ymm0

	vextracti128	$0x1, %ymm8, %xmm9
	vpshufd	$177, %ymm15, %ymm8
	vpxor	%ymm12, %ymm1, %ymm1
	vpxor	%ymm0, %ymm8, %ymm11
	vmovq	%r14, %xmm3
	vmovq	%xmm9, %r10
	vpextrq	$1, %xmm9, %rcx
	vextracti128	$0x1, %ymm1, %xmm14
	vpxor	%ymm11, %ymm12, %ymm9
	vpermq	$144, %ymm1, %ymm13
	rorx	$16, %rcx, %rax
	vpbroadcastq	%xmm3, %ymm12
	vpblendd	$3, %ymm12, %ymm13, %ymm10
	rorx	$32, %r10, %r12
	vpextrq	$1, %xmm14, %rsi
	xorq	%rax, %r12
	vpand	%ymm2, %ymm12, %ymm7
	vpxor	%ymm10, %ymm0, %ymm15
	xorq	%rax, %rsi
	xorq	%r12, %rdi
	vpxor	%ymm15, %ymm7, %ymm14
	vextracti128	$0x1, %ymm9, %xmm0
	vmovq	%rdi, %xmm8
	movq	%r14, %rcx
	vmovq	%rsi, %xmm7
	vpbroadcastq	%xmm8, %ymm3
	vpextrq	$1, %xmm0, %r15
	vpbroadcastq	%xmm7, %ymm8
	vpermq	$144, %ymm14, %ymm0
	vpermq	$144, %ymm9, %ymm9
	xorq	%r15, %rcx
	vpblendd	$3, %ymm3, %ymm9, %ymm12
	vpand	%ymm2, %ymm3, %ymm13
	vpblendd	$3, %ymm8, %ymm0, %ymm3
	xorq	%rcx, %rsi
	vpxor	%ymm12, %ymm13, %ymm15
	vpand	%ymm2, %ymm8, %ymm9
	vpxor	%ymm3, %ymm11, %ymm11
	rorq	$48, %rsi
	vpxor	%ymm11, %ymm9, %ymm12
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm14, %xmm10
	vpxor	%ymm12, %ymm15, %ymm13
	vpxor	%ymm1, %ymm14, %ymm14
	vpextrq	$1, %xmm10, %r10
	vpxor	.LC24(%rip), %ymm1, %ymm1
	vpshufb	.LXRH_rot48(%rip), %ymm13, %ymm3
	xorq	%r12, %r10

	vpshufd	$177, %ymm12, %ymm9
	xorq	%r10, %r15
	rorx	$32, %r10, %rax

	vpshufb	.LXRH_rot16(%rip), %ymm14, %ymm12
	rorq	$16, %r15

	vpunpcklqdq	%ymm3, %ymm1, %ymm14
	vpunpckhqdq	%ymm3, %ymm1, %ymm13
	vmovq	%rax, %xmm7
	vpunpcklqdq	%ymm12, %ymm9, %ymm15
	vpunpckhqdq	%ymm12, %ymm9, %ymm3
	vmovq	%rcx, %xmm11
	vperm2i128	$32, %ymm3, %ymm13, %ymm1
	vperm2i128	$32, %ymm15, %ymm14, %ymm8
	vpinsrq	$1, %r15, %xmm11, %xmm12
	vpinsrq	$1, %rsi, %xmm7, %xmm9
	vperm2i128	$49, %ymm15, %ymm14, %ymm10
	vpxor	%ymm1, %ymm8, %ymm15
	vinserti128	$0x1, %xmm9, %ymm12, %ymm7
	vperm2i128	$49, %ymm3, %ymm13, %ymm0
	vpxor	%ymm4, %ymm15, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm0, %ymm11, %ymm9
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm1, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm9, %ymm10, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm9, %ymm13
	vpor	%ymm11, %ymm1, %ymm7
	vpxor	%ymm0, %ymm13, %ymm9
	vpxor	%ymm14, %ymm1, %ymm1
	vpxor	%ymm0, %ymm1, %ymm14
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm10, %ymm9, %ymm15
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpcklqdq	%ymm8, %ymm12, %ymm0
	vpextrq	$1, %xmm3, %r15
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm3, %r12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm0, %ymm7
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm10, %ymm0, %ymm9
	vperm2i128	$49, %ymm8, %ymm12, %ymm15
	xorq	%r15, %r12
	vextracti128	$0x1, %ymm3, %xmm1
	vpxor	%ymm13, %ymm7, %ymm14
	vpxor	%ymm15, %ymm9, %ymm12
	vmovq	%r12, %xmm8
	vpxor	%ymm12, %ymm13, %ymm10
	vpextrq	$1, %xmm1, %rdi
	vpbroadcastq	%xmm8, %ymm7
	vextracti128	$0x1, %ymm14, %xmm0
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm1, %r10
	vpblendd	$3, %ymm7, %ymm13, %ymm9
	vpextrq	$1, %xmm0, %rsi
	vpand	%ymm2, %ymm7, %ymm3
	vpxor	%ymm9, %ymm15, %ymm15
	vextracti128	$0x1, %ymm10, %xmm1
	xorq	%rdi, %r10
	xorq	%r10, %r15
	vpxor	%ymm15, %ymm3, %ymm11
	vpextrq	$1, %xmm1, %rcx
	xorq	%rsi, %rdi
	vmovq	%r15, %xmm0
	vmovq	%rdi, %xmm15
	vpermq	$144, %ymm10, %ymm10
	vpbroadcastq	%xmm0, %ymm8
	vpbroadcastq	%xmm15, %ymm1
	vpermq	$144, %ymm11, %ymm0
	vpblendd	$3, %ymm8, %ymm10, %ymm7
	vpand	%ymm2, %ymm8, %ymm13
	vpblendd	$3, %ymm1, %ymm0, %ymm8
	vpxor	%ymm7, %ymm13, %ymm9
	vextracti128	$0x1, %ymm11, %xmm3
	vpand	%ymm2, %ymm1, %ymm2
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm9, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r8
	vpxor	%ymm12, %ymm2, %ymm10
	xorq	%r10, %r8
	vpxor	%ymm14, %ymm11, %ymm11
	xorq	%rcx, %r12
	vpxor	%ymm10, %ymm9, %ymm7
	vpunpckhqdq	%ymm11, %ymm10, %ymm15
	vpunpcklqdq	%ymm11, %ymm10, %ymm9
	xorq	%r8, %rcx
	vpunpcklqdq	%ymm7, %ymm14, %ymm13
	vpunpckhqdq	%ymm7, %ymm14, %ymm3
	vmovq	%r8, %xmm2
	xorq	%r12, %rdi
	vmovq	%r12, %xmm10
	vperm2i128	$32, %ymm15, %ymm3, %ymm14
	vperm2i128	$32, %ymm9, %ymm13, %ymm0
	movabsq	$1229782938247303441, %r10
	vpinsrq	$1, %rdi, %xmm2, %xmm12
	vpinsrq	$1, %rcx, %xmm10, %xmm11
	vperm2i128	$49, %ymm9, %ymm13, %ymm8
	vinserti128	$0x1, %xmm12, %ymm11, %ymm7
	vpxor	%ymm14, %ymm0, %ymm13
	vperm2i128	$49, %ymm15, %ymm3, %ymm1
	vpxor	%ymm4, %ymm13, %ymm3
	vpxor	%ymm7, %ymm0, %ymm13
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm1, %ymm3, %ymm4
	vpandn	%ymm7, %ymm8, %ymm10
	vpand	%ymm13, %ymm3, %ymm9
	vpand	%ymm7, %ymm14, %ymm3
	vpxor	%ymm9, %ymm10, %ymm11
	vpor	%ymm8, %ymm0, %ymm12
	vpxor	%ymm15, %ymm3, %ymm9
	vpand	%ymm4, %ymm8, %ymm2
	vpand	%ymm7, %ymm0, %ymm0
	vpxor	%ymm1, %ymm9, %ymm9
	vpand	%ymm1, %ymm15, %ymm1
	vpor	%ymm13, %ymm4, %ymm15
	vpxor	%ymm2, %ymm11, %ymm10
	vpxor	%ymm12, %ymm0, %ymm7
	vpor	%ymm9, %ymm14, %ymm11
	vpxor	%ymm1, %ymm15, %ymm4
	vpxor	%ymm12, %ymm14, %ymm14
	vpxor	%ymm11, %ymm7, %ymm3
	vpxor	%ymm8, %ymm4, %ymm13
	vpxor	%ymm1, %ymm14, %ymm12
	vpunpcklqdq	%ymm3, %ymm10, %ymm8
	vpunpckhqdq	%ymm12, %ymm13, %ymm11
	vpxor	%ymm9, %ymm2, %ymm2
	vpunpckhqdq	%ymm3, %ymm10, %ymm10
	vperm2i128	$32, %ymm11, %ymm10, %ymm15
	vpunpcklqdq	%ymm12, %ymm13, %ymm0
	vextracti128	$0x1, %ymm2, %xmm3
	vpextrq	$1, %xmm2, %r12
	vperm2i128	$49, %ymm11, %ymm10, %ymm14
	vperm2i128	$32, %ymm0, %ymm8, %ymm7
	vperm2i128	$49, %ymm0, %ymm8, %ymm13
	vpextrq	$1, %xmm3, %r15
	vpand	.LC3(%rip), %ymm13, %ymm8
	vpand	.LC4(%rip), %ymm13, %ymm0
	vpand	%ymm5, %ymm15, %ymm5
	vpand	%ymm6, %ymm15, %ymm6
	leaq	(%r12,%r12), %rax
	vpand	.LC5(%rip), %ymm14, %ymm15
	shrq	$3, %r12
	vpand	.LC6(%rip), %ymm14, %ymm14
	andq	%r10, %r12
	vmovq	%xmm2, %r14
	vmovq	%xmm3, %r8
	andq	%r13, %rax
	leaq	0(,%r15,8), %rsi
	vpsrlq	$3, %ymm5, %ymm1
	orq	%r12, %rax
	shrq	%r15
	vpsllq	$1, %ymm6, %ymm4
	vpsrlq	$2, %ymm8, %ymm10
	andq	%rbx, %rsi
	movq	%r14, %rbx
	vpsllq	$2, %ymm0, %ymm11
	vpsrlq	$1, %ymm15, %ymm13
	xorq	%rax, %rbx
	andq	%r11, %r15
	vpsllq	$3, %ymm14, %ymm2
	vpor	%ymm1, %ymm4, %ymm12
	vpor	%ymm10, %ymm11, %ymm3
	orq	%r15, %rsi
	vpor	%ymm13, %ymm2, %ymm9
	leaq	0(,%r8,4), %rdi
	shrq	$2, %r8
	movabsq	$-3689348814741910324, %r12
	vpxor	%ymm12, %ymm7, %ymm7
	vpxor	%ymm9, %ymm3, %ymm5
	andq	%r9, %r8
	andq	%r12, %rdi
	vmovq	%rbx, %xmm4
	vpxor	%ymm5, %ymm12, %ymm1
	vpermq	$144, %ymm7, %ymm8
	orq	%r8, %rdi
	vmovdqa	.LC0(%rip), %ymm0
	vpbroadcastq	%xmm4, %ymm12
	vextracti128	$0x1, %ymm7, %xmm6
	xorq	%rsi, %rdi
	vpblendd	$3, %ymm12, %ymm8, %ymm10
	vpextrq	$1, %xmm6, %rcx
	vpermq	$144, %ymm1, %ymm3
	xorq	%rdi, %rax
	vpand	%ymm0, %ymm12, %ymm11
	vpxor	%ymm10, %ymm9, %ymm15
	vmovq	%rax, %xmm2
	xorq	%rsi, %rcx
	vpxor	%ymm15, %ymm11, %ymm13
	vmovq	%rcx, %xmm8
	vpbroadcastq	%xmm2, %ymm9
	vpbroadcastq	%xmm8, %ymm10
	vextracti128	$0x1, %ymm1, %xmm14
	vextracti128	$0x1, %ymm13, %xmm4
	vpermq	$144, %ymm13, %ymm11
	vpextrq	$1, %xmm14, %r11
	vpblendd	$3, %ymm9, %ymm3, %ymm1
	vpand	%ymm0, %ymm9, %ymm6
	vpblendd	$3, %ymm10, %ymm11, %ymm15
	movq	%r11, %r15
	xorq	%r11, %rbx
	vpextrq	$1, %xmm4, %r14
	vpxor	%ymm1, %ymm6, %ymm12
	movq	%r10, %r11
	movq	%rbx, %r8
	xorq	%r14, %rdi
	vpand	%ymm0, %ymm10, %ymm14
	vpxor	%ymm15, %ymm5, %ymm5
	xorq	%rbx, %rcx
	vpxor	%ymm5, %ymm14, %ymm9
	xorq	%rdi, %r15
	vpxor	%ymm12, %ymm7, %ymm7
	movq	%r13, %rbx
	vpxor	%ymm9, %ymm12, %ymm2
	vpxor	%ymm7, %ymm13, %ymm13
	vpand	.LC5(%rip), %ymm2, %ymm3
	movabsq	$-8608480567731124088, %r14
	vpand	.LC6(%rip), %ymm2, %ymm6
	vpand	.LC3(%rip), %ymm9, %ymm4
	movabsq	$8608480567731124087, %r13
	vpand	.LC4(%rip), %ymm9, %ymm11
	vpand	.LC1(%rip), %ymm13, %ymm5
	leaq	0(,%r15,8), %r10
	shrq	%r15
	vpand	.LC2(%rip), %ymm13, %ymm13
	leaq	0(,%rdi,4), %rax
	shrq	$2, %rdi
	andq	%r13, %r15
	andq	%r9, %rdi
	andq	%r14, %r10
	leaq	(%rcx,%rcx), %r9
	andq	%r12, %rax
	orq	%r15, %r10
	vpsrlq	$1, %ymm3, %ymm1
	orq	%rdi, %rax
	andq	%rbx, %r9
	vpsllq	$3, %ymm6, %ymm12
	shrq	$3, %rcx
	vpsrlq	$2, %ymm4, %ymm8
	vpxor	.LC25(%rip), %ymm7, %ymm7
	vpsllq	$2, %ymm11, %ymm15
	vpsrlq	$3, %ymm5, %ymm9
	vpor	%ymm1, %ymm12, %ymm10
	andq	%r11, %rcx
	vpsllq	$1, %ymm13, %ymm2
	vpor	%ymm8, %ymm15, %ymm14
	vpunpcklqdq	%ymm10, %ymm7, %ymm12
	orq	%r9, %rcx
	vpor	%ymm9, %ymm2, %ymm3
	vpunpckhqdq	%ymm10, %ymm7, %ymm1
	vmovq	%rax, %xmm4
	movabsq	$-4222189076152336, %r9
	vpunpcklqdq	%ymm3, %ymm14, %ymm6
	vpunpckhqdq	%ymm3, %ymm14, %ymm10
	vmovq	%r8, %xmm5
	vperm2i128	$32, %ymm10, %ymm1, %ymm14
	vperm2i128	$32, %ymm6, %ymm12, %ymm15
	vpinsrq	$1, %rcx, %xmm4, %xmm11
	vpinsrq	$1, %r10, %xmm5, %xmm9
	vperm2i128	$49, %ymm6, %ymm12, %ymm8
	vperm2i128	$49, %ymm10, %ymm1, %ymm2
	vpcmpeqd	%ymm6, %ymm6, %ymm6
	vinserti128	$0x1, %xmm11, %ymm9, %ymm1
	vpxor	%ymm14, %ymm15, %ymm13
	vpxor	%ymm6, %ymm13, %ymm4
	vpxor	%ymm1, %ymm15, %ymm13
	vpxor	%ymm1, %ymm8, %ymm11
	vpxor	%ymm2, %ymm4, %ymm7
	vpandn	%ymm1, %ymm8, %ymm10
	vpand	%ymm13, %ymm4, %ymm5
	vpand	%ymm1, %ymm14, %ymm4
	vpand	%ymm7, %ymm8, %ymm3
	vpxor	%ymm5, %ymm10, %ymm9
	vpxor	%ymm11, %ymm4, %ymm5
	vpor	%ymm8, %ymm15, %ymm12
	vpxor	%ymm3, %ymm9, %ymm10
	vpand	%ymm2, %ymm11, %ymm11
	vpxor	%ymm2, %ymm5, %ymm9
	vpand	%ymm1, %ymm15, %ymm15
	vpor	%ymm13, %ymm7, %ymm2
	vpxor	%ymm12, %ymm15, %ymm1
	vpor	%ymm9, %ymm14, %ymm4
	vpxor	%ymm11, %ymm2, %ymm7
	vpxor	%ymm12, %ymm14, %ymm14
	vpxor	%ymm4, %ymm1, %ymm5
	vpxor	%ymm11, %ymm14, %ymm12
	vpxor	%ymm9, %ymm3, %ymm3
	vpxor	%ymm8, %ymm7, %ymm13
	vpunpcklqdq	%ymm5, %ymm10, %ymm15
	vpunpcklqdq	%ymm12, %ymm13, %ymm8
	vpunpckhqdq	%ymm12, %ymm13, %ymm1
	vpunpckhqdq	%ymm5, %ymm10, %ymm10
	vpextrq	$1, %xmm3, %r15
	vmovq	%xmm3, %rsi
	vperm2i128	$32, %ymm1, %ymm10, %ymm5
	vperm2i128	$32, %ymm8, %ymm15, %ymm4
	vperm2i128	$49, %ymm1, %ymm10, %ymm2
	xorq	%r15, %rsi
	vpxor	%ymm5, %ymm4, %ymm13
	vextracti128	$0x1, %ymm3, %xmm7
	vmovq	%rsi, %xmm10
	vperm2i128	$49, %ymm8, %ymm15, %ymm11
	vpextrq	$1, %xmm7, %rcx
	vpbroadcastq	%xmm10, %ymm8
	vextracti128	$0x1, %ymm13, %xmm15
	vpermq	$144, %ymm13, %ymm1
	vmovq	%xmm7, %r8
	xorq	%rcx, %r8
	vpxor	%ymm2, %ymm11, %ymm14
	vpblendd	$3, %ymm8, %ymm1, %ymm4
	vpextrq	$1, %xmm15, %rdi
	vpxor	%ymm14, %ymm5, %ymm12
	vpxor	%ymm4, %ymm2, %ymm11
	xorq	%r8, %r15
	vpand	%ymm0, %ymm8, %ymm5
	vmovq	%r15, %xmm9
	vpermq	$144, %ymm12, %ymm7
	xorq	%rdi, %rcx
	vpxor	%ymm11, %ymm5, %ymm2
	vmovq	%rcx, %xmm4
	vpbroadcastq	%xmm9, %ymm15
	vpbroadcastq	%xmm4, %ymm5
	vextracti128	$0x1, %ymm12, %xmm3
	vpermq	$144, %ymm2, %ymm11
	vpblendd	$3, %ymm15, %ymm7, %ymm12
	vpand	%ymm0, %ymm15, %ymm10
	vpextrq	$1, %xmm3, %r10
	vpxor	%ymm12, %ymm10, %ymm1
	vpand	%ymm0, %ymm5, %ymm9
	vextracti128	$0x1, %ymm2, %xmm8
	xorq	%r10, %rsi
	vpblendd	$3, %ymm5, %ymm11, %ymm3
	vpxor	%ymm1, %ymm13, %ymm13
	vpextrq	$1, %xmm8, %rax
	xorq	%rsi, %rcx
	vpxor	%ymm3, %ymm14, %ymm14
	xorq	%r8, %rax
	vpxor	%ymm13, %ymm2, %ymm12
	movabsq	$4222189076152335, %r8
	vpxor	%ymm14, %ymm9, %ymm15
	xorq	%rax, %r10
	vmovq	%rax, %xmm5
	vpxor	%ymm15, %ymm1, %ymm2
	vpunpcklqdq	%ymm12, %ymm15, %ymm8
	vpunpckhqdq	%ymm12, %ymm15, %ymm4
	vpunpcklqdq	%ymm2, %ymm13, %ymm7
	vpunpckhqdq	%ymm2, %ymm13, %ymm10
	vmovq	%rsi, %xmm9
	vperm2i128	$32, %ymm8, %ymm7, %ymm1
	vperm2i128	$32, %ymm4, %ymm10, %ymm3
	vpinsrq	$1, %rcx, %xmm5, %xmm11
	vpinsrq	$1, %r10, %xmm9, %xmm13
	vpxor	%ymm3, %ymm1, %ymm12
	vperm2i128	$49, %ymm8, %ymm7, %ymm14
	movabsq	$-1152657617789587456, %r10
	vinserti128	$0x1, %xmm11, %ymm13, %ymm2
	vperm2i128	$49, %ymm4, %ymm10, %ymm15
	vpxor	%ymm6, %ymm12, %ymm10
	vpxor	%ymm2, %ymm1, %ymm12
	vpxor	%ymm2, %ymm14, %ymm7
	vpxor	%ymm15, %ymm10, %ymm4
	vpandn	%ymm2, %ymm14, %ymm9
	vpand	%ymm12, %ymm10, %ymm8
	vpand	%ymm2, %ymm3, %ymm10
	vpxor	%ymm8, %ymm9, %ymm13
	vpxor	%ymm7, %ymm10, %ymm8
	vpor	%ymm14, %ymm1, %ymm11
	vpand	%ymm4, %ymm14, %ymm5
	vpxor	%ymm15, %ymm8, %ymm8
	vpand	%ymm2, %ymm1, %ymm1
	vpand	%ymm15, %ymm7, %ymm15
	vpor	%ymm12, %ymm4, %ymm7
	vpxor	%ymm5, %ymm13, %ymm9
	vpxor	%ymm15, %ymm7, %ymm4
	vpxor	%ymm8, %ymm5, %ymm5
	vpor	%ymm8, %ymm3, %ymm13
	vpxor	%ymm11, %ymm1, %ymm2
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm14, %ymm4, %ymm14
	vpxor	%ymm13, %ymm2, %ymm1
	vpxor	%ymm15, %ymm3, %ymm11
	vpextrq	$1, %xmm5, %rdi
	vpunpckhqdq	%ymm11, %ymm14, %ymm13
	vpunpckhqdq	%ymm1, %ymm9, %ymm10
	vextracti128	$0x1, %ymm5, %xmm4
	movq	%rdi, %rax
	salq	$4, %rax
	vpunpcklqdq	%ymm1, %ymm9, %ymm12
	vperm2i128	$32, %ymm13, %ymm10, %ymm7
	shrq	$12, %rdi
	vmovq	%xmm5, %rcx
	vpunpcklqdq	%ymm11, %ymm14, %ymm9
	vperm2i128	$49, %ymm13, %ymm10, %ymm1
	andq	%r8, %rdi
	vpbroadcastq	.LC35(%rip), %ymm5
	vperm2i128	$49, %ymm9, %ymm12, %ymm2
	vperm2i128	$32, %ymm9, %ymm12, %ymm15
	andq	%r9, %rax
	vpand	.LC8(%rip), %ymm7, %ymm14
	vpand	.LC9(%rip), %ymm7, %ymm3
	vpextrq	$1, %xmm4, %rsi
	orq	%rdi, %rax
	vpand	%ymm5, %ymm1, %ymm7
	vmovq	%xmm4, %r15
	movq	%rsi, %rdi
	xorq	%rax, %rcx
	vpsrlq	$12, %ymm14, %ymm11
	vpsllq	$4, %ymm3, %ymm12
	salq	$12, %rdi
	vpsrlq	$4, %ymm7, %ymm4
	vpor	%ymm11, %ymm12, %ymm13
	shrq	$4, %rsi
	andq	%r10, %rdi
	vpshufb	.LXRH_bswap16(%rip), %ymm2, %ymm3
	vpbroadcastq	.LC36(%rip), %ymm7
	movabsq	$1152657617789587455, %r8
	andq	%r8, %rsi
	vpxor	%ymm13, %ymm15, %ymm15
	movq	%r15, %r8
	movabsq	$71777214294589695, %r9
	vpand	%ymm7, %ymm1, %ymm1
	orq	%rsi, %rdi
	salq	$8, %r8
	vmovq	%rcx, %xmm8
	vpermq	$144, %ymm15, %ymm2
	shrq	$8, %r15
	vpbroadcastq	%xmm8, %ymm9
	vpsllq	$12, %ymm1, %ymm14
	vextracti128	$0x1, %ymm15, %xmm10
	andq	%r9, %r15
	movabsq	$-71777214294589696, %rsi
	vpor	%ymm4, %ymm14, %ymm11
	vpblendd	$3, %ymm9, %ymm2, %ymm1
	vpxor	%ymm11, %ymm3, %ymm12
	vpand	%ymm0, %ymm9, %ymm4
	vpxor	%ymm1, %ymm11, %ymm14
	andq	%rsi, %r8
	orq	%r15, %r8
	vpxor	%ymm14, %ymm4, %ymm11
	vpxor	%ymm12, %ymm13, %ymm13
	xorq	%rdi, %r8
	vextracti128	$0x1, %ymm13, %xmm3
	vpermq	$144, %ymm13, %ymm13
	xorq	%r8, %rax
	vextracti128	$0x1, %ymm11, %xmm1
	vpextrq	$1, %xmm3, %r15
	vpextrq	$1, %xmm10, %rsi
	vmovq	%rax, %xmm10
	xorq	%r15, %rcx
	vpextrq	$1, %xmm1, %rax
	xorq	%rdi, %rsi
	vpbroadcastq	%xmm10, %ymm8
	movabsq	$1152657617789587455, %rdi
	vmovq	%rsi, %xmm4
	vpermq	$144, %ymm11, %ymm10
	vpand	%ymm0, %ymm8, %ymm2
	xorq	%r8, %rax
	xorq	%rcx, %rsi
	xorq	%rax, %r15
	vpbroadcastq	%xmm4, %ymm3
	movq	%r15, %r8
	salq	$12, %r15
	vpblendd	$3, %ymm8, %ymm13, %ymm9
	vpblendd	$3, %ymm3, %ymm10, %ymm8
	andq	%r10, %r15
	shrq	$4, %r8
	vpand	%ymm0, %ymm3, %ymm13
	vpxor	%ymm8, %ymm12, %ymm12
	andq	%rdi, %r8
	movq	%r15, %rdi
	vpxor	%ymm9, %ymm2, %ymm14
	movabsq	$-71777214294589696, %r15
	vpxor	%ymm12, %ymm13, %ymm9
	orq	%r8, %rdi
	movq	%rax, %r8
	shrq	$8, %rax
	vpxor	%ymm9, %ymm14, %ymm2
	salq	$8, %r8
	andq	%r9, %rax
	vpxor	%ymm14, %ymm15, %ymm15
	vpand	%ymm5, %ymm2, %ymm14
	vpand	%ymm7, %ymm2, %ymm4
	vpxor	%ymm15, %ymm11, %ymm11
	andq	%r15, %r8
	orq	%rax, %r8
	movq	%rsi, %rax
	movabsq	$-4222189076152336, %r15
	vpsrlq	$4, %ymm14, %ymm1
	vpand	.LC8(%rip), %ymm11, %ymm14
	salq	$4, %rax
	vpand	.LC9(%rip), %ymm11, %ymm11
	vpsllq	$12, %ymm4, %ymm3
	andq	%r15, %rax
	shrq	$12, %rsi
	vpshufb	.LXRH_bswap16(%rip), %ymm9, %ymm2
	vpor	%ymm1, %ymm3, %ymm10
	movabsq	$4222189076152335, %r15
	vpxor	.LC26(%rip), %ymm15, %ymm15
	vpsrlq	$12, %ymm14, %ymm1
	andq	%r15, %rsi
	vpsllq	$4, %ymm11, %ymm4
	orq	%rsi, %rax
	vmovq	%rcx, %xmm14
	vpor	%ymm1, %ymm4, %ymm3
	vpunpcklqdq	%ymm10, %ymm15, %ymm13
	vpunpckhqdq	%ymm10, %ymm15, %ymm10
	vpunpckhqdq	%ymm3, %ymm2, %ymm12
	vpunpcklqdq	%ymm3, %ymm2, %ymm8
	vmovq	%r8, %xmm2
	vperm2i128	$32, %ymm8, %ymm13, %ymm3
	vperm2i128	$32, %ymm12, %ymm10, %ymm4
	vpinsrq	$1, %rax, %xmm2, %xmm11
	vpinsrq	$1, %rdi, %xmm14, %xmm15
	vperm2i128	$49, %ymm12, %ymm10, %ymm1
	vpxor	%ymm4, %ymm3, %ymm10
	vinserti128	$0x1, %xmm11, %ymm15, %ymm15
	vperm2i128	$49, %ymm8, %ymm13, %ymm9
	vpxor	%ymm6, %ymm10, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpxor	%ymm1, %ymm10, %ymm2
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm4, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm2, %ymm9, %ymm8
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm1, %ymm10, %ymm10
	vpand	%ymm1, %ymm12, %ymm1
	vpor	%ymm14, %ymm2, %ymm12
	vpor	%ymm10, %ymm4, %ymm15
	vpxor	%ymm1, %ymm12, %ymm2
	vpxor	%ymm13, %ymm4, %ymm4
	vpxor	%ymm9, %ymm2, %ymm14
	vpxor	%ymm1, %ymm4, %ymm13
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm15, %ymm3, %ymm3
	vpxor	%ymm10, %ymm8, %ymm8
	vpunpcklqdq	%ymm3, %ymm11, %ymm9
	vpunpckhqdq	%ymm3, %ymm11, %ymm15
	vpextrq	$1, %xmm8, %rax
	vpunpckhqdq	%ymm13, %ymm14, %ymm3
	vpunpcklqdq	%ymm13, %ymm14, %ymm11
	vmovq	%xmm8, %rsi
	vperm2i128	$32, %ymm11, %ymm9, %ymm1
	vperm2i128	$32, %ymm3, %ymm15, %ymm12
	vperm2i128	$49, %ymm3, %ymm15, %ymm2
	xorq	%rax, %rsi
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	vextracti128	$0x1, %ymm8, %xmm4
	vpxor	%ymm12, %ymm1, %ymm13
	vpxor	%ymm2, %ymm14, %ymm9
	vmovq	%rsi, %xmm3
	vpextrq	$1, %xmm4, %rcx
	vpxor	%ymm9, %ymm12, %ymm15
	vpbroadcastq	%xmm3, %ymm1
	vextracti128	$0x1, %ymm13, %xmm11
	vpermq	$144, %ymm13, %ymm12
	vmovq	%xmm4, %rdi
	vpextrq	$1, %xmm11, %r8
	xorq	%rcx, %rdi
	vpblendd	$3, %ymm1, %ymm12, %ymm14
	vpand	%ymm0, %ymm1, %ymm8
	xorq	%r8, %rcx
	xorq	%rdi, %rax
	vpxor	%ymm14, %ymm2, %ymm2
	vextracti128	$0x1, %ymm15, %xmm4
	vpxor	%ymm2, %ymm8, %ymm10
	vmovq	%rax, %xmm11
	vmovq	%rcx, %xmm2
	vpbroadcastq	%xmm11, %ymm3
	vpextrq	$1, %xmm4, %r15
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm2, %ymm4
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm0, %ymm3, %ymm12
	xorq	%r15, %rsi
	vpblendd	$3, %ymm3, %ymm15, %ymm1
	vpblendd	$3, %ymm4, %ymm11, %ymm3
	vpand	%ymm0, %ymm4, %ymm15
	xorq	%rsi, %rcx
	vpxor	%ymm1, %ymm12, %ymm14
	vextracti128	$0x1, %ymm10, %xmm8
	vpxor	%ymm3, %ymm9, %ymm9
	vpxor	%ymm9, %ymm15, %ymm12
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm8, %rax
	xorq	%rdi, %rax
	vpxor	%ymm13, %ymm10, %ymm10
	vpxor	%ymm12, %ymm14, %ymm1
	vpunpcklqdq	%ymm1, %ymm13, %ymm14
	vpunpcklqdq	%ymm10, %ymm12, %ymm11
	vpunpckhqdq	%ymm10, %ymm12, %ymm8
	xorq	%rax, %r15
	vpunpckhqdq	%ymm1, %ymm13, %ymm2
	vmovq	%rax, %xmm15
	vmovq	%rsi, %xmm13
	vperm2i128	$32, %ymm8, %ymm2, %ymm4
	vperm2i128	$32, %ymm11, %ymm14, %ymm3
	vpinsrq	$1, %r15, %xmm13, %xmm10
	vpinsrq	$1, %rcx, %xmm15, %xmm12
	vpxor	%ymm4, %ymm3, %ymm1
	vperm2i128	$49, %ymm11, %ymm14, %ymm9
	vinserti128	$0x1, %xmm12, %ymm10, %ymm15
	vperm2i128	$49, %ymm8, %ymm2, %ymm2
	vpxor	%ymm6, %ymm1, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpxor	%ymm2, %ymm10, %ymm1
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm4, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm1, %ymm9, %ymm8
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm2, %ymm10, %ymm10
	vpand	%ymm2, %ymm12, %ymm2
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm10, %ymm4, %ymm15
	vpxor	%ymm2, %ymm12, %ymm1
	vpxor	%ymm13, %ymm4, %ymm4
	vpxor	%ymm2, %ymm4, %ymm13
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm9, %ymm1, %ymm14
	vpxor	%ymm15, %ymm3, %ymm3
	vpunpckhqdq	%ymm13, %ymm14, %ymm12
	vpunpcklqdq	%ymm13, %ymm14, %ymm9
	vpunpcklqdq	%ymm3, %ymm11, %ymm15
	vpunpckhqdq	%ymm3, %ymm11, %ymm3
	vpxor	%ymm10, %ymm8, %ymm8
	vperm2i128	$32, %ymm12, %ymm3, %ymm11
	vperm2i128	$49, %ymm9, %ymm15, %ymm2
	vperm2i128	$49, %ymm12, %ymm3, %ymm1
	vperm2i128	$32, %ymm9, %ymm15, %ymm14
	vpextrq	$1, %xmm8, %rsi
	vmovq	%xmm8, %rcx
	vpshufb	.LXRH_rot16(%rip), %ymm11, %ymm9

	vextracti128	$0x1, %ymm8, %xmm4
	rorx	$48, %rsi, %r15
	vpshufd	$177, %ymm2, %ymm8

	xorq	%r15, %rcx
	vpshufb	.LXRH_rot48(%rip), %ymm1, %ymm1

	vpxor	%ymm9, %ymm14, %ymm14
	vpextrq	$1, %xmm4, %rdi
	vmovq	%rcx, %xmm15
	vmovq	%xmm4, %r8
	vpxor	%ymm1, %ymm8, %ymm4
	rorx	$16, %rdi, %rax
	vpxor	%ymm4, %ymm9, %ymm10
	vextracti128	$0x1, %ymm14, %xmm13
	vpermq	$144, %ymm14, %ymm3
	rorq	$32, %r8
	vpbroadcastq	%xmm15, %ymm9
	xorq	%rax, %r8
	vpextrq	$1, %xmm13, %rsi
	vpblendd	$3, %ymm9, %ymm3, %ymm12
	vpand	%ymm0, %ymm9, %ymm11
	xorq	%r8, %r15
	xorq	%rax, %rsi
	vpxor	%ymm12, %ymm1, %ymm2
	vmovq	%r15, %xmm8
	vextracti128	$0x1, %ymm10, %xmm1
	vpxor	%ymm2, %ymm11, %ymm13
	vmovq	%rsi, %xmm2
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm1, %rdi
	vpbroadcastq	%xmm2, %ymm8
	vpermq	$144, %ymm13, %ymm1
	vpermq	$144, %ymm10, %ymm10
	vpand	%ymm0, %ymm15, %ymm3
	vextracti128	$0x1, %ymm13, %xmm11
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm15, %ymm10, %ymm9
	vpblendd	$3, %ymm8, %ymm1, %ymm15
	vpand	%ymm0, %ymm8, %ymm10
	vpxor	%ymm9, %ymm3, %ymm12
	vpxor	%ymm15, %ymm4, %ymm4
	vpextrq	$1, %xmm11, %r15
	vpxor	%ymm4, %ymm10, %ymm9
	vpxor	%ymm12, %ymm14, %ymm14
	xorq	%r8, %r15
	vpxor	%ymm14, %ymm13, %ymm13
	vpxor	%ymm9, %ymm12, %ymm3
	xorq	%r15, %rdi
	rorx	$32, %r15, %rax
	vpxor	.LC27(%rip), %ymm14, %ymm14
	vpshufb	.LXRH_rot48(%rip), %ymm3, %ymm2
	xorq	%rcx, %rsi
	rorq	$16, %rdi

	vpshufd	$177, %ymm9, %ymm15
	rorq	$48, %rsi

	vpshufb	.LXRH_rot16(%rip), %ymm13, %ymm13

	vpunpckhqdq	%ymm2, %ymm14, %ymm12
	vpunpcklqdq	%ymm2, %ymm14, %ymm9
	vmovq	%rax, %xmm8
	vpunpcklqdq	%ymm13, %ymm15, %ymm11
	vpunpckhqdq	%ymm13, %ymm15, %ymm2
	vmovq	%rcx, %xmm15
	vperm2i128	$32, %ymm11, %ymm9, %ymm3
	vperm2i128	$32, %ymm2, %ymm12, %ymm4
	vpinsrq	$1, %rsi, %xmm8, %xmm10
	vpinsrq	$1, %rdi, %xmm15, %xmm13
	vpxor	%ymm4, %ymm3, %ymm14
	vperm2i128	$49, %ymm11, %ymm9, %ymm9
	vinserti128	$0x1, %xmm10, %ymm13, %ymm15
	vperm2i128	$49, %ymm2, %ymm12, %ymm1
	vpxor	%ymm6, %ymm14, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpxor	%ymm1, %ymm10, %ymm2
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm4, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm2, %ymm9, %ymm8
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm1, %ymm10, %ymm10
	vpand	%ymm1, %ymm12, %ymm1
	vpor	%ymm14, %ymm2, %ymm12
	vpor	%ymm10, %ymm4, %ymm15
	vpxor	%ymm1, %ymm12, %ymm2
	vpxor	%ymm13, %ymm4, %ymm4
	vpxor	%ymm9, %ymm2, %ymm14
	vpxor	%ymm1, %ymm4, %ymm13
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm15, %ymm3, %ymm3
	vpxor	%ymm10, %ymm8, %ymm8
	vpunpcklqdq	%ymm3, %ymm11, %ymm9
	vpunpckhqdq	%ymm3, %ymm11, %ymm15
	vpextrq	$1, %xmm8, %rax
	vpunpckhqdq	%ymm13, %ymm14, %ymm3
	vpunpcklqdq	%ymm13, %ymm14, %ymm11
	vmovq	%xmm8, %r15
	vperm2i128	$32, %ymm11, %ymm9, %ymm1
	vperm2i128	$32, %ymm3, %ymm15, %ymm12
	vperm2i128	$49, %ymm3, %ymm15, %ymm2
	xorq	%rax, %r15
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	vextracti128	$0x1, %ymm8, %xmm4
	vpxor	%ymm12, %ymm1, %ymm13
	vpxor	%ymm2, %ymm14, %ymm9
	vmovq	%r15, %xmm3
	vpextrq	$1, %xmm4, %rcx
	vpxor	%ymm9, %ymm12, %ymm15
	vpbroadcastq	%xmm3, %ymm1
	vextracti128	$0x1, %ymm13, %xmm11
	vpermq	$144, %ymm13, %ymm12
	vmovq	%xmm4, %rdi
	vpextrq	$1, %xmm11, %r8
	xorq	%rcx, %rdi
	vpblendd	$3, %ymm1, %ymm12, %ymm14
	vpand	%ymm0, %ymm1, %ymm8
	xorq	%r8, %rcx
	xorq	%rdi, %rax
	vpxor	%ymm14, %ymm2, %ymm2
	vextracti128	$0x1, %ymm15, %xmm4
	vpxor	%ymm2, %ymm8, %ymm10
	vmovq	%rax, %xmm11
	vmovq	%rcx, %xmm2
	vpbroadcastq	%xmm11, %ymm3
	vpextrq	$1, %xmm4, %rsi
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm2, %ymm4
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm0, %ymm3, %ymm12
	xorq	%rsi, %r15
	vpblendd	$3, %ymm3, %ymm15, %ymm1
	vpblendd	$3, %ymm4, %ymm11, %ymm3
	vpand	%ymm0, %ymm4, %ymm15
	xorq	%r15, %rcx
	vpxor	%ymm1, %ymm12, %ymm14
	vextracti128	$0x1, %ymm10, %xmm8
	vpxor	%ymm3, %ymm9, %ymm9
	vpxor	%ymm9, %ymm15, %ymm12
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm8, %rax
	xorq	%rdi, %rax
	vpxor	%ymm13, %ymm10, %ymm10
	vpxor	%ymm12, %ymm14, %ymm1
	vpunpcklqdq	%ymm1, %ymm13, %ymm14
	vpunpcklqdq	%ymm10, %ymm12, %ymm11
	vpunpckhqdq	%ymm10, %ymm12, %ymm8
	xorq	%rax, %rsi
	vpunpckhqdq	%ymm1, %ymm13, %ymm2
	vmovq	%rax, %xmm15
	vmovq	%r15, %xmm13
	vperm2i128	$32, %ymm8, %ymm2, %ymm4
	vperm2i128	$32, %ymm11, %ymm14, %ymm3
	vpinsrq	$1, %rsi, %xmm13, %xmm10
	vpinsrq	$1, %rcx, %xmm15, %xmm12
	vpxor	%ymm4, %ymm3, %ymm1
	vperm2i128	$49, %ymm11, %ymm14, %ymm9
	vinserti128	$0x1, %xmm12, %ymm10, %ymm15
	vperm2i128	$49, %ymm8, %ymm2, %ymm2
	vpxor	%ymm6, %ymm1, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpxor	%ymm2, %ymm10, %ymm1
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm4, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm1, %ymm9, %ymm8
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm2, %ymm10, %ymm10
	vpand	%ymm2, %ymm12, %ymm2
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm10, %ymm4, %ymm15
	vpxor	%ymm2, %ymm12, %ymm1
	vpxor	%ymm13, %ymm4, %ymm4
	vpxor	%ymm2, %ymm4, %ymm13
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm9, %ymm1, %ymm14
	vpxor	%ymm10, %ymm8, %ymm8
	vpxor	%ymm15, %ymm3, %ymm3
	vpunpckhqdq	%ymm13, %ymm14, %ymm12
	vpunpcklqdq	%ymm3, %ymm11, %ymm15
	vextracti128	$0x1, %ymm8, %xmm4
	vpunpckhqdq	%ymm3, %ymm11, %ymm3
	vpextrq	$1, %xmm8, %r15
	vpunpcklqdq	%ymm13, %ymm14, %ymm9
	vperm2i128	$49, %ymm12, %ymm3, %ymm1
	vperm2i128	$32, %ymm12, %ymm3, %ymm14
	vpextrq	$1, %xmm4, %rsi
	leaq	(%r15,%r15), %rax
	shrq	$3, %r15
	vpand	.LC1(%rip), %ymm14, %ymm13
	andq	%r11, %r15
	vperm2i128	$49, %ymm9, %ymm15, %ymm2
	andq	%rbx, %rax
	vpand	.LC2(%rip), %ymm14, %ymm3
	vperm2i128	$32, %ymm9, %ymm15, %ymm11
	vmovq	%xmm4, %rdi
	orq	%r15, %rax
	leaq	0(,%rsi,8), %r8
	shrq	%rsi
	vmovq	%xmm8, %rcx
	movabsq	$3689348814741910323, %r15
	andq	%r14, %r8
	vpsrlq	$3, %ymm13, %ymm15
	andq	%r13, %rsi
	xorq	%rax, %rcx
	vpand	.LC3(%rip), %ymm2, %ymm14
	vpsllq	$1, %ymm3, %ymm9
	orq	%r8, %rsi
	vpand	.LC4(%rip), %ymm2, %ymm2
	vpand	.LC5(%rip), %ymm1, %ymm8
	vpor	%ymm15, %ymm9, %ymm12
	vpand	.LC6(%rip), %ymm1, %ymm1
	vpsrlq	$2, %ymm14, %ymm10
	vpxor	%ymm12, %ymm11, %ymm11
	leaq	0(,%rdi,4), %r8
	vpsllq	$2, %ymm2, %ymm4
	shrq	$2, %rdi
	vmovq	%rcx, %xmm2
	andq	%r15, %rdi
	vpsrlq	$1, %ymm8, %ymm13
	vpor	%ymm10, %ymm4, %ymm3
	andq	%r12, %r8
	vpsllq	$3, %ymm1, %ymm15
	vpbroadcastq	%xmm2, %ymm4
	vpermq	$144, %ymm11, %ymm8
	orq	%rdi, %r8
	vpor	%ymm13, %ymm15, %ymm9
	vextracti128	$0x1, %ymm11, %xmm10
	vpand	%ymm0, %ymm4, %ymm15
	xorq	%rsi, %r8
	vpxor	%ymm9, %ymm3, %ymm14
	vpblendd	$3, %ymm4, %ymm8, %ymm13
	vpextrq	$1, %xmm10, %rdi
	xorq	%r8, %rax
	vpxor	%ymm14, %ymm12, %ymm12
	vpxor	%ymm13, %ymm9, %ymm1
	vmovq	%rax, %xmm10
	xorq	%rsi, %rdi
	vpxor	%ymm1, %ymm15, %ymm9
	vextracti128	$0x1, %ymm12, %xmm3
	vmovq	%rdi, %xmm1
	vpbroadcastq	%xmm10, %ymm2
	vpextrq	$1, %xmm3, %rsi
	vpermq	$144, %ymm9, %ymm10
	vpbroadcastq	%xmm1, %ymm3
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm0, %ymm2, %ymm4
	xorq	%rsi, %rcx
	vpblendd	$3, %ymm2, %ymm12, %ymm8
	vextracti128	$0x1, %ymm9, %xmm15
	vpand	%ymm0, %ymm3, %ymm12
	xorq	%rcx, %rdi
	vpblendd	$3, %ymm3, %ymm10, %ymm2
	vpextrq	$1, %xmm15, %rax
	vpxor	%ymm8, %ymm4, %ymm13
	vpxor	%ymm2, %ymm14, %ymm14
	xorq	%r8, %rax
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm14, %ymm12, %ymm8
	xorq	%rax, %rsi
	vpxor	%ymm11, %ymm9, %ymm9
	vpxor	%ymm8, %ymm13, %ymm4
	vpand	.LC5(%rip), %ymm4, %ymm13
	vpand	.LC6(%rip), %ymm4, %ymm1
	leaq	0(,%rsi,8), %r8
	shrq	%rsi
	vpand	.LC3(%rip), %ymm8, %ymm2
	vpsrlq	$1, %ymm13, %ymm15
	vpand	.LC4(%rip), %ymm8, %ymm14
	andq	%r14, %r8
	andq	%r13, %rsi
	vpand	.LC1(%rip), %ymm9, %ymm13
	vpsllq	$3, %ymm1, %ymm3
	vpand	.LC2(%rip), %ymm9, %ymm9
	orq	%r8, %rsi
	vpsrlq	$2, %ymm2, %ymm12
	vpsllq	$2, %ymm14, %ymm8
	vpor	%ymm15, %ymm3, %ymm10
	vpsllq	$1, %ymm9, %ymm1
	vpsrlq	$3, %ymm13, %ymm15
	vpor	%ymm12, %ymm8, %ymm4
	leaq	0(,%rax,4), %r14
	shrq	$2, %rax
	vpxor	.LC28(%rip), %ymm11, %ymm11
	andq	%r15, %rax
	andq	%r12, %r14
	leaq	(%rdi,%rdi), %r12
	shrq	$3, %rdi
	vpor	%ymm15, %ymm1, %ymm3
	orq	%rax, %r14
	andq	%r11, %rdi
	andq	%r12, %rbx
	vpunpcklqdq	%ymm10, %ymm11, %ymm12
	vpunpckhqdq	%ymm3, %ymm4, %ymm8
	vpunpckhqdq	%ymm10, %ymm11, %ymm10
	orq	%rdi, %rbx
	vpunpcklqdq	%ymm3, %ymm4, %ymm14
	vmovq	%r14, %xmm13
	vmovq	%rcx, %xmm15
	vperm2i128	$32, %ymm14, %ymm12, %ymm4
	vperm2i128	$32, %ymm8, %ymm10, %ymm2
	vpinsrq	$1, %rbx, %xmm13, %xmm3
	vpinsrq	$1, %rsi, %xmm15, %xmm11
	vperm2i128	$49, %ymm8, %ymm10, %ymm1
	vpxor	%ymm2, %ymm4, %ymm10
	vinserti128	$0x1, %xmm3, %ymm11, %ymm15
	vperm2i128	$49, %ymm14, %ymm12, %ymm9
	vpxor	%ymm6, %ymm10, %ymm10
	vpxor	%ymm15, %ymm4, %ymm14
	vpxor	%ymm1, %ymm10, %ymm8
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm4, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm2, %ymm10
	vpand	%ymm15, %ymm4, %ymm4
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm8, %ymm9, %ymm3
	vpxor	%ymm13, %ymm4, %ymm4
	vpxor	%ymm1, %ymm10, %ymm10
	vpand	%ymm1, %ymm12, %ymm1
	vpor	%ymm14, %ymm8, %ymm12
	vpor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm1, %ymm12, %ymm8
	vpxor	%ymm13, %ymm2, %ymm2
	vpxor	%ymm3, %ymm11, %ymm11
	vpxor	%ymm9, %ymm8, %ymm14
	vpxor	%ymm10, %ymm3, %ymm3
	vpxor	%ymm1, %ymm2, %ymm13
	vpxor	%ymm15, %ymm4, %ymm4
	vpextrq	$1, %xmm3, %r11
	vpunpcklqdq	%ymm4, %ymm11, %ymm15
	vpunpckhqdq	%ymm13, %ymm14, %ymm1
	vpunpcklqdq	%ymm13, %ymm14, %ymm9
	vpunpckhqdq	%ymm4, %ymm11, %ymm4
	vmovq	%xmm3, %rbx
	vperm2i128	$32, %ymm9, %ymm15, %ymm12
	vperm2i128	$32, %ymm1, %ymm4, %ymm11
	vperm2i128	$49, %ymm1, %ymm4, %ymm2
	vperm2i128	$49, %ymm9, %ymm15, %ymm14
	xorq	%r11, %rbx
	vextracti128	$0x1, %ymm3, %xmm8
	vpxor	%ymm11, %ymm12, %ymm13
	vpxor	%ymm2, %ymm14, %ymm15
	vmovq	%rbx, %xmm1
	vpxor	%ymm15, %ymm11, %ymm9
	vpextrq	$1, %xmm8, %rcx
	vpbroadcastq	%xmm1, %ymm12
	vextracti128	$0x1, %ymm13, %xmm4
	vpermq	$144, %ymm13, %ymm11
	vmovq	%xmm8, %r13
	vpblendd	$3, %ymm12, %ymm11, %ymm14
	vpextrq	$1, %xmm4, %r15
	vpand	%ymm0, %ymm12, %ymm3
	vpxor	%ymm14, %ymm2, %ymm2
	vextracti128	$0x1, %ymm9, %xmm8
	xorq	%rcx, %r13
	xorq	%r13, %r11
	vpxor	%ymm2, %ymm3, %ymm10
	vpextrq	$1, %xmm8, %rdi
	xorq	%r15, %rcx
	vmovq	%r11, %xmm4
	vmovq	%rcx, %xmm2
	vpermq	$144, %ymm9, %ymm9
	xorq	%rdi, %rbx
	vpbroadcastq	%xmm4, %ymm1
	vpbroadcastq	%xmm2, %ymm8
	vpermq	$144, %ymm10, %ymm4
	xorq	%rbx, %rcx
	vpblendd	$3, %ymm1, %ymm9, %ymm12
	vpand	%ymm0, %ymm1, %ymm11
	vpblendd	$3, %ymm8, %ymm4, %ymm1
	movabsq	$-71777214294589696, %r15
	vpxor	%ymm12, %ymm11, %ymm14
	vextracti128	$0x1, %ymm10, %xmm3
	vpand	%ymm0, %ymm8, %ymm12
	vpxor	%ymm1, %ymm15, %ymm15
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm3, %rsi
	vpxor	%ymm15, %ymm12, %ymm11
	xorq	%r13, %rsi
	vpxor	%ymm13, %ymm10, %ymm3
	movabsq	$4222189076152335, %r13
	vpxor	%ymm11, %ymm14, %ymm10
	vpunpcklqdq	%ymm3, %ymm11, %ymm2
	vpunpckhqdq	%ymm3, %ymm11, %ymm4
	xorq	%rsi, %rdi
	vpunpcklqdq	%ymm10, %ymm13, %ymm9
	vpunpckhqdq	%ymm10, %ymm13, %ymm14
	vmovq	%rsi, %xmm12
	vmovq	%rbx, %xmm13
	vperm2i128	$32, %ymm2, %ymm9, %ymm1
	vperm2i128	$32, %ymm4, %ymm14, %ymm8
	movabsq	$-4222189076152336, %rbx
	vpinsrq	$1, %rcx, %xmm12, %xmm11
	vpinsrq	$1, %rdi, %xmm13, %xmm3
	vperm2i128	$49, %ymm2, %ymm9, %ymm15
	vpxor	%ymm8, %ymm1, %ymm10
	vinserti128	$0x1, %xmm11, %ymm3, %ymm2
	vperm2i128	$49, %ymm4, %ymm14, %ymm9
	vpxor	%ymm2, %ymm1, %ymm14
	vpxor	%ymm6, %ymm10, %ymm4
	vpandn	%ymm2, %ymm15, %ymm11
	vpxor	%ymm9, %ymm4, %ymm6
	vpand	%ymm14, %ymm4, %ymm10
	vpxor	%ymm2, %ymm15, %ymm12
	vpand	%ymm6, %ymm15, %ymm3
	vpxor	%ymm10, %ymm11, %ymm4
	vpand	%ymm2, %ymm8, %ymm10
	vpxor	%ymm3, %ymm4, %ymm11
	vpxor	%ymm12, %ymm10, %ymm4
	vpor	%ymm15, %ymm1, %ymm13
	vpxor	%ymm9, %ymm4, %ymm10
	vpand	%ymm2, %ymm1, %ymm1
	vpand	%ymm9, %ymm12, %ymm9
	vpor	%ymm14, %ymm6, %ymm12
	vpxor	%ymm10, %ymm3, %ymm3
	vpor	%ymm10, %ymm8, %ymm4
	vpxor	%ymm9, %ymm12, %ymm6
	vpxor	%ymm13, %ymm1, %ymm2
	vpxor	%ymm13, %ymm8, %ymm8
	vpxor	%ymm4, %ymm2, %ymm1
	vpxor	%ymm9, %ymm8, %ymm13
	vpxor	%ymm15, %ymm6, %ymm15
	vpextrq	$1, %xmm3, %rax
	vpunpcklqdq	%ymm1, %ymm11, %ymm14
	vpunpcklqdq	%ymm13, %ymm15, %ymm2
	vpunpckhqdq	%ymm1, %ymm11, %ymm11
	vpunpckhqdq	%ymm13, %ymm15, %ymm1
	vperm2i128	$49, %ymm2, %ymm14, %ymm12
	movq	%rax, %r11
	vperm2i128	$32, %ymm1, %ymm11, %ymm9
	vperm2i128	$49, %ymm1, %ymm11, %ymm6
	vperm2i128	$32, %ymm2, %ymm14, %ymm4
	salq	$4, %r11
	shrq	$12, %rax
	vextracti128	$0x1, %ymm3, %xmm15
	vpand	%ymm5, %ymm6, %ymm10
	andq	%rbx, %r11
	vpand	.LC8(%rip), %ymm9, %ymm8
	vpand	%ymm7, %ymm6, %ymm6
	vmovq	%xmm15, %r12
	andq	%r13, %rax
	vpand	.LC9(%rip), %ymm9, %ymm14
	orq	%rax, %r11
	movq	%r12, %rsi
	movabsq	$1152657617789587455, %rbx
	vpextrq	$1, %xmm15, %r8
	vpsrlq	$12, %ymm8, %ymm13
	vmovq	%xmm3, %r14
	salq	$8, %rsi
	vpsllq	$4, %ymm14, %ymm11
	vpsrlq	$4, %ymm10, %ymm15
	xorq	%r11, %r14
	movq	%r8, %rcx
	vpsllq	$12, %ymm6, %ymm8
	vpor	%ymm13, %ymm11, %ymm1
	salq	$12, %rcx
	andq	%r15, %rsi
	vpor	%ymm15, %ymm8, %ymm13
	vpxor	%ymm1, %ymm4, %ymm4
	shrq	$4, %r8
	andq	%r10, %rcx
	shrq	$8, %r12
	andq	%rbx, %r8
	movq	%r14, %r13
	andq	%r9, %r12
	orq	%r8, %rcx
	vpshufb	.LXRH_bswap16(%rip), %ymm12, %ymm14
	vmovq	%r14, %xmm2
	orq	%r12, %rsi
	movabsq	$4222189076152335, %r14
	vpermq	$144, %ymm4, %ymm9
	xorq	%rcx, %rsi
	vpbroadcastq	%xmm2, %ymm3
	xorq	%rsi, %r11
	vpxor	%ymm13, %ymm14, %ymm11
	vpblendd	$3, %ymm3, %ymm9, %ymm10
	vpand	%ymm0, %ymm3, %ymm15
	vpxor	%ymm11, %ymm1, %ymm12
	vextracti128	$0x1, %ymm4, %xmm1
	vpxor	%ymm10, %ymm13, %ymm6
	vpextrq	$1, %xmm1, %rdi
	vpxor	%ymm6, %ymm15, %ymm13
	vextracti128	$0x1, %ymm12, %xmm8
	xorq	%rcx, %rdi
	vpextrq	$1, %xmm8, %rax
	vmovq	%r11, %xmm14
	vmovq	%rdi, %xmm15
	vpermq	$144, %ymm13, %ymm8
	vpermq	$144, %ymm12, %ymm12
	xorq	%rax, %r13
	vpbroadcastq	%xmm15, %ymm6
	vextracti128	$0x1, %ymm13, %xmm10
	vpbroadcastq	%xmm14, %ymm2
	xorq	%r13, %rdi
	vpblendd	$3, %ymm6, %ymm8, %ymm14
	vpextrq	$1, %xmm10, %r12
	vpblendd	$3, %ymm2, %ymm12, %ymm1
	movq	%rdi, %rcx
	xorq	%rsi, %r12
	vpand	%ymm0, %ymm2, %ymm3
	vpxor	%ymm14, %ymm11, %ymm11
	shrq	$12, %rcx
	vpand	%ymm0, %ymm6, %ymm0
	vpxor	%ymm1, %ymm3, %ymm9
	xorq	%r12, %rax
	movq	%r12, %r11
	vpxor	%ymm11, %ymm0, %ymm12
	vpxor	%ymm9, %ymm4, %ymm4
	movq	%rax, %r8
	salq	$8, %r11
	vpxor	%ymm12, %ymm9, %ymm2
	vpxor	%ymm4, %ymm13, %ymm13
	salq	$12, %r8
	andq	%r15, %r11
	vpand	%ymm5, %ymm2, %ymm5
	vpand	%ymm7, %ymm2, %ymm7
	andq	%r10, %r8
	shrq	$8, %r12
	movq	%rdi, %r10
	andq	%r9, %r12
	shrq	$4, %rax
	vpsrlq	$4, %ymm5, %ymm1
	vpsllq	$12, %ymm7, %ymm3
	andq	%rbx, %rax
	orq	%r12, %r11
	vpshufb	.LXRH_bswap16(%rip), %ymm12, %ymm14
	salq	$4, %r10
	vpor	%ymm1, %ymm3, %ymm9
	andq	%r14, %rcx
	vmovq	%rbx, %xmm12
	orq	%rax, %r8
	movabsq	$-4222189076152336, %r9
	vpand	.LC8(%rip), %ymm13, %ymm0
	vpbroadcastq	%xmm12, %ymm2
	vmovq	%r13, %xmm12
	andq	%r10, %r9
	vpand	%ymm2, %ymm13, %ymm13
	vpxor	.LC29(%rip), %ymm4, %ymm4
	orq	%r9, %rcx
	vpsrlq	$12, %ymm0, %ymm11
	vmovq	%r11, %xmm0
	vpsllq	$4, %ymm13, %ymm5
	vpunpcklqdq	%ymm9, %ymm4, %ymm7
	vpor	%ymm11, %ymm5, %ymm1
	vpunpckhqdq	%ymm9, %ymm4, %ymm9
	vpinsrq	$1, %rcx, %xmm0, %xmm11
	vpunpckhqdq	%ymm1, %ymm14, %ymm15
	vpunpcklqdq	%ymm1, %ymm14, %ymm10
	vpinsrq	$1, %r8, %xmm12, %xmm2
	vperm2i128	$32, %ymm10, %ymm7, %ymm14
	vperm2i128	$32, %ymm15, %ymm9, %ymm3
	vinserti128	$0x1, %xmm11, %ymm2, %ymm13
	vperm2i128	$49, %ymm10, %ymm7, %ymm8
	vpxor	%ymm3, %ymm14, %ymm5
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vpxor	%ymm13, %ymm8, %ymm10
	vpxor	%ymm4, %ymm5, %ymm7
	vpxor	%ymm13, %ymm14, %ymm11
	vperm2i128	$49, %ymm15, %ymm9, %ymm6
	vpand	%ymm13, %ymm3, %ymm5
	vpand	%ymm11, %ymm7, %ymm12
	vpxor	%ymm6, %ymm7, %ymm15
	vpxor	%ymm10, %ymm5, %ymm7
	vpor	%ymm8, %ymm14, %ymm9
	vpandn	%ymm13, %ymm8, %ymm1
	vpxor	%ymm6, %ymm7, %ymm7
	vpand	%ymm13, %ymm14, %ymm14
	vpand	%ymm6, %ymm10, %ymm6
	vpor	%ymm11, %ymm15, %ymm10
	vpand	%ymm15, %ymm8, %ymm0
	vpxor	%ymm9, %ymm14, %ymm13
	vpxor	%ymm6, %ymm10, %ymm15
	vpxor	%ymm12, %ymm1, %ymm2
	vpor	%ymm7, %ymm3, %ymm12
	vpxor	%ymm9, %ymm3, %ymm3
	vpxor	%ymm7, %ymm0, %ymm7
	vpxor	%ymm12, %ymm13, %ymm5
	vpxor	%ymm6, %ymm3, %ymm9
	vpxor	%ymm0, %ymm2, %ymm1
	vpxor	%ymm8, %ymm15, %ymm8
	vpunpcklqdq	%ymm5, %ymm1, %ymm11
	vpextrq	$1, %xmm7, %rax
	vpunpcklqdq	%ymm9, %ymm8, %ymm14
	vpunpckhqdq	%ymm9, %ymm8, %ymm2
	vpunpckhqdq	%ymm5, %ymm1, %ymm1
	vmovq	%xmm7, %rsi
	vperm2i128	$32, %ymm2, %ymm1, %ymm12
	vperm2i128	$32, %ymm14, %ymm11, %ymm13
	vpxor	%ymm12, %ymm13, %ymm15
	vperm2i128	$49, %ymm14, %ymm11, %ymm5
	vextracti128	$0x1, %ymm7, %xmm6
	xorq	%rax, %rsi
	vmovq	%rsi, %xmm11
	vperm2i128	$49, %ymm2, %ymm1, %ymm10
	vpermq	$144, %ymm15, %ymm14
	movq	%rsi, %r11
	vpbroadcastq	%xmm11, %ymm1
	vpextrq	$1, %xmm6, %r13
	vextracti128	$0x1, %ymm15, %xmm3
	vmovdqa	.LC0(%rip), %ymm13
	vmovq	%xmm6, %r12
	vpxor	%ymm10, %ymm5, %ymm8
	vpblendd	$3, %ymm1, %ymm14, %ymm2
	xorq	%r13, %r12
	vpextrq	$1, %xmm3, %rdi
	vpxor	%ymm8, %ymm12, %ymm9
	xorq	%r12, %rax
	vpand	%ymm13, %ymm1, %ymm12
	vpxor	%ymm2, %ymm10, %ymm5
	xorq	%rdi, %r13
	vpxor	%ymm5, %ymm12, %ymm10
	vmovq	%rax, %xmm0
	vmovq	%r13, %xmm2
	vpbroadcastq	%xmm0, %ymm6
	vpbroadcastq	%xmm2, %ymm12
	vextracti128	$0x1, %ymm9, %xmm7
	vpermq	$144, %ymm10, %ymm5
	vpermq	$144, %ymm9, %ymm9
	vpand	%ymm13, %ymm6, %ymm11
	vpblendd	$3, %ymm6, %ymm9, %ymm3
	vpextrq	$1, %xmm7, %rbx
	vpblendd	$3, %ymm12, %ymm5, %ymm7
	vpxor	%ymm3, %ymm11, %ymm14
	vpand	%ymm13, %ymm12, %ymm0
	vextracti128	$0x1, %ymm10, %xmm1
	xorq	%rbx, %r11
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm14, %ymm15, %ymm15
	vmovq	%r11, %xmm5
	xorq	%r11, %r13
	vpxor	%ymm8, %ymm0, %ymm6
	vpextrq	$1, %xmm1, %r8
	vpxor	%ymm15, %ymm10, %ymm10
	xorq	%r12, %r8
	vpxor	%ymm6, %ymm14, %ymm3
	vpunpckhqdq	%ymm10, %ymm6, %ymm1
	vpunpckhqdq	%ymm3, %ymm15, %ymm11
	vpunpcklqdq	%ymm3, %ymm15, %ymm9
	vpunpcklqdq	%ymm10, %ymm6, %ymm14
	xorq	%r8, %rbx
	vmovq	%r8, %xmm12
	vperm2i128	$32, %ymm14, %ymm9, %ymm6
	vperm2i128	$32, %ymm1, %ymm11, %ymm7
	vpinsrq	$1, %r13, %xmm12, %xmm8
	vpinsrq	$1, %rbx, %xmm5, %xmm0
	vperm2i128	$49, %ymm1, %ymm11, %ymm2
	vinserti128	$0x1, %xmm8, %ymm0, %ymm3
	vpxor	%ymm7, %ymm6, %ymm11
	vperm2i128	$49, %ymm14, %ymm9, %ymm15
	vpxor	%ymm3, %ymm6, %ymm12
	vpxor	%ymm4, %ymm11, %ymm14
	vpxor	%ymm3, %ymm15, %ymm9
	vpandn	%ymm3, %ymm15, %ymm1
	vpand	%ymm12, %ymm14, %ymm8
	vpand	%ymm3, %ymm7, %ymm11
	vpxor	%ymm2, %ymm14, %ymm5
	vpxor	%ymm8, %ymm1, %ymm0
	vpxor	%ymm9, %ymm11, %ymm1
	vpor	%ymm15, %ymm6, %ymm10
	vpxor	%ymm2, %ymm1, %ymm8
	vpand	%ymm5, %ymm15, %ymm4
	vpand	%ymm2, %ymm9, %ymm9
	vpand	%ymm3, %ymm6, %ymm6
	vpor	%ymm12, %ymm5, %ymm2
	vpxor	%ymm4, %ymm0, %ymm14
	vpxor	%ymm10, %ymm6, %ymm3
	vpor	%ymm8, %ymm7, %ymm0
	vpxor	%ymm9, %ymm2, %ymm5
	vpxor	%ymm10, %ymm7, %ymm7
	vpxor	%ymm0, %ymm3, %ymm1
	vpxor	%ymm9, %ymm7, %ymm10
	vpxor	%ymm15, %ymm5, %ymm15
	vpunpcklqdq	%ymm1, %ymm14, %ymm12
	vpunpckhqdq	%ymm10, %ymm15, %ymm3
	vpunpckhqdq	%ymm1, %ymm14, %ymm14
	vpxor	%ymm8, %ymm4, %ymm4
	vperm2i128	$32, %ymm3, %ymm14, %ymm2
	vpunpcklqdq	%ymm10, %ymm15, %ymm11
	vmovq	%xmm4, %r15
	vpshufb	.LXRH_rot16(%rip), %ymm2, %ymm7

	vpextrq	$1, %xmm4, %r10
	vperm2i128	$49, %ymm11, %ymm12, %ymm6
	vperm2i128	$49, %ymm3, %ymm14, %ymm1
	rorx	$48, %r10, %r14
	vperm2i128	$32, %ymm11, %ymm12, %ymm9
	xorq	%r14, %r15
	vextracti128	$0x1, %ymm4, %xmm0
	vpxor	%ymm7, %ymm9, %ymm9
	vpshufd	$177, %ymm6, %ymm2
	vmovq	%r15, %xmm8

	vpshufb	.LXRH_rot48(%rip), %ymm1, %ymm3
	vextracti128	$0x1, %ymm9, %xmm4

	vpermq	$144, %ymm9, %ymm5
	vmovq	%xmm0, %r9
	vpextrq	$1, %xmm0, %rcx
	vpbroadcastq	%xmm8, %ymm0
	vpxor	%ymm3, %ymm2, %ymm6
	rorx	$16, %rcx, %r12
	rorx	$32, %r9, %r13
	vpblendd	$3, %ymm0, %ymm5, %ymm15
	xorq	%r12, %r13
	vpextrq	$1, %xmm4, %rsi
	vpxor	%ymm6, %ymm7, %ymm1
	vpxor	%ymm15, %ymm3, %ymm10
	vpand	%ymm13, %ymm0, %ymm7
	xorq	%r13, %r14
	vpxor	%ymm10, %ymm7, %ymm12
	vmovq	%r14, %xmm11
	vpermq	$144, %ymm1, %ymm2
	xorq	%rsi, %r12
	vmovq	%r12, %xmm5
	vpbroadcastq	%xmm11, %ymm3
	vpermq	$144, %ymm12, %ymm7
	vpbroadcastq	%xmm5, %ymm15
	vpand	%ymm13, %ymm3, %ymm4
	vextracti128	$0x1, %ymm1, %xmm14
	vpblendd	$3, %ymm15, %ymm7, %ymm10
	vpblendd	$3, %ymm3, %ymm2, %ymm1
	vextracti128	$0x1, %ymm12, %xmm0
	vpxor	%ymm1, %ymm4, %ymm8
	vpextrq	$1, %xmm14, %rdi
	vpand	%ymm13, %ymm15, %ymm13
	vpxor	%ymm10, %ymm6, %ymm6
	vpextrq	$1, %xmm0, %rax
	vpxor	%ymm8, %ymm9, %ymm9
	xorq	%rdi, %r15
	vpxor	%ymm6, %ymm13, %ymm14
	xorq	%r13, %rax
	xorq	%r15, %r12
	movq	%r15, 32(%rdx)
	vpxor	%ymm9, %ymm12, %ymm12
	vpxor	%ymm14, %ymm8, %ymm11
	xorq	%rax, %rdi
	rorx	$48, %r12, %r11
	vpshufb	.LXRH_rot48(%rip), %ymm11, %ymm15
	rorx	$16, %rdi, %rbx
	vpxor	.LC30(%rip), %ymm9, %ymm4
	movq	%rbx, 72(%rdx)
	vpshufd	$177, %ymm14, %ymm7

	rorx	$32, %rax, %r8
	vpshufb	.LXRH_rot16(%rip), %ymm12, %ymm10
	movq	%r8, 112(%rdx)

	movq	%r11, 152(%rdx)
	vmovdqu	%ymm4, (%rdx)
	vmovdqu	%ymm15, 40(%rdx)
	vmovdqu	%ymm7, 80(%rdx)
	vmovdqu	%ymm10, 120(%rdx)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
	.size	XRH_edmc_permute, .-XRH_edmc_permute
	.p2align 4
	.section	.rodata
	.align 32
	.section	.rodata.cst32,"aM",@progbits,32
	.align 32
.LC0:
	.quad	0
	.quad	0
	.quad	-1
	.quad	0
	.align 32
.LC1:
	.quad	-8608480567731124088
	.quad	-8608480567731124088
	.quad	-8608480567731124088
	.quad	-8608480567731124088
	.align 32
.LC2:
	.quad	8608480567731124087
	.quad	8608480567731124087
	.quad	8608480567731124087
	.quad	8608480567731124087
	.align 32
.LC3:
	.quad	-3689348814741910324
	.quad	-3689348814741910324
	.quad	-3689348814741910324
	.quad	-3689348814741910324
	.align 32
.LC4:
	.quad	3689348814741910323
	.quad	3689348814741910323
	.quad	3689348814741910323
	.quad	3689348814741910323
	.align 32
.LC5:
	.quad	-1229782938247303442
	.quad	-1229782938247303442
	.quad	-1229782938247303442
	.quad	-1229782938247303442
	.align 32
.LC6:
	.quad	1229782938247303441
	.quad	1229782938247303441
	.quad	1229782938247303441
	.quad	1229782938247303441
	.align 32
.LC7:
	.quad	2611923443488327891
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC8:
	.quad	-1152657617789587456
	.quad	-1152657617789587456
	.quad	-1152657617789587456
	.quad	-1152657617789587456
	.align 32
.LC9:
	.quad	1152657617789587455
	.quad	1152657617789587455
	.quad	1152657617789587455
	.quad	1152657617789587455
	.align 32
.LC10:
	.quad	-71777214294589696
	.quad	-71777214294589696
	.quad	-71777214294589696
	.quad	-71777214294589696
	.align 32
.LC11:
	.quad	71777214294589695
	.quad	71777214294589695
	.quad	71777214294589695
	.quad	71777214294589695
	.align 32
.LC12:
	.quad	-4222189076152336
	.quad	-4222189076152336
	.quad	-4222189076152336
	.quad	-4222189076152336
	.align 32
.LC13:
	.quad	4222189076152335
	.quad	4222189076152335
	.quad	4222189076152335
	.quad	4222189076152335
	.align 32
.LC14:
	.quad	1376283091369227076
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC15:
	.quad	-6626703657320631856
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC16:
	.quad	589684135938649225
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC17:
	.quad	4983270260364809079
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC18:
	.quad	-4732044268327596948
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC19:
	.quad	-4563226453097033507
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC20:
	.quad	4577018097722394903
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC21:
	.quad	-7919907764393346277
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC22:
	.quad	-3372901835766516308
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC23:
	.quad	3458046377305235383
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC24:
	.quad	-5124621466747896170
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC25:
	.quad	-5008970055469465703
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC26:
	.quad	2639559389850201335
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC27:
	.quad	577009281997405206
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC28:
	.quad	7163292796296056425
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC29:
	.quad	-6604248873402417794
	.quad	0
	.quad	0
	.quad	0
	.align 32
.LC30:
	.quad	978816653474051672
	.quad	0
	.quad	0
	.quad	0
	.set	.LC31,.LC8
	.set	.LC32,.LC9
	.align 32
.LC33:
	.quad	-1
	.quad	-1
	.quad	-1
	.quad	-1
	.set	.LC34,.LC10
	.set	.LC35,.LC12
	.set	.LC36,.LC13
	.set	.LC37,.LC2
	.set	.LC38,.LC3
	.set	.LC39,.LC5
	.set	.LC40,.LC6
	.set	.LC41,.LC11
	.align 32
.LC42:
	.quad	0
	.quad	0
	.quad	-1
	.quad	-1
	.align 32
.LC43:
	.quad	40
	.quad	48
	.quad	56
	.quad	64
	.align 32
.LC44:
	.quad	8
	.quad	16
	.quad	24
	.quad	32
	.align 32
.LC45:
	.quad	-1
	.quad	-1
	.quad	-1
	.quad	-1
	.align 32
.LXRH_rot16:
	.byte	6, 7, 0, 1, 2, 3, 4, 5, 14, 15, 8, 9, 10, 11, 12, 13
	.byte	6, 7, 0, 1, 2, 3, 4, 5, 14, 15, 8, 9, 10, 11, 12, 13
	.align 32
.LXRH_rot48:
	.byte	2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9
	.byte	2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9
	.align 32
.LXRH_bswap16:
	.byte	1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
	.byte	1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
	.text
	.p2align 4
	.globl	CryptHash
	.type	CryptHash, @function
CryptHash:
	pushq	%rbp
	vpxor	%xmm0, %xmm0, %xmm0
	movq	%rsp, %rbp
	pushq	%r15
	pushq	%r14
	movslq	%edi, %r14
	pushq	%r13
	movq	%rdx, %r13
	pushq	%r12
	movq	%rcx, %r12
	pushq	%rbx
	movq	%rsi, %rbx
	andq	$-32, %rsp
	subq	$192, %rsp
	movl	$448, 160(%rsp)
	movq	%rsp, %r15
	vmovdqa	%ymm0, (%rsp)
	vmovdqa	%ymm0, 32(%rsp)
	vmovdqa	%ymm0, 64(%rsp)
	vmovdqa	%ymm0, 96(%rsp)
	vmovdqa	%ymm0, 128(%rsp)
	cmpq	$447, %rdx
	jbe	.Lrate2
	.p2align 4,,10
	.p2align 3
.Lrate3:
	vmovdqu	(%rbx), %ymm2
	vmovdqu	32(%rbx), %xmm3
	movq	%r15, %rdi
	vmovq	48(%rbx), %xmm1
	vpxor	(%rsp), %ymm2, %ymm0
	vmovdqa	%ymm0, (%rsp)
	vpxor	32(%rsp), %xmm3, %xmm0
	vmovdqa	%xmm0, 32(%rsp)
	vmovq	48(%rsp), %xmm0
	vpxor	%xmm1, %xmm0, %xmm0
	vmovq	%xmm0, 48(%rsp)
	vzeroupper
	call	XRH_edmc_permute
	subq	$448, %r13
	addq	$56, %rbx
	cmpq	$447, %r13
	ja	.Lrate3
.Lrate2:
	movq	%r13, %r8
	movq	%r13, %rdx
	andl	$7, %r8d
	shrq	$3, %rdx
	je	.Lrate4
	leaq	-1(%rdx), %rax
	cmpq	$30, %rax
	jbe	.Lrate21
	vmovdqu	(%rbx), %ymm4
	vpxor	(%rsp), %ymm4, %ymm0
	vmovdqa	%ymm0, (%rsp)
	cmpq	$32, %rdx
	je	.Lrate6
	movl	$32, %eax
.Lrate5:
	movq	%rdx, %rcx
	subq	%rax, %rcx
	leaq	-1(%rcx), %rsi
	cmpq	$14, %rsi
	jbe	.Lrate7
	vmovdqu	(%rbx,%rax), %xmm5
	leaq	(%r15,%rax), %rsi
	vpxor	(%rsi), %xmm5, %xmm0
	vmovdqa	%xmm0, (%rsi)
	movq	%rcx, %rsi
	andq	$-16, %rsi
	addq	%rsi, %rax
	andl	$15, %ecx
	je	.Lrate6
.Lrate7:
	movzbl	(%rbx,%rax), %ecx
	xorb	%cl, (%r15,%rax)
	leaq	1(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	1(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	2(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	2(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	3(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	3(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	4(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	4(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	5(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	5(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	6(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	6(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	7(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	7(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	8(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	8(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	9(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	9(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	10(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	10(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	11(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	11(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	12(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	12(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	13(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	13(%rbx,%rax), %esi
	xorb	%sil, (%r15,%rcx)
	leaq	14(%rax), %rcx
	cmpq	%rdx, %rcx
	jnb	.Lrate6
	movzbl	14(%rbx,%rax), %eax
	xorb	%al, (%r15,%rcx)
	.p2align 4,,10
	.p2align 3
.Lrate6:
	leaq	(%r15,%rdx), %rsi
	movzbl	(%rsi), %eax
	testq	%r8, %r8
	je	.Lrate41
.Lrate12:
	movl	$8, %ecx
	movl	$255, %eax
	leaq	(%r15,%rdx), %rsi
	movl	%r8d, %edi
	subl	%r8d, %ecx
	sall	%cl, %eax
	andb	(%rbx,%rdx), %al
	xorb	(%rsi), %al
	movb	%al, (%rsi)
.Lrate10:
	movl	$128, %edx
	movl	%edi, %ecx
	shrl	%cl, %edx
	xorl	%edx, %eax
	movb	%al, (%rsi)
	cmpq	$447, %r13
	je	.Lrate42
	vzeroupper
.Lrate11:
	movq	%r15, %rdi
	xorb	$1, 55(%rsp)
	xorl	%ebx, %ebx
	call	XRH_edmc_permute
	testq	%r14, %r14
	je	.Lrate26
.Lrate13:
	movq	%r14, %rdi
	movl	$448, %eax
	movq	%rbx, %rdx
	subq	%rbx, %rdi
	cmpq	%rax, %rdi
	cmova	%rax, %rdi
	shrq	$3, %rdx
	addq	%r12, %rdx
	movq	%rdi, %rax
	shrq	$3, %rax
	cmpl	$8, %eax
	jnb	.Lrate14
	testb	$4, %al
	jne	.Lrate43
	testl	%eax, %eax
	je	.Lrate15
	movzbl	(%r15), %ecx
	movb	%cl, (%rdx)
	testb	$2, %al
	jne	.Lrate44
.Lrate15:
	addq	%rdi, %rbx
	cmpq	%r14, %rbx
	jb	.Lrate45
.Lrate26:
	leaq	-40(%rbp), %rsp
	xorl	%eax, %eax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	ret
	.p2align 4,,10
	.p2align 3
.Lrate14:
	movq	(%r15), %rcx
	leaq	8(%rdx), %r9
	movq	%r15, %r8
	andq	$-8, %r9
	movq	%rcx, (%rdx)
	movl	%eax, %ecx
	movq	-8(%r15,%rcx), %rsi
	movq	%rsi, -8(%rdx,%rcx)
	subq	%r9, %rdx
	addl	%edx, %eax
	subq	%rdx, %r8
	andl	$-8, %eax
	cmpl	$8, %eax
	jb	.Lrate15
	andl	$-8, %eax
	xorl	%edx, %edx
.Lrate18:
	movl	%edx, %ecx
	addl	$8, %edx
	movq	(%r8,%rcx), %rsi
	movq	%rsi, (%r9,%rcx)
	cmpl	%eax, %edx
	jb	.Lrate18
	addq	%rdi, %rbx
	cmpq	%r14, %rbx
	jnb	.Lrate26
.Lrate45:
	movq	%r15, %rdx
	movl	$18, %esi
	xorl	%edi, %edi
	call	XRH1280
	jmp	.Lrate13
.Lrate41:
	xorl	%edi, %edi
	jmp	.Lrate10
.Lrate43:
	movl	(%r15), %ecx
	movl	%eax, %eax
	movl	%ecx, (%rdx)
	movl	-4(%r15,%rax), %ecx
	movl	%ecx, -4(%rdx,%rax)
	jmp	.Lrate15
.Lrate42:
	movq	%r15, %rdi
	vzeroupper
	call	XRH_edmc_permute
	jmp	.Lrate11
.Lrate44:
	movl	%eax, %eax
	movzwl	-2(%r15,%rax), %ecx
	movw	%cx, -2(%rdx,%rax)
	jmp	.Lrate15
.Lrate4:
	testq	%r8, %r8
	jne	.Lrate12
	addb	$-128, (%rsp)
	vzeroupper
	jmp	.Lrate11
.Lrate21:
	xorl	%eax, %eax
	jmp	.Lrate5
	.size	CryptHash, .-CryptHash
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
