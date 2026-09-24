.file	"CryptHash_AlgorithmInstance.c"
.text
# XRH1280(R0, R1, state) is implemented in CryptHash_AlgorithmInstance.c.
# Keeping the variable-round helper in C avoids a 9000-line compiler-unrolled
# block here while the hot XRH_dmc_permute path below remains AVX2-specialized.
	.p2align 4
	.globl	XRH_dmc_permute
	.type	XRH_dmc_permute, @function
XRH_dmc_permute:
	movabsq	$-1229782938247303442, %r10
	movq	%rdi, %rax
	pushq	%r15
	pushq	%r14
	pushq	%r13
	movabsq	$-8608480567731124088, %r13
	pushq	%r12
	movabsq	$8608480567731124087, %r12
	pushq	%rbx
	vmovdqu	(%rdi), %ymm4
	vmovdqu	80(%rdi), %ymm0
	vpunpcklqdq	40(%rdi), %ymm4, %ymm5
	vpunpckhqdq	40(%rdi), %ymm4, %ymm9
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vpunpcklqdq	120(%rdi), %ymm0, %ymm1
	vpunpckhqdq	120(%rdi), %ymm0, %ymm6
	vmovq	112(%rdi), %xmm7
	vmovq	32(%rdi), %xmm10
	vpinsrq	$1, 72(%rdi), %xmm10, %xmm12
	vperm2i128	$32, %ymm6, %ymm9, %ymm2
	vperm2i128	$32, %ymm1, %ymm5, %ymm3
	vpinsrq	$1, 152(%rdi), %xmm7, %xmm8
	vperm2i128	$49, %ymm1, %ymm5, %ymm13
	vpxor	%ymm2, %ymm3, %ymm5
	vperm2i128	$49, %ymm6, %ymm9, %ymm14
	vpxor	%ymm4, %ymm5, %ymm9
	vinserti128	$0x1, %xmm8, %ymm12, %ymm15
	vpxor	%ymm14, %ymm9, %ymm1
	vpor	%ymm13, %ymm3, %ymm8
	vpxor	%ymm15, %ymm13, %ymm11
	vpxor	%ymm15, %ymm3, %ymm10
	vpand	%ymm15, %ymm2, %ymm5
	vpandn	%ymm15, %ymm13, %ymm6
	vpand	%ymm10, %ymm9, %ymm7
	vpxor	%ymm11, %ymm5, %ymm9
	vpand	%ymm1, %ymm13, %ymm0
	vpxor	%ymm7, %ymm6, %ymm12
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm14, %ymm9, %ymm7
	vpand	%ymm14, %ymm11, %ymm14
	vpxor	%ymm0, %ymm12, %ymm6
	vpxor	%ymm8, %ymm3, %ymm15
	vpor	%ymm7, %ymm2, %ymm12
	vpor	%ymm10, %ymm1, %ymm11
	vpxor	%ymm14, %ymm13, %ymm13
	vpxor	%ymm8, %ymm2, %ymm2
	vpxor	%ymm12, %ymm15, %ymm3
	vpxor	%ymm13, %ymm11, %ymm1
	vpxor	%ymm14, %ymm2, %ymm10
	vpxor	%ymm7, %ymm0, %ymm0
	vpunpcklqdq	%ymm10, %ymm1, %ymm8
	vpunpckhqdq	%ymm10, %ymm1, %ymm9
	vpunpcklqdq	%ymm3, %ymm6, %ymm5
	vpextrq	$1, %xmm0, %rcx
	vpunpckhqdq	%ymm3, %ymm6, %ymm6
	vmovq	%xmm0, %rsi
	vperm2i128	$32, %ymm9, %ymm6, %ymm12
	vperm2i128	$32, %ymm8, %ymm5, %ymm15
	vperm2i128	$49, %ymm9, %ymm6, %ymm14
	xorq	%rcx, %rsi
	vpxor	%ymm12, %ymm15, %ymm13
	vperm2i128	$49, %ymm8, %ymm5, %ymm3
	vextracti128	$0x1, %ymm0, %xmm11
	vmovq	%rsi, %xmm5
	vpxor	%ymm14, %ymm3, %ymm2
	vpextrq	$1, %xmm11, %rdx
	vmovdqa	.LC0(%rip), %ymm3
	vpbroadcastq	%xmm5, %ymm6
	vextracti128	$0x1, %ymm13, %xmm10
	vpermq	$144, %ymm13, %ymm8
	vmovq	%xmm11, %r9
	vpblendd	$3, %ymm6, %ymm8, %ymm9
	vpextrq	$1, %xmm10, %rdi
	xorq	%rdx, %r9
	vpxor	%ymm2, %ymm12, %ymm1
	vpand	%ymm3, %ymm6, %ymm15
	xorq	%rdi, %rdx
	xorq	%r9, %rcx
	vpxor	%ymm9, %ymm14, %ymm12
	vmovq	%rdx, %xmm9
	vpxor	%ymm12, %ymm15, %ymm14
	vmovq	%rcx, %xmm7
	vpbroadcastq	%xmm9, %ymm15
	vpbroadcastq	%xmm7, %ymm11
	vextracti128	$0x1, %ymm1, %xmm0
	vpermq	$144, %ymm14, %ymm12
	vpermq	$144, %ymm1, %ymm1
	vpand	%ymm3, %ymm11, %ymm5
	vpextrq	$1, %xmm0, %r8
	vpblendd	$3, %ymm11, %ymm1, %ymm10
	vpblendd	$3, %ymm15, %ymm12, %ymm0
	vpand	%ymm3, %ymm15, %ymm11
	xorq	%r8, %rsi
	vpxor	%ymm10, %ymm5, %ymm8
	vextracti128	$0x1, %ymm14, %xmm6
	vpxor	%ymm0, %ymm2, %ymm2
	xorq	%rsi, %rdx
	vpxor	%ymm2, %ymm11, %ymm7
	vpxor	%ymm8, %ymm13, %ymm13
	vpextrq	$1, %xmm6, %rbx
	vpxor	%ymm7, %ymm8, %ymm1
	vpxor	%ymm13, %ymm14, %ymm14
	vmovq	%rsi, %xmm0
	xorq	%r9, %rbx
	vpunpcklqdq	%ymm1, %ymm13, %ymm10
	vpunpckhqdq	%ymm1, %ymm13, %ymm5
	vpunpckhqdq	%ymm14, %ymm7, %ymm9
	xorq	%rbx, %r8
	vpunpcklqdq	%ymm14, %ymm7, %ymm8
	vmovq	%rbx, %xmm12
	vperm2i128	$32, %ymm9, %ymm5, %ymm15
	movabsq	$1229782938247303441, %r9
	vperm2i128	$32, %ymm8, %ymm10, %ymm6
	vpinsrq	$1, %rdx, %xmm12, %xmm11
	vpinsrq	$1, %r8, %xmm0, %xmm13
	movabsq	$-3689348814741910324, %rbx
	vinserti128	$0x1, %xmm11, %ymm13, %ymm14
	vpxor	%ymm15, %ymm6, %ymm1
	vperm2i128	$49, %ymm8, %ymm10, %ymm7
	vperm2i128	$49, %ymm9, %ymm5, %ymm2
	vpxor	%ymm14, %ymm6, %ymm13
	vpxor	%ymm4, %ymm1, %ymm9
	vpxor	%ymm14, %ymm7, %ymm10
	vpxor	%ymm2, %ymm9, %ymm5
	vpandn	%ymm14, %ymm7, %ymm11
	vpand	%ymm13, %ymm9, %ymm0
	vpand	%ymm14, %ymm15, %ymm9
	vpor	%ymm7, %ymm6, %ymm12
	vpxor	%ymm0, %ymm11, %ymm1
	vpxor	%ymm10, %ymm9, %ymm11
	vpand	%ymm14, %ymm6, %ymm6
	vpxor	%ymm2, %ymm11, %ymm9
	vpand	%ymm2, %ymm10, %ymm10
	vpor	%ymm13, %ymm5, %ymm2
	vpand	%ymm5, %ymm7, %ymm8
	vpor	%ymm9, %ymm15, %ymm0
	vpxor	%ymm10, %ymm2, %ymm5
	vpxor	%ymm12, %ymm6, %ymm14
	vpxor	%ymm12, %ymm15, %ymm15
	vpxor	%ymm7, %ymm5, %ymm7
	vpxor	%ymm0, %ymm14, %ymm6
	vpxor	%ymm10, %ymm15, %ymm12
	vpxor	%ymm8, %ymm1, %ymm1
	vpunpckhqdq	%ymm12, %ymm7, %ymm14
	vpunpcklqdq	%ymm6, %ymm1, %ymm13
	vpunpckhqdq	%ymm6, %ymm1, %ymm1
	vpunpcklqdq	%ymm12, %ymm7, %ymm11
	vperm2i128	$32, %ymm14, %ymm1, %ymm10
	vpxor	%ymm9, %ymm8, %ymm8
	vpand	.LC1(%rip), %ymm10, %ymm5
	vpand	.LC2(%rip), %ymm10, %ymm12
	vpextrq	$1, %xmm8, %r11
	vperm2i128	$49, %ymm11, %ymm13, %ymm6
	vperm2i128	$32, %ymm11, %ymm13, %ymm0
	leaq	(%r11,%r11), %rdx
	shrq	$3, %r11
	vpsrlq	$3, %ymm5, %ymm7
	vpsllq	$1, %ymm12, %ymm13
	andq	%r9, %r11
	andq	%r10, %rdx
	vperm2i128	$49, %ymm14, %ymm1, %ymm15
	vextracti128	$0x1, %ymm8, %xmm2
	vpor	%ymm7, %ymm13, %ymm11
	orq	%r11, %rdx
	vmovq	%r9, %xmm7
	vmovq	%xmm2, %r14
	vmovq	%xmm8, %r15
	movabsq	$3689348814741910323, %r11
	vpand	.LC3(%rip), %ymm6, %ymm1
	vpextrq	$1, %xmm2, %rsi
	vpxor	%ymm11, %ymm0, %ymm0
	xorq	%rdx, %r15
	vpand	.LC4(%rip), %ymm6, %ymm10
	vmovq	%r10, %xmm6
	leaq	0(,%rsi,8), %rdi
	shrq	%rsi
	vpbroadcastq	%xmm6, %ymm5
	vpbroadcastq	%xmm7, %ymm6
	andq	%r12, %rsi
	andq	%r13, %rdi
	vpand	%ymm5, %ymm15, %ymm9
	vpsllq	$2, %ymm10, %ymm8
	vpand	%ymm6, %ymm15, %ymm15
	orq	%rsi, %rdi
	vpsrlq	$2, %ymm1, %ymm14
	vpsrlq	$1, %ymm9, %ymm2
	vextracti128	$0x1, %ymm0, %xmm10
	vpsllq	$3, %ymm15, %ymm12
	vpor	%ymm14, %ymm8, %ymm1
	leaq	0(,%r14,4), %rcx
	shrq	$2, %r14
	andq	%rbx, %rcx
	vmovq	%r15, %xmm8
	vpor	%ymm2, %ymm12, %ymm13
	andq	%r11, %r14
	vpbroadcastq	%xmm8, %ymm9
	vpermq	$144, %ymm0, %ymm2
	vpxor	%ymm13, %ymm1, %ymm14
	orq	%rcx, %r14
	xorq	%rdi, %r14
	vpblendd	$3, %ymm9, %ymm2, %ymm15
	vpextrq	$1, %xmm10, %rsi
	vpand	%ymm3, %ymm9, %ymm7
	xorq	%rdi, %rsi
	vpxor	%ymm14, %ymm11, %ymm11
	xorq	%r14, %rdx
	vpxor	%ymm15, %ymm13, %ymm12
	vextracti128	$0x1, %ymm11, %xmm1
	vmovq	%rdx, %xmm10
	vpxor	%ymm12, %ymm7, %ymm13
	vmovq	%rsi, %xmm12
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm1, %r8
	vextracti128	$0x1, %ymm13, %xmm7
	vpbroadcastq	%xmm12, %ymm1
	vpermq	$144, %ymm13, %ymm10
	vpermq	$144, %ymm11, %ymm11
	vpand	%ymm3, %ymm8, %ymm2
	xorq	%r8, %r15
	vpblendd	$3, %ymm8, %ymm11, %ymm9
	vpextrq	$1, %xmm7, %rdx
	vpand	%ymm3, %ymm1, %ymm8
	xorq	%r15, %rsi
	vpblendd	$3, %ymm1, %ymm10, %ymm11
	xorq	%r14, %rdx
	vpxor	%ymm9, %ymm2, %ymm15
	vpxor	%ymm11, %ymm14, %ymm14
	xorq	%rdx, %r8
	vpxor	%ymm15, %ymm0, %ymm0
	vpxor	%ymm14, %ymm8, %ymm9
	movq	%r8, %rdi
	vpxor	%ymm0, %ymm13, %ymm13
	vpxor	%ymm9, %ymm15, %ymm15
	leaq	0(,%r8,8), %r14
	shrq	%rdi
	vpand	.LC3(%rip), %ymm9, %ymm10
	vpand	%ymm5, %ymm15, %ymm2
	vpand	%ymm6, %ymm15, %ymm12
	andq	%r13, %r14
	andq	%r12, %rdi
	vpand	.LC4(%rip), %ymm9, %ymm8
	vpsrlq	$1, %ymm2, %ymm7
	orq	%r14, %rdi
	vpand	.LC1(%rip), %ymm13, %ymm2
	vpand	.LC2(%rip), %ymm13, %ymm13
	leaq	0(,%rdx,4), %rcx
	shrq	$2, %rdx
	vpsllq	$3, %ymm12, %ymm1
	vpsrlq	$2, %ymm10, %ymm14
	andq	%r11, %rdx
	andq	%rbx, %rcx
	vpor	%ymm7, %ymm1, %ymm11
	vpsllq	$2, %ymm8, %ymm9
	leaq	(%rsi,%rsi), %r11
	orq	%rdx, %rcx
	vpsrlq	$3, %ymm2, %ymm12
	shrq	$3, %rsi
	vpor	%ymm14, %ymm9, %ymm15
	andq	%r10, %r11
	vpxor	.LC7(%rip), %ymm0, %ymm0
	vpsllq	$1, %ymm13, %ymm7
	andq	%r9, %rsi
	vpor	%ymm12, %ymm7, %ymm1
	orq	%rsi, %r11
	vmovq	%rcx, %xmm2
	vpunpcklqdq	%ymm1, %ymm15, %ymm14
	vpunpcklqdq	%ymm11, %ymm0, %ymm10
	vpunpckhqdq	%ymm1, %ymm15, %ymm15
	vpunpckhqdq	%ymm11, %ymm0, %ymm11
	vmovq	%r15, %xmm13
	vperm2i128	$32, %ymm14, %ymm10, %ymm8
	vperm2i128	$32, %ymm15, %ymm11, %ymm9
	vpinsrq	$1, %r11, %xmm2, %xmm12
	vpinsrq	$1, %rdi, %xmm13, %xmm7
	vinserti128	$0x1, %xmm12, %ymm7, %ymm7
	vpxor	%ymm9, %ymm8, %ymm1
	vperm2i128	$49, %ymm14, %ymm10, %ymm10
	vperm2i128	$49, %ymm15, %ymm11, %ymm0
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm1, %ymm11
	vpxor	%ymm0, %ymm11, %ymm1
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm13, %ymm11, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpand	%ymm1, %ymm10, %ymm2
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm2, %ymm2
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm2, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm2, %r15
	vperm2i128	$32, %ymm10, %ymm7, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm2, %xmm9
	xorq	%rsi, %r15
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm7
	vmovq	%r15, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vpextrq	$1, %xmm9, %rdi
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r14
	xorq	%rdi, %r14
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %rdx
	xorq	%r14, %rsi
	vpand	%ymm3, %ymm0, %ymm2
	vpxor	%ymm15, %ymm1, %ymm1
	xorq	%rdx, %rdi
	vpxor	%ymm1, %ymm2, %ymm11
	vextracti128	$0x1, %ymm12, %xmm9
	vmovq	%rsi, %xmm10
	vmovq	%rdi, %xmm1
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %r8
	vpbroadcastq	%xmm1, %ymm10
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	xorq	%r8, %r15
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpand	%ymm3, %ymm8, %ymm13
	vpblendd	$3, %ymm10, %ymm9, %ymm8
	xorq	%r15, %rdi
	vpxor	%ymm0, %ymm13, %ymm15
	vpand	%ymm3, %ymm10, %ymm12
	vextracti128	$0x1, %ymm11, %xmm2
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm2, %rcx
	vpxor	%ymm7, %ymm12, %ymm13
	xorq	%r14, %rcx
	vpxor	%ymm14, %ymm11, %ymm11
	movabsq	$-4222189076152336, %r14
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpckhqdq	%ymm11, %ymm13, %ymm1
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	xorq	%rcx, %r8
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpckhqdq	%ymm0, %ymm14, %ymm2
	vmovq	%rcx, %xmm9
	vmovq	%r15, %xmm13
	vperm2i128	$32, %ymm1, %ymm2, %ymm8
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	movabsq	$-1152657617789587456, %r15
	vpinsrq	$1, %rdi, %xmm9, %xmm12
	vpinsrq	$1, %r8, %xmm13, %xmm14
	vperm2i128	$49, %ymm1, %ymm2, %ymm0
	movabsq	$4222189076152335, %rdi
	vinserti128	$0x1, %xmm12, %ymm14, %ymm2
	vpxor	%ymm8, %ymm7, %ymm11
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm2, %ymm7, %ymm15
	vpandn	%ymm2, %ymm10, %ymm12
	vpxor	%ymm0, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm2, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm2, %ymm8, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm2, %ymm7, %ymm7
	vpand	%ymm0, %ymm13, %ymm13
	vpxor	%ymm0, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm0
	vpand	%ymm9, %ymm10, %ymm1
	vpor	%ymm11, %ymm8, %ymm2
	vpxor	%ymm13, %ymm0, %ymm9
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm1, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm2, %ymm7, %ymm7
	vmovq	%r15, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vperm2i128	$49, %ymm12, %ymm15, %ymm2
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vpbroadcastq	.LC32(%rip), %ymm12
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	%xmm9, %ymm9
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpand	%ymm9, %ymm10, %ymm14
	vpand	%ymm12, %ymm10, %ymm13
	vpxor	%ymm11, %ymm1, %ymm1
	vpsrlq	$12, %ymm14, %ymm15
	vpsllq	$4, %ymm13, %ymm10
	vextracti128	$0x1, %ymm1, %xmm0
	vpextrq	$1, %xmm1, %r11
	vmovq	%xmm1, %rsi
	vpor	%ymm15, %ymm10, %ymm1
	vpbroadcastq	.LC34(%rip), %ymm10
	movq	%r11, %r8
	vmovq	%xmm0, %rcx
	shrq	$12, %r11
	vpxor	%ymm1, %ymm8, %ymm8
	salq	$4, %r8
	vpextrq	$1, %xmm0, %rdx
	andq	%rdi, %r11
	andq	%r14, %r8
	movq	%rdx, %rdi
	shrq	$4, %rdx
	vpbroadcastq	.LC35(%rip), %ymm11
	orq	%r11, %r8
	salq	$12, %rdi
	movabsq	$71777214294589695, %r14
	vmovq	%r14, %xmm0
	andq	%r15, %rdi
	xorq	%r8, %rsi
	movabsq	$1152657617789587455, %r11
	vpand	%ymm11, %ymm7, %ymm14
	vpbroadcastq	%xmm0, %ymm13
	andq	%r11, %rdx
	movabsq	$-71777214294589696, %r11
	vpsrlq	$4, %ymm14, %ymm0
	vpshufb	.LXRH_bswap16(%rip), %ymm2, %ymm15
	orq	%rdi, %rdx
	movq	%rcx, %rdi
	salq	$8, %rdi
	shrq	$8, %rcx
	vpbroadcastq	.LC36(%rip), %ymm14
	andq	%r14, %rcx
	andq	%r11, %rdi
	vpand	%ymm14, %ymm7, %ymm7
	orq	%rcx, %rdi
	vpsllq	$12, %ymm7, %ymm7
	xorq	%rdx, %rdi
	vpor	%ymm0, %ymm7, %ymm7
	vmovq	%rsi, %xmm0
	xorq	%rdi, %r8
	vpxor	%ymm7, %ymm15, %ymm2
	vpxor	%ymm2, %ymm1, %ymm15
	vextracti128	$0x1, %ymm8, %xmm1
	vpextrq	$1, %xmm1, %rcx
	vpbroadcastq	%xmm0, %ymm1
	vpermq	$144, %ymm8, %ymm0
	vpblendd	$3, %ymm1, %ymm0, %ymm0
	vpand	%ymm3, %ymm1, %ymm1
	xorq	%rcx, %rdx
	vpxor	%ymm0, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm0
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm1, %ymm1
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm0, %rcx
	vpbroadcastq	%xmm7, %ymm0
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm0, %ymm15, %ymm7
	vextracti128	$0x1, %ymm1, %xmm15
	vpand	%ymm3, %ymm0, %ymm0
	vpextrq	$1, %xmm15, %r8
	vpxor	%ymm7, %ymm0, %ymm0
	vmovq	%rdx, %xmm7
	xorq	%rsi, %rdx
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm1, %ymm15
	vpxor	%ymm0, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm3, %ymm7, %ymm7
	movq	%rcx, %r8
	shrq	$4, %rcx
	vpxor	%ymm15, %ymm2, %ymm2
	vpxor	%ymm8, %ymm1, %ymm1
	vpxor	%ymm2, %ymm7, %ymm15
	salq	$12, %r8
	vpand	%ymm9, %ymm1, %ymm9
	vpand	%ymm12, %ymm1, %ymm12
	vpxor	%ymm15, %ymm0, %ymm0
	andq	%r15, %r8
	vpand	%ymm11, %ymm0, %ymm11
	vpand	%ymm14, %ymm0, %ymm14
	movabsq	$1152657617789587455, %r15
	andq	%r15, %rcx
	vpsrlq	$4, %ymm11, %ymm7
	movabsq	$4222189076152335, %r15
	orq	%r8, %rcx
	movq	%rdi, %r8
	shrq	$8, %rdi
	vpxor	.LC14(%rip), %ymm8, %ymm8
	salq	$8, %r8
	vpsllq	$12, %ymm14, %ymm2
	andq	%r14, %rdi
	movabsq	$-4222189076152336, %r14
	andq	%r11, %r8
	vpor	%ymm7, %ymm2, %ymm0
	orq	%rdi, %r8
	movq	%rdx, %rdi
	shrq	$12, %rdx
	vpunpcklqdq	%ymm0, %ymm8, %ymm10
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm14
	salq	$4, %rdi
	andq	%r15, %rdx
	vpunpckhqdq	%ymm0, %ymm8, %ymm0
	vpsrlq	$12, %ymm9, %ymm7
	vpsllq	$4, %ymm12, %ymm1
	andq	%r14, %rdi
	vpor	%ymm7, %ymm1, %ymm2
	orq	%rdx, %rdi
	vmovq	%r8, %xmm15
	vpunpcklqdq	%ymm2, %ymm14, %ymm11
	vpunpckhqdq	%ymm2, %ymm14, %ymm13
	vmovq	%rsi, %xmm7
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm0, %ymm9
	vpinsrq	$1, %rdi, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm7, %xmm12
	vpxor	%ymm9, %ymm8, %ymm1
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vinserti128	$0x1, %xmm14, %ymm12, %ymm7
	vperm2i128	$49, %ymm13, %ymm0, %ymm0
	vpxor	%ymm4, %ymm1, %ymm11
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
	vpand	%ymm1, %ymm10, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm2, %ymm2
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpextrq	$1, %xmm2, %rdx
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vmovq	%xmm2, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm7, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	xorq	%rdx, %rsi
	vextracti128	$0x1, %ymm2, %xmm9
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpextrq	$1, %xmm9, %rcx
	vpbroadcastq	%xmm8, %ymm0
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r11
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %r8
	vpand	%ymm3, %ymm0, %ymm2
	vpxor	%ymm15, %ymm1, %ymm1
	vextracti128	$0x1, %ymm12, %xmm9
	xorq	%rcx, %r11
	xorq	%r11, %rdx
	vpxor	%ymm1, %ymm2, %ymm11
	vpextrq	$1, %xmm9, %rdi
	xorq	%r8, %rcx
	vmovq	%rdx, %xmm10
	vmovq	%rcx, %xmm1
	vpermq	$144, %ymm12, %ymm12
	xorq	%rdi, %rsi
	vpbroadcastq	%xmm10, %ymm8
	vpbroadcastq	%xmm1, %ymm9
	vpermq	$144, %ymm11, %ymm10
	xorq	%rsi, %rcx
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpand	%ymm3, %ymm8, %ymm13
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	vpxor	%ymm0, %ymm13, %ymm15
	vpand	%ymm3, %ymm9, %ymm12
	vextracti128	$0x1, %ymm11, %xmm2
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm2, %r14
	vpxor	%ymm7, %ymm12, %ymm13
	xorq	%r11, %r14
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpckhqdq	%ymm11, %ymm13, %ymm2
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	xorq	%r14, %rdi
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpckhqdq	%ymm0, %ymm14, %ymm1
	vmovq	%r14, %xmm8
	vmovq	%rsi, %xmm13
	vperm2i128	$32, %ymm2, %ymm1, %ymm9
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	vpinsrq	$1, %rcx, %xmm8, %xmm12
	vpinsrq	$1, %rdi, %xmm13, %xmm14
	vperm2i128	$49, %ymm2, %ymm1, %ymm1
	vpxor	%ymm9, %ymm7, %ymm11
	vinserti128	$0x1, %xmm12, %ymm14, %ymm2
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm2, %ymm7, %ymm15
	vpandn	%ymm2, %ymm10, %ymm12
	vpxor	%ymm1, %ymm11, %ymm0
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm2, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm2, %ymm9, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm2, %ymm7, %ymm7
	vpand	%ymm0, %ymm10, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpor	%ymm11, %ymm9, %ymm2
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm2, %ymm7, %ymm7
	vpxor	%ymm10, %ymm0, %ymm10
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpcklqdq	%ymm14, %ymm10, %ymm2
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm7
	vperm2i128	$49, %ymm2, %ymm15, %ymm1
	vperm2i128	$32, %ymm2, %ymm15, %ymm13
	vperm2i128	$32, %ymm7, %ymm12, %ymm10
	vperm2i128	$49, %ymm7, %ymm12, %ymm0
	vpextrq	$1, %xmm8, %rsi
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm12
	vmovq	%xmm8, %r11
	rorx	$48, %rsi, %rcx
	vpsrlq	$32, %ymm1, %ymm2
	vpsllq	$32, %ymm1, %ymm10
	xorq	%rcx, %r11
	vextracti128	$0x1, %ymm8, %xmm9
	vpshufb	.LXRH_rot48(%rip), %ymm0, %ymm0
	vpor	%ymm2, %ymm10, %ymm8
	vpxor	%ymm12, %ymm13, %ymm13
	vpxor	%ymm0, %ymm8, %ymm11
	vmovq	%r11, %xmm15
	vmovq	%xmm9, %r8
	vpextrq	$1, %xmm9, %rdx
	vpermq	$144, %ymm13, %ymm2
	vpxor	%ymm11, %ymm12, %ymm9
	rorx	$32, %r8, %r14
	vpbroadcastq	%xmm15, %ymm12
	vextracti128	$0x1, %ymm13, %xmm14
	rorx	$16, %rdx, %rdi
	xorq	%rdi, %r14
	vpblendd	$3, %ymm12, %ymm2, %ymm10
	vpextrq	$1, %xmm14, %r8
	vpand	%ymm3, %ymm12, %ymm7
	xorq	%r14, %rcx
	vpxor	%ymm10, %ymm0, %ymm1
	xorq	%rdi, %r8
	vextracti128	$0x1, %ymm9, %xmm0
	vpxor	%ymm1, %ymm7, %ymm14
	vmovq	%rcx, %xmm8
	vmovq	%r8, %xmm1
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm0, %rsi
	vpbroadcastq	%xmm1, %ymm8
	vpermq	$144, %ymm14, %ymm0
	vpermq	$144, %ymm9, %ymm9
	vpand	%ymm3, %ymm15, %ymm2
	xorq	%rsi, %r11
	vpblendd	$3, %ymm15, %ymm9, %ymm12
	vpblendd	$3, %ymm8, %ymm0, %ymm15
	vpand	%ymm3, %ymm8, %ymm9
	xorq	%r11, %r8
	vpxor	%ymm12, %ymm2, %ymm10
	vpxor	%ymm15, %ymm11, %ymm11
	vextracti128	$0x1, %ymm14, %xmm7
	vpxor	%ymm11, %ymm9, %ymm12
	vpxor	%ymm10, %ymm13, %ymm13
	vpextrq	$1, %xmm7, %rdx
	vpxor	%ymm13, %ymm14, %ymm14
	vpshufd	$177, %ymm12, %ymm15
	vpxor	%ymm12, %ymm10, %ymm10
	xorq	%r14, %rdx
	vpxor	.LC15(%rip), %ymm13, %ymm13
	vpshufb	.LXRH_rot48(%rip), %ymm10, %ymm1
	xorq	%rdx, %rsi
	rorx	$32, %rdx, %rdi
	rorq	$16, %rsi
	rorq	$48, %r8
	vpshufb	.LXRH_rot16(%rip), %ymm14, %ymm12
	vmovq	%r11, %xmm11
	vpunpcklqdq	%ymm1, %ymm13, %ymm14
	vpunpckhqdq	%ymm1, %ymm13, %ymm7
	vpunpckhqdq	%ymm12, %ymm15, %ymm2
	vpunpcklqdq	%ymm12, %ymm15, %ymm10
	vmovq	%rdi, %xmm1
	vperm2i128	$32, %ymm2, %ymm7, %ymm9
	vperm2i128	$32, %ymm10, %ymm14, %ymm8
	vpinsrq	$1, %r8, %xmm1, %xmm15
	vpinsrq	$1, %rsi, %xmm11, %xmm12
	vperm2i128	$49, %ymm2, %ymm7, %ymm0
	vinserti128	$0x1, %xmm15, %ymm12, %ymm7
	vpxor	%ymm9, %ymm8, %ymm2
	vperm2i128	$49, %ymm10, %ymm14, %ymm10
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm2, %ymm11
	vpxor	%ymm0, %ymm11, %ymm1
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm13, %ymm11, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm0, %ymm11, %ymm11
	vpand	%ymm0, %ymm13, %ymm0
	vpor	%ymm15, %ymm1, %ymm13
	vpand	%ymm1, %ymm10, %ymm2
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm0, %ymm13, %ymm1
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm0, %ymm9, %ymm14
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm2, %ymm2
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm2, %rdx
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm2, %rsi
	vperm2i128	$32, %ymm10, %ymm7, %ymm0
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm2, %xmm9
	xorq	%rdx, %rsi
	vpxor	%ymm13, %ymm0, %ymm14
	vpxor	%ymm1, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm0
	vpextrq	$1, %xmm9, %rcx
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r11
	xorq	%rcx, %r11
	vpblendd	$3, %ymm0, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %r8
	xorq	%r11, %rdx
	vpand	%ymm3, %ymm0, %ymm2
	vpxor	%ymm15, %ymm1, %ymm1
	xorq	%r8, %rcx
	vpxor	%ymm1, %ymm2, %ymm11
	vextracti128	$0x1, %ymm12, %xmm9
	vmovq	%rdx, %xmm10
	vmovq	%rcx, %xmm1
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %rdi
	vpermq	$144, %ymm11, %ymm10
	vpbroadcastq	%xmm1, %ymm9
	vpermq	$144, %ymm12, %ymm12
	xorq	%rdi, %rsi
	vpblendd	$3, %ymm8, %ymm12, %ymm0
	vpand	%ymm3, %ymm8, %ymm13
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	xorq	%rsi, %rcx
	vpxor	%ymm0, %ymm13, %ymm15
	vpand	%ymm3, %ymm9, %ymm12
	vextracti128	$0x1, %ymm11, %xmm2
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm2, %r14
	vpxor	%ymm7, %ymm12, %ymm13
	vpxor	%ymm14, %ymm11, %ymm11
	vmovq	%rsi, %xmm7
	xorq	%r11, %r14
	vpxor	%ymm13, %ymm15, %ymm0
	vpunpckhqdq	%ymm11, %ymm13, %ymm2
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	xorq	%r14, %rdi
	vpunpcklqdq	%ymm0, %ymm14, %ymm15
	vpunpckhqdq	%ymm0, %ymm14, %ymm1
	vmovq	%r14, %xmm12
	vperm2i128	$32, %ymm2, %ymm1, %ymm9
	vperm2i128	$32, %ymm10, %ymm15, %ymm8
	vpinsrq	$1, %rcx, %xmm12, %xmm13
	vpinsrq	$1, %rdi, %xmm7, %xmm14
	vpxor	%ymm9, %ymm8, %ymm11
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vinserti128	$0x1, %xmm13, %ymm14, %ymm7
	vperm2i128	$49, %ymm2, %ymm1, %ymm1
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
	vpand	%ymm0, %ymm10, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm0, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm0
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm10, %ymm0, %ymm15
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm2, %ymm2
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vextracti128	$0x1, %ymm2, %xmm9
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm2, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vpextrq	$1, %xmm9, %r11
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm0
	vpand	.LC1(%rip), %ymm13, %ymm14
	vpand	.LC2(%rip), %ymm13, %ymm12
	leaq	(%rsi,%rsi), %rdx
	shrq	$3, %rsi
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	andq	%r9, %rsi
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vmovq	%xmm9, %rcx
	andq	%r10, %rdx
	vpand	.LC3(%rip), %ymm15, %ymm8
	leaq	0(,%r11,8), %rdi
	shrq	%r11
	orq	%rsi, %rdx
	andq	%r12, %r11
	vpsrlq	$3, %ymm14, %ymm7
	vmovq	%xmm2, %r8
	andq	%r13, %rdi
	vpsllq	$1, %ymm12, %ymm10
	orq	%r11, %rdi
	vpand	%ymm5, %ymm0, %ymm9
	xorq	%rdx, %r8
	vpand	.LC4(%rip), %ymm15, %ymm15
	vpor	%ymm7, %ymm10, %ymm13
	vpand	%ymm6, %ymm0, %ymm0
	movabsq	$3689348814741910323, %r11
	vpsrlq	$2, %ymm8, %ymm2
	vpsrlq	$1, %ymm9, %ymm14
	vpxor	%ymm13, %ymm1, %ymm1
	vpsllq	$2, %ymm15, %ymm11
	vpsllq	$3, %ymm0, %ymm7
	vextracti128	$0x1, %ymm1, %xmm8
	leaq	0(,%rcx,4), %r14
	vpor	%ymm2, %ymm11, %ymm10
	shrq	$2, %rcx
	vpor	%ymm14, %ymm7, %ymm12
	andq	%r11, %rcx
	vmovq	%r8, %xmm2
	vpermq	$144, %ymm1, %ymm9
	andq	%rbx, %r14
	vpbroadcastq	%xmm2, %ymm11
	orq	%rcx, %r14
	vpxor	%ymm12, %ymm10, %ymm15
	xorq	%rdi, %r14
	vpblendd	$3, %ymm11, %ymm9, %ymm14
	vpextrq	$1, %xmm8, %rcx
	vpand	%ymm3, %ymm11, %ymm7
	xorq	%rdi, %rcx
	vpxor	%ymm15, %ymm13, %ymm13
	xorq	%r14, %rdx
	vpxor	%ymm14, %ymm12, %ymm0
	vextracti128	$0x1, %ymm13, %xmm10
	vmovq	%rdx, %xmm8
	movq	%r8, %rdx
	vpxor	%ymm0, %ymm7, %ymm12
	vmovq	%rcx, %xmm7
	vpbroadcastq	%xmm8, %ymm2
	vpextrq	$1, %xmm10, %rdi
	vextracti128	$0x1, %ymm12, %xmm14
	vpbroadcastq	%xmm7, %ymm10
	vpermq	$144, %ymm12, %ymm8
	vpermq	$144, %ymm13, %ymm13
	vpand	%ymm3, %ymm2, %ymm9
	vpblendd	$3, %ymm2, %ymm13, %ymm11
	vpextrq	$1, %xmm14, %rsi
	vpblendd	$3, %ymm10, %ymm8, %ymm2
	vpxor	%ymm11, %ymm9, %ymm0
	vpand	%ymm3, %ymm10, %ymm3
	vpxor	%ymm2, %ymm15, %ymm15
	xorq	%r14, %rsi
	vpxor	%ymm15, %ymm3, %ymm13
	xorq	%rdi, %rdx
	vpxor	%ymm0, %ymm1, %ymm1
	xorq	%rsi, %rdi
	vpxor	%ymm13, %ymm0, %ymm11
	vpand	.LC3(%rip), %ymm13, %ymm7
	vpxor	%ymm1, %ymm12, %ymm12
	xorq	%rdx, %rcx
	vpand	.LC4(%rip), %ymm13, %ymm8
	vpand	.LC1(%rip), %ymm12, %ymm15
	leaq	0(,%rdi,8), %r8
	shrq	%rdi
	vpand	%ymm5, %ymm11, %ymm5
	vpand	%ymm6, %ymm11, %ymm6
	andq	%r13, %r8
	andq	%r12, %rdi
	vpand	.LC2(%rip), %ymm12, %ymm12
	leaq	0(,%rsi,4), %r13
	shrq	$2, %rsi
	orq	%rdi, %r8
	andq	%r11, %rsi
	vpsrlq	$1, %ymm5, %ymm9
	leaq	(%rcx,%rcx), %r12
	andq	%rbx, %r13
	vpsllq	$3, %ymm6, %ymm0
	vpsrlq	$2, %ymm7, %ymm10
	orq	%rsi, %r13
	andq	%r10, %r12
	vpxor	.LC16(%rip), %ymm1, %ymm1
	vpsllq	$2, %ymm8, %ymm2
	shrq	$3, %rcx
	vpor	%ymm9, %ymm0, %ymm14
	vpsrlq	$3, %ymm15, %ymm13
	vpsllq	$1, %ymm12, %ymm11
	vpor	%ymm10, %ymm2, %ymm3
	andq	%r9, %rcx
	vpor	%ymm13, %ymm11, %ymm9
	vpunpcklqdq	%ymm14, %ymm1, %ymm6
	vmovq	%r13, %xmm10
	orq	%r12, %rcx
	vpunpcklqdq	%ymm9, %ymm3, %ymm5
	vpunpckhqdq	%ymm14, %ymm1, %ymm14
	vpunpckhqdq	%ymm9, %ymm3, %ymm0
	movabsq	$-1152657617789587456, %r13
	vmovq	%rdx, %xmm2
	vperm2i128	$32, %ymm5, %ymm6, %ymm15
	vperm2i128	$32, %ymm0, %ymm14, %ymm3
	movabsq	$1152657617789587455, %r12
	vpinsrq	$1, %rcx, %xmm10, %xmm8
	vpinsrq	$1, %r8, %xmm2, %xmm12
	vperm2i128	$49, %ymm5, %ymm6, %ymm7
	vinserti128	$0x1, %xmm8, %ymm12, %ymm9
	vpxor	%ymm3, %ymm15, %ymm11
	vperm2i128	$49, %ymm0, %ymm14, %ymm13
	vpxor	%ymm9, %ymm7, %ymm1
	vpxor	%ymm9, %ymm15, %ymm2
	vpxor	%ymm4, %ymm11, %ymm4
	vpand	%ymm9, %ymm3, %ymm11
	vpxor	%ymm13, %ymm4, %ymm8
	vpandn	%ymm9, %ymm7, %ymm6
	vpand	%ymm2, %ymm4, %ymm5
	vpxor	%ymm1, %ymm11, %ymm4
	vpor	%ymm7, %ymm15, %ymm14
	vpxor	%ymm5, %ymm6, %ymm10
	vpand	%ymm9, %ymm15, %ymm15
	vpxor	%ymm13, %ymm4, %ymm6
	vpand	%ymm13, %ymm1, %ymm13
	vpor	%ymm2, %ymm8, %ymm1
	vpand	%ymm8, %ymm7, %ymm0
	vpxor	%ymm14, %ymm15, %ymm5
	vpor	%ymm6, %ymm3, %ymm9
	vpxor	%ymm13, %ymm1, %ymm8
	vpxor	%ymm14, %ymm3, %ymm3
	vpxor	%ymm9, %ymm5, %ymm11
	vpxor	%ymm6, %ymm0, %ymm6
	vpxor	%ymm13, %ymm3, %ymm14
	vpxor	%ymm7, %ymm8, %ymm2
	vpxor	%ymm0, %ymm10, %ymm12
	vmovdqa	.LC0(%rip), %ymm0
	vpunpcklqdq	%ymm14, %ymm2, %ymm7
	vpunpckhqdq	%ymm14, %ymm2, %ymm15
	vpunpcklqdq	%ymm11, %ymm12, %ymm10
	vpextrq	$1, %xmm6, %rbx
	vpunpckhqdq	%ymm11, %ymm12, %ymm12
	vmovq	%xmm6, %r10
	vperm2i128	$32, %ymm15, %ymm12, %ymm4
	vperm2i128	$32, %ymm7, %ymm10, %ymm9
	vextracti128	$0x1, %ymm6, %xmm13
	xorq	%rbx, %r10
	vpxor	%ymm4, %ymm9, %ymm8
	vmovq	%r10, %xmm14
	vperm2i128	$49, %ymm15, %ymm12, %ymm5
	vperm2i128	$49, %ymm7, %ymm10, %ymm11
	vpextrq	$1, %xmm13, %r14
	vpbroadcastq	%xmm14, %ymm10
	vextracti128	$0x1, %ymm8, %xmm1
	vpermq	$144, %ymm8, %ymm12
	vmovq	%xmm13, %r9
	xorq	%r14, %r9
	vpblendd	$3, %ymm10, %ymm12, %ymm7
	vpextrq	$1, %xmm1, %r11
	vpxor	%ymm5, %ymm11, %ymm3
	xorq	%r9, %rbx
	vpand	%ymm0, %ymm10, %ymm15
	xorq	%r11, %r14
	vpxor	%ymm7, %ymm5, %ymm9
	vpxor	%ymm3, %ymm4, %ymm2
	vmovq	%rbx, %xmm5
	movabsq	$71777214294589695, %r11
	vpxor	%ymm9, %ymm15, %ymm4
	vmovq	%r14, %xmm12
	vpbroadcastq	%xmm5, %ymm6
	vpbroadcastq	%xmm12, %ymm7
	vpermq	$144, %ymm4, %ymm15
	vpermq	$144, %ymm2, %ymm13
	vpand	%ymm0, %ymm6, %ymm1
	vpblendd	$3, %ymm7, %ymm15, %ymm9
	vextracti128	$0x1, %ymm2, %xmm11
	vpblendd	$3, %ymm6, %ymm13, %ymm2
	vpextrq	$1, %xmm11, %rcx
	vextracti128	$0x1, %ymm4, %xmm10
	vpxor	%ymm2, %ymm1, %ymm14
	vpand	%ymm0, %ymm7, %ymm11
	vpxor	%ymm9, %ymm3, %ymm3
	xorq	%rcx, %r10
	vpxor	%ymm3, %ymm11, %ymm5
	vpxor	%ymm14, %ymm8, %ymm8
	vmovq	%r10, %xmm7
	xorq	%r10, %r14
	vpextrq	$1, %xmm10, %rdi
	vpxor	%ymm8, %ymm4, %ymm4
	vpxor	%ymm5, %ymm14, %ymm6
	movabsq	$-4222189076152336, %r10
	vpunpcklqdq	%ymm6, %ymm8, %ymm13
	vpunpcklqdq	%ymm4, %ymm5, %ymm1
	vpunpckhqdq	%ymm6, %ymm8, %ymm14
	xorq	%r9, %rdi
	vpunpckhqdq	%ymm4, %ymm5, %ymm2
	xorq	%rdi, %rcx
	vmovq	%rdi, %xmm10
	movq	%r15, %r9
	vperm2i128	$32, %ymm2, %ymm14, %ymm8
	vperm2i128	$32, %ymm1, %ymm13, %ymm15
	vpinsrq	$1, %r14, %xmm10, %xmm12
	vpinsrq	$1, %rcx, %xmm7, %xmm9
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vpxor	%ymm8, %ymm15, %ymm5
	vinserti128	$0x1, %xmm12, %ymm9, %ymm3
	vperm2i128	$49, %ymm1, %ymm13, %ymm11
	vperm2i128	$49, %ymm2, %ymm14, %ymm2
	vpxor	%ymm4, %ymm5, %ymm6
	vpxor	%ymm3, %ymm15, %ymm14
	vpxor	%ymm3, %ymm11, %ymm12
	vpxor	%ymm2, %ymm6, %ymm5
	vpandn	%ymm3, %ymm11, %ymm1
	vpand	%ymm14, %ymm6, %ymm10
	vpand	%ymm3, %ymm8, %ymm6
	vpxor	%ymm10, %ymm1, %ymm7
	vpor	%ymm11, %ymm15, %ymm13
	vpxor	%ymm12, %ymm6, %ymm1
	vpand	%ymm3, %ymm15, %ymm15
	vpand	%ymm5, %ymm11, %ymm9
	vpxor	%ymm2, %ymm1, %ymm10
	vpand	%ymm2, %ymm12, %ymm2
	vpor	%ymm14, %ymm5, %ymm12
	vpxor	%ymm13, %ymm15, %ymm3
	vpor	%ymm10, %ymm8, %ymm6
	vpxor	%ymm2, %ymm12, %ymm5
	vpxor	%ymm13, %ymm8, %ymm8
	vpxor	%ymm6, %ymm3, %ymm1
	vpxor	%ymm11, %ymm5, %ymm11
	vpxor	%ymm2, %ymm8, %ymm13
	vpxor	%ymm9, %ymm7, %ymm7
	vpxor	%ymm10, %ymm9, %ymm9
	vpunpcklqdq	%ymm1, %ymm7, %ymm14
	vpunpckhqdq	%ymm13, %ymm11, %ymm3
	vpunpckhqdq	%ymm1, %ymm7, %ymm7
	vpunpcklqdq	%ymm13, %ymm11, %ymm15
	vperm2i128	$32, %ymm3, %ymm7, %ymm12
	vextracti128	$0x1, %ymm9, %xmm2
	vpand	.LC8(%rip), %ymm12, %ymm5
	vpand	.LC9(%rip), %ymm12, %ymm13
	vpextrq	$1, %xmm9, %rbx
	vperm2i128	$49, %ymm15, %ymm14, %ymm6
	vperm2i128	$32, %ymm15, %ymm14, %ymm1
	vperm2i128	$49, %ymm3, %ymm7, %ymm11
	movq	%rbx, %rdx
	vpextrq	$1, %xmm2, %rcx
	vpsrlq	$12, %ymm5, %ymm8
	shrq	$12, %rbx
	vpsllq	$4, %ymm13, %ymm14
	andq	%r15, %rbx
	salq	$4, %rdx
	vpor	%ymm8, %ymm14, %ymm15
	vmovq	%r15, %xmm8
	movq	%rcx, %r15
	andq	%r10, %rdx
	vmovq	%xmm2, %r8
	salq	$12, %r15
	orq	%rbx, %rdx
	vpxor	%ymm15, %ymm1, %ymm1
	vpshufb	.LXRH_bswap16(%rip), %ymm6, %ymm7
	vmovq	%r10, %xmm6
	shrq	$4, %rcx
	andq	%r13, %r15
	movq	%r8, %rdi
	vpbroadcastq	%xmm6, %ymm5
	andq	%r12, %rcx
	vpbroadcastq	%xmm8, %ymm6
	movabsq	$-71777214294589696, %rbx
	vmovq	%xmm9, %r14
	orq	%rcx, %r15
	vpand	%ymm5, %ymm11, %ymm9
	salq	$8, %rdi
	xorq	%rdx, %r14
	vpand	%ymm6, %ymm11, %ymm11
	shrq	$8, %r8
	andq	%rbx, %rdi
	vextracti128	$0x1, %ymm1, %xmm3
	andq	%r11, %r8
	vpsrlq	$4, %ymm9, %ymm2
	vpsllq	$12, %ymm11, %ymm13
	orq	%rdi, %r8
	vmovq	%r14, %xmm10
	vpor	%ymm2, %ymm13, %ymm14
	vpermq	$144, %ymm1, %ymm2
	xorq	%r15, %r8
	vpbroadcastq	%xmm10, %ymm9
	vpxor	%ymm14, %ymm7, %ymm12
	vpextrq	$1, %xmm3, %rsi
	xorq	%r8, %rdx
	vpand	%ymm0, %ymm9, %ymm11
	vpxor	%ymm12, %ymm15, %ymm15
	vmovq	%rdx, %xmm3
	xorq	%r15, %rsi
	vpblendd	$3, %ymm9, %ymm2, %ymm8
	vextracti128	$0x1, %ymm15, %xmm7
	vpbroadcastq	%xmm3, %ymm10
	vpxor	%ymm8, %ymm14, %ymm13
	vpextrq	$1, %xmm7, %rcx
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm13, %ymm11, %ymm14
	vmovq	%rsi, %xmm13
	vpand	%ymm0, %ymm10, %ymm2
	xorq	%rcx, %r14
	vpbroadcastq	%xmm13, %ymm7
	vpermq	$144, %ymm14, %ymm3
	vextracti128	$0x1, %ymm14, %xmm8
	xorq	%r14, %rsi
	vpblendd	$3, %ymm10, %ymm15, %ymm9
	vpblendd	$3, %ymm7, %ymm3, %ymm10
	vpand	%ymm0, %ymm7, %ymm15
	movq	%rsi, %rdi
	salq	$4, %rdi
	vpxor	%ymm10, %ymm12, %ymm12
	vpxor	%ymm9, %ymm2, %ymm11
	shrq	$12, %rsi
	vpxor	%ymm12, %ymm15, %ymm9
	vpextrq	$1, %xmm8, %rdx
	andq	%r9, %rsi
	andq	%r10, %rdi
	vpxor	%ymm9, %ymm11, %ymm2
	xorq	%r8, %rdx
	vpxor	%ymm11, %ymm1, %ymm1
	orq	%rsi, %rdi
	vpand	%ymm5, %ymm2, %ymm11
	xorq	%rdx, %rcx
	vpand	%ymm6, %ymm2, %ymm13
	movq	%rdx, %r15
	vpxor	%ymm1, %ymm14, %ymm14
	vpsrlq	$4, %ymm11, %ymm8
	movq	%rcx, %r8
	salq	$8, %r15
	vpand	.LC8(%rip), %ymm14, %ymm11
	shrq	$8, %rdx
	andq	%rbx, %r15
	vpsllq	$12, %ymm13, %ymm7
	andq	%r11, %rdx
	salq	$12, %rcx
	vpor	%ymm8, %ymm7, %ymm15
	shrq	$4, %r8
	orq	%rdx, %r15
	vpshufb	.LXRH_bswap16(%rip), %ymm9, %ymm2
	vpsrlq	$12, %ymm11, %ymm8
	andq	%r12, %r8
	andq	%r13, %rcx
	vpand	.LC9(%rip), %ymm14, %ymm14
	vpxor	.LC17(%rip), %ymm1, %ymm1
	orq	%r8, %rcx
	vpsllq	$4, %ymm14, %ymm13
	vpunpcklqdq	%ymm15, %ymm1, %ymm10
	vmovq	%r14, %xmm14
	vpor	%ymm8, %ymm13, %ymm7
	vpunpckhqdq	%ymm15, %ymm1, %ymm15
	vpinsrq	$1, %rcx, %xmm14, %xmm13
	vpunpcklqdq	%ymm7, %ymm2, %ymm3
	vpunpckhqdq	%ymm7, %ymm2, %ymm12
	vmovq	%r15, %xmm2
	vperm2i128	$32, %ymm3, %ymm10, %ymm8
	vperm2i128	$32, %ymm12, %ymm15, %ymm9
	vpinsrq	$1, %rdi, %xmm2, %xmm11
	vinserti128	$0x1, %xmm11, %ymm13, %ymm7
	vperm2i128	$49, %ymm12, %ymm15, %ymm1
	vpxor	%ymm9, %ymm8, %ymm15
	vperm2i128	$49, %ymm3, %ymm10, %ymm10
	vpxor	%ymm4, %ymm15, %ymm11
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm1, %ymm11, %ymm2
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm13, %ymm11, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpand	%ymm2, %ymm10, %ymm3
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rsi
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %r14
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	xorq	%rsi, %r14
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%r14, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vpextrq	$1, %xmm9, %rdx
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r15
	xorq	%rdx, %r15
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %rcx
	vpand	%ymm0, %ymm1, %ymm3
	vpxor	%ymm15, %ymm2, %ymm2
	vextracti128	$0x1, %ymm12, %xmm9
	xorq	%r15, %rsi
	vpxor	%ymm2, %ymm3, %ymm11
	vmovq	%rsi, %xmm10
	vpermq	$144, %ymm12, %ymm12
	xorq	%rcx, %rdx
	vmovq	%rdx, %xmm2
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %r8
	vpermq	$144, %ymm11, %ymm10
	vpbroadcastq	%xmm2, %ymm9
	vpand	%ymm0, %ymm8, %ymm13
	xorq	%r8, %r14
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	vpand	%ymm0, %ymm9, %ymm12
	xorq	%r14, %rdx
	vpxor	%ymm1, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm7, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rdi
	xorq	%r15, %rdi
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm1
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	vpunpckhqdq	%ymm1, %ymm14, %ymm2
	xorq	%rdi, %r8
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	vmovq	%rdi, %xmm8
	vmovq	%r14, %xmm13
	vperm2i128	$32, %ymm3, %ymm2, %ymm9
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	vpinsrq	$1, %rdx, %xmm8, %xmm12
	vpinsrq	$1, %r8, %xmm13, %xmm14
	vperm2i128	$49, %ymm3, %ymm2, %ymm2
	vpxor	%ymm9, %ymm7, %ymm11
	vinserti128	$0x1, %xmm12, %ymm14, %ymm3
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm2, %ymm11, %ymm1
	vpandn	%ymm3, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm9, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm1, %ymm10, %ymm8
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm2, %ymm11, %ymm11
	vpand	%ymm2, %ymm13, %ymm2
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm3
	vpxor	%ymm2, %ymm13, %ymm1
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm2, %ymm9, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vpxor	%ymm10, %ymm1, %ymm10
	vpxor	%ymm8, %ymm12, %ymm12
	vpunpcklqdq	%ymm14, %ymm10, %ymm3
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm7
	vperm2i128	$32, %ymm7, %ymm12, %ymm10
	vperm2i128	$49, %ymm3, %ymm15, %ymm2
	vperm2i128	$49, %ymm7, %ymm12, %ymm1
	vperm2i128	$32, %ymm3, %ymm15, %ymm13
	vpextrq	$1, %xmm8, %rsi
	vmovq	%xmm8, %r15
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm12
	vextracti128	$0x1, %ymm8, %xmm9
	rorx	$48, %rsi, %rcx
	vpshufd	$177, %ymm2, %ymm8
	xorq	%rcx, %r15
	vpshufb	.LXRH_rot48(%rip), %ymm1, %ymm1
	vpxor	%ymm12, %ymm13, %ymm13
	vmovq	%r15, %xmm15
	vpxor	%ymm1, %ymm8, %ymm11
	vpextrq	$1, %xmm9, %rdx
	vextracti128	$0x1, %ymm13, %xmm14
	vpermq	$144, %ymm13, %ymm3
	vmovq	%xmm9, %r14
	vpxor	%ymm11, %ymm12, %ymm9
	rorx	$16, %rdx, %r8
	vpbroadcastq	%xmm15, %ymm12
	vpextrq	$1, %xmm14, %rdx
	rorx	$32, %r14, %r14
	xorq	%r8, %r14
	vpblendd	$3, %ymm12, %ymm3, %ymm10
	vpand	%ymm0, %ymm12, %ymm7
	xorq	%r8, %rdx
	xorq	%r14, %rcx
	vpxor	%ymm10, %ymm1, %ymm2
	vmovq	%rcx, %xmm8
	vextracti128	$0x1, %ymm9, %xmm1
	vpxor	%ymm2, %ymm7, %ymm14
	vmovq	%rdx, %xmm2
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm1, %rsi
	vpbroadcastq	%xmm2, %ymm8
	vpermq	$144, %ymm14, %ymm1
	vpermq	$144, %ymm9, %ymm9
	vpand	%ymm0, %ymm15, %ymm3
	vextracti128	$0x1, %ymm14, %xmm7
	xorq	%rsi, %r15
	vpblendd	$3, %ymm15, %ymm9, %ymm12
	vpblendd	$3, %ymm8, %ymm1, %ymm15
	vpand	%ymm0, %ymm8, %ymm9
	xorq	%r15, %rdx
	vpxor	%ymm12, %ymm3, %ymm10
	vpxor	%ymm15, %ymm11, %ymm11
	vpextrq	$1, %xmm7, %rdi
	vpxor	%ymm11, %ymm9, %ymm12
	vpxor	%ymm10, %ymm13, %ymm13
	xorq	%r14, %rdi
	rorq	$48, %rdx
	vpxor	%ymm13, %ymm14, %ymm14
	vpxor	%ymm12, %ymm10, %ymm10
	xorq	%rdi, %rsi
	rorx	$32, %rdi, %r8
	vpshufb	.LXRH_rot48(%rip), %ymm10, %ymm2
	rorq	$16, %rsi
	vpxor	.LC18(%rip), %ymm13, %ymm13
	vpshufd	$177, %ymm12, %ymm15
	vpshufb	.LXRH_rot16(%rip), %ymm14, %ymm12
	vpunpcklqdq	%ymm2, %ymm13, %ymm14
	vpunpckhqdq	%ymm2, %ymm13, %ymm7
	vpunpckhqdq	%ymm12, %ymm15, %ymm3
	vpunpcklqdq	%ymm12, %ymm15, %ymm10
	vmovq	%r8, %xmm2
	vmovq	%r15, %xmm11
	vperm2i128	$32, %ymm10, %ymm14, %ymm8
	vperm2i128	$32, %ymm3, %ymm7, %ymm9
	vpinsrq	$1, %rdx, %xmm2, %xmm15
	vpinsrq	$1, %rsi, %xmm11, %xmm12
	vperm2i128	$49, %ymm3, %ymm7, %ymm1
	vinserti128	$0x1, %xmm15, %ymm12, %ymm7
	vpxor	%ymm9, %ymm8, %ymm3
	vperm2i128	$49, %ymm10, %ymm14, %ymm10
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm3, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm1, %ymm11, %ymm2
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm2, %ymm10, %ymm3
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm7, %ymm8, %ymm8
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %r15
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rsi
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	xorq	%r15, %rsi
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpextrq	$1, %xmm9, %rdx
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	vpextrq	$1, %xmm10, %rcx
	xorq	%rdx, %rdi
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpand	%ymm0, %ymm1, %ymm3
	xorq	%rcx, %rdx
	xorq	%rdi, %r15
	vpxor	%ymm15, %ymm2, %ymm2
	vextracti128	$0x1, %ymm12, %xmm9
	movabsq	$-1229782938247303442, %rcx
	vpxor	%ymm2, %ymm3, %ymm11
	vmovq	%r15, %xmm10
	vmovq	%rdx, %xmm2
	movabsq	$-8608480567731124088, %r15
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %r8
	vpbroadcastq	%xmm2, %ymm10
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm0, %ymm8, %ymm13
	xorq	%r8, %rsi
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpblendd	$3, %ymm10, %ymm9, %ymm8
	vpand	%ymm0, %ymm10, %ymm12
	xorq	%rsi, %rdx
	vpxor	%ymm1, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm7, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r14
	xorq	%rdi, %r14
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm1
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm13, %ymm2
	vpunpckhqdq	%ymm1, %ymm14, %ymm3
	xorq	%r14, %r8
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	vmovq	%r14, %xmm9
	vmovq	%rsi, %xmm13
	movabsq	$1229782938247303441, %r14
	vperm2i128	$32, %ymm2, %ymm3, %ymm8
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	vpinsrq	$1, %rdx, %xmm9, %xmm12
	vpinsrq	$1, %r8, %xmm13, %xmm14
	vperm2i128	$49, %ymm2, %ymm3, %ymm1
	vpxor	%ymm8, %ymm7, %ymm11
	vinserti128	$0x1, %xmm12, %ymm14, %ymm3
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm1, %ymm11, %ymm9
	vpandn	%ymm3, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm8, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm13
	vpand	%ymm9, %ymm10, %ymm2
	vpxor	%ymm1, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm1
	vpxor	%ymm14, %ymm7, %ymm7
	vpor	%ymm11, %ymm8, %ymm3
	vpxor	%ymm13, %ymm1, %ymm9
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vmovq	%r15, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vperm2i128	$49, %ymm12, %ymm15, %ymm3
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	.LC37(%rip), %ymm12
	vpbroadcastq	%xmm9, %ymm9
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpxor	%ymm11, %ymm2, %ymm2
	vpand	%ymm9, %ymm10, %ymm14
	vpand	%ymm12, %ymm10, %ymm13
	vextracti128	$0x1, %ymm2, %xmm1
	vpsrlq	$3, %ymm14, %ymm15
	vpsllq	$1, %ymm13, %ymm10
	vmovq	%xmm2, %rsi
	vmovq	%xmm1, -8(%rsp)
	vpextrq	$1, %xmm2, %rdi
	vpor	%ymm15, %ymm10, %ymm2
	vpextrq	$1, %xmm1, %rdx
	vpbroadcastq	.LC38(%rip), %ymm10
	leaq	(%rdi,%rdi), %r8
	shrq	$3, %rdi
	vpxor	%ymm2, %ymm8, %ymm8
	vpand	%ymm10, %ymm3, %ymm11
	andq	%r14, %rdi
	andq	%rcx, %r8
	movabsq	$3689348814741910323, %r14
	vpsrlq	$2, %ymm11, %ymm15
	orq	%rdi, %r8
	leaq	0(,%rdx,8), %rdi
	shrq	%rdx
	vpbroadcastq	.LC39(%rip), %ymm11
	andq	%r15, %rdi
	vmovq	%r14, %xmm1
	xorq	%r8, %rsi
	movabsq	$8608480567731124087, %rcx
	vpbroadcastq	%xmm1, %ymm13
	vpand	%ymm11, %ymm7, %ymm14
	andq	%rcx, %rdx
	vpand	%ymm13, %ymm3, %ymm3
	movabsq	$-3689348814741910324, %rcx
	vpsrlq	$1, %ymm14, %ymm1
	vpsllq	$2, %ymm3, %ymm3
	orq	%rdi, %rdx
	vpbroadcastq	.LC40(%rip), %ymm14
	movq	-8(%rsp), %rdi
	vpor	%ymm15, %ymm3, %ymm15
	vpand	%ymm14, %ymm7, %ymm7
	leaq	0(,%rdi,4), %rdi
	vpsllq	$3, %ymm7, %ymm7
	andq	%rcx, %rdi
	movq	-8(%rsp), %rcx
	vpor	%ymm1, %ymm7, %ymm7
	vmovq	%rsi, %xmm1
	vpxor	%ymm7, %ymm15, %ymm3
	shrq	$2, %rcx
	vpxor	%ymm3, %ymm2, %ymm15
	vextracti128	$0x1, %ymm8, %xmm2
	andq	%r14, %rcx
	orq	%rcx, %rdi
	vpextrq	$1, %xmm2, %rcx
	vpbroadcastq	%xmm1, %ymm2
	vpermq	$144, %ymm8, %ymm1
	xorq	%rdx, %rdi
	xorq	%rcx, %rdx
	vpblendd	$3, %ymm2, %ymm1, %ymm1
	xorq	%rdi, %r8
	vpand	%ymm0, %ymm2, %ymm2
	vpxor	%ymm1, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm1
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm2, %ymm2
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm1, %rcx
	vpbroadcastq	%xmm7, %ymm1
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm1, %ymm15, %ymm7
	vextracti128	$0x1, %ymm2, %xmm15
	vpand	%ymm0, %ymm1, %ymm1
	vpxor	%ymm7, %ymm1, %ymm1
	vpextrq	$1, %xmm15, %r8
	vmovq	%rdx, %xmm7
	xorq	%rsi, %rdx
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm2, %ymm15
	vpxor	%ymm1, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm0, %ymm7, %ymm7
	vpxor	%ymm15, %ymm3, %ymm3
	leaq	0(,%rcx,8), %r8
	shrq	%rcx
	vpxor	%ymm8, %ymm2, %ymm2
	vpxor	%ymm3, %ymm7, %ymm15
	vpand	%ymm9, %ymm2, %ymm9
	vpand	%ymm12, %ymm2, %ymm12
	andq	%r15, %r8
	vpxor	%ymm15, %ymm1, %ymm1
	vpand	%ymm10, %ymm15, %ymm10
	vpand	%ymm13, %ymm15, %ymm13
	movabsq	$8608480567731124087, %r15
	vpand	%ymm11, %ymm1, %ymm11
	vpsllq	$2, %ymm13, %ymm15
	vpand	%ymm14, %ymm1, %ymm14
	andq	%r15, %rcx
	orq	%r8, %rcx
	leaq	0(,%rdi,4), %r8
	movabsq	$-3689348814741910324, %r15
	shrq	$2, %rdi
	vpsrlq	$1, %ymm11, %ymm7
	vpsllq	$3, %ymm14, %ymm3
	andq	%r14, %rdi
	andq	%r15, %r8
	vpor	%ymm7, %ymm3, %ymm1
	vpsrlq	$2, %ymm10, %ymm11
	leaq	(%rdx,%rdx), %r14
	orq	%rdi, %r8
	vpxor	.LC19(%rip), %ymm8, %ymm8
	vpsrlq	$3, %ymm9, %ymm7
	shrq	$3, %rdx
	movabsq	$1229782938247303441, %r15
	vpsllq	$1, %ymm12, %ymm2
	vpor	%ymm11, %ymm15, %ymm14
	vmovq	%r8, %xmm15
	andq	%r15, %rdx
	movabsq	$-1229782938247303442, %rdi
	vpor	%ymm7, %ymm2, %ymm3
	vpunpcklqdq	%ymm1, %ymm8, %ymm10
	vpunpcklqdq	%ymm3, %ymm14, %ymm11
	vpunpckhqdq	%ymm3, %ymm14, %ymm13
	vpunpckhqdq	%ymm1, %ymm8, %ymm1
	andq	%rdi, %r14
	vmovq	%rsi, %xmm7
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm1, %ymm9
	orq	%rdx, %r14
	vpinsrq	$1, %r14, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm7, %xmm12
	vpxor	%ymm9, %ymm8, %ymm2
	vinserti128	$0x1, %xmm14, %ymm12, %ymm7
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vperm2i128	$49, %ymm13, %ymm1, %ymm1
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm2, %ymm11
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm1, %ymm11, %ymm2
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm2, %ymm10, %ymm3
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm7, %ymm8, %ymm8
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %rcx
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %rsi
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	xorq	%rcx, %rsi
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpextrq	$1, %xmm9, %rdx
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r14
	vpextrq	$1, %xmm10, %r8
	xorq	%rdx, %r14
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpand	%ymm0, %ymm1, %ymm3
	xorq	%r8, %rdx
	xorq	%r14, %rcx
	vpxor	%ymm15, %ymm2, %ymm2
	vextracti128	$0x1, %ymm12, %xmm9
	vpxor	%ymm2, %ymm3, %ymm11
	vmovq	%rcx, %xmm10
	vmovq	%rdx, %xmm2
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %rdi
	vpermq	$144, %ymm11, %ymm10
	vpbroadcastq	%xmm2, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm0, %ymm8, %ymm13
	xorq	%rdi, %rsi
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	vpand	%ymm0, %ymm9, %ymm12
	xorq	%rsi, %rdx
	vpxor	%ymm1, %ymm13, %ymm15
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm7, %ymm12, %ymm13
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rcx
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm1
	vmovq	%rsi, %xmm7
	xorq	%r14, %rcx
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	vpunpckhqdq	%ymm1, %ymm14, %ymm2
	xorq	%rcx, %rdi
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	vmovq	%rcx, %xmm12
	vperm2i128	$32, %ymm3, %ymm2, %ymm9
	vperm2i128	$32, %ymm10, %ymm15, %ymm8
	vpinsrq	$1, %rdx, %xmm12, %xmm13
	vpinsrq	$1, %rdi, %xmm7, %xmm14
	vinserti128	$0x1, %xmm13, %ymm14, %ymm7
	vpxor	%ymm9, %ymm8, %ymm11
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vperm2i128	$49, %ymm3, %ymm2, %ymm2
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm2, %ymm11, %ymm1
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm13, %ymm11, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpand	%ymm1, %ymm10, %ymm3
	vpxor	%ymm2, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm2, %ymm13, %ymm2
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm2, %ymm13, %ymm1
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm1, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm2, %ymm9, %ymm14
	vpextrq	$1, %xmm3, %rsi
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	movq	%rsi, %rdx
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm1
	salq	$4, %rdx
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	shrq	$12, %rsi
	andq	%r10, %rdx
	vperm2i128	$32, %ymm10, %ymm7, %ymm2
	vpand	.LC8(%rip), %ymm13, %ymm14
	andq	%r9, %rsi
	vpand	.LC9(%rip), %ymm13, %ymm12
	vpextrq	$1, %xmm9, %r14
	orq	%rsi, %rdx
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm11
	vpsrlq	$12, %ymm14, %ymm7
	vmovq	%xmm3, %r8
	movq	%r14, %rsi
	salq	$12, %rsi
	vpsllq	$4, %ymm12, %ymm10
	xorq	%rdx, %r8
	shrq	$4, %r14
	vpor	%ymm7, %ymm10, %ymm13
	andq	%r12, %r14
	vmovq	%xmm9, %rdi
	andq	%r13, %rsi
	vpand	%ymm5, %ymm1, %ymm9
	vpxor	%ymm13, %ymm2, %ymm2
	vpand	%ymm6, %ymm1, %ymm1
	orq	%r14, %rsi
	vmovq	%r8, %xmm15
	movq	%rdi, %r14
	vmovdqa	%ymm11, %ymm10
	vpsrlq	$4, %ymm9, %ymm14
	vpermq	$144, %ymm2, %ymm9
	salq	$8, %r14
	vpbroadcastq	%xmm15, %ymm11
	vpsllq	$12, %ymm1, %ymm7
	shrq	$8, %rdi
	andq	%rbx, %r14
	vpor	%ymm14, %ymm7, %ymm12
	vpand	%ymm0, %ymm11, %ymm7
	vextracti128	$0x1, %ymm2, %xmm3
	andq	%r11, %rdi
	vpblendd	$3, %ymm11, %ymm9, %ymm14
	vpxor	%ymm12, %ymm10, %ymm8
	orq	%rdi, %r14
	vpxor	%ymm14, %ymm12, %ymm1
	xorq	%rsi, %r14
	vpxor	%ymm8, %ymm13, %ymm13
	vpxor	%ymm1, %ymm7, %ymm12
	vpextrq	$1, %xmm3, %rcx
	xorq	%r14, %rdx
	vextracti128	$0x1, %ymm12, %xmm7
	xorq	%rsi, %rcx
	vextracti128	$0x1, %ymm13, %xmm10
	vpextrq	$1, %xmm7, %rsi
	vpextrq	$1, %xmm10, %rdi
	vmovq	%rdx, %xmm3
	movq	%r8, %rdx
	xorq	%r14, %rsi
	vmovq	%rcx, %xmm1
	vpbroadcastq	%xmm3, %ymm15
	xorq	%rdi, %rdx
	vpbroadcastq	%xmm1, %ymm10
	xorq	%rsi, %rdi
	vpermq	$144, %ymm12, %ymm3
	xorq	%rdx, %rcx
	vpermq	$144, %ymm13, %ymm13
	vpand	%ymm0, %ymm15, %ymm9
	movq	%rdi, %r8
	shrq	$4, %rdi
	vpblendd	$3, %ymm15, %ymm13, %ymm11
	vpblendd	$3, %ymm10, %ymm3, %ymm15
	vpand	%ymm0, %ymm10, %ymm13
	andq	%r12, %rdi
	vpxor	%ymm11, %ymm9, %ymm14
	vpxor	%ymm15, %ymm8, %ymm8
	salq	$12, %r8
	movq	%rcx, %r12
	vpxor	%ymm8, %ymm13, %ymm11
	vpxor	%ymm14, %ymm2, %ymm2
	andq	%r13, %r8
	movq	%rsi, %r13
	vpxor	%ymm11, %ymm14, %ymm9
	vpxor	%ymm2, %ymm12, %ymm12
	salq	$8, %r13
	orq	%rdi, %r8
	shrq	$8, %rsi
	vpand	%ymm5, %ymm9, %ymm5
	vpand	%ymm6, %ymm9, %ymm6
	andq	%rbx, %r13
	salq	$4, %r12
	vpsrlq	$4, %ymm5, %ymm14
	andq	%r11, %rsi
	vpshufb	.LXRH_bswap16(%rip), %ymm11, %ymm8
	vpand	.LC8(%rip), %ymm12, %ymm11
	orq	%rsi, %r13
	andq	%r10, %r12
	vpsllq	$12, %ymm6, %ymm7
	shrq	$12, %rcx
	vpand	.LC9(%rip), %ymm12, %ymm12
	vpxor	.LC20(%rip), %ymm2, %ymm2
	vpor	%ymm14, %ymm7, %ymm1
	andq	%r9, %rcx
	vpsrlq	$12, %ymm11, %ymm9
	vpsllq	$4, %ymm12, %ymm5
	orq	%r12, %rcx
	vpor	%ymm9, %ymm5, %ymm14
	vpunpcklqdq	%ymm1, %ymm2, %ymm10
	vpunpckhqdq	%ymm1, %ymm2, %ymm3
	vpunpckhqdq	%ymm14, %ymm8, %ymm7
	vpunpcklqdq	%ymm14, %ymm8, %ymm1
	vmovq	%r13, %xmm11
	vmovq	%rdx, %xmm12
	vperm2i128	$32, %ymm1, %ymm10, %ymm6
	vperm2i128	$32, %ymm7, %ymm3, %ymm15
	vpinsrq	$1, %rcx, %xmm11, %xmm9
	vpinsrq	$1, %r8, %xmm12, %xmm5
	vperm2i128	$49, %ymm1, %ymm10, %ymm13
	vinserti128	$0x1, %xmm9, %ymm5, %ymm14
	vperm2i128	$49, %ymm7, %ymm3, %ymm8
	vpxor	%ymm15, %ymm6, %ymm3
	vpxor	%ymm14, %ymm13, %ymm2
	vpxor	%ymm4, %ymm3, %ymm7
	vpxor	%ymm14, %ymm6, %ymm4
	vpand	%ymm14, %ymm15, %ymm3
	vpxor	%ymm8, %ymm7, %ymm11
	vpand	%ymm4, %ymm7, %ymm12
	vpxor	%ymm2, %ymm3, %ymm7
	vpor	%ymm13, %ymm6, %ymm10
	vpandn	%ymm14, %ymm13, %ymm9
	vpxor	%ymm8, %ymm7, %ymm7
	vpand	%ymm14, %ymm6, %ymm6
	vpand	%ymm8, %ymm2, %ymm8
	vpor	%ymm4, %ymm11, %ymm2
	vpand	%ymm11, %ymm13, %ymm1
	vpxor	%ymm10, %ymm6, %ymm14
	vpxor	%ymm8, %ymm2, %ymm11
	vpxor	%ymm12, %ymm9, %ymm5
	vpor	%ymm7, %ymm15, %ymm12
	vpxor	%ymm10, %ymm15, %ymm15
	vpxor	%ymm1, %ymm5, %ymm9
	vpxor	%ymm7, %ymm1, %ymm7
	vpxor	%ymm8, %ymm15, %ymm10
	vpxor	%ymm12, %ymm14, %ymm5
	vpxor	%ymm13, %ymm11, %ymm13
	vpunpcklqdq	%ymm10, %ymm13, %ymm12
	vpunpckhqdq	%ymm10, %ymm13, %ymm3
	vpunpcklqdq	%ymm5, %ymm9, %ymm6
	vpunpckhqdq	%ymm5, %ymm9, %ymm14
	vpextrq	$1, %xmm7, %rbx
	vmovq	%xmm7, %r10
	vperm2i128	$32, %ymm3, %ymm14, %ymm4
	vperm2i128	$32, %ymm12, %ymm6, %ymm9
	vperm2i128	$49, %ymm12, %ymm6, %ymm8
	xorq	%rbx, %r10
	vpxor	%ymm4, %ymm9, %ymm11
	vextracti128	$0x1, %ymm7, %xmm2
	vmovq	%r10, %xmm6
	vperm2i128	$49, %ymm3, %ymm14, %ymm5
	vpextrq	$1, %xmm2, %r11
	vpbroadcastq	%xmm6, %ymm14
	vextracti128	$0x1, %ymm11, %xmm10
	vpermq	$144, %ymm11, %ymm12
	vmovq	%xmm2, %r9
	xorq	%r11, %r9
	vpxor	%ymm5, %ymm8, %ymm13
	vpblendd	$3, %ymm14, %ymm12, %ymm3
	vpextrq	$1, %xmm10, %r14
	vpxor	%ymm13, %ymm4, %ymm15
	vpand	%ymm0, %ymm14, %ymm9
	xorq	%r9, %rbx
	vpxor	%ymm3, %ymm5, %ymm4
	vmovq	%rbx, %xmm7
	vpermq	$144, %ymm15, %ymm2
	xorq	%r14, %r11
	vpxor	%ymm4, %ymm9, %ymm8
	vmovq	%r11, %xmm12
	vpbroadcastq	%xmm7, %ymm1
	vpbroadcastq	%xmm12, %ymm3
	vpermq	$144, %ymm8, %ymm9
	vpand	%ymm0, %ymm1, %ymm10
	vpblendd	$3, %ymm3, %ymm9, %ymm4
	vextracti128	$0x1, %ymm15, %xmm5
	vpblendd	$3, %ymm1, %ymm2, %ymm15
	vpxor	%ymm15, %ymm10, %ymm14
	vextracti128	$0x1, %ymm8, %xmm6
	vpand	%ymm0, %ymm3, %ymm0
	vpxor	%ymm4, %ymm13, %ymm13
	vpxor	%ymm14, %ymm11, %ymm11
	vpextrq	$1, %xmm5, %rcx
	vpextrq	$1, %xmm6, %rdi
	vpxor	%ymm13, %ymm0, %ymm5
	vpxor	%ymm11, %ymm8, %ymm8
	xorq	%rcx, %r10
	xorq	%r9, %rdi
	vpxor	%ymm5, %ymm14, %ymm7
	vpunpcklqdq	%ymm8, %ymm5, %ymm10
	xorq	%r10, %r11
	vpunpcklqdq	%ymm7, %ymm11, %ymm1
	vpunpckhqdq	%ymm7, %ymm11, %ymm15
	vpunpckhqdq	%ymm8, %ymm5, %ymm2
	xorq	%rdi, %rcx
	vmovq	%rdi, %xmm6
	vmovq	%r10, %xmm4
	vperm2i128	$32, %ymm2, %ymm15, %ymm13
	vperm2i128	$32, %ymm10, %ymm1, %ymm14
	vpinsrq	$1, %r11, %xmm6, %xmm12
	vpinsrq	$1, %rcx, %xmm4, %xmm0
	vinserti128	$0x1, %xmm12, %ymm0, %ymm8
	vpcmpeqd	%ymm4, %ymm4, %ymm4
	vpxor	%ymm13, %ymm14, %ymm5
	vperm2i128	$49, %ymm10, %ymm1, %ymm9
	vperm2i128	$49, %ymm2, %ymm15, %ymm3
	vpxor	%ymm4, %ymm5, %ymm7
	vpxor	%ymm8, %ymm14, %ymm11
	vpxor	%ymm8, %ymm9, %ymm15
	vpxor	%ymm3, %ymm7, %ymm2
	vpandn	%ymm8, %ymm9, %ymm6
	vpand	%ymm11, %ymm7, %ymm12
	vpand	%ymm8, %ymm13, %ymm7
	vpxor	%ymm12, %ymm6, %ymm0
	vpxor	%ymm15, %ymm7, %ymm6
	vpor	%ymm9, %ymm14, %ymm10
	vpxor	%ymm3, %ymm6, %ymm12
	vpand	%ymm2, %ymm9, %ymm1
	vpand	%ymm3, %ymm15, %ymm15
	vpand	%ymm8, %ymm14, %ymm14
	vpor	%ymm11, %ymm2, %ymm3
	vpxor	%ymm1, %ymm0, %ymm5
	vpxor	%ymm10, %ymm14, %ymm8
	vpor	%ymm12, %ymm13, %ymm0
	vpxor	%ymm15, %ymm3, %ymm2
	vpxor	%ymm10, %ymm13, %ymm13
	vpxor	%ymm0, %ymm8, %ymm7
	vpxor	%ymm9, %ymm2, %ymm11
	vpxor	%ymm15, %ymm13, %ymm10
	vpunpcklqdq	%ymm7, %ymm5, %ymm14
	vpunpckhqdq	%ymm7, %ymm5, %ymm5
	vpunpckhqdq	%ymm10, %ymm11, %ymm8
	vpxor	%ymm12, %ymm1, %ymm1
	vpunpcklqdq	%ymm10, %ymm11, %ymm9
	vperm2i128	$32, %ymm8, %ymm5, %ymm7
	vpextrq	$1, %xmm1, %rdx
	vmovq	%xmm1, %r13
	vperm2i128	$32, %ymm9, %ymm14, %ymm6
	rorx	$48, %rdx, %r10
	vpshufb	.LXRH_rot16(%rip), %ymm7, %ymm13
	xorq	%r10, %r13
	vperm2i128	$49, %ymm9, %ymm14, %ymm15
	vperm2i128	$49, %ymm8, %ymm5, %ymm3
	vextracti128	$0x1, %ymm1, %xmm0
	vmovq	%r13, %xmm12
	vpxor	%ymm13, %ymm6, %ymm6
	vpbroadcastq	%xmm12, %ymm2
	vmovq	%xmm0, %r8
	vpextrq	$1, %xmm0, %r12
	vpshufd	$177, %ymm15, %ymm7
	vpermq	$144, %ymm6, %ymm0
	rorx	$32, %r8, %r9
	vpshufb	.LXRH_rot48(%rip), %ymm3, %ymm8
	vextracti128	$0x1, %ymm6, %xmm1
	rorx	$16, %r12, %rbx
	xorq	%rbx, %r9
	vpblendd	$3, %ymm2, %ymm0, %ymm11
	vpextrq	$1, %xmm1, %r11
	xorq	%r9, %r10
	vpxor	%ymm8, %ymm7, %ymm15
	vpxor	%ymm11, %ymm8, %ymm10
	vmovq	%r10, %xmm9
	xorq	%rbx, %r11
	vpxor	%ymm15, %ymm13, %ymm3
	vmovq	%r11, %xmm11
	vpbroadcastq	%xmm9, %ymm8
	vmovdqa	.LC0(%rip), %ymm0
	vextracti128	$0x1, %ymm3, %xmm5
	vpermq	$144, %ymm3, %ymm7
	vpand	%ymm0, %ymm2, %ymm13
	vpblendd	$3, %ymm8, %ymm7, %ymm3
	vpand	%ymm0, %ymm8, %ymm1
	vpxor	%ymm10, %ymm13, %ymm14
	vpbroadcastq	%xmm11, %ymm13
	vpextrq	$1, %xmm5, %r14
	vpermq	$144, %ymm14, %ymm10
	vpxor	%ymm3, %ymm1, %ymm12
	vpand	%ymm0, %ymm13, %ymm9
	xorq	%r14, %r13
	vpblendd	$3, %ymm13, %ymm10, %ymm5
	vpxor	%ymm12, %ymm6, %ymm6
	vextracti128	$0x1, %ymm14, %xmm2
	xorq	%r13, %r11
	vpxor	%ymm5, %ymm15, %ymm15
	vpxor	%ymm6, %ymm14, %ymm14
	vpextrq	$1, %xmm2, %rdi
	rorq	$48, %r11
	vpxor	%ymm15, %ymm9, %ymm8
	vpshufb	.LXRH_rot16(%rip), %ymm14, %ymm9
	vpxor	.LC21(%rip), %ymm6, %ymm15
	xorq	%r9, %rdi
	vpxor	%ymm8, %ymm12, %ymm7
	vpshufd	$177, %ymm8, %ymm13
	xorq	%rdi, %r14
	rorx	$32, %rdi, %rsi
	vpshufb	.LXRH_rot48(%rip), %ymm7, %ymm12
	rorq	$16, %r14
	vpunpckhqdq	%ymm12, %ymm15, %ymm6
	vpunpcklqdq	%ymm9, %ymm13, %ymm14
	vpunpckhqdq	%ymm9, %ymm13, %ymm1
	vpunpcklqdq	%ymm12, %ymm15, %ymm8
	vmovq	%r13, %xmm13
	vmovq	%rsi, %xmm12
	vperm2i128	$32, %ymm14, %ymm8, %ymm7
	vperm2i128	$32, %ymm1, %ymm6, %ymm2
	vpinsrq	$1, %r11, %xmm12, %xmm11
	vpinsrq	$1, %r14, %xmm13, %xmm10
	vperm2i128	$49, %ymm14, %ymm8, %ymm15
	vpxor	%ymm2, %ymm7, %ymm5
	vinserti128	$0x1, %xmm11, %ymm10, %ymm8
	vpxor	%ymm4, %ymm5, %ymm9
	vpxor	%ymm8, %ymm7, %ymm12
	vperm2i128	$49, %ymm1, %ymm6, %ymm3
	vpxor	%ymm8, %ymm15, %ymm14
	vpxor	%ymm3, %ymm9, %ymm6
	vpandn	%ymm8, %ymm15, %ymm13
	vpand	%ymm12, %ymm9, %ymm10
	vpand	%ymm8, %ymm2, %ymm9
	vpor	%ymm15, %ymm7, %ymm11
	vpxor	%ymm10, %ymm13, %ymm5
	vpxor	%ymm14, %ymm9, %ymm13
	vpand	%ymm8, %ymm7, %ymm7
	vpxor	%ymm3, %ymm13, %ymm9
	vpand	%ymm3, %ymm14, %ymm3
	vpor	%ymm12, %ymm6, %ymm14
	vpand	%ymm6, %ymm15, %ymm1
	vpxor	%ymm11, %ymm7, %ymm8
	vpor	%ymm9, %ymm2, %ymm10
	vpxor	%ymm3, %ymm14, %ymm6
	vpxor	%ymm11, %ymm2, %ymm2
	vpxor	%ymm10, %ymm8, %ymm13
	vpxor	%ymm3, %ymm2, %ymm11
	vpxor	%ymm15, %ymm6, %ymm15
	vpxor	%ymm1, %ymm5, %ymm5
	vpxor	%ymm9, %ymm1, %ymm1
	vpunpcklqdq	%ymm11, %ymm15, %ymm8
	vpunpcklqdq	%ymm13, %ymm5, %ymm12
	vpunpckhqdq	%ymm11, %ymm15, %ymm10
	vpunpckhqdq	%ymm13, %ymm5, %ymm5
	vpextrq	$1, %xmm1, %r12
	vmovq	%xmm1, %r13
	vperm2i128	$32, %ymm10, %ymm5, %ymm7
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm10, %ymm5, %ymm15
	vpxor	%ymm7, %ymm13, %ymm11
	vextracti128	$0x1, %ymm1, %xmm3
	xorq	%r12, %r13
	vmovq	%r13, %xmm5
	vperm2i128	$49, %ymm8, %ymm12, %ymm14
	vpermq	$144, %ymm11, %ymm10
	movq	%r13, %r14
	vpbroadcastq	%xmm5, %ymm8
	vpextrq	$1, %xmm3, %rbx
	vextracti128	$0x1, %ymm11, %xmm12
	movabsq	$-1229782938247303442, %r13
	vmovq	%xmm3, %r10
	vpxor	%ymm15, %ymm14, %ymm6
	vpblendd	$3, %ymm8, %ymm10, %ymm13
	xorq	%rbx, %r10
	vpextrq	$1, %xmm12, %r8
	vpxor	%ymm6, %ymm7, %ymm2
	xorq	%r10, %r12
	vpand	%ymm0, %ymm8, %ymm7
	vpxor	%ymm13, %ymm15, %ymm14
	xorq	%r8, %rbx
	vpxor	%ymm14, %ymm7, %ymm15
	vmovq	%r12, %xmm9
	vmovq	%rbx, %xmm13
	movq	%r15, %r12
	vpbroadcastq	%xmm9, %ymm3
	vpbroadcastq	%xmm13, %ymm14
	vextracti128	$0x1, %ymm2, %xmm1
	vpermq	$144, %ymm15, %ymm7
	vpermq	$144, %ymm2, %ymm2
	vpand	%ymm0, %ymm3, %ymm5
	vpblendd	$3, %ymm3, %ymm2, %ymm12
	vpextrq	$1, %xmm1, %r9
	vpblendd	$3, %ymm14, %ymm7, %ymm1
	vpxor	%ymm12, %ymm5, %ymm8
	vpand	%ymm0, %ymm14, %ymm9
	vextracti128	$0x1, %ymm15, %xmm10
	xorq	%r9, %r14
	vpxor	%ymm1, %ymm6, %ymm6
	vpxor	%ymm8, %ymm11, %ymm11
	vmovq	%r14, %xmm7
	xorq	%r14, %rbx
	vpxor	%ymm6, %ymm9, %ymm2
	vpextrq	$1, %xmm10, %r11
	vpxor	%ymm11, %ymm15, %ymm15
	vpxor	%ymm2, %ymm8, %ymm3
	vpunpckhqdq	%ymm15, %ymm2, %ymm13
	vpunpcklqdq	%ymm15, %ymm2, %ymm10
	xorq	%r10, %r11
	vpunpcklqdq	%ymm3, %ymm11, %ymm12
	vpunpckhqdq	%ymm3, %ymm11, %ymm5
	vmovq	%r11, %xmm14
	xorq	%r11, %r9
	vperm2i128	$32, %ymm10, %ymm12, %ymm6
	vperm2i128	$32, %ymm13, %ymm5, %ymm8
	vpinsrq	$1, %rbx, %xmm14, %xmm1
	movabsq	$8608480567731124087, %r11
	vpinsrq	$1, %r9, %xmm7, %xmm9
	vpxor	%ymm8, %ymm6, %ymm11
	vperm2i128	$49, %ymm10, %ymm12, %ymm15
	movabsq	$-8608480567731124088, %rbx
	vinserti128	$0x1, %xmm1, %ymm9, %ymm2
	vpxor	%ymm4, %ymm11, %ymm10
	vperm2i128	$49, %ymm13, %ymm5, %ymm3
	movabsq	$-3689348814741910324, %r10
	vpxor	%ymm2, %ymm6, %ymm14
	vpandn	%ymm2, %ymm15, %ymm1
	vpxor	%ymm2, %ymm15, %ymm12
	movabsq	$3689348814741910323, %r9
	vpand	%ymm14, %ymm10, %ymm7
	vpxor	%ymm3, %ymm10, %ymm13
	vpor	%ymm15, %ymm6, %ymm5
	vpxor	%ymm7, %ymm1, %ymm11
	vpand	%ymm2, %ymm8, %ymm1
	vpand	%ymm13, %ymm15, %ymm9
	vpxor	%ymm12, %ymm1, %ymm7
	vpxor	%ymm9, %ymm11, %ymm10
	vpand	%ymm3, %ymm12, %ymm12
	vpxor	%ymm3, %ymm7, %ymm11
	vpand	%ymm2, %ymm6, %ymm6
	vpor	%ymm14, %ymm13, %ymm3
	vpxor	%ymm5, %ymm6, %ymm1
	vpor	%ymm11, %ymm8, %ymm2
	vpxor	%ymm12, %ymm3, %ymm13
	vpxor	%ymm5, %ymm8, %ymm8
	vpxor	%ymm2, %ymm1, %ymm7
	vpxor	%ymm15, %ymm13, %ymm15
	vpxor	%ymm12, %ymm8, %ymm14
	vpunpcklqdq	%ymm7, %ymm10, %ymm6
	vpxor	%ymm11, %ymm9, %ymm9
	vpunpcklqdq	%ymm14, %ymm15, %ymm5
	vpunpckhqdq	%ymm14, %ymm15, %ymm2
	vpunpckhqdq	%ymm7, %ymm10, %ymm7
	vperm2i128	$49, %ymm5, %ymm6, %ymm12
	vperm2i128	$32, %ymm5, %ymm6, %ymm1
	vmovq	%rbx, %xmm15
	vmovq	%r11, %xmm6
	vperm2i128	$32, %ymm2, %ymm7, %ymm10
	vpbroadcastq	%xmm15, %ymm5
	vpextrq	$1, %xmm9, %rdi
	vpbroadcastq	%xmm6, %ymm6
	vperm2i128	$49, %ymm2, %ymm7, %ymm3
	vextracti128	$0x1, %ymm9, %xmm13
	vpand	.LC5(%rip), %ymm3, %ymm15
	vpand	%ymm5, %ymm10, %ymm8
	vpand	%ymm6, %ymm10, %ymm7
	vpand	.LC6(%rip), %ymm3, %ymm3
	vmovq	%xmm9, %rsi
	leaq	(%rdi,%rdi), %rdx
	shrq	$3, %rdi
	vpextrq	$1, %xmm13, %rcx
	andq	%r15, %rdi
	vpsrlq	$3, %ymm8, %ymm14
	vmovq	%xmm13, %r8
	andq	%r13, %rdx
	vpand	.LC3(%rip), %ymm12, %ymm9
	vpand	.LC4(%rip), %ymm12, %ymm12
	orq	%rdi, %rdx
	vpsllq	$1, %ymm7, %ymm2
	vpsrlq	$1, %ymm15, %ymm8
	leaq	0(,%rcx,8), %r15
	vpor	%ymm14, %ymm2, %ymm10
	vpsrlq	$2, %ymm9, %ymm11
	andq	%rbx, %r15
	shrq	%rcx
	vpsllq	$2, %ymm12, %ymm13
	vpsllq	$3, %ymm3, %ymm14
	xorq	%rdx, %rsi
	andq	%r11, %rcx
	vpor	%ymm8, %ymm14, %ymm12
	vpor	%ymm11, %ymm13, %ymm7
	vpxor	%ymm10, %ymm1, %ymm1
	orq	%rcx, %r15
	leaq	0(,%r8,4), %r14
	vpxor	%ymm12, %ymm7, %ymm9
	shrq	$2, %r8
	vpermq	$144, %ymm1, %ymm15
	andq	%r10, %r14
	vmovq	%rsi, %xmm11
	vpxor	%ymm9, %ymm10, %ymm2
	andq	%r9, %r8
	vpbroadcastq	%xmm11, %ymm13
	orq	%r14, %r8
	vextracti128	$0x1, %ymm1, %xmm10
	xorq	%r15, %r8
	vpblendd	$3, %ymm13, %ymm15, %ymm8
	vpextrq	$1, %xmm10, %rcx
	vpand	%ymm0, %ymm13, %ymm14
	xorq	%r15, %rcx
	vpxor	%ymm8, %ymm12, %ymm3
	xorq	%r8, %rdx
	vpxor	%ymm3, %ymm14, %ymm12
	vextracti128	$0x1, %ymm2, %xmm7
	vmovq	%rdx, %xmm10
	vmovq	%rcx, %xmm3
	vpbroadcastq	%xmm10, %ymm11
	vpextrq	$1, %xmm7, %rdi
	vpermq	$144, %ymm12, %ymm10
	vpbroadcastq	%xmm3, %ymm7
	vpermq	$144, %ymm2, %ymm2
	xorq	%rdi, %rsi
	vpblendd	$3, %ymm11, %ymm2, %ymm13
	vpand	%ymm0, %ymm11, %ymm15
	vpblendd	$3, %ymm7, %ymm10, %ymm11
	xorq	%rsi, %rcx
	vpand	%ymm0, %ymm7, %ymm2
	vextracti128	$0x1, %ymm12, %xmm8
	vpxor	%ymm11, %ymm9, %ymm9
	vpxor	%ymm13, %ymm15, %ymm14
	vpextrq	$1, %xmm8, %rdx
	vpxor	%ymm9, %ymm2, %ymm13
	xorq	%r8, %rdx
	vpxor	%ymm13, %ymm14, %ymm15
	vpxor	%ymm14, %ymm1, %ymm1
	vpand	.LC6(%rip), %ymm15, %ymm3
	xorq	%rdx, %rdi
	vpand	.LC5(%rip), %ymm15, %ymm14
	vpand	.LC3(%rip), %ymm13, %ymm10
	vpand	.LC4(%rip), %ymm13, %ymm9
	vpxor	%ymm1, %ymm12, %ymm12
	movq	%rdi, %r8
	vpsrlq	$1, %ymm14, %ymm8
	vpsllq	$3, %ymm3, %ymm7
	vpand	%ymm5, %ymm12, %ymm14
	shrq	%r8
	vpand	%ymm6, %ymm12, %ymm12
	vpsrlq	$2, %ymm10, %ymm2
	vpor	%ymm8, %ymm7, %ymm11
	andq	%r11, %r8
	leaq	0(,%rdx,4), %r14
	vpsllq	$2, %ymm9, %ymm13
	shrq	$2, %rdx
	vpxor	.LC22(%rip), %ymm1, %ymm1
	andq	%r9, %rdx
	vpsrlq	$3, %ymm14, %ymm8
	leaq	(%rcx,%rcx), %r9
	andq	%r10, %r14
	vpsllq	$1, %ymm12, %ymm3
	shrq	$3, %rcx
	orq	%rdx, %r14
	vpor	%ymm2, %ymm13, %ymm15
	vpor	%ymm8, %ymm3, %ymm7
	andq	%r12, %rcx
	vpunpcklqdq	%ymm11, %ymm1, %ymm10
	andq	%r13, %r9
	vpunpcklqdq	%ymm7, %ymm15, %ymm13
	vpunpckhqdq	%ymm7, %ymm15, %ymm2
	vpunpckhqdq	%ymm11, %ymm1, %ymm11
	orq	%rcx, %r9
	leaq	0(,%rdi,8), %r15
	vmovq	%r14, %xmm15
	vmovq	%rsi, %xmm12
	vperm2i128	$32, %ymm13, %ymm10, %ymm8
	vperm2i128	$32, %ymm2, %ymm11, %ymm9
	vperm2i128	$49, %ymm2, %ymm11, %ymm1
	andq	%rbx, %r15
	orq	%r8, %r15
	vpinsrq	$1, %r9, %xmm15, %xmm14
	vpxor	%ymm9, %ymm8, %ymm11
	vpinsrq	$1, %r15, %xmm12, %xmm3
	vperm2i128	$49, %ymm13, %ymm10, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vinserti128	$0x1, %xmm14, %ymm3, %ymm7
	vpxor	%ymm1, %ymm11, %ymm2
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm7, %ymm8, %ymm15
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm7, %ymm10, %ymm13
	vpand	%ymm15, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm2, %ymm10, %ymm3
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpextrq	$1, %xmm3, %rcx
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vmovq	%xmm3, %rsi
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	xorq	%rcx, %rsi
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vpextrq	$1, %xmm9, %rdx
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rdi
	xorq	%rdx, %rdi
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %r8
	xorq	%rdi, %rcx
	vpand	%ymm0, %ymm1, %ymm3
	vpxor	%ymm15, %ymm2, %ymm2
	xorq	%r8, %rdx
	vpxor	%ymm2, %ymm3, %ymm11
	vextracti128	$0x1, %ymm12, %xmm9
	vmovq	%rcx, %xmm10
	vmovq	%rdx, %xmm2
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %r15
	vpbroadcastq	%xmm2, %ymm10
	vpermq	$144, %ymm11, %ymm9
	vpermq	$144, %ymm12, %ymm12
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpand	%ymm0, %ymm8, %ymm13
	vpblendd	$3, %ymm10, %ymm9, %ymm8
	vpxor	%ymm1, %ymm13, %ymm15
	vpand	%ymm0, %ymm10, %ymm12
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r14
	vpxor	%ymm7, %ymm12, %ymm13
	xorq	%rdi, %r14
	vpxor	%ymm14, %ymm11, %ymm11
	xorq	%r15, %rsi
	vpxor	%ymm13, %ymm15, %ymm1
	vpunpckhqdq	%ymm11, %ymm13, %ymm2
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	xorq	%r14, %r15
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm1, %ymm14, %ymm3
	vmovq	%r14, %xmm9
	xorq	%rsi, %rdx
	vmovq	%rsi, %xmm13
	vperm2i128	$32, %ymm2, %ymm3, %ymm8
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	movabsq	$4222189076152335, %r14
	vpinsrq	$1, %rdx, %xmm9, %xmm12
	vpinsrq	$1, %r15, %xmm13, %xmm14
	vperm2i128	$49, %ymm2, %ymm3, %ymm1
	movabsq	$-1152657617789587456, %r15
	vinserti128	$0x1, %xmm12, %ymm14, %ymm3
	vpxor	%ymm8, %ymm7, %ymm11
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	movabsq	$-4222189076152336, %rdi
	vpxor	%ymm3, %ymm7, %ymm15
	vpxor	%ymm4, %ymm11, %ymm11
	vpandn	%ymm3, %ymm10, %ymm12
	vpxor	%ymm1, %ymm11, %ymm9
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm8, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpand	%ymm1, %ymm13, %ymm13
	vpxor	%ymm1, %ymm11, %ymm11
	vpor	%ymm15, %ymm9, %ymm1
	vpand	%ymm9, %ymm10, %ymm2
	vpor	%ymm11, %ymm8, %ymm3
	vpxor	%ymm13, %ymm1, %ymm9
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm2, %ymm12, %ymm12
	vpxor	%ymm10, %ymm9, %ymm10
	vpxor	%ymm13, %ymm8, %ymm14
	vpxor	%ymm3, %ymm7, %ymm7
	vmovq	%r15, %xmm9
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpckhqdq	%ymm7, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm10, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm13
	vperm2i128	$49, %ymm12, %ymm15, %ymm3
	vperm2i128	$32, %ymm12, %ymm15, %ymm8
	vpbroadcastq	.LC32(%rip), %ymm12
	vperm2i128	$32, %ymm13, %ymm7, %ymm10
	vpbroadcastq	%xmm9, %ymm9
	vperm2i128	$49, %ymm13, %ymm7, %ymm7
	vpand	%ymm9, %ymm10, %ymm14
	vpand	%ymm12, %ymm10, %ymm13
	vpxor	%ymm11, %ymm2, %ymm2
	vpsrlq	$12, %ymm14, %ymm15
	vpsllq	$4, %ymm13, %ymm10
	vextracti128	$0x1, %ymm2, %xmm1
	vpextrq	$1, %xmm2, %r9
	vmovq	%xmm2, %rsi
	vpor	%ymm15, %ymm10, %ymm2
	vpbroadcastq	.LC34(%rip), %ymm10
	movq	%r9, %r8
	shrq	$12, %r9
	vmovq	%xmm1, %rcx
	vpxor	%ymm2, %ymm8, %ymm8
	andq	%r14, %r9
	vpextrq	$1, %xmm1, %rdx
	salq	$4, %r8
	andq	%rdi, %r8
	movq	%rdx, %rdi
	shrq	$4, %rdx
	vpbroadcastq	.LC35(%rip), %ymm11
	orq	%r9, %r8
	salq	$12, %rdi
	movabsq	$71777214294589695, %r14
	vmovq	%r14, %xmm1
	andq	%r15, %rdi
	xorq	%r8, %rsi
	movabsq	$1152657617789587455, %r9
	vpand	%ymm11, %ymm7, %ymm14
	vpbroadcastq	%xmm1, %ymm13
	andq	%r9, %rdx
	movabsq	$-71777214294589696, %r9
	vpsrlq	$4, %ymm14, %ymm1
	vpshufb	.LXRH_bswap16(%rip), %ymm3, %ymm15
	orq	%rdi, %rdx
	movq	%rcx, %rdi
	salq	$8, %rdi
	shrq	$8, %rcx
	vpbroadcastq	.LC36(%rip), %ymm14
	andq	%r14, %rcx
	andq	%r9, %rdi
	movabsq	$-71777214294589696, %r9
	vpand	%ymm14, %ymm7, %ymm7
	orq	%rcx, %rdi
	vpsllq	$12, %ymm7, %ymm7
	xorq	%rdx, %rdi
	vpor	%ymm1, %ymm7, %ymm7
	vmovq	%rsi, %xmm1
	xorq	%rdi, %r8
	vpxor	%ymm7, %ymm15, %ymm3
	vpxor	%ymm3, %ymm2, %ymm15
	vextracti128	$0x1, %ymm8, %xmm2
	vpextrq	$1, %xmm2, %rcx
	vpbroadcastq	%xmm1, %ymm2
	vpermq	$144, %ymm8, %ymm1
	vpblendd	$3, %ymm2, %ymm1, %ymm1
	vpand	%ymm0, %ymm2, %ymm2
	xorq	%rcx, %rdx
	vpxor	%ymm1, %ymm7, %ymm7
	vextracti128	$0x1, %ymm15, %xmm1
	vpermq	$144, %ymm15, %ymm15
	vpxor	%ymm7, %ymm2, %ymm2
	vmovq	%r8, %xmm7
	vpextrq	$1, %xmm1, %rcx
	vpbroadcastq	%xmm7, %ymm1
	xorq	%rcx, %rsi
	vpblendd	$3, %ymm1, %ymm15, %ymm7
	vextracti128	$0x1, %ymm2, %xmm15
	vpand	%ymm0, %ymm1, %ymm1
	vpextrq	$1, %xmm15, %r8
	vpxor	%ymm7, %ymm1, %ymm1
	vmovq	%rdx, %xmm7
	xorq	%rsi, %rdx
	vpbroadcastq	%xmm7, %ymm7
	vpermq	$144, %ymm2, %ymm15
	vpxor	%ymm1, %ymm8, %ymm8
	xorq	%r8, %rdi
	xorq	%rdi, %rcx
	vpblendd	$3, %ymm7, %ymm15, %ymm15
	vpand	%ymm0, %ymm7, %ymm7
	movq	%rcx, %r8
	shrq	$4, %rcx
	vpxor	%ymm15, %ymm3, %ymm3
	vpxor	%ymm8, %ymm2, %ymm2
	vpxor	%ymm3, %ymm7, %ymm15
	salq	$12, %r8
	vpand	%ymm9, %ymm2, %ymm9
	vpand	%ymm12, %ymm2, %ymm12
	vpxor	%ymm15, %ymm1, %ymm1
	andq	%r15, %r8
	vpand	%ymm11, %ymm1, %ymm11
	vpand	%ymm14, %ymm1, %ymm14
	movabsq	$1152657617789587455, %r15
	andq	%r15, %rcx
	vpsrlq	$4, %ymm11, %ymm7
	vpxor	.LC23(%rip), %ymm8, %ymm8
	movabsq	$4222189076152335, %r15
	vpsllq	$12, %ymm14, %ymm3
	orq	%r8, %rcx
	movq	%rdi, %r8
	salq	$8, %r8
	vpshufb	.LXRH_bswap16(%rip), %ymm15, %ymm14
	vpor	%ymm7, %ymm3, %ymm1
	vpsllq	$4, %ymm12, %ymm2
	vpunpcklqdq	%ymm1, %ymm8, %ymm10
	andq	%r9, %r8
	shrq	$8, %rdi
	vpsrlq	$12, %ymm9, %ymm7
	vpunpckhqdq	%ymm1, %ymm8, %ymm1
	andq	%r14, %rdi
	vpor	%ymm7, %ymm2, %ymm3
	vmovq	%rsi, %xmm7
	movabsq	$-4222189076152336, %r14
	orq	%rdi, %r8
	movq	%rdx, %rdi
	shrq	$12, %rdx
	vpunpcklqdq	%ymm3, %ymm14, %ymm11
	salq	$4, %rdi
	vpunpckhqdq	%ymm3, %ymm14, %ymm13
	vmovq	%r8, %xmm15
	andq	%r15, %rdx
	andq	%r14, %rdi
	vperm2i128	$32, %ymm11, %ymm10, %ymm8
	vperm2i128	$32, %ymm13, %ymm1, %ymm9
	vpxor	%ymm9, %ymm8, %ymm2
	vperm2i128	$49, %ymm11, %ymm10, %ymm10
	vperm2i128	$49, %ymm13, %ymm1, %ymm1
	orq	%rdx, %rdi
	vpinsrq	$1, %rdi, %xmm15, %xmm14
	vpinsrq	$1, %rcx, %xmm7, %xmm12
	vpxor	%ymm4, %ymm2, %ymm11
	vinserti128	$0x1, %xmm14, %ymm12, %ymm7
	vpxor	%ymm1, %ymm11, %ymm2
	vpor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm7, %ymm8, %ymm15
	vpandn	%ymm7, %ymm10, %ymm12
	vpxor	%ymm7, %ymm10, %ymm13
	vpand	%ymm15, %ymm11, %ymm11
	vpand	%ymm7, %ymm8, %ymm8
	vpand	%ymm2, %ymm10, %ymm3
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm11, %ymm3, %ymm3
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm7, %ymm8, %ymm8
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpextrq	$1, %xmm3, %rcx
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vmovq	%xmm3, %rsi
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	xorq	%rcx, %rsi
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	vextracti128	$0x1, %ymm3, %xmm9
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%rsi, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vpextrq	$1, %xmm9, %rdx
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %r9
	xorq	%rdx, %r9
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %r8
	xorq	%r9, %rcx
	vpand	%ymm0, %ymm1, %ymm3
	vpxor	%ymm15, %ymm2, %ymm2
	xorq	%r8, %rdx
	vpxor	%ymm2, %ymm3, %ymm11
	vextracti128	$0x1, %ymm12, %xmm9
	vmovq	%rcx, %xmm10
	vmovq	%rdx, %xmm2
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %rdi
	vpermq	$144, %ymm11, %ymm10
	vpbroadcastq	%xmm2, %ymm9
	vpermq	$144, %ymm12, %ymm12
	xorq	%rdi, %rsi
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpand	%ymm0, %ymm8, %ymm13
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	xorq	%rsi, %rdx
	vpxor	%ymm1, %ymm13, %ymm15
	vpand	%ymm0, %ymm9, %ymm12
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %r14
	vpxor	%ymm7, %ymm12, %ymm13
	xorq	%r9, %r14
	vpxor	%ymm14, %ymm11, %ymm11
	vpxor	%ymm13, %ymm15, %ymm1
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	xorq	%r14, %rdi
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm1, %ymm14, %ymm2
	vmovq	%r14, %xmm8
	vmovq	%rsi, %xmm13
	vperm2i128	$32, %ymm3, %ymm2, %ymm9
	vperm2i128	$32, %ymm10, %ymm15, %ymm7
	vpinsrq	$1, %rdx, %xmm8, %xmm12
	vpinsrq	$1, %rdi, %xmm13, %xmm14
	vperm2i128	$49, %ymm3, %ymm2, %ymm2
	vpxor	%ymm9, %ymm7, %ymm11
	vinserti128	$0x1, %xmm12, %ymm14, %ymm3
	vperm2i128	$49, %ymm10, %ymm15, %ymm10
	vpxor	%ymm4, %ymm11, %ymm11
	vpxor	%ymm3, %ymm7, %ymm15
	vpandn	%ymm3, %ymm10, %ymm12
	vpxor	%ymm2, %ymm11, %ymm1
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm3, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm3, %ymm9, %ymm11
	vpor	%ymm10, %ymm7, %ymm14
	vpxor	%ymm13, %ymm11, %ymm11
	vpand	%ymm3, %ymm7, %ymm7
	vpand	%ymm1, %ymm10, %ymm8
	vpxor	%ymm2, %ymm11, %ymm11
	vpand	%ymm2, %ymm13, %ymm2
	vpor	%ymm15, %ymm1, %ymm13
	vpor	%ymm11, %ymm9, %ymm3
	vpxor	%ymm2, %ymm13, %ymm1
	vpxor	%ymm14, %ymm7, %ymm7
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm3, %ymm7, %ymm7
	vpxor	%ymm10, %ymm1, %ymm10
	vpxor	%ymm2, %ymm9, %ymm14
	vpxor	%ymm8, %ymm12, %ymm12
	vpxor	%ymm11, %ymm8, %ymm8
	vpunpcklqdq	%ymm7, %ymm12, %ymm15
	vpunpcklqdq	%ymm14, %ymm10, %ymm3
	vpunpckhqdq	%ymm7, %ymm12, %ymm12
	vpunpckhqdq	%ymm14, %ymm10, %ymm7
	vperm2i128	$49, %ymm3, %ymm15, %ymm2
	vperm2i128	$32, %ymm3, %ymm15, %ymm13
	vperm2i128	$32, %ymm7, %ymm12, %ymm10
	vperm2i128	$49, %ymm7, %ymm12, %ymm1
	vpextrq	$1, %xmm8, %rsi
	vpshufb	.LXRH_rot16(%rip), %ymm10, %ymm12
	vmovq	%xmm8, %r15
	rorx	$48, %rsi, %rcx
	vpsrlq	$32, %ymm2, %ymm3
	vpsllq	$32, %ymm2, %ymm10
	xorq	%rcx, %r15
	vextracti128	$0x1, %ymm8, %xmm9
	vpshufb	.LXRH_rot48(%rip), %ymm1, %ymm1
	vpor	%ymm3, %ymm10, %ymm8
	vpxor	%ymm12, %ymm13, %ymm13
	vpxor	%ymm1, %ymm8, %ymm11
	vmovq	%r15, %xmm15
	vmovq	%xmm9, %r9
	vpextrq	$1, %xmm9, %rdx
	vpermq	$144, %ymm13, %ymm3
	vpxor	%ymm11, %ymm12, %ymm9
	rorx	$32, %r9, %r14
	vpbroadcastq	%xmm15, %ymm12
	vextracti128	$0x1, %ymm13, %xmm14
	rorx	$16, %rdx, %r8
	xorq	%r8, %r14
	vpblendd	$3, %ymm12, %ymm3, %ymm10
	vpextrq	$1, %xmm14, %r9
	vpand	%ymm0, %ymm12, %ymm7
	xorq	%r14, %rcx
	vpxor	%ymm10, %ymm1, %ymm2
	xorq	%r8, %r9
	vextracti128	$0x1, %ymm9, %xmm1
	vpxor	%ymm2, %ymm7, %ymm14
	vmovq	%rcx, %xmm8
	vmovq	%r9, %xmm2
	vpbroadcastq	%xmm8, %ymm15
	vpextrq	$1, %xmm1, %rsi
	vpbroadcastq	%xmm2, %ymm8
	vpermq	$144, %ymm14, %ymm1
	vpermq	$144, %ymm9, %ymm9
	vpand	%ymm0, %ymm15, %ymm3
	xorq	%rsi, %r15
	vpblendd	$3, %ymm15, %ymm9, %ymm12
	vpblendd	$3, %ymm8, %ymm1, %ymm15
	vpand	%ymm0, %ymm8, %ymm9
	xorq	%r15, %r9
	vpxor	%ymm12, %ymm3, %ymm10
	vpxor	%ymm15, %ymm11, %ymm11
	vextracti128	$0x1, %ymm14, %xmm7
	rorq	$48, %r9
	vpxor	%ymm11, %ymm9, %ymm12
	vpxor	%ymm10, %ymm13, %ymm13
	vpextrq	$1, %xmm7, %rdi
	vpxor	%ymm13, %ymm14, %ymm14
	vpshufd	$177, %ymm12, %ymm15
	vpxor	%ymm12, %ymm10, %ymm10
	xorq	%r14, %rdi
	vpxor	.LC24(%rip), %ymm13, %ymm13
	vpshufb	.LXRH_rot48(%rip), %ymm10, %ymm2
	xorq	%rdi, %rsi
	rorx	$32, %rdi, %rdx
	rorq	$16, %rsi
	vpshufb	.LXRH_rot16(%rip), %ymm14, %ymm12
	vmovq	%r15, %xmm11
	vpunpcklqdq	%ymm2, %ymm13, %ymm14
	vpunpckhqdq	%ymm2, %ymm13, %ymm7
	vpunpckhqdq	%ymm12, %ymm15, %ymm3
	vpunpcklqdq	%ymm12, %ymm15, %ymm10
	vmovq	%rdx, %xmm2
	vperm2i128	$32, %ymm3, %ymm7, %ymm9
	vperm2i128	$32, %ymm10, %ymm14, %ymm8
	vpinsrq	$1, %r9, %xmm2, %xmm15
	vpinsrq	$1, %rsi, %xmm11, %xmm12
	vperm2i128	$49, %ymm3, %ymm7, %ymm1
	vinserti128	$0x1, %xmm15, %ymm12, %ymm7
	vpxor	%ymm9, %ymm8, %ymm3
	vperm2i128	$49, %ymm10, %ymm14, %ymm10
	vpxor	%ymm7, %ymm8, %ymm15
	vpxor	%ymm4, %ymm3, %ymm11
	vpxor	%ymm1, %ymm11, %ymm2
	vpandn	%ymm7, %ymm10, %ymm12
	vpand	%ymm15, %ymm11, %ymm11
	vpxor	%ymm7, %ymm10, %ymm13
	vpxor	%ymm11, %ymm12, %ymm12
	vpand	%ymm7, %ymm9, %ymm11
	vpxor	%ymm13, %ymm11, %ymm11
	vpor	%ymm10, %ymm8, %ymm14
	vpand	%ymm7, %ymm8, %ymm8
	vpxor	%ymm1, %ymm11, %ymm11
	vpand	%ymm1, %ymm13, %ymm1
	vpor	%ymm15, %ymm2, %ymm13
	vpand	%ymm2, %ymm10, %ymm3
	vpor	%ymm11, %ymm9, %ymm7
	vpxor	%ymm1, %ymm13, %ymm2
	vpxor	%ymm14, %ymm8, %ymm8
	vpxor	%ymm14, %ymm9, %ymm9
	vpxor	%ymm10, %ymm2, %ymm15
	vpxor	%ymm1, %ymm9, %ymm14
	vpxor	%ymm3, %ymm12, %ymm12
	vpxor	%ymm7, %ymm8, %ymm8
	vpxor	%ymm11, %ymm3, %ymm3
	vpunpcklqdq	%ymm8, %ymm12, %ymm7
	vpunpcklqdq	%ymm14, %ymm15, %ymm10
	vpunpckhqdq	%ymm8, %ymm12, %ymm12
	vpextrq	$1, %xmm3, %r9
	vpunpckhqdq	%ymm14, %ymm15, %ymm8
	vmovq	%xmm3, %r14
	vperm2i128	$32, %ymm10, %ymm7, %ymm1
	vperm2i128	$32, %ymm8, %ymm12, %ymm13
	vperm2i128	$49, %ymm8, %ymm12, %ymm2
	vperm2i128	$49, %ymm10, %ymm7, %ymm15
	vextracti128	$0x1, %ymm3, %xmm9
	xorq	%r9, %r14
	vpxor	%ymm13, %ymm1, %ymm14
	vpxor	%ymm2, %ymm15, %ymm7
	vmovq	%r14, %xmm8
	vpxor	%ymm7, %ymm13, %ymm12
	vpbroadcastq	%xmm8, %ymm1
	vpextrq	$1, %xmm9, %rdi
	vextracti128	$0x1, %ymm14, %xmm10
	vpermq	$144, %ymm14, %ymm13
	vmovq	%xmm9, %rsi
	xorq	%rdi, %rsi
	vpblendd	$3, %ymm1, %ymm13, %ymm15
	vpextrq	$1, %xmm10, %r15
	xorq	%rsi, %r9
	vpand	%ymm0, %ymm1, %ymm3
	vpxor	%ymm15, %ymm2, %ymm2
	xorq	%r15, %rdi
	vpxor	%ymm2, %ymm3, %ymm11
	vextracti128	$0x1, %ymm12, %xmm9
	vmovq	%r9, %xmm10
	vmovq	%rdi, %xmm2
	vpbroadcastq	%xmm10, %ymm8
	vpextrq	$1, %xmm9, %rdx
	vpermq	$144, %ymm11, %ymm10
	vpbroadcastq	%xmm2, %ymm9
	vpermq	$144, %ymm12, %ymm12
	xorq	%rdx, %r14
	vpblendd	$3, %ymm8, %ymm12, %ymm1
	vpand	%ymm0, %ymm8, %ymm13
	vpblendd	$3, %ymm9, %ymm10, %ymm8
	vpxor	%ymm1, %ymm13, %ymm15
	vpand	%ymm0, %ymm9, %ymm12
	vextracti128	$0x1, %ymm11, %xmm3
	vpxor	%ymm8, %ymm7, %ymm7
	vpxor	%ymm15, %ymm14, %ymm14
	vpextrq	$1, %xmm3, %rcx
	vpxor	%ymm7, %ymm12, %ymm13
	vpxor	%ymm14, %ymm11, %ymm11
	vmovq	%r14, %xmm7
	xorq	%rsi, %rcx
	vpxor	%ymm13, %ymm15, %ymm1
	vpunpcklqdq	%ymm11, %ymm13, %ymm10
	vpunpckhqdq	%ymm11, %ymm13, %ymm3
	xorq	%rcx, %rdx
	vpunpcklqdq	%ymm1, %ymm14, %ymm15
	vpunpckhqdq	%ymm1, %ymm14, %ymm2
	vmovq	%rcx, %xmm12
	xorq	%r14, %rdi
	vperm2i128	$32, %ymm10, %ymm15, %ymm8
	vperm2i128	$32, %ymm3, %ymm2, %ymm9
	vpinsrq	$1, %rdi, %xmm12, %xmm13
	vpinsrq	$1, %rdx, %xmm7, %xmm14
	vperm2i128	$49, %ymm10, %ymm15, %ymm15
	vpxor	%ymm9, %ymm8, %ymm11
	vinserti128	$0x1, %xmm13, %ymm14, %ymm10
	vperm2i128	$49, %ymm3, %ymm2, %ymm2
	vpxor	%ymm4, %ymm11, %ymm4
	vpxor	%ymm10, %ymm8, %ymm14
	vpxor	%ymm2, %ymm4, %ymm1
	vpandn	%ymm10, %ymm15, %ymm7
	vpand	%ymm14, %ymm4, %ymm11
	vpxor	%ymm10, %ymm15, %ymm12
	vpand	%ymm1, %ymm15, %ymm3
	vpxor	%ymm11, %ymm7, %ymm4
	vpand	%ymm10, %ymm9, %ymm7
	vpor	%ymm15, %ymm8, %ymm13
	vpxor	%ymm3, %ymm4, %ymm11
	vpxor	%ymm12, %ymm7, %ymm4
	vpand	%ymm10, %ymm8, %ymm8
	vpxor	%ymm2, %ymm4, %ymm4
	vpand	%ymm2, %ymm12, %ymm2
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm4, %ymm9, %ymm7
	vpxor	%ymm2, %ymm12, %ymm1
	vpxor	%ymm13, %ymm8, %ymm10
	vpxor	%ymm13, %ymm9, %ymm9
	vpxor	%ymm7, %ymm10, %ymm8
	vpxor	%ymm15, %ymm1, %ymm15
	vpxor	%ymm2, %ymm9, %ymm13
	vpunpcklqdq	%ymm8, %ymm11, %ymm14
	vpxor	%ymm4, %ymm3, %ymm3
	vpunpcklqdq	%ymm13, %ymm15, %ymm7
	vpunpckhqdq	%ymm8, %ymm11, %ymm11
	vpunpckhqdq	%ymm13, %ymm15, %ymm8
	vperm2i128	$32, %ymm8, %ymm11, %ymm12
	vperm2i128	$49, %ymm7, %ymm14, %ymm2
	vperm2i128	$49, %ymm8, %ymm11, %ymm1
	vextracti128	$0x1, %ymm3, %xmm15
	vpand	.LC3(%rip), %ymm2, %ymm11
	vpextrq	$1, %xmm3, %r9
	vpand	.LC4(%rip), %ymm2, %ymm8
	vpand	.LC5(%rip), %ymm1, %ymm2
	vpextrq	$1, %xmm15, %rsi
	vpand	.LC6(%rip), %ymm1, %ymm1
	leaq	(%r9,%r9), %rdi
	shrq	$3, %r9
	vperm2i128	$32, %ymm7, %ymm14, %ymm10
	andq	%r12, %r9
	vmovq	%xmm15, %r8
	vpand	%ymm5, %ymm12, %ymm5
	andq	%r13, %rdi
	vpand	%ymm6, %ymm12, %ymm6
	vpsrlq	$2, %ymm11, %ymm7
	orq	%r9, %rdi
	vpsllq	$2, %ymm8, %ymm12
	vpsllq	$3, %ymm1, %ymm4
	vmovq	%xmm3, %r14
	leaq	0(,%rsi,8), %r12
	vpsrlq	$1, %ymm2, %ymm3
	shrq	%rsi
	xorq	%rdi, %r14
	vpor	%ymm3, %ymm4, %ymm15
	vpsrlq	$3, %ymm5, %ymm13
	andq	%r11, %rsi
	andq	%r12, %rbx
	vpor	%ymm7, %ymm12, %ymm5
	vpsllq	$1, %ymm6, %ymm9
	leaq	0(,%r8,4), %r11
	shrq	$2, %r8
	vpor	%ymm13, %ymm9, %ymm14
	andq	%r10, %r11
	orq	%rsi, %rbx
	movabsq	$3689348814741910323, %r10
	vpxor	%ymm15, %ymm5, %ymm13
	vpxor	%ymm14, %ymm10, %ymm10
	andq	%r10, %r8
	movabsq	$8608480567731124087, %r12
	vpxor	%ymm13, %ymm14, %ymm6
	vmovq	%r14, %xmm14
	orq	%r8, %r11
	movq	%r13, %r10
	vpbroadcastq	%xmm14, %ymm11
	vpermq	$144, %ymm10, %ymm7
	vpermq	$144, %ymm6, %ymm5
	xorq	%rbx, %r11
	vextracti128	$0x1, %ymm10, %xmm9
	vpblendd	$3, %ymm11, %ymm7, %ymm8
	vpand	%ymm0, %ymm11, %ymm12
	xorq	%r11, %rdi
	vpextrq	$1, %xmm9, %rcx
	vpxor	%ymm8, %ymm15, %ymm2
	vmovq	%rdi, %xmm4
	movabsq	$-8608480567731124088, %r13
	xorq	%rbx, %rcx
	vpxor	%ymm2, %ymm12, %ymm3
	vpbroadcastq	%xmm4, %ymm15
	movabsq	$-3689348814741910324, %rbx
	vmovq	%rcx, %xmm7
	vpermq	$144, %ymm3, %ymm12
	vpand	%ymm0, %ymm15, %ymm9
	movabsq	$1229782938247303441, %rsi
	vpbroadcastq	%xmm7, %ymm8
	vextracti128	$0x1, %ymm6, %xmm1
	vpblendd	$3, %ymm15, %ymm5, %ymm6
	vpblendd	$3, %ymm8, %ymm12, %ymm2
	vpxor	%ymm6, %ymm9, %ymm14
	vpextrq	$1, %xmm1, %rdx
	vextracti128	$0x1, %ymm3, %xmm11
	vpand	%ymm0, %ymm8, %ymm1
	vpxor	%ymm2, %ymm13, %ymm13
	xorq	%rdx, %r14
	vpxor	%ymm13, %ymm1, %ymm15
	vpxor	%ymm14, %ymm10, %ymm10
	vpextrq	$1, %xmm11, %r15
	xorq	%r14, %rcx
	vpxor	%ymm15, %ymm14, %ymm5
	xorq	%r11, %r15
	vpand	.LC5(%rip), %ymm5, %ymm4
	movabsq	$3689348814741910323, %r11
	vpand	.LC6(%rip), %ymm5, %ymm9
	vpand	.LC3(%rip), %ymm15, %ymm7
	vpxor	%ymm10, %ymm3, %ymm3
	xorq	%r15, %rdx
	vpand	.LC4(%rip), %ymm15, %ymm12
	vpand	.LC1(%rip), %ymm3, %ymm1
	leaq	0(,%r15,4), %r8
	shrq	$2, %r15
	vpsrlq	$1, %ymm4, %ymm6
	vpsllq	$3, %ymm9, %ymm14
	andq	%r11, %r15
	andq	%rbx, %r8
	vpand	.LC2(%rip), %ymm3, %ymm3
	vpxor	.LC25(%rip), %ymm10, %ymm10
	leaq	(%rcx,%rcx), %rdi
	orq	%r15, %r8
	vpsrlq	$2, %ymm7, %ymm8
	shrq	$3, %rcx
	vpor	%ymm6, %ymm14, %ymm11
	andq	%r10, %rdi
	vpsllq	$2, %ymm12, %ymm2
	vpsrlq	$3, %ymm1, %ymm15
	andq	%rsi, %rcx
	vpsllq	$1, %ymm3, %ymm5
	leaq	0(,%rdx,8), %r9
	shrq	%rdx
	vpor	%ymm8, %ymm2, %ymm13
	vpor	%ymm15, %ymm5, %ymm4
	andq	%r13, %r9
	vpunpcklqdq	%ymm11, %ymm10, %ymm9
	andq	%r12, %rdx
	vpunpckhqdq	%ymm11, %ymm10, %ymm14
	vpunpckhqdq	%ymm4, %ymm13, %ymm12
	vpunpcklqdq	%ymm4, %ymm13, %ymm11
	orq	%r9, %rdx
	vmovq	%r8, %xmm13
	vperm2i128	$32, %ymm11, %ymm9, %ymm6
	vperm2i128	$32, %ymm12, %ymm14, %ymm7
	orq	%rcx, %rdi
	vmovq	%r14, %xmm1
	vpinsrq	$1, %rdi, %xmm13, %xmm15
	vpcmpeqd	%ymm5, %ymm5, %ymm5
	vpinsrq	$1, %rdx, %xmm1, %xmm3
	vperm2i128	$49, %ymm12, %ymm14, %ymm2
	vpxor	%ymm7, %ymm6, %ymm4
	vinserti128	$0x1, %xmm15, %ymm3, %ymm14
	vperm2i128	$49, %ymm11, %ymm9, %ymm8
	vpxor	%ymm5, %ymm4, %ymm9
	vpxor	%ymm14, %ymm6, %ymm13
	vpxor	%ymm14, %ymm8, %ymm11
	vpxor	%ymm2, %ymm9, %ymm15
	vpandn	%ymm14, %ymm8, %ymm10
	vpand	%ymm13, %ymm9, %ymm1
	vpand	%ymm14, %ymm7, %ymm9
	vpxor	%ymm1, %ymm10, %ymm3
	vpxor	%ymm11, %ymm9, %ymm1
	vpor	%ymm8, %ymm6, %ymm12
	vpxor	%ymm2, %ymm1, %ymm9
	vpand	%ymm15, %ymm8, %ymm4
	vpand	%ymm2, %ymm11, %ymm2
	vpand	%ymm14, %ymm6, %ymm6
	vpor	%ymm13, %ymm15, %ymm11
	vpxor	%ymm4, %ymm3, %ymm10
	vpxor	%ymm12, %ymm6, %ymm14
	vpor	%ymm9, %ymm7, %ymm3
	vpxor	%ymm2, %ymm11, %ymm15
	vpxor	%ymm12, %ymm7, %ymm7
	vpxor	%ymm9, %ymm4, %ymm4
	vpxor	%ymm3, %ymm14, %ymm1
	vpxor	%ymm2, %ymm7, %ymm12
	vpxor	%ymm8, %ymm15, %ymm13
	vpunpcklqdq	%ymm1, %ymm10, %ymm14
	vpunpcklqdq	%ymm12, %ymm13, %ymm8
	vpunpckhqdq	%ymm12, %ymm13, %ymm6
	vpunpckhqdq	%ymm1, %ymm10, %ymm10
	vpextrq	$1, %xmm4, %r15
	vmovq	%xmm4, %r14
	vperm2i128	$32, %ymm6, %ymm10, %ymm11
	vperm2i128	$32, %ymm8, %ymm14, %ymm1
	vperm2i128	$49, %ymm6, %ymm10, %ymm3
	vextracti128	$0x1, %ymm4, %xmm15
	xorq	%r15, %r14
	vpxor	%ymm11, %ymm1, %ymm13
	vmovq	%r14, %xmm10
	vperm2i128	$49, %ymm8, %ymm14, %ymm2
	vpextrq	$1, %xmm15, %rdx
	vpbroadcastq	%xmm10, %ymm8
	vextracti128	$0x1, %ymm13, %xmm14
	vpermq	$144, %ymm13, %ymm6
	vmovq	%xmm15, %r9
	vpxor	%ymm3, %ymm2, %ymm12
	xorq	%rdx, %r9
	vpblendd	$3, %ymm8, %ymm6, %ymm1
	vpextrq	$1, %xmm14, %rcx
	vpxor	%ymm12, %ymm11, %ymm7
	xorq	%r9, %r15
	vpand	%ymm0, %ymm8, %ymm11
	xorq	%rcx, %rdx
	vpxor	%ymm1, %ymm3, %ymm2
	vmovq	%r15, %xmm9
	vmovq	%rdx, %xmm1
	vpxor	%ymm2, %ymm11, %ymm4
	vpbroadcastq	%xmm9, %ymm15
	vpbroadcastq	%xmm1, %ymm11
	vextracti128	$0x1, %ymm7, %xmm3
	vpermq	$144, %ymm4, %ymm2
	vpermq	$144, %ymm7, %ymm7
	vpblendd	$3, %ymm15, %ymm7, %ymm14
	vpand	%ymm0, %ymm15, %ymm10
	vpextrq	$1, %xmm3, %rdi
	vpxor	%ymm14, %ymm10, %ymm8
	vpand	%ymm0, %ymm11, %ymm9
	vextracti128	$0x1, %ymm4, %xmm6
	xorq	%rdi, %r14
	vpblendd	$3, %ymm11, %ymm2, %ymm3
	vpxor	%ymm8, %ymm13, %ymm13
	vpextrq	$1, %xmm6, %r8
	xorq	%r14, %rdx
	vpxor	%ymm3, %ymm12, %ymm12
	xorq	%r8, %r9
	vpxor	%ymm13, %ymm4, %ymm4
	movabsq	$-1152657617789587456, %r8
	vpxor	%ymm12, %ymm9, %ymm15
	xorq	%r9, %rdi
	vmovq	%r9, %xmm1
	vpxor	%ymm15, %ymm8, %ymm7
	vpunpckhqdq	%ymm4, %ymm15, %ymm6
	vpunpcklqdq	%ymm4, %ymm15, %ymm8
	vpunpcklqdq	%ymm7, %ymm13, %ymm14
	vpunpckhqdq	%ymm7, %ymm13, %ymm10
	vmovq	%r14, %xmm9
	movabsq	$-4222189076152336, %r14
	vperm2i128	$32, %ymm8, %ymm14, %ymm2
	vperm2i128	$32, %ymm6, %ymm10, %ymm15
	vpinsrq	$1, %rdx, %xmm1, %xmm11
	vpinsrq	$1, %rdi, %xmm9, %xmm12
	vperm2i128	$49, %ymm8, %ymm14, %ymm4
	vpxor	%ymm15, %ymm2, %ymm13
	movabsq	$4222189076152335, %rdi
	vinserti128	$0x1, %xmm11, %ymm12, %ymm14
	vperm2i128	$49, %ymm6, %ymm10, %ymm3
	vpxor	%ymm5, %ymm13, %ymm1
	vpxor	%ymm14, %ymm2, %ymm13
	vpxor	%ymm14, %ymm4, %ymm8
	vpxor	%ymm3, %ymm1, %ymm6
	vpandn	%ymm14, %ymm4, %ymm10
	vpand	%ymm13, %ymm1, %ymm11
	vpand	%ymm14, %ymm15, %ymm1
	vpand	%ymm6, %ymm4, %ymm7
	vpxor	%ymm11, %ymm10, %ymm9
	vpxor	%ymm8, %ymm1, %ymm11
	vpor	%ymm4, %ymm2, %ymm12
	vpxor	%ymm7, %ymm9, %ymm10
	vpand	%ymm3, %ymm8, %ymm8
	vpxor	%ymm3, %ymm11, %ymm9
	vpand	%ymm14, %ymm2, %ymm2
	vpor	%ymm13, %ymm6, %ymm3
	vpor	%ymm9, %ymm15, %ymm1
	vpxor	%ymm8, %ymm3, %ymm6
	vpxor	%ymm12, %ymm2, %ymm14
	vpxor	%ymm12, %ymm15, %ymm15
	vpxor	%ymm1, %ymm14, %ymm2
	vpxor	%ymm4, %ymm6, %ymm13
	vpxor	%ymm8, %ymm15, %ymm4
	vpunpcklqdq	%ymm2, %ymm10, %ymm12
	vpunpckhqdq	%ymm2, %ymm10, %ymm10
	vpunpckhqdq	%ymm4, %ymm13, %ymm14
	vpunpcklqdq	%ymm4, %ymm13, %ymm11
	vpxor	%ymm9, %ymm7, %ymm7
	vperm2i128	$32, %ymm14, %ymm10, %ymm3
	vpand	.LC9(%rip), %ymm3, %ymm4
	vpextrq	$1, %xmm7, %r9
	vperm2i128	$49, %ymm11, %ymm12, %ymm6
	vperm2i128	$32, %ymm11, %ymm12, %ymm8
	vextracti128	$0x1, %ymm7, %xmm2
	movq	%r9, %rdx
	vpsllq	$4, %ymm4, %ymm12
	vperm2i128	$49, %ymm14, %ymm10, %ymm1
	vpextrq	$1, %xmm2, %rsi
	vpbroadcastq	.LC35(%rip), %ymm4
	salq	$4, %rdx
	vpand	.LC8(%rip), %ymm3, %ymm13
	shrq	$12, %r9
	andq	%rdi, %r9
	vmovq	%xmm2, %r15
	vmovq	%xmm7, %rcx
	andq	%r14, %rdx
	vpsrlq	$12, %ymm13, %ymm15
	movq	%rsi, %r14
	orq	%r9, %rdx
	salq	$12, %r14
	shrq	$4, %rsi
	xorq	%rdx, %rcx
	vpor	%ymm15, %ymm12, %ymm11
	vpshufb	.LXRH_bswap16(%rip), %ymm6, %ymm14
	vpand	%ymm4, %ymm1, %ymm6
	vpxor	%ymm11, %ymm8, %ymm15
	andq	%r8, %r14
	vpsrlq	$4, %ymm6, %ymm2
	vextracti128	$0x1, %ymm15, %xmm10
	vpbroadcastq	.LC36(%rip), %ymm6
	movabsq	$1152657617789587455, %r9
	andq	%r9, %rsi
	movabsq	$-71777214294589696, %r9
	movabsq	$71777214294589695, %rdi
	orq	%rsi, %r14
	vmovdqa	%ymm14, %ymm8
	vpand	%ymm6, %ymm1, %ymm1
	movq	%r15, %rsi
	vmovq	%rcx, %xmm14
	vpsllq	$12, %ymm1, %ymm9
	vpermq	$144, %ymm15, %ymm7
	salq	$8, %rsi
	vpbroadcastq	%xmm14, %ymm3
	shrq	$8, %r15
	vpor	%ymm2, %ymm9, %ymm13
	andq	%r9, %rsi
	vpblendd	$3, %ymm3, %ymm7, %ymm2
	vpxor	%ymm13, %ymm8, %ymm12
	vpand	%ymm0, %ymm3, %ymm1
	andq	%rdi, %r15
	orq	%rsi, %r15
	vpxor	%ymm2, %ymm13, %ymm9
	vpxor	%ymm12, %ymm11, %ymm11
	movabsq	$1152657617789587455, %rdi
	xorq	%r14, %r15
	vpxor	%ymm9, %ymm1, %ymm13
	vextracti128	$0x1, %ymm11, %xmm8
	vextracti128	$0x1, %ymm13, %xmm2
	vpermq	$144, %ymm13, %ymm9
	vpermq	$144, %ymm11, %ymm11
	xorq	%r15, %rdx
	vpextrq	$1, %xmm10, %rsi
	vmovq	%rdx, %xmm10
	vpextrq	$1, %xmm2, %rdx
	xorq	%r14, %rsi
	vpextrq	$1, %xmm8, %r14
	vpbroadcastq	%xmm10, %ymm14
	xorq	%r15, %rdx
	movq	%r14, %r15
	vmovq	%rsi, %xmm1
	vpand	%ymm0, %ymm14, %ymm3
	xorq	%r14, %rcx
	xorq	%rdx, %r15
	vpbroadcastq	%xmm1, %ymm8
	xorq	%rcx, %rsi
	movq	%r15, %r14
	vpblendd	$3, %ymm14, %ymm11, %ymm7
	vpblendd	$3, %ymm8, %ymm9, %ymm14
	vpand	%ymm0, %ymm8, %ymm11
	salq	$12, %r14
	vpxor	%ymm14, %ymm12, %ymm12
	vpxor	%ymm7, %ymm3, %ymm10
	shrq	$4, %r15
	vpxor	%ymm12, %ymm11, %ymm2
	vpxor	%ymm10, %ymm15, %ymm15
	andq	%r8, %r14
	andq	%rdi, %r15
	vpxor	%ymm2, %ymm10, %ymm7
	vpxor	%ymm15, %ymm13, %ymm13
	movabsq	$-4222189076152336, %rdi
	orq	%r15, %r14
	vpand	%ymm4, %ymm7, %ymm3
	vpand	%ymm6, %ymm7, %ymm1
	movq	%rdx, %r15
	salq	$8, %r15
	shrq	$8, %rdx
	vpand	.LC8(%rip), %ymm13, %ymm7
	andq	%r9, %r15
	movabsq	$71777214294589695, %r9
	andq	%r9, %rdx
	vpand	.LC9(%rip), %ymm13, %ymm13
	vpxor	.LC26(%rip), %ymm15, %ymm15
	movabsq	$4222189076152335, %r9
	vpsrlq	$4, %ymm3, %ymm10
	vpsllq	$12, %ymm1, %ymm8
	orq	%rdx, %r15
	movq	%rsi, %rdx
	vpor	%ymm10, %ymm8, %ymm14
	salq	$4, %rdx
	vpshufb	.LXRH_bswap16(%rip), %ymm2, %ymm3
	shrq	$12, %rsi
	andq	%rdi, %rdx
	vpunpcklqdq	%ymm14, %ymm15, %ymm9
	vpsrlq	$12, %ymm7, %ymm10
	vpsllq	$4, %ymm13, %ymm1
	andq	%r9, %rsi
	movq	%r9, %rdi
	vpor	%ymm10, %ymm1, %ymm11
	vpunpckhqdq	%ymm14, %ymm15, %ymm14
	orq	%rsi, %rdx
	vpunpckhqdq	%ymm11, %ymm3, %ymm12
	vpunpcklqdq	%ymm11, %ymm3, %ymm8
	vmovq	%r15, %xmm2
	vmovq	%rcx, %xmm13
	vperm2i128	$32, %ymm8, %ymm9, %ymm3
	vperm2i128	$32, %ymm12, %ymm14, %ymm7
	vpinsrq	$1, %rdx, %xmm2, %xmm10
	vpinsrq	$1, %r14, %xmm13, %xmm11
	vperm2i128	$49, %ymm12, %ymm14, %ymm1
	vinserti128	$0x1, %xmm10, %ymm11, %ymm15
	vpxor	%ymm7, %ymm3, %ymm14
	vperm2i128	$49, %ymm8, %ymm9, %ymm9
	vpxor	%ymm5, %ymm14, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpandn	%ymm15, %ymm9, %ymm11
	vpxor	%ymm1, %ymm10, %ymm2
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm7, %ymm10
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpand	%ymm2, %ymm9, %ymm8
	vpxor	%ymm1, %ymm10, %ymm10
	vpand	%ymm1, %ymm12, %ymm1
	vpor	%ymm14, %ymm2, %ymm12
	vpor	%ymm10, %ymm7, %ymm15
	vpxor	%ymm1, %ymm12, %ymm2
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm13, %ymm7, %ymm7
	vpxor	%ymm9, %ymm2, %ymm14
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm1, %ymm7, %ymm13
	vpxor	%ymm10, %ymm8, %ymm8
	vpxor	%ymm15, %ymm3, %ymm3
	vpunpcklqdq	%ymm3, %ymm11, %ymm9
	vpunpckhqdq	%ymm3, %ymm11, %ymm15
	vpextrq	$1, %xmm8, %rdx
	vpunpckhqdq	%ymm13, %ymm14, %ymm3
	vpunpcklqdq	%ymm13, %ymm14, %ymm11
	vmovq	%xmm8, %rsi
	vperm2i128	$32, %ymm11, %ymm9, %ymm1
	vperm2i128	$32, %ymm3, %ymm15, %ymm12
	vperm2i128	$49, %ymm3, %ymm15, %ymm2
	xorq	%rdx, %rsi
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	vextracti128	$0x1, %ymm8, %xmm7
	vpxor	%ymm12, %ymm1, %ymm13
	vpxor	%ymm2, %ymm14, %ymm9
	vmovq	%rsi, %xmm3
	vpextrq	$1, %xmm7, %rcx
	vpxor	%ymm9, %ymm12, %ymm15
	vpbroadcastq	%xmm3, %ymm1
	vextracti128	$0x1, %ymm13, %xmm11
	vpermq	$144, %ymm13, %ymm12
	vmovq	%xmm7, %r14
	vpextrq	$1, %xmm11, %r15
	xorq	%rcx, %r14
	vpblendd	$3, %ymm1, %ymm12, %ymm14
	vpand	%ymm0, %ymm1, %ymm8
	xorq	%r15, %rcx
	xorq	%r14, %rdx
	vpxor	%ymm14, %ymm2, %ymm2
	vextracti128	$0x1, %ymm15, %xmm7
	vpxor	%ymm2, %ymm8, %ymm10
	vmovq	%rdx, %xmm11
	vmovq	%rcx, %xmm2
	vpbroadcastq	%xmm11, %ymm3
	vpextrq	$1, %xmm7, %r9
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm2, %ymm7
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm0, %ymm3, %ymm12
	xorq	%r9, %rsi
	vpblendd	$3, %ymm3, %ymm15, %ymm1
	vpblendd	$3, %ymm7, %ymm11, %ymm3
	vpand	%ymm0, %ymm7, %ymm15
	xorq	%rsi, %rcx
	vpxor	%ymm1, %ymm12, %ymm14
	vextracti128	$0x1, %ymm10, %xmm8
	vpxor	%ymm3, %ymm9, %ymm9
	vpxor	%ymm9, %ymm15, %ymm12
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm8, %rdx
	xorq	%r14, %rdx
	vpxor	%ymm13, %ymm10, %ymm10
	vpxor	%ymm12, %ymm14, %ymm1
	vpunpcklqdq	%ymm1, %ymm13, %ymm14
	vpunpcklqdq	%ymm10, %ymm12, %ymm11
	vpunpckhqdq	%ymm10, %ymm12, %ymm8
	xorq	%rdx, %r9
	vpunpckhqdq	%ymm1, %ymm13, %ymm2
	vmovq	%rdx, %xmm15
	vmovq	%rsi, %xmm13
	vperm2i128	$32, %ymm8, %ymm2, %ymm7
	vperm2i128	$32, %ymm11, %ymm14, %ymm3
	vpinsrq	$1, %r9, %xmm13, %xmm10
	vpinsrq	$1, %rcx, %xmm15, %xmm12
	vpxor	%ymm7, %ymm3, %ymm1
	vperm2i128	$49, %ymm11, %ymm14, %ymm9
	vinserti128	$0x1, %xmm12, %ymm10, %ymm15
	vperm2i128	$49, %ymm8, %ymm2, %ymm2
	vpxor	%ymm5, %ymm1, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpxor	%ymm2, %ymm10, %ymm1
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm7, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm1, %ymm9, %ymm8
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm2, %ymm10, %ymm10
	vpand	%ymm2, %ymm12, %ymm2
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm10, %ymm7, %ymm15
	vpxor	%ymm2, %ymm12, %ymm1
	vpxor	%ymm13, %ymm7, %ymm7
	vpxor	%ymm2, %ymm7, %ymm13
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
	vextracti128	$0x1, %ymm8, %xmm7
	rorx	$48, %rsi, %r9
	vpshufd	$177, %ymm2, %ymm8
	xorq	%r9, %rcx
	vpshufb	.LXRH_rot48(%rip), %ymm1, %ymm1
	vpxor	%ymm9, %ymm14, %ymm14
	vpextrq	$1, %xmm7, %r14
	vpxor	%ymm1, %ymm8, %ymm10
	vmovq	%rcx, %xmm15
	rorx	$16, %r14, %rdx
	vpermq	$144, %ymm14, %ymm3
	vmovq	%xmm7, %r15
	vpxor	%ymm10, %ymm9, %ymm13
	vextracti128	$0x1, %ymm14, %xmm7
	vpbroadcastq	%xmm15, %ymm9
	rorx	$32, %r15, %r15
	xorq	%rdx, %r15
	vpextrq	$1, %xmm7, %rsi
	vpblendd	$3, %ymm9, %ymm3, %ymm12
	xorq	%r15, %r9
	vpand	%ymm0, %ymm9, %ymm11
	xorq	%rdx, %rsi
	vpxor	%ymm12, %ymm1, %ymm2
	vmovq	%r9, %xmm8
	vextracti128	$0x1, %ymm13, %xmm1
	vpxor	%ymm2, %ymm11, %ymm15
	vpbroadcastq	%xmm8, %ymm7
	vpermq	$144, %ymm13, %ymm13
	vmovq	%rsi, %xmm11
	vpblendd	$3, %ymm7, %ymm13, %ymm9
	vpextrq	$1, %xmm1, %r14
	vpbroadcastq	%xmm11, %ymm2
	vpand	%ymm0, %ymm7, %ymm0
	vpermq	$144, %ymm15, %ymm1
	xorq	%r14, %rcx
	vmovdqa	.LC0(%rip), %ymm7
	vpblendd	$3, %ymm2, %ymm1, %ymm8
	vpxor	%ymm9, %ymm0, %ymm3
	xorq	%rcx, %rsi
	vextracti128	$0x1, %ymm15, %xmm12
	vpxor	%ymm8, %ymm10, %ymm10
	vpxor	%ymm3, %ymm14, %ymm14
	rorx	$48, %rsi, %rsi
	vpand	%ymm7, %ymm2, %ymm13
	vpextrq	$1, %xmm12, %r9
	vpxor	%ymm14, %ymm15, %ymm15
	vpxor	%ymm10, %ymm13, %ymm12
	vpxor	.LC27(%rip), %ymm14, %ymm14
	xorq	%r15, %r9
	vpxor	%ymm12, %ymm3, %ymm9
	vpshufd	$177, %ymm12, %ymm13
	xorq	%r9, %r14
	rorx	$32, %r9, %rdx
	vpshufb	.LXRH_rot48(%rip), %ymm9, %ymm11
	rorq	$16, %r14
	vpshufb	.LXRH_rot16(%rip), %ymm15, %ymm12
	vpunpcklqdq	%ymm11, %ymm14, %ymm15
	vpunpckhqdq	%ymm11, %ymm14, %ymm0
	vmovq	%rdx, %xmm1
	vpunpckhqdq	%ymm12, %ymm13, %ymm11
	vpunpcklqdq	%ymm12, %ymm13, %ymm9
	vmovq	%rcx, %xmm8
	vperm2i128	$32, %ymm9, %ymm15, %ymm2
	vperm2i128	$32, %ymm11, %ymm0, %ymm3
	vpinsrq	$1, %rsi, %xmm1, %xmm13
	vpinsrq	$1, %r14, %xmm8, %xmm10
	vperm2i128	$49, %ymm9, %ymm15, %ymm9
	vpxor	%ymm3, %ymm2, %ymm14
	vinserti128	$0x1, %xmm13, %ymm10, %ymm15
	vperm2i128	$49, %ymm11, %ymm0, %ymm0
	vpxor	%ymm5, %ymm14, %ymm10
	vpxor	%ymm15, %ymm2, %ymm14
	vpxor	%ymm0, %ymm10, %ymm1
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm2, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm3, %ymm10
	vpand	%ymm15, %ymm2, %ymm2
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm1, %ymm9, %ymm8
	vpxor	%ymm13, %ymm2, %ymm2
	vpxor	%ymm0, %ymm10, %ymm10
	vpand	%ymm0, %ymm12, %ymm0
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm10, %ymm3, %ymm15
	vpxor	%ymm0, %ymm12, %ymm1
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm9, %ymm1, %ymm14
	vpxor	%ymm0, %ymm3, %ymm13
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm15, %ymm2, %ymm2
	vpxor	%ymm10, %ymm8, %ymm8
	vpunpcklqdq	%ymm2, %ymm11, %ymm9
	vpunpckhqdq	%ymm2, %ymm11, %ymm15
	vpextrq	$1, %xmm8, %rdx
	vpunpckhqdq	%ymm13, %ymm14, %ymm2
	vpunpcklqdq	%ymm13, %ymm14, %ymm11
	vmovq	%xmm8, %r9
	vperm2i128	$32, %ymm11, %ymm9, %ymm0
	vperm2i128	$32, %ymm2, %ymm15, %ymm12
	vperm2i128	$49, %ymm2, %ymm15, %ymm1
	xorq	%rdx, %r9
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	vextracti128	$0x1, %ymm8, %xmm3
	vpxor	%ymm12, %ymm0, %ymm13
	vpxor	%ymm1, %ymm14, %ymm9
	vmovq	%r9, %xmm2
	vpextrq	$1, %xmm3, %rcx
	vpxor	%ymm9, %ymm12, %ymm15
	vpbroadcastq	%xmm2, %ymm0
	vextracti128	$0x1, %ymm13, %xmm11
	vpermq	$144, %ymm13, %ymm12
	vmovq	%xmm3, %r14
	vpextrq	$1, %xmm11, %r15
	xorq	%rcx, %r14
	vpblendd	$3, %ymm0, %ymm12, %ymm14
	vpand	%ymm7, %ymm0, %ymm8
	xorq	%r15, %rcx
	xorq	%r14, %rdx
	vpxor	%ymm14, %ymm1, %ymm1
	vextracti128	$0x1, %ymm15, %xmm3
	vpxor	%ymm1, %ymm8, %ymm10
	vmovq	%rdx, %xmm11
	vmovq	%rcx, %xmm1
	vpbroadcastq	%xmm11, %ymm2
	vpextrq	$1, %xmm3, %rsi
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm1, %ymm3
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm7, %ymm2, %ymm12
	xorq	%rsi, %r9
	vpblendd	$3, %ymm2, %ymm15, %ymm0
	vpblendd	$3, %ymm3, %ymm11, %ymm2
	vpand	%ymm7, %ymm3, %ymm15
	xorq	%r9, %rcx
	vpxor	%ymm0, %ymm12, %ymm14
	vextracti128	$0x1, %ymm10, %xmm8
	vpxor	%ymm2, %ymm9, %ymm9
	vpxor	%ymm9, %ymm15, %ymm12
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm8, %rdx
	xorq	%r14, %rdx
	vpxor	%ymm13, %ymm10, %ymm10
	vpxor	%ymm12, %ymm14, %ymm0
	vpunpcklqdq	%ymm0, %ymm13, %ymm14
	vpunpcklqdq	%ymm10, %ymm12, %ymm11
	vpunpckhqdq	%ymm10, %ymm12, %ymm8
	xorq	%rdx, %rsi
	vpunpckhqdq	%ymm0, %ymm13, %ymm1
	vmovq	%rdx, %xmm15
	vmovq	%r9, %xmm13
	movabsq	$1229782938247303441, %r9
	vperm2i128	$32, %ymm8, %ymm1, %ymm3
	vperm2i128	$32, %ymm11, %ymm14, %ymm2
	vpinsrq	$1, %rsi, %xmm13, %xmm10
	vpinsrq	$1, %rcx, %xmm15, %xmm12
	vpxor	%ymm3, %ymm2, %ymm0
	vperm2i128	$49, %ymm11, %ymm14, %ymm9
	vinserti128	$0x1, %xmm12, %ymm10, %ymm15
	vperm2i128	$49, %ymm8, %ymm1, %ymm1
	vpxor	%ymm5, %ymm0, %ymm10
	vpxor	%ymm15, %ymm2, %ymm14
	vpxor	%ymm1, %ymm10, %ymm0
	vpandn	%ymm15, %ymm9, %ymm11
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpor	%ymm9, %ymm2, %ymm13
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm3, %ymm10
	vpand	%ymm0, %ymm9, %ymm8
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm15, %ymm2, %ymm2
	vpxor	%ymm8, %ymm11, %ymm11
	vpxor	%ymm1, %ymm10, %ymm10
	vpand	%ymm1, %ymm12, %ymm1
	vpor	%ymm14, %ymm0, %ymm12
	vpxor	%ymm10, %ymm8, %ymm8
	vpor	%ymm10, %ymm3, %ymm15
	vpxor	%ymm1, %ymm12, %ymm0
	vpxor	%ymm13, %ymm2, %ymm2
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm9, %ymm0, %ymm14
	vpxor	%ymm1, %ymm3, %ymm13
	vpextrq	$1, %xmm8, %r15
	vpxor	%ymm15, %ymm2, %ymm2
	vpunpcklqdq	%ymm2, %ymm11, %ymm15
	vpunpckhqdq	%ymm13, %ymm14, %ymm12
	vpunpckhqdq	%ymm2, %ymm11, %ymm2
	vextracti128	$0x1, %ymm8, %xmm3
	leaq	(%r15,%r15), %rdx
	shrq	$3, %r15
	vpunpcklqdq	%ymm13, %ymm14, %ymm9
	vperm2i128	$49, %ymm12, %ymm2, %ymm0
	vperm2i128	$32, %ymm12, %ymm2, %ymm14
	andq	%r9, %r15
	andq	%r10, %rdx
	vpand	.LC1(%rip), %ymm14, %ymm13
	vpextrq	$1, %xmm3, %rsi
	vperm2i128	$49, %ymm9, %ymm15, %ymm1
	orq	%r15, %rdx
	vperm2i128	$32, %ymm9, %ymm15, %ymm11
	vmovq	%xmm3, %r14
	vmovq	%xmm8, %rcx
	vpand	.LC2(%rip), %ymm14, %ymm2
	vpsrlq	$3, %ymm13, %ymm15
	leaq	0(,%rsi,8), %r15
	shrq	%rsi
	vpand	.LC3(%rip), %ymm1, %ymm14
	andq	%r13, %r15
	andq	%r12, %rsi
	xorq	%rdx, %rcx
	vpand	.LC5(%rip), %ymm0, %ymm8
	vpand	.LC4(%rip), %ymm1, %ymm1
	orq	%r15, %rsi
	vpand	.LC6(%rip), %ymm0, %ymm0
	vpsllq	$1, %ymm2, %ymm9
	leaq	0(,%r14,4), %r15
	shrq	$2, %r14
	vpor	%ymm15, %ymm9, %ymm12
	vpsrlq	$2, %ymm14, %ymm10
	andq	%r11, %r14
	andq	%rbx, %r15
	vpxor	%ymm12, %ymm11, %ymm11
	vpsllq	$2, %ymm1, %ymm3
	vmovq	%rcx, %xmm1
	orq	%r14, %r15
	vpsrlq	$1, %ymm8, %ymm13
	vpsllq	$3, %ymm0, %ymm15
	vpor	%ymm10, %ymm3, %ymm2
	xorq	%rsi, %r15
	vpor	%ymm13, %ymm15, %ymm9
	vpbroadcastq	%xmm1, %ymm3
	vpermq	$144, %ymm11, %ymm8
	xorq	%r15, %rdx
	vextracti128	$0x1, %ymm11, %xmm10
	vpxor	%ymm9, %ymm2, %ymm14
	vpblendd	$3, %ymm3, %ymm8, %ymm13
	vpextrq	$1, %xmm10, %r14
	vpand	%ymm7, %ymm3, %ymm15
	vpxor	%ymm14, %ymm12, %ymm12
	xorq	%rsi, %r14
	vpxor	%ymm13, %ymm9, %ymm0
	vextracti128	$0x1, %ymm12, %xmm2
	vpxor	%ymm0, %ymm15, %ymm9
	vmovq	%rdx, %xmm10
	vmovq	%r14, %xmm0
	vpbroadcastq	%xmm10, %ymm1
	vpextrq	$1, %xmm2, %rsi
	vpermq	$144, %ymm9, %ymm10
	vpbroadcastq	%xmm0, %ymm2
	vpermq	$144, %ymm12, %ymm12
	vpand	%ymm7, %ymm1, %ymm3
	xorq	%rsi, %rcx
	vpblendd	$3, %ymm1, %ymm12, %ymm8
	vextracti128	$0x1, %ymm9, %xmm15
	vpand	%ymm7, %ymm2, %ymm12
	xorq	%rcx, %r14
	vpblendd	$3, %ymm2, %ymm10, %ymm1
	vpextrq	$1, %xmm15, %rdx
	vpxor	%ymm8, %ymm3, %ymm13
	vpxor	%ymm1, %ymm14, %ymm14
	xorq	%r15, %rdx
	vpxor	%ymm13, %ymm11, %ymm11
	vpxor	%ymm14, %ymm12, %ymm8
	xorq	%rdx, %rsi
	vpxor	%ymm11, %ymm9, %ymm9
	vpxor	%ymm8, %ymm13, %ymm3
	vpand	.LC5(%rip), %ymm3, %ymm13
	vpand	.LC6(%rip), %ymm3, %ymm0
	leaq	0(,%rsi,8), %r15
	shrq	%rsi
	vpand	.LC3(%rip), %ymm8, %ymm1
	vpsrlq	$1, %ymm13, %ymm15
	vpsllq	$3, %ymm0, %ymm2
	andq	%r13, %r15
	andq	%r12, %rsi
	vpand	.LC4(%rip), %ymm8, %ymm14
	leaq	0(,%rdx,4), %r13
	vpor	%ymm15, %ymm2, %ymm10
	orq	%r15, %rsi
	vpand	.LC1(%rip), %ymm9, %ymm13
	vpand	.LC2(%rip), %ymm9, %ymm9
	andq	%rbx, %r13
	shrq	$2, %rdx
	vpxor	.LC28(%rip), %ymm11, %ymm11
	vpsrlq	$2, %ymm1, %ymm12
	andq	%r11, %rdx
	vpsllq	$2, %ymm14, %ymm8
	vpsrlq	$3, %ymm13, %ymm15
	leaq	(%r14,%r14), %r11
	orq	%rdx, %r13
	vpsllq	$1, %ymm9, %ymm0
	shrq	$3, %r14
	vpor	%ymm12, %ymm8, %ymm3
	andq	%r10, %r11
	vpor	%ymm15, %ymm0, %ymm2
	vpunpcklqdq	%ymm10, %ymm11, %ymm1
	vpunpckhqdq	%ymm10, %ymm11, %ymm12
	andq	%r9, %r14
	vpunpckhqdq	%ymm2, %ymm3, %ymm14
	vpunpcklqdq	%ymm2, %ymm3, %ymm10
	vmovq	%r13, %xmm13
	orq	%r14, %r11
	vmovq	%rcx, %xmm15
	vperm2i128	$32, %ymm14, %ymm12, %ymm8
	vperm2i128	$32, %ymm10, %ymm1, %ymm3
	movabsq	$-4222189076152336, %r9
	vpinsrq	$1, %r11, %xmm13, %xmm11
	vpinsrq	$1, %rsi, %xmm15, %xmm2
	vperm2i128	$49, %ymm10, %ymm1, %ymm9
	vinserti128	$0x1, %xmm11, %ymm2, %ymm15
	vpxor	%ymm8, %ymm3, %ymm1
	vperm2i128	$49, %ymm14, %ymm12, %ymm0
	vpxor	%ymm5, %ymm1, %ymm10
	vpxor	%ymm15, %ymm3, %ymm14
	vpandn	%ymm15, %ymm9, %ymm11
	vpxor	%ymm0, %ymm10, %ymm1
	vpand	%ymm14, %ymm10, %ymm10
	vpxor	%ymm15, %ymm9, %ymm12
	vpxor	%ymm10, %ymm11, %ymm11
	vpand	%ymm15, %ymm8, %ymm10
	vpor	%ymm9, %ymm3, %ymm13
	vpxor	%ymm12, %ymm10, %ymm10
	vpand	%ymm15, %ymm3, %ymm3
	vpand	%ymm1, %ymm9, %ymm2
	vpxor	%ymm0, %ymm10, %ymm10
	vpand	%ymm0, %ymm12, %ymm0
	vpor	%ymm14, %ymm1, %ymm12
	vpor	%ymm10, %ymm8, %ymm15
	vpxor	%ymm0, %ymm12, %ymm1
	vpxor	%ymm13, %ymm3, %ymm3
	vpxor	%ymm13, %ymm8, %ymm8
	vpxor	%ymm9, %ymm1, %ymm14
	vpxor	%ymm2, %ymm11, %ymm11
	vpxor	%ymm0, %ymm8, %ymm13
	vpxor	%ymm10, %ymm2, %ymm2
	vpxor	%ymm15, %ymm3, %ymm3
	vpunpcklqdq	%ymm3, %ymm11, %ymm9
	vpunpckhqdq	%ymm3, %ymm11, %ymm15
	vpextrq	$1, %xmm2, %rbx
	vpunpckhqdq	%ymm13, %ymm14, %ymm3
	vpunpcklqdq	%ymm13, %ymm14, %ymm11
	vmovq	%xmm2, %r10
	vperm2i128	$32, %ymm11, %ymm9, %ymm0
	vperm2i128	$32, %ymm3, %ymm15, %ymm12
	vperm2i128	$49, %ymm3, %ymm15, %ymm1
	xorq	%rbx, %r10
	vperm2i128	$49, %ymm11, %ymm9, %ymm14
	vextracti128	$0x1, %ymm2, %xmm8
	vpxor	%ymm12, %ymm0, %ymm13
	vpxor	%ymm1, %ymm14, %ymm9
	vmovq	%r10, %xmm3
	vpextrq	$1, %xmm8, %r14
	vpxor	%ymm9, %ymm12, %ymm15
	vpbroadcastq	%xmm3, %ymm0
	vextracti128	$0x1, %ymm13, %xmm11
	vpermq	$144, %ymm13, %ymm12
	vmovq	%xmm8, %r12
	vpextrq	$1, %xmm11, %rcx
	xorq	%r14, %r12
	vpblendd	$3, %ymm0, %ymm12, %ymm14
	vpand	%ymm7, %ymm0, %ymm2
	xorq	%rcx, %r14
	xorq	%r12, %rbx
	vpxor	%ymm14, %ymm1, %ymm1
	vextracti128	$0x1, %ymm15, %xmm8
	vpxor	%ymm1, %ymm2, %ymm10
	vmovq	%rbx, %xmm11
	vmovq	%r14, %xmm1
	movabsq	$1152657617789587455, %rbx
	vpbroadcastq	%xmm11, %ymm3
	vpextrq	$1, %xmm8, %rsi
	vpermq	$144, %ymm10, %ymm11
	vpbroadcastq	%xmm1, %ymm8
	vpermq	$144, %ymm15, %ymm15
	vpand	%ymm7, %ymm3, %ymm12
	xorq	%rsi, %r10
	vpblendd	$3, %ymm3, %ymm15, %ymm0
	vpblendd	$3, %ymm8, %ymm11, %ymm3
	vpand	%ymm7, %ymm8, %ymm15
	xorq	%r10, %r14
	vpxor	%ymm0, %ymm12, %ymm14
	vextracti128	$0x1, %ymm10, %xmm2
	vpxor	%ymm3, %ymm9, %ymm9
	vpxor	%ymm9, %ymm15, %ymm12
	vpxor	%ymm14, %ymm13, %ymm13
	vpextrq	$1, %xmm2, %rdx
	xorq	%rdx, %r12
	vpxor	%ymm13, %ymm10, %ymm2
	vpxor	%ymm12, %ymm14, %ymm0
	movabsq	$71777214294589695, %rdx
	vpunpcklqdq	%ymm0, %ymm13, %ymm10
	vpunpckhqdq	%ymm0, %ymm13, %ymm14
	vpunpcklqdq	%ymm2, %ymm12, %ymm1
	xorq	%r12, %rsi
	vpunpckhqdq	%ymm2, %ymm12, %ymm11
	vmovq	%r12, %xmm3
	vmovq	%r10, %xmm13
	vperm2i128	$32, %ymm1, %ymm10, %ymm8
	vperm2i128	$32, %ymm11, %ymm14, %ymm15
	vpinsrq	$1, %r14, %xmm3, %xmm12
	vpinsrq	$1, %rsi, %xmm13, %xmm2
	vperm2i128	$49, %ymm1, %ymm10, %ymm9
	vpxor	%ymm15, %ymm8, %ymm0
	movabsq	$-71777214294589696, %rsi
	vinserti128	$0x1, %xmm12, %ymm2, %ymm1
	vperm2i128	$49, %ymm11, %ymm14, %ymm10
	vpxor	%ymm5, %ymm0, %ymm5
	vpxor	%ymm1, %ymm8, %ymm14
	vpxor	%ymm10, %ymm5, %ymm0
	vpandn	%ymm1, %ymm9, %ymm11
	vpand	%ymm14, %ymm5, %ymm3
	vpxor	%ymm1, %ymm9, %ymm12
	vpand	%ymm0, %ymm9, %ymm2
	vpxor	%ymm3, %ymm11, %ymm5
	vpand	%ymm1, %ymm15, %ymm3
	vpor	%ymm9, %ymm8, %ymm13
	vpxor	%ymm2, %ymm5, %ymm11
	vpxor	%ymm12, %ymm3, %ymm5
	vpand	%ymm1, %ymm8, %ymm8
	vpxor	%ymm10, %ymm5, %ymm5
	vpand	%ymm10, %ymm12, %ymm10
	vpor	%ymm14, %ymm0, %ymm12
	vpxor	%ymm5, %ymm2, %ymm2
	vpor	%ymm5, %ymm15, %ymm3
	vpxor	%ymm10, %ymm12, %ymm0
	vpxor	%ymm13, %ymm8, %ymm1
	vpxor	%ymm13, %ymm15, %ymm15
	vpxor	%ymm9, %ymm0, %ymm14
	vpxor	%ymm3, %ymm1, %ymm8
	vpxor	%ymm10, %ymm15, %ymm9
	vpextrq	$1, %xmm2, %r13
	vpunpcklqdq	%ymm8, %ymm11, %ymm13
	vpunpckhqdq	%ymm8, %ymm11, %ymm11
	vpunpckhqdq	%ymm9, %ymm14, %ymm8
	movq	%r13, %r14
	vpunpcklqdq	%ymm9, %ymm14, %ymm3
	vperm2i128	$32, %ymm8, %ymm11, %ymm10
	salq	$4, %r14
	vperm2i128	$49, %ymm8, %ymm11, %ymm0
	vextracti128	$0x1, %ymm2, %xmm14
	shrq	$12, %r13
	vperm2i128	$49, %ymm3, %ymm13, %ymm12
	andq	%r9, %r14
	vpand	.LC8(%rip), %ymm10, %ymm15
	andq	%rdi, %r13
	vperm2i128	$32, %ymm3, %ymm13, %ymm1
	movabsq	$1152657617789587455, %rdi
	vpand	.LC9(%rip), %ymm10, %ymm9
	vmovq	%xmm14, %r11
	orq	%r13, %r14
	vpextrq	$1, %xmm14, %r12
	vmovq	%xmm2, %r15
	movq	%r11, %rcx
	vpsrlq	$12, %ymm15, %ymm13
	vpsllq	$4, %ymm9, %ymm11
	xorq	%r14, %r15
	movq	%r12, %r10
	salq	$8, %rcx
	vpor	%ymm13, %ymm11, %ymm10
	shrq	$8, %r11
	vpand	%ymm4, %ymm0, %ymm5
	vpshufb	.LXRH_bswap16(%rip), %ymm12, %ymm9
	andq	%rdx, %r11
	andq	%rsi, %rcx
	vpxor	%ymm10, %ymm1, %ymm1
	vpand	%ymm6, %ymm0, %ymm0
	salq	$12, %r10
	vmovq	%r15, %xmm2
	shrq	$4, %r12
	orq	%r11, %rcx
	andq	%r8, %r10
	andq	%rbx, %r12
	vpbroadcastq	%xmm2, %ymm8
	vpermq	$144, %ymm1, %ymm12
	movq	%rcx, %r13
	vpsrlq	$4, %ymm5, %ymm14
	vpsllq	$12, %ymm0, %ymm15
	orq	%r12, %r10
	movabsq	$-71777214294589696, %rbx
	vpor	%ymm14, %ymm15, %ymm13
	vpblendd	$3, %ymm8, %ymm12, %ymm5
	vpand	%ymm7, %ymm8, %ymm14
	xorq	%r10, %r13
	vextracti128	$0x1, %ymm1, %xmm3
	vpxor	%ymm13, %ymm9, %ymm11
	vpxor	%ymm5, %ymm13, %ymm0
	movabsq	$71777214294589695, %rdx
	vpextrq	$1, %xmm3, %r11
	vpxor	%ymm0, %ymm14, %ymm15
	vpxor	%ymm11, %ymm10, %ymm10
	xorq	%r11, %r10
	vextracti128	$0x1, %ymm10, %xmm13
	vextracti128	$0x1, %ymm15, %xmm5
	xorq	%r13, %r14
	vmovq	%r14, %xmm9
	vmovq	%r10, %xmm14
	vpermq	$144, %ymm10, %ymm10
	movq	%r10, %rcx
	vpbroadcastq	%xmm9, %ymm3
	vpbroadcastq	%xmm14, %ymm0
	vpextrq	$1, %xmm13, %r12
	movabsq	$-4222189076152336, %r11
	vpextrq	$1, %xmm5, %r14
	vpermq	$144, %ymm15, %ymm13
	vpand	%ymm7, %ymm3, %ymm2
	xorq	%r12, %r15
	xorq	%r14, %r13
	vpblendd	$3, %ymm3, %ymm10, %ymm8
	vpand	%ymm7, %ymm0, %ymm7
	xorq	%r15, %rcx
	vpblendd	$3, %ymm0, %ymm13, %ymm9
	vpxor	%ymm8, %ymm2, %ymm12
	xorq	%r13, %r12
	movq	%r13, %r10
	vpxor	%ymm9, %ymm11, %ymm11
	vpxor	%ymm12, %ymm1, %ymm1
	movq	%r12, %r9
	movq	%r13, %rsi
	vpxor	%ymm11, %ymm7, %ymm3
	vpxor	%ymm1, %ymm15, %ymm15
	salq	$12, %r9
	movq	%rcx, %r13
	vpxor	%ymm3, %ymm12, %ymm10
	shrq	$4, %r12
	andq	%r9, %r8
	andq	%rdi, %r12
	vpand	%ymm4, %ymm10, %ymm4
	vpand	%ymm6, %ymm10, %ymm6
	salq	$8, %r10
	shrq	$8, %rsi
	orq	%r12, %r8
	andq	%rbx, %r10
	salq	$4, %r13
	andq	%rdx, %rsi
	vpsrlq	$4, %ymm4, %ymm8
	shrq	$12, %rcx
	andq	%r11, %r13
	vpand	.LC8(%rip), %ymm15, %ymm7
	orq	%rsi, %r10
	movabsq	$4222189076152335, %r12
	vpshufb	.LXRH_bswap16(%rip), %ymm3, %ymm9
	vpand	.LC9(%rip), %ymm15, %ymm3
	vpxor	.LC29(%rip), %ymm1, %ymm1
	andq	%r12, %rcx
	vpsllq	$12, %ymm6, %ymm2
	vmovq	%r10, %xmm5
	orq	%r13, %rcx
	vpsrlq	$12, %ymm7, %ymm11
	vpor	%ymm8, %ymm2, %ymm12
	vpsllq	$4, %ymm3, %ymm15
	vpunpcklqdq	%ymm12, %ymm1, %ymm8
	vpor	%ymm11, %ymm15, %ymm10
	vpunpckhqdq	%ymm12, %ymm1, %ymm6
	vmovq	%r15, %xmm7
	vpunpcklqdq	%ymm10, %ymm9, %ymm2
	vpunpckhqdq	%ymm10, %ymm9, %ymm12
	vpinsrq	$1, %rcx, %xmm5, %xmm0
	vperm2i128	$32, %ymm12, %ymm6, %ymm14
	vperm2i128	$32, %ymm2, %ymm8, %ymm4
	vpinsrq	$1, %r8, %xmm7, %xmm11
	vinserti128	$0x1, %xmm0, %ymm11, %ymm3
	vperm2i128	$49, %ymm2, %ymm8, %ymm13
	vpxor	%ymm14, %ymm4, %ymm10
	vpcmpeqd	%ymm2, %ymm2, %ymm2
	vperm2i128	$49, %ymm12, %ymm6, %ymm9
	vpxor	%ymm3, %ymm4, %ymm7
	vpxor	%ymm2, %ymm10, %ymm6
	vpxor	%ymm3, %ymm13, %ymm15
	vpandn	%ymm3, %ymm13, %ymm12
	vpxor	%ymm9, %ymm6, %ymm1
	vpand	%ymm7, %ymm6, %ymm5
	vpand	%ymm3, %ymm14, %ymm6
	vpand	%ymm1, %ymm13, %ymm0
	vpxor	%ymm5, %ymm12, %ymm11
	vpxor	%ymm15, %ymm6, %ymm12
	vpor	%ymm13, %ymm4, %ymm8
	vpxor	%ymm0, %ymm11, %ymm10
	vpand	%ymm3, %ymm4, %ymm4
	vpxor	%ymm9, %ymm12, %ymm11
	vpand	%ymm9, %ymm15, %ymm9
	vpor	%ymm7, %ymm1, %ymm15
	vpxor	%ymm8, %ymm4, %ymm3
	vpor	%ymm11, %ymm14, %ymm5
	vpxor	%ymm9, %ymm15, %ymm1
	vpxor	%ymm8, %ymm14, %ymm14
	vpxor	%ymm5, %ymm3, %ymm6
	vpxor	%ymm11, %ymm0, %ymm0
	vpxor	%ymm9, %ymm14, %ymm12
	vpxor	%ymm13, %ymm1, %ymm13
	vpunpcklqdq	%ymm6, %ymm10, %ymm7
	vpunpcklqdq	%ymm12, %ymm13, %ymm8
	vpunpckhqdq	%ymm12, %ymm13, %ymm5
	vpunpckhqdq	%ymm6, %ymm10, %ymm10
	vpextrq	$1, %xmm0, %r14
	vmovq	%xmm0, %r15
	vperm2i128	$32, %ymm5, %ymm10, %ymm3
	vperm2i128	$49, %ymm5, %ymm10, %ymm9
	vperm2i128	$32, %ymm8, %ymm7, %ymm4
	vperm2i128	$49, %ymm8, %ymm7, %ymm6
	xorq	%r14, %r15
	vpxor	%ymm3, %ymm4, %ymm13
	vextracti128	$0x1, %ymm0, %xmm15
	vpxor	%ymm9, %ymm6, %ymm14
	vmovq	%r15, %xmm7
	vpxor	%ymm14, %ymm3, %ymm1
	vpextrq	$1, %xmm15, %r9
	vmovdqa	.LC0(%rip), %ymm3
	vpbroadcastq	%xmm7, %ymm10
	vextracti128	$0x1, %ymm13, %xmm12
	vpermq	$144, %ymm13, %ymm8
	vmovq	%xmm15, %rcx
	vpblendd	$3, %ymm10, %ymm8, %ymm5
	vpextrq	$1, %xmm12, %r8
	vpand	%ymm3, %ymm10, %ymm4
	vpxor	%ymm5, %ymm9, %ymm6
	vextracti128	$0x1, %ymm1, %xmm0
	xorq	%r9, %rcx
	xorq	%rcx, %r14
	vpxor	%ymm6, %ymm4, %ymm9
	vpermq	$144, %ymm1, %ymm1
	xorq	%r8, %r9
	vmovq	%r14, %xmm11
	vmovq	%r9, %xmm5
	vpermq	$144, %ymm9, %ymm6
	movq	32(%rax), %r8
	vpbroadcastq	%xmm11, %ymm15
	vpbroadcastq	%xmm5, %ymm4
	vpextrq	$1, %xmm0, %r10
	vpand	%ymm3, %ymm15, %ymm7
	vpand	%ymm3, %ymm4, %ymm11
	vextracti128	$0x1, %ymm9, %xmm8
	xorq	%r10, %r15
	vpblendd	$3, %ymm15, %ymm1, %ymm12
	vpblendd	$3, %ymm4, %ymm6, %ymm0
	vpextrq	$1, %xmm8, %rdi
	xorq	%r15, %r9
	vpxor	%ymm12, %ymm7, %ymm10
	vpxor	%ymm0, %ymm14, %ymm14
	vmovq	%r15, %xmm0
	xorq	%rcx, %rdi
	vpxor	%ymm14, %ymm11, %ymm15
	vpxor	%ymm10, %ymm13, %ymm13
	vmovq	%rdi, %xmm6
	xorq	%rdi, %r10
	vpxor	%ymm13, %ymm9, %ymm1
	vpxor	%ymm15, %ymm10, %ymm12
	vpinsrq	$1, %r9, %xmm6, %xmm11
	vpunpcklqdq	%ymm12, %ymm13, %ymm9
	vpunpckhqdq	%ymm12, %ymm13, %ymm8
	vpunpcklqdq	%ymm1, %ymm15, %ymm5
	vpunpckhqdq	%ymm1, %ymm15, %ymm4
	vperm2i128	$32, %ymm5, %ymm9, %ymm10
	vpinsrq	$1, %r10, %xmm0, %xmm13
	vperm2i128	$32, %ymm4, %ymm8, %ymm7
	vperm2i128	$49, %ymm5, %ymm9, %ymm14
	vperm2i128	$49, %ymm4, %ymm8, %ymm15
	vinserti128	$0x1, %xmm11, %ymm13, %ymm9
	vpxor	%ymm7, %ymm10, %ymm8
	vpor	%ymm14, %ymm10, %ymm1
	vpxor	%ymm2, %ymm8, %ymm5
	vpxor	%ymm9, %ymm10, %ymm11
	vpxor	%ymm9, %ymm14, %ymm12
	vpxor	%ymm15, %ymm5, %ymm4
	vpandn	%ymm9, %ymm14, %ymm6
	vpand	%ymm11, %ymm5, %ymm0
	vpand	%ymm9, %ymm7, %ymm8
	vpand	%ymm4, %ymm14, %ymm2
	vpxor	%ymm0, %ymm6, %ymm13
	vpxor	%ymm12, %ymm8, %ymm5
	vpxor	%ymm2, %ymm13, %ymm6
	vpand	%ymm9, %ymm10, %ymm10
	vpxor	%ymm15, %ymm5, %ymm13
	vpand	%ymm15, %ymm12, %ymm15
	vpor	%ymm11, %ymm4, %ymm12
	vpxor	%ymm1, %ymm10, %ymm9
	vpor	%ymm13, %ymm7, %ymm0
	vpxor	%ymm15, %ymm12, %ymm4
	vpxor	%ymm1, %ymm7, %ymm7
	vpxor	%ymm0, %ymm9, %ymm8
	vpxor	%ymm14, %ymm4, %ymm14
	vpxor	%ymm15, %ymm7, %ymm1
	vpunpcklqdq	%ymm8, %ymm6, %ymm11
	vpunpckhqdq	%ymm8, %ymm6, %ymm6
	vpunpckhqdq	%ymm1, %ymm14, %ymm10
	vpxor	%ymm13, %ymm2, %ymm2
	vpunpcklqdq	%ymm1, %ymm14, %ymm5
	vperm2i128	$32, %ymm10, %ymm6, %ymm8
	vmovq	%xmm2, %r13
	vpextrq	$1, %xmm2, %rsi
	vperm2i128	$49, %ymm5, %ymm11, %ymm15
	vperm2i128	$49, %ymm10, %ymm6, %ymm12
	vperm2i128	$32, %ymm5, %ymm11, %ymm9
	rorx	$48, %rsi, %rdx
	vpshufb	.LXRH_rot16(%rip), %ymm8, %ymm7
	xorq	%rdx, %r13
	vextracti128	$0x1, %ymm2, %xmm0
	vmovq	%r13, %xmm13
	movq	%r13, %rdi
	vpxor	%ymm7, %ymm9, %ymm9
	vpshufd	$177, %ymm15, %ymm8
	vmovq	%xmm0, %r11
	vpshufb	.LXRH_rot48(%rip), %ymm12, %ymm10
	vextracti128	$0x1, %ymm9, %xmm2
	rorx	$32, %r11, %r14
	vpextrq	$1, %xmm0, %r12
	vpbroadcastq	%xmm13, %ymm0
	vpermq	$144, %ymm9, %ymm4
	rorx	$16, %r12, %r15
	vpxor	%ymm10, %ymm8, %ymm15
	xorq	%r15, %r14
	vpblendd	$3, %ymm0, %ymm4, %ymm14
	vpextrq	$1, %xmm2, %r9
	vpxor	%ymm15, %ymm7, %ymm12
	vpxor	%ymm14, %ymm10, %ymm1
	vpand	%ymm3, %ymm0, %ymm7
	vextracti128	$0x1, %ymm12, %xmm6
	vpermq	$144, %ymm12, %ymm8
	xorq	%r15, %r9
	xorq	%r14, %rdx
	vpxor	%ymm1, %ymm7, %ymm11
	vmovq	%r9, %xmm4
	vmovq	%rdx, %xmm5
	vpbroadcastq	%xmm4, %ymm14
	vpermq	$144, %ymm11, %ymm7
	vpbroadcastq	%xmm5, %ymm10
	vpextrq	$1, %xmm6, %rcx
	vextracti128	$0x1, %ymm11, %xmm0
	vpblendd	$3, %ymm10, %ymm8, %ymm12
	vpand	%ymm3, %ymm10, %ymm2
	vpand	%ymm3, %ymm14, %ymm3
	xorq	%rcx, %rdi
	vpblendd	$3, %ymm14, %ymm7, %ymm6
	vpxor	%ymm12, %ymm2, %ymm13
	xorq	%rdi, %r9
	xorq	%rdi, %r8
	vpxor	%ymm6, %ymm15, %ymm15
	vpxor	%ymm13, %ymm9, %ymm9
	rorx	$48, %r9, %r11
	movq	%r8, 32(%rax)
	vpextrq	$1, %xmm0, %r10
	vpxor	%ymm15, %ymm3, %ymm1
	vpxor	%ymm9, %ymm11, %ymm11
	vmovdqu	(%rax), %ymm0
	xorq	%r14, %r10
	vpxor	%ymm1, %ymm13, %ymm5
	vpand	.LC33(%rip), %ymm0, %ymm4
	xorq	152(%rax), %r11
	vpxor	.LC30(%rip), %ymm4, %ymm7
	xorq	%r10, %rcx
	rorx	$32, %r10, %r13
	xorq	112(%rax), %r13
	vpshufb	.LXRH_rot48(%rip), %ymm5, %ymm3
	rorx	$16, %rcx, %rbx
	xorq	72(%rax), %rbx
	vpxor	%ymm9, %ymm7, %ymm6
	movq	%rbx, 72(%rax)
	movq	%r13, 112(%rax)
	vpshufd	$177, %ymm1, %ymm1
	vpxor	40(%rax), %ymm3, %ymm15
	vpxor	80(%rax), %ymm1, %ymm9
	movq	%r11, 152(%rax)
	vpshufb	.LXRH_rot16(%rip), %ymm11, %ymm11
	vpxor	120(%rax), %ymm11, %ymm5
	vmovdqu	%ymm6, (%rax)
	vmovdqu	%ymm15, 40(%rax)
	vmovdqu	%ymm9, 80(%rax)
	vmovdqu	%ymm5, 120(%rax)
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	ret
	.size	XRH_dmc_permute, .-XRH_dmc_permute
	.p2align 4
	.globl	CryptHash
	.type	CryptHash, @function
