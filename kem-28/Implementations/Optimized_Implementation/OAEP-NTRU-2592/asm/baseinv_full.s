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

.macro CENTER32_ONCE reg tmp
	vpcmpgtd baseinv_8xupper(%rip), \reg, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpsubd   \tmp, \reg, \reg
	vmovdqa  baseinv_8xlower(%rip), \tmp
	vpcmpgtd \reg, \tmp, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpaddd   \tmp, \reg, \reg
.endm

.macro CENTER32 reg tmp
	CENTER32_ONCE \reg, \tmp
	CENTER32_ONCE \reg, \tmp
.endm

.macro LOAD8_TRIPLES base
	vmovdqu   0(\base), %xmm3
	vmovdqu  16(\base), %xmm4
	vmovdqu  32(\base), %xmm5

	vpshufb baseinv_load_c0_m0(%rip), %xmm3, %xmm6
	vpshufb baseinv_load_c0_m1(%rip), %xmm4, %xmm7
	vpshufb baseinv_load_c0_m2(%rip), %xmm5, %xmm8
	vpor    %xmm7, %xmm6, %xmm6
	vpor    %xmm8, %xmm6, %xmm6
	vpmovsxwd %xmm6, %ymm0

	vpshufb baseinv_load_c1_m0(%rip), %xmm3, %xmm6
	vpshufb baseinv_load_c1_m1(%rip), %xmm4, %xmm7
	vpshufb baseinv_load_c1_m2(%rip), %xmm5, %xmm8
	vpor    %xmm7, %xmm6, %xmm6
	vpor    %xmm8, %xmm6, %xmm6
	vpmovsxwd %xmm6, %ymm1

	vpshufb baseinv_load_c2_m0(%rip), %xmm3, %xmm6
	vpshufb baseinv_load_c2_m1(%rip), %xmm4, %xmm7
	vpshufb baseinv_load_c2_m2(%rip), %xmm5, %xmm8
	vpor    %xmm7, %xmm6, %xmm6
	vpor    %xmm8, %xmm6, %xmm6
	vpmovsxwd %xmm6, %ymm2
.endm

.macro LOAD8_ZETAS zptr
	vmovq      (\zptr), %xmm3
	vpxor      %xmm4, %xmm4, %xmm4
	vpsubw     %xmm3, %xmm4, %xmm4
	vpunpcklwd %xmm4, %xmm3, %xmm3
	vpmovsxwd  %xmm3, %ymm3
.endm

.macro PACK8X32_TO_XMM src dst
	vpackssdw \src, \src, \dst
	vpermq    $0x08, \dst, \dst
.endm

.macro STORE8_TRIPLES base r0 r1 r2
	PACK8X32_TO_XMM \r0, %ymm7
	PACK8X32_TO_XMM \r1, %ymm8
	PACK8X32_TO_XMM \r2, %ymm9

	vpshufb baseinv_store_c0_m0(%rip), %xmm7, %xmm10
	vpshufb baseinv_store_c0_m1(%rip), %xmm8, %xmm11
	vpshufb baseinv_store_c0_m2(%rip), %xmm9, %xmm4
	vpor    %xmm11, %xmm10, %xmm10
	vpor    %xmm4,  %xmm10, %xmm10
	vmovdqu %xmm10, 0(\base)

	vpshufb baseinv_store_c1_m0(%rip), %xmm7, %xmm10
	vpshufb baseinv_store_c1_m1(%rip), %xmm8, %xmm11
	vpshufb baseinv_store_c1_m2(%rip), %xmm9, %xmm4
	vpor    %xmm11, %xmm10, %xmm10
	vpor    %xmm4,  %xmm10, %xmm10
	vmovdqu %xmm10, 16(\base)

	vpshufb baseinv_store_c2_m0(%rip), %xmm7, %xmm10
	vpshufb baseinv_store_c2_m1(%rip), %xmm8, %xmm11
	vpshufb baseinv_store_c2_m2(%rip), %xmm9, %xmm4
	vpor    %xmm11, %xmm10, %xmm10
	vpor    %xmm4,  %xmm10, %xmm10
	vmovdqu %xmm10, 32(\base)
