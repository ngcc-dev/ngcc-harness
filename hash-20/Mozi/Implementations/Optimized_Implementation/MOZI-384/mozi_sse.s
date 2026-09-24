	.file	"CryptHash_AlgorithmInstance_fast.cpp"
	.text
	.p2align 4
	.type	hash_sponge.part.0, @function
hash_sponge.part.0:
.LFB626:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	pxor	%xmm0, %xmm0
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	pushq	%r15
	.cfi_offset 15, -24
	movq	%rsi, %r15
	pushq	%r14
	.cfi_offset 14, -32
	leaq	1280(%rsi), %r14
	movl	$1, %esi
	pushq	%r13
	.cfi_offset 13, -40
	movq	%rdi, %r13
	pushq	%r12
	.cfi_offset 12, -48
	movq	%rdx, %r12
	movabsq	$-3689348814741910323, %rdx
	pushq	%rbx
	andq	$-64, %rsp
	subq	$640, %rsp
	.cfi_offset 3, -56
	movq	%fs:40, %rax
	movq	%rax, 632(%rsp)
	xorl	%eax, %eax
	movq	%r14, %rax
	movaps	%xmm0, 64(%rsp)
	mulq	%rdx
	movaps	%xmm0, 80(%rsp)
	movaps	%xmm0, 96(%rsp)
	movaps	%xmm0, 112(%rsp)
	movq	%rdx, %rbx
	movaps	%xmm0, 128(%rsp)
	shrq	$10, %rbx
	movaps	%xmm0, 144(%rsp)
	leaq	(%rbx,%rbx,4), %rdi
	movaps	%xmm0, 160(%rsp)
	salq	$5, %rdi
	movaps	%xmm0, 176(%rsp)
	movaps	%xmm0, 192(%rsp)
	movaps	%xmm0, 208(%rsp)
	movaps	%xmm0, 224(%rsp)
	movaps	%xmm0, 240(%rsp)
	movaps	%xmm0, 256(%rsp)
	movaps	%xmm0, 272(%rsp)
	movaps	%xmm0, 288(%rsp)
	movaps	%xmm0, 304(%rsp)
	call	calloc@PLT
	testq	%rax, %rax
	je	.L36
	movq	%r15, %rdx
	movq	%rax, %rdi
	movq	%r13, %rsi
	andl	$7, %r15d
	shrq	$3, %rdx
	movq	%rdx, 48(%rsp)
	call	memcpy@PLT
	movq	48(%rsp), %rdx
	movq	%rax, %rdi
	leaq	(%rax,%rdx), %rsi
	movl	$-128, %eax
	testl	%r15d, %r15d
	jne	.L54
.L3:
	pxor	%xmm0, %xmm0
	movb	%al, (%rsi)
	movaps	%xmm0, 368(%rsp)
	movaps	%xmm0, 384(%rsp)
	movaps	%xmm0, 400(%rsp)
	movaps	%xmm0, 432(%rsp)
	movaps	%xmm0, 448(%rsp)
	movaps	%xmm0, 464(%rsp)
	movaps	%xmm0, 480(%rsp)
	movaps	%xmm0, 496(%rsp)
	movaps	%xmm0, 512(%rsp)
	movaps	%xmm0, 528(%rsp)
	movaps	%xmm0, 544(%rsp)
	movaps	%xmm0, 560(%rsp)
	movaps	%xmm0, 576(%rsp)
	movaps	%xmm0, 592(%rsp)
	cmpq	$1279, %r14
	jbe	.L55
	pxor	%xmm6, %xmm6
	movq	%rdi, %r8
	xorl	%r11d, %r11d
	movl	$64, %r10d
	movdqa	%xmm6, %xmm5
	leaq	_ZL3rho(%rip), %rsi
	leaq	_ZL4RCON(%rip), %r9
	movdqa	%xmm6, %xmm3
.L28:
	movdqu	(%r8), %xmm7
	movdqu	16(%r8), %xmm14
	xorl	%edx, %edx
	pxor	368(%rsp), %xmm7
	movdqu	32(%r8), %xmm2
	pxor	432(%rsp), %xmm14
	movdqu	64(%r8), %xmm0
	pxor	496(%rsp), %xmm2
	movdqu	96(%r8), %xmm15
	pxor	384(%rsp), %xmm0
	movdqu	112(%r8), %xmm10
	movdqa	%xmm14, %xmm8
	movdqa	%xmm7, %xmm14
	movdqu	48(%r8), %xmm13
	movdqu	80(%r8), %xmm6
	movdqa	%xmm2, %xmm7
	movdqa	%xmm3, %xmm2
	pxor	512(%rsp), %xmm15
	movdqa	%xmm0, %xmm11
	pxor	576(%rsp), %xmm10
	movdqu	144(%r8), %xmm4
	pxor	464(%rsp), %xmm4
	movdqu	128(%r8), %xmm12
	movdqa	%xmm15, %xmm0
	movdqa	%xmm5, %xmm15
	pxor	560(%rsp), %xmm13
	pxor	448(%rsp), %xmm6
	movdqa	%xmm4, %xmm3
	movdqa	%xmm10, %xmm4
	pxor	400(%rsp), %xmm12
	.p2align 4,,10
	.p2align 3