CryptHash:
	pushq	%rbp
	vpxor	%xmm0, %xmm0, %xmm0
	movq	%rsp, %rbp
	pushq	%r15
	movslq	%edi, %r15
	pushq	%r14
	movq	%rdx, %r14
	pushq	%r13
	movq	%rcx, %r13
	pushq	%r12
	pushq	%rbx
	movq	%rsi, %rbx
	subq	$200, %rsp
	xorl	%eax, %eax
	leaq	16(%rsp), %r12
	vmovdqu	%ymm0, 16(%rsp)
	movl	$192, 176(%rsp)
	vmovdqu	%ymm0, 48(%rsp)
	vmovdqu	%ymm0, 80(%rsp)
	vmovdqu	%ymm0, 112(%rsp)
	vmovdqu	%ymm0, 144(%rsp)
	cmpq	$191, %rdx
	jbe	.L141
	leaq	-192(%rdx), %rcx
	movabsq	$-6148914691236517205, %rax
	mulq	%rcx
	shrq	$7, %rdx
	leaq	1(%rdx), %rsi
	andl	$3, %esi
	je	.L137
	cmpq	$1, %rsi
	je	.L138
	cmpq	$2, %rsi
	je	.L139
	movq	16(%rbx), %rdi
	vmovdqu	(%rbx), %xmm7
	movq	%rcx, 8(%rsp)
	xorq	%rdi, 32(%rsp)
	vpxor	16(%rsp), %xmm7, %xmm1
	movq	%r12, %rdi
	vmovdqa	%xmm1, 16(%rsp)
	call	XRH_dmc_permute
	movq	8(%rsp), %r14
	addq	$24, %rbx
