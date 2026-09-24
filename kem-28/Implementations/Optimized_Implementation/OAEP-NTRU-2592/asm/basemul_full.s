.text

.macro MONT32 src dst tmp
	vpmulld %ymm15, \src, \tmp
	vpand   %ymm13, \tmp, \tmp
	vpslld  $16,    \tmp, \tmp
	vpsrad  $16,    \tmp, \tmp
	vpmulld %ymm14, \tmp, \tmp
	vpsubd  \tmp,  \src, \dst
	vpsrad  $16,   \dst, \dst
.endm

.macro FQMUL32 a b dst tmp
	vpmulld \b, \a, \dst
	MONT32 \dst, \dst, \tmp
.endm

.macro CENTER32 reg tmp
	vpcmpgtd %ymm12, \reg, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpsubd   \tmp, \reg, \reg
	vpcmpgtd \reg, %ymm11, \tmp
	vpand    %ymm14, \tmp, \tmp
	vpaddd   \tmp, \reg, \reg
.endm

.macro CENTER32_WIDE reg tmp
	CENTER32 \reg, \tmp
	CENTER32 \reg, \tmp
.endm

.macro ACCUM_OUT_OF_CENTER reg acc tmp
	vpcmpgtd %ymm12, \reg, \tmp
	vpor     \tmp, \acc, \acc
	vpcmpgtd \reg, %ymm11, \tmp
	vpor     \tmp, \acc, \acc
.endm

.macro LOAD8_TRIPLES_32 base off out0 out1 out2
	vmovdqu \off(\base), %xmm6
	vmovdqu (16 + \off)(\base), %xmm7
	vmovdqu (32 + \off)(\base), %xmm8

	vpshufb basemul_load_c0_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_load_c0_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_load_c0_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpmovsxwd %xmm9, \out0

	vpshufb basemul_load_c1_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_load_c1_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_load_c1_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpmovsxwd %xmm9, \out1

	vpshufb basemul_load_c2_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_load_c2_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_load_c2_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpmovsxwd %xmm9, \out2
.endm

.macro LOAD8_ZETAS_32 zptr off out
	vmovq      \off(\zptr), %xmm6
	vpxor      %xmm7, %xmm7, %xmm7
	vpsubw     %xmm6, %xmm7, %xmm8
	vpunpcklwd %xmm8, %xmm6, %xmm6
	vpmovsxwd  %xmm6, \out
.endm

.macro PACK8X32_TO_XMM src dst
	vpackssdw \src, \src, \dst
	vpermq    $0x08, \dst, \dst
.endm

.macro STORE8_TRIPLES_32 base off in0 in1 in2
	PACK8X32_TO_XMM \in0, %ymm6
	PACK8X32_TO_XMM \in1, %ymm7
	PACK8X32_TO_XMM \in2, %ymm8

	vpshufb basemul_store_c0_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_store_c0_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_store_c0_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vmovdqu %xmm9, \off(\base)

	vpshufb basemul_store_c1_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_store_c1_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_store_c1_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vmovdqu %xmm9, (16 + \off)(\base)

	vpshufb basemul_store_c2_m0(%rip), %xmm6, %xmm9
	vpshufb basemul_store_c2_m1(%rip), %xmm7, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vpshufb basemul_store_c2_m2(%rip), %xmm8, %xmm10
	vpor    %xmm10, %xmm9, %xmm9
	vmovdqu %xmm9, (32 + \off)(\base)
.endm

.macro BASEMUL8_BLOCK aoff boff roff zoff
	LOAD8_TRIPLES_32 %rsi, \aoff, %ymm0, %ymm1, %ymm2
	LOAD8_TRIPLES_32 %rdx, \boff, %ymm3, %ymm4, %ymm5
	LOAD8_ZETAS_32 %r8, \zoff, %ymm6

	vpxor %ymm10, %ymm10, %ymm10
	ACCUM_OUT_OF_CENTER %ymm0, %ymm10, %ymm9
	ACCUM_OUT_OF_CENTER %ymm1, %ymm10, %ymm9
	ACCUM_OUT_OF_CENTER %ymm2, %ymm10, %ymm9
	ACCUM_OUT_OF_CENTER %ymm3, %ymm10, %ymm9
	ACCUM_OUT_OF_CENTER %ymm4, %ymm10, %ymm9
	ACCUM_OUT_OF_CENTER %ymm5, %ymm10, %ymm9
	vptest %ymm10, %ymm10
	jnz .Lbasemul_slow_\@

	vpmulld %ymm5, %ymm1, %ymm7
	vpmulld %ymm4, %ymm2, %ymm8
	vpaddd  %ymm8, %ymm7, %ymm7
	MONT32 %ymm7, %ymm7, %ymm10
	CENTER32 %ymm7, %ymm10

	vpmulld %ymm5, %ymm2, %ymm8
	MONT32 %ymm8, %ymm8, %ymm10

	vpmulld %ymm6, %ymm7, %ymm9
	vpmulld %ymm3, %ymm0, %ymm10
	vpaddd  %ymm10, %ymm9, %ymm9
	MONT32 %ymm9, %ymm9, %ymm10
	CENTER32 %ymm9, %ymm10

	vpmulld %ymm6, %ymm8, %ymm7
	vpmulld %ymm4, %ymm0, %ymm10
	vpaddd  %ymm10, %ymm7, %ymm7
	vpmulld %ymm3, %ymm1, %ymm10
	vpaddd  %ymm10, %ymm7, %ymm7
	MONT32 %ymm7, %ymm7, %ymm10
	CENTER32 %ymm7, %ymm10

	vpmulld %ymm3, %ymm2, %ymm8
	vpmulld %ymm4, %ymm1, %ymm10
	vpaddd  %ymm10, %ymm8, %ymm8
	vpmulld %ymm5, %ymm0, %ymm10
	vpaddd  %ymm10, %ymm8, %ymm8
	MONT32 %ymm8, %ymm8, %ymm10
	CENTER32 %ymm8, %ymm10

	STORE8_TRIPLES_32 %rdi, \roff, %ymm9, %ymm7, %ymm8
	jmp .Lbasemul_done_\@