.L27:
	movdqa	480(%rsp), %xmm1
	movdqa	%xmm15, %xmm9
	movq	%rdx, %rax
	pand	%xmm12, %xmm9
	andl	$3, %eax
	movdqa	%xmm1, %xmm5
	pxor	%xmm11, %xmm9
	por	%xmm12, %xmm11
	movq	%rax, %rcx
	pand	%xmm3, %xmm5
	pxor	%xmm14, %xmm11
	pand	%xmm9, %xmm14
	salq	$4, %rcx
	pxor	%xmm6, %xmm5
	por	%xmm3, %xmm6
	pxor	%xmm15, %xmm14
	pxor	%xmm8, %xmm6
	pand	%xmm5, %xmm8
	pxor	%xmm5, %xmm9
	pxor	%xmm1, %xmm8
	pand	%xmm6, %xmm1
	movdqa	%xmm1, %xmm10
	movaps	%xmm8, 32(%rsp)
	movdqa	592(%rsp), %xmm1
	pxor	32(%rsp), %xmm14
	pxor	%xmm3, %xmm10
	movdqa	544(%rsp), %xmm3
	pand	528(%rsp), %xmm3
	movdqa	%xmm3, %xmm8
	movdqa	%xmm2, %xmm3
	pand	%xmm1, %xmm3
	pxor	%xmm0, %xmm8
	por	528(%rsp), %xmm0
	pxor	%xmm4, %xmm3
	por	%xmm1, %xmm4
	pxor	%xmm13, %xmm4
	pand	%xmm3, %xmm13
	pxor	%xmm7, %xmm0
	pxor	%xmm2, %xmm13
	pand	%xmm4, %xmm2
	pand	%xmm8, %xmm7
	pxor	%xmm1, %xmm2
	movdqa	%xmm0, %xmm1
	movaps	%xmm13, (%rsp)
	pxor	544(%rsp), %xmm7
	pxor	%xmm4, %xmm1
	pxor	%xmm3, %xmm8
	pand	544(%rsp), %xmm0
	pxor	528(%rsp), %xmm0
	pxor	%xmm13, %xmm7
	movdqa	%xmm1, %xmm13
	movdqa	%xmm11, %xmm1
	pand	%xmm15, %xmm11
	pxor	%xmm12, %xmm11
	pxor	%xmm6, %xmm1
	pxor	%xmm2, %xmm0
	movdqa	%xmm11, %xmm12
	movaps	%xmm1, 16(%rsp)
	pxor	%xmm0, %xmm6
	movdqa	16(%rsp), %xmm15
	pxor	%xmm10, %xmm12
	movdqa	%xmm10, %xmm11
	movdqa	%xmm0, %xmm10
	movdqa	%xmm12, %xmm1
	movdqa	%xmm7, %xmm12
	pxor	%xmm8, %xmm11
	pxor	%xmm5, %xmm12
	pxor	%xmm1, %xmm4
	pxor	%xmm13, %xmm5
	movaps	%xmm12, 48(%rsp)
	movdqa	%xmm14, %xmm12
	pxor	%xmm4, %xmm7
	pxor	32(%rsp), %xmm5
	pxor	%xmm3, %xmm12
	pxor	%xmm15, %xmm3
	pxor	(%rsp), %xmm3
	pxor	%xmm8, %xmm7
	pxor	%xmm9, %xmm2
	pxor	%xmm12, %xmm10
	pxor	%xmm3, %xmm0
	pxor	%xmm11, %xmm15
	movaps	%xmm10, 496(%rsp)
	pxor	48(%rsp), %xmm10
	pxor	%xmm8, %xmm0
	movdqa	%xmm13, %xmm8
	movdqa	48(%rsp), %xmm13
	movaps	%xmm7, 528(%rsp)
	pxor	%xmm2, %xmm8
	pxor	%xmm6, %xmm7
	movaps	%xmm0, 512(%rsp)
	pxor	%xmm15, %xmm2
	pxor	%xmm1, %xmm13
	pxor	%xmm9, %xmm1
	pxor	%xmm14, %xmm9
	movaps	%xmm8, 544(%rsp)
	pxor	%xmm5, %xmm1
	pxor	%xmm6, %xmm9
	pxor	%xmm5, %xmm0
	movaps	%xmm13, 368(%rsp)
	pxor	%xmm11, %xmm8
	pxor	%xmm13, %xmm12
	pxor	%xmm1, %xmm3
	movaps	%xmm1, 384(%rsp)
	pxor	%xmm9, %xmm4
	movaps	%xmm0, 448(%rsp)
	movaps	%xmm9, 400(%rsp)
	movaps	%xmm10, 432(%rsp)
	movaps	%xmm7, 464(%rsp)
	movaps	%xmm8, 480(%rsp)
	movaps	%xmm12, 560(%rsp)
	movaps	%xmm3, 576(%rsp)
	movaps	%xmm4, 592(%rsp)
	movaps	%xmm2, 608(%rsp)
	movl	4(%rsi,%rcx), %ecx
	movl	%ecx, %r13d
	andl	$127, %r13d
	je	.L5
	andl	$64, %ecx
	je	.L56
	pshufd	$78, %xmm10, %xmm10
	subl	$64, %r13d
	je	.L57
	movl	%r10d, %ecx
	movd	%r13d, %xmm3
	movdqa	%xmm10, %xmm2
	pshufd	$78, 480(%rsp), %xmm8
	subl	%r13d, %ecx
	psrlq	%xmm3, %xmm2
	pshufd	$78, %xmm0, %xmm0
	movq	%rcx, %xmm4
	pshufd	$78, %xmm7, %xmm7
	psllq	%xmm4, %xmm10
	pshufd	$78, %xmm10, %xmm1
	por	%xmm2, %xmm1
	movaps	%xmm1, 432(%rsp)
	movdqa	%xmm0, %xmm1
	psllq	%xmm4, %xmm0
	psrlq	%xmm3, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
	movaps	%xmm0, 448(%rsp)
	movdqa	%xmm7, %xmm0
	psllq	%xmm4, %xmm7
	psrlq	%xmm3, %xmm0
	pshufd	$78, %xmm7, %xmm7
	por	%xmm7, %xmm0
	movaps	%xmm0, 464(%rsp)
