/*
 * Linux x86-64 AVX2+BMI2 assembly implementation for Cuishen-768.
 *
 * This is a mechanically split GCC 11.4 -O3 assembly output generated from
 * the AVX2 C implementation with an 8-round full-block loop precursor:
 *
 *   Cuishen-768/02-c-avx2/CryptHash_Cuishen-768-c-avx2.c
 *
 * Build with AVX2/BMI/BMI2/ADX enabled; do not use AVX-512 flags here.
 */

	.file	"CryptHash_Cuishen-768-c-avx2-u8.c"
	.intel_syntax noprefix
	.text
	.p2align 4
	.globl	CryptHash
	.type	CryptHash, @function
CryptHash:
	endbr64
	push	rbp
	mov	rbp, rsp
	push	r15
	push	r14
	push	r13
	push	r12
	push	rbx
	and	rsp, -32
	sub	rsp, 992
	mov	QWORD PTR 48[rsp], rsi
	mov	QWORD PTR 40[rsp], rdx
	mov	QWORD PTR 32[rsp], rcx
	mov	rax, QWORD PTR fs:40
	mov	QWORD PTR 984[rsp], rax
	xor	eax, eax
	cmp	edi, 768
	jne	.L55
	test	rcx, rcx
	je	.L55
	test	rdx, rdx
	je	.L63
	test	rsi, rsi
	je	.L55
