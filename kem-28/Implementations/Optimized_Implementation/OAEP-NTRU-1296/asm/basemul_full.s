.text

.macro FQMUL16 a b dst
	vpmullw \b,     \a,   %ymm4
	vpmulhw \b,     \a,   \dst
	vpmullw %ymm11, %ymm4, %ymm5
	vpmullw %ymm10, %ymm5, %ymm6
	vpmulhw %ymm10, %ymm5, %ymm5
	vpxor   %ymm12, %ymm6, %ymm6
	vpxor   %ymm12, %ymm4, %ymm4
	vpcmpgtw %ymm4, %ymm6, %ymm6
	vpand   %ymm13, %ymm6, %ymm6
	vpsubw  %ymm5,  \dst, \dst
	vpsubw  %ymm6,  \dst, \dst
.endm

.macro MONT32 src dst
	vpmulld %ymm15, \src, %ymm4
	vpand   %ymm9,  %ymm4, %ymm4
	vpslld  $16,    %ymm4, %ymm4
	vpsrad  $16,    %ymm4, %ymm4
	vpmulld %ymm14, %ymm4, %ymm4
	vpsubd  %ymm4,  \src,  \dst
	vpsrad  $16,    \dst,  \dst
.endm

.macro LOAD8_ZETAS_ODD16 zptr off
	vmovq      \off(\zptr), %xmm3
	vpxor      %xmm4, %xmm4, %xmm4
	vpsubw     %xmm3, %xmm4, %xmm5
	vpunpcklwd %xmm5, %xmm3, %xmm3
	vpxor      %xmm4, %xmm4, %xmm4
	vpunpckhwd %xmm3, %xmm4, %xmm5
	vpunpcklwd %xmm3, %xmm4, %xmm3
	vinserti128 $1, %xmm5, %ymm3, %ymm3
.endm

.macro PACK8X32_TO_XMM src dst
	vpackssdw \src, \src, \dst
	vpermq    $0x08, \dst, \dst
.endm

.macro STORE8_PAIRS base off r0 r1
	PACK8X32_TO_XMM \r0, %ymm4
	PACK8X32_TO_XMM \r1, %ymm5
	vpunpcklwd %xmm5, %xmm4, %xmm6
	vpunpckhwd %xmm5, %xmm4, %xmm7
	vmovdqu %xmm6, \off(\base)
	vmovdqu %xmm7, (16 + \off)(\base)
.endm

.macro BASEMUL8_BLOCK aoff boff roff zoff
	vmovdqu \aoff(%rsi), %ymm0
	vmovdqu \boff(%rdx), %ymm1
	LOAD8_ZETAS_ODD16 %r8, \zoff

	FQMUL16 %ymm0, %ymm1, %ymm2
	vpblendw $0xaa, %ymm2, %ymm0, %ymm2
	vpblendw $0xaa, %ymm3, %ymm1, %ymm3
	vpmaddwd %ymm3, %ymm2, %ymm2
	MONT32 %ymm2, %ymm2

	vpshufb  %ymm8, %ymm1, %ymm3
	vpmaddwd %ymm3, %ymm0, %ymm3
	MONT32 %ymm3, %ymm3

	STORE8_PAIRS %rdi, \roff, %ymm2, %ymm3
.endm

.global poly_basemul_asm
.type poly_basemul_asm, @function
poly_basemul_asm:
	vmovdqa basemul_16xq(%rip),     %ymm10
	vmovdqa basemul_16xqinv(%rip),  %ymm11
	vmovdqa basemul_16xsign(%rip),  %ymm12
	vmovdqa basemul_16xone(%rip),   %ymm13
	vmovdqa basemul_8xq(%rip),      %ymm14
	vmovdqa basemul_8xqinv(%rip),   %ymm15
	vmovdqa basemul_8xmask(%rip),   %ymm9
	vmovdqa basemul_pair_swap(%rip), %ymm8
	lea     zetas+648(%rip), %r8
	mov     $3, %ecx

.p2align 5
.Lbasemul_loop:
	.set BM_OFF, 0
	.set BM_ZOFF, 0
	.rept 27
	BASEMUL8_BLOCK BM_OFF, BM_OFF, BM_OFF, BM_ZOFF
	.set BM_OFF, BM_OFF + 32
	.set BM_ZOFF, BM_ZOFF + 8
	.endr

	add $864, %rsi
	add $864, %rdx
	add $864, %rdi
	add $216, %r8
	dec %ecx
	jnz .Lbasemul_loop

	vzeroupper
	ret

.section .rodata
.p2align 5
basemul_16xq:
	.short 17497, 17497, 17497, 17497, 17497, 17497, 17497, 17497
	.short 17497, 17497, 17497, 17497, 17497, 17497, 17497, 17497
basemul_16xqinv:
	.short -15383, -15383, -15383, -15383, -15383, -15383, -15383, -15383
	.short -15383, -15383, -15383, -15383, -15383, -15383, -15383, -15383
basemul_16xsign:
	.short -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768
	.short -32768, -32768, -32768, -32768, -32768, -32768, -32768, -32768
basemul_16xone:
	.short 1, 1, 1, 1, 1, 1, 1, 1
	.short 1, 1, 1, 1, 1, 1, 1, 1

.p2align 5
basemul_8xq:
	.long 17497, 17497, 17497, 17497, 17497, 17497, 17497, 17497
basemul_8xqinv:
	.long 50153, 50153, 50153, 50153, 50153, 50153, 50153, 50153
basemul_8xmask:
	.long 65535, 65535, 65535, 65535, 65535, 65535, 65535, 65535

.p2align 5
basemul_pair_swap:
	.byte 2,3, 0,1, 6,7, 4,5, 10,11, 8,9, 14,15, 12,13
	.byte 2,3, 0,1, 6,7, 4,5, 10,11, 8,9, 14,15, 12,13

.section .note.GNU-stack,"",@progbits