.L8:
	movl	%r10d, %ecx
	movdqa	%xmm8, %xmm0
	subl	%r13d, %ecx
	psrlq	%xmm3, %xmm0
	movq	%rcx, %xmm6
	psllq	%xmm6, %xmm8
	pshufd	$78, %xmm8, %xmm8
	por	%xmm0, %xmm8
	.p2align 4,,10
	.p2align 3
.L5:
	movdqa	544(%rsp), %xmm0
	movq	%rax, %rcx
	movaps	%xmm8, 480(%rsp)
	salq	$4, %rcx
	movl	8(%rsi,%rcx), %ecx
	movl	%ecx, %r13d
	andl	$127, %r13d
	je	.L13
	movdqa	496(%rsp), %xmm2
	andl	$64, %ecx
	movdqa	512(%rsp), %xmm1
	je	.L58
	pshufd	$78, %xmm2, %xmm2
	subl	$64, %r13d
	je	.L59
	movl	%r10d, %ecx
	movd	%r13d, %xmm4
	movdqa	%xmm2, %xmm5
	subl	%r13d, %ecx
	psrlq	%xmm4, %xmm5
	pshufd	$78, %xmm1, %xmm1
	movq	%rcx, %xmm3
	psllq	%xmm3, %xmm2
	pshufd	$78, %xmm2, %xmm0
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	por	%xmm5, %xmm0
	movaps	%xmm0, 496(%rsp)
	pshufd	$78, %xmm1, %xmm0
	por	%xmm2, %xmm0
	movaps	%xmm0, 512(%rsp)
	pshufd	$78, 528(%rsp), %xmm0
	movdqa	%xmm0, %xmm1
	psllq	%xmm3, %xmm0
	psrlq	%xmm4, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm0, %xmm1
	pshufd	$78, 544(%rsp), %xmm0
	movaps	%xmm1, 528(%rsp)
.L16:
	movl	%r10d, %ecx
	movdqa	%xmm0, %xmm1
	subl	%r13d, %ecx
	psrlq	%xmm4, %xmm1
	movq	%rcx, %xmm6
	psllq	%xmm6, %xmm0
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
	.p2align 4,,10
	.p2align 3
.L13:
	salq	$4, %rax
	movaps	%xmm0, 544(%rsp)
	movdqa	608(%rsp), %xmm0
	movl	12(%rsi,%rax), %eax
	movl	%eax, %ecx
	andl	$127, %ecx
	je	.L21
	movdqa	560(%rsp), %xmm2
	movdqa	576(%rsp), %xmm1
	testb	$64, %al
	je	.L22
	pshufd	$78, %xmm2, %xmm2
	subl	$64, %ecx
	je	.L60
	movd	%ecx, %xmm4
	movdqa	%xmm2, %xmm5
	pshufd	$78, %xmm1, %xmm1
	movl	%r10d, %eax
	subl	%ecx, %eax
	psrlq	%xmm4, %xmm5
	movq	%rax, %xmm3
	psllq	%xmm3, %xmm2
	pshufd	$78, %xmm2, %xmm0
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	por	%xmm5, %xmm0
	movaps	%xmm0, 560(%rsp)
	pshufd	$78, %xmm1, %xmm0
	por	%xmm2, %xmm0
	movaps	%xmm0, 576(%rsp)
	pshufd	$78, 592(%rsp), %xmm0
	movdqa	%xmm0, %xmm1
	psllq	%xmm3, %xmm0
	psrlq	%xmm4, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm0, %xmm1
	pshufd	$78, 608(%rsp), %xmm0
	movaps	%xmm1, 592(%rsp)
.L31:
	movl	%r10d, %eax
	movdqa	%xmm0, %xmm1
	subl	%ecx, %eax
	psrlq	%xmm4, %xmm1
	movq	%rax, %xmm6
	psllq	%xmm6, %xmm0
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
	.p2align 4,,10
	.p2align 3
.L21:
	movdqa	368(%rsp), %xmm14
	movzbl	(%r9,%rdx), %eax
	addq	$1, %rdx
	movdqa	384(%rsp), %xmm11
	movdqa	400(%rsp), %xmm12
	movq	%rax, %xmm2
	movdqa	%xmm14, %xmm1
	pxor	%xmm0, %xmm2
	cmpq	$20, %rdx
	je	.L26
	movdqa	512(%rsp), %xmm0
	movdqa	496(%rsp), %xmm7
	movdqa	576(%rsp), %xmm4
	movdqa	560(%rsp), %xmm13
	movdqa	464(%rsp), %xmm3
	movdqa	448(%rsp), %xmm6
	movdqa	432(%rsp), %xmm8
	jmp	.L27