.Lbasemul_slow_\@:
	FQMUL32 %ymm1, %ymm5, %ymm7, %ymm10
	FQMUL32 %ymm2, %ymm4, %ymm8, %ymm10
	vpaddd  %ymm8, %ymm7, %ymm7
	CENTER32_WIDE %ymm7, %ymm10

	FQMUL32 %ymm2, %ymm5, %ymm8, %ymm10

	vpmulld %ymm6, %ymm7, %ymm9
	vpmulld %ymm3, %ymm0, %ymm10
	vpaddd  %ymm10, %ymm9, %ymm9
	MONT32 %ymm9, %ymm9, %ymm10
	CENTER32_WIDE %ymm9, %ymm10

	vpmulld %ymm6, %ymm8, %ymm7
	vpmulld %ymm4, %ymm0, %ymm10
	vpaddd  %ymm10, %ymm7, %ymm7
	MONT32 %ymm7, %ymm7, %ymm10
	FQMUL32 %ymm1, %ymm3, %ymm6, %ymm10
	vpaddd  %ymm6, %ymm7, %ymm7
	CENTER32_WIDE %ymm7, %ymm10

	FQMUL32 %ymm2, %ymm3, %ymm8, %ymm10
	FQMUL32 %ymm1, %ymm4, %ymm6, %ymm10
	vpaddd  %ymm6, %ymm8, %ymm8
	FQMUL32 %ymm0, %ymm5, %ymm6, %ymm10
	vpaddd  %ymm6, %ymm8, %ymm8
	CENTER32_WIDE %ymm8, %ymm10

	STORE8_TRIPLES_32 %rdi, \roff, %ymm9, %ymm7, %ymm8
.Lbasemul_done_\@:
.endm

.global poly_basemul_asm
.type poly_basemul_asm, @function
poly_basemul_asm:
	vmovdqa oaep2592_8xqinv(%rip), %ymm15
	vmovdqa oaep2592_8xq(%rip), %ymm14
	vmovdqa oaep2592_8xmask(%rip), %ymm13
	vmovdqa basemul_8xupper(%rip), %ymm12
	vmovdqa basemul_8xlower(%rip), %ymm11
	lea     zetas+864(%rip), %r8
	mov     $27, %ecx

.p2align 5
.Lbasemul_loop:
	.set BM_OFF, 0
	.set BM_ZOFF, 0
	.rept 4
	BASEMUL8_BLOCK BM_OFF, BM_OFF, BM_OFF, BM_ZOFF
	.set BM_OFF, BM_OFF + 48
	.set BM_ZOFF, BM_ZOFF + 8
	.endr

	add $192, %rsi
	add $192, %rdx
	add $192, %rdi
	add $32, %r8
	dec %ecx
	jnz .Lbasemul_loop

	vzeroupper
	ret

.section .rodata
.p2align 5
basemul_8xupper:
	.long 14256, 14256, 14256, 14256, 14256, 14256, 14256, 14256
basemul_8xlower:
	.long -14256, -14256, -14256, -14256, -14256, -14256, -14256, -14256

.p2align 4
basemul_load_c0_m0:
	.byte 0,1, 6,7, 12,13, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
basemul_load_c0_m1:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 2,3, 8,9, 14,15, 0x80,0x80, 0x80,0x80
basemul_load_c0_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 4,5, 10,11
basemul_load_c1_m0:
	.byte 2,3, 8,9, 14,15, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
basemul_load_c1_m1:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 4,5, 10,11, 0x80,0x80, 0x80,0x80, 0x80,0x80
basemul_load_c1_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0,1, 6,7, 12,13
basemul_load_c2_m0:
	.byte 4,5, 10,11, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80
basemul_load_c2_m1:
	.byte 0x80,0x80, 0x80,0x80, 0,1, 6,7, 12,13, 0x80,0x80, 0x80,0x80, 0x80,0x80
basemul_load_c2_m2:
	.byte 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 0x80,0x80, 2,3, 8,9, 14,15

.p2align 4
basemul_store_c0_m0:
	.byte 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80, 4,5, 0x80,0x80
basemul_store_c0_m1:
	.byte 0x80,0x80, 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80, 4,5
basemul_store_c0_m2:
	.byte 0x80,0x80, 0x80,0x80, 0,1, 0x80,0x80, 0x80,0x80, 2,3, 0x80,0x80, 0x80,0x80
basemul_store_c1_m0:
	.byte 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80, 0x80,0x80, 10,11
basemul_store_c1_m1:
	.byte 0x80,0x80, 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80, 0x80,0x80
basemul_store_c1_m2:
	.byte 4,5, 0x80,0x80, 0x80,0x80, 6,7, 0x80,0x80, 0x80,0x80, 8,9, 0x80,0x80
basemul_store_c2_m0:
	.byte 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15, 0x80,0x80, 0x80,0x80
basemul_store_c2_m1:
	.byte 10,11, 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15, 0x80,0x80
basemul_store_c2_m2:
	.byte 0x80,0x80, 10,11, 0x80,0x80, 0x80,0x80, 12,13, 0x80,0x80, 0x80,0x80, 14,15

.section .note.GNU-stack,"",@progbits
