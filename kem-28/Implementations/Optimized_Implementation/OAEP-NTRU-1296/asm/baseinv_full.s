.text

.macro MONT32 src dst tmp
	vpmulld %ymm15, \src, \tmp
	vpand   %ymm13, \tmp, \tmp
	vpslld  $16,    \tmp, \tmp
	vpsrad  $16,    \tmp, \tmp
	vpmulld %ymm14, \tmp, \tmp
	vpsubd  \tmp,   \src, \dst
	vpsrad  $16,    \dst, \dst
.endm

.macro FQMUL32 a b dst tmp
	vpmulld \b, \a, \dst
	MONT32  \dst, \dst, \tmp
.endm

.macro LOAD8_PAIRS base
	vmovdqu      (\base), %ymm2
	vextracti128 $1, %ymm2, %xmm3

	vpshufb baseinv_pair_even(%rip), %xmm2, %xmm4
	vpshufb baseinv_pair_even(%rip), %xmm3, %xmm5
	vpslldq $8, %xmm5, %xmm5
	vpor    %xmm5, %xmm4, %xmm4
	vpmovsxwd %xmm4, %ymm0

	vpshufb baseinv_pair_odd(%rip), %xmm2, %xmm4
	vpshufb baseinv_pair_odd(%rip), %xmm3, %xmm5
	vpslldq $8, %xmm5, %xmm5
	vpor    %xmm5, %xmm4, %xmm4
	vpmovsxwd %xmm4, %ymm1
.endm

.macro LOAD8_ZETAS zptr
	vmovq      (\zptr), %xmm2
	vpxor      %xmm3, %xmm3, %xmm3
	vpsubw     %xmm2, %xmm3, %xmm3
	vpunpcklwd %xmm3, %xmm2, %xmm2
	vpmovsxwd  %xmm2, %ymm2
.endm

.macro PACK8X32_TO_XMM src dst
	vpackssdw \src, \src, \dst
	vpermq    $0x08, \dst, \dst
.endm

.macro STORE8_PAIRS base r0 r1
	PACK8X32_TO_XMM \r0, %ymm7
	PACK8X32_TO_XMM \r1, %ymm8
	vpunpcklwd %xmm8, %xmm7, %xmm9
	vpunpckhwd %xmm8, %xmm7, %xmm10
	vmovdqu %xmm9,  0(\base)
	vmovdqu %xmm10, 16(\base)
.endm

.macro FQINV_Q_MINUS_2
	vmovdqa %ymm0, %ymm5
	FQMUL32 %ymm0, %ymm0, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm2, %ymm10
	FQMUL32 %ymm2, %ymm2, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm3, %ymm10
	FQMUL32 %ymm3, %ymm3, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm5, %ymm4, %ymm10
	FQMUL32 %ymm4, %ymm2, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm3, %ymm7, %ymm10
	FQMUL32 %ymm7, %ymm6, %ymm8, %ymm10
	FQMUL32 %ymm8, %ymm4, %ymm9, %ymm10
	FQMUL32 %ymm9, %ymm9, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm7, %ymm0, %ymm10
.endm

.global poly_baseinv_prepare_asm
.type poly_baseinv_prepare_asm, @function
poly_baseinv_prepare_asm:
	vmovdqa baseinv_8xq(%rip),    %ymm14
	vmovdqa baseinv_8xqinv(%rip), %ymm15
	vmovdqa baseinv_8xmask(%rip), %ymm13
	lea     zetas+648(%rip), %r8
	mov     $81, %ecx

.p2align 5
.Lprepare_loop:
	LOAD8_PAIRS %rsi
	LOAD8_ZETAS %r8

	FQMUL32 %ymm1, %ymm2, %ymm3, %ymm11
	vpmulld %ymm1, %ymm3, %ymm4
	vpmulld %ymm0, %ymm0, %ymm5
	vpsubd  %ymm5, %ymm4, %ymm4
	MONT32  %ymm4, %ymm4, %ymm11
	vmovdqu %ymm4, (%rdi)

	add $32, %rsi
	add $32, %rdi
	add $8,  %r8
	dec %ecx
	jnz .Lprepare_loop

	vzeroupper
	ret

