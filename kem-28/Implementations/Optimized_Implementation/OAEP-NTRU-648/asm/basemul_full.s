.text

.macro FQMUL16 a b dst tmp
	vpmullw %ymm15, \a, \tmp
	vpmullw \b,     \tmp, \tmp
	vpmulhw \b,     \a,   \dst
	vpmulhw %ymm14, \tmp, \tmp
	vpsubw  \tmp,   \dst, \dst
.endm

.macro LOAD8_TRIPLES_AT base out0 out1 out2 off0 off1 off2
	vmovdqu \off0(\base), %xmm10
	vmovdqu \off1(\base), %xmm11
	vmovdqu \off2(\base), %xmm12

	vpshufb basemul_load_c0_m0(%rip), %xmm10, \out0
	vpshufb basemul_load_c0_m1(%rip), %xmm11, %xmm13
	vpor    %xmm13, \out0, \out0
	vpshufb basemul_load_c0_m2(%rip), %xmm12, %xmm13
	vpor    %xmm13, \out0, \out0

	vpshufb basemul_load_c1_m0(%rip), %xmm10, \out1
	vpshufb basemul_load_c1_m1(%rip), %xmm11, %xmm13
	vpor    %xmm13, \out1, \out1
	vpshufb basemul_load_c1_m2(%rip), %xmm12, %xmm13
	vpor    %xmm13, \out1, \out1

	vpshufb basemul_load_c2_m0(%rip), %xmm10, \out2
	vpshufb basemul_load_c2_m1(%rip), %xmm11, %xmm13
	vpor    %xmm13, \out2, \out2
	vpshufb basemul_load_c2_m2(%rip), %xmm12, %xmm13
	vpor    %xmm13, \out2, \out2
.endm

.macro LOAD16_TRIPLES_A base
	LOAD8_TRIPLES_AT \base, %xmm0, %xmm1, %xmm2, 0, 16, 32
	LOAD8_TRIPLES_AT \base, %xmm6, %xmm7, %xmm8, 48, 64, 80
	vinserti128 $1, %xmm6, %ymm0, %ymm0
	vinserti128 $1, %xmm7, %ymm1, %ymm1
	vinserti128 $1, %xmm8, %ymm2, %ymm2
.endm

.macro LOAD16_TRIPLES_B base
	LOAD8_TRIPLES_AT \base, %xmm3, %xmm4, %xmm5, 0, 16, 32
	LOAD8_TRIPLES_AT \base, %xmm6, %xmm7, %xmm8, 48, 64, 80
	vinserti128 $1, %xmm6, %ymm3, %ymm3
	vinserti128 $1, %xmm7, %ymm4, %ymm4
	vinserti128 $1, %xmm8, %ymm5, %ymm5
.endm

.macro LOAD_ZETAS16 ptr out
	vmovdqa   (\ptr), %ymm10
	vmovdqa 32(\ptr), %ymm11
	vpackssdw %ymm11, %ymm10, \out
	vpermq    $0xd8, \out, \out
.endm

.macro LOAD_ZETAS8 ptr out
	vmovdqa   (\ptr), %ymm10
	vpxor     %ymm11, %ymm11, %ymm11
	vpackssdw %ymm11, %ymm10, \out
	vpermq    $0xd8, \out, \out
.endm

.macro BASEMUL_BODY
	FQMUL16 %ymm1, %ymm5, %ymm7,  %ymm13
	FQMUL16 %ymm2, %ymm4, %ymm8,  %ymm13
	vpaddw  %ymm8, %ymm7, %ymm7

	FQMUL16 %ymm2, %ymm5, %ymm9,  %ymm13

	FQMUL16 %ymm7, %ymm6, %ymm10, %ymm13
	FQMUL16 %ymm0, %ymm3, %ymm11, %ymm13
	vpaddw  %ymm11, %ymm10, %ymm10

	FQMUL16 %ymm9, %ymm6, %ymm11, %ymm13
	FQMUL16 %ymm0, %ymm4, %ymm12, %ymm13
	vpaddw  %ymm12, %ymm11, %ymm11
	FQMUL16 %ymm1, %ymm3, %ymm12, %ymm13
	vpaddw  %ymm12, %ymm11, %ymm11

	FQMUL16 %ymm2, %ymm3, %ymm12, %ymm13
	FQMUL16 %ymm1, %ymm4, %ymm8,  %ymm13
	vpaddw  %ymm8, %ymm12, %ymm12
	FQMUL16 %ymm0, %ymm5, %ymm8,  %ymm13
	vpaddw  %ymm8, %ymm12, %ymm12
