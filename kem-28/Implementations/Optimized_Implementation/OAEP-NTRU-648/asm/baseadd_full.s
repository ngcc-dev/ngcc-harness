.text

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
	vmovdqa oaep648_16xq(%rip), %ymm14
.endm

.macro BASEADD16 offset
	vmovdqu \offset(%rsi), %ymm3
	vpaddw  \offset(%rdx), %ymm3, %ymm3
	BARRETT16 %ymm3
	vmovdqu %ymm3, \offset(%rdi)
.endm

.global poly_baseadd_asm
.type poly_baseadd_asm, @function
poly_baseadd_asm:
	LOAD_BARRETT_CONSTS
	mov $10, %ecx

.p2align 5
.Lbaseadd_loop64:
	BASEADD16 0
	BASEADD16 32
	BASEADD16 64
	BASEADD16 96

	add $128, %rsi
	add $128, %rdx
	add $128, %rdi
	dec %ecx
	jnz .Lbaseadd_loop64

	vpxor   %ymm3, %ymm3, %ymm3
	vmovdqu (%rsi), %xmm3
	vpaddw  (%rdx), %xmm3, %xmm3
	BARRETT16 %ymm3
	vmovdqu %xmm3, (%rdi)

	vzeroupper
	ret
.size poly_baseadd_asm, .-poly_baseadd_asm

.section .note.GNU-stack,"",@progbits