.L113:
	vmovdqu	(%rbx), %xmm2
	movq	%r12, %rdi
	movq	16(%rbx), %r8
	addq	$24, %rbx
	vpxor	16(%rsp), %xmm2, %xmm3
	xorq	%r8, 32(%rsp)
	subq	$192, %r14
	vmovdqa	%xmm3, 16(%rsp)
	call	XRH_dmc_permute
.L112:
	vmovdqu	(%rbx), %xmm6
	movq	%r12, %rdi
	movq	16(%rbx), %r9
	addq	$24, %rbx
	vpxor	16(%rsp), %xmm6, %xmm4
	subq	$192, %r14
	xorq	%r9, 32(%rsp)
	vmovdqa	%xmm4, 16(%rsp)
	call	XRH_dmc_permute
	cmpq	$191, %r14
	jbe	.L44
	.p2align 4,,10
	.p2align 3
.L45:
	vmovdqu	(%rbx), %xmm5
	movq	%r12, %rdi
	movq	16(%rbx), %r10
	addq	$96, %rbx
	vpxor	16(%rsp), %xmm5, %xmm8
	xorq	%r10, 32(%rsp)
	subq	$768, %r14
	vmovdqa	%xmm8, 16(%rsp)
	call	XRH_dmc_permute
	vmovdqu	-72(%rbx), %xmm9
	movq	%r12, %rdi
	movq	-56(%rbx), %r11
	vpxor	16(%rsp), %xmm9, %xmm10
	xorq	%r11, 32(%rsp)
	vmovdqa	%xmm10, 16(%rsp)
	call	XRH_dmc_permute
	vmovdqu	-48(%rbx), %xmm11
	movq	%r12, %rdi
	movq	-32(%rbx), %rcx
	vpxor	16(%rsp), %xmm11, %xmm12
	xorq	%rcx, 32(%rsp)
	vmovdqa	%xmm12, 16(%rsp)
	call	XRH_dmc_permute
	vmovdqu	-24(%rbx), %xmm13
	movq	%r12, %rdi
	movq	-8(%rbx), %rax
	vpxor	16(%rsp), %xmm13, %xmm14
	xorq	%rax, 32(%rsp)
	vmovdqa	%xmm14, 16(%rsp)
	call	XRH_dmc_permute
	cmpq	$191, %r14
	ja	.L45