.L26:
	addq	$1, %r11
	movdqa	%xmm15, %xmm5
	movdqa	%xmm2, %xmm3
	addq	$160, %r8
	cmpq	%r11, %rbx
	ja	.L28
.L29:
	movdqa	432(%rsp), %xmm2
	movups	%xmm1, (%r12)
	movdqa	496(%rsp), %xmm0
	movaps	%xmm1, 64(%rsp)
	movups	%xmm2, 16(%r12)
	movups	%xmm0, 32(%r12)
	movaps	%xmm2, 80(%rsp)
	movaps	%xmm0, 96(%rsp)
	call	free@PLT
	xorl	%eax, %eax
.L1:
	movq	632(%rsp), %rdx
	subq	%fs:40, %rdx
	jne	.L61
	leaq	-40(%rbp), %rsp
	popq	%rbx
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	%rbp
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
.L22:
	.cfi_restore_state
	movl	%r10d, %eax
	movd	%ecx, %xmm4
	movdqa	%xmm2, %xmm5
	subl	%ecx, %eax
	psrlq	%xmm4, %xmm5
	movq	%rax, %xmm3
	psllq	%xmm3, %xmm2
	pshufd	$78, %xmm2, %xmm2
	por	%xmm5, %xmm2
	movaps	%xmm2, 560(%rsp)
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm1, %xmm1
	por	%xmm2, %xmm1
	movaps	%xmm1, 576(%rsp)
	movdqa	592(%rsp), %xmm1
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm2
	movaps	%xmm2, 592(%rsp)
	jmp	.L31
.L56:
	movl	%r10d, %ecx
	movd	%r13d, %xmm3
	movdqa	%xmm10, %xmm2
	subl	%r13d, %ecx
	psrlq	%xmm3, %xmm2
	movq	%rcx, %xmm4
	psllq	%xmm4, %xmm10
	pshufd	$78, %xmm10, %xmm1
	por	%xmm2, %xmm1
	movaps	%xmm1, 432(%rsp)
	movdqa	%xmm0, %xmm1
	psllq	%xmm4, %xmm0
	psrlq	%xmm3, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
	movaps	%xmm0, 448(%rsp)
	movdqa	%xmm7, %xmm0
	psllq	%xmm4, %xmm7
	psrlq	%xmm3, %xmm0
	pshufd	$78, %xmm7, %xmm7
	por	%xmm7, %xmm0
	movaps	%xmm0, 464(%rsp)
	jmp	.L8
.L58:
	movl	%r10d, %ecx
	movd	%r13d, %xmm4
	movdqa	%xmm2, %xmm5
	subl	%r13d, %ecx
	psrlq	%xmm4, %xmm5
	movq	%rcx, %xmm3
	psllq	%xmm3, %xmm2
	pshufd	$78, %xmm2, %xmm2
	por	%xmm5, %xmm2
	movaps	%xmm2, 496(%rsp)
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm1, %xmm1
	por	%xmm2, %xmm1
	movaps	%xmm1, 512(%rsp)
	movdqa	528(%rsp), %xmm1
	movdqa	%xmm1, %xmm2
	psllq	%xmm3, %xmm1
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm2
	movaps	%xmm2, 528(%rsp)
	jmp	.L16
.L59:
	pshufd	$78, %xmm1, %xmm1
	movaps	%xmm2, 496(%rsp)
	pshufd	$78, %xmm0, %xmm0
	movaps	%xmm1, 512(%rsp)
	pshufd	$78, 528(%rsp), %xmm1
	movaps	%xmm1, 528(%rsp)
	jmp	.L13
.L60:
	pshufd	$78, %xmm1, %xmm1
	movaps	%xmm2, 560(%rsp)
	pshufd	$78, %xmm0, %xmm0
	movaps	%xmm1, 576(%rsp)
	pshufd	$78, 592(%rsp), %xmm1
	movaps	%xmm1, 592(%rsp)
	jmp	.L21
.L57:
	pshufd	$78, %xmm0, %xmm0
	pshufd	$78, %xmm7, %xmm7
	movaps	%xmm10, 432(%rsp)
	pshufd	$78, %xmm8, %xmm8
	movaps	%xmm0, 448(%rsp)
	movaps	%xmm7, 464(%rsp)
	jmp	.L5
.L54:
	movl	$8, %ecx
	movl	$255, %eax
	subl	%r15d, %ecx
	sall	%cl, %eax
	movl	%r15d, %ecx
	andb	0(%r13,%rdx), %al
	movl	$128, %edx
	shrl	%cl, %edx
	orl	%edx, %eax
	jmp	.L3
.L55:
	movdqa	%xmm0, %xmm1
	jmp	.L29
.L61:
	call	__stack_chk_fail@PLT
.L36:
	movl	$1, %eax
	jmp	.L1
	.cfi_endproc
.LFE626:
	.size	hash_sponge.part.0, .-hash_sponge.part.0
	.p2align 4
	.globl	permutation
	.type	permutation, @function