.global poly_baseinv_batchinvert_asm
.type poly_baseinv_batchinvert_asm, @function
poly_baseinv_batchinvert_asm:
	sub $2592, %rsp
	vmovdqa baseinv_8xq(%rip),    %ymm14
	vmovdqa baseinv_8xqinv(%rip), %ymm15
	vmovdqa baseinv_8xmask(%rip), %ymm13

	mov %rdi, %r8
	mov %rsp, %r9
	vmovdqu (%r8), %ymm0
	vmovdqu %ymm0, (%r9)
	add $32, %r8
	add $32, %r9
	mov $80, %ecx

.p2align 5
.Lbatch_product_loop:
	vmovdqu (%r8), %ymm1
	FQMUL32 %ymm0, %ymm1, %ymm0, %ymm10
	vmovdqu %ymm0, (%r9)
	add $32, %r8
	add $32, %r9
	dec %ecx
	jnz .Lbatch_product_loop

	vpxor    %ymm12, %ymm12, %ymm12
	vpcmpeqd %ymm12, %ymm0, %ymm1
	vpmovmskb %ymm1, %eax
	test %eax, %eax
	jnz .Lbatch_zero

	FQINV_Q_MINUS_2

	lea 2560(%rdi), %r8
	lea 2528(%rsp), %r9
	mov $80, %ecx

.p2align 5
.Lbatch_derive_loop:
	vmovdqu (%r8), %ymm5
	vmovdqu (%r9), %ymm6
	FQMUL32 %ymm6, %ymm0, %ymm7, %ymm10
	vmovdqu %ymm7, (%r8)
	FQMUL32 %ymm0, %ymm5, %ymm0, %ymm10
	sub $32, %r8
	sub $32, %r9
	dec %ecx
	jnz .Lbatch_derive_loop

	vmovdqu %ymm0, (%rdi)
	xor %eax, %eax
	add $2592, %rsp
	vzeroupper
	ret

.Lbatch_zero:
	mov $1, %eax
	add $2592, %rsp
	vzeroupper
	ret

.global poly_baseinv_finalize_asm
.type poly_baseinv_finalize_asm, @function
poly_baseinv_finalize_asm:
	vmovdqa baseinv_8xq(%rip),    %ymm14
	vmovdqa baseinv_8xqinv(%rip), %ymm15
	vmovdqa baseinv_8xmask(%rip), %ymm13
	vpxor   %ymm12, %ymm12, %ymm12
	mov     $81, %ecx

.p2align 5
.Lfinalize_loop:
	LOAD8_PAIRS %rsi
	vmovdqu (%rdx), %ymm2

	vpmulld %ymm2, %ymm0, %ymm3
	vpsubd  %ymm3, %ymm12, %ymm3
	MONT32  %ymm3, %ymm3, %ymm11
	FQMUL32 %ymm1, %ymm2, %ymm4, %ymm11
	STORE8_PAIRS %rdi, %ymm3, %ymm4

	add $32, %rsi
	add $32, %rdi
	add $32, %rdx
	dec %ecx
	jnz .Lfinalize_loop

	vzeroupper
	ret

.section .rodata
.p2align 5
baseinv_8xq:
	.long 17497, 17497, 17497, 17497, 17497, 17497, 17497, 17497
baseinv_8xqinv:
	.long 50153, 50153, 50153, 50153, 50153, 50153, 50153, 50153
baseinv_8xmask:
	.long 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535

.p2align 4
baseinv_pair_even:
	.byte 0,1, 4,5, 8,9, 12,13, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_pair_odd:
	.byte 2,3, 6,7, 10,11, 14,15, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80

.section .note.GNU-stack,"",@progbits