.L44:
	movq	%r14, %rdi
	movq	%r14, %r8
	shrq	$3, %rdi
	andl	$7, %r8d
	cmpq	$63, %r14
	jbe	.L46
	movq	(%rbx), %rsi
	xorq	%rsi, 16(%rsp)
	cmpq	$15, %rdi
	jbe	.L67
	movq	8(%rbx), %r9
	xorq	%r9, 24(%rsp)
	movl	$16, %r11d
.L47:
	cmpq	%rdi, %r11
	jnb	.L49
	movq	%rdi, %rdx
	subq	%r11, %rdx
	leaq	-1(%rdx), %rax
	cmpq	$6, %rax
	jbe	.L69
	leaq	(%r12,%r11), %r9
	vmovq	(%rbx,%r11), %xmm15
	movq	%rdx, %r10
	vmovq	(%r9), %xmm0
	andq	$-8, %r10
	leaq	(%r10,%r11), %rsi
	vpxor	%xmm15, %xmm0, %xmm7
	vmovq	%xmm7, (%r9)
	testb	$7, %dl
	je	.L50
.L48:
	subq	%r10, %rdx
	leaq	-1(%rdx), %rcx
	cmpq	$2, %rcx
	jbe	.L53
	addq	%r10, %r11
	movl	(%r12,%r11), %r10d
	movl	(%rbx,%r11), %eax
	xorl	%eax, %r10d
	movl	%r10d, (%r12,%r11)
	movq	%rdx, %r11
	andq	$-4, %r11
	addq	%r11, %rsi
	andl	$3, %edx
	je	.L50