.endm

.macro BARRETT16 reg
	vpmulhw  %ymm0, \reg, %ymm13
	vpsraw   $11, %ymm13, %ymm13
	vpmullw  %ymm14, %ymm13, %ymm13
	vpsubw   %ymm13, \reg, \reg
	vpcmpgtw %ymm2, \reg, %ymm13
	vpand    %ymm14, %ymm13, %ymm13
	vpsubw   %ymm13, \reg, \reg
	vpcmpgtw \reg, %ymm1, %ymm13
	vpand    %ymm14, %ymm13, %ymm13
	vpaddw   %ymm13, \reg, \reg
.endm

.macro LOAD_BARRETT_CONSTS
	vmovdqa oaep648_16xv(%rip), %ymm0
	vmovdqa oaep648_16xneghalf(%rip), %ymm1
	vmovdqa oaep648_16xhalf(%rip), %ymm2
.endm

.macro STORE8_TRIPLES_AT base in0 in1 in2 off0 off1 off2
	vpshufb basemul_store_c0_m0(%rip), \in0, %xmm0
	vpshufb basemul_store_c0_m1(%rip), \in1, %xmm1
	vpshufb basemul_store_c0_m2(%rip), \in2, %xmm2
	vpor    %xmm1, %xmm0, %xmm0
	vpor    %xmm2, %xmm0, %xmm0
	vmovdqu %xmm0, \off0(\base)

	vpshufb basemul_store_c1_m0(%rip), \in0, %xmm0
	vpshufb basemul_store_c1_m1(%rip), \in1, %xmm1
	vpshufb basemul_store_c1_m2(%rip), \in2, %xmm2
	vpor    %xmm1, %xmm0, %xmm0
	vpor    %xmm2, %xmm0, %xmm0
	vmovdqu %xmm0, \off1(\base)

	vpshufb basemul_store_c2_m0(%rip), \in0, %xmm0
	vpshufb basemul_store_c2_m1(%rip), \in1, %xmm1
	vpshufb basemul_store_c2_m2(%rip), \in2, %xmm2
	vpor    %xmm1, %xmm0, %xmm0
	vpor    %xmm2, %xmm0, %xmm0
	vmovdqu %xmm0, \off2(\base)
.endm

.macro STORE16_TRIPLES base in0 in1 in2
	STORE8_TRIPLES_AT \base, %xmm10, %xmm11, %xmm12, 0, 16, 32
	vextracti128 $1, \in0, %xmm6
	vextracti128 $1, \in1, %xmm7
	vextracti128 $1, \in2, %xmm8
	STORE8_TRIPLES_AT \base, %xmm6, %xmm7, %xmm8, 48, 64, 80
.endm

.global poly_basemul_asm
.type poly_basemul_asm, @function
poly_basemul_asm:
	vmovdqa oaep648_16xq(%rip), %ymm14
	vmovdqa basemul_16xqinv(%rip), %ymm15
	lea     oaep648_baseinv_zetas8(%rip), %r8
	mov     $13, %ecx

.p2align 5
.Lbasemul_loop16:
	LOAD16_TRIPLES_A %rsi
	LOAD16_TRIPLES_B %rdx
	LOAD_ZETAS16 %r8, %ymm6
	BASEMUL_BODY
	LOAD_BARRETT_CONSTS
	BARRETT16 %ymm10
	BARRETT16 %ymm11
	BARRETT16 %ymm12
	STORE16_TRIPLES %rdi, %ymm10, %ymm11, %ymm12

	add $96, %rsi
	add $96, %rdx
	add $96, %rdi
	add $64, %r8
	dec %ecx
	jnz .Lbasemul_loop16

	LOAD8_TRIPLES_AT %rsi, %xmm0, %xmm1, %xmm2, 0, 16, 32
	LOAD8_TRIPLES_AT %rdx, %xmm3, %xmm4, %xmm5, 0, 16, 32
	LOAD_ZETAS8 %r8, %ymm6
	BASEMUL_BODY
	LOAD_BARRETT_CONSTS
	BARRETT16 %ymm10
	BARRETT16 %ymm11
	BARRETT16 %ymm12
	STORE8_TRIPLES_AT %rdi, %xmm10, %xmm11, %xmm12, 0, 16, 32

	vzeroupper
	ret

.section .rodata
.p2align 5
basemul_16xqinv:
	.short -19351, -19351, -19351, -19351, -19351, -19351, -19351, -19351
	.short -19351, -19351, -19351, -19351, -19351, -19351, -19351, -19351

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