permutation:
.LFB623:
	.cfi_startproc
	endbr64
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset 6, -16
	movq	%rdi, %rdx
	movq	%rsp, %rbp
	.cfi_def_cfa_register 6
	andq	$-64, %rsp
	subq	$640, %rsp
	movdqu	128(%rdi), %xmm5
	movdqu	(%rdi), %xmm3
	movq	%fs:40, %rax
	movq	%rax, 632(%rsp)
	xorl	%eax, %eax
	movdqu	192(%rdi), %xmm6
	movaps	%xmm5, 400(%rsp)
	movdqa	%xmm5, %xmm4
	movdqu	16(%rdi), %xmm5
	movaps	%xmm6, 416(%rsp)
	movdqa	%xmm6, %xmm7
	movdqu	208(%rdi), %xmm6
	movaps	%xmm5, 432(%rsp)
	movdqu	80(%rdi), %xmm5
	movaps	%xmm6, 480(%rsp)
	movdqu	96(%rdi), %xmm6
	movaps	%xmm5, 448(%rsp)
	movdqu	144(%rdi), %xmm5
	movaps	%xmm6, 512(%rsp)
	movdqu	224(%rdi), %xmm6
	movaps	%xmm5, 464(%rsp)
	movdqu	32(%rdi), %xmm5
	movaps	%xmm3, 48(%rsp)
	movaps	%xmm5, 496(%rsp)
	movdqu	160(%rdi), %xmm5
	movaps	%xmm3, 368(%rsp)
	movdqu	64(%rdi), %xmm3
	movaps	%xmm5, 528(%rsp)
	movdqu	48(%rdi), %xmm5
	movaps	%xmm6, 544(%rsp)
	movdqu	112(%rdi), %xmm6
	movaps	%xmm5, 560(%rsp)
	movaps	%xmm3, 384(%rsp)
	movaps	%xmm6, 576(%rsp)
	movdqu	176(%rdi), %xmm5
	movdqu	240(%rdi), %xmm2
	movaps	%xmm5, 592(%rsp)
	movaps	%xmm2, 608(%rsp)
	testl	%esi, %esi
	je	.L86
	movdqa	%xmm7, %xmm9
	movl	%esi, %r10d
	movdqa	%xmm2, %xmm7
	movl	$64, %edi
	leaq	_ZL3rho(%rip), %r8
	leaq	_ZL4RCON(%rip), %r9
	movdqa	%xmm4, %xmm10
	jmp	.L85
	.p2align 4,,10
	.p2align 3
.L65:
	pshufd	$78, %xmm8, %xmm8
	subl	$64, %ecx
	je	.L110
	movd	%ecx, %xmm4
	movdqa	%xmm8, %xmm6
	pshufd	$78, %xmm0, %xmm0
	movl	%edi, %esi
	subl	%ecx, %esi
	psrlq	%xmm4, %xmm6
	pshufd	$78, %xmm1, %xmm1
	pshufd	$78, 480(%rsp), %xmm7
	movq	%rsi, %xmm5
	psllq	%xmm5, %xmm8
	pshufd	$78, %xmm8, %xmm2
	por	%xmm6, %xmm2
	movaps	%xmm2, 432(%rsp)
	movdqa	%xmm0, %xmm2
	psllq	%xmm5, %xmm0
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm0, %xmm0
	por	%xmm2, %xmm0
	movdqa	%xmm4, %xmm2
	movaps	%xmm0, 448(%rsp)
	movdqa	%xmm1, %xmm0
	psllq	%xmm5, %xmm1
	psrlq	%xmm4, %xmm0
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm0
	movaps	%xmm0, 464(%rsp)
.L67:
	movl	%edi, %esi
	movdqa	%xmm7, %xmm0
	subl	%ecx, %esi
	psrlq	%xmm2, %xmm0
	movq	%rsi, %xmm6
	psllq	%xmm6, %xmm7
	pshufd	$78, %xmm7, %xmm7
	por	%xmm0, %xmm7
.L64:
	movq	%r11, %rcx
	movaps	%xmm7, 480(%rsp)
	movdqa	544(%rsp), %xmm0
	salq	$4, %rcx
	movl	8(%r8,%rcx), %esi
	movl	%esi, %ecx
	andl	$127, %ecx
	je	.L72
	movdqa	496(%rsp), %xmm1
	andl	$64, %esi
	movdqa	512(%rsp), %xmm2
	je	.L111
	pshufd	$78, %xmm1, %xmm1
	subl	$64, %ecx
	je	.L112
	movd	%ecx, %xmm4
	movdqa	%xmm1, %xmm6
	pshufd	$78, %xmm2, %xmm2
	movl	%edi, %esi
	subl	%ecx, %esi
	psrlq	%xmm4, %xmm6
	movq	%rsi, %xmm5
	psllq	%xmm5, %xmm1
	pshufd	$78, %xmm1, %xmm0
	movdqa	%xmm2, %xmm1
	psllq	%xmm5, %xmm2
	psrlq	%xmm4, %xmm1
	por	%xmm6, %xmm0
	movaps	%xmm0, 496(%rsp)
	pshufd	$78, %xmm2, %xmm0
	por	%xmm1, %xmm0
	movaps	%xmm0, 512(%rsp)
	pshufd	$78, 528(%rsp), %xmm0
	movdqa	%xmm0, %xmm1
	psllq	%xmm5, %xmm0
	psrlq	%xmm4, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm0, %xmm1
	pshufd	$78, 544(%rsp), %xmm0
	movaps	%xmm1, 528(%rsp)