.L53:
	leaq	1(%rsi), %r9
	movzbl	(%rbx,%rsi), %edx
	xorb	%dl, (%r12,%rsi)
	cmpq	%rdi, %r9
	jnb	.L50
	leaq	2(%rsi), %rax
	movzbl	1(%rbx,%rsi), %ecx
	xorb	%cl, (%r12,%r9)
	cmpq	%rdi, %rax
	jnb	.L50
	movzbl	2(%rbx,%rsi), %esi
	xorb	%sil, (%r12,%rax)
.L50:
	leaq	(%r12,%rdi), %r10
	movzbl	(%r10), %ecx
	testq	%r8, %r8
	je	.L52
.L65:
	movl	$8, %r11d
	movl	$255, %edx
	subl	%r8d, %r11d
	shlx	%r11d, %edx, %r9d
	andb	(%rbx,%rdi), %r9b
	xorl	%r9d, %ecx
.L52:
	movl	$128, %ebx
	shrx	%r8d, %ebx, %edi
	xorl	%ecx, %edi
	movb	%dil, (%r10)
	cmpq	$191, %r14
	je	.L142
.L55:
	movq	%r12, %rdi
	xorb	$1, 39(%rsp)
	xorl	%ebx, %ebx
	call	XRH_dmc_permute
	testq	%r15, %r15
	je	.L63