.L63:
	mov	rax, QWORD PTR 40[rsp]
	mov	rcx, rax
	and	ecx, 1023
	shr	rax, 10
	mov	QWORD PTR 24[rsp], rcx
	mov	QWORD PTR 16[rsp], rax
	je	.L56
	add	rax, 1
	vmovdqa	ymm7, YMMWORD PTR .LC5[rip]
	vmovdqa	ymm5, YMMWORD PTR .LC9[rip]
	mov	QWORD PTR 232[rsp], 1024
	sal	rax, 10
	vmovdqa	ymm1, YMMWORD PTR .LC0[rip]
	vmovdqa	ymm2, YMMWORD PTR .LC1[rip]
	mov	QWORD PTR 56[rsp], rax
	mov	rax, QWORD PTR 48[rsp]
	vmovdqa	ymm0, YMMWORD PTR .LC2[rip]
	vmovdqa	ymm4, YMMWORD PTR .LC3[rip]
	vmovdqa	YMMWORD PTR 192[rsp], ymm7
	vmovdqa	ymm7, YMMWORD PTR .LC10[rip]
	mov	QWORD PTR 224[rsp], rax
	lea	rax, E_CONST[rip+512]
	mov	QWORD PTR 240[rsp], rax
	mov	QWORD PTR 8[rsp], rbp
	mov	rbp, rax
	vmovdqa	ymm15, YMMWORD PTR .LC8[rip]
	vmovdqa	ymm11, YMMWORD PTR .LC14[rip]
	vmovdqa	ymm12, YMMWORD PTR .LC15[rip]
	vmovdqa	YMMWORD PTR 512[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC11[rip]
	vmovdqa	YMMWORD PTR 544[rsp], ymm7
	vmovdqa	ymm7, YMMWORD PTR .LC12[rip]
	vmovdqa	YMMWORD PTR 608[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC13[rip]
	vmovdqa	YMMWORD PTR 576[rsp], ymm7
	vmovdqa	YMMWORD PTR 480[rsp], ymm5
	.p2align 4,,10
	.p2align 3

.L6:
	mov	rax, QWORD PTR 224[rsp]
	prefetcht0	128[rax]
	vpunpckhqdq	ymm13, ymm2, ymm1
	vpunpckhqdq	ymm3, ymm4, ymm0
	lea	r11, E_CONST[rip]
	vpermq	ymm13, ymm13, 216
	vpermq	ymm3, ymm3, 216
	vmovdqu	ymm7, YMMWORD PTR [rax]
	vmovdqu	ymm6, YMMWORD PTR 32[rax]
	vmovq	rsi, xmm13
	vmovdqa	ymm5, YMMWORD PTR 192[rsp]
	vpshufb	ymm7, ymm7, ymm5
	vmovdqa	YMMWORD PTR 160[rsp], ymm7
	vpshufb	ymm14, ymm6, ymm5
	vmovdqu	ymm6, YMMWORD PTR 64[rax]
	vpextrq	QWORD PTR 272[rsp], xmm7, 1
	vmovdqa	YMMWORD PTR 128[rsp], ymm14
	vmovq	rbx, xmm14
	vpextrq	QWORD PTR 288[rsp], xmm14, 1
	vmovq	QWORD PTR 376[rsp], xmm7
	vpshufb	ymm8, ymm6, ymm5
	vmovdqu	ymm6, YMMWORD PTR 96[rax]
	vpextrq	rax, xmm13, 1
	vmovdqa	YMMWORD PTR 96[rsp], ymm8
	vpextrq	r14, xmm8, 1
	vpshufb	ymm10, ymm6, ymm5
	vpunpcklqdq	ymm5, ymm2, ymm1
	vpunpcklqdq	ymm6, ymm4, ymm0
	vpermq	ymm5, ymm5, 216
	vpermq	ymm6, ymm6, 216
	vmovdqa	YMMWORD PTR 64[rsp], ymm10
	vpermq	ymm2, ymm5, 80
	vpermq	ymm1, ymm5, 250
	vpxor	ymm1, ymm1, YMMWORD PTR .LC7[rip]
	vextracti128	xmm5, ymm7, 0x1
	vmovq	QWORD PTR 352[rsp], xmm10
	vmovq	QWORD PTR 336[rsp], xmm5
	vpermq	ymm4, ymm6, 80
	vpermq	ymm0, ymm6, 250
	vmovdqa	YMMWORD PTR 640[rsp], ymm2
	vmovdqa	YMMWORD PTR 672[rsp], ymm1
	vmovdqa	YMMWORD PTR 704[rsp], ymm4
	vmovdqa	YMMWORD PTR 736[rsp], ymm0
	vpextrq	QWORD PTR 328[rsp], xmm5, 1
	vextracti128	xmm5, ymm14, 0x1
	vmovq	QWORD PTR 264[rsp], xmm5
	vpextrq	QWORD PTR 256[rsp], xmm5, 1
	vextracti128	xmm5, ymm8, 0x1
	vmovq	QWORD PTR 248[rsp], xmm5
	vpextrq	QWORD PTR 320[rsp], xmm5, 1
	vpextrq	QWORD PTR 368[rsp], xmm10, 1
	mov	rcx, QWORD PTR 232[rsp]
	vextracti128	xmm5, ymm10, 0x1
	mov	QWORD PTR 416[rsp], rsi
	vpextrq	QWORD PTR 408[rsp], xmm3, 1
	vmovq	QWORD PTR 392[rsp], xmm3
	vextracti128	xmm3, ymm3, 0x1
	vpextrq	r12, xmm5, 1
	vmovq	r13, xmm5
	vmovq	xmm5, rax
	vmovq	QWORD PTR 448[rsp], xmm3
	vpextrq	QWORD PTR 400[rsp], xmm3, 1
	mov	QWORD PTR 424[rsp], rcx
	vmovdqa	xmm3, xmm13
	vextracti128	xmm13, ymm13, 0x1
	vmovq	QWORD PTR 440[rsp], xmm13
	vpbroadcastq	xmm14, xmm3
	vpextrq	QWORD PTR 432[rsp], xmm13, 1
	vpunpcklqdq	xmm3, xmm5, xmm5
	vpbroadcastq	xmm13, xmm13
	mov	QWORD PTR 384[rsp], rax
	vmovq	rax, xmm8
	vmovdqa	XMMWORD PTR 304[rsp], xmm13
	vmovdqa	xmm13, xmm3
	.p2align 4,,10
	.p2align 3

.L5:
	mov	r10, QWORD PTR 408[rsp]
	vpxor	ymm7, ymm1, ymm2
	vperm2i128	ymm3, ymm7, ymm7, 1
	mov	rdx, QWORD PTR 272[rsp]
	mov	r8, QWORD PTR 424[rsp]
	mov	r15, QWORD PTR 392[rsp]
	xor	rdx, r10
	vpxor	ymm7, ymm7, ymm3
	vmovddup	xmm3, QWORD PTR 432[rsp]
	vinserti128	ymm5, ymm3, XMMWORD PTR 304[rsp], 0x1
	mov	rsi, rdx
	mov	rdx, QWORD PTR [r11]
	mov	rdi, QWORD PTR 400[rsp]
	xor	rsi, r8
	vpxor	ymm8, ymm7, ymm5
	mov	r9, QWORD PTR 448[rsp]
	mov	rcx, QWORD PTR 248[rsp]
	vmovq	xmm9, rsi
	mov	rsi, QWORD PTR 376[rsp]
	vpsrlvq	ymm10, ymm8, ymm15
	xor	rcx, QWORD PTR 320[rsp]
	vpsllvq	ymm8, ymm8, YMMWORD PTR 512[rsp]
	xor	rsi, r15
	xor	rdx, rsi
	mov	rsi, rdi
	vmovq	xmm6, rdx
	vpor	ymm8, ymm8, ymm10
	xor	rsi, r14
	rorx	rdx, rcx, 58
	vpunpcklqdq	xmm6, xmm6, xmm9
	rorx	rcx, rcx, 52
	vpermq	ymm6, ymm6, 80
	vpxor	ymm6, ymm4, ymm6
	vpsrlvq	ymm4, ymm6, YMMWORD PTR 544[rsp]
	vpsllvq	ymm6, ymm6, YMMWORD PTR 608[rsp]
	vpor	ymm6, ymm6, ymm4
	vpaddq	ymm8, ymm8, ymm6
	vinserti128	ymm6, ymm13, xmm14, 0x1
	vpxor	ymm6, ymm6, ymm7
	vmovq	xmm7, rsi
	mov	rsi, r9
	vpsrlvq	ymm9, ymm6, YMMWORD PTR 576[rsp]
	xor	rsi, rax
	rorx	rax, rax, 35
	vmovq	xmm4, rsi
	mov	rsi, QWORD PTR 336[rsp]
	add	rax, rdx
	xor	rsi, QWORD PTR 328[rsp]
	vpunpcklqdq	xmm4, xmm4, xmm7
	rorx	rdx, r14, 1
	rorx	r14, rsi, 45
	add	rcx, rdx
	vpermq	ymm4, ymm4, 80
	rorx	rsi, rsi, 36
	vpsllvq	ymm6, ymm6, YMMWORD PTR 480[rsp]
	rorx	rdx, QWORD PTR 376[rsp], 50
	vpxor	ymm4, ymm0, ymm4
	add	r14, rdx
	rorx	rdx, QWORD PTR 272[rsp], 21
	add	rsi, rdx
	vpsrlvq	ymm0, ymm4, ymm11
	vpsllvq	ymm4, ymm4, ymm12
	rorx	rdx, r15, 17
	xor	rdx, r10
	vpor	ymm6, ymm6, ymm9
	mov	QWORD PTR 392[rsp], rdx
	mov	r15, QWORD PTR 264[rsp]
	vpor	ymm4, ymm4, ymm0
	vpaddq	ymm6, ymm6, ymm4
	vpxor	ymm9, ymm8, ymm6
	vperm2i128	ymm0, ymm9, ymm9, 1
	vpxor	ymm9, ymm9, ymm0
	vmovq	xmm0, rdx
	mov	rdx, QWORD PTR 256[rsp]
	vpunpcklqdq	xmm0, xmm0, xmm0
	xor	rdx, r9
	vinserti128	ymm3, ymm0, xmm3, 0x1
	mov	r9, rdi
	vmovq	xmm10, rdx
	mov	rdx, r10
	vpxor	ymm7, ymm9, ymm3
	xor	rdx, r8
	vpsrlvq	ymm14, ymm7, ymm15
	rorx	r8, r12, 1
	vpsllvq	ymm7, ymm7, YMMWORD PTR 512[rsp]
	xor	rdx, r15
	xor	rdx, QWORD PTR 8[r11]
	vmovq	xmm4, rdx
	mov	rdx, QWORD PTR 416[rsp]
	vpunpcklqdq	xmm4, xmm4, xmm10
	vpermq	ymm4, ymm4, 80
	xor	rdx, r12
	vpor	ymm7, ymm7, ymm14
	vmovdqa	xmm14, XMMWORD PTR 304[rsp]
	vpxor	ymm4, ymm4, ymm2
	vpsrlvq	ymm2, ymm4, YMMWORD PTR 544[rsp]
	vpsllvq	ymm4, ymm4, YMMWORD PTR 608[rsp]
	vinserti128	ymm13, ymm14, xmm13, 0x1
	vpxor	ymm13, ymm13, ymm9
	vpsrlvq	ymm9, ymm13, YMMWORD PTR 576[rsp]
	vpsllvq	ymm13, ymm13, YMMWORD PTR 480[rsp]
	vpor	ymm4, ymm4, ymm2
	vpaddq	ymm7, ymm7, ymm4
	vmovq	xmm4, rdx
	mov	rdx, rdi
	mov	rdi, r14
	xor	rdx, r13
	vpunpcklqdq	xmm4, xmm4, xmm4
	xor	rdi, rsi
	rorx	r13, r13, 35
	vmovq	xmm2, rdx
	vpor	ymm13, ymm13, ymm9
	rorx	r10, rdi, 58
	mov	rdx, rax
	vpunpcklqdq	xmm2, xmm2, xmm2
	rorx	rdi, rdi, 52
	lea	r12, [rdi+r8]
	add	r10, r13
	vinserti128	ymm2, ymm2, xmm4, 0x1
	mov	QWORD PTR 304[rsp], r12
	xor	rdx, rcx
	rorx	rdi, r15, 50
	vpxor	ymm2, ymm2, ymm1
	rorx	r12, rdx, 45
	add	r12, rdi
	mov	r13, QWORD PTR 448[rsp]
	vpsrlvq	ymm1, ymm2, ymm11
	vpsllvq	ymm2, ymm2, ymm12
	rorx	rdi, QWORD PTR 256[rsp], 21
	rorx	rdx, rdx, 36
	add	rdx, rdi
	mov	r15, QWORD PTR 288[rsp]
	rorx	rdi, QWORD PTR 408[rsp], 17
	xor	rdi, r13
	vpor	ymm2, ymm2, ymm1
	mov	QWORD PTR 408[rsp], rdi
	rorx	r8, QWORD PTR 424[rsp], 62
	vpaddq	ymm13, ymm13, ymm2
	vpxor	ymm2, ymm7, ymm13
	vperm2i128	ymm1, ymm2, ymm2, 1
	vpxor	ymm2, ymm2, ymm1
	vmovq	xmm1, rdi
	mov	rdi, QWORD PTR 368[rsp]
	vpunpcklqdq	xmm4, xmm1, xmm1
	vpxor	ymm5, ymm5, ymm2
	xor	rdi, r9
	vinserti128	ymm0, ymm4, xmm0, 0x1
	mov	r9, QWORD PTR 416[rsp]
	xor	rdi, r8
	vpxor	ymm9, ymm2, ymm0
	vmovq	xmm10, rdi
	mov	rdi, QWORD PTR 352[rsp]
	vpsrlvq	ymm14, ymm9, ymm15
	xor	r9, rbx
	vpunpcklqdq	xmm10, xmm10, xmm10
	rorx	rbx, rbx, 35
	vpsllvq	ymm9, ymm9, YMMWORD PTR 512[rsp]
	xor	rdi, r13
	xor	rdi, QWORD PTR 16[r11]
	vmovq	xmm1, rdi
	mov	rdi, QWORD PTR 384[rsp]
	vpunpcklqdq	xmm1, xmm1, xmm1
	vinserti128	ymm1, ymm1, xmm10, 0x1
	xor	rdi, r15
	vpxor	ymm1, ymm1, ymm8
	vpor	ymm8, ymm9, ymm14
	vmovq	xmm2, rdi
	vpsrlvq	ymm10, ymm1, YMMWORD PTR 544[rsp]
	vpunpcklqdq	xmm2, xmm2, xmm2
	vpsllvq	ymm1, ymm1, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm9, ymm5, YMMWORD PTR 576[rsp]
	vpsllvq	ymm5, ymm5, YMMWORD PTR 480[rsp]
	vpor	ymm1, ymm1, ymm10
	vpaddq	ymm8, ymm8, ymm1
	vmovq	xmm1, r9
	vpor	ymm5, ymm5, ymm9
	mov	r9, QWORD PTR 304[rsp]
	vpunpcklqdq	xmm1, xmm1, xmm1
	xor	r9, r10
	vinserti128	ymm1, ymm1, xmm2, 0x1
	mov	rdi, r9
	mov	r9, r12
	vpxor	ymm1, ymm1, ymm6
	xor	r9, rdx
	vpsrlvq	ymm2, ymm1, ymm11
	mov	r13, r9
	rorx	r9, r9, 58
	vpsllvq	ymm1, ymm1, ymm12
	add	r9, rbx
	rorx	r13, r13, 52
	rorx	rbx, r15, 1
	add	rbx, r13
	rorx	r13, rdi, 45
	mov	QWORD PTR 288[rsp], rbx
	rorx	rdi, rdi, 36
	vpor	ymm1, ymm1, ymm2
	rorx	rbx, QWORD PTR 352[rsp], 50
	add	rbx, r13
	vpaddq	ymm5, ymm5, ymm1
	vpxor	ymm6, ymm8, ymm5
	vperm2i128	ymm1, ymm6, ymm6, 1
	vpxor	ymm6, ymm6, ymm1
	mov	QWORD PTR 352[rsp], rbx
	rorx	rbx, QWORD PTR 368[rsp], 21
	add	rdi, rbx
	mov	QWORD PTR 368[rsp], rdi
	mov	rbx, QWORD PTR 400[rsp]
	rorx	rdi, QWORD PTR 448[rsp], 17
	mov	r13, rdi
	mov	rdi, QWORD PTR 336[rsp]
	xor	r13, rbx
	xor	rdi, rbx
	vmovq	xmm9, r13
	mov	QWORD PTR 448[rsp], r13
	vpxor	ymm3, ymm3, ymm6
	xor	rdi, r8
	mov	r13, QWORD PTR 328[rsp]
	xor	rdi, QWORD PTR 24[r11]
	vpunpcklqdq	xmm2, xmm9, xmm9
	xor	r13, QWORD PTR 416[rsp]
	vmovq	xmm1, rdi
	vinserti128	ymm4, ymm2, xmm4, 0x1
	vmovq	xmm10, r13
	vpunpcklqdq	xmm1, xmm1, xmm1
	vpxor	ymm9, ymm6, ymm4
	vpunpcklqdq	xmm10, xmm10, xmm10
	vpsrlvq	ymm14, ymm9, ymm15
	vpsllvq	ymm9, ymm9, YMMWORD PTR 512[rsp]
	vinserti128	ymm1, ymm1, xmm10, 0x1
	vpxor	ymm1, ymm1, ymm7
	vpsrlvq	ymm7, ymm1, YMMWORD PTR 544[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 608[rsp]
	vpor	ymm9, ymm9, ymm14
	vpor	ymm1, ymm1, ymm7
	vpsrlvq	ymm7, ymm3, YMMWORD PTR 576[rsp]
	mov	r15, QWORD PTR 384[rsp]
	mov	r13, QWORD PTR 440[rsp]
	vpaddq	ymm9, ymm9, ymm1
	xor	r13, QWORD PTR 320[rsp]
	vmovq	xmm6, r13
	mov	r8, r15
	mov	r13, QWORD PTR 248[rsp]
	mov	rdi, QWORD PTR 288[rsp]
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpsllvq	ymm3, ymm3, YMMWORD PTR 480[rsp]
	xor	r8, r13
	rorx	r13, r13, 35
	vmovq	xmm1, r8
	mov	r8, QWORD PTR 352[rsp]
	xor	rdi, r9
	xor	r8, QWORD PTR 368[rsp]
	mov	rbx, r8
	rorx	r8, r8, 58
	add	r8, r13
	vpunpcklqdq	xmm1, xmm1, xmm1
	rorx	rbx, rbx, 52
	vinserti128	ymm1, ymm1, xmm6, 0x1
	vpor	ymm3, ymm3, ymm7
	rorx	r13, QWORD PTR 320[rsp], 1
	add	r13, rbx
	vpxor	ymm1, ymm1, ymm13
	rorx	rbx, QWORD PTR 336[rsp], 50
	mov	QWORD PTR 320[rsp], r13
	rorx	r13, rdi, 45
	add	r13, rbx
	rorx	rdi, rdi, 36
	rorx	rbx, QWORD PTR 328[rsp], 21
	mov	QWORD PTR 336[rsp], r13
	add	rdi, rbx
	vpsrlvq	ymm6, ymm1, ymm11
	mov	r13, QWORD PTR 416[rsp]
	mov	QWORD PTR 328[rsp], rdi
	vpsllvq	ymm1, ymm1, ymm12
	rorx	rdi, QWORD PTR 400[rsp], 17
	mov	rbx, rdi
	rorx	rdi, QWORD PTR 424[rsp], 60
	xor	rbx, r13
	vpor	ymm1, ymm1, ymm6
	vmovq	xmm7, rbx
	mov	QWORD PTR 400[rsp], rbx
	mov	rbx, r15
	vpaddq	ymm3, ymm3, ymm1
	vpxor	ymm10, ymm9, ymm3
	xor	rbx, rdi
	vperm2i128	ymm1, ymm10, ymm10, 1
	vpunpcklqdq	xmm6, xmm7, xmm7
	xor	rbx, rcx
	rorx	rcx, rcx, 21
	vmovq	xmm13, rbx
	mov	rbx, QWORD PTR 32[r11]
	vpxor	ymm10, ymm10, ymm1
	vinserti128	ymm2, ymm6, xmm2, 0x1
	vpunpcklqdq	xmm13, xmm13, xmm13
	vpxor	ymm7, ymm10, ymm2
	vpxor	ymm0, ymm0, ymm10
	xor	rbx, r13
	mov	r13, QWORD PTR 432[rsp]
	vpsrlvq	ymm14, ymm7, ymm15
	vpsllvq	ymm7, ymm7, YMMWORD PTR 512[rsp]
	xor	rbx, rax
	rorx	rax, rax, 50
	vmovq	xmm1, rbx
	xor	r13, rsi
	mov	rbx, QWORD PTR 320[rsp]
	rorx	rsi, rsi, 1
	vpunpcklqdq	xmm1, xmm1, xmm1
	vmovq	xmm10, r13
	mov	r13, QWORD PTR 440[rsp]
	vinserti128	ymm1, ymm1, xmm13, 0x1
	vpunpcklqdq	xmm10, xmm10, xmm10
	vpor	ymm7, ymm7, ymm14
	xor	rbx, r8
	vpsrlvq	ymm13, ymm0, YMMWORD PTR 576[rsp]
	vpxor	ymm1, ymm1, ymm8
	xor	r13, r14
	rorx	r14, r14, 35
	vpsrlvq	ymm8, ymm1, YMMWORD PTR 544[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 608[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 480[rsp]
	vpor	ymm1, ymm1, ymm8
	vmovq	xmm8, r13
	mov	r13, QWORD PTR 336[rsp]
	xor	r13, QWORD PTR 328[rsp]
	vpunpcklqdq	xmm8, xmm8, xmm8
	rorx	r15, r13, 58
	add	r14, r15
	vpor	ymm0, ymm0, ymm13
	vinserti128	ymm8, ymm8, xmm10, 0x1
	mov	QWORD PTR 376[rsp], r14
	rorx	r13, r13, 52
	vpaddq	ymm7, ymm7, ymm1
	vpxor	ymm8, ymm8, ymm5
	lea	r14, 0[r13+rsi]
	rorx	rsi, rbx, 45
	add	rax, rsi
	vpsrlvq	ymm5, ymm8, ymm11
	vpsllvq	ymm8, ymm8, ymm12
	mov	r13, r14
	rorx	rbx, rbx, 36
	mov	QWORD PTR 272[rsp], r14
	lea	r14, [rbx+rcx]
	rorx	rcx, QWORD PTR 416[rsp], 17
	xor	rdi, QWORD PTR 40[r11]
	vpor	ymm8, ymm8, ymm5
	mov	rbx, QWORD PTR 384[rsp]
	mov	r15, rcx
	mov	rcx, QWORD PTR 304[rsp]
	vpaddq	ymm1, ymm0, ymm8
	vpxor	ymm0, ymm7, ymm1
	xor	rcx, QWORD PTR 440[rsp]
	mov	rsi, QWORD PTR 376[rsp]
	vperm2i128	ymm5, ymm0, ymm0, 1
	xor	r15, rbx
	vmovq	xmm10, rcx
	xor	rdi, rbx
	xor	rdi, r10
	xor	rsi, r13
	vpxor	ymm0, ymm0, ymm5
	vpunpcklqdq	xmm10, xmm10, xmm10
	vmovq	xmm5, r15
	mov	rcx, QWORD PTR 392[rsp]
	vpunpcklqdq	xmm14, xmm5, xmm5
	vpxor	ymm4, ymm4, ymm0
	rorx	r10, r10, 50
	add	r11, 64
	vmovq	xmm5, rdi
	xor	rcx, rdx
	rorx	rdx, rdx, 1
	mov	QWORD PTR 416[rsp], r15
	vpunpcklqdq	xmm5, xmm5, xmm5
	mov	r15, QWORD PTR 432[rsp]
	vinserti128	ymm6, ymm14, xmm6, 0x1
	vinserti128	ymm5, ymm5, xmm10, 0x1
	vpxor	ymm8, ymm0, ymm6
	vpxor	ymm5, ymm5, ymm9
	vpsrlvq	ymm13, ymm8, ymm15
	vpsrlvq	ymm9, ymm5, YMMWORD PTR 544[rsp]
	vpsllvq	ymm5, ymm5, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm10, ymm4, YMMWORD PTR 576[rsp]
	vpsllvq	ymm8, ymm8, YMMWORD PTR 512[rsp]
	vpsllvq	ymm4, ymm4, YMMWORD PTR 480[rsp]
	vpor	ymm5, ymm5, ymm9
	vmovq	xmm9, rcx
	mov	rcx, r15
	xor	rcx, r12
	vpunpcklqdq	xmm9, xmm9, xmm9
	rorx	r12, r12, 35
	vpor	ymm8, ymm8, ymm13
	vmovq	xmm0, rcx
	vpor	ymm4, ymm4, ymm10
	vpaddq	ymm5, ymm8, ymm5
	mov	rcx, rsi
	vpunpcklqdq	xmm0, xmm0, xmm0
	mov	rsi, rax
	rorx	r13, rcx, 45
	rorx	rcx, rcx, 36
	xor	rsi, r14
	vinserti128	ymm0, ymm0, xmm9, 0x1
	add	r13, r10
	mov	r10, QWORD PTR 408[rsp]
	rorx	rdi, rsi, 58
	vpxor	ymm0, ymm0, ymm3
	rorx	rsi, rsi, 52
	add	rdx, rsi
	mov	rsi, QWORD PTR 440[rsp]
	add	r12, rdi
	vpsrlvq	ymm3, ymm0, ymm11
	mov	rdi, rdx
	vpsllvq	ymm0, ymm0, ymm12
	mov	QWORD PTR 264[rsp], r12
	mov	QWORD PTR 256[rsp], rdx
	rorx	rdx, QWORD PTR 304[rsp], 21
	lea	r12, [rcx+rdx]
	rorx	rdx, rbx, 17
	xor	rdx, rsi
	mov	rcx, QWORD PTR -16[r11]
	vpor	ymm0, ymm0, ymm3
	mov	rbx, rdx
	vpaddq	ymm8, ymm4, ymm0
	vpxor	ymm3, ymm5, ymm8
	mov	QWORD PTR 384[rsp], rdx
	rorx	rdx, QWORD PTR 424[rsp], 58
	vmovq	xmm4, rbx
	xor	rdx, r15
	vperm2i128	ymm0, ymm3, ymm3, 1
	xor	rcx, rsi
	xor	rcx, r9
	mov	rsi, r13
	vpxor	ymm3, ymm3, ymm0
	vmovq	xmm0, rcx
	vpunpcklqdq	xmm13, xmm4, xmm4
	xor	rsi, r12
	mov	rbx, QWORD PTR 288[rsp]
	vpunpcklqdq	xmm0, xmm0, xmm0
	vinserti128	ymm4, ymm13, xmm14, 0x1
	mov	r15, QWORD PTR 368[rsp]
	vpxor	ymm4, ymm4, ymm3
	vpxor	ymm2, ymm2, ymm3
	mov	rcx, QWORD PTR 264[rsp]
	rorx	r9, r9, 50
	xor	rbx, rdx
	vpsrlvq	ymm10, ymm4, ymm15
	xor	r10, r15
	vmovq	xmm9, rbx
	mov	rbx, QWORD PTR 392[rsp]
	xor	rcx, rdi
	rorx	rdi, rsi, 58
	vpunpcklqdq	xmm9, xmm9, xmm9
	rorx	rsi, rsi, 52
	vpsllvq	ymm4, ymm4, YMMWORD PTR 512[rsp]
	vinserti128	ymm0, ymm0, xmm9, 0x1
	vpsrlvq	ymm9, ymm2, YMMWORD PTR 576[rsp]
	vpxor	ymm0, ymm0, ymm7
	vpsrlvq	ymm7, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpor	ymm4, ymm4, ymm10
	vmovdqa	ymm10, YMMWORD PTR 480[rsp]
	vpsllvq	ymm2, ymm2, ymm10
	vpor	ymm0, ymm0, ymm7
	vmovq	xmm7, r10
	mov	r10, QWORD PTR 352[rsp]
	vpunpcklqdq	xmm7, xmm7, xmm7
	vpor	ymm2, ymm2, ymm9
	vpaddq	ymm4, ymm4, ymm0
	xor	rbx, r10
	rorx	r10, r10, 35
	add	r10, rdi
	rorx	rdi, r15, 1
	vmovq	xmm3, rbx
	add	rsi, rdi
	rorx	rbx, rcx, 45
	add	rbx, r9
	vpunpcklqdq	xmm3, xmm3, xmm3
	mov	QWORD PTR 352[rsp], r10
	rorx	rcx, rcx, 36
	mov	r10, QWORD PTR 392[rsp]
	vinserti128	ymm3, ymm3, xmm7, 0x1
	mov	QWORD PTR 368[rsp], rsi
	rorx	rsi, QWORD PTR 288[rsp], 21
	lea	r9, [rcx+rsi]
	vpxor	ymm3, ymm3, ymm1
	mov	r15, r9
	xor	rdx, QWORD PTR -8[r11]
	mov	QWORD PTR 288[rsp], r9
	vpsrlvq	ymm1, ymm3, ymm11
	vpsllvq	ymm3, ymm3, ymm12
	xor	rdx, r8
	mov	r9, QWORD PTR 432[rsp]
	rorx	rcx, QWORD PTR 440[rsp], 17
	mov	rsi, rcx
	mov	rdi, QWORD PTR 336[rsp]
	rorx	r8, r8, 50
	vpor	ymm3, ymm3, ymm1
	xor	rsi, r9
	mov	rcx, QWORD PTR 408[rsp]
	vpaddq	ymm0, ymm2, ymm3
	vpxor	ymm1, ymm4, ymm0
	mov	QWORD PTR 440[rsp], rsi
	vmovq	xmm3, rdx
	mov	rdx, QWORD PTR 448[rsp]
	vperm2i128	ymm2, ymm1, ymm1, 1
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	rcx, rdi
	xor	rdx, QWORD PTR 328[rsp]
	rorx	rdi, rdi, 35
	vpxor	ymm1, ymm1, ymm2
	vmovq	xmm2, rsi
	mov	rsi, QWORD PTR 320[rsp]
	vpunpcklqdq	xmm2, xmm2, xmm2
	vpxor	ymm6, ymm6, ymm1
	vmovdqa	XMMWORD PTR 304[rsp], xmm2
	xor	rsi, r10
	vinserti128	ymm2, ymm2, xmm13, 0x1
	vmovq	xmm7, rsi
	vpxor	ymm2, ymm2, ymm1
	vpunpcklqdq	xmm7, xmm7, xmm7
	vpsrlvq	ymm9, ymm2, ymm15
	vpsllvq	ymm2, ymm2, YMMWORD PTR 512[rsp]
	vinserti128	ymm3, ymm3, xmm7, 0x1
	vpsrlvq	ymm7, ymm6, YMMWORD PTR 576[rsp]
	vpxor	ymm3, ymm3, ymm5
	vpsrlvq	ymm5, ymm3, YMMWORD PTR 544[rsp]
	vpsllvq	ymm6, ymm6, ymm10
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	vpor	ymm2, ymm2, ymm9
	vpor	ymm6, ymm6, ymm7
	vpor	ymm3, ymm3, ymm5
	vmovq	xmm5, rdx
	mov	rdx, QWORD PTR 352[rsp]
	xor	rdx, QWORD PTR 368[rsp]
	vpaddq	ymm2, ymm2, ymm3
	vmovq	xmm3, rcx
	vpunpcklqdq	xmm5, xmm5, xmm5
	mov	rcx, r15
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	rcx, rbx
	vinserti128	ymm3, ymm3, xmm5, 0x1
	rorx	rsi, rcx, 58
	rorx	rcx, rcx, 52
	lea	r15, [rsi+rdi]
	vpxor	ymm3, ymm3, ymm8
	mov	QWORD PTR 336[rsp], r15
	rorx	rsi, QWORD PTR 328[rsp], 1
	add	rcx, rsi
	vpsrlvq	ymm5, ymm3, ymm11
	vpsllvq	ymm3, ymm3, ymm12
	mov	QWORD PTR 328[rsp], rcx
	rorx	rcx, rdx, 45
	add	rcx, r8
	rorx	rdx, rdx, 36
	mov	QWORD PTR 248[rsp], rcx
	vpor	ymm3, ymm3, ymm5
	rorx	rcx, QWORD PTR 320[rsp], 21
	add	rcx, rdx
	rorx	rdx, r9, 17
	xor	rdx, r10
	vpaddq	ymm1, ymm6, ymm3
	mov	QWORD PTR 320[rsp], rcx
	mov	QWORD PTR 432[rsp], rdx
	rorx	rdi, QWORD PTR 424[rsp], 56
	mov	QWORD PTR 424[rsp], rdi
	cmp	r11, rbp
	jne	.L5
	vpxor	ymm2, ymm2, YMMWORD PTR 640[rsp]
	vpxor	ymm1, ymm1, YMMWORD PTR 672[rsp]
	vpxor	ymm4, ymm4, YMMWORD PTR 704[rsp]
	vpxor	ymm0, ymm0, YMMWORD PTR 736[rsp]

	add	QWORD PTR 232[rsp], 1024
	mov	rax, QWORD PTR 232[rsp]
	sub	QWORD PTR 224[rsp], -128
	cmp	rax, QWORD PTR 56[rsp]
	jne	.L6
	mov	rbp, QWORD PTR 8[rsp]
	mov	rax, QWORD PTR 16[rsp]
	vmovdqa	ymm7, YMMWORD PTR 64[rsp]
	vmovdqa	ymm5, YMMWORD PTR 96[rsp]
	vmovdqa	YMMWORD PTR 736[rsp], ymm7
	sal	rax, 7
	vmovdqa	ymm7, YMMWORD PTR 128[rsp]
	add	QWORD PTR 48[rsp], rax
	vmovdqa	YMMWORD PTR 704[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR 160[rsp]
	vmovdqa	YMMWORD PTR 672[rsp], ymm7
	vmovdqa	YMMWORD PTR 640[rsp], ymm5

.L4:
	mov	rcx, QWORD PTR 24[rsp]
	mov	r10, rcx
	mov	r8, rcx
	mov	r9d, ecx
	shr	r10, 3
	shr	r8, 6
	and	r9d, 7
	mov	edi, r10d
	or	r10d, ecx
	mov	eax, r8d
	and	edi, 7
	and	r10d, 7
	cmp	rcx, 512
	ja	.L219
	vpunpcklqdq	ymm3, ymm4, ymm0
	vpunpcklqdq	ymm5, ymm2, ymm1
	vpunpckhqdq	ymm4, ymm4, ymm0
	vpunpckhqdq	ymm0, ymm2, ymm1
	vpermq	ymm3, ymm3, 216
	vpermq	ymm4, ymm4, 216
	vpermq	ymm5, ymm5, 216
	vpermq	ymm0, ymm0, 216
	vmovdqa	YMMWORD PTR 768[rsp], ymm3
	vmovdqa	YMMWORD PTR 800[rsp], ymm5
	vmovdqa	YMMWORD PTR 832[rsp], ymm4
	vmovdqa	YMMWORD PTR 864[rsp], ymm0
	cmp	r8d, 3
	jbe	.L59
	vmovdqa	ymm5, YMMWORD PTR 192[rsp]
	mov	rcx, QWORD PTR 48[rsp]
	vmovdqu	ymm7, YMMWORD PTR [rcx]
	vmovdqa	YMMWORD PTR 448[rsp], ymm7
	vmovdqa	ymm7, YMMWORD PTR 448[rsp]
	vpshufb	ymm0, ymm7, ymm5
	vmovdqa	YMMWORD PTR 896[rsp], ymm0
	cmp	r8d, 8
	jne	.L30
	vmovdqu	ymm7, YMMWORD PTR 32[rcx]
	vmovdqa	YMMWORD PTR 448[rsp], ymm7
	vmovdqa	ymm7, YMMWORD PTR 448[rsp]
	vpshufb	ymm0, ymm7, ymm5
	vmovdqa	YMMWORD PTR 928[rsp], ymm0
.L30:
	lea	edx, -4[rax]
	and	edx, -4
	add	edx, 4
.L29:
	cmp	edx, eax
	jnb	.L34
	mov	rbx, QWORD PTR 48[rsp]
	lea	esi, 0[0+rdx*8]
	mov	r11d, edx
	mov	rcx, rsi
	mov	rsi, QWORD PTR [rbx+rsi]
	bswap	rsi
	mov	QWORD PTR 896[rsp+r11*8], rsi
	lea	esi, 1[rdx]
	cmp	esi, eax
	jnb	.L34
	lea	r11d, 8[rcx]
	mov	r11, QWORD PTR [rbx+r11]
	bswap	r11
	mov	QWORD PTR 896[rsp+rsi*8], r11
	lea	esi, 2[rdx]
	cmp	esi, eax
	jnb	.L34
	lea	r11d, 16[rcx]
	add	edx, 3
	mov	r11, QWORD PTR [rbx+r11]
	bswap	r11
	mov	QWORD PTR 896[rsp+rsi*8], r11
	cmp	edx, eax
	jnb	.L34
	add	ecx, 24
	mov	rcx, QWORD PTR [rbx+rcx]
	bswap	rcx
	mov	QWORD PTR 896[rsp+rdx*8], rcx
.L34:
	test	r10d, r10d
	jne	.L32
.L33:
	test	al, 3
	je	.L38
	cmp	eax, 7
	ja	.L38
	mov	edx, eax
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 1[rax]
	test	dl, 3
	je	.L39
	cmp	edx, 7
	ja	.L39
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 2[rax]
	test	dl, 3
	je	.L39
	cmp	edx, 7
	ja	.L39
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 3[rax]
	test	dl, 3
	je	.L39
	cmp	edx, 7
	ja	.L39
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 4[rax]
	test	al, 3
	je	.L42
	cmp	edx, 7
	ja	.L41
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 5[rax]
	test	dl, 3
	je	.L42
	cmp	edx, 7
	ja	.L41
	mov	QWORD PTR 896[rsp+rdx*8], 0
	lea	edx, 6[rax]
	test	dl, 3
	je	.L42
	cmp	edx, 7
	ja	.L41
	mov	QWORD PTR 952[rsp], 0
.L41:
	mov	rax, QWORD PTR 768[rsp]
	mov	rsi, QWORD PTR 800[rsp]
	vpxor	xmm0, xmm0, xmm0
	mov	rcx, QWORD PTR 808[rsp]
	vmovdqa	ymm1, ymm0
	vmovdqa	ymm10, ymm0
	mov	rbx, QWORD PTR 816[rsp]
	mov	QWORD PTR 416[rsp], rax
	mov	rax, QWORD PTR 776[rsp]
	mov	r15, QWORD PTR 832[rsp]
	mov	r10, QWORD PTR 880[rsp]
	mov	QWORD PTR 408[rsp], rax
	mov	rax, QWORD PTR 784[rsp]
	mov	QWORD PTR 368[rsp], rax
	mov	rax, QWORD PTR 792[rsp]
	mov	QWORD PTR 336[rsp], rax
	mov	rax, QWORD PTR 824[rsp]
	mov	QWORD PTR 392[rsp], rax
	mov	rax, QWORD PTR 840[rsp]
	mov	QWORD PTR 400[rsp], rax
	mov	rax, QWORD PTR 848[rsp]
	mov	QWORD PTR 320[rsp], rax
	mov	rax, QWORD PTR 856[rsp]
	mov	QWORD PTR 328[rsp], rax
	mov	rax, QWORD PTR 864[rsp]
	mov	QWORD PTR 376[rsp], rax
	mov	rax, QWORD PTR 872[rsp]
	mov	QWORD PTR 352[rsp], rax
	mov	rax, QWORD PTR 888[rsp]
	mov	QWORD PTR 384[rsp], rax
	mov	rax, QWORD PTR 896[rsp]
	mov	QWORD PTR 448[rsp], rax
	mov	rax, QWORD PTR 904[rsp]
	mov	QWORD PTR 440[rsp], rax
	mov	rax, QWORD PTR 912[rsp]
	mov	QWORD PTR 432[rsp], rax
	mov	r8, QWORD PTR 944[rsp]
	mov	rdx, QWORD PTR 928[rsp]
	mov	rdi, QWORD PTR 936[rsp]
	vmovq	xmm5, r8
	mov	r14, QWORD PTR 952[rsp]
	mov	rax, QWORD PTR 920[rsp]
	vpunpcklqdq	xmm5, xmm5, xmm5
	vmovdqa	ymm13, YMMWORD PTR .LC4[rip]
	mov	r9, QWORD PTR 40[rsp]
	vmovdqa	XMMWORD PTR 304[rsp], xmm5
	vmovq	xmm5, rdx
	vpunpcklqdq	xmm5, xmm5, xmm5
	mov	QWORD PTR 424[rsp], r14
	lea	r14, E_CONST[rip]
	vmovdqa	XMMWORD PTR 288[rsp], xmm5
	vmovq	xmm5, rdi
	vpunpcklqdq	xmm14, xmm5, xmm5
	.p2align 4,,10
	.p2align 3

.L45:
	mov	r11, QWORD PTR 408[rsp]
	vpxor	ymm3, ymm13, ymm10
	xor	r11, QWORD PTR 440[rsp]
	vperm2i128	ymm2, ymm3, ymm3, 1
	mov	r13, r11
	mov	r11, QWORD PTR [r14]
	xor	r13, r9
	vpxor	ymm3, ymm3, ymm2
	vmovddup	xmm4, QWORD PTR 424[rsp]
	vinserti128	ymm9, ymm4, XMMWORD PTR 304[rsp], 0x1
	vmovq	xmm6, r13
	mov	r13, QWORD PTR 416[rsp]
	xor	r13, QWORD PTR 448[rsp]
	xor	r11, r13
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpxor	ymm5, ymm3, ymm9
	mov	r12, QWORD PTR 400[rsp]
	vmovq	xmm2, r11
	vpsrlvq	ymm7, ymm5, ymm15
	mov	r13, QWORD PTR 432[rsp]
	mov	r11, QWORD PTR 368[rsp]
	vpunpcklqdq	xmm2, xmm2, xmm2
	xor	r12, rax
	xor	r11, QWORD PTR 336[rsp]
	vinserti128	ymm2, ymm2, xmm6, 0x1
	xor	r13, r15
	rorx	r15, r15, 35
	vpsllvq	ymm5, ymm5, YMMWORD PTR 512[rsp]
	vpxor	ymm2, ymm1, ymm2
	vpsrlvq	ymm1, ymm2, YMMWORD PTR 544[rsp]
	vpsllvq	ymm2, ymm2, YMMWORD PTR 608[rsp]
	vpor	ymm5, ymm5, ymm7
	vpor	ymm2, ymm2, ymm1
	vinserti128	ymm1, ymm14, XMMWORD PTR 288[rsp], 0x1
	vpaddq	ymm5, ymm5, ymm2
	vmovq	xmm2, r13
	vpxor	ymm1, ymm1, ymm3
	vmovq	xmm3, r12
	vpunpcklqdq	xmm2, xmm2, xmm2
	mov	r12, QWORD PTR 320[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	r12, QWORD PTR 328[rsp]
	vinserti128	ymm2, ymm2, xmm3, 0x1
	rorx	r13, r12, 58
	rorx	r12, r12, 52
	add	r15, r13
	vpsrlvq	ymm6, ymm1, YMMWORD PTR 576[rsp]
	vpxor	ymm2, ymm0, ymm2
	rorx	r13, QWORD PTR 400[rsp], 1
	vpsllvq	ymm1, ymm1, YMMWORD PTR 480[rsp]
	vpsrlvq	ymm0, ymm2, ymm11
	add	r12, r13
	rorx	r13, QWORD PTR 416[rsp], 50
	vpsllvq	ymm2, ymm2, ymm12
	mov	QWORD PTR 400[rsp], r12
	rorx	r12, r11, 45
	rorx	r11, r11, 36
	add	r13, r12
	rorx	r12, QWORD PTR 408[rsp], 21
	add	r12, r11
	rorx	r11, QWORD PTR 448[rsp], 17
	vpor	ymm2, ymm2, ymm0
	mov	QWORD PTR 408[rsp], r12
	vpor	ymm1, ymm1, ymm6
	mov	r12, r11
	mov	r11, QWORD PTR 440[rsp]
	vpaddq	ymm1, ymm1, ymm2
	vpxor	ymm2, ymm5, ymm1
	vperm2i128	ymm0, ymm2, ymm2, 1
	xor	r12, r11
	xor	r11, r9
	xor	r11, rbx
	xor	r11, QWORD PTR 8[r14]
	vpxor	ymm2, ymm2, ymm0
	rorx	rbx, rbx, 50
	mov	QWORD PTR 448[rsp], r12
	vmovq	xmm3, r12
	vmovq	xmm0, r11
	mov	r12, QWORD PTR 432[rsp]
	xor	r12, QWORD PTR 392[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpunpcklqdq	xmm0, xmm0, xmm0
	mov	r11, rax
	vmovq	xmm6, r12
	vinserti128	ymm4, ymm3, xmm4, 0x1
	xor	r11, r10
	rorx	r10, r10, 35
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpxor	ymm8, ymm2, ymm4
	vinserti128	ymm0, ymm0, xmm6, 0x1
	vpsrlvq	ymm7, ymm8, ymm15
	vpsllvq	ymm8, ymm8, YMMWORD PTR 512[rsp]
	vpxor	ymm0, ymm0, ymm10
	vpsrlvq	ymm6, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	mov	QWORD PTR 416[rsp], r13
	mov	r12, QWORD PTR 384[rsp]
	xor	r13, QWORD PTR 408[rsp]
	vpor	ymm8, ymm8, ymm7
	vpor	ymm0, ymm0, ymm6
	xor	r12, rdx
	vpaddq	ymm8, ymm8, ymm0
	vmovdqa	xmm0, XMMWORD PTR 304[rsp]
	vinserti128	ymm6, ymm0, xmm14, 0x1
	vmovq	xmm0, r11
	vpxor	ymm6, ymm6, ymm2
	vmovq	xmm2, r12
	vpunpcklqdq	xmm0, xmm0, xmm0
	mov	r12, QWORD PTR 400[rsp]
	vpsrlvq	ymm7, ymm6, YMMWORD PTR 576[rsp]
	vpunpcklqdq	xmm2, xmm2, xmm2
	vinserti128	ymm0, ymm0, xmm2, 0x1
	xor	r12, r15
	vpsllvq	ymm6, ymm6, YMMWORD PTR 480[rsp]
	vpxor	ymm0, ymm0, ymm13
	mov	r11, r12
	mov	r12, r13
	vpsrlvq	ymm2, ymm0, ymm11
	vpsllvq	ymm0, ymm0, ymm12
	rorx	r12, r12, 52
	rorx	r13, r13, 58
	add	r13, r10
	rorx	r10, QWORD PTR 384[rsp], 1
	add	r10, r12
	vpor	ymm0, ymm0, ymm2
	vpor	ymm6, ymm6, ymm7
	mov	r12, r10
	vpaddq	ymm6, ymm6, ymm0
	vpxor	ymm13, ymm8, ymm6
	rorx	r10, r11, 45
	rorx	r11, r11, 36
	add	rbx, r10
	vperm2i128	ymm0, ymm13, ymm13, 1
	rorx	r10, QWORD PTR 392[rsp], 21
	add	r10, r11
	mov	r11, QWORD PTR 432[rsp]
	mov	QWORD PTR 392[rsp], r10
	rorx	r10, QWORD PTR 440[rsp], 17
	vpxor	ymm13, ymm13, ymm0
	mov	QWORD PTR 384[rsp], rbx
	rorx	rbx, r9, 62
	xor	r10, r11
	mov	QWORD PTR 272[rsp], r12
	vmovq	xmm0, r10
	mov	QWORD PTR 440[rsp], r10
	mov	r10, QWORD PTR 352[rsp]
	vpunpcklqdq	xmm2, xmm0, xmm0
	xor	r10, rax
	vinserti128	ymm3, ymm2, xmm3, 0x1
	xor	r10, rbx
	vpxor	ymm7, ymm13, ymm3
	vmovq	xmm10, r10
	mov	r10, QWORD PTR 376[rsp]
	vpsrlvq	ymm14, ymm7, ymm15
	vpsllvq	ymm7, ymm7, YMMWORD PTR 512[rsp]
	vpunpcklqdq	xmm10, xmm10, xmm10
	xor	r10, r11
	xor	r10, QWORD PTR 16[r14]
	mov	r11, rdi
	vmovq	xmm0, r10
	xor	r11, rcx
	mov	r10, r12
	rorx	rcx, rcx, 1
	vpunpcklqdq	xmm0, xmm0, xmm0
	vpor	ymm7, ymm7, ymm14
	mov	r12, QWORD PTR 384[rsp]
	vinserti128	ymm0, ymm0, xmm10, 0x1
	vpxor	ymm0, ymm0, ymm5
	vpsrlvq	ymm5, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpor	ymm0, ymm0, ymm5
	vpxor	ymm5, ymm9, ymm13
	vmovq	xmm9, r11
	mov	r11, rdx
	xor	r11, rsi
	vpaddq	ymm7, ymm7, ymm0
	xor	r10, r13
	xor	r12, QWORD PTR 392[rsp]
	vmovq	xmm0, r11
	vpunpcklqdq	xmm9, xmm9, xmm9
	mov	r11, r12
	rorx	rsi, rsi, 35
	vpunpcklqdq	xmm0, xmm0, xmm0
	rorx	r11, r11, 52
	add	rcx, r11
	rorx	r12, r12, 58
	vinserti128	ymm0, ymm0, xmm9, 0x1
	add	r12, rsi
	mov	r11, rcx
	rorx	rcx, r10, 45
	vpsrlvq	ymm10, ymm5, YMMWORD PTR 576[rsp]
	vpxor	ymm0, ymm0, ymm1
	rorx	r10, r10, 36
	rorx	rsi, QWORD PTR 376[rsp], 50
	vpsllvq	ymm5, ymm5, YMMWORD PTR 480[rsp]
	vpsrlvq	ymm1, ymm0, ymm11
	add	rcx, rsi
	vpsllvq	ymm0, ymm0, ymm12
	mov	QWORD PTR 376[rsp], rcx
	rorx	rcx, QWORD PTR 352[rsp], 21
	lea	rsi, [rcx+r10]
	rorx	rcx, QWORD PTR 432[rsp], 17
	mov	r10, rcx
	mov	QWORD PTR 352[rsp], rsi
	vpor	ymm0, ymm0, ymm1
	vpor	ymm5, ymm5, ymm10
	xor	r10, rax
	vpaddq	ymm5, ymm5, ymm0
	vpxor	ymm10, ymm7, ymm5
	vmovq	xmm1, r10
	mov	QWORD PTR 432[rsp], r10
	vperm2i128	ymm0, ymm10, ymm10, 1
	vpxor	ymm10, ymm10, ymm0
	vpunpcklqdq	xmm0, xmm1, xmm1
	vinserti128	ymm2, ymm0, xmm2, 0x1
	vpxor	ymm4, ymm4, ymm10
	vpxor	ymm9, ymm10, ymm2
	vpsrlvq	ymm14, ymm9, ymm15
	vpsllvq	ymm9, ymm9, YMMWORD PTR 512[rsp]
	mov	rcx, QWORD PTR 368[rsp]
	mov	QWORD PTR 304[rsp], r11
	mov	r10, QWORD PTR 336[rsp]
	mov	rsi, QWORD PTR 328[rsp]
	vpsrlvq	ymm10, ymm4, YMMWORD PTR 576[rsp]
	xor	rcx, rax
	rorx	rax, rax, 17
	xor	rax, rdx
	xor	r10, rdx
	xor	rcx, rbx
	xor	rcx, QWORD PTR 24[r14]
	xor	rsi, r8
	vmovq	xmm13, r10
	vmovq	xmm1, rcx
	vpor	ymm9, ymm9, ymm14
	mov	r10, r11
	vpunpcklqdq	xmm13, xmm13, xmm13
	vpunpcklqdq	xmm1, xmm1, xmm1
	mov	rbx, QWORD PTR 320[rsp]
	xor	r10, r12
	vinserti128	ymm1, ymm1, xmm13, 0x1
	mov	rcx, r10
	vpxor	ymm1, ymm1, ymm8
	rorx	r11, rbx, 35
	vpsrlvq	ymm8, ymm1, YMMWORD PTR 544[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 608[rsp]
	vpsllvq	ymm4, ymm4, YMMWORD PTR 480[rsp]
	vpor	ymm1, ymm1, ymm8
	vmovq	xmm8, rsi
	vpor	ymm4, ymm4, ymm10
	mov	rsi, rbx
	xor	rsi, rdi
	vpaddq	ymm9, ymm9, ymm1
	vpunpcklqdq	xmm8, xmm8, xmm8
	vmovq	xmm1, rsi
	mov	rsi, QWORD PTR 376[rsp]
	xor	rsi, QWORD PTR 352[rsp]
	vpunpcklqdq	xmm1, xmm1, xmm1
	rorx	r10, rsi, 58
	add	r10, r11
	rorx	rsi, rsi, 52
	vinserti128	ymm1, ymm1, xmm8, 0x1
	mov	QWORD PTR 320[rsp], r10
	rorx	r10, QWORD PTR 328[rsp], 1
	lea	rbx, [r10+rsi]
	vpxor	ymm1, ymm1, ymm6
	mov	QWORD PTR 328[rsp], rbx
	rorx	rsi, rcx, 45
	rorx	r10, QWORD PTR 368[rsp], 50
	vpsrlvq	ymm6, ymm1, ymm11
	rorx	rcx, rcx, 36
	vpsllvq	ymm1, ymm1, ymm12
	lea	rbx, [r10+rsi]
	mov	QWORD PTR 368[rsp], rbx
	mov	r10, QWORD PTR 408[rsp]
	rorx	rsi, QWORD PTR 336[rsp], 21
	lea	r11, [rsi+rcx]
	rorx	rcx, r9, 60
	mov	rsi, rcx
	vpor	ymm1, ymm1, ymm6
	xor	rsi, rdi
	xor	rsi, QWORD PTR 400[rsp]
	vpaddq	ymm4, ymm4, ymm1
	vpxor	ymm13, ymm9, ymm4
	xor	r10, QWORD PTR 424[rsp]
	vmovq	xmm10, rsi
	mov	rsi, QWORD PTR 32[r14]
	vperm2i128	ymm1, ymm13, ymm13, 1
	vmovq	xmm6, rax
	vpunpcklqdq	xmm10, xmm10, xmm10
	mov	QWORD PTR 336[rsp], r11
	xor	rsi, rdx
	vpxor	ymm13, ymm13, ymm1
	vpunpcklqdq	xmm1, xmm6, xmm6
	rorx	rdx, rdx, 17
	xor	rsi, r15
	vinserti128	ymm0, ymm1, xmm0, 0x1
	vpxor	ymm3, ymm3, ymm13
	xor	rdx, rdi
	vmovq	xmm6, rsi
	vpxor	ymm8, ymm13, ymm0
	rorx	r15, r15, 50
	mov	rsi, QWORD PTR 416[rsp]
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpsrlvq	ymm14, ymm8, ymm15
	vinserti128	ymm6, ymm6, xmm10, 0x1
	mov	rbx, rsi
	rorx	rsi, rsi, 35
	vpsllvq	ymm8, ymm8, YMMWORD PTR 512[rsp]
	vpxor	ymm6, ymm6, ymm7
	xor	rbx, r8
	vpsrlvq	ymm7, ymm6, YMMWORD PTR 544[rsp]
	vpsllvq	ymm6, ymm6, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm10, ymm3, YMMWORD PTR 576[rsp]
	vpsllvq	ymm3, ymm3, YMMWORD PTR 480[rsp]
	vpor	ymm8, ymm8, ymm14
	vpor	ymm6, ymm6, ymm7
	vmovq	xmm7, rbx
	vpaddq	ymm8, ymm8, ymm6
	vmovq	xmm6, r10
	vpunpcklqdq	xmm7, xmm7, xmm7
	mov	r10, QWORD PTR 320[rsp]
	vpunpcklqdq	xmm6, xmm6, xmm6
	xor	r10, QWORD PTR 328[rsp]
	vpor	ymm3, ymm3, ymm10
	mov	r11, QWORD PTR 368[rsp]
	vinserti128	ymm7, ymm7, xmm6, 0x1
	xor	r11, QWORD PTR 336[rsp]
	xor	rcx, QWORD PTR 40[r14]
	vpxor	ymm7, ymm7, ymm5
	rorx	rbx, r11, 58
	add	rbx, rsi
	rorx	r11, r11, 52
	vpsrlvq	ymm5, ymm7, ymm11
	vpsllvq	ymm7, ymm7, ymm12
	mov	QWORD PTR 416[rsp], rbx
	xor	rcx, rdi
	rorx	rsi, QWORD PTR 408[rsp], 1
	lea	rbx, [rsi+r11]
	xor	rcx, r13
	mov	r11, QWORD PTR 272[rsp]
	vpor	ymm7, ymm7, ymm5
	rorx	rsi, r10, 45
	rorx	r10, r10, 36
	add	r15, rsi
	vpaddq	ymm6, ymm3, ymm7
	vpxor	ymm7, ymm8, ymm6
	vmovq	xmm14, rcx
	rorx	rsi, QWORD PTR 400[rsp], 21
	add	rsi, r10
	vperm2i128	ymm3, ymm7, ymm7, 1
	mov	QWORD PTR 408[rsp], rbx
	mov	QWORD PTR 400[rsp], rsi
	mov	rsi, r11
	vmovq	xmm5, rdx
	xor	rsi, r8
	vpunpcklqdq	xmm5, xmm5, xmm5
	vpxor	ymm7, ymm7, ymm3
	mov	rbx, QWORD PTR 384[rsp]
	vmovq	xmm10, rsi
	vpunpcklqdq	xmm3, xmm14, xmm14
	vmovdqa	XMMWORD PTR 288[rsp], xmm5
	vinserti128	ymm5, ymm5, xmm1, 0x1
	vpunpcklqdq	xmm10, xmm10, xmm10
	vpxor	ymm1, ymm7, ymm5
	mov	r10, QWORD PTR 424[rsp]
	mov	rcx, QWORD PTR 448[rsp]
	vinserti128	ymm3, ymm3, xmm10, 0x1
	vpsrlvq	ymm13, ymm1, ymm15
	vpxor	ymm2, ymm2, ymm7
	xor	rcx, QWORD PTR 392[rsp]
	vpxor	ymm3, ymm3, ymm9
	xor	r10, rbx
	rorx	rbx, rbx, 35
	mov	rsi, QWORD PTR 416[rsp]
	xor	rsi, QWORD PTR 408[rsp]
	rorx	rdi, rdi, 17
	xor	rdi, r8
	add	r14, 64
	vpsrlvq	ymm10, ymm3, YMMWORD PTR 544[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 512[rsp]
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm7, ymm2, YMMWORD PTR 576[rsp]
	vpsllvq	ymm2, ymm2, YMMWORD PTR 480[rsp]
	vpor	ymm1, ymm1, ymm13
	vmovq	xmm13, rdi
	vpor	ymm3, ymm3, ymm10
	vpunpcklqdq	xmm14, xmm13, xmm13
	vmovdqa	ymm13, YMMWORD PTR 512[rsp]
	vpaddq	ymm9, ymm1, ymm3
	vmovq	xmm3, rcx
	vmovq	xmm1, r10
	mov	rcx, rsi
	vpunpcklqdq	xmm3, xmm3, xmm3
	mov	rsi, QWORD PTR 400[rsp]
	vpunpcklqdq	xmm1, xmm1, xmm1
	vpor	ymm2, ymm2, ymm7
	vinserti128	ymm1, ymm1, xmm3, 0x1
	vpxor	ymm1, ymm1, ymm4
	xor	rsi, r15
	vpsrlvq	ymm3, ymm1, ymm11
	rorx	r10, rsi, 58
	vpsllvq	ymm1, ymm1, ymm12
	add	rbx, r10
	rorx	r10, QWORD PTR 392[rsp], 1
	rorx	rsi, rsi, 52
	add	rsi, r10
	rorx	r10, r13, 50
	mov	QWORD PTR 392[rsp], rsi
	rorx	rsi, rcx, 45
	rorx	rcx, rcx, 36
	add	r10, rsi
	rorx	rsi, r11, 21
	lea	r13, [rsi+rcx]
	mov	rcx, QWORD PTR -16[r14]
	vpor	ymm1, ymm1, ymm3
	mov	QWORD PTR 384[rsp], r13
	vpaddq	ymm2, ymm2, ymm1
	rorx	r11, r9, 58
	xor	rcx, r8
	vpxor	ymm4, ymm9, ymm2
	vperm2i128	ymm1, ymm4, ymm4, 1
	rorx	r8, r8, 17
	mov	r13, QWORD PTR 304[rsp]
	xor	rcx, r12
	rorx	r9, r9, 56
	xor	r11, QWORD PTR 424[rsp]
	vmovq	xmm3, rcx
	vpxor	ymm4, ymm4, ymm1
	xor	r13, r11
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpxor	ymm0, ymm0, ymm4
	vinserti128	ymm1, ymm14, XMMWORD PTR 288[rsp], 0x1
	vmovq	xmm7, r13
	vpunpcklqdq	xmm7, xmm7, xmm7
	vinserti128	ymm3, ymm3, xmm7, 0x1
	vpxor	ymm1, ymm1, ymm4
	vpxor	ymm3, ymm3, ymm8
	vpsrlvq	ymm10, ymm1, ymm15
	vmovdqa	ymm8, YMMWORD PTR 544[rsp]
	vpsllvq	ymm1, ymm1, ymm13
	vpsrlvq	ymm7, ymm3, ymm8
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	vpor	ymm1, ymm1, ymm10
	vpor	ymm3, ymm3, ymm7
	vpsrlvq	ymm7, ymm0, YMMWORD PTR 576[rsp]
	mov	rcx, QWORD PTR 440[rsp]
	xor	rcx, QWORD PTR 352[rsp]
	vpaddq	ymm1, ymm1, ymm3
	mov	rsi, QWORD PTR 448[rsp]
	vmovq	xmm4, rcx
	mov	rcx, QWORD PTR 376[rsp]
	xor	r8, QWORD PTR 424[rsp]
	vpunpcklqdq	xmm4, xmm4, xmm4
	xor	r11, QWORD PTR -8[r14]
	xor	r11, QWORD PTR 320[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 480[rsp]
	xor	rsi, rcx
	rorx	rcx, rcx, 35
	vmovq	xmm3, rsi
	mov	rsi, QWORD PTR 392[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	rsi, rbx
	vinserti128	ymm3, ymm3, xmm4, 0x1
	vpor	ymm0, ymm0, ymm7
	mov	QWORD PTR 272[rsp], rsi
	mov	rsi, QWORD PTR 384[rsp]
	vpxor	ymm3, ymm3, ymm6
	vmovq	xmm6, r8
	vpsrlvq	ymm4, ymm3, ymm11
	vpsllvq	ymm3, ymm3, ymm12
	vpunpcklqdq	xmm6, xmm6, xmm6
	xor	rsi, r10
	vinserti128	ymm10, ymm6, xmm14, 0x1
	rorx	r13, rsi, 58
	add	rcx, r13
	vpor	ymm3, ymm3, ymm4
	rorx	rsi, rsi, 52
	mov	QWORD PTR 376[rsp], rcx
	vpaddq	ymm0, ymm0, ymm3
	rorx	rcx, QWORD PTR 352[rsp], 1
	add	rsi, rcx
	mov	QWORD PTR 352[rsp], rsi
	rorx	rsi, r12, 50
	mov	r12, QWORD PTR 272[rsp]
	vpxor	ymm4, ymm1, ymm0
	vperm2i128	ymm3, ymm4, ymm4, 1
	mov	r13, QWORD PTR 432[rsp]
	xor	r13, QWORD PTR 336[rsp]
	rorx	rcx, r12, 45
	add	rsi, rcx
	rorx	rcx, QWORD PTR 304[rsp], 21
	rorx	r12, r12, 36
	add	rcx, r12
	vpxor	ymm4, ymm4, ymm3
	mov	r12, QWORD PTR 448[rsp]
	xor	r12, QWORD PTR 328[rsp]
	vpxor	ymm10, ymm10, ymm4
	vpxor	ymm5, ymm5, ymm4
	vmovq	xmm3, r11
	vpsrlvq	ymm7, ymm10, ymm15
	vmovq	xmm4, r13
	vmovdqa	XMMWORD PTR 304[rsp], xmm6
	vmovq	xmm6, r12
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpunpcklqdq	xmm4, xmm4, xmm4
	mov	r13, rsi
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpsllvq	ymm10, ymm10, ymm13
	xor	r13, rcx
	mov	r11, QWORD PTR 368[rsp]
	vinserti128	ymm3, ymm3, xmm6, 0x1
	mov	r12, QWORD PTR 440[rsp]
	vpxor	ymm3, ymm3, ymm9
	vpor	ymm10, ymm10, ymm7
	vpsrlvq	ymm6, ymm3, ymm8
	xor	r12, r11
	rorx	r11, r11, 35
	mov	QWORD PTR 368[rsp], r11
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	rorx	r11, r13, 58
	add	r11, QWORD PTR 368[rsp]
	rorx	r13, r13, 52
	mov	QWORD PTR 368[rsp], r11
	vpor	ymm3, ymm3, ymm6
	vpsrlvq	ymm6, ymm5, YMMWORD PTR 576[rsp]
	vpsllvq	ymm5, ymm5, YMMWORD PTR 480[rsp]
	vpaddq	ymm10, ymm10, ymm3
	vmovq	xmm3, r12
	mov	r12, QWORD PTR 376[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	r12, QWORD PTR 352[rsp]
	rorx	r11, QWORD PTR 336[rsp], 1
	add	r11, r13
	vinserti128	ymm3, ymm3, xmm4, 0x1
	rorx	r13, r12, 45
	mov	QWORD PTR 336[rsp], r11
	rorx	r12, r12, 36
	vpxor	ymm3, ymm3, ymm2
	vpor	ymm5, ymm5, ymm6
	rorx	r11, QWORD PTR 320[rsp], 50
	add	r11, r13
	vpsrlvq	ymm2, ymm3, ymm11
	vpsllvq	ymm3, ymm3, ymm12
	mov	QWORD PTR 320[rsp], r11
	rorx	r11, QWORD PTR 328[rsp], 21
	lea	r13, [r11+r12]
	rorx	r11, QWORD PTR 424[rsp], 17
	xor	r11, QWORD PTR 448[rsp]
	vpor	ymm3, ymm3, ymm2
	mov	QWORD PTR 328[rsp], r13
	mov	QWORD PTR 424[rsp], r11
	vpaddq	ymm13, ymm5, ymm3
	cmp	r14, QWORD PTR 240[rsp]
	jne	.L45
	vpunpcklqdq	ymm3, ymm1, ymm0
	vpunpcklqdq	ymm2, ymm10, ymm13
	vpunpckhqdq	ymm1, ymm1, ymm0
	vmovdqa	ymm7, YMMWORD PTR 192[rsp]
	vpunpckhqdq	ymm0, ymm10, ymm13
	mov	rax, QWORD PTR 32[rsp]
	vpermq	ymm3, ymm3, 216
	vpermq	ymm1, ymm1, 216
	vpermq	ymm2, ymm2, 216
	vpermq	ymm0, ymm0, 216
	vpshufb	ymm3, ymm3, ymm7
	vpshufb	ymm2, ymm2, ymm7
	vpshufb	ymm1, ymm1, ymm7
	vpshufb	ymm0, ymm0, ymm7
	vmovdqu	YMMWORD PTR [rax], ymm3
	vmovdqu	YMMWORD PTR 32[rax], ymm2
	vmovdqu	YMMWORD PTR 64[rax], ymm1
	xor	eax, eax
	vzeroupper

.L1:
	mov	rdx, QWORD PTR 984[rsp]
	sub	rdx, QWORD PTR fs:40
	jne	.L220
	lea	rsp, -40[rbp]
	pop	rbx
	pop	r12
	pop	r13
	pop	r14
	pop	r15
	pop	rbp
	ret
.L39:
	lea	eax, 3[rdx]
	cmp	eax, 7
	ja	.L42
	mov	eax, edx
	vpxor	xmm0, xmm0, xmm0
	lea	ecx, 4[rdx]
	vmovdqa	YMMWORD PTR 896[rsp+rax*8], ymm0
	mov	eax, edx
	.p2align 4,,10
	.p2align 3
.L43:
	mov	edx, 4
	sub	edx, eax
	and	edx, -4
	add	edx, ecx
.L42:
	cmp	edx, 7
	ja	.L41
	mov	eax, 8
	xor	esi, esi
	sub	eax, edx
	lea	rdx, 896[rsp+rdx*8]
	sal	rax, 3
	mov	QWORD PTR [rdx], 0
	lea	rdi, 8[rdx]
	mov	ecx, eax
	sub	eax, 8
	mov	QWORD PTR -8[rdx+rcx], 0
	cmp	eax, 8
	jb	.L41
	and	eax, -8
	xor	edx, edx
.L47:
	mov	ecx, edx
	add	edx, 8
	mov	QWORD PTR [rdi+rcx], rsi
	cmp	edx, eax
	jb	.L47
	jmp	.L41
.L32:
	lea	edx, 0[0+rax*8]
	test	edi, edi
	je	.L60
	mov	rbx, QWORD PTR 48[rsp]
	movzx	ecx, BYTE PTR [rbx+rdx]
	sal	rcx, 56
	cmp	edi, 1
	je	.L35
	movzx	esi, BYTE PTR 1[rbx+rdx]
	sal	rsi, 48
	or	rcx, rsi
	cmp	edi, 2
	je	.L35
	movzx	esi, BYTE PTR 2[rbx+rdx]
	sal	rsi, 40
	or	rcx, rsi
	cmp	edi, 3
	je	.L35
	movzx	esi, BYTE PTR 3[rbx+rdx]
	sal	rsi, 32
	or	rcx, rsi
	cmp	edi, 4
	je	.L35
	movzx	esi, BYTE PTR 4[rbx+rdx]
	sal	rsi, 24
	or	rcx, rsi
	cmp	edi, 5
	je	.L35
	movzx	esi, BYTE PTR 5[rbx+rdx]
	sal	rsi, 16
	or	rcx, rsi
	cmp	edi, 7
	jne	.L35
	movzx	esi, BYTE PTR 6[rbx+rdx]
	sal	rsi, 8
	or	rcx, rsi
.L35:
	test	r9d, r9d
	je	.L37
	mov	r11d, 8
	add	rdx, QWORD PTR 48[rsp]
	mov	r10d, edi
	mov	esi, 255
	sub	r11d, r9d
	shlx	esi, esi, r11d
	and	sil, BYTE PTR [rdx+r10]
	movzx	edx, sil
	mov	esi, 7
	sub	esi, edi
	sal	esi, 3
	shlx	rdx, rdx, rsi
	or	rcx, rdx
.L37:
	mov	QWORD PTR 896[rsp+r8*8], rcx
	add	eax, 1
	jmp	.L33

.L219:
	lea	rbx, 640[rsp]
	mov	r12, QWORD PTR 48[rsp]
	xor	edx, edx
	vmovdqa	ymm5, YMMWORD PTR 192[rsp]
	mov	rsi, rbx
	.p2align 4,,10
	.p2align 3
.L8:
	lea	ecx, 0[0+rdx*8]
	add	rsi, 32
	vmovdqu	ymm3, YMMWORD PTR [r12+rcx]
	mov	r11, rcx
	mov	ecx, edx
	add	edx, 4
	add	ecx, 7
	vpshufb	ymm3, ymm3, ymm5
	vmovdqa	YMMWORD PTR -32[rsi], ymm3
	cmp	ecx, eax
	jb	.L8
	cmp	edx, eax
	jnb	.L14
	mov	esi, eax
	sub	esi, edx
	cmp	esi, 1
	je	.L12
	lea	ecx, 32[r11]
	add	rcx, QWORD PTR 48[rsp]
	vmovdqa	xmm5, XMMWORD PTR .LC16[rip]
	mov	r11d, edx
	vmovdqu	xmm7, XMMWORD PTR [rcx]
	lea	r11, [rbx+r11*8]
	mov	ebx, esi
	shr	ebx
	vpshufb	xmm3, xmm7, xmm5
	vmovdqa	XMMWORD PTR [r11], xmm3
	cmp	ebx, 1
	jne	.L221
.L13:
	mov	ecx, esi
	and	ecx, -2
	add	edx, ecx
	cmp	ecx, esi
	je	.L14
.L12:
	mov	rsi, QWORD PTR 48[rsp]
	lea	ecx, 0[0+rdx*8]
	mov	rcx, QWORD PTR [rsi+rcx]
	bswap	rcx
	mov	QWORD PTR 640[rsp+rdx*8], rcx
.L14:
	test	r10d, r10d
	je	.L222
	lea	edx, 0[0+rax*8]
	test	edi, edi
	je	.L57
	mov	rbx, QWORD PTR 48[rsp]
	movzx	ecx, BYTE PTR [rbx+rdx]
	sal	rcx, 56
	cmp	edi, 1
	je	.L16
	movzx	esi, BYTE PTR 1[rbx+rdx]
	sal	rsi, 48
	or	rcx, rsi
	cmp	edi, 2
	je	.L16
	movzx	esi, BYTE PTR 2[rbx+rdx]
	sal	rsi, 40
	or	rcx, rsi
	cmp	edi, 3
	je	.L16
	movzx	esi, BYTE PTR 3[rbx+rdx]
	sal	rsi, 32
	or	rcx, rsi
	cmp	edi, 4
	je	.L16
	movzx	esi, BYTE PTR 4[rbx+rdx]
	sal	rsi, 24
	or	rcx, rsi
	cmp	edi, 5
	je	.L16
	movzx	esi, BYTE PTR 5[rbx+rdx]
	sal	rsi, 16
	or	rcx, rsi
	cmp	edi, 7
	jne	.L16
	movzx	esi, BYTE PTR 6[rbx+rdx]
	sal	rsi, 8
	or	rcx, rsi
.L16:
	test	r9d, r9d
	jne	.L223
.L18:
	add	eax, 1
	mov	QWORD PTR 640[rsp+r8*8], rcx
	test	al, 3
	je	.L19
	cmp	eax, 16
	je	.L23
.L20:
	mov	edx, eax
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 1[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 2[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 3[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 4[rax]
	test	al, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 5[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 6[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 7[rax]
	test	dl, 3
	je	.L22
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 8[rax]
	test	al, 3
	je	.L58
	cmp	edx, 16
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	add	eax, 9
	test	al, 3
	je	.L19

.L23:
	mov	rax, QWORD PTR 640[rsp]
	vpunpcklqdq	ymm5, ymm4, ymm0
	vpunpckhqdq	ymm0, ymm4, ymm0
	vpunpcklqdq	ymm3, ymm2, ymm1
	vpermq	ymm0, ymm0, 216
	vpunpckhqdq	ymm1, ymm2, ymm1
	vpermq	ymm5, ymm5, 216
	mov	rbx, QWORD PTR 672[rsp]
	mov	QWORD PTR 432[rsp], rax
	vpermq	ymm1, ymm1, 216
	vpermq	ymm2, ymm5, 80
	mov	rax, QWORD PTR 648[rsp]
	vpextrq	r14, xmm1, 1
	vpextrq	QWORD PTR 448[rsp], xmm0, 1
	mov	r13, QWORD PTR 752[rsp]
	vmovq	rdi, xmm1
	vpermq	ymm10, ymm5, 250
	vpermq	ymm3, ymm3, 216
	mov	r8, r14
	mov	QWORD PTR 424[rsp], rax
	mov	rax, QWORD PTR 656[rsp]
	vpermq	ymm13, ymm3, 80
	vpermq	ymm14, ymm3, 250
	vpxor	ymm14, ymm14, YMMWORD PTR .LC7[rip]
	vmovdqa	ymm9, ymm2
	vmovdqa	YMMWORD PTR 768[rsp], ymm9
	vmovdqa	YMMWORD PTR 800[rsp], ymm10
	vmovdqa	YMMWORD PTR 832[rsp], ymm13
	vmovdqa	YMMWORD PTR 864[rsp], ymm14
	mov	r15, QWORD PTR 704[rsp]
	lea	r11, E_CONST[rip]
	mov	QWORD PTR 288[rsp], rax
	mov	rax, QWORD PTR 664[rsp]
	mov	QWORD PTR 376[rsp], rax
	mov	rax, QWORD PTR 680[rsp]
	mov	QWORD PTR 328[rsp], rax
	mov	rax, QWORD PTR 688[rsp]
	mov	QWORD PTR 416[rsp], rax
	mov	rax, QWORD PTR 696[rsp]
	mov	QWORD PTR 408[rsp], rax
	mov	rax, QWORD PTR 712[rsp]
	mov	QWORD PTR 368[rsp], rax
	mov	rax, QWORD PTR 720[rsp]
	mov	QWORD PTR 320[rsp], rax
	mov	rax, QWORD PTR 728[rsp]
	mov	QWORD PTR 304[rsp], rax
	mov	rax, QWORD PTR 736[rsp]
	mov	QWORD PTR 392[rsp], rax
	mov	rax, QWORD PTR 744[rsp]
	mov	QWORD PTR 384[rsp], rax
	mov	rax, QWORD PTR 760[rsp]
	mov	QWORD PTR 400[rsp], rax
	vmovq	rax, xmm0
	vextracti128	xmm0, ymm0, 0x1
	mov	rdx, QWORD PTR 448[rsp]
	vmovq	rcx, xmm0
	mov	r10, QWORD PTR 40[rsp]
	vpextrq	QWORD PTR 448[rsp], xmm0, 1
	vmovdqa	xmm0, xmm1
	vextracti128	xmm1, ymm1, 0x1
	mov	QWORD PTR 440[rsp], rdx
	mov	rsi, QWORD PTR 448[rsp]
	vpbroadcastq	xmm5, xmm1
	mov	QWORD PTR 264[rsp], r13
	vmovq	r9, xmm1
	vpextrq	QWORD PTR 448[rsp], xmm1, 1
	vmovdqa	XMMWORD PTR 336[rsp], xmm5
	vpbroadcastq	xmm5, xmm0
	vmovdqa	XMMWORD PTR 272[rsp], xmm5
	vmovq	xmm5, r14
	vpunpcklqdq	xmm5, xmm5, xmm5
	mov	QWORD PTR 256[rsp], rbx
	mov	rbx, r10
	vmovdqa	XMMWORD PTR 352[rsp], xmm5
	.p2align 4,,10
	.p2align 3

.L21:
	mov	r10, QWORD PTR 424[rsp]
	vpxor	ymm1, ymm14, ymm13
	xor	r10, QWORD PTR 440[rsp]
	vperm2i128	ymm0, ymm1, ymm1, 1
	mov	r14, r10
	mov	r10, QWORD PTR [r11]
	xor	r14, rbx
	vpxor	ymm1, ymm1, ymm0
	vmovddup	xmm4, QWORD PTR 448[rsp]
	vinserti128	ymm2, ymm4, XMMWORD PTR 336[rsp], 0x1
	vmovq	xmm3, r14
	mov	r14, QWORD PTR 432[rsp]
	vmovdqa	ymm7, YMMWORD PTR 512[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpxor	ymm5, ymm1, ymm2
	mov	rdx, QWORD PTR 368[rsp]
	mov	r12, QWORD PTR 320[rsp]
	xor	r14, rax
	vpsrlvq	ymm6, ymm5, ymm15
	xor	r12, QWORD PTR 304[rsp]
	rorx	rax, rax, 17
	xor	r10, r14
	vpsllvq	ymm5, ymm5, ymm7
	mov	r13, rdx
	rorx	r14, r12, 58
	vmovq	xmm0, r10
	mov	r10, r15
	xor	r13, rsi
	rorx	r12, r12, 52
	vpunpcklqdq	xmm0, xmm0, xmm0
	vpor	ymm5, ymm5, ymm6
	xor	r10, rcx
	vmovdqa	xmm6, XMMWORD PTR 352[rsp]
	vinserti128	ymm0, ymm0, xmm3, 0x1
	vpxor	ymm0, ymm9, ymm0
	vpsrlvq	ymm3, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpor	ymm0, ymm0, ymm3
	vmovq	xmm3, r13
	rorx	r13, r15, 35
	lea	r15, [r14+r13]
	vpaddq	ymm5, ymm5, ymm0
	vpunpcklqdq	xmm3, xmm3, xmm3
	rorx	r13, rdx, 1
	add	r12, r13
	mov	rdx, QWORD PTR 440[rsp]
	mov	QWORD PTR 368[rsp], r12
	rorx	r12, QWORD PTR 432[rsp], 50
	vinserti128	ymm0, ymm6, XMMWORD PTR 272[rsp], 0x1
	xor	rax, rdx
	vpxor	ymm1, ymm0, ymm1
	vmovq	xmm0, r10
	mov	r10, QWORD PTR 288[rsp]
	xor	r10, QWORD PTR 376[rsp]
	vpunpcklqdq	xmm0, xmm0, xmm0
	rorx	r13, r10, 45
	rorx	r10, r10, 36
	vpsrlvq	ymm6, ymm1, YMMWORD PTR 576[rsp]
	vinserti128	ymm0, ymm0, xmm3, 0x1
	lea	r14, 0[r13+r12]
	rorx	r12, QWORD PTR 424[rsp], 21
	lea	r13, [r10+r12]
	vpxor	ymm0, ymm10, ymm0
	mov	r10, QWORD PTR 408[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 480[rsp]
	vpsrlvq	ymm3, ymm0, ymm11
	vpsllvq	ymm0, ymm0, ymm12
	xor	r10, rcx
	vpor	ymm0, ymm0, ymm3
	vpor	ymm1, ymm1, ymm6
	vmovq	xmm3, rax
	vpaddq	ymm1, ymm1, ymm0
	vpxor	ymm6, ymm5, ymm1
	vpunpcklqdq	xmm3, xmm3, xmm3
	vperm2i128	ymm0, ymm6, ymm6, 1
	vinserti128	ymm4, ymm3, xmm4, 0x1
	vpxor	ymm6, ymm6, ymm0
	vpxor	ymm8, ymm6, ymm4
	vpsrlvq	ymm9, ymm8, ymm15
	vpsllvq	ymm8, ymm8, ymm7
	vmovq	xmm7, r10
	mov	r10, rdx
	xor	r10, rbx
	xor	r10, QWORD PTR 416[rsp]
	xor	r10, QWORD PTR 8[r11]
	vmovq	xmm10, r10
	vpunpcklqdq	xmm7, xmm7, xmm7
	vpor	ymm8, ymm8, ymm9
	vmovdqa	xmm9, XMMWORD PTR 336[rsp]
	vpunpcklqdq	xmm0, xmm10, xmm10
	mov	r10, QWORD PTR 400[rsp]
	vinserti128	ymm0, ymm0, xmm7, 0x1
	vpxor	ymm0, ymm0, ymm13
	xor	r10, rdi
	vpsrlvq	ymm7, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpor	ymm0, ymm0, ymm7
	vmovq	xmm7, r10
	vpaddq	ymm8, ymm8, ymm0
	vpunpcklqdq	xmm7, xmm7, xmm7
	vinserti128	ymm0, ymm9, XMMWORD PTR 352[rsp], 0x1
	vpxor	ymm6, ymm0, ymm6
	vpsrlvq	ymm9, ymm6, YMMWORD PTR 576[rsp]
	vpsllvq	ymm6, ymm6, YMMWORD PTR 480[rsp]
	mov	rdx, QWORD PTR 264[rsp]
	mov	QWORD PTR 432[rsp], r14
	xor	r14, r13
	mov	r12, r14
	mov	QWORD PTR 352[rsp], r15
	rorx	r14, r14, 58
	mov	r10, rdx
	mov	QWORD PTR 424[rsp], r13
	rorx	r12, r12, 52
	rorx	r13, rdx, 35
	xor	r10, rsi
	add	r14, r13
	vpor	ymm6, ymm6, ymm9
	rorx	r13, QWORD PTR 400[rsp], 1
	vmovq	xmm0, r10
	mov	r10, QWORD PTR 368[rsp]
	rorx	rdx, QWORD PTR 440[rsp], 17
	mov	QWORD PTR 264[rsp], r14
	vpunpcklqdq	xmm0, xmm0, xmm0
	xor	r10, r15
	lea	r15, [r12+r13]
	vinserti128	ymm0, ymm0, xmm7, 0x1
	rorx	r12, QWORD PTR 416[rsp], 50
	mov	QWORD PTR 400[rsp], r15
	rorx	r13, r10, 45
	rorx	r10, r10, 36
	lea	r15, 0[r13+r12]
	vpxor	ymm0, ymm0, ymm14
	rorx	r13, rbx, 62
	rorx	r12, QWORD PTR 408[rsp], 21
	add	r10, r12
	mov	QWORD PTR 408[rsp], r10
	vpsrlvq	ymm7, ymm0, ymm11
	vpsllvq	ymm0, ymm0, ymm12
	mov	r10, QWORD PTR 384[rsp]
	mov	r12, rdx
	mov	rdx, QWORD PTR 256[rsp]
	mov	QWORD PTR 416[rsp], r15
	xor	r10, rsi
	vpor	ymm0, ymm0, ymm7
	xor	r12, rcx
	xor	r10, r13
	vpaddq	ymm6, ymm6, ymm0
	vpxor	ymm10, ymm8, ymm6
	vmovq	xmm14, r12
	mov	QWORD PTR 440[rsp], r12
	vmovq	xmm13, r10
	vperm2i128	ymm0, ymm10, ymm10, 1
	mov	r10, QWORD PTR 392[rsp]
	vpunpcklqdq	xmm9, xmm14, xmm14
	vinserti128	ymm3, ymm9, xmm3, 0x1
	mov	r12, QWORD PTR 328[rsp]
	xor	r10, rcx
	xor	r10, QWORD PTR 16[r11]
	vpxor	ymm10, ymm10, ymm0
	rorx	rcx, rcx, 17
	vmovq	xmm0, r10
	vpunpcklqdq	xmm13, xmm13, xmm13
	vpxor	ymm7, ymm10, ymm3
	xor	r12, r8
	vpunpcklqdq	xmm0, xmm0, xmm0
	vpsrlvq	ymm14, ymm7, ymm15
	vpxor	ymm2, ymm2, ymm10
	mov	r10, rdx
	vinserti128	ymm0, ymm0, xmm13, 0x1
	xor	r10, rdi
	vpsllvq	ymm7, ymm7, YMMWORD PTR 512[rsp]
	vpxor	ymm0, ymm0, ymm5
	vpsrlvq	ymm5, ymm0, YMMWORD PTR 544[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm10, ymm2, YMMWORD PTR 576[rsp]
	vpsllvq	ymm2, ymm2, YMMWORD PTR 480[rsp]
	vpor	ymm7, ymm7, ymm14
	vpor	ymm0, ymm0, ymm5
	vmovq	xmm5, r12
	mov	r12, QWORD PTR 408[rsp]
	vpaddq	ymm7, ymm7, ymm0
	vmovq	xmm0, r10
	mov	r10, QWORD PTR 400[rsp]
	vpunpcklqdq	xmm5, xmm5, xmm5
	xor	r12, r15
	vpunpcklqdq	xmm0, xmm0, xmm0
	vpor	ymm2, ymm2, ymm10
	xor	r10, r14
	rorx	r15, r12, 58
	rorx	r14, rdx, 35
	add	r14, r15
	mov	QWORD PTR 336[rsp], r14
	rorx	r12, r12, 52
	vinserti128	ymm0, ymm0, xmm5, 0x1
	rorx	r14, QWORD PTR 328[rsp], 1
	add	r12, r14
	rorx	r14, r10, 45
	rorx	r10, r10, 36
	mov	QWORD PTR 328[rsp], r12
	vpxor	ymm0, ymm0, ymm1
	rorx	r12, QWORD PTR 392[rsp], 50
	add	r12, r14
	mov	QWORD PTR 392[rsp], r12
	vpsrlvq	ymm1, ymm0, ymm11
	vpsllvq	ymm0, ymm0, ymm12
	rorx	r12, QWORD PTR 384[rsp], 21
	lea	r15, [r10+r12]
	xor	rcx, rsi
	mov	QWORD PTR 384[rsp], r15
	mov	r15, QWORD PTR 288[rsp]
	vpor	ymm0, ymm0, ymm1
	mov	r14, QWORD PTR 376[rsp]
	vpaddq	ymm5, ymm2, ymm0
	vpxor	ymm10, ymm7, ymm5
	mov	rdx, QWORD PTR 304[rsp]
	vmovq	xmm2, rcx
	mov	r10, r15
	vperm2i128	ymm0, ymm10, ymm10, 1
	mov	r12, QWORD PTR 392[rsp]
	xor	r12, QWORD PTR 384[rsp]
	xor	r10, rsi
	xor	r14, rdi
	rorx	rsi, rsi, 17
	xor	r10, r13
	xor	r10, QWORD PTR 24[r11]
	vmovq	xmm13, r14
	mov	r13, rdx
	vmovq	xmm1, r10
	vpunpcklqdq	xmm13, xmm13, xmm13
	xor	r13, r9
	vpunpcklqdq	xmm1, xmm1, xmm1
	vpxor	ymm10, ymm10, ymm0
	vpunpcklqdq	xmm0, xmm2, xmm2
	xor	rsi, rdi
	vinserti128	ymm1, ymm1, xmm13, 0x1
	vinserti128	ymm2, ymm0, xmm9, 0x1
	mov	r10, QWORD PTR 336[rsp]
	vpxor	ymm4, ymm4, ymm10
	vpxor	ymm1, ymm1, ymm8
	vpxor	ymm9, ymm10, ymm2
	xor	r10, QWORD PTR 328[rsp]
	vpsrlvq	ymm8, ymm1, YMMWORD PTR 544[rsp]
	vpsrlvq	ymm14, ymm9, ymm15
	vpsllvq	ymm1, ymm1, YMMWORD PTR 608[rsp]
	vpsllvq	ymm9, ymm9, YMMWORD PTR 512[rsp]
	vpsrlvq	ymm10, ymm4, YMMWORD PTR 576[rsp]
	vpsllvq	ymm4, ymm4, YMMWORD PTR 480[rsp]
	vpor	ymm1, ymm1, ymm8
	vmovq	xmm8, r13
	vpor	ymm9, ymm9, ymm14
	mov	r13, QWORD PTR 320[rsp]
	vpaddq	ymm9, ymm9, ymm1
	vpunpcklqdq	xmm8, xmm8, xmm8
	vpor	ymm4, ymm4, ymm10
	mov	r14, r13
	rorx	r13, r13, 35
	xor	r14, r8
	vmovq	xmm1, r14
	rorx	r14, r12, 58
	rorx	r12, r12, 52
	add	r14, r13
	vpunpcklqdq	xmm1, xmm1, xmm1
	rorx	r13, rdx, 1
	add	r13, r12
	rorx	r12, r15, 50
	vinserti128	ymm1, ymm1, xmm8, 0x1
	mov	QWORD PTR 304[rsp], r13
	rorx	r13, r10, 45
	rorx	r10, r10, 36
	add	r13, r12
	vpxor	ymm1, ymm1, ymm6
	rorx	r12, QWORD PTR 376[rsp], 21
	lea	r15, [r10+r12]
	vpsrlvq	ymm6, ymm1, ymm11
	rorx	r10, rbx, 60
	mov	QWORD PTR 376[rsp], r15
	vpsllvq	ymm1, ymm1, ymm12
	mov	r15, r10
	mov	r12, QWORD PTR 352[rsp]
	mov	rdx, QWORD PTR 448[rsp]
	mov	QWORD PTR 320[rsp], r14
	xor	r15, r8
	xor	r15, QWORD PTR 368[rsp]
	vpor	ymm1, ymm1, ymm6
	mov	r14, r13
	vmovq	xmm13, r15
	mov	r15, QWORD PTR 32[r11]
	vpaddq	ymm4, ymm4, ymm1
	vpxor	ymm10, ymm9, ymm4
	vmovq	xmm6, rsi
	vperm2i128	ymm1, ymm10, ymm10, 1
	vpunpcklqdq	xmm13, xmm13, xmm13
	mov	r13, QWORD PTR 424[rsp]
	mov	QWORD PTR 288[rsp], r14
	xor	r15, rdi
	rorx	rdi, rdi, 17
	xor	r12, r15
	vpxor	ymm10, ymm10, ymm1
	vpunpcklqdq	xmm1, xmm6, xmm6
	xor	r13, rdx
	vmovq	xmm6, r12
	vinserti128	ymm0, ymm1, xmm0, 0x1
	vpxor	ymm3, ymm3, ymm10
	xor	rdi, r8
	vpunpcklqdq	xmm6, xmm6, xmm6
	vpxor	ymm8, ymm10, ymm0
	mov	r12, QWORD PTR 432[rsp]
	vinserti128	ymm6, ymm6, xmm13, 0x1
	vpsrlvq	ymm14, ymm8, ymm15
	vpsllvq	ymm8, ymm8, YMMWORD PTR 512[rsp]
	vpxor	ymm6, ymm6, ymm7
	mov	r15, r12
	rorx	r12, r12, 35
	vpsrlvq	ymm7, ymm6, YMMWORD PTR 544[rsp]
	xor	r15, r9
	vpsllvq	ymm6, ymm6, YMMWORD PTR 608[rsp]
	vpsrlvq	ymm10, ymm3, YMMWORD PTR 576[rsp]
	vpsllvq	ymm3, ymm3, YMMWORD PTR 480[rsp]
	vpor	ymm8, ymm8, ymm14
	vpor	ymm6, ymm6, ymm7
	vmovq	xmm7, r13
	mov	r13, QWORD PTR 320[rsp]
	xor	r13, QWORD PTR 304[rsp]
	vpaddq	ymm8, ymm8, ymm6
	vmovq	xmm6, r15
	vpunpcklqdq	xmm7, xmm7, xmm7
	xor	r14, QWORD PTR 376[rsp]
	vpunpcklqdq	xmm6, xmm6, xmm6
	rorx	r15, r14, 58
	rorx	r14, r14, 52
	add	r15, r12
	vinserti128	ymm6, ymm6, xmm7, 0x1
	mov	QWORD PTR 432[rsp], r15
	vpor	ymm3, ymm3, ymm10
	xor	r10, QWORD PTR 40[r11]
	vpxor	ymm6, ymm6, ymm5
	vmovq	xmm10, rdi
	xor	r10, r8
	rorx	r8, r8, 17
	vpsrlvq	ymm7, ymm6, ymm11
	vpsllvq	ymm6, ymm6, ymm12
	xor	r10, QWORD PTR 264[rsp]
	rorx	r12, QWORD PTR 424[rsp], 1
	vmovdqa	ymm14, YMMWORD PTR 512[rsp]
	add	r14, r12
	rorx	r12, r13, 45
	rorx	r13, r13, 36
	mov	QWORD PTR 424[rsp], r14
	vpor	ymm6, ymm6, ymm7
	rorx	r14, QWORD PTR 352[rsp], 50
	lea	r15, [r12+r14]
	vpaddq	ymm5, ymm3, ymm6
	vpxor	ymm7, ymm8, ymm5
	vpunpcklqdq	xmm10, xmm10, xmm10
	rorx	r12, QWORD PTR 368[rsp], 21
	add	r13, r12
	vperm2i128	ymm3, ymm7, ymm7, 1
	vmovdqa	XMMWORD PTR 272[rsp], xmm10
	vinserti128	ymm1, ymm10, xmm1, 0x1
	mov	QWORD PTR 368[rsp], r13
	mov	r13, QWORD PTR 400[rsp]
	vpxor	ymm7, ymm7, ymm3
	vmovq	xmm3, r10
	mov	r10, QWORD PTR 416[rsp]
	mov	r12, QWORD PTR 408[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpxor	ymm6, ymm7, ymm1
	vpxor	ymm2, ymm2, ymm7
	xor	r13, r9
	vmovq	xmm10, r13
	vpsrlvq	ymm13, ymm6, ymm15
	mov	r14, r10
	xor	r12, rax
	vpunpcklqdq	xmm10, xmm10, xmm10
	vpsllvq	ymm6, ymm6, ymm14
	xor	r14, rdx
	rorx	r10, r10, 35
	vinserti128	ymm3, ymm3, xmm10, 0x1
	vmovq	xmm7, r12
	mov	r12, QWORD PTR 432[rsp]
	xor	r12, QWORD PTR 424[rsp]
	vpxor	ymm3, ymm3, ymm9
	vpor	ymm6, ymm6, ymm13
	vpunpcklqdq	xmm7, xmm7, xmm7
	vpsrlvq	ymm9, ymm3, YMMWORD PTR 544[rsp]
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	vpor	ymm3, ymm3, ymm9
	vpsrlvq	ymm9, ymm2, YMMWORD PTR 576[rsp]
	vpsllvq	ymm2, ymm2, YMMWORD PTR 480[rsp]
	vpaddq	ymm6, ymm6, ymm3
	vmovq	xmm3, r14
	mov	r14, QWORD PTR 368[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	r14, r15
	vinserti128	ymm3, ymm3, xmm7, 0x1
	xor	r8, r9
	add	r11, 64
	vpxor	ymm3, ymm3, ymm4
	vpor	ymm2, ymm2, ymm9
	vmovq	xmm9, r8
	mov	r13, r14
	rorx	r14, r14, 58
	add	r10, r14
	vpsrlvq	ymm4, ymm3, ymm11
	rorx	r13, r13, 52
	mov	QWORD PTR 416[rsp], r10
	vpsllvq	ymm3, ymm3, ymm12
	vpunpcklqdq	xmm9, xmm9, xmm9
	rorx	r10, QWORD PTR 408[rsp], 1
	add	r10, r13
	vmovdqa	xmm13, xmm9
	rorx	r13, QWORD PTR 264[rsp], 50
	mov	QWORD PTR 408[rsp], r10
	rorx	r10, r12, 45
	add	r10, r13
	vpor	ymm3, ymm3, ymm4
	mov	QWORD PTR 264[rsp], r10
	rorx	r12, r12, 36
	rorx	r10, QWORD PTR 400[rsp], 21
	add	r12, r10
	vpaddq	ymm2, ymm2, ymm3
	vpxor	ymm4, ymm6, ymm2
	mov	QWORD PTR 400[rsp], r12
	mov	r12, QWORD PTR -16[r11]
	rorx	r10, rbx, 58
	vperm2i128	ymm3, ymm4, ymm4, 1
	xor	r10, rdx
	rorx	rbx, rbx, 56
	mov	r13, QWORD PTR 328[rsp]
	xor	r12, r9
	rorx	r9, r9, 17
	vmovdqa	XMMWORD PTR 352[rsp], xmm9
	vpxor	ymm4, ymm4, ymm3
	vinserti128	ymm9, ymm9, XMMWORD PTR 272[rsp], 0x1
	xor	r12, QWORD PTR 336[rsp]
	xor	r13, r10
	vmovq	xmm3, r12
	vpxor	ymm0, ymm0, ymm4
	mov	rdx, QWORD PTR 392[rsp]
	vmovq	xmm7, r13
	vpunpcklqdq	xmm3, xmm3, xmm3
	vpxor	ymm9, ymm9, ymm4
	mov	r13, QWORD PTR 384[rsp]
	vpunpcklqdq	xmm7, xmm7, xmm7
	vpsrlvq	ymm10, ymm9, ymm15
	mov	r14, rdx
	rorx	rdx, rdx, 35
	vinserti128	ymm3, ymm3, xmm7, 0x1
	vpsllvq	ymm9, ymm9, ymm14
	xor	r14, rax
	xor	r10, QWORD PTR -8[r11]
	vpxor	ymm3, ymm3, ymm8
	xor	r13, QWORD PTR 440[rsp]
	vmovdqa	ymm8, YMMWORD PTR 544[rsp]
	vpor	ymm9, ymm9, ymm10
	vmovq	xmm4, r13
	mov	r13, QWORD PTR 416[rsp]
	xor	r13, QWORD PTR 408[rsp]
	vpsrlvq	ymm7, ymm3, ymm8
	vpunpcklqdq	xmm4, xmm4, xmm4
	xor	r9, QWORD PTR 448[rsp]
	xor	r10, QWORD PTR 320[rsp]
	vpsllvq	ymm3, ymm3, YMMWORD PTR 608[rsp]
	vpor	ymm3, ymm3, ymm7
	vpsrlvq	ymm7, ymm0, YMMWORD PTR 576[rsp]
	vpsllvq	ymm0, ymm0, YMMWORD PTR 480[rsp]
	vpaddq	ymm9, ymm9, ymm3
	vmovq	xmm3, r14
	mov	r14, QWORD PTR 264[rsp]
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	r14, QWORD PTR 400[rsp]
	vinserti128	ymm3, ymm3, xmm4, 0x1
	rorx	r12, r14, 58
	add	r12, rdx
	rorx	r14, r14, 52
	vpxor	ymm3, ymm3, ymm5
	vpor	ymm0, ymm0, ymm7
	mov	QWORD PTR 392[rsp], r12
	rorx	r12, QWORD PTR 384[rsp], 1
	vpsrlvq	ymm4, ymm3, ymm11
	vpsllvq	ymm3, ymm3, ymm12
	add	r14, r12
	rorx	r12, QWORD PTR 336[rsp], 50
	mov	QWORD PTR 384[rsp], r14
	rorx	r14, r13, 45
	rorx	r13, r13, 36
	add	r14, r12
	vpor	ymm3, ymm3, ymm4
	mov	QWORD PTR 256[rsp], r14
	rorx	r12, QWORD PTR 328[rsp], 21
	add	r12, r13
	vpaddq	ymm10, ymm0, ymm3
	vpxor	ymm3, ymm9, ymm10
	mov	rdx, r12
	mov	QWORD PTR 328[rsp], r12
	vperm2i128	ymm0, ymm3, ymm3, 1
	mov	r12, QWORD PTR 304[rsp]
	mov	r14, QWORD PTR 440[rsp]
	vpxor	ymm3, ymm3, ymm0
	vmovq	xmm0, r9
	xor	r12, rax
	vpunpcklqdq	xmm0, xmm0, xmm0
	vmovq	xmm4, r12
	vpxor	ymm1, ymm1, ymm3
	mov	r12, QWORD PTR 376[rsp]
	vmovdqa	XMMWORD PTR 336[rsp], xmm0
	vinserti128	ymm13, ymm0, xmm13, 0x1
	vmovq	xmm0, r10
	vpunpcklqdq	xmm4, xmm4, xmm4
	vpunpcklqdq	xmm0, xmm0, xmm0
	vpxor	ymm13, ymm13, ymm3
	mov	r10, QWORD PTR 288[rsp]
	xor	r12, rcx
	vinserti128	ymm0, ymm0, xmm4, 0x1
	vpsrlvq	ymm5, ymm13, ymm15
	vmovq	xmm3, r12
	mov	r12, QWORD PTR 392[rsp]
	vpxor	ymm0, ymm0, ymm6
	vpsllvq	ymm13, ymm13, ymm14
	vpunpcklqdq	xmm3, xmm3, xmm3
	xor	r14, r10
	vpsrlvq	ymm4, ymm0, ymm8
	xor	r12, QWORD PTR 384[rsp]
	rorx	r10, r10, 35
	vpsllvq	ymm0, ymm0, YMMWORD PTR 608[rsp]
	vpor	ymm13, ymm13, ymm5
	vpor	ymm0, ymm0, ymm4
	vpsrlvq	ymm4, ymm1, YMMWORD PTR 576[rsp]
	vpsllvq	ymm1, ymm1, YMMWORD PTR 480[rsp]
	vpaddq	ymm13, ymm13, ymm0
	mov	r13, QWORD PTR 256[rsp]
	vmovq	xmm0, r14
	vpunpcklqdq	xmm0, xmm0, xmm0
	xor	r13, rdx
	vinserti128	ymm0, ymm0, xmm3, 0x1
	vpxor	ymm0, ymm0, ymm2
	rorx	r14, r13, 58
	lea	rdx, [r14+r10]
	rorx	r13, r13, 52
	vpsrlvq	ymm2, ymm0, ymm11
	mov	QWORD PTR 288[rsp], rdx
	vpsllvq	ymm0, ymm0, ymm12
	vpor	ymm1, ymm1, ymm4
	rorx	r10, QWORD PTR 376[rsp], 1
	lea	rdx, 0[r13+r10]
	rorx	r10, QWORD PTR 320[rsp], 50
	rorx	r13, r12, 45
	mov	QWORD PTR 376[rsp], rdx
	vpor	ymm0, ymm0, ymm2
	rorx	r12, r12, 36
	lea	rdx, 0[r13+r10]
	mov	QWORD PTR 320[rsp], rdx
	vpaddq	ymm14, ymm1, ymm0
	rorx	r10, QWORD PTR 304[rsp], 21
	lea	rdx, [r12+r10]
	rorx	r10, QWORD PTR 448[rsp], 17
	xor	r10, rax
	mov	QWORD PTR 304[rsp], rdx
	mov	QWORD PTR 448[rsp], r10
	cmp	r11, QWORD PTR 240[rsp]
	jne	.L21
	vpxor	ymm9, ymm9, YMMWORD PTR 768[rsp]
	vpxor	ymm10, ymm10, YMMWORD PTR 800[rsp]
	vpxor	ymm13, ymm13, YMMWORD PTR 832[rsp]
	vpxor	ymm14, ymm14, YMMWORD PTR 864[rsp]
	vpunpcklqdq	ymm1, ymm9, ymm10

	vpunpckhqdq	ymm2, ymm9, ymm10
	vpunpcklqdq	ymm3, ymm13, ymm14
	xor	eax, eax
	vpunpckhqdq	ymm0, ymm13, ymm14
	vpermq	ymm1, ymm1, 216
	vpermq	ymm2, ymm2, 216
	vpermq	ymm3, ymm3, 216
	vpermq	ymm0, ymm0, 216
	vmovdqa	YMMWORD PTR 768[rsp], ymm1
	vmovdqa	YMMWORD PTR 800[rsp], ymm3
	vmovdqa	YMMWORD PTR 832[rsp], ymm2
	vmovdqa	YMMWORD PTR 864[rsp], ymm0
	jmp	.L33
.L222:
	test	al, 3
	jne	.L20
	lea	edx, 3[rax]
	cmp	edx, 15
	ja	.L26
	mov	edx, eax
	jmp	.L24
	.p2align 4,,10
	.p2align 3
.L22:
	lea	eax, 3[rdx]
	cmp	eax, 15
	ja	.L224
.L24:
	mov	eax, edx
	vpxor	xmm3, xmm3, xmm3
	lea	ecx, 4[rdx]
	vmovdqa	YMMWORD PTR 640[rsp+rax*8], ymm3
	lea	eax, 7[rdx]
	cmp	eax, 15
	ja	.L27
	mov	eax, ecx
	lea	esi, 11[rdx]
	vmovdqa	YMMWORD PTR 640[rsp+rax*8], ymm3
	lea	eax, 8[rdx]
	cmp	esi, 15
	ja	.L27
	vmovdqa	YMMWORD PTR 640[rsp+rax*8], ymm3
	test	edx, edx
	jne	.L27
	vmovdqa	YMMWORD PTR 736[rsp], ymm3
.L27:
	mov	eax, 12
	sub	eax, edx
	and	eax, -4
	add	eax, ecx
.L25:
	cmp	eax, 16
	je	.L23
.L26:
	mov	edx, eax
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 1[rax]
	cmp	eax, 15
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 2[rax]
	cmp	eax, 14
	je	.L23
	mov	QWORD PTR 640[rsp+rdx*8], 0
	lea	edx, 3[rax]
	cmp	eax, 13
	je	.L23
	mov	eax, edx
	mov	QWORD PTR 640[rsp+rax*8], 0
	jmp	.L23
.L221:
	vmovdqu	xmm3, XMMWORD PTR 16[rcx]
	vpshufb	xmm3, xmm3, xmm5
	vmovdqa	XMMWORD PTR 16[r11], xmm3
	jmp	.L13
.L223:
	mov	r11d, 8
	add	rdx, QWORD PTR 48[rsp]
	mov	r10d, edi
	mov	esi, 255
	sub	r11d, r9d
	shlx	esi, esi, r11d
	and	sil, BYTE PTR [rdx+r10]
	movzx	edx, sil
	mov	esi, 7
	sub	esi, edi
	sal	esi, 3
	shlx	rdx, rdx, rsi
	or	rcx, rdx
	jmp	.L18
.L38:
	lea	edx, 3[rax]
	cmp	edx, 7
	mov	edx, eax
	ja	.L42
	vpxor	xmm0, xmm0, xmm0
	lea	ecx, 4[rax]
	vmovdqa	YMMWORD PTR 896[rsp+rdx*8], ymm0
	test	eax, eax
	jne	.L43
	mov	edx, ecx
	vmovdqa	YMMWORD PTR 896[rsp+rdx*8], ymm0
	jmp	.L43
.L56:
	vmovdqa	ymm5, YMMWORD PTR .LC5[rip]
	lea	rax, E_CONST[rip+512]
	vmovdqa	ymm1, YMMWORD PTR .LC0[rip]
	mov	QWORD PTR 240[rsp], rax
	vmovdqa	ymm2, YMMWORD PTR .LC1[rip]
	vmovdqa	ymm0, YMMWORD PTR .LC2[rip]
	vmovdqa	ymm4, YMMWORD PTR .LC3[rip]
	vmovdqa	YMMWORD PTR 192[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC9[rip]
	vmovdqa	ymm15, YMMWORD PTR .LC8[rip]
	vmovdqa	ymm11, YMMWORD PTR .LC14[rip]
	vmovdqa	ymm12, YMMWORD PTR .LC15[rip]
	vmovdqa	YMMWORD PTR 512[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC10[rip]
	vmovdqa	YMMWORD PTR 544[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC11[rip]
	vmovdqa	YMMWORD PTR 608[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC12[rip]
	vmovdqa	YMMWORD PTR 576[rsp], ymm5
	vmovdqa	ymm5, YMMWORD PTR .LC13[rip]
	vmovdqa	YMMWORD PTR 480[rsp], ymm5
	jmp	.L4
.L60:
	xor	ecx, ecx
	jmp	.L35
.L58:
	mov	eax, edx
.L19:
	lea	edx, 3[rax]
	cmp	edx, 15
	ja	.L25
	mov	edx, eax
	vpxor	xmm3, xmm3, xmm3
	lea	ecx, 4[rax]
	vmovdqa	YMMWORD PTR 640[rsp+rdx*8], ymm3
	mov	edx, eax
	jmp	.L27
.L55:
	mov	eax, -1
	jmp	.L1
.L57:
	xor	ecx, ecx
	jmp	.L16
.L224:
	mov	eax, edx
	jmp	.L25
.L220:
	call	__stack_chk_fail@PLT
.L59:
	xor	edx, edx
	jmp	.L29
	.size	CryptHash, .-CryptHash
	.section	.rodata
	.align 32
	.type	E_CONST, @object
	.size	E_CONST, 512

E_CONST:
	.quad	-5196783011329398166
	.quad	-4651839630684654649
	.quad	7126689189968796246
	.quad	-6375732559878238225
	.quad	3624965834938317797
	.quad	-810773768821418141
	.quad	-2736279775167281685
	.quad	5727588441176511876
	.quad	-8011951576650761406
	.quad	1553061638678277070
	.quad	-3072495659436268330
	.quad	7006529836077952479
	.quad	-8459405606061628984
	.quad	8630412583670095060
	.quad	-4417141466998381078
	.quad	-1140881805900780153
	.quad	4909668164182446062
	.quad	-1897305318382120544
	.quad	8447920087341807570
	.quad	-5090273066495579036
	.quad	3179153362732561858
	.quad	8650275271378701235
	.quad	58567911913078150
	.quad	5029388783503937534
	.quad	4124251398630671887
	.quad	1150929658523334092
	.quad	-1236252456945942505
	.quad	1602091855696760523
	.quad	2003302055548796906
	.quad	2426605538906161011
	.quad	8706985222243356014
	.quad	-1093297755420358431
	.quad	-8731988026733157347
	.quad	-1484933771145781319
	.quad	7364163754891371634
	.quad	-4902084467959641662
	.quad	-3437317100957473517
	.quad	-3173611346254284979
	.quad	-7796988273236824067
	.quad	8266931810353011323
	.quad	-6794613435065112522
	.quad	-5830887877437314200
	.quad	8574406237986300216
	.quad	3094889592723044284
	.quad	-1122377376541018640
	.quad	-7525386688366347323
	.quad	-5062807889686047763
	.quad	-1962540760282603814
	.quad	-5623451071985896351
	.quad	-2481698395038875630
	.quad	2293861175618122217
	.quad	-4262901019722070485
	.quad	-5356899795120578358
	.quad	-6620994958113062127
	.quad	-5251791014423428896
	.quad	2640910986456739304
	.quad	7527455571419594992
	.quad	-6252339164877673680
	.quad	3029312001901197118
	.quad	4262328748564713901
	.quad	3951523052885773191
	.quad	1884758615029933064
	.quad	3455829750118724073
	.quad	4151743472036784360
	.section	.rodata.cst32,"aM",@progbits,32
	.align 32
.LC0:
	.quad	0x092197f60194adc1
	.quad	0x379f513f856fc7a9
	.quad	0x1b530c95f8b3def8
	.quad	0x66b651a8ab0e883b
	.align 32
.LC1:
	.quad	0x7420b49edc5a21ee
	.quad	0xd7cd8173f479197a
	.quad	0xd1fd8a3396bdeee8
	.quad	0x07fe00ff606fac41
	.align 32
.LC2:
	.quad	0xec3fc3f38a10ea02
	.quad	0x43ab9fb62162bb7f
	.quad	0x610bebf29db2faf5
	.quad	0x75a9f91d5813e9e8
	.align 32
.LC3:
	.quad	0x34e0d42e61a33f99
	.quad	0x869d6342f6d22822
	.quad	0x87abb9f2087207ed
	.quad	0x11076689f6aff6b0
	.align 32
.LC4:
	.quad	0
	.quad	0
	.quad	2
	.quad	3
	.align 32
.LC5:
	.byte	7
	.byte	6
	.byte	5
	.byte	4
	.byte	3
	.byte	2
	.byte	1
	.byte	0
	.byte	15
	.byte	14
	.byte	13
	.byte	12
	.byte	11
	.byte	10
	.byte	9
	.byte	8
	.byte	7
	.byte	6
	.byte	5
	.byte	4
	.byte	3
	.byte	2
	.byte	1
	.byte	0
	.byte	15
	.byte	14
	.byte	13
	.byte	12
	.byte	11
	.byte	10
	.byte	9
	.byte	8
	.align 32
.LC7:
	.quad	0
	.quad	0
	.quad	0
	.quad	1
	.align 32
.LC8:
	.quad	43
	.quad	43
	.quad	48
	.quad	48
	.align 32
.LC9:
	.quad	21
	.quad	21
	.quad	16
	.quad	16
	.align 32
.LC10:
	.quad	46
	.quad	46
	.quad	31
	.quad	31
	.align 32
.LC11:
	.quad	18
	.quad	18
	.quad	33
	.quad	33
	.align 32
.LC12:
	.quad	44
	.quad	44
	.quad	15
	.quad	15
	.align 32
.LC13:
	.quad	20
	.quad	20
	.quad	49
	.quad	49
	.align 32
.LC14:
	.quad	17
	.quad	17
	.quad	4
	.quad	4
	.align 32
.LC15:
	.quad	47
	.quad	47
	.quad	60
	.quad	60
	.set	.LC16,.LC5
	.ident	"GCC: (Ubuntu 11.4.0-1ubuntu1~22.04.3) 11.4.0"
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