.L75:
	movl	%edi, %esi
	movdqa	%xmm0, %xmm1
	subl	%ecx, %esi
	psrlq	%xmm4, %xmm1
	movq	%rsi, %xmm5
	psllq	%xmm5, %xmm0
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
.L72:
	salq	$4, %r11
	movaps	%xmm0, 544(%rsp)
	movdqa	608(%rsp), %xmm0
	movl	12(%r8,%r11), %esi
	movl	%esi, %ecx
	andl	$127, %ecx
	je	.L80
	movdqa	560(%rsp), %xmm1
	andl	$64, %esi
	movdqa	576(%rsp), %xmm6
	je	.L81
	pshufd	$78, %xmm1, %xmm1
	subl	$64, %ecx
	je	.L113
	movd	%ecx, %xmm2
	movdqa	%xmm1, %xmm5
	pshufd	$78, %xmm6, %xmm6
	movl	%edi, %esi
	subl	%ecx, %esi
	psrlq	%xmm2, %xmm5
	movq	%rsi, %xmm4
	psllq	%xmm4, %xmm1
	pshufd	$78, %xmm1, %xmm0
	movdqa	%xmm6, %xmm1
	psllq	%xmm4, %xmm6
	psrlq	%xmm2, %xmm1
	por	%xmm5, %xmm0
	movaps	%xmm0, 560(%rsp)
	pshufd	$78, %xmm6, %xmm0
	por	%xmm1, %xmm0
	movaps	%xmm0, 576(%rsp)
	pshufd	$78, 592(%rsp), %xmm0
	movdqa	%xmm0, %xmm1
	psllq	%xmm4, %xmm0
	psrlq	%xmm2, %xmm1
	pshufd	$78, %xmm0, %xmm0
	por	%xmm0, %xmm1
	pshufd	$78, 608(%rsp), %xmm0
	movaps	%xmm1, 592(%rsp)
.L89:
	movl	%edi, %esi
	movdqa	%xmm0, %xmm1
	subl	%ecx, %esi
	psrlq	%xmm2, %xmm1
	movq	%rsi, %xmm6
	psllq	%xmm6, %xmm0
	pshufd	$78, %xmm0, %xmm0
	por	%xmm1, %xmm0
.L80:
	movzbl	(%r9,%rax), %ecx
	addq	$1, %rax
	movq	%rcx, %xmm7
	pxor	%xmm0, %xmm7
	movaps	%xmm7, 608(%rsp)
	cmpq	%rax, %r10
	je	.L114