.L56:
	movq	%r15, %rcx
	movl	$192, %r14d
	movq	%rbx, %r10
	subq	%rbx, %rcx
	cmpq	%r14, %rcx
	cmova	%r14, %rcx
	shrq	$3, %r10
	addq	%r13, %r10
	movq	%rcx, %rax
	shrq	$3, %rax
	cmpl	$8, %eax
	jnb	.L57
	testb	$4, %al
	jne	.L143
	testl	%eax, %eax
	je	.L58
	movzbl	(%r12), %edx
	movb	%dl, (%r10)
	testb	$2, %al
	jne	.L144
.L58:
	addq	%rcx, %rbx
	cmpq	%r15, %rbx
	jb	.L145
.L63:
	addq	$200, %rsp
	xorl	%eax, %eax
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	ret
.L138:
	jmp	.L112
	.p2align 4,,10
	.p2align 3
.L57:
	movl	%eax, %r14d
	decl	%eax
	movq	-8(%r12,%r14), %r8
	movq	%r8, -8(%r10,%r14)
	cmpl	$8, %eax
	jb	.L58
	andl	$-8, %eax
	movq	(%r12), %rsi
	movl	$8, %r9d
	leal	-1(%rax), %r11d
	shrl	$3, %r11d
	movq	%rsi, (%r10)
	andl	$7, %r11d
	cmpl	%eax, %r9d
	jnb	.L58
	testl	%r11d, %r11d
	je	.L61
	cmpl	$1, %r11d
	je	.L115
	cmpl	$2, %r11d
	je	.L116
	cmpl	$3, %r11d
	je	.L117
	cmpl	$4, %r11d
	je	.L118
	cmpl	$5, %r11d
	je	.L119
	cmpl	$6, %r11d
	je	.L120
	movq	(%r12,%r9), %rdx
	movq	%rdx, (%r10,%r9)
	movl	$16, %r9d