.endm

.macro FQINV_Q_MINUS_2
	vmovdqa %ymm0, %ymm5
	FQMUL32 %ymm0, %ymm0, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm1, %ymm10
	FQMUL32 %ymm1, %ymm1, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm2, %ymm10
	FQMUL32 %ymm2, %ymm2, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm3, %ymm10
	FQMUL32 %ymm3, %ymm3, %ymm4, %ymm10
	FQMUL32 %ymm4, %ymm3, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm1, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm2, %ymm7, %ymm10
	FQMUL32 %ymm7, %ymm6, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm7, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm5, %ymm6, %ymm10
	FQMUL32 %ymm6, %ymm6, %ymm7, %ymm10
	FQMUL32 %ymm7, %ymm7, %ymm8, %ymm10
	FQMUL32 %ymm8, %ymm7, %ymm8, %ymm10
	FQMUL32 %ymm8, %ymm6, %ymm0, %ymm10
.endm

.global poly_baseinv_prepare_asm
.type poly_baseinv_prepare_asm, @function
poly_baseinv_prepare_asm:
	vmovdqa oaep2592_8xq(%rip),     %ymm14
	vmovdqa oaep2592_8xqinv(%rip),  %ymm15
	vmovdqa oaep2592_8xmask(%rip),  %ymm13
	vmovdqa baseinv_8xthree_mont(%rip), %ymm12
	lea     zetas+864(%rip), %r8
	mov     $108, %ecx

.p2align 5
.Lprepare_loop:
	LOAD8_TRIPLES %rsi
	LOAD8_ZETAS %r8

	FQMUL32 %ymm1, %ymm1, %ymm4, %ymm11
	FQMUL32 %ymm0, %ymm2, %ymm5, %ymm11
	FQMUL32 %ymm5, %ymm12, %ymm6, %ymm11
	vpsubd  %ymm6, %ymm4, %ymm7
	CENTER32 %ymm7, %ymm11

	FQMUL32 %ymm2, %ymm3, %ymm8, %ymm11
	FQMUL32 %ymm8, %ymm8, %ymm9, %ymm11
	FQMUL32 %ymm7, %ymm1, %ymm10, %ymm11
	FQMUL32 %ymm0, %ymm0, %ymm4, %ymm11

	FQMUL32 %ymm9,  %ymm2, %ymm5, %ymm11
	FQMUL32 %ymm4,  %ymm0, %ymm6, %ymm11
	FQMUL32 %ymm10, %ymm3, %ymm7, %ymm11
	vpaddd  %ymm6, %ymm5, %ymm5
	vpaddd  %ymm7, %ymm5, %ymm5
	CENTER32 %ymm5, %ymm11
	vmovdqu %ymm5, (%rdi)

	add $48, %rsi
	add $32, %rdi
	add $8,  %r8
	dec %ecx
	jnz .Lprepare_loop

	vzeroupper
	ret

.global poly_baseinv_batchinvert_asm
.type poly_baseinv_batchinvert_asm, @function
poly_baseinv_batchinvert_asm:
	sub $3456, %rsp
	vmovdqa oaep2592_8xq(%rip),     %ymm14
	vmovdqa oaep2592_8xqinv(%rip),  %ymm15
	vmovdqa oaep2592_8xmask(%rip),  %ymm13

	mov %rdi, %r8
	mov %rsp, %r9
	vmovdqu (%r8), %ymm0
	vmovdqu %ymm0, (%r9)
	add $32, %r8
	add $32, %r9
	mov $107, %ecx

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

	lea 3424(%rdi), %r8
	lea 3392(%rsp), %r9
	mov $107, %ecx

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
	add $3456, %rsp
	vzeroupper
	ret

.Lbatch_zero:
	mov $1, %eax
	add $3456, %rsp
	vzeroupper
	ret