.L85:
	movdqa	480(%rsp), %xmm0
	movdqa	%xmm7, %xmm4
	movq	%rax, %r11
	movdqa	464(%rsp), %xmm2
	movdqa	448(%rsp), %xmm5
	movdqa	%xmm9, %xmm8
	movdqa	432(%rsp), %xmm1
	andl	$3, %r11d
	movdqa	%xmm0, %xmm6
	pand	%xmm10, %xmm8
	movq	%r11, %rcx
	movdqa	592(%rsp), %xmm12
	pand	%xmm2, %xmm6
	por	%xmm2, %xmm5
	pxor	448(%rsp), %xmm6
	movdqa	560(%rsp), %xmm14
	pand	%xmm12, %xmm4
	pxor	%xmm3, %xmm8
	por	%xmm10, %xmm3
	movdqa	528(%rsp), %xmm15
	pand	%xmm6, %xmm1
	pxor	48(%rsp), %xmm3
	salq	$4, %rcx
	pxor	432(%rsp), %xmm5
	pxor	576(%rsp), %xmm4
	pxor	%xmm0, %xmm1
	pand	%xmm5, %xmm0
	movdqa	%xmm1, %xmm13
	movdqa	544(%rsp), %xmm1
	pxor	%xmm2, %xmm0
	pand	%xmm4, %xmm14
	movdqa	576(%rsp), %xmm2
	pxor	%xmm7, %xmm14
	pand	%xmm15, %xmm1
	movdqa	%xmm0, %xmm11
	movdqa	512(%rsp), %xmm0
	pxor	512(%rsp), %xmm1
	por	%xmm12, %xmm2
	pxor	560(%rsp), %xmm2
	por	%xmm15, %xmm0
	pxor	496(%rsp), %xmm0
	pand	%xmm2, %xmm7
	pxor	%xmm12, %xmm7
	movdqa	496(%rsp), %xmm12
	pand	%xmm1, %xmm12
	pxor	%xmm4, %xmm1
	pxor	544(%rsp), %xmm12
	pxor	%xmm14, %xmm12
	movaps	%xmm12, 32(%rsp)
	movdqa	%xmm0, %xmm12
	pand	544(%rsp), %xmm0
	pxor	%xmm2, %xmm12
	pxor	%xmm15, %xmm0
	movdqa	48(%rsp), %xmm15
	movaps	%xmm12, 16(%rsp)
	pxor	%xmm7, %xmm0
	pand	%xmm8, %xmm15
	movdqa	%xmm15, %xmm12
	pxor	%xmm9, %xmm12
	movdqa	%xmm12, %xmm15
	pxor	%xmm13, %xmm15
	movdqa	%xmm15, %xmm12
	movdqa	%xmm3, %xmm15
	pand	%xmm9, %xmm3
	pxor	%xmm10, %xmm3
	movdqa	%xmm8, %xmm9
	pxor	%xmm5, %xmm15
	movdqa	32(%rsp), %xmm10
	pxor	%xmm6, %xmm9
	pxor	%xmm11, %xmm3
	pxor	%xmm0, %xmm5
	movaps	%xmm12, (%rsp)
	pxor	%xmm6, %xmm10
	pxor	16(%rsp), %xmm6
	movdqa	%xmm0, %xmm8
	pxor	%xmm3, %xmm2
	pxor	%xmm13, %xmm6
	movdqa	%xmm11, %xmm13
	movdqa	%xmm12, %xmm11
	pxor	%xmm4, %xmm11
	pxor	%xmm15, %xmm4
	pxor	%xmm1, %xmm13
	pxor	%xmm14, %xmm4
	movdqa	%xmm11, %xmm12
	movdqa	%xmm7, %xmm11
	movdqa	16(%rsp), %xmm7
	pxor	%xmm4, %xmm0
	pxor	%xmm9, %xmm11
	movdqa	%xmm10, %xmm14
	pxor	%xmm1, %xmm0
	pxor	32(%rsp), %xmm1
	pxor	%xmm12, %xmm8
	pxor	%xmm11, %xmm7
	pxor	%xmm3, %xmm14
	pxor	%xmm6, %xmm3
	movaps	%xmm8, 496(%rsp)
	pxor	%xmm2, %xmm1
	pxor	%xmm9, %xmm3
	pxor	%xmm14, %xmm12
	movaps	%xmm0, 512(%rsp)
	movaps	%xmm1, 528(%rsp)
	pxor	%xmm6, %xmm0
	pxor	%xmm5, %xmm1
	pxor	%xmm3, %xmm4
	movaps	%xmm7, 544(%rsp)
	pxor	%xmm13, %xmm7
	movaps	%xmm10, 32(%rsp)
	pxor	(%rsp), %xmm9
	movl	4(%r8,%rcx), %esi
	pxor	32(%rsp), %xmm8
	movaps	%xmm14, 48(%rsp)
	pxor	%xmm5, %xmm9
	movl	%esi, %ecx
	movaps	%xmm0, 448(%rsp)
	movdqa	%xmm9, %xmm10
	movdqa	%xmm15, %xmm9
	movaps	%xmm8, 432(%rsp)
	pxor	%xmm13, %xmm9
	pxor	%xmm10, %xmm2
	movaps	%xmm1, 464(%rsp)
	pxor	%xmm9, %xmm11
	movaps	%xmm7, 480(%rsp)
	movaps	%xmm12, 560(%rsp)
	movaps	%xmm4, 576(%rsp)
	movaps	%xmm2, 592(%rsp)
	movaps	%xmm11, 608(%rsp)
	andl	$127, %ecx
	je	.L64
	andl	$64, %esi
	jne	.L65
	movl	%edi, %esi
	movd	%ecx, %xmm2
	movdqa	%xmm8, %xmm5
	subl	%ecx, %esi
	psrlq	%xmm2, %xmm5
	movq	%rsi, %xmm4
	psllq	%xmm4, %xmm8
	pshufd	$78, %xmm8, %xmm8
	por	%xmm8, %xmm5
	movaps	%xmm5, 432(%rsp)
	movdqa	%xmm0, %xmm5
	psllq	%xmm4, %xmm0
	psrlq	%xmm2, %xmm5
	pshufd	$78, %xmm0, %xmm0
	por	%xmm5, %xmm0
	movaps	%xmm0, 448(%rsp)
	movdqa	%xmm1, %xmm0
	psllq	%xmm4, %xmm1
	psrlq	%xmm2, %xmm0
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm0
	movaps	%xmm0, 464(%rsp)
	jmp	.L67
	.p2align 4,,10
	.p2align 3
.L81:
	movl	%edi, %esi
	movd	%ecx, %xmm2
	movdqa	%xmm1, %xmm4
	subl	%ecx, %esi
	psrlq	%xmm2, %xmm4
	movq	%rsi, %xmm5
	psllq	%xmm5, %xmm1
	pshufd	$78, %xmm1, %xmm1
	por	%xmm4, %xmm1
	movaps	%xmm1, 560(%rsp)
	movdqa	%xmm6, %xmm1
	psllq	%xmm5, %xmm6
	psrlq	%xmm2, %xmm1
	pshufd	$78, %xmm6, %xmm6
	por	%xmm6, %xmm1
	movaps	%xmm1, 576(%rsp)
	movdqa	592(%rsp), %xmm1
	movdqa	%xmm1, %xmm4
	psllq	%xmm5, %xmm1
	psrlq	%xmm2, %xmm4
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm4
	movaps	%xmm4, 592(%rsp)
	jmp	.L89
	.p2align 4,,10
	.p2align 3
.L111:
	movl	%edi, %esi
	movd	%ecx, %xmm4
	movdqa	%xmm1, %xmm6
	subl	%ecx, %esi
	psrlq	%xmm4, %xmm6
	movq	%rsi, %xmm5
	psllq	%xmm5, %xmm1
	pshufd	$78, %xmm1, %xmm1
	por	%xmm6, %xmm1
	movaps	%xmm1, 496(%rsp)
	movdqa	%xmm2, %xmm1
	psllq	%xmm5, %xmm2
	psrlq	%xmm4, %xmm1
	pshufd	$78, %xmm2, %xmm2
	por	%xmm2, %xmm1
	movaps	%xmm1, 512(%rsp)
	movdqa	528(%rsp), %xmm1
	movdqa	%xmm1, %xmm2
	psllq	%xmm5, %xmm1
	psrlq	%xmm4, %xmm2
	pshufd	$78, %xmm1, %xmm1
	por	%xmm1, %xmm2
	movaps	%xmm2, 528(%rsp)
	jmp	.L75