.L120:
	movl	%r9d, %edi
	addl	$8, %r9d
	movq	(%r12,%rdi), %r14
	movq	%r14, (%r10,%rdi)
.L119:
	movl	%r9d, %r11d
	addl	$8, %r9d
	movq	(%r12,%r11), %r8
	movq	%r8, (%r10,%r11)
.L118:
	movl	%r9d, %esi
	addl	$8, %r9d
	movq	(%r12,%rsi), %rdx
	movq	%rdx, (%r10,%rsi)
.L117:
	movl	%r9d, %edi
	addl	$8, %r9d
	movq	(%r12,%rdi), %r14
	movq	%r14, (%r10,%rdi)
.L116:
	movl	%r9d, %r11d
	addl	$8, %r9d
	movq	(%r12,%r11), %r8
	movq	%r8, (%r10,%r11)
.L115:
	movl	%r9d, %esi
	addl	$8, %r9d
	movq	(%r12,%rsi), %rdx
	movq	%rdx, (%r10,%rsi)
	cmpl	%eax, %r9d
	jnb	.L58
.L61:
	movl	%r9d, %edi
	leal	8(%r9), %r11d
	leal	16(%r9), %esi
	movq	(%r12,%rdi), %r14
	movq	%r14, (%r10,%rdi)
	movq	(%r12,%r11), %r8
	leal	24(%r9), %edi
	movq	%r8, (%r10,%r11)
	movq	(%r12,%rsi), %rdx
	leal	32(%r9), %r11d
	movq	%rdx, (%r10,%rsi)
	movq	(%r12,%rdi), %r14
	leal	40(%r9), %esi
	movq	%r14, (%r10,%rdi)
	movq	(%r12,%r11), %r8
	leal	48(%r9), %edi
	movq	%r8, (%r10,%r11)
	movq	(%r12,%rsi), %rdx
	leal	56(%r9), %r11d
	addl	$64, %r9d
	movq	%rdx, (%r10,%rsi)
	movq	(%r12,%rdi), %r14
	movq	%r14, (%r10,%rdi)
	movq	(%r12,%r11), %r8
	movq	%r8, (%r10,%r11)
	cmpl	%eax, %r9d
	jb	.L61
	addq	%rcx, %rbx
	cmpq	%r15, %rbx
	jnb	.L63
	.p2align 4,,10
	.p2align 3