.global poly_baseinv_finalize_asm
.type poly_baseinv_finalize_asm, @function
poly_baseinv_finalize_asm:
	vmovdqa oaep2592_8xq(%rip),     %ymm14
	vmovdqa oaep2592_8xqinv(%rip),  %ymm15
	vmovdqa oaep2592_8xmask(%rip),  %ymm13
	lea     zetas+864(%rip), %r8
	mov     $108, %ecx

.p2align 5
.Lfinalize_loop:
	LOAD8_TRIPLES %rsi
	LOAD8_ZETAS %r8
	vmovdqu (%rdx), %ymm4

	FQMUL32 %ymm1, %ymm1, %ymm5, %ymm11
	FQMUL32 %ymm0, %ymm2, %ymm6, %ymm11
	vpsubd  %ymm6, %ymm5, %ymm7
	CENTER32 %ymm7, %ymm11

	FQMUL32 %ymm2, %ymm3, %ymm8, %ymm11
	FQMUL32 %ymm8, %ymm4, %ymm9, %ymm11
	FQMUL32 %ymm0, %ymm4, %ymm10, %ymm11
	FQMUL32 %ymm7, %ymm4, %ymm6, %ymm11

	FQMUL32 %ymm0, %ymm10, %ymm5, %ymm11
	FQMUL32 %ymm1, %ymm9,  %ymm8, %ymm11
	vpsubd  %ymm8, %ymm5, %ymm5
	CENTER32 %ymm5, %ymm11

	FQMUL32 %ymm2, %ymm9,  %ymm8, %ymm11
	FQMUL32 %ymm1, %ymm10, %ymm4, %ymm11
	vpsubd  %ymm4, %ymm8, %ymm8
	CENTER32 %ymm8, %ymm11

	STORE8_TRIPLES %rdi, %ymm5, %ymm8, %ymm6

	add $48, %rsi
	add $48, %rdi
	add $32, %rdx
	add $8,  %r8
	dec %ecx
	jnz .Lfinalize_loop

	vzeroupper
	ret

.section .rodata
.p2align 5
baseinv_8xthree_mont:
	.long -2983, -2983, -2983, -2983, -2983, -2983, -2983, -2983
baseinv_8xupper:
	.long 14255, 14255, 14255, 14255, 14255, 14255, 14255, 14255
baseinv_8xlower:
	.long -14256, -14256, -14256, -14256, -14256, -14256, -14256, -14256

.p2align 4
baseinv_load_c0_m0:
	.byte 0,1, 6,7, 12,13, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_load_c0_m1:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 2,3, 8,9, 14,15, 0x80,0x80, 0x80,0x80
baseinv_load_c0_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 4,5, 10,11
baseinv_load_c1_m0:
	.byte 2,3, 8,9, 14,15, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_load_c1_m1:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 4,5, 10,11, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_load_c1_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0,1, 6,7, 12,13
baseinv_load_c2_m0:
	.byte 4,5, 10,11, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_load_c2_m1:
	.byte 0x80,0x80, 0x80,0x80, 0,1, 6,7, 12,13, 0x80,0x80, 0x80,0x80, 0x80,0x80
baseinv_load_c2_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 2,3, 8,9, 14,15

.p2align 4
baseinv_store_c0_m0:
	.byte 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80, 4,5, 0x80,0x80
baseinv_store_c0_m1:
	.byte 0x80,0x80, 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80, 4,5
baseinv_store_c0_m2:
	.byte 0x80,0x80, 0x80,0x80, 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80
baseinv_store_c1_m0:
	.byte 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80, 0x80,0x80, 10,11
baseinv_store_c1_m1:
	.byte 0x80,0x80, 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80, 0x80,0x80
baseinv_store_c1_m2:
	.byte 4,5, 0x80,0x80, 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80
baseinv_store_c2_m0:
	.byte 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15, 0x80,0x80, 0x80,0x80
baseinv_store_c2_m1:
	.byte 10,11, 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15, 0x80,0x80
baseinv_store_c2_m2:
	.byte 0x80,0x80, 10,11, 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15

.section .note.GNU-stack,"",@progbits