.L114:
	movdqa	48(%rsp), %xmm6
	movaps	%xmm3, 384(%rsp)
	movaps	%xmm9, 416(%rsp)
	movaps	%xmm6, 368(%rsp)
	movaps	%xmm10, 400(%rsp)
.L86:
	movdqa	368(%rsp), %xmm3
	movups	%xmm3, (%rdx)
	movdqa	432(%rsp), %xmm3
	movups	%xmm3, 16(%rdx)
	movdqa	496(%rsp), %xmm3
	movups	%xmm3, 32(%rdx)
	movdqa	560(%rsp), %xmm3
	movups	%xmm3, 48(%rdx)
	movdqa	384(%rsp), %xmm3
	movups	%xmm3, 64(%rdx)
	movdqa	448(%rsp), %xmm3
	movups	%xmm3, 80(%rdx)
	movdqa	512(%rsp), %xmm3
	movups	%xmm3, 96(%rdx)
	movdqa	576(%rsp), %xmm3
	movups	%xmm3, 112(%rdx)
	movdqa	400(%rsp), %xmm3
	movups	%xmm3, 128(%rdx)
	movdqa	464(%rsp), %xmm3
	movups	%xmm3, 144(%rdx)
	movdqa	528(%rsp), %xmm3
	movups	%xmm3, 160(%rdx)
	movdqa	592(%rsp), %xmm3
	movups	%xmm3, 176(%rdx)
	movdqa	416(%rsp), %xmm3
	movups	%xmm3, 192(%rdx)
	movdqa	480(%rsp), %xmm3
	movups	%xmm3, 208(%rdx)
	movdqa	544(%rsp), %xmm3
	movups	%xmm3, 224(%rdx)
	movdqa	608(%rsp), %xmm3
	movups	%xmm3, 240(%rdx)
	movq	632(%rsp), %rax
	subq	%fs:40, %rax
	jne	.L115
	leave
	.cfi_remember_state
	.cfi_def_cfa 7, 8
	ret
.L112:
	.cfi_restore_state
	pshufd	$78, %xmm2, %xmm2
	movaps	%xmm1, 496(%rsp)
	pshufd	$78, %xmm0, %xmm0
	pshufd	$78, 528(%rsp), %xmm1
	movaps	%xmm2, 512(%rsp)
	movaps	%xmm1, 528(%rsp)
	jmp	.L72
.L113:
	pshufd	$78, %xmm6, %xmm6
	movaps	%xmm1, 560(%rsp)
	pshufd	$78, %xmm0, %xmm0
	pshufd	$78, 592(%rsp), %xmm1
	movaps	%xmm6, 576(%rsp)
	movaps	%xmm1, 592(%rsp)
	jmp	.L80
.L110:
	pshufd	$78, %xmm0, %xmm0
	pshufd	$78, %xmm1, %xmm1
	pshufd	$78, %xmm7, %xmm7
	movaps	%xmm8, 432(%rsp)
	movaps	%xmm0, 448(%rsp)
	movaps	%xmm1, 464(%rsp)
	jmp	.L64
.L115:
	call	__stack_chk_fail@PLT
	.cfi_endproc
.LFE623:
	.size	permutation, .-permutation
	.section	.rodata.str1.1,"aMS",@progbits,1
.LC0:
	.string	"Invalid rate or hash length\n"
	.text
	.p2align 4
	.globl	hash_sponge
	.type	hash_sponge, @function
hash_sponge:
.LFB624:
	.cfi_startproc
	endbr64
	cmpl	$160, %esi
	jne	.L120
	cmpl	$48, %edi
	jne	.L120
	movq	%rdx, %r8
	movq	%rcx, %rsi
	movq	%r9, %rdx
	movq	%r8, %rdi
	jmp	hash_sponge.part.0
	.p2align 4,,10
	.p2align 3
.L120:
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	leaq	.LC0(%rip), %rsi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk@PLT
	movl	$1, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE624:
	.size	hash_sponge, .-hash_sponge
	.p2align 4
	.globl	CryptHash
	.type	CryptHash, @function
CryptHash:
.LFB625:
	.cfi_startproc
	endbr64
	subl	$384, %edi
	cmpl	$7, %edi
	ja	.L128
	movq	%rsi, %r8
	movq	%rdx, %rsi
	movq	%rcx, %rdx
	movq	%r8, %rdi
	jmp	hash_sponge.part.0
	.p2align 4,,10
	.p2align 3
.L128:
	subq	$8, %rsp
	.cfi_def_cfa_offset 16
	leaq	.LC0(%rip), %rsi
	movl	$1, %edi
	xorl	%eax, %eax
	call	__printf_chk@PLT
	movl	$1, %eax
	addq	$8, %rsp
	.cfi_def_cfa_offset 8
	ret
	.cfi_endproc
.LFE625:
	.size	CryptHash, .-CryptHash
	.section	.rodata
	.align 16
	.type	_ZL4RCON, @object
	.size	_ZL4RCON, 24
_ZL4RCON:
	.ascii	"$?j\210\205\243\b\323\023\031\212.\003psD\244\t8\")\2371\320"
	.align 32
	.type	_ZL3rho, @object
	.size	_ZL3rho, 64
_ZL3rho:
	.long	0
	.long	14
	.long	20
	.long	22
	.long	0
	.long	13
	.long	68
	.long	91
	.long	0
	.long	27
	.long	42
	.long	106
	.long	0
	.long	32
	.long	48
	.long	80
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