.L145:
	movq	%r12, %rdx
	movl	$18, %esi
	xorl	%edi, %edi
	call	XRH1280
	jmp	.L56
.L143:
	movl	(%r12), %r8d
	movl	%eax, %r11d
	movl	%r8d, (%r10)
	movl	-4(%r12,%r11), %esi
	movl	%esi, -4(%r10,%r11)
	jmp	.L58
.L139:
	jmp	.L113
.L137:
	jmp	.L45
.L142:
	movq	%r12, %rdi
	call	XRH_dmc_permute
	jmp	.L55
.L144:
	movl	%eax, %r9d
	movzwl	-2(%r12,%r9), %edi
	movw	%di, -2(%r10,%r9)
	jmp	.L58
.L49:
	leaq	(%r12,%rdi), %r10
	movzbl	(%r10), %ecx
	testq	%r8, %r8
	jne	.L65
	addl	$-128, %ecx
	movb	%cl, (%r10)
	jmp	.L55
.L46:
	testq	%rdi, %rdi
	je	.L49
	movq	%rdi, %rdx
	xorl	%r11d, %r11d
	xorl	%r10d, %r10d
	xorl	%esi, %esi
	jmp	.L48
.L141:
	jmp	.L44
.L67:
	movl	$8, %r11d
	jmp	.L47
.L69:
	movq	%r11, %rsi
	xorl	%r10d, %r10d
	jmp	.L48
	.size	CryptHash, .-CryptHash
	.section	.rodata
	.align 32
	.type	XRH_ROUND_CONSTANTS, @object
	.size	XRH_ROUND_CONSTANTS, 192
XRH_ROUND_CONSTANTS:
	.quad	2611923443488327891
	.quad	1376283091369227076
	.quad	-6626703657320631856
	.quad	589684135938649225
	.quad	4983270260364809079
	.quad	-4732044268327596948
	.quad	-4563226453097033507
	.quad	4577018097722394903
	.quad	-7919907764393346277
	.quad	-3372901835766516308
	.quad	3458046377305235383
	.quad	-5124621466747896170
	.quad	-5008970055469465703
	.quad	2639559389850201335
	.quad	577009281997405206
	.quad	7163292796296056425
	.quad	-6604248873402417794
	.quad	978816653474051672
	.quad	8181858928071887598
	.quad	8886908412430539189
	.quad	-7192014163400302573
	.quad	-4192376113057462800
	.quad	-3872681057474365201
	.quad	-8180264598055020530
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
	.align 32
.LXRH_bswap16:
	.byte 1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14
	.byte 1,0,3,2,5,4,7,6,9,8,11,10,13,12,15,14
.LXRH_rot16:
	.byte 6,7,0,1,2,3,4,5, 14,15,8,9,10,11,12,13
	.byte 6,7,0,1,2,3,4,5, 14,15,8,9,10,11,12,13
	.align 32
.LXRH_rot48:
	.byte 2,3,4,5,6,7,0,1, 10,11,12,13,14,15,8,9
	.byte 2,3,4,5,6,7,0,1, 10,11,12,13,14,15,8,9
	.ident	"GCC: (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0"
	.section	.note.GNU-stack,"",@progbits
	.section	.note.gnu.property,"a"
	.align 8
	.long	1f - 0f
	.long	4f - 1f
	.long	5
0:
	.string	"GNU"
1:
	.align 8
	.long	0xc0000002
	.long	3f - 2f
2:
	.long	0x3
3:
	.align 8
4:
